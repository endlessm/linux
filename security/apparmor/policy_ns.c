/*
 * AppArmor security module
 *
 * This file contains AppArmor policy manipulation functions
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * AppArmor policy namespaces, allow for different sets of policies
 * to be loaded for tasks within the namespace.
 */

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "include/apparmor.h"
#include "include/context.h"
#include "include/policy_ns.h"
#include "include/label.h"
#include "include/policy.h"

/* root profile namespace */
struct aa_ns *root_ns;
const char *aa_hidden_ns_name = "---";

/**
 * aa_ns_visible - test if @view is visible from @curr
 * @curr: namespace to treat as the parent (NOT NULL)
 * @view: namespace to test if visible from @curr (NOT NULL)
 * @subns: whether view of a subns is allowed
 *
 * Returns: true if @view is visible from @curr else false
 */
bool aa_ns_visible(struct aa_ns *curr, struct aa_ns *view, bool subns)
{
	if (curr == view)
		return true;

	if (!subns)
		return false;

	for ( ; view; view = view->parent) {
		if (view->parent == curr)
			return true;
	}

	return false;
}

/**
 * aa_na_name - Find the ns name to display for @view from @curr
 * @curr - current namespace (NOT NULL)
 * @view - namespace attempting to view (NOT NULL)
 * @subns - are subns visible
 *
 * Returns: name of @view visible from @curr
 */
const char *aa_ns_name(struct aa_ns *curr, struct aa_ns *view, bool subns)
{
	/* if view == curr then the namespace name isn't displayed */
	if (curr == view)
		return "";

	if (aa_ns_visible(curr, view, subns)) {
		/* at this point if a ns is visible it is in a view ns
		 * thus the curr ns.hname is a prefix of its name.
		 * Only output the virtualized portion of the name
		 * Add + 2 to skip over // separating curr hname prefix
		 * from the visible tail of the views hname
		 */
		return view->base.hname + strlen(curr->base.hname) + 2;
	} else
		return aa_hidden_ns_name;
}

/**
 * alloc_ns - allocate, initialize and return a new namespace
 * @prefix: parent namespace name (MAYBE NULL)
 * @name: a preallocated name  (NOT NULL)
 *
 * Returns: refcounted namespace or NULL on failure.
 */
static struct aa_ns *alloc_ns(const char *prefix, const char *name)
{
	struct aa_ns *ns;

	ns = kzalloc(sizeof(*ns), GFP_KERNEL);
	AA_DEBUG("%s(%p)\n", __func__, ns);
	if (!ns)
		return NULL;
	if (!aa_policy_init(&ns->base, prefix, name, GFP_KERNEL))
		goto fail_ns;

	INIT_LIST_HEAD(&ns->sub_ns);
	mutex_init(&ns->lock);

	/* released by free_namespace */
	ns->unconfined = aa_alloc_profile("unconfined", NULL, GFP_KERNEL);
	if (!ns->unconfined)
		goto fail_unconfined;

	ns->unconfined->label.flags |= FLAG_IX_ON_NAME_ERROR |
		FLAG_IMMUTIBLE | FLAG_NS_COUNT | FLAG_UNCONFINED;
	ns->unconfined->mode = APPARMOR_UNCONFINED;

	/* ns and ns->unconfined share ns->unconfined refcount */
	ns->unconfined->ns = ns;

	atomic_set(&ns->uniq_null, 0);

	aa_labelset_init(&ns->labels);

	return ns;

fail_unconfined:
	kzfree(ns->base.hname);
fail_ns:
	kzfree(ns);
	return NULL;
}

/**
 * aa_free_ns - free a profile namespace
 * @ns: the namespace to free  (MAYBE NULL)
 *
 * Requires: All references to the namespace must have been put, if the
 *           namespace was referenced by a profile confining a task,
 */
void aa_free_ns(struct aa_ns *ns)
{
	if (!ns)
		return;

	aa_policy_destroy(&ns->base);
	aa_labelset_destroy(&ns->labels);
	aa_put_ns(ns->parent);

	ns->unconfined->ns = NULL;
	aa_free_profile(ns->unconfined);
	kzfree(ns);
}

/**
 * __aa_findn_ns - find a namespace on a list by @name
 * @head: list to search for namespace on  (NOT NULL)
 * @name: name of namespace to look for  (NOT NULL)
 * @n: length of @name
 * Returns: unrefcounted namespace
 *
 * Requires: rcu_read_lock be held
 */
static struct aa_ns *__aa_findn_ns(struct list_head *head, const char *name,
				   size_t n)
{
	return (struct aa_ns *)__policy_strn_find(head, name, n);
}

