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

#include "xgene_enet_main.h"
#include "xgene_enet_csr.h"

static void xgene_gmac_set_mac_addr(struct xgene_enet_priv *priv,
				    unsigned char *dev_addr)
{
	u32 a_hi = *(u32 *)&dev_addr[0];
	u32 a_lo = (u32) *(u16 *)&dev_addr[4];
	xgene_enet_wr(priv, BLOCK_MCX_MAC, STATION_ADDR0_ADDR, a_hi);

	a_lo <<= 16;
	a_lo |= (priv->phy_addr & 0xFFFF);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, STATION_ADDR1_ADDR, a_lo);
}

static int xgene_enet_ecc_init(struct xgene_enet_priv *priv)
{
	u32 data;
	int wait;

	xgene_enet_wr(priv, BLOCK_ETH_DIAG_CSR,
			ENET_CFG_MEM_RAM_SHUTDOWN_ADDR, 0x0);
	/* check for at leaset 1 ms */
	wait = 1000;
	do {
		xgene_enet_rd(priv, BLOCK_ETH_DIAG_CSR,
			ENET_BLOCK_MEM_RDY_ADDR, &data);
		usleep_range(1, 100);
	} while (--wait && data != 0xffffffff);
	if (!wait) {
		pr_err("Failed to release memory from shutdown\n");
		return -ENODEV;
	}

	return 0;
}

static void xgene_gmac_change_mtu(struct xgene_enet_priv *priv, u32 new_mtu)
{
	u32 data;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAX_FRAME_LEN_ADDR, &data);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAX_FRAME_LEN_ADDR,
		      MAX_FRAME_LEN_SET(data, new_mtu));
}

static void xgene_gmac_phy_enable_scan_cycle(struct xgene_enet_priv *priv,
					     int enable)
{
	u32 val;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MII_MGMT_COMMAND_ADDR, &val);
	if (enable)
		val |= SCAN_CYCLE_MASK;
	else
		val &= ~SCAN_CYCLE_MASK;
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MII_MGMT_COMMAND_ADDR, val);

	/* Program phy address start scan from 0 and register at address 0x1 */
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MII_MGMT_ADDRESS_ADDR, &val);
	val = PHY_ADDR_SET(val, 0);
	val = REG_ADDR_SET(val, 1);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MII_MGMT_ADDRESS_ADDR, val);
}

static void xgene_gmac_reset(struct xgene_enet_priv *priv)
{
	u32 value;
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &value);
	if (!(value & SOFT_RESET1_MASK))
		return;

	value = RESET_TX_FUN1_WR(1)
	    | RESET_RX_FUN1_WR(1)
	    | RESET_TX_MC1_WR(1)
	    | RESET_RX_MC1_WR(1)
	    | SIM_RESET1_WR(1)
	    | SOFT_RESET1_WR(1);

	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, value);
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &value);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, 0);
}

