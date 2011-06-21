/* Copyright 2009-2011 Freescale Semiconductor, Inc.
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

#ifndef FSL_PME_H
#define FSL_PME_H

/* pme_fd_res_status() returns this enum */
enum pme_status {
	pme_status_ok = 0x00,
	pme_status_kes_ccl = 0x40, /* KES Confidence Collision Limit */
	pme_status_kes_cme = 0x41, /* KES Confidence Mask Error */
	pme_status_dxe_ire = 0x48, /* DXE Invalid Repeat Error */
	pme_status_dxe_tlse = 0x49, /* DXE Test Line Syntax Error */
	pme_status_dxe_ile = 0x4b, /* DXE Instruction Limit Error */
	pme_status_dxe_pdsrsore = 0x4c, /* DXE PDSR Space Out Range Error */
	pme_status_dxe_soe = 0x4d, /* DXE Stack Overflow Error */
	pme_status_dxe_alse = 0x4e, /* DXE Alternate Link Same Error */
	pme_status_dxe_slse = 0x4f, /* DXE Subsequent Link Same Error */
	pme_status_dxe_slre = 0x50, /* DXE Subsequent Link Reverse Error */
	pme_status_dxe_itlb = 0x51, /* DXE Invalid Test Line Branch */
	pme_status_dxe_cle = 0x52, /* DXE Compare Limit Exceeded */
	pme_status_dxe_mle = 0x53, /* DXE Match Limit Exceeded */
	pme_status_sre_irhbni = 0x59, /* SRE Invalid Reaction Head Block */
				      /* Number Instructions */
	pme_status_sre_rl = 0x5a, /* SRE Reaction Limit */
	pme_status_sre_pdsrsore = 0x5b, /* SRE PDSR Space Out Range Error */
	pme_status_sre_score = 0x5c, /* SRE Session Context Out Range Error */
	pme_status_sre_ctore = 0x5d, /* SRE Context Table Out Range Error */
	pme_status_sre_il = 0x5e, /* SRE Instruction Limit */
	pme_status_sre_iij = 0x5f, /* SRE Invalid Instruction Jump */
	pme_status_sre_ise = 0x60, /* SRE Instruction Syntax Error */
	pme_status_pmfa_pmtcce = 0x80, /* PMFA PCTCC Error */
	pme_status_pmfa_fcwe = 0x90, /* PMFA Flow Context Write Command Error */
	pme_status_pmfa_fcre = 0x91, /* PMFA Flow Context Read Command Error */
	pme_status_pmfa_ume = 0x93, /* PMFA Unrecognized Mode Error */
	pme_status_pmfa_uce = 0x94, /* PMFA Unrecognized Command Error */
	pme_status_pmfa_ufe = 0x95, /* PMFA Unrecognized Frame Error */
	pme_status_sre_csmre = 0xc0, /* SRE Context System Memory Read Error */
	pme_status_sre_ismre = 0xc1, /* SRE Instruction System Memory Read */
				     /* Error */
	pme_status_dxe_smre = 0xc2, /* DXE System Memory Read Error */
	pme_status_pmfa_pmtccsmre = 0xc4, /* PMFA PMTCC System Memory Read */
					  /* Error */
	pme_status_pmfa_csmre = 0xc5, /* PMFA Context System Memory Read */
				      /* Error */
	pme_status_pmfa_dsmre = 0xc6, /* PMFA Data System Memory Read Error */
	pme_status_kes_cmecce = 0xd2, /* KES Confidence Memory ECC Error */
	pme_status_kes_2btmecce = 0xd4, /*KES 2-Byte Trigger Memory ECC Error */
	pme_status_kes_vltmecce = 0xd5, /*KES Variable Length Trigger Memory */
					/* ECC Error */
	pme_status_pmfa_cmecce = 0xd7, /* PMFA Confidence Memory ECC Error */
	pme_status_pmfa_2btmecce = 0xd9, /* PMFA 2-Byte Trigger Memory ECC */
					 /* Error */
	pme_status_pmfa_vltmecce = 0xda, /* PMFA Variable Length Trigger */
					  /* Memory ECC Error */
	pme_status_dxe_iemce = 0xdb, /* DXE Internal Examination Memory */
				     /* Collision Error */
	pme_status_dxe_iemecce = 0xdc, /* DXE Internal Examination Memory */
				       /* ECC Error */
	pme_status_dxe_icmecce = 0xdd, /* DXE Internal Context Memory ECC */
				       /* Error */
	pme_status_sre_ctsmwe = 0xe0, /* SRE Context Table System Memory */
				      /* Write Error */
	pme_status_pmfa_pmtccsmwe = 0xe7, /* PMFA PMTCC System Memory Write */
					  /* Error */
	pme_status_pmfa_csmwe = 0xe8, /* PMFA Context System Memory Write */
				      /* Error */
	pme_status_pmfa_dsmwe = 0xe9, /* PMFA Data System Memory Write Error */
};

