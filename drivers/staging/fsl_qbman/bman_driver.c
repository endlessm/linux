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

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "bman_private.h"

/*
 * Global variables of the max portal/pool number this bman version supported
 */
u16 bman_ip_rev;
EXPORT_SYMBOL(bman_ip_rev);
u16 bman_pool_max;
EXPORT_SYMBOL(bman_pool_max);

/*****************/
/* Portal driver */
/*****************/

/* Compatibility behaviour (when no bpool-range is present) is that;
 * (a) on a control plane, all pools that aren't explicitly mentioned in the dtb
 *     are available for allocation,
 * (b) on a non-control plane, there is never any allocation possible at all.
 *
 * New behaviour is that if any "fsl,bpool-range" nodes are declared, they
 * declare what is available for allocation, and this is independent of which
 * pools are/aren't mentioned in the dtb. Eg. to stipulate that no allocation is
 * possible, a fsl,bpool-range should be specified with zero items in it.
 *
 * This "pools" struct contains the allocator, and "explicit allocator"
 * indicates whether the range is seeded explicitly (via at least one range) or
 * implicitly (by being the set of pools that aren't declared).
 */
static struct bman_depletion pools;
static u8 num_pools;
static DEFINE_SPINLOCK(pools_lock);
static int explicit_allocator;

static struct dpa_uio_class bman_uio = {
	.list = LIST_HEAD_INIT(bman_uio.list),
	.dev_prefix = "bman-uio-"
};
const struct dpa_uio_class *dpa_uio_bman(void)
{
	return &bman_uio;
}
EXPORT_SYMBOL(dpa_uio_bman);

static int __bm_pool_add(u32 bpid, u32 *cfg, int triplets)
{
	u64 total = 0;
	BUG_ON(bpid >= bman_pool_max);
#ifdef CONFIG_FSL_BMAN_PORTAL
	while (triplets--) {
		struct bman_pool_params params = {
			.bpid = bpid,
			.flags = BMAN_POOL_FLAG_ONLY_RELEASE
		};
		u64 c = ((u64)cfg[0] << 32) | cfg[1];
		u64 d = ((u64)cfg[2] << 32) | cfg[3];
		u64 b = ((u64)cfg[4] << 32) | cfg[5];
		struct bman_pool *pobj = bman_new_pool(&params);
		if (!pobj)
			return -ENOMEM;
		while (c) {
			struct bm_buffer bufs[8];
			int ret, num_bufs = 0;
			do {
				BUG_ON(b > 0xffffffffffffull);
				bufs[num_bufs].bpid = bpid;
				bm_buffer_set64(&bufs[num_bufs++], b);
				b += d;
			} while (--c && (num_bufs < 8));
			ret = bman_release(pobj, bufs, num_bufs,
					BMAN_RELEASE_FLAG_WAIT);
			if (ret)
				panic("Seeding reserved buffer pool failed\n");
			total += num_bufs;
		}
		bman_free_pool(pobj);
		cfg += 6;
	}
#endif
	/* Remove this pool from the allocator (by treating its declaration as
	 * an implicit "reservation") iff the allocator is *not* being set up
	 * explicitly defined via "bpool-range" nodes. */
	if (!explicit_allocator && !bman_depletion_get(&pools, bpid)) {
		bman_depletion_set(&pools, bpid);
		num_pools++;
	}
	if (total)
		pr_info("Bman: reserved bpid %d, seeded %lld items\n", bpid,
			total);
	else
		pr_info("Bman: reserved bpid %d\n", bpid);
	return 0;
}

int bm_pool_new(u32 *bpid)
{
	int ret = 0, b = bman_pool_max;
	spin_lock(&pools_lock);
	if (num_pools >= bman_pool_max)
		ret = -ENOMEM;
	else {
		while (b-- && bman_depletion_get(&pools, b))
			;
		BUG_ON(b < 0);
		bman_depletion_set(&pools, b);
		*bpid = b;
		num_pools++;
	}
	spin_unlock(&pools_lock);
	return ret;
}
EXPORT_SYMBOL(bm_pool_new);

