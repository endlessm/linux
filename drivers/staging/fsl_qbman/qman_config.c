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

#ifndef CONFIG_SMP
#include <linux/smp.h>	/* get_hard_smp_processor_id() */
#endif

#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "qman_private.h"

/* Last updated for v00.800 of the BG */

/* Register offsets */
#define REG_QCSP_PID_CFG(n)	(0x0000 + ((n) * 0x10))
#define REG_QCSP_IO_CFG(n)	(0x0004 + ((n) * 0x10))
#define REG_QCSP_DD_CFG(n)	(0x000c + ((n) * 0x10))
#define REG_DD_CFG		0x0200
#define REG_DCP_CFG(n)		(0x0300 + ((n) * 0x10))
#define REG_DCP_DD_CFG(n)	(0x0304 + ((n) * 0x10))
#define REG_DCP_DLM_AVG(n)	(0x030c + ((n) * 0x10))
#define REG_PFDR_FPC		0x0400
#define REG_PFDR_FP_HEAD	0x0404
#define REG_PFDR_FP_TAIL	0x0408
#define REG_PFDR_FP_LWIT	0x0410
#define REG_PFDR_CFG		0x0414
#define REG_SFDR_CFG		0x0500
#define REG_SFDR_IN_USE		0x0504
#define REG_WQ_CS_CFG(n)	(0x0600 + ((n) * 0x04))
#define REG_WQ_DEF_ENC_WQID	0x0630
#define REG_WQ_SC_DD_CFG(n)	(0x640 + ((n) * 0x04))
#define REG_WQ_PC_DD_CFG(n)	(0x680 + ((n) * 0x04))
#define REG_WQ_DC0_DD_CFG(n)	(0x6c0 + ((n) * 0x04))
#define REG_WQ_DC1_DD_CFG(n)	(0x700 + ((n) * 0x04))
#define REG_WQ_DCn_DD_CFG(n)	(0x6c0 + ((n) * 0x40)) /* n=2,3 */
#define REG_CM_CFG		0x0800
#define REG_ECSR		0x0a00
#define REG_ECIR		0x0a04
#define REG_EADR		0x0a08
#define REG_EDATA(n)		(0x0a10 + ((n) * 0x04))
#define REG_SBEC(n)		(0x0a80 + ((n) * 0x04))
#define REG_MCR			0x0b00
#define REG_MCP(n)		(0x0b04 + ((n) * 0x04))
#define REG_HID_CFG		0x0bf0
#define REG_IDLE_STAT		0x0bf4
#define REG_IP_REV_1		0x0bf8
#define REG_IP_REV_2		0x0bfc
#define REG_FQD_BARE		0x0c00
#define REG_PFDR_BARE		0x0c20
#define REG_offset_BAR		0x0004	/* relative to REG_[FQD|PFDR]_BARE */
#define REG_offset_AR		0x0010	/* relative to REG_[FQD|PFDR]_BARE */
#define REG_QCSP_BARE		0x0c80
#define REG_QCSP_BAR		0x0c84
#define REG_CI_SCHED_CFG	0x0d00
#define REG_SRCIDR		0x0d04
#define REG_LIODNR		0x0d08
#define REG_CI_RLM_AVG		0x0d14
#define REG_ERR_ISR		0x0e00	/* + "enum qm_isr_reg" */

/* Assists for QMAN_MCR */
#define MCR_INIT_PFDR		0x01000000
#define MCR_get_rslt(v)		(u8)((v) >> 24)
#define MCR_rslt_idle(r)	(!rslt || (rslt >= 0xf0))
#define MCR_rslt_ok(r)		(rslt == 0xf0)
#define MCR_rslt_eaccess(r)	(rslt == 0xf8)
#define MCR_rslt_inval(r)	(rslt == 0xff)

struct qman;

/* Follows WQ_CS_CFG0-5 */
enum qm_wq_class {
	qm_wq_portal = 0,
	qm_wq_pool = 1,
	qm_wq_fman0 = 2,
	qm_wq_fman1 = 3,
	qm_wq_caam = 4,
	qm_wq_pme = 5,
	qm_wq_first = qm_wq_portal,
	qm_wq_last = qm_wq_pme
};

/* Follows FQD_[BARE|BAR|AR] and PFDR_[BARE|BAR|AR] */
enum qm_memory {
	qm_memory_fqd,
	qm_memory_pfdr
};

