/* Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qman_low.h"

/* Compilation constants */
#define DQRR_MAXFILL	15
#define EQCR_ITHRESH	4	/* if EQCR congests, interrupt threshold */
#define IRQNAME		"QMan portal %d"
#define MAX_IRQNAME	16	/* big enough for "QMan portal %d" */

/* Lock/unlock frame queues, subject to the "LOCKED" flag. This is about
 * inter-processor locking only. Note, FQLOCK() is always called either under a
 * local_irq_save() or from interrupt context - hence there's no need for irq
 * protection (and indeed, attempting to nest irq-protection doesn't work, as
 * the "irq en/disable" machinery isn't recursive...). */
#define FQLOCK(fq) \
	do { \
		struct qman_fq *__fq478 = (fq); \
		if (fq_isset(__fq478, QMAN_FQ_FLAG_LOCKED)) \
			spin_lock(&__fq478->fqlock); \
	} while(0)
#define FQUNLOCK(fq) \
	do { \
		struct qman_fq *__fq478 = (fq); \
		if (fq_isset(__fq478, QMAN_FQ_FLAG_LOCKED)) \
			spin_unlock(&__fq478->fqlock); \
	} while(0)

static inline void fq_set(struct qman_fq *fq, u32 mask)
{
	set_bits(mask, &fq->flags);
}
static inline void fq_clear(struct qman_fq *fq, u32 mask)
{
	clear_bits(mask, &fq->flags);
}
static inline int fq_isset(struct qman_fq *fq, u32 mask)
{
	return fq->flags & mask;
}
static inline int fq_isclear(struct qman_fq *fq, u32 mask)
{
	return !(fq->flags & mask);
}

#define PORTAL_BITS_CI_PREFETCH	0x00020000	/* EQCR::CI prefetched */
#define PORTAL_BITS_RECOVERY	0x00040000	/* recovery mode */

struct qman_portal {
	struct qm_portal p;
	unsigned long bits; /* PORTAL_BITS_*** - dynamic, strictly internal */
	unsigned long irq_sources;
	u32 slowpoll;	/* only used when interrupts are off */
	struct qman_fq *vdqcr_owned; /* only 1 volatile dequeue at a time */
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	struct qman_fq *eqci_owned; /* only 1 enqueue WAIT_SYNC at a time */
#endif
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	spinlock_t sharing_lock; /* only used if is_shared */
	int is_shared;
	struct qman_portal *sharing_redirect;
#endif
	u32 sdqcr;
	int dqrr_disable_ref;
#ifdef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
	/* If we receive a DQRR or MR ring entry for a "null" FQ, ie. for which
	 * FQD::contextB is NULL rather than pointing to a FQ object, we use
	 * these handlers. (This is not considered a fast-path mechanism.) */
	struct qman_fq_cb null_cb;
#endif
	/* When the cpu-affine portal is activated, this is non-NULL */
	const struct qm_portal_config *config;
	/* This is needed for providing a non-NULL device to dma_map_***() */
	struct platform_device *pdev;
	struct dpa_rbtree retire_table;
	char irqname[MAX_IRQNAME];
	/* 2-element array. cgrs[0] is mask, cgrs[1] is snapshot. */
	struct qman_cgrs *cgrs;
	/* 256-element array, each is a linked-list of CSCN handlers. */
	struct list_head cgr_cbs[256];
	/* list lock */
	spinlock_t cgr_lock;
};

#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
#define PORTAL_IRQ_LOCK(p, irqflags) \
	do { \
		if ((p)->is_shared) \
			spin_lock_irqsave(&(p)->sharing_lock, irqflags); \
		else \
			local_irq_save(irqflags); \
	} while (0)
#define PORTAL_IRQ_UNLOCK(p, irqflags) \
	do { \
		if ((p)->is_shared) \
			spin_unlock_irqrestore(&(p)->sharing_lock, irqflags); \
		else \
			local_irq_restore(irqflags); \
	} while (0)
#else
#define PORTAL_IRQ_LOCK(p, irqflags) local_irq_save(irqflags)
#define PORTAL_IRQ_UNLOCK(p, irqflags) local_irq_restore(irqflags)
#endif

static cpumask_t affine_mask;
static DEFINE_SPINLOCK(affine_mask_lock);
static DEFINE_PER_CPU(struct qman_portal, qman_affine_portal);
/* "raw" gets the cpu-local struct whether it's a redirect or not. */
static inline struct qman_portal *get_raw_affine_portal(void)
{
	return &get_cpu_var(qman_affine_portal);
}
/* For ops that can redirect, this obtains the portal to use */
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
static inline struct qman_portal *get_affine_portal(void)
{
	struct qman_portal *p = get_raw_affine_portal();
	if (p->sharing_redirect)
		return p->sharing_redirect;
	return p;
}
#else
#define get_affine_portal() get_raw_affine_portal()
#endif
/* For every "get", there must be a "put" */
static inline void put_affine_portal(void)
{
	put_cpu_var(qman_affine_portal);
}

/* This gives a FQID->FQ lookup to cover the fact that we can't directly demux
 * retirement notifications (the fact they are sometimes h/w-consumed means that
 * contextB isn't always a s/w demux - and as we can't know which case it is
 * when looking at the notification, we have to use the slow lookup for all of
 * them). NB, it's possible to have multiple FQ objects refer to the same FQID
 * (though at most one of them should be the consumer), so this table isn't for
 * all FQs - FQs are added when retirement commands are issued, and removed when
 * they complete, which also massively reduces the size of this table. */
IMPLEMENT_DPA_RBTREE(fqtree, struct qman_fq, node, fqid);

/* This is what everything can wait on, even if it migrates to a different cpu
 * to the one whose affine portal it is waiting on. */
static DECLARE_WAIT_QUEUE_HEAD(affine_queue);

static inline int table_push_fq(struct qman_portal *p, struct qman_fq *fq)
{
	int ret = fqtree_push(&p->retire_table, fq);
	if (ret)
		pr_err("ERROR: double FQ-retirement %d\n", fq->fqid);
	return ret;
}

static inline void table_del_fq(struct qman_portal *p, struct qman_fq *fq)
{
	fqtree_del(&p->retire_table, fq);
}

static inline struct qman_fq *table_find_fq(struct qman_portal *p, u32 fqid)
{
	return fqtree_find(&p->retire_table, fqid);
}

#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
static void **qman_fq_lookup_table;
static size_t qman_fq_lookup_table_size;

int qman_setup_fq_lookup_table(size_t num_entries)
{
	num_entries++;
	/* Allocate 1 more entry since the first entry is not used */
	qman_fq_lookup_table = vmalloc((num_entries * sizeof(void *)));
	if (!qman_fq_lookup_table) {
		pr_err("QMan: Could not allocate fq lookup table\n");
		return -ENOMEM;
	}
	memset(qman_fq_lookup_table, 0, num_entries * sizeof(void *));
	qman_fq_lookup_table_size = num_entries;
	pr_info("QMan: Allocated lookup table at %p, entry count %lu\n",
			qman_fq_lookup_table,
			(unsigned long)qman_fq_lookup_table_size);
	return 0;
}

/* global structure that maintains fq object mapping */
static DEFINE_SPINLOCK(fq_hash_table_lock);

static int find_empty_fq_table_entry(u32 *entry, struct qman_fq *fq)
{
	u32 i;

	spin_lock(&fq_hash_table_lock);
	/* Can't use index zero because this has special meaning
	 * in context_b field. */
	for (i = 1; i < qman_fq_lookup_table_size; i++) {
		if (qman_fq_lookup_table[i] == NULL) {
			*entry = i;
			qman_fq_lookup_table[i] = fq;
			spin_unlock(&fq_hash_table_lock);
			return 0;
		}
	}
	spin_unlock(&fq_hash_table_lock);
	return -ENOMEM;
}

static void clear_fq_table_entry(u32 entry)
{
	spin_lock(&fq_hash_table_lock);
	BUG_ON(entry >= qman_fq_lookup_table_size);
	qman_fq_lookup_table[entry] = NULL;
	spin_unlock(&fq_hash_table_lock);
}

static inline struct qman_fq *get_fq_table_entry(u32 entry)
{
	BUG_ON(entry >= qman_fq_lookup_table_size);
	return  qman_fq_lookup_table[entry];
}
#endif

/* In the case that slow- and fast-path handling are both done by qman_poll()
 * (ie. because there is no interrupt handling), we ought to balance how often
 * we do the fast-path poll versus the slow-path poll. We'll use two decrementer
 * sources, so we call the fast poll 'n' times before calling the slow poll
 * once. The idle decrementer constant is used when the last slow-poll detected
 * no work to do, and the busy decrementer constant when the last slow-poll had
 * work to do. */
#define SLOW_POLL_IDLE   1000
#define SLOW_POLL_BUSY   10
static u32 __poll_portal_slow(struct qman_portal *p, u32 is);
static inline unsigned int __poll_portal_fast(struct qman_portal *p,
					unsigned int poll_limit);

#ifdef CONFIG_FSL_DPA_HAVE_IRQ
/* Portal interrupt handler */
static irqreturn_t portal_isr(__always_unused int irq, void *ptr)
{
	struct qman_portal *p = ptr;
	u32 clear = QM_DQAVAIL_MASK | p->irq_sources;
	u32 is = qm_isr_status_read(&p->p) & p->irq_sources;
	/* DQRR-handling if it's interrupt-driven */
	if (is & QM_PIRQ_DQRI)
		__poll_portal_fast(p, CONFIG_FSL_QMAN_POLL_LIMIT);
	/* Handling of anything else that's interrupt-driven */
	clear |= __poll_portal_slow(p, is);
	qm_isr_status_clear(&p->p, clear);
	return IRQ_HANDLED;
}
#endif

/* This inner version is used privately by qman_create_affine_portal(), as well
 * as by the exported qman_stop_dequeues(). */
static inline void qman_stop_dequeues_ex(struct qman_portal *p)
{
	unsigned long irqflags __maybe_unused;
	PORTAL_IRQ_LOCK(p, irqflags);
	if (!(p->dqrr_disable_ref++))
		qm_dqrr_set_maxfill(&p->p, 0);
	PORTAL_IRQ_UNLOCK(p, irqflags);
}

