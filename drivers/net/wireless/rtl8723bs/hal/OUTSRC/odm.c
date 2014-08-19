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

//============================================================
// include files
//============================================================

#include "odm_precomp.h"


const u2Byte dB_Invert_Table[8][12] = {
	{	1,		1,		1,		2,		2,		2,		2,		3,		3,		3,		4,		4},
	{	4,		5,		6,		6,		7,		8,		9,		10,		11,		13,		14,		16},
	{	18,		20,		22,		25,		28,		32,		35,		40,		45,		50,		56,		63},
	{	71,		79,		89,		100,	112,	126,	141,	158,	178,	200,	224,	251},
	{	282,	316,	355,	398,	447,	501,	562,	631,	708,	794,	891,	1000},
	{	1122,	1259,	1413,	1585,	1778,	1995,	2239,	2512,	2818,	3162,	3548,	3981},
	{	4467,	5012,	5623,	6310,	7079,	7943,	8913,	10000,	11220,	12589,	14125,	15849},
	{	17783,	19953,	22387,	25119,	28184,	31623,	35481,	39811,	44668,	50119,	56234,	65535}};


//============================================================
// Global var
//============================================================

u4Byte	OFDMSwingTable[OFDM_TABLE_SIZE] = {
	0x7f8001fe,	// 0, +6.0dB
	0x788001e2,	// 1, +5.5dB
	0x71c001c7,	// 2, +5.0dB
	0x6b8001ae,	// 3, +4.5dB
	0x65400195,	// 4, +4.0dB
	0x5fc0017f,	// 5, +3.5dB
	0x5a400169,	// 6, +3.0dB
	0x55400155,	// 7, +2.5dB
	0x50800142,	// 8, +2.0dB
	0x4c000130,	// 9, +1.5dB
	0x47c0011f,	// 10, +1.0dB
	0x43c0010f,	// 11, +0.5dB
	0x40000100,	// 12, +0dB
	0x3c8000f2,	// 13, -0.5dB
	0x390000e4,	// 14, -1.0dB
	0x35c000d7,	// 15, -1.5dB
	0x32c000cb,	// 16, -2.0dB
	0x300000c0,	// 17, -2.5dB
	0x2d4000b5,	// 18, -3.0dB
	0x2ac000ab,	// 19, -3.5dB
	0x288000a2,	// 20, -4.0dB
	0x26000098,	// 21, -4.5dB
	0x24000090,	// 22, -5.0dB
	0x22000088,	// 23, -5.5dB
	0x20000080,	// 24, -6.0dB
	0x1e400079,	// 25, -6.5dB
	0x1c800072,	// 26, -7.0dB
	0x1b00006c,	// 27. -7.5dB
	0x19800066,	// 28, -8.0dB
	0x18000060,	// 29, -8.5dB
	0x16c0005b,	// 30, -9.0dB
	0x15800056,	// 31, -9.5dB
	0x14400051,	// 32, -10.0dB
	0x1300004c,	// 33, -10.5dB
	0x12000048,	// 34, -11.0dB
	0x11000044,	// 35, -11.5dB
	0x10000040,	// 36, -12.0dB
};

u1Byte	CCKSwingTable_Ch1_Ch13[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},	// 0, +0dB
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},	// 1, -0.5dB
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},	// 2, -1.0dB
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},	// 3, -1.5dB
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},	// 4, -2.0dB 
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},	// 5, -2.5dB
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},	// 6, -3.0dB
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},	// 7, -3.5dB
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},	// 8, -4.0dB 
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},	// 9, -4.5dB
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},	// 10, -5.0dB 
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},	// 11, -5.5dB
	{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},	// 12, -6.0dB <== default
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},	// 13, -6.5dB
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},	// 14, -7.0dB 
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},	// 15, -7.5dB
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},	// 16, -8.0dB 
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},	// 17, -8.5dB
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},	// 18, -9.0dB 
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 19, -9.5dB
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 20, -10.0dB
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 21, -10.5dB
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 22, -11.0dB
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},	// 23, -11.5dB
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},	// 24, -12.0dB
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},	// 25, -12.5dB
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},	// 26, -13.0dB
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},	// 27, -13.5dB
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},	// 28, -14.0dB
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},	// 29, -14.5dB
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},	// 30, -15.0dB
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},	// 31, -15.5dB
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01}	// 32, -16.0dB
};


u1Byte	CCKSwingTable_Ch14[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},	// 0, +0dB  
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},	// 1, -0.5dB 
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},	// 2, -1.0dB  
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},	// 3, -1.5dB
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},	// 4, -2.0dB  
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},	// 5, -2.5dB
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},	// 6, -3.0dB  
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},	// 7, -3.5dB  
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},	// 8, -4.0dB  
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},	// 9, -4.5dB
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},	// 10, -5.0dB  
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 11, -5.5dB
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 12, -6.0dB  <== default
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},	// 13, -6.5dB 
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},	// 14, -7.0dB  
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 15, -7.5dB
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 16, -8.0dB  
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 17, -8.5dB
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 18, -9.0dB  
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 19, -9.5dB
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 20, -10.0dB
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},	// 21, -10.5dB
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},	// 22, -11.0dB
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 23, -11.5dB
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 24, -12.0dB
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},	// 25, -12.5dB
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 26, -13.0dB
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 27, -13.5dB
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 28, -14.0dB
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 29, -14.5dB
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 30, -15.0dB
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 31, -15.5dB
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00}	// 32, -16.0dB
};


u4Byte OFDMSwingTable_New[OFDM_TABLE_SIZE] = {
	0x0b40002d, // 0,  -15.0dB	
	0x0c000030, // 1,  -14.5dB
	0x0cc00033, // 2,  -14.0dB
	0x0d800036, // 3,  -13.5dB
	0x0e400039, // 4,  -13.0dB    
	0x0f00003c, // 5,  -12.5dB
	0x10000040, // 6,  -12.0dB
	0x11000044, // 7,  -11.5dB
	0x12000048, // 8,  -11.0dB
	0x1300004c, // 9,  -10.5dB
	0x14400051, // 10, -10.0dB
	0x15800056, // 11, -9.5dB
	0x16c0005b, // 12, -9.0dB
	0x18000060, // 13, -8.5dB
	0x19800066, // 14, -8.0dB
	0x1b00006c, // 15, -7.5dB
	0x1c800072, // 16, -7.0dB
	0x1e400079, // 17, -6.5dB
	0x20000080, // 18, -6.0dB
	0x22000088, // 19, -5.5dB
	0x24000090, // 20, -5.0dB
	0x26000098, // 21, -4.5dB
	0x288000a2, // 22, -4.0dB
	0x2ac000ab, // 23, -3.5dB
	0x2d4000b5, // 24, -3.0dB
	0x300000c0, // 25, -2.5dB
	0x32c000cb, // 26, -2.0dB
	0x35c000d7, // 27, -1.5dB
	0x390000e4, // 28, -1.0dB
	0x3c8000f2, // 29, -0.5dB
	0x40000100, // 30, +0dB
	0x43c0010f, // 31, +0.5dB
	0x47c0011f, // 32, +1.0dB
	0x4c000130, // 33, +1.5dB
	0x50800142, // 34, +2.0dB
	0x55400155, // 35, +2.5dB
	0x5a400169, // 36, +3.0dB
	0x5fc0017f, // 37, +3.5dB
	0x65400195, // 38, +4.0dB
	0x6b8001ae, // 39, +4.5dB
	0x71c001c7, // 40, +5.0dB
	0x788001e2, // 41, +5.5dB
	0x7f8001fe  // 42, +6.0dB
};               


u1Byte CCKSwingTable_Ch1_Ch13_New[CCK_TABLE_SIZE][8] = {
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01},	//  0, -16.0dB
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},	//  1, -15.5dB
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},	//  2, -15.0dB
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},	//  3, -14.5dB
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},	//  4, -14.0dB
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},	//  5, -13.5dB
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},	//  6, -13.0dB
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},	//  7, -12.5dB
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},	//  8, -12.0dB
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},	//  9, -11.5dB
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 10, -11.0dB
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 11, -10.5dB
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 12, -10.0dB
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 13, -9.5dB
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},	// 14, -9.0dB 
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},	// 15, -8.5dB
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},	// 16, -8.0dB 
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},	// 17, -7.5dB
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},	// 18, -7.0dB 
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},	// 19, -6.5dB
    {0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},	// 20, -6.0dB 
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},	// 21, -5.5dB
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},	// 22, -5.0dB 
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},	// 23, -4.5dB
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},	// 24, -4.0dB 
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},	// 25, -3.5dB
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},	// 26, -3.0dB
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},	// 27, -2.5dB
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},	// 28, -2.0dB 
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},	// 29, -1.5dB
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},	// 30, -1.0dB
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},	// 31, -0.5dB
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04} 	// 32, +0dB
};                                                                  


u1Byte CCKSwingTable_Ch14_New[CCK_TABLE_SIZE][8]= {
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00},	//  0, -16.0dB
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	//  1, -15.5dB
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	//  2, -15.0dB
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	//  3, -14.5dB
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	//  4, -14.0dB
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	//  5, -13.5dB
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	//  6, -13.0dB
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},	//  7, -12.5dB
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	//  8, -12.0dB
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	//  9, -11.5dB
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},	// 10, -11.0dB
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},	// 11, -10.5dB
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 12, -10.0dB
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 13, -9.5dB
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 14, -9.0dB  
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 15, -8.5dB
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 16, -8.0dB  
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 17, -7.5dB
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},	// 18, -7.0dB  
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},	// 19, -6.5dB 
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 20, -6.0dB  
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 21, -5.5dB
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},	// 22, -5.0dB  
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},	// 23, -4.5dB
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},	// 24, -4.0dB  
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},	// 25, -3.5dB  
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},	// 26, -3.0dB  
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},	// 27, -2.5dB
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},	// 28, -2.0dB  
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},	// 29, -1.5dB
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},	// 30, -1.0dB  
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},	// 31, -0.5dB 
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00} 	// 32, +0dB	
};

u4Byte TxScalingTable_Jaguar[TXSCALE_TABLE_SIZE] =
{
	0x081, // 0,  -12.0dB
	0x088, // 1,  -11.5dB
	0x090, // 2,  -11.0dB
	0x099, // 3,  -10.5dB
	0x0A2, // 4,  -10.0dB
	0x0AC, // 5,  -9.5dB
	0x0B6, // 6,  -9.0dB
	0x0C0, // 7,  -8.5dB
	0x0CC, // 8,  -8.0dB
	0x0D8, // 9,  -7.5dB
	0x0E5, // 10, -7.0dB
	0x0F2, // 11, -6.5dB
	0x101, // 12, -6.0dB
	0x110, // 13, -5.5dB
	0x120, // 14, -5.0dB
	0x131, // 15, -4.5dB
	0x143, // 16, -4.0dB
	0x156, // 17, -3.5dB
	0x16A, // 18, -3.0dB
	0x180, // 19, -2.5dB
	0x197, // 20, -2.0dB
	0x1AF, // 21, -1.5dB
	0x1C8, // 22, -1.0dB
	0x1E3, // 23, -0.5dB
	0x200, // 24, +0  dB
	0x21E, // 25, +0.5dB
	0x23E, // 26, +1.0dB
	0x261, // 27, +1.5dB
	0x285, // 28, +2.0dB
	0x2AB, // 29, +2.5dB
	0x2D3, // 30, +3.0dB
	0x2FE, // 31, +3.5dB
	0x32B, // 32, +4.0dB
	0x35C, // 33, +4.5dB
	0x38E, // 34, +5.0dB
	0x3C4, // 35, +5.5dB
	0x3FE  // 36, +6.0dB	
};

#ifdef AP_BUILD_WORKAROUND

unsigned int TxPwrTrk_OFDM_SwingTbl[TxPwrTrk_OFDM_SwingTbl_Len] = {
	/*  +6.0dB */ 0x7f8001fe,
	/*  +5.5dB */ 0x788001e2,
	/*  +5.0dB */ 0x71c001c7,
	/*  +4.5dB */ 0x6b8001ae,
	/*  +4.0dB */ 0x65400195,
	/*  +3.5dB */ 0x5fc0017f,
	/*  +3.0dB */ 0x5a400169,
	/*  +2.5dB */ 0x55400155,
	/*  +2.0dB */ 0x50800142,
	/*  +1.5dB */ 0x4c000130,
	/*  +1.0dB */ 0x47c0011f,
	/*  +0.5dB */ 0x43c0010f,
	/*   0.0dB */ 0x40000100,
	/*  -0.5dB */ 0x3c8000f2,
	/*  -1.0dB */ 0x390000e4,
	/*  -1.5dB */ 0x35c000d7,
	/*  -2.0dB */ 0x32c000cb,
	/*  -2.5dB */ 0x300000c0,
	/*  -3.0dB */ 0x2d4000b5,
	/*  -3.5dB */ 0x2ac000ab,
	/*  -4.0dB */ 0x288000a2,
	/*  -4.5dB */ 0x26000098,
	/*  -5.0dB */ 0x24000090,
	/*  -5.5dB */ 0x22000088,
	/*  -6.0dB */ 0x20000080,
	/*  -6.5dB */ 0x1a00006c,
	/*  -7.0dB */ 0x1c800072,
	/*  -7.5dB */ 0x18000060,
	/*  -8.0dB */ 0x19800066,
	/*  -8.5dB */ 0x15800056,
	/*  -9.0dB */ 0x26c0005b,
	/*  -9.5dB */ 0x14400051,
	/* -10.0dB */ 0x24400051,
	/* -10.5dB */ 0x1300004c,
	/* -11.0dB */ 0x12000048,
	/* -11.5dB */ 0x11000044,
	/* -12.0dB */ 0x10000040
};
#endif

//============================================================
// Local Function predefine.
//============================================================

