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

#if 0
	/* ============================================================
	*  include files
	* ============================================================ */
#endif

#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8822B_SUPPORT == 1)


void
phydm_dynamic_switch_htstf_mumimo_8822b(
	struct PHY_DM_STRUCT		*p_dm_odm
)
{
	s32		sig_power;
	u32		value32;
	u8		pwdb;

	value32 = odm_get_bb_reg(p_dm_odm, 0xF90, MASKDWORD);
	pwdb = (u8)((value32 & MASKBYTE1) >> 8);
	pwdb = pwdb >> 1;
	sig_power = -110 + pwdb;

	/*if Pin > -60dBm, enable HT-STF gain controller, otherwise, if rssi < -65dBm, disable the controller*/

	if (sig_power >= -60)
		odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x1);
	else if (sig_power < -65)
		odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x0);

}

void
phydm_dynamic_parameters_ota(
	struct PHY_DM_STRUCT		*p_dm_odm
)
{
	s32    	sig_power;
	u32		value32, bktreg, bktreg1, bktreg2, bktreg3;
	u8    	pwdb;
	
	value32 = odm_get_bb_reg(p_dm_odm, 0xF90, MASKDWORD);
	pwdb = (u8)((value32 & MASKBYTE1) >> 8);
	pwdb = pwdb >> 1;
	sig_power = -110 + pwdb;

	bktreg = odm_get_bb_reg(p_dm_odm, 0x98c, MASKDWORD);
	bktreg1 = odm_get_bb_reg(p_dm_odm, 0x818, MASKDWORD);
	bktreg2 = odm_get_bb_reg(p_dm_odm, 0x98c, MASKDWORD);
	bktreg3 = odm_get_bb_reg(p_dm_odm, 0x818, MASKDWORD);

	if ((*p_dm_odm->p_channel <= 14) && (*p_dm_odm->p_band_width == ODM_BW20M)) {
		if (sig_power >= -60) {
			odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x1);
			odm_set_bb_reg(p_dm_odm, 0x98c, 0x7fc0000, 0x0);
			odm_set_bb_reg(p_dm_odm, 0x818, BIT(26), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xc04, BIT(18), 0x1);
			odm_set_bb_reg(p_dm_odm, 0xe04, BIT(18), 0x1);
			if (p_dm_odm->p_advance_ota & BIT(1)) {
				odm_set_bb_reg(p_dm_odm, 0x19d8, MASKDWORD, 0x444);
				odm_set_bb_reg(p_dm_odm, 0x19d4, MASKDWORD, 0x4444aaaa);
			} else if (p_dm_odm->p_advance_ota & BIT(2)) {
				odm_set_bb_reg(p_dm_odm, 0x19d8, MASKDWORD, 0x444);
				odm_set_bb_reg(p_dm_odm, 0x19d4, MASKDWORD, 0x444444aa);
			}
		} else if (sig_power < -65) {
			odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x0);
			odm_set_bb_reg(p_dm_odm, 0x98c, MASKDWORD, 0x43440000);
			odm_set_bb_reg(p_dm_odm, 0x818, BIT(26), 0x1);
			odm_set_bb_reg(p_dm_odm, 0xc04, BIT(18), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xe04, BIT(18), 0x0);
			odm_set_bb_reg(p_dm_odm, 0x19d8, MASKDWORD, 0xaaa);
			odm_set_bb_reg(p_dm_odm, 0x19d4, MASKDWORD, 0xaaaaaaaa);
		}
	} else {
			//odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x0);
			odm_set_bb_reg(p_dm_odm, 0x98c, MASKDWORD, 0x43440000);
			odm_set_bb_reg(p_dm_odm, 0x818, BIT(26), 0x1);
			odm_set_bb_reg(p_dm_odm, 0xc04, BIT(18), 0x0);
			odm_set_bb_reg(p_dm_odm, 0xe04, BIT(18), 0x0);
			odm_set_bb_reg(p_dm_odm, 0x19d8, MASKDWORD, 0xaaa);
			odm_set_bb_reg(p_dm_odm, 0x19d4, MASKDWORD, 0xaaaaaaaa);
		}
}