/* pme_fd_res_flags() returns these flags */
#define PME_STATUS_UNRELIABLE	0x80
#define PME_STATUS_TRUNCATED	0x10
#define PME_STATUS_MASK		0x90

/**************/
/* USER SPACE */
/**************/

#define PME_IOCTL_MAGIC 'p'

/* Wrapper for a pointer and size. */
struct pme_buffer {
	void __user *data;
	size_t size;
};

/***************/
/* SCAN DEVICE */
/***************/
/* The /dev/pme_scan device creates a file-descriptor that uses scheduled FQs
 * serviced by PME's datapath portal. This can only be used for scanning. */
#define PME_DEV_SCAN_NODE	"pme_scan"
#define PME_DEV_SCAN_PATH	"/dev/" PME_DEV_SCAN_NODE

/* ioctls for 'scan' device */
#define PMEIO_SETSCAN	_IOW(PME_IOCTL_MAGIC, 0x06, struct pme_scan_params)
#define PMEIO_GETSCAN	_IOR(PME_IOCTL_MAGIC, 0x07, struct pme_scan_params)
#define PMEIO_RESETSEQ	_IO(PME_IOCTL_MAGIC, 0x08)
#define PMEIO_RESETRES	_IO(PME_IOCTL_MAGIC, 0x09)
#define PMEIO_SCAN_W1	_IOW(PME_IOCTL_MAGIC, 0x0a, struct pme_scan_cmd)
#define PMEIO_SCAN_Wn	_IOWR(PME_IOCTL_MAGIC, 0x0b, struct pme_scan_cmds)
#define PMEIO_SCAN_R1	_IOR(PME_IOCTL_MAGIC, 0x0c, struct pme_scan_result)
#define PMEIO_SCAN_Rn	_IOWR(PME_IOCTL_MAGIC, 0x0d, struct pme_scan_results)
#define PMEIO_SCAN	_IOWR(PME_IOCTL_MAGIC, 0x0e, struct pme_scan)
/* The release_bufs ioctl takes as parameter a (void *) */
#define PMEIO_RELEASE_BUFS _IOW(PME_IOCTL_MAGIC, 0x0f, void *)

/* Parameters for PMEIO_SETSCAN and PMEIO_GETSCAN ioctl()s. This doesn't cover
 * "sequence" fields ('soc' and 'seqnum'), they can only be influenced by flags
 * passed to scan operations, or by PMEIO_RESETSEQ ioctl()s. */
struct pme_scan_params {
	__u32 flags; /* PME_SCAN_PARAMS_*** bitmask */
	struct pme_scan_params_residue {
		__u8 enable; /* boolean, residue enable */
		__u8 length; /* read-only for GETSCAN, ignored for SETSCAN */
	} residue;
	struct pme_scan_params_sre {
		__u32 sessionid; /* 27-bit */
		__u8 verbose; /* 0-3 */
		__u8 esee; /* boolean, End Of Sui Event Enable */
	} sre;
	struct pme_scan_params_dxe {
		__u16 clim; /* compare limit */
		__u16 mlim; /* match limit */
	} dxe;
	struct pme_scan_params_pattern {
		__u8 set;
		__u16 subset;
	} pattern;
};
#define PME_SCAN_PARAMS_RESIDUE	0x00000001
#define PME_SCAN_PARAMS_SRE	0x00000002
#define PME_SCAN_PARAMS_DXE	0x00000004
#define PME_SCAN_PARAMS_PATTERN	0x00000008

/* argument to PMEIO_SCAN_W1 ioctl */
struct pme_scan_cmd {
	__u32 flags; /* PME_SCAN_CMD_*** bitmask */
	void *opaque; /* value carried through in the pme_scan_result */
	struct pme_buffer input;
	struct pme_buffer output; /* ignored for 'RES_BMAN' output */
};

#define PME_SCAN_CMD_RES_BMAN	0x00000001 /* use Bman for output */
#define PME_SCAN_CMD_STARTRESET	0x00000002
#define PME_SCAN_CMD_END	0x00000004

/* argument to PMEIO_SCAN_Wn ioctl
 * 'num' indicates how many 'cmds' are present on input and is updated on the
 * response to indicate how many were sent. */
struct pme_scan_cmds {
	unsigned num;
	struct pme_scan_cmd __user *cmds;
};