/* Used by all error interrupt registers except 'inhibit' */
#define QM_EIRQ_CIDE	0x20000000	/* Corenet Initiator Data Error */
#define QM_EIRQ_CTDE	0x10000000	/* Corenet Target Data Error */
#define QM_EIRQ_CITT	0x08000000	/* Corenet Invalid Target Transaction */
#define QM_EIRQ_PLWI	0x04000000	/* PFDR Low Watermark */
#define QM_EIRQ_MBEI	0x02000000	/* Multi-bit ECC Error */
#define QM_EIRQ_SBEI	0x01000000	/* Single-bit ECC Error */
#define QM_EIRQ_PEBI	0x00800000	/* PFDR Enqueues Blocked Interrupt */
#define QM_EIRQ_ICVI	0x00010000	/* Invalid Command Verb */
#define QM_EIRQ_IDDI	0x00000800	/* Invalid Dequeue (Direct-connect) */
#define QM_EIRQ_IDFI	0x00000400	/* Invalid Dequeue FQ */
#define QM_EIRQ_IDSI	0x00000200	/* Invalid Dequeue Source */
#define QM_EIRQ_IDQI	0x00000100	/* Invalid Dequeue Queue */
#define QM_EIRQ_IEOI	0x00000008	/* Invalid Enqueue Overflow */
#define QM_EIRQ_IESI	0x00000004	/* Invalid Enqueue State */
#define QM_EIRQ_IECI	0x00000002	/* Invalid Enqueue Channel */
#define QM_EIRQ_IEQI	0x00000001	/* Invalid Enqueue Queue */

/* QMAN_ECIR valid error bit */
#define PORTAL_ECSR_ERR	(QM_EIRQ_IEQI | QM_EIRQ_IESI | QM_EIRQ_IEOI | \
				QM_EIRQ_IDQI | QM_EIRQ_IDSI | QM_EIRQ_IDFI | \
				QM_EIRQ_IDDI | QM_EIRQ_ICVI)
#define FQID_ECSR_ERR	(QM_EIRQ_IEQI | QM_EIRQ_IECI | QM_EIRQ_IESI | \
			QM_EIRQ_IEOI | QM_EIRQ_IDQI | QM_EIRQ_IDFI)

union qman_ecir {
	u32 ecir_raw;
	struct {
		u32 __reserved:2;
		u32 portal_type:1;
		u32 portal_num:5;
		u32 fqid:24;
	} __packed info;
};

union qman_eadr {
	u32 eadr_raw;
	struct {
		u32 __reserved1:4;
		u32 memid:4;
		u32 __reserved2:12;
		u32 eadr:12;
	} __packed info;
};

struct qman_hwerr_txt {
	u32 mask;
	const char *txt;
};

#define QMAN_HWE_TXT(a, b) { .mask = QM_EIRQ_##a, .txt = b }

static const struct qman_hwerr_txt qman_hwerr_txts[] = {
	QMAN_HWE_TXT(CIDE, "Corenet Initiator Data Error"),
	QMAN_HWE_TXT(CTDE, "Corenet Target Data Error"),
	QMAN_HWE_TXT(CITT, "Corenet Invalid Target Transaction"),
	QMAN_HWE_TXT(PLWI, "PFDR Low Watermark"),
	QMAN_HWE_TXT(MBEI, "Multi-bit ECC Error"),
	QMAN_HWE_TXT(SBEI, "Single-bit ECC Error"),
	QMAN_HWE_TXT(PEBI, "PFDR Enqueues Blocked Interrupt"),
	QMAN_HWE_TXT(ICVI, "Invalid Command Verb"),
	QMAN_HWE_TXT(IDDI, "Invalid Dequeue (Direct-connect)"),
	QMAN_HWE_TXT(IDFI, "Invalid Dequeue FQ"),
	QMAN_HWE_TXT(IDSI, "Invalid Dequeue Source"),
	QMAN_HWE_TXT(IDQI, "Invalid Dequeue Queue"),
	QMAN_HWE_TXT(IEOI, "Invalid Enqueue Overflow"),
	QMAN_HWE_TXT(IESI, "Invalid Enqueue State"),
	QMAN_HWE_TXT(IECI, "Invalid Enqueue Channel"),
	QMAN_HWE_TXT(IEQI, "Invalid Enqueue Queue")
};
#define QMAN_HWE_COUNT (sizeof(qman_hwerr_txts)/sizeof(struct qman_hwerr_txt))

struct qman_error_info_mdata {
	u16 addr_mask;
	u16 bits;
	const char *txt;
};

#define QMAN_ERR_MDATA(a, b, c) { .addr_mask = a, .bits = b, .txt = c}
static const struct qman_error_info_mdata error_mdata[] = {
	QMAN_ERR_MDATA(0x01FF, 24, "FQD cache tag memory 0"),
	QMAN_ERR_MDATA(0x01FF, 24, "FQD cache tag memory 1"),
	QMAN_ERR_MDATA(0x01FF, 24, "FQD cache tag memory 2"),
	QMAN_ERR_MDATA(0x01FF, 24, "FQD cache tag memory 3"),
	QMAN_ERR_MDATA(0x0FFF, 512, "FQD cache memory"),
	QMAN_ERR_MDATA(0x07FF, 128, "SFDR memory"),
	QMAN_ERR_MDATA(0x01FF, 72, "WQ context memory"),
	QMAN_ERR_MDATA(0x00FF, 240, "CGR memory"),
	QMAN_ERR_MDATA(0x00FF, 302, "Internal Order Restoration List memory"),
	QMAN_ERR_MDATA(0x01FF, 256, "SW portal ring memory"),
};
#define QMAN_ERR_MDATA_COUNT \
	(sizeof(error_mdata)/sizeof(struct qman_error_info_mdata))

