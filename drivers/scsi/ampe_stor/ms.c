#include <linux/vmalloc.h>

#include "amcr_stor.h"
#include "ms-fmt.h"



/*************************************************************************************************/
int ms_add_device(struct _DEVICE_EXTENSION *pdx)
{
	struct ms_host *ms;

	TRACEW(("ms_add_device ===>"));

	//ms = pdx->ms = (struct ms_host *)ExAllocatePool(NonPagedPool, sizeof(struct ms_host));
	ms = pdx->ms = (struct ms_host *)kmalloc(sizeof(struct ms_host), GFP_KERNEL);
	if (!ms) {
		TRACEY(("kmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	memset(ms, 0x00, sizeof(struct ms_host));
	ms->pdx	= pdx;
	ms->ioaddr = pdx->ioaddr;


	KeInitializeDpc(&ms->card_tasklet,	ms_tasklet_card,	(unsigned long)pdx);
	KeInitializeDpc(&ms->finish_tasklet,ms_tasklet_finish,	(unsigned long)pdx);
	//KeInitializeDpc(&ms->dma_tasklet,	ms_tasklet_dma,		pdx);

	//KeInitializeTimerEx(&ms->timeout_timer, NotificationTimer);
	//KeInitializeDpc(&ms->timeout_tasklet, ms_tasklet_timeout, pdx);

	setup_timer(&ms->timeout_timer, ms_tasklet_timeout, (unsigned long)pdx);

	/* debug to force to pio mode */
	ms->flags |= MS_SUPPORT_DMA;

	/*===================================================*/
	ms_get_external_setting(ms);

	ms_reset(ms);

	return ERR_SUCCESS;
}


/*************************************************************************************************/
int ms_remove_device(struct _DEVICE_EXTENSION *pdx)
{
	struct ms_host *ms = pdx->ms;

	TRACEW(("ms_remove_device ===>"));

	//KeCancelTimer(&ms->timeout_timer);
	del_timer_sync(&ms->timeout_timer);

	tasklet_kill(&ms->card_tasklet);
	tasklet_kill(&ms->finish_tasklet);

	writel(0x00, ms->ioaddr + MS_INT_ENABLE);

	ms_power_off(ms);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void __________ms_external_options__________(void)
{

}

/*************************************************************************************************/
int ms_get_external_setting(struct ms_host *ms)
{

	TRACE2(("ms_get_external_setting ===>"));

	/*===============================*/
	/* Ms Host controller			 */
	/*===============================*/
	ms->uMsExt_force_pio = 0;
	if (ms->uMsExt_force_pio) {
		ms->flags &= ~MS_SUPPORT_DMA;
	}

	ms->uMsExtPwrOnDelayTime = 100;

	ms->uMsExtPwrOffDelayTime = 100;

	ms->uMsExtInitRetry = 1;

	ms->uMsExtEnFormatter = 1;


	/*===============================*/
	/* Ms card						 */
	/*===============================*/
	ms->uMsExtClock = 20;


	/*===============================*/
	/* Ms Pro card					 */
	/*===============================*/
	/* clock unit: MHz */
	ms->uMsExtPro1BitClock = 20;

	ms->uMsExtPro4BitClock = 40;

	ms->uMsExtPro8BitClock = 60;


	ms->uMsExtProForce1Bit = 0;

	ms->uMsExtProNo8Bit = 0;

	ms->uMsExtHwTimeOutCnt = 125; /* default: 125 * 40ms = 5 sec */


	return ERR_SUCCESS;
}

/*************************************************************************************************/
void ms_update_media_card(struct ms_host *ms)
{
	struct ms_card		*mcard	= &ms->mcard;
	struct _MEDIA_CARD	*media_card = &ms->pdx->media_card;

	TRACE1(("========================="));
	TRACE1(("ms_update_media_card ===>"));


	if (!ms->card_inserted) {

		TRACEW(("Ms card is removed, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));

		media_card->media_state    = 0;
		media_card->media_capacity = 0;
		media_card->over_current   = 0;

		ms_power_off(ms);
		return;
	}

	TRACEW(("Ms card is inserted, OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
	media_card->card_type = ms->card_type;

	// After ms formatter with full format, driver can't report media change
	media_card->media_state	   = MEDIA_STATE_PRESENT | MEDIA_STATE_CHANGE;
	media_card->media_capacity = mcard->total_lba;

	if (mcard->state & MS_STATE_READONLY) {
		media_card->media_state |= MEDIA_STATE_READONLY;
	}

	return;
}

/*************************************************************************************************/
void __________ms_scsi_thread__________(void)
{

}

/*************************************************************************************************/
void ms_mdl_prepare_data(struct ms_host *ms, u8 force_pio)
{

	if (!ms->sg) {
		TRACEX(("ms->sg == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	if (ms->flags & MS_SUPPORT_DMA) {
		ms->flags |= MS_REQ_USE_DMA;
	}
	else {
		ms->flags &= ~MS_REQ_USE_DMA;
	}

	if (force_pio) {
		ms->flags &= ~MS_REQ_USE_DMA;
	}


	if (ms->flags & MS_REQ_USE_DMA) {
		ms->dma_addr_map = dma_map_page(&ms->pdx->pci->dev,
							sg_page(ms->sg),
							ms->sg->offset,
							ms->sg->length,
						   (ms->data_direction & MS_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		ms->dma_addr_cur = ms->dma_addr_map;
		ms->sg_map = ms->sg;

		writel((u32)ms->dma_addr_cur, ms->ioaddr + MS_DMA_ADDRESS);
		TRACE1(("MAP: ms->sg: %p, ms->dma_addr_map: %x, sg->offset: %x, sg->length: %x, (u32)dma_addr_cur: %x",
			     ms->sg, (u32)ms->dma_addr_map, ms->sg->offset, ms->sg->length, (u32)ms->dma_addr_cur));

	}
	else {
		int flags;

		flags = SG_MITER_ATOMIC;
		if (ms->data_direction & MS_DATA_READ) {
			flags |= SG_MITER_TO_SG;
		}
		else {
			flags |= SG_MITER_FROM_SG;
		}
		sg_miter_start(&ms->sg_miter, ms->sg, ms->sg_cnt, flags);
	}
}

/*************************************************************************************************/
int ms_scsi_thread(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct ms_host *ms = pdx->ms;
	int err;

	TRACE1(("============================="));
	TRACE1(("ms_scsi_thread: %x, lba: 0x%02x, (%u), lba_len: (%u)", (srb->sc_data_direction == DMA_TO_DEVICE), lba, lba, lba_len));
	TRACE1(("sg: %p, sg_cnt: %x, scsi_bufflen: %x", scsi_sglist(srb), scsi_sg_count(srb), scsi_bufflen(srb)));

	// check card active
	if (pdx->current_card_active != MS_CARD_BIT) {
		ms_card_active_ctrl(ms, TRUE);
	}

	if (srb->sc_data_direction == DMA_TO_DEVICE) {
		ms->data_direction = MS_DATA_WRITE;
	}
	else {
		ms->data_direction = MS_DATA_READ;
	}

	ms->sg     = scsi_sglist(srb);
	ms->sg_cnt = scsi_sg_count(srb);

	if (ms->mcard.bMsType & MEMORY_STICK_PRO_GROUP) {
		ms_mdl_prepare_data(ms, 0);
		if (srb->sc_data_direction == DMA_TO_DEVICE) {
			err = ms_pro_scsi_write(srb, pdx, lba, lba_len);
		}
		else {
			err = ms_pro_scsi_read(srb, pdx, lba, lba_len);
		}
	}
	else {
		ms_mdl_prepare_data(ms, 1);
		if (srb->sc_data_direction == DMA_TO_DEVICE) {
			err = ms_scsi_write(srb, pdx, lba, lba_len);
		}
		else {
			err = ms_scsi_read(srb, pdx, lba, lba_len);
		}
	}

	if(ms->sg_map) {
		TRACE1(("unmap_page ::: ms->dma_addr_map: %x, sg_map->length: %x", (u32)ms->dma_addr_map, ms->sg_map->length));
		dma_unmap_page(&ms->pdx->pci->dev,
						ms->dma_addr_map,
						ms->sg_map->length,
						(ms->data_direction & MS_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
		ms->sg_map = 0;
	}

	if (err) {
		ms->scsi_io_err_cnt++;
		TRACEW(("scsi_io_err_cnt: %x, -----------------", ms->scsi_io_err_cnt));
	}

	return err;
}

/*************************************************************************************************/
int ms_scsi_read(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct ms_host      *ms         = pdx->ms;
	struct ms_card		*mcard		= &ms->mcard;
	struct ms_page		page;
	int		err;
	u32 remain_cnt, xfered_cnt;

	TRACE1(("==============================================================="));
	TRACE1(("ms_scsi_read ===>"));

	memset(&page, 0, sizeof(struct ms_page));

	page.bPageAdr = (u8)(lba & (mcard->bBlockSize - 1));
	page.wLogicalAdr = (u16)(lba / mcard->bBlockSize);

	TRACE1(("wLogicalAdr: %x, bPageAdr: %x", page.wLogicalAdr, page.bPageAdr));
	msi_log2phy_block(ms, &page);
	TRACE1(("wBlockAdr: %x", page.wBlockAdr));
	page.bCmdParameter = DATA_AND_EXTRA_DATA_BY_PAGE;

	page.buf_type = BUF_IS_MDL;

	xfered_cnt = 0;
	remain_cnt = lba_len * 0x200;
	while (remain_cnt) {


		err = msi_read_page_data(ms, &page);
		if (err) {
			TRACEX(("msi_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			ms_dumpregs(ms);
			return err;
		}

		/*
		if (ms->bMsErrorType & MS_READ_PROTECTED) {
			u8 *pbuf;
			TRACE1(("ms->bMsErrorType & MS_READ_PROTECTED, ......................"));
			if (scsi_io->buf_type == BUF_NOT_MDL) {
				pbuf = page.buf;
			}
			else {
				pbuf = (u8*)MmGetSystemAddressForMdlSafe(scsi_io->mdl, HighPagePriority);
				pbuf += scsi_io->xfered_cnt;
			}
			memset(pbuf, 0xff, MS_PAGE_SIZE);
		}
		*/

		xfered_cnt += MS_PAGE_SIZE;
		remain_cnt -= MS_PAGE_SIZE;
		if (remain_cnt == 0) {
			return ERR_SUCCESS;
		}

		page.bPageAdr++;
		if (page.bPageAdr == mcard->bBlockSize) {
			page.bPageAdr = 0;
			page.wLogicalAdr++;

			msi_log2phy_block(ms, &page);
		}
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_scsi_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct ms_host      *ms         = pdx->ms;
	struct ms_card		*mcard		= &ms->mcard;
	struct ms_page		rd_page;
	struct ms_page		wr_page;
	int	err, copypage_err;
	u8 bpage, i;
	u32 remain_cnt, xfered_cnt;


	TRACE1(("==============================================================="));
	TRACE1(("ms_scsi_write ===>, lba: %x, lba_len: %x", lba, lba_len));

	memset(&rd_page, 0, sizeof(struct ms_page));
	memset(&wr_page, 0, sizeof(struct ms_page));

	bpage	= (u8)(lba & (mcard->bBlockSize - 1));
	wr_page.wLogicalAdr	= rd_page.wLogicalAdr	= (u16)(lba / mcard->bBlockSize);

	rd_page.buf_type		= wr_page.buf_type		= BUF_IS_MDL;
	rd_page.bCmdParameter	= wr_page.bCmdParameter = DATA_AND_EXTRA_DATA_BY_PAGE;

	TRACE1(("wLogicalAdr: %x, bPageAdr: %x", rd_page.wLogicalAdr, bpage));
	msi_log2phy_block(ms, &rd_page);
	TRACE1(("wBlockAdr: %x", rd_page.wBlockAdr));

	err = msi_evenly_find_freeblock(ms, &wr_page);
	if (err) {
		TRACEX(("msi_evenly_find_freeblock: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto error_exit;
	}

	TRACE1(("wr_page.wBlockAdr: %x", wr_page.wBlockAdr));

	err = msi_overwrite_extra_data(ms, &rd_page, UPDATE_STATUS_MASK_DATA);
	if (err) {
		TRACEX(("msi_overwrite_extra_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto error_exit;
	}

	copypage_err = FALSE;
	for (i=0; i < bpage; i++) {
		rd_page.bPageAdr = wr_page.bPageAdr = i;
		err = msi_copy_page(ms, &rd_page, &wr_page);
		if (err) {
			TRACEX(("msi_copy_page: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			copypage_err = TRUE;
		}
	}

	TRACE1(("copypage_err: %x", copypage_err));

	xfered_cnt = 0;
	remain_cnt = lba_len * 0x200;
	while (remain_cnt) {

		rd_page.bPageAdr = wr_page.bPageAdr = bpage;
		err = msi_write_page_data(ms, &wr_page);
		if (err) {
			TRACEX(("msi_write_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto error_exit;
		}

		xfered_cnt += MS_PAGE_SIZE;
		remain_cnt -= MS_PAGE_SIZE;

		bpage++;
		if (bpage == mcard->bBlockSize) {

			bpage = 0;
			msi_store_block_to_table(ms, &wr_page);

			if (copypage_err == FALSE) {
				err = msi_erase_block_and_put_to_table(ms, &rd_page);
				if (err) {
					TRACEX(("msi_erase_block_and_put_to_table: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
					goto error_exit;
				}
			}
			copypage_err = FALSE;

			rd_page.bPageAdr = wr_page.bPageAdr = 0;
			rd_page.wLogicalAdr++;
			wr_page.wLogicalAdr++;

			msi_log2phy_block(ms, &rd_page);

			err = msi_evenly_find_freeblock(ms, &wr_page);
			if (err) {
				TRACEX(("msi_evenly_find_freeblock: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto error_exit;
			}

			err = msi_overwrite_extra_data(ms, &rd_page, UPDATE_STATUS_MASK_DATA);
			if (err) {
				TRACEX(("msi_overwrite_extra_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
				goto error_exit;
			}
		}
	}

	for (i=bpage; i < mcard->bBlockSize; i++) {
		rd_page.bPageAdr = wr_page.bPageAdr = i;
		err = msi_copy_page(ms, &rd_page, &wr_page);
		if (err) {
			TRACEX(("msi_copy_page: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			copypage_err = TRUE;
		}
	}

	msi_store_block_to_table(ms, &wr_page);

	if (copypage_err == FALSE) {
		err = msi_erase_block_and_put_to_table(ms, &rd_page);
		if (err) {
			TRACEX(("msi_erase_block_and_put_to_table: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto error_exit;
		}
	}

	return ERR_SUCCESS;

error_exit:

	ms_dumpregs(ms);
	return err;
}

/*************************************************************************************************/
int ms_pro_scsi_read(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct ms_host      *ms         = pdx->ms;
	struct ms_page		page;
	int	err;
	u8	buf[7];
	u32 xfered_cnt, remain_cnt;

	TRACE1(("==============================================================="));
	TRACE1(("ms_pro_scsi_read ===>"));

	buf[0] = MSPRO_READ_DATA;
	buf[1] = (u8)(lba_len >> 8);
	buf[2] = (u8)(lba_len);
	buf[3] = (u8)(lba >> 24);
	buf[4] = (u8)(lba >> 16);
	buf[5] = (u8)(lba >>  8);
	buf[6] = (u8)(lba);

	err = msi_pro_set_cmd_parameter(ms, buf);
	if (err) {
		TRACEX(("msi_pro_set_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto error_exit;
	}

	memset(&page, 0, sizeof(struct ms_page));
	page.get_int	= TRUE;
	page.buf_type	= BUF_IS_MDL;

	xfered_cnt = 0;
	remain_cnt = lba_len * MS_PAGE_SIZE;

	/* DMA mode transfer */
	if (ms->flags & MS_REQ_USE_DMA) {
		TRACE1(("blk_cnt: %x", lba_len));
		page.blk_cnt = (u16)lba_len;
		err = ms_tpc_read_page_data(ms, &page);
		if (err) {
			TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}
		xfered_cnt = lba_len * MS_PAGE_SIZE;
		remain_cnt = 0;

		if (ms->int_resp != MS_PRO_CED) {
			TRACEX(("ms->int_resp != MS_PRO_CED, %x, XXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
 		}
		TRACE1(("ms_pro_scsi_read <==="));
		return ERR_SUCCESS;
	}

	/* PIO mode transfer */
	while (remain_cnt) {

		if (!(ms->int_resp & MS_PRO_BREQ)) {
			TRACEX(("ms->int_resp & MS_PRO_BREQ == 0x00, %x, XXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}

		err = ms_tpc_read_page_data(ms, &page);
		if (err) {
			TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}

		xfered_cnt += MS_PAGE_SIZE;
		remain_cnt -= MS_PAGE_SIZE;
	}


 	if (ms->int_resp != MS_PRO_CED) {
 		TRACEX(("ms->int_resp != MS_PRO_CED, %x, XXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
 		goto error_exit;
 	}

	return ERR_SUCCESS;

error_exit:

	TRACEX(("Read lba: %x, lba_len: %x", lba, lba_len));
	ms_dumpregs(ms);

	if (ms->int_resp & MS_PRO_BREQ) {
		TRACEX(("ms->int_resp & MS_PRO_BREQ == TRUE: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		msi_pro_stop(ms);
	}
	return ERR_ERROR;
}

/*************************************************************************************************/
int ms_pro_scsi_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len)
{
	struct ms_host      *ms         = pdx->ms;
	struct ms_page		page;
	int		err;
	u8	buf[7];
	u32 xfered_cnt, remain_cnt;

	TRACE1(("==============================================================="));
	TRACE1(("ms_pro_scsi_write ===>"));

	buf[0] = MSPRO_WRITE_DATA;
	buf[1] = (u8)(lba_len >> 8);
	buf[2] = (u8)(lba_len);
	buf[3] = (u8)(lba >> 24);
	buf[4] = (u8)(lba >> 16);
	buf[5] = (u8)(lba >>  8);
	buf[6] = (u8)(lba);

	err = msi_pro_set_cmd_parameter(ms, buf);
	if (err) {
		TRACEX(("msi_pro_set_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		goto error_exit;
	}

	memset(&page, 0, sizeof(struct ms_page));
	page.get_int	= TRUE;
	page.buf_type	= BUF_IS_MDL;


	xfered_cnt = 0;
	remain_cnt = lba_len * MS_PAGE_SIZE;

	/* DMA write operation */
	if (ms->flags & MS_REQ_USE_DMA) {
		TRACE1(("blk_cnt: %x", lba_len));
		page.blk_cnt = (u16)lba_len;
		err = ms_tpc_write_page_data(ms, &page);
		if (err) {
			TRACEX(("ms_tpc_write_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}
		xfered_cnt = lba_len * MS_PAGE_SIZE;
		remain_cnt = 0;
		TRACE1(("ms_pro_scsi_write <==="));
		return ERR_SUCCESS;
	}

	/* PIO write operation */
	while (remain_cnt) {

		if (!(ms->int_resp & MS_PRO_BREQ)) {
			TRACEX(("ms->int_resp & MS_PRO_BREQ == 0x00, %x, XXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}

		err = ms_tpc_write_page_data(ms, &page);
		if (err) {
			TRACEX(("ms_tpc_write_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
			goto error_exit;
		}

		xfered_cnt += MS_PAGE_SIZE;
		remain_cnt -= MS_PAGE_SIZE;
	}

	if (ms->int_resp != MS_PRO_CED) {
		TRACEX(("ms->int_resp != MS_PRO_CED, %x, XXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		goto error_exit;
	}

	return ERR_SUCCESS;

error_exit:

	TRACEX(("Write lba: %x, lba_len: %x", lba, lba_len));
	ms_dumpregs(ms);

	if (ms->int_resp & MS_PRO_BREQ) {
		TRACEX(("ms->int_resp & MS_PRO_BREQ == TRUE: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		msi_pro_stop(ms);
	}
	return ERR_ERROR;
}

/*************************************************************************************************/
void __________ms_irq__________(void)
{

}

/*************************************************************************************************/
void ms_data_pio_read(struct ms_host *ms)
{
	struct ms_tpc *tpc = ms->tpc;
	u8	*pbuf;
	int	buf_len, cnt;
	u32	data;

	if (!tpc) {
		TRACEX(("tpc == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	pbuf	= tpc->buf;
	buf_len = tpc->buf_len;

	TRACE(("MS_INT_BUF_READ_RDY ..."));


	if (tpc->buf_type == BUF_IS_MDL) {
		unsigned long flags;
		u8 *buf;
		u32 scratch, chunk, len;

		chunk = 0;
		scratch = 0;
		local_irq_save(flags);

		while (buf_len) {
			if (!sg_miter_next(&ms->sg_miter))
				BUG();

			len = (u32)min(ms->sg_miter.length, (size_t)buf_len);

			buf_len -= len;
			ms->sg_miter.consumed = len;

			buf = ms->sg_miter.addr;

			while (len) {
				if (chunk == 0) {
					scratch = readl(ms->ioaddr + MS_BUFFER_PORT);
					chunk = 4;
				}

				*buf = scratch & 0xFF;

				buf++;
				scratch >>= 8;
				chunk--;
				len--;
			}
		}

		sg_miter_stop(&ms->sg_miter);

		local_irq_restore(flags);
	}
	else {

		pbuf = (char*)tpc->buf;

		while (buf_len) {

			data = readl(ms->ioaddr + MS_BUFFER_PORT);
			cnt = min(buf_len, 4);
			buf_len -= cnt;

			while(cnt) {
				*pbuf = (u8)(data & 0xff);
				pbuf++;
				data >>= 8;
				cnt--;
			}
		}
	}

}

/*************************************************************************************************/
void ms_data_pio_write(struct ms_host *ms)
{
	struct ms_tpc *tpc = ms->tpc;
	u8	*pbuf;
	int	buf_len;
	u32	data, i;
	u32 cnt;

	TRACE(("MS_INT_BUF_WRITE_RDY ..."));

	if (!tpc) {
		TRACEX(("tpc == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}


	pbuf	= tpc->buf;
	buf_len = tpc->buf_len;


	if (tpc->buf_type == BUF_IS_MDL) {
		unsigned long flags;
		size_t chunk;
		u32 scratch, len;
		u8 *buf;

		chunk = 0;
		scratch = 0;

		local_irq_save(flags);

		while (buf_len) {
			if (!sg_miter_next(&ms->sg_miter))
				BUG();

			len = (u32)min(ms->sg_miter.length, (size_t)buf_len);

			buf_len -= len;
			ms->sg_miter.consumed = len;

			buf = ms->sg_miter.addr;

			while (len) {
				scratch |= (u32)*buf << (chunk * 8);

				buf++;
				chunk++;
				len--;

				if ((chunk == 4) || ((len == 0) && (buf_len == 0))) {
					writel(scratch, ms->ioaddr + MS_BUFFER_PORT);
					chunk = 0;
					scratch = 0;
				}
			}
		}

		sg_miter_stop(&ms->sg_miter);

		local_irq_restore(flags);
	}
	else {
		while(buf_len) {

			cnt = min(buf_len, 4);
			buf_len -= cnt;

			data = 0;
			for (i=0; i < cnt; i++) {
				data |= ((u32)*pbuf << (i*8));
				pbuf++;
			}
			writel(data, ms->ioaddr + MS_BUFFER_PORT);
		}
	}


}

/*************************************************************************************************/
void ms_irq_data(struct ms_host	*ms, u32 intmask)
{
	struct ms_tpc	*tpc = ms->tpc;


	if (!tpc) {
		TRACEX(("ms_irq_data, ms->tpc == NULL, intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		ms_dumpregs(ms);
		return;
	}

	// pio transfer
	if (intmask & (MS_INT_BUF_READ_RDY | MS_INT_BUF_WRITE_RDY)) {
		if (intmask & MS_INT_BUF_READ_RDY) {
			/* PIO read */
			ms_data_pio_read(ms);
		}
		else {
			/* PIO write */
			ms_data_pio_write(ms);
		}
	}

	// dma transfer
	if (intmask & MS_INT_DMA_END) {
		/* DMA transfer */
		if (ms->dma_addr_map == ms->dma_addr_cur) {
			ms->dma_addr_cur += (DMA_UNIT_SIZE - (ms->dma_addr_map & (DMA_UNIT_SIZE-1)));
		}
		else {
			ms->dma_addr_cur += DMA_UNIT_SIZE;
		}
		TRACE1(("dma_addr_cur: %x, dma_addr_map: %x, sg->offset: %x, sg->length: %x", (u32)ms->dma_addr_cur, (u32)ms->dma_addr_map, ms->sg->offset, ms->sg->length));
		if (ms->dma_addr_cur >= (ms->dma_addr_map + ms->sg->length)) {
			TRACE1(("dma_addr_cur: %x, dma_addr_map: %x, sg->length: %x", (u32)ms->dma_addr_cur, (u32)ms->dma_addr_map, ms->sg->length));
			dma_unmap_page(&ms->pdx->pci->dev,
							ms->dma_addr_map,
							ms->sg_map->length,
							(ms->data_direction & MS_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
			ms->dma_addr_map = 0;
			ms->sg_map = 0;

			ms->sg = sg_next(ms->sg);
			TRACE1(("next sg: %p", ms->sg));
			if (ms->sg) {
				ms->dma_addr_map = dma_map_page(&ms->pdx->pci->dev,
								sg_page(ms->sg),
								ms->sg->offset,
								ms->sg->length,
							   (ms->data_direction & MS_DATA_READ) ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
				ms->dma_addr_cur = ms->dma_addr_map;
				ms->sg_map = ms->sg;

				TRACE1(("MAP: ms->sg: %p, ms->dma_addr_map: %x, sg->offset: %x, sg->length: %x",
					ms->sg, (u32)ms->dma_addr_map, ms->sg->offset, ms->sg->length));
				writel((u32)ms->dma_addr_cur, ms->ioaddr + MS_DMA_ADDRESS);
			}
		}
		else {
			TRACE(("ms->dma_addr_cur: %x", (u32)ms->dma_addr_cur));
			writel((u32)ms->dma_addr_cur, ms->ioaddr + MS_DMA_ADDRESS);
		}
	}

}

/*************************************************************************************************/
irqreturn_t ms_irq(struct _DEVICE_EXTENSION *pdx)
{
	struct ms_host *ms = pdx->ms;
	u32	intmask;

	///////////////////////////////////////////////////////////////////////////

	if(!ms) {
		TRACEX(("ms == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return IRQ_NONE;
	}

	intmask = readl(ms->ioaddr + MS_INT_STATUS);
	if (!intmask || intmask == 0xffffffff) {
		//TRACE(("intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		return IRQ_NONE;
	}

	TRACE1(("ms_irq ===>, intmask: %x", intmask));
	if (intmask & (MS_INT_CARD_INSERT | MS_INT_CARD_REMOVE)) {
		TRACEW(("CARD_INSERT, CARD_REMOVE, intmask: %x", intmask));
		if (intmask & MS_INT_CARD_REMOVE) {
			ms->card_inserted = 0;
		}
		else {
			ms->card_inserted = 1;
		}
		writel(intmask & (MS_INT_CARD_INSERT | MS_INT_CARD_REMOVE),	ms->ioaddr + MS_INT_STATUS);
		tasklet_schedule(&ms->card_tasklet);
		intmask &= ~(MS_INT_CARD_INSERT | MS_INT_CARD_REMOVE);
	}

	if (intmask & MS_INT_DATA_MASK) {
		TRACE(("intmask & MS_INT_DATA_MASK ..."));
		writel(intmask & MS_INT_DATA_MASK, ms->ioaddr + MS_INT_STATUS);
		ms_irq_data(ms, intmask & MS_INT_DATA_MASK);
		intmask &= ~(MS_INT_DATA_MASK);
	}

	if (intmask & MS_INT_TPC_MASK) {
		writel(intmask & MS_INT_TPC_MASK, ms->ioaddr + MS_INT_STATUS);

		if (intmask & MS_INT_TPC_ERROR) {
			TRACEX(("intmask & (MS_INT_TPC_ERROR), %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
			if (ms->tpc) {
				ms->tpc->error |= (intmask & MS_INT_TPC_ERROR);
			}
			else {
				TRACEX(("ms->tpc == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
				ms_dumpregs(ms);
			}
		}

		tasklet_schedule(&ms->finish_tasklet);
		intmask &= ~MS_INT_TPC_MASK;
	}


	if (intmask & MS_INT_OVER_CURRENT_ERROR) {
		TRACEX(("Card is consuming too much power, intmask: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		writel(MS_INT_OVER_CURRENT_ERROR, ms->ioaddr + MS_INT_STATUS);
		intmask &= ~MS_INT_OVER_CURRENT_ERROR;
		if (pdx->uExtEnOverCurrent) {
			ms->mcard.over_current = pdx->media_card.over_current = (OC_IS_OVER_CURRENT | OC_POWER_IS_ON);
		}
	}

	if (intmask) {
		TRACEX(("Unexpected interrupt: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", intmask));
		writel(intmask, ms->ioaddr + MS_INT_STATUS);
	}

	return IRQ_HANDLED;
}

/*************************************************************************************************/
void ms_tasklet_finish(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct ms_host		*ms		= pdx->ms;
	struct ms_request	*srq	= ms->srq;

	TRACE(("==============================================="));
	TRACE(("ms_tasklet_finish ===>"));

	//KeCancelTimer(&ms->timeout_timer);
	del_timer(&ms->timeout_timer);

	ms->srq = NULL;
	ms->tpc = NULL;

	if (srq) {

		if (srq->done) {
			srq->done(srq);
		}
	}
	else {
		TRACEX(("srq == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
	}
}

/*************************************************************************************************/
void ms_tasklet_card(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct ms_host *ms = pdx->ms;

	TRACEW(("==============================================="));
	TRACEW(("ms_tasklet_card ===>"));


	if (ms->card_inserted) {
		TRACEW(("MS card is inserted, OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"));
		pdx->card_init_flag |= MS_CARD_TYPE;
		//KeSetEvent(&pdx->CardInitEvent, 0, FALSE);
		//KeSetEvent(&pdx->LedCtrlEvent, 0, FALSE);
		complete(&pdx->card_ready);
		return;
	}

	TRACEW(("Ms card is removed, =========================="));
	ms_update_media_card(ms);

	if (ms->srq) {
		TRACEX(("Card removed during transfer, Resetting controller, XXXXXXXXXXXXXXXXXXXX"));

		tasklet_schedule(&ms->finish_tasklet);
	}


	ms_reset(ms);

	return;
}

/*************************************************************************************************/
void ms_tasklet_timeout(unsigned long parm)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)parm;
	struct ms_host *ms = pdx->ms;

	TRACEW(("ms_tasklet_timeout ===>"));
	ms_dumpregs(ms);

	if (!ms) {
		TRACEX(("ms == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return;
	}

	ms_reset(ms);

	if (ms->srq) {
		if (ms->tpc) {
			ms->tpc->error |= MS_INT_TPC_TIMEOUT;
		}
		else {
			TRACEX(("ms->tpc == NULL, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		}

		tasklet_schedule(&ms->finish_tasklet);
	}

	return;
}

/*************************************************************************************************/
void ms_set_timer(struct ms_host *ms)
{
	//LARGE_INTEGER		uuTime;
	//uuTime.QuadPart = (LONGLONG)-10000000 * 10; /* unit: sec */
	//KeSetTimerEx(&ms->timeout_timer, uuTime, 0, &ms->timeout_tasklet);

	mod_timer(&ms->timeout_timer, jiffies + 10 * HZ);

}

/*************************************************************************************************/
void __________ms_low_level__________(void)
{

}

/*************************************************************************************************/
int ms_card_active_ctrl(struct ms_host *ms, u8 active)
{
	u8 flag;

	TRACE1(("ms_card_active_ctrl ===>, active: %x", active));

	if (active) {
		flag = MS_CARD_BIT;
	}
	else {
		flag = 0;
	}

	card_active_ctrl(ms->pdx, flag);
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_reset(struct ms_host	*ms)
{
	int	err;

	TRACEW(("============================================="));
	TRACEW(("ms_reset ===>"));

	err = card_reset(ms->pdx, MS_CARD_BIT);
	if (err) {
		TRACEX(("card_reset, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return err;
	}

	ms_init_int(ms);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_init_int(struct ms_host *ms)
{
	u32	intmask;

	TRACEW(("============================================="));
	TRACEW(("ms_init_int ===>"));

	ms_card_active_ctrl(ms, TRUE);

	intmask =	MS_INT_TPC_END | MS_INT_DMA_END | MS_INT_BUF_READ_RDY |
		MS_INT_BUF_WRITE_RDY | MS_INT_CARD_INSERT | MS_INT_CARD_REMOVE |
		MS_INT_TPC_TIMEOUT | MS_INT_CED_ERROR | MS_INT_INT_RESP_ERROR | MS_INT_INT_TIMEOUT | MS_INT_DATA_CRC_ERROR;

	writel(intmask, ms->ioaddr + MS_INT_ENABLE);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_power_on(struct ms_host	*ms)
{
	TRACEW(("ms_power_on ===>"));

	card_power_ctrl(ms->pdx, MS_CARD_BIT, TRUE);

	sys_delay(ms->uMsExtPwrOnDelayTime);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_power_off(struct ms_host	*ms)
{

	TRACEW(("ms_power_off ===>"));

	card_power_ctrl(ms->pdx, MS_CARD_BIT, FALSE);

	sys_delay(ms->uMsExtPwrOffDelayTime);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_set_clock(struct ms_host *ms, u32 clock)
{

	TRACEW(("ms_set_clock ===>, %x", clock));

	card_set_clock(ms->pdx, clock);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_dumpregs(struct ms_host *ms)
{
	TRACEY(("=================================================================="));
	TRACEY(("CARD_DMA_ADDRESS   (00): %08x", readl(ms->ioaddr + CARD_DMA_ADDRESS)));
	TRACEY(("CARD_CLK_SELECT    (72): %04x", readw(ms->ioaddr + CARD_CLK_SELECT)));
	TRACEY(("CARD_ACTIVE_CTRL   (75): %02x", readb(ms->ioaddr + CARD_ACTIVE_CTRL)));
	TRACEY(("CARD_POWER_CTRL    (70): %02x", readb(ms->ioaddr + CARD_POWER_CONTROL)));
	TRACEY(("CARD_OUTPUT_ENABLE (7a): %02x", readb(ms->ioaddr + CARD_OUTPUT_ENABLE)));
	TRACEY(("CARD_XFER_LENGTH   (6c): %08x", readl(ms->ioaddr + CARD_XFER_LENGTH)));

	TRACEY(("MS_STATUS          (a0): %08x", readl(ms->ioaddr + MS_STATUS)));
	TRACEY(("MS_BUS_MODE_CTRL   (a1): %02x", readb(ms->ioaddr + MS_BUS_MODE_CTRL)));
	TRACEY(("MS_TPC_CTRL        (a2): %02x", readb(ms->ioaddr + MS_TPC_CMD)));
	TRACEY(("MS_TRANSFER_MODE   (a3): %02x", readb(ms->ioaddr + MS_TRANSFER_MODE)));
	TRACEY(("MS_DATA_PIN_STATE  (a4): %02x", readb(ms->ioaddr + MS_DATA_PIN_STATE)));

	TRACEY(("MS_INT_STATUS      (b0): %08x", readl(ms->ioaddr + MS_INT_STATUS)));
	TRACEY(("MS_INT_ENABLE      (b4): %08x", readl(ms->ioaddr + MS_INT_ENABLE)));
	TRACEY(("=================================================================="));

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void __________ms_tpc__________(void)
{

}

/*************************************************************************************************/
void ms_send_tpc(struct ms_host *ms, struct ms_tpc *tpc)
{
	u32 xfer_length;
	u8	xfer_mode;

	TRACE(("ms_send_tpc ===>"));

	if (tpc->blk_cnt) {
		xfer_length = tpc->blk_cnt * tpc->buf_len;
	}
	else {
		xfer_length = tpc->buf_len;
	}

	TRACE1(("MS_XFER_LENGTH: %x", xfer_length));
	writel(xfer_length,	ms->ioaddr + MS_XFER_LENGTH);

	xfer_mode = MS_XFER_START;

	if (ms->flags & MS_REQ_USE_DMA) {
		if (tpc->buf_type == BUF_IS_MDL) {
			xfer_mode |= MS_XFER_DMA_ENABLE;
		}
	}

	if (tpc->get_int) {
		xfer_mode |= MS_XFER_INT_TIMEOUT_CHK;
	}

	writeb(tpc->tpc_ctrl, ms->ioaddr + MS_TPC_CMD);
	writeb(xfer_mode, ms->ioaddr + MS_TRANSFER_MODE);
}

/*************************************************************************************************/
void ms_wait_for_tpc_done(struct ms_request *srq)
{
	if (srq->done_data) {
		/*KeSetEvent(srq->done_data, 0, FALSE);*/
		complete(srq->done_data);
	}
}

/*************************************************************************************************/
void ms_wait_for_tpc(struct ms_host *ms, struct ms_tpc *tpc)
{
	struct ms_request	srq;
	/*KEVENT				event;*/
	DECLARE_COMPLETION_ONSTACK(complete);

	TRACE(("ms_wait_for_tpc ===>"));

	if (!ms->card_inserted) {
		TRACEW(("Card is removed, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		tpc->error = ERR_ERROR;
		return;
	}

	ms_set_timer(ms);

	memset(&srq, 0, sizeof(struct ms_request));

	/*KeInitializeEvent(&event, NotificationEvent, FALSE);*/

	srq.tpc			= tpc;

	ms->srq			= &srq;
	ms->tpc			= tpc;

	srq.done_data	= &complete;
	srq.done		= ms_wait_for_tpc_done;

	ms_send_tpc(ms, tpc);

	/*KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);*/
	wait_for_completion(&complete);

	TRACE(("ms_wait_for_tpc <==="));
}

/*************************************************************************************************/
/* TPC Write operation */
/*************************************************************************************************/

/*************************************************************************************************/
int ms_tpc_write(struct ms_host *ms, u8 *pbuf, u8 buf_len, u8 tpc_ctrl)
{
	struct ms_tpc	tpc;
	int				err;

	TRACE(("ms_tpc_write ===>"));

	memset(&tpc, 0x00, sizeof(struct ms_tpc));

	if (tpc_ctrl == MS_TPC_SET_CMD) {
		u8 cmd_op = *pbuf;
		/* MS_RESET: No INT signal is generated at completion */
		if (cmd_op != MS_RESET) {
			tpc.get_int = TRUE;
		}
	}

	if (tpc_ctrl == MS_TPC_EX_SET_CMD) {
		tpc.get_int = TRUE;
	}


	tpc.buf			= pbuf;
	tpc.buf_type	= BUF_NOT_MDL;
	tpc.buf_len		= buf_len;

	tpc.tpc_ctrl	= tpc_ctrl;
	ms_wait_for_tpc(ms, &tpc);
	if (tpc.error) {
		TRACEX(("mshc_wait_for_tpc: %x,XXXXXXXXXXXXXXXXXXXXXXXX", tpc.error));
		return tpc.error;
	}

	if (tpc.get_int) {
		err = ms_tpc_get_int(ms);
		if (err) {
			TRACEX(("ms_tpc_get_int: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}
		return err;
	}

	return tpc.error;
}

/*************************************************************************************************/
int ms_tpc_write_reg(struct ms_host *ms, u8 *pbuf, u8 buf_len)
{
	int err;

	TRACE(("ms_tpc_write_reg ===>"));

	err = ms_tpc_write(ms, pbuf, buf_len, MS_TPC_WRITE_REG);
	if (err) {
		TRACEX(("ms_tpc_write: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int ms_tpc_set_rw_reg_adrs(struct ms_host *ms, u8 *pbuf)
{
	int err;

	TRACE(("ms_tpc_set_rdwr_reg_adrs ===>"));

	err = ms_tpc_write(ms, pbuf, 0x04, MS_TPC_SET_RW_REG_ADRS);
	if (err) {
		TRACEX(("ms_tpc_write: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int ms_tpc_set_cmd(struct ms_host *ms, u8 cmd_op, u8 chk_breq, struct ms_page *page)
{
	int err;

	TRACE(("ms_tpc_set_cmd ===>"));

	err = ms_tpc_write(ms, &cmd_op, 1, MS_TPC_SET_CMD);
	if (err) {
		TRACEX(("ms_tpc_write: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/*===================================================*/
	if (cmd_op == MS_RESET) {
		/* MS_RESET: No INT signal is generated at completion */
		return ERR_SUCCESS;
	}


	/*===========================================================*/
	/* MS_FLASH_STOP, MS_BLOCK_ERASE, MS_CLEAR_BUF, MS_BLOCK_END */
	/*===========================================================*/
	if ((cmd_op == MS_FLASH_STOP) || (cmd_op == MS_BLOCK_ERASE) ||
		(cmd_op == MS_CLEAR_BUF)  || (cmd_op == MS_BLOCK_END) ||
		(chk_breq == FALSE)) {

		if (ms->int_resp & MS_PRO_ERR_CMDNK) {
			TRACEX(("ms->int_resp & MS_PRO_CMDNK: %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
			return ERR_ERROR;
		}
		return ERR_SUCCESS;
	}

	/*===================================================*/
	/* Send MS_BLOCK_READ and wait interrupt			 */
	/*===================================================*/
	if (cmd_op == MS_BLOCK_READ) {
		/* MS Pro */
		TRACE(("MS_BLOCK_READ, int_resp: %x", ms->int_resp));

		if (ms->int_resp & MS_PRO_ERR_CMDNK) {
			TRACEX(("ms->int_resp & MS_PRO_ERR_CMDNK == TRUE, %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
			if (ms->int_resp & MS_PRO_CMDNK) {
				TRACEX(("ms->int_resp & MS_PRO_CMDNK: %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
				return ERR_ERROR;
			}
			else {
				err = msi_read_error_correctable(ms, page);
				if (err) {
					TRACEX(("msi_read_error_correctable: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
					return err;
				}
			}
		}

		if (chk_breq) {
			if (!(ms->int_resp & MS_PRO_BREQ)) {
				TRACEX(("ms->int_resp & MS_PRO_BREQ == 0, %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
				return ERR_ERROR;
			}
		}
		return ERR_SUCCESS;
	}

	/*===================================================*/
	/* Send MS_BLOCK_WRITE and wait interrupt			 */
	/*===================================================*/
	if (cmd_op == MS_BLOCK_WRITE) {
		if (ms->int_resp & MS_PRO_ERR_CMDNK) {
			TRACEX(("ms->int_resp & MS_PRO_CMDNK: %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
			return ERR_ERROR;
		}
		/*
 		if (!(ms->int_resp & MS_PRO_BREQ)) {
 			TRACEX(("(ms->int_resp & MS_PRO_BREQ) == FALSE: %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
 			return ERR_ERROR;
 		}
		*/
		return ERR_SUCCESS;
	}

	return err;
}

/*************************************************************************************************/
int ms_tpc_write_page_data(struct ms_host *ms, struct ms_page *page)
{
	struct ms_tpc	tpc;
	int				err;

	TRACE1(("=========================="));
	TRACE1(("ms_tpc_write_page_data ===>"));

	memset(&tpc, 0x00, sizeof(struct ms_tpc));
	tpc.blk_cnt		= page->blk_cnt;

	tpc.get_int		= TRUE;

	tpc.buf			= page->buf;
	tpc.buf_type	= page->buf_type;
	tpc.buf_len		= MS_PAGE_SIZE;

	tpc.tpc_ctrl	= MS_TPC_WRITE_PAGE_DATA;

	ms_wait_for_tpc(ms, &tpc);
	if (tpc.error) {
		TRACEX(("ms_wait_for_tpc: %x,XXXXXXXXXXXXXXXXXXXXXXXX", tpc.error));
		return tpc.error;
	}

	err = ms_tpc_get_int(ms);
	if (err) {
		TRACEX(("ms_tpc_get_int: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->int_resp & MS_PRO_ERR_CMDNK) {
		TRACEX(("ms->int_resp & MS_PRO_ERR_CMDNK == TRUE, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		if (ms->int_resp & MS_PRO_ERR) {
			page->bPageAdr = 0;
			if (!page->IsMsMgAccess) {
				msi_overwrite_extra_data(ms, page, BLOCK_STATUS_MASK_DATA);
			}
		}
		return ERR_ERROR;
	}
	return err;
}

/*************************************************************************************************/
/* TPC Read operation */
/*************************************************************************************************/

/*************************************************************************************************/
int ms_tpc_read(struct ms_host *ms, u8 buf_len, u8 tpc_ctrl)
{
	struct ms_tpc	tpc;

	TRACE(("ms_tpc_read ===>, tpc_ctrl: %x, buf_len: %x", tpc_ctrl, buf_len));

	memset(&tpc, 0x00, sizeof(struct ms_tpc));

	if (tpc_ctrl & MS_TPC_CTRL_GET_INT) {
		tpc.get_int	= TRUE;
	}
	tpc_ctrl	   &= 0x0f;

	tpc.buf			= &ms->resp[0];
	tpc.buf_type	= BUF_NOT_MDL;
	tpc.buf_len		= buf_len;

	tpc.tpc_ctrl	= tpc_ctrl;

	ms_wait_for_tpc(ms, &tpc);
	if (tpc.error) {
		TRACEX(("mshc_wait_for_tpc: %x,XXXXXXXXXXXXXXXXXXXXXXXX", tpc.error));
		return tpc.error;
	}

	return tpc.error;
}

/*************************************************************************************************/
int ms_tpc_read_reg(struct ms_host *ms, u8 buf_len)
{
	int err;

	TRACE(("ms_tpc_read_reg ===>, buf_len: %x", buf_len));

	err = ms_tpc_read(ms, buf_len, MS_TPC_READ_REG);
	if (err) {
		TRACEX(("ms_tpc_read: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int ms_tpc_get_int(struct ms_host *ms)
{
	int err = 0;

	TRACE(("ms_tpc_get_int ===>"));

	ms->int_resp = 0;

	if (ms->mcard.ms_bus_mode & MS_PRO_PARALLEL_MODE) {
		ms->int_resp = readb(ms->ioaddr + MS_DATA_PIN_STATE);
		TRACE(("ms->int_resp: %x", ms->int_resp));
	}
	else {
		/* Serial Mode */
		err = ms_tpc_read(ms, 1, MS_TPC_GET_INT);
		if (err) {
			TRACEX(("ms_tpc_read: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}
		TRACE(("int_resp, ms->resp[0]: %x", ms->resp[0]));
		/* sync to ms pro interrupt state */
		ms->int_resp = 0;
		if (ms->resp[0] & CMDNK) {
			ms->int_resp |= MS_PRO_CMDNK;
		}
		if (ms->resp[0] & BREQ) {
			ms->int_resp |= MS_PRO_BREQ;
		}
		if (ms->resp[0] & ERR) {
			ms->int_resp |= MS_PRO_ERR;
		}
		if (ms->resp[0] & CED) {
			ms->int_resp |= MS_PRO_CED;
		}
	}

	return err;
}

/*************************************************************************************************/
int ms_tpc_read_page_data(struct ms_host *ms, struct ms_page *page)
{
	struct ms_tpc	tpc;
	int	err;

	TRACE1(("=========================="));
	TRACE1(("ms_tpc_read_page_data ===>"));

	memset(&tpc, 0x00, sizeof(struct ms_tpc));
	tpc.blk_cnt		= page->blk_cnt;

	tpc.get_int		= page->get_int;

	tpc.buf			= page->buf;
	tpc.buf_type	= page->buf_type;
	tpc.buf_len		= MS_PAGE_SIZE;

	tpc.tpc_ctrl	= MS_TPC_READ_PAGE_DATA;

	ms_wait_for_tpc(ms, &tpc);
	if (tpc.error) {
		TRACEX(("mshc_wait_for_tpc: %x,XXXXXXXXXXXXXXXXXXXXXXXX", tpc.error));
		return tpc.error;
	}

	if (tpc.get_int) {
		err = ms_tpc_get_int(ms);
		if (err) {
			TRACEX(("ms_tpc_get_int: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}
		return err;
	}

	return tpc.error;
}

/*************************************************************************************************/
void __________ms_init_card__________(void)
{

}

/*************************************************************************************************/
int ms_init_ms_card(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	struct ms_page	page;
	int		err;
	u16	segment, max_seg;

	TRACEW(("==========================================="));
	TRACEW(("ms_init_ms_card ===>"));

	err = msi_reset(ms);
	if (err) {
		TRACEX(("msi_reset: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* Retrieval procedure for boot block */
	err = msi_get_boot_block(ms);
	if (err) {
		TRACEX(("msi_get_boot_block: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* Read page1 of boot block(disabled block data) to buffer */
	memset(&page, 0x00, sizeof(struct ms_page));
	page.get_int		= FALSE;
	page.wBlockAdr		= mcard->bBootBlockAdr0;
	page.bPageAdr		= 1;
	page.bCmdParameter	= DATA_AND_EXTRA_DATA_BY_PAGE;

	page.buf			= mcard->init_badblk;
	page.buf_type		= BUF_NOT_MDL;
	err = msi_read_boot_block_data(ms, &page);
	if (err) {
		TRACEX(("msi_read_boot_block_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/*
	for (i=0; i< MS_PAGE_SIZE; i++) {
		PRINT0(("%02x, ", mcard->init_badblk[i]));
		if ((i & 0xf) == 0xf) {
			PRINT0(("\n"));
		}
	}
	PRINT0(("\n"));
	*/

	/* Process boot area protection */
	if (mcard->bBootAreaProtectFlag & NEED_BOOT_AREA_PROTECT_PROCESS) {
		if (mcard->bMsType != MEMORY_STICK_ROM) {
			msi_bootarea_protection_process(ms);
		}
	}
	mcard->bBootAreaProtectFlag = 0;

	err = msi_make_trans_table(ms);
	if (err) {
		TRACEX(("msi_make_trans_table: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	max_seg = ms->mcard.wTotalBlockNum >> 9; /* 0x200 */
	for (segment=0; segment < max_seg; segment++) {
		err = msi_check_free_block_count(ms, segment);
		if (err) {
			TRACEX(("msi_check_free_block_count: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
			break;
		}
	}
	TRACEW(("ms_init_ms_card <==="));
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_init_mspro_card(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	int		err;
	int		i;

	TRACEW(("==========================================="));
	TRACEW(("ms_init_mspro_card ===>"));

	ms_set_clock(ms, ms->uMsExtPro1BitClock);

	/* Start-up delay. 10 sec in worst case */
	for (i=0; i < 100; i++) {
		if (!ms->card_inserted) {
			TRACEW(("No card is inserted ................."));
			return ERR_ERROR;
		}
		sys_delay(100);

		err = ms_tpc_read(ms, 1, MS_TPC_GET_INT);
		if (err) {
			TRACEX(("ms_tpc_read: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
			return ERR_ERROR;
		}

		if (ms->resp[0] & CED) {
			break;
		}
		else if (((ms->resp[0] & MS_INT) == CED_ERR) || ((ms->resp[0] & MS_INT) == ERR_CMDNK_CED)) {
			return ERR_ERROR;
		}
	}

	if (i == 100) {
		TRACEX(("MS Pro initial timeout, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	err = ms_tpc_read(ms, 1, MS_TPC_GET_INT);
	if (err) {
		TRACEX(("ms_tpc_read: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->resp[0] & ERR) {
		if (ms->resp[0] & CMDNK) {
			mcard->bMsType	= MEMORY_STICK_PRO_ROM;
			mcard->state   |= MS_STATE_HW_READONLY;
		}
		else {
			TRACEX(("Media Error: %x,XXXXXXXXXXXXXXXXXXXXXXXX", ms->resp[0]));
			return ERR_ERROR;
		}
	}

	if (ms->uMsExtProForce1Bit == FALSE) {
		/* Switch to parallel 4-bit mode */
		err = msi_set_if_mode(ms, MS_PRO_PARALLEL_4BIT_IF);
		if (err) {
			TRACEX(("msi_set_if_mode: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}

		ms_set_clock(ms, ms->uMsExtPro4BitClock);
	}

	if (ms->uMsExtProForce1Bit) {
		/* Force to 1-bit mode, don't switch to 8 bit mode */
		TRACEW(("ms->uMsExtProForce1Bit == TRUE, Don't switch to 8 bit mode................."));
		goto exit_pro_get_sys_info;
	}

	if (ms->mcard.bMsType & MEMORY_STICK_PRO_HG_GROUP) {
		TRACEW(("MEMORY_STICK_PRO_HG_GROUP ......"));

		if (ms->uMsExtProNo8Bit || mcard->bMsProInit8BitErr ) {
			TRACEW(("uMsExtProNo8Bit: %x, bMsProInit8BitErr: %x, Don't switch to 8 bit mode, .......", ms->uMsExtProNo8Bit, mcard->bMsProInit8BitErr));
			goto exit_pro_get_sys_info;
		}

		/* Switch to parallel 8 Bit I/F mode */
		err = msi_set_if_mode(ms, MS_PRO_PARALLEL_8BIT_IF);
		if (err) {
			TRACEX(("msi_set_if_mode: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}

		ms_set_clock(ms, ms->uMsExtPro8BitClock);

		/* Check 8 bit transfer is OK or not */
		err = msi_pro_get_sys_info(ms);
		if (err) {
			TRACEX(("msi_pro_get_sys_info: %x, 8-bit mode, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			mcard->bMsProInit8BitErr = 1;
			return ERR_ERROR;
		}
		return ERR_SUCCESS;
	}

exit_pro_get_sys_info:
	err = msi_pro_get_sys_info(ms);
	if (err) {
		TRACEX(("msi_pro_get_sys_info: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int ms_init_card(struct ms_host *ms)
{
	struct _DEVICE_EXTENSION *pdx = (struct _DEVICE_EXTENSION *)ms->pdx;
	struct ms_card	*mcard = &ms->mcard;
	u8		type, category;
	u8		class;
	u8		tmp;
	int		err;
	int		i;
	u32		retry;


	TRACEW(("==========================================="));
	TRACEW(("ms_init_card ===>"));

	// initial mcard
	memset(mcard, 0, sizeof(struct ms_card));

	if (ms->pdx->current_card_active == SD_CARD_BIT) {
		writeb(0x00, ms->ioaddr + SD_OPT); // disbale 1.8v
	}

	ms_power_off(ms);

	retry = 0;


START_INIT:


	if (!ms->card_inserted) {
		TRACEX(("Ms card is removed, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		ms_update_media_card(ms);
		goto error_exit;
	}

	writeb((u8)pdx->uExtPadDrive0, pdx->ioaddr + CARD_PAD_DRIVE0);
	writeb((u8)pdx->uExtPadDrive1, pdx->ioaddr + CARD_PAD_DRIVE1);
	writeb((u8)pdx->uExtPadDrive2, pdx->ioaddr + CARD_PAD_DRIVE2);

	ms_card_active_ctrl(ms, TRUE);

	ms_reset(ms);

	ms_power_on(ms);
	TRACEW(("MS_POWER_CONTROL: %x", readb(ms->ioaddr + MS_POWER_CONTROL)));

	writeb((u8)ms->uMsExtHwTimeOutCnt, ms->ioaddr + CARD_TIME_OUT_CTRL);

	if (mcard->ptable) {
		/*ExFreePool(mcard->ptable);*/
		kfree(mcard->ptable);
		mcard->ptable = NULL;
	}


	if (ms->mcard.over_current) {
		TRACEX(("Over Current, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		goto error_exit;
	}


	/* Set serial mode */
	writeb(0x00, ms->ioaddr + MS_BUS_MODE_CTRL);

	ms_set_clock(ms, ms->uMsExtClock);

	err = msi_read_reg(ms, 0x02, 0x06);
	if (err) {
		TRACEX(("msi_read_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		ms_dumpregs(ms);
		goto error_exit;
	}

	for (i=0; i < 6; i++) {
		TRACEW(("Ms Reg[%x]: %x", i+2, ms->resp[i]));
	}

	type	= ms->resp[2];
	tmp		= ms->resp[3];
	category= ms->resp[4];
	class	= ms->resp[5];

	TRACEW(("type: %x, category: %x, class: %x", type, category, class));

	if ((type==0x00) || (type==0xff)) {
		if (type == 0x00) {
			if (category) {
				goto error_exit;
			}

			if (class > 0x03) {
				goto error_exit;
			}
		}
		else {
			if (category != 0xff) {
				goto error_exit;
			}

			if (class == 0x00) {
				goto error_exit;
			}

			if ((class >= 0x04) && (class <= 0xfe)) {
				goto error_exit;
			}
		}

		switch(class) {
		case 1:
		case 2:
		case 3:
			TRACEW(("MEMORY_STICK_ROM ......"));
			mcard->state  |= MS_STATE_HW_READONLY;
			mcard->bMsType = MEMORY_STICK_ROM;
			break;
		default:
			break;
		}

	}
	else if (type == 0x01) {
		if (category == 0) {
			switch (class) {
			case 0x81:
			case 0x82:
			case 0x83:
				mcard->bMsType = MEMORY_STICK_ROM;
				TRACEW(("MEMORY_STICK_ROM ......"));
				break;

			case 0x80:
				break;

			case 0x00:
				mcard->bMsType = MEMORY_STICK_PRO;
				TRACEW(("MEMORY_STICK_PRO ......"));
				break;

			case 0x01:
			case 0x02:
			case 0x03:
				mcard->bMsType = MEMORY_STICK_PRO_ROM;
				TRACEW(("MEMORY_STICK_PRO_ROM ......"));
				break;

			default:
				goto error_exit;
			}

			if (tmp == 0x07) {
				mcard->bMsType |= MEMORY_STICK_PRO_HG;
				TRACEW(("MEMORY_STICK_PRO_HG ......"));
			}
		}
		else {
			goto error_exit;
		}
	}
	else {
		goto error_exit;
	}


	/* Check if MS write-protected */
	/* Assign write-protect-bit for MS */
	if ((ms->resp[0] & WP) || (mcard->bMsType & MEMORY_STICK_ROM_GROUP)) {
		mcard->state   |= MS_STATE_HW_READONLY;
		mcard->bMsType |= MEMORY_STICK_ROM_GROUP;	/* Both MS Rom and MSPRO Rom, the LSb is 1 */
		TRACEW(("MEMORY_STICK_ROM_GROUP ......"));
	}
	else {
		mcard->state  &= ~MS_STATE_HW_READONLY;
	}

	TRACEW(("bMsType: %x", mcard->bMsType));

	/* Different path for MemoryStick & MemoryStick PRO */
	if (mcard->bMsType & MEMORY_STICK_PRO_GROUP) {
		err = ms_init_mspro_card(ms);
		if (err) {
			TRACEX(("ms_init_mspro: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto error_exit;
		}
		ms->card_type = MS_CARD_TYPE_MS_PRO;
	}
	else {
		err = ms_init_ms_card(ms);
		if (err) {
			TRACEX(("ms_init_ms: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			goto error_exit;
		}
		ms->card_type = MS_CARD_TYPE_MS;
	}

	ms_update_media_card(ms);
	return ERR_SUCCESS;

error_exit:

	TRACEX(("ms_init_card, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));

	retry++;

	ms_dumpregs(ms);

	ms_power_off(ms);


	if (ms->mcard.over_current) {
		TRACEY(("ms->mcard.over_current, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	if (ms->card_inserted) {
		TRACEW(("retry: %x, ms->uMsExtInitRetry: %x", retry, ms->uMsExtInitRetry));
		if (retry <= ms->uMsExtInitRetry) {
			u8 bMsProInit8BitErr;

			bMsProInit8BitErr = mcard->bMsProInit8BitErr; /* Store last error */
			TRACEW(("retry: %x, bMsProInit8BitErr: %x, ========================================", retry, bMsProInit8BitErr));

			if (mcard->ptable) {
				/*ExFreePool(mcard->ptable);*/
				kfree(mcard->ptable);
				mcard->ptable = NULL;
			}
			memset(mcard, 0x00, sizeof(struct ms_card));

			mcard->bMsProInit8BitErr = bMsProInit8BitErr;
			goto START_INIT;
		}
	}

	return ERR_ERROR;
}

/*************************************************************************************************/
void __________ms_card_access__________(void)
{

}

/*************************************************************************************************/
int msi_check_free_block_count(struct ms_host *ms, u16 segment)
{
	u16	*ptable	= ms->mcard.ptable;
	u16	good_cnt, max_seg;
	u16	offset, i;

	if (ptable == NULL) {
		return ERR_ERROR;
	}

	good_cnt = 0;
	offset = segment * MS_PAGE_SIZE + 496;
	for (i=0; i < 16; i++) {
		if (ptable[offset] != MS_UNUSED_BLOCK) {
			good_cnt++;
		}
		offset++;
	}

	max_seg = ms->mcard.wTotalBlockNum >> 9; /* 0x200 */

	TRACE1(("max_seg: %x, segment: %x, good_cnt: %x", max_seg, segment, good_cnt));
	if ((max_seg-1) == segment) {
		if (good_cnt <= 1) {
			TRACEX(("Last segment: MS_STATE_READONLY"));
			ms->mcard.state |= MS_STATE_READONLY;
			ms->pdx->media_card.media_state |= MEDIA_STATE_READONLY;
			return ERR_ERROR;
		}
	}
	else {
		if (good_cnt == 0) {
			TRACEX(("Not last segment: MS_STATE_READONLY"));
			ms->mcard.state |= MS_STATE_READONLY;
			ms->pdx->media_card.media_state |= MEDIA_STATE_READONLY;
			return ERR_ERROR;
		}
	}
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_evenly_find_freeblock(struct ms_host *ms, struct ms_page *page)
{
	u16	*ptable	= ms->mcard.ptable;
	u16	segment, segment_size;
	u16	offset, unused_blk, i;
	int	err;


	offset = page->wLogicalAdr;

	segment = 0;
	segment_size = 494;
	if (offset >= segment_size) {
		segment++;
		offset -= segment_size;

		segment_size = 496;
		while (offset >= segment_size) {
			segment++;
			offset -= segment_size;
		}
	}


	err = msi_check_free_block_count(ms, segment);
	if (err) {
		TRACEX(("msi_check_free_block_count: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	offset = segment * MS_PAGE_SIZE + 496;
	for (i=0; i < 16; i++) {
		if (ptable[offset] == MS_UNUSED_BLOCK) {
			offset++;
			continue;
		}

		unused_blk = ptable[offset];
		ptable[offset] = MS_UNUSED_BLOCK;
		page->wBlockAdr = unused_blk;
		return ERR_SUCCESS;
	}

	return ERR_ERROR;
}

/*************************************************************************************************/
int msi_store_block_to_table(struct ms_host *ms, struct ms_page *page)
{
	u16	offset;
	u16	segment, segment_size;


	offset  = page->wLogicalAdr;

	segment = 0;
	segment_size = 494;
	if (offset >= segment_size) {
		segment++;
		offset -= segment_size;

		segment_size = 496;
		while (offset >= segment_size) {
			segment++;
			offset -= segment_size;
		}
	}

	ms->mcard.ptable[segment * MS_PAGE_SIZE + offset] = page->wBlockAdr;

	return ERR_SUCCESS;

}

/*************************************************************************************************/
int msi_log2phy_block(struct ms_host *ms, struct ms_page *page)
{
	u16	offset;
	u16	segment, segment_size;

	offset = page->wLogicalAdr;

	segment = 0;
	segment_size = 494;
	if (offset >= segment_size) {
		segment++;
		offset -= segment_size;

		segment_size = 496;
		while (offset >= segment_size) {
			segment++;
			offset -= segment_size;
		}
	}

	page->wBlockAdr = ms->mcard.ptable[segment * MS_PAGE_SIZE + offset];

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_read_error_correctable(struct ms_host *ms, struct ms_page *page)
{
	int		err;

	TRACEW(("========================================"));
	TRACEW(("msi_read_error_correctable ===>"));

	ms->bMsErrorType = 0;

	err = msi_read_reg(ms, 0x03, 0x01);
	if (err) {
		TRACEX(("msi_read_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	TRACEW(("Status1 Reg: %x", ms->resp[0]));
	if (ms->resp[0] & UCDT_UCEX_UCFG) {
		TRACEX(("ms->resp[0] & UCDT_UCEX_UCFG: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->resp[0]));
		msi_clear_buffer(ms);

		if (ms->mcard.bMsType != MEMORY_STICK_ROM) {
			TRACEX(("msi_overwrite_extra_data ..."));
			msi_overwrite_extra_data(ms, page, PAGE_STATUS_NG_MASK_DATA);
		}
		return ERR_ERROR;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_read_reg(struct ms_host *ms, u8 rd_addr, u8 rd_size)
{
	u8	buf[4];
	int	err;

	TRACE(("msi_read_reg ===>"));

	buf[0] = rd_addr;
	buf[1] = rd_size;
	buf[2] = 0x10;
	buf[3] = 0x06;

	err = ms_tpc_set_rw_reg_adrs(ms, buf);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_read_reg(ms, rd_size);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_set_if_mode(struct ms_host *ms, u8 if_mode)
{
	u8	buf[4];
	int	err;

	TRACEW(("========================================="));
	TRACEW(("msi_set_if_mode ===>, if_mode: %x", if_mode));

	if (if_mode == MS_SERIAL_IF) {
		TRACEW(("MS_SERIAL_IF"));
	}
	if (if_mode == MS_PARALLEL_4BIT_IF) {
		TRACEW(("MS_PARALLEL_4BIT_IF"));
	}
	if (if_mode == MS_PRO_PARALLEL_4BIT_IF) {
		TRACEW(("MS_PRO_PARALLEL_4BIT_IF"));
	}
	if (if_mode == MS_PRO_PARALLEL_8BIT_IF) {
		TRACEW(("MS_PRO_PARALLEL_8BIT_IF"));
	}

	buf[0] = 0x10;
	buf[1] = 0x06;
	buf[2] = 0x10;
	buf[3] = 0x01;

	err = ms_tpc_set_rw_reg_adrs(ms, buf);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	buf[0] = if_mode;
	err = ms_tpc_write_reg(ms, buf, 1);
	if (err) {
		TRACEX(("ms_tpc_write_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (if_mode == MS_PRO_PARALLEL_4BIT_IF) {
		/* Set to 4 bit mode */
		writeb(MS_BUS_4BIT_MODE, ms->ioaddr + MS_BUS_MODE_CTRL);
		ms->mcard.ms_bus_mode |= MS_PRO_PARALLEL_MODE;
	}

	if (if_mode == MS_PRO_PARALLEL_8BIT_IF) {
		writeb(MS_BUS_8BIT_MODE, ms->ioaddr + MS_BUS_MODE_CTRL);
		ms->mcard.ms_bus_mode |= MS_PRO_PARALLEL_MODE;
	}

	return err;
}

/*************************************************************************************************/
int msi_reset(struct ms_host *ms)
{
	int err;

	TRACEW(("=========================="));
	TRACEW(("msi_reset ===>"));

	if ((ms->resp[0] & MS_MALFUNCTION) == MS_MALFUNCTION) {
		TRACEW(("MS_MALFUNCTION ....."));

		err = ms_tpc_set_cmd(ms, MS_FLASH_STOP, 0, NULL);
		if (err) {
			TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}
	}

	err = ms_tpc_set_cmd(ms, MS_RESET, 0, NULL);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_read_extra_data_only_by_page(struct ms_host *ms, struct ms_page *page)
{
	int err;

	TRACE(("msi_read_extra_data_only_by_page ===>"));

	/* Double assign */
	page->bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
	err = msi_set_read_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_read_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_READ, FALSE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = msi_read_reg(ms, 0x16, 0x04);
	if (err) {
		TRACEX(("msi_read_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_get_boot_block(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	struct ms_page	page;
	int	err;
	u8	blk;
	u8	buf[MS_PAGE_SIZE];

	TRACEW(("=========================="));
	TRACEW(("msi_get_boot_block ===>"));

	mcard->bBootBlockAdr0 = 0xff;

	for(blk=0; blk < 12; blk++) {

		TRACEW(("blk: %x", blk));

		memset(&page, 0x00, sizeof(struct ms_page));

		page.wBlockAdr		= blk;
		page.bPageAdr		= 0x00;
		page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
		err = msi_read_extra_data_only_by_page(ms, &page);
		if (err) {
			TRACEX(("msi_read_extra_data_only_by_page: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
			continue;
		}

		TRACE2(("ms->resp[0]: %02x, ms->resp[1]: %02x", ms->resp[0], ms->resp[1]));

		/* check if Block Status is 1 (OK) */
		if ((ms->resp[0] & 0x80) == 0x00) {
			TRACEW(("ms->resp[0] & 0x80) == 0x00, resp[0]: %x", ms->resp[0]));
			continue;
		}

		if (ms->resp[1] & NOT_BOOT_BLOCK) {
			ms->mcard.bBootAreaProtectFlag |= FIND_GOOD_BLOCK;
			TRACEW(("blk: %x, ms->resp[1] & NOT_BOOT_BLOCK == TRUE, %x, XXXXXXXXXXXXXXXXXXXXXX", blk, ms->resp[1]));
			continue;
		}

		/* Read page0 of boot block data to buffer1 */
		page.bCmdParameter = DATA_AND_EXTRA_DATA_BY_PAGE;

		page.buf		= buf;
		page.buf_type	= BUF_NOT_MDL;
		err = msi_read_boot_block_data(ms, &page);
		if (err) {
			TRACEW(("msi_read_boot_block_data: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
			continue;
		}


		TRACE2(("buf[0]: %x, buf[1]: %x", buf[0], buf[1]));
		/* Check boot block id */
		if ((buf[0] != 0x00) || (buf[1] != 0x01)) {
			mcard->bBootAreaProtectFlag |= FIND_GOOD_BLOCK;
			TRACEW(("mcard->bBootAreaProtectFlag |= FIND_GOOD_BLOCK; ........."));
			continue;
		}

		TRACE2(("buf[416]: %x, buf[470]: %x, buf[472]: %x", buf[416], buf[470], buf[472]));
		if ((buf[416] != MS_CLASS_VER_1XX) || (buf[470] != FORMAT_TYPE_FAT)) {
			TRACEW(("(buf[416] != MS_CLASS_VER_1XX) || (buf[470] != FORMAT_TYPE_FAT)"));
			continue;
		}

		/* Check MemoryStick device type */
		if (buf[472] >= DEVICE_TYPE_ROM) {
			TRACEW(("buf[472] >= DEVICE_TYPE_ROM, %x", buf[472]));
			continue;
		}

		if (mcard->bBootAreaProtectFlag & FIND_GOOD_BLOCK) {
			mcard->bBootAreaProtectFlag = NEED_BOOT_AREA_PROTECT_PROCESS;
			TRACEW(("mcard->bBootAreaProtectFlag = NEED_BOOT_AREA_PROTECT_PROCESS;"));
		}

		if (mcard->bBootBlockAdr0 == 0xff) {
			TRACEW(("blk: %x, mcard->bBootBlockAdr0 == 0xff, ................", blk));
			if (buf[472]) {
				TRACEW(("buf[472]: %x, MEMORY_STICK_ROM ....", buf[472]));
				mcard->bMsType = MEMORY_STICK_ROM;
				mcard->state  |= MS_STATE_HW_READONLY;
			}

			mcard->bBlockSize		= buf[419] << 1;
			TRACEW(("mcard->bBlockSize: %x", mcard->bBlockSize));
			mcard->wTotalBlockNum	= (u16)buf[420] << 8;
			TRACEW(("mcard->wTotalBlockNum: %x", mcard->wTotalBlockNum));

			//mcard->ptable = (u16*)ExAllocatePool(NonPagedPool, mcard->wTotalBlockNum * 2);
			mcard->ptable = (u16*)kmalloc(mcard->wTotalBlockNum * 2, GFP_KERNEL);
			if (!mcard->ptable) {
				TRACEX(("kmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
				return ERR_ERROR;
			}
			memset(mcard->ptable, 0xff, mcard->wTotalBlockNum * 2);

			mcard->total_lba = ((u32)buf[422] << 8) + buf[423];
			mcard->total_lba -= 2;

			mcard->total_lba *= (mcard->bBlockSize);
			TRACEW(("mcard->total_lba: %x, (%u)", mcard->total_lba, mcard->total_lba));

			mcard->bBootBlockAdr0 = blk;

			memcpy(ms->boot_block, buf, SECTOR_SIZE);
		}
		else {
			TRACEW(("mcard->bBootBlockAdr0: %x, new: %x", mcard->bBootBlockAdr0, blk));
			mcard->bBootBlockAdr0 = blk;
			break;
		}
	}

	if (mcard->wTotalBlockNum < 0x200) {
		TRACEX(("mcard->wTotalBlockNum < 0x200, %x, XXXXXXXXXXXXXXXXXXXXXXXX", mcard->wTotalBlockNum));
		return ERR_ERROR;
	}

	if (mcard->bBootBlockAdr0 == 0xff) {
		TRACEX(("mcard->bBootBlockAdr0 == 0xff, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	TRACEW(("msi_get_boot_block <==="));
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_read_boot_block_data(struct ms_host *ms, struct ms_page *page)
{
	int	err;

	TRACEW(("============================="));
	TRACEW(("msi_read_boot_block_data ===>"));

	err = msi_set_read_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_read_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
 	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_READ, TRUE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_read_page_data(ms, page);
	if (err) {
		TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_set_read_cmd_parameter(struct ms_host *ms, struct ms_page *page)
{
	int	err;
	u8	buf[6];

	buf[0]	= 0x16;
	buf[1]	= 0x04;
	buf[2]	= 0x10;
	buf[3]	= 0x06;

	err = ms_tpc_set_rw_reg_adrs(ms, buf);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->mcard.ms_bus_mode & MS_PRO_PARALLEL_MODE) {
		buf[0]	= MS_PARALLEL_4BIT_IF;
	}
	else {
		buf[0]	= MS_SERIAL_IF;
	}
	buf[1] = 0x00; /* Block address 2 (we needn't use this byte) */
	buf[2] = page->bBlockAdr1;		/* Block address 1 */
	buf[3] = page->bBlockAdr0;		/* Block address 0 */
	buf[4] = page->bCmdParameter;	/* Cmd parameter */
	buf[5] = page->bPageAdr;		/* Page address */

	err = ms_tpc_write_reg(ms, buf, 6);
	if (err) {
		TRACEX(("ms_tpc_write_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
	}

	return err;
}

/*************************************************************************************************/
int msi_send_read_cmd_wo_data(struct ms_host *ms, struct ms_page *page)
{
	int	err;

	TRACE1(("msi_send_read_cmd_wo_data ===>"));

	err = msi_set_read_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_read_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_READ, TRUE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_send_write_cmd_wo_data(struct ms_host *ms, struct ms_page *page)
{
	int	err;

	TRACE1(("msi_send_write_cmd_wo_data ===>"));

	err = msi_set_write_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_write_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_WRITE, FALSE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int	msi_check_disabled_block(struct ms_host *ms, u16 blk)
{
	u8	*buf = ms->mcard.init_badblk;
	u32	i;

	i = 0;
	while ((buf[i] != 0xff) || (buf[i+1] != 0xff)) {

		/* Data is big-endian in boot-block */
		if ((buf[i] == (u8)(blk >> 8)) && (buf[i+1] == (u8)blk)) {
			return TRUE; /* it is a disabled block */
		}
		i += 2;
	}

	return FALSE; /* this is a good block */
}

/*************************************************************************************************/
int msi_set_write_cmd_parameter(struct ms_host *ms, struct ms_page *page)
{
	int	err;
	u8	buf[15];

	buf[0]	= 0x16;
	buf[1]	= 0x06;
	buf[2]	= 0x10;
	buf[3]	= 0x0f;

	err = ms_tpc_set_rw_reg_adrs(ms, buf);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->mcard.ms_bus_mode & MS_PRO_PARALLEL_MODE) {
		buf[0]	= MS_PARALLEL_4BIT_IF;
	}
	else {
		buf[0]	= MS_SERIAL_IF;
	}

	buf[1] = 0x00; /* Block address 2 (we needn't use this byte) */
	buf[2] = page->bBlockAdr1;		/* Block address 1 */
	buf[3] = page->bBlockAdr0;		/* Block address 0 */
	buf[4] = page->bCmdParameter;	/* Cmd parameter */
	buf[5] = page->bPageAdr;		/* Page address */
	buf[6] = page->bMsOverwriteFlag;
	buf[7] = 0xff;
	buf[8] = page->bLogicalAdr1;
	buf[9] = page->bLogicalAdr0;

	buf[10] = 0xff;
	buf[11] = 0xff;
	buf[12] = 0xff;
	buf[13] = 0xff;
	buf[14] = 0xff;

	err = ms_tpc_write_reg(ms, buf, 15);
	if (err) {
		TRACEX(("ms_tpc_write_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_clear_buffer(struct ms_host *ms)
{
	int	err;

	TRACE1(("msi_clear_buffer ===>"));

	err = ms_tpc_set_cmd(ms, MS_CLEAR_BUF, 0, NULL);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_copy_page(struct ms_host *ms, struct ms_page *rd, struct ms_page *wr)
{
	int				err;

	TRACE(("msi_copy_page ===>"));

	err = msi_send_read_cmd_wo_data(ms, rd);
	if (err) {
		TRACEX(("msi_send_read_cmd_wo_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		rd->bPageAdr = 0;
		msi_overwrite_extra_data(ms, rd, BLOCK_STATUS_MASK_DATA);
		msi_overwrite_extra_data(ms, wr, PAGE_STATUS_DATA_ERROR_MASK_DATA);
		return err;
	}

	err = msi_read_reg(ms, 0x16, 0x01);
	if (err) {
		TRACEX(("msi_read_reg: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		msi_clear_buffer(ms);
		return err;
	}

	wr->bMsOverwriteFlag = ms->resp[0] | UPDATE_STATUS;
	err = msi_send_write_cmd_wo_data(ms, wr);
	if (err) {
		TRACEX(("msi_send_write_cmd_wo_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		if((ms->int_resp & MS_PRO_ERR_CMDNK) == MS_PRO_ERR) {
			wr->bPageAdr = 0;
			msi_overwrite_extra_data(ms, wr, BLOCK_STATUS_MASK_DATA);
		}
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_copy_block(struct ms_host *ms, struct ms_page *rd, struct ms_page *wr)
{
	u8	i;
	int	err;

	for (i=0; i < ms->mcard.bBlockSize; i++) {
		rd->bPageAdr = i;
		wr->bPageAdr = i;
		err = msi_copy_page(ms, rd, wr);
		if (err) {
			TRACEX(("msi_copy_page: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_overwrite_extra_data(struct ms_host *ms, struct ms_page *page, u8 bOverwriteMask)
{
	int	err;
	u8	buf[7];

	TRACE1(("msi_overwrite_extra_data ===>"));

	buf[0]	= 0x10;
	buf[1]	= 0x06;
	buf[2]	= 0x10;
	buf[3]	= 0x07;

	err = ms_tpc_set_rw_reg_adrs(ms, buf);
	if (err) {
		TRACEX(("ms_tpc_set_rw_reg_adrs: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->mcard.ms_bus_mode & MS_PRO_PARALLEL_MODE) {
		buf[0]	= MS_PARALLEL_4BIT_IF;
	}
	else {
		buf[0]	= MS_SERIAL_IF;
	}
	buf[1] = 0x00;					/* Block address 2 (we needn't use this byte) */
	buf[2] = page->bBlockAdr1;		/* Block address 1 */
	buf[3] = page->bBlockAdr0;		/* Block address 0 */
	buf[4] = OVERWRITE_FLAG_BY_PAGE;/* Cmd parameter */
	buf[5] = page->bPageAdr;		/* Page address */
	buf[6] = bOverwriteMask;		/* Overwrite flag (Mask data) */

	err = ms_tpc_write_reg(ms, buf, 7);
	if (err) {
		TRACEX(("ms_tpc_write_reg: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_WRITE, FALSE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_bootarea_protection_process(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	struct ms_page	page;
	struct ms_page	free_page;
	u16			free_blk;
	u8			blk;
	u8			i;
	int			err;
	u16			src_log_addr;

	TRACEW(("====================================="));
	TRACEW(("msi_bootarea_protection_process ===>, mcard->bBootBlockAdr0: %x", mcard->bBootBlockAdr0));


	for(blk=0; blk < mcard->bBootBlockAdr0; blk++) {

		if (msi_check_disabled_block(ms, blk) == TRUE) {
			TRACEW(("Disable block: %04x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", blk));
			continue;
		}

		/* Read ExtraDataArea */
		page.wBlockAdr		= blk;
		page.bPageAdr		= 0x00;
		page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
		err = msi_read_extra_data_only_by_page(ms, &page);
		if (err) {
			continue;
		}

		/* if this is boot block, skip it */
		if (!(ms->resp[1] & NOT_BOOT_BLOCK)) {
			continue;
		}

		/* if Block Status is NG, skip it */
		if ((ms->resp[0] & 0x80) == 0x00) {
			continue;
		}

		/* Check if logical block address assigned */
		if ((ms->resp[2] == 0xff) && (ms->resp[3] == 0xff)) {
			continue;
		}
		src_log_addr = ms->resp[2] << 8 | ms->resp[3];

		TRACEW(("blk: %x, mcard->bBootBlockAdr0: %x, resp[2]: %x, resp[3]: %x", blk, mcard->bBootBlockAdr0, ms->resp[2], ms->resp[3]));

		/*==================================================*/
		/* Found effective block in boot area, we have to 	*/
		/* copy this block to user area.				 	*/
		/*==================================================*/
		for (free_blk=mcard->bBootBlockAdr0+1; free_blk < MS_SEGMENT_SIZE; free_blk++) {
			if (msi_check_disabled_block(ms, free_blk) == TRUE) {
				TRACEW(("Disable block: %04x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", free_blk));
				continue;
			}

			TRACEW(("free blk: %x", free_blk));

			free_page.bBlockAdr1	= (u8)(free_blk >> 8);
			free_page.bBlockAdr0	= (u8)(free_blk);
			free_page.bPageAdr		= 0x00;
			free_page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
			err = msi_read_extra_data_only_by_page(ms, &free_page);
			if (err) {
				TRACEX(("msi_read_extra_data_only_by_page: %x", err));
				continue;
			}

			free_page.bMsOverwriteFlag = ms->resp[0];
			TRACEW(("free_page.bMsOverwriteFlag: %x", free_page.bMsOverwriteFlag));
			if ((free_page.bMsOverwriteFlag & BLOCK_PAGE_STATUS_OK) != BLOCK_PAGE_STATUS_OK) {
				continue;
			}

			TRACEW(("resp[2]: %x, resp[3]: %x", ms->resp[2], ms->resp[3]));
			if ((ms->resp[2] != 0xff) || (ms->resp[3] != 0xff)) {
				continue;
			}

			/* if fail to erase this block, skip it and search for next free block */
			err = msi_erase_block_and_forget_it(ms, &free_page);
			if (err) {
				TRACEW(("msi_erase_block_and_forget_it: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
				continue;
			}

			/* msi_copy_block */
			for (i=0; i < ms->mcard.bBlockSize; i++) {
				page.bPageAdr		= i;
				page.bCmdParameter  = DATA_AND_EXTRA_DATA_BY_PAGE;
				free_page.bPageAdr	= i;
				free_page.bCmdParameter = DATA_AND_EXTRA_DATA_BY_PAGE;
				free_page.wLogicalAdr   = src_log_addr;
				err = msi_copy_page(ms, &page, &free_page);
				if (err) {
					TRACEW(("msi_copy_page: %x, XXXXXXXXXXXXXXXXXXXXXXXX", err));
					continue;
				}
			}
			break;
		}

		TRACEW(("msi_overwrite_extra_data, wBlockAdr: %x", page.wBlockAdr));
		page.bPageAdr		= 0;
		free_page.bPageAdr	= 0;

		err = msi_overwrite_extra_data(ms, &page, BLOCK_STATUS_MASK_DATA);
		if (err) {
			TRACEW(("msi_overwrite_extra_data: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		}
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_erase_block_and_forget_it(struct ms_host *ms, struct ms_page *page)
{
	int err;

	err = msi_set_read_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_read_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_ERASE, 0, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_put_block_to_free_table(struct ms_host *ms, struct ms_page *page)
/*************************************************************************************************/
{
	u16			*ptable	= ms->mcard.ptable;
	u16			wseg, blk, i;

	wseg = page->wBlockAdr >> 9;
	blk = wseg * MS_PAGE_SIZE + 496;
	for (i=0; i < 16; i++) {
		if (ptable[blk] != MS_UNUSED_BLOCK) {
			blk++;
			continue;
		}
		ptable[blk] = page->wBlockAdr;
		return ERR_SUCCESS;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_erase_block_and_put_to_table(struct ms_host *ms, struct ms_page *page)
{
	int err;

	TRACE1(("msi_erase_block_and_put_to_table ===>, blk: %x, log_blk: %x", page->wBlockAdr, page->wLogicalAdr));

	err = msi_erase_block_and_forget_it(ms, page);
	if (err) {
		TRACEX(("msi_erase_block_and_forget_it: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	msi_put_block_to_free_table(ms, page);

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_set_logblock_adr(struct ms_host *ms, struct ms_page *page)
{
	int err;

	page->bPageAdr			= 0;
	page->bCmdParameter		= EXTRA_DATA_ONLY_BY_PAGE;
	page->bMsOverwriteFlag	= 0xf8;

	err = msi_set_write_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_write_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_WRITE, FALSE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return err;
}

/*************************************************************************************************/
int msi_log_addr_confirm_procedure(struct ms_host *ms, u16 wLowerBound, u16 wUpperBound, u16 lastblk, u8 *change)
{
	struct ms_page	page;
	u16	blk, wlogAdr;
	int	err;

	TRACEW(("==============================================="));
	TRACEW(("msi_log_addr_confirm_procedure ===>, wLowerBound: %x, wUpperBound: %x", wLowerBound, wUpperBound));

	*change = 0;
	for (wlogAdr=wLowerBound; wlogAdr < wUpperBound; wlogAdr++) {

		if (!ms->card_inserted) {
			TRACEX(("msi_log_addr_confirm_procedure --- Ms card is removed, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			return ERR_ERROR;
		}
		page.wLogicalAdr = wlogAdr;
		msi_log2phy_block(ms, &page);
		if (page.wBlockAdr == MS_UNUSED_BLOCK) {

			TRACEW(("wlogAdr: %x", wlogAdr));
			*change = 1;

			/* Find a free block to set logical block address */
			for (blk=lastblk-MS_SEGMENT_SIZE; blk < lastblk; blk++) {

				if (msi_check_disabled_block(ms, blk) == TRUE) {
					TRACEW(("Disable block: %04x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", blk));
					continue;
				}

				page.wBlockAdr		= blk;
				page.bPageAdr		= 0x00;
				page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
				err = msi_read_extra_data_only_by_page(ms, &page);
				if (err) {
					continue;
				}

				wlogAdr = ((u16)ms->resp[2] << 8) | ms->resp[3];
				if (wlogAdr != MS_UNUSED_BLOCK) {
					continue;
				}

				page.wBlockAdr = blk;
				err = msi_set_logblock_adr(ms, &page);
				if (err) {
					TRACEX(("msi_set_logblock_adr: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
					return err;
				}
				break;
			}

			if (blk == lastblk) {
				TRACEW(("blk == lastblk, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
				return ERR_ERROR;
			}
		}
	}

	TRACEW(("msi_log_addr_confirm_procedure <==="));
	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_make_trans_table(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	struct ms_page	page;
	struct ms_page	old_page;
	u16	*ptable	= mcard->ptable;
	u16	blk, log_blk, phy_blk;
	u16	lower_bound, upper_bound;
	u8	overwrite_flag;
	int	err;

	TRACEW(("====================================="));
	TRACEW(("msi_make_trans_table ===>, wTotalBlockNum: %x", mcard->wTotalBlockNum));

	lower_bound = 0;
	upper_bound = 494;

	TRACE1(("lower_bound: %x, upper_bound: %x", lower_bound, upper_bound));

	for (blk=0; blk < mcard->wTotalBlockNum; blk++) {

		TRACE(("blk: %x, ==========================", blk));
		if (!ms->card_inserted) {
			TRACEX(("msi_make_trans_table --- Ms card is removed, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
			return ERR_ERROR;
		}

		if (blk && (((blk-1) & 0x1ff) == 0x1ff)) {

			if (mcard->bMsType != MEMORY_STICK_ROM) {
				u8 change = 0;
				TRACEW(("blk: %x, --- msi_log_addr_confirm_procedure ...", blk));
				err = msi_log_addr_confirm_procedure(ms, lower_bound, upper_bound, blk, &change);
				if (err) {
					TRACEX(("msi_log_addr_confirm_procedure: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
					return err;
				}

				if (change) {
					TRACEW(("re-build this segment, ....................."));
					/* MUST re-build this segment */
					blk -= MS_SEGMENT_SIZE;
					memset(ptable + blk, 0xff, MS_SEGMENT_SIZE * 2);
					blk --; /* after continue, it will run blk++ */
					continue;
				}
			}

			lower_bound = upper_bound;
			upper_bound += 496;
			TRACE1(("lower_bound: %x, upper_bound: %x", lower_bound, upper_bound));
		}

rebuild_segment:

		if (mcard->bMsType != MEMORY_STICK_ROM) {
			if (msi_check_disabled_block(ms, blk) == TRUE) {
				TRACEW(("Disable block: %04x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", blk));
				continue;
			}
		}

		page.wBlockAdr		= blk;
		page.bPageAdr		= 0x00;
		page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;
		err = msi_read_extra_data_only_by_page(ms, &page);
		if (err) {
			TRACEW(("blk: %x, msi_read_extra_data_only_by_page: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", blk, err));
			continue;
		}

		overwrite_flag = ms->resp[0];
		TRACE(("blk: %x,   %x, %x,    %x, %x", blk, ms->resp[0], ms->resp[1], ms->resp[2], ms->resp[3]));
		if (((overwrite_flag & BLOCK_PAGE_STATUS_OK) != BLOCK_PAGE_STATUS_OK) &&
			((overwrite_flag & BLOCK_PAGE_STATUS_OK) != BLOCK_OK_PAGE_DATA_ERROR)) {
			TRACEW(("blk: %x, overwrite_flag: %x", blk, overwrite_flag));
			continue;
		}

		log_blk = ((u16)ms->resp[2] << 8) | ms->resp[3];
		TRACE(("blk: %04x, log_blk: %04x", blk, log_blk));

		/* Fake boot-block */
		if (!(ms->resp[1] & NOT_BOOT_BLOCK) || (log_blk == MS_UNUSED_BLOCK)) {
			TRACEW(("blk: %x, Fake boot-block, ms->resp[1]: %x, log_blk: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", blk, ms->resp[1], log_blk));
			if (mcard->bMsType != MEMORY_STICK_ROM) {
				err = msi_erase_block_and_put_to_table(ms, &page);
				continue;
			}
		}

		/* Logical addr isn't in the segment range, erase and assign to Free Table */
		if ((log_blk < lower_bound) || (log_blk >= upper_bound)) {
			TRACEW(("blk: %x, log_blk: %x, lower_bound: %x, upper_bound: %x, XXXXXXXXXXXXXXXXXXX", blk, log_blk, lower_bound, upper_bound));
			if (mcard->bMsType != MEMORY_STICK_ROM) {
				err = msi_erase_block_and_put_to_table(ms, &page);
				continue;
			}
		}

		page.wLogicalAdr = log_blk;
		msi_log2phy_block(ms, &page);
		phy_blk = page.wBlockAdr;
		page.wBlockAdr = blk;
		TRACE(("table phy_blk: %04x", phy_blk));
		if (phy_blk == MS_UNUSED_BLOCK) {
			/* If the block address hasn't been assigned, write physical address to Table */
			goto STORE_TABLE;
		}


		TRACEW(("blk: %x, phy_blk: %x, log_blk: %x", blk, phy_blk, log_blk));
		/* Multiple identical logic block address correction */
		old_page.wBlockAdr		= phy_blk;
		old_page.bPageAdr		= 0x00;
		old_page.bCmdParameter	= EXTRA_DATA_ONLY_BY_PAGE;

		/* Check update-status for the block in progress */
		if (!(overwrite_flag & UPDATE_STATUS)) {
			if (mcard->bMsType != MEMORY_STICK_ROM) {
				TRACEW(("overwrite_flag: %x", overwrite_flag));
				msi_erase_block_and_put_to_table(ms, &old_page);
			}
			goto STORE_TABLE;
		}

		err = msi_read_extra_data_only_by_page(ms, &old_page);
		if (err) {
			TRACEW(("msi_read_extra_data_only_by_page: %x", err));
			if (mcard->bMsType != MEMORY_STICK_ROM) {
				msi_erase_block_and_put_to_table(ms, &old_page);
			}
			goto STORE_TABLE;
		}

		if (!(ms->resp[0] & UPDATE_STATUS)) {
			TRACEW(("blk: %x, phy_blk: %x, (ms->resp[0] & UPDATE_STATUS) == 0, XXXXXXXXXXXXXXXXXXXXXXXXXXX", blk, phy_blk));
			TRACEW(("wBlockAdr: %x", page.wBlockAdr));
			if (mcard->bMsType != MEMORY_STICK_ROM) {
				msi_erase_block_and_put_to_table(ms, &page);
			}
			continue;
		}

		if (mcard->bMsType != MEMORY_STICK_ROM) {
			msi_erase_block_and_put_to_table(ms, &old_page);
		}

STORE_TABLE:

		TRACE(("blk: %x, log_blk: %x", blk, log_blk));
		page.wBlockAdr   = blk;
		page.wLogicalAdr = log_blk;
		msi_store_block_to_table(ms, &page);
	}

	if (mcard->bMsType != MEMORY_STICK_ROM) {
		u8 change = 0;
		err = msi_log_addr_confirm_procedure(ms, lower_bound, upper_bound, blk, &change);
		if (err) {
			TRACEX(("msi_log_addr_confirm_procedure: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
			return err;
		}

		if (change) {
			TRACEW(("re-build this segment, ....................."));
			/* MUST re-build this segment */
			blk -= MS_SEGMENT_SIZE;
			memset(ptable + blk, 0xff, MS_SEGMENT_SIZE * 2);

			goto rebuild_segment;
		}
	}

	for (blk=0; blk < mcard->wTotalBlockNum; blk++) {
		TRACE1(("blk: %04x, Table: %04x", blk, ptable[blk]));
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_write_page_data(struct ms_host *ms, struct ms_page *page)
{
	int	err;

	page->bMsOverwriteFlag = 0xf8;
	err = msi_set_write_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_write_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_WRITE, TRUE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_write_page_data(ms, page);
	if (err) {
		TRACEX(("ms_tpc_write_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_read_page_data(struct ms_host *ms, struct ms_page *page)
{
	int err;

	ms->bMsErrorType = 0;

	err = msi_set_read_cmd_parameter(ms, page);
	if (err) {
		TRACEX(("msi_set_read_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_set_cmd(ms, MS_BLOCK_READ, TRUE, page);
	if (err) {
		TRACEX(("ms_tpc_set_cmd: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	err = ms_tpc_read_page_data(ms, page);
	if (err) {
		TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* Check if the page is Digtal-Read-Protected */
	/* MsReadManagementFlagReg */
	err = ms_tpc_read_reg(ms, 4);
	if (err) {
		TRACEX(("ms_tpc_read_reg: %x,XXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	/* this page is read-protected */
	if ((ms->resp[1] & SCMS0_SCMS1) != SCMS0_SCMS1) {
		TRACEX(("(ms->resp[1] & SCMS0_SCMS1) != SCMS0_SCMS1, %x", ms->resp[1]));
		ms->bMsErrorType |= MS_READ_PROTECTED;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
void __________ms_pro_card_access__________(void)
{

}

/*************************************************************************************************/
int msi_pro_set_cmd_parameter(struct ms_host *ms, u8 *pbuf)
{
	int	err;

	err = ms_tpc_write(ms, pbuf, 7, MS_TPC_EX_SET_CMD);
	if (err) {
		TRACEX(("ms_tpc_write: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", err));
		return err;
	}

	if (ms->int_resp & MS_PRO_ERR_CMDNK) {
		TRACEX(("ms->int_resp & MS_PRO_ERR_CMDNK == TRUE, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		return ERR_ERROR;
	}

	return err;
}

/*************************************************************************************************/
int msi_pro_stop(struct ms_host *ms)
{
	int	err;
	u8	buf[7];

	TRACEW(("msi_pro_stop ===>"));

	buf[0]	= MSPRO_STOP;
	buf[1]	= 0;
	buf[2]	= 0;
	buf[3]	= 0;
	buf[4]	= 0;
	buf[5]	= 0;
	buf[6]	= 0;

	err = msi_pro_set_cmd_parameter(ms, buf);
	if (err) {
		TRACEX(("msi_pro_set_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
		return ERR_ERROR;
	}

	if (ms->int_resp & MS_PRO_ERR_CMDNK) {
		TRACEX(("msi_pro_set_cmd_parameter, ms->int_resp & MS_PRO_ERR_CMDNK == TRUE, ms->int_resp: %x, XXXXXXXXXXXXXXXXXXX", ms->int_resp));
		return ERR_ERROR;
	}

	if (!(ms->int_resp & MS_PRO_CED)) {
		TRACEX(("(ms->int_resp & MS_PRO_CED) == 0, int_resp: %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		return ERR_ERROR;
	}

	return ERR_SUCCESS;
}

/*************************************************************************************************/
int msi_pro_get_sys_info(struct ms_host *ms)
{
	struct ms_card	*mcard = &ms->mcard;
	struct ms_page	page;
	int	err, i;
	u8	*buf;
	u16	wstart;
	u32	blksize;

	TRACEW(("==================================================="));
	TRACEW(("msi_pro_get_sys_info ===>"));

	buf = (u8 *)vmalloc(1024);
	if (!buf) {
		TRACEX(("vmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return ERR_ERROR;
	}

	buf[0]	= MSPRO_READ_ATRB;
	buf[1]	= 0;
	buf[2]	= 1;
	buf[3]	= 0;
	buf[4]	= 0;
	buf[5]	= 0;
	buf[6]	= 0;
	err = msi_pro_set_cmd_parameter(ms, buf);
	if (err) {
		TRACEX(("msi_pro_set_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
		err = ERR_ERROR;
		goto Exit;
	}

	if (ms->int_resp & MS_PRO_ERR_CMDNK) {
		TRACEX(("ms->int_resp & MS_PRO_ERR_CMDNK: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		err = ERR_ERROR;
		goto Exit;
	}

	memset(&page, 0x00, sizeof(struct ms_page));

	page.get_int	= TRUE;
	page.buf		= buf;
	page.buf_type	= BUF_NOT_MDL;

	err = ms_tpc_read_page_data(ms, &page);
	if (err) {
		TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
		goto Exit;
	}

	/*
	for (i=0; i< MS_PAGE_SIZE; i++) {
		PRINT0(("%02x, ", buf[i]));
		if ((i & 0xf) == 0xf) {
			PRINT0(("\n"));
		}
	}
	PRINT0(("\n"));
	*/


	/* Check signature 0xA5C3 */
	if ((buf[0] != 0xa5) || (buf[1] != 0xc3)) {
		TRACEX(("(buf[0] != 0xa5) || (buf[1] != 0xc3), : %x, %x, XXXXXXXXXXXXXXXX", buf[0], buf[1]));
		err = ERR_ERROR;
		goto Exit;
	}

	/* check the number of device information entries is between 1 to 12 */
	if ((buf[0x04] == 0) || (buf[0x04] > 12)) {
		TRACEX(("(buf[0x04] == 0) || (buf[0x04] > 12), : %x, XXXXXXXXXXXXXXXX", buf[4]));
		err = ERR_ERROR;
		goto Exit;
	}

	/*
	 Look for the entry of System information, which Device information ID being 0x10
	 And get the entry address and size
	*/
	for (i = 24; i < 168 ; i+=12) {
		if (buf[i] == 0x10) { /* Device information ID = System Information */
			break;
		}
		else if (buf[i] == 0x13) { /* Device information ID of EHC card */
			ms->mcard.bMsType &= ~MEMORY_STICK_PRO;
			ms->mcard.bMsType |= MEMORY_STICK_EHC;
			break;
		}
	}

	if (i == 168) {
		TRACEX(("i == 168, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = ERR_ERROR;
		goto Exit;
	}

	if ((buf[i-1] != 96) || (buf[i-2] != 0) || (buf[i-3] != 0) || (buf[i-4] != 0)) {
		err = ERR_ERROR;
		goto Exit;
	}

	if ((buf[i-7] != 0) || (buf[i-8] != 0)) {
		err = ERR_ERROR;
		goto Exit;
	}

	wstart = ((u16)buf[i-6] << 8) | buf[i-5];

	/* if the address of system information is not 0x01A0, another READ_ATRB is needed */
	if (wstart != 0x01a0) {
		if ((wstart > 0x8000) || (wstart < 0x01a0)) {
			TRACEX(("(wstart > 0x8000) || (wstart < 0x01a0), wstart: %x, XXXXXXXXXXXXXXXXXXXXXXXXXX", wstart));
			err = ERR_ERROR;
			goto Exit;
		}

		buf[0]	= MSPRO_READ_ATRB;
		buf[1]	= 0;
		buf[2]	= 2;
		buf[3]	= 0;
		buf[4]	= 0;
		buf[5]	= 0;
		buf[6]	= (u8)(wstart >> 9);
		err = msi_pro_set_cmd_parameter(ms, buf);
		if (err) {
			TRACEX(("msi_pro_set_cmd_parameter: %x, XXXXXXXXXXXXXXXXXXXXXX", err));
			goto Exit;
		}

		for (i=0; i < 2; i++) {
			page.buf	= buf + (i * MS_PAGE_SIZE);
			err = ms_tpc_read_page_data(ms, &page);
			if (err) {
				TRACEX(("ms_tpc_read_page_data: %x, XXXXXXXXXXXXXXXXXXXXXXX", ms->int_resp));
				goto Exit;
			}
		}

		/*
		for (i=0; i< 1024; i++) {
			PRINT0(("%02x, ", buf[i]));
			if ((i & 0xf) == 0xf) {
				PRINT0(("\n"));
			}
		}
		PRINT0(("\n"));
		*/

		wstart &= 0x3ff;
	}

	/* Re-arrange the attribute in buffer0/1. Make the start of system information align to buffer0 */
	for (i=0; i < 96; i++) {
		buf[i] = buf[wstart + i];
	}

	/* Memory Stick Class -> Memory Stick PRO:2, EHC:3 */
	TRACEW(("System information: buf[0]: %x", buf[0]));

	if (buf[0] & 0x02) {
		if ((buf[0] != 0x02) && (mcard->bMsType & MEMORY_STICK_PRO)) {
			TRACEX(("MEMORY_STICK_PRO, buf[0] != 0x02, XXXXXXXXXXXXXXXXXXXXX"));
			err = ERR_ERROR;
			goto Exit;
		}
		else if((buf[0] != 0x03) && (mcard->bMsType & MEMORY_STICK_EHC)) {
			TRACEX(("MEMORY_STICK_EHC, buf[0] != 0x03, XXXXXXXXXXXXXXXXXXXXX"));
			err = ERR_ERROR;
			goto Exit;
		}
	}
	else {
		TRACEX(("buf[0] & 0x02 == 0, XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = ERR_ERROR;
		goto Exit;
	}

	/* Card Lock, define MS sub class of system information */
	if (buf[46] & 0xC0) {
		TRACEX(("buf[46] & 0xC0 == TRUE, %x, XXXXXXXXXXXXXXXXXXXXXXXXXXX", buf[46]));
		err = ERR_ERROR;
		goto Exit;
	}

	/* check EHC interface type */
	if (mcard->bMsType & MEMORY_STICK_EHC) {
		if (MEMORY_STICK_EHC_INTERFACE_TYPE_4BIT == (buf[51] & MEMORY_STICK_EHC_INTERFACE_TYPE)) {
			if (mcard->bMsType & MEMORY_STICK_PRO_HG) {
				mcard->bMsType &= ~MEMORY_STICK_PRO_HG;
			}
		}
	}

	if (buf[0x38] == 0) {
		/* Device type = 0, Read/Write */
	}
	else if (buf[0x38] & 0xFC) {
		TRACEX(("buf[0x38] & 0xFC == TRUE, %x, XXXXXXXXXXXXXXXXXXXXXXX", buf[0x38]));
		err = ERR_ERROR;
		goto Exit;
	}
	else
	{
		mcard->state |= MS_STATE_HW_READONLY;
		if (mcard->bMsType & MEMORY_STICK_PRO) {
			mcard->bMsType = MEMORY_STICK_PRO_ROM;
		}
		else {
			mcard->bMsType = MEMORY_STICK_EHC_ROM;
		}
	}

	if (mcard->bMsType & MEMORY_STICK_EHC) {
		TRACEW(("bMsType & MEMORY_STICK_EHC == TRUE"));
		mcard->total_lba =  ((u32)buf[6] << 24) |
							((u32)buf[7] << 16) |
							((u32)buf[8] <<  8) |
							((u32)buf[9]);

		blksize =	((u32)buf[32] << 24) |
					((u32)buf[33] << 16) |
					((u32)buf[34] <<  8) |
					((u32)buf[35]);
		TRACEW(("block: %x, blksize: %x", mcard->total_lba, blksize));
		mcard->total_lba *= blksize;
	}
	else {

		blksize			 = ((u32)buf[2] << 8) | buf[3];
		mcard->total_lba = ((u32)buf[6] << 8) | buf[7];

		mcard->total_lba *= blksize;
	}

	TRACEW(("mcard->total_lba: %x, (%d) MB", mcard->total_lba, mcard->total_lba / 2 / 1024));

	memcpy(ms->buf, buf, SECTOR_SIZE);

Exit:
	vfree(buf);
	return err;
}

