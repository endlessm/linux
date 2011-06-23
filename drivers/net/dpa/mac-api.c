/* Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *	 notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *	 names of its contributors may be used to endorse or promote products
 *	 derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/netdevice.h>

#include "dpaa_eth-common.h"
#include "dpaa_eth.h"
#include "mac.h"

#include "error_ext.h"	/* GET_ERROR_TYPE, E_OK */
#include "fm_mac_ext.h"
#include "fm_rtc_ext.h"

#define MAC_DESCRIPTION "FSL FMan MAC API based driver"

MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Emil Medve <Emilian.Medve@Freescale.com>");

MODULE_DESCRIPTION(MAC_DESCRIPTION);

struct mac_priv_s {
	t_Handle	mac;
};

const char	*mac_driver_description __initconst = MAC_DESCRIPTION;
const size_t	 mac_sizeof_priv[] = {
	[DTSEC] = sizeof(struct mac_priv_s),
	[XGMAC] = sizeof(struct mac_priv_s)
};

static const e_EnetMode _100[] =
{
	[PHY_INTERFACE_MODE_MII]	= e_ENET_MODE_MII_100,
	[PHY_INTERFACE_MODE_RMII]	= e_ENET_MODE_RMII_100
};

static const e_EnetMode _1000[] =
{
	[PHY_INTERFACE_MODE_GMII]	= e_ENET_MODE_GMII_1000,
	[PHY_INTERFACE_MODE_SGMII]	= e_ENET_MODE_SGMII_1000,
	[PHY_INTERFACE_MODE_TBI]	= e_ENET_MODE_TBI_1000,
	[PHY_INTERFACE_MODE_RGMII]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_ID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_RXID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RGMII_TXID]	= e_ENET_MODE_RGMII_1000,
	[PHY_INTERFACE_MODE_RTBI]	= e_ENET_MODE_RTBI_1000
};

static e_EnetMode __cold __attribute__((nonnull))
macdev2enetinterface(const struct mac_device *mac_dev)
{
	switch (mac_dev->max_speed) {
	case SPEED_100:
		return _100[mac_dev->phy_if];
	case SPEED_1000:
		return _1000[mac_dev->phy_if];
	case SPEED_10000:
		return e_ENET_MODE_XGMII_10000;
	default:
		return e_ENET_MODE_MII_100;
	}
}

static void mac_exception(t_Handle _mac_dev, e_FmMacExceptions exception)
{
	struct mac_device	*mac_dev;

	mac_dev = (struct mac_device *)_mac_dev;

	if (e_FM_MAC_EX_10G_RX_FIFO_OVFL == exception) {
		/* don't flag RX FIFO after the first */
		FM_MAC_SetException(
		    ((struct mac_priv_s *)macdev_priv(_mac_dev))->mac,
		    e_FM_MAC_EX_10G_RX_FIFO_OVFL, false);
		printk(KERN_ERR "10G MAC got RX FIFO Error = %x\n", exception);
	}

	cpu_dev_dbg(mac_dev->dev, "%s:%s() -> %d\n", __file__, __func__,
		exception);
}