/* Add this in Kconfig */
#define QMAN_ERRS_TO_UNENABLE (QM_EIRQ_PLWI | QM_EIRQ_PEBI)

/**
 * qm_err_isr_<reg>_<verb> - Manipulate global interrupt registers
 * @v: for accessors that write values, this is the 32-bit value
 *
 * Manipulates QMAN_ERR_ISR, QMAN_ERR_IER, QMAN_ERR_ISDR, QMAN_ERR_IIR. All
 * manipulations except qm_err_isr_[un]inhibit() use 32-bit masks composed of
 * the QM_EIRQ_*** definitions. Note that "qm_err_isr_enable_write" means
 * "write the enable register" rather than "enable the write register"!
 */
#define qm_err_isr_status_read(qm)	__qm_err_isr_read(qm, qm_isr_status)
#define qm_err_isr_status_clear(qm, m)	__qm_err_isr_write(qm, qm_isr_status,m)
#define qm_err_isr_enable_read(qm)	__qm_err_isr_read(qm, qm_isr_enable)
#define qm_err_isr_enable_write(qm, v)	__qm_err_isr_write(qm, qm_isr_enable,v)
#define qm_err_isr_disable_read(qm)	__qm_err_isr_read(qm, qm_isr_disable)
#define qm_err_isr_disable_write(qm, v)	__qm_err_isr_write(qm, qm_isr_disable,v)
#define qm_err_isr_inhibit(qm)		__qm_err_isr_write(qm, qm_isr_inhibit,1)
#define qm_err_isr_uninhibit(qm)	__qm_err_isr_write(qm, qm_isr_inhibit,0)

/*
 * TODO: unimplemented registers
 *
 * Keeping a list here of Qman registers I have not yet covered;
 * QCSP_DD_IHRSR, QCSP_DD_IHRFR, QCSP_DD_HASR,
 * DCP_DD_IHRSR, DCP_DD_IHRFR, DCP_DD_HASR, CM_CFG,
 * QMAN_EECC, QMAN_SBET, QMAN_EINJ, QMAN_SBEC0-12
 */

/* Encapsulate "struct qman *" as a cast of the register space address. */

static struct qman *qm_create(void *regs)
{
	return (struct qman *)regs;
}

static inline u32 __qm_in(struct qman *qm, u32 offset)
{
	return in_be32((void *)qm + offset);
}
static inline void __qm_out(struct qman *qm, u32 offset, u32 val)
{
	out_be32((void *)qm + offset, val);
}
#define qm_in(reg)		__qm_in(qm, REG_##reg)
#define qm_out(reg, val)	__qm_out(qm, REG_##reg, val)

static u32 __qm_err_isr_read(struct qman *qm, enum qm_isr_reg n)
{
	return __qm_in(qm, REG_ERR_ISR + (n << 2));
}

static void __qm_err_isr_write(struct qman *qm, enum qm_isr_reg n, u32 val)
{
	__qm_out(qm, REG_ERR_ISR + (n << 2), val);
}

#if 0

static void qm_set_portal(struct qman *qm, u8 swportalID,
			u16 ec_tp_cfg, u16 ecd_tp_cfg)
{
	qm_out(QCSP_DD_CFG(swportalID),
		((ec_tp_cfg & 0x1ff) << 16) | (ecd_tp_cfg & 0x1ff));
}

static void qm_set_ddebug(struct qman *qm, u8 mdd, u8 m_cfg)
{
	qm_out(DD_CFG, ((mdd & 0x3) << 4) | (m_cfg & 0xf));
}

static void qm_set_dc_ddebug(struct qman *qm, enum qm_dc_portal portal, u16 ecd_tp_cfg)
{
	qm_out(DCP_DD_CFG(portal), ecd_tp_cfg & 0x1ff);
}

static u32 qm_get_pfdr_free_pool_count(struct qman *qm)
{
	return qm_in(PFDR_FPC);
}

static void qm_get_pfdr_free_pool(struct qman *qm, u32 *head, u32 *tail)
{
	*head = qm_in(PFDR_FP_HEAD);
	*tail = qm_in(PFDR_FP_TAIL);
}

static void qm_set_default_wq(struct qman *qm, u16 wqid)
{
	qm_out(WQ_DEF_ENC_WQID, wqid);
}