static int drain_mr_fqrni(struct qm_portal *p)
{
	const struct qm_mr_entry *msg;
loop:
	msg = qm_mr_current(p);
	if (!msg) {
		/* if MR was full and h/w had other FQRNI entries to produce, we
		 * need to allow it time to produce those entries once the
		 * existing entries are consumed. A worst-case situation
		 * (fully-loaded system) means h/w sequencers may have to do 3-4
		 * other things before servicing the portal's MR pump, each of
		 * which (if slow) may take ~50 qman cycles (which is ~200
		 * processor cycles). So rounding up and then multiplying this
		 * worst-case estimate by a factor of 10, just to be
		 * ultra-paranoid, goes as high as 10,000 cycles. NB, we consume
		 * one entry at a time, so h/w has an opportunity to produce new
		 * entries well before the ring has been fully consumed, so
		 * we're being *really* paranoid here. */
		u64 now, then = mfatb();
		do {
			now = mfatb();
		} while ((then + 10000) > now);
		msg = qm_mr_current(p);
		if (!msg)
			return 0;
	}
	if ((msg->verb & QM_MR_VERB_TYPE_MASK) != QM_MR_VERB_FQRNI)
		/* We aren't draining anything but FQRNIs */
		return -1;
	qm_mr_next(p);
	qm_mr_cci_consume(p, 1);
	goto loop;
}

/* this is called from qman_create_affine_portal() if not initialising in
 * recovery mode, otherwise from qman_recovery_exit_local() after recovery is
 * done. */
static void post_recovery(struct qman_portal *p __always_unused,
			const struct qm_portal_config *config)
{
	struct device_node *tmp_node, *node = config->node;
	/* Enable DMA on portal LIODNs (stashing) and those of its sub-nodes
	 * (Fman TX and SEC/PME accelerators, where available). */
	if (pamu_enable_liodn(node, -1))
		/* If there's a PAMU problem, best to continue anyway and let
		 * the corresponding traffic hit whatever problems it will hit,
		 * than to fail portal initialisation and trigger a crash in
		 * dependent code that has no relationship to the PAMU issue. */
		pr_err("Failed to enable portal LIODN %s\n",
			node->full_name);
	for_each_child_of_node(node, tmp_node)
		if (pamu_enable_liodn(tmp_node, -1))
			pr_err("Failed to enable portal LIODN %s\n",
				tmp_node->full_name);
}

struct qman_portal *qman_create_affine_portal(
			const struct qm_portal_config *config,
			const struct qman_cgrs *cgrs,
			const struct qman_fq_cb *null_cb,
			int recovery_mode)
{
	struct qman_portal *portal = get_raw_affine_portal();
	struct qm_portal *__p = &portal->p;
	char buf[16];
	int ret;
	u32 isdr;

	/* A criteria for calling this function (from qman_driver.c) is that
	 * we're already affine to the cpu and won't schedule onto another cpu.
	 * This means we can put_affine_portal() and yet continue to use
	 * "portal", which in turn means aspects of this routine can sleep. */
	put_affine_portal();
#ifndef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
	if (null_cb) {
		pr_err("Driver does not support 'NULL FQ' callbacks\n");
		return NULL;
	}
#endif
	/* prep the low-level portal struct with the mapped addresses from the
	 * config, everything that follows depends on it and "config" is more
	 * for (de)reference... */
	__p->addr.addr_ce = config->addr_virt[QM_ADDR_CE];
	__p->addr.addr_ci = config->addr_virt[QM_ADDR_CI];
	if (qm_eqcr_init(__p, qm_eqcr_pvb, qm_eqcr_cce)) {
		pr_err("Qman EQCR initialisation failed\n");
		goto fail_eqcr;
	}
#ifdef CONFIG_FSL_QMAN_PORTAL_DISABLEAUTO_DCA
#define QM_DQRR_CMODE qm_dqrr_cci
#else
#define QM_DQRR_CMODE qm_dqrr_cdc
#endif
	/* for recovery mode, don't enable stashing yet */
	if (qm_dqrr_init(__p, config, qm_dqrr_dpush, qm_dqrr_pvb,
			recovery_mode ?  qm_dqrr_cci : QM_DQRR_CMODE,
			DQRR_MAXFILL, recovery_mode)) {
		pr_err("Qman DQRR initialisation failed\n");
		goto fail_dqrr;
	}
	if (qm_mr_init(__p, qm_mr_pvb, qm_mr_cci)) {
		pr_err("Qman MR initialisation failed\n");
		goto fail_mr;
	}
	/* for recovery mode, quiesce SDQCR/VDQCR and drain DQRR+MR until h/w
	 * wraps up anything it was doing (5ms is ample idle time). */
	if (recovery_mode) {
		const struct qm_dqrr_entry *dq;
		const struct qm_mr_entry *msg;
		int idle = 0;
		/* quiesce SDQCR/VDQCR, then drain till h/w wraps up anything it
		 * was doing (5ms is more than enough to ensure it's done). */
		qm_dqrr_sdqcr_set(__p, 0);
		qm_dqrr_vdqcr_set(__p, 0);
drain_loop:
		qm_dqrr_pvb_update(__p, 0);
		dq = qm_dqrr_current(__p);
		qm_mr_pvb_update(__p);
		msg = qm_mr_current(__p);
		if (dq) {
			pr_warning("DQRR recovery: dumping dqrr %02x:%02x for "
				"FQID %d\n", dq->verb & QM_DQRR_VERB_MASK,
				dq->stat, dq->fqid);
			qm_dqrr_next(__p);
			qm_dqrr_cci_consume(__p, 1);
		}
		if (msg) {
			pr_warning("MR recovery: dumping msg 0x%02x for "
				"FQID %d\n", msg->verb & QM_MR_VERB_TYPE_MASK,
				msg->fq.fqid);
			qm_mr_next(__p);
			qm_mr_cci_consume(__p, 1);
		}
		if (!dq && !msg) {
			if (++idle < 5) {
				msleep(1);
				goto drain_loop;
			}
		} else {
			idle = 0;
			goto drain_loop;
		}
	}
	if (qm_mc_init(__p)) {
		pr_err("Qman MC initialisation failed\n");
		goto fail_mc;
	}
	if (qm_isr_init(__p)) {
		pr_err("Qman ISR initialisation failed\n");
		goto fail_isr;
	}
	/* static interrupt-gating controls */
	qm_dqrr_set_ithresh(__p, CONFIG_FSL_QMAN_PIRQ_DQRR_ITHRESH);
	qm_mr_set_ithresh(__p, CONFIG_FSL_QMAN_PIRQ_MR_ITHRESH);
	qm_isr_set_iperiod(__p, CONFIG_FSL_QMAN_PIRQ_IPERIOD);
	portal->cgrs = kmalloc(2 * sizeof(*cgrs), GFP_KERNEL);
	if (!portal->cgrs)
		goto fail_cgrs;
	/* initial snapshot is no-depletion */
	qman_cgrs_init(&portal->cgrs[1]);
	if (cgrs)
		portal->cgrs[0] = *cgrs;
	else
		/* if the given mask is NULL, assume all CGRs can be seen */
		qman_cgrs_fill(&portal->cgrs[0]);
	for (ret = 0; ret < __CGR_NUM; ret++)
		INIT_LIST_HEAD(&portal->cgr_cbs[ret]);
	spin_lock_init(&portal->cgr_lock);
	portal->bits = recovery_mode ? PORTAL_BITS_RECOVERY : 0;
	portal->slowpoll = 0;
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	portal->eqci_owned = NULL;
#endif
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	spin_lock_init(&portal->sharing_lock);
	portal->is_shared = config->public_cfg.is_shared;
	portal->sharing_redirect = NULL;
#endif
	portal->sdqcr = QM_SDQCR_SOURCE_CHANNELS | QM_SDQCR_COUNT_UPTO3 |
			QM_SDQCR_DEDICATED_PRECEDENCE | QM_SDQCR_TYPE_PRIO_QOS |
			QM_SDQCR_TOKEN_SET(0xab) | QM_SDQCR_CHANNELS_DEDICATED;
	portal->dqrr_disable_ref = 0;
#ifdef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
	if (null_cb)
		portal->null_cb = *null_cb;
	else
		memset(&portal->null_cb, 0, sizeof(*null_cb));
#endif
	sprintf(buf, "qportal-%d", config->public_cfg.channel);
	portal->pdev = platform_device_alloc(buf, -1);
	if (!portal->pdev)
		goto fail_devalloc;
	if (dma_set_mask(&portal->pdev->dev, DMA_BIT_MASK(40)))
		goto fail_devadd;
	ret = platform_device_add(portal->pdev);
	if (ret)
		goto fail_devadd;
	dpa_rbtree_init(&portal->retire_table);
	isdr = 0xffffffff;
	qm_isr_disable_write(__p, isdr);
	portal->irq_sources = 0;
	qm_isr_enable_write(__p, portal->irq_sources);
	qm_isr_status_clear(__p, 0xffffffff);
#ifdef CONFIG_FSL_DPA_HAVE_IRQ
	snprintf(portal->irqname, MAX_IRQNAME, IRQNAME, config->public_cfg.cpu);
	if (request_irq(config->public_cfg.irq, portal_isr,
			IRQF_NOBALANCING | IRQF_DISABLED, portal->irqname,
			portal)) {
		pr_err("request_irq() failed\n");
		goto fail_irq;
	}
	if (config->public_cfg.cpu != -1) {
		disable_irq(config->public_cfg.irq);
		irq_set_affinity(config->public_cfg.irq,
				 cpumask_of(config->public_cfg.cpu));
	}
	enable_irq(config->public_cfg.irq);

	if (recovery_mode) {
		qm_isr_inhibit(__p);
	} else {
		post_recovery(portal, config);
		qm_isr_uninhibit(__p);
	}
#endif
	/* Need EQCR to be empty before continuing */
	isdr ^= QM_PIRQ_EQCI;
	qm_isr_disable_write(__p, isdr);
	ret = qm_eqcr_get_fill(__p);
	if (ret) {
		pr_err("Qman EQCR unclean, need recovery\n");
		goto fail_eqcr_empty;
	}
	isdr ^= (QM_PIRQ_DQRI | QM_PIRQ_MRI);
	qm_isr_disable_write(__p, isdr);
	if (qm_dqrr_current(__p) != NULL) {
		pr_err("Qman DQRR unclean, need recovery\n");
		goto fail_dqrr_mr_empty;
	}
	if (qm_mr_current(__p) != NULL) {
		/* special handling, drain just in case it's a few FQRNIs */
		if (drain_mr_fqrni(__p)) {
			pr_err("Qman MR unclean, need recovery\n");
			goto fail_dqrr_mr_empty;
		}
	}
	/* Success */
	portal->config = config;
	spin_lock(&affine_mask_lock);
	cpumask_set_cpu(config->public_cfg.cpu, &affine_mask);
	spin_unlock(&affine_mask_lock);
	qm_isr_disable_write(__p, 0);
	/* Write a sane SDQCR */
	qm_dqrr_sdqcr_set(__p, recovery_mode ? 0 : portal->sdqcr);
	return portal;
fail_dqrr_mr_empty:
fail_eqcr_empty:
#ifdef CONFIG_FSL_DPA_HAVE_IRQ
	free_irq(config->public_cfg.irq, portal);
fail_irq:
#endif
	platform_device_del(portal->pdev);
fail_devadd:
	platform_device_put(portal->pdev);
fail_devalloc:
	if (portal->cgrs)
		kfree(portal->cgrs);
fail_cgrs:
	qm_isr_finish(__p);
fail_isr:
	qm_mc_finish(__p);
fail_mc:
	qm_mr_finish(__p);
fail_mr:
	qm_dqrr_finish(__p);
fail_dqrr:
	qm_eqcr_finish(__p);
fail_eqcr:
	put_affine_portal();
	return NULL;
}

