/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * AppArmor security module
 *
 * This file contains AppArmor inet fine grained mediation
 *
 * Copyright 2024 Canonical Ltd.
 *
 */

#include <linux/sctp.h>
#include <net/tcp_states.h>
#include <net/sctp/structs.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/inet_connection_sock.h>
#include <net/net_namespace.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#include "include/audit.h"
#include "include/af_inet.h"
#include "include/apparmor.h"
#include "include/file.h"
#include "include/label.h"
#include "include/net.h"
#include "include/path.h"
#include "include/policy.h"
#include "include/cred.h"
#include "include/secid.h"

static inline aa_state_t RULE_MEDIATES_SK(struct aa_ruleset *rules,
					  const struct sock *sk)
{
	return RULE_MEDIATES_NET(rules);
}


enum addr_type {
	ADDR_LOCAL = 0,
	ADDR_LOCAL_PRIV	= 1,
	ADDR_REMOTE = 2,
};

struct match_addr {
	const char *addrp;
	enum addr_type addrtype;
	int len;
	__be16 port;
};

struct stored_match_addr {
	union {
		struct sockaddr addr;
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
	};
	int addrlen;
	struct match_addr maddr;
};

static void set_ad_create(struct apparmor_audit_data *ad,
			 int family, int type, int protocol)
{
	ad->common.u.net->family = family;
	ad->net.type = type;
	ad->net.protocol = protocol;
}

static int set_ad_addr(struct apparmor_audit_data *ad,
		       u16 family, bool source, struct match_addr *maddr)
{
	ad->common.u.net->family = family;

	if (source) {
		ad->common.u.net->sport = maddr->port;
		if (maddr->addrp) {
			if (family == AF_INET)
				/* ad.u.net->v4info.saddr = addr4->sin_addr.s_addr; */
				ad->common.u.net->v4info.saddr = *(__be32 *)maddr->addrp;
			else
				/* ad.u.net->v4info.saddr = addr6->sin6_addr.s6_addr; */
				ad->common.u.net->v6info.saddr = *(struct in6_addr *)maddr->addrp;
		}
	} else {
		ad->common.u.net->dport = maddr->port;
		if (maddr->addrp) {
			if (family == AF_INET)
				/* ad.u.net->v4info.saddr = addr4->sin_addr.s_addr; */
				ad->common.u.net->v4info.daddr = *(__be32 *)maddr->addrp;
			else
				/* ad.u.net->v4info.saddr = addr6->sin6_addr.s6_addr; */
				ad->common.u.net->v6info.daddr = *(struct in6_addr *)maddr->addrp;
		}
	}
	return 0;
}

/* returns 0 on success
* raw_port - if set raw_port (protocol) when SOCK_RAW */
static int map_addr(struct sockaddr *addr, int addrlen, u16 raw_port,
		    enum addr_type addrtype, struct match_addr *maddr,
		    struct apparmor_audit_data *ad)
{
	struct sockaddr_in *addr4 = NULL;
	struct sockaddr_in6 *addr6 = NULL;

	AA_BUG(!addr);
	AA_BUG(!maddr);

	maddr->addrtype = addrtype;
	if (!addr || addrlen < offsetofend(struct sockaddr, sa_family)) {
		maddr->addrp = NULL;
		maddr->port = 0;
		maddr->len = 0;
		return 0;
	}

	/*
	 * its possibly to have sk->sk_family == PF_INET6 and
	 * addr->sa_family == AF_INET. sk_family is used for socket
	 * mediation, sa_family for when we have address ...
	 */
	switch (addr->sa_family) {
	case AF_INET:
		addr4 = (struct sockaddr_in *)addr;
		if (addrlen < sizeof(struct sockaddr_in))
			return -EINVAL;
		maddr->port = addr4->sin_port;
		maddr->addrp = (char *)&addr4->sin_addr.s_addr;
		maddr->len = 4;
		break;
	case AF_INET6:
		addr6 = (struct sockaddr_in6 *)addr;
		if (addrlen < SIN6_LEN_RFC2133)
			return -EINVAL;
		maddr->port = addr6->sin6_port;
		maddr->addrp = (char *)&addr6->sin6_addr.s6_addr;
		maddr->len = 16;
		break;
	default:
		return -EAFNOSUPPORT;
	}
	/* per ip spec, && sk->sk_type == SOCK_RAW*/
	if (raw_port && addrtype != ADDR_REMOTE)
		maddr->port = htons(raw_port);
	if (ad)
		set_ad_addr(ad, addr->sa_family, addrtype != ADDR_REMOTE, maddr);

	return 0;
}

/* -ENOTCONN if not connected */
static int map_sock_addr(struct socket *sock, enum addr_type addrtype,
			 struct stored_match_addr *maddr,
			 struct apparmor_audit_data *ad)
{
	/* do we need early bailout for !family ... */
	maddr->addrlen = sock->ops->getname(sock, (struct sockaddr *) &maddr->addr, addrtype != ADDR_REMOTE ? 0 : 1);
	if (maddr->addrlen == -ENOTCONN) {
		maddr->addrlen = 0;
		return map_addr(NULL, 0, 0, addrtype, &maddr->maddr, ad);
	} else if (maddr->addrlen < 0)
		return maddr->addrlen;
	return map_addr(&maddr->addr, maddr->addrlen, 0, addrtype,
			&maddr->maddr, ad);
}

/* TODO: combine with connect map addr */
/* TODO: raw_port */
static int bind_map_addr(const struct sock *sk, struct sockaddr *addr,
			 int addrlen,
			 struct match_addr *maddr,
			 struct apparmor_audit_data *ad)
{
	struct sockaddr_in *addr4 = NULL;
	struct sockaddr_in6 *addr6 = NULL;
	u16 family;

	AA_BUG(!sk);
	AA_BUG(!addr);
	AA_BUG(!maddr);

	if (addrlen < offsetofend(struct sockaddr, sa_family))
		return -EINVAL;