int xgene_gmac_init(struct xgene_enet_priv *priv, unsigned char *dev_addr,
		    int speed, int mtu, int crc)
{
	u32 value, temp;
	u32 addr_hi, addr_lo;

	u32 interface_control;
	u32 mac_config_2;
	u32 rgmii;
	u32 icm_config0 = 0x0008503f;
	u32 icm_config2 = 0x0010000f;
	u32 ecm_config0 = 0x00000032;
	u32 enet_spare_cfg = 0x00006040;

	/* Reset subsystem */
	value = RESET_TX_FUN1_WR(1)
	    | RESET_RX_FUN1_WR(1)
	    | RESET_TX_MC1_WR(1)
	    | RESET_RX_MC1_WR(1)
	    | SIM_RESET1_WR(1)
	    | SOFT_RESET1_WR(1);

	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, value);
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &temp);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, 0);
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &temp);

	value = TX_EN1_WR(1)
	    | RX_EN1_WR(1)
	    | TX_FLOW_EN1_WR(0)
	    | LOOP_BACK1_WR(0)
	    | RX_FLOW_EN1_WR(0);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, value);
	xgene_enet_rd(priv, BLOCK_ETH_CSR,
		      ENET_SPARE_CFG_REG_ADDR, &enet_spare_cfg);

	if (speed == XGENE_ENET_SPEED_10) {
		interface_control = ENET_LHD_MODE_WR(0)
		    | ENET_GHD_MODE_WR(0);
		mac_config_2 = FULL_DUPLEX2_WR(1)
		    | LENGTH_CHECK2_WR(0)
		    | HUGE_FRAME_EN2_WR(0)
		    | ENET_INTERFACE_MODE2_WR(1)	/* 10Mbps */
		    |PAD_CRC2_WR(crc)
		    | CRC_EN2_WR(crc)
		    | PREAMBLE_LENGTH2_WR(7);
		rgmii = 0;
		icm_config0 = 0x0000503f;
		icm_config2 = 0x000101f4;
		ecm_config0 = 0x600032;
		enet_spare_cfg = enet_spare_cfg | (0x0000c040);
	} else if (speed == XGENE_ENET_SPEED_100) {
		interface_control = ENET_LHD_MODE_WR(1);
		mac_config_2 = FULL_DUPLEX2_WR(1)
		    | LENGTH_CHECK2_WR(0)
		    | HUGE_FRAME_EN2_WR(0)
		    | ENET_INTERFACE_MODE2_WR(1)	/* 100Mbps */
		    |PAD_CRC2_WR(crc)
		    | CRC_EN2_WR(crc)
		    | PREAMBLE_LENGTH2_WR(7);
		rgmii = 0;
		icm_config0 = 0x0004503f;
		icm_config2 = 0x00010050;
		ecm_config0 = 0x600032;
		enet_spare_cfg = enet_spare_cfg | (0x0000c040);
	} else {
		interface_control = ENET_GHD_MODE_WR(1);
		mac_config_2 = FULL_DUPLEX2_WR(1)
		    | LENGTH_CHECK2_WR(0)
		    | HUGE_FRAME_EN2_WR(0)
		    | ENET_INTERFACE_MODE2_WR(2)	/* 1Gbps */
		    |PAD_CRC2_WR(crc)
		    | CRC_EN2_WR(crc)
		    | PREAMBLE_LENGTH2_WR(7);
		rgmii = CFG_SPEED_1250_WR(1) | CFG_TXCLK_MUXSEL0_WR(4);
		icm_config0 = 0x0008503f;
		icm_config2 = 0x0001000f;
		ecm_config0 = 0x32;
		enet_spare_cfg = (enet_spare_cfg & ~0x0000c000)
		    | (0x00000040);
	}

	enet_spare_cfg |= 0x00006040;

	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_2_ADDR, mac_config_2);

	xgene_enet_wr(priv, BLOCK_MCX_MAC, INTERFACE_CONTROL_ADDR,
		      interface_control);

	value = MAX_FRAME_LEN_WR(0x0600);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAX_FRAME_LEN_ADDR, value);

	/* Program the station MAC address */
	addr_hi = *(u32 *) &dev_addr[0];
	addr_lo = *(u16 *) &dev_addr[4];
	addr_lo <<= 16;
	addr_lo |= (priv->phy_addr & 0xFFFF);

	xgene_enet_wr(priv, BLOCK_MCX_MAC, STATION_ADDR0_ADDR, addr_hi);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, STATION_ADDR1_ADDR, addr_lo);

	/* Adjust MDC clock frequency */
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MII_MGMT_CONFIG_ADDR, &value);
	value = MGMT_CLOCK_SEL_SET(value, 7);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MII_MGMT_CONFIG_ADDR, value);

	/* Enable drop if FP not available */
	xgene_enet_rd(priv, BLOCK_ETH_CSR, RSIF_CONFIG_REG_ADDR, &value);
	value |= CFG_RSIF_FPBUFF_TIMEOUT_EN_WR(1);
	xgene_enet_wr(priv, BLOCK_ETH_CSR, RSIF_CONFIG_REG_ADDR, value);

	/* Rtype should be copied from FP */
	value = 0;
	xgene_enet_wr(priv, BLOCK_ETH_CSR, RSIF_RAM_DBG_REG0_ADDR, value);
	/* Initialize RGMII PHY */
	if (priv->phy_mode == PHY_MODE_RGMII)
		xgene_enet_wr(priv, BLOCK_ETH_CSR, RGMII_REG_0_ADDR, rgmii);

	xgene_enet_wr(priv, BLOCK_MCX_MAC_CSR, ICM_CONFIG0_REG_0_ADDR,
		      icm_config0);
	xgene_enet_wr(priv, BLOCK_MCX_MAC_CSR, ICM_CONFIG2_REG_0_ADDR,
		      icm_config2);
	xgene_enet_wr(priv, BLOCK_MCX_MAC_CSR, ECM_CONFIG0_REG_0_ADDR,
		      ecm_config0);
	xgene_enet_wr(priv, BLOCK_ETH_CSR, ENET_SPARE_CFG_REG_ADDR,
		      enet_spare_cfg);

	/* Rx-Tx traffic resume */
	xgene_enet_wr(priv, BLOCK_ETH_CSR,
		      CFG_LINK_AGGR_RESUME_0_ADDR, TX_PORT0_WR(0x1));

	if (speed != XGENE_ENET_SPEED_10 && speed != XGENE_ENET_SPEED_100) {
		xgene_enet_rd(priv, BLOCK_ETH_CSR, DEBUG_REG_ADDR, &value);
		value |= CFG_BYPASS_UNISEC_TX_WR(1)
		    | CFG_BYPASS_UNISEC_RX_WR(1);
		xgene_enet_wr(priv, BLOCK_ETH_CSR, DEBUG_REG_ADDR, value);
	}

	xgene_enet_rd(priv, BLOCK_MCX_MAC_CSR, RX_DV_GATE_REG_0_ADDR, &value);
	value = TX_DV_GATE_EN0_SET(value, 0);
	value = RX_DV_GATE_EN0_SET(value, 0);
	value = RESUME_RX0_SET(value, 1);
	xgene_enet_wr(priv, BLOCK_MCX_MAC_CSR, RX_DV_GATE_REG_0_ADDR, value);

	xgene_enet_wr(priv, BLOCK_ETH_CSR, CFG_BYPASS_ADDR, RESUME_TX_WR(1));
	return 0;
}