static void qm_set_channel_ddebug(struct qman *qm, enum qm_channel channel,
				u16 tp_cfg)
{
	u32 offset;
	int upperhalf = 0;
	if ((channel >= qm_channel_swportal0) &&
				(channel <= qm_channel_swportal9)) {
		offset = (channel - qm_channel_swportal0);
		upperhalf = offset & 0x1;
		offset = REG_WQ_SC_DD_CFG(offset / 2);
	} else if ((channel >= qm_channel_pool1) &&
				(channel <= qm_channel_pool15)) {
		offset = (channel + 1 - qm_channel_pool1);
		upperhalf = offset & 0x1;
		offset = REG_WQ_PC_DD_CFG(offset / 2);
	} else if ((channel >= qm_channel_fman0_sp0) &&
				(channel <= qm_channel_fman0_sp11)) {
		offset = (channel - qm_channel_fman0_sp0);
		upperhalf = offset & 0x1;
		offset = REG_WQ_DC0_DD_CFG(offset / 2);
	}
	else if ((channel >= qm_channel_fman1_sp0) &&
				(channel <= qm_channel_fman1_sp11)) {
		offset = (channel - qm_channel_fman1_sp0);
		upperhalf = offset & 0x1;
		offset = REG_WQ_DC1_DD_CFG(offset / 2);
	}
	else if (channel == qm_channel_caam)
		offset = REG_WQ_DCn_DD_CFG(2);
	else if (channel == qm_channel_pme)
		offset = REG_WQ_DCn_DD_CFG(3);
	else {
		pr_crit("Illegal qm_channel type %d\n", channel);
		return;
	}
	__qm_out(qm, offset, upperhalf ? ((u32)tp_cfg << 16) : tp_cfg);
}

static void qm_get_details(struct qman *qm, u8 *int_options, u8 *errata,
			u8 *conf_options)
{
	u32 v = qm_in(IP_REV_1);
	*int_options = (v >> 16) & 0xff;
	*errata = (v >> 8) & 0xff;
	*conf_options = v & 0xff;
}

static void qm_set_corenet_bar(struct qman *qm, u16 eba, u32 ba)
{
	/* choke if 'ba' isn't properly aligned */
	DPA_ASSERT(!(ba & 0x001fffff));
	qm_out(QCSP_BARE, eba);
	qm_out(QCSP_BAR, ba);
}

static u8 qm_get_corenet_sourceid(struct qman *qm)
{
	return qm_in(SRCIDR);
}

static u16 qm_get_liodn(struct qman *qm)
{
	return qm_in(LIODNR);
}

static void qm_set_congestion_config(struct qman *qm, u16 pres)
{
	qm_out(CM_CFG, pres);
}

#endif

static void qm_set_dc(struct qman *qm, enum qm_dc_portal portal,
			int ed, u8 sernd)
{
	DPA_ASSERT(!ed || (portal == qm_dc_portal_fman0) ||
			(portal == qm_dc_portal_fman1));
	qm_out(DCP_CFG(portal), (ed ? 0x100 : 0) | (sernd & 0x1f));
}

static void qm_set_wq_scheduling(struct qman *qm, enum qm_wq_class wq_class,
			u8 cs_elev, u8 csw2, u8 csw3, u8 csw4, u8 csw5,
			u8 csw6, u8 csw7)
{
#ifdef CONFIG_FSL_QMAN_BUG_AND_FEATURE_REV1
#define csw(x) \
do { \
	if (++x == 8) \
		x = 7; \
} while (0)
	if (qman_ip_rev == QMAN_REV10) {
		csw(csw2);
		csw(csw3);
		csw(csw4);
		csw(csw5);
		csw(csw6);
		csw(csw7);
	}
#endif
	qm_out(WQ_CS_CFG(wq_class), ((cs_elev & 0xff) << 24) |
		((csw2 & 0x7) << 20) | ((csw3 & 0x7) << 16) |
		((csw4 & 0x7) << 12) | ((csw5 & 0x7) << 8) |
		((csw6 & 0x7) << 4) | (csw7 & 0x7));
}

static void qm_set_hid(struct qman *qm)
{
#ifdef CONFIG_FSL_QMAN_BUG_AND_FEATURE_REV1
	if (qman_ip_rev == QMAN_REV10)
		qm_out(HID_CFG, 3);
	else
#endif
	qm_out(HID_CFG, 0);
}

static void qm_set_corenet_initiator(struct qman *qm)
{
	qm_out(CI_SCHED_CFG,
		0x80000000 | /* write srcciv enable */
		(CONFIG_FSL_QMAN_CI_SCHED_CFG_SRCCIV << 24) |
		(CONFIG_FSL_QMAN_CI_SCHED_CFG_SRQ_W << 8) |
		(CONFIG_FSL_QMAN_CI_SCHED_CFG_RW_W << 4) |
		CONFIG_FSL_QMAN_CI_SCHED_CFG_BMAN_W);
}

static void qm_get_version(struct qman *qm, u16 *id, u8 *major, u8 *minor)
{
	u32 v = qm_in(IP_REV_1);
	*id = (v >> 16);
	*major = (v >> 8) & 0xff;
	*minor = v & 0xff;
}

static void qm_set_memory(struct qman *qm, enum qm_memory memory, u64 ba,
			int enable, int prio, int stash, u32 size)
{
	u32 offset = (memory == qm_memory_fqd) ? REG_FQD_BARE : REG_PFDR_BARE;
	u32 exp = ilog2(size);
	/* choke if size isn't within range */
	DPA_ASSERT((size >= 4096) && (size <= 1073741824) &&
			is_power_of_2(size));
	/* choke if 'ba' has lower-alignment than 'size' */
	DPA_ASSERT(!(ba & (size - 1)));
	__qm_out(qm, offset, upper_32_bits(ba));
	__qm_out(qm, offset + REG_offset_BAR, lower_32_bits(ba));
	__qm_out(qm, offset + REG_offset_AR,
		(enable ? 0x80000000 : 0) |
		(prio ? 0x40000000 : 0) |
		(stash ? 0x20000000 : 0) |
		(exp - 1));
}

