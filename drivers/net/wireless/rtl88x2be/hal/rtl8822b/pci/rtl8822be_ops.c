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

#define _HCI_OPS_OS_C_

#include <drv_types.h>		/* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* HAL_DATA_TYPE, GET_HAL_DATA() and etc. */
#include <hal_intf.h>		/* struct hal_ops */
#include "../rtl8822b.h"
#include "rtl8822be.h"

static void init_bd_ring_var(_adapter *padapter)
{
	struct recv_priv *r_priv = &padapter->recvpriv;
	struct xmit_priv *t_priv = &padapter->xmitpriv;
	u8 i = 0;

	for (i = 0; i < HW_QUEUE_ENTRY; i++)
		t_priv->txringcount[i] = TX_BD_NUM_8822BE;

	/*
	 * we just alloc 2 desc for beacon queue,
	 * because we just need first desc in hw beacon.
	 */
	t_priv->txringcount[BCN_QUEUE_INX] = TX_BD_NUM_8822BE_BCN;
	t_priv->txringcount[TXCMD_QUEUE_INX] = TX_BD_NUM_8822BE_CMD;

	/*
	 * BE queue need more descriptor for performance consideration
	 * or, No more tx desc will happen, and may cause mac80211 mem leakage.
	 */
	r_priv->rxbuffersize = MAX_RECVBUF_SZ;
	r_priv->rxringcount = PCI_MAX_RX_COUNT;
}

static void rtl8822be_reset_bd(_adapter *padapter)
{
	_irqL	irqL;
	struct xmit_priv *t_priv = &padapter->xmitpriv;
	struct recv_priv *r_priv = &padapter->recvpriv;
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(padapter);
	struct xmit_buf	*pxmitbuf = NULL;
	u8 *tx_bd, *rx_bd;
	int i, rx_queue_idx;

	InitMAC_TRXBD_8822BE(padapter);

	for (rx_queue_idx = 0; rx_queue_idx < 1; rx_queue_idx++) {
		if (r_priv->rx_ring[rx_queue_idx].buf_desc) {
			rx_bd = NULL;
			for (i = 0; i < r_priv->rxringcount; i++) {
				rx_bd = (u8 *)
					&r_priv->rx_ring[rx_queue_idx].buf_desc[i];
			}
			r_priv->rx_ring[rx_queue_idx].idx = 0;
		}
	}

	_enter_critical(&pdvobjpriv->irq_th_lock, &irqL);
	for (i = 0; i < PCI_MAX_TX_QUEUE_COUNT; i++) {
		if (t_priv->tx_ring[i].buf_desc) {
			struct rtw_tx_ring *ring = &t_priv->tx_ring[i];

			while (ring->qlen) {
				tx_bd = (u8 *)(&ring->buf_desc[ring->idx]);
				SET_TX_BD_OWN(tx_bd, 0);

				if (i != BCN_QUEUE_INX)
					ring->idx =
						(ring->idx + 1) % ring->entries;

				pxmitbuf = rtl8822be_dequeue_xmitbuf(ring);
				if (pxmitbuf) {
					pci_unmap_single(pdvobjpriv->ppcidev,
						GET_TX_BD_PHYSICAL_ADDR0_LOW(tx_bd),
						pxmitbuf->len, PCI_DMA_TODEVICE);
					rtw_free_xmitbuf(t_priv, pxmitbuf);
				} else {
					RTW_INFO("%s(): qlen(%d) is not zero, but have xmitbuf in pending queue\n",
						 __func__, ring->qlen);
					break;
				}
			}
			ring->idx = 0;
		}
	}
	_exit_critical(&pdvobjpriv->irq_th_lock, &irqL);
}

