/*
 * XGene MSI Driver
 *
 * Copyright (c) 2010, Applied Micro Circuits Corporation
 * Author: Tanmay Inamdar <tinamdar@apm.com>
 *         Tuan Phan <tphan@apm.com>
 *         Jim Hull <jim.hull@hp.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/bootmem.h>
#include <linux/msi.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/acpi.h>
#include <linux/efi.h>
#include <asm/hw_irq.h>
#include <asm/io.h>
#include <asm/msi_bitmap.h>

#define NR_MSI_REG		16
#define IRQS_PER_MSI_INDEX	32
#define IRQS_PER_MSI_REG	256
#define NR_MSI_IRQS		(NR_MSI_REG * IRQS_PER_MSI_REG)

#define XGENE_PIC_IP_MASK	0x0000000F
#define XGENE_PIC_IP_GIC	0x00000001

/* PCIe MSI Index Registers */
#define MSI0IR0			0x000000
#define MSIFIR7			0x7F0000

/* PCIe MSI Interrupt Register */
#define MSI1INT0		0x800000
#define MSI1INTF		0x8F0000

struct xgene_msi {
	struct			irq_domain *irqhost;
	unsigned long		cascade_irq;
	u32			msiir_offset;
	u32			msi_addr_lo;
	u32			msi_addr_hi;
	void __iomem		*msi_regs;
	u32			feature;
	int			msi_virqs[NR_MSI_REG];
	struct			msi_bitmap bitmap;
	struct list_head 	list; /* support multiple MSI banks */
	phandle 		phandle;
};

#ifdef CONFIG_ARCH_MSLIM
static inline u64 xgene_pcie_get_iof_addr(u64 addr)
{
	return mslim_pa_to_iof_axi(lower_32_bits(addr));
}
#else
#define xgene_pcie_get_iof_addr(addr)	addr
#endif

#define MSI_DRIVER_VERSION "0.1"

struct xgene_msi_feature {
	u32 xgene_pic_ip;
	u32 msiir_offset; /* Offset of MSIIR, relative to start of MSIR bank */
};

struct xgene_msi_cascade_data {
	struct xgene_msi *msi_data;
	int index;
};

LIST_HEAD(msi_head);

static const struct xgene_msi_feature gic_msi_feature = {
	.xgene_pic_ip = XGENE_PIC_IP_GIC,
	.msiir_offset = 0,
};

irq_hw_number_t virq_to_hw(unsigned int virq)
{
	struct irq_data *irq_data = irq_get_irq_data(virq);
	return WARN_ON(!irq_data) ? 0 : irq_data->hwirq;
} 

static inline u32 xgene_msi_intr_read(phys_addr_t __iomem *base, 
				      unsigned int reg)
{
	u32 irq_reg = MSI1INT0 + (reg << 16);
	return readl((void *)((phys_addr_t) base + irq_reg));
}

static inline u32 xgene_msi_read(phys_addr_t __iomem *base, unsigned int group,
			         unsigned int reg)
{
	u32 irq_reg = MSI0IR0 + (group << 19) + (reg << 16);
	return readl((void *)((phys_addr_t) base + irq_reg));
}

/*
 * We do not need this actually. The MSIR register has been read once
 * in the cascade interrupt. So, this MSI interrupt has been acked
*/
static void xgene_msi_end_irq(struct irq_data *d)
{
}

#ifdef CONFIG_SMP
static int xgene_msi_set_affinity(struct irq_data *d,
				const struct cpumask *mask_val, bool force)
{
	u64 virt_msir;

	virt_msir = (u64)irq_get_handler_data(d->irq);
	return irq_set_affinity(virt_msir, mask_val);
}
#endif

static struct irq_chip xgene_msi_chip = {
	.irq_mask	= mask_msi_irq,
	.irq_unmask	= unmask_msi_irq,
	.irq_ack	= xgene_msi_end_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity = xgene_msi_set_affinity,
#endif
	.name		= "xgene-msi",
};

static int xgene_msi_host_map(struct irq_domain *h, unsigned int virq,
			      irq_hw_number_t hw)
{
	struct xgene_msi *msi_data = h->host_data;
	struct irq_chip *chip = &xgene_msi_chip;

	pr_debug("\nENTER %s, virq=%u\n", __func__, virq);
	irq_set_status_flags(virq, IRQ_TYPE_LEVEL_HIGH);
	irq_set_chip_data(virq, msi_data);
	irq_set_chip_and_handler(virq, chip, handle_level_irq);
	
	return 0;
}

