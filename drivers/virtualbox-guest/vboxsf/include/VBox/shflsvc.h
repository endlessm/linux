/** @file
 * Shared Folders - Common header for host service and guest clients.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOX_INCLUDED_shflsvc_h
#define VBOX_INCLUDED_shflsvc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef IN_MODULE
# include <VBox/VMMDevCoreTypes.h>
# include <VBox/VBoxGuestCoreTypes.h>
#endif
#include <iprt/string.h>
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/fs.h>
#include <iprt/assert.h>
#include <iprt/errcore.h>
#if defined(IN_RING3) || (defined(IN_RING0) && defined(RT_OS_DARWIN))
# include <iprt/mem.h>
#endif
#include <iprt/utf16.h>



/** @defgroup grp_vbox_shfl     Shared Folder Interface Definition.
 *
 * Structures shared between guest and the service can be relocated and use
 * offsets to point to variable length parts.
 *
 * Shared folders protocol works with handles.  Before doing any action on a
 * file system object, one have to obtain the object handle via a SHFL_FN_CREATE
 * request. A handle must be closed with SHFL_FN_CLOSE.
 *
 * @{
 */

/** @name Some bit flag manipulation macros.
 * @{  */
#ifndef BIT_FLAG
#define BIT_FLAG(__Field,__Flag)       ((__Field) & (__Flag))
#endif

#ifndef BIT_FLAG_SET
#define BIT_FLAG_SET(__Field,__Flag)   ((__Field) |= (__Flag))
#endif

#ifndef BIT_FLAG_CLEAR
#define BIT_FLAG_CLEAR(__Field,__Flag) ((__Field) &= ~(__Flag))
#endif
/** @} */


/** @name Shared Folders service functions. (guest)
 * @{
 */
/** Query mappings changes.
 * @note Description is currently misleading, it will always return all
 *       current mappings with SHFL_MS_NEW status.  Only modification is the
 *       SHFL_MF_AUTOMOUNT flag that causes filtering out non-auto mounts. */
#define SHFL_FN_QUERY_MAPPINGS      (1)
/** Query the name of a map. */
#define SHFL_FN_QUERY_MAP_NAME      (2)
/** Open/create object. */
#define SHFL_FN_CREATE              (3)
/** Close object handle. */
#define SHFL_FN_CLOSE               (4)
/** Read object content. */
#define SHFL_FN_READ                (5)
/** Write new object content. */
#define SHFL_FN_WRITE               (6)
/** Lock/unlock a range in the object. */
#define SHFL_FN_LOCK                (7)
/** List object content. */
#define SHFL_FN_LIST                (8)
/** Query/set object information. */
#define SHFL_FN_INFORMATION         (9)
/** Remove object */
#define SHFL_FN_REMOVE              (11)
/** Map folder (legacy) */
#define SHFL_FN_MAP_FOLDER_OLD      (12)
/** Unmap folder */
#define SHFL_FN_UNMAP_FOLDER        (13)
/** Rename object (possibly moving it to another directory) */
#define SHFL_FN_RENAME              (14)
/** Flush file */
#define SHFL_FN_FLUSH               (15)
/** @todo macl, a description, please. */
#define SHFL_FN_SET_UTF8            (16)
/** Map folder */
#define SHFL_FN_MAP_FOLDER          (17)
/** Read symlink destination.
 * @since VBox 4.0  */
#define SHFL_FN_READLINK            (18) /**< @todo rename to SHFL_FN_READ_LINK (see struct capitalization) */
/** Create symlink.
 * @since VBox 4.0  */
#define SHFL_FN_SYMLINK             (19)
/** Ask host to show symlinks
 * @since VBox 4.0  */
#define SHFL_FN_SET_SYMLINKS        (20)
/** Query information about a map.
 * @since VBox 6.0  */
#define SHFL_FN_QUERY_MAP_INFO      (21)
/** Wait for changes to the mappings.
 * @since VBox 6.0  */
#define SHFL_FN_WAIT_FOR_MAPPINGS_CHANGES       (22)
/** Cancel all waits for changes to the mappings for the calling client.
 * The wait calls will return VERR_CANCELLED.
 * @since VBox 6.0  */
#define SHFL_FN_CANCEL_MAPPINGS_CHANGES_WAITS   (23)
/** Sets the file size.
 * @since VBox 6.0  */
#define SHFL_FN_SET_FILE_SIZE       (24)
/** Queries supported features.
 * @since VBox 6.0.6  */
#define SHFL_FN_QUERY_FEATURES      (25)
/** Copies a file to another.
 * @since VBox 6.0.6  */
#define SHFL_FN_COPY_FILE           (26)
/** Copies part of a file to another.
 * @since VBox 6.0.6  */
#define SHFL_FN_COPY_FILE_PART      (27)
/** Close handle to (optional) and remove object by path.
 * This function is tailored for Windows guests.
 * @since VBox 6.0.8  */
#define SHFL_FN_CLOSE_AND_REMOVE    (28)
/** Set the host error code style.
 * This is for more efficiently getting the correct error status when the host
 * and guest OS types differs and it won't happen naturally.
 * @since VBox 6.0.10  */
#define SHFL_FN_SET_ERROR_STYLE     (29)
/** The last function number. */
#define SHFL_FN_LAST                SHFL_FN_SET_ERROR_STYLE
/** @} */


/** @name Shared Folders service functions. (host)
 * @{
 */
/** Add shared folder mapping. */
#define SHFL_FN_ADD_MAPPING         (1)
/** Remove shared folder mapping. */
#define SHFL_FN_REMOVE_MAPPING      (2)
/** Set the led status light address. */
#define SHFL_FN_SET_STATUS_LED      (3)
/** Allow the guest to create symbolic links
 * @since VBox 4.0  */
#define SHFL_FN_ALLOW_SYMLINKS_CREATE (4)
/** @} */


/** Root handle for a mapping. Root handles are unique.
 *
 * @note Function parameters structures consider the root handle as 32 bit
 *       value. If the typedef will be changed, then function parameters must be
 *       changed accordingly. All those parameters are marked with SHFLROOT in
 *       comments.
 */
typedef uint32_t SHFLROOT;

/** NIL shared folder root handle. */
#define SHFL_ROOT_NIL ((SHFLROOT)~0)


/** A shared folders handle for an opened object. */
typedef uint64_t SHFLHANDLE;

#define SHFL_HANDLE_NIL  ((SHFLHANDLE)~0LL)
#define SHFL_HANDLE_ROOT ((SHFLHANDLE)0LL)

/** Hardcoded maximum length (in chars) of a shared folder name. */
#define SHFL_MAX_LEN         (256)
/** Hardcoded maximum number of shared folder mapping available to the guest. */
#define SHFL_MAX_MAPPINGS    (64)


/** @name Shared Folders strings. They can be either UTF-8 or UTF-16.
 * @{
 */

/**
 * Shared folder string buffer structure.
 */
typedef struct _SHFLSTRING
{
    /** Allocated size of the String member in bytes. */
    uint16_t u16Size;

    /** Length of string without trailing nul in bytes. */
    uint16_t u16Length;

    /** UTF-8 or UTF-16 string. Nul terminated. */
    union
    {
#if 1
        char     ach[1];                /**< UTF-8 but with a type that makes some more sense. */
        uint8_t  utf8[1];
        RTUTF16  utf16[1];
        uint16_t ucs2[1];                                 /**< misnomer, use utf16. */
#else
        uint8_t  utf8[RT_FLEXIBLE_ARRAY_IN_NESTED_UNION];
        RTUTF16  utf16[RT_FLEXIBLE_ARRAY_IN_NESTED_UNION];
        RTUTF16  ucs2[RT_FLEXIBLE_ARRAY_IN_NESTED_UNION]; /**< misnomer, use utf16. */
#endif
    } String;
} SHFLSTRING;
AssertCompileSize(RTUTF16, 2);
AssertCompileSize(SHFLSTRING, 6);
AssertCompileMemberOffset(SHFLSTRING, String, 4);
/** The size of SHFLSTRING w/o the string part. */
#define SHFLSTRING_HEADER_SIZE  4
AssertCompileMemberOffset(SHFLSTRING, String, SHFLSTRING_HEADER_SIZE);

/** Pointer to a shared folder string buffer. */
typedef SHFLSTRING *PSHFLSTRING;
/** Pointer to a const shared folder string buffer. */
typedef const SHFLSTRING *PCSHFLSTRING;

/** Calculate size of the string. */
DECLINLINE(uint32_t) ShflStringSizeOfBuffer(PCSHFLSTRING pString)
{
    return pString ? (uint32_t)(SHFLSTRING_HEADER_SIZE + pString->u16Size) : 0;
}

DECLINLINE(uint32_t) ShflStringLength(PCSHFLSTRING pString)
{
    return pString ? pString->u16Length : 0;
}

