/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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
/*

 @File          lnxwrp_resources.c

 @Description   FMD wrapper resource allocation functions.

*/

#include <linux/version.h>
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <config/modversions.h>
#endif /* MODVERSIONS */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/skbuff.h>

#include "lnxwrp_resources.h"

extern int fsl_fman_phy_maxfrm;	/* MAC file */

static struct device_node *match_mac_to_dpaa_port(struct device_node
						  *enet_mac_node)
{
	struct device_node *dpaa_node = NULL;
	struct device_node *dpaa_itf = NULL;

	/* find DPAA node starting from root */
	dpaa_node = of_find_compatible_node(NULL, NULL, "fsl,dpaa");
	if (dpaa_node) {
		/* for all dpaa ports check which one refers this mac node. */
		for_each_child_of_node(dpaa_node, dpaa_itf) {
			struct device_node *by_handle_enet_mac_node = NULL;
			const phandle *phandle_prop = NULL;
			int lenp = 0;

			phandle_prop =
				(typeof(phandle_prop))
				of_get_property(dpaa_itf, "fsl,fman-mac",
						&lenp);
			if (phandle_prop == NULL)
				continue;

			if (WARN_ON(lenp != sizeof(phandle)))
				return NULL;

			by_handle_enet_mac_node =
				of_find_node_by_phandle(*phandle_prop);
			if (unlikely(by_handle_enet_mac_node == NULL))
				return NULL;

			/* check */
			if (by_handle_enet_mac_node == enet_mac_node) {
				of_node_put(by_handle_enet_mac_node);
				return dpaa_itf;
			}

			of_node_put(by_handle_enet_mac_node);
		}
		of_node_put(dpaa_node);
	}

	return NULL;
}

static struct device_node *match_fman_port_to_mac(struct device_node *fm_node,
						  struct device_node
						  *fm_port_node)
{
	struct device_node *fm_node_idx = NULL;

	/* for all enet nodes (macs) check which one refers this FMan port. */
	for_each_child_of_node(fm_node, fm_node_idx) {
		if (of_device_is_compatible(fm_node_idx, "fsl,fman-1g-mac") ||
		    of_device_is_compatible(fm_node_idx,
					    "fsl,fman-10g-mac")) {
			struct device_node *fman_port_node_rx = NULL;
			struct device_node *fman_port_node_tx = NULL;
			/* RX is first */
			fman_port_node_rx = of_parse_phandle(fm_node_idx,
					"fsl,port-handles", 0);
			if (unlikely(fman_port_node_rx == NULL))
				continue;
			/* TX is second */
			fman_port_node_tx = of_parse_phandle(fm_node_idx,
					"fsl,port-handles", 1);
			if (unlikely(fman_port_node_tx == NULL)) {
				of_node_put(fman_port_node_rx);
				continue;
			}

			/* check */
			if (fman_port_node_rx == fm_port_node
				|| fman_port_node_tx == fm_port_node) {
				of_node_put(fman_port_node_rx);
				of_node_put(fman_port_node_tx);
				return fm_node_idx;
			}

			of_node_put(fman_port_node_rx);
			of_node_put(fman_port_node_tx);
		}
	}

	return NULL;
}

static bool is_fman_port_active(struct device_node *fm_node,
				struct device_node *fm_port_node)
{
	struct device_node *enet_mac_node = NULL;
	struct device_node *itf_node = NULL;

	/* Which MAC node refers to this FMan port. */
	enet_mac_node = match_fman_port_to_mac(fm_node, fm_port_node);

	if (unlikely(enet_mac_node == NULL))
		return false;

	/* Which dpaa port node refers this MAC node. */
	itf_node = match_mac_to_dpaa_port(enet_mac_node);
	of_node_put(enet_mac_node);

	if (unlikely(!itf_node))
		return false;

	/* check if itf (DPAA ports) is available.
	 * if available, means that the FMan port is
	 * also available - return true
	 */
	if (!of_device_is_available(itf_node)) {
		of_node_put(itf_node);
		return false;
	}
	of_node_put(itf_node);

	return true;
}

int fm_set_active_fman_ports(struct platform_device *of_dev,
			  t_LnxWrpFmDev *p_LnxWrpFmDev)
{
	struct device_node *fm_node = NULL;
	struct device_node *fm_port_node = NULL;

	memset(&p_LnxWrpFmDev->fm_active_ports_info, 0,
	       sizeof(struct fm_active_ports));

	/* get FMan node */
	fm_node = of_dev->dev.of_node;

	/* for all ports which belong to this FMan, check if they are active.
	 * If active, set their parameters. */
	for_each_child_of_node(fm_node, fm_port_node) {

		/* OH FMan ports */
		if (of_device_is_compatible(fm_port_node,
						"fsl,fman-port-oh"))
			/* all oh ports are active */
			p_LnxWrpFmDev->fm_active_ports_info.num_oh_ports++;

		if (!is_fman_port_active(fm_node, fm_port_node))
			continue;

		/* 10g TX FMan ports */
		if (of_device_is_compatible(fm_port_node,
						"fsl,fman-port-10g-tx"))
			p_LnxWrpFmDev->fm_active_ports_info.num_tx10_ports++;

		/* 10g RX FMan ports */
		else if (of_device_is_compatible(fm_port_node,
						"fsl,fman-port-10g-rx"))
			p_LnxWrpFmDev->fm_active_ports_info.num_rx10_ports++;

		/* 1G TX FMan ports */
		else if (of_device_is_compatible(fm_port_node,
						"fsl,fman-port-1g-tx"))
			p_LnxWrpFmDev->fm_active_ports_info.num_tx_ports++;

		/* 1G RX FMan ports */
		else if (of_device_is_compatible(fm_port_node,
						"fsl,fman-port-1g-rx"))
			p_LnxWrpFmDev->fm_active_ports_info.num_rx_ports++;
	}

	/* If performance is needed no oh port is probed
	 * except the one used for host command. */
#if defined(CONFIG_FMAN_DISABLE_OH_TO_REUSE_RESOURCES)
	if (p_LnxWrpFmDev->fm_active_ports_info.num_oh_ports)
		p_LnxWrpFmDev->fm_active_ports_info.num_oh_ports = 1;

	printk(KERN_WARNING "FMAN(%u)-Performance mode - no OH support...\n",
	       p_LnxWrpFmDev->id);
#endif

	return 0;
}