/* These checks are BUG_ON()s because the driver is already supposed to avoid
 * these cases. */
struct qman_portal *qman_create_affine_slave(struct qman_portal *redirect)
{
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	struct qman_portal *p = get_raw_affine_portal();
	/* Check that we don't already have our own portal */
	BUG_ON(p->config);
	/* Check that we aren't already slaving to another portal */
	BUG_ON(p->is_shared);
	/* Check that 'redirect' is prepared to have us */
	BUG_ON(!redirect->config->public_cfg.is_shared);
	/* These are the only elements to initialise when redirecting */
	p->irq_sources = 0;
	p->sharing_redirect = redirect;
	put_affine_portal();
	return p;
#else
	BUG();
	return NULL;
#endif
}

const struct qm_portal_config *qman_destroy_affine_portal(void)
{
	/* We don't want to redirect if we're a slave, use "raw" */
	struct qman_portal *qm = get_raw_affine_portal();
	const struct qm_portal_config *pcfg;
	int cpu;
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (qm->sharing_redirect) {
		qm->sharing_redirect = NULL;
		put_affine_portal();
		return NULL;
	}
	qm->is_shared = 0;
#endif
	pcfg = qm->config;
	cpu = pcfg->public_cfg.cpu;
	/* NB we do this to "quiesce" EQCR. If we add enqueue-completions or
	 * something related to QM_PIRQ_EQCI, this may need fixing.
	 * Also, due to the prefetching model used for CI updates in the enqueue
	 * path, this update will only invalidate the CI cacheline *after*
	 * working on it, so we need to call this twice to ensure a full update
	 * irrespective of where the enqueue processing was at when the teardown
	 * began. */
	qm_eqcr_cce_update(&qm->p);
	qm_eqcr_cce_update(&qm->p);
#ifdef CONFIG_FSL_DPA_HAVE_IRQ
	free_irq(pcfg->public_cfg.irq, qm);
#endif
	kfree(qm->cgrs);
	qm_isr_finish(&qm->p);
	qm_mc_finish(&qm->p);
	qm_mr_finish(&qm->p);
	qm_dqrr_finish(&qm->p);
	qm_eqcr_finish(&qm->p);
	qm->config = NULL;
	spin_lock(&affine_mask_lock);
	cpumask_clear_cpu(cpu, &affine_mask);
	spin_unlock(&affine_mask_lock);
	put_affine_portal();
	return pcfg;
}

const struct qman_portal_config *qman_get_portal_config(void)
{
	struct qman_portal *p = get_affine_portal();
	const struct qman_portal_config *ret = &p->config->public_cfg;
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_get_portal_config);

#ifdef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
void qman_get_null_cb(struct qman_fq_cb *null_cb)
{
	struct qman_portal *p = get_affine_portal();
	*null_cb = p->null_cb;
	put_affine_portal();
}
EXPORT_SYMBOL(qman_get_null_cb);

void qman_set_null_cb(const struct qman_fq_cb *null_cb)
{
	struct qman_portal *p = get_affine_portal();
	p->null_cb = *null_cb;
	put_affine_portal();
}
EXPORT_SYMBOL(qman_set_null_cb);
#endif

/* Inline helper to reduce nesting in __poll_portal_slow() */
static inline void fq_state_change(struct qman_portal *p, struct qman_fq *fq,
				const struct qm_mr_entry *msg, u8 verb)
{
	FQLOCK(fq);
	switch(verb) {
	case QM_MR_VERB_FQRL:
		DPA_ASSERT(fq_isset(fq, QMAN_FQ_STATE_ORL));
		fq_clear(fq, QMAN_FQ_STATE_ORL);
		table_del_fq(p, fq);
		break;
	case QM_MR_VERB_FQRN:
		DPA_ASSERT((fq->state == qman_fq_state_parked) ||
			(fq->state == qman_fq_state_sched));
		DPA_ASSERT(fq_isset(fq, QMAN_FQ_STATE_CHANGING));
		fq_clear(fq, QMAN_FQ_STATE_CHANGING);
		if (msg->fq.fqs & QM_MR_FQS_NOTEMPTY)
			fq_set(fq, QMAN_FQ_STATE_NE);
		if (msg->fq.fqs & QM_MR_FQS_ORLPRESENT)
			fq_set(fq, QMAN_FQ_STATE_ORL);
		else
			table_del_fq(p, fq);
		fq->state = qman_fq_state_retired;
		break;
	case QM_MR_VERB_FQPN:
		DPA_ASSERT(fq->state == qman_fq_state_sched);
		DPA_ASSERT(fq_isclear(fq, QMAN_FQ_STATE_CHANGING));
		fq->state = qman_fq_state_parked;
	}
	FQUNLOCK(fq);
}

static u32 __poll_portal_slow(struct qman_portal *p, u32 is)
{
	const struct qm_mr_entry *msg;

	BUG_ON(p->bits & PORTAL_BITS_RECOVERY);

	if (is & QM_PIRQ_CSCI) {
		struct qman_cgrs rr, c;
		struct qm_mc_result *mcr;
		struct qman_cgr *cgr;
		int i;
		unsigned long irqflags __maybe_unused;

		spin_lock_irqsave(&p->cgr_lock, irqflags);
		qm_mc_start(&p->p);
		qm_mc_commit(&p->p, QM_MCC_VERB_QUERYCONGESTION);
		while (!(mcr = qm_mc_result(&p->p)))
			cpu_relax();
		/* mask out the ones I'm not interested in */
		qman_cgrs_and(&rr, (const struct qman_cgrs *)
			&mcr->querycongestion.state, &p->cgrs[0]);
		/* check previous snapshot for delta, enter/exit congestion */
		qman_cgrs_xor(&c, &rr, &p->cgrs[1]);
		/* update snapshot */
		qman_cgrs_cp(&p->cgrs[1], &rr);
		/* Invoke callback */
		qman_cgrs_for_each_1(i, &c)
			list_for_each_entry(cgr, &p->cgr_cbs[i], node) {
				if (cgr->cb)
					cgr->cb(p, cgr, qman_cgrs_get(&rr, i));
			}
		spin_unlock_irqrestore(&p->cgr_lock, irqflags);
	}

#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	if (is & QM_PIRQ_EQCI) {
		unsigned long irqflags;
		PORTAL_IRQ_LOCK(p, irqflags);
		p->eqci_owned = NULL;
		PORTAL_IRQ_UNLOCK(p, irqflags);
		wake_up(&affine_queue);
	}
#endif

	if (is & QM_PIRQ_EQRI) {
		unsigned long irqflags __maybe_unused;
		PORTAL_IRQ_LOCK(p, irqflags);
		qm_eqcr_cce_update(&p->p);
		qm_eqcr_set_ithresh(&p->p, 0);
		PORTAL_IRQ_UNLOCK(p, irqflags);
		wake_up(&affine_queue);
	}

	if (is & QM_PIRQ_MRI) {
		u8 num = 0;
mr_loop:
		qm_mr_pvb_update(&p->p);
		msg = qm_mr_current(&p->p);
		if (msg) {
			u8 verb = msg->verb & QM_MR_VERB_TYPE_MASK;
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
			struct qman_fq *fq;
#else
			struct qman_fq *fq = (void *)(uintptr_t)msg->ern.tag;
#endif
			if (verb == QM_MR_VERB_FQRNI) {
				; /* nada, we drop FQRNIs on the floor */
			} else if ((verb == QM_MR_VERB_FQRN) ||
					(verb == QM_MR_VERB_FQRL)) {
				/* It's retirement related - need a lookup */
				fq = table_find_fq(p, msg->fq.fqid);
				if (!fq)
					panic("unexpected FQ retirement");
				fq_state_change(p, fq, msg, verb);
				if (fq->cb.fqs)
					fq->cb.fqs(p, fq, msg);
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
			} else if (likely(msg->ern.tag)) {
				fq = get_fq_table_entry(msg->ern.tag);
#else
			} else if (likely(fq)) {
#endif
				/* As per the header note, this is the way to
				 * determine if it's a s/w ERN or not. */
				if (likely(!(verb & QM_MR_VERB_DC_ERN)))
					fq->cb.ern(p, fq, msg);
				else
					fq->cb.dc_ern(p, fq, msg);
			}
#ifdef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
			else {
				/* use portal default handlers for 'null's */
				if (likely(!(verb & QM_MR_VERB_DC_ERN)))
					p->null_cb.ern(p, NULL, msg);
				else if (verb == QM_MR_VERB_DC_ERN)
					p->null_cb.dc_ern(p, NULL, msg);
				else if (p->null_cb.fqs)
					p->null_cb.fqs(p, NULL, msg);
			}
#endif
			num++;
			qm_mr_next(&p->p);
			goto mr_loop;
		}
		qm_mr_cci_consume(&p->p, num);
	}

	return is & (QM_PIRQ_CSCI | QM_PIRQ_EQCI | QM_PIRQ_EQRI | QM_PIRQ_MRI);
}

/* remove some slowish-path stuff from the "fast path" and make sure it isn't
 * inlined. */
static noinline void clear_vdqcr(struct qman_portal *p, struct qman_fq *fq)
{
	p->vdqcr_owned = NULL;
	FQLOCK(fq);
	fq_clear(fq, QMAN_FQ_STATE_VDQCR);
	FQUNLOCK(fq);
	wake_up(&affine_queue);
}

/* Look: no locks, no irq_save()s, no preempt_disable()s! :-) The only states
 * that would conflict with other things if they ran at the same time on the
 * same cpu are;
 *
 *   (i) setting/clearing vdqcr_owned, and
 *  (ii) clearing the NE (Not Empty) flag.
 *
 * Both are safe. Because;
 *
 *   (i) this clearing can only occur after qman_volatile_dequeue() has set the
 *       vdqcr_owned field (which it does before setting VDQCR), and
 *       qman_volatile_dequeue() blocks interrupts and preemption while this is
 *       done so that we can't interfere.
 *  (ii) the NE flag is only cleared after qman_retire_fq() has set it, and as
 *       with (i) that API prevents us from interfering until it's safe.
 *
 * The good thing is that qman_volatile_dequeue() and qman_retire_fq() run far
 * less frequently (ie. per-FQ) than __poll_portal_fast() does, so the nett
 * advantage comes from this function not having to "lock" anything at all.
 *
 * Note also that the callbacks are invoked at points which are safe against the
 * above potential conflicts, but that this function itself is not re-entrant
 * (this is because the function tracks one end of each FIFO in the portal and
 * we do *not* want to lock that). So the consequence is that it is safe for
 * user callbacks to call into any Qman API *except* qman_poll() (as that's the
 * sole API that could be invoking the callback through this function).
 */
