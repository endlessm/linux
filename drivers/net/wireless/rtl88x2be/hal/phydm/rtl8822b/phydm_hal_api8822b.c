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
#include "mp_precomp.h"
#include "../phydm_precomp.h"


#if (RTL8822B_SUPPORT == 1)
#if (PHYDM_FW_API_ENABLE_8822B == 1)
/* ======================================================================== */
/* These following functions can be used for PHY DM only*/

enum odm_bw_e	bw_8822b;
u8	central_ch_8822b;

#if !(DM_ODM_SUPPORT_TYPE == ODM_CE)
	u32	cca_ifem_bcut[3][4] = {
		{0x75D97010, 0x75D97010, 0x75D97010, 0x75D97010}, /*Reg82C*/
		{0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
		{0x87766441, 0x87746341, 0x87765541, 0x87746341} /*Reg838*/
	};
	u32	cca_efem_bcut[3][4] = {
		{0x75B76010, 0x75B76010, 0x75B76010, 0x75B75010}, /*Reg82C*/
		{0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
		{0x87766451, 0x87766431, 0x87766451, 0x87766431} /*Reg838*/
	};
#endif

u32 cca_ifem_ccut[3][4] = {
	{0x75C97010, 0x75C97010, 0x75C97010, 0x75C97010}, /*Reg82C*/
	{0x79a0ea2a, 0x79A0EA2C, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
	{0x87765541, 0x87746341, 0x87765541, 0x87746341} /*Reg838*/
};
u32 cca_efem_ccut[3][4] = {
	{0x75B86010, 0x75B76010, 0x75B86010, 0x75B76010}, /*Reg82C*/
	{0x79A0EA28, 0x79A0EA2C, 0x79A0EA28, 0x79a0ea2a}, /*Reg830*/
	{0x87766451, 0x87766431, 0x87766451, 0x87766431} /*Reg838*/
};
u32 cca_ifem_ccut_rfetype[3][4] = {
	{0x75C97010, 0x75C97010, 0x75C97010, 0x75C97010}, /*Reg82C*/
	{0x79a0ea2a, 0x97A0EA2C, 0x79a0ea2a, 0x79a0ea2a}, /*Reg830*/
	{0x87765541, 0x86666341, 0x87765561, 0x86666361} /*Reg838*/
};

__iram_odm_func__
void
phydm_igi_toggle_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm
)
{
	u32 igi = 0x20;

	igi = odm_get_bb_reg(p_dm_odm, 0xc50, 0x7f);
	odm_set_bb_reg(p_dm_odm, 0xc50, 0x7f, (igi - 2));
	odm_set_bb_reg(p_dm_odm, 0xc50, 0x7f, igi);
	odm_set_bb_reg(p_dm_odm, 0xe50, 0x7f, (igi - 2));
	odm_set_bb_reg(p_dm_odm, 0xe50, 0x7f, igi);
}

__iram_odm_func__
u32
phydm_check_bit_mask(u32 bit_mask, u32 data_original, u32 data)
{
	u8 bit_shift;
	if (bit_mask != 0xfffff) {
		for (bit_shift = 0; bit_shift <= 19; bit_shift++) {
			if (((bit_mask >> bit_shift) & 0x1) == 1)
				break;
		}
		return ((data_original)&(~bit_mask)) | (data << bit_shift);
	}

	return data;
}

__iram_odm_func__
boolean
phydm_rfe_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					channel
)
{

	/* Distinguish the setting band */
	if (channel <= 14)
		p_dm_odm->rfe_hwsetting_band = 1;
	else
		p_dm_odm->rfe_hwsetting_band = 2;

	/* HW Setting for each RFE type */
	if ((p_dm_odm->rfe_type == 4) || (p_dm_odm->rfe_type == 11)) {

		/*TRSW  = trsw_forced_BT ? 0x804[0] : (0xCB8[2] ? 0xCB8[0] : trsw_lut);	trsw_lut = TXON*/
		/*TRSWB = trsw_forced_BT ? (~0x804[0]) : (0xCB8[2] ? 0xCB8[1] : trswb_lut);	trswb_lut = TXON*/
		/*trsw_forced_BT = 0x804[1] ? 0 : (~GNT_WL); */
		/*odm_set_bb_reg(p_dm_odm, 0x804, (BIT1|BIT0), 0x0);*/
		/* Default setting is in PHY parameters */

		if (channel <= 14) {
			/* signal source */
			odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x745774);
			odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x745774);
			odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x57);
			odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x57);

			/* inverse or not */
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x8);
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(11) | BIT(10)), 0x2);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x8);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(11) | BIT(10)), 0x2);

			/* antenna switch table */
			if ((p_dm_odm->rx_ant_status == (ODM_RF_A | ODM_RF_B)) || (p_dm_odm->tx_ant_status == (ODM_RF_A | ODM_RF_B))) {
				/* 2TX or 2RX */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xf050);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xf050);
			} else if (p_dm_odm->rx_ant_status == p_dm_odm->tx_ant_status) {
				/* TXA+RXA or TXB+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xf055);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xf055);
			} else {
				/* TXB+RXA or TXA+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xf550);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xf550);
			}

		} else if (channel > 35) {
			/* signal source */
			odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x477547);
			odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x477547);
			odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x75);
			odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x75);

			/* inverse or not */
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(11) | BIT(10)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(11) | BIT(10)), 0x0);

			/* antenna switch table */
			if ((p_dm_odm->rx_ant_status == (ODM_RF_A | ODM_RF_B)) || (p_dm_odm->tx_ant_status == (ODM_RF_A | ODM_RF_B))) {
				/* 2TX or 2RX */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa501);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa501);
			} else if (p_dm_odm->rx_ant_status == p_dm_odm->tx_ant_status) {
				/* TXA+RXA or TXB+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa500);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa500);
			} else {
				/* TXB+RXA or TXA+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa005);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa005);
			}
		} else
			return false;


	} else if ((p_dm_odm->rfe_type == 1) || (p_dm_odm->rfe_type == 2) || (p_dm_odm->rfe_type == 6) || (p_dm_odm->rfe_type == 7) || (p_dm_odm->rfe_type == 9)) {
		/* eFem */
		if ((p_dm_odm->cut_version == ODM_CUT_B) && (p_dm_odm->rfe_type < 2)) {
			if (channel <= 14) {
				/* signal source */
				odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x704570);
				odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x704570);
				odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x45);
				odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x45);
			} else if (channel > 35) {
				odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x174517);
				odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x174517);
				odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x45);
				odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x45);
			} else
				return false;

			/* delay 400ns for PAPE */
			odm_set_bb_reg(p_dm_odm, 0x810, MASKBYTE3 | BIT(20) | BIT(21) | BIT(22) | BIT(23), 0x211);

			/* antenna switch table */
			odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa555);
			odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa555);

			/* inverse or not */
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(11) | BIT(10)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(11) | BIT(10)), 0x0);

			ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Using old RFE control pin setting for A-cut and B-cut\n", __func__));
		} else {
			if (channel <= 14) {
				/* signal source */
				odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x705770);
				odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x705770);
				odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x57);
				odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x57);
				odm_set_bb_reg(p_dm_odm, 0xcb8, BIT(4), 0);
				odm_set_bb_reg(p_dm_odm, 0xeb8, BIT(4), 0);
			} else if (channel > 35) {
				/* signal source */
				odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x177517);
				odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x177517);
				odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x75);
				odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x75);
				odm_set_bb_reg(p_dm_odm, 0xcb8, BIT(5), 0);
				odm_set_bb_reg(p_dm_odm, 0xeb8, BIT(5), 0);
			} else
				return false;

			/* inverse or not */
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(11) | BIT(10)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(11) | BIT(10)), 0x0);

			/* delay 400ns for PAPE */
			/* odm_set_bb_reg(p_dm_odm, 0x810, MASKBYTE3|BIT20|BIT21|BIT22|BIT23, 0x211); */

			/* antenna switch table */
			if ((p_dm_odm->rx_ant_status == (ODM_RF_A | ODM_RF_B)) || (p_dm_odm->tx_ant_status == (ODM_RF_A | ODM_RF_B))) {
				/* 2TX or 2RX */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa501);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa501);
			} else if (p_dm_odm->rx_ant_status == p_dm_odm->tx_ant_status) {
				/* TXA+RXA or TXB+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa500);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa500);
			} else {
				/* TXB+RXA or TXA+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa005);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa005);
			}
		}
	} else if ((p_dm_odm->rfe_type == 0) || (p_dm_odm->rfe_type == 3) || (p_dm_odm->rfe_type == 5) || (p_dm_odm->rfe_type == 8) || (p_dm_odm->rfe_type == 10) || (p_dm_odm->rfe_type == 12)) {
		/* iFEM */
		if (channel <= 14) {
			/* signal source */
			odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x745774);
			odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x745774);
			odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x57);
			odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x57);
	
		} else if (channel > 35) {
			/* signal source */
			odm_set_bb_reg(p_dm_odm, 0xcb0, (MASKBYTE2 | MASKLWORD), 0x477547);
			odm_set_bb_reg(p_dm_odm, 0xeb0, (MASKBYTE2 | MASKLWORD), 0x477547);
			odm_set_bb_reg(p_dm_odm, 0xcb4, MASKBYTE1, 0x75);
			odm_set_bb_reg(p_dm_odm, 0xeb4, MASKBYTE1, 0x75);
			
		} else
			return false;

		/* inverse or not */
		odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xcbc, (BIT(11) | BIT(10)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xebc, (BIT(11) | BIT(10)), 0x0);

		/* antenna switch table */
		if (channel <= 14) {
			if ((p_dm_odm->rx_ant_status == (ODM_RF_A | ODM_RF_B)) || (p_dm_odm->tx_ant_status == (ODM_RF_A | ODM_RF_B))) {
				/* 2TX or 2RX */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa501);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa501);
			} else if (p_dm_odm->rx_ant_status == p_dm_odm->tx_ant_status) {
				/* TXA+RXA or TXB+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa500);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa500);
			} else {
				/* TXB+RXA or TXA+RXB */
				odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa005);
				odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa005);
			}
		} else if (channel > 35) {
			odm_set_bb_reg(p_dm_odm, 0xca0, MASKLWORD, 0xa5a5);
			odm_set_bb_reg(p_dm_odm, 0xea0, MASKLWORD, 0xa5a5);
		}
	}

	/* chip top mux */
	odm_set_bb_reg(p_dm_odm, 0x64, BIT(29) | BIT(28), 0x3);
	odm_set_bb_reg(p_dm_odm, 0x4c, BIT(26) | BIT(25), 0x0);
	odm_set_bb_reg(p_dm_odm, 0x40, BIT(2), 0x1);

	/* from s0 or s1 */
	odm_set_bb_reg(p_dm_odm, 0x1990, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x30);
	odm_set_bb_reg(p_dm_odm, 0x1990, (BIT(11) | BIT(10)), 0x3);

	/* input or output */
	odm_set_bb_reg(p_dm_odm, 0x974, (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)), 0x3f);
	odm_set_bb_reg(p_dm_odm, 0x974, (BIT(11) | BIT(10)), 0x3);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Update RFE control pin setting (ch%d, tx_path 0x%x, rx_path 0x%x)\n", __func__, channel, p_dm_odm->tx_ant_status, p_dm_odm->rx_ant_status));

	return true;
}