	maddr->addrtype = ADDR_LOCAL;
	/*
	 * its possibly to have sk->sk_family == PF_INET6 and
	 * addr->sa_family == AF_INET. sk_family is used for socket
	 * mediation, sa_family for when we have address ...
	 */
	family = addr->sa_family;
	switch (addr->sa_family) {
	case AF_UNSPEC:
		if (sk->sk_family == PF_INET6) {
			/* Length check from inet6_bind_sk() */
			if (addrlen < SIN6_LEN_RFC2133)
				return -EINVAL;
			/* Family check from __inet6_bind() */
			return -EAFNOSUPPORT;
		}
		/* see __inet_bind(), we only want to allow
		 * AF_UNSPEC if the address is INADDR_ANY
		 */
		addr4 = (struct sockaddr_in *)addr;
		if (addr4->sin_addr.s_addr != htonl(INADDR_ANY))
			return -EAFNOSUPPORT;
		family = AF_INET;
		fallthrough;
	case AF_INET:
		addr4 = (struct sockaddr_in *)addr;
		if (addrlen < sizeof(struct sockaddr_in))
			return -EINVAL;
		maddr->port = addr4->sin_port;
		maddr->addrp = (char *)&addr4->sin_addr.s_addr;
		maddr->len = 4;
		break;
	case AF_INET6:
		addr6 = (struct sockaddr_in6 *)addr;
		if (addrlen < SIN6_LEN_RFC2133)
			return -EINVAL;
		maddr->port = addr6->sin6_port;
		maddr->addrp = (char *)&addr6->sin6_addr.s6_addr;
		maddr->len = 16;
		break;
	default:
		return -EAFNOSUPPORT;
	}

	if (ad)
		set_ad_addr(ad, family, true, maddr);

	return 0;
}

/* only continue match if
 *   insufficient current perms at current state
 *   indicates there are more perms in later state
 * Returns: perms struct if early match
 */
static struct aa_perms *early_match(struct aa_policydb *policy,
				    aa_state_t state, u32 request)
{
	struct aa_perms *p;

	p = aa_lookup_perms(policy, state);
	if (((p->allow & request) != request) && (p->allow & AA_CONT_MATCH))
		return NULL;
	return p;
}

static int do_perms(struct aa_profile *profile, aa_state_t state, u32 request,
		    const struct aa_perms *p, struct apparmor_audit_data *ad)
{
	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms perms;

	AA_BUG(!profile);

	if (state || !p)
		p = aa_lookup_perms(rules->policy, state);
	perms = *p;
	aa_apply_modes_to_perms(profile, &perms);
	return aa_check_perms(profile, &perms, request, ad,
			      audit_net_cb);
}

static aa_state_t match_addr(const struct aa_dfa *dfa, aa_state_t state,
			     struct match_addr *maddr)
{
	char l = (char) maddr->addrtype;

	state = aa_dfa_match_len(dfa, state, &l, 1);
	state = aa_dfa_match_len(dfa, state, (char *)&maddr->port, 2);
	if (maddr->len == 0 && !maddr->addrp) {
		l = 0;
	} else if (maddr->len == 4) {
		l = 1;
	} else if (maddr->len == 16) {
		l = 2;
	} else {
		AA_BUG("address length unsupported");
		return 0;
	}
	state = aa_dfa_match_len(dfa, state, &l, 1);
	if (maddr->addrp)
		state = aa_dfa_match_len(dfa, state, maddr->addrp, maddr->len);
	/* null transition between addr and label */
	state = aa_dfa_null_transition(dfa, state);

	return state;
}


static aa_state_t match_addr_info(const struct aa_dfa *dfa, aa_state_t state,
				  struct match_addr *maddr,
				  const char **info)
{
	state = match_addr(dfa, state, maddr);
	if (!state) {
		*info = maddr->addrtype == ADDR_REMOTE ?
			"failed remote addr match" :
			"failed local addr match";
	}

	return state;
}

static aa_state_t match_addr_label(struct aa_policydb *policy, aa_state_t state,
				   u32 request, struct match_addr *maddr,
				   struct aa_perms **p, const char **info)
{
	state = match_addr_info(policy->dfa, state, maddr, info);
	*p = early_match(policy, state, request);
	if (!*p) {
		/* TODO: actual label match: */
		if (!state) {
			*info = maddr->addrtype == ADDR_REMOTE ?
				"failed remote label match" :
				"failed local label match";
		}

		/* null transition after label match */
		state = aa_dfa_null_transition(policy->dfa, state);
	}

	return state;
}


static aa_state_t match_to_sk(struct aa_policydb *policy, aa_state_t state,
			      u32 request, const struct sock *sk,
			      struct match_addr *maddr,
			      struct aa_perms **p, const char **info)
{
	*p = NULL;
	state = aa_match_to_prot(policy, state, request, sk->sk_family,
				 sk->sk_type, sk->sk_protocol, p, info);
	if (*p || !state)
		return state;
	return match_addr_label(policy, state, request, maddr, p, info);
}

enum cmd_type {
	CMD_ADDR = 1,
	CMD_LISTEN = 2,
	CMD_OPT = 4,
};

static inline aa_state_t match_to_cmd(struct aa_policydb *policy,
				      aa_state_t state, u32 request,
				      const struct sock *sk, enum cmd_type cmd,
				      struct match_addr *maddr,
				      struct aa_perms **p, const char **info)
{
	state = match_to_sk(policy, state, request, sk, maddr, p, info);
	if (!*p && state) {
		char c = (char) cmd;
		state = aa_dfa_match_len(policy->dfa, state, &c, 1);
		if (!state)
			*info = "failed cmd selection match";
	}

	return state;
}

/*
static int match_label(struct aa_profile *profile, struct aa_profile *peer,
			      aa_state_t state, u32 request,
			      struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!peer);

	ad->peer = &peer->label;

	if (state) {
		state = aa_dfa_match(profile->policy.dfa, state,
				     peer->base.hname);
		if (!state)
			ad->info = "failed peer label match";
	}
	return do_perms(profile, state, request, ad);
}
*/

/* ---------------------------------------------------------------------- */


static inline int profile_sk_perm(struct aa_profile *profile, u32 request,
				  const struct sock *sk,
				  struct match_addr *maddr,
				  struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;

	state = RULE_MEDIATES_NET(rules);
	if (state) {
		state = match_to_sk(rules->policy, state, request, sk,
				    maddr, &p, &ad->info);
		return do_perms(profile, state, request, p, ad);
	}

	return aa_profile_af_sk_perm(profile, ad, request, sk);
}

/* no kernel_t bailout */
static int profile_create_perm(struct aa_profile *profile, int family,
			       int type, int protocol,
			       struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;

	state = RULE_MEDIATES_NET(rules);
	if (state) {
		state = aa_match_to_prot(rules->policy, state, AA_MAY_CREATE,
					 family, type, protocol, &p, &ad->info);
		return do_perms(profile, state, AA_MAY_CREATE, p, ad);
	}

	return aa_profile_af_compat_perm(profile, ad, AA_MAY_CREATE, family,
					 type);
}


