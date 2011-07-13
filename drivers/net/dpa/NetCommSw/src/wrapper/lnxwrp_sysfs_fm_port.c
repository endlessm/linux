/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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
/*

 @File          lnxwrp_sysfs_fm_port.c

 @Description   FM port sysfs related functions.

*/

#include "lnxwrp_sysfs.h"
#include "lnxwrp_fm.h"

static const struct SysfsStats_t portSysfsStats[] = {
	/* RX/TX/OH common statistics */
	{
	 .statisticName = "port_frame",
	 .statisticCounter = e_FM_PORT_COUNTERS_FRAME,
	 },
	{
	 .statisticName = "port_discard_frame",
	 .statisticCounter = e_FM_PORT_COUNTERS_DISCARD_FRAME,
	 },
	{
	 .statisticName = "port_dealloc_buf",
	 .statisticCounter = e_FM_PORT_COUNTERS_DEALLOC_BUF,
	 },
	{
	 .statisticName = "port_enq_total",
	 .statisticCounter = e_FM_PORT_COUNTERS_ENQ_TOTAL,
	 },
	/* TX/OH */
	{
	 .statisticName = "port_length_err",
	 .statisticCounter = e_FM_PORT_COUNTERS_LENGTH_ERR,
	 },
	{
	 .statisticName = "port_unsupprted_format",
	 .statisticCounter = e_FM_PORT_COUNTERS_UNSUPPRTED_FORMAT,
	 },
	{
	 .statisticName = "port_deq_total",
	 .statisticCounter = e_FM_PORT_COUNTERS_DEQ_TOTAL,
	 },
	{
	 .statisticName = "port_deq_from_default",
	 .statisticCounter = e_FM_PORT_COUNTERS_DEQ_FROM_DEFAULT,
	 },
	{
	 .statisticName = "port_deq_confirm",
	 .statisticCounter = e_FM_PORT_COUNTERS_DEQ_CONFIRM,
	 },
	/* RX/OH */
	{
	 .statisticName = "port_rx_bad_frame",
	 .statisticCounter = e_FM_PORT_COUNTERS_RX_BAD_FRAME,
	 },
	{
	 .statisticName = "port_rx_large_frame",
	 .statisticCounter = e_FM_PORT_COUNTERS_RX_LARGE_FRAME,
	 },
	{
	 .statisticName = "port_rx_out_of_buffers_discard",
	 .statisticCounter = e_FM_PORT_COUNTERS_RX_OUT_OF_BUFFERS_DISCARD,
	 },
	{
	 .statisticName = "port_rx_filter_frame",
	 .statisticCounter = e_FM_PORT_COUNTERS_RX_FILTER_FRAME,
	 },
	/* TODO: Particular statistics for OH ports */
	{}
};

static ssize_t show_fm_port_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev;
	t_LnxWrpFmDev *p_LnxWrpFmDev;
	unsigned long flags;
	int n = 0;
	uint8_t counter = 0;

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

	p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmPortDev == NULL))
		return -EINVAL;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
		return -EIO;

	if (!p_LnxWrpFmPortDev->h_Dev) {
		n = snprintf(buf, PAGE_SIZE, "\tFM Port not configured...\n");
		return n;
	}

	counter =
	    fm_find_statistic_counter_by_name(attr->attr.name,
					      (struct SysfsStats_t *) &
					      portSysfsStats[0], NULL);

	if (counter == e_FM_PORT_COUNTERS_RX_LIST_DMA_ERR) {
		uint32_t fmRev = 0;
		fmRev = 0xffff & ioread32(UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr
							+ 0x000c30c4));

		if (fmRev == 0x0100) {
			local_irq_save(flags);
			n = snprintf(buf, PAGE_SIZE,
				     "counter not available for revision 1\n");
			local_irq_restore(flags);
		}
		return n;
	}

	local_irq_save(flags);
	n = snprintf(buf, PAGE_SIZE, "\tFM %d Port %d counter: %d\n",
		     p_LnxWrpFmDev->id,
		     p_LnxWrpFmPortDev->id,
		     FM_PORT_GetCounter(p_LnxWrpFmPortDev->h_Dev,
					(e_FmPortCounters) counter));
	local_irq_restore(flags);

	return n;
}

/* FM PORT RX/TX/OH statistics */
static DEVICE_ATTR(port_frame, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_discard_frame, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_dealloc_buf, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_enq_total, S_IRUGO, show_fm_port_stats, NULL);
/* FM PORT TX/OH statistics */
static DEVICE_ATTR(port_length_err, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_unsupprted_format, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_deq_total, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_deq_from_default, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_deq_confirm, S_IRUGO, show_fm_port_stats, NULL);
/* FM PORT RX/OH statistics */
static DEVICE_ATTR(port_rx_bad_frame, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_rx_large_frame, S_IRUGO, show_fm_port_stats, NULL);
static DEVICE_ATTR(port_rx_out_of_buffers_discard, S_IRUGO,
		   show_fm_port_stats, NULL);
static DEVICE_ATTR(port_rx_filter_frame, S_IRUGO, show_fm_port_stats, NULL);

/* FM PORT TX statistics */
static struct attribute *fm_tx_port_dev_stats_attributes[] = {
	&dev_attr_port_frame.attr,
	&dev_attr_port_discard_frame.attr,
	&dev_attr_port_dealloc_buf.attr,
	&dev_attr_port_enq_total.attr,
	&dev_attr_port_length_err.attr,
	&dev_attr_port_unsupprted_format.attr,
	&dev_attr_port_deq_total.attr,
	&dev_attr_port_deq_from_default.attr,
	&dev_attr_port_deq_confirm.attr,
	NULL
};

