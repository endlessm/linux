/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025 Advanced Micro Devices, Inc.
 */

#ifndef _ISP4_INTERFACE_
#define _ISP4_INTERFACE_

#define ISP4IF_RB_MAX (25)
#define ISP4IF_RESP_CHAN_TO_RB_OFFSET (9)
#define ISP4IF_RB_PMBMAP_MEM_SIZE (16 * 1024 * 1024 - 1)
#define ISP4IF_RB_PMBMAP_MEM_CHUNK (ISP4IF_RB_PMBMAP_MEM_SIZE \
	/ (ISP4IF_RB_MAX - 1))
#define ISP4IF_HOST2FW_COMMAND_SIZE (sizeof(struct isp4fw_cmd))
#define ISP4IF_FW_CMD_BUF_COUNT 4
#define ISP4IF_FW_RESP_BUF_COUNT 4
#define ISP4IF_MAX_NUM_HOST2FW_COMMAND (40)
#define ISP4IF_FW_CMD_BUF_SIZE (ISP4IF_MAX_NUM_HOST2FW_COMMAND \
	* ISP4IF_HOST2FW_COMMAND_SIZE)
#define ISP4IF_MAX_SLEEP_COUNT (10)
#define ISP4IF_MAX_SLEEP_TIME (33)

#define ISP4IF_META_INFO_BUF_SIZE ALIGN(sizeof(struct isp4fw_meta_info), 0x8000)
#define ISP4IF_MAX_STREAM_BUF_COUNT 8

#define ISP4IF_FW_LOG_RINGBUF_SIZE (2 * 1024 * 1024)

#define ISP4IF_MAX_CMD_RESPONSE_BUF_SIZE (4 * 1024)

#define GET_ISP4IF_REG_BASE(ispif) (((ispif))->mmio)

enum isp4if_stream_id {
	ISP4IF_STREAM_ID_GLOBAL = 0,
	ISP4IF_STREAM_ID_1 = 1,
	ISP4IF_STREAM_ID_MAX = 4
};

enum isp4if_status {
	ISP4IF_STATUS_PWR_OFF,
	ISP4IF_STATUS_PWR_ON,
	ISP4IF_STATUS_FW_RUNNING,
	ISP4IF_FSM_STATUS_MAX
};

struct isp4if_gpu_mem_info {
	u32	mem_domain;
	u64	mem_size;
	u32	mem_align;
	u64	gpu_mc_addr;
	void	*sys_addr;
	void	*mem_handle;
};

struct isp4if_img_buf_info {
	struct {
		void *sys_addr;
		u64 mc_addr;
		u32 len;
	} planes[3];
};

struct isp4if_img_buf_node {
	struct list_head node;
	struct isp4if_img_buf_info buf_info;
};

struct isp4if_cmd_element {
	struct list_head list;
	u32 seq_num;
	u32 cmd_id;
	enum isp4if_stream_id stream;
	u64 mc_addr;
	wait_queue_head_t *wq;
	u32 *wq_cond;
	struct isp4if_gpu_mem_info *gpu_pkg;
};

struct isp4_interface {
	struct device *dev;
	void __iomem *mmio;

	struct mutex cmdq_mutex; /* used for cmdq access */
	struct mutex bufq_mutex; /* used for bufq access */
	struct mutex isp4if_mutex; /* used to send fw cmd and read fw log */

	struct list_head cmdq; /* commands sent to fw */
	struct list_head bufq; /* buffers sent to fw */

	enum isp4if_status status;
	u32 host2fw_seq_num;

	/* FW ring buffer configs */
	u32 cmd_rb_base_index;
	u32 resp_rb_base_index;
	u32 aligned_rb_chunk_size;

	/* ISP fw buffers */
	struct isp4if_gpu_mem_info *fw_log_buf;
	struct isp4if_gpu_mem_info *fw_cmd_resp_buf;
	struct isp4if_gpu_mem_info *fw_mem_pool;
	struct isp4if_gpu_mem_info *
		metainfo_buf_pool[ISP4IF_MAX_STREAM_BUF_COUNT];
};

static inline void isp4if_split_addr64(u64 addr, u32 *lo, u32 *hi)
{
	if (lo)
		*lo = addr & 0xffffffff;
	if (hi)
		*hi = addr >> 32;
}

static inline u64 isp4if_join_addr64(u32 lo, u32 hi)
{
	return (((u64)hi) << 32) | (u64)lo;
}

int isp4if_f2h_resp(struct isp4_interface *ispif, enum isp4if_stream_id stream, void *response);

int isp4if_send_command(struct isp4_interface *ispif, u32 cmd_id, void *package,
			u32 package_size);

int isp4if_send_command_sync(struct isp4_interface *ispif, u32 cmd_id, void *package,
			     u32 package_size, u32 timeout);

struct isp4if_cmd_element *isp4if_rm_cmd_from_cmdq(struct isp4_interface *ispif, u32 seq_num,
						   u32 cmd_id);

void isp4if_clear_cmdq(struct isp4_interface *ispif);

void isp4if_clear_bufq(struct isp4_interface *ispif);

void isp4if_dealloc_buffer_node(struct isp4if_img_buf_node *buf_node);

struct isp4if_img_buf_node *isp4if_alloc_buffer_node(struct isp4if_img_buf_info *buf_info);

struct isp4if_img_buf_node *isp4if_dequeue_buffer(struct isp4_interface *ispif);

int isp4if_queue_buffer(struct isp4_interface *ispif, struct isp4if_img_buf_node *buf_node);

int isp4if_stop(struct isp4_interface *ispif);

int isp4if_start(struct isp4_interface *ispif);

int isp4if_deinit(struct isp4_interface *ispif);

int isp4if_init(struct isp4_interface *ispif, struct device *dev, void __iomem *isp_mmip);

#endif
