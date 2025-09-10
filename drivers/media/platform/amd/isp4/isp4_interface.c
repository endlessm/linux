// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#include <drm/amd/isp.h>
#include <linux/iopoll.h>
#include <linux/mutex.h>

#include "isp4_debug.h"
#include "isp4_fw_cmd_resp.h"
#include "isp4_hw_reg.h"
#include "isp4_interface.h"

#define ISP4IF_FW_RESP_RB_IRQ_EN_MASK \
	(ISP_SYS_INT0_EN__SYS_INT_RINGBUFFER_WPT9_EN_MASK |  \
	 ISP_SYS_INT0_EN__SYS_INT_RINGBUFFER_WPT10_EN_MASK | \
	 ISP_SYS_INT0_EN__SYS_INT_RINGBUFFER_WPT11_EN_MASK | \
	 ISP_SYS_INT0_EN__SYS_INT_RINGBUFFER_WPT12_EN_MASK)

struct isp4if_rb_config {
	const char *name;
	u32 index;
	u32 reg_rptr;
	u32 reg_wptr;
	u32 reg_base_lo;
	u32 reg_base_hi;
	u32 reg_size;
	u32 val_size;
	u64 base_mc_addr;
	void *base_sys_addr;
};

/* FW cmd ring buffer configuration */
static struct isp4if_rb_config
	isp4if_cmd_rb_config[ISP4IF_STREAM_ID_MAX] = {
	{
		.name = "CMD_RB_GBL0",
		.index = 3,
		.reg_rptr = ISP_RB_RPTR4,
		.reg_wptr = ISP_RB_WPTR4,
		.reg_base_lo = ISP_RB_BASE_LO4,
		.reg_base_hi = ISP_RB_BASE_HI4,
		.reg_size = ISP_RB_SIZE4,
	},
	{
		.name = "CMD_RB_STR1",
		.index = 0,
		.reg_rptr = ISP_RB_RPTR1,
		.reg_wptr = ISP_RB_WPTR1,
		.reg_base_lo = ISP_RB_BASE_LO1,
		.reg_base_hi = ISP_RB_BASE_HI1,
		.reg_size = ISP_RB_SIZE1,
	},
	{
		.name = "CMD_RB_STR2",
		.index = 1,
		.reg_rptr = ISP_RB_RPTR2,
		.reg_wptr = ISP_RB_WPTR2,
		.reg_base_lo = ISP_RB_BASE_LO2,
		.reg_base_hi = ISP_RB_BASE_HI2,
		.reg_size = ISP_RB_SIZE2,
	},
	{
		.name = "CMD_RB_STR3",
		.index = 2,
		.reg_rptr = ISP_RB_RPTR3,
		.reg_wptr = ISP_RB_WPTR3,
		.reg_base_lo = ISP_RB_BASE_LO3,
		.reg_base_hi = ISP_RB_BASE_HI3,
		.reg_size = ISP_RB_SIZE3,
	},
};

/* FW resp ring buffer configuration */
static struct isp4if_rb_config
	isp4if_resp_rb_config[ISP4IF_STREAM_ID_MAX] = {
	{
		.name = "RES_RB_GBL0",
		.index = 3,
		.reg_rptr = ISP_RB_RPTR12,
		.reg_wptr = ISP_RB_WPTR12,
		.reg_base_lo = ISP_RB_BASE_LO12,
		.reg_base_hi = ISP_RB_BASE_HI12,
		.reg_size = ISP_RB_SIZE12,
	},
	{
		.name = "RES_RB_STR1",
		.index = 0,
		.reg_rptr = ISP_RB_RPTR9,
		.reg_wptr = ISP_RB_WPTR9,
		.reg_base_lo = ISP_RB_BASE_LO9,
		.reg_base_hi = ISP_RB_BASE_HI9,
		.reg_size = ISP_RB_SIZE9,
	},
	{
		.name = "RES_RB_STR2",
		.index = 1,
		.reg_rptr = ISP_RB_RPTR10,
		.reg_wptr = ISP_RB_WPTR10,
		.reg_base_lo = ISP_RB_BASE_LO10,
		.reg_base_hi = ISP_RB_BASE_HI10,
		.reg_size = ISP_RB_SIZE10,
	},
	{
		.name = "RES_RB_STR3",
		.index = 2,
		.reg_rptr = ISP_RB_RPTR11,
		.reg_wptr = ISP_RB_WPTR11,
		.reg_base_lo = ISP_RB_BASE_LO11,
		.reg_base_hi = ISP_RB_BASE_HI11,
		.reg_size = ISP_RB_SIZE11,
	},
};