static void intf_chip_configure(PADAPTER padapter)
{

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(padapter);
	struct pwrctrl_priv *pwrpriv = dvobj_to_pwrctl(pdvobjpriv);

	/* close ASPM for AMD defaultly */
	pdvobjpriv->const_amdpci_aspm = 0;

	/* ASPM PS mode. */
	/* 0 - Disable ASPM, 1 - Enable ASPM without Clock Req, */
	/* 2 - Enable ASPM with Clock Req, 3- Alwyas Enable ASPM with Clock Req, */
	/* 4-  Always Enable ASPM without Clock Req. */
	/* set default to rtl8188ee:3 RTL8192E:2 */
	pdvobjpriv->const_pci_aspm = 0;

	/* Setting for PCI-E device */
	pdvobjpriv->const_devicepci_aspm_setting = 0x03;

	/* Setting for PCI-E bridge */
	pdvobjpriv->const_hostpci_aspm_setting = 0x03;

	/* In Hw/Sw Radio Off situation. */
	/* 0 - Default, 1 - From ASPM setting without low Mac Pwr, */
	/* 2 - From ASPM setting with low Mac Pwr, 3 - Bus D3 */
	/* set default to RTL8192CE:0 RTL8192SE:2 */
	pdvobjpriv->const_hwsw_rfoff_d3 = 0;

	/* This setting works for those device with backdoor ASPM setting such as EPHY setting. */
	/* 0: Not support ASPM, 1: Support ASPM, 2: According to chipset. */
	pdvobjpriv->const_support_pciaspm = 1;

	pwrpriv->reg_rfoff = 0;
	pwrpriv->rfoff_reason = 0;

	pHalData->bL1OffSupport = _FALSE;
}

static BOOLEAN rtl8822be_InterruptRecognized(PADAPTER Adapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN bRecognized = _FALSE;

	/* 2013.11.18 Glayrainx suggests that turn off IMR and
	 * restore after cleaning ISR.
	 */
	rtw_write32(Adapter, REG_HIMR0, 0);
	rtw_write32(Adapter, REG_HIMR1, 0);
	rtw_write32(Adapter, REG_HIMR3, 0);

	pHalData->IntArray[0] = rtw_read32(Adapter, REG_HISR0);
	pHalData->IntArray[0] &= pHalData->IntrMask[0];
	rtw_write32(Adapter, REG_HISR0, pHalData->IntArray[0]);

	/* For HISR extension. Added by tynli. 2009.10.07. */
	pHalData->IntArray[1] = rtw_read32(Adapter, REG_HISR1);
	pHalData->IntArray[1] &= pHalData->IntrMask[1];
	rtw_write32(Adapter, REG_HISR1, pHalData->IntArray[1]);

	/* for H2C cmd queue */
	pHalData->IntArray[3] = rtw_read32(Adapter, REG_HISR3);
	pHalData->IntArray[3] &= pHalData->IntrMask[3];
	rtw_write32(Adapter, REG_HISR3, pHalData->IntArray[3]);

	if (((pHalData->IntArray[0]) & pHalData->IntrMask[0]) != 0 ||
	    ((pHalData->IntArray[1]) & pHalData->IntrMask[1]) != 0)
		bRecognized = _TRUE;

	/* restore IMR */
	rtw_write32(Adapter, REG_HIMR0, pHalData->IntrMask[0] & 0xFFFFFFFF);
	rtw_write32(Adapter, REG_HIMR1, pHalData->IntrMask[1] & 0xFFFFFFFF);
	rtw_write32(Adapter, REG_HIMR3, pHalData->IntrMask[3] & 0xFFFFFFFF);

	return bRecognized;
}

static VOID DisableInterrupt8822be(PADAPTER Adapter)
{
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(Adapter);

	rtw_write32(Adapter, REG_HIMR0, 0x0);
	rtw_write32(Adapter, REG_HIMR1, 0x0);
	rtw_write32(Adapter, REG_HIMR3, 0x0);
	pdvobjpriv->irq_enabled = 0;
}

static VOID rtl8822be_enable_interrupt(PADAPTER Adapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(Adapter);

	pdvobjpriv->irq_enabled = 1;

	rtw_write32(Adapter, REG_HIMR0, pHalData->IntrMask[0] & 0xFFFFFFFF);
	rtw_write32(Adapter, REG_HIMR1, pHalData->IntrMask[1] & 0xFFFFFFFF);
	rtw_write32(Adapter, REG_HIMR3, pHalData->IntrMask[3] & 0xFFFFFFFF);

}

