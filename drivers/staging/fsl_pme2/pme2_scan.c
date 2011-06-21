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

#include "pme2_private.h"
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/compat.h>

#define WAIT_AND_INTERRUPTABLE	(PME_CTX_OP_WAIT|PME_CTX_OP_WAIT_INT)
#define INPUT_FRM	1
#define OUTPUT_FRM	0
/* Private structure that is allocated for each open that is done on the
 * pme_scan device. */
struct scan_session {
	/* The ctx that is needed to communicate with the pme high level */
	struct pme_ctx ctx;
	/* Locks completed_commands */
	spinlock_t set_subset_lock;
	__u8 set;
	__u16 subset;
	/* For asynchronous processing */
	wait_queue_head_t waiting_for_completion;
	struct list_head completed_commands;
	/* Locks completed_commands */
	spinlock_t completed_commands_lock;
	u32 completed_count;
};

/* Command Token for scan operations. One of these is created for every
 * operation on a context. When the context operation is complete cleanup
 * is done */
struct cmd_token {
	/* pme high level token */
	struct pme_ctx_token hl_token;
	/* The kernels copy of the user op structure */
	struct pme_scan_cmd kernel_op;
	/* Set to non zero if this is a synchronous request */
	u8 synchronous;
	/* data */
	struct qm_fd tx_fd;
	struct qm_sg_entry tx_comp[2];
	struct qm_fd rx_fd;
	void *tx_data;
	size_t tx_size;
	void *rx_data;
	size_t rx_size;
	/* For blocking requests, we need a wait point and condition */
	wait_queue_head_t *queue;
	/* List management for completed async requests */
	struct list_head completed_list;
	u8 done;
	u8 ern;
};

struct ctrl_op {
	struct pme_ctx_ctrl_token ctx_ctr;
	struct completion cb_done;
	enum pme_status cmd_status;
	u8 res_flag;
	u8 ern;
};

#ifdef CONFIG_COMPAT
static void compat_to_scan_cmd(struct pme_scan_cmd *dst,
			struct compat_pme_scan_cmd *src)
{
	dst->flags = src->flags;
	dst->opaque = compat_ptr(src->opaque);
	dst->input.data = compat_ptr(src->input.data);
	dst->input.size = src->input.size;
	dst->output.data = compat_ptr(src->output.data);
	dst->output.size = src->output.size;
}

static void scan_result_to_compat(struct compat_pme_scan_result *dst,
			struct pme_scan_result *src)
{
	dst->flags = src->flags;
	dst->opaque = ptr_to_compat(src->opaque);
	dst->status = src->status;
	dst->output.data = ptr_to_compat(src->output.data);
	dst->output.size = src->output.size;
}

static void compat_to_scan_result(struct pme_scan_result *dst,
			struct compat_pme_scan_result *src)
{
	dst->flags = src->flags;
	dst->opaque = compat_ptr(src->opaque);
	dst->status = src->status;
	dst->output.data = compat_ptr(src->output.data);
	dst->output.size = src->output.size;
}
#endif

static void ctrl_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
		struct pme_ctx_ctrl_token *token)
{
	struct ctrl_op *ctrl = (struct ctrl_op *)token;
	ctrl->cmd_status = pme_fd_res_status(fd);
	ctrl->res_flag = pme_fd_res_flags(fd) & PME_STATUS_UNRELIABLE;
	complete(&ctrl->cb_done);
}

static void ctrl_ern_cb(struct pme_ctx *ctx, const struct qm_mr_entry *mr,
		struct pme_ctx_ctrl_token *token)
{
	struct ctrl_op *ctrl = (struct ctrl_op *)token;
	ctrl->ern = 1;
	complete(&ctrl->cb_done);
}

static inline int scan_data_empty(struct scan_session *session)
{
	return list_empty(&session->completed_commands);
}

/* Cleanup for the execute_cmd method */
static inline void cleanup_token(struct cmd_token *token_p)
{
	kfree(token_p->tx_data);
	kfree(token_p->rx_data);
	return;
}

/* Callback for scan operations */
static void scan_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
				struct pme_ctx_token *ctx_token)
{
	struct cmd_token *token = (struct cmd_token *)ctx_token;
	struct scan_session *session = (struct scan_session *)ctx;

	token->rx_fd = *fd;
	/* If this is a asynchronous command, queue the token */
	if (!token->synchronous) {
		spin_lock(&session->completed_commands_lock);
		list_add_tail(&token->completed_list,
			      &session->completed_commands);
		session->completed_count++;
		spin_unlock(&session->completed_commands_lock);
	}
	/* Wake up the thread that's waiting for us */
	token->done = 1;
	wake_up(token->queue);
	return;
}

