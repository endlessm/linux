/**
 * AppliedMicro X-Gene SOC Queue Manager/Traffic Manager driver
 *
 * Copyright (c) 2013 Applied Micro Circuits Corporation.
 * Author: Ravi Patel <rapatel@apm.com>
 *         Keyur Chudgar <kchudgar@apm.com>
 *         Fushen Chen <fchen@apm.com>
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

#include <linux/slab.h>
#include "xgene_qmtm_main.h"
#include "xgene_qmtm_storm.h"

#define CSR_QSTATE_ADDR		0x0000006c
#define QNUMBER_WR(src)		(((u32)(src)) & 0x000003ff)

#define CSR_QSTATE_WR_0_ADDR	0x00000070
#define CSR_QSTATE_WR_1_ADDR	0x00000074
#define CSR_QSTATE_WR_2_ADDR	0x00000078
#define CSR_QSTATE_WR_3_ADDR	0x0000007c
#define CSR_QSTATE_WR_4_ADDR	0x00000080

static struct xgene_qmtm_sdev storm_sdev[SLAVE_MAX] = {
	[SLAVE_ETH0] = {
			.name = "SGMII0",
			.compatible = "apm,xgene-qmtm-soc",
			.slave = SLAVE_ETH0,
			.qmtm_ip = QMTM1,
			.slave_id = QMTM_SLAVE_ID_ETH0,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_ETH1] = {
			.name = "SGMII1",
			.compatible = "apm,xgene-qmtm-soc",
			.slave = SLAVE_ETH1,
			.qmtm_ip = QMTM1,
			.slave_id = QMTM_SLAVE_ID_ETH0,
			.wq_pbn_start = 0x08,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x28,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_ETH2] = {
			.name = "SGMII2",
			.compatible = "apm,xgene-qmtm-soc",
			.slave = SLAVE_ETH2,
			.qmtm_ip = QMTM1,
			.slave_id = QMTM_SLAVE_ID_ETH1,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_ETH3] = {
			.name = "SGMII3",
			.compatible = "apm,xgene-qmtm-soc",
			.slave = SLAVE_ETH3,
			.qmtm_ip = QMTM1,
			.slave_id = QMTM_SLAVE_ID_ETH1,
			.wq_pbn_start = 0x08,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x28,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_XGE0] = {
			.name = "SXGMII0",
			.compatible = "apm,xgene-qmtm-xge0",
			.slave = SLAVE_XGE0,
			.qmtm_ip = QMTM0,
			.slave_id = QMTM_SLAVE_ID_ETH0,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_XGE1] = {
			.name = "SXGMII1",
			.compatible = "apm,xgene-qmtm-xge0",
			.slave = SLAVE_XGE1,
			.qmtm_ip = QMTM0,
			.slave_id = QMTM_SLAVE_ID_ETH1,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_XGE2] = {
			.name = "SXGMII2",
			.compatible = "apm,xgene-qmtm-xge2",
			.slave = SLAVE_XGE2,
			.qmtm_ip = QMTM2,
			.slave_id = QMTM_SLAVE_ID_ETH0,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_XGE3] = {
			.name = "SXGMII3",
			.compatible = "apm,xgene-qmtm-xge2",
			.slave = SLAVE_XGE3,
			.qmtm_ip = QMTM2,
			.slave_id = QMTM_SLAVE_ID_ETH1,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x08,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x08,
			},
	[SLAVE_METH] = {
			.name = "RGMII",
			.compatible = "apm,xgene-qmtm-lite",
			.slave = SLAVE_METH,
			.qmtm_ip = QMTM3,
			.slave_id = QMTM_SLAVE_ID_ETH0,
			.wq_pbn_start = 0x00,
			.wq_pbn_count = 0x04,
			.fq_pbn_start = 0x20,
			.fq_pbn_count = 0x04,
			},
	[SLAVE_PKTDMA] = {
			  .name = "PKTDMA",
			  .compatible = "apm,xgene-qmtm-soc",
			  .slave = SLAVE_PKTDMA,
			  .qmtm_ip = QMTM1,
			  .slave_id = QMTM_SLAVE_ID_PKTDMA,
			  .wq_pbn_start = 0x00,
			  .wq_pbn_count = 0x04,
			  .fq_pbn_start = 0x20,
			  .fq_pbn_count = 0x08,
			  },
	[SLAVE_CPU_QMTM0] = {
			     .name = "CPU_QMTM0",
			     .compatible = "apm,xgene-qmtm-xge0",
			     .slave = SLAVE_CPU_QMTM0,
			     .qmtm_ip = QMTM0,
			     .slave_id = QMTM_SLAVE_ID_CPU,
			     .wq_pbn_start = 0x00,
			     .wq_pbn_count = 0x10,
			     .fq_pbn_start = 0x20,
			     .fq_pbn_count = 0x10,
			     },
	[SLAVE_CPU_QMTM1] = {
			     .name = "CPU_QMTM1",
			     .compatible = "apm,xgene-qmtm-soc",
			     .slave = SLAVE_CPU_QMTM1,
			     .qmtm_ip = QMTM1,
			     .slave_id = QMTM_SLAVE_ID_CPU,
			     .wq_pbn_start = 0x00,
			     .wq_pbn_count = 0x20,
			     .fq_pbn_start = 0x20,
			     .fq_pbn_count = 0x20,
			     },
	[SLAVE_CPU_QMTM2] = {
			     .name = "CPU_QMTM2",
			     .compatible = "apm,xgene-qmtm-xge2",
			     .slave = SLAVE_CPU_QMTM2,
			     .qmtm_ip = QMTM2,
			     .slave_id = QMTM_SLAVE_ID_CPU,
			     .wq_pbn_start = 0x10,
			     .wq_pbn_count = 0x10,
			     .fq_pbn_start = 0x30,
			     .fq_pbn_count = 0x10,
			     },
	[SLAVE_CPU_QMTM3] = {
			     .name = "CPU_QMTM3",
			     .compatible = "apm,xgene-qmtm-lite",
			     .slave = SLAVE_CPU_QMTM3,
			     .qmtm_ip = QMTM3,
			     .slave_id = QMTM_SLAVE_ID_CPU,
			     .wq_pbn_start = 0x00,
			     .wq_pbn_count = 0x01,
			     .fq_pbn_start = 0x20,
			     .fq_pbn_count = 0x01,
			     },
};

static void storm_qmtm_write_qstate(struct xgene_qmtm_qinfo *qinfo)
{
	struct xgene_qmtm *qmtm = qinfo->qmtm;
	u16 queue_id = qinfo->queue_id;
	struct storm_qmtm_csr_qstate *csr_qstate =
	    &((union storm_qmtm_qstate *)qinfo->qstate)->csr;

	/* write queue number */
	queue_id = QNUMBER_WR(queue_id);
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_ADDR, (u32) queue_id);

	/* write queue state */
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_WR_0_ADDR, csr_qstate->w0);
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_WR_1_ADDR, csr_qstate->w1);
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_WR_2_ADDR, csr_qstate->w2);
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_WR_3_ADDR, csr_qstate->w3);
	xgene_qmtm_wr32(qmtm, CSR_QSTATE_WR_4_ADDR, csr_qstate->w4);
}

