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

#include "pme2_test.h"

enum scan_ctrl_mode {
	no_scan = 0,
	do_scan = 1,
};

enum db_ctrl_mode {
	create_destroy = 0,
	create = 1,
	destroy = 2,
	nothing = 3
};

MODULE_AUTHOR("Jeffrey Ladouceur");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("PME scan testing");

static enum db_ctrl_mode db_ctrl;
module_param(db_ctrl, uint, 0644);
MODULE_PARM_DESC(db_ctrl, "PME Database control");

static enum scan_ctrl_mode scan_ctrl = 1;
module_param(scan_ctrl, uint, 0644);
MODULE_PARM_DESC(scan_ctrl, "Scan control");

static u8 scan_result_direct_mode_inc_mode[] = {
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
};

static u8 fl_ctx_exp[] = {
	0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
};

/* same again with 'sos' bit cleared */
static u8 fl_ctx_exp_post_scan[] = {
	0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe0,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
};

struct scan_ctx {
	struct pme_ctx base_ctx;
	struct qm_fd result_fd;
};

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
	ctrl->cmd_status = pme_fd_res_status(fd);
	ctrl->res_flag = pme_fd_res_flags(fd) & PME_STATUS_UNRELIABLE;
	/* hexdump(fd, sizeof(*fd)); */
	complete(&ctrl->cb_done);
}

static DECLARE_COMPLETION(scan_comp);

static void scan_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_token *ctx_token)
{
	struct scan_ctx *my_ctx = (struct scan_ctx *)ctx;
	memcpy(&my_ctx->result_fd, fd, sizeof(*fd));
	complete(&scan_comp);
}

#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID

static struct bman_pool *pool;
static u32 pme_bpid;
static void *bman_buffers_virt_base;
static dma_addr_t bman_buffers_phys_base;

static void release_buffer(dma_addr_t addr)
{
	struct bm_buffer bufs_in;
	bm_buffer_set64(&bufs_in, addr);
	if (bman_release(pool, &bufs_in, 1, BMAN_RELEASE_FLAG_WAIT))
		panic("bman_release() failed\n");
}

static void empty_buffer(void)
{
	struct bm_buffer bufs_in;
	int ret;

	do {
		ret = bman_acquire(pool, &bufs_in, 1, 0);
	} while (!ret);
}
#endif /*CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID*/

static int scan_test_direct(int trunc, int use_bp)
{
	struct scan_ctx a_scan_ctx = {
		.base_ctx = {
			.cb = scan_cb
		}
	};
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.cmd_status = 0,
		.res_flag = 0
	};
	struct qm_fd fd;
	struct qm_sg_entry sg_table[2];
	int ret;
	enum pme_status status;
	struct pme_ctx_token token;
	u8 *scan_result;
	u32 scan_result_size;
	u8 scan_data[] = {
		0x41, 0x42, 0x43, 0x44, 0x45
	};
	u8 result_data[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};

	init_completion(&ctx_ctrl.cb_done);
	scan_result = scan_result_direct_mode_inc_mode;
	scan_result_size = sizeof(scan_result_direct_mode_inc_mode);

	ret = pme_ctx_init(&a_scan_ctx.base_ctx,
		PME_CTX_FLAG_DIRECT | PME_CTX_FLAG_LOCAL,
		0, 4, 4, 0, NULL);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		return ret;
	}
	/* enable the context */
	ret = pme_ctx_enable(&a_scan_ctx.base_ctx);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_finish;
	}

	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	if (trunc) {
		fd.length20 = sizeof(scan_data);
		qm_fd_addr_set64(&fd, pme_map(scan_data));
	} else {
		/* build the result */
		qm_sg_entry_set64(&sg_table[0], pme_map(result_data));
		sg_table[0].length = sizeof(result_data);
		qm_sg_entry_set64(&sg_table[1], pme_map(scan_data));
		sg_table[1].length = sizeof(scan_data);
		sg_table[1].final = 1;
		fd._format2 = qm_fd_compound;
		qm_fd_addr_set64(&fd, pme_map(sg_table));
	}

	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_disable;
	}
	wait_for_completion(&scan_comp);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto ctx_disable;
	}
	if (trunc) {
		int res_flag = pme_fd_res_flags(&a_scan_ctx.result_fd);
		/* Check the response...expect truncation bit to be set */
		if (!(res_flag & PME_STATUS_TRUNCATED)) {
			pr_err("pme scan test failed, expected truncation\n");
			goto ctx_disable;
		}
	} else {
		if (memcmp(scan_result, result_data, scan_result_size) != 0) {
			pr_err("pme scan test result not expected\n");
			hexdump(scan_result, scan_result_size);
			pr_err("Received...\n");
			hexdump(result_data, sizeof(result_data));
			goto ctx_disable;
		}
	}

	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT, NULL);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_finish;
	}
	if (!use_bp) {
		pme_ctx_finish(&a_scan_ctx.base_ctx);
		return 0;
	}
	/* use buffer pool */
	/* Check with bman */
	/* reconfigure */

