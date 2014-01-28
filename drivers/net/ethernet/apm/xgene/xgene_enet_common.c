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
#include "xgene_enet_common.h"
#include "xgene_enet_csr.h"

/* Indirect Address - read/write commands */
#define PHY_ADDR_WR(src)		(((u32)(src) < 8) & 0x00001f00)
#define REG_ADDR_WR(src)		(((u32)(src)) & 0x0000001f)

int xgene_enet_wr(struct xgene_enet_priv *priv, u8 block_id,
		  u32 reg_offset, u32 value)
{
	u32 cmd_done;
	u32 indirect = 0;
	int wait;
	void *addr_reg_offst, *cmd_reg_offst, *wr_reg_offst;
	void *cmd_done_reg_offst;

	switch (block_id) {
	case BLOCK_ETH_CSR:
		addr_reg_offst = priv->eth_csr_addr_v + reg_offset;
		pr_debug("ETH CSR write\n");
		break;

	case BLOCK_ETH_MDIO_CSR:
		addr_reg_offst = priv->vmii_base + reg_offset
		    + BLOCK_ETH_CSR_OFFSET;
		pr_debug("BLOCK_ETH_MDIO_CSR write 0x%p\n", addr_reg_offst);
		break;

	case BLOCK_ETH_CLE:
		addr_reg_offst = priv->eth_cle_addr_v + reg_offset;
		pr_debug("ETH CLE write\n");
		break;

	case BLOCK_ETH_QMI:
		addr_reg_offst = priv->eth_qmi_addr_v + reg_offset;
		pr_debug("ETH QMI write\n");
		break;

	case BLOCK_ETH_SDS_CSR:
		addr_reg_offst = priv->eth_sds_csr_addr_v + reg_offset;
		pr_debug("ETH SDS CSR write\n");
		break;

	case BLOCK_ETH_CLKRST_CSR:
		addr_reg_offst = priv->eth_clkrst_csr_addr_v + reg_offset;
		pr_debug("ETH CLKRST CSR write\n");
		break;

	case BLOCK_ETH_DIAG_CSR:
		addr_reg_offst = priv->eth_diag_csr_addr_v + reg_offset;
		pr_debug("ETH DIAG CSR write\n");
		break;

	case BLOCK_MCX_MAC:
	case BLOCK_ETH_INTPHY:
		addr_reg_offst = priv->mcx_mac_addr_v + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->mcx_mac_addr_v + MAC_COMMAND_REG_OFFSET;
		wr_reg_offst = priv->mcx_mac_addr_v + MAC_WRITE_REG_OFFSET;
		cmd_done_reg_offst = priv->mcx_mac_addr_v
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX MAC/Internal PHY write\n");
		break;

	case BLOCK_ETH_EXTPHY:
		addr_reg_offst = priv->vmii_base + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->vmii_base + MAC_COMMAND_REG_OFFSET;
		wr_reg_offst = priv->vmii_base + MAC_WRITE_REG_OFFSET;
		cmd_done_reg_offst = priv->vmii_base
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX MAC/External PHY write\n");
		break;

	case BLOCK_MCX_STATS:
		addr_reg_offst = priv->mcx_stats_addr_v + STAT_ADDR_REG_OFFSET;
		cmd_reg_offst =
		    priv->mcx_stats_addr_v + STAT_COMMAND_REG_OFFSET;
		wr_reg_offst = priv->mcx_stats_addr_v + STAT_WRITE_REG_OFFSET;
		cmd_done_reg_offst = priv->mcx_stats_addr_v
		    + STAT_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX STATS write\n");
		break;

	case BLOCK_MCX_MAC_CSR:
		addr_reg_offst = priv->mcx_mac_csr_addr_v + reg_offset;
		pr_debug("MCX MAC CSR write\n");
		break;

	case BLOCK_SATA_ENET_CSR:
		addr_reg_offst = priv->sata_enet_csr_addr_v + reg_offset;
		pr_debug("SATA ENET CSR write\n");
		break;

	case BLOCK_AXG_MAC:
		addr_reg_offst = priv->axg_mac_addr_v + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->axg_mac_addr_v + MAC_COMMAND_REG_OFFSET;
		wr_reg_offst = priv->axg_mac_addr_v + MAC_WRITE_REG_OFFSET;
		cmd_done_reg_offst = priv->axg_mac_addr_v
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("AXG MAC write\n");
		break;

	case BLOCK_AXG_STATS:
		addr_reg_offst = priv->axg_stats_addr_v + STAT_ADDR_REG_OFFSET;
		cmd_reg_offst =
		    priv->axg_stats_addr_v + STAT_COMMAND_REG_OFFSET;
		wr_reg_offst = priv->axg_stats_addr_v + STAT_WRITE_REG_OFFSET;
		cmd_done_reg_offst = priv->axg_stats_addr_v
		    + STAT_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("AXG STATS write\n");
		break;

	case BLOCK_AXG_MAC_CSR:
		addr_reg_offst = priv->axg_mac_csr_addr_v + reg_offset;
		pr_debug("AXG MAC CSR write\n");
		break;

	case BLOCK_XGENET_PCS:
		addr_reg_offst = priv->xgenet_pcs_addr_v + reg_offset;
		pr_debug("XGENET PCS write\n");
		break;

	case BLOCK_XGENET_MDIO_CSR:
		addr_reg_offst = priv->xgenet_mdio_csr_addr_v + reg_offset;
		pr_debug("XGENET MDIO CSR write\n");
		break;

	default:
		pr_err("Invalid blockid in write reg: %d\n", block_id);
		return -1;
	}

	if (indirect) {
		xgene_enet_wr32(addr_reg_offst, reg_offset);
		xgene_enet_wr32(wr_reg_offst, value);
		xgene_enet_wr32(cmd_reg_offst, XGENE_ENET_WR_CMD);
		pr_debug("Indirect write: addr: 0x%X, value: 0x%X\n",
			 reg_offset, value);

		/* wait upto 5 us for completion */
		wait = 5;
		do {
			xgene_enet_rd32(cmd_done_reg_offst, &cmd_done);
			usleep_range(1, 2);
		} while (--wait && !cmd_done);
		if (!wait) {
			pr_err("Write failed for blk: %d\n", block_id);
			BUG();
		}

		xgene_enet_wr32(cmd_reg_offst, 0);
	} else {
		xgene_enet_wr32(addr_reg_offst, value);
		pr_debug("Direct write addr: 0x%p, value: 0x%X\n",
			 addr_reg_offst, value);
	}

	return 0;
}

