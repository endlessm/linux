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


#ifndef	__HALDMOUTSRC_H__
#define __HALDMOUTSRC_H__


#include "odm_EdcaTurboCheck.h"
#include "odm_DIG.h"
#include "odm_PathDiv.h"
#include "odm_RaInfo.h"
#include "odm_DynamicBBPowerSaving.h"
#include "odm_DynamicTxPower.h"
#include "odm_CfoTracking.h"
#include "odm_NoiseMonitor.h"

//============================================================
// Definition
//============================================================
//
// 2011/09/22 MH Define all team supprt ability.
//

//
// 2011/09/22 MH Define for all teams. Please Define the constan in your precomp header.
//
//#define		DM_ODM_SUPPORT_AP			0
//#define		DM_ODM_SUPPORT_ADSL			0
//#define		DM_ODM_SUPPORT_CE			0
//#define		DM_ODM_SUPPORT_MP			1

//
// 2011/09/28 MH Define ODM SW team support flag.
//



//
// Antenna Switch Relative Definition.
//

//
// 20100503 Joseph:
// Add new function SwAntDivCheck8192C().
// This is the main function of Antenna diversity function before link.
// Mainly, it just retains last scan result and scan again.
// After that, it compares the scan result to see which one gets better RSSI.
// It selects antenna with better receiving power and returns better scan result.
//
#define	TP_MODE		0
#define	RSSI_MODE		1
#define	TRAFFIC_LOW	0
#define	TRAFFIC_HIGH	1


//============================================================
//3 Tx Power Tracking
//3============================================================
#define		DPK_DELTA_MAPPING_NUM	13
#define		index_mapping_HP_NUM	15
#define	OFDM_TABLE_SIZE		43
#define	CCK_TABLE_SIZE			33
#define TXSCALE_TABLE_SIZE		37
#define TXPWR_TRACK_TABLE_SIZE	30
#define DELTA_SWINGIDX_SIZE     30
#define BAND_NUM				4

//============================================================
//3 PSD Handler
//3============================================================

#define	AFH_PSD		1	//0:normal PSD scan, 1: only do 20 pts PSD
#define	MODE_40M		0	//0:20M, 1:40M
#define	PSD_TH2		3
#define	PSD_CHMIN		20   // Minimum channel number for BT AFH
#define	SIR_STEP_SIZE	3
#define   Smooth_Size_1		5
#define	Smooth_TH_1	3
#define   Smooth_Size_2		10
#define	Smooth_TH_2	4
#define   Smooth_Size_3		20
#define	Smooth_TH_3	4
#define   Smooth_Step_Size 5
#define	Adaptive_SIR	1
#if(RTL8723_FPGA_VERIFICATION == 1)
#define	PSD_RESCAN		1
#else
#define	PSD_RESCAN		4
#endif
#define	PSD_SCAN_INTERVAL	700 //ms



//8723A High Power IGI Setting
#define		DM_DIG_HIGH_PWR_IGI_LOWER_BOUND	0x22
#define			DM_DIG_Gmode_HIGH_PWR_IGI_LOWER_BOUND 0x28
#define		DM_DIG_HIGH_PWR_THRESHOLD	0x3a
#define		DM_DIG_LOW_PWR_THRESHOLD	0x14

//ANT Test
#define			ANTTESTALL		0x00		//Ant A or B will be Testing
#define		ANTTESTA		0x01		//Ant A will be Testing
#define		ANTTESTB		0x02		//Ant B will be testing

//for 8723A Ant Definition--2012--06--07 due to different IC may be different ANT define
#define		MAIN_ANT		1		//Ant A or Ant Main
#define		AUX_ANT		2		//AntB or Ant Aux
#define		MAX_ANT		3		// 3 for AP using


//Antenna Diversity Type
#define	SW_ANTDIV	0
#define	HW_ANTDIV	1
//============================================================
// structure and define
//============================================================

//
// 2011/09/20 MH Add for AP/ADSLpseudo DM structuer requirement.
// We need to remove to other position???
//
typedef		struct rtl8192cd_priv {
	u1Byte		temp;

}rtl8192cd_priv, *prtl8192cd_priv;

//Remove DIG by Yuchen

//Remoce BB power saving by Yuchn

//Remove DIG by yuchen

typedef struct _Dynamic_Primary_CCA{
	u1Byte		PriCCA_flag;
	u1Byte		intf_flag;
	u1Byte		intf_type;
	u1Byte		DupRTS_flag;
	u1Byte		Monitor_flag;
	u1Byte		CH_offset;
	u1Byte			MF_state;
}Pri_CCA_T, *pPri_CCA_T;

//Remove RA_T,*pRA_T by RS_James

typedef struct _RX_High_Power_
{
	u1Byte		RXHP_flag;
	u1Byte		PSD_func_trigger;
	u1Byte		PSD_bitmap_RXHP[80];
	u1Byte		Pre_IGI;
	u1Byte		Cur_IGI;
	u1Byte		Pre_pw_th;
	u1Byte		Cur_pw_th;
	BOOLEAN		First_time_enter;
	BOOLEAN		RXHP_enable;
	u1Byte		TP_Mode;
	RT_TIMER	PSDTimer;
}RXHP_T, *pRXHP_T;

#define ASSOCIATE_ENTRY_NUM					32 // Max size of AsocEntry[].
#define	ODM_ASSOCIATE_ENTRY_NUM				ASSOCIATE_ENTRY_NUM

// This indicates two different the steps.
// In SWAW_STEP_PEAK, driver needs to switch antenna and listen to the signal on the air.
// In SWAW_STEP_DETERMINE, driver just compares the signal captured in SWAW_STEP_PEAK
// with original RSSI to determine if it is necessary to switch antenna.
#define SWAW_STEP_PEAK		0
#define SWAW_STEP_DETERMINE	1

#define	TP_MODE		0
#define	RSSI_MODE		1
#define	TRAFFIC_LOW	0
#define	TRAFFIC_HIGH	1
#define	TRAFFIC_UltraLOW	2

typedef struct _SW_Antenna_Switch_
{
	u1Byte		Double_chk_flag;
	u1Byte		try_flag;
	s4Byte		PreRSSI;
	u1Byte		CurAntenna;
	u1Byte		PreAntenna;
	u1Byte		RSSI_Trying;
	u1Byte		TestMode;
	u1Byte		bTriggerAntennaSwitch;
	u1Byte		SelectAntennaMap;
	u1Byte		RSSI_target;
	u1Byte		reset_idx;

	// Before link Antenna Switch check
	u1Byte		SWAS_NoLink_State;
	u4Byte		SWAS_NoLink_BK_Reg860;
	u4Byte		SWAS_NoLink_BK_Reg92c;
	BOOLEAN		ANTA_ON;	//To indicate Ant A is or not
	BOOLEAN		ANTB_ON;	//To indicate Ant B is on or not
	u1Byte		Ant5G;
	u1Byte		Ant2G;

	s4Byte		RSSI_sum_A;
	s4Byte		RSSI_sum_B;
	s4Byte		RSSI_cnt_A;
	s4Byte		RSSI_cnt_B;

	u8Byte		lastTxOkCnt;
	u8Byte		lastRxOkCnt;
	u8Byte		TXByteCnt_A;
	u8Byte		TXByteCnt_B;
	u8Byte		RXByteCnt_A;
	u8Byte		RXByteCnt_B;
	u1Byte		TrafficLoad;
	u1Byte		Train_time;
	u1Byte		Train_time_flag;
	RT_TIMER	SwAntennaSwitchTimer;
	RT_TIMER	SwAntennaSwitchTimer_8723B;
#ifdef CONFIG_HW_ANTENNA_DIVERSITY
	//Hybrid Antenna Diversity
	u4Byte		CCK_Ant1_Cnt[ASSOCIATE_ENTRY_NUM+1];
	u4Byte		CCK_Ant2_Cnt[ASSOCIATE_ENTRY_NUM+1];
	u4Byte		OFDM_Ant1_Cnt[ASSOCIATE_ENTRY_NUM+1];
	u4Byte		OFDM_Ant2_Cnt[ASSOCIATE_ENTRY_NUM+1];
	u4Byte		RSSI_Ant1_Sum[ASSOCIATE_ENTRY_NUM+1];
	u4Byte		RSSI_Ant2_Sum[ASSOCIATE_ENTRY_NUM+1];
	u1Byte		TxAnt[ASSOCIATE_ENTRY_NUM+1];
	u1Byte		TargetSTA;
	u1Byte		antsel;
	u1Byte		RxIdleAnt;

#endif

}SWAT_T, *pSWAT_T;

