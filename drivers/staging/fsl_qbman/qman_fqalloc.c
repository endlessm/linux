/* Copyright 2009-2011 Freescale Semiconductor, Inc.
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

#include "qman_private.h"

#include <linux/fsl_bman.h>

/****************/
/* FQ allocator */
/****************/

/* Global flag: use BPID==0 (fq_pool), or use the range-allocator? */
static int use_bman;

#ifdef CONFIG_FSL_BMAN_PORTAL
static struct bman_pool *fq_pool;
static const struct bman_pool_params fq_pool_params;
#endif

__init int fqalloc_init(int __use_bman)
{
	use_bman = __use_bman;
#ifdef CONFIG_FSL_BMAN_PORTAL
	if (use_bman) {
		fq_pool = bman_new_pool(&fq_pool_params);
		if (!fq_pool)
			return -ENOMEM;
	}
#else
	BUG_ON(use_bman);
#endif
	return 0;
}

u32 qm_fq_new(void)
{
#ifdef CONFIG_FSL_BMAN_PORTAL
	struct bm_buffer buf;
	int ret;
#endif

	if (!use_bman) {
		u32 result;
		if (qman_alloc_fqid(&result) < 0)
			return 0;
		return result;
	}
#ifdef CONFIG_FSL_BMAN_PORTAL
	BUG_ON(!fq_pool);
	ret = bman_acquire(fq_pool, &buf, 1, 0);
	if (ret != 1)
		return 0;
	return (u32)bm_buffer_get64(&buf);
#else
	BUG();
#endif
}
EXPORT_SYMBOL(qm_fq_new);

int qm_fq_free_flags(u32 fqid, __maybe_unused u32 flags)
{
#ifdef CONFIG_FSL_BMAN_PORTAL
	struct bm_buffer buf;
	u32 bflags = 0;
	int ret;
	bm_buffer_set64(&buf, fqid);
#endif

	if (!use_bman) {
		qman_release_fqid(fqid);
		return 0;
	}
#ifdef CONFIG_FSL_BMAN_PORTAL
#ifdef CONFIG_FSL_DPA_CAN_WAIT
	if (flags & QM_FQ_FREE_WAIT) {
		bflags |= BMAN_RELEASE_FLAG_WAIT;
		if (flags & BMAN_RELEASE_FLAG_WAIT_INT)
			bflags |= BMAN_RELEASE_FLAG_WAIT_INT;
		if (flags & BMAN_RELEASE_FLAG_WAIT_SYNC)
			bflags |= BMAN_RELEASE_FLAG_WAIT_SYNC;
	}
#endif
	ret = bman_release(fq_pool, &buf, 1, bflags);
	return ret;
#else
	BUG();
#endif
}
EXPORT_SYMBOL(qm_fq_free_flags);

/* Global state for the allocator */
static DEFINE_SPINLOCK(alloc_lock);
static LIST_HEAD(alloc_list);

/* The allocator is a (possibly-empty) list of these; */
struct alloc_node {
	struct list_head list;
	u32 base;
	u32 num;
};

/* #define FQRANGE_DEBUG */

#ifdef FQRANGE_DEBUG
#define DPRINT		pr_info
static void DUMP(void)
{
	int off = 0;
	char buf[256];
	struct alloc_node *p;
	list_for_each_entry(p, &alloc_list, list) {
		if (off < 255)
			off += snprintf(buf + off, 255-off, "{%d,%d}",
				p->base, p->base + p->num - 1);
	}
	pr_info("%s\n", buf);
}
#else
#define DPRINT(x...)	do { ; } while(0)
#define DUMP()		do { ; } while(0)
#endif

int qman_alloc_fqid_range(u32 *result, u32 count, u32 align, int partial)
{
	struct alloc_node *i = NULL, *next_best = NULL;
	u32 base, next_best_base = 0, num = 0, next_best_num = 0;
	struct alloc_node *margin_left, *margin_right;

	*result = (u32)-1;
	DPRINT("alloc_range(%d,%d,%d)\n", count, align, partial);
	DUMP();
	/* If 'align' is 0, it should behave as though it was 1 */
	if (!align)
		align = 1;
	margin_left = kmalloc(sizeof(*margin_left), GFP_KERNEL);
	if (!margin_left)
		goto err;
	margin_right = kmalloc(sizeof(*margin_right), GFP_KERNEL);
	if (!margin_right) {
		kfree(margin_left);
		goto err;
	}
	spin_lock_irq(&alloc_lock);
	list_for_each_entry(i, &alloc_list, list) {
		base = (i->base + align - 1) / align;
		base *= align;
		if ((base - i->base) >= i->num)
			/* alignment is impossible, regardless of count */
			continue;
		num = i->num - (base - i->base);
		if (num >= count) {
			/* this one will do nicely */
			num = count;
			goto done;
		}
		if (num > next_best_num) {
			next_best = i;
			next_best_base = base;
			next_best_num = num;
		}
	}
	if (partial && next_best) {
		i = next_best;
		base = next_best_base;
		num = next_best_num;
	} else
		i = NULL;
done:
	if (i) {
		if (base != i->base) {
			margin_left->base = i->base;
			margin_left->num = base - i->base;
			list_add_tail(&margin_left->list, &i->list);
		} else
			kfree(margin_left);
		if ((base + num) < (i->base + i->num)) {
			margin_right->base = base + num;
			margin_right->num = (i->base + i->num) -
						(base + num);
			list_add(&margin_right->list, &i->list);
		} else
			kfree(margin_right);
		list_del(&i->list);
		kfree(i);
		*result = base;
	}
	spin_unlock_irq(&alloc_lock);
err:
	DPRINT("returning %d\n", i ? num : -ENOMEM);
	DUMP();
	return i ? (int)num : -ENOMEM;
}
EXPORT_SYMBOL(qman_alloc_fqid_range);

void qman_release_fqid_range(u32 fqid, u32 count)
{
	struct alloc_node *i, *node = kmalloc(sizeof(*node), GFP_KERNEL);
	DPRINT("release_range(%d,%d)\n", fqid, count);
	DUMP();
	spin_lock_irq(&alloc_lock);
	node->base = fqid;
	node->num = count;
	list_for_each_entry(i, &alloc_list, list) {
		if (i->base >= node->base) {
			list_add_tail(&node->list, &i->list);
			goto done;
		}
	}
	list_add_tail(&node->list, &alloc_list);
done:
	/* Merge to the left */
	i = list_entry(node->list.prev, struct alloc_node, list);
	if (node->list.prev != &alloc_list) {
		BUG_ON((i->base + i->num) > node->base);
		if ((i->base + i->num) == node->base) {
			node->base = i->base;
			node->num += i->num;
			list_del(&i->list);
			kfree(i);
		}
	}
	/* Merge to the right */
	i = list_entry(node->list.next, struct alloc_node, list);
	if (node->list.next != &alloc_list) {
		BUG_ON((node->base + node->num) > i->base);
		if ((node->base + node->num) == i->base) {
			node->num += i->num;
			list_del(&i->list);
			kfree(i);
		}
	}
	spin_unlock_irq(&alloc_lock);
	DUMP();
}
EXPORT_SYMBOL(qman_release_fqid_range);

