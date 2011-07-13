/* Copyright 2011 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
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

#include "bman_private.h"
#include "qman_private.h"

static const char dpa_uio_version[] = "USDPAA UIO portal driver v0.2";

static LIST_HEAD(uio_portal_list);

struct dpa_uio_info {
	atomic_t ref; /* exclusive, only one open() at a time */
	struct uio_info uio;
	void *addr_ci;
	char name[16]; /* big enough for "qman-uio-xx" */
	struct platform_device *pdev;
	struct list_head node;
};

static int dpa_uio_open(struct uio_info *info, struct inode *inode)
{
	struct dpa_uio_info *i = container_of(info, struct dpa_uio_info, uio);
	if (!atomic_dec_and_test(&i->ref)) {
		atomic_inc(&i->ref);
		return -EBUSY;
	}
	return 0;
}

static int dpa_uio_release(struct uio_info *info, struct inode *inode)
{
	struct dpa_uio_info *i = container_of(info, struct dpa_uio_info, uio);
	atomic_inc(&i->ref);
	return 0;
}

static int dpa_uio_mmap(struct uio_info *info, struct vm_area_struct *vma)
{
	struct uio_mem *mem;
	struct dpa_uio_info *i = container_of(info, struct dpa_uio_info, uio);

	if (vma->vm_pgoff == 0) {
		/* CENA */
		mem = &i->uio.mem[0];
		vma->vm_page_prot &=
			~(_PAGE_GUARDED | _PAGE_NO_CACHE | _PAGE_COHERENT);
	} else if (vma->vm_pgoff == 1) {
		/* CINH */
		mem = &i->uio.mem[1];
		vma->vm_page_prot |= _PAGE_GUARDED | _PAGE_NO_CACHE;
	} else {
		pr_err("%s: unknown mmap offset %d, rejecting\n",
			i->name, (int)vma->vm_pgoff);
		return -EINVAL;
	}
	if ((vma->vm_end - vma->vm_start) != mem->size) {
		pr_err("%s: invalid mmap() size %d, expect %d\n",
			i->name, (int)(vma->vm_end - vma->vm_start),
			(int)mem->size);
		return -EINVAL;
	}
	/* FIXME: UIO appears not to support sizeof(phys_addr_t) > sizeof(void*)
	 * as mem->addr is 32-bit. Also, it would have been more natural (and in
	 * keeping with UIO's design intent) to have used the UIO_MEM_PHYS type
	 * for our two memory regions, and to rely on UIO's own mmap() handler
	 * (by not declaring our own). Unfortunately UIO does not allow any
	 * specification of pgprots and assumes cache-inhibited mappings for
	 * anything physical (see drivers/uio/uio.c, eg. uio_mmap_physical()).
	 * So UIO could use a couple of improvements as it is not saving us much
	 * on the kernel nor the user side. The first would be to use PFN
	 * instead of a raw base address in the uio_mem structs (same reason as
	 * everywhere else, this covers 4096 times as much address space, and
	 * why waste lower bits given it has to be page-aligned anyway?). The
	 * second is to add a pgprot field to uio_mem to be used with _PHYS
	 * mappings. (Or use a new _PHYS_PGPROT type, for backwards
	 * compatibility?) */
	/* Normally, we'd ">>PAGE_SHIFT" the mem->addr value here, but due to
	 * the 36-bit issue, it is already stored as a PFN. */
	return io_remap_pfn_range(vma, vma->vm_start, mem->addr, mem->size,
				vma->vm_page_prot);
}

static irqreturn_t dpa_uio_irq_handler(int irq, struct uio_info *info)
{
	struct dpa_uio_info *i = container_of(info, struct dpa_uio_info, uio);
	/* This is the only code outside the regular portal driver that
	 * manipulates any portal register, so rather than breaking that
	 * encapsulation I am simply hard-coding the offset to the inhibit
	 * register here. */
	out_be32(i->addr_ci + 0xe0c, ~(u32)0);
	return IRQ_HANDLED;
}

