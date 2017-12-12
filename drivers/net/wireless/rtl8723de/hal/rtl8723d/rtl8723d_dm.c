/******************************************************************************
 *
 * Copyright(c) 2007 - 2012 Realtek Corporation. All rights reserved.
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
 * Description:
 *
 * This file is for 92CE/92CU dynamic mechanism only
 *
 *
 * ************************************************************ */
#define _RTL8723D_DM_C_

/* ************************************************************
 * include files
 * ************************************************************ */
#include <rtl8723d_hal.h>

/* ************************************************************
 * Global var
 * ************************************************************ */


static VOID
dm_CheckProtection(
	IN	PADAPTER	Adapter
)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			CurRate, RateThreshold;

	if (pMgntInfo->pHTInfo->bCurBW40MHz)
		RateThreshold = MGN_MCS1;
	else
		RateThreshold = MGN_MCS3;

	if (Adapter->TxStats.CurrentInitTxRate <= RateThreshold) {
		pMgntInfo->bDmDisableProtect = TRUE;
		DbgPrint("Forced disable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	} else {
		pMgntInfo->bDmDisableProtect = FALSE;
		DbgPrint("Enable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	}
#endif
}

static VOID
dm_CheckStatistics(
	IN	PADAPTER	Adapter
)
{
#if 0
	if (!Adapter->MgntInfo.bMediaConnect)
		return;

	/* 2008.12.10 tynli Add for getting Current_Tx_Rate_Reg flexibly. */
	rtw_hal_get_hwreg(Adapter, HW_VAR_INIT_TX_RATE, (pu1Byte)(&Adapter->TxStats.CurrentInitTxRate));

	/* Calculate current Tx Rate(Successful transmited!!) */

	/* Calculate current Rx Rate(Successful received!!) */

	/* for tx tx retry count */
	rtw_hal_get_hwreg(Adapter, HW_VAR_RETRY_COUNT, (pu1Byte)(&Adapter->TxStats.NumTxRetryCount));
#endif
}
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
static void dm_CheckPbcGPIO(_adapter *padapter)
{
	u8	tmp1byte;
	u8	bPbcPressed = _FALSE;

	if (!padapter->registrypriv.hw_wps_pbc)
		return;

#ifdef CONFIG_USB_HCI
	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte |= (HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	/* enable GPIO[2] as output mode */

	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter,  GPIO_IN, tmp1byte);		/* reset the floating voltage level */

	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	/* enable GPIO[2] as input mode */

	tmp1byte = rtw_read8(padapter, GPIO_IN);

	if (tmp1byte == 0xff)
		return;

	if (tmp1byte & HAL_8192C_HW_GPIO_WPS_BIT)
		bPbcPressed = _TRUE;
#else
	tmp1byte = rtw_read8(padapter, GPIO_IN);

	if (tmp1byte == 0xff || padapter->init_adpt_in_progress)
		return;

	if ((tmp1byte & HAL_8192C_HW_GPIO_WPS_BIT) == 0)
		bPbcPressed = _TRUE;
#endif

	if (_TRUE == bPbcPressed) {
		/* Here we only set bPbcPressed to true */
		/* After trigger PBC, the variable will be set to false */
		RTW_INFO("CheckPbcGPIO - PBC is pressed\n");
		rtw_request_wps_pbc_event(padapter);
	}
}
#endif /* #ifdef CONFIG_SUPPORT_HW_WPS_PBC */


#ifdef CONFIG_PCI_HCI
/*
 *	Description:
 *		Perform interrupt migration dynamically to reduce CPU utilization.
 *
 *	Assumption:
 *		1. Do not enable migration under WIFI test.
 *
 *	Created by Roger, 2010.03.05.
 *   */
VOID
dm_InterruptMigration(
	IN	PADAPTER	Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	BOOLEAN			bCurrentIntMt, bCurrentACIntDisable;
	BOOLEAN			IntMtToSet = _FALSE;
	BOOLEAN			ACIntToSet = _FALSE;


	/* Retrieve current interrupt migration and Tx four ACs IMR settings first. */
	bCurrentIntMt = pHalData->bInterruptMigration;
	bCurrentACIntDisable = pHalData->bDisableTxInt;

	/* */
	/* <Roger_Notes> Currently we use busy traffic for reference instead of RxIntOK counts to prevent non-linear Rx statistics */
	/* when interrupt migration is set before. 2010.03.05. */
	/* */
	if (!Adapter->registrypriv.wifi_spec &&
	    (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) &&
	    pmlmepriv->LinkDetectInfo.bHigherBusyTraffic) {
		IntMtToSet = _TRUE;

		/* To check whether we should disable Tx interrupt or not. */
		if (pmlmepriv->LinkDetectInfo.bHigherBusyRxTraffic)
			ACIntToSet = _TRUE;
	}

	/* Update current settings. */
	if (bCurrentIntMt != IntMtToSet) {
		RTW_INFO("%s(): Update interrupt migration(%d)\n", __FUNCTION__, IntMtToSet);
		if (IntMtToSet) {
			/* */
			/* <Roger_Notes> Set interrupt migration timer and corresponging Tx/Rx counter. */
			/* timer 25ns*0xfa0=100us for 0xf packets. */
			/* 2010.03.05. */
			/* */
			rtw_write32(Adapter, REG_INT_MIG_8723D, 0xff000fa0);/* 0x306:Rx, 0x307:Tx */
			pHalData->bInterruptMigration = IntMtToSet;
		} else {
			/* Reset all interrupt migration settings. */
			rtw_write32(Adapter, REG_INT_MIG_8723D, 0);
			pHalData->bInterruptMigration = IntMtToSet;
		}
	}

	/*if( bCurrentACIntDisable != ACIntToSet ){
		RTW_INFO("%s(): Update AC interrupt(%d)\n",__FUNCTION__,ACIntToSet);
		if(ACIntToSet)
		{





			UpdateInterruptMask8192CE( Adapter, 0, RT_AC_INT_MASKS );
			pHalData->bDisableTxInt = ACIntToSet;
		}
		else
		{
			UpdateInterruptMask8192CE( Adapter, RT_AC_INT_MASKS, 0 );
			pHalData->bDisableTxInt = ACIntToSet;
		}
	}*/

}

#endif

/*
 * Initialize GPIO setting registers
 *   */
static void
dm_InitGPIOSetting(
	IN	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);

	u8	tmp1byte;

	tmp1byte = rtw_read8(Adapter, REG_GPIO_MUXCFG);
	tmp1byte &= (GPIOSEL_GPIO | ~GPIOSEL_ENBT);

	rtw_write8(Adapter, REG_GPIO_MUXCFG, tmp1byte);
}
/* ************************************************************
 * functions
 * ************************************************************ */
static void Init_ODM_ComInfo_8723d(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	u32 SupportAbility = 0;
	u8	cut_ver, fab_ver;

	Init_ODM_ComInfo(Adapter);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_PACKAGE_TYPE, pHalData->PackageType);

	fab_ver = ODM_TSMC;
	cut_ver = GET_CVID_CUT_VERSION(pHalData->VersionID);

	RTW_INFO("%s(): fab_ver=%d cut_ver=%d\n", __func__, fab_ver, cut_ver);
	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_FAB_VER, fab_ver);
	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_CUT_VER, cut_ver);

