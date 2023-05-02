/* SPDX-License-Identifier: GPL-2.0-only */
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

#ifndef __AA_NOTIFY_H
#define __AA_NOTIFY_H

#include <linux/audit.h>
#include <linux/lsm_audit.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include <uapi/linux/apparmor.h>

#include "match.h"

struct aa_ns;
struct aa_audit_node;
struct apparmor_audit_data;

struct aa_listener {
	struct kref count;
	spinlock_t lock;
	wait_queue_head_t wait;
	struct list_head ns_proxies;	/* aa_listener_proxy */
	struct list_head notifications;	/* aa_audit_proxy */
	struct list_head pending;	/* aa_audit_proxy */
	struct aa_ns *ns;		/* counted - ns of listener */
	struct aa_dfa *filter;
	u64 last_id;
	u32 mask;
	u32 flags;
};

struct aa_listener_proxy {
	struct aa_ns *ns;		/* counted - ns listening to */
	struct aa_listener *listener;
	struct list_head llist;
	struct list_head nslist;
};

#define KNOTIF_PULSE
#define KNOTIF_PENDING
#define KNOTIF_CANCELLED
/* need to split knofif into audit_proxy
 * prompt notifications only go to first taker so no need for completion
 * in the proxy, it increases size of proxy in non-prompt case
 */
struct aa_knotif {
	struct apparmor_audit_data *ad;	/* counted */
	struct list_head list;
	struct completion ready;
	u64 id;
	u16 ntype;
	u16 flags;
};

void aa_free_listener_proxy(struct aa_listener_proxy *proxy);
bool aa_register_listener_proxy(struct aa_listener *listener, struct aa_ns *ns);
struct aa_listener *aa_new_listener(struct aa_ns *ns, gfp_t gfp);
struct aa_knotif *__aa_find_notif(struct aa_listener *listener, u64 id);
int aa_do_notification(u16 ntype, struct aa_audit_node *node);

long aa_listener_unotif_recv(struct aa_listener *listener, void __user *buf,
			     u16 max_size);
long aa_listener_unotif_response(struct aa_listener *listener,
				 struct apparmor_notif_resp *uresp,
				 u16 size);

void aa_listener_kref(struct kref *kref);

static inline struct aa_listener *aa_get_listener(struct aa_listener *listener)
{
	if (listener)
		kref_get(&(listener->count));

	return listener;
}

static inline void aa_put_listener(struct aa_listener *listener)
{
	if (listener)
		kref_put(&listener->count, aa_listener_kref);
}

#endif /* __AA_NOTIFY_H */
