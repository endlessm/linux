// SPDX-License-Identifier: GPL-2.0
/*
 *  Endless watchdog device
 *
 *  Copyright (C) 2019 Endless Mobile, Inc.
 *  Author: Derek Foreman <derek@endlessm.com>
 */
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/types.h>
#include <linux/watchdog.h>

/*
 * 19 minutes until a soft shutdown attempt
 * 60 seconds after that we assume failure and shut down hard
 */
#define BARK_TIMEOUT 1140
#define BITE_TIMEOUT 60

static bool running = false;
static bool destroyed = false;

static struct work_struct endlessdog_bite;

static struct hrtimer bark;

static int reboot_me_harder = false;

static enum hrtimer_restart endlessdog_bark(struct hrtimer *t)
{
	printk("xsss: successfully activated (9)\n");
	hrtimer_start(&bark, ktime_set(BITE_TIMEOUT, 0), HRTIMER_MODE_REL);
	schedule_work(&endlessdog_bite);
	return HRTIMER_NORESTART;
}

static int endlessdog_start(struct watchdog_device *w)
{
	if (!running) {
		__module_get(THIS_MODULE);
		running = true;
	}

	if (destroyed)
		return 0;

	hrtimer_start(&bark, ktime_set(BARK_TIMEOUT, 0), HRTIMER_MODE_REL);
	return 0;
}

static int endlessdog_stop(struct watchdog_device *w)
{
	if (destroyed)
		return 0;
	printk("xsss: successfully activated (7)\n");
	destroyed = true;
	return 0;
}

static int endlessdog_ping(struct watchdog_device *w)
{
	if (destroyed)
		return 0;
	hrtimer_start(&bark, ktime_set(BARK_TIMEOUT, 0), HRTIMER_MODE_REL);
	return 0;
}

static struct watchdog_info endlessdog_info = {
	.identity = "Endless Watchdog",
	.options = WDIOF_KEEPALIVEPING,
};

static struct watchdog_ops endlessdog_ops = {
	.owner = THIS_MODULE,
	.start = endlessdog_start,
	.stop = endlessdog_stop,
	.ping = endlessdog_ping,
};

static struct watchdog_device endlessdog_dev = {
	.info = &endlessdog_info,
	.ops = &endlessdog_ops,
	.timeout = BARK_TIMEOUT,
};

MODULE_AUTHOR("Derek Foreman <derek@endlessm.com>");
MODULE_DESCRIPTION("Endless Watchdog");
MODULE_LICENSE("GPL v2");

static void endlessdog_power_off(struct work_struct *w)
{
	if (!reboot_me_harder) {
		orderly_poweroff(true);
		reboot_me_harder = true;
	} else
		kernel_power_off();
}

static int __init endlessdog_init(void)
{
	/*
	 * nowayout to false because we want the stop call to happen so
	 * we can break the ping() and start() functionality
	 */
	watchdog_set_nowayout(&endlessdog_dev, false);
	hrtimer_init(&bark,
		     CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);
	bark.function = endlessdog_bark;

	INIT_WORK(&endlessdog_bite, endlessdog_power_off);
	return watchdog_register_device(&endlessdog_dev);
}
module_init(endlessdog_init);

static void __exit endlessdog_exit(void)
{
	watchdog_unregister_device(&endlessdog_dev);
}
module_exit(endlessdog_exit);