/* Start Statistics related functions */
static void xgene_gmac_get_rx_stats(struct xgene_enet_priv *priv,
				    struct xgene_enet_rx_stats *rx_stat)
{
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RBYT_ADDR,
		      &rx_stat->rx_byte_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RPKT_ADDR,
		      &rx_stat->rx_packet_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RDRP_ADDR,
		      &rx_stat->rx_drop_pkt_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RFCS_ADDR,
		      &rx_stat->rx_fcs_err_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RFLR_ADDR,
		      &rx_stat->rx_frm_len_err_pkt_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RALN_ADDR,
		      &rx_stat->rx_alignment_err_pkt_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, ROVR_ADDR,
		      &rx_stat->rx_oversize_pkt_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, RUND_ADDR,
		      &rx_stat->rx_undersize_pkt_count);

	rx_stat->rx_byte_count &= RX_BYTE_CNTR_MASK;
	rx_stat->rx_packet_count &= RX_PKT_CNTR_MASK;
	rx_stat->rx_drop_pkt_count &= RX_DROPPED_PKT_CNTR_MASK;
	rx_stat->rx_fcs_err_count &= RX_FCS_ERROR_CNTR_MASK;
	rx_stat->rx_frm_len_err_pkt_count &= RX_LEN_ERR_CNTR_MASK;
	rx_stat->rx_alignment_err_pkt_count &= RX_ALIGN_ERR_CNTR_MASK;
	rx_stat->rx_oversize_pkt_count &= RX_OVRSIZE_PKT_CNTR_MASK;
	rx_stat->rx_undersize_pkt_count &= RX_UNDRSIZE_PKT_CNTR_MASK;
}

