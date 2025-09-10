// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#include <linux/mutex.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>

#include "isp4_fw_cmd_resp.h"
#include "isp4_interface.h"
#include "isp4_subdev.h"
#include <linux/units.h>

#define ISP4SD_MAX_CMD_RESP_BUF_SIZE (4 * 1024)
#define ISP4SD_MIN_BUF_CNT_BEF_START_STREAM 4

#define ISP4SD_PERFORMANCE_STATE_LOW 0
#define ISP4SD_PERFORMANCE_STATE_HIGH 1

#define ISP4SD_FW_CMD_TIMEOUT_IN_MS  500
#define ISP4SD_WAIT_RESP_IRQ_TIMEOUT  5 /* ms */
/* align 32KB */
#define ISP4SD_META_BUF_SIZE ALIGN(sizeof(struct isp4fw_meta_info), 0x8000)

#define to_isp4_subdev(v4l2_sdev)  \
	container_of(v4l2_sdev, struct isp4_subdev, sdev)

static const char *isp4sd_entity_name = "amd isp4";

static void isp4sd_module_enable(struct isp4_subdev *isp_subdev, bool enable)
{
	if (isp_subdev->enable_gpio) {
		gpiod_set_value(isp_subdev->enable_gpio, enable ? 1 : 0);
		dev_dbg(isp_subdev->dev, "%s isp_subdev module\n",
			enable ? "enable" : "disable");
	}
}

static int isp4sd_setup_fw_mem_pool(struct isp4_subdev *isp_subdev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4fw_cmd_send_buffer buf_type = {};
	struct device *dev = isp_subdev->dev;
	int ret;

	if (!ispif->fw_mem_pool) {
		dev_err(dev, "fail to alloc mem pool\n");
		return -ENOMEM;
	}

	buf_type.buffer_type = BUFFER_TYPE_MEM_POOL;
	buf_type.buffer.buf_tags = 0;
	buf_type.buffer.vmid_space.bit.vmid = 0;
	buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
	isp4if_split_addr64(ispif->fw_mem_pool->gpu_mc_addr,
			    &buf_type.buffer.buf_base_a_lo,
			    &buf_type.buffer.buf_base_a_hi);
	buf_type.buffer.buf_size_a = (u32)ispif->fw_mem_pool->mem_size;

	ret = isp4if_send_command(ispif, CMD_ID_SEND_BUFFER,
				  &buf_type, sizeof(buf_type));
	if (ret) {
		dev_err(dev, "send fw mem pool 0x%llx(%u) fail %d\n",
			ispif->fw_mem_pool->gpu_mc_addr,
			buf_type.buffer.buf_size_a,
			ret);
		return ret;
	}

	dev_dbg(dev, "send fw mem pool 0x%llx(%u) suc\n",
		ispif->fw_mem_pool->gpu_mc_addr,
		buf_type.buffer.buf_size_a);

	return 0;
};

static int isp4sd_set_stream_path(struct isp4_subdev *isp_subdev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4fw_cmd_set_stream_cfg cmd = {};
	struct device *dev = isp_subdev->dev;

	cmd.stream_cfg.mipi_pipe_path_cfg.isp4fw_sensor_id = SENSOR_ID_ON_MIPI0;
	cmd.stream_cfg.mipi_pipe_path_cfg.b_enable = true;
	cmd.stream_cfg.isp_pipe_path_cfg.isp_pipe_id = MIPI0_ISP_PIPELINE_ID;

	cmd.stream_cfg.b_enable_tnr = true;
	dev_dbg(dev, "isp4fw_sensor_id %d, pipeId 0x%x EnableTnr %u\n",
		cmd.stream_cfg.mipi_pipe_path_cfg.isp4fw_sensor_id,
		cmd.stream_cfg.isp_pipe_path_cfg.isp_pipe_id,
		cmd.stream_cfg.b_enable_tnr);

	return isp4if_send_command(ispif, CMD_ID_SET_STREAM_CONFIG,
				   &cmd, sizeof(cmd));
}

static int isp4sd_send_meta_buf(struct isp4_subdev *isp_subdev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4fw_cmd_send_buffer buf_type = {};
	struct isp4sd_sensor_info *sensor_info;
	struct device *dev = isp_subdev->dev;
	u32 i;

	sensor_info = &isp_subdev->sensor_info;
	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++) {
		int ret;

		if (!sensor_info->meta_info_buf[i]) {
			dev_err(dev, "fail for no meta info buf(%u)\n", i);
			return -ENOMEM;
		}
		buf_type.buffer_type = BUFFER_TYPE_META_INFO;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp4if_split_addr64(sensor_info->meta_info_buf[i]->gpu_mc_addr,
				    &buf_type.buffer.buf_base_a_lo,
				    &buf_type.buffer.buf_base_a_hi);
		buf_type.buffer.buf_size_a =
			(u32)sensor_info->meta_info_buf[i]->mem_size;
		ret = isp4if_send_command(ispif, CMD_ID_SEND_BUFFER,
					  &buf_type,
					  sizeof(buf_type));
		if (ret) {
			dev_err(dev, "send meta info(%u) fail\n", i);
			return ret;
		}
	}

	dev_dbg(dev, "send meta info suc\n");
	return 0;
}