/* FW log ring buffer configuration */
static struct isp4if_rb_config isp4if_log_rb_config = {
	.name = "LOG_RB",
	.index = 0,
	.reg_rptr = ISP_LOG_RB_RPTR0,
	.reg_wptr = ISP_LOG_RB_WPTR0,
	.reg_base_lo = ISP_LOG_RB_BASE_LO0,
	.reg_base_hi = ISP_LOG_RB_BASE_HI0,
	.reg_size = ISP_LOG_RB_SIZE0,
};

static struct isp4if_gpu_mem_info *isp4if_gpu_mem_alloc(struct isp4_interface *ispif, u32 mem_size)
{
	struct isp4if_gpu_mem_info *mem_info;
	struct device *dev = ispif->dev;
	int ret;

	if (!mem_size)
		return NULL;

	mem_info = kzalloc(sizeof(*mem_info), GFP_KERNEL);
	if (!mem_info)
		return NULL;

	mem_info->mem_size = mem_size;
	ret = isp_kernel_buffer_alloc(dev, mem_info->mem_size, &mem_info->mem_handle,
				      &mem_info->gpu_mc_addr, &mem_info->sys_addr);
	if (ret) {
		kfree(mem_info);
		return NULL;
	}

	return mem_info;
}

static int isp4if_gpu_mem_free(struct isp4_interface *ispif, struct isp4if_gpu_mem_info *mem_info)
{
	struct device *dev = ispif->dev;

	if (!mem_info) {
		dev_err(dev, "invalid mem_info\n");
		return -EINVAL;
	}

	isp_kernel_buffer_free(&mem_info->mem_handle, &mem_info->gpu_mc_addr, &mem_info->sys_addr);

	kfree(mem_info);

	return 0;
}

static int isp4if_dealloc_fw_gpumem(struct isp4_interface *ispif)
{
	int i;

	if (ispif->fw_mem_pool) {
		isp4if_gpu_mem_free(ispif, ispif->fw_mem_pool);
		ispif->fw_mem_pool = NULL;
	}

	if (ispif->fw_cmd_resp_buf) {
		isp4if_gpu_mem_free(ispif, ispif->fw_cmd_resp_buf);
		ispif->fw_cmd_resp_buf = NULL;
	}

	if (ispif->fw_log_buf) {
		isp4if_gpu_mem_free(ispif, ispif->fw_log_buf);
		ispif->fw_log_buf = NULL;
	}

	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++) {
		if (ispif->metainfo_buf_pool[i]) {
			isp4if_gpu_mem_free(ispif, ispif->metainfo_buf_pool[i]);
			ispif->metainfo_buf_pool[i] = NULL;
		}
	}

	return 0;
}

static int isp4if_alloc_fw_gpumem(struct isp4_interface *ispif)
{
	struct device *dev = ispif->dev;
	unsigned int i;

	ispif->fw_mem_pool = isp4if_gpu_mem_alloc(ispif, FW_MEMORY_POOL_SIZE);
	if (!ispif->fw_mem_pool)
		goto error_no_memory;

	ispif->fw_cmd_resp_buf =
		isp4if_gpu_mem_alloc(ispif, ISP4IF_RB_PMBMAP_MEM_SIZE);
	if (!ispif->fw_cmd_resp_buf)
		goto error_no_memory;

	ispif->fw_log_buf =
		isp4if_gpu_mem_alloc(ispif, ISP4IF_FW_LOG_RINGBUF_SIZE);
	if (!ispif->fw_log_buf)
		goto error_no_memory;

	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++) {
		ispif->metainfo_buf_pool[i] =
			isp4if_gpu_mem_alloc(ispif,
					     ISP4IF_META_INFO_BUF_SIZE);
		if (!ispif->metainfo_buf_pool[i])
			goto error_no_memory;
	}

	return 0;

error_no_memory:
	dev_err(dev, "failed to allocate gpu memory\n");
	return -ENOMEM;
}