#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	ret = pme_ctx_reconfigure_tx(&a_scan_ctx.base_ctx, pme_bpid, 5);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_finish;
	}
	ret = pme_ctx_enable(&a_scan_ctx.base_ctx);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_finish;
	}
	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	/* build the result */
	/* result is all zero...use bman */
	qm_sg_entry_set64(&sg_table[1], pme_map(scan_data));
	sg_table[1].length = sizeof(scan_data);
	sg_table[1].final = 1;

	fd._format2 = qm_fd_compound;
	qm_fd_addr_set64(&fd, pme_map(sg_table));

	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_disable;
	}
	wait_for_completion(&scan_comp);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto ctx_disable;
	}
	/* sg result should point to bman buffer */
	if (!qm_sg_entry_get64(&sg_table[0])) {
		pr_err("pme scan test failed, sg result not bman buffer\n");
		goto ctx_disable;
	}
	if (memcmp(scan_result, bman_buffers_virt_base, scan_result_size)
			!= 0) {
		pr_err("pme scan test not expected, Expected\n");
		hexdump(scan_result, scan_result_size);
		pr_err("Received...\n");
		hexdump(bman_buffers_virt_base, scan_result_size);
		release_buffer(qm_sg_entry_get64(&sg_table[0]));
		goto ctx_disable;
	}
	release_buffer(qm_sg_entry_get64(&sg_table[0]));
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT, NULL);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto ctx_finish;
	}
	pme_ctx_finish(&a_scan_ctx.base_ctx);
	return 0;
#endif

/* failure path */
ctx_disable:
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT, NULL);
ctx_finish:
	pme_ctx_finish(&a_scan_ctx.base_ctx);
	return (!ret) ? -EINVAL : ret;
}