DECLINLINE(PSHFLSTRING) ShflStringInitBuffer(void *pvBuffer, uint32_t u32Size)
{
    PSHFLSTRING pString = NULL;
    const uint32_t u32HeaderSize = SHFLSTRING_HEADER_SIZE;

    /*
     * Check that the buffer size is big enough to hold a zero sized string
     * and is not too big to fit into 16 bit variables.
     */
    if (u32Size >= u32HeaderSize && u32Size - u32HeaderSize <= 0xFFFF)
    {
        pString = (PSHFLSTRING)pvBuffer;
        pString->u16Size = (uint16_t)(u32Size - u32HeaderSize);
        pString->u16Length = 0;
        if (pString->u16Size >= sizeof(pString->String.ucs2[0]))
            pString->String.ucs2[0] = 0;
        else if (pString->u16Size >= sizeof(pString->String.utf8[0]))
            pString->String.utf8[0] = 0;
    }

    return pString;
}

/**
 * Helper for copying one string into another.
 *
 * @returns IPRT status code.
 * @retval  VERR_BUFFER_OVERFLOW and pDst->u16Length set to source length.
 * @param   pDst        The destination string.
 * @param   pSrc        The source string.
 * @param   cbTerm      The size of the string terminator.
 */
DECLINLINE(int) ShflStringCopy(PSHFLSTRING pDst, PCSHFLSTRING pSrc, size_t cbTerm)
{
    int rc = VINF_SUCCESS;
    if (pDst->u16Size >= pSrc->u16Length + cbTerm)
    {
        memcpy(&pDst->String, &pSrc->String, pSrc->u16Length);
        switch (cbTerm)
        {
            default:
            case 2: pDst->String.ach[pSrc->u16Length + 1] = '\0'; RT_FALL_THROUGH();
            case 1: pDst->String.ach[pSrc->u16Length + 0] = '\0'; break;
            case 0: break;
        }
    }
    else
        rc = VERR_BUFFER_OVERFLOW;
    pDst->u16Length = pSrc->u16Length;
    return rc;
}

#if defined(IN_RING3) \
 || (defined(IN_RING0) && defined(RT_OS_DARWIN))

/**
 * Duplicates a string using RTMemAlloc as allocator.
 *
 * @returns Copy, NULL if out of memory.
 * @param   pSrc        The source string.
 */
DECLINLINE(PSHFLSTRING) ShflStringDup(PCSHFLSTRING pSrc)
{
    PSHFLSTRING pDst = (PSHFLSTRING)RTMemAlloc(SHFLSTRING_HEADER_SIZE + pSrc->u16Size);
    if (pDst)
    {
        pDst->u16Length = pSrc->u16Length;
        pDst->u16Size   = pSrc->u16Size;
        memcpy(&pDst->String, &pSrc->String, pSrc->u16Size);
    }
    return pDst;
}

/**
 * Duplicates a UTF-16 string using RTMemAlloc as allocator.
 *
 * The returned string will be using UTF-16 encoding too.
 *
 * @returns Pointer to copy on success - pass to RTMemFree to free.
 *          NULL if out of memory.
 * @param   pwszSrc     The source string.  Encoding is not checked.
 */
DECLINLINE(PSHFLSTRING) ShflStringDupUtf16(PCRTUTF16 pwszSrc)
{
    size_t cwcSrc = RTUtf16Len(pwszSrc);
    if (cwcSrc < UINT16_MAX / sizeof(RTUTF16))
    {
        PSHFLSTRING pDst = (PSHFLSTRING)RTMemAlloc(SHFLSTRING_HEADER_SIZE + (cwcSrc + 1) * sizeof(RTUTF16));
        if (pDst)
        {
            pDst->u16Length = (uint16_t)(cwcSrc * sizeof(RTUTF16));
            pDst->u16Size   = (uint16_t)((cwcSrc + 1) * sizeof(RTUTF16));
            memcpy(&pDst->String, pwszSrc, (cwcSrc + 1) * sizeof(RTUTF16));
            return pDst;
        }
    }
    AssertFailed();
    return NULL;
}

/**
 * Duplicates a UTF-8 string using RTMemAlloc as allocator.
 *
 * The returned string will be using UTF-8 encoding too.
 *
 * @returns Pointer to copy on success - pass to RTMemFree to free.
 *          NULL if out of memory.
 * @param   pszSrc      The source string.  Encoding is not checked.
 */
DECLINLINE(PSHFLSTRING) ShflStringDupUtf8(const char *pszSrc)
{
    size_t cchSrc = strlen(pszSrc);
    if (cchSrc < UINT16_MAX)
    {
        PSHFLSTRING pDst = (PSHFLSTRING)RTMemAlloc(SHFLSTRING_HEADER_SIZE + cchSrc + 1);
        if (pDst)
        {
            pDst->u16Length = (uint16_t)cchSrc;
            pDst->u16Size   = (uint16_t)(cchSrc + 1);
            memcpy(&pDst->String, pszSrc, cchSrc + 1);
            return pDst;
        }
    }
    AssertFailed();
    return NULL;
}

/**
 * Creates a UTF-16 duplicate of the UTF-8 string @a pszSrc using RTMemAlloc as
 * allocator.
 *
 * @returns Pointer to copy on success - pass to RTMemFree to free.
 *          NULL if out of memory or invalid UTF-8 encoding.
 * @param   pszSrc      The source string.
 */
DECLINLINE(PSHFLSTRING) ShflStringDupUtf8AsUtf16(const char *pszSrc)
{
    size_t cwcConversion = 0;
    int rc = RTStrCalcUtf16LenEx(pszSrc, RTSTR_MAX, &cwcConversion);
    if (   RT_SUCCESS(rc)
        && cwcConversion < UINT16_MAX / sizeof(RTUTF16))
    {
        PSHFLSTRING pDst = (PSHFLSTRING)RTMemAlloc(SHFLSTRING_HEADER_SIZE + (cwcConversion + 1) * sizeof(RTUTF16));
        if (pDst)
        {
            PRTUTF16 pwszDst = pDst->String.ucs2;
            pDst->u16Size = (uint16_t)((cwcConversion + 1) * sizeof(RTUTF16));
            rc = RTStrToUtf16Ex(pszSrc, RTSTR_MAX, &pwszDst, cwcConversion + 1, &cwcConversion);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                pDst->u16Length = (uint16_t)(cwcConversion * sizeof(RTUTF16));
                return pDst;
            }
            RTMemFree(pDst);
        }
    }
    AssertMsgFailed(("rc=%Rrc cwcConversion=%#x\n", rc, cwcConversion));
    return NULL;
}

/**
 * Copies a UTF-8 string to a buffer as UTF-16.
 *
 * @returns IPRT status code.
 * @param   pDst        The destination buffer.
 * @param   pszSrc      The source string.
 * @param   cchSrc      The source string length, or RTSTR_MAX.
 */
DECLINLINE(int) ShflStringCopyUtf8AsUtf16(PSHFLSTRING pDst, const char *pszSrc, size_t cchSrc)
{
    int rc;
    size_t cwcDst = 0;
    if (pDst->u16Size >= sizeof(RTUTF16))
    {
        PRTUTF16 pwszDst = pDst->String.utf16;
        rc = RTStrToUtf16Ex(pszSrc, cchSrc, &pwszDst, pDst->u16Size / sizeof(RTUTF16), &cwcDst);
    }
    else
    {
        RTStrCalcUtf16LenEx(pszSrc, cchSrc, &cwcDst);
        rc = VERR_BUFFER_OVERFLOW;
    }
    pDst->u16Length = (uint16_t)(cwcDst * sizeof(RTUTF16));
    return rc != VERR_BUFFER_OVERFLOW || cwcDst < UINT16_MAX / sizeof(RTUTF16) ? rc : VERR_TOO_MUCH_DATA;
}

/**
 * Copies a UTF-8 string buffer to another buffer as UTF-16
 *
 * @returns IPRT status code.
 * @param   pDst        The destination buffer (UTF-16).
 * @param   pSrc        The source buffer (UTF-8).
 */
DECLINLINE(int) ShflStringCopyUtf8BufAsUtf16(PSHFLSTRING pDst, PCSHFLSTRING pSrc)
{
    return ShflStringCopyUtf8AsUtf16(pDst, pSrc->String.ach, pSrc->u16Length);
}

/**
 * Copies a UTF-16 string to a buffer as UTF-8
 *
 * @returns IPRT status code.
 * @param   pDst        The destination buffer.
 * @param   pwszSrc     The source string.
 * @param   cwcSrc      The source string length, or RTSTR_MAX.
 */