static int __cold init(struct mac_device *mac_dev)
{
	int					_errno;
	t_Error				err;
	struct mac_priv_s	*priv;
	t_FmMacParams		param;
	uint32_t			version;

	priv = macdev_priv(mac_dev);

	param.baseAddr =  (typeof(param.baseAddr))(uintptr_t)devm_ioremap(
		mac_dev->dev, mac_dev->res->start, 0x2000);
	param.enetMode	= macdev2enetinterface(mac_dev);
	memcpy(&param.addr, mac_dev->addr, min(sizeof(param.addr),
		sizeof(mac_dev->addr)));
	param.macId			= mac_dev->cell_index;
	param.h_Fm 			= (t_Handle)mac_dev->fm;
	param.mdioIrq		= NO_IRQ;
	param.f_Exception	= mac_exception;
	param.f_Event		= mac_exception;
	param.h_App			= mac_dev;

	priv->mac = FM_MAC_Config(&param);
	if (unlikely(priv->mac == NULL)) {
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Config() failed\n");
		_errno = -EINVAL;
		goto _return;
	}

	err = FM_MAC_ConfigMaxFrameLength(priv->mac, fsl_fman_phy_maxfrm);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		dpaa_eth_err(mac_dev->dev,
			"FM_MAC_ConfigMaxFrameLength() = 0x%08x\n", err);
		goto _return_fm_mac_free;
	}

	if (macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000) {
		/* 10G always works with pad and CRC */
		err = FM_MAC_ConfigPadAndCrc(priv->mac, true);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_ConfigPadAndCrc() = 0x%08x\n", err);
			goto _return_fm_mac_free;
		}

		err = FM_MAC_ConfigHalfDuplex(priv->mac, mac_dev->half_duplex);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_ConfigHalfDuplex() = 0x%08x\n", err);
			goto _return_fm_mac_free;
		}
	}
	else  {
		err = FM_MAC_ConfigResetOnInit(priv->mac, true);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_ConfigResetOnInit() = 0x%08x\n", err);
			goto _return_fm_mac_free;
		}
	}

	err = FM_MAC_Init(priv->mac);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Init() = 0x%08x\n", err);
		goto _return_fm_mac_free;
	}

#ifndef CONFIG_FMAN_MIB_CNT_OVF_IRQ_EN
	/* For 1G MAC, disable by default the MIB counters overflow interrupt */
	if (macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000) {
		err = FM_MAC_SetException(priv->mac,
				e_FM_MAC_EX_1G_RX_MIB_CNT_OVFL, FALSE);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_SetException() = 0x%08x\n", err);
			goto _return_fm_mac_free;
		}
	}
#endif /* !CONFIG_FMAN_MIB_CNT_OVF_IRQ_EN */

	/* For 10G MAC, disable Tx ECC exception */
	if (macdev2enetinterface(mac_dev) == e_ENET_MODE_XGMII_10000) {
		err = FM_MAC_SetException(priv->mac,
					  e_FM_MAC_EX_10G_1TX_ECC_ER, FALSE);
		_errno = -GET_ERROR_TYPE(err);
		if (unlikely(_errno < 0)) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_SetException() = 0x%08x\n", err);
			goto _return_fm_mac_free;
		}
	}

	err = FM_MAC_GetVesrion(priv->mac, &version);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0)) {
		dpaa_eth_err(mac_dev->dev, "FM_MAC_GetVesrion() = 0x%08x\n",
				err);
		goto _return_fm_mac_free;
	}
	cpu_dev_info(mac_dev->dev, "FMan %s version: 0x%08x\n",
		((macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000) ?
			"dTSEC" : "XGEC"), version);

	goto _return;


