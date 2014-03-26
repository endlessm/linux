/*
 * AppArmor security module
 *
 * This file contains AppArmor basic permission sets definitions.
 *
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#ifndef __AA_PERM_H
#define __AA_PERM_H

#include <linux/fs.h>
#include "label.h"

#define AA_MAY_EXEC		MAY_EXEC
#define AA_MAY_WRITE		MAY_WRITE
#define AA_MAY_READ		MAY_READ
#define AA_MAY_APPEND		MAY_APPEND

#define AA_MAY_CREATE		0x0010
#define AA_MAY_DELETE		0x0020
#define AA_MAY_OPEN		0x0040
#define AA_MAY_RENAME		0x0080		/* pair */

#define AA_MAY_META_WRITE	0x0100
#define AA_MAY_META_READ	0x0200
#define AA_MAY_GET_SECURITY	0x0400
#define AA_MAY_SET_SECURITY	0x0800

#define AA_MAY_CHMOD		0x1000		/* pair */
#define AA_MAY_CHOWN		0x2000		/* pair */
#define AA_MAY_CHGRP		0x4000		/* pair */
#define AA_MAY_LOCK		0x8000		/* LINK_SUBSET overlaid */

#define AA_EXEC_MMAP		0x00010000
#define AA_MAY_MPROT_WX		0x00020000
#define AA_MAY_MPROT_XW		0x00040000
#define AA_MAY_LINK		0x00080000	/* pair */

#define AA_MAY_SNAPSHOT		0x00100000	/* pair */
#define AA_MAY_BIND		0x00200000
#define AA_MAY_ACCEPT		0x00400000
#define AA_MAY_LISTEN		0x00800000

#define AA_MAY_DELEGATE
#define AA_CONT_MATCH		0x08000000

#define AA_MAY_STACK		0x10000000
#define AA_MAY_ONEXEC		0x20000000 /* either stack or change_profile */
#define AA_MAY_CHANGE_PROFILE	0x40000000
#define AA_MAY_CHANGEHAT	0x80000000

#define AA_LINK_SUBSET		AA_MAY_LOCK	/* overlaid */

#define AA_MAY_CONNECT		AA_MAY_OPEN
#define AA_MAY_SEND		AA_MAY_WRITE
#define AA_MAY_RECEIVE		AA_MAY_READ
#define AA_MAY_XATTR_READ	AA_MAY_READ	/* stored on pair like link */
#define AA_MAY_XATTR_WRITE	AA_MAY_WRITE	/* stored on pair like link */


#define AA_PERM_CHR_MASK (MAY_READ | MAY_WRITE | AA_MAY_CREATE |	\
			  AA_MAY_DELETE | AA_MAY_LINK | AA_MAY_LOCK |	\
			  AA_MAY_EXEC | AA_EXEC_MMAP)


struct aa_perms {
	u32 allow;
	u32 audit;	/* set only when allow is set */

	u32 deny;	/* explicit deny, or conflict if allow also set */
	u32 quiet;	/* set only when ~allow | deny */
	u32 kill;	/* set only when ~allow | deny */
	u32 stop;	/* set only when ~allow | deny */

	u32 complain;	/* accumulates only used when ~allow & ~deny */
	u32 cond;	/* set only when ~allow and ~deny */

	u32 hide;	/* set only when  ~allow | deny */

	/* Reserved:
	 * u32 subtree;	/ * set only when allow is set * /
	 * u32 prompt;	/ * accumulates only used when ~allow & ~deny * /
	 */
};

#define ALL_PERMS_MASK 0xffffffff


#define xcheck(FN1, FN2)	\
({				\
	int e, error = FN1;	\
	e = FN2;		\
	if (e)			\
		error = e;	\
	error;			\
})

/* pattern: perform send/receive style paired cross check of permissions
 *
 */
#define xcheck_profiles(P1, P2, FN1, FN2, PERM, args...)	\
({								\
	xcheck(FN1(P1, P2, PERM, args),				\
	       FN2(P2, P1, PERM, args));			\
})


/* TODO: update for labels pointing to labels instead of profiles
*  Note: this only works for profiles from a single namespace
*/

#define xcheck_profile_label(P, L, FN, args...)			\
({								\
	struct aa_profile *__p2;				\
	fn_for_each((L), __p2, FN((P), __p2, args));		\
})

#define xcheck_ns_labels(L1, L2, FN, args...)			\
({								\
	struct aa_profile *__p1;				\
	fn_for_each((L1), __p1, FN(__p1, (L2), args));		\
})

/* todo: fix to handle multiple namespaces */
#define xcheck_labels(L1, L2, FN, args...)			\
	xcheck_ns_labels((L1), (L2), FN, args)

/* Do the cross check but applying FN at the profiles level */
#define xcheck_labels_profiles(L1, L2, FN, args...)		\
	xcheck_ns_labels((L1), (L2), xcheck_profile_label, (FN), args)


#define FINAL_CHECK true

void aa_perm_mask_to_chr(u32 mask, char *str);
void aa_audit_perm_mask(struct audit_buffer *ab, u32 mask);
void aa_apply_modes_to_perms(struct aa_profile *profile,
			     struct aa_perms *perms);
void aa_compute_perms(struct aa_dfa *dfa, unsigned int state,
		      struct aa_perms *perms);
void aa_profile_match_label(struct aa_profile *profile, const char *label,
			    int type, struct aa_perms *perms);
int aa_profile_label_perm(struct aa_profile *profile, struct aa_profile *target,
			  u32 request, int type, u32 *deny,
			  struct common_audit_data *sa);
int aa_check_perms(struct aa_profile *profile, struct aa_perms *perms,
		   u32 request, struct common_audit_data *sa,
		   void (*cb) (struct audit_buffer *, void *));
const char *aa_peer_name(struct aa_profile *peer);


static inline int aa_cross_label_perm(struct aa_profile *profile,
				      struct aa_profile *target,
				      int type, u32 request, u32 reverse,
				      u32 * deny, struct common_audit_data *sa)
{
  /* TODO: ??? 2nd aa_profile_label_perm needs to reverse perms */
	return xcheck_profiles(profile, target, aa_profile_label_perm,
			       aa_profile_label_perm, request, type, deny,
			       sa);
}


#endif /* __AA_PERM_H */
