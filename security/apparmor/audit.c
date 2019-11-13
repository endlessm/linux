// SPDX-License-Identifier: GPL-2.0-only
/*
 * AppArmor security module
 *
 * This file contains AppArmor auditing functions
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2010 Canonical Ltd.
 */

#include <linux/audit.h>
#include <linux/socket.h>

#include "include/apparmor.h"
#include "include/audit.h"
#include "include/policy.h"
#include "include/policy_ns.h"
#include "include/secid.h"

const char *const audit_mode_names[] = {
	"normal",
	"quiet_denied",
	"quiet",
	"noquiet",
	"all"
};

static const char *const aa_audit_type[] = {
	"AUDIT",
	"ALLOWED",
	"DENIED",
	"HINT",
	"STATUS",
	"ERROR",
	"KILLED",
	"AUTO"
};

static const char *const aa_class_names[] = {
	"none",
	"unknown",
	"file",
	"cap",
	"net",
	"rlimits",
	"domain",
	"mount",
	"unknown",
	"ptrace",
	"signal",
	"xmatch",
	"unknown",
	"unknown",
	"net",
	"unknown",
	"label",
	"posix_mqueue",
	"io_uring",
	"module",
	"lsm",
	"namespace",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"X",
	"dbus",
};


/*
 * Currently AppArmor auditing is fed straight into the audit framework.
 *
 * TODO:
 * netlink interface for complain mode
 * user auditing, - send user auditing to netlink interface
 * system control of whether user audit messages go to system log
 */

/**
 * audit_pre() - core AppArmor function.
 * @ab: audit buffer to fill (NOT NULL)
 * @ca: audit structure containing data to audit (NOT NULL)
 *
 * Record common AppArmor audit data from @sa
 */
static void audit_pre(struct audit_buffer *ab, void *ca)
{
	struct common_audit_data *sa = ca;
	struct apparmor_audit_data *ad = aad(sa);

	if (aa_g_audit_header) {
		audit_log_format(ab, "apparmor=\"%s\"",
				 aa_audit_type[ad->type]);
	}

	if (ad->op)
		audit_log_format(ab, " operation=\"%s\"", ad->op);

	if (ad->class)
		audit_log_format(ab, " class=\"%s\"",
				 ad->class <= AA_CLASS_LAST ?
				 aa_class_names[ad->class] :
				 "unknown");

	if (ad->info) {
		audit_log_format(ab, " info=\"%s\"", ad->info);
		if (ad->error)
			audit_log_format(ab, " error=%d", ad->error);
	}

	if (ad->subj_label) {
		struct aa_label *label = ad->subj_label;

		if (label_isprofile(label)) {
			struct aa_profile *profile = labels_profile(label);

			if (profile->ns != root_ns) {
				audit_log_format(ab, " namespace=");
				audit_log_untrustedstring(ab,
						       profile->ns->base.hname);
			}
			audit_log_format(ab, " profile=");
			audit_log_untrustedstring(ab, profile->base.hname);
		} else {
			audit_log_format(ab, " label=");
			aa_label_xaudit(ab, root_ns, label, FLAG_VIEW_SUBNS,
					GFP_ATOMIC);
		}
	}

	if (ad->name) {
		audit_log_format(ab, " name=");
		audit_log_untrustedstring(ab, ad->name);
	}
}

/**
 * aa_audit_msg - Log a message to the audit subsystem
 * @ad: audit event structure (NOT NULL)
 * @cb: optional callback fn for type specific fields (MAYBE NULL)
 */
void aa_audit_msg(int type, struct apparmor_audit_data *ad,
		  void (*cb) (struct audit_buffer *, void *))
{
	ad->type = type;
	common_lsm_audit(&ad->common, audit_pre, cb);
}

/**
 * aa_audit - Log a profile based audit event to the audit subsystem
 * @type: audit type for the message
 * @profile: profile to check against (NOT NULL)
 * @ad: audit event (NOT NULL)
 * @cb: optional callback fn for type specific fields (MAYBE NULL)
 *
 * Handle default message switching based off of audit mode flags
 *
 * Returns: error on failure
 */
