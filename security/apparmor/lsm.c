/*
 * AppArmor security module
 *
 * This file contains AppArmor LSM hooks.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#include <linux/lsm_hooks.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/ptrace.h>
#include <linux/ctype.h>
#include <linux/sysctl.h>
#include <linux/audit.h>
#include <linux/user_namespace.h>
#include <linux/kmemleak.h>
#include <net/sock.h>

#include "include/af_unix.h"
#include "include/apparmor.h"
#include "include/apparmorfs.h"
#include "include/audit.h"
#include "include/capability.h"
#include "include/context.h"
#include "include/file.h"
#include "include/ipc.h"
#include "include/net.h"
#include "include/path.h"
#include "include/policy.h"
#include "include/procattr.h"
#include "include/mount.h"

/* Flag indicating whether initialization completed */
int apparmor_initialized __initdata;

DEFINE_PER_CPU(struct aa_buffers, aa_buffers);


/*
 * LSM hook functions
 */

/*
 * free the associated aa_task_ctx and put its labels
 */
static void apparmor_cred_free(struct cred *cred)
{
	aa_free_task_context(cred_ctx(cred));
	cred_ctx(cred) = NULL;
}

/*
 * allocate the apparmor part of blank credentials
 */
static int apparmor_cred_alloc_blank(struct cred *cred, gfp_t gfp)
{
	/* freed by apparmor_cred_free */
	struct aa_task_ctx *ctx = aa_alloc_task_context(gfp);
	if (!ctx)
		return -ENOMEM;

	cred_ctx(cred) = ctx;
	return 0;
}

/*
 * prepare new aa_task_ctx for modification by prepare_cred block
 */
static int apparmor_cred_prepare(struct cred *new, const struct cred *old,
				 gfp_t gfp)
{
	/* freed by apparmor_cred_free */
	struct aa_task_ctx *ctx = aa_alloc_task_context(gfp);
	if (!ctx)
		return -ENOMEM;

	aa_dup_task_context(ctx, cred_ctx(old));
	cred_ctx(new) = ctx;
	return 0;
}

/*
 * transfer the apparmor data to a blank set of creds
 */
static void apparmor_cred_transfer(struct cred *new, const struct cred *old)
{
	const struct aa_task_ctx *old_ctx = cred_ctx(old);
	struct aa_task_ctx *new_ctx = cred_ctx(new);

	aa_dup_task_context(new_ctx, old_ctx);
}

static int apparmor_ptrace_access_check(struct task_struct *child,
					unsigned int mode)
{
	struct aa_label *tracer, *tracee;
	int error;
	
	tracer = aa_begin_current_label(DO_UPDATE);
	tracee = aa_get_task_label(child);
	error = aa_may_ptrace(tracer, tracee,
		  mode == PTRACE_MODE_READ ? AA_PTRACE_READ : AA_PTRACE_TRACE);
	aa_put_label(tracee);
	aa_end_current_label(tracer);

	return error;
}

static int apparmor_ptrace_traceme(struct task_struct *parent)
{
	struct aa_label *tracer, *tracee;
	int error;
	
	tracee = aa_begin_current_label(DO_UPDATE);
	tracer = aa_get_task_label(parent);
	error = aa_may_ptrace(tracer, tracee, AA_PTRACE_TRACE);
	aa_put_label(tracer);
	aa_end_current_label(tracee);

	return error;
}

/* Derived from security/commoncap.c:cap_capget */
static int apparmor_capget(struct task_struct *target, kernel_cap_t *effective,
			   kernel_cap_t *inheritable, kernel_cap_t *permitted)
{
	struct aa_label *label;
	const struct cred *cred;

	rcu_read_lock();
	cred = __task_cred(target);
	label = aa_get_newest_cred_label(cred);

	/*
	 * cap_capget is stacked ahead of this and will
	 * initialize effective and permitted.
	 */
	if (!unconfined(label)) {
		struct aa_profile *profile;
		struct label_it i;
		label_for_each_confined(i, label, profile) {
			if (COMPLAIN_MODE(profile))
				continue;
			*effective = cap_intersect(*effective,
						   profile->caps.allow);
			*permitted = cap_intersect(*permitted,
						   profile->caps.allow);
		}
	}
	rcu_read_unlock();
	aa_put_label(label);

	return 0;
}

static int apparmor_capable(const struct cred *cred, struct user_namespace *ns,
			    int cap, int audit)
{
	struct aa_label *label;
	int error = 0;

	label = aa_get_newest_cred_label(cred);
	if (!unconfined(label))
		error = aa_capable(label, cap, audit);
	aa_put_label(label);

	return error;
}

/**
 * common_perm - basic common permission check wrapper fn for paths
 * @op: operation being checked
 * @path: path to check permission of  (NOT NULL)
 * @mask: requested permissions mask
 * @cond: conditional info for the permission request  (NOT NULL)
 *
 * Returns: %0 else error code if error or permission denied
 */
static int common_perm(const char *op, struct path *path, u32 mask,
		       struct path_cond *cond)
{
	struct aa_label *label;
	int error = 0;

	label = aa_begin_current_label(NO_UPDATE);
	if (!unconfined(label))
		error = aa_path_perm(op, label, path, 0, mask, cond);
	aa_end_current_label(label);

	return error;
}

static int common_perm_cond(const char *op, struct path *path, u32 mask)
{
	struct path_cond cond = { d_backing_inode(path->dentry)->i_uid,
				  d_backing_inode(path->dentry)->i_mode
	};

	return common_perm(op, path, mask, &cond);
}

static void apparmor_inode_free_security(struct inode *inode)
{
	struct aa_label *ctx = inode_ctx(inode);

	if (ctx) {
		inode_ctx(inode) = NULL;
		aa_put_label(ctx);
	}
}

/**
 * common_perm_dir_dentry - common permission wrapper when path is dir, dentry
 * @op: operation being checked
 * @dir: directory of the dentry  (NOT NULL)
 * @dentry: dentry to check  (NOT NULL)
 * @mask: requested permissions mask
 * @cond: conditional info for the permission request  (NOT NULL)
 *
 * Returns: %0 else error code if error or permission denied
 */
static int common_perm_dir_dentry(const char *op, struct path *dir,
				  struct dentry *dentry, u32 mask,
				  struct path_cond *cond)
{
	struct path path = { dir->mnt, dentry };

	return common_perm(op, &path, mask, cond);
}

