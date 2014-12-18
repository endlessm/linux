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
#ifndef __RTW_MLME_EXT_H_
#define __RTW_MLME_EXT_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wlan_bssdef.h>


//	Commented by Albert 20101105
//	Increase the SURVEY_TO value from 100 to 150  ( 100ms to 150ms )
//	The Realtek 8188CE SoftAP will spend around 100ms to send the probe response after receiving the probe request.
//	So, this driver tried to extend the dwell time for each scanning channel.
//	This will increase the chance to receive the probe response from SoftAP.

#define SURVEY_TO		(100)
#define REAUTH_TO		(300) //(50)
#define REASSOC_TO		(300) //(50)
//#define DISCONNECT_TO	(3000)
#define ADDBA_TO			(2000)

#define LINKED_TO (1) //unit:2 sec, 1x2=2 sec

#define REAUTH_LIMIT	(4)
#define REASSOC_LIMIT	(4)
#define READDBA_LIMIT	(2)

//#define	IOCMD_REG0		0x10250370		 	
//#define	IOCMD_REG1		0x10250374
//#define	IOCMD_REG2		0x10250378

//#define	FW_DYNAMIC_FUN_SWITCH	0x10250364

//#define	WRITE_BB_CMD		0xF0000001
//#define	SET_CHANNEL_CMD	0xF3000000
//#define	UPDATE_RA_CMD	0xFD0000A2

#define	DYNAMIC_FUNC_DISABLE		(0x0)
#define	DYNAMIC_FUNC_DIG			BIT(0)
#define	DYNAMIC_FUNC_HP			BIT(1)
#define	DYNAMIC_FUNC_SS			BIT(2) //Tx Power Tracking
#define DYNAMIC_FUNC_BT			BIT(3)
#define DYNAMIC_FUNC_ANT_DIV		BIT(4)
#define DYNAMIC_FUNC_ADAPTIVITY	BIT(5)

#define _HW_STATE_NOLINK_		0x00
#define _HW_STATE_ADHOC_		0x01
#define _HW_STATE_STATION_ 	0x02
#define _HW_STATE_AP_			0x03


#define		_1M_RATE_	0
#define		_2M_RATE_	1
#define		_5M_RATE_	2
#define		_11M_RATE_	3
#define		_6M_RATE_	4
#define		_9M_RATE_	5
#define		_12M_RATE_	6
#define		_18M_RATE_	7
#define		_24M_RATE_	8
#define		_36M_RATE_	9
#define		_48M_RATE_	10
#define		_54M_RATE_	11


extern unsigned char RTW_WPA_OUI[];
extern unsigned char WMM_OUI[];
extern unsigned char WPS_OUI[];
extern unsigned char WFD_OUI[];
extern unsigned char P2P_OUI[];

extern unsigned char WMM_INFO_OUI[];
extern unsigned char WMM_PARA_OUI[];


//
// Channel Plan Type.
// Note: 
//	We just add new channel plan when the new channel plan is different from any of the following 
//	channel plan. 
//	If you just wnat to customize the acitions(scan period or join actions) about one of the channel plan,
//	customize them in RT_CHANNEL_INFO in the RT_CHANNEL_LIST.
// 
typedef enum _RT_CHANNEL_DOMAIN
{
	//===== old channel plan mapping =====//
	RT_CHANNEL_DOMAIN_FCC = 0x00,
	RT_CHANNEL_DOMAIN_IC = 0x01,
	RT_CHANNEL_DOMAIN_ETSI = 0x02,
	RT_CHANNEL_DOMAIN_SPAIN = 0x03,
	RT_CHANNEL_DOMAIN_FRANCE = 0x04,
	RT_CHANNEL_DOMAIN_MKK = 0x05,
	RT_CHANNEL_DOMAIN_MKK1 = 0x06,
	RT_CHANNEL_DOMAIN_ISRAEL = 0x07,
	RT_CHANNEL_DOMAIN_TELEC = 0x08,
	RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN = 0x09,
	RT_CHANNEL_DOMAIN_WORLD_WIDE_13 = 0x0A,
	RT_CHANNEL_DOMAIN_TAIWAN = 0x0B,
	RT_CHANNEL_DOMAIN_CHINA = 0x0C,
	RT_CHANNEL_DOMAIN_SINGAPORE_INDIA_MEXICO = 0x0D,
	RT_CHANNEL_DOMAIN_KOREA = 0x0E,
	RT_CHANNEL_DOMAIN_TURKEY = 0x0F,
	RT_CHANNEL_DOMAIN_JAPAN = 0x10,
	RT_CHANNEL_DOMAIN_FCC_NO_DFS = 0x11,
	RT_CHANNEL_DOMAIN_JAPAN_NO_DFS = 0x12,
	RT_CHANNEL_DOMAIN_WORLD_WIDE_5G = 0x13,
	RT_CHANNEL_DOMAIN_TAIWAN_NO_DFS = 0x14,	

	//===== new channel plan mapping, (2GDOMAIN_5GDOMAIN) =====//
	RT_CHANNEL_DOMAIN_WORLD_NULL = 0x20,
	RT_CHANNEL_DOMAIN_ETSI1_NULL = 0x21,
	RT_CHANNEL_DOMAIN_FCC1_NULL = 0x22,
	RT_CHANNEL_DOMAIN_MKK1_NULL = 0x23,
	RT_CHANNEL_DOMAIN_ETSI2_NULL = 0x24,
	RT_CHANNEL_DOMAIN_FCC1_FCC1 = 0x25,
	RT_CHANNEL_DOMAIN_WORLD_ETSI1 = 0x26,
	RT_CHANNEL_DOMAIN_MKK1_MKK1 = 0x27,
	RT_CHANNEL_DOMAIN_WORLD_KCC1 = 0x28,
	RT_CHANNEL_DOMAIN_WORLD_FCC2 = 0x29,
	RT_CHANNEL_DOMAIN_WORLD_FCC3 = 0x30,
	RT_CHANNEL_DOMAIN_WORLD_FCC4 = 0x31,
	RT_CHANNEL_DOMAIN_WORLD_FCC5 = 0x32,
	RT_CHANNEL_DOMAIN_WORLD_FCC6 = 0x33,
	RT_CHANNEL_DOMAIN_FCC1_FCC7 = 0x34,
	RT_CHANNEL_DOMAIN_WORLD_ETSI2 = 0x35,
	RT_CHANNEL_DOMAIN_WORLD_ETSI3 = 0x36,
	RT_CHANNEL_DOMAIN_MKK1_MKK2 = 0x37,
	RT_CHANNEL_DOMAIN_MKK1_MKK3 = 0x38,
	RT_CHANNEL_DOMAIN_FCC1_NCC1 = 0x39,
	RT_CHANNEL_DOMAIN_FCC1_NCC2 = 0x40,

	//===== Add new channel plan above this line===============//
	RT_CHANNEL_DOMAIN_MAX,
	RT_CHANNEL_DOMAIN_REALTEK_DEFINE = 0x7F,
}RT_CHANNEL_DOMAIN, *PRT_CHANNEL_DOMAIN;