int xgene_enet_rd(struct xgene_enet_priv *priv, u8 block_id,
		  u32 reg_offset, u32 *value)
{
	u32 cmd_done;
	u32 indirect = 0;
	int wait;
	void *addr_reg_offst, *cmd_reg_offst, *rd_reg_offst;
	void *cmd_done_reg_offst;

	switch (block_id) {
	case BLOCK_ETH_CSR:
		addr_reg_offst = priv->eth_csr_addr_v + reg_offset;
		pr_debug("ETH CSR read\n");
		break;

	case BLOCK_ETH_MDIO_CSR:
		addr_reg_offst = priv->vmii_base + reg_offset
		    + BLOCK_ETH_CSR_OFFSET;
		pr_debug("BLOCK_ETH_MDIO_CSR read 0x%p\n", addr_reg_offst);
		break;

	case BLOCK_ETH_CLE:
		addr_reg_offst = priv->eth_cle_addr_v + reg_offset;
		pr_debug("ETH CLE read\n");
		break;

	case BLOCK_ETH_QMI:
		addr_reg_offst = priv->eth_qmi_addr_v + reg_offset;
		pr_debug("ETH QMI read\n");
		break;

	case BLOCK_ETH_SDS_CSR:
		addr_reg_offst = priv->eth_sds_csr_addr_v + reg_offset;
		pr_debug("ETH SDS CSR read\n");
		break;

	case BLOCK_ETH_CLKRST_CSR:
		addr_reg_offst = priv->eth_clkrst_csr_addr_v + reg_offset;
		pr_debug("ETH CLKRST CSR read\n");
		break;

	case BLOCK_ETH_DIAG_CSR:
		addr_reg_offst = priv->eth_diag_csr_addr_v + reg_offset;
		pr_debug("ETH DIAG CSR read\n");
		break;

	case BLOCK_MCX_MAC:
	case BLOCK_ETH_INTPHY:
		addr_reg_offst = priv->mcx_mac_addr_v + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->mcx_mac_addr_v + MAC_COMMAND_REG_OFFSET;
		rd_reg_offst = priv->mcx_mac_addr_v + MAC_READ_REG_OFFSET;
		cmd_done_reg_offst = priv->mcx_mac_addr_v
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX MAC/Internal PHY read\n");
		break;

	case BLOCK_ETH_EXTPHY:
		addr_reg_offst = priv->vmii_base + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->vmii_base + MAC_COMMAND_REG_OFFSET;
		rd_reg_offst = priv->vmii_base + MAC_READ_REG_OFFSET;
		cmd_done_reg_offst = priv->vmii_base
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX MAC/External PHY read\n");
		break;

	case BLOCK_MCX_STATS:
		addr_reg_offst = priv->mcx_stats_addr_v + STAT_ADDR_REG_OFFSET;
		cmd_reg_offst =
		    priv->mcx_stats_addr_v + STAT_COMMAND_REG_OFFSET;
		rd_reg_offst = priv->mcx_stats_addr_v + STAT_READ_REG_OFFSET;
		cmd_done_reg_offst = priv->mcx_stats_addr_v
		    + STAT_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("MCX STATS read\n");
		break;

	case BLOCK_MCX_MAC_CSR:
		addr_reg_offst = priv->mcx_mac_csr_addr_v + reg_offset;
		pr_debug("MCX MAC CSR read\n");
		break;

	case BLOCK_SATA_ENET_CSR:
		addr_reg_offst = priv->sata_enet_csr_addr_v + reg_offset;
		pr_debug("SATA ENET CSR read\n");
		break;

	case BLOCK_AXG_MAC:
		addr_reg_offst = priv->axg_mac_addr_v + MAC_ADDR_REG_OFFSET;
		cmd_reg_offst = priv->axg_mac_addr_v + MAC_COMMAND_REG_OFFSET;
		rd_reg_offst = priv->axg_mac_addr_v + MAC_READ_REG_OFFSET;
		cmd_done_reg_offst = priv->axg_mac_addr_v
		    + MAC_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("AXG MAC read\n");
		break;

	case BLOCK_AXG_STATS:
		addr_reg_offst = priv->axg_stats_addr_v + STAT_ADDR_REG_OFFSET;
		cmd_reg_offst =
		    priv->axg_stats_addr_v + STAT_COMMAND_REG_OFFSET;
		rd_reg_offst = priv->axg_stats_addr_v + STAT_READ_REG_OFFSET;
		cmd_done_reg_offst = priv->axg_stats_addr_v
		    + STAT_COMMAND_DONE_REG_OFFSET;
		indirect = 1;
		pr_debug("AXG STATS read\n");
		break;

	case BLOCK_AXG_MAC_CSR:
		addr_reg_offst = priv->axg_mac_csr_addr_v + reg_offset;
		pr_debug("AXG MAC CSR read\n");
		break;

	case BLOCK_XGENET_PCS:
		addr_reg_offst = priv->xgenet_pcs_addr_v + reg_offset;
		pr_debug("XGENET PCS read\n");
		break;

	case BLOCK_XGENET_MDIO_CSR:
		addr_reg_offst = priv->xgenet_mdio_csr_addr_v + reg_offset;
		pr_debug("XGENET MDIO CSR read\n");
		break;

	default:
		pr_err("Invalid blockid in read reg: %d\n", block_id);
		return -1;
	}

	if (indirect) {
		xgene_enet_wr32(addr_reg_offst, reg_offset);
		xgene_enet_wr32(cmd_reg_offst, XGENE_ENET_RD_CMD);
		pr_debug("Indirect read: addr: 0x%X\n", reg_offset);

		/* wait upto 5 us for completion */
		wait = 5;
		do {
			xgene_enet_rd32(cmd_done_reg_offst, &cmd_done);
		} while (--wait && !cmd_done);
		if (!wait) {
			pr_err("Read failed for blk: %d\n", block_id);
			BUG();
		}

		xgene_enet_rd32(rd_reg_offst, value);
		pr_debug("Indirect read value: 0x%X\n", *value);

		xgene_enet_wr32(cmd_reg_offst, 0);
	} else {
		xgene_enet_rd32(addr_reg_offst, value);
		pr_debug("Direct read addr: 0x%p, value: 0x%X\n",
			 addr_reg_offst, *value);
	}

	return 0;
}