static u32 isp4if_compute_check_sum(u8 *buf, u32 buf_size)
{
	u32 checksum = 0;
	u8 *surplus_ptr;
	u32 *buffer;
	u32 i;

	buffer = (u32 *)buf;
	for (i = 0; i < buf_size / sizeof(u32); i++)
		checksum += buffer[i];

	surplus_ptr = (u8 *)&buffer[i];
	/* add surplus data crc checksum */
	for (i = 0; i < buf_size % sizeof(u32); i++)
		checksum += surplus_ptr[i];

	return checksum;
}

void isp4if_clear_cmdq(struct isp4_interface *ispif)
{
	struct isp4if_cmd_element *buf_node = NULL;
	struct isp4if_cmd_element *tmp_node = NULL;

	guard(mutex)(&ispif->cmdq_mutex);

	list_for_each_entry_safe(buf_node, tmp_node, &ispif->cmdq, list) {
		list_del(&buf_node->list);
		kfree(buf_node);
	}
}

static bool isp4if_is_cmdq_rb_full(struct isp4_interface *ispif, enum isp4if_stream_id cmd_buf_idx)
{
	struct isp4if_rb_config *rb_config;
	u32 rd_ptr, wr_ptr;
	u32 new_wr_ptr;
	u32 rreg;
	u32 wreg;
	u32 len;

	rb_config = &isp4if_cmd_rb_config[cmd_buf_idx];
	rreg = rb_config->reg_rptr;
	wreg = rb_config->reg_wptr;
	len = rb_config->val_size;

	rd_ptr = isp4hw_rreg(ispif->mmio, rreg);
	wr_ptr = isp4hw_rreg(ispif->mmio, wreg);

	new_wr_ptr = wr_ptr + sizeof(struct isp4fw_cmd);

	if (wr_ptr >= rd_ptr) {
		if (new_wr_ptr < len) {
			return false;
		} else if (new_wr_ptr == len) {
			if (rd_ptr == 0)
				return true;

			return false;
		}

		new_wr_ptr -= len;
		if (new_wr_ptr < rd_ptr)
			return false;

		return true;
	}

	if (new_wr_ptr < rd_ptr)
		return false;

	return true;
}

static struct isp4if_cmd_element *isp4if_append_cmd_2_cmdq(struct isp4_interface *ispif,
							   struct isp4if_cmd_element *cmd_ele)
{
	struct isp4if_cmd_element *copy_command = NULL;

	copy_command = kmemdup(cmd_ele, sizeof(*cmd_ele), GFP_KERNEL);
	if (!copy_command)
		return NULL;

	guard(mutex)(&ispif->cmdq_mutex);

	list_add_tail(&copy_command->list, &ispif->cmdq);

	return copy_command;
}

struct isp4if_cmd_element *isp4if_rm_cmd_from_cmdq(struct isp4_interface *ispif, u32 seq_num,
						   u32 cmd_id)
{
	struct isp4if_cmd_element *buf_node = NULL;
	struct isp4if_cmd_element *tmp_node = NULL;

	guard(mutex)(&ispif->cmdq_mutex);

	list_for_each_entry_safe(buf_node, tmp_node, &ispif->cmdq, list) {
		if (buf_node->seq_num == seq_num &&
		    buf_node->cmd_id == cmd_id) {
			list_del(&buf_node->list);
			return buf_node;
		}
	}

	return NULL;
}

static int isp4if_insert_isp_fw_cmd(struct isp4_interface *ispif, enum isp4if_stream_id stream,
				    struct isp4fw_cmd *cmd)
{
	struct isp4if_rb_config *rb_config;
	struct device *dev = ispif->dev;
	u64 mem_addr;
	u64 mem_sys;
	u32 wr_ptr;
	u32 rd_ptr;
	u32 rreg;
	u32 wreg;
	u32 len;

	rb_config = &isp4if_cmd_rb_config[stream];
	rreg = rb_config->reg_rptr;
	wreg = rb_config->reg_wptr;
	mem_sys = (u64)rb_config->base_sys_addr;
	mem_addr = rb_config->base_mc_addr;
	len = rb_config->val_size;

	if (isp4if_is_cmdq_rb_full(ispif, stream)) {
		dev_err(dev, "fail no cmdslot %s(%d)\n",
			isp4dbg_get_if_stream_str(stream), stream);
		return -EINVAL;
	}