static const struct irq_domain_ops xgene_msi_host_ops = {
	.map = xgene_msi_host_map,
};

static int xgene_msi_init_allocator(struct xgene_msi *msi_data)
{
#ifdef CONFIG_ACPI
	if (efi_enabled(EFI_BOOT))
		return msi_bitmap_alloc(&msi_data->bitmap, NR_MSI_IRQS, NULL);
	else
#endif
		return msi_bitmap_alloc(&msi_data->bitmap, NR_MSI_IRQS,
			      msi_data->irqhost->of_node);
}

int arch_msi_check_device(struct pci_dev *dev, int nvec, int type)
{
	pr_debug("ENTER %s\n", __func__);
	return 0;
}

void arch_teardown_msi_irqs(struct pci_dev *dev)
{
	struct msi_desc *entry;
	struct xgene_msi *msi_data;
	
	pr_debug("ENTER %s\n", __func__);
	list_for_each_entry(entry, &dev->msi_list, list) {
		if (entry->irq == 0)
			continue;

		msi_data = irq_get_chip_data(entry->irq);
		irq_set_msi_desc(entry->irq, NULL);
		msi_bitmap_free_hwirqs(&msi_data->bitmap,
				       virq_to_hw(entry->irq), 1);
		irq_dispose_mapping(entry->irq);
	}
}

static void xgene_compose_msi_msg(struct pci_dev *dev, int hwirq,
				  struct msi_msg *msg,
				  struct xgene_msi *msi_data)
{
	int reg_set, group;

	group = hwirq % NR_MSI_REG;
	reg_set = hwirq / (NR_MSI_REG * IRQS_PER_MSI_INDEX);

	pr_debug("group = %d, reg_set : 0x%x\n", group, reg_set);
	msg->address_lo = msi_data->msi_addr_lo +
			  (((8 * group) + reg_set) << 16);
	msg->address_hi = msi_data->msi_addr_hi;
	msg->data = (hwirq / NR_MSI_REG) % IRQS_PER_MSI_INDEX;
	pr_debug("addr : 0x%08x, data : 0x%x\n", msg->address_lo, msg->data);
}

int arch_setup_msi_irqs(struct pci_dev *pdev, int nvec, int type)
{
	struct device_node *np;
	struct msi_desc *entry;
	struct msi_msg msg;
	u64 gic_irq;
	unsigned int virq;
	struct xgene_msi *msi_data;
	phandle phandle = 0;
	int rc = 0;
	int hwirq = -1;

	pr_debug("ENTER %s - nvec = %d, type = %d\n", __func__, nvec, type);
#ifdef CONFIG_ACPI
	if (!efi_enabled(EFI_BOOT))
#endif
	{
		np = pci_device_to_OF_node(pdev);
		/*
		 * If the PCI node has an xgene,msi property, 
		 * then we need to use it to find the specific MSI.
		 */
		np = of_parse_phandle(np, "xgene,msi", 0);
		if (np) {
			if (of_device_is_compatible(np, 
						   "xgene,gic-msi-cascade")) 
				phandle = np->phandle;
			else {
				dev_err(&pdev->dev,
				"node %s has an invalid xgene,msi phandle %u\n",
					 np->full_name, np->phandle);
				rc = -EINVAL;
				goto exit;
			}
		} else
			dev_info(&pdev->dev, "Found no xgene,msi phandle\n");
	}
	
	list_for_each_entry(entry, &pdev->msi_list, list) {
		pr_debug("Loop over MSI devices\n");
		/*
		 * Loop over all the MSI devices until we find one that has an
		 * available interrupt.
		 */
		list_for_each_entry(msi_data, &msi_head, list) {
			if (phandle && (phandle != msi_data->phandle))
				continue;
			pr_debug("Xgene msi pointer : %p\n", msi_data);
			hwirq = msi_bitmap_alloc_hwirqs(&msi_data->bitmap, 1);
			break;
		}

		if (hwirq < 0) {
			dev_err(&pdev->dev, 
				 "could not allocate MSI interrupt\n");
			rc = -ENOSPC;
			goto exit;
		}

		virq = irq_create_mapping(msi_data->irqhost, hwirq);
		if (virq == 0) {
			dev_err(&pdev->dev, "fail mapping hwirq %i\n", hwirq);
			msi_bitmap_free_hwirqs(&msi_data->bitmap, hwirq, 1);
			rc = -ENOSPC;
			goto exit;
		}

		gic_irq = msi_data->msi_virqs[hwirq % NR_MSI_REG];
		pr_debug("Created Mapping HWIRQ %d on GIC IRQ %llu "
			 "TO VIRQ %d\n",
			 hwirq, gic_irq, virq);
		/* chip_data is msi_data via host->hostdata in host->map() */
		irq_set_msi_desc(virq, entry);
		xgene_compose_msi_msg(pdev, hwirq, &msg, msi_data);
		irq_set_handler_data(virq, (void *)gic_irq);
		write_msi_msg(virq, &msg);
	}

exit:
	return rc;
}