static VOID rtl8822be_clear_interrupt(PADAPTER Adapter)
{
	u32 u32b;
	HAL_DATA_TYPE   *pHalData = GET_HAL_DATA(Adapter);

	u32b = rtw_read32(Adapter, REG_HISR0_8822B);
	rtw_write32(Adapter, REG_HISR0_8822B, u32b);
	pHalData->IntArray[0] = 0;

	u32b = rtw_read32(Adapter, REG_HISR1_8822B);
	rtw_write32(Adapter, REG_HISR1_8822B, u32b);
	pHalData->IntArray[1] = 0;
}

static VOID rtl8822be_disable_interrupt(PADAPTER Adapter)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(Adapter);

	rtw_write32(Adapter, REG_HIMR0, 0x0);
	rtw_write32(Adapter, REG_HIMR1, 0x0);	/* by tynli */
	pdvobjpriv->irq_enabled = 0;
}

VOID UpdateInterruptMask8822BE(PADAPTER Adapter, u32 AddMSR, u32 AddMSR1,
			       u32 RemoveMSR, u32 RemoveMSR1)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);

	DisableInterrupt8822be(Adapter);

	if (AddMSR)
		pHalData->IntrMask[0] |= AddMSR;

	if (AddMSR1)
		pHalData->IntrMask[1] |= AddMSR1;

	if (RemoveMSR)
		pHalData->IntrMask[0] &= (~RemoveMSR);

	if (RemoveMSR1)
		pHalData->IntrMask[1] &= (~RemoveMSR1);

#if 0 /* TODO */
	if (RemoveMSR3)
		pHalData->IntrMask[3] &= (~RemoveMSR3);
#endif

	rtl8822be_enable_interrupt(Adapter);
}

static void rtl8822be_bcn_handler(PADAPTER Adapter, u32 handled[])
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);

	if (pHalData->IntArray[0] & BIT_TXBCN0OK_MSK) {
#ifdef CONFIG_BCN_ICF
		/* do nothing */
#else
		/* Modify for MI temporary,
		 * this processor cannot apply to multi-ap */
		PADAPTER bcn_adapter = rtw_mi_get_ap_adapter(Adapter);

		if (bcn_adapter->xmitpriv.beaconDMAing) {
			bcn_adapter->xmitpriv.beaconDMAing = _FAIL;
			rtl8822be_tx_isr(Adapter, BCN_QUEUE_INX);
		}
#endif /* CONFIG_BCN_ICF */
		handled[0] |= BIT_TXBCN0OK_MSK;
	}

	if (pHalData->IntArray[0] & BIT_TXBCN0ERR_MSK) {
#ifdef CONFIG_BCN_ICF
		RTW_INFO("IMR_TXBCN0ERR isr!\n");
#else /* !CONFIG_BCN_ICF */
		/* Modify for MI temporary,
		 * this processor cannot apply to multi-ap */
		PADAPTER bcn_adapter = rtw_mi_get_ap_adapter(Adapter);

		if (bcn_adapter->xmitpriv.beaconDMAing) {
			bcn_adapter->xmitpriv.beaconDMAing = _FAIL;
			rtl8822be_tx_isr(Adapter, BCN_QUEUE_INX);
		}
#endif /* CONFIG_BCN_ICF */
		handled[0] |= BIT_TXBCN0ERR_MSK;
	}

	if (pHalData->IntArray[0] & BIT_BCNDERR0_MSK) {
#ifdef CONFIG_BCN_ICF
		RTW_INFO("BIT_BCNDERR0_MSK isr!\n");
#else /* !CONFIG_BCN_ICF */
		/* Release resource and re-transmit beacon to HW */
		struct tasklet_struct  *bcn_tasklet;
		/* Modify for MI temporary,
		 * this processor cannot apply to multi-ap */
		PADAPTER bcn_adapter = rtw_mi_get_ap_adapter(Adapter);

		rtl8822be_tx_isr(Adapter, BCN_QUEUE_INX);
		bcn_adapter->mlmepriv.update_bcn = _TRUE;
		bcn_tasklet = &bcn_adapter->recvpriv.irq_prepare_beacon_tasklet;
		tasklet_hi_schedule(bcn_tasklet);
#endif /* CONFIG_BCN_ICF */
		handled[0] |= BIT_BCNDERR0_MSK;
	}

	if (pHalData->IntArray[0] & BIT_BCNDMAINT0_MSK) {
		struct tasklet_struct  *bcn_tasklet;
		/* Modify for MI temporary,
		  this processor cannot apply to multi-ap */
		PADAPTER bcn_adapter = rtw_mi_get_ap_adapter(Adapter);

		bcn_tasklet = &bcn_adapter->recvpriv.irq_prepare_beacon_tasklet;
		tasklet_hi_schedule(bcn_tasklet);
		handled[0] |= BIT_BCNDMAINT0_MSK;
	}
}

