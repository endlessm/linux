/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __ULPSS_BRIDGE_H__
#define __ULPSS_BRIDGE_H__

#include <linux/kfifo.h>
#include <linux/types.h>
#include <linux/usb.h>
#include "usb_stub.h"

struct bridge_msg {
	size_t len;
	u8 buf[MAX_PACKET_SIZE];
	unsigned long read_time;
};

enum USB_BRIDGE_STATE {
	USB_BRIDGE_STOPPED = 0,
	USB_BRIDGE_INITED,
	USB_BRIDGE_START_SYNCING,
	USB_BRIDGE_START_DISPATCH_STARTED,
	USB_BRIDGE_START_READ_STARTED,
	USB_BRIDGE_RESET_HANDSHAKE,
	USB_BRIDGE_RESET_SYNCED,
	USB_BRIDGE_ENUM_GPIO_PENDING,
	USB_BRIDGE_ENUM_GPIO_COMPLETE,
	USB_BRIDGE_ENUM_I2C_PENDING,
	USB_BRIDGE_ENUM_I2C_COMPLETE,
	USB_BRIDGE_STARTED,
	USB_BRIDGE_FAILED,
};

struct usb_bridge {
	struct usb_device *udev;
	struct usb_interface *intf;
	u8 bulk_in_endpointAddr; /* the address of the bulk in endpoint */
	u8 bulk_out_endpointAddr; /* the address of the bulk out endpoint */
	/* in case we need to retract our submissions */
	struct usb_anchor write_submitted;

	/* the urb to read data with */
	struct urb *bulk_in_urb;
	/* the buffer to receive data */
	unsigned char *bulk_in_buffer;
	size_t bulk_in_size;

	/* bridge status */
	int errors; /* the last request tanked */
	unsigned int disconnected;
	int state;

	struct mutex write_mutex;

	/* stub */
	size_t stub_count;
	struct list_head stubs_list;

	/* buffers to store bridge msg temporary */
	DECLARE_KFIFO(msg_fifo, struct bridge_msg, 16);
	spinlock_t msg_fifo_spinlock;

	/* dispatch message from fw */
	struct work_struct event_work;

	/* to wait for an ongoing write ack */
	wait_queue_head_t bulk_out_ack;
};

ssize_t intel_ulpss_bridge_write(struct usb_interface *intf, void *data, size_t len,
		     unsigned int timeout);

#endif /* __ULPSS_BRIDGE_H__ */