//Remove Edca by YuChen

//Remove ODM_RATE_ADAPTIVE by RS_James

#define IQK_MAC_REG_NUM		4
#define IQK_ADDA_REG_NUM		16
#define IQK_BB_REG_NUM_MAX	10
#define IQK_BB_REG_NUM		9
#define HP_THERMAL_NUM		8

#define AVG_THERMAL_NUM		8
#define IQK_Matrix_REG_NUM	8
#define IQK_Matrix_Settings_NUM	14+24+21 // Channels_2_4G_NUM + Channels_5G_20M_NUM + Channels_5G

#define		DM_Type_ByFW			0
#define		DM_Type_ByDriver		1

//
// Declare for common info
//
#define MAX_PATH_NUM_92CS		2
#define MAX_PATH_NUM_8188E		1
#define MAX_PATH_NUM_8192E		2
#define MAX_PATH_NUM_8723B		1
#define MAX_PATH_NUM_8812A		2
#define MAX_PATH_NUM_8821A		1

#define IQK_THRESHOLD			8

typedef struct _ODM_Phy_Status_Info_
{
	//
	// Be care, if you want to add any element please insert between
	// RxPWDBAll & SignalStrength.
	//
	u1Byte		RxPWDBAll;

	u1Byte		SignalQuality;			// in 0-100 index.
	s1Byte		RxMIMOSignalQuality[4];	//per-path's EVM
	u1Byte		RxMIMOEVMdbm[4];		//per-path's EVM dbm

	u1Byte		RxMIMOSignalStrength[4];// in 0~100 index

	u2Byte		Cfo_short[4];			// per-path's Cfo_short
	u2Byte		Cfo_tail[4];			// per-path's Cfo_tail

	s1Byte		RxPower;				// in dBm Translate from PWdB
	s1Byte		RecvSignalPower;		// Real power in dBm for this packet, no beautification and aggregation. Keep this raw info to be used for the other procedures.
	u1Byte		BTRxRSSIPercentage;
	u1Byte		SignalStrength;			// in 0-100 index.

	s1Byte		RxPwr[4];				//per-path's pwdb
	u1Byte		RxSNR[4];				//per-path's SNR
	u1Byte		BandWidth;
	u1Byte		btCoexPwrAdjust;
}ODM_PHY_INFO_T,*PODM_PHY_INFO_T;


typedef struct _ODM_Per_Pkt_Info_
{
	//u1Byte		Rate;
	u1Byte		DataRate;
	u1Byte		StationID;
	BOOLEAN		bPacketMatchBSSID;
	BOOLEAN		bPacketToSelf;
	BOOLEAN		bPacketBeacon;
}ODM_PACKET_INFO_T,*PODM_PACKET_INFO_T;


typedef struct _ODM_Phy_Dbg_Info_
{
	//ODM Write,debug info
	s1Byte		RxSNRdB[4];
	u4Byte		NumQryPhyStatus;
	u4Byte		NumQryPhyStatusCCK;
	u4Byte		NumQryPhyStatusOFDM;
	u1Byte		NumQryBeaconPkt;
	//Others
	s4Byte		RxEVM[4];

}ODM_PHY_DBG_INFO_T;


typedef struct _ODM_Mac_Status_Info_
{
	u1Byte	test;

}ODM_MAC_INFO;


typedef enum tag_Dynamic_ODM_Support_Ability_Type
{
	// BB Team
	ODM_DIG				= 0x00000001,
	ODM_HIGH_POWER		= 0x00000002,
	ODM_CCK_CCA_TH		= 0x00000004,
	ODM_FA_STATISTICS		= 0x00000008,
	ODM_RAMASK			= 0x00000010,
	ODM_RSSI_MONITOR		= 0x00000020,
	ODM_SW_ANTDIV		= 0x00000040,
	ODM_HW_ANTDIV		= 0x00000080,
	ODM_BB_PWRSV			= 0x00000100,
	ODM_2TPATHDIV			= 0x00000200,
	ODM_1TPATHDIV			= 0x00000400,
	ODM_PSD2AFH			= 0x00000800
}ODM_Ability_E;

//
// 2011/20/20 MH For MP driver RT_WLAN_STA =  STA_INFO_T
// Please declare below ODM relative info in your STA info structure.
//
#if 1
typedef		struct _ODM_STA_INFO{
	// Driver Write
	BOOLEAN		bUsed;				// record the sta status link or not?
	//u1Byte		WirelessMode;		//
	u1Byte		IOTPeer;			// Enum value.	HT_IOT_PEER_E

	// ODM Write
	//1 PHY_STATUS_INFO
	u1Byte		RSSI_Path[4];		//
	u1Byte		RSSI_Ave;
	u1Byte		RXEVM[4];
	u1Byte		RXSNR[4];

	// ODM Write
	//1 TX_INFO (may changed by IC)
	//TX_INFO_T		pTxInfo;				// Define in IC folder. Move lower layer.

	//
	//	Please use compile flag to disabe the strcutrue for other IC except 88E.
	//	Move To lower layer.
	//
	// ODM Write Wilson will handle this part(said by Luke.Lee)
	//TX_RPT_T		pTxRpt;				// Define in IC folder. Move lower layer.

}ODM_STA_INFO_T, *PODM_STA_INFO_T;
#endif

