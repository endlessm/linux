/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#include "halmac_88xx_cfg.h"
#include "halmac_gpio_88xx.h"

/**
 * halmac_init_adapter_para_88xx() - int halmac adapter
 * @pHalmac_adapter
 *
 * SD1 internal use
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : VOID
 */
VOID
halmac_init_adapter_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	pHalmac_adapter->pHalAdapter_backup = pHalmac_adapter;
	pHalmac_adapter->pHalEfuse_map = (u8 *)NULL;
	pHalmac_adapter->hal_efuse_map_valid = _FALSE;
	pHalmac_adapter->efuse_end = 0;
	pHalmac_adapter->pHal_mac_addr[0].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_mac_addr[0].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_mac_addr[1].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_mac_addr[1].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_bss_addr[0].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_bss_addr[0].Address_L_H.Address_High = 0;
	pHalmac_adapter->pHal_bss_addr[1].Address_L_H.Address_Low = 0;
	pHalmac_adapter->pHal_bss_addr[1].Address_L_H.Address_High = 0;

	pHalmac_adapter->low_clk = _FALSE;
	pHalmac_adapter->max_download_size = HALMAC_FW_MAX_DL_SIZE_88XX;

	pHalmac_adapter->config_para_info.pCfg_para_buf = NULL;
	pHalmac_adapter->config_para_info.pPara_buf_w = NULL;
	pHalmac_adapter->config_para_info.para_num = 0;
	pHalmac_adapter->config_para_info.full_fifo_mode = _FALSE;
	pHalmac_adapter->config_para_info.para_buf_size = 0;
	pHalmac_adapter->config_para_info.avai_para_buf_size = 0;
	pHalmac_adapter->config_para_info.offset_accumulation = 0;
	pHalmac_adapter->config_para_info.value_accumulation = 0;
	pHalmac_adapter->config_para_info.datapack_segment = 0;

	pHalmac_adapter->ch_sw_info.ch_info_buf = NULL;
	pHalmac_adapter->ch_sw_info.ch_info_buf_w = NULL;
	pHalmac_adapter->ch_sw_info.extra_info_en = 0;
	pHalmac_adapter->ch_sw_info.buf_size = 0;
	pHalmac_adapter->ch_sw_info.avai_buf_size = 0;
	pHalmac_adapter->ch_sw_info.total_size = 0;
	pHalmac_adapter->ch_sw_info.ch_num = 0;

	pHalmac_adapter->drv_info_size = 0;

	pHalmac_adapter->txff_allocation.tx_fifo_pg_num = 0;
	pHalmac_adapter->txff_allocation.ac_q_pg_num = 0;
	pHalmac_adapter->txff_allocation.rsvd_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_h2c_queue_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_cpu_instr_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.rsvd_fw_txbuff_pg_bndy = 0;
	pHalmac_adapter->txff_allocation.pub_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.high_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.low_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.normal_queue_pg_num = 0;
	pHalmac_adapter->txff_allocation.extra_queue_pg_num = 0;

	pHalmac_adapter->txff_allocation.la_mode = HALMAC_LA_MODE_DISABLE;
	pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode = HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE;

	pHalmac_adapter->hw_config_info.ac_oqt_size = HALMAC_OQT_ENTRY_AC_88XX;
	pHalmac_adapter->hw_config_info.non_ac_oqt_size = HALMAC_OQT_ENTRY_NOAC_88XX;
	pHalmac_adapter->hw_config_info.ac_queue_num = 8;

	pHalmac_adapter->sdio_cmd53_4byte = HALMAC_SDIO_CMD53_4BYTE_MODE_DISABLE;
	pHalmac_adapter->sdio_hw_info.io_hi_speed_flag = 0;
	pHalmac_adapter->sdio_hw_info.spec_ver = HALMAC_SDIO_SPEC_VER_2_00;
	pHalmac_adapter->sdio_hw_info.clock_speed = 50;

	pHalmac_adapter->pinmux_info.wl_led = 0;
	pHalmac_adapter->pinmux_info.sdio_int = 0;
	pHalmac_adapter->pinmux_info.sw_io_0 = 0;
	pHalmac_adapter->pinmux_info.sw_io_1 = 0;
	pHalmac_adapter->pinmux_info.sw_io_2 = 0;
	pHalmac_adapter->pinmux_info.sw_io_3 = 0;
	pHalmac_adapter->pinmux_info.sw_io_4 = 0;
	pHalmac_adapter->pinmux_info.sw_io_5 = 0;
	pHalmac_adapter->pinmux_info.sw_io_6 = 0;
	pHalmac_adapter->pinmux_info.sw_io_7 = 0;
	pHalmac_adapter->pinmux_info.sw_io_8 = 0;
	pHalmac_adapter->pinmux_info.sw_io_9 = 0;
	pHalmac_adapter->pinmux_info.sw_io_10 = 0;
	pHalmac_adapter->pinmux_info.sw_io_11 = 0;
	pHalmac_adapter->pinmux_info.sw_io_12 = 0;
	pHalmac_adapter->pinmux_info.sw_io_13 = 0;
	pHalmac_adapter->pinmux_info.sw_io_14 = 0;
	pHalmac_adapter->pinmux_info.sw_io_15 = 0;

	halmac_init_adapter_dynamic_para_88xx(pHalmac_adapter);
	halmac_init_state_machine_88xx(pHalmac_adapter);
}

/**
 * halmac_init_adapter_dynamic_para_88xx() - int halmac adapter
 * @pHalmac_adapter
 *
 * SD1 internal use
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : VOID
 */
VOID
halmac_init_adapter_dynamic_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	pHalmac_adapter->h2c_packet_seq = 0;
	pHalmac_adapter->h2c_buf_free_space = 0;
	pHalmac_adapter->gen_info_valid = _FALSE;
}

/**
 * halmac_init_state_machine_88xx() - init halmac software state machine
 * @pHalmac_adapter
 *
 * SD1 internal use.
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : VOID
 */
VOID
halmac_init_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	PHALMAC_STATE pState = &pHalmac_adapter->halmac_state;

	halmac_init_offload_feature_state_machine_88xx(pHalmac_adapter);

	pState->api_state = HALMAC_API_STATE_INIT;

	pState->dlfw_state = HALMAC_DLFW_NONE;
	pState->mac_power = HALMAC_MAC_POWER_OFF;
	pState->ps_state = HALMAC_PS_STATE_UNDEFINE;
	pState->gpio_cfg_state = HALMAC_GPIO_CFG_STATE_IDLE;
}

/**
 * halmac_mount_api_88xx() - attach functions to function pointer
 * @pHalmac_adapter
 *
 * SD1 internal use
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_mount_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	PHALMAC_API pHalmac_api = (PHALMAC_API)NULL;

	pHalmac_adapter->pHalmac_api = (PHALMAC_API)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(HALMAC_API));
	if (pHalmac_adapter->pHalmac_api == NULL)
		return HALMAC_RET_MALLOC_FAIL;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, HALMAC_SVN_VER_88XX"\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MAJOR_VER_88XX = %x\n", HALMAC_MAJOR_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PROTOTYPE_88XX = %x\n", HALMAC_PROTOTYPE_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_MINOR_VER_88XX = %x\n", HALMAC_MINOR_VER_88XX);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ALWAYS, "HALMAC_PATCH_VER_88XX = %x\n", HALMAC_PATCH_VER_88XX);

	/* Mount function pointer */
	pHalmac_api->halmac_download_firmware = halmac_download_firmware_88xx;
	pHalmac_api->halmac_free_download_firmware = halmac_free_download_firmware_88xx;
	pHalmac_api->halmac_get_fw_version = halmac_get_fw_version_88xx;
	pHalmac_api->halmac_cfg_mac_addr = halmac_cfg_mac_addr_88xx;
	pHalmac_api->halmac_cfg_bssid = halmac_cfg_bssid_88xx;
	pHalmac_api->halmac_cfg_multicast_addr = halmac_cfg_multicast_addr_88xx;
	pHalmac_api->halmac_pre_init_system_cfg = halmac_pre_init_system_cfg_88xx;
	pHalmac_api->halmac_init_system_cfg = halmac_init_system_cfg_88xx;
	pHalmac_api->halmac_init_edca_cfg = halmac_init_edca_cfg_88xx;
	pHalmac_api->halmac_cfg_operation_mode = halmac_cfg_operation_mode_88xx;
	pHalmac_api->halmac_cfg_ch_bw = halmac_cfg_ch_bw_88xx;
	pHalmac_api->halmac_cfg_bw = halmac_cfg_bw_88xx;
	pHalmac_api->halmac_init_wmac_cfg = halmac_init_wmac_cfg_88xx;
	pHalmac_api->halmac_init_mac_cfg = halmac_init_mac_cfg_88xx;
	pHalmac_api->halmac_init_sdio_cfg = halmac_init_sdio_cfg_88xx;
	pHalmac_api->halmac_init_usb_cfg = halmac_init_usb_cfg_88xx;
	pHalmac_api->halmac_init_pcie_cfg = halmac_init_pcie_cfg_88xx;
	pHalmac_api->halmac_deinit_sdio_cfg = halmac_deinit_sdio_cfg_88xx;
	pHalmac_api->halmac_deinit_usb_cfg = halmac_deinit_usb_cfg_88xx;
	pHalmac_api->halmac_deinit_pcie_cfg = halmac_deinit_pcie_cfg_88xx;
	pHalmac_api->halmac_dump_efuse_map = halmac_dump_efuse_map_88xx;
	pHalmac_api->halmac_dump_efuse_map_bt = halmac_dump_efuse_map_bt_88xx;
	pHalmac_api->halmac_write_efuse_bt = halmac_write_efuse_bt_88xx;
	pHalmac_api->halmac_read_efuse_bt = halmac_read_efuse_bt_88xx;
	pHalmac_api->halmac_cfg_efuse_auto_check = halmac_cfg_efuse_auto_check_88xx;
	pHalmac_api->halmac_dump_logical_efuse_map = halmac_dump_logical_efuse_map_88xx;
	pHalmac_api->halmac_pg_efuse_by_map = halmac_pg_efuse_by_map_88xx;
	pHalmac_api->halmac_get_efuse_size = halmac_get_efuse_size_88xx;
	pHalmac_api->halmac_get_efuse_available_size = halmac_get_efuse_available_size_88xx;
	pHalmac_api->halmac_get_c2h_info = halmac_get_c2h_info_88xx;

	pHalmac_api->halmac_get_logical_efuse_size = halmac_get_logical_efuse_size_88xx;

	pHalmac_api->halmac_write_logical_efuse = halmac_write_logical_efuse_88xx;
	pHalmac_api->halmac_read_logical_efuse = halmac_read_logical_efuse_88xx;

	pHalmac_api->halmac_h2c_lb = halmac_h2c_lb_88xx;
	pHalmac_api->halmac_debug = halmac_debug_88xx;
	pHalmac_api->halmac_cfg_parameter = halmac_cfg_parameter_88xx;
	pHalmac_api->halmac_update_datapack = halmac_update_datapack_88xx;
	pHalmac_api->halmac_run_datapack = halmac_run_datapack_88xx;
	pHalmac_api->halmac_cfg_drv_info = halmac_cfg_drv_info_88xx;
	pHalmac_api->halmac_send_bt_coex = halmac_send_bt_coex_88xx;
	pHalmac_api->halmac_verify_platform_api = halmac_verify_platform_api_88xx;
	pHalmac_api->halmac_update_packet = halmac_update_packet_88xx;
	pHalmac_api->halmac_bcn_ie_filter = halmac_bcn_ie_filter_88xx;
	pHalmac_api->halmac_cfg_txbf = halmac_cfg_txbf_88xx;
	pHalmac_api->halmac_cfg_mumimo = halmac_cfg_mumimo_88xx;
	pHalmac_api->halmac_cfg_sounding = halmac_cfg_sounding_88xx;
	pHalmac_api->halmac_del_sounding = halmac_del_sounding_88xx;
	pHalmac_api->halmac_su_bfer_entry_init = halmac_su_bfer_entry_init_88xx;
	pHalmac_api->halmac_su_bfee_entry_init = halmac_su_bfee_entry_init_88xx;
	pHalmac_api->halmac_mu_bfer_entry_init = halmac_mu_bfer_entry_init_88xx;
	pHalmac_api->halmac_mu_bfee_entry_init = halmac_mu_bfee_entry_init_88xx;
	pHalmac_api->halmac_su_bfer_entry_del = halmac_su_bfer_entry_del_88xx;
	pHalmac_api->halmac_su_bfee_entry_del = halmac_su_bfee_entry_del_88xx;
	pHalmac_api->halmac_mu_bfer_entry_del = halmac_mu_bfer_entry_del_88xx;
	pHalmac_api->halmac_mu_bfee_entry_del = halmac_mu_bfee_entry_del_88xx;

	pHalmac_api->halmac_add_ch_info = halmac_add_ch_info_88xx;
	pHalmac_api->halmac_add_extra_ch_info = halmac_add_extra_ch_info_88xx;
	pHalmac_api->halmac_ctrl_ch_switch = halmac_ctrl_ch_switch_88xx;
	pHalmac_api->halmac_p2pps = halmac_p2pps_88xx;
	pHalmac_api->halmac_clear_ch_info = halmac_clear_ch_info_88xx;
	pHalmac_api->halmac_send_general_info = halmac_send_general_info_88xx;

	pHalmac_api->halmac_start_iqk = halmac_start_iqk_88xx;
	pHalmac_api->halmac_ctrl_pwr_tracking = halmac_ctrl_pwr_tracking_88xx;
	pHalmac_api->halmac_psd = halmac_psd_88xx;
	pHalmac_api->halmac_cfg_la_mode = halmac_cfg_la_mode_88xx;
	pHalmac_api->halmac_cfg_rx_fifo_expanding_mode = halmac_cfg_rx_fifo_expanding_mode_88xx;

	pHalmac_api->halmac_config_security = halmac_config_security_88xx;
	pHalmac_api->halmac_get_used_cam_entry_num = halmac_get_used_cam_entry_num_88xx;
	pHalmac_api->halmac_read_cam_entry = halmac_read_cam_entry_88xx;
	pHalmac_api->halmac_write_cam = halmac_write_cam_88xx;
	pHalmac_api->halmac_clear_cam_entry = halmac_clear_cam_entry_88xx;

	pHalmac_api->halmac_get_hw_value = halmac_get_hw_value_88xx;
	pHalmac_api->halmac_set_hw_value = halmac_set_hw_value_88xx;

	pHalmac_api->halmac_cfg_drv_rsvd_pg_num = halmac_cfg_drv_rsvd_pg_num_88xx;
	pHalmac_api->halmac_get_chip_version = halmac_get_chip_version_88xx;

	pHalmac_api->halmac_query_status = halmac_query_status_88xx;
	pHalmac_api->halmac_reset_feature = halmac_reset_feature_88xx;
	pHalmac_api->halmac_check_fw_status = halmac_check_fw_status_88xx;
	pHalmac_api->halmac_dump_fw_dmem = halmac_dump_fw_dmem_88xx;
	pHalmac_api->halmac_cfg_max_dl_size = halmac_cfg_max_dl_size_88xx;

	pHalmac_api->halmac_dump_fifo = halmac_dump_fifo_88xx;
	pHalmac_api->halmac_get_fifo_size = halmac_get_fifo_size_88xx;

	pHalmac_api->halmac_chk_txdesc = halmac_chk_txdesc_88xx;
	pHalmac_api->halmac_dl_drv_rsvd_page = halmac_dl_drv_rsvd_page_88xx;
	pHalmac_api->halmac_cfg_csi_rate = halmac_cfg_csi_rate_88xx;

	pHalmac_api->halmac_sdio_cmd53_4byte = halmac_sdio_cmd53_4byte_88xx;
	pHalmac_api->halmac_sdio_hw_info = halmac_sdio_hw_info_88xx;
	pHalmac_api->halmac_txfifo_is_empty = halmac_txfifo_is_empty_88xx;
	pHalmac_api->halmac_download_flash = halmac_download_flash_88xx;
	pHalmac_api->halmac_read_flash = halmac_read_flash_88xx;
	pHalmac_api->halmac_erase_flash = halmac_erase_flash_88xx;
	pHalmac_api->halmac_check_flash = halmac_check_flash_88xx;
	pHalmac_api->halmac_cfg_edca_para = halmac_cfg_edca_para_88xx;
	pHalmac_api->halmac_pinmux_wl_led_mode = halmac_pinmux_wl_led_mode_88xx;
	pHalmac_api->halmac_pinmux_wl_led_sw_ctrl = halmac_pinmux_wl_led_sw_ctrl_88xx;
	pHalmac_api->halmac_pinmux_sdio_int_polarity = halmac_pinmux_sdio_int_polarity_88xx;
	pHalmac_api->halmac_pinmux_gpio_mode = halmac_pinmux_gpio_mode_88xx;
	pHalmac_api->halmac_pinmux_gpio_output = halmac_pinmux_gpio_output_88xx;
	pHalmac_api->halmac_pinmux_pin_status = halmac_pinmux_pin_status_88xx;

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_sdio;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_sdio_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_sdio_cfg_88xx;
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_sdio_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_sdio_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_sdio_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_sdio_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_sdio_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_sdio_88xx;
		pHalmac_api->halmac_reg_read_indirect_32 = halmac_reg_read_indirect_32_sdio_88xx;
		pHalmac_api->halmac_reg_sdio_cmd53_read_n = halmac_reg_read_nbyte_sdio_88xx;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_usb;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_usb_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_usb_cfg_88xx;
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_usb_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_usb_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_usb_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_usb_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_usb_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_usb_88xx;
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_PCIE) {
		pHalmac_api->halmac_cfg_rx_aggregation = halmac_cfg_rx_aggregation_88xx_pcie;
		pHalmac_api->halmac_init_interface_cfg = halmac_init_pcie_cfg_88xx;
		pHalmac_api->halmac_deinit_interface_cfg = halmac_deinit_pcie_cfg_88xx;
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_pcie_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_pcie_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_pcie_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_pcie_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_pcie_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_pcie_88xx;
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "Set halmac io function Error!!\n");
	}

	pHalmac_api->halmac_set_bulkout_num = halmac_set_bulkout_num_88xx;
	pHalmac_api->halmac_get_sdio_tx_addr = halmac_get_sdio_tx_addr_88xx;
	pHalmac_api->halmac_get_usb_bulkout_id = halmac_get_usb_bulkout_id_88xx;
	pHalmac_api->halmac_timer_2s = halmac_timer_2s_88xx;
	pHalmac_api->halmac_fill_txdesc_checksum = halmac_fill_txdesc_check_sum_88xx;


	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B) {
#if HALMAC_8822B_SUPPORT
		/*mount 8822b function and data*/
		halmac_mount_api_8822b(pHalmac_adapter);
#endif

	} else if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C) {
#if HALMAC_8821C_SUPPORT
		/*mount 8822b function and data*/
		halmac_mount_api_8821c(pHalmac_adapter);
#endif
	} else {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "Chip ID undefine!!\n");
		return HALMAC_RET_CHIP_NOT_SUPPORT;
	}