static void qm_set_pfdr_threshold(struct qman *qm, u32 th, u8 k)
{
	qm_out(PFDR_FP_LWIT, th & 0xffffff);
	qm_out(PFDR_CFG, k);
}

static void qm_set_sfdr_threshold(struct qman *qm, u16 th)
{
	qm_out(SFDR_CFG, th & 0x3ff);
}

static int qm_init_pfdr(struct qman *qm, u32 pfdr_start, u32 num)
{
	u8 rslt = MCR_get_rslt(qm_in(MCR));

	DPA_ASSERT(pfdr_start && !(pfdr_start & 7) && !(num & 7) && num);
	/* Make sure the command interface is 'idle' */
	if(!MCR_rslt_idle(rslt))
		panic("QMAN_MCR isn't idle");

	/* Write the MCR command params then the verb */
	qm_out(MCP(0), pfdr_start );
	/* TODO: remove this - it's a workaround for a model bug that is
	 * corrected in more recent versions. We use the workaround until
	 * everyone has upgraded. */
	qm_out(MCP(1), (pfdr_start + num - 16));
	lwsync();
	qm_out(MCR, MCR_INIT_PFDR);

	/* Poll for the result */
	do {
		rslt = MCR_get_rslt(qm_in(MCR));
	} while(!MCR_rslt_idle(rslt));
	if (MCR_rslt_ok(rslt))
		return 0;
	if (MCR_rslt_eaccess(rslt))
		return -EACCES;
	if (MCR_rslt_inval(rslt))
		return -EINVAL;
	pr_crit("Unexpected result from MCR_INIT_PFDR: %02x\n", rslt);
	return -ENOSYS;
}

/*****************/
/* Config driver */
/*****************/

/* TODO: Kconfig these? */
#define DEFAULT_FQD_SZ	(PAGE_SIZE << CONFIG_FSL_QMAN_FQD_SZ)
#define DEFAULT_PFDR_SZ	(PAGE_SIZE << 12)

/* We support only one of these */
static struct qman *qm;
static struct device_node *qm_node;

/* Parse the <name> property to extract the memory location and size and
 * memblock_reserve() it. If it isn't supplied, memblock_alloc() the default size. */
static __init int parse_mem_property(struct device_node *node, const char *name,
				dma_addr_t *addr, size_t *sz, int zero)
{
	const u32 *pint;
	int ret;

	pint = of_get_property(node, name, &ret);
	if (!pint || (ret != 16)) {
		pr_info("No %s property '%s', using memblock_alloc(%016zx)\n",
				node->full_name, name, *sz);
		*addr = memblock_alloc(*sz, *sz);
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
		return 0;
	}
	pr_info("Using %s property '%s'\n", node->full_name, name);
	/* If using a "zero-pma", don't try to zero it, even if you asked */
	if (zero && of_find_property(node, "zero-pma", &ret)) {
		pr_info("  it's a 'zero-pma', not zeroing from s/w\n");
		zero = 0;
	}
	*addr = ((u64)pint[0] << 32) | (u64)pint[1];
	*sz = ((u64)pint[2] << 32) | (u64)pint[3];
	/* Keep things simple, it's either all in the DRAM range or it's all
	 * outside. */
	if (*addr < memblock_end_of_DRAM()) {
		BUG_ON((u64)*addr + (u64)*sz > memblock_end_of_DRAM());
		if (memblock_reserve(*addr, *sz) < 0) {
			pr_err("Failed to reserve %s\n", name);
			return -ENOMEM;
		}
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
	} else if (zero) {
		/* map as cacheable, non-guarded */
		void *tmpp = ioremap_prot(*addr, *sz, 0);
		memset(tmpp, 0, *sz);
		iounmap(tmpp);
	}
	return 0;
}

/* TODO:
 * - there is obviously no handling of errors,
 * - the calls to qm_set_memory() hard-code the priority and CPC-stashing for
 *   both memory resources to zero.
 */