int aa_audit(int type, struct aa_profile *profile,
	     struct apparmor_audit_data *ad,
	     void (*cb) (struct audit_buffer *, void *))
{
	AA_BUG(!profile);

	if (type == AUDIT_APPARMOR_AUTO) {
		if (likely(!ad->error)) {
			if (AUDIT_MODE(profile) != AUDIT_ALL)
				return 0;
			type = AUDIT_APPARMOR_AUDIT;
		} else if (COMPLAIN_MODE(profile))
			type = AUDIT_APPARMOR_ALLOWED;
		else
			type = AUDIT_APPARMOR_DENIED;
	}
	if (AUDIT_MODE(profile) == AUDIT_QUIET ||
	    (type == AUDIT_APPARMOR_DENIED &&
	     AUDIT_MODE(profile) == AUDIT_QUIET_DENIED))
		return ad->error;

	if (KILL_MODE(profile) && type == AUDIT_APPARMOR_DENIED)
		type = AUDIT_APPARMOR_KILL;

	ad->subj_label = &profile->label;

	aa_audit_msg(type, ad, cb);

	if (ad->type == AUDIT_APPARMOR_KILL)
		(void)send_sig_info(SIGKILL, NULL,
			ad->common.type == LSM_AUDIT_DATA_TASK &&
			ad->common.u.tsk ? ad->common.u.tsk : current);

	if (ad->type == AUDIT_APPARMOR_ALLOWED)
		return complain_error(ad->error);

	return ad->error;
}

struct aa_audit_rule {
	struct aa_label *label;
};

void aa_audit_rule_free(void *vrule)
{
	struct aa_audit_rule *rule = vrule;

	if (rule) {
		if (!IS_ERR(rule->label))
			aa_put_label(rule->label);
		kfree(rule);
	}
}

