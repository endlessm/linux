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
#include <linux/compat.h>

/* Forward declaration */
static struct miscdevice fsl_pme2_db_dev;

/* Global spinlock for handling exclusive inc/dec */
static DEFINE_SPINLOCK(exclusive_lock);

/* Private structure that is allocated for each open that is done on the
 * pme_db device. This is used to maintain the state of a database session */
struct db_session {
	/* The ctx that is needed to communicate with the pme high level */
	struct pme_ctx ctx;
	/* Used to track the EXCLUSIVE_INC and EXCLUSIVE_DEC ioctls */
	unsigned int exclusive_counter;
};

struct cmd_token {
	/* pme high level token */
	struct pme_ctx_token hl_token;
	/* data */
	struct qm_fd rx_fd;
	/* Completion interface */
	struct completion cb_done;
	u8 ern;
};

#ifdef CONFIG_COMPAT
static void compat_to_db(struct pme_db *dst, struct compat_pme_db *src)
{
	dst->flags = src->flags;
	dst->status = src->status;
	dst->input.data = compat_ptr(src->input.data);
	dst->input.size = src->input.size;
	dst->output.data = compat_ptr(src->output.data);
	dst->output.size = src->output.size;
}

static void db_to_compat(struct compat_pme_db *dst, struct pme_db *src)
{
	dst->flags = src->flags;
	dst->status  = src->status;
	dst->output.data = ptr_to_compat(src->output.data);
	dst->output.size = src->output.size;
	dst->input.data = ptr_to_compat(src->input.data);
	dst->input.size = src->input.size;
}
#endif

/* PME Compound Frame Index */
#define INPUT_FRM	1
#define OUTPUT_FRM	0

/* Callback for database operations */
static void db_cb(struct pme_ctx *ctx, const struct qm_fd *fd,
				struct pme_ctx_token *ctx_token)
{
	struct cmd_token *token = (struct cmd_token *)ctx_token;
	token->rx_fd = *fd;
	complete(&token->cb_done);
}

static void db_ern_cb(struct pme_ctx *ctx, const struct qm_mr_entry *mr,
		struct pme_ctx_token *ctx_token)
{
	struct cmd_token *token = (struct cmd_token *)ctx_token;
	token->ern = 1;
	token->rx_fd = mr->ern.fd;
	complete(&token->cb_done);
}

struct ctrl_op {
	struct pme_ctx_ctrl_token ctx_ctr;
	struct completion cb_done;
	enum pme_status cmd_status;
	u8 res_flag;
	u8 ern;
};

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

static int exclusive_inc(struct file *fp, struct db_session *db)
{
	int ret;

	BUG_ON(!db);
	BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
	spin_lock(&exclusive_lock);
	ret = pme_ctx_exclusive_inc(&db->ctx,
			(PME_CTX_OP_WAIT | PME_CTX_OP_WAIT_INT));
	if (!ret)
		db->exclusive_counter++;
	spin_unlock(&exclusive_lock);
	return ret;
}

static int exclusive_dec(struct file *fp, struct db_session *db)
{
	int ret = 0;

	BUG_ON(!db);
	BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
	spin_lock(&exclusive_lock);
	if (!db->exclusive_counter) {
		PMEPRERR("exclusivity counter already zero\n");
		ret = -EINVAL;
	} else {
		pme_ctx_exclusive_dec(&db->ctx);
		db->exclusive_counter--;
	}
	spin_unlock(&exclusive_lock);
	return ret;
}

static int execute_cmd(struct file *fp, struct db_session *db,
			struct pme_db *db_cmd)
{
	int ret = 0;
	struct cmd_token token;
	struct qm_sg_entry tx_comp[2];
	struct qm_fd tx_fd;
	void *tx_data = NULL;
	void *rx_data = NULL;
	u32 src_sz, dst_sz;
	dma_addr_t dma_addr;

	memset(&token, 0, sizeof(struct cmd_token));
	memset(tx_comp, 0, sizeof(tx_comp));
	memset(&tx_fd, 0, sizeof(struct qm_fd));
	init_completion(&token.cb_done);

	PMEPRINFO("Received User Space Contiguous mem\n");
	PMEPRINFO("length = %d\n", db_cmd->input.size);
	tx_data = kmalloc(db_cmd->input.size, GFP_KERNEL);
	if (!tx_data) {
		PMEPRERR("Err alloc %zd byte\n", db_cmd->input.size);
		return -ENOMEM;
	}

	if (copy_from_user(tx_data,
			(void __user *)db_cmd->input.data,
			db_cmd->input.size)) {
		PMEPRERR("Error copying contigous user data\n");
		ret = -EFAULT;
		goto free_tx_data;
	}

