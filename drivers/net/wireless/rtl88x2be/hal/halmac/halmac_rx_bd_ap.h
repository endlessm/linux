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

#ifndef _HALMAC_RX_BD_AP_H_
#define _HALMAC_RX_BD_AP_H_
#if (HALMAC_8814A_SUPPORT || HALMAC_8822B_SUPPORT || HALMAC_8197F_SUPPORT || HALMAC_8821C_SUPPORT || HALMAC_8188F_SUPPORT)

/*TXBD_DW0*/

#define GET_RX_BD_RXFAIL(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x1, 31)
#define GET_RX_BD_TOTALRXPKTSIZE(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x1fff, 16)
#define GET_RX_BD_RXTAG(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x1fff, 16)
#define GET_RX_BD_FS(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x1, 15)
#define GET_RX_BD_LS(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x1, 14)
#define GET_RX_BD_RXBUFFSIZE(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword0, 0x3fff, 0)

/*TXBD_DW1*/

#define GET_RX_BD_PHYSICAL_ADDR_LOW(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword1, 0xffffffff, 0)

/*TXBD_DW2*/

#define GET_RX_BD_PHYSICAL_ADDR_HIGH(__pRxBd)    HALMAC_GET_BD_FIELD(((PHALMAC_RX_BD)__pRxBd)->Dword2, 0xffffffff, 0)

#endif


#endif
