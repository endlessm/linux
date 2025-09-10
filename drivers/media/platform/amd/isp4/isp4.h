/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#ifndef _ISP4_H_
#define _ISP4_H_

#include <drm/amd/isp.h>
#include <linux/mutex.h>
#include "isp4_subdev.h"

#define ISP4_GET_ISP_REG_BASE(isp4sd) (((isp4sd))->mmio)

struct isp4_device {
	struct v4l2_device v4l2_dev;
	struct isp4_subdev isp_sdev;
	struct media_device mdev;

	struct isp_platform_data *pltf_data;
	struct platform_device *pdev;
	struct notifier_block i2c_nb;
	struct v4l2_async_notifier notifier;
};

#endif /* _ISP4_H_ */