typedef enum _RT_CHANNEL_DOMAIN_2G
{
	RT_CHANNEL_DOMAIN_2G_WORLD = 0x00,		//Worldwird 13
	RT_CHANNEL_DOMAIN_2G_ETSI1 = 0x01,		//Europe
	RT_CHANNEL_DOMAIN_2G_FCC1 = 0x02,		//US
	RT_CHANNEL_DOMAIN_2G_MKK1 = 0x03,		//Japan
	RT_CHANNEL_DOMAIN_2G_ETSI2 = 0x04,		//France
	RT_CHANNEL_DOMAIN_2G_NULL = 0x05,
	//===== Add new channel plan above this line===============//
	RT_CHANNEL_DOMAIN_2G_MAX,
}RT_CHANNEL_DOMAIN_2G, *PRT_CHANNEL_DOMAIN_2G;

typedef enum _RT_CHANNEL_DOMAIN_5G
{
	RT_CHANNEL_DOMAIN_5G_NULL = 0x00,
	RT_CHANNEL_DOMAIN_5G_ETSI1 = 0x01,		//Europe
	RT_CHANNEL_DOMAIN_5G_ETSI2 = 0x02,		//Australia, New Zealand
	RT_CHANNEL_DOMAIN_5G_ETSI3 = 0x03,		//Russia
	RT_CHANNEL_DOMAIN_5G_FCC1 = 0x04,		//US
	RT_CHANNEL_DOMAIN_5G_FCC2 = 0x05,		//FCC o/w DFS Channels
	RT_CHANNEL_DOMAIN_5G_FCC3 = 0x06,		//India, Mexico
	RT_CHANNEL_DOMAIN_5G_FCC4 = 0x07,		//Venezuela
	RT_CHANNEL_DOMAIN_5G_FCC5 = 0x08,		//China
	RT_CHANNEL_DOMAIN_5G_FCC6 = 0x09,		//Israel
	RT_CHANNEL_DOMAIN_5G_FCC7_IC1 = 0x0A,	//US, Canada
	RT_CHANNEL_DOMAIN_5G_KCC1 = 0x0B,		//Korea
	RT_CHANNEL_DOMAIN_5G_MKK1 = 0x0C,		//Japan
	RT_CHANNEL_DOMAIN_5G_MKK2 = 0x0D,		//Japan (W52, W53)
	RT_CHANNEL_DOMAIN_5G_MKK3 = 0x0E,		//Japan (W56)
	RT_CHANNEL_DOMAIN_5G_NCC1 = 0x0F,		//Taiwan
	RT_CHANNEL_DOMAIN_5G_NCC2 = 0x10,		//Taiwan o/w DFS
	//===== Add new channel plan above this line===============//
	//===== Driver Self Defined =====//
	RT_CHANNEL_DOMAIN_5G_FCC = 0x11,
	RT_CHANNEL_DOMAIN_5G_JAPAN_NO_DFS = 0x12,
	RT_CHANNEL_DOMAIN_5G_FCC4_NO_DFS = 0x13,
	RT_CHANNEL_DOMAIN_5G_MAX,
}RT_CHANNEL_DOMAIN_5G, *PRT_CHANNEL_DOMAIN_5G;

#define rtw_is_channel_plan_valid(chplan) (chplan<RT_CHANNEL_DOMAIN_MAX || chplan == RT_CHANNEL_DOMAIN_REALTEK_DEFINE)

typedef struct _RT_CHANNEL_PLAN
{
	unsigned char	Channel[MAX_CHANNEL_NUM];
	unsigned char	Len;
}RT_CHANNEL_PLAN, *PRT_CHANNEL_PLAN;

typedef struct _RT_CHANNEL_PLAN_2G
{
	unsigned char	Channel[MAX_CHANNEL_NUM_2G];
	unsigned char	Len;
}RT_CHANNEL_PLAN_2G, *PRT_CHANNEL_PLAN_2G;

