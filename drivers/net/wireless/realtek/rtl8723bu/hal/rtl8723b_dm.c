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
//============================================================
// Description:
//
// This file is for 92CE/92CU dynamic mechanism only
//
//
//============================================================
#define _RTL8723B_DM_C_

//============================================================
// include files
//============================================================
#include <rtl8723b_hal.h>

//============================================================
// Global var
//============================================================


static VOID
dm_CheckProtection(
	IN	PADAPTER	Adapter
	)
{
}

static VOID
dm_CheckStatistics(
	IN	PADAPTER	Adapter
	)
{
}
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
static void dm_CheckPbcGPIO(_adapter *padapter)
{
	u8	tmp1byte;
	u8	bPbcPressed = _FALSE;

	if(!padapter->registrypriv.hw_wps_pbc)
		return;

	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte |= (HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as output mode

	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter,  GPIO_IN, tmp1byte);		//reset the floating voltage level

	tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
	tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
	rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as input mode

	tmp1byte =rtw_read8(padapter, GPIO_IN);

	if (tmp1byte == 0xff)
		return ;

	if (tmp1byte&HAL_8192C_HW_GPIO_WPS_BIT)
	{
		bPbcPressed = _TRUE;
	}
	if( _TRUE == bPbcPressed)
	{
		// Here we only set bPbcPressed to true
		// After trigger PBC, the variable will be set to false
		DBG_8192C("CheckPbcGPIO - PBC is pressed\n");
		rtw_request_wps_pbc_event(padapter);
	}
}
#endif //#ifdef CONFIG_SUPPORT_HW_WPS_PBC

//
// Initialize GPIO setting registers
//
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
//============================================================
// functions
//============================================================
static void Init_ODM_ComInfo_8723b(PADAPTER	Adapter)
{

	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u8	cut_ver,fab_ver;

	//
	// Init Value
	//
	_rtw_memset(pDM_Odm,0,sizeof(*pDM_Odm));

	pDM_Odm->Adapter = Adapter;
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_PLATFORM,ODM_CE);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_INTERFACE,Adapter->interface_type);//RTL871X_HCI_TYPE
	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_PACKAGE_TYPE, pHalData->PackageType);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_IC_TYPE, ODM_RTL8723B);

	fab_ver = ODM_TSMC;
	cut_ver = ODM_CUT_A;

	DBG_871X("%s(): fab_ver=%d cut_ver=%d\n", __func__, fab_ver, cut_ver);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_FAB_VER,fab_ver);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_CUT_VER,cut_ver);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_MP_TEST_CHIP,IS_NORMAL_CHIP(pHalData->VersionID));

	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_BOARD_TYPE,pHalData->BoardType);

	if(pHalData->BoardType == BOARD_USB_High_PA){
		ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_EXT_LNA,_TRUE);
		ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_EXT_PA,_TRUE);
	}
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_PATCH_ID,pHalData->CustomerID);
	//	ODM_CMNINFO_BINHCT_TEST only for MP Team
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_BWIFI_TEST,Adapter->registrypriv.wifi_spec);


	if(pHalData->rf_type == RF_1T1R){
		ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_RF_TYPE,ODM_1T1R);
	}
	else if(pHalData->rf_type == RF_2T2R){
		ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_RF_TYPE,ODM_2T2R);
	}
	else if(pHalData->rf_type == RF_1T2R){
		ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_RF_TYPE,ODM_1T2R);
	}

	#ifdef CONFIG_DISABLE_ODM
	pdmpriv->InitODMFlag = 0;
	#else
	pdmpriv->InitODMFlag =	ODM_RF_CALIBRATION		|
							ODM_RF_TX_PWR_TRACK	//|
							;
	//if(pHalData->AntDivCfg)
	//	pdmpriv->InitODMFlag |= ODM_BB_ANT_DIV;
	#endif

	ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_ABILITY,pdmpriv->InitODMFlag);
}

