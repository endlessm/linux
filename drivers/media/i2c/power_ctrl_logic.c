// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020-2021 Intel Corporation.

#include <linux/delay.h>
#include <linux/module.h>
#include "power_ctrl_logic.h"

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

static struct power_ctrl_logic pcl = {
	.reset_gpio = NULL,
	.powerdn_gpio = NULL,
	.clocken_gpio = NULL,
	.indled_gpio = NULL,
	.power_on = false,
	.gpio_ready = false,
};

static int get_gpio_pin(struct gpio_desc **pin_d, struct pci_dev *pdev, int idx)
{
	int count = PCL_PROBE_MAX_TRY;

	do {
		dev_dbg(&pdev->dev, "get %s:tried once\n", pin_names[idx]);
		*pin_d = devm_gpiod_get(&pdev->dev, pin_names[idx],
					GPIOD_OUT_LOW);
		if (!IS_ERR(*pin_d))
			return 0;
		*pin_d = NULL;
		msleep_interruptible(PCL_PROBE_TRY_GAP);
	} while (--count > 0);

	return -EBUSY;
}

static int power_ctrl_logic_probe(struct pci_dev *pdev,
				  const struct pci_device_id *id)
{
	int ret;

	if (!pdev) {
		dev_err(&pdev->dev, "@%s: probed null pdev %x:%x\n",
			__func__, PCL_PCI_BRG_VEN_ID, PCL_PCI_BRG_PDT_ID);
		return -ENODEV;
	}
	dev_dbg(&pdev->dev, "@%s, enter\n", __func__);

	ret = devm_acpi_dev_add_driver_gpios(&pdev->dev, dsc1_acpi_gpios);
	if (ret) {
		dev_err(&pdev->dev, "@%s: fail to add gpio\n", __func__);
		return -EBUSY;
	}
	ret = get_gpio_pin(&pcl.reset_gpio, pdev, 0);
	if (ret)
		goto power_ctrl_logic_probe_end;
	ret = get_gpio_pin(&pcl.powerdn_gpio, pdev, 1);
	if (ret)
		goto power_ctrl_logic_probe_end;
	ret = get_gpio_pin(&pcl.clocken_gpio, pdev, 2);
	if (ret)
		goto power_ctrl_logic_probe_end;
	ret = get_gpio_pin(&pcl.indled_gpio, pdev, 3);
	if (ret)
		goto power_ctrl_logic_probe_end;

	mutex_lock(&pcl.status_lock);
	pcl.gpio_ready = true;
	mutex_unlock(&pcl.status_lock);

power_ctrl_logic_probe_end:
	dev_dbg(&pdev->dev, "@%s, exit\n", __func__);
	return ret;
}

static void power_ctrl_logic_remove(struct pci_dev *pdev)
{
	dev_dbg(&pdev->dev, "@%s, enter\n", __func__);
	mutex_lock(&pcl.status_lock);
	pcl.gpio_ready = false;
	gpiod_set_value_cansleep(pcl.reset_gpio, 0);
	gpiod_put(pcl.reset_gpio);
	gpiod_set_value_cansleep(pcl.powerdn_gpio, 0);
	gpiod_put(pcl.powerdn_gpio);
	gpiod_set_value_cansleep(pcl.clocken_gpio, 0);
	gpiod_put(pcl.clocken_gpio);
	gpiod_set_value_cansleep(pcl.indled_gpio, 0);
	gpiod_put(pcl.indled_gpio);
	mutex_unlock(&pcl.status_lock);
	dev_dbg(&pdev->dev, "@%s, exit\n", __func__);
}

static struct pci_device_id power_ctrl_logic_ids[] = {
	{ PCI_DEVICE(PCL_PCI_BRG_VEN_ID, PCL_PCI_BRG_PDT_ID) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, power_ctrl_logic_ids);

static struct pci_driver power_ctrl_logic_driver = {
	.name     = PCL_DRV_NAME,
	.id_table = power_ctrl_logic_ids,
	.probe    = power_ctrl_logic_probe,
	.remove   = power_ctrl_logic_remove,
};

static int __init power_ctrl_logic_init(void)
{
	mutex_init(&pcl.status_lock);
	return pci_register_driver(&power_ctrl_logic_driver);
}

static void __exit power_ctrl_logic_exit(void)
{
	pci_unregister_driver(&power_ctrl_logic_driver);
}
module_init(power_ctrl_logic_init);
module_exit(power_ctrl_logic_exit);

int power_ctrl_logic_set_power(int on)
{
	mutex_lock(&pcl.status_lock);
	if (!pcl.gpio_ready || on < 0 || on > 1) {
		pr_debug("@%s,failed to set power, gpio_ready=%d, on=%d\n",
			 __func__, pcl.gpio_ready, on);
		mutex_unlock(&pcl.status_lock);
		return -EBUSY;
	}
	if (pcl.power_on != on) {
		gpiod_set_value_cansleep(pcl.reset_gpio, on);
		gpiod_set_value_cansleep(pcl.powerdn_gpio, on);
		gpiod_set_value_cansleep(pcl.clocken_gpio, on);
		gpiod_set_value_cansleep(pcl.indled_gpio, on);
		pcl.power_on = on;
	}
	mutex_unlock(&pcl.status_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(power_ctrl_logic_set_power);

MODULE_AUTHOR("Bingbu Cao <bingbu.cao@intel.com>");
MODULE_AUTHOR("Qiu, Tianshu <tian.shu.qiu@intel.com>");
MODULE_AUTHOR("Xu, Chongyang <chongyang.xu@intel.com>");
MODULE_DESCRIPTION("Power Control Logic Driver");
MODULE_LICENSE("GPL v2");