static void scan_ern_cb(struct pme_ctx *ctx, const struct qm_mr_entry *mr,
		struct pme_ctx_token *ctx_token)
{
	struct cmd_token *token = (struct cmd_token *)ctx_token;
	struct scan_session *session = (struct scan_session *)ctx;

	token->ern = 1;
	token->rx_fd = mr->ern.fd;
	/* If this is a asynchronous command, queue the token */
	if (!token->synchronous) {
		spin_lock(&session->completed_commands_lock);
		list_add_tail(&token->completed_list,
			      &session->completed_commands);
		session->completed_count++;
		spin_unlock(&session->completed_commands_lock);
	}
	/* Wake up the thread that's waiting for us */
	token->done = 1;
	wake_up(token->queue);
	return;
}

static int process_completed_token(struct file *fp, struct cmd_token *token_p,
					struct pme_scan_result *scan_result)
{
	int ret = 0;
	u32 src_sz, dst_sz;

	memset(scan_result, 0, sizeof(struct pme_scan_result));
	if (token_p->ern) {
		ret = -EIO;
		goto done;
	}
	scan_result->output.data = token_p->kernel_op.output.data;

	if (token_p->rx_fd.format == qm_fd_compound) {
		/* Need to copy  output */
		src_sz = token_p->tx_comp[OUTPUT_FRM].length;
		dst_sz = token_p->kernel_op.output.size;
		scan_result->output.size = min(dst_sz, src_sz);
		/* Doesn't make sense we generated more than available space
		 * should have got truncation.
		 */
		BUG_ON(dst_sz < src_sz);
		if (copy_to_user(scan_result->output.data, token_p->rx_data,
				scan_result->output.size)) {
			pr_err("Error copying to user data\n");
			cleanup_token(token_p);
			return -EFAULT;
		}
	} else if (token_p->rx_fd.format == qm_fd_sg_big)
		scan_result->output.size = 0;
	else
		pr_err("pme2_scan: unexpected frame type received\n");

	scan_result->flags |= pme_fd_res_flags(&token_p->rx_fd);
	scan_result->status |= pme_fd_res_status(&token_p->rx_fd);
done:
	scan_result->opaque = token_p->kernel_op.opaque;
	cleanup_token(token_p);
	return ret;
}

static int getscan_cmd(struct file *fp, struct scan_session *session,
	struct pme_scan_params __user *user_scan_params)
{
	int ret = 0;
	struct pme_flow params;
	struct pme_scan_params local_scan_params;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};
	init_completion(&ctx_ctrl.cb_done);

	memset(&local_scan_params, 0, sizeof(local_scan_params));

	/* must be enabled */
	if (pme_ctx_is_disabled(&session->ctx)) {
		pr_err("pme2_scan: ctx is disabled\n");
		ret = -EINVAL;
		goto done;
	}
	ret = pme_ctx_ctrl_read_flow(&session->ctx, WAIT_AND_INTERRUPTABLE,
			&params, &ctx_ctrl.ctx_ctr);
	if (ret) {
		PMEPRINFO("read flow error %d\n", ret);
		goto done;
	}
	wait_for_completion(&ctx_ctrl.cb_done);

	if (ctx_ctrl.ern || ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		PMEPRINFO("read flow error %d\n", ctx_ctrl.cmd_status);
		ret = -EFAULT;
		goto done;
	}
	local_scan_params.residue.enable = params.ren;
	local_scan_params.residue.length = params.rlen;
	local_scan_params.sre.sessionid = params.sessionid;
	local_scan_params.sre.verbose = params.srvm;
	local_scan_params.sre.esee = params.esee;
	local_scan_params.dxe.clim = params.clim;
	local_scan_params.dxe.mlim = params.mlim;
	spin_lock(&session->set_subset_lock);
	local_scan_params.pattern.set = session->set;
	local_scan_params.pattern.subset = session->subset;
	spin_unlock(&session->set_subset_lock);

	if (copy_to_user(user_scan_params, &local_scan_params,
			sizeof(local_scan_params))) {
		pr_err("Error copying to user data\n");
		ret = -EFAULT;
	}
done:
	return ret;
}

