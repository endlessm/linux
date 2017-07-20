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

#ifndef	__HALRF_FEATURES_H__
#define __HALRF_FEATURES

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)

	#define	CONFIG_HALRF_POWERTRACKING	1

#elif (DM_ODM_SUPPORT_TYPE == ODM_AP)

	#define	CONFIG_HALRF_POWERTRACKING	1

#elif (DM_ODM_SUPPORT_TYPE == ODM_CE)

	#define	CONFIG_HALRF_POWERTRACKING	1

#endif

#endif
