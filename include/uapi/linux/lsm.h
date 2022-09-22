/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Linus Security Modules (LSM) - User space API
 *
 * Copyright (C) 2022 Casey Schaufler <casey@schaufler-ca.com>
 * Copyright (C) Intel Corporation
 */

#ifndef _UAPI_LINUX_LSM_H
#define _UAPI_LINUX_LSM_H

/*
 * ID values to identify security modules.
 * A system may use more than one security module.
 *
 * LSM_ID_XXX values 32 and below are reserved for future use
 */
#define LSM_ID_INVALID		-1
#define LSM_ID_SELINUX		33
#define LSM_ID_SMACK		34
#define LSM_ID_TOMOYO		35
#define LSM_ID_IMA		36
#define LSM_ID_APPARMOR		37
#define LSM_ID_YAMA		38
#define LSM_ID_LOADPIN		39
#define LSM_ID_SAFESETID	40
#define LSM_ID_LOCKDOWN		41
#define LSM_ID_BPF		42
#define LSM_ID_LANDLOCK		43
#define LSM_ID_CAPABILITY	44

#endif /* _UAPI_LINUX_LSM_H */
