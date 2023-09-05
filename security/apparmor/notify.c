// SPDX-License-Identifier: GPL-2.0-only
/*
 * AppArmor security module
 *
 * This file contains AppArmor notifications function definitions.
 *
 * Copyright 2019 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */
#include <linux/ctype.h>
#include <linux/utsname.h>
#include <linux/poll.h>

#include <uapi/linux/apparmor.h>

#include "include/audit.h"
#include "include/cred.h"
#include "include/lib.h"
#include "include/notify.h"
#include "include/policy_ns.h"

/* TODO: when adding listener or ns propagate, on recursive add to child ns */

// TODO: currently all knotif will have audit_node but not all in future
static inline struct aa_knotif *aa_get_knotif(struct aa_knotif *knotif)
{
	if (knotif)
		aa_get_audit_node(container_of(knotif, struct aa_audit_node,
					       knotif));

	return knotif;
}

static inline void aa_put_knotif(struct aa_knotif *knotif)
{
	if (knotif)
		aa_put_audit_node(container_of(knotif, struct aa_audit_node,
					       knotif));
}

static void put_refs(struct aa_listener *listener, struct aa_knotif *knotif)
{
	aa_put_listener(listener);
	aa_put_knotif(knotif);
}

static void get_refs(struct aa_listener *listener, struct aa_knotif *knotif)
{
	aa_get_listener(listener);
	aa_get_knotif(knotif);
}

static void __knotif_del_and_hold(struct aa_knotif *knotif)
{
	list_del_init(&knotif->list);
	knotif->flags &= ~KNOTIF_ON_LIST;
	/* keep list refcounts */
}

static void __list_append_held(struct list_head *lh, struct aa_knotif *knotif)
{
	AA_BUG(!lh);
	AA_BUG(!knotif);

	list_add_tail_entry(knotif, lh, list);
	knotif->flags |= KNOTIF_ON_LIST;
}

static void __list_push_held(struct list_head *lh, struct aa_knotif *knotif)
{
	AA_BUG(!lh);
	AA_BUG(!knotif);

	list_add_entry(knotif, lh, list);
	knotif->flags |= KNOTIF_ON_LIST;
}

static void __listener_add_knotif(struct aa_listener *listener,
				  struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	get_refs(listener, knotif);
	__list_append_held(&listener->notifications, knotif);
}

// drops refs
static void __listener_del_knotif(struct aa_listener *listener,
				  struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	list_del_init(&knotif->list);
	if (knotif->flags & KNOTIF_ON_LIST) {
		knotif->flags &= ~KNOTIF_ON_LIST;
		put_refs(listener, knotif);
	}
}

void aa_free_listener_proxy(struct aa_listener_proxy *proxy)
{
	if (proxy) {
		AA_BUG(!list_empty(&proxy->llist));
		AA_BUG(!list_empty(&proxy->nslist));

		aa_put_ns(proxy->ns);
		/* listener is owned by file, handled there */
		kfree_sensitive(proxy);
	}
}

static struct aa_listener_proxy *new_listener_proxy(struct aa_listener *listener,
						   struct aa_ns *ns)
{
	struct aa_listener_proxy *proxy;

	AA_BUG(!listener);
	lockdep_assert_not_held(&listener->lock);

	proxy = kzalloc(sizeof(*proxy), GFP_KERNEL);
	if (!proxy)
		return NULL;
	INIT_LIST_HEAD(&proxy->llist);
	INIT_LIST_HEAD(&proxy->nslist);

	proxy->listener = listener;
	if (ns)
		ns = aa_get_ns(ns);
	else
		ns = aa_get_current_ns();
	proxy->ns = ns;

	spin_lock(&listener->lock);
	list_add_tail_entry(proxy, &listener->ns_proxies, llist);
	spin_unlock(&listener->lock);

	spin_lock(&ns->listener_lock);
	list_add_tail_entry(proxy, &ns->listeners, nslist);
	spin_unlock(&ns->listener_lock);

	return proxy;
}


bool aa_register_listener_proxy(struct aa_listener *listener, struct aa_ns *ns)
{
	struct aa_listener_proxy *proxy;

	AA_BUG(!listener);

	proxy = new_listener_proxy(listener, ns);
	if (!proxy)
		return false;

	return true;
}

