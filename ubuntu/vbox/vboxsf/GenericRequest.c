/* $Id: GenericRequest.cpp $ */
/** @file
 * VBoxGuestLibR0 - Generic VMMDev request management.
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

#include "VBGLInternal.h"
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/string.h>

DECLVBGL(int) VbglGRVerify (const VMMDevRequestHeader *pReq, size_t cbReq)
{
    size_t cbReqExpected;

    if (!pReq || cbReq < sizeof (VMMDevRequestHeader))
    {
        dprintf(("VbglGRVerify: Invalid parameter: pReq = %p, cbReq = %zu\n", pReq, cbReq));
        return VERR_INVALID_PARAMETER;
    }

    if (pReq->size > cbReq)
    {
        dprintf(("VbglGRVerify: request size %u > buffer size %zu\n", pReq->size, cbReq));
        return VERR_INVALID_PARAMETER;
    }

    /* The request size must correspond to the request type. */
    cbReqExpected = vmmdevGetRequestSize(pReq->requestType);

    if (cbReq < cbReqExpected)
    {
        dprintf(("VbglGRVerify: buffer size %zu < expected size %zu\n", cbReq, cbReqExpected));
        return VERR_INVALID_PARAMETER;
    }

    if (cbReqExpected == cbReq)
    {
        /* This is most likely a fixed size request, and in this case the request size
         * must be also equal to the expected size.
         */
        if (pReq->size != cbReqExpected)
        {
            dprintf(("VbglGRVerify: request size %u != expected size %zu\n", pReq->size, cbReqExpected));
            return VERR_INVALID_PARAMETER;
        }

        return VINF_SUCCESS;
    }

    /*
     * This can be a variable size request. Check the request type and limit the size
     * to VMMDEV_MAX_VMMDEVREQ_SIZE, which is max size supported by the host.
     *
     * Note: Keep this list sorted for easier human lookup!
     */
    if (   pReq->requestType == VMMDevReq_ChangeMemBalloon
#ifdef VBOX_WITH_64_BITS_GUESTS
        || pReq->requestType == VMMDevReq_HGCMCall32
        || pReq->requestType == VMMDevReq_HGCMCall64
#else
        || pReq->requestType == VMMDevReq_HGCMCall
#endif /* VBOX_WITH_64_BITS_GUESTS */
        || pReq->requestType == VMMDevReq_RegisterSharedModule
        || pReq->requestType == VMMDevReq_ReportGuestUserState
        || pReq->requestType == VMMDevReq_LogString
        || pReq->requestType == VMMDevReq_SetPointerShape
        || pReq->requestType == VMMDevReq_VideoSetVisibleRegion)
    {
        if (cbReq > VMMDEV_MAX_VMMDEVREQ_SIZE)
        {
            dprintf(("VbglGRVerify: VMMDevReq_LogString: buffer size %zu too big\n", cbReq));
            return VERR_BUFFER_OVERFLOW; /* @todo is this error code ok? */
        }
    }
    else
    {
        dprintf(("VbglGRVerify: request size %u > buffer size %zu\n", pReq->size, cbReq));
        return VERR_IO_BAD_LENGTH; /* @todo is this error code ok? */
    }

    return VINF_SUCCESS;
}

DECLVBGL(int) VbglGRAlloc (VMMDevRequestHeader **ppReq, uint32_t cbSize, VMMDevRequestType reqType)
{
    VMMDevRequestHeader *pReq;
    int rc = vbglR0Enter ();

    if (RT_FAILURE(rc))
        return rc;

    if (!ppReq || cbSize < sizeof (VMMDevRequestHeader))
    {
        dprintf(("VbglGRAlloc: Invalid parameter: ppReq = %p, cbSize = %u\n", ppReq, cbSize));
        return VERR_INVALID_PARAMETER;
    }

    pReq = (VMMDevRequestHeader *)VbglPhysHeapAlloc (cbSize);
    if (!pReq)
    {
        AssertMsgFailed(("VbglGRAlloc: no memory\n"));
        rc = VERR_NO_MEMORY;
    }
    else
    {
        memset(pReq, 0xAA, cbSize);

        pReq->size        = cbSize;
        pReq->version     = VMMDEV_REQUEST_HEADER_VERSION;
        pReq->requestType = reqType;
        pReq->rc          = VERR_GENERAL_FAILURE;
        pReq->reserved1   = 0;
        pReq->reserved2   = 0;

        *ppReq = pReq;
    }

    return rc;
}

DECLVBGL(int) VbglGRPerform (VMMDevRequestHeader *pReq)
{
    RTCCPHYS physaddr;
    int rc = vbglR0Enter ();

    if (RT_FAILURE(rc))
        return rc;

    if (!pReq)
        return VERR_INVALID_PARAMETER;

    physaddr = VbglPhysHeapGetPhysAddr (pReq);
    if (  !physaddr
       || (physaddr >> 32) != 0) /* Port IO is 32 bit. */
    {
        rc = VERR_VBGL_INVALID_ADDR;
    }
    else
    {
        ASMOutU32(g_vbgldata.portVMMDev + VMMDEV_PORT_OFF_REQUEST, (uint32_t)physaddr);
        /* Make the compiler aware that the host has changed memory. */
        ASMCompilerBarrier();
        rc = pReq->rc;
    }
    return rc;
}

DECLVBGL(void) VbglGRFree (VMMDevRequestHeader *pReq)
{
    int rc = vbglR0Enter ();

    if (RT_FAILURE(rc))
        return;

    VbglPhysHeapFree (pReq);
}

