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

#ifndef _HAL_H2CEXTRAINFO_H2C_C2H_NIC_H_
#define _HAL_H2CEXTRAINFO_H2C_C2H_NIC_H_
#define PHY_PARAMETER_INFO_GET_LENGTH(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 0, 8)
#define PHY_PARAMETER_INFO_SET_LENGTH(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 0, 8, __Value)
#define PHY_PARAMETER_INFO_GET_IO_CMD(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 8, 7)
#define PHY_PARAMETER_INFO_SET_IO_CMD(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 8, 7, __Value)
#define PHY_PARAMETER_INFO_GET_MSK_EN(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 15, 1)
#define PHY_PARAMETER_INFO_SET_MSK_EN(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 15, 1, __Value)
#define PHY_PARAMETER_INFO_GET_LLT_PG_BNDY(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 8)
#define PHY_PARAMETER_INFO_SET_LLT_PG_BNDY(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 8, __Value)
#define PHY_PARAMETER_INFO_GET_EFUSE_RSVDPAGE_LOC(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 8)
#define PHY_PARAMETER_INFO_SET_EFUSE_RSVDPAGE_LOC(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 8, __Value)
#define PHY_PARAMETER_INFO_GET_EFUSE_PATCH_EN(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 8)
#define PHY_PARAMETER_INFO_SET_EFUSE_PATCH_EN(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 8, __Value)
#define PHY_PARAMETER_INFO_GET_RF_ADDR(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 8)
#define PHY_PARAMETER_INFO_SET_RF_ADDR(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 8, __Value)
#define PHY_PARAMETER_INFO_GET_IO_ADDR(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 16)
#define PHY_PARAMETER_INFO_SET_IO_ADDR(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 16, __Value)
#define PHY_PARAMETER_INFO_GET_DELAY_VALUE(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 16)
#define PHY_PARAMETER_INFO_SET_DELAY_VALUE(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 16, __Value)
#define PHY_PARAMETER_INFO_GET_RF_PATH(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 24, 8)
#define PHY_PARAMETER_INFO_SET_RF_PATH(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 24, 8, __Value)
#define PHY_PARAMETER_INFO_GET_DATA(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X04, 0, 32)
#define PHY_PARAMETER_INFO_SET_DATA(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X04, 0, 32, __Value)
#define PHY_PARAMETER_INFO_GET_MASK(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X08, 0, 32)
#define PHY_PARAMETER_INFO_SET_MASK(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X08, 0, 32, __Value)
#define CHANNEL_INFO_GET_CHANNEL(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 0, 8)
#define CHANNEL_INFO_SET_CHANNEL(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 0, 8, __Value)
#define CHANNEL_INFO_GET_PRI_CH_IDX(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 8, 4)
#define CHANNEL_INFO_SET_PRI_CH_IDX(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 8, 4, __Value)
#define CHANNEL_INFO_GET_BANDWIDTH(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 12, 4)
#define CHANNEL_INFO_SET_BANDWIDTH(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 12, 4, __Value)
#define CHANNEL_INFO_GET_TIMEOUT(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 8)
#define CHANNEL_INFO_SET_TIMEOUT(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 8, __Value)
#define CHANNEL_INFO_GET_ACTION_ID(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 24, 7)
#define CHANNEL_INFO_SET_ACTION_ID(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 24, 7, __Value)
#define CHANNEL_INFO_GET_CH_EXTRA_INFO(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 31, 1)
#define CHANNEL_INFO_SET_CH_EXTRA_INFO(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 31, 1, __Value)
#define CH_EXTRA_INFO_GET_CH_EXTRA_INFO_ID(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 0, 7)
#define CH_EXTRA_INFO_SET_CH_EXTRA_INFO_ID(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 0, 7, __Value)
#define CH_EXTRA_INFO_GET_CH_EXTRA_INFO(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 7, 1)
#define CH_EXTRA_INFO_SET_CH_EXTRA_INFO(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 7, 1, __Value)
#define CH_EXTRA_INFO_GET_CH_EXTRA_INFO_SIZE(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 8, 8)
#define CH_EXTRA_INFO_SET_CH_EXTRA_INFO_SIZE(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 8, 8, __Value)
#define CH_EXTRA_INFO_GET_CH_EXTRA_INFO_DATA(__pExtraInfo)    LE_BITS_TO_4BYTE(__pExtraInfo + 0X00, 16, 1)
#define CH_EXTRA_INFO_SET_CH_EXTRA_INFO_DATA(__pExtraInfo, __Value)    SET_BITS_TO_LE_4BYTE(__pExtraInfo + 0X00, 16, 1, __Value)
#endif
