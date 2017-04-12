#if !defined(__AMCR_MS_H_)
/*************************************************************************************************/
#define __AMCR_MS_H_



#define MS_SEGMENT_SIZE			(0x200)
#define MS_PAGE_SIZE			(0x200)
#define MS_UNUSED_BLOCK			(0xffff)



/*==========================================================
 * Controller registers
 *=========================================================*/
#define MS_DMA_ADDRESS			CARD_DMA_ADDRESS

#define MS_DMA_BOUNDARY			CARD_DMA_BOUNDARY

#define MS_BUFFER_PORT			CARD_BUFFER_PORT

#define MS_DMA_CTRL				CARD_DMA_CTRL
#define		MS_DMA_ENABLE			CARD_DMA_ENABLE

#define MS_XFER_LENGTH			CARD_XFER_LENGTH

#define MS_POWER_CONTROL		CARD_POWER_CONTROL


#define MS_CARD_ACTIVE_CTRL		CARD_ACTIVE_CTRL

#define MS_SOFTWARE_RESET		CARD_SOFTWARE_RESET

#define MS_OUTPUT_ENABLE		CARD_OUTPUT_ENABLE


#define MS_STATUS				0xa0

#define MS_BUS_MODE_CTRL		0xa1
#define  MS_BUS_1BIT_MODE			0x00
#define  MS_BUS_4BIT_MODE			0x01
#define  MS_BUS_8BIT_MODE			0x03

#define MS_TPC_CMD				0xa2
#define  MS_TPC_READ_PAGE_DATA		0x02
#define  MS_TPC_READ_REG			0x04
#define  MS_TPC_GET_INT				0x07
#define  MS_TPC_WRITE_PAGE_DATA		0x0D
#define  MS_TPC_WRITE_REG			0x0B
#define  MS_TPC_SET_RW_REG_ADRS		0x08
#define  MS_TPC_SET_CMD				0x0E
#define  MS_TPC_EX_SET_CMD			0x09
#define  MS_TPC_READ_SHORT_DATA		0x03
#define  MS_TPC_WRITE_SHORT_DATA	0x0C


#define MS_TRANSFER_MODE		0xa3
#define		MS_XFER_START			0x01
#define		MS_XFER_DMA_ENABLE		0x02
#define		MS_XFER_INT_TIMEOUT_CHK 0x04

#define MS_DATA_PIN_STATE		0xa4



#define MS_INT_STATUS			0xb0
#define MS_INT_ENABLE			0xb4
#define  MS_INT_TPC_END				0x00000002
#define  MS_INT_DMA_END				0x00000008
#define  MS_INT_BUF_WRITE_RDY		0x00000010
#define  MS_INT_BUF_READ_RDY		0x00000020
#define  MS_INT_CARD_REMOVE			0x00000040
#define  MS_INT_CARD_INSERT			0x00000080
#define  MS_INT_ERROR				0x00008000

#define  MS_INT_DATA_MASK			0x00000038

#define  MS_INT_TPC_TIMEOUT			0x00010000
#define  MS_INT_CED_ERROR			0x00040000
#define  MS_INT_INT_RESP_ERROR		0x00080000
#define  MS_INT_INT_TIMEOUT			0x00100000
#define  MS_INT_DATA_CRC_ERROR		0x00200000

#define  MS_INT_OVER_CURRENT_ERROR	0x00800000

#define  MS_INT_TPC_MASK			0x003d8002
#define  MS_INT_TPC_ERROR			0x003d0000


///////////////////////////////////////////////////////////////////////////
#define MS_EEPROM_ADDRESS		0x08

#define MS_EEPROM_DATA			0x09

#define MS_EEPROM_CONTROL		0x0a
#define  MS_EEPROM_READ			 0x01
#define  MS_EEPROM_WRITE		 0x02

#define MS_EEPROM_STATUS		0x0b
#define  MS_EEPROM_STATUS_BUSY	 0x01
#define  MS_EEPROM_STATUS_ERROR  0x02
///////////////////////////////////////////////////////////////////////////





/***********************
 * Constant Definition *
 ***********************/

/* 0x30: MsStatusReg */

