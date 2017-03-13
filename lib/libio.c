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

#include <linux/of.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/rculist.h>
#include <linux/sizes.h>
#include <linux/slab.h>

/* A list of all the IO hosts registered. ONLY THE HOST nodes. */
static LIST_HEAD(io_range_list);
static DEFINE_MUTEX(io_range_mutex);

/*
 * allocate a free range for this registration.
 *
 * @new_range: point to the node awaiting this registration.
 *		part of the fields are as input parameters. This node
 *		is allocated and initialized by caller;
 * @prev: points to the last node before the return;
 *
 * return 0 for success, other are fail.
 */
static int libio_alloc_range(struct libio_range *new_range,
		struct list_head **prev)
{
	struct libio_range *entry;
	unsigned long align = 1;
	unsigned long tmp_start;
	unsigned long idle_start, idle_end;

	if (new_range->flags & IO_CPU_MMIO)
		align = PAGE_SIZE;
	idle_start = 0;
	*prev = &io_range_list;
	list_for_each_entry_rcu(entry, &io_range_list, list) {
		if (idle_start > entry->io_start) {
			WARN(1, "skip an invalid io range during traversal!\n");
			goto nextentry;
		}
		/* set the end edge. */
		if (idle_start == entry->io_start) {
			struct libio_range *next;

			idle_start = entry->io_start + entry->size;
			next = list_next_or_null_rcu(&io_range_list,
				&entry->list, struct libio_range, list);
			if (next) {
				entry = next;
			} else {
				*prev = &entry->list;
				break;
			}
		}
		idle_end = entry->io_start - 1;

		/* contiguous range... */
		if (idle_start > idle_end)
			goto nextentry;

		tmp_start = idle_start;
		idle_start = ALIGN(idle_start, align);
		if (idle_start >= tmp_start &&
			idle_start + new_range->size <= idle_end) {
			new_range->io_start = idle_start;
			*prev = &entry->list;
			return 0;
		}

nextentry:
		idle_start = entry->io_start + entry->size;
		*prev = &entry->list;
	}
	/* check the last free gap... */
	idle_end = IO_SPACE_LIMIT;

	tmp_start = idle_start;
	idle_start = ALIGN(idle_start, align);
	if (idle_start >= tmp_start &&
		idle_start + new_range->size <= idle_end) {
		new_range->io_start = idle_start;
		return 0;
	}

	return -EBUSY;
}

/*
 * traverse the io_range_list to find the registered node whose device node
 * and/or physical IO address match to.
 */
struct libio_range *find_io_range_from_fwnode(struct fwnode_handle *fwnode)
{
	struct libio_range *range;

	list_for_each_entry_rcu(range, &io_range_list, list) {
		if (range->node == fwnode)
			return range;
	}
	return NULL;
}

/*
 * Search a io_range registered which match the fwnode and addr.
 *
 * @fwnode: the host fwnode which must be valid;
 * @start: the start hardware address of this search;
 * @end: the end hardware address of this search. can be equal to @start;
 *
 * return NULL when there is no matched node; IS_ERR() means ERROR;
 * valid virtual address represent a matched node was found.
 */
static struct libio_range *
libio_find_range_byaddr(struct fwnode_handle *fwnode,
			resource_size_t start, resource_size_t end)
{
	struct libio_range *entry;

	list_for_each_entry_rcu(entry, &io_range_list, list) {
		if (entry->node != fwnode)
			continue;
		/* without any overlap with current range */
		if (start >= entry->hw_start + entry->size ||
			end < entry->hw_start)
			continue;
		/* overlap is not supported now. */
		if (start < entry->hw_start ||
			end >= entry->hw_start + entry->size)
			return ERR_PTR(-EBUSY);
		/* had been registered. */
		return entry;
	}

	return NULL;
}

/*
 * register a io range node in the io range list.
 *
 * @newrange: pointer to the io range to be registered.
 *
 * return 'newrange' when success, ERR_VALUE() is for failures.
 * specially, return a valid pointer which is not equal to 'newrange' when
 * the io range had been registered before.
 */
struct libio_range *register_libio_range(struct libio_range *newrange)
{
	int err;
	struct libio_range *range;
	struct list_head *prev;

	if (!newrange || !newrange->node || !newrange->size)
		return ERR_PTR(-EINVAL);

	mutex_lock(&io_range_mutex);
	range = libio_find_range_byaddr(newrange->node, newrange->hw_start,
			newrange->hw_start + newrange->size - 1);
	if (range) {
		if (!IS_ERR(range))
			pr_info("the request IO range had been registered!\n");
		else
			pr_err("registering IO[%pa - sz%pa) got failed!\n",
				&newrange->hw_start, &newrange->size);
		return range;
	}