static int scan_test_flow(void)
{
	struct pme_flow flow;
	struct pme_flow rb_flow;
	struct scan_ctx a_scan_ctx = {
		.base_ctx = {
			.cb = scan_cb
		}
	};
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.cmd_status = 0,
		.res_flag = 0
	};
	struct qm_fd fd;
	struct qm_sg_entry sg_table[2];
	int ret;
	enum pme_status status;
	struct pme_ctx_token token;
	u8 *scan_result;
	u32 scan_result_size;
	u8 scan_data[] = {
		0x41, 0x42, 0x43, 0x44, 0x45
	};
	u8 result_data[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00
	};

	pme_sw_flow_init(&flow);
	init_completion(&ctx_ctrl.cb_done);
	scan_result = scan_result_direct_mode_inc_mode;
	scan_result_size = sizeof(scan_result_direct_mode_inc_mode);

	ret = pme_ctx_init(&a_scan_ctx.base_ctx,
		PME_CTX_FLAG_LOCAL, 0, 4, 4, 0, NULL);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		return ret;
	}
	/* enable the context */
	ret = pme_ctx_enable(&a_scan_ctx.base_ctx);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_finish;
	}
	ret = pme_ctx_ctrl_update_flow(&a_scan_ctx.base_ctx,
		PME_CTX_OP_WAIT | PME_CMD_FCW_ALL, &flow, &ctx_ctrl.ctx_ctr);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	/* read back flow settings */
	ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT, &rb_flow, &ctx_ctrl.ctx_ctr);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	if (memcmp(&rb_flow, fl_ctx_exp, sizeof(rb_flow)) != 0) {
		pr_err("pme scan test Flow Context Read FAIL\n");
		pr_err("Expected\n");
		hexdump(fl_ctx_exp, sizeof(fl_ctx_exp));
		pr_err("Received...\n");
		hexdump(&rb_flow, sizeof(rb_flow));
		goto flow_ctx_disable;
	}

	/* Do a pre-built output, scan with match test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));
	memset(&sg_table, 0, sizeof(sg_table));

	/* build the result */
	qm_sg_entry_set64(&sg_table[0], pme_map(result_data));
	sg_table[0].length = sizeof(result_data);
	qm_sg_entry_set64(&sg_table[1], pme_map(scan_data));
	sg_table[1].length = sizeof(scan_data);
	sg_table[1].final = 1;

	fd._format2 = qm_fd_compound;
	qm_fd_addr_set64(&fd, pme_map(sg_table));

	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	wait_for_completion(&scan_comp);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto flow_ctx_disable;
	}

	if (memcmp(scan_result, result_data, scan_result_size) != 0) {
		pr_err("pme scan test result not expected\n");
		hexdump(scan_result, scan_result_size);
		pr_err("Received...\n");
		hexdump(result_data, sizeof(result_data));
		goto flow_ctx_disable;
	}

	/* read back flow settings */
	ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT, &rb_flow, &ctx_ctrl.ctx_ctr);
	if (ret) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto flow_ctx_disable;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	if (memcmp(&rb_flow, fl_ctx_exp_post_scan, sizeof(rb_flow)) != 0) {
		pr_err("pme scan test Flow Context Read FAIL\n");
		pr_err("Expected\n");
		hexdump(fl_ctx_exp_post_scan, sizeof(fl_ctx_exp_post_scan));
		pr_err("Received\n");
		hexdump(&rb_flow, sizeof(rb_flow));
		goto flow_ctx_disable;
	}

	/* Test truncation test */
	/* Build a frame descriptor */
	memset(&fd, 0, sizeof(struct qm_fd));

	fd.length20 = sizeof(scan_data);
	qm_fd_addr_set64(&fd, pme_map(scan_data));

	ret = pme_ctx_scan(&a_scan_ctx.base_ctx, 0, &fd,
		PME_SCAN_ARGS(PME_CMD_SCAN_SR | PME_CMD_SCAN_E, 0, 0xff00),
		&token);
	if (ret) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto flow_ctx_disable;
	}
	wait_for_completion(&scan_comp);

	status = pme_fd_res_status(&a_scan_ctx.result_fd);
	if (status) {
		pr_err("pme scan test failed 0x%x\n", status);
		goto flow_ctx_disable;
	}
	/* Check the response...expect truncation bit to be set */
	if (!(pme_fd_res_flags(&a_scan_ctx.result_fd) & PME_STATUS_TRUNCATED)) {
		pr_err("st: Scan result failed...expected trunc\n");
		goto flow_ctx_disable;
	}

	/* read back flow settings */
	ret = pme_ctx_ctrl_read_flow(&a_scan_ctx.base_ctx,
			PME_CTX_OP_WAIT, &rb_flow, &ctx_ctrl.ctx_ctr);
	if (ret) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		pr_err("pme scan test failed: 0x%x\n", ret);
		goto flow_ctx_disable;
	}
	if (memcmp(&rb_flow, fl_ctx_exp_post_scan, sizeof(rb_flow)) != 0) {
		pr_err("pme scan test Flow Context Read FAIL\n");
		pr_err("Expected\n");
		hexdump(fl_ctx_exp_post_scan, sizeof(fl_ctx_exp_post_scan));
		pr_err("Received\n");
		hexdump(&rb_flow, sizeof(rb_flow));
		goto flow_ctx_disable;
	}

	/* Disable */
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT,
				&ctx_ctrl.ctx_ctr);
	if (ret < 1) {
		pr_err("pme scan test failed 0x%x\n", ret);
		goto flow_ctx_finish;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	pme_ctx_finish(&a_scan_ctx.base_ctx);
	return 0;
	/* error path */
/* failure path */
flow_ctx_disable:
	ret = pme_ctx_disable(&a_scan_ctx.base_ctx, PME_CTX_OP_WAIT, NULL);
flow_ctx_finish:
	pme_ctx_finish(&a_scan_ctx.base_ctx);
	return (!ret) ? -EINVAL : ret;
}

