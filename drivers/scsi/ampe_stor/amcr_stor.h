/* Driver for Realtek PCI-Express card reader
 * Header file
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http:
 *
 * Author:
 *   wwang (wei_wang@realsil.com.cn)
 *   No. 450, Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#ifndef __AMCR_STOR_H
#define __AMCR_STOR_H

#include <asm/io.h>
#include <asm/bitops.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/mutex.h>
#include <linux/cdrom.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/time.h>

#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_devinfo.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_host.h>





#define CR_DRIVER_NAME		"ampe_stor"
#define DRIVER_VERSION 		"v1.18.03"


/*****************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 14)
#ifdef CONFIG_PCI
#undef pci_intx
#define pci_intx(pci,x)
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#define sg_page(sg)	(sg)->page
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define scsi_set_resid(srb, residue)	((srb)->resid = (residue))
#define scsi_get_resid(srb)		((srb)->resid)

static inline unsigned scsi_bufflen(struct scsi_cmnd *cmd)
{
	return cmd->request_bufflen;
}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 32)
#define pci_get_bus_and_slot(bus, devfn)	\
	pci_get_domain_bus_and_slot(0, (bus), (devfn))
#endif

/*****************************************************************************/
/*
 * macros for easy use
 */

#define wait_timeout_x(task_state, msecs)	  set_current_state((task_state)); \
						schedule_timeout((msecs) * HZ / 1000)
#define wait_timeout(msecs)		wait_timeout_x(TASK_INTERRUPTIBLE, (msecs))


#define STATE_TRANS_NONE	0
#define STATE_TRANS_CMD		1
#define STATE_TRANS_BUF		2
#define STATE_TRANS_SG		3

#define TRANS_NOT_READY		0
#define TRANS_RESULT_OK		1
#define TRANS_RESULT_FAIL	2

#define SCSI_LUN(srb)		(srb)->device->lun


/* The scsi_lock() and scsi_unlock() macros protect the sm_state and the
 * single queue element srb for write access */
#define scsi_unlock(host)	spin_unlock_irq(host->host_lock)
#define scsi_lock(host)		spin_lock_irq(host->host_lock)


/*****************************************************************************/
struct sense_data_t {
	u8	sense[18];
};

/*****************************************************************************/
typedef struct _MEDIA_CARD
{
	struct _DEVICE_EXTENSION	*pdx;

	u8  card_type;

#define SD_CARD_TYPE		0x01
#define MS_CARD_TYPE		0x02
#define XD_CARD_TYPE		0x04

#define SD_CARD_TYPE_SD		0x01
#define SD_CARD_TYPE_SDHC	0x11
#define SD_CARD_TYPE_SDXC	0x21
#define SD_CARD_TYPE_MMC	0x81

#define MS_CARD_TYPE_MS		0x02
#define MS_CARD_TYPE_MS_PRO	0x12


	u32	media_state;		/* (our) card state */

#define MEDIA_STATE_PRESENT		(1<<0)		/* present in sysfs */
#define MEDIA_STATE_READONLY	(1<<1)		/* card is read-only */
#define MEDIA_STATE_CHANGE		(1<<2)		/* card is changed */

	u32	media_capacity;		/* card capacity */

	u32	over_current;

#define	OC_IS_OVER_CURRENT		(1<<0)
#define OC_POWER_IS_ON			(1<<1)


}MEDIA_CARD, *PMEDIA_CARD;




/*****************************************************************************/
typedef struct _DEVICE_EXTENSION {

	struct pci_dev 		*pci;

	void __iomem *		ioaddr;		/* Mapped address */
	int					irq;

	u32		PciFunNum;
	u32		devfn;

	struct _MEDIA_CARD	media_card;

	u32	dflags;

	struct completion	cmnd_ready;	/* to sleep thread on	    */
	struct completion	card_ready;
	struct completion	notify;		/* thread begin/end	    */




	struct mutex		dev_mutex;

	wait_queue_head_t	delay_wait;


	struct sd_host		*sd;
	struct ms_host		*ms;


	struct sense_data_t 	sense_buffer;

	struct scsi_cmnd *srb;


	u8 card_init_flag;
	u8 current_card_active;

	u8 current_clk_src;

	u8 current_inf_ctrl;

	int pci_cap_off;
	u8  pci_aspm_cap;

	struct pci_dev *parent_pci;
	int parent_cap_off;
	u8  parent_aspm_cap;

	u8 dev_is_6621;

	/************************************************/
	int msi_en;

	u8 uExtEnOverCurrent;

	u8 uExtPadDrive0;
	u8 uExtPadDrive1;
	u8 uExtPadDrive2;

	u8 uExtSdPadDrive0;
	u8 uExtSdPadDrive1;
	u8 uExtSdPadDrive2;

	u8 uExtEnInterruptDelay;
	u8 uExtSignalCtrl;

	u8 uExtX2Mode;

	u8 uExtConfigDevAspm;
	u8 uExtDynamicAspm;


}DEVICE_EXTENSION, *PDEVICE_EXTENSION;








static inline struct Scsi_Host * pdx_to_host(struct _DEVICE_EXTENSION *pdx)
{
	return container_of((void *) pdx, struct Scsi_Host, hostdata);
}

static inline struct _DEVICE_EXTENSION * host_to_pdx(struct Scsi_Host *host)
{
	return (struct _DEVICE_EXTENSION *) host->hostdata;
}

static inline void get_current_time(u8 *timeval_buf, int buf_len)
{
	struct timeval tv;

	if (!timeval_buf || (buf_len < 8)) {
		return;
	}

	do_gettimeofday(&tv);

	timeval_buf[0] = (u8)(tv.tv_sec >> 24);
	timeval_buf[1] = (u8)(tv.tv_sec >> 16);
	timeval_buf[2] = (u8)(tv.tv_sec >> 8);
	timeval_buf[3] = (u8)(tv.tv_sec);
	timeval_buf[4] = (u8)(tv.tv_usec >> 24);
	timeval_buf[5] = (u8)(tv.tv_usec >> 16);
	timeval_buf[6] = (u8)(tv.tv_usec >> 8);
	timeval_buf[7] = (u8)(tv.tv_usec);
}


#include "win2linux.h"
#include "general.h"
#include "trace.h"

#include "amcr_scsi.h"
#include "ms.h"
#include "sd.h"





int pci_find_cap_offset(struct pci_dev *pci);
int pci_init_check_aspm(struct _DEVICE_EXTENSION *pdx);
void pci_aspm_ctrl(struct _DEVICE_EXTENSION *pdx, u8 aspm_enable);


/*****************************************************************************/
#endif