static void rtl8822be_rx_handler(PADAPTER Adapter, u32 handled[])
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);

	if ((pHalData->IntArray[0] & (BIT_RXOK | BIT_RDU)) ||
	    (pHalData->IntArray[1] & (BIT_FOVW | BIT_RXERR_INT))) {
		pHalData->IntrMask[0] &= (~(BIT_RXOK_MSK | BIT_RDU_MSK));
		pHalData->IntrMask[1] &= (~(BIT_FOVW_MSK | BIT_RXERR_MSK));
		rtw_write32(Adapter, REG_HIMR0, pHalData->IntrMask[0]);
		rtw_write32(Adapter, REG_HIMR1, pHalData->IntrMask[1]);
		tasklet_hi_schedule(&Adapter->recvpriv.recv_tasklet);
		handled[0] |= pHalData->IntArray[0] & (BIT_RXOK | BIT_RDU);
		handled[1] |= pHalData->IntArray[1] & (BIT_FOVW | BIT_RXERR_INT);
	}
}

static void rtl8822be_tx_handler(PADAPTER Adapter, u32 events[], u32 handled[])
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);

	if (events[0] & BIT_MGTDOK_MSK) {
		rtl8822be_tx_isr(Adapter, MGT_QUEUE_INX);
		handled[0] |= BIT_MGTDOK_MSK;
	}

	if (events[0] & BIT_HIGHDOK_MSK) {
		rtl8822be_tx_isr(Adapter, HIGH_QUEUE_INX);
		handled[0] |= BIT_HIGHDOK_MSK;
	}

	if (events[0] & BIT_BKDOK_MSK) {
		rtl8822be_tx_isr(Adapter, BK_QUEUE_INX);
		handled[0] |= BIT_BKDOK_MSK;
	}

	if (events[0] & BIT_BEDOK_MSK) {
		rtl8822be_tx_isr(Adapter, BE_QUEUE_INX);
		handled[0] |= BIT_BEDOK_MSK;
	}

	if (events[0] & BIT_VIDOK_MSK) {
		rtl8822be_tx_isr(Adapter, VI_QUEUE_INX);
		handled[0] |= BIT_VIDOK_MSK;
	}

	if (events[0] & BIT_VODOK_MSK) {
		rtl8822be_tx_isr(Adapter, VO_QUEUE_INX);
		handled[0] |= BIT_VODOK_MSK;
	}
}

static void rtl8822be_cmd_handler(PADAPTER Adapter, u32 handled[])
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);

	if (pHalData->IntArray[3] & BIT_SETH2CDOK_MASK) {
		rtl8822be_tx_isr(Adapter, TXCMD_QUEUE_INX);
		handled[3] |= BIT_SETH2CDOK_MASK;
	}
}

