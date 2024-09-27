// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2024 Maya Matuszczyk <maya.matuszczyk@gmail.com>
 */
#include <linux/auxiliary_bus.h>
#include <linux/cleanup.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irqreturn.h>
#include <linux/lockdep.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/input.h>
//#include <linux/platform_data/lenovo-yoga-slim7x.h>

// These are the registers that i know about available from SMBUS
#define EC_IRQ_REASON_REG 0x05
#define EC_SUSPEND_RESUME_REG 0x23
#define EC_IRQ_ENABLE_REG 0x35
#define EC_BACKLIGHT_STATUS_REG 0x83
#define EC_MIC_MUTE_LED_REG 0x84
#define EC_AC_STATUS_REG 0x90

// Valid values for EC_SUSPEND_RESUME_REG
#define EC_NOTIFY_SUSPEND_ENTER 0x01
#define EC_NOTIFY_SUSPEND_EXIT 0x00
#define EC_NOTIFY_SCREEN_OFF 0x03
#define EC_NOTIFY_SCREEN_ON 0x04

// These are the values in EC_IRQ_REASON_REG that i could find in DSDT
#define EC_IRQ_MICMUTE_BUTTON 0x04
#define EC_IRQ_FAN1_STATUS_CHANGE 0x30
#define EC_IRQ_FAN2_STATUS_CHANGE 0x31
#define EC_IRQ_FAN1_SPEED_CHANGE 0x32
#define EC_IRQ_FAN2_SPEED_CHANGE 0x33
#define EC_IRQ_COMPLETED_LUT_UPDATE 0x34
#define EC_IRQ_COMPLETED_FAN_PROFILE_SWITCH 0x35
#define EC_IRQ_THERMISTOR_1_TEMP_THRESHOLD_CROSS 0x36
#define EC_IRQ_THERMISTOR_2_TEMP_THRESHOLD_CROSS 0x37
#define EC_IRQ_THERMISTOR_3_TEMP_THRESHOLD_CROSS 0x38
#define EC_IRQ_THERMISTOR_4_TEMP_THRESHOLD_CROSS 0x39
#define EC_IRQ_THERMISTOR_5_TEMP_THRESHOLD_CROSS 0x3a
#define EC_IRQ_THERMISTOR_6_TEMP_THRESHOLD_CROSS 0x3b
#define EC_IRQ_THERMISTOR_7_TEMP_THRESHOLD_CROSS 0x3c
#define EC_IRQ_RECOVERED_FROM_RESET 0x3d
#define EC_IRQ_LENOVO_SUPPORT_KEY 0x90
#define EC_IRQ_FN_Q 0x91
#define EC_IRQ_FN_M 0x92
#define EC_IRQ_FN_SPACE 0x93
#define EC_IRQ_FN_R 0x94
#define EC_IRQ_FNLOCK_ON 0x95
#define EC_IRQ_FNLOCK_OFF 0x96
#define EC_IRQ_FN_N 0x97
#define EC_IRQ_AI 0x9a
#define EC_IRQ_NPU 0x9b

struct yoga_slim7x_ec {
	struct i2c_client *client;
	struct input_dev *idev;
	struct mutex lock;
};

static irqreturn_t yoga_slim7x_ec_irq(int irq, void *data)
{
	struct yoga_slim7x_ec *ec = data;
	struct device *dev = &ec->client->dev;
	int val;

	guard(mutex)(&ec->lock);

	val = i2c_smbus_read_byte_data(ec->client, EC_IRQ_REASON_REG);
	if (val < 0) {
		dev_err(dev, "Failed to get EC IRQ reason: %d\n", val);
		return IRQ_HANDLED;
	}

	switch (val) {
	case EC_IRQ_MICMUTE_BUTTON:
		input_report_key(ec->idev, KEY_MICMUTE, 1);
		input_sync(ec->idev);
		input_report_key(ec->idev, KEY_MICMUTE, 0);
		input_sync(ec->idev);
		break;
	default:
		dev_info(dev, "Unhandled EC IRQ reason: %d\n", val);
	}

	return IRQ_HANDLED;
}

static int yoga_slim7x_ec_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct yoga_slim7x_ec *ec;
	int ret;

	ec = devm_kzalloc(dev, sizeof(*ec), GFP_KERNEL);
	if (!ec)
		return -ENOMEM;

	mutex_init(&ec->lock);
	ec->client = client;

	ec->idev = devm_input_allocate_device(dev);
	if (!ec->idev)
		return -ENOMEM;
	ec->idev->name = "yoga-slim7x-ec";
	ec->idev->phys = "yoga-slim7x-ec/input0";
	input_set_capability(ec->idev, EV_KEY, KEY_MICMUTE);

	ret = input_register_device(ec->idev);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to register input device\n");

	ret = devm_request_threaded_irq(dev, client->irq,
					NULL, yoga_slim7x_ec_irq,
					IRQF_ONESHOT, "yoga_slim7x_ec", ec);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Unable to request irq\n");

	ret = i2c_smbus_write_byte_data(client, EC_IRQ_ENABLE_REG, 0x01);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to enable interrupts\n");

	return 0;
}

static void yoga_slim7x_ec_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	int ret;

	ret = i2c_smbus_write_byte_data(client, EC_IRQ_ENABLE_REG, 0x00);
	if (ret < 0)
		dev_err(dev, "Failed to disable interrupts: %d\n", ret);
}

static int yoga_slim7x_ec_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret;

	ret = i2c_smbus_write_byte_data(client, EC_SUSPEND_RESUME_REG, EC_NOTIFY_SCREEN_OFF);
	if (ret)
		return ret;

	ret = i2c_smbus_write_byte_data(client, EC_SUSPEND_RESUME_REG, EC_NOTIFY_SUSPEND_ENTER);
	if (ret)
		return ret;

	return 0;
}

static int yoga_slim7x_ec_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret;

	ret = i2c_smbus_write_byte_data(client, EC_SUSPEND_RESUME_REG, EC_NOTIFY_SUSPEND_EXIT);
	if (ret)
		return ret;

	ret = i2c_smbus_write_byte_data(client, EC_SUSPEND_RESUME_REG, EC_NOTIFY_SCREEN_ON);
	if (ret)
		return ret;

	return 0;
}

static const struct of_device_id yoga_slim7x_ec_of_match[] = {
	{ .compatible = "lenovo,yoga-slim7x-ec" },
	{}
};
MODULE_DEVICE_TABLE(of, yoga_slim7x_ec_of_match);

static const struct i2c_device_id yoga_slim7x_ec_i2c_id_table[] = {
	{ "yoga-slim7x-ec", },
	{}
};
MODULE_DEVICE_TABLE(i2c, yoga_slim7x_ec_i2c_id_table);

static DEFINE_SIMPLE_DEV_PM_OPS(yoga_slim7x_ec_pm_ops,
		yoga_slim7x_ec_suspend,
		yoga_slim7x_ec_resume);

static struct i2c_driver yoga_slim7x_ec_i2c_driver = {
	.driver = {
		.name = "yoga-slim7x-ec",
		.of_match_table = yoga_slim7x_ec_of_match,
		.pm = &yoga_slim7x_ec_pm_ops
	},
	.probe = yoga_slim7x_ec_probe,
	.remove = yoga_slim7x_ec_remove,
	.id_table = yoga_slim7x_ec_i2c_id_table,
};
module_i2c_driver(yoga_slim7x_ec_i2c_driver);

MODULE_DESCRIPTION("Lenovo Yoga Slim 7x Embedded Controller");
MODULE_LICENSE("GPL");
