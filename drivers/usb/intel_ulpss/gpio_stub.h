/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __GPIO_STUB_H__
#define __GPIO_STUB_H__

int gpio_stub_init(struct usb_interface *intf, void *cookie, u8 len);

#endif