/* sendmsg/rcvmsg/connect */
static int profile_remote_perm(struct aa_profile *profile,
			       const struct sock *sk,
			       u32 request, struct match_addr *raddr,
			       struct match_addr *laddr,
			       struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!raddr);
	AA_BUG(!laddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;

	state = RULE_MEDIATES_SK(rules, sk);
	if (!state)
		return aa_profile_af_sk_perm(profile, ad, request, sk);

	/* TODO: deal with sa_family vs. sk_family */
	state = match_to_cmd(rules->policy, state, request, sk, CMD_ADDR,
			     raddr, &p, &ad->info);
	if (state && !p)
		/* check if perm is restricted to a pairing */
		state = match_addr_label(rules->policy, state, request,
					 laddr, &p, &ad->info);
	return do_perms(profile, state, request, p, ad);
}

static int profile_bind_perm(struct aa_profile *profile,
			     const struct sock *sk,
			     struct match_addr *maddr,
			     struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!maddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;
	unsigned short sport;

	state = RULE_MEDIATES_SK(rules, sk);
	if (!state)
		return aa_profile_af_sk_perm(profile, ad, AA_MAY_BIND, sk);

	/*
	 * its possibly to have sk->sk_family == PF_INET6 and
	 * addr->sa_family == AF_INET
	 */
	sport = ntohs(maddr->port);
	if (sport) {
		if (inet_port_requires_bind_service(sock_net(sk), sport)) {
			/* cap NET_BIND_SERVICE will get raised */
			maddr->addrtype = ADDR_LOCAL_PRIV;
		}
	}
	state = match_to_sk(rules->policy, state, AA_MAY_BIND, sk,
			    maddr, &p, &ad->info);
	return do_perms(profile, state, AA_MAY_BIND, p, ad);
}

static int profile_listen_perm(struct aa_profile *profile,
			       const struct sock *sk,
			       struct match_addr *maddr, int backlog,
			       struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!maddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;


	state = RULE_MEDIATES_SK(rules, sk);
	if (state) {
		__be16 b = htons(backlog);

		state = match_to_cmd(rules->policy, state, AA_MAY_LISTEN, sk,
				     CMD_LISTEN, maddr, &p, &ad->info);
		if (state && !p) {
			state = aa_dfa_match_len(rules->policy->dfa, state,
						 (char *) &b, 2);
			if (!state)
				ad->info = "failed listen backlog match";
		}
		return do_perms(profile, state, AA_MAY_LISTEN, p, ad);
	}

	return aa_profile_af_sk_perm(profile, ad, AA_MAY_LISTEN, sk);
}

static int profile_accept_perm(struct aa_profile *profile,
			       const struct sock *sk,
			       struct match_addr *maddr,
			       const struct sock *newsk,
			       struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);
	/* AA_BUG(!newsk);  newsk can be null here, since not using atm ... */
	AA_BUG(!maddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;

	state = RULE_MEDIATES_SK(rules, sk);
	if (state) {
		state = match_to_sk(rules->policy, state, AA_MAY_ACCEPT, sk,
				    maddr, &p, &ad->info);
		return do_perms(profile, state, AA_MAY_ACCEPT, p, ad);
	}

	return aa_profile_af_sk_perm(profile, ad, AA_MAY_ACCEPT, sk);
}

/* getopt/setopt */
static int profile_opt_perm(struct aa_profile *profile, u32 request,
			    const struct sock *sk, struct match_addr *maddr,
			    int level, int optname,
			    struct apparmor_audit_data *ad)
{
	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!maddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms *p = NULL;
	aa_state_t state;

	state = RULE_MEDIATES_SK(rules, sk);
	if (state) {
		__be16 l = htons(level);
		__be16 n = htons(optname);

		state = match_to_cmd(rules->policy, state, request, sk,
				     CMD_OPT, maddr, &p, &ad->info);
		if (state && !p) {
			state = aa_dfa_match_len(rules->policy->dfa, state,
						 (char *) &l, 2);
			state = aa_dfa_match_len(rules->policy->dfa, state,
						 (char *) &n, 2);
			if (!state)
				ad->info = "failed sockopt match";
		}
		return do_perms(profile, state, request, p, ad);
	}

	return aa_profile_af_sk_perm(profile, ad, request, sk);
}

/* ---------------------------------------------------------------------- */

// TODO: cleanup init to use recursion, so we can have N init fns, in 1 macro
// TODO: lift DEFINE_AUDIT out of macro into init fn???

/* no kernel_t bailout */
#define label_sk_has_perm2(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, XXXX, YYYY, CALLBACKFN) \
({								\
	int __EERROR = 0;					\
	if (label_mediates(LABEL, AA_CLASS_NET)) {		\
		struct aa_profile *PROFILE;			\
		DEFINE_AUDIT_SK(AAD, OP, CRED, SOCKSK);		\
		(AAD).subj_cred = (CRED);			\
		(AAD).request = (REQUEST);			\
		__EERROR = (XXXX);				\
		if (__EERROR == 0) {				\
			__EERROR = (YYYY);			\
			if (__EERROR == 0) {			\
				__EERROR = fn_for_each(LABEL, PROFILE,	\
						       (CALLBACKFN));	\
			}						\
		}							\
	}							\
	__EERROR;						\
})

/* no kernel_t bailout */
#define label_sk_has_perm(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, CALLBACKFN) \
	label_sk_has_perm2(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, \
			   0, 0, CALLBACKFN)

/* no kernel_t bailout */
#define label_sk_has_perm1(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, XXXX, CALLBACKFN) \
	label_sk_has_perm2(CRED, LABEL, SOCKSK, OP, REQUEST, PROFILE, AAD, \
			   XXXX, 0, CALLBACKFN)


/* Early bailout for kernel_t - 2 init args before callback */
#define sk_has_perm2(SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, YYYYX, CALLBACKFN) \
({									\
	struct aa_label *__label;					\
	struct aa_sk_ctx *__ctx = aa_sock(SOCKSK);			\
	int __ERROR = 0;						\
	bool __needput;						\
	if (rcu_access_pointer(__ctx->label) != kernel_t) {		\
									\
		__label = begin_current_label_crit_section(&__needput);	\
                __ERROR = label_sk_has_perm2(current_cred(), __label, SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, YYYYX, CALLBACKFN); \
		end_current_label_crit_section(__label, __needput);	\
	}								\
	__ERROR;							\
})