/* 0x32: MsProReg */
/* bit 6~7, 1/4/8bit */
#define MS_PRO_REG_INTERFACE_BIT			0xC0
#define MS_PRO_SERIAL_MODE					0x00
#define MS_PRO_PARALLEL_MODE				0x40
#define MS_PRO_PARALLEL_8BIT_MODE			0xC0


#define MS_PRO_INT							0x0f
#define MS_PRO_ERR_CMDNK					0x0A
#define MS_PRO_ERR_CMDNK_CED				0x0B
#define MS_PRO_CED_BREQ						0x05

#define MS_PRO_CMDNK						0x08
#define MS_PRO_BREQ							0x04
#define MS_PRO_ERR							0x02
#define MS_PRO_CED							0x01

/* Int register constant */
#define CED_ERR_BREQ						0xE0
#define CED_BREQ							0xA0
#define CED									0x80
#define CED_ERR								0xC0
#define ERR_BREQ							0x60
#define ERR_CMDNK							0x41
#define ERR_CMDNK_CED						0xC1
#define MS_INT								0xE1
#define ERR									0x40
#define BREQ_CMDNK							0x21
#define BREQ								0x20
#define CMDNK								0x01
#define MS_MALFUNCTION						0x40
#define WP									0x01

/* Status1 register constant */
#define UCDT_UCEX_UCFG						0x15

/* Management flag register constant */
#define SCMS0_SCMS1							0x30

/* System parameter register */
#define MS_SERIAL_IF						0x80
#define MS_PARALLEL_4BIT_IF					0x88
#define MS_PRO_PARALLEL_4BIT_IF				0x00
#define MS_PRO_PARALLEL_8BIT_IF				0x40

/* Command parameter */
#define OVERWRITE_FLAG_BY_PAGE				0x80
#define EXTRA_DATA_ONLY_BY_PAGE				0x40
#define DATA_AND_EXTRA_DATA_BY_PAGE			0x20
#define DATA_AND_EXTRA_DATA_BY_BLOCK		0x00

/* Other constant */
#define OVERWRITE_FLAG_BKST					0x80
#define OVERWRITE_FLAG_PGST					0x60
#define OVERWRITE_FLAG_UDST					0x10

#define BLOCK_PAGE_STATUS_OK				0xe0
#define BLOCK_OK_PAGE_DATA_ERROR			0x80
#define BOOT_BLOCK_SIGNATURE				0xc0
#define NOT_BOOT_BLOCK						0x04
#define UPDATE_STATUS_MASK_DATA				0xef
#define BLOCK_STATUS_MASK_DATA				0x7f
#define PAGE_STATUS_DATA_ERROR_MASK_DATA 	0x9f
#define PAGE_STATUS_NG_MASK_DATA			0xbf
#define UPDATE_STATUS						0x10

#define MS_CLASS_VER_1XX					0x01
#define FORMAT_TYPE_FAT						0x01
#define DEVICE_TYPE_ROM						0x04
#define PARALLEL_SUPPORT					0x01


#define MSPRO_READ_DATA						0x20
#define MSPRO_WRITE_DATA					0x21
#define MSPRO_READ_ATRB						0x24
#define MSPRO_STOP							0x25
#define MSPRO_ERASE							0x26
#define MSPRO_FORMAT						0x10
#define MSPRO_SLEEP							0x11




#define MS_CONTROL_REG					   0x23
 #define SET_CMD_1_BYTE						0x0e
 #define EX_SET_CMD_7_BYTE					0x69
 #define GET_INT_1_BYTE						0x07
 #define READ_REG_1_BYTE					0x04
 #define READ_REG_4_BYTE					0x34
 #define READ_REG_6_BYTE					0x54
 #define READ_REG_15_BYTE					0xE4
 #define SET_RW_REG_ADRS_4_BYTE				0x38
 #define WRITE_REG_1_BYTE					0x0b
 #define WRITE_REG_2_BYTE					0x1b
 #define WRITE_REG_6_BYTE					0x5b
 #define WRITE_REG_7_BYTE					0x6b
 #define WRITE_REG_8_BYTE					0x7b
 #define WRITE_REG_15_BYTE					0xeb
 #define READ_PAGE_DATA_512_BYTE			0x02
 #define WRITE_PAGE_DATA_512_BYTE			0x0d
 #define READ_SHORT_DATA					0x03

