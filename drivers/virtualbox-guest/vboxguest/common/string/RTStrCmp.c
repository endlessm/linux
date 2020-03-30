/* $Id: RTStrCmp.cpp $ */
/** @file
 * IPRT - RTStrCmp.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/string.h>
#include "internal/iprt.h"


/**
 * Performs a case sensitive string compare between two UTF-8 strings.
 *
 * Encoding errors are ignored by the current implementation. So, the only
 * difference between this and the CRT strcmp function is the handling of
 * NULL arguments.
 *
 * @returns < 0 if the first string less than the second string.
 * @returns 0 if the first string identical to the second string.
 * @returns > 0 if the first string greater than the second string.
 * @param   psz1        First UTF-8 string. Null is allowed.
 * @param   psz2        Second UTF-8 string. Null is allowed.
 */
RTDECL(int) RTStrCmp(const char *psz1, const char *psz2)
{
    if (psz1 == psz2)
        return 0;
    if (!psz1)
        return -1;
    if (!psz2)
        return 1;

    return strcmp(psz1, psz2);
}
RT_EXPORT_SYMBOL(RTStrCmp);

