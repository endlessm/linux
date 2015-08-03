/* $Id: strprintf.cpp $ */
/** @file
 * IPRT - String Formatters.
 */

/*
 * Copyright (C) 2006-2015 Oracle Corporation
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/string.h>
#include "internal/iprt.h"

#include <iprt/assert.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** strbufoutput() argument structure. */
typedef struct STRBUFARG
{
    /** Pointer to current buffer position. */
    char   *psz;
    /** Number of bytes left in the buffer - not including the trailing zero. */
    size_t  cch;
} STRBUFARG;
/** Pointer to a strbufoutput() argument structure. */
typedef STRBUFARG *PSTRBUFARG;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static DECLCALLBACK(size_t) strbufoutput(void *pvArg, const char *pachChars, size_t cbChars);


/**
 * Output callback.
 *
 * @returns number of bytes written.
 * @param   pvArg       Pointer to a STRBUFARG structure.
 * @param   pachChars   Pointer to an array of utf-8 characters.
 * @param   cbChars     Number of bytes in the character array pointed to by pachChars.
 */
static DECLCALLBACK(size_t) strbufoutput(void *pvArg, const char *pachChars, size_t cbChars)
{
    PSTRBUFARG  pArg = (PSTRBUFARG)pvArg;

    cbChars = RT_MIN(pArg->cch, cbChars);
    if (cbChars)
    {
        memcpy(pArg->psz, pachChars, cbChars);
        pArg->cch -= cbChars;
        pArg->psz += cbChars;
    }
    *pArg->psz = '\0';

    return cbChars;
}


RTDECL(size_t) RTStrPrintfExV(PFNSTRFORMAT pfnFormat, void *pvArg, char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args)
{
    STRBUFARG Arg;

    if (!cchBuffer)
    {
        AssertMsgFailed(("Excellent idea! Format a string with no space for the output!\n"));
        return 0;
    }

    Arg.psz = pszBuffer;
    Arg.cch = cchBuffer - 1;
    return RTStrFormatV(strbufoutput, &Arg, pfnFormat, pvArg, pszFormat, args);
}
RT_EXPORT_SYMBOL(RTStrPrintfExV);


RTDECL(size_t) RTStrPrintfV(char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args)
{
    return RTStrPrintfExV(NULL, NULL, pszBuffer, cchBuffer, pszFormat, args);
}
RT_EXPORT_SYMBOL(RTStrPrintfV);


RTDECL(size_t) RTStrPrintfEx(PFNSTRFORMAT pfnFormat, void *pvArg, char *pszBuffer, size_t cchBuffer, const char *pszFormat, ...)
{
    va_list args;
    size_t cbRet;
    va_start(args, pszFormat);
    cbRet = RTStrPrintfExV(pfnFormat, pvArg, pszBuffer, cchBuffer, pszFormat, args);
    va_end(args);
    return cbRet;
}
RT_EXPORT_SYMBOL(RTStrPrintfEx);


RTDECL(size_t) RTStrPrintf(char *pszBuffer, size_t cchBuffer, const char *pszFormat, ...)
{
    va_list args;
    size_t cbRet;
    va_start(args, pszFormat);
    cbRet = RTStrPrintfV(pszBuffer, cchBuffer, pszFormat, args);
    va_end(args);
    return cbRet;
}
RT_EXPORT_SYMBOL(RTStrPrintf);