/* Ms Flash command */
#define MS_BLOCK_READ						0xaa
#define MS_BLOCK_WRITE						0x55
#define MS_BLOCK_END						0x33
#define MS_BLOCK_ERASE						0x99
#define MS_FLASH_STOP						0xcc
#define MS_CLEAR_BUF						0xc3
#define MS_RESET							0x3c

/* Access state constant */
#define READ_PROCESS						0x01
#define WRITE_PROCESS						0x02

/* ReInitialize constant */
/* Modify value for 4 slot issue */
#define TABLE_CACHE_FAILURE					0x08
#define NO_FREE_BLOCK_LEFT					0x40
#define MS_CARD_CHANGE_NO_FREE_BLOCK		0x48

/* RecoveryMode constant */
#define READ_ERROR_RECOVERY					0x01
#define WRITE_ERROR_RECOVERY				0x02
#define NONE_ADR_ASSIGN_RECOVERY			0x03

/* bMsType constant */
#define MEMORY_STICK						0x00
#define MEMORY_STICK_ROM					0x01
#define MEMORY_STICK_ROM_GROUP				0x01
#define MEMORY_STICK_PRO					0x02
#define MEMORY_STICK_PRO_GROUP				0x0A
#define MEMORY_STICK_PRO_ROM				0x03
#define MEMORY_STICK_PRO_HG					0x04
#define MEMORY_STICK_PRO_HG_ROM				0x05
#define MEMORY_STICK_PRO_HG_GROUP			0x04
#define MEMORY_STICK_EHC					0x08
#define MEMORY_STICK_EHC_ROM				0x09

/* MSPRO EHC Interface type define */
#define MEMORY_STICK_EHC_INTERFACE_TYPE			0x03
#define MEMORY_STICK_EHC_INTERFACE_TYPE_1BIT	0x00
#define MEMORY_STICK_EHC_INTERFACE_TYPE_4BIT	0x01
#define MEMORY_STICK_EHC_INTERFACE_TYPE_8BIT	0x03

/* bBootAreaProtectStatus	*/
#define	NEED_BOOT_AREA_PROTECT_PROCESS		0x01
#define	FIND_GOOD_BLOCK						0x02
#define	FIND_BOOT_BLOCK						0x04


/* MS Card Capacity  */
#define MS_4M					0x00
#define MS_8M					0x01
#define MS_16M					0x02
#define MS_32M					0x03
#define MS_64M					0x04
#define MS_128M					0x05

#define FAT1_OFFSET				0x00
#define FAT2_OFFSET				0x01
#define ROOT_OFFSET				0x02

/*=================================================================
 * MagicGate MS command define
 *=================================================================*/
#define MSPRO_GET_LEKB				0x42
#define MSPRO_GET_ID				0x40
#define MSPRO_MAKE_RMS				0x44
#define MSPRO_GET_IBD				0x47
#define MSPRO_SET_LID				0x41
#define MSPRO_SET_RD				0x43
#define MSPRO_MAKE_KSE				0x45
#define MSPRO_SET_IBD				0x46
#define WRITE_SHORT_DATA			0x0C

/* MagicGate Command Data Length of different Key Format */
#define SET_LEAF_ID_DATA_LEN				0x0C
#define START_AUTH_OR_SET_RSP_DATA_LEN		0x0C
#define SET_ICV_DATA_LEN					0x0404
#define GET_LEKB_DATA_LEN					0x41C
#define GET_ID_RSP_CHALLENGE_DATA_LEN		0x24
#define GET_ICV_DATA_LEN					0x0404

/* MagicGate SHORT DATA parameter */
#define SHORT_DATA_32_BYTE					0
#define SHORT_DATA_64_BYTE					1
#define SHORT_DATA_128_BYTE					2
#define SHORT_DATA_256_BYTE					3
#define WRITE_DIRECTION						0x00
#define READ_DIRECTION						0x80

/* MagicGate Secure TPC Excution parameter */
#define NO_PARAMETER		0
#define WITH_PARAMETER		1


/*************************************************************************************************/
/*
 * Struct definition
 */
