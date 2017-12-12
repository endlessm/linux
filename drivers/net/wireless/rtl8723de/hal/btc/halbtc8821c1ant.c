/* ************************************************************
 * Description:
 *
 * This file is for RTL8821C Co-exist mechanism
 *
 * History
 * 2012/11/15 Cosa first check in.
 *
 * ************************************************************ */

/* ************************************************************
 * include files
 * ************************************************************ */
#include "Mp_Precomp.h"

#if (BT_SUPPORT == 1 && COEX_SUPPORT == 1)

#if (RTL8821C_SUPPORT == 1)
/* ************************************************************
 * Global variables, these are static variables
 * ************************************************************ */
static u8	*trace_buf = &gl_btc_trace_buf[0];
static struct  coex_dm_8821c_1ant		glcoex_dm_8821c_1ant;
static struct  coex_dm_8821c_1ant	*coex_dm = &glcoex_dm_8821c_1ant;
static struct  coex_sta_8821c_1ant		glcoex_sta_8821c_1ant;
static struct  coex_sta_8821c_1ant	*coex_sta = &glcoex_sta_8821c_1ant;
static struct  psdscan_sta_8821c_1ant		gl_psd_scan_8821c_1ant;
static struct  psdscan_sta_8821c_1ant 	*psd_scan = &gl_psd_scan_8821c_1ant;
static struct	rfe_type_8821c_1ant		gl_rfe_type_8821c_1ant;
static struct	rfe_type_8821c_1ant		*rfe_type = &gl_rfe_type_8821c_1ant;


const char *const glbt_info_src_8821c_1ant[] = {
	"BT Info[wifi fw]",
	"BT Info[bt rsp]",
	"BT Info[bt auto report]",
};

u32	glcoex_ver_date_8821c_1ant = 20160316;
u32	glcoex_ver_8821c_1ant = 0x00;

/* ************************************************************
 * local function proto type if needed
 * ************************************************************
 * ************************************************************
 * local function start with halbtc8821c1ant_
 * ************************************************************ */
u8 halbtc8821c1ant_bt_rssi_state(u8 level_num, u8 rssi_thresh, u8 rssi_thresh1)
{
	s32			bt_rssi = 0;
	u8			bt_rssi_state = coex_sta->pre_bt_rssi_state;

	bt_rssi = coex_sta->bt_rssi;

	if (level_num == 2) {
		if ((coex_sta->pre_bt_rssi_state == BTC_RSSI_STATE_LOW) ||
		    (coex_sta->pre_bt_rssi_state ==
		     BTC_RSSI_STATE_STAY_LOW)) {
			if (bt_rssi >= (rssi_thresh +
					BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				bt_rssi_state = BTC_RSSI_STATE_HIGH;
			else
				bt_rssi_state = BTC_RSSI_STATE_STAY_LOW;
		} else {
			if (bt_rssi < rssi_thresh)
				bt_rssi_state = BTC_RSSI_STATE_LOW;
			else
				bt_rssi_state = BTC_RSSI_STATE_STAY_HIGH;
		}
	} else if (level_num == 3) {
		if (rssi_thresh > rssi_thresh1) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], BT Rssi thresh error!!\n");
			BTC_TRACE(trace_buf);
			return coex_sta->pre_bt_rssi_state;
		}

		if ((coex_sta->pre_bt_rssi_state == BTC_RSSI_STATE_LOW) ||
		    (coex_sta->pre_bt_rssi_state ==
		     BTC_RSSI_STATE_STAY_LOW)) {
			if (bt_rssi >= (rssi_thresh +
					BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				bt_rssi_state = BTC_RSSI_STATE_MEDIUM;
			else
				bt_rssi_state = BTC_RSSI_STATE_STAY_LOW;
		} else if ((coex_sta->pre_bt_rssi_state ==
			    BTC_RSSI_STATE_MEDIUM) ||
			   (coex_sta->pre_bt_rssi_state ==
			    BTC_RSSI_STATE_STAY_MEDIUM)) {
			if (bt_rssi >= (rssi_thresh1 +
					BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				bt_rssi_state = BTC_RSSI_STATE_HIGH;
			else if (bt_rssi < rssi_thresh)
				bt_rssi_state = BTC_RSSI_STATE_LOW;
			else
				bt_rssi_state = BTC_RSSI_STATE_STAY_MEDIUM;
		} else {
			if (bt_rssi < rssi_thresh1)
				bt_rssi_state = BTC_RSSI_STATE_MEDIUM;
			else
				bt_rssi_state = BTC_RSSI_STATE_STAY_HIGH;
		}
	}

	coex_sta->pre_bt_rssi_state = bt_rssi_state;

	return bt_rssi_state;
}

u8 halbtc8821c1ant_wifi_rssi_state(IN struct btc_coexist *btcoexist,
	   IN u8 index, IN u8 level_num, IN u8 rssi_thresh, IN u8 rssi_thresh1)
{
	s32			wifi_rssi = 0;
	u8			wifi_rssi_state = coex_sta->pre_wifi_rssi_state[index];

	btcoexist->btc_get(btcoexist, BTC_GET_S4_WIFI_RSSI, &wifi_rssi);

	if (level_num == 2) {
		if ((coex_sta->pre_wifi_rssi_state[index] == BTC_RSSI_STATE_LOW)
		    ||
		    (coex_sta->pre_wifi_rssi_state[index] ==
		     BTC_RSSI_STATE_STAY_LOW)) {
			if (wifi_rssi >= (rssi_thresh +
					  BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				wifi_rssi_state = BTC_RSSI_STATE_HIGH;
			else
				wifi_rssi_state = BTC_RSSI_STATE_STAY_LOW;
		} else {
			if (wifi_rssi < rssi_thresh)
				wifi_rssi_state = BTC_RSSI_STATE_LOW;
			else
				wifi_rssi_state = BTC_RSSI_STATE_STAY_HIGH;
		}
	} else if (level_num == 3) {
		if (rssi_thresh > rssi_thresh1) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], wifi RSSI thresh error!!\n");
			BTC_TRACE(trace_buf);
			return coex_sta->pre_wifi_rssi_state[index];
		}

		if ((coex_sta->pre_wifi_rssi_state[index] == BTC_RSSI_STATE_LOW)
		    ||
		    (coex_sta->pre_wifi_rssi_state[index] ==
		     BTC_RSSI_STATE_STAY_LOW)) {
			if (wifi_rssi >= (rssi_thresh +
					  BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				wifi_rssi_state = BTC_RSSI_STATE_MEDIUM;
			else
				wifi_rssi_state = BTC_RSSI_STATE_STAY_LOW;
		} else if ((coex_sta->pre_wifi_rssi_state[index] ==
			    BTC_RSSI_STATE_MEDIUM) ||
			   (coex_sta->pre_wifi_rssi_state[index] ==
			    BTC_RSSI_STATE_STAY_MEDIUM)) {
			if (wifi_rssi >= (rssi_thresh1 +
					  BTC_RSSI_COEX_THRESH_TOL_8821C_1ANT))
				wifi_rssi_state = BTC_RSSI_STATE_HIGH;
			else if (wifi_rssi < rssi_thresh)
				wifi_rssi_state = BTC_RSSI_STATE_LOW;
			else
				wifi_rssi_state = BTC_RSSI_STATE_STAY_MEDIUM;
		} else {
			if (wifi_rssi < rssi_thresh1)
				wifi_rssi_state = BTC_RSSI_STATE_MEDIUM;
			else
				wifi_rssi_state = BTC_RSSI_STATE_STAY_HIGH;
		}
	}

	coex_sta->pre_wifi_rssi_state[index] = wifi_rssi_state;

	return wifi_rssi_state;
}

void halbtc8821c1ant_update_ra_mask(IN struct btc_coexist *btcoexist,
				    IN boolean force_exec, IN u32 dis_rate_mask)
{
	coex_dm->cur_ra_mask = dis_rate_mask;

	if (force_exec || (coex_dm->pre_ra_mask != coex_dm->cur_ra_mask))
		btcoexist->btc_set(btcoexist, BTC_SET_ACT_UPDATE_RAMASK,
				   &coex_dm->cur_ra_mask);
	coex_dm->pre_ra_mask = coex_dm->cur_ra_mask;
}

void halbtc8821c1ant_auto_rate_fallback_retry(IN struct btc_coexist *btcoexist,
		IN boolean force_exec, IN u8 type)
{
	boolean	wifi_under_b_mode = false;

	coex_dm->cur_arfr_type = type;

	if (force_exec || (coex_dm->pre_arfr_type != coex_dm->cur_arfr_type)) {
		switch (coex_dm->cur_arfr_type) {
		case 0:	/* normal mode */
			btcoexist->btc_write_4byte(btcoexist, 0x430,
						   coex_dm->backup_arfr_cnt1);
			btcoexist->btc_write_4byte(btcoexist, 0x434,
						   coex_dm->backup_arfr_cnt2);
			break;
		case 1:
			btcoexist->btc_get(btcoexist,
					   BTC_GET_BL_WIFI_UNDER_B_MODE,
					   &wifi_under_b_mode);
			if (wifi_under_b_mode) {
				btcoexist->btc_write_4byte(btcoexist,
							   0x430, 0x0);
				btcoexist->btc_write_4byte(btcoexist,
							   0x434, 0x01010101);
			} else {
				btcoexist->btc_write_4byte(btcoexist,
							   0x430, 0x0);
				btcoexist->btc_write_4byte(btcoexist,
							   0x434, 0x04030201);
			}
			break;
		default:
			break;
		}
	}

	coex_dm->pre_arfr_type = coex_dm->cur_arfr_type;
}

void halbtc8821c1ant_retry_limit(IN struct btc_coexist *btcoexist,
				 IN boolean force_exec, IN u8 type)
{
	coex_dm->cur_retry_limit_type = type;

	if (force_exec ||
	    (coex_dm->pre_retry_limit_type !=
	     coex_dm->cur_retry_limit_type)) {
		switch (coex_dm->cur_retry_limit_type) {
		case 0:	/* normal mode */
			btcoexist->btc_write_2byte(btcoexist, 0x42a,
						   coex_dm->backup_retry_limit);
			break;
		case 1:	/* retry limit=8 */
			btcoexist->btc_write_2byte(btcoexist, 0x42a,
						   0x0808);
			break;
		default:
			break;
		}
	}

	coex_dm->pre_retry_limit_type = coex_dm->cur_retry_limit_type;
}

void halbtc8821c1ant_ampdu_max_time(IN struct btc_coexist *btcoexist,
				    IN boolean force_exec, IN u8 type)
{
	coex_dm->cur_ampdu_time_type = type;

	if (force_exec ||
	    (coex_dm->pre_ampdu_time_type != coex_dm->cur_ampdu_time_type)) {
		switch (coex_dm->cur_ampdu_time_type) {
		case 0:	/* normal mode */
			btcoexist->btc_write_1byte(btcoexist, 0x456,
					   coex_dm->backup_ampdu_max_time);
			break;
		case 1:	/* AMPDU timw = 0x38 * 32us */
			btcoexist->btc_write_1byte(btcoexist, 0x456,
						   0x38);
			break;
		default:
			break;
		}
	}

	coex_dm->pre_ampdu_time_type = coex_dm->cur_ampdu_time_type;
}

void halbtc8821c1ant_limited_tx(IN struct btc_coexist *btcoexist,
		IN boolean force_exec, IN u8 ra_mask_type, IN u8 arfr_type,
				IN u8 retry_limit_type, IN u8 ampdu_time_type)
{
	switch (ra_mask_type) {
	case 0:	/* normal mode */
		halbtc8821c1ant_update_ra_mask(btcoexist, force_exec,
					       0x0);
		break;
	case 1:	/* disable cck 1/2 */
		halbtc8821c1ant_update_ra_mask(btcoexist, force_exec,
					       0x00000003);
		break;
	case 2:	/* disable cck 1/2/5.5, ofdm 6/9/12/18/24, mcs 0/1/2/3/4 */
		halbtc8821c1ant_update_ra_mask(btcoexist, force_exec,
					       0x0001f1f7);
		break;
	default:
		break;
	}

	halbtc8821c1ant_auto_rate_fallback_retry(btcoexist, force_exec,
			arfr_type);
	halbtc8821c1ant_retry_limit(btcoexist, force_exec, retry_limit_type);
	halbtc8821c1ant_ampdu_max_time(btcoexist, force_exec, ampdu_time_type);
}

void halbtc8821c1ant_limited_rx(IN struct btc_coexist *btcoexist,
			IN boolean force_exec, IN boolean rej_ap_agg_pkt,
			IN boolean bt_ctrl_agg_buf_size, IN u8 agg_buf_size)
{
	boolean	reject_rx_agg = rej_ap_agg_pkt;
	boolean	bt_ctrl_rx_agg_size = bt_ctrl_agg_buf_size;
	u8	rx_agg_size = agg_buf_size;

	/* ============================================ */
	/*	Rx Aggregation related setting */
	/* ============================================ */
	btcoexist->btc_set(btcoexist, BTC_SET_BL_TO_REJ_AP_AGG_PKT,
			   &reject_rx_agg);
	/* decide BT control aggregation buf size or not */
	btcoexist->btc_set(btcoexist, BTC_SET_BL_BT_CTRL_AGG_SIZE,
			   &bt_ctrl_rx_agg_size);
	/* aggregation buf size, only work when BT control Rx aggregation size. */
	btcoexist->btc_set(btcoexist, BTC_SET_U1_AGG_BUF_SIZE, &rx_agg_size);
	/* real update aggregation setting */
	btcoexist->btc_set(btcoexist, BTC_SET_ACT_AGGREGATE_CTRL, NULL);


}

void halbtc8821c1ant_query_bt_info(IN struct btc_coexist *btcoexist)
{
	u8			h2c_parameter[1] = {0};

	coex_sta->c2h_bt_info_req_sent = true;

	h2c_parameter[0] |= BIT(0);	/* trigger */

	btcoexist->btc_fill_h2c(btcoexist, 0x61, 1, h2c_parameter);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], WL query BT info!!\n");
	BTC_TRACE(trace_buf);
}

void halbtc8821c1ant_monitor_bt_ctr(IN struct btc_coexist *btcoexist)
{
	u32			reg_hp_txrx, reg_lp_txrx, u32tmp;
	u32			reg_hp_tx = 0, reg_hp_rx = 0, reg_lp_tx = 0, reg_lp_rx = 0;
	static u8		num_of_bt_counter_chk = 0, cnt_slave = 0;
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;

	/* to avoid 0x76e[3] = 1 (WLAN_Act control by PTA) during IPS */
	/* if (! (btcoexist->btc_read_1byte(btcoexist, 0x76e) & 0x8) ) */

	reg_hp_txrx = 0x770;
	reg_lp_txrx = 0x774;

	u32tmp = btcoexist->btc_read_4byte(btcoexist, reg_hp_txrx);
	reg_hp_tx = u32tmp & MASKLWORD;
	reg_hp_rx = (u32tmp & MASKHWORD) >> 16;

	u32tmp = btcoexist->btc_read_4byte(btcoexist, reg_lp_txrx);
	reg_lp_tx = u32tmp & MASKLWORD;
	reg_lp_rx = (u32tmp & MASKHWORD) >> 16;

	coex_sta->high_priority_tx = reg_hp_tx;
	coex_sta->high_priority_rx = reg_hp_rx;
	coex_sta->low_priority_tx = reg_lp_tx;
	coex_sta->low_priority_rx = reg_lp_rx;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], Hi-Pri Rx/Tx: %d/%d, Lo-Pri Rx/Tx: %d/%d\n",
		    reg_hp_rx, reg_hp_tx, reg_lp_rx, reg_lp_tx);

	BTC_TRACE(trace_buf);

	/* reset counter */
	btcoexist->btc_write_1byte(btcoexist, 0x76e, 0xc);

	if ((coex_sta->low_priority_tx > 1150)  &&
	    (!coex_sta->c2h_bt_inquiry_page))
		coex_sta->pop_event_cnt++;

	if ((coex_sta->low_priority_rx >= 1150) && (coex_sta->low_priority_rx >= coex_sta->low_priority_tx)
	    && (!coex_sta->under_ips)  && (!coex_sta->c2h_bt_inquiry_page) && (coex_sta->bt_link_exist))	{
		if (cnt_slave >= 3) {
			bt_link_info->slave_role = true;
			cnt_slave = 3;
		} else
			cnt_slave++;
	} else {
		if (cnt_slave == 0)	{
			bt_link_info->slave_role = false;
			cnt_slave = 0;
		} else
			cnt_slave--;

	}

	if ((coex_sta->high_priority_tx == 0) && (coex_sta->high_priority_rx == 0) && (coex_sta->low_priority_tx == 0) &&
	    (coex_sta->low_priority_rx == 0)) {
		num_of_bt_counter_chk++;
		if (num_of_bt_counter_chk >= 3) {
			halbtc8821c1ant_query_bt_info(btcoexist);
			num_of_bt_counter_chk = 0;
		}
	}

#if 0
	/* Add Hi-Pri Tx/Rx counter to avoid false detection */
	if (((coex_sta->hid_exist) || (coex_sta->sco_exist)) &&
	    (coex_sta->high_priority_tx + coex_sta->high_priority_rx
	     >= 160)
	    && (!coex_sta->c2h_bt_inquiry_page))
		coex_sta->bt_hi_pri_link_exist = true;
	else
		coex_sta->bt_hi_pri_link_exist = false;

	if ((coex_sta->acl_busy) &&
	    (coex_sta->num_of_profile == 0)) {
		if (coex_sta->low_priority_tx +
		    coex_sta->low_priority_rx >= 160) {
			coex_sta->pan_exist = true;
			coex_sta->num_of_profile++;
			coex_sta->wrong_profile_notification++;
		}
	}
#endif

}



void halbtc8821c1ant_monitor_wifi_ctr(IN struct btc_coexist *btcoexist)
{
	s32	wifi_rssi = 0;
	boolean wifi_busy = false, wifi_under_b_mode = false;
	static u8 cck_lock_counter = 0;
	u32	total_cnt;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);
	btcoexist->btc_get(btcoexist, BTC_GET_S4_WIFI_RSSI, &wifi_rssi);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_B_MODE,
			   &wifi_under_b_mode);

	if (coex_sta->under_ips) {
		coex_sta->crc_ok_cck = 0;
		coex_sta->crc_ok_11g = 0;
		coex_sta->crc_ok_11n = 0;
		coex_sta->crc_ok_11n_agg = 0;

		coex_sta->crc_err_cck = 0;
		coex_sta->crc_err_11g = 0;
		coex_sta->crc_err_11n = 0;
		coex_sta->crc_err_11n_agg = 0;
	} else {
		coex_sta->crc_ok_cck	= btcoexist->btc_read_2byte(btcoexist,
					  0xf04);
		coex_sta->crc_ok_11g	= btcoexist->btc_read_2byte(btcoexist,
					  0xf14);
		coex_sta->crc_ok_11n	= btcoexist->btc_read_2byte(btcoexist,
					  0xf10);
		coex_sta->crc_ok_11n_agg = btcoexist->btc_read_2byte(btcoexist,
					   0xf0c);

		coex_sta->crc_err_cck	 = btcoexist->btc_read_2byte(btcoexist,0xf00) +  btcoexist->btc_read_2byte(btcoexist,0xf06);
		
		coex_sta->crc_err_11g	 = btcoexist->btc_read_2byte(btcoexist,
					   0xf16);
		coex_sta->crc_err_11n	 = btcoexist->btc_read_2byte(btcoexist,
					   0xf12);
		coex_sta->crc_err_11n_agg = btcoexist->btc_read_2byte(btcoexist,
					    0xf0e);
	}


	/* reset counter */
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0xb58, 0x1, 0x1);
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0xb58, 0x1, 0x0);

	if ((wifi_busy) && (wifi_rssi >= 30) && (!wifi_under_b_mode)) {
		total_cnt = coex_sta->crc_ok_cck + coex_sta->crc_ok_11g +
			    coex_sta->crc_ok_11n +
			    coex_sta->crc_ok_11n_agg;

		if ((coex_dm->bt_status == BT_8821C_1ANT_BT_STATUS_ACL_BUSY) ||
		    (coex_dm->bt_status ==
		     BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY) ||
		    (coex_dm->bt_status ==
		     BT_8821C_1ANT_BT_STATUS_SCO_BUSY)) {
			if (coex_sta->crc_ok_cck > (total_cnt -
						    coex_sta->crc_ok_cck)) {
				if (cck_lock_counter < 3)
					cck_lock_counter++;
			} else {
				if (cck_lock_counter > 0)
					cck_lock_counter--;
			}

		} else {
			if (cck_lock_counter > 0)
				cck_lock_counter--;
		}
	} else {
		if (cck_lock_counter > 0)
			cck_lock_counter--;
	}

	if (!coex_sta->pre_ccklock) {

		if (cck_lock_counter >= 3)
			coex_sta->cck_lock = true;
		else
			coex_sta->cck_lock = false;
	} else {
		if (cck_lock_counter == 0)
			coex_sta->cck_lock = false;
		else
			coex_sta->cck_lock = true;
	}

	if (coex_sta->cck_lock)
		coex_sta->cck_ever_lock = true;

	coex_sta->pre_ccklock =  coex_sta->cck_lock;


}

boolean halbtc8821c1ant_is_wifibt_status_changed(IN struct btc_coexist *btcoexist)
{
	static boolean	pre_wifi_busy = false, pre_under_4way = false,
			pre_bt_hs_on = false, pre_bt_off = false;
	boolean wifi_busy = false, under_4way = false, bt_hs_on = false;
	boolean wifi_connected = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_4_WAY_PROGRESS,
			   &under_4way);

	if (coex_sta->bt_disabled != pre_bt_off) {
		pre_bt_off = coex_sta->bt_disabled;

		if (coex_sta->bt_disabled)
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], BT is disabled !!\n");
		else
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], BT is enabled !!\n");

		BTC_TRACE(trace_buf);

		coex_sta->bt_coex_supported_feature = 0;
		coex_sta->bt_coex_supported_version = 0;
		return true;
	}

	if (wifi_connected) {
		if (wifi_busy != pre_wifi_busy) {
			pre_wifi_busy = wifi_busy;
			return true;
		}
		if (under_4way != pre_under_4way) {
			pre_under_4way = under_4way;
			return true;
		}
		if (bt_hs_on != pre_bt_hs_on) {
			pre_bt_hs_on = bt_hs_on;
			return true;
		}
	}

	return false;
}

void halbtc8821c1ant_update_bt_link_info(IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info	*bt_link_info = &btcoexist->bt_link_info;
	boolean				bt_hs_on = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);

	bt_link_info->bt_link_exist = coex_sta->bt_link_exist;
	bt_link_info->sco_exist = coex_sta->sco_exist;
	bt_link_info->a2dp_exist = coex_sta->a2dp_exist;
	bt_link_info->pan_exist = coex_sta->pan_exist;
	bt_link_info->hid_exist = coex_sta->hid_exist;
	bt_link_info->bt_hi_pri_link_exist = coex_sta->bt_hi_pri_link_exist;
	bt_link_info->acl_busy = coex_sta->acl_busy;

	/* work around for HS mode. */
	if (bt_hs_on) {
		bt_link_info->pan_exist = true;
		bt_link_info->bt_link_exist = true;
	}

	/* check if Sco only */
	if (bt_link_info->sco_exist &&
	    !bt_link_info->a2dp_exist &&
	    !bt_link_info->pan_exist &&
	    !bt_link_info->hid_exist)
		bt_link_info->sco_only = true;
	else
		bt_link_info->sco_only = false;

	/* check if A2dp only */
	if (!bt_link_info->sco_exist &&
	    bt_link_info->a2dp_exist &&
	    !bt_link_info->pan_exist &&
	    !bt_link_info->hid_exist)
		bt_link_info->a2dp_only = true;
	else
		bt_link_info->a2dp_only = false;

	/* check if Pan only */
	if (!bt_link_info->sco_exist &&
	    !bt_link_info->a2dp_exist &&
	    bt_link_info->pan_exist &&
	    !bt_link_info->hid_exist)
		bt_link_info->pan_only = true;
	else
		bt_link_info->pan_only = false;

	/* check if Hid only */
	if (!bt_link_info->sco_exist &&
	    !bt_link_info->a2dp_exist &&
	    !bt_link_info->pan_exist &&
	    bt_link_info->hid_exist)
		bt_link_info->hid_only = true;
	else
		bt_link_info->hid_only = false;
}

