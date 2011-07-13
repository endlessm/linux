/* Copyright (c) 2009-2011 Freescale Semiconductor, Inc.
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

#ifndef CONFIG_SMP
#include <linux/smp.h>	/* get_hard_smp_processor_id() */
#endif
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "bman_private.h"

/* Last updated for v00.79 of the BG */

struct bman;

/* Register offsets */
#define REG_POOL_SWDET(n)	(0x0000 + ((n) * 0x04))
#define REG_POOL_HWDET(n)	(0x0100 + ((n) * 0x04))
#define REG_POOL_SWDXT(n)	(0x0200 + ((n) * 0x04))
#define REG_POOL_HWDXT(n)	(0x0300 + ((n) * 0x04))
#define REG_POOL_CONTENT(n)	(0x0600 + ((n) * 0x04))
#define REG_FBPR_FPC		0x0800
#define REG_ECSR		0x0a00
#define REG_ECIR		0x0a04
#define REG_EADR		0x0a08
#define REG_EDATA(n)		(0x0a10 + ((n) * 0x04))
#define REG_SBEC(n)		(0x0a80 + ((n) * 0x04))
#define REG_IP_REV_1		0x0bf8
#define REG_IP_REV_2		0x0bfc
#define REG_FBPR_BARE		0x0c00
#define REG_FBPR_BAR		0x0c04
#define REG_FBPR_AR		0x0c10
#define REG_SRCIDR		0x0d04
#define REG_LIODNR		0x0d08
#define REG_ERR_ISR		0x0e00	/* + "enum bm_isr_reg" */

/* Used by all error interrupt registers except 'inhibit' */
#define BM_EIRQ_IVCI	0x00000010	/* Invalid Command Verb */
#define BM_EIRQ_FLWI	0x00000008	/* FBPR Low Watermark */
#define BM_EIRQ_MBEI	0x00000004	/* Multi-bit ECC Error */
#define BM_EIRQ_SBEI	0x00000002	/* Single-bit ECC Error */
#define BM_EIRQ_BSCN	0x00000001	/* pool State Change Notification */

/* BMAN_ECIR valid error bit */
#define PORTAL_ECSR_ERR	(BM_EIRQ_IVCI)

union bman_ecir {
	u32 ecir_raw;
	struct {
		u32 __reserved1:4;
		u32 portal_num:4;
		u32 __reserved2:12;
		u32 numb:4;
		u32 __reserved3:2;
		u32 pid:6;
	} __packed info;
};

union bman_eadr {
	u32 eadr_raw;
	struct {
		u32 __reserved1:5;
		u32 memid:3;
		u32 __reserved2:14;
		u32 eadr:10;
	} __packed info;
};

struct bman_hwerr_txt {
	u32 mask;
	const char *txt;
};

#define BMAN_HWE_TXT(a, b) { .mask = BM_EIRQ_##a, .txt = b }

static const struct bman_hwerr_txt bman_hwerr_txts[] = {
	BMAN_HWE_TXT(IVCI, "Invalid Command Verb"),
	BMAN_HWE_TXT(FLWI, "FBPR Low Watermark"),
	BMAN_HWE_TXT(MBEI, "Multi-bit ECC Error"),
	BMAN_HWE_TXT(SBEI, "Single-bit ECC Error"),
	BMAN_HWE_TXT(BSCN, "Pool State Change Notification"),
};
#define BMAN_HWE_COUNT (sizeof(bman_hwerr_txts)/sizeof(struct bman_hwerr_txt))

struct bman_error_info_mdata {
	u16 addr_mask;
	u16 bits;
	const char *txt;
};

#define BMAN_ERR_MDATA(a, b, c) { .addr_mask = a, .bits = b, .txt = c}
static const struct bman_error_info_mdata error_mdata[] = {
	BMAN_ERR_MDATA(0x03FF, 192, "Stockpile memory"),
	BMAN_ERR_MDATA(0x00FF, 256, "SW portal ring memory port 1"),
	BMAN_ERR_MDATA(0x00FF, 256, "SW portal ring memory port 2"),
};
#define BMAN_ERR_MDATA_COUNT \
	(sizeof(error_mdata)/sizeof(struct bman_error_info_mdata))

