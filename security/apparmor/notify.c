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
	wake_up_interruptible_poll(&listener->wait, EPOLLOUT | EPOLLWRNORM);
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

	if (uresp) {
		AA_DEBUG(DEBUG_UPCALL,
			 "notif %lld: response allow/reply 0x%x/0x%x, denied/reply 0x%x/0x%x, error %d/%d",
			 knotif->id, knotif->ad->request, uresp->allow,
			 knotif->ad->denied, uresp->deny, knotif->ad->error,
			 uresp->base.error);

		knotif->ad->denied = uresp->deny;
		knotif->ad->request = uresp->allow | uresp->deny;
		knotif->ad->flags = uresp->base.flags;
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
	complete(&knotif->ready);
	aa_put_listener(listener);
}


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

static void dispatch_notif(struct aa_listener *listener,
			   u16 ntype,
			   struct aa_knotif *knotif)
{
	AA_BUG(!listener);
	AA_BUG(!knotif);
	lockdep_assert_held(&listener->lock);

	AA_DEBUG_ON(knotif->id, DEBUG_UPCALL,
		    "redispatching notification with id %lld as new id %lld",
		    knotif->id, listener->last_id);
	knotif->ntype = ntype;
	knotif->id = ++listener->last_id;
	init_completion(&knotif->ready);
	INIT_LIST_HEAD(&knotif->list);
	__listener_add_knotif(listener, knotif);
}

// permissions changed in ad
int aa_do_notification(u16 ntype, struct aa_audit_node *node,
		       bool *cache_response)
{
	struct aa_ns *ns = labels_ns(node->data.subj_label);
	struct aa_listener_proxy *proxy;
	struct aa_listener *listener;
	struct aa_knotif *knotif;
	int count = 0, err = 0;

	AA_BUG(!node);
	AA_BUG(!ns);

	*cache_response = false;
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
		AA_DEBUG(DEBUG_UPCALL, "found listener for %lld\n",
			 knotif->id);

		/* break to prompt */
		if (node->data.type == AUDIT_APPARMOR_USER) {
			spin_unlock(&ns->listener_lock);
			goto prompt;
		}
		count++;
		aa_put_listener(listener);
	}
	spin_unlock(&ns->listener_lock);
	AA_DEBUG(DEBUG_UPCALL, "%d listener matches for %lld\n", count,
		 knotif->id);

	/* count == 0 is no match found. No change to audit params
	 * long term need to fold prompt perms into denied
	 **/
out:
	return err;

prompt:
	AA_DEBUG(DEBUG_UPCALL, "prompt doing wake_up_interruptible %lld",
		 knotif->id);
	wake_up_interruptible_poll(&listener->wait, EPOLLIN | EPOLLRDNORM);

	err = wait_for_completion_interruptible_timeout(&knotif->ready, msecs_to_jiffies(60000));
	if (err <= 0) {
		if (err == 0)
			AA_DEBUG(DEBUG_UPCALL, "prompt timed out %lld",
				knotif->id);
		else
			AA_DEBUG(DEBUG_UPCALL, "prompt errored out %lld",
				knotif->id);

		/* ensure knotif is not on list because of early exit */
		spin_lock(&listener->lock);
		__listener_del_knotif(listener, knotif);
		spin_unlock(&listener->lock);
		/* time out is not considered an error and will fallback
		 * to regular mediation
		 */
	} else {
		err = 0;
		if (!(node->data.flags & 1))
			*cache_response = true;
		spin_lock(&listener->lock);
		if (!list_empty(&knotif->list)) {
			__listener_del_knotif(listener, knotif);
			AA_DEBUG(DEBUG_UPCALL,
				 "bug prompt knotif still on listener list at notif completion %lld",
				 knotif->id);
		}
		spin_unlock(&listener->lock);
	}
	aa_put_listener(listener);

	goto out;
}


static bool response_is_valid(struct apparmor_notif_resp *reply,
			      struct aa_knotif *knotif)
{
	if (reply->base.ntype != APPARMOR_NOTIF_RESP)
		return false;
	if ((knotif->ad->request | knotif->ad->denied) &
	    ~(reply->allow | reply->deny)) {
		AA_DEBUG(DEBUG_UPCALL,
			 "response does not cover permission bits in the upcall request/reply 0x%x/0x%x deny/reply 0x%x/0x%x",
			 knotif->ad->request, reply->allow, knotif->ad->denied,
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

long aa_listener_unotif_response(struct aa_listener *listener,
				 struct apparmor_notif_resp *uresp,
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
	if (!response_is_valid(uresp, knotif)) {
		ret = -EINVAL;
		AA_DEBUG(DEBUG_UPCALL, "response not valid");
		__listener_complete_user_pending(listener, knotif, NULL);
		goto out;
	}

	ret = size;

	AA_DEBUG(DEBUG_UPCALL, "completing notif %lld", knotif->id);
	__listener_complete_user_pending(listener, knotif, uresp);
out:
	spin_unlock(&listener->lock);

	return ret;
}


static long build_unotif(struct aa_knotif *knotif, void __user *buf,
			 u16 max_size)
{
	union apparmor_notif_all unotif = { };
	struct user_namespace *user_ns;
	struct aa_profile *profile;
	int psize, nsize = 0;
	u16 size;

	size = sizeof(unotif);
	profile = labels_profile(knotif->ad->subj_label);
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
	if (copy_to_user(buf + sizeof(unotif) + psize, knotif->ad->name, nsize))
		return -EFAULT;

	return size;
}

// TODO: output multiple messages in one recv
long aa_listener_unotif_recv(struct aa_listener *listener, void __user *buf,
			     u16 max_size)
{
	struct aa_knotif *knotif;
	long ret;

repeat:
	knotif = listener_pop_and_hold_knotif(listener);
	if (!knotif) {
		ret = -ENOENT;
		goto out;
	}
	AA_DEBUG(DEBUG_UPCALL, "removed notif %lld from listener queue",
		 knotif->id);
	switch (knotif->ad->class) {
	case AA_CLASS_FILE:
		ret = build_unotif(knotif, buf, max_size);
		if (ret < 0) {
			AA_DEBUG(DEBUG_UPCALL,
				 "error (%ld): failed to copy to notif %lld data to user reading size %ld, maxsize %d",
				 ret, knotif->id,
				 sizeof(union apparmor_notif_all), max_size);
			listener_repush_knotif(listener, knotif);
			goto out;
		}
		break;
	default:
		AA_DEBUG(DEBUG_UPCALL, "unknown notification class");
		/* skip and move onto the next notification
		 * release knotif
		 * currently knotif cleanup handled by waking task in
		 * aa_do_notification. Need to switch to refcount
		 */
		aa_put_listener(listener);
		goto repeat;
	}

	if (knotif->ad->type == AUDIT_APPARMOR_USER) {
		AA_DEBUG(DEBUG_UPCALL, "adding notif %lld to pending", knotif->id);
		listener_push_user_pending(listener, knotif);
	} else {
		AA_DEBUG(DEBUG_UPCALL, "non-prompt audit in notif");
	}
out:
	return ret;
}
