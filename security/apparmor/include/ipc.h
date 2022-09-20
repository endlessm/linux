/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * AppArmor security module
 *
 * This file contains AppArmor ipc mediation function definitions.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2017 Canonical Ltd.
 */

#ifndef __AA_IPC_H
#define __AA_IPC_H

#include <linux/msg.h>
#include <linux/sched.h>

#include "inode.h"
#include "perms.h"

struct aa_msg_sec {
	struct aa_label *label;
};

struct aa_ipc_sec {
	struct aa_label *label;
};

static inline struct aa_ipc_sec *apparmor_ipc(const struct kern_ipc_perm *ipc)
{
	return ipc->security + apparmor_blob_sizes.lbs_ipc;
}

static inline struct aa_msg_sec *apparmor_msg_msg(const struct msg_msg *msg_msg)
{
	return msg_msg->security + apparmor_blob_sizes.lbs_msg_msg;
}


static inline bool is_mqueue_sb(struct super_block *sb)
{
	if (!sb)
		pr_warn("mqueue sb == NULL\n");
	if (!sb && !sb->s_type->name)
		pr_warn("mqueue sb name == NULL\n");
	return sb && sb->s_type->name && strcmp(sb->s_type->name, "mqueue") == 0;
}

static inline bool is_mqueue_inode(struct inode *i)
{
	struct aa_inode_sec *isec;

	if (!i)
		return false;

	isec = apparmor_inode(i);
	return isec && isec->sclass == AA_CLASS_POSIX_MQUEUE;
}

int aa_may_signal(const struct cred *subj_cred, struct aa_label *sender,
		  const struct cred *target_cred, struct aa_label *target,
		  int sig);

#define AA_AUDIT_POSIX_MQUEUE_MASK (AA_MAY_WRITE | AA_MAY_READ |    \
				    AA_MAY_CREATE | AA_MAY_DELETE | \
				    AA_MAY_OPEN | AA_MAY_SETATTR |  \
				    AA_MAY_GETATTR)


int aa_profile_mqueue_perm(struct aa_profile *profile,
			   const struct path *path,
			   u32 request, char *buffer,
			   struct apparmor_audit_data *ad);

int aa_mqueue_perm(const char *op, const struct cred *subj_cred,
		   struct aa_label *label,
		   const struct path *path, u32 request);

#endif /* __AA_IPC_H */