#if HALMAC_PLATFORM_TESTPROGRAM
	pHalmac_api->halmac_write_efuse = halmac_write_efuse_88xx;
	pHalmac_api->halmac_read_efuse = halmac_read_efuse_88xx;
	pHalmac_api->halmac_switch_efuse_bank = halmac_switch_efuse_bank_88xx;

	pHalmac_api->halmac_gen_txdesc = halmac_gen_tx_desc_88xx;
	pHalmac_api->halmac_txdesc_parser = halmac_tx_desc_parser_88xx;
	pHalmac_api->halmac_rxdesc_parser = halmac_rx_desc_parser_88xx;
	pHalmac_api->halmac_get_txdesc_size = halmac_get_txdesc_size_88xx;
	pHalmac_api->halmac_send_packet = halmac_send_packet_88xx;
	pHalmac_api->halmac_parse_packet = halmac_parse_packet_88xx;

	pHalmac_api->halmac_get_pcie_packet = halmac_get_pcie_packet_88xx;
	pHalmac_api->halmac_gen_txagg_desc = halmac_gen_txagg_desc_88xx;

	pHalmac_api->halmac_bb_reg_read = halmac_bb_reg_read_88xx;
	pHalmac_api->halmac_bb_reg_write = halmac_bb_reg_write_88xx;

	pHalmac_api->halmac_rf_reg_read = halmac_rf_ac_reg_read_88xx;
	pHalmac_api->halmac_rf_reg_write = halmac_rf_ac_reg_write_88xx;
	pHalmac_api->halmac_init_antenna_selection = halmac_init_antenna_selection_88xx;
	pHalmac_api->halmac_bb_preconfig = halmac_bb_preconfig_88xx;
	pHalmac_api->halmac_init_crystal_capacity = halmac_init_crystal_capacity_88xx;
	pHalmac_api->halmac_trx_antenna_setting = halmac_trx_antenna_setting_88xx;

	pHalmac_api->halmac_himr_setting_sdio = halmac_himr_setting_88xx_sdio;

	pHalmac_api->halmac_send_beacon = halmac_send_beacon_88xx;
	pHalmac_api->halmac_stop_beacon = halmac_stop_beacon_88xx;
	pHalmac_api->halmac_check_trx_status = halmac_check_trx_status_88xx;
	pHalmac_api->halmac_set_agg_num = halmac_set_agg_num_88xx;
	pHalmac_api->halmac_get_management_txdesc = halmac_get_management_txdesc_88xx;
	pHalmac_api->halmac_send_control = halmac_send_control_88xx;
	pHalmac_api->halmac_send_hiqueue = halmac_send_hiqueue_88xx;

	pHalmac_api->halmac_timer_10ms = halmac_timer_10ms_88xx;

	pHalmac_api->halmac_download_firmware_fpag = halmac_download_firmware_fpga_88xx;
	pHalmac_api->halmac_download_rom_fpga = halmac_download_rom_fpga_88xx;
	pHalmac_api->halmac_send_nlo = halmac_send_nlo_88xx;

	pHalmac_api->halmac_dump_cam_table = halmac_dump_cam_table_88xx;
	pHalmac_api->halmac_load_cam_table = halmac_load_cam_table_88xx;

	pHalmac_api->halmac_get_chip_type = halmac_get_chip_type_88xx;

	pHalmac_api->halmac_get_rx_agg_num = halmac_get_rx_agg_num_88xx;
	pHalmac_api->halmac_check_rx_scsi_resp = halmac_check_rx_scsi_resp_88xx_usb;

	pHalmac_api->halmac_get_hcpwm = halmac_get_hcpwm_88xx;
	pHalmac_api->halmac_get_hcpwm2 = halmac_get_hcpwm2_88xx;
	pHalmac_api->halmac_set_hrpwm = halmac_set_hrpwm_88xx;
	pHalmac_api->halmac_set_hrpwm2 = halmac_set_hrpwm2_88xx;
	pHalmac_api->halmac_coex_cfg = halmac_coex_cfg_88xx;
#if HALMAC_8822B_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B)
		pHalmac_api->halmac_run_pwrseq = halmac_run_pwrseq_8822b;
#endif
#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C)
		pHalmac_api->halmac_run_pwrseq = halmac_run_pwrseq_8821c;
#endif
	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		pHalmac_api->halmac_reg_read_8 = halmac_reg_read_8_sdio_88xx;
		pHalmac_api->halmac_reg_write_8 = halmac_reg_write_8_sdio_88xx;
		pHalmac_api->halmac_reg_read_16 = halmac_reg_read_16_sdio_88xx;
		pHalmac_api->halmac_reg_write_16 = halmac_reg_write_16_sdio_88xx;
		pHalmac_api->halmac_reg_read_32 = halmac_reg_read_32_sdio_88xx;
		pHalmac_api->halmac_reg_write_32 = halmac_reg_write_32_sdio_88xx;
	}
#endif
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_download_firmware_88xx() - download Firmware
 * @pHalmac_adapter : the adapter of halmac
 * @pHamacl_fw : firmware bin
 * @halmac_fw_size : firmware size
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
)
{
	u8 value8;
	u8 *pFile_ptr;
	u16 value16;
	u32 restore_index = 0;
	u16 halmac_h2c_ver = 0, fw_h2c_ver = 0;
	u32 iram_pkt_size, dmem_pkt_size, eram_pkt_size = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RESTORE_INFO restore_info[DLFW_RESTORE_REG_NUM_88XX];

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_firmware_88xx ==========>\n");

	if (halmac_fw_size > HALMAC_FW_SIZE_MAX_88XX || halmac_fw_size < HALMAC_FWHDR_SIZE_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "FW size error!\n");
		return HALMAC_RET_FW_SIZE_ERR;
	}

	fw_h2c_ver = rtk_le16_to_cpu(*((u16 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_H2C_FORMAT_VER_88XX)));
	halmac_h2c_ver = H2C_FORMAT_VERSION;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac h2c/c2h format = %x, fw h2c/c2h format = %x!!\n", halmac_h2c_ver, fw_h2c_ver);
	if (fw_h2c_ver != halmac_h2c_ver)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]H2C/C2H version between HALMAC and FW is compatible!!\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1);
	value8 = (u8)(value8 & ~(BIT(2)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, value8); /* Disable CPU reset */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_RSV_CTRL + 1);
	value8 = (u8)(value8 & ~(BIT(0)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RSV_CTRL + 1, value8);

	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_TXDMA_PQ_MAP + 1;
	restore_info[restore_index].value = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1);
	restore_index++;
	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);  /* set HIQ to hi priority */

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_CR;
	restore_info[restore_index].value = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR);
	restore_index++;
	restore_info[restore_index].length = 4;
	restore_info[restore_index].mac_register = REG_H2CQ_CSR;
	restore_info[restore_index].value = BIT(31);
	restore_index++;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	restore_info[restore_index].length = 2;
	restore_info[restore_index].mac_register = REG_FIFOPAGE_INFO_1;
	restore_info[restore_index].value = HALMAC_REG_READ_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1);
	restore_index++;
	restore_info[restore_index].length = 4;
	restore_info[restore_index].mac_register = REG_RQPN_CTRL_2;
	restore_info[restore_index].value = HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31);
	restore_index++;
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, restore_info[restore_index - 1].value);

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO)
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);

	halmac_update_fw_info_88xx(pHalmac_adapter, pHamacl_fw, halmac_fw_size);

	dmem_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_SIZE_88XX));
	iram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_SIZE_88XX));
	if (0 != ((*(pHamacl_fw + HALMAC_FWHDR_OFFSET_MEM_USAGE_88XX)) & BIT(4)))
		eram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_ERAM_SIZE_88XX));

	dmem_pkt_size = rtk_le32_to_cpu(dmem_pkt_size);
	iram_pkt_size = rtk_le32_to_cpu(iram_pkt_size);
	eram_pkt_size = rtk_le32_to_cpu(eram_pkt_size);

	dmem_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	iram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	if (eram_pkt_size != 0)
		eram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;

	if (halmac_fw_size != (HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size + eram_pkt_size)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "FW size mismatch the real fw size!\n");
		goto DLFW_FAIL;
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_CR + 1;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8); /* Enable SW TX beacon */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_BCN_CTRL;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)((value8 & (~BIT(3))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8); /* Disable beacon related functions */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore_info[restore_index].length = 1;
	restore_info[restore_index].mac_register = REG_FWHW_TXQ_CTRL + 2;
	restore_info[restore_index].value = value8;
	restore_index++;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8); /* Disable ptcl tx bcnq */

	restore_info[restore_index].length = 2;
	restore_info[restore_index].mac_register = REG_FIFOPAGE_CTRL_2;
	restore_info[restore_index].value = HALMAC_REG_READ_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2) | BIT(15);
	restore_index++;
	value16 = 0x8000;
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, value16); /* Set beacon header to  0 */

	value16 = (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_MCUFW_CTRL) & 0x3800);
	value16 |= BIT(0);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MCUFW_CTRL, value16); /* MCU/FW setting */

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2);
	value8 &= ~(BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CPU_DMEM_CON + 2, value8);

	pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX;
	if (halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr,
		    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_ADDR_88XX))) & ~(BIT(31)), dmem_pkt_size) != HALMAC_RET_SUCCESS)
		goto DLFW_END;

	pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size;
	if (halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr,
		    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_ADDR_88XX))) & ~(BIT(31)), iram_pkt_size) != HALMAC_RET_SUCCESS)
		goto DLFW_END;

	if (eram_pkt_size != 0) {
		pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size;
		if (halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr,
			    rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_EMEM_ADDR_88XX))) & ~(BIT(31)), eram_pkt_size) != HALMAC_RET_SUCCESS)
			goto DLFW_END;
	}

	halmac_init_offload_feature_state_machine_88xx(pHalmac_adapter);
DLFW_END:

	halmac_restore_mac_register_88xx(pHalmac_adapter, restore_info, DLFW_RESTORE_REG_NUM_88XX);

	if (halmac_dlfw_end_flow_88xx(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		goto DLFW_FAIL;

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_DONE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_firmware_88xx <==========\n");

	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	/* Disable FWDL_EN */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUFW_CTRL, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUFW_CTRL) & ~(BIT(0))));

	return HALMAC_RET_DLFW_FAIL;
}

/**
 * halmac_free_download_firmware_88xx() - download specific memory firmware
 * @pHalmac_adapter
 * @dlfw_mem : memory selection
 * @pHamacl_fw : firmware bin
 * @halmac_fw_size : firmware size
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 */
HALMAC_RET_STATUS
halmac_free_download_firmware_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DLFW_MEM dlfw_mem,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
)
{
	u8 tx_pause_backup;
	u8 *pFile_ptr;
	u16 bcn_head_backup;
	u32 iram_pkt_size, dmem_pkt_size, eram_pkt_size = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_DLFW_FAIL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_free_download_firmware_88xx ==========>\n");

	if (halmac_fw_size > HALMAC_FW_SIZE_MAX_88XX || halmac_fw_size < HALMAC_FWHDR_SIZE_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size error!\n");
		return HALMAC_RET_FW_SIZE_ERR;
	}

	dmem_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_DMEM_SIZE_88XX));
	iram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_IRAM_SIZE_88XX));
	if (0 != ((*(pHamacl_fw + HALMAC_FWHDR_OFFSET_MEM_USAGE_88XX)) & BIT(4)))
		eram_pkt_size = *((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_ERAM_SIZE_88XX));

	dmem_pkt_size = rtk_le32_to_cpu(dmem_pkt_size);
	iram_pkt_size = rtk_le32_to_cpu(iram_pkt_size);
	eram_pkt_size = rtk_le32_to_cpu(eram_pkt_size);

	dmem_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	iram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;
	if (eram_pkt_size != 0)
		eram_pkt_size += HALMAC_FW_CHKSUM_DUMMY_SIZE_88XX;

	if (halmac_fw_size != (HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size + eram_pkt_size)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]FW size mismatch the real fw size!\n");
		return HALMAC_RET_DLFW_FAIL;
	}

	tx_pause_backup = HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPAUSE);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXPAUSE, tx_pause_backup | BIT(7));

	bcn_head_backup = HALMAC_REG_READ_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2) | BIT(15);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, 0x8000);

	if (eram_pkt_size != 0) {
		pFile_ptr = pHamacl_fw + HALMAC_FWHDR_SIZE_88XX + dmem_pkt_size + iram_pkt_size;
		status = halmac_dlfw_to_mem_88xx(pHalmac_adapter, pFile_ptr,
					rtk_le32_to_cpu(*((u32 *)(pHamacl_fw + HALMAC_FWHDR_OFFSET_EMEM_ADDR_88XX))) & ~(BIT(31)), eram_pkt_size);
		if (status != HALMAC_RET_SUCCESS)
			goto DL_FREE_FW_END;
	}

	status = halmac_free_dl_fw_end_flow_88xx(pHalmac_adapter);