static bool isp4sd_get_str_out_prop(struct isp4_subdev *isp_subdev,
				    struct isp4fw_image_prop *out_prop,
				    struct v4l2_subdev_state *state, u32 pad)
{
	struct v4l2_mbus_framefmt *format = NULL;
	struct device *dev = isp_subdev->dev;
	bool ret;

	format = v4l2_subdev_state_get_format(state, pad, 0);
	if (!format) {
		dev_err(dev, "fail get subdev state format\n");
		return false;
	}

	switch (format->code) {
	case MEDIA_BUS_FMT_YUYV8_1_5X8:
		out_prop->image_format = IMAGE_FORMAT_NV12;
		out_prop->width = format->width;
		out_prop->height = format->height;
		out_prop->luma_pitch = format->width;
		out_prop->chroma_pitch = out_prop->width;
		ret = true;
		break;
	case MEDIA_BUS_FMT_YUYV8_1X16:
		out_prop->image_format = IMAGE_FORMAT_YUV422INTERLEAVED;
		out_prop->width = format->width;
		out_prop->height = format->height;
		out_prop->luma_pitch = format->width * 2;
		out_prop->chroma_pitch = 0;
		ret = true;
		break;
	default:
		dev_err(dev, "fail for bad image format:0x%x\n",
			format->code);
		ret = false;
		break;
	}

	if (!out_prop->width || !out_prop->height)
		ret = false;
	return ret;
}

static int isp4sd_kickoff_stream(struct isp4_subdev *isp_subdev, u32 w, u32 h)
{
	struct isp4sd_sensor_info *sensor_info = &isp_subdev->sensor_info;
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;

	if (sensor_info->status == ISP4SD_START_STATUS_STARTED) {
		return 0;
	} else if (sensor_info->status == ISP4SD_START_STATUS_START_FAIL) {
		dev_err(dev, "fail for previous start fail\n");
		return -EINVAL;
	}

	dev_dbg(dev, "w:%u,h:%u\n", w, h);

	sensor_info->status = ISP4SD_START_STATUS_START_FAIL;

	if (isp4sd_send_meta_buf(isp_subdev)) {
		dev_err(dev, "fail to send meta buf\n");
		return -EINVAL;
	};

	sensor_info->status = ISP4SD_START_STATUS_NOT_START;

	if (!sensor_info->start_stream_cmd_sent &&
	    sensor_info->buf_sent_cnt >=
	    ISP4SD_MIN_BUF_CNT_BEF_START_STREAM) {
		int ret = isp4if_send_command(ispif, CMD_ID_START_STREAM,
					      NULL, 0);
		if (ret) {
			dev_err(dev, "fail to start stream\n");
			return ret;
		}

		sensor_info->start_stream_cmd_sent = true;
	} else {
		dev_dbg(dev,
			"no send START_STREAM, start_sent %u, buf_sent %u\n",
			sensor_info->start_stream_cmd_sent,
			sensor_info->buf_sent_cnt);
	}

	return 0;
}

static int isp4sd_setup_output(struct isp4_subdev *isp_subdev,
			       struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4sd_sensor_info *sensor_info = &isp_subdev->sensor_info;
	struct isp4sd_output_info *output_info =
			&isp_subdev->sensor_info.output_info;
	struct isp4fw_cmd_set_out_ch_prop cmd_ch_prop = {};
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4fw_cmd_enable_out_ch cmd_ch_en = {};
	struct device *dev = isp_subdev->dev;
	struct isp4fw_image_prop *out_prop;
	int ret;

	if (output_info->start_status == ISP4SD_START_STATUS_STARTED)
		return 0;

	if (output_info->start_status == ISP4SD_START_STATUS_START_FAIL) {
		dev_err(dev, "fail for previous start fail\n");
		return -EINVAL;
	}

	out_prop = &cmd_ch_prop.image_prop;
	cmd_ch_prop.ch = ISP_PIPE_OUT_CH_PREVIEW;
	cmd_ch_en.ch = ISP_PIPE_OUT_CH_PREVIEW;
	cmd_ch_en.is_enable = true;

	if (!isp4sd_get_str_out_prop(isp_subdev, out_prop, state, pad)) {
		dev_err(dev, "fail to get out prop\n");
		return -EINVAL;
	}

	dev_dbg(dev, "channel: w:h=%u:%u,lp:%u,cp%u\n",
		cmd_ch_prop.image_prop.width, cmd_ch_prop.image_prop.height,
		cmd_ch_prop.image_prop.luma_pitch,
		cmd_ch_prop.image_prop.chroma_pitch);

	ret = isp4if_send_command(ispif, CMD_ID_SET_OUT_CHAN_PROP,
				  &cmd_ch_prop,
				  sizeof(cmd_ch_prop));
	if (ret) {
		output_info->start_status = ISP4SD_START_STATUS_START_FAIL;
		dev_err(dev, "fail to set out prop\n");
		return ret;
	};

	ret = isp4if_send_command(ispif, CMD_ID_ENABLE_OUT_CHAN,
				  &cmd_ch_en, sizeof(cmd_ch_en));

	if (ret) {
		output_info->start_status = ISP4SD_START_STATUS_START_FAIL;
		dev_err(dev, "fail to enable channel\n");
		return ret;
	}

	if (!sensor_info->start_stream_cmd_sent) {
		ret = isp4sd_kickoff_stream(isp_subdev, out_prop->width,
					    out_prop->height);
		if (ret) {
			dev_err(dev, "kickoff stream fail %d\n", ret);
			return ret;
		}
		/*
		 * sensor_info->start_stream_cmd_sent will be set to true
		 * 1. in isp4sd_kickoff_stream, if app first send buffer then
		 * start stream
		 * 2. in isp_set_stream_buf, if app first start stream, then
		 * send buffer
		 * because ISP FW has the requirement, host needs to send buffer
		 * before send start stream cmd
		 */
		if (sensor_info->start_stream_cmd_sent) {
			sensor_info->status = ISP4SD_START_STATUS_STARTED;
			output_info->start_status = ISP4SD_START_STATUS_STARTED;
			dev_dbg(dev, "kickoff stream suc,start cmd sent\n");
		}
	} else {
		dev_dbg(dev, "stream running, no need kickoff\n");
		output_info->start_status = ISP4SD_START_STATUS_STARTED;
	}

	dev_dbg(dev, "setup output suc\n");
	return 0;
}

