/* $Id: HGCM.cpp $ */
/** @file
 * VBoxGuestLib - Host-Guest Communication Manager.
 *
 * These public functions can be only used by other drivers. They all
 * do an IOCTL to VBoxGuest via IDC.
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

/* Entire file is ifdef'ed with !VBGL_VBOXGUEST */
#ifndef VBGL_VBOXGUEST

#include "VBGLInternal.h"

#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>

#define VBGL_HGCM_ASSERTMsg AssertReleaseMsg

/**
 * Initializes the HGCM VBGL bits.
 *
 * @return VBox status code.
 */
int vbglR0HGCMInit (void)
{
    return RTSemFastMutexCreate(&g_vbgldata.mutexHGCMHandle);
}

/**
 * Initializes the HGCM VBGL bits.
 *
 * @return VBox status code.
 */
int vbglR0HGCMTerminate (void)
{
    RTSemFastMutexDestroy(g_vbgldata.mutexHGCMHandle);
    g_vbgldata.mutexHGCMHandle = NIL_RTSEMFASTMUTEX;

    return VINF_SUCCESS;
}

DECLINLINE(int) vbglHandleHeapEnter (void)
{
    int rc = RTSemFastMutexRequest(g_vbgldata.mutexHGCMHandle);

    VBGL_HGCM_ASSERTMsg(RT_SUCCESS(rc),
                        ("Failed to request handle heap mutex, rc = %Rrc\n", rc));

    return rc;
}

DECLINLINE(void) vbglHandleHeapLeave (void)
{
    RTSemFastMutexRelease(g_vbgldata.mutexHGCMHandle);
}

struct VBGLHGCMHANDLEDATA *vbglHGCMHandleAlloc (void)
{
    struct VBGLHGCMHANDLEDATA *p;
    int rc = vbglHandleHeapEnter ();
    uint32_t i;

    if (RT_FAILURE (rc))
        return NULL;

    p = NULL;

    /** Simple linear search in array. This will be called not so often, only connect/disconnect.
     * @todo bitmap for faster search and other obvious optimizations.
     */

    for (i = 0; i < RT_ELEMENTS(g_vbgldata.aHGCMHandleData); i++)
    {
        if (!g_vbgldata.aHGCMHandleData[i].fAllocated)
        {
            p = &g_vbgldata.aHGCMHandleData[i];
            p->fAllocated = 1;
            break;
        }
    }

    vbglHandleHeapLeave ();

    VBGL_HGCM_ASSERTMsg(p != NULL,
                        ("Not enough HGCM handles.\n"));

    return p;
}

void vbglHGCMHandleFree (struct VBGLHGCMHANDLEDATA *pHandle)
{
    int rc;

    if (!pHandle)
       return;

    rc = vbglHandleHeapEnter ();

    if (RT_FAILURE (rc))
        return;

    VBGL_HGCM_ASSERTMsg(pHandle->fAllocated,
                        ("Freeing not allocated handle.\n"));

    memset(pHandle, 0, sizeof (struct VBGLHGCMHANDLEDATA));
    vbglHandleHeapLeave ();
    return;
}

DECLVBGL(int) VbglHGCMConnect (VBGLHGCMHANDLE *pHandle, VBoxGuestHGCMConnectInfo *pData)
{
    int rc;
    struct VBGLHGCMHANDLEDATA *pHandleData;

    if (!pHandle || !pData)
        return VERR_INVALID_PARAMETER;

    pHandleData = vbglHGCMHandleAlloc();
    if (!pHandleData)
        rc = VERR_NO_MEMORY;
    else
    {
        rc = vbglDriverOpen (&pHandleData->driver);
        if (RT_SUCCESS(rc))
        {
            rc = vbglDriverIOCtl (&pHandleData->driver, VBOXGUEST_IOCTL_HGCM_CONNECT, pData, sizeof (*pData));
            if (RT_SUCCESS(rc))
                rc = pData->result;
            if (RT_SUCCESS(rc))
            {
                *pHandle = pHandleData;
                return rc;
            }

            vbglDriverClose (&pHandleData->driver);
        }

        vbglHGCMHandleFree (pHandleData);
    }

    return rc;
}

DECLVBGL(int) VbglHGCMDisconnect (VBGLHGCMHANDLE handle, VBoxGuestHGCMDisconnectInfo *pData)
{
    int rc = VINF_SUCCESS;

    rc = vbglDriverIOCtl (&handle->driver, VBOXGUEST_IOCTL_HGCM_DISCONNECT, pData, sizeof (*pData));

    vbglDriverClose (&handle->driver);

    vbglHGCMHandleFree (handle);

    return rc;
}

DECLVBGL(int) VbglHGCMCall (VBGLHGCMHANDLE handle, VBoxGuestHGCMCallInfo *pData, uint32_t cbData)
{
    int rc = VINF_SUCCESS;

    VBGL_HGCM_ASSERTMsg(cbData >= sizeof (VBoxGuestHGCMCallInfo) + pData->cParms * sizeof (HGCMFunctionParameter),
                        ("cbData = %d, cParms = %d (calculated size %d)\n", cbData, pData->cParms, sizeof (VBoxGuestHGCMCallInfo) + pData->cParms * sizeof (VBoxGuestHGCMCallInfo)));

    rc = vbglDriverIOCtl (&handle->driver, VBOXGUEST_IOCTL_HGCM_CALL(cbData), pData, cbData);

    return rc;
}

DECLVBGL(int) VbglHGCMCallUserData (VBGLHGCMHANDLE handle, VBoxGuestHGCMCallInfo *pData, uint32_t cbData)
{
    int rc = VINF_SUCCESS;

    VBGL_HGCM_ASSERTMsg(cbData >= sizeof (VBoxGuestHGCMCallInfo) + pData->cParms * sizeof (HGCMFunctionParameter),
                        ("cbData = %d, cParms = %d (calculated size %d)\n", cbData, pData->cParms, sizeof (VBoxGuestHGCMCallInfo) + pData->cParms * sizeof (VBoxGuestHGCMCallInfo)));

    rc = vbglDriverIOCtl (&handle->driver, VBOXGUEST_IOCTL_HGCM_CALL_USERDATA(cbData), pData, cbData);

    return rc;
}


DECLVBGL(int) VbglHGCMCallTimed (VBGLHGCMHANDLE handle,
                                 VBoxGuestHGCMCallInfoTimed *pData, uint32_t cbData)
{
    int rc = VINF_SUCCESS;

    uint32_t cbExpected =   sizeof (VBoxGuestHGCMCallInfoTimed)
                          + pData->info.cParms * sizeof (HGCMFunctionParameter);
    VBGL_HGCM_ASSERTMsg(cbData >= cbExpected,
                        ("cbData = %d, cParms = %d (calculated size %d)\n",
                        cbData, pData->info.cParms, cbExpected));

    rc = vbglDriverIOCtl (&handle->driver, VBOXGUEST_IOCTL_HGCM_CALL_TIMED(cbData),
                          pData, cbData);

    return rc;
}

#endif /* !VBGL_VBOXGUEST */