#ifdef FM_FIFO_ALLOCATION_OLD_ALG
/* BPOOL size is constant and equal w/ DPA_BP_SIZE */
static uint32_t get_largest_buf_size(uint32_t max_rx_frame_size, uint32_t buf_size)
{
	uint32_t priv_data_size = 16;		/* DPA_PRIV_DATA_SIZE */
	uint32_t hash_results_size = 16;	/* DPA_HASH_RESULTS_SIZE */
	uint32_t parse_results_size =
		sizeof(t_FmPrsResult);		/* DPA_PARSE_RESULTS_SIZE */
	uint32_t bp_head = priv_data_size + hash_results_size
		+ parse_results_size; 		/* DPA_BP_HEAD */
	uint32_t bp_size = bp_head + max_rx_frame_size
		+ NET_IP_ALIGN;			/* DPA_BP_SIZE */

	return CEIL_DIV(bp_size, buf_size);
}
#endif

/* Calculate the fifosize based on MURAM allocation, number of ports, dpde
   value and s/g software support (! Kernel does not suport s/g).

   Algorithm summary:
   - Calculate the the minimum fifosize required for every type of port
   (TX,RX for 1G, 2.5G and 10G).
   - Set TX the minimum fifosize required.
   - Distribute the remaining buffers (after all TX were set) to RX ports
   based on:
     1G   RX = Remaining_buffers * 1/(1+2.5+10)
     2.5G RX = Remaining_buffers * 2.5/(1+2.5+10)
     10G  RX = Remaining_buffers * 10/(1+2.5+10)
   - if the RX is smaller than the minimum required, then set the minimum
   required
   - In the end distribuite the leftovers if there are any (due to
   unprecise calculus) or if over allocation cat some buffers from all RX
   ports w/o pass over minimum required treshold, but if there must be
   pass the treshold in order to cat the over allocation ,then this
   configuration can not be set - KERN_ALERT.
*/
int fm_precalculate_fifosizes(t_LnxWrpFmDev *p_LnxWrpFmDev, int muram_fifo_size)
{

	/* input parameters */
	struct fm_active_ports *fm_active_ports_info = NULL;
	int num_1g_ports = 0;
	int num_2g5_ports = 0;
	int num_10g_ports = 0;
	int num_oh_ports = 0;

	/* output parameters */
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	int oh_buff = 0;
	int tx_1g_bufs = 0, rx_1g_bufs = 0;
	int tx_2g5_bufs = 0, rx_2g5_bufs = 0;
	int tx_10g_bufs = 0, rx_10g_bufs = 0;
	int err = 0;

	/* throughput parameters: divide it by 10 when used */
	int gb1g = 10, gb2g5 = 25, gb10g = 100, gb_sum = 0;

	/* buffers parameters */
	int buf_size = 0x100;	 /* Buffer unit size */
	int total_no_buffers = 0; /* Calculus based on MURAM size for
				     fifos and buf. unit  size */

	int shared_ext_buff = 0; /* External buffers allocated - LLD
				    boundaries:DEFAULT_PORT_extraSizeOfFifo */

	int min_tx_1g_2g5_bufs = 0; /* minimum TX1g buffers required
				       (see refman.) */
	int min_tx_10g_bufs = 0; /* minimum TX10g buffers required
				       (see refman.) */
	int min_rx_bufs = 0; /* minimum RX buffers required (see refman.) */

	/* Buffer sizes calculus */
	int max_frame_size =
		fsl_fman_phy_maxfrm ? fsl_fman_phy_maxfrm :
		CONFIG_DPA_MAX_FRM_SIZE;
	int remaining_bufs = 0;
	int rx_1g_bufs_ceil = 0, rx_2g5_bufs_ceil = 0, rx_10g_bufs_ceil = 0;
	int rx_2g5_max_bufs = 0, rx_10g_max_bufs = 0;
	int rx_1g_used = 0, rx_1g_2g5_used = 0, rx_1g_10g_used =0,
		rx_2g5_used = 0, rx_2g5_10g_used = 0, rx_1g_2g5_10g_used = 0;

	/* overflow checking */
	int tot_rx_buffs, tot_tx_buffs, tot_oh_buffs, tot_used_buffs,
		leftovers = 0;
	int overflow = 0;
	bool loop = false;

	/* check input parameters correctness */
	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_active_ports_info = &p_LnxWrpFmDev->fm_active_ports_info;
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;
	ASSERT_COND(fm_active_ports_info != NULL);
	ASSERT_COND(fm_resource_settings_info != NULL);
	ASSERT_COND(fm_active_ports_info->num_tx_ports ==
		    fm_active_ports_info->num_rx_ports);
	ASSERT_COND(fm_active_ports_info->num_tx25_ports ==
		    fm_active_ports_info->num_tx25_ports);
	ASSERT_COND(fm_active_ports_info->num_tx10_ports ==
		    fm_active_ports_info->num_tx10_ports);
	ASSERT_COND(max_frame_size != 0);
	ASSERT_COND(muram_fifo_size != 0);

	/* set input parameters */
	num_1g_ports = fm_active_ports_info->num_tx_ports;
	num_2g5_ports = fm_active_ports_info->num_tx25_ports;
	num_10g_ports = fm_active_ports_info->num_tx10_ports;
	num_oh_ports = fm_active_ports_info->num_oh_ports;

	/* throughput calculus */
	gb_sum = gb1g * num_1g_ports + gb2g5 * num_2g5_ports
			+ gb10g * num_10g_ports;	/* divide it by 10 */

	/* Base buffer calculus */
	oh_buff = DPDE_1G + 4;	/* should be:
		get_largest_buf_size(max_frame_size, buf_size),
		but LLD: DPDE + 4 */
	total_no_buffers = muram_fifo_size / buf_size;

	min_tx_1g_2g5_bufs = CEIL_DIV(max_frame_size, buf_size)
			+ DPDE_1G + 3 + 1; /* +1 to handle Jumbo Frames */
	min_tx_10g_bufs = CEIL_DIV(max_frame_size, buf_size)
			+ DPDE_10G + 3 + 1; /* +1 to handle Jumbo Frames */

	{
#ifdef FM_FIFO_ALLOCATION_OLD_ALG
		uint8_t fm_rev_major = 0;
		fm_rev_major =
			(uint8_t) ((*
				    ((volatile uint32_t *)
				     UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr +
						 0x000c30c4)) & 0xff00) >> 8);

		if (fm_rev_major < 4)
			min_rx_bufs =
				get_largest_buf_size(max_frame_size,
						     buf_size) + 7;
		else
#endif
			min_rx_bufs = 8;
	}

	shared_ext_buff = num_10g_ports ? 32 : 16; /* LLD boundaries:
					DEFAULT_PORT_extraSizeOfFifo */

	/* TX ports will have minimum required buffers
	   Calculus of the remaining buffers for all RX ports */
	tx_1g_bufs = num_1g_ports ? min_tx_1g_2g5_bufs : 0;
	tx_2g5_bufs = num_2g5_ports ? min_tx_1g_2g5_bufs : 0;
	tx_10g_bufs = num_10g_ports ? min_tx_10g_bufs : 0;

	remaining_bufs = total_no_buffers -
		oh_buff * num_oh_ports -
		num_1g_ports * min_tx_1g_2g5_bufs -
		num_2g5_ports * min_tx_1g_2g5_bufs -
		num_10g_ports * min_tx_10g_bufs - shared_ext_buff;

	if (remaining_bufs < 0) {
		printk(KERN_ALERT
		       "This configuration will not work due to low number of"
			" buffers (%u buffers)...\n",
		       total_no_buffers);
		err = -1;
		goto precalculated_fifosize_out;
	}

	/* Per port buffer size calculus
	   . for TX ports give always minimum required
	   . for RX ports give whatever left scaled per port type */
	/* ------------------------------------------------------- */
	if (num_1g_ports) {
		rx_1g_bufs_ceil =
			(gb_sum /
			 10) ? CEIL_DIV(((remaining_bufs * gb1g) / 10),
					(gb_sum / 10)) : 0;
		rx_1g_bufs = MAX(min_rx_bufs, rx_1g_bufs_ceil);
		rx_1g_used = rx_1g_bufs - rx_1g_bufs_ceil; /* always >= 0 */
		/* distribute to 2.5g and 10g ports */
		rx_1g_2g5_used =
			(num_2g5_ports +
			 num_10g_ports) ? CEIL_DIV(rx_1g_used * num_1g_ports *
						  num_2g5_ports,
						  num_2g5_ports +
						  num_10g_ports) : 0;
		rx_1g_10g_used =
			(num_2g5_ports +
			 num_10g_ports) ? CEIL_DIV(rx_1g_used * num_1g_ports *
						  num_10g_ports,
						  num_2g5_ports +
						  num_10g_ports) : 0;
	}

	if (num_2g5_ports) {
		rx_2g5_bufs_ceil =
			(gb_sum /
			 10) ? CEIL_DIV(((remaining_bufs * gb2g5) / 10),
					(gb_sum / 10)) : 0;
		rx_2g5_max_bufs = MAX(min_rx_bufs, rx_2g5_bufs_ceil);
		rx_2g5_bufs =
			MAX(min_rx_bufs, rx_2g5_max_bufs - rx_1g_2g5_used);
		rx_2g5_used = rx_2g5_bufs - rx_2g5_bufs_ceil; /* always >= 0 */
		/* distribute to 10g ports */
		rx_2g5_10g_used =
			num_10g_ports ? CEIL_DIV(rx_2g5_used * num_2g5_ports,
						num_10g_ports) : 0;
	}

	if (num_10g_ports) {
		rx_10g_bufs_ceil =
			(gb_sum /
			 10) ? CEIL_DIV(((remaining_bufs * gb10g) / 10),
					(gb_sum / 10)) : 0;
		rx_10g_max_bufs = MAX(min_rx_bufs, rx_10g_bufs_ceil);
		/* keep count of all distribution */
		rx_1g_2g5_10g_used = rx_1g_10g_used + rx_2g5_10g_used;
		rx_10g_bufs =
			MAX(min_rx_bufs,
			    rx_10g_max_bufs - rx_1g_2g5_10g_used);
	}

	/* overflow-leftover calculus */
	tot_rx_buffs = rx_1g_bufs * num_1g_ports +
		rx_2g5_bufs * num_2g5_ports + rx_10g_bufs * num_10g_ports;
	tot_tx_buffs = tx_1g_bufs * num_1g_ports +
		tx_2g5_bufs * num_2g5_ports + tx_10g_bufs * num_10g_ports;
	tot_oh_buffs = oh_buff * num_oh_ports;
	tot_used_buffs =
		tot_oh_buffs + tot_tx_buffs + tot_rx_buffs + shared_ext_buff;

	overflow = tot_used_buffs - total_no_buffers;
	/* used more than available */
	if (overflow > 0) {
		loop = true;
		while (overflow > 0 && loop) {
			loop = false;
			if (overflow && num_10g_ports
			    && rx_10g_bufs > min_rx_bufs) {
				rx_10g_bufs--;
				overflow -= num_10g_ports;
				loop = true;
			}
			if (overflow && num_2g5_ports
			    && rx_2g5_bufs > min_rx_bufs) {
				rx_2g5_bufs--;
				overflow -= num_2g5_ports;
				loop = true;
			}
			if (overflow && num_1g_ports
			    && rx_1g_bufs > min_rx_bufs) {
				rx_1g_bufs--;
				overflow -= num_1g_ports;
				loop = true;
			}
		}

		if (overflow > 0) {
			printk(KERN_ALERT
			       "This configuration will not work due to over"
				" buffer allocation (%d buffers)...\n",
			       overflow);
			err = -1;
			goto precalculated_fifosize_out;
		}
	}
	/* left a few buffers */
	else if (overflow < 0) {
		leftovers = total_no_buffers - tot_used_buffs;
		loop = true;
		while (leftovers > 0 && loop) {
			loop = false;
			if (leftovers && num_1g_ports) {
				rx_1g_bufs++;
				leftovers -= num_1g_ports;
				loop = true;
			}

			if (leftovers && num_2g5_ports) {
				rx_2g5_bufs++;
				leftovers -= num_2g5_ports;
				loop = true;
			}

			if (leftovers && num_10g_ports) {
				rx_10g_bufs++;
				leftovers -= num_10g_ports;
				loop = true;
			}
		}
	}

	/* set fifosizes for this FMan ports */
	fm_resource_settings_info->tx1g_num_buffers = tx_1g_bufs;
	fm_resource_settings_info->rx1g_num_buffers = rx_1g_bufs;
	fm_resource_settings_info->tx2g5_num_buffers = tx_2g5_bufs;
	fm_resource_settings_info->rx2g5_num_buffers = rx_2g5_bufs;
	fm_resource_settings_info->tx10g_num_buffers = tx_10g_bufs;
	fm_resource_settings_info->rx10g_num_buffers = rx_10g_bufs;
	fm_resource_settings_info->oh_num_buffers = oh_buff;
	fm_resource_settings_info->shared_ext_buffers = shared_ext_buff;