static int setscan_cmd(struct file *fp, struct scan_session *session,
	struct pme_scan_params __user *user_params)
{
	int ret = 0;
	u32 flag = WAIT_AND_INTERRUPTABLE;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};
	struct pme_flow params;
	struct pme_scan_params local_params;

	pme_sw_flow_init(&params);
	init_completion(&ctx_ctrl.cb_done);
	if (copy_from_user(&local_params, user_params, sizeof(local_params)))
		return -EFAULT;

	/* must be enabled */
	if (pme_ctx_is_disabled(&session->ctx)) {
		ret = -EINVAL;
		goto done;
	}
	/* Only send a flw_ctx_w if PME_SCAN_PARAMS_{RESIDUE, SRE or DXE}
	 * is being done */
	if (local_params.flags == PME_SCAN_PARAMS_PATTERN)
		goto set_subset;
	if (local_params.flags & PME_SCAN_PARAMS_RESIDUE)
		flag |= PME_CMD_FCW_RES;
	if (local_params.flags & PME_SCAN_PARAMS_SRE)
		flag |= PME_CMD_FCW_SRE;
	if (local_params.flags & PME_SCAN_PARAMS_DXE)
		flag |= PME_CMD_FCW_DXE;
	params.ren = local_params.residue.enable;
	params.sessionid = local_params.sre.sessionid;
	params.srvm = local_params.sre.verbose;
	params.esee = local_params.sre.esee;
	params.clim = local_params.dxe.clim;
	params.mlim = local_params.dxe.mlim;

	ret = pme_ctx_ctrl_update_flow(&session->ctx, flag, &params,
			&ctx_ctrl.ctx_ctr);
	if (ret) {
		PMEPRINFO("update flow error %d\n", ret);
		goto done;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.ern || ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		PMEPRINFO("update flow err %d\n", ctx_ctrl.cmd_status);
		ret = -EFAULT;
		goto done;
	}

set_subset:
	if (local_params.flags & PME_SCAN_PARAMS_PATTERN) {
		spin_lock(&session->set_subset_lock);
		session->set = local_params.pattern.set;
		session->subset = local_params.pattern.subset;
		spin_unlock(&session->set_subset_lock);
		goto done;
	}
done:
	return ret;
}

static int resetseq_cmd(struct file *fp, struct scan_session *session)
{
	int ret = 0;
	struct pme_flow params;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};
	init_completion(&ctx_ctrl.cb_done);
	pme_sw_flow_init(&params);

	/* must be enabled */
	if (pme_ctx_is_disabled(&session->ctx)) {
		pr_err("pme2_scan: ctx is disabled\n");
		ret =  -EINVAL;
		goto done;
	}
	pme_flow_seqnum_set64(&params, 0);
	params.sos = 1;

	ret = pme_ctx_ctrl_update_flow(&session->ctx, PME_CMD_FCW_SEQ, &params,
			&ctx_ctrl.ctx_ctr);
	if (ret) {
		pr_err("pme2_scan: update flow error %d\n", ret);
		return ret;
	}
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.ern || ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		PMEPRINFO("update flow err %d\n", ctx_ctrl.cmd_status);
		ret = -EFAULT;
	}
done:
	return ret;
}

static int resetresidue_cmd(struct file *fp, struct scan_session *session)
{
	int ret = 0;
	struct pme_flow params;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};

	init_completion(&ctx_ctrl.cb_done);
	pme_sw_flow_init(&params);
	/* must be enabled */
	if (pme_ctx_is_disabled(&session->ctx)) {
		pr_err("pme2_scan: ctx is disabled\n");
		ret =  -EINVAL;
		goto done;
	}
	params.rlen = 0;
	ret = pme_ctx_ctrl_update_flow(&session->ctx,
			WAIT_AND_INTERRUPTABLE | PME_CTX_OP_RESETRESLEN,
			&params, &ctx_ctrl.ctx_ctr);
	if (ret)
		pr_info("pme2_scan: update flow error %d\n", ret);
	wait_for_completion(&ctx_ctrl.cb_done);
	if (ctx_ctrl.ern || ctx_ctrl.cmd_status || ctx_ctrl.res_flag) {
		PMEPRINFO("update flow err %d\n", ctx_ctrl.cmd_status);
		ret = -EFAULT;
	}
done:
	return ret;
}

