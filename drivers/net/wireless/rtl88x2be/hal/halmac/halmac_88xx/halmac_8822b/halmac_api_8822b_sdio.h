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

#ifndef _HALMAC_API_8822B_SDIO_H_
#define _HALMAC_API_8822B_SDIO_H_

#include "../../halmac_2_platform.h"
#include "../../halmac_type.h"

HALMAC_RET_STATUS
halmac_mac_power_switch_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_MAC_POWER halmac_power
);

#if 0
HALMAC_RET_STATUS
halmac_tx_allowed_sdio_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
);
#endif

HALMAC_RET_STATUS
halmac_phy_cfg_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_INTF_PHY_PLATFORM platform
);

HALMAC_RET_STATUS
halmac_interface_integration_tuning_8822b_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

#endif/* _HALMAC_API_8822B_SDIO_H_ */