	wr_ptr = isp4hw_rreg(ispif->mmio, wreg);
	rd_ptr = isp4hw_rreg(ispif->mmio, rreg);

	if (rd_ptr > len) {
		dev_err(dev, "fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			isp4dbg_get_if_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return -EINVAL;
	}

	if (wr_ptr > len) {
		dev_err(dev, "fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			isp4dbg_get_if_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return -EINVAL;
	}

	if (wr_ptr < rd_ptr) {
		mem_addr += wr_ptr;

		memcpy((u8 *)(mem_sys + wr_ptr),
		       (u8 *)cmd, sizeof(struct isp4fw_cmd));
	} else {
		if ((len - wr_ptr) >= (sizeof(struct isp4fw_cmd))) {
			mem_addr += wr_ptr;

			memcpy((u8 *)(mem_sys + wr_ptr),
			       (u8 *)cmd, sizeof(struct isp4fw_cmd));
		} else {
			u32 size;
			u8 *src;

			src = (u8 *)cmd;
			size = len - wr_ptr;

			memcpy((u8 *)(mem_sys + wr_ptr), src, size);

			src += size;
			size = sizeof(struct isp4fw_cmd) - size;
			memcpy((u8 *)(mem_sys), src, size);
		}
	}

	wr_ptr += sizeof(struct isp4fw_cmd);
	if (wr_ptr >= len)
		wr_ptr -= len;

	isp4hw_wreg(ispif->mmio, wreg, wr_ptr);

	return 0;
}

static inline enum isp4if_stream_id isp4if_get_fw_stream(u32 cmd_id)
{
	return ISP4IF_STREAM_ID_1;
}

static int isp4if_send_fw_cmd(struct isp4_interface *ispif, u32 cmd_id, void *package,
			      u32 package_size, wait_queue_head_t *wq, u32 *wq_cond, u32 *seq)
{
	enum isp4if_stream_id stream = isp4if_get_fw_stream(cmd_id);
	struct isp4if_cmd_element command_element = {};
	struct isp4if_gpu_mem_info *gpu_mem = NULL;
	struct isp4if_cmd_element *cmd_ele = NULL;
	struct isp4if_rb_config *rb_config;
	struct device *dev = ispif->dev;
	struct isp4fw_cmd cmd = {};
	u64 package_base = 0;
	u32 seq_num;
	u32 rreg;
	u32 wreg;
	int ret;

	if (package_size > sizeof(cmd.cmd_param)) {
		dev_err(dev, "fail pkgsize(%u)>%lu cmd:0x%x,stream %d\n",
			package_size, sizeof(cmd.cmd_param), cmd_id, stream);
		return -EINVAL;
	}

	rb_config = &isp4if_resp_rb_config[stream];
	rreg = rb_config->reg_rptr;
	wreg = rb_config->reg_wptr;

	guard(mutex)(&ispif->isp4if_mutex);

	ret = read_poll_timeout(isp4if_is_cmdq_rb_full, ret, !ret, ISP4IF_MAX_SLEEP_TIME * 1000,
				ISP4IF_MAX_SLEEP_COUNT * ISP4IF_MAX_SLEEP_TIME * 1000, false,
				ispif, stream);

	if (ret) {
		u32 rd_ptr = isp4hw_rreg(ispif->mmio, rreg);
		u32 wr_ptr = isp4hw_rreg(ispif->mmio, wreg);

		dev_err(dev,
			"failed to get free cmdq slot, stream %s(%d),rd %u, wr %u\n",
			isp4dbg_get_if_stream_str(stream),
			stream, rd_ptr, wr_ptr);
		return -ETIMEDOUT;
	}

	cmd.cmd_id = cmd_id;
	switch (stream) {
	case ISP4IF_STREAM_ID_GLOBAL:
		cmd.cmd_stream_id = STREAM_ID_INVALID;
		break;
	case ISP4IF_STREAM_ID_1:
		cmd.cmd_stream_id = STREAM_ID_1;
		break;
	default:
		dev_err(dev, "fail bad stream id %d\n", stream);
		return -EINVAL;
	}

	if (package && package_size)
		memcpy(cmd.cmd_param, package, package_size);

	seq_num = ispif->host2fw_seq_num++;
	cmd.cmd_seq_num = seq_num;
	cmd.cmd_check_sum =
		isp4if_compute_check_sum((u8 *)&cmd, sizeof(cmd) - 4);

	if (seq)
		*seq = seq_num;
	command_element.seq_num = seq_num;
	command_element.cmd_id = cmd_id;
	command_element.mc_addr = package_base;
	command_element.wq = wq;
	command_element.wq_cond = wq_cond;
	command_element.gpu_pkg = gpu_mem;
	command_element.stream = stream;
	/*
	 * only append the fw cmd to queue when its response needs to be waited for,
	 * currently there are only two such commands, disable channel and stop stream
	 * which are only sent after close camera
	 */
	if (wq && wq_cond) {
		cmd_ele = isp4if_append_cmd_2_cmdq(ispif, &command_element);
		if (!cmd_ele) {
			dev_err(dev, "fail for isp_append_cmd_2_cmdq\n");
			return -ENOMEM;
		}
	}

	ret = isp4if_insert_isp_fw_cmd(ispif, stream, &cmd);
	if (ret) {
		dev_err(dev, "fail for insert_isp_fw_cmd camId %s(0x%08x)\n",
			isp4dbg_get_cmd_str(cmd_id), cmd_id);
		if (cmd_ele) {
			isp4if_rm_cmd_from_cmdq(ispif, cmd_ele->seq_num,
						cmd_ele->cmd_id);
			kfree(cmd_ele);
		}
	}

	return ret;
}

static int isp4if_send_buffer(struct isp4_interface *ispif, struct isp4if_img_buf_info *buf_info)
{
	struct isp4fw_cmd_send_buffer cmd = {};

	cmd.buffer_type = BUFFER_TYPE_PREVIEW;
	cmd.buffer.vmid_space.bit.vmid = 0;
	cmd.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp4if_split_addr64(buf_info->planes[0].mc_addr,
			    &cmd.buffer.buf_base_a_lo,
			    &cmd.buffer.buf_base_a_hi);
	cmd.buffer.buf_size_a = buf_info->planes[0].len;

	isp4if_split_addr64(buf_info->planes[1].mc_addr,
			    &cmd.buffer.buf_base_b_lo,
			    &cmd.buffer.buf_base_b_hi);
	cmd.buffer.buf_size_b = buf_info->planes[1].len;

	isp4if_split_addr64(buf_info->planes[2].mc_addr,
			    &cmd.buffer.buf_base_c_lo,
			    &cmd.buffer.buf_base_c_hi);
	cmd.buffer.buf_size_c = buf_info->planes[2].len;

	return isp4if_send_fw_cmd(ispif, CMD_ID_SEND_BUFFER, &cmd,
				  sizeof(cmd), NULL, NULL, NULL);
}

static void isp4if_init_rb_config(struct isp4_interface *ispif, struct isp4if_rb_config *rb_config)
{
	u32 lo;
	u32 hi;

	isp4if_split_addr64(rb_config->base_mc_addr, &lo, &hi);

	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
		    rb_config->reg_rptr, 0x0);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
		    rb_config->reg_wptr, 0x0);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
		    rb_config->reg_base_lo, lo);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
		    rb_config->reg_base_hi, hi);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
		    rb_config->reg_size, rb_config->val_size);
}