void bm_pool_free(u32 bpid)
{
	spin_lock(&pools_lock);
	BUG_ON(bpid >= bman_pool_max);
	BUG_ON(!bman_depletion_get(&pools, bpid));
	bman_depletion_unset(&pools, bpid);
	num_pools--;
	spin_unlock(&pools_lock);
}
EXPORT_SYMBOL(bm_pool_free);

#ifdef CONFIG_FSL_BMAN_PORTAL
/* To understand the use of this structure and the flow of operation for all
 * this portal-setup code, please see qman_driver.c. The Bman case is much the
 * same, but simpler (no Qman-specific fiddly bits). */
struct affine_portal_data {
	struct completion done;
	const struct bm_portal_config *pconfig;
	struct bman_portal *redirect;
	int recovery_mode;
	struct bman_portal *portal;
};

static __init int thread_init_affine_portal(void *__data)
{
	struct affine_portal_data *data = __data;
	const struct bm_portal_config *pconfig = data->pconfig;
	if (data->redirect)
		data->portal = bman_create_affine_slave(data->redirect);
	else {
		data->portal = bman_create_affine_portal(pconfig,
					data->recovery_mode);
#ifdef CONFIG_FSL_DPA_PIRQ_SLOW
		if (data->portal)
			bman_irqsource_add(BM_PIRQ_RCRI | BM_PIRQ_BSCN);
#endif
	}
	complete(&data->done);
	return 0;
}

static __init struct bman_portal *init_affine_portal(
					struct bm_portal_config *pconfig,
					int cpu, struct bman_portal *redirect,
					int recovery_mode)
{
	struct affine_portal_data data = {
		.done = COMPLETION_INITIALIZER_ONSTACK(data.done),
		.pconfig = pconfig,
		.redirect = redirect,
		.recovery_mode = recovery_mode,
		.portal = NULL
	};
	struct task_struct *k = kthread_create(thread_init_affine_portal, &data,
		"bman_affine%d", cpu);
	int ret;
	if (IS_ERR(k)) {
		pr_err("Failed to init %sBman affine portal for cpu %d\n",
			redirect ? "(slave) " : "", cpu);
		return NULL;
	}
	kthread_bind(k, cpu);
	wake_up_process(k);
	wait_for_completion(&data.done);
	ret = kthread_stop(k);
	if (ret) {
		pr_err("Bman portal initialisation failed, cpu %d, code %d\n",
			cpu, ret);
		return NULL;
	}
	if (data.portal)
		pr_info("Bman portal %sinitialised, cpu %d\n",
			redirect ? "(slave) " :
			pconfig->public_cfg.is_shared ? "(shared) " : "", cpu);
	return data.portal;
}
#endif

static struct bm_portal_config * __init fsl_bman_portal_init(
						struct device_node *node)
{
	struct bm_portal_config *pcfg;
	const u32 *index;
	const phandle *ph = NULL;
	int irq, ret;

	pcfg = kmalloc(sizeof(*pcfg), GFP_KERNEL);
	if (!pcfg) {
		pr_err("can't allocate portal config");
		return NULL;
	}

	if (of_device_is_compatible(node, "fsl,bman-portal-1.0")) {
		bman_ip_rev = BMAN_REV10;
		bman_pool_max = 64;
	} else if (of_device_is_compatible(node, "fsl,bman-portal-2.0")) {
		bman_ip_rev = BMAN_REV20;
		bman_pool_max = 8;
	}

	ret = of_address_to_resource(node, BM_ADDR_CE,
				&pcfg->addr_phys[BM_ADDR_CE]);
	if (ret) {
		pr_err("Can't get %s property 'reg::CE'\n", node->full_name);
		goto err;
	}
	ret = of_address_to_resource(node, BM_ADDR_CI,
				&pcfg->addr_phys[BM_ADDR_CI]);
	if (ret) {
		pr_err("Can't get %s property 'reg::CI'\n", node->full_name);
		goto err;
	}
	index = of_get_property(node, "cell-index", &ret);
	if (!index || (ret != 4)) {
		pr_err("Can't get %s property '%s'\n", node->full_name,
			"cell-index");
		goto err;
	}
	ph = of_get_property(node, "cpu-handle", &ret);
	if (ph) {
		if (ret != sizeof(phandle)) {
			pr_err("Malformed %s property '%s'\n", node->full_name,
				"cpu-handle");
			goto err;
		}
		ret = check_cpu_phandle(*ph);
		if (ret < 0)
			goto err;
		pcfg->public_cfg.cpu = ret;
	} else
		pcfg->public_cfg.cpu = -1;