	/* Setup input frame */
	tx_comp[INPUT_FRM].final = 1;
	tx_comp[INPUT_FRM].length = db_cmd->input.size;
	dma_addr = pme_map(tx_data);
	if (pme_map_error(dma_addr)) {
		PMEPRERR("Error pme_map_error\n");
		ret = -EIO;
		goto free_tx_data;
	}
	set_sg_addr(&tx_comp[INPUT_FRM], dma_addr);
	/* setup output frame, if output is expected */
	if (db_cmd->output.size) {
		PMEPRINFO("expect output %d\n", db_cmd->output.size);
		rx_data = kmalloc(db_cmd->output.size, GFP_KERNEL);
		if (!rx_data) {
			PMEPRERR("Err alloc %zd byte", db_cmd->output.size);
			ret = -ENOMEM;
			goto unmap_input_frame;
		}
		/* Setup output frame */
		tx_comp[OUTPUT_FRM].length = db_cmd->output.size;
		dma_addr = pme_map(rx_data);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto comp_frame_free_rx;
		}
		set_sg_addr(&tx_comp[OUTPUT_FRM], dma_addr);
		tx_fd.format = qm_fd_compound;
		/* Build compound frame */
		dma_addr = pme_map(tx_comp);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto comp_frame_unmap_output;
		}
		set_fd_addr(&tx_fd, dma_addr);
	} else {
		tx_fd.format = qm_fd_sg_big;
		tx_fd.length29 = db_cmd->input.size;
		/* Build sg frame */
		dma_addr = pme_map(&tx_comp[INPUT_FRM]);
		if (pme_map_error(dma_addr)) {
			PMEPRERR("Error pme_map_error\n");
			ret = -EIO;
			goto unmap_input_frame;
		}
		set_fd_addr(&tx_fd, dma_addr);
	}
	ret = pme_ctx_pmtcc(&db->ctx, PME_CTX_OP_WAIT, &tx_fd,
				(struct pme_ctx_token *)&token);
	if (unlikely(ret)) {
		PMEPRINFO("pme_ctx_pmtcc error %d\n", ret);
		goto unmap_frame;
	}
	PMEPRINFO("Wait for completion\n");
	/* Wait for the command to complete */
	wait_for_completion(&token.cb_done);

	if (token.ern) {
		ret = -EIO;
		goto unmap_frame;
	}

	PMEPRINFO("pme2_db: process_completed_token\n");
	PMEPRINFO("pme2_db: received %d frame type\n", token.rx_fd.format);
	if (token.rx_fd.format == qm_fd_compound) {
		/* Need to copy  output */
		src_sz = tx_comp[OUTPUT_FRM].length;
		dst_sz = db_cmd->output.size;
		PMEPRINFO("pme gen %u data, have space for %u\n",
				src_sz, dst_sz);
		db_cmd->output.size = min(dst_sz, src_sz);
		/* Doesn't make sense we generated more than available space
		 * should have got truncation.
		 */
		BUG_ON(dst_sz < src_sz);
		if (copy_to_user((void __user *)db_cmd->output.data, rx_data,
				db_cmd->output.size)) {
			PMEPRERR("Error copying to user data\n");
			ret = -EFAULT;
			goto comp_frame_unmap_cf;
		}
	} else if (token.rx_fd.format == qm_fd_sg_big)
		db_cmd->output.size = 0;
	else
		panic("unexpected frame type received %d\n",
				token.rx_fd.format);

	db_cmd->flags = pme_fd_res_flags(&token.rx_fd);
	db_cmd->status = pme_fd_res_status(&token.rx_fd);

unmap_frame:
	if (token.rx_fd.format == qm_fd_sg_big)
		goto single_frame_unmap_frame;

comp_frame_unmap_cf:
comp_frame_unmap_output:
comp_frame_free_rx:
	kfree(rx_data);
	goto unmap_input_frame;
single_frame_unmap_frame:
unmap_input_frame:
free_tx_data:
	kfree(tx_data);

	return ret;
}

static int execute_nop(struct file *fp, struct db_session *db)
{
	int ret = 0;
	struct ctrl_op ctx_ctrl =  {
		.ctx_ctr.cb = ctrl_cb,
		.ctx_ctr.ern_cb = ctrl_ern_cb
	};
	init_completion(&ctx_ctrl.cb_done);

	ret = pme_ctx_ctrl_nop(&db->ctx, PME_CTX_OP_WAIT|PME_CTX_OP_WAIT_INT,
			&ctx_ctrl.ctx_ctr);
	if (!ret)
		wait_for_completion(&ctx_ctrl.cb_done);

	if (ctx_ctrl.ern)
		ret = -EIO;
	return ret;
}

