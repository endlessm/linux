/*
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

#ifndef __XGENE_QMTM_H__
#define __XGENE_QMTM_H__

/* QMTM Queue types */
enum xgene_qmtm_qtype {
	QTYPE_DISABLED,		/* Queue Type is un-configured or disabled */
	QTYPE_PQ,		/* Queue Type is Physical Work Queue */
	QTYPE_FP,		/* Queue Type is Free Pool Queue */
	QTYPE_VQ,		/* Queue Type is Virtual Queue */
	QTYPE_MAX,
};

/* QMTM Queue possible sizes */
enum xgene_qmtm_qsize {
	QSIZE_512B,
	QSIZE_2KB,
	QSIZE_16KB,
	QSIZE_64KB,
	QSIZE_512KB,
	QSIZE_MAX,
};

/* QMTM Queue Access Method */
enum xgene_qmtm_qaccess {
	QACCESS_ALT,		/* Alternate enq/deq */
	QACCESS_QMI,		/* Access using QMI interface */
	QACCESS_MBOX,		/* Access using mailboxes */
	QACCESS_MAX,
};

/* QMTM Data Length encoded as per QM message format */
enum xgene_qmtm_data_len {
	DATA_LEN_256B = 0x0100,
	DATA_LEN_1K = 0x0400,
	DATA_LEN_2K = 0x0800,
	DATA_LEN_4K = 0x1000,
	DATA_LEN_16K = 0x4000,
};

enum xgene_qmtm_mask_len {
	MASK_LEN_256B = (DATA_LEN_256B - 1),
	MASK_LEN_1K = (DATA_LEN_1K - 1),
	MASK_LEN_2K = (DATA_LEN_2K - 1),
	MASK_LEN_4K = (DATA_LEN_4K - 1),
	MASK_LEN_16K = (DATA_LEN_16K - 1),
};

/* QMTM Buffer Length encoded as per QM message format */
enum xgene_qmtm_buf_len {
	BUF_LEN_256B = 0x7000,
	BUF_LEN_1K = 0x6000,
	BUF_LEN_2K = 0x5000,
	BUF_LEN_4K = 0x4000,
	BUF_LEN_16K = 0x0000,
};

/* QMTM messaging structures */
/* 16 byte QMTM message format */
struct xgene_qmtm_msg16 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	/* memory word 0 (bit 31:0) */
	u32 UserInfo;

	/* memory word 1 (bit 63:32) */
	u32 HL:1;
	u32 LErr:3;
	u32 RType:4;
	u32 IN:1;
	u32 Rv:1;
	u32 HB:1;
	u32 PB:1;
	u32 LL:1;
	u32 NV:1;
	u32 LEI:2;
	u32 ELErr:2;
	u32 Rv2:2;
	u32 FPQNum:12;

	/* memory word 2 (bit 95:64) */
	u32 DataAddrL;    /* split 10/32 */

	/* memory word 3 (bit 127:96) */
	u32 C:1;
	u32 BufDataLen:15;
	u32 Rv6:6;
	u32 DataAddrH:10; /* split 10/32 */
#else
	/* memory word 0 (bit 31:0) */
	u32 UserInfo;

	/* memory word 1 (bit 63:32) */
	u32 FPQNum:12;
	u32 Rv2:2;
	u32 ELErr:2;
	u32 LEI:2;
	u32 NV:1;
	u32 LL:1;
	u32 PB:1;
	u32 HB:1;
	u32 Rv:1;
	u32 IN:1;
	u32 RType:4;
	u32 LErr:3;
	u32 HL:1;

	/* memory word 2 (bit 95:64) */
	u32 DataAddrL;    /* split 10/32 */

	/* memory word 3 (bit 127:96) */
	u32 DataAddrH:10; /* split 10/32 */
	u32 Rv6:6;
	u32 BufDataLen:15;
	u32 C:1;
#endif
} __packed;

