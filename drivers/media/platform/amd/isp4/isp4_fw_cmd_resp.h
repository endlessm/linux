/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#ifndef _ISP4_CMD_RESP_H_
#define _ISP4_CMD_RESP_H_

/*
 * @brief Host and Firmware command & response channel.
 *        Two types of command/response channel.
 *          Type Global Command has one command/response channel.
 *          Type Stream Command has one command/response channel.
 *-----------                                        ------------
 *|         |       ---------------------------      |          |
 *|         |  ---->|  Global Command         |----> |          |
 *|         |       ---------------------------      |          |
 *|         |                                        |          |
 *|         |                                        |          |
 *|         |       ---------------------------      |          |
 *|         |  ---->|   Stream Command        |----> |          |
 *|         |       ---------------------------      |          |
 *|         |                                        |          |
 *|         |                                        |          |
 *|         |                                        |          |
 *|  HOST   |                                        | Firmware |
 *|         |                                        |          |
 *|         |                                        |          |
 *|         |       --------------------------       |          |
 *|         |  <----|  Global Response       |<----  |          |
 *|         |       --------------------------       |          |
 *|         |                                        |          |
 *|         |                                        |          |
 *|         |       --------------------------       |          |
 *|         |  <----|  Stream Response       |<----  |          |
 *|         |       --------------------------       |          |
 *|         |                                        |          |
 *|         |                                        |          |
 *-----------                                        ------------
 */

/*
 * @brief command ID format
 *        cmd_id is in the format of following type:
 *        type: indicate command type, global/stream commands.
 *        group: indicate the command group.
 *        id: A unique command identification in one type and group.
 *        |<-Bit31 ~ Bit24->|<-Bit23 ~ Bit16->|<-Bit15 ~ Bit0->|
 *        |      type       |      group      |       id       |
 */

#define CMD_TYPE_SHIFT                  24
#define CMD_GROUP_SHIFT                 16
#define CMD_TYPE_STREAM_CTRL            (0x2U << CMD_TYPE_SHIFT)

#define CMD_GROUP_STREAM_CTRL           (0x1U << CMD_GROUP_SHIFT)
#define CMD_GROUP_STREAM_BUFFER         (0x4U << CMD_GROUP_SHIFT)

/* Stream  Command */
#define CMD_ID_SET_STREAM_CONFIG        (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_CTRL | 0x1)
#define CMD_ID_SET_OUT_CHAN_PROP        (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_CTRL | 0x3)
#define CMD_ID_ENABLE_OUT_CHAN          (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_CTRL | 0x5)
#define CMD_ID_START_STREAM             (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_CTRL | 0x7)
#define CMD_ID_STOP_STREAM              (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_CTRL | 0x8)

/* Stream Buffer Command */
#define CMD_ID_SEND_BUFFER              (CMD_TYPE_STREAM_CTRL | CMD_GROUP_STREAM_BUFFER | 0x1)

/*
 * @brief response ID format
 *        resp_id is in the format of following type:
 *        type: indicate command type, global/stream commands.
 *        group: indicate the command group.
 *        id: A unique command identification in one type and group.
 *        |<-Bit31 ~ Bit24->|<-Bit23 ~ Bit16->|<-Bit15 ~ Bit0->|
 *        |      type       |      group      |       id       |
 */

#define RESP_GROUP_SHIFT                16
#define RESP_GROUP_MASK                 (0xff << RESP_GROUP_SHIFT)

#define GET_RESP_GROUP_VALUE(resp_id)   (((resp_id) & RESP_GROUP_MASK) >> RESP_GROUP_SHIFT)
#define GET_RESP_ID_VALUE(resp_id)      ((resp_id) & 0xffff)

#define RESP_GROUP_GENERAL              (0x1 << RESP_GROUP_SHIFT)
#define RESP_GROUP_NOTIFICATION         (0x3 << RESP_GROUP_SHIFT)

/* General Response */
#define RESP_ID_CMD_DONE                (RESP_GROUP_GENERAL | 0x1)