static void free_listener(struct aa_listener *listener)
{
	struct aa_listener_proxy *proxy;
	struct aa_knotif *knotif;

	if (!listener)
		return;

	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);

	spin_lock(&listener->lock);
	while (!list_empty(&listener->ns_proxies)) {
		proxy = list_first_entry(&listener->ns_proxies,
					 struct aa_listener_proxy,
					 llist);
		list_del_init(&proxy->llist);
		spin_unlock(&listener->lock);

		spin_lock(&proxy->ns->listener_lock);
		list_del_init(&proxy->nslist);
		spin_unlock(&proxy->ns->listener_lock);

		aa_put_ns(proxy->ns);
		kfree_sensitive(proxy);

		spin_lock(&listener->lock);
	}
	spin_unlock(&listener->lock);

	spin_lock(&listener->lock);
	while (!list_empty(&listener->notifications)) {
		knotif = list_first_entry(&listener->notifications,
					 struct aa_knotif,
					 list);
		__listener_del_knotif(listener, knotif);
		complete(&knotif->ready);
		put_refs(listener, knotif);
	}
	spin_unlock(&listener->lock);

	spin_lock(&listener->lock);
	while (!list_empty(&listener->pending)) {
		knotif = list_first_entry(&listener->pending,
					  struct aa_knotif,
					  list);
		__listener_del_knotif(listener, knotif);
		complete(&knotif->ready);
		put_refs(listener, knotif);
	}
	spin_unlock(&listener->lock);

	/* todo count on audit_data */
	aa_put_ns(listener->ns);
	aa_put_dfa(listener->filter);

	kfree_sensitive(listener);
}

void aa_listener_kref(struct kref *kref)
{
	struct aa_listener *l = container_of(kref, struct aa_listener, count);

	free_listener(l);
}

struct aa_listener *aa_new_listener(struct aa_ns *ns, gfp_t gfp)
{
	struct aa_listener *listener = kzalloc(sizeof(*listener), gfp);

	if (!listener)
		return NULL;

	kref_init(&listener->count);
	spin_lock_init(&listener->lock);
	init_waitqueue_head(&listener->wait);
	INIT_LIST_HEAD(&listener->ns_proxies);
	INIT_LIST_HEAD(&listener->notifications);
	INIT_LIST_HEAD(&listener->pending);
	kref_init(&listener->count);

	if (ns)
		ns = aa_get_ns(ns);
	else
		ns = aa_get_current_ns();
	listener->ns = ns;
	listener->last_id = 1;

	return listener;
}

static struct aa_knotif *__aa_find_notif_pending(struct aa_listener *listener,
						 u64 id)
{
	struct aa_knotif *knotif;

	AA_BUG(!listener);
	lockdep_assert_held(&listener->lock);

	list_for_each_entry(knotif, &listener->pending, list) {
		if (knotif->id == id)
			return knotif;
	}

	return NULL;
}

struct aa_knotif *__aa_find_notif(struct aa_listener *listener, u64 id)
{
	struct aa_knotif *knotif;

	AA_BUG(!listener);
	lockdep_assert_held(&listener->lock);

	list_for_each_entry(knotif, &listener->notifications, list) {
		if (knotif->id == id)
			goto out;
	}

	knotif = __aa_find_notif_pending(listener, id);
out:

	return knotif;
}

// don't drop refcounts
struct aa_knotif *listener_pop_and_hold_knotif(struct aa_listener *listener)
{
	struct aa_knotif *knotif = NULL;

	spin_lock(&listener->lock);
	if (!list_empty(&listener->notifications)) {
		knotif = list_first_entry(&listener->notifications, typeof(*knotif), list);
		__knotif_del_and_hold(knotif);
	}
	spin_unlock(&listener->lock);

	return knotif;
}

// require refcounts held
void listener_push_held_knotif(struct aa_listener *listener,
			       struct aa_knotif *knotif)
{
	spin_lock(&listener->lock);
	/* listener ref held from pop and hold */
	__list_push_held(&listener->notifications, knotif);
	spin_unlock(&listener->lock);
	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);
}