#ifdef CONFIG_DISABLE_ODM
	SupportAbility = 0;
#else
	SupportAbility =
#if 1
		ODM_RF_CALIBRATION |
		ODM_RF_TX_PWR_TRACK
#else
		0
#endif
		;
	if (pHalData->AntDivCfg)
		SupportAbility |= ODM_BB_ANT_DIV;
#endif

	ODM_CmnInfoUpdate(pDM_Odm, ODM_CMNINFO_ABILITY, SupportAbility);
}

static void Update_ODM_ComInfo_8723d(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	u32 SupportAbility = 0;

	SupportAbility = 0
			 | ODM_BB_DIG		/* For BB */
			 | ODM_BB_RA_MASK
			 | ODM_BB_FA_CNT
			 | ODM_BB_RSSI_MONITOR
			 | ODM_BB_CCK_PD
			 | ODM_BB_CFO_TRACKING
			 /* | ODM_BB_PWR_TRAIN */
			 | ODM_BB_NHM_CNT
			 | ODM_RF_TX_PWR_TRACK /* For RF */
			 | ODM_RF_CALIBRATION
			 | ODM_MAC_EDCA_TURBO		/* For MAC */
			 ;

	if (rtw_odm_adaptivity_needed(Adapter) == _TRUE) {
		rtw_odm_adaptivity_config_msg(RTW_DBGDUMP, Adapter);
		SupportAbility |= ODM_BB_ADAPTIVITY;
	}

#ifdef CONFIG_ANTENNA_DIVERSITY
	if (pHalData->AntDivCfg)
		SupportAbility |= ODM_BB_ANT_DIV;
#endif

#if (MP_DRIVER == 1)
	if (Adapter->registrypriv.mp_mode == 1) {
		SupportAbility = 0
				 | ODM_RF_CALIBRATION
				 | ODM_RF_TX_PWR_TRACK
				 ;
	}
#endif/* (MP_DRIVER==1) */

#ifdef CONFIG_DISABLE_ODM
	SupportAbility = 0;
#endif/* CONFIG_DISABLE_ODM */

	ODM_CmnInfoUpdate(pDM_Odm, ODM_CMNINFO_ABILITY, SupportAbility);
	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_RF_ANTENNA_TYPE,
			pHalData->TRxAntDivType);
}