	irq = irq_of_parse_and_map(node, 0);
	if (irq == NO_IRQ) {
		pr_err("Can't get %s property 'interrupts'\n", node->full_name);
		goto err;
	}
	pcfg->public_cfg.irq = irq;
	pcfg->public_cfg.index = *index;
	bman_depletion_fill(&pcfg->public_cfg.mask);

	if (of_get_property(node, "fsl,usdpaa-portal", &ret)) {
		struct dpa_uio_portal *u = kmalloc(sizeof(*u), GFP_KERNEL);
		if (!u)
			goto err;
		u->type = dpa_uio_portal_bman;
		u->bm_cfg = pcfg;
		list_add_tail(&u->node, &bman_uio.list);
		/* Return NULL, otherwise the kernel may share it on CPUs that
		 * don't have their own portals, which would be ... *bad*. */
		return NULL;
	}

	/* Map the portals now we know they aren't for UIO (the UIO code doesn't
	 * need the CE mapping, and so will do its own CI-only mapping). */
	pcfg->addr_virt[BM_ADDR_CE] = ioremap_prot(
				pcfg->addr_phys[BM_ADDR_CE].start,
				resource_size(&pcfg->addr_phys[BM_ADDR_CE]),
				0);
	pcfg->addr_virt[BM_ADDR_CI] = ioremap_prot(
				pcfg->addr_phys[BM_ADDR_CI].start,
				resource_size(&pcfg->addr_phys[BM_ADDR_CI]),
				_PAGE_GUARDED | _PAGE_NO_CACHE);
	return pcfg;
err:
	kfree(pcfg);
	return NULL;
}

static void __init fsl_bman_portal_destroy(struct bm_portal_config *pcfg)
{
	iounmap(pcfg->addr_virt[BM_ADDR_CE]);
	iounmap(pcfg->addr_virt[BM_ADDR_CI]);
	kfree(pcfg);
}

static int __init fsl_bpool_init(struct device_node *node)
{
	int ret;
	u32 *cfg = NULL, *thresh;
	struct device_node *tmp_node;
	u32 *bpid = (u32 *)of_get_property(node, "fsl,bpid", &ret);
	if (!bpid || (ret!= 4)) {
		pr_err("Can't get %s property 'fsl,bpid'\n", node->full_name);
		return -ENODEV;
	}
	thresh = (u32 *)of_get_property(node, "fsl,bpool-thresholds", &ret);
	if (thresh) {
		if (ret != 16) {
			pr_err("Invalid %s property '%s'\n",
				node->full_name, "fsl,bpool-thresholds");
			return -ENODEV;
		}
#ifndef CONFIG_FSL_BMAN_CONFIG
		pr_err("Ignoring %s property '%s', no CCSR support\n",
			node->full_name, "fsl,bpool-thresholds");
#endif
	}
	/* If rebooted, we should not re-seed any pools via bpool-cfg. */
	/* TODO: parsing hypervisor fields to determine qualitative things like
	 * "was I rebooted" should probably be wrapped in fsl_hypervisor.h. */
	tmp_node = of_find_node_by_name(NULL, "hypervisor");
	if (!tmp_node || !of_find_property(tmp_node, "fsl,hv-stopped-by",
						&ret))
		cfg = (u32 *)of_get_property(node, "fsl,bpool-cfg", &ret);
	if (cfg && (!ret || (ret % 24))) {
		pr_err("Invalid %s property '%s'\n", node->full_name,
			"fsl,bpool-cfg");
		return -ENODEV;
	}
	if (cfg)
		ret = __bm_pool_add(*bpid, cfg, ret / 24);
	else
		ret = __bm_pool_add(*bpid, NULL, 0);
	if (ret) {
		pr_err("Can't reserve bpid %d from node %s\n", *bpid,
			node->full_name);
		return ret;
	}
#ifdef CONFIG_FSL_BMAN_CONFIG
	if (thresh) {
		ret = bm_pool_set(*bpid, thresh);
		if (ret)
			pr_err("No CCSR node for %s property '%s'\n",
				node->full_name, "fsl,bpool-thresholds");
	}
#endif
	return ret;
}

