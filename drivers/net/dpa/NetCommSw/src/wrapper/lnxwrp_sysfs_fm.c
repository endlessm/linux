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

 @File          lnxwrp_sysfs_fm.c

 @Description   FM sysfs related functions.

*/

#include "lnxwrp_sysfs.h"
#include "lnxwrp_fm.h"

enum e_FmDmaMatchStatistics {
	e_FM_DMA_COUNTERS_CMQ_NOT_EMPTY,
	e_FM_DMA_COUNTERS_BUS_ERROR,
	e_FM_DMA_COUNTERS_READ_BUF_ECC_ERROR,
	e_FM_DMA_COUNTERS_WRITE_BUF_ECC_SYS_ERROR,
	e_FM_DMA_COUNTERS_WRITE_BUF_ECC_FM_ERROR
};

static const struct SysfsStats_t fmSysfsStats[] = {
	/* FM statistics */
	{
	 .statisticName = "enq_total_frame",
	 .statisticCounter = e_FM_COUNTERS_ENQ_TOTAL_FRAME,
	 },
	{
	 .statisticName = "deq_total_frame",
	 .statisticCounter = e_FM_COUNTERS_DEQ_TOTAL_FRAME,
	 },
	{
	 .statisticName = "deq_0",
	 .statisticCounter = e_FM_COUNTERS_DEQ_0,
	 },
	{
	 .statisticName = "deq_1",
	 .statisticCounter = e_FM_COUNTERS_DEQ_1,
	 },
	{
	 .statisticName = "deq_2",
	 .statisticCounter = e_FM_COUNTERS_DEQ_2,
	 },
	{
	 .statisticName = "deq_from_default",
	 .statisticCounter = e_FM_COUNTERS_DEQ_FROM_DEFAULT,
	 },
	{
	 .statisticName = "deq_from_context",
	 .statisticCounter = e_FM_COUNTERS_DEQ_FROM_CONTEXT,
	 },
	{
	 .statisticName = "deq_from_fd",
	 .statisticCounter = e_FM_COUNTERS_DEQ_FROM_FD,
	 },
	{
	 .statisticName = "deq_confirm",
	 .statisticCounter = e_FM_COUNTERS_DEQ_CONFIRM,
	 },
	/* FM:DMA  statistics */
	{
	 .statisticName = "cmq_not_empty",
	 .statisticCounter = e_FM_DMA_COUNTERS_CMQ_NOT_EMPTY,
	 },
	{
	 .statisticName = "bus_error",
	 .statisticCounter = e_FM_DMA_COUNTERS_BUS_ERROR,
	 },
	{
	 .statisticName = "read_buf_ecc_error",
	 .statisticCounter = e_FM_DMA_COUNTERS_READ_BUF_ECC_ERROR,
	 },
	{
	 .statisticName = "write_buf_ecc_sys_error",
	 .statisticCounter = e_FM_DMA_COUNTERS_WRITE_BUF_ECC_SYS_ERROR,
	 },
	{
	 .statisticName = "write_buf_ecc_fm_error",
	 .statisticCounter = e_FM_DMA_COUNTERS_WRITE_BUF_ECC_FM_ERROR,
	 },
	/* FM:PCD  statistics */
	{
	 .statisticName = "pcd_enq_total_frame",
	 .statisticCounter = e_FM_COUNTERS_ENQ_TOTAL_FRAME,
	 },
	{
	 .statisticName = "pcd_kg_total",
	 .statisticCounter = e_FM_PCD_KG_COUNTERS_TOTAL,
	 },
	{
	 .statisticName = "pcd_plcr_yellow",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_YELLOW,
	 },
	{
	 .statisticName = "pcd_plcr_red",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_RED,
	 },
	{
	 .statisticName = "pcd_plcr_recolored_to_red",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_RED,
	 },
	{
	 .statisticName = "pcd_plcr_recolored_to_yellow",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_RECOLORED_TO_YELLOW,
	 },
	{
	 .statisticName = "pcd_plcr_total",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_TOTAL,
	 },
	{
	 .statisticName = "pcd_plcr_length_mismatch",
	 .statisticCounter = e_FM_PCD_PLCR_COUNTERS_LENGTH_MISMATCH,
	 },
	{
	 .statisticName = "pcd_prs_parse_dispatch",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_PARSE_DISPATCH,
	 },
	{
	 .statisticName = "pcd_prs_l2_parse_result_returned",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED,
	 },
	{
	 .statisticName = "pcd_prs_l3_parse_result_returned",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED,
	 },
	{
	 .statisticName = "pcd_prs_l4_parse_result_returned",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED,
	 },
	{
	 .statisticName = "pcd_prs_shim_parse_result_returned",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED,
	 },
	{
	 .statisticName = "pcd_prs_l2_parse_result_returned_with_err",
	 .statisticCounter =
	 e_FM_PCD_PRS_COUNTERS_L2_PARSE_RESULT_RETURNED_WITH_ERR,
	 },
	{
	 .statisticName = "pcd_prs_l3_parse_result_returned_with_err",
	 .statisticCounter =
	 e_FM_PCD_PRS_COUNTERS_L3_PARSE_RESULT_RETURNED_WITH_ERR,
	 },
	{
	 .statisticName = "pcd_prs_l4_parse_result_returned_with_err",
	 .statisticCounter =
	 e_FM_PCD_PRS_COUNTERS_L4_PARSE_RESULT_RETURNED_WITH_ERR,
	 },
	{
	 .statisticName = "pcd_prs_shim_parse_result_returned_with_err",
	 .statisticCounter =
	 e_FM_PCD_PRS_COUNTERS_SHIM_PARSE_RESULT_RETURNED_WITH_ERR,
	 },
	{
	 .statisticName = "pcd_prs_soft_prs_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_SOFT_PRS_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_soft_prs_stall_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_SOFT_PRS_STALL_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_hard_prs_cycle_incl_stall_cycles",
	 .statisticCounter =
	 e_FM_PCD_PRS_COUNTERS_HARD_PRS_CYCLE_INCL_STALL_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_muram_read_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_MURAM_READ_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_muram_read_stall_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_MURAM_READ_STALL_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_muram_write_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_muram_write_stall_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_MURAM_WRITE_STALL_CYCLES,
	 },
	{
	 .statisticName = "pcd_prs_fpm_command_stall_cycles",
	 .statisticCounter = e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES,
	 },
	{}
};

