/* Copyright 2008-2011 Freescale Semiconductor, Inc.
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

#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "pme2_private.h"
#include "pme2_regs.h"

/* PME HW Revision */
#define PME_REV(rev1_reg) (rev1_reg & 0x0000FFFF)
#define PME_REV_2_0 0x00000200
#define PME_REV_2_1 0x00000201
#define DEC1_MAX_REV_2_0 0x000FFFFC
#define DEC1_MAX_REV_2_1 0x0007FFFC


/* Driver Name is used in naming the sysfs directory
 * /sys/bus/of_platform/drivers/DRV_NAME
 */
#define DRV_NAME	"fsl-pme"

#define DEFAULT_PDSR_SZ (CONFIG_FSL_PME2_PDSRSIZE << 7)
#define DEFAULT_SRE_SZ  (CONFIG_FSL_PME2_SRESIZE << 5)
#define PDSR_TBL_ALIGN  (1 << 7)
#define SRE_TBL_ALIGN   (1 << 5)
#define DEFAULT_SRFCC   400

/* Defaults */
#define DEFAULT_DEC0_MTE   0x3FFF
#define DEFAULT_DLC_MPM    0xFFFF
#define DEFAULT_DLC_MPE    0xFFFF
/* Boot parameters */
DECLARE_GLOBAL(max_test_line_per_pat, unsigned int, uint,
		DEFAULT_DEC0_MTE,
		"Maximum allowed Test Line Executions per pattern, "
		"scaled by a factor of 8");
DECLARE_GLOBAL(max_pat_eval_per_sui, unsigned int, uint,
		DEFAULT_DLC_MPE,
		"Maximum Pattern Evaluations per SUI, scaled by a factor of 8")
DECLARE_GLOBAL(max_pat_matches_per_sui, unsigned int, uint,
		DEFAULT_DLC_MPM,
		"Maximum Pattern Matches per SUI");
/* SRE */
DECLARE_GLOBAL(sre_rule_num, unsigned int, uint,
		CONFIG_FSL_PME2_SRE_CNR,
		"Configured Number of Stateful Rules");
DECLARE_GLOBAL(sre_session_ctx_size, unsigned int, uint,
		1 << CONFIG_FSL_PME2_SRE_CTX_SIZE_PER_SESSION,
		"SRE Context Size per Session");

/************
 * Section 1
 ************
 * This code is called during kernel early-boot and could never be made
 * loadable.
 */
static dma_addr_t dxe_a, sre_a;
static size_t dxe_sz = DEFAULT_PDSR_SZ, sre_sz = DEFAULT_SRE_SZ;

/* Parse the <name> property to extract the memory location and size and
 * memblock_reserve() it. If it isn't supplied, memblock_alloc() the default size. */
