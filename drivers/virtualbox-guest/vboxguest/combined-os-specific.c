/* $Id: combined-os-specific.c $ */
/** @file
 * VBoxGuest - Combine a bunch of OS specific sources into one compile unit.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


#include "the-linux-kernel.h"

#include "r0drv/linux/alloc-r0drv-linux.c"
#include "r0drv/linux/assert-r0drv-linux.c"
#include "r0drv/linux/initterm-r0drv-linux.c"
#include "r0drv/linux/memobj-r0drv-linux.c"
#include "r0drv/linux/memuserkernel-r0drv-linux.c"
#include "r0drv/linux/mp-r0drv-linux.c"
#include "r0drv/linux/mpnotification-r0drv-linux.c"
#include "r0drv/linux/process-r0drv-linux.c"
#include "r0drv/linux/semevent-r0drv-linux.c"
#include "r0drv/linux/semeventmulti-r0drv-linux.c"
#include "r0drv/linux/semfastmutex-r0drv-linux.c"
#include "r0drv/linux/semmutex-r0drv-linux.c"
#include "r0drv/linux/spinlock-r0drv-linux.c"
#include "r0drv/linux/thread-r0drv-linux.c"
#include "r0drv/linux/thread2-r0drv-linux.c"
#undef LOG_GROUP
#include "r0drv/linux/time-r0drv-linux.c"
#include "r0drv/linux/timer-r0drv-linux.c"
#include "r0drv/linux/RTLogWriteDebugger-r0drv-linux.c"
#include "common/err/RTErrConvertFromErrno.c"
#include "common/err/RTErrConvertToErrno.c"

