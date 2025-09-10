// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#include <linux/pm_runtime.h>
#include <linux/vmalloc.h>

#include <media/v4l2-fwnode.h>
#include <media/v4l2-ioctl.h>

#include "isp4.h"
#include "isp4_hw_reg.h"

#define ISP4_DRV_NAME "amd_isp_capture"
#define ISP4_FW_RESP_RB_IRQ_STATUS_MASK \
	(ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT9_INT_MASK  | \
	 ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT10_INT_MASK | \
	 ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT11_INT_MASK | \
	 ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT12_INT_MASK)

/* interrupt num */
static const u32 isp4_ringbuf_interrupt_num[] = {
	0, /* ISP_4_1__SRCID__ISP_RINGBUFFER_WPT9 */
	1, /* ISP_4_1__SRCID__ISP_RINGBUFFER_WPT10 */
	3, /* ISP_4_1__SRCID__ISP_RINGBUFFER_WPT11 */
	4, /* ISP_4_1__SRCID__ISP_RINGBUFFER_WPT12 */
};

#define to_isp4_device(dev) container_of(dev, struct isp4_device, v4l2_dev)

static void isp4_wake_up_resp_thread(struct isp4_subdev *isp, u32 index)
{
	if (isp && index < ISP4SD_MAX_FW_RESP_STREAM_NUM) {
		struct isp4sd_thread_handler *thread_ctx =
				&isp->fw_resp_thread[index];

		thread_ctx->wq_cond = 1;
		wake_up_interruptible(&thread_ctx->waitq);
	}
}

static void isp4_resp_interrupt_notify(struct isp4_subdev *isp, u32 intr_status)
{
	bool wake = (isp->ispif.status == ISP4IF_STATUS_FW_RUNNING);

	u32 intr_ack = 0;

	/* global response */
	if (intr_status &
	    ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT12_INT_MASK) {
		if (wake)
			isp4_wake_up_resp_thread(isp, 0);

		intr_ack |= ISP_SYS_INT0_ACK__SYS_INT_RINGBUFFER_WPT12_ACK_MASK;
	}

	/* stream 1 response */
	if (intr_status &
	    ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT9_INT_MASK) {
		if (wake)
			isp4_wake_up_resp_thread(isp, 1);

		intr_ack |= ISP_SYS_INT0_ACK__SYS_INT_RINGBUFFER_WPT9_ACK_MASK;
	}

	/* stream 2 response */
	if (intr_status &
	    ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT10_INT_MASK) {
		if (wake)
			isp4_wake_up_resp_thread(isp, 2);

		intr_ack |= ISP_SYS_INT0_ACK__SYS_INT_RINGBUFFER_WPT10_ACK_MASK;
	}

	/* stream 3 response */
	if (intr_status &
	    ISP_SYS_INT0_STATUS__SYS_INT_RINGBUFFER_WPT11_INT_MASK) {
		if (wake)
			isp4_wake_up_resp_thread(isp, 3);

		intr_ack |= ISP_SYS_INT0_ACK__SYS_INT_RINGBUFFER_WPT11_ACK_MASK;
	}

	/* clear ISP_SYS interrupts */
	isp4hw_wreg(ISP4_GET_ISP_REG_BASE(isp), ISP_SYS_INT0_ACK, intr_ack);
}

static irqreturn_t isp4_irq_handler(int irq, void *arg)
{
	struct isp4_device *isp_dev = dev_get_drvdata(arg);
	struct isp4_subdev *isp = NULL;
	u32 isp_sys_irq_status = 0x0;
	u32 r1;

	if (!isp_dev)
		goto error_drv_data;

	isp = &isp_dev->isp_sdev;
	/* check ISP_SYS interrupts status */
	r1 = isp4hw_rreg(ISP4_GET_ISP_REG_BASE(isp), ISP_SYS_INT0_STATUS);

	isp_sys_irq_status = r1 & ISP4_FW_RESP_RB_IRQ_STATUS_MASK;

	isp4_resp_interrupt_notify(isp, isp_sys_irq_status);

error_drv_data:
	return IRQ_HANDLED;
}