typedef struct _RT_CHANNEL_PLAN_5G
{
	unsigned char	Channel[MAX_CHANNEL_NUM_5G];
	unsigned char	Len;
}RT_CHANNEL_PLAN_5G, *PRT_CHANNEL_PLAN_5G;

typedef struct _RT_CHANNEL_PLAN_MAP
{
	unsigned char	Index2G;
	unsigned char	Index5G;
}RT_CHANNEL_PLAN_MAP, *PRT_CHANNEL_PLAN_MAP;

enum Associated_AP
{
	atherosAP	= 0,
	broadcomAP	= 1,
	ciscoAP		= 2,
	marvellAP	= 3,
	ralinkAP	= 4,
	realtekAP	= 5,
	airgocapAP 	= 6,
	unknownAP	= 7,
	maxAP,
};

enum SCAN_STATE
{
	SCAN_DISABLE = 0,
	SCAN_START = 1,
	SCAN_TXNULL = 2,
	SCAN_PROCESS = 3,
	SCAN_COMPLETE = 4,
	SCAN_STATE_MAX,
};

struct mlme_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(_adapter *padapter, union recv_frame *precv_frame);
};

struct action_handler {
	unsigned int   num;
	char* str;
	unsigned int (*func)(_adapter *padapter, union recv_frame *precv_frame);
};

struct	ss_res	
{
	int	state;
	int	bss_cnt;
	int	channel_idx;
	int	scan_mode;
	u8 ssid_num;
	u8 ch_num;
	NDIS_802_11_SSID ssid[RTW_SSID_SCAN_AMOUNT];
	struct rtw_ieee80211_channel ch[RTW_CHANNEL_SCAN_AMOUNT];
};

//#define AP_MODE				0x0C
//#define STATION_MODE	0x08
//#define AD_HOC_MODE		0x04
//#define NO_LINK_MODE	0x00

#define 	WIFI_FW_NULL_STATE			_HW_STATE_NOLINK_
#define	WIFI_FW_STATION_STATE		_HW_STATE_STATION_
#define	WIFI_FW_AP_STATE				_HW_STATE_AP_
#define	WIFI_FW_ADHOC_STATE			_HW_STATE_ADHOC_

#define	WIFI_FW_AUTH_NULL			0x00000100
#define	WIFI_FW_AUTH_STATE			0x00000200
#define	WIFI_FW_AUTH_SUCCESS			0x00000400

#define	WIFI_FW_ASSOC_STATE			0x00002000
#define	WIFI_FW_ASSOC_SUCCESS		0x00004000

#define	WIFI_FW_LINKING_STATE		(WIFI_FW_AUTH_NULL | WIFI_FW_AUTH_STATE | WIFI_FW_AUTH_SUCCESS |WIFI_FW_ASSOC_STATE)

#ifdef CONFIG_TDLS
// 1: Write RCR DATA BIT
// 2: Issue peer traffic indication
// 3: Go back to the channel linked with AP, terminating channel switch procedure
// 4: Init channel sensing, receive all data and mgnt frame
// 5: Channel sensing and report candidate channel
// 6: First time set channel to off channel
// 7: Go back tp the channel linked with AP when set base channel as target channel
// 8: Set channel back to base channel
// 9: Set channel back to off channel
// 10: Restore RCR DATA BIT
// 11: Check alive
// 12: Check alive
// 13: Free TDLS sta
enum TDLS_option
{
	TDLS_WRCR			= 	1,
	TDLS_SD_PTI		= 	2,
	TDLS_CS_OFF		= 	3,
	TDLS_INIT_CH_SEN	= 	4,
	TDLS_DONE_CH_SEN	=	5,
	TDLS_OFF_CH		=	6,
	TDLS_BASE_CH 		=	7,
	TDLS_P_OFF_CH		=	8,
	TDLS_P_BASE_CH	= 	9,
	TDLS_RS_RCR		=	10,
	TDLS_CKALV_PH1	=	11,
	TDLS_CKALV_PH2	=	12,
	TDLS_FREE_STA		=	13,
	maxTDLS,
};

#endif //CONFIG_TDLS

struct FW_Sta_Info
{
	struct sta_info	*psta;
	u32	status;
	u32	rx_pkt;
	u32	retry;
	NDIS_802_11_RATES_EX  SupportedRates;
};

/*
 * Usage:
 * When one iface acted as AP mode and the other iface is STA mode and scanning, 
 * it should switch back to AP's operating channel periodically.
 * Parameters info:
 * When the driver scanned RTW_SCAN_NUM_OF_CH channels, it would switch back to AP's operating channel for
 * RTW_STAY_AP_CH_MILLISECOND * SURVEY_TO milliseconds.
 * Example:
 * For chip supports 2.4G + 5GHz and AP mode is operating in channel 1, 
 * RTW_SCAN_NUM_OF_CH is 8, RTW_STAY_AP_CH_MILLISECOND is 3 and SURVEY_TO is 100.
 * When it's STA mode gets set_scan command, 
 * it would 
 * 1. Doing the scan on channel 1.2.3.4.5.6.7.8 
 * 2. Back to channel 1 for 300 milliseconds
 * 3. Go through doing site survey on channel 9.10.11.36.40.44.48.52
 * 4. Back to channel 1 for 300 milliseconds
 * 5. ... and so on, till survey done.
 */
#if defined CONFIG_STA_MODE_SCAN_UNDER_AP_MODE && defined CONFIG_CONCURRENT_MODE
#define RTW_SCAN_NUM_OF_CH			8
#define RTW_STAY_AP_CH_MILLISECOND	3	// this value is a multiplier,for example, when this value is 3, it would stay AP's op ch for 
											// 3 * SURVEY_TO millisecond. 
