#ifndef _HALMAC_API_8821C_SDIO_H_
#define _HALMAC_API_8821C_SDIO_H_

#include "../../halmac_2_platform.h"
#include "../../halmac_type.h"

HALMAC_RET_STATUS
halmac_mac_power_switch_8821c_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_MAC_POWER halmac_power
);

#if 0
HALMAC_RET_STATUS
halmac_tx_allowed_sdio_8821c(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pHalmac_buf,
	IN u32 halmac_size
);
#endif

HALMAC_RET_STATUS
halmac_phy_cfg_8821c_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_INTF_PHY_PLATFORM platform
);

HALMAC_RET_STATUS
halmac_interface_integration_tuning_8821c_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

#endif/* _HALMAC_API_8821C_SDIO_H_ */