//START------------COMMON INFO RELATED---------------//
VOID
odm_CommonInfoSelfInit(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_CommonInfoSelfUpdate(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_CmnInfoInit_Debug(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_CmnInfoHook_Debug(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_CmnInfoUpdate_Debug(
	IN		PDM_ODM_T		pDM_Odm
	);
VOID
odm_BasicDbgMessage
(
	IN		PDM_ODM_T		pDM_Odm
	);

//END------------COMMON INFO RELATED---------------//

//START---------------DIG---------------------------//

//Remove by Yuchen

//END---------------DIG---------------------------//

//START-------BB POWER SAVE-----------------------//
//Remove BB power Saving by YuChen
//END---------BB POWER SAVE-----------------------//

//START-----------------PSD-----------------------//
#if(DM_ODM_SUPPORT_TYPE & (ODM_WIN)) 
//============================================================
// Function predefine.
//============================================================

//Remove PathDiversity related funtion predefine to odm_PathDiv.h

//Start-------------------- RX High Power------------------------//
VOID	odm_RXHPInit(	IN		PDM_ODM_T		pDM_Odm);
VOID	odm_RXHP(	IN		PDM_ODM_T		pDM_Odm);
VOID	odm_Write_RXHP(	IN	PDM_ODM_T	pDM_Odm);

VOID	odm_PSD_RXHP(		IN	PDM_ODM_T	pDM_Odm);
VOID	odm_PSD_RXHPCallback(	PRT_TIMER		pTimer);
VOID	odm_PSD_RXHPWorkitemCallback(	IN PVOID            pContext);
//End--------------------- RX High Power -----------------------//

VOID	odm_PathDivInit_92D(	IN	PDM_ODM_T 	pDM_Odm);

VOID
odm_SetRespPath_92C(
	IN	PADAPTER	Adapter,
	IN	u1Byte	DefaultRespPath 
	);

#endif
//END-------------------PSD-----------------------//

VOID
odm_RefreshRateAdaptiveMaskMP(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_RefreshRateAdaptiveMaskCE(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_RefreshRateAdaptiveMaskAPADSL(
	IN		PDM_ODM_T		pDM_Odm
	);

//Remove by YuChen

VOID
odm_RSSIMonitorInit(
	IN	PDM_ODM_T	pDM_Odm
	);

VOID
odm_RSSIMonitorCheckMP(
	IN	PDM_ODM_T	pDM_Odm
	);

VOID 
odm_RSSIMonitorCheckCE(
	IN		PDM_ODM_T		pDM_Odm
	);
VOID 
odm_RSSIMonitorCheckAP(
	IN		PDM_ODM_T		pDM_Odm
	);



VOID
odm_RSSIMonitorCheck(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_SwAntDetectInit(
	IN 		PDM_ODM_T 		pDM_Odm
	);

VOID
odm_AntennaDiversityInit(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_AntennaDiversity(
	IN		PDM_ODM_T		pDM_Odm
	);


#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
VOID
odm_SwAntDivChkAntSwitchCallback(
	PRT_TIMER		pTimer
);
VOID
odm_SwAntDivChkAntSwitchWorkitemCallback(
    IN PVOID            pContext
    );
VOID
ODM_UpdateInitRateWorkItemCallback(
    IN PVOID            pContext
    );
#elif (DM_ODM_SUPPORT_TYPE == ODM_CE)
VOID odm_SwAntDivChkAntSwitchCallback(void *FunctionContext);
#elif (DM_ODM_SUPPORT_TYPE & (ODM_AP|ODM_ADSL))
VOID odm_SwAntDivChkAntSwitchCallback(void *FunctionContext);
#endif



VOID
odm_GlobalAdapterCheck(
	IN		VOID
	);

VOID
odm_RefreshBasicRateMask(
	IN		PDM_ODM_T		pDM_Odm	
	);

VOID
odm_RefreshRateAdaptiveMask(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
ODM_TXPowerTrackingCheck(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_TXPowerTrackingCheckAP(
	IN		PDM_ODM_T		pDM_Odm
	);

VOID
odm_RateAdaptiveMaskInit(
	IN	PDM_ODM_T	pDM_Odm
	);

VOID
odm_TXPowerTrackingThermalMeterInit(
	IN	PDM_ODM_T	pDM_Odm
	);


VOID
odm_IQCalibrate(
		IN	PDM_ODM_T	pDM_Odm 
		);

VOID
odm_TXPowerTrackingInit(
	IN	PDM_ODM_T	pDM_Odm
	);

VOID
odm_TXPowerTrackingCheckMP(
	IN	PDM_ODM_T	pDM_Odm
	);


VOID
odm_TXPowerTrackingCheckCE(
	IN	PDM_ODM_T	pDM_Odm
	);

#if(DM_ODM_SUPPORT_TYPE & (ODM_WIN)) 

VOID
ODM_RateAdaptiveStateApInit(
	IN	PADAPTER	Adapter	,
	IN	PRT_WLAN_STA  pEntry
	);

VOID 
odm_TXPowerTrackingCallbackThermalMeter92C(
            IN PADAPTER	Adapter
            );

VOID
odm_TXPowerTrackingCallbackRXGainThermalMeter92D(
	IN PADAPTER 	Adapter
	);

VOID
odm_TXPowerTrackingCallbackThermalMeter92D(
            IN PADAPTER	Adapter
            );

VOID
odm_TXPowerTrackingDirectCall92C(
            IN	PADAPTER		Adapter
            );

VOID
odm_TXPowerTrackingThermalMeterCheck(
	IN	PADAPTER		Adapter
	);

#endif

//Remove Edca by Yu Chen


#define 	RxDefaultAnt1		0x65a9
#define	RxDefaultAnt2		0x569a

VOID
odm_InitHybridAntDiv(
	IN PDM_ODM_T	pDM_Odm 
	);

BOOLEAN
odm_StaDefAntSel(
	IN PDM_ODM_T	pDM_Odm,
	IN u4Byte		OFDM_Ant1_Cnt,
	IN u4Byte		OFDM_Ant2_Cnt,
	IN u4Byte		CCK_Ant1_Cnt,
	IN u4Byte		CCK_Ant2_Cnt,
	OUT u1Byte		*pDefAnt 
	);

VOID
odm_SetRxIdleAnt(
	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte	Ant,
	IN   BOOLEAN   bDualPath                     
);



VOID
odm_HwAntDiv(
	IN	PDM_ODM_T	pDM_Odm
);


//============================================================
//3 Export Interface
//============================================================

//
// 2011/09/21 MH Add to describe different team necessary resource allocate??
//
VOID
ODM_DMInit(
	IN		PDM_ODM_T		pDM_Odm
	)
{

	odm_CommonInfoSelfInit(pDM_Odm);
	odm_CmnInfoInit_Debug(pDM_Odm);
	odm_DIGInit(pDM_Odm);
	odm_NHMCounterStatisticsInit(pDM_Odm);
	odm_AdaptivityInit(pDM_Odm);
	odm_RateAdaptiveMaskInit(pDM_Odm);
	ODM_CfoTrackingInit(pDM_Odm);
	ODM_EdcaTurboInit(pDM_Odm);
	odm_RSSIMonitorInit(pDM_Odm);
	odm_TXPowerTrackingInit(pDM_Odm);
	odm_AntennaDiversityInit(pDM_Odm);

#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN|ODM_CE))
	ODM_ClearTxPowerTrackingState(pDM_Odm);

	if ( *(pDM_Odm->mp_mode) != 1)
	odm_PathDiversityInit(pDM_Odm);

#endif

	if(pDM_Odm->SupportICType & ODM_IC_11N_SERIES)
	{
		odm_DynamicBBPowerSavingInit(pDM_Odm);
		odm_DynamicTxPowerInit(pDM_Odm);

#if (RTL8188E_SUPPORT == 1)
		if(pDM_Odm->SupportICType==ODM_RTL8188E)
		{
			odm_PrimaryCCA_Init(pDM_Odm);
			ODM_RAInfo_Init_all(pDM_Odm);
		}
#endif

#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN|ODM_CE))
	
	#if (RTL8723B_SUPPORT == 1)
		if(pDM_Odm->SupportICType == ODM_RTL8723B)
			odm_SwAntDetectInit(pDM_Odm);
	#endif

	#if (RTL8192E_SUPPORT == 1)
		if(pDM_Odm->SupportICType==ODM_RTL8192E)
			odm_PrimaryCCA_Check_Init(pDM_Odm);
	#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	#if (RTL8723A_SUPPORT == 1)
		if(pDM_Odm->SupportICType == ODM_RTL8723A)
			odm_PSDMonitorInit(pDM_Odm);
	#endif

	#if (RTL8192D_SUPPORT == 1)
		if(pDM_Odm->SupportICType==ODM_RTL8192D)
			odm_PathDivInit_92D(pDM_Odm);
	#endif

	#if ((RTL8192C_SUPPORT == 1) || (RTL8192D_SUPPORT == 1))
		if(pDM_Odm->SupportICType & (ODM_RTL8192C|ODM_RTL8192D))
			odm_RXHPInit(pDM_Odm);
	#endif
#endif
#endif

	}

}

VOID
ODM_DMReset(
	IN		PDM_ODM_T		pDM_Odm
	)
{
#ifdef CONFIG_HW_ANTENNA_DIVERSITY
	ODM_AntDivReset(pDM_Odm);
#endif // CONFIG_HW_ANTENNA_DIVERSITY
}

//
// 2011/09/20 MH This is the entry pointer for all team to execute HW out source DM.
// You can not add any dummy function here, be care, you can only use DM structure
// to perform any new ODM_DM.
//
VOID
ODM_DMWatchdog(
	IN		PDM_ODM_T		pDM_Odm
	)
{	
	if((pDM_Odm->SupportICType == ODM_RTL8821) && (pDM_Odm->SupportInterface == ODM_ITRF_USB))
	{
		if(pDM_Odm->RSSI_Min > 25)
			ODM_Write1Byte(pDM_Odm, 0x4CF, 0x02);
		else if(pDM_Odm->RSSI_Min < 20)
			ODM_Write1Byte(pDM_Odm, 0x4CF, 0x00);
	}


	odm_CommonInfoSelfUpdate(pDM_Odm);
	odm_BasicDbgMessage(pDM_Odm);
	odm_FalseAlarmCounterStatistics(pDM_Odm);
	odm_NHMCounterStatistics(pDM_Odm);
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_DIG, ODM_DBG_LOUD, ("odm_DIG(): RSSI=0x%x\n",pDM_Odm->RSSI_Min));

	odm_RSSIMonitorCheck(pDM_Odm);

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
//#ifdef CONFIG_PLATFORM_SPRD
	//For CE Platform(SPRD or Tablet)
	//8723A or 8189ES platform
	//NeilChen--2012--08--24--
	//Fix Leave LPS issue
	if( 	(adapter_to_pwrctl(pDM_Odm->Adapter)->pwr_mode != PS_MODE_ACTIVE) // in LPS mode
		//&&( 			
		//	(pDM_Odm->SupportICType & (ODM_RTL8723A ) )||
		//   	(pDM_Odm->SupportICType & (ODM_RTL8188E) &&((pDM_Odm->SupportInterface  == ODM_ITRF_SDIO)) ) 
	  	//)	
	)
	{
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_DIG, ODM_DBG_LOUD, ("----Step1: odm_DIG is in LPS mode\n"));				
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_DIG, ODM_DBG_LOUD, ("---Step2: 8723AS is in LPS mode\n"));
			odm_DIGbyRSSI_LPS(pDM_Odm);
	}		
	else				
//#endif
#endif
	{
		odm_DIG(pDM_Odm);
	}

	{
		pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
		odm_Adaptivity(pDM_Odm, pDM_DigTable->CurIGValue);
	}
	odm_CCKPacketDetectionThresh(pDM_Odm);

	if(*(pDM_Odm->pbPowerSaving)==TRUE)
		return;

	
	odm_RefreshRateAdaptiveMask(pDM_Odm);
	odm_RefreshBasicRateMask(pDM_Odm);
	odm_DynamicBBPowerSaving(pDM_Odm);	
	odm_EdcaTurboCheck(pDM_Odm);
	odm_PathDiversity(pDM_Odm);
	ODM_CfoTracking(pDM_Odm);
	odm_DynamicTxPower(pDM_Odm);	
	odm_AntennaDiversity(pDM_Odm);

#if (RTL8192E_SUPPORT == 1)
        if(pDM_Odm->SupportICType==ODM_RTL8192E)
                odm_DynamicPrimaryCCA_Check(pDM_Odm); 
#endif

	if(pDM_Odm->SupportICType & ODM_IC_11AC_SERIES)
	{
		ODM_TXPowerTrackingCheck(pDM_Odm);

		odm_IQCalibrate(pDM_Odm);
	}
	else if(pDM_Odm->SupportICType & ODM_IC_11N_SERIES)
	{
		ODM_TXPowerTrackingCheck(pDM_Odm);

		//odm_EdcaTurboCheck(pDM_Odm);

		#if( DM_ODM_SUPPORT_TYPE & (ODM_WIN))	
		if(!(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8188E)))
		        odm_RXHP(pDM_Odm);	
		#endif

	//2010.05.30 LukeLee: For CE platform, files in IC subfolders may not be included to be compiled,
	// so compile flags must be left here to prevent from compile errors
#if (RTL8192D_SUPPORT == 1)
	        if(pDM_Odm->SupportICType==ODM_RTL8192D)
	                ODM_DynamicEarlyMode(pDM_Odm);
#endif
	        odm_DynamicBBPowerSaving(pDM_Odm);
#if (RTL8188E_SUPPORT == 1)
	        if(pDM_Odm->SupportICType==ODM_RTL8188E)
	                odm_DynamicPrimaryCCA(pDM_Odm);	
#endif

	}
	pDM_Odm->PhyDbgInfo.NumQryBeaconPkt = 0;

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	odm_dtc(pDM_Odm);
#endif
}


//
// Init /.. Fixed HW value. Only init time.
//
VOID
ODM_CmnInfoInit(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u4Byte			Value	
	)
{
	//
	// This section is used for init value
	//
	switch	(CmnInfo)
	{
		//
		// Fixed ODM value.
		//
		case	ODM_CMNINFO_ABILITY:
			pDM_Odm->SupportAbility = (u4Byte)Value;
			break;

		case	ODM_CMNINFO_RF_TYPE:
			pDM_Odm->RFType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_PLATFORM:
			pDM_Odm->SupportPlatform = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_INTERFACE:
			pDM_Odm->SupportInterface = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_MP_TEST_CHIP:
			pDM_Odm->bIsMPChip= (u1Byte)Value;
			break;
            
		case	ODM_CMNINFO_IC_TYPE:
			pDM_Odm->SupportICType = Value;
			break;

		case	ODM_CMNINFO_CUT_VER:
			pDM_Odm->CutVersion = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_FAB_VER:
			pDM_Odm->FabVersion = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_RFE_TYPE:
			pDM_Odm->RFEType = (u1Byte)Value;
			break;

		case    ODM_CMNINFO_RF_ANTENNA_TYPE:
			pDM_Odm->AntDivType= (u1Byte)Value;
			break;

		case	ODM_CMNINFO_BOARD_TYPE:
			pDM_Odm->BoardType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_PACKAGE_TYPE:
			pDM_Odm->PackageType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_EXT_LNA:
			pDM_Odm->ExtLNA = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_5G_EXT_LNA:
			pDM_Odm->ExtLNA5G = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_EXT_PA:
			pDM_Odm->ExtPA = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_5G_EXT_PA:
			pDM_Odm->ExtPA5G = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_GPA:
			pDM_Odm->TypeGPA= (ODM_TYPE_GPA_E)Value;
			break;
		case	ODM_CMNINFO_APA:
			pDM_Odm->TypeAPA= (ODM_TYPE_APA_E)Value;
			break;
		case	ODM_CMNINFO_GLNA:
			pDM_Odm->TypeGLNA= (ODM_TYPE_GLNA_E)Value;
			break;
		case	ODM_CMNINFO_ALNA:
			pDM_Odm->TypeALNA= (ODM_TYPE_ALNA_E)Value;
			break;

		case	ODM_CMNINFO_EXT_TRSW:
			pDM_Odm->ExtTRSW = (u1Byte)Value;
			break;
		case 	ODM_CMNINFO_PATCH_ID:
			pDM_Odm->PatchID = (u1Byte)Value;
			break;
		case 	ODM_CMNINFO_BINHCT_TEST:
			pDM_Odm->bInHctTest = (BOOLEAN)Value;
			break;
		case 	ODM_CMNINFO_BWIFI_TEST:
			pDM_Odm->bWIFITest = (BOOLEAN)Value;
			break;	

		case	ODM_CMNINFO_SMART_CONCURRENT:
			pDM_Odm->bDualMacSmartConcurrent = (BOOLEAN )Value;
			break;
		
		//To remove the compiler warning, must add an empty default statement to handle the other values.	
		default:
			//do nothing
			break;	
		
	}

}


VOID
ODM_CmnInfoHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		PVOID			pValue	
	)
{
	//
	// Hook call by reference pointer.
	//
	switch	(CmnInfo)
	{
		//
		// Dynamic call by reference pointer.
		//
		case	ODM_CMNINFO_MAC_PHY_MODE:
			pDM_Odm->pMacPhyMode = (u1Byte *)pValue;
			break;
		
		case	ODM_CMNINFO_TX_UNI:
			pDM_Odm->pNumTxBytesUnicast = (u8Byte *)pValue;
			break;

		case	ODM_CMNINFO_RX_UNI:
			pDM_Odm->pNumRxBytesUnicast = (u8Byte *)pValue;
			break;

		case	ODM_CMNINFO_WM_MODE:
			pDM_Odm->pWirelessMode = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_BAND:
			pDM_Odm->pBandType = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_SEC_CHNL_OFFSET:
			pDM_Odm->pSecChOffset = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_SEC_MODE:
			pDM_Odm->pSecurity = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_BW:
			pDM_Odm->pBandWidth = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_CHNL:
			pDM_Odm->pChannel = (u1Byte *)pValue;
			break;
		
		case	ODM_CMNINFO_DMSP_GET_VALUE:
			pDM_Odm->pbGetValueFromOtherMac = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_BUDDY_ADAPTOR:
			pDM_Odm->pBuddyAdapter = (PADAPTER *)pValue;
			break;

		case	ODM_CMNINFO_DMSP_IS_MASTER:
			pDM_Odm->pbMasterOfDMSP = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_SCAN:
			pDM_Odm->pbScanInProcess = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_POWER_SAVING:
			pDM_Odm->pbPowerSaving = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_ONE_PATH_CCA:
			pDM_Odm->pOnePathCCA = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_DRV_STOP:
			pDM_Odm->pbDriverStopped =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_PNP_IN:
			pDM_Odm->pbDriverIsGoingToPnpSetPowerSleep =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_INIT_ON:
			pDM_Odm->pinit_adpt_in_progress =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_ANT_TEST:
			pDM_Odm->pAntennaTest =  (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_NET_CLOSED:
			pDM_Odm->pbNet_closed = (BOOLEAN *)pValue;
			break;

		case 	ODM_CMNINFO_FORCED_RATE:
			pDM_Odm->pForcedDataRate = (pu2Byte)pValue;
			break;

		case  ODM_CMNINFO_FORCED_IGI_LB:
			pDM_Odm->pu1ForcedIgiLb = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_MP_MODE:
			pDM_Odm->mp_mode = (u1Byte *)pValue;
			break;

		//case	ODM_CMNINFO_RTSTA_AID:
		//	pDM_Odm->pAidMap =  (u1Byte *)pValue;
		//	break;

		//case	ODM_CMNINFO_BT_COEXIST:
		//	pDM_Odm->BTCoexist = (BOOLEAN *)pValue;		

		//case	ODM_CMNINFO_STA_STATUS:
			//pDM_Odm->pODM_StaInfo[] = (PSTA_INFO_T)pValue;
			//break;

		//case	ODM_CMNINFO_PHY_STATUS:
		//	pDM_Odm->pPhyInfo = (ODM_PHY_INFO *)pValue;
		//	break;

		//case	ODM_CMNINFO_MAC_STATUS:
		//	pDM_Odm->pMacInfo = (ODM_MAC_INFO *)pValue;
		//	break;
		//To remove the compiler warning, must add an empty default statement to handle the other values.				
		default:
			//do nothing
			break;

	}

}


VOID
ODM_CmnInfoPtrArrayHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u2Byte			Index,
	IN		PVOID			pValue	
	)
{
	//
	// Hook call by reference pointer.
	//
	switch	(CmnInfo)
	{
		//
		// Dynamic call by reference pointer.
		//		
		case	ODM_CMNINFO_STA_STATUS:
			pDM_Odm->pODM_StaInfo[Index] = (PSTA_INFO_T)pValue;
			break;		
		//To remove the compiler warning, must add an empty default statement to handle the other values.				
		default:
			//do nothing
			break;
	}
	
}


//
// Update Band/CHannel/.. The values are dynamic but non-per-packet.
//
VOID
ODM_CmnInfoUpdate(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u4Byte			CmnInfo,
	IN		u8Byte			Value	
	)
{
	//
	// This init variable may be changed in run time.
	//
	switch	(CmnInfo)
	{
		case ODM_CMNINFO_LINK_IN_PROGRESS:
			pDM_Odm->bLinkInProcess = (BOOLEAN)Value;
			break;
		
		case	ODM_CMNINFO_ABILITY:
			pDM_Odm->SupportAbility = (u4Byte)Value;
			break;

		case	ODM_CMNINFO_RF_TYPE:
			pDM_Odm->RFType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_WIFI_DIRECT:
			pDM_Odm->bWIFI_Direct = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_WIFI_DISPLAY:
			pDM_Odm->bWIFI_Display = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_LINK:
			pDM_Odm->bLinked = (BOOLEAN)Value;
			break;
			
		case	ODM_CMNINFO_STATION_STATE:
			pDM_Odm->bsta_state = (BOOLEAN)Value;
			break;
			
		case	ODM_CMNINFO_RSSI_MIN:
			pDM_Odm->RSSI_Min= (u1Byte)Value;
			break;

		case	ODM_CMNINFO_DBG_COMP:
			pDM_Odm->DebugComponents = Value;
			break;

		case	ODM_CMNINFO_DBG_LEVEL:
			pDM_Odm->DebugLevel = (u4Byte)Value;
			break;
		case	ODM_CMNINFO_RA_THRESHOLD_HIGH:
			pDM_Odm->RateAdaptive.HighRSSIThresh = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_RA_THRESHOLD_LOW:
			pDM_Odm->RateAdaptive.LowRSSIThresh = (u1Byte)Value;
			break;
		// The following is for BT HS mode and BT coexist mechanism.
		case ODM_CMNINFO_BT_ENABLED:
			pDM_Odm->bBtEnabled = (BOOLEAN)Value;
			break;
			
		case ODM_CMNINFO_BT_HS_CONNECT_PROCESS:
			pDM_Odm->bBtConnectProcess = (BOOLEAN)Value;
			break;
		
		case ODM_CMNINFO_BT_HS_RSSI:
			pDM_Odm->btHsRssi = (u1Byte)Value;
			break;
			
		case	ODM_CMNINFO_BT_OPERATION:
			pDM_Odm->bBtHsOperation = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_BT_LIMITED_DIG:
			pDM_Odm->bBtLimitedDig = (BOOLEAN)Value;
			break;	

		case	ODM_CMNINFO_BT_DISABLE_EDCA:
			pDM_Odm->bBtDisableEdcaTurbo = (BOOLEAN)Value;
			break;
			
/*
		case	ODM_CMNINFO_OP_MODE:
			pDM_Odm->OPMode = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_WM_MODE:
			pDM_Odm->WirelessMode = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_BAND:
			pDM_Odm->BandType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_SEC_CHNL_OFFSET:
			pDM_Odm->SecChOffset = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_SEC_MODE:
			pDM_Odm->Security = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_BW:
			pDM_Odm->BandWidth = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_CHNL:
			pDM_Odm->Channel = (u1Byte)Value;
			break;			
*/	
                default:
			//do nothing
			break;
	}

	
}

VOID
odm_CommonInfoSelfInit(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	pFAT_T			pDM_FatTable = &pDM_Odm->DM_FatTable;
	pDM_Odm->bCckHighPower = (BOOLEAN) ODM_GetBBReg(pDM_Odm, ODM_REG(CCK_RPT_FORMAT,pDM_Odm), ODM_BIT(CCK_RPT_FORMAT,pDM_Odm));		
	pDM_Odm->RFPathRxEnable = (u1Byte) ODM_GetBBReg(pDM_Odm, ODM_REG(BB_RX_PATH,pDM_Odm), ODM_BIT(BB_RX_PATH,pDM_Odm));
#if (DM_ODM_SUPPORT_TYPE != ODM_CE)	
	pDM_Odm->pbNet_closed = &pDM_Odm->BOOLEAN_temp;
#endif

	ODM_InitDebugSetting(pDM_Odm);

	pDM_Odm->TxRate = 0xFF;
}

VOID
odm_CommonInfoSelfUpdate(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u1Byte	EntryCnt=0;
	u1Byte	i;
	PSTA_INFO_T   	pEntry;

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

	PADAPTER	Adapter =  pDM_Odm->Adapter;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	pEntry = pDM_Odm->pODM_StaInfo[0];
	if(pMgntInfo->mAssoc)
	{
		pEntry->bUsed=TRUE;
		for (i=0; i<6; i++)
			pEntry->MacAddr[i] = pMgntInfo->Bssid[i];
	}
	else
	{
		pEntry->bUsed=FALSE;
		for (i=0; i<6; i++)
			pEntry->MacAddr[i] = 0;
	}
#endif


	if(*(pDM_Odm->pBandWidth) == ODM_BW40M)
	{
		if(*(pDM_Odm->pSecChOffset) == 1)
			pDM_Odm->ControlChannel = *(pDM_Odm->pChannel) -2;
		else if(*(pDM_Odm->pSecChOffset) == 2)
			pDM_Odm->ControlChannel = *(pDM_Odm->pChannel) +2;
	}
	else
		pDM_Odm->ControlChannel = *(pDM_Odm->pChannel);

	for (i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = pDM_Odm->pODM_StaInfo[i];
		if(IS_STA_VALID(pEntry))
			EntryCnt++;
	}
	if(EntryCnt == 1)
		pDM_Odm->bOneEntryOnly = TRUE;
	else
		pDM_Odm->bOneEntryOnly = FALSE;
}

VOID
odm_CmnInfoInit_Debug(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("odm_CmnInfoInit_Debug==>\n"));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("SupportPlatform=%d\n",pDM_Odm->SupportPlatform) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("SupportAbility=0x%x\n",pDM_Odm->SupportAbility) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("SupportInterface=%d\n",pDM_Odm->SupportInterface) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("SupportICType=0x%x\n",pDM_Odm->SupportICType) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("CutVersion=%d\n",pDM_Odm->CutVersion) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("FabVersion=%d\n",pDM_Odm->FabVersion) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("RFType=%d\n",pDM_Odm->RFType) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("BoardType=%d\n",pDM_Odm->BoardType) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("ExtLNA=%d\n",pDM_Odm->ExtLNA) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("ExtPA=%d\n",pDM_Odm->ExtPA) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("ExtTRSW=%d\n",pDM_Odm->ExtTRSW) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("PatchID=%d\n",pDM_Odm->PatchID) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bInHctTest=%d\n",pDM_Odm->bInHctTest) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bWIFITest=%d\n",pDM_Odm->bWIFITest) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bDualMacSmartConcurrent=%d\n",pDM_Odm->bDualMacSmartConcurrent) );

}

VOID
odm_CmnInfoHook_Debug(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("odm_CmnInfoHook_Debug==>\n"));	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pNumTxBytesUnicast=%llu\n",*(pDM_Odm->pNumTxBytesUnicast)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pNumRxBytesUnicast=%llu\n",*(pDM_Odm->pNumRxBytesUnicast)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pWirelessMode=0x%x\n",*(pDM_Odm->pWirelessMode)) );	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pSecChOffset=%d\n",*(pDM_Odm->pSecChOffset)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pSecurity=%d\n",*(pDM_Odm->pSecurity)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pBandWidth=%d\n",*(pDM_Odm->pBandWidth)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pChannel=%d\n",*(pDM_Odm->pChannel)) );

	if(pDM_Odm->SupportICType==ODM_RTL8192D)
	{
		if(pDM_Odm->pBandType)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pBandType=%d\n",*(pDM_Odm->pBandType)) );
		if(pDM_Odm->pMacPhyMode)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pMacPhyMode=%d\n",*(pDM_Odm->pMacPhyMode)) );
		if(pDM_Odm->pBuddyAdapter)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pbGetValueFromOtherMac=%d\n",*(pDM_Odm->pbGetValueFromOtherMac)) );
		if(pDM_Odm->pBuddyAdapter)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pBuddyAdapter=%p\n",*(pDM_Odm->pBuddyAdapter)) );
		if(pDM_Odm->pbMasterOfDMSP)
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pbMasterOfDMSP=%d\n",*(pDM_Odm->pbMasterOfDMSP)) );
	}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pbScanInProcess=%d\n",*(pDM_Odm->pbScanInProcess)) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pbPowerSaving=%d\n",*(pDM_Odm->pbPowerSaving)) );

	if(pDM_Odm->SupportPlatform & (ODM_AP|ODM_ADSL))
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("pOnePathCCA=%d\n",*(pDM_Odm->pOnePathCCA)) );
}

VOID
odm_CmnInfoUpdate_Debug(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("odm_CmnInfoUpdate_Debug==>\n"));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bWIFI_Direct=%d\n",pDM_Odm->bWIFI_Direct) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bWIFI_Display=%d\n",pDM_Odm->bWIFI_Display) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bLinked=%d\n",pDM_Odm->bLinked) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("RSSI_Min=%d\n",pDM_Odm->RSSI_Min) );
}

VOID
odm_BasicDbgMessage
(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pDM_Odm->FalseAlmCnt);
	pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
	
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("odm_BasicDbgMsg==>\n"));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("bLinked = %d, RSSI_Min = %d, CurrentIGI = 0x%x \n",
		pDM_Odm->bLinked, pDM_Odm->RSSI_Min, pDM_DigTable->CurIGValue) );
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("Cnt_Cck_fail = %d, Cnt_Ofdm_fail = %d, Total False Alarm = %d\n",	
		FalseAlmCnt->Cnt_Cck_fail, FalseAlmCnt->Cnt_Ofdm_fail, FalseAlmCnt->Cnt_all));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("RxRate = 0x%x, RSSI_A = %d, RSSI_B = %d\n", 
		pDM_Odm->RxRate, pDM_Odm->RSSI_A, pDM_Odm->RSSI_B));
	//ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("RSSI_C = %d, RSSI_D = %d\n", pDM_Odm->RSSI_C, pDM_Odm->RSSI_D));

}

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
VOID
ODM_InitAllWorkItems(IN PDM_ODM_T	pDM_Odm )
{
#if USE_WORKITEM
	PADAPTER		pAdapter = pDM_Odm->Adapter;

	ODM_InitializeWorkItem(	pDM_Odm, 
							&pDM_Odm->DM_SWAT_Table.SwAntennaSwitchWorkitem_8723B, 
							(RT_WORKITEM_CALL_BACK)ODM_SW_AntDiv_WorkitemCallback,
							(PVOID)pAdapter,
							"AntennaSwitchWorkitem");
	
	ODM_InitializeWorkItem(	pDM_Odm, 
							&pDM_Odm->DM_SWAT_Table.SwAntennaSwitchWorkitem, 
							(RT_WORKITEM_CALL_BACK)odm_SwAntDivChkAntSwitchWorkitemCallback,
							(PVOID)pAdapter,
							"AntennaSwitchWorkitem");
	

	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->PathDivSwitchWorkitem), 
		(RT_WORKITEM_CALL_BACK)odm_PathDivChkAntSwitchWorkitemCallback, 
		(PVOID)pAdapter,
		"SWAS_WorkItem");

	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->CCKPathDiversityWorkitem), 
		(RT_WORKITEM_CALL_BACK)odm_CCKTXPathDiversityWorkItemCallback, 
		(PVOID)pAdapter,
		"CCKTXPathDiversityWorkItem");

	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->MPT_DIGWorkitem), 
		(RT_WORKITEM_CALL_BACK)odm_MPT_DIGWorkItemCallback, 
		(PVOID)pAdapter,
		"MPT_DIGWorkitem");

	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->RaRptWorkitem), 
		(RT_WORKITEM_CALL_BACK)ODM_UpdateInitRateWorkItemCallback, 
		(PVOID)pAdapter,
		"RaRptWorkitem");
	
#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
#if (RTL8188E_SUPPORT == 1)
	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->FastAntTrainingWorkitem), 
		(RT_WORKITEM_CALL_BACK)odm_FastAntTrainingWorkItemCallback, 
		(PVOID)pAdapter,
		"FastAntTrainingWorkitem");
#endif
#endif
	ODM_InitializeWorkItem(
		pDM_Odm,
		&(pDM_Odm->DM_RXHP_Table.PSDTimeWorkitem), 
		(RT_WORKITEM_CALL_BACK)odm_PSD_RXHPWorkitemCallback, 
		(PVOID)pAdapter,
		"PSDRXHP_WorkItem");  
#endif
}

VOID
ODM_FreeAllWorkItems(IN PDM_ODM_T	pDM_Odm )
{
#if USE_WORKITEM
	ODM_FreeWorkItem(	&(pDM_Odm->DM_SWAT_Table.SwAntennaSwitchWorkitem_8723B));
	
	ODM_FreeWorkItem(	&(pDM_Odm->DM_SWAT_Table.SwAntennaSwitchWorkitem));

	ODM_FreeWorkItem(&(pDM_Odm->PathDivSwitchWorkitem));      

	ODM_FreeWorkItem(&(pDM_Odm->CCKPathDiversityWorkitem));
	
	ODM_FreeWorkItem(&(pDM_Odm->FastAntTrainingWorkitem));

	ODM_FreeWorkItem(&(pDM_Odm->MPT_DIGWorkitem));

	ODM_FreeWorkItem(&(pDM_Odm->RaRptWorkitem));

	ODM_FreeWorkItem((&pDM_Odm->DM_RXHP_Table.PSDTimeWorkitem));
#endif

}
#endif

/*
VOID
odm_FindMinimumRSSI(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u4Byte	i;
	u1Byte	RSSI_Min = 0xFF;

	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
//		if(pDM_Odm->pODM_StaInfo[i] != NULL)
		if(IS_STA_VALID(pDM_Odm->pODM_StaInfo[i]) )
		{
			if(pDM_Odm->pODM_StaInfo[i]->RSSI_Ave < RSSI_Min)
			{
				RSSI_Min = pDM_Odm->pODM_StaInfo[i]->RSSI_Ave;
			}
		}
	}

	pDM_Odm->RSSI_Min = RSSI_Min;

}

VOID
odm_IsLinked(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u4Byte i;
	BOOLEAN Linked = FALSE;
	
	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
			if(IS_STA_VALID(pDM_Odm->pODM_StaInfo[i]) )
			{			
				Linked = TRUE;
				break;
			}
		
	}

	pDM_Odm->bLinked = Linked;
}
*/


//3============================================================
//3 DIG
//3============================================================
/*-----------------------------------------------------------------------------
 * Function:	odm_DIGInit()
 *
 * Overview:	Set DIG scheme init value.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *
 *---------------------------------------------------------------------------*/

//Remove DIG by yuchen

//Remove DIG and FA check by Yu Chen


//3============================================================
//3 BB Power Save
//3============================================================

//Remove BB power saving by Yuchen

//3============================================================
//3 RATR MASK
//3============================================================
//3============================================================
//3 Rate Adaptive
//3============================================================

VOID
odm_RateAdaptiveMaskInit(
	IN 	PDM_ODM_T	pDM_Odm
	)
{
	PODM_RATE_ADAPTIVE	pOdmRA = &pDM_Odm->RateAdaptive;

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PMGNT_INFO		pMgntInfo = &pDM_Odm->Adapter->MgntInfo;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pDM_Odm->Adapter);

	pMgntInfo->Ratr_State = DM_RATR_STA_INIT;

	if (pMgntInfo->DM_Type == DM_Type_ByDriver)
		pHalData->bUseRAMask = TRUE;
	else
		pHalData->bUseRAMask = FALSE;	

#elif (DM_ODM_SUPPORT_TYPE == ODM_CE)
	pOdmRA->Type = DM_Type_ByDriver;
	if (pOdmRA->Type == DM_Type_ByDriver)
		pDM_Odm->bUseRAMask = _TRUE;
	else
		pDM_Odm->bUseRAMask = _FALSE;	
#endif

	pOdmRA->RATRState = DM_RATR_STA_INIT;
	pOdmRA->LdpcThres = 35;
	pOdmRA->bUseLdpc = FALSE;
	pOdmRA->HighRSSIThresh = 50;
	pOdmRA->LowRSSIThresh = 20;
}

#if (DM_ODM_SUPPORT_TYPE & ODM_WIN) 
VOID
ODM_RateAdaptiveStateApInit(	
	IN	PADAPTER		Adapter	,
	IN	PRT_WLAN_STA  	pEntry
	)
{
	pEntry->Ratr_State = DM_RATR_STA_INIT;
}
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
u4Byte ODM_Get_Rate_Bitmap(
	IN	PDM_ODM_T	pDM_Odm,	
	IN	u4Byte		macid,
	IN	u4Byte 		ra_mask,	
	IN	u1Byte 		rssi_level)
{
	PSTA_INFO_T   	pEntry;
	u4Byte 	rate_bitmap = 0;
	u1Byte 	WirelessMode;
	//u1Byte 	WirelessMode =*(pDM_Odm->pWirelessMode);
	
	
	pEntry = pDM_Odm->pODM_StaInfo[macid];
	if(!IS_STA_VALID(pEntry))
		return ra_mask;

	WirelessMode = pEntry->wireless_mode;
	
	switch(WirelessMode)
	{
		case ODM_WM_B:
			if(ra_mask & 0x0000000c)		//11M or 5.5M enable				
				rate_bitmap = 0x0000000d;
			else
				rate_bitmap = 0x0000000f;
			break;
			
		case (ODM_WM_G):
		case (ODM_WM_A):
			if(rssi_level == DM_RATR_STA_HIGH)
				rate_bitmap = 0x00000f00;
			else
				rate_bitmap = 0x00000ff0;
			break;
			
		case (ODM_WM_B|ODM_WM_G):
			if(rssi_level == DM_RATR_STA_HIGH)
				rate_bitmap = 0x00000f00;
			else if(rssi_level == DM_RATR_STA_MIDDLE)
				rate_bitmap = 0x00000ff0;
			else
				rate_bitmap = 0x00000ff5;
			break;		

		case (ODM_WM_B|ODM_WM_G|ODM_WM_N24G)	:
		case (ODM_WM_B|ODM_WM_N24G)	:
		case (ODM_WM_G|ODM_WM_N24G)	:
		case (ODM_WM_A|ODM_WM_N5G)	:
			{					
				if (	pDM_Odm->RFType == ODM_1T2R ||pDM_Odm->RFType == ODM_1T1R)
				{
					if(rssi_level == DM_RATR_STA_HIGH)
					{
						rate_bitmap = 0x000f0000;
					}
					else if(rssi_level == DM_RATR_STA_MIDDLE)
					{
						rate_bitmap = 0x000ff000;
					}
					else{
						if (*(pDM_Odm->pBandWidth) == ODM_BW40M)
							rate_bitmap = 0x000ff015;
						else
							rate_bitmap = 0x000ff005;
					}				
				}
				else
				{
					if(rssi_level == DM_RATR_STA_HIGH)
					{		
						rate_bitmap = 0x0f8f0000;
					}
					else if(rssi_level == DM_RATR_STA_MIDDLE)
					{
						rate_bitmap = 0x0f8ff000;
					}
					else
					{
						if (*(pDM_Odm->pBandWidth) == ODM_BW40M)
							rate_bitmap = 0x0f8ff015;
						else
							rate_bitmap = 0x0f8ff005;
					}					
				}
			}
			break;

		case (ODM_WM_AC|ODM_WM_G):
			if(rssi_level == 1)
				rate_bitmap = 0xfc3f0000;
			else if(rssi_level == 2)
				rate_bitmap = 0xfffff000;
			else
				rate_bitmap = 0xffffffff;
			break;

		case (ODM_WM_AC|ODM_WM_A):

			if (pDM_Odm->RFType == RF_1T1R)
			{
				if(rssi_level == 1)				// add by Gary for ac-series
					rate_bitmap = 0x003f8000;
				else if (rssi_level == 2)
					rate_bitmap = 0x003ff000;
				else
					rate_bitmap = 0x003ff010;
			}
			else
			{
				if(rssi_level == 1)				// add by Gary for ac-series
					rate_bitmap = 0xfe3f8000;       // VHT 2SS MCS3~9
				else if (rssi_level == 2)
					rate_bitmap = 0xfffff000;       // VHT 2SS MCS0~9
				else
					rate_bitmap = 0xfffff010;       // All
			}
			break;
			
		default:
			if(pDM_Odm->RFType == RF_1T2R)
				rate_bitmap = 0x000fffff;
			else
				rate_bitmap = 0x0fffffff;
			break;	

	}

	//printk("%s ==> rssi_level:0x%02x, WirelessMode:0x%02x, rate_bitmap:0x%08x \n",__FUNCTION__,rssi_level,WirelessMode,rate_bitmap);
	ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, (" ==> rssi_level:0x%02x, WirelessMode:0x%02x, rate_bitmap:0x%08x \n",rssi_level,WirelessMode,rate_bitmap));

	return (ra_mask&rate_bitmap);
	
}	
#endif