DECLINLINE(int) ShflStringCopyUtf16AsUtf8(PSHFLSTRING pDst, PCRTUTF16 pwszSrc, size_t cwcSrc)
{
    int rc;
    size_t cchDst = 0;
    if (pDst->u16Size > 0)
    {
        char *pszDst = pDst->String.ach;
        rc = RTUtf16ToUtf8Ex(pwszSrc, cwcSrc, &pszDst, pDst->u16Size, &cchDst);
    }
    else
    {
        RTUtf16CalcUtf8LenEx(pwszSrc, cwcSrc, &cchDst);
        rc = VERR_BUFFER_OVERFLOW;
    }
    pDst->u16Length = (uint16_t)cchDst;
    return rc != VERR_BUFFER_OVERFLOW || cchDst < UINT16_MAX ? rc : VERR_TOO_MUCH_DATA;
}

/**
 * Copies a UTF-16 string buffer to another buffer as UTF-8
 *
 * @returns IPRT status code.
 * @param   pDst        The destination buffer (UTF-8).
 * @param   pSrc        The source buffer (UTF-16).
 */
DECLINLINE(int) ShflStringCopyUtf16BufAsUtf8(PSHFLSTRING pDst, PCSHFLSTRING pSrc)
{
    return ShflStringCopyUtf16AsUtf8(pDst, pSrc->String.utf16, pSrc->u16Length / sizeof(RTUTF16));
}

#endif /* IN_RING3 */

/**
 * Validates a HGCM string output parameter.
 *
 * @returns true if valid, false if not.
 *
 * @param   pString     The string buffer pointer.
 * @param   cbBuf       The buffer size from the parameter.
 */
DECLINLINE(bool) ShflStringIsValidOut(PCSHFLSTRING pString, uint32_t cbBuf)
{
    if (RT_LIKELY(cbBuf > RT_UOFFSETOF(SHFLSTRING, String)))
        if (RT_LIKELY((uint32_t)pString->u16Size + RT_UOFFSETOF(SHFLSTRING, String) <= cbBuf))
            if (RT_LIKELY(pString->u16Length < pString->u16Size))
                return true;
    return false;
}

/**
 * Validates a HGCM string input parameter.
 *
 * @returns true if valid, false if not.
 *
 * @param   pString     The string buffer pointer.
 * @param   cbBuf       The buffer size from the parameter.
 * @param   fUtf8Not16  Set if UTF-8 encoding, clear if UTF-16 encoding.
 */