static s32 rtl8822be_interrupt(PADAPTER Adapter)
{
	_irqL irqL;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	struct dvobj_priv *pdvobjpriv = adapter_to_dvobj(Adapter);
	struct xmit_priv *t_priv = &Adapter->xmitpriv;
	int ret = _SUCCESS;
	u32 handled[4] = {0};

	_enter_critical(&pdvobjpriv->irq_th_lock, &irqL);

	/* read ISR: 4/8bytes */
	if (rtl8822be_InterruptRecognized(Adapter) == _FALSE) {
		ret = _FAIL;
		goto done;
	}

	/* <1> beacon related */
	rtl8822be_bcn_handler(Adapter, handled);

	/* <2> Rx related */
	rtl8822be_rx_handler(Adapter, handled);

	/* <3> Tx related */
	rtl8822be_tx_handler(Adapter, pHalData->IntArray, handled);

	if (pHalData->IntArray[1] & BIT_TXFOVW) {
		if (printk_ratelimit())
			RTW_WARN("[TXFOVW]\n");
		handled[1] |= BIT_TXFOVW;
	}

	/* <4> Cmd related */
	rtl8822be_cmd_handler(Adapter, handled);

	if ((pHalData->IntArray[0] & (~handled[0])) ||
		(pHalData->IntArray[1] & (~handled[1])) ||
		(pHalData->IntArray[3] & (~handled[3]))) {

		if (printk_ratelimit()) {
			RTW_WARN("Unhandled ISR = %x, %x, %x\n",
				(pHalData->IntArray[0] & (~handled[0])),
				(pHalData->IntArray[1] & (~handled[1])),
				(pHalData->IntArray[3] & (~handled[3])));
	}
	}
done:
	_exit_critical(&pdvobjpriv->irq_th_lock, &irqL);
	return ret;
}

u32 rtl8822be_init_bd(_adapter *padapter)
{
	struct xmit_priv *t_priv = &padapter->xmitpriv;
	int	i, ret = _SUCCESS;

	init_bd_ring_var(padapter);
	ret = rtl8822be_init_rxbd_ring(padapter);

	if (ret == _FAIL)
		return ret;

	/* general process for other queue */
	for (i = 0; i < PCI_MAX_TX_QUEUE_COUNT; i++) {
		ret = rtl8822be_init_txbd_ring(padapter, i,
					       t_priv->txringcount[i]);
		if (ret == _FAIL)
			goto err_free_rings;
	}

	return ret;

err_free_rings:

	rtl8822be_free_rxbd_ring(padapter);

	for (i = 0; i < PCI_MAX_TX_QUEUE_COUNT; i++)
		if (t_priv->tx_ring[i].buf_desc)
			rtl8822be_free_txbd_ring(padapter, i);


	return ret;
}

u32 rtl8822be_free_bd(_adapter *padapter)
{
	struct xmit_priv	*t_priv = &padapter->xmitpriv;
	u32 i;


	/* free rxbd rings */
	rtl8822be_free_rxbd_ring(padapter);

	/* free txbd rings */
	for (i = 0; i < HW_QUEUE_ENTRY; i++)
		rtl8822be_free_txbd_ring(padapter, i);


	return _SUCCESS;
}

static u16
hal_mdio_read_8822be(PADAPTER Adapter, u8 Addr)
{
	u2Byte ret = 0;
	u1Byte tmpU1b = 0, count = 0;

	rtw_write8(Adapter, REG_PCIE_MIX_CFG_8822B, Addr | BIT6);
	tmpU1b = rtw_read8(Adapter, REG_PCIE_MIX_CFG_8822B) & BIT6;
	count = 0;
	while (tmpU1b && count < 20) {
		rtw_udelay_os(10);
		tmpU1b = rtw_read8(Adapter, REG_PCIE_MIX_CFG_8822B) & BIT6;
		count++;
	}
	if (tmpU1b == 0)
		ret = rtw_read16(Adapter, REG_MDIO_V1_8822B);

	return ret;
}


static VOID
hal_mdio_write_8822be(PADAPTER Adapter, u8 Addr, u16 Data)
{
	u1Byte tmpU1b = 0, count = 0;

	rtw_write16(Adapter, REG_MDIO_V1_8822B, Data);
	rtw_write8(Adapter, REG_PCIE_MIX_CFG_8822B, Addr | BIT5);
	tmpU1b = rtw_read8(Adapter, REG_PCIE_MIX_CFG_8822B) & BIT5;
	count = 0;
	while (tmpU1b && count < 20) {
		rtw_udelay_os(10);
		tmpU1b = rtw_read8(Adapter, REG_PCIE_MIX_CFG_8822B) & BIT5;
		count++;
	}
}


