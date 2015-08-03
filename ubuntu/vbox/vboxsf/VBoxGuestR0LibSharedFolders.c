/* $Id: VBoxGuestR0LibSharedFolders.c $ */
/** @file
 * VBoxGuestR0LibSharedFolders - Ring 0 Shared Folders calls.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_FOLDERS
#ifdef RT_OS_LINUX
# include "VBoxGuestR0LibSharedFolders.h"
#else
# include "VBoxGuestR0LibSharedFolders.h"
#endif
#include <VBox/log.h>
#include <iprt/time.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/string.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define SHFL_CPARMS_SET_UTF8 0
#define SHFL_CPARMS_SET_SYMLINKS 0

#define VBOX_INIT_CALL(a, b, c)          \
    LogFunc(("%s, u32ClientID=%d\n", "SHFL_FN_" # b, \
            (c)->ulClientID)); \
    (a)->result      = VINF_SUCCESS;     \
    (a)->u32ClientID = (c)->ulClientID;  \
    (a)->u32Function = SHFL_FN_##b;      \
    (a)->cParms      = SHFL_CPARMS_##b



DECLVBGL(int) vboxInit (void)
{
    int rc = VINF_SUCCESS;

    rc = VbglInit ();
    return rc;
}

DECLVBGL(void) vboxUninit (void)
{
    VbglTerminate ();
}

DECLVBGL(int) vboxConnect (PVBSFCLIENT pClient)
{
    int rc;
    VBoxGuestHGCMConnectInfo data;

    pClient->handle = NULL;

    RT_ZERO(data);
    data.result   = VINF_SUCCESS;
    data.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    strcpy (data.Loc.u.host.achName, "VBoxSharedFolders");

    rc = VbglHGCMConnect (&pClient->handle, &data);

/*    Log(("VBOXSF: VBoxSF::vboxConnect: VbglHGCMConnect rc = %#x, result = %#x\n",
         rc, data.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.result;
    }

    if (RT_SUCCESS (rc))
    {
        pClient->ulClientID = data.u32ClientID;
        LogFunc(("u32ClientID=%d\n", pClient->ulClientID));
    }
    return rc;
}

DECLVBGL(void) vboxDisconnect (PVBSFCLIENT pClient)
{
    int rc;
    VBoxGuestHGCMDisconnectInfo data;

    LogFunc(("u32ClientID=%d\n", pClient->ulClientID));
    if (pClient->handle == NULL)
        return;                 /* not connected */

    RT_ZERO(data);
    data.result      = VINF_SUCCESS;
    data.u32ClientID = pClient->ulClientID;

    rc = VbglHGCMDisconnect (pClient->handle, &data);
    NOREF(rc);
/*    Log(("VBOXSF: VBoxSF::vboxDisconnect: "
         "VbglHGCMDisconnect rc = %#x, result = %#x\n", rc, data.result));
*/
    return;
}