static void Update_ODM_ComInfo_8723b(PADAPTER	Adapter)
{
	struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
	struct mlme_priv		*pmlmepriv = &Adapter->mlmepriv;
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(Adapter);
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	int i;

	pdmpriv->InitODMFlag = 0
		| ODM_BB_DIG
		| ODM_BB_RA_MASK
		| ODM_BB_DYNAMIC_TXPWR
		| ODM_BB_FA_CNT
		| ODM_BB_RSSI_MONITOR
		| ODM_BB_CCK_PD
		| ODM_BB_PWR_SAVE
		| ODM_BB_CFO_TRACKING
		| ODM_MAC_EDCA_TURBO
		| ODM_RF_TX_PWR_TRACK
		| ODM_RF_CALIBRATION
		;

	if (rtw_odm_adaptivity_needed(Adapter) == _TRUE)
		pdmpriv->InitODMFlag |= ODM_BB_ADAPTIVITY;

#ifdef CONFIG_ANTENNA_DIVERSITY
	if(pHalData->AntDivCfg)
		pdmpriv->InitODMFlag |= ODM_BB_ANT_DIV;
#endif

#if (MP_DRIVER==1)
	if (Adapter->registrypriv.mp_mode == 1) {
		pdmpriv->InitODMFlag = 0
			| ODM_RF_CALIBRATION
			| ODM_RF_TX_PWR_TRACK
			;
	}
#endif//(MP_DRIVER==1)

#ifdef CONFIG_DISABLE_ODM
	pdmpriv->InitODMFlag = 0;
#endif//CONFIG_DISABLE_ODM

	//
	// Pointer reference
	//
	//ODM_CMNINFO_MAC_PHY_MODE pHalData->MacPhyMode92D
	//	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_MAC_PHY_MODE,&(pDM_Odm->u1Byte_temp));

	ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_ABILITY,pdmpriv->InitODMFlag);

	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_TX_UNI,&(Adapter->xmitpriv.tx_bytes));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_RX_UNI,&(Adapter->recvpriv.rx_bytes));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_WM_MODE,&(pmlmeext->cur_wireless_mode));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_SEC_CHNL_OFFSET,&(pHalData->nCur40MhzPrimeSC));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_SEC_MODE,&(Adapter->securitypriv.dot11PrivacyAlgrthm));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BW,&(pHalData->CurrentChannelBW ));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_CHNL,&( pHalData->CurrentChannel));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_NET_CLOSED,&( Adapter->net_closed));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_MP_MODE,&(Adapter->registrypriv.mp_mode));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BAND,&(pHalData->CurrentBandType));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_FORCED_IGI_LB,&(pHalData->u1ForcedIgiLb));
	//================= only for 8192D   =================
	/*
	//pHalData->CurrentBandType92D
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BAND,&(pDM_Odm->u1Byte_temp));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_DMSP_GET_VALUE,&(pDM_Odm->u1Byte_temp));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BUDDY_ADAPTOR,&(pDM_Odm->PADAPTER_temp));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_DMSP_IS_MASTER,&(pDM_Odm->u1Byte_temp));
	//================= only for 8192D   =================
	// driver havn't those variable now
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BT_OPERATION,&(pDM_Odm->u1Byte_temp));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_BT_DISABLE_EDCA,&(pDM_Odm->u1Byte_temp));
	*/
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_FORCED_RATE,&(pHalData->ForcedDataRate));

	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_SCAN,&(pmlmepriv->bScanInProcess));
	ODM_CmnInfoHook(pDM_Odm,ODM_CMNINFO_POWER_SAVING,&(pwrctrlpriv->bpower_saving));


	for(i=0; i< NUM_STA; i++)
	{
		//pDM_Odm->pODM_StaInfo[i] = NULL;
		ODM_CmnInfoPtrArrayHook(pDM_Odm, ODM_CMNINFO_STA_STATUS,i,NULL);
	}
}

void
rtl8723b_InitHalDm(
	IN	PADAPTER	Adapter
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	u8	i;

	dm_InitGPIOSetting(Adapter);

	pdmpriv->DM_Type = DM_Type_ByDriver;
	pdmpriv->DMFlag = DYNAMIC_FUNC_DISABLE;

#ifdef CONFIG_BT_COEXIST
	pdmpriv->DMFlag |= DYNAMIC_FUNC_BT;
#endif
	pdmpriv->InitDMFlag = pdmpriv->DMFlag;

	Update_ODM_ComInfo_8723b(Adapter);

	if (Adapter->registrypriv.mp_mode == 0)
		ODM_DMInit(pDM_Odm);

}