static int isp4if_fw_init(struct isp4_interface *ispif)
{
	struct isp4if_rb_config *rb_config;
	u32 offset;
	int i;

	/* initialize CMD_RB streams */
	for (i = 0; i < ISP4IF_STREAM_ID_MAX; i++) {
		rb_config = (isp4if_cmd_rb_config + i);
		offset = ispif->aligned_rb_chunk_size *
			 (rb_config->index + ispif->cmd_rb_base_index);

		rb_config->val_size = ISP4IF_FW_CMD_BUF_SIZE;
		rb_config->base_sys_addr =
			(u8 *)ispif->fw_cmd_resp_buf->sys_addr + offset;
		rb_config->base_mc_addr =
			ispif->fw_cmd_resp_buf->gpu_mc_addr + offset;

		isp4if_init_rb_config(ispif, rb_config);
	}

	/* initialize RESP_RB streams */
	for (i = 0; i < ISP4IF_STREAM_ID_MAX; i++) {
		rb_config = (isp4if_resp_rb_config + i);
		offset = ispif->aligned_rb_chunk_size *
			 (rb_config->index + ispif->resp_rb_base_index);

		rb_config->val_size = ISP4IF_FW_CMD_BUF_SIZE;
		rb_config->base_sys_addr =
			(u8 *)ispif->fw_cmd_resp_buf->sys_addr + offset;
		rb_config->base_mc_addr =
			ispif->fw_cmd_resp_buf->gpu_mc_addr + offset;

		isp4if_init_rb_config(ispif, rb_config);
	}

	/* initialize LOG_RB stream */
	rb_config = &isp4if_log_rb_config;
	rb_config->val_size = ISP4IF_FW_LOG_RINGBUF_SIZE;
	rb_config->base_mc_addr = ispif->fw_log_buf->gpu_mc_addr;
	rb_config->base_sys_addr = ispif->fw_log_buf->sys_addr;

	isp4if_init_rb_config(ispif, rb_config);

	return 0;
}

