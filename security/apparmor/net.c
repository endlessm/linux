// SPDX-License-Identifier: GPL-2.0-only
/*
 * AppArmor security module
 *
 * This file contains AppArmor network mediation
 *
 * Copyright (C) 1998-2008 Novell/SUSE
 * Copyright 2009-2017 Canonical Ltd.
 */

#include "include/af_unix.h"
#include "include/apparmor.h"
#include "include/audit.h"
#include "include/cred.h"
#include "include/label.h"
#include "include/net.h"
#include "include/policy.h"
#include "include/secid.h"

#include "net_names.h"


struct aa_sfs_entry aa_sfs_entry_network[] = {
	AA_SFS_FILE_STRING("af_mask",	AA_SFS_AF_MASK),
	{ }
};

struct aa_sfs_entry aa_sfs_entry_network_compat[] = {
	AA_SFS_FILE_STRING("af_mask",	AA_SFS_AF_MASK),
	AA_SFS_FILE_BOOLEAN("af_unix",	1),
	{ }
};

static const char * const net_mask_names[] = {
	"unknown",
	"send",
	"receive",
	"unknown",

	"create",
	"shutdown",
	"connect",
	"unknown",

	"setattr",
	"getattr",
	"setcred",
	"getcred",

	"chmod",
	"chown",
	"chgrp",
	"lock",

	"mmap",
	"mprot",
	"unknown",
	"unknown",

	"accept",
	"bind",
	"listen",
	"unknown",

	"setopt",
	"getopt",
	"unknown",
	"unknown",

	"unknown",
	"unknown",
	"unknown",
	"unknown",
};

static void audit_unix_addr(struct audit_buffer *ab, const char *str,
			    struct sockaddr_un *addr, int addrlen)
{
	int len = unix_addr_len(addrlen);

	if (!addr || len <= 0) {
		audit_log_format(ab, " %s=none", str);
	} else if (addr->sun_path[0]) {
		audit_log_format(ab, " %s=", str);
		audit_log_untrustedstring(ab, addr->sun_path);
	} else {
		audit_log_format(ab, " %s=\"@", str);
		if (audit_string_contains_control(&addr->sun_path[1], len - 1))
			audit_log_n_hex(ab, &addr->sun_path[1], len - 1);
		else
			audit_log_format(ab, "%.*s", len - 1,
					 &addr->sun_path[1]);
		audit_log_format(ab, "\"");
	}
}

static void audit_unix_sk_addr(struct audit_buffer *ab, const char *str,
			       const struct sock *sk)
{
	const struct unix_sock *u = unix_sk(sk);

	if (u && u->addr)
		audit_unix_addr(ab, str, u->addr->name, u->addr->len);
	else
		audit_unix_addr(ab, str, NULL, 0);
}

/* audit callback for net specific fields */
void audit_net_cb(struct audit_buffer *ab, void *va)
{
	struct common_audit_data *sa = va;
	struct apparmor_audit_data *ad = aad(sa);

	if (address_family_names[ad->common.u.net->family])
		audit_log_format(ab, " family=\"%s\"",
				 address_family_names[ad->common.u.net->family]);
	else
		audit_log_format(ab, " family=\"unknown(%d)\"",
				 ad->common.u.net->family);
	if (sock_type_names[ad->net.type])
		audit_log_format(ab, " sock_type=\"%s\"",
				 sock_type_names[ad->net.type]);
	else
		audit_log_format(ab, " sock_type=\"unknown(%d)\"",
				 ad->net.type);
	audit_log_format(ab, " protocol=%d", ad->net.protocol);

	if (ad->request & NET_PERMS_MASK) {
		audit_log_format(ab, " requested_mask=");
		aa_audit_perm_mask(ab, ad->request, NULL, 0,
				   net_mask_names, NET_PERMS_MASK);

		if (ad->denied & NET_PERMS_MASK) {
			audit_log_format(ab, " denied_mask=");
			aa_audit_perm_mask(ab, ad->denied, NULL, 0,
					   net_mask_names, NET_PERMS_MASK);
		}
	}
	if (ad->common.u.net->family == AF_UNIX) {
		if ((ad->request & ~NET_PEER_MASK) && ad->net.addr)
			audit_unix_addr(ab, "addr",
					unix_addr(ad->net.addr),
					ad->net.addrlen);
		else
			audit_unix_sk_addr(ab, "addr", ad->common.u.net->sk);
		if (ad->request & NET_PEER_MASK) {
			if (ad->net.addr)
				audit_unix_addr(ab, "peer_addr",
						unix_addr(ad->net.addr),
						ad->net.addrlen);
			else
				audit_unix_sk_addr(ab, "peer_addr",
						   ad->net.peer_sk);
		}
	}
	if (ad->peer) {
		audit_log_format(ab, " peer=");
		aa_label_xaudit(ab, labels_ns(ad->subj_label), ad->peer,
				FLAGS_NONE, GFP_ATOMIC);
	}
}

