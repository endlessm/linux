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

/* ************************************************************
 * include files
 * ************************************************************ */

#include "mp_precomp.h"
#include "phydm_precomp.h"

void phydm_rf_basic_profile(
	void			*p_dm_void,
	u32			*_used,
	char			*output,
	u32			*_out_len
)
{
#if CONFIG_PHYDM_DEBUG_FUNCTION
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	u32 used = *_used;
	u32 out_len = *_out_len;

	/* HAL RF version List */
	PHYDM_SNPRINTF((output + used, out_len - used, "%-35s\n", "% HAL RF version %"));
	PHYDM_SNPRINTF((output + used, out_len - used, "  %-35s: %s\n", "Power Tracking", HALRF_POWRTRACKING_VER));
	PHYDM_SNPRINTF((output + used, out_len - used, "  %-35s: %s\n", "IQK", HALRF_IQK_VER));
	PHYDM_SNPRINTF((output + used, out_len - used, "  %-35s: %s\n", "LCK", HALRF_LCK_VER));
	PHYDM_SNPRINTF((output + used, out_len - used, "  %-35s: %s\n", "DPK", HALRF_DPK_VER));

	*_used = used;
	*_out_len = out_len;
#endif /*#if CONFIG_PHYDM_DEBUG_FUNCTION*/
}

void
halrf_support_ability_debug(
	void		*p_dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len
)
{
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm_odm->rf_table);
	u32	dm_value[10] = {0};
	u32 used = *_used;
	u32 out_len = *_out_len;
	u8	i;

	for (i = 0; i < 5; i++) {
		if (input[i + 1]) {
			PHYDM_SSCANF(input[i + 1], DCMD_DECIMAL, &dm_value[i]);
		}
	}
	
	PHYDM_SNPRINTF((output + used, out_len - used, "\n%s\n", "================================"));
	if (dm_value[0] == 100) {
		PHYDM_SNPRINTF((output + used, out_len - used, "[RF Supportability]\n"));
		PHYDM_SNPRINTF((output + used, out_len - used, "%s\n", "================================"));
		PHYDM_SNPRINTF((output + used, out_len - used, "00. (( %s ))Power Tracking\n", ((p_rf->rf_supportability & HAL_RF_TX_PWR_TRACK) ? ("V") : ("."))));
		PHYDM_SNPRINTF((output + used, out_len - used, "01. (( %s ))IQK\n", ((p_rf->rf_supportability & HAL_RF_IQK) ? ("V") : ("."))));
		PHYDM_SNPRINTF((output + used, out_len - used, "02. (( %s ))LCK\n", ((p_rf->rf_supportability & HAL_RF_LCK) ? ("V") : ("."))));
		PHYDM_SNPRINTF((output + used, out_len - used, "03. (( %s ))DPK\n", ((p_rf->rf_supportability & HAL_RF_DPK) ? ("V") : ("."))));
		PHYDM_SNPRINTF((output + used, out_len - used, "%s\n", "================================"));		
	}
	else {

		if (dm_value[1] == 1) { /* enable */
			p_rf->rf_supportability |= BIT(dm_value[0]) ;
		} else if (dm_value[1] == 2) /* disable */
			p_rf->rf_supportability &= ~(BIT(dm_value[0])) ;
		else {
			PHYDM_SNPRINTF((output + used, out_len - used, "%s\n", "[Warning!!!]  1:enable,  2:disable"));
		}
	}
	PHYDM_SNPRINTF((output + used, out_len - used, "Curr-RF_supportability =  0x%x\n", p_rf->rf_supportability));
	PHYDM_SNPRINTF((output + used, out_len - used, "%s\n", "================================"));
}

void
halrf_cmn_info_set(
	void		*p_dm_void,
	u32			cmn_info,
	u64			value
)
{
	/*  */
	/* This init variable may be changed in run time. */
	/*  */
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm_odm->rf_table);
	
	switch	(cmn_info) {

		case	HALRF_CMNINFO_ABILITY:
			p_rf->rf_supportability = (u32)value;
			break;

		case	ODM_CMNINFO_DPK_EN:
			p_rf->dpk_en = (u1Byte)value;
			break;
			
		default:
			/* do nothing */
			break;
	}
}

u64
halrf_cmn_info_get(
	void		*p_dm_void,
	u32			cmn_info
)
{
	/*  */
	/* This init variable may be changed in run time. */
	/*  */
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm_odm->rf_table);
	u64	return_value = 0;
	
	switch	(cmn_info) {

		case	HALRF_CMNINFO_ABILITY:
			return_value = (u32)p_rf->rf_supportability;
			break;
		default:
			/* do nothing */
			break;
	}

	return	return_value;
}

void
halrf_supportability_init_mp(
	void		*p_dm_void
)
{
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm_odm->rf_table);

	switch (p_dm_odm->support_ic_type) {

	case ODM_RTL8814B:
		#if (RTL8814B_SUPPORT == 1) 
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		#endif
		break;
	#if (RTL8822B_SUPPORT == 1) 
	case ODM_RTL8822B:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;
	#endif

	#if (RTL8821C_SUPPORT == 1) 
	case ODM_RTL8821C:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;
	#endif

	default:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;

	}

	ODM_RT_TRACE(p_dm_odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("IC = ((0x%x)), RF_Supportability Init MP = ((0x%x))\n", p_dm_odm->support_ic_type, p_rf->rf_supportability));
}

void
halrf_supportability_init(
	void		*p_dm_void
)
{
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	struct _hal_rf_				*p_rf = &(p_dm_odm->rf_table);

	switch (p_dm_odm->support_ic_type) {

	case ODM_RTL8814B:
		#if (RTL8814B_SUPPORT == 1) 
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		#endif
		break;
	#if (RTL8822B_SUPPORT == 1) 
	case ODM_RTL8822B:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;
	#endif

	#if (RTL8821C_SUPPORT == 1) 
	case ODM_RTL8821C:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;
	#endif

	default:
		p_rf->rf_supportability = 
			HAL_RF_TX_PWR_TRACK	|
			HAL_RF_IQK				|
			HAL_RF_LCK				|
			/*HAL_RF_DPK				|*/
			0;
		break;

	}

	ODM_RT_TRACE(p_dm_odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("IC = ((0x%x)), RF_Supportability Init = ((0x%x))\n", p_dm_odm->support_ic_type, p_rf->rf_supportability));
}

void
halrf_watchdog(
	void			*p_dm_void
)
{
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	phydm_rf_watchdog(p_dm_odm);
}

void
halrf_init(
	void			*p_dm_void
)
{
	struct PHY_DM_STRUCT		*p_dm_odm = (struct PHY_DM_STRUCT *)p_dm_void;
	
	ODM_RT_TRACE(p_dm_odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("HALRF_Init\n"));

	if (*(p_dm_odm->p_mp_mode) == true)
		halrf_supportability_init_mp(p_dm_odm);
	else
		halrf_supportability_init(p_dm_odm);
}




