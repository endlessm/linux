/*
 *  HID driver for some Asus Gaming model equipped with ITE "special" keyboard 
 *  devices (8910)
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

#define ite_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, max, \
					EV_KEY, (c))
static int ite_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	if ((usage->hid & HID_USAGE_PAGE) != HID_UP_ITEVENDOR)
		return 0;

	set_bit(EV_REP, hi->input->evbit);
	switch (usage->hid & HID_USAGE) {
	/* Reported on ASUS Gaming model keyboards */
	case 0x6c: ite_map_key_clear(KEY_SLEEP);		break;
	case 0x88: ite_map_key_clear(KEY_WLAN);			break;
	case 0xc5: ite_map_key_clear(KEY_KBDILLUMDOWN);		break;
	case 0xc4: ite_map_key_clear(KEY_KBDILLUMUP);		break;
	case 0x10: ite_map_key_clear(KEY_BRIGHTNESSDOWN);	break;
	case 0x20: ite_map_key_clear(KEY_BRIGHTNESSUP);		break;
	case 0x35: ite_map_key_clear(KEY_DISPLAY_OFF);		break;
	// KEY_F21 is for ASUS notebook touchpad toggle
	case 0x6b: ite_map_key_clear(KEY_F21);			break;
	default:
		return 0;
	}
	return 1;
}

static const struct hid_device_id ite_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_ASUSTEK, USB_DEVICE_ID_ASUSTEK_GAMING_NB_KBD) },
	{ }
};
MODULE_DEVICE_TABLE(hid, ite_devices);

static struct hid_driver ite_usbkbd_driver = {
	.name = "ite8910",
	.id_table = ite_devices,
	.input_mapping = ite_input_mapping,
};
module_hid_driver(ite_usbkbd_driver);

MODULE_LICENSE("GPL");
