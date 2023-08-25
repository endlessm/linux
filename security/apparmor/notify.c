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


static void __listener_add_knotif(struct aa_listener *listener,
				  struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	aa_get_listener(listener);
	list_add_tail_entry(knotif, &listener->notifications, list);
}

static void __listener_del_knotif(struct aa_listener *listener,
				  struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	list_del_init(&knotif->list);
	aa_put_listener(listener);
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
	}
	spin_unlock(&listener->lock);

	spin_lock(&listener->lock);
	while (!list_empty(&listener->pending)) {
		knotif = list_first_entry(&listener->pending,
					  struct aa_knotif,
					  list);
		__listener_del_knotif(listener, knotif);
		complete(&knotif->ready);
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

struct aa_knotif *listener_pop_and_hold_knotif(struct aa_listener *listener)
{
	struct aa_knotif *knotif = NULL;

	spin_lock(&listener->lock);
	if (!list_empty(&listener->notifications)) {
		knotif = list_first_entry(&listener->notifications, typeof(*knotif), list);
		list_del_init(&knotif->list);
	}
	spin_unlock(&listener->lock);

	return knotif;
}

void listener_repush_knotif(struct aa_listener *listener,
			    struct aa_knotif *knotif)
{
	spin_lock(&listener->lock);
	/* listener ref held from pop and hold */
	list_add_tail_entry(knotif, &listener->notifications, list);
	spin_unlock(&listener->lock);
	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);
}

void listener_push_user_pending(struct aa_listener *listener,
				struct aa_knotif *knotif)
{
	spin_lock(&listener->lock);
	list_add_tail_entry(knotif, &listener->pending, list);
	spin_unlock(&listener->lock);
//extraneous wakeup, called after reading notification
//	wake_up_interruptible_poll(&listener->wait, EPOLLOUT | EPOLLWRNORM);
}

struct aa_knotif *__del_and_hold_user_pending(struct aa_listener *listener,
					      u64 id)
{
	struct aa_knotif *knotif;

	AA_BUG(!listener);
	lockdep_assert_held(&listener->lock);

	list_for_each_entry(knotif, &listener->pending, list) {
		if (knotif->id == id) {
			list_del_init(&knotif->list);
			return knotif;
		}
	}

	return NULL;
}

void __listener_complete_user_pending(struct aa_listener *listener,
				      struct aa_knotif *knotif,
				      struct apparmor_notif_resp *uresp)
{
	list_del_init(&knotif->list);

	complete(&knotif->ready);
	aa_put_listener(listener);
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
	init_completion(&knotif->ready);
	INIT_LIST_HEAD(&knotif->list);
	__listener_add_knotif(listener, knotif);
	AA_DEBUG(DEBUG_UPCALL, "id %lld: %s wake_up_interruptible",
		 knotif->id, __func__);
	// TODO: wake up listener
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

	AA_DEBUG(DEBUG_UPCALL, "prompt doing wake_up_interruptible %lld",
		 knotif->id);
	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);

	werr = wait_for_completion_interruptible_timeout(&knotif->ready, msecs_to_jiffies(60000));
	if (werr <= 0) {
		/* ensure knotif is not on list because of early exit */
		spin_lock(&listener->lock);
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

/* base checks userspace respnse to a notification is valid */
static bool response_is_valid(struct apparmor_notif_resp *reply,
			      struct aa_knotif *knotif)
{
	if (reply->base.ntype != APPARMOR_NOTIF_RESP)
		return false;
	if ((knotif->ad->request | knotif->ad->denied) &
	    ~(reply->allow | reply->deny)) {
		AA_DEBUG(DEBUG_UPCALL,
			 "id %lld: response does not cover permission bits in the upcall request/reply 0x%x/0x%x deny/reply 0x%x/0x%x",
			 knotif->id, knotif->ad->request, reply->allow, knotif->ad->denied,
			 reply->deny);
		return false;
	}
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
	return true;
}

/* copy uresponse into knotif */
static void knotif_update_from_uresp(struct aa_knotif *knotif,
				     struct apparmor_notif_resp *uresp,
				     u16 *flags)
{
	if (uresp) {
		AA_DEBUG(DEBUG_UPCALL,
			 "notif %lld: response allow/reply 0x%x/0x%x, denied/reply 0x%x/0x%x, error %d/%d",
			 knotif->id, knotif->ad->request, uresp->allow,
			 knotif->ad->denied, uresp->deny, knotif->ad->error,
			 uresp->base.error);

		knotif->ad->denied = uresp->deny;
		knotif->ad->request = uresp->allow | uresp->deny;
		*flags = uresp->base.flags;
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
			 "notif %lld: respons bad going with: allow 0x%x, denied 0x%x, error %d",
			 knotif->id, knotif->ad->request, knotif->ad->denied,
			 knotif->ad->error);
	}

}