/* Upper 16 byte portion of 32 byte of QMTM message format */
struct xgene_qmtm_msg_up16 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	/* memory word 4 (bit 159:128) */
	u32 H0Info_msbL;    /* split 16/32 */

	/* memory word 5 (bit 191:160) */
	u32 HR:1;
	u32 Rv0:1;
	u32 DR:1;
	u32 Rv1:1;
	u32 TotDataLengthLinkListLSBs:12;
	u16 H0Info_msbH; /* split 16/32 */

	/* memory word 6 (bit 223:192) */
	u32 H0Info_lsbL; /* split 16/32 */

	/* memory word 7 (bit 255:224) */
	u32 H0FPSel:4;
	u32 H0Enq_Num:12;
	u16 H0Info_lsbH; /* split 16/32 */
#else
	/* memory word 4 (bit 159:128) */
	u32 H0Info_msbL; /* split 16/32 */

	/* memory word 5 (bit 191:160) */
	u16 H0Info_msbH; /* split 16/32 */
	u32 TotDataLengthLinkListLSBs:12;
	u32 Rv1:1;
	u32 DR:1;
	u32 Rv0:1;
	u32 HR:1;

	/* memory word 6 (bit 223:192) */
	u32 H0Info_lsbL; /* split 16/32 */

	/* memory word 7 (bit 255:224) */
	u16 H0Info_lsbH; /* split 16/32 */
	u32 H0Enq_Num:12;
	u32 H0FPSel:4;
#endif
} __packed;

/* 8 byte portion of QMTM extended (64B) message format */
struct xgene_qmtm_msg_ext8 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	u32 NxtDataAddrL;
	u32 Rv1:1;
	u32 NxtBufDataLength:15;
	u32 NxtFPQNum:4;
	u32 Rv2:2;
	u32 NxtDataAddrH:10;
#else
	u32 NxtDataAddrL;
	u32 NxtDataAddrH:10;
	u32 Rv2:2;
	u32 NxtFPQNum:4;
	u32 NxtBufDataLength:15;
	u32 Rv1:1;
#endif
} __packed;

/* 8 byte Link list portion of QMTM extended (64B) message format */
struct xgene_qmtm_msg_ll8 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	u32 NxtDataPtrL;
	u8  TotDataLengthLinkListMSBs;
	u8  NxtLinkListength;
	u32 NxtFPQNum:4;
	u32 Rv2:2;
	u32 NxtDataPtrH:10;
#else
	u32 NxtDataPtrL;
	u32 NxtDataPtrH:10;
	u32 Rv2:2;
	u32 NxtFPQNum:4;
	u8  NxtLinkListength;
	u8  TotDataLengthLinkListMSBs;
#endif
} __packed;

/* This structure represents 32 byte QMTM message format */
struct xgene_qmtm_msg32 {
	struct xgene_qmtm_msg16 msg16;
	struct xgene_qmtm_msg_up16 msgup16;
} __packed;

 /* 32 byte of QMTM extended (64B) message format */
struct xgene_qmtm_msg_ext32 {
	struct xgene_qmtm_msg_ext8 msg8_2;
	struct xgene_qmtm_msg_ext8 msg8_1;
	union {
		struct xgene_qmtm_msg_ext8 msg8_4;
		struct xgene_qmtm_msg_ll8 msg8_ll;
	};
	struct xgene_qmtm_msg_ext8 msg8_3;
} __packed;

/* 64 byte QMTM message format */
struct xgene_qmtm_msg64 {
	struct xgene_qmtm_msg32 msg32_1;
	struct xgene_qmtm_msg_ext32 msg32_2;
} __packed;

#ifdef CONFIG_CPU_BIG_ENDIAN
#define xgene_qmtm_msg_le32(word, words) \
	do { \
		int w; \
		for (w = 0; w < words; w++) \
			*(word + w)= cpu_to_le32(*(word + w)); \
	} while (0);
#else
#define xgene_qmtm_msg_le32(word, words) \
	do {} while (0);
#endif

/* Empty Slot Soft Signature */
#define EMPTY_SLOT_INDEX	7
#define EMPTY_SLOT		0x22222222

/* Destination QM, 2 MSb in work queue, dstqid */
#define QMTM_QUEUE_ID(qm, qid)	(((u16)(qm) << 10) | qid)

/* QMTM Slave Device Information */
struct xgene_qmtm_sdev {
	u8 qmtm_ip;
	u8 slave;
	u8 wq_pbn_start;
	u8 wq_pbn_count;
	u8 fq_pbn_start;
	u8 fq_pbn_count;
	u16 slave_id;		/* slave id see xgene_qmtm_slave_id */
	u32 wq_pbn_pool;	/* Bit mask indicates in use WQ PBN */
	u32 fq_pbn_pool;	/* Bit mask indicates in use FP PBN */
	char *name;
	char *compatible;
	struct xgene_qmtm *qmtm;
	struct xgene_qmtm_sdev *idev;
};