/**
 * common_perm_mnt_dentry - common permission wrapper when mnt, dentry
 * @op: operation being checked
 * @mnt: mount point of dentry (NOT NULL)
 * @dentry: dentry to check  (NOT NULL)
 * @mask: requested permissions mask
 *
 * Returns: %0 else error code if error or permission denied
 */
static int common_perm_mnt_dentry(const char *op, struct vfsmount *mnt,
				  struct dentry *dentry, u32 mask)
{
	struct path path = { mnt, dentry };

	return common_perm_cond(op, &path, mask);
}

/**
 * common_perm_rm - common permission wrapper for operations doing rm
 * @op: operation being checked
 * @dir: directory that the dentry is in  (NOT NULL)
 * @dentry: dentry being rm'd  (NOT NULL)
 * @mask: requested permission mask
 *
 * Returns: %0 else error code if error or permission denied
 */
static int common_perm_rm(const char *op, struct path *dir,
			  struct dentry *dentry, u32 mask)
{
	struct inode *inode = d_backing_inode(dentry);
	struct path_cond cond = { };

	if (!inode || !dir->mnt || !path_mediated_fs(dentry))
		return 0;

	cond.uid = inode->i_uid;
	cond.mode = inode->i_mode;

	return common_perm_dir_dentry(op, dir, dentry, mask, &cond);
}

/**
 * common_perm_create - common permission wrapper for operations doing create
 * @op: operation being checked
 * @dir: directory that dentry will be created in  (NOT NULL)
 * @dentry: dentry to create   (NOT NULL)
 * @mask: request permission mask
 * @mode: created file mode
 *
 * Returns: %0 else error code if error or permission denied
 */
static int common_perm_create(const char *op, struct path *dir, struct dentry *dentry,
			      u32 mask, umode_t mode)
{
	struct path_cond cond = { current_fsuid(), mode };

	if (!dir->mnt || !path_mediated_fs(dir->dentry))
		return 0;

	return common_perm_dir_dentry(op, dir, dentry, mask, &cond);
}

static int apparmor_path_unlink(struct path *dir, struct dentry *dentry)
{
	return common_perm_rm(OP_UNLINK, dir, dentry, AA_MAY_DELETE);
}

static int apparmor_path_mkdir(struct path *dir, struct dentry *dentry,
			       umode_t mode)
{
	return common_perm_create(OP_MKDIR, dir, dentry, AA_MAY_CREATE,
				  S_IFDIR);
}

static int apparmor_path_rmdir(struct path *dir, struct dentry *dentry)
{
	return common_perm_rm(OP_RMDIR, dir, dentry, AA_MAY_DELETE);
}

static int apparmor_path_mknod(struct path *dir, struct dentry *dentry,
			       umode_t mode, unsigned int dev)
{
	return common_perm_create(OP_MKNOD, dir, dentry, AA_MAY_CREATE, mode);
}

static int apparmor_path_truncate(struct path *path)
{
	if (!path->mnt || !path_mediated_fs(path->dentry))
		return 0;

	return common_perm_cond(OP_TRUNC, path, MAY_WRITE | AA_MAY_SETATTR);
}

static int apparmor_path_symlink(struct path *dir, struct dentry *dentry,
				 const char *old_name)
{
	return common_perm_create(OP_SYMLINK, dir, dentry, AA_MAY_CREATE,
				  S_IFLNK);
}

static int apparmor_path_link(struct dentry *old_dentry, struct path *new_dir,
			      struct dentry *new_dentry)
{
	struct aa_label *label;
	int error = 0;

	if (!path_mediated_fs(old_dentry))
		return 0;

	label = aa_begin_current_label(DO_UPDATE);
	if (!unconfined(label))
		error = aa_path_link(label, old_dentry, new_dir, new_dentry);
	aa_end_current_label(label);

	return error;
}

static int apparmor_path_rename(struct path *old_dir, struct dentry *old_dentry,
				struct path *new_dir, struct dentry *new_dentry)
{
	struct aa_label *label;
	int error = 0;

	if (!path_mediated_fs(old_dentry))
		return 0;

	label = aa_begin_current_label(DO_UPDATE);
	if (!unconfined(label)) {
		struct path old_path = { old_dir->mnt, old_dentry };
		struct path new_path = { new_dir->mnt, new_dentry };
		struct path_cond cond = { d_backing_inode(old_dentry)->i_uid,
					  d_backing_inode(old_dentry)->i_mode
		};

		error = aa_path_perm(OP_RENAME_SRC, label, &old_path, 0,
				     MAY_READ | AA_MAY_GETATTR | MAY_WRITE |
				     AA_MAY_SETATTR | AA_MAY_DELETE,
				     &cond);
		if (!error)
			error = aa_path_perm(OP_RENAME_DEST, label, &new_path,
					     0, MAY_WRITE | AA_MAY_SETATTR |
					     AA_MAY_CREATE, &cond);

	}
	aa_end_current_label(label);

	return error;
}

static int apparmor_path_chmod(struct path *path, umode_t mode)
{
	if (!path_mediated_fs(path->dentry))
		return 0;

	return common_perm_cond(OP_CHMOD, path, AA_MAY_CHMOD);
}

static int apparmor_path_chown(struct path *path, kuid_t uid, kgid_t gid)
{
	if (!path_mediated_fs(path->dentry))
		return 0;

	return common_perm_cond(OP_CHOWN, path, AA_MAY_CHOWN);
}

static int apparmor_inode_getattr(const struct path *path)
{
	if (!path_mediated_fs(path->dentry))
		return 0;

	return common_perm_mnt_dentry(OP_GETATTR, path->mnt, path->dentry,
				      AA_MAY_GETATTR);
}

static int apparmor_file_open(struct file *file, const struct cred *cred)
{
	struct aa_file_ctx *fctx = file_ctx(file);
	struct aa_label *label;
	int error = 0;

	if (!path_mediated_fs(file->f_path.dentry))
		return 0;

	/* If in exec, permission is handled by bprm hooks.
	 * Cache permissions granted by the previous exec check, with
	 * implicit read and executable mmap which are required to
	 * actually execute the image.
	 */
	if (current->in_execve) {
		fctx->allow = MAY_EXEC | MAY_READ | AA_EXEC_MMAP;
		return 0;
	}

	label = aa_get_newest_cred_label(cred);
	if (!unconfined(label)) {
		struct inode *inode = file_inode(file);
		struct path_cond cond = { inode->i_uid, inode->i_mode };

		error = aa_path_perm(OP_OPEN, label, &file->f_path, 0,
				     aa_map_file_to_perms(file), &cond);
		/* todo cache full allowed permissions set and state */
		fctx->allow = aa_map_file_to_perms(file);
	}
	aa_put_label(label);

	return error;
}