VOID
odm_RefreshBasicRateMask(
	IN		PDM_ODM_T		pDM_Odm	
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER		Adapter	 =  pDM_Odm->Adapter;
	static u1Byte		Stage = 0;
	u1Byte			CurStage = 0;
	OCTET_STRING 	osRateSet;
	PMGNT_INFO		pMgntInfo = GetDefaultMgntInfo(Adapter);
	u1Byte 			RateSet[5] = {MGN_1M, MGN_2M, MGN_5_5M, MGN_11M, MGN_6M};

	if(pDM_Odm->SupportICType != ODM_RTL8812 && pDM_Odm->SupportICType != ODM_RTL8821 )
		return;

	if(pDM_Odm->bLinked == FALSE)	// unlink Default port information
		CurStage = 0;	
	else if(pDM_Odm->RSSI_Min < 40)	// link RSSI  < 40%
		CurStage = 1;
	else if(pDM_Odm->RSSI_Min > 45)	// link RSSI > 45%
		CurStage = 3;	
	else
		CurStage = 2;					// link  25% <= RSSI <= 30%

	if(CurStage != Stage)
	{
		if(CurStage == 1)
		{
			FillOctetString(osRateSet, RateSet, 5);
			FilterSupportRate(pMgntInfo->mBrates, &osRateSet, FALSE);
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_BASIC_RATE, (pu1Byte)&osRateSet);
		}
		else if(CurStage == 3 && (Stage == 1 || Stage == 2))
		{
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BASIC_RATE, (pu1Byte)(&pMgntInfo->mBrates) );
		}
	}
	
	Stage = CurStage;
#endif
}