DL_FREE_FW_END:
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXPAUSE, tx_pause_backup);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, bcn_head_backup);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_free_download_firmware_88xx <==========\n");

	return status;
}

/**
 * halmac_get_fw_version_88xx() - get FW version
 * @pHalmac_adapter : the adapter of halmac
 * @pFw_version : fw version info
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_fw_version_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	OUT PHALMAC_FW_VERSION	pFw_version
)
{
	PHALMAC_FW_VERSION pFw_info = &pHalmac_adapter->fw_version;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (pFw_version == NULL)
		return HALMAC_RET_NULL_POINTER;

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE)
		return HALMAC_RET_NO_DLFW;

	pFw_version->version = pFw_info->version;
	pFw_version->sub_version = pFw_info->sub_version;
	pFw_version->sub_index = pFw_info->sub_index;
	pFw_version->h2c_version = pFw_info->h2c_version;
	pFw_version->build_time.month = pFw_info->build_time.month;
	pFw_version->build_time.date = pFw_info->build_time.date;
	pFw_version->build_time.hour = pFw_info->build_time.hour;
	pFw_version->build_time.min = pFw_info->build_time.min;
	pFw_version->build_time.year = pFw_info->build_time.year;

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_mac_addr_88xx() - config mac address
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_port : 0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : mac address
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_mac_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mac_addr_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index > 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	mac_address_L = pHal_address->Address_L_H.Address_Low;
	mac_address_H = pHal_address->Address_L_H.Address_High;

	mac_address_L = rtk_le32_to_cpu(mac_address_L);
	mac_address_H = rtk_le16_to_cpu(mac_address_H);

	pHalmac_adapter->pHal_mac_addr[halmac_port].Address_L_H.Address_Low = mac_address_L;
	pHalmac_adapter->pHal_mac_addr[halmac_port].Address_L_H.Address_High = mac_address_H;

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID + 4, mac_address_H);
		break;

	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID1, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID1 + 4, mac_address_H);
		break;

	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID2, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID2 + 4, mac_address_H);
		break;

	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID3, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID3 + 4, mac_address_H);
		break;

	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MACID4, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MACID4 + 4, mac_address_H);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mac_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_bssid_88xx() - config BSSID
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_port : 0 for port0, 1 for port1, 2 for port2, 3 for port3, 4 for port4
 * @pHal_address : bssid
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_bssid_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 halmac_port,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 bssid_address_H;
	u32 bssid_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bssid_88xx ==========>\n");

	if (halmac_port >= HALMAC_PORTIDMAX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]port index > 5\n");
		return HALMAC_RET_PORT_NOT_SUPPORT;
	}

	bssid_address_L = pHal_address->Address_L_H.Address_Low;
	bssid_address_H = pHal_address->Address_L_H.Address_High;

	bssid_address_L = rtk_le32_to_cpu(bssid_address_L);
	bssid_address_H = rtk_le16_to_cpu(bssid_address_H);

	pHalmac_adapter->pHal_bss_addr[halmac_port].Address_L_H.Address_Low = bssid_address_L;
	pHalmac_adapter->pHal_bss_addr[halmac_port].Address_L_H.Address_High = bssid_address_H;

	switch (halmac_port) {
	case HALMAC_PORTID0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID + 4, bssid_address_H);
		break;

	case HALMAC_PORTID1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID1, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID1 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID2:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID2, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID2 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID3:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID3, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID3 + 4, bssid_address_H);
		break;

	case HALMAC_PORTID4:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BSSID4, bssid_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_BSSID4 + 4, bssid_address_H);
		break;

	default:
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bssid_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_multicast_addr_88xx() - config multicast address
 * @pHalmac_adapter : the adapter of halmac
 * @pHal_address : multicast address
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_multicast_addr_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_WLAN_ADDR pHal_address
)
{
	u16 address_H;
	u32 address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_multicast_addr_88xx ==========>\n");

	address_L = pHal_address->Address_L_H.Address_Low;
	address_H = pHal_address->Address_L_H.Address_High;

	address_L = rtk_le32_to_cpu(address_L);
	address_H = rtk_le16_to_cpu(address_H);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MAR, address_L);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MAR + 4, address_H);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_multicast_addr_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pre_init_system_cfg_88xx() - pre-init system config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pre_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 value32, counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8 enable_bb;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_pre_init_system_cfg ==========>\n");

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL) & ~(BIT(0)));
		counter = 10000;
		while (!(HALMAC_REG_READ_8(pHalmac_adapter, REG_SDIO_HSUS_CTRL) & 0x02)) {
			counter--;
			if (counter == 0)
				return HALMAC_RET_SDIO_LEAVE_SUSPEND_FAIL;
		}

		if (pHalmac_adapter->sdio_hw_info.spec_ver == HALMAC_SDIO_SPEC_VER_3_00)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2) | BIT(2));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_HCI_OPT_CTRL + 2) & ~(BIT(2)));
	} else if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_USB) {
		if (HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_CFG2 + 3) == 0x20)	 /* usb3.0 */
			HALMAC_REG_WRITE_8(pHalmac_adapter, 0xFE5B, HALMAC_REG_READ_8(pHalmac_adapter, 0xFE5B) | BIT(4));
	}

	/* Config PIN Mux */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_PAD_CTRL1);
	value32 = value32 & (~(BIT(28) | BIT(29)));
	value32 = value32 | BIT(28) | BIT(29);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_PAD_CTRL1, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_LED_CFG);
	value32 = value32 & (~(BIT(25) | BIT(26)));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_LED_CFG, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_GPIO_MUXCFG);
	value32 = value32 & (~(BIT(2)));
	value32 = value32 | BIT(2);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_GPIO_MUXCFG, value32);

	enable_bb = _FALSE;
	halmac_set_hw_value_88xx(pHalmac_adapter, HALMAC_HW_EN_BB_RF, &enable_bb);


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_pre_init_system_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_system_cfg_88xx() -  init system config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u32 temp = 0;


	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_system_cfg ==========>\n");

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_FUNC_EN + 1, HALMAC_FUNCTION_ENABLE_88XX);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SYS_SDIO_CTRL, (u32)(HALMAC_REG_READ_32(pHalmac_adapter, REG_SYS_SDIO_CTRL) | BIT_LTE_MUX_CTRL_PATH));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CPU_DMEM_CON, (u32)(HALMAC_REG_READ_32(pHalmac_adapter, REG_CPU_DMEM_CON) | BIT_WL_PLATFORM_RST));

	/*disable boot-from-flash for driver's DL FW*/
	temp = HALMAC_REG_READ_32(pHalmac_adapter, REG_MCUFW_CTRL);
	if (temp & BIT_BOOT_FSPI_EN) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MCUFW_CTRL, temp & (~BIT_BOOT_FSPI_EN));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_GPIO_MUXCFG, HALMAC_REG_READ_32(pHalmac_adapter, REG_GPIO_MUXCFG) & (~BIT_FSPI_EN));
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_system_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_edca_cfg_88xx() - init EDCA config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_edca_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_edca_cfg_88xx ==========>\n");

	/* Clear TX pause */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXPAUSE, 0x0000);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SLOT, HALMAC_SLOT_TIME_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_PIFS, HALMAC_PIFS_TIME_88XX);
	value32 = HALMAC_SIFS_CCK_CTX_88XX | (HALMAC_SIFS_OFDM_CTX_88XX << BIT_SHIFT_SIFS_OFDM_CTX) |
		  (HALMAC_SIFS_CCK_TRX_88XX << BIT_SHIFT_SIFS_CCK_TRX) | (HALMAC_SIFS_OFDM_TRX_88XX << BIT_SHIFT_SIFS_OFDM_TRX);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SIFS, value32);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_EDCA_VO_PARAM, HALMAC_REG_READ_32(pHalmac_adapter, REG_EDCA_VO_PARAM) & 0xFFFF);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_EDCA_VO_PARAM + 2, HALMAC_VO_TXOP_LIMIT_88XX);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_EDCA_VI_PARAM + 2, HALMAC_VI_TXOP_LIMIT_88XX);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RD_NAV_NXT, HALMAC_RDG_NAV_88XX | (HALMAC_TXOP_NAV_88XX << 16));
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RXTSF_OFFSET_CCK, HALMAC_CCK_RX_TSF_88XX | (HALMAC_OFDM_RX_TSF_88XX) << 8);

	/* Set beacon cotnrol - enable TSF and other related functions */
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL) | BIT_EN_BCN_FUNCTION));

	/* Set send beacon related registers */
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TBTT_PROHIBIT, HALMAC_TBTT_PROHIBIT_88XX | (HALMAC_TBTT_HOLD_TIME_88XX << BIT_SHIFT_TBTT_HOLD_TIME_AP));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DRVERLYINT, HALMAC_DRIVER_EARLY_INT_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCNDMATIM, HALMAC_BEACON_DMA_TIM_88XX);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_edca_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_wmac_cfg_88xx() - init wmac config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_wmac_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_wmac_cfg_88xx ==========>\n");

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RXFLTMAP0, HALMAC_RX_FILTER0_88XX);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RXFLTMAP, HALMAC_RX_FILTER_88XX);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RCR, HALMAC_RCR_CONFIG_88XX);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RX_PKT_LIMIT, HALMAC_RXPKT_MAX_SIZE_BASE512);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TCR + 1, (u8)(HALMAC_REG_READ_8(pHalmac_adapter, REG_TCR + 1) | 0x30));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TCR + 2, 0x30);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TCR + 1, 0x00);

#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_ACKTO_CCK, HALMAC_ACK_TO_CCK_88XX);
#endif

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 8, 0x30810041);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4, 0x50802098);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_wmac_cfg_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_init_mac_cfg_88xx() - config page1~page7 register
 * @pHalmac_adapter : the adapter of halmac
 * @mode : trx mode
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_init_mac_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE mode
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_mac_cfg_88xx ==========>mode = %d\n", mode);

	status = pHalmac_api->halmac_init_trx_cfg(pHalmac_adapter, mode);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_init_trx_cfg errorr = %x\n", status);
		return status;
	}
#if 1
	status = pHalmac_api->halmac_init_protocol_cfg(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_init_protocol_cfg_88xx error = %x\n", status);
		return status;
	}

	status = halmac_init_edca_cfg_88xx(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_init_edca_cfg_88xx error = %x\n", status);
		return status;
	}

	status = halmac_init_wmac_cfg_88xx(pHalmac_adapter);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_init_wmac_cfg_88xx error = %x\n", status);
		return status;
	}
#endif
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_init_mac_cfg_88xx <==========\n");

	return status;
}