_return_fm_mac_free:
	err = FM_MAC_Free(priv->mac);
	if (unlikely(-GET_ERROR_TYPE(err) < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Free() = 0x%08x\n", err);
_return:
	return _errno;
}

static int __cold start(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;
	struct phy_device *phy_dev = mac_dev->phy_dev;

	err = FM_MAC_Enable(((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Enable() = 0x%08x\n", err);

	if (phy_dev) {
		if (macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000)
			phy_start(phy_dev);
		else if (phy_dev->drv->read_status)
			phy_dev->drv->read_status(phy_dev);
	}

	return _errno;
}

static int __cold stop(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;

	if (mac_dev->phy_dev &&
		(macdev2enetinterface(mac_dev) != e_ENET_MODE_XGMII_10000))
		phy_stop(mac_dev->phy_dev);

	err = FM_MAC_Disable(((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
				e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Disable() = 0x%08x\n", err);

	return _errno;
}

static int __cold change_promisc(struct mac_device *mac_dev)
{
	int	 _errno;
	t_Error	 err;

	err = FM_MAC_SetPromiscuous(
			((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			mac_dev->promisc = !mac_dev->promisc);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev,
				"FM_MAC_SetPromiscuous() = 0x%08x\n", err);

	return _errno;
}

static int __cold set_multi(struct net_device *net_dev)
{
	struct dpa_priv_s       *priv;
	struct mac_device       *mac_dev;
	struct mac_priv_s 	*mac_priv;
	struct mac_address	*old_addr, *tmp;
	struct netdev_hw_addr	*ha;
	int 			 _errno;
	t_Error 		 err;

	priv = netdev_priv(net_dev);
	mac_dev = priv->mac_dev;
	mac_priv = macdev_priv(mac_dev);

	/* Clear previous address list */
	list_for_each_entry_safe(old_addr, tmp, &mac_dev->mc_addr_list, list) {
		err = FM_MAC_RemoveHashMacAddr(mac_priv->mac,
					       (t_EnetAddr  *)old_addr->addr);
		_errno = -GET_ERROR_TYPE(err);
		if (_errno < 0) {
			dpaa_eth_err(mac_dev->dev,
				"FM_MAC_RemoveHashMacAddr() = 0x%08x\n", err);
			return _errno;
		}
		list_del(&old_addr->list);
		kfree(old_addr);
	}

	/* Add all the addresses from the new list */
	netdev_for_each_mc_addr(ha, net_dev) {
		err = FM_MAC_AddHashMacAddr(mac_priv->mac,
				(t_EnetAddr *)ha->addr);
		_errno = -GET_ERROR_TYPE(err);
		if (_errno < 0) {
			dpaa_eth_err(mac_dev->dev,
				     "FM_MAC_AddHashMacAddr() = 0x%08x\n", err);
			return _errno;
		}
		tmp = kmalloc(sizeof(struct mac_address), GFP_ATOMIC);
		if (!tmp) {
			dpaa_eth_err(mac_dev->dev, "Out of memory\n");
			return -ENOMEM;
		}
		memcpy(tmp->addr, ha->addr, ETH_ALEN);
		list_add(&tmp->list, &mac_dev->mc_addr_list);
	}
	return 0;
}

static int __cold change_addr(struct mac_device *mac_dev, uint8_t *addr)
{
	int	_errno;
	t_Error err;

	err = FM_MAC_ModifyMacAddr(
			((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			(t_EnetAddr *)addr);
	_errno = -GET_ERROR_TYPE(err);
	if (_errno < 0)
		dpaa_eth_err(mac_dev->dev,
			     "FM_MAC_ModifyMacAddr() = 0x%08x\n", err);

	return _errno;
}

static void adjust_link(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	struct phy_device *phy_dev = mac_dev->phy_dev;
	int	 _errno;
	t_Error	 err;

	if (!phy_dev->link)
		return;

	err = FM_MAC_AdjustLink(
			((struct mac_priv_s *)macdev_priv(mac_dev))->mac,
			phy_dev->speed, phy_dev->duplex);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_AdjustLink() = 0x%08x\n",
				err);

	return;
}

/* Initializes driver's PHY state, and attaches to the PHY.
 * Returns 0 on success.
 */
static int dtsec_init_phy(struct net_device *net_dev)
{
	struct dpa_priv_s	*priv;
	struct mac_device	*mac_dev;
	struct phy_device	*phy_dev;

	priv = netdev_priv(net_dev);
	mac_dev = priv->mac_dev;

	if (!mac_dev->phy_node)
		phy_dev = phy_connect(net_dev, mac_dev->fixed_bus_id,
				&adjust_link, mac_dev->phy_if);
	else
		phy_dev = of_phy_connect(net_dev, mac_dev->phy_node,
				&adjust_link, 0, mac_dev->phy_if);
	if (unlikely(phy_dev == NULL) || IS_ERR(phy_dev)) {
		cpu_netdev_err(net_dev, "Could not connect to PHY %s\n",
				mac_dev->phy_node ?
					mac_dev->phy_node->full_name :
					mac_dev->fixed_bus_id);
		return phy_dev == NULL ? -ENODEV : PTR_ERR(phy_dev);
	}

	/* Remove any features not supported by the controller */
	phy_dev->supported &= priv->mac_dev->if_support;
	phy_dev->advertising = phy_dev->supported;

	priv->mac_dev->phy_dev = phy_dev;

	return 0;
}

static int xgmac_init_phy(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	struct phy_device *phy_dev;

	if (!mac_dev->phy_node)
		phy_dev = phy_attach(net_dev, mac_dev->fixed_bus_id,
				mac_dev->phy_if);
	else
		phy_dev = of_phy_attach(net_dev, mac_dev->phy_node, 0,
				mac_dev->phy_if);
	if (unlikely(phy_dev == NULL) || IS_ERR(phy_dev)) {
		cpu_netdev_err(net_dev, "Could not attach to PHY %s\n",
				mac_dev->phy_node ?
					mac_dev->phy_node->full_name :
					mac_dev->fixed_bus_id);
		return phy_dev == NULL ? -ENODEV : PTR_ERR(phy_dev);
	}

	phy_dev->supported &= priv->mac_dev->if_support;
	phy_dev->advertising = phy_dev->supported;

	mac_dev->phy_dev = phy_dev;

	return 0;
}

static int __cold uninit(struct mac_device *mac_dev)
{
	int			 _errno, __errno;
	t_Error			 err;
	const struct mac_priv_s	*priv;

	priv = macdev_priv(mac_dev);

	err = FM_MAC_Disable(priv->mac, e_COMM_MODE_RX_AND_TX);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Disable() = 0x%08x\n", err);

	err = FM_MAC_Free(priv->mac);
	__errno = -GET_ERROR_TYPE(err);
	if (unlikely(__errno < 0)) {
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Free() = 0x%08x\n", err);
		if (_errno < 0)
			_errno = __errno;
	}

	return _errno;
}

static int __cold ptp_enable(struct mac_device *mac_dev)
{
	int			 _errno;
	t_Error			 err;
	const struct mac_priv_s	*priv;

	priv = macdev_priv(mac_dev);

	err = FM_MAC_Enable1588TimeStamp(priv->mac);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Enable1588TimeStamp()"
				"= 0x%08x\n", err);
	return _errno;
}

static int __cold ptp_disable(struct mac_device *mac_dev)
{
	int			 _errno;
	t_Error			 err;
	const struct mac_priv_s	*priv;

	priv = macdev_priv(mac_dev);

	err = FM_MAC_Disable1588TimeStamp(priv->mac);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_MAC_Disable1588TimeStamp()"
				"= 0x%08x\n", err);
	return _errno;
}

static int __cold fm_rtc_enable(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int			 _errno;
	t_Error			 err;

	err = FM_RTC_Enable(fm_get_rtc_handle(mac_dev->fm_dev), 0);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_Enable = 0x%08x\n", err);

	return _errno;
}

static int __cold fm_rtc_disable(struct net_device *net_dev)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int			 _errno;
	t_Error			 err;

	err = FM_RTC_Disable(fm_get_rtc_handle(mac_dev->fm_dev));
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_Disable = 0x%08x\n", err);

	return _errno;
}

static int __cold fm_rtc_get_cnt(struct net_device *net_dev, uint64_t *ts)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int _errno;
	t_Error	err;

	err = FM_RTC_GetCurrentTime(fm_get_rtc_handle(mac_dev->fm_dev), ts);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_GetCurrentTime = 0x%08x\n",
				err);

	return _errno;
}

static int __cold fm_rtc_set_cnt(struct net_device *net_dev, uint64_t ts)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int _errno;
	t_Error	err;

	err = FM_RTC_SetCurrentTime(fm_get_rtc_handle(mac_dev->fm_dev), ts);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_SetCurrentTime = 0x%08x\n",
				err);

	return _errno;
}

static int __cold fm_rtc_get_drift(struct net_device *net_dev, uint32_t *drift)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int _errno;
	t_Error	err;

	err = FM_RTC_GetFreqCompensation(fm_get_rtc_handle(mac_dev->fm_dev),
			drift);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_GetFreqCompensation ="
				"0x%08x\n", err);

	return _errno;
}