__iram_odm_func__
u8
phydm_is_dfs_channel(u8 channel_num)
{
	if(channel_num >= 52 && channel_num <= 140)
		return 1;
	else
		return 0;
}

__iram_odm_func__
void
phydm_ccapar_by_rfe_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm
)
{
	u32	cca_ifem[3][4], cca_efem[3][4];
	u8	col;
	u32	reg82c, reg830, reg838;
	boolean	is_efem_cca = false, is_ifem_cca = false, is_rfe_type = false;

#if !(DM_ODM_SUPPORT_TYPE == ODM_CE)
	if (p_dm_odm->cut_version == ODM_CUT_B) {
		odm_move_memory(p_dm_odm, cca_efem, cca_efem_bcut, 12 * 4);
		odm_move_memory(p_dm_odm, cca_ifem, cca_ifem_bcut, 12 * 4);
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Update CCA parameters for Bcut\n", __func__));
	} else
#endif
	{
		odm_move_memory(p_dm_odm, cca_efem, cca_efem_ccut, 12 * 4);
	if ((p_dm_odm->rfe_type == 3) || (p_dm_odm->rfe_type == 5)) {
		odm_move_memory(p_dm_odm, cca_ifem, cca_ifem_ccut_rfetype, 12 * 4);
		is_rfe_type = true;
	} else
		odm_move_memory(p_dm_odm, cca_ifem, cca_ifem_ccut, 12 * 4);
	
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Update CCA parameters for Ccut\n", __func__));
	}

	if (central_ch_8822b <= 14) {
		if ((p_dm_odm->rx_ant_status == ODM_RF_A) || (p_dm_odm->rx_ant_status == ODM_RF_B))
			col = 0;	/*1R 2G*/
		else
			col = 1;	/*2R 2G*/
	} else {
		if ((p_dm_odm->rx_ant_status == ODM_RF_A) || (p_dm_odm->rx_ant_status == ODM_RF_B))
			col = 2;	/*1R 5G*/
		else
			col = 3;	/*2R 5G*/
	}

	if ((p_dm_odm->rfe_type == 1) || (p_dm_odm->rfe_type == 4) || (p_dm_odm->rfe_type == 6) || (p_dm_odm->rfe_type == 7) || (p_dm_odm->rfe_type == 11)) {
		/*eFEM => RFE type 1 & RFE type 4 & RFE type 6 & RFE type 7 & RFE type 11*/
		reg82c = cca_efem[0][col];
		reg830 = cca_efem[1][col];
		reg838 = cca_efem[2][col];
		is_efem_cca = true;
	} else if ((p_dm_odm->rfe_type == 2) || (p_dm_odm->rfe_type == 9)) {
		/*5G eFEM, 2G iFEM => RFE type 2, 5G eFEM => RFE type 9 */
		if (central_ch_8822b <= 14) {
			reg82c = cca_ifem[0][col];
			reg830 = cca_ifem[1][col];
			reg838 = cca_ifem[2][col];
			is_ifem_cca = true;
		} else {
			reg82c = cca_efem[0][col];
			reg830 = cca_efem[1][col];
			reg838 = cca_efem[2][col];
			is_efem_cca = true;
		}
	} else {
		/* iFEM =>RFEtype 3 & RFE type 5 & RFE type 0 & RFE type 8 & RFE type 10 & RFE type 12*/
		reg82c = cca_ifem[0][col];
		reg830 = cca_ifem[1][col];
		reg838 = cca_ifem[2][col];
		is_ifem_cca = true;
	}

	odm_set_bb_reg(p_dm_odm, 0x82c, MASKDWORD, reg82c);

	if (is_ifem_cca == true)
		if (((p_dm_odm->cut_version == ODM_CUT_B) && (col == 1 || col == 3) && (bw_8822b == ODM_BW40M)) ||
			((is_rfe_type == false) && (col == 3) && (bw_8822b == ODM_BW40M)) ||
			((p_dm_odm->rfe_type == 5) && (col == 3)))
			odm_set_bb_reg(p_dm_odm, 0x830, MASKDWORD, 0x79a0ea28);
		else
			odm_set_bb_reg(p_dm_odm, 0x830, MASKDWORD, reg830);
	else
		odm_set_bb_reg(p_dm_odm, 0x830, MASKDWORD, reg830);

	odm_set_bb_reg(p_dm_odm, 0x838, MASKDWORD, reg838);

	if ((is_efem_cca == true) && !(p_dm_odm->cut_version == ODM_CUT_B))
		odm_set_bb_reg(p_dm_odm, 0x83c, MASKDWORD, 0x9194b2b9);

	if (phydm_is_dfs_channel(central_ch_8822b) && (bw_8822b == ODM_BW80M))
		odm_set_bb_reg(p_dm_odm, 0x838, 0x1, 0);
	
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: (Pkt%d, Intf%d, RFE%d), col = %d\n",
		__func__, p_dm_odm->package_type, p_dm_odm->support_interface, p_dm_odm->rfe_type, col));
}

__iram_odm_func__
void
phydm_rxdfirpar_by_bw_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_bw_e				bandwidth
)
{
	if (bandwidth == ODM_BW40M) {
		/* RX DFIR for BW40 */
		odm_set_bb_reg(p_dm_odm, 0x948, BIT(29) | BIT(28), 0x1);
		odm_set_bb_reg(p_dm_odm, 0x94c, BIT(29) | BIT(28), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xc20, BIT(31), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xe20, BIT(31), 0x0);
	} else if (bandwidth == ODM_BW80M) {
		/* RX DFIR for BW80 */
		odm_set_bb_reg(p_dm_odm, 0x948, BIT(29) | BIT(28), 0x2);
		odm_set_bb_reg(p_dm_odm, 0x94c, BIT(29) | BIT(28), 0x1);
		odm_set_bb_reg(p_dm_odm, 0xc20, BIT(31), 0x0);
		odm_set_bb_reg(p_dm_odm, 0xe20, BIT(31), 0x0);
	} else {
		/* RX DFIR for BW20, BW10 and BW5*/
		odm_set_bb_reg(p_dm_odm, 0x948, BIT(29) | BIT(28), 0x2);
		odm_set_bb_reg(p_dm_odm, 0x94c, BIT(29) | BIT(28), 0x2);
		odm_set_bb_reg(p_dm_odm, 0xc20, BIT(31), 0x1);
		odm_set_bb_reg(p_dm_odm, 0xe20, BIT(31), 0x1);
	}
}