//
// 2011/10/20 MH Define Common info enum for all team.
//
typedef enum _ODM_Common_Info_Definition
{
//-------------REMOVED CASE-----------//
	//ODM_CMNINFO_CCK_HP,
	//ODM_CMNINFO_RFPATH_ENABLE,		// Define as ODM write???
	//ODM_CMNINFO_BT_COEXIST,				// ODM_BT_COEXIST_E
	//ODM_CMNINFO_OP_MODE,				// ODM_OPERATION_MODE_E
//-------------REMOVED CASE-----------//

	//
	// Fixed value:
	//

	//-----------HOOK BEFORE REG INIT-----------//
	ODM_CMNINFO_PLATFORM = 0,
	ODM_CMNINFO_ABILITY,					// ODM_ABILITY_E
	ODM_CMNINFO_INTERFACE,				// ODM_INTERFACE_E
	ODM_CMNINFO_MP_TEST_CHIP,
	ODM_CMNINFO_IC_TYPE,					// ODM_IC_TYPE_E
	ODM_CMNINFO_CUT_VER,					// ODM_CUT_VERSION_E
	ODM_CMNINFO_FAB_VER,					// ODM_FAB_E
	ODM_CMNINFO_RF_TYPE,					// ODM_RF_PATH_E or ODM_RF_TYPE_E?
	ODM_CMNINFO_RFE_TYPE,
	ODM_CMNINFO_BOARD_TYPE,				// ODM_BOARD_TYPE_E
	ODM_CMNINFO_PACKAGE_TYPE,
	ODM_CMNINFO_EXT_LNA,					// TRUE
	ODM_CMNINFO_5G_EXT_LNA,
	ODM_CMNINFO_EXT_PA,
	ODM_CMNINFO_5G_EXT_PA,
	ODM_CMNINFO_GPA,
	ODM_CMNINFO_APA,
	ODM_CMNINFO_GLNA,
	ODM_CMNINFO_ALNA,
	ODM_CMNINFO_EXT_TRSW,
	ODM_CMNINFO_PATCH_ID,				//CUSTOMER ID
	ODM_CMNINFO_BINHCT_TEST,
	ODM_CMNINFO_BWIFI_TEST,
	ODM_CMNINFO_SMART_CONCURRENT,
	ODM_CMNINFO_DOMAIN_CODE_2G,
	ODM_CMNINFO_DOMAIN_CODE_5G,
	//-----------HOOK BEFORE REG INIT-----------//


	//
	// Dynamic value:
	//
//--------- POINTER REFERENCE-----------//
	ODM_CMNINFO_MAC_PHY_MODE,			// ODM_MAC_PHY_MODE_E
	ODM_CMNINFO_TX_UNI,
	ODM_CMNINFO_RX_UNI,
	ODM_CMNINFO_WM_MODE,				// ODM_WIRELESS_MODE_E
	ODM_CMNINFO_BAND,					// ODM_BAND_TYPE_E
	ODM_CMNINFO_SEC_CHNL_OFFSET,		// ODM_SEC_CHNL_OFFSET_E
	ODM_CMNINFO_SEC_MODE,				// ODM_SECURITY_E
	ODM_CMNINFO_BW,						// ODM_BW_E
	ODM_CMNINFO_CHNL,
	ODM_CMNINFO_FORCED_RATE,

	ODM_CMNINFO_DMSP_GET_VALUE,
	ODM_CMNINFO_BUDDY_ADAPTOR,
	ODM_CMNINFO_DMSP_IS_MASTER,
	ODM_CMNINFO_SCAN,
	ODM_CMNINFO_POWER_SAVING,
	ODM_CMNINFO_ONE_PATH_CCA,			// ODM_CCA_PATH_E
	ODM_CMNINFO_DRV_STOP,
	ODM_CMNINFO_PNP_IN,
	ODM_CMNINFO_INIT_ON,
	ODM_CMNINFO_ANT_TEST,
	ODM_CMNINFO_NET_CLOSED,
	ODM_CMNINFO_MP_MODE,
	//ODM_CMNINFO_RTSTA_AID,				// For win driver only?
	ODM_CMNINFO_FORCED_IGI_LB,
//--------- POINTER REFERENCE-----------//

//------------CALL BY VALUE-------------//
	ODM_CMNINFO_WIFI_DIRECT,
	ODM_CMNINFO_WIFI_DISPLAY,
	ODM_CMNINFO_LINK_IN_PROGRESS,
	ODM_CMNINFO_LINK,
	ODM_CMNINFO_STATION_STATE,
	ODM_CMNINFO_RSSI_MIN,
	ODM_CMNINFO_DBG_COMP,				// u8Byte
	ODM_CMNINFO_DBG_LEVEL,				// u4Byte
	ODM_CMNINFO_RA_THRESHOLD_HIGH,		// u1Byte
	ODM_CMNINFO_RA_THRESHOLD_LOW,		// u1Byte
	ODM_CMNINFO_RF_ANTENNA_TYPE,		// u1Byte
	ODM_CMNINFO_BT_ENABLED,
	ODM_CMNINFO_BT_HS_CONNECT_PROCESS,
	ODM_CMNINFO_BT_HS_RSSI,
	ODM_CMNINFO_BT_OPERATION,
	ODM_CMNINFO_BT_LIMITED_DIG,					//Need to Limited Dig or not
	ODM_CMNINFO_BT_DISABLE_EDCA,
//------------CALL BY VALUE-------------//

	//
	// Dynamic ptr array hook itms.
	//
	ODM_CMNINFO_STA_STATUS,
	ODM_CMNINFO_PHY_STATUS,
	ODM_CMNINFO_MAC_STATUS,

	ODM_CMNINFO_MAX,


}ODM_CMNINFO_E;

//
// 2011/10/20 MH Define ODM support ability.  ODM_CMNINFO_ABILITY
//
typedef enum _ODM_Support_Ability_Definition
{
	//
	// BB ODM section BIT 0-15
	//
	ODM_BB_DIG					= BIT0,
	ODM_BB_RA_MASK				= BIT1,
	ODM_BB_DYNAMIC_TXPWR		= BIT2,
	ODM_BB_FA_CNT					= BIT3,
	ODM_BB_RSSI_MONITOR			= BIT4,
	ODM_BB_CCK_PD					= BIT5,
	ODM_BB_ANT_DIV				= BIT6,
	ODM_BB_PWR_SAVE				= BIT7,
	ODM_BB_PWR_TRAIN				= BIT8,
	ODM_BB_RATE_ADAPTIVE			= BIT9,
	ODM_BB_PATH_DIV				= BIT10,
	ODM_BB_PSD					= BIT11,
	ODM_BB_RXHP					= BIT12,
	ODM_BB_ADAPTIVITY				= BIT13,
	ODM_BB_CFO_TRACKING			= BIT14,

	//
	// MAC DM section BIT 16-23
	//
	ODM_MAC_EDCA_TURBO			= BIT16,
	ODM_MAC_EARLY_MODE			= BIT17,

	//
	// RF ODM section BIT 24-31
	//
	ODM_RF_TX_PWR_TRACK			= BIT24,
	ODM_RF_RX_GAIN_TRACK			= BIT25,
	ODM_RF_CALIBRATION				= BIT26,

}ODM_ABILITY_E;

//	ODM_CMNINFO_INTERFACE
typedef enum tag_ODM_Support_Interface_Definition
{
	ODM_ITRF_PCIE	=	0x1,
	ODM_ITRF_USB	=	0x2,
	ODM_ITRF_SDIO	=	0x4,
	ODM_ITRF_ALL	=	0x7,
}ODM_INTERFACE_E;

// ODM_CMNINFO_IC_TYPE
typedef enum tag_ODM_Support_IC_Type_Definition
{
	ODM_RTL8192S	=	BIT0,
	ODM_RTL8192C	=	BIT1,
	ODM_RTL8192D	=	BIT2,
	ODM_RTL8723A	=	BIT3,
	ODM_RTL8188E	=	BIT4,
	ODM_RTL8812	=	BIT5,
	ODM_RTL8821	=	BIT6,
	ODM_RTL8192E	=	BIT7,
	ODM_RTL8723B	=	BIT8,
	ODM_RTL8814A	=	BIT9,
	ODM_RTL8881A	=	BIT10,
	ODM_RTL8821B	=	BIT11
}ODM_IC_TYPE_E;

#define ODM_IC_11N_SERIES		(ODM_RTL8192S|ODM_RTL8192C|ODM_RTL8192D|ODM_RTL8723A|ODM_RTL8188E|ODM_RTL8192E|ODM_RTL8723B)
#define ODM_IC_11AC_SERIES		(ODM_RTL8812|ODM_RTL8821|ODM_RTL8814A|ODM_RTL8881A)

#define ODM_IC_11AC_SERIES_SUPPORT		1

//ODM_CMNINFO_CUT_VER
typedef enum tag_ODM_Cut_Version_Definition
{
	ODM_CUT_A		=	0,
	ODM_CUT_B		=	1,
	ODM_CUT_C		=	2,
	ODM_CUT_D		=	3,
	ODM_CUT_E		=	4,
	ODM_CUT_F		=	5,

	ODM_CUT_I		=	8,
	ODM_CUT_TEST	=	15,
}ODM_CUT_VERSION_E;

// ODM_CMNINFO_FAB_VER
typedef enum tag_ODM_Fab_Version_Definition
{
	ODM_TSMC	=	0,
	ODM_UMC		=	1,
}ODM_FAB_E;

// ODM_CMNINFO_RF_TYPE
//
// For example 1T2R (A+AB = BIT0|BIT4|BIT5)
//
typedef enum tag_ODM_RF_Path_Bit_Definition
{
	ODM_RF_TX_A	=	BIT0,
	ODM_RF_TX_B	=	BIT1,
	ODM_RF_TX_C	=	BIT2,
	ODM_RF_TX_D	=	BIT3,
	ODM_RF_RX_A	=	BIT4,
	ODM_RF_RX_B	=	BIT5,
	ODM_RF_RX_C	=	BIT6,
	ODM_RF_RX_D	=	BIT7,
}ODM_RF_PATH_E;


typedef enum tag_ODM_RF_Type_Definition
{
	ODM_1T1R	=	0,
	ODM_1T2R	=	1,
	ODM_2T2R	=	2,
	ODM_2T3R	=	3,
	ODM_2T4R	=	4,
	ODM_3T3R	=	5,
	ODM_3T4R	=	6,
	ODM_4T4R	=	7,
}ODM_RF_TYPE_E;


//
// ODM Dynamic common info value definition
//

