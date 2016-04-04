/*
 * Copyright (C) 2014 Intel Corporation
 *
 * Authors:
 * Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
 *
 * Maintained by: <tpmdd-devel@lists.sourceforge.net>
 *
 * This device driver implements the TPM interface as defined in
 * the TCG CRB 2.0 TPM specification.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/acpi.h>
#include <linux/highmem.h>
#include <linux/rculist.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include "tpm.h"

#define ACPI_SIG_TPM2 "TPM2"

static const u8 CRB_ACPI_START_UUID[] = {
	/* 0000 */ 0xAB, 0x6C, 0xBF, 0x6B, 0x63, 0x54, 0x14, 0x47,
	/* 0008 */ 0xB7, 0xCD, 0xF0, 0x20, 0x3C, 0x03, 0x68, 0xD4
};

enum crb_defaults {
	CRB_ACPI_START_REVISION_ID = 1,
	CRB_ACPI_START_INDEX = 1,
};

enum crb_ca_request {
	CRB_CA_REQ_GO_IDLE	= BIT(0),
	CRB_CA_REQ_CMD_READY	= BIT(1),
};

enum crb_ca_status {
	CRB_CA_STS_ERROR	= BIT(0),
	CRB_CA_STS_TPM_IDLE	= BIT(1),
};

enum crb_start {
	CRB_START_INVOKE	= BIT(0),
};

enum crb_cancel {
	CRB_CANCEL_INVOKE	= BIT(0),
};

struct crb_control_area {
	u32 req;
	u32 sts;
	u32 cancel;
	u32 start;
	u32 int_enable;
	u32 int_sts;
	u32 cmd_size;
	u32 cmd_pa_low;
	u32 cmd_pa_high;
	u32 rsp_size;
	u64 rsp_pa;
} __packed;

enum crb_status {
	CRB_STS_COMPLETE	= BIT(0),
};

enum crb_flags {
	CRB_FL_ACPI_START	= BIT(0),
	CRB_FL_CRB_START	= BIT(1),
};

struct crb_priv {
	unsigned int flags;
	struct resource res;
	void __iomem *iobase;
	struct crb_control_area __iomem *cca;
	u8 __iomem *cmd;
	u8 __iomem *rsp;
};

static SIMPLE_DEV_PM_OPS(crb_pm, tpm_pm_suspend, tpm_pm_resume);

static u8 crb_status(struct tpm_chip *chip)
{
	struct crb_priv *priv = chip->vendor.priv;
	u8 sts = 0;

	if ((ioread32(&priv->cca->start) & CRB_START_INVOKE) !=
	    CRB_START_INVOKE)
		sts |= CRB_STS_COMPLETE;

	return sts;
}

static int crb_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
	struct crb_priv *priv = chip->vendor.priv;
	unsigned int expected;

	/* sanity check */
	if (count < 6)
		return -EIO;

	if (ioread32(&priv->cca->sts) & CRB_CA_STS_ERROR)
		return -EIO;

	memcpy_fromio(buf, priv->rsp, 6);
	expected = be32_to_cpup((__be32 *) &buf[2]);

	if (expected > count)
		return -EIO;

	memcpy_fromio(&buf[6], &priv->rsp[6], expected - 6);

	return expected;
}

static int crb_do_acpi_start(struct tpm_chip *chip)
{
	union acpi_object *obj;
	int rc;

	obj = acpi_evaluate_dsm(chip->acpi_dev_handle,
				CRB_ACPI_START_UUID,
				CRB_ACPI_START_REVISION_ID,
				CRB_ACPI_START_INDEX,
				NULL);
	if (!obj)
		return -ENXIO;
	rc = obj->integer.value == 0 ? 0 : -ENXIO;
	ACPI_FREE(obj);
	return rc;
}

static int crb_send(struct tpm_chip *chip, u8 *buf, size_t len)
{
	struct crb_priv *priv = chip->vendor.priv;
	int rc = 0;

	if (len > ioread32(&priv->cca->cmd_size)) {
		dev_err(&chip->dev,
			"invalid command count value %x %zx\n",
			(unsigned int) len,
			(size_t) ioread32(&priv->cca->cmd_size));
		return -E2BIG;
	}

	memcpy_toio(priv->cmd, buf, len);

	/* Make sure that cmd is populated before issuing start. */
	wmb();

	if (priv->flags & CRB_FL_CRB_START)
		iowrite32(cpu_to_le32(CRB_START_INVOKE), &priv->cca->start);

	if (priv->flags & CRB_FL_ACPI_START)
		rc = crb_do_acpi_start(chip);

	return rc;
}

static void crb_cancel(struct tpm_chip *chip)
{
	struct crb_priv *priv = chip->vendor.priv;

	iowrite32(cpu_to_le32(CRB_CANCEL_INVOKE), &priv->cca->cancel);

	/* Make sure that cmd is populated before issuing cancel. */
	wmb();

	if ((priv->flags & CRB_FL_ACPI_START) && crb_do_acpi_start(chip))
		dev_err(&chip->dev, "ACPI Start failed\n");

	iowrite32(0, &priv->cca->cancel);
}

static bool crb_req_canceled(struct tpm_chip *chip, u8 status)
{
	struct crb_priv *priv = chip->vendor.priv;
	u32 cancel = ioread32(&priv->cca->cancel);

	return (cancel & CRB_CANCEL_INVOKE) == CRB_CANCEL_INVOKE;
}

static const struct tpm_class_ops tpm_crb = {
	.status = crb_status,
	.recv = crb_recv,
	.send = crb_send,
	.cancel = crb_cancel,
	.req_canceled = crb_req_canceled,
	.req_complete_mask = CRB_STS_COMPLETE,
	.req_complete_val = CRB_STS_COMPLETE,
};