int aa_audit_rule_init(u32 field, u32 op, char *rulestr, void **vrule)
{
	struct aa_audit_rule *rule;

	switch (field) {
	case AUDIT_SUBJ_ROLE:
		if (op != Audit_equal && op != Audit_not_equal)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	rule = kzalloc(sizeof(struct aa_audit_rule), GFP_KERNEL);

	if (!rule)
		return -ENOMEM;

	/* Currently rules are treated as coming from the root ns */
	rule->label = aa_label_parse(&root_ns->unconfined->label, rulestr,
				     GFP_KERNEL, true, false);
	if (IS_ERR(rule->label)) {
		int err = PTR_ERR(rule->label);
		aa_audit_rule_free(rule);
		return err;
	}

	*vrule = rule;
	return 0;
}

int aa_audit_rule_known(struct audit_krule *rule)
{
	int i;

	for (i = 0; i < rule->field_count; i++) {
		struct audit_field *f = &rule->fields[i];

		switch (f->type) {
		case AUDIT_SUBJ_ROLE:
			return 1;
		}
	}

	return 0;
}

int aa_audit_rule_match(u32 sid, u32 field, u32 op, void *vrule)
{
	struct aa_audit_rule *rule = vrule;
	struct aa_label *label;
	int found = 0;

	label = aa_secid_to_label(sid);

	if (!label)
		return -ENOENT;

	if (aa_label_is_subset(label, rule->label))
		found = 1;

	switch (field) {
	case AUDIT_SUBJ_ROLE:
		switch (op) {
		case Audit_equal:
			return found;
		case Audit_not_equal:
			return !found;
		}
	}
	return 0;
}

/****************************** audit cache *******************************/

static int uid_cmp(kuid_t lhs, kuid_t rhs)
{
	if (uid_lt(lhs, rhs))
		return -1;
	if (uid_gt(lhs, rhs))
		return 1;
	return 0;
}

/* std C cmp.  negative is less than, 0 is equal, positive greater than */
long aa_audit_data_cmp(struct apparmor_audit_data *lhs,
		       struct apparmor_audit_data *rhs)
{
	long res;

	/* don't compare type */
	res = lhs->class - rhs->class;
	if (res)
		return res;
	/* don't compare op */
	res = strcmp(lhs->name, rhs->name);
	if (res)
		return res;
	res = aa_label_cmp(lhs->subj_label, rhs->subj_label);
	if (res)
		return res;
	switch (lhs->class) {
	case AA_CLASS_FILE:
		if (lhs->subj_cred) {
			if (rhs->subj_cred) {
				return uid_cmp(lhs->subj_cred->fsuid,
					       rhs->subj_cred->fsuid);
			} else {
				return 1;
			}
		} else if (rhs->subj_cred) {
			return -1;
		}
		res = uid_cmp(lhs->fs.ouid, rhs->fs.ouid);
		if (res)
			return res;
		res = lhs->fs.target - rhs->fs.target;
		if (res)
			return res;
	}
	return 0;
}

void aa_audit_node_free(struct aa_audit_node *node)
{
	if (!node)
		return;

	AA_BUG(!list_empty(&node->list));

	/* common data that needs freed */
	kfree(node->data.name);
	aa_put_label(node->data.subj_label);
	if (node->data.subj_cred)
		put_cred(node->data.subj_cred);

	/* class specific data that needs freed */
	switch (node->data.class) {
	case AA_CLASS_FILE:
		aa_put_label(node->data.peer);
		kfree(node->data.fs.target);
	}

	kmem_cache_free(aa_audit_slab, node);
}

struct aa_audit_node *aa_dup_audit_data(struct apparmor_audit_data *orig,
					gfp_t gfp)
{
	struct aa_audit_node *copy;

	copy = kmem_cache_zalloc(aa_audit_slab, gfp);
	if (!copy)
		return NULL;

	copy->knotif.ad = &copy->data;
	INIT_LIST_HEAD(&copy->list);
	/* copy class early so aa_free_audit_node can use switch on failure */
	copy->data.class = orig->class;

	/* handle anything with possible failure first */
	if (orig->name) {
		copy->data.name = kstrdup(orig->name, gfp);
		if (!copy->data.name)
			goto fail;
	}
	/* don't dup info */
	switch (orig->class) {
	case AA_CLASS_FILE:
		if (orig->fs.target) {
			copy->data.fs.target = kstrdup(orig->fs.target, gfp);
			if (!copy->data.fs.target)
				goto fail;
		}
		break;
	case AA_CLASS_MOUNT:
		if (orig->mnt.src_name) {
			copy->data.mnt.src_name = kstrdup(orig->mnt.src_name, gfp);
			if (!copy->data.mnt.src_name)
				goto fail;
		}
		if (orig->mnt.type) {
			copy->data.mnt.type = kstrdup(orig->mnt.type, gfp);
			if (!copy->data.mnt.type)
				goto fail;
		}
		// copy->mnt.trans; not used atm
		if (orig->mnt.data) {
			copy->data.mnt.data = kstrdup(orig->mnt.data, gfp);
			if (!copy->data.mnt.data)
				goto fail;
		}
		break;
	}

	/* now inc counts, and copy data that can't fail */
	copy->data.error = orig->error;
	copy->data.type = orig->type;
	copy->data.request = orig->request;
	copy->data.denied = orig->denied;
	copy->data.subj_label = aa_get_label(orig->subj_label);

	if (orig->subj_cred)
		copy->data.subj_cred = get_cred(orig->subj_cred);

	switch (orig->class) {
	case AA_CLASS_NET:
		/*
		 * peer_sk;
		 * addr;
		 */
		fallthrough;
	case AA_CLASS_FILE:
		copy->data.fs.ouid = orig->fs.ouid;
		break;
	case AA_CLASS_RLIMITS:
	case AA_CLASS_SIGNAL:
	case AA_CLASS_POSIX_MQUEUE:
		copy->data.peer = aa_get_label(orig->peer);
		break;
/*
 *	case AA_CLASS_IFACE:
 *		copy->data.iface.profile = aa_get_label(orig.iface.profile);
 *		break;
 */
	};


	return copy;
fail:
	aa_audit_node_free(copy);
	return NULL;
}

#define __audit_cache_find(C, AD, COND...)				\
({									\
	struct aa_audit_node *__node;					\
	list_for_each_entry_rcu(__node, &(C)->head, list, COND) {	\
		if (aa_audit_data_cmp(&__node->data, AD) == 0)		\
			goto __out_skip;				\
	}								\
	__node = NULL;							\
__out_skip:								\
	__node;								\
})

struct aa_audit_node *aa_audit_cache_find(struct aa_audit_cache *cache,
					  struct apparmor_audit_data *ad)
{
	struct aa_audit_node *node;

	rcu_read_lock();
	node = __audit_cache_find(cache, ad);
	rcu_read_unlock();

	return node;
}

/**
 * aa_audit_cache_insert - insert an audit node into the cache
 * @cache: the cache to insert into
 * @node: the audit node to insert into the cache
 *
 * Returns: matching node in cache OR @node if @node was inserted.
 */

struct aa_audit_node *aa_audit_cache_insert(struct aa_audit_cache *cache,
					    struct aa_audit_node *node)

{
	struct aa_audit_node *tmp;

	spin_lock(&cache->lock);
	tmp = __audit_cache_find(cache, &node->data,
				 spin_is_lock(&cache->lock));
	if (!tmp) {
		list_add_rcu(&node->list, &cache->head);
		tmp = node;
		cache->size++;
	}
	/* else raced another insert */
	spin_unlock(&cache->lock);

	return tmp;
}

void aa_audit_cache_update_ent(struct aa_audit_cache *cache,
			       struct aa_audit_node *node,
			       struct apparmor_audit_data *data)
{
	spin_lock(&cache->lock);
	node->data.denied |= data->denied;
	node->data.request = (node->data.request | data->request) &
		~node->data.denied;
	spin_unlock(&cache->lock);
}

/* assumes rcu callback has already happened and list can not be walked */
void aa_audit_cache_destroy(struct aa_audit_cache *cache)
{
	struct aa_audit_node *node, *tmp;

	list_for_each_entry_safe(node, tmp, &cache->head, list) {
		list_del_init(&node->list);
		aa_audit_node_free(node);
	}
	cache->size = 0;
}