static int isp4if_wait_fw_ready(struct isp4_interface *ispif, u32 isp_status_addr)
{
	struct device *dev = ispif->dev;
	u32 timeout_ms = 100;
	u32 interval_ms = 1;
	u32 reg_val;

	/* wait for FW initialize done! */
	if (!read_poll_timeout(isp4hw_rreg, reg_val, reg_val & ISP_STATUS__CCPU_REPORT_MASK,
			       interval_ms * 1000, timeout_ms * 1000, false,
			       GET_ISP4IF_REG_BASE(ispif), isp_status_addr))
		return 0;

	dev_err(dev, "ISP CCPU FW boot failed\n");

	return -ETIME;
}

static void isp4if_enable_ccpu(struct isp4_interface *ispif)
{
	u32 reg_val;

	reg_val = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), ISP_SOFT_RESET);
	reg_val &= (~ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_SOFT_RESET, reg_val);

	usleep_range(100, 150);

	reg_val = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), ISP_CCPU_CNTL);
	reg_val &= (~ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK);
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_CCPU_CNTL, reg_val);
}

static void isp4if_disable_ccpu(struct isp4_interface *ispif)
{
	u32 reg_val;

	reg_val = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), ISP_CCPU_CNTL);
	reg_val |= ISP_CCPU_CNTL__CCPU_HOST_SOFT_RST_MASK;
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_CCPU_CNTL, reg_val);

	usleep_range(100, 150);

	reg_val = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), ISP_SOFT_RESET);
	reg_val |= ISP_SOFT_RESET__CCPU_SOFT_RESET_MASK;
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_SOFT_RESET, reg_val);
}

static int isp4if_fw_boot(struct isp4_interface *ispif)
{
	struct device *dev = ispif->dev;

	if (ispif->status != ISP4IF_STATUS_PWR_ON) {
		dev_err(dev, "invalid isp power status %d\n", ispif->status);
		return -EINVAL;
	}

	isp4if_disable_ccpu(ispif);

	isp4if_fw_init(ispif);

	/* clear ccpu status */
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_STATUS, 0x0);

	isp4if_enable_ccpu(ispif);

	if (isp4if_wait_fw_ready(ispif, ISP_STATUS)) {
		isp4if_disable_ccpu(ispif);
		return -EINVAL;
	}

	/* enable interrupts */
	isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif), ISP_SYS_INT0_EN,
		    ISP4IF_FW_RESP_RB_IRQ_EN_MASK);

	ispif->status = ISP4IF_STATUS_FW_RUNNING;

	dev_dbg(dev, "ISP CCPU FW boot success\n");

	return 0;
}