/* Add this in Kconfig */
#define BMAN_ERRS_TO_UNENABLE (BM_EIRQ_FLWI)

/**
 * bm_err_isr_<reg>_<verb> - Manipulate global interrupt registers
 * @v: for accessors that write values, this is the 32-bit value
 *
 * Manipulates BMAN_ERR_ISR, BMAN_ERR_IER, BMAN_ERR_ISDR, BMAN_ERR_IIR. All
 * manipulations except bm_err_isr_[un]inhibit() use 32-bit masks composed of
 * the BM_EIRQ_*** definitions. Note that "bm_err_isr_enable_write" means
 * "write the enable register" rather than "enable the write register"!
 */
#define bm_err_isr_status_read(bm)	__bm_err_isr_read(bm, bm_isr_status)
#define bm_err_isr_status_clear(bm, m)	__bm_err_isr_write(bm, bm_isr_status,m)
#define bm_err_isr_enable_read(bm)	__bm_err_isr_read(bm, bm_isr_enable)
#define bm_err_isr_enable_write(bm, v)	__bm_err_isr_write(bm, bm_isr_enable,v)
#define bm_err_isr_disable_read(bm)	__bm_err_isr_read(bm, bm_isr_disable)
#define bm_err_isr_disable_write(bm, v)	__bm_err_isr_write(bm, bm_isr_disable,v)
#define bm_err_isr_inhibit(bm)		__bm_err_isr_write(bm, bm_isr_inhibit,1)
#define bm_err_isr_uninhibit(bm)	__bm_err_isr_write(bm, bm_isr_inhibit,0)

/*
 * TODO: unimplemented registers
 *
 * BMAN_POOLk_SDCNT, BMAN_POOLk_HDCNT, BMAN_FULT,
 * BMAN_VLDPL, BMAN_EECC, BMAN_SBET, BMAN_EINJ
 */

/* Encapsulate "struct bman *" as a cast of the register space address. */

static struct bman *bm_create(void *regs)
{
	return (struct bman *)regs;
}

static inline u32 __bm_in(struct bman *bm, u32 offset)
{
	return in_be32((void *)bm + offset);
}
static inline void __bm_out(struct bman *bm, u32 offset, u32 val)
{
	out_be32((void *)bm + offset, val);
}
#define bm_in(reg)		__bm_in(bm, REG_##reg)
#define bm_out(reg, val)	__bm_out(bm, REG_##reg, val)

static u32 __bm_err_isr_read(struct bman *bm, enum bm_isr_reg n)
{
	return __bm_in(bm, REG_ERR_ISR + (n << 2));
}

static void __bm_err_isr_write(struct bman *bm, enum bm_isr_reg n, u32 val)
{
	__bm_out(bm, REG_ERR_ISR + (n << 2), val);
}

#if 0
static void bm_get_details(struct bman *bm, u8 *int_options, u8 *errata,
			u8 *conf_options)
{
	u32 v = bm_in(IP_REV_1);
	*int_options = (v >> 16) & 0xff;
	*errata = (v >> 8) & 0xff;
	*conf_options = v & 0xff;
}

static u8 bm_get_corenet_sourceid(struct bman *bm)
{
	return bm_in(SRCIDR);
}

static void bm_set_liodn(struct bman *bm, u16 liodn)
{
	bm_out(LIODNR, liodn & 0xfff);
}

#endif

static void bm_get_version(struct bman *bm, u16 *id, u8 *major, u8 *minor)
{
	u32 v = bm_in(IP_REV_1);
	*id = (v >> 16);
	*major = (v >> 8) & 0xff;
	*minor = v & 0xff;
}

static u32 __generate_thresh(u32 val, int roundup)
{
	u32 e = 0;	/* co-efficient, exponent */
	int oddbit = 0;
	while(val > 0xff) {
		oddbit = val & 1;
		val >>= 1;
		e++;
		if(roundup && oddbit)
			val++;
	}
	DPA_ASSERT(e < 0x10);
	return (val | (e << 8));
}

