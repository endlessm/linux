/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020-2021 Intel Corporation. */

#ifndef _POWER_CTRL_LOGIC_H_
#define _POWER_CTRL_LOGIC_H_

#include <linux/gpio/consumer.h>
#include <linux/mutex.h>
#include <linux/pci.h>

/* pci id for probe power control logic*/
#define PCL_PCI_BRG_VEN_ID 0x8086
#define PCL_PCI_BRG_PDT_ID 0x9a14

#define PCL_DRV_NAME "power_ctrl_logic"
#define PCL_PROBE_MAX_TRY 5
#define PCL_PROBE_TRY_GAP 500 /* in millseconds */

struct power_ctrl_logic {
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
int power_ctrl_logic_set_power(int on);
#endif