static int process_scan_cmd(
		struct file *fp,
		struct scan_session *session,
		struct pme_scan_cmd *user_cmd,
		struct pme_scan_result *user_ret,
		u8 synchronous)
{
	int ret = 0;
	struct cmd_token local_token;
	struct cmd_token *token_p = NULL;
	DECLARE_WAIT_QUEUE_HEAD(local_waitqueue);
	u8 scan_flags = 0;

	BUG_ON(synchronous && !user_ret);

	/* If synchronous, use a local token (from the stack)
	 * If asynchronous, allocate a token to use */
	if (synchronous)
		token_p = &local_token;
	else {
		token_p = kmalloc(sizeof(*token_p), GFP_KERNEL);
		if (!token_p)
			return -ENOMEM;
	}
	memset(token_p, 0, sizeof(*token_p));
	/* Copy the command to kernel space */
	memcpy(&token_p->kernel_op, user_cmd, sizeof(struct pme_scan_cmd));
	/* Copy the input */
	token_p->synchronous = synchronous;
	token_p->tx_size = token_p->kernel_op.input.size;
	token_p->tx_data = kmalloc(token_p->kernel_op.input.size, GFP_KERNEL);
	if (!token_p->tx_data) {
		pr_err("pme2_scan: Err alloc %zd byte", token_p->tx_size);
		cleanup_token(token_p);
		return -ENOMEM;
	}
	if (copy_from_user(token_p->tx_data,
			token_p->kernel_op.input.data,
			token_p->kernel_op.input.size)) {
		pr_err("Error copying contigous user data\n");
		cleanup_token(token_p);
		return -EFAULT;
	}
	/* Setup input frame */
	token_p->tx_comp[INPUT_FRM].final = 1;
	token_p->tx_comp[INPUT_FRM].length = token_p->tx_size;
	qm_sg_entry_set64(&token_p->tx_comp[INPUT_FRM],
			pme_map(token_p->tx_data));
	/* setup output frame, if output is expected */
	if (token_p->kernel_op.output.size) {
		token_p->rx_size = token_p->kernel_op.output.size;
		PMEPRINFO("pme2_scan: expect output %d\n", token_p->rx_size);
		token_p->rx_data = kmalloc(token_p->rx_size, GFP_KERNEL);
		if (!token_p->rx_data) {
			pr_err("pme2_scan: Err alloc %zd byte",
					token_p->rx_size);
			cleanup_token(token_p);
			return -ENOMEM;
		}
		/* Setup output frame */
		token_p->tx_comp[OUTPUT_FRM].length = token_p->rx_size;
		qm_sg_entry_set64(&token_p->tx_comp[OUTPUT_FRM],
				pme_map(token_p->rx_data));
		token_p->tx_fd.format = qm_fd_compound;
		/* Build compound frame */
		qm_fd_addr_set64(&token_p->tx_fd,
				pme_map(token_p->tx_comp));
	} else {
		token_p->tx_fd.format = qm_fd_sg_big;
		/* Build sg frame */
		qm_fd_addr_set64(&token_p->tx_fd,
				pme_map(&token_p->tx_comp[INPUT_FRM]));
		token_p->tx_fd.length29 = token_p->tx_size;
	}

	/* use the local wait queue if synchronous, the shared
	 * queue if asynchronous */
	if (synchronous)
		token_p->queue = &local_waitqueue;
	else
		token_p->queue = &session->waiting_for_completion;
	token_p->done = 0;

	if (token_p->kernel_op.flags & PME_SCAN_CMD_STARTRESET)
		scan_flags |= PME_CMD_SCAN_SR;
	if (token_p->kernel_op.flags & PME_SCAN_CMD_END)
		scan_flags |= PME_CMD_SCAN_E;
	ret = pme_ctx_scan(&session->ctx, WAIT_AND_INTERRUPTABLE,
		&token_p->tx_fd,
		PME_SCAN_ARGS(scan_flags, session->set, session->subset),
		&token_p->hl_token);
	if (unlikely(ret)) {
		cleanup_token(token_p);
		return ret;
	}

	if (!synchronous)
		/* Don't wait.  The command is away */
		return 0;

	PMEPRINFO("Wait for completion\n");
	/* Wait for the command to complete */
	/* TODO: Should this be wait_event_interruptible ?
	 * If so, will need logic to indicate */
	wait_event(*token_p->queue, token_p->done == 1);
	return process_completed_token(fp, token_p, user_ret);
}

/**
 * fsl_pme2_scan_open - open the driver
 *
 * Open the driver and prepare for requests.
 *
 * Every time an application opens the driver, we create a scan_session object
 * for that file handle.
 */
