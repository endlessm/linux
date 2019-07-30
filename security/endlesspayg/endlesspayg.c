// SPDX-License-Identifier: GPL-2.0
/*
 * LSM for PAYG security
 * Copyright (C) 2019 Endless Mobile, Inc.
 */

#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/types.h>

static struct dentry *payg_dir;
static struct dentry *paygd_pid_file;
static pid_t paygd_pid = -1;

static int payg_ptrace_access_check(struct task_struct *child,
				    unsigned int mode)
{
	/* Disallow tracing of paygd */
	if (paygd_pid >= 0 && child->pid == paygd_pid)
		return -EACCES;

	return 0;
}

static struct security_hook_list payg_hooks[] __lsm_ro_after_init = {
	LSM_HOOK_INIT(ptrace_access_check, payg_ptrace_access_check),
};

/* Set paygd PID. Can only be done if it is not already set. */
static int paygd_pid_file_open(struct inode *inode,
			       struct file *filp)
{
	if (paygd_pid != -1)
		return -EACCES;

	paygd_pid = task_pid_nr(current);

	return 0;
}

static int paygd_pid_file_release(struct inode *i, struct file *filp)
{
	/* This ensure we never work again */
	paygd_pid = -2;
	return 0;
}

static const struct file_operations paygd_pid_file_ops = {
	.release = paygd_pid_file_release,
	.open = paygd_pid_file_open,
	.llseek = generic_file_llseek,
};

static int __init payg_lsm_init(void)
{
	security_add_hooks(payg_hooks, ARRAY_SIZE(payg_hooks),
			   "endlesspayg");
	return 0;
}

DEFINE_LSM(endlesspayg) = {
	.name = "endlesspayg",
	.init = payg_lsm_init,
};

static int __init payg_init_securityfs(void)
{
	payg_dir = securityfs_create_dir("endlesspayg", NULL);
	if (IS_ERR(payg_dir))
		return PTR_ERR(payg_dir);

	paygd_pid_file = securityfs_create_file("paygd_pid",
						S_IWUSR, payg_dir, NULL,
						&paygd_pid_file_ops);
	if (IS_ERR(paygd_pid_file)) {
		securityfs_remove(payg_dir);
		return PTR_ERR(paygd_pid_file);
	}

	return 0;
}
fs_initcall(payg_init_securityfs);
