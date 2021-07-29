// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include "ulpss_bridge.h"
#include "mng_stub.h"
#include "protocol_intel_ulpss.h"
#include "usb_stub.h"
#include "i2c_stub.h"
#include "gpio_stub.h"

struct mng_stub_priv {
	long reset_id;
	struct usb_stub *stub;
	bool synced;
};

static void mng_cleanup(struct usb_stub *stub)
{
	struct mng_stub_priv *priv = stub->priv;

	if (priv)
		kfree(priv);
}

static int mng_reset_ack(struct usb_stub *stub, u32 reset_id)
{
	return usb_stub_write(stub, MNG_RESET_NOTIFY, (u8 *)&reset_id,
			      (u8)sizeof(reset_id), false,
			      USB_WRITE_ACK_TIMEOUT);
}

static int mng_stub_parse(struct usb_stub *stub, u8 cmd, u8 ack, u8 *data,
			  u32 len)
{
	struct mng_stub_priv *priv = stub->priv;
	int ret = 0;

	if (!stub || !stub->intf)
		return -EINVAL;

	switch (cmd) {
	case MNG_RESET_NOTIFY:
		if (data && (len >= sizeof(u32))) {
			u32 reset_id = *(u32 *)data;

			if (!ack)
				ret = mng_reset_ack(stub, reset_id);

			priv->synced = (!ret) ? true : false;
		}
		break;
	default:
		break;
	}

	return ret;
}

int mng_stub_init(struct usb_interface *intf, void *cookie, u8 len)
{
	struct usb_stub *stub = usb_stub_alloc(intf);
	struct mng_stub_priv *priv;

	if (!intf || !stub)
		return -EINVAL;

	priv = kzalloc(sizeof(struct mng_stub_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->reset_id = 0;
	priv->stub = stub;

	stub->priv = priv;
	stub->type = MNG_CMD_TYPE;
	stub->intf = intf;
	stub->parse = mng_stub_parse;
	stub->cleanup = mng_cleanup;

	return 0;
}

static int mng_reset_handshake(struct usb_stub *stub)
{
	int ret;
	struct mng_stub_priv *priv = stub->priv;
	long reset_id;

	if (!stub)
		return -EINVAL;

	reset_id = priv->reset_id++;
	ret = usb_stub_write(stub, MNG_RESET_NOTIFY, (u8 *)&reset_id,
			     (u8)sizeof(reset_id), true, USB_WRITE_ACK_TIMEOUT);

	if (ret || !priv->synced) {
		dev_err(&stub->intf->dev, "%s priv->synced:%d ret:%d\n",
			__func__, priv->synced, ret);
		return -EIO;
	}

	return 0;
}

int mng_reset(struct usb_stub *stub)
{
	if (!stub)
		return -EINVAL;

	return usb_stub_write(stub, MNG_RESET, NULL, 0, true,
			      USB_WRITE_ACK_TIMEOUT);
}

static int mng_enum_gpio(struct usb_stub *stub)
{
	int ret;

	if (!stub)
		return -EINVAL;

	ret = usb_stub_write(stub, MNG_ENUM_GPIO, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);
	if (ret || stub->len <= 0) {
		dev_err(&stub->intf->dev, "%s enum gpio failed ret:%d len:%d\n",
			__func__, ret, stub->len);
		return ret;
	}

	return gpio_stub_init(stub->intf, stub->buf, stub->len);
}

static int mng_enum_i2c(struct usb_stub *stub)
{
	int ret;

	if (!stub)
		return -EINVAL;

	ret = usb_stub_write(stub, MNG_ENUM_I2C, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);
	if (ret || stub->len <= 0) {
		dev_err(&stub->intf->dev, "%s enum gpio failed ret:%d len:%d\n",
			__func__, ret, stub->len);
		ret = -EIO;
		return ret;
	}

	ret = i2c_stub_init(stub->intf, stub->buf, stub->len);
	return ret;
}

int mng_get_version(struct usb_stub *stub, struct fw_version *version)
{
	int ret;

	if (!stub || !version)
		return -EINVAL;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, MNG_GET_VERSION, NULL, 0, true,
			     USB_WRITE_ACK_TIMEOUT);
	if (ret || stub->len < sizeof(struct fw_version)) {
		mutex_unlock(&stub->stub_mutex);
		dev_err(&stub->intf->dev,
			"%s MNG_GET_VERSION failed ret:%d len:%d\n", __func__,
			ret, stub->len);
		ret = -EIO;
		return ret;
	}

	memcpy(version, stub->buf, sizeof(struct fw_version));
	mutex_unlock(&stub->stub_mutex);

	return 0;
}

int mng_get_version_string(struct usb_stub *stub, char *buf)
{
	int ret;
	struct fw_version version;
	if (!buf)
		return -EINVAL;

	ret = mng_get_version(stub, &version);
	if (ret) {
		dev_err(&stub->intf->dev, "%s mng get fw version failed ret:%d",
			__func__, ret);

		ret = sprintf(buf, "%d.%d.%d.%d\n", 1, 1, 1, 1);
		return ret;
	}

	ret = sprintf(buf, "%d.%d.%d.%d\n", version.major, version.minor,
		      version.patch, version.build);

	return ret;
}

int mng_set_dfu_mode(struct usb_stub *stub)
{
	int ret;
	struct mng_stub_priv *priv = NULL;
	if (!stub)
		return -EINVAL;

	priv = stub->priv;

	mutex_lock(&stub->stub_mutex);
	ret = usb_stub_write(stub, MNG_SET_DFU_MODE, NULL, 0, false,
			     USB_WRITE_ACK_TIMEOUT);
	mutex_unlock(&stub->stub_mutex);

	return ret;
}

int mng_stub_link(struct usb_interface *intf, struct usb_stub *mng_stub)
{
	int ret;
	struct usb_bridge *intel_ulpss_dev = usb_get_intfdata(intf);

	BUG_ON(!intel_ulpss_dev || !mng_stub);

	ret = mng_reset_handshake(mng_stub);
	if (ret)
		return ret;
	intel_ulpss_dev->state = USB_BRIDGE_RESET_SYNCED;

	ret = mng_enum_gpio(mng_stub);
	if (ret)
		return ret;
	intel_ulpss_dev->state = USB_BRIDGE_ENUM_GPIO_COMPLETE;

	ret = mng_enum_i2c(mng_stub);
	if (ret)
		return ret;
	intel_ulpss_dev->state = USB_BRIDGE_ENUM_I2C_COMPLETE;
	intel_ulpss_dev->state = USB_BRIDGE_STARTED;

	return ret;
}