int isp4if_f2h_resp(struct isp4_interface *ispif, enum isp4if_stream_id stream, void *resp)
{
	struct isp4fw_resp *response = resp;
	struct isp4if_rb_config *rb_config;
	struct device *dev = ispif->dev;
	u32 rd_ptr_dbg;
	u32 wr_ptr_dbg;
	void *mem_sys;
	u64 mem_addr;
	u32 checksum;
	u32 rd_ptr;
	u32 wr_ptr;
	u32 rreg;
	u32 wreg;
	u32 len;

	rb_config = &isp4if_resp_rb_config[stream];
	rreg = rb_config->reg_rptr;
	wreg = rb_config->reg_wptr;
	mem_sys = rb_config->base_sys_addr;
	mem_addr = rb_config->base_mc_addr;
	len = rb_config->val_size;

	rd_ptr = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), rreg);
	wr_ptr = isp4hw_rreg(GET_ISP4IF_REG_BASE(ispif), wreg);
	rd_ptr_dbg = rd_ptr;
	wr_ptr_dbg = wr_ptr;

	if (rd_ptr > len) {
		dev_err(dev, "fail %s(%u),rd_ptr %u(should<=%u),wr_ptr %u\n",
			isp4dbg_get_if_stream_str(stream),
			stream, rd_ptr, len, wr_ptr);
		return -EINVAL;
	}

	if (wr_ptr > len) {
		dev_err(dev, "fail %s(%u),wr_ptr %u(should<=%u), rd_ptr %u\n",
			isp4dbg_get_if_stream_str(stream),
			stream, wr_ptr, len, rd_ptr);
		return -EINVAL;
	}

	if (rd_ptr < wr_ptr) {
		if ((wr_ptr - rd_ptr) >= (sizeof(struct isp4fw_resp))) {
			memcpy((u8 *)response, (u8 *)mem_sys + rd_ptr,
			       sizeof(struct isp4fw_resp));

			rd_ptr += sizeof(struct isp4fw_resp);
			if (rd_ptr < len) {
				isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
					    rreg, rd_ptr);
			} else {
				dev_err(dev, "%s(%u),rd %u(should<=%u),wr %u\n",
					isp4dbg_get_if_stream_str(stream),
					stream, rd_ptr, len, wr_ptr);
				return -EINVAL;
			}

		} else {
			dev_err(dev, "sth wrong with wptr and rptr\n");
			return -EINVAL;
		}
	} else if (rd_ptr > wr_ptr) {
		u32 size;
		u8 *dst;

		dst = (u8 *)response;

		size = len - rd_ptr;
		if (size > sizeof(struct isp4fw_resp)) {
			mem_addr += rd_ptr;
			memcpy((u8 *)response,
			       (u8 *)(mem_sys) + rd_ptr,
			       sizeof(struct isp4fw_resp));
			rd_ptr += sizeof(struct isp4fw_resp);
			if (rd_ptr < len) {
				isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
					    rreg, rd_ptr);
			} else {
				dev_err(dev, "%s(%u),rd %u(should<=%u),wr %u\n",
					isp4dbg_get_if_stream_str(stream),
					stream, rd_ptr, len, wr_ptr);
				return -EINVAL;
			}

		} else {
			if ((size + wr_ptr) < (sizeof(struct isp4fw_resp))) {
				dev_err(dev, "sth wrong with wptr and rptr1\n");
				return -EINVAL;
			}

			memcpy(dst, (u8 *)(mem_sys) + rd_ptr, size);

			dst += size;
			size = sizeof(struct isp4fw_resp) - size;
			if (size)
				memcpy(dst, (u8 *)(mem_sys), size);
			rd_ptr = size;
			if (rd_ptr < len) {
				isp4hw_wreg(GET_ISP4IF_REG_BASE(ispif),
					    rreg, rd_ptr);
			} else {
				dev_err(dev, "%s(%u),rd %u(should<=%u),wr %u\n",
					isp4dbg_get_if_stream_str(stream),
					stream, rd_ptr, len, wr_ptr);
				return -EINVAL;
			}
		}
	} else {
		return -ETIME;
	}

	checksum = isp4if_compute_check_sum((u8 *)response, sizeof(struct isp4fw_resp) - 4);

	if (checksum != response->resp_check_sum) {
		dev_err(dev, "resp checksum 0x%x,should 0x%x,rptr %u,wptr %u\n",
			checksum, response->resp_check_sum, rd_ptr_dbg, wr_ptr_dbg);

		dev_err(dev, "%s(%u), seqNo %u, resp_id %s(0x%x)\n",
			isp4dbg_get_if_stream_str(stream), stream, response->resp_seq_num,
			isp4dbg_get_resp_str(response->resp_id), response->resp_id);

		return -EINVAL;
	}

	return 0;
}

int isp4if_send_command(struct isp4_interface *ispif, u32 cmd_id, void *package, u32 package_size)
{
	return isp4if_send_fw_cmd(ispif, cmd_id, package, package_size, NULL, NULL, NULL);
}