static int apparmor_file_alloc_security(struct file *file)
{
	int error = 0;

	/* freed by apparmor_file_free_security */
	struct aa_label *label = aa_begin_current_label(DO_UPDATE);
	file->f_security = aa_alloc_file_ctx(label, GFP_KERNEL);
	if (!file_ctx(file))
		error = -ENOMEM;
	aa_end_current_label(label);

	return error;
}

static void apparmor_file_free_security(struct file *file)
{
	aa_free_file_ctx(file_ctx(file));
}

static int common_file_perm(const char *op, struct file *file, u32 mask)
{
	struct aa_label *label;
	int error = 0;

	label = aa_begin_current_label(NO_UPDATE);
	error = aa_file_perm(op, label, file, mask);
	aa_end_current_label(label);

	return error;
}

static int apparmor_file_receive(struct file *file)
{
	return common_file_perm(OP_FRECEIVE, file, aa_map_file_to_perms(file));
}

static int apparmor_file_permission(struct file *file, int mask)
{
	return common_file_perm(OP_FPERM, file, mask);
}

static int apparmor_file_lock(struct file *file, unsigned int cmd)
{
	u32 mask = AA_MAY_LOCK;

	if (cmd == F_WRLCK)
		mask |= MAY_WRITE;

	return common_file_perm(OP_FLOCK, file, mask);
}

static int common_mmap(const char *op, struct file *file, unsigned long prot,
		       unsigned long flags)
{
	int mask = 0;

	if (!file || !file_ctx(file))
		return 0;

	if (prot & PROT_READ)
		mask |= MAY_READ;
	/*
	 * Private mappings don't require write perms since they don't
	 * write back to the files
	 */
	if ((prot & PROT_WRITE) && !(flags & MAP_PRIVATE))
		mask |= MAY_WRITE;
	if (prot & PROT_EXEC)
		mask |= AA_EXEC_MMAP;

	return common_file_perm(op, file, mask);
}

static int apparmor_mmap_file(struct file *file, unsigned long reqprot,
			      unsigned long prot, unsigned long flags)
{
	return common_mmap(OP_FMMAP, file, prot, flags);
}

static int apparmor_file_mprotect(struct vm_area_struct *vma,
				  unsigned long reqprot, unsigned long prot)
{
	return common_mmap(OP_FMPROT, vma->vm_file, prot,
			   !(vma->vm_flags & VM_SHARED) ? MAP_PRIVATE : 0);
}

static int apparmor_sb_mount(const char *dev_name, struct path *path,
			     const char *type, unsigned long flags, void *data)
{
	struct aa_label *label;
	int error = 0;

	/* Discard magic */
	if ((flags & MS_MGC_MSK) == MS_MGC_VAL)
		flags &= ~MS_MGC_MSK;

	flags &= ~AA_MS_IGNORE_MASK;

	label = aa_begin_current_label(NO_UPDATE);
	if (!unconfined(label)) {
		if (flags & MS_REMOUNT)
			error = aa_remount(label, path, flags, data);
		else if (flags & MS_BIND)
			error = aa_bind_mount(label, path, dev_name, flags);
		else if (flags & (MS_SHARED | MS_PRIVATE | MS_SLAVE |
				  MS_UNBINDABLE))
			error = aa_mount_change_type(label, path, flags);
		else if (flags & MS_MOVE)
			error = aa_move_mount(label, path, dev_name);
		else
			error = aa_new_mount(label, dev_name, path, type,
					     flags, data);
	}
	aa_end_current_label(label);

	return error;
}

static int apparmor_sb_umount(struct vfsmount *mnt, int flags)
{
	struct aa_label *label;
	int error = 0;

	label = aa_begin_current_label(NO_UPDATE);
	if (!unconfined(label))
		error = aa_umount(label, mnt, flags);
	aa_end_current_label(label);

	return error;
}

static int apparmor_sb_pivotroot(struct path *old_path, struct path *new_path)
{
	struct aa_label *label;
	int error = 0;

	label = aa_get_current_label();
	if (!unconfined(label))
		error = aa_pivotroot(label, old_path, new_path);
	aa_put_label(label);

	return error;
}

static int apparmor_getprocattr(struct task_struct *task, char *name,
				char **value)
{
	int error = -ENOENT;
	/* released below */
	const struct cred *cred = get_task_cred(task);
	struct aa_task_ctx *ctx = cred_ctx(cred);
	struct aa_label *label = NULL;

	if (strcmp(name, "current") == 0)
		label = aa_get_newest_label(ctx->label);
	else if (strcmp(name, "prev") == 0  && ctx->previous)
		label = aa_get_newest_label(ctx->previous);
	else if (strcmp(name, "exec") == 0 && ctx->onexec)
		label = aa_get_newest_label(ctx->onexec);
	else
		error = -EINVAL;

	if (label)
		error = aa_getprocattr(label, value);

	aa_put_label(label);
	put_cred(cred);

	return error;
}

