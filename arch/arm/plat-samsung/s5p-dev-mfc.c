/*
 * Copyright (C) 2010-2011 Samsung Electronics Co.Ltd
 *
 * Base S5P MFC resource and device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/memblock.h>
#include <linux/ioport.h>
#include <linux/of_fdt.h>
#include <linux/of.h>

#include <plat/mfc.h>

#ifdef CONFIG_SAMSUNG_ATAGS
#include <mach/map.h>
#include <mach/irqs.h>
#include <plat/devs.h>

static struct resource s5p_mfc_resource[] = {
	[0] = DEFINE_RES_MEM(S5P_PA_MFC, SZ_64K),
	[1] = DEFINE_RES_IRQ(IRQ_MFC),
};

struct platform_device s5p_device_mfc = {
	.name		= "s5p-mfc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_mfc_resource),
	.resource	= s5p_mfc_resource,
};

/*
 * MFC hardware has 2 memory interfaces which are modelled as two separate
 * platform devices to let dma-mapping distinguish between them.
 *
 * MFC parent device (s5p_device_mfc) must be registered before memory
 * interface specific devices (s5p_device_mfc_l and s5p_device_mfc_r).
 */

struct platform_device s5p_device_mfc_l = {
	.name		= "s5p-mfc-l",
	.id		= -1,
	.dev		= {
		.parent			= &s5p_device_mfc.dev,
		.dma_mask		= &s5p_device_mfc_l.dev.coherent_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};

struct platform_device s5p_device_mfc_r = {
	.name		= "s5p-mfc-r",
	.id		= -1,
	.dev		= {
		.parent			= &s5p_device_mfc.dev,
		.dma_mask		= &s5p_device_mfc_r.dev.coherent_dma_mask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
#else
struct device s5p_device_mfc_l;
struct device s5p_device_mfc_r;
#endif

void __init s5p_mfc_reserve_mem(phys_addr_t rbase, unsigned int rsize,
				phys_addr_t lbase, unsigned int lsize)
{
	device_initialize(&s5p_device_mfc_r);
	s5p_device_mfc_r.coherent_dma_mask = DMA_BIT_MASK(32);
	if (dma_declare_contiguous(&s5p_device_mfc_r, rsize, 0, __pa(high_memory)))
		pr_err("Failed to reserve memory for MFC device (%u bytes at 0x%08lx)\n", rsize, (unsigned long) rbase);

	device_initialize(&s5p_device_mfc_l);
	s5p_device_mfc_l.coherent_dma_mask = DMA_BIT_MASK(32);
	if (dma_declare_contiguous(&s5p_device_mfc_l, lsize, 0, __pa(high_memory)))
		pr_err("Failed to reserve memory for MFC device (%u bytes at 0x%08lx)\n", lsize, (unsigned long) lbase);

}

#ifdef CONFIG_SAMSUNG_ATAGS
static int __init s5p_mfc_memory_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s5p_mfc_mem); i++) {
		struct s5p_mfc_reserved_mem *area = &s5p_mfc_mem[i];
		if (!area->base)
			continue;

		if (dma_declare_coherent_memory(area->dev, area->base,
				area->base, area->size,
				DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE) == 0)
			printk(KERN_ERR "Failed to declare coherent memory for MFC device (%ld bytes at 0x%08lx)\n",
			       area->size, (unsigned long) area->base);
	}
	return 0;
}
device_initcall(s5p_mfc_memory_init);
#endif

#ifdef CONFIG_OF
int __init s5p_fdt_alloc_mfc_mem(unsigned long node, const char *uname,
				int depth, void *data)
{
	const __be32 *prop;
	int len;
	struct s5p_mfc_dt_meminfo mfc_mem;

	if (!data)
		return 0;

	if (!of_flat_dt_is_compatible(node, data))
		return 0;

	prop = of_get_flat_dt_prop(node, "samsung,mfc-l", &len);
	if (!prop || (len != 2 * sizeof(unsigned long)))
		return 0;

	mfc_mem.loff = be32_to_cpu(prop[0]);
	mfc_mem.lsize = be32_to_cpu(prop[1]);

	prop = of_get_flat_dt_prop(node, "samsung,mfc-r", &len);
	if (!prop || (len != 2 * sizeof(unsigned long)))
		return 0;

	mfc_mem.roff = be32_to_cpu(prop[0]);
	mfc_mem.rsize = be32_to_cpu(prop[1]);

	s5p_mfc_reserve_mem(mfc_mem.roff, mfc_mem.rsize,
			mfc_mem.loff, mfc_mem.lsize);

	return 1;
}
#endif