int isp4if_send_command_sync(struct isp4_interface *ispif, u32 cmd_id, void *package,
			     u32 package_size, u32 timeout)
{
	struct device *dev = ispif->dev;
	DECLARE_WAIT_QUEUE_HEAD(cmd_wq);
	u32 wq_cond = 0;
	int ret;
	u32 seq;

	ret = isp4if_send_fw_cmd(ispif, cmd_id, package, package_size, &cmd_wq, &wq_cond, &seq);

	if (ret) {
		dev_err(dev, "send fw cmd fail %d\n", ret);
		return ret;
	}

	ret = wait_event_timeout(cmd_wq, wq_cond != 0, msecs_to_jiffies(timeout));
	if (ret == 0) {
		struct isp4if_cmd_element *ele;

		ele = isp4if_rm_cmd_from_cmdq(ispif, seq, cmd_id);
		kfree(ele);
		return -ETIMEDOUT;
	}

	return 0;
}

void isp4if_clear_bufq(struct isp4_interface *ispif)
{
	struct isp4if_img_buf_node *buf_node = NULL;
	struct isp4if_img_buf_node *tmp_node = NULL;

	guard(mutex)(&ispif->bufq_mutex);

	list_for_each_entry_safe(buf_node, tmp_node, &ispif->bufq, node) {
		list_del(&buf_node->node);
		kfree(buf_node);
	}
}

void isp4if_dealloc_buffer_node(struct isp4if_img_buf_node *buf_node)
{
	kfree(buf_node);
}

struct isp4if_img_buf_node *isp4if_alloc_buffer_node(struct isp4if_img_buf_info *buf_info)
{
	struct isp4if_img_buf_node *node = NULL;

	node = kmalloc(sizeof(*node), GFP_KERNEL);
	if (node)
		node->buf_info = *buf_info;

	return node;
};

struct isp4if_img_buf_node *isp4if_dequeue_buffer(struct isp4_interface *ispif)
{
	struct isp4if_img_buf_node *buf_node = NULL;

	guard(mutex)(&ispif->bufq_mutex);

	buf_node = list_first_entry_or_null(&ispif->bufq, typeof(*buf_node), node);
	if (buf_node)
		list_del(&buf_node->node);

	return buf_node;
}

int isp4if_queue_buffer(struct isp4_interface *ispif, struct isp4if_img_buf_node *buf_node)
{
	int ret;

	ret = isp4if_send_buffer(ispif, &buf_node->buf_info);
	if (ret)
		return ret;

	guard(mutex)(&ispif->bufq_mutex);

	list_add_tail(&buf_node->node, &ispif->bufq);

	return 0;
}

int isp4if_stop(struct isp4_interface *ispif)
{
	isp4if_disable_ccpu(ispif);

	isp4if_dealloc_fw_gpumem(ispif);

	return 0;
}

int isp4if_start(struct isp4_interface *ispif)
{
	int ret;

	ret = isp4if_alloc_fw_gpumem(ispif);
	if (ret)
		return -ENOMEM;

	ret = isp4if_fw_boot(ispif);
	if (ret)
		goto failed_fw_boot;

	return 0;

failed_fw_boot:
	isp4if_dealloc_fw_gpumem(ispif);
	return ret;
}

int isp4if_deinit(struct isp4_interface *ispif)
{
	isp4if_clear_cmdq(ispif);

	isp4if_clear_bufq(ispif);

	mutex_destroy(&ispif->cmdq_mutex);
	mutex_destroy(&ispif->bufq_mutex);
	mutex_destroy(&ispif->isp4if_mutex);

	return 0;
}

int isp4if_init(struct isp4_interface *ispif, struct device *dev, void __iomem *isp_mmip)
{
	ispif->dev = dev;
	ispif->mmio = isp_mmip;

	ispif->cmd_rb_base_index = 0;
	ispif->resp_rb_base_index = ISP4IF_RESP_CHAN_TO_RB_OFFSET - 1;
	ispif->aligned_rb_chunk_size = ISP4IF_RB_PMBMAP_MEM_CHUNK & 0xffffffc0;

	mutex_init(&ispif->cmdq_mutex); /* used for cmdq access */
	mutex_init(&ispif->bufq_mutex); /* used for bufq access */
	mutex_init(&ispif->isp4if_mutex); /* used for commands sent to ispfw */

	INIT_LIST_HEAD(&ispif->cmdq);
	INIT_LIST_HEAD(&ispif->bufq);

	return 0;
}