static int apparmor_setprocattr(struct task_struct *task, char *name,
				void *value, size_t size)
{
	char *command, *args = value;
	size_t arg_size;
	int error;
	DEFINE_AUDIT_DATA(sa, LSM_AUDIT_DATA_NONE, OP_SETPROCATTR);

	if (size == 0)
		return -EINVAL;
	/* args points to a PAGE_SIZE buffer, AppArmor requires that
	 * the buffer must be null terminated or have size <= PAGE_SIZE -1
	 * so that AppArmor can null terminate them
	 */
	if (args[size - 1] != '\0') {
		if (size == PAGE_SIZE)
			return -EINVAL;
		args[size] = '\0';
	}

	/* task can only write its own attributes */
	if (current != task)
		return -EACCES;

	args = value;
	args = strim(args);
	command = strsep(&args, " ");
	if (!args)
		return -EINVAL;
	args = skip_spaces(args);
	if (!*args)
		return -EINVAL;

	arg_size = size - (args - (char *) value);
	if (strcmp(name, "current") == 0) {
		if (strcmp(command, "changehat") == 0) {
			error = aa_setprocattr_changehat(args, arg_size,
							 !AA_DO_TEST);
		} else if (strcmp(command, "permhat") == 0) {
			error = aa_setprocattr_changehat(args, arg_size,
							 AA_DO_TEST);
		} else if (strcmp(command, "changeprofile") == 0) {
			error = aa_change_profile(args, !AA_ONEXEC,
						  !AA_DO_TEST, false);
		} else if (strcmp(command, "permprofile") == 0) {
			error = aa_change_profile(args, !AA_ONEXEC, AA_DO_TEST,
						  false);
		} else if (strcmp(command, "stack") == 0) {
			error = aa_change_profile(args, !AA_ONEXEC, !AA_DO_TEST,
						  true);
		} else
			goto fail;
	} else if (strcmp(name, "exec") == 0) {
		if (strcmp(command, "exec") == 0)
			error = aa_change_profile(args, AA_ONEXEC, !AA_DO_TEST,
						  false);
		else if (strcmp(command, "stack") == 0)
			error = aa_change_profile(args, AA_ONEXEC, !AA_DO_TEST,
						  true);
		else
			goto fail;
	} else
		/* only support the "current" and "exec" process attributes */
		return -EINVAL;

	if (!error)
		error = size;
	return error;

fail:
	aad(&sa)->label = aa_begin_current_label(DO_UPDATE);
	aad(&sa)->info = name;
	aad(&sa)->error = -EINVAL;
	aa_audit_msg(AUDIT_APPARMOR_DENIED, &sa, NULL);
	aa_end_current_label(aad(&sa)->label);
	return -EINVAL;
}

/**
 * apparmor_bprm_committing_creds - do task cleanup on committing new creds
 * @bprm: binprm for the exec  (NOT NULL)
 */
void apparmor_bprm_committing_creds(struct linux_binprm *bprm)
{
	struct aa_label *label = aa_current_raw_label();
	struct aa_task_ctx *new_ctx = cred_ctx(bprm->cred);

	/* bail out if unconfined or not changing profile */
	if ((new_ctx->label->proxy == label->proxy) ||
	    (unconfined(new_ctx->label)))
		return;

	aa_inherit_files(bprm->cred, current->files);

	current->pdeath_signal = 0;

	/* reset soft limits and set hard limits for the new label */
	__aa_transition_rlimits(label, new_ctx->label);
}

/**
 * apparmor_bprm_commited_cred - do cleanup after new creds committed
 * @bprm: binprm for the exec  (NOT NULL)
 */
void apparmor_bprm_committed_creds(struct linux_binprm *bprm)
{
	/* TODO: cleanup signals - ipc mediation */
	return;
}

static int apparmor_task_setrlimit(struct task_struct *task,
		unsigned int resource, struct rlimit *new_rlim)
{
	struct aa_label *label = aa_begin_current_label(NO_UPDATE);
	int error = 0;

	if (!unconfined(label))
		error = aa_task_setrlimit(label, task, resource, new_rlim);
	aa_end_current_label(label);

	return error;
}

/**
 * apparmor_sk_alloc_security - allocate and attach the sk_security field
 */
static int apparmor_sk_alloc_security(struct sock *sk, int family, gfp_t flags)
{
	struct aa_sk_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), flags);
	if (!ctx)
		return -ENOMEM;

	SK_CTX(sk) = ctx;
	//??? set local too current???

	return 0;
}

/**
 * apparmor_sk_free_security - free the sk_security field
 */
static void apparmor_sk_free_security(struct sock *sk)
{
	struct aa_sk_ctx *ctx = SK_CTX(sk);

	SK_CTX(sk) = NULL;
	aa_put_label(ctx->label);
	aa_put_label(ctx->peer);
	path_put(&ctx->path);
	kfree(ctx);
}

/**
 * apparmor_clone_security - clone the sk_security field
 */
static void apparmor_sk_clone_security(const struct sock *sk,
				       struct sock *newsk)
{
	struct aa_sk_ctx *ctx = SK_CTX(sk);
	struct aa_sk_ctx *new = SK_CTX(newsk);

	new->label = aa_get_label(ctx->label);
	new->peer = aa_get_label(ctx->peer);
	new->path = ctx->path;
	path_get(&new->path);
}

static struct path *UNIX_FS_CONN_PATH(struct sock *sk, struct sock *newsk)
{
	if (sk->sk_family == PF_UNIX && UNIX_FS(sk))
		return &unix_sk(sk)->path;
	else if (newsk->sk_family == PF_UNIX && UNIX_FS(newsk))
		return &unix_sk(newsk)->path;
	return NULL;
}

/**
 * apparmor_unix_stream_connect - check perms before making unix domain conn
 *
 * peer is locked when this hook is called
 */
static int apparmor_unix_stream_connect(struct sock *sk, struct sock *peer_sk,
					struct sock *newsk)
{
	struct aa_sk_ctx *sk_ctx = SK_CTX(sk);
	struct aa_sk_ctx *peer_ctx = SK_CTX(peer_sk);
	struct aa_sk_ctx *new_ctx = SK_CTX(newsk);
	struct aa_label *label;
	struct path *path;
	int error;

	label = aa_begin_current_label(NO_UPDATE);
	error = aa_unix_peer_perm(label, OP_CONNECT,
				(AA_MAY_CONNECT | AA_MAY_SEND | AA_MAY_RECEIVE),
				  sk, peer_sk, NULL);
	if (!UNIX_FS(peer_sk)) {
		last_error(error,
			aa_unix_peer_perm(peer_ctx->label, OP_CONNECT,
				(AA_MAY_ACCEPT | AA_MAY_SEND | AA_MAY_RECEIVE),
				peer_sk, sk, label));
	}
	aa_end_current_label(label);

	if (error)
		return error;

	/* label newsk if it wasn't labeled in post_create. Normally this
	 * would be done in sock_graft, but because we are directly looking
	 * at the peer_sk to obtain peer_labeling for unix socks this
	 * does not work
	 */
	if (!new_ctx->label)
		new_ctx->label = aa_get_label(peer_ctx->label);

	/* Cross reference the peer labels for SO_PEERSEC */
	if (new_ctx->peer)
		aa_put_label(new_ctx->peer);

	if (sk_ctx->peer)
		aa_put_label(sk_ctx->peer);

	new_ctx->peer = aa_get_label(sk_ctx->label);
	sk_ctx->peer = aa_get_label(peer_ctx->label);

