#ifndef __AMCR_GENERAL_H
#define __AMCR_GENERAL_H

#define ERR_SUCCESS		0
#define ERR_ERROR		1
#define ERR_NO_MEDIUM		2
#define ERR_TIME_OUT		3
#define ERR_OP_NO_SUPPORT	4
#define ERR_IO_ERR			5
#define	ERR_INVALID_PARAM	6


/* boolean value */
#define FALSE			0
#define TRUE			1





#define DMA_UNIT_SIZE		0x1000

#define SECTOR_SIZE		0x200


/* Dynamic bitflag definitions (us->dflags): used in set_bit() etc. */
#define US_FLIDX_URB_ACTIVE		0x01	/* current_urb is in use    */
#define US_FLIDX_SG_ACTIVE		0x02	/* current_sg is in use     */
#define US_FLIDX_ABORTING		0x04	/* abort is in progress     */
#define US_FLIDX_DISCONNECTING	0x08	/* disconnect in progress   */
#define US_FLIDX_RESETTING		0x10	/* device reset in progress */
#define US_FLIDX_TIMED_OUT		0x20	/* SCSI midlayer timed out  */
#define US_FLIDX_DONT_SCAN		0x40	/* don't scan (disconnect)  */



/*
 * Transport return codes
 */

#define TRANSPORT_GOOD	   	0
#define TRANSPORT_FAILED  	1
#define TRANSPORT_NO_SENSE 	2
#define TRANSPORT_ERROR   	3


enum xfer_buf_dir	{TO_XFER_BUF, FROM_XFER_BUF};


#define CARD_IS_PRESENT(media_card)				(media_card->media_state & MEDIA_STATE_PRESENT)
#define CARD_IS_WR_PROTECT(media_card)			(media_card->media_state & MEDIA_STATE_READONLY)
#define CARD_IS_CHANGE(media_card)				(media_card->media_state & MEDIA_STATE_CHANGE)
#define LBA_ADDRESS_OUT_OF_RANGE(media_card)	((media_card->lba + media_card->lba_len) > media_card->media_capacity)



#define LO	0
#define HI	1

#define LL	0
#define LH	1
#define HL	2
#define HH	3



#define MGfmt_FULL_FORMAT	0x10
#define MGfmt_QUICK_FORMAT	0x01
#define MGfmt_INACTIVE		0x00

// bFormatterState
#define FORMATTER_STATE_FORMAT_START			0x01
#define FORMATTER_STATE_FORMAT_IN_PROGRESS1 	0x02
#define FORMATTER_STATE_FORMAT_IN_PROGRESS2 	0x03
#define FORMATTER_STATE_FORMAT_IN_PROGRESS3 	0x04
#define FORMATTER_STATE_FORMAT_REINITIALIZE 	0x05
#define FORMATTER_STATE_FORMAT_COMPLETE			0x06
#define FORMATTER_STATE_FORMAT_COMMAND_FAILED	0x81


/* led_state */
#define LED_CARD_REMOVED	0x00
#define LED_CARD_INISERTED	0x01
#define LED_CARD_IO			0x02


/*****************************************************************************/
typedef union {
	u32  dword;
	u16  word[2];
	u8   byte[4];

} DWORDB;


typedef union {
	u16  word;
	u8   byte[2];

} WORDB;






/* card_type */
#define SD_CARD_BIT			0x01
#define MS_CARD_BIT			0x08
#define XD_CARD_BIT			0x10


#define CARD_MIN_CLOCK			1

/* common register */
#define CARD_DMA_ADDRESS		0x00

// 6601
#define CARD_DMA_BOUNDARY		0x05
// 6621
#define	CARD_DMA_PAGE_CNT       0x05

#define CARD_BUFFER_PORT			0x08

#define CARD_DMA_CTRL			0x0c
#define		CARD_DMA_ENABLE			0x01

#define CARD_XFER_LENGTH		0x6c


#define CARD_TIME_OUT_CTRL		0x69

#define CARD_POWER_CONTROL		0x70

#define CARD_CLK_SELECT			0x72
#define		CARD_CLK_ENABLE			0x01
#define		CARD_CLK_X2_MODE		0x02
#define		CARD_CLK_EXT_PLL		0x04

#define		CARD_CLK_31_25_MHZ		0x00
#define		CARD_CLK_48_MHZ			0x10
#define		CARD_CLK_125_MHZ		0x20
#define		CARD_CLK_384_MHZ		0x30
#define	    CARD_CLK_OVER_CLK		0x80

#define CARD_CLK_DIVIDER			0x73

#define CARD_INTERFACE_MODE_CTRL	0x74
#define		CARD_DLINK_MODE				0x80
#define		INTERRUPT_DELAY_TIME		0x40
#define		CARD_SIGNAL_REQ_CTRL		0x30

#define CARD_ACTIVE_CTRL		0x75

#define CARD_DETECT_STATUS		0x76
#define		CARD_DETECT_ENABLE		0x80


#define CARD_SOFTWARE_RESET		0x79
#define		BUFFER_CTRL_RESET		0x80

#define CARD_OUTPUT_ENABLE		0x7a

#define CARD_PAD_DRIVE0			0x7b
#define CARD_PAD_DRIVE1			0x7c
#define CARD_PAD_DRIVE2			0x7d


#define CHIP_FUNCTION           0x7f 





u32 am_swap_u32(u32 i);


int card_active_ctrl(struct _DEVICE_EXTENSION *pdx, u8 value);
int card_reset(struct _DEVICE_EXTENSION	*pdx, u8 card_type);
int card_power_ctrl(struct _DEVICE_EXTENSION *pdx, u8 card_type, u8 pwr_on);
int card_set_clock(struct _DEVICE_EXTENSION *pdx, u32 clock);

void card_init_interface_mode_ctrl(struct _DEVICE_EXTENSION *pdx);

#endif