/*-----------------------------------------------------------------------------
 * Function:	odm_RefreshRateAdaptiveMask()
 *
 * Overview:	Update rate table mask according to rssi
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/27/2009	hpfan	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
VOID
odm_RefreshRateAdaptiveMask(
	IN		PDM_ODM_T		pDM_Odm
	)
{

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_TRACE, ("odm_RefreshRateAdaptiveMask()---------->\n"));	
	if (!(pDM_Odm->SupportAbility & ODM_BB_RA_MASK))
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_TRACE, ("odm_RefreshRateAdaptiveMask(): Return cos not supported\n"));
		return;	
	}
	//
	// 2011/09/29 MH In HW integration first stage, we provide 4 different handle to operate
	// at the same time. In the stage2/3, we need to prive universal interface and merge all
	// HW dynamic mechanism.
	//
	switch	(pDM_Odm->SupportPlatform)
	{
		case	ODM_WIN:
			odm_RefreshRateAdaptiveMaskMP(pDM_Odm);
			break;

		case	ODM_CE:
			odm_RefreshRateAdaptiveMaskCE(pDM_Odm);
			break;

		case	ODM_AP:
		case	ODM_ADSL:
			odm_RefreshRateAdaptiveMaskAPADSL(pDM_Odm);
			break;
	}
	
}

VOID
odm_RefreshRateAdaptiveMaskMP(
	IN		PDM_ODM_T		pDM_Odm	
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER				pAdapter	 =  pDM_Odm->Adapter;
	PADAPTER 				pTargetAdapter = NULL;
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(pAdapter);
	PMGNT_INFO				pMgntInfo = GetDefaultMgntInfo(pAdapter);
	PODM_RATE_ADAPTIVE		pRA = &pDM_Odm->RateAdaptive;

	if(pAdapter->bDriverStopped)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_TRACE, ("<---- odm_RefreshRateAdaptiveMask(): driver is going to unload\n"));
		return;
	}

	if(!pHalData->bUseRAMask)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("<---- odm_RefreshRateAdaptiveMask(): driver does not control rate adaptive mask\n"));
		return;
	}

	// if default port is connected, update RA table for default port (infrastructure mode only)
	if(pMgntInfo->mAssoc && (!ACTING_AS_AP(pAdapter)))
	{
	
		if(pHalData->UndecoratedSmoothedPWDB < pRA->LdpcThres)
		{
			pRA->bUseLdpc = TRUE;
			pRA->bLowerRtsRate = TRUE;
			if((pDM_Odm->SupportICType == ODM_RTL8821) && (pDM_Odm->CutVersion == ODM_CUT_A))
				MgntSet_TX_LDPC(pAdapter,0,TRUE);
			//DbgPrint("RSSI=%d, bUseLdpc = TRUE\n", pHalData->UndecoratedSmoothedPWDB);
		}
		else if(pHalData->UndecoratedSmoothedPWDB > (pRA->LdpcThres-5))
		{
			pRA->bUseLdpc = FALSE;
			pRA->bLowerRtsRate = FALSE;
			if((pDM_Odm->SupportICType == ODM_RTL8821) && (pDM_Odm->CutVersion == ODM_CUT_A))
				MgntSet_TX_LDPC(pAdapter,0,FALSE);
			//DbgPrint("RSSI=%d, bUseLdpc = FALSE\n", pHalData->UndecoratedSmoothedPWDB);
		}
	
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("odm_RefreshRateAdaptiveMask(): Infrasture Mode\n"));
		if( ODM_RAStateCheck(pDM_Odm, pHalData->UndecoratedSmoothedPWDB, pMgntInfo->bSetTXPowerTrainingByOid, &pMgntInfo->Ratr_State) )
		{
			ODM_PRINT_ADDR(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("Target AP addr : "), pMgntInfo->Bssid);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("RSSI:%d, RSSI_LEVEL:%d\n", pHalData->UndecoratedSmoothedPWDB, pMgntInfo->Ratr_State));
			pAdapter->HalFunc.UpdateHalRAMaskHandler(pAdapter, pMgntInfo->mMacId, NULL, pMgntInfo->Ratr_State);
		}
	}

	//
	// The following part configure AP/VWifi/IBSS rate adaptive mask.
	//

	if(pMgntInfo->mIbss) 	// Target: AP/IBSS peer.
		pTargetAdapter = GetDefaultAdapter(pAdapter);
	else
		pTargetAdapter = GetFirstAPAdapter(pAdapter);

	// if extension port (softap) is started, updaet RA table for more than one clients associate
	if(pTargetAdapter != NULL)
	{
		int	i;
		PRT_WLAN_STA	pEntry;

		for(i = 0; i < ODM_ASSOCIATE_ENTRY_NUM; i++)
		{
			pEntry = AsocEntry_EnumStation(pTargetAdapter, i);
			if(NULL != pEntry)
			{
				if(pEntry->bAssociated)
				{
					if(ODM_RAStateCheck(pDM_Odm, pEntry->rssi_stat.UndecoratedSmoothedPWDB, pMgntInfo->bSetTXPowerTrainingByOid, &pEntry->Ratr_State) )
					{
						ODM_PRINT_ADDR(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("Target STA addr : "), pEntry->MacAddr);
						ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("RSSI:%d, RSSI_LEVEL:%d\n", pEntry->rssi_stat.UndecoratedSmoothedPWDB, pEntry->Ratr_State));
						pAdapter->HalFunc.UpdateHalRAMaskHandler(pTargetAdapter, pEntry->AssociatedMacId, pEntry, pEntry->Ratr_State);
					}
				}
			}
		}
	}

	if(pMgntInfo->bSetTXPowerTrainingByOid)
		pMgntInfo->bSetTXPowerTrainingByOid = FALSE;	
#endif	// #if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
}


VOID
odm_RefreshRateAdaptiveMaskCE(
	IN		PDM_ODM_T		pDM_Odm	
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	u1Byte	i;
	PADAPTER	pAdapter	 =  pDM_Odm->Adapter;
	PODM_RATE_ADAPTIVE		pRA = &pDM_Odm->RateAdaptive;

	if(pAdapter->bDriverStopped)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_TRACE, ("<---- odm_RefreshRateAdaptiveMask(): driver is going to unload\n"));
		return;
	}

	if(!pDM_Odm->bUseRAMask)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("<---- odm_RefreshRateAdaptiveMask(): driver does not control rate adaptive mask\n"));
		return;
	}

	//printk("==> %s \n",__FUNCTION__);

	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++){
		PSTA_INFO_T pstat = pDM_Odm->pODM_StaInfo[i];
		if(IS_STA_VALID(pstat) ) {
			if(IS_MCAST( pstat->hwaddr))  //if(psta->mac_id ==1)
				 continue;
			if(IS_MCAST( pstat->hwaddr))
				continue;

			#if((RTL8812A_SUPPORT==1)||(RTL8821A_SUPPORT==1))
			if((pDM_Odm->SupportICType == ODM_RTL8812)||(pDM_Odm->SupportICType == ODM_RTL8821))
			{
				if(pstat->rssi_stat.UndecoratedSmoothedPWDB < pRA->LdpcThres)
				{
					pRA->bUseLdpc = TRUE;
					pRA->bLowerRtsRate = TRUE;
					if((pDM_Odm->SupportICType == ODM_RTL8821) && (pDM_Odm->CutVersion == ODM_CUT_A))
						Set_RA_LDPC_8812(pstat, TRUE);
					//DbgPrint("RSSI=%d, bUseLdpc = TRUE\n", pHalData->UndecoratedSmoothedPWDB);
				}
				else if(pstat->rssi_stat.UndecoratedSmoothedPWDB > (pRA->LdpcThres-5))
				{
					pRA->bUseLdpc = FALSE;
					pRA->bLowerRtsRate = FALSE;
					if((pDM_Odm->SupportICType == ODM_RTL8821) && (pDM_Odm->CutVersion == ODM_CUT_A))
						Set_RA_LDPC_8812(pstat, FALSE);
					//DbgPrint("RSSI=%d, bUseLdpc = FALSE\n", pHalData->UndecoratedSmoothedPWDB);
				}
			}
			#endif

			if( TRUE == ODM_RAStateCheck(pDM_Odm, pstat->rssi_stat.UndecoratedSmoothedPWDB, FALSE , &pstat->rssi_level) )
			{
				ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("RSSI:%d, RSSI_LEVEL:%d\n", pstat->rssi_stat.UndecoratedSmoothedPWDB, pstat->rssi_level));
				//printk("RSSI:%d, RSSI_LEVEL:%d\n", pstat->rssi_stat.UndecoratedSmoothedPWDB, pstat->rssi_level);
				rtw_hal_update_ra_mask(pstat, pstat->rssi_level);
			}
		
		}
	}			
	
#endif
}

VOID
odm_RefreshRateAdaptiveMaskAPADSL(
	IN		PDM_ODM_T		pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE & (ODM_AP|ODM_ADSL))
	struct rtl8192cd_priv *priv = pDM_Odm->priv;
	struct stat_info	*pstat;

	if (!priv->pmib->dot11StationConfigEntry.autoRate) 
		return;

	if (list_empty(&priv->asoc_list))
		return;

	list_for_each_entry(pstat, &priv->asoc_list, asoc_list) {
		if(ODM_RAStateCheck(pDM_Odm, (s4Byte)pstat->rssi, FALSE, &pstat->rssi_level) ) {
			ODM_PRINT_ADDR(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("Target STA addr : "), pstat->hwaddr);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("RSSI:%d, RSSI_LEVEL:%d\n", pstat->rssi, pstat->rssi_level));

#ifdef CONFIG_RTL_88E_SUPPORT
			if (GET_CHIP_VER(priv)==VERSION_8188E) {
#ifdef TXREPORT
				add_RATid(priv, pstat);
#endif
			} else
#endif
			{
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)			
			add_update_RATid(priv, pstat);
#endif
		        }
	        }
	}
#endif
}

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
VOID
ODM_DynamicARFBSelect(
	IN		PDM_ODM_T		pDM_Odm,
	IN 		u1Byte			rate,
	IN  		BOOLEAN			Collision_State	
)
{

	if(pDM_Odm->SupportICType != ODM_RTL8192E)
		return;

	if (rate >= DESC_RATEMCS8  && rate <= DESC_RATEMCS12){
		if (Collision_State == 1){
			if(rate == DESC_RATEMCS12){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x0);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x07060501);	
			}
			else if(rate == DESC_RATEMCS11){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x0);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x07070605);	
			}
			else if(rate == DESC_RATEMCS10){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x0);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x08080706);	
			}
			else if(rate == DESC_RATEMCS9){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x0);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x08080707);	
			}
			else{

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x0);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x09090808);	
			}
		}
		else{   // Collision_State == 0
			if(rate == DESC_RATEMCS12){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x05010000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x09080706);	
			}
			else if(rate == DESC_RATEMCS11){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x06050000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x09080807);	
			}
			else if(rate == DESC_RATEMCS10){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x07060000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x0a090908);	
			}
			else if(rate == DESC_RATEMCS9){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x07070000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x0a090808);	
			}
			else{

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x08080000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x0b0a0909);	
			}
		}
	}
	else{  // MCS13~MCS15,  1SS, G-mode
		if (Collision_State == 1){
			if(rate == DESC_RATEMCS15){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x00000000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x05040302);	
			}
			else if(rate == DESC_RATEMCS14){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x00000000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x06050302);	
			}
			else if(rate == DESC_RATEMCS13){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x00000000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x07060502);	
			}
			else{

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x00000000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x06050402);	
			}
		}
		else{   // Collision_State == 0
  			if(rate == DESC_RATEMCS15){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x03020000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x07060504);	
			}
			else if(rate == DESC_RATEMCS14){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x03020000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x08070605);	
			}
			else if(rate == DESC_RATEMCS13){

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x05020000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x09080706);	
			}
			else{

				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E, 0x04020000);
				ODM_Write4Byte(pDM_Odm, REG_DARFRC_8192E+4, 0x08070605);	
			}


		}

	}	

}

#endif

// Return Value: BOOLEAN
// - TRUE: RATRState is changed.
BOOLEAN 
ODM_RAStateCheck(
	IN		PDM_ODM_T		pDM_Odm,
	IN		s4Byte			RSSI,
	IN		BOOLEAN			bForceUpdate,
	OUT		pu1Byte			pRATRState
	)
{
	PODM_RATE_ADAPTIVE pRA = &pDM_Odm->RateAdaptive;
	const u1Byte GoUpGap = 5;
	u1Byte HighRSSIThreshForRA = pRA->HighRSSIThresh;
	u1Byte LowRSSIThreshForRA = pRA->LowRSSIThresh;
	u1Byte RATRState;

	// Threshold Adjustment: 
	// when RSSI state trends to go up one or two levels, make sure RSSI is high enough.
	// Here GoUpGap is added to solve the boundary's level alternation issue.
	switch (*pRATRState)
	{
		case DM_RATR_STA_INIT:
		case DM_RATR_STA_HIGH:
			break;

		case DM_RATR_STA_MIDDLE:
			HighRSSIThreshForRA += GoUpGap;
			break;

		case DM_RATR_STA_LOW:
			HighRSSIThreshForRA += GoUpGap;
			LowRSSIThreshForRA += GoUpGap;
			break;

		default: 
			ODM_RT_ASSERT(pDM_Odm, FALSE, ("wrong rssi level setting %d !", *pRATRState) );
			break;
	}

	// Decide RATRState by RSSI.
	if(RSSI > HighRSSIThreshForRA)
		RATRState = DM_RATR_STA_HIGH;
	else if(RSSI > LowRSSIThreshForRA)
		RATRState = DM_RATR_STA_MIDDLE;
	else
		RATRState = DM_RATR_STA_LOW;
	//printk("==>%s,RATRState:0x%02x ,RSSI:%d \n",__FUNCTION__,RATRState,RSSI);

	if( *pRATRState!=RATRState || bForceUpdate)
	{
		ODM_RT_TRACE( pDM_Odm, ODM_COMP_RA_MASK, ODM_DBG_LOUD, ("RSSI Level %d -> %d\n", *pRATRState, RATRState) );
		*pRATRState = RATRState;
		return TRUE;
	}

	return FALSE;
}


//============================================================

//3============================================================
//3 Dynamic Tx Power
//3============================================================

//Remove BY YuChen

//3============================================================
//3 RSSI Monitor
//3============================================================

VOID
odm_RSSIDumpToRegister(
	IN	PDM_ODM_T	pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER		Adapter = pDM_Odm->Adapter;

	if(pDM_Odm->SupportICType == ODM_RTL8812)
	{
		PlatformEFIOWrite1Byte(Adapter, rA_RSSIDump_Jaguar, Adapter->RxStats.RxRSSIPercentage[0]);
		PlatformEFIOWrite1Byte(Adapter, rB_RSSIDump_Jaguar, Adapter->RxStats.RxRSSIPercentage[1]);

		// Rx EVM
		PlatformEFIOWrite1Byte(Adapter, rS1_RXevmDump_Jaguar, Adapter->RxStats.RxEVMdbm[0]);
		PlatformEFIOWrite1Byte(Adapter, rS2_RXevmDump_Jaguar, Adapter->RxStats.RxEVMdbm[1]);

		// Rx SNR
		PlatformEFIOWrite1Byte(Adapter, rA_RXsnrDump_Jaguar, (u1Byte)(Adapter->RxStats.RxSNRdB[0]));
		PlatformEFIOWrite1Byte(Adapter, rB_RXsnrDump_Jaguar, (u1Byte)(Adapter->RxStats.RxSNRdB[1]));

		// Rx Cfo_Short
		PlatformEFIOWrite2Byte(Adapter, rA_CfoShortDump_Jaguar, Adapter->RxStats.RxCfoShort[0]);
		PlatformEFIOWrite2Byte(Adapter, rB_CfoShortDump_Jaguar, Adapter->RxStats.RxCfoShort[1]);

		// Rx Cfo_Tail
		PlatformEFIOWrite2Byte(Adapter, rA_CfoLongDump_Jaguar, Adapter->RxStats.RxCfoTail[0]);
		PlatformEFIOWrite2Byte(Adapter, rB_CfoLongDump_Jaguar, Adapter->RxStats.RxCfoTail[1]);
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8192E)
	{
		PlatformEFIOWrite1Byte(Adapter, rA_RSSIDump_92E, Adapter->RxStats.RxRSSIPercentage[0]);
		PlatformEFIOWrite1Byte(Adapter, rB_RSSIDump_92E, Adapter->RxStats.RxRSSIPercentage[1]);
		// Rx EVM
		PlatformEFIOWrite1Byte(Adapter, rS1_RXevmDump_92E, Adapter->RxStats.RxEVMdbm[0]);
		PlatformEFIOWrite1Byte(Adapter, rS2_RXevmDump_92E, Adapter->RxStats.RxEVMdbm[1]);
		// Rx SNR
		PlatformEFIOWrite1Byte(Adapter, rA_RXsnrDump_92E, (u1Byte)(Adapter->RxStats.RxSNRdB[0]));
		PlatformEFIOWrite1Byte(Adapter, rB_RXsnrDump_92E, (u1Byte)(Adapter->RxStats.RxSNRdB[1]));
		// Rx Cfo_Short
		PlatformEFIOWrite2Byte(Adapter, rA_CfoShortDump_92E, Adapter->RxStats.RxCfoShort[0]);
		PlatformEFIOWrite2Byte(Adapter, rB_CfoShortDump_92E, Adapter->RxStats.RxCfoShort[1]);
		// Rx Cfo_Tail
		PlatformEFIOWrite2Byte(Adapter, rA_CfoLongDump_92E, Adapter->RxStats.RxCfoTail[0]);
		PlatformEFIOWrite2Byte(Adapter, rB_CfoLongDump_92E, Adapter->RxStats.RxCfoTail[1]);
	 }
#endif
}


VOID
odm_RSSIMonitorInit(
	IN	PDM_ODM_T	pDM_Odm
	)
{
	pRA_T		pRA_Table = &pDM_Odm->DM_RA_Table;

   	pRA_Table->firstconnect = FALSE;

}

VOID
odm_RSSIMonitorCheck(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	// 
	// For AP/ADSL use prtl8192cd_priv
	// For CE/NIC use PADAPTER
	//

	if (!(pDM_Odm->SupportAbility & ODM_BB_RSSI_MONITOR))
		return;
	
	//
	// 2011/09/29 MH In HW integration first stage, we provide 4 different handle to operate
	// at the same time. In the stage2/3, we need to prive universal interface and merge all
	// HW dynamic mechanism.
	//
	switch	(pDM_Odm->SupportPlatform)
	{
		case	ODM_WIN:
			odm_RSSIMonitorCheckMP(pDM_Odm);
			break;

		case	ODM_CE:
			odm_RSSIMonitorCheckCE(pDM_Odm);
			break;

		case	ODM_AP:
			odm_RSSIMonitorCheckAP(pDM_Odm);
			break;		

		case	ODM_ADSL:
			//odm_DIGAP(pDM_Odm);
			break;	
	}
	
}	// odm_RSSIMonitorCheck


VOID
odm_RSSIMonitorCheckMP(
	IN	PDM_ODM_T	pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER		Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PRT_WLAN_STA	pEntry;
	u1Byte			i;
	s4Byte			tmpEntryMaxPWDB=0, tmpEntryMinPWDB=0xff;
	u1Byte			H2C_Parameter[4] ={0};
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u8Byte			curTxOkCnt = 0, curRxOkCnt = 0;	
	u1Byte			STBC_TX = 0;
	BOOLEAN			FirstConnect;                                                    
	pRA_T			pRA_Table = &pDM_Odm->DM_RA_Table;      
#if (BEAMFORMING_SUPPORT == 1)	
	BEAMFORMING_CAP Beamform_cap = BEAMFORMING_CAP_NONE;
	u1Byte			TxBF_EN = 0;
#endif

	RT_DISP(FDM, DM_PWDB, ("pHalData->UndecoratedSmoothedPWDB = 0x%x( %d)\n", 
		pHalData->UndecoratedSmoothedPWDB,
		pHalData->UndecoratedSmoothedPWDB));

	curTxOkCnt = Adapter->TxStats.NumTxBytesUnicast - pMgntInfo->lastTxOkCnt;
	curRxOkCnt = Adapter->RxStats.NumRxBytesUnicast - pMgntInfo->lastRxOkCnt;
	pMgntInfo->lastTxOkCnt = curTxOkCnt;
	pMgntInfo->lastRxOkCnt = curRxOkCnt;

	RT_DISP(FDM, DM_PWDB, ("Tx = %d Rx = %d\n", curTxOkCnt, curRxOkCnt));

       FirstConnect = (pHalData->bLinked) && (pRA_Table->firstconnect == FALSE);    
	pRA_Table->firstconnect = pHalData->bLinked;                                               
       H2C_Parameter[3] |= FirstConnect << 5;

	if(pDM_Odm->SupportICType == ODM_RTL8188E && (pMgntInfo->CustomerID==RT_CID_819x_HP))
	{
		if(curRxOkCnt >(curTxOkCnt*6))
			PlatformEFIOWrite4Byte(Adapter, REG_ARFR0, 0x8f015);
		else
			PlatformEFIOWrite4Byte(Adapter, REG_ARFR0, 0xff015);
	}	

	if(pDM_Odm->SupportICType == ODM_RTL8812 || pDM_Odm->SupportICType == ODM_RTL8821)
	{
		if(curRxOkCnt >(curTxOkCnt*6))
			H2C_Parameter[3]=0x01;
		else
			H2C_Parameter[3]=0x00;
	}

	for(i = 0; i < ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
		if(IsAPModeExist(Adapter)  && GetFirstExtAdapter(Adapter) != NULL)
		{
			pEntry = AsocEntry_EnumStation(GetFirstExtAdapter(Adapter), i);
		}
		else
		{
			pEntry = AsocEntry_EnumStation(GetDefaultAdapter(Adapter), i);
		}

		if(pEntry != NULL)
		{
			if(pEntry->bAssociated)
			{
			
				RT_DISP_ADDR(FDM, DM_PWDB, ("pEntry->MacAddr ="), pEntry->MacAddr);
				RT_DISP(FDM, DM_PWDB, ("pEntry->rssi = 0x%x(%d)\n", 
					pEntry->rssi_stat.UndecoratedSmoothedPWDB, pEntry->rssi_stat.UndecoratedSmoothedPWDB));

				if(pDM_Odm->SupportICType == ODM_RTL8192E || pDM_Odm->SupportICType == ODM_RTL8812)
				{

#if (BEAMFORMING_SUPPORT == 1)
					Beamform_cap = Beamforming_GetEntryBeamCapByMacId(pMgntInfo, pEntry->AssociatedMacId);
					if(Beamform_cap & (BEAMFORMER_CAP_HT_EXPLICIT |BEAMFORMER_CAP_VHT_SU))
						TxBF_EN = 1;
					else
						TxBF_EN = 0;
	
					H2C_Parameter[3] |= TxBF_EN << 6; 
					
					if(TxBF_EN)
						STBC_TX = 0;
					else
#endif
					{
						if(IS_WIRELESS_MODE_AC(Adapter))
							STBC_TX = TEST_FLAG(pEntry->VHTInfo.STBC, STBC_VHT_ENABLE_TX);
						else
							STBC_TX = TEST_FLAG(pEntry->HTInfo.STBC, STBC_HT_ENABLE_TX);
					}

					H2C_Parameter[3] |= STBC_TX << 1;
				}

				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB < tmpEntryMinPWDB)
					tmpEntryMinPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > tmpEntryMaxPWDB)
					tmpEntryMaxPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;

				H2C_Parameter[2] = (u1Byte)(pEntry->rssi_stat.UndecoratedSmoothedPWDB & 0xFF);
				H2C_Parameter[1] = 0x20;   // fw v12 cmdid 5:use max macid ,for nic ,default macid is 0 ,max macid is 1
				H2C_Parameter[0] = (pEntry->AssociatedMacId);
				if(pDM_Odm->SupportICType == ODM_RTL8812)
					ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 4, H2C_Parameter);
				else if(pDM_Odm->SupportICType == ODM_RTL8192E)
					ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 4, H2C_Parameter);
				else	
					ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 3, H2C_Parameter);
			}
		}
		else
		{
			break;
		}
	}

	if(tmpEntryMaxPWDB != 0)	// If associated entry is found
	{
		pHalData->EntryMaxUndecoratedSmoothedPWDB = tmpEntryMaxPWDB;
		RT_DISP(FDM, DM_PWDB, ("EntryMaxPWDB = 0x%x(%d)\n",	tmpEntryMaxPWDB, tmpEntryMaxPWDB));
	}
	else
	{
		pHalData->EntryMaxUndecoratedSmoothedPWDB = 0;
	}
	
	if(tmpEntryMinPWDB != 0xff) // If associated entry is found
	{
		pHalData->EntryMinUndecoratedSmoothedPWDB = tmpEntryMinPWDB;
		RT_DISP(FDM, DM_PWDB, ("EntryMinPWDB = 0x%x(%d)\n", tmpEntryMinPWDB, tmpEntryMinPWDB));

	}
	else
	{
		pHalData->EntryMinUndecoratedSmoothedPWDB = 0;
	}

	// Indicate Rx signal strength to FW.
	if(pHalData->bUseRAMask)
	{
		if(pDM_Odm->SupportICType == ODM_RTL8192E || pDM_Odm->SupportICType == ODM_RTL8812)
		{
			PRT_HIGH_THROUGHPUT 		pHTInfo = GET_HT_INFO(pMgntInfo);
			PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

#if (BEAMFORMING_SUPPORT == 1)
			
			Beamform_cap = Beamforming_GetEntryBeamCapByMacId(pMgntInfo, pMgntInfo->mMacId);

			if(Beamform_cap & (BEAMFORMER_CAP_HT_EXPLICIT |BEAMFORMER_CAP_VHT_SU))
				TxBF_EN = 1;
			else
				TxBF_EN = 0;

			H2C_Parameter[3] |= TxBF_EN << 6; 

			if(TxBF_EN)
				STBC_TX = 0;
			else
#endif
			{
				if(IS_WIRELESS_MODE_AC(Adapter))
					STBC_TX = TEST_FLAG(pVHTInfo->VhtCurStbc, STBC_VHT_ENABLE_TX);
				else
					STBC_TX = TEST_FLAG(pHTInfo->HtCurStbc, STBC_HT_ENABLE_TX);
			}

			H2C_Parameter[3] |= STBC_TX << 1;
		}
		
		H2C_Parameter[2] = (u1Byte)(pHalData->UndecoratedSmoothedPWDB & 0xFF);
		H2C_Parameter[1] = 0x20;	// fw v12 cmdid 5:use max macid ,for nic ,default macid is 0 ,max macid is 1
		H2C_Parameter[0] = 0;		// fw v12 cmdid 5:use max macid ,for nic ,default macid is 0 ,max macid is 1
		if(pDM_Odm->SupportICType == ODM_RTL8812)
			ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 4, H2C_Parameter);
		else  if(pDM_Odm->SupportICType == ODM_RTL8192E)
			ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 4, H2C_Parameter);	
		else	
			ODM_FillH2CCmd(Adapter, ODM_H2C_RSSI_REPORT, 3, H2C_Parameter);
	}
	else
	{
		PlatformEFIOWrite1Byte(Adapter, 0x4fe, (u1Byte)pHalData->UndecoratedSmoothedPWDB);
	}

	if((pDM_Odm->SupportICType == ODM_RTL8812)||(pDM_Odm->SupportICType == ODM_RTL8192E))
		odm_RSSIDumpToRegister(pDM_Odm);

	odm_FindMinimumRSSI(Adapter);
	ODM_CmnInfoUpdate(&pHalData->DM_OutSrc ,ODM_CMNINFO_LINK, (u8Byte)pHalData->bLinked);
	ODM_CmnInfoUpdate(&pHalData->DM_OutSrc ,ODM_CMNINFO_RSSI_MIN, (u8Byte)pHalData->MinUndecoratedPWDBForDM);
#endif	// #if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
}

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
//
//sherry move from DUSC to here 20110517
//
static VOID
FindMinimumRSSI_Dmsp(
	IN	PADAPTER	pAdapter
)
{
#if 0
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	s32	Rssi_val_min_back_for_mac0;
	BOOLEAN		bGetValueFromBuddyAdapter = dm_DualMacGetParameterFromBuddyAdapter(pAdapter);
	BOOLEAN		bRestoreRssi = _FALSE;
	PADAPTER	BuddyAdapter = pAdapter->BuddyAdapter;

	if(pHalData->MacPhyMode92D == DUALMAC_SINGLEPHY)
	{
		if(BuddyAdapter!= NULL)
		{
			if(pHalData->bSlaveOfDMSP)
			{
				//ODM_RT_TRACE(pDM_Odm,COMP_EASY_CONCURRENT,DBG_LOUD,("bSlavecase of dmsp\n"));
				BuddyAdapter->DualMacDMSPControl.RssiValMinForAnotherMacOfDMSP = pdmpriv->MinUndecoratedPWDBForDM;
			}
			else
			{
				if(bGetValueFromBuddyAdapter)
				{
					//ODM_RT_TRACE(pDM_Odm,COMP_EASY_CONCURRENT,DBG_LOUD,("get new RSSI\n"));
					bRestoreRssi = _TRUE;
					Rssi_val_min_back_for_mac0 = pdmpriv->MinUndecoratedPWDBForDM;
					pdmpriv->MinUndecoratedPWDBForDM = pAdapter->DualMacDMSPControl.RssiValMinForAnotherMacOfDMSP;
				}
			}
		}
		
	}

	if(bRestoreRssi)
	{
		bRestoreRssi = _FALSE;
		pdmpriv->MinUndecoratedPWDBForDM = Rssi_val_min_back_for_mac0;
	}
#endif
}

static void
FindMinimumRSSI(
IN	PADAPTER	pAdapter
	)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;	
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);

	//1 1.Determine the minimum RSSI 

	if((pDM_Odm->bLinked != _TRUE) &&
		(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0))
	{
		pdmpriv->MinUndecoratedPWDBForDM = 0;
		//ODM_RT_TRACE(pDM_Odm,COMP_BB_POWERSAVING, DBG_LOUD, ("Not connected to any \n"));
	}
	else
	{
		pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
	}

	//DBG_8192C("%s=>MinUndecoratedPWDBForDM(%d)\n",__FUNCTION__,pdmpriv->MinUndecoratedPWDBForDM);
	//ODM_RT_TRACE(pDM_Odm,COMP_DIG, DBG_LOUD, ("MinUndecoratedPWDBForDM =%d\n",pHalData->MinUndecoratedPWDBForDM));
}
#endif

VOID
odm_RSSIMonitorCheckCE(
	IN		PDM_ODM_T		pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PADAPTER	Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(Adapter);
	int	i;
	int	tmpEntryMaxPWDB=0, tmpEntryMinPWDB=0xff;
	u8 	sta_cnt=0;
	u32	UL_DL_STATE = 0, STBC_TX = 0, TxBF_EN = 0;
	u32	PWDB_rssi[NUM_STA]={0};//[0~15]:MACID, [16~31]:PWDB_rssi
	BOOLEAN			FirstConnect = FALSE;
	pRA_T			pRA_Table = &pDM_Odm->DM_RA_Table;

	if(pDM_Odm->bLinked != _TRUE)
		return;

	#if((RTL8812A_SUPPORT==1)||(RTL8821A_SUPPORT==1))
	if((pDM_Odm->SupportICType == ODM_RTL8812)||(pDM_Odm->SupportICType == ODM_RTL8821))
	{
		u64	curTxOkCnt = pdvobjpriv->traffic_stat.cur_tx_bytes;
		u64	curRxOkCnt = pdvobjpriv->traffic_stat.cur_rx_bytes;

		if(curRxOkCnt >(curTxOkCnt*6))
			UL_DL_STATE = 1;
		else
			UL_DL_STATE = 0;
	}
	#endif

       FirstConnect = (pDM_Odm->bLinked) && (pRA_Table->firstconnect == FALSE);    
	pRA_Table->firstconnect = pDM_Odm->bLinked;

	//if(check_fwstate(&Adapter->mlmepriv, WIFI_AP_STATE|WIFI_ADHOC_STATE|WIFI_ADHOC_MASTER_STATE) == _TRUE)
	{
		#if 1
		struct sta_info *psta;
		
		for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++) {
			if (IS_STA_VALID(psta = pDM_Odm->pODM_StaInfo[i]))
			{
                        		if(IS_MCAST( psta->hwaddr))  //if(psta->mac_id ==1)
						 continue;
								
					if(psta->rssi_stat.UndecoratedSmoothedPWDB == (-1))
						 continue;
								
					if(psta->rssi_stat.UndecoratedSmoothedPWDB < tmpEntryMinPWDB)
						tmpEntryMinPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

					if(psta->rssi_stat.UndecoratedSmoothedPWDB > tmpEntryMaxPWDB)
						tmpEntryMaxPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

					#if 0
					DBG_871X("%s mac_id:%u, mac:"MAC_FMT", rssi:%d\n", __func__,
						psta->mac_id, MAC_ARG(psta->hwaddr), psta->rssi_stat.UndecoratedSmoothedPWDB);
					#endif

					if(psta->rssi_stat.UndecoratedSmoothedPWDB != (-1)) {

#ifdef CONFIG_80211N_HT
						if(pDM_Odm->SupportICType == ODM_RTL8192E || pDM_Odm->SupportICType == ODM_RTL8812)
						{
#ifdef CONFIG_BEAMFORMING
							BEAMFORMING_CAP Beamform_cap = beamforming_get_entry_beam_cap_by_mac_id(&Adapter->mlmepriv, psta->mac_id);

							if(Beamform_cap & (BEAMFORMER_CAP_HT_EXPLICIT |BEAMFORMER_CAP_VHT_SU))
								TxBF_EN = 1;
							else
								TxBF_EN = 0;

							if (TxBF_EN) {
								STBC_TX = 0;
							}
							else
#endif
							{
#ifdef CONFIG_80211AC_VHT
								if(IsSupportedVHT(psta->wireless_mode))
									STBC_TX = TEST_FLAG(psta->vhtpriv.stbc_cap, STBC_VHT_ENABLE_TX);
								else	
#endif
									STBC_TX = TEST_FLAG(psta->htpriv.stbc_cap, STBC_HT_ENABLE_TX);
							}
						}
#endif

						if(pDM_Odm->SupportICType == ODM_RTL8192D)
							PWDB_rssi[sta_cnt++] = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB<<16) | ((Adapter->stapriv.asoc_sta_count+1) << 8));
						else if ((pDM_Odm->SupportICType == ODM_RTL8192E)||(pDM_Odm->SupportICType == ODM_RTL8812)||(pDM_Odm->SupportICType == ODM_RTL8821))
							PWDB_rssi[sta_cnt++] = (((u8)(psta->mac_id&0xFF)) | ((psta->rssi_stat.UndecoratedSmoothedPWDB&0x7F)<<16) |(STBC_TX << 25) | (FirstConnect << 29) | (TxBF_EN << 30));
						else
							PWDB_rssi[sta_cnt++] = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB<<16) );
					}
			}
		}
		#else
		_irqL irqL;
		_list	*plist, *phead;
		struct sta_info *psta;
		struct sta_priv *pstapriv = &Adapter->stapriv;
		u8 bcast_addr[ETH_ALEN]= {0xff,0xff,0xff,0xff,0xff,0xff};

		_enter_critical_bh(&pstapriv->sta_hash_lock, &irqL);

		for(i=0; i< NUM_STA; i++)
		{
			phead = &(pstapriv->sta_hash[i]);
			plist = get_next(phead);
		
			while ((rtw_end_of_queue_search(phead, plist)) == _FALSE)
			{
				psta = LIST_CONTAINOR(plist, struct sta_info, hash_list);

				plist = get_next(plist);

				if(_rtw_memcmp(psta->hwaddr, bcast_addr, ETH_ALEN) || 
					_rtw_memcmp(psta->hwaddr, myid(&Adapter->eeprompriv), ETH_ALEN))
					continue;

				if(psta->state & WIFI_ASOC_STATE)
				{
					
					if(psta->rssi_stat.UndecoratedSmoothedPWDB < tmpEntryMinPWDB)
						tmpEntryMinPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

					if(psta->rssi_stat.UndecoratedSmoothedPWDB > tmpEntryMaxPWDB)
						tmpEntryMaxPWDB = psta->rssi_stat.UndecoratedSmoothedPWDB;

					if(psta->rssi_stat.UndecoratedSmoothedPWDB != (-1)){
						//printk("%s==> mac_id(%d),rssi(%d)\n",__FUNCTION__,psta->mac_id,psta->rssi_stat.UndecoratedSmoothedPWDB);
						#if(RTL8192D_SUPPORT==1)
						PWDB_rssi[sta_cnt++] = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB<<16) | ((Adapter->stapriv.asoc_sta_count+1) << 8));
						#else
						PWDB_rssi[sta_cnt++] = (psta->mac_id | (psta->rssi_stat.UndecoratedSmoothedPWDB<<16) );
						#endif
					}
				}
			
			}

		}
	
		_exit_critical_bh(&pstapriv->sta_hash_lock, &irqL);
		#endif

		//printk("%s==> sta_cnt(%d)\n",__FUNCTION__,sta_cnt);

		for(i=0; i< sta_cnt; i++)
		{
			if(PWDB_rssi[i] != (0)){
				if(pHalData->fw_ractrl == _TRUE)// Report every sta's RSSI to FW
				{
					#if(RTL8192D_SUPPORT==1)
					if(pDM_Odm->SupportICType == ODM_RTL8192D){
						FillH2CCmd92D(Adapter, H2C_RSSI_REPORT, 3, (u8 *)(&PWDB_rssi[i]));		
					}
					#endif
					
					#if((RTL8192C_SUPPORT==1)||(RTL8723A_SUPPORT==1))
					if((pDM_Odm->SupportICType == ODM_RTL8192C)||(pDM_Odm->SupportICType == ODM_RTL8723A)){
						rtl8192c_set_rssi_cmd(Adapter, (u8*)&PWDB_rssi[i]);
					}
					#endif
					
					#if((RTL8812A_SUPPORT==1)||(RTL8821A_SUPPORT==1))
					if((pDM_Odm->SupportICType == ODM_RTL8812)||(pDM_Odm->SupportICType == ODM_RTL8821)){	
						PWDB_rssi[i] |= (UL_DL_STATE << 24);
						rtl8812_set_rssi_cmd(Adapter, (u8 *)(&PWDB_rssi[i]));
					}
					#endif
					#if(RTL8192E_SUPPORT==1)
					if(pDM_Odm->SupportICType == ODM_RTL8192E){
						rtl8192e_set_rssi_cmd(Adapter, (u8 *)(&PWDB_rssi[i]));
					}
					#endif
					#if(RTL8723B_SUPPORT==1)
					if(pDM_Odm->SupportICType == ODM_RTL8723B){
						rtl8723b_set_rssi_cmd(Adapter, (u8 *)(&PWDB_rssi[i]));
					}
					#endif
				}
				else{
					#if((RTL8188E_SUPPORT==1)&&(RATE_ADAPTIVE_SUPPORT == 1))
					if(pDM_Odm->SupportICType == ODM_RTL8188E){
						ODM_RA_SetRSSI_8188E(
						&(pHalData->odmpriv), (PWDB_rssi[i]&0xFF), (u8)((PWDB_rssi[i]>>16) & 0xFF));
					}
					#endif
				}
			}
		}		
	}



	if(tmpEntryMaxPWDB != 0)	// If associated entry is found
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = tmpEntryMaxPWDB;		
	}
	else
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = 0;
	}

	if(tmpEntryMinPWDB != 0xff) // If associated entry is found
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = tmpEntryMinPWDB;		
	}
	else
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = 0;
	}

	FindMinimumRSSI(Adapter);//get pdmpriv->MinUndecoratedPWDBForDM

	#if(RTL8192D_SUPPORT==1)
	FindMinimumRSSI_Dmsp(Adapter);
	#endif
	pDM_Odm->RSSI_Min = pdmpriv->MinUndecoratedPWDBForDM;
	//ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_RSSI_MIN, pdmpriv->MinUndecoratedPWDBForDM);
#endif//if (DM_ODM_SUPPORT_TYPE == ODM_CE)
}
VOID
odm_RSSIMonitorCheckAP(
	IN		PDM_ODM_T		pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_AP)
#ifdef CONFIG_RTL_92C_SUPPORT || defined(CONFIG_RTL_92D_SUPPORT)

	u4Byte i;
	PSTA_INFO_T pstat;

	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
		pstat = pDM_Odm->pODM_StaInfo[i];
		if(IS_STA_VALID(pstat) )
		{			
#ifdef STA_EXT
			if (REMAP_AID(pstat) < (FW_NUM_STAT - 1))
#endif
				add_update_rssi(pDM_Odm->priv, pstat);

		}		
	}
#endif
#endif

}



VOID
ODM_InitAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{
#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
	ODM_AntDivTimers(pDM_Odm,INIT_ANTDIV_TIMMER);
#elif(defined(CONFIG_SW_ANTENNA_DIVERSITY))
	ODM_InitializeTimer(pDM_Odm,&pDM_Odm->DM_SWAT_Table.SwAntennaSwitchTimer,
		(RT_TIMER_CALL_BACK)odm_SwAntDivChkAntSwitchCallback, NULL, "SwAntennaSwitchTimer");
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	ODM_InitializeTimer(pDM_Odm, &pDM_Odm->PSDTimer, 
		(RT_TIMER_CALL_BACK)dm_PSDMonitorCallback, NULL, "PSDTimer");
	//
	//Path Diversity
	//Neil Chen--2011--06--16--  / 2012/02/23 MH Revise Arch.
	//
	ODM_InitializeTimer(pDM_Odm, &pDM_Odm->PathDivSwitchTimer, 
		(RT_TIMER_CALL_BACK)odm_PathDivChkAntSwitchCallback, NULL, "PathDivTimer");

	ODM_InitializeTimer(pDM_Odm, &pDM_Odm->CCKPathDiversityTimer, 
		(RT_TIMER_CALL_BACK)odm_CCKTXPathDiversityCallback, NULL, "CCKPathDiversityTimer");

	ODM_InitializeTimer(pDM_Odm, &pDM_Odm->MPT_DIGTimer, 
		(RT_TIMER_CALL_BACK)odm_MPT_DIGCallback, NULL, "MPT_DIGTimer");

	ODM_InitializeTimer(pDM_Odm, &pDM_Odm->DM_RXHP_Table.PSDTimer,
		(RT_TIMER_CALL_BACK)odm_PSD_RXHPCallback, NULL, "PSDRXHPTimer");  
#endif	
}

VOID
ODM_CancelAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	//
	// 2012/01/12 MH Temp BSOD fix. We need to find NIC allocate mem fail reason in 
	// win7 platform.
	//
	HAL_ADAPTER_STS_CHK(pDM_Odm)
#endif	

#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
	ODM_AntDivTimers(pDM_Odm,CANCEL_ANTDIV_TIMMER);
#elif(defined(CONFIG_SW_ANTENNA_DIVERSITY))
	ODM_CancelTimer(pDM_Odm,&pDM_Odm->DM_SWAT_Table.SwAntennaSwitchTimer);
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	ODM_CancelTimer(pDM_Odm, &pDM_Odm->PSDTimer);	
	//
	//Path Diversity
	//Neil Chen--2011--06--16--  / 2012/02/23 MH Revise Arch.
	//
	ODM_CancelTimer(pDM_Odm, &pDM_Odm->PathDivSwitchTimer);

	ODM_CancelTimer(pDM_Odm, &pDM_Odm->CCKPathDiversityTimer);

	ODM_CancelTimer(pDM_Odm, &pDM_Odm->MPT_DIGTimer);

	ODM_CancelTimer(pDM_Odm, &pDM_Odm->DM_RXHP_Table.PSDTimer);
#endif	
}


VOID
ODM_ReleaseAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{
#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
	ODM_AntDivTimers(pDM_Odm,RELEASE_ANTDIV_TIMMER);
#elif(defined(CONFIG_SW_ANTENNA_DIVERSITY))
	ODM_ReleaseTimer(pDM_Odm,&pDM_Odm->DM_SWAT_Table.SwAntennaSwitchTimer);
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

	ODM_ReleaseTimer(pDM_Odm, &pDM_Odm->PSDTimer);
	//
	//Path Diversity
	//Neil Chen--2011--06--16--  / 2012/02/23 MH Revise Arch.
	//
	ODM_ReleaseTimer(pDM_Odm, &pDM_Odm->PathDivSwitchTimer);

	ODM_ReleaseTimer(pDM_Odm, &pDM_Odm->CCKPathDiversityTimer);

	ODM_ReleaseTimer(pDM_Odm, &pDM_Odm->MPT_DIGTimer);

	ODM_ReleaseTimer(pDM_Odm, &pDM_Odm->DM_RXHP_Table.PSDTimer); 
#endif	
}


//3============================================================
//3 Tx Power Tracking
//3============================================================

VOID
odm_IQCalibrate(
		IN	PDM_ODM_T	pDM_Odm 
		)
{
	PADAPTER	Adapter = pDM_Odm->Adapter;
	
	if(!IS_HARDWARE_TYPE_JAGUAR(Adapter))
		return;
	else if(IS_HARDWARE_TYPE_8812AU(Adapter))
		return;
#if (RTL8821A_SUPPORT == 1)
	if(pDM_Odm->bLinked)
	{
		if((*pDM_Odm->pChannel != pDM_Odm->preChannel) && (!*pDM_Odm->pbScanInProcess))
		{
			pDM_Odm->preChannel = *pDM_Odm->pChannel;
			pDM_Odm->LinkedInterval = 0;
		}

		if(pDM_Odm->LinkedInterval < 3)
			pDM_Odm->LinkedInterval++;
		
		if(pDM_Odm->LinkedInterval == 2)
		{
			// Mark out IQK flow to prevent tx stuck. by Maddest 20130306
			// Open it verified by James 20130715
			PHY_IQCalibrate_8821A(Adapter, FALSE);
		}
	}
	else
		pDM_Odm->LinkedInterval = 0;
#endif
}


VOID
odm_TXPowerTrackingInit(
	IN	PDM_ODM_T	pDM_Odm 
	)
{
#if (DM_ODM_SUPPORT_TYPE & (ODM_AP|ODM_ADSL))
	if(!(pDM_Odm->SupportICType & (ODM_RTL8814A|ODM_IC_11N_SERIES)))
		return;
#endif

	odm_TXPowerTrackingThermalMeterInit(pDM_Odm);
}	

u1Byte 
getSwingIndex(
	IN	PDM_ODM_T	pDM_Odm 
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u1Byte 			i = 0;
	u4Byte 			bbSwing;
	u4Byte 			swingTableSize;
	pu4Byte 			pSwingTable;

	if (pDM_Odm->SupportICType == ODM_RTL8188E || pDM_Odm->SupportICType == ODM_RTL8723B ||
		pDM_Odm->SupportICType == ODM_RTL8192E) 
	{
		bbSwing = PHY_QueryBBReg(Adapter, rOFDM0_XATxIQImbalance, 0xFFC00000);

		pSwingTable = OFDMSwingTable_New;
		swingTableSize = OFDM_TABLE_SIZE;
	} else {
#if ((RTL8812A_SUPPORT==1)||(RTL8821A_SUPPORT==1))
		if (pDM_Odm->SupportICType == ODM_RTL8812 || pDM_Odm->SupportICType == ODM_RTL8821)
		{
			bbSwing = PHY_GetTxBBSwing_8812A(Adapter, pHalData->CurrentBandType, ODM_RF_PATH_A);
			pSwingTable = TxScalingTable_Jaguar;
			swingTableSize = TXSCALE_TABLE_SIZE;
		}
		else
#endif
		{
			bbSwing = 0;
			pSwingTable = OFDMSwingTable;
			swingTableSize = OFDM_TABLE_SIZE;
		}
	}

	for (i = 0; i < swingTableSize; ++i) {
		u4Byte tableValue = pSwingTable[i];
		
		if (tableValue >= 0x100000 )
			tableValue >>= 22;
		if (bbSwing == tableValue)
			break;
	}
	return i;
}

VOID
odm_TXPowerTrackingThermalMeterInit(
	IN	PDM_ODM_T	pDM_Odm 
	)
{
	u1Byte defaultSwingIndex = getSwingIndex(pDM_Odm);
	u1Byte 			p = 0;
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER		Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	#if	MP_DRIVER != 1					//for mp driver, turn off txpwrtracking as default
	pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = pHalData->TxPowerTrackControl = TRUE;		
	#endif
#elif (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PADAPTER			Adapter = pDM_Odm->Adapter;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);


	if (pDM_Odm->SupportICType >= ODM_RTL8188E) 
	{
		pDM_Odm->RFCalibrateInfo.bTXPowerTracking = _TRUE;
		pDM_Odm->RFCalibrateInfo.TXPowercount = 0;
		pDM_Odm->RFCalibrateInfo.bTXPowerTrackingInit = _FALSE;
		
		if ( *(pDM_Odm->mp_mode) != 1)
			pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = _TRUE;
		else
			pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = _FALSE;

		MSG_8192C("pDM_Odm TxPowerTrackControl = %d\n", pDM_Odm->RFCalibrateInfo.TxPowerTrackControl);
	}
	else
	{
		struct dm_priv	*pdmpriv = &pHalData->dmpriv;

		pdmpriv->bTXPowerTracking = _TRUE;
		pdmpriv->TXPowercount = 0;
		pdmpriv->bTXPowerTrackingInit = _FALSE;
		//#if	(MP_DRIVER != 1)		//for mp driver, turn off txpwrtracking as default

		if (*(pDM_Odm->mp_mode) != 1)
			pdmpriv->TxPowerTrackControl = _TRUE;
		else
			pdmpriv->TxPowerTrackControl = _FALSE;


		//MSG_8192C("pdmpriv->TxPowerTrackControl = %d\n", pdmpriv->TxPowerTrackControl);
	}
	
#elif (DM_ODM_SUPPORT_TYPE & (ODM_AP|ODM_ADSL))
	#ifdef RTL8188E_SUPPORT
	{
		pDM_Odm->RFCalibrateInfo.bTXPowerTracking = _TRUE;
		pDM_Odm->RFCalibrateInfo.TXPowercount = 0;
		pDM_Odm->RFCalibrateInfo.bTXPowerTrackingInit = _FALSE;
		pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = _TRUE;
	}
	#endif
#endif

	//pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = TRUE;
	pDM_Odm->RFCalibrateInfo.ThermalValue = pHalData->EEPROMThermalMeter;
	pDM_Odm->RFCalibrateInfo.ThermalValue_IQK = pHalData->EEPROMThermalMeter;
	pDM_Odm->RFCalibrateInfo.ThermalValue_LCK = pHalData->EEPROMThermalMeter;	

	// The index of "0 dB" in SwingTable.
	if (pDM_Odm->SupportICType == ODM_RTL8188E || pDM_Odm->SupportICType == ODM_RTL8723B ||
		pDM_Odm->SupportICType == ODM_RTL8192E) 
	{
		pDM_Odm->DefaultOfdmIndex = (defaultSwingIndex >= OFDM_TABLE_SIZE) ? 30 : defaultSwingIndex;
		pDM_Odm->DefaultCckIndex = 20;	
	}
	else
	{
		pDM_Odm->DefaultOfdmIndex = (defaultSwingIndex >= TXSCALE_TABLE_SIZE) ? 24 : defaultSwingIndex;
		pDM_Odm->DefaultCckIndex = 24;	
	}

	pDM_Odm->BbSwingIdxCckBase = pDM_Odm->DefaultCckIndex;
	pDM_Odm->RFCalibrateInfo.CCK_index = pDM_Odm->DefaultCckIndex;
	
	for (p = ODM_RF_PATH_A; p < MAX_RF_PATH; ++p)
	{
		pDM_Odm->BbSwingIdxOfdmBase[p] = pDM_Odm->DefaultOfdmIndex;		
	   	pDM_Odm->RFCalibrateInfo.OFDM_index[p] = pDM_Odm->DefaultOfdmIndex;		
		pDM_Odm->RFCalibrateInfo.DeltaPowerIndex[p] = 0;
		pDM_Odm->RFCalibrateInfo.DeltaPowerIndexLast[p] = 0;
		pDM_Odm->RFCalibrateInfo.PowerIndexOffset[p] = 0;
	}

}


VOID
ODM_TXPowerTrackingCheck(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	//
	// 2011/09/29 MH In HW integration first stage, we provide 4 different handle to operate
	// at the same time. In the stage2/3, we need to prive universal interface and merge all
	// HW dynamic mechanism.
	//
	switch	(pDM_Odm->SupportPlatform)
	{
		case	ODM_WIN:
			odm_TXPowerTrackingCheckMP(pDM_Odm);
			break;

		case	ODM_CE:
			odm_TXPowerTrackingCheckCE(pDM_Odm);
			break;

		case	ODM_AP:
			odm_TXPowerTrackingCheckAP(pDM_Odm);		
			break;		

		case	ODM_ADSL:
			//odm_DIGAP(pDM_Odm);
			break;	
	}

}

VOID
odm_TXPowerTrackingCheckCE(
	IN		PDM_ODM_T		pDM_Odm 
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
	PADAPTER	Adapter = pDM_Odm->Adapter;
	#if( (RTL8192C_SUPPORT==1) ||  (RTL8723A_SUPPORT==1) )
	if(IS_HARDWARE_TYPE_8192C(Adapter)){
		rtl8192c_odm_CheckTXPowerTracking(Adapter);
		return;
	}
	#endif

	#if (RTL8192D_SUPPORT==1) 
	if(IS_HARDWARE_TYPE_8192D(Adapter)){	
		#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
		if(!Adapter->bSlaveOfDMSP)
		#endif
			rtl8192d_odm_CheckTXPowerTracking(Adapter);
		return;	
	}
	#endif

	#if(((RTL8188E_SUPPORT==1) ||  (RTL8812A_SUPPORT==1) ||  (RTL8821A_SUPPORT==1) ||  (RTL8192E_SUPPORT==1)  ||  (RTL8723B_SUPPORT==1)  ))
	if(!(pDM_Odm->SupportAbility & ODM_RF_TX_PWR_TRACK))
	{
		return;
	}

	if(!pDM_Odm->RFCalibrateInfo.TM_Trigger)		//at least delay 1 sec
	{
		//pHalData->TxPowerCheckCnt++;	//cosa add for debug
		if(IS_HARDWARE_TYPE_8188E(Adapter) || IS_HARDWARE_TYPE_JAGUAR(Adapter) || IS_HARDWARE_TYPE_8192E(Adapter)||IS_HARDWARE_TYPE_8723B(Adapter))
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_T_METER_NEW, (BIT17 | BIT16), 0x03);
		else
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_T_METER_OLD, bRFRegOffsetMask, 0x60);
		
		//DBG_871X("Trigger Thermal Meter!!\n");
		
		pDM_Odm->RFCalibrateInfo.TM_Trigger = 1;
		return;
	}
	else
	{
		//DBG_871X("Schedule TxPowerTracking direct call!!\n");
		ODM_TXPowerTrackingCallback_ThermalMeter(Adapter);
		pDM_Odm->RFCalibrateInfo.TM_Trigger = 0;
	}
	#endif
#endif	
}

VOID
odm_TXPowerTrackingCheckMP(
	IN		PDM_ODM_T		pDM_Odm 
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	PADAPTER	Adapter = pDM_Odm->Adapter;

	if (ODM_CheckPowerStatus(Adapter) == FALSE) 
	{
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD, ("===>ODM_CheckPowerStatus() return FALSE\n"));
		return;
	}

	if(IS_HARDWARE_TYPE_8723A(Adapter))
		return;

	if(!Adapter->bSlaveOfDMSP || Adapter->DualMacSmartConcurrent == FALSE)
		odm_TXPowerTrackingThermalMeterCheck(Adapter);
	else {
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD, ("!Adapter->bSlaveOfDMSP || Adapter->DualMacSmartConcurrent == FALSE\n"));
	}
#endif
	
}


VOID
odm_TXPowerTrackingCheckAP(
	IN		PDM_ODM_T		pDM_Odm
	)
{
#if (DM_ODM_SUPPORT_TYPE == ODM_AP)
	prtl8192cd_priv	priv		= pDM_Odm->priv;

	if ( (priv->pmib->dot11RFEntry.ther) && ((priv->up_time % priv->pshare->rf_ft_var.tpt_period) == 0)){
#ifdef CONFIG_RTL_92D_SUPPORT
		if (GET_CHIP_VER(priv)==VERSION_8192D){
			tx_power_tracking_92D(priv);
		} else 
#endif
		{
#ifdef CONFIG_RTL_92C_SUPPORT			
			tx_power_tracking(priv);
#endif
		}
	}
#endif	

}



//antenna mapping info
// 1: right-side antenna
// 2/0: left-side antenna
//PDM_SWAT_Table->CCK_Ant1_Cnt /OFDM_Ant1_Cnt:  for right-side antenna:   Ant:1    RxDefaultAnt1
//PDM_SWAT_Table->CCK_Ant2_Cnt /OFDM_Ant2_Cnt:  for left-side antenna:     Ant:0    RxDefaultAnt2
// We select left antenna as default antenna in initial process, modify it as needed
//

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

VOID
odm_TXPowerTrackingThermalMeterCheck(
	IN	PADAPTER		Adapter
	)
{
#ifndef AP_BUILD_WORKAROUND
	static u1Byte			TM_Trigger = 0;

	if(!(GET_HAL_DATA(Adapter)->DM_OutSrc.SupportAbility & ODM_RF_TX_PWR_TRACK))
	{
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,
			("===>odm_TXPowerTrackingThermalMeterCheck(),pMgntInfo->bTXPowerTracking is FALSE, return!!\n"));
		return;
	}

	if(!TM_Trigger)		//at least delay 1 sec
	{
		if(IS_HARDWARE_TYPE_8192D(Adapter))
			PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_T_METER_92D, BIT17 | BIT16, 0x03);
		else if(IS_HARDWARE_TYPE_8188E(Adapter) || IS_HARDWARE_TYPE_JAGUAR(Adapter) || IS_HARDWARE_TYPE_8192E(Adapter) ||
			    IS_HARDWARE_TYPE_8723B(Adapter))
			PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_T_METER_88E, BIT17 | BIT16, 0x03);
		else
			PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_T_METER, bRFRegOffsetMask, 0x60);
		
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Trigger Thermal Meter!!\n"));
		
		TM_Trigger = 1;
		return;
	}
	else
	{
		RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Schedule TxPowerTracking direct call!!\n"));		
		odm_TXPowerTrackingDirectCall(Adapter); //Using direct call is instead, added by Roger, 2009.06.18.
		TM_Trigger = 0;
	}
#endif
}

// Only for 8723A SW ANT DIV INIT--2012--07--17
VOID
odm_SwAntDivInit_NIC_8723A(
	IN	PDM_ODM_T		pDM_Odm)
{
	pSWAT_T		pDM_SWAT_Table = &pDM_Odm->DM_SWAT_Table;
	PADAPTER		Adapter = pDM_Odm->Adapter;
	
	u1Byte 			btAntNum=BT_GetPgAntNum(Adapter);

	if(IS_HARDWARE_TYPE_8723A(Adapter))
	{
		pDM_SWAT_Table->ANTA_ON =TRUE;
		
		// Set default antenna B status by PG
		if(btAntNum == 2)
			pDM_SWAT_Table->ANTB_ON = TRUE;
		else if(btAntNum == 1)
			pDM_SWAT_Table->ANTB_ON = FALSE;
		else
			pDM_SWAT_Table->ANTB_ON = TRUE;
	}	
	
}

#endif //end #ifMP



//3============================================================
//3 SW Antenna Diversity
//3============================================================
VOID
odm_AntennaDiversityInit(
	IN 		PDM_ODM_T		pDM_Odm 
)
{
	if(*(pDM_Odm->mp_mode) == TRUE)
		return;

	if(pDM_Odm->SupportICType & (ODM_OLD_IC_ANTDIV_SUPPORT))
	{
		#if (RTL8192C_SUPPORT==1) 
		ODM_OldIC_AntDiv_Init(pDM_Odm);
		#endif
	}
	else
	{
		#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
		ODM_AntDiv_Config(pDM_Odm);
		ODM_AntDivInit(pDM_Odm);
		#endif
	}
}

VOID
odm_AntennaDiversity(
	IN 		PDM_ODM_T		pDM_Odm 
)
{
	if(*(pDM_Odm->mp_mode) == TRUE)
		return;

	if(pDM_Odm->SupportICType & (ODM_OLD_IC_ANTDIV_SUPPORT))
	{
		#if (RTL8192C_SUPPORT==1) 
		ODM_OldIC_AntDiv(pDM_Odm);
		#endif
	}
	else
	{
		#if(defined(CONFIG_HW_ANTENNA_DIVERSITY))
		ODM_AntDiv(pDM_Odm);
		#endif
	}
}


void
odm_SwAntDetectInit(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	pSWAT_T		pDM_SWAT_Table = &pDM_Odm->DM_SWAT_Table;
#if (RTL8723B_SUPPORT == 1)
	pDM_SWAT_Table->SWAS_NoLink_BK_Reg92c = ODM_Read4Byte(pDM_Odm, rDPDT_control);
#endif
	pDM_SWAT_Table->PreAntenna = MAIN_ANT;
	pDM_SWAT_Table->CurAntenna = MAIN_ANT;
	pDM_SWAT_Table->SWAS_NoLink_State = 0;
}

//============================================================
//EDCA Turbo
//============================================================

//Remove Edca by Yuchen


#if( DM_ODM_SUPPORT_TYPE == ODM_WIN) 
//
// 2011/07/26 MH Add an API for testing IQK fail case.
//
BOOLEAN
ODM_CheckPowerStatus(
	IN	PADAPTER		Adapter)
{

	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T			pDM_Odm = &pHalData->DM_OutSrc;
	RT_RF_POWER_STATE 	rtState;
	PMGNT_INFO			pMgntInfo	= &(Adapter->MgntInfo);

	// 2011/07/27 MH We are not testing ready~~!! We may fail to get correct value when init sequence.
	if (pMgntInfo->init_adpt_in_progress == TRUE)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("ODM_CheckPowerStatus Return TRUE, due to initadapter"));
		return	TRUE;
	}
	
	//
	//	2011/07/19 MH We can not execute tx pwoer tracking/ LLC calibrate or IQK.
	//
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if(Adapter->bDriverStopped || Adapter->bDriverIsGoingToPnpSetPowerSleep || rtState == eRfOff)
	{
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("ODM_CheckPowerStatus Return FALSE, due to %d/%d/%d\n", 
		Adapter->bDriverStopped, Adapter->bDriverIsGoingToPnpSetPowerSleep, rtState));
		return	FALSE;
	}
	return	TRUE;
}
#endif

// need to ODM CE Platform
//move to here for ANT detection mechanism using

#if ((DM_ODM_SUPPORT_TYPE == ODM_WIN)||(DM_ODM_SUPPORT_TYPE == ODM_CE))
u4Byte
GetPSDData(
	IN PDM_ODM_T	pDM_Odm,
	unsigned int 	point,
	u1Byte initial_gain_psd)
{
	//unsigned int	val, rfval;
	//int	psd_report;
	u4Byte	psd_report;
	
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	//Debug Message
	//val = PHY_QueryBBReg(Adapter,0x908, bMaskDWord);
	//DbgPrint("Reg908 = 0x%x\n",val);
	//val = PHY_QueryBBReg(Adapter,0xDF4, bMaskDWord);
	//rfval = PHY_QueryRFReg(Adapter, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask);
	//DbgPrint("RegDF4 = 0x%x, RFReg00 = 0x%x\n",val, rfval);
	//DbgPrint("PHYTXON = %x, OFDMCCA_PP = %x, CCKCCA_PP = %x, RFReg00 = %x\n",
		//(val&BIT25)>>25, (val&BIT14)>>14, (val&BIT15)>>15, rfval);

	//Set DCO frequency index, offset=(40MHz/SamplePts)*point
	ODM_SetBBReg(pDM_Odm, 0x808, 0x3FF, point);

	//Start PSD calculation, Reg808[22]=0->1
	ODM_SetBBReg(pDM_Odm, 0x808, BIT22, 1);
	//Need to wait for HW PSD report
	ODM_StallExecution(1000);
	ODM_SetBBReg(pDM_Odm, 0x808, BIT22, 0);
	//Read PSD report, Reg8B4[15:0]
	psd_report = ODM_GetBBReg(pDM_Odm,0x8B4, bMaskDWord) & 0x0000FFFF;
	
#if 1//(DEV_BUS_TYPE == RT_PCI_INTERFACE) && ( (RT_PLATFORM == PLATFORM_LINUX) || (RT_PLATFORM == PLATFORM_MACOSX))
	psd_report = (u4Byte) (ConvertTo_dB(psd_report))+(u4Byte)(initial_gain_psd-0x1c);
#else
	psd_report = (int) (20*log10((double)psd_report))+(int)(initial_gain_psd-0x1c);
#endif

	return psd_report;
	
}

u4Byte 
ConvertTo_dB(
	u4Byte 	Value)
{
	u1Byte i;
	u1Byte j;
	u4Byte dB;

	Value = Value & 0xFFFF;
	
	for (i=0;i<8;i++)
	{
		if (Value <= dB_Invert_Table[i][11])
		{
			break;
		}
	}

	if (i >= 8)
	{
		return (96);	// maximum 96 dB
	}

	for (j=0;j<12;j++)
	{
		if (Value <= dB_Invert_Table[i][j])
		{
			break;
		}
	}

	dB = i*12 + j + 1;

	return (dB);
}

#endif

//
// LukeLee: 
// PSD function will be moved to FW in future IC, but now is only implemented in MP platform
// So PSD function will not be incorporated to common ODM
//
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

#define	AFH_PSD		1	//0:normal PSD scan, 1: only do 20 pts PSD
#define	MODE_40M		0	//0:20M, 1:40M
#define	PSD_TH2		3  
#define	PSD_CHMIN		20   // Minimum channel number for BT AFH
#define	SIR_STEP_SIZE	3
#define   Smooth_Size_1 	5
#define	Smooth_TH_1	3
#define   Smooth_Size_2 	10
#define	Smooth_TH_2	4
#define   Smooth_Size_3 	20
#define	Smooth_TH_3	4
#define   Smooth_Step_Size 5
#define	Adaptive_SIR	1
//#if(RTL8723_FPGA_VERIFICATION == 1)
//#define	PSD_RESCAN		1
//#else
//#define	PSD_RESCAN		4
//#endif
#define	SCAN_INTERVAL	1500 //ms
#define	SYN_Length		5    // for 92D
	
#define	LNA_Low_Gain_1                      0x64
#define	LNA_Low_Gain_2                      0x5A
#define	LNA_Low_Gain_3                      0x58

#define	pw_th_10dB					0x0
#define	pw_th_16dB					0x3

#define	FA_RXHP_TH1                           5000
#define	FA_RXHP_TH2                           1500
#define	FA_RXHP_TH3                             800
#define	FA_RXHP_TH4                             600
#define	FA_RXHP_TH5                             500

#define	Idle_Mode					0
#define	High_TP_Mode				1
#define	Low_TP_Mode				2


VOID
odm_PSDMonitorInit(
	IN PDM_ODM_T	pDM_Odm)
{
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)|(DEV_BUS_TYPE == RT_USB_INTERFACE)
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	//PSD Monitor Setting
	//Which path in ADC/DAC is turnned on for PSD: both I/Q
	ODM_SetBBReg(pDM_Odm, ODM_PSDREG, BIT10|BIT11, 0x3);
	//Ageraged number: 8
	ODM_SetBBReg(pDM_Odm, ODM_PSDREG, BIT12|BIT13, 0x1);
	pDM_Odm->bPSDinProcess = FALSE;
	pDM_Odm->bUserAssignLevel = FALSE;
	pDM_Odm->bPSDactive = FALSE;
	//pDM_Odm->bDMInitialGainEnable=TRUE;		//change the initialization to DIGinit
	//Set Debug Port
	//PHY_SetBBReg(Adapter, 0x908, bMaskDWord, 0x803);
	//PHY_SetBBReg(Adapter, 0xB34, bMaskByte0, 0x00); // pause PSD
	//PHY_SetBBReg(Adapter, 0xB38, bMaskByte0, 10); //rescan
	//PHY_SetBBReg(Adapter, 0xB38, bMaskByte2|bMaskByte3, 100); //interval

	//PlatformSetTimer( Adapter, &pHalData->PSDTriggerTimer, 0); //ms
#endif
}

VOID
PatchDCTone(
	IN	PDM_ODM_T	pDM_Odm,
	pu4Byte		PSD_report,
	u1Byte 		initial_gain_psd
)
{
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	//PADAPTER	pAdapter;
	
	u4Byte	psd_report;

	//2 Switch to CH11 to patch CH9 and CH13 DC tone
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, 11);
	
	if(pDM_Odm->SupportICType== ODM_RTL8192D)
	{
		if((*(pDM_Odm->pMacPhyMode) == ODM_SMSP)||(*(pDM_Odm->pMacPhyMode) == ODM_DMSP))
		{
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_CHNLBW, 0x3FF, 11);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x25, 0xfffff, 0x643BC);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x26, 0xfffff, 0xFC038);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x27, 0xfffff, 0x77C1A);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2B, 0xfffff, 0x41289);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2C, 0xfffff, 0x01840);
		}
		else
		{
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x25, 0xfffff, 0x643BC);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x26, 0xfffff, 0xFC038);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x27, 0xfffff, 0x77C1A);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2B, 0xfffff, 0x41289);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2C, 0xfffff, 0x01840);
		}
	}
	
	//Ch9 DC tone patch
	psd_report = GetPSDData(pDM_Odm, 96, initial_gain_psd);
	PSD_report[50] = psd_report;
	//Ch13 DC tone patch
	psd_report = GetPSDData(pDM_Odm, 32, initial_gain_psd);
	PSD_report[70] = psd_report;
	
	//2 Switch to CH3 to patch CH1 and CH5 DC tone
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, 3);

	
	if(pDM_Odm->SupportICType==ODM_RTL8192D)
	{
		if((*(pDM_Odm->pMacPhyMode) == ODM_SMSP)||(*(pDM_Odm->pMacPhyMode) == ODM_DMSP))
		{
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_CHNLBW, 0x3FF, 3);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_B, 0x25, 0xfffff, 0x643BC);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_B, 0x26, 0xfffff, 0xFC038);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x27, 0xfffff, 0x07C1A);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_B, 0x2B, 0xfffff, 0x61289);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_B, 0x2C, 0xfffff, 0x01C41);
		}
		else
		{
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x25, 0xfffff, 0x643BC);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x26, 0xfffff, 0xFC038);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x27, 0xfffff, 0x07C1A);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x2B, 0xfffff, 0x61289);
			//PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x2C, 0xfffff, 0x01C41);
		}
	}
	
	//Ch1 DC tone patch
	psd_report = GetPSDData(pDM_Odm, 96, initial_gain_psd);
	PSD_report[10] = psd_report;
	//Ch5 DC tone patch
	psd_report = GetPSDData(pDM_Odm, 32, initial_gain_psd);
	PSD_report[30] = psd_report;

}


VOID
GoodChannelDecision(
	PDM_ODM_T	pDM_Odm,
	pu4Byte		PSD_report,
	pu1Byte		PSD_bitmap,
	u1Byte 		RSSI_BT,
	pu1Byte		PSD_bitmap_memory)
{
	pRXHP_T			pRX_HP_Table = &pDM_Odm->DM_RXHP_Table;
	//s4Byte	TH1 =  SSBT-0x15;    // modify TH by Neil Chen
	s4Byte	TH1= RSSI_BT+0x14;
	s4Byte	TH2 = RSSI_BT+85;
	//u2Byte    TH3;
//	s4Byte	RegB34;
	u1Byte	bitmap, Smooth_size[3], Smooth_TH[3];
	//u1Byte	psd_bit;
	u4Byte	i,n,j, byte_idx, bit_idx, good_cnt, good_cnt_smoothing, Smooth_Interval[3];
	int 		start_byte_idx,start_bit_idx,cur_byte_idx, cur_bit_idx,NOW_byte_idx ;
	
//	RegB34 = PHY_QueryBBReg(Adapter,0xB34, bMaskDWord)&0xFF;

	if((pDM_Odm->SupportICType == ODM_RTL8192C)||(pDM_Odm->SupportICType == ODM_RTL8192D))
       {
            TH1 = RSSI_BT + 0x14;  
	}

	Smooth_size[0]=Smooth_Size_1;
	Smooth_size[1]=Smooth_Size_2;
	Smooth_size[2]=Smooth_Size_3;
	Smooth_TH[0]=Smooth_TH_1;
	Smooth_TH[1]=Smooth_TH_2;
	Smooth_TH[2]=Smooth_TH_3;
	Smooth_Interval[0]=16;
	Smooth_Interval[1]=15;
	Smooth_Interval[2]=13;
	good_cnt = 0;
	if(pDM_Odm->SupportICType==ODM_RTL8723A)
	{
		//2 Threshold  

		if(RSSI_BT >=41)
			TH1 = 113;	
		else if(RSSI_BT >=38)   // >= -15dBm
			TH1 = 105;                              //0x69
		else if((RSSI_BT >=33)&(RSSI_BT <38))
			TH1 = 99+(RSSI_BT-33);         //0x63
		else if((RSSI_BT >=26)&(RSSI_BT<33))
			TH1 = 99-(33-RSSI_BT)+2;     //0x5e
	 	else if((RSSI_BT >=24)&(RSSI_BT<26))
			TH1 = 88-((RSSI_BT-24)*3);   //0x58
		else if((RSSI_BT >=18)&(RSSI_BT<24))
			TH1 = 77+((RSSI_BT-18)*2);
		else if((RSSI_BT >=14)&(RSSI_BT<18))
			TH1 = 63+((RSSI_BT-14)*2);
		else if((RSSI_BT >=8)&(RSSI_BT<14))
			TH1 = 58+((RSSI_BT-8)*2);
		else if((RSSI_BT >=3)&(RSSI_BT<8))
			TH1 = 52+(RSSI_BT-3);
		else
			TH1 = 51;
	}

	for (i = 0; i< 10; i++)
		PSD_bitmap[i] = 0;
	

	 // Add By Gary
       for (i=0; i<80; i++)
	   	pRX_HP_Table->PSD_bitmap_RXHP[i] = 0;
	// End



	if(pDM_Odm->SupportICType==ODM_RTL8723A)
	{
		TH1 =TH1-SIR_STEP_SIZE;
	}
	while (good_cnt < PSD_CHMIN)
	{
		good_cnt = 0;
		if(pDM_Odm->SupportICType==ODM_RTL8723A)
		{
		if(TH1 ==TH2)
			break;
		if((TH1+SIR_STEP_SIZE) < TH2)
			TH1 += SIR_STEP_SIZE;
		else
			TH1 = TH2;
		}
		else
		{
			if(TH1==(RSSI_BT+0x1E))
             		     break;    
   			if((TH1+2) < (RSSI_BT+0x1E))
				TH1+=3;
		     	else
				TH1 = RSSI_BT+0x1E;	
             
		}
		ODM_RT_TRACE(pDM_Odm,COMP_PSD,DBG_LOUD,("PSD: decision threshold is: %d", TH1));
			 
		for (i = 0; i< 80; i++)
		{
			if((s4Byte)(PSD_report[i]) < TH1)
			{
				byte_idx = i / 8;
				bit_idx = i -8*byte_idx;
				bitmap = PSD_bitmap[byte_idx];
				PSD_bitmap[byte_idx] = bitmap | (u1Byte) (1 << bit_idx);
			}
		}

#if DBG
		ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD: before smoothing\n"));
		for(n=0;n<10;n++)
		{
			//DbgPrint("PSD_bitmap[%u]=%x\n", n, PSD_bitmap[n]);
			for (i = 0; i<8; i++)
				ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD_bitmap[%u] =   %d\n", 2402+n*8+i, (PSD_bitmap[n]&BIT(i))>>i));
		}
#endif
	
		//1 Start of smoothing function

		for (j=0;j<3;j++)
		{
			start_byte_idx=0;
			start_bit_idx=0;
			for(n=0; n<Smooth_Interval[j]; n++)
			{
				good_cnt_smoothing = 0;
				cur_bit_idx = start_bit_idx;
				cur_byte_idx = start_byte_idx;
				for ( i=0; i < Smooth_size[j]; i++)
				{
					NOW_byte_idx = cur_byte_idx + (i+cur_bit_idx)/8;
					if ( (PSD_bitmap[NOW_byte_idx]& BIT( (cur_bit_idx + i)%8)) != 0)
						good_cnt_smoothing++;

				}

				if( good_cnt_smoothing < Smooth_TH[j] )
				{
					cur_bit_idx = start_bit_idx;
					cur_byte_idx = start_byte_idx;
					for ( i=0; i< Smooth_size[j] ; i++)
					{	
						NOW_byte_idx = cur_byte_idx + (i+cur_bit_idx)/8;				
						PSD_bitmap[NOW_byte_idx] = PSD_bitmap[NOW_byte_idx] & (~BIT( (cur_bit_idx + i)%8));
					}
				}
				start_bit_idx =  start_bit_idx + Smooth_Step_Size;
				while ( (start_bit_idx)  > 7 )
				{
					start_byte_idx= start_byte_idx+start_bit_idx/8;
					start_bit_idx = start_bit_idx%8;
				}
			}

			ODM_RT_TRACE(	pDM_Odm,COMP_PSD, DBG_LOUD,("PSD: after %u smoothing", j+1));
			for(n=0;n<10;n++)
			{
				for (i = 0; i<8; i++)
				{
					ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD_bitmap[%u] =   %d\n", 2402+n*8+i, (PSD_bitmap[n]&BIT(i))>>i));
					
					if ( ((PSD_bitmap[n]&BIT(i))>>i) ==1)  //----- Add By Gary
					{
	                                   pRX_HP_Table->PSD_bitmap_RXHP[8*n+i] = 1;
					}                                                  // ------end by Gary
				}
			}

		}

	
		good_cnt = 0;
		for ( i = 0; i < 10; i++)
		{
			for (n = 0; n < 8; n++)
				if((PSD_bitmap[i]& BIT(n)) != 0)
					good_cnt++;
		}
		ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD: good channel cnt = %u",good_cnt));
	}

	//RT_TRACE(COMP_PSD, DBG_LOUD,("PSD: SSBT=%d, TH2=%d, TH1=%d",SSBT,TH2,TH1));
	for (i = 0; i <10; i++)
		ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD: PSD_bitmap[%u]=%x",i,PSD_bitmap[i]));
/*	
	//Update bitmap memory
	for(i = 0; i < 80; i++)
	{
		byte_idx = i / 8;
		bit_idx = i -8*byte_idx;
		psd_bit = (PSD_bitmap[byte_idx] & BIT(bit_idx)) >> bit_idx;
		bitmap = PSD_bitmap_memory[i]; 
		PSD_bitmap_memory[i] = (bitmap << 1) |psd_bit;
	}
*/
}



