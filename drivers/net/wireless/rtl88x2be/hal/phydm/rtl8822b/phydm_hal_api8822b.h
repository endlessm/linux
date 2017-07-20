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
#ifndef __INC_PHYDM_API_H_8822B__
#define __INC_PHYDM_API_H_8822B__

#if (RTL8822B_SUPPORT == 1)

#define	PHY_CONFIG_VERSION_8822B			"28.5.34"	/*2017.01.18     (HW user guide version: R28, SW user guide version: R05, Modification: R34), remove A cut setting, refine CCK txfilter and OFDM CCA setting by YuChen*/

#define	INVALID_RF_DATA					0xffffffff
#define	INVALID_TXAGC_DATA				0xff

#define number_of_psd_value				5
#define number_of_sample                3
#define number_of_2g_freq_pt			14
#define number_of_5g_freq_pt			10

#define	config_phydm_read_rf_check_8822b(data)			(data != INVALID_RF_DATA)
#define	config_phydm_read_txagc_check_8822b(data)		(data != INVALID_TXAGC_DATA)

u32
config_phydm_read_rf_reg_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_radio_path_e		rf_path,
	u32					reg_addr,
	u32					bit_mask
);

boolean
config_phydm_write_rf_reg_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_radio_path_e		rf_path,
	u32					reg_addr,
	u32					bit_mask,
	u32					data
);

boolean
config_phydm_write_txagc_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u32					power_index,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
);

u8
config_phydm_read_txagc_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
);

void
phydm_dynamic_spur_det_elimitor(
	struct PHY_DM_STRUCT				*p_dm_odm
);

boolean
config_phydm_switch_band_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch
);

boolean
config_phydm_switch_channel_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch
);

boolean
config_phydm_switch_bandwidth_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					primary_ch_idx,
	enum odm_bw_e				bandwidth
);

boolean
config_phydm_switch_channel_bw_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch,
	u8					primary_ch_idx,
	enum odm_bw_e				bandwidth
);

boolean
config_phydm_trx_mode_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_path_e			tx_path,
	enum odm_rf_path_e			rx_path,
	boolean					is_tx2_path
);

boolean
config_phydm_parameter_init_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_parameter_init_e	type
);


/* ======================================================================== */
/* These following functions can be used for PHY DM only*/

boolean
phydm_write_txagc_1byte_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u32					power_index,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
);

void
phydm_init_hw_info_by_rfe_type_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm
);

s32
phydm_get_condition_number_8822B(
	struct PHY_DM_STRUCT				*p_dm_odm
);

/* ======================================================================== */

#endif	/* RTL8822B_SUPPORT == 1 */
#endif	/*  __INC_PHYDM_API_H_8822B__ */