/* argument to PMEIO_SCAN_R1 ioctl. The ioctl doesn't read any of these
 * fields, they are only written to. If the output comes from BMAN buffer
 * then 'flags' will have PME_SCAN_RESULT_BMAN set. */
struct pme_scan_result {
	__u8 flags; /* PME_SCAN_RESULT_*** bitmask */
	enum pme_status status;
	struct pme_buffer output;
	void *opaque; /* value carried from the pme_scan_cmd */
};
#define PME_SCAN_RESULT_UNRELIABLE	PME_STATUS_UNRELIABLE
#define PME_SCAN_RESULT_TRUNCATED	PME_STATUS_TRUNCATED
#define PME_SCAN_RESULT_BMAN		0x01

/* argument to PMEIO_SCAN_Rn ioctl.
 * 'num' indicates how many 'cmds' are present on input and is updated on the
 * response to indicate how many were retrieved. */
struct pme_scan_results {
	unsigned num;
	struct pme_scan_result *results;
};

/* argument to PMEIO_SCANWR ioctl. */
struct pme_scan {
	struct pme_scan_cmd cmd;
	struct pme_scan_result result;
};

/*************/
/* DB DEVICE */
/*************/
/* The /dev/pme_db device creates a file-descriptor that uses parked FQs
 * serviced by the PME's EFQC (Exclusive Frame Queue Control) mechanism. This is
 * usually for PMTCC commands for programming the database, though can also be
 * used for high-priority scanning. This device would typically require root
 * perms. The EFQC exclusivity is reference-counted, so by default is asserted
 * on-demand and released when processing quiesces for the context, but
 * exclusivity can be maintained across inter-frame gaps using the INC and DEC
 * ioctls, which provide supplementary increments and decrements of the
 * reference count. */
#define PME_DEV_DB_NODE	"pme_db"
#define PME_DEV_DB_PATH	"/dev/" PME_DEV_DB_NODE

/* ioctls for 'db' device */
#define PMEIO_EXL_INC	_IO(PME_IOCTL_MAGIC, 0x00)
#define PMEIO_EXL_DEC	_IO(PME_IOCTL_MAGIC, 0x01)
#define PMEIO_EXL_GET	_IOR(PME_IOCTL_MAGIC, 0x02, int)
#define PMEIO_PMTCC	_IOWR(PME_IOCTL_MAGIC, 0x03, struct pme_db)
#define PMEIO_SRE_RESET	_IOR(PME_IOCTL_MAGIC, 0x04, struct pme_db_sre_reset)
#define PMEIO_NOP	_IO(PME_IOCTL_MAGIC, 0x05)

/* Database structures */
#define PME_DB_RESULT_UNRELIABLE	PME_STATUS_UNRELIABLE
#define PME_DB_RESULT_TRUNCATED		PME_STATUS_TRUNCATED

struct pme_db {
	struct pme_buffer input;
	struct pme_buffer output;
	__u8 flags; /* PME_DB_RESULT_*** bitmask */
	enum pme_status status;
};

/* This is related to the sre_reset ioctl */
#define PME_SRE_RULE_VECTOR_SIZE  8
struct pme_db_sre_reset {
	__u32 rule_vector[PME_SRE_RULE_VECTOR_SIZE];
	__u32 rule_index;
	__u16 rule_increment;
	__u32 rule_repetitions;
	__u16 rule_reset_interval;
	__u8 rule_reset_priority;
};

/****************/
/* KERNEL SPACE */
/****************/

#ifdef __KERNEL__

#include <linux/fsl_qman.h>
#include <linux/fsl_bman.h>

/* "struct pme_hw_flow" represents a flow-context resource for h/w, whereas
 * "struct pme_flow" (below) is the s/w type used to provide (and receive)
 * parameters to(/from) the h/w resource. */
struct pme_hw_flow;

/* "struct pme_hw_residue" represents a residue resource for h/w. */
struct pme_hw_residue;

/* This is the pme_flow structure type, used for querying or updating a PME flow
 * context */
