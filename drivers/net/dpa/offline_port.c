/*
 * Copyright 2011 Freescale Semiconductor Inc.
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

/*
 * Offline Parsing / Host Command port driver for FSL QorIQ FMan.
 * Validates device-tree configuration and sets up the offline ports.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>

#include "offline_port.h"
#include "dpaa_eth-common.h"

#define OH_MOD_DESCRIPTION	"FSL FMan Offline Parsing port driver"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Bogdan Hamciuc <bogdan.hamciuc@freescale.com>");
MODULE_DESCRIPTION(OH_MOD_DESCRIPTION);


static const struct of_device_id oh_port_match_table[] = {
	{
		.compatible	= "fsl,dpa-oh"
	},
	{
		.compatible	= "fsl,dpa-oh-shared"
	},
	{}
};
MODULE_DEVICE_TABLE(of, oh_port_match_table);

static int oh_port_remove(struct platform_device *_of_dev);
static int oh_port_probe(struct platform_device *_of_dev);

static struct platform_driver oh_port_driver = {
	.driver = {
		.name		= KBUILD_MODNAME,
		.of_match_table	= oh_port_match_table,
		.owner		= THIS_MODULE,
	},
	.probe		= oh_port_probe,
	.remove		= oh_port_remove,
};

/* Allocation code for the OH port's PCD frame queues */
static int __cold oh_alloc_pcd_fqids(struct device *dev,
	uint32_t num,
	uint8_t alignment,
	uint32_t *base_fqid)
{
	cpu_dev_crit(dev, "callback not implemented!\n");
	BUG();

	return 0;
}

static int __cold oh_free_pcd_fqids(struct device *dev, uint32_t base_fqid)
{
	dpaa_eth_crit(dev, "callback not implemented!\n");
	BUG();

	return 0;
}

static int
oh_port_probe(struct platform_device *_of_dev)
{
	struct device		*dpa_oh_dev;
	struct device_node	*dpa_oh_node;
	int			 lenp, _errno = 0, fq_idx;
	const phandle		*oh_port_handle;
	struct platform_device	*oh_of_dev;
	struct device_node	*oh_node;
	struct device		*oh_dev;
	struct dpa_oh_config_s	*oh_config;
	uint32_t		*oh_all_queues;
	uint32_t		 queues_count;
	uint32_t		 crt_fqid_base;
	uint32_t		 crt_fq_count;
	struct fm_port_non_rx_params	oh_port_tx_params;
	struct fm_port_pcd_param	oh_port_pcd_params;
	/* True if the current partition owns the OH port. */
	bool init_oh_port;
	const struct of_device_id *match;

	dpa_oh_dev = &_of_dev->dev;
	dpa_oh_node = dpa_oh_dev->of_node;
	BUG_ON(dpa_oh_node == NULL);

	match = of_match_device(oh_port_match_table, dpa_oh_dev);
	if (!match)
		return -EINVAL;

	cpu_dev_dbg(dpa_oh_dev, "Probing OH port...\n");

	/*
	 * Find the referenced OH node
	 */

	oh_port_handle = of_get_property(dpa_oh_node,
		"fsl,fman-oh-port", &lenp);
	if (oh_port_handle == NULL) {
		cpu_dev_err(dpa_oh_dev, "No OH port handle found in node %s\n",
			dpa_oh_node->full_name);
		return -EINVAL;
	}

	BUG_ON(lenp % sizeof(*oh_port_handle));
	if (lenp != sizeof(*oh_port_handle)) {
		cpu_dev_err(dpa_oh_dev, "Found %lu OH port bindings in node %s, "
			"only 1 phandle is allowed.\n",
			(unsigned long int)(lenp / sizeof(*oh_port_handle)), dpa_oh_node->full_name);
		return -EINVAL;
	}

	/* Read configuration for the OH port */
	oh_node = of_find_node_by_phandle(*oh_port_handle);
	if (oh_node == NULL) {
		cpu_dev_err(dpa_oh_dev, "Can't find OH node referenced from "
			"node %s\n", dpa_oh_node->full_name);
		return -EINVAL;
	}
	cpu_dev_info(dpa_oh_dev, "Found OH node handle compatible with %s.\n",
		match->compatible);

	oh_of_dev = of_find_device_by_node(oh_node);
	BUG_ON(oh_of_dev == NULL);
	oh_dev = &oh_of_dev->dev;
	of_node_put(oh_node);

	/*
	 * The OH port must be initialized exactly once.
	 * The following scenarios are of interest:
	 *	- the node is Linux-private (will always initialize it);
	 *	- the node is shared between two Linux partitions
	 *	  (only one of them will initialize it);
	 *	- the node is shared between a Linux and a LWE partition
	 *	  (Linux will initialize it) - "fsl,dpa-oh-shared"
	 */

	/* Check if the current partition owns the OH port
	 * and ought to initialize it. It may be the case that we leave this
	 * to another (also Linux) partition. */
	init_oh_port = strcmp(match->compatible, "fsl,dpa-oh-shared");

	/* If we aren't the "owner" of the OH node, we're done here. */
	if (!init_oh_port) {
		cpu_dev_dbg(dpa_oh_dev, "Not owning the shared OH port %s, "
			"will not initialize it.\n", oh_node->full_name);
		return 0;
	}

	/* Allocate OH dev private data */
	oh_config = devm_kzalloc(dpa_oh_dev, sizeof(*oh_config), GFP_KERNEL);
	if (oh_config == NULL) {
		cpu_dev_err(dpa_oh_dev, "Can't allocate private data for "
			"OH node %s referenced from node %s!\n",
			oh_node->full_name, dpa_oh_node->full_name);
		return -ENOMEM;
	}

	/*
	 * Read FQ ids/nums for the DPA OH node
	 */
	oh_all_queues = (uint32_t *)of_get_property(dpa_oh_node,
		"fsl,qman-frame-queues-oh", &lenp);
	if (oh_all_queues == NULL) {
		cpu_dev_err(dpa_oh_dev, "No frame queues have been "
			"defined for OH node %s referenced from node %s\n",
			oh_node->full_name, dpa_oh_node->full_name);
		_errno = -EINVAL;
		goto return_kfree;
	}

	/* Check that the OH error and default FQs are there */
	BUG_ON(lenp % (2 * sizeof(*oh_all_queues)));
	queues_count = lenp / (2 * sizeof(*oh_all_queues));
	if (queues_count != 2) {
		dpaa_eth_err(dpa_oh_dev, "Error and Default queues must be "
			"defined for OH node %s referenced from node %s\n",
			oh_node->full_name, dpa_oh_node->full_name);
		_errno = -EINVAL;
		goto return_kfree;
	}

	/* Read the FQIDs defined for this OH port */
	cpu_dev_dbg(dpa_oh_dev, "Reading %d queues...\n", queues_count);
	fq_idx = 0;

	/* Error FQID - must be present */
	crt_fqid_base = oh_all_queues[fq_idx++];
	crt_fq_count = oh_all_queues[fq_idx++];
	if (crt_fq_count != 1) {
		cpu_dev_err(dpa_oh_dev, "Only 1 Error FQ allowed in OH node %s "
			"referenced from node %s (read: %d FQIDs).\n",
			oh_node->full_name, dpa_oh_node->full_name,
			crt_fq_count);
		_errno = -EINVAL;
		goto return_kfree;
	}
	oh_config->error_fqid = crt_fqid_base;
	cpu_dev_dbg(dpa_oh_dev, "Read Error FQID 0x%x for OH port %s.\n",
		oh_config->error_fqid, oh_node->full_name);

	/* Default FQID - must be present */
	crt_fqid_base = oh_all_queues[fq_idx++];
	crt_fq_count = oh_all_queues[fq_idx++];
	if (crt_fq_count != 1) {
		cpu_dev_err(dpa_oh_dev, "Only 1 Default FQ allowed "
			"in OH node %s referenced from %s (read: %d FQIDs).\n",
			oh_node->full_name, dpa_oh_node->full_name,
			crt_fq_count);
		_errno = -EINVAL;
		goto return_kfree;
	}
	oh_config->default_fqid = crt_fqid_base;
	cpu_dev_dbg(dpa_oh_dev, "Read Default FQID 0x%x for OH port %s.\n",
		oh_config->default_fqid, oh_node->full_name);

	/* Get a handle to the fm_port so we can set
	 * its configuration params */
	oh_config->oh_port = fm_port_bind(oh_dev);
	if (oh_config->oh_port == NULL) {
		cpu_dev_err(dpa_oh_dev, "NULL drvdata from fm port dev %s!\n",
			oh_node->full_name);
		_errno = -EINVAL;
		goto return_kfree;
	}

	/* Set Tx params */
	dpaa_eth_init_port(tx, oh_config->oh_port, oh_port_tx_params,
		oh_config->error_fqid, oh_config->default_fqid, FALSE);
	/* Set PCD params */
	oh_port_pcd_params.cba = oh_alloc_pcd_fqids;
	oh_port_pcd_params.cbf = oh_free_pcd_fqids;
	oh_port_pcd_params.dev = dpa_oh_dev;
	fm_port_pcd_bind(oh_config->oh_port, &oh_port_pcd_params);

	dev_set_drvdata(dpa_oh_dev, oh_config);

	/* Enable the OH port */
	fm_port_enable(oh_config->oh_port);
	cpu_dev_info(dpa_oh_dev, "OH port %s enabled.\n", oh_node->full_name);

	return 0;

return_kfree:
	devm_kfree(dpa_oh_dev, oh_config);
	return _errno;
}

