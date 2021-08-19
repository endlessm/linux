// SPDX-License-Identifier: GPL-2.0
/*
 * PCI Endpoint *Controller* (EPC) library
 *
 * Copyright (C) 2017 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_device.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci-ep-cfs.h>

static struct class *pci_epc_class;

static void devm_pci_epc_release(struct device *dev, void *res)
{
	struct pci_epc *epc = *(struct pci_epc **)res;

	pci_epc_destroy(epc);
}

static int devm_pci_epc_match(struct device *dev, void *res, void *match_data)
{
	struct pci_epc **epc = res;

	return *epc == match_data;
}

/**
 * pci_epc_put() - release the PCI endpoint controller
 * @epc: epc returned by pci_epc_get()
 *
 * release the refcount the caller obtained by invoking pci_epc_get()
 */
void pci_epc_put(struct pci_epc *epc)
{
	if (!epc || IS_ERR(epc))
		return;

	module_put(epc->ops->owner);
	put_device(&epc->dev);
}
EXPORT_SYMBOL_GPL(pci_epc_put);

/**
 * pci_epc_get() - get the PCI endpoint controller
 * @epc_name: device name of the endpoint controller
 *
 * Invoke to get struct pci_epc * corresponding to the device name of the
 * endpoint controller
 */
struct pci_epc *pci_epc_get(const char *epc_name)
{
	int ret = -EINVAL;
	struct pci_epc *epc;
	struct device *dev;
	struct class_dev_iter iter;

	class_dev_iter_init(&iter, pci_epc_class, NULL, NULL);
	while ((dev = class_dev_iter_next(&iter))) {
		if (strcmp(epc_name, dev_name(dev)))
			continue;

		epc = to_pci_epc(dev);
		if (!try_module_get(epc->ops->owner)) {
			ret = -EINVAL;
			goto err;
		}

		class_dev_iter_exit(&iter);
		get_device(&epc->dev);
		return epc;
	}

err:
	class_dev_iter_exit(&iter);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(pci_epc_get);

/**
 * pci_epc_get_first_free_bar() - helper to get first unreserved BAR
 * @epc_features: pci_epc_features structure that holds the reserved bar bitmap
 *
 * Invoke to get the first unreserved BAR that can be used for endpoint
 * function. For any incorrect value in reserved_bar return '0'.
 */
unsigned int pci_epc_get_first_free_bar(const struct pci_epc_features
					*epc_features)
{
	int free_bar;

	if (!epc_features)
		return 0;

	free_bar = ffz(epc_features->reserved_bar);
	if (free_bar > 5)
		return 0;

	return free_bar;
}
EXPORT_SYMBOL_GPL(pci_epc_get_first_free_bar);

/**
 * pci_epc_get_features() - get the features supported by EPC
 * @epc: the features supported by *this* EPC device will be returned
 * @func_no: the features supported by the EPC device specific to the
 *	     endpoint function with func_no will be returned
 *
 * Invoke to get the features provided by the EPC which may be
 * specific to an endpoint function. Returns pci_epc_features on success
 * and NULL for any failures.
 */
const struct pci_epc_features *pci_epc_get_features(struct pci_epc *epc,
						    u8 func_no)
{
	const struct pci_epc_features *epc_features;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return NULL;

	if (!epc->ops->get_features)
		return NULL;

	mutex_lock(&epc->lock);
	epc_features = epc->ops->get_features(epc, func_no);
	mutex_unlock(&epc->lock);

	return epc_features;
}
EXPORT_SYMBOL_GPL(pci_epc_get_features);

/**
 * pci_epc_stop() - stop the PCI link
 * @epc: the link of the EPC device that has to be stopped
 *
 * Invoke to stop the PCI link
 */
void pci_epc_stop(struct pci_epc *epc)
{
	if (IS_ERR(epc) || !epc->ops->stop)
		return;

	mutex_lock(&epc->lock);
	epc->ops->stop(epc);
	mutex_unlock(&epc->lock);
}
EXPORT_SYMBOL_GPL(pci_epc_stop);

/**
 * pci_epc_start() - start the PCI link
 * @epc: the link of *this* EPC device has to be started
 *
 * Invoke to start the PCI link
 */
int pci_epc_start(struct pci_epc *epc)
{
	int ret;

	if (IS_ERR(epc))
		return -EINVAL;

	if (!epc->ops->start)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->start(epc);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_start);

/**
 * pci_epc_raise_irq() - interrupt the host system
 * @epc: the EPC device which has to interrupt the host
 * @func_no: the endpoint function number in the EPC device
 * @type: specify the type of interrupt; legacy, MSI or MSI-X
 * @interrupt_num: the MSI or MSI-X interrupt number
 *
 * Invoke to raise an legacy, MSI or MSI-X interrupt
 */
int pci_epc_raise_irq(struct pci_epc *epc, u8 func_no,
		      enum pci_epc_irq_type type, u16 interrupt_num)
{
	int ret;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return -EINVAL;

