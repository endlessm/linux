/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#ifndef _ISP4_VIDEO_H_
#define _ISP4_VIDEO_H_

#include <linux/mutex.h>
#include <media/videobuf2-memops.h>
#include <media/v4l2-dev.h>
#include "isp4_interface.h"

enum isp4vid_buf_done_status {
	/* It means no corresponding image buf in fw response */
	ISP4VID_BUF_DONE_STATUS_ABSENT,
	ISP4VID_BUF_DONE_STATUS_SUCCESS,
	ISP4VID_BUF_DONE_STATUS_FAILED
};

struct isp4vid_buf_done_info {
	enum isp4vid_buf_done_status status;
	struct isp4if_img_buf_info buf;
};

/* call back parameter for CB_EVT_ID_FRAME_DONE */
struct isp4vid_framedone_param {
	s32 poc;
	s32 cam_id;
	s64 time_stamp;
	struct isp4vid_buf_done_info preview;
};

struct isp4vid_capture_buffer {
	/*
	 * struct vb2_v4l2_buffer must be the first element
	 * the videobuf2 framework will allocate this struct based on
	 * buf_struct_size and use the first sizeof(struct vb2_buffer) bytes of
	 * memory as a vb2_buffer
	 */
	struct vb2_v4l2_buffer vb2;
	struct isp4if_img_buf_info img_buf;
	struct list_head list;
};

struct isp4vid_dev;

struct isp4vid_ops {
	int (*send_buffer)(struct v4l2_subdev *sd,
			   struct isp4if_img_buf_info *img_buf);
};

struct isp4vid_dev {
	struct video_device vdev;
	struct media_pad vdev_pad;
	struct v4l2_pix_format format;

	/* mutex that protects vbq */
	struct mutex vbq_lock;
	struct vb2_queue vbq;

	/* mutex that protects buf_list */
	struct mutex buf_list_lock;
	struct list_head buf_list;

	u32 sequence;
	bool stream_started;
	struct task_struct *kthread;

	struct media_pipeline pipe;
	struct device *dev;
	struct v4l2_subdev *isp_sdev;
	struct v4l2_fract timeperframe;

	/* Callback operations */
	const struct isp4vid_ops *ops;
};

int isp4vid_dev_init(struct isp4vid_dev *isp_vdev,
		     struct v4l2_subdev *isp_sdev,
		     const struct isp4vid_ops *ops);

void isp4vid_dev_deinit(struct isp4vid_dev *isp_vdev);

s32 isp4vid_notify(void *cb_ctx, struct isp4vid_framedone_param *evt_param);

#endif
