/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * AppArmor security module
 *
 * This file contains AppArmor file mediation function definitions.
 *
 * Copyright 2022 Canonical Ltd.
 */

#ifndef __AA_INODE_H
#define __AA_INODE_H

#include <linux/spinlock.h>

#include "lib.h"

struct aa_inode_sec {
	struct inode *inode;	/* back pointer to inode object */
	struct aa_label *label;
	u16 sclass;		/* security class of this object */
	bool initialized;	/* initialization flag */
	spinlock_t lock;
};

struct aa_superblock_sec {
	struct aa_label *label;
};

static inline struct aa_inode_sec *apparmor_inode(const struct inode *inode)
{
	if (unlikely(!inode->i_security))
		return NULL;
	return inode->i_security + apparmor_blob_sizes.lbs_inode;
}

static inline struct aa_superblock_sec *apparmor_superblock(
					const struct super_block *sb)
{
	return sb->s_security + apparmor_blob_sizes.lbs_superblock;
}

#endif /* __AA_INODE_H */
