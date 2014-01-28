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

#include <linux/module.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "xgene_qmtm_main.h"

#define XGENE_QMTM_DRIVER_VER	"1.0"
#define XGENE_QMTM_DRIVER_NAME	"xgene-qmtm"
#define XGENE_QMTM_DRIVER_DESC	"APM X-Gene QMTM driver"

/* CSR Address Macros */
#define CSR_QM_CONFIG_ADDR	0x00000004
#define  QM_ENABLE_WR(src)	(((u32)(src)<<31) & 0x80000000)

#define CSR_PBM_ADDR		0x00000008
#define  OVERWRITE_WR(src)	(((u32)(src)<<31) & 0x80000000)
#define  SLVID_PBN_WR(src)	(((u32)(src)) & 0x000003ff)
#define  SLAVE_ID_SHIFT		6

#define CSR_PBM_BUF_WR_ADDR	0x0000000c
#define CSR_PBM_BUF_RD_ADDR	0x00000010
#define  PB_SIZE_WR(src)	(((u32)(src)<<31) & 0x80000000)
#define  PREFETCH_BUF_EN_SET(dst, src) \
	(((dst) & ~0x00200000) | (((u32)(src)<<21) & 0x00200000))
#define  IS_FREE_POOL_SET(dst, src) \
	(((dst) & ~0x00100000) | (((u32)(src)<<20) & 0x00100000))
#define  TLVQ_SET(dst, src) \
	(((dst) & ~0x00080000) | (((u32)(src)<<19) & 0x00080000))
#define  CORRESPONDING_QNUM_SET(dst, src) \
	(((dst) & ~0x0007fe00) | (((u32)(src)<<9) & 0x0007fe00))

#define CSR_PBM_CTICK0_ADDR	0x00000018
#define  MIN_COAL_TAP		0x0
#define  MAX_COAL_TAP		0x7

#define CSR_THRESHOLD0_SET1_ADDR	0x00000030
#define CSR_THRESHOLD1_SET1_ADDR	0x00000034
#define CSR_HYSTERESIS_ADDR		0x00000068
#define CSR_QM_MBOX_NE_INT_MODE_ADDR	0x0000017c
#define CSR_QMLITE_PBN_MAP_0_ADDR	0x00000228

#define CSR_RECOMB_CTRL_0_ADDR		0x00000230
#define  RECOMB_EN0_SET(dst, src) \
	(((dst) & ~0x00000001) | (((u32)(src)) & 0x00000001))

/* QMTM Diag CSR */
#define QM_GLBL_DIAG_CSR_BASE_ADDR_OFFSET	0xd000
#define QM_CFG_MEM_RAM_SHUTDOWN_ADDR		0x00000070
#define QM_CFG_MEM_RAM_SHUTDOWN_DEFAULT		0xffffffff

/* PBN macros */
#define QMTM_MIN_PBN_ID	0
#define QMTM_MAX_PBN_ID	31

/* Common for Queue ID and PBN */
#define RES_MASK(nr)	(1UL << ((nr) % 32))
#define RES_WORD(nr)	((nr) / 32)

void xgene_qmtm_wr32(struct xgene_qmtm *qmtm, u32 offset, u32 data)
{
	void *addr = (u8 *)qmtm->csr_vaddr + offset;
	writel(data, addr);
}

void xgene_qmtm_rd32(struct xgene_qmtm *qmtm, u32 offset, u32 *data)
{
	void *addr = (u8 *)qmtm->csr_vaddr + offset;
	*data = readl(addr);
}

/* Get available PBN or Queue Id */
static int xgene_qmtm_get_resource_id(u32 *resource, u32 start, u32 end)
{
	u32 index;

	for (index = start; index < end; index++) {
		if ((resource[RES_WORD(index)] & RES_MASK(index)) == 0) {
			resource[RES_WORD(index)] |= RES_MASK(index);
			return index;
		}
	}

	return -ENODEV;
}

/* Put used PBN or Queue Id */
static inline void xgene_qmtm_put_resource_id(u32 *resource, u32 index)
{
	resource[RES_WORD(index)] &= ~(u32) RES_MASK(index);
}