/*************************************************************************************************/

/***********************************************/
struct ms_card {
/***********************************************/
	u32		state;					/* (our) card state */

#define MS_STATE_PRESENT		(0x01)		/* present in sysfs */
#define MS_STATE_HW_READONLY	(0x02)		/* card is read-only */
#define MS_STATE_NO_UNUSED_BLK	(0x04)		/* card is read-only */
#define MS_STATE_READONLY		(0x06)

	/* ms card */
	u8	bBlockSize;
	u8	bBootBlockAdr0;
	u8	bMsType;
	u8	bBootAreaProtectFlag;
	u16	wTotalBlockNum;
	u8	init_badblk[MS_PAGE_SIZE];
	u16	*ptable;

	u8	ms_bus_mode;
	u32	total_lba;

	u8	bMsProInit8BitErr;

	u32	over_current;
};

/***********************************************/
struct ms_page{
/***********************************************/
	union {
		struct {
			u8	bBlockAdr0;
			u8	bBlockAdr1;
		};
		u16	wBlockAdr;
	};

	u8	bCmdParameter;
	u8	bPageAdr;

	u8	bMsOverwriteFlag;

	union {
		struct {
			u8  bLogicalAdr0;
			u8  bLogicalAdr1;
		};
		u16	wLogicalAdr;
	};


	u8  *buf;
	u8	buf_type;
	u16	blk_cnt;

	u8	get_int;

	u8	IsMsMgAccess;

};

/***********************************************/
struct ms_request{
/***********************************************/
	struct ms_tpc		*tpc;

	void	*done_data;					 /* completion data */
	void	(*done)(struct ms_request *);/* completion function */
};

/***********************************************/
struct ms_tpc {
/***********************************************/

	u16		blk_cnt;

	void	*buf;
	u32	buf_len;
	u32	buf_type;

	u8	tpc_ctrl;

#define	MS_TPC_CTRL_GET_INT		0x80

	u8	get_int;

	int	error;

};

/***********************************************/
struct ms_host {
/***********************************************/

	/*===============================*/
	/* common_host_data				 */
	/*===============================*/
	struct _DEVICE_EXTENSION *pdx;

	/* For Memory Base Address & Length */
	u8 *ioaddr;


	u32	flags;
#define MS_SUPPORT_DMA	(1<<0)		/* Host is DMA capable */
#define MS_REQ_USE_DMA	(1<<1)
	/*===============================*/
	/* End of common_host_data		 */
	/*===============================*/


	/*===============================*/
	/* MS host controller's Resource */
	/*===============================*/
	KDPC			card_tasklet;
	KDPC			finish_tasklet;

	/*===============================*/
	/* Parameters for MS card access */
	/*===============================*/

	struct ms_request	*srq;	/* Current request */
	struct ms_tpc		*tpc;	/* Current tpc */

	struct ms_card	mcard;	/* current ms card information */
	u8				boot_block[SECTOR_SIZE];
	u8				buf[SECTOR_SIZE];

	u8		card_inserted;


	u8				bMsErrorType;
#define MS_READ_PROTECTED			0x02

	u8				resp[16];
	u8				int_resp;


	u8	card_type;
	u8				bMGfmtFlag;
	u8				bFormatterState;
	u8				bProgressIndicator;
	u8				bMSCardCap;
	u8				xMsFormatReinitial;
	u16				wMsFtmCurBlkAdr;
	u16				wMsFtmCurLogAdr;
	u8				abMsCardId[0x10];

	u32	scsi_io_err_cnt;

	KTIMER			timeout_timer;
	KDPC			timeout_tasklet;


	/* linux */
	struct scatterlist	*sg;		/* I/O scatter list */
	struct scatterlist	*sg_map;	/* I/O scatter list */
	u32					 sg_cnt;
	dma_addr_t			 dma_addr_map;
	dma_addr_t			 dma_addr_cur;

	struct sg_mapping_iter	sg_miter;	/* SG state for PIO */
	u8 data_direction;
#define MS_DATA_READ		(1<<0)
#define MS_DATA_WRITE		(1<<1)