struct pme_flow {
	u8 sos:1;
	u8 __reserved1:1;
	u8 srvm:2;
	u8 esee:1;
	u8 __reserved2:3;
	u8 ren:1;
	u8 rlen:7;
	/* Sequence Number (48-bit) */
	u16 seqnum_hi;
	u32 seqnum_lo;
	u32 __reserved3;
	u32 sessionid:27;
	u32 __reserved4:5;
	u16 __reserved5;
	/* Residue pointer (48-bit), ignored if ren==0 */
	u16 rptr_hi;
	u32 rptr_lo;
	u16 clim;
	u16 mlim;
	u32 __reserved6;
} __packed;
static inline u64 pme_flow_seqnum_get64(const struct pme_flow *p)
{
	return ((u64)p->seqnum_hi << 32) | (u64)p->seqnum_lo;
}
static inline u64 pme_flow_rptr_get64(const struct pme_flow *p)
{
	return ((u64)p->rptr_hi << 32) | (u64)p->rptr_lo;
}
/* Macro, so we compile better if 'v' isn't always 64-bit */
#define pme_flow_seqnum_set64(p, v) \
	do { \
		struct pme_flow *__p931 = (p); \
		__p931->seqnum_hi = upper_32_bits(v); \
		__p931->seqnum_lo = lower_32_bits(v); \
	} while (0)
#define pme_flow_rptr_set64(p, v) \
	do { \
		struct pme_flow *__p931 = (p); \
		__p931->rptr_hi = upper_32_bits(v); \
		__p931->rptr_lo = lower_32_bits(v); \
	} while (0)

/* pme_ctx_ctrl_update_flow(), pme_fd_cmd_fcw() and pme_scan_params::flags
 * use these; */
#define PME_CMD_FCW_RES	0x80	/* "Residue": ren, rlen */
#define PME_CMD_FCW_SEQ	0x40	/* "Sequence": sos, sequnum */
#define PME_CMD_FCW_SRE	0x20	/* "Stateful Rule": srvm, esee, sessionid */
#define PME_CMD_FCW_DXE	0x10	/* "Data Examination": clim, mlim */
#define PME_CMD_FCW_ALL 0xf0

/* pme_ctx_scan() and pme_fd_cmd_scan() use these; */
#define PME_CMD_SCAN_SRVM(n) ((n) << 3) /* n in [0..3] */
#define PME_CMD_SCAN_FLUSH 0x04
#define PME_CMD_SCAN_SR    0x02 /* aka "Start of Flow or Reset */
#define PME_CMD_SCAN_E     0x01 /* aka "End of Flow */

/***********************/
/* low-level functions */
/***********************/

/* (De)Allocate PME hardware resources */
struct pme_hw_residue *pme_hw_residue_new(void);
void pme_hw_residue_free(struct pme_hw_residue *);
struct pme_hw_flow *pme_hw_flow_new(void);
void pme_hw_flow_free(struct pme_hw_flow *);

/* Initialise a flow context to known default values */
void pme_sw_flow_init(struct pme_flow *);

/* Fill in an "Initialise FQ" management command for a PME input FQ. NB, the
 * caller is responsible for setting the following fields, they will not be set
 * by the API;
 *   - initfq->fqid, the frame queue to be initialised
 *   - initfq->count, should most likely be zero. A count of 0 initialises 1 FQ,
 *			a count of 1 initialises 2 FQs, etc/
 * The 'qos' parameter indicates which workqueue in the PME channel the
 * FQ should schedule to for regular scanning (0..7). If 'flow' is non-NULL the
 * FQ is configured for Flow Mode, otherwise it is configured for Direct Action
 * Mode. 'bpid' is the buffer pool ID to use when Bman-based output is
 * produced, and 'rfqid' is the frame queue ID to enqueue output frames to.
 * Following this api, when calling qm_mc_commit(), use QM_MCC_VERB_INITFQ_SCHED
 * for regular PMEscanning or QM_MCC_VERB_INITFQ_PARK for exclusive PME
 * processing (usually PMTCC).*/
void pme_initfq(struct qm_mcc_initfq *initfq, struct pme_hw_flow *flow, u8 qos,
		u8 rbpid, u32 rfqid);

/* Given a dequeued frame from PME, return status/flags */
static inline enum pme_status pme_fd_res_status(const struct qm_fd *fd)
{
	return (enum pme_status)(fd->status >> 24);
}
static inline u8 pme_fd_res_flags(const struct qm_fd *fd)
{
	return (fd->status >> 16) & PME_STATUS_MASK;
}

/* Fill in a frame descriptor for a NOP command. */
void pme_fd_cmd_nop(struct qm_fd *fd);

/* Fill in a frame descriptor for a Flow Context Write command. NB, the caller
 * is responsible for setting all the relevant fields in 'flow', only the
 * following fields are set by the API;
 *   - flow->rptr_hi
 *   - flow->rptr_lo
 * The fields in 'flow' are divided into 4 groups, 'flags' indicates which of
 * them should be written to the h/w flow context using PME_CMD_FCW_*** defines.
 * 'residue' should be non-NULL iff flow->ren is non-zero and PME_CMD_FCW_RES is
 * set. */
void pme_fd_cmd_fcw(struct qm_fd *fd, u8 flags, struct pme_flow *flow,
		struct pme_hw_residue *residue);