#endif //defined CONFIG_STA_MODE_SCAN_UNDER_AP_MODE && defined CONFIG_CONCURRENT_MODE

struct mlme_ext_info
{
	u32	state;
	u32	reauth_count;
	u32	reassoc_count;
	u32	link_count;
	u32	auth_seq;
	u32	auth_algo;	// 802.11 auth, could be open, shared, auto
	u32	authModeToggle;
	u32	enc_algo;//encrypt algorithm;
	u32	key_index;	// this is only valid for legendary wep, 0~3 for key id.
	u32	iv;
	u8	chg_txt[128];
	u16	aid;
	u16	bcn_interval;
	u16	capability;
	u8	assoc_AP_vendor;
	u8	slotTime;
	u8	preamble_mode;
	u8	WMM_enable;
	u8	ERP_enable;
	u8	ERP_IE;
	u8	HT_enable;
	u8	HT_caps_enable;	
	u8	HT_info_enable;
	u8	HT_protection;
	u8	turboMode_cts2self;
	u8	turboMode_rtsen;
	u8	SM_PS;
	u8	agg_enable_bitmap;
	u8	ADDBA_retry_count;
	u8	candidate_tid_bitmap;
	u8	dialogToken;
	// Accept ADDBA Request
	BOOLEAN bAcceptAddbaReq;
	u8	bwmode_updated;
	u8	hidden_ssid_mode;

	struct ADDBA_request		ADDBA_req;
	struct WMM_para_element	WMM_param;
	struct HT_caps_element	HT_caps;
	struct HT_info_element		HT_info;
	WLAN_BSSID_EX			network;//join network or bss_network, if in ap mode, it is the same to cur_network.network
	struct FW_Sta_Info		FW_sta_info[NUM_STA];

#ifdef CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
	u8 scan_cnt;
#endif //CONFIG_STA_MODE_SCAN_UNDER_AP_MODE
};

// The channel information about this channel including joining, scanning, and power constraints.
typedef struct _RT_CHANNEL_INFO
{
	u8				ChannelNum;		// The channel number.
	RT_SCAN_TYPE	ScanType;		// Scan type such as passive or active scan.
	//u16				ScanPeriod;		// Listen time in millisecond in this channel.
	//s32				MaxTxPwrDbm;	// Max allowed tx power.
	//u32				ExInfo;			// Extended Information for this channel.
#ifdef CONFIG_FIND_BEST_CHANNEL
	u32				rx_count;
#endif
}RT_CHANNEL_INFO, *PRT_CHANNEL_INFO;

int rtw_ch_set_search_ch(RT_CHANNEL_INFO *ch_set, const u32 ch);
bool rtw_mlme_band_check(_adapter *adapter, const u32 ch);

// P2P_MAX_REG_CLASSES - Maximum number of regulatory classes
#define P2P_MAX_REG_CLASSES 10

// P2P_MAX_REG_CLASS_CHANNELS - Maximum number of channels per regulatory class
#define P2P_MAX_REG_CLASS_CHANNELS 20

//  struct p2p_channels - List of supported channels
struct p2p_channels {
	// struct p2p_reg_class - Supported regulatory class
	struct p2p_reg_class {
		// reg_class - Regulatory class (IEEE 802.11-2007, Annex J)
		u8 reg_class;

		// channel - Supported channels
		u8 channel[P2P_MAX_REG_CLASS_CHANNELS];

		// channels - Number of channel entries in use
		size_t channels;
	} reg_class[P2P_MAX_REG_CLASSES];

	// reg_classes - Number of reg_class entries in use
	size_t reg_classes;
};

struct p2p_oper_class_map {
	enum hw_mode {IEEE80211G,IEEE80211A} mode;
	u8 op_class;
	u8 min_chan;
	u8 max_chan;
	u8 inc;
	enum { BW20, BW40PLUS, BW40MINUS } bw;
};

struct mlme_ext_priv
{
	_adapter	*padapter;
	u8	mlmeext_init;
	ATOMIC_T		event_seq;
	u16	mgnt_seq;
#ifdef CONFIG_IEEE80211W
	u16	sa_query_seq;
	u64 mgnt_80211w_IPN;
	u64 mgnt_80211w_IPN_rx;
#endif //CONFIG_IEEE80211W
	//struct fw_priv 	fwpriv;
	
	unsigned char	cur_channel;
	unsigned char	cur_bwmode;
	unsigned char	cur_ch_offset;//PRIME_CHNL_OFFSET
	unsigned char	cur_wireless_mode;	

	unsigned char	max_chan_nums;
	RT_CHANNEL_INFO		channel_set[MAX_CHANNEL_NUM];
	struct p2p_channels channel_list;
	unsigned char	basicrate[NumRates];
	unsigned char	datarate[NumRates];
	
	struct ss_res		sitesurvey_res;		
	struct mlme_ext_info	mlmext_info;//for sta/adhoc mode, including current scanning/connecting/connected related info.
                                                     //for ap mode, network includes ap's cap_info
	_timer		survey_timer;
	_timer		link_timer;
#ifdef CONFIG_IEEE80211W
	_timer		sa_query_timer;
#endif //CONFIG_IEEE80211W
	//_timer		ADDBA_timer;
	u16			chan_scan_time;

	u8	scan_abort;
	u8	tx_rate; // TXRATE when USERATE is set.

	u32	retry; //retry for issue probereq
	
	u64 TSFValue;

#ifdef CONFIG_AP_MODE	
	unsigned char bstart_bss;
#endif