static int __cold oh_port_remove(struct platform_device *_of_dev)
{
	int _errno = 0;
	struct dpa_oh_config_s *oh_config;

	cpu_pr_info("Removing OH port...\n");

	oh_config = dev_get_drvdata(&_of_dev->dev);
	if (oh_config == NULL) {
		cpu_pr_err(KBUILD_MODNAME
			": %s:%hu:%s(): No OH config in device private data!\n",
			__file__, __LINE__, __func__);
		_errno = -ENODEV;
		goto return_error;
	}
	if (oh_config->oh_port == NULL) {
		cpu_pr_err(KBUILD_MODNAME
			": %s:%hu:%s(): No fm port in device private data!\n",
			__file__, __LINE__, __func__);
		_errno = -EINVAL;
		goto return_error;
	}

	fm_port_disable(oh_config->oh_port);
	devm_kfree(&_of_dev->dev, oh_config);
	dev_set_drvdata(&_of_dev->dev, NULL);

return_error:
	return _errno;
}

static int __init __cold oh_port_load(void)
{
	int _errno;

	cpu_pr_info(KBUILD_MODNAME ": " OH_MOD_DESCRIPTION " (" VERSION ")\n");

	_errno = platform_driver_register(&oh_port_driver);
	if (_errno < 0) {
		cpu_pr_err(KBUILD_MODNAME
			": %s:%hu:%s(): platform_driver_register() = %d\n",
			__file__, __LINE__, __func__, _errno);
	}

	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);
	return _errno;
}
module_init(oh_port_load);

static void __exit __cold oh_port_unload(void)
{
	cpu_pr_debug(KBUILD_MODNAME ": -> %s:%s()\n", __file__, __func__);

	platform_driver_unregister(&oh_port_driver);

	cpu_pr_debug(KBUILD_MODNAME ": %s:%s() ->\n", __file__, __func__);
}
module_exit(oh_port_unload);