/* Early bailout for kernel_t - no init args before callback */
#define sk_has_perm(SOCKSK, OP, REQUEST, PROFILE, AAD, CALLBACKFN)	\
	sk_has_perm2(SOCKSK, OP, REQUEST, PROFILE, AAD, 0, 0, CALLBACKFN)


/* Early bailout for kernel_t - 1 init arg before callback */
#define sk_has_perm1(SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, CALLBACKFN) \
	sk_has_perm2(SOCKSK, OP, REQUEST, PROFILE, AAD, XXXXY, 0, CALLBACKFN)



/* no kernel_t early bailout */
/* NOTE: already lifted label_mediates into lsm.c */
int aa_inet_create_perm(struct aa_label *label, int family, int type,
			int protocol)
{
	struct aa_profile *profile;
	int error = 0;
	DEFINE_AUDIT_NET(ad, OP_CREATE, current_cred(), NULL, family, type,
			 protocol);

	ad.subj_cred = current_cred();
	set_ad_create(&ad, family, type, protocol);
	error = fn_for_each(label, profile,
			    profile_create_perm(profile, family, type,
						protocol, &ad));


	return error;
}

int aa_inet_bind_perm(struct socket *sock, struct sockaddr *addr,
		      int addrlen)
{
	struct match_addr maddr;

	return sk_has_perm1(sock->sk, OP_BIND, AA_MAY_BIND, profile, ad,
			    bind_map_addr(sock->sk, addr, addrlen, &maddr,
					  &ad),
			    profile_bind_perm(profile, sock->sk, &maddr, &ad));
}

int aa_inet_connect_perm(struct socket *sock, struct sockaddr *addr,
			 int addrlen)
{
	struct stored_match_addr laddr;
	struct match_addr raddr;

	/* disconnect socket */
	if (addrlen < offsetofend(struct sockaddr, sa_family))
		return -EINVAL;
	if (addr->sa_family == AF_UNSPEC)
		return 0;

	/* do we need early bailout for !family ... */
	return sk_has_perm2(sock->sk, OP_CONNECT, AA_MAY_CONNECT, profile, ad,
			    map_sock_addr(sock, ADDR_LOCAL, &laddr, &ad),
			    map_addr(addr, addrlen, 0, ADDR_REMOTE, &raddr,
				     &ad),
			    profile_remote_perm(profile, sock->sk,
						AA_MAY_CONNECT, &raddr,
						&laddr.maddr, &ad));
}

int aa_inet_listen_perm(struct socket *sock, int backlog)
{
	struct stored_match_addr maddr;

	/* do we need early bailout for !family ... */
	return sk_has_perm1(sock->sk, OP_LISTEN, AA_MAY_LISTEN, profile, ad,
			    map_sock_addr(sock, ADDR_LOCAL, &maddr, &ad),
			    profile_listen_perm(profile, sock->sk, &maddr.maddr,
						backlog, &ad));
}

/* ability of sock to connect, not peer address binding */
int aa_inet_accept_perm(struct socket *sock, struct socket *newsock)
{
	struct stored_match_addr maddr;
	int error;

	error = sk_has_perm1(sock->sk, OP_ACCEPT, AA_MAY_ACCEPT, profile, ad,
			     map_sock_addr(sock, ADDR_LOCAL, &maddr, &ad),
			     profile_accept_perm(profile, sock->sk,
						 &maddr.maddr,
						 newsock->sk, &ad));

	/* selinux updates inode - need to investigate this more */
	return error;
}

/* sendmsg, recvmsg. */
int aa_inet_msg_perm(const char *op, u32 request, struct socket *sock,
		     struct msghdr *msg, int size)
{
	struct stored_match_addr laddr;
	struct match_addr raddr;

	/* do we need early bailout for !family ... */
	return sk_has_perm2(sock->sk, op, request, profile, ad,
			    map_sock_addr(sock, ADDR_LOCAL, &laddr, &ad),
			    map_addr(msg->msg_name, msg->msg_namelen, 0,
				     ADDR_REMOTE, &raddr, &ad),
			    profile_remote_perm(profile, sock->sk, request,
						&raddr, &laddr.maddr, &ad));
}

/* getopt, setopt */
int aa_inet_opt_perm(const char *op, u32 request, struct socket *sock,
		     int level, int optname)
{
	struct stored_match_addr maddr;

	return sk_has_perm1(sock->sk, op, request, profile, ad,
			    map_sock_addr(sock, ADDR_LOCAL, &maddr, &ad),
			    profile_opt_perm(profile, request, sock->sk,
					    &maddr.maddr, level, optname, &ad));
}

static int inet_label_sock_perm(const struct cred *cred, struct aa_label *label,
				const char *op, u32 request,
				struct socket *sock)
{
	struct stored_match_addr maddr;

	return label_sk_has_perm1(cred, label, sock->sk, op, request, profile,
				  ad,
			map_sock_addr(sock, ADDR_LOCAL, &maddr, &ad),
			profile_sk_perm(profile, request, sock->sk,
					&maddr.maddr, &ad));
}

/* revalidation, get/set attr/getsockname/peername */
int aa_inet_sock_perm(const char *op, u32 request, struct socket *sock)
{
	struct aa_sk_ctx *ctx = aa_sock(sock->sk);
	struct aa_label *label;
	bool needput;
	int error;

	if (rcu_access_pointer(ctx->label) == kernel_t)
		return 0;

	label = begin_current_label_crit_section(&needput);
	error = inet_label_sock_perm(current_cred(), label, op, request, sock);
	end_current_label_crit_section(label, needput);

	return error;
}

int aa_inet_file_perm(const struct cred *subj_cred, struct aa_label *label,
		      const char *op, u32 request, struct socket *sock)
{
	u32 sk_req = request & ~NET_PEER_MASK;
	struct stored_match_addr laddr;
	const struct sock *sk = sock->sk;
	int error = 0;

	AA_BUG(!label);
	AA_BUG(!sock);
	AA_BUG(!sock->sk);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	/* access to the local sock */
	error = label_sk_has_perm1(subj_cred, label, sock->sk, op, request,
				   profile, ad,
			map_sock_addr(sock, ADDR_LOCAL, &laddr, &ad),
			profile_sk_perm(profile, sk_req, sock->sk, &laddr.maddr,
					&ad));

	if (!error) {
		struct stored_match_addr raddr;

		/* TODO: have ad here: instead of in CB so we do have to redo */
		error = map_sock_addr(sock, ADDR_REMOTE, &raddr, NULL);
		if (!error && raddr.maddr.addrp) {
			error = label_sk_has_perm1(subj_cred, label, sock->sk,
						   op, request, profile, ad,
					set_ad_addr(&ad, raddr.addr.sa_family,
						    false, &raddr.maddr),
					profile_remote_perm(profile, sock->sk,
							    request,
							    &raddr.maddr,
							    &laddr.maddr, &ad));
		}
	}

	return error;
}

