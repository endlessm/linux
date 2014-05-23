/*
 * Copyright 2006-2008, Michael Ellerman, IBM Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
 * License.
 *
 * Borrowed from powerpc arch
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/bitmap.h>
#include <asm/msi_bitmap.h>
#include <asm/setup.h>

int msi_bitmap_alloc_hwirqs(struct msi_bitmap *bmp, int num)
{
	unsigned long flags;
	int offset, order = get_count_order(num);

	spin_lock_irqsave(&bmp->lock, flags);
	/*
	 * This is fast, but stricter than we need. We might want to add
	 * a fallback routine which does a linear search with no alignment.
	 */
	offset = bitmap_find_free_region(bmp->bitmap, bmp->irq_count, order);
	spin_unlock_irqrestore(&bmp->lock, flags);

	pr_debug("msi_bitmap: allocated 0x%x (2^%d) at offset 0x%x\n",
		 num, order, offset);

	return offset;
}

void msi_bitmap_free_hwirqs(struct msi_bitmap *bmp, unsigned int offset,
			    unsigned int num)
{
	unsigned long flags;
	int order = get_count_order(num);

	pr_debug("msi_bitmap: freeing 0x%x (2^%d) at offset 0x%x\n",
		 num, order, offset);

	spin_lock_irqsave(&bmp->lock, flags);
	bitmap_release_region(bmp->bitmap, offset, order);
	spin_unlock_irqrestore(&bmp->lock, flags);
}

void msi_bitmap_reserve_hwirq(struct msi_bitmap *bmp, unsigned int hwirq)
{
	unsigned long flags;

	pr_debug("msi_bitmap: reserving hwirq 0x%x\n", hwirq);

	spin_lock_irqsave(&bmp->lock, flags);
	bitmap_allocate_region(bmp->bitmap, hwirq, 0);
	spin_unlock_irqrestore(&bmp->lock, flags);
}

/**
 * msi_bitmap_reserve_dt_hwirqs - Reserve irqs specified in the device tree.
 * @bmp: pointer to the MSI bitmap.
 *
 * Looks in the device tree to see if there is a property specifying which
 * irqs can be used for MSI. If found those irqs reserved in the device tree
 * are reserved in the bitmap.
 *
 * Returns 0 for success, < 0 if there was an error, and > 0 if no property
 * was found in the device tree.
 **/
int msi_bitmap_reserve_dt_hwirqs(struct msi_bitmap *bmp)
{
	int i, j, len = 2;
	u32 array[2];
	u32 *p = array;
	int ret;

	if (!bmp->of_node)
		return 1;

	ret = of_property_read_u32_array(bmp->of_node,
					 "msi-available-ranges", p, len);
	if (ret)
		return ret;

	bitmap_allocate_region(bmp->bitmap, 0, get_count_order(bmp->irq_count));

	spin_lock(&bmp->lock);

	/* Format is: (<u32 start> <u32 count>)+ */
	len /= 2 * sizeof(u32);
	for (i = 0; i < len; i++, p += 2) {
		for (j = 0; j < *(p + 1); j++)
			bitmap_release_region(bmp->bitmap, *p + j, 0);
	}

	spin_unlock(&bmp->lock);

	return 0;
}

int msi_bitmap_alloc(struct msi_bitmap *bmp, unsigned int irq_count,
		     struct device_node *of_node)
{
	int size;

	if (!irq_count)
		return -EINVAL;

	size = BITS_TO_LONGS(irq_count) * sizeof(long);
	pr_debug("msi_bitmap: allocator bitmap size is 0x%x bytes\n", size);

	bmp->bitmap = kzalloc(size, GFP_KERNEL);
	if (!bmp->bitmap) {
		pr_debug("msi_bitmap: ENOMEM allocating allocator bitmap!\n");
		return -ENOMEM;
	}

	/* We zalloc'ed the bitmap, so all irqs are free by default */
	spin_lock_init(&bmp->lock);
	bmp->of_node = of_node_get(of_node);
	bmp->irq_count = irq_count;

	return 0;
}

void msi_bitmap_free(struct msi_bitmap *bmp)
{
	/* we can't free the bitmap we don't know if it's bootmem etc. */
	of_node_put(bmp->of_node);
	bmp->bitmap = NULL;
}