/**
 * halmac_cfg_operation_mode_88xx() - config operation mode
 * @pHalmac_adapter : the adapter of halmac
 * @wireless_mode : 802.11 standard(b/g/n/ac¡K)
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_operation_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_WIRELESS_MODE wireless_mode
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_operation_mode_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]wireless_mode = %d\n", wireless_mode);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_operation_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_ch_bw_88xx() - config channel & bandwidth
 * @pHalmac_adapter : the adapter of halmac
 * @channel : WLAN channel, support 2.4G & 5G
 * @pri_ch_idx : primary channel index, idx1, idx2, idx3, idx4
 * @bw : band width, 20, 40, 80, 160, 5 ,10
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_ch_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel,
	IN HALMAC_PRI_CH_IDX pri_ch_idx,
	IN HALMAC_BW bw
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_bw_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]ch = %d, idx=%d, bw=%d\n", channel, pri_ch_idx, bw);

	halmac_cfg_pri_ch_idx_88xx(pHalmac_adapter,  pri_ch_idx);

	halmac_cfg_bw_88xx(pHalmac_adapter, bw);

	halmac_cfg_ch_88xx(pHalmac_adapter,  channel);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_ch_bw_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_cfg_ch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 channel
)
{
	u8 value8;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_ch_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]ch = %d\n", channel);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CCK_CHECK);
	value8 = value8 & (~(BIT(7)));

	if (channel > 35)
		value8 = value8 | BIT(7);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CCK_CHECK, value8);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_ch_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_cfg_pri_ch_idx_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PRI_CH_IDX pri_ch_idx
)
{
	u8 txsc_40 = 0, txsc_20 = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_pri_ch_idx_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]idx=%d\n",  pri_ch_idx);

	txsc_20 = pri_ch_idx;
	if ((txsc_20 == HALMAC_CH_IDX_1) || (txsc_20 == HALMAC_CH_IDX_3))
		txsc_40 = 9;
	else
		txsc_40 = 10;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_DATA_SC, BIT_TXSC_20M(txsc_20) | BIT_TXSC_40M(txsc_40));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_pri_ch_idx_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_bw_88xx() - config bandwidth
 * @pHalmac_adapter : the adapter of halmac
 * @bw : band width, 20, 40, 80, 160, 5 ,10
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_bw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_BW bw
)
{
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_bw_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]bw=%d\n", bw);

	/* RF Mode */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WMAC_TRXPTCL_CTL);
	value32 = value32 & (~(BIT(7) | BIT(8)));

	switch (bw) {
	case HALMAC_BW_80:
		value32 = value32 | BIT(7);
		break;
	case HALMAC_BW_40:
		value32 = value32 | BIT(8);
		break;
	case HALMAC_BW_20:
	case HALMAC_BW_10:
	case HALMAC_BW_5:
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_cfg_bw_88xx switch case not support\n");
		break;
	}
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_TRXPTCL_CTL, value32);

	/* MAC CLK */
	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_AFE_CTRL1);
	value32 = (value32 & (~(BIT(20) | BIT(21)))) | (HALMAC_MAC_CLOCK_HW_DEF_80M << BIT_SHIFT_MAC_CLK_SEL);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_AFE_CTRL1, value32);

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_USTIME_TSF, HALMAC_MAC_CLOCK_88XX);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_USTIME_EDCA, HALMAC_MAC_CLOCK_88XX);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_cfg_bw_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_efuse_map_88xx() - dump "physical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @cfg : dump efuse method
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dump_efuse_map_88xx ==========>cfg=%d\n", cfg);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_WARN, "[WARN]Dump efuse in suspend mode\n");

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;
	pHalmac_adapter->event_trigger.physical_efuse_map = 1;

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_dump_efuse_88xx(pHalmac_adapter, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_read_efuse error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _TRUE) {
		*pProcess_status = HALMAC_CMD_PROCESS_DONE;

		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE, *pProcess_status,
			pHalmac_adapter->pHalEfuse_map, pHalmac_adapter->hw_config_info.efuse_size);
		pHalmac_adapter->event_trigger.physical_efuse_map = 0;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_dump_efuse_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_efuse_map_bt_88xx() - dump "BT physical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_efuse_bank : bt efuse bank
 * @bt_efuse_map_size : bt efuse map size. get from halmac_get_efuse_size API
 * @pBT_efuse_map : bt efuse map
 * Author : Soar / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_efuse_map_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank,
	IN u32 bt_efuse_map_size,
	OUT u8 *pBT_efuse_map
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dump_efuse_map_bt_88xx ==========>\n");

	if (pHalmac_adapter->hw_config_info.bt_efuse_size != bt_efuse_map_size)
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;

	if ((halmac_efuse_bank >= HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_read_hw_efuse_88xx(pHalmac_adapter, 0, bt_efuse_map_size, pBT_efuse_map);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_read_hw_efuse_88xx error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_dump_efuse_map_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_write_efuse_bt_88xx() - write "BT physical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @halmac_value : Write value
 * @pBT_efuse_map : bt efuse map
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_write_efuse_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_value,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_write_efuse_bt_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "offset : %X value : %X Bank : %X\n", halmac_offset, halmac_value, halmac_efuse_bank);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_offset >= pHalmac_adapter->hw_config_info.efuse_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((halmac_efuse_bank > HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_write_efuse_88xx(pHalmac_adapter, halmac_offset, halmac_value);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_write_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_write_efuse_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_read_efuse_bt_88xx() - read "BT physical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @pValue : 1 byte efuse value
 * @HALMAC_EFUSE_BANK : efuse bank
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_efuse_bt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	OUT u8 *pValue,
	IN HALMAC_EFUSE_BANK halmac_efuse_bank
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_read_efuse_bt_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_offset >= pHalmac_adapter->hw_config_info.efuse_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((halmac_efuse_bank > HALMAC_EFUSE_BANK_MAX) || (halmac_efuse_bank == HALMAC_EFUSE_BANK_WIFI)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Undefined BT bank\n");
		return HALMAC_RET_EFUSE_BANK_INCORRECT;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, halmac_efuse_bank);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_read_efuse_88xx(pHalmac_adapter, halmac_offset, 1, pValue);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_read_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_read_efuse_bt_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_efuse_auto_check_88xx() - check efuse after writing it
 * @pHalmac_adapter : the adapter of halmac
 * @enable : 1, enable efuse auto check. others, disable
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_efuse_auto_check_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_efuse_auto_check_88xx ==========> function enable = %d\n", enable);

	pHalmac_adapter->efuse_auto_check_en = enable;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_efuse_auto_check_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_efuse_available_size_88xx() - get efuse available size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : physical efuse available size
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_efuse_available_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	HALMAC_RET_STATUS status;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_efuse_available_size_88xx ==========>\n");

	status = halmac_dump_logical_efuse_map_88xx(pHalmac_adapter, HALMAC_EFUSE_R_DRV);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	*halmac_size = pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - pHalmac_adapter->efuse_end;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_efuse_available_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_efuse_size_88xx() - get "physical" efuse size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : physical efuse size
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_efuse_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_efuse_size_88xx ==========>\n");

	*halmac_size = pHalmac_adapter->hw_config_info.efuse_size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_efuse_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_logical_efuse_size_88xx() - get "logical" efuse size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_size : logical efuse size
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_logical_efuse_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u32 *halmac_size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_logical_efuse_size_88xx ==========>\n");

	*halmac_size = pHalmac_adapter->hw_config_info.eeprom_size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_get_logical_efuse_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_logical_efuse_map_88xx() - dump "logical" efuse map
 * @pHalmac_adapter : the adapter of halmac
 * @cfg : dump efuse method
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_logical_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_dump_logical_efuse_map_88xx ==========>cfg = %d\n", cfg);

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_OFF)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_WARN, "[WARN]Dump logical efuse in suspend mode\n");

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;
	pHalmac_adapter->event_trigger.logical_efuse_map = 1;

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_dump_efuse_88xx(pHalmac_adapter, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_eeprom_parser_88xx error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->hal_efuse_map_valid == _TRUE) {
		*pProcess_status = HALMAC_CMD_PROCESS_DONE;

		pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
		if (pEeprom_map == NULL) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac allocate local eeprom map Fail!!\n");
			return HALMAC_RET_MALLOC_FAIL;
		}
		PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

		if (halmac_eeprom_parser_88xx(pHalmac_adapter, pHalmac_adapter->pHalEfuse_map, pEeprom_map) != HALMAC_RET_SUCCESS) {
			PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
			return HALMAC_RET_EEPROM_PARSING_FAIL;
		}

		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_DUMP_LOGICAL_EFUSE, *pProcess_status, pEeprom_map, eeprom_size);
		pHalmac_adapter->event_trigger.logical_efuse_map = 0;

		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_dump_logical_efuse_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_read_logical_efuse_88xx() - read logical efuse map 1 byte
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @pValue : 1 byte efuse value
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	OUT u8 *pValue
)
{
	u8 *pEeprom_map = NULL;
	u32 eeprom_size = pHalmac_adapter->hw_config_info.eeprom_size;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_read_logical_efuse_88xx ==========>\n");

	if (halmac_offset >= eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}
	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	pEeprom_map = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, eeprom_size);
	if (pEeprom_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac allocate local eeprom map Fail!!\n");
		return HALMAC_RET_MALLOC_FAIL;
	}
	PLATFORM_RTL_MEMSET(pDriver_adapter, pEeprom_map, 0xFF, eeprom_size);

	status = halmac_read_logical_efuse_map_88xx(pHalmac_adapter, pEeprom_map);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_read_logical_efuse_map error = %x\n", status);
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return status;
	}

	*pValue = *(pEeprom_map + halmac_offset);

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS) {
		PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);
		return HALMAC_RET_ERROR_STATE;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_read_logical_efuse_88xx <==========\n");

	PLATFORM_RTL_FREE(pDriver_adapter, pEeprom_map, eeprom_size);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_write_logical_efuse_88xx() - write "logical" efuse offset
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_offset : offset
 * @halmac_value : value
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_write_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u8 halmac_value
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_write_logical_efuse_88xx ==========>\n");

	if (halmac_offset >= pHalmac_adapter->hw_config_info.eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "Offset is too large\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_write_logical_efuse_88xx(pHalmac_adapter, halmac_offset, halmac_value);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_write_logical_efuse error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_write_logical_efuse_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pg_efuse_by_map_88xx() - pg logical efuse by map
 * @pHalmac_adapter : the adapter of halmac
 * @pPg_efuse_info : efuse map information
 * @cfg : dump efuse method
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pg_efuse_by_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN HALMAC_EFUSE_READ_CFG cfg
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.efuse_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_pg_efuse_by_map_88xx ==========>\n");

	if (pPg_efuse_info->efuse_map_size != pHalmac_adapter->hw_config_info.eeprom_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "efuse_map_size is incorrect, should be %d bytes\n", pHalmac_adapter->hw_config_info.eeprom_size);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if ((pPg_efuse_info->efuse_map_size & 0xF) > 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "efuse_map_size should be multiple of 16\n");
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pPg_efuse_info->efuse_mask_size != pPg_efuse_info->efuse_map_size >> 4) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "efuse_mask_size is incorrect, should be %d bytes\n", pPg_efuse_info->efuse_map_size >> 4);
		return HALMAC_RET_EFUSE_SIZE_INCORRECT;
	}

	if (pPg_efuse_info->pEfuse_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "efuse_map is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (pPg_efuse_info->pEfuse_mask == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "efuse_mask is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait/Rcvd event(dump efuse)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (halmac_query_efuse_curr_state_88xx(pHalmac_adapter) != HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(dump efuse)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	status = halmac_func_switch_efuse_bank_88xx(pHalmac_adapter, HALMAC_EFUSE_BANK_WIFI);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_func_switch_efuse_bank error = %x\n", status);
		return status;
	}

	status = halmac_func_pg_efuse_by_map_88xx(pHalmac_adapter, pPg_efuse_info, cfg);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_pg_efuse_by_map error = %x\n", status);
		return status;
	}

	if (halmac_transition_efuse_state_88xx(pHalmac_adapter, HALMAC_EFUSE_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_TRACE, "halmac_pg_efuse_by_map_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_c2h_info_88xx() - process halmac C2H packet
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : RX Packet pointer
 * @halmac_size : RX Packet size
 * Author : KaiYuan Chang/Ivan Lin
 *
 * Used to process c2h packet info from RX path. After receiving the packet,
 * user need to call this api and pass the packet pointer.
 *
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_c2h_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;


	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_c2h_info_88xx ==========>\n"); */

	/* Check if it is C2H packet */
	if (GET_RX_DESC_C2H(halmac_buf) == _TRUE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "C2H packet, start parsing!\n");

		status = halmac_parse_c2h_packet_88xx(pHalmac_adapter, halmac_buf, halmac_size);

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_EFUSE, HALMAC_DBG_ERR, "halmac_parse_c2h_packet_88xx error = %x\n", status);
			return status;
		}
	}

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_c2h_info_88xx <==========\n"); */

	return HALMAC_RET_SUCCESS;
}

/**
 * (debug API)halmac_h2c_lb_88xx() - send h2c loopback packet
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_h2c_lb_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_h2c_lb_88xx ==========>\n");

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_h2c_lb_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_debug_88xx() - dump information for debugging
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_debug_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	u8 temp8 = 0;
	u32 i = 0, temp32 = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug_88xx ==========>\n");

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		/* Dump CCCR, it needs new platform api */

		/*Dump SDIO Local Register, use CMD52*/
		for (i = 0x10250000; i < 0x102500ff; i++) {
			temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: sdio[%x]=%x\n", i, temp8);
		}

		/*Dump MAC Register*/
		for (i = 0x0000; i < 0x17ff; i++) {
			temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp8);
		}

		/*Check RX Fifo status*/
		i = REG_RXFF_PTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_WTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_PTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp8);
		i = REG_RXFF_WTR_V1;
		temp8 = PLATFORM_SDIO_CMD52_READ(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp8);
	} else {
		/*Dump MAC Register*/
		for (i = 0x0000; i < 0x17fc; i += 4) {
			temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp32);
		}

		/*Check RX Fifo status*/
		i = REG_RXFF_PTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_WTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_PTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp32);
		i = REG_RXFF_WTR_V1;
		temp32 = HALMAC_REG_READ_32(pHalmac_adapter, i);
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug: mac[%x]=%x\n", i, temp32);
	}

	/*	TODO: Add check register code, including MAC CLK, CPU CLK */

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_debug_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_parameter_88xx() - config parameter by FW
 * @pHalmac_adapter : the adapter of halmac
 * @para_info : cmd id, content
 * @full_fifo : parameter information
 *
 * If msk_en = _TRUE, the format of array is {reg_info, mask, value}.
 * If msk_en =_FAUSE, the format of array is {reg_info, value}
 * The format of reg_info is
 * reg_info[31]=rf_reg, 0: MAC_BB reg, 1: RF reg
 * reg_info[27:24]=rf_path, 0: path_A, 1: path_B
 * if rf_reg=0(MAC_BB reg), rf_path is meaningless.
 * ref_info[15:0]=offset
 *
 * Example: msk_en = _FALSE
 * {0x8100000a, 0x00001122}
 * =>Set RF register, path_B, offset 0xA to 0x00001122
 * {0x00000824, 0x11224433}
 * =>Set MAC_BB register, offset 0x800 to 0x11224433
 *
 * Note : full fifo mode only for init flow
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.cfg_para_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_parameter_88xx ==========>\n"); */

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_cfg_parameter_88xx Fail due to DLFW NONE!!\n");
		return HALMAC_RET_DLFW_FAIL;
	}

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(cfg para)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if ((halmac_query_cfg_para_curr_state_88xx(pHalmac_adapter) != HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE) &&
	    (halmac_query_cfg_para_curr_state_88xx(pHalmac_adapter) != HALMAC_CFG_PARA_CMD_CONSTRUCT_CONSTRUCTING)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Not idle state(cfg para)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_IDLE;

	ret_status = halmac_send_h2c_phy_parameter_88xx(pHalmac_adapter, para_info, full_fifo);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_phy_parameter_88xx Fail!! = %x\n", ret_status);
		return ret_status;
	}

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_parameter_88xx <==========\n"); */

	return ret_status;
}

/**
 * halmac_update_packet_88xx() - send specific packet to FW
 * @pHalmac_adapter : the adapter of halmac
 * @pkt_id : packet id, to know the purpose of this packet
 * @pkt : packet
 * @pkt_size : packet size
 *
 * Note : TX_DESC is not included in the pkt
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID	pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.update_packet_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_update_packet_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(update_packet)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	status = halmac_send_h2c_update_packet_88xx(pHalmac_adapter, pkt_id, pkt, pkt_size);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_update_packet_88xx packet = %x,  fail = %x!!\n", pkt_id, status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_update_packet_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_bcn_ie_filter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_bcn_ie_filter_88xx ==========>\n");

	status = halmac_send_h2c_update_bcn_parse_info_88xx(pHalmac_adapter, pBcn_ie_info);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_update_bcn_parse_info_88xx fail = %x\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_bcn_ie_filter_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type,
	IN PHALMAC_PHY_PARAMETER_INFO para_info
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_datapack_88xx ==========>\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_update_datapack_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_run_datapack_88xx ==========>\n");

	ret_status = halmac_send_h2c_run_datapack_88xx(pHalmac_adapter, halmac_data_type);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_run_datapack_88xx Fail, datatype = %x, status = %x!!\n", halmac_data_type, ret_status);
		return ret_status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_update_datapack_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_drv_info_88xx() - config driver info
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_drv_info : driver information selection
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_drv_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_INFO halmac_drv_info
)
{
	u8 drv_info_size = 0;
	u8 phy_status_en = 0;
	u8 sniffer_en = 0;
	u8 plcp_hdr_en = 0;
	u32 value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_info_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_info = %d\n", halmac_drv_info);

	switch (halmac_drv_info) {
	case HALMAC_DRV_INFO_NONE:
		drv_info_size = 0;
		phy_status_en = 0;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_STATUS:
		drv_info_size = 4;
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_SNIFFER:
		drv_info_size = 5; /* phy status 4byte, sniffer info 1byte */
		phy_status_en = 1;
		sniffer_en = 1;
		plcp_hdr_en = 0;
		break;
	case HALMAC_DRV_INFO_PHY_PLCP:
		drv_info_size = 6; /* phy status 4byte, plcp header 2byte */
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 1;
		break;
	default:
		status = HALMAC_RET_SW_CASE_NOT_SUPPORT;
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_cfg_drv_info_88xx error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode != HALMAC_RX_FIFO_EXPANDING_MODE_DISABLE)
		drv_info_size = HALMAC_RX_DESC_DUMMY_SIZE_MAX_88XX >> 3;

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RX_DRVINFO_SZ, drv_info_size);

	pHalmac_adapter->drv_info_size = drv_info_size;

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_RCR);
	value32 = (value32 & (~BIT_APP_PHYSTS));
	if (phy_status_en == 1)
		value32 = value32 | BIT_APP_PHYSTS;
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RCR, value32);

	value32 = HALMAC_REG_READ_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4);
	value32 = (value32 & (~(BIT(8) | BIT(9))));
	if (sniffer_en == 1)
		value32 = value32 | BIT(9);
	if (plcp_hdr_en == 1)
		value32 = value32 | BIT(8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_WMAC_OPTION_FUNCTION + 4, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_drv_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_send_bt_coex_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_bt_coex_88xx ==========>\n");

	ret_status = halmac_send_bt_coex_cmd_88xx(pHalmac_adapter, pBt_buf, bt_size, ack);

	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_bt_coex_cmd_88xx Fail = %x!!\n", ret_status);
		return ret_status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_bt_coex_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * (debug API)halmac_verify_platform_api_88xx() - verify platform api
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_verify_platform_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS ret_status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_verify_platform_api_88xx ==========>\n");

	ret_status = halmac_verify_io_88xx(pHalmac_adapter);

	if (ret_status != HALMAC_RET_SUCCESS)
		return ret_status;

	if (pHalmac_adapter->txff_allocation.la_mode != HALMAC_LA_MODE_FULL)
		ret_status = halmac_verify_send_rsvd_page_88xx(pHalmac_adapter);

	if (ret_status != HALMAC_RET_SUCCESS)
		return ret_status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_verify_platform_api_88xx <==========\n");

	return ret_status;
}

