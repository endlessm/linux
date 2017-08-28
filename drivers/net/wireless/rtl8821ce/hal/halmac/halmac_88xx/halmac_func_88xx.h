#ifndef _HALMAC_FUNC_88XX_H_
#define _HALMAC_FUNC_88XX_H_

#include "../halmac_type.h"

VOID
halmac_init_offload_feature_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_send_h2c_pkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHal_buff,
	IN u32 size,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_download_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHal_buf,
	IN u32 size
);

HALMAC_RET_STATUS
halmac_set_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN u16 *seq,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_set_fw_offload_h2c_header_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT u8 *pHal_h2c_hdr,
	IN PHALMAC_H2C_HEADER_INFO pH2c_header_info,
	OUT u16 *pSeq_num
);

HALMAC_RET_STATUS
halmac_dump_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_READ_CFG cfg
);

HALMAC_RET_STATUS
halmac_func_read_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	OUT u8 *pEfuse_map
);

HALMAC_RET_STATUS
halmac_func_write_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u8 value
);

HALMAC_RET_STATUS
halmac_func_switch_efuse_bank_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_BANK efuse_bank
);

HALMAC_RET_STATUS
halmac_read_logical_efuse_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pMap
);

HALMAC_RET_STATUS
halmac_func_write_logical_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u8 value
);

HALMAC_RET_STATUS
halmac_func_pg_efuse_by_map_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PG_EFUSE_INFO pPg_efuse_info,
	IN HALMAC_EFUSE_READ_CFG cfg
);

HALMAC_RET_STATUS
halmac_eeprom_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pPhysical_efuse_map,
	OUT u8 *pLogical_efuse_map
);

HALMAC_RET_STATUS
halmac_read_hw_efuse_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	OUT u8 *pEfuse_map
);

HALMAC_RET_STATUS
halmac_update_fw_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHamacl_fw,
	IN u32 halmac_fw_size
);

HALMAC_RET_STATUS
halmac_dlfw_to_mem_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pRam_code,
	IN u32 dest,
	IN u32 code_size
);

HALMAC_RET_STATUS
halmac_send_fwpkt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pRam_code,
	IN u32 code_size
);

HALMAC_RET_STATUS
halmac_iddma_dlfw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 source,
	IN u32 dest,
	IN u32 length,
	IN u8 first
);

HALMAC_RET_STATUS
halmac_check_fw_chksum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 memory_address
);

HALMAC_RET_STATUS
halmac_dlfw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_free_dl_fw_end_flow_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_pwr_seq_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 cut,
	IN u8 fab,
	IN u8 intf,
	IN PHALMAC_WLAN_PWR_CFG *ppPwr_seq_cfg

);

HALMAC_RET_STATUS
halmac_get_h2c_buff_free_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_send_h2c_set_pwr_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_FWLPS_OPTION pHal_FwLps_Opt
);

HALMAC_RET_STATUS
halmac_func_send_original_h2c_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *original_h2c,
	IN u16 *seq,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_media_status_rpt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 op_mode,
	IN u8 mac_id_ind,
	IN u8 mac_id,
	IN u8 mac_id_end
);

HALMAC_RET_STATUS
halmac_send_h2c_update_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type,
	IN PHALMAC_PHY_PARAMETER_INFO para_info
);

HALMAC_RET_STATUS
halmac_send_h2c_run_datapack_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_DATA_TYPE halmac_data_type
);

HALMAC_RET_STATUS
halmac_send_bt_coex_cmd_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pBt_buf,
	IN u32 bt_size,
	IN u8 ack
);

HALMAC_RET_STATUS
halmac_func_ctrl_ch_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CH_SWITCH_OPTION pCs_option
);

HALMAC_RET_STATUS
halmac_func_send_general_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_GENERAL_INFO pGeneral_info
);

HALMAC_RET_STATUS
halmac_send_h2c_ps_tuning_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_parse_c2h_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size
);

