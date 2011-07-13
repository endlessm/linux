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

#ifndef DPA_SYS_H
#define DPA_SYS_H

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/of_platform.h>
#include <linux/kthread.h>
#include <linux/memblock.h>
#include <linux/completion.h>
#include <linux/log2.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/uio_driver.h>
#include <asm/smp.h>
#include <sysdev/fsl_soc.h>
#include <linux/fsl_hypervisor.h>
#include <linux/vmalloc.h>

/* When copying aligned words or shorts, try to avoid memcpy() */
#define CONFIG_TRY_BETTER_MEMCPY

/* This takes a "phandle" and dereferences to the cpu device-tree node,
 * returning the cpu index. Returns negative error codes. */
static inline int check_cpu_phandle(phandle ph)
{
	const u32 *cpu_val;
	struct device_node *tmp_node = of_find_node_by_phandle(ph);
	int cpu, ret;

	if (!tmp_node) {
		pr_err("Bad 'cpu-handle'\n");
		return -EINVAL;
	}
	cpu_val = of_get_property(tmp_node, "reg", &ret);
	if (!cpu_val || (ret != sizeof(*cpu_val))) {
		pr_err("Can't get %s property 'reg'\n", tmp_node->full_name);
		return -ENODEV;
	}
	for_each_present_cpu(cpu) {
		if (*cpu_val == get_hard_smp_processor_id(cpu))
			goto done;
	}
	pr_err("Invalid cpu index %d in %s\n", *cpu_val, tmp_node->full_name);
	return -ENODEV;
done:
	of_node_put(tmp_node);
	return cpu;
}

/* Handle portals destined for USDPAA (user-space).
 *
 * The UIO layer is mostly Qman/Bman-agnostic, however the rest of the driver is
 * separated along interface and implementation lines. So each Qman/Bman driver
 * instantiates a dpa_uio_class and links dpa_uio_portal objects into it. If
 * the dpa_uio driver module is built, it queries these two classes and creates
 * the UIO devices accordingly.
 */
struct dpa_uio_class {
	struct list_head list;
	const char *dev_prefix;
};
struct dpa_uio_portal {
	enum {
		dpa_uio_portal_bman,
		dpa_uio_portal_qman,
	} type;
	union {
		const struct bm_portal_config *bm_cfg;
		const struct qm_portal_config *qm_cfg;
	};
	struct list_head node;
};
const struct dpa_uio_class *dpa_uio_bman(void);
const struct dpa_uio_class *dpa_uio_qman(void);

/* These stubs are re-mapped to hypervisor+failover features in kernel trees
 * that contain that support. */
static inline int fsl_dpa_should_recover(void)
{
	return 0;
}
static inline int pamu_enable_liodn(struct device_node *n, int i)
{
	return 0;
}
/***********************/
/* Misc inline assists */
/***********************/

/* TODO: NB, we currently assume that hwsync() and lwsync() imply compiler
 * barriers and that dcb*() won't fall victim to compiler or execution
 * reordering with respect to other code/instructions that manipulate the same
 * cacheline. */
#define hwsync() \
	do { \
		__asm__ __volatile__ ("sync" : : : "memory"); \
	} while(0)
#define lwsync() \
	do { \
		__asm__ __volatile__ (stringify_in_c(LWSYNC) : : : "memory"); \
	} while(0)
#define dcbf(p) \
	do { \
		__asm__ __volatile__ ("dcbf 0,%0" : : "r" (p) : "memory"); \
	} while(0)
#define dcbt_ro(p) \
	do { \
		__asm__ __volatile__ ("dcbt 0,%0" : : "r" (p)); \
	} while(0)
#define dcbt_rw(p) \
	do { \
		__asm__ __volatile__ ("dcbtst 0,%0" : : "r" (p)); \
	} while(0)
#define dcbi(p) dcbf(p)
#ifdef CONFIG_PPC_E500MC
#define dcbzl(p) \
	do { \
		__asm__ __volatile__ ("dcbzl 0,%0" : : "r" (p)); \
	} while (0)
#define dcbz_64(p) \
	do { \
		dcbzl(p); \
	} while (0)
#define dcbf_64(p) \
	do { \
		dcbf(p); \
	} while (0)
/* Commonly used combo */
#define dcbit_ro(p) \
	do { \
		dcbi(p); \
		dcbt_ro(p); \
	} while (0)
#else
#define dcbz(p) \
	do { \
		__asm__ __volatile__ ("dcbz 0,%0" : : "r" (p)); \
	} while (0)