static inline unsigned int __poll_portal_fast(struct qman_portal *p,
					unsigned int poll_limit)
{
	const struct qm_dqrr_entry *dq;
	struct qman_fq *fq;
	enum qman_cb_dqrr_result res;
#ifdef CONFIG_FSL_QMAN_DQRR_PREFETCHING
	int coherent = (p->config->public_cfg.has_stashing);
#endif
	unsigned int limit = 0;

	BUG_ON(p->bits & PORTAL_BITS_RECOVERY);
loop:
#ifdef CONFIG_FSL_QMAN_DQRR_PREFETCHING
	qm_dqrr_pvb_update(&p->p, coherent);
#else
	qm_dqrr_pvb_update(&p->p, 1);
#endif
	dq = qm_dqrr_current(&p->p);
	if (!dq)
		goto done;
	if (dq->stat & QM_DQRR_STAT_UNSCHEDULED) {
		/* VDQCR: don't trust contextB as the FQ may have been
		 * configured for h/w consumption and we're draining it
		 * post-retirement. */
		fq = p->vdqcr_owned;
		/* We only set QMAN_FQ_STATE_NE when retiring, so we only need
		 * to check for clearing it when doing volatile dequeues. It's
		 * one less thing to check in the critical path (SDQCR). */
		if (dq->stat & QM_DQRR_STAT_FQ_EMPTY)
			fq_clear(fq, QMAN_FQ_STATE_NE);
		/* this is duplicated from the SDQCR code, but we have stuff to
		 * do before *and* after this callback, and we don't want
		 * multiple if()s in the critical path (SDQCR). */
		res = fq->cb.dqrr(p, fq, dq);
		if (res == qman_cb_dqrr_stop)
			goto done;
		/* Check for VDQCR completion */
		if (dq->stat & QM_DQRR_STAT_DQCR_EXPIRED)
			clear_vdqcr(p, fq);
	} else {
		/* SDQCR: contextB points to the FQ */
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
		fq = get_fq_table_entry(dq->contextB);
#else
		fq = (void *)(uintptr_t)dq->contextB;
#endif
#ifdef CONFIG_FSL_QMAN_NULL_FQ_DEMUX
		if (unlikely(!fq)) {
			/* use portal default handlers */
			res = p->null_cb.dqrr(p, NULL, dq);
			DPA_ASSERT(res == qman_cb_dqrr_consume);
			res = qman_cb_dqrr_consume;
		} else
#endif
		{
			/* Now let the callback do its stuff */
			res = fq->cb.dqrr(p, fq, dq);
			/* The callback can request that we exit without
			 * consuming this entry nor advancing; */
			if (res == qman_cb_dqrr_stop)
				goto done;
		}
	}
	/* Interpret 'dq' from a driver perspective. */
	/* Parking isn't possible unless HELDACTIVE was set. NB,
	 * FORCEELIGIBLE implies HELDACTIVE, so we only need to
	 * check for HELDACTIVE to cover both. */
	DPA_ASSERT((dq->stat & QM_DQRR_STAT_FQ_HELDACTIVE) ||
		(res != qman_cb_dqrr_park));
#ifdef CONFIG_FSL_QMAN_PORTAL_DISABLEAUTO_DCA
	if (res == qman_cb_dqrr_park)
		/* The only thing to do for non-DCA is the park-request */
		qm_dqrr_park_current(&p->p);
	/* Move forward */
	qm_dqrr_next(&p->p);
	qm_dqrr_cci_consume(&p->p, 1);
#else
	/* Defer just means "skip it, I'll consume it myself later on" */
	if (res != qman_cb_dqrr_defer)
		qm_dqrr_cdc_consume_1ptr(&p->p, dq, (res == qman_cb_dqrr_park));
	/* Move forward */
	qm_dqrr_next(&p->p);
#endif
	/* Entry processed and consumed, increment our counter. The callback can
	 * request that we exit after consuming the entry, and we also exit if
	 * we reach our processing limit, so loop back only if neither of these
	 * conditions is met. */
	if ((++limit < poll_limit) && (res != qman_cb_dqrr_consume_stop))
		goto loop;
done:
	return limit;
}

u32 qman_irqsource_get(void)
{
	/* "irqsource" and "poll" APIs mustn't redirect when sharing, they
	 * should shut the user out if they are not the primary CPU hosting the
	 * portal. That's why we use the "raw" interface. */
	struct qman_portal *p = get_raw_affine_portal();
	u32 ret = p->irq_sources & QM_PIRQ_VISIBLE;
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_irqsource_get);

int qman_irqsource_add(u32 bits __maybe_unused)
{
#ifdef CONFIG_FSL_DPA_HAVE_IRQ
	struct qman_portal *p = get_raw_affine_portal();
	int ret = 0;
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (p->sharing_redirect)
		ret = -EINVAL;
	else
#endif
	{
		__maybe_unused unsigned long irqflags;
		PORTAL_IRQ_LOCK(p, irqflags);
		set_bits(bits & QM_PIRQ_VISIBLE, &p->irq_sources);
		qm_isr_enable_write(&p->p, p->irq_sources);
		PORTAL_IRQ_UNLOCK(p, irqflags);
	}
	put_affine_portal();
	return ret;
#else
	pr_err("No Qman portal IRQ support, mustn't specify IRQ flags!");
	return -EINVAL;
#endif
}
EXPORT_SYMBOL(qman_irqsource_add);

int qman_irqsource_remove(u32 bits)
{
#ifdef CONFIG_FSL_DPA_HAVE_IRQ
	struct qman_portal *p = get_raw_affine_portal();
	__maybe_unused unsigned long irqflags;
	u32 ier;
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (p->sharing_redirect) {
		put_affine_portal();
		return -EINVAL;
	}
#endif
	/* Our interrupt handler only processes+clears status register bits that
	 * are in p->irq_sources. As we're trimming that mask, if one of them
	 * were to assert in the status register just before we remove it from
	 * the enable register, there would be an interrupt-storm when we
	 * release the IRQ lock. So we wait for the enable register update to
	 * take effect in h/w (by reading it back) and then clear all other bits
	 * in the status register. Ie. we clear them from ISR once it's certain
	 * IER won't allow them to reassert. */
	PORTAL_IRQ_LOCK(p, irqflags);
	bits &= QM_PIRQ_VISIBLE;
	clear_bits(bits, &p->irq_sources);
	qm_isr_enable_write(&p->p, p->irq_sources);
	ier = qm_isr_enable_read(&p->p);
	/* Using "~ier" (rather than "bits" or "~p->irq_sources") creates a
	 * data-dependency, ie. to protect against re-ordering. */
	qm_isr_status_clear(&p->p, ~ier);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return 0;
#else
	pr_err("No Qman portal IRQ support, mustn't specify IRQ flags!");
	return -EINVAL;
#endif
}
EXPORT_SYMBOL(qman_irqsource_remove);

const cpumask_t *qman_affine_cpus(void)
{
	return &affine_mask;
}
EXPORT_SYMBOL(qman_affine_cpus);

int qman_poll_dqrr(unsigned int limit)
{
	/* We need to fail when called for a "slave", so use "raw" */
	struct qman_portal *p = get_raw_affine_portal();
	int ret;
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (unlikely(p->sharing_redirect))
		ret = -EINVAL;
	else
#endif
	{
		BUG_ON(p->irq_sources & QM_PIRQ_DQRI);
		ret = __poll_portal_fast(p, limit);
	}
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_poll_dqrr);

u32 qman_poll_slow(void)
{
	/* We need to fail when called for a "slave", so use "raw" */
	struct qman_portal *p = get_raw_affine_portal();
	u32 ret;
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (unlikely(p->sharing_redirect))
		ret = (u32)-1;
	else
#endif
	{
		u32 is = qm_isr_status_read(&p->p) & ~p->irq_sources;
		ret = __poll_portal_slow(p, is);
		qm_isr_status_clear(&p->p, ret);
	}
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_poll_slow);

/* Legacy wrapper */
void qman_poll(void)
{
	struct qman_portal *p = get_raw_affine_portal();
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
	if (unlikely(p->sharing_redirect))
		goto done;
#endif
	if ((~p->irq_sources) & QM_PIRQ_SLOW) {
		if (!(p->slowpoll--)) {
			u32 is = qm_isr_status_read(&p->p) & ~p->irq_sources;
			u32 active = __poll_portal_slow(p, is);
			if (active) {
				qm_isr_status_clear(&p->p, active);
				p->slowpoll = SLOW_POLL_BUSY;
			} else
				p->slowpoll = SLOW_POLL_IDLE;
		}
	}
	if ((~p->irq_sources) & QM_PIRQ_DQRI)
		__poll_portal_fast(p, CONFIG_FSL_QMAN_POLL_LIMIT);
#ifdef CONFIG_FSL_DPA_PORTAL_SHARE
done:
#endif
	put_affine_portal();
}
EXPORT_SYMBOL(qman_poll);

