/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
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
 *****************************************************************************/
#define _RTL8822B_HALINIT_C_

#include <drv_types.h>		/* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* GET_HAL_SPEC(), HAL_DATA_TYPE */
#include "../hal_halmac.h"	/* HALMAC API */
#include "rtl8822b.h"


void rtl8822b_init_hal_spec(PADAPTER adapter)
{
	struct hal_spec_t *hal_spec;


	hal_spec = GET_HAL_SPEC(adapter);
	rtw_halmac_fill_hal_spec(adapter_to_dvobj(adapter), hal_spec);

	hal_spec->ic_name = "rtl8822b";
	hal_spec->macid_num = 128;
	/* hal_spec->sec_cam_ent_num follow halmac setting */
	hal_spec->sec_cap = SEC_CAP_CHK_BMC;
	hal_spec->rfpath_num_2g = 2;
	hal_spec->rfpath_num_5g = 2;
	hal_spec->max_tx_cnt = 2;
	hal_spec->tx_nss_num = 2;
	hal_spec->rx_nss_num = 2;
	hal_spec->band_cap = BAND_CAP_2G | BAND_CAP_5G;
	hal_spec->bw_cap = BW_CAP_20M | BW_CAP_40M | BW_CAP_80M;
	hal_spec->port_num = 5;
	hal_spec->proto_cap = PROTO_CAP_11B | PROTO_CAP_11G | PROTO_CAP_11N | PROTO_CAP_11AC;

	hal_spec->wl_func = 0
			    | WL_FUNC_P2P
			    | WL_FUNC_MIRACAST
			    | WL_FUNC_TDLS
			    ;

	hal_spec->hci_type = 0;
}

u32 rtl8822b_power_on(PADAPTER adapter)
{
	struct dvobj_priv *d;
	PHAL_DATA_TYPE hal;
	u8 bMacPwrCtrlOn;
	int err = 0;
	u8 ret = _SUCCESS;


	d = adapter_to_dvobj(adapter);

	bMacPwrCtrlOn = _FALSE;
	rtw_hal_get_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
	if (bMacPwrCtrlOn == _TRUE)
		goto out;

	err = rtw_halmac_poweron(d);
	if (err) {
		RTW_ERR("%s: Power ON Fail!!\n", __FUNCTION__);
		ret = _FAIL;
		goto out;
	}

	bMacPwrCtrlOn = _TRUE;
	rtw_hal_set_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

out:
	return ret;
}

void rtl8822b_power_off(PADAPTER adapter)
{
	struct dvobj_priv *d;
	u8 bMacPwrCtrlOn;
	int err = 0;


	d = adapter_to_dvobj(adapter);

	bMacPwrCtrlOn = _FALSE;
	rtw_hal_get_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
	if (bMacPwrCtrlOn == _FALSE)
		goto out;

	err = rtw_halmac_poweroff(d);
	if (err) {
		RTW_ERR("%s: Power OFF Fail!!\n", __FUNCTION__);
		goto out;
	}

	bMacPwrCtrlOn = _FALSE;
	rtw_hal_set_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

	GET_HAL_DATA(adapter)->bFWReady = _FALSE;

out:
	return;
}

u8 rtl8822b_hal_init(PADAPTER adapter)
{
	struct dvobj_priv *d;
	PHAL_DATA_TYPE hal;
	int err;
	u8 fw_bin = _TRUE;

	d = adapter_to_dvobj(adapter);
	hal = GET_HAL_DATA(adapter);

	hal->bFWReady = _FALSE;
	hal->fw_ractrl = _FALSE;

#ifdef CONFIG_FILE_FWIMG
	rtw_get_phy_file_path(adapter, MAC_FILE_FW_NIC);
	if (rtw_is_file_readable(rtw_phy_para_file_path) == _TRUE) {
		RTW_INFO("%s acquire FW from file:%s\n", __FUNCTION__, rtw_phy_para_file_path);
		fw_bin = _TRUE;
	} else
#endif /* CONFIG_FILE_FWIMG */
	{
		RTW_INFO("%s fw source from array\n", __FUNCTION__);
		fw_bin = _FALSE;
	}

#ifdef CONFIG_FILE_FWIMG
	if (_TRUE == fw_bin)
		err = rtw_halmac_init_hal_fw_file(d, rtw_phy_para_file_path);
	else
#endif /* CONFIG_FILE_FWIMG */
		err = rtw_halmac_init_hal_fw(d, array_mp_8822b_fw_nic, array_length_mp_8822b_fw_nic);

	if (err) {
		RTW_ERR("%s Download Firmware from %s failed\n", __FUNCTION__, (fw_bin) ? "file" : "array");
		return _FALSE;
	}

	

	RTW_INFO("%s Download Firmware from %s success\n", __FUNCTION__, (fw_bin) ? "file" : "array");
	RTW_INFO("%s FW Version:%d SubVersion:%d FW size:%d\n", "NIC",
		hal->firmware_version, hal->firmware_sub_version, hal->firmware_size);

	/* Sync driver status with hardware setting */
	rtl8822b_rcr_get(adapter, NULL);
	hal->bFWReady = _TRUE;
	hal->fw_ractrl = _TRUE;

	return _TRUE;
}