/* ------------------------ skb / interface ---------------------------- */

/* no kernel_t bailout */
#define label_skb_has_perm2(CRED, LABEL, SOCKSK, SKB, OP, REQUEST,	\
			    PROFILE, AAD, XXXX, YYYY, CALLBACKFN)	\
({									\
	int __EERROR = 0;						\
	if (label_mediates(LABEL, AA_CLASS_NETV9_SKB)) {		\
		struct aa_profile *PROFILE;				\
		DEFINE_AUDIT_SKB(AAD, OP, CRED, SOCKSK, SKB);		\
		(AAD).subj_cred = (CRED);				\
		(AAD).request = (REQUEST);				\
		__EERROR = (XXXX);					\
		if (__EERROR == 0) {					\
			__EERROR = (YYYY);				\
			if (__EERROR == 0) {				\
				__EERROR = fn_for_each(label, PROFILE,	\
						       (CALLBACKFN));	\
			}						\
		}							\
	}								\
	__EERROR;							\
})

/* no kernel_t bailout */
#define label_skb_has_perm(CRED, LABEL, SOCKSK, SKB, OP, REQUEST,	\
			   PROFILE, AAD, CALLBACKFN)			\
	label_skb_has_perm2(CRED, LABEL, SOCKSK, SKB, OP, REQUEST,	\
			    PROFILE, AAD, 0, 0, CALLBACKFN)

/* no kernel_t bailout */
#define label_skb_has_perm1(CRED, LABEL, SOCKSK, SKB, OP, REQUEST,	\
			    PROFILE, AAD, XXXX, CALLBACKFN)		\
	label_skb_has_perm2(CRED, LABEL, SOCKSK, SKB, OP, REQUEST,	\
			    PROFILE, AAD, XXXX, 0, CALLBACKFN)


/* Early bailout for kernel_t - 2 init args before callback */
#define skb_has_perm2(SOCKSK, SKB, OP, REQUEST, PROFILE, AAD, XXXXY,	\
		      YYYYX, CALLBACKFN)				\
({									\
	struct aa_label *label;						\
	struct aa_sk_ctx *ctx = aa_sock(SOCKSK);			\
	int __ERROR = 0;						\
	bool needput;							\
	if (rcu_access_pointer(ctx->label) != kernel_t) {		\
		label = begin_current_label_crit_section(&needput);	\
		__ERROR = label_skb_has_perm2(current_cred(), label,	\
				SOCKSK, SKB, OP, REQUEST, PROFILE, AAD,	\
				XXXXY, YYYYX, CALLBACKFN);		\
		end_current_label_crit_section(label, needput);		\
	}								\
	__ERROR;							\
})

/* Early bailout for kernel_t - no init args before callback */
#define skb_has_perm(SOCKSK, SKB, OP, REQUEST, PROFILE, AAD,		\
		     CALLBACKFN)					\
	sk_has_perm2(SOCKSK, SKB, OP, REQUEST, PROFILE, AAD, 0, 0, CALLBACKFN)


/* Early bailout for kernel_t - 1 init arg before callback */
#define skb_has_perm1(SOCKSK, SKB, OP, REQUEST, PROFILE, AAD, XXXXY,	\
		      CALLBACKFN)					\
	sk_has_perm2(SOCKSK, SKB, OP, REQUEST, PROFILE, AAD, XXXXY, 0,	\
		     CALLBACKFN)



/* based on selinux_parse_skb_ipv4 */
static int map_skb_ipv4_addrs(const struct sk_buff *skb,
			      enum addr_type saddrtype,
			      struct stored_match_addr *saddr,
			      enum addr_type daddrtype,
			      struct stored_match_addr *daddr,
			      struct apparmor_audit_data *ad)
{
	int offset, ihlen;
	struct iphdr _iph, *ih;
	u16 typ = 0;

	offset = skb_network_offset(skb);
	ih = skb_header_pointer(skb, offset, sizeof(_iph), &_iph);
	if (ih == NULL)
		return -EINVAL;

	ihlen = ih->ihl * 4;
	if (ihlen < sizeof(_iph))
		return -EINVAL;

	saddr->addr.sa_family = AF_INET;
	daddr->addr.sa_family = AF_INET;
	saddr->addr4.sin_addr.s_addr = ih->saddr;
	daddr->addr4.sin_addr.s_addr = ih->daddr;
	saddr->addrlen = 4;
	daddr->addrlen = 4;

	switch (ih->protocol) {
	case IPPROTO_TCP: {
		struct tcphdr _tcph, *th;

		if (ntohs(ih->frag_off) & IP_OFFSET)
			break;

		offset += ihlen;
		th = skb_header_pointer(skb, offset, sizeof(_tcph), &_tcph);
		if (th == NULL)
			break;

		saddr->addr4.sin_port = th->source;
		daddr->addr4.sin_port = th->dest;
		typ = SOCK_STREAM;
		break;
	}

	case IPPROTO_UDP: {
		struct udphdr _udph, *uh;

		if (ntohs(ih->frag_off) & IP_OFFSET)
			break;

		offset += ihlen;
		uh = skb_header_pointer(skb, offset, sizeof(_udph), &_udph);
		if (uh == NULL)
			break;

		ad->common.u.net->sport = uh->source;
		ad->common.u.net->dport = uh->dest;
		typ = SOCK_DGRAM;
		break;
	}
#if IS_ENABLED(CONFIG_IP_SCTP)
	case IPPROTO_SCTP: {
		struct sctphdr _sctph, *sh;
		struct socket *sock = skb->sk ? skb->sk->sk_socket : NULL;

		sh = skb_header_pointer(skb, offset, sizeof(_sctph), &_sctph);
		if (sh == NULL)
			break;

		ad->common.u.net->sport = sh->source;
		ad->common.u.net->dport = sh->dest;
		if (sock)
			typ = sock->type;
		break;
	}
#endif
	default:
		break;
	}

