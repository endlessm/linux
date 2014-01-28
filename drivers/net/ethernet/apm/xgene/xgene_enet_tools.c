/* AppliedMicro X-Gene SoC Ethernet Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Authors:	Hrishikesh Karanjikar <hkaranjikar@apm.com>
 *		Ravi Patel <rapatel@apm.com>
 *		Iyappan Subramanian <isubramanian@apm.com>
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

#include <linux/mii.h>
#include <linux/phy.h>
#include "xgene_enet_csr.h"
#include "xgene_enet_main.h"

struct xgene_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

static const struct xgene_stats xgene_gstrings_stats[] = {
	{ "rx_bytes",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_byte_count),
		offsetof(struct xgene_enet_pdev, stats.rx_stats.rx_byte_count)
	},
	{ "rx_packets",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_packet_count),
		offsetof(struct xgene_enet_pdev, stats.rx_stats.rx_packet_count)
	},
	{ "rx_fcs_err",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_fcs_err_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_fcs_err_count)
	},
	{ "rx_alignment_err",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_alignment_err_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_alignment_err_pkt_count)
	},
	{ "rx_frm_len_err",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_frm_len_err_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_frm_len_err_pkt_count)
	},
	{ "rx_undersize",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_undersize_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_undersize_pkt_count)
	},
	{ "rx_oversize",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_oversize_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_oversize_pkt_count)
	},
	{ "rx_drop",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.rx_stats.rx_drop_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.rx_stats.rx_drop_pkt_count)
	},
	{ "tx_bytes",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.tx_stats.tx_byte_count),
		offsetof(struct xgene_enet_pdev,
			stats.tx_stats.tx_byte_count)
	},
	{ "tx_packets",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.tx_stats.tx_pkt_count),
		offsetof(struct xgene_enet_pdev,
			stats.tx_stats.tx_pkt_count)
	},
	{ "tx_drop",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.tx_stats.tx_drop_frm_count),
		offsetof(struct xgene_enet_pdev,
			stats.tx_stats.tx_drop_frm_count)
	},
	{ "tx_fcs_err",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.tx_stats.tx_fcs_err_frm_count),
		offsetof(struct xgene_enet_pdev,
			stats.tx_stats.tx_fcs_err_frm_count)
	},
	{ "tx_undersize",
		FIELD_SIZEOF(struct xgene_enet_pdev,
			stats.tx_stats.tx_undersize_frm_count),
		offsetof(struct xgene_enet_pdev,
			stats.tx_stats.tx_undersize_frm_count)
	},
};

#define XGENE_GLOBAL_STATS_LEN ARRAY_SIZE(xgene_gstrings_stats)

/* Ethtool APIs */
static int xgene_ethtool_get_settings(struct net_device *ndev,
		struct ethtool_cmd *cmd)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct phy_device *phydev = pdev->phy_dev;
	struct xgene_enet_priv *priv = &pdev->priv;

	if (priv->phy_mode == PHY_MODE_RGMII) {
		if (!phydev)
			return -ENODEV;
		return phy_ethtool_gset(phydev, cmd);
	}
	return 0;
}

static int xgene_ethtool_set_settings(struct net_device *ndev,
		struct ethtool_cmd *cmd)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct phy_device *phydev = pdev->phy_dev;
	struct xgene_enet_priv *priv = &pdev->priv;

	if (priv->phy_mode == PHY_MODE_RGMII) {
		if (!phydev)
			return -ENODEV;
		return phy_ethtool_sset(phydev, cmd);
	}
	return 0;
}

static int xgene_ethtool_set_pauseparam(struct net_device *ndev,
		struct ethtool_pauseparam *pp)
{
	u32 data;
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &data);

	/* Modify value to set or reset rx flow control */
	if (pp->rx_pause)
		data |= RX_FLOW_EN1_MASK;
	else
		data &= ~RX_FLOW_EN1_MASK;

	/* Modify value to set or reset tx flow control */
	if (pp->tx_pause)
		data |= TX_FLOW_EN1_MASK;
	else
		data &= ~TX_FLOW_EN1_MASK;

	xgene_enet_wr(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, data);

	return 0;
}

