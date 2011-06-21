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

#include "pme2_test.h"

MODULE_AUTHOR("Geoff Thorpe");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 (p4080) high-level self-test");

/* Default Flow Context State */
static u8 fl_ctx_exp[]={
	0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
};

void scan_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_token *token)
{
	hexdump(fd, sizeof(*fd));
}

struct ctrl_op {
	struct pme_ctx_ctrl_token ctx_ctr;
	struct completion cb_done;
	enum pme_status cmd_status;
	u8 res_flag;
};

static void ctrl_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_ctrl_token *token)
{
	struct ctrl_op *ctrl = (struct ctrl_op *)token;
	pr_info("pme2_test_high: ctrl_cb() invoked, fd;!\n");
	ctrl->cmd_status = pme_fd_res_status(fd);
	ctrl->res_flag = pme_fd_res_flags(fd);
	hexdump(fd, sizeof(*fd));
	complete(&ctrl->cb_done);
}


#define POST_CTRL(val) \
do { \
	if (ret) \
		val = -1;\
	else if (pme_ctx_is_dead(&ctx))\
		val = -1;\
	else if (ctx_ctrl.cmd_status)\
		val = -1;\
	else if (ctx_ctrl.res_flag)\
		val = -1;\
} while (0)

void pme2_test_high(void)
{
	int post_ctrl = 0;
	struct pme_flow flow;
	struct qm_fqd_stashing stashing;
	struct pme_ctx ctx = {
		.cb = scan_cb
	};
	int ret;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.cmd_status = 0,
		.res_flag = 0
	};
	struct cpumask backup_mask = current->cpus_allowed;
	struct cpumask new_mask = *qman_affine_cpus();

	pr_info("PME2: high-level test starting\n");

	cpumask_and(&new_mask, &new_mask, bman_affine_cpus());
	ret = set_cpus_allowed_ptr(current, &new_mask);
	if (ret) {
		post_ctrl = -1;
		pr_info("PME2: test high: can't set cpumask\n");
		goto done;
	}

	pme_sw_flow_init(&flow);
	init_completion(&ctx_ctrl.cb_done);
	ret = pme_ctx_init(&ctx, PME_CTX_FLAG_LOCAL, 0, 4, 4, 0, NULL);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto restore_mask;

	/* enable the context */
	pme_ctx_enable(&ctx);
	pr_info("PME2: pme_ctx_enable done\n");
	ret = pme_ctx_ctrl_update_flow(&ctx, PME_CTX_OP_WAIT | PME_CMD_FCW_ALL,
					&flow, &ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_update_flow done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto disable_ctx;
	/* read back flow settings */
	ret = pme_ctx_ctrl_read_flow(&ctx, PME_CTX_OP_WAIT, &flow,
			&ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_read_flow done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto disable_ctx;
	if (memcmp(&flow, fl_ctx_exp, sizeof(flow))) {
		pr_info("Default Flow Context Read FAIL\n");
		pr_info("Expected:\n");
		hexdump(fl_ctx_exp, sizeof(fl_ctx_exp));
		pr_info("Received:\n");
		hexdump(&flow, sizeof(flow));
		post_ctrl = -1;
		goto disable_ctx;
	} else
		pr_info("Default Flow Context Read OK\n");
	/* start a NOP */
	ret = pme_ctx_ctrl_nop(&ctx, 0, &ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_nop done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto disable_ctx;
	/* start an update to add residue to the context */
	flow.ren = 1;
	ret = pme_ctx_ctrl_update_flow(&ctx, PME_CTX_OP_WAIT | PME_CMD_FCW_RES,
					&flow, &ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_update_flow done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto disable_ctx;
	/* start a blocking disable */
	ret = pme_ctx_disable(&ctx, PME_CTX_OP_WAIT, &ctx_ctrl.ctx_ctr);
	if (ret < 1) {
		post_ctrl = -1;
		goto finish_ctx;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	/* do some reconfiguration */
	ret = pme_ctx_reconfigure_tx(&ctx, 63, 7);
	if (ret) {
		post_ctrl = -1;
		goto finish_ctx;
	}
	stashing.exclusive = 0;
	stashing.annotation_cl = 0;
	stashing.data_cl = 2;
	stashing.context_cl = 2;
	ret = pme_ctx_reconfigure_rx(&ctx, 7, 0, &stashing);
	if (ret) {
		post_ctrl = -1;
		goto finish_ctx;
	}
	/* reenable */
	ret = pme_ctx_enable(&ctx);
	if (ret) {
		post_ctrl = -1;
		goto finish_ctx;
	}
	/* read back flow settings */
	ret = pme_ctx_ctrl_read_flow(&ctx,
		PME_CTX_OP_WAIT | PME_CTX_OP_WAIT_INT | PME_CMD_FCW_RES, &flow,
		&ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_read_flow done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	if (post_ctrl)
		goto disable_ctx;
	/* blocking NOP */
	ret = pme_ctx_ctrl_nop(&ctx, PME_CTX_OP_WAIT | PME_CTX_OP_WAIT_INT,
			&ctx_ctrl.ctx_ctr);
	pr_info("PME2: pme_ctx_ctrl_nop done\n");
	wait_for_completion(&ctx_ctrl.cb_done);
	POST_CTRL(post_ctrl);
	/* Disable, and done */
disable_ctx:
	ret = pme_ctx_disable(&ctx, PME_CTX_OP_WAIT, &ctx_ctrl.ctx_ctr);
	BUG_ON(ret < 1);
	wait_for_completion(&ctx_ctrl.cb_done);
finish_ctx:
	pme_ctx_finish(&ctx);
restore_mask:
	ret = set_cpus_allowed_ptr(current, &backup_mask);
	if (ret) {
		pr_err("PME2 test high: can't restore cpumask");
		post_ctrl = -1;
	}
done:
	if (post_ctrl)
		pr_info("PME2: high-level test failed\n");
	else
		pr_info("PME2: high-level test passed\n");
}

static int pme2_test_high_init(void)
{
	int big_loop = 2;
	while (big_loop--)
		pme2_test_high();
	return 0;
}

static void pme2_test_high_exit(void)
{
}

module_init(pme2_test_high_init);
module_exit(pme2_test_high_exit);