static int __init fsl_qman_init(struct device_node *node)
{
	struct resource res;
	u32 __iomem *regs;
	const char *s;
	dma_addr_t fqd_a = 0, pfdr_a = 0;
	size_t fqd_sz = DEFAULT_FQD_SZ, pfdr_sz = DEFAULT_PFDR_SZ;
	int ret, standby = 0;
	u16 id;
	u8 major, minor;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		pr_err("Can't get %s property '%s'\n", node->full_name, "reg");
		return ret;
	}
	s = of_get_property(node, "fsl,hv-claimable", &ret);
	if (s && !strcmp(s, "standby"))
		standby = 1;
	if (!standby) {
		ret = parse_mem_property(node, "fsl,qman-fqd",
					&fqd_a, &fqd_sz, 1);
		BUG_ON(ret);
		ret = parse_mem_property(node, "fsl,qman-pfdr",
					&pfdr_a, &pfdr_sz, 0);
		BUG_ON(ret);
	}
	/* Global configuration */
	regs = ioremap(res.start, res.end - res.start + 1);
	qm = qm_create(regs);
	qm_node = node;
	qm_get_version(qm, &id, &major, &minor);
	pr_info("Qman ver:%04x,%02x,%02x\n", id, major, minor);
	if (!qman_ip_rev) {
		if ((major == 1) && (minor == 0))
			qman_ip_rev = QMAN_REV10;
		else if ((major == 1) && (minor == 1))
			qman_ip_rev = QMAN_REV11;
		else if	((major == 1) && (minor == 2))
			qman_ip_rev = QMAN_REV12;
		else if ((major == 2) && (minor == 0))
			qman_ip_rev = QMAN_REV20;
		else {
			pr_warning("unknown Qman version, default to rev1.1\n");
			qman_ip_rev = QMAN_REV11;
		}
	}

	if (standby) {
		pr_info("  -> in standby mode\n");
		return 0;
	}
	/* FQD memory */
	qm_set_memory(qm, qm_memory_fqd, fqd_a, 1, 0, 0, fqd_sz);
	/* PFDR memory */
	qm_set_memory(qm, qm_memory_pfdr, pfdr_a, 1, 0, 0, pfdr_sz);
	qm_init_pfdr(qm, 8, pfdr_sz / 64 - 8);
	/* thresholds */
	qm_set_pfdr_threshold(qm, 512, 64);
	qm_set_sfdr_threshold(qm, 128);
	/* clear stale PEBI bit from interrupt status register */
	qm_err_isr_status_clear(qm, QM_EIRQ_PEBI);
	/* corenet initiator settings */
	qm_set_corenet_initiator(qm);
	/* HID settings */
	qm_set_hid(qm);
	/* Set scheduling weights to defaults */
	for (ret = qm_wq_first; ret <= qm_wq_last; ret++)
		qm_set_wq_scheduling(qm, ret, 0, 0, 0, 0, 0, 0, 0);
	/* We are not prepared to accept ERNs for hardware enqueues */
	qm_set_dc(qm, qm_dc_portal_fman0, 1, 0);
	qm_set_dc(qm, qm_dc_portal_fman1, 1, 0);
	return 0;
}

int qman_have_ccsr(void)
{
	return qm ? 1 : 0;
}

__init void qman_init_early(void)
{
	struct device_node *dn;
	for_each_compatible_node(dn, NULL, "fsl,qman") {
		if (qm)
			pr_err("%s: only one 'fsl,qman' allowed\n",
				dn->full_name);
		else {
			int ret = fsl_qman_init(dn);
			BUG_ON(ret);
		}
	}
}

static void log_edata_bits(u32 bit_count)
{
	u32 i, j, mask = 0xffffffff;

	pr_warning("Qman ErrInt, EDATA:\n");
	i = bit_count/32;
	if (bit_count%32) {
		i++;
		mask = ~(mask << bit_count%32);
	}
	j = 16-i;
	pr_warning("  0x%08x\n", qm_in(EDATA(j)) & mask);
	j++;
	for (; j < 16; j++)
		pr_warning("  0x%08x\n", qm_in(EDATA(j)));
}

static void log_additional_error_info(u32 isr_val, u32 ecsr_val)
{
	union qman_ecir ecir_val;
	union qman_eadr eadr_val;

	ecir_val.ecir_raw = qm_in(ECIR);
	/* Is portal info valid */
	if (ecsr_val & PORTAL_ECSR_ERR) {
		pr_warning("Qman ErrInt: %s id %d\n",
			(ecir_val.info.portal_type) ?
			"DCP" : "SWP", ecir_val.info.portal_num);
	}
	if (ecsr_val & FQID_ECSR_ERR) {
		pr_warning("Qman ErrInt: ecir.fqid 0x%x\n",
			ecir_val.info.fqid);
	}
	if (ecsr_val & (QM_EIRQ_SBEI|QM_EIRQ_MBEI)) {
		eadr_val.eadr_raw = qm_in(EADR);
		pr_warning("Qman ErrInt: EADR Memory: %s, 0x%x\n",
			error_mdata[eadr_val.info.memid].txt,
			error_mdata[eadr_val.info.memid].addr_mask
				& eadr_val.info.eadr);
		log_edata_bits(error_mdata[eadr_val.info.memid].bits);
	}
}

