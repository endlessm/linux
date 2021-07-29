// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include <linux/acpi.h>
#include <linux/bug.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/usb.h>

#include "i2c_stub.h"
#include "protocol_intel_ulpss.h"
#include "usb_stub.h"

#define USB_BRIDGE_I2C_HID_0 "INTC1075"
#define USB_BRIDGE_I2C_HID_1 "INTC1076"

static char *usb_bridge_i2c_hids[] = { USB_BRIDGE_I2C_HID_0,
				       USB_BRIDGE_I2C_HID_1 };

struct i2c_stub_priv {
	struct usb_stub *stub;
	struct i2c_descriptor descriptor;
	struct i2c_adapter *adap;

	bool ready;
};

static u8 i2c_format_slave_addr(u8 slave_addr, enum i2c_address_mode mode)
{
	if (mode == I2C_ADDRESS_MODE_7Bit)
		return slave_addr << 1;

	return 0xFF;
}

static void i2c_stub_cleanup(struct usb_stub *stub)
{
	struct i2c_stub_priv *priv = stub->priv;
	int i;

	if (!priv)
		return;

	if (priv->ready) {
		for (i = 0; i < priv->descriptor.num; i++)
			if (priv->adap[i].nr != -1)
				i2c_del_adapter(&priv->adap[i]);

		if (priv->adap)
			kfree(priv->adap);
	}

	kfree(priv);
}

static int i2c_stub_update_descriptor(struct usb_stub *stub,
				      struct i2c_descriptor *descriptor, u8 len)
{
	struct i2c_stub_priv *priv = stub->priv;

	if (!priv || !descriptor ||
	    len != offsetof(struct i2c_descriptor, info) +
			    descriptor->num *
				    sizeof(struct i2c_controller_info) ||
	    len > sizeof(priv->descriptor))
		return -EINVAL;

	memcpy(&priv->descriptor, descriptor, len);

	return 0;
}