static void storm_qmtm_read_qstate(struct xgene_qmtm_qinfo *qinfo)
{
	struct xgene_qmtm *qmtm = qinfo->qmtm;
	struct storm_qmtm_csr_qstate *qfabric = (struct storm_qmtm_csr_qstate *)
	    (qmtm->fabric_vaddr + (qinfo->queue_id << 6));
	struct storm_qmtm_csr_qstate *csr_qstate =
	    &((union storm_qmtm_qstate *)qinfo->qstate)->csr;
	struct storm_qmtm_pq_fp_qstate *pq_fp =
	    &((union storm_qmtm_qstate *)(qinfo->qstate))->pq;

	/* read queue state */
	csr_qstate->w0 = readl(&qfabric->w0);
	csr_qstate->w1 = readl(&qfabric->w1);
	csr_qstate->w2 = readl(&qfabric->w2);
	csr_qstate->w3 = readl(&qfabric->w3);
	csr_qstate->w4 = readl(&qfabric->w4);
	qinfo->nummsgs = pq_fp->nummsg;
}

static int storm_qmtm_set_qstate(struct xgene_qmtm_qinfo *qinfo)
{
	int rc = 0;
	struct storm_qmtm_pq_fp_qstate *pq_fp =
	    &((union storm_qmtm_qstate *)(qinfo->qstate))->pq;
	u64 cfgstartaddr;
	u32 qsize = 0;
	u8 qtype = qinfo->qtype;

	if (qtype != QTYPE_PQ && qtype != QTYPE_FP) {
		pr_err("Queue type %d is invalid\n", qinfo->qtype);
		rc = -EINVAL;
		goto _ret_set_qstate;
	}

	pq_fp->cfgqtype = qinfo->qtype;

	/* if its a free queue, ask QMTM to set len to 0 when dealloc */
	if (qtype == QTYPE_FP)
		pq_fp->fp_mode = CHANGE_LEN;

	if (qinfo->slave >= SLAVE_XGE0 && qinfo->slave <= SLAVE_XGE3) {
		pq_fp->cfgRecombBuf = 1;
		pq_fp->cfgRecombBufTimeoutL = 0xf;
		pq_fp->cfgRecombBufTimeoutH = 0x7;
	}

	pq_fp->cfgselthrsh = 1;

	/*  Allow the queue to accept message with non-zero LErr */
	pq_fp->cfgacceptlerr = 1;
	pq_fp->qcoherent = 1;

	switch (qinfo->qsize) {
	case QSIZE_512B:
		qsize = 512;
		break;
	case QSIZE_2KB:
		qsize = 2 * 1024;
		break;
	case QSIZE_16KB:
		qsize = 16 * 1024;
		break;
	case QSIZE_64KB:
		qsize = 64 * 1024;
		break;
	case QSIZE_512KB:
		qsize = 512 * 1024;
		break;
	default:
		pr_err("Queue size %d is invalid\n", qinfo->qsize);
		rc = -EINVAL;
		goto _ret_set_qstate;
	}

	qinfo->qdesc = kzalloc(sizeof(struct xgene_qmtm_qdesc), GFP_KERNEL);

	if (qinfo->qdesc == NULL) {
		rc = -ENOMEM;
		goto _ret_set_qstate;
	}

	qinfo->qdesc->count = (qtype == QTYPE_PQ) ? (qsize / 32) : (qsize / 16);

	if (qinfo->flags & XGENE_SLAVE_Q_ADDR_ALLOC) {
		qinfo->qdesc->qvaddr = kzalloc(qsize, GFP_KERNEL);
		if (qinfo->qdesc->qvaddr == NULL) {
			kfree(qinfo->qdesc);
			rc = -ENOMEM;
			goto _ret_set_qstate;
		}

		qinfo->qpaddr = (u64) VIRT_TO_PHYS(qinfo->qdesc->qvaddr);
	} else {
		qinfo->qdesc->qvaddr = PHYS_TO_VIRT(qinfo->qpaddr);
		memset(qinfo->qdesc->qvaddr, 0, qsize);
	}

	if ((qtype == QTYPE_PQ) && (qinfo->slave_id == QMTM_SLAVE_ID_CPU ||
				    qinfo->slave_id == QMTM_SLAVE_ID_MSLIM)) {
		u32 i;

		for (i = 0; i < qinfo->qdesc->count; i++) {
			u32 *slot = (u32 *)&qinfo->qdesc->msg32[i];

			slot[EMPTY_SLOT_INDEX] = EMPTY_SLOT;
		}
	}

	cfgstartaddr = qinfo->qpaddr >> 8;
	pq_fp->cfgstartaddrL = (u32)(cfgstartaddr & (BIT(27) - 1));
	pq_fp->cfgstartaddrH = (u32)(cfgstartaddr >> 27);
	pq_fp->cfgqsize = qinfo->qsize;
	storm_qmtm_write_qstate(qinfo);

_ret_set_qstate:
	return rc;
}