void
rtl8723d_InitHalDm(
	IN	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);

	u8	i;

#ifdef CONFIG_USB_HCI
	dm_InitGPIOSetting(Adapter);
#endif

	pHalData->DM_Type = DM_Type_ByDriver;

	Update_ODM_ComInfo_8723d(Adapter);

	ODM_DMInit(pDM_Odm);

}

VOID
rtl8723d_HalDmWatchDog(
	IN	PADAPTER	Adapter
)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

#ifdef CONFIG_MP_INCLUDED
	/* #if MP_DRIVER */
	if (Adapter->registrypriv.mp_mode == 1 && Adapter->mppriv.mp_dm == 0) /* for MP power tracking */
		return;
	/* #endif */
#endif

	if (!rtw_is_hw_init_completed(Adapter))
		goto skip_dm;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = adapter_to_pwrctl(Adapter)->bFwCurrentInPSMode;
	rtw_hal_get_hwreg(Adapter, HW_VAR_FWLPS_RF_ON, (u8 *)(&bFwPSAwake));
#endif

#ifdef CONFIG_P2P
	/* Fw is under p2p powersaving mode, driver should stop dynamic mechanism. */
	/* modifed by thomas. 2011.06.11. */
	if (Adapter->wdinfo.p2p_ps_mode)
		bFwPSAwake = _FALSE;
#endif /* CONFIG_P2P */


	if ((rtw_is_hw_init_completed(Adapter))
	    && ((!bFwCurrentInPSMode) && bFwPSAwake)) {
		/* */
		/* Calculate Tx/Rx statistics. */
		/* */
		dm_CheckStatistics(Adapter);
		rtw_hal_check_rxfifo_full(Adapter);
		/* */
		/* Dynamically switch RTS/CTS protection. */
		/* */
		/* dm_CheckProtection(Adapter); */

#ifdef CONFIG_PCI_HCI
		/* 20100630 Joseph: Disable Interrupt Migration mechanism temporarily because it degrades Rx throughput. */
		/* Tx Migration settings. */
		/* dm_InterruptMigration(Adapter); */

		/* if(Adapter->HalFunc.TxCheckStuckHandler(Adapter)) */
		/*	PlatformScheduleWorkItem(&(GET_HAL_DATA(Adapter)->HalResetWorkItem)); */
#endif
	}

	/* ODM */
	if (rtw_is_hw_init_completed(Adapter)) {
		u8	bLinked = _FALSE;
		u8	bsta_state = _FALSE;
		u8	bBtDisabled = _TRUE;

		if (rtw_mi_check_status(Adapter, MI_ASSOC)) {
			bLinked = _TRUE;
			if (rtw_mi_check_status(Adapter, MI_STA_LINKED))
				bsta_state = _TRUE;
		}

		ODM_CmnInfoUpdate(&pHalData->odmpriv , ODM_CMNINFO_LINK, bLinked);
		ODM_CmnInfoUpdate(&pHalData->odmpriv , ODM_CMNINFO_STATION_STATE, bsta_state);

		/* ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_RSSI_MIN, pdmpriv->MinUndecoratedPWDBForDM); */

#ifdef CONFIG_BT_COEXIST
		bBtDisabled = rtw_btcoex_IsBtDisabled(Adapter);
#endif /* CONFIG_BT_COEXIST */
		ODM_CmnInfoUpdate(&pHalData->odmpriv, ODM_CMNINFO_BT_ENABLED, ((bBtDisabled == _TRUE) ? _FALSE : _TRUE));

		ODM_DMWatchdog(&pHalData->odmpriv);
	}