static int __cold fm_rtc_set_drift(struct net_device *net_dev, uint32_t drift)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	int _errno;
	t_Error	err;

	err = FM_RTC_SetFreqCompensation(fm_get_rtc_handle(mac_dev->fm_dev),
			drift);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_SetFreqCompensation ="
				"0x%08x\n", err);

	return _errno;
}

static int __cold fm_rtc_set_alarm(struct net_device *net_dev, uint32_t id,
		uint64_t time)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	t_FmRtcAlarmParams alarm;
	int _errno;
	t_Error	err;

	alarm.alarmId = id;
	alarm.alarmTime = time;
	alarm.f_AlarmCallback = NULL;
	err = FM_RTC_SetAlarm(fm_get_rtc_handle(mac_dev->fm_dev),
			&alarm);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_SetAlarm ="
				"0x%08x\n", err);

	return _errno;
}

static int __cold fm_rtc_set_fiper(struct net_device *net_dev, uint32_t id,
		uint64_t fiper)
{
	struct dpa_priv_s *priv = netdev_priv(net_dev);
	struct mac_device *mac_dev = priv->mac_dev;
	t_FmRtcPeriodicPulseParams pp;
	int _errno;
	t_Error	err;

	pp.periodicPulseId = id;
	pp.periodicPulsePeriod = fiper;
	pp.f_PeriodicPulseCallback = NULL;
	err = FM_RTC_SetPeriodicPulse(fm_get_rtc_handle(mac_dev->fm_dev), &pp);
	_errno = -GET_ERROR_TYPE(err);
	if (unlikely(_errno < 0))
		dpaa_eth_err(mac_dev->dev, "FM_RTC_SetPeriodicPulse ="
				"0x%08x\n", err);

	return _errno;
}