static void storm_qmtm_clr_qstate(struct xgene_qmtm_qinfo *qinfo)
{
	memset(qinfo->qstate, 0, sizeof(union storm_qmtm_qstate));
	storm_qmtm_write_qstate(qinfo);

	if (qinfo->flags & XGENE_SLAVE_Q_ADDR_ALLOC && qinfo->qdesc->qvaddr) {
		kfree(qinfo->qdesc->qvaddr);
		qinfo->qdesc->qvaddr = NULL;
	}

	kfree(qinfo->qdesc);
}

struct xgene_qmtm_sdev *storm_qmtm_get_sdev(char *name)
{
	struct xgene_qmtm *qmtm = NULL;
	struct xgene_qmtm_sdev *sdev = NULL;
	struct device_node *np = NULL;
	struct platform_device *platdev;
	u8 slave;

	for (slave = 0; slave < SLAVE_MAX; slave++) {
		sdev = &storm_sdev[slave];
		if (sdev->name && strcmp(name, sdev->name) == 0) {
			np = of_find_compatible_node(NULL, NULL,
				sdev->compatible);
			break;
		}
	}

	if (np == NULL)
		return NULL;

	platdev = of_find_device_by_node(np);
	qmtm = platform_get_drvdata(platdev);

	if (!qmtm->write_qstate) {
		qmtm->write_qstate = storm_qmtm_write_qstate;
		qmtm->read_qstate = storm_qmtm_read_qstate;
		qmtm->set_qstate = storm_qmtm_set_qstate;
		qmtm->clr_qstate = storm_qmtm_clr_qstate;
	}

	sdev->qmtm = qmtm;
	sdev->idev = qmtm->idev;

	return sdev;
}