static void xgene_msi_cascade(unsigned int irq, struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct xgene_msi_cascade_data *cascade_data;
	struct xgene_msi *msi_data;
	unsigned int cascade_irq;
	int msir_index = -1;
	u32 msir_value = 0;
	u32 intr_index = 0;
	u32 msi_intr_reg_value = 0;
	u32 msi_intr_reg;

	chained_irq_enter(chip, desc);
	cascade_data = irq_get_handler_data(irq);
	msi_data = cascade_data->msi_data;

	msi_intr_reg = cascade_data->index;

	if (msi_intr_reg >= NR_MSI_REG)
		cascade_irq = 0;

	switch (msi_data->feature & XGENE_PIC_IP_MASK) {
	case XGENE_PIC_IP_GIC:
		msi_intr_reg_value = xgene_msi_intr_read(msi_data->msi_regs,
						       msi_intr_reg);
		break;
	}

	while (msi_intr_reg_value) {
		msir_index = ffs(msi_intr_reg_value) - 1;
		msir_value = xgene_msi_read(msi_data->msi_regs, msi_intr_reg,
					  msir_index);
		while (msir_value) {
			intr_index = ffs(msir_value) - 1;
			cascade_irq = irq_linear_revmap(msi_data->irqhost,
				 msir_index * IRQS_PER_MSI_INDEX * NR_MSI_REG +
				 intr_index * NR_MSI_REG + msi_intr_reg);
			if (cascade_irq != 0)
				generic_handle_irq(cascade_irq);
			msir_value &= ~(1 << intr_index);
		}
		msi_intr_reg_value &= ~(1 << msir_index);
	}

	chained_irq_exit(chip, desc);
}

static int xgene_msi_remove(struct platform_device *pdev)
{
	int virq, i;
	struct xgene_msi *msi = platform_get_drvdata(pdev);

	pr_debug("ENTER %s\n", __func__);
	for (i = 0; i < NR_MSI_REG; i++) {
		virq = msi->msi_virqs[i];
		if (virq != 0)
			irq_dispose_mapping(virq);
	}

	if (msi->bitmap.bitmap)
		msi_bitmap_free(&msi->bitmap);

	return 0;
}

static int xgene_msi_setup_hwirq(struct xgene_msi *msi,
				 struct platform_device *pdev,
				 int offset, int irq_index)
{
	int virt_msir;
	cpumask_var_t mask;
	struct xgene_msi_cascade_data *cascade_data = NULL;

	virt_msir = platform_get_irq(pdev, irq_index);
	if (virt_msir < 0) {
		dev_err(&pdev->dev, "Cannot translate IRQ index %d\n",
			irq_index);
		return -EINVAL;
	}

	cascade_data = devm_kzalloc(&pdev->dev, 
			sizeof(struct xgene_msi_cascade_data), GFP_KERNEL);
	if (!cascade_data) {
		dev_err(&pdev->dev, "No memory for MSI cascade data\n");
		return -ENOMEM;
	}

	if (alloc_cpumask_var(&mask, GFP_KERNEL)) {
		cpumask_setall(mask);
		irq_set_affinity(virt_msir, mask);
		free_cpumask_var(mask);
	}

	msi->msi_virqs[irq_index] = virt_msir;
	cascade_data->index = offset;
	cascade_data->msi_data = msi;
	irq_set_handler_data(virt_msir, cascade_data);
	irq_set_chained_handler(virt_msir, xgene_msi_cascade);
	pr_debug("mapped phys irq %d\n", virt_msir);

	return 0;
}

static int xgene_msi_get_param(struct platform_device *pdev, const char *name,
			       u32 *buf, int count)
{
	int rc = 0;

#ifdef CONFIG_ACPI
	if (efi_enabled(EFI_BOOT)) {
		struct acpi_dsm_entry entry;

		if (acpi_dsm_lookup_value(ACPI_HANDLE(&pdev->dev),
					  name, 0, &entry) || !entry.value)
			return -EINVAL;

		if (count == 2)
			sscanf(entry.value, "%d %d", &buf[0], &buf[1]);
		else
			rc = -EINVAL;

		kfree(entry.key);
		kfree(entry.value);
	} else
#endif
		if (of_property_read_u32_array(pdev->dev.of_node, name,
					        buf, count))
			rc = -EINVAL;

	return rc;
}