/* Notification */
#define RESP_ID_NOTI_FRAME_DONE         (RESP_GROUP_NOTIFICATION | 0x1)

#define CMD_STATUS_SUCCESS              0
#define CMD_STATUS_FAIL                 1
#define CMD_STATUS_SKIPPED              2

#define ADDR_SPACE_TYPE_GPU_VA          4

#define FW_MEMORY_POOL_SIZE             (200 * 1024 * 1024)

/*
 * standard ISP mipicsi=>isp
 */
#define MIPI0_ISP_PIPELINE_ID           0x5f91

enum isp4fw_sensor_id {
	SENSOR_ID_ON_MIPI0  = 0,  /* Sensor id for ISP input from MIPI port 0 */
};

enum isp4fw_stream_id {
	STREAM_ID_INVALID = -1, /* STREAM_ID_INVALID. */
	STREAM_ID_1 = 0,        /* STREAM_ID_1. */
	STREAM_ID_2 = 1,        /* STREAM_ID_2. */
	STREAM_ID_3 = 2,        /* STREAM_ID_3. */
	STREAM_ID_MAXIMUM       /* STREAM_ID_MAXIMUM. */
};

enum isp4fw_image_format {
	IMAGE_FORMAT_NV12 = 1,              /* 4:2:0,semi-planar, 8-bit */
	IMAGE_FORMAT_YUV422INTERLEAVED = 7, /* interleave, 4:2:2, 8-bit */
};

enum isp4fw_pipe_out_ch {
	ISP_PIPE_OUT_CH_PREVIEW = 0,
};

enum isp4fw_yuv_range {
	ISP_YUV_RANGE_FULL = 0,     /* YUV value range in 0~255 */
	ISP_YUV_RANGE_NARROW = 1,   /* YUV value range in 16~235 */
	ISP_YUV_RANGE_MAX
};

enum isp4fw_buffer_type {
	BUFFER_TYPE_PREVIEW = 8,
	BUFFER_TYPE_META_INFO = 10,
	BUFFER_TYPE_MEM_POOL = 15,
};

enum isp4fw_buffer_status {
	BUFFER_STATUS_INVALID,  /* The buffer is INVALID */
	BUFFER_STATUS_SKIPPED,  /* The buffer is not filled with image data */
	BUFFER_STATUS_EXIST,    /* The buffer is exist and waiting for filled */
	BUFFER_STATUS_DONE,     /* The buffer is filled with image data */
	BUFFER_STATUS_LACK,     /* The buffer is unavailable */
	BUFFER_STATUS_DIRTY,    /* The buffer is dirty, probably caused by
				 * LMI leakage
				 */
	BUFFER_STATUS_MAX       /* The buffer STATUS_MAX */
};

enum isp4fw_buffer_source {
	/* The buffer is from the stream buffer queue */
	BUFFER_SOURCE_STREAM,
};

struct isp4fw_error_code {
	u32 code1;
	u32 code2;
	u32 code3;
	u32 code4;
	u32 code5;
};

/*
 * Command Structure for FW
 */

struct isp4fw_cmd {
	u32 cmd_seq_num;
	u32 cmd_id;
	u32 cmd_param[12];
	u16 cmd_stream_id;
	u8 cmd_silent_resp;
	u8 reserved;
	u32 cmd_check_sum;
};

struct isp4fw_resp_cmd_done {
	/*
	 * The host2fw command seqNum.
	 * To indicate which command this response refer to.
	 */
	u32 cmd_seq_num;
	/* The host2fw command id for host double check. */
	u32 cmd_id;
	/*
	 * Indicate the command process status.
	 * 0 means success. 1 means fail. 2 means skipped
	 */
	u16 cmd_status;
	/*
	 * If the cmd_status is 1, that means the command is processed fail,
	 * host can check the isp4fw_error_code to get the detail
	 * error information
	 */
	u16 isp4fw_error_code;
	/* The response payload will be in different struct type */
	/* according to different cmd done response. */
	u8 payload[36];
};

