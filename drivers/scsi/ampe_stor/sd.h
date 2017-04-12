#if !defined(__AMCR_SD_H_)
/*************************************************************************************************/
#define __AMCR_SD_H_


/*==========================================================
 * Controller registers
 *=========================================================*/
#define SD_DMA_ADDRESS			CARD_DMA_ADDRESS

#define SD_DMA_BOUNDARY			CARD_DMA_BOUNDARY

#define SD_BUFFER_PORT			CARD_BUFFER_PORT

#define SD_DMA_CTRL				CARD_DMA_CTRL
#define		MS_DMA_ENABLE			CARD_DMA_ENABLE

#define SD_XFER_LENGTH			CARD_XFER_LENGTH

#define SD_POWER_CONTROL		CARD_POWER_CONTROL


#define SD_CARD_ACTIVE_CTRL		CARD_ACTIVE_CTRL

#define SD_SOFTWARE_RESET		CARD_SOFTWARE_RESET

#define SD_OUTPUT_ENABLE		CARD_OUTPUT_ENABLE


#define SD_COMMAND			0x20

#define SD_RESPONSE			0x30


#define SD_CARD_WRITE_PROTECT	0x74

#define SD_STATUS_CHK			0x80

#define SD_CMD_XFER_CTRL		0x81
#define		SD_CMD_17_BYTE_CRC		0xc0
#define		SD_CMD_6_BYTE_WO_CRC	0x80
#define		SD_CMD_6_BYTE_CRC		0x40
#define		SD_CMD_NO_RESP			0x00
#define		SD_CMD_START_XFER		0x20
#define		SD_CMD_STOP_WAIT_RDY	0x10


#define SD_BUS_MODE_CTRL		0x82
#define		SD_BUS_4_BIT_MODE		0x20
#define		SD_BUS_8_BIT_MODE		0x10
#define		SD_BUS_1_BIT_MODE		0x00



#define SD_DATA_XFER_CTRL		0x83
#define		SD_DATA_WRITE			0x80
#define		SD_DATA_DMA_MODE		0x40
#define		SD_DATA_START_XFER		0x01

#define SD_DATA_PIN_STATE		0x84
#define		SD_DATA_LINE_LEVEL		0x0f

#define SD_OPT					0x85
#define		SD_OPT_CMD_LINE_LEVEL	0x80
#define		SD_OPT_NCRC_16_CLK		0x10
#define		SD_OPT_CMD_NWT			0x08
#define		SD_OPT_STOP_CLK			0x04
#define		SD_OPT_DDR_MODE			0x02
#define		SD_OPT_SD_18V			0x01

#define SD_CLK_DELAY				0x86
#define		SD_CLK_DATA_POSITIVE_EDGE	0x80
#define		SD_CLK_CMD_POSITIVE_EDGE	0x40

/*************************************************************/
#define SDHCI_SOFTWARE_RESET	0x2F
#define  SDHCI_RESET_ALL	0x01
#define  SDHCI_RESET_CMD	0x02
#define  SDHCI_RESET_DATA	0x04
/*************************************************************/

#define SD_INT_STATUS	0x90
#define SD_INT_ENABLE	0x94
#define  SD_INT_CMD_END				0x00000001
#define  SD_INT_DATA_END			0x00000002
#define  SD_INT_DMA_END				0x00000008
#define  SD_INT_WRITE_BUF_RDY		0x00000010
#define  SD_INT_READ_BUF_RDY		0x00000020
#define  SD_INT_CARD_REMOVE			0x00000040
#define  SD_INT_CARD_INSERT			0x00000080

#define  SD_INT_ERROR				0x00008000

#define  SD_INT_CMD_TIMEOUT_ERR		0x00010000
#define  SD_INT_CMD_CRC_ERR			0x00020000
#define  SD_INT_CMD_END_BIT_ERR		0x00040000
#define  SD_INT_CMD_INDEX_ERR		0x00080000

#define  SD_INT_DATA_TIMEOUT_ERR	0x00100000
#define  SD_INT_DATA_CRC_ERR		0x00200000
#define  SD_INT_DATA_END_BIT_ERR	0x00400000