/* Generic af perm */
int aa_profile_af_perm(struct aa_profile *profile, struct apparmor_audit_data *ad,
		       u32 request, u16 family, int type)
{
	struct aa_ruleset *rules = list_first_entry(&profile->rules,
						    typeof(*rules), list);
	struct aa_perms perms = { };
	aa_state_t state;
	__be16 buffer[2];

	AA_BUG(family >= AF_MAX);
	AA_BUG(type < 0 || type >= SOCK_MAX);

	if (profile_unconfined(profile))
		return 0;
	state = RULE_MEDIATES(rules, AA_CLASS_NET);
	if (state) {
		buffer[0] = cpu_to_be16(family);
		buffer[1] = cpu_to_be16((u16) type);
		state = aa_dfa_match_len(rules->policy.dfa, state,
					 (char *) &buffer, 4);
		perms = *aa_lookup_perms(&rules->policy, state);
	} else if (profile->net_compat) {
		/* 2.x socket mediation compat */
		perms.allow = (profile->net_compat->allow[family] & (1 << type)) ?
			ALL_PERMS_MASK : 0;
		perms.audit = (profile->net_compat->audit[family] & (1 << type)) ?
			ALL_PERMS_MASK : 0;
		perms.quiet = (profile->net_compat->quiet[family] & (1 << type)) ?
			ALL_PERMS_MASK : 0;

	} else {
		return 0;
	}
	aa_apply_modes_to_perms(profile, &perms);

	return aa_check_perms(profile, &perms, request, ad, audit_net_cb);
}

int aa_af_perm(const struct cred *subj_cred, struct aa_label *label,
	       const char *op, u32 request, u16 family, int type, int protocol)
{
	struct aa_profile *profile;
	DEFINE_AUDIT_NET(ad, op, NULL, family, type, protocol);

	return fn_for_each_confined(label, profile,
			aa_profile_af_perm(profile, &ad, request, family,
					   type));
}

static int aa_label_sk_perm(const struct cred *subj_cred,
			    struct aa_label *label,
			    const char *op, u32 request,
			    struct sock *sk)
{
	struct aa_sk_ctx *ctx = aa_sock(sk);
	int error = 0;

	AA_BUG(!label);
	AA_BUG(!sk);

	if (ctx->label != kernel_t && !unconfined(label)) {
		struct aa_profile *profile;
		DEFINE_AUDIT_SK(ad, op, sk);

		ad.subj_cred = subj_cred;
		error = fn_for_each_confined(label, profile,
			    aa_profile_af_sk_perm(profile, &ad, request, sk));
	}

	return error;
}

int aa_sk_perm(const char *op, u32 request, struct sock *sk)
{
	struct aa_label *label;
	int error;

	AA_BUG(!sk);
	AA_BUG(in_interrupt());

	/* TODO: switch to begin_current_label ???? */
	label = begin_current_label_crit_section();
	error = aa_label_sk_perm(current_cred(), label, op, request, sk);
	end_current_label_crit_section(label);

	return error;
}


int aa_sock_file_perm(const struct cred *subj_cred, struct aa_label *label,
		      const char *op, u32 request, struct socket *sock)
{
	AA_BUG(!label);
	AA_BUG(!sock);
	AA_BUG(!sock->sk);

	return af_select(sock->sk->sk_family,
			 file_perm(subj_cred, label, op, request, sock),
			 aa_label_sk_perm(subj_cred, label, op, request,
					  sock->sk));
}

#ifdef CONFIG_NETWORK_SECMARK
static int apparmor_secmark_init(struct aa_secmark *secmark)
{
	struct aa_label *label;

	if (secmark->label[0] == '*') {
		secmark->secid = AA_SECID_WILDCARD;
		return 0;
	}

	label = aa_label_strn_parse(&root_ns->unconfined->label,
				    secmark->label, strlen(secmark->label),
				    GFP_ATOMIC, false, false);

	if (IS_ERR(label))
		return PTR_ERR(label);

	secmark->secid = label->secid;

	return 0;
}

static int aa_secmark_perm(struct aa_profile *profile, u32 request, u32 secid,
			   struct apparmor_audit_data *ad)
{
	int i, ret;
	struct aa_perms perms = { };
	struct aa_ruleset *rules = list_first_entry(&profile->rules,
						    typeof(*rules), list);

	if (rules->secmark_count == 0)
		return 0;

	for (i = 0; i < rules->secmark_count; i++) {
		if (!rules->secmark[i].secid) {
			ret = apparmor_secmark_init(&rules->secmark[i]);
			if (ret)
				return ret;
		}

		if (rules->secmark[i].secid == secid ||
		    rules->secmark[i].secid == AA_SECID_WILDCARD) {
			if (rules->secmark[i].deny)
				perms.deny = ALL_PERMS_MASK;
			else
				perms.allow = ALL_PERMS_MASK;

			if (rules->secmark[i].audit)
				perms.audit = ALL_PERMS_MASK;
		}
	}

	aa_apply_modes_to_perms(profile, &perms);

	return aa_check_perms(profile, &perms, request, ad, audit_net_cb);
}

int apparmor_secmark_check(struct aa_label *label, char *op, u32 request,
			   u32 secid, const struct sock *sk)
{
	struct aa_profile *profile;
	DEFINE_AUDIT_SK(ad, op, sk);

	return fn_for_each_confined(label, profile,
				    aa_secmark_perm(profile, request, secid,
						    &ad));
}
#endif