HALMAC_RET_STATUS
halmac_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_original_h2c_88xx ==========>\n");

	status = halmac_func_send_original_h2c_88xx(pHalmac_adapter, original_h2c, seq, ack);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_original_h2c FAIL = %x!!\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_original_h2c_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_timer_2s_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_timer_2s_88xx ==========>\n");


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_timer_2s_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_fill_txdesc_check_sum_88xx() -  fill in tx desc check sum
 * @pHalmac_adapter : the adapter of halmac
 * @pCur_desc : tx desc packet
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_fill_txdesc_check_sum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u8 *pCur_desc
)
{
	u16 chk_result = 0;
	u16 *pData = (u16 *)NULL;
	u32 i;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (pCur_desc == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_fill_txdesc_check_sum_88xx NULL PTR");
		return HALMAC_RET_NULL_POINTER;
	}

	SET_TX_DESC_TXDESC_CHECKSUM(pCur_desc, 0x0000);

	pData = (u16 *)(pCur_desc);

	/* HW clculates only 32byte */
	for (i = 0; i < 8; i++)
		chk_result ^= (*(pData + 2 * i) ^ *(pData + (2 * i + 1)));

	/* *(pData + 2 * i) & *(pData + (2 * i + 1) have endain issue*/
	/* Process eniadn issue after checksum calculation */
	chk_result = rtk_le16_to_cpu(chk_result);

	SET_TX_DESC_TXDESC_CHECKSUM(pCur_desc, chk_result);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dump_fifo_88xx() - dump fifo data
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_fifo_sel : FIFO selection
 * @halmac_start_addr : start address of selected FIFO
 * @halmac_fifo_dump_size : dump size of selected FIFO
 * @pFifo_map : FIFO data
 *
 * Note : before dump fifo, user need to call halmac_get_fifo_size to
 * get fifo size. Then input this size to halmac_dump_fifo.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dump_fifo_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	IN u32 halmac_start_addr,
	IN u32 halmac_fifo_dump_size,
	OUT u8 *pFifo_map
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dump_fifo_88xx ==========>\n");

	if (halmac_fifo_sel == HAL_FIFO_SEL_TX && (halmac_start_addr + halmac_fifo_dump_size) > pHalmac_adapter->hw_config_info.tx_fifo_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "TX fifo dump size is too large\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (halmac_fifo_sel == HAL_FIFO_SEL_RX && (halmac_start_addr + halmac_fifo_dump_size) > pHalmac_adapter->hw_config_info.rx_fifo_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "RX fifo dump size is too large\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if ((halmac_fifo_dump_size & (4 - 1)) != 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_fifo_dump_size shall 4byte align\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (pFifo_map == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "pFifo_map address is NULL\n");
		return HALMAC_RET_NULL_POINTER;
	}

	status = halmac_buffer_read_88xx(pHalmac_adapter, halmac_start_addr, halmac_fifo_dump_size, halmac_fifo_sel, pFifo_map);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_buffer_read_88xx error = %x\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dump_fifo_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_fifo_size_88xx() - get fifo size
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_fifo_sel : FIFO selection
 * Author : Ivan Lin/KaiYuan Chang
 * Return : u32
 * More details of status code can be found in prototype document
 */
u32
halmac_get_fifo_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_FIFO_SEL halmac_fifo_sel
)
{
	u32 fifo_size = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fifo_sel == HAL_FIFO_SEL_TX)
		fifo_size = pHalmac_adapter->hw_config_info.tx_fifo_size;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RX)
		fifo_size = pHalmac_adapter->hw_config_info.rx_fifo_size;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RSVD_PAGE)
		fifo_size = ((pHalmac_adapter->hw_config_info.tx_fifo_size >> HALMAC_TX_PAGE_SIZE_2_POWER_88XX)
						- pHalmac_adapter->txff_allocation.rsvd_pg_bndy) << HALMAC_TX_PAGE_SIZE_2_POWER_88XX;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_REPORT)
		fifo_size = 65536;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_LLT)
		fifo_size = 65536;
	else if (halmac_fifo_sel == HAL_FIFO_SEL_RXBUF_FW)
		fifo_size = HALMAC_RX_BUF_FW_88XX;

	return fifo_size;
}

/**
 * halmac_cfg_txbf_88xx() - enable/disable specific user's txbf
 * @pHalmac_adapter : the adapter of halmac
 * @userid : su bfee userid = 0 or 1 to apply TXBF
 * @bw : the sounding bandwidth
 * @txbf_en : 0: disable TXBF, 1: enable TXBF
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_txbf_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid,
	IN HALMAC_BW bw,
	IN u8 txbf_en
)
{
	u16 temp42C = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (txbf_en) {
		switch (bw) {
		case HALMAC_BW_80:
			temp42C |= BIT_R_TXBF0_80M;
		case HALMAC_BW_40:
			temp42C |= BIT_R_TXBF0_40M;
		case HALMAC_BW_20:
			temp42C |= BIT_R_TXBF0_20M;
			break;
		default:
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_cfg_txbf_88xx invalid TXBF BW setting 0x%x of userid %d\n", bw, userid);
			return HALMAC_RET_INVALID_SOUNDING_SETTING;
		}
	}

	switch (userid) {
	case 0:
		temp42C |= HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) & ~(BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, temp42C);
		break;
	case 1:
		temp42C |= HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) & ~(BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, temp42C);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_cfg_txbf_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_cfg_txbf_88xx, txbf_en = %x <==========\n", txbf_en);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_mumimo_88xx() -config mumimo
 * @pHalmac_adapter : the adapter of halmac
 * @pCfgmu : parameters to configure MU PPDU Tx/Rx
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_mumimo_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CFG_MUMIMO_PARA pCfgmu
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8 i, idx, id0, id1, gid, mu_tab_sel;
	u8 mu_tab_valid = 0;
	u32 gid_valid[6] = {0};
	u8 temp14C0 = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (pCfgmu->role == HAL_BFEE) {
		/*config MU BFEE*/
		temp14C0 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~BIT_MASK_R_MU_TABLE_VALID;
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, (temp14C0 | BIT(0) | BIT(1)) & ~(BIT(7)));	/*enable MU table 0 and 1, disable MU TX*/

		/*config GID valid table and user position table*/
		mu_tab_sel = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL + 1) & ~(BIT(0) | BIT(1) | BIT(2));
		for (i = 0; i < 2; i++) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL + 1, mu_tab_sel | i);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, pCfgmu->given_gid_tab[i]);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO, pCfgmu->given_user_pos[i * 2]);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO + 4, pCfgmu->given_user_pos[i * 2 + 1]);
		}
	} else {
		/*config MU BFER*/
		if (pCfgmu->mu_tx_en == _FALSE) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(7)));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_cfg_mumimo_88xx disable mu tx <==========\n");
			return HALMAC_RET_SUCCESS;
		}

		/*Transform BB grouping bitmap[14:0] to MAC GID_valid table*/
		for (idx = 0; idx < 15; idx++) {
			if (idx < 5) {
				/*grouping_bitmap bit0~4, MU_STA0 with MUSTA1~5*/
				id0 = 0;
				id1 = (u8)(idx + 1);
			} else if (idx < 9) {
				/*grouping_bitmap bit5~8, MU_STA1 with MUSTA2~5*/
				id0 = 1;
				id1 = (u8)(idx - 3);
			} else if (idx < 12) {
				/*grouping_bitmap bit9~11, MU_STA2 with MUSTA3~5*/
				id0 = 2;
				id1 = (u8)(idx - 6);
			} else if (idx < 14) {
				/*grouping_bitmap bit12~13, MU_STA3 with MUSTA4~5*/
				id0 = 3;
				id1 = (u8)(idx - 8);
			} else {
				/*grouping_bitmap bit14, MU_STA4 with MUSTA5*/
				id0 = 4;
				id1 = (u8)(idx - 9);
			}
			if (pCfgmu->grouping_bitmap & BIT(idx)) {
				/*Pair 1*/
				gid = (idx << 1) + 1;
				gid_valid[id0] |= (BIT(gid));
				gid_valid[id1] |= (BIT(gid));
				/*Pair 2*/
				gid += 1;
				gid_valid[id0] |= (BIT(gid));
				gid_valid[id1] |= (BIT(gid));
			} else {
				/*Pair 1*/
				gid = (idx << 1) + 1;
				gid_valid[id0] &= ~(BIT(gid));
				gid_valid[id1] &= ~(BIT(gid));
				/*Pair 2*/
				gid += 1;
				gid_valid[id0] &= ~(BIT(gid));
				gid_valid[id1] &= ~(BIT(gid));
			}
		}

		/*set MU STA GID valid TABLE*/
		mu_tab_sel = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL + 1) & ~(BIT(0) | BIT(1) | BIT(2));
		for (idx = 0; idx < 6; idx++) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL + 1, idx | mu_tab_sel);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, gid_valid[idx]);
		}

		/*To validate the sounding successful MU STA and enable MU TX*/
		for (i = 0; i < 6; i++) {
			if (pCfgmu->sounding_sts[i] == _TRUE)
				mu_tab_valid |= BIT(i);
		}
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, mu_tab_valid | BIT(7));
	}
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_cfg_mumimo_88xx <==========\n");
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_sounding_88xx() - configure general sounding
 * @pHalmac_adapter : the adapter of halmac
 * @role : driver's role, BFer or BFee
 * @datarate : set ndpa tx rate if driver is BFer, or set csi response rate if driver is BFee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_sounding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SND_ROLE role,
	IN HALMAC_DATA_RATE	datarate
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (role) {
	case HAL_BFER:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TXBF_CTRL, HALMAC_REG_READ_32(pHalmac_adapter, REG_TXBF_CTRL) | BIT_R_ENABLE_NDPA
			| BIT_USE_NDPA_PARAMETER | BIT_R_EN_NDPA_INT | BIT_DIS_NDP_BFEN);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_NDPA_RATE, datarate);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_NDPA_OPT_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_NDPA_OPT_CTRL) & (~(BIT(0) | BIT(1))));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 1, 0x2 | BIT(7));	/*service file length 2 bytes; fix non-STA1 csi start offset */
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 2, 0x2);
		break;
	case HAL_BFEE:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL, 0xDB);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 3, 0x24);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BBPSF_CTRL + 3, HALMAC_OFDM54 | BIT(6));		//use ndpa rx rate to decide csi rate
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, HALMAC_REG_READ_16(pHalmac_adapter, REG_RRSR) | BIT(datarate));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXFLTMAP1, HALMAC_REG_READ_8(pHalmac_adapter, REG_RXFLTMAP1) & (~(BIT(4))));	/*RXFF do not accept BF Rpt Poll, avoid CSI crc error*/
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXFLTMAP4, HALMAC_REG_READ_8(pHalmac_adapter, REG_RXFLTMAP4) & (~(BIT(4))));	/*FWFF do not accept BF Rpt Poll, avoid CSI crc error*/
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_cfg_sounding_88xx invalid role\n");
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_cfg_sounding_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_del_sounding_88xx() - reset general sounding
 * @pHalmac_adapter : the adapter of halmac
 * @role : driver's role, BFer or BFee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_del_sounding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SND_ROLE role
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (role) {
	case HAL_BFER:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXBF_CTRL + 3, 0);
		break;
	case HAL_BFEE:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_del_sounding_88xx invalid role\n");
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_del_sounding_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_init_88xx() - config SU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : SU bfee userid = 0 or 1 to be added
 * @paid : partial AID of this bfee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfee_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid,
	IN u16 paid
)
{
	u16 temp42C = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		temp42C = HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) & ~(BIT_MASK_R_TXBF0_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, temp42C | paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL, paid);
		break;
	case 1:
		temp42C = HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) & ~(BIT_MASK_R_TXBF1_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, temp42C | paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL + 2, paid | BIT(9));
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_su_bfee_entry_init_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_su_bfee_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_init_88xx() - config SU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pSu_bfer_init : parameters to configure SU BFER entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfer_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SU_BFER_INIT_PARA pSu_bfer_init
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	/* mac_address_L = bfer_address.Address_L_H.Address_Low; */
	/* mac_address_H = bfer_address.Address_L_H.Address_High; */

	mac_address_L = rtk_le32_to_cpu(pSu_bfer_init->bfer_address.Address_L_H.Address_Low);
	mac_address_H = rtk_le16_to_cpu(pSu_bfer_init->bfer_address.Address_L_H.Address_High);

	switch (pSu_bfer_init->userid) {
	case 0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, mac_address_H);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 6, pSu_bfer_init->paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20, pSu_bfer_init->csi_para);
		break;
	case 1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 4, mac_address_H);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 6, pSu_bfer_init->paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20 + 2, pSu_bfer_init->csi_para);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_su_bfer_entry_init_88xx invalid userid %d\n", pSu_bfer_init->userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_su_bfer_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfee_entry_init_88xx() - config MU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pMu_bfee_init : parameters to configure MU BFEE entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfee_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_MU_BFEE_INIT_PARA pMu_bfee_init
)
{
	u16 temp168X = 0, temp14C0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	temp168X |= pMu_bfee_init->paid | BIT(9);
	HALMAC_REG_WRITE_16(pHalmac_adapter, (0x1680 + pMu_bfee_init->userid * 2), temp168X);

	temp14C0 = HALMAC_REG_READ_16(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(8) | BIT(9) | BIT(10));
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MU_TX_CTL, temp14C0 | ((pMu_bfee_init->userid - 2) << 8));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, 0);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO, pMu_bfee_init->user_position_l);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO + 4, pMu_bfee_init->user_position_h);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_mu_bfee_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfer_entry_init_88xx() - config MU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pMu_bfer_init : parameters to configure MU BFER entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfer_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_MU_BFER_INIT_PARA pMu_bfer_init
)
{
	u16 temp1680 = 0;
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	/* mac_address_L = pHalmac_adapter->snd_info.bfer_address.Address_L_H.Address_Low; */
	/* mac_address_H = pHalmac_adapter->snd_info.bfer_address.Address_L_H.Address_High; */

	mac_address_L = rtk_le32_to_cpu(pMu_bfer_init->bfer_address.Address_L_H.Address_Low);
	mac_address_H = rtk_le16_to_cpu(pMu_bfer_init->bfer_address.Address_L_H.Address_High);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, mac_address_L);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, mac_address_H);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 6, pMu_bfer_init->paid);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20, pMu_bfer_init->csi_para);

	temp1680 = HALMAC_REG_READ_16(pHalmac_adapter, 0x1680) & 0xC000;
	temp1680 |= pMu_bfer_init->my_aid | (pMu_bfer_init->csi_length_sel << 12);
	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680, temp1680);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_mu_bfer_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_del_88xx() - reset SU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the SU BFee userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfee_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) &
			~(BIT_MASK_R_TXBF0_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M));
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL, 0);
		break;
	case 1:
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) &
			~(BIT_MASK_R_TXBF1_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M));
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL + 2, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_su_bfee_entry_del_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_su_bfee_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_del_88xx() - reset SU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the SU BFer userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfer_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, 0);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, 0);
		break;
	case 1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO, 0);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 4, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_su_bfer_entry_del_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_su_bfer_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfee_entry_del_88xx() - reset MU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the MU STA userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfee_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680 + userid * 2, 0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(userid - 2)));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_mu_bfee_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfer_entry_del_88xx() -reset MU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfer_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, 0);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, 0);
	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680, 0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, 0);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_mu_bfer_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_add_ch_info_88xx() -add channel information
 * @pHalmac_adapter : the adapter of halmac
 * @pCh_info : channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_add_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_INFO pCh_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_CS_INFO pCh_sw_info;
	HALMAC_SCAN_CMD_CONSTRUCT_STATE state_scan;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pCh_sw_info = &pHalmac_adapter->ch_sw_info;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_ch_info_88xx ==========>\n");

	if (pHalmac_adapter->halmac_state.dlfw_state != HALMAC_GEN_INFO_SENT) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: gen_info is not send to FW!!!!\n");
		return HALMAC_RET_GEN_INFO_NOT_SENT;
	}

	state_scan = halmac_query_scan_curr_state_88xx(pHalmac_adapter);
	if ((state_scan != HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) && (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_WARN, "[WARN]Scan machine fail(add ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (pCh_sw_info->ch_info_buf == NULL) {
		pCh_sw_info->ch_info_buf = (u8 *)PLATFORM_RTL_MALLOC(pDriver_adapter, HALMAC_EXTRA_INFO_BUFF_SIZE_88XX);
		if (pCh_sw_info->ch_info_buf == NULL)
			return HALMAC_RET_NULL_POINTER;
		pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf;
		pCh_sw_info->buf_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX;
		pCh_sw_info->avai_buf_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX;
		pCh_sw_info->total_size = 0;
		pCh_sw_info->extra_info_en = 0;
		pCh_sw_info->ch_num = 0;
	}

	if (pCh_sw_info->extra_info_en == 1) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: construct sequence wrong!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->avai_buf_size < 4) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_add_ch_info_88xx: no available buffer!!\n");
		return HALMAC_RET_CH_SW_NO_BUF;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	CHANNEL_INFO_SET_CHANNEL(pCh_sw_info->ch_info_buf_w, pCh_info->channel);
	CHANNEL_INFO_SET_PRI_CH_IDX(pCh_sw_info->ch_info_buf_w, pCh_info->pri_ch_idx);
	CHANNEL_INFO_SET_BANDWIDTH(pCh_sw_info->ch_info_buf_w, pCh_info->bw);
	CHANNEL_INFO_SET_TIMEOUT(pCh_sw_info->ch_info_buf_w, pCh_info->timeout);
	CHANNEL_INFO_SET_ACTION_ID(pCh_sw_info->ch_info_buf_w, pCh_info->action_id);
	CHANNEL_INFO_SET_CH_EXTRA_INFO(pCh_sw_info->ch_info_buf_w, pCh_info->extra_info);

	pCh_sw_info->avai_buf_size = pCh_sw_info->avai_buf_size - 4;
	pCh_sw_info->total_size = pCh_sw_info->total_size + 4;
	pCh_sw_info->ch_num++;
	pCh_sw_info->extra_info_en = pCh_info->extra_info;
	pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf_w + 4;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_add_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_add_extra_ch_info_88xx() -add extra channel information
 * @pHalmac_adapter : the adapter of halmac
 * @pCh_extra_info : extra channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_add_extra_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_EXTRA_INFO pCh_extra_info
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_CS_INFO pCh_sw_info;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pCh_sw_info = &pHalmac_adapter->ch_sw_info;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_add_extra_ch_info_88xx ==========>\n");

	if (pCh_sw_info->ch_info_buf == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_add_extra_ch_info_88xx: NULL==pCh_sw_info->ch_info_buf!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->extra_info_en == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_add_extra_ch_info_88xx: construct sequence wrong!!\n");
		return HALMAC_RET_CH_SW_SEQ_WRONG;
	}

	if (pCh_sw_info->avai_buf_size < (u32)(pCh_extra_info->extra_info_size + 2)) {/* 2:ch_extra_info_id, ch_extra_info, ch_extra_info_size are totally 2Byte */
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_add_extra_ch_info_88xx: no available buffer!!\n");
		return HALMAC_RET_CH_SW_NO_BUF;
	}

	if (halmac_query_scan_curr_state_88xx(pHalmac_adapter) != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Scan machine fail(add extra ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	CH_EXTRA_INFO_SET_CH_EXTRA_INFO_ID(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_action_id);
	CH_EXTRA_INFO_SET_CH_EXTRA_INFO(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_info);
	CH_EXTRA_INFO_SET_CH_EXTRA_INFO_SIZE(pCh_sw_info->ch_info_buf_w, pCh_extra_info->extra_info_size);
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCh_sw_info->ch_info_buf_w + 2, pCh_extra_info->extra_info_data, pCh_extra_info->extra_info_size);

	pCh_sw_info->avai_buf_size = pCh_sw_info->avai_buf_size - (2 + pCh_extra_info->extra_info_size);
	pCh_sw_info->total_size = pCh_sw_info->total_size + (2 + pCh_extra_info->extra_info_size);
	pCh_sw_info->extra_info_en = pCh_extra_info->extra_info;
	pCh_sw_info->ch_info_buf_w = pCh_sw_info->ch_info_buf_w + (2 + pCh_extra_info->extra_info_size);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_add_extra_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_ctrl_ch_switch_88xx() -send channel switch cmd
 * @pHalmac_adapter : the adapter of halmac
 * @pCs_option : channel switch config
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION	pCs_option
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_SCAN_CMD_CONSTRUCT_STATE state_scan;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.scan_state_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_ctrl_ch_switch_88xx  pCs_option->switch_en = %d==========>\n", pCs_option->switch_en);

	if (pCs_option->switch_en == _FALSE)
		*pProcess_status = HALMAC_CMD_PROCESS_IDLE;

	if ((*pProcess_status == HALMAC_CMD_PROCESS_SENDING) || (*pProcess_status == HALMAC_CMD_PROCESS_RCVD)) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(ctrl ch switch)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	state_scan = halmac_query_scan_curr_state_88xx(pHalmac_adapter);
	if (pCs_option->switch_en == _TRUE) {
		if (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_CONSTRUCTING) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_ctrl_ch_switch_88xx(on)  invalid in state %x\n", state_scan);
			return HALMAC_RET_ERROR_STATE;
		}
	} else {
		if (state_scan != HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_ctrl_ch_switch_88xx(off)  invalid in state %x\n", state_scan);
			return HALMAC_RET_ERROR_STATE;
		}
	}

	status = halmac_func_ctrl_ch_switch_88xx(pHalmac_adapter, pCs_option);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_ctrl_ch_switch FAIL = %x!!\n", status);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_ctrl_ch_switch_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_clear_ch_info_88xx() -clear channel information
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_clear_ch_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_clear_ch_info_88xx ==========>\n");

	if (halmac_query_scan_curr_state_88xx(pHalmac_adapter) == HALMAC_SCAN_CMD_CONSTRUCT_H2C_SENT) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Scan machine fail(clear ch info)...\n");
		return HALMAC_RET_ERROR_STATE;
	}

	if (halmac_transition_scan_state_88xx(pHalmac_adapter, HALMAC_SCAN_CMD_CONSTRUCT_BUFFER_CLEARED) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->ch_sw_info.ch_info_buf, pHalmac_adapter->ch_sw_info.buf_size);
	pHalmac_adapter->ch_sw_info.ch_info_buf = NULL;
	pHalmac_adapter->ch_sw_info.ch_info_buf_w = NULL;
	pHalmac_adapter->ch_sw_info.extra_info_en = 0;
	pHalmac_adapter->ch_sw_info.buf_size = 0;
	pHalmac_adapter->ch_sw_info.avai_buf_size = 0;
	pHalmac_adapter->ch_sw_info.total_size = 0;
	pHalmac_adapter->ch_sw_info.ch_num = 0;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_clear_ch_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS	pP2PPS
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 6)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	status = halmac_func_p2pps_88xx(pHalmac_adapter, pP2PPS);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_p2pps FAIL = %x!!\n", status);
		return status;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_func_p2pps_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_P2PPS	pP2PPS
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_p2pps !!\n");

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	P2PPS_SET_OFFLOAD_EN(pH2c_buff, pP2PPS->offload_en);
	P2PPS_SET_ROLE(pH2c_buff, pP2PPS->role);
	P2PPS_SET_CTWINDOW_EN(pH2c_buff, pP2PPS->ctwindow_en);
	P2PPS_SET_NOA_EN(pH2c_buff, pP2PPS->noa_en);
	P2PPS_SET_NOA_SEL(pH2c_buff, pP2PPS->noa_sel);
	P2PPS_SET_ALLSTASLEEP(pH2c_buff, pP2PPS->all_sta_sleep);
	P2PPS_SET_DISCOVERY(pH2c_buff, pP2PPS->discovery);
	P2PPS_SET_P2P_PORT_ID(pH2c_buff, pP2PPS->p2p_port_id);
	P2PPS_SET_P2P_GROUP(pH2c_buff, pP2PPS->p2p_group);
	P2PPS_SET_P2P_MACID(pH2c_buff, pP2PPS->p2p_macid);

	P2PPS_SET_CTWINDOW_LENGTH(pH2c_buff, pP2PPS->ctwindow_length);

	P2PPS_SET_NOA_DURATION_PARA(pH2c_buff, pP2PPS->noa_duration_para);
	P2PPS_SET_NOA_INTERVAL_PARA(pH2c_buff, pP2PPS->noa_interval_para);
	P2PPS_SET_NOA_START_TIME_PARA(pH2c_buff, pP2PPS->noa_start_time_para);
	P2PPS_SET_NOA_COUNT_PARA(pH2c_buff, pP2PPS->noa_count_para);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_P2PPS;
	h2c_header_info.content_size = 24;
	h2c_header_info.ack = _FALSE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _FALSE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_p2pps_88xx Fail = %x!!\n", status);

	return status;
}

