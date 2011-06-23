/* 
 * Copyright 2008-2011 Freescale Semiconductor Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DPA_H
#define __DPA_H

#include <linux/ethtool.h>	/* struct ethtool_ops */
#include <linux/netdevice.h>
#include <linux/list.h>		/* struct list_head */
#include <linux/workqueue.h>	/* struct work_struct */
#include <linux/skbuff.h>
#include <linux/hardirq.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/dcache.h>	/* struct dentry */
#endif

#include <linux/fsl_qman.h>	/* struct qman_fq */

#include "dpaa_eth-common.h"

#include "mac.h"		/* struct mac_device */


/* number of Tx queues to FMan */
#define DPAA_ETH_TX_QUEUES	8
#define DPAA_ETH_RX_QUEUES	128

struct pcd_range {
	uint32_t			 base;
	uint32_t			 count;
};

struct dpa_bp {
	struct bman_pool		*pool;
	uint8_t				bpid;
	struct device			*dev;
	size_t				count;
	size_t				size;
	bool				seed_pool;
	dma_addr_t			paddr;
	void				*vaddr;
	int kernel_pool;
	int *percpu_count;
	int *needs_refill;
	atomic_t refs;
};

struct dpa_percpu_priv_s {
	struct net_device *net_dev;
	int *dpa_bp_count;
	struct dpa_bp *dpa_bp;
	struct napi_struct napi;
	u32 start_tx;
	u32 in_interrupt;
	u32 ingress_calls;
	u32 tx_returned;
	u32 tx_confirm;
	struct net_device_stats	 stats;
};

struct dpa_priv_s {
	struct dpa_bp *dpa_bp;
	size_t bp_count;
	int shared;
	struct net_device *net_dev;

	uint16_t		 channel;	/* "fsl,qman-channel-id" */
	struct list_head	 dpa_fq_list;
	struct qman_fq		*egress_fqs[DPAA_ETH_TX_QUEUES];

	struct mac_device	*mac_dev;

	struct dpa_percpu_priv_s	*percpu_priv;
#ifdef CONFIG_DEBUG_FS
	struct dentry		*debugfs_file;
#endif

	uint32_t		 msg_enable;	/* net_device message level */
	struct dpa_ptp_tsu	 *tsu;
};

extern const struct ethtool_ops dpa_ethtool_ops;
extern int fsl_fman_phy_maxfrm;

static inline int dpaa_eth_napi_schedule(struct dpa_percpu_priv_s *percpu_priv)
{
	if (unlikely(in_irq())) {
		/* Disable QMan IRQ and invoke NAPI */
		int ret = qman_irqsource_remove(QM_PIRQ_DQRI);
		if (likely(!ret)) {
			napi_schedule(&percpu_priv->napi);
			return 1;
		}
	}
	return 0;
}

#endif	/* __DPA_H */
