// SPDX-License-Identifier: GPL-2.0-only
/*
 * Intel LPSS USB driver
 *
 * Copyright (c) 2020, Intel Corporation.
 */

#include <linux/acpi.h>
#include <linux/gpio/driver.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/usb.h>

#include "gpio_stub.h"
#include "protocol_intel_ulpss.h"
#include "usb_stub.h"

#define USB_BRIDGE_GPIO_HID "INTC1074"
struct pin_info {
	u8 bank_id;
	bool valid;
	u8 connect_mode;
};

struct gpio_stub_priv {
	u32 id;
	struct usb_stub *stub;

	/** gpio descriptor */
	struct gpio_descriptor descriptor;
	struct pin_info *pin_info_table;

	struct device dev;
	u32 valid_gpio_num;
	u32 total_gpio_num;
	struct gpio_chip gpio_chip;

	bool ready;
};

/** stub function */
static int gpio_stub_parse(struct usb_stub *stub, u8 cmd, u8 flags, u8 *data,
			   u32 len)
{
	if (!stub)
		return -EINVAL;

	if (cmd == GPIO_INTR_NOTIFY)
		if (stub->notify)
			stub->notify(stub, GPIO_INTR_EVENT, NULL);

	return 0;
}

static void gpio_stub_cleanup(struct usb_stub *stub)
{
	struct gpio_stub_priv *priv = stub->priv;

	if (!stub || !priv)
		return;

	dev_dbg(&stub->intf->dev, "%s unregister gpio dev\n", __func__);

	if (priv->ready) {
		gpiochip_remove(&priv->gpio_chip);
		device_unregister(&priv->dev);
	}

	if (priv->pin_info_table) {
		kfree(priv->pin_info_table);
		priv->pin_info_table = NULL;
	}
	kfree(priv);

	return;
}

static int gpio_stub_update_descriptor(struct usb_stub *stub,
				       struct gpio_descriptor *descriptor,
				       u8 len)
{
	struct gpio_stub_priv *priv = stub->priv;
	u32 i, j;
	int pin_id;

	if (!priv || !descriptor ||
	    len != offsetof(struct gpio_descriptor, bank_table) +
			    sizeof(struct bank_descriptor) *
				    descriptor->banks ||
	    len > sizeof(priv->descriptor))
		return -EINVAL;

	if ((descriptor->pins_per_bank <= 0) || (descriptor->banks <= 0)) {
		dev_err(&stub->intf->dev, "%s pins_per_bank:%d bans:%d\n",
			__func__, descriptor->pins_per_bank, descriptor->banks);
		return -EINVAL;
	}

	priv->total_gpio_num = descriptor->pins_per_bank * descriptor->banks;
	memcpy(&priv->descriptor, descriptor, len);

	priv->pin_info_table =
		kzalloc(sizeof(struct pin_info) * descriptor->pins_per_bank *
				descriptor->banks,
			GFP_KERNEL);
	if (!priv->pin_info_table)
		return -ENOMEM;

	for (i = 0; i < descriptor->banks; i++) {
		for (j = 0; j < descriptor->pins_per_bank; j++) {
			pin_id = descriptor->pins_per_bank * i + j;
			if ((descriptor->bank_table[i].bitmap & (1 << j))) {
				priv->pin_info_table[pin_id].valid = true;
				priv->valid_gpio_num++;
				dev_dbg(&stub->intf->dev,
					"%s found one valid pin (%d %d %d %d %d)\n",
					__func__, i, j,
					descriptor->bank_table[i].pin_num,
					descriptor->pins_per_bank,
					priv->valid_gpio_num);
			} else {
				priv->pin_info_table[pin_id].valid = false;
			}
			priv->pin_info_table[pin_id].bank_id = i;
		}
	}

	dev_dbg(&stub->intf->dev, "%s valid_gpio_num:%d total_gpio_num:%d\n",
		__func__, priv->valid_gpio_num, priv->total_gpio_num);
	return 0;
}

static struct pin_info *gpio_stub_get_pin_info(struct usb_stub *stub, u8 pin_id)
{
	struct pin_info *pin_info = NULL;
	struct gpio_stub_priv *priv = stub->priv;

	BUG_ON(!priv);

	if (!(pin_id <
	      priv->descriptor.banks * priv->descriptor.pins_per_bank)) {
		dev_err(&stub->intf->dev,
			"pin_id:%d banks:%d, pins_per_bank:%d\n", pin_id,
			priv->descriptor.banks, priv->descriptor.pins_per_bank);
		return NULL;
	}

	pin_info = &priv->pin_info_table[pin_id];
	if (!pin_info || !pin_info->valid) {
		dev_err(&stub->intf->dev,
			"%s pin_id:%d banks:%d, pins_per_bank:%d valid:%d",
			__func__, pin_id, priv->descriptor.banks,
			priv->descriptor.pins_per_bank, pin_info->valid);

		return NULL;
	}