static int isp4sd_init_meta_buf(struct isp4_subdev *isp_subdev)
{
	struct isp4sd_sensor_info *sensor_info = &isp_subdev->sensor_info;
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	u32 i;

	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++) {
		if (!sensor_info->meta_info_buf[i]) {
			sensor_info->meta_info_buf[i] = ispif->metainfo_buf_pool[i];
			if (!sensor_info->meta_info_buf[i]) {
				dev_err(dev, "invalid %u meta_info_buf fail\n", i);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static int isp4sd_init_stream(struct isp4_subdev *isp_subdev)
{
	struct device *dev = isp_subdev->dev;
	int ret;

	ret  = isp4sd_setup_fw_mem_pool(isp_subdev);
	if (ret) {
		dev_err(dev, "fail to  setup fw mem pool\n");
		return ret;
	}

	ret  = isp4sd_init_meta_buf(isp_subdev);
	if (ret) {
		dev_err(dev, "fail to alloc fw driver shared buf\n");
		return ret;
	}

	ret = isp4sd_set_stream_path(isp_subdev);
	if (ret) {
		dev_err(dev, "fail to setup stream path\n");
		return ret;
	}

	return 0;
}

static void isp4sd_reset_stream_info(struct isp4_subdev *isp_subdev,
				     struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4sd_sensor_info *sensor_info = &isp_subdev->sensor_info;
	struct v4l2_mbus_framefmt *format = NULL;
	struct isp4sd_output_info *str_info;
	int i;

	format = v4l2_subdev_state_get_format(state, pad, 0);

	if (!format) {
		dev_err(isp_subdev->dev, "fail to setup stream path\n");
	} else {
		memset(format, 0, sizeof(*format));
		format->code = MEDIA_BUS_FMT_YUYV8_1_5X8;
	}

	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++)
		sensor_info->meta_info_buf[i] = NULL;

	str_info = &sensor_info->output_info;
	str_info->start_status = ISP4SD_START_STATUS_NOT_START;
}

static bool isp4sd_is_stream_running(struct isp4_subdev *isp_subdev)
{
	struct isp4sd_sensor_info *sif;
	enum isp4sd_start_status stat;

	sif = &isp_subdev->sensor_info;
	stat = sif->output_info.start_status;
	if (stat == ISP4SD_START_STATUS_STARTED)
		return true;

	return false;
}

static void isp4sd_reset_camera_info(struct isp4_subdev *isp_subdev,
				     struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4sd_sensor_info *info  = &isp_subdev->sensor_info;

	info->status = ISP4SD_START_STATUS_NOT_START;
	isp4sd_reset_stream_info(isp_subdev, state, pad);

	info->start_stream_cmd_sent = false;
}

static int isp4sd_uninit_stream(struct isp4_subdev *isp_subdev,
				struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	bool running;

	running = isp4sd_is_stream_running(isp_subdev);

	if (running) {
		dev_dbg(dev, "fail for stream is still running\n");
		return -EINVAL;
	}

	isp4sd_reset_camera_info(isp_subdev, state, pad);

	isp4if_clear_cmdq(ispif);
	return 0;
}

static void isp4sd_fw_resp_cmd_done(struct isp4_subdev *isp_subdev,
				    enum isp4if_stream_id stream_id,
				    struct isp4fw_resp_cmd_done *para)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4if_cmd_element *ele =
		isp4if_rm_cmd_from_cmdq(ispif, para->cmd_seq_num, para->cmd_id);
	struct device *dev = isp_subdev->dev;

	dev_dbg(dev, "stream %d,cmd (0x%08x)(%d),seq %u, ele %p\n",
		stream_id,
		para->cmd_id, para->cmd_status, para->cmd_seq_num,
		ele);

	if (!ele)
		return;

	if (ele->wq) {
		dev_dbg(dev, "signal event %p\n", ele->wq);
		if (ele->wq_cond)
			*ele->wq_cond = 1;
		wake_up(ele->wq);
	}

	kfree(ele);
}

static struct isp4fw_meta_info *
isp4sd_get_meta_by_mc(struct isp4_subdev *isp_subdev,
		      u64 mc)
{
	u32 i;

	for (i = 0; i < ISP4IF_MAX_STREAM_BUF_COUNT; i++) {
		struct isp4if_gpu_mem_info *meta_info_buf =
				isp_subdev->sensor_info.meta_info_buf[i];

		if (meta_info_buf) {
			if (mc == meta_info_buf->gpu_mc_addr)
				return meta_info_buf->sys_addr;
		}
	}
	return NULL;
};

static struct isp4if_img_buf_node *
isp4sd_preview_done(struct isp4_subdev *isp_subdev,
		    struct isp4fw_meta_info *meta,
		    struct isp4vid_framedone_param *pcb)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4if_img_buf_node *prev = NULL;
	struct device *dev = isp_subdev->dev;

	pcb->preview.status = ISP4VID_BUF_DONE_STATUS_ABSENT;
	if (meta->preview.enabled &&
	    (meta->preview.status == BUFFER_STATUS_SKIPPED ||
	     meta->preview.status == BUFFER_STATUS_DONE ||
	     meta->preview.status == BUFFER_STATUS_DIRTY)) {
		prev = isp4if_dequeue_buffer(ispif);
		if (!prev) {
			dev_err(dev, "fail null prev buf\n");
		} else {
			pcb->preview.buf = prev->buf_info;
			pcb->preview.status = ISP4VID_BUF_DONE_STATUS_SUCCESS;
		}
	} else if (meta->preview.enabled) {
		dev_err(dev, "fail bad preview status %u\n",
			meta->preview.status);
	}

	return prev;
}