__iram_odm_func__
boolean
phydm_write_txagc_1byte_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u32					power_index,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)

	u32	offset_txagc[2] = {0x1d00, 0x1d80};
	u8	rate_idx = (hw_rate & 0xfc), i;
	u8	rate_offset = (hw_rate & 0x3);
	u32	txagc_content = 0x0;

	/* For debug command only!!!! */

	/* Error handling */
	if ((path > ODM_RF_PATH_B) || (hw_rate > 0x53)) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("phydm_write_txagc_1byte_8822b(): unsupported path (%d)\n", path));
		return false;
	}

	/* For HW limitation, We can't write TXAGC once a byte. */
	for (i = 0; i < 4; i++) {
		if (i != rate_offset)
			txagc_content = txagc_content | (config_phydm_read_txagc_8822b(p_dm_odm, path, rate_idx + i) << (i << 3));
		else
			txagc_content = txagc_content | ((power_index & 0x3f) << (i << 3));
	}
	odm_set_bb_reg(p_dm_odm, (offset_txagc[path] + rate_idx), MASKDWORD, txagc_content);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("phydm_write_txagc_1byte_8822b(): path-%d rate index 0x%x (0x%x) = 0x%x\n",
		path, hw_rate, (offset_txagc[path] + hw_rate), power_index));
	return true;
#else
	return false;
#endif
}

__iram_odm_func__
void
phydm_init_hw_info_by_rfe_type_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm
)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)
	u16	mask_path_a = 0x0303;
	u16	mask_path_b = 0x0c0c;
	/*u16	mask_path_c = 0x3030;*/
	/*u16	mask_path_d = 0xc0c0;*/

	p_dm_odm->is_init_hw_info_by_rfe = false;

	if ((p_dm_odm->rfe_type == 1) || (p_dm_odm->rfe_type == 6) || (p_dm_odm->rfe_type == 7)) {
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, (ODM_BOARD_EXT_LNA | ODM_BOARD_EXT_LNA_5G | ODM_BOARD_EXT_PA | ODM_BOARD_EXT_PA_5G));

		if (p_dm_odm->rfe_type == 6) {
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GPA, (TYPE_GPA1 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA1 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GLNA, (TYPE_GLNA1 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA1 & (mask_path_a | mask_path_b)));
		} else if (p_dm_odm->rfe_type == 7) {
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GPA, (TYPE_GPA2 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA2 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GLNA, (TYPE_GLNA2 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA2 & (mask_path_a | mask_path_b)));
		} else {
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GPA, (TYPE_GPA0 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA0 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GLNA, (TYPE_GLNA0 & (mask_path_a | mask_path_b)));
			odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA0 & (mask_path_a | mask_path_b)));
		}

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 1);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, true);
	} else if (p_dm_odm->rfe_type == 2) {
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, (ODM_BOARD_EXT_LNA_5G | ODM_BOARD_EXT_PA_5G));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA0 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA0 & (mask_path_a | mask_path_b)));

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, true);
	} else if (p_dm_odm->rfe_type == 9) {
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, (ODM_BOARD_EXT_LNA_5G));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA0 & (mask_path_a | mask_path_b)));

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 1);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else if (p_dm_odm->rfe_type == 3) {
		/* RFE type 3: 8822BS\8822BU TFBGA iFEM */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, 0);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else if (p_dm_odm->rfe_type == 5) {
		/* RFE type 5: 8822BE TFBGA iFEM */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, ODM_BOARD_SLIM);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else if (p_dm_odm->rfe_type == 12) {
		/* RFE type 12: QFN iFEM */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, 0);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 1);
		
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else if (p_dm_odm->rfe_type == 4) {
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, (ODM_BOARD_EXT_LNA | ODM_BOARD_EXT_LNA_5G | ODM_BOARD_EXT_PA | ODM_BOARD_EXT_PA_5G));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GPA, (TYPE_GPA0 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA0 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GLNA, (TYPE_GLNA0 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA0 & (mask_path_a | mask_path_b)));

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, true);
	} else if (p_dm_odm->rfe_type == 11) {
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, (ODM_BOARD_EXT_LNA | ODM_BOARD_EXT_LNA_5G | ODM_BOARD_EXT_PA | ODM_BOARD_EXT_PA_5G));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GPA, (TYPE_GPA1 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_APA, (TYPE_APA1 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_GLNA, (TYPE_GLNA1 & (mask_path_a | mask_path_b)));
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_ALNA, (TYPE_ALNA1 & (mask_path_a | mask_path_b)));
		
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);
		
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, true);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, true);

	} else if (p_dm_odm->rfe_type == 8) {
	/* RFE type 8: TFBGA iFEM AP */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, 0);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 2);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else if (p_dm_odm->rfe_type == 10) {
	/* RFE type 10: QFN iFEM AP PCIE TRSW */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, ODM_BOARD_EXT_TRSW);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 1);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	} else {
	/* RFE Type 0: QFN iFEM */
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_BOARD_TYPE, 0);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_PACKAGE_TYPE, 1);

		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_LNA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_EXT_PA, false);
		odm_cmn_info_init(p_dm_odm, ODM_CMNINFO_5G_EXT_PA, false);
	}

	p_dm_odm->is_init_hw_info_by_rfe = true;

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE,
		("%s: RFE type (%d), Board type (0x%x), Package type (%d)\n", __func__, p_dm_odm->rfe_type, p_dm_odm->board_type, p_dm_odm->package_type));
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE,
		("%s: 5G ePA (%d), 5G eLNA (%d), 2G ePA (%d), 2G eLNA (%d)\n", __func__, p_dm_odm->ext_pa_5g, p_dm_odm->ext_lna_5g, p_dm_odm->ext_pa, p_dm_odm->ext_lna));
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE,
		("%s: 5G PA type (%d), 5G LNA type (%d), 2G PA type (%d), 2G LNA type (%d)\n", __func__, p_dm_odm->type_apa, p_dm_odm->type_alna, p_dm_odm->type_gpa, p_dm_odm->type_glna));

#endif	/*PHYDM_FW_API_FUNC_ENABLE_8822B == 1*/
}

__iram_odm_func__
s32
phydm_get_condition_number_8822B(
	struct PHY_DM_STRUCT				*p_dm_odm
)
{
	s32	ret_val;

	odm_set_bb_reg(p_dm_odm, 0x1988, BIT(22), 0x1);
	ret_val = (s32)odm_get_bb_reg(p_dm_odm, 0xf84, (BIT(17) | BIT(16) | MASKLWORD));

	if (bw_8822b == 0) {
		ret_val = ret_val << (8 - 4);
		ret_val = ret_val / 234;
	} else if (bw_8822b == 1) {
		ret_val = ret_val << (7 - 4);
		ret_val = ret_val / 108;
	} else if (bw_8822b == 2) {
		ret_val = ret_val << (6 - 4);
		ret_val = ret_val / 52;
	}

	return ret_val;
}

/* ======================================================================== */

/* ======================================================================== */
/* These following functions can be used by driver*/

__iram_odm_func__
u32
config_phydm_read_rf_reg_8822b(
	struct PHY_DM_STRUCT		*p_dm_odm,
	enum odm_rf_radio_path_e		rf_path,
	u32					reg_addr,
	u32					bit_mask
)
{
	u32	readback_value, direct_addr;
	u32	offset_read_rf[2] = {0x2800, 0x2c00};

	/* Error handling.*/
	if (rf_path > ODM_RF_PATH_B) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_read_rf_reg_8822b(): unsupported path (%d)\n", rf_path));
		return INVALID_RF_DATA;
	}

	/* Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_read_rf[rf_path] + (reg_addr << 2);

	/* RF register only has 20bits */
	bit_mask &= RFREGOFFSETMASK;

	/* Read RF register directly */
	readback_value = odm_get_bb_reg(p_dm_odm, direct_addr, bit_mask);
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_read_rf_reg_8822b(): RF-%d 0x%x = 0x%x, bit mask = 0x%x\n",
			rf_path, reg_addr, readback_value, bit_mask));
	return readback_value;
}

__iram_odm_func__
boolean
config_phydm_write_rf_reg_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_radio_path_e		rf_path,
	u32					reg_addr,
	u32					bit_mask,
	u32					data
)
{
	u32	data_and_addr = 0, data_original = 0;
	u32	offset_write_rf[2] = {0xc90, 0xe90};

	/* Error handling.*/
	if (rf_path > ODM_RF_PATH_B) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_rf_reg_8822b(): unsupported path (%d)\n", rf_path));
		return false;
	}

	/* Read RF register content first */
	reg_addr &= 0xff;
	bit_mask = bit_mask & RFREGOFFSETMASK;

	if (bit_mask != RFREGOFFSETMASK) {
		data_original = config_phydm_read_rf_reg_8822b(p_dm_odm, rf_path, reg_addr, RFREGOFFSETMASK);

		/* Error handling. RF is disabled */
		if (config_phydm_read_rf_check_8822b(data_original) == false) {
			ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_rf_reg_8822b(): Write fail, RF is disable\n"));
			return false;
		}

		/* check bit mask */
		data = phydm_check_bit_mask(bit_mask, data_original, data);
	}

	/* Put write addr in [27:20]  and write data in [19:00] */
	data_and_addr = ((reg_addr << 20) | (data & 0x000fffff)) & 0x0fffffff;

	/* Write operation */
	odm_set_bb_reg(p_dm_odm, offset_write_rf[rf_path], MASKDWORD, data_and_addr);
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_rf_reg_8822b(): RF-%d 0x%x = 0x%x (original: 0x%x), bit mask = 0x%x\n",
			rf_path, reg_addr, data, data_original, bit_mask));
	return true;
}