void halbtc8821c1ant_update_wifi_channel_info(IN struct btc_coexist *btcoexist,
		IN u8 type)
{
	u8			h2c_parameter[3] = {0};
	u32			wifi_bw;
	u8			wifi_central_chnl;

	/* only 2.4G we need to inform bt the chnl mask */
	btcoexist->btc_get(btcoexist, BTC_GET_U1_WIFI_CENTRAL_CHNL,
			   &wifi_central_chnl);
	if ((BTC_MEDIA_CONNECT == type) &&
	    (wifi_central_chnl <= 14)) {
		h2c_parameter[0] =
			0x1;  /* enable BT AFH skip WL channel for 8821c because BT Rx LO interference */
		/* h2c_parameter[0] = 0x0; */
		h2c_parameter[1] = wifi_central_chnl;
		btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_BW, &wifi_bw);
		if (BTC_WIFI_BW_HT40 == wifi_bw)
			h2c_parameter[2] = 0x30;
		else
			h2c_parameter[2] = 0x20;
	}

	coex_dm->wifi_chnl_info[0] = h2c_parameter[0];
	coex_dm->wifi_chnl_info[1] = h2c_parameter[1];
	coex_dm->wifi_chnl_info[2] = h2c_parameter[2];

	btcoexist->btc_fill_h2c(btcoexist, 0x66, 3, h2c_parameter);

}

u8 halbtc8821c1ant_action_algorithm(IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info	*bt_link_info = &btcoexist->bt_link_info;
	boolean				bt_hs_on = false;
	u8				algorithm = BT_8821C_1ANT_COEX_ALGO_UNDEFINED;
	u8				num_of_diff_profile = 0;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);

	if (!bt_link_info->bt_link_exist) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], No BT link exists!!!\n");
		BTC_TRACE(trace_buf);
		return algorithm;
	}

	if (bt_link_info->sco_exist)
		num_of_diff_profile++;
	if (bt_link_info->hid_exist)
		num_of_diff_profile++;
	if (bt_link_info->pan_exist)
		num_of_diff_profile++;
	if (bt_link_info->a2dp_exist)
		num_of_diff_profile++;

	if (num_of_diff_profile == 1) {
		if (bt_link_info->sco_exist) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], BT Profile = SCO only\n");
			BTC_TRACE(trace_buf);
			algorithm = BT_8821C_1ANT_COEX_ALGO_SCO;
		} else {
			if (bt_link_info->hid_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = HID only\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_HID;
			} else if (bt_link_info->a2dp_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = A2DP only\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_A2DP;
			} else if (bt_link_info->pan_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = PAN(HS) only\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANHS;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = PAN(EDR) only\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR;
				}
			}
		}
	} else if (num_of_diff_profile == 2) {
		if (bt_link_info->sco_exist) {
			if (bt_link_info->hid_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = SCO + HID\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_HID;
			} else if (bt_link_info->a2dp_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = SCO + A2DP ==> SCO\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_SCO;
			} else if (bt_link_info->pan_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm = BT_8821C_1ANT_COEX_ALGO_SCO;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + PAN(EDR)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_HID;
				}
			}
		} else {
			if (bt_link_info->hid_exist &&
			    bt_link_info->a2dp_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = HID + A2DP\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_HID_A2DP;
			} else if (bt_link_info->hid_exist &&
				   bt_link_info->pan_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = HID + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_HID_A2DP;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = HID + PAN(EDR)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_HID;
				}
			} else if (bt_link_info->pan_exist &&
				   bt_link_info->a2dp_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = A2DP + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_A2DP_PANHS;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = A2DP + PAN(EDR)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_A2DP;
				}
			}
		}
	} else if (num_of_diff_profile == 3) {
		if (bt_link_info->sco_exist) {
			if (bt_link_info->hid_exist &&
			    bt_link_info->a2dp_exist) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT Profile = SCO + HID + A2DP ==> HID\n");
				BTC_TRACE(trace_buf);
				algorithm = BT_8821C_1ANT_COEX_ALGO_HID;
			} else if (bt_link_info->hid_exist &&
				   bt_link_info->pan_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + HID + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_HID_A2DP;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + HID + PAN(EDR)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_HID;
				}
			} else if (bt_link_info->pan_exist &&
				   bt_link_info->a2dp_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + A2DP + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm = BT_8821C_1ANT_COEX_ALGO_SCO;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + A2DP + PAN(EDR) ==> HID\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_HID;
				}
			}
		} else {
			if (bt_link_info->hid_exist &&
			    bt_link_info->pan_exist &&
			    bt_link_info->a2dp_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = HID + A2DP + PAN(HS)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_HID_A2DP;
				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = HID + A2DP + PAN(EDR)\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_HID_A2DP_PANEDR;
				}
			}
		}
	} else if (num_of_diff_profile >= 3) {
		if (bt_link_info->sco_exist) {
			if (bt_link_info->hid_exist &&
			    bt_link_info->pan_exist &&
			    bt_link_info->a2dp_exist) {
				if (bt_hs_on) {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], Error!!! BT Profile = SCO + HID + A2DP + PAN(HS)\n");
					BTC_TRACE(trace_buf);

				} else {
					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"[BTCoex], BT Profile = SCO + HID + A2DP + PAN(EDR)==>PAN(EDR)+HID\n");
					BTC_TRACE(trace_buf);
					algorithm =
						BT_8821C_1ANT_COEX_ALGO_PANEDR_HID;
				}
			}
		}
	}

	return algorithm;
}

void halbtc8821c1ant_set_bt_auto_report(IN struct btc_coexist *btcoexist,
					IN boolean enable_auto_report)
{
	u8			h2c_parameter[1] = {0};

	h2c_parameter[0] = 0;

	if (enable_auto_report)
		h2c_parameter[0] |= BIT(0);

	btcoexist->btc_fill_h2c(btcoexist, 0x68, 1, h2c_parameter);
}

void halbtc8821c1ant_bt_auto_report(IN struct btc_coexist *btcoexist,
		    IN boolean force_exec, IN boolean enable_auto_report)
{
	coex_dm->cur_bt_auto_report = enable_auto_report;

	if (!force_exec) {
		if (coex_dm->pre_bt_auto_report == coex_dm->cur_bt_auto_report)
			return;
	}
	halbtc8821c1ant_set_bt_auto_report(btcoexist,
					   coex_dm->cur_bt_auto_report);

	coex_dm->pre_bt_auto_report = coex_dm->cur_bt_auto_report;
}

void halbtc8821c1ant_set_fw_low_penalty_ra(IN struct btc_coexist
		*btcoexist, IN boolean low_penalty_ra)
{
	u8			h2c_parameter[6] = {0};

	h2c_parameter[0] = 0x6;	/* op_code, 0x6= Retry_Penalty */

	if (low_penalty_ra) {
		h2c_parameter[1] |= BIT(0);
		h2c_parameter[2] =
			0x00;  /* normal rate except MCS7/6/5, OFDM54/48/36 */
		h2c_parameter[3] = 0xf7;  /* MCS7 or OFDM54 */
		h2c_parameter[4] = 0xf8;  /* MCS6 or OFDM48 */
		h2c_parameter[5] = 0xf9;	/* MCS5 or OFDM36	 */
	}

	btcoexist->btc_fill_h2c(btcoexist, 0x69, 6, h2c_parameter);
}

void halbtc8821c1ant_low_penalty_ra(IN struct btc_coexist *btcoexist,
			    IN boolean force_exec, IN boolean low_penalty_ra)
{
	coex_dm->cur_low_penalty_ra = low_penalty_ra;

	if (!force_exec) {
		if (coex_dm->pre_low_penalty_ra == coex_dm->cur_low_penalty_ra)
			return;
	}
	halbtc8821c1ant_set_fw_low_penalty_ra(btcoexist,
					      coex_dm->cur_low_penalty_ra);

	coex_dm->pre_low_penalty_ra = coex_dm->cur_low_penalty_ra;
}

void halbtc8821c1ant_sw_mechanism(IN struct btc_coexist *btcoexist,
				  IN boolean low_penalty_ra)
{
	halbtc8821c1ant_low_penalty_ra(btcoexist, NORMAL_EXEC, low_penalty_ra);
}

void halbtc8821c1ant_write_score_board(
	IN	struct  btc_coexist		*btcoexist,
	IN	u16				bitpos,
	IN	BOOLEAN		state
)
{

	static u16 originalval = 0x8002;

	if (state)
		originalval = originalval | bitpos;
	else
		originalval = originalval & (~bitpos);

	btcoexist->btc_write_2byte(btcoexist, 0xaa, originalval);
}

void halbtc8821c1ant_read_score_board(
	IN	struct  btc_coexist		*btcoexist,
	IN   u16				*score_board_val
)
{

	*score_board_val = (btcoexist->btc_read_2byte(btcoexist,
			    0xaa)) & 0x7fff;
}

void halbtc8821c1ant_post_activestate_to_bt(
	IN	struct  btc_coexist		*btcoexist,
	IN	boolean				wifi_active
)
{

	if (wifi_active)
		halbtc8821c1ant_write_score_board(btcoexist, (u16) BIT(0), TRUE);
	else
		halbtc8821c1ant_write_score_board(btcoexist, (u16) BIT(0), FALSE);		
}


void halbtc8821c1ant_post_onoffstate_to_bt(
	IN	struct  btc_coexist		*btcoexist,
	IN	boolean				wifi_on
)
{

	if (wifi_on)
		halbtc8821c1ant_write_score_board(btcoexist, (u16) BIT(1), TRUE);
	else
		halbtc8821c1ant_write_score_board(btcoexist, (u16) BIT(1), FALSE);
}

void halbtc8821c1ant_monitor_bt_enable_disable(IN struct btc_coexist *btcoexist)
{
	static u32		bt_disable_cnt = 0;
	boolean			bt_active = true, bt_disabled = false;
	u16			u16tmp;

	/* This function check if bt is disabled */
#if 0
	if (coex_sta->high_priority_tx == 0 &&
	    coex_sta->high_priority_rx == 0 &&
	    coex_sta->low_priority_tx == 0 &&
	    coex_sta->low_priority_rx == 0)
		bt_active = false;
	if (coex_sta->high_priority_tx == 0xffff &&
	    coex_sta->high_priority_rx == 0xffff &&
	    coex_sta->low_priority_tx == 0xffff &&
	    coex_sta->low_priority_rx == 0xffff)
		bt_active = false;


#else

	/* Read BT on/off status from scoreboard[1], enable this only if BT patch support this feature */
	halbtc8821c1ant_read_score_board(btcoexist,	&u16tmp);

	bt_active = u16tmp & BIT(1);


#endif

	if (bt_active) {
		bt_disable_cnt = 0;
		bt_disabled = false;
		btcoexist->btc_set(btcoexist, BTC_SET_BL_BT_DISABLE,
				   &bt_disabled);
	} else {

		bt_disable_cnt++;
		if (bt_disable_cnt >= 2) {
			bt_disabled = true;
			bt_disable_cnt = 2;
		}

		btcoexist->btc_set(btcoexist, BTC_SET_BL_BT_DISABLE,
				   &bt_disabled);
	}


	if (coex_sta->bt_disabled != bt_disabled) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT is from %s to %s!!\n",
			    (coex_sta->bt_disabled ? "disabled" : "enabled"),
			    (bt_disabled ? "disabled" : "enabled"));
		BTC_TRACE(trace_buf);
		coex_sta->bt_disabled = bt_disabled;
	}

}

void halbtc8821c1ant_enable_gnt_to_gpio(IN struct btc_coexist *btcoexist,
					boolean isenable)
{
	static u8 			bitVal[5] = {0,0,0,0,0};
	static boolean 		state = false;

	if (state ==isenable)
		return;
	else
		state = isenable;

	if (isenable) {

		/* enable GNT_WL, GNT_BT to GPIO for debug  */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x73, 0x8, 0x1);

		/* store original value  */
		bitVal[0] = (btcoexist->btc_read_1byte(btcoexist, 0x66) & BIT(4)) >>4;	/*0x66[4] */
		bitVal[1] = (btcoexist->btc_read_1byte(btcoexist, 0x67) & BIT(0));		/*0x66[8] */
		bitVal[2] = (btcoexist->btc_read_1byte(btcoexist, 0x42) & BIT(3)) >> 3;  /*0x40[19] */
		bitVal[3] = (btcoexist->btc_read_1byte(btcoexist, 0x65) & BIT(7)) >> 7;  /*0x64[15] */
		bitVal[4] = (btcoexist->btc_read_1byte(btcoexist, 0x72) & BIT(2)) >> 2;  /*0x70[18] */

		/*  switch GPIO Mux */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x66, BIT(4), 0x0);  /*0x66[4] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x67, BIT(0), 0x0);  /*0x66[8] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x42, BIT(3), 0x0);  /*0x40[19] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x65, BIT(7), 0x0);  /*0x64[15] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x72, BIT(2), 0x0);  /*0x70[18] = 0 */
		

	} else {
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x73, 0x8, 0x0);

		/*  Restore original value   */
		/*  switch GPIO Mux */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x66, BIT(4), bitVal[0]);  /*0x66[4] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x67, BIT(0), bitVal[1]);  /*0x66[8] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x42, BIT(3), bitVal[2]);  /*0x40[19] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x65, BIT(7), bitVal[3]);  /*0x64[15] = 0 */
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x72, BIT(2), bitVal[4]);  /*0x70[18] = 0 */
	}

}

u32 halbtc8821c1ant_ltecoex_indirect_read_reg(IN struct btc_coexist *btcoexist,
		IN u16 reg_addr)
{
	u32 j = 0;


	/* wait for ready bit before access 0x1700		 */
	btcoexist->btc_write_4byte(btcoexist, 0x1700, 0x800F0000 | reg_addr);

	do {
		j++;
	} while (((btcoexist->btc_read_1byte(btcoexist,
					     0x1703)&BIT(5)) == 0) &&
		 (j < BT_8821C_1ANT_LTECOEX_INDIRECTREG_ACCESS_TIMEOUT));


	return btcoexist->btc_read_4byte(btcoexist,
					 0x1708);  /* get read data */

}

void halbtc8821c1ant_ltecoex_indirect_write_reg(IN struct btc_coexist *btcoexist,
		IN u16 reg_addr, IN u32 bit_mask, IN u32 reg_value)
{
	u32 val, i = 0, j = 0, bitpos = 0;


	if (bit_mask == 0x0)
		return;
	if (bit_mask == 0xffffffff) {
		btcoexist->btc_write_4byte(btcoexist, 0x1704,
					   reg_value); /* put write data */

		/* wait for ready bit before access 0x1700 */
		do {
			j++;
		} while (((btcoexist->btc_read_1byte(btcoexist,
						     0x1703)&BIT(5)) == 0) &&
			(j < BT_8821C_1ANT_LTECOEX_INDIRECTREG_ACCESS_TIMEOUT));


		btcoexist->btc_write_4byte(btcoexist, 0x1700,
					   0xc00F0000 | reg_addr);
	} else {
		for (i = 0; i <= 31; i++) {
			if (((bit_mask >> i) & 0x1) == 0x1) {
				bitpos = i;
				break;
			}
		}

		/* read back register value before write */
		val = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
				reg_addr);
		val = (val & (~bit_mask)) | (reg_value << bitpos);

		btcoexist->btc_write_4byte(btcoexist, 0x1704,
					   val); /* put write data */

		/* wait for ready bit before access 0x1700		 */
		do {
			j++;
		} while (((btcoexist->btc_read_1byte(btcoexist,
						     0x1703)&BIT(5)) == 0) &&
			(j < BT_8821C_1ANT_LTECOEX_INDIRECTREG_ACCESS_TIMEOUT));


		btcoexist->btc_write_4byte(btcoexist, 0x1700,
					   0xc00F0000 | reg_addr);

	}

}

void halbtc8821c1ant_ltecoex_enable(IN struct btc_coexist *btcoexist,
				    IN boolean enable)
{
	u8 val;

	val = (enable) ? 1 : 0;
	halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist, 0x38, 0x80,
			val);  /* 0x38[7] */

}

void halbtc8821c1ant_ltecoex_pathcontrol_owner(IN struct btc_coexist *btcoexist,
		IN boolean wifi_control)
{
	u8 val;

	val = (wifi_control) ? 1 : 0;
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0x73, 0x4,
					   val); /* 0x70[26] */

}

void halbtc8821c1ant_ltecoex_set_gnt_bt(IN struct btc_coexist *btcoexist,
			IN u8 control_block, IN boolean sw_control, IN u8 state)
{
	u32 val = 0, bit_mask;

	state = state & 0x1;
	val = (sw_control) ? ((state << 1) | 0x1) : 0;

	switch (control_block) {
	case BT_8821C_1ANT_GNT_BLOCK_RFC_BB:
	default:
		bit_mask = 0xc000;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[15:14] */
		bit_mask = 0x0c00;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[11:10]						 */
		break;
	case BT_8821C_1ANT_GNT_BLOCK_RFC:
		bit_mask = 0xc000;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[15:14] */
		break;
	case BT_8821C_1ANT_GNT_BLOCK_BB:
		bit_mask = 0x0c00;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[11:10] */
		break;

	}

}

void halbtc8821c1ant_ltecoex_set_gnt_wl(IN struct btc_coexist *btcoexist,
			IN u8 control_block, IN boolean sw_control, IN u8 state)
{
	u32 val = 0, bit_mask;

	state = state & 0x1;
	val = (sw_control) ? ((state << 1) | 0x1) : 0;

	switch (control_block) {
	case BT_8821C_1ANT_GNT_BLOCK_RFC_BB:
	default:
		bit_mask = 0x3000;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[13:12] */
		bit_mask = 0x0300;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[9:8]						 */
		break;
	case BT_8821C_1ANT_GNT_BLOCK_RFC:
		bit_mask = 0x3000;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[13:12] */
		break;
	case BT_8821C_1ANT_GNT_BLOCK_BB:
		bit_mask = 0x0300;
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist,
				0x38, bit_mask, val); /* 0x38[9:8] */
		break;

	}

}

void halbtc8821c1ant_ltecoex_set_coex_table(IN struct btc_coexist *btcoexist,
		IN u8 table_type, IN u16 table_content)
{
	u16 reg_addr = 0x0000;

	switch (table_type) {
	case BT_8821C_1ANT_CTT_WL_VS_LTE:
		reg_addr = 0xa0;
		break;
	case BT_8821C_1ANT_CTT_BT_VS_LTE:
		reg_addr = 0xa4;
		break;
	}

	if (reg_addr != 0x0000)
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist, reg_addr,
			0xffff, table_content); /* 0xa0[15:0] or 0xa4[15:0] */


}


void halbtc8821c1ant_ltecoex_set_break_table(IN struct btc_coexist *btcoexist,
		IN u8 table_type, IN u8 table_content)
{
	u16 reg_addr = 0x0000;

	switch (table_type) {
	case BT_8821C_1ANT_LBTT_WL_BREAK_LTE:
		reg_addr = 0xa8;
		break;
	case BT_8821C_1ANT_LBTT_BT_BREAK_LTE:
		reg_addr = 0xac;
		break;
	case BT_8821C_1ANT_LBTT_LTE_BREAK_WL:
		reg_addr = 0xb0;
		break;
	case BT_8821C_1ANT_LBTT_LTE_BREAK_BT:
		reg_addr = 0xb4;
		break;
	}

	if (reg_addr != 0x0000)
		halbtc8821c1ant_ltecoex_indirect_write_reg(btcoexist, reg_addr,
			0xff, table_content); /* 0xa8[15:0] or 0xb4[15:0] */


}

void halbtc8821c1ant_set_coex_table(IN struct btc_coexist *btcoexist,
	    IN u32 val0x6c0, IN u32 val0x6c4, IN u32 val0x6c8, IN u8 val0x6cc)
{
	btcoexist->btc_write_4byte(btcoexist, 0x6c0, val0x6c0);

	btcoexist->btc_write_4byte(btcoexist, 0x6c4, val0x6c4);

	btcoexist->btc_write_4byte(btcoexist, 0x6c8, val0x6c8);

	btcoexist->btc_write_1byte(btcoexist, 0x6cc, val0x6cc);
}

void halbtc8821c1ant_coex_table(IN struct btc_coexist *btcoexist,
			IN boolean force_exec, IN u32 val0x6c0, IN u32 val0x6c4,
				IN u32 val0x6c8, IN u8 val0x6cc)
{
	coex_dm->cur_val0x6c0 = val0x6c0;
	coex_dm->cur_val0x6c4 = val0x6c4;
	coex_dm->cur_val0x6c8 = val0x6c8;
	coex_dm->cur_val0x6cc = val0x6cc;

	if (!force_exec) {
		if ((coex_dm->pre_val0x6c0 == coex_dm->cur_val0x6c0) &&
		    (coex_dm->pre_val0x6c4 == coex_dm->cur_val0x6c4) &&
		    (coex_dm->pre_val0x6c8 == coex_dm->cur_val0x6c8) &&
		    (coex_dm->pre_val0x6cc == coex_dm->cur_val0x6cc))
			return;
	}

	halbtc8821c1ant_set_coex_table(btcoexist, val0x6c0, val0x6c4, val0x6c8,
				       val0x6cc);

	coex_dm->pre_val0x6c0 = coex_dm->cur_val0x6c0;
	coex_dm->pre_val0x6c4 = coex_dm->cur_val0x6c4;
	coex_dm->pre_val0x6c8 = coex_dm->cur_val0x6c8;
	coex_dm->pre_val0x6cc = coex_dm->cur_val0x6cc;
}

void halbtc8821c1ant_coex_table_with_type(IN struct btc_coexist *btcoexist,
		IN boolean force_exec, IN u8 type)
{
	u32	break_table;
	u8	select_table;

	coex_sta->coex_table_type = type;

	if (coex_sta->concurrent_rx_mode_on == true) {
		break_table = 0xf0ffffff;  /* set WL hi-pri can break BT */
		select_table =
			0xb;		/* set Tx response = Hi-Pri (ex: Transmitting ACK,BA,CTS) */
	} else {
		break_table = 0xffffff;
		select_table = 0x3;
	}

	switch (type) {
	case 0:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55555555, 0x55555555, break_table,
					   select_table);
		break;
	case 1:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55555555, 0x5a5a5a5a, break_table,
					   select_table);
		break;
	case 2:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0xaa5a5a5a, 0xaa5a5a5a, break_table,
					   select_table);
		break;
	case 3:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0xaa555555, 0xaa5a5a5a, break_table,
					   select_table);
		break;
	case 4:
		halbtc8821c1ant_coex_table(btcoexist,
					   force_exec, 0xaa555555, 0xaa5a5a5a,
					   break_table, select_table);
		break;
	case 5:
		halbtc8821c1ant_coex_table(btcoexist,
					   force_exec, 0x5a5a5a5a, 0x5a5a5a5a,
					   break_table, select_table);
		break;
	case 6:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55555555, 0xaaaaaaaa, break_table,
					   select_table);
		break;
	case 7:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0xaaaaaaaa, 0xaaaaaaaa, break_table,
					   select_table);
		break;
	case 8:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 9:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 10:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 11:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 12:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 13:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x5fff5fff, 0xaaaaaaaa, break_table,
					   select_table);
		break;
	case 14:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x5fff5fff, 0x5ada5ada, break_table,
					   select_table);
		break;
	case 15:
		halbtc8821c1ant_coex_table(btcoexist, force_exec,
					   0x55dd55dd, 0xaaaaaaaa, break_table,
					   select_table);
		break;
	default:
		break;
	}
}

void halbtc8821c1ant_set_fw_ignore_wlan_act(IN struct btc_coexist *btcoexist,
		IN boolean enable)
{
	u8			h2c_parameter[1] = {0};

	if (enable) {
		h2c_parameter[0] |= BIT(0);		/* function enable */
	}

	btcoexist->btc_fill_h2c(btcoexist, 0x63, 1, h2c_parameter);
}

void halbtc8821c1ant_ignore_wlan_act(IN struct btc_coexist *btcoexist,
				     IN boolean force_exec, IN boolean enable)
{
	coex_dm->cur_ignore_wlan_act = enable;

	if (!force_exec) {
		if (coex_dm->pre_ignore_wlan_act ==
		    coex_dm->cur_ignore_wlan_act)
			return;
	}
	halbtc8821c1ant_set_fw_ignore_wlan_act(btcoexist, enable);

	coex_dm->pre_ignore_wlan_act = coex_dm->cur_ignore_wlan_act;
}