static void __init dpa_uio_portal_init(struct dpa_uio_portal *p,
				const struct dpa_uio_class *c)
{
	struct dpa_uio_info *info;
	const struct resource *res;
	u32 index;
	int irq, ret;

	/* allocate 'info' */
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return;
	atomic_set(&info->ref, 1);
	if (p->type == dpa_uio_portal_bman) {
		res = &p->bm_cfg->addr_phys[0];
		index = p->bm_cfg->public_cfg.index;
		irq = p->bm_cfg->public_cfg.irq;
	} else {
		res = &p->qm_cfg->addr_phys[0];
		index = p->qm_cfg->public_cfg.index;
		irq = p->qm_cfg->public_cfg.irq;
	}
	/* We need to map the cache-inhibited region in the kernel for
	 * interrupt-handling purposes. */
	info->addr_ci = ioremap_prot(res[BM_ADDR_CI].start,
				resource_size(&res[BM_ADDR_CI]),
				_PAGE_GUARDED | _PAGE_NO_CACHE);
	/* Name the UIO device according to the cell-index. It's supposed to be
	 * unique for each device class (Qman/Bman), and is also a convenient
	 * way for user-space to find the UIO device that corresponds to a given
	 * portal device-tree node. */
	sprintf(info->name, "%s%x", c->dev_prefix, index);
	info->pdev = platform_device_alloc(info->name, -1);
	if (!info->pdev) {
		iounmap(info->addr_ci);
		kfree(info);
		pr_err("dpa_uio_portal: platform_device_alloc() failed\n");
		return;
	}
	ret = platform_device_add(info->pdev);
	if (ret) {
		platform_device_put(info->pdev);
		iounmap(info->addr_ci);
		kfree(info);
		pr_err("dpa_uio_portal: platform_device_add() failed\n");
		return;
	}
	info->uio.name = info->name;
	info->uio.version = dpa_uio_version;
	/* Work around the 36-bit UIO issue by bit-shifting the addresses */
	info->uio.mem[BM_ADDR_CE].name = "cena";
	info->uio.mem[BM_ADDR_CE].addr = res[BM_ADDR_CE].start >> PAGE_SHIFT;
	info->uio.mem[BM_ADDR_CE].size = resource_size(&res[BM_ADDR_CE]);
	info->uio.mem[BM_ADDR_CI].name = "cinh";
	info->uio.mem[BM_ADDR_CI].addr = res[BM_ADDR_CI].start >> PAGE_SHIFT;
	info->uio.mem[BM_ADDR_CI].size = resource_size(&res[BM_ADDR_CI]);
	info->uio.irq = irq;
	info->uio.handler = dpa_uio_irq_handler;
	info->uio.mmap = dpa_uio_mmap;
	info->uio.open = dpa_uio_open;
	info->uio.release = dpa_uio_release;
	ret = uio_register_device(&info->pdev->dev, &info->uio);
	if (ret) {
		platform_device_del(info->pdev);
		platform_device_put(info->pdev);
		iounmap(info->addr_ci);
		kfree(info);
		pr_err("dpa_uio_portal: UIO registration failed\n");
		return;
	}
	list_add_tail(&info->node, &uio_portal_list);
	pr_info("USDPAA portal initialised, %s\n", info->name);
}

static int __init dpa_uio_init(void)
{
	const struct dpa_uio_class *classes[3], **c = classes;
	classes[0] = dpa_uio_bman();
	classes[1] = dpa_uio_qman();
	classes[2] = NULL;
	while (*c) {
		struct dpa_uio_portal *p;
		list_for_each_entry(p, &(*c)->list, node)
			dpa_uio_portal_init(p, *c);
		c++;
	}
	pr_info("USDPAA portal layer loaded\n");
	return 0;
}

static void __exit dpa_uio_exit(void)
{
	struct dpa_uio_info *info, *tmp;
	list_for_each_entry_safe(info, tmp, &uio_portal_list, node) {
		list_del(&info->node);
		uio_unregister_device(&info->uio);
		platform_device_del(info->pdev);
		platform_device_put(info->pdev);
		iounmap(info->addr_ci);
		pr_info("USDPAA portal removed, %s\n", info->name);
		kfree(info);
	}
	pr_info("USDPAA portal layer unloaded\n");
}


module_init(dpa_uio_init)
module_exit(dpa_uio_exit)
MODULE_LICENSE("GPL");