#define dcbz_64(p) \
	do { \
		dcbz((u32)p + 32);	\
		dcbz(p);	\
	} while (0)
#define dcbf_64(p) \
	do { \
		dcbf((u32)p + 32); \
		dcbf(p); \
	} while (0)
/* Commonly used combo */
#define dcbit_ro(p) \
	do { \
		dcbi(p); \
		dcbi((u32)p + 32); \
		dcbt_ro(p); \
		dcbt_ro((u32)p + 32); \
	} while (0)
#endif /* CONFIG_PPC_E500MC */

static inline u64 mfatb(void)
{
	u32 hi, lo, chk;
	do {
		hi = mfspr(SPRN_ATBU);
		lo = mfspr(SPRN_ATBL);
		chk = mfspr(SPRN_ATBU);
	} while (unlikely(hi != chk));
	return ((u64)hi << 32) | (u64)lo;
}

#ifdef CONFIG_FSL_DPA_CHECKING
#define DPA_ASSERT(x) \
	do { \
		if (!(x)) { \
			pr_crit("ASSERT: (%s:%d) %s\n", __FILE__, __LINE__, \
				__stringify_1(x)); \
			dump_stack(); \
			panic("assertion failure"); \
		} \
	} while(0)
#else
#define DPA_ASSERT(x)
#endif

/* memcpy() stuff - when you know alignments in advance */
#ifdef CONFIG_TRY_BETTER_MEMCPY
static inline void copy_words(void *dest, const void *src, size_t sz)
{
	u32 *__dest = dest;
	const u32 *__src = src;
	size_t __sz = sz >> 2;
	BUG_ON((unsigned long)dest & 0x3);
	BUG_ON((unsigned long)src & 0x3);
	BUG_ON(sz & 0x3);
	while (__sz--)
		*(__dest++) = *(__src++);
}
static inline void copy_shorts(void *dest, const void *src, size_t sz)
{
	u16 *__dest = dest;
	const u16 *__src = src;
	size_t __sz = sz >> 1;
	BUG_ON((unsigned long)dest & 0x1);
	BUG_ON((unsigned long)src & 0x1);
	BUG_ON(sz & 0x1);
	while (__sz--)
		*(__dest++) = *(__src++);
}
static inline void copy_bytes(void *dest, const void *src, size_t sz)
{
	u8 *__dest = dest;
	const u8 *__src = src;
	while (sz--)
		*(__dest++) = *(__src++);
}
#else
#define copy_words memcpy
#define copy_shorts memcpy
#define copy_bytes memcpy
#endif

/************/
/* RB-trees */
/************/

/* We encapsulate RB-trees so that its easier to use non-linux forms in
 * non-linux systems. This also encapsulates the extra plumbing that linux code
 * usually provides when using RB-trees. This encapsulation assumes that the
 * data type held by the tree is u32. */

struct dpa_rbtree {
	struct rb_root root;
};
#define DPA_RBTREE { .root = RB_ROOT }

static inline void dpa_rbtree_init(struct dpa_rbtree *tree)
{
	tree->root = RB_ROOT;
}

#define IMPLEMENT_DPA_RBTREE(name, type, node_field, val_field) \
static inline int name##_push(struct dpa_rbtree *tree, type *obj) \
{ \
	struct rb_node *parent = NULL, **p = &tree->root.rb_node; \
	while (*p) { \
		u32 item; \
		parent = *p; \
		item = rb_entry(parent, type, node_field)->val_field; \
		if (obj->val_field < item) \
			p = &parent->rb_left; \
		else if (obj->val_field > item) \
			p = &parent->rb_right; \
		else \
			return -EBUSY; \
	} \
	rb_link_node(&obj->node_field, parent, p); \
	rb_insert_color(&obj->node_field, &tree->root); \
	return 0; \
} \
static inline void name##_del(struct dpa_rbtree *tree, type *obj) \
{ \
	rb_erase(&obj->node_field, &tree->root); \
} \
static inline type *name##_find(struct dpa_rbtree *tree, u32 val) \
{ \
	type *ret; \
	struct rb_node *p = tree->root.rb_node; \
	while (p) { \
		ret = rb_entry(p, type, node_field); \
		if (val < ret->val_field) \
			p = p->rb_left; \
		else if (val > ret->val_field) \
			p = p->rb_right; \
		else \
			return ret; \
	} \
	return NULL; \
}

#endif /* DPA_SYS_H */