DECLINLINE(bool) ShflStringIsValidIn(PCSHFLSTRING pString, uint32_t cbBuf, bool fUtf8Not16)
{
    int rc;
    if (RT_LIKELY(cbBuf > RT_UOFFSETOF(SHFLSTRING, String)))
    {
        if (RT_LIKELY((uint32_t)pString->u16Size + RT_UOFFSETOF(SHFLSTRING, String) <= cbBuf))
        {
            if (fUtf8Not16)
            {
                /* UTF-8: */
                if (RT_LIKELY(pString->u16Length < pString->u16Size))
                {
                    rc = RTStrValidateEncodingEx((const char *)&pString->String.utf8[0], pString->u16Length + 1,
                                                 RTSTR_VALIDATE_ENCODING_EXACT_LENGTH | RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
                    if (RT_SUCCESS(rc))
                        return true;
                }
            }
            else
            {
                /* UTF-16: */
                if (RT_LIKELY(!(pString->u16Length & 1)))
                {
                    if (RT_LIKELY((uint32_t)sizeof(RTUTF16) + pString->u16Length <= pString->u16Size))
                    {
                        rc = RTUtf16ValidateEncodingEx(&pString->String.ucs2[0], pString->u16Length / 2 + 1,
                                                       RTSTR_VALIDATE_ENCODING_EXACT_LENGTH
                                                       | RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
                        if (RT_SUCCESS(rc))
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

/**
 * Validates an optional HGCM string input parameter.
 *
 * @returns true if valid, false if not.
 *
 * @param   pString     The string buffer pointer. Can be NULL.
 * @param   cbBuf       The buffer size from the parameter.
 * @param   fUtf8Not16  Set if UTF-8 encoding, clear if UTF-16 encoding.
 */
DECLINLINE(bool) ShflStringIsValidOrNullIn(PCSHFLSTRING pString, uint32_t cbBuf, bool fUtf8Not16)
{
    if (pString)
        return ShflStringIsValidIn(pString, cbBuf, fUtf8Not16);
    if (RT_LIKELY(cbBuf == 0))
        return true;
    return false;
}

/** Macro for passing as string as a HGCM parmeter (pointer)  */
#define SHFLSTRING_TO_HGMC_PARAM(a_pParam, a_pString) \
    do { \
        (a_pParam)->type = VBOX_HGCM_SVC_PARM_PTR; \
        (a_pParam)->u.pointer.addr = (a_pString); \
        (a_pParam)->u.pointer.size = ShflStringSizeOfBuffer(a_pString); \
    } while (0)

/** @} */


/**
 * The available additional information in a SHFLFSOBJATTR object.
 */
typedef enum SHFLFSOBJATTRADD
{
    /** No additional information is available / requested. */
    SHFLFSOBJATTRADD_NOTHING = 1,
    /** The additional unix attributes (SHFLFSOBJATTR::u::Unix) are
     *  available / requested. */
    SHFLFSOBJATTRADD_UNIX,
    /** The additional extended attribute size (SHFLFSOBJATTR::u::EASize) is
     *  available / requested. */
    SHFLFSOBJATTRADD_EASIZE,
    /** The last valid item (inclusive).
     * The valid range is SHFLFSOBJATTRADD_NOTHING thru
     * SHFLFSOBJATTRADD_LAST. */
    SHFLFSOBJATTRADD_LAST = SHFLFSOBJATTRADD_EASIZE,

    /** The usual 32-bit hack. */
    SHFLFSOBJATTRADD_32BIT_SIZE_HACK = 0x7fffffff
} SHFLFSOBJATTRADD;


/* Assert sizes of the IRPT types we're using below. */
AssertCompileSize(RTFMODE,      4);
AssertCompileSize(RTFOFF,       8);
AssertCompileSize(RTINODE,      8);
AssertCompileSize(RTTIMESPEC,   8);
AssertCompileSize(RTDEV,        4);
AssertCompileSize(RTUID,        4);

/**
 * Shared folder filesystem object attributes.
 */
#pragma pack(1)
typedef struct SHFLFSOBJATTR
{
    /** Mode flags (st_mode). RTFS_UNIX_*, RTFS_TYPE_*, and RTFS_DOS_*.
     * @remarks We depend on a number of RTFS_ defines to remain unchanged.
     *          Fortuntately, these are depending on windows, dos and unix
     *          standard values, so this shouldn't be much of a pain. */
    RTFMODE         fMode;

    /** The additional attributes available. */
    SHFLFSOBJATTRADD  enmAdditional;

    /**
     * Additional attributes.
     *
     * Unless explicitly specified to an API, the API can provide additional
     * data as it is provided by the underlying OS.
     */
    union SHFLFSOBJATTRUNION
    {
        /** Additional Unix Attributes
         * These are available when SHFLFSOBJATTRADD is set in fUnix.
         */
         struct SHFLFSOBJATTRUNIX
         {
            /** The user owning the filesystem object (st_uid).
             * This field is ~0U if not supported. */
            RTUID           uid;

            /** The group the filesystem object is assigned (st_gid).
             * This field is ~0U if not supported. */
            RTGID           gid;

            /** Number of hard links to this filesystem object (st_nlink).
             * This field is 1 if the filesystem doesn't support hardlinking or
             * the information isn't available.
             */
            uint32_t        cHardlinks;

            /** The device number of the device which this filesystem object resides on (st_dev).
             * This field is 0 if this information is not available. */
            RTDEV           INodeIdDevice;

            /** The unique identifier (within the filesystem) of this filesystem object (st_ino).
             * Together with INodeIdDevice, this field can be used as a OS wide unique id
             * when both their values are not 0.
             * This field is 0 if the information is not available. */
            RTINODE         INodeId;

            /** User flags (st_flags).
             * This field is 0 if this information is not available. */
            uint32_t        fFlags;

            /** The current generation number (st_gen).
             * This field is 0 if this information is not available. */
            uint32_t        GenerationId;

            /** The device number of a character or block device type object (st_rdev).
             * This field is 0 if the file isn't of a character or block device type and
             * when the OS doesn't subscribe to the major+minor device idenfication scheme. */
            RTDEV           Device;
        } Unix;

        /**
         * Extended attribute size.
         */
        struct SHFLFSOBJATTREASIZE
        {
            /** Size of EAs. */
            RTFOFF          cb;
        } EASize;
    } u;
} SHFLFSOBJATTR;
#pragma pack()
AssertCompileSize(SHFLFSOBJATTR, 44);
/** Pointer to a shared folder filesystem object attributes structure. */
typedef SHFLFSOBJATTR *PSHFLFSOBJATTR;
/** Pointer to a const shared folder filesystem object attributes structure. */
typedef const SHFLFSOBJATTR *PCSHFLFSOBJATTR;


/**
 * Filesystem object information structure.
 */
#pragma pack(1)
typedef struct SHFLFSOBJINFO
{
   /** Logical size (st_size).
    * For normal files this is the size of the file.
    * For symbolic links, this is the length of the path name contained
    * in the symbolic link.
    * For other objects this fields needs to be specified.
    */
   RTFOFF       cbObject;

   /** Disk allocation size (st_blocks * DEV_BSIZE). */
   RTFOFF       cbAllocated;

   /** Time of last access (st_atime).
    * @remarks  Here (and other places) we depend on the IPRT timespec to
    *           remain unchanged. */
   RTTIMESPEC   AccessTime;

   /** Time of last data modification (st_mtime). */
   RTTIMESPEC   ModificationTime;

   /** Time of last status change (st_ctime).
    * If not available this is set to ModificationTime.
    */
   RTTIMESPEC   ChangeTime;

   /** Time of file birth (st_birthtime).
    * If not available this is set to ChangeTime.
    */
   RTTIMESPEC   BirthTime;

   /** Attributes. */
   SHFLFSOBJATTR Attr;

} SHFLFSOBJINFO;
#pragma pack()
AssertCompileSize(SHFLFSOBJINFO, 92);
/** Pointer to a shared folder filesystem object information structure. */
typedef SHFLFSOBJINFO *PSHFLFSOBJINFO;
/** Pointer to a const shared folder filesystem object information
 *  structure. */
typedef const SHFLFSOBJINFO *PCSHFLFSOBJINFO;


/**
 * Copy file system objinfo from IPRT to shared folder format.
 *
 * @param   pDst                The shared folder structure.
 * @param   pSrc                The IPRT structure.
 */
DECLINLINE(void) vbfsCopyFsObjInfoFromIprt(PSHFLFSOBJINFO pDst, PCRTFSOBJINFO pSrc)
{
    pDst->cbObject          = pSrc->cbObject;
    pDst->cbAllocated       = pSrc->cbAllocated;
    pDst->AccessTime        = pSrc->AccessTime;
    pDst->ModificationTime  = pSrc->ModificationTime;
    pDst->ChangeTime        = pSrc->ChangeTime;
    pDst->BirthTime         = pSrc->BirthTime;
    pDst->Attr.fMode        = pSrc->Attr.fMode;
    /* Clear bits which we don't pass through for security reasons. */
    pDst->Attr.fMode       &= ~(RTFS_UNIX_ISUID | RTFS_UNIX_ISGID | RTFS_UNIX_ISTXT);
    RT_ZERO(pDst->Attr.u);
    switch (pSrc->Attr.enmAdditional)
    {
        default:
        case RTFSOBJATTRADD_NOTHING:
            pDst->Attr.enmAdditional        = SHFLFSOBJATTRADD_NOTHING;
            break;

        case RTFSOBJATTRADD_UNIX:
            pDst->Attr.enmAdditional        = SHFLFSOBJATTRADD_UNIX;
            pDst->Attr.u.Unix.uid           = pSrc->Attr.u.Unix.uid;
            pDst->Attr.u.Unix.gid           = pSrc->Attr.u.Unix.gid;
            pDst->Attr.u.Unix.cHardlinks    = pSrc->Attr.u.Unix.cHardlinks;
            pDst->Attr.u.Unix.INodeIdDevice = pSrc->Attr.u.Unix.INodeIdDevice;
            pDst->Attr.u.Unix.INodeId       = pSrc->Attr.u.Unix.INodeId;
            pDst->Attr.u.Unix.fFlags        = pSrc->Attr.u.Unix.fFlags;
            pDst->Attr.u.Unix.GenerationId  = pSrc->Attr.u.Unix.GenerationId;
            pDst->Attr.u.Unix.Device        = pSrc->Attr.u.Unix.Device;
            break;

        case RTFSOBJATTRADD_EASIZE:
            pDst->Attr.enmAdditional        = SHFLFSOBJATTRADD_EASIZE;
            pDst->Attr.u.EASize.cb          = pSrc->Attr.u.EASize.cb;
            break;
    }
}


/** Result of an open/create request.
 *  Along with handle value the result code
 *  identifies what has happened while
 *  trying to open the object.
 */
typedef enum _SHFLCREATERESULT
{
    SHFL_NO_RESULT,
    /** Specified path does not exist. */
    SHFL_PATH_NOT_FOUND,
    /** Path to file exists, but the last component does not. */
    SHFL_FILE_NOT_FOUND,
    /** File already exists and either has been opened or not. */
    SHFL_FILE_EXISTS,
    /** New file was created. */
    SHFL_FILE_CREATED,
    /** Existing file was replaced or overwritten. */
    SHFL_FILE_REPLACED,
    /** Blow the type up to 32-bit. */
    SHFL_32BIT_HACK = 0x7fffffff
} SHFLCREATERESULT;
AssertCompile(SHFL_NO_RESULT == 0);
AssertCompileSize(SHFLCREATERESULT, 4);


/** @name Open/create flags.
 *  @{
 */

/** No flags. Initialization value. */
#define SHFL_CF_NONE                  (0x00000000)

/** Lookup only the object, do not return a handle. All other flags are ignored. */
#define SHFL_CF_LOOKUP                (0x00000001)

/** Open parent directory of specified object.
 *  Useful for the corresponding Windows FSD flag
 *  and for opening paths like \\dir\\*.* to search the 'dir'.
 *  @todo possibly not needed???
 */
#define SHFL_CF_OPEN_TARGET_DIRECTORY (0x00000002)

/** Create/open a directory. */
#define SHFL_CF_DIRECTORY             (0x00000004)

/** Open/create action to do if object exists
 *  and if the object does not exists.
 *  REPLACE file means atomically DELETE and CREATE.
 *  OVERWRITE file means truncating the file to 0 and
 *  setting new size.
 *  When opening an existing directory REPLACE and OVERWRITE
 *  actions are considered invalid, and cause returning
 *  FILE_EXISTS with NIL handle.
 */
#define SHFL_CF_ACT_MASK_IF_EXISTS      (0x000000F0)
#define SHFL_CF_ACT_MASK_IF_NEW         (0x00000F00)

/** What to do if object exists. */
#define SHFL_CF_ACT_OPEN_IF_EXISTS      (0x00000000)
#define SHFL_CF_ACT_FAIL_IF_EXISTS      (0x00000010)
#define SHFL_CF_ACT_REPLACE_IF_EXISTS   (0x00000020)
#define SHFL_CF_ACT_OVERWRITE_IF_EXISTS (0x00000030)

/** What to do if object does not exist. */
#define SHFL_CF_ACT_CREATE_IF_NEW       (0x00000000)
#define SHFL_CF_ACT_FAIL_IF_NEW         (0x00000100)

/** Read/write requested access for the object. */
#define SHFL_CF_ACCESS_MASK_RW          (0x00003000)

/** No access requested. */
#define SHFL_CF_ACCESS_NONE             (0x00000000)
/** Read access requested. */
#define SHFL_CF_ACCESS_READ             (0x00001000)
/** Write access requested. */
#define SHFL_CF_ACCESS_WRITE            (0x00002000)
/** Read/Write access requested. */
#define SHFL_CF_ACCESS_READWRITE        (SHFL_CF_ACCESS_READ | SHFL_CF_ACCESS_WRITE)

/** Requested share access for the object. */
#define SHFL_CF_ACCESS_MASK_DENY        (0x0000C000)

/** Allow any access. */
#define SHFL_CF_ACCESS_DENYNONE         (0x00000000)
/** Do not allow read. */
#define SHFL_CF_ACCESS_DENYREAD         (0x00004000)
/** Do not allow write. */
#define SHFL_CF_ACCESS_DENYWRITE        (0x00008000)
/** Do not allow access. */
#define SHFL_CF_ACCESS_DENYALL          (SHFL_CF_ACCESS_DENYREAD | SHFL_CF_ACCESS_DENYWRITE)

/** Requested access to attributes of the object. */
#define SHFL_CF_ACCESS_MASK_ATTR        (0x00030000)

/** No access requested. */
#define SHFL_CF_ACCESS_ATTR_NONE        (0x00000000)
/** Read access requested. */
#define SHFL_CF_ACCESS_ATTR_READ        (0x00010000)
/** Write access requested. */
#define SHFL_CF_ACCESS_ATTR_WRITE       (0x00020000)
/** Read/Write access requested. */
#define SHFL_CF_ACCESS_ATTR_READWRITE   (SHFL_CF_ACCESS_ATTR_READ | SHFL_CF_ACCESS_ATTR_WRITE)

/** The file is opened in append mode. Ignored if SHFL_CF_ACCESS_WRITE is not set. */
#define SHFL_CF_ACCESS_APPEND           (0x00040000)

/** @} */

#pragma pack(1)
typedef struct _SHFLCREATEPARMS
{
    /* Returned handle of opened object. */
    SHFLHANDLE Handle;

    /* Returned result of the operation */
    SHFLCREATERESULT Result;

    /* SHFL_CF_* */
    uint32_t CreateFlags;

    /* Attributes of object to create and
     * returned actual attributes of opened/created object.
     */
    SHFLFSOBJINFO Info;

} SHFLCREATEPARMS;
#pragma pack()

typedef SHFLCREATEPARMS *PSHFLCREATEPARMS;


/** @name Shared Folders mappings.
 * @{
 */

/** The mapping has been added since last query. */
#define SHFL_MS_NEW        (1)
/** The mapping has been deleted since last query. */
#define SHFL_MS_DELETED    (2)

typedef struct _SHFLMAPPING
{
    /** Mapping status.
     * @note Currently always set to SHFL_MS_NEW.  */
    uint32_t u32Status;
    /** Root handle. */
    SHFLROOT root;
} SHFLMAPPING;
/** Pointer to a SHFLMAPPING structure. */
typedef SHFLMAPPING *PSHFLMAPPING;

/** @} */


/** @name Shared Folder directory information
 * @{
 */

typedef struct _SHFLDIRINFO
{
    /** Full information about the object. */
    SHFLFSOBJINFO   Info;
    /** The length of the short field (number of RTUTF16 chars).
     * It is 16-bit for reasons of alignment. */
    uint16_t        cucShortName;
    /** The short name for 8.3 compatibility.
     * Empty string if not available.
     */
    RTUTF16         uszShortName[14];
    /** @todo malc, a description, please. */
    SHFLSTRING      name;
} SHFLDIRINFO, *PSHFLDIRINFO;


/**
 * Shared folder filesystem properties.
 */
typedef struct SHFLFSPROPERTIES
{
    /** The maximum size of a filesystem object name.
     * This does not include the '\\0'. */
    uint32_t cbMaxComponent;

    /** True if the filesystem is remote.
     * False if the filesystem is local. */
    bool    fRemote;

    /** True if the filesystem is case sensitive.
     * False if the filesystem is case insensitive. */
    bool    fCaseSensitive;

    /** True if the filesystem is mounted read only.
     * False if the filesystem is mounted read write. */
    bool    fReadOnly;

    /** True if the filesystem can encode unicode object names.
     * False if it can't. */
    bool    fSupportsUnicode;

    /** True if the filesystem is compresses.
     * False if it isn't or we don't know. */
    bool    fCompressed;

    /** True if the filesystem compresses of individual files.
     * False if it doesn't or we don't know. */
    bool    fFileCompression;

    /** @todo more? */
} SHFLFSPROPERTIES;
AssertCompileSize(SHFLFSPROPERTIES, 12);
/** Pointer to a shared folder filesystem properties structure. */
typedef SHFLFSPROPERTIES *PSHFLFSPROPERTIES;
/** Pointer to a const shared folder filesystem properties structure. */
typedef SHFLFSPROPERTIES const *PCSHFLFSPROPERTIES;


/**
 * Copy file system properties from IPRT to shared folder format.
 *
 * @param   pDst                The shared folder structure.
 * @param   pSrc                The IPRT structure.
 */
DECLINLINE(void) vbfsCopyFsPropertiesFromIprt(PSHFLFSPROPERTIES pDst, PCRTFSPROPERTIES pSrc)
{
    RT_ZERO(*pDst);                     /* zap the implicit padding. */
    pDst->cbMaxComponent   = pSrc->cbMaxComponent;
    pDst->fRemote          = pSrc->fRemote;
    pDst->fCaseSensitive   = pSrc->fCaseSensitive;
    pDst->fReadOnly        = pSrc->fReadOnly;
    pDst->fSupportsUnicode = pSrc->fSupportsUnicode;
    pDst->fCompressed      = pSrc->fCompressed;
    pDst->fFileCompression = pSrc->fFileCompression;
}


typedef struct _SHFLVOLINFO
{
    RTFOFF         ullTotalAllocationBytes;
    RTFOFF         ullAvailableAllocationBytes;
    uint32_t       ulBytesPerAllocationUnit;
    uint32_t       ulBytesPerSector;
    uint32_t       ulSerial;
    SHFLFSPROPERTIES fsProperties;
} SHFLVOLINFO, *PSHFLVOLINFO;

/** @} */


/** @defgroup grp_vbox_shfl_params  Function parameter structures.
 * @{
 */

/** @name SHFL_FN_QUERY_MAPPINGS
 * @{
 */
/** Validation mask.  Needs to be adjusted
  * whenever a new SHFL_MF_ flag is added. */
#define SHFL_MF_MASK       (0x00000011)
/** UTF-16 enconded strings. */
#define SHFL_MF_UCS2       (0x00000000)
/** Guest uses UTF8 strings, if not set then the strings are unicode (UCS2). */
#define SHFL_MF_UTF8       (0x00000001)
/** Just handle the auto-mounted folders. */
#define SHFL_MF_AUTOMOUNT  (0x00000010)

/** Parameters structure. */
typedef struct _VBoxSFQueryMappings
{
    VBGLIOCHGCMCALL callInfo;

    /** 32bit, in:
     * Flags describing various client needs.
     */
    HGCMFunctionParameter flags;

    /** 32bit, in/out:
     * Number of mappings the client expects.
     * This is the number of elements in the
     * mappings array.
     */
    HGCMFunctionParameter numberOfMappings;

    /** pointer, in/out:
     * Points to array of SHFLMAPPING structures.
     */
    HGCMFunctionParameter mappings;

} VBoxSFQueryMappings;

/** Number of parameters */
#define SHFL_CPARMS_QUERY_MAPPINGS (3)
/** @} */


/** @name SHFL_FN_QUERY_MAP_NAME
 * @{
 */

/** Parameters structure. */
typedef struct _VBoxSFQueryMapName
{
    VBGLIOCHGCMCALL callInfo;

    /** 32bit, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in/out:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter name;

} VBoxSFQueryMapName;

/** Number of parameters */
#define SHFL_CPARMS_QUERY_MAP_NAME (2)
/** @} */


/** @name SHFL_FN_MAP_FOLDER_OLD
 * @{
 */

/** Parameters structure. */
typedef struct _VBoxSFMapFolder_Old
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter path;

    /** pointer, out: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in: RTUTF16
     * Path delimiter
     */
    HGCMFunctionParameter delimiter;

} VBoxSFMapFolder_Old;

/** Number of parameters */
#define SHFL_CPARMS_MAP_FOLDER_OLD (3)
/** @} */


/** @name SHFL_FN_MAP_FOLDER
 * @{
 */

/** SHFL_FN_MAP_FOLDER parameters. */
typedef struct VBoxSFParmMapFolder
{
    /** pointer, in: SHFLSTRING with the name of the folder to map. */
    HGCMFunctionParameter pStrName;
    /** value32, out: The root ID (SHFLROOT) of the mapping. */
    HGCMFunctionParameter id32Root;
    /** value32, in: Path delimiter code point. */
    HGCMFunctionParameter uc32Delimiter;
    /** value32, in: case senstive flag */
    HGCMFunctionParameter fCaseSensitive;
} VBoxSFParmMapFolder;

/** Parameters structure. */
typedef struct _VBoxSFMapFolder
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter path;

    /** pointer, out: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in: RTUTF16
     * Path delimiter
     */
    HGCMFunctionParameter delimiter;

    /** pointer, in: SHFLROOT
     * Case senstive flag
     */
    HGCMFunctionParameter fCaseSensitive;

} VBoxSFMapFolder;

/** Number of parameters */
#define SHFL_CPARMS_MAP_FOLDER (4)
/** @} */


/** @name SHFL_FN_UNMAP_FOLDER
 * @{
 */

/** SHFL_FN_UNMAP_FOLDER parameters. */
typedef struct VBoxSFParmUnmapFolder
{
    /** value32, in: SHFLROOT of the mapping to unmap */
    HGCMFunctionParameter id32Root;
} VBoxSFParmUnmapFolder;

/** Parameters structure. */
typedef struct _VBoxSFUnmapFolder
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

} VBoxSFUnmapFolder;

/** Number of parameters */
#define SHFL_CPARMS_UNMAP_FOLDER (1)
/** @} */


/** @name SHFL_FN_CREATE
 * @{
 */

/** SHFL_FN_CREATE parameters. */
typedef struct VBoxSFParmCreate
{
    /** value32, in: SHFLROOT
     * Root handle of the mapping which name is queried.  */
    HGCMFunctionParameter id32Root;
    /** pointer, in: Points to SHFLSTRING buffer. */
    HGCMFunctionParameter pStrPath;
    /** pointer, in/out:  Points to SHFLCREATEPARMS buffer. */
    HGCMFunctionParameter pCreateParms;
} VBoxSFParmCreate;

/** Parameters structure. */
typedef struct _VBoxSFCreate
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter path;

    /** pointer, in/out:
     * Points to SHFLCREATEPARMS buffer.
     */
    HGCMFunctionParameter parms;

} VBoxSFCreate;

/** Number of parameters */
#define SHFL_CPARMS_CREATE (3)
/** @} */


/** @name SHFL_FN_CLOSE
 * @{
 */

/** SHFL_FN_CLOSE parameters. */
typedef struct VBoxSFParmClose
{
    /** value32, in: SHFLROOT of the mapping with the handle. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of object to close. */
    HGCMFunctionParameter u64Handle;
} VBoxSFParmClose;

/** Parameters structure. */
typedef struct _VBoxSFClose
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;


    /** value64, in:
     * SHFLHANDLE of object to close.
     */
    HGCMFunctionParameter handle;

} VBoxSFClose;

/** Number of parameters */
#define SHFL_CPARMS_CLOSE (2)
/** @} */


/** @name  SHFL_FN_READ
 * @{
 */

/** SHFL_FN_READ parameters. */
typedef struct VBoxSFParmRead
{
    /** value32, in: SHFLROOT of the mapping with the handle. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of object to read from . */
    HGCMFunctionParameter u64Handle;
    /** value64, in: Offset to start reading from. */
    HGCMFunctionParameter off64Read;
    /** value32, in/out: How much to try read / Actually read. */
    HGCMFunctionParameter cb32Read;
    /** pointer, out: Buffer to return the data in. */
    HGCMFunctionParameter pBuf;
} VBoxSFParmRead;

/** Parameters structure. */
typedef struct _VBoxSFRead
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to read from.
     */
    HGCMFunctionParameter handle;

    /** value64, in:
     * Offset to read from.
     */
    HGCMFunctionParameter offset;

    /** value64, in/out:
     * Bytes to read/How many were read.
     */
    HGCMFunctionParameter cb;

    /** pointer, out:
     * Buffer to place data to.
     */
    HGCMFunctionParameter buffer;

} VBoxSFRead;

/** Number of parameters */
#define SHFL_CPARMS_READ (5)
/** @} */


/** @name SHFL_FN_WRITE
 * @{
 */

/** SHFL_FN_WRITE parameters. */
typedef struct VBoxSFParmWrite
{
    /** value32, in: SHFLROOT of the mapping with the handle. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of object to write to. */
    HGCMFunctionParameter u64Handle;
    /** value64, in/out: Offset to start writing at / New offset.
     * @note The new offset isn't necessarily off + cb for files opened with
     *       SHFL_CF_ACCESS_APPEND since other parties (host programs, other VMs,
     *       other computers) could have extended the file since the last time the
     *       guest got a fresh size statistic.  So, this helps the guest avoiding
     *       a stat call to check the actual size. */
    HGCMFunctionParameter off64Write;
    /** value32, in/out: How much to try write / Actually written. */
    HGCMFunctionParameter cb32Write;
    /** pointer, out: Buffer to return the data in. */
    HGCMFunctionParameter pBuf;
} VBoxSFParmWrite;

/** Parameters structure. */
typedef struct _VBoxSFWrite
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to write to.
     */
    HGCMFunctionParameter handle;

    /** value64, in/out:
     * Offset to write to/New offset.
     * @note The new offset isn't necessarily off + cb for files opened with
     *       SHFL_CF_ACCESS_APPEND since other parties (host programs, other VMs,
     *       other computers) could have extended the file since the last time the
     *       guest got a fresh size statistic.  So, this helps the guest avoiding
     *       a stat call to check the actual size.
     */
    HGCMFunctionParameter offset;

    /** value64, in/out:
     * Bytes to write/How many were written.
     */
    HGCMFunctionParameter cb;

    /** pointer, in:
     * Data to write.
     */
    HGCMFunctionParameter buffer;

} VBoxSFWrite;

/** Number of parameters */
#define SHFL_CPARMS_WRITE (5)
/** @} */


/** @name SHFL_FN_LOCK
 * @remarks Lock owner is the HGCM client.
 * @{
 */

/** Lock mode bit mask. */
#define SHFL_LOCK_MODE_MASK  (0x3)
/** Cancel lock on the given range. */
#define SHFL_LOCK_CANCEL     (0x0)
/** Acquire read only lock. Prevent write to the range. */
#define SHFL_LOCK_SHARED     (0x1)
/** Acquire write lock. Prevent both write and read to the range. */
#define SHFL_LOCK_EXCLUSIVE  (0x2)

/** Do not wait for lock if it can not be acquired at the time. */
#define SHFL_LOCK_NOWAIT     (0x0)
/** Wait and acquire lock. */
#define SHFL_LOCK_WAIT       (0x4)

/** Lock the specified range. */
#define SHFL_LOCK_PARTIAL    (0x0)
/** Lock entire object. */
#define SHFL_LOCK_ENTIRE     (0x8)

/** Parameters structure. */
typedef struct _VBoxSFLock
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to be locked.
     */
    HGCMFunctionParameter handle;

    /** value64, in:
     * Starting offset of lock range.
     */
    HGCMFunctionParameter offset;

    /** value64, in:
     * Length of range.
     */
    HGCMFunctionParameter length;

    /** value32, in:
     * Lock flags SHFL_LOCK_*.
     */
    HGCMFunctionParameter flags;

} VBoxSFLock;

/** Number of parameters */
#define SHFL_CPARMS_LOCK (5)
/** @} */


/** @name SHFL_FN_FLUSH
 * @{
 */

/** SHFL_FN_FLUSH parameters. */
typedef struct VBoxSFParmFlush
{
    /** value32, in: SHFLROOT of the mapping with the handle. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of object to flush. */
    HGCMFunctionParameter u64Handle;
} VBoxSFParmFlush;

/** Parameters structure. */
typedef struct _VBoxSFFlush
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to be locked.
     */
    HGCMFunctionParameter handle;

} VBoxSFFlush;