	path = UNIX_FS_CONN_PATH(sk, peer_sk);
	if (path) {
		new_ctx->path = *path;
		sk_ctx->path = *path;
		path_get(path);
		path_get(path);
	}
	return 0;
}

/**
 * apparmor_unix_may_send - check perms before conn or sending unix dgrams
 *
 * other is locked when this hook is called
 *
 * dgram connect calls may_send, peer setup but path not copied?????
 */
static int apparmor_unix_may_send(struct socket *sock, struct socket *peer)
{
	struct aa_sk_ctx *peer_ctx = SK_CTX(peer->sk);
	struct aa_label *label = aa_begin_current_label(NO_UPDATE);
	int error;

	error = xcheck(aa_unix_peer_perm(label, OP_SENDMSG, AA_MAY_SEND,
					 sock->sk, peer->sk, NULL),
		       aa_unix_peer_perm(peer_ctx->label, OP_SENDMSG, AA_MAY_RECEIVE,
					 peer->sk, sock->sk, label));
	aa_end_current_label(label);

	return error;
}

/**
 * apparmor_socket_create - check perms before creating a new socket
 */
static int apparmor_socket_create(int family, int type, int protocol, int kern)
{
	struct aa_label *label;
	int error = 0;

	label = aa_begin_current_label(DO_UPDATE);
	if (!(kern || unconfined(label)))
		error = aa_sock_create_perm(label, family, type, protocol);
	aa_end_current_label(label);

	return error;
}

/**
 * apparmor_socket_post_create - setup the per-socket security struct
 *
 * Note:
 * -   kernel sockets currently labeled unconfined but we may want to
 *     move to a special kernel label
 * -   socket may not have sk here if created with sock_create_lite or
 *     sock_alloc. These should be accept cases which will be handled in
 *     sock_graft.
 */
static int apparmor_socket_post_create(struct socket *sock, int family,
				       int type, int protocol, int kern)
{
	struct aa_label *label;

	if (kern) {
		struct aa_ns *ns = aa_get_current_ns();
		label = aa_get_label(ns_unconfined(ns));
		aa_put_ns(ns);
	} else
		label = aa_get_current_label();

	if (sock->sk) {
		struct aa_sk_ctx *ctx = SK_CTX(sock->sk);
		aa_put_label(ctx->label);
		ctx->label = aa_get_label(label);
	}
	aa_put_label(label);

	return 0;
}

/**
 * apparmor_socket_bind - check perms before bind addr to socket
 */
static int apparmor_socket_bind(struct socket *sock,
				struct sockaddr *address, int addrlen)
{
	return aa_sock_bind_perm(sock, address, addrlen);
}

/**
 * apparmor_socket_connect - check perms before connecting @sock to @address
 */
static int apparmor_socket_connect(struct socket *sock,
				   struct sockaddr *address, int addrlen)
{
	return aa_sock_connect_perm(sock, address, addrlen);
}

/**
 * apparmor_socket_list - check perms before allowing listen
 */
static int apparmor_socket_listen(struct socket *sock, int backlog)
{
	return aa_sock_listen_perm(sock, backlog);
}

/**
 * apparmor_socket_accept - check perms before accepting a new connection.
 *
 * Note: while @newsock is created and has some information, the accept
 *       has not been done.
 */
static int apparmor_socket_accept(struct socket *sock, struct socket *newsock)
{
	return aa_sock_accept_perm(sock, newsock);
}

/**
 * apparmor_socket_sendmsg - check perms before sending msg to another socket
 */
static int apparmor_socket_sendmsg(struct socket *sock,
				   struct msghdr *msg, int size)
{
	int error = aa_sock_msg_perm(OP_SENDMSG, AA_MAY_SEND, sock, msg, size);
	if (!error) {
		/* TODO: setup delegation on scm rights
		   see smack for AF_INET, AF_INET6 */
		;
	}

	return error;
}

/**
 * apparmor_socket_recvmsg - check perms before receiving a message
 */
static int apparmor_socket_recvmsg(struct socket *sock,
				   struct msghdr *msg, int size, int flags)
{
	return aa_sock_msg_perm(OP_RECVMSG, AA_MAY_RECEIVE, sock, msg, size);
}

/**
 * apparmor_socket_getsockname - check perms before getting the local address
 */
static int apparmor_socket_getsockname(struct socket *sock)
{
	return aa_sock_perm(OP_GETSOCKNAME, AA_MAY_GETATTR, sock);
}

/**
 * apparmor_socket_getpeername - check perms before getting remote address
 */
static int apparmor_socket_getpeername(struct socket *sock)
{
	return aa_sock_perm(OP_GETPEERNAME, AA_MAY_GETATTR, sock);
}

/**
 * apparmor_getsockopt - check perms before getting socket options
 */
static int apparmor_socket_getsockopt(struct socket *sock, int level,
				      int optname)
{
	return aa_sock_opt_perm(OP_GETSOCKOPT, AA_MAY_GETOPT, sock,
				level, optname);
}

/**
 * apparmor_setsockopt - check perms before setting socket options
 */
static int apparmor_socket_setsockopt(struct socket *sock, int level,
				      int optname)
{
	return aa_sock_opt_perm(OP_SETSOCKOPT, AA_MAY_SETOPT, sock,
				level, optname);
}

/**
 * apparmor_socket_shutdown - check perms before shutting down @sock conn
 */
static int apparmor_socket_shutdown(struct socket *sock, int how)
{
	return aa_sock_perm(OP_SHUTDOWN, AA_MAY_SHUTDOWN, sock);
}

/**
 * apparmor_socket_sock_recv_skb - check perms before associating skb to sk
 *
 * Note: can not sleep maybe called with locks held

dont want protocol specific in __skb_recv_datagram()
to deny an incoming connection  socket_sock_rcv_skb()

 */
static int apparmor_socket_sock_rcv_skb(struct sock *sk, struct sk_buff *skb)
{
	/* TODO: */
	return 0;
}


static struct aa_label *sk_peer_label(struct sock *sk)
{
	struct sock *peer_sk;
	struct aa_sk_ctx *ctx = SK_CTX(sk);

	if (ctx->peer)
		return ctx->peer;

	if (sk->sk_family != PF_UNIX)
		return ERR_PTR(-ENOPROTOOPT);

	/* check for sockpair peering which does not go through
	 * security_unix_stream_connect
	 */
	peer_sk = unix_peer(sk);
	if (peer_sk) {
		ctx = SK_CTX(peer_sk);
		if (ctx->label)
			return ctx->label;
	}