	if (!epc->ops->raise_irq)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->raise_irq(epc, func_no, type, interrupt_num);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_raise_irq);

/**
 * pci_epc_get_msi() - get the number of MSI interrupt numbers allocated
 * @epc: the EPC device to which MSI interrupts was requested
 * @func_no: the endpoint function number in the EPC device
 *
 * Invoke to get the number of MSI interrupts allocated by the RC
 */
int pci_epc_get_msi(struct pci_epc *epc, u8 func_no)
{
	int interrupt;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return 0;

	if (!epc->ops->get_msi)
		return 0;

	mutex_lock(&epc->lock);
	interrupt = epc->ops->get_msi(epc, func_no);
	mutex_unlock(&epc->lock);

	if (interrupt < 0)
		return 0;

	interrupt = 1 << interrupt;

	return interrupt;
}
EXPORT_SYMBOL_GPL(pci_epc_get_msi);

/**
 * pci_epc_set_msi() - set the number of MSI interrupt numbers required
 * @epc: the EPC device on which MSI has to be configured
 * @func_no: the endpoint function number in the EPC device
 * @interrupts: number of MSI interrupts required by the EPF
 *
 * Invoke to set the required number of MSI interrupts.
 */
int pci_epc_set_msi(struct pci_epc *epc, u8 func_no, u8 interrupts)
{
	int ret;
	u8 encode_int;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions ||
	    interrupts > 32)
		return -EINVAL;

	if (!epc->ops->set_msi)
		return 0;

	encode_int = order_base_2(interrupts);

	mutex_lock(&epc->lock);
	ret = epc->ops->set_msi(epc, func_no, encode_int);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_set_msi);

/**
 * pci_epc_get_msix() - get the number of MSI-X interrupt numbers allocated
 * @epc: the EPC device to which MSI-X interrupts was requested
 * @func_no: the endpoint function number in the EPC device
 *
 * Invoke to get the number of MSI-X interrupts allocated by the RC
 */
int pci_epc_get_msix(struct pci_epc *epc, u8 func_no)
{
	int interrupt;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return 0;

	if (!epc->ops->get_msix)
		return 0;

	mutex_lock(&epc->lock);
	interrupt = epc->ops->get_msix(epc, func_no);
	mutex_unlock(&epc->lock);

	if (interrupt < 0)
		return 0;

	return interrupt + 1;
}
EXPORT_SYMBOL_GPL(pci_epc_get_msix);

/**
 * pci_epc_set_msix() - set the number of MSI-X interrupt numbers required
 * @epc: the EPC device on which MSI-X has to be configured
 * @func_no: the endpoint function number in the EPC device
 * @interrupts: number of MSI-X interrupts required by the EPF
 * @bir: BAR where the MSI-X table resides
 * @offset: Offset pointing to the start of MSI-X table
 *
 * Invoke to set the required number of MSI-X interrupts.
 */