static int crb_init(struct acpi_device *device, struct crb_priv *priv)
{
	struct tpm_chip *chip;
	int rc;

	chip = tpmm_chip_alloc(&device->dev, &tpm_crb);
	if (IS_ERR(chip))
		return PTR_ERR(chip);

	chip->vendor.priv = priv;
	chip->acpi_dev_handle = device->handle;
	chip->flags = TPM_CHIP_FLAG_TPM2;

	rc = tpm_get_timeouts(chip);
	if (rc)
		return rc;

	rc = tpm2_do_selftest(chip);
	if (rc)
		return rc;

	return tpm_chip_register(chip);
}

static int crb_check_resource(struct acpi_resource *ares, void *data)
{
	struct crb_priv *priv = data;
	struct resource res;

	if (acpi_dev_resource_memory(ares, &res)) {
		priv->res = res;
		priv->res.name = NULL;
	}

	return 1;
}

static void __iomem *crb_map_res(struct device *dev, struct crb_priv *priv,
				 u64 start, u32 size)
{
	struct resource new_res = {
		.start	= start,
		.end	= start + size - 1,
		.flags	= IORESOURCE_MEM,
	};

	/* Detect a 64 bit address on a 32 bit system */
	if (start != new_res.start)
		return ERR_PTR(-EINVAL);

	if (!resource_contains(&priv->res, &new_res))
		return devm_ioremap_resource(dev, &new_res);

	return priv->iobase + (new_res.start - priv->res.start);
}

static int crb_map_io(struct acpi_device *device, struct crb_priv *priv,
		      struct acpi_table_tpm2 *buf)
{
	struct list_head resources;
	struct device *dev = &device->dev;
	u64 pa;
	int ret;

	INIT_LIST_HEAD(&resources);
	ret = acpi_dev_get_resources(device, &resources, crb_check_resource,
				     priv);
	if (ret < 0)
		return ret;
	acpi_dev_free_resource_list(&resources);

	if (resource_type(&priv->res) != IORESOURCE_MEM) {
		dev_err(dev,
			FW_BUG "TPM2 ACPI table does not define a memory resource\n");
		return -EINVAL;
	}

	priv->iobase = devm_ioremap_resource(dev, &priv->res);
	if (IS_ERR(priv->iobase))
		return PTR_ERR(priv->iobase);

	priv->cca = crb_map_res(dev, priv, buf->control_address, 0x1000);
	if (IS_ERR(priv->cca))
		return PTR_ERR(priv->cca);

	pa = ((u64) ioread32(&priv->cca->cmd_pa_high) << 32) |
	      (u64) ioread32(&priv->cca->cmd_pa_low);
	priv->cmd = crb_map_res(dev, priv, pa, ioread32(&priv->cca->cmd_size));
	if (IS_ERR(priv->cmd))
		return PTR_ERR(priv->cmd);

	memcpy_fromio(&pa, &priv->cca->rsp_pa, 8);
	pa = le64_to_cpu(pa);
	priv->rsp = crb_map_res(dev, priv, pa, ioread32(&priv->cca->rsp_size));
	return PTR_ERR_OR_ZERO(priv->rsp);
}

static int crb_acpi_add(struct acpi_device *device)
{
	struct acpi_table_tpm2 *buf;
	struct crb_priv *priv;
	struct device *dev = &device->dev;
	acpi_status status;
	u32 sm;
	int rc;

	status = acpi_get_table(ACPI_SIG_TPM2, 1,
				(struct acpi_table_header **) &buf);
	if (ACPI_FAILURE(status) || buf->header.length < sizeof(*buf)) {
		dev_err(dev, FW_BUG "failed to get TPM2 ACPI table\n");
		return -EINVAL;
	}

	/* Should the FIFO driver handle this? */
	sm = buf->start_method;
	if (sm == ACPI_TPM2_MEMORY_MAPPED)
		return -ENODEV;

	priv = devm_kzalloc(dev, sizeof(struct crb_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* The reason for the extra quirk is that the PTT in 4th Gen Core CPUs
	 * report only ACPI start but in practice seems to require both
	 * ACPI start and CRB start.
	 */
	if (sm == ACPI_TPM2_COMMAND_BUFFER || sm == ACPI_TPM2_MEMORY_MAPPED ||
	    !strcmp(acpi_device_hid(device), "MSFT0101"))
		priv->flags |= CRB_FL_CRB_START;

	if (sm == ACPI_TPM2_START_METHOD ||
	    sm == ACPI_TPM2_COMMAND_BUFFER_WITH_START_METHOD)
		priv->flags |= CRB_FL_ACPI_START;

	rc = crb_map_io(device, priv, buf);
	if (rc)
		return rc;

	return crb_init(device, priv);
}

static int crb_acpi_remove(struct acpi_device *device)
{
	struct device *dev = &device->dev;
	struct tpm_chip *chip = dev_get_drvdata(dev);

	if (chip->flags & TPM_CHIP_FLAG_TPM2)
		tpm2_shutdown(chip, TPM2_SU_CLEAR);

	tpm_chip_unregister(chip);

	return 0;
}

static struct acpi_device_id crb_device_ids[] = {
	{"MSFT0101", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, crb_device_ids);

static struct acpi_driver crb_acpi_driver = {
	.name = "tpm_crb",
	.ids = crb_device_ids,
	.ops = {
		.add = crb_acpi_add,
		.remove = crb_acpi_remove,
	},
	.drv = {
		.pm = &crb_pm,
	},
};

module_acpi_driver(crb_acpi_driver);
MODULE_AUTHOR("Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>");
MODULE_DESCRIPTION("TPM2 Driver");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