	if (ad) {
		ad->net.protocol = ih->protocol;
		ad->net.type = typ;
	}
	// LOCAL may change based whether sending or receiving
	map_addr(&saddr->addr, saddr->addrlen, 0, saddrtype, &saddr->maddr,
		 ad);
	map_addr(&daddr->addr, daddr->addrlen, 0, daddrtype, &daddr->maddr,
		 ad);

	return 0;
}

#if IS_ENABLED(CONFIG_IPV6)

/* based on selinux parse_skb_ipv6 */
static int map_skb_ipv6_addrs(const struct sk_buff *skb,
			      enum addr_type saddrtype,
			      struct stored_match_addr *saddr,
			      enum addr_type daddrtype,
			      struct stored_match_addr *daddr,
			      struct apparmor_audit_data *ad)
{
	u8 nexthdr;
	int offset;
	struct ipv6hdr _ipv6h, *ih;
	__be16 frag_off;
	u16 typ = 0;

	offset = skb_network_offset(skb);
	ih = skb_header_pointer(skb, offset, sizeof(_ipv6h), &_ipv6h);
	if (ih == NULL)
		return -EINVAL;

	saddr->addr.sa_family = AF_INET6;
	daddr->addr.sa_family = AF_INET6;
	saddr->addr6.sin6_addr = ih->saddr;
	daddr->addr6.sin6_addr = ih->daddr;
	saddr->addrlen = 16;
	daddr->addrlen = 16;

//	ad->common.u.net->v6info.saddr = ih->saddr;
//	ad->common.u.net->v6info.daddr = ih->daddr;

	nexthdr = ih->nexthdr;
	offset += sizeof(_ipv6h);
	offset = ipv6_skip_exthdr(skb, offset, &nexthdr, &frag_off);
	if (offset < 0)
		return -EINVAL;

	switch (nexthdr) {
	case IPPROTO_TCP: {
		struct tcphdr _tcph, *th;

		th = skb_header_pointer(skb, offset, sizeof(_tcph), &_tcph);
		if (th == NULL)
			break;

		saddr->addr6.sin6_port = th->source;
		daddr->addr6.sin6_port = th->dest;
		typ = SOCK_STREAM;
		break;
	}

	case IPPROTO_UDP: {
		struct udphdr _udph, *uh;

		uh = skb_header_pointer(skb, offset, sizeof(_udph), &_udph);
		if (uh == NULL)
			break;

		ad->common.u.net->sport = uh->source;
		ad->common.u.net->dport = uh->dest;
		typ = SOCK_DGRAM;
		break;
	}

#if IS_ENABLED(CONFIG_IP_SCTP)
	case IPPROTO_SCTP: {
		struct sctphdr _sctph, *sh;
		struct socket *sock = skb->sk ? skb->sk->sk_socket : NULL;

		sh = skb_header_pointer(skb, offset, sizeof(_sctph), &_sctph);
		if (sh == NULL)
			break;

		ad->common.u.net->sport = sh->source;
		ad->common.u.net->dport = sh->dest;
		if (sock)
			typ = sock->type;
		break;
	}
#endif
	/* includes fragments */
	default:
		break;
	}

	if (ad) {
		ad->net.protocol = nexthdr;
		ad->net.type = typ;
	}

	// LOCAL may change based whether sending or receiving
	map_addr(&saddr->addr, saddr->addrlen, 0, saddrtype, &saddr->maddr,
		 ad);
	map_addr(&daddr->addr, daddr->addrlen, 0, daddrtype, &daddr->maddr,
		 ad);
	return 0;
}

#endif /* IPV6 */

static int map_skb_addrs(const struct sk_buff *skb,
			 u16 family,
			 enum addr_type saddrtype,
			 struct stored_match_addr *saddr,
			 enum addr_type daddrtype,
			 struct stored_match_addr *daddr,
			 struct apparmor_audit_data *ad)
{
	int ret;

	/* handle mapped IPv4 packets arriving via IPv6 sockets */
	if (family == PF_INET6 && skb->protocol == htons(ETH_P_IP))
		family = PF_INET;

	ad->common.u.net->family = family;
	switch (family) {
	case PF_INET:
		ret = map_skb_ipv4_addrs(skb, saddrtype, saddr,
					 daddrtype, daddr, ad);
		if (ret)
			goto parse_error;
		break;

#if IS_ENABLED(CONFIG_IPV6)
	case PF_INET6:
		ret = map_skb_ipv6_addrs(skb, saddrtype, saddr,
					 daddrtype, daddr, ad);
		if (ret)
			goto parse_error;
		break;
#endif	/* IPV6 */
	default:
		break;
	}

	return 0;

parse_error:
	pr_warn("AppArmor: failure in %s, unable to map skb addr", __func__);
	return ret;
}


/* todo: optimize, do this once and buffer, have destruct to put at end */
static aa_state_t match_iface(const struct aa_profile *profile,
			      const struct aa_dfa *dfa, aa_state_t state,
			      const struct sock *sk, int ifidx,
			      const char **info)
{
	if (ifidx > 0) {
		struct net_device *dev;
		struct net *ns;

		/* TODO: check netns against profile ctx */
		rcu_read_lock();
		ns = sock_net(sk);
		dev = dev_get_by_index(ns, ifidx);
		if (dev) {
			/* TODO: share this so it is only done once */
		       state = aa_dfa_match(dfa, state, dev->name);
			dev_put(dev);
		} else {
			/* if lookup fails don't just skip interface check
			 * with null transition, force fail */
			state = 0;
		}
		rcu_read_unlock();
	} else if (ifidx == 0) {
		/* this should not happen, caller specifies -1 to by-pass
		 * interface check.
		 */
		  *info = "interface for packet not defined";
		  state = 0;
	}
	/* null transition after iface match */
	state = aa_dfa_null_transition(dfa, state);
	if (!state)
		*info = "failed interface match";

	return state;
}

static aa_state_t match_addr_iface(const struct aa_profile *profile,
				   struct aa_policydb *policy,
				   aa_state_t state, u32 request,
				   struct match_addr *maddr,
				   const struct sock *sk, int ifidx,
				   struct aa_perms **p, const char **info)
{
	state = match_addr_info(policy->dfa, state, maddr, info);
	*p = early_match(policy, state, request);
	if (*p)
		return state;
	state = match_iface(profile, policy->dfa, state, sk, ifidx, info);
	*p = early_match(policy, state, request);