#define  SD_INT_OVER_CURRENT_ERR	0x00000100


#define  SD_INT_NORMAL_MASK			0x00007FFF
#define  SD_INT_ERROR_MASK			0xFFFF8000

#define  SD_INT_CMD_MASK	(SD_INT_CMD_END | SD_INT_CMD_TIMEOUT_ERR | \
		 SD_INT_CMD_CRC_ERR | SD_INT_CMD_END_BIT_ERR | SD_INT_CMD_INDEX_ERR)

#define  SD_INT_DATA_MASK	(SD_INT_DATA_END | SD_INT_DMA_END | \
		 SD_INT_READ_BUF_RDY | SD_INT_WRITE_BUF_RDY | \
		 SD_INT_DATA_TIMEOUT_ERR | SD_INT_DATA_CRC_ERR | \
		 SD_INT_DATA_END_BIT_ERR)



/*
* Different quirks to handle when the hardware deviates from a strict
* interpretation of the SDHCI specification.
*/

/* Controller doesn't honor resets unless we touch the clock register */
#define SDHCI_QUIRK_CLOCK_BEFORE_RESET			(1<<0)
/* Controller has bad caps bits, but really supports DMA */
#define SDHCI_QUIRK_FORCE_DMA					(1<<1)
/* Controller doesn't like some resets when there is no card inserted. */
#define SDHCI_QUIRK_NO_CARD_NO_RESET			(1<<2)
/* Controller doesn't like clearing the power reg before a change */
#define SDHCI_QUIRK_SINGLE_POWER_WRITE			(1<<3)
/* Controller has flaky internal state so reset it on each ios change */
#define SDHCI_QUIRK_RESET_CMD_DATA_ON_IOS		(1<<4)
/* Controller has an unusable DMA engine */
#define SDHCI_QUIRK_BROKEN_DMA					(1<<5)
/* Controller can only DMA from 32-bit aligned addresses */
#define SDHCI_QUIRK_32BIT_DMA_ADDR				(1<<6)
/* Controller can only DMA chunk sizes that are a multiple of 32 bits */
#define SDHCI_QUIRK_32BIT_DMA_SIZE				(1<<7)
/* Controller needs to be reset after each request to stay stable */
#define SDHCI_QUIRK_RESET_AFTER_REQUEST			(1<<8)


#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3


#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2



#define MMC_CMD_RETRIES        0

/* Standard MMC commands (4.1)           type  argument     response */
   /* class 1 */
#define MMC_GO_IDLE_STATE         0   /* bc                          */
#define MMC_SEND_OP_COND          1   /* bcr  [31:0] OCR         R3  */
#define MMC_ALL_SEND_CID          2   /* bcr                     R2  */
#define MMC_SET_RELATIVE_ADDR     3   /* ac   [31:16] RCA        R1  */
#define MMC_SET_DSR               4   /* bc   [31:16] RCA            */
#define MMC_SWITCH                6   /* ac   [31:0] See below   R1b */
#define MMC_SELECT_CARD           7   /* ac   [31:16] RCA        R1  */
#define MMC_SEND_EXT_CSD          8   /* adtc                    R1  */
#define MMC_SEND_CSD              9   /* ac   [31:16] RCA        R2  */
#define MMC_SEND_CID             10   /* ac   [31:16] RCA        R2  */
#define MMC_READ_DAT_UNTIL_STOP  11   /* adtc [31:0] dadr        R1  */
#define MMC_STOP_TRANSMISSION    12   /* ac                      R1b */
#define MMC_SEND_STATUS          13   /* ac   [31:16] RCA        R1  */
#define MMC_GO_INACTIVE_STATE    15   /* ac   [31:16] RCA            */
#define MMC_SPI_READ_OCR         58   /* spi                  spi_R3 */
#define MMC_SPI_CRC_ON_OFF       59   /* spi  [0:0] flag      spi_R1 */

  /* class 2 */
#define MMC_SET_BLOCKLEN         16   /* ac   [31:0] block len   R1  */
#define MMC_READ_SINGLE_BLOCK    17   /* adtc [31:0] data addr   R1  */
#define MMC_READ_MULTIPLE_BLOCK  18   /* adtc [31:0] data addr   R1  */

  /* class 3 */