skip_dm:

	/* Check GPIO to determine current RF on/off and Pbc status. */
	/* Check Hardware Radio ON/OFF or not */
	/* if(Adapter->MgntInfo.PowerSaveControl.bGpioRfSw) */
	/* { */
	/* RTPRINT(FPWR, PWRHW, ("dm_CheckRfCtrlGPIO\n")); */
	/*	dm_CheckRfCtrlGPIO(Adapter); */
	/* } */
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
	dm_CheckPbcGPIO(Adapter);
#endif
	return;
}

void rtl8723d_hal_dm_in_lps(PADAPTER padapter)
{
	u32	PWDB_rssi = 0;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
	pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info *psta = NULL;

	RTW_INFO("%s, RSSI_Min=%d\n", __func__, pDM_Odm->RSSI_Min);

	/* update IGI */
	ODM_Write_DIG(pDM_Odm, pDM_Odm->RSSI_Min);


	/* set rssi to fw */
	psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
	if (psta && (psta->rssi_stat.UndecoratedSmoothedPWDB > 0)) {
		PWDB_rssi = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB << 16));

		rtl8723d_set_rssi_cmd(padapter, (u8 *)&PWDB_rssi);
	}

}

void rtl8723d_HalDmWatchDog_in_LPS(IN	PADAPTER	Adapter)
{
	u8	bLinked = _FALSE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &Adapter->mlmepriv;
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
	pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
	struct sta_priv *pstapriv = &Adapter->stapriv;
	struct sta_info *psta = NULL;

	if (!rtw_is_hw_init_completed(Adapter))
		goto skip_lps_dm;

	if (rtw_mi_check_status(Adapter, MI_ASSOC))
		bLinked = _TRUE;

	ODM_CmnInfoUpdate(&pHalData->odmpriv , ODM_CMNINFO_LINK, bLinked);

	if (bLinked == _FALSE)
		goto skip_lps_dm;

	if (!(pDM_Odm->SupportAbility & ODM_BB_RSSI_MONITOR))
		goto skip_lps_dm;


	/* ODM_DMWatchdog(&pHalData->odmpriv);	 */
	/* Do DIG by RSSI In LPS-32K */

	/* .1 Find MIN-RSSI */
	psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
	if (psta == NULL)
		goto skip_lps_dm;

	pHalData->EntryMinUndecoratedSmoothedPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

	RTW_INFO("CurIGValue=%d, EntryMinUndecoratedSmoothedPWDB = %d\n", pDM_DigTable->CurIGValue, pHalData->EntryMinUndecoratedSmoothedPWDB);

	if (pHalData->EntryMinUndecoratedSmoothedPWDB <= 0)
		goto skip_lps_dm;

	pHalData->MinUndecoratedPWDBForDM = pHalData->EntryMinUndecoratedSmoothedPWDB;

	pDM_Odm->RSSI_Min = pHalData->MinUndecoratedPWDBForDM;

	/* if(pDM_DigTable->CurIGValue != pDM_Odm->RSSI_Min) */
	if ((pDM_DigTable->CurIGValue > pDM_Odm->RSSI_Min + 5) ||
	    (pDM_DigTable->CurIGValue < pDM_Odm->RSSI_Min - 5))

	{
#ifdef CONFIG_LPS
		rtw_dm_in_lps_wk_cmd(Adapter);
#endif /* CONFIG_LPS */
	}


skip_lps_dm:

	return;

}

void rtl8723d_init_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		podmpriv = &pHalData->odmpriv;

	Init_ODM_ComInfo_8723d(Adapter);
	ODM_InitAllTimers(podmpriv);

}

void rtl8723d_deinit_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		podmpriv = &pHalData->odmpriv;

	ODM_CancelAllTimers(podmpriv);

}
