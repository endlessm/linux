// SPDX-License-Identifier: GPL-2.0
/*
 * Intel remapped NVMe device support.
 *
 * Copyright (c) 2019 Endless Mobile, Inc.
 * Author: Daniel Drake <drake@endlessm.com>
 *
 * Some products ship by default with the SATA controller in "RAID" or
 * "Intel RST Premium With Intel Optane System Acceleration" mode. Under this
 * mode, which we refer to as "remapped NVMe" mode, any installed NVMe
 * devices disappear from the PCI bus, and instead their I/O memory becomes
 * available within the AHCI device BARs.
 *
 * This scheme is understood to be a way of avoiding usage of the standard
 * Windows NVMe driver under that OS, instead mandating usage of Intel's
 * driver instead, which has better power management, and presumably offers
 * some RAID/disk-caching solutions too.
 *
 * Here in this driver, we support the remapped NVMe mode by claiming the
 * AHCI device and creating a fake PCIe root port. On the new bus, the
 * original AHCI device is exposed with only minor tweaks. Then, fake PCI
 * devices corresponding to the remapped NVMe devices are created. The usual
 * ahci and nvme drivers are then expected to bind to these devices and
 * operate as normal.
 *
 * The PCI configuration space for the NVMe devices is completely
 * unavailable, so we fake a minimal one and hope for the best.
 *
 * Interrupts are shared between the AHCI and NVMe devices. For simplicity,
 * we only support the legacy interrupt here, although MSI support
 * could potentially be added later.
 */

#define MODULE_NAME "intel-nvme-remap"

#include <linux/ahci-remap.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#define AHCI_PCI_BAR_STANDARD 5

struct nvme_remap_dev {
	struct pci_dev		*dev;		/* AHCI device */
	struct pci_bus		*bus;		/* our fake PCI bus */
	struct pci_sysdata	sysdata;
	int			irq_base;	/* our fake interrupts */

	/*
	 * When we detect an all-ones write to a BAR register, this flag
	 * is set, so that we return the BAR size on the next read (a
	 * standard PCI behaviour).
	 * This includes the assumption that an all-ones BAR write is
	 * immediately followed by a read of the same register.
	 */
	bool			bar_sizing;

	/*
	 * Resources copied from the AHCI device, to be regarded as
	 * resources on our fake bus.
	 */
	struct resource		ahci_resources[PCI_NUM_RESOURCES];

	/* Resources corresponding to the NVMe devices. */
	struct resource		remapped_dev_mem[AHCI_MAX_REMAP];

	/* Number of remapped NVMe devices found. */
	int			num_remapped_devices;
};

static inline struct nvme_remap_dev *nrdev_from_bus(struct pci_bus *bus)
{
	return container_of(bus->sysdata, struct nvme_remap_dev, sysdata);
}


/******** PCI configuration space **********/

/*
 * Helper macros for tweaking returned contents of PCI configuration space.
 *
 * value contains len bytes of data read from reg.
 * If fixup_reg is included in that range, fix up the contents of that
 * register to fixed_value.
 */
#define NR_FIX8(fixup_reg, fixed_value) do { \
		if (reg <= fixup_reg && fixup_reg < reg + len) \
			((u8 *) value)[fixup_reg - reg] = (u8) (fixed_value); \
	} while (0)

#define NR_FIX16(fixup_reg, fixed_value) do { \
		NR_FIX8(fixup_reg, fixed_value); \
		NR_FIX8(fixup_reg + 1, fixed_value >> 8); \
	} while (0)

#define NR_FIX24(fixup_reg, fixed_value) do { \
		NR_FIX8(fixup_reg, fixed_value); \
		NR_FIX8(fixup_reg + 1, fixed_value >> 8); \
		NR_FIX8(fixup_reg + 2, fixed_value >> 16); \
	} while (0)

#define NR_FIX32(fixup_reg, fixed_value) do { \
		NR_FIX16(fixup_reg, (u16) fixed_value); \
		NR_FIX16(fixup_reg + 2, fixed_value >> 16); \
	} while (0)

/*
 * Read PCI config space of the slot 0 (AHCI) device.
 * We pass through the read request to the underlying device, but
 * tweak the results in some cases.
 */