static void bm_set_pool(struct bman *bm, u8 pool, u32 swdet, u32 swdxt,
			u32 hwdet, u32 hwdxt)
{
	DPA_ASSERT(pool < bman_pool_max);
	bm_out(POOL_SWDET(pool), __generate_thresh(swdet, 0));
	bm_out(POOL_SWDXT(pool), __generate_thresh(swdxt, 1));
	bm_out(POOL_HWDET(pool), __generate_thresh(hwdet, 0));
	bm_out(POOL_HWDXT(pool), __generate_thresh(hwdxt, 1));
}

static void bm_set_memory(struct bman *bm, u64 ba, int prio, u32 size)
{
	u32 exp = ilog2(size);
	/* choke if size isn't within range */
	DPA_ASSERT((size >= 4096) && (size <= 1073741824) &&
			is_power_of_2(size));
	/* choke if '[e]ba' has lower-alignment than 'size' */
	DPA_ASSERT(!(ba & (size - 1)));
	bm_out(FBPR_BARE, upper_32_bits(ba));
	bm_out(FBPR_BAR, lower_32_bits(ba));
	bm_out(FBPR_AR, (prio ? 0x40000000 : 0) | (exp - 1));
}

/*****************/
/* Config driver */
/*****************/

/* We support only one of these. */
static struct bman *bm;
static struct device_node *bm_node;

/* TODO: Kconfig these? */
#define DEFAULT_FBPR_SZ	(PAGE_SIZE << 12)

/* Parse the <name> property to extract the memory location and size and
 * memblock_reserve() it. If it isn't supplied, memblock_alloc() the default size. */
static __init int parse_mem_property(struct device_node *node, const char *name,
				dma_addr_t *addr, size_t *sz, int zero)
{
	const u32 *pint;
	int ret;

	pint = of_get_property(node, name, &ret);
	if (!pint || (ret != 16)) {
		pr_info("No %s property '%s', using memblock_alloc(%016zx)\n",
				node->full_name, name, *sz);
		*addr = memblock_alloc(*sz, *sz);
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
		return 0;
	}
	pr_info("Using %s property '%s'\n", node->full_name, name);
	/* If using a "zero-pma", don't try to zero it, even if you asked */
	if (zero && of_find_property(node, "zero-pma", &ret)) {
		pr_info("  it's a 'zero-pma', not zeroing from s/w\n");
		zero = 0;
	}
	*addr = ((u64)pint[0] << 32) | (u64)pint[1];
	*sz = ((u64)pint[2] << 32) | (u64)pint[3];
	/* Keep things simple, it's either all in the DRAM range or it's all
	 * outside. */
	if (*addr < memblock_end_of_DRAM()) {
		BUG_ON((u64)*addr + (u64)*sz > memblock_end_of_DRAM());
		if (memblock_reserve(*addr, *sz) < 0) {
			pr_err("Failed to reserve %s\n", name);
			return -ENOMEM;
		}
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
	} else if (zero) {
		/* map as cacheable, non-guarded */
		void *tmpp = ioremap_prot(*addr, *sz, 0);
		memset(tmpp, 0, *sz);
		iounmap(tmpp);
	}
	return 0;
}

static int __init fsl_bman_init(struct device_node *node)
{
	struct resource res;
	u32 __iomem *regs;
	const char *s;
	dma_addr_t fbpr_a = 0; /* gcc doesn't know this is unnecessary */
	size_t fbpr_sz = DEFAULT_FBPR_SZ;
	int ret, standby = 0;
	u16 id;
	u8 major, minor;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		pr_err("Can't get %s property 'reg'\n",
				node->full_name);
		return ret;
	}
	s = of_get_property(node, "fsl,hv-claimable", &ret);
	if (s && !strcmp(s, "standby"))
		standby = 1;
	if (!standby) {
		ret = parse_mem_property(node, "fsl,bman-fbpr",
					&fbpr_a, &fbpr_sz, 0);
		BUG_ON(ret);
	}
	/* Global configuration */
	regs = ioremap(res.start, res.end - res.start + 1);
	bm = bm_create(regs);
	BUG_ON(!bm);
	bm_node = node;
	bm_get_version(bm, &id, &major, &minor);
	pr_info("Bman ver:%04x,%02x,%02x\n", id, major, minor);
	if ((major == 1) && (minor == 0)) {
		bman_ip_rev = BMAN_REV10;
		bman_pool_max = 64;
	} else if ((major == 2) && (minor == 0)) {
		bman_ip_rev = BMAN_REV20;
		bman_pool_max = 8;
	} else {
		pr_warning("unknown Bman version, default to rev1.0\n");
	}

	if (standby) {
		pr_info("  -> in standby mode\n");
		return 0;
	}
	/* FBPR memory */
	bm_set_memory(bm, fbpr_a, 0, fbpr_sz);
	return 0;
}

