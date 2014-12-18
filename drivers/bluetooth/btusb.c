/*
 *
 *  Generic Bluetooth USB driver
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/firmware.h>
#include <linux/delay.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#define VERSION "0.6"

static bool ignore_dga;
static bool ignore_csr;
static bool ignore_sniffer;
static bool disable_scofix;
static bool force_scofix;

static bool reset = 1;

static struct usb_driver btusb_driver;

#define BTUSB_IGNORE		0x01
#define BTUSB_DIGIANSWER	0x02
#define BTUSB_CSR		0x04
#define BTUSB_SNIFFER		0x08
#define BTUSB_BCM92035		0x10
#define BTUSB_BROKEN_ISOC	0x20
#define BTUSB_WRONG_SCO_MTU	0x40
#define BTUSB_ATH3012		0x80
#define BTUSB_INTEL		0x100
#define BTUSB_REALTEK		0x200
#define BTUSB_BCM_PATCHRAM	0x800

static const struct usb_device_id btusb_table[] = {
	/* Generic Bluetooth USB device */
	{ USB_DEVICE_INFO(0xe0, 0x01, 0x01) },

	/*Realtek bluetooth usb device*/
	{ USB_VENDOR_AND_INTERFACE_INFO(0x0bda, 0xe0, 0x01, 0x01) },

	/* Apple-specific (Broadcom) devices */
	{ USB_VENDOR_AND_INTERFACE_INFO(0x05ac, 0xff, 0x01, 0x01) },

	/* MediaTek MT76x0E */
	{ USB_DEVICE(0x0e8d, 0x763f) },

	/* Broadcom SoftSailing reporting vendor specific */
	{ USB_DEVICE(0x0a5c, 0x21e1) },

	/* Apple MacBookPro 7,1 */
	{ USB_DEVICE(0x05ac, 0x8213) },

	/* Apple iMac11,1 */
	{ USB_DEVICE(0x05ac, 0x8215) },

	/* Apple MacBookPro6,2 */
	{ USB_DEVICE(0x05ac, 0x8218) },

	/* Apple MacBookAir3,1, MacBookAir3,2 */
	{ USB_DEVICE(0x05ac, 0x821b) },

	/* Apple MacBookAir4,1 */
	{ USB_DEVICE(0x05ac, 0x821f) },

	/* Apple MacBookPro8,2 */
	{ USB_DEVICE(0x05ac, 0x821a) },

	/* Apple MacMini5,1 */
	{ USB_DEVICE(0x05ac, 0x8281) },

	/* AVM BlueFRITZ! USB v2.0 */
	{ USB_DEVICE(0x057c, 0x3800) },

	/* Bluetooth Ultraport Module from IBM */
	{ USB_DEVICE(0x04bf, 0x030a) },

	/* ALPS Modules with non-standard id */
	{ USB_DEVICE(0x044e, 0x3001) },
	{ USB_DEVICE(0x044e, 0x3002) },

	/* Ericsson with non-standard id */
	{ USB_DEVICE(0x0bdb, 0x1002) },

	/* Canyon CN-BTU1 with HID interfaces */
	{ USB_DEVICE(0x0c10, 0x0000) },

	/* Broadcom BCM20702A0 */
	{ USB_DEVICE(0x0b05, 0x17b5) },
	{ USB_DEVICE(0x0b05, 0x17cb) },
	{ USB_DEVICE(0x04ca, 0x2003) },
	{ USB_DEVICE(0x0489, 0xe042) },
	{ USB_DEVICE(0x13d3, 0x3388), .driver_info = BTUSB_BCM_PATCHRAM },
	{ USB_DEVICE(0x13d3, 0x3389), .driver_info = BTUSB_BCM_PATCHRAM },
	{ USB_DEVICE(0x413c, 0x8197), .driver_info = BTUSB_BCM_PATCHRAM },
	{ USB_DEVICE(0x413c, 0x8143), .driver_info = BTUSB_BCM_PATCHRAM },

	/* Broadcom BCM43142A0 */
	{ USB_DEVICE(0x04ca, 0x2007), .driver_info = BTUSB_BCM_PATCHRAM },
	{ USB_DEVICE(0x105b, 0xe065), .driver_info = BTUSB_BCM_PATCHRAM },

	/* Foxconn - Hon Hai */
	{ USB_VENDOR_AND_INTERFACE_INFO(0x0489, 0xff, 0x01, 0x01), .driver_info = BTUSB_BCM_PATCHRAM },

	/*Broadcom devices with vendor specific id */
	{ USB_VENDOR_AND_INTERFACE_INFO(0x0a5c, 0xff, 0x01, 0x01), .driver_info = BTUSB_BCM_PATCHRAM },

	/* Belkin F8065bf - Broadcom based */
	{ USB_VENDOR_AND_INTERFACE_INFO(0x050d, 0xff, 0x01, 0x01) },

	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, btusb_table);