/** Number of parameters */
#define SHFL_CPARMS_FLUSH (2)
/** @} */


/** @name SHFL_FN_SET_UTF8
 * @{ */
/** NUmber of parameters for SHFL_FN_SET_UTF8.   */
#define SHFL_CPARMS_SET_UTF8 (0)
/** @} */


/** @name SHFL_FN_LIST
 * @remarks Listing information includes variable length RTDIRENTRY[EX]
 *          structures.
 * @{
 */

/** @todo might be necessary for future. */
#define SHFL_LIST_NONE          0
#define SHFL_LIST_RETURN_ONE    1
#define SHFL_LIST_RESTART       2

/** SHFL_FN_LIST parameters. */
typedef struct VBoxSFParmList
{
    /** value32, in: SHFLROOT of the mapping the handle belongs to. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of the directory. */
    HGCMFunctionParameter u64Handle;
    /** value32, in: List flags SHFL_LIST_XXX. */
    HGCMFunctionParameter f32Flags;
    /** value32, in/out: Input buffer size / Returned bytes count. */
    HGCMFunctionParameter cb32Buffer;
    /** pointer, in[optional]: SHFLSTRING filter string (full path). */
    HGCMFunctionParameter pStrFilter;
    /** pointer, out: Buffer to return listing information in (SHFLDIRINFO).
     * When SHFL_LIST_RETURN_ONE is not specfied, multiple record may be
     * returned, deriving the entry size using SHFLDIRINFO::name.u16Size.  */
    HGCMFunctionParameter pBuffer;
    /** value32, out: Set to 0 if the listing is done, 1 if there are more entries.
     * @note Must be set to zero on call as it was declared in/out parameter and
     *       may be used as such again. */
    HGCMFunctionParameter f32More;
    /** value32, out:  Number of entries returned. */
    HGCMFunctionParameter c32Entries;
} VBoxSFParmList;


