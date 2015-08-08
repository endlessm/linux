/*
 * Asus Wireless Radio Control Driver
 *
 * Copyright (C) 2015 Endless Mobile, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>

#define ASUS_WRC_MODULE_NAME "Asus Wireless Radio Control Driver"

struct asus_wrc_data {
	struct input_dev *inputdev;
};

static const struct key_entry asus_wrc_keymap[] = {
	{ KE_KEY, 0x88, { KEY_RFKILL } },
	{ KE_END, 0 }
};

static void asus_wrc_notify(struct acpi_device *device, u32 event)
{
	struct asus_wrc_data *data = acpi_driver_data(device);

	pr_debug("event=0x%X\n", event);

	if (!sparse_keymap_report_event(data->inputdev, event, 1, true))
		pr_info("Unknown ASHS event: 0x%X\n", event);
}

static int asus_wrc_add(struct acpi_device *device)
{
	struct asus_wrc_data *data;
	int err = -ENOMEM;

	pr_info(ASUS_WRC_MODULE_NAME"\n");

	data = kzalloc(sizeof(struct asus_wrc_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->inputdev = input_allocate_device();
	if (!data->inputdev)
		goto free_driver_data;

	data->inputdev->name = "Asus Wireless Radio Control";
	data->inputdev->phys = "asus-wrc/input0";
	data->inputdev->id.bustype = BUS_HOST;
	data->inputdev->dev.parent = &device->dev;
	set_bit(EV_REP, data->inputdev->evbit);

	err = sparse_keymap_setup(data->inputdev, asus_wrc_keymap, NULL);
	if (err)
		goto free_inputdev;

	err = input_register_device(data->inputdev);
	if (err)
		goto free_keymap;

	device->driver_data = data;
	return 0;

free_keymap:
	sparse_keymap_free(data->inputdev);
free_inputdev:
	input_free_device(data->inputdev);
free_driver_data:
	kfree(data);

	return err;
}

static int asus_wrc_remove(struct acpi_device *device)
{
	struct asus_wrc_data *data = acpi_driver_data(device);

	pr_info("Removing "ASUS_WRC_MODULE_NAME"\n");

	if (data) {
		if (data->inputdev) {
			sparse_keymap_free(data->inputdev);
			input_unregister_device(data->inputdev);
			data->inputdev = NULL;
		}

		kfree(data);
		data = NULL;
	}
	return 0;
}

static const struct acpi_device_id device_ids[] = {
	{"ATK4002", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, device_ids);

static struct acpi_driver asus_wrc_driver = {
	.name = ASUS_WRC_MODULE_NAME,
	.class = "hotkey",
	.ids = device_ids,
	.ops = {
		.add = asus_wrc_add,
		.remove = asus_wrc_remove,
		.notify = asus_wrc_notify,
	},
};
module_acpi_driver(asus_wrc_driver);

MODULE_DESCRIPTION(ASUS_WRC_MODULE_NAME);
MODULE_AUTHOR("João Paulo Rechi Vita <jprvita@gmail.com>");
MODULE_LICENSE("GPL");
