// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/usb.h>

#include "diag_stub.h"
#include "i2c_stub.h"
#include "mng_stub.h"
#include "ulpss_bridge.h"

/* Define these values to match your devices */
#define USB_BRIDGE_VENDOR_ID 0x8086
#define USB_BRIDGE_PRODUCT_ID 0x0b63

/* table of devices that work with this driver */
static const struct usb_device_id intel_ulpss_bridge_table[] = {
	{ USB_DEVICE(USB_BRIDGE_VENDOR_ID, USB_BRIDGE_PRODUCT_ID) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, intel_ulpss_bridge_table);

static void intel_ulpss_bridge_read_cb(struct urb *urb)
{
	struct usb_bridge *intel_ulpss_dev;
	struct bridge_msg msg;
	unsigned long flags;
	bool need_sched;
	int ret;

	intel_ulpss_dev = urb->context;
	BUG_ON(!intel_ulpss_dev);
	dev_dbg(&intel_ulpss_dev->intf->dev,
		"%s bulk read urb got message from fw, status:%d data_len:%d\n",
		__func__, urb->status, urb->actual_length);

	if (urb->status || intel_ulpss_dev->errors) {
		/* sync/async unlink faults aren't errors */
		if (urb->status == -ENOENT || urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN) {
			dev_dbg(&intel_ulpss_dev->intf->dev,
				"%s read bulk urb unlink: %d %d\n", __func__,
				urb->status, intel_ulpss_dev->errors);
			return;
		} else {
			dev_err(&intel_ulpss_dev->intf->dev,
				"%s read bulk urb transfer failed: %d %d\n",
				__func__, urb->status, intel_ulpss_dev->errors);
			goto resubmmit;
		}
	}

	msg.len = urb->actual_length;
	memcpy(msg.buf, intel_ulpss_dev->bulk_in_buffer, urb->actual_length);

	spin_lock_irqsave(&intel_ulpss_dev->msg_fifo_spinlock, flags);
	need_sched = kfifo_is_empty(&intel_ulpss_dev->msg_fifo);

	if (kfifo_put(&intel_ulpss_dev->msg_fifo, msg)) {
		if (need_sched)
			schedule_work(&intel_ulpss_dev->event_work);
	} else {
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s put msg faild full:%d\n", __func__,
			kfifo_is_full(&intel_ulpss_dev->msg_fifo));
	}

	spin_unlock_irqrestore(&intel_ulpss_dev->msg_fifo_spinlock, flags);

resubmmit:
	/* resubmmit urb to receive fw msg */
	ret = usb_submit_urb(intel_ulpss_dev->bulk_in_urb, GFP_KERNEL);
	if (ret) {
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s failed submitting read urb, error %d\n", __func__,
			ret);
	}
}

static int intel_ulpss_bridge_read_start(struct usb_bridge *intel_ulpss_dev)
{
	int ret;

	/* prepare a read */
	usb_fill_bulk_urb(
		intel_ulpss_dev->bulk_in_urb, intel_ulpss_dev->udev,
		usb_rcvbulkpipe(intel_ulpss_dev->udev,
				intel_ulpss_dev->bulk_in_endpointAddr),
		intel_ulpss_dev->bulk_in_buffer, intel_ulpss_dev->bulk_in_size,
		intel_ulpss_bridge_read_cb, intel_ulpss_dev);

	/* submit read urb */
	ret = usb_submit_urb(intel_ulpss_dev->bulk_in_urb, GFP_KERNEL);
	if (ret) {
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s - failed submitting read urb, error %d\n", __func__,
			ret);
	}
	return ret;
}

static void intel_ulpss_bridge_write_cb(struct urb *urb)
{
	struct usb_bridge *intel_ulpss_dev;

	intel_ulpss_dev = urb->context;

	if (!intel_ulpss_dev)
		return;

	if (urb->status) {
		/* sync/async unlink faults aren't errors */
		if (urb->status == -ENOENT || urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN) {
			dev_warn(&intel_ulpss_dev->intf->dev,
				 "%s write bulk urb unlink: %d\n", __func__,
				 urb->status);
		} else {
			dev_err(&intel_ulpss_dev->intf->dev,
				"%s write bulk urb transfer failed: %d\n",
				__func__, urb->status);

			intel_ulpss_dev->errors = urb->status;
		}
	}

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length,
			  urb->transfer_buffer, urb->transfer_dma);

	dev_dbg(&intel_ulpss_dev->intf->dev, "%s write callback out\n",
		__func__);
}