static void xgene_gmac_get_tx_stats(struct xgene_enet_priv *priv,
				    struct xgene_enet_tx_stats *tx_stats)
{
	xgene_enet_rd(priv, BLOCK_MCX_STATS, TBYT_ADDR,
		      &tx_stats->tx_byte_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, TPKT_ADDR,
		      &tx_stats->tx_pkt_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, TDRP_ADDR,
		      &tx_stats->tx_drop_frm_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, TFCS_ADDR,
		      &tx_stats->tx_fcs_err_frm_count);
	xgene_enet_rd(priv, BLOCK_MCX_STATS, TUND_ADDR,
		      &tx_stats->tx_undersize_frm_count);

	tx_stats->tx_byte_count &= TX_BYTE_CNTR_MASK;
	tx_stats->tx_pkt_count &= TX_PKT_CNTR_MASK;
	tx_stats->tx_drop_frm_count &= TX_DROP_FRAME_CNTR_MASK;
	tx_stats->tx_fcs_err_frm_count &= TX_FCS_ERROR_CNTR_MASK;
	tx_stats->tx_undersize_frm_count &= TX_UNDSIZE_FRAME_CNTR_MASK;
}

static void xgene_gmac_get_detailed_stats(struct xgene_enet_priv *priv,
		struct xgene_enet_detailed_stats *stats)
{
	xgene_gmac_get_rx_stats(priv, &(stats->rx_stats));
	xgene_gmac_get_tx_stats(priv, &(stats->tx_stats));
}

/* Configure Ethernet QMI: WQ and FPQ association to QML */
static void xgene_enet_config_qmi_assoc(struct xgene_enet_priv *priv)
{
	xgene_enet_wr(priv, BLOCK_ETH_QMI, ENET_CFGSSQMIWQASSOC_ADDR,
		      0xffffffff);
	xgene_enet_wr(priv, BLOCK_ETH_QMI, ENET_CFGSSQMIFPQASSOC_ADDR,
		      0xffffffff);
	xgene_enet_wr(priv, BLOCK_ETH_QMI, ENET_CFGSSQMIQMLITEFPQASSOC_ADDR,
		      0xffffffff);
	xgene_enet_wr(priv, BLOCK_ETH_QMI, ENET_CFGSSQMIQMLITEWQASSOC_ADDR,
		      0xffffffff);
}

static void xgene_enet_cle_bypass_mode_cfg(struct xgene_enet_priv *priv,
					   u32 cle_dstqid, u32 cle_fpsel)
{
	u32 reg;

	xgene_enet_rd(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG0_0_ADDR, &reg);
	reg = CFG_CLE_BYPASS_EN0_SET(reg, 1);
	reg = CFG_CLE_IP_PROTOCOL0_SET(reg, 3);
	xgene_enet_wr(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG0_0_ADDR, reg);

	xgene_enet_rd(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG1_0_ADDR, &reg);
	reg = CFG_CLE_DSTQID0_SET(reg, cle_dstqid);
	reg = CFG_CLE_FPSEL0_SET(reg, cle_fpsel);
	xgene_enet_wr(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG1_0_ADDR, reg);

	xgene_enet_rd(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG8_0_ADDR, &reg);
	reg = CFG_CLE_HENQNUM0_SET(reg, cle_dstqid);
	xgene_enet_wr(priv, BLOCK_ETH_CSR, CLE_BYPASS_REG8_0_ADDR, reg);
}

static void xgene_gmac_rx_state(struct xgene_enet_priv *priv, u32 enable)
{
	u32 data, rx_en;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &data);
	rx_en = (enable) ? RX_EN1_SET(data, 1) : RX_EN1_SET(data, 0);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, rx_en);
}

static void xgene_gmac_tx_state(struct xgene_enet_priv *priv, u32 enable)
{
	u32 data, tx_en;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &data);
	tx_en = (enable) ? TX_EN1_SET(data, 1) : TX_EN1_SET(data, 0);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, tx_en);
}