	err = libio_alloc_range(newrange, &prev);
	if (!err)
		/* the bus IO range list is ordered by pio. */
		list_add_rcu(&newrange->list, prev);
	else
		pr_err("can't find free %pa logical IO range!\n",
			&newrange->size);

	mutex_unlock(&io_range_mutex);
	return err ? ERR_PTR(err) : newrange;
}

/*
 * Translate the input logical pio to the corresponding hardware address.
 * The input pio should be unique in the whole logical PIO space.
 */
resource_size_t libio_to_hwaddr(unsigned long pio)
{
	struct libio_range *range;

	list_for_each_entry_rcu(range, &io_range_list, list) {
		if (pio < range->io_start)
			break;

		if (pio < range->io_start + range->size)
			return pio - range->io_start + range->hw_start;
	}

	return -1;
}

/*
 * This function is generic for translating a hardware address to logical PIO.
 * @hw_addr: the hardware address of host, can be CPU address or host-local
 *		address;
 */
unsigned long
libio_translate_hwaddr(struct fwnode_handle *fwnode, resource_size_t addr)
{
	struct libio_range *range;

	range = libio_find_range_byaddr(fwnode, addr, addr);
	if (!range)
		return -1;

	return addr - range->hw_start + range->io_start;
}

unsigned long
libio_translate_cpuaddr(resource_size_t addr)
{
	struct libio_range *range;

	list_for_each_entry_rcu(range, &io_range_list, list) {
		if (!(range->flags & IO_CPU_MMIO))
			continue;
		if (addr >= range->hw_start &&
			addr < range->hw_start + range->size)
			return addr - range->hw_start + range->io_start;
	}
	return -1;
}

#ifdef PCI_IOBASE
static struct libio_range *find_io_range(unsigned long pio)
{
	struct libio_range *range;

	list_for_each_entry_rcu(range, &io_range_list, list) {
		if (range->io_start > pio)
			return NULL;
		if (pio < range->io_start + range->size)
			return range;
	}
	return NULL;
}

#define BUILD_IO(bw, type)						\
type libio_in##bw(unsigned long addr)					\
{									\
	struct libio_range *entry = find_io_range(addr);		\
									\
	if (entry && entry->ops)					\
		return entry->ops->pfin(entry->devpara,			\
					addr, sizeof(type));		\
	return read##bw(PCI_IOBASE + addr);				\
}									\
									\
void libio_out##bw(type value, unsigned long addr)			\
{									\
	struct libio_range *entry = find_io_range(addr);		\
									\
	if (entry && entry->ops)					\
		entry->ops->pfout(entry->devpara,			\
					addr, value, sizeof(type));	\
	else								\
		write##bw(value, PCI_IOBASE + addr);			\
}									\
									\
void libio_ins##bw(unsigned long addr, void *buffer, unsigned int count)\
{									\
	struct libio_range *entry = find_io_range(addr);		\
									\
	if (entry && entry->ops)					\
		entry->ops->pfins(entry->devpara,			\
				addr, buffer, sizeof(type), count);	\
	else								\
		reads##bw(PCI_IOBASE + addr, buffer, count);		\
}									\
									\
void libio_outs##bw(unsigned long addr, const void *buffer,		\
		    unsigned int count)					\
{									\
	struct libio_range *entry = find_io_range(addr);		\
									\
	if (entry && entry->ops)					\
		entry->ops->pfouts(entry->devpara,			\
				addr, buffer, sizeof(type), count);	\
	else								\
		writes##bw(PCI_IOBASE + addr, buffer, count);	\
}

BUILD_IO(b, u8)

EXPORT_SYMBOL(libio_inb);
EXPORT_SYMBOL(libio_outb);
EXPORT_SYMBOL(libio_insb);
EXPORT_SYMBOL(libio_outsb);

BUILD_IO(w, u16)

EXPORT_SYMBOL(libio_inw);
EXPORT_SYMBOL(libio_outw);
EXPORT_SYMBOL(libio_insw);
EXPORT_SYMBOL(libio_outsw);

BUILD_IO(l, u32)

EXPORT_SYMBOL(libio_inl);
EXPORT_SYMBOL(libio_outl);
EXPORT_SYMBOL(libio_insl);
EXPORT_SYMBOL(libio_outsl);
#endif /* PCI_IOBASE */
