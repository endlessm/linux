#include <linux/vmalloc.h>
#include "amcr_stor.h"


/***********************************************************************
 * Scatter-gather transfer buffer access routines
 ***********************************************************************/

/* Copy a buffer of length buflen to/from the srb's transfer buffer.
 * (Note: for scatter-gather transfers (srb->use_sg > 0), srb->request_buffer
 * points to a list of s-g entries and we ignore srb->request_bufflen.
 * For non-scatter-gather transfers, srb->request_buffer points to the
 * transfer buffer itself and srb->request_bufflen is the buffer's length.)
 * Update the *index and *offset variables so that the next copy will
 * pick up from where this one left off. */

unsigned int scsi_access_xfer_buf(unsigned char *buffer,
	unsigned int buflen, struct scsi_cmnd *srb, unsigned int *index,
	unsigned int *offset, enum xfer_buf_dir dir)
{
	unsigned int cnt;

	/* If not using scatter-gather, just transfer the data directly.
	 * Make certain it will fit in the available buffer space. */
	if (scsi_sg_count(srb) == 0) {
		if (*offset >= scsi_bufflen(srb))
			return 0;
		cnt = min(buflen, scsi_bufflen(srb) - *offset);
		if (dir == TO_XFER_BUF)
			memcpy((unsigned char *) scsi_sglist(srb) + *offset,
					buffer, cnt);
		else
			memcpy(buffer, (unsigned char *) scsi_sglist(srb) +
					*offset, cnt);
		*offset += cnt;

	/* Using scatter-gather.  We have to go through the list one entry
	 * at a time.  Each s-g entry contains some number of pages, and
	 * each page has to be kmap()'ed separately.  If the page is already
	 * in kernel-addressable memory then kmap() will return its address.
	 * If the page is not directly accessible -- such as a user buffer
	 * located in high memory -- then kmap() will map it to a temporary
	 * position in the kernel's virtual address space. */
	} else {
		struct scatterlist *sg =
				(struct scatterlist *) scsi_sglist(srb)
				+ *index;

		/* This loop handles a single s-g list entry, which may
		 * include multiple pages.  Find the initial page structure
		 * and the starting offset within the page, and update
		 * the *offset and *index values for the next loop. */
		cnt = 0;
		while (cnt < buflen && *index < scsi_sg_count(srb)) {
			struct page *page = sg_page(sg) +
					((sg->offset + *offset) >> PAGE_SHIFT);
			unsigned int poff =
					(sg->offset + *offset) & (PAGE_SIZE-1);
			unsigned int sglen = sg->length - *offset;

			if (sglen > buflen - cnt) {


				sglen = buflen - cnt;
				*offset += sglen;
			} else {


				*offset = 0;
				++*index;
				++sg;
			}

			/* Transfer the data for all the pages in this
			 * s-g entry.  For each page: call kmap(), do the
			 * transfer, and call kunmap() immediately after. */
			while (sglen > 0) {
				unsigned int plen = min(sglen, (unsigned int)
						PAGE_SIZE - poff);
				unsigned char *ptr = kmap(page);

				if (dir == TO_XFER_BUF)
					memcpy(ptr + poff, buffer + cnt, plen);
				else
					memcpy(buffer + cnt, ptr + poff, plen);
				kunmap(page);


				poff = 0;
				++page;
				cnt += plen;
				sglen -= plen;
			}
		}
	}


	return cnt;
}


/* Store the contents of buffer into srb's transfer buffer and set the
* SCSI residue. */
void scsi_set_xfer_buf(unsigned char *buffer, unsigned int buflen, struct scsi_cmnd *srb)
{
	unsigned int index = 0, offset = 0;

	scsi_access_xfer_buf(buffer, buflen, srb, &index, &offset,
	   TO_XFER_BUF);
	if (buflen < scsi_bufflen(srb)) {
		scsi_set_resid(srb, scsi_bufflen(srb) - buflen);
	}

}