static int fsl_pme2_scan_open(struct inode *node, struct file *fp)
{
	int ret;
	struct scan_session *session;
	struct pme_flow flow;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};

	pme_sw_flow_init(&flow);
	init_completion(&ctx_ctrl.cb_done);
	PMEPRINFO("pme2_scan: open %d\n", smp_processor_id());
	fp->private_data = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!fp->private_data)
		return -ENOMEM;
	session = (struct scan_session *)fp->private_data;
	/* Set up the structures used for asynchronous requests */
	init_waitqueue_head(&session->waiting_for_completion);
	INIT_LIST_HEAD(&session->completed_commands);
	spin_lock_init(&session->completed_commands_lock);
	spin_lock_init(&session->set_subset_lock);
	PMEPRINFO("kmalloc session %p\n", fp->private_data);
	session = fp->private_data;
	session->ctx.cb = scan_cb;
	session->ctx.ern_cb = scan_ern_cb;

	/* qosin, qosout should be driver attributes */
	ret = pme_ctx_init(&session->ctx, PME_CTX_FLAG_LOCAL, 0, 4, 4, 0, NULL);
	if (ret) {
		pr_err("pme2_scan: pme_ctx_init %d\n", ret);
		goto exit;
	}
	/* enable the context */
	ret = pme_ctx_enable(&session->ctx);
	if (ret) {
		PMEPRINFO("error enabling ctx %d\n", ret);
		pme_ctx_finish(&session->ctx);
		goto exit;
	}
	/* Update flow to set sane defaults in the flow context */
	ret = pme_ctx_ctrl_update_flow(&session->ctx,
		PME_CTX_OP_WAIT | PME_CMD_FCW_ALL, &flow, &ctx_ctrl.ctx_ctr);
	if (!ret) {
		wait_for_completion(&ctx_ctrl.cb_done);
		if (ctx_ctrl.ern || ctx_ctrl.cmd_status || ctx_ctrl.res_flag)
			ret = -EFAULT;
	}
	if (ret) {
		int my_ret;
		PMEPRINFO("error updating flow ctx %d\n", ret);
		my_ret = pme_ctx_disable(&session->ctx, PME_CTX_OP_WAIT,
					&ctx_ctrl.ctx_ctr);
		if (my_ret > 0)
			wait_for_completion(&ctx_ctrl.cb_done);
		else if (my_ret < 0)
			PMEPRINFO("error disabling ctx %d\n", ret);
		pme_ctx_finish(&session->ctx);
		goto exit;
	}
	/* Set up the structures used for asynchronous requests */
	PMEPRINFO("pme2_scan: Finish pme_scan open %d\n", smp_processor_id());
	return 0;
exit:
	kfree(fp->private_data);
	fp->private_data = NULL;
	return ret;
}

static int fsl_pme2_scan_close(struct inode *node, struct file *fp)
{
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb,
		.cmd_status = 0,
		.res_flag = 0,
		.ern = 0
	};
	int ret = 0;
	struct scan_session *session = fp->private_data;

	init_completion(&ctx_ctrl.cb_done);
	/* Before disabling check to see if it's already disabled. This can
	 * happen if a pme serious error has occurred for instance.*/
	if (!pme_ctx_is_disabled(&session->ctx)) {
		ret = pme_ctx_disable(&session->ctx, PME_CTX_OP_WAIT,
					&ctx_ctrl.ctx_ctr);
		if (ret > 0) {
			wait_for_completion(&ctx_ctrl.cb_done);
			if (ctx_ctrl.ern)
				PMEPRCRIT("Unexpected ERN\n");
		} else if (ret < 0) {
			pr_err("pme2_scan: Error disabling ctx %d\n", ret);
			return ret;
		}
	}
	pme_ctx_finish(&session->ctx);
	kfree(session);
	PMEPRINFO("pme2_scan: Finish pme_session close\n");
	return 0;
}

static unsigned int fsl_pme2_scan_poll(struct file *fp,
				      struct poll_table_struct *wait)
{
	struct scan_session *session;
	unsigned int mask = POLLOUT | POLLWRNORM;

	if (!fp->private_data)
		return -EINVAL;

	session = (struct scan_session *)fp->private_data;

	poll_wait(fp, &session->waiting_for_completion, wait);

	if (!scan_data_empty(session))
		mask |= (POLLIN | POLLRDNORM);
	return mask;
}


