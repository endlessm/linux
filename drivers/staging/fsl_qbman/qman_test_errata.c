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

#include "qman_test.h"

/* Waiting on a model fix from virtutech */
#if 0
/*********************/
/* generic utilities */
/*********************/

static int do_enqueues(struct qman_fq *fq, const struct qm_fd *fds, int num)
{
	int ret = 0;
	u32 flags = QMAN_ENQUEUE_FLAG_WAIT;
	while (num-- && !ret) {
		if (!num)
			flags |= QMAN_ENQUEUE_FLAG_WAIT_SYNC;
		pr_info("about to enqueue\n");
		ret = qman_enqueue(fq, fds++, flags);
	}
	return ret;
}

/***************************/
/* "tdthresh" test (QMAN6) */
/***************************/

/* First thresh == 201 * (2^21) == 421527552 (0x19200000) */
#define THRESH_MANT	201
#define THRESH_EXP	21

/* first three equal thresh, fourth takes us over */
static const struct qm_fd td_eq[] = {
	QM_FD_FMT_20(0, 0x34, 0x87654321, QM_FD_SG, 0, 79321),
	QM_FD_FMT_29(0, 0x34, 0x87654321, QM_FD_COMPOUND, 29923679),
	QM_FD_FMT_29(0, 0x0d, 0xacadabba, QM_FD_CONTIG_BIG, 391524552),
	QM_FD_FMT_20(0, 0x0b, 0x0fa10ada, QM_FD_CONTIG, 0, 1),
	QM_FD_FMT_20(0, 0x0b, 0x0fa10ada, QM_FD_CONTIG, 0, 1),
};

struct tdthresh_fq {
	struct qman_fq fq;
	int got_ern;
	int num_dqrr;
};

static enum qman_cb_dqrr_result cb_dqrr_tdthresh(struct qman_portal *p,
					struct qman_fq *__fq,
					const struct qm_dqrr_entry *dqrr)
{
	struct tdthresh_fq *t = (void *)__fq;
	t->num_dqrr++;
	return qman_cb_dqrr_consume;
}

static void cb_ern_tdthresh(struct qman_portal *p, struct qman_fq *__fq,
					const struct qm_mr_entry *mr)
{
	struct tdthresh_fq *t = (void *)__fq;
	t->got_ern = 1;
}

static void test_tdthresh(void)
{
	struct tdthresh_fq tdfq = {
		.fq = {
			.cb = {
				.dqrr = cb_dqrr_tdthresh,
				.ern = cb_ern_tdthresh
			}
		},
		.got_ern = 0,
		.num_dqrr = 0
	};
	struct qman_fq *fq = &tdfq.fq;
	struct qm_mcc_initfq opts = {
		.we_mask = QM_INITFQ_WE_FQCTRL | QM_INITFQ_WE_TDTHRESH,
		.fqd = {
			.fq_ctrl = QM_FQCTRL_TDE,
			.td = {
				.exp = THRESH_EXP,
				.mant = THRESH_MANT,
			}
		}
	};
	struct qm_fqd fqd;
	u32 flags;
	int ret = qman_create_fq(0, QMAN_FQ_FLAG_DYNAMIC_FQID, fq);
	BUG_ON(ret);
	/* leave it parked, and set it for local dequeue (loopback) */
	ret = qman_init_fq(fq, QMAN_INITFQ_FLAG_LOCAL, &opts);
	BUG_ON(ret);
	/* query it back and confirm everything is ok */
	ret = qman_query_fq(fq, &fqd);
	BUG_ON(ret);
	if (fqd.fq_ctrl != opts.fqd.fq_ctrl) {
		pr_err("queried fq_ctrl=%x, should be=%x\n", fqd.fq_ctrl,
			opts.fqd.fq_ctrl);
		panic("fail");
	}
	if (memcmp(&fqd.td, &opts.fqd.td, sizeof(fqd.td))) {
		pr_err("queried td_thresh=%x:%x, should be=%x:%x\n",
			fqd.td.exp, fqd.td.mant,
			opts.fqd.td.exp, opts.fqd.td.mant);
		panic("fail");
	}
	ret = do_enqueues(fq, td_eq, 3);
	BUG_ON(ret);
	pr_info("  tdthresh: eq[0..2] complete\n");
	/* enqueues are flushed, so if Qman is going to throw an ERN, the irq
	 * assertion will already be on its way. */
	msleep(500);
	BUG_ON(tdfq.got_ern);
	pr_info("  tdthresh: eq <= thresh OK\n");
	ret = do_enqueues(fq, td_eq + 3, 1);
	BUG_ON(ret);
	pr_info("  tdthresh: eq[3] complete\n");
	/* enqueues are flushed, so if Qman is going to throw an ERN, the irq
	 * assertion will already be on its way. */
	msleep(500);
	BUG_ON(tdfq.got_ern);
	pr_info("  tdthresh: eq <= thresh OK\n");
	ret = do_enqueues(fq, td_eq + 4, 1);
	BUG_ON(ret);
	pr_info("  tdthresh: eq[4] complete\n");
	/* enqueues are flushed, so if Qman is going to throw an ERN, the irq
	 * assertion will already be on its way. */
	msleep(500);
	BUG_ON(!tdfq.got_ern);
	pr_info("  tdthresh: eq > thresh OK\n");
	ret = qman_volatile_dequeue(fq,
		QMAN_VOLATILE_FLAG_WAIT | QMAN_VOLATILE_FLAG_FINISH,
		QM_VDQCR_NUMFRAMES_TILLEMPTY);
	BUG_ON(ret);
	BUG_ON(tdfq.num_dqrr != 4);
	ret = qman_retire_fq(fq, &flags);
	BUG_ON(ret);
	BUG_ON(flags);
	ret = qman_oos_fq(fq);
	BUG_ON(ret);
}

