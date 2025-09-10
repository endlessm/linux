/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#ifndef _ISP4_CONTEXT_H_
#define _ISP4_CONTEXT_H_

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <media/v4l2-device.h>

#include "isp4_fw_cmd_resp.h"
#include "isp4_hw_reg.h"
#include "isp4_interface.h"
#include "isp4_video.h"

/*
 * one is for none sesnor specefic response which is not used now
 * another is for sensor specific response
 */
#define ISP4SD_MAX_FW_RESP_STREAM_NUM 2

/*
 * cmd used to register frame done callback, parameter is
 * struct isp4sd_register_framedone_cb_param *
 * when a image buffer is filled by ISP, ISP will call the registered callback.
 * callback func prototype is isp4sd_framedone_cb, cb_ctx can be anything
 * provided by caller which will be provided back as the first parameter of the
 * callback function.
 * both cb_func and cb_ctx are provide by caller, set cb_func to NULL to
 * unregister the callback
 */

/* used to indicate the ISP status */
enum isp4sd_status {
	ISP4SD_STATUS_PWR_OFF,
	ISP4SD_STATUS_PWR_ON,
	ISP4SD_STATUS_FW_RUNNING,
	ISP4SD_STATUS_MAX
};

/* used to indicate the status of sensor, output stream */
enum isp4sd_start_status {
	ISP4SD_START_STATUS_NOT_START,
	ISP4SD_START_STATUS_STARTED,
	ISP4SD_START_STATUS_START_FAIL,
};

struct isp4sd_img_buf_node {
	struct list_head node;
	struct isp4if_img_buf_info buf_info;
};

/* this is isp output after processing bayer raw input from sensor */
struct isp4sd_output_info {
	enum isp4sd_start_status start_status;
	u32 image_size;
};

/*
 * This struct represents the sensor info which is input or source of ISP,
 * meta_info_buf is the buffer store the fw to driver metainfo response
 * status is the sensor status
 * output_info is the isp output info after ISP processing the sensor input,
 * start_stream_cmd_sent mean if CMD_ID_START_STREAM has sent to fw.
 * buf_sent_cnt is buffer count app has sent to receive the images
 */
struct isp4sd_sensor_info {
	struct isp4if_gpu_mem_info *
		meta_info_buf[ISP4IF_MAX_STREAM_BUF_COUNT];
	struct isp4sd_output_info output_info;
	enum isp4sd_start_status status;
	bool start_stream_cmd_sent;
	u32 buf_sent_cnt;
};

/*
 * Thread created by driver to receive fw response
 * thread will be wakeup by fw to driver response interrupt
 */
struct isp4sd_thread_handler {
	struct task_struct *thread;
	struct mutex mutex; /* mutex */
	wait_queue_head_t waitq;
	int wq_cond;
};

struct isp4_subdev_thread_param {
	u32 idx;
	struct isp4_subdev *isp_subdev;
};

struct isp4_subdev {
	struct v4l2_subdev sdev;
	struct isp4_interface ispif;
	struct isp4vid_dev isp_vdev;

	struct media_pad sdev_pad;

	enum isp4sd_status isp_status;
	struct mutex ops_mutex; /* ops_mutex */

	/* Used to store fw cmds sent to FW whose response driver needs to wait for */
	struct isp4sd_thread_handler
		fw_resp_thread[ISP4SD_MAX_FW_RESP_STREAM_NUM];

	u32 host2fw_seq_num;

	struct isp4sd_sensor_info sensor_info;

	/* gpio descriptor */
	struct gpio_desc *enable_gpio;
	struct device *dev;
	void __iomem *mmio;
	struct isp4_subdev_thread_param
		isp_resp_para[ISP4SD_MAX_FW_RESP_STREAM_NUM];
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
	bool enable_fw_log;
	char *fw_log_output;
#endif
};

int isp4sd_init(struct isp4_subdev *isp_subdev,
		struct v4l2_device *v4l2_dev);
void isp4sd_deinit(struct isp4_subdev *isp_subdev);

#endif