/** Parameters structure. */
typedef struct _VBoxSFList
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to be listed.
     */
    HGCMFunctionParameter handle;

    /** value32, in:
     * List flags SHFL_LIST_*.
     */
    HGCMFunctionParameter flags;

    /** value32, in/out:
     * Bytes to be used for listing information/How many bytes were used.
     */
    HGCMFunctionParameter cb;

    /** pointer, in/optional
     * Points to SHFLSTRING buffer that specifies a search path.
     */
    HGCMFunctionParameter path;

    /** pointer, out:
     * Buffer to place listing information to. (SHFLDIRINFO)
     */
    HGCMFunctionParameter buffer;

    /** value32, in/out:
     * Indicates a key where the listing must be resumed.
     * in: 0 means start from begin of object.
     * out: 0 means listing completed.
     */
    HGCMFunctionParameter resumePoint;

    /** pointer, out:
     * Number of files returned
     */
    HGCMFunctionParameter cFiles;

} VBoxSFList;

/** Number of parameters */
#define SHFL_CPARMS_LIST (8)
/** @} */


/** @name SHFL_FN_READLINK
 * @{
 */

/** SHFL_FN_READLINK parameters. */
typedef struct VBoxSFParmReadLink
{
    /** value32, in: SHFLROOT of the mapping which the symlink is read. */
    HGCMFunctionParameter id32Root;
    /** pointer, in: SHFLSTRING full path to the symlink. */
    HGCMFunctionParameter pStrPath;
    /** pointer, out: Buffer to place the symlink target into.
     * @note Buffer contains UTF-8 characters on success, regardless of the
     *       UTF-8/UTF-16 setting of the connection.  Will be zero terminated.
     *
     * @todo r=bird: This should've been a string!
     * @todo r=bird: There should've been a byte count returned! */
    HGCMFunctionParameter pBuffer;
} VBoxSFParmReadLink;

