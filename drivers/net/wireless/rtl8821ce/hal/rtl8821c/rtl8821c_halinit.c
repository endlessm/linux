/******************************************************************************
 *
 * Copyright(c) 2016 Realtek Corporation. All rights reserved.
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
#define _RTL8821C_HALINIT_C_

#include <drv_types.h>		/* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* GET_HAL_SPEC(), HAL_DATA_TYPE */
#include "../hal_halmac.h"	/* HALMAC API */
#include "rtl8821c.h"

void init_hal_spec_rtl8821c(PADAPTER adapter)
{
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(adapter);

	rtw_halmac_fill_hal_spec(adapter_to_dvobj(adapter), hal_spec);

	hal_spec->ic_name = "rtl8821c";
	hal_spec->macid_num = 128;
	/* hal_spec->sec_cam_ent_num follow halmac setting */
	hal_spec->sec_cap = SEC_CAP_CHK_BMC;

	hal_spec->rfpath_num_2g = 2;
	hal_spec->rfpath_num_5g = 1;
	hal_spec->max_tx_cnt = 1;
	hal_spec->tx_nss_num = 1;
	hal_spec->rx_nss_num = 1;
	hal_spec->band_cap = BAND_CAP_2G | BAND_CAP_5G;
	hal_spec->bw_cap = BW_CAP_20M | BW_CAP_40M | BW_CAP_80M;
	hal_spec->port_num = 5;
	hal_spec->hci_type = 0;
	hal_spec->proto_cap = PROTO_CAP_11B | PROTO_CAP_11G | PROTO_CAP_11N | PROTO_CAP_11AC;

	hal_spec->wl_func = 0
#ifdef CONFIG_P2P
			    | WL_FUNC_P2P
#ifdef CONFIG_WFD
			    | WL_FUNC_MIRACAST
#endif /* CONFIG_WFD */
#endif /* CONFIG_P2P */
#ifdef CONFIG_TDLS
			    | WL_FUNC_TDLS
#endif /* CONFIG_TDLS */
			    ;
}

u32 rtl8821c_power_on(PADAPTER adapter)
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

	#ifdef CONFIG_POWER_STATE_UNEXPECTED_HDL
	if ((-2) == err) {
		RTW_ERR("%s:Power ON Fail, Try to power on again !!\n", __FUNCTION__);
		rtw_halmac_poweroff(d);
		rtw_msleep_os(2);
		err = rtw_halmac_poweron(d);
	}
	#endif

	if (err) {
		RTW_ERR("%s: Power ON Fail!!\n", __FUNCTION__);
		rtw_warn_on(1);
		ret = _FAIL;
		goto out;
	}

	bMacPwrCtrlOn = _TRUE;
	rtw_hal_set_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

out:
	return ret;
}

void rtl8821c_power_off(PADAPTER adapter)
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
		rtw_warn_on(1);
		goto out;
	}

	bMacPwrCtrlOn = _FALSE;
	rtw_hal_set_hwreg(adapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

	adapter->bFWReady = _FALSE;

out:
	return;
}

u8 rtl8821c_hal_init_main(PADAPTER adapter)
{
	struct dvobj_priv *d = adapter_to_dvobj(adapter);
	PHAL_DATA_TYPE hal = GET_HAL_DATA(adapter);
	int err;
	s32 ret;

	adapter->bFWReady = _FALSE;
	hal->fw_ractrl = _FALSE;

#ifdef CONFIG_NO_FW
	err = rtw_halmac_init_hal(d);
#else
	#ifdef CONFIG_FILE_FWIMG
	rtw_get_phy_file_path(adapter, MAC_FILE_FW_NIC);
	err = rtw_halmac_init_hal_fw_file(d, rtw_phy_para_file_path);
	#else
	err = rtw_halmac_init_hal_fw(d, array_mp_8821c_fw_nic, array_length_mp_8821c_fw_nic);
	#endif

	if (!err) {
		adapter->bFWReady = _TRUE;
		hal->fw_ractrl = _TRUE;
	}
	RTW_INFO("FW Version:%d SubVersion:%d\n", hal->firmware_version, hal->firmware_sub_version);
#endif
	if (err) {
		RTW_INFO("%s: fail\n", __FUNCTION__);
		return _FALSE;
	}

	RTW_INFO("%s: successful\n", __FUNCTION__);

	return _TRUE;
}

u8 rtl8821c_mac_verify(PADAPTER adapter)
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

void rtl8821c_hal_init_channel_setting(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal_data = GET_HAL_DATA(adapter);

	/* initial channel setting */
	hal_data->current_channel = 0;
	hal_data->current_channel_bw = CHANNEL_WIDTH_MAX;
	hal_data->current_band_type = BAND_MAX;
}