static __init int parse_mem_property(struct device_node *node, const char *name,
			dma_addr_t *addr, size_t *sz, u64 align, int zero)
{
	const u32 *pint;
	int ret;

	pint = of_get_property(node, name, &ret);
	if (!pint || (ret != 16)) {
		pr_info("pme: No %s property '%s', using memblock_alloc(0x%016zx)\n",
				node->full_name, name, *sz);
		*addr = memblock_alloc(*sz, align);
		if (zero)
			memset(phys_to_virt(*addr), 0, *sz);
		return 0;
	}
	pr_info("pme: Using %s property '%s'\n", node->full_name, name);
	/* If using a "zero-pma", don't try to zero it, even if you asked */
	if (zero && of_find_property(node, "zero-pma", &ret)) {
		pr_info("  it's a 'zero-pma', not zeroing from s/w\n");
		zero = 0;
	}
	*addr = ((u64)pint[0] << 32) | (u64)pint[1];
	*sz = ((u64)pint[2] << 32) | (u64)pint[3];
	if((u64)*addr & (align - 1)) {
		pr_err("pme: Invalid alignment, address %016llx\n",(u64)*addr);
		return -EINVAL;
	}
	/* Keep things simple, it's either all in the DRAM range or it's all
	 * outside. */
	if (*addr < memblock_end_of_DRAM()) {
		if ((u64)*addr + (u64)*sz > memblock_end_of_DRAM()){
			pr_err("pme: outside DRAM range\n");
			return -EINVAL;
		}
		if (memblock_reserve(*addr, *sz) < 0) {
			pr_err("pme: Failed to reserve %s\n", name);
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

/* No errors/interrupts. Physical addresses are assumed <= 32bits. */
static int __init fsl_pme2_init(struct device_node *node)
{
	const char *s;
	int ret = 0;

	s = of_get_property(node, "fsl,hv-claimable", &ret);
	if (s && !strcmp(s, "standby")) {
		pr_info("  -> in standby mode\n");
		return 0;
	}
	/* Check if pdsr memory already allocated */
	if (dxe_a) {
		pr_err("pme: Error fsl_pme2_init already done\n");
		return -EINVAL;
	}
	ret = parse_mem_property(node, "fsl,pme-pdsr", &dxe_a, &dxe_sz,
			PDSR_TBL_ALIGN, 0);
	if (ret)
		return ret;
	ret = parse_mem_property(node, "fsl,pme-sre", &sre_a, &sre_sz,
			SRE_TBL_ALIGN, 0);
	return ret;
}

__init void pme2_init_early(void)
{
	struct device_node *dn;
	int ret;
	for_each_compatible_node(dn, NULL, "fsl,pme") {
		ret = fsl_pme2_init(dn);
		if (ret)
			pr_err("pme: Error fsl_pme2_init\n");
	}
}

/************
 * Section 2
 ***********
 * This code is called during driver initialisation. It doesn't do anything with
 * the device-tree entries nor the PME device, it simply creates the sysfs stuff
 * and gives the user something to hold. This could be made loadable, if there
 * was any benefit to doing so - but as the device is already "bound" by static
 * code, there's little point to hiding the fact.
 */

MODULE_AUTHOR("Geoff Thorpe");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FSL PME2 (p4080) device control");

/* Opaque pointer target used to represent the PME CCSR map, ... */
struct pme;

/* ... and the instance of it. */
static struct pme *global_pme;
static int pme_err_irq;

static inline void __pme_out(struct pme *p, u32 offset, u32 val)
{
	u32 __iomem *regs = (void *)p;
	out_be32(regs + (offset >> 2), val);
}
#define pme_out(p, r, v) __pme_out(p, PME_REG_##r, v)
static inline u32 __pme_in(struct pme *p, u32 offset)
{
	u32 __iomem *regs = (void *)p;
	return in_be32(regs + (offset >> 2));
}
#define pme_in(p, r) __pme_in(p, PME_REG_##r)

#define PME_EFQC(en, fq) \
	({ \
		/* Assume a default delay of 64 cycles */ \
		u8 __i419 = 0x1; \
		u32 __fq419 = (fq) & 0x00ffffff; \
		((en) ? 0x80000000 : 0) | (__i419 << 28) | __fq419; \
	})

#define PME_FACONF_ENABLE   0x00000002
#define PME_FACONF_RESET    0x00000001

/* pme stats accumulator work */
static void accumulator_update(struct work_struct *work);
void accumulator_update_interval(u32 interval);
static DECLARE_DELAYED_WORK(accumulator_work, accumulator_update);
u32 pme_stat_interval = CONFIG_FSL_PME2_STAT_ACCUMULATOR_UPDATE_INTERVAL;
#define PME_SBE_ERR 0x01000000
#define PME_DBE_ERR 0x00080000
#define PME_PME_ERR 0x00000100
#define PME_ALL_ERR (PME_SBE_ERR | PME_DBE_ERR | PME_PME_ERR)

static struct of_device_id of_fsl_pme_ids[] = {
	{
		.compatible = "fsl,pme",
	},
	{}
};
MODULE_DEVICE_TABLE(of, of_fsl_pme_ids);

/* Pme interrupt handler */
static irqreturn_t pme_isr(int irq, void *ptr)
{
	static u32 last_isrstate;
	u32 isrstate = pme_in(global_pme, ISR) ^ last_isrstate;

	/* What new ISR state has been raise */
	if (!isrstate)
		return IRQ_NONE;
	if (isrstate & PME_SBE_ERR)
		pr_crit("PME: SBE detected\n");
	if (isrstate & PME_DBE_ERR)
		pr_crit("PME: DBE detected\n");
	if (isrstate & PME_PME_ERR)
		pr_crit("PME: PME serious detected\n");
	/* Clear the ier interrupt bit */
	last_isrstate |= isrstate;
	pme_out(global_pme, IER, ~last_isrstate);
	return IRQ_HANDLED;
}

static int of_fsl_pme_remove(struct platform_device *ofdev)
{
	/* Cancel pme accumulator */
	accumulator_update_interval(0);
	cancel_delayed_work_sync(&accumulator_work);
	/* Disable PME..TODO need to wait till it's quiet */
	pme_out(global_pme, FACONF, PME_FACONF_RESET);
	/* Release interrupt */
	if (likely(pme_err_irq != NO_IRQ))
		free_irq(pme_err_irq, &ofdev->dev);
	/* Remove sysfs attribute */
	pme2_remove_sysfs_dev_files(ofdev);
	/* Unmap controller region */
	iounmap(global_pme);
	global_pme = NULL;
	return 0;
}

static int of_fsl_pme_probe(struct platform_device *ofdev)
{
	int ret, err = 0;
	void __iomem *regs;
	struct device *dev = &ofdev->dev;
	struct device_node *nprop = dev->of_node;
	u32 clkfreq = DEFAULT_SRFCC * 1000000;
	const u32 *value;
	const char *s;
	int srec_aim = 0, srec_esr = 0;
	u32 srecontextsize_code;
	u32 dec1;

	/* TODO: This standby handling won't work properly after failover, it's
	 * just to allow bring up for now. */
	s = of_get_property(nprop, "fsl,hv-claimable", &ret);
	if (s && !strcmp(s, "standby"))
		return 0;
	pme_err_irq = of_irq_to_resource(nprop, 0, NULL);
	if (unlikely(pme_err_irq == NO_IRQ))
		dev_warn(dev, "Can't get %s property '%s'\n", nprop->full_name,
			 "interrupts");

	/* Get configuration properties from device tree */
	/* First, get register page */
	regs = of_iomap(nprop, 0);
	if (regs == NULL) {
		dev_err(dev, "of_iomap() failed\n");
		err = -EINVAL;
		goto out;
	}

	/* Global configuration, leave pme disabled */
	global_pme = (struct pme *)regs;
	pme_out(global_pme, FACONF, 0);
	pme_out(global_pme, EFQC, PME_EFQC(0, 0));

	/* TODO: these coherency settings for PMFA, DXE, and SRE force all
	 * transactions to snoop, as the kernel does not yet support flushing in
	 * dma_map_***() APIs (ie. h/w can not treat otherwise coherent memory
	 * in a non-coherent manner, temporarily or otherwise). When the kernel
	 * supports this, we should tune these settings back to;
	 *     FAMCR = 0x00010001
	 *      DMCR = 0x00000000
	 *      SMCR = 0x00000000
	 */
	/* PME HW rev 2.1: Added TWC field in FAMCR */
	pme_out(global_pme, FAMCR, 0x11010101);
	pme_out(global_pme, DMCR, 0x00000001);
	pme_out(global_pme, SMCR, 0x00000211);

	if (likely(pme_err_irq != NO_IRQ)) {
		/* Register the pme ISR handler */
		err = request_irq(pme_err_irq, pme_isr, IRQF_SHARED, "pme-err",
				  dev);
		if (err) {
			dev_err(dev, "request_irq() failed\n");
			goto out_unmap_ctrl_region;
		}
	}

#ifdef CONFIG_FSL_PME2_SRE_AIM
	srec_aim = 1;
#endif
#ifdef CONFIG_FSL_PME2_SRE_ESR
	srec_esr = 1;
#endif
	/* Validate some parameters */
	if (!sre_session_ctx_size || !is_power_of_2(sre_session_ctx_size) ||
			(sre_session_ctx_size < 32) ||
			(sre_session_ctx_size > (131072))) {
		dev_err(dev, "invalid sre_session_ctx_size\n");
		err = -EINVAL;
		goto out_free_irq;
	}
	srecontextsize_code = ilog2(sre_session_ctx_size);
	srecontextsize_code -= 4;

	/* Configure Clock Frequency */
	value = of_get_property(nprop, "clock-frequency", NULL);
	if (value)
		clkfreq = *value;
	pme_out(global_pme, SFRCC, DIV_ROUND_UP(clkfreq, 1000000));

	pme_out(global_pme, PDSRBAH, upper_32_bits(dxe_a));
	pme_out(global_pme, PDSRBAL, lower_32_bits(dxe_a));
	pme_out(global_pme, SCBARH, upper_32_bits(sre_a));
	pme_out(global_pme, SCBARL, lower_32_bits(sre_a));
	/* Maximum allocated index into the PDSR table available to the DXE
	 * Rev 2.0: Max 0xF_FFFC
	 * Rev 2.1: Max 0x7_FFFC
	 */
	if (PME_REV(pme_in(global_pme, PM_IP_REV1)) == PME_REV_2_0) {
		if (((dxe_sz/PDSR_TBL_ALIGN)-1) > DEC1_MAX_REV_2_0)
			dec1 = DEC1_MAX_REV_2_0;
		else
			dec1 = (dxe_sz/PDSR_TBL_ALIGN)-1;
	} else {
		if (((dxe_sz/PDSR_TBL_ALIGN)-1) > DEC1_MAX_REV_2_1)
			dec1 = DEC1_MAX_REV_2_1;
		else
			dec1 = (dxe_sz/PDSR_TBL_ALIGN)-1;
	}
	pme_out(global_pme, DEC1, dec1);
	/* Maximum allocated index into the PDSR table available to the SRE */
	pme_out(global_pme, SEC2, dec1);
	/* Maximum allocated 32-byte offset into SRE Context Table.*/
	if (sre_sz)
		pme_out(global_pme, SEC3, (sre_sz/SRE_TBL_ALIGN)-1);
	/* Max test line execution */
	pme_out(global_pme, DEC0, max_test_line_per_pat);
	pme_out(global_pme, DLC,
		(max_pat_eval_per_sui << 16) | max_pat_matches_per_sui);

	/* SREC - SRE Config */
	pme_out(global_pme, SREC,
		/* Number of rules in database */
		(sre_rule_num << 0) |
		/* Simple Report Enabled */
		((srec_esr ? 1 : 0) << 18) |
		/* Context Size per Session */
		(srecontextsize_code << 19) |
		/* Alternate Inclusive Mode */
		((srec_aim ? 1 : 0) << 29));
	pme_out(global_pme, SEC1,
		(CONFIG_FSL_PME2_SRE_MAX_INSTRUCTION_LIMIT << 16) |
		CONFIG_FSL_PME2_SRE_MAX_BLOCK_NUMBER);

	/* Setup Accumulator */
	if (pme_stat_interval)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(pme_stat_interval));
	/* Create sysfs entries */
	err = pme2_create_sysfs_dev_files(ofdev);
	if (err)
		goto out_stop_accumulator;

	/* Enable interrupts */
	pme_out(global_pme, IER, PME_ALL_ERR);
	dev_info(dev, "ver: 0x%08x\n", pme_in(global_pme, PM_IP_REV1));

	/* Enable pme */
	pme_out(global_pme, FACONF, PME_FACONF_ENABLE);
	return 0;

out_stop_accumulator:
	if (pme_stat_interval) {
		accumulator_update_interval(0);
		cancel_delayed_work_sync(&accumulator_work);
	}
out_free_irq:
	if (likely(pme_err_irq != NO_IRQ))
		free_irq(pme_err_irq, &ofdev->dev);
out_unmap_ctrl_region:
	pme_out(global_pme, FACONF, PME_FACONF_RESET);
	iounmap(global_pme);
	global_pme = NULL;
out:
	return err;
}

static struct platform_driver of_fsl_pme_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = DRV_NAME,
		.of_match_table = of_fsl_pme_ids,
	},
	.probe = of_fsl_pme_probe,
	.remove      = of_fsl_pme_remove,
};