/** Parameters structure. */
typedef struct _VBoxSFReadLink
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter path;

    /** pointer, out:
     * Buffer to place data to.
     * @note Buffer contains UTF-8 characters on success, regardless of the
     *       UTF-8/UTF-16 setting of the connection.  Will be zero terminated.
     */
    HGCMFunctionParameter buffer;

} VBoxSFReadLink;

/** Number of parameters */
#define SHFL_CPARMS_READLINK (3)
/** @} */


/** @name SHFL_FN_INFORMATION
 * @{
 */

/** Mask of Set/Get bit. */
#define SHFL_INFO_MODE_MASK    (0x1)
/** Get information */
#define SHFL_INFO_GET          (0x0)
/** Set information */
#define SHFL_INFO_SET          (0x1)

/** Get name of the object. */
#define SHFL_INFO_NAME         (0x2)
/** Set size of object (extend/trucate); only applies to file objects */
#define SHFL_INFO_SIZE         (0x4)
/** Get/Set file object info. */
#define SHFL_INFO_FILE         (0x8)
/** Get volume information. */
#define SHFL_INFO_VOLUME       (0x10)

/** @todo different file info structures */

/** SHFL_FN_INFORMATION parameters. */
typedef struct VBoxSFParmInformation
{
    /** value32, in: SHFLROOT of the mapping the handle belongs to. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of object to be queried/set. */
    HGCMFunctionParameter u64Handle;
    /** value32, in: SHFL_INFO_XXX  */
    HGCMFunctionParameter f32Flags;
    /** value32, in/out: Bytes to be used for information/How many bytes were used.  */
    HGCMFunctionParameter cb32;
    /** pointer, in/out: Information to be set/get (SHFLFSOBJINFO, SHFLVOLINFO, or SHFLSTRING).
     * Do not forget to set the SHFLFSOBJINFO::Attr::enmAdditional for Get operation as well.  */
    HGCMFunctionParameter pInfo;
} VBoxSFParmInformation;


/** Parameters structure. */
typedef struct _VBoxSFInformation
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** value64, in:
     * SHFLHANDLE of object to be listed.
     */
    HGCMFunctionParameter handle;

    /** value32, in:
     * SHFL_INFO_*
     */
    HGCMFunctionParameter flags;

    /** value32, in/out:
     * Bytes to be used for information/How many bytes were used.
     */
    HGCMFunctionParameter cb;

    /** pointer, in/out:
     * Information to be set/get (SHFLFSOBJINFO or SHFLSTRING). Do not forget
     * to set the SHFLFSOBJINFO::Attr::enmAdditional for Get operation as well.
     */
    HGCMFunctionParameter info;

} VBoxSFInformation;

/** Number of parameters */
#define SHFL_CPARMS_INFORMATION (5)
/** @}  */


/** @name SHFL_FN_REMOVE
 * @{
 */

#define SHFL_REMOVE_FILE        (0x1)
#define SHFL_REMOVE_DIR         (0x2)
#define SHFL_REMOVE_SYMLINK     (0x4)

/** SHFL_FN_REMOVE parameters. */
typedef struct VBoxSFParmRemove
{
    /** value32, in: SHFLROOT of the mapping the path is relative to. */
    HGCMFunctionParameter id32Root;
    /** pointer, in: Points to SHFLSTRING buffer. */
    HGCMFunctionParameter pStrPath;
    /** value32, in: SHFL_REMOVE_XXX */
    HGCMFunctionParameter f32Flags;
} VBoxSFParmRemove;

/** Parameters structure. */
typedef struct _VBoxSFRemove
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in:
     * Points to SHFLSTRING buffer.
     */
    HGCMFunctionParameter path;

    /** value32, in:
     * remove flags (file/directory)
     */
    HGCMFunctionParameter flags;

} VBoxSFRemove;

#define SHFL_CPARMS_REMOVE  (3)
/** @} */


/** @name SHFL_FN_CLOSE_AND_REMOVE
 * Extends SHFL_FN_REMOVE with a 4th handle parameter that can be nil.
 * @{
 */
/** SHFL_FN_CLOSE_AND_REMOVE parameters. */
typedef struct VBoxSFParmCloseAndRemove
{
    /** value32, in: SHFLROOT of the mapping the path is relative to. */
    HGCMFunctionParameter id32Root;
    /** pointer, in: Points to SHFLSTRING buffer. */
    HGCMFunctionParameter pStrPath;
    /** value32, in: SHFL_REMOVE_XXX */
    HGCMFunctionParameter f32Flags;
    /** value64, in: SHFLHANDLE to the object to be removed & close, optional. */
    HGCMFunctionParameter u64Handle;
} VBoxSFParmCloseAndRemove;
/** Number of parameters */
#define SHFL_CPARMS_CLOSE_AND_REMOVE    (4)
AssertCompileSize(VBoxSFParmCloseAndRemove, SHFL_CPARMS_CLOSE_AND_REMOVE * sizeof(HGCMFunctionParameter));
/** @} */


/** @name SHFL_FN_RENAME
 * @{
 */

#define SHFL_RENAME_FILE                (0x1)
#define SHFL_RENAME_DIR                 (0x2)
#define SHFL_RENAME_REPLACE_IF_EXISTS   (0x4)

/** SHFL_FN_RENAME parameters. */
typedef struct VBoxSFParmRename
{
    /** value32, in: SHFLROOT of the mapping the paths are relative to. */
    HGCMFunctionParameter id32Root;
    /** pointer, in: SHFLSTRING giving the source (old) path. */
    HGCMFunctionParameter pStrSrcPath;
    /** pointer, in: SHFLSTRING giving the destination (new) path. */
    HGCMFunctionParameter pStrDstPath;
    /** value32, in: SHFL_RENAME_XXX  */
    HGCMFunctionParameter f32Flags;
} VBoxSFParmRename;

/** Parameters structure. */
typedef struct _VBoxSFRename
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in:
     * Points to SHFLSTRING src.
     */
    HGCMFunctionParameter src;

    /** pointer, in:
     * Points to SHFLSTRING dest.
     */
    HGCMFunctionParameter dest;

    /** value32, in:
     * rename flags (file/directory)
     */
    HGCMFunctionParameter flags;

} VBoxSFRename;

#define SHFL_CPARMS_RENAME  (4)
/** @} */


/** @name SHFL_FN_SYMLINK
 * @{
 */

/** Parameters structure. */
typedef struct VBoxSFParmCreateSymlink
{
    /** value32, in: SHFLROOT of the mapping the symlink should be created on. */
    HGCMFunctionParameter id32Root;
    /** pointer, in: SHFLSTRING giving the path to the symlink. */
    HGCMFunctionParameter pStrSymlink;
    /** pointer, in: SHFLSTRING giving the target. */
    HGCMFunctionParameter pStrTarget;
    /** pointer, out: SHFLFSOBJINFO buffer to be filled with info about the created symlink. */
    HGCMFunctionParameter pInfo;
} VBoxSFParmCreateSymlink;

/** Parameters structure. */
typedef struct _VBoxSFSymlink
{
    VBGLIOCHGCMCALL callInfo;

    /** pointer, in: SHFLROOT
     * Root handle of the mapping which name is queried.
     */
    HGCMFunctionParameter root;

    /** pointer, in:
     * Points to SHFLSTRING of path for the new symlink.
     */
    HGCMFunctionParameter newPath;

    /** pointer, in:
     * Points to SHFLSTRING of destination for symlink.
     */
    HGCMFunctionParameter oldPath;

    /** pointer, out:
     * Information about created symlink.
     */
    HGCMFunctionParameter info;

} VBoxSFSymlink;

#define SHFL_CPARMS_SYMLINK  (4)
/** @} */


/** @name SHFL_FN_SET_SYMLINKS
 * @{ */
/** NUmber of parameters for SHFL_FN_SET_SYMLINKS.   */
#define SHFL_CPARMS_SET_SYMLINKS (0)
/** @} */


/** @name SHFL_FN_QUERY_MAP_INFO
 * @{
 */
/** Query flag: Guest prefers drive letters as mount points. */
#define SHFL_MIQF_DRIVE_LETTER      RT_BIT_64(0)
/** Query flag: Guest prefers paths as mount points. */
#define SHFL_MIQF_PATH              RT_BIT_64(1)

