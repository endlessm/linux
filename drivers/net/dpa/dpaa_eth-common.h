/*
 * Copyright 2008-2011 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DPA_COMMON_H
#define __DPA_COMMON_H

#include <linux/kernel.h>	/* pr_*() */
#include <linux/device.h>	/* dev_*() */
#include <linux/smp.h>		/* smp_processor_id() */

/* The basename of the source file is being compiled */
#define __file__ KBUILD_BASENAME".c"

#define __hot

#define cpu_printk(level, format, arg...) \
	pr_##level("cpu%d: " format, smp_processor_id(), ##arg)

#define cpu_pr_emerg(format, arg...)	\
	cpu_printk(emerg, format, ##arg)
#define cpu_pr_alert(format, arg...)	\
	cpu_printk(alert, format, ##arg)
#define cpu_pr_crit(format, arg...)	\
	cpu_printk(crit, format, ##arg)
#define cpu_pr_err(format, arg...)	\
	cpu_printk(err, format, ##arg)
#define cpu_pr_warning(format, arg...)	\
	cpu_printk(warning, format, ##arg)
#define cpu_pr_notice(format, arg...)	\
	cpu_printk(notice, format, ##arg)
#define cpu_pr_info(format, arg...)	\
	cpu_printk(info, format, ##arg)
#define cpu_pr_debug(format, arg...)	\
	cpu_printk(debug, format, ##arg)

/* Keep this in sync with the dev_*() definitions from linux/device.h */
#define cpu_dev_printk(level, dev, format, arg...) \
	cpu_pr_##level("%s: %s: " format, dev_driver_string(dev), \
			dev_name(dev), ##arg)

#define cpu_dev_emerg(dev, format, arg...)	\
	cpu_dev_printk(emerg, dev, format, ##arg)
#define cpu_dev_alert(dev, format, arg...)	\
	cpu_dev_printk(alert, dev, format, ##arg)
#define cpu_dev_crit(dev, format, arg...)	\
	cpu_dev_printk(crit, dev, format, ##arg)
#define cpu_dev_err(dev, format, arg...)	\
	cpu_dev_printk(err, dev, format, ##arg)
#define cpu_dev_warn(dev, format, arg...)	\
	cpu_dev_printk(warning, dev, format, ##arg)
#define cpu_dev_notice(dev, format, arg...)	\
	cpu_dev_printk(notice, dev, format, ##arg)
#define cpu_dev_info(dev, format, arg...)	\
	cpu_dev_printk(info, dev, format, ##arg)
#define cpu_dev_dbg(dev, format, arg...)	\
	cpu_dev_printk(debug, dev, format, ##arg)

#define dpaa_eth_printk(level, dev, format, arg...) \
	cpu_dev_printk(level, dev, "%s:%hu:%s() " format, \
			__file__, __LINE__, __func__, ##arg)

#define dpaa_eth_emerg(dev, format, arg...)	\
	dpaa_eth_printk(emerg, dev, format, ##arg)
#define dpaa_eth_alert(dev, format, arg...)	\
	dpaa_eth_printk(alert, dev, format, ##arg)
#define dpaa_eth_crit(dev, format, arg...)	\
	dpaa_eth_printk(crit, dev, format, ##arg)
#define dpaa_eth_err(dev, format, arg...)	\
	dpaa_eth_printk(err, dev, format, ##arg)
#define dpaa_eth_warning(dev, format, arg...)	\
	dpaa_eth_printk(warning, dev, format, ##arg)
#define dpaa_eth_notice(dev, format, arg...)	\
	dpaa_eth_printk(notice, dev, format, ##arg)
#define dpaa_eth_info(dev, format, arg...)	\
	dpaa_eth_printk(info, dev, format, ##arg)
#define dpaa_eth_debug(dev, format, arg...)	\
	dpaa_eth_printk(debug, dev, format, ##arg)

#define cpu_netdev_emerg(net_dev, format, arg...)	\
	dpaa_eth_emerg((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_alert(net_dev, format, arg...)	\
	dpaa_eth_alert((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_crit(net_dev, format, arg...)	\
	dpaa_eth_crit((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_err(net_dev, format, arg...)		\
	dpaa_eth_err((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_warn(net_dev, format, arg...)	\
	dpaa_eth_warning((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_notice(net_dev, format, arg...)	\
	dpaa_eth_notice((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_info(net_dev, format, arg...)	\
	dpaa_eth_info((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)
#define cpu_netdev_dbg(net_dev, format, arg...)		\
	dpaa_eth_debug((net_dev)->dev.parent, "%s: " format, \
			(net_dev)->name , ##arg)

enum {RX, TX};

#define DPA_PRIV_DATA_SIZE 16
#define DPA_PARSE_RESULTS_SIZE sizeof(t_FmPrsResult)
#define DPA_HASH_RESULTS_SIZE 16

#define dpaa_eth_init_port(type, port, param, errq_id, defq_id, has_timer) \
{ \
	param.errq = errq_id; \
	param.defq = defq_id; \
	param.priv_data_size = DPA_PRIV_DATA_SIZE; \
	param.parse_results = true; \
	param.hash_results = true; \
	param.time_stamp = has_timer; \
	fm_set_##type##_port_params(port, &param); \
}

#endif	/* __DPA_COMMON_H */