void pme2_test_scan(void)
{
	int ret;

	ret = scan_test_direct(0, 0);
	if (ret)
		goto done;
	ret = scan_test_direct(1, 0);
	if (ret)
		goto done;
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	ret = scan_test_direct(0, 1);
	if (ret)
		goto done;
#endif
	ret = scan_test_flow();
done:
	if (ret)
		pr_info("pme scan test FAILED 0x%x\n", ret);
	else
		pr_info("pme Scan Test Passed\n");
}

static int setup_buffer_pool(void)
{
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	u32 bpid_size = CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID_SIZE;
	struct bman_pool_params pparams = {
		.flags = BMAN_POOL_FLAG_DYNAMIC_BPID,
		.thresholds = {
			0,
			0,
			0,
			0
		}
	};

	if (!pme2_have_control()) {
		pr_err("pme scan test: Not the ctrl-plane\n");
		return -EINVAL;
	}
	pool = bman_new_pool(&pparams);
	if (!pool) {
		pr_err("pme scan test: can't get buffer pool\n");
		return -EINVAL;
	}
	pme_bpid = bman_get_params(pool)->bpid;
	bman_buffers_virt_base = kmalloc(1<<(bpid_size+5), GFP_KERNEL);
	bman_buffers_phys_base = pme_map(bman_buffers_virt_base);
	if (pme_map_error(bman_buffers_phys_base)) {
		pr_info("pme scan test: pme_map_error\n");
		bman_free_pool(pool);
		kfree(bman_buffers_virt_base);
		return -ENODEV;
	}
	release_buffer(bman_buffers_phys_base);
	/* Configure the buffer pool */
	pme_attr_set(pme_attr_bsc(pme_bpid), bpid_size);
	/* realease to the specified buffer pool */
	return 0;
#endif
	return 0;
}

static int teardown_buffer_pool(void)
{
#ifdef CONFIG_FSL_PME2_TEST_SCAN_WITH_BPID
	pme_attr_set(pme_attr_bsc(pme_bpid), 0);
	empty_buffer();
	bman_free_pool(pool);
	kfree(bman_buffers_virt_base);
#endif
	return 0;
}

static int pme2_test_scan_init(void)
{
	int big_loop = 2;
	int ret = 0;
	struct cpumask backup_mask = current->cpus_allowed;
	struct cpumask new_mask = *qman_affine_cpus();

	cpumask_and(&new_mask, &new_mask, bman_affine_cpus());
	ret = set_cpus_allowed_ptr(current, &new_mask);
	if (ret) {
		pr_info("pme scan test: can't set cpumask\n");
		goto done_all;
	}

	ret = setup_buffer_pool();
	if (ret)
		goto done_cpu_mask;

	/* create sample database */
	if (db_ctrl == create_destroy || db_ctrl == create) {
		if (!pme2_have_control()) {
			pr_err("pme scan test: Not the ctrl-plane\n");
			ret = -EINVAL;
			goto done_scan;
		}
		if (pme2_sample_db()) {
			pr_err("pme scan test: error creating db\n");
			goto done_scan;
		}
	}

	if (scan_ctrl == do_scan) {
		while (big_loop--)
			pme2_test_scan();
	}

	if (db_ctrl == create_destroy || db_ctrl == destroy) {
		/* Clear database */
		if (pme2_clear_sample_db())
			pr_err("pme scan test: error clearing db\n");
	}

done_scan:
	teardown_buffer_pool();
done_cpu_mask:
	ret = set_cpus_allowed_ptr(current, &backup_mask);
	if (ret)
		pr_err("PME2 test high: can't restore cpumask");
done_all:
	return ret;
}

static void pme2_test_scan_exit(void)
{
}

module_init(pme2_test_scan_init);
module_exit(pme2_test_scan_exit);