/* Recovery processing. */
static int recovery_poll_mr(struct qman_portal *p, u32 fqid)
{
	const struct qm_mr_entry *msg;
	enum {
		wait_for_fqrn,
		wait_for_fqrl,
		done
	} state = wait_for_fqrn;
	u8 v, fqs = 0;

loop:
	qm_mr_pvb_update(&p->p);
	msg = qm_mr_current(&p->p);
	if (!msg) {
		cpu_relax();
		goto loop;
	}
	v = msg->verb & QM_MR_VERB_TYPE_MASK;
	/* all MR messages have "fqid" in the same place */
	if (msg->fq.fqid != fqid) {
ignore_msg:
		pr_warning("recovery_poll_mr(), ignoring msg 0x%02x for "
			"FQID %d\n", v, msg->fq.fqid);
		goto next_msg;
	}
	if (state == wait_for_fqrn) {
		if ((v != QM_MR_VERB_FQRN) && (v != QM_MR_VERB_FQRNI))
			goto ignore_msg;
		fqs = msg->fq.fqs;
		if (!(fqs & QM_MR_FQS_ORLPRESENT))
			state = done;
		else
			state = wait_for_fqrl;
	} else {
		if (v != QM_MR_VERB_FQRL)
			goto ignore_msg;
		state = done;
	}
next_msg:
	qm_mr_next(&p->p);
	qm_mr_cci_consume(&p->p, 1);
	if (state != done)
		goto loop;
	return (fqs & QM_MR_FQS_NOTEMPTY) ? 1 : 0;
}
static unsigned int recovery_poll_dqrr(struct qman_portal *p, u32 fqid)
{
	const struct qm_dqrr_entry *dq;
	u8 empty = 0, num_fds = 0;

loop:
	qm_dqrr_pvb_update(&p->p, 0);
	dq = qm_dqrr_current(&p->p);
	if (!dq) {
		cpu_relax();
		goto loop;
	}
	if (!(dq->stat & QM_DQRR_STAT_UNSCHEDULED)) {
ignore_dqrr:
		pr_warning("recovery_poll_dqrr(), ignoring dqrr %02x:%02x "
			"for FQID %d\n",
			dq->verb & QM_DQRR_VERB_MASK, dq->stat, dq->fqid);
		goto next_dqrr;
	}
	if (dq->fqid != fqid)
		goto ignore_dqrr;
	if (dq->stat & QM_DQRR_STAT_FD_VALID)
		num_fds++;
	if (dq->stat & QM_DQRR_STAT_FQ_EMPTY)
		empty = 1;
next_dqrr:
	qm_dqrr_next(&p->p);
	qm_dqrr_cci_consume(&p->p, 1);
	if (!empty)
		goto loop;
	return num_fds;
}
int qman_recovery_cleanup_fq(u32 fqid)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	int ret = 0;
	unsigned int num_fds = 0;
	const char *s;
	u8 state;

	/* Lock this whole flow down via the portal's "vdqcr" */
	PORTAL_IRQ_LOCK(p, irqflags);
	BUG_ON(!(p->bits & PORTAL_BITS_RECOVERY));
	if (p->vdqcr_owned)
		ret = -EBUSY;
	else
		p->vdqcr_owned = (void *)1;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	if (ret)
		goto out;

	/* Query the FQ's state */
	mcc = qm_mc_start(&p->p);
	mcc->queryfq.fqid = fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYFQ_NP);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_QUERYFQ_NP);
	if (mcr->result != QM_MCR_RESULT_OK) {
		ret = -EIO;
		goto out;
	}
	state = mcr->queryfq_np.state & QM_MCR_NP_STATE_MASK;

	/* OOS: nothing to do */
	if (state == QM_MCR_NP_STATE_OOS)
		goto out;
	/* Otherwise: must be retired */
	if (state != QM_MCR_NP_STATE_RETIRED) {
		mcc = qm_mc_start(&p->p);
		mcc->alterfq.fqid = fqid;
		qm_mc_commit(&p->p, QM_MCC_VERB_ALTER_RETIRE);
		while (!(mcr = qm_mc_result(&p->p)))
			cpu_relax();
		DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) ==
				QM_MCR_VERB_ALTER_RETIRE);
		if ((mcr->result != QM_MCR_RESULT_OK) &&
				(mcr->result != QM_MCR_RESULT_PENDING)) {
			ret = -EIO;
			goto out;
		}
		ret = recovery_poll_mr(p, fqid);
		if (!ret)
			/* FQ empty */
			goto oos;
	}
	/* Drain till empty */
	qm_dqrr_vdqcr_set(&p->p, fqid & 0x00ffffff);
	num_fds = recovery_poll_dqrr(p, fqid);

oos:
	mcc = qm_mc_start(&p->p);
	mcc->alterfq.fqid = fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_ALTER_OOS);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_ALTER_OOS);
	if (mcr->result != QM_MCR_RESULT_OK)
		ret = -EIO;
	/* done */
	s = (state == QM_MCR_NP_STATE_RETIRED) ? "retired" :
		(state == QM_MCR_NP_STATE_PARKED) ? "parked" : "scheduled";
	pr_info("Qman: %s FQID %d recovered (%d frames)\n", s, fqid, num_fds);
out:
	PORTAL_IRQ_LOCK(p, irqflags);
	p->vdqcr_owned = NULL;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_recovery_cleanup_fq);

/* called from qman_driver.c::qman_recovery_exit() only (if exporting, use
 * get_raw_affine_portal() and check for the "SLAVE" bit). */
void qman_recovery_exit_local(void)
{
	struct qman_portal *p = get_affine_portal();
	BUG_ON(!(p->bits & PORTAL_BITS_RECOVERY));
	/* Reinitialise DQRR using expected settings */
	qm_dqrr_finish(&p->p);
	post_recovery(p, p->config);
	clear_bits(PORTAL_BITS_RECOVERY, &p->bits);
	if (qm_dqrr_init(&p->p, p->config, qm_dqrr_dpush, qm_dqrr_pvb,
			QM_DQRR_CMODE, DQRR_MAXFILL, 0))
		panic("Qman DQRR initialisation failed, recovery broken");
	qm_dqrr_sdqcr_set(&p->p, p->sdqcr);
	qm_isr_status_clear(&p->p, 0xffffffff);
	qm_isr_uninhibit(&p->p);
	put_affine_portal();
}

void qman_stop_dequeues(void)
{
	struct qman_portal *p = get_affine_portal();
	qman_stop_dequeues_ex(p);
	put_affine_portal();
}
EXPORT_SYMBOL(qman_stop_dequeues);

void qman_start_dequeues(void)
{
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	PORTAL_IRQ_LOCK(p, irqflags);
	DPA_ASSERT(p->dqrr_disable_ref > 0);
	if (!(--p->dqrr_disable_ref))
		qm_dqrr_set_maxfill(&p->p, DQRR_MAXFILL);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
}
EXPORT_SYMBOL(qman_start_dequeues);

void qman_static_dequeue_add(u32 pools)
{
	unsigned long irqflags __maybe_unused;
	struct qman_portal *p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	pools &= p->config->public_cfg.pools;
	p->sdqcr |= pools;
	qm_dqrr_sdqcr_set(&p->p, p->sdqcr);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
}
EXPORT_SYMBOL(qman_static_dequeue_add);

void qman_static_dequeue_del(u32 pools)
{
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	PORTAL_IRQ_LOCK(p, irqflags);
	pools &= p->config->public_cfg.pools;
	p->sdqcr &= ~pools;
	qm_dqrr_sdqcr_set(&p->p, p->sdqcr);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
}
EXPORT_SYMBOL(qman_static_dequeue_del);

u32 qman_static_dequeue_get(void)
{
	struct qman_portal *p = get_affine_portal();
	u32 ret = p->sdqcr;
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_static_dequeue_get);

void qman_dca(struct qm_dqrr_entry *dq, int park_request)
{
	struct qman_portal *p = get_affine_portal();
	qm_dqrr_cdc_consume_1ptr(&p->p, dq, park_request);
	put_affine_portal();
}
EXPORT_SYMBOL(qman_dca);

/*******************/
/* Frame queue API */
/*******************/

static const char *mcr_result_str(u8 result)
{
	switch (result) {
	case QM_MCR_RESULT_NULL:
		return "QM_MCR_RESULT_NULL";
	case QM_MCR_RESULT_OK:
		return "QM_MCR_RESULT_OK";
	case QM_MCR_RESULT_ERR_FQID:
		return "QM_MCR_RESULT_ERR_FQID";
	case QM_MCR_RESULT_ERR_FQSTATE:
		return "QM_MCR_RESULT_ERR_FQSTATE";
	case QM_MCR_RESULT_ERR_NOTEMPTY:
		return "QM_MCR_RESULT_ERR_NOTEMPTY";
	case QM_MCR_RESULT_PENDING:
		return "QM_MCR_RESULT_PENDING";
	case QM_MCR_RESULT_ERR_BADCOMMAND:
		return "QM_MCR_RESULT_ERR_BADCOMMAND";
	}
	return "<unknown MCR result>";
}

int qman_create_fq(u32 fqid, u32 flags, struct qman_fq *fq)
{
	struct qm_fqd fqd;
	struct qm_mcr_queryfq_np np;
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p;
	unsigned long irqflags __maybe_unused;

	if (flags & QMAN_FQ_FLAG_DYNAMIC_FQID) {
		fqid = qm_fq_new();
		if (!fqid)
			return -ENOMEM;
	}
	spin_lock_init(&fq->fqlock);
	fq->fqid = fqid;
	fq->flags = flags;
	fq->state = qman_fq_state_oos;
	fq->cgr_groupid = 0;
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
	if (unlikely(find_empty_fq_table_entry(&fq->key, fq)))
		return -ENOMEM;
#endif
	if (!(flags & QMAN_FQ_FLAG_AS_IS) || (flags & QMAN_FQ_FLAG_NO_MODIFY))
		return 0;
	/* Everything else is AS_IS support */
	p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	mcc->queryfq.fqid = fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYFQ);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCC_VERB_QUERYFQ);
	if (mcr->result != QM_MCR_RESULT_OK) {
		pr_err("QUERYFQ failed: %s\n", mcr_result_str(mcr->result));
		goto err;
	}
	fqd = mcr->queryfq.fqd;
	mcc = qm_mc_start(&p->p);
	mcc->queryfq_np.fqid = fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYFQ_NP);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCC_VERB_QUERYFQ_NP);
	if (mcr->result != QM_MCR_RESULT_OK) {
		pr_err("QUERYFQ_NP failed: %s\n", mcr_result_str(mcr->result));
		goto err;
	}
	np = mcr->queryfq_np;
	/* Phew, have queryfq and queryfq_np results, stitch together
	 * the FQ object from those. */
	fq->cgr_groupid = fqd.cgid;
	switch (np.state & QM_MCR_NP_STATE_MASK) {
	case QM_MCR_NP_STATE_OOS:
		break;
	case QM_MCR_NP_STATE_RETIRED:
		fq->state = qman_fq_state_retired;
		if (np.frm_cnt)
			fq_set(fq, QMAN_FQ_STATE_NE);
		break;
	case QM_MCR_NP_STATE_TEN_SCHED:
	case QM_MCR_NP_STATE_TRU_SCHED:
	case QM_MCR_NP_STATE_ACTIVE:
		fq->state = qman_fq_state_sched;
		if (np.state & QM_MCR_NP_STATE_R)
			fq_set(fq, QMAN_FQ_STATE_CHANGING);
		break;
	case QM_MCR_NP_STATE_PARKED:
		fq->state = qman_fq_state_parked;
		break;
	default:
		DPA_ASSERT(NULL == "invalid FQ state");
	}
	if (fqd.fq_ctrl & QM_FQCTRL_CGE)
		fq->state |= QMAN_FQ_STATE_CGR_EN;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return 0;
err:
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (flags & QMAN_FQ_FLAG_DYNAMIC_FQID)
		qm_fq_free(fqid);
	return -EIO;
}
EXPORT_SYMBOL(qman_create_fq);