	return pin_info;
}

static int gpio_stub_ready(struct usb_stub *stub, void *cookie, u8 len);
int gpio_stub_init(struct usb_interface *intf, void *cookie, u8 len)
{
	struct usb_stub *stub = usb_stub_alloc(intf);
	struct gpio_stub_priv *priv;

	if (!intf || !stub)
		return -EINVAL;

	priv = kzalloc(sizeof(struct gpio_stub_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->stub = stub;
	priv->id = DEFAULT_GPIO_CONTROLLER_ID;

	stub->priv = priv;
	stub->type = GPIO_CMD_TYPE;
	stub->intf = intf;
	stub->parse = gpio_stub_parse;
	stub->cleanup = gpio_stub_cleanup;

	return gpio_stub_ready(stub, cookie, len);
}

/** gpio function */
static u8 gpio_get_payload_len(u8 pin_num)
{
	return sizeof(struct gpio_packet) + pin_num * sizeof(struct gpio_op);
}

static int gpio_config(struct usb_stub *stub, u8 pin_id, u8 config)
{
	struct gpio_packet *packet;
	struct pin_info *pin_info;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };

	if (!stub)
		return -EINVAL;

	dev_dbg(&stub->intf->dev, "%s pin_id:%u\n", __func__, pin_id);

	packet = (struct gpio_packet *)buf;

	pin_info = gpio_stub_get_pin_info(stub, pin_id);
	if (!pin_info) {
		dev_err(&stub->intf->dev, "invalid gpio pin pin_id:%d\n",
			pin_id);
		return -EINVAL;
	}

	packet->item[0].index = pin_id;
	packet->item[0].value = config | pin_info->connect_mode;
	packet->num = 1;

	return usb_stub_write(stub, GPIO_CONFIG, (u8 *)packet,
			      (u8)gpio_get_payload_len(packet->num), true,
			      USB_WRITE_ACK_TIMEOUT);
}

static int intel_ulpss_gpio_read(struct usb_stub *stub, u8 pin_id, int *data)
{
	struct gpio_packet *packet;
	struct pin_info *pin_info;
	struct gpio_packet *ack_packet;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };
	int ret;

	if (!stub)
		return -EINVAL;

	packet = (struct gpio_packet *)buf;
	packet->num = 1;

	pin_info = gpio_stub_get_pin_info(stub, pin_id);
	if (!pin_info) {
		dev_err(&stub->intf->dev, "invalid gpio pin_id:[%u]", pin_id);
		return -EINVAL;
	}

	packet->item[0].index = pin_id;
	ret = usb_stub_write(stub, GPIO_READ, (u8 *)packet,
			     (u8)gpio_get_payload_len(packet->num), true,
			     USB_WRITE_ACK_TIMEOUT);

	ack_packet = (struct gpio_packet *)stub->buf;

	BUG_ON(!ack_packet);
	if (ret || !stub->len || ack_packet->num != packet->num) {
		dev_err(&stub->intf->dev,
			"%s usb_stub_write failed pin_id:%d ret %d", __func__,
			pin_id, ret);
		return -EIO;
	}

	*data = (ack_packet->item[0].value > 0) ? 1 : 0;

	return ret;
}

static int intel_ulpss_gpio_write(struct usb_stub *stub, u8 pin_id, int value)
{
	struct gpio_packet *packet;
	u8 buf[MAX_PAYLOAD_SIZE] = { 0 };

	BUG_ON(!stub);

	packet = (struct gpio_packet *)buf;
	packet->num = 1;

	packet->item[0].index = pin_id;
	packet->item[0].value = (value & 1);

	return usb_stub_write(stub, GPIO_WRITE, buf,
			      gpio_get_payload_len(packet->num), true,
			      USB_WRITE_ACK_TIMEOUT);
}

/* gpio chip*/
static int intel_ulpss_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct gpio_stub_priv *priv = gpiochip_get_data(chip);
	int value = 0;
	int ret;

	dev_dbg(chip->parent, "%s off:%u\n", __func__, off);
	ret = intel_ulpss_gpio_read(priv->stub, off, &value);
	if (ret) {
		dev_err(chip->parent, "%s off:%d get vaule failed %d\n",
			__func__, off, ret);
	}
	return value;
}

static void intel_ulpss_gpio_set_value(struct gpio_chip *chip, unsigned off, int val)
{
	struct gpio_stub_priv *priv = gpiochip_get_data(chip);
	int ret;

	dev_dbg(chip->parent, "%s off:%u val:%d\n", __func__, off, val);
	ret = intel_ulpss_gpio_write(priv->stub, off, val);
	if (ret) {
		dev_err(chip->parent, "%s off:%d val:%d set vaule failed %d\n",
			__func__, off, val, ret);
	}
}