/* Fm stats and regs dumps via sysfs */
static ssize_t show_fm_dma_stats(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;
	t_FmDmaStatus fmDmaStatus;
	unsigned long flags = 0;
	unsigned n = 0;
	uint8_t counter_value = 0, counter = 0;

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
		return -EIO;

	counter =
		fm_find_statistic_counter_by_name(attr->attr.name,
						  (struct SysfsStats_t *)
						  &fmSysfsStats[0], NULL);

	local_irq_save(flags);

	memset(&fmDmaStatus, 0, sizeof(fmDmaStatus));
	FM_GetDmaStatus(p_LnxWrpFmDev->h_Dev, &fmDmaStatus);

	switch (counter) {
	case e_FM_DMA_COUNTERS_CMQ_NOT_EMPTY:
		counter_value = fmDmaStatus.cmqNotEmpty;
		break;
	case e_FM_DMA_COUNTERS_BUS_ERROR:
		counter_value = fmDmaStatus.busError;
		break;
	case e_FM_DMA_COUNTERS_READ_BUF_ECC_ERROR:
		counter_value = fmDmaStatus.readBufEccError;
		break;
	case e_FM_DMA_COUNTERS_WRITE_BUF_ECC_SYS_ERROR:
		counter_value = fmDmaStatus.writeBufEccSysError;
		break;
	case e_FM_DMA_COUNTERS_WRITE_BUF_ECC_FM_ERROR:
		counter_value = fmDmaStatus.writeBufEccFmError;
		break;
	default:
		WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__,
		     __func__);
		break;
	};

	n = snprintf(buf, PAGE_SIZE, "\tFM %u counter: %c\n",
		     p_LnxWrpFmDev->id, counter_value ? 'T' : 'F');

	local_irq_restore(flags);

	return n;
}

static ssize_t show_fm_stats(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;
	unsigned long flags = 0;
	unsigned n = 0, counter = 0;

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
		return -EIO;

	counter =
		fm_find_statistic_counter_by_name(attr->attr.name,
						  (struct SysfsStats_t *)
						  &fmSysfsStats[0], NULL);

	local_irq_save(flags);

	n = snprintf(buf, PAGE_SIZE, "\tFM %d counter: %d\n",
		     p_LnxWrpFmDev->id,
		     FM_GetCounter(p_LnxWrpFmDev->h_Dev,
				   (e_FmCounters) counter));

	local_irq_restore(flags);

	return n;
}