/* Main switch loop for ioctl operations */
static long fsl_pme2_scan_ioctl(struct file *fp, unsigned int cmd,
				unsigned long arg)
{
	struct scan_session *session = fp->private_data;
	int ret = 0;

	switch (cmd) {

	case PMEIO_GETSCAN:
		return getscan_cmd(fp, session, (struct pme_scan_params *)arg);
	break;

	case PMEIO_SETSCAN:
		return setscan_cmd(fp, session, (struct pme_scan_params *)arg);
	break;

	case PMEIO_RESETSEQ:
		return resetseq_cmd(fp, session);
	break;

	case PMEIO_RESETRES:
		return resetresidue_cmd(fp, session);
	break;

	case PMEIO_SCAN:
	{
		int ret;
		struct pme_scan scan;

		if (copy_from_user(&scan, (void __user *)arg, sizeof(scan)))
			return -EFAULT;
		ret = process_scan_cmd(fp, session, &scan.cmd, &scan.result, 1);
		if (!ret) {
			struct pme_scan_result __user *user_result =
				&((struct pme_scan __user *)arg)->result;
			ret = copy_to_user(user_result, &scan.result,
						sizeof(*user_result));
		}
		return ret;
	}
	break;

	case PMEIO_SCAN_W1:
	{
		struct pme_scan_cmd scan_cmd;

		if (copy_from_user(&scan_cmd, (void __user *)arg,
					sizeof(scan_cmd)))
			return -EFAULT;
		return process_scan_cmd(fp, session, &scan_cmd, NULL, 0);
	}
	break;

	case PMEIO_SCAN_R1:
	{
		struct pme_scan_result result;
		struct cmd_token *completed_cmd = NULL;
		struct pme_scan_result __user *ur =
			(struct pme_scan_result __user *)arg;
		int ret;

		if (copy_from_user(&result, (void __user *)arg,
				sizeof(result)))
			return -EFAULT;

		/* Check to see if any results */
		spin_lock(&session->completed_commands_lock);
		if (!list_empty(&session->completed_commands)) {
			completed_cmd = list_first_entry(
					&session->completed_commands,
					struct cmd_token,
					completed_list);
			list_del(&completed_cmd->completed_list);
			session->completed_count--;
		}
		spin_unlock(&session->completed_commands_lock);
		if (completed_cmd) {
			ret = process_completed_token(fp, completed_cmd,
					&result);
			if (!ret)
				ret = copy_to_user(ur, &result, sizeof(result));
			return ret;
		} else
			return -EIO;
	}
	break;

	case PMEIO_SCAN_Wn:
	{
		struct pme_scan_cmds scan_cmds;
		int i, ret = 0;

		/* Copy the command to kernel space */
		if (copy_from_user(&scan_cmds, (void __user *)arg,
				sizeof(scan_cmds)))
			return -EFAULT;
		PMEPRINFO("Received Wn for %d cmds\n", scan_cmds.num);
		for (i = 0; i < scan_cmds.num; i++) {
			struct pme_scan_cmd scan_cmd;

			if (copy_from_user(&scan_cmd, &scan_cmds.cmds[i],
					sizeof(scan_cmd))) {
				pr_err("pme2_scan: Err with %d\n", i);
				scan_cmds.num = i;
				if (copy_to_user((void __user *)arg, &scan_cmds,
						sizeof(scan_cmds))) {
					return -EFAULT;
				}
				return -EFAULT;
			}
			ret = process_scan_cmd(fp, session, &scan_cmd, NULL, 0);
			if (ret) {
				pr_err("pme2_scan: Err with %d cmd %d\n",
					i, ret);
				scan_cmds.num = i;
				if (copy_to_user((void *)arg, &scan_cmds,
						sizeof(scan_cmds))) {
					pr_err("Error copying to user data\n");
					return -EFAULT;
				}
				return -EINTR;
			}
		}
		return ret;
	}
	break;

	case PMEIO_SCAN_Rn:
	{
		struct pme_scan_results results;
		struct pme_scan_result result;
		struct pme_scan_result __user *ur;
		int i = 0, ret = 0;
		struct cmd_token *completed_cmd = NULL;

		/* Copy the command to kernel space */
		if (copy_from_user(&results, (void __user *)arg,
				sizeof(results)))
			return -EFAULT;
		ur = ((struct pme_scan_results __user *)arg)->results
		PMEPRINFO("pme2_scan: Received Rn for %d res\n", results.num);
		if (!results.num)
			return 0;
		do {
			completed_cmd = NULL;
			ret = 0;
			/* Check to see if any results */
			spin_lock(&session->completed_commands_lock);
			if (!list_empty(&session->completed_commands)) {
				/* Move to a different list */
				PMEPRINFO("pme2_scan: Pop response\n");
				completed_cmd = list_first_entry(
						&session->completed_commands,
						struct cmd_token,
						completed_list);
				list_del(&completed_cmd->completed_list);
				session->completed_count--;
			}
			spin_unlock(&session->completed_commands_lock);
			if (completed_cmd) {
				if (copy_from_user(&result, (void __user *)ur+i,
						sizeof(result)))
					return -EFAULT;
				ret = process_completed_token(fp, completed_cmd,
						&result);
				if (!ret)
					ret = copy_to_user(ur, &result,
						sizeof(struct pme_scan_result));
				if (!ret) {
					i++;
					ur++;
				}
			}
		} while (!ret && completed_cmd && (i != results.num));

		if (i != results.num) {
			PMEPRINFO("pme2_scan: Only filled %d responses\n", i);
			results.num = i;
			PMEPRINFO("pme2_scan: results.num = %d\n", results.num);
			if (copy_to_user((void __user *)arg, &results,
					sizeof(struct pme_scan_results))) {
				pr_err("Error copying to user data\n");
				return -EFAULT;
			}
		}
		return ret;
	}
	break;

	case PMEIO_RELEASE_BUFS:
		return -EINVAL;
		break;

#ifdef CONFIG_COMPAT
	case PMEIO_SCAN32:
	{
		int ret;
		struct compat_pme_scan scan32;
		struct compat_pme_scan __user *user_scan = compat_ptr(arg);
		struct pme_scan scan;

		if (copy_from_user(&scan32, user_scan, sizeof(scan32)))
			return -EFAULT;
		/* Convert to 64-bit structs */
		compat_to_scan_cmd(&scan.cmd, &scan32.cmd);
		compat_to_scan_result(&scan.result, &scan32.result);

		ret = process_scan_cmd(fp, session, &scan.cmd, &scan.result, 1);
		if (!ret) {
			struct compat_pme_scan_result __user *user_result =
				&user_scan->result;
			/* Convert to 32-bit struct */
			scan_result_to_compat(&scan32.result, &scan.result);
			ret = copy_to_user(user_result, &scan32.result,
						sizeof(*user_result));
		}
		return ret;
	}
	break;

	case PMEIO_SCAN_W132:
	{
		struct compat_pme_scan_cmd scan_cmd32;
		struct pme_scan_cmd scan_cmd;

		if (copy_from_user(&scan_cmd32, compat_ptr(arg),
				sizeof(scan_cmd32)))
			return -EFAULT;
		/* Convert to 64-bit struct */
		compat_to_scan_cmd(&scan_cmd, &scan_cmd32);
		return process_scan_cmd(fp, session, &scan_cmd, NULL, 0);
	}
	break;

	case PMEIO_SCAN_R132:
	{
		struct compat_pme_scan_result result32;
		struct pme_scan_result result;
		struct cmd_token *completed_cmd = NULL;
		struct compat_pme_scan_result __user *ur = compat_ptr(arg);
		int ret;

		if (copy_from_user(&result32, (void __user *)arg,
				sizeof(result32)))
			return -EFAULT;
		/* copy to 64-bit structure */
		compat_to_scan_result(&result, &result32);

		/* Check to see if any results */
		spin_lock(&session->completed_commands_lock);
		if (!list_empty(&session->completed_commands)) {
			completed_cmd = list_first_entry(
					&session->completed_commands,
					struct cmd_token,
					completed_list);
			list_del(&completed_cmd->completed_list);
			session->completed_count--;
		}
		spin_unlock(&session->completed_commands_lock);
		if (completed_cmd) {
			ret =  process_completed_token(fp, completed_cmd,
					&result);
			scan_result_to_compat(&result32, &result);
			ret = copy_to_user(ur, &result32, sizeof(result32));
		} else
			return -EIO;
	}
	break;

	case PMEIO_SCAN_Wn32:
	{
		struct compat_pme_scan_cmds scan_cmds32;
		int i, ret = 0;

		/* Copy the command to kernel space */
		if (copy_from_user(&scan_cmds32, compat_ptr(arg),
				sizeof(scan_cmds32)))
			return -EFAULT;
		PMEPRINFO("Received Wn for %d cmds\n", scan_cmds32.num);
		for (i = 0; i < scan_cmds32.num; i++) {
			struct pme_scan_cmd scan_cmd;
			struct compat_pme_scan_cmd __user *u_scan_cmd32;
			struct compat_pme_scan_cmd scan_cmd32;

			u_scan_cmd32 = compat_ptr(scan_cmds32.cmds);
			u_scan_cmd32 += i;

			if (copy_from_user(&scan_cmd32, u_scan_cmd32,
					sizeof(scan_cmd32))) {
				pr_err("pme2_scan: Err with %d\n", i);
				scan_cmds32.num = i;
				if (copy_to_user(compat_ptr(arg), &scan_cmds32,
							sizeof(scan_cmds32)))
					return -EFAULT;
				return -EFAULT;
			}
			compat_to_scan_cmd(&scan_cmd, &scan_cmd32);
			ret = process_scan_cmd(fp, session, &scan_cmd, NULL, 0);
			if (ret) {
				pr_err("pme2_scan: Err with %d cmd %d\n",
					i, ret);
				scan_cmds32.num = i;
				if (copy_to_user(compat_ptr(arg), &scan_cmds32,
							sizeof(scan_cmds32)))
					return -EFAULT;
				return -EINTR;
			}
		}
		return ret;
	}
	break;

	case PMEIO_SCAN_Rn32:
	{
		struct compat_pme_scan_results results32;
		int i = 0, ret = 0;
		struct cmd_token *completed_cmd = NULL;
		struct compat_pme_scan_result __user *ur;

		/* Copy the command to kernel space */
		if (copy_from_user(&results32, compat_ptr(arg),
				sizeof(results32)))
			return -EFAULT;
		ur = compat_ptr(results32.results);
		PMEPRINFO("pme2_scan: Rx Rn for %d res\n", results32.num);
		if (!results32.num)
			return 0;
		do {
			completed_cmd = NULL;
			ret = 0;
			/* Check to see if any results */
			spin_lock(&session->completed_commands_lock);
			if (!list_empty(&session->completed_commands)) {
				/* Move to a different list */
				PMEPRINFO("pme2_scan: Pop response\n");
				completed_cmd = list_first_entry(
						&session->completed_commands,
						struct cmd_token,
						completed_list);
				list_del(&completed_cmd->completed_list);
				session->completed_count--;
			}
			spin_unlock(&session->completed_commands_lock);
			if (completed_cmd) {
				struct compat_pme_scan_result l_result32;
				struct pme_scan_result result;

				if (copy_from_user(&l_result32, ur+i,
							sizeof(l_result32)))
						return -EFAULT;
				compat_to_scan_result(&result, &l_result32);
				ret = process_completed_token(fp, completed_cmd,
								&result);
				scan_result_to_compat(&l_result32, &result);
				ret = copy_to_user(ur+i, &l_result32,
							sizeof(l_result32));
				if (!ret)
					i++;
			}
		} while (!ret && completed_cmd && (i != results32.num));

		if (i != results32.num) {
			PMEPRINFO("pme2_scan: Only filled %d responses\n", i);
			results32.num = i;
			PMEPRINFO("pme2_scan: results32.num = %d\n",
				results32.num);
			if (copy_to_user(compat_ptr(arg), &results32,
					sizeof(struct pme_scan_results))) {
				pr_err("Error copying to user data\n");
				return -EFAULT;
			}
		}
		return ret;
	}
	break;
#endif /* CONFIG_COMPAT */

	default:
		pr_err("UNKNOWN IOCTL cmd 0x%x\n", cmd);
		return -EINVAL;
		break;
	}

	return ret;
}