VOID
odm_PSD_Monitor(
	PDM_ODM_T	pDM_Odm
)
{
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	//PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;

	unsigned int 		pts, start_point, stop_point;
	u1Byte			initial_gain ;
	static u1Byte		PSD_bitmap_memory[80], init_memory = 0;
	static u1Byte 		psd_cnt=0;
	static u4Byte		PSD_report[80], PSD_report_tmp;
	static u8Byte		lastTxOkCnt=0, lastRxOkCnt=0;
	u1Byte 			H2C_PSD_DATA[5]={0,0,0,0,0};
	static u1Byte		H2C_PSD_DATA_last[5] ={0,0,0,0,0};
	u1Byte			idx[20]={96,99,102,106,109,112,115,118,122,125,
					0,3,6,10,13,16,19,22,26,29};
	u1Byte			n, i, channel, BBReset,tone_idx;
	u1Byte			PSD_bitmap[10], SSBT=0,initial_gain_psd=0, RSSI_BT=0, initialGainUpper;
	s4Byte    			PSD_skip_start, PSD_skip_stop;
	u4Byte			CurrentChannel, RXIQI, RxIdleLowPwr, wlan_channel;
	u4Byte			ReScan, Interval, Is40MHz;
	u8Byte			curTxOkCnt, curRxOkCnt;
	int 				cur_byte_idx, cur_bit_idx;
	PADAPTER		Adapter = pDM_Odm->Adapter;
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	
	if( (*(pDM_Odm->pbScanInProcess)) ||
		pDM_Odm->bLinkInProcess)
	{
		if((pDM_Odm->SupportICType==ODM_RTL8723A)&(pDM_Odm->SupportInterface==ODM_ITRF_PCIE))
		{
			ODM_SetTimer( pDM_Odm, &pDM_Odm->PSDTimer, 1500); //ms	
			//psd_cnt=0;
		}
		return;
	}

	if(pDM_Odm->bBtHsOperation)
	{
		ReScan = 1;
		Interval = SCAN_INTERVAL;
	}
	else
	{
	ReScan = PSD_RESCAN;
	Interval = SCAN_INTERVAL;
	}

	//1 Initialization
	if(init_memory == 0)
	{
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("Init memory\n"));
		for(i = 0; i < 80; i++)
			PSD_bitmap_memory[i] = 0xFF; // channel is always good
		init_memory = 1;
	}
	if(psd_cnt == 0)
	{
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("Enter dm_PSD_Monitor\n"));
		for(i = 0; i < 80; i++)
			PSD_report[i] = 0;
	}

	//1 Backup Current Settings
	CurrentChannel = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);