/* Fill in a frame descriptor for a Flow Context Read command. */
void pme_fd_cmd_fcr(struct qm_fd *fd, struct pme_flow *flow);

/* Modify a frame descriptor for a PMTCC command (only modifies 'cmd' field) */
void pme_fd_cmd_pmtcc(struct qm_fd *fd);

/* Modify a frame descriptor for a Scan command (only modifies 'cmd' field).
 * 'flags' are chosen from PME_CMD_SCAN_*** symbols. NB, the use of the
 * intermediary representation (and PME_SCAN_ARGS) improves performance - ie.
 * if the scan params are essentially constant, this compacts them for storage
 * into the same format used in the interface to h/w. So it reduces parameter
 * passing, stack-use, and encoding time. */
#define PME_SCAN_ARGS(flags, set, subset) \
({ \
	u8 __flags461 = (flags); \
	u8 __set461 = (set); \
	u16 __subset461 = (subset); \
	u32 __res461 = ((u32)__flags461 << 24) | \
			((u32)__set461 << 16) | \
			(u32)__subset461; \
	__res461; \
})
void pme_fd_cmd_scan(struct qm_fd *fd, u32 args);

/* convert pointer to physical address for use by PME */
dma_addr_t pme_map(void *ptr);
int pme_map_error(dma_addr_t dma_addr);

enum pme_cmd_type {
	pme_cmd_nop = 0x7,
	pme_cmd_flow_read = 0x5,	/* aka FCR */
	pme_cmd_flow_write = 0x4,	/* aka FCW */
	pme_cmd_pmtcc = 0x1,
	pme_cmd_scan = 0
};

/************************/
/* high-level functions */
/************************/

/* predeclaration of a private structure" */
struct pme_ctx;
struct pme_nostash;

/* Calls to pme_ctx_scan() and pme_ctx_pmtcc() provide these, and they are
 * provided back in the completion callback. You can embed this within a larger
 * structure in order to maintain per-command data of your own. The fields are
 * owned by the driver until the callback is invoked, so for example do not link
 * this token into a list while the command is in-flight! */
struct pme_ctx_token {
	u32 blob[4];
	struct list_head node;
	enum pme_cmd_type cmd_type:8;
	u8 is_disable_flush;
};

struct pme_ctx_ctrl_token {
	void (*cb)(struct pme_ctx *, const struct qm_fd *,
			struct pme_ctx_ctrl_token *);
	void (*ern_cb)(struct pme_ctx *, const struct qm_mr_entry *,
			struct pme_ctx_ctrl_token *);
	/* don't touch the rest */
	struct pme_hw_flow *internal_flow_ptr;
	struct pme_flow *usr_flow_ptr;
	struct pme_ctx_token base_token;
};

/* Scan results invoke a user-provided callback of this type */
typedef void (*pme_scan_cb)(struct pme_ctx *, const struct qm_fd *,
				struct pme_ctx_token *);
/* Enqueue rejections may happen before order-restoration or after (eg. if due
 * to congestion or tail-drop). Use * 'rc' code of the 'mr_entry' to
 * determine. */
typedef void (*pme_scan_ern_cb)(struct pme_ctx *, const struct qm_mr_entry *,
				struct pme_ctx_token *);

/* PME "association" - ie. connects two frame-queues, with or without a PME flow
 * (if not, direct action mode), and manages mux/demux of scans and flow-context
 * updates. To allow state used by your callback to be stashed, as well as
 * optimising the PME driver and the Qman driver beneath it, embed this
 * structure as the first field in your own context structure. */
struct pme_ctx {
	struct qman_fq fq;
	/* IMPORTANT: Set (only) these two fields prior to calling *
	 * pme_ctx_init(). 'ern_cb' can be NULL if you know you will not
	 * receive enqueue rejections. */
	pme_scan_cb cb;
	pme_scan_ern_cb ern_cb;
	/* These fields should not be manipulated directly. Also the structure
	 * may change and/or grow, so avoid making any alignment or size
	 * assumptions. */
	atomic_t refs;
	volatile u32 flags;
	spinlock_t lock;
	wait_queue_head_t queue;
	struct list_head tokens;
	/* TODO: the following "slow-path" values should be bundled into a
	 * secondary structure so that sizeof(struct pme_ctx) is minimised (for
	 * stashing of caller-side fast-path state). */
	struct pme_hw_flow *hw_flow;
	struct pme_hw_residue *hw_residue;
	struct qm_fqd_stashing stashing;
	struct qm_fd update_fd;
	struct pme_nostash *us_data;
};