void qman_destroy_fq(struct qman_fq *fq, u32 flags __maybe_unused)
{
	/* We don't need to lock the FQ as it is a pre-condition that the FQ be
	 * quiesced. Instead, run some checks. */
	switch (fq->state) {
	case qman_fq_state_parked:
		DPA_ASSERT(flags & QMAN_FQ_DESTROY_PARKED);
	case qman_fq_state_oos:
		if (fq_isset(fq, QMAN_FQ_FLAG_DYNAMIC_FQID))
			qm_fq_free(fq->fqid);
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
		clear_fq_table_entry(fq->key);
#endif
		return;
	default:
		break;
	}
	DPA_ASSERT(NULL == "qman_free_fq() on unquiesced FQ!");
}
EXPORT_SYMBOL(qman_destroy_fq);

u32 qman_fq_fqid(struct qman_fq *fq)
{
	return fq->fqid;
}
EXPORT_SYMBOL(qman_fq_fqid);

void qman_fq_state(struct qman_fq *fq, enum qman_fq_state *state, u32 *flags)
{
	if (state)
		*state = fq->state;
	if (flags)
		*flags = fq->flags;
}
EXPORT_SYMBOL(qman_fq_state);

int qman_init_fq(struct qman_fq *fq, u32 flags, struct qm_mcc_initfq *opts)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p;
	unsigned long irqflags __maybe_unused;
	u8 res, myverb = (flags & QMAN_INITFQ_FLAG_SCHED) ?
		QM_MCC_VERB_INITFQ_SCHED : QM_MCC_VERB_INITFQ_PARKED;

	DPA_ASSERT((fq->state == qman_fq_state_oos) ||
		(fq->state == qman_fq_state_parked));
#ifdef CONFIG_FSL_DPA_CHECKING
	if (unlikely(fq_isset(fq, QMAN_FQ_FLAG_NO_MODIFY)))
		return -EINVAL;
#endif
	if (opts && (opts->we_mask & QM_INITFQ_WE_OAC)) {
		/* OAC not supported on rev1.0 */
		if (unlikely(qman_ip_rev == QMAN_REV10))
			return -EINVAL;
		/* And can't be set at the same time as TDTHRESH */
		if (opts->we_mask & QM_INITFQ_WE_TDTHRESH)
			return -EINVAL;
	}
	/* Issue an INITFQ_[PARKED|SCHED] management command */
	p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	FQLOCK(fq);
	if (unlikely((fq_isset(fq, QMAN_FQ_STATE_CHANGING)) ||
			((fq->state != qman_fq_state_oos) &&
				(fq->state != qman_fq_state_parked)))) {
		FQUNLOCK(fq);
		PORTAL_IRQ_UNLOCK(p, irqflags);
		put_affine_portal();
		return -EBUSY;
	}
	mcc = qm_mc_start(&p->p);
	if (opts)
		mcc->initfq = *opts;
	mcc->initfq.fqid = fq->fqid;
	mcc->initfq.count = 0;
	/* If INITFQ_FLAG_NULL is passed, contextB is set to zero. Otherwise,
	 * if the FQ does *not* have the TO_DCPORTAL flag, contextB is set as a
	 * demux pointer. Otherwise, TO_DCPORTAL is set, so the caller-provided
	 * value is allowed to stand, don't overwrite it. */
	if ((flags & QMAN_INITFQ_FLAG_NULL) ||
			fq_isclear(fq, QMAN_FQ_FLAG_TO_DCPORTAL)) {
		dma_addr_t phys_fq;
		mcc->initfq.we_mask |= QM_INITFQ_WE_CONTEXTB;
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
		mcc->initfq.fqd.context_b = (flags & QMAN_INITFQ_FLAG_NULL) ?
						0 : fq->key;
#else
		mcc->initfq.fqd.context_b = (flags & QMAN_INITFQ_FLAG_NULL) ?
						0 : (u32)(uintptr_t)fq;
#endif
		/* and the physical address - NB, if the user wasn't trying to
		 * set CONTEXTA, clear the stashing settings. */
		if (!(mcc->initfq.we_mask & QM_INITFQ_WE_CONTEXTA)) {
			mcc->initfq.we_mask |= QM_INITFQ_WE_CONTEXTA;
			memset(&mcc->initfq.fqd.context_a, 0,
				sizeof(mcc->initfq.fqd.context_a));
		} else {
			phys_fq = dma_map_single(&p->pdev->dev, fq, sizeof(*fq),
						DMA_TO_DEVICE);
			qm_fqd_stashing_set64(&mcc->initfq.fqd, phys_fq);
		}
	}
	if (flags & QMAN_INITFQ_FLAG_LOCAL) {
		mcc->initfq.fqd.dest.channel = p->config->public_cfg.channel;
		if (!(mcc->initfq.we_mask & QM_INITFQ_WE_DESTWQ)) {
			mcc->initfq.we_mask |= QM_INITFQ_WE_DESTWQ;
			mcc->initfq.fqd.dest.wq = 4;
		}
	}
	qm_mc_commit(&p->p, myverb);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == myverb);
	res = mcr->result;
	if (res != QM_MCR_RESULT_OK) {
		FQUNLOCK(fq);
		PORTAL_IRQ_UNLOCK(p, irqflags);
		put_affine_portal();
		return -EIO;
	}
	if (opts) {
		if (opts->we_mask & QM_INITFQ_WE_FQCTRL) {
			if (opts->fqd.fq_ctrl & QM_FQCTRL_CGE)
				fq_set(fq, QMAN_FQ_STATE_CGR_EN);
			else
				fq_clear(fq, QMAN_FQ_STATE_CGR_EN);
		}
		if (opts->we_mask & QM_INITFQ_WE_CGID)
			fq->cgr_groupid = opts->fqd.cgid;
	}
	fq->state = (flags & QMAN_INITFQ_FLAG_SCHED) ?
			qman_fq_state_sched : qman_fq_state_parked;
	FQUNLOCK(fq);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return 0;
}
EXPORT_SYMBOL(qman_init_fq);

int qman_schedule_fq(struct qman_fq *fq)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p;
	unsigned long irqflags __maybe_unused;
	int ret = 0;
	u8 res;

	DPA_ASSERT(fq->state == qman_fq_state_parked);
#ifdef CONFIG_FSL_DPA_CHECKING
	if (unlikely(fq_isset(fq, QMAN_FQ_FLAG_NO_MODIFY)))
		return -EINVAL;
#endif
	/* Issue a ALTERFQ_SCHED management command */
	p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	FQLOCK(fq);
	if (unlikely((fq_isset(fq, QMAN_FQ_STATE_CHANGING)) ||
			(fq->state != qman_fq_state_parked))) {
		ret = -EBUSY;
		goto out;
	}
	mcc = qm_mc_start(&p->p);
	mcc->alterfq.fqid = fq->fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_ALTER_SCHED);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_ALTER_SCHED);
	res = mcr->result;
	if (res != QM_MCR_RESULT_OK) {
		ret = -EIO;
		goto out;
	}
	fq->state = qman_fq_state_sched;
out:
	FQUNLOCK(fq);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_schedule_fq);

int qman_retire_fq(struct qman_fq *fq, u32 *flags)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p;
	unsigned long irqflags __maybe_unused;
	int rval;
	u8 res;

	DPA_ASSERT((fq->state == qman_fq_state_parked) ||
		(fq->state == qman_fq_state_sched));
#ifdef CONFIG_FSL_DPA_CHECKING
	if (unlikely(fq_isset(fq, QMAN_FQ_FLAG_NO_MODIFY)))
		return -EINVAL;
#endif
	p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	FQLOCK(fq);
	if (unlikely((fq_isset(fq, QMAN_FQ_STATE_CHANGING)) ||
			(fq->state == qman_fq_state_retired) ||
				(fq->state == qman_fq_state_oos))) {
		rval = -EBUSY;
		goto out;
	}
	rval = table_push_fq(p, fq);
	if (rval)
		goto out;
	mcc = qm_mc_start(&p->p);
	mcc->alterfq.fqid = fq->fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_ALTER_RETIRE);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_ALTER_RETIRE);
	res = mcr->result;
	/* "Elegant" would be to treat OK/PENDING the same way; set CHANGING,
	 * and defer the flags until FQRNI or FQRN (respectively) show up. But
	 * "Friendly" is to process OK immediately, and not set CHANGING. We do
	 * friendly, otherwise the caller doesn't necessarily have a fully
	 * "retired" FQ on return even if the retirement was immediate. However
	 * this does mean some code duplication between here and
	 * fq_state_change(). */
	if (likely(res == QM_MCR_RESULT_OK)) {
		rval = 0;
		/* Process 'fq' right away, we'll ignore FQRNI */
		if (mcr->alterfq.fqs & QM_MCR_FQS_NOTEMPTY)
			fq_set(fq, QMAN_FQ_STATE_NE);
		if (mcr->alterfq.fqs & QM_MCR_FQS_ORLPRESENT)
			fq_set(fq, QMAN_FQ_STATE_ORL);
		else
			table_del_fq(p, fq);
		if (flags)
			*flags = fq->flags;
		fq->state = qman_fq_state_retired;
		if (fq->cb.fqs) {
			/* Another issue with supporting "immediate" retirement
			 * is that we're forced to drop FQRNIs, because by the
			 * time they're seen it may already be "too late" (the
			 * fq may have been OOS'd and free()'d already). But if
			 * the upper layer wants a callback whether it's
			 * immediate or not, we have to fake a "MR" entry to
			 * look like an FQRNI... */
			struct qm_mr_entry msg;
			msg.verb = QM_MR_VERB_FQRNI;
			msg.fq.fqs = mcr->alterfq.fqs;
			msg.fq.fqid = fq->fqid;
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
			msg.fq.contextB = fq->key;
#else
			msg.fq.contextB = (u32)(uintptr_t)fq;
#endif
			fq->cb.fqs(p, fq, &msg);
		}
	} else if (res == QM_MCR_RESULT_PENDING) {
		rval = 1;
		fq_set(fq, QMAN_FQ_STATE_CHANGING);
	} else {
		rval = -EIO;
		table_del_fq(p, fq);
	}
out:
	FQUNLOCK(fq);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return rval;
}
EXPORT_SYMBOL(qman_retire_fq);

int qman_oos_fq(struct qman_fq *fq)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p;
	unsigned long irqflags __maybe_unused;
	int ret = 0;
	u8 res;

	DPA_ASSERT(fq->state == qman_fq_state_retired);
#ifdef CONFIG_FSL_DPA_CHECKING
	if (unlikely(fq_isset(fq, QMAN_FQ_FLAG_NO_MODIFY)))
		return -EINVAL;
#endif
	p = get_affine_portal();
	PORTAL_IRQ_LOCK(p, irqflags);
	FQLOCK(fq);
	if (unlikely((fq_isset(fq, QMAN_FQ_STATE_BLOCKOOS)) ||
			(fq->state != qman_fq_state_retired))) {
		ret = -EBUSY;
		goto out;
	}
	mcc = qm_mc_start(&p->p);
	mcc->alterfq.fqid = fq->fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_ALTER_OOS);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_ALTER_OOS);
	res = mcr->result;
	if (res != QM_MCR_RESULT_OK) {
		ret = -EIO;
		goto out;
	}
	fq->state = qman_fq_state_oos;
