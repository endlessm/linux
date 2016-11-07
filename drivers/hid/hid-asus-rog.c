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

#define asus_rog_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, \
						max, EV_KEY, (c))
static int asus_rog_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	if ((usage->hid & HID_USAGE_PAGE) != HID_UP_ASUS_ROG_HOTKEY)
		return 0;

	set_bit(EV_REP, hi->input->evbit);
	switch (usage->hid & HID_USAGE) {
	/* Reported on ASUS Gaming model (Republic of Gamers) keyboards */
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
	return 1;
}

static const struct hid_device_id asus_rog_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_ASUSTEK, USB_DEVICE_ID_ASUSTEK_ROG_MACROKEY1) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_ASUSTEK, USB_DEVICE_ID_ASUSTEK_ROG_MACROKEY2) },
	{ }
};
MODULE_DEVICE_TABLE(hid, asus_rog_devices);

static struct hid_driver asus_rog_driver = {
	.name = "asus-rog",
	.id_table = asus_rog_devices,
	.input_mapping = asus_rog_input_mapping,
};
module_hid_driver(asus_rog_driver);

MODULE_LICENSE("GPL");
