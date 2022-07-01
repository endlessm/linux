/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_X86_TDX_H
#define _UAPI_ASM_X86_TDX_H

#include <linux/types.h>
#include <linux/ioctl.h>

/* Length of the REPORTDATA used in TDG.MR.REPORT TDCALL */
#define TDX_REPORTDATA_LEN              64

/* Length of TDREPORT used in TDG.MR.REPORT TDCALL */
#define TDX_REPORT_LEN                  1024

/**
 * struct tdx_report_req: Get TDREPORT using REPORTDATA as input.
 *
 * @subtype        : Subtype of TDREPORT (fixed as 0 by TDX Module
 *                   specification, but added a parameter to handle
 *                   future extension).
 * @reportdata     : User-defined REPORTDATA to be included into
 *                   TDREPORT. Typically it can be some nonce
 *                   provided by attestation service, so the
 *                   generated TDREPORT can be uniquely verified.
 * @rpd_len        : Length of the REPORTDATA (fixed as 64 bytes by
 *                   the TDX Module specification, but parameter is
 *                   added to handle future extension).
 * @tdreport       : TDREPORT output from TDCALL[TDG.MR.REPORT].
 * @tdr_len        : Length of the TDREPORT (fixed as 1024 bytes by
 *                   the TDX Module specification, but a parameter
 *                   is added to accommodate future extension).
 *
 * Used in TDX_CMD_GET_REPORT IOCTL request.
 */
struct tdx_report_req {
	__u8  subtype;
	__u64 reportdata;
	__u32 rpd_len;
	__u64 tdreport;
	__u32 tdr_len;
};

/*
 * TDX_CMD_GET_REPORT - Get TDREPORT using TDCALL[TDG.MR.REPORT]
 *
 * Return 0 on success, -EIO on TDCALL execution failure, and
 * standard errno on other general error cases.
 *
 */
#define TDX_CMD_GET_REPORT		_IOWR('T', 0x01, __u64)

#endif /* _UAPI_ASM_X86_TDX_H */