// move to apparmor.h
#define KNOTIF_NO_CACHE 1

/* handle userspace responding to a synchronous notification */
long aa_listener_unotif_response(struct aa_listener *listener,
				 struct apparmor_notif_resp *uresp,
				 u16 size)
{
	struct aa_knotif *knotif = NULL;
	u16 flags;
	long ret;

	spin_lock(&listener->lock);
	knotif = __del_and_hold_user_pending(listener, uresp->base.id);
	if (!knotif) {
		ret = -ENOENT;
		AA_DEBUG(DEBUG_UPCALL, "could not find id %lld",
			 uresp->base.id);
		goto out;
	}
	if (!response_is_valid(uresp, knotif)) {
		ret = -EINVAL;
		AA_DEBUG(DEBUG_UPCALL, "id %lld: response not valid", knotif->id);
		__listener_complete_user_pending(listener, knotif, NULL);
		goto out;
	}

	/* handle updating actual data */
	knotif_update_from_uresp(knotif, uresp, &flags);
	if (!(flags & KNOTIF_NO_CACHE)) {
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
		AA_DEBUG(DEBUG_UPCALL, "id %lld: cache insert %s: name %s node %s\n",
			 knotif->id, hit != node ? "lost race" : "",
			 hit->data.name, node->data.name);
		if (hit != node) {
			AA_DEBUG(DEBUG_UPCALL, "id %lld: updating existing cache entry",
				 knotif->id);
			aa_audit_cache_update_ent(&profile->learning_cache,
						  hit, &node->data);
			aa_put_audit_node(hit);
		} else {

			AA_DEBUG(DEBUG_UPCALL, "inserted into cache");
		}
		/* now to audit */
	} /* cache_response */


	ret = size;

	AA_DEBUG(DEBUG_UPCALL, "id %lld: completing notif", knotif->id);
	__listener_complete_user_pending(listener, knotif, uresp);
out:
	spin_unlock(&listener->lock);

	return ret;
}

/******************** task reading notification to userspace ****************/

/* copy to userspace: notification data */
static long build_v3_unotif(struct aa_knotif *knotif, void __user *buf,
			    u16 max_size)
{
	union apparmor_notif_all unotif = { };
	struct user_namespace *user_ns;
	struct aa_profile *profile;
	int psize, nsize = 0;
	u16 size;

	AA_DEBUG(DEBUG_UPCALL, "building notif max size %d", max_size);
	size = sizeof(unotif);
	profile = labels_profile(knotif->ad->subj_label);
	AA_BUG(profile == NULL);
	psize = strlen(profile->base.hname) + 1;
	size += psize;
	if (knotif->ad->name)
		nsize = strlen(knotif->ad->name) + 1;
	size += nsize;
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
		unotif.file.suid = from_kuid(user_ns, task_uid(knotif->ad->subjtsk));
	}
	unotif.op.class = knotif->ad->class;
	unotif.op.label = sizeof(unotif);
	unotif.file.name = sizeof(unotif) + psize;
	unotif.file.ouid = from_kuid(user_ns, knotif->ad->fs.ouid);

	put_user_ns(user_ns);

	if (copy_to_user(buf, &unotif, sizeof(unotif)))
		return -EFAULT;
	if (copy_to_user(buf + sizeof(unotif), profile->base.hname, psize))
		return -EFAULT;
	if (knotif->ad->name &&
	    copy_to_user(buf + sizeof(unotif) + psize, knotif->ad->name, nsize))
		return -EFAULT;

	return size;
}

// return < 0 == error
//          0 == repeat
//        > 0 == built notification successfully
static long build_mediation_notif(struct aa_listener *listener,
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
			listener_repush_knotif(listener, knotif);
			goto out;
		}
		break;
	default:
		AA_BUG("unknown notification class");
		AA_DEBUG(DEBUG_UPCALL, "id %lld: unknown notification class", knotif->id);
		/* skip and move onto the next notification
		 * release knotif
		 * currently knotif cleanup handled by waking task in
		 * aa_do_notification. Need to switch to refcount
		 */
//??? why put here
		aa_put_listener(listener);
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

		ret = build_mediation_notif(listener, knotif, buf, max_size);

	} while (ret == 0);
	if (ret < 0)
		goto out;

	if (knotif->ad->type == AUDIT_APPARMOR_USER) {
		AA_DEBUG(DEBUG_UPCALL, "id %lld: adding notif to pending", knotif->id);
		listener_push_user_pending(listener, knotif);
	} else {
		AA_DEBUG(DEBUG_UPCALL, "id %lld: non-prompt audit notif delivered", knotif->id);
	}
out:
	return ret;
}