/**
 * halmac_send_general_info_88xx() -send general information to FW
 * @pHalmac_adapter : the adapter of halmac
 * @pGeneral_info : general information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_general_info_88xx ==========>\n");

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_NONE) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_general_info_88xx Fail due to DLFW NONE!!\n");
		return HALMAC_RET_DLFW_FAIL;
	}

	status = halmac_func_send_general_info_88xx(pHalmac_adapter, pGeneral_info);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_send_general_info error = %x\n", status);
		return status;
	}

	if (pHalmac_adapter->halmac_state.dlfw_state == HALMAC_DLFW_DONE)
		pHalmac_adapter->halmac_state.dlfw_state = HALMAC_GEN_INFO_SENT;

	pHalmac_adapter->gen_info_valid = _TRUE;
	PLATFORM_RTL_MEMCPY(pDriver_adapter, &pHalmac_adapter->general_info, pGeneral_info, sizeof(HALMAC_GENERAL_INFO));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_send_general_info_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_start_iqk_88xx() -trigger FW IQK
 * @pHalmac_adapter : the adapter of halmac
 * @pIqk_para : IQK parameter
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_start_iqk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_IQK_PARA pIqk_para
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_num = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.iqk_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_start_iqk_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(iqk)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	IQK_SET_CLEAR(pH2c_buff, pIqk_para->clear);
	IQK_SET_SEGMENT_IQK(pH2c_buff, pIqk_para->segment_iqk);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_IQK;
	h2c_header_info.content_size = 1;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_num);

	pHalmac_adapter->halmac_state.iqk_set.seq_num = h2c_seq_num;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_IQK);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_start_iqk_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_ctrl_pwr_tracking_88xx() -trigger FW power tracking
 * @pHalmac_adapter : the adapter of halmac
 * @pPwr_tracking_opt : power tracking option
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_ctrl_pwr_tracking_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PWR_TRACKING_OPTION pPwr_tracking_opt
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.power_tracking_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_start_iqk_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(pwr tracking)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	POWER_TRACKING_SET_TYPE(pH2c_buff, pPwr_tracking_opt->type);
	POWER_TRACKING_SET_BBSWING_INDEX(pH2c_buff, pPwr_tracking_opt->bbswing_index);
	POWER_TRACKING_SET_ENABLE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_A(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_A].tssi_value);
	POWER_TRACKING_SET_ENABLE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_B(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_B].tssi_value);
	POWER_TRACKING_SET_ENABLE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_C(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_C].tssi_value);
	POWER_TRACKING_SET_ENABLE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].enable);
	POWER_TRACKING_SET_TX_PWR_INDEX_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].tx_pwr_index);
	POWER_TRACKING_SET_PWR_TRACKING_OFFSET_VALUE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].pwr_tracking_offset_value);
	POWER_TRACKING_SET_TSSI_VALUE_D(pH2c_buff, pPwr_tracking_opt->pwr_tracking_para[HALMAC_RF_PATH_D].tssi_value);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_POWER_TRACKING;
	h2c_header_info.content_size = 20;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	pHalmac_adapter->halmac_state.power_tracking_set.seq_num = h2c_seq_mum;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_POWER_TRACKING);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_start_iqk_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_query_status_88xx() -query the offload feature status
 * @pHalmac_adapter : the adapter of halmac
 * @feature_id : feature_id
 * @pProcess_status : feature_status
 * @data : data buffer
 * @size : data size
 *
 * Note :
 * If user wants to know the data size, use can allocate zero
 * size buffer first. If this size less than the data size, halmac
 * will return  HALMAC_RET_BUFFER_TOO_SMALL. User need to
 * re-allocate data buffer with correct data size.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_query_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_query_status_88xx ==========>\n"); */

	if (pProcess_status == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "null pointer!!\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (feature_id) {
	case HALMAC_FEATURE_CFG_PARA:
		status = halmac_query_cfg_para_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE:
		status = halmac_query_dump_physical_efuse_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE:
		status = halmac_query_dump_logical_efuse_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_CHANNEL_SWITCH:
		status = halmac_query_channel_switch_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_UPDATE_PACKET:
		status = halmac_query_update_packet_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_IQK:
		status = halmac_query_iqk_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_POWER_TRACKING:
		status = halmac_query_power_tracking_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	case HALMAC_FEATURE_PSD:
		status = halmac_query_psd_status_88xx(pHalmac_adapter, pProcess_status, data, size);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "halmac_query_status_88xx invalid feature id %d\n", feature_id);
		return HALMAC_RET_INVALID_FEATURE_ID;
	}

	/* PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_query_status_88xx <==========\n"); */

	return status;
}

