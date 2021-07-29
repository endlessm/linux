/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#ifndef __I2C_STUB_H__
#define __I2C_STUB_H__

#include <linux/types.h>

enum i2c_address_mode { I2C_ADDRESS_MODE_7Bit, I2C_ADDRESS_MODE_10Bit };
enum xfer_type {
	READ_XFER_TYPE,
	WRITE_XFER_TYPE,
};

int i2c_stub_init(struct usb_interface *intf, void *cookie, u8 len);

#endif