__iram_odm_func__
boolean
config_phydm_write_txagc_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u32					power_index,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)

	u32	offset_txagc[2] = {0x1d00, 0x1d80};
	u8	rate_idx = (hw_rate & 0xfc);

	/* Input need to be HW rate index, not driver rate index!!!! */

	if (p_dm_odm->is_disable_phy_api) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_txagc_8822b(): disable PHY API for debug!!\n"));
		return true;
	}

	/* Error handling */
	if ((path > ODM_RF_PATH_B) || (hw_rate > 0x53)) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_txagc_8822b(): unsupported path (%d)\n", path));
		return false;
	}

	/* driver need to construct a 4-byte power index */
	odm_set_bb_reg(p_dm_odm, (offset_txagc[path] + rate_idx), MASKDWORD, power_index);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_write_txagc_8822b(): path-%d rate index 0x%x (0x%x) = 0x%x\n",
		path, hw_rate, (offset_txagc[path] + hw_rate), power_index));
	return true;

#else
	return false;
#endif
}

__iram_odm_func__
u8
config_phydm_read_txagc_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_radio_path_e		path,
	u8					hw_rate
)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)
	u8	read_back_data;

	/* Input need to be HW rate index, not driver rate index!!!! */

	/* Error handling */
	if ((path > ODM_RF_PATH_B) || (hw_rate > 0x53)) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_read_txagc_8822b(): unsupported path (%d)\n", path));
		return INVALID_TXAGC_DATA;
	}

	/* Disable TX AGC report */
	odm_set_bb_reg(p_dm_odm, 0x1998, BIT(16), 0x0);							/* need to check */

	/* Set data rate index (bit0~6) and path index (bit7) */
	odm_set_bb_reg(p_dm_odm, 0x1998, MASKBYTE0, (hw_rate | (path << 7)));

	/* Enable TXAGC report */
	odm_set_bb_reg(p_dm_odm, 0x1998, BIT(16), 0x1);

	/* Read TX AGC report */
	read_back_data = (u8)odm_get_bb_reg(p_dm_odm, 0xd30, 0x7f0000);

	/* Driver have to disable TXAGC report after reading TXAGC (ref. user guide v11) */
	odm_set_bb_reg(p_dm_odm, 0x1998, BIT(16), 0x0);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_read_txagc_8822b(): path-%d rate index 0x%x = 0x%x\n", path, hw_rate, read_back_data));
	return read_back_data;
#else
	return 0;
#endif
}