#define MMC_WRITE_DAT_UNTIL_STOP 20   /* adtc [31:0] data addr   R1  */

  /* class 4 */
#define MMC_SET_BLOCK_COUNT      23   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_BLOCK          24   /* adtc [31:0] data addr   R1  */
#define MMC_WRITE_MULTIPLE_BLOCK 25   /* adtc                    R1  */
#define MMC_PROGRAM_CID          26   /* adtc                    R1  */
#define MMC_PROGRAM_CSD          27   /* adtc                    R1  */

  /* class 6 */
#define MMC_SET_WRITE_PROT       28   /* ac   [31:0] data addr   R1b */
#define MMC_CLR_WRITE_PROT       29   /* ac   [31:0] data addr   R1b */
#define MMC_SEND_WRITE_PROT      30   /* adtc [31:0] wpdata addr R1  */

  /* class 5 */
#define MMC_ERASE_GROUP_START    35   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE_GROUP_END      36   /* ac   [31:0] data addr   R1  */
#define MMC_ERASE                38   /* ac                      R1b */

  /* class 9 */
#define MMC_FAST_IO              39   /* ac   <Complex>          R4  */
#define MMC_GO_IRQ_STATE         40   /* bcr                     R5  */

  /* class 7 */
#define MMC_LOCK_UNLOCK          42   /* adtc                    R1b */

  /* class 8 */
#define MMC_APP_CMD              55   /* ac   [31:16] RCA        R1  */
#define MMC_GEN_CMD              56   /* adtc [0] RD/WR          R1  */


/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */

  /* class 10 */
#define SD_SWITCH                 6   /* adtc [31:0] See below   R1  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_SEND_NUM_WR_BLKS  22   /* adtc                    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

#define SD_APP_SEND_STATUS       13



#define CMD11_SWITCH_SIGNAL_VOLTAGE		11
#define CMD19_TUNING_TIMEING			19

#define TUNING_BLOCK_SIZE				64


#define CMD19_BUSTEST_W					19
#define CMD14_BUSTEST_R					14


/*
 * MMC_SWITCH argument format:
 *
 *	[31:26] Always 0
 *	[25:24] Access Mode
 *	[23:16] Location of target Byte in EXT_CSD
 *	[15:08] Value Byte
 *	[07:03] Always 0
 *	[02:00] Command Set
 */

/*
  MMC status in R1, for native mode (SPI bits are different)
  Type
	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		(1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	(1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	(1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		(1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		(1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	(1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	(1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	(1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	(1 << 21)	/* ex, c */
#define R1_CC_ERROR		(1 << 20)	/* erx, c */
#define R1_ERROR		(1 << 19)	/* erx, c */
#define R1_UNDERRUN		(1 << 18)	/* ex, c */
#define R1_OVERRUN		(1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	(1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		(1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	(1 << 8)	/* sx, a */
#define R1_APP_CMD		(1 << 5)	/* sr, c */


/*
 * OCR bits are mostly in host.h
 */
#define MMC_CARD_READY	0x80000000	/* Card Power up status bit */


/*
 * Card Command Classes (CCC)
 */
#define CCC_BASIC		(1<<0)	/* (0) Basic protocol functions */
					/* (CMD0,1,2,3,4,7,9,10,12,13,15) */
					/* (and for SPI, CMD58,59) */
#define CCC_STREAM_READ		(1<<1)	/* (1) Stream read commands */
					/* (CMD11) */
#define CCC_BLOCK_READ		(1<<2)	/* (2) Block read commands */
					/* (CMD16,17,18) */
#define CCC_STREAM_WRITE	(1<<3)	/* (3) Stream write commands */
					/* (CMD20) */
#define CCC_BLOCK_WRITE		(1<<4)	/* (4) Block write commands */
					/* (CMD16,24,25,26,27) */
#define CCC_ERASE		(1<<5)	/* (5) Ability to erase blocks */
					/* (CMD32,33,34,35,36,37,38,39) */
#define CCC_WRITE_PROT		(1<<6)	/* (6) Able to write protect blocks */
					/* (CMD28,29,30) */