ssize_t intel_ulpss_bridge_write(struct usb_interface *intf, void *data,
				 size_t len, unsigned int timeout)
{
	struct urb *urb = NULL;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);
	char *buf = NULL;
	int time;
	int ret;

	if (!len || len > MAX_PACKET_SIZE) {
		dev_err(&intf->dev, "%s write len not correct len:%ld\n",
			__func__, len);
		return -EINVAL;
	}

	mutex_lock(&intel_ulpss_dev->write_mutex);
	usb_autopm_get_interface(intf);

	if (intel_ulpss_dev->errors) {
		dev_err(&intf->dev, "%s dev error %d\n", __func__,
			intel_ulpss_dev->errors);
		intel_ulpss_dev->errors = 0;
		ret = -EINVAL;
		goto error;
	}

	/* create a urb, and a buffer for it, and copy the data to the urb */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		ret = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(intel_ulpss_dev->udev, len, GFP_KERNEL,
				 &urb->transfer_dma);

	if (!buf) {
		ret = -ENOMEM;
		goto error;
	}

	memcpy(buf, data, len);

	if (intel_ulpss_dev->disconnected) { /* disconnect() was called */
		ret = -ENODEV;
		goto error;
	}

	/* initialize the urb properly */
	usb_fill_bulk_urb(
		urb, intel_ulpss_dev->udev,
		usb_sndbulkpipe(intel_ulpss_dev->udev,
				intel_ulpss_dev->bulk_out_endpointAddr),
		buf, len, intel_ulpss_bridge_write_cb, intel_ulpss_dev);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usb_anchor_urb(urb, &intel_ulpss_dev->write_submitted);
	/* send the data out the bulk port */
	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret) {
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s - failed submitting write urb, error %d\n",
			__func__, ret);
		goto error_unanchor;
	}

	/*
	 * release our reference to this urb, the USB core will eventually free
	 * it entirely
	 */
	usb_free_urb(urb);

	time = usb_wait_anchor_empty_timeout(&intel_ulpss_dev->write_submitted,
					     timeout);
	if (!time) {
		usb_kill_anchored_urbs(&intel_ulpss_dev->write_submitted);
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s waiting out urb sending timeout, error %d %d\n",
			__func__, time, timeout);
	}

	usb_autopm_put_interface(intf);
	mutex_unlock(&intel_ulpss_dev->write_mutex);
	return len;

error_unanchor:
	usb_unanchor_urb(urb);
error:
	if (urb) {
		/* free up our allocated buffer */
		usb_free_coherent(urb->dev, urb->transfer_buffer_length,
				  urb->transfer_buffer, urb->transfer_dma);
		usb_free_urb(urb);
	}

	usb_autopm_put_interface(intf);
	mutex_unlock(&intel_ulpss_dev->write_mutex);
	return ret;
}

static void intel_ulpss_bridge_delete(struct usb_bridge *intel_ulpss_dev)
{
	usb_free_urb(intel_ulpss_dev->bulk_in_urb);
	usb_put_intf(intel_ulpss_dev->intf);
	usb_put_dev(intel_ulpss_dev->udev);
	kfree(intel_ulpss_dev->bulk_in_buffer);
	kfree(intel_ulpss_dev);
}

static int intel_ulpss_bridge_init(struct usb_bridge *intel_ulpss_dev)
{
	mutex_init(&intel_ulpss_dev->write_mutex);
	init_usb_anchor(&intel_ulpss_dev->write_submitted);
	init_waitqueue_head(&intel_ulpss_dev->bulk_out_ack);
	INIT_LIST_HEAD(&intel_ulpss_dev->stubs_list);
	INIT_KFIFO(intel_ulpss_dev->msg_fifo);
	spin_lock_init(&intel_ulpss_dev->msg_fifo_spinlock);

	intel_ulpss_dev->state = USB_BRIDGE_INITED;

	return 0;
}

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_stub *mng_stub = usb_stub_find(intf, MNG_CMD_TYPE);
	struct usb_stub *diag_stub = usb_stub_find(intf, DIAG_CMD_TYPE);
	int ret;

	dev_dbg(dev, "%s:%u %s\n", __func__, __LINE__, buf);
	if (sysfs_streq(buf, "dfu")) {
		ret = mng_set_dfu_mode(mng_stub);
	} else if (sysfs_streq(buf, "reset")) {
		ret = mng_reset(mng_stub);
	} else if (sysfs_streq(buf, "debug")) {
		ret = diag_set_trace_level(diag_stub, 3);
	}

	return count;
}