__iram_odm_func__
void
phydm_dynamic_spur_det_elimitor(
	struct PHY_DM_STRUCT				*p_dm_odm
)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)

	u32		freq_2g[number_of_2g_freq_pt] = {0xFC67, 0xFC27, 0xFFE6, 0xFFA6, 0xFC67, 0xFCE7, 0xFCA7, 0xFC67, 0xFC27, 0xFFE6, 0xFFA6, 0xFF66, 0xFF26, 0xFCE7};
	u32		freq_5g[number_of_5g_freq_pt] = {0xFFC0, 0xFFC0, 0xFC81, 0xFC81, 0xFC41, 0xFC40, 0xFF80, 0xFF80, 0xFF40, 0xFD42};
	u32		freq_2g_n1[number_of_2g_freq_pt] = {0}, freq_2g_p1[number_of_2g_freq_pt] = {0};
	u32		freq_5g_n1[number_of_5g_freq_pt] = {0}, freq_5g_p1[number_of_5g_freq_pt] = {0};
	u32		freq_pt_2g_final = 0, freq_pt_5g_final = 0, freq_pt_2g_b_final = 0, freq_pt_5g_b_final = 0;
	u32		max_ret_psd_final = 0, max_ret_psd_b_final = 0;
	u32		max_ret_psd_2nd[number_of_sample] = {0}, max_ret_psd_b_2nd[number_of_sample] = {0};
	u32		psd_set[number_of_psd_value] = {0}, psd_set_B[number_of_psd_value] = {0};
	u32		rank_psd_index_in[number_of_psd_value] = {0}, rank_sample_index_in[number_of_sample] = {0};
	u32		rank_psd_index_out[number_of_psd_value] = {0};
	u32		rank_sample_index_out[number_of_sample] = {0};
	u32		reg_910_15_12 = 0;
	u8		j = 0, k = 0, threshold_nbi = 0x8D, threshold_csi = 0x8D;
	u8		idx = 0, set_result_nbi = SET_NO_NEED, set_result_csi = SET_NO_NEED;
	boolean	s_dopsd = false, s_donbi_a = false, s_docsi = false, s_donbi_b = false;

	if (!(p_dm_odm->support_ability & ODM_BB_DYNAMIC_PSDTOOL)) {
		ODM_RT_TRACE(p_dm_odm, ODM_BB_DYNAMIC_PSDTOOL, ODM_DBG_LOUD, ("[Return Init]   Not Support Dynamic Spur Detection and Eliminator\n"));
		return;
	}

	ODM_RT_TRACE(p_dm_odm, ODM_BB_DYNAMIC_PSDTOOL, ODM_DBG_LOUD, ("Dynamic Spur Detection and Eliminator is ON\n"));

	/* 2G Channel Setting > 20M: 5, 6, 7, 8, 13; 40M: 3~11 */
	if ((*p_dm_odm->p_channel >= 1) && (*p_dm_odm->p_channel <= 14)) {
		if (*p_dm_odm->p_band_width == ODM_BW20M) {

			if (*p_dm_odm->p_channel >= 5 && *p_dm_odm->p_channel <= 8)
				idx = *p_dm_odm->p_channel - 5;
			else if (*p_dm_odm->p_channel == 13)
				idx = 4;
			else
				idx = 16;
		} else {

			if (*p_dm_odm->p_channel >= 3 && *p_dm_odm->p_channel <= 11)
				idx = *p_dm_odm->p_channel + 2;
			else
				idx = 16;
		}
	} else { /* 5G Channel Setting > 20M: 153, 161; 40M: 54, 118, 151, 159; 80M: 58, 122, 155, 155 */
		switch (*p_dm_odm->p_channel) {
		case 153:
			idx = 0;
			break;
		case 161:
			idx = 1;
			break;
		case 54:
			idx = 2;
			break;
		case 118:
			idx = 3;
			break;
		case 151:
			idx = 4;
			break;
		case 159:
			idx = 5;
			break;
		case 58:
			idx = 6;
			break;
		case 122:
			idx = 7;
			break;
		case 155:
			idx = 8;
			break;
		default:
			idx = 16;
			break;
		}
	}

	if (idx == 16)
		s_dopsd = false;
	else
		s_dopsd = true;

	ODM_RT_TRACE(p_dm_odm, ODM_BB_DYNAMIC_PSDTOOL, ODM_DBG_LOUD, ("[%s] idx = %d, BW = %d, Channel = %d\n", __func__, idx, *p_dm_odm->p_band_width, *p_dm_odm->p_channel));

	for (k = 0; k < number_of_2g_freq_pt; k++) {
		freq_2g_n1[k] = freq_2g[k] - 1;
		freq_2g_p1[k] = freq_2g[k] + 1;
	}

	for (k = 0; k < number_of_5g_freq_pt; k++) {
		freq_5g_n1[k] = freq_5g[k] - 1;
		freq_5g_p1[k] = freq_5g[k] + 1;
	}

	if ((s_dopsd == TRUE) && (idx <= 13)) {
		for (k = 0; k < number_of_sample; k++) {
			if (k == 0) {
				freq_pt_2g_final = freq_2g_n1[idx];
				freq_pt_5g_final = freq_5g_n1[idx];
				freq_pt_2g_b_final = freq_2g_n1[idx] | BIT(16);
				freq_pt_5g_b_final = freq_5g_n1[idx] | BIT(16);
			} else if (k == 1) {
				freq_pt_2g_final = freq_2g[idx];
				freq_pt_5g_final = freq_5g[idx];
				freq_pt_2g_b_final = freq_2g[idx] | BIT(16);
				freq_pt_5g_b_final = freq_5g[idx] | BIT(16);
			} else if (k == 2) {
				freq_pt_2g_final = freq_2g_p1[idx];
				freq_pt_5g_final = freq_5g_p1[idx];
				freq_pt_2g_b_final = freq_2g_p1[idx] | BIT(16);
				freq_pt_5g_b_final = freq_5g_p1[idx] | BIT(16);
			}

			for (j = 0; j < number_of_psd_value; j++) {
				odm_set_bb_reg(p_dm_odm, 0xc00, MASKBYTE0, 0x4);/* disable 3-wire, path-A */
				odm_set_bb_reg(p_dm_odm, 0xe00, MASKBYTE0, 0x4);/* disable 3-wire, path-B */
				reg_910_15_12 = odm_get_bb_reg(p_dm_odm, 0x910, (BIT15 | BIT14 | BIT13 | BIT12));

				if (p_dm_odm->rx_ant_status & ODM_RF_A) {
					odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, (((ODM_RF_A)<<4) | ODM_RF_A));/*path-A*/

					if ((*p_dm_odm->p_channel >= 1) && (*p_dm_odm->p_channel <= 14))
						odm_set_bb_reg(p_dm_odm, 0x910, MASKDWORD, BIT(22) | freq_pt_2g_final);/* Start PSD */
					else
						odm_set_bb_reg(p_dm_odm, 0x910, MASKDWORD, BIT(22) | freq_pt_5g_final);/* Start PSD */

					ODM_delay_us(500);

					psd_set[j] = odm_get_bb_reg(p_dm_odm, 0xf44, MASKLWORD);

					odm_set_bb_reg(p_dm_odm, 0x910, BIT22, 0x0);/* turn off PSD */
				}

				if (p_dm_odm->rx_ant_status & ODM_RF_B) {
					odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, (((ODM_RF_B)<<4) | ODM_RF_B));/*path-B*/

					if ((*p_dm_odm->p_channel > 0) && (*p_dm_odm->p_channel <= 14))
						odm_set_bb_reg(p_dm_odm, 0x910, MASKDWORD, BIT(22) | freq_pt_2g_b_final);/* Start PSD */
					else
						odm_set_bb_reg(p_dm_odm, 0x910, MASKDWORD, BIT(22) | freq_pt_5g_b_final);/* Start PSD */

					ODM_delay_us(500);

					psd_set_B[j] = odm_get_bb_reg(p_dm_odm, 0xf44, MASKLWORD);

					odm_set_bb_reg(p_dm_odm, 0x910, BIT22, 0x0);/* turn off PSD */
				}

				odm_set_bb_reg(p_dm_odm, 0xc00, MASKBYTE0, 0x7);/*eanble 3-wire*/
				odm_set_bb_reg(p_dm_odm, 0xe00, MASKBYTE0, 0x7);
				odm_set_bb_reg(p_dm_odm, 0x910, (BIT15 | BIT14 | BIT13 | BIT12), reg_910_15_12);

				odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, (((p_dm_odm->rx_ant_status)<<4) | p_dm_odm->rx_ant_status));

				/* Toggle IGI to let RF enter RX mode, because BB doesn't send 3-wire command when RX path is enable */
				phydm_igi_toggle_8822b(p_dm_odm);

			}
			if (p_dm_odm->rx_ant_status & ODM_RF_A) {
				phydm_seq_sorting(p_dm_odm, psd_set, rank_psd_index_in, rank_psd_index_out, number_of_psd_value);
				max_ret_psd_2nd[k] = psd_set[0];
			}
			if (p_dm_odm->rx_ant_status & ODM_RF_B) {
				phydm_seq_sorting(p_dm_odm, psd_set_B, rank_psd_index_in, rank_psd_index_out, number_of_psd_value);
				max_ret_psd_b_2nd[k] = psd_set_B[0];
			}
		}

		if (p_dm_odm->rx_ant_status & ODM_RF_A) {
			phydm_seq_sorting(p_dm_odm, max_ret_psd_2nd, rank_sample_index_in, rank_sample_index_out, number_of_sample);
			max_ret_psd_final = max_ret_psd_2nd[0];

			if (max_ret_psd_final >= threshold_nbi)
				s_donbi_a = true;
			else
				s_donbi_a = false;
		}
		if (p_dm_odm->rx_ant_status & ODM_RF_B) {
			phydm_seq_sorting(p_dm_odm, max_ret_psd_b_2nd, rank_sample_index_in, rank_sample_index_out, number_of_sample);
			max_ret_psd_b_final = max_ret_psd_b_2nd[0];

			if (max_ret_psd_b_final >= threshold_nbi)
				s_donbi_b = true;
			else
				s_donbi_b = false;
		}

		ODM_RT_TRACE(p_dm_odm, ODM_BB_DYNAMIC_PSDTOOL, ODM_DBG_LOUD, ("[%s] max_ret_psd_final = %d, max_ret_psd_b_final = %d\n", __func__, max_ret_psd_final, max_ret_psd_b_final));

		if ((max_ret_psd_final >= threshold_csi) || (max_ret_psd_b_final >= threshold_csi))
			s_docsi = true;
		else
			s_docsi = false;

	}

	/* Reset NBI/CSI everytime after changing channel/BW/band  */
	odm_set_bb_reg(p_dm_odm, 0x880, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x884, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x888, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x88c, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x890, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x894, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x898, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x89c, MASKDWORD, 0);
	odm_set_bb_reg(p_dm_odm, 0x874, BIT(0), 0x0);

	odm_set_bb_reg(p_dm_odm, 0x87c, BIT(13), 0x0);
	odm_set_bb_reg(p_dm_odm, 0xc20, BIT(28), 0x0);
	odm_set_bb_reg(p_dm_odm, 0xe20, BIT(28), 0x0);

	if (s_donbi_a == true || s_donbi_b == true) {
		if (*p_dm_odm->p_band_width == ODM_BW20M) {
			if (*p_dm_odm->p_channel == 153)
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 20, 5760, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 161)
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 20, 5800, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel >= 5 && *p_dm_odm->p_channel <= 8)
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 20, 2440, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 13)
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 20, 2480, PHYDM_DONT_CARE);
			else
				set_result_nbi = SET_NO_NEED;
		} else if (*p_dm_odm->p_band_width == ODM_BW40M) {
			if (*p_dm_odm->p_channel == 54) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 5280, PHYDM_DONT_CARE);
			} else if (*p_dm_odm->p_channel == 118) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 5600, PHYDM_DONT_CARE);
			} else if (*p_dm_odm->p_channel == 151) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 5760, PHYDM_DONT_CARE);
			} else if (*p_dm_odm->p_channel == 159) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 5800, PHYDM_DONT_CARE);
				/* 2.4G */
			} else if ((*p_dm_odm->p_channel >= 4) && (*p_dm_odm->p_channel <= 6)) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 2440, PHYDM_DONT_CARE);
			} else if (*p_dm_odm->p_channel == 11) {
				set_result_nbi = phydm_nbi_setting(p_dm_odm, NBI_ENABLE, *p_dm_odm->p_channel, 40, 2480, PHYDM_DONT_CARE);
			} else
				set_result_nbi = SET_NO_NEED;
		} else
			set_result_nbi = SET_NO_NEED;
	}

	if (s_docsi == true) {
		if (*p_dm_odm->p_band_width == ODM_BW20M) {
			if (*p_dm_odm->p_channel == 153)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 20, 5760, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 161)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 20, 5800, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel >= 5 && *p_dm_odm->p_channel <= 8)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 20, 2440, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 13)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 20, 2480, PHYDM_DONT_CARE);
			else
				set_result_csi = SET_NO_NEED;
		} else if (*p_dm_odm->p_band_width == ODM_BW40M) {
			if (*p_dm_odm->p_channel == 54)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 5280, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 118)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 5600, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 151)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 5760, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 159)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 5800, PHYDM_DONT_CARE);
			else if ((*p_dm_odm->p_channel >= 3) && (*p_dm_odm->p_channel <= 10))
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 2440, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 11)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 40, 2480, PHYDM_DONT_CARE);
			else
				set_result_csi = SET_NO_NEED;
		} else if (*p_dm_odm->p_band_width == ODM_BW80M) {
			if (*p_dm_odm->p_channel == 58)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 80, 5280, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 122)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 80, 5600, PHYDM_DONT_CARE);
			else if (*p_dm_odm->p_channel == 155)
				set_result_csi = phydm_csi_mask_setting(p_dm_odm, CSI_MASK_ENABLE, *p_dm_odm->p_channel, 80, 5760, PHYDM_DONT_CARE);
			else
				set_result_csi = SET_NO_NEED;
		} else
			set_result_csi = SET_NO_NEED;
	}

#endif	/*PHYDM_SPUR_CANCELL_ENABLE_8822B == 1*/
}