struct isp4fw_resp_param_package {
	u32 package_addr_lo;	/* The low 32 bit addr of the pkg address. */
	u32 package_addr_hi;	/* The high 32 bit addr of the pkg address. */
	u32 package_size;	/* The total pkg size in bytes. */
	u32 package_check_sum;	/* The byte sum of the pkg. */
};

struct isp4fw_resp {
	u32 resp_seq_num;
	u32 resp_id;
	union {
		struct isp4fw_resp_cmd_done cmd_done;
		struct isp4fw_resp_param_package frame_done;
		u32 resp_param[12];
	} param;
	u8  reserved[4];
	u32 resp_check_sum;
};

struct isp4fw_mipi_pipe_path_cfg {
	u32 b_enable;
	enum isp4fw_sensor_id isp4fw_sensor_id;
};

struct isp4fw_isp_pipe_path_cfg {
	u32  isp_pipe_id;	/* pipe ids for pipeline construction */
};

struct isp4fw_isp_stream_cfg {
	/* Isp mipi path */
	struct isp4fw_mipi_pipe_path_cfg mipi_pipe_path_cfg;
	/* Isp pipe path */
	struct isp4fw_isp_pipe_path_cfg  isp_pipe_path_cfg;
	/* enable TNR */
	u32 b_enable_tnr;
	/*
	 * number of frame rta per-processing,
	 * set to 0 to use fw default value
	 */
	u32 rta_frames_per_proc;
};

struct isp4fw_image_prop {
	enum isp4fw_image_format image_format;	/* Image format */
	u32 width;				/* Width */
	u32 height;				/* Height */
	u32 luma_pitch;				/* Luma pitch */
	u32 chroma_pitch;			/* Chrom pitch */
	enum isp4fw_yuv_range yuv_range;		/* YUV value range */
};

struct isp4fw_buffer {
	/* A check num for debug usage, host need to */
	/* set the buf_tags to different number */
	u32 buf_tags;
	union {
		u32 value;
		struct {
			u32 space : 16;
			u32 vmid  : 16;
		} bit;
	} vmid_space;
	u32 buf_base_a_lo;		/* Low address of buffer A */
	u32 buf_base_a_hi;		/* High address of buffer A */
	u32 buf_size_a;			/* Buffer size of buffer A */

	u32 buf_base_b_lo;		/* Low address of buffer B */
	u32 buf_base_b_hi;		/* High address of buffer B */
	u32 buf_size_b;			/* Buffer size of buffer B */

	u32 buf_base_c_lo;		/* Low address of buffer C */
	u32 buf_base_c_hi;		/* High address of buffer C */
	u32 buf_size_c;			/* Buffer size of buffer C */
};

struct isp4fw_buffer_meta_info {
	u32 enabled;					/* enabled flag */
	enum isp4fw_buffer_status status;		/* BufferStatus */
	struct isp4fw_error_code err;			/* err code */
	enum isp4fw_buffer_source source;		/* BufferSource */
	struct isp4fw_image_prop image_prop;		/* image_prop */
	struct isp4fw_buffer buffer;			/* buffer */
};

struct isp4fw_meta_info {
	u32 poc;				/* frame id */
	u32 fc_id;				/* frame ctl id */
	u32 time_stamp_lo;			/* time_stamp_lo */
	u32 time_stamp_hi;			/* time_stamp_hi */
	struct isp4fw_buffer_meta_info preview;	/* preview BufferMetaInfo */
};

struct isp4fw_cmd_send_buffer {
	enum isp4fw_buffer_type buffer_type;	/* buffer Type */
	struct isp4fw_buffer buffer;		/* buffer info */
};

struct isp4fw_cmd_set_out_ch_prop {
	enum isp4fw_pipe_out_ch ch;	/* ISP pipe out channel */
	struct isp4fw_image_prop image_prop;	/* image property */
};

struct isp4fw_cmd_enable_out_ch {
	enum isp4fw_pipe_out_ch ch;	/* ISP pipe out channel */
	u32 is_enable;			/* If enable channel or not */
};

struct isp4fw_cmd_set_stream_cfg {
	struct isp4fw_isp_stream_cfg stream_cfg; /* stream path config */
};

#endif