precalculated_fifosize_out:
	printk(KERN_INFO " FMAN(%u) Fifo size settings:\n",
	       p_LnxWrpFmDev->id);
	printk(KERN_INFO "  - Total buffers available(%u - 256B/buffer)\n",
	       total_no_buffers);
	printk(KERN_INFO "  - Total throughput(%uGbps)\n", (gb_sum / 10));
	printk(KERN_INFO "  - Max frame size(%uB)\n", max_frame_size);
	if (num_1g_ports) {
		printk(KERN_INFO
		       "  - 1G ports TX %u(%u bufs set (min: %u))\n",
		       num_1g_ports, tx_1g_bufs, min_tx_1g_2g5_bufs);
		printk(KERN_INFO
		       "  - 1G ports RX %u(%u bufs set (min: %u))\n",
		       num_1g_ports, rx_1g_bufs, min_rx_bufs);
	}
	if (num_2g5_ports) {
		printk(KERN_INFO
		       "  - 2.5G ports TX %u(%u bufs set (min: %u))\n",
		       num_2g5_ports, tx_2g5_bufs, min_tx_1g_2g5_bufs);
		printk(KERN_INFO
		       "  - 2.5G ports RX %u(%u bufs set (min: %u))\n",
		       num_2g5_ports, rx_2g5_bufs, min_rx_bufs);
	}
	if (num_10g_ports) {
		printk(KERN_INFO
		       "  - 10G ports TX %u(%u bufs set (min: %u))\n",
		       num_10g_ports, tx_10g_bufs, min_tx_10g_bufs);
		printk(KERN_INFO
		       "  - 10G ports RX %u(%u bufs set (min: %u))\n",
		       num_10g_ports, rx_10g_bufs, min_rx_bufs);
	}
	if (num_oh_ports)
		printk(KERN_INFO "  - OH-HC ports %u(%u)\n", num_oh_ports,
		       oh_buff);
	printk(KERN_INFO "  - Shared extra buffers(%u)\n", shared_ext_buff);

	return err;
}