static void isp4sd_send_meta_info(struct isp4_subdev *isp_subdev,
				  u64 meta_info_mc)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4fw_cmd_send_buffer buf_type = {};
	struct device *dev = isp_subdev->dev;

	if (isp_subdev->sensor_info.status != ISP4SD_START_STATUS_STARTED) {
		dev_warn(dev, "not working status %i, meta_info 0x%llx\n",
			 isp_subdev->sensor_info.status, meta_info_mc);
		return;
	}

	if (meta_info_mc) {
		buf_type.buffer_type = BUFFER_TYPE_META_INFO;
		buf_type.buffer.buf_tags = 0;
		buf_type.buffer.vmid_space.bit.vmid = 0;
		buf_type.buffer.vmid_space.bit.space = ADDR_SPACE_TYPE_GPU_VA;
		isp4if_split_addr64(meta_info_mc,
				    &buf_type.buffer.buf_base_a_lo,
				    &buf_type.buffer.buf_base_a_hi);

		buf_type.buffer.buf_size_a = ISP4SD_META_BUF_SIZE;
		if (isp4if_send_command(ispif, CMD_ID_SEND_BUFFER,
					&buf_type, sizeof(buf_type))) {
			dev_err(dev, "fail send meta_info 0x%llx\n",
				meta_info_mc);
		} else {
			dev_dbg(dev, "resend meta_info 0x%llx\n", meta_info_mc);
		}
	}
}

static void isp4sd_fw_resp_frame_done(struct isp4_subdev *isp_subdev,
				      enum isp4if_stream_id stream_id,
				      struct isp4fw_resp_param_package *para)
{
	struct isp4vid_framedone_param pcb = {};
	struct isp4if_img_buf_node *prev = NULL;
	struct device *dev = isp_subdev->dev;
	struct isp4fw_meta_info *meta;
	u64 mc = 0;

	mc = isp4if_join_addr64(para->package_addr_lo, para->package_addr_hi);
	meta = isp4sd_get_meta_by_mc(isp_subdev, mc);
	if (mc == 0 || !meta) {
		dev_err(dev, "fail to get meta from mc %llx\n", mc);
		return;
	}

	pcb.poc = meta->poc;
	pcb.cam_id = 0;

	dev_dbg(dev, "ts:%llu,streamId:%d,poc:%u,preview_en:%u,(%i)\n",
		ktime_get_ns(), stream_id, meta->poc,
		meta->preview.enabled,
		meta->preview.status);

	prev = isp4sd_preview_done(isp_subdev, meta, &pcb);
	if (pcb.preview.status != ISP4VID_BUF_DONE_STATUS_ABSENT)
		isp4vid_notify(&isp_subdev->isp_vdev, &pcb);

	isp4if_dealloc_buffer_node(prev);

	if (isp_subdev->sensor_info.status == ISP4SD_START_STATUS_STARTED)
		isp4sd_send_meta_info(isp_subdev, mc);

	dev_dbg(dev, "stream_id:%d, status:%d\n", stream_id,
		isp_subdev->sensor_info.status);
}

