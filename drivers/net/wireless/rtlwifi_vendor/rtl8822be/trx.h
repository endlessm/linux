/******************************************************************************
 *
 * Copyright(c) 2016  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#ifndef __RTL8822B_TRX_H__
#define __RTL8822B_TRX_H__

#include "../halmac/halmac_tx_desc_nic.h"
#include "../halmac/halmac_rx_desc_nic.h"

#if (DMA_IS_64BIT == 1)
#if (RTL8822BE_SEG_NUM == 2)
#define TX_BD_DESC_SIZE	128
#elif (RTL8822BE_SEG_NUM == 1)
#define TX_BD_DESC_SIZE	64
#elif (RTL8822BE_SEG_NUM == 0)
#define TX_BD_DESC_SIZE	32
#endif
#else
#if (RTL8822BE_SEG_NUM == 2)
#define TX_BD_DESC_SIZE	64
#elif (RTL8822BE_SEG_NUM == 1)
#define TX_BD_DESC_SIZE	32
#elif (RTL8822BE_SEG_NUM == 0)
#define TX_BD_DESC_SIZE	16
#endif
#endif

#define TX_DESC_SIZE	64

#define RX_DRV_INFO_SIZE_UNIT	8

#define TX_DESC_NEXT_DESC_OFFSET	48
#define USB_HWDESC_HEADER_LEN	48

#define RX_DESC_SIZE	24
#define MAX_RECEIVE_BUFFER_SIZE	8192

#define SET_EARLYMODE_PKTNUM(__paddr, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__paddr, 0, 4, __val)
#define SET_EARLYMODE_LEN0(__paddr, __val)                                     \
	SET_BITS_TO_LE_4BYTE(__paddr, 4, 15, __val)
#define SET_EARLYMODE_LEN1(__paddr, __val)                                     \
	SET_BITS_TO_LE_4BYTE(__paddr, 16, 2, __val)
#define SET_EARLYMODE_LEN1_1(__paddr, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__paddr, 19, 13, __val)
#define SET_EARLYMODE_LEN1_2(__paddr, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__paddr + 4, 0, 2, __val)
#define SET_EARLYMODE_LEN2(__paddr, __val)                                     \
	SET_BITS_TO_LE_4BYTE(__paddr + 4, 2, 15, __val)
#define SET_EARLYMODE_LEN2_1(__paddr, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__paddr, 2, 4, __val)
#define SET_EARLYMODE_LEN2_2(__paddr, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__paddr + 4, 0, 8, __val)
#define SET_EARLYMODE_LEN3(__paddr, __val)                                     \
	SET_BITS_TO_LE_4BYTE(__paddr + 4, 17, 15, __val)
#define SET_EARLYMODE_LEN4(__paddr, __val)                                     \
	SET_BITS_TO_LE_4BYTE(__paddr + 4, 20, 12, __val)

/* TX/RX buffer descriptor */

/* for Txfilldescroptor8822be, fill the desc content. */
#if (DMA_IS_64BIT == 1)
#define SET_TXBUFFER_DESC_LEN_WITH_OFFSET(__pdesc, __offset, __val)            \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 16), 0, 16, __val)
#define SET_TXBUFFER_DESC_AMSDU_WITH_OFFSET(__pdesc, __offset, __val)          \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 16), 31, 1, __val)
#define SET_TXBUFFER_DESC_ADD_LOW_WITH_OFFSET(__pdesc, __offset, __val)        \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 16) + 4, 0, 32, __val)
#define SET_TXBUFFER_DESC_ADD_HIGH_WITH_OFFSET(__pdesc, __offset, __val)       \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 16) + 8, 0, 32, __val)
#define GET_TXBUFFER_DESC_ADDR_LOW(__pdesc, __offset)                          \
	LE_BITS_TO_4BYTE(__pdesc + (__offset * 16) + 4, 0, 32)
#define GET_TXBUFFER_DESC_ADDR_HIGH(__pdesc, __offset)                         \
	LE_BITS_TO_4BYTE(__pdesc + (__offset * 16) + 8, 0, 32)
#else
#define SET_TXBUFFER_DESC_LEN_WITH_OFFSET(__pdesc, __offset, __val)            \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 8), 0, 16, __val)
#define SET_TXBUFFER_DESC_AMSDU_WITH_OFFSET(__pdesc, __offset, __val)          \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 8), 31, 1, __val)
#define SET_TXBUFFER_DESC_ADD_LOW_WITH_OFFSET(__pdesc, __offset, __val)        \
	SET_BITS_TO_LE_4BYTE(__pdesc + (__offset * 8) + 4, 0, 32, __val)