static const struct attribute_group fm_tx_port_dev_stats_attr_grp = {
	.name = "statistics",
	.attrs = fm_tx_port_dev_stats_attributes
};

/* FM PORT RX statistics */
static struct attribute *fm_rx_port_dev_stats_attributes[] = {
	&dev_attr_port_frame.attr,
	&dev_attr_port_discard_frame.attr,
	&dev_attr_port_dealloc_buf.attr,
	&dev_attr_port_enq_total.attr,
	&dev_attr_port_rx_bad_frame.attr,
	&dev_attr_port_rx_large_frame.attr,
	&dev_attr_port_rx_out_of_buffers_discard.attr,
	&dev_attr_port_rx_filter_frame.attr,
	NULL
};

static const struct attribute_group fm_rx_port_dev_stats_attr_grp = {
	.name = "statistics",
	.attrs = fm_rx_port_dev_stats_attributes
};

/* TODO: add particular OH ports statistics */
static struct attribute *fm_oh_port_dev_stats_attributes[] = {
	&dev_attr_port_frame.attr,
	&dev_attr_port_discard_frame.attr,
	&dev_attr_port_dealloc_buf.attr,
	&dev_attr_port_enq_total.attr,
	 /*TX*/ &dev_attr_port_length_err.attr,
	&dev_attr_port_unsupprted_format.attr,
	&dev_attr_port_deq_total.attr,
	&dev_attr_port_deq_from_default.attr,
	&dev_attr_port_deq_confirm.attr,
	 /*RX*/ &dev_attr_port_rx_bad_frame.attr,
	&dev_attr_port_rx_large_frame.attr,
	&dev_attr_port_rx_out_of_buffers_discard.attr,
	/*&dev_attr_port_rx_filter_frame.attr, */
	NULL
};

static const struct attribute_group fm_oh_port_dev_stats_attr_grp = {
	.name = "statistics",
	.attrs = fm_oh_port_dev_stats_attributes
};

static ssize_t show_fm_port_regs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	unsigned n = 0;
#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev =
	    (t_LnxWrpFmPortDev *) dev_get_drvdata(dev);
#endif

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
	local_irq_save(flags);

	if (!p_LnxWrpFmPortDev->h_Dev) {
		n = snprintf(buf, PAGE_SIZE, "\tFM Port not configured...\n");
		return n;
	} else {
		n = snprintf(buf, PAGE_SIZE,
			     "FM port driver registers dump.\n");
		FM_PORT_DumpRegs(p_LnxWrpFmPortDev->h_Dev);
	}

	local_irq_restore(flags);

	return n;
#else

	local_irq_save(flags);
	n = snprintf(buf, PAGE_SIZE,
		     "Debug level is too low to dump registers!!!\n");
	local_irq_restore(flags);

	return n;
#endif
}

static DEVICE_ATTR(fm_port_regs, 0x644, show_fm_port_regs, NULL);

int fm_port_sysfs_create(struct device *dev)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev;

	if (dev == NULL)
		return -EINVAL;

	p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmPortDev == NULL))
		return -EINVAL;

	/* store to remove them when module is disabled */
	p_LnxWrpFmPortDev->dev_attr_regs = &dev_attr_fm_port_regs;

	/* Registers dump entry - in future will be moved to debugfs */
	if (device_create_file(dev, &dev_attr_fm_port_regs) != 0)
		return -EIO;

	/* FM Ports statistics */
	switch (p_LnxWrpFmPortDev->settings.param.portType) {
	case e_FM_PORT_TYPE_TX:
	case e_FM_PORT_TYPE_TX_10G:
		if (sysfs_create_group
		    (&dev->kobj, &fm_tx_port_dev_stats_attr_grp) != 0)
			return -EIO;
		break;
	case e_FM_PORT_TYPE_RX:
	case e_FM_PORT_TYPE_RX_10G:
		if (sysfs_create_group
		    (&dev->kobj, &fm_rx_port_dev_stats_attr_grp) != 0)
			return -EIO;
		break;
	case e_FM_PORT_TYPE_OH_OFFLINE_PARSING:
	case e_FM_PORT_TYPE_OH_HOST_COMMAND:
		if (sysfs_create_group
		    (&dev->kobj, &fm_oh_port_dev_stats_attr_grp) != 0)
			return -EIO;
		break;
	case e_FM_PORT_TYPE_DUMMY:
	default:
		WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__,
		     __func__);
		return -EINVAL;
		break;
	};

	return 0;
}

void fm_port_sysfs_destroy(struct device *dev)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = NULL;

	/* this function has never been tested !!! */

	if (WARN_ON(dev == NULL))
		return;

	p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmPortDev == NULL))
		return;

	/* The name attribute will be freed also by these 2 functions? */
	switch (p_LnxWrpFmPortDev->settings.param.portType) {
	case e_FM_PORT_TYPE_TX:
	case e_FM_PORT_TYPE_TX_10G:
		sysfs_remove_group(&dev->kobj, &fm_tx_port_dev_stats_attr_grp);
		break;
	case e_FM_PORT_TYPE_RX:
	case e_FM_PORT_TYPE_RX_10G:
		sysfs_remove_group(&dev->kobj, &fm_rx_port_dev_stats_attr_grp);
		break;
	case e_FM_PORT_TYPE_OH_OFFLINE_PARSING:
	case e_FM_PORT_TYPE_OH_HOST_COMMAND:
		sysfs_remove_group(&dev->kobj, &fm_oh_port_dev_stats_attr_grp);
		break;
	case e_FM_PORT_TYPE_DUMMY:
	default:
		WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__,
		     __func__);
		break;
	};

	device_remove_file(dev, p_LnxWrpFmPortDev->dev_attr_regs);
}
