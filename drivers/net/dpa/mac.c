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
#include <linux/of_net.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/phy.h>

#include "dpaa_eth-common.h"

#include "lnxwrp_fm_ext.h"

#include "mac.h"

#define DTSEC_SUPPORTED \
	(SUPPORTED_10baseT_Half \
	| SUPPORTED_10baseT_Full \
	| SUPPORTED_100baseT_Half \
	| SUPPORTED_100baseT_Full \
	| SUPPORTED_Autoneg \
	| SUPPORTED_MII)

static const char phy_str[][11] =
{
	[PHY_INTERFACE_MODE_MII]	= "mii",
	[PHY_INTERFACE_MODE_GMII]	= "gmii",
	[PHY_INTERFACE_MODE_SGMII]	= "sgmii",
	[PHY_INTERFACE_MODE_TBI]	= "tbi",
	[PHY_INTERFACE_MODE_RMII]	= "rmii",
	[PHY_INTERFACE_MODE_RGMII]	= "rgmii",
	[PHY_INTERFACE_MODE_RGMII_ID]	= "rgmii-id",
	[PHY_INTERFACE_MODE_RGMII_RXID]	= "rgmii-rxid",
	[PHY_INTERFACE_MODE_RGMII_TXID]	= "rgmii-txid",
	[PHY_INTERFACE_MODE_RTBI]	= "rtbi",
	[PHY_INTERFACE_MODE_XGMII]	= "xgmii"
};

static phy_interface_t __pure __attribute__((nonnull)) str2phy(const char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(phy_str); i++)
		if (strcmp(str, phy_str[i]) == 0)
			return (phy_interface_t)i;

	return PHY_INTERFACE_MODE_MII;
}

static const uint16_t phy2speed[] =
{
	[PHY_INTERFACE_MODE_MII]	= SPEED_100,
	[PHY_INTERFACE_MODE_GMII]	= SPEED_1000,
	[PHY_INTERFACE_MODE_SGMII]	= SPEED_1000,
	[PHY_INTERFACE_MODE_TBI]	= SPEED_1000,
	[PHY_INTERFACE_MODE_RMII]	= SPEED_100,
	[PHY_INTERFACE_MODE_RGMII]	= SPEED_1000,
	[PHY_INTERFACE_MODE_RGMII_ID]	= SPEED_1000,
	[PHY_INTERFACE_MODE_RGMII_RXID]	= SPEED_1000,
	[PHY_INTERFACE_MODE_RGMII_TXID]	= SPEED_1000,
	[PHY_INTERFACE_MODE_RTBI]	= SPEED_1000,
	[PHY_INTERFACE_MODE_XGMII]	= SPEED_10000
};

static struct mac_device * __cold
alloc_macdev(struct device *dev, size_t sizeof_priv, void (*setup)(struct mac_device *mac_dev))
{
	struct mac_device	*mac_dev;

	mac_dev = devm_kzalloc(dev, sizeof(*mac_dev) + sizeof_priv, GFP_KERNEL);
	if (unlikely(mac_dev == NULL))
		mac_dev = ERR_PTR(-ENOMEM);
	else {
		mac_dev->dev = dev;
		dev_set_drvdata(dev, mac_dev);
		setup(mac_dev);
	}

	return mac_dev;
}

static int __cold free_macdev(struct mac_device *mac_dev)
{
	dev_set_drvdata(mac_dev->dev, NULL);

	return mac_dev->uninit(mac_dev);
}

static const struct of_device_id mac_match[] = {
	[DTSEC] = {
		.compatible	= "fsl,fman-1g-mac"
	},
	[XGMAC] = {
		.compatible	= "fsl,fman-10g-mac"
	},
	{}
};
MODULE_DEVICE_TABLE(of, mac_match);