/* Qman interrupt handler */
static irqreturn_t qman_isr(int irq, void *ptr)
{
	u32 isr_val, ier_val, ecsr_val, isr_mask, i;

	ier_val = qm_err_isr_enable_read(qm);
	isr_val = qm_err_isr_status_read(qm);
	ecsr_val = qm_in(ECSR);
	isr_mask = isr_val & ier_val;

	if (!isr_mask)
		return IRQ_NONE;
	for (i = 0; i < QMAN_HWE_COUNT; i++) {
		if (qman_hwerr_txts[i].mask & isr_mask) {
			pr_warning("Qman ErrInt: %s\n", qman_hwerr_txts[i].txt);
			if (qman_hwerr_txts[i].mask & ecsr_val) {
				log_additional_error_info(isr_mask, ecsr_val);
				/* Re-arm error capture registers */
				qm_out(ECSR, ecsr_val);
			}
			if (qman_hwerr_txts[i].mask & QMAN_ERRS_TO_UNENABLE) {
				pr_devel("Qman un-enabling error 0x%x\n",
					qman_hwerr_txts[i].mask);
				ier_val &= ~qman_hwerr_txts[i].mask;
				qm_err_isr_enable_write(qm, ier_val);
			}
		}
	}
	qm_err_isr_status_clear(qm, isr_val);
	return IRQ_HANDLED;
}

static int __bind_irq(void)
{
	int ret, err_irq;

	err_irq = of_irq_to_resource(qm_node, 0, NULL);
	if (err_irq == NO_IRQ) {
		pr_info("Can't get %s property '%s'\n", qm_node->full_name,
			"interrupts");
		return -ENODEV;
	}
	ret = request_irq(err_irq, qman_isr, IRQF_SHARED, "qman-err", qm_node);
	if (ret)  {
		pr_err("request_irq() failed %d for '%s'\n", ret,
			qm_node->full_name);
		return -ENODEV;
	}
	/* Write-to-clear any stale bits, (eg. starvation being asserted prior
	 * to resource allocation during driver init). */
	qm_err_isr_status_clear(qm, 0xffffffff);
	/* Enable Error Interrupts */
	qm_err_isr_enable_write(qm, 0xffffffff);
	return 0;
}

/* Initialise Error Interrupt Handler */
int qman_init_error_int(struct device_node *node)
{
	if (!qman_have_ccsr())
		return 0;
	if (node != qm_node)
		return -EINVAL;
	return __bind_irq();
}

#define PID_CFG_LIODN_MASK 0x0fff0000
void qman_liodn_fixup(enum qm_channel channel)
{
	static int done;
	static u32 liodn_offset;
	u32 before, after;
	int idx = channel - qm_channel_swportal0;

	if (!qman_have_ccsr())
		return;
	before = qm_in(QCSP_PID_CFG(idx));
	if (!done) {
		liodn_offset = before & PID_CFG_LIODN_MASK;
		done = 1;
		return;
	}
	after = (before & (~PID_CFG_LIODN_MASK)) | liodn_offset;
	qm_out(QCSP_PID_CFG(idx), after);
}

#ifdef CONFIG_SYSFS

#define DRV_NAME	"fsl-qman"

static ssize_t show_pfdr_fpc(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", qm_in(PFDR_FPC));
};

static ssize_t show_dlm_avg(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	u32 data;
	int i;

	if (!sscanf(dev_attr->attr.name, "dcp%d_dlm_avg", &i))
		return -EINVAL;
	data = qm_in(DCP_DLM_AVG(i));
	return snprintf(buf, PAGE_SIZE, "%d.%08d\n", data>>8,
			(data & 0x000000ff)*390625);
};

static ssize_t set_dlm_avg(struct device *dev,
	struct device_attribute *dev_attr, const char *buf, size_t count)
{
	unsigned long val;
	int i;

	if (!sscanf(dev_attr->attr.name, "dcp%d_dlm_avg", &i))
		return -EINVAL;
	if (strict_strtoul(buf, 0, &val)) {
		dev_dbg(dev, "invalid input %s\n", buf);
		return -EINVAL;
	}
	qm_out(DCP_DLM_AVG(i), val);
	return count;
};

static ssize_t show_pfdr_cfg(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", qm_in(PFDR_CFG));
};

static ssize_t set_pfdr_cfg(struct device *dev,
	struct device_attribute *dev_attr, const char *buf, size_t count)
{
	unsigned long val;

	if (strict_strtoul(buf, 0, &val)) {
		dev_dbg(dev, "invalid input %s\n", buf);
		return -EINVAL;
	}
	qm_out(PFDR_CFG, val);
	return count;
};

static ssize_t show_sfdr_in_use(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", qm_in(SFDR_IN_USE));
};

static ssize_t show_idle_stat(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", qm_in(IDLE_STAT));
};

static ssize_t show_ci_rlm_avg(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	u32 data = qm_in(CI_RLM_AVG);
	return snprintf(buf, PAGE_SIZE, "%d.%08d\n", data>>8,
			(data & 0x000000ff)*390625);
};

static ssize_t set_ci_rlm_avg(struct device *dev,
	struct device_attribute *dev_attr, const char *buf, size_t count)
{
	unsigned long val;

	if (strict_strtoul(buf, 0, &val)) {
		dev_dbg(dev, "invalid input %s\n", buf);
		return -EINVAL;
	}
	qm_out(CI_RLM_AVG, val);
	return count;
};