/*
	if(pDM_Odm->SupportICType==ODM_RTL8192D)
	{
		//2 Record Current synthesizer parameters based on current channel
		if((*pDM_Odm->MacPhyMode92D == SINGLEMAC_SINGLEPHY)||(*pDM_Odm->MacPhyMode92D == DUALMAC_SINGLEPHY))
		{
			SYN_RF25 = ODM_GetRFReg(Adapter, ODM_RF_PATH_B, 0x25, bMaskDWord);
			SYN_RF26 = ODM_GetRFReg(Adapter, ODM_RF_PATH_B, 0x26, bMaskDWord);
			SYN_RF27 = ODM_GetRFReg(Adapter, ODM_RF_PATH_B, 0x27, bMaskDWord);
			SYN_RF2B = ODM_GetRFReg(Adapter, ODM_RF_PATH_B, 0x2B, bMaskDWord);
			SYN_RF2C = ODM_GetRFReg(Adapter, ODM_RF_PATH_B, 0x2C, bMaskDWord);
       	}
		else     // DualMAC_DualPHY 2G
		{
			SYN_RF25 = ODM_GetRFReg(Adapter, ODM_RF_PATH_A, 0x25, bMaskDWord);
			SYN_RF26 = ODM_GetRFReg(Adapter, ODM_RF_PATH_A, 0x26, bMaskDWord);
			SYN_RF27 = ODM_GetRFReg(Adapter, ODM_RF_PATH_A, 0x27, bMaskDWord);
			SYN_RF2B = ODM_GetRFReg(Adapter, ODM_RF_PATH_A, 0x2B, bMaskDWord);
			SYN_RF2C = ODM_GetRFReg(Adapter, ODM_RF_PATH_A, 0x2C, bMaskDWord);
		}
	}
*/
	//RXIQI = PHY_QueryBBReg(Adapter, 0xC14, bMaskDWord);
	RXIQI = ODM_GetBBReg(pDM_Odm, 0xC14, bMaskDWord);

	//RxIdleLowPwr = (PHY_QueryBBReg(Adapter, 0x818, bMaskDWord)&BIT28)>>28;
	RxIdleLowPwr = (ODM_GetBBReg(pDM_Odm, 0x818, bMaskDWord)&BIT28)>>28;

	//2???
	if(CHNL_RUN_ABOVE_40MHZ(pMgntInfo))
		Is40MHz = TRUE;
	else
		Is40MHz = FALSE;

	ODM_RT_TRACE(pDM_Odm,	ODM_COMP_PSD, DBG_LOUD,("PSD Scan Start\n"));
	//1 Turn off CCK
	//PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT24, 0);
	ODM_SetBBReg(pDM_Odm, rFPGA0_RFMOD, BIT24, 0);
	//1 Turn off TX
	//Pause TX Queue
	//PlatformEFIOWrite1Byte(Adapter, REG_TXPAUSE, 0xFF);
	ODM_Write1Byte(pDM_Odm,REG_TXPAUSE, 0xFF);
	
	//Force RX to stop TX immediately
	//PHY_SetRFReg(Adapter, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask, 0x32E13);

	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask, 0x32E13);
	//1 Turn off RX
	//Rx AGC off  RegC70[0]=0, RegC7C[20]=0
	//PHY_SetBBReg(Adapter, 0xC70, BIT0, 0);
	//PHY_SetBBReg(Adapter, 0xC7C, BIT20, 0);

	ODM_SetBBReg(pDM_Odm, 0xC70, BIT0, 0);
	ODM_SetBBReg(pDM_Odm, 0xC7C, BIT20, 0);

	
	//Turn off CCA
	//PHY_SetBBReg(Adapter, 0xC14, bMaskDWord, 0x0);
	ODM_SetBBReg(pDM_Odm, 0xC14, bMaskDWord, 0x0);
	
	//BB Reset
	//BBReset = PlatformEFIORead1Byte(Adapter, 0x02);
	BBReset = ODM_Read1Byte(pDM_Odm, 0x02);
	
	//PlatformEFIOWrite1Byte(Adapter, 0x02, BBReset&(~BIT0));
	//PlatformEFIOWrite1Byte(Adapter, 0x02, BBReset|BIT0);
	ODM_SetBBReg(pDM_Odm, 0x87C, BIT31, 1); //clock gated to prevent from AGC table mess 
	ODM_Write1Byte(pDM_Odm,  0x02, BBReset&(~BIT0));
	ODM_Write1Byte(pDM_Odm,  0x02, BBReset|BIT0);
	ODM_SetBBReg(pDM_Odm, 0x87C, BIT31, 0);
	
	//1 Leave RX idle low power
	//PHY_SetBBReg(Adapter, 0x818, BIT28, 0x0);

	ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 0x0);
	//1 Fix initial gain
	//if (IS_HARDWARE_TYPE_8723AE(Adapter))
	//RSSI_BT = pHalData->RSSI_BT;
       //else if((IS_HARDWARE_TYPE_8192C(Adapter))||(IS_HARDWARE_TYPE_8192D(Adapter)))      // Add by Gary
       //    RSSI_BT = RSSI_BT_new;

	if((pDM_Odm->SupportICType==ODM_RTL8723A)&(pDM_Odm->SupportInterface==ODM_ITRF_PCIE))
	RSSI_BT=pDM_Odm->RSSI_BT;		//need to check C2H to pDM_Odm RSSI BT

	if(RSSI_BT>=47)
		RSSI_BT=47;
	   
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("PSD: RSSI_BT= %d\n", RSSI_BT));
	
	if(pDM_Odm->SupportICType==ODM_RTL8723A)
	{
	       //Neil add--2011--10--12
		//2 Initial Gain index 
		if(RSSI_BT >=35)   // >= -15dBm
			initial_gain_psd = RSSI_BT*2;
		else if((RSSI_BT >=33)&(RSSI_BT<35))
			initial_gain_psd = RSSI_BT*2+6;
		else if((RSSI_BT >=24)&(RSSI_BT<33))
			initial_gain_psd = 70-(33-RSSI_BT);
	 	else if((RSSI_BT >=19)&(RSSI_BT<24))
			initial_gain_psd = 64-((24-RSSI_BT)*4);
		else if((RSSI_BT >=14)&(RSSI_BT<19))
			initial_gain_psd = 44-((18-RSSI_BT)*2);
		else if((RSSI_BT >=8)&(RSSI_BT<14))
			initial_gain_psd = 35-(14-RSSI_BT);
		else
			initial_gain_psd = 0x1B;
	}
	else
	{
	
		//need to do	
         	initial_gain_psd = pDM_Odm->RSSI_Min;    // PSD report based on RSSI
           	//}  	
	}
	//if(RSSI_BT<0x17)
	//	RSSI_BT +=3;
	//DbgPrint("PSD: RSSI_BT= %d\n", RSSI_BT);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("PSD: RSSI_BT= %d\n", RSSI_BT));

	//initialGainUpper = 0x5E;  //Modify by neil chen
	
	if(pDM_Odm->bUserAssignLevel)
	{
		pDM_Odm->bUserAssignLevel = FALSE;
		initialGainUpper = 0x7f;
	}
	else
	{
		initialGainUpper = 0x5E;
	}
	
	/*
	if (initial_gain_psd < 0x1a)
		initial_gain_psd = 0x1a;
	if (initial_gain_psd > initialGainUpper)
		initial_gain_psd = initialGainUpper;
	*/

	//if(pDM_Odm->SupportICType==ODM_RTL8723A)
	SSBT = RSSI_BT  * 2 +0x3E;
	
	
	//if(IS_HARDWARE_TYPE_8723AE(Adapter))
	//	SSBT = RSSI_BT  * 2 +0x3E;
	//else if((IS_HARDWARE_TYPE_8192C(Adapter))||(IS_HARDWARE_TYPE_8192D(Adapter)))   // Add by Gary
	//{
	//	RSSI_BT = initial_gain_psd;
	//	SSBT = RSSI_BT;
	//}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("PSD: SSBT= %d\n", SSBT));
	ODM_RT_TRACE(	pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("PSD: initial gain= 0x%x\n", initial_gain_psd));
	//DbgPrint("PSD: SSBT= %d", SSBT);
	//need to do
	//pMgntInfo->bDMInitialGainEnable = FALSE;
	pDM_Odm->bDMInitialGainEnable = FALSE;
	initial_gain =(u1Byte) (ODM_GetBBReg(pDM_Odm, 0xc50, bMaskDWord) & 0x7F);
	
        // make sure the initial gain is under the correct range.
	//initial_gain_psd &= 0x7f;
	ODM_Write_DIG(pDM_Odm, initial_gain_psd);
	//1 Turn off 3-wire
	ODM_SetBBReg(pDM_Odm, 0x88c, BIT20|BIT21|BIT22|BIT23, 0xF);

	//pts value = 128, 256, 512, 1024
	pts = 128;

	if(pts == 128)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x0);
		start_point = 64;
		stop_point = 192;
	}
	else if(pts == 256)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x1);
		start_point = 128;
		stop_point = 384;
	}
	else if(pts == 512)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x2);
		start_point = 256;
		stop_point = 768;
	}
	else
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x3);
		start_point = 512;
		stop_point = 1536;
	}
	

//3 Skip WLAN channels if WLAN busy

	curTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast) - lastTxOkCnt;
	curRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast) - lastRxOkCnt;
	lastTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast);
	lastRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast);	

	PSD_skip_start=80;
	PSD_skip_stop = 0;
	wlan_channel = CurrentChannel & 0x0f;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD,DBG_LOUD,("PSD: current channel: %x, BW:%d \n", wlan_channel, Is40MHz));
	if(pDM_Odm->SupportICType==ODM_RTL8723A)
	{
		if(pDM_Odm->bBtHsOperation)
		{
			if(pDM_Odm->bLinked)
			{
				if(Is40MHz)
				{
					PSD_skip_start = ((wlan_channel-1)*5 -Is40MHz*10)-2;  // Modify by Neil to add 10 chs to mask
					PSD_skip_stop = (PSD_skip_start + (1+Is40MHz)*20)+4;
				}
				else
				{
					PSD_skip_start = ((wlan_channel-1)*5 -Is40MHz*10)-10;  // Modify by Neil to add 10 chs to mask
					PSD_skip_stop = (PSD_skip_start + (1+Is40MHz)*20)+18; 
				}
			}
			else
			{
				// mask for 40MHz
				PSD_skip_start = ((wlan_channel-1)*5 -Is40MHz*10)-2;  // Modify by Neil to add 10 chs to mask
				PSD_skip_stop = (PSD_skip_start + (1+Is40MHz)*20)+4;
			}
			if(PSD_skip_start < 0)
				PSD_skip_start = 0;
			if(PSD_skip_stop >80)
				PSD_skip_stop = 80;
		}
		else
		{
			if((curRxOkCnt+curTxOkCnt) > 5)
			{
				if(Is40MHz)
				{
					PSD_skip_start = ((wlan_channel-1)*5 -Is40MHz*10)-2;  // Modify by Neil to add 10 chs to mask
					PSD_skip_stop = (PSD_skip_start + (1+Is40MHz)*20)+4;
				}
				else
				{
					PSD_skip_start = ((wlan_channel-1)*5 -Is40MHz*10)-10;  // Modify by Neil to add 10 chs to mask
					PSD_skip_stop = (PSD_skip_start + (1+Is40MHz)*20)+18; 
				}
				
				if(PSD_skip_start < 0)
					PSD_skip_start = 0;
				if(PSD_skip_stop >80)
					PSD_skip_stop = 80;
			}
		}
	}
#if 0	
	else
	{
		if((curRxOkCnt+curTxOkCnt) > 1000)
		{
			PSD_skip_start = (wlan_channel-1)*5 -Is40MHz*10;
			PSD_skip_stop = PSD_skip_start + (1+Is40MHz)*20;
		}
	}   
#endif  //Reove RXHP Issue
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD,DBG_LOUD,("PSD: Skip tone from %d to %d \n", PSD_skip_start, PSD_skip_stop));

 	for (n=0;n<80;n++)
 	{
 		if((n%20)==0)
 		{
			channel = (n/20)*4 + 1;
					
					ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, channel);
				}
		tone_idx = n%20;
		if ((n>=PSD_skip_start) && (n<PSD_skip_stop))
		{	
			PSD_report[n] = SSBT;
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD,DBG_LOUD,("PSD:Tone %d skipped \n", n));
		}
		else
		{
			PSD_report_tmp =  GetPSDData(pDM_Odm, idx[tone_idx], initial_gain_psd);

			if ( PSD_report_tmp > PSD_report[n])
				PSD_report[n] = PSD_report_tmp;
				
		}
	}

	PatchDCTone(pDM_Odm, PSD_report, initial_gain_psd);
      
       //----end
	//1 Turn on RX
	//Rx AGC on
	ODM_SetBBReg(pDM_Odm, 0xC70, BIT0, 1);
	ODM_SetBBReg(pDM_Odm, 0xC7C, BIT20, 1);
	//CCK on
	ODM_SetBBReg(pDM_Odm, rFPGA0_RFMOD, BIT24, 1);
	//1 Turn on TX
	//Resume TX Queue
	
	ODM_Write1Byte(pDM_Odm,REG_TXPAUSE, 0x00);
	//Turn on 3-wire
	ODM_SetBBReg(pDM_Odm, 0x88c, BIT20|BIT21|BIT22|BIT23, 0x0);
	//1 Restore Current Settings
	//Resume DIG
	pDM_Odm->bDMInitialGainEnable = TRUE;
	
	ODM_Write_DIG(pDM_Odm, initial_gain);

	// restore originl center frequency
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, CurrentChannel);

	//Turn on CCA
	ODM_SetBBReg(pDM_Odm, 0xC14, bMaskDWord, RXIQI);
	//Restore RX idle low power
	if(RxIdleLowPwr == TRUE)
		ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 1);
	
	psd_cnt++;
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("PSD:psd_cnt = %d \n",psd_cnt));
	if (psd_cnt < ReScan)
		ODM_SetTimer(pDM_Odm, &pDM_Odm->PSDTimer, Interval);		
	else
	{
		psd_cnt = 0;
		for(i=0;i<80;i++)
			//DbgPrint("psd_report[%d]=     %d \n", 2402+i, PSD_report[i]);
			RT_TRACE(	COMP_PSD, DBG_LOUD,("psd_report[%d]=     %d \n", 2402+i, PSD_report[i]));


		GoodChannelDecision(pDM_Odm, PSD_report, PSD_bitmap,RSSI_BT, PSD_bitmap_memory);

		if(pDM_Odm->SupportICType==ODM_RTL8723A)
		{
			cur_byte_idx=0;
			cur_bit_idx=0;

			//2 Restore H2C PSD Data to Last Data
		  	H2C_PSD_DATA_last[0] = H2C_PSD_DATA[0];
			H2C_PSD_DATA_last[1] = H2C_PSD_DATA[1];
			H2C_PSD_DATA_last[2] = H2C_PSD_DATA[2];
			H2C_PSD_DATA_last[3] = H2C_PSD_DATA[3];
			H2C_PSD_DATA_last[4] = H2C_PSD_DATA[4];

	
			//2 Translate 80bit channel map to 40bit channel	
			for ( i=0;i<5;i++)
			{
				for(n=0;n<8;n++)
				{
					cur_byte_idx = i*2 + n/4;
					cur_bit_idx = (n%4)*2;
					if ( ((PSD_bitmap[cur_byte_idx]& BIT(cur_bit_idx)) != 0) && ((PSD_bitmap[cur_byte_idx]& BIT(cur_bit_idx+1)) != 0))
						H2C_PSD_DATA[i] = H2C_PSD_DATA[i] | (u1Byte) (1 << n);
				}
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("H2C_PSD_DATA[%d]=0x%x\n" ,i, H2C_PSD_DATA[i]));
			}
	
			//3 To Compare the difference
			for ( i=0;i<5;i++)
			{
				if(H2C_PSD_DATA[i] !=H2C_PSD_DATA_last[i])
				{
					FillH2CCmd(Adapter, H2C_92C_PSD_RESULT, 5, H2C_PSD_DATA);
					ODM_RT_TRACE(pDM_Odm, ODM_COMP_PSD, DBG_LOUD,("Need to Update the AFH Map \n"));
					break;
				}
				else
				{
					if(i==5)
						ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("Not need to Update\n"));	
				}
			}
			if(pDM_Odm->bBtHsOperation)
			{
				ODM_SetTimer(pDM_Odm, &pDM_Odm->PSDTimer, 10000);
				ODM_RT_TRACE(	pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("Leave dm_PSD_Monitor\n"));		
			}
			else
			{
				ODM_SetTimer(pDM_Odm, &pDM_Odm->PSDTimer, 1500);
				ODM_RT_TRACE(	pDM_Odm,ODM_COMP_PSD, DBG_LOUD,("Leave dm_PSD_Monitor\n"));		
		}
	}
    }
}
/*
//Neil for Get BT RSSI
// Be Triggered by BT C2H CMD
VOID
ODM_PSDGetRSSI(
	IN	u1Byte	RSSI_BT)
{


}

*/

VOID
ODM_PSDMonitor(
	IN	PDM_ODM_T	pDM_Odm
	)
{
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	//if(IS_HARDWARE_TYPE_8723AE(Adapter))
	
	if(pDM_Odm->SupportICType == ODM_RTL8723A)   //may need to add other IC type
	{
		if(pDM_Odm->SupportInterface==ODM_ITRF_PCIE)
		{
			if(!pDM_Odm->bBtEnabled) //need to check upper layer connection
			{
				pDM_Odm->bPSDactive=FALSE;
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD, ("odm_PSDMonitor, return for BT is disabled!!!\n"));
		   		return; 
			}

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_PSD, DBG_LOUD, ("odm_PSDMonitor\n"));
		//{
			pDM_Odm->bPSDinProcess = TRUE;
	 		pDM_Odm->bPSDactive=TRUE;
			odm_PSD_Monitor(pDM_Odm);
			pDM_Odm->bPSDinProcess = FALSE;
		}	
	}	

}
VOID
odm_PSDMonitorCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter = (PADAPTER)pTimer->Adapter;
       HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	PlatformScheduleWorkItem(&pHalData->PSDMonitorWorkitem);
}

VOID
odm_PSDMonitorWorkItemCallback(
    IN PVOID            pContext
    )
{
	PADAPTER	Adapter = (PADAPTER)pContext;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;

	ODM_PSDMonitor(pDM_Odm);
}

//Remove by Yuchen (seperate to odm_DIG.c)

 //cosa debug tool need to modify

VOID
ODM_PSDDbgControl(
	IN	PADAPTER	Adapter,
	IN	u4Byte		mode,
	IN	u4Byte		btRssi
	)
{
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;

	ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD, (" Monitor mode=%d, btRssi=%d\n", mode, btRssi));
	if(mode)
	{
		pDM_Odm->RSSI_BT = (u1Byte)btRssi;
		pDM_Odm->bUserAssignLevel = TRUE;
		ODM_SetTimer( pDM_Odm, &pDM_Odm->PSDTimer, 0); //ms		
	}
	else
	{
		ODM_CancelTimer(pDM_Odm, &pDM_Odm->PSDTimer);
	}
#endif
}


//#if(DEV_BUS_TYPE == RT_PCI_INTERFACE)|(DEV_BUS_TYPE == RT_USB_INTERFACE)

void	odm_RXHPInit(
	IN		PDM_ODM_T		pDM_Odm)
{
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE)|(DEV_BUS_TYPE == RT_USB_INTERFACE)
	pRXHP_T			pRX_HP_Table  = &pDM_Odm->DM_RXHP_Table;
   	u1Byte			index;

	pRX_HP_Table->RXHP_enable = TRUE;
	pRX_HP_Table->RXHP_flag = 0;
	pRX_HP_Table->PSD_func_trigger = 0;
	pRX_HP_Table->Pre_IGI = 0x20;
	pRX_HP_Table->Cur_IGI = 0x20;
	pRX_HP_Table->Cur_pw_th = pw_th_10dB;
	pRX_HP_Table->Pre_pw_th = pw_th_10dB;
	for(index=0; index<80; index++)
		pRX_HP_Table->PSD_bitmap_RXHP[index] = 1;

#if(DEV_BUS_TYPE == RT_USB_INTERFACE)
	pRX_HP_Table->TP_Mode = Idle_Mode;
#endif
#endif
}

void odm_RXHP(
	IN		PDM_ODM_T		pDM_Odm)
{
#if( DM_ODM_SUPPORT_TYPE & (ODM_WIN))
#if (DEV_BUS_TYPE == RT_PCI_INTERFACE) | (DEV_BUS_TYPE == RT_USB_INTERFACE)
	PADAPTER	Adapter =  pDM_Odm->Adapter;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	pDIG_T		pDM_DigTable = &pDM_Odm->DM_DigTable;
	pRXHP_T		pRX_HP_Table  = &pDM_Odm->DM_RXHP_Table;
       PFALSE_ALARM_STATISTICS		FalseAlmCnt = &(pDM_Odm->FalseAlmCnt);
	
	u1Byte              	i, j, sum;
	u1Byte			Is40MHz;
	s1Byte              	Intf_diff_idx, MIN_Intf_diff_idx = 16;   
       s4Byte              	cur_channel;    
       u1Byte              	ch_map_intf_5M[17] = {0};     
       static u4Byte		FA_TH = 0;	
	static u1Byte      	psd_intf_flag = 0;
	static s4Byte      	curRssi = 0;                
       static s4Byte  		preRssi = 0;                                                                
	static u1Byte		PSDTriggerCnt = 1;
	
	u1Byte			RX_HP_enable = (u1Byte)(ODM_GetBBReg(pDM_Odm, rOFDM0_XAAGCCore2, bMaskDWord)>>31);   // for debug!!

#if(DEV_BUS_TYPE == RT_USB_INTERFACE)	
	static s8Byte  		lastTxOkCnt = 0, lastRxOkCnt = 0;  
       s8Byte			curTxOkCnt, curRxOkCnt;
	s8Byte			curTPOkCnt;
	s8Byte			TP_Acc3, TP_Acc5;
	static s8Byte		TP_Buff[5] = {0};
	static u1Byte		pre_state = 0, pre_state_flag = 0;
	static u1Byte		Intf_HighTP_flag = 0, De_counter = 16; 
	static u1Byte		TP_Degrade_flag = 0;
#endif	   
	static u1Byte		LatchCnt = 0;
	
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8188E))
		return;
	//AGC RX High Power Mode is only applied on 2G band in 92D!!!
	if(pDM_Odm->SupportICType == ODM_RTL8192D)
	{
		if(*(pDM_Odm->pBandType) != ODM_BAND_2_4G)
			return;
	}

	if(!(pDM_Odm->SupportAbility==ODM_BB_RXHP))
		return;


	//RX HP ON/OFF
	if(RX_HP_enable == 1)
		pRX_HP_Table->RXHP_enable = FALSE;
	else
		pRX_HP_Table->RXHP_enable = TRUE;

	if(pRX_HP_Table->RXHP_enable == FALSE)
	{
		if(pRX_HP_Table->RXHP_flag == 1)
		{
			pRX_HP_Table->RXHP_flag = 0;
			psd_intf_flag = 0;
		}
		return;
	}

