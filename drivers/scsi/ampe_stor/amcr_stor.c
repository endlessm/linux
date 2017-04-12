
#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#include "amcr_stor.h"



MODULE_DESCRIPTION("Alcor Micro PCI-Express card reader driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);



unsigned int delay_use = 1;
module_param(delay_use, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(delay_use, "seconds to delay before using a new device");


/* These are used to make sure the module doesn't unload before all the
 * threads have exited.
 */
static atomic_t total_threads = ATOMIC_INIT(0);
static DECLARE_COMPLETION(threads_gone);


static irqreturn_t amcr_irq(int irq, void *dev_id);



/*****************************************************************************/
static void amcr_init_options(struct _DEVICE_EXTENSION *pdx)
{
	pdx->uExtConfigDevAspm = 0;
	pdx->uExtDynamicAspm = 0;

	pdx->uExtPadDrive0 = 0x44;
	pdx->uExtPadDrive1 = 0x44;
	pdx->uExtPadDrive2 = 0x00;

	pdx->uExtSdPadDrive0 = 0x31;
	pdx->uExtSdPadDrive1 = 0x33;
	pdx->uExtSdPadDrive2 = 0x00;

	pdx->uExtX2Mode = 1;

	pdx->msi_en = 0;

	return;
}


/***********************************************************************
 * Host functions
 ***********************************************************************/

static const char* host_info(struct Scsi_Host *host)
{
	return "SCSI emulation for PCI-Express Mass Storage devices";
}

static int slave_alloc(struct scsi_device *sdev)
{
	/*
	 * Set the INQUIRY transfer length to 36.  We don't use any of
	 * the extra data and many devices choke if asked for more or
	 * less than 36 bytes.
	 */
	sdev->inquiry_len = 36;

	/* USB has unusual DMA-alignment requirements: Although the
	 * starting address of each scatter-gather element doesn't matter,
	 * the length of each element except the last must be divisible
	 * by the Bulk maxpacket value.  There's currently no way to
	 * express this by block-layer constraints, so we'll cop out
	 * and simply require addresses to be aligned at 512-byte
	 * boundaries.  This is okay since most block I/O involves
	 * hardware sectors that are multiples of 512 bytes in length,
	 * and since host controllers up through USB 2.0 have maxpacket
	 * values no larger than 512.
	 *
	 * But it doesn't suffice for Wireless USB, where Bulk maxpacket
	 * values can be as large as 2048.  To make that work properly
	 * will require changes to the block layer.
	 */
	//blk_queue_update_dma_alignment(sdev->request_queue, (512 - 1));

	return 0;
}

static int slave_configure(struct scsi_device *sdev)
{

	TRACE2(("slave_configure ===>, sdev->type: %x", sdev->type));

	/* Scatter-gather buffers (all but the last) must have a length
	 * divisible by the bulk maxpacket size.  Otherwise a data packet
	 * would end up being short, causing a premature end to the data
	 * transfer.  Since high-speed bulk pipes have a maxpacket size
	 * of 512, we'll use that as the scsi device queue's DMA alignment
	 * mask.  Guaranteeing proper alignment of the first buffer will
	 * have the desired effect because, except at the beginning and
	 * the end, scatter-gather buffers follow page boundaries. */
	blk_queue_dma_alignment(sdev->request_queue, (512 - 1));


	/* Set the SCSI level to at least 2.  We'll leave it at 3 if that's
	 * what is originally reported.  We need this to avoid confusing
	 * the SCSI layer with devices that report 0 or 1, but need 10-byte
	 * commands (ala ATAPI devices behind certain bridges, or devices
	 * which simply have broken INQUIRY data).
	 *
	 * NOTE: This means /dev/sg programs (ala cdrecord) will get the
	 * actual information.  This seems to be the preference for
	 * programs like that.
	 *
	 * NOTE: This also means that /proc/scsi/scsi and sysfs may report
	 * the actual value or the modified one, depending on where the
	 * data comes from.
	 */
	if (sdev->scsi_level < SCSI_2)
		sdev->scsi_level = sdev->sdev_target->scsi_level = SCSI_2;

	return 0;
}


/***********************************************************************
 * /proc/scsi/ functions
 ***********************************************************************/

static int queuecommand_lck(struct scsi_cmnd *srb, void (*done)(struct scsi_cmnd *))
{
	struct _DEVICE_EXTENSION *pdx = host_to_pdx(srb->device->host);


	if (pdx->srb != NULL) {
		TRACEX(("pdx->srb: %p, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", pdx->srb));
		return SCSI_MLQUEUE_HOST_BUSY;
	}

	/* fail the command if we are disconnecting */
	if (US_FLIDX_DISCONNECTING & pdx->dflags) {
		TRACEX(("pdx->dflags & US_FLIDX_DISCONNECTING, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		srb->result = DID_NO_CONNECT << 16;
		done(srb);
		return 0;
	}

	srb->scsi_done = done;
	pdx->srb = srb;
	complete(&pdx->cmnd_ready);
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 37)
static int queuecommand(struct scsi_cmnd *srb,
			void (*done)(struct scsi_cmnd *))
{
	return queuecommand_lck(srb, done);
}
#else
static DEF_SCSI_QCMD(queuecommand)
#endif

/***********************************************************************
 * Error handling functions
 ***********************************************************************/


static int command_abort(struct scsi_cmnd *srb)
{
	struct Scsi_Host *host = srb->device->host;
	struct _DEVICE_EXTENSION *pdx = host_to_pdx(host);

	TRACEZ(("command_abort ===>, srb: %p", srb));

	scsi_lock(host);

	if (pdx->srb != srb) {
		scsi_unlock(host);
		TRACEX(("pdx->srb: %p, srb: %p, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", pdx->srb, srb));
		return FAILED;
	}

	pdx->dflags |= US_FLIDX_ABORTING;

	scsi_unlock(host);

	/* Wait for the aborted command to finish */
	wait_for_completion(&pdx->notify);
	return SUCCESS;
}

/* This invokes the transport reset mechanism to reset the state of the
 * device
 */
static int device_reset(struct scsi_cmnd *srb)
{
	int result = 0;

	TRACEZ(("device_reset ===>, srb: %p", srb));

	return result < 0 ? FAILED : SUCCESS;
}


static int bus_reset(struct scsi_cmnd *srb)
{
	int result = 0;

	TRACEZ(("bus_reset ===>, srb: %p", srb));

	return result < 0 ? FAILED : SUCCESS;
}


/*
 * this defines our host template, with which we'll allocate hosts
 */

struct scsi_host_template amcr_host_template = {

	.name =				CR_DRIVER_NAME,
	.proc_name =			CR_DRIVER_NAME,
	.info =				host_info,


	.queuecommand =			queuecommand,


	.eh_abort_handler =		command_abort,
	.eh_device_reset_handler =	device_reset,
	.eh_bus_reset_handler =		bus_reset,


	.can_queue =			1,
	.cmd_per_lun =			1,


	.this_id =			-1,

	.slave_alloc =			slave_alloc,
	.slave_configure =		slave_configure,


	.sg_tablesize =			SG_ALL,


	.max_sectors =                  240,

	/* merge commands... this seems to help performance, but
	 * periodically someone should test to see which setting is more
	 * optimal.
	 */
	.use_clustering =		1,


	.emulated =			1,


	.skip_settle_delay =		1,


	.module =			THIS_MODULE
};

static int amcr_acquire_irq(struct _DEVICE_EXTENSION *pdx)
{

	TRACE2(("amcr_acquire_irq ===>"));

	if (request_irq(pdx->pci->irq,
					amcr_irq,
					pdx->msi_en ? 0 : IRQF_SHARED,
					CR_DRIVER_NAME,
					pdx)) {
		TRACEX(("request_irq, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return -1;
	}

	pdx->irq = pdx->pci->irq;
	pci_intx(pdx->pci, !pdx->msi_en);

	return 0;
}


void amcr_do_before_power_down(struct _DEVICE_EXTENSION *pdx)
{
	struct sd_host *sd;
	struct ms_host *ms;
	struct _MEDIA_CARD	*media_card = &pdx->media_card;

	sd = pdx->sd;
	ms = pdx->ms;

	sd_stop_last_cmd(sd);

	pci_aspm_ctrl(pdx, 0);

	// disable card detect interrupt
	writeb(0x00, pdx->ioaddr + CARD_DETECT_STATUS);

	// sd card
	sd->card_inserted = FALSE;
	writel(0x00, sd->ioaddr + SD_INT_ENABLE);
	sd_power_off(sd);

	// ms card
	ms->card_inserted = FALSE;
	writel(0x00, ms->ioaddr + MS_INT_ENABLE);
	ms_power_off(ms);


	media_card->media_state    = 0;
	media_card->media_capacity = 0;
	media_card->over_current   = 0;

}

#ifdef CONFIG_PM
/*
 * power management
 */
static int amcr_suspend(struct pci_dev *pci, pm_message_t state)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)pci_get_drvdata(pci);

	TRACE2(("==================================================="));
	TRACE2(("amcr_suspend ===>"));

	if (!pdx) {
		TRACEX(("pdx == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return 0;
	}


	mutex_lock(&(pdx->dev_mutex));

	amcr_do_before_power_down(pdx);

	if (pdx->irq >= 0) {
		synchronize_irq(pdx->irq);
		free_irq(pdx->irq, (void *)pdx);
		pdx->irq = -1;
	}

	if (pdx->msi_en) {
		pci_disable_msi(pci);
	}

	pci_save_state(pci);
	pci_enable_wake(pci, pci_choose_state(pci, state), 0);
	pci_disable_device(pci);
	pci_set_power_state(pci, pci_choose_state(pci, state));

	mutex_unlock(&pdx->dev_mutex);

	return 0;
}

static int amcr_resume(struct pci_dev *pci)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)pci_get_drvdata(pci);
	struct sd_host *sd;
	struct ms_host *ms;
	u8 card_detect;

	TRACE2(("*******************************************************"));
	TRACE2(("amcr_resume ===>"));

	if (!pdx) {
		TRACEX(("pdx == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return 0;
	}


	mutex_lock(&(pdx->dev_mutex));

	pci_set_power_state(pci, PCI_D0);
	pci_restore_state(pci);
	if (pci_enable_device(pci) < 0) {
		TRACEX(("pci_enable_device < 0, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		mutex_unlock(&pdx->dev_mutex);
		return -EIO;
	}
	pci_set_master(pci);

	if (pdx->msi_en) {
		if (pci_enable_msi(pci) < 0) {
			pdx->msi_en = 0;
		}
	}

	if (amcr_acquire_irq(pdx) < 0) {
		TRACEX(("amcr_acquire_irq, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		mutex_unlock(&pdx->dev_mutex);
		return -EIO;
	}

	sd = pdx->sd;
	ms = pdx->ms;

	pci_init_check_aspm(pdx);
	pci_aspm_ctrl(pdx, 0);

	card_init_interface_mode_ctrl(pdx);

	writeb((u8)pdx->uExtPadDrive0, pdx->ioaddr + CARD_PAD_DRIVE0);
	writeb((u8)pdx->uExtPadDrive1, pdx->ioaddr + CARD_PAD_DRIVE1);
	writeb((u8)pdx->uExtPadDrive2, pdx->ioaddr + CARD_PAD_DRIVE2);

	/* enable card detect interrupt */
	writeb(0x00, pdx->ioaddr + CARD_DETECT_STATUS);
	writeb(CARD_DETECT_ENABLE, pdx->ioaddr + CARD_DETECT_STATUS);
	card_detect = readb(pdx->ioaddr + CARD_DETECT_STATUS);

	TRACEW(("card_detect: %x", card_detect));

	if (card_detect & SD_CARD_BIT) {
		TRACEW(("sd card insert ......"));
		sd->card_inserted = TRUE;
	}

	if (card_detect & MS_CARD_BIT) {
		TRACEW(("ms card insert ......"));
		ms->card_inserted = TRUE;

	}

	sd_reset(sd);
	ms_reset(ms);

	if (sd->card_inserted) {
		sd->uhs_mode_err = 0;
		sd->sd_err_init_clk = 0;
		sd_init_card(sd);
	}

	if (ms->card_inserted) {
		ms_init_card(ms);
	}

	pci_aspm_ctrl(pdx, 1);


	mutex_unlock(&pdx->dev_mutex);

	return 0;
}
#endif

void amcr_shutdown(struct pci_dev *pci)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)pci_get_drvdata(pci);

	TRACE2(("amcr_shutdown ===>"));

	if (!pdx) {
		TRACEX(("pdx == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	amcr_do_before_power_down(pdx);

	if (pdx->irq >= 0) {
		synchronize_irq(pdx->irq);
		free_irq(pdx->irq, (void *)pdx);
		pdx->irq = -1;
	}

	if (pdx->msi_en) {
		pci_disable_msi(pci);
	}

	pci_disable_device(pci);

	return;
}


static int amcr_scan_thread(void * __dev)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)__dev;

	if (delay_use > 0) {
		TRACEZ(("amcr_scan_thread: delay_use: %x", delay_use));
		wait_event_interruptible_timeout(pdx->delay_wait,
				(pdx->dflags & US_FLIDX_DISCONNECTING),
				delay_use * HZ);
	}


	if (!(pdx->dflags & US_FLIDX_DISCONNECTING)) {
		scsi_scan_host(pdx_to_host(pdx));
		TRACEZ(("device scan complete ..."));
	}

	scsi_host_put(pdx_to_host(pdx));
	complete_and_exit(&threads_gone, 0);
}



static int amcr_card_init_thread(void *dev_id)
{
	struct _DEVICE_EXTENSION *pdx = dev_id;
	struct sd_host *sd = pdx->sd;
	struct ms_host *ms = pdx->ms;

	current->flags |= PF_NOFREEZE;

	for(;;) {

		if (wait_for_completion_interruptible(&pdx->card_ready)) {
			break;
		}

		TRACE1(("amcr_card_init_thread ===>"));

		mutex_lock(&(pdx->dev_mutex));

		if (US_FLIDX_DISCONNECTING & pdx->dflags) {
			TRACEX(("pdx->dflags & US_FLIDX_DISCONNECTING, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			mutex_unlock(&pdx->dev_mutex);
			break;
		}

		if (pdx->uExtDynamicAspm) {
			pci_aspm_ctrl(pdx, 0);
		}

		if (pdx->card_init_flag & SD_CARD_TYPE) {
		    pdx->card_init_flag &= ~SD_CARD_TYPE;
			if (sd) {
				sd->uhs_mode_err = 0;
				sd->sd_err_init_clk = 0;
				sd_init_card(sd);
			}
		}

		if (pdx->card_init_flag & MS_CARD_TYPE) {
		    pdx->card_init_flag &= ~MS_CARD_TYPE;
		    if (ms) {
				ms_init_card(ms);
			}
		}

		if (pdx->uExtDynamicAspm) {
			pci_aspm_ctrl(pdx, 1);
		}

		mutex_unlock(&pdx->dev_mutex);
	}

	scsi_host_put(pdx_to_host(pdx));
	complete_and_exit(&threads_gone, 0);
}

static int amcr_control_thread(void *dev_id)
{
	struct _DEVICE_EXTENSION *pdx = dev_id;
	struct Scsi_Host *host = pdx_to_host(pdx);


	current->flags |= PF_NOFREEZE;

	for(;;) {

		if (wait_for_completion_interruptible(&pdx->cmnd_ready)) {
			break;
		}

		/* lock the device pointers */
		mutex_lock(&(pdx->dev_mutex));


		if (US_FLIDX_DISCONNECTING & pdx->dflags) {
			TRACEX(("pdx->dflags & US_FLIDX_DISCONNECTING, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			mutex_unlock(&pdx->dev_mutex);
			break;
		}

		/* lock access to the state */
		scsi_lock(host);

		if (US_FLIDX_ABORTING & pdx->dflags) {
			TRACEX(("pdx->dflags & US_FLIDX_ABORTING, XXXXXXXXXXXXXXXXXXXXXXXXXX"));
			pdx->srb->result = DID_ABORT << 16;
			goto SkipForAbort;
		}

		scsi_unlock(host);

		/* reject the command if the direction indicator
		 * is UNKNOWN
		 */
		if (pdx->srb->sc_data_direction == DMA_BIDIRECTIONAL) {
			TRACEX(("UNKNOWN data direction, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			pdx->srb->result = DID_ERROR << 16;
		}

		/* reject if target != 0 or if LUN is higher than
		 * the maximum known LUN
		 */
		else if (pdx->srb->device->id) {
			TRACEX(("Bad target number (%x:%x)", pdx->srb->device->id, pdx->srb->device->lun));
			pdx->srb->result = DID_BAD_TARGET << 16;
		}

		else if (pdx->srb->device->lun > 0) {
			TRACEX(("Bad LUN (%x:%x)", pdx->srb->device->id, pdx->srb->device->lun));
			pdx->srb->result = DID_BAD_TARGET << 16;
		}


		else {
			DEBUG((scsi_show_command(pdx->srb)));

			if (pdx->uExtDynamicAspm) {
				pci_aspm_ctrl(pdx, 0);
			}

			amcr_invoke_transport(pdx->srb, pdx);

			if (pdx->uExtDynamicAspm) {
				pci_aspm_ctrl(pdx, 1);
			}
		}

		scsi_lock(host);


		if (pdx->srb == NULL) {
			;
		}

		/* indicate that the command is done */
		else if (pdx->srb->result != DID_ABORT << 16) {
			pdx->srb->scsi_done(pdx->srb);
		}

		else {
SkipForAbort:
			TRACEX(("scsi command aborted, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		}

		/* If an abort request was received we need to signal that
		 * the abort has finished.  The proper test for this is
		 * the TIMED_OUT flag, not srb->result == DID_ABORT, because
		 * the timeout might have occurred after the command had
		 * already completed with a different result code. */
		if (US_FLIDX_ABORTING & pdx->dflags) {
			pdx->dflags &= ~US_FLIDX_ABORTING;
			complete(&(pdx->notify));
		}

		pdx->srb = NULL;
		scsi_unlock(host);


		mutex_unlock(&pdx->dev_mutex);
	}

	scsi_host_put(host);

	/* notify the exit routine that we're actually exiting now
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	complete_and_exit(&threads_gone, 0);

}


static irqreturn_t amcr_irq(int irq, void *dev_id)
{
	struct _DEVICE_EXTENSION *pdx = dev_id;
	irqreturn_t	handled = IRQ_NONE;
	u8     handle_status;


	if (pdx == NULL) {
		return IRQ_NONE;
	}


	handle_status = 0;
	switch (pdx->media_card.card_type & 0x0f) {
	case SD_CARD_TYPE:
		handle_status = SD_CARD_TYPE;
		handled = sdhci_irq(pdx);
		break;
	case MS_CARD_TYPE:
		handle_status = MS_CARD_TYPE;
		handled = ms_irq(pdx);
		break;
	}

	if (handled == IRQ_HANDLED) {
		goto irq_end;
	}

	if (!(handle_status & SD_CARD_TYPE)) {
		handled = sdhci_irq(pdx);
		if (handled == IRQ_HANDLED) {
			goto irq_end;
		}
	}

	if (!(handle_status & MS_CARD_TYPE)) {
		handled = ms_irq(pdx);
		if (handled == IRQ_HANDLED) {
			goto irq_end;
		}
	}

irq_end:
	return handled;

}


/* First stage of disconnect processing: stop all commands and remove
 * the host */
static void quiesce_and_remove_host(struct _DEVICE_EXTENSION *pdx)
{
	struct Scsi_Host *host = pdx_to_host(pdx);

	/* Prevent new transfers, stop the current command, and
	 * interrupt a SCSI-scan or device-reset delay */
	mutex_lock(&pdx->dev_mutex);
	scsi_lock(host);
	pdx->dflags |= US_FLIDX_DISCONNECTING;
	scsi_unlock(host);
	mutex_unlock(&pdx->dev_mutex);
	wake_up(&pdx->delay_wait);


	wait_timeout(100);

	/* queuecommand won't accept any new commands and the control
	 * thread won't execute a previously-queued command.  If there
	 * is such a command pending, complete it with an error. */
	mutex_lock(&pdx->dev_mutex);
	if (pdx->srb) {
		pdx->srb->result = DID_NO_CONNECT << 16;
		scsi_lock(host);
		pdx->srb->scsi_done(pdx->srb);
		pdx->srb = NULL;
		scsi_unlock(host);
	}
	mutex_unlock(&pdx->dev_mutex);

	scsi_remove_host(host);
}



static void release_resources(struct _DEVICE_EXTENSION *pdx)
{

	TRACEZ(("release_resources ===>"));

	complete(&pdx->cmnd_ready);
	complete(&pdx->card_ready);

	pci_disable_device(pdx->pci);
	pci_release_regions(pdx->pci);

	if (pdx->irq > 0) {
		free_irq(pdx->irq, (void *)pdx);
		pdx->irq = -1;
	}

	if (pdx->msi_en) {
		pci_disable_msi(pdx->pci);
	}

}


static void release_everything(struct _DEVICE_EXTENSION *pdx)
{

	TRACEZ(("release_everything ===>"));

	release_resources(pdx);


	sd_remove_device(pdx);

	ms_remove_device(pdx);



	/* Drop our reference to the host; the SCSI core will free it
	 * when the refcount becomes 0. */
	scsi_host_put(pdx_to_host(pdx));
}


int pci_find_cap_offset(struct pci_dev *pci)
{
	int where;
	u8 val8;
	u32 val32;

#define CAP_START_OFFSET	0x34

	where = CAP_START_OFFSET;
	pci_read_config_byte(pci, where, &val8);
	if (!val8) {
		return 0;
	}

	where = (int)val8;
	while (1) {
		pci_read_config_dword(pci, where, &val32);
		if (val32 == 0xffffffff) {
			TRACEX(("invaild, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			return 0;
		}

		if ((val32 & 0xff) == 0x10) {
			TRACE2(("pcie cap offset: %x", where));
			return where;
		}

		if ((val32 & 0xff00) == 0x00) {
			TRACEX(("invaild, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			break;
		}
		where = (int)((val32 >> 8) & 0xff);
	}

	return 0;
}

int pci_init_check_aspm(struct _DEVICE_EXTENSION *pdx)
{
#define PCIE_LINK_CAP_OFFSET	0x0c

	struct pci_dev *pci;
	int where;
	u32 val32;

	TRACE2(("pci_init_check_aspm ===>"));

	pdx->pci_cap_off    = pci_find_cap_offset(pdx->pci);
	pdx->parent_cap_off = pci_find_cap_offset(pdx->parent_pci);

	if ((pdx->pci_cap_off == 0) || (pdx->parent_cap_off == 0)) {
		TRACEX(("pci_cap_off: %x, parent_cap_off: %x, XXXXXXXXXXXXXXXXXX",
		pdx->pci_cap_off, pdx->parent_cap_off));
		return 0;
	}

	/* link capability */
	pci   = pdx->pci;
	where = pdx->pci_cap_off + PCIE_LINK_CAP_OFFSET;
	pci_read_config_dword(pci, where, &val32);
	pdx->pci_aspm_cap = (u8)(val32 >> 10) & 0x03;

	pci   = pdx->parent_pci;
	where = pdx->parent_cap_off + PCIE_LINK_CAP_OFFSET;
	pci_read_config_dword(pci, where, &val32);
	pdx->parent_aspm_cap = (u8)(val32 >> 10) & 0x03;

	if (pdx->pci_aspm_cap != pdx->parent_aspm_cap) {
		u8 aspm_cap;
		TRACE2(("pdx->pci_aspm_cap   : %x", pdx->pci_aspm_cap));
		TRACE2(("pdx->parent_aspm_cap: %x", pdx->parent_aspm_cap));
		aspm_cap = pdx->pci_aspm_cap & pdx->parent_aspm_cap;
		pdx->pci_aspm_cap    = aspm_cap;
		pdx->parent_aspm_cap = aspm_cap;
	}

	TRACE2(("uExtConfigDevAspm: %x, pdx->pci_aspm_cap: %x", pdx->uExtConfigDevAspm, pdx->pci_aspm_cap));
	pdx->uExtConfigDevAspm &= pdx->pci_aspm_cap;
	return 1;
}

void pci_aspm_ctrl(struct _DEVICE_EXTENSION *pdx, u8 aspm_enable)
{
#define PCIE_LINK_CTRL_OFFSET	0x10

	struct pci_dev *pci;
	u8 aspm_ctrl, i;
	int where;
	u32 val32;

	TRACE1(("pci_aspm_ctrl ===>, aspm_enable: %x", aspm_enable));

	if ((pdx->pci_cap_off == 0) || (pdx->parent_cap_off == 0)) {
		TRACEX(("pci_cap_off: %x, parent_cap_off: %x, XXXXXXXXXXXXXXXXXX",
		pdx->pci_cap_off, pdx->parent_cap_off));
		return;
	}

	if (pdx->pci_aspm_cap == 0) {
		return;
	}

	aspm_ctrl = 0;
	if (aspm_enable) {
		aspm_ctrl = (u8)pdx->uExtConfigDevAspm;

		if (aspm_ctrl == 0) {
			TRACE1(("aspm_ctrl == 0, ........................"));
			return;
		}
	}

	for (i=0; i < 2; i++) {

		if (i==0) {
			pci   = pdx->pci;
			where = pdx->pci_cap_off + PCIE_LINK_CTRL_OFFSET;
		}
		else {
			pci   = pdx->parent_pci;
			where = pdx->parent_cap_off + PCIE_LINK_CTRL_OFFSET;
		}

		pci_read_config_dword(pci, where, &val32);
		val32 &= (~0x03);
		val32 |= (aspm_ctrl & pdx->pci_aspm_cap);
		pci_write_config_byte(pci, where, (u8)val32);
	}

}


static int amcr_probe(struct pci_dev *pci, const struct pci_device_id *pci_id)
{
	struct Scsi_Host *host;
	struct _DEVICE_EXTENSION *pdx;
	int err = 0;
	struct task_struct *th;
	unsigned long 		addr;
	u32	config, i;
	u8 card_detect;
	struct pci_dev *pci_bus;
	u8 reg_value;

	TRACEX(("================================================================"));
	TRACEZ(("amcr_probe ===>"));
	TRACEZ(("pci->devfn: %x", pci->devfn));

	TRACEZ(("pci->vendor: %x", pci->vendor));
	TRACEZ(("pci->device: %x", pci->device));
	TRACEZ(("pci->sub->vendor: %x", pci->subsystem_vendor));
	TRACEZ(("pci->sub->device: %x", pci->subsystem_device));


	pci_bus = pci->bus->self;
	TRACEZ(("pci_bus->vendor: %x", pci_bus->vendor));
	TRACEZ(("pci_bus->device: %x", pci_bus->device));

	for (i=0; i < 64; i+=4) {
		pci_read_config_dword(pci, i, &config);
		TRACEZ(("pci config %02x: %08x", i, config));
	}

	err = pci_enable_device(pci);
	if (err < 0) {
		TRACEX(("pci_enable_device, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}


	err = pci_request_regions(pci, CR_DRIVER_NAME);
	if (err < 0) {
		TRACEX(("pci_request_regions, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		pci_disable_device(pci);
		return err;
	}

	pci_read_config_dword(pci, 0, &config);
	TRACEZ(("pci config 00: %x", config));

	pci_read_config_dword(pci, 4, &config);
	TRACEZ(("pci config 04: %x", config));

	/*
	 * Ask the SCSI layer to allocate a host structure, with extra
	 * space at the end for our private pdx structure.
	 */
	host = scsi_host_alloc(&amcr_host_template, sizeof(struct _DEVICE_EXTENSION));
	if (!host) {
		TRACEX(("scsi_host_alloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		pci_release_regions(pci);
		pci_disable_device(pci);
		return -ENOMEM;
	}

	pdx = host_to_pdx(host);

	memset(pdx, 0, sizeof(struct _DEVICE_EXTENSION));

	amcr_init_options(pdx);

	pdx->pci = pci;
	pdx->parent_pci = pci->bus->self;
	pdx->devfn = pdx->PciFunNum = pci->devfn;

	pdx->irq = -1;

	init_completion(&pdx->cmnd_ready);
	init_completion(&pdx->card_ready);
	init_completion(&pdx->notify);

	mutex_init(&pdx->dev_mutex);
	init_waitqueue_head(&pdx->delay_wait);


	addr = pci_resource_start(pci, 0);

	TRACEZ(("addr: %lx", addr));

	/*pdx->ioaddr = ioremap_nocache(addr, pci_resource_len(pci,0));*/
	pdx->ioaddr = ioremap(addr, pci_resource_len(pci,0));
	if (pdx->ioaddr == NULL) {
		TRACEX(("ioremap_nocache, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = -ENXIO;
		goto errout;
	}

	TRACEZ(("pdx->ioaddr: %lx", (unsigned long)pdx->ioaddr));
	TRACEZ(("pci->irq: %x", pci->irq));

	if (pdx->msi_en) {
		if (pci_enable_msi(pci) < 0) {
			pdx->msi_en = 0;
		}
	}

	if (amcr_acquire_irq(pdx) < 0) {
		TRACEX(("amcr_acquire_irq, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = -EBUSY;
		goto errout;
	}

	pci_set_master(pci);
	synchronize_irq(pdx->irq);

	err = sd_add_device(pdx);
	if (err) {
		TRACEX(("sd_add_device, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = -ENXIO;
		goto errout;
	}

	err = ms_add_device(pdx);
	if (err) {
		TRACEX(("ms_add_device, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = -ENXIO;
		goto errout;
	}



	err = scsi_add_host(host, &pci->dev);
	if (err) {
		TRACEX(("scsi_add_host, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		goto errout;
	}

	th = kthread_create(amcr_control_thread, pdx, CR_DRIVER_NAME);
	if (IS_ERR(th)) {
		TRACEX(("kthread_create: amcr, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = PTR_ERR(th);
		goto errout;
	}

	/* Take a reference to the host for the control thread and
	 * count it among all the threads we have launched.  Then
	 * start it up. */
	scsi_host_get(pdx_to_host(pdx));
	atomic_inc(&total_threads);
	wake_up_process(th);


	th = kthread_create(amcr_scan_thread, pdx, "amcr-scan");
	if (IS_ERR(th)) {
		TRACEX(("kthread_create: amcr-scan, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		quiesce_and_remove_host(pdx);
		err = PTR_ERR(th);
		goto errout;
	}

	/* Take a reference to the host for the scanning thread and
	 * count it among all the threads we have launched.  Then
	 * start it up. */
	scsi_host_get(pdx_to_host(pdx));
	atomic_inc(&total_threads);
	wake_up_process(th);


	th = kthread_create(amcr_card_init_thread, pdx, "amcr-init");
	if (IS_ERR(th)) {
		TRACEX(("kthread_create: amcr-init, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		quiesce_and_remove_host(pdx);
		err = PTR_ERR(th);
		goto errout;
	}

	/* Take a reference to the host for the scanning thread and
	 * count it among all the threads we have launched.  Then
	 * start it up. */
	scsi_host_get(pdx_to_host(pdx));
	atomic_inc(&total_threads);
	wake_up_process(th);


	pci_set_drvdata(pci, pdx);

	pci_init_check_aspm(pdx);
	pci_aspm_ctrl(pdx, 0);

	card_init_interface_mode_ctrl(pdx);

	writeb((u8)pdx->uExtPadDrive0, pdx->ioaddr + CARD_PAD_DRIVE0);
	writeb((u8)pdx->uExtPadDrive1, pdx->ioaddr + CARD_PAD_DRIVE1);
	writeb((u8)pdx->uExtPadDrive2, pdx->ioaddr + CARD_PAD_DRIVE2);

	writeb(0x01, pdx->ioaddr + CHIP_FUNCTION);
	reg_value = readb(pdx->ioaddr + CHIP_FUNCTION);
	TRACEW(("CHIP_FUNCTION 0x7f: %x, ************************************", reg_value));
	if (reg_value == 0x21) {
		pdx->dev_is_6621 = 1;
	}

	if (pdx->dev_is_6621) {
		writeb(0x01, pdx->ioaddr + CARD_DMA_PAGE_CNT);
	}
	else {
		writeb(0x00, pdx->ioaddr + CARD_DMA_BOUNDARY);
	}	


	/* enable card detect interrupt */
	writeb(0x00, pdx->ioaddr + CARD_DETECT_STATUS);
	writeb(CARD_DETECT_ENABLE, pdx->ioaddr + CARD_DETECT_STATUS);
	card_detect = readb(pdx->ioaddr + CARD_DETECT_STATUS);

	TRACEW(("card_detect: %x", card_detect));

	if (card_detect & SD_CARD_BIT) {
		pdx->sd->card_inserted = TRUE;
		pdx->sd->uhs_mode_err = 0;
		pdx->sd->sd_err_init_clk = 0;
		sd_init_card(pdx->sd);
	}

	if (card_detect & MS_CARD_BIT) {
		pdx->ms->card_inserted = TRUE;
		ms_init_card(pdx->ms);
	}

	pci_aspm_ctrl(pdx, 1);


	TRACEZ(("amcr_probe <==="));
	return 0;


errout:
	TRACEX(("amcr_probe, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
	release_everything(pdx);

	return err;
}


static void amcr_remove(struct pci_dev *pci)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)pci_get_drvdata(pci);

	TRACEZ(("amcr_remove ===>"));

	sd_stop_last_cmd(pdx->sd);

	pci_aspm_ctrl(pdx, 0);

	quiesce_and_remove_host(pdx);

	release_everything(pdx);


	pci_set_drvdata(pci, NULL);

	TRACEZ(("amcr_remove <==="));
}


static struct pci_device_id amcr_ids[] = {
	{ .vendor = 0x1aea, .device = 0x6601,	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, },
	{ .vendor = 0x1aea, .device = 0x6621,	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, },
	{ .vendor = 0x1aea, .device = 0x6625,	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, },
	{ .vendor = 0x105b, .device = 0x0ef6,	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, },
	{ .vendor = 0x105b, .device = 0x0ef7,	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, },
	{ 0, },
};

MODULE_DEVICE_TABLE(pci, amcr_ids);


static struct pci_driver driver = {
	.name = CR_DRIVER_NAME,
	.id_table = amcr_ids,
	.probe = amcr_probe,
	.remove = amcr_remove,

#ifdef CONFIG_PM
	.suspend = amcr_suspend,
	.resume = amcr_resume,
#endif

	.shutdown = amcr_shutdown,

};

static int __init amcr_init(void)
{
	TRACEZ(("**************************************************************"));
	TRACEZ(("amcr_init ==>, %s", DRIVER_VERSION));


	return pci_register_driver(&driver);
}

static void __exit amcr_exit(void)
{

	TRACEZ(("amcr_exit ===>"));


	pci_unregister_driver(&driver);

	/* Don't return until all of our control and scanning threads
	 * have exited.  Since each thread signals threads_gone as its
	 * last act, we have to call wait_for_completion the right number
	 * of times.
	 */
	while (atomic_read(&total_threads) > 0) {
		wait_for_completion(&threads_gone);
		atomic_dec(&total_threads);
	}

	TRACEZ(("amcr_exit <==="));
	TRACEZ(("**************************************************************"));
}

module_init(amcr_init)
module_exit(amcr_exit)