static ssize_t show_err_isr(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%08x\n", qm_in(ERR_ISR));
};


static ssize_t show_sbec(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	int i;

	if (!sscanf(dev_attr->attr.name, "sbec_%d", &i))
		return -EINVAL;
	return snprintf(buf, PAGE_SIZE, "%u\n", qm_in(SBEC(i)));
};

static DEVICE_ATTR(pfdr_fpc, S_IRUSR, show_pfdr_fpc, NULL);
static DEVICE_ATTR(pfdr_cfg, S_IRUSR, show_pfdr_cfg, set_pfdr_cfg);
static DEVICE_ATTR(idle_stat, S_IRUSR, show_idle_stat, NULL);
static DEVICE_ATTR(ci_rlm_avg, (S_IRUSR|S_IWUGO),
		show_ci_rlm_avg, set_ci_rlm_avg);
static DEVICE_ATTR(err_isr, S_IRUSR, show_err_isr, NULL);
static DEVICE_ATTR(sfdr_in_use, S_IRUSR, show_sfdr_in_use, NULL);

static DEVICE_ATTR(dcp0_dlm_avg, (S_IRUSR|S_IWUGO), show_dlm_avg, set_dlm_avg);
static DEVICE_ATTR(dcp1_dlm_avg, (S_IRUSR|S_IWUGO), show_dlm_avg, set_dlm_avg);
static DEVICE_ATTR(dcp2_dlm_avg, (S_IRUSR|S_IWUGO), show_dlm_avg, set_dlm_avg);
static DEVICE_ATTR(dcp3_dlm_avg, (S_IRUSR|S_IWUGO), show_dlm_avg, set_dlm_avg);

static DEVICE_ATTR(sbec_0, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_1, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_2, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_3, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_4, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_5, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_6, S_IRUSR, show_sbec, NULL);


static struct attribute *qman_dev_attributes[] = {
	&dev_attr_pfdr_fpc.attr,
	&dev_attr_pfdr_cfg.attr,
	&dev_attr_idle_stat.attr,
	&dev_attr_ci_rlm_avg.attr,
	&dev_attr_err_isr.attr,
	&dev_attr_dcp0_dlm_avg.attr,
	&dev_attr_dcp1_dlm_avg.attr,
	&dev_attr_dcp2_dlm_avg.attr,
	&dev_attr_dcp3_dlm_avg.attr,
	/* sfdr_in_use will be added if necessary */
	NULL
};

static struct attribute *qman_dev_ecr_attributes[] = {
	&dev_attr_sbec_0.attr,
	&dev_attr_sbec_1.attr,
	&dev_attr_sbec_2.attr,
	&dev_attr_sbec_3.attr,
	&dev_attr_sbec_4.attr,
	&dev_attr_sbec_5.attr,
	&dev_attr_sbec_6.attr,
	NULL
};

/* root level */
static const struct attribute_group qman_dev_attr_grp = {
	.name = NULL,
	.attrs = qman_dev_attributes
};
static const struct attribute_group qman_dev_ecr_grp = {
	.name = "error_capture",
	.attrs = qman_dev_ecr_attributes
};

static int of_fsl_qman_remove(struct platform_device *ofdev)
{
	sysfs_remove_group(&ofdev->dev.kobj, &qman_dev_attr_grp);
	return 0;
};

static int of_fsl_qman_probe(struct platform_device *ofdev)
{
	int ret;

	ret = sysfs_create_group(&ofdev->dev.kobj, &qman_dev_attr_grp);
	if (ret)
		goto done;
	if (qman_ip_rev != QMAN_REV10) {
		ret = sysfs_add_file_to_group(&ofdev->dev.kobj,
			&dev_attr_sfdr_in_use.attr, qman_dev_attr_grp.name);
		if (ret)
			goto del_group_0;
	}
	ret = sysfs_create_group(&ofdev->dev.kobj, &qman_dev_ecr_grp);
	if (ret)
		goto del_group_0;

	goto done;

del_group_0:
	sysfs_remove_group(&ofdev->dev.kobj, &qman_dev_attr_grp);
done:
	if (ret)
		dev_err(&ofdev->dev,
				"Cannot create dev attributes ret=%d\n", ret);
	return ret;
};

static struct of_device_id of_fsl_qman_ids[] = {
	{
		.compatible = "fsl,qman",
	},
	{}
};
MODULE_DEVICE_TABLE(of, of_fsl_qman_ids);

static struct platform_driver of_fsl_qman_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = DRV_NAME,
		.of_match_table = of_fsl_qman_ids,
	},
	.probe = of_fsl_qman_probe,
	.remove      = of_fsl_qman_remove,
};

static int qman_ctrl_init(void)
{
	return platform_driver_register(&of_fsl_qman_driver);
}

static void qman_ctrl_exit(void)
{
	platform_driver_unregister(&of_fsl_qman_driver);
}

module_init(qman_ctrl_init);
module_exit(qman_ctrl_exit);

#endif /* CONFIG_SYSFS */