DECLVBGL(int) vboxCallQueryMappings (PVBSFCLIENT pClient, SHFLMAPPING paMappings[],
                                     uint32_t *pcMappings)
{
    int rc = VINF_SUCCESS;

    VBoxSFQueryMappings data;

    VBOX_INIT_CALL(&data.callInfo, QUERY_MAPPINGS, pClient);

    data.flags.type                      = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                 = SHFL_MF_UCS2;

    data.numberOfMappings.type           = VMMDevHGCMParmType_32bit;
    data.numberOfMappings.u.value32      = *pcMappings;

    data.mappings.type                   = VMMDevHGCMParmType_LinAddr;
    data.mappings.u.Pointer.size         = sizeof (SHFLMAPPING) * *pcMappings;
    data.mappings.u.Pointer.u.linearAddr = (uintptr_t)&paMappings[0];

/*    Log(("VBOXSF: in ifs difference %d\n",
         (char *)&data.flags.type - (char *)&data.callInfo.cParms));
*/
    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*
    Log(("VBOXSF: VBoxSF::vboxCallQueryMappings: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }

    if (RT_SUCCESS (rc))
    {
        *pcMappings = data.numberOfMappings.u.value32;
    }

    return rc;
}

DECLVBGL(int) vboxCallQueryMapName (PVBSFCLIENT pClient, SHFLROOT root, SHFLSTRING *pString, uint32_t size)
{
    int rc = VINF_SUCCESS;

    VBoxSFQueryMapName data;

    VBOX_INIT_CALL(&data.callInfo, QUERY_MAP_NAME, pClient);

    data.root.type                   = VMMDevHGCMParmType_32bit;
    data.root.u.value32              = root;

    data.name.type                   = VMMDevHGCMParmType_LinAddr;
    data.name.u.Pointer.size         = size;
    data.name.u.Pointer.u.linearAddr = (uintptr_t)pString;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallQueryMapName: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }

    return rc;
}

DECLVBGL(int) vboxCallMapFolder(PVBSFCLIENT pClient, PSHFLSTRING szFolderName,
                                PVBSFMAP pMap)
{
    int rc = VINF_SUCCESS;

    VBoxSFMapFolder data;

    VBOX_INIT_CALL(&data.callInfo, MAP_FOLDER, pClient);

    data.path.type                    = VMMDevHGCMParmType_LinAddr;
    data.path.u.Pointer.size          = ShflStringSizeOfBuffer (szFolderName);
    data.path.u.Pointer.u.linearAddr  = (uintptr_t)szFolderName;

    data.root.type                    = VMMDevHGCMParmType_32bit;
    data.root.u.value32               = 0;

    data.delimiter.type               = VMMDevHGCMParmType_32bit;
    data.delimiter.u.value32          = RTPATH_DELIMITER;

    data.fCaseSensitive.type          = VMMDevHGCMParmType_32bit;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    data.fCaseSensitive.u.value32     = 0;
#else
    data.fCaseSensitive.u.value32     = 1;
#endif

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallMapFolder: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        pMap->root = data.root.u.value32;
        rc         = data.callInfo.result;
    }
    else if (rc == VERR_NOT_IMPLEMENTED)
    {
        /* try the legacy interface too; temporary to assure backwards compatibility */
        VBoxSFMapFolder_Old OldData;

        VBOX_INIT_CALL(&OldData.callInfo, MAP_FOLDER_OLD, pClient);

        OldData.path.type                    = VMMDevHGCMParmType_LinAddr;
        OldData.path.u.Pointer.size          = ShflStringSizeOfBuffer (szFolderName);
        OldData.path.u.Pointer.u.linearAddr  = (uintptr_t)szFolderName;

        OldData.root.type                    = VMMDevHGCMParmType_32bit;
        OldData.root.u.value32               = 0;

        OldData.delimiter.type               = VMMDevHGCMParmType_32bit;
        OldData.delimiter.u.value32          = RTPATH_DELIMITER;

        rc = VbglHGCMCall (pClient->handle, &OldData.callInfo, sizeof (OldData));

        if (RT_SUCCESS (rc))
        {
            pMap->root = OldData.root.u.value32;
            rc         = OldData.callInfo.result;
        }
    }
    return rc;
}