static int pme2_ctrl_init(void)
{
	return platform_driver_register(&of_fsl_pme_driver);
}

static void pme2_ctrl_exit(void)
{
	platform_driver_unregister(&of_fsl_pme_driver);
}

module_init(pme2_ctrl_init);
module_exit(pme2_ctrl_exit);

/************
 * Section 3
 ************
 * These APIs are the only functional hooks into the control driver, besides the
 * sysfs attributes.
 */

int pme2_have_control(void)
{
	return global_pme ? 1 : 0;
}
EXPORT_SYMBOL(pme2_have_control);

int pme2_exclusive_set(struct qman_fq *fq)
{
	if (!pme2_have_control())
		return -ENODEV;
	pme_out(global_pme, EFQC, PME_EFQC(1, qman_fq_fqid(fq)));
	return 0;
}
EXPORT_SYMBOL(pme2_exclusive_set);

int pme2_exclusive_unset(void)
{
	if (!pme2_have_control())
		return -ENODEV;
	pme_out(global_pme, EFQC, PME_EFQC(0, 0));
	return 0;
}
EXPORT_SYMBOL(pme2_exclusive_unset);

int pme_attr_set(enum pme_attr attr, u32 val)
{
	u32 mask;
	u32 attr_val;

	if (!pme2_have_control())
		return -ENODEV;

	/* Check if Buffer size configuration */
	if (attr >= pme_attr_bsc_first && attr <= pme_attr_bsc_last) {
		u32 bsc_pool_id = attr - pme_attr_bsc_first;
		u32 bsc_pool_offset = bsc_pool_id % 8;
		u32 bsc_pool_mask = ~(0xF << ((7-bsc_pool_offset)*4));
		/* range for val 0..0xB */
		if (val > 0xb)
			return -EINVAL;
		/* calculate which sky-blue reg */
		/* 0..7 -> bsc_(0..7), PME_REG_BSC0 */
		/* 8..15 -> bsc_(8..15) PME_REG_BSC1*/
		/* ... */
		/* 56..63 -> bsc_(56..63) PME_REG_BSC7*/
		attr_val = pme_in(global_pme, BSC0 + ((bsc_pool_id/8)*4));
		/* Now mask in the new value */
		attr_val = attr_val & bsc_pool_mask;
		attr_val = attr_val | (val << ((7-bsc_pool_offset)*4));
		pme_out(global_pme,  BSC0 + ((bsc_pool_id/8)*4), attr_val);
		return 0;
	}

	switch (attr) {
	case pme_attr_efqc_int:
		if (val > 4)
			return -EINVAL;
		mask = 0x8FFFFFFF;
		attr_val = pme_in(global_pme, EFQC);
		/* clear efqc_int */
		attr_val &= mask;
		val <<= 28;
		val |= attr_val;
		pme_out(global_pme, EFQC, val);
		break;

	case pme_attr_sw_db:
		pme_out(global_pme, SWDB, val);
		break;

	case pme_attr_dmcr:
		pme_out(global_pme, DMCR, val);
		break;

	case pme_attr_smcr:
		pme_out(global_pme, SMCR, val);
		break;

	case pme_attr_famcr:
		pme_out(global_pme, FAMCR, val);
		break;

	case pme_attr_kvlts:
		if (val < 2 || val > 16)
			return -EINVAL;
		/* HW range: 1..15, SW range: 2..16 */
		pme_out(global_pme, KVLTS, --val);
		break;

	case pme_attr_max_chain_length:
		if (val > 0x7FFF)
			val = 0x7FFF;
		pme_out(global_pme, KEC, val);
		break;

	case pme_attr_pattern_range_counter_idx:
		if (val > 0x1FFFF)
			val = 0x1FFFF;
		pme_out(global_pme, DRCIC, val);
		break;

	case pme_attr_pattern_range_counter_mask:
		if (val > 0x1FFFF)
			val = 0x1FFFF;
		pme_out(global_pme, DRCMC, val);
		break;

	case pme_attr_max_allowed_test_line_per_pattern:
		if (val > 0x3FFF)
			val = 0x3FFF;
		pme_out(global_pme, DEC0, val);
		break;

	case pme_attr_max_pattern_matches_per_sui:
		/* mpe, mpm */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0xFFFF0000;
		attr_val = pme_in(global_pme, DLC);
		/* clear mpm */
		attr_val &= mask;
		val &= ~mask;
		val |= attr_val;
		pme_out(global_pme, DLC, val);
		break;

	case pme_attr_max_pattern_evaluations_per_sui:
		/* mpe, mpm */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0x0000FFFF;
		attr_val = pme_in(global_pme, DLC);
		/* clear mpe */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= mask;
		val <<= 16;
		val |= attr_val;
		pme_out(global_pme, DLC, val);
		break;

	case pme_attr_report_length_limit:
		if (val > 0xFFFF)
			val = 0xFFFF;
		pme_out(global_pme, RLL, val);
		break;

	case pme_attr_end_of_simple_sui_report:
		/* bit 13 */
		mask = 0x00040000;
		attr_val = pme_in(global_pme, SREC);
		if (val)
			attr_val |= mask;
		else
			attr_val &= ~mask;
		pme_out(global_pme, SREC, attr_val);
		break;

	case pme_attr_aim:
		/* bit 2 */
		mask = 0x20000000;
		attr_val = pme_in(global_pme, SREC);
		if (val)
			attr_val |= mask;
		else
			attr_val &= ~mask;
		pme_out(global_pme, SREC, attr_val);
		break;

	case pme_attr_end_of_sui_reaction_ptr:
		if (val > 0xFFFFF)
			val = 0xFFFFF;
		pme_out(global_pme, ESRP, val);
		break;

	case pme_attr_sre_pscl:
		pme_out(global_pme, SFRCC, val);
		break;

	case pme_attr_sre_max_block_num:
		/* bits 17..31 */
		if (val > 0x7FFF)
			val = 0x7FFF;
		mask = 0xFFFF8000;
		attr_val = pme_in(global_pme, SEC1);
		/* clear mbn */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= ~mask;
		val |= attr_val;
		pme_out(global_pme, SEC1, val);
		break;

	case pme_attr_sre_max_instruction_limit:
		/* bits 0..15 */
		if (val > 0xFFFF)
			val = 0xFFFF;
		mask = 0x0000FFFF;
		attr_val = pme_in(global_pme, SEC1);
		/* clear mil */
		attr_val &= mask;
		/* clear unwanted bits in val*/
		val &= mask;
		val <<= 16;
		val |= attr_val;
		pme_out(global_pme, SEC1, val);
		break;

	case pme_attr_srrv0:
		pme_out(global_pme, SRRV0, val);
		break;
	case pme_attr_srrv1:
		pme_out(global_pme, SRRV1, val);
		break;
	case pme_attr_srrv2:
		pme_out(global_pme, SRRV2, val);
		break;
	case pme_attr_srrv3:
		pme_out(global_pme, SRRV3, val);
		break;
	case pme_attr_srrv4:
		pme_out(global_pme, SRRV4, val);
		break;
	case pme_attr_srrv5:
		pme_out(global_pme, SRRV5, val);
		break;
	case pme_attr_srrv6:
		pme_out(global_pme, SRRV6, val);
		break;
	case pme_attr_srrv7:
		pme_out(global_pme, SRRV7, val);
		break;
	case pme_attr_srrfi:
		pme_out(global_pme, SRRFI, val);
		break;
	case pme_attr_srri:
		pme_out(global_pme, SRRI, val);
		break;
	case pme_attr_srrwc:
		pme_out(global_pme, SRRWC, val);
		break;
	case pme_attr_srrr:
		pme_out(global_pme, SRRR, val);
		break;
	case pme_attr_tbt0ecc1th:
		pme_out(global_pme, TBT0ECC1TH, val);
		break;
	case pme_attr_tbt1ecc1th:
		pme_out(global_pme, TBT1ECC1TH, val);
		break;
	case pme_attr_vlt0ecc1th:
		pme_out(global_pme, VLT0ECC1TH, val);
		break;
	case pme_attr_vlt1ecc1th:
		pme_out(global_pme, VLT1ECC1TH, val);
		break;
	case pme_attr_cmecc1th:
		pme_out(global_pme, CMECC1TH, val);
		break;
	case pme_attr_dxcmecc1th:
		pme_out(global_pme, DXCMECC1TH, val);
		break;
	case pme_attr_dxemecc1th:
		pme_out(global_pme, DXEMECC1TH, val);
		break;
	case pme_attr_esr:
		pme_out(global_pme, ESR, val);
		break;
	case pme_attr_pehd:
		pme_out(global_pme, PEHD, val);
		break;
	case pme_attr_ecc1bes:
		pme_out(global_pme, ECC1BES, val);
		break;
	case pme_attr_ecc2bes:
		pme_out(global_pme, ECC2BES, val);
		break;
	case pme_attr_miace:
		pme_out(global_pme, MIA_CE, val);
		break;
	case pme_attr_miacr:
		pme_out(global_pme, MIA_CR, val);
		break;
	case pme_attr_cdcr:
		pme_out(global_pme, CDCR, val);
		break;
	case pme_attr_pmtr:
		pme_out(global_pme, PMTR, val);
		break;

	default:
		pr_err("pme: Unknown attr %u\n", attr);
		return -EINVAL;
	};
	return 0;
}
EXPORT_SYMBOL(pme_attr_set);