int bman_have_ccsr(void)
{
	return (bm ? 1 : 0);
}

int bm_pool_set(u32 bpid, const u32 *thresholds)
{
	if (!bm)
		return -ENODEV;
	bm_set_pool(bm, bpid, thresholds[0], thresholds[1],
		thresholds[2], thresholds[3]);
	return 0;
}
EXPORT_SYMBOL(bm_pool_set);

__init void bman_init_early(void)
{
	struct device_node *dn;
	for_each_compatible_node(dn, NULL, "fsl,bman") {
		if (bm)
			pr_err("%s: only one 'fsl,bman' allowed\n",
				dn->full_name);
		else {
			int ret = fsl_bman_init(dn);
			BUG_ON(ret);
		}
	}
}

static void log_edata_bits(u32 bit_count)
{
	u32 i, j, mask = 0xffffffff;

	pr_warning("Bman ErrInt, EDATA:\n");
	i = bit_count/32;
	if (bit_count%32) {
		i++;
		mask = ~(mask << bit_count%32);
	}
	j = 16-i;
	pr_warning("  0x%08x\n", bm_in(EDATA(j)) & mask);
	j++;
	for (; j < 16; j++)
		pr_warning("  0x%08x\n", bm_in(EDATA(j)));
}

static void log_additional_error_info(u32 isr_val, u32 ecsr_val)
{
	union bman_ecir ecir_val;
	union bman_eadr eadr_val;

	ecir_val.ecir_raw = bm_in(ECIR);
	/* Is portal info valid */
	if (ecsr_val & PORTAL_ECSR_ERR) {
		pr_warning("Bman ErrInt: SWP id %d, numb %d, pid %d\n",
			ecir_val.info.portal_num, ecir_val.info.numb,
			ecir_val.info.pid);
	}
	if (ecsr_val & (BM_EIRQ_SBEI|BM_EIRQ_MBEI)) {
		eadr_val.eadr_raw = bm_in(EADR);
		pr_warning("Bman ErrInt: EADR Memory: %s, 0x%x\n",
			error_mdata[eadr_val.info.memid].txt,
			error_mdata[eadr_val.info.memid].addr_mask
				& eadr_val.info.eadr);
		log_edata_bits(error_mdata[eadr_val.info.memid].bits);
	}
}

/* Bman interrupt handler */
static irqreturn_t bman_isr(int irq, void *ptr)
{
	u32 isr_val, ier_val, ecsr_val, isr_mask, i;

	ier_val = bm_err_isr_enable_read(bm);
	isr_val = bm_err_isr_status_read(bm);
	ecsr_val = bm_in(ECSR);
	isr_mask = isr_val & ier_val;

	if (!isr_mask)
		return IRQ_NONE;
	for (i = 0; i < BMAN_HWE_COUNT; i++) {
		if (bman_hwerr_txts[i].mask & isr_mask) {
			pr_warning("Bman ErrInt: %s\n", bman_hwerr_txts[i].txt);
			if (bman_hwerr_txts[i].mask & ecsr_val) {
				log_additional_error_info(isr_mask, ecsr_val);
				/* Re-arm error capture registers */
				bm_out(ECSR, ecsr_val);
			}
			if (bman_hwerr_txts[i].mask & BMAN_ERRS_TO_UNENABLE) {
				pr_devel("Bman un-enabling error 0x%x\n",
					bman_hwerr_txts[i].mask);
				ier_val &= ~bman_hwerr_txts[i].mask;
				bm_err_isr_enable_write(bm, ier_val);
			}
		}
	}
	bm_err_isr_status_clear(bm, isr_val);
	return IRQ_HANDLED;
}

