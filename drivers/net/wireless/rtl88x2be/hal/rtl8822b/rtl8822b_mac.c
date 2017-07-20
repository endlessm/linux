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
#define _RTL8822B_MAC_C_

#include <drv_types.h>		/* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* HAL_DATA_TYPE */
#include "../hal_halmac.h"	/* Register Definition and etc. */
#include "rtl8822b.h"		/* FW array */


inline u8 rtl8822b_rcr_config(PADAPTER p, u32 rcr)
{
	u32 v32;
	int err;


	v32 = GET_HAL_DATA(p)->ReceiveConfig;
	v32 ^= rcr;
	v32 &= BIT_APP_PHYSTS_8822B;
	if (v32) {
		v32 = rcr & BIT_APP_PHYSTS_8822B;
		RTW_INFO("%s: runtime %s rx phy status!\n",
			 __FUNCTION__, v32 ? "ENABLE" : "DISABLE");
		if (v32) {
			err = rtw_halmac_config_rx_info(adapter_to_dvobj(p), HALMAC_DRV_INFO_PHY_STATUS);
			if (err) {
				RTW_INFO("%s: Enable rx phy status FAIL!!", __FUNCTION__);
				rcr &= ~BIT_APP_PHYSTS_8822B;
			}
		} else {
			err = rtw_halmac_config_rx_info(adapter_to_dvobj(p), HALMAC_DRV_INFO_NONE);
			if (err) {
				RTW_INFO("%s: Disable rx phy status FAIL!!", __FUNCTION__);
				rcr |= BIT_APP_PHYSTS_8822B;
			}
		}
	}

	err = rtw_write32(p, REG_RCR_8822B, rcr);
	if (_FAIL == err)
		return _FALSE;

	GET_HAL_DATA(p)->ReceiveConfig = rcr;
	return _TRUE;
}

inline u8 rtl8822b_rcr_get(PADAPTER p, u32 *rcr)
{
	u32 v32;

	v32 = rtw_read32(p, REG_RCR_8822B);
	if (rcr)
		*rcr = v32;
	GET_HAL_DATA(p)->ReceiveConfig = v32;
	return _TRUE;
}

inline u8 rtl8822b_rcr_check(PADAPTER p, u32 check_bit)
{
	PHAL_DATA_TYPE hal;
	u32 rcr;

	hal = GET_HAL_DATA(p);
	rcr = hal->ReceiveConfig;
	if ((rcr & check_bit) == check_bit)
		return _TRUE;

	return _FALSE;
}

inline u8 rtl8822b_rcr_add(PADAPTER p, u32 add)
{
	PHAL_DATA_TYPE hal;
	u32 rcr;
	u8 ret = _TRUE;

	hal = GET_HAL_DATA(p);

	rcr = hal->ReceiveConfig;
	rcr |= add;
	if (rcr != hal->ReceiveConfig)
		ret = rtl8822b_rcr_config(p, rcr);

	return ret;
}

inline u8 rtl8822b_rcr_clear(PADAPTER p, u32 clear)
{
	PHAL_DATA_TYPE hal;
	u32 rcr;
	u8 ret = _TRUE;

	hal = GET_HAL_DATA(p);

	rcr = hal->ReceiveConfig;
	rcr &= ~clear;
	if (rcr != hal->ReceiveConfig)
		ret = rtl8822b_rcr_config(p, rcr);

	return ret;
}

inline u8 rtl8822b_rx_ba_ssn_appended(PADAPTER p)
{
	return rtl8822b_rcr_check(p, BIT_APP_BASSN_8822B);
}

inline u8 rtl8822b_rx_fcs_append_switch(PADAPTER p, u8 enable)
{
	u32 rcr_bit;
	u8 ret = _TRUE;

	rcr_bit = BIT_APP_FCS_8822B;
	if (_TRUE == enable)
		ret = rtl8822b_rcr_add(p, rcr_bit);
	else
		ret = rtl8822b_rcr_clear(p, rcr_bit);

	return ret;
}

inline u8 rtl8822b_rx_fcs_appended(PADAPTER p)
{
	return rtl8822b_rcr_check(p, BIT_APP_FCS_8822B);
}

inline u8 rtl8822b_rx_tsf_addr_filter_config(PADAPTER p, u8 config)
{
	u8 v8;
	int err;

	v8 = GET_HAL_DATA(p)->rx_tsf_addr_filter_config;

	if (v8 != config) {

		err = rtw_write8(p, REG_NAN_RX_TSF_FILTER_8822B, config);
		if (_FAIL == err)
			return _FALSE;
	}

	GET_HAL_DATA(p)->rx_tsf_addr_filter_config = config;
	return _TRUE;
}

/*
 * Return:
 *	_SUCCESS	Download Firmware OK.
 *	_FAIL		Download Firmware FAIL!
 */
s32 rtl8822b_fw_dl(PADAPTER adapter, u8 wowlan)
{
	struct dvobj_priv *d = adapter_to_dvobj(adapter);
	HAL_DATA_TYPE *hal = GET_HAL_DATA(adapter);
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);
	int err;
	u8 fw_bin = _TRUE;

#ifdef CONFIG_FILE_FWIMG
#ifdef CONFIG_WOWLAN
	if (wowlan)
		rtw_get_phy_file_path(adapter, MAC_FILE_FW_WW_IMG);
	else
#endif /* CONFIG_WOWLAN */
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
	if (_TRUE == fw_bin) {
		err = rtw_halmac_dlfw_from_file(d, rtw_phy_para_file_path);
	} else
#endif /* CONFIG_FILE_FWIMG */
	{
		#ifdef CONFIG_WOWLAN
		if (_TRUE == wowlan)
			err = rtw_halmac_dlfw(d, array_mp_8822b_fw_wowlan, array_length_mp_8822b_fw_wowlan);
		else
		#endif /* CONFIG_WOWLAN */
			err = rtw_halmac_dlfw(d, array_mp_8822b_fw_nic, array_length_mp_8822b_fw_nic);
	}

	if (!err) {
		hal->bFWReady = _TRUE;
		hal->fw_ractrl = _TRUE;
		RTW_INFO("%s Download Firmware from %s success\n", __FUNCTION__, (fw_bin) ? "file" : "array");
		RTW_INFO("%s FW Version:%d SubVersion:%d FW size:%d\n", (wowlan) ? "WOW" : "NIC",
			hal->firmware_version, hal->firmware_sub_version, hal->firmware_size);
		return _SUCCESS;
	} else {
		hal->bFWReady = _FALSE;
		hal->fw_ractrl = _FALSE;
		RTW_ERR("%s Download Firmware from %s failed\n", __FUNCTION__, (fw_bin) ? "file" : "array");
		return _FAIL;
	}
}
