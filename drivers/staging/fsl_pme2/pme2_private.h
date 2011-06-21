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

#include "pme2_sys.h"
#include <linux/fsl_pme.h>

#undef PME2_DEBUG

#ifdef PME2_DEBUG
#define PMEPRINFO(fmt, args...) pr_info("PME2: %s: " fmt, __func__, ## args)
#else
#define PMEPRINFO(fmt, args...)
#endif

#define PMEPRERR(fmt, args...) pr_err("PME2: %s: " fmt, __func__, ## args)
#define PMEPRCRIT(fmt, args...) pr_crit("PME2: %s: " fmt, __func__, ## args)

#ifdef CONFIG_FSL_PME2_CTRL
/* Hooks */
int pme2_create_sysfs_dev_files(struct platform_device *ofdev);
void pme2_remove_sysfs_dev_files(struct platform_device *ofdev);
void accumulator_update_interval(u32 interval);
#endif

static inline void set_fd_addr(struct qm_fd *fd, dma_addr_t addr)
{
	qm_fd_addr_set64(fd, addr);
}
static inline dma_addr_t get_fd_addr(const struct qm_fd *fd)
{
	return (dma_addr_t)qm_fd_addr_get64(fd);
}
static inline void set_sg_addr(struct qm_sg_entry *sg, dma_addr_t addr)
{
	qm_sg_entry_set64(sg, addr);
}
static inline dma_addr_t get_sg_addr(const struct qm_sg_entry *sg)
{
	return (dma_addr_t)qm_sg_entry_get64(sg);
}

/******************/
/* Datapath types */
/******************/

enum pme_mode {
	pme_mode_direct = 0x00,
	pme_mode_flow = 0x80
};

struct pme_context_a {
	enum pme_mode mode:8;
	u8 __reserved;
	/* Flow Context pointer (48-bit), ignored if mode==direct */
	u16 flow_hi;
	u32 flow_lo;
} __packed;
static inline u64 pme_context_a_get64(const struct pme_context_a *p)
{
	return ((u64)p->flow_hi << 32) | (u64)p->flow_lo;
}
/* Macro, so we compile better if 'v' isn't always 64-bit */
#define pme_context_a_set64(p, v) \
	do { \
		struct pme_context_a *__p931 = (p); \
		__p931->flow_hi = upper_32_bits(v); \
		__p931->flow_lo = lower_32_bits(v); \
	} while (0)

struct pme_context_b {
	u32 rbpid:8;
	u32 rfqid:24;
} __packed;


/* This is the 32-bit frame "cmd/status" field, sent to PME */
union pme_cmd {
	struct pme_cmd_nop {
		enum pme_cmd_type cmd:3;
	} nop;
	struct pme_cmd_flow_read {
		enum pme_cmd_type cmd:3;
	} fcr;
	struct pme_cmd_flow_write {
		enum pme_cmd_type cmd:3;
		u8 __reserved:5;
		u8 flags;	/* See PME_CMD_FCW_*** */
	} __packed fcw;
	struct pme_cmd_pmtcc {
		enum pme_cmd_type cmd:3;
	} pmtcc;
	struct pme_cmd_scan {
		union {
			struct {
				enum pme_cmd_type cmd:3;
				u8 flags:5; /* See PME_CMD_SCAN_*** */
			} __packed;
		};
		u8 set;
		u16 subset;
	} __packed scan;
};

/* The exported macro forms a "scan_args" u32 from 3 inputs, these private
 * inlines do the inverse, if you need to crack one apart. */
static inline u8 scan_args_get_flags(u32 args)
{
	return args >> 24;
}
static inline u8 scan_args_get_set(u32 args)
{
	return (args >> 16) & 0xff;
}
static inline u16 scan_args_get_subset(u32 args)
{
	return args & 0xffff;
}

/* Hook from pme2_high to pme2_low */
struct qman_fq *slabfq_alloc(void);
void slabfq_free(struct qman_fq *fq);

/* Hook from pme2_high to pme2_ctrl */
int pme2_have_control(void);
int pme2_exclusive_set(struct qman_fq *fq);
int pme2_exclusive_unset(void);

#define DECLARE_GLOBAL(name, t, mt, def, desc) \
        static t name = def; \
        module_param(name, mt, 0644); \
        MODULE_PARM_DESC(name, desc ", default: " __stringify(def));

/* Constants used by the SRE ioctl. */
#define PME_PMFA_SRE_POLL_MS		100
#define PME_PMFA_SRE_INDEX_MAX		(1 << 27)
#define PME_PMFA_SRE_INC_MAX		(1 << 12)
#define PME_PMFA_SRE_REP_MAX		(1 << 28)
#define PME_PMFA_SRE_INTERVAL_MAX	(1 << 12)

/* Encapsulations for mapping */
#define flow_map(flow) \
({ \
	struct pme_flow *__f913 = (flow); \
	pme_map(__f913); \
})

#define residue_map(residue) \
({ \
	struct pme_hw_residue *__f913 = (residue); \
	pme_map(__f913); \
})

