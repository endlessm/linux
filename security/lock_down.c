/* Lock down the kernel
 *
 * Copyright (C) 2016 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/security.h>
#include <linux/export.h>

static __read_mostly bool kernel_locked_down;

/*
 * Put the kernel into lock-down mode.
 */
void lock_kernel_down(void)
{
	kernel_locked_down = true;
}

/*
 * Take the kernel out of lockdown mode.
 */
void lift_kernel_lockdown(void)
{
	kernel_locked_down = false;
}

/**
 * kernel_is_locked_down - Find out if the kernel is locked down
 */
bool kernel_is_locked_down(void)
{
	return kernel_locked_down;
}
EXPORT_SYMBOL(kernel_is_locked_down);