__iram_odm_func__
boolean
config_phydm_switch_band_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch
)
{
	u32		rf_reg18;
	boolean		rf_reg_status = true;

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_band_8822b()======================>\n"));

	if (p_dm_odm->is_disable_phy_api) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_band_8822b(): disable PHY API for debug!!\n"));
		return true;
	}

	rf_reg18 = config_phydm_read_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK);
	rf_reg_status = rf_reg_status & config_phydm_read_rf_check_8822b(rf_reg18);

	if (central_ch <= 14) {
		/* 2.4G */

		/* Enable CCK block */
		odm_set_bb_reg(p_dm_odm, 0x808, BIT(28), 0x1);

		/* Disable MAC CCK check */
		odm_set_bb_reg(p_dm_odm, 0x454, BIT(7), 0x0);

		/* Disable BB CCK check */
		odm_set_bb_reg(p_dm_odm, 0xa80, BIT(18), 0x0);

		/*CCA Mask*/
		odm_set_bb_reg(p_dm_odm, 0x814, 0x0000FC00, 15); /*default value*/

		/* RF band */
		rf_reg18 = (rf_reg18 & (~(BIT(16) | BIT(9) | BIT(8))));

		/* RxHP dynamic control */
		if ((p_dm_odm->rfe_type == 2) || (p_dm_odm->rfe_type == 3) || (p_dm_odm->rfe_type == 5)) {
			odm_set_bb_reg(p_dm_odm, 0x8cc, MASKDWORD, 0x08108492);
			odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(27), 0x1);
			}
		
	} else if (central_ch > 35) {
		/* 5G */

		/* Enable BB CCK check */
		odm_set_bb_reg(p_dm_odm, 0xa80, BIT(18), 0x1);

		/* Enable CCK check */
		odm_set_bb_reg(p_dm_odm, 0x454, BIT(7), 0x1);

		/* Disable CCK block */
		odm_set_bb_reg(p_dm_odm, 0x808, BIT(28), 0x0);

		/*CCA Mask*/
		if (!p_dm_odm->wifi_test)
			odm_set_bb_reg(p_dm_odm, 0x814, 0x0000FC00, 34); /*CCA mask = 13.6us*/
		else
			odm_set_bb_reg(p_dm_odm, 0x814, 0x0000FC00, 15); /*default value*/

		/* RF band */
		rf_reg18 = (rf_reg18 & (~(BIT(16) | BIT(9) | BIT(8))));
		rf_reg18 = (rf_reg18 | BIT(8) | BIT(16));

		/* RxHP dynamic control */
		if ((p_dm_odm->rfe_type == 2) || (p_dm_odm->rfe_type == 3) || (p_dm_odm->rfe_type == 5)) {
			odm_set_bb_reg(p_dm_odm, 0x8cc, MASKDWORD, 0x08100000);
			odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(27), 0x0);
			}
		
	} else {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_band_8822b(): Fail to switch band (ch: %d)\n", central_ch));
		return false;
	}

	rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (p_dm_odm->rf_type > ODM_1T1R)
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_B, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (phydm_rfe_8822b(p_dm_odm, central_ch) == false)
		return false;

	if (rf_reg_status == false) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_band_8822b(): Fail to switch band (ch: %d), because writing RF register is fail\n", central_ch));
		return false;
	}

	/* Dynamic spur detection by PSD and NBI/CSI mask */
	if (*(p_dm_odm->p_mp_mode))
		phydm_dynamic_spur_det_elimitor(p_dm_odm);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_band_8822b(): Success to switch band (ch: %d)\n", central_ch));
	return true;
}

__iram_odm_func__
boolean
config_phydm_switch_channel_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch
)
{
	struct _dynamic_initial_gain_threshold_		*p_dm_dig_table = &p_dm_odm->dm_dig_table;
	u32		rf_reg18 = 0, rf_reg_be = 0xff;
	boolean		rf_reg_status = true;
	u8		low_band[15] = {0x7, 0x6, 0x6, 0x5, 0x0, 0x0, 0x7, 0xff, 0x6, 0x5, 0x0, 0x0, 0x7, 0x6, 0x6};
	u8		middle_band[23] = {0x6, 0x5, 0x0, 0x0, 0x7, 0x6, 0x6, 0xff, 0x0, 0x0, 0x7, 0x6, 0x6, 0x5, 0x0, 0xff, 0x7, 0x6, 0x6, 0x5, 0x0, 0x0, 0x7};
	u8		high_band[15] = {0x5, 0x5, 0x0, 0x7, 0x7, 0x6, 0x5, 0xff, 0x0, 0x7, 0x7, 0x6, 0x5, 0x5, 0x0};
	u8		band_index = 0;

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b()====================>\n"));

	if (p_dm_odm->is_disable_phy_api) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): disable PHY API for debug!!\n"));
		return true;
	}

	central_ch_8822b = central_ch;

	/* Errir handling for wrong HW setting due to wrong channel setting */
	if (central_ch_8822b <= 14)
		band_index = 1;
	else
		band_index = 2;

	if (p_dm_odm->rfe_hwsetting_band != band_index)
		phydm_rfe_8822b(p_dm_odm, central_ch_8822b);

	/* RF register setting */
	rf_reg18 = config_phydm_read_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK);
	rf_reg_status = rf_reg_status & config_phydm_read_rf_check_8822b(rf_reg18);
	rf_reg18 = (rf_reg18 & (~(BIT(18) | BIT(17) | MASKBYTE0)));

	/* Switch band and channel */
	if (central_ch <= 14) {
		/* 2.4G */

		/* 1. RF band and channel*/
		rf_reg18 = (rf_reg18 | central_ch);

		/* 2. AGC table selection */
		odm_set_bb_reg(p_dm_odm, 0x958, 0x1f, 0x0);
		p_dm_dig_table->agc_table_idx = 0x0;

		/* 3. Set central frequency for clock offset tracking */
		odm_set_bb_reg(p_dm_odm, 0x860, 0x1ffe0000, 0x96a);

		/* CCK TX filter parameters */

		if (central_ch == 14) {
			odm_set_bb_reg(p_dm_odm, 0xa24, MASKDWORD, 0x00006577);
			odm_set_bb_reg(p_dm_odm, 0xa28, MASKLWORD, 0x0000);
		} else {
			odm_set_bb_reg(p_dm_odm, 0xa24, MASKDWORD, 0x384f6577);
			odm_set_bb_reg(p_dm_odm, 0xa28, MASKLWORD, 0x1525);
		}

	} else if (central_ch > 35) {
		/* 5G */

		/* 1. RF band and channel*/
		rf_reg18 = (rf_reg18 | central_ch);

		/* 2. AGC table selection */
		if ((central_ch >= 36) && (central_ch <= 64)) {
			odm_set_bb_reg(p_dm_odm, 0x958, 0x1f, 0x1);
			p_dm_dig_table->agc_table_idx = 0x1;
		} else if ((central_ch >= 100) && (central_ch <= 144)) {
			odm_set_bb_reg(p_dm_odm, 0x958, 0x1f, 0x2);
			p_dm_dig_table->agc_table_idx = 0x2;
		} else if (central_ch >= 149) {
			odm_set_bb_reg(p_dm_odm, 0x958, 0x1f, 0x3);
			p_dm_dig_table->agc_table_idx = 0x3;
		} else {
			ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Fail to switch channel (AGC) (ch: %d)\n", central_ch));
			return false;
		}

		/* 3. Set central frequency for clock offset tracking */
		if ((central_ch >= 36) && (central_ch <= 48))
			odm_set_bb_reg(p_dm_odm, 0x860, 0x1ffe0000, 0x494);
		else if ((central_ch >= 52) && (central_ch <= 64))
			odm_set_bb_reg(p_dm_odm, 0x860, 0x1ffe0000, 0x453);
		else if ((central_ch >= 100) && (central_ch <= 116))
			odm_set_bb_reg(p_dm_odm, 0x860, 0x1ffe0000, 0x452);
		else if ((central_ch >= 118) && (central_ch <= 177))
			odm_set_bb_reg(p_dm_odm, 0x860, 0x1ffe0000, 0x412);
		else {
			ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Fail to switch channel (fc_area) (ch: %d)\n", central_ch));
			return false;
		}

	} else {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Fail to switch channel (ch: %d)\n", central_ch));
		return false;
	}

	/* Modify IGI for MP driver to aviod PCIE interference */
	if ((*(p_dm_odm->p_mp_mode) == true) && ((p_dm_odm->rfe_type == 3) || (p_dm_odm->rfe_type == 5))) {
		if (central_ch == 14)
			odm_write_dig(p_dm_odm, 0x26);
		else
			odm_write_dig(p_dm_odm, 0x20);
	}

	/* Modify the setting of register 0xBE to reduce phase noise */
	if (central_ch <= 14)
		rf_reg_be = 0x0;
	else if ((central_ch >= 36) && (central_ch <= 64))
		rf_reg_be = low_band[(central_ch - 36) >> 1];
	else if ((central_ch >= 100) && (central_ch <= 144))
		rf_reg_be = middle_band[(central_ch - 100) >> 1];
	else if ((central_ch >= 149) && (central_ch <= 177))
		rf_reg_be = high_band[(central_ch - 149) >> 1];
	else
		rf_reg_be = 0xff;

	if (rf_reg_be != 0xff)
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xbe, (BIT(17) | BIT(16) | BIT(15)), rf_reg_be);
	else {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Fail to switch channel (ch: %d, Phase noise)\n", central_ch));
		return false;
	}

	/* Fix channel 144 issue, ask by RFSI Alvin*/
	/* 00 when freq < 5400;  01 when 5400<=freq<=5720; 10 when freq > 5720; 2G don't care*/
	/* need to set 0xdf[18]=1 before writing RF18 when channel 144 */
	if (central_ch == 144) {
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xdf, BIT(18), 0x1);
		rf_reg18 = (rf_reg18 | BIT(17));
	} else {
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xdf, BIT(18), 0x0);

		if (central_ch > 144)
			rf_reg18 = (rf_reg18 | BIT(18));
		else if (central_ch >= 80)
			rf_reg18 = (rf_reg18 | BIT(17));
	}

	rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (p_dm_odm->rf_type > ODM_1T1R)
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_B, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (rf_reg_status == false) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Fail to switch channel (ch: %d), because writing RF register is fail\n", central_ch));
		return false;
	}

	/* Debug for RF resister reading error during synthesizer parameters parsing */
	odm_set_rf_reg(p_dm_odm, ODM_RF_PATH_A, 0xb8, BIT(19), 0);
	odm_set_rf_reg(p_dm_odm, ODM_RF_PATH_A, 0xb8, BIT(19), 1);

	phydm_igi_toggle_8822b(p_dm_odm);
	/* Dynamic spur detection by PSD and NBI/CSI mask */
	if (*(p_dm_odm->p_mp_mode))
		phydm_dynamic_spur_det_elimitor(p_dm_odm);

	phydm_ccapar_by_rfe_8822b(p_dm_odm);
	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_channel_8822b(): Success to switch channel (ch: %d)\n", central_ch));
	return true;
}