static void xgene_gmac_tx_offload(struct xgene_enet_priv *priv,
				  u32 command, u32 value)
{
	u32 data;

		switch (command) {
			/* TCP MSS 0 */
		case XGENE_ENET_MSS0:
			xgene_enet_rd(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG0_0_ADDR, &data);
			xgene_enet_wr(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG0_0_ADDR,
				      CFG_TSIF_MSS_SZ00_SET(data, value));
			break;
			/* TCP MSS 1 */
		case XGENE_ENET_MSS1:
			xgene_enet_rd(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG0_0_ADDR, &data);
			xgene_enet_wr(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG0_0_ADDR,
				      CFG_TSIF_MSS_SZ10_SET(data, value));
			break;
			/* TCP MSS 2 */
		case XGENE_ENET_MSS2:
			xgene_enet_rd(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG1_0_ADDR, &data);
			xgene_enet_wr(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG1_0_ADDR,
				      CFG_TSIF_MSS_SZ20_SET(data, value));
			break;
			/* TCP MSS 3 */
		case XGENE_ENET_MSS3:
			xgene_enet_rd(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG1_0_ADDR, &data);
			xgene_enet_wr(priv, BLOCK_ETH_CSR,
				      TSIF_MSS_REG1_0_ADDR,
				      CFG_TSIF_MSS_SZ30_SET(data, value));
			break;
			/* Program TSO config */
		case XGENE_ENET_TSO_CFG:
			xgene_enet_wr(priv, BLOCK_ETH_CSR, TSO_CFG_0_ADDR,
				      value);
			break;
			/* Insert Inser tVLAN TAG */
		case XGENE_ENET_INSERT_VLAN:
			xgene_enet_wr(priv, BLOCK_ETH_CSR,
				      TSO_CFG_INSERT_VLAN_0_ADDR, value);
			break;
		}
}

static void xgene_enet_clkrst_cfg(struct xgene_enet_priv *priv)
{
	u32 data;

	/* disable all clocks */
	data = CSR0_CLKEN_WR(0) | ENET0_CLKEN_WR(0) |
	    CSR1_CLKEN_WR(0) | ENET1_CLKEN_WR(0);
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_CLKEN_ADDR, data);

	/* enable all clocks */
	data = CSR0_CLKEN_WR(1) | ENET0_CLKEN_WR(1) |
	    CSR1_CLKEN_WR(1) | ENET1_CLKEN_WR(1);
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_CLKEN_ADDR, data);

	/* put csr and core reset */
	data = CSR0_RESET_WR(1) | ENET0_RESET_WR(1) |
	    CSR1_RESET_WR(1) | ENET1_RESET_WR(1);
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_SRST_ADDR, data);

	/* release csr and core reset */
	data = CSR0_RESET_WR(0) | ENET0_RESET_WR(0) |
	    CSR1_RESET_WR(0) | ENET1_RESET_WR(0);
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_SRST_ADDR, data);

	xgene_enet_ecc_init(priv);
}

static void xgene_gport_reset(struct xgene_enet_priv *priv)
{
	u32 val;

	xgene_enet_clkrst_cfg(priv);
	xgene_enet_config_qmi_assoc(priv);

	/* Enable auto-incr for scanning */
	xgene_enet_rd(priv, BLOCK_MCX_MAC, MII_MGMT_CONFIG_ADDR, &val);
	val |= SCAN_AUTO_INCR_MASK;
	val = MGMT_CLOCK_SEL_SET(val, 1);
	xgene_enet_wr(priv, BLOCK_MCX_MAC, MII_MGMT_CONFIG_ADDR, val);
	xgene_gmac_phy_enable_scan_cycle(priv, 1);
}

static void xgene_gport_shutdown(struct xgene_enet_priv *priv)
{
	u32 clk, rst;

	rst = CSR0_RESET_WR(1) | ENET0_RESET_WR(1);
	clk = CSR0_CLKEN_WR(0) | ENET0_CLKEN_WR(0);

	/* reset ethernet csr, core and disable clock */
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_SRST_ADDR, rst);
	xgene_enet_wr(priv, BLOCK_ETH_CLKRST_CSR, ENET_CLKEN_ADDR, clk);
}

