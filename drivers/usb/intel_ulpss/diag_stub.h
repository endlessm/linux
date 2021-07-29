/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __DIAG_STUB_H__
#define __DIAG_STUB_H__

#include "usb_stub.h"

int diag_set_trace_level(struct usb_stub *stub, u8 level);
int diag_get_statistic_info(struct usb_stub *stub);
int diag_stub_init(struct usb_interface *intf, void *cookie, u8 len);
int diag_get_state(struct usb_stub *stub);
int diag_get_fw_log(struct usb_stub *stub, u8 *buf, ssize_t *len);
int diag_get_coredump(struct usb_stub *stub, u8 *buf, ssize_t *len);
#endif