int fm_set_precalculate_fifosize(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	struct fm_active_ports *fm_active_ports_info = NULL;
	t_FmPortRsrc portRsrc;
	t_Error errCode;
	uint32_t buf_size = 0x100;

	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;
	fm_active_ports_info = &p_LnxWrpFmDev->fm_active_ports_info;

	memset(&portRsrc, 0, sizeof(t_FmPortRsrc));

/* IF 1G PORT */
	if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX) {
		portRsrc.num =
			fm_resource_settings_info->tx1g_num_buffers * buf_size;
		portRsrc.extra = 0;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX) {
		portRsrc.num =
			fm_resource_settings_info->rx1g_num_buffers * buf_size;
		portRsrc.extra =
			fm_resource_settings_info->shared_ext_buffers *
			buf_size;
	}
/* IF 2.5G PORT */
	/* TODO: Not supported by LLD yet. */

/* IF 10G PORT */
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_TX_10G) {
		portRsrc.num =
			fm_resource_settings_info->tx10g_num_buffers *
			buf_size;
		portRsrc.extra = 0;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX_10G) {
		portRsrc.num =
			fm_resource_settings_info->rx10g_num_buffers *
			buf_size;
		portRsrc.extra =
			fm_resource_settings_info->shared_ext_buffers *
			buf_size;
	} else { /* IF OH PORT */
		portRsrc.num =
			fm_resource_settings_info->oh_num_buffers * buf_size;
		portRsrc.extra = 0;
	}

	errCode = FM_PORT_SetSizeOfFifo(p_LnxWrpFmPortDev->h_Dev, &portRsrc);
	if (errCode != E_OK) {
		printk(KERN_WARNING
		       "FM_PORT_SetSizeOfFifo failed (errCode:0x%2x)",
		       errCode);
		return -EIO;
	}

	return 0;
}