static void xgene_qmtm_write_pbm(struct xgene_qmtm_qinfo *qinfo, u32 val)
{
	u32 pbm = SLVID_PBN_WR((qinfo->slave_id << SLAVE_ID_SHIFT) |
				    qinfo->pbn) | OVERWRITE_WR(1);

	xgene_qmtm_wr32(qinfo->qmtm, CSR_PBM_ADDR, pbm);

	if (qinfo->qmtm_ip == QMTM0 || qinfo->qmtm_ip == QMTM2)
		val |= PB_SIZE_WR(1);

	xgene_qmtm_wr32(qinfo->qmtm, CSR_PBM_BUF_WR_ADDR, val);
}

static u32 xgene_qmtm_read_pbm(struct xgene_qmtm_qinfo *qinfo)
{
	u32 pbm = SLVID_PBN_WR((qinfo->slave_id << SLAVE_ID_SHIFT) |
				    qinfo->pbn);

	xgene_qmtm_wr32(qinfo->qmtm, CSR_PBM_ADDR, pbm);
	xgene_qmtm_rd32(qinfo->qmtm, CSR_PBM_BUF_RD_ADDR, &pbm);

	return pbm;
}

static void xgene_qmtm_set_pbm(struct xgene_qmtm_qinfo *qinfo)
{
	u16 is_fp = qinfo->qtype == QTYPE_FP ? 1 : 0;
	u16 is_vq = qinfo->qtype == QTYPE_VQ ? 1 : 0;
	u32 val = 0;

	val = CORRESPONDING_QNUM_SET(val, qinfo->queue_id);
	val = IS_FREE_POOL_SET(val, is_fp);
	val = TLVQ_SET(val, is_vq);
	val = PREFETCH_BUF_EN_SET(val, 1);
	xgene_qmtm_write_pbm(qinfo, val);
}

static void xgene_qmtm_clr_pbm(struct xgene_qmtm_qinfo *qinfo)
{
	xgene_qmtm_write_pbm(qinfo, 0);
}

/**
 * xgene_qmtm_set_qinfo - Create and configure a queue
 * @sdev:	Slave context
 * @qtype:	Queue type (P_QUEUE or FP_QUEUE)
 * @qsize:	Queue size see xgene_qmtm_qsize
 * @qaccess:	Queue access method see xgene_qmtm_qaccess
 * @flags:	Queue Information flags
 * @qpaddr:	If Desire Queue Physical Address to use
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to create and configure a queue.
 *
 * Return:	0 on Success or -1 on Failure
 *		On Success, updates following in qinfo,
 *		qmtm_ip - QMTM0, QMTM1, QMTM2 or QMTM3
 *		qmtm - QMTM instance context
 *		slave - Slave see xgene_slave
 *		slave_id - Slave ID see xgene_qmtm_slave_id
 *		pbn - Slave ID's prefetch buffer number
 *		queue_id - Queue ID
 *		qdesc - Queue descriptor
 */
