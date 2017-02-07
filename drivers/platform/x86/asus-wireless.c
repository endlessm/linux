/*
 * Asus Wireless Radio Control Driver
 *
 * Copyright (C) 2015-2016 Endless Mobile, Inc.
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
#include <linux/leds.h>

struct hswc_params {
	u8 on;
	u8 off;
	u8 status;
};

struct asus_wireless_data {
	struct input_dev *idev;
	struct acpi_device *adev;
	const struct hswc_params *hswc_params;
	struct workqueue_struct *wq;
	struct work_struct led_work;
	struct led_classdev led;
	int led_state;
};

static const struct hswc_params id_params[] = {
	{0x0, 0x1, 0x2},
	{0x5, 0x4, 0x2},
};

static const struct acpi_device_id device_ids[] = {
	{"ATK4001", (kernel_ulong_t)&id_params[0]},
	{"ATK4002", (kernel_ulong_t)&id_params[1]},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, device_ids);

static u64 asus_wireless_method(acpi_handle handle, const char *method,
				int param)
{
	struct acpi_object_list p;
	union acpi_object obj;
	acpi_status s;
	u64 ret;

	acpi_handle_debug(handle, "Evaluating method %s, parameter %#x\n",
			  method, param);
	obj.type = ACPI_TYPE_INTEGER;
	obj.integer.value = param;
	p.count = 1;
	p.pointer = &obj;

	s = acpi_evaluate_integer(handle, (acpi_string) method, &p, &ret);
	if (ACPI_FAILURE(s))
		acpi_handle_err(handle,
				"Failed to eval method %s, param %#x (%d)\n",
				method, param, s);
	acpi_handle_debug(handle, "%s returned %#x\n", method, (uint) ret);
	return ret;
}

static enum led_brightness led_state_get(struct led_classdev *led)
{
	struct asus_wireless_data *data;
	int s;

	data = container_of(led, struct asus_wireless_data, led);
	s = asus_wireless_method(acpi_device_handle(data->adev), "HSWC",
				 data->hswc_params->status);
	if (s == data->hswc_params->on)
		return LED_FULL;
	return LED_OFF;
}

static void led_state_update(struct work_struct *work)
{
	struct asus_wireless_data *data;

	data = container_of(work, struct asus_wireless_data, led_work);
	asus_wireless_method(acpi_device_handle(data->adev), "HSWC",
			     data->led_state);
}

static void led_state_set(struct led_classdev *led, enum led_brightness value)
{
	struct asus_wireless_data *data;

	data = container_of(led, struct asus_wireless_data, led);
	data->led_state = value == LED_OFF ? data->hswc_params->off :
					     data->hswc_params->on;
	queue_work(data->wq, &data->led_work);
}

static void asus_wireless_notify(struct acpi_device *adev, u32 event)
{
	struct asus_wireless_data *data = acpi_driver_data(adev);

	dev_dbg(&adev->dev, "event=%#x\n", event);
	if (event != 0x88) {
		dev_notice(&adev->dev, "Unknown ASHS event: %#x\n", event);
		return;
	}
	input_report_key(data->idev, KEY_RFKILL, 1);
	input_report_key(data->idev, KEY_RFKILL, 0);
	input_sync(data->idev);
}

static int asus_wireless_add(struct acpi_device *adev)
{
	struct asus_wireless_data *data;
	const struct acpi_device_id *id, *device_id = NULL;
	struct acpi_hardware_id *hwid;
	int err;

	data = devm_kzalloc(&adev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	adev->driver_data = data;
	data->adev = adev;

	data->idev = devm_input_allocate_device(&adev->dev);
	if (!data->idev)
		return -ENOMEM;
	data->idev->name = "Asus Wireless Radio Control";
	data->idev->phys = "asus-wireless/input0";
	data->idev->id.bustype = BUS_HOST;
	data->idev->id.vendor = PCI_VENDOR_ID_ASUSTEK;
	set_bit(EV_KEY, data->idev->evbit);
	set_bit(KEY_RFKILL, data->idev->keybit);
	err = input_register_device(data->idev);
	if (err)
		return err;

	list_for_each_entry(hwid, &adev->pnp.ids, list) {
		for (id = device_ids; id->id[0]; id++) {
			if (id->id[0] && !strcmp((char *) id->id, hwid->id)) {
				device_id = id;
				break;
			}
		}
	}
	if (!device_id)
		return 0;

	data->hswc_params = (const struct hswc_params *)device_id->driver_data;
	data->wq = create_singlethread_workqueue("asus_wireless_workqueue");
	if (!data->wq)
		return -ENOMEM;
	INIT_WORK(&data->led_work, led_state_update);
	data->led.name = "asus-wireless::airplane";
	data->led.brightness_set = led_state_set;
	data->led.brightness_get = led_state_get;
	data->led.flags = LED_CORE_SUSPENDRESUME;
	data->led.max_brightness = 1;
	data->led.default_trigger = "rfkill-airplane-mode";
	err = devm_led_classdev_register(&adev->dev, &data->led);
	if (err)
		destroy_workqueue(data->wq);

	return err;
}

static int asus_wireless_remove(struct acpi_device *adev)
{
	struct asus_wireless_data *data = acpi_driver_data(adev);

	if (data->wq)
		destroy_workqueue(data->wq);
	return 0;
}

static struct acpi_driver asus_wireless_driver = {
	.name = "Asus Wireless Radio Control Driver",
	.class = "hotkey",
	.ids = device_ids,
	.ops = {
		.add = asus_wireless_add,
		.remove = asus_wireless_remove,
		.notify = asus_wireless_notify,
	},
};
module_acpi_driver(asus_wireless_driver);

MODULE_DESCRIPTION("Asus Wireless Radio Control Driver");
MODULE_AUTHOR("João Paulo Rechi Vita <jprvita@gmail.com>");
MODULE_LICENSE("GPL");