static int nvme_remap_pci_read_slot0(struct pci_bus *bus, int reg,
				     int len, u32 *value)
{
	struct nvme_remap_dev *nrdev = nrdev_from_bus(bus);
	struct pci_bus *ahci_dev_bus = nrdev->dev->bus;
	int ret;

	ret = ahci_dev_bus->ops->read(ahci_dev_bus, nrdev->dev->devfn,
				      reg, len, value);
	if (ret)
		return ret;

	/*
	 * Adjust the device class, to prevent this driver from attempting to
	 * additionally probe the device we're simulating here.
	 */
	NR_FIX24(PCI_CLASS_PROG, PCI_CLASS_STORAGE_SATA_AHCI);

	/*
	 * Unset interrupt pin, otherwise ACPI tries to find routing
	 * info for our virtual IRQ, fails, and complains.
	 */
	NR_FIX8(PCI_INTERRUPT_PIN, 0);

	/*
	 * Truncate the AHCI BAR to not include the region that covers the
	 * hidden devices. This will cause the ahci driver to successfully
	 * probe th new device (instead of handing it over to this driver).
	 */
	if (nrdev->bar_sizing) {
		NR_FIX32(PCI_BASE_ADDRESS_5, ~(SZ_16K - 1));
		nrdev->bar_sizing = false;
	}

	return PCIBIOS_SUCCESSFUL;
}

/*
 * Read PCI config space of a remapped device.
 * Since the original PCI config space is inaccessible, we provide a minimal,
 * fake config space instead.
 */
