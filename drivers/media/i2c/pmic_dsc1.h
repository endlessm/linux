/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 Intel Corporation. */

#ifndef _PMIC_DSC1_H_
#define _PMIC_DSC1_H_

#include <linux/gpio/consumer.h>
#include <linux/mutex.h>
#include <linux/pci.h>

/* pmic dsc1 pci id */
#define PCI_BRG_VENDOR_ID  0x8086
#define PCI_BRG_PRODUCT_ID 0x9a14

#define PMIC_DRV_NAME "pmic_dsc1"
#define PMIC_DSC1_PROBE_MAX_TRY 5
#define PMIC_DSC1_PROBE_TRY_GAP 500 /* in millseconds */

struct pmic_dsc1 {
	/* gpio resource*/
	struct gpio_desc *reset_gpio;
	struct gpio_desc *powerdn_gpio;
	struct gpio_desc *clocken_gpio;
	struct gpio_desc *indled_gpio;
	/* status */
	struct mutex status_lock;
	bool power_on;
	bool gpio_ready;
};

/* exported function for extern module */
int pmic_dsc1_set_power(int on);
#endif