static ssize_t show_fm_pcd_stats(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;
	unsigned long flags = 0;
	unsigned n = 0, counter = 0;

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
		return -EIO;

	counter =
		fm_find_statistic_counter_by_name(attr->attr.name,
						  (struct SysfsStats_t *)
						  &fmSysfsStats[0], NULL);

	local_irq_save(flags);

	n = snprintf(buf, PAGE_SIZE, "\tFM %d counter: %d\n",
		     p_LnxWrpFmDev->id,
		     FM_PCD_GetCounter(p_LnxWrpFmDev->h_PcdDev,
				       (e_FmPcdCounters) counter));

	local_irq_restore(flags);

	return n;
}

/* FM */
static DEVICE_ATTR(enq_total_frame, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_total_frame, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_0, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_1, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_2, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_from_default, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_from_context, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_from_fd, S_IRUGO, show_fm_stats, NULL);
static DEVICE_ATTR(deq_confirm, S_IRUGO, show_fm_stats, NULL);
/* FM:DMA */
static DEVICE_ATTR(cmq_not_empty, S_IRUGO, show_fm_dma_stats, NULL);
static DEVICE_ATTR(bus_error, S_IRUGO, show_fm_dma_stats, NULL);
static DEVICE_ATTR(read_buf_ecc_error, S_IRUGO, show_fm_dma_stats, NULL);
static DEVICE_ATTR(write_buf_ecc_sys_error, S_IRUGO, show_fm_dma_stats, NULL);
static DEVICE_ATTR(write_buf_ecc_fm_error, S_IRUGO, show_fm_dma_stats, NULL);
/* FM:PCD */
static DEVICE_ATTR(pcd_enq_total_frame, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_kg_total, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_plcr_yellow, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_plcr_red, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_plcr_recolored_to_red, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_plcr_recolored_to_yellow, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_plcr_total, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_plcr_length_mismatch, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_prs_parse_dispatch, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l2_parse_result_returned, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l3_parse_result_returned, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l4_parse_result_returned, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_shim_parse_result_returned, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l2_parse_result_returned_with_err, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l3_parse_result_returned_with_err, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_l4_parse_result_returned_with_err, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_shim_parse_result_returned_with_err, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_soft_prs_cycles, S_IRUGO, show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_soft_prs_stall_cycles, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_prs_hard_prs_cycle_incl_stall_cycles, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_muram_read_cycles, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_prs_muram_read_stall_cycles, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_muram_write_cycles, S_IRUGO, show_fm_pcd_stats,
		   NULL);
static DEVICE_ATTR(pcd_prs_muram_write_stall_cycles, S_IRUGO,
		   show_fm_pcd_stats, NULL);
static DEVICE_ATTR(pcd_prs_fpm_command_stall_cycles, S_IRUGO,
		   show_fm_pcd_stats, NULL);

static struct attribute *fm_dev_stats_attributes[] = {
	&dev_attr_enq_total_frame.attr,
	&dev_attr_deq_total_frame.attr,
	&dev_attr_deq_0.attr,
	&dev_attr_deq_1.attr,
	&dev_attr_deq_2.attr,
	&dev_attr_deq_from_default.attr,
	&dev_attr_deq_from_context.attr,
	&dev_attr_deq_from_fd.attr,
	&dev_attr_deq_confirm.attr,
	&dev_attr_cmq_not_empty.attr,
	&dev_attr_bus_error.attr,
	&dev_attr_read_buf_ecc_error.attr,
	&dev_attr_write_buf_ecc_sys_error.attr,
	&dev_attr_write_buf_ecc_fm_error.attr,
	&dev_attr_pcd_enq_total_frame.attr,
	&dev_attr_pcd_kg_total.attr,
	&dev_attr_pcd_plcr_yellow.attr,
	&dev_attr_pcd_plcr_red.attr,
	&dev_attr_pcd_plcr_recolored_to_red.attr,
	&dev_attr_pcd_plcr_recolored_to_yellow.attr,
	&dev_attr_pcd_plcr_total.attr,
	&dev_attr_pcd_plcr_length_mismatch.attr,
	&dev_attr_pcd_prs_parse_dispatch.attr,
	&dev_attr_pcd_prs_l2_parse_result_returned.attr,
	&dev_attr_pcd_prs_l3_parse_result_returned.attr,
	&dev_attr_pcd_prs_l4_parse_result_returned.attr,
	&dev_attr_pcd_prs_shim_parse_result_returned.attr,
	&dev_attr_pcd_prs_l2_parse_result_returned_with_err.attr,
	&dev_attr_pcd_prs_l3_parse_result_returned_with_err.attr,
	&dev_attr_pcd_prs_l4_parse_result_returned_with_err.attr,
	&dev_attr_pcd_prs_shim_parse_result_returned_with_err.attr,
	&dev_attr_pcd_prs_soft_prs_cycles.attr,
	&dev_attr_pcd_prs_soft_prs_stall_cycles.attr,
	&dev_attr_pcd_prs_hard_prs_cycle_incl_stall_cycles.attr,
	&dev_attr_pcd_prs_muram_read_cycles.attr,
	&dev_attr_pcd_prs_muram_read_stall_cycles.attr,
	&dev_attr_pcd_prs_muram_write_cycles.attr,
	&dev_attr_pcd_prs_muram_write_stall_cycles.attr,
	&dev_attr_pcd_prs_fpm_command_stall_cycles.attr,
	NULL
};