static void isp4sd_fw_resp_func(struct isp4_subdev *isp_subdev,
				enum isp4if_stream_id stream_id)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	struct isp4fw_resp resp;

	if (ispif->status < ISP4IF_STATUS_FW_RUNNING)
		return;

	while (true) {
		s32 ret;

		ret = isp4if_f2h_resp(ispif, stream_id, &resp);
		if (ret)
			break;

		switch (resp.resp_id) {
		case RESP_ID_CMD_DONE:
			isp4sd_fw_resp_cmd_done(isp_subdev, stream_id,
						&resp.param.cmd_done);
			break;
		case RESP_ID_NOTI_FRAME_DONE:
			isp4sd_fw_resp_frame_done(isp_subdev, stream_id,
						  &resp.param.frame_done);
			break;
		default:
			dev_err(dev, "-><- fail respid (0x%x)\n",
				resp.resp_id);
			break;
		}
	}
}

static s32 isp4sd_fw_resp_thread_wrapper(void *context)
{
	struct isp4_subdev_thread_param *para = context;
	struct isp4sd_thread_handler *thread_ctx;
	enum isp4if_stream_id stream_id;

	struct isp4_subdev *isp_subdev;
	struct device *dev;
	u64 timeout;

	if (!para)
		return -EINVAL;

	isp_subdev = para->isp_subdev;
	dev = isp_subdev->dev;

	switch (para->idx) {
	case 0:
		stream_id = ISP4IF_STREAM_ID_GLOBAL;
		break;
	case 1:
		stream_id = ISP4IF_STREAM_ID_1;
		break;
	default:
		dev_err(dev, "fail invalid %d\n", para->idx);
		return -EINVAL;
	}

	thread_ctx = &isp_subdev->fw_resp_thread[para->idx];

	thread_ctx->wq_cond = 0;
	mutex_init(&thread_ctx->mutex);
	init_waitqueue_head(&thread_ctx->waitq);
	timeout = msecs_to_jiffies(ISP4SD_WAIT_RESP_IRQ_TIMEOUT);

	dev_dbg(dev, "[%u] started\n", para->idx);

	while (true) {
		wait_event_interruptible_timeout(thread_ctx->waitq,
						 thread_ctx->wq_cond != 0,
						 timeout);
		thread_ctx->wq_cond = 0;

		if (kthread_should_stop()) {
			dev_dbg(dev, "[%u] quit\n", para->idx);
			break;
		}

		mutex_lock(&thread_ctx->mutex);
		isp4sd_fw_resp_func(isp_subdev, stream_id);
		mutex_unlock(&thread_ctx->mutex);
	}

	mutex_destroy(&thread_ctx->mutex);

	return 0;
}

static int isp4sd_start_resp_proc_threads(struct isp4_subdev *isp_subdev)
{
	struct device *dev = isp_subdev->dev;
	int i;

	for (i = 0; i < ISP4SD_MAX_FW_RESP_STREAM_NUM; i++) {
		struct isp4sd_thread_handler *thread_ctx =
				&isp_subdev->fw_resp_thread[i];

		isp_subdev->isp_resp_para[i].idx = i;
		isp_subdev->isp_resp_para[i].isp_subdev = isp_subdev;

		thread_ctx->thread = kthread_run(isp4sd_fw_resp_thread_wrapper,
						 &isp_subdev->isp_resp_para[i],
						 "amd_isp4_thread");
		if (IS_ERR(thread_ctx->thread)) {
			dev_err(dev, "create thread [%d] fail\n", i);
			return -EINVAL;
		}
	}

	return 0;
}

static int isp4sd_stop_resp_proc_threads(struct isp4_subdev *isp_subdev)
{
	int i;

	for (i = 0; i < ISP4SD_MAX_FW_RESP_STREAM_NUM; i++) {
		struct isp4sd_thread_handler *thread_ctx =
				&isp_subdev->fw_resp_thread[i];

		if (thread_ctx->thread) {
			kthread_stop(thread_ctx->thread);
			thread_ctx->thread = NULL;
		}
	}

	return 0;
}

static u32 isp4sd_get_started_stream_count(struct isp4_subdev *isp_subdev)
{
	u32 cnt = 0;

	if (isp_subdev->sensor_info.status == ISP4SD_START_STATUS_STARTED)
		cnt++;
	return cnt;
}

static int isp4sd_pwroff_and_deinit(struct isp4_subdev *isp_subdev)
{
	struct isp4sd_sensor_info *sensor_info = &isp_subdev->sensor_info;
	unsigned int perf_state = ISP4SD_PERFORMANCE_STATE_LOW;
	struct isp4_interface *ispif = &isp_subdev->ispif;

	struct device *dev = isp_subdev->dev;
	u32 cnt;
	int ret;

	mutex_lock(&isp_subdev->ops_mutex);

	if (sensor_info->status == ISP4SD_START_STATUS_STARTED) {
		dev_err(dev, "fail for stream still running\n");
		mutex_unlock(&isp_subdev->ops_mutex);
		return -EINVAL;
	}

	sensor_info->status = ISP4SD_START_STATUS_NOT_START;
	cnt = isp4sd_get_started_stream_count(isp_subdev);
	if (cnt > 0) {
		dev_dbg(dev, "no need power off isp_subdev\n");
		mutex_unlock(&isp_subdev->ops_mutex);
		return 0;
	}

	isp4if_stop(ispif);

	ret = dev_pm_genpd_set_performance_state(dev, perf_state);
	if (ret)
		dev_err(dev,
			"fail to set isp_subdev performance state %u,ret %d\n",
			perf_state, ret);
	isp4sd_stop_resp_proc_threads(isp_subdev);
	dev_dbg(dev, "isp_subdev stop resp proc streads suc");
	/* hold ccpu reset */
	isp4hw_wreg(isp_subdev->mmio, ISP_SOFT_RESET, 0x0);
	isp4hw_wreg(isp_subdev->mmio, ISP_POWER_STATUS, 0);
	ret = pm_runtime_put_sync(dev);
	if (ret)
		dev_err(dev, "power off isp_subdev fail %d\n", ret);
	else
		dev_dbg(dev, "power off isp_subdev suc\n");

	ispif->status = ISP4IF_STATUS_PWR_OFF;
	isp4if_clear_cmdq(ispif);
	isp4sd_module_enable(isp_subdev, false);

	msleep(20);

	mutex_unlock(&isp_subdev->ops_mutex);

	return 0;
}