	//recv_decache check for Action_public frame
        u8 action_public_dialog_token;
	u16 	 action_public_rxseq;

#ifdef CONFIG_80211D
	u8 update_channel_plan_by_ap_done;
#endif

#ifdef CONFIG_ACTIVE_KEEP_ALIVE_CHECK
	u8 active_keep_alive_check;
#endif
	
};

int init_mlme_ext_priv(_adapter* padapter);
int init_hw_mlme_ext(_adapter *padapter);
void free_mlme_ext_priv (struct mlme_ext_priv *pmlmeext);
extern void init_mlme_ext_timer(_adapter *padapter);
extern void init_addba_retry_timer(_adapter *padapter, struct sta_info *psta);
extern struct xmit_frame *alloc_mgtxmitframe(struct xmit_priv *pxmitpriv);
struct xmit_frame *alloc_mgtxmitframe_once(struct xmit_priv *pxmitpriv);

//void fill_fwpriv(_adapter * padapter, struct fw_priv *pfwpriv);

unsigned char networktype_to_raid(unsigned char network_type);
int judge_network_type(_adapter *padapter, unsigned char *rate, int ratelen);
void get_rate_set(_adapter *padapter, unsigned char *pbssrate, int *bssrate_len);
void UpdateBrateTbl(_adapter *padapter,u8 *mBratesOS);
void UpdateBrateTblForSoftAP(u8 *bssrateset, u32 bssratelen);
void change_band_update_ie(_adapter *padapter, WLAN_BSSID_EX *pnetwork);

void Save_DM_Func_Flag(_adapter *padapter);
void Restore_DM_Func_Flag(_adapter *padapter);
void Switch_DM_Func(_adapter *padapter, u8 mode, u8 enable);

//void Set_NETYPE1_MSR(_adapter *padapter, u8 type);
//void Set_NETYPE0_MSR(_adapter *padapter, u8 type);
void Set_MSR(_adapter *padapter, u8 type);

u8 rtw_get_oper_ch(_adapter *adapter);
void rtw_set_oper_ch(_adapter *adapter, u8 ch);
u8 rtw_get_oper_bw(_adapter *adapter);
void rtw_set_oper_bw(_adapter *adapter, u8 bw);
u8 rtw_get_oper_choffset(_adapter *adapter);
void rtw_set_oper_choffset(_adapter *adapter, u8 offset);
u32 rtw_get_on_oper_ch_time(_adapter *adapter);
u32 rtw_get_on_cur_ch_time(_adapter *adapter);

void set_channel_bwmode(_adapter *padapter, unsigned char channel, unsigned char channel_offset, unsigned short bwmode);
void SelectChannel(_adapter *padapter, unsigned char channel);
void SetBWMode(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset);

unsigned int decide_wait_for_beacon_timeout(unsigned int bcn_interval);

void write_cam(_adapter *padapter, u8 entry, u16 ctrl, u8 *mac, u8 *key);
void clear_cam_entry(_adapter *padapter, u8 entry);

void invalidate_cam_all(_adapter *padapter);
void CAM_empty_entry(PADAPTER Adapter, u8 ucIndex);


int allocate_fw_sta_entry(_adapter *padapter);
void flush_all_cam_entry(_adapter *padapter);

BOOLEAN IsLegal5GChannel(PADAPTER Adapter, u8 channel);

void site_survey(_adapter *padapter);
u8 collect_bss_info(_adapter *padapter, union recv_frame *precv_frame, WLAN_BSSID_EX *bssid);

int get_bsstype(unsigned short capability);
u8* get_my_bssid(WLAN_BSSID_EX *pnetwork);
u16 get_beacon_interval(WLAN_BSSID_EX *bss);

int is_client_associated_to_ap(_adapter *padapter);
int is_client_associated_to_ibss(_adapter *padapter);
int is_IBSS_empty(_adapter *padapter);

unsigned char check_assoc_AP(u8 *pframe, uint len);

int WMM_param_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs	pIE);
#ifdef CONFIG_WFD
int WFD_info_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs	pIE);
#endif
void WMMOnAssocRsp(_adapter *padapter);

void HT_caps_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void HT_info_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void HTOnAssocRsp(_adapter *padapter);

void ERP_IE_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE);
void VCS_update(_adapter *padapter, struct sta_info *psta);

void update_beacon_info(_adapter *padapter, u8 *pframe, uint len, struct sta_info *psta);
#ifdef CONFIG_DFS
void process_csa_ie(_adapter *padapter, u8 *pframe, uint len);
#endif //CONFIG_DFS
void update_IOT_info(_adapter *padapter);
void update_capinfo(PADAPTER Adapter, u16 updateCap);
void update_wireless_mode(_adapter * padapter);
void update_tx_basic_rate(_adapter *padapter, u8 modulation);
void update_bmc_sta_support_rate(_adapter *padapter, u32 mac_id);
int update_sta_support_rate(_adapter *padapter, u8* pvar_ie, uint var_ie_len, int cam_idx);

//for sta/adhoc mode
void update_sta_info(_adapter *padapter, struct sta_info *psta);
unsigned int update_basic_rate(unsigned char *ptn, unsigned int ptn_sz);
unsigned int update_supported_rate(unsigned char *ptn, unsigned int ptn_sz);
unsigned int update_MSC_rate(struct HT_caps_element *pHT_caps);
void Update_RA_Entry(_adapter *padapter, u32 mac_id);
void set_sta_rate(_adapter *padapter, struct sta_info *psta);

unsigned int receive_disconnect(_adapter *padapter, unsigned char *MacAddr, unsigned short reason);

