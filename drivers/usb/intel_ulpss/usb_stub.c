// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include "protocol_intel_ulpss.h"
#include "ulpss_bridge.h"

#define MAKE_CMD_ID(type, cmd) ((type << 8) | cmd)
static bool usb_stub_validate(u8 *data, u32 data_len)
{
	bool is_valid = true;
	struct cmd_header *header = (struct cmd_header *)data;

	/* verify the respone flag */
	if (header->cmd != GPIO_INTR_NOTIFY &&
	    ((header->flags & RESP_FLAG) == 0))
		is_valid = false;

	/* verify the payload len */
	is_valid = is_valid && (header->len + sizeof(*header) == data_len);

	return is_valid;
}

static int usb_stub_parse(struct usb_stub *stub, struct cmd_header *header)
{
	int ret = 0;

	if (!stub || !header || (header->len < 0))
		return -EINVAL;

	stub->len = header->len;

	if (header->len == 0)
		return 0;

	memcpy(stub->buf, header->data, header->len);
	if (stub->parse)
		ret = stub->parse(stub, header->cmd, header->flags,
				  header->data, header->len);

	return ret;
}

/*
 * Bottom half processing work function (instead of thread handler)
 * for processing fw messages
 */
static void event_work_cb(struct work_struct *work)
{
	struct usb_bridge *intel_ulpss_dev;
	struct bridge_msg msg_in_proc = { 0 };
	struct usb_stub *stub;
	struct cmd_header *header;
	int rcv_cmd_id;
	int ret;

	intel_ulpss_dev = container_of(work, struct usb_bridge, event_work);
	BUG_ON(!intel_ulpss_dev);

	while (kfifo_get(&intel_ulpss_dev->msg_fifo, &msg_in_proc)) {
		if (!msg_in_proc.len)
			continue;

		header = (struct cmd_header *)msg_in_proc.buf;

		dev_dbg(&intel_ulpss_dev->intf->dev,
			"receive: type:%d cmd:%d flags:%d len:%d\n",
			header->type, header->cmd, header->flags, header->len);

		/* verify the data */
		if (!usb_stub_validate(msg_in_proc.buf, msg_in_proc.len)) {
			dev_err(&intel_ulpss_dev->intf->dev,
				"%s header->len:%d payload_len:%ld\n ",
				__func__, header->len, msg_in_proc.len);
			continue;
		}

		stub = usb_stub_find(intel_ulpss_dev->intf, header->type);
		ret = usb_stub_parse(stub, header);
		if (ret) {
			dev_err(&intel_ulpss_dev->intf->dev,
				"%s failed to parse data: ret:%d type:%d len: %d",
				__func__, ret, header->type, header->len);
			continue;
		}

		rcv_cmd_id = MAKE_CMD_ID(stub->type, header->cmd);
		if (rcv_cmd_id == stub->cmd_id) {
			stub->acked = true;
			wake_up_interruptible(&intel_ulpss_dev->bulk_out_ack);

		} else {
			dev_warn(&intel_ulpss_dev->intf->dev,
				 "%s rcv_cmd_id:%x != stub->cmd_id:%x",
				 __func__, rcv_cmd_id, stub->cmd_id);
		}
	}
}

struct usb_stub *usb_stub_alloc(struct usb_interface *intf)
{
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);
	struct usb_stub *cur_stub;

	cur_stub = kzalloc(sizeof(struct usb_stub), GFP_KERNEL);
	if (!cur_stub) {
		dev_err(&intf->dev, "%s no memory for new stub", __func__);
		return NULL;
	}

	mutex_init(&cur_stub->stub_mutex);
	INIT_LIST_HEAD(&cur_stub->list);

	list_add_tail(&cur_stub->list, &intel_ulpss_dev->stubs_list);
	intel_ulpss_dev->stub_count++;
	dev_dbg(&intf->dev,
		"%s enuming stub intel_ulpss_dev->stub_count:%ld type:%d success\n",
		__func__, intel_ulpss_dev->stub_count, cur_stub->type);

	return cur_stub;
}

int usb_stub_init(struct usb_interface *intf)
{
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);

	if (!intel_ulpss_dev)
		return -EINVAL;

	INIT_WORK(&intel_ulpss_dev->event_work, event_work_cb);

	return 0;
}