static const struct attribute_group fm_dev_stats_attr_grp = {
	.name = "statistics",
	.attrs = fm_dev_stats_attributes
};

static ssize_t show_fm_regs(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	unsigned long flags;
	unsigned n = 0;
#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;
#endif

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	local_irq_save(flags);

	n = snprintf(buf, PAGE_SIZE, "FM driver registers dump.\n");

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_Dev)
		return -EIO;
	else
		FM_DumpRegs(p_LnxWrpFmDev->h_Dev);

	local_irq_restore(flags);
#else

	local_irq_save(flags);
	n = snprintf(buf, PAGE_SIZE,
		     "Debug level is too low to dump registers!!!\n");
	local_irq_restore(flags);
#endif /* (defined(DEBUG_ERRORS) && ... */

	return n;
}

static ssize_t show_pcd_regs(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	unsigned n = 0;
#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;
#endif

	if (attr == NULL || buf == NULL || dev == NULL)
		return -EINVAL;

#if (defined(DEBUG_ERRORS) && (DEBUG_ERRORS > 0))
	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return -EINVAL;

	local_irq_save(flags);
	n = snprintf(buf, PAGE_SIZE, "FM driver registers dump.\n");

	if (!p_LnxWrpFmDev->active || !p_LnxWrpFmDev->h_PcdDev)
		return -EIO;
	else
		FM_PCD_DumpRegs(p_LnxWrpFmDev->h_PcdDev);

	local_irq_restore(flags);
#else

	local_irq_save(flags);
	n = snprintf(buf, PAGE_SIZE,
		     "Debug level is too low to dump registers!!!\n");
	local_irq_restore(flags);

#endif /* (defined(DEBUG_ERRORS) && ... */

	return n;
}

static DEVICE_ATTR(fm_regs, S_IRUGO, show_fm_regs, NULL);
static DEVICE_ATTR(fm_pcd_regs, S_IRUGO, show_pcd_regs, NULL);

int fm_sysfs_create(struct device *dev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;

	if (dev == NULL)
		return -EIO;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);

	/* store to remove them when module is disabled */
	p_LnxWrpFmDev->dev_attr_regs = &dev_attr_fm_regs;
	p_LnxWrpFmDev->dev_pcd_attr_regs = &dev_attr_fm_pcd_regs;

	/* Create sysfs statistics group for FM module */
	if (sysfs_create_group(&dev->kobj, &fm_dev_stats_attr_grp) != 0)
		return -EIO;

	/* Registers dump entry - in future will be moved to debugfs */
	if (device_create_file(dev, &dev_attr_fm_regs) != 0 ||
	    device_create_file(dev, &dev_attr_fm_pcd_regs) != 0)
		return -EIO;

	return 0;
}

void fm_sysfs_destroy(struct device *dev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = NULL;

	if (WARN_ON(dev == NULL))
		return;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) dev_get_drvdata(dev);
	if (WARN_ON(p_LnxWrpFmDev == NULL))
		return;

	sysfs_remove_group(&dev->kobj, &fm_dev_stats_attr_grp);
	device_remove_file(dev, p_LnxWrpFmDev->dev_attr_regs);
	device_remove_file(dev, p_LnxWrpFmDev->dev_pcd_attr_regs);
}
