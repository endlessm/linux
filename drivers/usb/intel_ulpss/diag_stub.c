// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include <linux/types.h>

#include "diag_stub.h"

struct diag_stub_priv {
	struct usb_stub *stub;
};

int diag_get_state(struct usb_stub *stub)
{
	if (!stub)
		return -EINVAL;

	return usb_stub_write(stub, DIAG_GET_STATE, NULL, 0, true,
			      USB_WRITE_ACK_TIMEOUT);
}

int diag_get_fw_log(struct usb_stub *stub, u8 *buf, ssize_t *len)
{
	int ret;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, DIAG_GET_FW_LOG, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);

	*len = stub->len;
	if (!ret && stub->len > 0)
		memcpy(buf, stub->buf, stub->len);

	mutex_unlock(&stub->stub_mutex);
	return ret;
}

int diag_get_coredump(struct usb_stub *stub, u8 *buf, ssize_t *len)
{
	int ret;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, DIAG_GET_FW_COREDUMP, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);

	*len = stub->len;
	if (!ret && !stub->len)
		memcpy(buf, stub->buf, stub->len);

	mutex_unlock(&stub->stub_mutex);

	return ret;
}

int diag_get_statistic_info(struct usb_stub *stub)
{
	int ret;

	if (stub == NULL)
		return -EINVAL;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, DIAG_GET_STATISTIC, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);
	mutex_unlock(&stub->stub_mutex);

	return ret;
}

int diag_set_trace_level(struct usb_stub *stub, u8 level)
{
	int ret;

	if (stub == NULL)
		return -EINVAL;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, DIAG_SET_TRACE_LEVEL, &level, sizeof(level),
			     true, USB_WRITE_ACK_TIMEOUT);
	mutex_unlock(&stub->stub_mutex);

	return ret;
}

static void diag_stub_cleanup(struct usb_stub *stub)
{
	BUG_ON(!stub);
	if (stub->priv)
		kfree(stub->priv);

	return;
}

int diag_stub_init(struct usb_interface *intf, void *cookie, u8 len)
{
	struct usb_stub *stub = usb_stub_alloc(intf);
	struct diag_stub_priv *priv;

	if (!intf || !stub)
		return -EINVAL;

	priv = kzalloc(sizeof(struct diag_stub_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->stub = stub;

	stub->priv = priv;
	stub->type = DIAG_CMD_TYPE;
	stub->intf = intf;
	stub->cleanup = diag_stub_cleanup;
	return 0;
}