/** Set if writable. */
#define SHFL_MIF_WRITABLE           RT_BIT_64(0)
/** Indicates that the mapping should be auto-mounted. */
#define SHFL_MIF_AUTO_MOUNT         RT_BIT_64(1)
/** Set if host is case insensitive. */
#define SHFL_MIF_HOST_ICASE         RT_BIT_64(2)
/** Set if guest is case insensitive. */
#define SHFL_MIF_GUEST_ICASE        RT_BIT_64(3)
/** Symbolic link creation is allowed. */
#define SHFL_MIF_SYMLINK_CREATION   RT_BIT_64(4)

/** Parameters structure. */
typedef struct VBoxSFQueryMapInfo
{
    /** Common header. */
    VBGLIOCHGCMCALL callInfo;
    /** 32-bit, in: SHFLROOT - root handle of the mapping to query. */
    HGCMFunctionParameter root;
    /** pointer, in/out: SHFLSTRING buffer for the name. */
    HGCMFunctionParameter name;
    /** pointer, in/out: SHFLSTRING buffer for the auto mount point. */
    HGCMFunctionParameter mountPoint;
    /** 64-bit, in: SHFL_MIQF_XXX; out: SHFL_MIF_XXX. */
    HGCMFunctionParameter flags;
    /** 32-bit, out: Root ID version number - root handle reuse guard. */
    HGCMFunctionParameter rootIdVersion;
} VBoxSFQueryMapInfo;
/** Number of parameters */
#define SHFL_CPARMS_QUERY_MAP_INFO (5)
/** @} */


/** @name SHFL_FN_WAIT_FOR_MAPPINGS_CHANGES
 *
 * Returns VINF_SUCCESS on change and VINF_TRY_AGAIN when restored from saved
 * state.  If the guest makes too many calls (max 64) VERR_OUT_OF_RESOURCES will
 * be returned.
 *
 * @{
 */
/** Parameters structure. */
typedef struct VBoxSFWaitForMappingsChanges
{
    /** Common header. */
    VBGLIOCHGCMCALL callInfo;
    /** 32-bit, in/out: The mappings configuration version.
     * On input the client sets it to the last config it knows about, on return
     * it holds the current version.  */
    HGCMFunctionParameter version;
} VBoxSFWaitForMappingsChanges;
/** Number of parameters */
#define SHFL_CPARMS_WAIT_FOR_MAPPINGS_CHANGES       (1)
/** @} */


/** @name SHFL_FN_CANCEL_MAPPINGS_CHANGES_WAITS
 * @{
 */
/** Number of parameters */
#define SHFL_CPARMS_CANCEL_MAPPINGS_CHANGES_WAITS   (0)
/** @} */


/** @name SHFL_FN_SET_FILE_SIZE
 * @{
 */
/** SHFL_FN_SET_FILE_SIZE parameters. */
typedef struct VBoxSFParmSetFileSize
{
    /** value32, in: SHFLROOT of the mapping the handle belongs to. */
    HGCMFunctionParameter id32Root;
    /** value64, in: SHFLHANDLE of the file to change the size of. */
    HGCMFunctionParameter u64Handle;
    /** value64, in: The new file size. */
    HGCMFunctionParameter cb64NewSize;
} VBoxSFParmSetFileSize;
/** Number of parameters */
#define SHFL_CPARMS_SET_FILE_SIZE (3)
/** @} */


/** @name SHFL_FN_QUERY_FEATURES
 * @{ */
/** SHFL_FN_QUERY_FEATURES parameters. */
typedef struct VBoxSFParmQueryFeatures
{
    /** value64, out: Feature flags, SHFL_FEATURE_XXX. */
    HGCMFunctionParameter f64Features;
    /** value32, out: The ordinal of the last valid function */
    HGCMFunctionParameter u32LastFunction;
} VBoxSFParmQueryFeatures;
/** Number of parameters for SHFL_FN_QUERY_FEATURES. */
#define SHFL_CPARMS_QUERY_FEATURES (2)

/** The write functions updates the file offset upon return.
 * This can be helpful for files open in append mode. */
#define SHFL_FEATURE_WRITE_UPDATES_OFFSET       RT_BIT_64(0)
/** @} */


/** @name SHFL_FN_COPY_FILE
 * @{ */
/** SHFL_FN_COPY_FILE parameters. */
typedef struct VBoxSFParmCopyFile
{
    /** value32, in: SHFLROOT of the mapping the source handle belongs to. */
    HGCMFunctionParameter id32RootSrc;
    /** pointer, in: SHFLSTRING giving the source file path. */
    HGCMFunctionParameter pStrPathSrc;

    /** value32, in: SHFLROOT of the mapping the destination handle belongs to. */
    HGCMFunctionParameter id32RootDst;
    /** pointer, in: SHFLSTRING giving the destination file path. */
    HGCMFunctionParameter pStrPathDst;

    /** value32, in: Reserved for the future, must be zero. */
    HGCMFunctionParameter f32Flags;
} VBoxSFParmCopyFile;
/** Number of parameters for SHFL_FN_COPY_FILE. */
#define SHFL_CPARMS_COPY_FILE (5)
/** @} */


/** @name SHFL_FN_COPY_FILE_PART
 * @{ */
/** SHFL_FN_COPY_FILE_PART parameters. */
typedef struct VBoxSFParmCopyFilePart
{
    /** value32, in: SHFLROOT of the mapping the source handle belongs to. */
    HGCMFunctionParameter id32RootSrc;
    /** value64, in: SHFLHANDLE of the source file. */
    HGCMFunctionParameter u64HandleSrc;
    /** value64, in: The source file offset. */
    HGCMFunctionParameter off64Src;

    /** value32, in: SHFLROOT of the mapping the destination handle belongs to. */
    HGCMFunctionParameter id32RootDst;
    /** value64, in: SHFLHANDLE of the destination file. */
    HGCMFunctionParameter u64HandleDst;
    /** value64, in: The destination file offset. */
    HGCMFunctionParameter off64Dst;

    /** value64, in/out: The number of bytes to copy on input / bytes actually copied. */
    HGCMFunctionParameter cb64ToCopy;
    /** value32, in: Reserved for the future, must be zero. */
    HGCMFunctionParameter f32Flags;
} VBoxSFParmCopyFilePart;
/** Number of parameters for SHFL_FN_COPY_FILE_PART. */
#define SHFL_CPARMS_COPY_FILE_PART (8)
/** @} */


/** @name SHFL_FN_SET_ERROR_STYLE
 * @{ */
/** The defined error styles. */
typedef enum SHFLERRORSTYLE
{
    kShflErrorStyle_Invalid = 0,
    kShflErrorStyle_Windows,
    kShflErrorStyle_Linux,
    kShflErrorStyle_End,
    kShflErrorStyle_32BitHack = 0x7fffffff
} SHFLERRORSTYLE;
/** The native error style. */
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
# define SHFLERRORSTYLE_NATIVE      kShflErrorStyle_Windows
#else
# define SHFLERRORSTYLE_NATIVE      kShflErrorStyle_Linux
#endif

/** SHFL_FN_SET_ERROR_STYLE parameters. */
typedef struct VBoxSFParmSetErrorStyle
{
    /** value32, in: The style, SHFLERRORSTYLE. */
    HGCMFunctionParameter u32Style;
    /** value32, in: Reserved for the future, must be zero. */
    HGCMFunctionParameter u32Reserved;
} VBoxSFParmSetErrorStyle;
/** Number of parameters for SHFL_FN_SET_ERROR_STYLE. */
#define SHFL_CPARMS_SET_ERROR_STYLE (2)
/** @} */


/** @name SHFL_FN_ADD_MAPPING
 * @note  Host call, no guest structure is used.
 * @{
 */

/** mapping is writable */
#define SHFL_ADD_MAPPING_F_WRITABLE         (RT_BIT_32(0))
/** mapping is automounted by the guest */
#define SHFL_ADD_MAPPING_F_AUTOMOUNT        (RT_BIT_32(1))
/** allow the guest to create symlinks */
#define SHFL_ADD_MAPPING_F_CREATE_SYMLINKS  (RT_BIT_32(2))
/** mapping is actually missing on the host */
#define SHFL_ADD_MAPPING_F_MISSING          (RT_BIT_32(3))

#define SHFL_CPARMS_ADD_MAPPING  (4)
/** @} */


/** @name SHFL_FN_REMOVE_MAPPING
 * @note  Host call, no guest structure is used.
 * @{
 */

#define SHFL_CPARMS_REMOVE_MAPPING (1)
/** @} */


/** @name SHFL_FN_SET_STATUS_LED
 * @note  Host call, no guest structure is used.
 * @{
 */

#define SHFL_CPARMS_SET_STATUS_LED (1)
/** @} */


/** @} */
/** @} */

#endif /* !VBOX_INCLUDED_shflsvc_h */

