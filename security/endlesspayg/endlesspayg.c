// SPDX-License-Identifier: GPL-2.0
/*
 * LSM for PAYG security
 * Copyright (C) 2019 Endless Mobile, Inc.
 */

#include <linux/dcache.h>
#include <linux/efi.h>
#include <linux/fs.h>
#include <linux/lsm_hooks.h>
#include <linux/magic.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include "endlesspayg.h"

static bool payg_active = 0;
static struct dentry *payg_dir;
static struct dentry *paygd_pid_file;
static pid_t paygd_pid = -1;

#define EOSPAYG_GUID EFI_GUID(0xd89c3871, 0xae0c, 0x4fc5, 0xa4, 0x09, 0xdc, 0x71, 0x7a, 0xee, 0x61, 0xe7)

bool eospayg_skip_name(const char *name)
{
	if (paygd_pid == -1)
		return false;

	if (strncmp(name, "EOSPAYG", 7) == 0 &&
	    task_pid_nr(current) != paygd_pid)
		return true;

	if (strncmp(name, "SecureBootOption-", 17) == 0 &&
	    task_pid_nr(current) != paygd_pid)
		return true;

	return false;
}

bool eospayg_skip_name_wide(const u16 *name)
{
	if (paygd_pid == -1)
		return false;

	if (memcmp(name, L"EOSPAYG", 14) == 0 &&
	    task_pid_nr(current) != paygd_pid)
		return true;

	if (memcmp(name, L"SecureBootOption-", 34) == 0 &&
	    task_pid_nr(current) != paygd_pid)
		return true;

	return false;
}

bool eospayg_proc_pid_is_safe(pid_t pid)
{
	if (paygd_pid < 0)
		return true;

	if (pid == paygd_pid)
		return false;

	return true;
}

static bool is_payg_master(void)
{
	/* If payg isn't enforced, treat everyone like master */
	if (paygd_pid == -1)
		return true;

	if (paygd_pid == task_pid_nr(current))
		return true;

	return false;
}

static bool is_inode_on_efivarfs(struct inode *inode)
{
	if (inode->i_sb->s_magic == EFIVARFS_MAGIC ||
	    inode->i_sb->s_magic == SYSFS_MAGIC)
		return true;

	return false;
}

static bool is_dentry_name_protected(struct dentry *dentry)
{
	if (strncmp(dentry->d_name.name, "EOSPAYG", 7) == 0)
		return true;

	if (strncmp(dentry->d_name.name, "SecureBootOption-", 17) == 0)
		return true;

	return false;
}

static int payg_file_open(struct file *file)
{
	unsigned long magic = file->f_inode->i_sb->s_magic;

	if (is_payg_master())
		return 0;

	if (magic == EFIVARFS_MAGIC) {
		/* If we're on efivarfs, we're filtering efi variables */
		if (is_dentry_name_protected(file->f_path.dentry))
			return -ENOENT;
	} else if (magic == SYSFS_MAGIC) {
		const char *name = file->f_path.dentry->d_name.name;
		/*
		 * If we're on sysfs, filter emmc controls - and
		 * we're a bit sloppy about those, ignoring
		 * directory, as currently only emmc has these
		 * sysfs controls.
		 */
		if (strcmp(name, "force_ro") == 0 ||
		    strcmp(name, "ro_lock_until_next_power_on") == 0)
			return -EPERM;
	}

	return 0;
}

static int payg_inode_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	if (is_payg_master())
		return 0;

	if (!is_inode_on_efivarfs(dir))
		return 0;

	if (!is_dentry_name_protected(dentry))
		return 0;

	return -ENOENT;
}

static int payg_inode_unlink(struct inode *dir, struct dentry *dentry)
{
	if (is_payg_master())
		return 0;

	if (!is_inode_on_efivarfs(dir))
		return 0;

	if (!is_dentry_name_protected(dentry))
		return 0;

	return -ENOENT;
}

/*
 * Two problems here:
 *  - using rename as a delete
 *  - using rename as an overwrite
 */
static int payg_inode_rename(struct inode *old_dir,
				   struct dentry *old_dentry,
				   struct inode *new_dir,
				   struct dentry *new_dentry)
{
	if (is_payg_master())
		return 0;

	/* thwart use as a delete */
	if (is_inode_on_efivarfs(old_dir) &&
	    is_dentry_name_protected(old_dentry))
		return -ENOENT;

	/* thwart use as an overwrite */
	if (is_inode_on_efivarfs(new_dir) &&
	    is_dentry_name_protected(new_dentry))
		return -EPERM;

	return 0;
}

static int payg_ptrace_access_check(struct task_struct *child,
				    unsigned int mode)
{
	/* Disallow tracing of paygd */
	if (paygd_pid >= 0 && child->pid == paygd_pid)
		return -EACCES;

	return 0;
}

static int payg_task_kill(struct task_struct *p, struct kernel_siginfo *info,
                          int sig, const struct cred *cred)
{
	if (p->pid != paygd_pid)
		return 0;

	return -EPERM;
}

static struct security_hook_list payg_hooks[] __lsm_ro_after_init = {
	LSM_HOOK_INIT(ptrace_access_check, payg_ptrace_access_check),
	LSM_HOOK_INIT(file_open, payg_file_open),
	LSM_HOOK_INIT(inode_unlink, payg_inode_unlink),
	LSM_HOOK_INIT(inode_rename, payg_inode_rename),
	LSM_HOOK_INIT(inode_create, payg_inode_create),
	LSM_HOOK_INIT(task_kill, payg_task_kill),
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
	if (!payg_active)
		return 0;

	security_add_hooks(payg_hooks, ARRAY_SIZE(payg_hooks),
			   "endlesspayg");
	return 0;
}

DEFINE_LSM(endlesspayg) = {
	.name = "endlesspayg",
	.init = payg_lsm_init,
};

static int __init eospayg_active(char *str)
{
	payg_active = true;
	return 1;
}
__setup("eospayg", eospayg_active);

static bool __init uefi_check_eospayg_active(void)
{
	efi_status_t status;
	unsigned int db = 0;
	unsigned long size = sizeof(db);
	efi_guid_t guid = EOSPAYG_GUID;

	if (!efi.get_variable)
		return false;

	status = efi.get_variable(L"EOSPAYG_active", &guid, NULL, &size, &db);

	/* We only care about existence not contents so ignore buffer size */
	return status == EFI_SUCCESS || status == EFI_BUFFER_TOO_SMALL;
}

static int __init payg_init_securityfs(void)
{
	if (!uefi_check_eospayg_active())
		payg_active = false;

	if (!payg_active)
		return 0;

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