out:
	FQUNLOCK(fq);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_oos_fq);

int qman_query_fq(struct qman_fq *fq, struct qm_fqd *fqd)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;

	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	mcc->queryfq.fqid = fq->fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYFQ);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_QUERYFQ);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*fqd = mcr->queryfq.fqd;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK)
		return -EIO;
	return 0;
}
EXPORT_SYMBOL(qman_query_fq);

int qman_query_fq_np(struct qman_fq *fq, struct qm_mcr_queryfq_np *np)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;

	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	mcc->queryfq.fqid = fq->fqid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYFQ_NP);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCR_VERB_QUERYFQ_NP);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*np = mcr->queryfq_np;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK)
		return -EIO;
	return 0;
}
EXPORT_SYMBOL(qman_query_fq_np);

int qman_query_wq(u8 query_dedicated, struct qm_mcr_querywq *wq)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res, myverb;

	PORTAL_IRQ_LOCK(p, irqflags);
	myverb = (query_dedicated) ? QM_MCR_VERB_QUERYWQ_DEDICATED :
				 QM_MCR_VERB_QUERYWQ;
	mcc = qm_mc_start(&p->p);
	mcc->querywq.channel.id = wq->channel.id;
	qm_mc_commit(&p->p, myverb);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == myverb);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*wq = mcr->querywq;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK) {
		pr_err("QUERYWQ failed: %s\n", mcr_result_str(res));
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(qman_query_wq);

int qman_testwrite_cgr(struct qman_cgr *cgr, u64 i_bcnt,
			struct qm_mcr_cgrtestwrite *result)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;

	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	mcc->cgrtestwrite.cgid = cgr->cgrid;
	mcc->cgrtestwrite.i_bcnt_hi = (u8)(i_bcnt >> 32);
	mcc->cgrtestwrite.i_bcnt_lo = (u32)i_bcnt;
	qm_mc_commit(&p->p, QM_MCC_VERB_CGRTESTWRITE);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCC_VERB_CGRTESTWRITE);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*result = mcr->cgrtestwrite;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK) {
		pr_err("CGR TEST WRITE failed: %s\n", mcr_result_str(res));
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(qman_testwrite_cgr);

int qman_query_cgr(struct qman_cgr *cgr, struct qm_mcr_querycgr *cgrd)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;

	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	mcc->querycgr.cgid = cgr->cgrid;
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYCGR);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == QM_MCC_VERB_QUERYCGR);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*cgrd = mcr->querycgr;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK) {
		pr_err("QUERY_CGR failed: %s\n", mcr_result_str(res));
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(qman_query_cgr);

int qman_query_congestion(struct qm_mcr_querycongestion *congestion)
{
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;

	PORTAL_IRQ_LOCK(p, irqflags);
	qm_mc_start(&p->p);
	qm_mc_commit(&p->p, QM_MCC_VERB_QUERYCONGESTION);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) ==
			QM_MCC_VERB_QUERYCONGESTION);
	res = mcr->result;
	if (res == QM_MCR_RESULT_OK)
		*congestion = mcr->querycongestion;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	if (res != QM_MCR_RESULT_OK) {
		pr_err("QUERY_CONGESTION failed: %s\n", mcr_result_str(res));
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(qman_query_congestion);

/* internal function used as a wait_event() expression */
static int set_vdqcr(struct qman_portal **p, struct qman_fq *fq, u32 vdqcr)
{
	unsigned long irqflags __maybe_unused;
	int ret = -EBUSY;
	*p = get_affine_portal();
	PORTAL_IRQ_LOCK(*p, irqflags);
	if (!(*p)->vdqcr_owned) {
		FQLOCK(fq);
		if (fq_isset(fq, QMAN_FQ_STATE_VDQCR))
			goto escape;
		fq_set(fq, QMAN_FQ_STATE_VDQCR);
		FQUNLOCK(fq);
		(*p)->vdqcr_owned = fq;
		ret = 0;
	}
escape:
	PORTAL_IRQ_UNLOCK(*p, irqflags);
	if (!ret)
		qm_dqrr_vdqcr_set(&(*p)->p, vdqcr);
	put_affine_portal();
	return ret;
}

#ifdef CONFIG_FSL_DPA_CAN_WAIT
static int wait_vdqcr_start(struct qman_portal **p, struct qman_fq *fq,
				u32 vdqcr, u32 flags)
{
	int ret = 0;
	if (flags & QMAN_VOLATILE_FLAG_WAIT_INT)
		ret = wait_event_interruptible(affine_queue,
				!(ret = set_vdqcr(p, fq, vdqcr)));
	else
		wait_event(affine_queue, !(ret = set_vdqcr(p, fq, vdqcr)));
	return ret;
}
#endif

int qman_volatile_dequeue(struct qman_fq *fq, u32 flags __maybe_unused,
				u32 vdqcr)
{
	struct qman_portal *p;
	int ret;

	DPA_ASSERT(!fq || (fq->state == qman_fq_state_parked) ||
			(fq->state == qman_fq_state_retired));
	DPA_ASSERT(!fq || !(vdqcr & QM_VDQCR_FQID_MASK));
	DPA_ASSERT(!fq || !fq_isset(fq, QMAN_FQ_STATE_VDQCR));
	if (fq)
		vdqcr = (vdqcr & ~QM_VDQCR_FQID_MASK) | fq->fqid;
#ifdef CONFIG_FSL_DPA_CAN_WAIT
	if (flags & QMAN_VOLATILE_FLAG_WAIT)
		ret = wait_vdqcr_start(&p, fq, vdqcr, flags);
	else
#endif
		ret = set_vdqcr(&p, fq, vdqcr);
	if (ret)
		return ret;
	/* VDQCR is set */
#ifdef CONFIG_FSL_DPA_CAN_WAIT
	if (flags & QMAN_VOLATILE_FLAG_FINISH) {
		if (flags & QMAN_VOLATILE_FLAG_WAIT_INT)
			/* NB: don't propagate any error - the caller wouldn't
			 * know whether the VDQCR was issued or not. A signal
			 * could arrive after returning anyway, so the caller
			 * can check signal_pending() if that's an issue. */
			wait_event_interruptible(affine_queue,
				!fq_isset(fq, QMAN_FQ_STATE_VDQCR));
		else
			wait_event(affine_queue,
				!fq_isset(fq, QMAN_FQ_STATE_VDQCR));
	}
#endif
	return 0;
}
EXPORT_SYMBOL(qman_volatile_dequeue);

static noinline void update_eqcr_ci(struct qman_portal *p, u8 avail)
{
	if (avail)
		qm_eqcr_cce_prefetch(&p->p);
	else
		qm_eqcr_cce_update(&p->p);
}

int qman_eqcr_is_empty(void)
{
	unsigned long irqflags __maybe_unused;
	struct qman_portal *p = get_affine_portal();
	u8 avail;

	PORTAL_IRQ_LOCK(p, irqflags);
	update_eqcr_ci(p, 0);
	avail = qm_eqcr_get_fill(&p->p);
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return (avail == 0);
}
EXPORT_SYMBOL(qman_eqcr_is_empty);

static inline struct qm_eqcr_entry *try_eq_start(struct qman_portal **p,
					unsigned long *irqflags __maybe_unused,
					struct qman_fq *fq,
					const struct qm_fd *fd,
					u32 flags)
{
	struct qm_eqcr_entry *eq;
	u8 avail;

	*p = get_affine_portal();
	PORTAL_IRQ_LOCK(*p, (*irqflags));
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	if (unlikely((flags & QMAN_ENQUEUE_FLAG_WAIT) &&
			(flags & QMAN_ENQUEUE_FLAG_WAIT_SYNC))) {
		if ((*p)->eqci_owned) {
			PORTAL_IRQ_UNLOCK(*p, (*irqflags));
			put_affine_portal();
			return NULL;
		}
		(*p)->eqci_owned = fq;
	}
#endif
	avail = qm_eqcr_get_avail(&(*p)->p);
	if (avail < 2)
		update_eqcr_ci(*p, avail);
	eq = qm_eqcr_start(&(*p)->p);
	if (unlikely(!eq)) {
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
		if (unlikely((flags & QMAN_ENQUEUE_FLAG_WAIT) &&
				(flags & QMAN_ENQUEUE_FLAG_WAIT_SYNC)))
			(*p)->eqci_owned = NULL;
#endif
		PORTAL_IRQ_UNLOCK(*p, (*irqflags));
		put_affine_portal();
		return NULL;
	}
	if (flags & QMAN_ENQUEUE_FLAG_DCA)
		eq->dca = QM_EQCR_DCA_ENABLE |
			((flags & QMAN_ENQUEUE_FLAG_DCA_PARK) ?
					QM_EQCR_DCA_PARK : 0) |
			((flags >> 8) & QM_EQCR_DCA_IDXMASK);
	eq->fqid = fq->fqid;
#ifdef CONFIG_FSL_QMAN_FQ_LOOKUP
	eq->tag = fq->key;
#else
	eq->tag = (u32)(uintptr_t)fq;
#endif
	/* From p4080 rev1 -> rev2, the FD struct's address went from 48-bit to
	 * 40-bit but rev1 chips will still interpret it as 48-bit, meaning we
	 * have to scrub the upper 8-bits, just in case the user left noise in
	 * there. Doing this selectively via a run-time check of the h/w
	 * revision (as we do for most errata, for example) is too slow in this
	 * critical path code. The most inexpensive way to handle this is just
	 * to reinterpret the FD as 4 32-bit words and to mask the first word
	 * appropriately, irrespecitive of the h/w revision. The struct fields
	 * corresponding to this word are;
	 *     u8 dd:2;
	 *     u8 liodn_offset:6;
	 *     u8 bpid;
	 *     u8 eliodn_offset:4;
	 *     u8 __reserved:4;
	 *     u8 addr_hi;
	 * So we mask this word with 0xc0ff00ff, which implicitly scrubs out
	 * liodn_offset, eliodn_offset, and __reserved - the latter two fields
	 * are interpreted as the 8 msbits of the 48-bit address in the case of
	 * rev1.
	 */
	{
		const u32 *src = (const u32 *)fd;
		u32 *dest = (u32 *)&eq->fd;
		dest[0] = src[0] & 0xc0ff00ff;
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
	}
	return eq;
}

#ifdef CONFIG_FSL_DPA_CAN_WAIT
static noinline struct qm_eqcr_entry *__wait_eq_start(struct qman_portal **p,
					unsigned long *irqflags __maybe_unused,
					struct qman_fq *fq,
					const struct qm_fd *fd,
					u32 flags)
{
	struct qm_eqcr_entry *eq = try_eq_start(p, irqflags, fq, fd, flags);
	if (!eq)
		qm_eqcr_set_ithresh(&(*p)->p, EQCR_ITHRESH);
	return eq;
}
static noinline struct qm_eqcr_entry *wait_eq_start(struct qman_portal **p,
					unsigned long *irqflags __maybe_unused,
					struct qman_fq *fq,
					const struct qm_fd *fd,
					u32 flags)
{
	struct qm_eqcr_entry *eq;
	if (flags & QMAN_ENQUEUE_FLAG_WAIT_INT)
		wait_event_interruptible(affine_queue,
			(eq = __wait_eq_start(p, irqflags, fq, fd, flags)));
	else
		wait_event(affine_queue,
			(eq = __wait_eq_start(p, irqflags, fq, fd, flags)));
	return eq;
}
#endif

int qman_enqueue(struct qman_fq *fq, const struct qm_fd *fd, u32 flags)
{
	struct qman_portal *p;
	struct qm_eqcr_entry *eq;
	unsigned long irqflags __maybe_unused;

#ifdef CONFIG_FSL_DPA_CAN_WAIT
	if (flags & QMAN_ENQUEUE_FLAG_WAIT)
		eq = wait_eq_start(&p, &irqflags, fq, fd, flags);
	else
#endif
	eq = try_eq_start(&p, &irqflags, fq, fd, flags);
	if (!eq)
		return -EBUSY;
	/* Note: QM_EQCR_VERB_INTERRUPT == QMAN_ENQUEUE_FLAG_WAIT_SYNC */
	qm_eqcr_pvb_commit(&p->p, QM_EQCR_VERB_CMD_ENQUEUE |
		(flags & (QM_EQCR_VERB_COLOUR_MASK | QM_EQCR_VERB_INTERRUPT)));
	/* Factor the below out, it's used from qman_enqueue_orp() too */
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	if (unlikely((flags & QMAN_ENQUEUE_FLAG_WAIT) &&
			(flags & QMAN_ENQUEUE_FLAG_WAIT_SYNC))) {
		if (flags & QMAN_ENQUEUE_FLAG_WAIT_INT)
			wait_event_interruptible(affine_queue,
					(p->eqci_owned != fq));
		else
			wait_event(affine_queue, (p->eqci_owned != fq));
	}
#endif
	return 0;
}
EXPORT_SYMBOL(qman_enqueue);

int qman_enqueue_orp(struct qman_fq *fq, const struct qm_fd *fd, u32 flags,
			struct qman_fq *orp, u16 orp_seqnum)
{
	struct qman_portal *p;
	struct qm_eqcr_entry *eq;
	unsigned long irqflags __maybe_unused;

#ifdef CONFIG_FSL_DPA_CAN_WAIT
	if (flags & QMAN_ENQUEUE_FLAG_WAIT)
		eq = wait_eq_start(&p, &irqflags, fq, fd, flags);
	else
#endif
	eq = try_eq_start(&p, &irqflags, fq, fd, flags);
	if (!eq)
		return -EBUSY;
	/* Process ORP-specifics here */
	if (flags & QMAN_ENQUEUE_FLAG_NLIS)
		orp_seqnum |= QM_EQCR_SEQNUM_NLIS;
	else {
		orp_seqnum &= ~QM_EQCR_SEQNUM_NLIS;
		if (flags & QMAN_ENQUEUE_FLAG_NESN)
			orp_seqnum |= QM_EQCR_SEQNUM_NESN;
		else
			/* No need to check 4 QMAN_ENQUEUE_FLAG_HOLE */
			orp_seqnum &= ~QM_EQCR_SEQNUM_NESN;
	}
	eq->seqnum = orp_seqnum;
	eq->orp = orp->fqid;
	/* Note: QM_EQCR_VERB_INTERRUPT == QMAN_ENQUEUE_FLAG_WAIT_SYNC */
	qm_eqcr_pvb_commit(&p->p, QM_EQCR_VERB_ORP |
		((flags & (QMAN_ENQUEUE_FLAG_HOLE | QMAN_ENQUEUE_FLAG_NESN)) ?
				0 : QM_EQCR_VERB_CMD_ENQUEUE) |
		(flags & (QM_EQCR_VERB_COLOUR_MASK | QM_EQCR_VERB_INTERRUPT)));
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
#ifdef CONFIG_FSL_DPA_CAN_WAIT_SYNC
	if (unlikely((flags & QMAN_ENQUEUE_FLAG_WAIT) &&
			(flags & QMAN_ENQUEUE_FLAG_WAIT_SYNC))) {
		if (flags & QMAN_ENQUEUE_FLAG_WAIT_INT)
			wait_event_interruptible(affine_queue,
					(p->eqci_owned != fq));
		else
			wait_event(affine_queue, (p->eqci_owned != fq));
	}
#endif
	return 0;
}
EXPORT_SYMBOL(qman_enqueue_orp);

int qman_modify_cgr(struct qman_cgr *cgr, u32 flags,
			struct qm_mcc_initcgr *opts)
{
	struct qm_mc_command *mcc;
	struct qm_mc_result *mcr;
	struct qman_portal *p = get_affine_portal();
	unsigned long irqflags __maybe_unused;
	u8 res;
	u8 verb = QM_MCC_VERB_MODIFYCGR;

	/* frame mode not supported on rev1.0 */
	if (unlikely(qman_ip_rev == QMAN_REV10)) {
		if (opts && (opts->we_mask & QM_CGR_WE_MODE) &&
				opts->cgr.mode == QMAN_CGR_MODE_FRAME) {
			put_affine_portal();
			return -EIO;
		}
	}
	PORTAL_IRQ_LOCK(p, irqflags);
	mcc = qm_mc_start(&p->p);
	if (opts)
		mcc->initcgr = *opts;
	mcc->initcgr.cgid = cgr->cgrid;
	if (flags & QMAN_CGR_FLAG_USE_INIT)
		verb = QM_MCC_VERB_INITCGR;
	qm_mc_commit(&p->p, verb);
	while (!(mcr = qm_mc_result(&p->p)))
		cpu_relax();
	DPA_ASSERT((mcr->verb & QM_MCR_VERB_MASK) == verb);
	res = mcr->result;
	PORTAL_IRQ_UNLOCK(p, irqflags);
	put_affine_portal();
	return (res == QM_MCR_RESULT_OK) ? 0 : -EIO;
}
EXPORT_SYMBOL(qman_modify_cgr);

#define TARG_MASK(n) (0x80000000 >> (n->config->public_cfg.channel - \
					qm_channel_swportal0))