static ssize_t cmd_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	dev_dbg(dev, "%s:%u \n", __func__, __LINE__);

	return sprintf(buf, "%s\n", "supported cmd: [dfu, reset, debug]");
}
static DEVICE_ATTR_RW(cmd);

static ssize_t version_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_stub *mng_stub = usb_stub_find(intf, MNG_CMD_TYPE);

	dev_dbg(dev, "%s:%u\n", __func__, __LINE__);
	return mng_get_version_string(mng_stub, buf);
}
static DEVICE_ATTR_RO(version);

static ssize_t log_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	ssize_t len;
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_stub *diag_stub = usb_stub_find(intf, DIAG_CMD_TYPE);

	ret = diag_get_fw_log(diag_stub, buf, &len);
	dev_dbg(dev, "%s:%u len %ld\n", __func__, __LINE__, len);

	if (ret)
		return ret;
	else
		return len;
}
static DEVICE_ATTR_RO(log);

static ssize_t coredump_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;
	ssize_t len;
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_stub *diag_stub = usb_stub_find(intf, DIAG_CMD_TYPE);

	ret = diag_get_coredump(diag_stub, buf, &len);
	dev_dbg(dev, "%s:%u len %ld\n", __func__, __LINE__, len);

	if (ret)
		return ret;
	else
		return len;
}
static DEVICE_ATTR_RO(coredump);

static struct attribute *intel_ulpss_bridge_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_cmd.attr,
	&dev_attr_log.attr,
	&dev_attr_coredump.attr,
	NULL,
};
ATTRIBUTE_GROUPS(intel_ulpss_bridge);

static int intel_ulpss_bridge_probe(struct usb_interface *intf,
				    const struct usb_device_id *id)
{
	struct usb_bridge *intel_ulpss_dev;
	struct usb_endpoint_descriptor *bulk_in, *bulk_out;
	struct usb_stub *stub;
	int ret;

	/* allocate memory for our device state and initialize it */
	intel_ulpss_dev = kzalloc(sizeof(*intel_ulpss_dev), GFP_KERNEL);
	if (!intel_ulpss_dev)
		return -ENOMEM;

	intel_ulpss_bridge_init(intel_ulpss_dev);
	intel_ulpss_dev->udev = usb_get_dev(interface_to_usbdev(intf));
	intel_ulpss_dev->intf = usb_get_intf(intf);

	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */
	ret = usb_find_common_endpoints(intf->cur_altsetting, &bulk_in,
					&bulk_out, NULL, NULL);
	if (ret) {
		dev_err(&intf->dev,
			"Could not find both bulk-in and bulk-out endpoints\n");
		goto error;
	}

	intel_ulpss_dev->bulk_in_size = usb_endpoint_maxp(bulk_in);
	intel_ulpss_dev->bulk_in_endpointAddr = bulk_in->bEndpointAddress;
	intel_ulpss_dev->bulk_in_buffer =
		kzalloc(intel_ulpss_dev->bulk_in_size, GFP_KERNEL);
	if (!intel_ulpss_dev->bulk_in_buffer) {
		ret = -ENOMEM;
		goto error;
	}
	intel_ulpss_dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!intel_ulpss_dev->bulk_in_urb) {
		ret = -ENOMEM;
		goto error;
	}
	intel_ulpss_dev->bulk_out_endpointAddr = bulk_out->bEndpointAddress;

	dev_dbg(&intf->dev, "bulk_in size:%ld addr:%d bulk_out addr:%d\n",
		intel_ulpss_dev->bulk_in_size,
		intel_ulpss_dev->bulk_in_endpointAddr,
		intel_ulpss_dev->bulk_out_endpointAddr);

	/* save our data pointer in this intf device */
	usb_set_intfdata(intf, intel_ulpss_dev);

	ret = intel_ulpss_bridge_read_start(intel_ulpss_dev);
	if (ret) {
		dev_err(&intf->dev, "%s bridge read start failed ret %d\n",
			__func__, ret);
		goto error;
	}

	ret = usb_stub_init(intf);
	if (ret) {
		dev_err(&intf->dev, "%s usb stub init failed ret %d\n",
			__func__, ret);
		usb_set_intfdata(intf, NULL);
		goto error;
	}

	ret = mng_stub_init(intf, NULL, 0);
	if (ret) {
		dev_err(&intf->dev, "%s register mng stub failed ret %d\n",
			__func__, ret);
		return ret;
	}

	ret = diag_stub_init(intf, NULL, 0);
	if (ret) {
		dev_err(&intf->dev, "%s register diag stub failed ret %d\n",
			__func__, ret);
		return ret;
	}

	stub = usb_stub_find(intf, MNG_CMD_TYPE);
	if (!stub) {
		ret = -EINVAL;
		return ret;
	}

	ret = mng_stub_link(intf, stub);
	if (intel_ulpss_dev->state != USB_BRIDGE_STARTED) {
		dev_err(&intf->dev, "%s mng stub link done ret:%d state:%d\n",
			__func__, ret, intel_ulpss_dev->state);
		return ret;
	}

	usb_enable_autosuspend(intel_ulpss_dev->udev);
	dev_info(&intf->dev, "intel_ulpss USB bridge device init success\n");
	return 0;

