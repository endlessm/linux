/*
 * AppArmor security module
 *
 * This file contains AppArmor resource mediation and attachment
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#include <linux/audit.h>

#include "include/audit.h"
#include "include/context.h"
#include "include/resource.h"
#include "include/policy.h"

/*
 * Table of rlimit names: we generate it from resource.h.
 */
#include "rlim_names.h"

struct aa_fs_entry aa_fs_entry_rlimit[] = {
	AA_FS_FILE_STRING("mask", AA_FS_RLIMIT_MASK),
	{ }
};

/* audit callback for resource specific fields */
static void audit_cb(struct audit_buffer *ab, void *va)
{
	struct common_audit_data *sa = va;

	audit_log_format(ab, " rlimit=%s value=%lu",
			 rlim_names[aad(sa)->rlim.rlim], aad(sa)->rlim.max);
}

/**
 * audit_resource - audit setting resource limit
 * @profile: profile being enforced  (NOT NULL)
 * @resoure: rlimit being auditing
 * @value: value being set
 * @error: error value
 *
 * Returns: 0 or sa->error else other error code on failure
 */
static int audit_resource(struct aa_profile *profile, unsigned int resource,
			  unsigned long value, int error)
{
	struct common_audit_data sa;
	struct apparmor_audit_data aad = {0,};

	sa.type = LSM_AUDIT_DATA_NONE;
	aad_set(&sa, &aad);
	aad.op = OP_SETRLIMIT,
	aad.rlim.rlim = resource;
	aad.rlim.max = value;
	aad.error = error;
	return aa_audit(AUDIT_APPARMOR_AUTO, profile, GFP_KERNEL, &sa,
			audit_cb);
}

/**
 * aa_map_resouce - map compiled policy resource to internal #
 * @resource: flattened policy resource number
 *
 * Returns: resource # for the current architecture.
 *
 * rlimit resource can vary based on architecture, map the compiled policy
 * resource # to the internal representation for the architecture.
 */
int aa_map_resource(int resource)
{
	return rlim_map[resource];
}

/**
 * aa_task_setrlimit - test permission to set an rlimit
 * @label - label confining the task  (NOT NULL)
 * @task - task the resource is being set on
 * @resource - the resource being set
 * @new_rlim - the new resource limit  (NOT NULL)
 *
 * Control raising the processes hard limit.
 *
 * Returns: 0 or error code if setting resource failed
 */
int aa_task_setrlimit(struct aa_label *label, struct task_struct *task,
		      unsigned int resource, struct rlimit *new_rlim)
{
	struct aa_profile *profile;
	struct aa_label *task_label;
	int i, error = 0;

	rcu_read_lock();
	task_label = aa_get_label(aa_cred_label(__task_cred(task)));
	rcu_read_unlock();

	/* TODO: extend resource control to handle other (non current)
	 * profiles.  AppArmor rules currently have the implicit assumption
	 * that the task is setting the resource of a task confined with
	 * the same profile.
	 */

	label_for_each_confined(i, label, profile) {
		int e = 0;
		if (label != task_label ||
		    (profile->rlimits.mask & (1 << resource) &&
		     new_rlim->rlim_max >
		     profile->rlimits.limits[resource].rlim_max))
			e = -EACCES;
		e = audit_resource(labels_profile(label), resource,
				   new_rlim->rlim_max, e);
		if (e)
			error = e;
	}
	aa_put_label(task_label);

	return error;
}

/**
 * __aa_transition_rlimits - apply new profile rlimits
 * @old_l: old label on task  (NOT NULL)
 * @new_l: new label with rlimits to apply  (NOT NULL)
 */
void __aa_transition_rlimits(struct aa_label *old_l, struct aa_label *new_l)
{
	unsigned int mask = 0;
	struct rlimit *rlim, *initrlim;
	struct aa_profile *old, *new;
	int i;

	old = labels_profile(old_l);
	new = labels_profile(new_l);

	/* for any rlimits the profile controlled, reset the soft limit
	 * to the lesser of the tasks hard limit and the init tasks soft limit
	 */
	label_for_each_confined(i, old_l, old) {
		if (old->rlimits.mask) {
			for (i = 0, mask = 1; i < RLIM_NLIMITS; i++,
				     mask <<= 1) {
				if (old->rlimits.mask & mask) {
					rlim = current->signal->rlim + i;
					initrlim = init_task.signal->rlim + i;
					rlim->rlim_cur = min(rlim->rlim_max,
							    initrlim->rlim_cur);
				}
			}
		}
	}

	/* set any new hard limits as dictated by the new profile */
	label_for_each_confined(i, new_l, new) {
		if (!new->rlimits.mask)
			continue;
		for (i = 0, mask = 1; i < RLIM_NLIMITS; i++, mask <<= 1) {
			if (!(new->rlimits.mask & mask))
				continue;

			rlim = current->signal->rlim + i;
			rlim->rlim_max = min(rlim->rlim_max,
					     new->rlimits.limits[i].rlim_max);
			/* soft limit should not exceed hard limit */
			rlim->rlim_cur = min(rlim->rlim_cur, rlim->rlim_max);
		}
	}
}
