/*
 * Acer Wireless Radio Control Driver
 *
 * Copyright (C) 2017 Endless Mobile, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <linux/input.h>
#include <linux/pci_ids.h>

struct acer_wireless_data {
	struct input_dev *idev;
	struct acpi_device *adev;
};

static const struct acpi_device_id device_ids[] = {
	{"10251229", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, device_ids);

static void acer_wireless_notify(struct acpi_device *adev, u32 event)
{
	struct acer_wireless_data *data = acpi_driver_data(adev);

	dev_dbg(&adev->dev, "event=%#x\n", event);
	if (event != 0x80) {
		dev_notice(&adev->dev, "Unknown SMKB event: %#x\n", event);
		return;
	}
	input_report_key(data->idev, KEY_RFKILL, 1);
	input_sync(data->idev);
	input_report_key(data->idev, KEY_RFKILL, 0);
	input_sync(data->idev);
}

static int acer_wireless_add(struct acpi_device *adev)
{
	struct acer_wireless_data *data;

	data = devm_kzalloc(&adev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	adev->driver_data = data;
	data->adev = adev;

	data->idev = devm_input_allocate_device(&adev->dev);
	if (!data->idev)
		return -ENOMEM;
	data->idev->name = "Acer Wireless Radio Control";
	data->idev->phys = "acer-wireless/input0";
	data->idev->id.bustype = BUS_HOST;
	data->idev->id.vendor = PCI_VENDOR_ID_AI;
	set_bit(EV_KEY, data->idev->evbit);
	set_bit(KEY_RFKILL, data->idev->keybit);

	return input_register_device(data->idev);
}

static int acer_wireless_remove(struct acpi_device *adev)
{
	return 0;
}

static struct acpi_driver acer_wireless_driver = {
	.name = "Acer Wireless Radio Control Driver",
	.class = "hotkey",
	.ids = device_ids,
	.ops = {
		.add = acer_wireless_add,
		.remove = acer_wireless_remove,
		.notify = acer_wireless_notify,
	},
};
module_acpi_driver(acer_wireless_driver);

MODULE_DESCRIPTION("Acer Wireless Radio Control Driver");
MODULE_AUTHOR("Chris Chiu <chiu@gmail.com>");
MODULE_LICENSE("GPL");