static int __init fsl_bpool_range_init(struct device_node *node,
					int recovery_mode)
{
	int ret, warned = 0;
	u32 bpid;
	u32 *range = (u32 *)of_get_property(node, "fsl,bpool-range", &ret);
	if (!range) {
		pr_err("No 'fsl,bpool-range' property in node %s\n",
			node->full_name);
		return -EINVAL;
	}
	if (ret != 8) {
		pr_err("'fsl,bpool-range' is not a 2-cell range in node %s\n",
			node->full_name);
		return -EINVAL;
	}
	for (bpid = range[0]; bpid < (range[0] + range[1]); bpid++) {
		if (bpid >= bman_pool_max) {
			pr_err("BPIDs out of range in node %s\n",
				node->full_name);
			return -EINVAL;
		}
		if (!bman_depletion_get(&pools, bpid)) {
			if (!warned) {
				warned = 1;
				pr_err("BPID overlap in node %s, ignoring\n",
					node->full_name);
			}
		} else {
			bman_depletion_unset(&pools, bpid);
			num_pools--;
		}
	}
#ifdef CONFIG_FSL_BMAN_PORTAL
	/* If in recovery mode *and* we are using a private BPID allocation
	 * range, then automatically clean up all BPIDs in that range so we can
	 * automatically exit recovery mode too. */
	if (recovery_mode) {
		for (bpid = range[0]; bpid < (range[0] + range[1]); bpid++) {
			ret = bman_recovery_cleanup_bpid(bpid);
			if (ret) {
				pr_err("Failed to recovery BPID %d\n", bpid);
				return ret;
			}
		}
	}
#else
	BUG_ON(recovery_mode);
#endif
	pr_info("Bman: BPID allocator includes range %d:%d%s\n",
		range[0], range[1], recovery_mode ? " (recovered)" : "");
	return 0;
}

#ifdef CONFIG_FSL_BMAN_PORTAL
static __init int __leave_recovery(void *__data)
{
	struct completion *done = __data;
	bman_recovery_exit_local();
	complete(done);
	return 0;
}

int bman_recovery_exit(void)
{
	struct completion done = COMPLETION_INITIALIZER_ONSTACK(done);
	unsigned int cpu;

	for_each_cpu(cpu, bman_affine_cpus()) {
		struct task_struct *k = kthread_create(__leave_recovery, &done,
						"bman_recovery");
		int ret;
		if (IS_ERR(k)) {
			pr_err("Thread failure (recovery) on cpu %d\n", cpu);
			return -ENOMEM;
		}
		kthread_bind(k, cpu);
		wake_up_process(k);
		wait_for_completion(&done);
		ret = kthread_stop(k);
		if (ret) {
			pr_err("Failed to exit recovery on cpu %d\n", cpu);
			return ret;
		}
		pr_info("Bman portal exited recovery, cpu %d\n", cpu);
	}
	return 0;
}
EXPORT_SYMBOL(bman_recovery_exit);
#endif