static const struct file_operations fsl_pme2_scan_fops = {
	.owner		= THIS_MODULE,
	.open		= fsl_pme2_scan_open,
	.release	= fsl_pme2_scan_close,
	.poll		= fsl_pme2_scan_poll,
	.unlocked_ioctl = fsl_pme2_scan_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fsl_pme2_scan_ioctl,
#endif
};

static struct miscdevice fsl_pme2_scan_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PME_DEV_SCAN_NODE,
	.fops = &fsl_pme2_scan_fops
};

static int __init fsl_pme2_scan_init(void)
{
	int err = 0;

	pr_info("Freescale pme2 scan driver\n");
	err = misc_register(&fsl_pme2_scan_dev);
	if (err) {
		pr_err("fsl-pme2-scan: cannot register device\n");
		return err;
	}
	pr_info("fsl-pme2-scan: device %s registered\n",
		fsl_pme2_scan_dev.name);
	return 0;
}

static void __exit fsl_pme2_scan_exit(void)
{
	int err = misc_deregister(&fsl_pme2_scan_dev);
	if (err)
		pr_err("fsl-pme2-scan: Failed to deregister device %s, "
				"code %d\n", fsl_pme2_scan_dev.name, err);
	pr_info("fsl-pme2-scan: device %s deregistered\n",
			fsl_pme2_scan_dev.name);
}

module_init(fsl_pme2_scan_init);
module_exit(fsl_pme2_scan_exit);

MODULE_AUTHOR("Jeffrey Ladouceur <jeffrey.ladouceur@freescale.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Freescale PME2 scan driver");
