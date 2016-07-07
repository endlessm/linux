/* Marvell Wireless LAN device driver: TDLS handling
 *
 * Copyright (C) 2014, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available on the worldwide web at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#include <net/mac80211.h>
#include <net/netlink.h>
#include "vendor.h"
#include "main.h"

static int
marvell_vendor_cmd_set_turbo_mode(struct wiphy *wiphy,
				  struct wireless_dev *wdev,
				  const void *data, int data_len)
{
	struct mwifiex_private *priv = mwifiex_netdev_get_priv(wdev->netdev);
	u8 mode = *(u8 *)data;
	int ret;

	ret = mwifiex_send_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
			       HostCmd_ACT_GEN_SET, TURBO_MODE_I, &mode, true);

	return 0;
}

static int
mwifiex_vendor_cmd_set_cfg_data(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				const void *data, int data_len)
{
	struct mwifiex_private *priv = mwifiex_netdev_get_priv(wdev->netdev);
	int ret;

	priv->adapter->cfg_data = (u8 *)data;
	priv->adapter->cfg_len = data_len;

	ret = mwifiex_send_cmd(priv, HostCmd_CMD_CFG_DATA,
			       HostCmd_ACT_GEN_SET, 0, NULL, true);

	priv->adapter->cfg_data = NULL;
	priv->adapter->cfg_len = 0;

	return 0;
}

static const struct wiphy_vendor_command marvell_vendor_commands[] = {
	{
		.info = {
			.vendor_id = MARVELL_OUI,
			.subcmd = MARVELL_VENDOR_CMD_SET_TURBO_MODE,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = marvell_vendor_cmd_set_turbo_mode,
	},
	{
		.info = {
			.vendor_id = MARVELL_OUI,
			.subcmd = MARVELL_VENDOR_CMD_SET_CONF_DATA,
		},
		.flags = WIPHY_VENDOR_CMD_NEED_NETDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = mwifiex_vendor_cmd_set_cfg_data,
	},
};

void marvell_set_vendor_commands(struct wiphy *wiphy)
{
	wiphy->vendor_commands = marvell_vendor_commands;
	wiphy->n_vendor_commands = ARRAY_SIZE(marvell_vendor_commands);
}