static int i2c_stub_ready(struct usb_stub *stub, void *cookie, u8 len);
int i2c_stub_init(struct usb_interface *intf, void *cookie, u8 len)
{
	struct usb_stub *stub = usb_stub_alloc(intf);
	struct i2c_stub_priv *priv;

	if (!intf || !stub)
		return -EINVAL;

	priv = kzalloc(sizeof(struct i2c_stub_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	stub->priv = priv;
	stub->type = I2C_CMD_TYPE;
	stub->intf = intf;
	stub->cleanup = i2c_stub_cleanup;

	priv->stub = stub;

	return i2c_stub_ready(stub, cookie, len);
}

/** i2c intf */
static int intel_ulpss_i2c_init(struct usb_stub *stub, u8 id)
{
	struct i2c_raw_io *i2c_config;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	i2c_config = (struct i2c_raw_io *)buf;

	i2c_config->id = id;
	i2c_config->len = 1;
	i2c_config->data[0] = I2C_FLAG_FREQ_400K;

	ret = usb_stub_write(stub, I2C_INIT, (u8 *)i2c_config,
			     sizeof(struct i2c_raw_io) + i2c_config->len, true,
			     USB_WRITE_ACK_TIMEOUT);

	return ret;
}

static int intel_ulpss_i2c_start(struct usb_stub *stub, u8 id, u8 slave_addr,
				 enum xfer_type type)
{
	struct i2c_raw_io *raw_io;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	BUG_ON(!stub);

	raw_io = (struct i2c_raw_io *)buf;
	raw_io->id = id;
	raw_io->len = 1;
	raw_io->data[0] =
		i2c_format_slave_addr(slave_addr, I2C_ADDRESS_MODE_7Bit);
	raw_io->data[0] |= (type == READ_XFER_TYPE) ? I2C_SLAVE_TRANSFER_READ :
						      I2C_SLAVE_TRANSFER_WRITE;

	ret = usb_stub_write(stub, I2C_START, (u8 *)raw_io,
			     sizeof(struct i2c_raw_io) + raw_io->len, true,
			     USB_WRITE_ACK_TIMEOUT);

	if (stub->len < sizeof(struct i2c_raw_io))
		return -EIO;

	raw_io = (struct i2c_raw_io *)stub->buf;
	if (raw_io->len < 0 || raw_io->id != id) {
		dev_err(&stub->intf->dev,
			"%s i2c start failed len:%d id:%d %d\n", __func__,
			raw_io->len, raw_io->id, id);
		return -EIO;
	}

	return ret;
}

static int intel_ulpss_i2c_stop(struct usb_stub *stub, u8 id, u8 slave_addr)
{
	struct i2c_raw_io *raw_io;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	BUG_ON(!stub);

	raw_io = (struct i2c_raw_io *)buf;
	raw_io->id = id;
	raw_io->len = 1;
	raw_io->data[0] = 0;

	ret = usb_stub_write(stub, I2C_STOP, (u8 *)raw_io,
			     sizeof(struct i2c_raw_io) + 1, true,
			     USB_WRITE_ACK_TIMEOUT);

	if (stub->len < sizeof(struct i2c_raw_io))
		return -EIO;

	raw_io = (struct i2c_raw_io *)stub->buf;
	if (raw_io->len < 0 || raw_io->id != id) {
		dev_err(&stub->intf->dev,
			"%s i2c stop failed len:%d id:%d %d\n", __func__,
			raw_io->len, raw_io->id, id);
		return -EIO;
	}

	return ret;
}

static int intel_ulpss_i2c_pure_read(struct usb_stub *stub, u8 id, u8 *data,
				     u8 len)
{
	struct i2c_raw_io *raw_io;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	raw_io = (struct i2c_raw_io *)buf;

	BUG_ON(!stub);
	if (len > MAX_PAYLOAD_SIZE - sizeof(struct i2c_raw_io)) {
		return -EINVAL;
	}

	raw_io->id = id;
	raw_io->len = len;
	raw_io->data[0] = 0;
	ret = usb_stub_write(stub, I2C_READ, (u8 *)raw_io,
			     sizeof(struct i2c_raw_io) + 1, true,
			     USB_WRITE_ACK_TIMEOUT);
	if (ret) {
		dev_err(&stub->intf->dev, "%s ret:%d\n", __func__, ret);
		return ret;
	}

	if (stub->len < sizeof(struct i2c_raw_io))
		return -EIO;

	raw_io = (struct i2c_raw_io *)stub->buf;
	if (raw_io->len < 0 || raw_io->id != id) {
		dev_err(&stub->intf->dev,
			"%s i2 raw read failed len:%d id:%d %d\n", __func__,
			raw_io->len, raw_io->id, id);
		return -EIO;
	}

	BUG_ON(raw_io->len != len);
	memcpy(data, raw_io->data, raw_io->len);

	return 0;
}

static int intel_ulpss_i2c_read(struct usb_stub *stub, u8 id, u8 slave_addr,
				u8 *data, u8 len)
{
	int ret;

	BUG_ON(!stub);
	ret = intel_ulpss_i2c_start(stub, id, slave_addr, READ_XFER_TYPE);
	if (ret) {
		dev_err(&stub->intf->dev, "%s i2c start failed ret:%d\n",
			__func__, ret);
		return ret;
	}

	ret = intel_ulpss_i2c_pure_read(stub, id, data, len);
	if (ret) {
		dev_err(&stub->intf->dev, "%s i2c raw read failed ret:%d\n",
			__func__, ret);

		return ret;
	}

	ret = intel_ulpss_i2c_stop(stub, id, slave_addr);
	if (ret) {
		dev_err(&stub->intf->dev, "%s i2c stop failed ret:%d\n",
			__func__, ret);

		return ret;
	}

	return ret;
}

static int intel_ulpss_i2c_pure_write(struct usb_stub *stub, u8 id, u8 *data,
				      u8 len)
{
	struct i2c_raw_io *raw_io;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	if (len > MAX_PAYLOAD_SIZE - sizeof(struct i2c_raw_io)) {
		dev_err(&stub->intf->dev, "%s unexpected long data, len: %d",
			__func__, len);
		return -EINVAL;
	}

	raw_io = (struct i2c_raw_io *)buf;
	raw_io->id = id;
	raw_io->len = len;
	memcpy(raw_io->data, data, len);

	ret = usb_stub_write(stub, I2C_WRITE, (u8 *)raw_io,
			     sizeof(struct i2c_raw_io) + raw_io->len, true,
			     USB_WRITE_ACK_TIMEOUT);

	if (stub->len < sizeof(struct i2c_raw_io))
		return -EIO;

	raw_io = (struct i2c_raw_io *)stub->buf;
	if (raw_io->len < 0 || raw_io->id != id) {
		dev_err(&stub->intf->dev,
			"%s i2c write failed len:%d id:%d %d\n", __func__,
			raw_io->len, raw_io->id, id);
		return -EIO;
	}
	return ret;
}

static int intel_ulpss_i2c_write(struct usb_stub *stub, u8 id, u8 slave_addr,
				 u8 *data, u8 len)
{
	int ret;
	BUG_ON(!stub);

	ret = intel_ulpss_i2c_start(stub, id, slave_addr, WRITE_XFER_TYPE);
	if (ret)
		return ret;

	ret = intel_ulpss_i2c_pure_write(stub, id, data, len);
	if (ret)
		return ret;

	ret = intel_ulpss_i2c_stop(stub, id, slave_addr);

	return ret;
}

static int intel_ulpss_i2c_xfer(struct i2c_adapter *adapter,
				struct i2c_msg *msg, int num)
{
	struct i2c_stub_priv *priv;
	struct i2c_msg *cur_msg;
	struct usb_stub *stub;
	int id = -1;
	int i, ret;

	priv = i2c_get_adapdata(adapter);
	stub = priv->stub;

	if (!stub || !priv) {
		dev_err(&adapter->dev, "%s num:%d stub:0x%lx priv:0x%lx\n",
			__func__, num, (long)stub, (long)priv);
		return 0;
	}

	for (i = 0; i < priv->descriptor.num; i++)
		if (&priv->adap[i] == adapter)
			id = i;

	mutex_lock(&stub->stub_mutex);
	ret = intel_ulpss_i2c_init(stub, id);
	if (ret) {
		dev_err(&adapter->dev, "%s i2c init failed id:%d\n", __func__,
			adapter->nr);
		mutex_unlock(&stub->stub_mutex);
		return 0;
	}

	for (i = 0; !ret && i < num && id >= 0; i++) {
		cur_msg = &msg[i];
		dev_dbg(&adapter->dev, "%s i:%d id:%d msg:(%d %d)\n", __func__,
			i, id, cur_msg->flags, cur_msg->len);
		if (cur_msg->flags & I2C_M_RD)
			ret = intel_ulpss_i2c_read(priv->stub, id,
						   cur_msg->addr, cur_msg->buf,
						   cur_msg->len);

		else
			ret = intel_ulpss_i2c_write(priv->stub, id,
						    cur_msg->addr, cur_msg->buf,
						    cur_msg->len);
	}

	mutex_unlock(&stub->stub_mutex);
	dev_dbg(&adapter->dev, "%s id:%d ret:%d\n", __func__, id, ret);

	/* return the number of messages processed, or the error code */
	if (ret)
		return ret;
	return num;
}

static u32 intel_ulpss_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}
static const struct i2c_algorithm intel_ulpss_i2c_algo = {
	.master_xfer = intel_ulpss_i2c_xfer,
	.functionality = intel_ulpss_i2c_func,
};

static int intel_ulpss_i2c_adapter_register(struct usb_interface *intf,
					    struct i2c_adapter *adap, int index)
{
	struct acpi_device *adev;

	if (!adap || index < 0 || index > sizeof(usb_bridge_i2c_hids))
		return -EINVAL;

	adev = find_adev_by_hid(
		ACPI_COMPANION(&(interface_to_usbdev(intf)->dev)),
		usb_bridge_i2c_hids[index]);
	if (adev) {
		dev_info(&intf->dev, "Found: %s -> %s\n", dev_name(&intf->dev),
			 acpi_device_hid(adev));
		ACPI_COMPANION_SET(&adap->dev, adev);
	} else {
		dev_err(&intf->dev, "Not Found: %s\n",
			usb_bridge_i2c_hids[index]);
	}

	return i2c_add_adapter(adap);
}

static int intel_ulpss_i2c_adapter_setup(struct usb_interface *intf,
					 struct usb_stub *stub)
{
	struct i2c_stub_priv *priv;
	int ret;
	int i;

	if (!intf || !stub)
		return -EINVAL;

	priv = stub->priv;
	if (!priv)
		return -EINVAL;

	priv->adap = kzalloc(sizeof(struct i2c_adapter) * priv->descriptor.num,
			     GFP_KERNEL);

	for (i = 0; i < priv->descriptor.num; i++) {
		priv->adap[i].owner = THIS_MODULE;
		snprintf(priv->adap[i].name, sizeof(priv->adap[i].name),
			 "intel-ulpss-i2c-%d", i);
		priv->adap[i].algo = &intel_ulpss_i2c_algo;
		priv->adap[i].dev.parent = &intf->dev;

		i2c_set_adapdata(&priv->adap[i], priv);

		ret = intel_ulpss_i2c_adapter_register(intf, &priv->adap[i], i);
		if (ret)
			return ret;
	}

	priv->ready = true;
	return ret;
}

static int i2c_stub_ready(struct usb_stub *stub, void *cookie, u8 len)
{
	struct i2c_descriptor *descriptor = cookie;
	int ret;

	if (!descriptor || descriptor->num <= 0) {
		dev_err(&stub->intf->dev,
			"%s i2c stub descriptor not correct\n", __func__);
		return -EINVAL;
	}

	ret = i2c_stub_update_descriptor(stub, descriptor, len);
	if (ret) {
		dev_err(&stub->intf->dev,
			"%s i2c stub update descriptor failed\n", __func__);
		return ret;
	}

	ret = intel_ulpss_i2c_adapter_setup(stub->intf, stub);
	return ret;
}
