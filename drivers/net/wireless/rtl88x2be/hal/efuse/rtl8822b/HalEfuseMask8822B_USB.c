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
#include <drv_types.h>

#include "HalEfuseMask8822B_USB.h"

/******************************************************************************
*                           MUSB.TXT
******************************************************************************/

u1Byte Array_MP_8822B_MUSB[] = {
	0xFF,
	0xF7,
	0xEF,
	0xDE,
	0xFC,
	0xFB,
	0x10,
	0x00,
	0x00,
	0x00,
	0x00,
	0x03,
	0xF7,
	0x00,
	0x00,
	0x00,
	0xFF,
	0xFF,
	0xFF,
	0xFF,
	0xF0,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

u2Byte EFUSE_GetArrayLen_MP_8822B_MUSB(VOID)
{
	return sizeof(Array_MP_8822B_MUSB) / sizeof(u1Byte);
}

VOID EFUSE_GetMaskArray_MP_8822B_MUSB(pu1Byte Array)
{
	u2Byte len = EFUSE_GetArrayLen_MP_8822B_MUSB(), i = 0;

	for (i = 0; i < len; ++i)
		Array[i] = Array_MP_8822B_MUSB[i];
}

BOOLEAN EFUSE_IsAddressMasked_MP_8822B_MUSB(u2Byte  Offset)
{
	int r = Offset / 16;
	int c = (Offset % 16) / 2;
	int result = 0;

	if (c < 4) /*Upper double word*/
		result = (Array_MP_8822B_MUSB[r] & (0x10 << c));
	else
		result = (Array_MP_8822B_MUSB[r] & (0x01 << (c - 4)));

	return (result > 0) ? 0 : 1;
}
