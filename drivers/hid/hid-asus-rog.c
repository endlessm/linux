/*
 *  HID driver for some Asus Gaming model equipped with "special" macrokey
 *  devices for hotkey handling
 *
 *  Currently supported devices are:
 *    ASUS laptops for "Republic of Gamers"
 *    GL553VD/GL553VE
 *    GL753VD/GL753VE
 *
 *  Copyright (c) 2016 Chris Chiu <chiu@endlessm.com>
 *
 *  This module based on hid-gyration
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/device.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/module.h>

#include "hid-ids.h"

#define FEATURE_KBD_REPORT_ID 0x5a
#define FEATURE_KBD_REPORT_SIZE 16

#define SUPPORT_KBD_BACKLIGHT BIT(0)

#define QUIRK_USE_KBD_BACKLIGHT		BIT(0)

struct asus_kbd_leds {
	struct led_classdev cdev;
	struct hid_device *hdev;
	struct work_struct work;
	unsigned int brightness;
	bool removed;
};

struct asus_drvdata {
	unsigned long quirks;
	struct asus_kbd_leds *kbd_backlight;
	bool enable_backlight;
};

#define asus_rog_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, \
						max, EV_KEY, (c))
static int asus_rog_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	struct asus_drvdata *drvdata = hid_get_drvdata(hdev);

	if ((usage->hid & HID_USAGE_PAGE) != HID_UP_ASUS_ROG_HOTKEY
		&& (usage->hid & HID_USAGE_PAGE) != HID_UP_MSVENDOR)
		return 0;

	if ((usage->hid & HID_USAGE_PAGE) == HID_UP_ASUS_ROG_HOTKEY) {
		set_bit(EV_REP, hi->input->evbit);
		switch (usage->hid & HID_USAGE) {
		/* Reported on ASUS Republic of Gamers keyboards */
		case 0x6c: asus_rog_map_key_clear(KEY_SLEEP);		break;
		case 0x88: asus_rog_map_key_clear(KEY_WLAN);		break;
		case 0xc5: asus_rog_map_key_clear(KEY_KBDILLUMDOWN);	break;
		case 0xc4: asus_rog_map_key_clear(KEY_KBDILLUMUP);	break;
		case 0x10: asus_rog_map_key_clear(KEY_BRIGHTNESSDOWN);	break;
		case 0x20: asus_rog_map_key_clear(KEY_BRIGHTNESSUP);	break;
		case 0x35: asus_rog_map_key_clear(KEY_DISPLAY_OFF);	break;
		// KEY_F21 is for ASUS touchpad toggle
		case 0x6b: asus_rog_map_key_clear(KEY_F21);             break;
		case 0x82: asus_rog_map_key_clear(KEY_CAMERA);          break;
		case 0xb5: asus_rog_map_key_clear(KEY_CALC);            break;
		// KEY_PROG1 for ROG key
		case 0x38: asus_rog_map_key_clear(KEY_PROG1);           break;
		// KEY_PROG2 for Fn+C ASUS Splendid
		case 0xba: asus_rog_map_key_clear(KEY_PROG2);           break;
		// KEY_PROG3 for Fn+Space Power4Gear Hybrid, may not be present
		case 0x5c: asus_rog_map_key_clear(KEY_PROG3);           break;

		default:
			return 0;
		}

		/*
		 * Check and enable backlight only on devices with UsagePage ==
		 * 0xff31 to avoid initializing the keyboard firmware multiple
		 * times on devices with multiple HID descriptors but same
		 * PID/VID.
		 */
		if (drvdata->quirks & QUIRK_USE_KBD_BACKLIGHT)
			drvdata->enable_backlight = true;
	}

	// For ASUS ZEN AIO customized wired/wireless keyboards
	if ((usage->hid & HID_USAGE_PAGE) == HID_UP_MSVENDOR) {
		set_bit(EV_REP, hi->input->evbit);
		switch (usage->hid & HID_USAGE) {
		case 0xf1:
			asus_rog_map_key_clear(KEY_WLAN);
			break;
		case 0xf2:
			asus_rog_map_key_clear(KEY_BRIGHTNESSDOWN);
			break;
		case 0xf3:
			asus_rog_map_key_clear(KEY_BRIGHTNESSUP);
			break;
		default:
			return 0;
		}
	}

	return 1;
}