u8 rtl8822b_mac_verify(PADAPTER adapter)
{
	struct dvobj_priv *d;
	int err;


	d = adapter_to_dvobj(adapter);

	err = rtw_halmac_self_verify(d);
	if (err) {
		RTW_INFO("%s fail\n", __FUNCTION__);
		return _FALSE;
	}

	RTW_INFO("%s successful\n", __FUNCTION__);
	return _TRUE;
}

void rtl8822b_init_beacon_variable(struct _ADAPTER *adapter)
{
	struct hal_com_data *hal = GET_HAL_DATA(adapter);


	hal->RegBcnCtrlVal = rtw_read8(adapter, REG_BCN_CTRL_8822B);
	hal->RegTxPause = rtw_read8(adapter, REG_TXPAUSE_8822B);
	hal->RegFwHwTxQCtrl = rtw_read8(adapter, REG_FWHW_TXQ_CTRL_8822B + 2);
	hal->RegCR_1 = rtw_read8(adapter, REG_CR_8822B + 1);
}

void rtl8822b_init_misc(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	u8 v8 = 0;
	u32 v32 = 0;


	hal = GET_HAL_DATA(adapter);


	/*
	 * Sync driver status and hardware setting
	 */

	/* initial channel setting */
	if (IS_A_CUT(hal->version_id) || IS_B_CUT(hal->version_id)) {
		/* for A/B cut use under only 5G */
		u8 i = 0;
		struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
		PADAPTER iface = NULL;

		RTW_INFO("%s: under only 5G for A/B cut\n", __FUNCTION__);
		RTW_INFO("%s: not support HT/VHT RX STBC for A/B cut\n", __FUNCTION__);

		for (i = 0; i < dvobj->iface_nums; i++) {
			iface = dvobj->padapters[i];
			if (iface) {
				iface->registrypriv.wireless_mode = WIRELESS_MODE_5G;
				iface->registrypriv.channel = 149;

				iface->registrypriv.stbc_cap &= ~(BIT0 | BIT4);
			}
		}
	}

	/* Modify to make sure first time change channel(band) would be done properly */
	hal->current_channel = 0;
	hal->current_channel_bw = CHANNEL_WIDTH_MAX;
	hal->current_band_type = BAND_MAX;

	/* initial security setting */
	invalidate_cam_all(adapter);

	/* check RCR/ICV bit */
	rtl8822b_rcr_clear(adapter, BIT_ACRC32_8822B | BIT_AICV_8822B);

	/* clear rx ctrl frame */
	rtw_write16(adapter, REG_RXFLTMAP1_8822B, 0);

	/*Enable MAC security engine*/
	rtw_write16(adapter, REG_CR, (rtw_read16(adapter, REG_CR) | BIT_MAC_SEC_EN));

#ifdef CONFIG_XMIT_ACK
	/* ack for xmit mgmt frames. */
	rtw_write32(adapter, REG_FWHW_TXQ_CTRL_8822B,
		rtw_read32(adapter, REG_FWHW_TXQ_CTRL_8822B) | BIT_EN_QUEUE_RPT_8822B(BIT(4)));
#endif /* CONFIG_XMIT_ACK */

	rtl8822b_init_beacon_variable(adapter);
}

