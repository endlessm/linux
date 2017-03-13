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
#include <linux/acpi.h>
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

#ifdef	CONFIG_ACPI
static inline bool acpi_libio_supported_resource(struct acpi_resource *res)
{
	switch (res->type) {
	case ACPI_RESOURCE_TYPE_ADDRESS32:
	case ACPI_RESOURCE_TYPE_ADDRESS64:
		return true;
	}
	return false;
}

static acpi_status acpi_count_libiores(struct acpi_resource *res,
					   void *data)
{
	int *res_cnt = data;

	if (acpi_libio_supported_resource(res) &&
		!acpi_dev_filter_resource_type(res, IORESOURCE_IO))
		(*res_cnt)++;

	return AE_OK;
}

static acpi_status acpi_read_one_libiores(struct acpi_resource *res,
		void *data)
{
	struct acpi_resource **resource = data;

	if (acpi_libio_supported_resource(res) &&
		!acpi_dev_filter_resource_type(res, IORESOURCE_IO)) {
		memcpy((*resource), res, sizeof(struct acpi_resource));
		(*resource)->length = sizeof(struct acpi_resource);
		(*resource)->type = res->type;
		(*resource)++;
	}

	return AE_OK;
}

static acpi_status
acpi_build_libiores_template(struct acpi_device *adev,
			struct acpi_buffer *buffer)
{
	acpi_handle handle = adev->handle;
	struct acpi_resource *resource;
	acpi_status status;
	int res_cnt = 0;

	status = acpi_walk_resources(handle, METHOD_NAME__CRS,
				     acpi_count_libiores, &res_cnt);
	if (ACPI_FAILURE(status) || !res_cnt) {
		dev_err(&adev->dev, "can't evaluate _CRS: %d\n", status);
		return -EINVAL;
	}

	buffer->length = sizeof(struct acpi_resource) * (res_cnt + 1) + 1;
	buffer->pointer = kzalloc(buffer->length - 1, GFP_KERNEL);
	if (!buffer->pointer)
		return -ENOMEM;

	resource = (struct acpi_resource *)buffer->pointer;
	status = acpi_walk_resources(handle, METHOD_NAME__CRS,
				     acpi_read_one_libiores, &resource);
	if (ACPI_FAILURE(status)) {
		kfree(buffer->pointer);
		dev_err(&adev->dev, "can't evaluate _CRS: %d\n", status);
		return -EINVAL;
	}

	resource->type = ACPI_RESOURCE_TYPE_END_TAG;
	resource->length = sizeof(struct acpi_resource);

	return 0;
}

static int acpi_translate_libiores(struct acpi_device *adev,
		struct acpi_device *host, struct acpi_buffer *buffer)
{
	int res_cnt = (buffer->length - 1) / sizeof(struct acpi_resource) - 1;
	struct acpi_resource *resource = buffer->pointer;
	struct acpi_resource_address64 addr;
	unsigned long sys_port;
	struct device *dev = &adev->dev;

	/* only one I/O resource now */
	if (res_cnt != 1) {
		dev_err(dev, "encode %d resources whose type is(%d)!\n",
			res_cnt, resource->type);
		return -EINVAL;
	}

	if (ACPI_FAILURE(acpi_resource_to_address64(resource, &addr))) {
		dev_err(dev, "convert acpi resource(%d) as addr64 FAIL!\n",
			resource->type);
		return -EFAULT;
	}

	/* For indirect-IO, addr length must be fixed. (>0, 0/1, 0/1)(0,0,0) */
	if (addr.min_address_fixed != addr.max_address_fixed) {
		dev_warn(dev, "variable I/O resource is invalid!\n");
		return -EINVAL;
	}

	dev_dbg(dev, "CRS IO: len=0x%llx [0x%llx - 0x%llx]\n",
			addr.address.address_length, addr.address.minimum,
			addr.address.maximum);
	sys_port = libio_translate_hwaddr(&host->fwnode, addr.address.minimum);
	if (sys_port == -1) {
		dev_err(dev, "translate bus-addr(0x%llx) fail!\n",
			addr.address.minimum);
		return -EFAULT;
	}

	switch (resource->type) {
	case ACPI_RESOURCE_TYPE_ADDRESS32:
	{
		struct acpi_resource_address32 *out_res;

		out_res = &resource->data.address32;
		if (!addr.address.address_length)
			addr.address.address_length = out_res->address.maximum -
				out_res->address.minimum + 1;
		out_res->address.minimum = sys_port;
		out_res->address.maximum = sys_port +
				addr.address.address_length - 1;
		out_res->address.address_length = addr.address.address_length;

		dev_info(dev, "_SRS 32IO: [0x%x - 0x%x] len = 0x%x\n",
			out_res->address.minimum,
			out_res->address.maximum,
			out_res->address.address_length);

		break;
	}

	case ACPI_RESOURCE_TYPE_ADDRESS64:
	{
		struct acpi_resource_address64 *out_res;

		out_res = &resource->data.address64;
		if (!addr.address.address_length)
			addr.address.address_length = out_res->address.maximum -
				out_res->address.minimum + 1;
		out_res->address.minimum = sys_port;
		out_res->address.maximum = sys_port +
				addr.address.address_length - 1;
		out_res->address.address_length = addr.address.address_length;

		dev_info(dev, "_SRS 64IO: [0x%llx - 0x%llx] len = 0x%llx\n",
			out_res->address.minimum,
			out_res->address.maximum,
			out_res->address.address_length);

		break;
	}

	default:
		return -EINVAL;

	}

	return 0;
}

/*
 * update/set the current I/O resource of the designated device node.
 * after this calling, the enumeration can be started as the I/O resource
 * had been translated to logicial I/O from bus-local I/O.
 *
 * @adev: the device node to be updated the I/O resource;
 * @host: the device node where 'adev' is attached, which can be not
 *	the parent of 'adev';
 *
 * return 0 when successful, negative is for failure.
 */
int acpi_set_libio_resource(struct device *child,
		struct device *hostdev)
{
	struct acpi_device *adev;
	struct acpi_device *host;
	struct acpi_buffer buffer;
	acpi_status status;
	int ret;

	if (!child || !hostdev)
		return -EINVAL;

	host = to_acpi_device(hostdev);
	adev = to_acpi_device(child);

	/* check the device state */
	if (!adev->status.present) {
		dev_info(child, "ACPI: device is not present!\n");
		return 0;
	}
	/* whether the child had been enumerated? */
	if (acpi_device_enumerated(adev)) {
		dev_info(child, "ACPI: had been enumerated!\n");
		return 0;
	}

	/* read the _CRS and convert as acpi_buffer */
	status = acpi_build_libiores_template(adev, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_warn(child, "Failure evaluating %s\n", METHOD_NAME__CRS);
		return -ENODEV;
	}

	/* translate the I/O resources */
	ret = acpi_translate_libiores(adev, host, &buffer);
	if (ret) {
		kfree(buffer.pointer);
		dev_err(child, "Translate I/O range FAIL!\n");
		return ret;
	}

	/* set current resource... */
	status = acpi_set_current_resources(adev->handle, &buffer);
	kfree(buffer.pointer);
	if (ACPI_FAILURE(status)) {
		dev_err(child, "Error evaluating _SRS (0x%x)\n", status);
		ret = -EIO;
	}

	return ret;
}
#endif

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
