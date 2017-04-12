#include "amcr_stor.h"



/*************************************************************************************************/
static const u32 tran_exp[] = {
	10000, 100000, 1000000, 10000000,
		0,		0,		 0,		   0
};

static const u8 tran_mant[] = {
	0, 10, 12, 13, 15, 20, 25, 30,
   35, 40, 45, 50, 55, 60, 70, 80
};

static const u32 tacc_exp[] = {
	1, 10, 100,	1000, 10000, 100000, 1000000, 10000000
};

static const u32 tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
		35,	40,	45,	50,	55,	60,	70,	80
};

u8 sdxc_tuning_pattern[]={
	0xFF,0x0F,0xFF,0x00, 0xFF,0xCC,0xC3,0xCC, 0xC3,0x3C,0xCC,0xFF, 0xFE,0xFF,0xFE,0xEF,
	0xFF,0xDF,0xFF,0xDD, 0xFF,0xFB,0xFF,0xFB, 0xBF,0xFF,0x7F,0xFF, 0x77,0xF7,0xBD,0xEF,
	0xFF,0xF0,0xFF,0xF0, 0x0F,0xFC,0xCC,0x3C, 0xCC,0x33,0xCC,0xCF, 0xFF,0xEF,0xFF,0xEE,
	0xFF,0xFD,0xFF,0xFD, 0xDF,0xFF,0xBF,0xFF, 0xBB,0xFF,0xF7,0xFF, 0xF7,0x7F,0x7B,0xDE
};

static const u32 g_uhs_clk_array[] = {
	208000, 194000, 130000, 100000, 80000, 60000
};

static const u32 g_sd_clk_array[] = {
	50000, 40000, 25000, 20000, 10000
};