void scsi_get_xfer_buf(unsigned char *buffer, unsigned int buflen, struct scsi_cmnd *srb)
{
	unsigned int index = 0, offset = 0;

	scsi_access_xfer_buf(buffer, buflen, srb, &index, &offset,
	       FROM_XFER_BUF);
	if (buflen < scsi_bufflen(srb)) {
	   scsi_set_resid(srb, scsi_bufflen(srb) - buflen);
	}
}



void scsi_show_command(struct scsi_cmnd *srb)
{
	char *what = NULL;
	int i, unknown_cmd = 0;

	switch (srb->cmnd[0]) {
	case TEST_UNIT_READY: what = "TEST_UNIT_READY"; break;
	case REZERO_UNIT: what = "REZERO_UNIT"; break;
	case REQUEST_SENSE: what = "REQUEST_SENSE"; break;
	case FORMAT_UNIT: what = "FORMAT_UNIT"; break;
	case READ_BLOCK_LIMITS: what = "READ_BLOCK_LIMITS"; break;
	case REASSIGN_BLOCKS: what = "REASSIGN_BLOCKS"; break;
	case READ_6: what = "READ_6"; break;
	case WRITE_6: what = "WRITE_6"; break;
	case SEEK_6: what = "SEEK_6"; break;
	case READ_REVERSE: what = "READ_REVERSE"; break;
	case WRITE_FILEMARKS: what = "WRITE_FILEMARKS"; break;
	case SPACE: what = "SPACE"; break;
	case INQUIRY: what = "INQUIRY"; break;
	case RECOVER_BUFFERED_DATA: what = "RECOVER_BUFFERED_DATA"; break;
	case MODE_SELECT: what = "MODE_SELECT"; break;
	case RESERVE: what = "RESERVE"; break;
	case RELEASE: what = "RELEASE"; break;
	case COPY: what = "COPY"; break;
	case ERASE: what = "ERASE"; break;
	case MODE_SENSE: what = "MODE_SENSE"; break;
	case START_STOP: what = "START_STOP"; break;
	case RECEIVE_DIAGNOSTIC: what = "RECEIVE_DIAGNOSTIC"; break;
	case SEND_DIAGNOSTIC: what = "SEND_DIAGNOSTIC"; break;
	case ALLOW_MEDIUM_REMOVAL: what = "ALLOW_MEDIUM_REMOVAL"; break;
	case SET_WINDOW: what = "SET_WINDOW"; break;
	case READ_CAPACITY: what = "READ_CAPACITY"; break;
	case READ_10: what = "READ_10"; break;
	case WRITE_10: what = "WRITE_10"; break;
	case SEEK_10: what = "SEEK_10"; break;
	case WRITE_VERIFY: what = "WRITE_VERIFY"; break;
	case VERIFY: what = "VERIFY"; break;
	case SEARCH_HIGH: what = "SEARCH_HIGH"; break;
	case SEARCH_EQUAL: what = "SEARCH_EQUAL"; break;
	case SEARCH_LOW: what = "SEARCH_LOW"; break;
	case SET_LIMITS: what = "SET_LIMITS"; break;
	case READ_POSITION: what = "READ_POSITION"; break;
	case SYNCHRONIZE_CACHE: what = "SYNCHRONIZE_CACHE"; break;
	case LOCK_UNLOCK_CACHE: what = "LOCK_UNLOCK_CACHE"; break;
	case READ_DEFECT_DATA: what = "READ_DEFECT_DATA"; break;
	case MEDIUM_SCAN: what = "MEDIUM_SCAN"; break;
	case COMPARE: what = "COMPARE"; break;
	case COPY_VERIFY: what = "COPY_VERIFY"; break;
	case WRITE_BUFFER: what = "WRITE_BUFFER"; break;
	case READ_BUFFER: what = "READ_BUFFER"; break;
	case UPDATE_BLOCK: what = "UPDATE_BLOCK"; break;
	case READ_LONG: what = "READ_LONG"; break;
	case WRITE_LONG: what = "WRITE_LONG"; break;
	case CHANGE_DEFINITION: what = "CHANGE_DEFINITION"; break;
	case WRITE_SAME: what = "WRITE_SAME"; break;
	case GPCMD_READ_SUBCHANNEL: what = "READ SUBCHANNEL"; break;
	case READ_TOC: what = "READ_TOC"; break;
	case GPCMD_READ_HEADER: what = "READ HEADER"; break;
	case GPCMD_PLAY_AUDIO_10: what = "PLAY AUDIO (10)"; break;
	case GPCMD_PLAY_AUDIO_MSF: what = "PLAY AUDIO MSF"; break;
	case GPCMD_GET_EVENT_STATUS_NOTIFICATION:
		what = "GET EVENT/STATUS NOTIFICATION"; break;
	case GPCMD_PAUSE_RESUME: what = "PAUSE/RESUME"; break;
	case LOG_SELECT: what = "LOG_SELECT"; break;
	case LOG_SENSE: what = "LOG_SENSE"; break;
	case GPCMD_STOP_PLAY_SCAN: what = "STOP PLAY/SCAN"; break;
	case GPCMD_READ_DISC_INFO: what = "READ DISC INFORMATION"; break;
	case GPCMD_READ_TRACK_RZONE_INFO:
		what = "READ TRACK INFORMATION"; break;
	case GPCMD_RESERVE_RZONE_TRACK: what = "RESERVE TRACK"; break;
	case GPCMD_SEND_OPC: what = "SEND OPC"; break;
	case MODE_SELECT_10: what = "MODE_SELECT_10"; break;
	case GPCMD_REPAIR_RZONE_TRACK: what = "REPAIR TRACK"; break;
	case 0x59: what = "READ MASTER CUE"; break;
	case MODE_SENSE_10: what = "MODE_SENSE_10"; break;
	case GPCMD_CLOSE_TRACK: what = "CLOSE TRACK/SESSION"; break;
	case 0x5C: what = "READ BUFFER CAPACITY"; break;
	case 0x5D: what = "SEND CUE SHEET"; break;
	case GPCMD_BLANK: what = "BLANK"; break;
	case REPORT_LUNS: what = "REPORT LUNS"; break;
	case MOVE_MEDIUM: what = "MOVE_MEDIUM or PLAY AUDIO (12)"; break;
	case READ_12: what = "READ_12"; break;
	case WRITE_12: what = "WRITE_12"; break;
	case WRITE_VERIFY_12: what = "WRITE_VERIFY_12"; break;
	case SEARCH_HIGH_12: what = "SEARCH_HIGH_12"; break;
	case SEARCH_EQUAL_12: what = "SEARCH_EQUAL_12"; break;
	case SEARCH_LOW_12: what = "SEARCH_LOW_12"; break;
	case SEND_VOLUME_TAG: what = "SEND_VOLUME_TAG"; break;
	case READ_ELEMENT_STATUS: what = "READ_ELEMENT_STATUS"; break;
	case GPCMD_READ_CD_MSF: what = "READ CD MSF"; break;
	case GPCMD_SCAN: what = "SCAN"; break;
	case GPCMD_SET_SPEED: what = "SET CD SPEED"; break;
	case GPCMD_MECHANISM_STATUS: what = "MECHANISM STATUS"; break;
	case GPCMD_READ_CD: what = "READ CD"; break;
	case 0xE1: what = "WRITE CONTINUE"; break;
	case WRITE_LONG_2: what = "WRITE_LONG_2"; break;
	default: what = "(unknown command)"; unknown_cmd = 1; break;
	}

	if (srb->cmnd[0] != TEST_UNIT_READY) {
		TRACE1(("Command: %s (%d bytes)", what, srb->cmd_len));
	}

	if (unknown_cmd) {
		for (i = 0; i < srb->cmd_len && i < 16; i++) {
			DEBUGPN(("%02x, ", srb->cmnd[i]));
		}
		DEBUGPN(("\n"));
	}
}