/* Compute FMan open DMA based on total number of open DMAs and
 * number of available FMan ports.
 *
 * By default 10g ports are set to input parameters. The other ports
 * tries to keep the proportion rx=2tx open DMAs or thresholds.
 *
 * If leftovers, then those will be set as shared.
 *
 * If after computing overflow appears, then it decrements open DMA
 * for all ports w/o cross the thresholds. If the thresholds are meet
 * and is still overflow, then it returns error.
 */
int fm_precalculate_open_dma(t_LnxWrpFmDev *p_LnxWrpFmDev,
			  int max_fm_open_dma,
			  int default_tx_10g_dmas,
			  int default_rx_10g_dmas,
			  int min_tx_10g_treshold, int min_rx_10g_treshold)
{
	/* input parameters */
	struct fm_active_ports *fm_active_ports_info = NULL;
	int num_1g_ports = 0;
	int num_2g5_ports = 0;
	int num_10g_ports = 0;
	int num_oh_ports = 0;

	/* output parameters */
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	int tx_1g_dmas = 0, rx_1g_dmas = 0;
	int tx_2g5_dmas = 0, rx_2g5_dmas = 0;
	int tx_10g_dmas = 0, rx_10g_dmas = 0;
	int oh_dmas = 0;
	int shared_ext_open_dma = 0;
	int err = 0;

	/* open dma calculus */
	int remaing_dmas = 0;
	int rx_tx_raport =
		FM_OPENDMA_RX_TX_RAPORT; /* RX = FM_OPENDMA_RX_TX_RAPORT *TX */
	int min_tx_1_2g5_treshold = 1;
	int min_rx_1_2g5_treshold = 1;
	int max_open_dma_treshold = 16;	/* LLD: MAX_NUM_OF_DMAS */
	int max_ext_open_dma_treshold = 8; /* LLD: MAX_NUM_OF_EXTRA_DMAS */

	int open_dmas_computed = 0;
	int weighted_remaining_ports = 0;
	int overflow = 0;
	bool re_loop = false;

	/* check input parameters correctness */
	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_active_ports_info = &p_LnxWrpFmDev->fm_active_ports_info;
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;
	ASSERT_COND(fm_active_ports_info != NULL);
	ASSERT_COND(fm_resource_settings_info != NULL);
	ASSERT_COND(fm_active_ports_info->num_tx_ports ==
		    fm_active_ports_info->num_rx_ports);
	ASSERT_COND(fm_active_ports_info->num_tx25_ports ==
		    fm_active_ports_info->num_tx25_ports);
	ASSERT_COND(fm_active_ports_info->num_tx10_ports ==
		    fm_active_ports_info->num_tx10_ports);
	ASSERT_COND(min_tx_10g_treshold <= max_open_dma_treshold);
	ASSERT_COND(min_tx_10g_treshold <= max_open_dma_treshold);

	/* set input parameters */
	num_1g_ports = fm_active_ports_info->num_tx_ports;
	num_2g5_ports = fm_active_ports_info->num_tx25_ports;
	num_10g_ports = fm_active_ports_info->num_tx10_ports;
	num_oh_ports = fm_active_ports_info->num_oh_ports;

	/* compute open DMAs per port */
	/* ------------------------------------------------------- */
	if (num_10g_ports) {
		tx_10g_dmas = default_tx_10g_dmas;	/* per 10G TX port */
		rx_10g_dmas = default_rx_10g_dmas;	/* per 10G RX port */
	}
	if (num_oh_ports)
		oh_dmas = 1;	/* per OH port */

	/* should this be null? or LLD:
		DEFAULT_PORT_extraNumOfOpenDmas:10g-8,else 1 */
	shared_ext_open_dma = 0;

	/* based on total number of ports set open DMAs for all other ports */
	remaing_dmas = max_fm_open_dma -
		(oh_dmas * num_oh_ports) -
		(tx_10g_dmas * num_10g_ports + rx_10g_dmas * num_10g_ports) -
		shared_ext_open_dma;

	if (remaing_dmas < 0) {
		printk(KERN_ALERT
			"This configuration will not work due to low number"
			" of open dmas (%u open dmas)...\n",
		       max_fm_open_dma);
		err = -1;
		goto precalculated_open_dma_out;
	}

	weighted_remaining_ports =
		/*tx */ num_1g_ports * rx_tx_raport + /*rx */ num_1g_ports +
		/*tx */ num_2g5_ports * rx_tx_raport + /*rx */ num_2g5_ports;

	/* compute the other ports */
	if (num_1g_ports) {
		tx_1g_dmas =
			MAX(MIN
			    (ROUND_DIV
			     (remaing_dmas, weighted_remaining_ports),
			     max_open_dma_treshold), min_tx_1_2g5_treshold);
		rx_1g_dmas =
			MAX(MIN
			    (ROUND_DIV
			     ((remaing_dmas * rx_tx_raport),
			      weighted_remaining_ports),
			     max_open_dma_treshold), min_rx_1_2g5_treshold);
	}
	if (num_2g5_ports) {
		tx_2g5_dmas =
			MAX(MIN
			    (CEIL_DIV(remaing_dmas, weighted_remaining_ports),
			     max_open_dma_treshold), min_tx_1_2g5_treshold);
		rx_2g5_dmas =
			MAX(MIN
			    (CEIL_DIV
			     ((remaing_dmas * rx_tx_raport),
			      weighted_remaining_ports),
			     max_open_dma_treshold), min_rx_1_2g5_treshold);

	}

	/* Check if these settings is not exceding treshold */
	open_dmas_computed = num_1g_ports * tx_1g_dmas +
		num_1g_ports * rx_1g_dmas +
		num_2g5_ports * tx_2g5_dmas +
		num_2g5_ports * rx_2g5_dmas +
		num_10g_ports * tx_10g_dmas +
		num_10g_ports * rx_10g_dmas +
		num_oh_ports * oh_dmas + shared_ext_open_dma;

	/* overflow-leftover calculus */
	overflow = open_dmas_computed - max_fm_open_dma;
	re_loop = true;
	while (overflow > 0 && re_loop == true) {
		re_loop = false;
		if (num_1g_ports && overflow
		    && rx_1g_dmas > min_rx_1_2g5_treshold) {
			rx_1g_dmas--;
			overflow -= num_1g_ports;
			re_loop = true;
		}
		if (num_2g5_ports && overflow
		    && rx_2g5_dmas > min_rx_1_2g5_treshold) {
			rx_2g5_dmas--;
			overflow -= num_2g5_ports;
			re_loop = true;
		}
		if (num_10g_ports && overflow
		    && rx_10g_dmas > min_rx_10g_treshold) {
			rx_10g_dmas--;
			overflow -= num_10g_ports;
			re_loop = true;
		}

		if (num_1g_ports && overflow
		    && tx_1g_dmas > min_tx_1_2g5_treshold) {
			tx_1g_dmas--;
			overflow -= num_1g_ports;
			re_loop = true;
		}
		if (num_2g5_ports && overflow
		    && tx_2g5_dmas > min_tx_1_2g5_treshold) {
			tx_2g5_dmas--;
			overflow -= num_2g5_ports;
			re_loop = true;
		}
		if (num_10g_ports && overflow
		    && tx_10g_dmas > min_tx_10g_treshold) {
			tx_10g_dmas--;
			overflow -= num_10g_ports;
			re_loop = true;
		}
	}

	if (overflow > 0) {
		printk(KERN_ALERT
			"This configuration will not work due to over open dma"
			" allocation (%d open dmas)...\n",
		       overflow);
		err = -1;
		goto precalculated_open_dma_out;
	}

	/* could remain leftovers... e.g. overflow=1,
		2ports => leftover=1 => shared=1 */
	open_dmas_computed = num_1g_ports * tx_1g_dmas +
		num_1g_ports * rx_1g_dmas +
		num_2g5_ports * tx_2g5_dmas +
		num_2g5_ports * rx_2g5_dmas +
		num_10g_ports * tx_10g_dmas +
		num_10g_ports * rx_10g_dmas +
		num_oh_ports * oh_dmas + shared_ext_open_dma;

	if (max_fm_open_dma - open_dmas_computed > 0)
		shared_ext_open_dma =
			MIN(shared_ext_open_dma + max_fm_open_dma -
			    open_dmas_computed, max_ext_open_dma_treshold);

	/* set open dmas */
	fm_resource_settings_info->tx_1g_dmas = tx_1g_dmas;
	fm_resource_settings_info->rx_1g_dmas = rx_1g_dmas;
	fm_resource_settings_info->tx_2g5_dmas = tx_2g5_dmas;
	fm_resource_settings_info->rx_2g5_dmas = rx_2g5_dmas;
	fm_resource_settings_info->tx_10g_dmas = tx_10g_dmas;
	fm_resource_settings_info->rx_10g_dmas = rx_10g_dmas;
	fm_resource_settings_info->oh_dmas = oh_dmas;
	fm_resource_settings_info->shared_ext_open_dma = shared_ext_open_dma;

precalculated_open_dma_out:
	printk(KERN_INFO " FMAN(%u) open dma settings:\n",
	       p_LnxWrpFmDev->id);
	printk(KERN_INFO "  - Total open dma available(%u)\n",
	       max_fm_open_dma);
	if (num_1g_ports) {
		printk(KERN_INFO "  - 1G ports TX %u(%u)\n", num_1g_ports,
		       tx_1g_dmas);
		printk(KERN_INFO "  - 1G ports RX %u(%u)\n", num_1g_ports,
		       rx_1g_dmas);
	}
	if (num_2g5_ports) {
		printk(KERN_INFO "  - 2.5G ports TX %u(%u)\n", num_2g5_ports,
		       tx_2g5_dmas);
		printk(KERN_INFO "  - 2.5G ports RX %u(%u)\n", num_2g5_ports,
		       tx_2g5_dmas);
	}
	if (num_10g_ports) {
		printk(KERN_INFO "  - 10G ports TX %u(%u)\n", num_10g_ports,
		       tx_10g_dmas);
		printk(KERN_INFO "  - 10G ports RX %u(%u)\n", num_10g_ports,
		       rx_10g_dmas);
	}
	if (num_oh_ports)
		printk(KERN_INFO "  - OH-HC ports %u(%u)\n", num_oh_ports,
		       oh_dmas);
	printk(KERN_INFO "  - Shared extra open dma(%u)\n",
	       shared_ext_open_dma ? shared_ext_open_dma : 0);

	return err;
}