// require refcounts held
// list of knotifs waiting for response
void listener_append_held_user_pending(struct aa_listener *listener,
				       struct aa_knotif *knotif)
{
	spin_lock(&listener->lock);
	__list_append_held(&listener->pending, knotif);
	spin_unlock(&listener->lock);
	//extraneous wakeup, called after reading notification
	//wake_up_interruptible_poll(&listener->wait, EPOLLOUT | EPOLLWRNORM);
}

// don't drop refcounts
struct aa_knotif *__del_and_hold_user_pending(struct aa_listener *listener,
					      u64 id)
{
	struct aa_knotif *knotif;

	AA_BUG(!listener);
	lockdep_assert_held(&listener->lock);

	list_for_each_entry(knotif, &listener->pending, list) {
		if (knotif->id == id) {
			__knotif_del_and_hold(knotif);
			return knotif;
		}
	}

	return NULL;
}


/***************** kernel dispatching notification ********************/

/*
 * cancelled notification message due to non-timer wake-up vs.
 * keep alive message
 * cancel notification because ns removed?
 * - proxy pins ns
 * - ns can remove its list of proxies
 * - and remove queued notifications
 */

/* TODO: allow registering on multiple namespaces */
/* TODO: make filter access read side lockless */
static bool notification_match(struct aa_listener *listener,
			       struct aa_audit_node *ad)
{
	if (!(listener->mask & (1 << ad->data.type)))
		return false;

	if (!listener->filter)
		return true;

	return true;
}

/* Add a notification to the listener queue and wake up listener??? */
static void dispatch_notif(struct aa_listener *listener,
			   u16 ntype,
			   struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	AA_DEBUG_ON(knotif->id, DEBUG_UPCALL,
		    "id %lld: redispatching notification as new id %lld",
		    knotif->id, listener->last_id);
	knotif->ntype = ntype;
	knotif->id = ++listener->last_id;
	knotif->flags = 0;
	// only needed if syncrhonous notit
	init_completion(&knotif->ready);
	INIT_LIST_HEAD(&knotif->list);
	__listener_add_knotif(listener, knotif);
	AA_DEBUG(DEBUG_UPCALL, "id %lld: %s wake_up_interruptible",
		 knotif->id, __func__);
	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);
}


/* handle waiting for a user space reply to a notification
 * Returns: <0 : error or -ERESTARTSYS if interrupted
 *           0 : success
 */
static int handle_synchronous_notif(struct aa_listener *listener,
				    struct aa_knotif *knotif)
{
	long werr;
	int err;

	werr = wait_for_completion_interruptible_timeout(&knotif->ready, msecs_to_jiffies(60000));
	if (werr <= 0) {
		/* ensure knotif is not on list because of early exit */
		spin_lock(&listener->lock);
		// puts refs but still have calling refs
		__listener_del_knotif(listener, knotif);
		spin_unlock(&listener->lock);
		if (werr == 0) {
			AA_DEBUG(DEBUG_UPCALL, "id %lld: prompt timed out",
				knotif->id);
			//err = -1; // TODO: ???;
			err = 0;
		} else if (werr == -ERESTARTSYS) {
			// interrupt fired syscall needs to be restarted
			// instead of mediated
			AA_DEBUG(DEBUG_UPCALL, "id %lld: prompt interrupted, error %ld",
				 knotif->id, werr);
			err = -ERESTARTSYS;
		} else {
			AA_DEBUG(DEBUG_UPCALL, "id %lld: prompt errored out error %ld",
				 knotif->id, werr);
			err = (int) werr;
		}
		/* time out is not considered an error and will fallback
		 * to regular mediation
		 */
	} else {
		err = 0;
		spin_lock(&listener->lock);
		if (!list_empty(&knotif->list)) {
			// puts refs but still have calling refs
			__listener_del_knotif(listener, knotif);
			AA_DEBUG(DEBUG_UPCALL,
				 "id %lld: bug prompt knotif still on listener list at notif completion",
				 knotif->id);
		}
		spin_unlock(&listener->lock);
	}

	return err;
}

