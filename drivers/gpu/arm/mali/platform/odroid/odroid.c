/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_kernel_linux.h"
#include "mali_osk.h"

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>

static struct clk *sclk_g3d_clock = NULL;

/* Please take special care lowering these values, specially the voltage
 * as it can cause system stability problems: random oops, usb hub resets */
int mali_gpu_clk = 533; /* 533 MHz */
int mali_gpu_vol = 1125000;/* 1.1125 V */

struct regulator *g3d_regulator = NULL;

_mali_osk_mutex_t *mali_dvfs_lock = 0;

static void mali_regulator_set_voltage(int min_uV, int max_uV)
{
	int voltage;

	_mali_osk_mutex_wait(mali_dvfs_lock);

	MALI_DEBUG_PRINT(2, ("= regulator_set_voltage: %d, %d \n",min_uV, max_uV));
	regulator_set_voltage(g3d_regulator,min_uV,max_uV);
	voltage = regulator_get_voltage(g3d_regulator);
	mali_gpu_vol = voltage;
	MALI_DEBUG_PRINT(1, ("= regulator_get_voltage: %d \n",mali_gpu_vol));

	_mali_osk_mutex_signal(mali_dvfs_lock);
}

static int mali_clk_enable(void)
{
	struct device *dev = &mali_platform_device->dev;
	unsigned long rate;
	int ret = 0;

	sclk_g3d_clock = clk_get(dev, "sclk_g3d");
	if (IS_ERR(sclk_g3d_clock)) {
		MALI_PRINT( ("MALI Error : failed to get source mali clock\n"));
		return PTR_ERR(sclk_g3d_clock);
	}

	_mali_osk_mutex_wait(mali_dvfs_lock);

	ret = clk_prepare_enable(sclk_g3d_clock);
	if (ret < 0) {
		printk("~~~~~~~~ERROR: [%s] %d\n ",__func__,__LINE__);
		clk_put(sclk_g3d_clock);
		goto out;
	}

	rate = clk_get_rate(sclk_g3d_clock);
	mali_gpu_clk = (int)(rate / 1000000);

	MALI_DEBUG_PRINT(2,("= clk_get_rate: %d \n",mali_gpu_clk));

out:
	_mali_osk_mutex_signal(mali_dvfs_lock);

	return ret;
}

int mali_platform_device_init(struct platform_device *pdev)
{
	int ret = 0;

	mali_dvfs_lock = _mali_osk_mutex_init(0, 0);
	if (mali_dvfs_lock == NULL)
		return -ENOMEM;

	ret = mali_clk_enable();
	if (ret)
		return ret;

	MALI_PRINT(("init_mali_clock\n"));

	g3d_regulator = regulator_get(NULL, "vdd_g3d");
	if (IS_ERR(g3d_regulator)) {
		MALI_PRINT( ("MALI Error : failed to get vdd_g3d\n"));
		ret = PTR_ERR(g3d_regulator);
		goto err_regulator;
	}

	regulator_enable(g3d_regulator);
	mali_regulator_set_voltage(mali_gpu_vol, mali_gpu_vol);

	MALI_DEBUG_PRINT(2, ("MALI Clock is set at mali driver\n"));

	pm_runtime_set_autosuspend_delay(&mali_platform_device->dev, 1000);
	pm_runtime_use_autosuspend(&mali_platform_device->dev);
	pm_runtime_enable(&mali_platform_device->dev);
	return 0;

err_regulator:
	regulator_put(g3d_regulator);
	return ret;
}

int mali_platform_device_deinit(struct platform_device *pdev)
{
	pm_runtime_disable(&(mali_platform_device->dev));

	if (sclk_g3d_clock) {
		clk_disable_unprepare(sclk_g3d_clock);
		clk_put(sclk_g3d_clock);
		sclk_g3d_clock = NULL;
	}

	if (g3d_regulator) {
		regulator_put(g3d_regulator);
		g3d_regulator = NULL;
	}

	return 0;
}