/**
 * halmac_reset_feature_88xx() -reset async api cmd status
 * @pHalmac_adapter : the adapter of halmac
 * @feature_id : feature_id
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS.
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_reset_feature_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_STATE pState = &pHalmac_adapter->halmac_state;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_reset_feature_88xx ==========>\n");

	switch (feature_id) {
	case HALMAC_FEATURE_CFG_PARA:
		pState->cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->cfg_para_state_set.cfg_para_cmd_construct_state = HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE:
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE:
		pState->efuse_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->efuse_state_set.efuse_cmd_construct_state = HALMAC_EFUSE_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_CHANNEL_SWITCH:
		pState->scan_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->scan_state_set.scan_cmd_construct_state = HALMAC_SCAN_CMD_CONSTRUCT_IDLE;
		break;
	case HALMAC_FEATURE_UPDATE_PACKET:
		pState->update_packet_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_IQK:
		pState->iqk_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_POWER_TRACKING:
		pState->power_tracking_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_PSD:
		pState->psd_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	case HALMAC_FEATURE_ALL:
		pState->cfg_para_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->cfg_para_state_set.cfg_para_cmd_construct_state = HALMAC_CFG_PARA_CMD_CONSTRUCT_IDLE;
		pState->efuse_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->efuse_state_set.efuse_cmd_construct_state = HALMAC_EFUSE_CMD_CONSTRUCT_IDLE;
		pState->scan_state_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->scan_state_set.scan_cmd_construct_state = HALMAC_SCAN_CMD_CONSTRUCT_IDLE;
		pState->update_packet_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->iqk_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->power_tracking_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		pState->psd_set.process_status = HALMAC_CMD_PROCESS_IDLE;
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_reset_feature_88xx invalid feature id %d\n", feature_id);
		return HALMAC_RET_INVALID_FEATURE_ID;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_reset_feature_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_check_fw_status_88xx() -check fw status
 * @pHalmac_adapter : the adapter of halmac
 * @fw_status : fw status
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_check_fw_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *fw_status
)
{
	u32 value32 = 0, value32_backup = 0, i = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_check_fw_status_88xx ==========>\n");

	value32 = PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG6);

	if (value32 != 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_check_fw_status REG_FW_DBG6 !=0\n");
		*fw_status = _FALSE;
		return status;
	}

	value32_backup = PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG7);

	for (i = 0; i <= 10; i++) {
		value32 = PLATFORM_REG_READ_32(pDriver_adapter, REG_FW_DBG7);
		if (value32_backup != value32)
			break;

		if (i == 10) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_check_fw_status Polling FW PC fail\n");
			*fw_status = _FALSE;
			return status;
		}
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_check_fw_status_88xx <==========\n");

	return status;
}

HALMAC_RET_STATUS
halmac_dump_fw_dmem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u8 *dmem,
	INOUT u32 *size
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_dump_fw_dmem_88xx ==========>\n");


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_dump_fw_dmem_88xx <==========\n");

	return status;
}

/**
 * halmac_cfg_max_dl_size_88xx() - config max download FW size
 * @pHalmac_adapter : the adapter of halmac
 * @size : max download fw size
 *
 * Halmac uses this setting to set max packet size for
 * download FW.
 * If user has not called this API, halmac use default
 * setting for download FW
 * Note1 : size need multiple of 2
 * Note2 : max size is 31K
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_max_dl_size_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 size
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "halmac_cfg_max_dl_size_88xx ==========>\n");

	if (size > HALMAC_FW_CFG_MAX_DL_SIZE_MAX_88XX) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "size > HALMAC_FW_CFG_MAX_DL_SIZE_MAX!\n");
		return HALMAC_RET_CFG_DLFW_SIZE_FAIL;
	}

	if (0 != (size & (2 - 1))) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_ERR, "size is not power of 2!\n");
		return HALMAC_RET_CFG_DLFW_SIZE_FAIL;
	}

	pHalmac_adapter->max_download_size = size;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "Cfg max size is : %X\n", size);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_FW, HALMAC_DBG_TRACE, "halmac_cfg_max_dl_size_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_psd_88xx() - trigger fw psd
 * @pHalmac_adapter : the adapter of halmac
 * @start_psd : start PSD
 * @end_psd : end PSD
 * Author : KaiYuan Chang/Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_psd_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 start_psd,
	IN u16 end_psd
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum = 0;
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.psd_set.process_status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_psd_88xx ==========>\n");

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "Wait event(psd)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	if (pHalmac_adapter->halmac_state.psd_set.pData != NULL) {
		PLATFORM_RTL_FREE(pDriver_adapter, pHalmac_adapter->halmac_state.psd_set.pData, pHalmac_adapter->halmac_state.psd_set.data_size);
		pHalmac_adapter->halmac_state.psd_set.pData = (u8 *)NULL;
	}

	pHalmac_adapter->halmac_state.psd_set.data_size = 0;
	pHalmac_adapter->halmac_state.psd_set.segment_size = 0;

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	PSD_SET_START_PSD(pH2c_buff, start_psd);
	PSD_SET_END_PSD(pH2c_buff, end_psd);

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_PSD;
	h2c_header_info.content_size = 4;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_PSD);
		return status;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_psd_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_la_mode_88xx() - config la mode
 * @pHalmac_adapter : the adapter of halmac
 * @la_mode :
 *	disable : no TXFF space reserved for LA debug
 *	partial : partial TXFF space is reserved for LA debug
 *	full : all TXFF space is reserved for LA debug
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_la_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_LA_MODE la_mode
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_la_mode_88xx ==========>la_mode = %d\n", la_mode);

	pHalmac_adapter->txff_allocation.la_mode = la_mode;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_la_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_rx_fifo_expanding_mode_88xx() - rx fifo expanding
 * @pHalmac_adapter : the adapter of halmac
 * @la_mode :
 *	disable : normal mode
 *	1 block : Rx FIFO + 1 FIFO block; Tx fifo - 1 FIFO block
 *	2 block : Rx FIFO + 2 FIFO block; Tx fifo - 2 FIFO block
 *	3 block : Rx FIFO + 3 FIFO block; Tx fifo - 3 FIFO block
 * Author : Soar
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_rx_fifo_expanding_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_RX_FIFO_EXPANDING_MODE rx_fifo_expanding_mode
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_rx_fifo_expanding_mode_88xx ==========>rx_fifo_expanding_mode = %d\n", rx_fifo_expanding_mode);

	pHalmac_adapter->txff_allocation.rx_fifo_expanding_mode = rx_fifo_expanding_mode;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_rx_fifo_expanding_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_config_security_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SECURITY_SETTING pSec_setting
)
{
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
	return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_config_security_88xx ==========>\n");

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_CR, (u16)(HALMAC_REG_READ_16(pHalmac_adapter, REG_CR) | BIT_MAC_SEC_EN));

	if (pSec_setting->tx_encryption == 1)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) | BIT(2));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) & ~(BIT(2)));

	if (pSec_setting->rx_decryption == 1)
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) | BIT(3));
	else
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SECCFG, HALMAC_REG_READ_8(pHalmac_adapter, REG_SECCFG) & ~(BIT(3)));

	if (pSec_setting->bip_enable == 1) {
		if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8822B)
			return HALMAC_RET_BIP_NO_SUPPORT;
#if HALMAC_8821C_SUPPORT
		if (pSec_setting->tx_encryption == 1)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) | (BIT(3) | BIT(5)));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) & ~(BIT(3) | BIT(5)));

		if (pSec_setting->rx_decryption == 1)
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) | (BIT(4) | BIT(6)));
		else
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_WSEC_OPTION + 2, HALMAC_REG_READ_8(pHalmac_adapter, REG_WSEC_OPTION + 2) & ~(BIT(4) | BIT(6)));
#endif
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_config_security_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

u8
halmac_get_used_cam_entry_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HAL_SECURITY_TYPE sec_type
)
{
	u8 entry_num;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_get_used_cam_entry_num_88xx ==========>\n");

	switch (sec_type) {
	case HAL_SECURITY_TYPE_WEP40:
	case HAL_SECURITY_TYPE_WEP104:
	case HAL_SECURITY_TYPE_TKIP:
	case HAL_SECURITY_TYPE_AES128:
	case HAL_SECURITY_TYPE_GCMP128:
	case HAL_SECURITY_TYPE_GCMSMS4:
	case HAL_SECURITY_TYPE_BIP:
		entry_num = 1;
		break;
	case HAL_SECURITY_TYPE_WAPI:
	case HAL_SECURITY_TYPE_AES256:
	case HAL_SECURITY_TYPE_GCMP256:
		entry_num = 2;
		break;
	default:
		entry_num = 0;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_get_used_cam_entry_num_88xx <==========\n");

	return entry_num;
}

HALMAC_RET_STATUS
halmac_write_cam_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	IN PHALMAC_CAM_ENTRY_INFO pCam_entry_info
)
{
	u32 i;
	u32 command = 0x80010000;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;
	PHALMAC_CAM_ENTRY_FORMAT pCam_entry_format = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_write_cam_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	if (pCam_entry_info->key_id > 3)
		return HALMAC_RET_FAIL;

	pCam_entry_format = (PHALMAC_CAM_ENTRY_FORMAT)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(*pCam_entry_format));
	if (pCam_entry_format == NULL)
		return HALMAC_RET_NULL_POINTER;
	PLATFORM_RTL_MEMSET(pDriver_adapter, pCam_entry_format, 0x00, sizeof(*pCam_entry_format));

	pCam_entry_format->key_id = pCam_entry_info->key_id;
	pCam_entry_format->valid = pCam_entry_info->valid;
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->mac_address, pCam_entry_info->mac_address, 6);
	PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->key, pCam_entry_info->key, 16);

	switch (pCam_entry_info->security_type) {
	case HAL_SECURITY_TYPE_NONE:
		pCam_entry_format->type = 0;
		break;
	case HAL_SECURITY_TYPE_WEP40:
		pCam_entry_format->type = 1;
		break;
	case HAL_SECURITY_TYPE_WEP104:
		pCam_entry_format->type = 5;
		break;
	case HAL_SECURITY_TYPE_TKIP:
		pCam_entry_format->type = 2;
		break;
	case HAL_SECURITY_TYPE_AES128:
		pCam_entry_format->type = 4;
		break;
	case HAL_SECURITY_TYPE_WAPI:
		pCam_entry_format->type = 6;
		break;
	case HAL_SECURITY_TYPE_AES256:
		pCam_entry_format->type = 4;
		pCam_entry_format->ext_sectype = 1;
		break;
	case HAL_SECURITY_TYPE_GCMP128:
		pCam_entry_format->type = 7;
		break;
	case HAL_SECURITY_TYPE_GCMP256:
	case HAL_SECURITY_TYPE_GCMSMS4:
		pCam_entry_format->type = 7;
		pCam_entry_format->ext_sectype = 1;
		break;
	case HAL_SECURITY_TYPE_BIP:
		pCam_entry_format->type = (pCam_entry_info->unicast == 1) ? 4 : 0;
		pCam_entry_format->mgnt = 1;
		pCam_entry_format->grp = (pCam_entry_info->unicast == 1) ? 0 : 1;
		break;
	default:
		PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));
		return HALMAC_RET_FAIL;
	}


	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]1 - CAM entry format : %X\n", *((u32 *)pCam_entry_format + i));
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]1 - REG_CAMCMD : %X\n", command | ((entry_index << 3) + i));
	}

	if (HAL_SECURITY_TYPE_WAPI == pCam_entry_info->security_type || HAL_SECURITY_TYPE_AES256 == pCam_entry_info->security_type ||
			HAL_SECURITY_TYPE_GCMP256 == pCam_entry_info->security_type || HAL_SECURITY_TYPE_GCMSMS4 == pCam_entry_info->security_type) {
		pCam_entry_format->mic = 1;
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pCam_entry_format->key, pCam_entry_info->key_ext, 16);

		for (i = 0; i < 8; i++) {
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | (((entry_index + 1) << 3) + i));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]2 - CAM entry format : %X\n", *((u32 *)pCam_entry_format + i));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]2 - REG_CAMCMD : %X\n", command | (((entry_index + 1) << 3) + i));
		}
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_write_cam_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_read_cam_entry_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u32 entry_index,
	OUT PHALMAC_CAM_ENTRY_FORMAT pContent
)
{
	u32 i;
	u32 command = 0x80000000;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_read_cam_entry_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
		*((u32 *)pContent + i) = HALMAC_REG_READ_32(pHalmac_adapter, REG_CAMREAD);
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_read_cam_entry_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_clear_cam_entry_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 entry_index
)
{
	u32 i;
	u32 command = 0x80010000;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	PHALMAC_CAM_ENTRY_FORMAT pCam_entry_format;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_security_cam_88xx ==========>\n");

	if (entry_index >= pHalmac_adapter->hw_config_info.cam_entry_num)
		return HALMAC_RET_ENTRY_INDEX_ERROR;

	pCam_entry_format = (PHALMAC_CAM_ENTRY_FORMAT)PLATFORM_RTL_MALLOC(pDriver_adapter, sizeof(*pCam_entry_format));
	if (pCam_entry_format == NULL)
		return HALMAC_RET_NULL_POINTER;
	PLATFORM_RTL_MEMSET(pDriver_adapter, pCam_entry_format, 0x00, sizeof(*pCam_entry_format));

	for (i = 0; i < 8; i++) {
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMWRITE, *((u32 *)pCam_entry_format + i));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_CAMCMD, command | ((entry_index << 3) + i));
	}

	PLATFORM_RTL_FREE(pDriver_adapter, pCam_entry_format, sizeof(*pCam_entry_format));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_clear_security_cam_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_get_hw_value_88xx() -get hw config value
 * @pHalmac_adapter : the adapter of halmac
 * @hw_id : hw id for driver to query
 * @pvalue : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_get_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	OUT VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_hw_value_88xx ==========>\n");

	if (pvalue == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_get_hw_value_88xx (NULL ==pvalue)==========>\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (hw_id) {
	case HALMAC_HW_RQPN_MAPPING:
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_vo = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VO];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_vi = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_VI];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_be = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BE];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_bk = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_BK];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_mg = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_MG];
		((PHALMAC_RQPN_MAP)pvalue)->dma_map_hi = pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI];
		break;
	case HALMAC_HW_EFUSE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.efuse_size;
		break;
	case HALMAC_HW_EEPROM_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.eeprom_size;
		break;
	case HALMAC_HW_BT_BANK_EFUSE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.bt_efuse_size;
		break;
	case HALMAC_HW_BT_BANK1_EFUSE_SIZE:
	case HALMAC_HW_BT_BANK2_EFUSE_SIZE:
		*(u32 *)pvalue = 0;
		break;
	case HALMAC_HW_TXFIFO_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.tx_fifo_size;
		break;
	case HALMAC_HW_RSVD_PG_BNDY:
		*(u16 *)pvalue = pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy;
		break;
	case HALMAC_HW_CAM_ENTRY_NUM:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.cam_entry_num;
		break;
	case HALMAC_HW_WLAN_EFUSE_AVAILABLE_SIZE: /*Remove later*/
		status = halmac_dump_logical_efuse_map_88xx(pHalmac_adapter, HALMAC_EFUSE_R_DRV);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.efuse_size - HALMAC_PROTECTED_EFUSE_SIZE_88XX - pHalmac_adapter->efuse_end;
		break;
	case HALMAC_HW_IC_VERSION:
		*(u8 *)pvalue = pHalmac_adapter->chip_version;
		break;
	case HALMAC_HW_PAGE_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.page_size;
		break;
	case HALMAC_HW_TX_AGG_ALIGN_SIZE:
		*(u16 *)pvalue = pHalmac_adapter->hw_config_info.tx_align_size;
		break;
	case HALMAC_HW_RX_AGG_ALIGN_SIZE:
		*(u8 *)pvalue = 8;
		break;
	case HALMAC_HW_DRV_INFO_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->drv_info_size;
		break;
	case HALMAC_HW_TXFF_ALLOCATION:
		PLATFORM_RTL_MEMCPY(pDriver_adapter, pvalue, &pHalmac_adapter->txff_allocation, sizeof(HALMAC_TXFF_ALLOCATION));
		break;
	case HALMAC_HW_TX_DESC_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.txdesc_size;
		break;
	case HALMAC_HW_RX_DESC_SIZE:
		*(u32 *)pvalue = pHalmac_adapter->hw_config_info.rxdesc_size;
		break;
	case HALMAC_HW_AC_OQT_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.ac_oqt_size;
		break;
	case HALMAC_HW_NON_AC_OQT_SIZE:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.non_ac_oqt_size;
		break;
	case HALMAC_HW_AC_QUEUE_NUM:
		*(u8 *)pvalue = pHalmac_adapter->hw_config_info.ac_queue_num;
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_hw_value_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_set_hw_value_88xx() -set hw config value
 * @pHalmac_adapter : the adapter of halmac
 * @hw_id : hw id for driver to config
 * @pvalue : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_set_hw_value_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_HW_ID hw_id,
	IN VOID *pvalue
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_set_hw_value_88xx ==========>\n");

	if (pvalue == NULL) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_set_hw_value_88xx (NULL == pvalue)==========>\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (hw_id) {
	case HALMAC_HW_USB_MODE:
		status = halmac_set_usb_mode_88xx(pHalmac_adapter, *(HALMAC_USB_MODE *)pvalue);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		break;
	case HALMAC_HW_SEQ_EN:
		/*if (_TRUE == hw_seq_en) {
		} else {
		}*/
		break;
	case HALMAC_HW_BANDWIDTH:
		halmac_cfg_bw_88xx(pHalmac_adapter, *(HALMAC_BW *)pvalue);
		break;
	case HALMAC_HW_CHANNEL:
		halmac_cfg_ch_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_PRI_CHANNEL_IDX:
		halmac_cfg_pri_ch_idx_88xx(pHalmac_adapter, *(HALMAC_PRI_CH_IDX *)pvalue);
		break;
	case HALMAC_HW_EN_BB_RF:
		halmac_enable_bb_rf_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	case HALMAC_HW_SDIO_TX_PAGE_THRESHOLD:
		halmac_config_sdio_tx_page_threshold_88xx(pHalmac_adapter, (PHALMAC_TX_PAGE_THRESHOLD_INFO)pvalue);
		break;
	case HALMAC_HW_AMPDU_CONFIG:
		halmac_config_ampdu_88xx(pHalmac_adapter, (PHALMAC_AMPDU_CONFIG)pvalue);
		break;
	case HALMAC_HW_RX_SHIFT:
		halmac_rx_shift_88xx(pHalmac_adapter, *(u8 *)pvalue);
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_set_hw_value_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_drv_rsvd_pg_num_88xx() -config reserved page number for driver
 * @pHalmac_adapter : the adapter of halmac
 * @pg_num : page number
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_drv_rsvd_pg_num_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DRV_RSVD_PG_NUM pg_num
)
{
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_drv_rsvd_pg_num_88xx ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]pg_num = %d\n", pg_num);

	switch (pg_num) {
	case HALMAC_RSVD_PG_NUM16:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 16;
		break;
	case HALMAC_RSVD_PG_NUM24:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 24;
		break;
	case HALMAC_RSVD_PG_NUM32:
		pHalmac_adapter->txff_allocation.rsvd_drv_pg_num = 32;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_cfg_drv_rsvd_pg_num_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_get_chip_version_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_VER pVersion
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_chip_version_88xx ==========>\n");
	pVersion->major_ver = (u8)HALMAC_MAJOR_VER_88XX;
	pVersion->prototype_ver = (u8)HALMAC_PROTOTYPE_VER_88XX;
	pVersion->minor_ver = (u8)HALMAC_MINOR_VER_88XX;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "halmac_get_chip_version_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_chk_txdesc_88xx() -check if the tx packet format is incorrect
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_buf : tx Packet buffer, tx desc is included
 * @halmac_size : tx packet size
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_chk_txdesc_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
)
{
	u32 mac_clk = 0;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_chk_txdesc_88xx ==========>\n");

	if (GET_TX_DESC_BMC(pHalmac_buf) == _TRUE)
		if (GET_TX_DESC_AGG_EN(pHalmac_buf) == _TRUE)
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: Agg should not be set when BMC\n");

	if (halmac_size < (GET_TX_DESC_TXPKTSIZE(pHalmac_buf) + GET_TX_DESC_OFFSET(pHalmac_buf)))
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: PktSize too small\n");

	if (GET_TX_DESC_AMSDU_PAD_EN(pHalmac_buf) != 0)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]TxDesc: Do not set AMSDU_PAD_EN\n");

	switch (BIT_GET_MAC_CLK_SEL(HALMAC_REG_READ_32(pHalmac_adapter, REG_AFE_CTRL1))) {
	case 0x0:
		mac_clk = 80;
		break;
	case 0x1:
		mac_clk = 40;
		break;
	case 0x2:
		mac_clk = 20;
		break;
	case 0x3:
		mac_clk = 10;
		break;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "MAC clock : 0x%XM\n", mac_clk);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "TX mac agg enable : 0x%X\n", GET_TX_DESC_AGG_EN(pHalmac_buf));
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ALWAYS, "TX mac agg num : 0x%X\n", GET_TX_DESC_MAX_AGG_NUM(pHalmac_buf));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_chk_txdesc_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_dl_drv_rsvd_page_88xx() - download packet to rsvd page
 * @pHalmac_adapter : the adapter of halmac
 * @pg_offset : page offset of driver's rsvd page
 * @halmac_buf : data to be downloaded, tx_desc is not included
 * @halmac_size : data size to be downloaded
 * Author : KaiYuan Chang
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_dl_drv_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 pg_offset,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS ret_status;
	u16 drv_pg_bndy = 0;
	u32 dl_pg_num = 0;


	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dl_drv_rsvd_page_88xx ==========>\n");

	/*check boundary and size valid*/
	dl_pg_num = halmac_size / pHalmac_adapter->hw_config_info.page_size + ((halmac_size & (pHalmac_adapter->hw_config_info.page_size - 1)) ? 1 : 0);
	if (pg_offset + dl_pg_num > pHalmac_adapter->txff_allocation.rsvd_drv_pg_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERROR] driver download offset or size error ==========>\n");
		return HALMAC_RET_DRV_DL_ERR;
	}

	/*update to target download boundary*/
	drv_pg_bndy = pHalmac_adapter->txff_allocation.rsvd_drv_pg_bndy + pg_offset;
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(drv_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));

	ret_status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_buf, halmac_size);

	/*restore to original bundary*/
	if (ret_status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_download_rsvd_page_88xx Fail = %x!!\n", ret_status);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
		return ret_status;
	}

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_dl_drv_rsvd_page_88xx < ==========\n");
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_csi_rate_88xx() - config CSI frame Tx rate
 * @pHalmac_adapter : the adapter of halmac
 * @rssi : rssi in decimal value
 * @current_rate : current CSI frame rate
 * @fixrate_en : enable to fix CSI frame in VHT rate, otherwise legacy OFDM rate
 * @new_rate : API returns the final CSI frame rate
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_csi_rate_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 rssi,
	IN u8 current_rate,
	IN u8 fixrate_en,
	OUT u8 *new_rate
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u32 temp_csi_setting;
	u16 current_rrsr;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "halmac_cfg_csi_rate_88xx ==========>\n");