static int isp4sd_pwron_and_init(struct isp4_subdev *isp_subdev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	int ret;

	if (ispif->status == ISP4IF_STATUS_FW_RUNNING) {
		dev_dbg(dev, "camera already opened, do nothing\n");
		return 0;
	}

	mutex_lock(&isp_subdev->ops_mutex);

	isp4sd_module_enable(isp_subdev, true);

	isp_subdev->sensor_info.start_stream_cmd_sent = false;
	isp_subdev->sensor_info.buf_sent_cnt = 0;

	if (ispif->status < ISP4IF_STATUS_PWR_ON) {
		unsigned int perf_state = ISP4SD_PERFORMANCE_STATE_HIGH;

		ret = pm_runtime_resume_and_get(dev);
		if (ret) {
			dev_err(dev, "fail to power on isp_subdev ret %d\n",
				ret);
			goto err_unlock_and_close;
		}

		/* ISPPG ISP Power Status */
		isp4hw_wreg(isp_subdev->mmio, ISP_POWER_STATUS, 0x7FF);
		ret = dev_pm_genpd_set_performance_state(dev, perf_state);
		if (ret) {
			dev_err(dev,
				"fail to set performance state %u, ret %d\n",
				perf_state, ret);
			goto err_unlock_and_close;
		}

		ispif->status = ISP4IF_STATUS_PWR_ON;

		if (isp4sd_start_resp_proc_threads(isp_subdev)) {
			dev_err(dev, "isp_start_resp_proc_threads fail");
			goto err_unlock_and_close;
		} else {
			dev_dbg(dev, "create resp threads ok");
		}
	}

	isp_subdev->sensor_info.start_stream_cmd_sent = false;
	isp_subdev->sensor_info.buf_sent_cnt = 0;

	ret = isp4if_start(ispif);
	if (ret) {
		dev_err(dev, "fail to start isp_subdev interface\n");
		goto err_unlock_and_close;
	}

	mutex_unlock(&isp_subdev->ops_mutex);
	return 0;
err_unlock_and_close:
	mutex_unlock(&isp_subdev->ops_mutex);
	isp4sd_pwroff_and_deinit(isp_subdev);
	return -EINVAL;
}

static int isp4sd_stop_stream(struct isp4_subdev *isp_subdev,
			      struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4sd_output_info *output_info =
			&isp_subdev->sensor_info.output_info;
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	int ret = 0;

	dev_dbg(dev, "status %i\n", output_info->start_status);
	mutex_lock(&isp_subdev->ops_mutex);

	if (output_info->start_status == ISP4SD_START_STATUS_STARTED) {
		struct isp4fw_cmd_enable_out_ch cmd_ch_disable;

		cmd_ch_disable.ch = ISP_PIPE_OUT_CH_PREVIEW;
		cmd_ch_disable.is_enable = false;
		ret = isp4if_send_command_sync(ispif,
					       CMD_ID_ENABLE_OUT_CHAN,
					       &cmd_ch_disable,
					       sizeof(cmd_ch_disable),
					       ISP4SD_FW_CMD_TIMEOUT_IN_MS);
		if (ret)
			dev_err(dev, "fail to disable stream\n");
		else
			dev_dbg(dev, "wait disable stream suc\n");

		ret = isp4if_send_command_sync(ispif, CMD_ID_STOP_STREAM,
					       NULL,
					       0,
					       ISP4SD_FW_CMD_TIMEOUT_IN_MS);
		if (ret)
			dev_err(dev, "fail to stop steam\n");
		else
			dev_dbg(dev, "wait stop stream suc\n");
	}

	isp4if_clear_bufq(ispif);

	output_info->start_status = ISP4SD_START_STATUS_NOT_START;
	isp4sd_reset_stream_info(isp_subdev, state, pad);

	mutex_unlock(&isp_subdev->ops_mutex);

	isp4sd_uninit_stream(isp_subdev, state, pad);

	return ret;
}

static int isp4sd_start_stream(struct isp4_subdev *isp_subdev,
			       struct v4l2_subdev_state *state, u32 pad)
{
	struct isp4sd_output_info *output_info =
			&isp_subdev->sensor_info.output_info;
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct device *dev = isp_subdev->dev;
	int ret;

	mutex_lock(&isp_subdev->ops_mutex);

