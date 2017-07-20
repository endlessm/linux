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


#ifndef	_HALRF_H__
#define _HALRF_H__

/*============================================================*/
/*include files*/
/*============================================================*/



/*============================================================*/
/*Definition */
/*============================================================*/


#if 0/*(RTL8821C_SUPPORT == 1)*/
#define HALRF_IQK_VER	IQK_VERSION
#define HALRF_LCK_VER	LCK_VERSION
#define HALRF_DPK_VER	DPK_VERSION
#else
#define HALRF_IQK_VER	"1.0"
#define HALRF_LCK_VER	"1.0"
#define HALRF_DPK_VER	"1.0"
#endif

/*============================================================*/
/* enumeration */
/*============================================================*/
enum halrf_ability_e {

	HAL_RF_TX_PWR_TRACK	= BIT(0),
	HAL_RF_IQK				= BIT(1),
	HAL_RF_LCK				= BIT(2),
	HAL_RF_DPK				= BIT(3)
};

enum halrf_cmninfo_e {
	
	HALRF_CMNINFO_ABILITY = 0,
	HALRF_CMNINFO_DPK_EN = 1,
	HALRF_CMNINFO_tmp
};

/*============================================================*/
/* structure */
/*============================================================*/

struct _hal_rf_ {
	u32		rf_supportability;
	u8		dpk_en;			/*Enable Function DPK OFF/ON = 0/1*/
	boolean	dpk_done;

};

/*============================================================*/
/* function prototype */
/*============================================================*/

void phydm_rf_basic_profile(
	void			*p_dm_void,
	u32			*_used,
	char			*output,
	u32			*_out_len
);

void
halrf_support_ability_debug(
	void		*p_dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len
);

void
halrf_cmn_info_set(
	void		*p_dm_void,
	u32			cmn_info,
	u64			value
);

u64
halrf_cmn_info_get(
	void		*p_dm_void,
	u32			cmn_info
);

void
halrf_watchdog(
	void			*p_dm_void
);

void
halrf_supportability_init(
	void		*p_dm_void
);

void
halrf_init(
	void			*p_dm_void
);



#endif