__iram_odm_func__
boolean
config_phydm_switch_bandwidth_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					primary_ch_idx,
	enum odm_bw_e				bandwidth
)
{
	u32		rf_reg18;
	boolean		rf_reg_status = true;

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b()===================>\n"));

	if (p_dm_odm->is_disable_phy_api) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b(): disable PHY API for debug!!\n"));
		return true;
	}

	/* Error handling */
	if ((bandwidth >= ODM_BW_MAX) || ((bandwidth == ODM_BW40M) && (primary_ch_idx > 2)) || ((bandwidth == ODM_BW80M) && (primary_ch_idx > 4))) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b(): Fail to switch bandwidth (bw: %d, primary ch: %d)\n", bandwidth, primary_ch_idx));
		return false;
	}

	bw_8822b = bandwidth;
	rf_reg18 = config_phydm_read_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK);
	rf_reg_status = rf_reg_status & config_phydm_read_rf_check_8822b(rf_reg18);

	/* Switch bandwidth */
	switch (bandwidth) {
	case ODM_BW20M:
	{
		/* Small BW([7:6]) = 0, primary channel ([5:2]) = 0, rf mode([1:0]) = 20M */
		odm_set_bb_reg(p_dm_odm, 0x8ac, MASKBYTE0, ODM_BW20M);

		/* ADC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(9) | BIT(8)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(16), 0x1);

		/* DAC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(21) | BIT(20)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(28), 0x1);

		/* ADC buffer clock */
		odm_set_bb_reg(p_dm_odm, 0x8c4, BIT(30), 0x1);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	case ODM_BW40M:
	{
		/* Small BW([7:6]) = 0, primary channel ([5:2]) = sub-channel, rf mode([1:0]) = 40M */
		odm_set_bb_reg(p_dm_odm, 0x8ac, MASKBYTE0, (((primary_ch_idx & 0xf) << 2) | ODM_BW40M));

		/* CCK primary channel */
		if (primary_ch_idx == 1)
			odm_set_bb_reg(p_dm_odm, 0xa00, BIT(4), primary_ch_idx);
		else
			odm_set_bb_reg(p_dm_odm, 0xa00, BIT(4), 0);

		/* ADC clock = 160M clock for BW40 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(11) | BIT(10)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(17), 0x1);

		/* DAC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(23) | BIT(22)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(29), 0x1);

		/* ADC buffer clock */
		odm_set_bb_reg(p_dm_odm, 0x8c4, BIT(30), 0x1);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 & (~(BIT(11) | BIT(10))));
		rf_reg18 = (rf_reg18 | BIT(11));

		break;
	}
	case ODM_BW80M:
	{
		/* Small BW([7:6]) = 0, primary channel ([5:2]) = sub-channel, rf mode([1:0]) = 80M */
		odm_set_bb_reg(p_dm_odm, 0x8ac, MASKBYTE0, (((primary_ch_idx & 0xf) << 2) | ODM_BW80M));

		/* ADC clock = 160M clock for BW80 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(13) | BIT(12)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(18), 0x1);

		/* DAC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(25) | BIT(24)), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(30), 0x1);

		/* ADC buffer clock */
		odm_set_bb_reg(p_dm_odm, 0x8c4, BIT(30), 0x1);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 & (~(BIT(11) | BIT(10))));
		rf_reg18 = (rf_reg18 | BIT(10));

		break;
	}
	case ODM_BW5M:
	{
		/* Small BW([7:6]) = 1, primary channel ([5:2]) = 0, rf mode([1:0]) = 20M */
		odm_set_bb_reg(p_dm_odm, 0x8ac, MASKBYTE0, (BIT(6) | ODM_BW20M));

		/* ADC clock = 40M clock */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(9) | BIT(8)), 0x2);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(16), 0x0);

		/* DAC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(21) | BIT(20)), 0x2);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(28), 0x0);

		/* ADC buffer clock */
		odm_set_bb_reg(p_dm_odm, 0x8c4, BIT(30), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8c8, BIT(31), 0x1);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	case ODM_BW10M:
	{
		/* Small BW([7:6]) = 1, primary channel ([5:2]) = 0, rf mode([1:0]) = 20M */
		odm_set_bb_reg(p_dm_odm, 0x8ac, MASKBYTE0, (BIT(7) | ODM_BW20M));

		/* ADC clock = 80M clock */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(9) | BIT(8)), 0x3);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(16), 0x0);

		/* DAC clock = 160M clock for BW20 */
		odm_set_bb_reg(p_dm_odm, 0x8ac, (BIT(21) | BIT(20)), 0x3);
		odm_set_bb_reg(p_dm_odm, 0x8ac, BIT(28), 0x0);

		/* ADC buffer clock */
		odm_set_bb_reg(p_dm_odm, 0x8c4, BIT(30), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x8c8, BIT(31), 0x1);

		/* RF bandwidth */
		rf_reg18 = (rf_reg18 | BIT(11) | BIT(10));

		break;
	}
	default:
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b(): Fail to switch bandwidth (bw: %d, primary ch: %d)\n", bandwidth, primary_ch_idx));
	}

	/* Write RF register */
	rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (p_dm_odm->rf_type > ODM_1T1R)
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_B, 0x18, RFREGOFFSETMASK, rf_reg18);

	if (rf_reg_status == false) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b(): Fail to switch bandwidth (bw: %d, primary ch: %d), because writing RF register is fail\n", bandwidth, primary_ch_idx));
		return false;
	}

	/* Modify RX DFIR parameters */
	phydm_rxdfirpar_by_bw_8822b(p_dm_odm, bandwidth);

	/* Toggle IGI to let RF enter RX mode */
	phydm_igi_toggle_8822b(p_dm_odm);

	/* Dynamic spur detection by PSD and NBI/CSI mask */
	if (*(p_dm_odm->p_mp_mode))
		phydm_dynamic_spur_det_elimitor(p_dm_odm);

	/* Modify CCA parameters */
	phydm_ccapar_by_rfe_8822b(p_dm_odm);

	/* Toggle RX path to avoid RX dead zone issue */
	odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, 0x0);
	odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, (p_dm_odm->rx_ant_status | (p_dm_odm->rx_ant_status << 4)));

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_switch_bandwidth_8822b(): Success to switch bandwidth (bw: %d, primary ch: %d)\n", bandwidth, primary_ch_idx));
	return true;
}

__iram_odm_func__
boolean
config_phydm_switch_channel_bw_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	u8					central_ch,
	u8					primary_ch_idx,
	enum odm_bw_e				bandwidth
)
{

	/* Switch band */
	if (config_phydm_switch_band_8822b(p_dm_odm, central_ch) == false)
		return false;

	/* Switch channel */
	if (config_phydm_switch_channel_8822b(p_dm_odm, central_ch) == false)
		return false;

	/* Switch bandwidth */
	if (config_phydm_switch_bandwidth_8822b(p_dm_odm, primary_ch_idx, bandwidth) == false)
		return false;

	return true;
}