error:
	/* this frees allocated memory */
	intel_ulpss_bridge_delete(intel_ulpss_dev);

	return ret;
}

static void intel_ulpss_bridge_disconnect(struct usb_interface *intf)
{
	struct usb_bridge *intel_ulpss_dev;

	intel_ulpss_dev = usb_get_intfdata(intf);
	intel_ulpss_dev->disconnected = 1;

	usb_kill_urb(intel_ulpss_dev->bulk_in_urb);
	usb_kill_anchored_urbs(&intel_ulpss_dev->write_submitted);

	usb_stub_cleanup(intf);
	intel_ulpss_dev->state = USB_BRIDGE_STOPPED;

	cancel_work_sync(&intel_ulpss_dev->event_work);

	usb_set_intfdata(intf, NULL);
	/* decrement our usage len */
	intel_ulpss_bridge_delete(intel_ulpss_dev);

	dev_dbg(&intf->dev, "USB bridge now disconnected\n");
}

static void intel_ulpss_bridge_draw_down(struct usb_bridge *intel_ulpss_dev)
{
	int time;

	time = usb_wait_anchor_empty_timeout(&intel_ulpss_dev->write_submitted,
					     1000);
	if (!time)
		usb_kill_anchored_urbs(&intel_ulpss_dev->write_submitted);
	usb_kill_urb(intel_ulpss_dev->bulk_in_urb);
}

static int intel_ulpss_bridge_suspend(struct usb_interface *intf,
				      pm_message_t message)
{
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);
	dev_dbg(&intf->dev, "USB bridge now suspend\n");

	intel_ulpss_bridge_draw_down(intel_ulpss_dev);
	return 0;
}

static int intel_ulpss_bridge_resume(struct usb_interface *intf)
{
	int ret;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);
	dev_dbg(&intf->dev, "USB bridge now resume\n");

	ret = intel_ulpss_bridge_read_start(intel_ulpss_dev);
	if (ret) {
		dev_err(&intf->dev, "%s bridge read start failed ret %d\n",
			__func__, ret);
	}
	return ret;
}
static struct usb_driver bridge_driver = {
	.name = "intel_ulpss",
	.probe = intel_ulpss_bridge_probe,
	.disconnect = intel_ulpss_bridge_disconnect,
	.suspend = intel_ulpss_bridge_suspend,
	.resume = intel_ulpss_bridge_resume,
	.id_table = intel_ulpss_bridge_table,
	.dev_groups = intel_ulpss_bridge_groups,
	.supports_autosuspend = 1,
};

module_usb_driver(bridge_driver);

MODULE_AUTHOR("Ye Xiang <xiang.ye@intel.com>");
MODULE_AUTHOR("Zhang Lixu <lixu.zhang@intel.com>");
MODULE_DESCRIPTION("Intel LPSS USB driver");
MODULE_LICENSE("GPL v2");