u32 rtl8822b_init(PADAPTER adapter)
{
	u8 ok = _TRUE;
	PHAL_DATA_TYPE hal;

	hal = GET_HAL_DATA(adapter);

	ok = rtl8822b_hal_init(adapter);
	if (_FALSE == ok)
		return _FAIL;

	rtl8822b_phy_init_haldm(adapter);
#ifdef CONFIG_BEAMFORMING
	rtl8822b_phy_bf_init(adapter);
#endif

#ifdef CONFIG_BT_COEXIST
	/* Init BT hw config. */
	if (_TRUE == hal->EEPROMBluetoothCoexist)
		rtw_btcoex_HAL_Initialize(adapter, _FALSE);
	else
		rtw_btcoex_wifionly_hw_config(adapter);
#else /* CONFIG_BT_COEXIST */
	rtw_btcoex_wifionly_hw_config(adapter);
#endif /* CONFIG_BT_COEXIST */

	rtl8822b_init_misc(adapter);

	return _SUCCESS;
}

u32 rtl8822b_deinit(PADAPTER adapter)
{
	struct dvobj_priv *d;
	PHAL_DATA_TYPE hal;
	int err;


	d = adapter_to_dvobj(adapter);
	hal = GET_HAL_DATA(adapter);

	hal->bFWReady = _FALSE;
	hal->fw_ractrl = _FALSE;

	err = rtw_halmac_deinit_hal(d);
	if (err)
		return _FAIL;

	return _SUCCESS;
}

void rtl8822b_init_default_value(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	u8 i;


	hal = GET_HAL_DATA(adapter);

	if (adapter->registrypriv.wireless_mode == WIRELESS_MODE_MAX)
		adapter->registrypriv.wireless_mode = WIRELESS_MODE_24G | WIRELESS_MODE_5G;

	/* init default value */
	hal->fw_ractrl = _FALSE;

	if (!adapter_to_pwrctl(adapter)->bkeepfwalive)
		hal->LastHMEBoxNum = 0;

	/* init phydm default value */
	hal->bIQKInitialized = _FALSE;
	hal->odmpriv.rf_calibrate_info.tm_trigger = 0; /* for IQK */
	hal->odmpriv.rf_calibrate_info.thermal_value_hp_index = 0;
	for (i = 0; i < HP_THERMAL_NUM; i++)
		hal->odmpriv.rf_calibrate_info.thermal_value_hp[i] = 0;

	/* init Efuse variables */
	hal->EfuseUsedBytes = 0;
	hal->EfuseUsedPercentage = 0;

	hal->EfuseHal.fakeEfuseBank = 0;
	hal->EfuseHal.fakeEfuseUsedBytes = 0;
	_rtw_memset(hal->EfuseHal.fakeEfuseContent, 0xFF, EFUSE_MAX_HW_SIZE);
	_rtw_memset(hal->EfuseHal.fakeEfuseInitMap, 0xFF, EFUSE_MAX_MAP_LEN);
	_rtw_memset(hal->EfuseHal.fakeEfuseModifiedMap, 0xFF, EFUSE_MAX_MAP_LEN);
	hal->EfuseHal.BTEfuseUsedBytes = 0;
	hal->EfuseHal.BTEfuseUsedPercentage = 0;
	_rtw_memset(hal->EfuseHal.BTEfuseContent, 0xFF, EFUSE_MAX_BT_BANK * EFUSE_MAX_HW_SIZE);
	_rtw_memset(hal->EfuseHal.BTEfuseInitMap, 0xFF, EFUSE_BT_MAX_MAP_LEN);
	_rtw_memset(hal->EfuseHal.BTEfuseModifiedMap, 0xFF, EFUSE_BT_MAX_MAP_LEN);
	hal->EfuseHal.fakeBTEfuseUsedBytes = 0;
	_rtw_memset(hal->EfuseHal.fakeBTEfuseContent, 0xFF, EFUSE_MAX_BT_BANK * EFUSE_MAX_HW_SIZE);
	_rtw_memset(hal->EfuseHal.fakeBTEfuseInitMap, 0xFF, EFUSE_BT_MAX_MAP_LEN);
	_rtw_memset(hal->EfuseHal.fakeBTEfuseModifiedMap, 0xFF, EFUSE_BT_MAX_MAP_LEN);

}