	return state;
}

static aa_state_t match_addr_iface_label(const struct aa_profile *profile,
					 struct aa_ruleset *rules,
					 aa_state_t state,
					 u32 request, struct match_addr *maddr,
					 const struct sock *sk, int ifidx,
					 struct aa_label *seclabel,
					 struct aa_perms **p,
					 struct aa_perms *perms,
					 const char **info)
{
	state = match_addr_iface(profile, rules->policy, state, request, maddr,
				 sk, ifidx, p, info);
	if (*p)
		return state;

	if (seclabel)
		state = aa_label_match(profile, rules, seclabel, state, false,
				       request, perms);
	/* null transition after label match */
	state = aa_dfa_null_transition(rules->policy->dfa, state);

	if (!state) {
		*info = maddr->addrtype == ADDR_REMOTE ?
			"failed remote label match" :
			"failed local label match";
	}

	return state;
}

static aa_state_t skb_match_to_sk(const struct aa_profile *profile,
				  struct aa_ruleset *rules,
				  aa_state_t state, u32 request,
				  const struct sock *sk,
				  const struct sk_buff *skb,
				  struct match_addr *maddr,
				  int ifidx, struct aa_label *seclabel,
				  struct aa_perms **p, struct aa_perms *perms,
				  const char **info)
{
	u16 family = sk->sk_family;

	*p = NULL;

	/* handle mapped IPv4 packets arriving via IPv6 sockets */
	if (family == PF_INET6 && skb->protocol == htons(ETH_P_IP))
		family = PF_INET;

	state = aa_match_to_prot(rules->policy, state, request, sk->sk_family,
				 sk->sk_type, sk->sk_protocol, p, info);
	if (*p)
		return state;
	return match_addr_iface_label(profile, rules, state, request, maddr,
				      sk, ifidx, seclabel, p, perms, info);
}

static inline aa_state_t skb_match_to_cmd(const struct aa_profile *profile,
					  struct aa_ruleset *rules,
					  aa_state_t state, u32 request,
					  const struct sock *sk,
					  const struct sk_buff *skb,
					  enum cmd_type cmd,
					  struct match_addr *maddr,
					  int ifidx,
					  struct aa_label *seclabel,
					  struct aa_perms **p,
					  struct aa_perms *perms,
					  const char **info)
{
	state = skb_match_to_sk(profile, rules, state, request, sk, skb, maddr,
				ifidx, seclabel, p, perms, info);
	if (!*p && state) {
		char c = (char) cmd;

		state = aa_dfa_match_len(rules->policy->dfa, state, &c, 1);
		if (!state)
			*info = "failed cmd selection match";
	}

	return state;
}

static int profile_relabel_packet(struct aa_profile *profile,
				  struct aa_label *seclabel,
				  struct apparmor_audit_data *ad)
{
	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms perms = { };
	aa_state_t state;

	AA_BUG(!profile);
	AA_BUG(!seclabel);

	state = RULE_MEDIATES(rules, AA_CLASS_NETV9_SKB);
	if (!state)
		return 0;
	state = aa_dfa_outofband_transition(rules->policy->dfa, state);
	aa_label_match(profile, rules, seclabel, state, false, ad->request,
		       &perms);
	aa_apply_modes_to_perms(profile, &perms);
	return aa_check_perms(profile, &perms, ad->request, ad,
			      audit_net_cb);
}

/* TODO: make so we can move this into lsm.c */
int aa_secmark_relabel_packet(u32 sid)
{
	struct aa_label *label, *seclabel;
	int error = 0;
	bool needput;

	seclabel = aa_secid_to_label(sid);
	if (!seclabel)
		return -EINVAL;
	label = begin_current_label_crit_section(&needput);

	if (label_mediates(label, AA_CLASS_NETV9_SKB)) {
		struct aa_profile *profile;
		DEFINE_AUDIT_DATA(ad, LSM_AUDIT_DATA_NONE, AA_CLASS_NET,
				  OP_RELABEL_PACKET);
		ad.subj_cred = current_cred();
		ad.peer = seclabel;
		ad.request = AA_MAY_SETCRED;

		error = fn_for_each(label, profile,
				    profile_relabel_packet(profile, seclabel,
							   &ad));
	}

	end_current_label_crit_section(label, needput);

	return error;
}

/* rcu_read_lock held */
static int __profile_skb_perm(struct aa_profile *profile,
			      const struct sock *sk, const struct sk_buff *skb,
			      u32 request, struct match_addr *raddr,
			      int ifidx, struct aa_label *seclabel,
			      struct match_addr *laddr,
			      struct apparmor_audit_data *ad)
{
	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms perms = { };
	struct aa_perms *p = NULL;
	aa_state_t state;

	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!raddr);
	AA_BUG(!laddr);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	state = RULE_MEDIATES_SKB(rules);
	if (!state) {
		return apparmor_secmark_check(&profile->label,
					      ad->op, request,
					      skb->secmark, sk);
	}

	/* includes match on outbound iface */
	state = skb_match_to_cmd(profile, rules, state, request, sk, skb,
				 CMD_ADDR, raddr, ifidx, seclabel, &p, &perms,
				 &ad->info);
	if (state && !p) {
		struct aa_sk_ctx *ctx = aa_sock(sk);

		/* check if perm is restricted to a pairing */
		/* Note: ifidx on the second pairing is always -1,
		 * this is just present if we ever choose to support
		 * forward which has in and out iterfaces
		 */
		state = match_addr_iface_label(profile, rules, state, request,
					       laddr, sk, -1,
					       rcu_dereference(ctx->label),
					       &p, &perms, &ad->info);
       }
	return do_perms(profile, state, request, p, ad);
}

/**
 * __aa_sock_rcv_skb - check permission to receive skb
 * @label: label doing the mediation (label off of sk)
 * @sk: socket that is receiving the packet
 * @skb: skb that is being mediated
 *
 * Returns: 0 on success or -errno on failure/denial
 *
 * assumes called in rcu_read_lock
 * it is possible that the skb is unlabeled, by apparmor (which only will
 * label outbound packets if explicitly requested to do so) or netfilter.
 * At this point we have already determined the packet is being mediated,
 * so treat the unlabeled packet case as unlabeled_t
 */