void halbtc8821c1ant_set_lps_rpwm(IN struct btc_coexist *btcoexist,
				  IN u8 lps_val, IN u8 rpwm_val)
{
	u8	lps = lps_val;
	u8	rpwm = rpwm_val;

	btcoexist->btc_set(btcoexist, BTC_SET_U1_LPS_VAL, &lps);
	btcoexist->btc_set(btcoexist, BTC_SET_U1_RPWM_VAL, &rpwm);
}

void halbtc8821c1ant_lps_rpwm(IN struct btc_coexist *btcoexist,
		      IN boolean force_exec, IN u8 lps_val, IN u8 rpwm_val)
{
	coex_dm->cur_lps = lps_val;
	coex_dm->cur_rpwm = rpwm_val;

	if (!force_exec) {
		if ((coex_dm->pre_lps == coex_dm->cur_lps) &&
		    (coex_dm->pre_rpwm == coex_dm->cur_rpwm))
			return;
	}
	halbtc8821c1ant_set_lps_rpwm(btcoexist, lps_val, rpwm_val);

	coex_dm->pre_lps = coex_dm->cur_lps;
	coex_dm->pre_rpwm = coex_dm->cur_rpwm;
}

void halbtc8821c1ant_ps_tdma_check_for_power_save_state(
	IN struct btc_coexist *btcoexist, IN boolean new_ps_state)
{
	u8	lps_mode = 0x0;
	u8	h2c_parameter[5] = {0x8, 0, 0, 0, 0};

	btcoexist->btc_get(btcoexist, BTC_GET_U1_LPS_MODE, &lps_mode);

	if (lps_mode) {	/* already under LPS state */
		if (new_ps_state) {
			/* keep state under LPS, do nothing. */
		} else {
			/* will leave LPS state, turn off psTdma first */
			/*halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false,
						8); */
			btcoexist->btc_fill_h2c(btcoexist, 0x60, 5, h2c_parameter);
		}
	} else {					/* NO PS state */
		if (new_ps_state) {
			/* will enter LPS state, turn off psTdma first */
			/*halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false,
						8);*/
			btcoexist->btc_fill_h2c(btcoexist, 0x60, 5, h2c_parameter);
		} else {
			/* keep state under NO PS state, do nothing. */
		}
	}
}

void halbtc8821c1ant_power_save_state(IN struct btc_coexist *btcoexist,
			      IN u8 ps_type, IN u8 lps_val, IN u8 rpwm_val)
{
	boolean		low_pwr_disable = false;

	switch (ps_type) {
	case BTC_PS_WIFI_NATIVE:
		/* recover to original 32k low power setting */
		coex_sta->force_lps_on = false;
		low_pwr_disable = false;
		btcoexist->btc_set(btcoexist,
				   BTC_SET_ACT_DISABLE_LOW_POWER,
				   &low_pwr_disable);
		btcoexist->btc_set(btcoexist, BTC_SET_ACT_NORMAL_LPS,
				   NULL);

		break;
	case BTC_PS_LPS_ON:
		coex_sta->force_lps_on = true;
		halbtc8821c1ant_ps_tdma_check_for_power_save_state(
			btcoexist, true);
		halbtc8821c1ant_lps_rpwm(btcoexist, NORMAL_EXEC,
					 lps_val, rpwm_val);
		/* when coex force to enter LPS, do not enter 32k low power. */
		low_pwr_disable = true;
		btcoexist->btc_set(btcoexist,
				   BTC_SET_ACT_DISABLE_LOW_POWER,
				   &low_pwr_disable);
		/* power save must executed before psTdma.			 */
		btcoexist->btc_set(btcoexist, BTC_SET_ACT_ENTER_LPS,
				   NULL);

		break;
	case BTC_PS_LPS_OFF:
		coex_sta->force_lps_on = false;
		halbtc8821c1ant_ps_tdma_check_for_power_save_state(
			btcoexist, false);
		btcoexist->btc_set(btcoexist, BTC_SET_ACT_LEAVE_LPS,
				   NULL);

		break;
	default:
		break;
	}
}


void halbtc8821c1ant_set_fw_pstdma(IN struct btc_coexist *btcoexist,
	   IN u8 byte1, IN u8 byte2, IN u8 byte3, IN u8 byte4, IN u8 byte5)
{
	u8			h2c_parameter[5] = {0};
	u8			real_byte1 = byte1, real_byte5 = byte5;
	boolean			ap_enable = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_AP_MODE_ENABLE,
			   &ap_enable);

	if (ap_enable) {
		if (byte1 & BIT(4) && !(byte1 & BIT(5))) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], FW for 1Ant AP mode\n");
			BTC_TRACE(trace_buf);
			real_byte1 &= ~BIT(4);
			real_byte1 |= BIT(5);

			real_byte5 |= BIT(5);
			real_byte5 &= ~BIT(6);

			halbtc8821c1ant_power_save_state(btcoexist, BTC_PS_WIFI_NATIVE, 0x0,
							 0x0);
		}
	} else if (byte1 & BIT(4) && !(byte1 & BIT(5))) {

		halbtc8821c1ant_power_save_state(
			btcoexist, BTC_PS_LPS_ON, 0x50,
			0x4);
	} else {
		halbtc8821c1ant_power_save_state(btcoexist, BTC_PS_WIFI_NATIVE, 0x0,
						 0x0);
	}


	h2c_parameter[0] = real_byte1;
	h2c_parameter[1] = byte2;
	h2c_parameter[2] = byte3;
	h2c_parameter[3] = byte4;
	h2c_parameter[4] = real_byte5;

	coex_dm->ps_tdma_para[0] = real_byte1;
	coex_dm->ps_tdma_para[1] = byte2;
	coex_dm->ps_tdma_para[2] = byte3;
	coex_dm->ps_tdma_para[3] = byte4;
	coex_dm->ps_tdma_para[4] = real_byte5;

	btcoexist->btc_fill_h2c(btcoexist, 0x60, 5, h2c_parameter);
}


void halbtc8821c1ant_ps_tdma(IN struct btc_coexist *btcoexist,
		     IN boolean force_exec, IN boolean turn_on, IN u8 type)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	struct  btc_board_info	*board_info = &btcoexist->board_info;
	boolean			wifi_busy = false;
	static u8			psTdmaByte4Modify = 0x0, pre_psTdmaByte4Modify = 0x0;
	static boolean	 pre_wifi_busy = false;


#if BT_8821C_1ANT_ANTDET_ENABLE
#if BT_8821C_1ANT_ANTDET_COEXMECHANISMSWITCH_ENABLE
	if (board_info->btdm_ant_num_by_ant_det == 2) {
		if (turn_on)
			type = type +
			       100; /* for WiFi RSSI low or BT RSSI low */
	}

#endif
#endif

	coex_dm->cur_ps_tdma_on = turn_on;
	coex_dm->cur_ps_tdma = type;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);

	if (wifi_busy != pre_wifi_busy) {
		force_exec = true;
		pre_wifi_busy = wifi_busy;
	}

	/* 0x778 = 0x1 at wifi slot (no blocking BT Low-Pri pkts) */
	if ((bt_link_info->slave_role) && (bt_link_info->a2dp_exist))
		psTdmaByte4Modify = 0x1;
	else
		psTdmaByte4Modify = 0x0;

	if (pre_psTdmaByte4Modify != psTdmaByte4Modify) {

		force_exec = true;
		pre_psTdmaByte4Modify = psTdmaByte4Modify;
	}

	if (!force_exec) {
		if ((coex_dm->pre_ps_tdma_on == coex_dm->cur_ps_tdma_on) &&
		    (coex_dm->pre_ps_tdma == coex_dm->cur_ps_tdma))
			return;
	}

	if (coex_dm->cur_ps_tdma_on) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], ********** TDMA(on, %d) **********\n",
			    coex_dm->cur_ps_tdma);
		BTC_TRACE(trace_buf);
	} else {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], ********** TDMA(off, %d) **********\n",
			    coex_dm->cur_ps_tdma);
		BTC_TRACE(trace_buf);
	}


	if (turn_on) {

		btcoexist->btc_write_1byte_bitmask(btcoexist, 0x550, 0x8,
					   0x1);  /* enable TBTT nterrupt */
		
		switch (type) {
		default:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x35, 0x03, 0x11, 0x11);
			break;

		case 3:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x51, 0x3a, 0x03, 0x10, 0x10);
			break;
		case 4:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x51, 0x21, 0x03, 0x10, 0x10);
			break;
		case 5:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x15, 0x03, 0x11, 0x11);
			break;
		case 6:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x20, 0x03, 0x11, 0x11);
			break;
		case 7:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x51, 0x10, 0x03, 0x10,  0x14 | psTdmaByte4Modify);
			break;
		case 8:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x51, 0x10, 0x03, 0x10,  0x14 | psTdmaByte4Modify);
			break;
		case 13:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x51, 0x25, 0x03, 0x10,  0x10 | psTdmaByte4Modify);
			break;
		case 14:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x51, 0x15, 0x03, 0x10,  0x10 | psTdmaByte4Modify);
			break;
		case 15:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x51, 0x20, 0x03, 0x10,  0x10 | psTdmaByte4Modify);
			break;
		case 17:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0x61, 0x10, 0x03, 0x11,  0x14 | psTdmaByte4Modify);
			break;;
		case 19:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x15, 0x03, 0x11, 0x10);
			break;
		case 20:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x30, 0x03, 0x11, 0x10);
			break;
		case 21:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x30, 0x03, 0x11, 0x10);
			break;
		case 22:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x25, 0x03, 0x11, 0x10);
			break;
		case 32:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x35, 0x03, 0x11, 0x11);
			break;
		case 33:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0x61, 0x35, 0x03, 0x11, 0x10);
			break;

		/*     1-Ant to 2-Ant      TDMA case */
		case 103:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xd3, 0x3a, 0x03, 0x70, 0x10);
			break;
		case 104:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xd3, 0x21, 0x03, 0x70, 0x10);
			break;
		case 105:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x15, 0x03, 0x71, 0x11);
			break;
		case 106:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x20, 0x03, 0x71, 0x11);
			break;
		case 107:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xd3, 0x10, 0x03, 0x70,  0x14 | psTdmaByte4Modify);
			break;
		case 108:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xd3, 0x10, 0x03, 0x70,  0x14 | psTdmaByte4Modify);
			break;
		case 113:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xd3, 0x25, 0x03, 0x70,  0x10 | psTdmaByte4Modify);
			break;
		case 114:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xd3, 0x15, 0x03, 0x70,  0x10 | psTdmaByte4Modify);
			break;
		case 115:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xd3, 0x20, 0x03, 0x70,  0x10 | psTdmaByte4Modify);
			break;
		case 117:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
				0xe3, 0x10, 0x03, 0x71,  0x14 | psTdmaByte4Modify);
			break;;
		case 119:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x15, 0x03, 0x71, 0x10);
			break;
		case 120:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x30, 0x03, 0x71, 0x10);
			break;
		case 121:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x30, 0x03, 0x71, 0x10);
			break;
		case 122:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x25, 0x03, 0x71, 0x10);
			break;
		case 132:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x35, 0x03, 0x71, 0x11);
			break;
		case 133:
			halbtc8821c1ant_set_fw_pstdma(btcoexist,
					      0xe3, 0x35, 0x03, 0x71, 0x10);
			break;

		}
	} else {

		/* disable PS tdma */
		switch (type) {
		case 8: /* PTA Control */
			halbtc8821c1ant_set_fw_pstdma(btcoexist, 0x8,
						      0x0, 0x0, 0x0, 0x0);
			break;
		case 0:
		default:  /* Software control, Antenna at BT side */
			halbtc8821c1ant_set_fw_pstdma(btcoexist, 0x0,
						      0x0, 0x0, 0x0, 0x0);
			break;
		case 1: /* 2-Ant, 0x778=3, antenna control by antenna diversity */
			halbtc8821c1ant_set_fw_pstdma(btcoexist, 0x0,
						      0x0, 0x0, 0x48, 0x0);
			break;
		}
	}

	/* update pre state */
	coex_dm->pre_ps_tdma_on = coex_dm->cur_ps_tdma_on;
	coex_dm->pre_ps_tdma = coex_dm->cur_ps_tdma;
}

void halbtc8821c1ant_set_int_block(IN struct btc_coexist *btcoexist, IN boolean force_exec,  IN u8 pos_type)
{
	u8		regval_0xcba;
	u32		u32tmp1 = 0;

	 coex_dm->cur_int_block_status = pos_type;
	 	
	if (!force_exec) {
		if (coex_dm->pre_int_block_status == coex_dm->cur_int_block_status)
			return;	
	} 

	coex_dm->pre_int_block_status = coex_dm->cur_int_block_status;

	regval_0xcba =  btcoexist->btc_read_1byte(btcoexist, 0xcba);

	switch(pos_type){

		case BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_BTG:
			regval_0xcba =  regval_0xcba & (~(BIT(2)) ) | BIT(0);  			/* 0xcb8[16] = 1, 0xcb8[18] = 0, WL_G select BTG */
			break;
		case BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_WLAG:
			regval_0xcba =   regval_0xcba & (~(BIT(2) | BIT(0)) );   		/* 0xcb8[16] = 0, 0xcb8[18] = 0, WL_G select WLAG */
			regval_0xcba =   regval_0xcba | BIT(4) | BIT(5) ;  				/* 0xcb8[21:20] = 2b'11, WL_G @ WLAG on */
			regval_0xcba =    regval_0xcba & (~(BIT(7)) ) | BIT(6);		/* 0xcb8[23:22] = 2b'01, WL_A @ WLAG off */
			
			break;
		case BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLA_OF_WLAG:
			regval_0xcba =   regval_0xcba & (~(BIT(2) | BIT(0)) );    		/* 0xcb8[16] = 0, 0xcb8[18] = 0, WL_G select WLAG */
			regval_0xcba =   regval_0xcba & (~(BIT(5))) | BIT(4);			/* 0xcb8[21:20] = 2b'01, WL_G @ WLAG off */
			regval_0xcba =   regval_0xcba | BIT(6) | BIT(7);  	 	  		/* 0xcb8[23:22] = 2b'11, WL_A @ WLAG on */
			
			break;
	}

	btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcba, 0xff, regval_0xcba);

	u32tmp1 = btcoexist->btc_read_4byte(btcoexist, 0xcb8);
			
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (After Int Block setup) 0xcb8 = 0x%08x **********\n",
		    u32tmp1);
	BTC_TRACE(trace_buf);

}

void halbtc8821c1ant_set_ext_band_switch(IN struct btc_coexist *btcoexist, IN boolean force_exec, IN u8 pos_type)
{

#if 0
	boolean	switch_polatiry_inverse = false;
	u8		regval_0xcb6; 
	u32		u32tmp1 = 0, u32tmp2 = 0;
	
	if (!rfe_type->ext_band_switch_exist)
		return;	

	 coex_dm->cur_ext_band_switch_status = pos_type;
	 	
	if (!force_exec) {
		if (coex_dm->pre_ext_band_switch_status == coex_dm->cur_ext_band_switch_status)
			return;	
	} 

	coex_dm->pre_ext_band_switch_status = coex_dm->cur_ext_band_switch_status;

	/* swap control polarity if use different switch control polarity*/
	switch_polatiry_inverse = (rfe_type->ext_band_switch_ctrl_polarity == 1? ~switch_polatiry_inverse: switch_polatiry_inverse); 

	/*swap control polarity for WL_A, default polarity 0xcb4[21] = 0 && 0xcb4[23] = 1 is for WL_G */
	switch_polatiry_inverse =  (pos_type == BT_8821C_1ANT_EXT_BAND_SWITCH_TO_WLA? ~switch_polatiry_inverse: switch_polatiry_inverse); 

	regval_0xcb6 =  btcoexist->btc_read_1byte(btcoexist, 0xcb6);

	/* for normal switch polrity, 0xcb4[21] =1 && 0xcb4[23] = 0 for WL_A, vice versa  */
	regval_0xcb6 = (switch_polatiry_inverse == 1? ( (regval_0xcb6 & (~(BIT(7))) ) | BIT(5)) : ( (regval_0xcb6 & (~(BIT(5))) ) | BIT(7)) ); 
	
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb6, 0xff, regval_0xcb6);	

	u32tmp1 = btcoexist->btc_read_4byte(btcoexist, 0xcb0);
	u32tmp2 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
			
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (After Ext Band switch setup) 0xcb0 = 0x%08x, 0xcb4 = 0x%08x**********\n",
		    u32tmp1, u32tmp2);
	BTC_TRACE(trace_buf);
#endif	

}

void halbtc8821c1ant_set_ext_ant_switch(IN struct btc_coexist *btcoexist, IN boolean force_exec, IN u8 ctrl_type, IN u8 pos_type)
{
	struct  btc_board_info	*board_info = &btcoexist->board_info;
	boolean	switch_polatiry_inverse = false;
	u8		regval_0xcb7 = 0, regval_0x64;
	u32		u32tmp1 = 0, u32tmp2 = 0, u32tmp3 = 0;
	
	if (!rfe_type->ext_ant_switch_exist)
		return;	

	 coex_dm->cur_ext_ant_switch_status = (ctrl_type << 8)  + pos_type;
	 	
	if (!force_exec) {
		if (coex_dm->pre_ext_ant_switch_status == coex_dm->cur_ext_ant_switch_status)
			return;	
	} 

	coex_dm->pre_ext_ant_switch_status = coex_dm->cur_ext_ant_switch_status;

	/* swap control polarity if use different switch control polarity*/
	/*  Normal switch polarity for DPDT, 0xcb4[29:28] = 2b'01 => BTG to Main, WLG to Aux,  0xcb4[29:28] = 2b'10 => BTG to Aux, WLG to Main */
	/*  Normal switch polarity for SPDT, 0xcb4[29:28] = 2b'01 => Ant to BTG,  0xcb4[29:28] = 2b'10 => Ant to WLG */
	switch_polatiry_inverse = (rfe_type->ext_ant_switch_ctrl_polarity == 1? ~switch_polatiry_inverse: switch_polatiry_inverse); 

	/* swap control polarity if 1-Ant at Aux  */
	if ( (rfe_type->ext_ant_switch_type == BT_8821C_1ANT_EXT_ANT_SWITCH_USE_DPDT) && (board_info->single_ant_path == 0) ) 
		switch_polatiry_inverse =  ~switch_polatiry_inverse;
	
	switch(pos_type)
	{	
		default:
		case BT_8821C_1ANT_EXT_ANT_SWITCH_TO_BT:
		case BT_8821C_1ANT_EXT_ANT_SWITCH_TO_NOCARE:
			
			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_TO_WLG:
			switch_polatiry_inverse =  (rfe_type->wlg_Locate_at_btg == 0?  ~switch_polatiry_inverse: switch_polatiry_inverse);
			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_TO_WLA:
			/*switch_polatiry_inverse =  (rfe_type->ext_band_switch_exist == 1?  ~switch_polatiry_inverse: switch_polatiry_inverse);*/
			break;
	}
	

	switch(ctrl_type)
	{
		default:
		case BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BBSW:
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4e, 0x80, 0x0);  /*  0x4c[23] = 0 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4f, 0x01, 0x1);  /* 0x4c[24] = 1  */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb4, 0xff, 0x77);	/* BB SW, DPDT use RFE_ctrl8 and RFE_ctrl9 as cotrol pin */

			regval_0xcb7 = (switch_polatiry_inverse == false?  0x1 : 0x2);     /* 0xcb4[29:28] = 2b'01 for no switch_polatiry_inverse, DPDT_SEL_N =1, DPDT_SEL_P =0   */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb7, 0x30, regval_0xcb7);

			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_PTA:
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4e, 0x80, 0x0);  /* 0x4c[23] = 0 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4f, 0x01, 0x1);  /* 0x4c[24] = 1 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb4, 0xff, 0x66);	/* PTA,  DPDT use RFE_ctrl8 and RFE_ctrl9 as cotrol pin */

			regval_0xcb7 = (switch_polatiry_inverse == false?  0x2 : 0x1);     /* 0xcb4[29:28] = 2b'10 for no switch_polatiry_inverse, DPDT_SEL_N =1, DPDT_SEL_P =0  @ GNT_BT=1  */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb7, 0x30, regval_0xcb7);
			
			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_ANTDIV:
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4e, 0x80, 0x0);  /* 0x4c[23] = 0 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4f, 0x01, 0x1);  /* 0x4c[24] = 1 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb4, 0xff, 0x88);  /* */

			/* no regval_0xcb7 setup required, because  antenna switch control value by antenna diversity */

			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_MAC:
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4e, 0x80, 0x1);  /*  0x4c[23] = 1 */

			regval_0x64 = (switch_polatiry_inverse == false?  0x0 : 0x1);     /* 0x64[0] = 1b'0 for no switch_polatiry_inverse, DPDT_SEL_N =1, DPDT_SEL_P =0 */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x64, 0x1, regval_0x64);
			break;
		case BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BT:
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4e, 0x80, 0x0);  /* 0x4c[23] = 0  */
			btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4f, 0x01, 0x0);  /* 0x4c[24] = 0  */

			/* no  setup required, because  antenna switch control value by BT vendor 0xac[1:0]  */
			break;
	}

		u32tmp1 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
		u32tmp2 = btcoexist->btc_read_4byte(btcoexist, 0x4c);
		u32tmp3 = btcoexist->btc_read_4byte(btcoexist, 0x64) & 0xff;
			
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (After Ext Ant switch setup) 0xcb4 = 0x%08x, 0x4c = 0x%08x, 0x64= 0x%02x**********\n",
		    u32tmp1, u32tmp2, u32tmp3);
		BTC_TRACE(trace_buf);


}

void halbtc8821c1ant_set_rfe_type(IN struct btc_coexist *btcoexist)
{
	struct  btc_board_info *board_info = &btcoexist->board_info;

#if 0
	rfe_type->ext_band_switch_exist = false;
	rfe_type->ext_band_switch_type = BT_8821C_1ANT_EXT_BAND_SWITCH_USE_SPDT;     // SPDT;
	rfe_type->ext_band_switch_ctrl_polarity = 0;

	if (rfe_type->ext_band_switch_exist) {

		/* band switch use RFE_ctrl1 (pin name: PAPE_A) and RFE_ctrl3 (pin name: LNAON_A)  */

		/* set RFE_ctrl1 as software control  */		
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb0, 0xf0, 0x7);

		/* set RFE_ctrl3 as software control  */	
		btcoexist->btc_write_1byte_bitmask(btcoexist, 0xcb1, 0xf0, 0x7);

		}
#endif

	/* the following setup should be got from Efuse in the future */
	rfe_type->rfe_module_type = board_info->rfe_type;
	
	rfe_type->ext_ant_switch_ctrl_polarity = 0;
	rfe_type->ext_ant_switch_diversity = false;

	switch(rfe_type->rfe_module_type)
	{
		case 0:
		default:	
			rfe_type->ext_ant_switch_exist = true;
			rfe_type->ext_ant_switch_type = BT_8821C_1ANT_EXT_ANT_SWITCH_USE_DPDT;          /*2-Ant, DPDT, WLG*/
			rfe_type->wlg_Locate_at_btg = false;
			rfe_type->ext_ant_switch_diversity = true;
			break;
		case 1:
			rfe_type->ext_ant_switch_exist = true;
			rfe_type->ext_ant_switch_type = BT_8821C_1ANT_EXT_ANT_SWITCH_USE_SPDT;          /*1-Ant, Main, DPDT or SPDT, WLG   */
			rfe_type->wlg_Locate_at_btg = false;
			break;
		case 2:
			rfe_type->ext_ant_switch_exist = true;
			rfe_type->ext_ant_switch_type = BT_8821C_1ANT_EXT_ANT_SWITCH_USE_SPDT;            /*1-Ant, Main, DPDT or SPDT, BTG   */
			rfe_type->wlg_Locate_at_btg = true;
			break;	
		case 3:
			rfe_type->ext_ant_switch_exist = true;
			rfe_type->ext_ant_switch_type = BT_8821C_1ANT_EXT_ANT_SWITCH_USE_DPDT;          /*1-Ant, Aux, DPDT, WLG   */
			rfe_type->wlg_Locate_at_btg = false;
			break;
		case 4:
			rfe_type->ext_ant_switch_exist = true;
			rfe_type->ext_ant_switch_type = BT_8821C_1ANT_EXT_ANT_SWITCH_USE_DPDT;          /*1-Ant, Aux, DPDT, BTG   */;
			rfe_type->wlg_Locate_at_btg = true;
			break;	
		case 5:
			rfe_type->ext_ant_switch_exist = false;										 /*2-Ant, no antenna switch, WLG*/
			rfe_type->wlg_Locate_at_btg = false;	

	}


	if (rfe_type->wlg_Locate_at_btg) 
		halbtc8821c1ant_set_int_block(btcoexist, FORCE_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_BTG);
	else
		halbtc8821c1ant_set_int_block(btcoexist, FORCE_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_WLAG);

	
}