void rtl8821c_hal_init_misc(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal_data = GET_HAL_DATA(adapter);
	u8 drv_info_sz = 0;

	/*
	 * Sync driver status and hardware setting
	 */

	/* initial security setting */
	invalidate_cam_all(adapter);

	/* enable to rx ps-poll ,disable Control frame filter*/
	rtw_write16(adapter, REG_RXFLTMAP1_8821C, 0x0400);
	/* Accept all data frames */
	rtw_write16(adapter, REG_RXFLTMAP_8821C, 0xFFFF);

	/* Accept all management frames */
	rtw_write16(adapter, REG_RXFLTMAP0_8821C, 0xFFFF);

	/*RCR setting - Sync driver status with hardware setting */
	rtl8821c_rcr_get(adapter, NULL);

	rtl8821c_rcr_clear(adapter, BIT_AICV_8821C | BIT_ACRC32_8821C | BIT_APP_FCS_8821C | BIT_APWRMGT_8821C);

	rtw_halmac_get_drv_info_sz(adapter_to_dvobj(adapter), &drv_info_sz);
	if (drv_info_sz)
		rtl8821c_rcr_add(adapter, BIT_APP_PHYSTS_8821C);

#ifdef CONFIG_RX_PACKET_APPEND_FCS
	rtl8821c_rcr_add(adapter, BIT_APP_FCS_8821C);
#endif

#ifdef CONFIG_RX_PACKET_APPEND_ICV_ERROR
	rtl8821c_rcr_add(adapter, BIT_AICV_8821C);
#endif

#ifdef CONFIG_XMIT_ACK
	rtl8821c_set_mgnt_xmit_ack(adapter);
#endif /*CONFIG_XMIT_ACK*/

	/*Disable BAR, suggested by Scott */
	rtw_write32(adapter, REG_BAR_MODE_CTRL_8821C, 0x0201ffff);
	/*Disable secondary CCA 20M,40M?*/
	rtw_write8(adapter, REG_MISC_CTRL_8821C, 0x03);

	/*Enable MAC security engine*/
	rtw_write16(adapter, REG_CR, (rtw_read16(adapter, REG_CR) | BIT_MAC_SEC_EN));

	rtl8821c_rx_tsf_addr_filter_config(adapter, BIT_CHK_TSF_EN_8821C | BIT_CHK_TSF_CBSSID_8821C);

	/*for 1212 module - 5G RX issue*/
	if (hal_data->rfe_type == 2)
		rtw_write8(adapter, REG_PAD_CTRL1 + 3, 0x36);

#ifdef CONFIG_CHECK_AC_LIFETIME
	/* Enable lifetime check for the four ACs */
	rtw_write8(adapter, REG_LIFETIME_EN_8821C, rtw_read8(adapter, REG_LIFETIME_EN_8821C) | 0x0f);
#endif /* CONFIG_CHECK_AC_LIFETIME */

#if defined(CONFIG_CONCURRENT_MODE) || defined(CONFIG_TX_MCAST2UNI)
#ifdef CONFIG_TX_MCAST2UNI
	rtw_write16(adapter, REG_PKT_LIFE_TIME_8821C, 0x0400);	/* unit: 256us. 256ms */
	rtw_write16(adapter, REG_PKT_LIFE_TIME_8821C+2, 0x0400);	/* unit: 256us. 256ms */
#else	/* CONFIG_TX_MCAST2UNI */
	rtw_write16(adapter, REG_PKT_LIFE_TIME_8821C, 0x3000);	/* unit: 256us. 3s */
	rtw_write16(adapter, REG_PKT_LIFE_TIME_8821C+2, 0x3000);	/* unit: 256us. 3s */
#endif /* CONFIG_TX_MCAST2UNI */
#endif /* CONFIG_CONCURRENT_MODE || CONFIG_TX_MCAST2UNI */

}

u32 rtl8821c_hal_init(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;

	hal = GET_HAL_DATA(adapter);

	if (_FALSE == rtl8821c_hal_init_main(adapter))
		return _FAIL;

	rtl8821c_hal_init_misc(adapter);

	rtl8821c_phy_init_haldm(adapter);

#ifdef CONFIG_BT_COEXIST
	/* Init BT hw config. */
	if (_TRUE == hal->EEPROMBluetoothCoexist)
		rtw_btcoex_HAL_Initialize(adapter, _FALSE);
	else
		rtw_btcoex_wifionly_hw_config(adapter);
#else /* CONFIG_BT_COEXIST */
	rtw_btcoex_wifionly_hw_config(adapter);
#endif /* CONFIG_BT_COEXIST */

	rtl8821c_hal_init_channel_setting(adapter);

	return _SUCCESS;
}

u32 rtl8821c_hal_deinit(PADAPTER adapter)
{
	struct dvobj_priv *d;
	PHAL_DATA_TYPE hal;
	int err;


	d = adapter_to_dvobj(adapter);
	hal = GET_HAL_DATA(adapter);

	adapter->bFWReady = _FALSE;
	hal->fw_ractrl = _FALSE;

	err = rtw_halmac_deinit_hal(d);
	if (err)
		return _FAIL;

	return _SUCCESS;
}

void rtl8821c_init_default_value(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	u8 i;


	hal = GET_HAL_DATA(adapter);

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
#ifdef HAL_EFUSE_MEMORY
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
#endif
}