int pci_epc_set_msix(struct pci_epc *epc, u8 func_no, u16 interrupts,
		     enum pci_barno bir, u32 offset)
{
	int ret;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions ||
	    interrupts < 1 || interrupts > 2048)
		return -EINVAL;

	if (!epc->ops->set_msix)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->set_msix(epc, func_no, interrupts - 1, bir, offset);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_set_msix);

/**
 * pci_epc_unmap_addr() - unmap CPU address from PCI address
 * @epc: the EPC device on which address is allocated
 * @func_no: the endpoint function number in the EPC device
 * @phys_addr: physical address of the local system
 *
 * Invoke to unmap the CPU address from PCI address.
 */
void pci_epc_unmap_addr(struct pci_epc *epc, u8 func_no,
			phys_addr_t phys_addr)
{
	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return;

	if (!epc->ops->unmap_addr)
		return;

	mutex_lock(&epc->lock);
	epc->ops->unmap_addr(epc, func_no, phys_addr);
	mutex_unlock(&epc->lock);
}
EXPORT_SYMBOL_GPL(pci_epc_unmap_addr);

/**
 * pci_epc_map_addr() - map CPU address to PCI address
 * @epc: the EPC device on which address is allocated
 * @func_no: the endpoint function number in the EPC device
 * @phys_addr: physical address of the local system
 * @pci_addr: PCI address to which the physical address should be mapped
 * @size: the size of the allocation
 *
 * Invoke to map CPU address with PCI address.
 */
int pci_epc_map_addr(struct pci_epc *epc, u8 func_no,
		     phys_addr_t phys_addr, u64 pci_addr, size_t size)
{
	int ret;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return -EINVAL;

	if (!epc->ops->map_addr)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->map_addr(epc, func_no, phys_addr, pci_addr, size);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_map_addr);

/**
 * pci_epc_clear_bar() - reset the BAR
 * @epc: the EPC device for which the BAR has to be cleared
 * @func_no: the endpoint function number in the EPC device
 * @epf_bar: the struct epf_bar that contains the BAR information
 *
 * Invoke to reset the BAR of the endpoint device.
 */
void pci_epc_clear_bar(struct pci_epc *epc, u8 func_no,
		       struct pci_epf_bar *epf_bar)
{
	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions ||
	    (epf_bar->barno == BAR_5 &&
	     epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64))
		return;

	if (!epc->ops->clear_bar)
		return;

	mutex_lock(&epc->lock);
	epc->ops->clear_bar(epc, func_no, epf_bar);
	mutex_unlock(&epc->lock);
}
EXPORT_SYMBOL_GPL(pci_epc_clear_bar);

/**
 * pci_epc_set_bar() - configure BAR in order for host to assign PCI addr space
 * @epc: the EPC device on which BAR has to be configured
 * @func_no: the endpoint function number in the EPC device
 * @epf_bar: the struct epf_bar that contains the BAR information
 *
 * Invoke to configure the BAR of the endpoint device.
 */
int pci_epc_set_bar(struct pci_epc *epc, u8 func_no,
		    struct pci_epf_bar *epf_bar)
{
	int ret;
	int flags = epf_bar->flags;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions ||
	    (epf_bar->barno == BAR_5 &&
	     flags & PCI_BASE_ADDRESS_MEM_TYPE_64) ||
	    (flags & PCI_BASE_ADDRESS_SPACE_IO &&
	     flags & PCI_BASE_ADDRESS_IO_MASK) ||
	    (upper_32_bits(epf_bar->size) &&
	     !(flags & PCI_BASE_ADDRESS_MEM_TYPE_64)))
		return -EINVAL;

	if (!epc->ops->set_bar)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->set_bar(epc, func_no, epf_bar);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_set_bar);

/**
 * pci_epc_write_header() - write standard configuration header
 * @epc: the EPC device to which the configuration header should be written
 * @func_no: the endpoint function number in the EPC device
 * @header: standard configuration header fields
 *
 * Invoke to write the configuration header to the endpoint controller. Every
 * endpoint controller will have a dedicated location to which the standard
 * configuration header would be written. The callback function should write
 * the header fields to this dedicated location.
 */
