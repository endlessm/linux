/*
 * AMD ALSA SoC PCM Driver
 *
 * Copyright 2016 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/pci.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include "acp3x.h"

struct acp3x_dev_data {
	void __iomem *acp3x_base;
	bool acp3x_audio_mode;
	struct resource *res;
	struct platform_device *pdev[3];
};

static int snd_acp3x_probe(struct pci_dev *pci,
			   const struct pci_device_id *pci_id)
{
	int ret;
	u32 addr, val;
	struct acp3x_dev_data *adata;
	struct platform_device_info pdevinfo;
	unsigned int irqflags;

	if (pci_enable_device(pci)) {
		dev_err(&pci->dev, "pci_enable_device failed\n");
		return -ENODEV;
	}

	ret = pci_request_regions(pci, "AMD ACP3x audio");
	if (ret < 0) {
		dev_err(&pci->dev, "pci_request_regions failed\n");
		goto disable_pci;
	}

	adata = devm_kzalloc(&pci->dev, sizeof(struct acp3x_dev_data),
				GFP_KERNEL);
	if (adata == NULL) {
		ret = -ENOMEM;
		goto release_regions;
	}

	/* check for msi interrupt support */
	ret = pci_enable_msi(pci);
	if (ret)
		/* msi is not enabled */
		irqflags = IRQF_SHARED;
	else
		/* msi is enabled */
		irqflags = 0;

	addr = pci_resource_start(pci, 0);
	adata->acp3x_base = ioremap(addr, pci_resource_len(pci, 0));
	if (adata->acp3x_base == NULL) {
		ret = -ENOMEM;
		goto release_regions;
	}

	pci_set_drvdata(pci, adata);

	val = rv_readl(adata->acp3x_base + mmACP_I2S_PIN_CONFIG);
	if (val == 0x4) {
		adata->res = devm_kzalloc(&pci->dev,
				sizeof(struct resource) * 2,
				GFP_KERNEL);
		if (adata->res == NULL) {
			ret = -ENOMEM;
			goto unmap_mmio;
		}

		adata->res[0].name = "acp3x_i2s_iomem";
		adata->res[0].flags = IORESOURCE_MEM;
		adata->res[0].start = addr;
		adata->res[0].end = addr + (ACP3x_REG_END - ACP3x_REG_START);

		adata->res[1].name = "acp3x_i2s_irq";
		adata->res[1].flags = IORESOURCE_IRQ;
		adata->res[1].start = pci->irq;
		adata->res[1].end = pci->irq;

		adata->acp3x_audio_mode = ACP3x_I2S_MODE;

		memset(&pdevinfo, 0, sizeof(pdevinfo));
		pdevinfo.name = "acp3x_rv_i2s";
		pdevinfo.id = 0;
		pdevinfo.parent = &pci->dev;
		pdevinfo.num_res = 2;
		pdevinfo.res = adata->res;
		pdevinfo.data = &irqflags;
		pdevinfo.size_data = sizeof(irqflags);

		adata->pdev[0] = platform_device_register_full(&pdevinfo);
		if (adata->pdev[0] == NULL) {
			dev_err(&pci->dev, "cannot register %s device\n",
				pdevinfo.name);
			ret = -ENODEV;
			goto unmap_mmio;
		}

		/* create dummy codec device */
		adata->pdev[1] = platform_device_register_simple("dummy_w5102",
					0, NULL, 0);
		if (IS_ERR(adata->pdev[1])) {
			dev_err(&pci->dev, "Cannot register dummy_w5102\n");
			ret = -ENODEV;
			goto unregister_pdev0;
		}
		/* create dummy mach device */
		adata->pdev[2] = platform_device_register_simple(
					"acp3x_w5102_mach", 0, NULL, 0);
		if (IS_ERR(adata->pdev[2])) {
			dev_err(&pci->dev, "Cannot register acp3x_w5102_mach\n");
			ret = -ENODEV;
			goto unregister_pdev1;
		}
	} else {
		dev_err(&pci->dev, "Inavlid ACP audio mode : %d\n", val);
		ret = -ENODEV;
		goto unmap_mmio;
	}

	return 0;

unregister_pdev1:
	platform_device_unregister(adata->pdev[1]);
unregister_pdev0:
	platform_device_unregister(adata->pdev[0]);
unmap_mmio:
	pci_disable_msi(pci);
	iounmap(adata->acp3x_base);
release_regions:
	pci_release_regions(pci);
disable_pci:
	pci_disable_device(pci);

	return ret;
}

static void snd_acp3x_remove(struct pci_dev *pci)
{
	int i;
	struct acp3x_dev_data *adata = pci_get_drvdata(pci);

	if (adata->acp3x_audio_mode == ACP3x_I2S_MODE) {
		for (i = 2; i >= 0; i--)
			platform_device_unregister(adata->pdev[i]);
	}

	iounmap(adata->acp3x_base);

	pci_disable_msi(pci);
	pci_release_regions(pci);
	pci_disable_device(pci);
}

static const struct pci_device_id snd_acp3x_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_AMD, 0x15e2),
	.class = PCI_CLASS_MULTIMEDIA_OTHER << 8,
	.class_mask = 0xffffff },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, snd_acp3x_ids);

static struct pci_driver acp3x_driver  = {
	.name = KBUILD_MODNAME,
	.id_table = snd_acp3x_ids,
	.probe = snd_acp3x_probe,
	.remove = snd_acp3x_remove,
};

module_pci_driver(acp3x_driver);

MODULE_AUTHOR("Maruthi.Bayyavarapu@amd.com");
MODULE_DESCRIPTION("AMD ACP3x PCI driver");
MODULE_LICENSE("GPL v2");