#if(DEV_BUS_TYPE == RT_USB_INTERFACE)	
	//2 Record current TP for USB interface
	curTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast)-lastTxOkCnt;
	curRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast)-lastRxOkCnt;
	lastTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast);
	lastRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast);

	curTPOkCnt = curTxOkCnt+curRxOkCnt;
	TP_Buff[0] = curTPOkCnt;    // current TP  
	TP_Acc3 = PlatformDivision64((TP_Buff[1]+TP_Buff[2]+TP_Buff[3]), 3);
	TP_Acc5 = PlatformDivision64((TP_Buff[0]+TP_Buff[1]+TP_Buff[2]+TP_Buff[3]+TP_Buff[4]), 5);
	
	if(TP_Acc5 < 1000)
		pRX_HP_Table->TP_Mode = Idle_Mode;
	else if((1000 < TP_Acc5)&&(TP_Acc5 < 3750000))
		pRX_HP_Table->TP_Mode = Low_TP_Mode;
	else
		pRX_HP_Table->TP_Mode = High_TP_Mode;

	ODM_RT_TRACE(pDM_Odm, 	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RX HP TP Mode = %d\n", pRX_HP_Table->TP_Mode));
	// Since TP result would be sampled every 2 sec, it needs to delay 4sec to wait PSD processing.
	// When LatchCnt = 0, we would Get PSD result.
	if(TP_Degrade_flag == 1)
	{
		LatchCnt--;
		if(LatchCnt == 0)
		{
			TP_Degrade_flag = 0;
		}
	}
	// When PSD function triggered by TP degrade 20%, and Interference Flag = 1
	// Set a De_counter to wait IGI = upper bound. If time is UP, the Interference flag will be pull down.
	if(Intf_HighTP_flag == 1)
	{
		De_counter--;
		if(De_counter == 0)
		{
			Intf_HighTP_flag = 0;
			psd_intf_flag = 0;
		}
	}
#endif

	//2 AGC RX High Power Mode by PSD only applied to STA Mode
	//3 NOT applied 1. Ad Hoc Mode.
	//3 NOT applied 2. AP Mode
	if ((pMgntInfo->mAssoc) && (!pMgntInfo->mIbss) && (!ACTING_AS_AP(Adapter)))
	{    
		Is40MHz = *(pDM_Odm->pBandWidth);
		curRssi = pDM_Odm->RSSI_Min;
		cur_channel = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x0fff) & 0x0f;
		ODM_RT_TRACE(pDM_Odm, 	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RXHP RX HP flag = %d\n", pRX_HP_Table->RXHP_flag));
		ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RXHP FA = %d\n", FalseAlmCnt->Cnt_all));
		ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RXHP cur RSSI = %d, pre RSSI=%d\n", curRssi, preRssi));
		ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RXHP current CH = %d\n", cur_channel));
		ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RXHP Is 40MHz = %d\n", Is40MHz));
       	//2 PSD function would be triggered 
       	//3 1. Every 4 sec for PCIE
       	//3 2. Before TP Mode (Idle TP<4kbps) for USB
       	//3 3. After TP Mode (High TP) for USB 
		if((curRssi > 68) && (pRX_HP_Table->RXHP_flag == 0))	// Only RSSI>TH and RX_HP_flag=0 will Do PSD process 
		{
#if (DEV_BUS_TYPE == RT_USB_INTERFACE)
			//2 Before TP Mode ==> PSD would be trigger every 4 sec
			if(pRX_HP_Table->TP_Mode == Idle_Mode)		//2.1 less wlan traffic <4kbps
			{
#endif
				if(PSDTriggerCnt == 1)       
				{    	
					odm_PSD_RXHP(pDM_Odm);
					pRX_HP_Table->PSD_func_trigger = 1;
					PSDTriggerCnt = 0;
				}
				else
				{
             				PSDTriggerCnt++;
				}
#if(DEV_BUS_TYPE == RT_USB_INTERFACE)
			}	
			//2 After TP Mode ==> Check if TP degrade larger than 20% would trigger PSD function
			if(pRX_HP_Table->TP_Mode == High_TP_Mode)
			{
				if((pre_state_flag == 0)&&(LatchCnt == 0)) 
				{
					// TP var < 5%
					if((((curTPOkCnt-TP_Acc3)*20)<(TP_Acc3))&&(((curTPOkCnt-TP_Acc3)*20)>(-TP_Acc3)))
					{
						pre_state++;
						if(pre_state == 3)      // hit pre_state condition => consecutive 3 times
						{
							pre_state_flag = 1;
							pre_state = 0;
						}

					}
					else
					{
						pre_state = 0;
					}
				}
				//3 If pre_state_flag=1 ==> start to monitor TP degrade 20%
				if(pre_state_flag == 1)		
				{
					if(((TP_Acc3-curTPOkCnt)*5)>(TP_Acc3))      // degrade 20%
					{
						odm_PSD_RXHP(pDM_Odm);
						pRX_HP_Table->PSD_func_trigger = 1;
						TP_Degrade_flag = 1;
						LatchCnt = 2;
						pre_state_flag = 0;
					}
					else if(((TP_Buff[2]-curTPOkCnt)*5)>TP_Buff[2])
					{
						odm_PSD_RXHP(pDM_Odm);
						pRX_HP_Table->PSD_func_trigger = 1;
						TP_Degrade_flag = 1;
						LatchCnt = 2;
						pre_state_flag = 0;
					}
					else if(((TP_Buff[3]-curTPOkCnt)*5)>TP_Buff[3])
					{
						odm_PSD_RXHP(pDM_Odm);
						pRX_HP_Table->PSD_func_trigger = 1;
						TP_Degrade_flag = 1;
						LatchCnt = 2;
						pre_state_flag = 0;
					}
				}
			}
#endif
}

#if (DEV_BUS_TYPE == RT_USB_INTERFACE)
		for (i=0;i<4;i++)
		{
			TP_Buff[4-i] = TP_Buff[3-i];
		}
#endif
		//2 Update PSD bitmap according to PSD report 
		if((pRX_HP_Table->PSD_func_trigger == 1)&&(LatchCnt == 0))
    		{	
           		//2 Separate 80M bandwidth into 16 group with smaller 5M BW.
			for (i = 0 ; i < 16 ; i++)
           		{
				sum = 0;
				for(j = 0; j < 5 ; j++)
                			sum += pRX_HP_Table->PSD_bitmap_RXHP[5*i + j];
            
                		if(sum < 5)
                		{
                			ch_map_intf_5M[i] = 1;  // interference flag
                		}
           		}
			//=============just for debug=========================
			//for(i=0;i<16;i++)
				//DbgPrint("RX HP: ch_map_intf_5M[%d] = %d\n", i, ch_map_intf_5M[i]);
			//===============================================
			//2 Mask target channel 5M index
	    		for(i = 0; i < (4+4*Is40MHz) ; i++)
           		{
				ch_map_intf_5M[cur_channel - (1+2*Is40MHz) + i] = 0;  
           		}
				
           		psd_intf_flag = 0;
	    		for(i = 0; i < 16; i++)
           		{
         			if(ch_map_intf_5M[i] == 1)
	              	{
	              		psd_intf_flag = 1;            // interference is detected!!!	
	              		break;
         			}
	    		}
				
#if (DEV_BUS_TYPE == RT_USB_INTERFACE)
			if(pRX_HP_Table->TP_Mode!=Idle_Mode)
			{
				if(psd_intf_flag == 1)     // to avoid psd_intf_flag always 1
				{
					Intf_HighTP_flag = 1;
					De_counter = 32;     // 0x1E -> 0x3E needs 32 times by each IGI step =1
				}
			}
#endif
			ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RX HP psd_intf_flag = %d\n", psd_intf_flag));
			//2 Distance between target channel and interference
           		for(i = 0; i < 16; i++)
          		{
				if(ch_map_intf_5M[i] == 1)
                		{
					Intf_diff_idx = ((cur_channel+Is40MHz-(i+1))>0) ? (s1Byte)(cur_channel-2*Is40MHz-(i-2)) : (s1Byte)((i+1)-(cur_channel+2*Is40MHz));  
                      		if(Intf_diff_idx < MIN_Intf_diff_idx)
						MIN_Intf_diff_idx = Intf_diff_idx;    // the min difference index between interference and target
		  		}
	    		}
	    		ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RX HP MIN_Intf_diff_idx = %d\n", MIN_Intf_diff_idx)); 
			//2 Choose False Alarm Threshold
			switch (MIN_Intf_diff_idx){
      				case 0: 
	   			case 1:
	        		case 2:
	        		case 3:	 	 
                 			FA_TH = FA_RXHP_TH1;  
                     		break;
	        		case 4:				// CH5
	        		case 5:				// CH6
		   			FA_TH = FA_RXHP_TH2;	
               			break;
	        		case 6:				// CH7
	        		case 7:				// CH8
		      			FA_TH = FA_RXHP_TH3;
                    			break; 
               		case 8:				// CH9
	        		case 9:				//CH10
		      			FA_TH = FA_RXHP_TH4;
                    			break; 	
	        		case 10:
	        		case 11:
	        		case 12:
	        		case 13:	 
	        		case 14:
	      			case 15:	 	
		      			FA_TH = FA_RXHP_TH5;
                    			break;  		
       		}	
			ODM_RT_TRACE(pDM_Odm,	ODM_COMP_RXHP, ODM_DBG_LOUD, ("RX HP FA_TH = %d\n", FA_TH));
			pRX_HP_Table->PSD_func_trigger = 0;
		}
		//1 Monitor RSSI variation to choose the suitable IGI or Exit AGC RX High Power Mode
         	if(pRX_HP_Table->RXHP_flag == 1)
         	{
              	if ((curRssi > 80)&&(preRssi < 80))
              	{ 
                   		pRX_HP_Table->Cur_IGI = LNA_Low_Gain_1;
              	}
              	else if ((curRssi < 80)&&(preRssi > 80))
              	{
                   		pRX_HP_Table->Cur_IGI = LNA_Low_Gain_2;
			}
	       	else if ((curRssi > 72)&&(preRssi < 72))
	      		{
                		pRX_HP_Table->Cur_IGI = LNA_Low_Gain_2;
	       	}
              	else if ((curRssi < 72)&&( preRssi > 72))
	     		{
                   		pRX_HP_Table->Cur_IGI = LNA_Low_Gain_3;
	       	}
	       	else if (curRssi < 68)		 //RSSI is NOT large enough!!==> Exit AGC RX High Power Mode
	       	{
                   		pRX_HP_Table->Cur_pw_th = pw_th_10dB;
				pRX_HP_Table->RXHP_flag = 0;    // Back to Normal DIG Mode		  
				psd_intf_flag = 0;
			}
		}
		else    // pRX_HP_Table->RXHP_flag == 0
		{
			//1 Decide whether to enter AGC RX High Power Mode
			if ((curRssi > 70) && (psd_intf_flag == 1) && (FalseAlmCnt->Cnt_all > FA_TH) &&  
				(pDM_DigTable->CurIGValue == pDM_DigTable->rx_gain_range_max))
			{
             			if (curRssi > 80)
             			{
					pRX_HP_Table->Cur_IGI = LNA_Low_Gain_1;
				}
				else if (curRssi > 72) 
              		{
               			pRX_HP_Table->Cur_IGI = LNA_Low_Gain_2;
				}
             			else
            			{
                   			pRX_HP_Table->Cur_IGI = LNA_Low_Gain_3;
				}
           			pRX_HP_Table->Cur_pw_th = pw_th_16dB;		//RegC54[9:8]=2'b11: to enter AGC Flow 3
				pRX_HP_Table->First_time_enter = TRUE;
				pRX_HP_Table->RXHP_flag = 1;    //	RXHP_flag=1: AGC RX High Power Mode, RXHP_flag=0: Normal DIG Mode
			}
		}
		preRssi = curRssi; 
		odm_Write_RXHP(pDM_Odm);	
	}
#endif //#if( DM_ODM_SUPPORT_TYPE & (ODM_WIN))
#endif //#if (DEV_BUS_TYPE == RT_PCI_INTERFACE) | (DEV_BUS_TYPE == RT_USB_INTERFACE)
}

void odm_Write_RXHP(
	IN	PDM_ODM_T	pDM_Odm)
{
	pRXHP_T		pRX_HP_Table = &pDM_Odm->DM_RXHP_Table;
	u4Byte		currentIGI;

	if(pRX_HP_Table->Cur_IGI != pRX_HP_Table->Pre_IGI)
	{
		ODM_SetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, bMaskByte0, pRX_HP_Table->Cur_IGI);
	     	ODM_SetBBReg(pDM_Odm, rOFDM0_XBAGCCore1, bMaskByte0, pRX_HP_Table->Cur_IGI);	
	}
	
	if(pRX_HP_Table->Cur_pw_th != pRX_HP_Table->Pre_pw_th)
{
		ODM_SetBBReg(pDM_Odm, rOFDM0_XAAGCCore2, BIT8|BIT9, pRX_HP_Table->Cur_pw_th);  // RegC54[9:8]=2'b11:  AGC Flow 3
	}

	if(pRX_HP_Table->RXHP_flag == 0)
	{
		pRX_HP_Table->Cur_IGI = 0x20;
	}
	else
	{
		currentIGI = ODM_GetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, bMaskByte0);
		if(currentIGI<0x50)
		{
			ODM_SetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, bMaskByte0, pRX_HP_Table->Cur_IGI);
	     		ODM_SetBBReg(pDM_Odm, rOFDM0_XBAGCCore1, bMaskByte0, pRX_HP_Table->Cur_IGI);	
		}
	}
	pRX_HP_Table->Pre_IGI = pRX_HP_Table->Cur_IGI;
	pRX_HP_Table->Pre_pw_th = pRX_HP_Table->Cur_pw_th;

}

VOID
odm_PSD_RXHP(
	IN	PDM_ODM_T	pDM_Odm
)
{
	pRXHP_T			pRX_HP_Table  = &pDM_Odm->DM_RXHP_Table;
	PADAPTER		Adapter =  pDM_Odm->Adapter;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	unsigned int 		pts, start_point, stop_point, initial_gain ;
	static u1Byte		PSD_bitmap_memory[80], init_memory = 0;
	static u1Byte 		psd_cnt=0;
	static u4Byte		PSD_report[80], PSD_report_tmp;
	static u8Byte		lastTxOkCnt=0, lastRxOkCnt=0;
	u1Byte			idx[20]={96,99,102,106,109,112,115,118,122,125,
					0,3,6,10,13,16,19,22,26,29};
	u1Byte			n, i, channel, BBReset,tone_idx;
	u1Byte			PSD_bitmap[10]/*, SSBT=0*/,initial_gain_psd=0, RSSI_BT=0, initialGainUpper;
	s4Byte    			PSD_skip_start, PSD_skip_stop;
	u4Byte			CurrentChannel, RXIQI, RxIdleLowPwr, wlan_channel;
	u4Byte			ReScan, Interval, Is40MHz;
	u8Byte			curTxOkCnt, curRxOkCnt;
	//--------------2G band synthesizer for 92D switch RF channel using----------------- 
	u1Byte			group_idx=0;
	u4Byte			SYN_RF25=0, SYN_RF26=0, SYN_RF27=0, SYN_RF2B=0, SYN_RF2C=0;
	u4Byte			SYN[5] = {0x25, 0x26, 0x27, 0x2B, 0x2C};    // synthesizer RF register for 2G channel
	u4Byte			SYN_group[3][5] = {{0x643BC, 0xFC038, 0x77C1A, 0x41289, 0x01840},     // For CH1,2,4,9,10.11.12   {0x643BC, 0xFC038, 0x77C1A, 0x41289, 0x01840}
									    {0x643BC, 0xFC038, 0x07C1A, 0x41289, 0x01840},     // For CH3,13,14
									    {0x243BC, 0xFC438, 0x07C1A, 0x4128B, 0x0FC41}};   // For Ch5,6,7,8
       //--------------------- Add by Gary for Debug setting ----------------------
  	u1Byte                 RSSI_BT_new = (u1Byte) ODM_GetBBReg(pDM_Odm, 0xB9C, 0xFF);
       u1Byte                 rssi_ctrl = (u1Byte) ODM_GetBBReg(pDM_Odm, 0xB38, 0xFF);
       //---------------------------------------------------------------------
	
	if(pMgntInfo->bScanInProgress)
	{
		return;
	}

	ReScan = PSD_RESCAN;
	Interval = SCAN_INTERVAL;


	//1 Initialization
	if(init_memory == 0)
	{
		RT_TRACE(	COMP_PSD, DBG_LOUD,("Init memory\n"));
		for(i = 0; i < 80; i++)
			PSD_bitmap_memory[i] = 0xFF; // channel is always good
		init_memory = 1;
	}
	if(psd_cnt == 0)
	{
		RT_TRACE(COMP_PSD, DBG_LOUD,("Enter dm_PSD_Monitor\n"));
		for(i = 0; i < 80; i++)
			PSD_report[i] = 0;
	}

	//1 Backup Current Settings
	CurrentChannel = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask);
	if(pDM_Odm->SupportICType == ODM_RTL8192D)
	{
		//2 Record Current synthesizer parameters based on current channel
		if((*(pDM_Odm->pMacPhyMode)==ODM_SMSP)||(*(pDM_Odm->pMacPhyMode)==ODM_DMSP))
		{
			SYN_RF25 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x25, bMaskDWord);
			SYN_RF26 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x26, bMaskDWord);
			SYN_RF27 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x27, bMaskDWord);
			SYN_RF2B = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2B, bMaskDWord);
			SYN_RF2C = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2C, bMaskDWord);
       	}
		else     // DualMAC_DualPHY 2G
		{
			SYN_RF25 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x25, bMaskDWord);
			SYN_RF26 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x26, bMaskDWord);
			SYN_RF27 = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x27, bMaskDWord);
			SYN_RF2B = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2B, bMaskDWord);
			SYN_RF2C = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2C, bMaskDWord);
		}
	}
	RXIQI = ODM_GetBBReg(pDM_Odm, 0xC14, bMaskDWord);
	RxIdleLowPwr = (ODM_GetBBReg(pDM_Odm, 0x818, bMaskDWord)&BIT28)>>28;
	Is40MHz = *(pDM_Odm->pBandWidth);
	ODM_RT_TRACE(pDM_Odm,	COMP_PSD, DBG_LOUD,("PSD Scan Start\n"));
	//1 Turn off CCK
	ODM_SetBBReg(pDM_Odm, rFPGA0_RFMOD, BIT24, 0);
	//1 Turn off TX
	//Pause TX Queue
	ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0xFF);
	//Force RX to stop TX immediately
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bRFRegOffsetMask, 0x32E13);
	//1 Turn off RX
	//Rx AGC off  RegC70[0]=0, RegC7C[20]=0
	ODM_SetBBReg(pDM_Odm, 0xC70, BIT0, 0);
	ODM_SetBBReg(pDM_Odm, 0xC7C, BIT20, 0);
	//Turn off CCA
	ODM_SetBBReg(pDM_Odm, 0xC14, bMaskDWord, 0x0);
	//BB Reset
	ODM_SetBBReg(pDM_Odm, 0x87C, BIT31, 1); //clock gated to prevent from AGC table mess 
	BBReset = ODM_Read1Byte(pDM_Odm, 0x02);
	ODM_Write1Byte(pDM_Odm, 0x02, BBReset&(~BIT0));
	ODM_Write1Byte(pDM_Odm, 0x02, BBReset|BIT0);
	ODM_SetBBReg(pDM_Odm, 0x87C, BIT31, 0);
	//1 Leave RX idle low power
	ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 0x0);
	//1 Fix initial gain
      	RSSI_BT = RSSI_BT_new;
	RT_TRACE(COMP_PSD, DBG_LOUD,("PSD: RSSI_BT= %d\n", RSSI_BT));
	
	if(rssi_ctrl == 1)        // just for debug!!
		initial_gain_psd = RSSI_BT_new; 
     	else
		initial_gain_psd = pDM_Odm->RSSI_Min;    // PSD report based on RSSI
	
	RT_TRACE(COMP_PSD, DBG_LOUD,("PSD: RSSI_BT= %d\n", RSSI_BT));
	
	initialGainUpper = 0x54;
	
	RSSI_BT = initial_gain_psd;
	//SSBT = RSSI_BT;
	
	//RT_TRACE(	COMP_PSD, DBG_LOUD,("PSD: SSBT= %d\n", SSBT));
	RT_TRACE(	COMP_PSD, DBG_LOUD,("PSD: initial gain= 0x%x\n", initial_gain_psd));
	
	pDM_Odm->bDMInitialGainEnable = FALSE;		
	initial_gain = ODM_GetBBReg(pDM_Odm, 0xc50, bMaskDWord) & 0x7F;
	//ODM_SetBBReg(pDM_Odm, 0xc50, 0x7F, initial_gain_psd);	
	ODM_Write_DIG(pDM_Odm, initial_gain_psd);
	//1 Turn off 3-wire
	ODM_SetBBReg(pDM_Odm, 0x88c, BIT20|BIT21|BIT22|BIT23, 0xF);

	//pts value = 128, 256, 512, 1024
	pts = 128;

	if(pts == 128)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x0);
		start_point = 64;
		stop_point = 192;
	}
	else if(pts == 256)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x1);
		start_point = 128;
		stop_point = 384;
	}
	else if(pts == 512)
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x2);
		start_point = 256;
		stop_point = 768;
	}
	else
	{
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x3);
		start_point = 512;
		stop_point = 1536;
	}
	

//3 Skip WLAN channels if WLAN busy
	curTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast) - lastTxOkCnt;
	curRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast) - lastRxOkCnt;
	lastTxOkCnt = *(pDM_Odm->pNumTxBytesUnicast);
	lastRxOkCnt = *(pDM_Odm->pNumRxBytesUnicast);
	
	PSD_skip_start=80;
	PSD_skip_stop = 0;
	wlan_channel = CurrentChannel & 0x0f;

	RT_TRACE(COMP_PSD,DBG_LOUD,("PSD: current channel: %x, BW:%d \n", wlan_channel, Is40MHz));
	
	if((curRxOkCnt+curTxOkCnt) > 1000)
	{
		PSD_skip_start = (wlan_channel-1)*5 -Is40MHz*10;
		PSD_skip_stop = PSD_skip_start + (1+Is40MHz)*20;
	}

	RT_TRACE(COMP_PSD,DBG_LOUD,("PSD: Skip tone from %d to %d \n", PSD_skip_start, PSD_skip_stop));

 	for (n=0;n<80;n++)
 	{
 		if((n%20)==0)
 		{
			channel = (n/20)*4 + 1;
			if(pDM_Odm->SupportICType == ODM_RTL8192D)
			{
				switch(channel)
				{
					case 1: 
					case 9:
						group_idx = 0;
						break;
					case 5:
						group_idx = 2;
						break;
					case 13:
				 		group_idx = 1;
						break;
				}
				if((*(pDM_Odm->pMacPhyMode)==ODM_SMSP)||(*(pDM_Odm->pMacPhyMode)==ODM_DMSP))   
		{
					for(i = 0; i < SYN_Length; i++)
						ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, SYN[i], bMaskDWord, SYN_group[group_idx][i]);

					ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, channel);
					ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_CHNLBW, 0x3FF, channel);
				}
				else  // DualMAC_DualPHY 2G
			{
					for(i = 0; i < SYN_Length; i++)
						ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, SYN[i], bMaskDWord, SYN_group[group_idx][i]);   
					
					ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, channel);
				}
			}
			else
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, 0x3FF, channel);
			}	
		tone_idx = n%20;
		if ((n>=PSD_skip_start) && (n<PSD_skip_stop))
		{	
			PSD_report[n] = initial_gain_psd;//SSBT;
			ODM_RT_TRACE(pDM_Odm,COMP_PSD,DBG_LOUD,("PSD:Tone %d skipped \n", n));
		}
		else
		{
			PSD_report_tmp =  GetPSDData(pDM_Odm, idx[tone_idx], initial_gain_psd);

			if ( PSD_report_tmp > PSD_report[n])
				PSD_report[n] = PSD_report_tmp;
				
		}
	}

	PatchDCTone(pDM_Odm, PSD_report, initial_gain_psd);
      
       //----end
	//1 Turn on RX
	//Rx AGC on
	ODM_SetBBReg(pDM_Odm, 0xC70, BIT0, 1);
	ODM_SetBBReg(pDM_Odm, 0xC7C, BIT20, 1);
	//CCK on
	ODM_SetBBReg(pDM_Odm, rFPGA0_RFMOD, BIT24, 1);
	//1 Turn on TX
	//Resume TX Queue
	ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0x00);
	//Turn on 3-wire
	ODM_SetBBReg(pDM_Odm, 0x88c, BIT20|BIT21|BIT22|BIT23, 0x0);
	//1 Restore Current Settings
	//Resume DIG
	pDM_Odm->bDMInitialGainEnable= TRUE;
	//ODM_SetBBReg(pDM_Odm, 0xc50, 0x7F, initial_gain);
	ODM_Write_DIG(pDM_Odm,(u1Byte) initial_gain);
	// restore originl center frequency
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask, CurrentChannel);
	if(pDM_Odm->SupportICType == ODM_RTL8192D)
	{
		if((*(pDM_Odm->pMacPhyMode)==ODM_SMSP)||(*(pDM_Odm->pMacPhyMode)==ODM_DMSP))
		{
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_CHNLBW, bMaskDWord, CurrentChannel);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x25, bMaskDWord, SYN_RF25);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x26, bMaskDWord, SYN_RF26);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x27, bMaskDWord, SYN_RF27);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2B, bMaskDWord, SYN_RF2B);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, 0x2C, bMaskDWord, SYN_RF2C);
		}
		else     // DualMAC_DualPHY
		{
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x25, bMaskDWord, SYN_RF25);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x26, bMaskDWord, SYN_RF26);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x27, bMaskDWord, SYN_RF27);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2B, bMaskDWord, SYN_RF2B);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x2C, bMaskDWord, SYN_RF2C);
		}
	}
	//Turn on CCA
	ODM_SetBBReg(pDM_Odm, 0xC14, bMaskDWord, RXIQI);
	//Restore RX idle low power
	if(RxIdleLowPwr == TRUE)
		ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 1);
	
	psd_cnt++;
	//gPrint("psd cnt=%d\n", psd_cnt);
	ODM_RT_TRACE(pDM_Odm,COMP_PSD, DBG_LOUD,("PSD:psd_cnt = %d \n",psd_cnt));
	if (psd_cnt < ReScan)
	{
		ODM_SetTimer(pDM_Odm, &pRX_HP_Table->PSDTimer, Interval);  //ms
	}
	else
			{	
		psd_cnt = 0;
		for(i=0;i<80;i++)
			RT_TRACE(	COMP_PSD, DBG_LOUD,("psd_report[%d]=     %d \n", 2402+i, PSD_report[i]));
			//DbgPrint("psd_report[%d]=     %d \n", 2402+i, PSD_report[i]);

		GoodChannelDecision(pDM_Odm, PSD_report, PSD_bitmap,RSSI_BT, PSD_bitmap_memory);

			}
		}

VOID
odm_PSD_RXHPCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter = (PADAPTER)pTimer->Adapter;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
	pRXHP_T			pRX_HP_Table  = &pDM_Odm->DM_RXHP_Table;
	
#if DEV_BUS_TYPE==RT_PCI_INTERFACE
	#if USE_WORKITEM
	ODM_ScheduleWorkItem(&pRX_HP_Table->PSDTimeWorkitem);
	#else
	odm_PSD_RXHP(pDM_Odm);
	#endif
#else
	ODM_ScheduleWorkItem(&pRX_HP_Table->PSDTimeWorkitem);
#endif
	
	}

VOID
odm_PSD_RXHPWorkitemCallback(
    IN PVOID            pContext
    )
{
	PADAPTER	pAdapter = (PADAPTER)pContext;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;
	
	odm_PSD_RXHP(pDM_Odm);
}

#endif //#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

//Remove PathDiversity related function to odm_PathDiv.c

#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN| ODM_CE))

VOID
odm_PHY_SaveAFERegisters(
	IN	PDM_ODM_T	pDM_Odm,
	IN	pu4Byte		AFEReg,
	IN	pu4Byte		AFEBackup,
	IN	u4Byte		RegisterNum
	)
{
	u4Byte	i;
	
	//RT_DISP(FINIT, INIT_IQK, ("Save ADDA parameters.\n"));
	for( i = 0 ; i < RegisterNum ; i++){
		AFEBackup[i] = ODM_GetBBReg(pDM_Odm, AFEReg[i], bMaskDWord);
	}
}

VOID
odm_PHY_ReloadAFERegisters(
	IN	PDM_ODM_T	pDM_Odm,
	IN	pu4Byte		AFEReg,
	IN	pu4Byte		AFEBackup,
	IN	u4Byte		RegiesterNum
	)
{
	u4Byte	i;

	//RT_DISP(FINIT, INIT_IQK, ("Reload ADDA power saving parameters !\n"));
	for(i = 0 ; i < RegiesterNum; i++)
	{
	
		ODM_SetBBReg(pDM_Odm, AFEReg[i], bMaskDWord, AFEBackup[i]);
	}
}