//typedef enum _MACPHY_MODE_8192D{
//	SINGLEMAC_SINGLEPHY,
//	DUALMAC_DUALPHY,
//	DUALMAC_SINGLEPHY,
//}MACPHY_MODE_8192D,*PMACPHY_MODE_8192D;
// Above is the original define in MP driver. Please use the same define. THX.
typedef enum tag_ODM_MAC_PHY_Mode_Definition
{
	ODM_SMSP	= 0,
	ODM_DMSP	= 1,
	ODM_DMDP	= 2,
}ODM_MAC_PHY_MODE_E;


typedef enum tag_BT_Coexist_Definition
{
	ODM_BT_BUSY		= 1,
	ODM_BT_ON			= 2,
	ODM_BT_OFF		= 3,
	ODM_BT_NONE		= 4,
}ODM_BT_COEXIST_E;

// ODM_CMNINFO_OP_MODE
typedef enum tag_Operation_Mode_Definition
{
	ODM_NO_LINK		= BIT0,
	ODM_LINK			= BIT1,
	ODM_SCAN			= BIT2,
	ODM_POWERSAVE	= BIT3,
	ODM_AP_MODE		= BIT4,
	ODM_CLIENT_MODE	= BIT5,
	ODM_AD_HOC		= BIT6,
	ODM_WIFI_DIRECT	= BIT7,
	ODM_WIFI_DISPLAY	= BIT8,
}ODM_OPERATION_MODE_E;

// ODM_CMNINFO_WM_MODE
typedef enum tag_Wireless_Mode_Definition
{
        ODM_WM_UNKNOW     = 0x0,
        ODM_WM_B                  = BIT0,
        ODM_WM_G                  = BIT1,
        ODM_WM_A                  = BIT2,
        ODM_WM_N24G           = BIT3,
        ODM_WM_N5G             = BIT4,
        ODM_WM_AUTO           = BIT5,
        ODM_WM_AC                = BIT6,
}ODM_WIRELESS_MODE_E;

// ODM_CMNINFO_BAND
typedef enum tag_Band_Type_Definition
{
    ODM_BAND_2_4G = 0,
    ODM_BAND_5G,
    ODM_BAND_ON_BOTH,
    ODM_BANDMAX

}ODM_BAND_TYPE_E;

// ODM_CMNINFO_SEC_CHNL_OFFSET
typedef enum tag_Secondary_Channel_Offset_Definition
{
	ODM_DONT_CARE	= 0,
	ODM_BELOW		= 1,
	ODM_ABOVE			= 2
}ODM_SEC_CHNL_OFFSET_E;

// ODM_CMNINFO_SEC_MODE
typedef enum tag_Security_Definition
{
	ODM_SEC_OPEN			= 0,
	ODM_SEC_WEP40		= 1,
	ODM_SEC_TKIP			= 2,
	ODM_SEC_RESERVE			= 3,
	ODM_SEC_AESCCMP			= 4,
	ODM_SEC_WEP104		= 5,
	ODM_WEP_WPA_MIXED    = 6, // WEP + WPA
	ODM_SEC_SMS4			= 7,
}ODM_SECURITY_E;

// ODM_CMNINFO_BW
typedef enum tag_Bandwidth_Definition
{
	ODM_BW20M		= 0,
	ODM_BW40M		= 1,
	ODM_BW80M		= 2,
	ODM_BW160M		= 3,
	ODM_BW10M		= 4,
}ODM_BW_E;


// ODM_CMNINFO_BOARD_TYPE
// For non-AC-series IC , ODM_BOARD_5G_EXT_PA and ODM_BOARD_5G_EXT_LNA are ignored
// For AC-series IC, external PA & LNA can be indivisuallly added on 2.4G and/or 5G
typedef enum tag_Board_Definition
{
    ODM_BOARD_DEFAULT	= 0,	  // The DEFAULT case.
    ODM_BOARD_MINICARD  = BIT(0), // 0 = non-mini card, 1= mini card.
    ODM_BOARD_SLIM      = BIT(1), // 0 = non-slim card, 1 = slim card
    ODM_BOARD_BT        = BIT(2), // 0 = without BT card, 1 = with BT
    ODM_BOARD_EXT_PA    = BIT(3), // 0 = no 2G ext-PA, 1 = existing 2G ext-PA
    ODM_BOARD_EXT_LNA   = BIT(4), // 0 = no 2G ext-LNA, 1 = existing 2G ext-LNA
    ODM_BOARD_EXT_TRSW  = BIT(5), // 0 = no ext-TRSW, 1 = existing ext-TRSW
    ODM_BOARD_EXT_PA_5G	= BIT(6), // 0 = no 5G ext-PA, 1 = existing 5G ext-PA
    ODM_BOARD_EXT_LNA_5G= BIT(7), // 0 = no 5G ext-LNA, 1 = existing 5G ext-LNA
}ODM_BOARD_TYPE_E;

typedef enum tag_ODM_Package_Definition
{
    ODM_PACKAGE_DEFAULT		 = 0,
    ODM_PACKAGE_QFN68        = BIT(0),
    ODM_PACKAGE_TFBGA90      = BIT(1),
    ODM_PACKAGE_TFBGA79      = BIT(2),
}ODM_Package_TYPE_E;

typedef enum tag_ODM_TYPE_GPA_Definition
{
    TYPE_GPA0 = 0,
    TYPE_GPA1 = BIT(1)|BIT(0)
}ODM_TYPE_GPA_E;

typedef enum tag_ODM_TYPE_APA_Definition
{
    TYPE_APA0 = 0,
    TYPE_APA1 = BIT(1)|BIT(0)
}ODM_TYPE_APA_E;

typedef enum tag_ODM_TYPE_GLNA_Definition
{
    TYPE_GLNA0 = 0,
    TYPE_GLNA1 = BIT(2)|BIT(0),
    TYPE_GLNA2 = BIT(3)|BIT(1),
    TYPE_GLNA3 = BIT(3)|BIT(2)|BIT(1)|BIT(0)
}ODM_TYPE_GLNA_E;

typedef enum tag_ODM_TYPE_ALNA_Definition
{
    TYPE_ALNA0 = 0,
    TYPE_ALNA1 = BIT(2)|BIT(0),
    TYPE_ALNA2 = BIT(3)|BIT(1),
    TYPE_ALNA3 = BIT(3)|BIT(2)|BIT(1)|BIT(0)
}ODM_TYPE_ALNA_E;

// ODM_CMNINFO_ONE_PATH_CCA
typedef enum tag_CCA_Path
{
	ODM_CCA_2R			= 0,
	ODM_CCA_1R_A		= 1,
	ODM_CCA_1R_B		= 2,
}ODM_CCA_PATH_E;


typedef struct _ODM_RA_Info_
{
	u1Byte RateID;
	u4Byte RateMask;
	u4Byte RAUseRate;
	u1Byte RateSGI;
	u1Byte RssiStaRA;
	u1Byte PreRssiStaRA;
	u1Byte SGIEnable;
	u1Byte DecisionRate;
	u1Byte PreRate;
	u1Byte HighestRate;
	u1Byte LowestRate;
	u4Byte NscUp;
	u4Byte NscDown;
	u2Byte RTY[5];
	u4Byte TOTAL;
	u2Byte DROP;
	u1Byte Active;
	u2Byte RptTime;
	u1Byte RAWaitingCounter;
	u1Byte RAPendingCounter;
#if 1 //POWER_TRAINING_ACTIVE == 1 // For compile  pass only~!
	u1Byte PTActive;  // on or off
	u1Byte PTTryState;  // 0 trying state, 1 for decision state
	u1Byte PTStage;  // 0~6
	u1Byte PTStopCount; //Stop PT counter
	u1Byte PTPreRate;  // if rate change do PT
	u1Byte PTPreRssi; // if RSSI change 5% do PT
	u1Byte PTModeSS;  // decide whitch rate should do PT
	u1Byte RAstage;  // StageRA, decide how many times RA will be done between PT
	u1Byte PTSmoothFactor;
#endif
} ODM_RA_INFO_T,*PODM_RA_INFO_T;