/* Flags for pme_ctx_init() */
#define PME_CTX_FLAG_LOCKED      0x00000001 /* use QMAN_FQ_FLAG_LOCKED */
#define PME_CTX_FLAG_EXCLUSIVE   0x00000002 /* unscheduled, exclusive mode */
#define PME_CTX_FLAG_PMTCC       0x00000004 /* PMTCC rather than scanning */
#define PME_CTX_FLAG_DIRECT      0x00000008 /* Direct Action mode (not Flow) */
#define PME_CTX_FLAG_LOCAL       0x00000020 /* Ignore dest, use cpu portal */

/* Flags for operations */
#ifdef CONFIG_FSL_DPA_CAN_WAIT
#define PME_CTX_OP_WAIT          QMAN_ENQUEUE_FLAG_WAIT
#define PME_CTX_OP_WAIT_INT      QMAN_ENQUEUE_FLAG_WAIT_INT
#endif
#define PME_CTX_OP_RESETRESLEN   0x00000001 /* no en/disable, just set len */
/* Note that pme_ctx_ctrl_update_flow() also uses PME_CMD_FCW flags, so they
 * mustn't conflict with PME_CTX_OP_***.
 * Also, the above are defined to match QMAN_ENQUEUE values for optimisation
 * purposes (ie. fast-path operations that don't _WAIT will not incur PME->QMAN
 * flag conversion overheads). */

/**
 * pme_ctx_init - Initialise a PME context
 * @ctx: the context structure to initialise
 * @flags: bit-mask of PME_CTX_FLAG_*** options
 * @bpid: buffer pool ID used for any Bman-generated output
 * @qosin: workqueue priority on the PME channel (0-7)
 * @qosout: workqueue priority on the result channel (0-7)
 * @dest: channel to receive results from PME
 * @stashing: desired dequeue stashing behaviour
 *
 * This creates and initialises a PME context, composed of two FQs, an optional
 * flow-context, and scheduling parameters for the datapath. The ctx->cb and
 * ctx->pool fields must have been initialised prior to calling this api. The
 * initialised context is left 'disabled', meaning that the FQ towards PME is
 * Parked and no operations are possible. If PME_CTX_INIT_EXCLUSIVE is specified
 * in @flags, then the input FQ is not scheduled, otherwise enabling the context
 * will schedule the FQ to PME. Exclusive access is only available if the driver
 * is built with control functionality and if the operating system has access to
 * PME's CCSR map. @qosin applies if EXCLUSIVE is not set, and indicates which
 * of the PME's 8 prioritised workqueues the FQ should schedule to. @dest
 * indicates the channel that should receive results from PME, unless
 * PME_CTX_FLAG_LOCAL is set in which case this parameter is ignored and the
 * dedicated portal channel for the current cpu will be used instead. @qosout
 * indicates which of the 8 prioritised workqueus the FQ should schedule to on
 * the s/w portal. @stashing configures whether FQ context, frame data, and/or
 * frame annotation should be stashed into cpu cache when dequeuing output, and
 * if so, how many cachelines.  For the FQ context part, set the number of
 * cachelines to cover; 1. sizeof(struct qman_fq_base), to accelerate only Qman
 * driver processing, 2. sizeof(struct pme_ctx), to accelerate Qman and PME
 * driver processing, or 3. sizeof(<user-struct>), where <user-struct> is the
 * caller's structure of which the pme_ctx is the first member - this will allow
 * callbacks to operate on state which has a high probability of already being
 * in-cache.
 * Returns 0 on success.
 */
int pme_ctx_init(struct pme_ctx *ctx, u32 flags, u32 bpid, u8 qosin,
			u8 qosout, enum qm_channel dest,
			const struct qm_fqd_stashing *stashing);

/* Cleanup allocated resources */
void pme_ctx_finish(struct pme_ctx *ctx);

/* enable a context */
int pme_ctx_enable(struct pme_ctx *ctx);

/* disable a context
 * If it returns zero, the context is disabled.
 * If it returns +1, the context is disabling and the token's completion
 * callback will be invoked when disabling is complete.
 * Returns -EBUSY on error, in which case the context remains enabled.
 * If the PME_CTX_OP_WAIT flag is specified, it should only fail if
 * PME_CTX_OP_WAIT_INT is also specified and a signal is pending. */
int pme_ctx_disable(struct pme_ctx *ctx, u32 flags,
		struct pme_ctx_ctrl_token *token);

/* query whether a context is disabled. Returns > 0 if the ctx is disabled. */
int pme_ctx_is_disabled(struct pme_ctx *ctx);

/* query whether a context is in an error state. */
int pme_ctx_is_dead(struct pme_ctx *ctx);