	if (ispif->status != ISP4IF_STATUS_FW_RUNNING) {
		mutex_unlock(&isp_subdev->ops_mutex);
		dev_err(dev, "fail, bad fsm %d", ispif->status);
		return -EINVAL;
	}

	ret = isp4sd_init_stream(isp_subdev);

	if (ret) {
		dev_err(dev, "fail to init isp_subdev stream\n");
		ret = -EINVAL;
		goto unlock_and_check_ret;
	}

	if (output_info->start_status == ISP4SD_START_STATUS_STARTED) {
		ret = 0;
		dev_dbg(dev, "stream started, do nothing\n");
		goto unlock_and_check_ret;
	} else if (output_info->start_status ==
		   ISP4SD_START_STATUS_START_FAIL) {
		ret = -EINVAL;
		dev_err(dev, "stream  fail to start before\n");
		goto unlock_and_check_ret;
	}

	if (isp4sd_setup_output(isp_subdev, state, pad)) {
		dev_err(dev, "fail to setup output\n");
		ret = -EINVAL;
	} else {
		ret = 0;
		dev_dbg(dev, "suc to setup out\n");
	}

unlock_and_check_ret:
	mutex_unlock(&isp_subdev->ops_mutex);
	if (ret) {
		isp4sd_stop_stream(isp_subdev, state, pad);
		dev_err(dev, "start stream fail\n");
	}

	return ret;
}

static int isp4sd_ioc_send_img_buf(struct v4l2_subdev *sd,
				   struct isp4if_img_buf_info *buf_info)
{
	struct isp4_subdev *isp_subdev = to_isp4_subdev(sd);
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4if_img_buf_node *buf_node = NULL;
	struct device *dev = isp_subdev->dev;
	int ret = -EINVAL;

	mutex_lock(&isp_subdev->ops_mutex);
	/* TODO: remove isp_status */
	if (ispif->status != ISP4IF_STATUS_FW_RUNNING) {
		dev_err(dev, "fail send img buf for bad fsm %d\n",
			ispif->status);
		mutex_unlock(&isp_subdev->ops_mutex);
		return -EINVAL;
	}

	buf_node = isp4if_alloc_buffer_node(buf_info);
	if (!buf_node) {
		dev_err(dev, "fail alloc sys img buf info node\n");
		ret = -ENOMEM;
		goto unlock_and_return;
	}

	ret = isp4if_queue_buffer(ispif, buf_node);
	if (ret) {
		dev_err(dev, "fail to queue image buf, %d\n", ret);
		goto error_release_buf_node;
	}

	if (!isp_subdev->sensor_info.start_stream_cmd_sent) {
		isp_subdev->sensor_info.buf_sent_cnt++;

		if (isp_subdev->sensor_info.buf_sent_cnt >=
		    ISP4SD_MIN_BUF_CNT_BEF_START_STREAM) {
			ret = isp4if_send_command(ispif, CMD_ID_START_STREAM,
						  NULL, 0);

			if (ret) {
				dev_err(dev, "fail to START_STREAM");
				goto error_release_buf_node;
			}
			isp_subdev->sensor_info.start_stream_cmd_sent = true;
			isp_subdev->sensor_info.output_info.start_status =
				ISP4SD_START_STATUS_STARTED;
			isp_subdev->sensor_info.status =
				ISP4SD_START_STATUS_STARTED;
		} else {
			dev_dbg(dev,
				"no send start,required %u,buf sent %u\n",
				ISP4SD_MIN_BUF_CNT_BEF_START_STREAM,
				isp_subdev->sensor_info.buf_sent_cnt);
		}
	}

	mutex_unlock(&isp_subdev->ops_mutex);

	return 0;

error_release_buf_node:
	isp4if_dealloc_buffer_node(buf_node);

unlock_and_return:
	mutex_unlock(&isp_subdev->ops_mutex);

	return ret;
}

static int isp4sd_set_power(struct v4l2_subdev *sd, int on)
{
	struct isp4_subdev *ispsd = to_isp4_subdev(sd);

	if (on)
		return isp4sd_pwron_and_init(ispsd);
	else
		return isp4sd_pwroff_and_deinit(ispsd);
};

static const struct v4l2_subdev_core_ops isp4sd_core_ops = {
	.s_power = isp4sd_set_power,
};

static const struct v4l2_subdev_video_ops isp4sd_video_ops = {
	.s_stream = v4l2_subdev_s_stream_helper,
};

static int isp4sd_set_pad_format(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *sd_state,
				 struct v4l2_subdev_format *fmt)
{
	struct isp4sd_output_info *steam_info =
		&(to_isp4_subdev(sd)->sensor_info.output_info);
	struct v4l2_mbus_framefmt *format;

	format = v4l2_subdev_state_get_format(sd_state, fmt->pad);

	if (!format) {
		dev_err(sd->dev, "fail to get state format\n");
		return -EINVAL;
	}