static int isp4_capture_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct isp4_subdev *isp_sdev;
	struct isp4_device *isp_dev;
	size_t i;
	int irq;
	int ret;

	isp_dev = devm_kzalloc(&pdev->dev, sizeof(*isp_dev), GFP_KERNEL);
	if (!isp_dev)
		return -ENOMEM;

	isp_dev->pdev = pdev;
	dev->init_name = ISP4_DRV_NAME;

	isp_sdev = &isp_dev->isp_sdev;
	isp_sdev->mmio = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(isp_sdev->mmio))
		return dev_err_probe(dev, PTR_ERR(isp_sdev->mmio),
				     "isp ioremap fail\n");

	for (i = 0; i < ARRAY_SIZE(isp4_ringbuf_interrupt_num); i++) {
		irq = platform_get_irq(pdev, isp4_ringbuf_interrupt_num[i]);
		if (irq < 0)
			return dev_err_probe(dev, -ENODEV,
					     "fail to get irq %d\n",
					     isp4_ringbuf_interrupt_num[i]);
		ret = devm_request_irq(&pdev->dev, irq, isp4_irq_handler, 0,
				       "ISP_IRQ", &pdev->dev);
		if (ret)
			return dev_err_probe(dev, ret, "fail to req irq %d\n",
					     irq);
	}

	isp_dev->pltf_data = pdev->dev.platform_data;

	/* Link the media device within the v4l2_device */
	isp_dev->v4l2_dev.mdev = &isp_dev->mdev;

	/* Initialize media device */
	strscpy(isp_dev->mdev.model, "amd_isp41_mdev",
		sizeof(isp_dev->mdev.model));
	snprintf(isp_dev->mdev.bus_info, sizeof(isp_dev->mdev.bus_info),
		 "platform:%s", ISP4_DRV_NAME);
	isp_dev->mdev.dev = &pdev->dev;
	media_device_init(&isp_dev->mdev);

	pm_runtime_set_suspended(dev);
	pm_runtime_enable(dev);
	/* register v4l2 device */
	snprintf(isp_dev->v4l2_dev.name, sizeof(isp_dev->v4l2_dev.name),
		 "AMD-V4L2-ROOT");
	ret = v4l2_device_register(&pdev->dev, &isp_dev->v4l2_dev);
	if (ret)
		return dev_err_probe(dev, ret,
				     "fail register v4l2 device\n");

	ret = isp4sd_init(&isp_dev->isp_sdev, &isp_dev->v4l2_dev);
	if (ret) {
		dev_err(dev, "fail init isp4 sub dev %d\n", ret);
		goto err_unreg_v4l2;
	}

	ret = media_device_register(&isp_dev->mdev);
	if (ret) {
		dev_err(dev, "fail to register media device %d\n", ret);
		goto err_isp4_deinit;
	}

	platform_set_drvdata(pdev, isp_dev);

	return 0;

err_isp4_deinit:
	isp4sd_deinit(&isp_dev->isp_sdev);
err_unreg_v4l2:
	v4l2_device_unregister(&isp_dev->v4l2_dev);

	return dev_err_probe(dev, ret, "isp probe fail\n");
}

static void isp4_capture_remove(struct platform_device *pdev)
{
	struct isp4_device *isp_dev = platform_get_drvdata(pdev);

	v4l2_device_unregister_subdev(&isp_dev->isp_sdev.sdev);

	media_device_unregister(&isp_dev->mdev);
	media_entity_cleanup(&isp_dev->isp_sdev.sdev.entity);
	v4l2_device_unregister(&isp_dev->v4l2_dev);

	isp4sd_deinit(&isp_dev->isp_sdev);
}

static struct platform_driver isp4_capture_drv = {
	.probe = isp4_capture_probe,
	.remove = isp4_capture_remove,
	.driver = {
		.name = ISP4_DRV_NAME,
		.owner = THIS_MODULE,
	}
};

module_platform_driver(isp4_capture_drv);

MODULE_ALIAS("platform:" ISP4_DRV_NAME);
MODULE_IMPORT_NS("DMA_BUF");

MODULE_DESCRIPTION("AMD ISP4 Driver");
MODULE_AUTHOR("Bin Du <bin.du@amd.com>");
MODULE_AUTHOR("Pratap Nirujogi <pratap.nirujogi@amd.com>");
MODULE_LICENSE("GPL");