/* A pre-condition for the following APIs is the ctx must be disabled
 * dest maybe ignored if the flags parameter indicated LOCAL during the
 * corresponding pme_ctx_init.
 */
int pme_ctx_reconfigure_tx(struct pme_ctx *ctx, u32 bpid, u8 qosin);
int pme_ctx_reconfigure_rx(struct pme_ctx *ctx, u8 qosout,
		enum qm_channel dest, const struct qm_fqd_stashing *stashing);

/* Precondition: pme_ctx must be enabled
 * if PME_CTX_OP_WAIT is specified, it'll wait (if it has to) to start the ctrl
 * command but never waits for it to complete. The callback serves that purpose.
 * NB: 'params' may be modified by this call. For instance if
 * PME_CTX_OP_RESETRESLEN was specified and residue is enabled, then the
 * params->ren will be set to 1 (in order not to disabled residue).
 * NB: _update() will overwrite the 'params->rptr_[hi/low]' fields since the
 * residue resource is managed by this layer.
 */
int pme_ctx_ctrl_update_flow(struct pme_ctx *ctx, u32 flags,
		struct pme_flow *params, struct pme_ctx_ctrl_token *token);
int pme_ctx_ctrl_read_flow(struct pme_ctx *ctx, u32 flags,
		struct pme_flow *params, struct pme_ctx_ctrl_token *token);
int pme_ctx_ctrl_nop(struct pme_ctx *ctx, u32 flags,
		struct pme_ctx_ctrl_token *token);

/* if PME_CTX_OP_WAIT is specified, it'll wait (if it has to) to start the scan
 * but never waits for it to complete. The scan callback serves that purpose.
 * 'fd' is modified by both these calls, but only the 'cmd' field. The 'args'
 * parameters is produced by the PME_SCAN_ARGS() inline function. */
int pme_ctx_scan(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd, u32 args,
		struct pme_ctx_token *token);
int pme_ctx_pmtcc(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd,
		struct pme_ctx_token *token);

/* This is extends pme_ctx_scan() to provide ORP support. 'orp_fq' represents
 * the FQD that is used as the ORP and 'seqnum' is the sequence number to use
 * for order restoration, these are usually the FQ the frame was dequeued from
 * and the sequence number of that dequeued frame (respectively). */
int pme_ctx_scan_orp(struct pme_ctx *ctx, u32 flags, struct qm_fd *fd, u32 args,
	       struct pme_ctx_token *token, struct qman_fq *orp_fq, u16 seqnum);

/* Precondition: must be PME_CTX_FLAG_EXCLUSIVE */
int pme_ctx_exclusive_inc(struct pme_ctx *ctx, u32 flags);
void pme_ctx_exclusive_dec(struct pme_ctx *ctx);

/* Does pme have access to ccsr */
int pme2_have_control(void);

/**************************/
/* control-plane only API */
/**************************/
#ifdef CONFIG_FSL_PME2_CTRL