#define CCC_LOCK_CARD		(1<<7)	/* (7) Able to lock down card */
					/* (CMD16,CMD42) */
#define CCC_APP_SPEC		(1<<8)	/* (8) Application specific */
					/* (CMD55,56,57,ACMD*) */
#define CCC_IO_MODE		(1<<9)	/* (9) I/O mode */
					/* (CMD5,39,40,52,53) */
#define CCC_SWITCH		(1<<10)	/* (10) High speed switch */
					/* (CMD6,34,35,36,37,50) */
					/* (11) Reserved */
					/* (CMD?) */

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1 - 3.2 - 3.31 - 4.0 - 4.1 */
#define CSD_STRUCT_EXT_CSD  3           /* Version is coded in CSD_STRUCTURE in EXT_CSD */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 - 3.2 - 3.31 */
#define CSD_SPEC_VER_4      4           /* Implements system specification 4.0 - 4.1 */

/*
 * EXT_CSD fields
 */

#define EXT_CSD_BUS_WIDTH	183	/* R/W */
#define EXT_CSD_HS_TIMING	185	/* R/W */
#define EXT_CSD_CARD_TYPE	196	/* RO */
#define EXT_CSD_REV			192	/* RO */
#define EXT_CSD_SEC_CNT		212	/* RO, 4 bytes */

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL		(1<<0)
#define EXT_CSD_CMD_SET_SECURE		(1<<1)
#define EXT_CSD_CMD_SET_CPSECURE	(1<<2)

#define EXT_CSD_CARD_TYPE_26	(1<<0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52	(1<<1)	/* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */

/*
 * MMC_SWITCH access modes
 */

#define MMC_SWITCH_MODE_CMD_SET		0x00	/* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS	0x01	/* Set bits which are 1 in value */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02	/* Clear bits which are 1 in value */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03	/* Set target to value */



/* ocr response */
#define	SD_HCS			0x40
#define	SD_XPC			0x10
#define	SD_S18R			0x01


/* sd_app_set_bus_width */
#define SD_BUS_WIDTH_1		0
#define SD_BUS_WIDTH_4		2


#define SD_SWITCH_CHECK_MODE	0
#define SD_SWITCH_SET_MODE		1
#define SD_FUNCTION_GROUP_1		0x01
#define SD_FUNCTION_GROUP_4		0x04

#define SD_ACCESS_MODE_DEFAULT_SPEED		0x00
#define SD_ACCESS_MODE_CURRENT_SPEED		0x0F
#define SD_ACCESS_MODE_HIGH_SPEED			0x01

#define SD_DEFAULT_CURRENT_LIMIT_200mA		0x00
#define SD_CURRENT_LIMIT_CURRENT_CURRENT	0x0F

/* SD3.0 CMD6 switch function status */
#define SDR12_ACCESS_MODE				0x01
#define SDR25_ACCESS_MODE				0x02
#define SDR50_ACCESS_MODE				0x04
#define SDR104_ACCESS_MODE				0x08
#define DDR50_ACCESS_MODE				0x10
#define	CMD6_MAX_ACCESS_MODE			(SDR12_ACCESS_MODE|SDR25_ACCESS_MODE|SDR50_ACCESS_MODE|SDR104_ACCESS_MODE)
#define	CMD6_MAX_CURRENT_LIMIT			0x03

#define MAX_TUNING_PHASE				0x10



/*************************************************************************************************/


#define mmc_card_mmc(c)			((c)->type == MMC_TYPE_MMC)
#define mmc_card_sd(c)			((c)->type == MMC_TYPE_SD)


#define mmc_card_present(c)		((c)->state & MMC_STATE_PRESENT)
#define mmc_card_readonly(c)	((c)->state & MMC_STATE_READONLY)
#define mmc_card_highspeed(c)	((c)->state & MMC_STATE_HIGHSPEED)
#define mmc_card_blockaddr(c)	((c)->state & MMC_STATE_BLOCKADDR)