__iram_odm_func__
boolean
config_phydm_trx_mode_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_rf_path_e			tx_path,
	enum odm_rf_path_e			rx_path,
	boolean					is_tx2_path
)
{
	boolean		rf_reg_status = true;
	u32		rf_reg33 = 0;
	u16		counter = 0;
	/* struct PHY_DM_STRUCT*		p_dm_odm = (struct PHY_DM_STRUCT*)p_dm_void; */
	/* struct _ADAPTER*		p_adapter	= p_dm_odm->adapter; */
	/* PMGNT_INFO		p_mgnt_info = &(p_adapter->mgnt_info); */

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b()=====================>\n"));

	if (p_dm_odm->is_disable_phy_api) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): disable PHY API for debug!!\n"));
		return true;
	}

	if ((tx_path & (~(ODM_RF_A | ODM_RF_B))) != 0) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Wrong TX setting (TX: 0x%x)\n", tx_path));
		return false;
	}

	if ((rx_path & (~(ODM_RF_A | ODM_RF_B))) != 0) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Wrong RX setting (RX: 0x%x)\n", rx_path));
		return false;
	}

	/* RF mode of path-A and path-B */
	/* Cannot shut down path-A, beacause synthesizer will be shut down when path-A is in shut down mode */
	if ((tx_path | rx_path) & ODM_RF_A)
		odm_set_bb_reg(p_dm_odm, 0xc08, MASKLWORD, 0x3231);
	else
		odm_set_bb_reg(p_dm_odm, 0xc08, MASKLWORD, 0x1111);

	if ((tx_path | rx_path) & ODM_RF_B)
		odm_set_bb_reg(p_dm_odm, 0xe08, MASKLWORD, 0x3231);
	else
		odm_set_bb_reg(p_dm_odm, 0xe08, MASKLWORD, 0x1111);

	/* Set TX antenna by Nsts */
	odm_set_bb_reg(p_dm_odm, 0x93c, (BIT(19) | BIT(18)), 0x3);
	odm_set_bb_reg(p_dm_odm, 0x80c, (BIT(29) | BIT(28)), 0x1);

	/* Control CCK TX path by 0xa07[7] */
	odm_set_bb_reg(p_dm_odm, 0x80c, BIT(30), 0x1);

	/* TX logic map and TX path en for Nsts = 1, and CCK TX path*/
	if (tx_path & ODM_RF_A) {
		odm_set_bb_reg(p_dm_odm, 0x93c, 0xfff00000, 0x001);
		odm_set_bb_reg(p_dm_odm, 0xa04, 0xf0000000, 0x8);
	} else if (tx_path & ODM_RF_B) {
		odm_set_bb_reg(p_dm_odm, 0x93c, 0xfff00000, 0x002);
		odm_set_bb_reg(p_dm_odm, 0xa04, 0xf0000000, 0x4);
	}

	/* TX logic map and TX path en for Nsts = 2*/
	if ((tx_path == ODM_RF_A) || (tx_path == ODM_RF_B))
		odm_set_bb_reg(p_dm_odm, 0x940, 0xfff0, 0x01);
	else
		odm_set_bb_reg(p_dm_odm, 0x940, 0xfff0, 0x43);

	/* TX path enable */
	odm_set_bb_reg(p_dm_odm, 0x80c, MASKBYTE0, ((tx_path << 4) | tx_path));

	/* Tx2path for 1ss */
	if (!((tx_path == ODM_RF_A) || (tx_path == ODM_RF_B))) {
		if (is_tx2_path || *(p_dm_odm->p_mp_mode)) {
			/* 2Tx for OFDM */
			odm_set_bb_reg(p_dm_odm, 0x93c, 0xfff00000, 0x043);

			/* 2Tx for CCK */
			odm_set_bb_reg(p_dm_odm, 0xa04, 0xf0000000, 0xc);
		}
	}

	/* Always disable MRC for CCK CCA */
	odm_set_bb_reg(p_dm_odm, 0xa2c, BIT(22), 0x0);

	/* Always disable MRC for CCK barker */
	odm_set_bb_reg(p_dm_odm, 0xa2c, BIT(18), 0x0);

	/* CCK RX 1st and 2nd path setting*/
	if (rx_path & ODM_RF_A)
		odm_set_bb_reg(p_dm_odm, 0xa04, 0x0f000000, 0x0);
	else if (rx_path & ODM_RF_B)
		odm_set_bb_reg(p_dm_odm, 0xa04, 0x0f000000, 0x5);

	/* RX path enable */
	odm_set_bb_reg(p_dm_odm, 0x808, MASKBYTE0, ((rx_path << 4) | rx_path));

	if ((rx_path == ODM_RF_A) || (rx_path == ODM_RF_B)) {
		/* 1R */

		/* Disable MRC for CCA */
		/* odm_set_bb_reg(p_dm_odm, 0xa2c, BIT22, 0x0); */

		/* Disable MRC for barker */
		/* odm_set_bb_reg(p_dm_odm, 0xa2c, BIT18, 0x0); */

		/* Disable CCK antenna diversity */
		/* odm_set_bb_reg(p_dm_odm, 0xa00, BIT15, 0x0); */

		/* Disable Antenna weighting */
		odm_set_bb_reg(p_dm_odm, 0x1904, BIT(16), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x800, BIT(28), 0x0);
		odm_set_bb_reg(p_dm_odm, 0x850, BIT(23), 0x0);
	} else {
		/* 2R */

		/* Enable MRC for CCA */
		/* odm_set_bb_reg(p_dm_odm, 0xa2c, BIT22, 0x1); */

		/* Enable MRC for barker */
		/* odm_set_bb_reg(p_dm_odm, 0xa2c, BIT18, 0x1); */

		/* Disable CCK antenna diversity */
		/* odm_set_bb_reg(p_dm_odm, 0xa00, BIT15, 0x0); */

		/* Enable Antenna weighting */
		odm_set_bb_reg(p_dm_odm, 0x1904, BIT(16), 0x1);
		odm_set_bb_reg(p_dm_odm, 0x800, BIT(28), 0x1);
		odm_set_bb_reg(p_dm_odm, 0x850, BIT(23), 0x1);
	}

	/* Update TXRX antenna status for PHYDM */
	p_dm_odm->tx_ant_status = (tx_path & 0x3);
	p_dm_odm->rx_ant_status = (rx_path & 0x3);

	/* MP driver need to support path-B TX\RX */

	while (1) {
		counter++;
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x80000);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x33, RFREGOFFSETMASK, 0x00001);

		ODM_delay_us(2);
		rf_reg33 = config_phydm_read_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x33, RFREGOFFSETMASK);

		if ((rf_reg33 == 0x00001) && (config_phydm_read_rf_check_8822b(rf_reg33)))
			break;
		else if (counter == 100) {
			ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Fail to set TRx mode setting, because writing RF mode table is fail\n"));
			return false;
		}
	}

	if (*(p_dm_odm->p_mp_mode) || (*p_dm_odm->p_antenna_test) || (p_dm_odm->normal_rx_path)) {
		/*	0xef 0x80000  0x33 0x00001  0x3e 0x00034  0x3f 0x4080e  0xef 0x00000    suggested by Lucas*/
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x80000);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x33, RFREGOFFSETMASK, 0x00001);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x3e, RFREGOFFSETMASK, 0x00034);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x3f, RFREGOFFSETMASK, 0x4080e);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x00000);
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): MP mode or Antenna test mode!! support path-B TX and RX\n"));
	} else {
		/*	0xef 0x80000  0x33 0x00001  0x3e 0x00034  0x3f 0x4080c  0xef 0x00000 */
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x80000);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x33, RFREGOFFSETMASK, 0x00001);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x3e, RFREGOFFSETMASK, 0x00034);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0x3f, RFREGOFFSETMASK, 0x4080c);
		rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x00000);
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Normal mode!! Do not support path-B TX and RX\n"));
	}

	rf_reg_status = rf_reg_status & config_phydm_write_rf_reg_8822b(p_dm_odm, ODM_RF_PATH_A, 0xef, RFREGOFFSETMASK, 0x00000);

	if (rf_reg_status == false) {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Fail to set TRx mode setting (TX: 0x%x, RX: 0x%x), because writing RF register is fail\n", tx_path, rx_path));
		return false;
	}

	/* Toggle igi to let RF enter RX mode, because BB doesn't send 3-wire command when RX path is enable */
	phydm_igi_toggle_8822b(p_dm_odm);

	/* Modify CCA parameters */
	phydm_ccapar_by_rfe_8822b(p_dm_odm);
	phydm_rfe_8822b(p_dm_odm, central_ch_8822b);

	/* Dynamic spur detection by PSD and NBI/CSI mask */
	if (*(p_dm_odm->p_mp_mode))
		phydm_dynamic_spur_det_elimitor(p_dm_odm);

	ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("config_phydm_trx_mode_8822b(): Success to set TRx mode setting (TX: 0x%x, RX: 0x%x)\n", tx_path, rx_path));
	return true;
}

__iram_odm_func__
boolean
config_phydm_parameter_init_8822b(
	struct PHY_DM_STRUCT				*p_dm_odm,
	enum odm_parameter_init_e	type
)
{
	if (type == ODM_PRE_SETTING) {
		odm_set_bb_reg(p_dm_odm, 0x808, (BIT(28) | BIT(29)), 0x0);
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Pre setting: disable OFDM and CCK block\n", __func__));
	} else if (type == ODM_POST_SETTING) {
		odm_set_bb_reg(p_dm_odm, 0x808, (BIT(28) | BIT(29)), 0x3);
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Post setting: enable OFDM and CCK block\n", __func__));
#if (PHYDM_FW_API_FUNC_ENABLE_8822B == 1)
	} else if (type == ODM_INIT_FW_SETTING) {
		u8	h2c_content[4] = {0};
		
		h2c_content[0] = p_dm_odm->rfe_type;
		h2c_content[1] = p_dm_odm->rf_type;
		h2c_content[2] = p_dm_odm->cut_version;
		h2c_content[3] = (p_dm_odm->tx_ant_status << 4) | p_dm_odm->rx_ant_status;
		
		odm_fill_h2c_cmd(p_dm_odm, PHYDM_H2C_FW_GENERAL_INIT, 4, h2c_content);
#endif
	} else {
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_TRACE, ("%s: Wrong type!!\n", __func__));
		return false;
	}

	return true;
}

/* ======================================================================== */
#endif	/*PHYDM_FW_API_ENABLE_8822B == 1*/
#endif	/* RTL8822B_SUPPORT == 1 */