/* Attributes for pme_reg_[set|get]() */
enum pme_attr {
	pme_attr_efqc_int,
	pme_attr_sw_db,
	pme_attr_dmcr,
	pme_attr_smcr,
	pme_attr_famcr,
	pme_attr_kvlts,
	pme_attr_max_chain_length,
	pme_attr_pattern_range_counter_idx,
	pme_attr_pattern_range_counter_mask,
	pme_attr_max_allowed_test_line_per_pattern,
	pme_attr_max_pdsr_index,
	pme_attr_max_pattern_matches_per_sui,
	pme_attr_max_pattern_evaluations_per_sui,
	pme_attr_report_length_limit,
	pme_attr_end_of_simple_sui_report,
	pme_attr_aim,
	pme_attr_sre_context_size,
	pme_attr_sre_rule_num,
	pme_attr_sre_session_ctx_num,
	pme_attr_end_of_sui_reaction_ptr,
	pme_attr_sre_pscl,
	pme_attr_sre_max_block_num,
	pme_attr_sre_max_instruction_limit,
	pme_attr_sre_max_index_size,
	pme_attr_sre_max_offset_ctrl,
	pme_attr_src_id,
	pme_attr_liodnr,
	pme_attr_rev1,
	pme_attr_rev2,
	pme_attr_srrv0,
	pme_attr_srrv1,
	pme_attr_srrv2,
	pme_attr_srrv3,
	pme_attr_srrv4,
	pme_attr_srrv5,
	pme_attr_srrv6,
	pme_attr_srrv7,
	pme_attr_srrfi,
	pme_attr_srri,
	pme_attr_srrwc,
	pme_attr_srrr,
	pme_attr_trunci,
	pme_attr_rbc,
	pme_attr_tbt0ecc1ec,
	pme_attr_tbt1ecc1ec,
	pme_attr_vlt0ecc1ec,
	pme_attr_vlt1ecc1ec,
	pme_attr_cmecc1ec,
	pme_attr_dxcmecc1ec,
	pme_attr_dxemecc1ec,
	pme_attr_stnib,
	pme_attr_stnis,
	pme_attr_stnth1,
	pme_attr_stnth2,
	pme_attr_stnthv,
	pme_attr_stnths,
	pme_attr_stnch,
	pme_attr_stnpm,
	pme_attr_stns1m,
	pme_attr_stnpmr,
	pme_attr_stndsr,
	pme_attr_stnesr,
	pme_attr_stns1r,
	pme_attr_stnob,
	pme_attr_mia_byc,
	pme_attr_mia_blc,
	pme_attr_isr,
	pme_attr_tbt0ecc1th,
	pme_attr_tbt1ecc1th,
	pme_attr_vlt0ecc1th,
	pme_attr_vlt1ecc1th,
	pme_attr_cmecc1th,
	pme_attr_dxcmecc1th,
	pme_attr_dxemecc1th,
	pme_attr_esr,
	pme_attr_ecr0,
	pme_attr_ecr1,
	pme_attr_pmstat,
	pme_attr_pmtr,
	pme_attr_pehd,
	pme_attr_ecc1bes,
	pme_attr_ecc2bes,
	pme_attr_eccaddr,
	pme_attr_ecccode,
	pme_attr_miace,
	pme_attr_miacr,
	pme_attr_cdcr,
	pme_attr_faconf,
	pme_attr_ier,
	pme_attr_isdr,
	pme_attr_iir,
	pme_attr_pdsrbah,
	pme_attr_pdsrbal,
	pme_attr_scbarh,
	pme_attr_scbarl,
	pme_attr_bsc_first, /* create 64-wide space for bsc */
	pme_attr_bsc_last = pme_attr_bsc_first + 63,
};

#define pme_attr_bsc(n) (pme_attr_bsc_first + (n))
/* Get/set driver attributes */
int pme_attr_set(enum pme_attr attr, u32 val);
int pme_attr_get(enum pme_attr attr, u32 *val);
int pme_stat_get(enum pme_attr stat, u64 *value, int reset);
#endif /* defined(CONFIG_FSL_PME2_CTRL) */

#ifdef CONFIG_COMPAT
#include <linux/compat.h>

struct compat_pme_buffer {
	compat_uptr_t data;
	compat_size_t size;
};

struct compat_pme_scan_cmd {
	__u32 flags; /* PME_SCAN_CMD_*** bitmask */
	compat_uptr_t opaque;
	struct compat_pme_buffer input;
	struct compat_pme_buffer output;
};
#define PMEIO_SCAN_W132	_IOW(PME_IOCTL_MAGIC, 0x0a, struct compat_pme_scan_cmd)

struct compat_pme_scan_cmds {
	compat_uint_t num;
	compat_uptr_t cmds;
};
#define PMEIO_SCAN_Wn32	_IOWR(PME_IOCTL_MAGIC, 0x0b, \
				struct compat_pme_scan_cmds)


struct compat_pme_scan_result {
	__u8 flags; /* PME_SCAN_RESULT_*** bitmask */
	enum pme_status status;
	struct compat_pme_buffer output;
	compat_uptr_t opaque;  /* value carried from the pme_scan_cmd */
};
#define PMEIO_SCAN_R132	_IOR(PME_IOCTL_MAGIC, 0x0c, \
				struct compat_pme_scan_result)


struct compat_pme_scan_results {
	compat_uint_t num;
	compat_uptr_t results;
};
#define PMEIO_SCAN_Rn32	_IOWR(PME_IOCTL_MAGIC, 0x0d, \
				struct compat_pme_scan_results)


struct compat_pme_scan {
	struct compat_pme_scan_cmd cmd;
	struct compat_pme_scan_result result;
};
#define PMEIO_SCAN32	_IOWR(PME_IOCTL_MAGIC, 0x0e, struct compat_pme_scan)

struct compat_pme_db {
	struct compat_pme_buffer input;
	struct compat_pme_buffer output;
	__u8 flags; /* PME_DB_RESULT_*** bitmask */
	enum pme_status status;
};
#define PMEIO_PMTCC32	_IOWR(PME_IOCTL_MAGIC, 0x03, struct compat_pme_db)

#endif /* CONFIG_COMPAT */

#endif /* __KERNEL__ */

#endif /* FSL_PME_H */
