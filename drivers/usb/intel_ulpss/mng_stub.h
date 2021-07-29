
/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __MNG_STUB_H__
#define __MNG_STUB_H__
#include "usb_stub.h"

int mng_stub_init(struct usb_interface *intf, void *cookie, u8 len);
int mng_get_version_string(struct usb_stub *stub, char *buf);
int mng_set_dfu_mode(struct usb_stub *stub);
int mng_stub_link(struct usb_interface *intf, struct usb_stub *mng_stub);
int mng_reset(struct usb_stub *stub);
#endif
