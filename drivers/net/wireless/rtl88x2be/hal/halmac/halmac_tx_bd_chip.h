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

#ifndef _HALMAC_TX_BD_CHIP_H_
#define _HALMAC_TX_BD_CHIP_H_
#if (HALMAC_8814A_SUPPORT)

/*TXBD_DW0*/

#define SET_TX_BD_OWN_8814A(__pTxBd, __Value)    SET_TX_BD_OWN(__pTxBd, __Value)
#define GET_TX_BD_OWN_8814A(__pTxBd)    GET_TX_BD_OWN(__pTxBd)
#define SET_TX_BD_PSB_8814A(__pTxBd, __Value)    SET_TX_BD_PSB(__pTxBd, __Value)
#define GET_TX_BD_PSB_8814A(__pTxBd)    GET_TX_BD_PSB(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE0_8814A(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE0(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE0_8814A(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE0(__pTxBd)

/*TXBD_DW1*/

#define SET_TX_BD_PHYSICAL_ADDR0_LOW_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_LOW_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd)

/*TXBD_DW2*/

#define SET_TX_BD_PHYSICAL_ADDR0_HIGH_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_HIGH_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd)

/*TXBD_DW4*/

#define SET_TX_BD_A1_8814A(__pTxBd, __Value)    SET_TX_BD_A1(__pTxBd, __Value)
#define GET_TX_BD_A1_8814A(__pTxBd)    GET_TX_BD_A1(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE1_8814A(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE1(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE1_8814A(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE1(__pTxBd)

/*TXBD_DW5*/

#define SET_TX_BD_PHYSICAL_ADDR1_LOW_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_LOW_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd)

/*TXBD_DW6*/

#define SET_TX_BD_PHYSICAL_ADDR1_HIGH_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_HIGH_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd)

/*TXBD_DW8*/

#define SET_TX_BD_A2_8814A(__pTxBd, __Value)    SET_TX_BD_A2(__pTxBd, __Value)
#define GET_TX_BD_A2_8814A(__pTxBd)    GET_TX_BD_A2(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE2_8814A(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE2(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE2_8814A(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE2(__pTxBd)

/*TXBD_DW9*/

#define SET_TX_BD_PHYSICAL_ADDR2_LOW_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_LOW_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd)

/*TXBD_DW10*/

#define SET_TX_BD_PHYSICAL_ADDR2_HIGH_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_HIGH_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd)

/*TXBD_DW12*/

#define SET_TX_BD_A3_8814A(__pTxBd, __Value)    SET_TX_BD_A3(__pTxBd, __Value)
#define GET_TX_BD_A3_8814A(__pTxBd)    GET_TX_BD_A3(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE3_8814A(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE3(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE3_8814A(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE3(__pTxBd)

/*TXBD_DW13*/

#define SET_TX_BD_PHYSICAL_ADDR3_LOW_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_LOW_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd)

/*TXBD_DW14*/

#define SET_TX_BD_PHYSICAL_ADDR3_HIGH_8814A(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_HIGH_8814A(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd)

#endif

#if (HALMAC_8822B_SUPPORT)

/*TXBD_DW0*/

#define SET_TX_BD_OWN_8822B(__pTxBd, __Value)    SET_TX_BD_OWN(__pTxBd, __Value)
#define GET_TX_BD_OWN_8822B(__pTxBd)    GET_TX_BD_OWN(__pTxBd)
#define SET_TX_BD_PSB_8822B(__pTxBd, __Value)    SET_TX_BD_PSB(__pTxBd, __Value)
#define GET_TX_BD_PSB_8822B(__pTxBd)    GET_TX_BD_PSB(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE0_8822B(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE0(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE0_8822B(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE0(__pTxBd)

/*TXBD_DW1*/

#define SET_TX_BD_PHYSICAL_ADDR0_LOW_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_LOW_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd)

/*TXBD_DW2*/

#define SET_TX_BD_PHYSICAL_ADDR0_HIGH_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_HIGH_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd)

/*TXBD_DW4*/

#define SET_TX_BD_A1_8822B(__pTxBd, __Value)    SET_TX_BD_A1(__pTxBd, __Value)
#define GET_TX_BD_A1_8822B(__pTxBd)    GET_TX_BD_A1(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE1_8822B(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE1(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE1_8822B(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE1(__pTxBd)

/*TXBD_DW5*/

#define SET_TX_BD_PHYSICAL_ADDR1_LOW_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_LOW_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd)

/*TXBD_DW6*/

#define SET_TX_BD_PHYSICAL_ADDR1_HIGH_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_HIGH_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd)

/*TXBD_DW8*/

#define SET_TX_BD_A2_8822B(__pTxBd, __Value)    SET_TX_BD_A2(__pTxBd, __Value)
#define GET_TX_BD_A2_8822B(__pTxBd)    GET_TX_BD_A2(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE2_8822B(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE2(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE2_8822B(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE2(__pTxBd)

/*TXBD_DW9*/