/*************************************************************************************************/
void scsi_set_request_sense(struct _DEVICE_EXTENSION *pdx, u32 uChipState)
{
	u8 bSenseKey = 0, bAsc = 0, bAscq = 0;
	u8 abRequestSense[18] =
		{0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if (uChipState != CHIP_STATE_ERROR_MEDIUM_NOT_PRESENT) {
		TRACEX(("scsi_set_request_sense ===>, uChipState: %x", uChipState));
	}

	uChipState &= 0xFF000000;

	switch( uChipState )
	{
	case CHIP_STATE_ERROR_INVALID_CBW_CMD:
		TRACEX(("CHIP_STATE_ERROR_INVALID_CBW_CMD"));
		bSenseKey = SENSE_KEY_INVALID_CBW_CMD;
		bAsc = ASC_INVALID_CBW_CMD;
		bAscq = ASCQ_INVALID_CBW_CMD;
		break;

	case CHIP_STATE_ERROR_INVALID_SCSI_CMD:
		TRACEX(("CHIP_STATE_ERROR_INVALID_SCSI_CMD"));
		bSenseKey = SENSE_KEY_INVALID_SCSI_CMD;
		bAsc = ASC_INVALID_SCSI_CMD;
		bAscq = ASCQ_INVALID_SCSI_CMD;
		break;

	case CHIP_STATE_ERROR_SCSI_PARAMETER_VALUE_INVALID:
		TRACEX(("CHIP_STATE_ERROR_SCSI_PARAMETER_VALUE_INVALID"));
		bSenseKey = SENSE_KEY_SCSI_PARAMETER_VALUE_INVALID;
		bAsc = ASC_SCSI_PARAMETER_VALUE_INVALID;
		bAscq = ASCQ_SCSI_PARAMETER_VALUE_INVALID;
		break;

	case CHIP_STATE_ERROR_MEDIUM_NOT_PRESENT:
		TRACE1(("CHIP_STATE_ERROR_MEDIUM_NOT_PRESENT"));
		bSenseKey = SENSE_KEY_MEDIUM_NOT_PRESENT;
		bAsc = ASC_MEDIUM_NOT_PRESENT;
		bAscq = ASCQ_MEDIUM_NOT_PRESENT;
		break;

	case CHIP_STATE_ERROR_MEDIUM_CHANGE:
		TRACEX(("CHIP_STATE_ERROR_MEDIUM_CHANGE"));
		bSenseKey = SENSE_KEY_MEDIUM_CHANGE;
		bAsc = ASC_MEDIUM_CHANGE;
		bAscq = ASCQ_MEDIUM_CHANGE;
		break;

	case CHIP_STATE_ERROR_MEDIUM_WRITE_PROTECT:
		TRACEX(("CHIP_STATE_ERROR_MEDIUM_WRITE_PROTECT"));
		bSenseKey = SENSE_KEY_MEDIUM_WRITE_PROTECT;
		bAsc = ASC_MEDIUM_WRITE_PROTECT;
		bAscq = ASCQ_MEDIUM_WRITE_PROTECT;
		break;

	case CHIP_STATE_ERROR_MEDIUM_BUSY:
		TRACEX(("CHIP_STATE_ERROR_MEDIUM_BUSY"));
		bSenseKey = SENSE_KEY_MEDIUM_BUSY;
		bAsc = ASC_MEDIUM_BUSY;
		bAscq = ASCQ_MEDIUM_BUSY;
		break;

	case CHIP_STATE_ERROR_MEDIUM_ERROR:
		TRACEX(("CHIP_STATE_ERROR_MEDIUM_ERROR"));
		bSenseKey = SENSE_KEY_MEDIUM_ERROR;
		bAsc = ASC_MEDIUM_ERROR;
		bAscq = ASCQ_MEDIUM_ERROR;
		break;

	case CHIP_STATE_ERROR_ADDRESS_OUT_OF_RANGE:
		TRACEX(("CHIP_STATE_ERROR_ADDRESS_OUT_OF_RANGE"));
		bSenseKey = SENSE_KEY_ADDRESS_OUT_OF_RANGE;
		bAsc = ASC_ADDRESS_OUT_OF_RANGE;
		bAscq = ASCQ_ADDRESS_OUT_OF_RANGE;
		break;

	case CHIP_STATE_ERROR_UNRECOVERED_READ_ERROR:
		bSenseKey = SENSE_KEY_UNRECOVERED_READ_ERROR;
		bAsc = ASC_UNRECOVERED_READ_ERROR;
		bAscq = ASCQ_UNRECOVERED_READ_ERROR;

	case CHIP_STATE_ERROR_INVALID_FIELD_IN_COMMAND_PACKET:
		TRACEX(("CHIP_STATE_ERROR_INVALID_FIELD_IN_COMMAND_PACKET"));
		bSenseKey = SENSE_KEY_INVALID_FIELD_IN_COMMAND_PACKET;
		bAsc = ASC_INVALID_FIELD_IN_COMMAND_PACKET;
		bAscq = ASCQ_INVALID_FIELD_IN_COMMAND_PACKET;
		break;

	case CHIP_STATE_ERROR_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS:
		TRACEX(("CHIP_STATE_ERROR_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS"));
		bSenseKey = SENSE_KEY_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS;
		bAsc = ASC_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS;
		bAscq = ASCQ_LOGICAL_UNIT_NOT_READY_FORMAT_IN_PROGRESS;
		break;

	case CHIP_STATE_ERROR_FORMAT_COMMAND_FAILED:
		TRACEX(("CHIP_STATE_ERROR_FORMAT_COMMAND_FAILED"));
		bSenseKey = SENSE_KEY_FORMAT_COMMAND_FAILED;
		bAsc = ASC_FORMAT_COMMAND_FAILED;
		bAscq = ASCQ_FORMAT_COMMAND_FAILED;
		break;

	case CHIP_STATE_ERROR_COPY_PROTECTION_KEY_EXCHANGE_AUTHENTICATION_FAILURE:
		TRACEX(("CHIP_STATE_ERROR_COPY_PROTECTION_KEY_EXCHANGE_AUTHENTICATION_FAILURE"));
		bSenseKey = SENSE_KEY_PROTECTION_KEY_EXCHANGE_AUTHENTICATION_FAILURE;
		bAsc = ASC_KEY_EXCHANGE_AUTHENTICATION_FAILURE;
		bAscq = ASCQ_KEY_EXCHANGE_AUTHENTICATION_FAILURE;
		break;

	case CHIP_STATE_ERROR_INCOMPATIBLE_MEDIUM_INSTALLED:
		TRACEX(("CHIP_STATE_ERROR_INCOMPATIBLE_MEDIUM_INSTALLED"));
		bSenseKey = SENSE_KEY_INCOMPATIBLE_MEDIUM_INSTALLED;
		bAsc = ASC_KEY_INCOMPATIBLE_MEDIUM_INSTALLED;
		bAscq = ASCQ_KEY_INCOMPATIBLE_MEDIUM_INSTALLED;
		break;


	default:
		TRACEX(("Unkonwn uChipState: %x, XXXXXXXXXXXXXXXXXXXXXXX", uChipState));
		break;
	}

	abRequestSense[2]  = 0x0F & bSenseKey;
	abRequestSense[12] = bAsc;
	abRequestSense[13] = bAscq;

	memcpy(&pdx->sense_buffer, abRequestSense, sizeof(abRequestSense));
	return;

}

/*************************************************************************************************/
int scsi_media_status(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	struct _MEDIA_CARD *media_card = &pdx->media_card;

	if (!CARD_IS_PRESENT(media_card)) {
		scsi_set_request_sense(pdx, CHIP_STATE_ERROR_MEDIUM_NOT_PRESENT);
		return TRANSPORT_FAILED;
	}

	if (CARD_IS_CHANGE(media_card)) {
		media_card->media_state &= ~MEDIA_STATE_CHANGE;
		scsi_set_request_sense(pdx, CHIP_STATE_ERROR_MEDIUM_CHANGE);
		return TRANSPORT_FAILED;
	}

	return TRANSPORT_GOOD;
}

/*************************************************************************************************/
int scsi_inquiry(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	u8 sendbytes;
	char *buf;
	char *inquiry_string = (char *)"Generic-SD/MS Reader    1.00 ";


	u8 inquiry_buf[] =
	{
		0x00,
		0x80,
		0x00,
		0x00,

		0x1f,
		0x00,
		0x00,
		0x00
	};


	buf = vmalloc(scsi_bufflen(srb));
	if (buf == NULL) {
		return TRANSPORT_ERROR;
	}

	if (scsi_bufflen(srb) < 36) {
		sendbytes = (u8)(scsi_bufflen(srb));
	} else {
		sendbytes = 36;
	}


	if (sendbytes > 8) {
		memcpy(buf, inquiry_buf, 8);
		memcpy(buf + 8, inquiry_string,	sendbytes - 8);
	} else {
		memcpy(buf, inquiry_buf, sendbytes);
	}

	scsi_set_xfer_buf(buf, scsi_bufflen(srb), srb);

	vfree(buf);
	return TRANSPORT_GOOD;
}

/*************************************************************************************************/
int scsi_testunit_ready(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	return scsi_media_status(srb, pdx);
}

/*************************************************************************************************/
int scsi_mode_sense6(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int		err;
	struct _MEDIA_CARD *media_card = &pdx->media_card;
	u8 buf[] = {0x03, 0x00, 0x00, 0x00};
	int len;

	err = scsi_media_status(srb, pdx);
	if (err) {
		scsi_set_resid(srb, scsi_bufflen(srb));
		return err;
	}

	if (CARD_IS_WR_PROTECT(media_card)) {
		buf[2] = 0x80;
	}

	len = min((int)scsi_bufflen(srb), (int)sizeof(buf));
	scsi_set_xfer_buf(buf, len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - len);
	return TRANSPORT_GOOD;
}

/*************************************************************************************************/
int scsi_mode_sense10(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int		err;
	struct _MEDIA_CARD *media_card = &pdx->media_card;
	u8 buf[] = {0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int len;

	err = scsi_media_status(srb, pdx);
	if (err) {
		scsi_set_resid(srb, scsi_bufflen(srb));
		return err;
	}

	if (CARD_IS_WR_PROTECT(media_card)) {
		buf[3] = 0x80;
	}

	len = min((int)scsi_bufflen(srb), (int)sizeof(buf));
	scsi_set_xfer_buf(buf, len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - len);
	return TRANSPORT_GOOD;
}


/*************************************************************************************************/
int scsi_prevent_allow_removal(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	/* The reader supports no locking mechanism */
	if (srb->cmnd[4] & PREVENT_REMOVAL) {
		scsi_set_request_sense(pdx, CHIP_STATE_ERROR_SCSI_PARAMETER_VALUE_INVALID);
		return TRANSPORT_FAILED;
	}

	return TRANSPORT_GOOD;
}

/*************************************************************************************************/
int scsi_start_stop_unit(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	struct _MEDIA_CARD *media_card = &pdx->media_card;

	scsi_set_resid(srb, scsi_bufflen(srb));

	if ((srb->cmnd[4] & LOEJ_START) == UNLOAD_MEDIUM) {
		TRACE1(("UNLOAD_MEDIUM"));
		media_card->media_state &= ~MEDIA_STATE_PRESENT;
	}
	return TRANSPORT_GOOD;
}


/*************************************************************************************************/
int scsi_request_sense(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	u8 buf[18] =
		{0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	int len;


	len = min((int)scsi_bufflen(srb), (int)sizeof(buf));
	scsi_set_xfer_buf(buf, len, srb);
	scsi_set_resid(srb, scsi_bufflen(srb) - len);
	return TRANSPORT_GOOD;
}

/*************************************************************************************************/
int scsi_read_format_capacity(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int err;
	u8 *buf;
	unsigned int buf_len;
	u32 media_capacity;

	TRACE1(("scsi_read_format_capacity ===>"));

	err = scsi_media_status(srb, pdx);
	if (err) {
		scsi_set_resid(srb, scsi_bufflen(srb));
		return err;
	}

	buf_len = 20;
	buf = kmalloc(buf_len, GFP_KERNEL);
	if (buf == NULL) {
		TRACEX(("kmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return TRANSPORT_ERROR;
	}

	media_capacity = pdx->media_card.media_capacity;

	memset(buf, 0, buf_len);
	buf[3] = 0x10;

	buf[4] = buf[12] = (unsigned char)(media_capacity >> 24);
	buf[5] = buf[13] = (unsigned char)(media_capacity >> 16);
	buf[6] = buf[14] = (unsigned char)(media_capacity >> 8);
	buf[7] = buf[15] = (unsigned char)(media_capacity);

	buf[8] = buf[10] = buf[18] = 0x02;

	buf_len = min(scsi_bufflen(srb), buf_len);
	scsi_set_xfer_buf(buf, buf_len, srb);
	kfree(buf);

	scsi_set_resid(srb, scsi_bufflen(srb) - buf_len);

	return TRANSPORT_GOOD;
}

static int scsi_read_capacity(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	u8 *buf;
	unsigned int buf_len;
	int err;
	u32 media_capacity;

	TRACE1(("scsi_read_capacity ===>"));

	err = scsi_media_status(srb, pdx);
	if (err) {
		return err;
	}

	buf_len = 8;
	buf = kmalloc(buf_len, GFP_KERNEL);
	if (buf == NULL) {
		TRACEX(("kmalloc, XXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		return TRANSPORT_ERROR;
	}

	media_capacity = pdx->media_card.media_capacity;
	media_capacity--;
	buf[0] = (unsigned char)((media_capacity) >> 24);
	buf[1] = (unsigned char)((media_capacity) >> 16);
	buf[2] = (unsigned char)((media_capacity) >> 8);
	buf[3] = (unsigned char) (media_capacity);

	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = 0x02;
	buf[7] = 0x00;

	buf_len = min(scsi_bufflen(srb), buf_len);
	scsi_set_xfer_buf(buf, scsi_bufflen(srb), srb);

	kfree(buf);

	scsi_set_resid(srb, scsi_bufflen(srb) - buf_len);
	return TRANSPORT_GOOD;
}

int scsi_read_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int	err = TRANSPORT_GOOD;
	u32	lba, lba_len;

	lba = lba_len = 0;


	err = scsi_media_status(srb, pdx);
	if (err) {
		return err;
	}

	if ((srb->cmnd[0] == READ_10) || (srb->cmnd[0] == WRITE_10)) {
		lba = ((u32)srb->cmnd[2] << 24) | ((u32)srb->cmnd[3] << 16) |
			  ((u32)srb->cmnd[4] << 8)  | ((u32)srb->cmnd[5]);
		lba_len = ((u32)(srb->cmnd[7]) << 8) | srb->cmnd[8];

	} else if ((srb->cmnd[0] == READ_6) || (srb->cmnd[0] == WRITE_6)) {
		lba = ((u32)(srb->cmnd[1] & 0x1F) << 16) |
			  ((u32)srb->cmnd[2] << 8) | ((u32)srb->cmnd[3]);
		lba_len = srb->cmnd[4];
	}

	TRACE1(("scsi_read_write, lba: %x, lba_len: %x", lba, lba_len));

	if (pdx->uExtDynamicAspm) {
		pci_aspm_ctrl(pdx, 0);
	}

	switch (pdx->media_card.card_type & 0x0f) {
	case SD_CARD_TYPE:
		err = sd_scsi_read_write(srb, pdx, lba, lba_len);
		break;

	case MS_CARD_TYPE:
		err = ms_scsi_thread(srb, pdx, lba, lba_len);
		break;

	default:
		err = TRANSPORT_FAILED;
		break;

	}

	if (pdx->uExtDynamicAspm) {
		pci_aspm_ctrl(pdx, 1);
	}

	if (err) {
		TRACEX(("sd_scsi_read_write, XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
		err = TRANSPORT_FAILED;
	}
	else {
		err = TRANSPORT_GOOD;
	}

	scsi_set_resid(srb, 0);
	return err;
}


int scsi_handler(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int result;


	switch (srb->cmnd[0]) {
		case READ_10:
		case WRITE_10:
		case READ_6:
		case WRITE_6:
			result = scsi_read_write(srb, pdx);
			break;

		case TEST_UNIT_READY:
			result = scsi_testunit_ready(srb, pdx);
			sd_stop_last_cmd(pdx->sd);
			break;

		case INQUIRY:
			result = scsi_inquiry(srb, pdx);
			break;

		case READ_CAPACITY:
			result = scsi_read_capacity(srb, pdx);
			break;

		case 0x23:
			result = scsi_read_format_capacity(srb, pdx);
			break;

		case START_STOP:
			result = scsi_start_stop_unit(srb, pdx);
			break;

		case ALLOW_MEDIUM_REMOVAL:
			result = scsi_prevent_allow_removal(srb, pdx);
			break;

		case REQUEST_SENSE:
			result = scsi_request_sense(srb, pdx);
			break;

		case MODE_SENSE:
			result = scsi_mode_sense6(srb, pdx);
			break;

		case MODE_SENSE_10:
			result = scsi_mode_sense10(srb, pdx);
			break;

		case FORMAT_UNIT:
		case MODE_SELECT:
		case VERIFY:
			result = TRANSPORT_GOOD;
			break;

		default:
			scsi_set_request_sense(pdx, CHIP_STATE_ERROR_INVALID_SCSI_CMD);
			result = TRANSPORT_FAILED;
			break;
	}

	return result;
}

/* Invoke the transport and basic error-handling/recovery methods
 *
 * This is used to send the message to the device and receive the response.
 */
void amcr_invoke_transport(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx)
{
	int result;

	scsi_set_resid(srb, 0);
	result = scsi_handler(srb, pdx);

	/* if the command gets aborted by the higher layers, we need to
	 * short-circuit all other processing
	 */
	if (US_FLIDX_ABORTING & pdx->dflags) {
		TRACEX(("pdx->dflags & US_FLIDX_ABORTING, XXXXXXXXXXXXXXXXXXXXXXXXXX"));
		srb->result = DID_ABORT << 16;
		goto ERR_EXIT;
	}


	if (result == TRANSPORT_ERROR) {
		TRACEX(("scsi_handler == TRANSPORT_ERROR, XXXXXXXXXXXXXXXXXXXXXXXXXX"));
		srb->result = DID_ERROR << 16;
		goto ERR_EXIT;
	}

	srb->result = SAM_STAT_GOOD;

	/*
	 * If we have a failure, we're going to do a REQUEST_SENSE
	 * automatically.  Note that we differentiate between a command
	 * "failure" and an "error" in the transport mechanism.
	 */
	if (result == TRANSPORT_FAILED) {
		srb->result = SAM_STAT_CHECK_CONDITION;
		memcpy(srb->sense_buffer, (unsigned char *)&(pdx->sense_buffer),
				sizeof(struct sense_data_t));
	}

	return;

	/* Error and abort processing: try to resynchronize with the device
	 * by issuing a port reset.  If that fails, try a class-specific
	 * device reset. */
ERR_EXIT:
	return;
}
