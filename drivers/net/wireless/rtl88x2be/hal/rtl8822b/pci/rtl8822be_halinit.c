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

#define _RTL8822BE_HALINIT_C_
#include <drv_types.h>          /* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* HAL_DATA_TYPE */
#include "../rtl8822b.h"
#include "rtl8822be.h"

u32 InitMAC_TRXBD_8822BE(PADAPTER Adapter)
{
	u8 tmpU1b;
	u16 tmpU2b;
	u32 tmpU4b;
	int q_idx;
	struct recv_priv *precvpriv = &Adapter->recvpriv;
	struct xmit_priv *pxmitpriv = &Adapter->xmitpriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	RTW_INFO("=======>InitMAC_TXBD_8822BE()\n");

	/*
	 * Set CMD TX BD (buffer descriptor) physical address(from OS API).
	 */
	rtw_write32(Adapter, REG_H2CQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[TXCMD_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_H2CQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE_CMD | ((RTL8822BE_SEG_NUM << 12) &
					 0x3000));

#ifdef CONFIG_64BIT_DMA
	rtw_write32(Adapter, REG_H2CQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[TXCMD_QUEUE_INX].dma) >> 32);
#endif
	/*
	 * Set TX/RX BD (buffer descriptor) physical address(from OS API).
	 */
	rtw_write32(Adapter, REG_BCNQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_MGQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_VOQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_VIQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_BEQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));

	/* vincent sync windows */
	tmpU4b = rtw_read32(Adapter, REG_BEQ_TXBD_DESA_8822B);

	rtw_write32(Adapter, REG_BKQ_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_HI0Q_TXBD_DESA_8822B,
		    (u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_RXQ_RXBD_DESA_8822B,
		    (u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma &
		    DMA_BIT_MASK(32));

#ifdef CONFIG_64BIT_DMA
	/*
	 * 2009/10/28 MH For DMA 64 bits. We need to assign the high
	 * 32 bit address for NIC HW to transmit data to correct path.
	 */
	rtw_write32(Adapter, REG_BCNQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_MGQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_VOQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_VIQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_BEQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_BKQ_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_HI0Q_TXBD_DESA_8822B + 4,
		    ((u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_RXQ_RXBD_DESA_8822B + 4,
		    ((u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma) >> 32);


	/* 2009/10/28 MH If RX descriptor address is not equal to zero.
	* We will enable DMA 64 bit functuion.
	* Note: We never saw thd consition which the descripto address are
	*	divided into 4G down and 4G upper separate area.
	*/
	if (((u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma) >> 32 != 0) {
		RTW_INFO("Enable DMA64 bit\n");

		/* Check if other descriptor address is zero and
		 * abnormally be in 4G lower area. */
		if (((u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma) >> 32)
			RTW_INFO("MGNT_QUEUE HA=0\n");

		PlatformEnableDMA64(Adapter);
	} else
		RTW_INFO("Enable DMA32 bit\n");
#endif

	/* pci buffer descriptor mode: Reset the Read/Write point to 0 */
	PlatformEFIOWrite4Byte(Adapter, REG_TSFTIMER_HCI_8822B, 0x3fffffff);

	/* Reset the H2CQ R/W point index to 0 */
	tmpU4b = rtw_read32(Adapter, REG_H2CQ_CSR_8822B);
	rtw_write32(Adapter, REG_H2CQ_CSR_8822B, (tmpU4b | BIT8 | BIT16));

	tmpU1b = rtw_read8(Adapter, REG_PCIE_CTRL + 3);
	rtw_write8(Adapter, REG_PCIE_CTRL + 3, (tmpU1b | 0xF7));

	/* 20100318 Joseph: Reset interrupt migration setting
	 * when initialization. Suggested by SD1. */
	rtw_write32(Adapter, REG_INT_MIG, 0);
	pHalData->bInterruptMigration = _FALSE;

	/* 2009.10.19. Reset H2C protection register. by tynli. */
	rtw_write32(Adapter, REG_MCUTST_I_8822B, 0x0);

#if MP_DRIVER == 1
	if (Adapter->registrypriv.mp_mode == 1) {
		rtw_write32(Adapter, REG_MACID, 0x87654321);
		rtw_write32(Adapter, 0x0700, 0x87654321);
	}
#endif

	/* pic buffer descriptor mode: */
	/* ---- tx */
	rtw_write16(Adapter, REG_MGQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_VOQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_VIQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_BEQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_BKQ_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI0Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI1Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI2Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI3Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI4Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI5Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI6Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI7Q_TXBD_NUM_8822B,
		    TX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 12) & 0x3000));


	/* rx. support 32 bits in linux */


	/* using 64bit
	rtw_write16(Adapter, REG_RX_RXBD_NUM_8822B,
		RX_BD_NUM_8822BE |((RTL8822BE_SEG_NUM<<13 ) & 0x6000) |0x8000);
	*/


	/* using 32bit */
	rtw_write16(Adapter, REG_RX_RXBD_NUM_8822B,
		    RX_BD_NUM_8822BE | ((RTL8822BE_SEG_NUM << 13) & 0x6000));

	/* reset read/write point */
	rtw_write32(Adapter, REG_TSFTIMER_HCI_8822B, 0XFFFFFFFF);

#if 1 /* vincent windows */
	/* Start debug mode */
	{
		u8 reg0x3f3 = 0;

		reg0x3f3 = rtw_read8(Adapter, 0x3f3);
		rtw_write8(Adapter, 0x3f3, reg0x3f3 | BIT2);
	}

	{
		/* Need to disable BT coex to let MP tool Tx, this would be done in FW
		 * in the future, suggest by ChunChu, 2015.05.19
		 */

		u8 tmp1Byte;
		u16 tmp2Byte;
		u32 tmp4Byte;

		tmp2Byte = rtw_read16(Adapter, REG_SYS_FUNC_EN_8822B);
		rtw_write16(Adapter, REG_SYS_FUNC_EN_8822B, tmp2Byte | BIT10);
		tmp1Byte = rtw_read8(Adapter, REG_DIS_TXREQ_CLR_8822B);
		rtw_write8(Adapter, REG_DIS_TXREQ_CLR_8822B, tmp1Byte | BIT7);
		tmp4Byte = rtw_read32(Adapter, 0x1080);
		rtw_write32(Adapter, 0x1080, tmp4Byte | BIT16);
	}
#endif

	RTW_INFO("InitMAC_TXBD_8822BE() <====\n");

	return _SUCCESS;
}

u32 rtl8822be_init(PADAPTER padapter)
{
	u8 ok = _TRUE;
	u8 val8;
	PHAL_DATA_TYPE hal;
	struct registry_priv  *registry_par = &padapter->registrypriv;

	hal = GET_HAL_DATA(padapter);

	InitMAC_TRXBD_8822BE(padapter);

	ok = rtl8822b_hal_init(padapter);
	if (_FALSE == ok)
		return _FAIL;

#if defined(USING_RX_TAG)
	/* have to init after halmac init */
	val8 = rtw_read8(padapter, REG_PCIE_CTRL_8822B + 2);
	rtw_write8(padapter, REG_PCIE_CTRL_8822B + 2, (val8 | BIT4));
	rtw_write16(padapter, REG_PCIE_CTRL_8822B, 0x8000);
#else
	rtw_write16(padapter, REG_PCIE_CTRL_8822B, 0x0000);
#endif

	rtw_write8(padapter, REG_RX_DRVINFO_SZ_8822B, 0x4);

	rtl8822b_phy_init_haldm(padapter);
#ifdef CONFIG_BEAMFORMING
	rtl8822b_phy_bf_init(padapter);
#endif

#ifdef CONFIG_BT_COEXIST
	/* Init BT hw config. */
	if (hal->EEPROMBluetoothCoexist == _TRUE)
		rtw_btcoex_HAL_Initialize(padapter, _FALSE);
	else
		rtw_btcoex_wifionly_hw_config(padapter);
#else /* CONFIG_BT_COEXIST */
	rtw_btcoex_wifionly_hw_config(padapter);
#endif /* CONFIG_BT_COEXIST */

	hal->pci_backdoor_ctrl = registry_par->pci_aspm_config;

	rtw_pci_aspm_config(padapter);

	rtl8822b_init_misc(padapter);

	return _SUCCESS;
}

void rtl8822be_init_default_value(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData;


	pHalData = GET_HAL_DATA(padapter);

	rtl8822b_init_default_value(padapter);

	/* interface related variable */
	pHalData->CurrentWirelessMode = WIRELESS_MODE_AUTO;
	pHalData->bDefaultAntenna = 1;
	pHalData->TransmitConfig = BIT_CFEND_FORMAT | BIT_WMAC_TCR_ERRSTEN_3;

	/* Set RCR-Receive Control Register .
	 * The value is set in InitializeAdapter8190Pci().
	 */
	pHalData->ReceiveConfig = (
#ifdef CONFIG_RX_PACKET_APPEND_FCS
					  BIT_APP_FCS		|
#endif
					  BIT_APP_MIC		|
					  BIT_APP_ICV		|
					  BIT_APP_PHYSTS		|
					  BIT_VHT_DACK		|
					  BIT_HTC_LOC_CTRL	|
					  /* BIT_AMF		| */
					  BIT_CBSSID_DATA		|
					  BIT_CBSSID_BCN		|
					  /* BIT_ACF		| */
					  /* BIT_ADF		| */ /* PS-Poll filter */
					  BIT_AB			|
					  BIT_AB			|
					  BIT_APM			|
					  0);

	/*
	 * Set default value of Interrupt Mask Register0
	 */
	pHalData->IntrMaskDefault[0] = (u32)(
					       BIT(29)			| /* BIT_PSTIMEOUT */
					       BIT(27)			| /* BIT_GTINT3 */
					       BIT_TXBCN0ERR_MSK	|
					       BIT_TXBCN0OK_MSK	|
					       BIT_BCNDMAINT0_MSK	|
					       BIT_HSISR_IND_ON_INT_MSK |
					       BIT_C2HCMD_MSK		|
					       BIT_HIGHDOK_MSK		|
					       BIT_MGTDOK_MSK		|
					       BIT_BKDOK_MSK		|
					       BIT_BEDOK_MSK		|
					       BIT_VIDOK_MSK		|
					       BIT_VODOK_MSK		|
					       BIT_RDU_MSK		|
					       BIT_RXOK_MSK		|
					       0);

	/*
	 * Set default value of Interrupt Mask Register1
	 */
	pHalData->IntrMaskDefault[1] = (u32)(
					       BIT(9)		| /* TXFOVW */
					       BIT_FOVW_MSK	|
					       0);

	/*
	 * Set default value of Interrupt Mask Register3
	 */
	pHalData->IntrMaskDefault[3] = (u32)(
				       BIT_SETH2CDOK_MASK	| /* H2C_TX_OK */
					       0);

	/* 2012/03/27 hpfan Add for win8 DTM DPC ISR test */
	pHalData->IntrMaskReg[0] = (u32)(
					   BIT_RDU_MSK	|
					   BIT(29)		| /* BIT_PSTIMEOUT */
					   0);

	pHalData->IntrMaskReg[1] = (u32)(
					   BIT_C2HCMD_MSK	|
					   0);

	pHalData->IntrMask[0] = pHalData->IntrMaskDefault[0];
	pHalData->IntrMask[1] = pHalData->IntrMaskDefault[1];
	pHalData->IntrMask[3] = pHalData->IntrMaskDefault[3];

}