int xgene_qmtm_set_qinfo(struct xgene_qmtm_qinfo *set)
{
	struct xgene_qmtm_sdev *sdev = set->sdev;
	struct device *dev;
	struct xgene_qmtm *qmtm;
	struct xgene_qmtm_qinfo *qinfo;
	u32 *queue_resource = NULL, *pbn_resource = NULL;
	int rc;
	u8 pbn = 0;
	u16 queue_id = 0;

	qmtm = sdev->qmtm;
	dev = &qmtm->pdev->dev;

	if (set->flags & XGENE_SLAVE_PB_CONFIGURE) {
		u8 pbn_start, pbn_count;

		if (set->qtype == QTYPE_FP) {
			pbn_resource = &sdev->fq_pbn_pool;
			pbn_start = sdev->fq_pbn_start & ~(u8) 0x20;
			pbn_count = sdev->fq_pbn_count;
		} else {
			pbn_resource = &sdev->wq_pbn_pool;
			pbn_start = sdev->wq_pbn_start;
			pbn_count = sdev->wq_pbn_count;
		}

		rc = xgene_qmtm_get_resource_id(pbn_resource, pbn_start,
						pbn_start + pbn_count);
		if (rc < 0) {
			dev_err(dev, "SETQ: slave %d out of PBN\n",
				sdev->slave);
			goto _ret_set_qinfo;
		}

		pbn = rc;
	}

	queue_resource = qmtm->queue_pool;
	rc = xgene_qmtm_get_resource_id(queue_resource, 0, qmtm->max_queues);
	if (rc < 0) {
		dev_err(dev, "SETQ: QMTM %d out of Queue ID\n", sdev->qmtm_ip);
		goto _put_pbn_resource;
	}

	queue_id = rc;
	qinfo = kzalloc(sizeof(struct xgene_qmtm_qinfo), GFP_KERNEL);
	if (qinfo == NULL) {
		dev_err(dev, "SETQ: Unable to allocate qinfo\n");
		rc = -ENOMEM;
		goto _put_queue_resource;
	}

	qinfo->slave = sdev->slave;
	qinfo->slave_id = sdev->slave_id;
	qinfo->qmtm_ip = sdev->qmtm_ip;
	qinfo->qtype = set->qtype;
	qinfo->qsize = set->qsize;
	qinfo->qaccess = set->qaccess;
	qinfo->flags = set->flags;
	qinfo->pbn = set->qtype == QTYPE_FP ? (pbn | 0x20) : pbn;
	qinfo->queue_id = queue_id;
	qinfo->qpaddr = set->qpaddr;
	qinfo->qfabric = qmtm->fabric_vaddr + (queue_id << 6);
	qinfo->sdev = sdev;
	qinfo->qmtm = qmtm;
	rc = qmtm->set_qstate(qinfo);
	if (rc < 0) {
		dev_err(dev, "SETQ: set_qstate error for %s Queue ID %d\n",
			sdev->name, queue_id);
		goto _del_qstate;
	}

	if (qinfo->qaccess == QACCESS_ALT)
		qinfo->qdesc->command = qinfo->qfabric + 0x2C;

	if (set->flags & XGENE_SLAVE_PB_CONFIGURE) {
		xgene_qmtm_set_pbm(qinfo);

		if (set->qaccess == QACCESS_ALT &&
		    sdev->slave_id == QMTM_SLAVE_ID_CPU &&
		    set->qtype == QTYPE_PQ) {
			u32 data;

			xgene_qmtm_rd32(qmtm, CSR_QM_MBOX_NE_INT_MODE_ADDR,
					&data);
			data |= (u32) (1 << (31 - pbn));
			xgene_qmtm_wr32(qmtm, CSR_QM_MBOX_NE_INT_MODE_ADDR,
					data);
		}

		if (set->qtype == QTYPE_PQ &&
		    (sdev->slave_id == QMTM_SLAVE_ID_CPU ||
		     sdev->slave_id == QMTM_SLAVE_ID_MSLIM))
			qinfo->qdesc->irq = qmtm->dequeue_irq[pbn];
	}

	qmtm->qinfo[queue_id] = qinfo;
	memcpy(set, qinfo, sizeof(struct xgene_qmtm_qinfo));
	return rc;

_del_qstate:
	qmtm->clr_qstate(qinfo);
	kfree(qinfo);

_put_queue_resource:
	xgene_qmtm_put_resource_id(queue_resource, queue_id);

_put_pbn_resource:
	if (set->flags & XGENE_SLAVE_PB_CONFIGURE)
		xgene_qmtm_put_resource_id(pbn_resource, pbn);

_ret_set_qinfo:
	return rc;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_set_qinfo);

/**
 * xgene_qmtm_clr_qinfo - Unconfigure and delete a queue
 * @sdev:	Slave context
 * @queue_id:	Queue ID
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to unconfigure and delete a queue.
 */