DECLVBGL(int) vboxCallUnmapFolder(PVBSFCLIENT pClient, PVBSFMAP pMap)
{
    int rc = VINF_SUCCESS;

    VBoxSFUnmapFolder data;

    VBOX_INIT_CALL(&data.callInfo, UNMAP_FOLDER, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallUnmapFolder: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc         = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallCreate (PVBSFCLIENT pClient, PVBSFMAP pMap,
                              PSHFLSTRING pParsedPath, PSHFLCREATEPARMS pCreateParms)
{
    /** @todo copy buffers to physical or mapped memory. */
    int rc = VINF_SUCCESS;

    VBoxSFCreate data;

    VBOX_INIT_CALL(&data.callInfo, CREATE, pClient);

    data.root.type                    = VMMDevHGCMParmType_32bit;
    data.root.u.value32               = pMap->root;

    data.path.type                    = VMMDevHGCMParmType_LinAddr;
    data.path.u.Pointer.size          = ShflStringSizeOfBuffer (pParsedPath);
    data.path.u.Pointer.u.linearAddr  = (uintptr_t)pParsedPath;

    data.parms.type                   = VMMDevHGCMParmType_LinAddr;
    data.parms.u.Pointer.size         = sizeof (SHFLCREATEPARMS);
    data.parms.u.Pointer.u.linearAddr = (uintptr_t)pCreateParms;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallCreate: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallClose (PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE Handle)
{
    int rc = VINF_SUCCESS;

    VBoxSFClose data;

    VBOX_INIT_CALL(&data.callInfo, CLOSE, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = Handle;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallClose: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }

    return rc;
}

DECLVBGL(int) vboxCallRemove (PVBSFCLIENT pClient, PVBSFMAP pMap,
                              PSHFLSTRING pParsedPath, uint32_t flags)
{
    int rc = VINF_SUCCESS;

    VBoxSFRemove data;

    VBOX_INIT_CALL(&data.callInfo, REMOVE, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.path.type                      = VMMDevHGCMParmType_LinAddr_In;
    data.path.u.Pointer.size            = ShflStringSizeOfBuffer (pParsedPath);
    data.path.u.Pointer.u.linearAddr    = (uintptr_t)pParsedPath;

    data.flags.type                     = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                = flags;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallRemove: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }

    return rc;
}

DECLVBGL(int) vboxCallRename (PVBSFCLIENT pClient, PVBSFMAP pMap,
                              PSHFLSTRING pSrcPath, PSHFLSTRING pDestPath, uint32_t flags)
{
    int rc = VINF_SUCCESS;

    VBoxSFRename data;

    VBOX_INIT_CALL(&data.callInfo, RENAME, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.src.type                       = VMMDevHGCMParmType_LinAddr_In;
    data.src.u.Pointer.size             = ShflStringSizeOfBuffer (pSrcPath);
    data.src.u.Pointer.u.linearAddr     = (uintptr_t)pSrcPath;

    data.dest.type                      = VMMDevHGCMParmType_LinAddr_In;
    data.dest.u.Pointer.size            = ShflStringSizeOfBuffer (pDestPath);
    data.dest.u.Pointer.u.linearAddr    = (uintptr_t)pDestPath;

    data.flags.type                     = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                = flags;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallRename: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallRead(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                           uint64_t offset, uint32_t *pcbBuffer, uint8_t *pBuffer, bool fLocked)
{
    int rc = VINF_SUCCESS;

    VBoxSFRead data;

    VBOX_INIT_CALL(&data.callInfo, READ, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;
    data.offset.type                    = VMMDevHGCMParmType_64bit;
    data.offset.u.value64               = offset;
    data.cb.type                        = VMMDevHGCMParmType_32bit;
    data.cb.u.value32                   = *pcbBuffer;
    data.buffer.type                    = (fLocked) ? VMMDevHGCMParmType_LinAddr_Locked_Out : VMMDevHGCMParmType_LinAddr_Out;
    data.buffer.u.Pointer.size          = *pcbBuffer;
    data.buffer.u.Pointer.u.linearAddr  = (uintptr_t)pBuffer;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallRead: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
        *pcbBuffer = data.cb.u.value32;
    }
    return rc;
}

DECLVBGL(int) VbglR0SharedFolderReadPageList(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                                             uint64_t offset, uint32_t *pcbBuffer,
                                             uint16_t offFirstPage, uint16_t cPages, RTGCPHYS64 *paPages)
{
    uint32_t            cbToRead  = *pcbBuffer;
    uint32_t            cbData    = (uint32_t)(sizeof(VBoxSFRead) + RT_UOFFSETOF(HGCMPageListInfo, aPages[cPages]));
    VBoxSFRead         *pData     = (VBoxSFRead *)RTMemTmpAlloc(cbData);
    HGCMPageListInfo   *pPgLst    = (HGCMPageListInfo *)(pData + 1);
    uint16_t            iPage;
    int                 rc;

    if (RT_UNLIKELY(!pData))
        return VERR_NO_TMP_MEMORY;

    VBOX_INIT_CALL(&pData->callInfo, READ, pClient);

    pData->root.type                      = VMMDevHGCMParmType_32bit;
    pData->root.u.value32                 = pMap->root;

    pData->handle.type                    = VMMDevHGCMParmType_64bit;
    pData->handle.u.value64               = hFile;
    pData->offset.type                    = VMMDevHGCMParmType_64bit;
    pData->offset.u.value64               = offset;
    pData->cb.type                        = VMMDevHGCMParmType_32bit;
    pData->cb.u.value32                   = cbToRead;
    pData->buffer.type                    = VMMDevHGCMParmType_PageList;
    pData->buffer.u.PageList.size         = cbToRead;
    pData->buffer.u.PageList.offset       = sizeof(VBoxSFRead);

    pPgLst->flags = VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    pPgLst->offFirstPage = offFirstPage;
    pPgLst->cPages = cPages;
    for (iPage = 0; iPage < cPages; iPage++)
        pPgLst->aPages[iPage] = paPages[iPage];

    rc = VbglHGCMCall(pClient->handle, &pData->callInfo, cbData);
    if (RT_SUCCESS (rc))
    {
        rc = pData->callInfo.result;
        *pcbBuffer = pData->cb.u.value32;
    }

    RTMemTmpFree(pData);
    return rc;
}

DECLVBGL(int) vboxCallWrite(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                            uint64_t offset, uint32_t *pcbBuffer, uint8_t *pBuffer, bool fLocked)
{
    int rc = VINF_SUCCESS;

    VBoxSFWrite data;

    VBOX_INIT_CALL(&data.callInfo, WRITE, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;
    data.offset.type                    = VMMDevHGCMParmType_64bit;
    data.offset.u.value64               = offset;
    data.cb.type                        = VMMDevHGCMParmType_32bit;
    data.cb.u.value32                   = *pcbBuffer;
    data.buffer.type                    = (fLocked) ? VMMDevHGCMParmType_LinAddr_Locked_In : VMMDevHGCMParmType_LinAddr_In;
    data.buffer.u.Pointer.size          = *pcbBuffer;
    data.buffer.u.Pointer.u.linearAddr  = (uintptr_t)pBuffer;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallWrite: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
        *pcbBuffer = data.cb.u.value32;
    }
    return rc;
}

DECLVBGL(int) VbglR0SfWritePhysCont(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile, uint64_t offset, uint32_t *pcbBuffer, RTCCPHYS PhysBuffer)
{
    uint32_t            cbToWrite = *pcbBuffer;
    uint32_t            cPages    = RT_ALIGN_32((PhysBuffer & PAGE_OFFSET_MASK) + cbToWrite, PAGE_SIZE) >> PAGE_SHIFT;
    uint32_t            cbData    = (uint32_t)(sizeof(VBoxSFWrite) + RT_UOFFSETOF(HGCMPageListInfo, aPages[cPages]));
    VBoxSFWrite        *pData     = (VBoxSFWrite *)RTMemTmpAlloc(cbData);
    HGCMPageListInfo   *pPgLst    = (HGCMPageListInfo *)(pData + 1);
    uint32_t            iPage;
    int                 rc;

    if (RT_UNLIKELY(!pData))
        return VERR_NO_TMP_MEMORY;

    VBOX_INIT_CALL(&pData->callInfo, WRITE, pClient);

    pData->root.type                      = VMMDevHGCMParmType_32bit;
    pData->root.u.value32                 = pMap->root;

    pData->handle.type                    = VMMDevHGCMParmType_64bit;
    pData->handle.u.value64               = hFile;
    pData->offset.type                    = VMMDevHGCMParmType_64bit;
    pData->offset.u.value64               = offset;
    pData->cb.type                        = VMMDevHGCMParmType_32bit;
    pData->cb.u.value32                   = cbToWrite;
    pData->buffer.type                    = VMMDevHGCMParmType_PageList;
    pData->buffer.u.PageList.size         = cbToWrite;
    pData->buffer.u.PageList.offset       = sizeof(VBoxSFWrite);

    pPgLst->flags = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    pPgLst->offFirstPage = (uint16_t)(PhysBuffer & PAGE_OFFSET_MASK);
    pPgLst->cPages = cPages;
    PhysBuffer &= ~(RTCCPHYS)PAGE_OFFSET_MASK;
    for (iPage = 0; iPage < cPages; iPage++, PhysBuffer += PAGE_SIZE)
        pPgLst->aPages[iPage] = PhysBuffer;

    rc = VbglHGCMCall (pClient->handle, &pData->callInfo, cbData);
    if (RT_SUCCESS (rc))
    {
        rc = pData->callInfo.result;
        *pcbBuffer = pData->cb.u.value32;
    }

    RTMemTmpFree(pData);
    return rc;

}

DECLVBGL(int) VbglR0SharedFolderWritePageList(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                                              uint64_t offset, uint32_t *pcbBuffer,
                                              uint16_t offFirstPage, uint16_t cPages, RTGCPHYS64 *paPages)
{
    uint32_t            cbToWrite = *pcbBuffer;
    uint32_t            cbData    = (uint32_t)(sizeof(VBoxSFWrite) + RT_UOFFSETOF(HGCMPageListInfo, aPages[cPages]));
    VBoxSFWrite        *pData     = (VBoxSFWrite *)RTMemTmpAlloc(cbData);
    HGCMPageListInfo   *pPgLst    = (HGCMPageListInfo *)(pData + 1);
    uint16_t            iPage;
    int                 rc;

    if (RT_UNLIKELY(!pData))
        return VERR_NO_TMP_MEMORY;

    VBOX_INIT_CALL(&pData->callInfo, WRITE, pClient);

    pData->root.type                      = VMMDevHGCMParmType_32bit;
    pData->root.u.value32                 = pMap->root;

    pData->handle.type                    = VMMDevHGCMParmType_64bit;
    pData->handle.u.value64               = hFile;
    pData->offset.type                    = VMMDevHGCMParmType_64bit;
    pData->offset.u.value64               = offset;
    pData->cb.type                        = VMMDevHGCMParmType_32bit;
    pData->cb.u.value32                   = cbToWrite;
    pData->buffer.type                    = VMMDevHGCMParmType_PageList;
    pData->buffer.u.PageList.size         = cbToWrite;
    pData->buffer.u.PageList.offset       = sizeof(VBoxSFWrite);

    pPgLst->flags = VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    pPgLst->offFirstPage = offFirstPage;
    pPgLst->cPages = cPages;
    for (iPage = 0; iPage < cPages; iPage++)
        pPgLst->aPages[iPage] = paPages[iPage];

    rc = VbglHGCMCall (pClient->handle, &pData->callInfo, cbData);
    if (RT_SUCCESS (rc))
    {
        rc = pData->callInfo.result;
        *pcbBuffer = pData->cb.u.value32;
    }

    RTMemTmpFree(pData);
    return rc;
}

DECLVBGL(int) vboxCallFlush(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile)
{
    int rc = VINF_SUCCESS;

    VBoxSFFlush data;

    VBOX_INIT_CALL(&data.callInfo, FLUSH, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallFlush: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallDirInfo (
    PVBSFCLIENT pClient,
    PVBSFMAP pMap,
    SHFLHANDLE hFile,
    PSHFLSTRING ParsedPath,
    uint32_t flags,
    uint32_t index,
    uint32_t *pcbBuffer,
    PSHFLDIRINFO pBuffer,
    uint32_t *pcFiles)
{
    int rc = VINF_SUCCESS;

    VBoxSFList data;

    VBOX_INIT_CALL(&data.callInfo, LIST, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;
    data.flags.type                     = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                = flags;
    data.cb.type                        = VMMDevHGCMParmType_32bit;
    data.cb.u.value32                   = *pcbBuffer;
    data.path.type                      = VMMDevHGCMParmType_LinAddr_In;
    data.path.u.Pointer.size            =
        (ParsedPath) ? ShflStringSizeOfBuffer(ParsedPath) : 0;
    data.path.u.Pointer.u.linearAddr    = (uintptr_t) ParsedPath;

    data.buffer.type                    = VMMDevHGCMParmType_LinAddr_Out;
    data.buffer.u.Pointer.size          = *pcbBuffer;
    data.buffer.u.Pointer.u.linearAddr  = (uintptr_t)pBuffer;

    data.resumePoint.type               = VMMDevHGCMParmType_32bit;
    data.resumePoint.u.value32          = index;
    data.cFiles.type                    = VMMDevHGCMParmType_32bit;
    data.cFiles.u.value32               = 0; /* out parameters only */

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallDirInfo: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    *pcbBuffer = data.cb.u.value32;
    *pcFiles   = data.cFiles.u.value32;
    return rc;
}

DECLVBGL(int) vboxCallFSInfo(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                             uint32_t flags, uint32_t *pcbBuffer, PSHFLDIRINFO pBuffer)
{
    int rc = VINF_SUCCESS;

    VBoxSFInformation data;

    VBOX_INIT_CALL(&data.callInfo, INFORMATION, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;
    data.flags.type                     = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                = flags;
    data.cb.type                        = VMMDevHGCMParmType_32bit;
    data.cb.u.value32                   = *pcbBuffer;
    data.info.type                      = VMMDevHGCMParmType_LinAddr;
    data.info.u.Pointer.size            = *pcbBuffer;
    data.info.u.Pointer.u.linearAddr    = (uintptr_t)pBuffer;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallFileInfo: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
        *pcbBuffer = data.cb.u.value32;
    }
    return rc;
}

DECLVBGL(int) vboxCallLock(PVBSFCLIENT pClient, PVBSFMAP pMap, SHFLHANDLE hFile,
                           uint64_t offset, uint64_t cbSize, uint32_t fLock)
{
    int rc = VINF_SUCCESS;

    VBoxSFLock data;

    VBOX_INIT_CALL(&data.callInfo, LOCK, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.handle.type                    = VMMDevHGCMParmType_64bit;
    data.handle.u.value64               = hFile;
    data.offset.type                    = VMMDevHGCMParmType_64bit;
    data.offset.u.value64               = offset;
    data.length.type                    = VMMDevHGCMParmType_64bit;
    data.length.u.value64               = cbSize;

    data.flags.type                     = VMMDevHGCMParmType_32bit;
    data.flags.u.value32                = fLock;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallLock: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallSetUtf8 (PVBSFCLIENT pClient)
{
    int rc = VINF_SUCCESS;

    VBoxGuestHGCMCallInfo callInfo;

    VBOX_INIT_CALL (&callInfo, SET_UTF8, pClient);
    rc = VbglHGCMCall (pClient->handle, &callInfo, sizeof (callInfo));
    if (RT_SUCCESS (rc))
    {
        rc = callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxReadLink (PVBSFCLIENT pClient, PVBSFMAP pMap, PSHFLSTRING pParsedPath,
                            uint32_t cbBuffer, uint8_t *pBuffer)
{
    int rc = VINF_SUCCESS;

    VBoxSFReadLink data;

    VBOX_INIT_CALL(&data.callInfo, READLINK, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.path.type                      = VMMDevHGCMParmType_LinAddr_In;
    data.path.u.Pointer.size            = ShflStringSizeOfBuffer (pParsedPath);
    data.path.u.Pointer.u.linearAddr    = (uintptr_t)pParsedPath;

    data.buffer.type                    = VMMDevHGCMParmType_LinAddr_Out;
    data.buffer.u.Pointer.size          = cbBuffer;
    data.buffer.u.Pointer.u.linearAddr  = (uintptr_t)pBuffer;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallReadline: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallSymlink (PVBSFCLIENT pClient, PVBSFMAP pMap, PSHFLSTRING pNewPath, PSHFLSTRING pOldPath,
                               PSHFLFSOBJINFO pBuffer)
{
    int rc = VINF_SUCCESS;

    VBoxSFSymlink data;

    VBOX_INIT_CALL(&data.callInfo, SYMLINK, pClient);

    data.root.type                      = VMMDevHGCMParmType_32bit;
    data.root.u.value32                 = pMap->root;

    data.newPath.type                   = VMMDevHGCMParmType_LinAddr_In;
    data.newPath.u.Pointer.size         = ShflStringSizeOfBuffer (pNewPath);
    data.newPath.u.Pointer.u.linearAddr = (uintptr_t)pNewPath;

    data.oldPath.type                   = VMMDevHGCMParmType_LinAddr_In;
    data.oldPath.u.Pointer.size         = ShflStringSizeOfBuffer (pOldPath);
    data.oldPath.u.Pointer.u.linearAddr = (uintptr_t)pOldPath;

    data.info.type                      = VMMDevHGCMParmType_LinAddr_Out;
    data.info.u.Pointer.size            = sizeof(SHFLFSOBJINFO);
    data.info.u.Pointer.u.linearAddr    = (uintptr_t)pBuffer;

    rc = VbglHGCMCall (pClient->handle, &data.callInfo, sizeof (data));

/*    Log(("VBOXSF: VBoxSF::vboxCallSymlink: "
         "VbglHGCMCall rc = %#x, result = %#x\n", rc, data.callInfo.result));
*/
    if (RT_SUCCESS (rc))
    {
        rc = data.callInfo.result;
    }
    return rc;
}

DECLVBGL(int) vboxCallSetSymlinks (PVBSFCLIENT pClient)
{
    int rc = VINF_SUCCESS;

    VBoxGuestHGCMCallInfo callInfo;

    VBOX_INIT_CALL (&callInfo, SET_SYMLINKS, pClient);
    rc = VbglHGCMCall (pClient->handle, &callInfo, sizeof (callInfo));
    if (RT_SUCCESS (rc))
    {
        rc = callInfo.result;
    }
    return rc;
}


#endif /* !VBGL_VBOXGUEST */