static void
FindMinimumRSSI_8723b(
IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	struct mlme_priv	*pmlmepriv = &pAdapter->mlmepriv;

	//1 1.Determine the minimum RSSI


#ifdef CONFIG_CONCURRENT_MODE
	//	FindMinimumRSSI()	per-adapter
	{
		PADAPTER pbuddy_adapter = pAdapter->pbuddy_adapter;
		PHAL_DATA_TYPE	pbuddy_HalData = GET_HAL_DATA(pbuddy_adapter);
		struct dm_priv *pbuddy_dmpriv = &pbuddy_HalData->dmpriv;

		if((pdmpriv->EntryMinUndecoratedSmoothedPWDB != 0) &&
                  (pbuddy_dmpriv->EntryMinUndecoratedSmoothedPWDB != 0))
		{

			if(pdmpriv->EntryMinUndecoratedSmoothedPWDB > pbuddy_dmpriv->EntryMinUndecoratedSmoothedPWDB)
				pdmpriv->EntryMinUndecoratedSmoothedPWDB = pbuddy_dmpriv->EntryMinUndecoratedSmoothedPWDB;
		} else {
			if(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0)
			      pdmpriv->EntryMinUndecoratedSmoothedPWDB = pbuddy_dmpriv->EntryMinUndecoratedSmoothedPWDB;

		}
	}
#endif

	if((check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE) &&
		(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0))
	{
		pdmpriv->MinUndecoratedPWDBForDM = 0;
		//ODM_RT_TRACE(pDM_Odm,COMP_BB_POWERSAVING, DBG_LOUD, ("Not connected to any \n"));
	}
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	// Default port
	{
		pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
	}
	else // associated entry pwdb
	{
		pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
		//ODM_RT_TRACE(pDM_Odm,COMP_BB_POWERSAVING, DBG_LOUD, ("AP Ext Port or disconnet PWDB = 0x%x \n", pHalData->MinUndecoratedPWDBForDM));
	}

	//odm_FindMinimumRSSI_Dmsp(pAdapter);
	//DBG_8192C("%s=>MinUndecoratedPWDBForDM(%d)\n",__FUNCTION__,pdmpriv->MinUndecoratedPWDBForDM);
	//ODM_RT_TRACE(pDM_Odm,COMP_DIG, DBG_LOUD, ("MinUndecoratedPWDBForDM =%d\n",pHalData->MinUndecoratedPWDBForDM));
}

VOID
rtl8723b_HalDmWatchDog(
	IN	PADAPTER	Adapter
	)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;
	u8 hw_init_completed = _FALSE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
#ifdef CONFIG_CONCURRENT_MODE
	PADAPTER pbuddy_adapter = Adapter->pbuddy_adapter;
#endif //CONFIG_CONCURRENT_MODE

	hw_init_completed = Adapter->hw_init_completed;

	if (hw_init_completed == _FALSE)
		goto skip_dm;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = adapter_to_pwrctl(Adapter)->bFwCurrentInPSMode;
	rtw_hal_get_hwreg(Adapter, HW_VAR_FWLPS_RF_ON, (u8 *)(&bFwPSAwake));
#endif

#ifdef CONFIG_P2P
	// Fw is under p2p powersaving mode, driver should stop dynamic mechanism.
	// modifed by thomas. 2011.06.11.
	if(Adapter->wdinfo.p2p_ps_mode)
		bFwPSAwake = _FALSE;
#endif //CONFIG_P2P


	if( (hw_init_completed == _TRUE)
		&& ((!bFwCurrentInPSMode) && bFwPSAwake))
	{
		//
		// Calculate Tx/Rx statistics.
		//
		dm_CheckStatistics(Adapter);
		rtw_hal_check_rxfifo_full(Adapter);
	}

	//ODM
	if (hw_init_completed == _TRUE)
	{
		u8	bLinked=_FALSE;
		u8	bsta_state=_FALSE;
		u8	bBtDisabled = _TRUE;

		if(rtw_linked_check(Adapter)){
			bLinked = _TRUE;
			if (check_fwstate(&Adapter->mlmepriv, WIFI_STATION_STATE))
				bsta_state = _TRUE;
		}

#ifdef CONFIG_CONCURRENT_MODE
		if(pbuddy_adapter && rtw_linked_check(pbuddy_adapter)){
			bLinked = _TRUE;
			if(pbuddy_adapter && check_fwstate(&pbuddy_adapter->mlmepriv, WIFI_STATION_STATE))
				bsta_state = _TRUE;
		}
#endif //CONFIG_CONCURRENT_MODE

		ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_LINK, bLinked);
		ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_STATION_STATE, bsta_state);

		//FindMinimumRSSI_8723b(Adapter);
		//ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_RSSI_MIN, pdmpriv->MinUndecoratedPWDBForDM);

#ifdef CONFIG_BT_COEXIST
		bBtDisabled = rtw_btcoex_IsBtDisabled(Adapter);
#endif // CONFIG_BT_COEXIST
		ODM_CmnInfoUpdate(&pHalData->odmpriv, ODM_CMNINFO_BT_ENABLED, ((bBtDisabled == _TRUE)?_FALSE:_TRUE));

		ODM_DMWatchdog(&pHalData->odmpriv);
	}