void halbtc8821c1ant_set_ant_path(IN struct btc_coexist *btcoexist,
	  IN u8 ant_pos_type, IN boolean force_exec,
	  IN u8 phase  )
{
	struct  btc_board_info *board_info = &btcoexist->board_info;
	u32			cnt_bt_cal_chk = 0;
	boolean			is_in_mp_mode = false;
	u8			u8tmp = 0;
	u32			u32tmp1 = 0, u32tmp2 = 0, u32tmp3 = 0;
	u16			u16tmp1 = 0;

#if BT_8821C_1ANT_ANTDET_ENABLE
#if BT_8821C_1ANT_ANTDET_COEXMECHANISMSWITCH_ENABLE
	if (ant_pos_type == BTC_ANT_PATH_PTA) {
		if ((board_info->btdm_ant_det_finish) &&
		    (board_info->btdm_ant_num_by_ant_det == 2)) {
			if (board_info->btdm_ant_pos ==
			    BTC_ANTENNA_AT_MAIN_PORT)
				ant_pos_type = BTC_ANT_PATH_WIFI;
			else
				ant_pos_type = BTC_ANT_PATH_BT;
		}
	}
#endif
#endif

	 coex_dm->cur_ant_pos_type = (ant_pos_type << 8)  + phase;

	if (!force_exec) {
		if (coex_dm->cur_ant_pos_type == coex_dm->pre_ant_pos_type)
			return;	
	} 
	
	coex_dm->pre_ant_pos_type = coex_dm->cur_ant_pos_type;

#if 1
	u32tmp1 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0x38);
	u32tmp2 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0x54);
	u8tmp  = btcoexist->btc_read_1byte(btcoexist, 0x73);

	u32tmp3 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (Before Ant Setup) 0xcb4 = 0x%x, 0x73 = 0x%x, 0x38= 0x%x, 0x54= 0x%x**********\n",
		    u32tmp3, u8tmp, u32tmp1, u32tmp2);
	BTC_TRACE(trace_buf);
#endif


	switch (phase)
	{
		case BT_8821C_1ANT_PHASE_COEX_INIT:
				/* Disable LTE Coex Function in WiFi side (this should be on if LTE coex is required) */
				halbtc8821c1ant_ltecoex_enable(btcoexist, 0x0);

				/* GNT_WL_LTE always = 1 (this should be config if LTE coex is required) */
				halbtc8821c1ant_ltecoex_set_coex_table(btcoexist,
									       BT_8821C_1ANT_CTT_WL_VS_LTE, 0xffff);

				/* GNT_BT_LTE always = 1 (this should be config if LTE coex is required) */
				halbtc8821c1ant_ltecoex_set_coex_table(btcoexist,
						       BT_8821C_1ANT_CTT_BT_VS_LTE, 0xffff);

				/* Wait If BT IQK running, because Path control owner is at BT during BT IQK (setup by WiFi firmware) */
				while (cnt_bt_cal_chk <= 20) {
							u8tmp = btcoexist->btc_read_1byte(btcoexist,
														  0x49d);
							cnt_bt_cal_chk++;
							if (u8tmp & BIT(0)) {
										BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
										"[BTCoex], ########### BT is calibrating (wait cnt=%d) ###########\n",
				 	 				      cnt_bt_cal_chk);
										BTC_TRACE(trace_buf);
										delay_ms(50);
							} else {
										BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
											"[BTCoex], ********** BT is NOT calibrating (wait cnt=%d)**********\n",
					    											cnt_bt_cal_chk);
										BTC_TRACE(trace_buf);
										break;
							}
				}

				/* set Path control owner to WL at initial step */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
							BT_8821C_1ANT_PCO_WLSIDE);

				/* set GNT_BT to SW high */
				halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
							BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
							BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);
				/* Set GNT_WL to SW low */
				halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
						      BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					  	      BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
						      BT_8821C_1ANT_SIG_STA_SET_TO_LOW);

				coex_sta->run_time_state = false;

				if (BTC_ANT_PATH_AUTO == ant_pos_type)
					ant_pos_type = BTC_ANT_PATH_BT;

			break;
		case BT_8821C_1ANT_PHASE_WLANONLY_INIT:
  			      /* Disable LTE Coex Function in WiFi side (this should be on if LTE coex is required) */
				halbtc8821c1ant_ltecoex_enable(btcoexist, 0x0);

				/* GNT_WL_LTE always = 1 (this should be config if LTE coex is required) */
				halbtc8821c1ant_ltecoex_set_coex_table(btcoexist,
									       BT_8821C_1ANT_CTT_WL_VS_LTE, 0xffff);

				/* GNT_BT_LTE always = 1 (this should be config if LTE coex is required) */
				halbtc8821c1ant_ltecoex_set_coex_table(btcoexist,
						       BT_8821C_1ANT_CTT_BT_VS_LTE, 0xffff);				

				/* set Path control owner to WL at initial step */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
							BT_8821C_1ANT_PCO_WLSIDE);

				/* set GNT_BT to SW Low */
				halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
							BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
							BT_8821C_1ANT_SIG_STA_SET_TO_LOW);
				/* Set GNT_WL to SW high */
				halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
						      BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					  	      BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
						      BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);

				coex_sta->run_time_state = false;

				if (BTC_ANT_PATH_AUTO == ant_pos_type)
					ant_pos_type = BTC_ANT_PATH_WIFI;

			break;
		case BT_8821C_1ANT_PHASE_WLAN_OFF:
				/* Disable LTE Coex Function in WiFi side */
				halbtc8821c1ant_ltecoex_enable(btcoexist, 0x0);

				/* set Path control owner to BT */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
						BT_8821C_1ANT_PCO_BTSIDE);

				/* Set Ext Ant Switch to BT control at wifi off step  */
				halbtc8821c1ant_set_ext_ant_switch(btcoexist, 
						   FORCE_EXEC, 
						   BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BT, 
					   	   BT_8821C_1ANT_EXT_ANT_SWITCH_TO_NOCARE);
				
				coex_sta->run_time_state = false;
			break;
		case BT_8821C_1ANT_PHASE_2G_RUNTIME:

				/* set Path control owner to WL at runtime step */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
							BT_8821C_1ANT_PCO_WLSIDE);
	
				/* set GNT_BT to PTA */
				halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
				  		    BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		BT_8821C_1ANT_GNT_TYPE_CTRL_BY_PTA,
					   		BT_8821C_1ANT_SIG_STA_SET_BY_HW);

				if (rfe_type->wlg_Locate_at_btg)  /* Set GNT_WL to PTA */				
					halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
								BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
							   	BT_8821C_1ANT_GNT_TYPE_CTRL_BY_PTA,
							   	BT_8821C_1ANT_SIG_STA_SET_BY_HW);
				else  /* Set GNT_WL to SW High */				
					halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
								BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
							   	BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
							   	BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);	

				coex_sta->run_time_state = true;

				if (BTC_ANT_PATH_AUTO == ant_pos_type){
					if (rfe_type->wlg_Locate_at_btg) 
						ant_pos_type = BTC_ANT_PATH_WIFI;
					else
						ant_pos_type = BTC_ANT_PATH_PTA;
				}				

				if (rfe_type->wlg_Locate_at_btg) 
					halbtc8821c1ant_set_int_block(btcoexist, NORMAL_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_BTG);
				else
					halbtc8821c1ant_set_int_block(btcoexist, NORMAL_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_WLAG);	

			break;
		case BT_8821C_1ANT_PHASE_5G_RUNTIME:	

				/* set Path control owner to WL at runtime step */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
							BT_8821C_1ANT_PCO_WLSIDE);

				/* set GNT_BT to SW Hi */
				halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
							   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   		   BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);
				
				/* Set GNT_WL to SW Hi */
				halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
							   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   		   BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);

				coex_sta->run_time_state = true;

				if (BTC_ANT_PATH_AUTO == ant_pos_type) {
				/*	if (rfe_type->ext_band_switch_exist)	
						ant_pos_type =   BTC_ANT_PATH_PTA;
					else */
						ant_pos_type = BTC_ANT_PATH_WIFI5G;
				}

				halbtc8821c1ant_set_int_block(btcoexist, NORMAL_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLA_OF_WLAG);

			break;
		case BT_8821C_1ANT_PHASE_BTMPMODE:
				/* Disable LTE Coex Function in WiFi side */
				halbtc8821c1ant_ltecoex_enable(btcoexist, 0x0);

				/* set Path control owner to WL */
				halbtc8821c1ant_ltecoex_pathcontrol_owner(btcoexist,
						BT_8821C_1ANT_PCO_WLSIDE);

				/* set GNT_BT to SW Hi */
				halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
							   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   		   BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);
				
				/* Set GNT_WL to SW Lo */
				halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
							   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   		   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   		   BT_8821C_1ANT_SIG_STA_SET_TO_LOW);

				coex_sta->run_time_state = false;
				
				/* Set Ext Ant Switch to BT side at BT MP mode  */
				if (BTC_ANT_PATH_AUTO == ant_pos_type) 
					ant_pos_type = BTC_ANT_PATH_BT;
				
			break;
	}

		if (phase != BT_8821C_1ANT_PHASE_WLAN_OFF)
			{
				switch (ant_pos_type) {
							case BTC_ANT_PATH_WIFI:
								halbtc8821c1ant_set_ext_ant_switch( btcoexist, 
													force_exec, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BBSW, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_TO_WLG);
								break;
							case BTC_ANT_PATH_WIFI5G:
								halbtc8821c1ant_set_ext_ant_switch( btcoexist, 
													force_exec, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BBSW, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_TO_WLA);
								break;
							case BTC_ANT_PATH_BT:
								halbtc8821c1ant_set_ext_ant_switch( btcoexist, 
													force_exec, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BBSW, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_TO_BT);			
								break;
							default:
							case BTC_ANT_PATH_PTA:
								halbtc8821c1ant_set_ext_ant_switch( btcoexist, 
													force_exec, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_PTA, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_TO_NOCARE);			
								break;
					}
	
			}
#if 1
	u32tmp1 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38);
	u32tmp2 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x54);
	u32tmp3 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
	u8tmp  = btcoexist->btc_read_1byte(btcoexist, 0x73);
	
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], ********** (After Ant-Setup phase---%d) 0xcb4 = 0x%x, 0x73 = 0x%x, 0x38= 0x%x, 0x54= 0x%x**********\n",
			    phase, u32tmp3, u8tmp, u32tmp1, u32tmp2);
		
		BTC_TRACE(trace_buf);
#endif
}


boolean halbtc8821c1ant_is_common_action(IN struct btc_coexist *btcoexist)
{
	boolean			common = false, wifi_connected = false, wifi_busy = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);

	if (!wifi_connected &&
	    BT_8821C_1ANT_BT_STATUS_NON_CONNECTED_IDLE ==
	    coex_dm->bt_status) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], Wifi non connected-idle + BT non connected-idle!!\n");
		BTC_TRACE(trace_buf);

		/* halbtc8821c1ant_sw_mechanism(btcoexist, false); */

		common = true;
	} else if (wifi_connected &&
		   (BT_8821C_1ANT_BT_STATUS_NON_CONNECTED_IDLE ==
		    coex_dm->bt_status)) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], Wifi connected + BT non connected-idle!!\n");
		BTC_TRACE(trace_buf);

		/* halbtc8821c1ant_sw_mechanism(btcoexist, false); */

		common = true;
	} else if (!wifi_connected &&
		(BT_8821C_1ANT_BT_STATUS_CONNECTED_IDLE == coex_dm->bt_status)) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], Wifi non connected-idle + BT connected-idle!!\n");
		BTC_TRACE(trace_buf);

		/* halbtc8821c1ant_sw_mechanism(btcoexist, false); */

		common = true;
	} else if (wifi_connected &&
		(BT_8821C_1ANT_BT_STATUS_CONNECTED_IDLE == coex_dm->bt_status)) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Wifi connected + BT connected-idle!!\n");
		BTC_TRACE(trace_buf);

		/* halbtc8821c1ant_sw_mechanism(btcoexist, false); */

		common = true;
	} else if (!wifi_connected &&
		(BT_8821C_1ANT_BT_STATUS_CONNECTED_IDLE != coex_dm->bt_status)) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Wifi non connected-idle + BT Busy!!\n");
		BTC_TRACE(trace_buf);

		/* halbtc8821c1ant_sw_mechanism(btcoexist, false); */

		common = true;
	} else {
		if (wifi_busy) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Wifi Connected-Busy + BT Busy!!\n");
			BTC_TRACE(trace_buf);
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Wifi Connected-Idle + BT Busy!!\n");
			BTC_TRACE(trace_buf);
		}

		common = false;
	}

	return common;
}


/* *********************************************
 *
 *	Software Coex Mechanism start
 *
 * ********************************************* */

/* SCO only or SCO+PAN(HS) */

/*
void halbtc8821c1ant_action_sco(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, true);
}


void halbtc8821c1ant_action_hid(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, true);
}


void halbtc8821c1ant_action_a2dp(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, false);
}

void halbtc8821c1ant_action_a2dp_pan_hs(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, false);
}

void halbtc8821c1ant_action_pan_edr(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, false);
}


void halbtc8821c1ant_action_pan_hs(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, false);
}


void halbtc8821c1ant_action_pan_edr_a2dp(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, false);
}

void halbtc8821c1ant_action_pan_edr_hid(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, true);
}


void halbtc8821c1ant_action_hid_a2dp_pan_edr(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, true);
}

void halbtc8821c1ant_action_hid_a2dp(IN struct btc_coexist* btcoexist)
{
	halbtc8821c1ant_sw_mechanism(btcoexist, true);
}

*/

/* *********************************************
 *
 *	Non-Software Coex Mechanism start
 *
 * ********************************************* */
void halbtc8821c1ant_action_bt_whck_test(IN struct btc_coexist *btcoexist)
{
	halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
	
	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, NORMAL_EXEC,
				     BT_8821C_1ANT_PHASE_2G_RUNTIME);
	
	halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 0);
}

void halbtc8821c1ant_action_wifi_under5g(IN struct btc_coexist *btcoexist)
{

	halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 8);
	halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 0);
	
	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, FORCE_EXEC,
				      BT_8821C_1ANT_PHASE_5G_RUNTIME);
}


void halbtc8821c1ant_action_wifi_only(IN struct btc_coexist *btcoexist)
{
	boolean wifi_under_5g = false;

    btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_5G, &wifi_under_5g);
	if(wifi_under_5g)
	{
		halbtc8821c1ant_action_wifi_under5g(btcoexist);
			
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,	
		    	"[BTCoex], ********** (wlan only -- under 5g ) **********\n");
		BTC_TRACE(trace_buf);
		return;
	}

	halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 8);
	
	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, FORCE_EXEC,
				      BT_8821C_1ANT_PHASE_2G_RUNTIME);
	
	halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 0);
}

void halbtc8821c1ant_action_wifi_multi_port(IN struct btc_coexist *btcoexist)
{
	halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
	
	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, NORMAL_EXEC,
				     BT_8821C_1ANT_PHASE_2G_RUNTIME);
	
	halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 2);
}

void halbtc8821c1ant_action_hs(IN struct btc_coexist *btcoexist)
{
	halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 5);

	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, NORMAL_EXEC,
				     BT_8821C_1ANT_PHASE_2G_RUNTIME);
	
	halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 2);
}

void halbtc8821c1ant_action_bt_inquiry(IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean			wifi_connected = false, ap_enable = false, wifi_busy = false,
				bt_busy = false;
	boolean	wifi_scan = false, wifi_link = false, wifi_roam = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_AP_MODE_ENABLE,
			   &ap_enable);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);
	btcoexist->btc_set(btcoexist, BTC_SET_BL_BT_TRAFFIC_BUSY, &bt_busy);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_SCAN, &wifi_scan);

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_LINK, &wifi_link);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_ROAM, &wifi_roam);

	if ((wifi_link) || (wifi_roam) || (coex_sta->wifi_is_high_pri_task)) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 33);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 6);

	} else if ((wifi_scan) && (coex_sta->bt_create_connection)) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 22);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 6);

	} else if ((!wifi_connected) && (!wifi_scan)) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 0);
	} else if ((bt_link_info->a2dp_exist) && (bt_link_info->pan_exist)) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 22);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);

	} else if (bt_link_info->a2dp_exist) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 32);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
	} else if (wifi_scan) {

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 20);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);

	} else if (wifi_busy) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 21);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
	} else {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 19);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
	}
}

void halbtc8821c1ant_action_bt_sco_hid_only_busy(IN struct btc_coexist
		*btcoexist, IN u8 wifi_status)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean	wifi_connected = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);

	/* tdma and coex table */

	if (bt_link_info->sco_exist) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 5);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 5);
	} else { /* HID */
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 6);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 5);
	}
}

void halbtc8821c1ant_action_wifi_connected_bt_acl_busy(IN struct btc_coexist
		*btcoexist, IN u8 wifi_status)
{

	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean			wifi_busy = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);

	if (bt_link_info->hid_only) { /* HID */
		halbtc8821c1ant_action_bt_sco_hid_only_busy(btcoexist,
				wifi_status);
		coex_dm->auto_tdma_adjust = false;
		return;
	} else if (bt_link_info->a2dp_only) { /* A2DP		 */
		if (BT_8821C_1ANT_WIFI_STATUS_CONNECTED_IDLE == wifi_status) {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						32);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
			coex_dm->auto_tdma_adjust = false;
		} else {

			if (coex_sta->scan_ap_num >=
			    BT_8821C_1ANT_WIFI_NOISY_THRESH)  {
				halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
							17);
			}	else {
				halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
							7);
			}

			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
			coex_dm->auto_tdma_adjust = true;
		}
	} else if (((bt_link_info->a2dp_exist) && (bt_link_info->pan_exist)) ||
		   (bt_link_info->hid_exist && bt_link_info->a2dp_exist &&
		bt_link_info->pan_exist)) { /* A2DP+PAN(OPP,FTP), HID+A2DP+PAN(OPP,FTP) */

		if (wifi_busy)	{

			if ((coex_sta->a2dp_bit_pool > 40) && (coex_sta->a2dp_bit_pool < 255))
				halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 15);
			else
				halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 13);
		} else
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 14);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 1);
		coex_dm->auto_tdma_adjust = false;
	} else if (bt_link_info->hid_exist &&
		   bt_link_info->a2dp_exist) { /* HID+A2DP */
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 8);
		coex_dm->auto_tdma_adjust = false;

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 1);
	} else if ((bt_link_info->pan_only) || (bt_link_info->hid_exist &&
		bt_link_info->pan_exist)) { /* PAN(OPP,FTP), HID+PAN(OPP,FTP)			 */
		if (BT_8821C_1ANT_WIFI_STATUS_CONNECTED_IDLE == wifi_status)
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						4);
		else
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 3);

		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 1);
		coex_dm->auto_tdma_adjust = false;
	} else {
		/* BT no-profile busy (0x9) */
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 33);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
		coex_dm->auto_tdma_adjust = false;
	}
}

void halbtc8821c1ant_action_wifi_not_connected(IN struct btc_coexist *btcoexist)
{
	/* tdma and coex table */
	halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 8);
	
	halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO, NORMAL_EXEC,
				    BT_8821C_1ANT_PHASE_2G_RUNTIME );
	
	halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 0);
}

void halbtc8821c1ant_action_wifi_not_connected_scan(IN struct btc_coexist
		*btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean wifi_connected = false, bt_hs_on = false;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0;
	boolean	bt_ctrl_agg_buf_size = false;
	u8	agg_buf_size = 5;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_LINK_STATUS,
			   &wifi_link_status);

	num_of_wifi_link = wifi_link_status >> 16;

	if (num_of_wifi_link >= 2) {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);
		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false,
					   bt_ctrl_agg_buf_size, agg_buf_size);

		if (coex_sta->c2h_bt_inquiry_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"############# [BTCoex],  BT Is Inquirying\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_action_bt_inquiry(btcoexist);
		} else
			halbtc8821c1ant_action_wifi_multi_port(btcoexist);
		return;
	}

	if (coex_sta->c2h_bt_inquiry_page) {
		halbtc8821c1ant_action_bt_inquiry(btcoexist);
		return;
	} else if (bt_hs_on) {
		halbtc8821c1ant_action_hs(btcoexist);
		return;
	}

	/* tdma and coex table */
	if (BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) {
		if (bt_link_info->a2dp_exist) {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						32);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		} else if (bt_link_info->a2dp_exist &&
			   bt_link_info->pan_exist) {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						22);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		} else {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 20);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		}
	} else if ((BT_8821C_1ANT_BT_STATUS_SCO_BUSY == coex_dm->bt_status) ||
		   (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY ==
		    coex_dm->bt_status)) {
		halbtc8821c1ant_action_bt_sco_hid_only_busy(btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_SCAN);
	} else {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
		
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
		
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 2);
	}
}

void halbtc8821c1ant_action_wifi_connected_scan(IN struct btc_coexist
		*btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean wifi_connected = false, bt_hs_on = false;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0;
	boolean	bt_ctrl_agg_buf_size = false;
	u8	agg_buf_size = 5;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_LINK_STATUS,
			   &wifi_link_status);

	num_of_wifi_link = wifi_link_status >> 16;

	if (num_of_wifi_link >= 2) {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);
		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false,
					   bt_ctrl_agg_buf_size, agg_buf_size);

		if (coex_sta->c2h_bt_inquiry_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"############# [BTCoex],  BT Is Inquirying\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_action_bt_inquiry(btcoexist);
		} else
			halbtc8821c1ant_action_wifi_multi_port(btcoexist);
		return;
	}

	if (coex_sta->c2h_bt_inquiry_page) {
		halbtc8821c1ant_action_bt_inquiry(btcoexist);
		return;
	} else if (bt_hs_on) {
		halbtc8821c1ant_action_hs(btcoexist);
		return;
	}


	/* tdma and coex table */
	if (BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) {
		if (bt_link_info->a2dp_exist) {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						32);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		} else if (bt_link_info->a2dp_exist &&
			   bt_link_info->pan_exist) {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						22);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		} else {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 20);
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		}
	} else if ((BT_8821C_1ANT_BT_STATUS_SCO_BUSY == coex_dm->bt_status) ||
		   (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY ==
		    coex_dm->bt_status)) {
		halbtc8821c1ant_action_bt_sco_hid_only_busy(btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_SCAN);
	} else {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
		
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
		
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 2);
	}
}

void halbtc8821c1ant_action_wifi_not_connected_asso_auth(
	IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean wifi_connected = false, bt_hs_on = false;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0;
	boolean	bt_ctrl_agg_buf_size = false;
	u8	agg_buf_size = 5;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_LINK_STATUS,
			   &wifi_link_status);

	num_of_wifi_link = wifi_link_status >> 16;

	if (num_of_wifi_link >= 2) {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);
		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false,
					   bt_ctrl_agg_buf_size, agg_buf_size);

		if (coex_sta->c2h_bt_inquiry_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"############# [BTCoex],  BT Is Inquirying\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_action_bt_inquiry(btcoexist);
		} else
			halbtc8821c1ant_action_wifi_multi_port(btcoexist);
		return;
	}

	if (coex_sta->c2h_bt_inquiry_page) {
		halbtc8821c1ant_action_bt_inquiry(btcoexist);
		return;
	} else if (bt_hs_on) {
		halbtc8821c1ant_action_hs(btcoexist);
		return;
	}

	/* tdma and coex table */
	if ((bt_link_info->sco_exist)  || (bt_link_info->hid_exist) ||
	    (bt_link_info->a2dp_exist)) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 32);
		halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 4);
	} else if (bt_link_info->pan_exist) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 20);
		halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 4);
	} else {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
		halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 2);
	}
}