	return ERR_PTR(-ENOPROTOOPT);
}

/**
 * apparmor_socket_getpeersec_stream - get security context of peer
 *
 * Note: for tcp only valid if using ipsec or cipso on lan
 */
static int apparmor_socket_getpeersec_stream(struct socket *sock,
					     char __user *optval,
					     int __user *optlen, unsigned len)
{
	char *name;
	int slen, error = 0;
	struct aa_label *label = aa_begin_current_label(DO_UPDATE);
	struct aa_label *peer = sk_peer_label(sock->sk);

	if (IS_ERR(peer))
		return PTR_ERR(peer);

	slen = aa_label_asxprint(&name, labels_ns(label), peer,
				 FLAG_SHOW_MODE | FLAG_VIEW_SUBNS |
				 FLAG_HIDDEN_UNCONFINED, GFP_KERNEL);
	/* don't include terminating \0 in slen, it breaks some apps */
	if (slen < 0) {
		error = -ENOMEM;
	} else {
		if (slen > len) {
			error = -ERANGE;
		} else if (copy_to_user(optval, name, slen)) {
			error = -EFAULT;
			goto out;
		}
		if (put_user(slen, optlen))
			error = -EFAULT;
	out:
		kfree(name);

	}
	aa_end_current_label(label);

	return error;
}

/**
 * apparmor_socket_getpeersec_dgram - get security label of packet
 * @sock: the peer socket
 * @skb: packet data
 * @secid: pointer to where to put the secid of the packet
 *
 * Sets the netlabel socket state on sk from parent
 */
static int apparmor_socket_getpeersec_dgram(struct socket *sock,
					    struct sk_buff *skb, u32 *secid)

{
	/* TODO: requires secid support, and netlabel */
	return -ENOPROTOOPT;
}

/**
 * apparmor_sock_graft - Initialize newly created socket
 * @sk: child sock
 * @parent: parent socket
 *
 * Note: could set off of SOCK_CTX(parent) but need to track inode and we can
 *       just set sk security information off of current creating process label
 *       Labeling of sk for accept case - probably should be sock based
 *       instead of task, because of the case where an implicitly labeled
 *       socket is shared by different tasks.
 */
static void apparmor_sock_graft(struct sock *sk, struct socket *parent)
{
	struct aa_sk_ctx *ctx = SK_CTX(sk);
	if (!ctx->label)
		ctx->label = aa_get_current_label();
}

static int apparmor_task_kill(struct task_struct *target, struct siginfo *info,
			      int sig, u32 secid)
{
	struct aa_label *cl, *tl;
	int error;

	if (secid)
		/* TODO: after secid to label mapping is done.
		 *  Dealing with USB IO specific behavior
		 */
		return 0;
	cl = aa_begin_current_label(NO_UPDATE);
	tl = aa_get_task_label(target);
	error = aa_may_signal(cl, tl, sig);
	aa_put_label(tl);
	aa_end_current_label(cl);

	return error;
}

#ifndef LSM_HOOKS_NAME
#define LSM_HOOKS_NAME(X) //.name =	(X),
#endif
static struct security_hook_list apparmor_hooks[] = {
	LSM_HOOKS_NAME("apparmor")

	LSM_HOOK_INIT(ptrace_access_check, apparmor_ptrace_access_check),
	LSM_HOOK_INIT(ptrace_traceme, apparmor_ptrace_traceme),
	LSM_HOOK_INIT(capget, apparmor_capget),
	LSM_HOOK_INIT(capable, apparmor_capable),

	LSM_HOOK_INIT(inode_free_security, apparmor_inode_free_security),

	LSM_HOOK_INIT(sb_mount, apparmor_sb_mount),
	LSM_HOOK_INIT(sb_umount, apparmor_sb_umount),
	LSM_HOOK_INIT(sb_pivotroot, apparmor_sb_pivotroot),

	LSM_HOOK_INIT(path_link, apparmor_path_link),
	LSM_HOOK_INIT(path_unlink, apparmor_path_unlink),
	LSM_HOOK_INIT(path_symlink, apparmor_path_symlink),
	LSM_HOOK_INIT(path_mkdir, apparmor_path_mkdir),
	LSM_HOOK_INIT(path_rmdir, apparmor_path_rmdir),
	LSM_HOOK_INIT(path_mknod, apparmor_path_mknod),
	LSM_HOOK_INIT(path_rename, apparmor_path_rename),
	LSM_HOOK_INIT(path_chmod, apparmor_path_chmod),
	LSM_HOOK_INIT(path_chown, apparmor_path_chown),
	LSM_HOOK_INIT(path_truncate, apparmor_path_truncate),
	LSM_HOOK_INIT(inode_getattr, apparmor_inode_getattr),

	LSM_HOOK_INIT(file_open, apparmor_file_open),
	LSM_HOOK_INIT(file_receive, apparmor_file_receive),
	LSM_HOOK_INIT(file_permission, apparmor_file_permission),
	LSM_HOOK_INIT(file_alloc_security, apparmor_file_alloc_security),
	LSM_HOOK_INIT(file_free_security, apparmor_file_free_security),
	LSM_HOOK_INIT(mmap_file, apparmor_mmap_file),
	LSM_HOOK_INIT(mmap_addr, cap_mmap_addr),
	LSM_HOOK_INIT(file_mprotect, apparmor_file_mprotect),
	LSM_HOOK_INIT(file_lock, apparmor_file_lock),

	LSM_HOOK_INIT(getprocattr, apparmor_getprocattr),
	LSM_HOOK_INIT(setprocattr, apparmor_setprocattr),

	LSM_HOOK_INIT(sk_alloc_security, apparmor_sk_alloc_security),
	LSM_HOOK_INIT(sk_free_security, apparmor_sk_free_security),
	LSM_HOOK_INIT(sk_clone_security, apparmor_sk_clone_security),

	LSM_HOOK_INIT(unix_stream_connect, apparmor_unix_stream_connect),
	LSM_HOOK_INIT(unix_may_send, apparmor_unix_may_send),

