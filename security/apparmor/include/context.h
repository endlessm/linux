/*
 * AppArmor security module
 *
 * This file contains AppArmor contexts used to associate "labels" to objects.
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_CONTEXT_H
#define __AA_CONTEXT_H

#include <linux/cred.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "label.h"
#include "policy.h"

#define cred_cxt(X) (X)->security
#define current_cxt() cred_cxt(current_cred())

/* struct aa_file_cxt - the AppArmor context the file was opened in
 * @perms: the permission the file was opened with
 *
 * The file_cxt could currently be directly stored in file->f_security
 * as the label reference is now stored in the f_cred.  However the
 * cxt struct will expand in the future so we keep the struct.
 */
struct aa_file_cxt {
	u16 allow;
};

/**
 * aa_alloc_file_context - allocate file_cxt
 * @gfp: gfp flags for allocation
 *
 * Returns: file_cxt or NULL on failure
 */
static inline struct aa_file_cxt *aa_alloc_file_context(gfp_t gfp)
{
	return kzalloc(sizeof(struct aa_file_cxt), gfp);
}

/**
 * aa_free_file_context - free a file_cxt
 * @cxt: file_cxt to free  (MAYBE_NULL)
 */
static inline void aa_free_file_context(struct aa_file_cxt *cxt)
{
	if (cxt)
		kzfree(cxt);
}

/**
 * struct aa_task_cxt - primary label for confined tasks
 * @label: the current label   (NOT NULL)
 * @exec: label to transition to on next exec  (MAYBE NULL)
 * @previous: label the task may return to     (MAYBE NULL)
 * @token: magic value the task must know for returning to @previous
 *
 * Contains the task's current label (which could change due to
 * change_hat).  Plus the hat_magic needed during change_hat.
 *
 * TODO: make so a task can be confined by a stack of contexts
 */
struct aa_task_cxt {
	struct aa_label *label;
	struct aa_label *onexec;
	struct aa_label *previous;
	u64 token;
};

struct aa_task_cxt *aa_alloc_task_context(gfp_t flags);
void aa_free_task_context(struct aa_task_cxt *cxt);
void aa_dup_task_context(struct aa_task_cxt *new,
			 const struct aa_task_cxt *old);
int aa_replace_current_label(struct aa_label *label);
int aa_set_current_onexec(struct aa_label *label);
int aa_set_current_hat(struct aa_label *label, u64 token);
int aa_restore_previous_label(u64 cookie);
struct aa_label *aa_get_task_label(struct task_struct *task);


/**
 * aa_cred_label - obtain cred's label
 * @cred: cred to obtain label from  (NOT NULL)
 *
 * Returns: confining label
 *
 * does NOT increment reference count
 */
static inline struct aa_label *aa_cred_label(const struct cred *cred)
{
	struct aa_task_cxt *cxt = cred_cxt(cred);
	BUG_ON(!cxt || !cxt->label);
	return cxt->label;
}

/**
 * __aa_task_label - retrieve another task's label
 * @task: task to query  (NOT NULL)
 *
 * Returns: @task's label without incrementing its ref count
 *
 * If @task != current needs to be called in RCU safe critical section
 */
static inline struct aa_label *__aa_task_label(struct task_struct *task)
{
	return aa_cred_label(__task_cred(task));
}

/**
 * __aa_task_is_confined - determine if @task has any confinement
 * @task: task to check confinement of  (NOT NULL)
 *
 * If @task != current needs to be called in RCU safe critical section
 */
static inline bool __aa_task_is_confined(struct task_struct *task)
{
	return !unconfined(__aa_task_label(task));
}

/**
 * __aa_current_label - find the current tasks confining label
 *
 * Returns: up to date confining label or the ns unconfined label (NOT NULL)
 *
 * This fn will not update the tasks cred to the most up to date version
 * of the label so it is safe to call when inside of locks.
 */
static inline struct aa_label *__aa_current_label(void)
{
	return aa_cred_label(current_cred());
}

/**
 * __aa_get_current_label - find newest version of the current tasks label
 *
 * Returns: newest version of confining label (NOT NULL)
 *
 * This fn will not update the tasks cred, so it is safe inside of locks
 *
 * The returned reference must be put with __aa_put_current_label()
 */
static inline struct aa_label *__aa_get_current_label(void)
{
	struct aa_label *l = __aa_current_label();

	if (label_invalid(l))
		l = aa_get_newest_label(l);
	return l;
}

/**
 * __aa_put_current_label - put a reference found with aa_get_current_label
 * @label: label reference to put
 *
 * Should only be used with a reference obtained with __aa_get_current_label
 * and never used in situations where the task cred may be updated
 */
static inline void __aa_put_current_label(struct aa_label *label)
{
	if (label != __aa_current_label())
		aa_put_label(label);
}

/**
 * aa_current_label - find the current tasks confining label and update it
 *
 * Returns: up to date confining label or the ns unconfined label (NOT NULL)
 *
 * This fn will update the tasks cred structure if the label has been
 * replaced.  Not safe to call inside locks
 */
static inline struct aa_label *aa_current_label(void)
{
	const struct aa_task_cxt *cxt = current_cxt();
	struct aa_label *label;
	BUG_ON(!cxt || !cxt->label);

	if (label_invalid(cxt->label)) {
		label = aa_get_newest_label(cxt->label);
		aa_replace_current_label(label);
		aa_put_label(label);
		cxt = current_cxt();
	}

	return cxt->label;
}

/**
 * aa_clear_task_cxt_trans - clear transition tracking info from the cxt
 * @cxt: task context to clear (NOT NULL)
 */
static inline void aa_clear_task_cxt_trans(struct aa_task_cxt *cxt)
{
	aa_put_label(cxt->previous);
	aa_put_label(cxt->onexec);
	cxt->previous = NULL;
	cxt->onexec = NULL;
	cxt->token = 0;
}

#endif /* __AA_CONTEXT_H */
