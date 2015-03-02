/*
 * AppArmor security module
 *
 * This file contains basic common functions used in AppArmor
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "include/apparmor.h"
#include "include/audit.h"
#include "include/label.h"
#include "include/perms.h"
#include "include/policy.h"

/**
 * aa_split_fqname - split a fqname into a profile and namespace name
 * @fqname: a full qualified name in namespace profile format (NOT NULL)
 * @ns_name: pointer to portion of the string containing the ns name (NOT NULL)
 *
 * Returns: profile name or NULL if one is not specified
 *
 * Split a namespace name from a profile name (see policy.c for naming
 * description).  If a portion of the name is missing it returns NULL for
 * that portion.
 *
 * NOTE: may modify the @fqname string.  The pointers returned point
 *       into the @fqname string.
 */
char *aa_split_fqname(char *fqname, char **ns_name)
{
	char *name = strim(fqname);

	*ns_name = NULL;
	if (name[0] == ':') {
		char *split = strchr(&name[1], ':');
		*ns_name = skip_spaces(&name[1]);
		if (split) {
			/* overwrite ':' with \0 */
			*split++ = 0;
			if (strncmp(split, "//", 2) == 0)
				split += 2;
			name = skip_spaces(split);
		} else
			/* a ns name without a following profile is allowed */
			name = NULL;
	}
	if (name && *name == 0)
		name = NULL;

	return name;
}

/**
 * aa_info_message - log a none profile related status message
 * @str: message to log
 */
void aa_info_message(const char *str)
{
	if (audit_enabled) {
	  DEFINE_AUDIT_DATA(sa, LSM_AUDIT_DATA_NONE, 0);
		aad(&sa)->info = str;
		aa_audit_msg(AUDIT_APPARMOR_STATUS, &sa, NULL);
	}
	printk(KERN_INFO "AppArmor: %s\n", str);
}

/**
 * __aa_kvmalloc - do allocation preferring kmalloc but falling back to vmalloc
 * @size: how many bytes of memory are required
 * @flags: the type of memory to allocate (see kmalloc).
 *
 * Return: allocated buffer or NULL if failed
 *
 * It is possible that policy being loaded from the user is larger than
 * what can be allocated by kmalloc, in those cases fall back to vmalloc.
 */
void *__aa_kvmalloc(size_t size, gfp_t flags)
{
	void *buffer = NULL;

	if (size == 0)
		return NULL;

	/* do not attempt kmalloc if we need more than 16 pages at once */
	if (size <= (16*PAGE_SIZE))
		buffer = kmalloc(size, flags | GFP_NOIO | __GFP_NOWARN);
	if (!buffer) {
		if (flags & __GFP_ZERO)
			buffer = vzalloc(size);
		else
			buffer = vmalloc(size);
	}
	return buffer;
}

__counted char *aa_str_alloc(int size, gfp_t gfp)
{
	struct counted_str *str;
	str = kmalloc(sizeof(struct counted_str) + size, gfp);
	if (!str)
		return NULL;

	kref_init(&str->count);
	return str->name;
}

void aa_str_kref(struct kref *kref)
{
	kfree(container_of(kref, struct counted_str, count));
}

/**
 * aa_perm_mask_to_chr - convert a perm mask to its short string
 * @mask: permission mask to convert
 * @str: character buffer to store string in (at least 10 characters)
 */
void aa_perm_mask_to_chr(u32 mask, char *str)
{
	if (mask & AA_EXEC_MMAP)
		*str++ = 'm';
	if (mask & MAY_READ)
		*str++ = 'r';
	if (mask & MAY_WRITE)
		*str++ = 'w';
	else if (mask & MAY_APPEND)
		*str++ = 'a';
	if (mask & AA_MAY_CREATE)
		*str++ = 'c';
	if (mask & AA_MAY_DELETE)
		*str++ = 'd';
	if (mask & AA_MAY_LINK)
		*str++ = 'l';
	if (mask & AA_MAY_LOCK)
		*str++ = 'k';
	if (mask & MAY_EXEC)
		*str++ = 'x';
	*str = '\0';
}

void aa_audit_perm_mask(struct audit_buffer *ab, u32 mask)
{
	char str[10];

	aa_perm_mask_to_chr(mask, str);

	audit_log_format(ab, "\"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\"", str,
			 mask & AA_MAY_OPEN ? " open" : "",
			 mask & AA_MAY_RENAME ? " rename" : "",
			 mask & AA_MAY_META_WRITE ? " metawrite" : "",
			 mask & AA_MAY_META_READ ? " metaread" : "",
			 mask & AA_MAY_GET_SECURITY ? " getsecurity" : "",
			 mask & AA_MAY_SET_SECURITY ? " setsecurity" : "",
			 mask & AA_MAY_CHMOD ? " chmod" : "",
			 mask & AA_MAY_CHOWN ? " chown" : "",
			 mask & AA_MAY_CHGRP ? " chgrp" : "",
			 mask & AA_MAY_MPROT_WX ? " mprot_wx" : "",
			 mask & AA_MAY_MPROT_XW ? " mprot_xw" : "",
			 mask & AA_MAY_SNAPSHOT ? " snapshot" : "",
			 mask & AA_MAY_STACK ? " stack" : "",
			 mask & AA_MAY_ONEXEC ? " onexec" : "",
			 mask & AA_MAY_CHANGE_PROFILE ? " change_profile" : "",
			 mask & AA_MAY_CHANGEHAT ? " change_hat" : "");
}

/**
 * aa_audit_perms_cb - generic callback fn for auditing perms
 * @ab: audit buffer (NOT NULL)
 * @va: audit struct to audit values of (NOT NULL)
 */
