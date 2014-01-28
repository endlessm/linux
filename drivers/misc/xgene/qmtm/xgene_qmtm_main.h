/*
 * AppliedMicro X-Gene SOC Queue Manager/Traffic Manager driver
 *
 * Copyright (c) 2013 Applied Micro Circuits Corporation.
 * Author: Ravi Patel <rapatel@apm.com>
 *         Keyur Chudgar <kchudgar@apm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __XGENE_QMTM_MAIN_H__
#define __XGENE_QMTM_MAIN_H__

#include <linux/of_platform.h>
#include <misc/xgene/xgene_qmtm.h>

/* QMTM Platform Information */
enum xgene_qmtm_pinfo {
	STORM_QMTM,
	MAX_PLATFORM,
};

/* QMTM IP Blocks */
enum xgene_qmtm_ip {
	QMTM0,
	QMTM1,
	QMTM2,
	QMTM3,
	QMTM_MAX,
};

#define QMTM_MAX_QUEUES	1024
#define QMTM_MAX_PBN	32

struct xgene_qmtm {
	void *csr_vaddr;
	void *fabric_vaddr;
	u16 qmtm_ip;		/* qmtm_ip, see xgene_qmtm_ip */
	u16 error_irq;
	u32 max_queues;
	u16 dequeue_irq[QMTM_MAX_PBN];
	char error_irq_s[16];
	char error_queue_irq_s[16];
	struct xgene_qmtm_sdev *idev;
	u32 *queue_pool;
	struct xgene_qmtm_qinfo *(*qinfo);
	struct xgene_qmtm_qinfo *error_qinfo;
	struct clk *clk;
	struct platform_device *pdev;
	void (*write_qstate) (struct xgene_qmtm_qinfo *qinfo);
	void (*read_qstate) (struct xgene_qmtm_qinfo *qinfo);
	int (*set_qstate) (struct xgene_qmtm_qinfo *qinfo);
	void (*clr_qstate) (struct xgene_qmtm_qinfo *qinfo);
};

/* QMTM Slave */
enum xgene_slave {
	SLAVE_ETH0,
	SLAVE_ETH1,
	SLAVE_ETH2,
	SLAVE_ETH3,
	SLAVE_XGE0,
	SLAVE_XGE1,
	SLAVE_XGE2,
	SLAVE_XGE3,
	SLAVE_METH,
	SLAVE_PKTDMA,
	SLAVE_CTX_QMTM0,
	SLAVE_CTX_QMTM1,
	SLAVE_CTX_QMTM2,
	SLAVE_SEC,
	SLAVE_CLASS,
	SLAVE_MSLIM_QMTM0,
	SLAVE_MSLIM_QMTM1,
	SLAVE_MSLIM_QMTM2,
	SLAVE_MSLIM_QMTM3,
	SLAVE_PMPRO,
	SLAVE_SMPRO_QMTM0,
	SLAVE_SMPRO_QMTM1,
	SLAVE_SMPRO_QMTM2,
	SLAVE_SMPRO_QMTM3,
	SLAVE_CPU_QMTM0,
	SLAVE_CPU_QMTM1,
	SLAVE_CPU_QMTM2,
	SLAVE_CPU_QMTM3,
	SLAVE_MAX,
};

/* QMTM Slave IDs */
enum xgene_qmtm_slave_id {
	QMTM_SLAVE_ID_ETH0,
	QMTM_SLAVE_ID_ETH1,
	QMTM_SLAVE_ID_RES2,
	QMTM_SLAVE_ID_PKTDMA,
	QMTM_SLAVE_ID_CTX,
	QMTM_SLAVE_ID_SEC,
	QMTM_SLAVE_ID_CLASS,
	QMTM_SLAVE_ID_MSLIM,
	QMTM_SLAVE_ID_RES8,
	QMTM_SLAVE_ID_RES9,
	QMTM_SLAVE_ID_RESA,
	QMTM_SLAVE_ID_RESB,
	QMTM_SLAVE_ID_RESC,
	QMTM_SLAVE_ID_PMPRO,
	QMTM_SLAVE_ID_SMPRO,
	QMTM_SLAVE_ID_CPU,
	QMTM_SLAVE_ID_MAX,
};

/* QMTM Free Pool Queue modes */
enum xgene_qmtm_fp_mode {
	MSG_NO_CHANGE,
	ROUND_ADDR,
	REDUCE_LEN,
	CHANGE_LEN,
};

#define VIRT_TO_PHYS(x)	virt_to_phys(x)
#define PHYS_TO_VIRT(x)	phys_to_virt(x)

/* QMTM CSR read/write routine */
void xgene_qmtm_wr32(struct xgene_qmtm *qmtm, u32 offset, u32 data);
void xgene_qmtm_rd32(struct xgene_qmtm *qmtm, u32 offset, u32 *data);

/* QMTM Error handling */
int xgene_qmtm_enable_error(struct xgene_qmtm *qmtm);
void xgene_qmtm_disable_error(struct xgene_qmtm *qmtm);

struct xgene_qmtm_sdev *storm_qmtm_get_sdev(char *name);

#endif /* __XGENE_QMTM_MAIN_H__ */