#define SET_TX_BD_PHYSICAL_ADDR2_LOW_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_LOW_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd)

/*TXBD_DW10*/

#define SET_TX_BD_PHYSICAL_ADDR2_HIGH_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_HIGH_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd)

/*TXBD_DW12*/

#define SET_TX_BD_A3_8822B(__pTxBd, __Value)    SET_TX_BD_A3(__pTxBd, __Value)
#define GET_TX_BD_A3_8822B(__pTxBd)    GET_TX_BD_A3(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE3_8822B(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE3(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE3_8822B(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE3(__pTxBd)

/*TXBD_DW13*/

#define SET_TX_BD_PHYSICAL_ADDR3_LOW_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_LOW_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd)

/*TXBD_DW14*/

#define SET_TX_BD_PHYSICAL_ADDR3_HIGH_8822B(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_HIGH_8822B(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd)

#endif

#if (HALMAC_8197F_SUPPORT)

/*TXBD_DW0*/

#define SET_TX_BD_OWN_8197F(__pTxBd, __Value)    SET_TX_BD_OWN(__pTxBd, __Value)
#define GET_TX_BD_OWN_8197F(__pTxBd)    GET_TX_BD_OWN(__pTxBd)
#define SET_TX_BD_PSB_8197F(__pTxBd, __Value)    SET_TX_BD_PSB(__pTxBd, __Value)
#define GET_TX_BD_PSB_8197F(__pTxBd)    GET_TX_BD_PSB(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE0_8197F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE0(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE0_8197F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE0(__pTxBd)

/*TXBD_DW1*/

#define SET_TX_BD_PHYSICAL_ADDR0_LOW_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_LOW_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd)

/*TXBD_DW2*/

#define SET_TX_BD_PHYSICAL_ADDR0_HIGH_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_HIGH_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd)

/*TXBD_DW4*/

#define SET_TX_BD_A1_8197F(__pTxBd, __Value)    SET_TX_BD_A1(__pTxBd, __Value)
#define GET_TX_BD_A1_8197F(__pTxBd)    GET_TX_BD_A1(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE1_8197F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE1(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE1_8197F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE1(__pTxBd)

/*TXBD_DW5*/

#define SET_TX_BD_PHYSICAL_ADDR1_LOW_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_LOW_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd)

/*TXBD_DW6*/

#define SET_TX_BD_PHYSICAL_ADDR1_HIGH_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_HIGH_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd)

/*TXBD_DW8*/

#define SET_TX_BD_A2_8197F(__pTxBd, __Value)    SET_TX_BD_A2(__pTxBd, __Value)
#define GET_TX_BD_A2_8197F(__pTxBd)    GET_TX_BD_A2(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE2_8197F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE2(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE2_8197F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE2(__pTxBd)

/*TXBD_DW9*/

#define SET_TX_BD_PHYSICAL_ADDR2_LOW_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_LOW_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd)

/*TXBD_DW10*/

#define SET_TX_BD_PHYSICAL_ADDR2_HIGH_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_HIGH_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd)

/*TXBD_DW12*/

#define SET_TX_BD_A3_8197F(__pTxBd, __Value)    SET_TX_BD_A3(__pTxBd, __Value)
#define GET_TX_BD_A3_8197F(__pTxBd)    GET_TX_BD_A3(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE3_8197F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE3(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE3_8197F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE3(__pTxBd)

/*TXBD_DW13*/

#define SET_TX_BD_PHYSICAL_ADDR3_LOW_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_LOW_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd)

/*TXBD_DW14*/

#define SET_TX_BD_PHYSICAL_ADDR3_HIGH_8197F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_HIGH_8197F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd)

#endif

#if (HALMAC_8821C_SUPPORT)

/*TXBD_DW0*/

#define SET_TX_BD_OWN_8821C(__pTxBd, __Value)    SET_TX_BD_OWN(__pTxBd, __Value)
#define GET_TX_BD_OWN_8821C(__pTxBd)    GET_TX_BD_OWN(__pTxBd)
#define SET_TX_BD_PSB_8821C(__pTxBd, __Value)    SET_TX_BD_PSB(__pTxBd, __Value)
#define GET_TX_BD_PSB_8821C(__pTxBd)    GET_TX_BD_PSB(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE0_8821C(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE0(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE0_8821C(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE0(__pTxBd)

/*TXBD_DW1*/

#define SET_TX_BD_PHYSICAL_ADDR0_LOW_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_LOW_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd)

/*TXBD_DW2*/

#define SET_TX_BD_PHYSICAL_ADDR0_HIGH_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_HIGH_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd)

/*TXBD_DW4*/

#define SET_TX_BD_A1_8821C(__pTxBd, __Value)    SET_TX_BD_A1(__pTxBd, __Value)
#define GET_TX_BD_A1_8821C(__pTxBd)    GET_TX_BD_A1(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE1_8821C(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE1(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE1_8821C(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE1(__pTxBd)

/*TXBD_DW5*/