/****************************/
/* "ern code6" test (QMAN9) */
/****************************/

/* Dummy FD to enqueue out-of-sequence and generate an ERN */
static const struct qm_fd c6_eq =
	QM_FD_FMT_29(0, 0xba, 0xdeadbeef, QM_FD_CONTIG_BIG, 1234);

struct code6_fq {
	struct qman_fq fq;
	struct qm_mr_entry mr;
	struct completion got_ern;
};

static void cb_ern_code6(struct qman_portal *p, struct qman_fq *__fq,
					const struct qm_mr_entry *mr)
{
	struct code6_fq *c = (void *)__fq;
	memcpy(&c->mr, mr, sizeof(*mr));
	complete(&c->got_ern);
}

static void test_ern_code6(void)
{
	struct code6_fq c6fq = {
		.fq = {
			.cb = {
				.ern = cb_ern_code6
			}
		},
		.got_ern = COMPLETION_INITIALIZER(c6fq.got_ern)
	};
	struct qman_fq *fq = &c6fq.fq;
	struct qm_mcc_initfq opts = {
		.we_mask = QM_INITFQ_WE_FQCTRL,
		.fqd = {
			.fq_ctrl = QM_FQCTRL_ORP
		}
	};
	u32 flags;
	int ret = qman_create_fq(0, QMAN_FQ_FLAG_DYNAMIC_FQID, fq);
	BUG_ON(ret);
	/* leave it parked, and set it for local dequeue (loopback) */
	ret = qman_init_fq(fq, QMAN_INITFQ_FLAG_LOCAL, &opts);
	BUG_ON(ret);
	/* enqueue with ORP using a "too early" sequence number */
	ret = qman_enqueue_orp(fq, &c6_eq,
		QMAN_ENQUEUE_FLAG_WAIT | QMAN_ENQUEUE_FLAG_WAIT_SYNC, fq, 5);
	BUG_ON(ret);
	pr_info("  code6: eq complete\n");
	ret = qman_retire_fq(fq, &flags);
	BUG_ON(ret);
	pr_info("  code6: retire complete, flags=%08x\n", flags);
	BUG_ON(flags != QMAN_FQ_STATE_ORL);
	wait_for_completion(&c6fq.got_ern);
	pr_info("  code6: ERN, VERB=0x%02x, RC==0x%02x\n",
		c6fq.mr.verb, c6fq.mr.ern.rc);
	BUG_ON(c6fq.mr.verb & 0x20);
	BUG_ON((c6fq.mr.ern.rc & QM_MR_RC_MASK) != QM_MR_RC_ORPWINDOW_RETIRED);
	ret = qman_oos_fq(fq);
	BUG_ON(ret);
}

void qman_test_errata(void)
{
	pr_info("Testing Qman errata handling ...\n");
	test_tdthresh();
	test_ern_code6();
	pr_info("                              ... SUCCESS!\n");
}
#else
void qman_test_errata(void)
{
	pr_info("Qman errata-handling test disabled, waiting on model fix\n");
}
#endif