int qman_create_cgr(struct qman_cgr *cgr, u32 flags,
			struct qm_mcc_initcgr *opts)
{
	unsigned long irqflags __maybe_unused;
	struct qm_mcr_querycgr cgr_state;
	struct qm_mcc_initcgr local_opts;
	int ret;
	struct qman_portal *p;

	/* We have to check that the provided CGRID is within the limits of the
	 * data-structures, for obvious reasons. However we'll let h/w take
	 * care of determining whether it's within the limits of what exists on
	 * the SoC. */
	if (cgr->cgrid >= __CGR_NUM)
		return -EINVAL;

	p = get_affine_portal();

	memset(&local_opts, 0, sizeof(struct qm_mcc_initcgr));
	cgr->chan = p->config->public_cfg.channel;
	spin_lock_irqsave(&p->cgr_lock, irqflags);

	/* if no opts specified and I'm not the first for this portal, just add
	 * to the list */
	if ((opts == NULL) && !list_empty(&p->cgr_cbs[cgr->cgrid]))
		goto add_list;

	ret = qman_query_cgr(cgr, &cgr_state);
	if (ret)
		goto release_lock;
	if (opts)
		local_opts = *opts;
	/* Overwrite TARG */
	local_opts.cgr.cscn_targ = cgr_state.cgr.cscn_targ | TARG_MASK(p);
	local_opts.we_mask |= QM_CGR_WE_CSCN_TARG;

	/* send init if flags indicate so */
	if (opts && (flags & QMAN_CGR_FLAG_USE_INIT))
		ret = qman_modify_cgr(cgr, QMAN_CGR_FLAG_USE_INIT, &local_opts);
	else
		ret = qman_modify_cgr(cgr, 0, &local_opts);
	if (ret)
		goto release_lock;
add_list:
	list_add(&cgr->node, &p->cgr_cbs[cgr->cgrid]);

	/* Determine if newly added object requires its callback to be called */
	ret = qman_query_cgr(cgr, &cgr_state);
	if (ret) {
		/* we can't go back, so proceed and return success, but screen
		 * and wail to the log file */
		pr_crit("CGR HW state partially modified\n");
		ret = 0;
		goto release_lock;
	}
	if (cgr->cb && cgr_state.cgr.cscn_en && qman_cgrs_get(&p->cgrs[1],
							cgr->cgrid))
		cgr->cb(p, cgr, 1);
release_lock:
	spin_unlock_irqrestore(&p->cgr_lock, irqflags);
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_create_cgr);

int qman_delete_cgr(struct qman_cgr *cgr)
{
	unsigned long irqflags __maybe_unused;
	struct qm_mcr_querycgr cgr_state;
	struct qm_mcc_initcgr local_opts;
	int ret = 0;
	struct qman_portal *p = get_affine_portal();

	if (cgr->chan != p->config->public_cfg.channel) {
		pr_crit("Attempting to delete cgr from different portal "
			"than it was create: create 0x%x, delete 0x%x\n",
			cgr->chan, p->config->public_cfg.channel);
		ret = -EINVAL;
		goto put_portal;
	}
	memset(&local_opts, 0, sizeof(struct qm_mcc_initcgr));
	spin_lock_irqsave(&p->cgr_lock, irqflags);
	list_del(&cgr->node);
	/* If last in list, CSCN_TARG must be set accordingly */
	if (!list_empty(&p->cgr_cbs[cgr->cgrid]))
		goto release_lock;
	ret = qman_query_cgr(cgr, &cgr_state);
	if (ret)  {
		/* add back to the list */
		list_add(&cgr->node, &p->cgr_cbs[cgr->cgrid]);
		goto release_lock;
	}
	/* Overwrite TARG */
	local_opts.we_mask = QM_CGR_WE_CSCN_TARG;
	local_opts.cgr.cscn_targ = cgr_state.cgr.cscn_targ & ~(TARG_MASK(p));
	ret = qman_modify_cgr(cgr, 0, &local_opts);
	if (ret)
		/* add back to the list */
		list_add(&cgr->node, &p->cgr_cbs[cgr->cgrid]);
release_lock:
	spin_unlock_irqrestore(&p->cgr_lock, irqflags);
put_portal:
	put_affine_portal();
	return ret;
}
EXPORT_SYMBOL(qman_delete_cgr);