#define SET_TXBUFFER_DESC_ADD_HIGH_WITH_OFFSET(__pdesc, __offset, __val)
#define GET_TXBUFFER_DESC_ADDR_LOW(__pdesc, __offset)                          \
	LE_BITS_TO_4BYTE(__pdesc + (__offset * 8) + 4, 0, 32)
#endif

/* Dword 0 */
#define SET_TX_BUFF_DESC_LEN_0(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc, 0, 14, __val)
#define SET_TX_BUFF_DESC_PSB(__pdesc, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__pdesc, 16, 15, __val)
#define SET_TX_BUFF_DESC_OWN(__pdesc, __val)                                   \
	SET_BITS_TO_LE_4BYTE(__pdesc, 31, 1, __val)

/* Dword 1 */
#define SET_TX_BUFF_DESC_ADDR_LOW_0(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 4, 0, 32, __val)
#if (DMA_IS_64BIT == 1)
/* Dword 2 */
#define SET_TX_BUFF_DESC_ADDR_HIGH_0(__pdesc, __val)                           \
	SET_BITS_TO_LE_4BYTE(__pdesc + 8, 0, 32, __val)
/* Dword 3 / RESERVED 0 */
/* Dword 4 */
#define SET_TX_BUFF_DESC_LEN_1(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 16, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_1(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 16, 31, 1, __val)
/* Dword 5 */
#define SET_TX_BUFF_DESC_ADDR_LOW_1(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 20, 0, 32, __val)
/* Dword 6 */
#define SET_TX_BUFF_DESC_ADDR_HIGH_1(__pdesc, __val)                           \
	SET_BITS_TO_LE_4BYTE(__pdesc + 24, 0, 32, __val)
/* Dword 7 / RESERVED 0 */
/* Dword 8 */
#define SET_TX_BUFF_DESC_LEN_2(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 32, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_2(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 32, 31, 1, __val)
/* Dword 9 */
#define SET_TX_BUFF_DESC_ADDR_LOW_2(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 36, 0, 32, __val)
/* Dword 10 */
#define SET_TX_BUFF_DESC_ADDR_HIGH_2(__pdesc, __val)                           \
	SET_BITS_TO_LE_4BYTE(__pdesc + 40, 0, 32, __val)
/* Dword 11 / RESERVED 0 */
/* Dword 12 */
#define SET_TX_BUFF_DESC_LEN_3(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 48, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_3(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 48, 31, 1, __val)
/* Dword 13 */
#define SET_TX_BUFF_DESC_ADDR_LOW_3(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 52, 0, 32, __val)
/* Dword 14 */
#define SET_TX_BUFF_DESC_ADDR_HIGH_3(__pdesc, __val)                           \
	SET_BITS_TO_LE_4BYTE(__pdesc + 56, 0, 32, __val)
/* Dword 15 / RESERVED 0 */
#else
#define SET_TX_BUFF_DESC_ADDR_HIGH_0(__pdesc, __val)
/* Dword 2 */
#define SET_TX_BUFF_DESC_LEN_1(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 8, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_1(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 8, 31, 1, __val)
/* Dword 3 */
#define SET_TX_BUFF_DESC_ADDR_LOW_1(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 12, 0, 32, __val)
#define SET_TX_BUFF_DESC_ADDR_HIGH_1(__pdesc, __val)
/* Dword 4 */
#define SET_TX_BUFF_DESC_LEN_2(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 16, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_2(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 16, 31, 1, __val)
/* Dword 5 */
#define SET_TX_BUFF_DESC_ADDR_LOW_2(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 20, 0, 32, __val)
#define SET_TX_BUFF_DESC_ADDR_HIGH_2(__pdesc, __val)
/* Dword 6 */
#define SET_TX_BUFF_DESC_LEN_3(__pdesc, __val)                                 \
	SET_BITS_TO_LE_4BYTE(__pdesc + 24, 0, 16, __val)
#define SET_TX_BUFF_DESC_AMSDU_3(__pdesc, __val)                               \
	SET_BITS_TO_LE_4BYTE(__pdesc + 24, 31, 1, __val)
/* Dword 7 */
#define SET_TX_BUFF_DESC_ADDR_LOW_3(__pdesc, __val)                            \
	SET_BITS_TO_LE_4BYTE(__pdesc + 28, 0, 32, __val)