// permissions changed in ad
int aa_do_notification(u16 ntype, struct aa_audit_node *node)
{
	struct aa_ns *ns = labels_ns(node->data.subj_label);
	struct aa_listener_proxy *proxy;
	struct aa_listener *listener;
	struct aa_knotif *knotif;
	int count = 0, err = 0;

	AA_BUG(!node);
	AA_BUG(!ns);

	knotif = &node->knotif;

	/* TODO: make read side of list walk lockless */
	spin_lock(&ns->listener_lock);
	list_for_each_entry(proxy, &ns->listeners, nslist) {

		AA_BUG(!proxy);
		listener = aa_get_listener(proxy->listener);
		AA_BUG(!listener);
		spin_lock(&listener->lock);
		if (!notification_match(listener, node)) {
			spin_unlock(&listener->lock);
			aa_put_listener(listener);
			continue;
		}
		/* delvier notification - dispatch determines if we break */
		dispatch_notif(listener, ntype, knotif); // count);
		spin_unlock(&listener->lock);
		AA_DEBUG(DEBUG_UPCALL, "id %lld: found listener\n",
			 knotif->id);

		/* break to prompt */
		if (node->data.type == AUDIT_APPARMOR_USER) {
			spin_unlock(&ns->listener_lock);
			err = handle_synchronous_notif(listener, knotif);
			aa_put_listener(listener);
			return err;
		}
		count++;
		aa_put_listener(listener);
	}
	spin_unlock(&ns->listener_lock);
	AA_DEBUG(DEBUG_UPCALL, "id %lld: %d listener matches\n",
		 knotif->id, count);

	/* count == 0 is no match found. No change to audit params
	 * long term need to fold prompt perms into denied
	 **/
	return err;
}

/******************** task responding to notification **********************/

// drop references
// complete anything pending on ready
static void __listener_complete_held_user_pending(struct aa_listener *listener,
						  struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	lockdep_assert_held(&listener->lock);

	__knotif_del_and_hold(knotif);
	complete(&knotif->ready);
	put_refs(listener, knotif);
}

static void listener_complete_held_user_pending(struct aa_listener *listener,
						struct aa_knotif *knotif)
{
	spin_lock(&listener->lock);
	__listener_complete_held_user_pending(listener, knotif);
	spin_unlock(&listener->lock);
}

static bool response_is_valid_perm(struct apparmor_notif_resp_perm *reply,
				   struct aa_knotif *knotif, u16 size)
{
	if ((knotif->ad->denied) & ~(reply->allow | reply->deny)) {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: response does not cover permission bits in the upcall request/reply 0x%x/0x%x deny/reply 0x%x/0x%x",
			 knotif->id, knotif->ad->request, reply->allow, knotif->ad->denied,
			 reply->deny);
		return false;
	}
	return true;
	/* TODO: this was disabled per snapd request, setup flag to do check
	 * // allow bits that were never requested
	 * if (reply->allow & ~knotif->ad->request) {
	 *	AA_DEBUG(DEBUG_UPCALL, "response allows more than requested");
	 *	return false;
	 * }
	 * // denying perms not in either permission set in the original
	 * // notification
	 * if (reply->deny & ~(knotif->ad->request | knotif->ad->denied)) {
	 *	AA_DEBUG(DEBUG_UPCALL, "response denies more than requested");
	 *	return false;
	 * }
	 */
}

static bool response_is_valid_name(struct apparmor_notif_resp_name *reply,
				   struct aa_knotif *knotif, u16 size)
{
	long i;

	if (size <= sizeof(*reply)) {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: reply bad size %u < %ld",
			 knotif->id, size, sizeof(*reply));
		return -EMSGSIZE;
	}
	if (reply->name < sizeof(*reply)) {
		/* inside of data declared fields */
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: reply bad name offset in fields %u < %ld",
			 knotif->id, reply->name, sizeof(*reply));
		return -EINVAL;
	}
	if (reply->name > size) {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: reply name pasted end of data size %u > %ld",
			 knotif->id, reply->name, sizeof(*reply));
		return -EINVAL;
	}
	/* currently supported flags */
	if (reply->perm.base.flags != (URESPONSE_LOOKUP | URESPONSE_PROFILE)) {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: reply bad flags 0x%x expected 0x%x",
			 knotif->id, reply->perm.base.flags,
			 URESPONSE_LOOKUP | URESPONSE_PROFILE);
		return -EINVAL;
	}
	/* check name for terminating null */
	for (i = reply->name - sizeof(*reply); i < size - sizeof(*reply); i++) {
		if (reply->data[i] == 0)
			return true;
	}
	/* reached end of data without finding null */
	return -EINVAL;

	return false;
}