static int intel_ulpss_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	struct gpio_stub_priv *priv = gpiochip_get_data(chip);
	u8 config = GPIO_CONF_INPUT | GPIO_CONF_CLR;

	dev_dbg(chip->parent, "%s off:%u\n", __func__, off);
	return gpio_config(priv->stub, off, config);
}

static int intel_ulpss_gpio_direction_output(struct gpio_chip *chip, unsigned off,
				     int val)
{
	struct gpio_stub_priv *priv = gpiochip_get_data(chip);
	u8 config = GPIO_CONF_OUTPUT | GPIO_CONF_CLR;
	int ret;

	dev_dbg(chip->parent, "%s off:%u\n", __func__, off);
	ret = gpio_config(priv->stub, off, config);
	if (ret)
		return ret;

	intel_ulpss_gpio_set_value(chip, off, val);
	return ret;
}

static int intel_ulpss_gpio_set_config(struct gpio_chip *chip, unsigned int off,
			       unsigned long config)
{
	struct gpio_stub_priv *priv = gpiochip_get_data(chip);
	struct pin_info *pin_info;

	dev_dbg(chip->parent, "%s off:%d\n", __func__, off);

	pin_info = gpio_stub_get_pin_info(priv->stub, off);
	if (!pin_info) {
		dev_err(chip->parent, "invalid gpio pin off:%d pin_id:%d\n",
			off, off);

		return -EINVAL;
	}

	dev_dbg(chip->parent, " %s off:%d config:%d\n", __func__, off,
		pinconf_to_config_param(config));

	pin_info->connect_mode = 0;
	switch (pinconf_to_config_param(config)) {
	case PIN_CONFIG_BIAS_PULL_UP:
		pin_info->connect_mode |= GPIO_CONF_PULLUP;
		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		pin_info->connect_mode |= GPIO_CONF_PULLDOWN;
		break;
	case PIN_CONFIG_DRIVE_PUSH_PULL:
		break;
	default:
		return -ENOTSUPP;
	}

	dev_dbg(chip->parent, " %s off:%d connect_mode:%d\n", __func__, off,
		pin_info->connect_mode);
	return 0;
}

static void gpio_dev_release(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
}

static int intel_ulpss_gpio_chip_setup(struct usb_interface *intf,
			       struct usb_stub *stub)
{
	struct gpio_stub_priv *priv;
	struct gpio_chip *gc;
	struct acpi_device *adev;
	int ret;

	priv = stub->priv;
	priv->dev.parent = &intf->dev;
	priv->dev.init_name = "intel-ulpss-gpio";
	priv->dev.release = gpio_dev_release;
	adev = find_adev_by_hid(
		ACPI_COMPANION(&(interface_to_usbdev(intf)->dev)),
		USB_BRIDGE_GPIO_HID);
	if (adev) {
		ACPI_COMPANION_SET(&priv->dev, adev);
		dev_info(&intf->dev, "found: %s -> %s\n", dev_name(&intf->dev),
			 acpi_device_hid(adev));
	} else {
		dev_err(&intf->dev, "not found: %s\n", USB_BRIDGE_GPIO_HID);
	}

	ret = device_register(&priv->dev);
	if (ret) {
		dev_err(&intf->dev, "device register failed\n");
		device_unregister(&priv->dev);

		return ret;
	}

	gc = &priv->gpio_chip;
	gc->direction_input = intel_ulpss_gpio_direction_input;
	gc->direction_output = intel_ulpss_gpio_direction_output;
	gc->get = intel_ulpss_gpio_get_value;
	gc->set = intel_ulpss_gpio_set_value;
	gc->set_config = intel_ulpss_gpio_set_config;
	gc->can_sleep = true;
	gc->parent = &priv->dev;

	gc->base = -1;
	gc->ngpio = priv->total_gpio_num;
	gc->label = "intel_ulpss gpiochip";
	gc->owner = THIS_MODULE;

	ret = gpiochip_add_data(gc, priv);
	if (ret) {
		dev_err(&intf->dev, "%s gpiochip add failed ret:%d\n", __func__,
			ret);
	} else {
		priv->ready = true;
		dev_info(&intf->dev,
			 "%s gpiochip add success, base:%d ngpio:%d\n",
			 __func__, gc->base, gc->ngpio);
	}

	return ret;
}

static int gpio_stub_ready(struct usb_stub *stub, void *cookie, u8 len)
{
	struct gpio_descriptor *descriptor = cookie;
	int ret;

	if (!descriptor || (descriptor->pins_per_bank <= 0) ||
	    (descriptor->banks <= 0)) {
		dev_err(&stub->intf->dev,
			"%s gpio stub descriptor not correct\n", __func__);
		return -EINVAL;
	}

	ret = gpio_stub_update_descriptor(stub, descriptor, len);
	if (ret) {
		dev_err(&stub->intf->dev,
			"%s gpio stub update descriptor failed\n", __func__);
		return ret;
	}

	ret = intel_ulpss_gpio_chip_setup(stub->intf, stub);

	return ret;
}