	LSM_HOOK_INIT(socket_create, apparmor_socket_create),
	LSM_HOOK_INIT(socket_post_create, apparmor_socket_post_create),
	LSM_HOOK_INIT(socket_bind, apparmor_socket_bind),
	LSM_HOOK_INIT(socket_connect, apparmor_socket_connect),
	LSM_HOOK_INIT(socket_listen, apparmor_socket_listen),
	LSM_HOOK_INIT(socket_accept, apparmor_socket_accept),
	LSM_HOOK_INIT(socket_sendmsg, apparmor_socket_sendmsg),
	LSM_HOOK_INIT(socket_recvmsg, apparmor_socket_recvmsg),
	LSM_HOOK_INIT(socket_getsockname, apparmor_socket_getsockname),
	LSM_HOOK_INIT(socket_getpeername, apparmor_socket_getpeername),
	LSM_HOOK_INIT(socket_getsockopt, apparmor_socket_getsockopt),
	LSM_HOOK_INIT(socket_setsockopt, apparmor_socket_setsockopt),
	LSM_HOOK_INIT(socket_shutdown, apparmor_socket_shutdown),
	LSM_HOOK_INIT(socket_sock_rcv_skb, apparmor_socket_sock_rcv_skb),
	LSM_HOOK_INIT(socket_getpeersec_stream,	apparmor_socket_getpeersec_stream),
	LSM_HOOK_INIT(socket_getpeersec_dgram, apparmor_socket_getpeersec_dgram),
	LSM_HOOK_INIT(sock_graft, apparmor_sock_graft),

	LSM_HOOK_INIT(cred_alloc_blank, apparmor_cred_alloc_blank),
	LSM_HOOK_INIT(cred_free, apparmor_cred_free),
	LSM_HOOK_INIT(cred_prepare, apparmor_cred_prepare),
	LSM_HOOK_INIT(cred_transfer, apparmor_cred_transfer),

	LSM_HOOK_INIT(bprm_set_creds, apparmor_bprm_set_creds),
	LSM_HOOK_INIT(bprm_committing_creds, apparmor_bprm_committing_creds),
	LSM_HOOK_INIT(bprm_committed_creds, apparmor_bprm_committed_creds),
	LSM_HOOK_INIT(bprm_secureexec, apparmor_bprm_secureexec),

	LSM_HOOK_INIT(task_setrlimit, apparmor_task_setrlimit),
	LSM_HOOK_INIT(task_kill, apparmor_task_kill),
};

/*
 * AppArmor sysfs module parameters
 */

static int param_set_aabool(const char *val, const struct kernel_param *kp);
static int param_get_aabool(char *buffer, const struct kernel_param *kp);
#define param_check_aabool param_check_bool
static const struct kernel_param_ops param_ops_aabool = {
	.flags = KERNEL_PARAM_OPS_FL_NOARG,
	.set = param_set_aabool,
	.get = param_get_aabool
};

static int param_set_aauint(const char *val, const struct kernel_param *kp);
static int param_get_aauint(char *buffer, const struct kernel_param *kp);
#define param_check_aauint param_check_uint
static const struct kernel_param_ops param_ops_aauint = {
	.set = param_set_aauint,
	.get = param_get_aauint
};

static int param_set_aalockpolicy(const char *val, const struct kernel_param *kp);
static int param_get_aalockpolicy(char *buffer, const struct kernel_param *kp);
#define param_check_aalockpolicy param_check_bool
static const struct kernel_param_ops param_ops_aalockpolicy = {
	.flags = KERNEL_PARAM_OPS_FL_NOARG,
	.set = param_set_aalockpolicy,
	.get = param_get_aalockpolicy
};

static int param_set_audit(const char *val, struct kernel_param *kp);
static int param_get_audit(char *buffer, struct kernel_param *kp);

static int param_set_mode(const char *val, struct kernel_param *kp);
static int param_get_mode(char *buffer, struct kernel_param *kp);

/* Flag values, also controllable via /sys/module/apparmor/parameters
 * We define special types as we want to do additional mediation.
 */

/* AppArmor global enforcement switch - complain, enforce, kill */
enum profile_mode aa_g_profile_mode = APPARMOR_ENFORCE;
module_param_call(mode, param_set_mode, param_get_mode,
		  &aa_g_profile_mode, S_IRUGO | S_IWUSR);

/* whether policy verification hashing is enabled */
bool aa_g_hash_policy = CONFIG_SECURITY_APPARMOR_HASH_DEFAULT;
module_param_named(hash_policy, aa_g_hash_policy, aabool, S_IRUGO | S_IWUSR);

/* Debug mode */
bool aa_g_debug;
module_param_named(debug, aa_g_debug, aabool, S_IRUGO | S_IWUSR);

/* Audit mode */
enum audit_mode aa_g_audit;
module_param_call(audit, param_set_audit, param_get_audit, &aa_g_audit,
		  S_IRUGO | S_IWUSR);

/* Determines if audit header is included in audited messages.  This
 * provides more context if the audit daemon is not running
 */
bool aa_g_audit_header = 1;
module_param_named(audit_header, aa_g_audit_header, aabool, S_IRUGO | S_IWUSR);

/* lock out loading/removal of policy
 * TODO: add in at boot loading of policy, which is the only way to
 *       load policy, if lock_policy is set
 */
bool aa_g_lock_policy;
module_param_named(lock_policy, aa_g_lock_policy, aalockpolicy,
		   S_IRUGO | S_IWUSR);

/* Syscall logging mode */
bool aa_g_logsyscall;
module_param_named(logsyscall, aa_g_logsyscall, aabool, S_IRUGO | S_IWUSR);

/* Maximum pathname length before accesses will start getting rejected */
unsigned int aa_g_path_max = 2 * PATH_MAX;
module_param_named(path_max, aa_g_path_max, aauint, S_IRUGO | S_IWUSR);

/* Determines how paranoid loading of policy is and how much verification
 * on the loaded policy is done.
 * DEPRECATED: read only as strict checking of load is always done now
 * that none root users (user namespaces) can load policy.
 */
bool aa_g_paranoid_load = 1;
module_param_named(paranoid_load, aa_g_paranoid_load, aabool, S_IRUGO);

/* Boot time disable flag */
static bool apparmor_enabled = CONFIG_SECURITY_APPARMOR_BOOTPARAM_VALUE;
module_param_named(enabled, apparmor_enabled, bool, S_IRUGO);

/* Boot time to set use of default or unconfined as initial profile */
bool aa_g_unconfined_init = CONFIG_SECURITY_APPARMOR_UNCONFINED_INIT;
module_param_named(unconfined, aa_g_unconfined_init, aabool, S_IRUGO);