/* base checks userspace respnse to a notification is valid */
static bool response_is_valid(union apparmor_notif_resp *reply,
			      struct aa_knotif *knotif, u16 size)
{
	if (reply->base.ntype == APPARMOR_NOTIF_RESP_PERM)
		return response_is_valid_perm(&reply->perm, knotif, size);
	else if (reply->base.ntype == APPARMOR_NOTIF_RESP_NAME)
		return response_is_valid_name(&reply->name, knotif, size);
	else
		return false;
	return false;
}


/* copy uresponse into knotif */
static void knotif_update_from_uresp_perm(struct aa_knotif *knotif,
				     struct apparmor_notif_resp_perm *uresp)
{
	u16 flags;

	if (uresp) {
		AA_DEBUG(DEBUG_UPCALL,
			 "notif %lld: response allow/reply 0x%x/0x%x, denied/reply 0x%x/0x%x, error %d/%d",
			 knotif->id, knotif->ad->request, uresp->allow,
			 knotif->ad->denied, uresp->deny, knotif->ad->error,
			 uresp->base.error);

		knotif->ad->denied = uresp->deny;
		knotif->ad->request = (knotif->ad->request | uresp->allow) &
			~uresp->deny;
		flags = uresp->base.flags;
		if (!knotif->ad->denied) {
			/* no more denial, clear the error*/
			knotif->ad->error = 0;
			AA_DEBUG(DEBUG_UPCALL,
				 "notif %lld: response allowed, clearing error\n",
				 knotif->id);
		} else {
			AA_DEBUG(DEBUG_UPCALL,
				 "notif %lld: response denied returning error %d\n",
				 knotif->id, knotif->ad->error);
		}
	} else {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: respons bad going with: allow 0x%x, denied 0x%x, error %d",
			 knotif->id, knotif->ad->request, knotif->ad->denied,
			 knotif->ad->error);
	}

	if (!(flags & URESPONSE_NO_CACHE)) {
		/* cache of response requested */
		struct aa_audit_node *node = container_of(knotif,
							  struct aa_audit_node,
							  knotif);
		struct aa_audit_node *hit;
		struct aa_profile *profile = labels_profile(node->data.subj_label);

		AA_DEBUG(DEBUG_UPCALL, "id %lld: inserting cache entry requ 0x%x  denied 0x%x",
			 knotif->id, node->data.request, node->data.denied);
		hit = aa_audit_cache_insert(&profile->learning_cache,
					    node);
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: (node %p, hit %p) cache insert %s: name %s node %s\n",
			 knotif->id, node, hit, hit != node ? "entry already exists" : "",
			 hit->data.name, node->data.name);
		if (hit != node) {
			AA_DEBUG(DEBUG_UPCALL,
				 "id %lld: updating existing cache entry",
				 knotif->id);
			aa_audit_cache_update_ent(&profile->learning_cache,
						  hit, &node->data);
			aa_put_audit_node(hit);
		} else {

			AA_DEBUG(DEBUG_UPCALL, "inserted into cache");
		}
		/* now to audit */
	} /* cache_response */
}


void aa_free_ruleset(struct aa_ruleset *rules)
{
	if (!rules)
		return;
	aa_put_pdb(rules->policy);
	aa_put_pdb(rules->file);
	kfree_sensitive(rules);
}

struct aa_ruleset *aa_new_ruleset(gfp_t gfp)
{
	struct aa_ruleset *rules = kzalloc(sizeof(*rules), gfp);

	INIT_LIST_HEAD(&rules->list);

	return rules;
}

struct aa_ruleset *aa_clone_ruleset(struct aa_ruleset *rules)
{
	struct aa_ruleset *clone;

	clone = aa_new_ruleset(GFP_KERNEL);
	if (!clone)
		return NULL;
	clone->size = rules->size;
	clone->policy = aa_get_pdb(rules->policy);
	clone->file = aa_get_pdb(rules->file);
	clone->caps = rules->caps;
	clone->rlimits = rules->rlimits;