int fm_set_precalculate_open_dma(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	t_FmPortRsrc numOfOpenDmas;
	t_Error errCode;

	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;

	memset(&numOfOpenDmas, 0, sizeof(t_FmPortRsrc));

/* IF 1G PORT */
	if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX)
		numOfOpenDmas.num = fm_resource_settings_info->tx_1g_dmas;
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX)
		numOfOpenDmas.num = fm_resource_settings_info->rx_1g_dmas;
/* IF 2.5G PORT*/
	/* TODO: Not supported by LLD yet. */

/* IF 10G PORT */
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_TX_10G)
		numOfOpenDmas.num = fm_resource_settings_info->tx_10g_dmas;
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX_10G)
		numOfOpenDmas.num = fm_resource_settings_info->rx_10g_dmas;
/* IF OH PORT */
	else
		numOfOpenDmas.num = fm_resource_settings_info->oh_dmas;

	numOfOpenDmas.extra = fm_resource_settings_info->shared_ext_open_dma;

	errCode = FM_PORT_SetNumOfOpenDmas(p_LnxWrpFmPortDev->h_Dev,
					      &numOfOpenDmas);
	if (errCode != E_OK) {
		printk(KERN_WARNING
		       "FM_PORT_SetNumOfOpenDmas failed (errCode:0x%2x)",
		       errCode);
		return -EIO;
	}

	return 0;
}