static int __bind_irq(void)
{
	int ret, err_irq;

	err_irq = of_irq_to_resource(bm_node, 0, NULL);
	if (err_irq == NO_IRQ) {
		pr_info("Can't get %s property '%s'\n", bm_node->full_name,
			"interrupts");
		return -ENODEV;
	}
	ret = request_irq(err_irq, bman_isr, IRQF_SHARED, "bman-err", bm_node);
	if (ret)  {
		pr_err("request_irq() failed %d for '%s'\n", ret,
			bm_node->full_name);
		return -ENODEV;
	}
	/* Disable Buffer Pool State Change */
	bm_err_isr_disable_write(bm, BM_EIRQ_BSCN);
	/* Write-to-clear any stale bits, (eg. starvation being asserted prior
	 * to resource allocation during driver init). */
	bm_err_isr_status_clear(bm, 0xffffffff);
	/* Enable Error Interrupts */
	bm_err_isr_enable_write(bm, 0xffffffff);
	return 0;
}

/* Initialise Error Interrupt Handler */
int bman_init_error_int(struct device_node *node)
{
	if (!bman_have_ccsr())
		return 0;
	if (node != bm_node)
		return -EINVAL;
	return __bind_irq();
}

#ifdef CONFIG_SYSFS

#define DRV_NAME "fsl-bman"

static ssize_t show_fbpr_fpc(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", bm_in(FBPR_FPC));
};

static ssize_t show_pool_count(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	u32 data;
	int i;

	if (!sscanf(dev_attr->attr.name, "%d", &i))
		return -EINVAL;
	data = bm_in(POOL_CONTENT(i));
	return snprintf(buf, PAGE_SIZE, "%d\n", data);
};

static ssize_t show_err_isr(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%08x\n", bm_in(ERR_ISR));
};

static ssize_t show_sbec(struct device *dev,
	struct device_attribute *dev_attr, char *buf)
{
	int i;

	if (!sscanf(dev_attr->attr.name, "sbec_%d", &i))
		return -EINVAL;
	return snprintf(buf, PAGE_SIZE, "%u\n", bm_in(SBEC(i)));
};

static DEVICE_ATTR(err_isr, S_IRUSR, show_err_isr, NULL);
static DEVICE_ATTR(fbpr_fpc, S_IRUSR, show_fbpr_fpc, NULL);

/* Didn't use DEVICE_ATTR as 64 of this would be required.
 * Initialize them when needed. */
static char *name_attrs_pool_count; /* "xx" + null-terminator */
static struct device_attribute *dev_attr_buffer_pool_count;

static DEVICE_ATTR(sbec_0, S_IRUSR, show_sbec, NULL);
static DEVICE_ATTR(sbec_1, S_IRUSR, show_sbec, NULL);

static struct attribute *bman_dev_attributes[] = {
	&dev_attr_fbpr_fpc.attr,
	&dev_attr_err_isr.attr,
	NULL
};

static struct attribute *bman_dev_ecr_attributes[] = {
	&dev_attr_sbec_0.attr,
	&dev_attr_sbec_1.attr,
	NULL
};

static struct attribute **bman_dev_pool_count_attributes;


/* root level */
static const struct attribute_group bman_dev_attr_grp = {
	.name = NULL,
	.attrs = bman_dev_attributes
};
static const struct attribute_group bman_dev_ecr_grp = {
	.name = "error_capture",
	.attrs = bman_dev_ecr_attributes
};
static struct attribute_group bman_dev_pool_countent_grp = {
	.name = "pool_count",
};

static int of_fsl_bman_remove(struct platform_device *ofdev)
{
	sysfs_remove_group(&ofdev->dev.kobj, &bman_dev_attr_grp);
	return 0;
};

