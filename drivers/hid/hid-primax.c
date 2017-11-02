/*
 * HID driver for primax and similar keyboards with in-band modifiers
 *
 * Copyright 2011 Google Inc. All Rights Reserved
 *
 * Author:
 *	Terry Lambert <tlambert@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <linux/module.h>

#include "hid-ids.h"

static int pr_input_mapping(struct hid_device *hdev, struct hid_input *hi,
                struct hid_field *field, struct hid_usage *usage,
                unsigned long **bit, int *max)
{
	/* Primax wireless keyboard have HID_USAGE_PAGE field with
         * HID_UP_LED instead of HID_UP_KEYBOARD. Correct the wrong
	 * usage page and let the standard code to do the rest.
	 */

	if (field->application != HID_GD_KEYBOARD)
		return 0;

	if ((usage->hid & HID_USAGE_PAGE) != HID_UP_LED)
		return 0;

	usage->hid &= HID_USAGE;
	usage->hid |= HID_UP_KEYBOARD;

	return 0;
}

static int px_raw_event(struct hid_device *hid, struct hid_report *report,
	 u8 *data, int size)
{
	int idx = size;

        struct usb_interface *intf = to_usb_interface(hid->dev.parent);
        unsigned short ifnum = intf->cur_altsetting->desc.bInterfaceNumber;

	/* No need to do anything on interface other than keyboard */
	if (ifnum != 0) {
		dbg_hid("%s: ignoring ifnum %d\n", __func__, ifnum);
		return 0;
	}

	switch (report->id) {
	case 0:		/* keyboard input */
		/*
		 * Convert in-band modifier key values into out of band
		 * modifier bits and pull the key strokes from the report.
		 * Thus a report data set which looked like:
		 *
		 * [00][00][E0][30][00][00][00][00]
		 * (no modifier bits + "Left Shift" key + "1" key)
		 *
		 * Would be converted to:
		 *
		 * [01][00][00][30][00][00][00][00]
		 * (Left Shift modifier bit + "1" key)
		 *
		 * As long as it's in the size range, the upper level
		 * drivers don't particularly care if there are in-band
		 * 0-valued keys, so they don't stop parsing.
		 */
		while (--idx > 1) {
			if (data[idx] < 0xE0 || data[idx] > 0xE7)
				continue;
			data[0] |= (1 << (data[idx] - 0xE0));
			data[idx] = 0;
		}
		hid_report_raw_event(hid, HID_INPUT_REPORT, data, size, 0);
		return 1;

	default:	/* unknown report */
		/* Unknown report type; pass upstream */
		hid_info(hid, "unknown report type %d\n", report->id);
		break;
	}

	return 0;
}

static const struct hid_device_id px_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_PRIMAX, USB_DEVICE_ID_PRIMAX_KEYBOARD) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_PRIMAX, USB_DEVICE_ID_PRIMAX_WIRELESS_KBD_MOUSE_4E80) },
	{ HID_USB_DEVICE(USB_VENDOR_ID_PRIMAX, USB_DEVICE_ID_PRIMAX_WIRELESS_KBD_MOUSE_4E57) },
	{ }
};
MODULE_DEVICE_TABLE(hid, px_devices);

static struct hid_driver px_driver = {
	.name = "primax",
	.id_table = px_devices,
	.raw_event = px_raw_event,
	.input_mapping = pr_input_mapping,
};
module_hid_driver(px_driver);

MODULE_AUTHOR("Terry Lambert <tlambert@google.com>");
MODULE_LICENSE("GPL");