/* Compute FMan tnums based on available tnums and number of ports.
   Set defaults (minim tresholds) and then distribute leftovers.*/
int fm_precalculate_tnums(t_LnxWrpFmDev *p_LnxWrpFmDev, int max_fm_tnums)
{
	/* input parameters */
	struct fm_active_ports *fm_active_ports_info = NULL;
	int num_1g_ports = 0;
	int num_2g5_ports = 0;
	int num_10g_ports = 0;
	int num_oh_ports = 0;

	/* output parameters */
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	int tx_1g_tnums = 0, rx_1g_tnums = 0;
	int tx_2g5_tnums = 0, rx_2g5_tnums = 0;
	int tx_10g_tnums = 0, rx_10g_tnums = 0;
	int oh_tnums = 0;
	int shared_ext_tnums = 0;
	int err = 0;

	/* open dma calculus */
	int default_and_treshold_rx_tx_10g_tnums = 16;	/* DPDE_10g */
	int default_and_treshold_rx_tx_1g_2g5_tnums = 4;	/* DPDE_1g */
	int default_and_treshold_oh_tnums = 2;	/* Hell knows why */
	int max_tnums_treshold = 64;	/* LLD: MAX_NUM_OF_TASKS */
	int max_ext_tnums_treshold = 8;	/* LLD: MAX_NUM_OF_EXTRA_TASKS */
	int remaing_tnums = 0;
	int tnums_computed = 0;
	int leftovers = 0;
	bool re_loop = true;

	/* check input parameters correctness */
	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_active_ports_info = &p_LnxWrpFmDev->fm_active_ports_info;
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;
	ASSERT_COND(fm_active_ports_info != NULL);
	ASSERT_COND(fm_resource_settings_info != NULL);
	ASSERT_COND(fm_active_ports_info->num_tx_ports ==
		    fm_active_ports_info->num_rx_ports);
	ASSERT_COND(fm_active_ports_info->num_tx25_ports ==
		    fm_active_ports_info->num_tx25_ports);
	ASSERT_COND(fm_active_ports_info->num_tx10_ports ==
		    fm_active_ports_info->num_tx10_ports);

	/* set input parameters */
	num_1g_ports = fm_active_ports_info->num_tx_ports;
	num_2g5_ports = fm_active_ports_info->num_tx25_ports;
	num_10g_ports = fm_active_ports_info->num_tx10_ports;
	num_oh_ports = fm_active_ports_info->num_oh_ports;

	/* compute FMan TNUMs per port */
	/* ------------------------------------------------------- */
	if (num_1g_ports) {
		tx_1g_tnums = default_and_treshold_rx_tx_1g_2g5_tnums;
		rx_1g_tnums = default_and_treshold_rx_tx_1g_2g5_tnums;
	}
	if (num_2g5_ports) {
		tx_2g5_tnums = default_and_treshold_rx_tx_1g_2g5_tnums;
		rx_2g5_tnums = default_and_treshold_rx_tx_1g_2g5_tnums;
	}
	if (num_10g_ports) {
		tx_10g_tnums = default_and_treshold_rx_tx_10g_tnums;
		rx_10g_tnums = default_and_treshold_rx_tx_10g_tnums;
	}
	if (num_oh_ports)
		oh_tnums = default_and_treshold_oh_tnums;

	shared_ext_tnums = num_10g_ports ?
		max_ext_tnums_treshold : 2; /* DEFAULT_PORT_extraNumOfTasks */

	/* based on total number of ports set open DMAs for all other ports */
	remaing_tnums = max_fm_tnums -
		(oh_tnums * num_oh_ports) -
		(tx_1g_tnums * num_1g_ports + rx_1g_tnums * num_1g_ports) -
		(tx_2g5_tnums * num_2g5_ports + rx_2g5_tnums * num_2g5_ports) -
		(tx_10g_tnums * num_10g_ports + rx_10g_tnums * num_10g_ports) -
		shared_ext_tnums;

	if (remaing_tnums < 0) {
		printk(KERN_ALERT
			"This configuration will not work due to low number"
			" of tnums (%u tnums) and number of total ports"
			" available...\n",
		       max_fm_tnums);
		err = -1;
		goto precalculated_tnums_out;
	}

	leftovers = remaing_tnums;
	re_loop = true;
	while (leftovers > 0 && re_loop == true) {
		re_loop = false;
		if (num_10g_ports && (leftovers - (int) num_10g_ports) >= 0
		    && (rx_10g_tnums < max_tnums_treshold)) {
			rx_10g_tnums++;
			leftovers -= num_10g_ports;
			re_loop = true;
		}

		if (num_10g_ports && (leftovers - (int) num_10g_ports) >= 0
		    && (tx_10g_tnums < max_tnums_treshold)) {
			tx_10g_tnums++;
			leftovers -= num_10g_ports;
			re_loop = true;
		}

		if (num_2g5_ports && (leftovers - (int) num_2g5_ports) >= 0
		    && (rx_2g5_tnums < max_tnums_treshold)) {
			rx_2g5_tnums++;
			leftovers -= num_2g5_ports;
			re_loop = true;
		}

		if (num_2g5_ports && (leftovers - (int) num_2g5_ports) >= 0
		    && (tx_2g5_tnums < max_tnums_treshold)) {
			tx_2g5_tnums++;
			leftovers -= num_2g5_ports;
			re_loop = true;
		}

		if (num_1g_ports && (leftovers - (int) num_1g_ports) >= 0
		    && (rx_1g_tnums < max_tnums_treshold)) {
			rx_1g_tnums++;
			leftovers -= num_1g_ports;
			re_loop = true;
		}

		if (num_1g_ports && (leftovers - (int) num_1g_ports) >= 0
		    && (tx_1g_tnums < max_tnums_treshold)) {
			tx_1g_tnums++;
			leftovers -= num_1g_ports;
			re_loop = true;
		}
	}

	tnums_computed = 
		num_1g_ports * tx_1g_tnums +
		num_1g_ports * rx_1g_tnums +
		num_2g5_ports * tx_2g5_tnums +
		num_2g5_ports * rx_2g5_tnums +
		num_10g_ports * tx_10g_tnums +
		num_10g_ports * rx_10g_tnums +
		num_oh_ports * oh_tnums + 
		shared_ext_tnums;

	if (leftovers > 0)
		shared_ext_tnums =
			MIN(shared_ext_tnums + max_fm_tnums - tnums_computed,
			    max_ext_tnums_treshold);

	ASSERT_COND((oh_tnums * num_oh_ports) +
		    (tx_1g_tnums * num_1g_ports + rx_1g_tnums * num_1g_ports) +
		    (tx_2g5_tnums * num_2g5_ports +
		     rx_2g5_tnums * num_2g5_ports) +
		    (tx_10g_tnums * num_10g_ports +
		     rx_10g_tnums * num_10g_ports) + shared_ext_tnums <=
		    max_fm_tnums);

	/* set computed tnums */
	fm_resource_settings_info->tx_1g_tnums = tx_1g_tnums;
	fm_resource_settings_info->rx_1g_tnums = rx_1g_tnums;
	fm_resource_settings_info->tx_2g5_tnums = tx_2g5_tnums;
	fm_resource_settings_info->rx_2g5_tnums = rx_2g5_tnums;
	fm_resource_settings_info->tx_10g_tnums = tx_10g_tnums;
	fm_resource_settings_info->rx_10g_tnums = rx_10g_tnums;
	fm_resource_settings_info->oh_tnums = oh_tnums;
	fm_resource_settings_info->shared_ext_tnums = shared_ext_tnums;

precalculated_tnums_out:
	printk(KERN_INFO " FMAN(%u) Tnums settings:\n", p_LnxWrpFmDev->id);
	printk(KERN_INFO "  - Total Tnums available(%u)\n", max_fm_tnums);
	if (num_1g_ports) {
		printk(KERN_INFO "  - 1G ports TX %u(%u)\n", num_1g_ports,
		       tx_1g_tnums);
		printk(KERN_INFO "  - 1G ports RX %u(%u)\n", num_1g_ports,
		       rx_1g_tnums);
	}
	if (num_2g5_ports) {
		printk(KERN_INFO "  - 2.5G ports TX %u(%u)\n", num_2g5_ports,
		       tx_2g5_tnums);
		printk(KERN_INFO "  - 2.5G ports RX %u(%u)\n", num_2g5_ports,
		       rx_2g5_tnums);
	}
	if (num_10g_ports) {
		printk(KERN_INFO "  - 10G ports TX %u(%u)\n", num_10g_ports,
		       tx_10g_tnums);
		printk(KERN_INFO "  - 10G ports RX %u(%u)\n", num_10g_ports,
		       rx_10g_tnums);
	}
	if (num_oh_ports)
		printk(KERN_INFO "  - OH-HC ports %u(%u)\n", num_oh_ports,
		       oh_tnums);
	printk(KERN_INFO "  - Shared extra tnums(%u)\n", shared_ext_tnums);

	return err;
}