void xgene_qmtm_clr_qinfo(struct xgene_qmtm_qinfo *clr)
{
	struct xgene_qmtm_sdev *sdev = clr->sdev;
	struct xgene_qmtm *qmtm;
	struct xgene_qmtm_qinfo *qinfo;
	u8 queue_id = clr->queue_id;

	qmtm = sdev->qmtm;
	qinfo = qmtm->qinfo[queue_id];

	if (qinfo->flags & XGENE_SLAVE_PB_CONFIGURE) {
		u32 *pbn_resource;
		u8 qtype = qinfo->qtype;
		u8 pbn = (qtype == QTYPE_FP) ?
		    (qinfo->pbn & ~(u8) 0x20) : qinfo->pbn;

		if (qinfo->qaccess == QACCESS_ALT &&
		    qinfo->slave_id == QMTM_SLAVE_ID_CPU && qtype == QTYPE_PQ) {
			u32 data;

			xgene_qmtm_rd32(qmtm, CSR_QM_MBOX_NE_INT_MODE_ADDR,
					&data);
			data &= ~(u32) (1 << (31 - pbn));
			xgene_qmtm_wr32(qmtm, CSR_QM_MBOX_NE_INT_MODE_ADDR,
					data);
		}

		if (qinfo->qtype == QTYPE_FP)
			pbn_resource = &sdev->fq_pbn_pool;
		else
			pbn_resource = &sdev->wq_pbn_pool;

		xgene_qmtm_clr_pbm(qinfo);
		xgene_qmtm_put_resource_id(pbn_resource, pbn);
	}

	qmtm->clr_qstate(qinfo);
	kfree(qinfo);
	xgene_qmtm_put_resource_id(qmtm->queue_pool, queue_id);
	qmtm->qinfo[queue_id] = NULL;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_clr_qinfo);

/**
 * xgene_qmtm_read_qstate - Get Queue State information for a queue
 * @qmtm:	QMTM instance context
 * @queue_id:	Queue ID
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to read queue state of a queue.
 *
 * Return:	Updates following in qinfo,
 *		qstate - Current Queue State in QMTM
 *		nummsgs - Number os messages in the Queue
 *		pbm_state - Current prefetch buffer manager state
 */
void xgene_qmtm_read_qstate(struct xgene_qmtm_qinfo *qinfo)
{
	struct xgene_qmtm *qmtm = qinfo->qmtm;
	u8 queue_id = qinfo->queue_id;

	memcpy(qinfo, qmtm->qinfo[queue_id], sizeof(struct xgene_qmtm_qinfo));
	qmtm->read_qstate(qinfo);

	if (qinfo->flags & XGENE_SLAVE_PB_CONFIGURE)
		qinfo->pbm_state = xgene_qmtm_read_pbm(qinfo);
}
EXPORT_SYMBOL_GPL(xgene_qmtm_read_qstate);

/**
 * xgene_qmtm_intr_coalesce - Set interrupt coalescing for ingrgess queue
 * @qmtm:	QMTM instance context
 * @pbn:	CPU's prefetch buffer number corresponding to the interrupt
 * @tap:	Tap value to set
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to set interrupt for its ingress queue.
 *
 * Return:	0 on Success or -1 on Failure
 */
int xgene_qmtm_intr_coalesce(struct xgene_qmtm_qinfo *qinfo, u8 tap)
{
	u32 val, offset, mask, shift;
	u8 pbn = qinfo->pbn;

	if (tap < MIN_COAL_TAP || tap > MAX_COAL_TAP)
		return -EINVAL;

	if (pbn < QMTM_MIN_PBN_ID || pbn > QMTM_MAX_PBN_ID)
		return -EINVAL;

	offset = 4 * (pbn / 8);
	shift = 4 * (7 - (pbn % 8));
	mask = 7 << shift;

	xgene_qmtm_rd32(qinfo->qmtm, CSR_PBM_CTICK0_ADDR + offset, &val);
	val = (val & ~(u32) mask) | (((u32) tap << shift) & mask);
	xgene_qmtm_wr32(qinfo->qmtm, CSR_PBM_CTICK0_ADDR + offset, val);

	return 0;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_intr_coalesce);

/**
 * xgene_qmtm_fp_dealloc_msg - Fill a buffer in a free pool queue
 * @qdesc:	Queue descriptor
 * @msg:	QMTM free pool buffer message to fill in to queue
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to fill a buffer in its free pool queue.
 */