#define SET_TX_BD_PHYSICAL_ADDR1_LOW_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_LOW_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd)

/*TXBD_DW6*/

#define SET_TX_BD_PHYSICAL_ADDR1_HIGH_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_HIGH_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd)

/*TXBD_DW8*/

#define SET_TX_BD_A2_8821C(__pTxBd, __Value)    SET_TX_BD_A2(__pTxBd, __Value)
#define GET_TX_BD_A2_8821C(__pTxBd)    GET_TX_BD_A2(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE2_8821C(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE2(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE2_8821C(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE2(__pTxBd)

/*TXBD_DW9*/

#define SET_TX_BD_PHYSICAL_ADDR2_LOW_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_LOW_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd)

/*TXBD_DW10*/

#define SET_TX_BD_PHYSICAL_ADDR2_HIGH_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_HIGH_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd)

/*TXBD_DW12*/

#define SET_TX_BD_A3_8821C(__pTxBd, __Value)    SET_TX_BD_A3(__pTxBd, __Value)
#define GET_TX_BD_A3_8821C(__pTxBd)    GET_TX_BD_A3(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE3_8821C(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE3(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE3_8821C(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE3(__pTxBd)

/*TXBD_DW13*/

#define SET_TX_BD_PHYSICAL_ADDR3_LOW_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_LOW_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd)

/*TXBD_DW14*/

#define SET_TX_BD_PHYSICAL_ADDR3_HIGH_8821C(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_HIGH_8821C(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd)

#endif

#if (HALMAC_8188F_SUPPORT)

/*TXBD_DW0*/

#define SET_TX_BD_OWN_8188F(__pTxBd, __Value)    SET_TX_BD_OWN(__pTxBd, __Value)
#define GET_TX_BD_OWN_8188F(__pTxBd)    GET_TX_BD_OWN(__pTxBd)
#define SET_TX_BD_PSB_8188F(__pTxBd, __Value)    SET_TX_BD_PSB(__pTxBd, __Value)
#define GET_TX_BD_PSB_8188F(__pTxBd)    GET_TX_BD_PSB(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE0_8188F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE0(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE0_8188F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE0(__pTxBd)

/*TXBD_DW1*/

#define SET_TX_BD_PHYSICAL_ADDR0_LOW_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_LOW_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_LOW(__pTxBd)

/*TXBD_DW2*/

#define SET_TX_BD_PHYSICAL_ADDR0_HIGH_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR0_HIGH_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR0_HIGH(__pTxBd)

/*TXBD_DW4*/

#define SET_TX_BD_A1_8188F(__pTxBd, __Value)    SET_TX_BD_A1(__pTxBd, __Value)
#define GET_TX_BD_A1_8188F(__pTxBd)    GET_TX_BD_A1(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE1_8188F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE1(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE1_8188F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE1(__pTxBd)

/*TXBD_DW5*/

#define SET_TX_BD_PHYSICAL_ADDR1_LOW_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_LOW_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_LOW(__pTxBd)

/*TXBD_DW6*/

#define SET_TX_BD_PHYSICAL_ADDR1_HIGH_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR1_HIGH_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR1_HIGH(__pTxBd)

/*TXBD_DW8*/

#define SET_TX_BD_A2_8188F(__pTxBd, __Value)    SET_TX_BD_A2(__pTxBd, __Value)
#define GET_TX_BD_A2_8188F(__pTxBd)    GET_TX_BD_A2(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE2_8188F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE2(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE2_8188F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE2(__pTxBd)

/*TXBD_DW9*/

#define SET_TX_BD_PHYSICAL_ADDR2_LOW_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_LOW_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_LOW(__pTxBd)

/*TXBD_DW10*/

#define SET_TX_BD_PHYSICAL_ADDR2_HIGH_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR2_HIGH_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR2_HIGH(__pTxBd)

/*TXBD_DW12*/

#define SET_TX_BD_A3_8188F(__pTxBd, __Value)    SET_TX_BD_A3(__pTxBd, __Value)
#define GET_TX_BD_A3_8188F(__pTxBd)    GET_TX_BD_A3(__pTxBd)
#define SET_TX_BD_TX_BUFF_SIZE3_8188F(__pTxBd, __Value)    SET_TX_BD_TX_BUFF_SIZE3(__pTxBd, __Value)
#define GET_TX_BD_TX_BUFF_SIZE3_8188F(__pTxBd)    GET_TX_BD_TX_BUFF_SIZE3(__pTxBd)

/*TXBD_DW13*/

#define SET_TX_BD_PHYSICAL_ADDR3_LOW_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_LOW_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_LOW(__pTxBd)

/*TXBD_DW14*/

#define SET_TX_BD_PHYSICAL_ADDR3_HIGH_8188F(__pTxBd, __Value)    SET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd, __Value)
#define GET_TX_BD_PHYSICAL_ADDR3_HIGH_8188F(__pTxBd)    GET_TX_BD_PHYSICAL_ADDR3_HIGH(__pTxBd)

#endif


#endif