int fm_set_precalculate_tnums(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	struct fm_resource_settings *fm_resource_settings_info = NULL;
	t_FmPortRsrc numOfTask;
	t_Error errCode;

	ASSERT_COND(p_LnxWrpFmDev != NULL);
	fm_resource_settings_info = &p_LnxWrpFmDev->fm_resource_settings_info;

	memset(&numOfTask, 0, sizeof(t_FmPortRsrc));

/* IF 1G PORT */
	if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_TX)
		numOfTask.num = fm_resource_settings_info->tx_1g_tnums;
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX)
		numOfTask.num = fm_resource_settings_info->rx_1g_tnums;
/* IF 2.5G PORT*/
	/* TODO: Not supported by LLD yet. */

/* IF 10G PORT */
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_TX_10G)
		numOfTask.num = fm_resource_settings_info->tx_10g_tnums;
	else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX_10G)
		numOfTask.num = fm_resource_settings_info->rx_10g_tnums;
/* IF OH PORT */
	else
		numOfTask.num = fm_resource_settings_info->oh_dmas;

	numOfTask.extra = fm_resource_settings_info->shared_ext_tnums;

	errCode = FM_PORT_SetNumOfTasks(p_LnxWrpFmPortDev->h_Dev, &numOfTask);
	if (errCode != E_OK) {
		printk(KERN_WARNING
		       "FM_PORT_SetNumOfTasks failed (errCode:0x%2x)",
		       errCode);
		return -EIO;
	}

	return 0;
}