int __aa_sock_rcv_skb(struct aa_label *label, const struct sock *sk,
		    const struct sk_buff *skb)
{
	struct stored_match_addr sladdr;
	struct stored_match_addr sraddr;
	struct aa_label *seclabel;
	int ifidx = skb->skb_iif;
	u32 request = AA_MAY_RECEIVE;

	AA_BUG(!label);
	AA_BUG(!sk);
	AA_BUG(!skb);

	seclabel = aa_secid_to_label(skb->secmark); /* may be NULL */
	if (!seclabel)
		seclabel = unlabeled_t;
	/* sock sk label is proxy for receiving task label,
	 * receiving so
	 *    source addr is remote
	 *    dest addr is local
	 */
	return label_skb_has_perm1(NULL, label, sk, skb,
				   OP_RCV_SKB, request, profile, ad,
				map_skb_addrs(skb, sk->sk_family,
					      ADDR_REMOTE, &sraddr,
					      ADDR_LOCAL, &sladdr, &ad),
				__profile_skb_perm(profile, sk, skb, request,
						 &sraddr.maddr, ifidx, seclabel,
						 &sladdr.maddr, &ad));
}


int __aa_inet_conn_request(struct aa_label *label, const struct sock *sk,
			   const struct sk_buff *skb,
			   const struct request_sock *req)
{
	struct stored_match_addr sladdr;
	struct stored_match_addr sraddr;
	struct aa_label *seclabel;
	int ifidx = skb->skb_iif;
	u16 family = req->rsk_ops->family;
	u32 request = AA_MAY_CONNECT;

	AA_BUG(!label);
	AA_BUG(!sk);
	AA_BUG(!skb);

	seclabel = aa_secid_to_label(skb->secmark); /* may be NULL */
	if (!seclabel)
		seclabel = unlabeled_t;

	/* sock sk label is proxy for receiving task label,
	 * receiving so
	 *    source addr is remote
	 *    dest addr is local
	 */
	return label_skb_has_perm1(NULL, label, sk, skb,
				   OP_CONN_REQ, request, profile, ad,
				map_skb_addrs(skb, family,
					      ADDR_REMOTE, &sraddr,
					      ADDR_LOCAL, &sladdr, &ad),
				__profile_skb_perm(profile, sk, skb, request,
						&sraddr.maddr, ifidx, seclabel,
						&sladdr.maddr, &ad));
}

int __aa_ip_postroute(struct aa_label *label, const struct sock *sk,
		      const struct sk_buff *skb,
		      const struct nf_hook_state *state)
{
	struct stored_match_addr sladdr;
	struct stored_match_addr sraddr;
	struct aa_label *seclabel;
	int ifidx = state->out->ifindex;

	u32 request = AA_MAY_SEND;

	AA_BUG(!label);
	AA_BUG(!sk);
	AA_BUG(!skb);

	seclabel = aa_secid_to_label(skb->secmark); /* may be NULL */
	if (!seclabel)
		seclabel = unlabeled_t;

	/* sock sk label is proxy for receiving task label,
	 * receiving so
	 *    source addr is remote
	 *    dest addr is local
	 */
	return label_skb_has_perm1(NULL, label, sk, skb,
				   OP_POSTROUTE, request, profile, ad,
				map_skb_addrs(skb, state->pf,
					      ADDR_LOCAL, &sladdr,
					      ADDR_REMOTE, &sraddr, &ad),
				__profile_skb_perm(profile, sk, skb, request,
						 &sraddr.maddr, ifidx, seclabel,
						 &sladdr.maddr, &ad));
}

/* returns refcounted label, ERR_PTR, or NULL if profile should be skipped */
static struct aa_label *profile_ip_localout(const struct cred *subj_cred,
					    struct aa_profile *profile,
					    const struct sock *sk,
					    const struct sk_buff *skb,
					    struct match_addr *raddr,
					    struct match_addr *laddr,
					    struct apparmor_audit_data *ad)
{
	struct aa_ruleset *rules = profile->label.rules[0];
	struct aa_perms perms = { };
	struct aa_perms *p = NULL;
	aa_state_t state;
	u32 request = AA_SET_LABEL;

	AA_BUG(!subj_cred);
	AA_BUG(!profile);
	AA_BUG(!sk);
	AA_BUG(!skb);
	AA_BUG(!raddr);
	AA_BUG(!laddr);
	AA_BUG(!ad);
	AA_BUG(skb->skb_iif);
	AA_BUG(sk->sk_family != PF_INET && sk->sk_family != PF_INET6,
	       "family=%d", sk->sk_family);

	state = RULE_MEDIATES_SKB(rules);
	if (!state)
		/* does not add to label or block */
		return NULL;

	state = skb_match_to_cmd(profile, rules, state, request, sk, skb,
				 CMD_ADDR, raddr, -1, NULL, &p, &perms, &ad->info);
	if (state && !p) {
		struct aa_sk_ctx *ctx = aa_sock(sk);

		/* check if perm is restricted to a pairing */
		state = match_addr_iface_label(profile, rules, state, request,
					       laddr, sk, -1,
					       rcu_dereference(ctx->label),
					       &p, &perms, &ad->info);
	}
	if (!p)
		p = aa_lookup_perms(rules->policy, state);
	if ((p->allow & request) == request) {
		if (p->label != 0) {
			AA_DEBUG_PROFILE(profile, DEBUG_SKB,
				"label specified but not currently supported");
			/* not supported yet */
			return ERR_PTR(-EACCES);
		}
		return aa_get_label(&profile->label);
	}
	return NULL;
}

/* called from within spinlocks */
struct aa_label *__aa_ip_localout(struct aa_label *label, const struct sock *sk,
				  const struct sk_buff *skb)
{
	struct stored_match_addr sladdr;
	struct stored_match_addr sraddr;
	struct aa_profile *profile;
	DEFINE_AUDIT_SKB(ad, OP_LOCALOUT, current_cred(), sk, skb);

	AA_BUG(!label);
	AA_BUG(!sk);
	AA_BUG(!skb);

	map_skb_addrs(skb, sk->sk_family, ADDR_LOCAL, &sladdr,
		      ADDR_REMOTE, &sraddr, &ad);
	ad.subj_cred = current_cred();

	/* TODO: caching mechanism - to reduce calls to build */

	return fn_label_build_in_netns_scope(label, profile, GFP_ATOMIC,
				profile_ip_localout(current_cred(), profile,
						    sk, skb, &sraddr.maddr,
						    &sladdr.maddr, &ad));
}
