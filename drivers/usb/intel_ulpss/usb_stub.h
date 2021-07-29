/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __USB_STUB_H__
#define __USB_STUB_H__

#include <linux/acpi.h>
#include <linux/types.h>

#include "protocol_intel_ulpss.h"

#define GPIO_INTR_EVENT 2

struct usb_stub {
	struct list_head list;
	u8 type;
	struct usb_interface *intf;

	struct mutex stub_mutex;
	u8 buf[MAX_PAYLOAD_SIZE];
	u32 len;

	bool acked;
	/** for identify ack */
	int cmd_id;

	int (*parse)(struct usb_stub *stub, u8 cmd, u8 flags, u8 *data,
		     u32 len);
	int (*notify)(struct usb_stub *stub, long event, void *evt_data);
	void (*cleanup)(struct usb_stub *stub);
	void *priv;
};

int usb_stub_init(struct usb_interface *intf);
struct usb_stub *usb_stub_find(struct usb_interface *intf, u8 type);
int usb_stub_write(struct usb_stub *stub, u8 cmd, u8 *data, u8 len,
		   bool wait_ack, long timeout);
void usb_stub_broadcast(struct usb_interface *intf, long event,
			void *event_data);
void usb_stub_cleanup(struct usb_interface *intf);
struct usb_stub *usb_stub_alloc(struct usb_interface *intf);

struct acpi_device *find_adev_by_hid(struct acpi_device *parent,
				     const char *hid);
#endif
