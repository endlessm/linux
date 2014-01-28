/* AppliedMicro X-Gene SoC Ethernet Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Authors:	Ravi Patel <rapatel@apm.com>
 *		Iyappan Subramanian <isubramanian@apm.com>
 *		Fushen Chen <fchen@apm.com>
 *		Keyur Chudgar <kchudgar@apm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __XGENE_ENET_MAIN_H__
#define __XGENE_ENET_MAIN_H__

#include <linux/clk.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/interrupt.h>
#include <linux/if_vlan.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/io.h>
#include <misc/xgene/xgene_qmtm.h>
#include "xgene_enet_common.h"

#define XGENE_ENET_DRIVER_NAME "xgene-enet"
#define XGENE_ENET_DRIVER_VERSION "1.0"
#define XGENE_ENET_DRIVER_DESC "APM X-Gene SoC Ethernet driver"

#define XGENE_ENET_MIN_MTU		64
#define XGENE_ENET_MAX_MTU		10000

/* Note: PKT_BUF_SIZE & PKT_NXTBUF_SIZE has to be one of the following:
 * 256, 1K, 2K, 4K, 16K for ethernet to work with optimum performance.
 */
#define XGENE_ENET_PKT_BUF_SIZE		2048
#define XGENE_NUM_PKT_BUF		256

/* define Enet system struct */
struct xgene_enet_dev {
	int refcnt;
	struct timer_list link_poll_timer;
	int ipp_loaded;
	int ipp_hw_mtu;
};

enum xgene_enet_phy_poll_interval {
	PHY_POLL_LINK_ON = HZ,
	PHY_POLL_LINK_OFF = (HZ / 5)
};

enum xgene_enet_debug_cmd {
	XGENE_ENET_READ_CMD,
	XGENE_ENET_WRITE_CMD,
	XGENE_ENET_MAX_CMD
};

#define MAX_TX_QUEUES 1
#define MAX_RX_QUEUES 1

/* This is soft flow context of queue */
struct xgene_enet_qcontext {
	struct xgene_enet_pdev *pdev;
	struct xgene_qmtm_qdesc *qdesc;
	struct xgene_qmtm_msg_ext8 *msg8;
	u32 *nummsgs;
	unsigned int queue_index;
	unsigned int eqnum;
	u32 buf_size;
	unsigned int c2e_count;
	struct sk_buff * (*skb);
	struct xgene_enet_qcontext *c2e_skb;
	struct xgene_enet_qcontext *c2e_page;
	struct napi_struct napi;
	char irq_name[16];
};

/* Queues related parameters per Enet port */
#define ENET_MAX_PBN	8
#define ENET_MAX_QSEL	8

struct eth_wqids {
	u16 qtype;
	u16 qid;
	u16 arb;
	u16 qcount;
	u16 qsel[ENET_MAX_QSEL];
};

struct eth_fqids {
	u16 qid;
	u16 pbn;
};

struct eth_queue_ids {
	u16 default_tx_qid;
	u16 tx_count;
	u16 tx_idx;
	struct eth_wqids tx[ENET_MAX_PBN];
	u16 default_rx_qid;
	u16 rx_count;
	u16 rx_idx;
	struct eth_wqids rx[ENET_MAX_PBN];
	u16 default_rx_fp_qid;
	u16 default_rx_fp_pbn;
	struct eth_fqids rx_fp[ENET_MAX_PBN];
	u16 default_rx_nxtfp_qid;
	u16 default_rx_nxtfp_pbn;
	struct eth_fqids rx_nxtfp[ENET_MAX_PBN];
	struct eth_fqids hw_fp;
	u16 default_hw_tx_qid;
	struct eth_fqids hw_tx[ENET_MAX_PBN];
	struct eth_wqids comp[ENET_MAX_PBN];
	u16 default_comp_qid;
	u32 qm_ip;
};

struct xgene_enet_platform_data {
	const char *sname;
	int irq;
	u32 phy_id;
	u8 ethaddr[6];
};

/* APM ethernet per port data */
struct xgene_enet_pdev {
	struct net_device *ndev;
	struct mii_bus *mdio_bus;
	struct phy_device *phy_dev;
	int phy_link;
	int phy_speed;
	struct clk *clk;
	struct device_node *node;
	struct platform_device *plat_dev;
	struct xgene_qmtm_sdev *sdev;
	struct xgene_enet_qcontext *tx[MAX_TX_QUEUES];
	struct xgene_enet_qcontext *rx_skb_pool[MAX_RX_QUEUES];
	u32 num_tx_queues;
	struct xgene_enet_qcontext *rx[MAX_RX_QUEUES];
	struct xgene_enet_qcontext *tx_completion[MAX_TX_QUEUES];
	u32 num_rx_queues;
	struct net_device_stats nstats;
	struct xgene_enet_detailed_stats stats;
	char *dev_name;
	int uc_count;
	struct eth_queue_ids qm_queues;
	u32 rx_buff_cnt, tx_cqt_low, tx_cqt_hi;
	int mss;
	unsigned int enet_err_irq;
	unsigned int enet_mac_err_irq;
	unsigned int enet_qmi_err_irq;
	struct xgene_enet_priv priv;
};

/* Ethernet raw register write, read routines */
void xgene_enet_wr32(void *addr, u32 data);
void xgene_enet_rd32(void *addr, u32 *data);

u32 xgene_enet_get_port(struct xgene_enet_pdev *pdev);

void xgene_enet_init_priv(struct xgene_enet_priv *priv);

int xgene_enet_parse_error(u8 LErr, int qid);
void xgene_enet_register_err_irqs(struct net_device *ndev);

extern const struct ethtool_ops xgene_ethtool_ops;
#endif /* __XGENE_ENET_MAIN_H__ */