unsigned char get_highest_rate_idx(u32 mask);
int support_short_GI(_adapter *padapter, struct HT_caps_element *pHT_caps);
unsigned int is_ap_in_tkip(_adapter *padapter);


void report_join_res(_adapter *padapter, int res);
void report_survey_event(_adapter *padapter, union recv_frame *precv_frame);
void report_surveydone_event(_adapter *padapter);
void report_del_sta_event(_adapter *padapter, unsigned char* MacAddr, unsigned short reason);
void report_add_sta_event(_adapter *padapter, unsigned char* MacAddr, int cam_idx);

void beacon_timing_control(_adapter *padapter);
extern u8 set_tx_beacon_cmd(_adapter*padapter);
unsigned int setup_beacon_frame(_adapter *padapter, unsigned char *beacon_frame);
void update_mgnt_tx_rate(_adapter *padapter, u8 rate);
void update_mgntframe_attrib(_adapter *padapter, struct pkt_attrib *pattrib);
void dump_mgntframe(_adapter *padapter, struct xmit_frame *pmgntframe);
s32 dump_mgntframe_and_wait(_adapter *padapter, struct xmit_frame *pmgntframe, int timeout_ms);
s32 dump_mgntframe_and_wait_ack(_adapter *padapter, struct xmit_frame *pmgntframe);

#ifdef CONFIG_P2P
void issue_probersp_p2p(_adapter *padapter, unsigned char *da);
void issue_p2p_provision_request( _adapter *padapter, u8* pssid, u8 ussidlen, u8* pdev_raddr);
void issue_p2p_GO_request(_adapter *padapter, u8* raddr);
void issue_probereq_p2p(_adapter *padapter, u8 *da);
int issue_probereq_p2p_ex(_adapter *adapter, u8 *da, int try_cnt, int wait_ms);
void issue_p2p_invitation_response(_adapter *padapter, u8* raddr, u8 dialogToken, u8 success);
void issue_p2p_invitation_request(_adapter *padapter, u8* raddr );
#endif //CONFIG_P2P
void issue_beacon(_adapter *padapter);
void issue_probersp(_adapter *padapter, unsigned char *da, u8 is_valid_p2p_probereq);
void issue_assocreq(_adapter *padapter);
void issue_asocrsp(_adapter *padapter, unsigned short status, struct sta_info *pstat, int pkt_type);
void issue_auth(_adapter *padapter, struct sta_info *psta, unsigned short status);
void issue_probereq(_adapter *padapter, NDIS_802_11_SSID *pssid, u8 *da);
s32 issue_probereq_ex(_adapter *padapter, NDIS_802_11_SSID *pssid, u8* da, int try_cnt, int wait_ms);
int issue_nulldata(_adapter *padapter, unsigned char *da, unsigned int power_mode, int try_cnt, int wait_ms);
int issue_qos_nulldata(_adapter *padapter, unsigned char *da, u16 tid, int try_cnt, int wait_ms);
int issue_deauth(_adapter *padapter, unsigned char *da, unsigned short reason);
int issue_deauth_ex(_adapter *padapter, u8 *da, unsigned short reason, int try_cnt, int wait_ms);
void issue_action_spct_ch_switch(_adapter *padapter, u8 *ra, u8 new_ch, u8 ch_offset);
void issue_action_BA(_adapter *padapter, unsigned char *raddr, unsigned char action, unsigned short status);
#ifdef CONFIG_IEEE80211W
void issue_action_SA_Query(_adapter *padapter, unsigned char *raddr, unsigned char action, unsigned short tid);
#endif //CONFIG_IEEE80211W
unsigned int send_delba(_adapter *padapter, u8 initiator, u8 *addr);
unsigned int send_beacon(_adapter *padapter);

void start_clnt_assoc(_adapter *padapter);
void start_clnt_auth(_adapter* padapter);
void start_clnt_join(_adapter* padapter);
void start_create_ibss(_adapter* padapter);