static int nvme_remap_pci_read_remapped(struct pci_bus *bus, unsigned int port,
					int reg, int len, u32 *value)
{
	struct nvme_remap_dev *nrdev = nrdev_from_bus(bus);
	struct resource *remapped_mem;

	if (port > nrdev->num_remapped_devices)
		return PCIBIOS_DEVICE_NOT_FOUND;

	*value = 0;
	remapped_mem = &nrdev->remapped_dev_mem[port - 1];

	/* Set a Vendor ID, otherwise Linux assumes no device is present */
	NR_FIX16(PCI_VENDOR_ID, PCI_VENDOR_ID_INTEL);

	/* Always appear on & bus mastering */
	NR_FIX16(PCI_COMMAND, PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	/* Set class so that nvme driver probes us */
	NR_FIX24(PCI_CLASS_PROG, PCI_CLASS_STORAGE_EXPRESS);

	if (nrdev->bar_sizing) {
		NR_FIX32(PCI_BASE_ADDRESS_0,
			 ~(resource_size(remapped_mem) - 1));
		nrdev->bar_sizing = false;
	} else {
		resource_size_t mem_start = remapped_mem->start;

		mem_start |= PCI_BASE_ADDRESS_MEM_TYPE_64;
		NR_FIX32(PCI_BASE_ADDRESS_0, mem_start);
		mem_start >>= 32;
		NR_FIX32(PCI_BASE_ADDRESS_1, mem_start);
	}

	return PCIBIOS_SUCCESSFUL;
}

/* Read PCI configuration space. */
static int nvme_remap_pci_read(struct pci_bus *bus, unsigned int devfn,
			       int reg, int len, u32 *value)
{
	if (PCI_SLOT(devfn) == 0)
		return nvme_remap_pci_read_slot0(bus, reg, len, value);
	else
		return nvme_remap_pci_read_remapped(bus, PCI_SLOT(devfn),
						    reg, len, value);
}

/*
 * Write PCI config space of the slot 0 (AHCI) device.
 * Apart from the special case of BAR sizing, we disable all writes.
 * Otherwise, the ahci driver could make changes (e.g. unset PCI bus master)
 * that would affect the operation of the NVMe devices.
 */
static int nvme_remap_pci_write_slot0(struct pci_bus *bus, int reg,
				      int len, u32 value)
{
	struct nvme_remap_dev *nrdev = nrdev_from_bus(bus);
	struct pci_bus *ahci_dev_bus = nrdev->dev->bus;

	if (reg >= PCI_BASE_ADDRESS_0 && reg <= PCI_BASE_ADDRESS_5) {
		/*
		 * Writing all-ones to a BAR means that the size of the
		 * memory region is being checked. Flag this so that we can
		 * reply with an appropriate size on the next read.
		 */
		if (value == ~0)
			nrdev->bar_sizing = true;

		return ahci_dev_bus->ops->write(ahci_dev_bus,
						nrdev->dev->devfn,
						reg, len, value);
	}

	return PCIBIOS_SET_FAILED;
}

/*
 * Write PCI config space of a remapped device.
 * Since the original PCI config space is inaccessible, we reject all
 * writes, except for the special case of BAR probing.
 */
static int nvme_remap_pci_write_remapped(struct pci_bus *bus,
					 unsigned int port,
					 int reg, int len, u32 value)
{
	struct nvme_remap_dev *nrdev = nrdev_from_bus(bus);

	if (port > nrdev->num_remapped_devices)
		return PCIBIOS_DEVICE_NOT_FOUND;

	/*
	 * Writing all-ones to a BAR means that the size of the memory
	 * region is being checked. Flag this so that we can reply with
	 * an appropriate size on the next read.
	 */
	if (value == ~0 && reg >= PCI_BASE_ADDRESS_0
			&& reg <= PCI_BASE_ADDRESS_5) {
		nrdev->bar_sizing = true;
		return PCIBIOS_SUCCESSFUL;
	}

	return PCIBIOS_SET_FAILED;
}

/* Write PCI configuration space. */
static int nvme_remap_pci_write(struct pci_bus *bus, unsigned int devfn,
				int reg, int len, u32 value)
{
	if (PCI_SLOT(devfn) == 0)
		return nvme_remap_pci_write_slot0(bus, reg, len, value);
	else
		return nvme_remap_pci_write_remapped(bus, PCI_SLOT(devfn),
						     reg, len, value);
}

static struct pci_ops nvme_remap_pci_ops = {
	.read	= nvme_remap_pci_read,
	.write	= nvme_remap_pci_write,
};


/******** Initialization & exit **********/

/*
 * Find a PCI domain ID to use for our fake bus.
 * Start at 0x10000 to not clash with ACPI _SEG domains (16 bits).
 */
static int find_free_domain(void)
{
	int domain = 0xffff;
	struct pci_bus *bus = NULL;

	while ((bus = pci_find_next_bus(bus)) != NULL)
		domain = max_t(int, domain, pci_domain_nr(bus));

	return domain + 1;
}

static int find_remapped_devices(struct nvme_remap_dev *nrdev,
				 struct list_head *resources)
{
	void __iomem *mmio;
	int i, count = 0;
	u32 cap;

	mmio = pcim_iomap(nrdev->dev, AHCI_PCI_BAR_STANDARD,
			  pci_resource_len(nrdev->dev,
					   AHCI_PCI_BAR_STANDARD));
	if (!mmio)
		return -ENODEV;

	/* Check if this device might have remapped nvme devices. */
	if (pci_resource_len(nrdev->dev, AHCI_PCI_BAR_STANDARD) < SZ_512K ||
	    !(readl(mmio + AHCI_VSCAP) & 1))
		return -ENODEV;

	cap = readq(mmio + AHCI_REMAP_CAP);
	for (i = AHCI_MAX_REMAP-1; i >= 0; i--) {
		struct resource *remapped_mem;

		if ((cap & (1 << i)) == 0)
			continue;
		if (readl(mmio + ahci_remap_dcc(i))
				!= PCI_CLASS_STORAGE_EXPRESS)
			continue;

		/* We've found a remapped device */
		remapped_mem = &nrdev->remapped_dev_mem[count++];
		remapped_mem->start =
			pci_resource_start(nrdev->dev, AHCI_PCI_BAR_STANDARD)
			+ ahci_remap_base(i);
		remapped_mem->end = remapped_mem->start
			+ AHCI_REMAP_N_SIZE - 1;
		remapped_mem->flags = IORESOURCE_MEM | IORESOURCE_PCI_FIXED;
		pci_add_resource(resources, remapped_mem);
	}

	pcim_iounmap(nrdev->dev, mmio);

	if (count == 0)
		return -ENODEV;

	nrdev->num_remapped_devices = count;
	dev_info(&nrdev->dev->dev, "Found %d remapped NVMe devices\n",
		 nrdev->num_remapped_devices);
	return 0;
}

static void nvme_remap_remove_root_bus(void *data)
{
	struct pci_bus *bus = data;

	pci_stop_root_bus(bus);
	pci_remove_root_bus(bus);
}

static int nvme_remap_probe(struct pci_dev *dev,
			    const struct pci_device_id *id)
{
	struct nvme_remap_dev *nrdev;
	LIST_HEAD(resources);
	int i;
	int ret;
	struct pci_dev *child;

	nrdev = devm_kzalloc(&dev->dev, sizeof(*nrdev), GFP_KERNEL);
	nrdev->sysdata.domain = find_free_domain();
	nrdev->sysdata.nvme_remap_dev = dev;
	nrdev->dev = dev;
	pci_set_drvdata(dev, nrdev);

	ret = pcim_enable_device(dev);
	if (ret < 0)
		return ret;

	pci_set_master(dev);

	ret = find_remapped_devices(nrdev, &resources);
	if (ret)
		return ret;

	/* Add resources from the original AHCI device */
	for (i = 0; i < PCI_NUM_RESOURCES; i++) {
		struct resource *res = &dev->resource[i];

		if (res->start) {
			struct resource *nr_res = &nrdev->ahci_resources[i];

			nr_res->start = res->start;
			nr_res->end = res->end;
			nr_res->flags = res->flags;
			pci_add_resource(&resources, nr_res);
		}
	}

	/* Create virtual interrupts */
	nrdev->irq_base = devm_irq_alloc_descs(&dev->dev, -1, 0,
					       nrdev->num_remapped_devices + 1,
					       0);
	if (nrdev->irq_base < 0)
		return nrdev->irq_base;

	/* Create and populate PCI bus */
	nrdev->bus = pci_create_root_bus(&dev->dev, 0, &nvme_remap_pci_ops,
					 &nrdev->sysdata, &resources);
	if (!nrdev->bus)
		return -ENODEV;

	if (devm_add_action_or_reset(&dev->dev, nvme_remap_remove_root_bus,
				     nrdev->bus))
		return -ENOMEM;

	/* We don't support sharing MSI interrupts between these devices */
	nrdev->bus->bus_flags |= PCI_BUS_FLAGS_NO_MSI;

	pci_scan_child_bus(nrdev->bus);

	list_for_each_entry(child, &nrdev->bus->devices, bus_list) {
		/*
		 * Prevent PCI core from trying to move memory BARs around.
		 * The hidden NVMe devices are at fixed locations.
		 */
		for (i = 0; i < PCI_NUM_RESOURCES; i++) {
			struct resource *res = &child->resource[i];

			if (res->flags & IORESOURCE_MEM)
				res->flags |= IORESOURCE_PCI_FIXED;
		}

		/* Share the legacy IRQ between all devices */
		child->irq = dev->irq;
	}

	pci_assign_unassigned_bus_resources(nrdev->bus);
	pci_bus_add_devices(nrdev->bus);

	return 0;
}

static const struct pci_device_id nvme_remap_ids[] = {
	/*
	 * Match all Intel RAID controllers.
	 *
	 * There's overlap here with the set of devices detected by the ahci
	 * driver, but ahci will only successfully probe when there
	 * *aren't* any remapped NVMe devices, and this driver will only
	 * successfully probe when there *are* remapped NVMe devices that
	 * need handling.
	 */
	{
		PCI_VDEVICE(INTEL, PCI_ANY_ID),
		.class = PCI_CLASS_STORAGE_RAID << 8,
		.class_mask = 0xffffff00,
	},
	{0,}
};
MODULE_DEVICE_TABLE(pci, nvme_remap_ids);

static struct pci_driver nvme_remap_drv = {
	.name		= MODULE_NAME,
	.id_table	= nvme_remap_ids,
	.probe		= nvme_remap_probe,
};
module_pci_driver(nvme_remap_drv);

MODULE_AUTHOR("Daniel Drake <drake@endlessm.com>");
MODULE_LICENSE("GPL v2");