static const struct usb_device_id blacklist_table[] = {
	/* CSR BlueCore devices */
	{ USB_DEVICE(0x0a12, 0x0001), .driver_info = BTUSB_CSR },

	/* Broadcom BCM2033 without firmware */
	{ USB_DEVICE(0x0a5c, 0x2033), .driver_info = BTUSB_IGNORE },

	/* Atheros 3011 with sflash firmware */
	{ USB_DEVICE(0x0cf3, 0x3002), .driver_info = BTUSB_IGNORE },
	{ USB_DEVICE(0x0cf3, 0xe019), .driver_info = BTUSB_IGNORE },
	{ USB_DEVICE(0x13d3, 0x3304), .driver_info = BTUSB_IGNORE },
	{ USB_DEVICE(0x0930, 0x0215), .driver_info = BTUSB_IGNORE },
	{ USB_DEVICE(0x0489, 0xe03d), .driver_info = BTUSB_IGNORE },
	{ USB_DEVICE(0x0489, 0xe027), .driver_info = BTUSB_IGNORE },

	/* Atheros AR9285 Malbec with sflash firmware */
	{ USB_DEVICE(0x03f0, 0x311d), .driver_info = BTUSB_IGNORE },

	/* Atheros 3012 with sflash firmware */
	{ USB_DEVICE(0x0cf3, 0x0036), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0x3004), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0x3008), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0x311d), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0x817a), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x13d3, 0x3375), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x04ca, 0x3004), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x04ca, 0x3005), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x04ca, 0x3006), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x04ca, 0x3008), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x13d3, 0x3362), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0xe004), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0xe005), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0930, 0x0219), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0930, 0x0220), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0489, 0xe057), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x13d3, 0x3393), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0489, 0xe04e), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0489, 0xe056), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0489, 0xe04d), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x04c5, 0x1330), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x13d3, 0x3402), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0x3121), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0cf3, 0xe003), .driver_info = BTUSB_ATH3012 },

	/* Atheros AR5BBU12 with sflash firmware */
	{ USB_DEVICE(0x0489, 0xe02c), .driver_info = BTUSB_IGNORE },

	/* Atheros AR5BBU12 with sflash firmware */
	{ USB_DEVICE(0x0489, 0xe03c), .driver_info = BTUSB_ATH3012 },
	{ USB_DEVICE(0x0489, 0xe036), .driver_info = BTUSB_ATH3012 },

	/* Broadcom BCM2035 */
	{ USB_DEVICE(0x0a5c, 0x2035), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x0a5c, 0x200a), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x0a5c, 0x2009), .driver_info = BTUSB_BCM92035 },

	/* Broadcom BCM2045 */
	{ USB_DEVICE(0x0a5c, 0x2039), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x0a5c, 0x2101), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* IBM/Lenovo ThinkPad with Broadcom chip */
	{ USB_DEVICE(0x0a5c, 0x201e), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x0a5c, 0x2110), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* HP laptop with Broadcom chip */
	{ USB_DEVICE(0x03f0, 0x171d), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* Dell laptop with Broadcom chip */
	{ USB_DEVICE(0x413c, 0x8126), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* Dell Wireless 370 and 410 devices */
	{ USB_DEVICE(0x413c, 0x8152), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x413c, 0x8156), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* Belkin F8T012 and F8T013 devices */
	{ USB_DEVICE(0x050d, 0x0012), .driver_info = BTUSB_WRONG_SCO_MTU },
	{ USB_DEVICE(0x050d, 0x0013), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* Asus WL-BTD202 device */
	{ USB_DEVICE(0x0b05, 0x1715), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* Kensington Bluetooth USB adapter */
	{ USB_DEVICE(0x047d, 0x105e), .driver_info = BTUSB_WRONG_SCO_MTU },

	/* RTX Telecom based adapters with buggy SCO support */
	{ USB_DEVICE(0x0400, 0x0807), .driver_info = BTUSB_BROKEN_ISOC },
	{ USB_DEVICE(0x0400, 0x080a), .driver_info = BTUSB_BROKEN_ISOC },

	/* CONWISE Technology based adapters with buggy SCO support */
	{ USB_DEVICE(0x0e5e, 0x6622), .driver_info = BTUSB_BROKEN_ISOC },

	/* Digianswer devices */
	{ USB_DEVICE(0x08fd, 0x0001), .driver_info = BTUSB_DIGIANSWER },
	{ USB_DEVICE(0x08fd, 0x0002), .driver_info = BTUSB_IGNORE },

	/* CSR BlueCore Bluetooth Sniffer */
	{ USB_DEVICE(0x0a12, 0x0002), .driver_info = BTUSB_SNIFFER },

	/* Frontline ComProbe Bluetooth Sniffer */
	{ USB_DEVICE(0x16d3, 0x0002), .driver_info = BTUSB_SNIFFER },

	/* Intel Bluetooth device */
	{ USB_DEVICE(0x8087, 0x07dc), .driver_info = BTUSB_INTEL },

	/*Realtek bluetooth usb device*/
	{ USB_VENDOR_AND_INTERFACE_INFO(0x0bda, 0xe0, 0x01, 0x01), .driver_info = BTUSB_REALTEK },
	{ USB_DEVICE(0x13d3, 0x3410), .driver_info = BTUSB_REALTEK },

	{ }	/* Terminating entry */
};

#define RTKBT_DBG BT_DBG
#define RTKBT_ERR pr_err

#define BTUSB_MAX_ISOC_FRAMES	10

#define BTUSB_INTR_RUNNING	0
#define BTUSB_BULK_RUNNING	1
#define BTUSB_ISOC_RUNNING	2
#define BTUSB_SUSPENDING	3
#define BTUSB_DID_ISO_RESUME	4

struct btusb_data {
	struct hci_dev       *hdev;
	struct usb_device    *udev;
	struct usb_interface *intf;
	struct usb_interface *isoc;

	spinlock_t lock;

	unsigned long flags;

	struct work_struct work;
	struct work_struct waker;

	struct usb_anchor tx_anchor;
	struct usb_anchor intr_anchor;
	struct usb_anchor bulk_anchor;
	struct usb_anchor isoc_anchor;
	struct usb_anchor deferred;
	int tx_in_flight;
	spinlock_t txlock;

	struct usb_endpoint_descriptor *intr_ep;
	struct usb_endpoint_descriptor *bulk_tx_ep;
	struct usb_endpoint_descriptor *bulk_rx_ep;
	struct usb_endpoint_descriptor *isoc_tx_ep;
	struct usb_endpoint_descriptor *isoc_rx_ep;

	__u8 cmdreq_type;

	unsigned int sco_num;
	int isoc_altsetting;
	int suspend_count;
};

static int inc_tx(struct btusb_data *data)
{
	unsigned long flags;
	int rv;

	spin_lock_irqsave(&data->txlock, flags);
	rv = test_bit(BTUSB_SUSPENDING, &data->flags);
	if (!rv)
		data->tx_in_flight++;
	spin_unlock_irqrestore(&data->txlock, flags);

	return rv;
}

static void btusb_intr_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = hci_get_drvdata(hdev);
	int err;

	BT_DBG("%s urb %p status %d count %d", hdev->name,
					urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (urb->status == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		if (hci_recv_fragment(hdev, HCI_EVENT_PKT,
						urb->transfer_buffer,
						urb->actual_length) < 0) {
			BT_ERR("%s corrupted event packet", hdev->name);
			hdev->stat.err_rx++;
		}
	}

	if (!test_bit(BTUSB_INTR_RUNNING, &data->flags))
		return;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p failed to resubmit (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;

	BT_DBG("%s", hdev->name);

	if (!data->intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(data->intr_ep->wMaxPacketSize);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvintpipe(data->udev, data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, data->udev, pipe, buf, size,
						btusb_intr_complete, hdev,
						data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p submission failed (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_bulk_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = hci_get_drvdata(hdev);
	int err;

	BT_DBG("%s urb %p status %d count %d", hdev->name,
					urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (urb->status == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		if (hci_recv_fragment(hdev, HCI_ACLDATA_PKT,
						urb->transfer_buffer,
						urb->actual_length) < 0) {
			BT_ERR("%s corrupted ACL packet", hdev->name);
			hdev->stat.err_rx++;
		}
	}

	if (!test_bit(BTUSB_BULK_RUNNING, &data->flags))
		return;

	usb_anchor_urb(urb, &data->bulk_anchor);
	usb_mark_last_busy(data->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p failed to resubmit (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_bulk_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size = HCI_MAX_FRAME_SIZE;

	BT_DBG("%s", hdev->name);

	if (!data->bulk_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvbulkpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);

	usb_fill_bulk_urb(urb, data->udev, pipe,
					buf, size, btusb_bulk_complete, hdev);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->bulk_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p submission failed (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_isoc_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = hci_get_drvdata(hdev);
	int i, err;

	BT_DBG("%s urb %p status %d count %d", hdev->name,
					urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (urb->status == 0) {
		for (i = 0; i < urb->number_of_packets; i++) {
			unsigned int offset = urb->iso_frame_desc[i].offset;
			unsigned int length = urb->iso_frame_desc[i].actual_length;

			if (urb->iso_frame_desc[i].status)
				continue;

			hdev->stat.byte_rx += length;

			if (hci_recv_fragment(hdev, HCI_SCODATA_PKT,
						urb->transfer_buffer + offset,
								length) < 0) {
				BT_ERR("%s corrupted SCO packet", hdev->name);
				hdev->stat.err_rx++;
			}
		}
	}

	if (!test_bit(BTUSB_ISOC_RUNNING, &data->flags))
		return;

	usb_anchor_urb(urb, &data->isoc_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p failed to resubmit (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static inline void __fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	BT_DBG("len %d mtu %d", len, mtu);

	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
					i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

static int btusb_submit_isoc_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;

	BT_DBG("%s", hdev->name);

	if (!data->isoc_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize) *
						BTUSB_MAX_ISOC_FRAMES;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvisocpipe(data->udev, data->isoc_rx_ep->bEndpointAddress);

	usb_fill_int_urb(urb, data->udev, pipe, buf, size, btusb_isoc_complete,
				hdev, data->isoc_rx_ep->bInterval);

	urb->transfer_flags  = URB_FREE_BUFFER | URB_ISO_ASAP;

	__fill_isoc_descriptor(urb, size,
			le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &data->isoc_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p submission failed (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;
	struct btusb_data *data = hci_get_drvdata(hdev);

	BT_DBG("%s urb %p status %d count %d", hdev->name,
					urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

done:
	spin_lock(&data->txlock);
	data->tx_in_flight--;
	spin_unlock(&data->txlock);

	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static void btusb_isoc_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;

	BT_DBG("%s urb %p status %d count %d", hdev->name,
					urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

done:
	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static int btusb_open(struct hci_dev *hdev)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	int err;

	BT_DBG("%s", hdev->name);

	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		return err;

	data->intf->needs_remote_wakeup = 1;

	if (test_and_set_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (test_and_set_bit(BTUSB_INTR_RUNNING, &data->flags))
		goto done;

	err = btusb_submit_intr_urb(hdev, GFP_KERNEL);
	if (err < 0)
		goto failed;

	err = btusb_submit_bulk_urb(hdev, GFP_KERNEL);
	if (err < 0) {
		usb_kill_anchored_urbs(&data->intr_anchor);
		goto failed;
	}

	set_bit(BTUSB_BULK_RUNNING, &data->flags);
	btusb_submit_bulk_urb(hdev, GFP_KERNEL);

done:
	usb_autopm_put_interface(data->intf);
	return 0;

failed:
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);
	clear_bit(HCI_RUNNING, &hdev->flags);
	usb_autopm_put_interface(data->intf);
	return err;
}

static void btusb_stop_traffic(struct btusb_data *data)
{
	usb_kill_anchored_urbs(&data->intr_anchor);
	usb_kill_anchored_urbs(&data->bulk_anchor);
	usb_kill_anchored_urbs(&data->isoc_anchor);
}

static int btusb_close(struct hci_dev *hdev)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	int err;

	BT_DBG("%s", hdev->name);

	if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags))
		return 0;

	cancel_work_sync(&data->work);
	cancel_work_sync(&data->waker);

	clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
	clear_bit(BTUSB_BULK_RUNNING, &data->flags);
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);

	btusb_stop_traffic(data);
	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		goto failed;

	data->intf->needs_remote_wakeup = 0;
	usb_autopm_put_interface(data->intf);

failed:
	usb_scuttle_anchored_urbs(&data->deferred);
	return 0;
}

static int btusb_flush(struct hci_dev *hdev)
{
	struct btusb_data *data = hci_get_drvdata(hdev);

	BT_DBG("%s", hdev->name);

	usb_kill_anchored_urbs(&data->tx_anchor);

	return 0;
}

static int btusb_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned int pipe;
	int err;

	BT_DBG("%s", hdev->name);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return -EBUSY;

	skb->dev = (void *) hdev;

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		dr = kmalloc(sizeof(*dr), GFP_ATOMIC);
		if (!dr) {
			usb_free_urb(urb);
			return -ENOMEM;
		}

		dr->bRequestType = data->cmdreq_type;
		dr->bRequest     = 0;
		dr->wIndex       = 0;
		dr->wValue       = 0;
		dr->wLength      = __cpu_to_le16(skb->len);

		pipe = usb_sndctrlpipe(data->udev, 0x00);

		usb_fill_control_urb(urb, data->udev, pipe, (void *) dr,
				skb->data, skb->len, btusb_tx_complete, skb);

		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		if (!data->bulk_tx_ep)
			return -ENODEV;

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndbulkpipe(data->udev,
					data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, data->udev, pipe,
				skb->data, skb->len, btusb_tx_complete, skb);

		hdev->stat.acl_tx++;
		break;

	case HCI_SCODATA_PKT:
		if (!data->isoc_tx_ep || hci_conn_num(hdev, SCO_LINK) < 1)
			return -ENODEV;

		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndisocpipe(data->udev,
					data->isoc_tx_ep->bEndpointAddress);

		usb_fill_int_urb(urb, data->udev, pipe,
				skb->data, skb->len, btusb_isoc_tx_complete,
				skb, data->isoc_tx_ep->bInterval);

		urb->transfer_flags  = URB_ISO_ASAP;

		__fill_isoc_descriptor(urb, skb->len,
				le16_to_cpu(data->isoc_tx_ep->wMaxPacketSize));

		hdev->stat.sco_tx++;
		goto skip_waking;

	default:
		return -EILSEQ;
	}

	err = inc_tx(data);
	if (err) {
		usb_anchor_urb(urb, &data->deferred);
		schedule_work(&data->waker);
		err = 0;
		goto done;
	}

skip_waking:
	usb_anchor_urb(urb, &data->tx_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			BT_ERR("%s urb %p submission failed (%d)",
						hdev->name, urb, -err);
		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else {
		usb_mark_last_busy(data->udev);
	}

done:
	usb_free_urb(urb);
	return err;
}

static void btusb_notify(struct hci_dev *hdev, unsigned int evt)
{
	struct btusb_data *data = hci_get_drvdata(hdev);

	BT_DBG("%s evt %d", hdev->name, evt);

	if (hci_conn_num(hdev, SCO_LINK) != data->sco_num) {
		data->sco_num = hci_conn_num(hdev, SCO_LINK);
		schedule_work(&data->work);
	}
}

static inline int __set_isoc_interface(struct hci_dev *hdev, int altsetting)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct usb_interface *intf = data->isoc;
	struct usb_endpoint_descriptor *ep_desc;
	int i, err;

	if (!data->isoc)
		return -ENODEV;

	err = usb_set_interface(data->udev, 1, altsetting);
	if (err < 0) {
		BT_ERR("%s setting interface failed (%d)", hdev->name, -err);
		return err;
	}

	data->isoc_altsetting = altsetting;

	data->isoc_tx_ep = NULL;
	data->isoc_rx_ep = NULL;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (!data->isoc_tx_ep && usb_endpoint_is_isoc_out(ep_desc)) {
			data->isoc_tx_ep = ep_desc;
			continue;
		}

		if (!data->isoc_rx_ep && usb_endpoint_is_isoc_in(ep_desc)) {
			data->isoc_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->isoc_tx_ep || !data->isoc_rx_ep) {
		BT_ERR("%s invalid SCO descriptors", hdev->name);
		return -ENODEV;
	}

	return 0;
}

static void btusb_work(struct work_struct *work)
{
	struct btusb_data *data = container_of(work, struct btusb_data, work);
	struct hci_dev *hdev = data->hdev;
	int new_alts;
	int err;

	if (data->sco_num > 0) {
		if (!test_bit(BTUSB_DID_ISO_RESUME, &data->flags)) {
			err = usb_autopm_get_interface(data->isoc ? data->isoc : data->intf);
			if (err < 0) {
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
				usb_kill_anchored_urbs(&data->isoc_anchor);
				return;
			}

			set_bit(BTUSB_DID_ISO_RESUME, &data->flags);
		}

		if (hdev->voice_setting & 0x0020) {
			static const int alts[3] = { 2, 4, 5 };
			new_alts = alts[data->sco_num - 1];
		} else {
			new_alts = data->sco_num;
		}

		if (data->isoc_altsetting != new_alts) {
			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
			usb_kill_anchored_urbs(&data->isoc_anchor);

			if (__set_isoc_interface(hdev, new_alts) < 0)
				return;
		}

		if (!test_and_set_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
			if (btusb_submit_isoc_urb(hdev, GFP_KERNEL) < 0)
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
			else
				btusb_submit_isoc_urb(hdev, GFP_KERNEL);
		}
	} else {
		clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		usb_kill_anchored_urbs(&data->isoc_anchor);

		__set_isoc_interface(hdev, 0);
		if (test_and_clear_bit(BTUSB_DID_ISO_RESUME, &data->flags))
			usb_autopm_put_interface(data->isoc ? data->isoc : data->intf);
	}
}

static void btusb_waker(struct work_struct *work)
{
	struct btusb_data *data = container_of(work, struct btusb_data, waker);
	int err;

	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		return;

	usb_autopm_put_interface(data->intf);
}

static int btusb_setup_bcm92035(struct hci_dev *hdev)
{
	struct sk_buff *skb;
	u8 val = 0x00;

	BT_DBG("%s", hdev->name);

	skb = __hci_cmd_sync(hdev, 0xfc3b, 1, &val, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb))
		BT_ERR("BCM92035 command failed (%ld)", -PTR_ERR(skb));
	else
		kfree_skb(skb);

	return 0;
}

struct intel_version {
	u8 status;
	u8 hw_platform;
	u8 hw_variant;
	u8 hw_revision;
	u8 fw_variant;
	u8 fw_revision;
	u8 fw_build_num;
	u8 fw_build_ww;
	u8 fw_build_yy;
	u8 fw_patch_num;
} __packed;

static const struct firmware *btusb_setup_intel_get_fw(struct hci_dev *hdev,
						struct intel_version *ver)
{
	const struct firmware *fw;
	char fwname[64];
	int ret;

	snprintf(fwname, sizeof(fwname),
		 "intel/ibt-hw-%x.%x.%x-fw-%x.%x.%x.%x.%x.bseq",
		 ver->hw_platform, ver->hw_variant, ver->hw_revision,
		 ver->fw_variant,  ver->fw_revision, ver->fw_build_num,
		 ver->fw_build_ww, ver->fw_build_yy);

	ret = request_firmware(&fw, fwname, &hdev->dev);
	if (ret < 0) {
		if (ret == -EINVAL) {
			BT_ERR("%s Intel firmware file request failed (%d)",
			       hdev->name, ret);
			return NULL;
		}

		BT_ERR("%s failed to open Intel firmware file: %s(%d)",
		       hdev->name, fwname, ret);

		/* If the correct firmware patch file is not found, use the
		 * default firmware patch file instead
		 */
		snprintf(fwname, sizeof(fwname), "intel/ibt-hw-%x.%x.bseq",
			 ver->hw_platform, ver->hw_variant);
		if (request_firmware(&fw, fwname, &hdev->dev) < 0) {
			BT_ERR("%s failed to open default Intel fw file: %s",
			       hdev->name, fwname);
			return NULL;
		}
	}

	BT_INFO("%s: Intel Bluetooth firmware file: %s", hdev->name, fwname);

	return fw;
}

static int btusb_setup_intel_patching(struct hci_dev *hdev,
				      const struct firmware *fw,
				      const u8 **fw_ptr, int *disable_patch)
{
	struct sk_buff *skb;
	struct hci_command_hdr *cmd;
	const u8 *cmd_param;
	struct hci_event_hdr *evt = NULL;
	const u8 *evt_param = NULL;
	int remain = fw->size - (*fw_ptr - fw->data);

	/* The first byte indicates the types of the patch command or event.
	 * 0x01 means HCI command and 0x02 is HCI event. If the first bytes
	 * in the current firmware buffer doesn't start with 0x01 or
	 * the size of remain buffer is smaller than HCI command header,
	 * the firmware file is corrupted and it should stop the patching
	 * process.
	 */
	if (remain > HCI_COMMAND_HDR_SIZE && *fw_ptr[0] != 0x01) {
		BT_ERR("%s Intel fw corrupted: invalid cmd read", hdev->name);
		return -EINVAL;
	}
	(*fw_ptr)++;
	remain--;

	cmd = (struct hci_command_hdr *)(*fw_ptr);
	*fw_ptr += sizeof(*cmd);
	remain -= sizeof(*cmd);

	/* Ensure that the remain firmware data is long enough than the length
	 * of command parameter. If not, the firmware file is corrupted.
	 */
	if (remain < cmd->plen) {
		BT_ERR("%s Intel fw corrupted: invalid cmd len", hdev->name);
		return -EFAULT;
	}

	/* If there is a command that loads a patch in the firmware
	 * file, then enable the patch upon success, otherwise just
	 * disable the manufacturer mode, for example patch activation
	 * is not required when the default firmware patch file is used
	 * because there are no patch data to load.
	 */
	if (*disable_patch && le16_to_cpu(cmd->opcode) == 0xfc8e)
		*disable_patch = 0;

	cmd_param = *fw_ptr;
	*fw_ptr += cmd->plen;
	remain -= cmd->plen;

	/* This reads the expected events when the above command is sent to the
	 * device. Some vendor commands expects more than one events, for
	 * example command status event followed by vendor specific event.
	 * For this case, it only keeps the last expected event. so the command
	 * can be sent with __hci_cmd_sync_ev() which returns the sk_buff of
	 * last expected event.
	 */
	while (remain > HCI_EVENT_HDR_SIZE && *fw_ptr[0] == 0x02) {
		(*fw_ptr)++;
		remain--;

		evt = (struct hci_event_hdr *)(*fw_ptr);
		*fw_ptr += sizeof(*evt);
		remain -= sizeof(*evt);

		if (remain < evt->plen) {
			BT_ERR("%s Intel fw corrupted: invalid evt len",
			       hdev->name);
			return -EFAULT;
		}

		evt_param = *fw_ptr;
		*fw_ptr += evt->plen;
		remain -= evt->plen;
	}

	/* Every HCI commands in the firmware file has its correspond event.
	 * If event is not found or remain is smaller than zero, the firmware
	 * file is corrupted.
	 */
	if (!evt || !evt_param || remain < 0) {
		BT_ERR("%s Intel fw corrupted: invalid evt read", hdev->name);
		return -EFAULT;
	}

	skb = __hci_cmd_sync_ev(hdev, le16_to_cpu(cmd->opcode), cmd->plen,
				cmd_param, evt->evt, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s sending Intel patch command (0x%4.4x) failed (%ld)",
		       hdev->name, cmd->opcode, PTR_ERR(skb));
		return PTR_ERR(skb);
	}

	/* It ensures that the returned event matches the event data read from
	 * the firmware file. At fist, it checks the length and then
	 * the contents of the event.
	 */
	if (skb->len != evt->plen) {
		BT_ERR("%s mismatch event length (opcode 0x%4.4x)", hdev->name,
		       le16_to_cpu(cmd->opcode));
		kfree_skb(skb);
		return -EFAULT;
	}

	if (memcmp(skb->data, evt_param, evt->plen)) {
		BT_ERR("%s mismatch event parameter (opcode 0x%4.4x)",
		       hdev->name, le16_to_cpu(cmd->opcode));
		kfree_skb(skb);
		return -EFAULT;
	}
	kfree_skb(skb);

	return 0;
}

static int btusb_setup_intel(struct hci_dev *hdev)
{
	struct sk_buff *skb;
	const struct firmware *fw;
	const u8 *fw_ptr;
	int disable_patch;
	struct intel_version *ver;

	const u8 mfg_enable[] = { 0x01, 0x00 };
	const u8 mfg_disable[] = { 0x00, 0x00 };
	const u8 mfg_reset_deactivate[] = { 0x00, 0x01 };
	const u8 mfg_reset_activate[] = { 0x00, 0x02 };

	BT_DBG("%s", hdev->name);

	/* The controller has a bug with the first HCI command sent to it
	 * returning number of completed commands as zero. This would stall the
	 * command processing in the Bluetooth core.
	 *
	 * As a workaround, send HCI Reset command first which will reset the
	 * number of completed commands and allow normal command processing
	 * from now on.
	 */
	skb = __hci_cmd_sync(hdev, HCI_OP_RESET, 0, NULL, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s sending initial HCI reset command failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}
	kfree_skb(skb);

	/* Read Intel specific controller version first to allow selection of
	 * which firmware file to load.
	 *
	 * The returned information are hardware variant and revision plus
	 * firmware variant, revision and build number.
	 */
	skb = __hci_cmd_sync(hdev, 0xfc05, 0, NULL, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s reading Intel fw version command failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}

	if (skb->len != sizeof(*ver)) {
		BT_ERR("%s Intel version event length mismatch", hdev->name);
		kfree_skb(skb);
		return -EIO;
	}

	ver = (struct intel_version *)skb->data;
	if (ver->status) {
		BT_ERR("%s Intel fw version event failed (%02x)", hdev->name,
		       ver->status);
		kfree_skb(skb);
		return -bt_to_errno(ver->status);
	}

	BT_INFO("%s: read Intel version: %02x%02x%02x%02x%02x%02x%02x%02x%02x",
		hdev->name, ver->hw_platform, ver->hw_variant,
		ver->hw_revision, ver->fw_variant,  ver->fw_revision,
		ver->fw_build_num, ver->fw_build_ww, ver->fw_build_yy,
		ver->fw_patch_num);

	/* fw_patch_num indicates the version of patch the device currently
	 * have. If there is no patch data in the device, it is always 0x00.
	 * So, if it is other than 0x00, no need to patch the deivce again.
	 */
	if (ver->fw_patch_num) {
		BT_INFO("%s: Intel device is already patched. patch num: %02x",
			hdev->name, ver->fw_patch_num);
		kfree_skb(skb);
		return 0;
	}

	/* Opens the firmware patch file based on the firmware version read
	 * from the controller. If it fails to open the matching firmware
	 * patch file, it tries to open the default firmware patch file.
	 * If no patch file is found, allow the device to operate without
	 * a patch.
	 */
	fw = btusb_setup_intel_get_fw(hdev, ver);
	if (!fw) {
		kfree_skb(skb);
		return 0;
	}
	fw_ptr = fw->data;

	/* This Intel specific command enables the manufacturer mode of the
	 * controller.
	 *
	 * Only while this mode is enabled, the driver can download the
	 * firmware patch data and configuration parameters.
	 */
	skb = __hci_cmd_sync(hdev, 0xfc11, 2, mfg_enable, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s entering Intel manufacturer mode failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		release_firmware(fw);
		return PTR_ERR(skb);
	}

	if (skb->data[0]) {
		u8 evt_status = skb->data[0];
		BT_ERR("%s enable Intel manufacturer mode event failed (%02x)",
		       hdev->name, evt_status);
		kfree_skb(skb);
		release_firmware(fw);
		return -bt_to_errno(evt_status);
	}
	kfree_skb(skb);

	disable_patch = 1;

	/* The firmware data file consists of list of Intel specific HCI
	 * commands and its expected events. The first byte indicates the
	 * type of the message, either HCI command or HCI event.
	 *
	 * It reads the command and its expected event from the firmware file,
	 * and send to the controller. Once __hci_cmd_sync_ev() returns,
	 * the returned event is compared with the event read from the firmware
	 * file and it will continue until all the messages are downloaded to
	 * the controller.
	 *
	 * Once the firmware patching is completed successfully,
	 * the manufacturer mode is disabled with reset and activating the
	 * downloaded patch.
	 *
	 * If the firmware patching fails, the manufacturer mode is
	 * disabled with reset and deactivating the patch.
	 *
	 * If the default patch file is used, no reset is done when disabling
	 * the manufacturer.
	 */
	while (fw->size > fw_ptr - fw->data) {
		int ret;

		ret = btusb_setup_intel_patching(hdev, fw, &fw_ptr,
						 &disable_patch);
		if (ret < 0)
			goto exit_mfg_deactivate;
	}

	release_firmware(fw);

	if (disable_patch)
		goto exit_mfg_disable;

	/* Patching completed successfully and disable the manufacturer mode
	 * with reset and activate the downloaded firmware patches.
	 */
	skb = __hci_cmd_sync(hdev, 0xfc11, sizeof(mfg_reset_activate),
			     mfg_reset_activate, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s exiting Intel manufacturer mode failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}
	kfree_skb(skb);

	BT_INFO("%s: Intel Bluetooth firmware patch completed and activated",
		hdev->name);

	return 0;

exit_mfg_disable:
	/* Disable the manufacturer mode without reset */
	skb = __hci_cmd_sync(hdev, 0xfc11, sizeof(mfg_disable), mfg_disable,
			     HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s exiting Intel manufacturer mode failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}
	kfree_skb(skb);

	BT_INFO("%s: Intel Bluetooth firmware patch completed", hdev->name);
	return 0;

exit_mfg_deactivate:
	release_firmware(fw);

	/* Patching failed. Disable the manufacturer mode with reset and
	 * deactivate the downloaded firmware patches.
	 */
	skb = __hci_cmd_sync(hdev, 0xfc11, sizeof(mfg_reset_deactivate),
			     mfg_reset_deactivate, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s exiting Intel manufacturer mode failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return PTR_ERR(skb);
	}
	kfree_skb(skb);

	BT_INFO("%s: Intel Bluetooth firmware patch completed and deactivated",
		hdev->name);

	return 0;
}

/*******************************
**    Reasil patch code
********************************/
#define CMD_CMP_EVT		0x0e
#define PKT_LEN			300
#define MSG_TO			1000
#define PATCH_SEG_MAX	252
#define DATA_END		0x80
#define DOWNLOAD_OPCODE	0xfc20
#define BTOFF_OPCODE	0xfc28
#define TRUE			1
#define FALSE			0
#define CMD_HDR_LEN		sizeof(struct hci_command_hdr)
#define EVT_HDR_LEN		sizeof(struct hci_event_hdr)
#define CMD_CMP_LEN		sizeof(struct hci_ev_cmd_complete)
static uint8_t global_eversion = 0xff;

typedef struct {
	uint16_t	prod_id;
	uint16_t	lmp_sub;
	char		*patch_name;
	char		*config_name;
	uint8_t		*fw_cache;
	int			fw_len;
} patch_info;

typedef struct {
	struct list_head		list_node;
	struct usb_interface	*intf;
	struct usb_device		*udev;
	struct notifier_block	pm_notifier;
	patch_info			*patch_entry;
} dev_data;

typedef struct {
	uint8_t index;
	uint8_t data[PATCH_SEG_MAX];
} __attribute__((packed)) download_cp;

typedef struct {
	uint8_t status;
	uint8_t index;
} __attribute__((packed)) download_rp;

struct rtk_eversion_evt {
	uint8_t status;
	uint8_t version;
}__attribute__ ((packed));

struct rtk_epatch_entry{
	uint16_t chipID;
	uint16_t patch_length;
	uint32_t start_offset;
} __attribute__ ((packed));

struct rtk_epatch{
    	uint8_t signature[8];
	uint32_t fm_version;
	uint16_t number_of_total_patch;
	struct rtk_epatch_entry entry[0];
} __attribute__ ((packed));


struct rtk_extension_entry{
	uint8_t opcode;
	uint8_t length;
	uint8_t *data;
} __attribute__ ((packed));

#define HCI_VENDOR_READ_RTK_ROM_VERISION 0xfc6d
#define ROM_LMP_8723a               0x1200
#define ROM_LMP_8723b               0x8723       
#define ROM_LMP_8821a               0X8821
#define ROM_LMP_8761a               0X8761 
const uint8_t Extension_Section_SIGNATURE[4]={0x51,0x04,0xFD,0x77};
const uint8_t  RTK_EPATCH_SIGNATURE[8]={0x52,0x65,0x61,0x6C,0x74,0x65,0x63,
0x68};
uint16_t project_id[]=
{
	ROM_LMP_8723a,
	ROM_LMP_8723b,
	ROM_LMP_8821a,
	ROM_LMP_8761a
};

static patch_info* get_patch_entry(struct usb_device* udev);
static int load_firmware(struct hci_dev *hdev, uint8_t** buff);
static int check_fw_version(struct hci_dev* hdev);

static patch_info patch_table[] = {
    { 0xA761, 0x8761, "rtl8761au_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761AU only
    { 0x818B, 0x8761, "rtl8761aw8192eu_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761Aw + 8192EU
    { 0x8760, 0x8761, "rtl8761au8192ee_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761AU + 8192EE
    { 0xB761, 0x8761, "rtl8761au8192ee_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761AU + 8192EE
    { 0x8761, 0x8761, "rtl8761au8192ee_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761AU + 8192EE for LI
    { 0x8A60, 0x8761, "rtl8761au8812ae_fw", "rtl8761a_config", NULL, 0 }, //Rtl8761AU + 8812AE
    
    { 0x8821, 0x8821, "rtl8821a_fw", "rtl8821a_config", NULL, 0 },  //Rtl8821AE
    { 0x0821, 0x8821, "rtl8821a_fw", "rtl8821a_config", NULL, 0 },  //Rtl8821AU
    
    { 0xb720, 0x8723, "rtl8723b_fw", "rtl8723b_config", NULL, 0 },  //Rtl8723BU
    { 0xb72A, 0x8723, "rtl8723b_fw", "rtl8723b_config", NULL, 0 },  //Rtl8723BU
    { 0xb728, 0x8723, "rtl8723b_fw", "rtl8723b_config", NULL, 0 },  //Rtl8723BE for LC
    { 0xb723, 0x8723, "rtl8723b_fw", "rtl8723b_config", NULL, 0 },  //Rtl8723BE
    { 0x3410, 0x8723, "rtl8723b_fw", "rtl8723b_config", NULL, 0 },  //Rtl8723BE
    
    { 0, 0x1200, "rtl8723a_fw", "rtl8723a_config", NULL, 0 } //Rtl8723AU & Rtl8723AE
};

patch_info* get_patch_entry(struct usb_device* udev)
{
	patch_info	*patch_entry;
	uint16_t	pid;

	patch_entry = patch_table;
	pid = le16_to_cpu(udev->descriptor.idProduct);
	BT_DBG("pid = 0x%x", pid);
	while (pid != patch_entry->prod_id)
	{
		if (0 == patch_entry->prod_id) break;
		patch_entry++;
	}

	return patch_entry;
}
uint8_t rtk_get_eversion(struct hci_dev* hdev)
{
	struct rtk_eversion_evt *eversion;
	int			ret_val = 0;
	struct sk_buff * skb;

	BT_DBG("rtk_get_eversion, global_eversion=0x%x \n",global_eversion);
	if(global_eversion != 0xff)
		return global_eversion;
	
	if(hdev == NULL)
		return global_eversion;

	skb = __hci_cmd_sync(hdev, HCI_VENDOR_READ_RTK_ROM_VERISION, 0, NULL, HCI_INIT_TIMEOUT);

	if (IS_ERR(skb)) {
		ret_val = 0;
		global_eversion = 0;	
		RTKBT_DBG("%s rtk_get_eversion failed (%ld)",
		       hdev->name, PTR_ERR(skb)); 
		return 0;		
	}
	
	if (skb->len != sizeof(*eversion)) {
		RTKBT_DBG("%s RTK version event length mismatch", hdev->name);
		kfree_skb(skb);
		return -EIO;
	}

	eversion = (struct rtk_eversion_evt *)skb->data;
	RTKBT_DBG("rtk_get_eversion : eversion->status = 0x%x, eversion->version = 0x%x",eversion->status, eversion->version);
	if (eversion->status)
	{
		ret_val = 0;
		global_eversion = 0;
	}
	else 
	{
		ret_val =  eversion->version;
		global_eversion = eversion->version;
	}

	BT_DBG("rtk_get_eversion ret_val=%d \n",ret_val);
	return ret_val;

}

int load_firmware(struct hci_dev *hdev, uint8_t** buff)
{
	const struct firmware	*fw;
	patch_info	*patch_entry;
	char		*fw_name;
	int		fw_len = 0, ret_val;

	int config_len = 0 ,buf_len =-1;
	uint8_t* buf = *buff, *config_file_buf = NULL;
   	uint8_t* epatch_buf = NULL;

	struct rtk_epatch* epatch_info = NULL;
        uint8_t need_download_fw = 1;
	struct rtk_extension_entry patch_lmp = {0};
	struct rtk_epatch_entry current_entry = {0};
	uint16_t lmp_version ;
	uint8_t gEVersion= 0;
	
	struct btusb_data *data = dev_get_drvdata(&hdev->dev);
	struct usb_device * udev = interface_to_usbdev(data->intf);
	patch_entry = get_patch_entry(udev);	
	lmp_version = patch_entry->lmp_sub;

	RTKBT_ERR("load_firmware start ,lmp_version = 0x%04x", lmp_version);
	
	fw_name = patch_entry->config_name;
	ret_val = request_firmware(&fw, fw_name, &udev->dev);
	if (ret_val < 0)  
		config_len = 0;
	else
	{
		config_file_buf = kzalloc(fw->size, GFP_KERNEL);
		if (NULL == config_file_buf) goto alloc_fail;
			memcpy(config_file_buf, fw->data, fw->size);
		config_len = fw->size;
	}
	
	release_firmware(fw);
	fw_name = patch_entry->patch_name;
	ret_val = request_firmware(&fw, fw_name, &udev->dev);
	if (ret_val < 0)
	{
		fw_len = 0;
		kfree(config_file_buf);
		config_file_buf= NULL;
		goto fw_fail;
	}
	epatch_buf = kzalloc(fw->size, GFP_KERNEL);
	if (NULL == epatch_buf) goto alloc_fail;	
	memcpy(epatch_buf, fw->data, fw->size);	
	buf_len = fw->size + config_len;

	if(lmp_version == ROM_LMP_8723a)
	{
		RTKBT_ERR("This is 8723a, use old patch style!");
		if(memcmp(epatch_buf, RTK_EPATCH_SIGNATURE, 8) == 0)
		{
			RTKBT_ERR("8723as Check signature error!");
			need_download_fw = 0;
		}
		else
		{
			if (!(buf = kzalloc(buf_len, GFP_KERNEL))) {
				RTKBT_ERR("Can't alloc memory for fw&config");
				buf_len = -1;
			}
			else
			{
				RTKBT_DBG("8723as, fw copy direct");
				memcpy(buf,epatch_buf,buf_len);
				kfree(epatch_buf);
				epatch_buf = NULL;
				if (config_len)
				{
					memcpy(&buf[buf_len - config_len], config_file_buf, config_len);
				}	
			}
		}
	}

	else
	{
		RTKBT_ERR("This is not 8723a, use new patch style!");
		gEVersion = global_eversion;
		RTKBT_DBG("gEVersion=%d", gEVersion);

		//check Extension Section Field 
		if(memcmp(epatch_buf + buf_len-config_len-4 ,Extension_Section_SIGNATURE,4) != 0)
		{
			RTKBT_ERR("Check Extension_Section_SIGNATURE error! do not download fw");
			need_download_fw = 0;
		}
		else
		{
			uint8_t *temp;  
			temp = epatch_buf+buf_len-config_len-5;
			do{
				if(*temp == 0x00)
				{
					patch_lmp.opcode = *temp;
					patch_lmp.length = *(temp-1);
					if ((patch_lmp.data = kzalloc(patch_lmp.length, GFP_KERNEL)))
					{
						memcpy(patch_lmp.data,temp-2,patch_lmp.length);
					}
					RTKBT_DBG("opcode = 0x%x",patch_lmp.opcode); 
					RTKBT_DBG("length = 0x%x",patch_lmp.length);
					RTKBT_DBG("data = 0x%x",*(patch_lmp.data));
					break;
				}
				temp -= *(temp-1)+2;
			}while(*temp != 0xFF);

			if(lmp_version != project_id[*(patch_lmp.data)])
			{
				RTKBT_ERR("lmp_version is %x, project_id is %x, does not match!!!",lmp_version,project_id[*(patch_lmp.data)]);
				need_download_fw = 0;
			}
			else
			{
				RTKBT_DBG("lmp_version is %x, project_id is %x, match!",lmp_version, project_id[*(patch_lmp.data)]);
		
				if(memcmp(epatch_buf, RTK_EPATCH_SIGNATURE, 8) != 0)
				{
					RTKBT_DBG("Check signature error!");
					need_download_fw = 0;
				}
				else
				{
					int i = 0;
					epatch_info = (struct rtk_epatch*)epatch_buf;
					RTKBT_DBG("fm_version = 0x%x",epatch_info->fm_version); 
					RTKBT_DBG("number_of_total_patch = %d",epatch_info->number_of_total_patch);
			
					//get right epatch entry
					for(i=0; i<epatch_info->number_of_total_patch; i++)
					{
						if(*(uint16_t*)(epatch_buf+14+2*i) == gEVersion + 1)
						{
							current_entry.chipID = gEVersion + 1;
							current_entry.patch_length = *(uint16_t*)(epatch_buf+14+2*epatch_info->number_of_total_patch+2*i);
							current_entry.start_offset = *(uint32_t*)(epatch_buf+14+4*epatch_info->number_of_total_patch+4*i);
							break;
						}
					}
					RTKBT_DBG("chipID = %d",current_entry.chipID); 
					RTKBT_DBG("patch_length = 0x%x",current_entry.patch_length); 
					RTKBT_DBG("start_offset = 0x%x",current_entry.start_offset); 
									
					//get right eversion patch: buf, buf_len
					buf_len = current_entry.patch_length + config_len;
					RTKBT_DBG("buf_len = 0x%x",buf_len);
									
					if (!(buf = kzalloc(buf_len, GFP_KERNEL))) {
						RTKBT_ERR("Can't alloc memory for multi fw&config");
						buf_len = -1;
					}
					else
					{
						memcpy(buf,&epatch_buf[current_entry.start_offset],current_entry.patch_length);
						memcpy(&buf[current_entry.patch_length-4],&epatch_info->fm_version,4);
					}
					kfree(epatch_buf);
						epatch_buf = NULL;

					if (config_len)
					{
						memcpy(&buf[buf_len - config_len], config_file_buf, config_len);
					}	
				}															
			}
		}					
	}					
                
       if (config_file_buf)
        	 kfree(config_file_buf);
      
	RTKBT_ERR("Fw:%s exists, config file:%s exists", (buf_len > 0) ? "":"not", (config_len>0)?"":"not");
	if (buf && (buf_len > 0) && (need_download_fw))
	{
		fw_len = buf_len;	
		*buff = buf;
	}

	RTKBT_DBG("load_firmware done");

alloc_fail:
	release_firmware(fw);
fw_fail:
	return fw_len;
}

int check_fw_version(struct hci_dev* hdev)
{	
	struct hci_rp_read_local_version *read_ver_rsp;
	struct sk_buff * skb;
       int ret_val;
	patch_info	*patch_entry;

	struct btusb_data *data = dev_get_drvdata(&hdev->dev);
	struct usb_device * udev = interface_to_usbdev(data->intf);
	patch_entry = get_patch_entry(udev);

	skb = __hci_cmd_sync(hdev, HCI_OP_READ_LOCAL_VERSION, 0, NULL, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		RTKBT_DBG("%s reading rtk fw version command failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return -PTR_ERR(skb);
	}

	if (skb->len != sizeof(*read_ver_rsp)) {
		RTKBT_DBG("%s rtk version event length mismatch", hdev->name);
		kfree_skb(skb);
		return -EIO;
	}

	read_ver_rsp = (struct hci_rp_read_local_version *)skb->data;
	if (read_ver_rsp->status) {
		RTKBT_DBG("%s rtk fw version event failed (%02x)", hdev->name,
		       read_ver_rsp->status);
		kfree_skb(skb);
		return bt_to_errno(read_ver_rsp->status);
	}

	RTKBT_DBG("check_fw_version : read_ver_rsp->lmp_subver = 0x%x",read_ver_rsp->lmp_subver);
	RTKBT_DBG("check_fw_version : read_ver_rsp->status = 0x%x",read_ver_rsp->status);
	RTKBT_DBG("check_fw_version : read_ver_rsp->hci_ver = 0x%x",read_ver_rsp->hci_ver);
	RTKBT_DBG("check_fw_version : read_ver_rsp->hci_rev = 0x%x",read_ver_rsp->hci_rev);
	RTKBT_DBG("check_fw_version : read_ver_rsp->lmp_ver = 0x%x",read_ver_rsp->lmp_ver);
	RTKBT_DBG("check_fw_version : read_ver_rsp->manufacturer = 0x%x",read_ver_rsp->manufacturer);
	RTKBT_DBG("check_fw_version : read_ver_rsp->lmp_subver = 0x%x",read_ver_rsp->lmp_subver);
	RTKBT_DBG("check_fw_version : patch_entry->lmp_subver = 0x%x",patch_entry->lmp_sub);
	if (patch_entry->lmp_sub != read_ver_rsp->lmp_subver)
	{
		return 1;
	}

	ret_val = 0;	

	return ret_val;
}

int btusb_setup_rtk(struct hci_dev *hdev)
{
	int ret_val = 0;
	patch_info	*patch_entry;
	download_cp  *cmd_para;
	download_rp  *evt_para;
	uint8_t		*pcur;
	int	pkt_len, frag_num, frag_len;
	int	i;

	struct sk_buff *skb;
 	struct btusb_data *data = dev_get_drvdata(&hdev->dev);
	struct usb_device * udev = interface_to_usbdev(data->intf);
	patch_entry = get_patch_entry(udev);
		
	BT_DBG("btusb_setup_rtk\n");

	ret_val = check_fw_version(hdev);
	if(ret_val != 0) {
		RTKBT_DBG("check_fw_version 1 error");
		return 0;
	}
	

	ret_val = rtk_get_eversion(hdev);
	if(ret_val < 0) {
		RTKBT_DBG("rtk_get_eversion error");
		return 0;
	}


	patch_entry->fw_len=load_firmware(hdev ,&patch_entry->fw_cache);
	RTKBT_DBG("patch_entry->fw_len = %d", patch_entry->fw_len);
		if (patch_entry->fw_len <= 0) return -1;

	pkt_len = CMD_HDR_LEN + sizeof(download_cp);
	frag_num = patch_entry->fw_len / PATCH_SEG_MAX + 1;
	frag_len = PATCH_SEG_MAX;
	cmd_para = kzalloc(sizeof(download_cp), GFP_KERNEL);
	pcur = patch_entry->fw_cache;
	
	for (i = 0; i < frag_num; i++)
	{
		RTKBT_DBG("download fw (%d)/(%d)", (i), frag_num);
		cmd_para->index = i;
		if (i == (frag_num - 1))
		{
			cmd_para->index |= DATA_END;
			frag_len = patch_entry->fw_len % PATCH_SEG_MAX;
			pkt_len -= (PATCH_SEG_MAX - frag_len);
		}
		memcpy(cmd_para->data, pcur, frag_len);
		
		skb = __hci_cmd_sync(hdev, le16_to_cpu(DOWNLOAD_OPCODE), sizeof(uint8_t) + frag_len,
				cmd_para, HCI_INIT_TIMEOUT);
		if (IS_ERR(skb)) {
			RTKBT_DBG("%s download fw command failed (%ld)",hdev->name, PTR_ERR(skb));
			goto fail;
		}

		if (skb->len != sizeof(*evt_para)) {
			RTKBT_DBG("%s download fw  event length mismatch", hdev->name);
			kfree_skb(skb);
			goto fail;
		}

		evt_para = (download_rp *)skb->data;
		if (evt_para->status != 0) {
			kfree_skb(skb);
			goto fail;
		}

		kfree_skb(skb);
	
		pcur += PATCH_SEG_MAX;
	}

	
	ret_val = check_fw_version(hdev);
	if (ret_val <= 0)
	{
		ret_val = -1;
		goto fail;
	}

	kfree(cmd_para);
	RTKBT_DBG("patch download success!");
	return 0;
fail:
	kfree(cmd_para);
	RTKBT_DBG("patch download fail!");
	return -1;

return 0;
}

static int btusb_setup_patchram_packet(struct hci_dev *hdev, u16 opcode, u32 plen, const void *param)
{
	struct sk_buff *skb;

	skb = __hci_cmd_sync(hdev, opcode, plen, param, HCI_INIT_TIMEOUT);
	if (IS_ERR(skb))
		return PTR_ERR(skb);
	kfree_skb(skb);
	return 0;
}

#define PATCHRAM_NAME_LEN	20

static int btusb_setup_patchram(struct hci_dev *hdev)
{
	struct btusb_data *data = hci_get_drvdata(hdev);
	struct usb_device *udev = data->udev;
	size_t pos = 0;
	int err = 0;
	char filename[PATCHRAM_NAME_LEN];
	const struct firmware *fw;
	u8 val = 0x00;

	snprintf(filename, PATCHRAM_NAME_LEN, "fw-%04x_%04x.hcd",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct));
	if (request_firmware(&fw, (const char *) filename, &udev->dev) < 0) {
		BT_INFO("can't load firmware, may not work correctly");
		return 0;
	}

	err = btusb_setup_patchram_packet(hdev, 0x0c03, 1, &val);
	if (err)
		goto out;

	msleep(300);
	err = btusb_setup_patchram_packet(hdev, 0xfc2e, 1, &val);
	if (err)
		goto out;

	msleep(1000);
	while (pos < fw->size) {
		size_t len;
		len = fw->data[pos + 2] + 3;
		if (pos + len > fw->size) {
			err = -EINVAL;
			goto out;
		}
		err = btusb_setup_patchram_packet(hdev, le16_to_cpu(*(u16*)(fw->data + pos)),
							fw->data[pos + 2] , &fw->data[pos + 3]);
		if (err)
			goto out;
		pos += len;
	}

	err = btusb_setup_patchram_packet(hdev, 0x0c03, 1, &val);
out:
	release_firmware(fw);
	if (err) {
		BT_INFO("fail to load firmware");
		return err;
	}
	BT_INFO("firmware loaded");
	return 0;
}

static int btusb_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	struct usb_endpoint_descriptor *ep_desc;
	struct btusb_data *data;
	struct hci_dev *hdev;
	int i, err;

	BT_DBG("intf %p id %p", intf, id);

	/* interface numbers are hardcoded in the spec */
	if (intf->cur_altsetting->desc.bInterfaceNumber != 0)
		return -ENODEV;

	if (!id->driver_info) {
		const struct usb_device_id *match;
		match = usb_match_id(intf, blacklist_table);
		if (match)
			id = match;
	}

	if (id->driver_info == BTUSB_IGNORE)
		return -ENODEV;

	if (ignore_dga && id->driver_info & BTUSB_DIGIANSWER)
		return -ENODEV;

	if (ignore_csr && id->driver_info & BTUSB_CSR)
		return -ENODEV;

	if (ignore_sniffer && id->driver_info & BTUSB_SNIFFER)
		return -ENODEV;

	if (id->driver_info & BTUSB_ATH3012) {
		struct usb_device *udev = interface_to_usbdev(intf);

		/* Old firmware would otherwise let ath3k driver load
		 * patch and sysconfig files */
		if (le16_to_cpu(udev->descriptor.bcdDevice) <= 0x0001)
			return -ENODEV;
	}

	data = devm_kzalloc(&intf->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (!data->intr_ep && usb_endpoint_is_int_in(ep_desc)) {
			data->intr_ep = ep_desc;
			continue;
		}

		if (!data->bulk_tx_ep && usb_endpoint_is_bulk_out(ep_desc)) {
			data->bulk_tx_ep = ep_desc;
			continue;
		}

		if (!data->bulk_rx_ep && usb_endpoint_is_bulk_in(ep_desc)) {
			data->bulk_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->intr_ep || !data->bulk_tx_ep || !data->bulk_rx_ep)
		return -ENODEV;

	data->cmdreq_type = USB_TYPE_CLASS;

	data->udev = interface_to_usbdev(intf);
	data->intf = intf;

	spin_lock_init(&data->lock);

	INIT_WORK(&data->work, btusb_work);
	INIT_WORK(&data->waker, btusb_waker);
	spin_lock_init(&data->txlock);

	init_usb_anchor(&data->tx_anchor);
	init_usb_anchor(&data->intr_anchor);
	init_usb_anchor(&data->bulk_anchor);
	init_usb_anchor(&data->isoc_anchor);
	init_usb_anchor(&data->deferred);

	hdev = hci_alloc_dev();
	if (!hdev)
		return -ENOMEM;

	hdev->bus = HCI_USB;
	hci_set_drvdata(hdev, data);

	data->hdev = hdev;

	SET_HCIDEV_DEV(hdev, &intf->dev);

	hdev->open   = btusb_open;
	hdev->close  = btusb_close;
	hdev->flush  = btusb_flush;
	hdev->send   = btusb_send_frame;
	hdev->notify = btusb_notify;

	if (id->driver_info & BTUSB_BCM92035)
		hdev->setup = btusb_setup_bcm92035;

	if (id->driver_info & BTUSB_INTEL)
		hdev->setup = btusb_setup_intel;

	if (id->driver_info & BTUSB_BCM_PATCHRAM)
		hdev->setup = btusb_setup_patchram;

	if (id->driver_info & BTUSB_REALTEK)
		hdev->setup = btusb_setup_rtk;

	/* Interface numbers are hardcoded in the specification */
	data->isoc = usb_ifnum_to_if(data->udev, 1);

	if (!reset)
		set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);

	if (force_scofix || id->driver_info & BTUSB_WRONG_SCO_MTU) {
		if (!disable_scofix)
			set_bit(HCI_QUIRK_FIXUP_BUFFER_SIZE, &hdev->quirks);
	}

	if (id->driver_info & BTUSB_BROKEN_ISOC)
		data->isoc = NULL;

	if (id->driver_info & BTUSB_DIGIANSWER) {
		data->cmdreq_type = USB_TYPE_VENDOR;
		set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);
	}

	if (id->driver_info & BTUSB_CSR) {
		struct usb_device *udev = data->udev;

		/* Old firmware would otherwise execute USB reset */
		if (le16_to_cpu(udev->descriptor.bcdDevice) < 0x117)
			set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);
	}

	if (id->driver_info & BTUSB_SNIFFER) {
		struct usb_device *udev = data->udev;

		/* New sniffer firmware has crippled HCI interface */
		if (le16_to_cpu(udev->descriptor.bcdDevice) > 0x997)
			set_bit(HCI_QUIRK_RAW_DEVICE, &hdev->quirks);

		data->isoc = NULL;
	}

	if (data->isoc) {
		err = usb_driver_claim_interface(&btusb_driver,
							data->isoc, data);
		if (err < 0) {
			hci_free_dev(hdev);
			return err;
		}
	}

	err = hci_register_dev(hdev);
	if (err < 0) {
		hci_free_dev(hdev);
		return err;
	}

	usb_set_intfdata(intf, data);

	return 0;
}

static void btusb_disconnect(struct usb_interface *intf)
{
	struct btusb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev;

	BT_DBG("intf %p", intf);

	if (!data)
		return;

	hdev = data->hdev;
	usb_set_intfdata(data->intf, NULL);

	if (data->isoc)
		usb_set_intfdata(data->isoc, NULL);

	hci_unregister_dev(hdev);

	if (intf == data->isoc)
		usb_driver_release_interface(&btusb_driver, data->intf);
	else if (data->isoc)
		usb_driver_release_interface(&btusb_driver, data->isoc);

	hci_free_dev(hdev);
}

#ifdef CONFIG_PM
static int btusb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct btusb_data *data = usb_get_intfdata(intf);

	BT_DBG("intf %p", intf);

	if (data->suspend_count++)
		return 0;

	spin_lock_irq(&data->txlock);
	if (!(PMSG_IS_AUTO(message) && data->tx_in_flight)) {
		set_bit(BTUSB_SUSPENDING, &data->flags);
		spin_unlock_irq(&data->txlock);
	} else {
		spin_unlock_irq(&data->txlock);
		data->suspend_count--;
		return -EBUSY;
	}

	cancel_work_sync(&data->work);

	btusb_stop_traffic(data);
	usb_kill_anchored_urbs(&data->tx_anchor);

	return 0;
}

static void play_deferred(struct btusb_data *data)
{
	struct urb *urb;
	int err;

	while ((urb = usb_get_from_anchor(&data->deferred))) {
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (err < 0)
			break;

		data->tx_in_flight++;
	}
	usb_scuttle_anchored_urbs(&data->deferred);
}

static int btusb_resume(struct usb_interface *intf)
{
	struct btusb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev = data->hdev;
	int err = 0;

	BT_DBG("intf %p", intf);

	if (--data->suspend_count)
		return 0;

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (test_bit(BTUSB_INTR_RUNNING, &data->flags)) {
		err = btusb_submit_intr_urb(hdev, GFP_NOIO);
		if (err < 0) {
			clear_bit(BTUSB_INTR_RUNNING, &data->flags);
			goto failed;
		}
	}

	if (test_bit(BTUSB_BULK_RUNNING, &data->flags)) {
		err = btusb_submit_bulk_urb(hdev, GFP_NOIO);
		if (err < 0) {
			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
			goto failed;
		}

		btusb_submit_bulk_urb(hdev, GFP_NOIO);
	}

	if (test_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
		if (btusb_submit_isoc_urb(hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		else
			btusb_submit_isoc_urb(hdev, GFP_NOIO);
	}

	spin_lock_irq(&data->txlock);
	play_deferred(data);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);
	schedule_work(&data->work);

	return 0;

failed:
	usb_scuttle_anchored_urbs(&data->deferred);
done:
	spin_lock_irq(&data->txlock);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);

	return err;
}
#endif

static struct usb_driver btusb_driver = {
	.name		= "btusb",
	.probe		= btusb_probe,
	.disconnect	= btusb_disconnect,
#ifdef CONFIG_PM
	.suspend	= btusb_suspend,
	.resume		= btusb_resume,
#endif
	.id_table	= btusb_table,
	.supports_autosuspend = 1,
	.disable_hub_initiated_lpm = 1,
};

module_usb_driver(btusb_driver);

module_param(ignore_dga, bool, 0644);
MODULE_PARM_DESC(ignore_dga, "Ignore devices with id 08fd:0001");

module_param(ignore_csr, bool, 0644);
MODULE_PARM_DESC(ignore_csr, "Ignore devices with id 0a12:0001");

module_param(ignore_sniffer, bool, 0644);
MODULE_PARM_DESC(ignore_sniffer, "Ignore devices with id 0a12:0002");

module_param(disable_scofix, bool, 0644);
MODULE_PARM_DESC(disable_scofix, "Disable fixup of wrong SCO buffer size");

module_param(force_scofix, bool, 0644);
MODULE_PARM_DESC(force_scofix, "Force fixup of wrong SCO buffers size");

module_param(reset, bool, 0644);
MODULE_PARM_DESC(reset, "Send HCI reset command on initialization");

MODULE_AUTHOR("Marcel Holtmann <marcel@holtmann.org>");
MODULE_DESCRIPTION("Generic Bluetooth USB driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
