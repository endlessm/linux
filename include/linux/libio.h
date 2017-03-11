/*
 * Copyright (C) 2017 Hisilicon Limited, All Rights Reserved.
 * Author: Zhichang Yuan <yuanzhichang@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LINUX_LIBIO_H
#define __LINUX_LIBIO_H

#ifdef __KERNEL__

#include <linux/device.h>
#include <linux/fwnode.h>

/* This is compatible to PCI MMIO. */
#define IO_CPU_MMIO		0x01
/* All hosts where there are no CPU addr */
#define IO_HOST_INDIRECT	0x02

struct libio_range {
	struct list_head list;
	struct fwnode_handle *node;
	resource_size_t size; /* range size populated */
	resource_size_t io_start;	/* logical pio start. inclusive */
	resource_size_t hw_start;
	unsigned long flags;
	void *devpara;	/* private parameter of the host device */
	struct libio_ops *ops;	/* ops operating on this node */
};

struct libio_ops {
	u32 (*pfin)(void *devobj, unsigned long ptaddr,	size_t dlen);
	void (*pfout)(void *devobj, unsigned long ptaddr, u32 outval,
			size_t dlen);
	u32 (*pfins)(void *devobj, unsigned long ptaddr, void *inbuf,
			size_t dlen, unsigned int count);
	void (*pfouts)(void *devobj, unsigned long ptaddr,
			const void *outbuf, size_t dlen, unsigned int count);
};

extern u8 libio_inb(unsigned long addr);
extern void libio_outb(u8 value, unsigned long addr);
extern void libio_outw(u16 value, unsigned long addr);
extern void libio_outl(u32 value, unsigned long addr);
extern u16 libio_inw(unsigned long addr);
extern u32 libio_inl(unsigned long addr);
extern void libio_outb(u8 value, unsigned long addr);
extern void libio_outw(u16 value, unsigned long addr);
extern void libio_outl(u32 value, unsigned long addr);
extern void libio_insb(unsigned long addr, void *buffer, unsigned int count);
extern void libio_insl(unsigned long addr, void *buffer, unsigned int count);
extern void libio_insw(unsigned long addr, void *buffer, unsigned int count);
extern void libio_outsb(unsigned long addr, const void *buffer,
			unsigned int count);
extern void libio_outsw(unsigned long addr, const void *buffer,
			unsigned int count);
extern void libio_outsl(unsigned long addr, const void *buffer,
			unsigned int count);
#ifdef CONFIG_LIBIO
extern struct libio_range
*find_io_range_from_fwnode(struct fwnode_handle *fwnode);
extern unsigned long libio_translate_hwaddr(struct fwnode_handle *fwnode,
			resource_size_t hw_addr);
extern struct libio_range *register_libio_range(struct libio_range *newrange);
extern resource_size_t libio_to_hwaddr(unsigned long pio);

extern unsigned long libio_translate_cpuaddr(resource_size_t hw_addr);
#else
static inline struct libio_range
*find_io_range_from_fwnode(struct fwnode_handle *fwnode)
{
	return NULL;
}

static inline unsigned long libio_translate_hwaddr(struct fwnode_handle *fwnode,
			resource_size_t hw_addr)
{
	return -1;
}

static inline struct libio_range 
*register_libio_range(struct libio_range *newrange)
{
	return NULL;
}

static inline resource_size_t libio_to_hwaddr(unsigned long pio)
{
	return -1;
}

static inline unsigned long libio_translate_cpuaddr(resource_size_t hw_addr)
{
	return -1;
}
#endif

#ifdef CONFIG_ACPI
extern int acpi_set_libio_resource(struct device *child,
		struct device *hostdev);
#else
static inline int acpi_set_libio_resource(struct device *child,
		struct device *hostdev)
{
	return -EFAULT;
}
#endif /* CONFIG_ACPI */
#endif /* __KERNEL__ */
#endif /* __LINUX_LIBIO_H */