#define mmc_card_set_present(c)		((c)->state |= MMC_STATE_PRESENT)
#define mmc_card_set_readonly(c)	((c)->state |= MMC_STATE_READONLY)
#define mmc_card_set_highspeed(c)	((c)->state |= MMC_STATE_HIGHSPEED)
#define mmc_card_set_blockaddr(c)	((c)->state |= MMC_STATE_BLOCKADDR)


/*************************************************************************************************/
/*
 * struct definition
 */
/*************************************************************************************************/

/***********************************************/
struct sd_csd {
/***********************************************/
	u8		mmca_vsn;
	u16		cmdclass;
	u16		tacc_clks;
	u32		tacc_ns;
	u32		r2w_factor;
	u32		max_dtr;
	u32		read_blkbits;
	u32		write_blkbits;
	u32		capacity;
	u32		read_partial;
	u32		read_misalign;
	u32		write_partial;
	u32		write_misalign;

	u32		perm_write_protect;
	u32		temp_write_protect;
};

/***********************************************/
struct sd_ext_csd {
/***********************************************/
	u32		hs_max_dtr;
	u32		sectors;
};

/***********************************************/
struct sd_scr {
/***********************************************/
	u8		sda_vsn;

#define SCR_SPEC_VER_0		0	/* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1		1	/* Implements system specification 1.10 */
#define SCR_SPEC_VER_2		2	/* Implements system specification 2.00 */

	u8		bus_widths;

#define SD_SCR_BUS_WIDTH_1	(1<<0)
#define SD_SCR_BUS_WIDTH_4	(1<<2)

};

/***********************************************/
struct sd_switch_caps {
/***********************************************/
	u32		hs_max_dtr;
};

/***********************************************/
struct sd_card {
/***********************************************/
	struct sd_host		*sd;

	u32		rca;		/* relative card address of device */
	u32		type;		/* card type */

#define MMC_TYPE_SD			0		/* SD card */
#define MMC_TYPE_MMC		1		/* MMC card */

	u32		state;		/* (our) card state */

#define MMC_STATE_PRESENT	(1<<0)		/* present in sysfs */
#define MMC_STATE_READONLY	(1<<1)		/* card is read-only */
#define MMC_STATE_HIGHSPEED	(1<<2)		/* card is in high speed mode */
#define MMC_STATE_BLOCKADDR	(1<<3)		/* card uses block-addressing */

	u32					raw_cid[4];	/* raw card CID */
	u32					raw_csd[4];	/* raw card CSD */
	u32					raw_scr[2];	/* raw card SCR */

	struct sd_csd			csd;
	struct sd_scr			scr;
	struct sd_switch_caps	sw_caps;	/* switch (CMD6) caps */
	struct sd_ext_csd		ext_csd;


	u8			support_18v;


	u8			xfer_err_cnt;
	u8			card_is_error;

	u32			over_current;

	u8 sd_access_mode;

	u8 sdxc_tuning;
	u8 sdxc_tuning_err;
	u8 sdxc_tuning_timeout;

};

/***********************************************/
struct sd_command {
/***********************************************/

	u32			opcode;
	u32			arg;
	u32			resp[4];
	u32			flags;			/* expected response type */
#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_136		(1 << 1)		/* 136 bit response */
#define MMC_RSP_CRC		(1 << 2)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 3)		/* card may send busy */
#define MMC_RSP_OPCODE	(1 << 4)		/* response contains opcode */

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC		(0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC		(2 << 5)
#define MMC_CMD_BCR		(3 << 5)

#define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */
#define MMC_RSP_SPI_B4	(1 << 9)		/* four data bytes */
#define MMC_RSP_SPI_BUSY (1 << 10)		/* card may send busy */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE	(0)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_PRESENT)
#define MMC_RSP_R4	(MMC_RSP_PRESENT)
#define MMC_RSP_R5	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define mmc_resp_type(cmd)	((cmd)->flags & (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC|MMC_RSP_BUSY|MMC_RSP_OPCODE))


	u32		retries;	/* max number of retries */
	u32		error;		/* command error */
};

/***********************************************/
struct ampe_sd_data {
/***********************************************/

	u32		blksz;			/* data block size */
	u32		blocks;			/* number of blocks */
	u32		error;			/* data error */
	u32		flags;

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)

