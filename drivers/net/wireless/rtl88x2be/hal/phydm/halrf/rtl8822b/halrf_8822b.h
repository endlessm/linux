/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#ifndef __HAL_PHY_RF_8822B_H__
#define __HAL_PHY_RF_8822B_H__

#define AVG_THERMAL_NUM_8822B	4
#define RF_T_METER_8822B		0x42

#define LCK_VERSION	"0x2" 


void configure_txpower_track_8822b(
	struct _TXPWRTRACK_CFG	*p_config
);

void
odm_tx_pwr_track_set_pwr8822b(
	void				*p_dm_void,
	enum pwrtrack_method	method,
	u8				rf_path,
	u8				channel_mapped_index
);

void
get_delta_swing_table_8822b(
	void		*p_dm_void,
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	u8 **temperature_up_a,
	u8 **temperature_down_a,
	u8 **temperature_up_b,
	u8 **temperature_down_b,
	u8 **temperature_up_cck_a,
	u8 **temperature_down_cck_a,
	u8 **temperature_up_cck_b,
	u8 **temperature_down_cck_b
#else
	u8 **temperature_up_a,
	u8 **temperature_down_a,
	u8 **temperature_up_b,
	u8 **temperature_down_b
#endif
);

void
phy_lc_calibrate_8822b(
	void *p_dm_void
);



void phy_set_rf_path_switch_8822b(
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	struct PHY_DM_STRUCT		*p_dm_odm,
#elif (DM_ODM_SUPPORT_TYPE == ODM_CE) && defined(DM_ODM_CE_MAC80211)
	struct PHY_DM_STRUCT		*p_dm_odm,
#else
	struct _ADAPTER	*p_adapter,
#endif
	boolean		is_main
);

#endif	/* #ifndef __HAL_PHY_RF_8822B_H__ */