static __init int bman_init(void)
{
#ifdef CONFIG_FSL_BMAN_PORTAL
	struct cpumask primary_cpus = *cpu_none_mask;
	struct cpumask slave_cpus = *cpu_online_mask;
	struct bman_portal *sharing_portal = NULL;
	int sharing_cpu = -1;
#endif
	struct device_node *dn;
	struct bm_portal_config *pcfg;
	int ret, recovery_mode = 0;
	LIST_HEAD(cfg_list);

	for_each_compatible_node(dn, NULL, "fsl,bman") {
		if (!bman_init_error_int(dn))
			pr_info("Bman err interrupt handler present\n");
		else
			pr_err("Bman err interrupt handler missing\n");
	}
	if (!bman_have_ccsr()) {
		/* If there's no CCSR, our bpid allocator is empty unless
		 * fsl,bpool-range nodes are used. */
		bman_depletion_fill(&pools);
		num_pools = bman_pool_max;
	}
#ifdef CONFIG_FSL_BMAN_PORTAL
	if (fsl_dpa_should_recover())
		recovery_mode = 1;
	for_each_compatible_node(dn, NULL, "fsl,bman-portal") {
		pcfg = fsl_bman_portal_init(dn);
		if (pcfg) {
			if (pcfg->public_cfg.cpu >= 0) {
				cpumask_set_cpu(pcfg->public_cfg.cpu,
						&primary_cpus);
				list_add(&pcfg->list, &cfg_list);
			} else
				fsl_bman_portal_destroy(pcfg);
		}
	}
	/* only consider "online" CPUs */
	cpumask_and(&primary_cpus, &primary_cpus, cpu_online_mask);
	if (cpumask_empty(&primary_cpus))
		/* No portals, we're done */
		return 0;
	if (!cpumask_subset(cpu_online_mask, &primary_cpus)) {
		/* Need to do some sharing. In lieu of anything more scientific
		 * (or configurable), we pick the last-most CPU that has a
		 * portal and share that one. */
		int next = cpumask_first(&primary_cpus);
		while (next < nr_cpu_ids) {
			sharing_cpu = next;
			next = cpumask_next(next, &primary_cpus);
		}
	}
	/* Parsing is done and sharing decisions are made, now initialise the
	 * portals and determine which "slave" CPUs are left over. */
	list_for_each_entry(pcfg, &cfg_list, list) {
		struct bman_portal *p;
		int is_shared = (!sharing_portal && (sharing_cpu >= 0) &&
				(pcfg->public_cfg.cpu == sharing_cpu));
		pcfg->public_cfg.is_shared = is_shared;
		/* If it's not mapped to a CPU, or another portal is already
		 * initialised to the same CPU, skip this portal. */
		if (pcfg->public_cfg.cpu < 0 || !cpumask_test_cpu(
					pcfg->public_cfg.cpu, &slave_cpus))
			continue;
		p = init_affine_portal(pcfg, pcfg->public_cfg.cpu, NULL,
					recovery_mode);
		if (p) {
			if (is_shared)
				sharing_portal = p;
			cpumask_clear_cpu(pcfg->public_cfg.cpu, &slave_cpus);
		}
	}

	if (sharing_portal) {
		int loop;
		for_each_cpu(loop, &slave_cpus) {
			struct bman_portal *p = init_affine_portal(NULL, loop,
					sharing_portal, recovery_mode);
			if (!p)
				pr_err("Failed slave Bman portal for cpu %d\n",
					loop);
		}
	}
#else
	for_each_compatible_node(dn, NULL, "fsl,bman-portal") {
		pcfg = fsl_bman_portal_init(dn);
		if (pcfg)
			/* No kernel portal support, so if USDPAA didn't consume
			 * the portal, we've no other use for it. */
			fsl_bman_portal_destroy(pcfg);
	}
#endif
	for_each_compatible_node(dn, NULL, "fsl,bpool-range") {
		if (!explicit_allocator) {
			explicit_allocator = 1;
			bman_depletion_fill(&pools);
			num_pools = 64;
		}
		ret = fsl_bpool_range_init(dn, recovery_mode);
		if (ret)
			return ret;
	}
#ifdef CONFIG_FSL_BMAN_PORTAL
	/* If using private BPID allocation, exit recovery mode automatically
	 * (ie. after automatic recovery) */
	if (recovery_mode && explicit_allocator) {
		ret = bman_recovery_exit();
		if (ret)
			return ret;
	}
#endif
	for_each_compatible_node(dn, NULL, "fsl,bpool") {
		ret = fsl_bpool_init(dn);
		if (ret)
			return ret;
	}
	pr_info("Bman portals initialised\n");
	return 0;
}
subsys_initcall(bman_init);