	*format = fmt->format;
	switch (format->code) {
	case MEDIA_BUS_FMT_YUYV8_1_5X8:
		steam_info->image_size = format->width * format->height * 3 / 2;
		break;
	case MEDIA_BUS_FMT_YUYV8_1X16:
		steam_info->image_size = format->width * format->height * 2;
		break;
	default:
		steam_info->image_size = 0;
		break;
	}
	if (!steam_info->image_size) {
		dev_err(sd->dev,
			"fail set pad format,code 0x%x,width %u, height %u\n",
			format->code, format->width, format->height);
		return -EINVAL;
	}
	dev_dbg(sd->dev,
		"set pad format suc, code:%x w:%u h:%u size:%u\n", format->code,
		format->width, format->height, steam_info->image_size);

	return 0;
}

static int isp4sd_enable_streams(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *state, u32 pad,
				 u64 streams_mask)
{
	struct isp4_subdev *ispsd = to_isp4_subdev(sd);

	return isp4sd_start_stream(ispsd, state, pad);
}

static int isp4sd_disable_streams(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *state, u32 pad,
				  u64 streams_mask)
{
	struct isp4_subdev *ispsd = to_isp4_subdev(sd);

	return isp4sd_stop_stream(ispsd, state, pad);
}

static const struct v4l2_subdev_pad_ops isp4sd_pad_ops = {
	.get_fmt = v4l2_subdev_get_fmt,
	.set_fmt = isp4sd_set_pad_format,
	.enable_streams = isp4sd_enable_streams,
	.disable_streams = isp4sd_disable_streams,
};

static const struct v4l2_subdev_ops isp4sd_subdev_ops = {
	.core = &isp4sd_core_ops,
	.video = &isp4sd_video_ops,
	.pad = &isp4sd_pad_ops,
};

static int isp4sd_sdev_link_validate(struct media_link *link)
{
	return 0;
}

static const struct media_entity_operations isp4sd_sdev_ent_ops = {
	.link_validate = isp4sd_sdev_link_validate,
};

static const struct isp4vid_ops isp4sd_isp4vid_ops = {
	.send_buffer = isp4sd_ioc_send_img_buf,
};

int isp4sd_init(struct isp4_subdev *isp_subdev,
		struct v4l2_device *v4l2_dev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;
	struct isp4sd_sensor_info *sensor_info;
	struct device *dev = v4l2_dev->dev;
	int ret;

	isp_subdev->dev = dev;
	v4l2_subdev_init(&isp_subdev->sdev, &isp4sd_subdev_ops);
	isp_subdev->sdev.owner = THIS_MODULE;
	isp_subdev->sdev.dev = dev;
	snprintf(isp_subdev->sdev.name, sizeof(isp_subdev->sdev.name), "%s",
		 dev_name(dev));

	isp_subdev->sdev.entity.name = isp4sd_entity_name;
	isp_subdev->sdev.entity.function = MEDIA_ENT_F_PROC_VIDEO_ISP;
	isp_subdev->sdev.entity.ops = &isp4sd_sdev_ent_ops;
	isp_subdev->sdev_pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&isp_subdev->sdev.entity, 1,
				     &isp_subdev->sdev_pad);
	if (ret) {
		dev_err(dev, "fail to init isp4 subdev entity pad %d\n", ret);
		return ret;
	}
	ret = v4l2_subdev_init_finalize(&isp_subdev->sdev);
	if (ret < 0) {
		dev_err(dev, "fail to init finalize isp4 subdev %d\n",
			ret);
		return ret;
	}
	ret = v4l2_device_register_subdev(v4l2_dev, &isp_subdev->sdev);
	if (ret) {
		dev_err(dev, "fail to register isp4 subdev to V4L2 device %d\n",
			ret);
		goto err_media_clean_up;
	}

	sensor_info = &isp_subdev->sensor_info;

	isp4if_init(ispif, dev, isp_subdev->mmio);

	mutex_init(&isp_subdev->ops_mutex);
	sensor_info->start_stream_cmd_sent = false;
	sensor_info->status = ISP4SD_START_STATUS_NOT_START;

	/* create ISP enable gpio control */
	isp_subdev->enable_gpio = devm_gpiod_get(isp_subdev->dev,
						 "enable_isp",
						 GPIOD_OUT_LOW);
	if (IS_ERR(isp_subdev->enable_gpio)) {
		dev_err(dev, "fail to get gpiod %d\n", ret);
		media_entity_cleanup(&isp_subdev->sdev.entity);
		return PTR_ERR(isp_subdev->enable_gpio);
	}

	isp_subdev->host2fw_seq_num = 1;
	ispif->status = ISP4IF_STATUS_PWR_OFF;

	ret = isp4vid_dev_init(&isp_subdev->isp_vdev, &isp_subdev->sdev,
			       &isp4sd_isp4vid_ops);
	if (ret)
		goto err_media_clean_up;
	return ret;

err_media_clean_up:
	media_entity_cleanup(&isp_subdev->sdev.entity);
	return ret;
}

void isp4sd_deinit(struct isp4_subdev *isp_subdev)
{
	struct isp4_interface *ispif = &isp_subdev->ispif;

	isp4vid_dev_deinit(&isp_subdev->isp_vdev);
	media_entity_cleanup(&isp_subdev->sdev.entity);
	isp4if_deinit(ispif);
	isp4sd_module_enable(isp_subdev, false);

	ispif->status = ISP4IF_STATUS_PWR_OFF;
}