skip_dm:

	// Check GPIO to determine current RF on/off and Pbc status.
	// Check Hardware Radio ON/OFF or not
	//if(Adapter->MgntInfo.PowerSaveControl.bGpioRfSw)
	//{
		//RTPRINT(FPWR, PWRHW, ("dm_CheckRfCtrlGPIO \n"));
	//	dm_CheckRfCtrlGPIO(Adapter);
	//}
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
	dm_CheckPbcGPIO(Adapter);
#endif
	return;
}

void rtl8723b_hal_dm_in_lps(PADAPTER padapter)
{
	u32	PWDB_rssi=0;
	struct mlme_priv	*pmlmepriv = &padapter->mlmepriv;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
	pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
	struct sta_priv *pstapriv = &padapter->stapriv;
	struct sta_info *psta = NULL;

	DBG_871X("%s, RSSI_Min=%d\n", __func__, pDM_Odm->RSSI_Min);

	//update IGI
	ODM_Write_DIG(pDM_Odm, pDM_Odm->RSSI_Min);


	//set rssi to fw
	psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
	if(psta && (psta->rssi_stat.UndecoratedSmoothedPWDB > 0))
	{
		PWDB_rssi = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB<<16) );

		rtl8723b_set_rssi_cmd(padapter, (u8*)&PWDB_rssi);
	}

}

void rtl8723b_HalDmWatchDog_in_LPS(IN	PADAPTER	Adapter)
{
	u8	bLinked=_FALSE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &Adapter->mlmepriv;
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;
	pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
	struct sta_priv *pstapriv = &Adapter->stapriv;
	struct sta_info *psta = NULL;
#ifdef CONFIG_CONCURRENT_MODE
	PADAPTER pbuddy_adapter = Adapter->pbuddy_adapter;
#endif //CONFIG_CONCURRENT_MODE

	if (Adapter->hw_init_completed == _FALSE)
		goto skip_lps_dm;


	if(rtw_linked_check(Adapter))
		bLinked = _TRUE;

#ifdef CONFIG_CONCURRENT_MODE
	if (pbuddy_adapter && rtw_linked_check(pbuddy_adapter))
		bLinked = _TRUE;
#endif //CONFIG_CONCURRENT_MODE

	ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_LINK, bLinked);

	if(bLinked == _FALSE)
		goto skip_lps_dm;

	if (!(pDM_Odm->SupportAbility & ODM_BB_RSSI_MONITOR))
		goto skip_lps_dm;


	//ODM_DMWatchdog(&pHalData->odmpriv);
	//Do DIG by RSSI In LPS-32K

      //.1 Find MIN-RSSI
	psta = rtw_get_stainfo(pstapriv, get_bssid(pmlmepriv));
	if(psta == NULL)
		goto skip_lps_dm;

	pdmpriv->EntryMinUndecoratedSmoothedPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

	DBG_871X("CurIGValue=%d, EntryMinUndecoratedSmoothedPWDB = %d\n", pDM_DigTable->CurIGValue, pdmpriv->EntryMinUndecoratedSmoothedPWDB );

	if(pdmpriv->EntryMinUndecoratedSmoothedPWDB <=0)
		goto skip_lps_dm;

	pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;

	pDM_Odm->RSSI_Min = pdmpriv->MinUndecoratedPWDBForDM;

	//if(pDM_DigTable->CurIGValue != pDM_Odm->RSSI_Min)
	if((pDM_DigTable->CurIGValue > pDM_Odm->RSSI_Min + 5) ||
             (pDM_DigTable->CurIGValue < pDM_Odm->RSSI_Min - 5))

	{
		rtw_dm_in_lps_wk_cmd(Adapter);
	}


skip_lps_dm:

	return;

}

void rtl8723b_init_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		podmpriv = &pHalData->odmpriv;
	_rtw_memset(pdmpriv, 0, sizeof(struct dm_priv));
	Init_ODM_ComInfo_8723b(Adapter);
#ifdef CONFIG_SW_ANTENNA_DIVERSITY
	//_init_timer(&(pdmpriv->SwAntennaSwitchTimer),  Adapter->pnetdev , odm_SW_AntennaSwitchCallback, Adapter);
	ODM_InitAllTimers(podmpriv );
#endif

}

void rtl8723b_deinit_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		podmpriv = &pHalData->odmpriv;
#ifdef CONFIG_SW_ANTENNA_DIVERSITY
	//_cancel_timer_ex(&pdmpriv->SwAntennaSwitchTimer);
	ODM_CancelAllTimers(podmpriv);
#endif
}