static int asus_kbd_set_report(struct hid_device *hdev, u8 *buf, size_t buf_size)
{
	unsigned char *dmabuf;
	int ret;

	dmabuf = kmemdup(buf, buf_size, GFP_KERNEL);
	if (!dmabuf)
		return -ENOMEM;

	ret = hid_hw_raw_request(hdev, FEATURE_KBD_REPORT_ID, dmabuf,
				 buf_size, HID_FEATURE_REPORT,
				 HID_REQ_SET_REPORT);
	kfree(dmabuf);

	return ret;
}

static int asus_kbd_init(struct hid_device *hdev)
{
	u8 buf[] = { FEATURE_KBD_REPORT_ID, 0x41, 0x53, 0x55, 0x53, 0x20, 0x54,
		     0x65, 0x63, 0x68, 0x2e, 0x49, 0x6e, 0x63, 0x2e, 0x00 };
	int ret;

	ret = asus_kbd_set_report(hdev, buf, sizeof(buf));
	if (ret < 0)
		hid_err(hdev, "Asus failed to send init command: %d\n", ret);

	return ret;
}

static int asus_kbd_get_functions(struct hid_device *hdev,
				  unsigned char *kbd_func)
{
	u8 buf[] = { FEATURE_KBD_REPORT_ID, 0x05, 0x20, 0x31, 0x00, 0x08 };
	u8 *readbuf;
	int ret;

	ret = asus_kbd_set_report(hdev, buf, sizeof(buf));
	if (ret < 0) {
		hid_err(hdev, "Asus failed to send configuration command: %d\n", ret);
		return ret;
	}

	readbuf = kzalloc(FEATURE_KBD_REPORT_SIZE, GFP_KERNEL);
	if (!readbuf)
		return -ENOMEM;

	ret = hid_hw_raw_request(hdev, FEATURE_KBD_REPORT_ID, readbuf,
				 FEATURE_KBD_REPORT_SIZE, HID_FEATURE_REPORT,
				 HID_REQ_GET_REPORT);
	if (ret < 0) {
		hid_err(hdev, "Asus failed to request functions: %d\n", ret);
		kfree(readbuf);
		return ret;
	}

	*kbd_func = readbuf[6];

	kfree(readbuf);
	return ret;
}

static void asus_kbd_backlight_set(struct led_classdev *led_cdev,
				   enum led_brightness brightness)
{
	struct asus_kbd_leds *led = container_of(led_cdev, struct asus_kbd_leds,
						 cdev);
	if (led->brightness == brightness)
		return;

	led->brightness = brightness;
	schedule_work(&led->work);
}

static enum led_brightness asus_kbd_backlight_get(struct led_classdev *led_cdev)
{
	struct asus_kbd_leds *led = container_of(led_cdev, struct asus_kbd_leds,
						 cdev);

	return led->brightness;
}

static void asus_kbd_backlight_work(struct work_struct *work)
{
	struct asus_kbd_leds *led = container_of(work, struct asus_kbd_leds, work);
	u8 buf[] = { FEATURE_KBD_REPORT_ID, 0xba, 0xc5, 0xc4, 0x00 };
	int ret;

	if (led->removed)
		return;

	buf[4] = led->brightness;

	ret = asus_kbd_set_report(led->hdev, buf, sizeof(buf));
	if (ret < 0)
		hid_err(led->hdev, "Asus failed to set keyboard backlight: %d\n", ret);
}