struct usb_stub *usb_stub_find(struct usb_interface *intf, u8 type)
{
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);
	struct usb_stub *cur_stub;

	if (!intel_ulpss_dev)
		return NULL;

	list_for_each_entry (cur_stub, &intel_ulpss_dev->stubs_list, list) {
		if (cur_stub->type == type)
			return cur_stub;
	}

	dev_err(&intf->dev, "%s usb stub not find, type: %d", __func__, type);
	return NULL;
}

int usb_stub_write(struct usb_stub *stub, u8 cmd, u8 *data, u8 len,
		   bool wait_ack, long timeout)
{
	int ret;
	u8 flags = 0;
	u8 buff[MAX_PACKET_SIZE] = { 0 };

	struct cmd_header *header;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(stub->intf);

	if (!stub || !intel_ulpss_dev)
		return -EINVAL;

	header = (struct cmd_header *)buff;
	if (len >= MAX_PAYLOAD_SIZE)
		return -ENOMEM;

	if (wait_ack)
		flags |= ACK_FLAG;

	flags |= CMPL_FLAG;

	header->type = stub->type;
	header->cmd = cmd;
	header->flags = flags;
	header->len = len;

	memcpy(header->data, data, len);
	dev_dbg(&intel_ulpss_dev->intf->dev,
		"send: type:%d cmd:%d flags:%d len:%d\n", header->type,
		header->cmd, header->flags, header->len);

	stub->cmd_id = MAKE_CMD_ID(stub->type, cmd);

	ret = intel_ulpss_bridge_write(
		stub->intf, header, sizeof(struct cmd_header) + len, timeout);

	if (ret != sizeof(struct cmd_header) + len) {
		dev_err(&intel_ulpss_dev->intf->dev,
			"%s bridge write failed ret:%d total_len:%ld\n ",
			__func__, ret, sizeof(struct cmd_header) + len);
		return -EIO;
	}

	if (flags & ACK_FLAG) {
		ret = wait_event_interruptible_timeout(
			intel_ulpss_dev->bulk_out_ack, (stub->acked), timeout);
		stub->acked = false;

		if (ret < 0) {
			dev_err(&intel_ulpss_dev->intf->dev,
				"acked wait interrupted ret:%d timeout:%ld ack:%d\n",
				ret, timeout, stub->acked);
			return ret;

		} else if (ret == 0) {
			dev_err(&intel_ulpss_dev->intf->dev,
				"acked sem wait timed out ret:%d timeout:%ld ack:%d\n",
				ret, timeout, stub->acked);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

void usb_stub_broadcast(struct usb_interface *intf, long event,
			void *event_data)
{
	int ret = 0;
	struct usb_stub *cur_stub = NULL;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);

	if (!intel_ulpss_dev)
		return;

	list_for_each_entry (cur_stub, &intel_ulpss_dev->stubs_list, list) {
		if (cur_stub && cur_stub->notify) {
			ret = cur_stub->notify(cur_stub, event, event_data);
			if (ret)
				continue;
		}
	}
}

void usb_stub_cleanup(struct usb_interface *intf)
{
	struct usb_stub *cur_stub = NULL;
	struct usb_stub *next = NULL;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);

	if (!intel_ulpss_dev)
		return;

	list_for_each_entry_safe (cur_stub, next, &intel_ulpss_dev->stubs_list,
				  list) {
		if (!cur_stub)
			continue;

		list_del_init(&cur_stub->list);
		dev_dbg(&intf->dev, "%s type:%d\n ", __func__, cur_stub->type);
		if (cur_stub->cleanup)
			cur_stub->cleanup(cur_stub);

		mutex_destroy(&cur_stub->stub_mutex);
		kfree(cur_stub);

		intel_ulpss_dev->stub_count--;
	}
}

struct acpi_device *find_adev_by_hid(struct acpi_device *parent,
				     const char *hid)
{
	struct acpi_device *adev;

	if (!parent)
		return NULL;

	list_for_each_entry (adev, &parent->children, node) {
		if (!strcmp(hid, acpi_device_hid(adev)))
			return adev;
	}

	return NULL;
}