void halbtc8821c1ant_action_wifi_connected_specific_packet(
	IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean wifi_connected = false, bt_hs_on = false;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0;
	boolean	bt_ctrl_agg_buf_size = false;
	u8	agg_buf_size = 5;
	boolean wifi_busy = false;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_LINK_STATUS,
			   &wifi_link_status);

	num_of_wifi_link = wifi_link_status >> 16;

	if (num_of_wifi_link >= 2) {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);
		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false,
					   bt_ctrl_agg_buf_size, agg_buf_size);

		if (coex_sta->c2h_bt_inquiry_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"############# [BTCoex],  BT Is Inquirying\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_action_bt_inquiry(btcoexist);
		} else
			halbtc8821c1ant_action_wifi_multi_port(btcoexist);
		return;
	}

	if (coex_sta->c2h_bt_inquiry_page) {
		halbtc8821c1ant_action_bt_inquiry(btcoexist);
		return;
	} else if (bt_hs_on) {
		halbtc8821c1ant_action_hs(btcoexist);
		return;
	}


	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);

	/* no specific packet process for both WiFi and BT very busy */
	if ((wifi_busy) && ((bt_link_info->pan_exist) ||
			    (coex_sta->num_of_profile >= 2)))
		return;

	/* tdma and coex table */
	if ((bt_link_info->sco_exist) || (bt_link_info->hid_exist)) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 32);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 5);
	} else if (bt_link_info->a2dp_exist) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 32);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
	} else if (bt_link_info->pan_exist) {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true, 20);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 4);
	} else {
		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 8);
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 2);
	}
}

void halbtc8821c1ant_action_wifi_connected(IN struct btc_coexist *btcoexist)
{
	boolean	wifi_busy = false;
	boolean		scan = false, link = false, roam = false;
	boolean		under_4way = false, ap_enable = false, wifi_under_5g = false;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], CoexForWifiConnect()===>\n");
	BTC_TRACE(trace_buf);

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_5G, &wifi_under_5g);

	if (wifi_under_5g) {
		
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], WiFi is under 5G!!!\n");
		BTC_TRACE(trace_buf);
		
		halbtc8821c1ant_action_wifi_under5g(btcoexist);
		return;
	} else {

		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     			NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_4_WAY_PROGRESS,
			   &under_4way);
	if (under_4way) {
		halbtc8821c1ant_action_wifi_connected_specific_packet(btcoexist);
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], CoexForWifiConnect(), return for wifi is under 4way<===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_SCAN, &scan);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_LINK, &link);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_ROAM, &roam);
	if (scan || link || roam) {
		if (scan)
			halbtc8821c1ant_action_wifi_connected_scan(btcoexist);
		else
			halbtc8821c1ant_action_wifi_connected_specific_packet(
				btcoexist);
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], CoexForWifiConnect(), return for wifi is under scan<===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_AP_MODE_ENABLE,
			   &ap_enable);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_BUSY, &wifi_busy);


	/* tdma and coex table */
	if (!wifi_busy) {
		if (BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) {
			halbtc8821c1ant_action_wifi_connected_bt_acl_busy(
				btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_IDLE);
		} else if ((BT_8821C_1ANT_BT_STATUS_SCO_BUSY ==
			    coex_dm->bt_status) ||
			   (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY ==
			    coex_dm->bt_status)) {
			halbtc8821c1ant_action_bt_sco_hid_only_busy(btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_IDLE);
		} else {
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false,
						8);
			
			halbtc8821c1ant_set_ant_path(btcoexist,
				BTC_ANT_PATH_AUTO, NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
			
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 2);
		}
	} else {
		if (BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) {
			halbtc8821c1ant_action_wifi_connected_bt_acl_busy(
				btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_BUSY);
		} else if ((BT_8821C_1ANT_BT_STATUS_SCO_BUSY ==
			    coex_dm->bt_status) ||
			   (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY ==
			    coex_dm->bt_status)) {
			halbtc8821c1ant_action_bt_sco_hid_only_busy(btcoexist,
				BT_8821C_1ANT_WIFI_STATUS_CONNECTED_BUSY);
		} else {
	
			halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, true,
						32);
			
			halbtc8821c1ant_set_ant_path(btcoexist,
				BTC_ANT_PATH_AUTO, NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
	
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     NORMAL_EXEC, 4);
		}
	}
}

void halbtc8821c1ant_run_sw_coexist_mechanism(IN struct btc_coexist *btcoexist)
{
	u8				algorithm = 0;

	algorithm = halbtc8821c1ant_action_algorithm(btcoexist);
	coex_dm->cur_algorithm = algorithm;

	if (halbtc8821c1ant_is_common_action(btcoexist)) {

	} else {
		switch (coex_dm->cur_algorithm) {
		case BT_8821C_1ANT_COEX_ALGO_SCO:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = SCO.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_sco(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_HID:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = HID.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_hid(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_A2DP:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = A2DP.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_a2dp(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_A2DP_PANHS:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Action algorithm = A2DP+PAN(HS).\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_a2dp_pan_hs(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_PANEDR:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = PAN(EDR).\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_pan_edr(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_PANHS:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = HS mode.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_pan_hs(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_PANEDR_A2DP:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = PAN+A2DP.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_pan_edr_a2dp(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_PANEDR_HID:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Action algorithm = PAN(EDR)+HID.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_pan_edr_hid(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_HID_A2DP_PANEDR:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Action algorithm = HID+A2DP+PAN.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_hid_a2dp_pan_edr(btcoexist); */
			break;
		case BT_8821C_1ANT_COEX_ALGO_HID_A2DP:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				    "[BTCoex], Action algorithm = HID+A2DP.\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_action_hid_a2dp(btcoexist); */
			break;
		default:
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], Action algorithm = coexist All Off!!\n");
			BTC_TRACE(trace_buf);
			/* halbtc8821c1ant_coex_all_off(btcoexist); */
			break;
		}
		coex_dm->pre_algorithm = coex_dm->cur_algorithm;
	}
}


void halbtc8821c1ant_run_coexist_mechanism(IN struct btc_coexist *btcoexist)
{
	struct  btc_bt_link_info *bt_link_info = &btcoexist->bt_link_info;
	boolean	wifi_connected = false, bt_hs_on = false;
	boolean	increase_scan_dev_num = false;
	boolean	bt_ctrl_agg_buf_size = false;
	boolean	miracast_plus_bt = false, wifi_under_5g = false;
	u8	agg_buf_size = 5;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0, wifi_bw;
	u8	iot_peer = BTC_IOT_PEER_UNKNOWN;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], RunCoexistMechanism()===>\n");
	BTC_TRACE(trace_buf);

	if (btcoexist->manual_control) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], RunCoexistMechanism(), return for Manual CTRL <===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (btcoexist->stop_coex_dm) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], RunCoexistMechanism(), return for Stop Coex DM <===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (coex_sta->under_ips) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], wifi is under IPS !!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (!coex_sta->run_time_state){
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], return for run_time_state = false !!!\n");
		BTC_TRACE(trace_buf);
		return;
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_5G, &wifi_under_5g);

	if (wifi_under_5g) {
		
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], WiFi is under 5G!!!\n");
		BTC_TRACE(trace_buf);
		
		halbtc8821c1ant_action_wifi_under5g(btcoexist);
		return;
	} else{

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], WiFi is under 2G!!!\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     NORMAL_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
	}

	if (coex_sta->bt_whck_test) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT is under WHCK TEST!!!\n");
		BTC_TRACE(trace_buf);
		halbtc8821c1ant_action_bt_whck_test(btcoexist);
		return;
	}

	if (coex_sta->bt_disabled) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BT is disabled !!!\n");
		halbtc8821c1ant_action_wifi_only(btcoexist);
		return;
	}

	if ((BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) ||
	    (BT_8821C_1ANT_BT_STATUS_SCO_BUSY == coex_dm->bt_status) ||
	    (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY == coex_dm->bt_status))
		increase_scan_dev_num = true;

	btcoexist->btc_set(btcoexist, BTC_SET_BL_INC_SCAN_DEV_NUM,
			   &increase_scan_dev_num);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);

	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_LINK_STATUS,
			   &wifi_link_status);
	num_of_wifi_link = wifi_link_status >> 16;

	if ((num_of_wifi_link >= 2) ||
	    (wifi_link_status & WIFI_P2P_GO_CONNECTED)) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"############# [BTCoex],  Multi-Port num_of_wifi_link = %d, wifi_link_status = 0x%x\n",
			    num_of_wifi_link, wifi_link_status);
		BTC_TRACE(trace_buf);

		if (bt_link_info->bt_link_exist) {
			halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 1, 1,
						   0, 1);
			miracast_plus_bt = true;
		} else {
			halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);
			miracast_plus_bt = false;
		}
		btcoexist->btc_set(btcoexist, BTC_SET_BL_MIRACAST_PLUS_BT,
				   &miracast_plus_bt);
		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false,
					   bt_ctrl_agg_buf_size, agg_buf_size);

		if (coex_sta->c2h_bt_inquiry_page) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"############# [BTCoex],  BT Is Inquirying\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_action_bt_inquiry(btcoexist);
		} else
			halbtc8821c1ant_action_wifi_multi_port(btcoexist);


		return;
	} else {
		miracast_plus_bt = false;
		btcoexist->btc_set(btcoexist, BTC_SET_BL_MIRACAST_PLUS_BT,
				   &miracast_plus_bt);
	}

	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_BW, &wifi_bw);

	if ((bt_link_info->bt_link_exist) && (wifi_connected)) {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 1, 1, 0, 1);

		btcoexist->btc_get(btcoexist, BTC_GET_U1_IOT_PEER, &iot_peer);

		if (BTC_IOT_PEER_CISCO != iot_peer) {
			if (bt_link_info->sco_exist) /* if (bt_link_info->bt_hi_pri_link_exist) */
				halbtc8821c1ant_limited_rx(btcoexist,
					   NORMAL_EXEC, true, false, 0x5);
			else
				halbtc8821c1ant_limited_rx(btcoexist,
					   NORMAL_EXEC, false, false, 0x5);
		} else {
			if (bt_link_info->sco_exist)
				halbtc8821c1ant_limited_rx(btcoexist,
					   NORMAL_EXEC, true, false, 0x5);
			else {
				if (BTC_WIFI_BW_HT40 == wifi_bw)
					halbtc8821c1ant_limited_rx(btcoexist,
						NORMAL_EXEC, false, true, 0x10);
				else
					halbtc8821c1ant_limited_rx(btcoexist,
						NORMAL_EXEC, false, true, 0x8);
			}
		}

		halbtc8821c1ant_sw_mechanism(btcoexist, true);
		halbtc8821c1ant_run_sw_coexist_mechanism(
			btcoexist);  /* just print debug message */
	} else {
		halbtc8821c1ant_limited_tx(btcoexist, NORMAL_EXEC, 0, 0, 0, 0);

		halbtc8821c1ant_limited_rx(btcoexist, NORMAL_EXEC, false, false,
					   0x5);

		halbtc8821c1ant_sw_mechanism(btcoexist, false);
		halbtc8821c1ant_run_sw_coexist_mechanism(
			btcoexist); /* just print debug message */
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_HS_OPERATION, &bt_hs_on);
	if (coex_sta->c2h_bt_inquiry_page) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "############# [BTCoex],  BT Is Inquirying\n");
		BTC_TRACE(trace_buf);
		halbtc8821c1ant_action_bt_inquiry(btcoexist);
		return;
	} else if (bt_hs_on) {
		halbtc8821c1ant_action_hs(btcoexist);
		return;
	}


	if (!wifi_connected) {
		boolean	scan = false, link = false, roam = false;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], wifi is non connected-idle !!!\n");
		BTC_TRACE(trace_buf);

		btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_SCAN, &scan);
		btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_LINK, &link);
		btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_ROAM, &roam);

		if (scan || link || roam) {
			if (scan)
				halbtc8821c1ant_action_wifi_not_connected_scan(
					btcoexist);
			else
				halbtc8821c1ant_action_wifi_not_connected_asso_auth(
					btcoexist);
		} else
			halbtc8821c1ant_action_wifi_not_connected(btcoexist);
	} else	/* wifi LPS/Busy */
		halbtc8821c1ant_action_wifi_connected(btcoexist);
}

void halbtc8821c1ant_init_coex_dm(IN struct btc_coexist *btcoexist)
{
	/* force to reset coex mechanism */

	/* sw all off */
	halbtc8821c1ant_sw_mechanism(btcoexist, false);

	coex_sta->pop_event_cnt = 0;
}

void halbtc8821c1ant_init_hw_config(IN struct btc_coexist *btcoexist,
				    IN boolean back_up, IN boolean wifi_only)
{
	u32			u32tmp1 = 0, u32tmp2 = 0, u32tmp3 = 0;
	u16			u16tmp1 = 0;
	struct  btc_board_info *board_info = &btcoexist->board_info;


	u32tmp3 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
	u32tmp1 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38);
	u32tmp2 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x54);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (Before Init HW config) 0xcb4 = 0x%x, 0x38= 0x%x, 0x54= 0x%x**********\n",
		    u32tmp3, u32tmp1, u32tmp2);
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], 1Ant Init HW Config!!\n");
	BTC_TRACE(trace_buf);

	coex_sta->bt_coex_supported_feature = 0;
	coex_sta->bt_coex_supported_version = 0;
	
	/* Setup RF front end type */
	halbtc8821c1ant_set_rfe_type(btcoexist);

	/* 0xf0[15:12] --> Chip Cut information */
	coex_sta->cut_version = (btcoexist->btc_read_1byte(btcoexist,
				 0xf1) & 0xf0) >> 4;

	btcoexist->btc_write_1byte_bitmask(btcoexist, 0x550, 0x8,
					   0x1);  /* enable TBTT nterrupt */

	/* BT report packet sample rate	 */
	btcoexist->btc_write_1byte(btcoexist, 0x790, 0x5);

	/* Init 0x778 = 0x1 for 1-Ant */
	btcoexist->btc_write_1byte(btcoexist, 0x778, 0x1);

	/* Enable PTA (3-wire function form BT side) */
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0x40, 0x20, 0x1);
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0x41, 0x02, 0x1);

	/* Enable PTA (tx/rx signal form WiFi side) */
	btcoexist->btc_write_1byte_bitmask(btcoexist, 0x4c6, 0x10, 0x1);

	halbtc8821c1ant_enable_gnt_to_gpio(btcoexist, TRUE);

	/* check if WL firmware download ok */
	if (btcoexist->btc_read_1byte(btcoexist, 0x80) == 0xc6)
		halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, TRUE);

	halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 8);

	/* Antenna config */
	if (wifi_only) {
		coex_sta->concurrent_rx_mode_on = false;
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_WIFI,
					     FORCE_EXEC, BT_8821C_1ANT_PHASE_WLANONLY_INIT);
	} else {
		coex_sta->concurrent_rx_mode_on = true;
		/* btcoexist->btc_write_1byte_bitmask(btcoexist, 0x953, 0x2, 0x1); */
		
		/* RF 0x1[1] = 0->Set GNT_WL_RF_Rx always = 1 for con-current Rx, mask Tx only */
		btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0x1, 0x2, 0x0);
		
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     FORCE_EXEC, BT_8821C_1ANT_PHASE_COEX_INIT);
	}

	/* PTA parameter */
	halbtc8821c1ant_coex_table_with_type(btcoexist, FORCE_EXEC, 0);

	psd_scan->ant_det_is_ant_det_available = TRUE;

	u32tmp3 = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
	u32tmp1 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38);
	u32tmp2 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x54);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], ********** (After Init HW config) 0xcb4 = 0x%x, 0x38= 0x%x, 0x54= 0x%x**********\n",
		    u32tmp3, u32tmp1, u32tmp2);
	BTC_TRACE(trace_buf);

}

u32 halbtc8821c1ant_psd_log2base(IN struct btc_coexist *btcoexist, IN u32 val)
{
	u8	j;
	u32	tmp, tmp2, val_integerd_b = 0, tindex, shiftcount = 0;
	u32	result, val_fractiond_b = 0, table_fraction[21] = {0, 432, 332, 274, 232, 200,
				   174, 151, 132, 115, 100, 86, 74, 62, 51, 42,
							   32, 23, 15, 7, 0
							      };

	if (val == 0)
		return 0;

	tmp = val;

	while (1) {
		if (tmp == 1)
			break;
		else {
			tmp = (tmp >> 1);
			shiftcount++;
		}
	}


	val_integerd_b = shiftcount + 1;

	tmp2 = 1;
	for (j = 1; j <= val_integerd_b; j++)
		tmp2 = tmp2 * 2;

	tmp = (val * 100) / tmp2;
	tindex = tmp / 5;

	if (tindex > 20)
		tindex = 20;

	val_fractiond_b = table_fraction[tindex];

	result = val_integerd_b * 100 - val_fractiond_b;

	return result;


}

void halbtc8821c1ant_psd_show_antenna_detect_result(IN struct btc_coexist
		*btcoexist)
{
	u8		*cli_buf = btcoexist->cli_buf;
	struct  btc_board_info	*board_info = &btcoexist->board_info;

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n============[Antenna Detection info]  ============\n");
	CL_PRINTF(cli_buf);

	if (psd_scan->ant_det_result == 1)
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s (>%d)",
			   "Ant Det Result", "2-Antenna (Bad-Isolation)",
			   BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_BADISOLATION);
	else if (psd_scan->ant_det_result == 2)
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s (%d~%d)",
			   "Ant Det Result", "2-Antenna (Good-Isolation)",
			   BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_GOODISOLATION
			   + psd_scan->ant_det_thres_offset,
			   BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_BADISOLATION);
	else
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s (%d~%d)",
			   "Ant Det Result", "1-Antenna",
			   BT_8821C_1ANT_ANTDET_PSDTHRES_1ANT,
			   BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_GOODISOLATION
			   + psd_scan->ant_det_thres_offset);

	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s ",
		   "Antenna Detection Finish",
		   (board_info->btdm_ant_det_finish
		    ? "Yes" : "No"));
	CL_PRINTF(cli_buf);

	switch (psd_scan->ant_det_result) {
	case 0:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is not available)");
		break;
	case 1:  /* 2-Ant bad-isolation */
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is available)");
		break;
	case 2:  /* 2-Ant good-isolation */
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is available)");
		break;
	case 3:  /* 1-Ant */
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is available)");
		break;
	case 4:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(Uncertainty result)");
		break;
	case 5:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "(Pre-Scan fai)");
		break;
	case 6:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(WiFi is Scanning)");
		break;
	case 7:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is not idle)");
		break;
	case 8:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(Abort by WiFi Scanning)");
		break;
	case 9:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(Antenna Init is not ready)");
		break;
	case 10:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is Inquiry or page)");
		break;
	case 11:
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "(BT is Disabled)");
		break;
	}
	CL_PRINTF(cli_buf);


	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
		   "Ant Detect Total Count", psd_scan->ant_det_try_count);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
		   "Ant Detect Fail Count", psd_scan->ant_det_fail_count);
	CL_PRINTF(cli_buf);

	if ((!board_info->btdm_ant_det_finish) &&
	    (psd_scan->ant_det_result != 5))
		return;

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s", "BT Response",
		   (psd_scan->ant_det_result ? "ok" : "fail"));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d ms", "BT Tx Time",
		   psd_scan->ant_det_bt_tx_time);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d", "BT Tx Ch",
		   psd_scan->ant_det_bt_le_channel);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d",
		   "WiFi PSD Cent-Ch/Offset/Span",
		   psd_scan->real_cent_freq, psd_scan->real_offset,
		   psd_scan->real_span);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d dB",
		   "PSD Pre-Scan Peak Value",
		   psd_scan->ant_det_pre_psdscan_peak_val / 100);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s (<= %d)",
		   "PSD Pre-Scan result",
		   (psd_scan->ant_det_result != 5 ? "ok" : "fail"),
		   BT_8821C_1ANT_ANTDET_PSDTHRES_BACKGROUND
		   + psd_scan->ant_det_thres_offset);
	CL_PRINTF(cli_buf);

	if (psd_scan->ant_det_result == 5)
		return;

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s dB",
		   "PSD Scan Peak Value", psd_scan->ant_det_peak_val);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s MHz",
		   "PSD Scan Peak Freq", psd_scan->ant_det_peak_freq);
	CL_PRINTF(cli_buf);


	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s", "TFBGA Package",
		   (board_info->tfbga_package) ?  "Yes" : "No");
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
		   "PSD Threshold Offset", psd_scan->ant_det_thres_offset);
	CL_PRINTF(cli_buf);

}



void halbtc8821c1ant_psd_showdata(IN struct btc_coexist *btcoexist)
{
	u8		*cli_buf = btcoexist->cli_buf;
	u32		delta_freq_per_point;
	u32		freq, freq1, freq2, n = 0, i = 0, j = 0, m = 0, psd_rep1, psd_rep2;

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n\n============[PSD info]  (%d)============\n",
		   psd_scan->psd_gen_count);
	CL_PRINTF(cli_buf);

	if (psd_scan->psd_gen_count == 0) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n No data !!\n");
		CL_PRINTF(cli_buf);
		return;
	}

	if (psd_scan->psd_point == 0)
		delta_freq_per_point = 0;
	else
		delta_freq_per_point = psd_scan->psd_band_width /
				       psd_scan->psd_point;

	/* if (psd_scan->is_psd_show_max_only) */
	if (0) {
		psd_rep1 = psd_scan->psd_max_value / 100;
		psd_rep2 = psd_scan->psd_max_value - psd_rep1 * 100;

		freq = ((psd_scan->real_cent_freq - 20) * 1000000 +
			psd_scan->psd_max_value_point * delta_freq_per_point);
		freq1 = freq / 1000000;
		freq2 = freq / 1000 - freq1 * 1000;

		if (freq2 < 100)
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				   "\r\n Freq = %d.0%d MHz",
				   freq1, freq2);
		else
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				   "\r\n Freq = %d.%d MHz",
				   freq1, freq2);

		if (psd_rep2 < 10)
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				   ", Value = %d.0%d dB, (%d)\n",
				   psd_rep1, psd_rep2, psd_scan->psd_max_value);
		else
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				   ", Value = %d.%d dB, (%d)\n",
				   psd_rep1, psd_rep2, psd_scan->psd_max_value);

		CL_PRINTF(cli_buf);
	} else {
		m = psd_scan->psd_start_point;
		n = psd_scan->psd_start_point;
		i = 1;
		j = 1;

		while (1) {
			do {
				freq = ((psd_scan->real_cent_freq - 20) * 1000000 + m *
					delta_freq_per_point);
				freq1 = freq / 1000000;
				freq2 = freq / 1000 - freq1 * 1000;

				if (i == 1) {
					if (freq2 == 0)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"\r\n Freq%6d.000", freq1);
					else if (freq2 < 100)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"\r\n Freq%6d.0%2d", freq1,
							   freq2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"\r\n Freq%6d.%3d", freq1,
							   freq2);
				} else if ((i % 8 == 0) ||
					   (m == psd_scan->psd_stop_point)) {
					if (freq2 == 0)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							   "%6d.000\n", freq1);
					else if (freq2 < 100)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%6d.0%2d\n", freq1, freq2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%6d.%3d\n", freq1, freq2);
				} else {
					if (freq2 == 0)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							   "%6d.000", freq1);
					else if (freq2 < 100)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%6d.0%2d", freq1, freq2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%6d.%3d", freq1, freq2);
				}

				i++;
				m++;
				CL_PRINTF(cli_buf);

			} while ((i <= 8) && (m <= psd_scan->psd_stop_point));


			do {
				psd_rep1 = psd_scan->psd_report_max_hold[n] / 100;
				psd_rep2 = psd_scan->psd_report_max_hold[n] - psd_rep1 *
					   100;

				if (j == 1) {
					if (psd_rep2 < 10)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"\r\n Val %7d.0%d", psd_rep1,
							   psd_rep2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"\r\n Val %7d.%d", psd_rep1,
							   psd_rep2);
				} else if ((j % 8 == 0)  ||
					   (n == psd_scan->psd_stop_point)) {
					if (psd_rep2 < 10)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%7d.0%d\n", psd_rep1,
							   psd_rep2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%7d.%d\n", psd_rep1, psd_rep2);
				} else {
					if (psd_rep2 < 10)
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%7d.0%d", psd_rep1, psd_rep2);
					else
						CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
							"%7d.%d", psd_rep1, psd_rep2);
				}

				j++;
				n++;
				CL_PRINTF(cli_buf);

			} while ((j <= 8) && (n <= psd_scan->psd_stop_point));

			if ((m > psd_scan->psd_stop_point) ||
			    (n > psd_scan->psd_stop_point))
				break;
			else {
				i = 1;
				j = 1;
			}

		}
	}


}