//
// Description:
//	Set Single/Dual Antenna default setting for products that do not do detection in advance.
//
// Added by Joseph, 2012.03.22
//
VOID
ODM_SingleDualAntennaDefaultSetting(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	pSWAT_T		pDM_SWAT_Table = &pDM_Odm->DM_SWAT_Table;
	PADAPTER	pAdapter	 =  pDM_Odm->Adapter;
	u1Byte btAntNum = 2;
#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN))
	btAntNum=BT_GetPgAntNum(pAdapter);
#elif (DM_ODM_SUPPORT_TYPE & (ODM_CE))
#ifdef CONFIG_BT_COEXIST
	btAntNum = hal_btcoex_GetPgAntNum(pAdapter);
#endif
#endif

	// Set default antenna A and B status
	if(btAntNum == 2)
	{
		pDM_SWAT_Table->ANTA_ON=TRUE;
		pDM_SWAT_Table->ANTB_ON=TRUE;
		//RT_TRACE(COMP_ANTENNA, DBG_LOUD, ("Dual antenna\n"));
	}
#ifdef CONFIG_BT_COEXIST
	else if(btAntNum == 1)
	{// Set antenna A as default
		pDM_SWAT_Table->ANTA_ON=TRUE;
		pDM_SWAT_Table->ANTB_ON=FALSE;
		//RT_TRACE(COMP_ANTENNA, DBG_LOUD, ("Single antenna\n"));
	}
	else
	{
		//RT_ASSERT(FALSE, ("Incorrect antenna number!!\n"));
	}
#endif
}



//2 8723A ANT DETECT
//
// Description:
//	Implement IQK single tone for RF DPK loopback and BB PSD scanning. 
//	This function is cooperated with BB team Neil. 
//
// Added by Roger, 2011.12.15
//
BOOLEAN
ODM_SingleDualAntennaDetection(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u1Byte			mode
	)
{
	PADAPTER	pAdapter	 =  pDM_Odm->Adapter;
	pSWAT_T		pDM_SWAT_Table = &pDM_Odm->DM_SWAT_Table;
	u4Byte		CurrentChannel,RfLoopReg;
	u1Byte		n;
	u4Byte		Reg88c, Regc08, Reg874, Regc50, Reg948=0, Regb2c=0, Reg92c=0, AFE_rRx_Wait_CCA=0;
	u1Byte		initial_gain = 0x5a;
	u4Byte		PSD_report_tmp;
	u4Byte		AntA_report = 0x0, AntB_report = 0x0,AntO_report=0x0;
	BOOLEAN		bResult = TRUE;
	u4Byte		AFE_Backup[16];
	u4Byte		AFE_REG_8723A[16] = {
					rRx_Wait_CCA, 	rTx_CCK_RFON, 
					rTx_CCK_BBON, 	rTx_OFDM_RFON,
					rTx_OFDM_BBON, 	rTx_To_Rx,
					rTx_To_Tx, 		rRx_CCK, 
					rRx_OFDM, 		rRx_Wait_RIFS, 
					rRx_TO_Rx,		rStandby,
					rSleep,			rPMPD_ANAEN, 	
					rFPGA0_XCD_SwitchControl, rBlue_Tooth};

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection()============> \n"));	

	
	if(!(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C|ODM_RTL8723B)))
		return bResult;

	// Retrieve antenna detection registry info, added by Roger, 2012.11.27.
	if(!IS_ANT_DETECT_SUPPORT_SINGLE_TONE(pAdapter))
		return bResult;

	if(pDM_Odm->SupportICType == ODM_RTL8192C)
	{
		//Which path in ADC/DAC is turnned on for PSD: both I/Q
		ODM_SetBBReg(pDM_Odm, 0x808, BIT10|BIT11, 0x3);
		//Ageraged number: 8
		ODM_SetBBReg(pDM_Odm, 0x808, BIT12|BIT13, 0x1);
		//pts = 128;
		ODM_SetBBReg(pDM_Odm, 0x808, BIT14|BIT15, 0x0);
	}

	//1 Backup Current RF/BB Settings	
	
	CurrentChannel = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, ODM_CHANNEL, bRFRegOffsetMask);
	RfLoopReg = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask);
	if(!(pDM_Odm->SupportICType == ODM_RTL8723B))
	ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, ODM_DPDT, Antenna_A);  // change to Antenna A
#if (RTL8723B_SUPPORT == 1)
	else
	{
		Reg92c = ODM_GetBBReg(pDM_Odm, 0x92c, bMaskDWord);
		Reg948 = ODM_GetBBReg(pDM_Odm, rS0S1_PathSwitch, bMaskDWord);
		Regb2c = ODM_GetBBReg(pDM_Odm, AGC_table_select, bMaskDWord);
		ODM_SetBBReg(pDM_Odm, rDPDT_control, 0x3, 0x1);
		ODM_SetBBReg(pDM_Odm, rfe_ctrl_anta_src, 0xff, 0x77);
		ODM_SetBBReg(pDM_Odm, rS0S1_PathSwitch, 0x3ff, 0x000);
		ODM_SetBBReg(pDM_Odm, AGC_table_select, BIT31, 0x0);
	}
#endif
	ODM_StallExecution(10);
	
	//Store A Path Register 88c, c08, 874, c50
	Reg88c = ODM_GetBBReg(pDM_Odm, rFPGA0_AnalogParameter4, bMaskDWord);
	Regc08 = ODM_GetBBReg(pDM_Odm, rOFDM0_TRMuxPar, bMaskDWord);
	Reg874 = ODM_GetBBReg(pDM_Odm, rFPGA0_XCD_RFInterfaceSW, bMaskDWord);
	Regc50 = ODM_GetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, bMaskDWord);	
	
	// Store AFE Registers
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
	odm_PHY_SaveAFERegisters(pDM_Odm, AFE_REG_8723A, AFE_Backup, 16);	
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
		AFE_rRx_Wait_CCA = ODM_GetBBReg(pDM_Odm, rRx_Wait_CCA,bMaskDWord);
	
	//Set PSD 128 pts
	ODM_SetBBReg(pDM_Odm, rFPGA0_PSDFunction, BIT14|BIT15, 0x0);  //128 pts
	
	// To SET CH1 to do
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, ODM_CHANNEL, bRFRegOffsetMask, 0x7401);     //Channel 1
	
	// AFE all on step
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
	{
		ODM_SetBBReg(pDM_Odm, rRx_Wait_CCA, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_CCK_RFON, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_CCK_BBON, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_OFDM_RFON, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_OFDM_BBON, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_To_Rx, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rTx_To_Tx, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rRx_CCK, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rRx_OFDM, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rRx_Wait_RIFS, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rRx_TO_Rx, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rStandby, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rSleep, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rPMPD_ANAEN, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rFPGA0_XCD_SwitchControl, bMaskDWord, 0x6FDB25A4);
		ODM_SetBBReg(pDM_Odm, rBlue_Tooth, bMaskDWord, 0x6FDB25A4);
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
	{
		ODM_SetBBReg(pDM_Odm, rRx_Wait_CCA, bMaskDWord, 0x01c00016);
	}

	// 3 wire Disable
	ODM_SetBBReg(pDM_Odm, rFPGA0_AnalogParameter4, bMaskDWord, 0xCCF000C0);
	
	//BB IQK Setting
	ODM_SetBBReg(pDM_Odm, rOFDM0_TRMuxPar, bMaskDWord, 0x000800E4);
	ODM_SetBBReg(pDM_Odm, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, 0x22208000);

	//IQK setting tone@ 4.34Mhz
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x10008C1C);
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);	

	//Page B init
	ODM_SetBBReg(pDM_Odm, rConfig_AntA, bMaskDWord, 0x00080000);
	ODM_SetBBReg(pDM_Odm, rConfig_AntA, bMaskDWord, 0x0f600000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x10008c1f);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82150008);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28150008);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x001028d0);	

	//RF loop Setting
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x0, 0xFFFFF, 0x50008);	
	
	//IQK Single tone start
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);
	
	ODM_StallExecution(10000);

	// PSD report of antenna A
	PSD_report_tmp=0x0;
	for (n=0;n<2;n++)
 	{
 		PSD_report_tmp =  GetPSDData(pDM_Odm, 14, initial_gain);	
		if(PSD_report_tmp >AntA_report)
			AntA_report=PSD_report_tmp;
	}

	 // change to Antenna B
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
		ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, ODM_DPDT, Antenna_B); 
#if (RTL8723B_SUPPORT == 1)
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
		ODM_SetBBReg(pDM_Odm, rDPDT_control, 0x3, 0x2);
#endif

	ODM_StallExecution(10);	

	// PSD report of antenna B
	PSD_report_tmp=0x0;
	for (n=0;n<2;n++)
 	{
 		PSD_report_tmp =  GetPSDData(pDM_Odm, 14, initial_gain);	
		if(PSD_report_tmp > AntB_report)
			AntB_report=PSD_report_tmp;
	}

	// change to open case
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
		ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, ODM_DPDT, 0);  // change to Antenna A
#if (RTL8723B_SUPPORT == 1)
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
		ODM_SetBBReg(pDM_Odm, rDPDT_control, 0x3, 0x0);
#endif

	ODM_StallExecution(10);	
	
	// PSD report of open case
	PSD_report_tmp=0x0;
	for (n=0;n<2;n++)
 	{
 		PSD_report_tmp =  GetPSDData(pDM_Odm, 14, initial_gain);	
		if(PSD_report_tmp > AntO_report)
			AntO_report=PSD_report_tmp;
	}

	//Close IQK Single Tone function
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);	

	//1 Return to antanna A
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
		ODM_SetBBReg(pDM_Odm, rFPGA0_XA_RFInterfaceOE, ODM_DPDT, Antenna_A);  // change to Antenna A
#if (RTL8723B_SUPPORT == 1)
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
	{
		// external DPDT
		ODM_SetBBReg(pDM_Odm, rDPDT_control, bMaskDWord, Reg92c);

		//internal S0/S1
		ODM_SetBBReg(pDM_Odm, rS0S1_PathSwitch, bMaskDWord, Reg948);
		ODM_SetBBReg(pDM_Odm, AGC_table_select, bMaskDWord, Regb2c);
	}
#endif
	ODM_SetBBReg(pDM_Odm, rFPGA0_AnalogParameter4, bMaskDWord, Reg88c);
	ODM_SetBBReg(pDM_Odm, rOFDM0_TRMuxPar, bMaskDWord, Regc08);
	ODM_SetBBReg(pDM_Odm, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, Reg874);
	ODM_SetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, 0x7F, 0x40);
	ODM_SetBBReg(pDM_Odm, rOFDM0_XAAGCCore1, bMaskDWord, Regc50);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bRFRegOffsetMask,CurrentChannel);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask,RfLoopReg);

	//Reload AFE Registers
	if(pDM_Odm->SupportICType & (ODM_RTL8723A|ODM_RTL8192C))
	odm_PHY_ReloadAFERegisters(pDM_Odm, AFE_REG_8723A, AFE_Backup, 16);	
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
		ODM_SetBBReg(pDM_Odm, rRx_Wait_CCA, bMaskDWord, AFE_rRx_Wait_CCA);

	if(pDM_Odm->SupportICType == ODM_RTL8723A)
	{
		//2 Test Ant B based on Ant A is ON
		if(mode==ANTTESTB)
		{
			if(AntA_report >=	100)
			{
				if(AntB_report > (AntA_report+1))
				{
					pDM_SWAT_Table->ANTB_ON=FALSE;
							ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Single Antenna A\n"));		
				}	
				else
				{
					pDM_SWAT_Table->ANTB_ON=TRUE;
							ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Dual Antenna is A and B\n"));	
				}	
			}
			else
			{
							ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Need to check again\n"));
				pDM_SWAT_Table->ANTB_ON=FALSE; // Set Antenna B off as default 
				bResult = FALSE;
			}
		}	
		//2 Test Ant A and B based on DPDT Open
		else if(mode==ANTTESTALL)
		{
			if((AntO_report >=100) && (AntO_report <=118))
			{
				if(AntA_report > (AntO_report+1))
				{
					pDM_SWAT_Table->ANTA_ON=FALSE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("Ant A is OFF\n"));
				}	
				else
				{
					pDM_SWAT_Table->ANTA_ON=TRUE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("Ant A is ON\n"));
				}

				if(AntB_report > (AntO_report+2))
				{
					pDM_SWAT_Table->ANTB_ON=FALSE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("Ant B is OFF\n"));
				}	
				else
				{
					pDM_SWAT_Table->ANTB_ON=TRUE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("Ant B is ON\n"));
				}
				
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_A[%d]= %d \n", 2416, AntA_report));	
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_B[%d]= %d \n", 2416, AntB_report));	
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_O[%d]= %d \n", 2416, AntO_report));
				
				pDM_Odm->AntDetectedInfo.bAntDetected= TRUE;
				pDM_Odm->AntDetectedInfo.dBForAntA = AntA_report;
				pDM_Odm->AntDetectedInfo.dBForAntB = AntB_report;
				pDM_Odm->AntDetectedInfo.dBForAntO = AntO_report;
				
				}
			else
				{
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("return FALSE!!\n"));
				bResult = FALSE;
			}
		}
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8192C)
	{
		if(AntA_report >=	100)
		{
			if(AntB_report > (AntA_report+2))
			{
				pDM_SWAT_Table->ANTA_ON=FALSE;
				pDM_SWAT_Table->ANTB_ON=TRUE;
				ODM_SetBBReg(pDM_Odm,  rFPGA0_XA_RFInterfaceOE, 0x300, Antenna_B);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Single Antenna B\n"));		
			}	
			else if(AntA_report > (AntB_report+2))
			{
				pDM_SWAT_Table->ANTA_ON=TRUE;
				pDM_SWAT_Table->ANTB_ON=FALSE;
				ODM_SetBBReg(pDM_Odm,  rFPGA0_XA_RFInterfaceOE, 0x300, Antenna_A);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Single Antenna A\n"));
			}	
			else
			{
				pDM_SWAT_Table->ANTA_ON=TRUE;
				pDM_SWAT_Table->ANTB_ON=TRUE;
				RT_TRACE(COMP_ANTENNA, DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Dual Antenna \n"));
			}
		}
		else
		{
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Need to check again\n"));
			pDM_SWAT_Table->ANTA_ON=TRUE; // Set Antenna A on as default 
			pDM_SWAT_Table->ANTB_ON=FALSE; // Set Antenna B off as default 
			bResult = FALSE;
		}
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
	{
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_A[%d]= %d \n", 2416, AntA_report));	
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_B[%d]= %d \n", 2416, AntB_report));	
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("psd_report_O[%d]= %d \n", 2416, AntO_report));
		
		//2 Test Ant B based on Ant A is ON
		if(mode==ANTTESTB)
		{
			if(AntA_report >=100 && AntA_report <= 116)
			{
				if(AntB_report >= (AntA_report+4) && AntB_report > 116)
				{
					pDM_SWAT_Table->ANTB_ON=FALSE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Single Antenna A\n"));		
				}	
				else if(AntB_report >=100 && AntB_report <= 116)
				{
					pDM_SWAT_Table->ANTB_ON=TRUE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Dual Antenna is A and B\n"));	
				}
				else
				{
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Need to check again\n"));
					pDM_SWAT_Table->ANTB_ON=FALSE; // Set Antenna B off as default 
					bResult = FALSE;
				}
			}
			else
			{
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Need to check again\n"));
				pDM_SWAT_Table->ANTB_ON=FALSE; // Set Antenna B off as default 
				bResult = FALSE;
			}
		}	
		//2 Test Ant A and B based on DPDT Open
		else if(mode==ANTTESTALL)
		{
			if((AntA_report >= 100) && (AntB_report >= 100) && (AntA_report <= 120) && (AntB_report <= 120))
			{
				if((AntA_report - AntB_report < 2) || (AntB_report - AntA_report < 2))
				{
					pDM_SWAT_Table->ANTA_ON=TRUE;
					pDM_SWAT_Table->ANTB_ON=TRUE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("ODM_SingleDualAntennaDetection(): Dual Antenna\n"));
				}
				else if(((AntA_report - AntB_report >= 2) && (AntA_report - AntB_report <= 4)) || 
					((AntB_report - AntA_report >= 2) && (AntB_report - AntA_report <= 4)))
				{
					pDM_SWAT_Table->ANTA_ON=FALSE;
					pDM_SWAT_Table->ANTB_ON=FALSE;
					bResult = FALSE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD, ("ODM_SingleDualAntennaDetection(): Need to check again\n"));
				}
				else
				{
					pDM_SWAT_Table->ANTA_ON = TRUE;
					pDM_SWAT_Table->ANTB_ON=FALSE;
					ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("ODM_SingleDualAntennaDetection(): Single Antenna A\n"));
				}
				
				pDM_Odm->AntDetectedInfo.bAntDetected= TRUE;
				pDM_Odm->AntDetectedInfo.dBForAntA = AntA_report;
				pDM_Odm->AntDetectedInfo.dBForAntB = AntB_report;
				pDM_Odm->AntDetectedInfo.dBForAntO = AntO_report;
				
			}
			else
			{
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_ANT_DIV, ODM_DBG_LOUD,("return FALSE!!\n"));
				bResult = FALSE;
			}
		}
	}
		
	return bResult;

}


#endif   // end odm_CE

#if (DM_ODM_SUPPORT_TYPE & (ODM_WIN| ODM_CE))

VOID
odm_Set_RA_DM_ARFB_by_Noisy(
	IN	PDM_ODM_T	pDM_Odm
)
{
	//DbgPrint("DM_ARFB ====> \n");
	if (pDM_Odm->bNoisyState){
		ODM_Write4Byte(pDM_Odm,0x430,0x00000000);
		ODM_Write4Byte(pDM_Odm,0x434,0x05040200);
		//DbgPrint("DM_ARFB ====> Noisy State\n");
	}
	else{
		ODM_Write4Byte(pDM_Odm,0x430,0x02010000);
		ODM_Write4Byte(pDM_Odm,0x434,0x07050403);
		//DbgPrint("DM_ARFB ====> Clean State\n");
	}
	
}

VOID
ODM_UpdateNoisyState(
	IN	PDM_ODM_T	pDM_Odm,
	IN 	BOOLEAN 	bNoisyStateFromC2H
	)
{
	//DbgPrint("Get C2H Command! NoisyState=0x%x\n ", bNoisyStateFromC2H);
	if(pDM_Odm->SupportICType == ODM_RTL8821  || pDM_Odm->SupportICType == ODM_RTL8812  || 
	   pDM_Odm->SupportICType == ODM_RTL8723B || pDM_Odm->SupportICType == ODM_RTL8192E || pDM_Odm->SupportICType == ODM_RTL8188E)
	{
		pDM_Odm->bNoisyState = bNoisyStateFromC2H;
	}
	odm_Set_RA_DM_ARFB_by_Noisy(pDM_Odm);
};

u4Byte
Set_RA_DM_Ratrbitmap_by_Noisy(
	IN	PDM_ODM_T	pDM_Odm,
	IN	WIRELESS_MODE	WirelessMode,
	IN	u4Byte			ratr_bitmap,
	IN	u1Byte			rssi_level
)
{
	u4Byte ret_bitmap = ratr_bitmap;
	switch (WirelessMode)
	{
		case WIRELESS_MODE_AC_24G :
		case WIRELESS_MODE_AC_5G :
		case WIRELESS_MODE_AC_ONLY:
			if (pDM_Odm->bNoisyState){ // in Noisy State
				if (rssi_level==1)
					ret_bitmap&=0xfe3f0e08;
				else if (rssi_level==2)
					ret_bitmap&=0xff3f8f8c;
				else if (rssi_level==3)
					ret_bitmap&=0xffffffcc ;
				else
					ret_bitmap&=0xffffffff ;
			}
			else{                                   // in SNR State
				if (rssi_level==1){
					ret_bitmap&=0xfc3e0c08;
				}
				else if (rssi_level==2){
					ret_bitmap&=0xfe3f0e08;
				}
				else if (rssi_level==3){
					ret_bitmap&=0xffbfefcc;
				}
				else{
					ret_bitmap&=0x0fffffff;
				}
			}
			break;
		case WIRELESS_MODE_B:
		case WIRELESS_MODE_A:
		case WIRELESS_MODE_G:
		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G:
			if (pDM_Odm->bNoisyState){
				if (rssi_level==1)
					ret_bitmap&=0x0f0e0c08;
				else if (rssi_level==2)
					ret_bitmap&=0x0f8f0e0c;
				else if (rssi_level==3)
					ret_bitmap&=0x0fefefcc ;
				else
					ret_bitmap&=0xffffffff ;
			}
			else{
				if (rssi_level==1){
					ret_bitmap&=0x0f8f0e08;
				}
				else if (rssi_level==2){
					ret_bitmap&=0x0fcf8f8c;
				}
				else if (rssi_level==3){
					ret_bitmap&=0x0fffffcc;
				}
				else{
					ret_bitmap&=0x0fffffff;
				}
			}
			break;
		default:
			break;
	}
	//DbgPrint("DM_RAMask ====> rssi_LV = %d, BITMAP = %x \n", rssi_level, ret_bitmap);
	return ret_bitmap;

}



VOID
ODM_UpdateInitRate(
	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte		Rate
	)
{
	u1Byte			p = 0;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,("Get C2H Command! Rate=0x%x\n", Rate));
	
	if(pDM_Odm->SupportICType == ODM_RTL8821  || pDM_Odm->SupportICType == ODM_RTL8812  || 
	   pDM_Odm->SupportICType == ODM_RTL8723B || pDM_Odm->SupportICType == ODM_RTL8192E || pDM_Odm->SupportICType == ODM_RTL8188E)
	{
		pDM_Odm->TxRate = Rate;
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	#if DEV_BUS_TYPE==RT_PCI_INTERFACE
		#if USE_WORKITEM
		PlatformScheduleWorkItem(&pDM_Odm->RaRptWorkitem);
		#else
		if(pDM_Odm->SupportICType == ODM_RTL8821)
		{
			ODM_TxPwrTrackSetPwr8821A(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
		}
		else if(pDM_Odm->SupportICType == ODM_RTL8812)
		{
			for (p = ODM_RF_PATH_A; p < MAX_PATH_NUM_8812A; p++) 		
			{
				ODM_TxPwrTrackSetPwr8812A(pDM_Odm, MIX_MODE, p, 0);
			}
		}
		else if(pDM_Odm->SupportICType == ODM_RTL8723B)
		{
			ODM_TxPwrTrackSetPwr_8723B(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
		}
		else if(pDM_Odm->SupportICType == ODM_RTL8192E)
		{
			for (p = ODM_RF_PATH_A; p < MAX_PATH_NUM_8192E; p++) 		
			{
				ODM_TxPwrTrackSetPwr92E(pDM_Odm, MIX_MODE, p, 0);
			}
		}
		else if(pDM_Odm->SupportICType == ODM_RTL8188E)
		{
			ODM_TxPwrTrackSetPwr88E(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
		}
		#endif
	#else
		PlatformScheduleWorkItem(&pDM_Odm->RaRptWorkitem);
	#endif	
#endif
	}
	else
		return;
}

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
VOID
ODM_UpdateInitRateWorkItemCallback(
    IN PVOID            pContext
    )
{
	PADAPTER	Adapter = (PADAPTER)pContext;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &pHalData->DM_OutSrc;

	u1Byte			p = 0;	

	if(pDM_Odm->SupportICType == ODM_RTL8821)
	{
		ODM_TxPwrTrackSetPwr8821A(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8812)
	{
		for (p = ODM_RF_PATH_A; p < MAX_PATH_NUM_8812A; p++)    //DOn't know how to include &c
		{
			ODM_TxPwrTrackSetPwr8812A(pDM_Odm, MIX_MODE, p, 0);
		}
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8723B)
	{
			ODM_TxPwrTrackSetPwr_8723B(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8192E)
	{
		for (p = ODM_RF_PATH_A; p < MAX_PATH_NUM_8192E; p++)    //DOn't know how to include &c
		{
			ODM_TxPwrTrackSetPwr92E(pDM_Odm, MIX_MODE, p, 0);
		}
	}
	else if(pDM_Odm->SupportICType == ODM_RTL8188E)
	{
			ODM_TxPwrTrackSetPwr88E(pDM_Odm, MIX_MODE, ODM_RF_PATH_A, 0);
	}
}
#endif
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_CE)
/* Justin: According to the current RRSI to adjust Response Frame TX power, 2012/11/05 */
void odm_dtc(PDM_ODM_T pDM_Odm)
{
#ifdef CONFIG_DM_RESP_TXAGC
	#define DTC_BASE            35	/* RSSI higher than this value, start to decade TX power */
	#define DTC_DWN_BASE       (DTC_BASE-5)	/* RSSI lower than this value, start to increase TX power */

	/* RSSI vs TX power step mapping: decade TX power */
	static const u8 dtc_table_down[]={
		DTC_BASE,
		(DTC_BASE+5),
		(DTC_BASE+10),
		(DTC_BASE+15),
		(DTC_BASE+20),
		(DTC_BASE+25)
	};

	/* RSSI vs TX power step mapping: increase TX power */
	static const u8 dtc_table_up[]={
		DTC_DWN_BASE,
		(DTC_DWN_BASE-5),
		(DTC_DWN_BASE-10),
		(DTC_DWN_BASE-15),
		(DTC_DWN_BASE-15),
		(DTC_DWN_BASE-20),
		(DTC_DWN_BASE-20),
		(DTC_DWN_BASE-25),
		(DTC_DWN_BASE-25),
		(DTC_DWN_BASE-30),
		(DTC_DWN_BASE-35)
	};

	u8 i;
	u8 dtc_steps=0;
	u8 sign;
	u8 resp_txagc=0;

	#if 0
	/* As DIG is disabled, DTC is also disable */
	if(!(pDM_Odm->SupportAbility & ODM_XXXXXX))
		return;
	#endif

	if (DTC_BASE < pDM_Odm->RSSI_Min) {
		/* need to decade the CTS TX power */
		sign = 1;
		for (i=0;i<ARRAY_SIZE(dtc_table_down);i++)
		{
			if ((dtc_table_down[i] >= pDM_Odm->RSSI_Min) || (dtc_steps >= 6))
				break;
			else
				dtc_steps++;
		}
	}
#if 0
	else if (DTC_DWN_BASE > pDM_Odm->RSSI_Min)
	{
		/* needs to increase the CTS TX power */
		sign = 0;
		dtc_steps = 1;
		for (i=0;i<ARRAY_SIZE(dtc_table_up);i++)
		{
			if ((dtc_table_up[i] <= pDM_Odm->RSSI_Min) || (dtc_steps>=10))
				break;
			else
				dtc_steps++;
		}
	}
#endif
	else
	{
		sign = 0;
		dtc_steps = 0;
	}

	resp_txagc = dtc_steps | (sign << 4);
	resp_txagc = resp_txagc | (resp_txagc << 5);
	ODM_Write1Byte(pDM_Odm, 0x06d9, resp_txagc);

	DBG_871X("%s RSSI_Min:%u, set RESP_TXAGC to %s %u\n", 
		__func__, pDM_Odm->RSSI_Min, sign?"minus":"plus", dtc_steps);
#endif /* CONFIG_RESP_TXAGC_ADJUST */
}

#endif /* #if (DM_ODM_SUPPORT_TYPE == ODM_CE) */