static int __init apparmor_enabled_setup(char *str)
{
	unsigned long enabled;
	int error = kstrtoul(str, 0, &enabled);
	if (!error)
		apparmor_enabled = enabled ? 1 : 0;
	return 1;
}

__setup("apparmor=", apparmor_enabled_setup);

/* set global flag turning off the ability to load policy */
static int param_set_aalockpolicy(const char *val, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (aa_g_lock_policy)
		return -EACCES;
	return param_set_bool(val, kp);
}

static int param_get_aalockpolicy(char *buffer, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return param_get_bool(buffer, kp);
}

static int param_set_aabool(const char *val, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return param_set_bool(val, kp);
}

static int param_get_aabool(char *buffer, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return param_get_bool(buffer, kp);
}

static int param_set_aauint(const char *val, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return param_set_uint(val, kp);
}

static int param_get_aauint(char *buffer, const struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return param_get_uint(buffer, kp);
}

static int param_get_audit(char *buffer, struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	return sprintf(buffer, "%s", audit_mode_names[aa_g_audit]);
}

static int param_set_audit(const char *val, struct kernel_param *kp)
{
	int i;
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	if (!val)
		return -EINVAL;

	for (i = 0; i < AUDIT_MAX_INDEX; i++) {
		if (strcmp(val, audit_mode_names[i]) == 0) {
			aa_g_audit = i;
			return 0;
		}
	}

	return -EINVAL;
}

static int param_get_mode(char *buffer, struct kernel_param *kp)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;

	return sprintf(buffer, "%s", aa_profile_mode_names[aa_g_profile_mode]);
}

static int param_set_mode(const char *val, struct kernel_param *kp)
{
	int i;
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;
	if (!val)
		return -EINVAL;

	for (i = 0; i < APPARMOR_MODE_NAMES_MAX_INDEX; i++) {
		if (strcmp(val, aa_profile_mode_names[i]) == 0) {
			aa_g_profile_mode = i;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * AppArmor init functions
 */

/**
 * set_init_ctx - set a task context and profile on the first task.
 */
static int __init set_init_ctx(void)
{
	struct cred *cred = (struct cred *)current->real_cred;
	struct aa_task_ctx *ctx;

	ctx = aa_alloc_task_context(GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	if (!aa_g_unconfined_init) {
		ctx->label = aa_setup_default_label();
		if (!ctx->label) {
			aa_free_task_context(ctx);
			return -ENOMEM;
		}
		/* fs setup of default is done in aa_create_aafs() */
	} else
		ctx->label = aa_get_label(ns_unconfined(root_ns));
	cred_ctx(cred) = ctx;

	return 0;
}

static void destroy_buffers(void)
{
	u32 i, j;

	for_each_possible_cpu(i) {
		for_each_cpu_buffer(j) {
			kfree(per_cpu(aa_buffers, i).buf[j]);
			per_cpu(aa_buffers, i).buf[j] = NULL;
		}
	}
}

static int __init alloc_buffers(void)
{
	u32 i, j;

	for_each_possible_cpu(i) {
		for_each_cpu_buffer(j) {
			char *buffer;
			if (cpu_to_node(i) > num_online_nodes())
				/* fallback to kmalloc for offline nodes */
				buffer = kmalloc(aa_g_path_max, GFP_KERNEL);
			else
				buffer = kmalloc_node(aa_g_path_max, GFP_KERNEL,
						      cpu_to_node(i));
			if (!buffer) {
				destroy_buffers();
				return -ENOMEM;
			}
			per_cpu(aa_buffers, i).buf[j] = buffer;
		}
	}

	return 0;
}

#ifdef CONFIG_SYSCTL
static int apparmor_dointvec(struct ctl_table *table, int write,
			     void __user *buffer, size_t *lenp, loff_t *ppos)
{
	if (!policy_admin_capable())
		return -EPERM;
	if (!apparmor_enabled)
		return -EINVAL;

	return proc_dointvec(table, write, buffer, lenp, ppos);
}

static struct ctl_path apparmor_sysctl_path[] = {
	{ .procname = "kernel", },
	{ }
};

static struct ctl_table apparmor_sysctl_table[] = {
	{
		.procname	= "unprivileged_userns_apparmor_policy",
		.data		= &unprivileged_userns_apparmor_policy,
		.maxlen		= sizeof(int),
		.mode		= 0600,
		.proc_handler	= apparmor_dointvec,
	},
	{ }
};

static int __init apparmor_init_sysctl(void)
{
	return register_sysctl_paths(apparmor_sysctl_path,
				     apparmor_sysctl_table) ? 0 : -ENOMEM;
}
#else
static inline int apparmor_init_sysctl(void)
{
	return 0;
}
#endif /* CONFIG_SYSCTL */

static int __init apparmor_init(void)
{
	int error;

	if (!apparmor_enabled || !security_module_enable("apparmor")) {
		aa_info_message("AppArmor disabled by boot time parameter");
		apparmor_enabled = 0;
		return 0;
	}

	error = aa_setup_dfa_engine();
	if (error) {
		AA_ERROR("Unable to setup dfa engine\n");
		goto alloc_out;
	}

	error = aa_alloc_root_ns();
	if (error) {
		AA_ERROR("Unable to allocate default profile namespace\n");
		goto alloc_out;
	}

	error = apparmor_init_sysctl();
	if (error) {
		AA_ERROR("Unable to register sysctls\n");
		goto alloc_out;

	}

	error = alloc_buffers();
	if (error) {
		AA_ERROR("Unable to allocate work buffers\n");
		goto buffers_out;
	}

	error = set_init_ctx();
	if (error) {
		AA_ERROR("Failed to set context on init task\n");
		aa_free_root_ns();
		goto buffers_out;
	}
	security_add_hooks(apparmor_hooks, ARRAY_SIZE(apparmor_hooks));

	/* Report that AppArmor successfully initialized */
	apparmor_initialized = 1;
	if (aa_g_profile_mode == APPARMOR_COMPLAIN)
		aa_info_message("AppArmor initialized: complain mode enabled");
	else if (aa_g_profile_mode == APPARMOR_KILL)
		aa_info_message("AppArmor initialized: kill mode enabled");
	else
		aa_info_message("AppArmor initialized");

	return error;

buffers_out:
	destroy_buffers();

alloc_out:
	aa_destroy_aafs();
	aa_teardown_dfa_engine();

	apparmor_enabled = 0;
	return error;
}

security_initcall(apparmor_init);