static int asus_kbd_register_leds(struct hid_device *hdev)
{
	struct asus_drvdata *drvdata = hid_get_drvdata(hdev);
	unsigned char kbd_func;
	int ret;

	/* Initialize keyboard */
	ret = asus_kbd_init(hdev);
	if (ret < 0)
		return ret;

	/* Get keyboard functions */
	ret = asus_kbd_get_functions(hdev, &kbd_func);
	if (ret < 0)
		return ret;

	/* Check for backlight support */
	if (!(kbd_func & SUPPORT_KBD_BACKLIGHT))
		return -ENODEV;

	drvdata->kbd_backlight = devm_kzalloc(&hdev->dev,
					      sizeof(struct asus_kbd_leds),
					      GFP_KERNEL);
	if (!drvdata->kbd_backlight)
		return -ENOMEM;

	drvdata->kbd_backlight->removed = false;
	drvdata->kbd_backlight->brightness = 0;
	drvdata->kbd_backlight->hdev = hdev;
	drvdata->kbd_backlight->cdev.name = "asus::kbd_backlight";
	drvdata->kbd_backlight->cdev.max_brightness = 3;
	drvdata->kbd_backlight->cdev.brightness_set = asus_kbd_backlight_set;
	drvdata->kbd_backlight->cdev.brightness_get = asus_kbd_backlight_get;
	INIT_WORK(&drvdata->kbd_backlight->work, asus_kbd_backlight_work);

	ret = devm_led_classdev_register(&hdev->dev, &drvdata->kbd_backlight->cdev);
	if (ret < 0) {
		/* No need to have this still around */
		devm_kfree(&hdev->dev, drvdata->kbd_backlight);
		cancel_work_sync(&drvdata->kbd_backlight->work);
	}

	return ret;
}

static int asus_rog_input_configured(struct hid_device *hdev, struct hid_input *hi)
{
	struct asus_drvdata *drvdata = hid_get_drvdata(hdev);

	if (drvdata->enable_backlight && asus_kbd_register_leds(hdev))
		hid_warn(hdev, "Failed to initialize backlight.\n");

	return 0;
}

static int asus_rog_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;
	struct asus_drvdata *drvdata;

	drvdata = devm_kzalloc(&hdev->dev, sizeof(*drvdata), GFP_KERNEL);
	if (drvdata == NULL) {
		hid_err(hdev, "Can't alloc Asus descriptor\n");
		return -ENOMEM;
	}

	hid_set_drvdata(hdev, drvdata);

	drvdata->quirks = id->driver_data;

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "Asus hid parse failed: %d\n", ret);
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "Asus hw start failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void asus_rog_remove(struct hid_device *hdev)
{
	struct asus_drvdata *drvdata = hid_get_drvdata(hdev);

	if (drvdata->kbd_backlight) {
		drvdata->kbd_backlight->removed = true;
		cancel_work_sync(&drvdata->kbd_backlight->work);
	}
}

static const struct hid_device_id asus_rog_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_ASUSTEK, USB_DEVICE_ID_ASUSTEK_ROG_MACROKEY1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_ASUSTEK, USB_DEVICE_ID_ASUSTEK_ROG_MACROKEY2), QUIRK_USE_KBD_BACKLIGHT },
	{ HID_USB_DEVICE(USB_VENDOR_ID_TURBOX, USB_DEVICE_ID_ASUSTEK_ZEN_AIO_KBD1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_JESS, USB_DEVICE_ID_ASUSTEK_ZEN_AIO_KBD2) },
	{ }
};
MODULE_DEVICE_TABLE(hid, asus_rog_devices);

static struct hid_driver asus_rog_driver = {
	.name = "asus-rog",
	.id_table = asus_rog_devices,
	.probe			= asus_rog_probe,
	.remove			= asus_rog_remove,
	.input_mapping		= asus_rog_input_mapping,
	.input_configured	= asus_rog_input_configured,
};
module_hid_driver(asus_rog_driver);

MODULE_LICENSE("GPL");