void xgene_enet_init_priv(struct xgene_enet_priv *priv)
{
	void *gbl_vaddr = priv->vaddr_base;
	void *port_vaddr = priv->vpaddr_base;

	/* Initialize base addresses for direct access */
	priv->eth_csr_addr_v = gbl_vaddr + BLOCK_ETH_CSR_OFFSET;
	priv->eth_cle_addr_v = gbl_vaddr + BLOCK_ETH_CLE_OFFSET;
	priv->eth_qmi_addr_v = gbl_vaddr + BLOCK_ETH_QMI_OFFSET;
	priv->eth_sds_csr_addr_v = gbl_vaddr + BLOCK_ETH_SDS_CSR_OFFSET;
	priv->eth_clkrst_csr_addr_v = gbl_vaddr + BLOCK_ETH_CLKRST_CSR_OFFSET;
	priv->eth_diag_csr_addr_v = gbl_vaddr + BLOCK_ETH_DIAG_CSR_OFFSET;

	/* Initialize per port base addr for indirect & direct MCX MAC access */
	priv->mcx_mac_addr_v = port_vaddr + BLOCK_ETH_MAC_OFFSET;
	priv->mcx_stats_addr_v = port_vaddr + BLOCK_ETH_STATS_OFFSET;
	priv->mcx_mac_csr_addr_v = gbl_vaddr + BLOCK_ETH_MAC_CSR_OFFSET;
	priv->sata_enet_csr_addr_v = gbl_vaddr + BLOCK_SATA_ENET_CSR_OFFSET;

	/* Enable autonegotiation by default */
	priv->autoneg_set = 1;

	pr_debug("      ETH PORT VADDR: 0x%p\n", priv->vpaddr_base);
	pr_debug("           ETH VADDR: 0x%p\n", priv->vaddr_base);
	pr_debug("       ETH CSR VADDR: 0x%p\n", priv->eth_csr_addr_v);
	pr_debug("       ETH CLE VADDR: 0x%p\n", priv->eth_cle_addr_v);
	pr_debug("       ETH QMI VADDR: 0x%p\n", priv->eth_qmi_addr_v);
	pr_debug("   ETH SDS CSR VADDR: 0x%p\n", priv->eth_sds_csr_addr_v);
	pr_debug("ETH CLKRST CSR VADDR: 0x%p\n", priv->eth_clkrst_csr_addr_v);
	pr_debug("      ETH DIAG VADDR: 0x%p\n", priv->eth_diag_csr_addr_v);
	pr_debug("       MAC MII VADDR: 0x%p\n", priv->vmii_base);
	pr_debug("       MCX MAC VADDR: 0x%p\n", priv->mcx_mac_addr_v);
	pr_debug("      MCX STAT VADDR: 0x%p\n", priv->mcx_stats_addr_v);
	pr_debug("   MCX MAC CSR VADDR: 0x%p\n", priv->mcx_mac_csr_addr_v);
	pr_debug(" SATA ENET CSR VADDR: 0x%p\n", priv->sata_enet_csr_addr_v);

	/* Initialize priv handlers */
	priv->port_reset = xgene_gport_reset;
	priv->mac_reset = xgene_gmac_reset;
	priv->mac_init = xgene_gmac_init;
	priv->mac_rx_state = xgene_gmac_rx_state;
	priv->mac_tx_state = xgene_gmac_tx_state;
	priv->mac_change_mtu = xgene_gmac_change_mtu;
	priv->set_mac_addr = xgene_gmac_set_mac_addr;
	priv->cle_bypass = xgene_enet_cle_bypass_mode_cfg;
	priv->tx_offload = xgene_gmac_tx_offload;
	priv->port_shutdown = xgene_gport_shutdown;
	priv->get_stats = xgene_gmac_get_detailed_stats;
}