/**
 * aa_find_ns  -  look up a profile namespace on the namespace list
 * @root: namespace to search in  (NOT NULL)
 * @name: name of namespace to find  (NOT NULL)
 * @n: length of @name
 *
 * Returns: a refcounted namespace on the list, or NULL if no namespace
 *          called @name exists.
 *
 * refcount released by caller
 */
struct aa_ns *aa_findn_ns(struct aa_ns *root, const char *name, size_t n)
{
	struct aa_ns *ns = NULL;

	rcu_read_lock();
	ns = aa_get_ns(__aa_findn_ns(&root->sub_ns, name, n));
	rcu_read_unlock();

	return ns;
}

/**
 * aa_find_ns  -  look up a profile namespace on the namespace list
 * @root: namespace to search in  (NOT NULL)
 * @name: name of namespace to find  (NOT NULL)
 *
 * Returns: a refcounted namespace on the list, or NULL if no namespace
 *          called @name exists.
 *
 * refcount released by caller
 */
struct aa_ns *aa_find_ns(struct aa_ns *root, const char *name)
{
	return aa_findn_ns(root, name, strlen(name));
}

/**
 * aa_prepare_ns - find an existing or create a new namespace of @name
 * @root: ns to treat as root
 * @name: the namespace to find or add  (NOT NULL)
 *
 * Returns: refcounted namespace or NULL if failed to create one
 */
struct aa_ns *aa_prepare_ns(struct aa_ns *root, const char *name)
{
	struct aa_ns *ns;

	mutex_lock(&root->lock);
	/* try and find the specified ns and if it doesn't exist create it */
	/* released by caller */
	ns = aa_get_ns(__aa_findn_ns(&root->sub_ns, name, strlen(name)));
	if (!ns) {
		ns = alloc_ns(root->base.hname, name);
		if (!ns)
			goto out;
		mutex_lock(&ns->lock);
		if (__aa_fs_ns_mkdir(ns, ns_subns_dir(root), name)) {
			AA_ERROR("Failed to create interface for ns %s\n",
				 ns->base.name);
			mutex_unlock(&ns->lock);
			aa_free_ns(ns);
			ns = NULL;
			goto out;
		}
		ns->parent = aa_get_ns(root);
		ns->level = root->level + 1;
		list_add_rcu(&ns->base.list, &root->sub_ns);
		/* add list ref */
		aa_get_ns(ns);
		mutex_unlock(&ns->lock);
	}
out:
	mutex_unlock(&root->lock);

	/* return ref */
	return ns;
}

static void __ns_list_release(struct list_head *head);

/**
 * destroy_namespace - remove everything contained by @ns
 * @ns: namespace to have it contents removed  (NOT NULL)
 */
static void destroy_ns(struct aa_ns *ns)
{
	if (!ns)
		return;

	mutex_lock(&ns->lock);
	/* release all profiles in this namespace */
	__aa_profile_list_release(&ns->base.profiles);

	/* release all sub namespaces */
	__ns_list_release(&ns->sub_ns);

	if (ns->parent)
		__aa_proxy_redirect(ns_unconfined(ns),
				    ns_unconfined(ns->parent));
	__aa_fs_ns_rmdir(ns);
	mutex_unlock(&ns->lock);
}

/**
 * __aa_remove_ns - remove a namespace and all its children
 * @ns: namespace to be removed  (NOT NULL)
 *
 * Requires: ns->parent->lock be held and ns removed from parent.
 */
void __aa_remove_ns(struct aa_ns *ns)
{
	/* remove ns from namespace list */
	list_del_rcu(&ns->base.list);
	destroy_ns(ns);
	aa_put_ns(ns);
}

/**
 * __ns_list_release - remove all profile namespaces on the list put refs
 * @head: list of profile namespaces  (NOT NULL)
 *
 * Requires: namespace lock be held
 */
static void __ns_list_release(struct list_head *head)
{
	struct aa_ns *ns, *tmp;
	list_for_each_entry_safe(ns, tmp, head, base.list)
		__aa_remove_ns(ns);

}

/**
 * aa_alloc_root_ns - allocate the root profile namespace
 *
 * Returns: %0 on success else error
 *
 */
int __init aa_alloc_root_ns(void)
{
	/* released by aa_free_root_ns - used as list ref*/
	root_ns = alloc_ns(NULL, "root");
	if (!root_ns)
		return -ENOMEM;

	return 0;
}

 /**
  * aa_free_root_ns - free the root profile namespace
  */
void __init aa_free_root_ns(void)
 {
	 struct aa_ns *ns = root_ns;
	 root_ns = NULL;

	 destroy_ns(ns);
	 aa_put_ns(ns);
}
