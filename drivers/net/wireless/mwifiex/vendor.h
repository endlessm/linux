/* Marvell Wireless LAN device driver: TDLS handling
 *
 * Copyright (C) 2014, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available on the worldwide web at
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 */

#ifndef __MARVELL_VENDOR_H__
#define __MARVELL_VENDOR_H__

#define MARVELL_OUI	0x005043

enum marvell_vendor_commands {
	MARVELL_VENDOR_CMD_SET_TURBO_MODE,
	MARVELL_VENDOR_CMD_SET_CONF_DATA,
};

#endif /* __MARVELL_VENDOR_H__ */