void halbtc8821c1ant_psd_maxholddata(IN struct btc_coexist *btcoexist,
				     IN u32 gen_count)
{
	u32	i = 0, i_max = 0, val_max = 0;

	if (gen_count == 1) {
		memcpy(psd_scan->psd_report_max_hold,
		       psd_scan->psd_report,
		       BT_8821C_1ANT_ANTDET_PSD_POINTS * sizeof(u32));

		for (i = psd_scan->psd_start_point;
		     i <= psd_scan->psd_stop_point; i++) {

		}

		psd_scan->psd_max_value_point = 0;
		psd_scan->psd_max_value = 0;

	} else {
		for (i = psd_scan->psd_start_point;
		     i <= psd_scan->psd_stop_point; i++) {
			if (psd_scan->psd_report[i] >
			    psd_scan->psd_report_max_hold[i])
				psd_scan->psd_report_max_hold[i] =
					psd_scan->psd_report[i];

			/* search Max Value */
			if (i == psd_scan->psd_start_point) {
				i_max = i;
				val_max = psd_scan->psd_report_max_hold[i];
			} else {
				if (psd_scan->psd_report_max_hold[i] >
				    val_max) {
					i_max = i;
					val_max = psd_scan->psd_report_max_hold[i];
				}
			}


		}

		psd_scan->psd_max_value_point = i_max;
		psd_scan->psd_max_value = val_max;

	}


}

u32 halbtc8821c1ant_psd_getdata(IN struct btc_coexist *btcoexist, IN u32 point)
{
	/* reg 0x808[9:0]: FFT data x */
	/* reg 0x808[22]: 0-->1 to get 1 FFT data y */
	/* reg 0x8b4[15:0]: FFT data y report */

	u32 val = 0, psd_report = 0;

	val = btcoexist->btc_read_4byte(btcoexist, 0x808);

	val &= 0xffbffc00;
	val |= point;

	btcoexist->btc_write_4byte(btcoexist, 0x808, val);

	val |= 0x00400000;
	btcoexist->btc_write_4byte(btcoexist, 0x808, val);


	val = btcoexist->btc_read_4byte(btcoexist, 0x8b4);

	psd_report = val & 0x0000ffff;

	return psd_report;
}


boolean halbtc8821c1ant_psd_sweep_point(IN struct btc_coexist *btcoexist,
		IN u32 cent_freq, IN s32 offset, IN u32 span, IN u32 points,
					IN u32 avgnum, IN u32 loopcnt)
{
	u32	 i = 0, val = 0, n = 0, k = 0, j, point_index = 0;
	u32	points1 = 0, psd_report = 0;
	u32	start_p = 0, stop_p = 0, delta_freq_per_point = 156250;
	u32    psd_center_freq = 20 * 10 ^ 6;
	boolean outloop = false, scan , roam, is_sweep_ok = true;
	u8	 flag = 0;
	u32	tmp = 0, u32tmp1 = 0;
	u32	wifi_original_channel = 1;

	psd_scan->is_psd_running = true;
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "xxxxxxxxxxxxxxxx PSD Sweep Start!!\n");
	BTC_TRACE(trace_buf);

	do {
		switch (flag) {
		case 0:  /* Get PSD parameters */
		default:

			psd_scan->psd_band_width = 40 * 1000000;
			psd_scan->psd_point = points;
			psd_scan->psd_start_base = points / 2;
			psd_scan->psd_avg_num = avgnum;
			psd_scan->real_cent_freq = cent_freq;
			psd_scan->real_offset = offset;
			psd_scan->real_span = span;


			points1 = psd_scan->psd_point;
			delta_freq_per_point = psd_scan->psd_band_width /
					       psd_scan->psd_point;

			/* PSD point setup */
			val = btcoexist->btc_read_4byte(btcoexist, 0x808);
			val &= 0xffff0fff;

			switch (psd_scan->psd_point) {
			case 128:
				val |= 0x0;
				break;
			case 256:
			default:
				val |= 0x00004000;
				break;
			case 512:
				val |= 0x00008000;
				break;
			case 1024:
				val |= 0x0000c000;
				break;
			}

			switch (psd_scan->psd_avg_num) {
			case 1:
				val |= 0x0;
				break;
			case 8:
				val |= 0x00001000;
				break;
			case 16:
				val |= 0x00002000;
				break;
			case 32:
			default:
				val |= 0x00003000;
				break;
			}
			btcoexist->btc_write_4byte(btcoexist, 0x808, val);

			flag = 1;
			break;
		case 1:	  /* calculate the PSD point index from freq/offset/span */
			psd_center_freq = psd_scan->psd_band_width / 2 +
					  offset * (1000000);

			start_p = psd_scan->psd_start_base + (psd_center_freq -
				span * (1000000) / 2) / delta_freq_per_point;
			psd_scan->psd_start_point = start_p -
						    psd_scan->psd_start_base;

			stop_p = psd_scan->psd_start_base + (psd_center_freq +
				span * (1000000) / 2) / delta_freq_per_point;
			psd_scan->psd_stop_point = stop_p -
						   psd_scan->psd_start_base - 1;

			flag = 2;
			break;
		case 2:  /* set RF channel/BW/Mode */

			/* set 3-wire off */
			val = btcoexist->btc_read_4byte(btcoexist, 0x88c);
			val |= 0x00300000;
			btcoexist->btc_write_4byte(btcoexist, 0x88c, val);

			/* CCK off */
			val = btcoexist->btc_read_4byte(btcoexist, 0x800);
			val &= 0xfeffffff;
			btcoexist->btc_write_4byte(btcoexist, 0x800, val);

			/* Tx-pause on */
			btcoexist->btc_write_1byte(btcoexist, 0x522, 0x6f);

			/* store WiFi original channel */
			wifi_original_channel = btcoexist->btc_get_rf_reg(
					btcoexist, BTC_RF_A, 0x18, 0x3ff);

			/* Set RF channel */
			if (cent_freq == 2484)
				btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A,
							  0x18, 0x3ff, 0xe);
			else
				btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A,
					  0x18, 0x3ff, (cent_freq - 2412) / 5 +
						  1); /* WiFi TRx Mask on */

			/* save original RCK value */
			u32tmp1 =  btcoexist->btc_get_rf_reg(
					   btcoexist, BTC_RF_A, 0x1d, 0xfffff);

			/* Enter debug mode */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0xde,
						  0x2, 0x1);

			/* Set RF Rx filter corner */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0x1d,
						  0xfffff, 0x2e);


			/* Set  RF mode = Rx, RF Gain = 0x320a0 */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0x0,
						  0xfffff, 0x320a0);

			while (1) {
				if (k++ > BT_8821C_1ANT_ANTDET_SWEEPPOINT_DELAY)
					break;
			}
			flag = 3;
			break;
		case 3:
			psd_scan->psd_gen_count = 0;
			for (j = 1; j <= loopcnt; j++) {

				btcoexist->btc_get(btcoexist,
						   BTC_GET_BL_WIFI_SCAN, &scan);
				btcoexist->btc_get(btcoexist,
						   BTC_GET_BL_WIFI_ROAM, &roam);

				if (scan || roam) {
					is_sweep_ok = false;
					break;
				}
				memset(psd_scan->psd_report, 0,
				       psd_scan->psd_point * sizeof(u32));
				start_p = psd_scan->psd_start_point +
					  psd_scan->psd_start_base;
				stop_p = psd_scan->psd_stop_point +
					 psd_scan->psd_start_base + 1;

				i = start_p;
				point_index = 0;

				while (i < stop_p) {
					if (i >= points1)
						psd_report =
							halbtc8821c1ant_psd_getdata(
							btcoexist, i - points1);
					else
						psd_report =
							halbtc8821c1ant_psd_getdata(
								btcoexist, i);

					BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						"Point=%d, psd_raw_data = 0x%08x\n",
						    i, psd_report);
					BTC_TRACE(trace_buf);
					if (psd_report == 0)
						tmp = 0;
					else
						/* tmp =  20*log10((double)psd_report); */
						/* 20*log2(x)/log2(10), log2Base return theresult of the psd_report*100 */
						tmp = 6 * halbtc8821c1ant_psd_log2base(
							btcoexist, psd_report);

					n = i - psd_scan->psd_start_base;
					psd_scan->psd_report[n] =  tmp;


					halbtc8821c1ant_psd_maxholddata(
						btcoexist, j);

					i++;

				}

				psd_scan->psd_gen_count = j;
			}

			flag = 100;
			break;
		case 99:	/* error */

			outloop = true;
			break;
		case 100: /* recovery */

			/* set 3-wire on */
			val = btcoexist->btc_read_4byte(btcoexist, 0x88c);
			val &= 0xffcfffff;
			btcoexist->btc_write_4byte(btcoexist, 0x88c, val);

			/* CCK on */
			val = btcoexist->btc_read_4byte(btcoexist, 0x800);
			val |= 0x01000000;
			btcoexist->btc_write_4byte(btcoexist, 0x800, val);

			/* Tx-pause off */
			btcoexist->btc_write_1byte(btcoexist, 0x522, 0x0);

			/* PSD off */
			val = btcoexist->btc_read_4byte(btcoexist, 0x808);
			val &= 0xffbfffff;
			btcoexist->btc_write_4byte(btcoexist, 0x808, val);

			/* restore RF Rx filter corner */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0x1d,
						  0xfffff, u32tmp1);

			/* Exit debug mode */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0xde,
						  0x2, 0x0);

			/* restore WiFi original channel */
			btcoexist->btc_set_rf_reg(btcoexist, BTC_RF_A, 0x18,
						  0x3ff, wifi_original_channel);

			outloop = true;
			break;

		}

	} while (!outloop);



	psd_scan->is_psd_running = false;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "xxxxxxxxxxxxxxxx PSD Sweep Stop!!\n");
	BTC_TRACE(trace_buf);
	return is_sweep_ok;

}


void halbtc8821c1ant_psd_antenna_detection(IN struct btc_coexist *btcoexist,
		IN u32 bt_tx_time, IN u32 bt_le_channel)
{
	u32	i = 0;
	u32	wlpsd_cent_freq = 2484, wlpsd_span = 2, wlpsd_sweep_count = 50;
	s32	wlpsd_offset = -4;
	u8	bt_le_ch[13] = {3, 6, 8, 11, 13, 16, 18, 21, 23, 26, 28, 31, 33};

	u8	h2c_parameter[3] = {0}, u8tmpa, u8tmpb;

	u8	state = 0;
	boolean		outloop = false, bt_resp = false;
	u32		freq, freq1, freq2, psd_rep1, psd_rep2, delta_freq_per_point,
			u32tmp, u32tmp0, u32tmp1, u32tmp2 ;
	struct  btc_board_info	*board_info = &btcoexist->board_info;

	board_info->btdm_ant_det_finish = false;
	memset(psd_scan->ant_det_peak_val, 0, 16 * sizeof(u8));
	memset(psd_scan->ant_det_peak_freq, 0, 16 * sizeof(u8));

	if (board_info->tfbga_package) /* for TFBGA */
		psd_scan->ant_det_thres_offset = 5;
	else
		psd_scan->ant_det_thres_offset = 0;

	do {
		switch (state) {
		case 0:
			if (bt_le_channel == 39)
				wlpsd_cent_freq = 2484;
			else {
				for (i = 1; i <= 13; i++) {
					if (bt_le_ch[i - 1] ==
					    bt_le_channel) {
						wlpsd_cent_freq = 2412
								  + (i - 1) * 5;
						break;
					}
				}

				if (i == 14) {

					BTC_SPRINTF(trace_buf,
						    BT_TMP_BUF_SIZE,
						"xxxxxxxxxxxxxxxx AntennaDetect(), Abort!!, Invalid LE channel = %d\n ",
						    bt_le_channel);
					BTC_TRACE(trace_buf);
					outloop = true;
					break;
				}
			}

			wlpsd_sweep_count = bt_tx_time * 238 /
					    100; /* bt_tx_time/0.42								 */
			wlpsd_sweep_count = wlpsd_sweep_count / 5;

			if (wlpsd_sweep_count % 5 != 0)
				wlpsd_sweep_count = (wlpsd_sweep_count /
						     5 + 1) * 5;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), BT_LETxTime=%d,  BT_LECh = %d\n",
				    bt_tx_time, bt_le_channel);
			BTC_TRACE(trace_buf);
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), wlpsd_cent_freq=%d,  wlpsd_offset = %d, wlpsd_span = %d, wlpsd_sweep_count = %d\n",
				    wlpsd_cent_freq,
				    wlpsd_offset,
				    wlpsd_span,
				    wlpsd_sweep_count);
			BTC_TRACE(trace_buf);

			state = 1;
			break;
		case 1: /* stop coex DM & set antenna path */
			/* Stop Coex DM */
			btcoexist->stop_coex_dm = true;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Stop Coex DM!!\n");
			BTC_TRACE(trace_buf);

			/* Set TDMA off,				 */
			halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC,
						false, 0);

			/* Set coex table */
			halbtc8821c1ant_coex_table_with_type(btcoexist,
							     FORCE_EXEC, 0);

			if (board_info->btdm_ant_pos ==
			    BTC_ANTENNA_AT_MAIN_PORT) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna at Main Port\n");
				BTC_TRACE(trace_buf);
			} else {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna at Aux Port\n");
				BTC_TRACE(trace_buf);
			}

			halbtc8821c1ant_set_int_block(btcoexist, FORCE_EXEC, BT_8821C_1ANT_INT_BLOCK_SWITCH_TO_WLG_OF_WLAG);

			halbtc8821c1ant_set_ext_ant_switch( btcoexist, 
													FORCE_EXEC, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_CTRL_BY_BBSW, 
													BT_8821C_1ANT_EXT_ANT_SWITCH_TO_BT);

			/* Set Antenna Path,  both GNT_WL/GNT_BT = 1, and control by SW */
			/* set GNT_BT to SW high */
			halbtc8821c1ant_ltecoex_set_gnt_bt(btcoexist,
					   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);
			/* Set GNT_WL to SW high */
			halbtc8821c1ant_ltecoex_set_gnt_wl(btcoexist,
					   BT_8821C_1ANT_GNT_BLOCK_RFC_BB,
					   BT_8821C_1ANT_GNT_TYPE_CTRL_BY_SW,
					   BT_8821C_1ANT_SIG_STA_SET_TO_HIGH);

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Set Antenna to BT!!\n");
			BTC_TRACE(trace_buf);

			/* Set AFH mask on at WiFi channel 2472MHz +/- 10MHz */
			h2c_parameter[0] = 0x1;
			h2c_parameter[1] = 0xd;
			h2c_parameter[2] = 0x14;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Set AFH on, Cent-Ch= %d,  Mask=%d\n",
				    h2c_parameter[1],
				    h2c_parameter[2]);
			BTC_TRACE(trace_buf);

			btcoexist->btc_fill_h2c(btcoexist, 0x66, 3,
						h2c_parameter);

			u32tmp0 = btcoexist->btc_read_4byte(btcoexist, 0x70);
			u32tmp1 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38);
			u32tmp2 = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x54);

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], ********** 0x70 = 0x%x, 0x38= 0x%x, 0x54= 0x%x (Before Ant Det) **********\n",
				    u32tmp0, u32tmp1, u32tmp2);
			BTC_TRACE(trace_buf);

			state = 2;
			break;
		case 2:	/* Pre-sweep background psd */
			if (!halbtc8821c1ant_psd_sweep_point(btcoexist,
				     wlpsd_cent_freq, wlpsd_offset, wlpsd_span,
					     BT_8821C_1ANT_ANTDET_PSD_POINTS,
				     BT_8821C_1ANT_ANTDET_PSD_AVGNUM, 3)) {
				board_info->btdm_ant_det_finish = false;
				board_info->btdm_ant_num_by_ant_det = 1;
				psd_scan->ant_det_result = 8;
				state = 99;
				break;
			}

			psd_scan->ant_det_pre_psdscan_peak_val =
				psd_scan->psd_max_value;

			if (psd_scan->psd_max_value >
			    (BT_8821C_1ANT_ANTDET_PSDTHRES_BACKGROUND
			     + psd_scan->ant_det_thres_offset) * 100) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Abort Antenna Detection!! becaus background = %d > thres (%d)\n",
					    psd_scan->psd_max_value / 100,
					BT_8821C_1ANT_ANTDET_PSDTHRES_BACKGROUND
					    + psd_scan->ant_det_thres_offset);
				BTC_TRACE(trace_buf);
				board_info->btdm_ant_det_finish = false;
				board_info->btdm_ant_num_by_ant_det = 1;
				psd_scan->ant_det_result = 5;
				state = 99;
			} else {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Start Antenna Detection!! becaus background = %d <= thres (%d)\n",
					    psd_scan->psd_max_value / 100,
					BT_8821C_1ANT_ANTDET_PSDTHRES_BACKGROUND
					    + psd_scan->ant_det_thres_offset);
				BTC_TRACE(trace_buf);
				state = 3;
			}
			break;
		case 3:

			bt_resp = btcoexist->btc_set_bt_ant_detection(
					  btcoexist, (u8)(bt_tx_time & 0xff),
					  (u8)(bt_le_channel & 0xff));

			if (!halbtc8821c1ant_psd_sweep_point(btcoexist,
					     wlpsd_cent_freq, wlpsd_offset,
							     wlpsd_span,
					     BT_8821C_1ANT_ANTDET_PSD_POINTS,
					     BT_8821C_1ANT_ANTDET_PSD_AVGNUM,
						     wlpsd_sweep_count)) {
				board_info->btdm_ant_det_finish
					= false;
				board_info->btdm_ant_num_by_ant_det
					= 1;
				psd_scan->ant_det_result = 8;
				state = 99;
				break;
			}

			psd_scan->ant_det_psd_scan_peak_val =
				psd_scan->psd_max_value;
			psd_scan->ant_det_psd_scan_peak_freq =
				psd_scan->psd_max_value_point;
			state = 4;
			break;
		case 4:

			if (psd_scan->psd_point == 0)
				delta_freq_per_point = 0;
			else
				delta_freq_per_point =
					psd_scan->psd_band_width /
					psd_scan->psd_point;

			psd_rep1 = psd_scan->psd_max_value / 100;
			psd_rep2 = psd_scan->psd_max_value - psd_rep1 *
				   100;

			freq = ((psd_scan->real_cent_freq - 20) *
				1000000 + psd_scan->psd_max_value_point
				* delta_freq_per_point);
			freq1 = freq / 1000000;
			freq2 = freq / 1000 - freq1 * 1000;

			if (freq2 < 100) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Max Value: Freq = %d.0%d MHz",
					    freq1, freq2);
				BTC_TRACE(trace_buf);
				CL_SPRINTF(psd_scan->ant_det_peak_freq,
					   BT_8821C_1ANT_ANTDET_BUF_LEN,
					   "%d.0%d", freq1, freq2);
			} else {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Max Value: Freq = %d.%d MHz",
					    freq1, freq2);
				BTC_TRACE(trace_buf);
				CL_SPRINTF(psd_scan->ant_det_peak_freq,
					   BT_8821C_1ANT_ANTDET_BUF_LEN,
					   "%d.%d", freq1, freq2);
			}

			if (psd_rep2 < 10) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					    ", Value = %d.0%d dB\n",
					    psd_rep1, psd_rep2);
				BTC_TRACE(trace_buf);
				CL_SPRINTF(psd_scan->ant_det_peak_val,
					   BT_8821C_1ANT_ANTDET_BUF_LEN,
					   "%d.0%d", psd_rep1, psd_rep2);
			} else {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					    ", Value = %d.%d dB\n",
					    psd_rep1, psd_rep2);
				BTC_TRACE(trace_buf);
				CL_SPRINTF(psd_scan->ant_det_peak_val,
					   BT_8821C_1ANT_ANTDET_BUF_LEN,
					   "%d.%d", psd_rep1, psd_rep2);
			}

			psd_scan->ant_det_is_btreply_available = true;

			if (bt_resp == false) {
				psd_scan->ant_det_is_btreply_available =
					false;
				psd_scan->ant_det_result = 0;
				board_info->btdm_ant_det_finish = false;
				board_info->btdm_ant_num_by_ant_det = 1;
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), BT Response = Fail\n ");
				BTC_TRACE(trace_buf);
			} else if (psd_scan->psd_max_value >
				(BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_BADISOLATION)
				   * 100) {
				psd_scan->ant_det_result = 1;
				board_info->btdm_ant_det_finish = true;
				board_info->btdm_ant_num_by_ant_det = 2;
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Detect Result = 2-Ant, Bad-Isolation!!\n");
				BTC_TRACE(trace_buf);
			} else if (psd_scan->psd_max_value >
				(BT_8821C_1ANT_ANTDET_PSDTHRES_2ANT_GOODISOLATION
				    + psd_scan->ant_det_thres_offset) * 100) {
				psd_scan->ant_det_result = 2;
				board_info->btdm_ant_det_finish = true;
				board_info->btdm_ant_num_by_ant_det = 2;
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Detect Result = 2-Ant, Good-Isolation!!\n");
				BTC_TRACE(trace_buf);
			} else if (psd_scan->psd_max_value >
				   (BT_8821C_1ANT_ANTDET_PSDTHRES_1ANT) *
				   100) {
				psd_scan->ant_det_result = 3;
				board_info->btdm_ant_det_finish = true;
				board_info->btdm_ant_num_by_ant_det = 1;
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Detect Result = 1-Ant!!\n");
				BTC_TRACE(trace_buf);
			} else {
				psd_scan->ant_det_result = 4;
				board_info->btdm_ant_det_finish = false;
				board_info->btdm_ant_num_by_ant_det = 1;
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Detect Result = 1-Ant, un-certainity!!\n");
				BTC_TRACE(trace_buf);
			}

			state = 99;
			break;
		case 99:  /* restore setup */

			/* Set AFH mask off at WiFi channel 2472MHz +/- 10MHz */
			h2c_parameter[0] = 0x0;
			h2c_parameter[1] = 0x0;
			h2c_parameter[2] = 0x0;

			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Set AFH on, Cent-Ch= %d,  Mask=%d\n",
				    h2c_parameter[1], h2c_parameter[2]);
			BTC_TRACE(trace_buf);

			btcoexist->btc_fill_h2c(btcoexist, 0x66, 3,
						h2c_parameter);

			/* Set Antenna Path, GNT_WL/GNT_BT control by PTA */
			halbtc8821c1ant_set_ant_path(btcoexist,
				     BTC_ANT_PATH_AUTO, 
				     FORCE_EXEC, 
				     BT_8821C_1ANT_PHASE_2G_RUNTIME);
			
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Set Antenna to PTA\n!!");
			BTC_TRACE(trace_buf);

			/* Resume Coex DM */
			btcoexist->stop_coex_dm = false;
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Resume Coex DM\n!!");
			BTC_TRACE(trace_buf);

			/* stimulate coex running */
			halbtc8821c1ant_run_coexist_mechanism(
				btcoexist);
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Stimulate Coex running\n!!");
			BTC_TRACE(trace_buf);

			outloop = true;
			break;
		}

	} while (!outloop);



}

