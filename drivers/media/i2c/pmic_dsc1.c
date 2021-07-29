// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020 Intel Corporation.

#include <linux/delay.h>
#include <linux/module.h>
#include "pmic_dsc1.h"

/* mcu gpio resources*/
static const struct acpi_gpio_params camreset_gpio  = { 0, 0, false };
static const struct acpi_gpio_params campwdn_gpio   = { 1, 0, false };
static const struct acpi_gpio_params midmclken_gpio = { 2, 0, false };
static const struct acpi_gpio_params led_gpio       = { 3, 0, false };
static const struct acpi_gpio_mapping dsc1_acpi_gpios[] = {
	{ "camreset-gpios", &camreset_gpio, 1 },
	{ "campwdn-gpios", &campwdn_gpio, 1 },
	{ "midmclken-gpios", &midmclken_gpio, 1 },
	{ "indled-gpios", &led_gpio, 1 },
	{ }
};

static const char * const pin_names[] = {
	"camreset", "campwdn", "midmclken", "indled"
};

struct pmic_dsc1 pmic = {
	.reset_gpio = NULL,
	.powerdn_gpio = NULL,
	.clocken_gpio = NULL,
	.indled_gpio = NULL,
	.power_on = false,
	.gpio_ready = false,
};

static int get_gpio_pin(struct gpio_desc **pin_d, struct pci_dev *pdev, int idx)
{
	int count = PMIC_DSC1_PROBE_MAX_TRY;

	do {
		dev_dbg(&pdev->dev, "get %s:tried once\n", pin_names[idx]);
		*pin_d = devm_gpiod_get(&pdev->dev, pin_names[idx],
					GPIOD_OUT_LOW);
		if (!IS_ERR(*pin_d))
			return 0;
		*pin_d = NULL;
		msleep_interruptible(PMIC_DSC1_PROBE_TRY_GAP);
	} while (--count > 0);

	return -EBUSY;
}

static int pmic_dsc1_probe(struct pci_dev *pdev,
			   const struct pci_device_id *id)
{
	int ret;

	if (!pdev) {
		dev_err(&pdev->dev, "@%s: probed null pdev %x:%x\n",
			__func__, PCI_BRG_VENDOR_ID, PCI_BRG_PRODUCT_ID);
		return -ENODEV;
	}
	dev_dbg(&pdev->dev, "@%s, enter\n", __func__);

	ret = devm_acpi_dev_add_driver_gpios(&pdev->dev, dsc1_acpi_gpios);
	if (ret) {
		dev_err(&pdev->dev, "@%s: fail to add gpio\n", __func__);
		return -EBUSY;
	}
	ret = get_gpio_pin(&pmic.reset_gpio, pdev, 0);
	if (ret)
		goto pmic_dsc1_probe_end;
	ret = get_gpio_pin(&pmic.powerdn_gpio, pdev, 1);
	if (ret)
		goto pmic_dsc1_probe_end;
	ret = get_gpio_pin(&pmic.clocken_gpio, pdev, 2);
	if (ret)
		goto pmic_dsc1_probe_end;
	ret = get_gpio_pin(&pmic.indled_gpio, pdev, 3);
	if (ret)
		goto pmic_dsc1_probe_end;

	mutex_lock(&pmic.status_lock);
	pmic.gpio_ready = true;
	mutex_unlock(&pmic.status_lock);

pmic_dsc1_probe_end:
	dev_dbg(&pdev->dev, "@%s, exit\n", __func__);
	return ret;
}

static void pmic_dsc1_remove(struct pci_dev *pdev)
{
	dev_dbg(&pdev->dev, "@%s, enter\n", __func__);
	mutex_lock(&pmic.status_lock);
	pmic.gpio_ready = false;
	gpiod_set_value_cansleep(pmic.reset_gpio, 0);
	gpiod_put(pmic.reset_gpio);
	gpiod_set_value_cansleep(pmic.powerdn_gpio, 0);
	gpiod_put(pmic.powerdn_gpio);
	gpiod_set_value_cansleep(pmic.clocken_gpio, 0);
	gpiod_put(pmic.clocken_gpio);
	gpiod_set_value_cansleep(pmic.indled_gpio, 0);
	gpiod_put(pmic.indled_gpio);
	mutex_unlock(&pmic.status_lock);
	dev_dbg(&pdev->dev, "@%s, exit\n", __func__);
}

static struct pci_device_id pmic_dsc1_ids[] = {
	{ PCI_DEVICE(PCI_BRG_VENDOR_ID, PCI_BRG_PRODUCT_ID) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, pmic_dsc1_ids);

static struct pci_driver pmic_dsc1_driver = {
	.name     = PMIC_DRV_NAME,
	.id_table = pmic_dsc1_ids,
	.probe    = pmic_dsc1_probe,
	.remove   = pmic_dsc1_remove,
};

static int __init pmic_dsc1_init(void)
{
	mutex_init(&pmic.status_lock);
	return pci_register_driver(&pmic_dsc1_driver);
}

static void __exit pmic_dsc1_exit(void)
{
	pci_unregister_driver(&pmic_dsc1_driver);
}
module_init(pmic_dsc1_init);
module_exit(pmic_dsc1_exit);

int pmic_dsc1_set_power(int on)
{
	mutex_lock(&pmic.status_lock);
	if (!pmic.gpio_ready || on < 0 || on > 1) {
		pr_debug("@%s,failed to set power, gpio_ready=%d, on=%d\n",
			 __func__, pmic.gpio_ready, on);
		mutex_unlock(&pmic.status_lock);
		return -EBUSY;
	}
	if (pmic.power_on != on) {
		gpiod_set_value_cansleep(pmic.reset_gpio, on);
		gpiod_set_value_cansleep(pmic.powerdn_gpio, on);
		gpiod_set_value_cansleep(pmic.clocken_gpio, on);
		gpiod_set_value_cansleep(pmic.indled_gpio, on);
		pmic.power_on = on;
	}
	mutex_unlock(&pmic.status_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(pmic_dsc1_set_power);

MODULE_AUTHOR("Bingbu Cao <bingbu.cao@intel.com>");
MODULE_AUTHOR("Qiu, Tianshu <tian.shu.qiu@intel.com>");
MODULE_AUTHOR("Xu, Chongyang <chongyang.xu@intel.com>");
MODULE_DESCRIPTION("PMIC-CRDG DSC1 driver");
MODULE_LICENSE("GPL v2");
