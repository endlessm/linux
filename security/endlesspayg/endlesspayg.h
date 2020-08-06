// SPDX-License-Identifier: GPL-2.0
/*
 * LSM for PAYG security
 * Copyright (C) 2019 Endless Mobile, Inc.
 */

#ifndef _SECURITY_ENDLESSPAYG_H
#define _SECURITY_ENDLESSPAYG_H

bool eospayg_enforcing(void);
bool eospayg_skip_name(const char *name);
bool eospayg_skip_name_wide(const u16 *name);
bool eospayg_proc_pid_is_safe(pid_t tid);

#endif /* _SECURITY_ENDLESSPAYG_H */