HALMAC_RET_STATUS
halmac_send_h2c_update_packet_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PACKET_ID pkt_id,
	IN u8 *pkt,
	IN u32 pkt_size
);

HALMAC_RET_STATUS
halmac_send_h2c_phy_parameter_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_PHY_PARAMETER_INFO para_info,
	IN u8 full_fifo
);

HALMAC_RET_STATUS
halmac_dump_physical_efuse_fw_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 Size,
	OUT u8 *pEfuse_map
);

HALMAC_RET_STATUS
halmac_send_h2c_update_bcn_parse_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_BCN_IE_INFO pBcn_ie_info
);

HALMAC_RET_STATUS
halmac_convert_to_sdio_bus_offset_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u32 *halmac_offset
);

HALMAC_RET_STATUS
halmac_update_sdio_free_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_update_oqt_free_space_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_EFUSE_CMD_CONSTRUCT_STATE
halmac_query_efuse_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_transition_efuse_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_EFUSE_CMD_CONSTRUCT_STATE dest_state
);

HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE
halmac_query_cfg_para_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_transition_cfg_para_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_CFG_PARA_CMD_CONSTRUCT_STATE dest_state
);

HALMAC_SCAN_CMD_CONSTRUCT_STATE
halmac_query_scan_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_transition_scan_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SCAN_CMD_CONSTRUCT_STATE dest_state
);

HALMAC_RET_STATUS
halmac_query_cfg_para_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_dump_physical_efuse_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_dump_logical_efuse_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_channel_switch_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_update_packet_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_iqk_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_power_tracking_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_query_psd_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
);

HALMAC_RET_STATUS
halmac_verify_io_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_verify_send_rsvd_page_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

VOID
halmac_power_save_cb_88xx(
	IN VOID *CbData
);

HALMAC_RET_STATUS
halmac_buffer_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 offset,
	IN u32 size,
	IN HAL_FIFO_SEL halmac_fifo_sel,
	OUT u8 *pFifo_map
);

VOID
halmac_restore_mac_register_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_RESTORE_INFO pRestore_info,
	IN u32 restore_num
);

VOID
halmac_api_record_id_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_API_ID api_id
);

HALMAC_RET_STATUS
halmac_set_usb_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_USB_MODE usb_mode
);

VOID
halmac_enable_bb_rf_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
);

VOID
halmac_config_sdio_tx_page_threshold_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_TX_PAGE_THRESHOLD_INFO pThreshold_info
);

HALMAC_RET_STATUS
halmac_rqpn_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_RQPN pPwr_seq_cfg
);

HALMAC_RET_STATUS
halmac_check_oqt_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 tx_agg_num,
	IN u8 *pHalmac_buf
);

HALMAC_RET_STATUS
halmac_pg_num_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_PG_NUM pPg_num_table
);

HALMAC_RET_STATUS
halmac_parse_intf_phy_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_PHY_PARA pIntf_phy_para,
	IN HALMAC_INTF_PHY_PLATFORM platform,
	IN HAL_INTF_PHY intf_phy
);

HALMAC_RET_STATUS
halmac_dbi_write32_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 addr,
	IN u32 data
);

u32
halmac_dbi_read32_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 addr
);

HALMAC_RET_STATUS
halmac_dbi_write8_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 addr,
	IN u8 data
);

u8
halmac_dbi_read8_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 addr
);

u16
halmac_mdio_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 addr,
	IN u8 speed

);

HALMAC_RET_STATUS
halmac_mdio_write_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 addr,
	IN u16 data,
	IN u8 speed
);

VOID
halmac_config_ampdu_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_AMPDU_CONFIG pAmpdu_config
);

HALMAC_RET_STATUS
halmac_usbphy_write_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 addr,
	IN u16 data,
	IN u8 speed
);

u16
halmac_usbphy_read_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 addr,
	IN u8 speed
);
#endif /* _HALMAC_FUNC_88XX_H_ */