int pme_attr_get(enum pme_attr attr, u32 *val)
{
	u32 mask;
	u32 attr_val;

	if (!pme2_have_control())
		return -ENODEV;

	/* Check if Buffer size configuration */
	if (attr >= pme_attr_bsc_first && attr <= pme_attr_bsc_last) {
		u32 bsc_pool_id = attr - pme_attr_bsc_first;
		u32 bsc_pool_offset = bsc_pool_id % 8;
		/* calculate which sky-blue reg */
		/* 0..7 -> bsc_(0..7), PME_REG_BSC0 */
		/* 8..15 -> bsc_(8..15) PME_REG_BSC1*/
		/* ... */
		/* 56..63 -> bsc_(56..63) PME_REG_BSC7*/
		attr_val = pme_in(global_pme, BSC0 + ((bsc_pool_id/8)*4));
		attr_val = attr_val >> ((7-bsc_pool_offset)*4);
		attr_val = attr_val & 0x0000000F;
		*val = attr_val;
		return 0;
	}

	switch (attr) {
	case pme_attr_efqc_int:
		mask = 0x8FFFFFFF;
		attr_val = pme_in(global_pme, EFQC);
		attr_val &= ~mask;
		attr_val >>= 28;
		break;

	case pme_attr_sw_db:
		attr_val = pme_in(global_pme, SWDB);
		break;

	case pme_attr_dmcr:
		attr_val = pme_in(global_pme, DMCR);
		break;

	case pme_attr_smcr:
		attr_val = pme_in(global_pme, SMCR);
		break;

	case pme_attr_famcr:
		attr_val = pme_in(global_pme, FAMCR);
		break;

	case pme_attr_kvlts:
		/* bit 28-31 */
		attr_val = pme_in(global_pme, KVLTS);
		attr_val &= 0x0000000F;
		/* HW range: 1..15, SW range: 2..16 */
		attr_val += 1;
		break;

	case pme_attr_max_chain_length:
		/* bit 17-31 */
		attr_val = pme_in(global_pme, KEC);
		attr_val &= 0x00007FFF;
		break;

	case pme_attr_pattern_range_counter_idx:
		/* bit 15-31 */
		attr_val = pme_in(global_pme, DRCIC);
		attr_val &= 0x0001FFFF;
		break;

	case pme_attr_pattern_range_counter_mask:
		/* bit 15-31 */
		attr_val = pme_in(global_pme, DRCMC);
		attr_val &= 0x0001FFFF;
		break;

	case pme_attr_max_allowed_test_line_per_pattern:
		/* bit 18-31 */
		attr_val = pme_in(global_pme, DEC0);
		attr_val &= 0x00003FFF;
		break;

	case pme_attr_max_pdsr_index:
		/* bit 12-31 */
		attr_val = pme_in(global_pme, DEC1);
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_max_pattern_matches_per_sui:
		attr_val = pme_in(global_pme, DLC);
		attr_val &= 0x0000FFFF;
		break;

	case pme_attr_max_pattern_evaluations_per_sui:
		attr_val = pme_in(global_pme, DLC);
		attr_val >>= 16;
		break;

	case pme_attr_report_length_limit:
		attr_val = pme_in(global_pme, RLL);
		/* clear unwanted bits in val*/
		attr_val &= 0x0000FFFF;
		break;

	case pme_attr_end_of_simple_sui_report:
		/* bit 13 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 18;
		/* clear unwanted bits in val*/
		attr_val &= 0x00000001;
		break;

	case pme_attr_aim:
		/* bit 2 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 29;
		/* clear unwanted bits in val*/
		attr_val &= 0x00000001;
		break;

	case pme_attr_sre_context_size:
		/* bits 9..12 */
		attr_val = pme_in(global_pme, SREC);
		attr_val >>= 19;
		/* clear unwanted bits in val*/
		attr_val &= 0x0000000F;
		attr_val += 4;
		attr_val = 1 << attr_val;
		break;

	case pme_attr_sre_rule_num:
		/* bits 24..31 */
		attr_val = pme_in(global_pme, SREC);
		/* clear unwanted bits in val*/
		attr_val &= 0x000000FF;
		/* Multiply by 256 */
		attr_val <<= 8;
		break;

	case pme_attr_sre_session_ctx_num: {
			u32 ctx_sz = 0;
			/* = sre_table_size / sre_session_ctx_size */
			attr_val = pme_in(global_pme, SEC3);
			/* clear unwanted bits in val*/
			attr_val &= 0x07FFFFFF;
			attr_val += 1;
			attr_val *= 32;
			ctx_sz = pme_in(global_pme, SREC);
			ctx_sz >>= 19;
			/* clear unwanted bits in val*/
			ctx_sz &= 0x0000000F;
			ctx_sz += 4;
			attr_val /= (1 << ctx_sz);
		}
		break;

	case pme_attr_end_of_sui_reaction_ptr:
		/* bits 12..31 */
		attr_val = pme_in(global_pme, ESRP);
		/* clear unwanted bits in val*/
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_sre_pscl:
		/* bits 22..31 */
		attr_val = pme_in(global_pme, SFRCC);
		break;

	case pme_attr_sre_max_block_num:
		/* bits 17..31 */
		attr_val = pme_in(global_pme, SEC1);
		/* clear unwanted bits in val*/
		attr_val &= 0x00007FFF;
		break;

	case pme_attr_sre_max_instruction_limit:
		/* bits 0..15 */
		attr_val = pme_in(global_pme, SEC1);
		attr_val >>= 16;
		break;

	case pme_attr_sre_max_index_size:
		/* bits 12..31 */
		attr_val = pme_in(global_pme, SEC2);
		/* clear unwanted bits in val*/
		attr_val &= 0x000FFFFF;
		break;

	case pme_attr_sre_max_offset_ctrl:
		/* bits 5..31 */
		attr_val = pme_in(global_pme, SEC3);
		/* clear unwanted bits in val*/
		attr_val &= 0x07FFFFFF;
		break;

	case pme_attr_src_id:
		/* bits 24..31 */
		attr_val = pme_in(global_pme, SRCIDR);
		/* clear unwanted bits in val*/
		attr_val &= 0x000000FF;
		break;

	case pme_attr_liodnr:
		/* bits 20..31 */
		attr_val = pme_in(global_pme, LIODNR);
		/* clear unwanted bits in val*/
		attr_val &= 0x00000FFF;
		break;

	case pme_attr_rev1:
		/* bits 0..31 */
		attr_val = pme_in(global_pme, PM_IP_REV1);
		break;

	case pme_attr_rev2:
		/* bits 0..31 */
		attr_val = pme_in(global_pme, PM_IP_REV2);
		break;

	case pme_attr_srrr:
		attr_val = pme_in(global_pme, SRRR);
		break;

	case pme_attr_trunci:
		attr_val = pme_in(global_pme, TRUNCI);
		break;

	case pme_attr_rbc:
		attr_val = pme_in(global_pme, RBC);
		break;

	case pme_attr_tbt0ecc1ec:
		attr_val = pme_in(global_pme, TBT0ECC1EC);
		break;

	case pme_attr_tbt1ecc1ec:
		attr_val = pme_in(global_pme, TBT1ECC1EC);
		break;

	case pme_attr_vlt0ecc1ec:
		attr_val = pme_in(global_pme, VLT0ECC1EC);
		break;

	case pme_attr_vlt1ecc1ec:
		attr_val = pme_in(global_pme, VLT1ECC1EC);
		break;

	case pme_attr_cmecc1ec:
		attr_val = pme_in(global_pme, CMECC1EC);
		break;

	case pme_attr_dxcmecc1ec:
		attr_val = pme_in(global_pme, DXCMECC1EC);
		break;

	case pme_attr_dxemecc1ec:
		attr_val = pme_in(global_pme, DXEMECC1EC);
		break;

	case pme_attr_tbt0ecc1th:
		attr_val = pme_in(global_pme, TBT0ECC1TH);
		break;

	case pme_attr_tbt1ecc1th:
		attr_val = pme_in(global_pme, TBT1ECC1TH);
		break;

	case pme_attr_vlt0ecc1th:
		attr_val = pme_in(global_pme, VLT0ECC1TH);
		break;

	case pme_attr_vlt1ecc1th:
		attr_val = pme_in(global_pme, VLT1ECC1TH);
		break;

	case pme_attr_cmecc1th:
		attr_val = pme_in(global_pme, CMECC1TH);
		break;

	case pme_attr_dxcmecc1th:
		attr_val = pme_in(global_pme, DXCMECC1TH);
		break;

	case pme_attr_dxemecc1th:
		attr_val = pme_in(global_pme, DXEMECC1TH);
		break;

	case pme_attr_stnib:
		attr_val = pme_in(global_pme, STNIB);
		break;

	case pme_attr_stnis:
		attr_val = pme_in(global_pme, STNIS);
		break;

	case pme_attr_stnth1:
		attr_val = pme_in(global_pme, STNTH1);
		break;

	case pme_attr_stnth2:
		attr_val = pme_in(global_pme, STNTH2);
		break;

	case pme_attr_stnthv:
		attr_val = pme_in(global_pme, STNTHV);
		break;

	case pme_attr_stnths:
		attr_val = pme_in(global_pme, STNTHS);
		break;

	case pme_attr_stnch:
		attr_val = pme_in(global_pme, STNCH);
		break;

	case pme_attr_stnpm:
		attr_val = pme_in(global_pme, STNPM);
		break;

	case pme_attr_stns1m:
		attr_val = pme_in(global_pme, STNS1M);
		break;

	case pme_attr_stnpmr:
		attr_val = pme_in(global_pme, STNPMR);
		break;

	case pme_attr_stndsr:
		attr_val = pme_in(global_pme, STNDSR);
		break;

	case pme_attr_stnesr:
		attr_val = pme_in(global_pme, STNESR);
		break;

	case pme_attr_stns1r:
		attr_val = pme_in(global_pme, STNS1R);
		break;

	case pme_attr_stnob:
		attr_val = pme_in(global_pme, STNOB);
		break;

	case pme_attr_mia_byc:
		attr_val = pme_in(global_pme, MIA_BYC);
		break;

	case pme_attr_mia_blc:
		attr_val = pme_in(global_pme, MIA_BLC);
		break;

	case pme_attr_isr:
		attr_val = pme_in(global_pme, ISR);
		break;

	case pme_attr_ecr0:
		attr_val = pme_in(global_pme, ECR0);
		break;

	case pme_attr_ecr1:
		attr_val = pme_in(global_pme, ECR1);
		break;

	case pme_attr_esr:
		attr_val = pme_in(global_pme, ESR);
		break;

	case pme_attr_pmstat:
		attr_val = pme_in(global_pme, PMSTAT);
		break;

	case pme_attr_pehd:
		attr_val = pme_in(global_pme, PEHD);
		break;

	case pme_attr_ecc1bes:
		attr_val = pme_in(global_pme, ECC1BES);
		break;

	case pme_attr_ecc2bes:
		attr_val = pme_in(global_pme, ECC2BES);
		break;

	case pme_attr_eccaddr:
		attr_val = pme_in(global_pme, ECCADDR);
		break;

	case pme_attr_ecccode:
		attr_val = pme_in(global_pme, ECCCODE);
		break;

	case pme_attr_miace:
		attr_val = pme_in(global_pme, MIA_CE);
		break;

	case pme_attr_miacr:
		attr_val = pme_in(global_pme, MIA_CR);
		break;

	case pme_attr_cdcr:
		attr_val = pme_in(global_pme, CDCR);
		break;

	case pme_attr_pmtr:
		attr_val = pme_in(global_pme, PMTR);
		break;

	case pme_attr_faconf:
		attr_val = pme_in(global_pme, FACONF);
		break;

	case pme_attr_pdsrbah:
		attr_val = pme_in(global_pme, PDSRBAH);
		break;

	case pme_attr_pdsrbal:
		attr_val = pme_in(global_pme, PDSRBAL);
		break;

	case pme_attr_scbarh:
		attr_val = pme_in(global_pme, SCBARH);
		break;

	case pme_attr_scbarl:
		attr_val = pme_in(global_pme, SCBARL);
		break;

	case pme_attr_srrv0:
		attr_val = pme_in(global_pme, SRRV0);
		break;

	case pme_attr_srrv1:
		attr_val = pme_in(global_pme, SRRV1);
		break;

	case pme_attr_srrv2:
		attr_val = pme_in(global_pme, SRRV2);
		break;

	case pme_attr_srrv3:
		attr_val = pme_in(global_pme, SRRV3);
		break;

	case pme_attr_srrv4:
		attr_val = pme_in(global_pme, SRRV4);
		break;

	case pme_attr_srrv5:
		attr_val = pme_in(global_pme, SRRV5);
		break;

	case pme_attr_srrv6:
		attr_val = pme_in(global_pme, SRRV6);
		break;

	case pme_attr_srrv7:
		attr_val = pme_in(global_pme, SRRV7);
		break;

	case pme_attr_srrfi:
		attr_val = pme_in(global_pme, SRRFI);
		break;

	case pme_attr_srri:
		attr_val = pme_in(global_pme, SRRI);
		break;

	case pme_attr_srrwc:
		attr_val = pme_in(global_pme, SRRWC);
		break;

	default:
		pr_err("pme: Unknown attr %u\n", attr);
		return -EINVAL;
	};
	*val = attr_val;
	return 0;
}
EXPORT_SYMBOL(pme_attr_get);