	/*******************************************/
	/* External setting						   */
	/*******************************************/
	u32	uMsExt_force_pio;
	u32	uMsExtPwrOnDelayTime;
	u32	uMsExtPwrOffDelayTime;
	u32	uMsExtInitRetry;

	u32	uMsExtEnFormatter;


	u32	uMsExtClock;

	u32	uMsExtPro1BitClock;
	u32	uMsExtPro4BitClock;
	u32	uMsExtPro8BitClock;

	u32	uMsExtProForce1Bit;
	u32	uMsExtProNo8Bit;

	u32 uMsExtHwTimeOutCnt;

};

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/
int ms_add_device(struct _DEVICE_EXTENSION *pdx);
int ms_remove_device(struct _DEVICE_EXTENSION *pdx);
int ms_get_external_setting(struct ms_host *ms);

int ms_scsi_thread(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);
int ms_scsi_read(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);
int ms_scsi_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);
int ms_pro_scsi_read(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);
int ms_pro_scsi_write(struct scsi_cmnd *srb, struct _DEVICE_EXTENSION *pdx, u32 lba, u32 lba_len);


void ms_tasklet_card(unsigned long parm);
void ms_tasklet_finish(unsigned long parm);

irqreturn_t ms_irq(struct _DEVICE_EXTENSION *pdx);
void ms_update_media_card(struct ms_host *ms);


int ms_card_active_ctrl(struct ms_host *ms, u8 active);

int msi_get_boot_block(struct ms_host *ms);
int msi_read_reg(struct ms_host *ms, u8 rd_addr, u8 rd_size);
int msi_set_if_mode(struct ms_host *ms, u8 bIfMode);
int msi_set_write_cmd_parameter(struct ms_host *ms, struct ms_page *page);

int msi_reset(struct ms_host *ms);
int msi_set_read_cmd_parameter(struct ms_host *ms, struct ms_page *page);
int msi_read_boot_block_data(struct ms_host *ms, struct ms_page *page);
int	msi_check_disabled_block(struct ms_host *ms, u16 blk);
int msi_erase_block_and_forget_it(struct ms_host *ms, struct ms_page *page);

int msi_bootarea_protection_process(struct ms_host *ms);
int msi_make_trans_table(struct ms_host *ms);

int ms_tpc_get_int(struct ms_host *ms);

int msi_read_error_correctable(struct ms_host *ms, struct ms_page *page);
int msi_clear_buffer(struct ms_host *ms);


int msi_erase_block_and_put_to_table(struct ms_host *ms, struct ms_page *page);
int msi_overwrite_extra_data(struct ms_host *ms, struct ms_page *page, u8 overwrite_mask);

int msi_copy_page(struct ms_host *ms, struct ms_page *rd, struct ms_page *wr);

int msi_write_page_data(struct ms_host *ms, struct ms_page *page);
int msi_read_page_data(struct ms_host *ms, struct ms_page *page);


int ms_dumpregs(struct ms_host *ms);
int ms_set_clock(struct ms_host *ms, u32 clock);
int msi_pro_get_sys_info(struct ms_host *ms);

int msi_pro_set_cmd_parameter(struct ms_host *ms, u8 *pbuf);
int ms_tpc_write_page_data(struct ms_host *ms, struct ms_page *page);
int ms_tpc_read_page_data(struct ms_host *ms, struct ms_page *page);


int ms_power_on(struct ms_host	*ms);
int ms_power_off(struct ms_host	*ms);
int ms_reset(struct ms_host	*ms);
int ms_init_int(struct ms_host *ms);
int ms_init_card(struct ms_host *ms);


void msi_pro_check_format_action(struct ms_host *ms);

int msi_erase_all_blocks(struct ms_host *ms);

int msi_log2phy_block(struct ms_host *ms, struct ms_page *page);
int msi_evenly_find_freeblock(struct ms_host *ms, struct ms_page *page);
int msi_store_block_to_table(struct ms_host *ms, struct ms_page *page);
int msi_pro_stop(struct ms_host *ms);

int msi_check_free_block_count(struct ms_host *ms, u16 segment);

void ms_tasklet_timeout(unsigned long parm);
void ms_set_timer(struct ms_host *ms);

/*************************************************************************************************/
#endif /* __AMCR_MS_H_ */