	/* TODO: secmark */
	return clone;
}

static long knotif_update_from_uresp_name(struct aa_knotif *knotif,
				struct apparmor_notif_resp_name *uresp,
				u16 size)
{
	struct aa_ruleset *rules;
	struct aa_profile *profile;
	struct aa_ns *ns;
	char *name;
	struct aa_audit_node *node = container_of(knotif,
						  struct aa_audit_node,
						  knotif);

	ns = aa_get_current_ns();
	name = (char *) &uresp->data[uresp->name - sizeof(*uresp)];
	profile = aa_lookup_profile(ns, name);
	if (!profile) {
		aa_put_ns(ns);
		return -ENOENT;
	}
	aa_put_ns(ns);

	rules = aa_clone_ruleset(list_first_entry(&profile->rules,
						  typeof(*rules), list));
	if (!rules) {
		aa_put_profile(profile);
		return -ENOMEM;
	}
	AA_DEBUG(DEBUG_UPCALL,
		 "id %lld: cloned profile '%s' rule set", knotif->id,
		 profile->base.hname);
	aa_put_profile(profile);

	/* add list to profile rules TODO: improve locking*/
	profile = labels_profile(node->data.subj_label);
	list_add_tail_entry(rules, &profile->rules, list);

	return size;
}

/* handle userspace responding to a synchronous notification */
long aa_listener_unotif_response(struct aa_listener *listener,
				 union apparmor_notif_resp *uresp,
				 u16 size)
{
	struct aa_knotif *knotif = NULL;
	long ret;

	spin_lock(&listener->lock);
	knotif = __del_and_hold_user_pending(listener, uresp->base.id);
	if (!knotif) {
		ret = -ENOENT;
		AA_DEBUG(DEBUG_UPCALL, "could not find id %lld",
			 uresp->base.id);
		goto out;
	}
	if (!response_is_valid(uresp, knotif, size)) {
		ret = -EINVAL;
		AA_DEBUG(DEBUG_UPCALL, "id %lld: response not valid", knotif->id);
		__listener_complete_held_user_pending(listener, knotif);
		goto out;
	}

	if (uresp->perm.base.ntype == APPARMOR_NOTIF_RESP_PERM) {
		knotif_update_from_uresp_perm(knotif, &uresp->perm);
	} else if (uresp->perm.base.ntype == APPARMOR_NOTIF_RESP_NAME) {
		size = knotif_update_from_uresp_name(knotif, &uresp->name, size);
	} else {
		AA_DEBUG(DEBUG_UPCALL, "id %lld: unknown response type", knotif->id);
		size = -EINVAL;
	}
	ret = size;

	AA_DEBUG(DEBUG_UPCALL, "id %lld: completing notif", knotif->id);
	__listener_complete_held_user_pending(listener, knotif);
out:
	spin_unlock(&listener->lock);

	return ret;
}

/******************** task reading notification to userspace ****************/

static long append_str(void __user *pos, long remaining, const char *str)
{
	long size;

	if (!str)
		return 0;
	size = strlen(str) + 1;
	if (size > remaining)
		return -EMSGSIZE;
	if (copy_to_user(pos, str, size))
		return -EFAULT;

	return size;
}

#define build_append_str(__BUF, __POS, __MAX, __STR, __FIELD, __SIZE)	\
({									\
	typeof(__SIZE) __tmp_size;					\
	__FIELD = __POS - __BUF;					\
	__tmp_size = append_str(__POS, max_size - (__POS - __BUF), __STR); \
	if (__tmp_size >= 0) {						\
		__POS += __tmp_size;					\
		__SIZE += __tmp_size;					\
	} else {							\
		__SIZE = __tmp_size;					\
	}								\
	(__tmp_size >= 0);						\
})