static enum pme_attr stat_list[] = {
	pme_attr_trunci,
	pme_attr_rbc,
	pme_attr_tbt0ecc1ec,
	pme_attr_tbt1ecc1ec,
	pme_attr_vlt0ecc1ec,
	pme_attr_vlt1ecc1ec,
	pme_attr_cmecc1ec,
	pme_attr_dxcmecc1ec,
	pme_attr_dxemecc1ec,
	pme_attr_stnib,
	pme_attr_stnis,
	pme_attr_stnth1,
	pme_attr_stnth2,
	pme_attr_stnthv,
	pme_attr_stnths,
	pme_attr_stnch,
	pme_attr_stnpm,
	pme_attr_stns1m,
	pme_attr_stnpmr,
	pme_attr_stndsr,
	pme_attr_stnesr,
	pme_attr_stns1r,
	pme_attr_stnob,
	pme_attr_mia_byc,
	pme_attr_mia_blc
};

static u64 pme_stats[sizeof(stat_list)/sizeof(enum pme_attr)];
static DEFINE_SPINLOCK(stat_lock);

int pme_stat_get(enum pme_attr stat, u64 *value, int reset)
{
	int i, ret = 0;
	int value_set = 0;
	u32 val;

	spin_lock_irq(&stat_lock);
	for (i = 0; i < sizeof(stat_list)/sizeof(enum pme_attr); i++) {
		if (stat_list[i] == stat) {
			ret = pme_attr_get(stat_list[i], &val);
			/* Do I need to check ret */
			pme_stats[i] += val;
			*value = pme_stats[i];
			value_set = 1;
			if (reset)
				pme_stats[i] = 0;
			break;
		}
	}
	if (!value_set) {
		pr_err("pme: Invalid stat request %d\n", stat);
		ret = -EINVAL;
	}
	spin_unlock_irq(&stat_lock);
	return ret;
}
EXPORT_SYMBOL(pme_stat_get);

void accumulator_update_interval(u32 interval)
{
	int schedule = 0;

	spin_lock_irq(&stat_lock);
	if (!pme_stat_interval && interval)
		schedule = 1;
	pme_stat_interval = interval;
	spin_unlock_irq(&stat_lock);
	if (schedule)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(interval));
}

static void accumulator_update(struct work_struct *work)
{
	int i, ret;
	u32 local_interval;
	u32 val;

	spin_lock_irq(&stat_lock);
	local_interval = pme_stat_interval;
	for (i = 0; i < sizeof(stat_list)/sizeof(enum pme_attr); i++) {
		ret = pme_attr_get(stat_list[i], &val);
		pme_stats[i] += val;
	}
	spin_unlock_irq(&stat_lock);
	if (local_interval)
		schedule_delayed_work(&accumulator_work,
				msecs_to_jiffies(local_interval));
}