void xgene_qmtm_fp_dealloc_msg(struct xgene_qmtm_qdesc *qdesc,
			       struct xgene_qmtm_msg16 *msg)
{
	u32 qtail = qdesc->qtail;
	u32 count = qdesc->count;
	u8 *tailptr = (u8 *)&qdesc->msg16[qtail];

	memcpy(tailptr, msg, 16);

	if (++qtail == count)
		qtail = 0;

	writel(1, qdesc->command);
	qdesc->qtail = qtail;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_fp_dealloc_msg);

/**
 * xgene_qmtm_enqueue_msg - Enqueue a work message in subsystem's work queue
 * @qdesc:	Queue descriptor
 * @msg:	X-Gene SOC subsystem's work message to enqueue in to queue
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to enqueue work message in its work queue.
 */
void xgene_qmtm_enqueue_msg(struct xgene_qmtm_qdesc *qdesc,
			    struct xgene_qmtm_msg64 *msg)
{
	u32 qtail = qdesc->qtail;
	u32 count = qdesc->count;
	u8 *tailptr = (u8 *)&qdesc->msg32[qtail];

	memcpy(tailptr, msg, 32);

	if (++qtail == count)
		qtail = 0;

	if (!msg->msg32_1.msg16.NV) {
		writel(1, qdesc->command);
	} else {
		memcpy(tailptr + 32, (u8 *) msg + 32, 32);

		if (++qtail == count)
			qtail = 0;

		writel(2, qdesc->command);
	}

	qdesc->qtail = qtail;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_enqueue_msg);

/**
 * xgene_qmtm_dequeue_msg - Dequeue a work message from QMTM instance
 * @qdesc:	Queue descriptor
 * @msg:	Dequeued work message from X-Gene SOC subsystem to CPU
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to dequeue work message from its ingress
 * queue.
 *
 * Return:	0 on Success or -1 on Failure
 */
int xgene_qmtm_dequeue_msg(struct xgene_qmtm_qdesc *qdesc,
			   struct xgene_qmtm_msg64 *msg)
{
	u32 qhead = qdesc->qhead;
	u32 count = qdesc->count;
	u32 *headptr = (u32 *)&qdesc->msg32[qhead];

	if (headptr[EMPTY_SLOT_INDEX] == EMPTY_SLOT)
		return -EAGAIN;

	memcpy(msg, headptr, 32);
	headptr[EMPTY_SLOT_INDEX] = EMPTY_SLOT;

	if (++qhead == count)
		qhead = 0;

	if (!msg->msg32_1.msg16.NV) {
		writel(0xFFFFFFFF, qdesc->command);
	} else {
		headptr += 8;

		if (headptr[EMPTY_SLOT_INDEX] == EMPTY_SLOT)
			return -EAGAIN;

		memcpy((u8 *) msg + 32, headptr, 32);
		headptr[EMPTY_SLOT_INDEX] = EMPTY_SLOT;

		if (++qhead == count)
			qhead = 0;

		writel(0xFFFFFFFE, qdesc->command);
	}

	qdesc->qhead = qhead;

	return 0;
}
EXPORT_SYMBOL_GPL(xgene_qmtm_dequeue_msg);

/**
 * xgene_qmtm_get_sdev - Get slave context from slave name
 * @name:	Slave name
 *
 * This API will be called by APM X-Gene SOC Ethernet, PktDMA (XOR Engine),
 * and Security Engine subsystems to get its slave context from its name.
 *
 * Return:	Slave context on Success or NULL on Failure
 */
struct xgene_qmtm_sdev *xgene_qmtm_get_sdev(char *name)
{
	return storm_qmtm_get_sdev(name);
}
EXPORT_SYMBOL_GPL(xgene_qmtm_get_sdev);