int pci_epc_write_header(struct pci_epc *epc, u8 func_no,
			 struct pci_epf_header *header)
{
	int ret;

	if (IS_ERR_OR_NULL(epc) || func_no >= epc->max_functions)
		return -EINVAL;

	if (!epc->ops->write_header)
		return 0;

	mutex_lock(&epc->lock);
	ret = epc->ops->write_header(epc, func_no, header);
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_write_header);

/**
 * pci_epc_add_epf() - bind PCI endpoint function to an endpoint controller
 * @epc: the EPC device to which the endpoint function should be added
 * @epf: the endpoint function to be added
 *
 * A PCI endpoint device can have one or more functions. In the case of PCIe,
 * the specification allows up to 8 PCIe endpoint functions. Invoke
 * pci_epc_add_epf() to add a PCI endpoint function to an endpoint controller.
 */
int pci_epc_add_epf(struct pci_epc *epc, struct pci_epf *epf)
{
	u32 func_no;
	int ret = 0;

	if (epf->epc)
		return -EBUSY;

	if (IS_ERR(epc))
		return -EINVAL;

	mutex_lock(&epc->lock);
	func_no = find_first_zero_bit(&epc->function_num_map,
				      BITS_PER_LONG);
	if (func_no >= BITS_PER_LONG) {
		ret = -EINVAL;
		goto ret;
	}

	if (func_no > epc->max_functions - 1) {
		dev_err(&epc->dev, "Exceeding max supported Function Number\n");
		ret = -EINVAL;
		goto ret;
	}

	set_bit(func_no, &epc->function_num_map);
	epf->func_no = func_no;
	epf->epc = epc;

	list_add_tail(&epf->list, &epc->pci_epf);

ret:
	mutex_unlock(&epc->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(pci_epc_add_epf);

/**
 * pci_epc_remove_epf() - remove PCI endpoint function from endpoint controller
 * @epc: the EPC device from which the endpoint function should be removed
 * @epf: the endpoint function to be removed
 *
 * Invoke to remove PCI endpoint function from the endpoint controller.
 */
void pci_epc_remove_epf(struct pci_epc *epc, struct pci_epf *epf)
{
	if (!epc || IS_ERR(epc) || !epf)
		return;

	mutex_lock(&epc->lock);
	clear_bit(epf->func_no, &epc->function_num_map);
	list_del(&epf->list);
	epf->epc = NULL;
	mutex_unlock(&epc->lock);
}
EXPORT_SYMBOL_GPL(pci_epc_remove_epf);

/**
 * pci_epc_linkup() - Notify the EPF device that EPC device has established a
 *		      connection with the Root Complex.
 * @epc: the EPC device which has established link with the host
 *
 * Invoke to Notify the EPF device that the EPC device has established a
 * connection with the Root Complex.
 */
void pci_epc_linkup(struct pci_epc *epc)
{
	if (!epc || IS_ERR(epc))
		return;

	atomic_notifier_call_chain(&epc->notifier, LINK_UP, NULL);
}
EXPORT_SYMBOL_GPL(pci_epc_linkup);

/**
 * pci_epc_init_notify() - Notify the EPF device that EPC device's core
 *			   initialization is completed.
 * @epc: the EPC device whose core initialization is completeds
 *
 * Invoke to Notify the EPF device that the EPC device's initialization
 * is completed.
 */
void pci_epc_init_notify(struct pci_epc *epc)
{
	if (!epc || IS_ERR(epc))
		return;

	atomic_notifier_call_chain(&epc->notifier, CORE_INIT, NULL);
}
EXPORT_SYMBOL_GPL(pci_epc_init_notify);

/**
 * pci_epc_destroy() - destroy the EPC device
 * @epc: the EPC device that has to be destroyed
 *
 * Invoke to destroy the PCI EPC device
 */
void pci_epc_destroy(struct pci_epc *epc)
{
	pci_ep_cfs_remove_epc_group(epc->group);
	device_unregister(&epc->dev);
	kfree(epc);
}
EXPORT_SYMBOL_GPL(pci_epc_destroy);

/**
 * devm_pci_epc_destroy() - destroy the EPC device
 * @dev: device that wants to destroy the EPC
 * @epc: the EPC device that has to be destroyed
 *
 * Invoke to destroy the devres associated with this
 * pci_epc and destroy the EPC device.
 */
void devm_pci_epc_destroy(struct device *dev, struct pci_epc *epc)
{
	int r;

	r = devres_destroy(dev, devm_pci_epc_release, devm_pci_epc_match,
			   epc);
	dev_WARN_ONCE(dev, r, "couldn't find PCI EPC resource\n");
}
EXPORT_SYMBOL_GPL(devm_pci_epc_destroy);

/**
 * __pci_epc_create() - create a new endpoint controller (EPC) device
 * @dev: device that is creating the new EPC
 * @ops: function pointers for performing EPC operations
 * @owner: the owner of the module that creates the EPC device
 *
 * Invoke to create a new EPC device and add it to pci_epc class.
 */
struct pci_epc *
__pci_epc_create(struct device *dev, const struct pci_epc_ops *ops,
		 struct module *owner)
{
	int ret;
	struct pci_epc *epc;

	if (WARN_ON(!dev)) {
		ret = -EINVAL;
		goto err_ret;
	}

	epc = kzalloc(sizeof(*epc), GFP_KERNEL);
	if (!epc) {
		ret = -ENOMEM;
		goto err_ret;
	}

	mutex_init(&epc->lock);
	INIT_LIST_HEAD(&epc->pci_epf);
	ATOMIC_INIT_NOTIFIER_HEAD(&epc->notifier);

	device_initialize(&epc->dev);
	epc->dev.class = pci_epc_class;
	epc->dev.parent = dev;
	epc->ops = ops;

	ret = dev_set_name(&epc->dev, "%s", dev_name(dev));
	if (ret)
		goto put_dev;

	ret = device_add(&epc->dev);
	if (ret)
		goto put_dev;

	epc->group = pci_ep_cfs_add_epc_group(dev_name(dev));

	return epc;

put_dev:
	put_device(&epc->dev);
	kfree(epc);

err_ret:
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(__pci_epc_create);

/**
 * __devm_pci_epc_create() - create a new endpoint controller (EPC) device
 * @dev: device that is creating the new EPC
 * @ops: function pointers for performing EPC operations
 * @owner: the owner of the module that creates the EPC device
 *
 * Invoke to create a new EPC device and add it to pci_epc class.
 * While at that, it also associates the device with the pci_epc using devres.
 * On driver detach, release function is invoked on the devres data,
 * then, devres data is freed.
 */
struct pci_epc *
__devm_pci_epc_create(struct device *dev, const struct pci_epc_ops *ops,
		      struct module *owner)
{
	struct pci_epc **ptr, *epc;

	ptr = devres_alloc(devm_pci_epc_release, sizeof(*ptr), GFP_KERNEL);
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	epc = __pci_epc_create(dev, ops, owner);
	if (!IS_ERR(epc)) {
		*ptr = epc;
		devres_add(dev, ptr);
	} else {
		devres_free(ptr);
	}

	return epc;
}
EXPORT_SYMBOL_GPL(__devm_pci_epc_create);

static int __init pci_epc_init(void)
{
	pci_epc_class = class_create(THIS_MODULE, "pci_epc");
	if (IS_ERR(pci_epc_class)) {
		pr_err("failed to create pci epc class --> %ld\n",
		       PTR_ERR(pci_epc_class));
		return PTR_ERR(pci_epc_class);
	}

	return 0;
}
module_init(pci_epc_init);

static void __exit pci_epc_exit(void)
{
	class_destroy(pci_epc_class);
}
module_exit(pci_epc_exit);

MODULE_DESCRIPTION("PCI EPC Library");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