#define SET_TX_BUFF_DESC_ADDR_HIGH_3(__pdesc, __val)
#endif

/* RX buffer  */

/* DWORD 0 */
#define SET_RX_BUFFER_DESC_DATA_LENGTH(__rx_status_desc, __val)                \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc, 0, 14, __val)
#define SET_RX_BUFFER_DESC_LS(__rx_status_desc, __val)                         \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc, 15, 1, __val)
#define SET_RX_BUFFER_DESC_FS(__rx_status_desc, __val)                         \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc, 16, 1, __val)
#define SET_RX_BUFFER_DESC_TOTAL_LENGTH(__rx_status_desc, __val)               \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc, 16, 15, __val)

#define GET_RX_BUFFER_DESC_OWN(__rx_status_desc)                               \
	LE_BITS_TO_4BYTE(__rx_status_desc, 31, 1)
#define GET_RX_BUFFER_DESC_LS(__rx_status_desc)                                \
	LE_BITS_TO_4BYTE(__rx_status_desc, 15, 1)
#define GET_RX_BUFFER_DESC_FS(__rx_status_desc)                                \
	LE_BITS_TO_4BYTE(__rx_status_desc, 16, 1)
#define GET_RX_BUFFER_DESC_TOTAL_LENGTH(__rx_status_desc)                      \
	LE_BITS_TO_4BYTE(__rx_status_desc, 16, 15)

/* DWORD 1 */
#define SET_RX_BUFFER_PHYSICAL_LOW(__rx_status_desc, __val)                    \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc + 4, 0, 32, __val)

/* DWORD 2 */
#define SET_RX_BUFFER_PHYSICAL_HIGH(__rx_status_desc, __val)                   \
	SET_BITS_TO_LE_4BYTE(__rx_status_desc + 8, 0, 32, __val)

#define CLEAR_PCI_TX_DESC_CONTENT(__pdesc, _size)                              \
	do {                                                                   \
		if (_size > TX_DESC_NEXT_DESC_OFFSET)                          \
			memset(__pdesc, 0, TX_DESC_NEXT_DESC_OFFSET);          \
		else                                                           \
			memset(__pdesc, 0, _size);                             \
	} while (0)

void rtl8822be_rx_check_dma_ok(struct ieee80211_hw *hw, u8 *header_desc,
			       u8 queue_index);
u16 rtl8822be_rx_desc_buff_remained_cnt(struct ieee80211_hw *hw,
					u8 queue_index);
u16 rtl8822be_get_available_desc(struct ieee80211_hw *hw, u8 queue_index);
void rtl8822be_pre_fill_tx_bd_desc(struct ieee80211_hw *hw, u8 *tx_bd_desc,
				   u8 *desc, u8 queue_index,
				   struct sk_buff *skb, dma_addr_t addr);

void rtl8822be_tx_fill_desc(struct ieee80211_hw *hw, struct ieee80211_hdr *hdr,
			    u8 *pdesc_tx, u8 *pbd_desc_tx,
			    struct ieee80211_tx_info *info,
			    struct ieee80211_sta *sta, struct sk_buff *skb,
			    u8 hw_queue, struct rtl_tcb_desc *ptcb_desc);
void rtl8822be_tx_fill_special_desc(struct ieee80211_hw *hw, u8 *pdesc,
				    u8 *pbd_desc, struct sk_buff *skb,
				    u8 hw_queue);
bool rtl8822be_rx_query_desc(struct ieee80211_hw *hw, struct rtl_stats *status,
			     struct ieee80211_rx_status *rx_status, u8 *pdesc,
			     struct sk_buff *skb);
void rtl8822be_set_desc(struct ieee80211_hw *hw, u8 *pdesc, bool istx,
			u8 desc_name, u8 *val);

u32 rtl8822be_get_desc(u8 *pdesc, bool istx, u8 desc_name);
bool rtl8822be_is_tx_desc_closed(struct ieee80211_hw *hw, u8 hw_queue,
				 u16 index);
void rtl8822be_tx_polling(struct ieee80211_hw *hw, u8 hw_queue);
void rtl8822be_tx_fill_cmddesc(struct ieee80211_hw *hw, u8 *pdesc,
			       bool firstseg, bool lastseg,
			       struct sk_buff *skb);
u32 rtl8822be_rx_command_packet(struct ieee80211_hw *hw,
				const struct rtl_stats *status,
				struct sk_buff *skb);
#endif