void halbtc8821c1ant_psd_antenna_detection_check(IN struct btc_coexist
		*btcoexist)
{
	static u32 ant_det_count = 0, ant_det_fail_count = 0;
	struct  btc_board_info	*board_info = &btcoexist->board_info;

	boolean scan, roam;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_SCAN, &scan);
	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_ROAM, &roam);


	/* psd_scan->ant_det_bt_tx_time = 20; */
	psd_scan->ant_det_bt_tx_time =
		BT_8821C_1ANT_ANTDET_BTTXTIME;	   /* 0.42ms*50 = 20ms (0.42ms = 1 PSD sweep) */
	psd_scan->ant_det_bt_le_channel = BT_8821C_1ANT_ANTDET_BTTXCHANNEL;

	ant_det_count++;

	psd_scan->ant_det_try_count = ant_det_count;

	if (scan || roam) {
		board_info->btdm_ant_det_finish = false;
		psd_scan->ant_det_result = 6;
	} else if (coex_sta->bt_disabled) {
		board_info->btdm_ant_det_finish = false;
		psd_scan->ant_det_result = 11;
	} else if (coex_sta->num_of_profile >= 1) {
		board_info->btdm_ant_det_finish = false;
		psd_scan->ant_det_result = 7;
	} else if (
		!psd_scan->ant_det_is_ant_det_available) { /* Antenna initial setup is not ready */
		board_info->btdm_ant_det_finish = false;
		psd_scan->ant_det_result = 9;
	} else if (coex_sta->c2h_bt_inquiry_page) {
		board_info->btdm_ant_det_finish = false;
		psd_scan->ant_det_result = 10;
	} else {

		btcoexist->stop_coex_dm = true;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"xxxxxxxxxxxxxxxx AntennaDetect(), Stop Coex DM!!\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_psd_antenna_detection(btcoexist,
					      psd_scan->ant_det_bt_tx_time,
					      psd_scan->ant_det_bt_le_channel);

		delay_ms(psd_scan->ant_det_bt_tx_time);

		btcoexist->stop_coex_dm = false;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"xxxxxxxxxxxxxxxx AntennaDetect(), Resume Coex DM\n!!");
		BTC_TRACE(trace_buf);

		/* stimulate coex running */
		/*
		halbtc8821c1ant_run_coexist_mechanism(
				btcoexist);
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Stimulate Coex running\n!!");
		BTC_TRACE(trace_buf);
		*/
	}

	if (!board_info->btdm_ant_det_finish)
		ant_det_fail_count++;

	psd_scan->ant_det_fail_count = ant_det_fail_count;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"xxxxxxxxxxxxxxxx AntennaDetect(), result = %d, fail_count = %d, finish = %s\n",
		    psd_scan->ant_det_result,
		    psd_scan->ant_det_fail_count,
		    board_info->btdm_ant_det_finish == TRUE ? "Yes" : "No");
	BTC_TRACE(trace_buf);

}


/* ************************************************************
 * work around function start with wa_halbtc8821c1ant_
 * ************************************************************
 * ************************************************************
 * extern function start with ex_halbtc8821c1ant_
 * ************************************************************ */
void ex_halbtc8821c1ant_power_on_setting(IN struct btc_coexist *btcoexist)
{
	struct  btc_board_info	*board_info = &btcoexist->board_info;
	u8 u8tmp = 0x0;
	u16 u16tmp = 0x0;
	u32	value = 0;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"xxxxxxxxxxxxxxxx Execute 8821c 1-Ant PowerOn Setting xxxxxxxxxxxxxxxx!!\n");
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "Ant Det Finish = %s, Ant Det Number  = %d\n",
		    (board_info->btdm_ant_det_finish ? "Yes" : "No"),
		    board_info->btdm_ant_num_by_ant_det);
	BTC_TRACE(trace_buf);

	btcoexist->stop_coex_dm = true;
	psd_scan->ant_det_is_ant_det_available = FALSE;
	
	/* enable BB, REG_SYS_FUNC_EN such that we can write BB Register correctly. */
	u16tmp = btcoexist->btc_read_2byte(btcoexist, 0x2);
	btcoexist->btc_write_2byte(btcoexist, 0x2, u16tmp | BIT(0) | BIT(1));

	/* Local setting bit define */
	/*	BIT0: "0" for no antenna inverse; "1" for antenna inverse  */
	/*	BIT1: "0" for internal switch; "1" for external switch */
	/*	BIT2: "0" for one antenna; "1" for two antenna */
	/* NOTE: here default all internal switch and 1-antenna ==> BIT1=0 and BIT2=0 */
		
	/* Check efuse 0xc3[6] for Single Antenna Path */
	if (board_info->single_ant_path == 0) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], **********  Single Antenna, Antenna at Aux Port**********\n");
		BTC_TRACE(trace_buf);

		board_info->btdm_ant_pos = BTC_ANTENNA_AT_AUX_PORT;

		u8tmp = 1;
	} else if (board_info->single_ant_path == 1) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], **********  Single Antenna, Antenna at Main Port**********\n");
		BTC_TRACE(trace_buf);

		board_info->btdm_ant_pos = BTC_ANTENNA_AT_MAIN_PORT;		

		u8tmp = 0;
	}


	/* Save"single antenna position" info in Local register setting for FW reading, because FW may not ready at  power on */
	if (btcoexist->chip_interface == BTC_INTF_PCI)
		btcoexist->btc_write_local_reg_1byte(btcoexist, 0x3e0, u8tmp);
	else if (btcoexist->chip_interface == BTC_INTF_USB)
		btcoexist->btc_write_local_reg_1byte(btcoexist, 0xfe08, u8tmp);
	else if (btcoexist->chip_interface == BTC_INTF_SDIO)
		btcoexist->btc_write_local_reg_1byte(btcoexist, 0x60, u8tmp);

	/* Setup RF front end type */
	halbtc8821c1ant_set_rfe_type(btcoexist);
	
	/* Set Antenna Path to BT side */
	halbtc8821c1ant_set_ant_path(btcoexist, 
					BTC_ANT_PATH_AUTO,
					FORCE_EXEC, 
					BT_8821C_1ANT_PHASE_COEX_INIT);

	/* enable GNT_WL/GNT_BT debug signal to GPIO14/15 */
	halbtc8821c1ant_enable_gnt_to_gpio(btcoexist, TRUE);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], **********  LTE coex Reg 0x38 (Power-On) = 0x%x**********\n",
		    halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38));
	BTC_TRACE(trace_buf);

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], **********  MAC Reg 0x70/ BB Reg 0xcb4 (Power-On) = 0x%x / 0x%x**********\n",
		btcoexist->btc_read_4byte(btcoexist, 0x70), btcoexist->btc_read_4byte(btcoexist, 0xcb4));
	BTC_TRACE(trace_buf);

}

void ex_halbtc8821c1ant_pre_load_firmware(IN struct btc_coexist *btcoexist)
{
}

void ex_halbtc8821c1ant_init_hw_config(IN struct btc_coexist *btcoexist,
				       IN boolean wifi_only)
{
	halbtc8821c1ant_init_hw_config(btcoexist, true, wifi_only);
	btcoexist->stop_coex_dm = false;

}

void ex_halbtc8821c1ant_init_coex_dm(IN struct btc_coexist *btcoexist)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], Coex Mechanism Init!!\n");
	BTC_TRACE(trace_buf);

	halbtc8821c1ant_init_coex_dm(btcoexist);

	/* halbtc8821c1ant_query_bt_info(btcoexist); */
}

void ex_halbtc8821c1ant_display_coex_info(IN struct btc_coexist *btcoexist)
{
	struct  btc_board_info		*board_info = &btcoexist->board_info;
	struct  btc_stack_info		*stack_info = &btcoexist->stack_info;
	struct  btc_bt_link_info	*bt_link_info = &btcoexist->bt_link_info;
	u8				*cli_buf = btcoexist->cli_buf;
	u8				u8tmp[4], i, bt_info_ext, ps_tdma_case = 0;
	u16				u16tmp[4];
	u32				u32tmp[4];
	u32				fa_of_dm, fa_cck;
	u32				fw_ver = 0, bt_patch_ver = 0;
	static u8			pop_report_in_10s = 0;

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n ============[BT Coexist info]============");
	CL_PRINTF(cli_buf);

	if (btcoexist->manual_control) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			"\r\n ============[Under Manual Control]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ==========================================");
		CL_PRINTF(cli_buf);
	}
	if (btcoexist->stop_coex_dm) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ============[Coex is STOPPED]============");
		CL_PRINTF(cli_buf);
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n ==========================================");
		CL_PRINTF(cli_buf);
	}

	if (psd_scan->ant_det_try_count == 0) {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %s / %d",
			   "Ant PG Num/ Mech/ Pos/ RFE",
			   board_info->pg_ant_num, board_info->btdm_ant_num,
			   (board_info->btdm_ant_pos == BTC_ANTENNA_AT_MAIN_PORT? "Main" : "Aux"),
			   rfe_type->rfe_module_type);
		CL_PRINTF(cli_buf);
	} else {
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
			   "\r\n %-35s = %d/ %d/ %s/ %d  (%d/%d/%d)",
			   "Ant PG Num/ Mech(Ant_Det)/ Pos/ RFE",
			  board_info->pg_ant_num, board_info->btdm_ant_num_by_ant_det,
			   (board_info->btdm_ant_pos == BTC_ANTENNA_AT_MAIN_PORT ? "Main" : "Aux"), 
			   rfe_type->rfe_module_type,
			   psd_scan->ant_det_try_count, psd_scan->ant_det_fail_count,
			   psd_scan->ant_det_result);
		CL_PRINTF(cli_buf);

		if (board_info->btdm_ant_det_finish) {
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s",
				   "Ant Det PSD Value",
				   psd_scan->ant_det_peak_val);
			CL_PRINTF(cli_buf);
		}
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s / %d",
		   "BT stack/ hci ext ver",
		   ((stack_info->profile_notified) ? "Yes" : "No"),
		   stack_info->hci_version);
	CL_PRINTF(cli_buf);

	btcoexist->btc_get(btcoexist, BTC_GET_U4_BT_PATCH_VER, &bt_patch_ver);
	btcoexist->btc_get(btcoexist, BTC_GET_U4_WIFI_FW_VER, &fw_ver);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d_%x/ 0x%x/ 0x%x(%d)/ %c",
		   "Version Coex/ Fw/ Patch/ Cut",
		   glcoex_ver_date_8821c_1ant, glcoex_ver_8821c_1ant, fw_ver,
		   bt_patch_ver, bt_patch_ver, coex_sta->cut_version + 65);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %02x %02x %02x ",
		   "Wifi channel informed to BT",
		   coex_dm->wifi_chnl_info[0], coex_dm->wifi_chnl_info[1],
		   coex_dm->wifi_chnl_info[2]);
	CL_PRINTF(cli_buf);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s/ %s/ %s",
		   "WifibHiPri/ Ccklock/ CckEverLock",
		   (coex_sta->wifi_is_high_pri_task ? "Yes" : "No"),
		   (coex_sta->cck_lock ? "Yes" : "No"),
		   (coex_sta->cck_ever_lock ? "Yes" : "No"));
	CL_PRINTF(cli_buf);

	/* wifi status */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[Wifi Status]============");
	CL_PRINTF(cli_buf);
	btcoexist->btc_disp_dbg_msg(btcoexist, BTC_DBG_DISP_WIFI_STATUS);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[BT Status]============");
	CL_PRINTF(cli_buf);

	pop_report_in_10s++;
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = [%s/ %d/ %d/ %d] ",
		   "BT [status/ rssi/ retryCnt/ popCnt]",
		   ((coex_sta->bt_disabled) ? ("disabled") :	((
			   coex_sta->c2h_bt_inquiry_page) ? ("inquiry/page")
			   : ((BT_8821C_1ANT_BT_STATUS_NON_CONNECTED_IDLE ==
			       coex_dm->bt_status) ? "non-connected idle" :
		((BT_8821C_1ANT_BT_STATUS_CONNECTED_IDLE == coex_dm->bt_status)
				       ? "connected-idle" : "busy")))),
		   coex_sta->bt_rssi - 100, coex_sta->bt_retry_cnt,
		   coex_sta->pop_event_cnt);
	CL_PRINTF(cli_buf);

	if (pop_report_in_10s >= 5) {
		coex_sta->pop_event_cnt = 0;
		pop_report_in_10s = 0;
	}

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %d / %d / %d / %d / %d",
		   "SCO/HID/PAN/A2DP/Hi-Pri",
		   bt_link_info->sco_exist, bt_link_info->hid_exist,
		   bt_link_info->pan_exist, bt_link_info->a2dp_exist,
		   bt_link_info->bt_hi_pri_link_exist);
	CL_PRINTF(cli_buf);

	if (stack_info->profile_notified)
		btcoexist->btc_disp_dbg_msg(btcoexist,
					    BTC_DBG_DISP_BT_LINK_INFO);
	else {
		bt_info_ext = coex_sta->bt_info_ext;

		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %s / %s / %d/ %x/ %04x",
			   "Role/A2DP Rate/Bitpool/Feature/Ver",
			   ((bt_link_info->slave_role) ? "Slave" : "Master"),
			(bt_info_ext & BIT(0)) ? "BR" : "EDR", coex_sta->a2dp_bit_pool,
			coex_sta->bt_coex_supported_feature,
			coex_sta->bt_coex_supported_version);
		CL_PRINTF(cli_buf);
	}


	for (i = 0; i < BT_INFO_SRC_8821C_1ANT_MAX; i++) {
		if (coex_sta->bt_info_c2h_cnt[i]) {
			CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
				"\r\n %-35s = %02x %02x %02x %02x %02x %02x %02x(%d)",
				   glbt_info_src_8821c_1ant[i],
				   coex_sta->bt_info_c2h[i][0],
				   coex_sta->bt_info_c2h[i][1],
				   coex_sta->bt_info_c2h[i][2],
				   coex_sta->bt_info_c2h[i][3],
				   coex_sta->bt_info_c2h[i][4],
				   coex_sta->bt_info_c2h[i][5],
				   coex_sta->bt_info_c2h[i][6],
				   coex_sta->bt_info_c2h_cnt[i]);
			CL_PRINTF(cli_buf);
		}
	}


	if (btcoexist->manual_control)
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
			"============[mechanisms] (before Manual)============");
	else
		CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
			   "============[mechanisms]============");

	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
		   "SM[LowPenaltyRA]",
		   coex_dm->cur_low_penalty_ra);
	CL_PRINTF(cli_buf);

	ps_tdma_case = coex_dm->cur_ps_tdma;
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = %02x %02x %02x %02x %02x case-%d (%s,%s)",
		   "PS TDMA",
		   coex_dm->ps_tdma_para[0], coex_dm->ps_tdma_para[1],
		   coex_dm->ps_tdma_para[2], coex_dm->ps_tdma_para[3],
		   coex_dm->ps_tdma_para[4], ps_tdma_case,
		   (coex_dm->cur_ps_tdma_on ? "On" : "Off"),
		   (coex_dm->auto_tdma_adjust ? "Adj" : "Fix"));

	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d",
		   "WL/BT Coex Table Type",
		   coex_sta->coex_table_type);
	CL_PRINTF(cli_buf);

	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0x6c0);
	u32tmp[1] = btcoexist->btc_read_4byte(btcoexist, 0x6c4);
	u32tmp[2] = btcoexist->btc_read_4byte(btcoexist, 0x6c8);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x",
		   "0x6c0/0x6c4/0x6c8(coexTable)",
		   u32tmp[0], u32tmp[1], u32tmp[2]);
	CL_PRINTF(cli_buf);

	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0x778);
	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0x6cc);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x",
		   "0x778/0x6cc/IgnWlanAct",
		   u8tmp[0], u32tmp[0],  coex_dm->cur_ignore_wlan_act);
	CL_PRINTF(cli_buf);

	u32tmp[0] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xa0);
	u32tmp[1] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xa4);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = 0x%x/ 0x%x",
		   "LTE Coex Table W_L/B_L",
		   u32tmp[0] & 0xffff, u32tmp[1] & 0xffff);
	CL_PRINTF(cli_buf);

	u32tmp[0] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xa8);
	u32tmp[1] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xac);
	u32tmp[2] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xb0);
	u32tmp[3] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist,
			0xb4);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x",
		   "LTE Break Table W_L/B_L/L_W/L_B",
		   u32tmp[0] & 0xffff, u32tmp[1] & 0xffff,
		   u32tmp[2] & 0xffff, u32tmp[3] & 0xffff);
	CL_PRINTF(cli_buf);

	/* Hw setting		 */
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s",
		   "============[Hw setting]============");
	CL_PRINTF(cli_buf);


	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0x430);
	u32tmp[1] = btcoexist->btc_read_4byte(btcoexist, 0x434);
	u16tmp[0] = btcoexist->btc_read_2byte(btcoexist, 0x42a);
	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0x456);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = 0x%x/0x%x/0x%x/0x%x",
		   "0x430/0x434/0x42a/0x456",
		   u32tmp[0], u32tmp[1], u16tmp[0], u8tmp[0]);
	CL_PRINTF(cli_buf);


	u32tmp[0] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x38);
	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0x73);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %s",
		   "LTE CoexOn/Path Ctrl Owner",
		(int)((u32tmp[0] & BIT(7)) >> 7), ((u8tmp[0] & BIT(2)) ? "WL" : "BT"));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "LTE 3Wire/OPMode/UART/UARTMode",
		(int)((u32tmp[0] & BIT(6)) >> 6), (int)((u32tmp[0] & (BIT(5) | BIT(4))) >> 4),
		   (int)((u32tmp[0] & BIT(3)) >> 3),
		   (int)(u32tmp[0] & (BIT(2) | BIT(1) | BIT(0))));
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %s",
		   "GNT_WL_SWCtrl/GNT_BT_SWCtrl/Dbg",
		(int)((u32tmp[0] & BIT(12)) >> 12), (int)((u32tmp[0] & BIT(14)) >> 14),
		   ((u8tmp[0] & BIT(3)) ? "On" : "Off"));
	CL_PRINTF(cli_buf);

	u32tmp[0] = halbtc8821c1ant_ltecoex_indirect_read_reg(btcoexist, 0x54);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "GNT_WL/GNT_BT/LTE_Busy/UART_Busy",
		(int)((u32tmp[0] & BIT(2)) >> 2), (int)((u32tmp[0] & BIT(3)) >> 3),
		   (int)((u32tmp[0] & BIT(1)) >> 1), (int)(u32tmp[0] & BIT(0)));
	CL_PRINTF(cli_buf);


	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0xcb0);
	u32tmp[1] = btcoexist->btc_read_4byte(btcoexist, 0xcb4);
	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0xcba);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = 0x%04x/ 0x%04x/ 0x%02x %s",
		   "0xcb0/0xcb4/0xcb8[23:16]",
		u32tmp[0], u32tmp[1],u8tmp[0],  (   (u8tmp[0] & 0x5) == 0x1 ?  "BTG":   ( (u8tmp[0] & 0xf0) == 0x70?  "WL_G"  : "WL_A" )  ));
	CL_PRINTF(cli_buf);

	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0x4c);
	u8tmp[2] = btcoexist->btc_read_1byte(btcoexist, 0x64);
	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0x4c6);
	u8tmp[1] = btcoexist->btc_read_1byte(btcoexist, 0x40);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x",
		   "0x4c[24:23]/0x64[0]/x4c6[4]/0x40[5]",
		(u32tmp[0] & ( BIT(24) | BIT(23) )) >> 23 , u8tmp[2]&0x1 , (int)((u8tmp[0] & BIT(4)) >> 4), (int)((u8tmp[1] & BIT(5)) >> 5));
	CL_PRINTF(cli_buf);

	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0x550);
	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0x522);
	u8tmp[1] = btcoexist->btc_read_1byte(btcoexist, 0x953);
	u8tmp[2] = btcoexist->btc_read_1byte(btcoexist, 0xc50);
	
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = 0x%x/ 0x%x/ %s/ 0x%x",
		   "0x550/0x522/4-RxAGC/0xc50",
		u32tmp[0], u8tmp[0], (u8tmp[1] & 0x2) ? "On" : "Off", u8tmp[2]);
	CL_PRINTF(cli_buf);

	u32tmp[0] = btcoexist->btc_read_4byte(btcoexist, 0xf48);
	u32tmp[1] = btcoexist->btc_read_4byte(btcoexist, 0xf4c);
	u32tmp[2] = btcoexist->btc_read_4byte(btcoexist, 0xf08);

	u8tmp[0] = btcoexist->btc_read_1byte(btcoexist, 0xa5c);
	u8tmp[1] = btcoexist->btc_read_1byte(btcoexist, 0xa5d);

	fa_of_dm = (u32tmp[0] & 0xffff)  +  (u32tmp[1] & 0xffff);
		
	fa_cck = (u8tmp[0] << 8) + u8tmp[1];

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE,
		   "\r\n %-35s = 0x%x/ 0x%x/ 0x%x/ 0x%x",
		   "CCK-CCA/CCK-FA/OFDM-CCA/OFDM-FA",
		   u32tmp[2] & 0xffff, fa_cck, ((u32tmp[2] & 0xffff0000) >> 16), fa_of_dm);
	CL_PRINTF(cli_buf);


	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "CRC_OK CCK/11g/11n/11ac",
		   coex_sta->crc_ok_cck, coex_sta->crc_ok_11g,
		   coex_sta->crc_ok_11n, coex_sta->crc_ok_11n_agg);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d/ %d/ %d",
		   "CRC_Err CCK/11g/11n/11ac",
		   coex_sta->crc_err_cck, coex_sta->crc_err_11g,
		   coex_sta->crc_err_11n, coex_sta->crc_err_11n_agg);
	CL_PRINTF(cli_buf);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d",
		   "0x770(high-pri rx/tx)",
		   coex_sta->high_priority_rx, coex_sta->high_priority_tx);
	CL_PRINTF(cli_buf);
	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %d/ %d",
		   "0x774(low-pri rx/tx)",
		   coex_sta->low_priority_rx, coex_sta->low_priority_tx);
	CL_PRINTF(cli_buf);

	halbtc8821c1ant_read_score_board(btcoexist,	&u16tmp[0]);

	CL_SPRINTF(cli_buf, BT_TMP_BUF_SIZE, "\r\n %-35s = %04x",
		   "ScoreBoard[14:0] (from BT)", u16tmp[0]);
	CL_PRINTF(cli_buf);

	btcoexist->btc_disp_dbg_msg(btcoexist, BTC_DBG_DISP_COEX_STATISTICS);
}


void ex_halbtc8821c1ant_ips_notify(IN struct btc_coexist *btcoexist, IN u8 type)
{
	if (btcoexist->manual_control ||	btcoexist->stop_coex_dm)
		return;

	if (BTC_IPS_ENTER == type) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], IPS ENTER notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->under_ips = true;

		/* Write WL "Active" in Score-board for LPS off */
		halbtc8821c1ant_post_activestate_to_bt(btcoexist, false);

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 0);
		
		halbtc8821c1ant_set_ant_path(btcoexist, 
							BTC_ANT_PATH_AUTO,
					     	FORCE_EXEC, 
					     	BT_8821C_1ANT_PHASE_WLAN_OFF);
		
		halbtc8821c1ant_coex_table_with_type(btcoexist, NORMAL_EXEC, 0);
	} else if (BTC_IPS_LEAVE == type) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], IPS LEAVE notify\n");
		BTC_TRACE(trace_buf);
		halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

		halbtc8821c1ant_init_hw_config(btcoexist, false, false);
		halbtc8821c1ant_init_coex_dm(btcoexist);;

		coex_sta->under_ips = false;
	}
}

void ex_halbtc8821c1ant_lps_notify(IN struct btc_coexist *btcoexist, IN u8 type)
{
	if (btcoexist->manual_control || btcoexist->stop_coex_dm)
		return;

	if (BTC_LPS_ENABLE == type) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], LPS ENABLE notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->under_lps = true;

		if (coex_sta->force_lps_on == true) { /* LPS No-32K */
			/* Write WL "Active" in Score-board for PS-TDMA */
			halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

		} else { /* LPS-32K, need check if this h2c 0x71 can work?? (2015/08/28) */
			/* Write WL "Non-Active" in Score-board for Native-PS */
			halbtc8821c1ant_post_activestate_to_bt(btcoexist, false);

		}
	} else if (BTC_LPS_DISABLE == type) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], LPS DISABLE notify\n");
		BTC_TRACE(trace_buf);
		coex_sta->under_lps = false;

		/* Write WL "Active" in Score-board for LPS off */
		halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

	}
}

void ex_halbtc8821c1ant_scan_notify(IN struct btc_coexist *btcoexist,
				    IN u8 type)
{
	boolean wifi_connected = false;



	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], SCAN notify()\n");
	BTC_TRACE(trace_buf);

	halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

	if (btcoexist->manual_control ||
	    btcoexist->stop_coex_dm)
		return;

	if (BTC_SCAN_START == type)   //for dual band, no action for BTC_SCAN_START, the action is change to switchband_notify
		return;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);

	halbtc8821c1ant_query_bt_info(btcoexist);

	if (BTC_SCAN_START_2G == type) {	

		if (!wifi_connected)
			coex_sta->wifi_is_high_pri_task = true;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], SCAN START notify  (2G)\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false,
					8);

		/* Force antenna setup for no scan result issue */
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     FORCE_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);

		if (!wifi_connected)	/* non-connected scan */
			halbtc8821c1ant_action_wifi_not_connected_scan(btcoexist);
		else	/* wifi is connected */
			halbtc8821c1ant_action_wifi_connected_scan(btcoexist);

	} else {

		coex_sta->wifi_is_high_pri_task = false;

		btcoexist->btc_get(btcoexist, BTC_GET_U1_AP_NUM,
				   &coex_sta->scan_ap_num);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], SCAN FINISH notify  (Scan-AP = %d)\n", coex_sta->scan_ap_num);
		BTC_TRACE(trace_buf);

		if (!wifi_connected)
			halbtc8821c1ant_action_wifi_not_connected(btcoexist);
		else
			halbtc8821c1ant_action_wifi_connected(btcoexist);
	}


}