static void hal_dbi_write_8822be(PADAPTER Adapter, u16 Addr, u8 Data)
{
	u1Byte tmpU1b = 0, count = 0;
	u2Byte WriteAddr = 0, Remainder = Addr % 4;


	/* Write DBI 1Byte Data */
	WriteAddr = REG_DBI_WDATA_V1_8822B + Remainder;
	rtw_write8(Adapter, WriteAddr, Data);

	/* Write DBI 2Byte Address & Write Enable */
	WriteAddr = (Addr & 0xfffc) | (BIT0 << (Remainder + 12));
	rtw_write16(Adapter, REG_DBI_FLAG_V1_8822B, WriteAddr);

	/* Write DBI Write Flag */
	rtw_write8(Adapter, REG_DBI_FLAG_V1_8822B + 2, 0x1);

	tmpU1b = rtw_read8(Adapter, REG_DBI_FLAG_V1_8822B + 2);
	count = 0;
	while (tmpU1b && count < 20) {
		rtw_udelay_os(10);
		tmpU1b = rtw_read8(Adapter, REG_DBI_FLAG_V1_8822B + 2);
		count++;
	}
}

static u8 hal_dbi_read_8822be(PADAPTER Adapter, u16 Addr)
{
	u16 ReadAddr = Addr & 0xfffc;
	u8 ret = 0, tmpU1b = 0, count = 0;

	rtw_write16(Adapter, REG_DBI_FLAG_V1_8822B, ReadAddr);
	rtw_write8(Adapter, REG_DBI_FLAG_V1_8822B + 2, 0x2);
	tmpU1b = rtw_read8(Adapter, REG_DBI_FLAG_V1_8822B + 2);
	count = 0;
	while (tmpU1b && count < 20) {
		rtw_udelay_os(10);
		tmpU1b = rtw_read8(Adapter, REG_DBI_FLAG_V1_8822B + 2);
		count++;
	}
	if (tmpU1b == 0) {
		ReadAddr = REG_DBI_RDATA_V1_8822B + Addr % 4;
		ret = rtw_read8(Adapter, ReadAddr);
	}

	return ret;
}

void rtl8822be_aspm_config_l1off(PADAPTER padapter, u8 enable)
{
	u8 tmp8;

	tmp8 = hal_dbi_read_8822be(padapter, 0x718);

	if (enable)
		hal_dbi_write_8822be(padapter, 0x718, (tmp8 | BIT5));
	else
		hal_dbi_write_8822be(padapter, 0x718, (tmp8 & (~BIT5)));
}

static void sethwreg(PADAPTER padapter, u8 variable, u8 *val)
{
	switch (variable) {
	case HW_VAR_DBI:
	{
		u16 *pCmd;

		pCmd = (u16 *)val;
		hal_dbi_write_8822be(padapter, pCmd[0], (u8)pCmd[1]);
		break;
	}
	case HW_VAR_MDIO:
	{
		u16 *pCmd;

		pCmd = (u16 *)val;
		hal_mdio_write_8822be(padapter, (u8)pCmd[0], pCmd[1]);
		break;
	}
	default:
		rtl8822b_sethwreg(padapter, variable, val);
		break;
	}
}

static void gethwreg(PADAPTER padapter, u8 variable, u8 *val)
{
	switch (variable) {
	case HW_VAR_DBI:
		*val = hal_dbi_read_8822be(padapter, *((u16 *)(val)));
		break;

	case HW_VAR_MDIO:
		*((u16 *)(val)) = hal_mdio_read_8822be(padapter, *val);
		break;

	case HW_VAR_L1OFF_NIC_SUPPORT:
		{
		u8 l1off;

		l1off = hal_dbi_read_8822be(padapter, 0x168);
		if (l1off & (BIT2|BIT3))
			*val = _TRUE;
		else
			*val = _FALSE;
		}
		break;

	case HW_VAR_L1OFF_CAPABILITY:
		{
		u8 l1off;

		l1off = hal_dbi_read_8822be(padapter, 0x164);
		if (l1off & (BIT2|BIT3))
			*val = _TRUE;
		else
			*val = _FALSE;
		}
		break;
	default:
		rtl8822b_gethwreg(padapter, variable, val);
		break;
	}

}