unsigned int OnAssocReq(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAssocRsp(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnProbeReq(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnProbeRsp(_adapter *padapter, union recv_frame *precv_frame);
unsigned int DoReserved(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnBeacon(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAtim(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnDisassoc(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAuth(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAuthClient(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnDeAuth(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction(_adapter *padapter, union recv_frame *precv_frame);

unsigned int on_action_spct(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_qos(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_dls(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_back(_adapter *padapter, union recv_frame *precv_frame);
unsigned int on_action_public(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_ht(_adapter *padapter, union recv_frame *precv_frame);
#ifdef CONFIG_IEEE80211W
unsigned int OnAction_sa_query(_adapter *padapter, union recv_frame *precv_frame);
#endif //CONFIG_IEEE80211W
unsigned int OnAction_wmm(_adapter *padapter, union recv_frame *precv_frame);
unsigned int OnAction_p2p(_adapter *padapter, union recv_frame *precv_frame);


void mlmeext_joinbss_event_callback(_adapter *padapter, int join_res);
void mlmeext_sta_del_event_callback(_adapter *padapter);
void mlmeext_sta_add_event_callback(_adapter *padapter, struct sta_info *psta);

void linked_status_chk(_adapter *padapter);

void survey_timer_hdl (_adapter *padapter);
void link_timer_hdl (_adapter *padapter);
void addba_timer_hdl(struct sta_info *psta);
#ifdef CONFIG_IEEE80211W
void sa_query_timer_hdl(_adapter *padapter);
#endif //CONFIG_IEEE80211W
//void reauth_timer_hdl(_adapter *padapter);
//void reassoc_timer_hdl(_adapter *padapter);

#define set_survey_timer(mlmeext, ms) \
	do { \
		/*DBG_871X("%s set_survey_timer(%p, %d)\n", __FUNCTION__, (mlmeext), (ms));*/ \
		_set_timer(&(mlmeext)->survey_timer, (ms)); \
	} while(0)

#define set_link_timer(mlmeext, ms) \
	do { \
		/*DBG_871X("%s set_link_timer(%p, %d)\n", __FUNCTION__, (mlmeext), (ms));*/ \
		_set_timer(&(mlmeext)->link_timer, (ms)); \
	} while(0)
#ifdef CONFIG_IEEE80211W
#define set_sa_query_timer(mlmeext, ms) \
	do { \
		DBG_871X("%s set_sa_query_timer(%p, %d)\n", __FUNCTION__, (mlmeext), (ms)); \
		_set_timer(&(mlmeext)->sa_query_timer, (ms)); \
	} while(0)
#endif //CONFIG_IEEE80211W
extern int cckrates_included(unsigned char *rate, int ratelen);
extern int cckratesonly_included(unsigned char *rate, int ratelen);

extern void process_addba_req(_adapter *padapter, u8 *paddba_req, u8 *addr);

extern void update_TSF(struct mlme_ext_priv *pmlmeext, u8 *pframe, uint len);
extern void correct_TSF(_adapter *padapter, struct mlme_ext_priv *pmlmeext);


#ifdef CONFIG_CONCURRENT_MODE
 sint check_buddy_mlmeinfo_state(_adapter *padapter, u32 state);
void concurrent_chk_joinbss_done(_adapter *padapter, int join_res);
#endif //CONFIG_CONCURRENT_MODE

#ifdef CONFIG_DUALMAC_CONCURRENT
void	dc_SelectChannel(_adapter *padapter, unsigned char channel);
void	dc_SetBWMode(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset);
void	dc_set_channel_bwmode_disconnect(_adapter *padapter);
u8	dc_handle_join_request(_adapter *padapter);
void	dc_handle_join_done(_adapter *padapter, u8 join_res);
sint	dc_check_fwstate(_adapter *padapter, sint fw_state);
u8	dc_handle_site_survey(_adapter *padapter);
void	dc_report_survey_event(_adapter *padapter, union recv_frame *precv_frame);
void	dc_set_channel_bwmode_survey_done(_adapter *padapter);
void	dc_set_ap_channel_bandwidth(_adapter *padapter, u8 channel, u8 channel_offset, u8 bwmode);
void	dc_resume_xmit(_adapter *padapter);
u8	dc_check_xmit(_adapter *padapter);
#endif

int rtw_chk_start_clnt_join(_adapter *padapter);
int rtw_get_ch_setting_union(_adapter *adapter, u8 *ch, u8 *bw, u8 *offset);

struct cmd_hdl {
	uint	parmsize;
	u8 (*h2cfuns)(struct _ADAPTER *padapter, u8 *pbuf);	
};


u8 read_macreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_macreg_hdl(_adapter *padapter, u8 *pbuf);
u8 read_bbreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_bbreg_hdl(_adapter *padapter, u8 *pbuf);
u8 read_rfreg_hdl(_adapter *padapter, u8 *pbuf);
u8 write_rfreg_hdl(_adapter *padapter, u8 *pbuf);


u8 NULL_hdl(_adapter *padapter, u8 *pbuf);
u8 join_cmd_hdl(_adapter *padapter, u8 *pbuf);
u8 disconnect_hdl(_adapter *padapter, u8 *pbuf);
u8 createbss_hdl(_adapter *padapter, u8 *pbuf);
u8 setopmode_hdl(_adapter *padapter, u8 *pbuf);
u8 sitesurvey_cmd_hdl(_adapter *padapter, u8 *pbuf);	
u8 setauth_hdl(_adapter *padapter, u8 *pbuf);
u8 setkey_hdl(_adapter *padapter, u8 *pbuf);
u8 set_stakey_hdl(_adapter *padapter, u8 *pbuf);
u8 set_assocsta_hdl(_adapter *padapter, u8 *pbuf);
u8 del_assocsta_hdl(_adapter *padapter, u8 *pbuf);
u8 add_ba_hdl(_adapter *padapter, unsigned char *pbuf);

u8 mlme_evt_hdl(_adapter *padapter, unsigned char *pbuf);
u8 h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf);
u8 tx_beacon_hdl(_adapter *padapter, unsigned char *pbuf);
u8 set_ch_hdl(_adapter *padapter, u8 *pbuf);
u8 set_chplan_hdl(_adapter *padapter, unsigned char *pbuf);
u8 led_blink_hdl(_adapter *padapter, unsigned char *pbuf);
u8 set_csa_hdl(_adapter *padapter, unsigned char *pbuf);	//Kurt: Handling DFS channel switch announcement ie.
u8 tdls_hdl(_adapter *padapter, unsigned char *pbuf);


#define GEN_DRV_CMD_HANDLER(size, cmd)	{size, &cmd ## _hdl},
#define GEN_MLME_EXT_HANDLER(size, cmd)	{size, cmd},

#ifdef _RTW_CMD_C_

struct cmd_hdl wlancmds[] = 
{
	GEN_DRV_CMD_HANDLER(0, NULL) /*0*/
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_DRV_CMD_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) /*10*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)		
	GEN_MLME_EXT_HANDLER(sizeof (struct joinbss_parm), join_cmd_hdl) /*14*/
	GEN_MLME_EXT_HANDLER(sizeof (struct disconnect_parm), disconnect_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct createbss_parm), createbss_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct setopmode_parm), setopmode_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct sitesurvey_parm), sitesurvey_cmd_hdl) /*18*/
	GEN_MLME_EXT_HANDLER(sizeof (struct setauth_parm), setauth_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct setkey_parm), setkey_hdl) /*20*/
	GEN_MLME_EXT_HANDLER(sizeof (struct set_stakey_parm), set_stakey_hdl)
	GEN_MLME_EXT_HANDLER(sizeof (struct set_assocsta_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct del_assocsta_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setstapwrstate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setbasicrate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getbasicrate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setdatarate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getdatarate_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct setphyinfo_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getphyinfo_parm), NULL)  /*30*/
	GEN_MLME_EXT_HANDLER(sizeof (struct setphy_parm), NULL)
	GEN_MLME_EXT_HANDLER(sizeof (struct getphy_parm), NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)	/*40*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(sizeof(struct addBaReq_parm), add_ba_hdl)	
	GEN_MLME_EXT_HANDLER(sizeof(struct set_ch_parm), set_ch_hdl) /* 46 */
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) /*50*/
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL)
	GEN_MLME_EXT_HANDLER(0, NULL) 
	GEN_MLME_EXT_HANDLER(sizeof(struct Tx_Beacon_param), tx_beacon_hdl) /*55*/

	GEN_MLME_EXT_HANDLER(0, mlme_evt_hdl) /*56*/
	GEN_MLME_EXT_HANDLER(0, rtw_drvextra_cmd_hdl) /*57*/

	GEN_MLME_EXT_HANDLER(0, h2c_msg_hdl) /*58*/
	GEN_MLME_EXT_HANDLER(sizeof(struct SetChannelPlan_param), set_chplan_hdl) /*59*/
	GEN_MLME_EXT_HANDLER(sizeof(struct LedBlink_param), led_blink_hdl) /*60*/
	GEN_MLME_EXT_HANDLER(sizeof(struct SetChannelSwitch_param), set_csa_hdl) /*61*/
	GEN_MLME_EXT_HANDLER(sizeof(struct TDLSoption_param), tdls_hdl) /*62*/
};

#endif

struct C2HEvent_Header
{

#ifdef CONFIG_LITTLE_ENDIAN

	unsigned int len:16;
	unsigned int ID:8;
	unsigned int seq:8;
	
#elif defined(CONFIG_BIG_ENDIAN)

	unsigned int seq:8;
	unsigned int ID:8;
	unsigned int len:16;
	
#else

#  error "Must be LITTLE or BIG Endian"

#endif

	unsigned int rsvd;

};

void rtw_dummy_event_callback(_adapter *adapter , u8 *pbuf);
void rtw_fwdbg_event_callback(_adapter *adapter , u8 *pbuf);

enum rtw_c2h_event
{
	GEN_EVT_CODE(_Read_MACREG)=0, /*0*/
	GEN_EVT_CODE(_Read_BBREG),
 	GEN_EVT_CODE(_Read_RFREG),
 	GEN_EVT_CODE(_Read_EEPROM),
 	GEN_EVT_CODE(_Read_EFUSE),
	GEN_EVT_CODE(_Read_CAM),			/*5*/
 	GEN_EVT_CODE(_Get_BasicRate),  
 	GEN_EVT_CODE(_Get_DataRate),   
 	GEN_EVT_CODE(_Survey),	 /*8*/
 	GEN_EVT_CODE(_SurveyDone),	 /*9*/
 	
 	GEN_EVT_CODE(_JoinBss) , /*10*/
 	GEN_EVT_CODE(_AddSTA),
 	GEN_EVT_CODE(_DelSTA),
 	GEN_EVT_CODE(_AtimDone) ,
 	GEN_EVT_CODE(_TX_Report),  
	GEN_EVT_CODE(_CCX_Report),			/*15*/
 	GEN_EVT_CODE(_DTM_Report),
 	GEN_EVT_CODE(_TX_Rate_Statistics),
 	GEN_EVT_CODE(_C2HLBK), 
 	GEN_EVT_CODE(_FWDBG),
	GEN_EVT_CODE(_C2HFEEDBACK),               /*20*/
	GEN_EVT_CODE(_ADDBA),
	GEN_EVT_CODE(_C2HBCN),
	GEN_EVT_CODE(_ReportPwrState),		//filen: only for PCIE, USB	
	GEN_EVT_CODE(_CloseRF),				//filen: only for PCIE, work around ASPM
 	MAX_C2HEVT
};


#ifdef _RTW_MLME_EXT_C_		

static struct fwevent wlanevents[] = 
{
	{0, rtw_dummy_event_callback}, 	/*0*/
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, &rtw_survey_event_callback},		/*8*/
	{sizeof (struct surveydone_event), &rtw_surveydone_event_callback},	/*9*/
		
	{0, &rtw_joinbss_event_callback},		/*10*/
	{sizeof(struct stassoc_event), &rtw_stassoc_event_callback},
	{sizeof(struct stadel_event), &rtw_stadel_event_callback},	
	{0, &rtw_atimdone_event_callback},
	{0, rtw_dummy_event_callback},
	{0, NULL},	/*15*/
	{0, NULL},
	{0, NULL},
	{0, NULL},
	{0, rtw_fwdbg_event_callback},
	{0, NULL},	 /*20*/
	{0, NULL},
	{0, NULL},	
	{0, &rtw_cpwm_event_callback},
};

#endif//_RTL8192C_CMD_C_

#endif