/*************************************************************************************************/
int sd_add_device(struct _DEVICE_EXTENSION *pdx)
{
	struct sd_host *sd;
	int status;

	TRACE2(("================================================="));
	TRACE2(("sd_add_device ===>"));

	pdx->sd = sd = (struct sd_host*)kmalloc(sizeof(struct sd_host), GFP_KERNEL);
	if (!sd) {
		TRACEY(("kmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	memset(sd, 0x00, sizeof(struct sd_host));

	sd->pdx	= pdx;
	sd->ioaddr = pdx->ioaddr;

	KeInitializeDpc(&sd->card_tasklet, sdhci_tasklet_card, (unsigned long)pdx);
	KeInitializeDpc(&sd->finish_tasklet, sdhci_tasklet_finish, (unsigned long)pdx);

	setup_timer(&sd->timeout_timer, sdhci_tasklet_timeout, (unsigned long)pdx);


	/* Debug - Force to PIO mode */
	sd->flags |= SDHCI_USE_DMA;


	status = sd_get_external_setting(sd);
	if (status) {
		TRACEX(("sd_get_external_setting, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", status));
		return status;
	}


	sd_reset(sd);

	sd_dumpregs(sd);


	return ERR_SUCCESS;
}


/*************************************************************************************************/
int sd_remove_device(struct _DEVICE_EXTENSION *pdx)
{
	struct sd_host *sd = pdx->sd;

	TRACEW(("================================================="));
	TRACEW(("sd_remove_device ===>"));


	tasklet_kill(&sd->card_tasklet);
	tasklet_kill(&sd->finish_tasklet);

	del_timer_sync(&sd->timeout_timer);


	writel(0, sd->ioaddr + SD_INT_ENABLE);

	sd_power_off(sd);

	kfree(sd);
	pdx->sd = NULL;

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_get_external_setting(struct sd_host *sd)
{

	TRACE2(("sd_get_external_setting ===>"));



	sd->uSdExtNoUhsMode = 0;


	sd->uSdExt200MHzOff = 0;


	sd->uSdExtSdr25Clock = 50000;
	sd->uSdExtSdr50Clock = 100000;
	sd->uSdExtSdr100Clock = 208000;


	sd->uSdExt50SetHighSpeed = 1;
	sd->uSdExt100SetHighSpeed = 1;
	sd->uSdExt200SetHighSpeed = 1;

	sd->uSdExtMmcForce1Bit = 0;

	sd->uSdExtMmcForce4Bit = 0;

	sd->uSdExtMmcType26Clock = 26000;
	sd->uSdExtMmcType52Clock = 52000;

	sd->uSdExtCprmEnable = 0;

	sd->uSdExtSingleRdWrCmd = 1;

	sd->uSdExtHwTimeOutCnt = 125; /* default: 125 * 40ms = 5 sec */

	sd->uSdExtInitErrReduceClk = 1;

	sd->uSdExtIoErrReduceClk = 1;

	sd->uSdExtInitClkDelay = 0x20;

	sd->uSdExtInitCmdDelayUs = 40;

	sd->uSdExtDisableMmc = 0;

	sd->uSdExtTuneUpSpeed = 1;

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void sd_update_media_card(struct sd_host *sd)
{
	struct sd_card		*scard		= &sd->scard;
	struct sd_csd		*csd		= &sd->scard.csd;
	struct _MEDIA_CARD	*media_card = &sd->pdx->media_card;


	if ((!sd->card_inserted) || scard->card_is_error) {
		TRACEW(("sd card is removed, --------------------------------------"));

		sd_power_off(sd);

		media_card->media_state    = 0;
		media_card->media_capacity = 0;
		media_card->over_current   = 0;
		return;
	}

	TRACEW(("sd card is inserted, OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
	media_card->card_type = sd->card_type;

	//media_card->media_state	   = MEDIA_STATE_PRESENT;
	media_card->media_state	   = MEDIA_STATE_PRESENT | MEDIA_STATE_CHANGE;
	media_card->media_capacity = csd->capacity;

	if (sd->scard.state & MMC_STATE_READONLY) {
		TRACEW(("sd->scard.state & MMC_STATE_READONLY ................"));
		media_card->media_state |= MEDIA_STATE_READONLY;
	}

	if (csd->perm_write_protect || csd->temp_write_protect) {
		TRACEW(("csd->perm_write_protect || csd->temp_write_protect, ................"));
		media_card->media_state |= MEDIA_STATE_READONLY;
	}

	return;
}

/*************************************************************************************************/
void __________sd_scsi_thread__________(void)
{

}

/*************************************************************************************************/
void sd_reduce_clock(struct sd_host *sd)
{
	struct sd_card			*scard	= &sd->scard;
	u32 clk_array_size, i;
	int err;

	TRACE2(("sd_reduce_clock ===>"));

	if (sd->clock > 52000) {
		clk_array_size = sizeof(g_uhs_clk_array) / sizeof(g_uhs_clk_array[0]);
		for (i=0; i < clk_array_size; i++) {
			if (g_uhs_clk_array[i] <= sd->clock) {
				TRACEW(("sdxc uhs clock, i: %x, g_uhs_clk_array[i]: (%u), sd->clock: (%u)", i, g_uhs_clk_array[i], sd->clock));
				break;
			}
		}

		TRACEW(("sd->clock: (%u), i: %x", sd->clock, i));

		if ((i == clk_array_size) || (i == (clk_array_size-1))) {
			// Re-initialize to SDHC mode
			sd->uhs_mode_err = 1;
			sd->sd_err_init_clk = 0;
		}
		else {
			sd->uhs_mode_err = 0;
			sd->sd_err_init_clk = g_uhs_clk_array[i+1];
		}

		err = sd_init_card(sd);
		if (err) {
			scard->card_is_error = TRUE;
		}
	}
	else {
		// clock <= 52000 MHz
		clk_array_size = sizeof(g_sd_clk_array) / sizeof(g_sd_clk_array[0]);
		for (i=0; i < clk_array_size; i++) {
			if (g_sd_clk_array[i] <= sd->clock) {
				TRACEW(("sdhc clock, i: %x, g_sd_clk_array[i]: (%u), sd->clock: (%u)", i, g_sd_clk_array[i], sd->clock));
				break;
			}
		}

		TRACEW(("sd->clock: (%u), i: %x", sd->clock, i));

		if ((i == clk_array_size) || (i == (clk_array_size-1))) {
			scard->card_is_error = TRUE;
		}
		else {
			// sd_set_clock(sd, g_sd_clk_array[i+1]);
			// Re-initialize to SDHC mode
			sd->uhs_mode_err = 1;
			sd->sd_err_init_clk = g_sd_clk_array[i+1];
			err = sd_init_card(sd);
			if (err) {
				scard->card_is_error = TRUE;
			}
		}
	}

	if (scard->card_is_error) {
		sd_update_media_card(sd);
	}

	return;
}

/*************************************************************************************************/
int sd_check_card_is_ready(struct sd_host *sd)
{
	int err;
	u32 status;

	sd->sd_chking_card_ready  = 1;
	sd->sd_card_ready_timeout = 0;

	mod_timer(&sd->timeout_timer, jiffies + 5 * HZ); /* default is 5 sec */

	while (1) {

		if (sd->sd_card_ready_timeout)	{
			TRACEX(("sd->sd_card_ready_timeout == 1, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			err = ERR_ERROR;
			goto error_exit;
		}

		err = sd_send_status(sd, &status);
		if (err) {
			TRACEX(("sd_send_status: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto error_exit;
		}

		TRACE1(("status: %08x", status));
		if (!(status & R1_READY_FOR_DATA)) {
			continue;
		}

		/* SD state is in program state */
		if (R1_CURRENT_STATE(status) != 4) {
			continue;
		}
		break;
	}

error_exit:

	sd->sd_chking_card_ready  = 0;
	sd->sd_card_ready_timeout = 0;

	return err;
}

/*************************************************************************************************/
int sd_stop_last_cmd(struct sd_host	*sd)
{

	if (sd->last_cmd) {
		struct _DEVICE_EXTENSION *pdx;
		int err;

		pdx = sd->pdx;

		if (pdx->uExtDynamicAspm) {
			pci_aspm_ctrl(pdx, 0);
		}

		sd->last_cmd = 0;
		sd->last_lba = 0;
		err = sd_stop(sd);
		if (err) {
			TRACEX(("sd_stop, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		}

		if (pdx->uExtDynamicAspm) {
			pci_aspm_ctrl(pdx, 1);
		}
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_io_ctrl_x2_mode(struct sd_host *sd, u8 is_write)
{
	struct _DEVICE_EXTENSION *pdx;

	pdx = sd->pdx;

	// AU6621 case
	if (pdx->dev_is_6621) {
		return ERR_SUCCESS;
	}

	// Old AU6601 case, X2 mode has a bug for read operation
	if (is_write) {
		// hardware debug purpose
		// write operation enable X2 mode
		if ((pdx->uExtX2Mode) && (sd->clock >= 60000)) {
			if ((pdx->current_clk_src & CARD_CLK_X2_MODE) == 0) {
				pdx->current_clk_src |= CARD_CLK_X2_MODE;
				writeb(pdx->current_clk_src, pdx->ioaddr + CARD_CLK_SELECT);
			}
		}
	}
	else {
		// hardware debug purpose
		// read operation disable X2 mode
		if ((pdx->uExtX2Mode) && (sd->clock >= 60000)) {
			if ((pdx->current_clk_src & CARD_CLK_X2_MODE)) {
				pdx->current_clk_src &= ~CARD_CLK_X2_MODE;
				writeb(pdx->current_clk_src, pdx->ioaddr + CARD_CLK_SELECT);
			}
		}
	}
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_scsi_read_write_plus(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct sd_host			*sd		= pdx->sd;
	struct sd_command	cmd;
	struct sd_data		data;
	int		err;
	u32		send_cmd = TRUE;

	TRACE1(("============================="));
	TRACE1(("sd_scsi_read_write_plus: isWrite: %02x, lba: 0x%x, (%u), lba_len: (%u)", (srb->sc_data_direction == DMA_TO_DEVICE), lba, lba, lba_len));
	TRACE1(("last read/write,   cmd: %02x, lba: 0x%x", sd->last_cmd, sd->last_lba));


	memset(&cmd,  0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));


	/* initialize cmd parameters */
	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		data.flags |= MMC_DATA_WRITE;
	}
	else {
		cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		data.flags |= MMC_DATA_READ;
	}


	if (sd->uSdExtSingleRdWrCmd) {
		if (lba_len == 1) {
			if (srb->sc_data_direction == DMA_TO_DEVICE) {
				cmd.opcode = MMC_WRITE_BLOCK;
			}
			else {
				cmd.opcode = MMC_READ_SINGLE_BLOCK;
			}
		}
	}

	cmd.flags = MMC_RSP_R1;
	cmd.arg = lba;
	if (!(sd->scard.state & MMC_STATE_BLOCKADDR)) {
		cmd.arg <<= 9;
	}


	/* initialize data parameters */
	data.blksz	 = 0x200;
	data.blocks	 = lba_len;
	data.buf_len = lba_len * 0x200;
	data.buf_type= BUF_IS_MDL;
	data.sg      = scsi_sglist(srb);
	data.sg_cnt  = scsi_sg_count(srb);


	/* check if it is continuing read/write */
	if (sd->last_cmd) {
		if ((sd->last_cmd == cmd.opcode) &&
			(sd->last_lba == lba)) {
			send_cmd = FALSE;
			TRACE1(("continue read/write -----------------------------"));
		}
		else {

			TRACE1(("stop ======================"));
			sd->last_cmd = 0;
			sd->last_lba = 0;
			err = sd_stop(sd);
			if (err) {
				TRACEX(("sd_stop: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
				return ERR_ERROR;
			}
		}
	}


	if (send_cmd) {

		err = sd_check_card_is_ready(sd);
		if (err) {
			TRACEX(("sd_check_card_is_ready: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto ERROR_EXIT;
		}

		TRACE1(("send cmd: cmd.opcode: %02x, lba: 0x%x, (%u), lba_len: (%u)", cmd.opcode, lba, lba, lba_len));


		sd_io_ctrl_x2_mode(sd, data.flags & MMC_DATA_WRITE);

		writeb(0x00, sd->ioaddr + SD_DATA_XFER_CTRL);

		sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
		if (cmd.error) {
			TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
			goto ERROR_EXIT;
		}

	}

	mod_timer(&sd->timeout_timer, jiffies + 10 * HZ);

	sd_prepare_data(sd, &data);

	sd_wait_for_data(sd, &data);

	if (data.sg_map) {
		dma_unmap_page(&sd->pdx->pci->dev,
						data.dma_addr_map,
						data.sg_map->length,
						(data.flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		data.sg_map = 0;
	}

	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		sd->last_cmd = 0;
		sd->last_lba = 0;
		if (lba_len != 1) {
			err = sd_stop(sd);
		}
		goto ERROR_EXIT;
	}


	/* no stop command */
	sd->last_cmd = cmd.opcode;
	sd->last_lba = lba + lba_len;


	if (sd->uSdExtSingleRdWrCmd) {
		if (lba_len == 1) {
			sd->last_cmd = 0;
			sd->last_lba = 0;
		}
	}


	//writeb(0x00, sd->ioaddr + SD_DATA_XFER_CTRL);
	TRACE1(("sd->last_cmd: %x, sd->last_lba: %x, ********************************", sd->last_cmd, sd->last_lba));
	return ERR_SUCCESS;

ERROR_EXIT:
	TRACEX(("cmd.resp[0]: %x", cmd.resp[0]));
	TRACEX(("sd_scsi_read_write_plus <===, sd->clock: (%u), cmd.opcode: %x, lba: %x, cmd.arg: %x, lba_len: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sd->clock, cmd.opcode, lba, cmd.arg, lba_len));

	sd->last_cmd = 0;
	sd->last_lba = 0;

	return ERR_ERROR;
}

/*************************************************************************************************/
int sd_scsi_read_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct sd_host			*sd		= pdx->sd;
	struct sd_request	srq;
	struct sd_command	cmd;
	struct sd_data		data;
	struct sd_command	stop;
	int		err;


	// check card active
	if (pdx->current_card_active != SD_CARD_BIT) {
		sd_card_active_ctrl(sd, TRUE);
	}

	if (sd->uSdExtTuneUpSpeed) {
		err = sd_scsi_read_write_plus(srb, pdx, lba, lba_len);
		if (err) {
			goto ERROR_EXIT;
		}
		return err;
	}


	memset(&srq,  0, sizeof(struct sd_request));
	memset(&cmd,  0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));
	memset(&stop, 0, sizeof(struct sd_command));

	TRACE1(("============================="));
	TRACE1(("sd_scsi_read_write: %x, lba: 0x%02x, (%u), lba_len: (%u)", (srb->sc_data_direction == DMA_TO_DEVICE), lba, lba, lba_len));
	TRACE1(("sg: %p, sg_cnt: %x, scsi_bufflen: %x", scsi_sglist(srb), scsi_sg_count(srb), scsi_bufflen(srb)));

	srq.cmd		= &cmd;
	srq.data	= &data;
	srq.stop	= &stop;


	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		data.flags |= MMC_DATA_WRITE;
	}
	else {
		cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		data.flags |= MMC_DATA_READ;
	}

	sd_io_ctrl_x2_mode(sd, data.flags & MMC_DATA_WRITE);

	if (sd->uSdExtSingleRdWrCmd) {
		if (lba_len == 1) {
			if (srb->sc_data_direction == DMA_TO_DEVICE) {
				cmd.opcode = MMC_WRITE_BLOCK;
			}
			else {
				cmd.opcode = MMC_READ_SINGLE_BLOCK;
			}
			srq.stop = NULL;
		}
	}

	cmd.flags = MMC_RSP_R1;
	cmd.arg = lba;
	if (!(sd->scard.state & MMC_STATE_BLOCKADDR)) {
		cmd.arg <<= 9;
	}

	data.blksz		= 0x200;
	data.blocks		= lba_len;
	data.buf_len    = lba_len * 0x200;
	data.buf_type   = BUF_IS_MDL;
	data.sg     = scsi_sglist(srb);
	data.sg_cnt = scsi_sg_count(srb);


	stop.opcode = MMC_STOP_TRANSMISSION;
	stop.arg	= 0;
	stop.flags	= MMC_RSP_R1B;

	err = sd_check_card_is_ready(sd);
	if (err) {
		TRACEX(("sd_check_card_is_ready: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto ERROR_EXIT;
	}

	sd_wait_for_req(sd, &srq);

	if (data.sg_map) {
		dma_unmap_page(&sd->pdx->pci->dev,
						data.dma_addr_map,
						data.sg_map->length,
						(data.flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		data.sg_map = 0;
	}

	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		goto ERROR_EXIT;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		sd_stop(sd);
		goto ERROR_EXIT;
	}

	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		writeb(0x00, sd->ioaddr + SD_DATA_XFER_CTRL);
		err = sd_check_card_is_ready(sd);
		if (err) {
			TRACEX(("sd_check_card_is_ready: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto ERROR_EXIT;
		}
	}


	TRACE1(("sd_scsi_read_write <==="));
	return ERR_SUCCESS;

ERROR_EXIT:


	if (sd->uSdExtIoErrReduceClk) {
		TRACEW(("sd->uSdExtIoErrReduceClk == 1, -----------------------------------"));
		sd_reduce_clock(sd);
	}

	return ERR_ERROR;
}


/*****************************************************************************\
 *                                                                           *
 * Interrupt handling                                                        *
 *                                                                           *
\*****************************************************************************/
/*************************************************************************************************/
void __________sd_irq__________(void)
{

}

/*************************************************************************************************/
void sd_transfer_pio_read(struct sd_host *sd)
{
	struct	sd_data		*data	= sd->data;

	TRACE(("sdhci_pio_read_block ===>"));

	if (!data) {
		TRACEX(("data == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	if (data->buf_type == BUF_IS_MDL) {
		unsigned long flags;
		size_t blksize, len, chunk;
		u32 uninitialized_var(scratch);
		u8 *buf;

		blksize = sd->data->blksz;
		chunk = 0;

		local_irq_save(flags);

		while (blksize) {

			if (!sg_miter_next(&sd->sg_miter))
				BUG();

			len = min(sd->sg_miter.length, blksize);

			data->remain_cnt -= len;

			blksize -= len;
			sd->sg_miter.consumed = len;

			buf = sd->sg_miter.addr;

			TRACE2(("buf: %p", buf));

			while (len) {
				if (chunk == 0) {
					scratch = readl(sd->ioaddr + SD_BUFFER_PORT);
					chunk = 4;
				}

				*buf = scratch & 0xFF;

				buf++;
				scratch >>= 8;
				chunk--;
				len--;
			}
		}

		sg_miter_stop(&sd->sg_miter);

		local_irq_restore(flags);
	}
	else {
		int	blksize;
		u32	value, chunk_remain;
		u8	*buffer;
		int	size;

		buffer = (char*)data->buf;
		buffer += data->xfered_cnt;

		blksize = data->blksz;
		while (blksize) {

			value = readl(sd->ioaddr + SD_BUFFER_PORT);
			chunk_remain = min(blksize, 4);
			size = min(data->remain_cnt, chunk_remain);

			blksize			 -= size;
			data->remain_cnt -= size;
			data->xfered_cnt += size;

			while (size) {
				*buffer = (u8)(value & 0xff);
				buffer++;
				value >>= 8;
				size--;
			}

			if (data->remain_cnt == 0) {
				return;
			}
		}
	}

}

/*************************************************************************************************/
void sd_transfer_pio_write(struct sd_host *sd)
{
	struct	sd_data		*data	= sd->data;
	int blksize;
	u32 value, chunk_remain;
	u8 *buffer;
	int  size, i;

	TRACE(("sdhci_write_block_pio ===>"));

	if (data->buf_type == BUF_IS_MDL) {
		unsigned long flags;
		size_t blksize, len, chunk;
		u32 scratch;
		u8 *buf;

		blksize = sd->data->blksz;
		chunk = 0;
		scratch = 0;

		local_irq_save(flags);

		while (blksize) {
			if (!sg_miter_next(&sd->sg_miter))
				BUG();

			len = min(sd->sg_miter.length, blksize);

			data->remain_cnt -= len;

			blksize -= len;
			sd->sg_miter.consumed = len;

			buf = sd->sg_miter.addr;

			while (len) {
				scratch |= (u32)*buf << (chunk * 8);

				buf++;
				chunk++;
				len--;

				if ((chunk == 4) || ((len == 0) && (blksize == 0))) {
					writel(scratch, sd->ioaddr + SD_BUFFER_PORT);
					chunk = 0;
					scratch = 0;
				}
			}
		}

		sg_miter_stop(&sd->sg_miter);

		local_irq_restore(flags);
	}
	else {
		buffer = (u8*)data->buf;
		buffer += data->xfered_cnt;

		blksize = data->blksz;
		while (blksize) {
			chunk_remain = min(blksize, 4);
			size = min(data->remain_cnt, chunk_remain);

			blksize			 -= size;
			data->remain_cnt -= size;
			data->xfered_cnt += size;

			value = 0;
			for (i=0; i < size; i++) {
				value |= ((u32)*buffer << (i*8));
				buffer++;
			}

			writel(value, sd->ioaddr + SD_BUFFER_PORT);

			if (data->remain_cnt == 0) {
				return;
			}
		}
	}
	return;
}

/*************************************************************************************************/
void sd_transfer_data(struct sd_host *sd, struct sd_data *data)
{
	u32 xfer_length;
	u8 xfer_ctrl;

	TRACE(("sd_transfer_data ===>"));

	if (data == NULL) {
		return;
	}

	sd->data = data;

	xfer_length = data->blksz * data->blocks;

	if (!(sd->flags & SDHCI_USE_DMA) || (data->buf_type == BUF_NOT_MDL)) {
		if (xfer_length > 0x200) {
			xfer_length = 0x200;
		}
	}


	writel(xfer_length,	sd->ioaddr + SD_XFER_LENGTH);


	xfer_ctrl = SD_DATA_START_XFER;
	if (data->flags & MMC_DATA_WRITE) {
		xfer_ctrl |= SD_DATA_WRITE;
	}

	if (sd->flags & SDHCI_USE_DMA) {
		if (data->buf_type == BUF_IS_MDL) {
			xfer_ctrl |= SD_DATA_DMA_MODE;
		}
	}

	TRACE1(("xfer_length: %x, xfer_ctrl: %x", xfer_length, xfer_ctrl));
	writeb(xfer_ctrl, sd->ioaddr + SD_DATA_XFER_CTRL);
}

/*************************************************************************************************/
void sd_finish_data(struct sd_host *sd)
{
	struct sd_data *data;

	TRACE(("sd_finish_data ===>"));

	if (sd->data) {

		if (sd->data->error) {
			tasklet_schedule(&sd->finish_tasklet);
			return;
		}

		if (sd->data->remain_cnt) {
			TRACE(("sd->data->remain_cnt: %x", sd->data->remain_cnt));
			if (!(sd->flags & SDHCI_USE_DMA) || (sd->data->buf_type == BUF_NOT_MDL)) {
				sd_transfer_data(sd, sd->data);
				return;
			}
		}
	}

	data = sd->data;
	sd->data = NULL;

	if (sd->srq->stop) {
		/*
		 * The controller needs a reset of internal state machines
		 * upon error conditions.
		 */
		if (data) {
			if (data->error) {
				sd_reset(sd);
			}
		}

		sd_send_command(sd, sd->srq->stop, TRUE);
	} else {
		tasklet_schedule(&sd->finish_tasklet);
	}
}

/*************************************************************************************************/
void sd_prepare_data(struct sd_host *sd, struct sd_data *data)
{

	TRACE(("sd_prepare_data ===>"));

	if (data == NULL) {
		return;
	}


	sd->data = data;

	if (sd->flags & SDHCI_USE_DMA) {
		sd->flags |= SDHCI_REQ_USE_DMA;
	}

	if (data->buf_type == BUF_NOT_MDL) {
		TRACE(("data->data_buf_type == BUF_NOT_MDL, Cancel DMA ..."));
		sd->flags &= ~SDHCI_REQ_USE_DMA;
		return;
	}

	if (sd->flags & SDHCI_REQ_USE_DMA) {

		TRACE(("Use DMA mode to transfer data, ddddddddddddddddddd"));
		if (data->sg) {
			data->dma_addr_map = dma_map_page(&sd->pdx->pci->dev,
								sg_page(data->sg),
								data->sg->offset,
								data->sg->length,
							   	(data->flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
			data->dma_addr_cur = data->dma_addr_map;
			data->sg_map = data->sg;

			writel((u32)data->dma_addr_cur, sd->ioaddr + SD_DMA_ADDRESS);
			TRACE1(("data->sg: %p, data->dma_addr_map: %x, sg->offset: %x, sg->length: %x, dma_addr_cur: %x",
				data->sg, (u32)data->dma_addr_map, data->sg->offset, data->sg->length, (u32)data->dma_addr_cur));
		}
		else {
			sd->flags &= ~SDHCI_REQ_USE_DMA;
		}
	}


	if (!(sd->flags & SDHCI_REQ_USE_DMA)) {
		int flags;

		TRACE(("Use PIO mode to transfer data, pppppppppppppppppp"));
		data->xfered_cnt = 0;
		data->remain_cnt = data->buf_len;

		flags = 0;
		flags = SG_MITER_ATOMIC;
		if (data->flags & MMC_DATA_READ)
			flags |= SG_MITER_TO_SG;
		else
			flags |= SG_MITER_FROM_SG;
		sg_miter_start(&sd->sg_miter, data->sg, data->sg_cnt, flags);
	}
	return;
}

/*************************************************************************************************/
void sd_send_command(struct sd_host *sd, struct sd_command *cmd, u8 is_stop)
{
	u8 cmd_ctrl;
	u32 cmd_parm;

	TRACE(("sd_send_command ===>"));

	cmd_ctrl = 0;

	if (is_stop) {
		sd->stop = cmd;
		/*writeb(0x00, sd->ioaddr + SD_OPT);*/
	}
	else {
		sd->cmd = cmd;

		if (sd->srq->data) {

			sd_prepare_data(sd, sd->srq->data);

			/* read command */
			if (sd->srq->data->flags & MMC_DATA_READ) {
				sd_transfer_data(sd, sd->srq->data);
			}
			else {
				writeb(SD_DATA_WRITE, sd->ioaddr + SD_DATA_XFER_CTRL);
			}
		}
	}

	if (cmd->opcode == MMC_STOP_TRANSMISSION) {
		cmd_ctrl |= SD_CMD_STOP_WAIT_RDY;
	}

// 	if (cmd->opcode == MMC_STOP_TRANSMISSION) {
// 		writeb(SD_OPT_CMD_NWT, sd->ioaddr + SD_OPT);
// 	}
// 	else {
// 		writeb(0, sd->ioaddr + SD_OPT);
// 	}

	TRACE1(("cmd->opcode: (%d), cmd->arg: %08x", cmd->opcode, cmd->arg));
	cmd_parm = am_swap_u32(cmd->arg);

	writeb((u8)(cmd->opcode | 0x40), sd->ioaddr + SD_COMMAND + 3);
	writel(cmd_parm, sd->ioaddr + SD_COMMAND + 4);


	cmd_ctrl |= SD_CMD_START_XFER;
	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			cmd_ctrl |= SD_CMD_17_BYTE_CRC;
		}
		else {
			if (cmd->flags & MMC_RSP_CRC) {
				cmd_ctrl |= SD_CMD_6_BYTE_CRC;
			}
			else {
				cmd_ctrl |= SD_CMD_6_BYTE_WO_CRC;
			}
		}
	}
	else {
		cmd_ctrl |= SD_CMD_NO_RESP;
	}

	/*tasklet_schedule(&sd->timer_tasklet);*/
	if (sd->sd_in_tuning) {
		mod_timer(&sd->timeout_timer, jiffies + 1 * HZ);
	}
	else {
		mod_timer(&sd->timeout_timer, jiffies + 10 * HZ);
	}

	writeb(cmd_ctrl, sd->ioaddr + SD_CMD_XFER_CTRL);

}

/*************************************************************************************************/
void sd_finish_command(struct sd_host *sd, struct sd_command *cmd)
{
	int i;
	u32 resp;

	TRACE(("sd_finish_command ===>"));

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				resp = readl(sd->ioaddr + SD_RESPONSE + i*4);
				cmd->resp[i] = am_swap_u32(resp);
			}
		} else {
			resp = readl(sd->ioaddr + SD_RESPONSE);
			cmd->resp[0] = am_swap_u32(resp);
			TRACE1(("cmd respond: %x", cmd->resp[0]));
		}
	}

}
/*************************************************************************************************/
void sdhci_irq_cmd(struct sd_host *sd, u32 intmask)
{
	u8 is_stop;
	struct sd_command *cmd;

	TRACE(("sdhci_irq_cmd ===>, intmask: %x", intmask));

	if (!sd->cmd && !sd->stop) {
		TRACEX(("sd->cmd == NULL && sd->stop == NULL, intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		sd_dumpregs(sd);
		return;
	}

	is_stop = 0;
	if (sd->cmd) {
		TRACE(("cmd"));
		cmd = sd->cmd;
		sd->cmd = NULL;
	}
	if (sd->stop) {
		TRACE(("stop"));
		is_stop = TRUE;
		cmd = sd->stop;
		sd->stop = NULL;
	}

	if (intmask & (SD_INT_CMD_TIMEOUT_ERR | SD_INT_CMD_CRC_ERR | SD_INT_CMD_END_BIT_ERR | SD_INT_CMD_INDEX_ERR)) {
		TRACEX(("is_stop: %x, cmd->error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", is_stop, intmask));
		cmd->error = intmask;

		/* sd 3.0 tunning case, hardware report command CRC error, but sd card still send data to host,
		   and hardware will report data CRC error again */
		sd->data = NULL;
	}

	if (cmd->error) {
		tasklet_schedule(&sd->finish_tasklet);
	}
	else if (intmask & SD_INT_CMD_END) {
		sd_finish_command(sd, cmd);
		if (sd->srq->data) {
			if (is_stop) {
				tasklet_schedule(&sd->finish_tasklet);
			}
			else {
				/* write command */
				if (sd->srq->data->flags & MMC_DATA_WRITE) {
					sd_transfer_data(sd, sd->srq->data);
				}
			}
		}
		else {
			/* no data command*/
			tasklet_schedule(&sd->finish_tasklet);
		}
	}
}

/*************************************************************************************************/
void sdhci_irq_data(struct sd_host *sd, u32 intmask)
{
	struct sd_data *data;

	TRACE(("sdhci_irq_data ===>, intmask: %x", intmask));

	if (!sd->data) {
		TRACEX(("sd->data == NULL, intmask: %x, XXXXXXXXXXXXXXXXXXX", intmask));
		sd_dumpregs(sd);
		return;
	}

	data = sd->data;

	if (intmask & (SD_INT_DATA_TIMEOUT_ERR | SD_INT_DATA_CRC_ERR | SD_INT_DATA_END_BIT_ERR)) {
		TRACEX(("sdhci_data_irq, intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		sd->data->error = intmask;
	}

	if (sd->data->error) {
		sd_finish_data(sd);
		return;
	}

	if (intmask & (SD_INT_READ_BUF_RDY | SD_INT_WRITE_BUF_RDY)) {
		if (intmask & SD_INT_READ_BUF_RDY) {
			sd_transfer_pio_read(sd);
		}
		else {
			sd_transfer_pio_write(sd);
		}
	}

	if (intmask & SD_INT_DMA_END) {
		TRACE(("SD_INT_DMA_END: "));

		if (data->dma_addr_map == data->dma_addr_cur) {
			data->dma_addr_cur += (DMA_UNIT_SIZE - (data->dma_addr_map & (DMA_UNIT_SIZE-1)));
		}
		else {
			data->dma_addr_cur += DMA_UNIT_SIZE;
		}
		TRACE1(("dma_addr_cur: %x, dma_addr_map: %x, sg->offset: %x, sg->lengh: %x", (u32)data->dma_addr_cur, (u32)data->dma_addr_map, data->sg->offset, data->sg->length));
		if (data->dma_addr_cur >= (data->dma_addr_map + data->sg->length)) {
			TRACE(("dma_addr_map: %x, sg->length: %x, dma_addr_cur: %x", (u32)data->dma_addr_map, data->sg->length, (u32)data->dma_addr_cur));
			dma_unmap_page(&sd->pdx->pci->dev,
							data->dma_addr_map,
							data->sg->length,
							(data->flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
			data->sg_map = 0;
			data->dma_addr_map = 0;

			data->sg = sg_next(data->sg);
			TRACE(("next sg: %p", data->sg));
			if (data->sg) {
				data->dma_addr_map = dma_map_page(&sd->pdx->pci->dev,
									sg_page(data->sg),
									data->sg->offset,
									data->sg->length,
							   		(data->flags & MMC_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
				data->dma_addr_cur = data->dma_addr_map;
				data->sg_map = data->sg;
				TRACE1(("data->sg: %p, data->dma_addr_map: %x, sg->offset: %x, sg->length: %x",
					data->sg, (u32)data->dma_addr_map, data->sg->offset, data->sg->length));
				writel((u32)data->dma_addr_cur, sd->ioaddr + SD_DMA_ADDRESS);
			}
		}
		else {
			writel((u32)data->dma_addr_cur, sd->ioaddr + SD_DMA_ADDRESS);
		}

	}

	if (intmask & SD_INT_DATA_END) {
		TRACE1(("SD_INT_DATA_END -----"));
		sd_finish_data(sd);
	}
}

/*************************************************************************************************/
irqreturn_t sdhci_irq(struct _DEVICE_EXTENSION *pdx)
{
	struct sd_host		*sd = pdx->sd;
	u32 intmask;

	if (sd == NULL) {
		TRACEX(("sdhci_irq, sd == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return IRQ_NONE;
	}

	intmask = readl(sd->ioaddr + SD_INT_STATUS);

	TRACE(("sdhci_irq ===>, intmask: %x", intmask));

	if (!intmask || intmask == 0xffffffff) {
		//TRACE(("intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		return IRQ_NONE;
	}

	if (intmask & (SD_INT_CARD_INSERT | SD_INT_CARD_REMOVE)) {
		if (intmask & SD_INT_CARD_REMOVE) {
			sd->card_inserted = 0;
		}
		else {
			sd->card_inserted = 1;
		}
		writel(intmask & (SD_INT_CARD_INSERT | SD_INT_CARD_REMOVE), sd->ioaddr + SD_INT_STATUS);
		tasklet_schedule(&sd->card_tasklet);
		intmask &= ~(SD_INT_CARD_INSERT | SD_INT_CARD_REMOVE);
	}

	if (intmask & (SD_INT_CMD_TIMEOUT_ERR | SD_INT_DATA_TIMEOUT_ERR)) {
		TRACEX(("TIMEOUT_ERR: intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
	}

	if (intmask & SD_INT_CMD_MASK) {
		writel(intmask & SD_INT_CMD_MASK, sd->ioaddr + SD_INT_STATUS);
		sdhci_irq_cmd(sd, intmask & SD_INT_CMD_MASK);
		intmask &= ~(SD_INT_CMD_MASK);
	}

	if (intmask & SD_INT_DATA_MASK) {
		writel(intmask & SD_INT_DATA_MASK, sd->ioaddr + SD_INT_STATUS);
		sdhci_irq_data(sd, intmask & SD_INT_DATA_MASK);
		intmask &= ~(SD_INT_DATA_MASK);
	}

	if (intmask & SD_INT_OVER_CURRENT_ERR) {
		TRACEX(("SD_INT_OVER_CURRENT_ERR, intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		writel(SD_INT_OVER_CURRENT_ERR, sd->ioaddr + SD_INT_STATUS);
		intmask &= ~SD_INT_OVER_CURRENT_ERR;
		if (pdx->uExtEnOverCurrent) {
			sd->scard.over_current = pdx->media_card.over_current = (OC_IS_OVER_CURRENT | OC_POWER_IS_ON);
		}
	}

	intmask &= ~SD_INT_ERROR;

	if (intmask) {
		TRACEX(("intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		sd_dumpregs(sd);
		writel(intmask, sd->ioaddr + SD_INT_STATUS);
	}

	return IRQ_HANDLED;
}

/*****************************************************************************\
 *                                                                           *
 * Tasklets                                                                  *
 *                                                                           *
\*****************************************************************************/
/*************************************************************************************************/
void sdhci_tasklet_card(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct sd_host		*sd = pdx->sd;

	TRACE2(("==========================================="));
	TRACE2(("sdhci_tasklet_card ===>"));


	if (sd->card_inserted) {
		TRACEW(("sd card is inserted, OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
		pdx->card_init_flag |= SD_CARD_TYPE;
		complete(&pdx->card_ready);
		return;
	}

	TRACEW(("sd card is removed, ...................................."));

	if (sd->srq) {
		TRACEW(("Card removed during transfer, Resetting controller, ..."));
		sd_reset(sd);

		if (sd->srq->cmd) {
			sd->srq->cmd->error = ERR_NO_MEDIUM;
		}
		tasklet_schedule(&sd->finish_tasklet);
	}

	sd_update_media_card(sd);

}

/*************************************************************************************************/
void sdhci_tasklet_finish(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct sd_host		*sd = pdx->sd;
	struct sd_request	*srq;
	int		err;

	TRACE(("sdhci_tasklet_finish ===>"));

	KeCancelTimer(&sd->timeout_timer);

	srq = sd->srq;

	err = 0;

	/* The controller needs a reset of internal state machines upon error conditions. */
	if (srq) {
		if (srq->cmd) {
			if (srq->cmd->error) {
				TRACEX(("srq->cmd->error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", srq->cmd->error));
				err = 1;
			}
		}

		if (srq->data) {
			if (srq->data->error) {
				TRACEX(("srq->data->error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", srq->data->error));
				err = 1;
			}

			if (srq->stop) {
				if (srq->stop->error) {
					TRACEX(("srq->stop->error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", srq->stop->error));
					err = 1;
					srq->data->error = 1; /* force re-stop command */
				}
			}
		}
	}
	else {
		TRACEX(("srq == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
	}

	if (err) {
 		sd_reset(sd);
	}

	sd->srq  = NULL;
	sd->cmd  = NULL;
	sd->data = NULL;
	sd->stop = NULL;

	if (srq) {
		sd_request_done(sd, srq);
	}
}

/*************************************************************************************************/
void sdhci_tasklet_timeout(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct sd_host		*sd = pdx->sd;

	TRACEX(("sdhci_tasklet_timeout ===>"));

	if (!sd) {
		TRACEX(("sd == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	if (sd->sd_chking_card_ready) {
		sd->sd_card_ready_timeout = 1;
	}

	if (sd->scard.sdxc_tuning) {
		TRACEX(("sdxc tuning timeout, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		sd->scard.sdxc_tuning_timeout = 1;
	}

	if (sd->srq) {
		TRACEX(("Timeout waiting for hardware interrupt, XXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		sd_dumpregs(sd);

		if (sd->data) {
			sd->data->error = ERR_TIME_OUT;
			/* sd_finish_data(sd); */
		}
		else {
			if (sd->cmd) {
				sd->cmd->error = ERR_TIME_OUT;
			}
			else {
				sd->srq->cmd->error = ERR_TIME_OUT;
			}

		}

		tasklet_schedule(&sd->finish_tasklet);
	}

}

/*****************************************************************************\
*                                                                           *
* Low level functions                                                       *
*                                                                           *
\*****************************************************************************/
/*************************************************************************************************/
void __________sd_low_level__________(void)
{

}

/*************************************************************************************************/
int sd_card_active_ctrl(struct sd_host *sd, u8 active)
{
	u8 flag;

	TRACE2(("sd_card_active_ctrl ===>, active: %x", active));

	if (active) {
		flag = SD_CARD_BIT;
	}
	else {
		flag = 0;
	}

	card_active_ctrl(sd->pdx, flag);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void sd_reset(struct sd_host *sd)
{
	TRACE2(("=================================="));
	TRACE2(("sd_reset ===>"));

	card_reset(sd->pdx, SD_CARD_BIT);

	sd_init_int(sd);

	/* hardware will reset this register */
	writeb(sd->bus_mode, sd->ioaddr + SD_BUS_MODE_CTRL);
}

/*************************************************************************************************/
void sd_init_int(struct sd_host *sd)
{
	u32 intmask;

	TRACE2(("================================"));
	TRACE2(("sd_init_int ===>"));

	sd_card_active_ctrl(sd, TRUE);

	intmask = SD_INT_OVER_CURRENT_ERR | SD_INT_DATA_END_BIT_ERR |
		SD_INT_DATA_CRC_ERR | SD_INT_DATA_TIMEOUT_ERR | SD_INT_CMD_INDEX_ERR |
		SD_INT_CMD_END_BIT_ERR | SD_INT_CMD_CRC_ERR | SD_INT_CMD_TIMEOUT_ERR |
		SD_INT_CARD_REMOVE | SD_INT_CARD_INSERT |
		SD_INT_READ_BUF_RDY | SD_INT_WRITE_BUF_RDY |
		SD_INT_DMA_END | SD_INT_DATA_END | SD_INT_CMD_END;

	writel(intmask, sd->ioaddr + SD_INT_ENABLE);
}

/*************************************************************************************************/
void sd_power_on(struct sd_host *sd)
{

	TRACEW(("sd_power_on ===>"));

	card_power_ctrl(sd->pdx, SD_CARD_BIT, TRUE);

	TRACE2(("set SD_DATA_XFER_CTRL: SD_DATA_WRITE ..."));
	writeb(SD_DATA_WRITE, sd->ioaddr + SD_DATA_XFER_CTRL);
	sys_delay(200);

}

/*************************************************************************************************/
void sd_power_off(struct sd_host *sd)
{

	TRACEW(("sd_power_off ===>"));

	card_power_ctrl(sd->pdx, SD_CARD_BIT, FALSE);

	sys_delay(100);
}

/*************************************************************************************************/
void sd_set_clock(struct sd_host *sd, u32 clock)
{
	TRACE2(("sd_set_clock ===>, clock: (%u)", clock));

	sd->clock = clock;

	if (clock == CARD_MIN_CLOCK) {
		card_set_clock(sd->pdx, CARD_MIN_CLOCK);
		return;
	}

	clock = clock / 1000;
	card_set_clock(sd->pdx, clock);

}

/*************************************************************************************************/
void sd_set_bus_mode(struct sd_host *sd, u8 bus_mode)
{
	TRACE2(("sd_set_bus_mode ===>,bus_mode: %x", bus_mode));
	sd->bus_mode = bus_mode;
	writeb(bus_mode, sd->ioaddr + SD_BUS_MODE_CTRL);

}

/*************************************************************************************************/
void sd_set_timing_mode(struct sd_host *sd, u8 positive_edge)
{
	u8 clk_delay;

	TRACE2(("sd_set_timing_mode ===>"));

	clk_delay = readb(sd->ioaddr + SD_CLK_DELAY);
	TRACE2(("read SD_CLK_DELAY: %x", clk_delay));
	if (positive_edge) {
		clk_delay |= (SD_CLK_DATA_POSITIVE_EDGE | SD_CLK_CMD_POSITIVE_EDGE);
	}
	else {
		clk_delay &= ~(SD_CLK_DATA_POSITIVE_EDGE | SD_CLK_CMD_POSITIVE_EDGE);
	}

	TRACE2(("write SD_CLK_DELAY: %x", clk_delay));
	writeb(clk_delay, sd->ioaddr + SD_CLK_DELAY);
}

/*************************************************************************************************/
void sd_clk_phase_ctrl(struct sd_host *sd, u8 phase)
{
	u8 clk_delay;

	TRACE2(("sd_clk_phase_ctrl ===>, phase: %x", phase));

	clk_delay = readb(sd->ioaddr + SD_CLK_DELAY);
	TRACE2(("read SD_CLK_DELAY : %x", clk_delay));

	clk_delay &= 0xf0;
	clk_delay |= phase;
	TRACE2(("write SD_CLK_DELAY: %x", clk_delay));

	writeb(clk_delay, sd->ioaddr + SD_CLK_DELAY);
}

/*************************************************************************************************/
void sd_dumpregs(struct sd_host *sd)
{
	TRACEY(("=================================================================="));
	TRACEY(("SD_DEBUG:			(0c): %08x", readl(sd->ioaddr + 0x0c)));
	TRACEY(("CARD_DMA_ADDRESS   (00): %08x", readl(sd->ioaddr + CARD_DMA_ADDRESS)));
	TRACEY(("CARD_CLK_SELECT    (72): %04x", readw(sd->ioaddr + CARD_CLK_SELECT)));
	TRACEY(("CARD_ACTIVE_CTRL   (75): %02x", readb(sd->ioaddr + CARD_ACTIVE_CTRL)));
	TRACEY(("CARD_POWER_CTRL    (70): %02x", readb(sd->ioaddr + CARD_POWER_CONTROL)));
	TRACEY(("CARD_OUTPUT_ENABLE (7a): %02x", readb(sd->ioaddr + CARD_OUTPUT_ENABLE)));
	TRACEY(("CARD_XFER_LENGTH   (6c): %08x", readl(sd->ioaddr + CARD_XFER_LENGTH)));

	TRACEY(("SD_STATUS_CHK      (80): %02x", readb(sd->ioaddr + SD_STATUS_CHK)));
	TRACEY(("SD_COMMAND OP      (23): %02x", readb(sd->ioaddr + SD_COMMAND + 3)));
	TRACEY(("SD_ARGUMENT        (24): %08x", readl(sd->ioaddr + SD_COMMAND + 4)));
	TRACEY(("SD_CMD_XFER_CTRL   (81): %02x", readb(sd->ioaddr + SD_CMD_XFER_CTRL)));
	TRACEY(("SD_BUS_MODE_CTRL   (82): %02x", readb(sd->ioaddr + SD_BUS_MODE_CTRL)));
	TRACEY(("SD_DATA_XFER_CTRL  (83): %02x", readb(sd->ioaddr + SD_DATA_XFER_CTRL)));
	TRACEY(("SD_DATA_PIN_STATE  (84): %02x", readb(sd->ioaddr + SD_DATA_PIN_STATE)));
	TRACEY(("SD_OPT             (85): %02x", readb(sd->ioaddr + SD_OPT)));
	TRACEY(("SD_CLK_DELAY       (86): %02x", readb(sd->ioaddr + SD_CLK_DELAY)));

	TRACEY(("SD_INT_STATUS      (90): %08x", readl(sd->ioaddr + SD_INT_STATUS)));
	TRACEY(("SD_INT_ENABLE      (94): %08x", readl(sd->ioaddr + SD_INT_ENABLE)));
	TRACEY(("=================================================================="));
}

/*************************************************************************************************/
void __________sd_card_command__________(void)
{

}

/*************************************************************************************************/
int sd_wait_for_data(struct sd_host *sd, struct sd_data *data)
{
	struct sd_request srq;

	memset(&srq, 0, sizeof(struct sd_request));

	srq.data = data;

	sd_wait_for_req(sd, &srq);

	return data->error;
}

/*************************************************************************************************/
int sd_wait_for_cmd(struct sd_host *sd, struct sd_command *cmd, int retries)
{
	struct sd_request srq;

	memset(&srq, 0, sizeof(struct sd_request));
	memset(cmd->resp, 0, sizeof(cmd->resp));
	cmd->retries = retries;

	srq.cmd = cmd;

	sd_wait_for_req(sd, &srq);

	return cmd->error;
}

/*************************************************************************************************/
void sd_wait_for_req(struct sd_host *sd, struct sd_request *srq)
{
	/*KEVENT event;*/
	DECLARE_COMPLETION_ONSTACK(complete);

	if (!sd->card_inserted) {
		TRACEW(("sd->card_inserted == 0, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		if (srq->cmd) {
			srq->cmd->error = ERR_NO_MEDIUM;
		}
		if (srq->data) {
			srq->data->error = ERR_NO_MEDIUM;
		}
		return;
	}

	/*KeInitializeEvent(&event, NotificationEvent, FALSE);*/

	srq->done_data	= &complete;/*&event;*/
	srq->done		= sd_wait_done;

	sd_start_request(sd, srq);

	/*KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);*/
	wait_for_completion(&complete);

	if (sd->clock == CARD_MIN_CLOCK) {
		udelay(sd->uSdExtInitCmdDelayUs);
	}

}

/*************************************************************************************************/
void sd_start_request(struct sd_host *sd, struct sd_request *srq)
{

	if (srq->cmd) {
		TRACE1(("(CMD %u), arg %08x, flags %08x", srq->cmd->opcode, srq->cmd->arg, srq->cmd->flags));
	}

	if (srq->data) {
		TRACE1(("blksz: (%d), blocks: (%d), flags: %08x",
			srq->data->blksz,
			srq->data->blocks,
			srq->data->flags));
		srq->data->remain_cnt = srq->data->blksz * srq->data->blocks;
	}

	if (srq->stop) {
		TRACE1(("CMD%u arg %08x flags %08x", srq->stop->opcode, srq->stop->arg, srq->stop->flags));
	}


	sd->srq = srq;

	if (srq->cmd) {
		sd_send_command(sd, srq->cmd, FALSE);
	}
	else {
		if (srq->data) {
			sd_transfer_data(sd, srq->data);
		}
	}
}

/*************************************************************************************************/
void sd_request_done(struct sd_host *sd, struct sd_request *srq)
{
	struct sd_command *cmd;
	int err;

	TRACE(("sd_request_done ===>"));

	/* only data transfer */
	if (srq->cmd == NULL) {

		if (srq->data) {
			TRACE(("%x bytes transferred, data->error: %x", srq->data->xfered_cnt, srq->data->error));
		}

		if (srq->done) {
			srq->done(srq);
		}
		return;
	}

	cmd = srq->cmd;
	err = cmd->error;
	if (err && cmd->retries) {
		TRACEW(("Error Retry: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd->retries));
		TRACEW(("CMD %u arg %08x flags %08x", srq->cmd->opcode, srq->cmd->arg, srq->cmd->flags));

		cmd->retries--;
		cmd->error = 0;
		sd_start_request(sd, srq);

	} else {

		TRACE(("Reg done: (CMD %u) Err: %x, %08x %08x %08x %08x", cmd->opcode, err,
			cmd->resp[0], cmd->resp[1],
			cmd->resp[2], cmd->resp[3]));

		if (srq->data) {
			TRACE(("%x bytes transferred, data->error: %x", srq->data->xfered_cnt, srq->data->error));
		}

		if (srq->stop) {
			TRACE(("(CMD%u): (%d): %08x %08x %08x %08x", srq->stop->opcode,
				srq->stop->error,
				srq->stop->resp[0], srq->stop->resp[1],
				srq->stop->resp[2], srq->stop->resp[3]));
		}

		if (srq->done) {
			srq->done(srq);
		}
	}
}

/*************************************************************************************************/
void sd_wait_done(struct sd_request *srq)
{
	if (srq->done_data) {
		/*KeSetEvent(srq->done_data, 0, FALSE);*/
		complete(srq->done_data);
	}
}

/*************************************************************************************************/
void __________init_card__________(void)
{

}

/*************************************************************************************************/
void sd_change_pad_drive(struct sd_host *sd)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)sd->pdx;

	TRACE2(("sd_change_pad_drive ===>"));
	TRACE2(("UHS card, change pad driving to high level ......................"));
	writeb((u8)pdx->uExtPadDrive0, pdx->ioaddr + CARD_PAD_DRIVE0);
	writeb((u8)pdx->uExtPadDrive1, pdx->ioaddr + CARD_PAD_DRIVE1);
	writeb((u8)pdx->uExtPadDrive2, pdx->ioaddr + CARD_PAD_DRIVE2);

}

/*************************************************************************************************/
int sd_init_card(struct sd_host *sd)
{
	struct sd_card	*scard = &sd->scard;
	int err;

	u32 cid[4];
	u32 ocr, ocr_resp;
	u32 status;
	u32 max_dtr;
	u32 err_retry = 0;
	u32	i;


	sd->sdr_25_init_clk  = sd->uSdExtSdr25Clock;
	sd->sdr_50_init_clk  = sd->uSdExtSdr50Clock;
	sd->sdr_100_init_clk = sd->uSdExtSdr100Clock;
	sd->mmc_26_init_clk  = sd->uSdExtMmcType26Clock;
	sd->mmc_52_init_clk  = sd->uSdExtMmcType52Clock;

	if (sd->sd_err_init_clk) {
		TRACE2(("sd->sd_err_init_clk: (%u)", sd->sd_err_init_clk));
		sd->sdr_25_init_clk  = sd->sd_err_init_clk;
		sd->sdr_50_init_clk  = sd->sd_err_init_clk;
		sd->sdr_100_init_clk = sd->sd_err_init_clk;
		sd->mmc_26_init_clk  = sd->sd_err_init_clk;
		sd->mmc_52_init_clk  = sd->sd_err_init_clk;
	}

reinit:

	TRACE2(("sd_init_card ===>"));

	sd->last_cmd = 0;
	sd->last_lba = 0;


	sd_power_off(sd);

	if (!sd->card_inserted) {
		TRACE2(("No card is inserted ...................."));
		sd_update_media_card(sd);
		goto err_exit;
	}

	sd_card_active_ctrl(sd, TRUE);
	writeb(0, sd->ioaddr + SD_OPT);
	writeb((u8)sd->uSdExtInitClkDelay, sd->ioaddr + SD_CLK_DELAY);

	memset(scard, 0, sizeof(struct sd_card));

	scard->sd = sd;

	sd->scard.state |= MMC_STATE_PRESENT;

	sd_set_bus_mode(sd, SD_BUS_1_BIT_MODE);
	sd_set_timing_mode(sd, MMC_TIMING_LEGACY);
	sd_set_clock(sd, CARD_MIN_CLOCK);

	sd_power_on(sd);

	writeb((u8)sd->uSdExtHwTimeOutCnt, sd->ioaddr + CARD_TIME_OUT_CTRL);

	TRACEW(("SD_CARD_WRITE_PROTECT: %x ", readb(sd->ioaddr + SD_CARD_WRITE_PROTECT)));
	if (readb(sd->ioaddr + SD_CARD_WRITE_PROTECT) & SD_CARD_BIT) {
		TRACE2(("SDHCI_WRITE_PROTECT, .........................."));
		scard->state |= MMC_STATE_READONLY;
	}

	if (scard->over_current) {
		TRACEX(("Over Current, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		goto err_exit;
	}

	sd->card_type = 0;
	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 */
	sd_go_idle(sd);

	sd->card_type = SD_CARD_TYPE_SD;
	/*
	 * If SD_SEND_IF_COND indicates an SD 2.0
	 * compliant card and we should set bit 30
	 * of the ocr to indicate that we can handle
	 * block-addressed SDHC cards.
	 */
	ocr = 0xfc0000; /* voltage 3.0 ~ 3.6 */
	err = sd_send_if_cond(sd);
	if (err) {
		TRACE2(("SD 1.0, 1.x or MMC card"));
	}
	else {
		TRACE2(("SD 2.0 card"));
		/* ocr |= 1 << 30; */
		ocr |= ((SD_HCS | SD_XPC | SD_S18R) << 24);
	}

	err = sd_send_app_op_cond(sd, ocr, &ocr_resp);
	if (err) {
		TRACE2(("MMC_TYPE_MMC"));
		if (sd->uSdExtDisableMmc) {
			TRACEX(("uSdExtDisableMmc == 1, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			goto err_exit;
		}

		sd->card_type = SD_CARD_TYPE_MMC;
		sd->scard.rca = 1;
		sd->scard.type = MMC_TYPE_MMC;

		/* The extra bit indicates that we support high capacity */
		err = sd_send_op_cond(sd, ocr | (1 << 30), &ocr_resp);
		if (err) {
			TRACEX(("sd_send_op_cond: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto err_exit;
		}
	}

	TRACE2(("ocr_resp: %x", ocr_resp));
	if (ocr_resp & (SD_HCS << 24)) {
		TRACE2(("SD card --- High Capacity mode"));
		sd->card_type = SD_CARD_TYPE_SDHC;
		scard->state |= MMC_STATE_BLOCKADDR;

		TRACE2(("check SDXC function ....."));
		if ((sd->uSdExtNoUhsMode == 0) && (sd->uhs_mode_err == 0)) {
			if (ocr_resp & (SD_S18R << 24)) {
				TRACE2(("Set card->support_18v = TRUE ...................."));

				sd_change_pad_drive(sd);

				sd->card_type = SD_CARD_TYPE_SDXC;
				scard->support_18v = TRUE;
				err = sd30_switch_signal_voltage(sd);
				if (err) {
					TRACEX(("sd30_switch_signal_voltage: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
					goto err_exit;
				}
			}
			else {
				TRACE2(("Set card->support_18v = FALSE ...................."));
			}
		}
	}
	else {
		TRACE2(("Normal Capacity mode"));
	}

	/* Send CID, CMD2_ALL_SEND_CID */
	err = sd_all_send_cid(sd, cid);
	if (err) {
		TRACEX(("sd_all_send_cid: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	/* Get(SD)/Set(MMC) RCA, CMD3_SEND_RELATIVE_ADDR */
	err = sd_set_relative_addr(sd);
	if (err) {
		TRACEX(("sd_set_relative_addr: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	/* Send CSD, CMD9_SEND_CSD */
	err = sd_send_csd(sd, sd->scard.raw_csd);
	if (err) {
		TRACEX(("sd_send_csd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	err = sd_decode_csd(sd);
	if (err) {
		TRACEX(("sd_decode_csd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	/* Select cards to go into transfer state */
	err = sd_select_card(sd);
	if (err) {
		TRACEX(("sd_select_card: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	/* Make sure the device at transfer state, CMD13_SEND_STATUS */
	err = sd_send_status(sd, &status);
	if (err) {
		TRACEX(("sd_send_status: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto err_exit;
	}

	if (mmc_card_sd(scard)) {

		TRACE2(("====================================="));
		TRACE2(("sd card init ===>"));
		/* Read SCR register to get SD specification Version number */
		err = sd_app_send_scr(sd);
		if (err) {
			TRACEX(("sd_app_send_scr: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto err_exit;
		}

		/* Switch to wider bus (if supported) */
		if (scard->scr.bus_widths & SD_SCR_BUS_WIDTH_4) {
			err = sd_app_set_bus_width(sd, MMC_BUS_WIDTH_4);
			if (err) {
				TRACEX(("sd_app_set_bus_width: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto err_exit;
			}

			sd_set_bus_mode(sd, SD_BUS_4_BIT_MODE);
		}

		/* Use CMD6 to switch Current Limit */
		if (ocr_resp & (SD_HCS << 24)) {
			err = sd_switch_current_limit(sd);
			if (err) {
				TRACEX(("sd_switch_current_limit: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto err_exit;
			}
		}

		/* Test if the card supports high-speed mode and, if so, switch to it. */
		err = sd_switch_hs(sd);
		if (err) {
			TRACEX(("sd_switch_hs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto err_exit;
		}

		/* Compute bus speed */
		if (mmc_card_highspeed(scard)) {
			max_dtr = scard->sw_caps.hs_max_dtr;
			TRACE2(("higt spped, max_dtr: (%u) KHz", max_dtr));
		}
		else {
			max_dtr = scard->csd.max_dtr;
			TRACE2(("max_dtr: (%u) KHz", max_dtr));
		}

		// switch clock output edge select
		if (130000 < max_dtr) {
			// hardware request feature
			sd_set_timing_mode(sd, MMC_TIMING_LEGACY);
		}
		if ((25000 < max_dtr) && (max_dtr <= 130000)) {
			sd_set_timing_mode(sd, MMC_TIMING_SD_HS);
		}
		if (max_dtr <= 25000) {
			sd_set_timing_mode(sd, MMC_TIMING_LEGACY);
		}

		sd_set_clock(sd, max_dtr);


		if (max_dtr > 50000) {
			sd->sd_in_tuning = 1;
			writeb(12, sd->ioaddr + CARD_TIME_OUT_CTRL); // 40 * 12 = 480 ms time-out

			err = sd30_tuning_sequence(sd);

			sd->sd_in_tuning = 0;
			writeb((u8)sd->uSdExtHwTimeOutCnt, sd->ioaddr + CARD_TIME_OUT_CTRL);

			if (err) {
				TRACEX(("sd30_tuning_sequence: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto err_exit;
			}
		}

		err = sd_set_blksize(sd);
		if (err) {
			TRACEX(("sd_set_blksize: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto err_exit;
		}
	}

	if (mmc_card_mmc(scard)) {

		TRACE2(("====================================="));
		TRACE2(("mmc card init ===>"));
		err = sd_send_ext_csd(sd);
		if (err) {
			TRACEX(("sd_send_ext_csd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto err_exit;
		}

		/* Activate high speed (if supported) */
		if (scard->ext_csd.hs_max_dtr != 0) {
			TRACE2(("================================"));
			TRACE2(("switch to high speed ===>"));
			err = sd_switch(sd, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
			if (err) {
				TRACEX(("sd_switch: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto err_exit;
			}

			TRACE2(("card->state: %x", scard->state));
			mmc_card_set_highspeed(scard);
			TRACE2(("card->state: %x", scard->state));

			if (scard->ext_csd.hs_max_dtr > 26000) {
				sd_set_timing_mode(sd, MMC_TIMING_MMC_HS);
			}
		}

		TRACE2(("card->state: %x", scard->state));
		/* Compute bus speed */
		if (mmc_card_highspeed(scard)) {
			TRACE2(("card->ext_csd.hs_max_dtr: %x", scard->ext_csd.hs_max_dtr));
			max_dtr = scard->ext_csd.hs_max_dtr;
		}
		else {
			TRACE2(("card->csd.max_dtr: %x", scard->csd.max_dtr));
			max_dtr = scard->csd.max_dtr;
		}
		sd_set_clock(sd, max_dtr);

		/* Activate wide bus (if supported) */
		if (scard->csd.mmca_vsn >= CSD_SPEC_VER_4) {
			err = sd_mmc_check_bus(sd);
			if (err) {
				TRACEX(("sd_mmc_check_bus: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto err_exit;
			}
		}
	}

	sd_update_media_card(sd);
	return ERR_SUCCESS;

err_exit:
	TRACEW(("init card fail, err_retry: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err_retry));

	sd_power_off(sd);

	err_retry++;

	if (scard->over_current) {
		TRACEW(("over_current == 1, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	if (!sd->uSdExtInitErrReduceClk) {
		TRACEW(("uSdExtInitErrReduceClk == 0, ====================================================="));
		return ERR_ERROR;
	}

	if(scard->sdxc_tuning_timeout) {
		sd->uhs_mode_err = 1;
	}

	if (scard->sdxc_tuning_err) {
		u32 clk_array_size;
		clk_array_size = sizeof(g_uhs_clk_array) / sizeof(g_uhs_clk_array[0]);
		for (i=0; i < clk_array_size; i++) {
			if (g_uhs_clk_array[i] <= sd->clock) {
				TRACEY(("sd uhs clock, i: %x, g_uhs_clk_array[i]: %x, sd->clock: %x, ++++++++++++++++++++++++++++++++++", i, g_uhs_clk_array[i], sd->clock));
				break;
			}
		}

		if ((i == clk_array_size) || (i == (clk_array_size-1))) {
			TRACEY(("i: %x, sdxc clk_array_size: %x", i, clk_array_size));
			sd->uhs_mode_err = 1;
		}
		else {
			TRACEY(("g_uhs_clk_array[i+1]: (%u)", g_uhs_clk_array[i+1]));
			if (scard->sd_access_mode & SDR104_ACCESS_MODE) {
				sd->sdr_100_init_clk = g_uhs_clk_array[i+1];
			}
			else {
				sd->sdr_50_init_clk = g_uhs_clk_array[i+1];
			}
		}
	}

	if (err_retry < 5) {
		goto reinit;
	}

	sd_dumpregs(sd);
	TRACEW(("init card fail, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
	return ERR_ERROR;
}

/*************************************************************************************************/
void __________sd_command__________(void)
{

}

/*************************************************************************************************/
int sd_go_idle(struct sd_host *sd)
{
	struct sd_command cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_go_idle ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= MMC_GO_IDLE_STATE;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_NONE;

	sd_wait_for_cmd(sd, &cmd, 0);

	sys_delay(1);
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_app_cmd(struct sd_host *sd, struct sd_card *scard)
{
	int		err;
	struct sd_command	cmd;

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = MMC_APP_CMD;

	if (scard) {
		cmd.arg = scard->rca << 16;
		cmd.flags = MMC_RSP_R1;
	} else {
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1;
	}


	err = sd_wait_for_cmd(sd, &cmd, 0);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* Check that card supported application commands */
	if (!(cmd.resp[0] & R1_APP_CMD)) {
		TRACEW(("cmd.resp[0]: %x, XXXXXXXXXXXXXXXXXXXXXXXX", cmd.resp[0]));
		return ERR_OP_NO_SUPPORT;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_send_app_cmd(struct sd_host *sd, struct sd_card *scard, struct sd_command *cmd, int retries)
{
	struct sd_request srq;

	int i, err;


	err = ERR_IO_ERR;

	for (i = 0;i <= retries;i++) {

		err = sd_app_cmd(sd, scard);
		if (err) {
			continue;
		}

		memset(&srq, 0, sizeof(struct sd_request));

		memset(cmd->resp, 0, sizeof(cmd->resp));
		cmd->retries = 0;

		srq.cmd = cmd;

		sd_wait_for_req(sd, &srq);

		err = cmd->error;
		if (!cmd->error) {
			break;
		}
	}

	return err;
}

/*************************************************************************************************/
int sd_send_op_cond(struct sd_host *sd, u32 ocr, u32 *rocr)
{
	struct sd_command cmd;
	int i, err = 0;


	TRACE2(("=================================================="));
	TRACE2(("sd_send_op_cond ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg   = ocr;
	cmd.flags = MMC_RSP_R3;

	for (i = 100; i; i--) {
		err = sd_wait_for_cmd(sd, &cmd, 0);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (cmd.resp[0] & MMC_CARD_READY) {
			break;
		}

		err = ERR_TIME_OUT;

		sys_delay(10);
	}

	if (rocr) {
		*rocr = cmd.resp[0];
	}

	return err;
}

/*************************************************************************************************/
int sd_send_app_op_cond(struct sd_host *sd, u32 ocr, u32 *rocr)
{
	struct sd_command cmd;
	int i, err = 0;

	TRACE2(("=================================================="));
	TRACE2(("sd_send_app_op_cond ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = SD_APP_OP_COND;
	cmd.arg = ocr;
	cmd.flags = MMC_RSP_R3;

	for (i = 100; i; i--) {
		err = sd_send_app_cmd(sd, NULL, &cmd, MMC_CMD_RETRIES);
		if (err) {
			TRACEX(("sd_send_app_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}

		/* if we're just probing, do a single pass */
		if (ocr == 0) {
			break;
		}

		/* otherwise wait until reset completes */
		if (cmd.resp[0] & MMC_CARD_READY) {
				break;
		}

		err = ERR_TIME_OUT;
		sys_delay(10);
	}

	if (rocr) {
		*rocr = cmd.resp[0];
	}

	return err;
}



/*************************************************************************************************/
int sd_send_if_cond(struct sd_host *sd)
{
	struct sd_command cmd;
	int err;
	static const u8 test_pattern = 0xAA;
	u8 result_pattern;

	TRACE2(("=================================================="));
	TRACE2(("sd_send_if_cond ===>"));
	/*
	* To support SD 2.0 cards, we must always invoke SD_SEND_IF_COND
	* before SD_APP_OP_COND. This command will harmlessly fail for
	* SD 1.0 cards.
	*/

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = (0x01 << 8) | test_pattern;  /* Page 49 of SD spec */
	cmd.flags = MMC_RSP_R7;

	err = sd_wait_for_cmd(sd, &cmd, 0);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	result_pattern = (u8)cmd.resp[0] & 0xFF;

	if (result_pattern != test_pattern) {
		TRACEX(("result_pattern != test_pattern,XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_IO_ERR;
	}

	return 0;
}

/*************************************************************************************************/
int sd_all_send_cid(struct sd_host *sd, u32 *cid)
{
	int		err;
	struct sd_command	cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_all_send_cid ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = MMC_ALL_SEND_CID;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_R2;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	memcpy(cid, cmd.resp, sizeof(u32) * 4);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_set_relative_addr(struct sd_host *sd)
{
	int err;
	struct sd_command cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_set_relative_addr ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = MMC_SET_RELATIVE_ADDR;
	cmd.arg = sd->scard.rca << 16;
	cmd.flags = MMC_RSP_R1;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) return err;

	if (sd->scard.type == MMC_TYPE_SD) {
		sd->scard.rca = (u32)(cmd.resp[0] >> 16);
	}
	else {
		TRACEX(("host->card.type: %x", sd->scard.type));
	}

	TRACE2(("host->card.rca: %x", sd->scard.rca));
	return 0;
}

/*************************************************************************************************/
int sd_send_cxd_native(struct sd_host *sd, u32 arg, u32 *cxd, int opcode)
{
	int err;
	struct sd_command cmd;

	TRACE2(("sd_send_cxd_native ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = opcode;
	cmd.arg   = arg;
	cmd.flags = MMC_RSP_R2;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) {
		return err;
	}

	memcpy(cxd, cmd.resp, sizeof(u32) * 4);

	return err;
}

/*************************************************************************************************/
int sd_send_cxd_data(struct sd_host *sd, u32 opcode, void *buf, u32 len)
{
	struct sd_request srq;
	struct sd_command cmd;
	struct sd_data data;

	TRACE2(("============================"));
	TRACE2(("sd_send_cxd_data ===>"));

	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	srq.cmd		= &cmd;
	srq.data	= &data;

	cmd.opcode	= opcode;
	cmd.arg		= 0;

	/* NOTE HACK:  the MMC_RSP_SPI_R1 is always correct here, but we
	* rely on callers to never use this with "native" calls for reading
	* CSD or CID.  Native versions of those commands use the R2 type,
	* not R1 plus a data block.
	*/
	cmd.flags = MMC_RSP_R1;

	data.blksz	= len;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;

	data.buf		= buf;
	data.buf_len	= len;
	data.buf_type	= BUF_NOT_MDL;

	sd_wait_for_req(sd, &srq);

	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		return data.error;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_send_csd(struct sd_host *sd, u32 *csd)
{
	TRACE2(("=================================================="));
	TRACE2(("sd_send_csd ===>"));

	return sd_send_cxd_native(sd, sd->scard.rca << 16, csd, MMC_SEND_CSD);
}

/*************************************************************************************************/
u32 UNSTUFF_BITS(u32 *resp, u32 start,u32 size)
{
	const int __size = size;
	const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;
	const int __off = 3 - ((start) / 32);
	const int __shft = (start) & 31;
	u32 __res;

	__res = resp[__off] >> __shft;
	if (__size + __shft > 32)
	__res |= resp[__off-1] << ((32 - __shft) % 32);
	return __res & __mask;
}

/*************************************************************************************************/
/*
* Given a 128-bit response, decode to our card CSD structure.
*/
/*************************************************************************************************/
int sd_decode_csd(struct sd_host *sd)
{
	struct sd_card *scard = &sd->scard;
	struct sd_csd *csd = &scard->csd;
	u32 e, m, csd_struct;
	u32 *resp = scard->raw_csd;

	for (e=0; e < 4; e++) {
		TRACE2(("%08x", *(resp+e)));
	}

	TRACE2(("=================================================="));
	TRACE2(("sd_decode_csd ===>"));

	csd_struct = 0;
	if (mmc_card_sd(scard)) {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		TRACE2(("SD card: csd_struct: %x", csd_struct));
		switch (csd_struct) {
		case 0:
		case 3:
			TRACE2(("Sd 1.1 card ............."));
			m = UNSTUFF_BITS(resp, 115, 4);
			e = UNSTUFF_BITS(resp, 112, 3);
			csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
			csd->tacc_clks	 = (u16)UNSTUFF_BITS(resp, 104, 8) * 100;

			m = UNSTUFF_BITS(resp, 99, 4);
			e = UNSTUFF_BITS(resp, 96, 3);
			csd->max_dtr	  = tran_exp[e] * tran_mant[m] / 1000;
			csd->cmdclass	  = (u16)UNSTUFF_BITS(resp, 84, 12);

			e = UNSTUFF_BITS(resp, 47, 3);
			m = UNSTUFF_BITS(resp, 62, 12);
			TRACE2(("m: %x, e: %x", m, e));
			csd->capacity		= (1 + m) << (e + 2);


			csd->read_blkbits	= UNSTUFF_BITS(resp, 80, 4);
			csd->read_partial	= UNSTUFF_BITS(resp, 79, 1);
			csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
			csd->read_misalign	= UNSTUFF_BITS(resp, 77, 1);
			csd->r2w_factor		= UNSTUFF_BITS(resp, 26, 3);
			csd->write_blkbits	= UNSTUFF_BITS(resp, 22, 4);
			csd->write_partial	= UNSTUFF_BITS(resp, 21, 1);

			csd->capacity		= csd->capacity * (1 << (csd->read_blkbits - 9));
			break;

		case 1:
			/*
			* This is a block-addressed SDHC card. Most
			* interesting fields are unused and have fixed
			* values. To avoid getting tripped by buggy cards,
			* we assume those fixed values ourselves.
			*/
			TRACE2(("Block-addressed SDHC card ............."));
			mmc_card_set_blockaddr(scard);

			csd->tacc_ns	 = 0; /* Unused */
			csd->tacc_clks	 = 0; /* Unused */

			m = UNSTUFF_BITS(resp, 99, 4);
			e = UNSTUFF_BITS(resp, 96, 3);
			csd->max_dtr	  = tran_exp[e] * tran_mant[m] / 1000;
			csd->cmdclass	  = (u16)UNSTUFF_BITS(resp, 84, 12);

			m = UNSTUFF_BITS(resp, 48, 22);
			csd->capacity		= (1 + m) << 10;

			csd->read_blkbits	= 9;
			csd->read_partial	= 0;
			csd->write_misalign = 0;
			csd->read_misalign	= 0;
			csd->r2w_factor		= 2; /* Unused */
			csd->write_blkbits	= 9;
			csd->write_partial	= 0;
			break;

		default:
			TRACEX(("csd_struct: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", csd_struct));
			return ERR_INVALID_PARAM;
		}
	}

	if (mmc_card_mmc(scard)) {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		TRACE2(("MMC card: csd_struct: %x", csd_struct));

		csd->mmca_vsn	 = (u8)UNSTUFF_BITS(resp, 122, 4);
		TRACE2(("csd->mmca_vsn: %x", csd->mmca_vsn));
		m = UNSTUFF_BITS(resp, 115, 4);
		e = UNSTUFF_BITS(resp, 112, 3);
		csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
		csd->tacc_clks	 = (u16)UNSTUFF_BITS(resp, 104, 8) * 100;

		m = UNSTUFF_BITS(resp, 99, 4);
		e = UNSTUFF_BITS(resp, 96, 3);
		csd->max_dtr	  = tran_exp[e] * tran_mant[m] / 1000; /* KHz */
		csd->cmdclass	  = (u16)UNSTUFF_BITS(resp, 84, 12);

		e = UNSTUFF_BITS(resp, 47, 3);
		m = UNSTUFF_BITS(resp, 62, 12);
		csd->capacity	  = (1 + m) << (e + 2);

		csd->read_blkbits	= UNSTUFF_BITS(resp, 80, 4);
		csd->read_partial	= UNSTUFF_BITS(resp, 79, 1);
		csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
		csd->read_misalign	= UNSTUFF_BITS(resp, 77, 1);
		csd->r2w_factor		= UNSTUFF_BITS(resp, 26, 3);
		csd->write_blkbits	= UNSTUFF_BITS(resp, 22, 4);
		csd->write_partial	= UNSTUFF_BITS(resp, 21, 1);

		csd->perm_write_protect  = UNSTUFF_BITS(resp, 13, 1);
		csd->temp_write_protect  = UNSTUFF_BITS(resp, 12, 1);

		csd->capacity		= csd->capacity * (1 << (csd->read_blkbits - 9));
	}

	TRACE2(("csd_struct: %x", csd_struct));
 	TRACE2(("csd->tacc_ns: %u, csd->tacc_clks: %u", csd->tacc_ns, csd->tacc_clks));
 	TRACE2(("csd->max_dtr: %u, csd->cmdclass: %x", csd->max_dtr, csd->cmdclass));
	TRACE2(("csd->capacity: 0x%x blocks, %u blocks, %u MB", csd->capacity, csd->capacity, csd->capacity/1024/2));
	TRACE2(("csd->read_blkbits: %x, csd->write_blkbits: %x", csd->read_blkbits, csd->write_blkbits));
	TRACE2(("csd->read_partial: %x, csd->write_partial: %x", csd->read_partial, csd->write_partial));
	TRACE2(("csd->read_misalign: %x, csd->write_misalign: %x", csd->read_misalign, csd->write_misalign));
	TRACE2(("csd->r2w_factor: %x", csd->r2w_factor));
	TRACE2(("csd->perm_write_protect: %x, csd->temp_write_protect: %x", csd->perm_write_protect, csd->temp_write_protect));

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_send_cid(struct sd_host *sd, u32 *cid)
{
	TRACE2(("=================================================="));
	TRACE2(("sd_send_cid ===>"));

	return sd_send_cxd_native(sd, sd->scard.rca << 16, cid, MMC_SEND_CID);

}

/*************************************************************************************************/
int sd_select_card(struct sd_host *sd)
{
	int err;
	struct sd_command cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_select_card ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= MMC_SELECT_CARD;
	cmd.arg		= sd->scard.rca << 16;
	cmd.flags	= MMC_RSP_R1;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_send_status(struct sd_host *sd, u32 *status)
{
	int err;
	struct sd_command cmd;

	TRACE1(("=================================================="));
	TRACE1(("sd_send_status ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode = MMC_SEND_STATUS;
	cmd.arg = sd->scard.rca << 16;
	cmd.flags = MMC_RSP_R1;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* NOTE: callers are required to understand the difference
	 * between "native" and SPI format status words!
	 */
	if (status) {
		*status = cmd.resp[0];
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_app_send_scr(struct sd_host *sd)
{
	struct sd_card	*scard	= &sd->scard;
	struct sd_scr	*scr	= &scard->scr;
	struct sd_request	srq;
	struct sd_command	cmd;
	struct sd_data		data;
	u32 scr_struct;
	u8 buf[8];
	int err;

	TRACE2(("=================================================="));
	TRACE2(("sd_app_send_scr ===>"));

	err = sd_app_cmd(sd, &sd->scard);
	if (err) {
		TRACEX(("sd_app_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	srq.cmd		= &cmd;
	srq.data	= &data;

	cmd.opcode	= SD_APP_SEND_SCR;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_R1;

	data.blksz	= 8;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;
	data.buf		= buf;
	data.buf_len	= 8;
	data.buf_type	= BUF_NOT_MDL;


	sd_wait_for_req(sd, &srq);

	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		return data.error;
	}


	scr_struct = (buf[0] & 0xF0) >> 4;
	if (scr_struct != 0) {
		TRACEX(("scr_struct: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", scr_struct));
		return ERR_INVALID_PARAM;
	}

	scr->sda_vsn	= buf[0] & 0x0f;
	scr->bus_widths = buf[1] & 0x0f;

	TRACE2(("scr->sda_vsn: %x",scr->sda_vsn));
	TRACE2(("scr->bus_widths: %x",scr->bus_widths));
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_app_send_status(struct sd_host *sd)
{
	struct sd_card	*scard	= &sd->scard;
	struct sd_request	srq;
	struct sd_command	cmd;
	struct sd_data		data;
	u8 buf[64];
	int err;

	TRACE2(("=================================================="));
	TRACE2(("sd_app_send_status ===>"));

	err = sd_app_cmd(sd, &sd->scard);
	if (err) {
		TRACEX(("sd_app_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	srq.cmd		= &cmd;

	srq.data	= &data;

	cmd.opcode	= SD_APP_SEND_STATUS;
	cmd.arg		= scard->rca << 16;
	cmd.flags	= MMC_RSP_R1;

	data.blksz	= 64;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;
	data.buf		= buf;
	data.buf_len	= 64;
	data.buf_type	= BUF_NOT_MDL;


	sd_wait_for_req(sd, &srq);

	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		return data.error;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_sd_switch(struct sd_host *sd, int mode, int group, u8 value, u8 *resp)
{
	struct sd_request srq;
	struct sd_command cmd;
	struct sd_data data;

	TRACE2(("==========================="));
	TRACE2(("sd_sd_switch ===>, mode: %x, group: %x, value: %x", mode, group, value));

	mode = !!mode;
	value &= 0xF;

	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	srq.cmd = &cmd;
	srq.data = &data;

	cmd.opcode = SD_SWITCH;
	cmd.arg = mode << 31 | 0x00FFFFFF;
	cmd.arg &= ~(0x0f << ((group-1) * 4));
	cmd.arg |= value << ((group-1) * 4);
	cmd.flags = MMC_RSP_R1;

	data.blksz	= 64;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;
	data.buf_type	= BUF_NOT_MDL;
	data.buf_len	= 64;
	data.buf		= resp;

	sd_wait_for_req(sd, &srq);

	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		return data.error;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_switch_current_limit(struct sd_host *sd)
{
	int err;
	u8 status[64], bTmpLimit, bCurrentLimit;

	TRACE2(("============================"));
	TRACE2(("sd_switch_current_limit ===>"));

	err = sd_sd_switch(sd, SD_SWITCH_CHECK_MODE, SD_FUNCTION_GROUP_4, SD_CURRENT_LIMIT_CURRENT_CURRENT, status);
	if (err) {
		TRACEX(("sd_sd_switch: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* get support functions of function group4 */
	bTmpLimit = (status[7] & CMD6_MAX_CURRENT_LIMIT) >> 1;
	bCurrentLimit = 0;
	while (bTmpLimit) {
		bCurrentLimit++;
		bTmpLimit = (bTmpLimit >> 1);
	}

	if (!bCurrentLimit) {
		TRACE2(("bCurrentLimit == 0, ................................."));
		return ERR_SUCCESS;
	}

	TRACE2(("bCurrentLimit: %x ", bCurrentLimit));
	err = sd_sd_switch(sd, SD_SWITCH_SET_MODE, SD_FUNCTION_GROUP_4, bCurrentLimit, status);
	if (err) {
		TRACEX(("sd_sd_switch: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_switch_hs(struct sd_host *sd)
{
	int err;
	struct sd_card *scard = &sd->scard;
	u8 status[64], bAccessMode;

	TRACE2(("=================================================="));
	TRACE2(("sd_switch_hs ===>"));

	bAccessMode = 0;
	/* if SD_SPEC version number is less than V1.10, skip this procedure */
	if (scard->scr.sda_vsn < SCR_SPEC_VER_1) {
		TRACEX(("card->scr.sda_vsn < SCR_SPEC_VER_1, XXXXXXXXXXXXXXXXXX"));
		return 0;
	}

	if (!(scard->csd.cmdclass & CCC_SWITCH)) {
		TRACEX(("card lacks mandatory switch function, performance might suffer, XXXXXXXXXXXXXXXX"));
		return 0;
	}


	err = sd_sd_switch(sd, SD_SWITCH_CHECK_MODE, SD_FUNCTION_GROUP_1, SD_ACCESS_MODE_CURRENT_SPEED, status);
	if (err) {
		TRACEX(("sd_sd_switch, err: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	scard->sd_access_mode = status[13];

	if (status[13] & SDR25_ACCESS_MODE) {
		TRACE2(("SDR25_ACCESS_MODE, support ...."));
		scard->sw_caps.hs_max_dtr = sd->sdr_25_init_clk; /* 50 MHz */
		bAccessMode = 1;
	}
	if (scard->support_18v) {
		if (status[13] & SDR50_ACCESS_MODE) {
			TRACE2(("SDR50_ACCESS_MODE, support ...."));
			scard->sw_caps.hs_max_dtr = sd->sdr_50_init_clk; /* 100 MHz */
			bAccessMode = 2;
		}
		if (scard->sd->uSdExt200MHzOff == 0) {
			if (status[13] & SDR104_ACCESS_MODE) {
				TRACE2(("SDR104_ACCESS_MODE, support ...."));
				scard->sw_caps.hs_max_dtr = sd->sdr_100_init_clk; /* 200 MHz */
				bAccessMode = 3;
			}
		}
	}

	err = sd_sd_switch(sd, SD_SWITCH_SET_MODE, SD_FUNCTION_GROUP_1, bAccessMode, status);
	if (err) {
		TRACEX(("sd_sd_switch: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	TRACE2(("status[16]: %x", status[16]));
	if ((status[16] & 0x0f) != bAccessMode) {
		TRACEX(("status[16]: %x, bAccessMode: %x, XXXXXXXXXXXXXXXXXXXXX", status[16], bAccessMode));
	} else {
		if (bAccessMode) {
			u8 set_high_speed = FALSE;
			if ((bAccessMode == 1) && (sd->uSdExt50SetHighSpeed)) {
				set_high_speed = TRUE;
			}
			if ((bAccessMode == 2) && (sd->uSdExt100SetHighSpeed)) {
				set_high_speed = TRUE;
			}
			if ((bAccessMode == 3) && (sd->uSdExt200SetHighSpeed)) {
				set_high_speed = TRUE;
			}
			if ((bAccessMode == 4) && (sd->uSdExt100SetHighSpeed)) {
				set_high_speed = TRUE;
			}

			if (set_high_speed) {
				sd_set_timing_mode(sd, MMC_TIMING_SD_HS);
			}

			mmc_card_set_highspeed(scard);
		}
	}

	return err;
}

/*************************************************************************************************/
int sd_switch(struct sd_host *sd, u8 set, u8 index, u8 value)
{
	int err;
	struct sd_command cmd;

	TRACE2(("================================="));
	TRACE2(("sd_switch ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= MMC_SWITCH;
	cmd.arg		= (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
		(index << 16) |
		(value << 8) |
		set;
	cmd.flags = MMC_RSP_R1B;

	err = sd_wait_for_cmd(sd, &cmd, MMC_CMD_RETRIES);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_app_set_bus_width(struct sd_host *sd, int width)
{
	int err;
	struct sd_command cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_app_set_bus_width ===>, width: %x", width));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= SD_APP_SET_BUS_WIDTH;
	cmd.flags	= MMC_RSP_R1;

	switch (width) {
	case MMC_BUS_WIDTH_1:
		cmd.arg = SD_BUS_WIDTH_1;
		break;
	case MMC_BUS_WIDTH_4:
		cmd.arg = SD_BUS_WIDTH_4;
		break;
	default:
		return ERR_INVALID_PARAM;
	}

	err = sd_send_app_cmd(sd, &sd->scard, &cmd, MMC_CMD_RETRIES);
	if (err) {
		TRACEX(("sd_send_app_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_set_blksize(struct sd_host *sd)
{
	int err;
	struct sd_command cmd;

	TRACE2(("=================================================="));
	TRACE2(("sd_set_blksize ===>"));

	/* Block-addressed cards ignore MMC_SET_BLOCKLEN */
	if (sd->scard.state & MMC_STATE_BLOCKADDR) {
		TRACEW(("Block-addressed cards ignore MMC_SET_BLOCKLEN ....."));
		return ERR_SUCCESS;
	}

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= MMC_SET_BLOCKLEN;
	cmd.arg		= 0x200;
	cmd.flags	= MMC_RSP_R1;
	err = sd_wait_for_cmd(sd, &cmd, 0);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return ERR_INVALID_PARAM;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd_send_ext_csd(struct sd_host *sd)
{
	int err;
	struct sd_card *scard = &sd->scard;
	u8 ext_csd[512];
	u32 ext_csd_struct;

	TRACE2(("=============================="));
	TRACE2(("sd_send_ext_csd ===>"));

	if (scard->csd.mmca_vsn < CSD_SPEC_VER_4) {
		TRACEX(("card->csd.mmca_vsn < CSD_SPEC_VER_4: %x,", scard->csd.mmca_vsn));
		return 0;
	}

	err = sd_send_cxd_data(sd, MMC_SEND_EXT_CSD, ext_csd, 512);
	if (err) {
		/*
		* We all hosts that cannot perform the command
		* to fail more gracefully
		*/
		if (err != ERR_INVALID_PARAM)
			goto out;

			/*
			* High capacity cards should have this "magic" size
			* stored in their CSD.
		*/
		if (scard->csd.capacity == (4096 * 512)) {
			TRACEX(("unable to read EXT_CSD on a possible high capacity card. Card will be ignored, XXXXXXXXXXXXXXXX"));
		} else {
			TRACEX(("unable to read EXT_CSD, performance might suffer, XXXXXXXXXXXXXXXX"));
			err = 0;
		}

		goto out;
	}

	ext_csd_struct = ext_csd[EXT_CSD_REV];
	if (ext_csd_struct > 2) {
		TRACEX(("ext_csd_struct > 2, %x, XXXXXXXXXXXXXX", ext_csd_struct));
	}

	if (ext_csd_struct >= 2) {
		scard->ext_csd.sectors =
			ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
			ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
			ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
			ext_csd[EXT_CSD_SEC_CNT + 3] << 24;

		TRACE2(("ext_csd.sectors: %x", scard->ext_csd.sectors));

        scard->csd.capacity = scard->ext_csd.sectors;

		if (scard->ext_csd.sectors)
			mmc_card_set_blockaddr(scard);
	}

	TRACE2(("ext_csd[EXT_CSD_CARD_TYPE]: %x", ext_csd[EXT_CSD_CARD_TYPE]));
	switch (ext_csd[EXT_CSD_CARD_TYPE]) {
	case EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_26:
		TRACE2(("52 MHz ..."));
		scard->ext_csd.hs_max_dtr = sd->mmc_52_init_clk;
		break;
	case EXT_CSD_CARD_TYPE_26:
		TRACE2(("26 MHz ..."));
		scard->ext_csd.hs_max_dtr = sd->mmc_26_init_clk;
		break;
	default:
		TRACEX(("card is mmc v4 but doesn't support any high-speed modes, XXXXXXXXXXXXXX"));
		goto out;
	}

out:

	return err;
}

/*************************************************************************************************/
int sd_stop(struct sd_host *sd)
{
	struct sd_command cmd;

	TRACE1(("=================================================="));
	TRACE1(("sd_stop ===>"));

	memset(&cmd, 0, sizeof(struct sd_command));

	cmd.opcode	= MMC_STOP_TRANSMISSION;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_R1B;

	sd_wait_for_cmd(sd, &cmd, 0);

	return cmd.error;
}

/*************************************************************************************************/
int sd_mmc_check_bus(struct sd_host *sd)
{
	struct sd_request srq;
	struct sd_command cmd;
	struct sd_data data;
	u8 buf[SECTOR_SIZE];
	int err;
	u8 bus_mode, bus_width;
	u32 resp;

	TRACE2(("============================"));
	TRACE2(("sd_mmc_check_bus ===>"));

	sd_set_bus_mode(sd, SD_BUS_8_BIT_MODE);

	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	memset(buf, 0, sizeof(buf));
	buf[0] = 0x55;
	buf[1] = 0xaa;

	srq.cmd		= &cmd;
	srq.data	= &data;

	cmd.opcode	= CMD19_BUSTEST_W;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_R1;

	data.blksz	= SECTOR_SIZE;
	data.blocks = 1;
	data.flags	= MMC_DATA_WRITE;

	data.buf		= buf;
	data.buf_len	= SECTOR_SIZE;
	data.buf_type	= BUF_NOT_MDL;

	sd_wait_for_req(sd, &srq);
	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	/* don't check write-data CRC status */

	cmd.opcode	= CMD14_BUSTEST_R;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_R1;

	data.blksz	= SECTOR_SIZE;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;

	sd_wait_for_req(sd, &srq);
	if (cmd.error) {
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	/* don't check read-data CRC status */

	resp = readl(sd->ioaddr + SD_BUFFER_PORT);
	buf[0] = (u8)resp;
	buf[1] = (u8)(resp >> 8);

	TRACE2(("resp: %x, buf[0]: %x, buf[1]: %x", resp, buf[0], buf[1]));

	if (((buf[0] & 0x0f) == 0x0a) && ((buf[1] & 0x0f) == 0x05)) {
		if ((buf[0] == 0xaa) && (buf[1] == 0x55)) {
			if (sd->uSdExtMmcForce4Bit) {
				TRACE2(("8 bit mode force to 4 bit mode"));
				bus_mode = SD_BUS_4_BIT_MODE;
				bus_width = EXT_CSD_BUS_WIDTH_4;
			}
			else {
				TRACE2(("8 bit mode"));
				bus_mode = SD_BUS_8_BIT_MODE;
				bus_width = EXT_CSD_BUS_WIDTH_8;
			}
		}
		else {
			TRACE2(("4 bit mode"));
			bus_mode = SD_BUS_4_BIT_MODE;
			bus_width = EXT_CSD_BUS_WIDTH_4;
		}
	}
	else {
		TRACE2(("1 bit mode"));
		bus_mode = SD_BUS_1_BIT_MODE;
		bus_width = EXT_CSD_BUS_WIDTH_1;
	}

	if (sd->uSdExtMmcForce1Bit) {
		TRACE2(("Force to 1 bit mode"));
		bus_mode = SD_BUS_1_BIT_MODE;
		bus_width = EXT_CSD_BUS_WIDTH_1;
	}

	err = sd_switch(sd, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, bus_width);
	if (err) {
		TRACEX(("sd_switch: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	sd_set_bus_mode(sd, bus_mode);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void __________sd30_tuning_operation__________(void)
{

}


/*************************************************************************************************/
int sd30_switch_signal_voltage(struct sd_host *sd)
{
	struct sd_command cmd;
	int		err;
	u32		bus;
	u16		mask;

	TRACE2(("=================================================="));
	TRACE2(("sd30_switch_signal_voltage ===>"));
	mask = (SD_OPT_CMD_LINE_LEVEL << 8) | SD_DATA_LINE_LEVEL;

	bus = readw(sd->ioaddr + SD_DATA_PIN_STATE);
	TRACE2(("bus state: %x, mask: %x, (bus & mask): %x", bus, mask, bus & mask));

	memset(&cmd, 0x00, sizeof(struct sd_command));

	cmd.opcode = CMD11_SWITCH_SIGNAL_VOLTAGE;
	cmd.arg    = 0;
	cmd.flags  = MMC_RSP_R1;

	err = sd_wait_for_cmd(sd, &cmd, 0);
	if (err) {
		TRACEX(("sd_wait_for_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	sys_delay(1);
	TRACE2(("sd30_switch_signal_voltage command is OK ......"));

	bus = readw(sd->ioaddr + SD_DATA_PIN_STATE);
	TRACE2(("bus state: %x, mask: %x, (bus & mask): %x, (after switch signal voltage command)", bus, mask, bus & mask));
	bus &= mask;
	if (bus) {
		TRACEX(("Cmd & Dat Line Signal: %08x, XXXXXXXXXXXXXXXXXXXXXXX", bus));
		return ERR_ERROR;
	}

	//sd_set_clock(sd, 0);
	writeb(0x00, sd->ioaddr + SD_DATA_XFER_CTRL);
	sys_delay(1);

	writeb(SD_OPT_SD_18V, sd->ioaddr + SD_OPT);

	sys_delay(1);
	writeb(SD_DATA_WRITE, sd->ioaddr + SD_DATA_XFER_CTRL);
	sd_set_clock(sd, CARD_MIN_CLOCK);

	sys_delay(10);

	bus = readw(sd->ioaddr + SD_DATA_PIN_STATE);
	TRACE2(("bus state: %x, mask: %x, (bus & mask): %x, (after set hardware 1.8v)", bus, mask, bus & mask));
	bus &= mask;

	if (bus != mask) {
		TRACEX(("bus != mask, %x, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", bus, mask));
		return ERR_ERROR;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd30_tuning_cmd(struct sd_host *sd)
{
	struct sd_request	srq;
	struct sd_command	cmd;
	struct sd_data		data;
	u8 buf[TUNING_BLOCK_SIZE], i;

	TRACE2(("=================================================="));
	TRACE2(("sd30_tuning_cmd ===>"));


	memset(&srq, 0, sizeof(struct sd_request));
	memset(&cmd, 0, sizeof(struct sd_command));
	memset(&data, 0, sizeof(struct sd_data));

	srq.cmd		= &cmd;
	srq.data	= &data;

	cmd.opcode	= CMD19_TUNING_TIMEING;
	cmd.arg		= 0;
	cmd.flags	= MMC_RSP_R1;

	data.blksz	= TUNING_BLOCK_SIZE;
	data.blocks = 1;
	data.flags	= MMC_DATA_READ;
	data.buf		= buf;
	data.buf_len	= TUNING_BLOCK_SIZE;
	data.buf_type	= BUF_NOT_MDL;


	sd_wait_for_req(sd, &srq);

	if (cmd.error) {
		sd_stop(sd);
		/* sd card's respond is ok, but hardware see it as wrong respond,
		   sd card want to send data to host. */
		/*
		writeb(SD_DATA_WRITE, sd->ioaddr + SD_DATA_XFER_CTRL);
		sys_delay(1);
		*/
		TRACEX(("cmd.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", cmd.error));
		return cmd.error;
	}
	if (data.error) {
		TRACEX(("data.error: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", data.error));
		return data.error;
	}

	for (i=0; i < TUNING_BLOCK_SIZE; i++) {
		if (buf[i] != sdxc_tuning_pattern[i]) {
			TRACEX(("i: %x, sdxc_tuning_pattern[i]: %x, buf[i]: %x, XXXXXXXXXXXXXXXXXXX", i, sdxc_tuning_pattern[i], buf[i]));
			return ERR_ERROR;
		}
	}

	TRACE2(("sd30_tuning_cmd success !!!"));
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int sd30_tuning_sequence(struct sd_host *sd)
{
	struct sd_card	*scard = &sd->scard;
	int		err;
	u8		bExistVal, i, retry;
	u8		bRangeStart;
	u8		abPhaseIsOk[MAX_TUNING_PHASE];
	u8		bCurrentLo, bCurrentHi, bFitLo, bFitHi;
	u8		bTuningValue;

	TRACE2(("====================================="));
	TRACE2(("sd30_tuning_sequence ===>"));

	memset(abPhaseIsOk, 0x00, sizeof(abPhaseIsOk));

	bExistVal = 0;

	scard->sdxc_tuning = 1;
	scard->sdxc_tuning_err = 0;
	scard->sdxc_tuning_timeout = 0;

	/* Backup solution for DDR operation (DDR don't support tuning command) */
	for (retry=0; retry < 2; retry++) {
		for (i=0; i < MAX_TUNING_PHASE; i++) {

			TRACE2(("====================="));
			TRACE2(("sd30_tuning_sequence, i: %x", i));
			sd_clk_phase_ctrl(sd, i);
			//sys_delay(1);

			if (retry == 1) {
				err = sd_app_send_status(sd); /* Backup solution for DDR operation (DDR don't support tuning command) */
			}
			else {
				err = sd30_tuning_cmd(sd);
			}

			if (err) {
				if (retry == 1) {
					TRACE2(("i: %x, sd_app_send_status: %x, XXXXXXXXXXXXXXXXXXXXXX", i, err));
				}
				else {
					TRACE2(("i: %x, sd30_tuning_cmd: %x, XXXXXXXXXXXXXXXXXXXXXX", i, err));
				}
			}
			else {
				bExistVal = TRUE;
				abPhaseIsOk[i] = TRUE;
			}

			if (scard->sdxc_tuning_timeout) {
				break;
			}
		}

		if (bExistVal) {
			/* Not DDR case, don't tune again */
			break;
		}
	}

	scard->sdxc_tuning = 0;

	if (scard->sdxc_tuning_timeout) {
		TRACEX(("sdxc_tuning_timeout == 1, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	if (!bExistVal) {
		TRACEX(("bExistVal == 0, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		scard->sdxc_tuning_err = 1;
		return ERR_ERROR;
	}

	// check multi-ranges
	bRangeStart = FALSE;
	bCurrentLo = bCurrentHi = 0xff;
	bFitLo = bFitHi = 0xff;
	for (i=0; i < MAX_TUNING_PHASE; i++) {
		if (abPhaseIsOk[i]) {

			bRangeStart = TRUE;
			if (bCurrentLo == 0xff) {
				bCurrentLo = i;  // get range start
			}
		}
		else {
			if (bRangeStart) {

				bRangeStart = FALSE;
				bCurrentHi = i - 1; // get range ending

				TRACEW(("0: bCurrentLo: %x, bCurrentHi: %x", bCurrentLo, bCurrentHi));
				if ((bFitHi == 0xff) && (bFitLo == 0xff)) { // special case, only 1 phase is ok
					bFitHi = bCurrentHi;
					bFitLo = bCurrentLo;
				}
				else {
					// if two ranges are the same size, we select the first
					if ((bCurrentHi - bCurrentLo) > (bFitHi - bFitLo)) {
						bFitHi = bCurrentHi;
						bFitLo = bCurrentLo;
					}
				}
				TRACEW(("1: bFitLo: %x, bFitHi: %x", bFitLo, bFitHi));
				bCurrentLo = 0xff;
			}
		}
	}

	// the last range has no ending, xxoooxxxxooxxoooooo
	if (bRangeStart) {
		bCurrentHi = MAX_TUNING_PHASE - 1;
		TRACEW(("2: bCurrentLo: %x, bCurrentHi: %x", bCurrentLo, bCurrentHi));
		if ((bFitHi == 0xff) && (bFitLo == 0xff)) { // special case, only 1 phase is ok
			bFitHi = bCurrentHi;
			bFitLo = bCurrentLo;
		}
		else {
			// if two ranges are the same size, we select the first
			if ((bCurrentHi - bCurrentLo) > (bFitHi - bFitLo)) {
				bFitHi = bCurrentHi;
				bFitLo = bCurrentLo;
			}
		}
		TRACEW(("3: bFitLo: %x, bFitHi: %x", bFitLo, bFitHi));
	}

	bTuningValue = (bFitHi + bFitLo) / 2;
	TRACEW(("4: bFitLo: %x, bFitHi: %x, ---> bTuningValue: %x", bFitLo, bFitHi, bTuningValue));

	sd_clk_phase_ctrl(sd, bTuningValue);

	return ERR_SUCCESS;
}