/*
	Description:
		Query setting of specified variable.
*/
static u8 gethaldefvar(PADAPTER	padapter, HAL_DEF_VARIABLE eVariable, PVOID pValue)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	u8 bResult = _SUCCESS;

	switch (eVariable) {

	case HAL_DEF_MAX_RECVBUF_SZ:
		*((u32 *)pValue) = MAX_RECVBUF_SZ;
		break;

	case HW_VAR_MAX_RX_AMPDU_FACTOR:
		*(HT_CAP_AMPDU_FACTOR *)pValue = MAX_AMPDU_FACTOR_64K;
		break;
	default:
		bResult = rtl8822b_gethaldefvar(padapter, eVariable, pValue);
		break;
	}

	return bResult;
}

void rtl8822be_set_hal_ops(PADAPTER padapter)
{
	struct hal_ops *ops;
	int err;

	err = rtl8822be_halmac_init_adapter(padapter);
	if (err) {
		RTW_INFO("%s: [ERROR]HALMAC initialize FAIL!\n", __func__);
		return;
	}

	rtl8822b_set_hal_ops(padapter);

	ops = &padapter->hal_func;

	ops->hal_init = rtl8822be_init;
	ops->inirp_init = rtl8822be_init_bd;
	ops->inirp_deinit = rtl8822be_free_bd;
	ops->irp_reset = rtl8822be_reset_bd;
	ops->init_xmit_priv = rtl8822be_init_xmit_priv;
	ops->free_xmit_priv = rtl8822be_free_xmit_priv;
	ops->init_recv_priv = rtl8822be_init_recv_priv;
	ops->free_recv_priv = rtl8822be_free_recv_priv;

#ifdef CONFIG_SW_LED
	ops->InitSwLeds = rtl8822be_InitSwLeds;
	ops->DeInitSwLeds = rtl8822be_DeInitSwLeds;
#else /* case of hw led or no led */
	ops->InitSwLeds = NULL;
	ops->DeInitSwLeds = NULL;
#endif

	ops->init_default_value = rtl8822be_init_default_value;
	ops->intf_chip_configure = intf_chip_configure;

	ops->enable_interrupt = rtl8822be_enable_interrupt;
	ops->disable_interrupt = rtl8822be_disable_interrupt;
	ops->interrupt_handler = rtl8822be_interrupt;
	/*
		ops->check_ips_status = check_ips_status;
	*/
	ops->clear_interrupt = rtl8822be_clear_interrupt;
#if defined(CONFIG_WOWLAN) || defined(CONFIG_AP_WOWLAN) ||\
	defined(CONFIG_PCI_HCI)
	/*
		ops->clear_interrupt = clear_interrupt_all;
	*/
#endif
	ops->get_hal_def_var_handler = gethaldefvar;

	ops->set_hw_reg_handler = sethwreg;
	ops->GetHwRegHandler = gethwreg;

	ops->hal_xmit = rtl8822be_hal_xmit;
	ops->mgnt_xmit = rtl8822be_mgnt_xmit;
	ops->hal_xmitframe_enqueue = rtl8822be_hal_xmitframe_enqueue;
#ifdef CONFIG_HOSTAPD_MLME
	ops->hostap_mgnt_xmit_entry = rtl8822be_hostap_mgnt_xmit_entry;
#endif

#ifdef CONFIG_XMIT_THREAD_MODE
	/* vincent TODO */
	ops->xmit_thread_handler = rtl8822be_xmit_buf_handler;
#endif

	ops->hal_set_l1ssbackdoor_handler = rtl8822be_aspm_config_l1off;

}