#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C) {
		if (fixrate_en) {
			temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_CSI_FORCE_RATE_EN | BIT_CSI_RSC(1) | BIT_WMAC_CSI_RATE(HALMAC_VHT_NSS1_MCS3));
			*new_rate = HALMAC_VHT_NSS1_MCS3;
			return HALMAC_RET_SUCCESS;
		}
	}
	temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE) & ~BIT_CSI_FORCE_RATE_EN;
#else
	temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE);
#endif

	current_rrsr = HALMAC_REG_READ_16(pHalmac_adapter, REG_RRSR);

	if (rssi >= 40) {
		if (current_rate != HALMAC_OFDM54) {
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, current_rrsr | BIT(HALMAC_OFDM54));
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_WMAC_CSI_RATE(HALMAC_OFDM54));
		}
		*new_rate = HALMAC_OFDM54;
		return HALMAC_RET_SUCCESS;
	}

	if (current_rate != HALMAC_OFDM24) {
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, current_rrsr & ~(BIT(HALMAC_OFDM54)));
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_WMAC_CSI_RATE(HALMAC_OFDM24));
	}
	*new_rate = HALMAC_OFDM24;
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_txfifo_is_empty_88xx() -check if txfifo is empty
 * @pHalmac_adapter : the adapter of halmac
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_txfifo_is_empty_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 chk_num
)
{
	u32 counter;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_txfifo_is_empty_88xx ==========>\n");

	counter = (chk_num <= 10) ? 10 : chk_num;
	do {
		if (HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPKT_EMPTY) != 0xFF)
			return HALMAC_RET_TXFIFO_NO_EMPTY;

		if ((HALMAC_REG_READ_8(pHalmac_adapter, REG_TXPKT_EMPTY + 1) & 0x07) != 0x07)
			return HALMAC_RET_TXFIFO_NO_EMPTY;
		counter--;

	} while (counter != 0);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "halmac_txfifo_is_empty_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_download_flash_88xx() -download firmware to flash
 * @pHalmac_adapter : the adapter of halmac
 * @pHalmac_fw : pointer to fw
 * @halmac_fw_size : fw size
 * @rom_address : flash start address where fw should be download
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_download_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 rom_address
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	u8 value8;
	u8 restore[3];
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;
	u32 send_pkt_size, mem_offset;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_flash_88xx ==========>\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31));

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_TX_CTRL, 0x00000000);
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore[0] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore[1] = value8;
	value8 = (u8)((value8 & (~(BIT(3)))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore[2] = value8;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8);

	/* Set beacon header to  0 */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, 0x8000);

	/* Download FW to Flash flow */
	mem_offset = 0;
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));

	while (halmac_fw_size != 0) {
		if (halmac_fw_size >= (HALMAC_EXTRA_INFO_BUFF_SIZE_88XX - 48))
			send_pkt_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX - 48;
		else
			send_pkt_size = halmac_fw_size;

		status = halmac_download_rsvd_page_88xx(pHalmac_adapter, pHalmac_fw + mem_offset, send_pkt_size);

		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "halmac_download_rsvd_page_88xx Fail = %x!!\n", status);
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
			goto DLFW_FAIL;
		} else {
			/* Construct H2C Content */
			DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, 0x02);
			DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
			DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, send_pkt_size);
			DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, rom_address);

			/* Fill in H2C Header */
			h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
			h2c_header_info.content_size = 20;
			h2c_header_info.ack = _TRUE;
			halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

			/* Send H2C Cmd Packet */
			status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
			if (status != HALMAC_RET_SUCCESS) {
				PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail!!\n");
				goto DLFW_FAIL;
			}

			value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
			value8 |= BIT(0);
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);
		}

		rom_address += send_pkt_size;
		mem_offset += send_pkt_size;
		halmac_fw_size -= send_pkt_size;

		while (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0)
			PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);

		if (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "download flash fail!!\n");
			goto DLFW_FAIL;
		}
	}

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, restore[2]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, restore[1]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, restore[0]);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_flash_88xx <==========\n");
	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_DLFW_FAIL;
}

/**
 * halmac_read_flash_88xx() -read data from flash
 * @pHalmac_adapter : the adapter of halmac
 * @addr : flash start address where fw should be read
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_read_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	u8 value8;
	u8 restore[3];
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter =  pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_flash_88xx ==========>\n");

	pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;

	value8 = HALMAC_DMA_MAPPING_HIGH << 6;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXDMA_PQ_MAP + 1, value8);

	/* DLFW only use HIQ, map HIQ to hi priority */
	pHalmac_adapter->halmac_ptcl_queue[HALMAC_PTCL_QUEUE_HI] = HALMAC_DMA_MAPPING_HIGH;
	value8 = BIT_HCI_TXDMA_EN | BIT_TXDMA_EN;
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR, value8);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_H2CQ_CSR, BIT(31));

	/* Config hi priority queue and public priority queue page number (only for DLFW) */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_INFO_1, 0x200);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_RQPN_CTRL_2, HALMAC_REG_READ_32(pHalmac_adapter, REG_RQPN_CTRL_2) | BIT(31));

	if (pHalmac_adapter->halmac_interface == HALMAC_INTERFACE_SDIO) {
		HALMAC_REG_READ_32(pHalmac_adapter, REG_SDIO_FREE_TXPG);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_SDIO_TX_CTRL, 0x00000000);
	}

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR + 1);
	restore[0] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_BCN_CTRL);
	restore[1] = value8;
	value8 = (u8)((value8 & (~(BIT(3)))) | BIT(4));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, value8);
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2);
	restore[2] = value8;
	value8 = (u8)(value8 & ~(BIT(6)));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, value8);

	/* Set beacon header to  0 */
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, 0x8000);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);

	/* Construct H2C Content */
	DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, 0x03);
	DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy - pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, 4096);
	DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, addr);

	/* Fill in H2C Header */
	h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
	h2c_header_info.content_size = 16;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	/* Send H2C Cmd Packet */
	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail!!\n");
		goto DLFW_FAIL;
	}

	while (((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0)
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);

	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_FIFOPAGE_CTRL_2, (u16)(pHalmac_adapter->txff_allocation.rsvd_pg_bndy & BIT_MASK_BCN_HEAD_1_V1));
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_FWHW_TXQ_CTRL + 2, restore[2]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BCN_CTRL, restore[1]);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_CR + 1, restore[0]);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "halmac_download_flash_88xx <==========\n");
	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_FAIL;
}

/**
 * halmac_erase_flash_88xx() -erase flash data
 * @pHalmac_adapter : the adapter of halmac
 * @erase_cmd : erase command
 * @addr : flash start address where fw should be erased
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_erase_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	u8 erase_cmd,
	u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	PHALMAC_API pHalmac_api;
	u8 value8;
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = {0};
	u16 h2c_seq_mum = 0;
	u32 timeout;

	/* Construct H2C Content */
	DOWNLOAD_FLASH_SET_SPI_CMD(pH2c_buff, erase_cmd);
	DOWNLOAD_FLASH_SET_LOCATION(pH2c_buff, 0);
	DOWNLOAD_FLASH_SET_START_ADDR(pH2c_buff, addr);
	DOWNLOAD_FLASH_SET_SIZE(pH2c_buff, 0);

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I);
	value8 |= BIT(0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MCUTST_I, value8);

	/* Fill in H2C Header */
	h2c_header_info.sub_cmd_id = SUB_CMD_ID_DOWNLOAD_FLASH;
	h2c_header_info.content_size = 16;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);

	/* Send H2C Cmd Packet */
	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);

	if (status != HALMAC_RET_SUCCESS)
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "halmac_send_h2c_pkt_88xx Fail!!\n");

	timeout = 5000;
	while ((((HALMAC_REG_READ_8(pHalmac_adapter, REG_MCUTST_I)) & BIT(0)) != 0) && (timeout != 0)) {
		PLATFORM_RTL_DELAY_US(pDriver_adapter, 1000);
		timeout--;
	}

	if (timeout == 0)
		return HALMAC_RET_FAIL;
	else
		return HALMAC_RET_SUCCESS;
}

/**
 * halmac_check_flash_88xx() -check flash data
 * @pHalmac_adapter : the adapter of halmac
 * @pHalmac_fw : pointer to fw
 * @halmac_fw_size : fw size
 * @addr : flash start address where fw should be checked
 * Author : Pablo Chiu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_check_flash_88xx(
	IN PHALMAC_ADAPTER	pHalmac_adapter,
	IN u8 *pHalmac_fw,
	IN u32 halmac_fw_size,
	IN u32 addr
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8	value8;
	u16 value16, residue;
	u32 send_pkt_size, start_page, counter;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	pDriver_adapter =  pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	while (halmac_fw_size != 0) {
		start_page = ((pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy << 7) >> 12) + 0x780;
		residue = (pHalmac_adapter->txff_allocation.rsvd_h2c_extra_info_pg_bndy << 7) & (4096 - 1);

		if (halmac_fw_size >= HALMAC_EXTRA_INFO_BUFF_SIZE_88XX)
			send_pkt_size = HALMAC_EXTRA_INFO_BUFF_SIZE_88XX;
		else
			send_pkt_size = halmac_fw_size;

		halmac_read_flash_88xx(pHalmac_adapter, addr);

		value16 = HALMAC_REG_READ_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL);
		counter = 0;
		while (counter < send_pkt_size) {
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_PKTBUF_DBG_CTRL, (u16)(start_page | (value16 & 0xF000)));
			for (value16 = 0x8000 + residue; value16 <= 0x8FFF; value16++) {
				value8 = HALMAC_REG_READ_8(pHalmac_adapter, value16);

				if (*pHalmac_fw != value8) {
					PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "check flash fail!!\n");
					goto DLFW_FAIL;
				}
				pHalmac_fw++;

				counter++;
				if (counter == send_pkt_size)
					break;
			}
			residue = 0;
			start_page++;
		}
		addr += send_pkt_size;
		halmac_fw_size -= send_pkt_size;
	}

	return HALMAC_RET_SUCCESS;

DLFW_FAIL:

	return HALMAC_RET_FAIL;
}


/**
 * halmac_cfg_edca_para_88xx() - config edca parameter
 * @pHalmac_adapter : the adapter of halmac
 * @acq_id : VO/VI/BE/BK
 * @pEdca_para : aifs, cw, txop limit
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_edca_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_ACQ_ID acq_id,
	IN PHALMAC_EDCA_PARA pEdca_para
)
{
	u32 offset, value32;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_edca_88xx ==========>\n");

	switch (acq_id) {
	case HALMAC_ACQ_ID_VO:
		offset = REG_EDCA_VO_PARAM;
		break;
	case HALMAC_ACQ_ID_VI:
		offset = REG_EDCA_VI_PARAM;
		break;
	case HALMAC_ACQ_ID_BE:
		offset = REG_EDCA_BE_PARAM;
		break;
	case HALMAC_ACQ_ID_BK:
		offset = REG_EDCA_BK_PARAM;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	value32 = (pEdca_para->aifs & 0xFF) | ((pEdca_para->cw & 0xFF) << 8) | ((pEdca_para->txop_limit & 0x7FF) << 16);

	HALMAC_REG_WRITE_32(pHalmac_adapter, offset, value32);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_edca_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}