static int xgene_qmtm_enable(struct xgene_qmtm *qmtm)
{
	struct xgene_qmtm_qinfo qinfo;
	struct device *dev = &qmtm->pdev->dev;
	int rc, mwait = 0;
	u32 val;
	u32 queue_id;

	qmtm->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(qmtm->clk)) {
		dev_err(dev, "can't get clock\n");
		return PTR_ERR(qmtm->clk);
	}

	rc = clk_prepare_enable(qmtm->clk);
	if (rc < 0) {
		dev_err(dev, "clock prepare enable failed");
		return rc;
	}

	xgene_qmtm_wr32(qmtm, QM_GLBL_DIAG_CSR_BASE_ADDR_OFFSET +
			QM_CFG_MEM_RAM_SHUTDOWN_ADDR, 0);
	do {
		/* Wait for Memory to come out of shutdown */
		usleep_range(1000, 2000);
		xgene_qmtm_rd32(qmtm, QM_GLBL_DIAG_CSR_BASE_ADDR_OFFSET +
				QM_CFG_MEM_RAM_SHUTDOWN_ADDR, &val);

		if (mwait++ >= 1000) {
			rc = -EIO;
			dev_err(dev, "RAM not out of shutdown %d\n", rc);
			clk_disable_unprepare(qmtm->clk);
			return rc;
		}
	} while (val == QM_CFG_MEM_RAM_SHUTDOWN_DEFAULT);

	switch (qmtm->qmtm_ip) {
	case QMTM0:
	case QMTM2:
		xgene_qmtm_rd32(qmtm, CSR_RECOMB_CTRL_0_ADDR, &val);
		val = RECOMB_EN0_SET(val, 1);
		xgene_qmtm_wr32(qmtm, CSR_RECOMB_CTRL_0_ADDR, val);
		break;
	case QMTM3:
		xgene_qmtm_wr32(qmtm, CSR_QMLITE_PBN_MAP_0_ADDR, 0x00000000);
	}

	/* program threshold set 1 and all hysteresis */
	xgene_qmtm_wr32(qmtm, CSR_THRESHOLD0_SET1_ADDR, 100);
	xgene_qmtm_wr32(qmtm, CSR_THRESHOLD1_SET1_ADDR, 200);
	xgene_qmtm_wr32(qmtm, CSR_HYSTERESIS_ADDR, 0xFFFFFFFF);

	/* Enable QPcore */
	xgene_qmtm_wr32(qmtm, CSR_QM_CONFIG_ADDR, QM_ENABLE_WR(1));

	/* Clear all HW queue state in case they were not de-activated */
	memset(&qinfo, 0, sizeof(qinfo));
	qinfo.qmtm = qmtm;

	for (queue_id = 0; queue_id < qmtm->max_queues; queue_id++) {
		qinfo.queue_id = queue_id;
		qmtm->write_qstate(&qinfo);
	}

	/* Enable error reporting */
	return xgene_qmtm_enable_error(qmtm);
}

static int xgene_qmtm_disable(struct xgene_qmtm *qmtm)
{
	u32 queue_id;

	/* Disable error reporting */
	xgene_qmtm_disable_error(qmtm);

	for (queue_id = 0; queue_id < qmtm->max_queues; queue_id++) {
		if (qmtm->qinfo[queue_id]) {
			dev_err(&qmtm->pdev->dev,
				"QMTM %d Queue ID %d Resource in use\n",
				qmtm->qmtm_ip, queue_id);
			return -EAGAIN;
		}
	}

	/* Disable QPcore */
	xgene_qmtm_wr32(qmtm, CSR_QM_CONFIG_ADDR, QM_ENABLE_WR(0));
	clk_disable_unprepare(qmtm->clk);

	return 0;
}

static struct xgene_qmtm *xgene_alloc_qmtm(struct platform_device *pdev)
{
	struct xgene_qmtm *qmtm;
	int max_queues, malloc_size;

	qmtm = devm_kzalloc(&pdev->dev, sizeof(struct xgene_qmtm), GFP_KERNEL);
	if (qmtm == NULL) {
		dev_err(&pdev->dev, "Unable to allocate QMTM context\n");
		return NULL;
	}

	qmtm->pdev = pdev;
	platform_set_drvdata(pdev, qmtm);
	max_queues = QMTM_MAX_QUEUES;
	malloc_size = max_queues * (sizeof(struct xgene_qmtm_info *));
	qmtm->qinfo = devm_kzalloc(&pdev->dev, malloc_size, GFP_KERNEL);
	if (qmtm->qinfo == NULL) {
		dev_err(&pdev->dev, "Unable to allocate QMTM Queue context\n");
		return NULL;
	}