static void __cold setup_dtsec(struct mac_device *mac_dev)
{
	mac_dev->init_phy	= dtsec_init_phy;
	mac_dev->init		= init;
	mac_dev->start		= start;
	mac_dev->stop		= stop;
	mac_dev->change_promisc	= change_promisc;
	mac_dev->change_addr    = change_addr;
	mac_dev->set_multi      = set_multi;
	mac_dev->uninit		= uninit;
	mac_dev->ptp_enable		= ptp_enable;
	mac_dev->ptp_disable		= ptp_disable;
	mac_dev->fm_rtc_enable		= fm_rtc_enable;
	mac_dev->fm_rtc_disable		= fm_rtc_disable;
	mac_dev->fm_rtc_get_cnt		= fm_rtc_get_cnt;
	mac_dev->fm_rtc_set_cnt		= fm_rtc_set_cnt;
	mac_dev->fm_rtc_get_drift	= fm_rtc_get_drift;
	mac_dev->fm_rtc_set_drift	= fm_rtc_set_drift;
	mac_dev->fm_rtc_set_alarm	= fm_rtc_set_alarm;
	mac_dev->fm_rtc_set_fiper	= fm_rtc_set_fiper;
}

static void __cold setup_xgmac(struct mac_device *mac_dev)
{
	mac_dev->init_phy	= xgmac_init_phy;
	mac_dev->init		= init;
	mac_dev->start		= start;
	mac_dev->stop		= stop;
	mac_dev->change_promisc	= change_promisc;
	mac_dev->change_addr    = change_addr;
	mac_dev->set_multi      = set_multi;
	mac_dev->uninit		= uninit;
}

void (*const mac_setup[])(struct mac_device *mac_dev) = {
	[DTSEC] = setup_dtsec,
	[XGMAC] = setup_xgmac
};