typedef struct _IQK_MATRIX_REGS_SETTING{
	BOOLEAN		bIQKDone;
	s4Byte		Value[3][IQK_Matrix_REG_NUM];
	BOOLEAN		bBWIqkResultSaved[3];
}IQK_MATRIX_REGS_SETTING,*PIQK_MATRIX_REGS_SETTING;


//Remove PATHDIV_PARA struct to odm_PathDiv.h

typedef struct ODM_RF_Calibration_Structure
{
	//for tx power tracking

	u4Byte	RegA24; // for TempCCK
	s4Byte	RegE94;
	s4Byte	RegE9C;
	s4Byte	RegEB4;
	s4Byte	RegEBC;

	u1Byte		TXPowercount;
	BOOLEAN bTXPowerTrackingInit;
	BOOLEAN bTXPowerTracking;
	u1Byte		TxPowerTrackControl; //for mp mode, turn off txpwrtracking as default
	u1Byte		TM_Trigger;
	u1Byte		InternalPA5G[2];	//pathA / pathB

	u1Byte		ThermalMeter[2];    // ThermalMeter, index 0 for RFIC0, and 1 for RFIC1
	u1Byte		ThermalValue;
	u1Byte		ThermalValue_LCK;
	u1Byte		ThermalValue_IQK;
	u1Byte	ThermalValue_DPK;
	u1Byte	ThermalValue_AVG[AVG_THERMAL_NUM];
	u1Byte	ThermalValue_AVG_index;
	u1Byte	ThermalValue_RxGain;
	u1Byte	ThermalValue_Crystal;
	u1Byte	ThermalValue_DPKstore;
	u1Byte	ThermalValue_DPKtrack;
	BOOLEAN	TxPowerTrackingInProgress;

	BOOLEAN	bReloadtxpowerindex;
	u1Byte	bRfPiEnable;
	u4Byte	TXPowerTrackingCallbackCnt; //cosa add for debug


	//------------------------- Tx power Tracking -------------------------//
	u1Byte	bCCKinCH14;
	u1Byte	CCK_index;
	u1Byte	OFDM_index[MAX_RF_PATH];
	s1Byte	PowerIndexOffset[MAX_RF_PATH];
	s1Byte	DeltaPowerIndex[MAX_RF_PATH];
	s1Byte	DeltaPowerIndexLast[MAX_RF_PATH];
	BOOLEAN bTxPowerChanged;

	u1Byte	ThermalValue_HP[HP_THERMAL_NUM];
	u1Byte	ThermalValue_HP_index;
	IQK_MATRIX_REGS_SETTING IQKMatrixRegSetting[IQK_Matrix_Settings_NUM];
	BOOLEAN	bNeedIQK;
	BOOLEAN	bIQKInProgress;
	u1Byte	Delta_IQK;
	u1Byte	Delta_LCK;
	s1Byte  BBSwingDiff2G, BBSwingDiff5G; // Unit: dB
    u1Byte  DeltaSwingTableIdx_2GCCKA_P[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GCCKA_N[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GCCKB_P[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GCCKB_N[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GA_P[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GA_N[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GB_P[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GB_N[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_5GA_P[BAND_NUM][DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_5GA_N[BAND_NUM][DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_5GB_P[BAND_NUM][DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_5GB_N[BAND_NUM][DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GA_P_8188E[DELTA_SWINGIDX_SIZE];
    u1Byte  DeltaSwingTableIdx_2GA_N_8188E[DELTA_SWINGIDX_SIZE];

	//--------------------------------------------------------------------//

	//for IQK
	u4Byte	RegC04;
	u4Byte	Reg874;
	u4Byte	RegC08;
	u4Byte	RegB68;
	u4Byte	RegB6C;
	u4Byte	Reg870;
	u4Byte	Reg860;
	u4Byte	Reg864;

	BOOLEAN	bIQKInitialized;
	BOOLEAN bLCKInProgress;
	BOOLEAN	bAntennaDetected;
	u4Byte	ADDA_backup[IQK_ADDA_REG_NUM];
	u4Byte	IQK_MAC_backup[IQK_MAC_REG_NUM];
	u4Byte	IQK_BB_backup_recover[9];
	u4Byte	IQK_BB_backup[IQK_BB_REG_NUM];

	//for APK
	u4Byte	APKoutput[2][2]; //path A/B; output1_1a/output1_2a
	u1Byte	bAPKdone;
	u1Byte	bAPKThermalMeterIgnore;
	u1Byte	bDPdone;
	u1Byte	bDPPathAOK;
	u1Byte	bDPPathBOK;

	u4Byte	TxIQC_8723B[2][3][2]; // { {S1: 0xc94, 0xc80, 0xc4c} , {S0: 0xc9c, 0xc88, 0xc4c}}
	u4Byte	RxIQC_8723B[2][2][2]; // { {S1: 0xc14, 0xca0} ,           {S0: 0xc14, 0xca0}}
	u4Byte	TxLOK[2];

}ODM_RF_CAL_T,*PODM_RF_CAL_T;
//
// ODM Dynamic common info value definition
//

typedef struct _FAST_ANTENNA_TRAINNING_
{
	u1Byte	Bssid[6];
	u1Byte	antsel_rx_keep_0;
	u1Byte	antsel_rx_keep_1;
	u1Byte	antsel_rx_keep_2;
	u4Byte	antSumRSSI[7];
	u4Byte	antRSSIcnt[7];
	u4Byte	antAveRSSI[7];
	u1Byte	FAT_State;
	u4Byte	TrainIdx;
	u1Byte	antsel_a[ODM_ASSOCIATE_ENTRY_NUM];
	u1Byte	antsel_b[ODM_ASSOCIATE_ENTRY_NUM];
	u1Byte	antsel_c[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	MainAnt_Sum[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	AuxAnt_Sum[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	MainAnt_Cnt[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	AuxAnt_Cnt[ODM_ASSOCIATE_ENTRY_NUM];
	u1Byte	RxIdleAnt;
	BOOLEAN	bBecomeLinked;
	u4Byte	MinMaxRSSI;
	u1Byte	idx_AntDiv_counter_2G;
	u1Byte	idx_AntDiv_counter_5G;
	u4Byte	AntDiv_2G_5G;
	u4Byte    CCK_counter_main;
	u4Byte    CCK_counter_aux;
	u4Byte    OFDM_counter_main;
	u4Byte    OFDM_counter_aux;

}FAT_T,*pFAT_T;

typedef enum _FAT_STATE
{
	FAT_NORMAL_STATE			= 0,
	FAT_TRAINING_STATE		= 1,
}FAT_STATE_E, *PFAT_STATE_E;

typedef enum _ANT_DIV_TYPE
{
	NO_ANTDIV			= 0xFF,
	CG_TRX_HW_ANTDIV		= 0x01,
	CGCS_RX_HW_ANTDIV	= 0x02,
	FIXED_HW_ANTDIV		= 0x03,
	CG_TRX_SMART_ANTDIV	= 0x04,
	CGCS_RX_SW_ANTDIV	= 0x05,
	S0S1_SW_ANTDIV          = 0x06 //8723B intrnal switch S0 S1
}ANT_DIV_TYPE_E, *PANT_DIV_TYPE_E;

typedef struct _ODM_PATH_DIVERSITY_
{
	u1Byte	RespTxPath;
	u1Byte	PathSel[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	PathA_Sum[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	PathB_Sum[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	PathA_Cnt[ODM_ASSOCIATE_ENTRY_NUM];
	u4Byte	PathB_Cnt[ODM_ASSOCIATE_ENTRY_NUM];
}PATHDIV_T, *pPATHDIV_T;


typedef enum _BASEBAND_CONFIG_PHY_REG_PG_VALUE_TYPE{
	PHY_REG_PG_RELATIVE_VALUE = 0,
	PHY_REG_PG_EXACT_VALUE = 1
} PHY_REG_PG_TYPE;


//
// Antenna detection information from single tone mechanism, added by Roger, 2012.11.27.
//
typedef struct _ANT_DETECTED_INFO{
	BOOLEAN			bAntDetected;
	u4Byte			dBForAntA;
	u4Byte			dBForAntB;
	u4Byte			dBForAntO;
}ANT_DETECTED_INFO, *PANT_DETECTED_INFO;

//
// 2011/09/22 MH Copy from SD4 defined structure. We use to support PHY DM integration.
//
typedef  struct DM_Out_Source_Dynamic_Mechanism_Structure
{
	//
	//	Add for different team use temporarily
	//
	PADAPTER		Adapter;		// For CE/NIC team
	prtl8192cd_priv	priv;			// For AP/ADSL team
	// WHen you use Adapter or priv pointer, you must make sure the pointer is ready.
	BOOLEAN			odm_ready;

	rtl8192cd_priv		fake_priv;

	PHY_REG_PG_TYPE		PhyRegPgValueType;
	u1Byte				PhyRegPgVersion;

	u8Byte			DebugComponents;
	u4Byte			DebugLevel;

	u4Byte			NumQryPhyStatusAll;	//CCK + OFDM
	u4Byte			LastNumQryPhyStatusAll;
	u4Byte			RxPWDBAve;
	BOOLEAN			MPDIG_2G;		//off MPDIG
	u1Byte			Times_2G;

//------ ODM HANDLE, DRIVER NEEDS NOT TO HOOK------//
	BOOLEAN			bCckHighPower;
	u1Byte			RFPathRxEnable;		// ODM_CMNINFO_RFPATH_ENABLE
	u1Byte			ControlChannel;
//------ ODM HANDLE, DRIVER NEEDS NOT TO HOOK------//

//--------REMOVED COMMON INFO----------//
	//u1Byte				PseudoMacPhyMode;
	//BOOLEAN			*BTCoexist;
	//BOOLEAN			PseudoBtCoexist;
	//u1Byte				OPMode;
	//BOOLEAN			bAPMode;
	//BOOLEAN			bClientMode;
	//BOOLEAN			bAdHocMode;
	//BOOLEAN			bSlaveOfDMSP;
//--------REMOVED COMMON INFO----------//


//1  COMMON INFORMATION

	//
	// Init Value
	//
//-----------HOOK BEFORE REG INIT-----------//
	// ODM Platform info AP/ADSL/CE/MP = 1/2/3/4
	u1Byte			SupportPlatform;
	// ODM Support Ability DIG/RATR/TX_PWR_TRACK/ �K�K = 1/2/3/�K
	u4Byte			SupportAbility;
	// ODM PCIE/USB/SDIO = 1/2/3
	u1Byte			SupportInterface;
	// ODM composite or independent. Bit oriented/ 92C+92D+ .... or any other type = 1/2/3/...
	u4Byte			SupportICType;
	// Cut Version TestChip/A-cut/B-cut... = 0/1/2/3/...
	u1Byte			CutVersion;
	// Fab Version TSMC/UMC = 0/1
	u1Byte			FabVersion;
	// RF Type 4T4R/3T3R/2T2R/1T2R/1T1R/...
	u1Byte			RFType;
	u1Byte			RFEType;
	// Board Type Normal/HighPower/MiniCard/SLIM/Combo/... = 0/1/2/3/4/...
	u1Byte			BoardType;
	u1Byte			PackageType;
	u1Byte			TypeGLNA;
	u1Byte			TypeGPA;
	u1Byte			TypeALNA;
	u1Byte			TypeAPA;
	// with external LNA  NO/Yes = 0/1
	u1Byte			ExtLNA;
	u1Byte			ExtLNA5G;
	// with external PA  NO/Yes = 0/1
	u1Byte			ExtPA;
	u1Byte			ExtPA5G;
	// with external TRSW  NO/Yes = 0/1
	u1Byte			ExtTRSW;
	u1Byte			PatchID; //Customer ID
	BOOLEAN			bInHctTest;
	BOOLEAN			bWIFITest;

	BOOLEAN			bDualMacSmartConcurrent;
	u4Byte			BK_SupportAbility;
	u1Byte			AntDivType;

	u1Byte			odm_Regulation2_4G;
	u1Byte			odm_Regulation5G;
//-----------HOOK BEFORE REG INIT-----------//

	//
	// Dynamic Value
	//
//--------- POINTER REFERENCE-----------//

	u1Byte			u1Byte_temp;
	BOOLEAN			BOOLEAN_temp;
	PADAPTER		PADAPTER_temp;

	// MAC PHY Mode SMSP/DMSP/DMDP = 0/1/2
	u1Byte			*pMacPhyMode;
	//TX Unicast byte count
	u8Byte			*pNumTxBytesUnicast;
	//RX Unicast byte count
	u8Byte			*pNumRxBytesUnicast;
	// Wireless mode B/G/A/N = BIT0/BIT1/BIT2/BIT3
	u1Byte			*pWirelessMode; //ODM_WIRELESS_MODE_E
	// Frequence band 2.4G/5G = 0/1
	u1Byte			*pBandType;
	// Secondary channel offset don't_care/below/above = 0/1/2
	u1Byte			*pSecChOffset;
	// Security mode Open/WEP/AES/TKIP = 0/1/2/3
	u1Byte			*pSecurity;
	// BW info 20M/40M/80M = 0/1/2
	u1Byte			*pBandWidth;
	// Central channel location Ch1/Ch2/....
	u1Byte			*pChannel;	//central channel number
	BOOLEAN			DPK_Done;
	// Common info for 92D DMSP

	BOOLEAN			*pbGetValueFromOtherMac;
	PADAPTER		*pBuddyAdapter;
	BOOLEAN			*pbMasterOfDMSP; //MAC0: master, MAC1: slave
	// Common info for Status
	BOOLEAN			*pbScanInProcess;
	BOOLEAN			*pbPowerSaving;
	// CCA Path 2-path/path-A/path-B = 0/1/2; using ODM_CCA_PATH_E.
	u1Byte			*pOnePathCCA;
	//pMgntInfo->AntennaTest
	u1Byte			*pAntennaTest;
	BOOLEAN			*pbNet_closed;
	u1Byte			*mp_mode;
	//u1Byte			*pAidMap;
	u1Byte			*pu1ForcedIgiLb;
//--------- POINTER REFERENCE-----------//
	pu2Byte			pForcedDataRate;
//------------CALL BY VALUE-------------//
	BOOLEAN			bLinkInProcess;
	BOOLEAN			bWIFI_Direct;
	BOOLEAN			bWIFI_Display;
	BOOLEAN			bLinked;

	BOOLEAN			bsta_state;
	u1Byte			RSSI_Min;
	u1Byte          InterfaceIndex; // Add for 92D  dual MAC: 0--Mac0 1--Mac1
	BOOLEAN         bIsMPChip;
	BOOLEAN			bOneEntryOnly;
	// Common info for BTDM
	BOOLEAN			bBtEnabled;			// BT is disabled
	BOOLEAN			bBtConnectProcess;	// BT HS is under connection progress.
	u1Byte			btHsRssi;				// BT HS mode wifi rssi value.
	BOOLEAN			bBtHsOperation;		// BT HS mode is under progress
	BOOLEAN			bBtDisableEdcaTurbo;	// Under some condition, don't enable the EDCA Turbo
	BOOLEAN			bBtLimitedDig;			// BT is busy.
//------------CALL BY VALUE-------------//
	u1Byte			RSSI_A;
	u1Byte			RSSI_B;
	u8Byte			RSSI_TRSW;
	u8Byte			RSSI_TRSW_H;
	u8Byte			RSSI_TRSW_L;
	u8Byte			RSSI_TRSW_iso;

	u1Byte			RxRate;
	BOOLEAN			bNoisyState;
	u1Byte			TxRate;
	u1Byte			LinkedInterval;
	u1Byte			preChannel;
	u4Byte			TxagcOffsetValueA;
	BOOLEAN			IsTxagcOffsetPositiveA;
	u4Byte			TxagcOffsetValueB;
	BOOLEAN			IsTxagcOffsetPositiveB;
	u8Byte			lastTxOkCnt;
	u8Byte			lastRxOkCnt;
	u4Byte			BbSwingOffsetA;
	BOOLEAN			IsBbSwingOffsetPositiveA;
	u4Byte			BbSwingOffsetB;
	BOOLEAN			IsBbSwingOffsetPositiveB;
	s1Byte			TH_L2H_ini;
	s1Byte			TH_EDCCA_HL_diff;
	s1Byte			IGI_Base;
	u1Byte			IGI_target;
	BOOLEAN			ForceEDCCA;
	u1Byte			AdapEn_RSSI;
	s1Byte			Force_TH_H;
	s1Byte			Force_TH_L;
	u1Byte			IGI_LowerBound;
	u1Byte	                antdiv_rssi;
	u1Byte			AntType;
	u1Byte			pre_AntType;
	u1Byte		        antdiv_period;
        u1Byte		        antdiv_select;
	u1Byte			NdpaPeriod;

	// add by Yu Cehn for adaptivtiy
	BOOLEAN			adaptivity_flag;
	BOOLEAN			NHM_disable;
	BOOLEAN			TxHangFlg;
	BOOLEAN			Carrier_Sense_enable;
	u1Byte			tolerance_cnt;
	u8Byte			NHMCurTxOkcnt;
	u8Byte			NHMCurRxOkcnt;
	u8Byte			NHMLastTxOkcnt;
	u8Byte			NHMLastRxOkcnt;
	u1Byte			txEdcca1;
	u1Byte			txEdcca0;
	s1Byte			H2L_lb;
	s1Byte			L2H_lb;
	u1Byte			Adaptivity_IGI_upper;
	u1Byte			NHM_cnt_0;


	ODM_NOISE_MONITOR noise_level;//[ODM_MAX_CHANNEL_NUM];
	//
	//2 Define STA info.
	// _ODM_STA_INFO
	// 2012/01/12 MH For MP, we need to reduce one array pointer for default port.??
	PSTA_INFO_T		pODM_StaInfo[ODM_ASSOCIATE_ENTRY_NUM];

#if (RATE_ADAPTIVE_SUPPORT == 1)
	u2Byte			CurrminRptTime;
	ODM_RA_INFO_T   RAInfo[ODM_ASSOCIATE_ENTRY_NUM]; //See HalMacID support
#endif
	//
	// 2012/02/14 MH Add to share 88E ra with other SW team.
	// We need to colelct all support abilit to a proper area.
	//
	BOOLEAN				RaSupport88E;

	// Define ...........

	// Latest packet phy info (ODM write)
	ODM_PHY_DBG_INFO_T	 PhyDbgInfo;
	//PHY_INFO_88E		PhyInfo;

	// Latest packet phy info (ODM write)
	ODM_MAC_INFO		*pMacInfo;
	//MAC_INFO_88E		MacInfo;

	// Different Team independt structure??

	//
	//TX_RTP_CMN		TX_retrpo;
	//TX_RTP_88E		TX_retrpo;
	//TX_RTP_8195		TX_retrpo;

	//
	//ODM Structure
	//
	FAT_T						DM_FatTable;
	DIG_T						DM_DigTable;
	PS_T						DM_PSTable;
	Pri_CCA_T					DM_PriCCA;
	RXHP_T						DM_RXHP_Table;
	RA_T						DM_RA_Table;
	FALSE_ALARM_STATISTICS		FalseAlmCnt;
	FALSE_ALARM_STATISTICS		FlaseAlmCntBuddyAdapter;
	//#ifdef CONFIG_ANTENNA_DIVERSITY
	SWAT_T						DM_SWAT_Table;
	BOOLEAN						RSSI_test;
	CFO_TRACKING					DM_CfoTrack;
	//#endif

	EDCA_T		DM_EDCA_Table;
	u4Byte		WMMEDCA_BE;
	PATHDIV_T	DM_PathDiv;
	// Copy from SD4 structure
	//
	// ==================================================
	//

	BOOLEAN			*pbDriverStopped;
	BOOLEAN			*pbDriverIsGoingToPnpSetPowerSleep;
	BOOLEAN			*pinit_adpt_in_progress;

	//PSD
	BOOLEAN			bUserAssignLevel;
	RT_TIMER		PSDTimer;
	u1Byte			RSSI_BT;			//come from BT
	BOOLEAN			bPSDinProcess;
	BOOLEAN			bPSDactive;
	BOOLEAN			bDMInitialGainEnable;

	//MPT DIG
	RT_TIMER		MPT_DIGTimer;

	//for rate adaptive, in fact,  88c/92c fw will handle this
	u1Byte			bUseRAMask;

	ODM_RATE_ADAPTIVE	RateAdaptive;

	ANT_DETECTED_INFO	AntDetectedInfo; // Antenna detected information for RSSI tool

	ODM_RF_CAL_T	RFCalibrateInfo;

	//
	// TX power tracking
	//
	u1Byte			BbSwingIdxOfdm[MAX_RF_PATH];
	u1Byte			BbSwingIdxOfdmCurrent;
	u1Byte			BbSwingIdxOfdmBase[MAX_RF_PATH];
	BOOLEAN			BbSwingFlagOfdm;
	u1Byte			BbSwingIdxCck;
	u1Byte			BbSwingIdxCckCurrent;
	u1Byte			BbSwingIdxCckBase;
	u1Byte			DefaultOfdmIndex;
	u1Byte			DefaultCckIndex;
	BOOLEAN			BbSwingFlagCck;

	s1Byte			Absolute_OFDMSwingIdx[MAX_RF_PATH];
	s1Byte			Remnant_OFDMSwingIdx[MAX_RF_PATH];
	s1Byte			Remnant_CCKSwingIdx;
	s1Byte			Modify_TxAGC_Value;       //Remnat compensate value at TxAGC
	BOOLEAN			Modify_TxAGC_Flag_PathA;
	BOOLEAN			Modify_TxAGC_Flag_PathB;
	BOOLEAN			Modify_TxAGC_Flag_PathA_CCK;

	//
	// ODM system resource.
	//

	// ODM relative time.
	RT_TIMER				PathDivSwitchTimer;
	//2011.09.27 add for Path Diversity
	RT_TIMER				CCKPathDiversityTimer;
	RT_TIMER	FastAntTrainingTimer;

	// ODM relative workitem.
} DM_ODM_T, *PDM_ODM_T;		// DM_Dynamic_Mechanism_Structure



#if 1 //92c-series
#define ODM_RF_PATH_MAX 2
#else //jaguar - series
#define ODM_RF_PATH_MAX 4
#endif

typedef enum _ODM_RF_RADIO_PATH {
    ODM_RF_PATH_A = 0,   //Radio Path A
    ODM_RF_PATH_B = 1,   //Radio Path B
    ODM_RF_PATH_C = 2,   //Radio Path C
    ODM_RF_PATH_D = 3,   //Radio Path D
    ODM_RF_PATH_AB,
    ODM_RF_PATH_AC,
    ODM_RF_PATH_AD,
    ODM_RF_PATH_BC,
    ODM_RF_PATH_BD,
    ODM_RF_PATH_CD,
    ODM_RF_PATH_ABC,
    ODM_RF_PATH_ACD,
    ODM_RF_PATH_BCD,
    ODM_RF_PATH_ABCD,
  //  ODM_RF_PATH_MAX,    //Max RF number 90 support
} ODM_RF_RADIO_PATH_E, *PODM_RF_RADIO_PATH_E;

 typedef enum _ODM_RF_CONTENT{
	odm_radioa_txt = 0x1000,
	odm_radiob_txt = 0x1001,
	odm_radioc_txt = 0x1002,
	odm_radiod_txt = 0x1003
} ODM_RF_CONTENT;

typedef enum _ODM_BB_Config_Type{
    CONFIG_BB_PHY_REG,
    CONFIG_BB_AGC_TAB,
    CONFIG_BB_AGC_TAB_2G,
    CONFIG_BB_AGC_TAB_5G,
    CONFIG_BB_PHY_REG_PG,
    CONFIG_BB_PHY_REG_MP,
    CONFIG_BB_AGC_TAB_DIFF,
} ODM_BB_Config_Type, *PODM_BB_Config_Type;

typedef enum _ODM_RF_Config_Type{
	CONFIG_RF_RADIO,
    CONFIG_RF_TXPWR_LMT,
} ODM_RF_Config_Type, *PODM_RF_Config_Type;

typedef enum _ODM_FW_Config_Type{
    CONFIG_FW_NIC,
    CONFIG_FW_NIC_2,
    CONFIG_FW_AP,
    CONFIG_FW_MP,
    CONFIG_FW_WoWLAN,
    CONFIG_FW_WoWLAN_2,
    CONFIG_FW_AP_WoWLAN,
    CONFIG_FW_BT,
} ODM_FW_Config_Type;

// Status code
typedef enum _RT_STATUS{
	RT_STATUS_SUCCESS,
	RT_STATUS_FAILURE,
	RT_STATUS_PENDING,
	RT_STATUS_RESOURCE,
	RT_STATUS_INVALID_CONTEXT,
	RT_STATUS_INVALID_PARAMETER,
	RT_STATUS_NOT_SUPPORT,
	RT_STATUS_OS_API_FAILED,
}RT_STATUS,*PRT_STATUS;

#ifdef REMOVE_PACK
#pragma pack()
#endif

//#include "odm_function.h"

//3===========================================================
//3 DIG
//3===========================================================

//Remove DIG by Yuchen

//3===========================================================
//3 AGC RX High Power Mode
//3===========================================================
#define          LNA_Low_Gain_1                      0x64
#define          LNA_Low_Gain_2                      0x5A
#define          LNA_Low_Gain_3                      0x58

#define          FA_RXHP_TH1                           5000
#define          FA_RXHP_TH2                           1500
#define          FA_RXHP_TH3                             800
#define          FA_RXHP_TH4                             600
#define          FA_RXHP_TH5                             500

//3===========================================================
//3 EDCA
//3===========================================================

//3===========================================================
//3 Dynamic Tx Power
//3===========================================================
//Dynamic Tx Power Control Threshold

//Remove By YuChen

//3===========================================================
//3 Tx Power Tracking
//3===========================================================

//3===========================================================
//3 Rate Adaptive
//3===========================================================
//Remove to odm_RaInfo.h by RS_James

//3===========================================================
//3 BB Power Save
//3===========================================================

typedef enum tag_1R_CCA_Type_Definition
{
	CCA_1R =0,
	CCA_2R = 1,
	CCA_MAX = 2,
}DM_1R_CCA_E;

typedef enum tag_RF_Type_Definition
{
	RF_Save =0,
	RF_Normal = 1,
	RF_MAX = 2,
}DM_RF_E;

//3===========================================================
//3 Antenna Diversity
//3===========================================================
typedef enum tag_SW_Antenna_Switch_Definition
{
	Antenna_A = 1,
	Antenna_B = 2,
	Antenna_MAX = 3,
}DM_SWAS_E;


// Maximal number of antenna detection mechanism needs to perform, added by Roger, 2011.12.28.
#define	MAX_ANTENNA_DETECTION_CNT	10

//
// Extern Global Variables.
//
extern	u4Byte OFDMSwingTable[OFDM_TABLE_SIZE];
extern	u1Byte CCKSwingTable_Ch1_Ch13[CCK_TABLE_SIZE][8];
extern	u1Byte CCKSwingTable_Ch14 [CCK_TABLE_SIZE][8];

extern	u4Byte OFDMSwingTable_New[OFDM_TABLE_SIZE];
extern	u1Byte CCKSwingTable_Ch1_Ch13_New[CCK_TABLE_SIZE][8];
extern	u1Byte CCKSwingTable_Ch14_New [CCK_TABLE_SIZE][8];

extern  u4Byte TxScalingTable_Jaguar[TXSCALE_TABLE_SIZE];

// <20121018, Kordan> In case fail to read TxPowerTrack.txt, we use the table of 88E as the default table.
static u1Byte DeltaSwingTableIdx_2GA_P_8188E[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4,  4,  4,  4,  4,  4,  5,  5,  7,  7,  8,  8,  8,  9,  9,  9,  9,  9};
static u1Byte DeltaSwingTableIdx_2GA_N_8188E[] = {0, 0, 0, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,  6,  6,  7,  7,  7,  7,  8,  8,  9,  9, 10, 10, 10, 11, 11, 11, 11};

//
// check Sta pointer valid or not
//
#define IS_STA_VALID(pSta)		(pSta)

// 20100514 Joseph: Add definition for antenna switching test after link.
// This indicates two different the steps.
// In SWAW_STEP_PEAK, driver needs to switch antenna and listen to the signal on the air.
// In SWAW_STEP_DETERMINE, driver just compares the signal captured in SWAW_STEP_PEAK
// with original RSSI to determine if it is necessary to switch antenna.
#define SWAW_STEP_PEAK		0
#define SWAW_STEP_DETERMINE	1

//Remove DIG by yuchen

VOID
ODM_SetAntenna(
	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte		Antenna);


//Remove BB power saving by Yuchen

#define SwAntDivRestAfterLink	ODM_SwAntDivRestAfterLink
VOID ODM_SwAntDivRestAfterLink(	IN	PDM_ODM_T	pDM_Odm);

#define dm_CheckTXPowerTracking		ODM_TXPowerTrackingCheck
VOID
ODM_TXPowerTrackingCheck(
	IN		PDM_ODM_T		pDM_Odm
	);

//Remove ODM_RAStateCheck() by RS_James

#define dm_SWAW_RSSI_Check	ODM_SwAntDivChkPerPktRssi
VOID ODM_SwAntDivChkPerPktRssi(
	IN PDM_ODM_T		pDM_Odm,
	IN u1Byte			StationID,
	IN PODM_PHY_INFO_T pPhyInfo
	);

u4Byte ConvertTo_dB(u4Byte Value);

u4Byte
GetPSDData(
	PDM_ODM_T	pDM_Odm,
	unsigned int	point,
	u1Byte initial_gain_psd);

//Remove ODM_Get_Rate_Bitmap() by RS_James

#if (BEAMFORMING_SUPPORT == 1)
BEAMFORMING_CAP
Beamforming_GetEntryBeamCapByMacId(
 IN PMGNT_INFO pMgntInfo,
 IN u1Byte  MacId
 );
#endif

VOID
odm_TXPowerTrackingInit(
	IN	PDM_ODM_T	pDM_Odm
	);

VOID ODM_DMInit( IN	PDM_ODM_T	pDM_Odm);

VOID
ODM_DMWatchdog(
	IN		PDM_ODM_T			pDM_Odm			// For common use in the future
	);

VOID
ODM_CmnInfoInit(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u4Byte			Value
	);

VOID
ODM_CmnInfoHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		PVOID			pValue
	);

VOID
ODM_CmnInfoPtrArrayHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u2Byte			Index,
	IN		PVOID			pValue
	);

VOID
ODM_CmnInfoUpdate(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u4Byte			CmnInfo,
	IN		u8Byte			Value
	);

VOID
ODM_InitAllTimers(
    IN PDM_ODM_T	pDM_Odm
    );

VOID
ODM_CancelAllTimers(
    IN PDM_ODM_T    pDM_Odm
    );

VOID
ODM_ReleaseAllTimers(
    IN PDM_ODM_T	pDM_Odm
    );

VOID
ODM_ResetIQKResult(
    IN PDM_ODM_T pDM_Odm
    );

VOID
ODM_AntselStatistics_88C(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u1Byte			MacId,
	IN		u4Byte			PWDBAll,
	IN		BOOLEAN			isCCKrate
);

VOID
ODM_SingleDualAntennaDefaultSetting(
	IN		PDM_ODM_T		pDM_Odm
	);

BOOLEAN
ODM_SingleDualAntennaDetection(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u1Byte			mode
	);

VOID
ODM_UpdateNoisyState(
	IN	PDM_ODM_T	pDM_Odm,
	IN	BOOLEAN		bNoisyStateFromC2H
);

u4Byte
Set_RA_DM_Ratrbitmap_by_Noisy(
	IN	PDM_ODM_T	pDM_Odm,
	IN	WIRELESS_MODE	WirelessMode,
	IN	u4Byte			ratr_bitmap,
	IN	u1Byte			rssi_level
);

VOID
ODM_UpdateInitRate(
	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte		Rate
	);

//Remove ODM_DynamicARFBSelect() by RS_James

void odm_dtc(PDM_ODM_T pDM_Odm);

#endif