void xgene_genericmiiphy_write(struct xgene_enet_priv *priv, u8 phy_id,
			       unsigned char reg, u32 data)
{
	u32 value;
	int wait;
	u32 blockid = BLOCK_ETH_EXTPHY;

	/* All PHYs lie on MII bus of Port0 MAC due to this
	 * each port should access its PHY through Port0 MAC.
	 * Hence we allow access to PHY_ID associated with this
	 * port only.
	 */

	/* Write PHY number and address in MII Mgmt Address */
	value = PHY_ADDR_WR(phy_id) | REG_ADDR_WR(reg);
	pr_debug("Write MII_MGMT_ADDRESS phy_id=0x%x, reg=0x%x, value=0x%x\n",
		 phy_id, reg << 2, value);
	xgene_enet_wr(priv, blockid, MII_MGMT_ADDRESS_ADDR, value);

	/* Write 16 bit data to MII MGMT CONTROL */
	value = PHY_CONTROL_WR(data);
	pr_debug("Write MII_MGMT_CONTROL phy_id=0x%x, reg=0x%x, value=0x%x\n",
		 phy_id, reg << 2, value);
	xgene_enet_wr(priv, blockid, MII_MGMT_CONTROL_ADDR, value);

	/* wait upto 20 us for completion */
	wait = 20;
	do {
		xgene_enet_rd(priv, blockid, MII_MGMT_INDICATORS_ADDR, &value);
		usleep_range(1, 2);
	} while (--wait && (value & BUSY_MASK));
	if (!wait)
		pr_err("MII_MGMT write failed\n");
}