	u32					xfered_cnt;
	u32					remain_cnt;

	void				*buf;
	u32					buf_len;
	u32					buf_type;

#define		BUF_NOT_MDL		0
#define		BUF_IS_MDL		1


	struct scatterlist	*sg;		/* I/O scatter list */
	struct scatterlist	*sg_map;	/* I/O scatter list */
	u32					 sg_cnt;
	dma_addr_t			 dma_addr_map;
	dma_addr_t			 dma_addr_cur;

};

/***********************************************/
struct sd_request {
/***********************************************/

	struct sd_command	*cmd;
	struct ampe_sd_data		*data;
	struct sd_command	*stop;

	void			*done_data;	/* completion data */
	void			(*done)(struct sd_request *);/* completion function */
};

/***********************************************/
struct sd_host {
/***********************************************/

	/*===============================*/
	/* common_host_data				 */
	/*===============================*/
	struct _DEVICE_EXTENSION *pdx;

	/* For Memory Base Address & Length */
	u8 *ioaddr;

	u32	flags;

#define SDHCI_USE_DMA		(1<<0)		/* Host is DMA capable */
#define SDHCI_REQ_USE_DMA	(1<<1)		/* Use DMA for this req. */

	/*===============================*/
	/* End of common_host_data		 */
	/*===============================*/

	/*===============================*/
	/* SD host controller's Resource */
	/*===============================*/
	KDPC			card_tasklet;
	KDPC			finish_tasklet;

	KTIMER			timeout_timer;
	KDPC			timeout_tasklet;

	/*===============================*/
	/* Parameters for MS card access */
	/*===============================*/
	struct sd_card		scard;


	struct sd_request	*srq;		/* Current request */
	struct sd_command	*cmd;		/* Current command */
	struct ampe_sd_data		*data;		/* Current data request */
	struct sd_command	*stop;		/* Current stop command */

	u32		clock;		/* Current clock (MHz) */
	u16		power;		/* Current voltage */


	u8		card_inserted;

	u8		card_type;

	u32		last_cmd;
	u32		last_lba;

	u8		bus_mode;


	u8 uhs_mode_err;

	u32 sdr_25_init_clk;
	u32 sdr_50_init_clk;
	u32 sdr_100_init_clk;
	u32 mmc_26_init_clk;
	u32 mmc_52_init_clk;

	u32 sd_err_init_clk;



	struct sg_mapping_iter	sg_miter;	/* SG state for PIO */
	struct scatterlist 		*sg;

	u8  sd_in_tuning;

	u8  sd_chking_card_ready;
	u8  sd_card_ready_timeout;

	/*******************************************/
	/* External setting						   */
	/*******************************************/

	u32	uSdExtNoUhsMode;
	u32	uSdExt200MHzOff;

	u32	uSdExtSdr25Clock;
	u32	uSdExtSdr50Clock;
	u32	uSdExtSdr100Clock;

	u32	uSdExtMmcType26Clock;
	u32	uSdExtMmcType52Clock;
	u32 uSdExtMmcForce1Bit;
	u32	uSdExtMmcForce4Bit;

	u32	uSdExt50SetHighSpeed;
	u32	uSdExt100SetHighSpeed;
	u32	uSdExt200SetHighSpeed;

	u32	uSdExtCprmEnable;

	u32 uSdExtSingleRdWrCmd;

	u32 uSdExtHwTimeOutCnt;


	u32	uSdExtInitErrReduceClk;
	u32	uSdExtIoErrReduceClk;

	u32 uSdExtInitClkDelay;

	u32 uSdExtInitCmdDelayUs;

	u32 uSdExtDisableMmc;

	u32 uSdExtTuneUpSpeed;

};



/*************************************************************************************************/
/* PcieSdhc.c */
/*************************************************************************************************/
int sd_add_device(struct _DEVICE_EXTENSION *pdx);
int sd_remove_device(struct _DEVICE_EXTENSION *pdx);
int sd_get_external_setting(struct sd_host *sd);

int sd_stop_last_cmd(struct sd_host	*sd);

int sd_scsi_read_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);

void sd_prepare_data(struct sd_host *sd, struct ampe_sd_data *data);