/* copy to userspace: notification data */
static long build_v3_unotif(struct aa_knotif *knotif, void __user *buf,
			    u16 max_size)
{
	union apparmor_notif_all unotif = { };
	struct user_namespace *user_ns;
	struct aa_profile *profile;
	void __user *pos;
	long size;

	AA_DEBUG(DEBUG_UPCALL, "building notif max size %d", max_size);
	size = sizeof(unotif);
	profile = labels_profile(knotif->ad->subj_label);
	AA_BUG(profile == NULL);
	if (size > max_size)
		return -EMSGSIZE;

	user_ns = get_user_ns(current->nsproxy->uts_ns->user_ns);

	/* build response */
	unotif.common.len = size;
	unotif.common.version = APPARMOR_NOTIFY_VERSION;
	unotif.base.ntype = knotif->ntype;
	unotif.base.id = knotif->id;
	unotif.base.error = knotif->ad->error;
	unotif.op.allow = knotif->ad->request & knotif->ad->denied;
	unotif.op.deny = knotif->ad->denied;
	AA_DEBUG(DEBUG_UPCALL,
		 "notif %lld: sent to user read request 0x%x, denied 0x%x, error %d",
		 knotif->id, knotif->ad->request, knotif->ad->denied, knotif->ad->error);

	if (knotif->ad->subjtsk != NULL) {
		unotif.op.pid = task_pid_vnr(knotif->ad->subjtsk);
		unotif.file.subj_uid = from_kuid(user_ns, task_uid(knotif->ad->subjtsk));
	}
	unotif.op.class = knotif->ad->class;
	unotif.file.obj_uid = from_kuid(user_ns, knotif->ad->fs.ouid);

	put_user_ns(user_ns);

	pos = buf + sizeof(unotif);
	if (!build_append_str(buf, pos, max_size, profile->base.hname,
			      unotif.op.label, size))
		return size;
	if (!build_append_str(buf, pos, max_size, knotif->ad->name,
			      unotif.file.name, size))
		return size;
	/* now the struct, at the start of user mem */
	if (copy_to_user(buf, &unotif, sizeof(unotif)))
		return -EFAULT;

	return pos - buf;
}

// return < 0 == error
//          0 == repeat
//        > 0 == built notification successfully
static long build_mediation_unotif(struct aa_listener *listener,
				   struct aa_knotif *knotif,
				   void __user *buf, u16 max_size)
{
	long ret;

	switch (knotif->ad->class) {
	case AA_CLASS_FILE:
		ret = build_v3_unotif(knotif, buf, max_size);
		if (ret < 0) {
			AA_DEBUG(DEBUG_UPCALL,
				 "id %lld: (error=%ld) failed to copy data to user reading size %ld, maxsize %d",
				 knotif->id, ret,
				 sizeof(union apparmor_notif_all), max_size);
			goto out;
		}
		break;
	default:
		AA_BUG("unknown notification class");
		AA_DEBUG(DEBUG_UPCALL, "id %lld: unknown notification class", knotif->id);
		/* skip and move onto the next notification */
		return 0;
	}
out:
	return ret;
}

/* Handle the listener reading a notification into userspace */
// TODO: output multiple messages in one recv
long aa_listener_unotif_recv(struct aa_listener *listener, void __user *buf,
			     u16 max_size)
{
	struct aa_knotif *knotif;
	long ret;

	do {
		knotif = listener_pop_and_hold_knotif(listener);
		if (!knotif) {
			ret = -ENOENT;
			break;
		}
		AA_DEBUG(DEBUG_UPCALL, "id %lld: removed notif from listener queue",
			 knotif->id);

		ret = build_mediation_unotif(listener, knotif, buf, max_size);
		if (ret < 0) {
			/* failed - drop notif and return error to reader */
			listener_complete_held_user_pending(listener, knotif);
			return ret;
		} else if (ret > 0) {
			/* else notification copied */
			break;
		}
		/* unknown notification: drop and try next */
		listener_complete_held_user_pending(listener, knotif);
	} while (ret == 0);

	/* success */
	if (knotif->ad->type == AUDIT_APPARMOR_USER) {
		AA_DEBUG(DEBUG_UPCALL, "id %lld: adding notif to pending", knotif->id);
		listener_append_held_user_pending(listener, knotif);
	} else {
		/* no one waiting on this notification drop it */
		AA_DEBUG(DEBUG_UPCALL, "id %lld: non-prompt audit notif delivered", knotif->id);
		listener_complete_held_user_pending(listener, knotif);
	}

	return ret;
}
