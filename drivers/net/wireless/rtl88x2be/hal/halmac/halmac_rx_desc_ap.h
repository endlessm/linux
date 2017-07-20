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

#ifndef _HALMAC_RX_DESC_AP_H_
#define _HALMAC_RX_DESC_AP_H_
#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 30)
#define GET_RX_DESC_PHYPKTIDC(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 28)
#define GET_RX_DESC_SWDEC(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 27)
#define GET_RX_DESC_PHYST(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 26)
#define GET_RX_DESC_SHIFT(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x3, 24)
#define GET_RX_DESC_QOS(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 23)
#define GET_RX_DESC_SECURITY(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x7, 20)
#define GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0xf, 16)
#define GET_RX_DESC_ICV_ERR(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 15)
#define GET_RX_DESC_CRC32(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x1, 14)
#define GET_RX_DESC_PKT_LEN(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword0, 0x3fff, 0)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 31)
#define GET_RX_DESC_MC(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 30)
#define GET_RX_DESC_TY_PE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x3, 28)
#define GET_RX_DESC_MF(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 27)
#define GET_RX_DESC_MD(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 26)
#define GET_RX_DESC_PWR(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 25)
#define GET_RX_DESC_PAM(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 24)
#define GET_RX_DESC_CHK_VLD(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 23)
#define GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 22)
#define GET_RX_DESC_RX_IPV(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 21)
#define GET_RX_DESC_CHKERR(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 20)
#define GET_RX_DESC_PAGGR(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 15)
#define GET_RX_DESC_RXID_MATCH(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 14)
#define GET_RX_DESC_AMSDU(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 13)
#define GET_RX_DESC_MACID_VLD(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 12)
#define GET_RX_DESC_TID(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0xf, 8)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT)

#define GET_RX_DESC_EXT_SECTYPE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x1, 7)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

#define GET_RX_DESC_MACID(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword1, 0x7f, 0)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0x1, 31)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT)

#define GET_RX_DESC_PPDU_CNT(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0x3, 29)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

#define GET_RX_DESC_C2H(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0x1, 28)
#define GET_RX_DESC_HWRSVD(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0xf, 24)
#define GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0x3f, 18)
#define GET_RX_DESC_RX_IS_QOS(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0x1, 16)
#define GET_RX_DESC_FRAG(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0xf, 12)
#define GET_RX_DESC_SEQ(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword2, 0xfff, 0)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 31)
#define GET_RX_DESC_UNICAST_WAKE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 30)
#define GET_RX_DESC_PATTERN_MATCH(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 29)

#endif

#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT)

#define GET_RX_DESC_RXPAYLOAD_MATCH(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 28)
#define GET_RX_DESC_RXPAYLOAD_ID(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0xf, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

#define GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0xff, 16)
#define GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x3, 12)
#define GET_RX_DESC_EOSP(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 11)
#define GET_RX_DESC_HTC(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x1, 10)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT)

#define GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x7, 7)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

#define GET_RX_DESC_RX_RATE(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword3, 0x7f, 0)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0x1f, 24)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT)

#define GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0x7f, 17)
#define GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0x1, 16)
#define GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0x7f, 9)
#define GET_RX_DESC_RX_EOF(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0x1, 8)

#endif

#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

#define GET_RX_DESC_PATTERN_IDX(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword4, 0xff, 0)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL(__pRxDesc)    HALMAC_GET_DESC_FIELD(((PHALMAC_RX_DESC)__pRxDesc)->Dword5, 0xffffffff, 0)

#endif


#endif