	malloc_size = RES_WORD(max_queues + 31) * sizeof(u32);
	qmtm->queue_pool = devm_kzalloc(&pdev->dev, malloc_size, GFP_KERNEL);
	if (qmtm->queue_pool == NULL) {
		dev_err(&pdev->dev, "Unable to allocate QMTM Queue Pool\n");
		return NULL;
	}

	qmtm->max_queues = max_queues;

	return qmtm;
}

static int xgene_get_qmtm(struct xgene_qmtm *qmtm)
{
	struct platform_device *pdev = qmtm->pdev;
	struct resource *res;
	struct xgene_qmtm_sdev *sdev;
	const char *name;
	int rc, inum = 1;
	u16 pbn;

	/* Retrieve QM CSR register address and size */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get QMTM CSR region\n");
		return -ENODEV;
	}

	qmtm->csr_vaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(qmtm->csr_vaddr)) {
		dev_err(&pdev->dev, "Invalid QMTM CSR region\n");
		return PTR_ERR(qmtm->csr_vaddr);
	}

	/* Retrieve Primary Fabric address and size */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get QMTM Fabric region\n");
		return -ENODEV;
	}

	qmtm->fabric_vaddr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(qmtm->fabric_vaddr)) {
		dev_err(&pdev->dev, "Invalid QMTM Fabric region\n");
		return PTR_ERR(qmtm->fabric_vaddr);
	}

	rc = of_property_read_string(pdev->dev.of_node, "slave-name", &name);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to get QMTM Ingress slave-name\n");
		return rc;
	}

	sdev = xgene_qmtm_get_sdev((char *)name);
	if (sdev == NULL) {
		dev_err(&pdev->dev, "Ingress Slave error\n");
		return -ENODEV;
	}

	qmtm->idev = sdev;
	qmtm->qmtm_ip = sdev->qmtm_ip;

	for (pbn = sdev->wq_pbn_start; pbn < (sdev->wq_pbn_start +
					      sdev->wq_pbn_count);
	     pbn++, inum++) {
		int irq = platform_get_irq(pdev, inum);

		if (irq < 0) {
			dev_err(&pdev->dev, "Failed to map QMTM%d PBN %d IRQ\n",
				qmtm->qmtm_ip, pbn);
			continue;
		}

		qmtm->dequeue_irq[pbn] = irq;
	}

	return rc;
}

static int xgene_qmtm_probe(struct platform_device *pdev)
{
	struct xgene_qmtm *qmtm;
	int rc;

	qmtm = xgene_alloc_qmtm(pdev);
	if (qmtm == NULL)
		return -ENOMEM;

	rc = xgene_get_qmtm(qmtm);
	if (rc)
		return rc;

	return xgene_qmtm_enable(qmtm);
}

static int xgene_qmtm_remove(struct platform_device *pdev)
{
	struct xgene_qmtm *qmtm = platform_get_drvdata(pdev);
	return xgene_qmtm_disable(qmtm);
}

static struct of_device_id xgene_qmtm_match[] = {
	{.compatible = "apm,xgene-qmtm-xge0",},
	{.compatible = "apm,xgene-qmtm-soc",},
	{.compatible = "apm,xgene-qmtm-xge2",},
	{.compatible = "apm,xgene-qmtm-lite",},
	{},
};

MODULE_DEVICE_TABLE(of, xgene_qmtm_match);

static struct platform_driver xgene_qmtm_driver = {
	.driver = {
		   .name = XGENE_QMTM_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = xgene_qmtm_match,
		   },
	.probe = xgene_qmtm_probe,
	.remove = xgene_qmtm_remove,
};

module_platform_driver(xgene_qmtm_driver);

MODULE_VERSION(XGENE_QMTM_DRIVER_VER);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ravi Patel <rapatel@apm.com>");
MODULE_DESCRIPTION(XGENE_QMTM_DRIVER_DESC);