/* QMTM Queue Information structure */
/* Per queue descriptor */
struct xgene_qmtm_qdesc {
	u16 qhead;
	u16 qtail;
	u16 count;
	u16 irq;
	void *command;
	union {
		void *qvaddr;
		struct xgene_qmtm_msg16 *msg16;
		struct xgene_qmtm_msg32 *msg32;
	};
};

/* Per queue state database */
struct xgene_qmtm_qinfo {
	u8 slave;
	u8 qtype;
	u8 qsize;
	u8 qaccess;
	u8 flags;
	u8 qmtm_ip;
	u8 slave_id;
	u8 pbn;
	u16 queue_id;
	u16 nummsgs;
	u32 pbm_state;
	u64 qpaddr;
	void *qfabric;
	u32 qstate[6];
	struct xgene_qmtm_qdesc *qdesc;
	struct xgene_qmtm_sdev *sdev;
	struct xgene_qmtm *qmtm;
};

/* QMTM Queue Information flags */
#define XGENE_SLAVE_PB_CONFIGURE	0x01
#define XGENE_SLAVE_Q_ADDR_ALLOC	0x02
#define XGENE_SLAVE_DEFAULT_FLAGS	(XGENE_SLAVE_PB_CONFIGURE | \
	XGENE_SLAVE_Q_ADDR_ALLOC)

static inline u16 xgene_qmtm_encode_bufdatalen(u32 len)
{
	if (len <= DATA_LEN_256B)
		return BUF_LEN_256B | (len & MASK_LEN_256B);
	else if (len <= DATA_LEN_1K)
		return BUF_LEN_1K | (len & MASK_LEN_1K);
	else if (len <= DATA_LEN_2K)
		return BUF_LEN_2K | (len & MASK_LEN_2K);
	else if (len <= DATA_LEN_4K)
		return BUF_LEN_4K | (len & MASK_LEN_4K);
	else if (len < DATA_LEN_16K)
		return BUF_LEN_16K | (len & MASK_LEN_16K);
	else
		return BUF_LEN_16K;
}

static inline u16 xgene_qmtm_encode_datalen(u32 len)
{
	return len & MASK_LEN_16K;
}

static inline u32 xgene_qmtm_decode_datalen(u16 bufdatalen)
{
	switch (bufdatalen & BUF_LEN_256B) {
	case BUF_LEN_256B:
		return bufdatalen & MASK_LEN_256B ? : DATA_LEN_256B;
	case BUF_LEN_1K:
		return bufdatalen & MASK_LEN_1K ? : DATA_LEN_1K;
	case BUF_LEN_2K:
		return bufdatalen & MASK_LEN_2K ? : DATA_LEN_2K;
	case BUF_LEN_4K:
		return bufdatalen & MASK_LEN_4K ? : DATA_LEN_4K;
	default:
		return bufdatalen & MASK_LEN_16K ? : DATA_LEN_16K;
	};
}

struct xgene_qmtm_sdev *xgene_qmtm_get_sdev(char *name);

int xgene_qmtm_set_qinfo(struct xgene_qmtm_qinfo *qinfo);

void xgene_qmtm_clr_qinfo(struct xgene_qmtm_qinfo *qinfo);

void xgene_qmtm_read_qstate(struct xgene_qmtm_qinfo *qinfo);

void xgene_qmtm_fp_dealloc_msg(struct xgene_qmtm_qdesc *qdesc,
			       struct xgene_qmtm_msg16 *msg);

void xgene_qmtm_enqueue_msg(struct xgene_qmtm_qdesc *qdesc,
			    struct xgene_qmtm_msg64 *msg);

int xgene_qmtm_dequeue_msg(struct xgene_qmtm_qdesc *qdesc,
			   struct xgene_qmtm_msg64 *msg);

int xgene_qmtm_intr_coalesce(struct xgene_qmtm_qinfo *qinfo, u8 tap);

#endif /* __XGENE_QMTM_H__ */