static
void
_setTxACaliValue(
	struct PHY_DM_STRUCT		*p_dm_odm,
	u8						eRFPath,
	u8 						offset,
	u8 						TxABiaOffset
	)
{
	u32 modiTxAValue = 0;
	u8 tmp1Byte = 0;
	boolean bMinus = false;
	u8 compValue = 0;

	
		switch (offset) {
		case 0x0:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X10124);
			break;
		case 0x1:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X10524);
			break;
		case 0x2:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X10924);
			break;
		case 0x3:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X10D24);
			break;
		case 0x4:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X30164);
			break;
		case 0x5:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X30564);
			break;
		case 0x6:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X30964);
			break;
		case 0x7:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X30D64);
			break;
		case 0x8:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X50195);
			break;
		case 0x9:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X50595);
			break;
		case 0xa:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X50995);
			break;
		case 0xb:
			odm_set_rf_reg(p_dm_odm, eRFPath, 0x18, 0xFFFFF, 0X50D95);
			break;
		default:
			ODM_RT_TRACE(p_dm_odm, ODM_COMP_COMMON, ODM_DBG_LOUD, ("Invalid TxA band offset...\n"));
			return;
			break;
	}

	/* Get TxA value */
	modiTxAValue = odm_get_rf_reg(p_dm_odm, eRFPath, 0x61, 0xFFFFF);
	tmp1Byte = (u8)modiTxAValue&(BIT(3)|BIT(2)|BIT(1)|BIT(0));

	/* check how much need to calibration */
		switch (TxABiaOffset) {
		case 0xF6:
			bMinus = true;
			compValue = 3;
			break;
			
		case 0xF4:
			bMinus = true;
			compValue = 2;
			break;
			
		case 0xF2:
			bMinus = true;
			compValue = 1;
			break;
			
		case 0xF3:
			bMinus = false;
			compValue = 1;
			break;
			
		case 0xF5:
			bMinus = false;
			compValue = 2;
			break;
			
		case 0xF7:
			bMinus = false;
			compValue = 3;
			break;
			
		case 0xF9:
			bMinus = false;
			compValue = 4;
			break;
		
		/* do nothing case */
		case 0xF0:
		default:
			ODM_RT_TRACE(p_dm_odm, ODM_COMP_COMMON, ODM_DBG_LOUD, ("No need to do TxA bias current calibration\n"));
			return;
			break;
	}

	/* calc correct value to calibrate */
	if (bMinus) {
		if (tmp1Byte >= compValue) {
			tmp1Byte -= compValue;
			//modiTxAValue += tmp1Byte;
		} else {
			tmp1Byte = 0;
		}
	} else {
		tmp1Byte += compValue;
		if (tmp1Byte >= 7) {
			tmp1Byte = 7;
		}
	}

	/* Write back to RF reg */
	odm_set_rf_reg(p_dm_odm, eRFPath, 0x30, 0xFFFF, (offset<<12|(modiTxAValue&0xFF0)|tmp1Byte));
}

static
void
_txaBiasCali4eachPath(
	struct PHY_DM_STRUCT		*p_dm_odm,
	u8	 eRFPath,
	u8	 efuseValue
	)
{
	/* switch on set TxA bias */
	odm_set_rf_reg(p_dm_odm, eRFPath, 0xEF, 0xFFFFF, 0x200);

	/* Set 12 sets of TxA value */
	_setTxACaliValue(p_dm_odm, eRFPath, 0x0, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x1, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x2, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x3, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x4, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x5, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x6, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x7, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x8, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0x9, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0xa, efuseValue);
	_setTxACaliValue(p_dm_odm, eRFPath, 0xb, efuseValue);

	// switch off set TxA bias
	odm_set_rf_reg(p_dm_odm, eRFPath, 0xEF, 0xFFFFF, 0x0);
}

/* for 8822B PCIE D-cut patch only */
/* Normal driver and MP driver need this patch */

void
phydm_txcurrentcalibration(
	struct PHY_DM_STRUCT		*p_dm_odm
	)
{
	u8			efuse0x3D8, efuse0x3D7;
	u32			origRF0x18PathA = 0, origRF0x18PathB = 0;

	if (!(p_dm_odm->support_ic_type & ODM_RTL8822B))
		return;

	ODM_RT_TRACE(p_dm_odm, ODM_COMP_MP, ODM_DBG_LOUD, ("8822b 5g tx current calibration 0x3d7=0x%X 0x3d8=0x%X\n", p_dm_odm->efuse0x3d7, p_dm_odm->efuse0x3d8));

	/* save original 0x18 value */
	origRF0x18PathA = odm_get_rf_reg(p_dm_odm, ODM_RF_PATH_A, 0x18, 0xFFFFF);
	origRF0x18PathB = odm_get_rf_reg(p_dm_odm, ODM_RF_PATH_B, 0x18, 0xFFFFF);
	
	/* define efuse content */
		efuse0x3D8 = p_dm_odm->efuse0x3d8;
		efuse0x3D7 = p_dm_odm->efuse0x3d7;
	
	/* check efuse content to judge whether need to calibration or not */
	if (0xFF == efuse0x3D7) {
		ODM_RT_TRACE(p_dm_odm, ODM_COMP_MP, ODM_DBG_LOUD, ("efuse content 0x3D7 == 0xFF, No need to do TxA cali\n"));
		return;
	}

	/* write RF register for calibration */
	_txaBiasCali4eachPath(p_dm_odm, ODM_RF_PATH_A, efuse0x3D7);
	_txaBiasCali4eachPath(p_dm_odm, ODM_RF_PATH_B, efuse0x3D8);
	
	/* restore original 0x18 value */
	odm_set_rf_reg(p_dm_odm, ODM_RF_PATH_A, 0x18, 0xFFFFF, origRF0x18PathA);
	odm_set_rf_reg(p_dm_odm, ODM_RF_PATH_B, 0x18, 0xFFFFF, origRF0x18PathB);
}

void
phydm_hwsetting_8822b(
	struct PHY_DM_STRUCT		*p_dm_odm
)
{

	if((p_dm_odm->p_advance_ota & BIT(1)) || (p_dm_odm->p_advance_ota & BIT(2))) {
		phydm_dynamic_parameters_ota(p_dm_odm);
	} else {
	if (p_dm_odm->bhtstfenabled == TRUE)
		phydm_dynamic_switch_htstf_mumimo_8822b(p_dm_odm);
	else
		/* odm_set_bb_reg(p_dm_odm, 0x8d8, BIT(17), 0x1);*/
		ODM_RT_TRACE(p_dm_odm, ODM_PHY_CONFIG, ODM_DBG_LOUD, ("Default HT-STF gain control setting\n"));
        }
}

#endif	/* RTL8822B_SUPPORT == 1 */