static atomic_t sre_reset_lock = ATOMIC_INIT(1);
static int ioctl_sre_reset(unsigned long arg)
{
	struct pme_db_sre_reset reset_vals;
	int i;
	u32 srrr_val;
	int ret = 0;

	if (copy_from_user(&reset_vals, (struct pme_db_sre_reset __user *)arg,
			sizeof(struct pme_db_sre_reset)))
		return -EFAULT;
	PMEPRINFO("sre_reset:\n");
	PMEPRINFO("  rule_index = 0x%x:\n", reset_vals.rule_index);
	PMEPRINFO("  rule_increment = 0x%x:\n", reset_vals.rule_increment);
	PMEPRINFO("  rule_repetitions = 0x%x:\n", reset_vals.rule_repetitions);
	PMEPRINFO("  rule_reset_interval = 0x%x:\n",
			reset_vals.rule_reset_interval);
	PMEPRINFO("  rule_reset_priority = 0x%x:\n",
			reset_vals.rule_reset_priority);

	/* Validate ranges */
	if ((reset_vals.rule_index >= PME_PMFA_SRE_INDEX_MAX) ||
			(reset_vals.rule_increment > PME_PMFA_SRE_INC_MAX) ||
			(reset_vals.rule_repetitions >= PME_PMFA_SRE_REP_MAX) ||
			(reset_vals.rule_reset_interval >=
				PME_PMFA_SRE_INTERVAL_MAX))
		return -ERANGE;
	/* Check and make sure only one caller is present */
	if (!atomic_dec_and_test(&sre_reset_lock)) {
		/* Someone else is already in this call */
		atomic_inc(&sre_reset_lock);
		return -EBUSY;
	};
	/* All validated.  Run the command */
	for (i = 0; i < PME_SRE_RULE_VECTOR_SIZE; i++)
		pme_attr_set(pme_attr_srrv0 + i, reset_vals.rule_vector[i]);
	pme_attr_set(pme_attr_srrfi, reset_vals.rule_index);
	pme_attr_set(pme_attr_srri, reset_vals.rule_increment);
	pme_attr_set(pme_attr_srrwc,
			(0xFFF & reset_vals.rule_reset_interval) << 1 |
			(reset_vals.rule_reset_priority ? 1 : 0));
	/* Need to set SRRR last */
	pme_attr_set(pme_attr_srrr, reset_vals.rule_repetitions);
	do {
		mdelay(PME_PMFA_SRE_POLL_MS);
		ret = pme_attr_get(pme_attr_srrr, &srrr_val);
		if (ret) {
			PMEPRCRIT("pme2: Error reading srrr\n");
			/* bail */
			break;
		}
		/* Check for error */
		else if (srrr_val & 0x10000000) {
			PMEPRERR("pme2: Error in SRRR\n");
			ret = -EIO;
		}
		PMEPRINFO("pme2: srrr count %d\n", srrr_val);
	} while (srrr_val);
	atomic_inc(&sre_reset_lock);
	return ret;
}

/**
 * fsl_pme2_db_open - open the driver
 *
 * Open the driver and prepare for requests.
 *
 * Every time an application opens the driver, we create a db_session object
 * for that file handle.
 */
static int fsl_pme2_db_open(struct inode *node, struct file *fp)
{
	int ret;
	struct db_session *db = NULL;

	db = kzalloc(sizeof(struct db_session), GFP_KERNEL);
	if (!db)
		return -ENOMEM;
	fp->private_data = db;
	db->ctx.cb = db_cb;
	db->ctx.ern_cb = db_ern_cb;

	ret = pme_ctx_init(&db->ctx,
			PME_CTX_FLAG_EXCLUSIVE |
			PME_CTX_FLAG_PMTCC |
			PME_CTX_FLAG_DIRECT|
			PME_CTX_FLAG_LOCAL,
			0, 4, CONFIG_FSL_PME2_DB_QOSOUT_PRIORITY, 0, NULL);
	if (ret) {
		PMEPRERR("pme_ctx_init %d\n", ret);
		goto free_data;
	}

	/* enable the context */
	ret = pme_ctx_enable(&db->ctx);
	if (ret) {
		PMEPRERR("error enabling ctx %d\n", ret);
		pme_ctx_finish(&db->ctx);
		goto free_data;
	}
	PMEPRINFO("pme2_db: Finish pme_db open %d\n", smp_processor_id());
	return 0;
free_data:
	kfree(fp->private_data);
	fp->private_data = NULL;
	return ret;
}