static int of_fsl_bman_probe(struct platform_device *ofdev)
{
	int ret, i;

	ret = sysfs_create_group(&ofdev->dev.kobj, &bman_dev_attr_grp);
	if (ret)
		goto done;
	ret = sysfs_create_group(&ofdev->dev.kobj, &bman_dev_ecr_grp);
	if (ret)
		goto del_group_0;

	name_attrs_pool_count = kmalloc(sizeof(char) * bman_pool_max * 3,
								 GFP_KERNEL);
	if (!name_attrs_pool_count) {
		pr_err("Can't alloc name_attrs_pool_count\n");
		goto del_group_1;
	}

	dev_attr_buffer_pool_count = kmalloc(sizeof(struct device_attribute) *
					bman_pool_max, GFP_KERNEL);
	if (!dev_attr_buffer_pool_count) {
		pr_err("Can't alloc dev_attr-buffer_pool_count\n");
		goto del_group_2;
	}

	bman_dev_pool_count_attributes = kmalloc(sizeof(struct attribute *) *
			 (bman_pool_max + 1), GFP_KERNEL);
	if (!bman_dev_pool_count_attributes) {
			pr_err("can't alloc bman_dev_pool_count_attributes\n");
			goto del_group_3;
	}

	for (i = 0; i < (bman_pool_max + 1); i++) {
		bman_dev_pool_count_attributes[i] =
			kmalloc(sizeof(struct attribute), GFP_KERNEL);
		if (!bman_dev_pool_count_attributes[i]) {
			pr_err("cannot alloc for each"
				" bman_dev_pool_count_attributes\n");
			goto del_group_3;
		}
	}

	for (i = 0; i < bman_pool_max; i++) {
		ret = scnprintf((name_attrs_pool_count + i * 3), 3, "%d", i);
		if (!ret)
			goto del_group_4;
		dev_attr_buffer_pool_count[i].attr.name =
			(name_attrs_pool_count + i * 3);
		dev_attr_buffer_pool_count[i].attr.mode = S_IRUSR;
		dev_attr_buffer_pool_count[i].show = show_pool_count;
		bman_dev_pool_count_attributes[i] =
			&dev_attr_buffer_pool_count[i].attr;
	}
	bman_dev_pool_count_attributes[bman_pool_max] = NULL;

	bman_dev_pool_countent_grp.attrs = bman_dev_pool_count_attributes;

	ret = sysfs_create_group(&ofdev->dev.kobj, &bman_dev_pool_countent_grp);
	if (ret)
		goto del_group_4;

	goto done;

del_group_4:
	for (i = 0; i < (bman_pool_max + 1); i++)
		kfree(bman_dev_pool_count_attributes[i]);
	kfree(bman_dev_pool_count_attributes);
del_group_3:
	kfree(dev_attr_buffer_pool_count);
del_group_2:
	kfree(name_attrs_pool_count);
del_group_1:
	sysfs_remove_group(&ofdev->dev.kobj, &bman_dev_ecr_grp);
del_group_0:
	sysfs_remove_group(&ofdev->dev.kobj, &bman_dev_attr_grp);
done:
	if (ret)
		dev_err(&ofdev->dev,
			"Cannot create dev attributes ret=%d\n", ret);
	return ret;
};

static struct of_device_id of_fsl_bman_ids[] = {
	{
		.compatible = "fsl,bman",
	},
	{}
};
MODULE_DEVICE_TABLE(of, of_fsl_bman_ids);

static struct platform_driver of_fsl_bman_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = DRV_NAME,
		.of_match_table = of_fsl_bman_ids,
	},
	.probe = of_fsl_bman_probe,
	.remove = of_fsl_bman_remove,
};

static int bman_ctrl_init(void)
{
	return platform_driver_register(&of_fsl_bman_driver);
}

static void bman_ctrl_exit(void)
{
	platform_driver_unregister(&of_fsl_bman_driver);
}

module_init(bman_ctrl_init);
module_exit(bman_ctrl_exit);

#endif /* CONFIG_SYSFS */