void ex_halbtc8821c1ant_switchband_notify(IN struct btc_coexist *btcoexist,
				    IN u8 type)
{

	boolean wifi_connected = false, bt_hs_on = false;
	u32	wifi_link_status = 0;
	u32	num_of_wifi_link = 0;
	boolean	bt_ctrl_agg_buf_size = false;
	u8	agg_buf_size = 5;

	
	if (btcoexist->manual_control ||
	    btcoexist->stop_coex_dm)
		return;


	if(type == BTC_SWITCH_TO_5G) {
		
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], switchband_notify ---  switch to 5G\n");
		BTC_TRACE(trace_buf);
		
		halbtc8821c1ant_action_wifi_under5g(btcoexist);

	} else {
	
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], switchband_notify ---  switch to 2G\n");
		BTC_TRACE(trace_buf);
		
		ex_halbtc8821c1ant_scan_notify(btcoexist,
				    BTC_SCAN_START_2G);
	}
}


void ex_halbtc8821c1ant_connect_notify(IN struct btc_coexist *btcoexist,
				       IN u8 type)
{
	boolean	wifi_connected = false;


	halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

	if (btcoexist->manual_control ||
	    btcoexist->stop_coex_dm)
		return;

	if ( (BTC_ASSOCIATE_5G_START == type) ||  (BTC_ASSOCIATE_5G_FINISH== type)) {

			if (BTC_ASSOCIATE_5G_START == type)
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						  "[BTCoex], connect_notify ---  5G start\n");
			else
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
						  "[BTCoex], connect_notify ---  5G finish\n");
				
			BTC_TRACE(trace_buf);

			halbtc8821c1ant_action_wifi_under5g(btcoexist);
			return;
		}

	if (BTC_ASSOCIATE_START == type) {

		coex_sta->wifi_is_high_pri_task = true;

		/* Force antenna setup for no scan result issue */
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     FORCE_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], CONNECT START notify (2G)\n");
		BTC_TRACE(trace_buf);

		coex_dm->arp_cnt = 0;

		halbtc8821c1ant_action_wifi_not_connected_asso_auth(btcoexist);
	} else {

		coex_sta->wifi_is_high_pri_task = false;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], CONNECT FINISH notify (2G)\n");
		BTC_TRACE(trace_buf);

		btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
			   &wifi_connected);

		if (!wifi_connected) /* non-connected scan */
			halbtc8821c1ant_action_wifi_not_connected(btcoexist);
		else
			halbtc8821c1ant_action_wifi_connected(btcoexist);
	}

}

void ex_halbtc8821c1ant_media_status_notify(IN struct btc_coexist *btcoexist,
		IN u8 type)
{
	boolean			wifi_under_b_mode = false, wifi_under_5g = false;


	if (btcoexist->manual_control ||
	    btcoexist->stop_coex_dm)
		return;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_5G, &wifi_under_5g);

	if (wifi_under_5g) {
		
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], WiFi is under 5G!!!\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);		
		halbtc8821c1ant_action_wifi_under5g(btcoexist);
		return;
	}

	if (BTC_MEDIA_CONNECT == type) {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], MEDIA connect notify\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, true);

		/* Force antenna setup for no scan result issue */
		halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_AUTO,
					     FORCE_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);

		btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_B_MODE,
				   &wifi_under_b_mode);

		/* Set CCK Tx/Rx high Pri except 11b mode */
		if (wifi_under_b_mode) {
			btcoexist->btc_write_1byte(btcoexist, 0x6cd,
						   0x00); /* CCK Tx */
			btcoexist->btc_write_1byte(btcoexist, 0x6cf,
						   0x00); /* CCK Rx */
		} else {
			/* btcoexist->btc_write_1byte(btcoexist, 0x6cd, 0x10); */ /*CCK Tx */
			/* btcoexist->btc_write_1byte(btcoexist, 0x6cf, 0x10); */ /*CCK Rx */
			btcoexist->btc_write_1byte(btcoexist, 0x6cd,
						   0x00); /* CCK Tx */
			btcoexist->btc_write_1byte(btcoexist, 0x6cf,
						   0x10); /* CCK Rx */
		}

		coex_dm->backup_arfr_cnt1 = btcoexist->btc_read_4byte(btcoexist,
					    0x430);
		coex_dm->backup_arfr_cnt2 = btcoexist->btc_read_4byte(btcoexist,
					    0x434);
		coex_dm->backup_retry_limit = btcoexist->btc_read_2byte(
						      btcoexist, 0x42a);
		coex_dm->backup_ampdu_max_time = btcoexist->btc_read_1byte(
				btcoexist, 0x456);
	} else {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], MEDIA disconnect notify\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, false);

		btcoexist->btc_write_1byte(btcoexist, 0x6cd, 0x0); /* CCK Tx */
		btcoexist->btc_write_1byte(btcoexist, 0x6cf, 0x0); /* CCK Rx */

		coex_sta->cck_ever_lock = false;
	}

	halbtc8821c1ant_update_wifi_channel_info(btcoexist, type);

}

void ex_halbtc8821c1ant_specific_packet_notify(IN struct btc_coexist *btcoexist,
		IN u8 type)
{
	boolean	under_4way = false, wifi_under_5g = false;

	if (btcoexist->manual_control ||
	    btcoexist->stop_coex_dm ||
	    coex_sta->bt_disabled)
		return;

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_UNDER_5G, &wifi_under_5g);

	if (wifi_under_5g) {
		
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], WiFi is under 5G!!!\n");
		BTC_TRACE(trace_buf);
		
		halbtc8821c1ant_action_wifi_under5g(btcoexist);
		return;
	}

	btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_4_WAY_PROGRESS,
			   &under_4way);

	if (under_4way) {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], specific Packet ---- under_4way!!\n");
		BTC_TRACE(trace_buf);

		coex_sta->wifi_is_high_pri_task = true;
		coex_sta->specific_pkt_period_cnt = 2;
	} else if (BTC_PACKET_ARP == type) {

		coex_dm->arp_cnt++;

		if (coex_sta->wifi_is_high_pri_task) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"[BTCoex], specific Packet ARP notify -cnt = %d\n", coex_dm->arp_cnt);
			BTC_TRACE(trace_buf);
		}

	} else {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], specific Packet DHCP or EAPOL notify [Type = %d]\n", type);
		BTC_TRACE(trace_buf);

		coex_sta->wifi_is_high_pri_task = true;
		coex_sta->specific_pkt_period_cnt = 2;
	}

	if (coex_sta->wifi_is_high_pri_task)
		halbtc8821c1ant_action_wifi_connected_specific_packet(btcoexist);
}

void ex_halbtc8821c1ant_bt_info_notify(IN struct btc_coexist *btcoexist,
				       IN u8 *tmp_buf, IN u8 length)
{
	u8				bt_info = 0;
	u8				i, rsp_source = 0;
	boolean				wifi_connected = false;
	boolean				bt_busy = false;

	coex_sta->c2h_bt_info_req_sent = false;

	rsp_source = tmp_buf[0] & 0xf;
	if (rsp_source >= BT_INFO_SRC_8821C_1ANT_MAX)
		rsp_source = BT_INFO_SRC_8821C_1ANT_WIFI_FW;
	coex_sta->bt_info_c2h_cnt[rsp_source]++;

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "[BTCoex], Bt info[%d], length=%d, hex data=[", rsp_source,
		    length);
	BTC_TRACE(trace_buf);
	for (i = 0; i < length; i++) {
		coex_sta->bt_info_c2h[rsp_source][i] = tmp_buf[i];
		if (i == 1)
			bt_info = tmp_buf[i];
		if (i == length - 1) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "0x%02x]\n",
				    tmp_buf[i]);
			BTC_TRACE(trace_buf);
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "0x%02x, ",
				    tmp_buf[i]);
			BTC_TRACE(trace_buf);
		}
	}

	if (BT_INFO_SRC_8821C_1ANT_WIFI_FW != rsp_source) {

		/* if 0xff, it means BT is under WHCK test */
		if (bt_info == 0xff)
			coex_sta->bt_whck_test = true;
		else
			coex_sta->bt_whck_test = false;

		coex_sta->bt_retry_cnt =	/* [3:0] */
			coex_sta->bt_info_c2h[rsp_source][2] & 0xf;

		if (coex_sta->bt_retry_cnt >= 1)
			coex_sta->pop_event_cnt++;

		if (coex_sta->bt_info_c2h[rsp_source][2] & 0x20)
			coex_sta->c2h_bt_page = true;
		else
			coex_sta->c2h_bt_page = false;

		if (coex_sta->bt_info_c2h[rsp_source][2] & 0x80)
			coex_sta->bt_create_connection = true;
		else
			coex_sta->bt_create_connection = false;

		/* unit: %, value-100 to translate to unit: dBm  */
		coex_sta->bt_rssi = coex_sta->bt_info_c2h[rsp_source][3] * 2 + 10;

		/* coex_sta->bt_info_c2h[rsp_source][3] * 2 - 90; */

		if ((coex_sta->bt_info_c2h[rsp_source][1] & 0x49) == 0x49) {
			coex_sta->a2dp_bit_pool =
				coex_sta->bt_info_c2h[rsp_source][6];
		} else
			coex_sta->a2dp_bit_pool = 0;

		if (coex_sta->bt_info_c2h[rsp_source][1] & 0x9)
			coex_sta->acl_busy = true;
		else
			coex_sta->acl_busy = false;

		coex_sta->bt_info_ext =
			coex_sta->bt_info_c2h[rsp_source][4];

		/* Here we need to resend some wifi info to BT */
		/* because bt is reset and loss of the info. */

		if ((!btcoexist->manual_control) && (!btcoexist->stop_coex_dm)) {

			/*  Re-Init */
			if (coex_sta->bt_info_ext & BIT(1)) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT ext info bit1 check, send wifi BW&Chnl to BT!!\n");
				BTC_TRACE(trace_buf);
				btcoexist->btc_get(btcoexist, BTC_GET_BL_WIFI_CONNECTED,
						   &wifi_connected);
				if (wifi_connected)
					halbtc8821c1ant_update_wifi_channel_info(btcoexist,
							BTC_MEDIA_CONNECT);
				else
					halbtc8821c1ant_update_wifi_channel_info(btcoexist,
							BTC_MEDIA_DISCONNECT);
			}

			/*  If Ignore_WLanAct && not SetUp_Link */
			if ((coex_sta->bt_info_ext & BIT(3)) && (!(coex_sta->bt_info_ext & BIT(2)))) {

				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"[BTCoex], BT ext info bit3 check, set BT NOT to ignore Wlan active!!\n");
				BTC_TRACE(trace_buf);
				halbtc8821c1ant_ignore_wlan_act(btcoexist,
							FORCE_EXEC, false);
			}
		}
		/* check BIT2 first ==> check if bt is under inquiry or page scan */
		if (bt_info & BT_INFO_8821C_1ANT_B_INQ_PAGE)
			coex_sta->c2h_bt_inquiry_page = true;
		else
			coex_sta->c2h_bt_inquiry_page = false;
	}

	coex_sta->num_of_profile = 0;

	/* set link exist status */
	if (!(bt_info & BT_INFO_8821C_1ANT_B_CONNECTION)) {
		coex_sta->bt_link_exist = false;
		coex_sta->pan_exist = false;
		coex_sta->a2dp_exist = false;
		coex_sta->hid_exist = false;
		coex_sta->sco_exist = false;

		coex_sta->bt_hi_pri_link_exist = false;
	} else {	/* connection exists */
		coex_sta->bt_link_exist = true;
		if (bt_info & BT_INFO_8821C_1ANT_B_FTP) {
			coex_sta->pan_exist = true;
			coex_sta->num_of_profile++;
		} else
			coex_sta->pan_exist = false;
		if (bt_info & BT_INFO_8821C_1ANT_B_A2DP) {
			coex_sta->a2dp_exist = true;
			coex_sta->num_of_profile++;
		} else
			coex_sta->a2dp_exist = false;
		if (bt_info & BT_INFO_8821C_1ANT_B_HID) {
			coex_sta->hid_exist = true;
			coex_sta->num_of_profile++;
		} else
			coex_sta->hid_exist = false;
		if (bt_info & BT_INFO_8821C_1ANT_B_SCO_ESCO) {
			coex_sta->sco_exist = true;
			coex_sta->num_of_profile++;
		} else
			coex_sta->sco_exist = false;

	}

	halbtc8821c1ant_update_bt_link_info(btcoexist);

	bt_info = bt_info &
		0x1f;  /* mask profile bit for connect-ilde identification ( for CSR case: A2DP idle --> 0x41) */

	if (!(bt_info & BT_INFO_8821C_1ANT_B_CONNECTION)) {
		coex_dm->bt_status = BT_8821C_1ANT_BT_STATUS_NON_CONNECTED_IDLE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], BtInfoNotify(), BT Non-Connected idle!!!\n");
		BTC_TRACE(trace_buf);
	} else if (bt_info ==
		BT_INFO_8821C_1ANT_B_CONNECTION) {	/* connection exists but no busy */
		coex_dm->bt_status = BT_8821C_1ANT_BT_STATUS_CONNECTED_IDLE;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BtInfoNotify(), BT Connected-idle!!!\n");
		BTC_TRACE(trace_buf);
	} else if ((bt_info & BT_INFO_8821C_1ANT_B_SCO_ESCO) ||
		   (bt_info & BT_INFO_8821C_1ANT_B_SCO_BUSY)) {
		coex_dm->bt_status = BT_8821C_1ANT_BT_STATUS_SCO_BUSY;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BtInfoNotify(), BT SCO busy!!!\n");
		BTC_TRACE(trace_buf);
	} else if (bt_info & BT_INFO_8821C_1ANT_B_ACL_BUSY) {
		if (BT_8821C_1ANT_BT_STATUS_ACL_BUSY != coex_dm->bt_status)
			coex_dm->auto_tdma_adjust = false;
		coex_dm->bt_status = BT_8821C_1ANT_BT_STATUS_ACL_BUSY;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], BtInfoNotify(), BT ACL busy!!!\n");
		BTC_TRACE(trace_buf);
	} else {
		coex_dm->bt_status = BT_8821C_1ANT_BT_STATUS_MAX;
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], BtInfoNotify(), BT Non-Defined state!!!\n");
		BTC_TRACE(trace_buf);
	}

	if ((BT_8821C_1ANT_BT_STATUS_ACL_BUSY == coex_dm->bt_status) ||
	    (BT_8821C_1ANT_BT_STATUS_SCO_BUSY == coex_dm->bt_status) ||
	    (BT_8821C_1ANT_BT_STATUS_ACL_SCO_BUSY == coex_dm->bt_status))
		bt_busy = true;
	else
		bt_busy = false;
	btcoexist->btc_set(btcoexist, BTC_SET_BL_BT_TRAFFIC_BUSY, &bt_busy);

	if (btcoexist->manual_control) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], BtInfoNotify(), No run_coexist_mechanism return for Manual CTRL<===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	if (btcoexist->stop_coex_dm) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], BtInfoNotify(),  No run_coexist_mechanism return for Stop Coex DM <===\n");
		BTC_TRACE(trace_buf);
		return;
	}

	halbtc8821c1ant_run_coexist_mechanism(btcoexist);
}

void ex_halbtc8821c1ant_rf_status_notify(IN struct btc_coexist *btcoexist,
		IN u8 type)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], RF Status notify\n");
	BTC_TRACE(trace_buf);

	if (BTC_RF_ON == type) {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], RF is turned ON!!\n");
		BTC_TRACE(trace_buf);

		coex_sta->wl_rf_off_on_event = true;
		btcoexist->stop_coex_dm = false;

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, TRUE);
		halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, TRUE);

	} else if (BTC_RF_OFF == type) {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], RF is turned OFF!!\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, FALSE);
		halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, FALSE);

		halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 0);
		halbtc8821c1ant_set_ant_path(btcoexist, 
							BTC_ANT_PATH_AUTO,
					     	FORCE_EXEC, 
					     	BT_8821C_1ANT_PHASE_WLAN_OFF);

		btcoexist->stop_coex_dm = true;
		coex_sta->wl_rf_off_on_event = false;
	}
}

void ex_halbtc8821c1ant_halt_notify(IN struct btc_coexist *btcoexist)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], Halt notify\n");
	BTC_TRACE(trace_buf);

	/* enable GNT_WL/GNT_BT debug signal to GPIO14/15 */
	halbtc8821c1ant_enable_gnt_to_gpio(btcoexist, FALSE);

	halbtc8821c1ant_post_activestate_to_bt(btcoexist, FALSE);
	halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, FALSE);

	halbtc8821c1ant_ps_tdma(btcoexist, FORCE_EXEC, false, 0);
	
	halbtc8821c1ant_set_ant_path(btcoexist, 
							BTC_ANT_PATH_AUTO,
					     	FORCE_EXEC, 
					     	BT_8821C_1ANT_PHASE_WLAN_OFF);

	halbtc8821c1ant_ignore_wlan_act(btcoexist, FORCE_EXEC, true);

	ex_halbtc8821c1ant_media_status_notify(btcoexist, BTC_MEDIA_DISCONNECT);

	btcoexist->stop_coex_dm = true;
}

void ex_halbtc8821c1ant_pnp_notify(IN struct btc_coexist *btcoexist,
				   IN u8 pnp_state)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE, "[BTCoex], Pnp notify\n");
	BTC_TRACE(trace_buf);

	if (BTC_WIFI_PNP_SLEEP == pnp_state) {
		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Pnp notify to SLEEP\n");
		BTC_TRACE(trace_buf);

		/* enable GNT_WL/GNT_BT debug signal to GPIO14/15 */
		halbtc8821c1ant_enable_gnt_to_gpio(btcoexist, FALSE);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, FALSE);
		halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, FALSE);

		halbtc8821c1ant_ps_tdma(btcoexist, NORMAL_EXEC, false, 0);
		
		halbtc8821c1ant_set_ant_path(btcoexist, 
							BTC_ANT_PATH_AUTO,
					     	FORCE_EXEC, 
					     	BT_8821C_1ANT_PHASE_WLAN_OFF);

		btcoexist->stop_coex_dm = true;
	} else if (BTC_WIFI_PNP_WAKE_UP == pnp_state) {

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], Pnp notify to WAKE UP\n");
		BTC_TRACE(trace_buf);

		halbtc8821c1ant_post_activestate_to_bt(btcoexist, TRUE);
		halbtc8821c1ant_post_onoffstate_to_bt(btcoexist, TRUE);

		btcoexist->stop_coex_dm = false;
	}
}


void ex_halbtc8821c1ant_coex_dm_reset(IN struct btc_coexist *btcoexist)
{
	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		"[BTCoex], *****************Coex DM Reset*****************\n");
	BTC_TRACE(trace_buf);

	halbtc8821c1ant_init_hw_config(btcoexist, false, false);
	halbtc8821c1ant_init_coex_dm(btcoexist);
}

void ex_halbtc8821c1ant_periodical(IN struct btc_coexist *btcoexist)
{

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			    "[BTCoex], ============== Periodical ==============\n");
	BTC_TRACE(trace_buf);

#if (BT_AUTO_REPORT_ONLY_8821C_1ANT == 0)
	halbtc8821c1ant_query_bt_info(btcoexist);

#endif

	halbtc8821c1ant_monitor_bt_ctr(btcoexist);
	halbtc8821c1ant_monitor_wifi_ctr(btcoexist);

	halbtc8821c1ant_monitor_bt_enable_disable(btcoexist);

	/* for 4-way, DHCP, EAPOL packet */
	if (coex_sta->specific_pkt_period_cnt > 0) {

		coex_sta->specific_pkt_period_cnt--;

		if ((coex_sta->specific_pkt_period_cnt == 0) && (coex_sta->wifi_is_high_pri_task))
			coex_sta->wifi_is_high_pri_task = false;

		BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
			"[BTCoex], ***************** Hi-Pri Task = %s*****************\n", (coex_sta->wifi_is_high_pri_task ? "Yes" : "No"));
		BTC_TRACE(trace_buf);
	}

	if (coex_sta->bt_coex_supported_feature == 0)
		coex_sta->bt_coex_supported_feature = btcoexist->btc_get_bt_coex_supported_feature(btcoexist);

	if  ( (coex_sta->bt_coex_supported_version == 0) && (!coex_sta->bt_disabled) )
		coex_sta->bt_coex_supported_version = btcoexist->btc_get_bt_coex_supported_version(btcoexist);

	if (halbtc8821c1ant_is_wifibt_status_changed(btcoexist))
		halbtc8821c1ant_run_coexist_mechanism(btcoexist);


}

void ex_halbtc8821c1ant_antenna_detection(IN struct btc_coexist *btcoexist,
		IN u32 cent_freq, IN u32 offset, IN u32 span, IN u32 seconds)
{
#if BT_8821C_1ANT_ANTDET_ENABLE
	static u32 ant_det_count = 0, ant_det_fail_count = 0;
	struct  btc_board_info	*board_info = &btcoexist->board_info;
	/*boolean scan, roam;*/

	BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
		    "xxxxxxxxxxxxxxxx Call AntennaDetect()!!\n");
	BTC_TRACE(trace_buf);

	if (seconds == 0) {
		psd_scan->ant_det_try_count	= 0;
		psd_scan->ant_det_fail_count	= 0;
		ant_det_count = 0;
		ant_det_fail_count = 0;
		board_info->btdm_ant_det_finish = false;
		board_info->btdm_ant_num_by_ant_det = 1;
		return;
	}

	if (!board_info->btdm_ant_det_finish) {
		psd_scan->ant_det_inteval_count =
			psd_scan->ant_det_inteval_count + 2;

		if (psd_scan->ant_det_inteval_count >=
		    BT_8821C_2ANT_ANTDET_RETRY_INTERVAL) {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna Det Timer is up, Try Detect!!\n");
			BTC_TRACE(trace_buf);
			halbtc8821c1ant_psd_antenna_detection_check(btcoexist);

			if (board_info->btdm_ant_det_finish) {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna Det Success!!\n");
				BTC_TRACE(trace_buf);
#if 1
				if (board_info->btdm_ant_num_by_ant_det == 2)
					halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_WIFI,
						     FORCE_EXEC, 	BT_8821C_1ANT_PHASE_2G_RUNTIME);
				else
					halbtc8821c1ant_set_ant_path(btcoexist, BTC_ANT_PATH_PTA,
						     FORCE_EXEC, BT_8821C_1ANT_PHASE_2G_RUNTIME);
#endif
			} else {
				BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
					"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna Det Fail!!\n");
				BTC_TRACE(trace_buf);
			}
			psd_scan->ant_det_inteval_count = 0;
		} else {
			BTC_SPRINTF(trace_buf, BT_TMP_BUF_SIZE,
				"xxxxxxxxxxxxxxxx AntennaDetect(), Antenna Det Timer is not up! (%d)\n",
				    psd_scan->ant_det_inteval_count);
			BTC_TRACE(trace_buf);
		}

	}
#endif


}


void ex_halbtc8821c1ant_display_ant_detection(IN struct btc_coexist *btcoexist)
{
#if BT_8821C_1ANT_ANTDET_ENABLE
	struct  btc_board_info	*board_info = &btcoexist->board_info;

	if (psd_scan->ant_det_try_count != 0)	{
		halbtc8821c1ant_psd_show_antenna_detect_result(btcoexist);

		if (board_info->btdm_ant_det_finish)
			halbtc8821c1ant_psd_showdata(btcoexist);
		return;
	}
#endif

}

void ex_halbtc8821c1ant_antenna_isolation(IN struct btc_coexist *btcoexist,
		IN u32 cent_freq, IN u32 offset, IN u32 span, IN u32 seconds)
{


}

void ex_halbtc8821c1ant_psd_scan(IN struct btc_coexist *btcoexist,
		 IN u32 cent_freq, IN u32 offset, IN u32 span, IN u32 seconds)
{


}


#endif

#endif	/* #if (BT_SUPPORT == 1 && COEX_SUPPORT == 1) */