static int fsl_pme2_db_close(struct inode *node, struct file *fp)
{
	int ret = 0;
	struct db_session *db = fp->private_data;

	PMEPRINFO("Start pme_db close\n");
	while (db->exclusive_counter) {
		pme_ctx_exclusive_dec(&db->ctx);
		db->exclusive_counter--;
	}

	/* Disable context. */
	ret = pme_ctx_disable(&db->ctx, PME_CTX_OP_WAIT, NULL);
	if (ret)
		PMEPRCRIT("Error disabling ctx %d\n", ret);
	pme_ctx_finish(&db->ctx);
	kfree(db);
	PMEPRINFO("Finish pme_db close\n");
	return 0;
}

/* Main switch loop for ioctl operations */
static long fsl_pme2_db_ioctl(struct file *fp, unsigned int cmd,
				unsigned long arg)
{
	struct db_session *db = fp->private_data;
	int ret = 0;

	switch (cmd) {

	case PMEIO_PMTCC: {
		int ret;
		struct pme_db db_cmd;

		/* Copy the command to kernel space */
		if (copy_from_user(&db_cmd, (void __user *)arg,
				sizeof(db_cmd)))
			return -EFAULT;
		ret = execute_cmd(fp, db, &db_cmd);
		if (!ret)
			ret = copy_to_user((struct pme_db __user *)arg,
						&db_cmd, sizeof(db_cmd));
		return ret;
	}
	break;

	case PMEIO_EXL_INC:
		return exclusive_inc(fp, db);
	case PMEIO_EXL_DEC:
		return exclusive_dec(fp, db);
	case PMEIO_EXL_GET:
		BUG_ON(!db);
		BUG_ON(!(db->ctx.flags & PME_CTX_FLAG_EXCLUSIVE));
		if (copy_to_user((void __user *)arg,
				&db->exclusive_counter,
				sizeof(db->exclusive_counter)))
			ret = -EFAULT;
		return ret;
	case PMEIO_NOP:
		return execute_nop(fp, db);
	case PMEIO_SRE_RESET:
		return ioctl_sre_reset(arg);

#ifdef CONFIG_COMPAT
	case PMEIO_PMTCC32: {
		int ret;
		struct pme_db db_cmd;
		struct compat_pme_db db_cmd32;
		struct compat_pme_db __user *user_db_cmd = compat_ptr(arg);

		/* Copy the command to kernel space */
		if (copy_from_user(&db_cmd32, user_db_cmd, sizeof(db_cmd32)))
			return -EFAULT;
		/* Convert to 64-bit struct */
		compat_to_db(&db_cmd, &db_cmd32);
		ret = execute_cmd(fp, db, &db_cmd);
		if (!ret) {
			/* Convert to compat struct */
			db_to_compat(&db_cmd32, &db_cmd);
			ret = copy_to_user(user_db_cmd, &db_cmd32,
						sizeof(*user_db_cmd));
		}
		return ret;
	}
	break;
#endif
	}
	pr_info("Unknown pme_db ioctl cmd %u\n", cmd);
	return -EINVAL;
}

static const struct file_operations fsl_pme2_db_fops = {
	.owner		= THIS_MODULE,
	.open		= fsl_pme2_db_open,
	.release	= fsl_pme2_db_close,
	.unlocked_ioctl	= fsl_pme2_db_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= fsl_pme2_db_ioctl,
#endif
};

static struct miscdevice fsl_pme2_db_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = PME_DEV_DB_NODE,
	.fops = &fsl_pme2_db_fops
};

static int __init fsl_pme2_db_init(void)
{
	int err = 0;

	pr_info("Freescale pme2 db driver\n");
	if (!pme2_have_control()) {
		PMEPRERR("not on ctrl-plane\n");
		return -ENODEV;
	}
	err = misc_register(&fsl_pme2_db_dev);
	if (err) {
		PMEPRERR("cannot register device\n");
		return err;
	}
	PMEPRINFO("device %s registered\n", fsl_pme2_db_dev.name);
	return 0;
}

static void __exit fsl_pme2_db_exit(void)
{
	int err = misc_deregister(&fsl_pme2_db_dev);
	if (err) {
		PMEPRERR("Failed to deregister device %s, "
			"code %d\n", fsl_pme2_db_dev.name, err);
		return;
	}
	PMEPRINFO("device %s deregistered\n", fsl_pme2_db_dev.name);
}

module_init(fsl_pme2_db_init);
module_exit(fsl_pme2_db_exit);

MODULE_AUTHOR("Freescale Semiconductor - OTC");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 db driver");