static void xgene_ethtool_get_pauseparam(struct net_device *ndev,
		struct ethtool_pauseparam *pp)
{
	u32 data;
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &data);
	pp->rx_pause = RX_FLOW_EN1_RD(data);

	xgene_enet_rd(priv, BLOCK_MCX_MAC, MAC_CONFIG_1_ADDR, &data);
	pp->tx_pause = TX_FLOW_EN1_RD(data);
}

static int xgene_ethtool_nway_reset(struct net_device *ndev)
{
	u32 data = 0, retry = 0;
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;

	if (priv->phy_mode == PHY_MODE_RGMII)
		mutex_lock(&pdev->mdio_bus->mdio_lock);

	/* Power-down PHY */
	data = MII_CR_POWER_DOWN;
	xgene_genericmiiphy_write(priv, priv->phy_addr,
			MII_CTRL_REG, data);

	/* Power-up PHY */
	data = 0x0;
	xgene_genericmiiphy_write(priv, priv->phy_addr,
			MII_CTRL_REG, data);

	/* Reset PHY */
	data = MII_CR_RESET;
	xgene_genericmiiphy_write(priv, priv->phy_addr,
			MII_CTRL_REG, data);

	/* PHY reset may take 100 ms */
	retry = 100;
	do {
		xgene_genericmiiphy_read(priv, priv->phy_addr,
			MII_CTRL_REG, &data);
		usleep_range(1000, 2000);
	} while (--retry && (data & MII_CR_RESET));

	xgene_genericmiiphy_write(priv, priv->phy_addr, MII_CTRL_REG,
			MII_CR_AUTO_EN|MII_CR_RESTART|MII_CR_FDX);

	priv->autoneg_set = 1;
	priv->speed = XGENE_ENET_SPEED_1000;
	priv->mac_init(priv, ndev->dev_addr, priv->speed,
			HW_MTU(ndev->mtu), priv->crc);

	if (priv->phy_mode == PHY_MODE_RGMII)
		mutex_unlock(&pdev->mdio_bus->mdio_lock);

	return 0;
}

static void xgene_get_strings(struct net_device *ndev, u32 stringset,
		    u8 *data)
{
	u8 *p = data;
	int i;

	switch (stringset) {
	case ETH_SS_TEST:
	case ETH_SS_STATS:
		for (i = 0; i < XGENE_GLOBAL_STATS_LEN; i++) {
			memcpy(p, xgene_gstrings_stats[i].stat_string,
					ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		break;
	}
}

static int xgene_get_sset_count(struct net_device *ndev, int sset)
{
	switch (sset) {
	case ETH_SS_TEST:
		return XGENE_GLOBAL_STATS_LEN;
	case ETH_SS_STATS:
		return XGENE_GLOBAL_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}

}

static void xgene_ethtool_get_ethtool_stats(struct net_device *ndev,
		struct ethtool_stats *ethtool_stats,
		u64 *data)
{

	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;
	struct xgene_enet_detailed_stats *stats = &pdev->stats;
	int i;

	xgene_enet_get_stats(priv, stats);
	for (i = 0; i < XGENE_GLOBAL_STATS_LEN; i++) {
		char *p = (char *)pdev + xgene_gstrings_stats[i].stat_offset;
		data[i] = (xgene_gstrings_stats[i].sizeof_stat ==
				sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}
}

static void xgene_ethtool_get_drvinfo(struct net_device *ndev,
		struct ethtool_drvinfo *info)
{
	strcpy(info->driver, ndev->name);
	strcpy(info->version, XGENE_ENET_DRIVER_VERSION);
	strcpy(info->fw_version, "N/A");
}

const struct ethtool_ops xgene_ethtool_ops = {
	.get_settings = xgene_ethtool_get_settings,
	.set_settings = xgene_ethtool_set_settings,
	.get_drvinfo = xgene_ethtool_get_drvinfo,
	.nway_reset = xgene_ethtool_nway_reset,
	.get_pauseparam = xgene_ethtool_get_pauseparam,
	.set_pauseparam = xgene_ethtool_set_pauseparam,
	.get_ethtool_stats = xgene_ethtool_get_ethtool_stats,
	.get_sset_count = xgene_get_sset_count,
	.get_strings = xgene_get_strings,
	.get_link = ethtool_op_get_link,
};