void sd_dumpregs(struct sd_host *sd);
int  sd_card_active_ctrl(struct sd_host *sd, u8 active);
void sd_reset(struct sd_host *sd);
void sd_init_int(struct sd_host *sd);
void sd_transfer_pio_read(struct sd_host *sd);
void sd_transfer_pio_write(struct sd_host *sd);
void sd_transfer_data(struct sd_host *sd, struct ampe_sd_data *data);
void sd_finish_data(struct sd_host *sd);
void sd_send_command(struct sd_host *sd, struct sd_command *cmd, u8 is_stop);
void sd_finish_command(struct sd_host *sd, struct sd_command *cmd);
void sd_set_clock(struct sd_host *sd, u32 clock);

void sdhci_tasklet_card(unsigned long parm);
void sdhci_tasklet_finish(unsigned long parm);
void sdhci_tasklet_timeout(unsigned long parm);


void sdhci_irq_cmd(struct sd_host *sd, u32 intmask);
void sdhci_irq_data(struct sd_host *sd, u32 intmask);
irqreturn_t sdhci_irq(struct _DEVICE_EXTENSION *pdx);


void sd_set_bus_mode(struct sd_host *sd, u8 bus_mode);
void sd_set_timing_mode(struct sd_host *sd, u8 timing_mode);
void sd_clk_phase_ctrl(struct sd_host *sd, u8 phase);

void sd_update_media_card(struct sd_host *sd);
int  sd_init_card(struct sd_host *sd);
void sd_power_on(struct sd_host *sd);
void sd_power_off(struct sd_host *sd);
int  sd_go_idle(struct sd_host *sd);
int  sd_wait_for_data(struct sd_host *sd, struct ampe_sd_data *data);
int  sd_wait_for_cmd(struct sd_host *sd, struct sd_command *cmd, int retries);
void sd_wait_for_req(struct sd_host *sd, struct sd_request *srq);
void sd_start_request(struct sd_host *sd, struct sd_request *srq);
void sd_request_done(struct sd_host *sd, struct sd_request *srq);
void sd_wait_done(struct sd_request *srq);
int  sd_app_cmd(struct sd_host *sd, struct sd_card *scard);
int  sd_send_app_cmd(struct sd_host *sd, struct sd_card *scard, struct sd_command *cmd, int retries);
int  sd_send_op_cond(struct sd_host *sd, u32 ocr, u32 *rocr);
int  sd_send_app_op_cond(struct sd_host *sd, u32 ocr, u32 *rocr);
int  sd_send_if_cond(struct sd_host *sd);
int  sd_all_send_cid(struct sd_host *sd, u32 *cid);
int  sd_set_relative_addr(struct sd_host *sd);
int  sd_send_cxd_native(struct sd_host *sd, u32 arg, u32 *cxd, int opcode);
int  sd_send_csd(struct sd_host *sd, u32 *csd);
int  sd_decode_csd(struct sd_host *sd);
u32  UNSTUFF_BITS(u32 *resp, u32 start,u32 size);
int  sd_send_cid(struct sd_host *sd, u32 *cid);
int  sd_select_card(struct sd_host *sd);
int  sd_send_status(struct sd_host *sd, u32 *status);
int  sd_app_send_scr(struct sd_host *sd);
int  sd_app_send_status(struct sd_host *sd);
int  sd_sd_switch(struct sd_host *sd, int mode, int group, u8 value, u8 *resp);
int  sd_switch_hs(struct sd_host *sd);
int  sd_switch(struct sd_host *sd, u8 set, u8 index, u8 value);
int  sd_app_set_bus_width(struct sd_host *sd, int width);
int  sd_set_blksize(struct sd_host *sd);
int  sd_send_ext_csd(struct sd_host *sd);


int  sd30_switch_signal_voltage(struct sd_host *sd);
int  sd_switch_current_limit(struct sd_host *sd);


int sd30_tuning_sequence(struct sd_host *sd);
int sd_stop(struct sd_host *sd);
int sd_mmc_check_bus(struct sd_host *sd);

/*************************************************************************************************/
#endif /* __AMCR_SD_H_ */