static void aa_audit_perms_cb(struct audit_buffer *ab, void *va)
{
	struct common_audit_data *sa = va;

	if (aad(sa)->request) {
		audit_log_format(ab, " requested_mask=");
		aa_audit_perm_mask(ab, aad(sa)->request);
	}
	if (aad(sa)->denied) {
		audit_log_format(ab, "denied_mask=");
		aa_audit_perm_mask(ab, aad(sa)->denied);
	}
	audit_log_format(ab, " target=");
	audit_log_untrustedstring(ab, aad(sa)->target);
}

/**
 * aa_apply_modes_to_perms - apply namespace and profile flags to perms
 * @profile: that perms where computed from
 * @perms: perms to apply mode modifiers to
 *
 * TODO: split into profile and ns based flags for when accumulating perms
 */
void aa_apply_modes_to_perms(struct aa_profile *profile, struct aa_perms *perms)
{
	switch (AUDIT_MODE(profile)) {
	case AUDIT_ALL:
		perms->audit = ALL_PERMS_MASK;
		/* fall through */
	case AUDIT_NOQUIET:
		perms->quiet = 0;
		break;
	case AUDIT_QUIET:
		perms->audit = 0;
		/* fall through */
	case AUDIT_QUIET_DENIED:
		perms->quiet = ALL_PERMS_MASK;
		break;
	}

	if (KILL_MODE(profile))
		perms->kill = ALL_PERMS_MASK;
	else if (COMPLAIN_MODE(profile))
		perms->complain = ALL_PERMS_MASK;
}

void aa_compute_perms(struct aa_dfa *dfa, unsigned int state,
		      struct aa_perms *perms)
{
	perms->deny = 0;
	perms->kill = perms->stop = 0;
	perms->complain = perms->cond = 0;
	perms->hide = 0;
	perms->allow = dfa_user_allow(dfa, state);
	perms->audit = dfa_user_audit(dfa, state);
	perms->quiet = dfa_user_quiet(dfa, state);
}

void aa_profile_match_label(struct aa_profile *profile, const char *label,
			    int type, struct aa_perms *perms)
{
	/* TODO: doesn't yet handle extended types */
	unsigned int state;
	if (profile->policy.dfa) {
		state = aa_dfa_next(profile->policy.dfa,
				    profile->policy.start[AA_CLASS_LABEL],
				    type);
		state = aa_dfa_match(profile->policy.dfa, state, label);
		aa_compute_perms(profile->policy.dfa, state, perms);
	} else
		memset(perms, 0, sizeof(*perms));
}


int aa_profile_label_perm(struct aa_profile *profile, struct aa_profile *target,
			  u32 request, int type, u32 *deny,
			  struct common_audit_data *sa)
{
	struct aa_perms perms;
	aad(sa)->label = &profile->label;
	aad(sa)->target = target;
	aad(sa)->request = request;

	aa_profile_match_label(profile, target->base.hname, type, &perms);
	aa_apply_modes_to_perms(profile, &perms);
	*deny |= request & perms.deny;
	return aa_check_perms(profile, &perms, request, sa, aa_audit_perms_cb);
}

/**
 * aa_check_perms - do audit mode selection based on perms set
 * @profile: profile being checked
 * @perms: perms computed for the request
 * @request: requested perms
 * @deny: Returns: explicit deny set
 * @sa: initialized audit structure (MAY BE NULL if not auditing)
 * @cb: callback fn for tpye specific fields (MAY BE NULL)
 *
 * Returns: 0 if permission else error code
 *
 * Note: profile audit modes need to be set before calling by setting the
 *       perm masks appropriately.
 *
 *       If not auditing then complain mode is not enabled and the
 *       error code will indicate whether there was an explicit deny
 *	 with a positive value.
 */
int aa_check_perms(struct aa_profile *profile, struct aa_perms *perms,
		   u32 request, struct common_audit_data *sa,
		   void (*cb) (struct audit_buffer *, void *))
{
	int type, error;
	bool stop = false;
	u32 denied = request & (~perms->allow | perms->deny);
	if (likely(!denied)) {
		/* mask off perms that are not being force audited */
		request &= perms->audit;
		if (!request || !sa)
			return 0;

		type = AUDIT_APPARMOR_AUDIT;
		error = 0;
	} else {
		error = -EACCES;

		if (denied & perms->kill)
			type = AUDIT_APPARMOR_KILL;
		else if (denied == (denied & perms->complain))
			type = AUDIT_APPARMOR_ALLOWED;
		else
			type = AUDIT_APPARMOR_DENIED;

		if (denied & perms->stop)
			stop = true;
		if (denied == (denied & perms->hide))
			error = -ENOENT;

		denied &= ~perms->quiet;
		if (type != AUDIT_APPARMOR_ALLOWED && (!sa || !denied))
			return error;
	}

	if (sa) {
		aad(sa)->label = &profile->label;
		aad(sa)->request = request;
		aad(sa)->denied = denied;
		aad(sa)->error = error;
		aa_audit_msg(type, sa, cb);
	}

	if (type == AUDIT_APPARMOR_ALLOWED)
		error = 0;

	return error;
}

const char *aa_imode_name(umode_t mode)
{
	switch(mode & S_IFMT) {
	case S_IFSOCK:
		return "sock";
	case S_IFLNK:
		return "link";
	case S_IFREG:
		return "reg";
	case S_IFBLK:
		return "blkdev";
	case S_IFDIR:
		return "dir";
	case S_IFCHR:
		return "chrdev";
	case S_IFIFO:
		return "fifo";
	}
	return "unknown";
}

const char *aa_peer_name(struct aa_profile *peer)
{
	if (profile_unconfined(peer))
		return "unconfined";

	return peer->base.hname;
}