static int xgene_msi_probe(struct platform_device *pdev)
{
	struct xgene_msi *msi;
	struct resource *res;
	phys_addr_t msiir_offset;
	int rc, j, irq_index, count;
	u32 offset;
	u32 buf[] = { 0, NR_MSI_IRQS};

	msi = devm_kzalloc(&pdev->dev, sizeof(struct xgene_msi), GFP_KERNEL);
	if (!msi) {
		dev_err(&pdev->dev, "No memory for MSI structure\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, msi);

#ifdef CONFIG_ACPI
	if (efi_enabled(EFI_BOOT))
		msi->irqhost = irq_domain_add_linear(NULL,
				      NR_MSI_IRQS, &xgene_msi_host_ops, msi);
	else
#endif
		msi->irqhost = irq_domain_add_linear(pdev->dev.of_node,
				      NR_MSI_IRQS, &xgene_msi_host_ops, msi);
	if (msi->irqhost == NULL) {
		dev_err(&pdev->dev, "No memory for MSI irqhost\n");
		rc = -ENOMEM;
		goto error;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	msi->msi_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(msi->msi_regs)) {
		dev_err(&pdev->dev, "no reg space\n");
		rc = -EINVAL;
		goto error;
	}

	pr_debug("mapped 0x%08llx to 0x%p Size : 0x%08llx\n", res->start,
		 msi->msi_regs, resource_size(res));

	msiir_offset = lower_32_bits(xgene_pcie_get_iof_addr(res->start));
	msi->msiir_offset = gic_msi_feature.msiir_offset +
				  (msiir_offset & 0xfffff);
	msi->msi_addr_hi = upper_32_bits(res->start);
	msi->msi_addr_lo = gic_msi_feature.msiir_offset +
				  (msiir_offset & 0xffffffff);
	msi->feature = gic_msi_feature.xgene_pic_ip;

#ifdef CONFIG_ACPI
	if (efi_enabled(EFI_BOOT))
		msi->phandle = 0;
	else
#endif
		msi->phandle = pdev->dev.of_node->phandle;

	rc = xgene_msi_init_allocator(msi);
	if (rc) {
		dev_err(&pdev->dev, "Error allocating MSI bitmap\n");
		goto error;
	}

	rc = xgene_msi_get_param(pdev, "msi-available-ranges", buf, 2);
	if (rc) {
		dev_err(&pdev->dev, "Error getting MSI ranges\n");
		goto error;
	}

	pr_debug("buf[0] = 0x%x buf[1] = 0x%x\n", buf[0], buf[1]);
	if (buf[0] % IRQS_PER_MSI_REG || buf[1] % IRQS_PER_MSI_REG) {
		pr_err_once("msi available range of"
				  "%u at %u is not IRQ-aligned\n",
				  buf[1], buf[0]);
		rc = -EINVAL;
		goto error;
	}

	offset = buf[0] / IRQS_PER_MSI_REG;
	count  = buf[1] / IRQS_PER_MSI_REG;
	pr_debug("offset = %d count = %d\n", offset, count);

	for (irq_index = 0, j = 0; j < count; j++, irq_index++) {
		rc = xgene_msi_setup_hwirq(msi, pdev, offset + j, irq_index);
		if (rc)
			goto error;
	}

	list_add_tail(&msi->list, &msi_head);

	pr_info("XGene: PCIe MSI driver v%s\n", MSI_DRIVER_VERSION);

	return 0;

error:
	xgene_msi_remove(pdev);
	return rc;
}

static const struct of_device_id xgene_msi_of_ids[] = {
	{
		.compatible = "xgene,gic-msi",
	},
	{}
};

#ifdef CONFIG_ACPI
static const struct acpi_device_id xgene_msi_acpi_ids[] = {
	{"APMC0D0E", 0},
	{},
};
#endif

static struct platform_driver xgene_msi_driver = {
	.driver = {
		.name = "xgene-msi",
		.owner = THIS_MODULE,
		.of_match_table = xgene_msi_of_ids,
#ifdef CONFIG_ACPI		
		.acpi_match_table = ACPI_PTR(xgene_msi_acpi_ids),
#endif
	},
	.probe = xgene_msi_probe,
	.remove = xgene_msi_remove,
};

static __init int xgene_msi_init(void)
{
	return platform_driver_register(&xgene_msi_driver);
}

subsys_initcall_sync(xgene_msi_init);