void xgene_genericmiiphy_read(struct xgene_enet_priv *priv, u8 phy_id,
			      unsigned char reg, u32 *data)
{
	u32 value;
	u32 blockid = BLOCK_ETH_EXTPHY;
	int wait;

	/* All PHYs lie on MII bus of Port0 MAC due to this
	 * each port should access its PHY through Port0 MAC.
	 * Hence we allow access to PHY_ID associated with this
	 * port only.
	 */

	/* Write PHY number and address in MII Mgmt Address */
	value = PHY_ADDR_WR(phy_id) | REG_ADDR_WR(reg);
	pr_debug("Write MII_MGMT_ADDR phy_id=0x%x, reg=0x%x, value=0x%x\n",
		 phy_id, reg << 2, value);
	xgene_enet_wr(priv, blockid, MII_MGMT_ADDRESS_ADDR, value);

	/* Write read command */
	xgene_enet_wr(priv, blockid, MII_MGMT_COMMAND_ADDR, READ_CYCLE_MASK);

	/* wait upto 20 us for completion */
	wait = 20;
	do {
		xgene_enet_rd(priv, blockid, MII_MGMT_INDICATORS_ADDR, &value);
		if (!(value & BUSY_MASK))
			break;
		usleep_range(1, 2);
	} while (--wait && (value & BUSY_MASK));

	xgene_enet_rd(priv, blockid, MII_MGMT_STATUS_ADDR, data);

	/* reset mii_mgmt_command register */
	xgene_enet_wr(priv, blockid, MII_MGMT_COMMAND_ADDR, 0);
}

inline void xgene_enet_port_reset(struct xgene_enet_priv *priv)
{
	if (priv->port_reset)
		priv->port_reset(priv);
}

inline void xgene_enet_mac_reset(struct xgene_enet_priv *priv)
{
	if (priv->mac_reset)
		priv->mac_reset(priv);
}

inline int xgene_enet_mac_init(struct xgene_enet_priv *priv,
			       unsigned char *dev_addr, int speed, int mtu,
			       int crc)
{
	int rc = 0;
	if (priv->mac_init)
		rc = priv->mac_init(priv, dev_addr, speed, mtu, crc);
	return rc;
}

inline void xgene_enet_mac_tx_state(struct xgene_enet_priv *priv, u32 enable)
{
	if (priv->mac_tx_state)
		priv->mac_tx_state(priv, enable);
}

inline void xgene_enet_mac_rx_state(struct xgene_enet_priv *priv, u32 enable)
{
	if (priv->mac_rx_state)
		priv->mac_rx_state(priv, enable);
}

inline void xgene_enet_mac_change_mtu(struct xgene_enet_priv *priv, u32 new_mtu)
{
	if (priv->mac_change_mtu)
		priv->mac_change_mtu(priv, new_mtu);
}

inline void xgene_enet_mac_set_ipg(struct xgene_enet_priv *priv, u16 ipg)
{
	if (priv->mac_set_ipg)
		priv->mac_set_ipg(priv, ipg);
}

inline void xgene_enet_get_stats(struct xgene_enet_priv *priv,
				 struct xgene_enet_detailed_stats *stats)
{
	if (priv->get_stats)
		priv->get_stats(priv, stats);
}

inline void xgene_enet_set_mac_addr(struct xgene_enet_priv *priv,
				    unsigned char *dev_addr)
{
	if (priv->set_mac_addr)
		priv->set_mac_addr(priv, dev_addr);
}

inline void xgene_enet_cle_bypass(struct xgene_enet_priv *priv,
				  u32 dstqid, u32 fpsel)
{
	if (priv->cle_bypass)
		priv->cle_bypass(priv, dstqid, fpsel);
}

inline void xgene_enet_tx_offload(struct xgene_enet_priv *priv,
				  u32 command, u32 value)
{
	if (priv->tx_offload)
		priv->tx_offload(priv, command, value);
}

inline void xgene_enet_port_shutdown(struct xgene_enet_priv *priv)
{
	if (priv->port_shutdown)
		priv->port_shutdown(priv);
}