static int __cold mac_probe(struct platform_device *_of_dev)
{
	int			 _errno, i, lenp;
	struct device		*dev;
	struct device_node	*mac_node, *dev_node;
	struct mac_device	*mac_dev;
	struct platform_device	*of_dev;
	struct resource		 res;
	const uint8_t		*mac_addr;
	const char		*char_prop;
	const phandle		*phandle_prop;
	const uint32_t		*uint32_prop;
        const struct of_device_id *match;

	dev = &_of_dev->dev;
	mac_node = dev->of_node;

        match = of_match_device(mac_match, dev);
        if (!match)
                return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(mac_match) - 1 && match != mac_match + i; i++);
	BUG_ON(i >= ARRAY_SIZE(mac_match) - 1);

	mac_dev = alloc_macdev(dev, mac_sizeof_priv[i], mac_setup[i]);
	if (IS_ERR(mac_dev)) {
		_errno = PTR_ERR(mac_dev);
		dpaa_eth_err(dev, "alloc_macdev() = %d\n", _errno);
		goto _return;
	}

	INIT_LIST_HEAD(&mac_dev->mc_addr_list);

	/* Get the FM node */
	dev_node = of_get_parent(mac_node);
	if (unlikely(dev_node == NULL)) {
		dpaa_eth_err(dev, "of_get_parent(%s) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}

	of_dev = of_find_device_by_node(dev_node);
	if (unlikely(of_dev == NULL)) {
		dpaa_eth_err(dev, "of_find_device_by_node(%s) failed\n",
				dev_node->full_name);
		_errno = -EINVAL;
		goto _return_of_node_put;
	}

	mac_dev->fm_dev = fm_bind(&of_dev->dev);
	if (unlikely(mac_dev->fm_dev == NULL)) {
		dpaa_eth_err(dev, "fm_bind(%s) failed\n", dev_node->full_name);
		_errno = -ENODEV;
		goto _return_of_node_put;
	}

    mac_dev->fm = (void *)fm_get_handle(mac_dev->fm_dev);
	of_node_put(dev_node);

	/* Get the address of the memory mapped registers */
	_errno = of_address_to_resource(mac_node, 0, &res);
	if (unlikely(_errno < 0)) {
		dpaa_eth_err(dev, "of_address_to_resource(%s) = %d\n",
				mac_node->full_name, _errno);
		goto _return_dev_set_drvdata;
	}

	mac_dev->res = __devm_request_region(
		dev,
		fm_get_mem_region(mac_dev->fm_dev),
		res.start, res.end + 1 - res.start, "mac");
	if (unlikely(mac_dev->res == NULL)) {
		dpaa_eth_err(dev, "__devm_request_mem_region(mac) failed\n");
		_errno = -EBUSY;
		goto _return_dev_set_drvdata;
	}

	mac_dev->vaddr = devm_ioremap(dev, mac_dev->res->start,
				      mac_dev->res->end + 1 - mac_dev->res->start);
	if (unlikely(mac_dev->vaddr == NULL)) {
		dpaa_eth_err(dev, "devm_ioremap() failed\n");
		_errno = -EIO;
		goto _return_dev_set_drvdata;
	}

	/*
	 * XXX: Warning, future versions of Linux will most likely not even
	 * call the driver code to allow us to override the TBIPA value,
	 * we'll need to address this when we move to newer kernel rev
	 */
#define TBIPA_OFFSET		0x1c
#define TBIPA_DEFAULT_ADDR	5
	mac_dev->tbi_node = of_parse_phandle(mac_node, "tbi-handle", 0);
	if (mac_dev->tbi_node) {
		u32 tbiaddr = TBIPA_DEFAULT_ADDR;

		uint32_prop = of_get_property(mac_dev->tbi_node, "reg", NULL);
		if (uint32_prop)
			tbiaddr = *uint32_prop;
		out_be32(mac_dev->vaddr + TBIPA_OFFSET, tbiaddr);
	}

	if (!of_device_is_available(mac_node)) {
		devm_iounmap(dev, mac_dev->vaddr);
		__devm_release_region(dev, fm_get_mem_region(mac_dev->fm_dev),
			res.start, res.end + 1 - res.start);
		fm_unbind(mac_dev->fm_dev);
		devm_kfree(dev, mac_dev);
		dev_set_drvdata(dev, NULL);
		return -ENODEV;
	}

	/* Get the cell-index */
	uint32_prop = of_get_property(mac_node, "cell-index", &lenp);
	if (unlikely(uint32_prop == NULL)) {
		dpaa_eth_err(dev, "of_get_property(%s, cell-index) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}
	BUG_ON(lenp != sizeof(uint32_t));
	mac_dev->cell_index = *uint32_prop;

	/* Get the MAC address */
	mac_addr = of_get_mac_address(mac_node);
	if (unlikely(mac_addr == NULL)) {
		dpaa_eth_err(dev, "of_get_mac_address(%s) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}
	memcpy(mac_dev->addr, mac_addr, sizeof(mac_dev->addr));

	/* Get the port handles */
	phandle_prop = of_get_property(mac_node, "fsl,port-handles", &lenp);
	if (unlikely(phandle_prop == NULL)) {
		dpaa_eth_err(dev, "of_get_property(%s, port-handles) failed\n",
				mac_node->full_name);
		_errno = -EINVAL;
		goto _return_dev_set_drvdata;
	}
	BUG_ON(lenp != sizeof(phandle) * ARRAY_SIZE(mac_dev->port_dev));

	for_each_port_device(i, mac_dev->port_dev) {
		/* Find the port node */
		dev_node = of_find_node_by_phandle(phandle_prop[i]);
		if (unlikely(dev_node == NULL)) {
			dpaa_eth_err(dev, "of_find_node_by_phandle() failed\n");
			_errno = -EINVAL;
			goto _return_of_node_put;
		}

		of_dev = of_find_device_by_node(dev_node);
		if (unlikely(of_dev == NULL)) {
			dpaa_eth_err(dev, "of_find_device_by_node(%s) failed\n",
					dev_node->full_name);
			_errno = -EINVAL;
			goto _return_of_node_put;
		}

		mac_dev->port_dev[i] = fm_port_bind(&of_dev->dev);
		if (unlikely(mac_dev->port_dev[i] == NULL)) {
			dpaa_eth_err(dev, "dev_get_drvdata(%s) failed\n",
					dev_node->full_name);
			_errno = -EINVAL;
			goto _return_of_node_put;
		}
		of_node_put(dev_node);
	}

	/* Get the PHY connection type */
	char_prop = (const char *)of_get_property(mac_node,
						"phy-connection-type", NULL);
	if (unlikely(char_prop == NULL)) {
		dpaa_eth_warning(dev,
				"of_get_property(%s, phy-connection-type) "
				"failed. Defaulting to MII\n",
				mac_node->full_name);
		mac_dev->phy_if = PHY_INTERFACE_MODE_MII;
	} else
		mac_dev->phy_if = str2phy(char_prop);

	mac_dev->link		= false;
	mac_dev->half_duplex	= false;
	mac_dev->speed		= phy2speed[mac_dev->phy_if];
	mac_dev->max_speed	= mac_dev->speed;
	mac_dev->if_support = DTSEC_SUPPORTED;
	/* We don't support half-duplex in SGMII mode */
	if (strstr(char_prop, "sgmii"))
		mac_dev->if_support &= ~(SUPPORTED_10baseT_Half |
					SUPPORTED_100baseT_Half);

	/* Gigabit support (no half-duplex) */
	if (mac_dev->max_speed == 1000)
		mac_dev->if_support |= SUPPORTED_1000baseT_Full;

	/* The 10G interface only supports one mode */
	if (strstr(char_prop, "xgmii"))
		mac_dev->if_support = SUPPORTED_10000baseT_Full;

	/* Get the rest of the PHY information */
	mac_dev->phy_node = of_parse_phandle(mac_node, "phy-handle", 0);
	if (mac_dev->phy_node == NULL) {
		int sz;
		const u32 *phy_id = of_get_property(mac_node, "fixed-link",
							&sz);
		if (!phy_id || sz < sizeof(*phy_id)) {
			cpu_dev_err(dev, "No PHY (or fixed link) found\n");
			_errno = -EINVAL;
			goto _return_dev_set_drvdata;
		}

		sprintf(mac_dev->fixed_bus_id, PHY_ID_FMT, "0", phy_id[0]);
	}

	_errno = mac_dev->init(mac_dev);
	if (unlikely(_errno < 0)) {
		dpaa_eth_err(dev, "mac_dev->init() = %d\n", _errno);
		goto _return_dev_set_drvdata;
	}

	cpu_dev_info(dev,
		"FMan MAC address: %02hx:%02hx:%02hx:%02hx:%02hx:%02hx\n",
		     mac_dev->addr[0], mac_dev->addr[1], mac_dev->addr[2],
		     mac_dev->addr[3], mac_dev->addr[4], mac_dev->addr[5]);

	goto _return;

_return_of_node_put:
	of_node_put(dev_node);
_return_dev_set_drvdata:
	dev_set_drvdata(dev, NULL);
_return:
	return _errno;
}

static int __cold mac_remove(struct platform_device *of_dev)
{
	int			 i, _errno;
	struct device		*dev;
	struct mac_device	*mac_dev;

	dev = &of_dev->dev;
	mac_dev = (struct mac_device *)dev_get_drvdata(dev);

	for_each_port_device(i, mac_dev->port_dev)
		fm_port_unbind(mac_dev->port_dev[i]);

	fm_unbind(mac_dev->fm_dev);

	_errno = free_macdev(mac_dev);

	return _errno;
}

static struct platform_driver mac_driver = {
	.driver = {
		.name		= KBUILD_MODNAME,
		.of_match_table	= mac_match,
		.owner		= THIS_MODULE,
	},
	.probe		= mac_probe,
	.remove		= mac_remove,
};

static int __init __cold mac_load(void)
{
	int	 _errno;

	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	cpu_pr_info(KBUILD_MODNAME ": %s (" VERSION ")\n", mac_driver_description);

	_errno = platform_driver_register(&mac_driver);
	if (unlikely(_errno < 0)) {
		cpu_pr_err(KBUILD_MODNAME ": %s:%hu:%s(): of_register_platform_driver() = %d\n",
			   __file__, __LINE__, __func__, _errno);
		goto _return;
	}

	goto _return;

_return:
	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);

	return _errno;
}
module_init(mac_load);

static void __exit __cold mac_unload(void)
{
	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	platform_driver_unregister(&mac_driver);

	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);
}
module_exit(mac_unload);
