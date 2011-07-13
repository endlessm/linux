/* Copyright (c) 2008-2011 Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*

 @File          lnxwrp_ioctls_fm.c

 @Author        Shlomi Gridish

 @Description   FM Linux wrapper functions.

*/

/* Linux Headers ------------------- */
#include <linux/version.h>

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <config/modversions.h>
#endif /* MODVERSIONS */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of_platform.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <sysdev/fsl_soc.h>

#if defined(CONFIG_COMPAT)
#include <linux/compat.h>
#endif

#include "part_ext.h"
#include "fm_ioctls.h"
#include "fm_pcd_ioctls.h"
#include "fm_port_ioctls.h"

#if defined(CONFIG_COMPAT)
#include "lnxwrp_ioctls_fm_compat.h"
#endif

#include "lnxwrp_fm.h"

#define CMP_IOC_DEFINE(def) (IOC_##def != def)

/* fm_pcd_ioctls.h === fm_pcd_ext.h assertions */
#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_PRIVATE_HDRS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_PRS_NUM_OF_HDRS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_KG_NUM_OF_SCHEMES)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS)
#error Error: please synchronize IOC_ defines!
#endif

/* please check for this one in fm_common.h: */
#define FM_PCD_MAX_NUM_OF_OPTIONS(clsPlanEntries)   ((clsPlanEntries==256)? 8:((clsPlanEntries==128)? 7: ((clsPlanEntries==64)? 6: ((clsPlanEntries==32)? 5:0))))
#if (IOC_FM_PCD_MAX_NUM_OF_OPTIONS != FM_PCD_MAX_NUM_OF_OPTIONS(FM_PCD_MAX_NUM_OF_CLS_PLANS))
#error Error: please synchronize IOC_ defines!
#endif
#undef FM_PCD_MAX_NUM_OF_OPTIONS

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_INTERCHANGEABLE_HDRS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_KG_NUM_OF_GENERIC_REGS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_KG_MAX_NUM_OF_EXTRACTS_PER_KEY)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CLS_PLANS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_KG_NUM_OF_EXTRACT_MASKS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_KG_NUM_OF_DEFAULT_GROUPS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_PRS_NUM_OF_LABELS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CC_NODES)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CC_TREES)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CC_GROUPS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CC_UNITS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_KEYS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_SIZE_OF_KEY)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(FM_PCD_MAX_NUM_OF_CC_ENTRIES_IN_GRP)
#error Error: please synchronize IOC_ defines!
#endif

/* net_ioctls.h === net_ext.h assertions */
#if CMP_IOC_DEFINE(NET_HEADER_FIELD_PPP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif


#if CMP_IOC_DEFINE(NET_HEADER_FIELD_PPPoE_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_PPPMUX_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_PPPMUX_SUBFRAME_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_ETH_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IPv4_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IPv6_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_ICMP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IGMP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_TCP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_SCTP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_DCCP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_UDP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_UDP_ENCAP_ESP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IPHC_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_SCTP_CHUNK_DATA_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_L2TPv2_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_L2TPv3_CTRL_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_L2TPv3_SESS_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_VLAN_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_LLC_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_NLPID_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_SNAP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_LLC_SNAP_ALL_FIELDS)
#warning Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_ARP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_RFC2684_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_USER_DEFINED_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_PAYLOAD_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_GRE_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_MINENCAP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IPSEC_AH_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_IPSEC_ESP_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_MPLS_LABEL_STACK_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

#if CMP_IOC_DEFINE(NET_HEADER_FIELD_MACSEC_ALL_FIELDS)
#error Error: please synchronize IOC_ defines!
#endif

/* fm_ioctls.h === fm_ext.h assertions */
#if CMP_IOC_DEFINE(FM_MAX_NUM_OF_VALID_PORTS)
#error Error: please synchronize IOC_ defines!
#endif

/* fm_port_ioctls.h === dpaa_integrations_ext.h assertions */
#if CMP_IOC_DEFINE(FM_PORT_NUM_OF_CONGESTION_GRPS)
#error Error: please synchronize IOC_ defines!
#endif

#define ASSERT_IOC_NET_ENUM(def) ASSERT_COND((unsigned long)e_IOC_NET_##def == (unsigned long)def)

static void LnxwrpAssertions(void)
{
    /* sampling checks */
    ASSERT_IOC_NET_ENUM(HEADER_TYPE_MACSEC);
    ASSERT_IOC_NET_ENUM(HEADER_TYPE_PPP);
    ASSERT_IOC_NET_ENUM(MAX_HEADER_TYPE_COUNT);
    ASSERT_COND((unsigned long)e_IOC_FM_PORT_TYPE_DUMMY == (unsigned long)e_FM_PORT_TYPE_DUMMY);
    ASSERT_COND((unsigned long)e_IOC_FM_EX_MURAM_ECC == (unsigned long)e_FM_EX_MURAM_ECC);
    ASSERT_COND((unsigned long)e_IOC_FM_COUNTERS_SEMAPHOR_SYNC_REJECT == (unsigned long)e_FM_COUNTERS_SEMAPHOR_SYNC_REJECT);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES == (unsigned long)e_FM_PCD_PRS_COUNTERS_FPM_COMMAND_STALL_CYCLES);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PRS_EXCEPTION_SINGLE_ECC == (unsigned long)e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PRS == (unsigned long)e_FM_PCD_PRS);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_EXTRACT_FULL_FIELD == (unsigned long)e_FM_PCD_EXTRACT_FULL_FIELD);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_EXTRACT_FROM_FLOW_ID == (unsigned long)e_FM_PCD_EXTRACT_FROM_FLOW_ID);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO == (unsigned long)e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_KG_DFLT_ILLEGAL == (unsigned long)e_FM_PCD_KG_DFLT_ILLEGAL);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_KG_GENERIC_NOT_FROM_DATA == (unsigned long)e_FM_PCD_KG_GENERIC_NOT_FROM_DATA);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_HDR_INDEX_3 == (unsigned long)e_FM_PCD_HDR_INDEX_3);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_SHARED == (unsigned long)e_FM_PCD_PLCR_SHARED);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_RFC_4115 == (unsigned long)e_FM_PCD_PLCR_RFC_4115);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_COLOR_AWARE == (unsigned long)e_FM_PCD_PLCR_COLOR_AWARE);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_OVERRIDE == (unsigned long)e_FM_PCD_PLCR_OVERRIDE);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_FULL_FRM_LEN == (unsigned long)e_FM_PCD_PLCR_FULL_FRM_LEN);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_ROLLBACK_FULL_FRM_LEN == (unsigned long)e_FM_PCD_PLCR_ROLLBACK_FULL_FRM_LEN);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_PACKET_MODE == (unsigned long)e_FM_PCD_PLCR_PACKET_MODE);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_DROP_FRAME == (unsigned long)e_FM_PCD_DROP_FRAME);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER == (unsigned long)e_FM_PCD_PLCR_PROFILE_RECOLOURED_RED_PACKET_TOTAL_COUNTER);
    ASSERT_COND((unsigned long)e_IOC_FM_PCD_ACTION_INDEXED_LOOKUP == (unsigned long)e_FM_PCD_ACTION_INDEXED_LOOKUP);
    ASSERT_COND((unsigned long)e_IOC_FM_PORT_PCD_SUPPORT_PRS_AND_KG_AND_PLCR == (unsigned long)e_FM_PORT_PCD_SUPPORT_PRS_AND_KG_AND_PLCR);
#ifdef FM_CAPWAP_SUPPORT
    ASSERT_COND((unsigned long)e_IOC_FM_PORT_PCD_SUPPORT_CC_AND_KG_AND_PLCR == (unsigned long)e_FM_PORT_PCD_SUPPORT_CC_AND_KG_AND_PLCR);
#endif
    ASSERT_COND((unsigned long)e_IOC_FM_PORT_COUNTERS_DEQ_CONFIRM == (unsigned long)e_FM_PORT_COUNTERS_DEQ_CONFIRM);
    ASSERT_COND((unsigned long)e_IOC_FM_PORT_DUAL_RATE_LIMITER_SCALE_DOWN_BY_8 == (unsigned long)e_FM_PORT_DUAL_RATE_LIMITER_SCALE_DOWN_BY_8);
}

static t_Error LnxwrpFmPcdIOCTL(t_LnxWrpFmDev *p_LnxWrpFmDev, unsigned int cmd, unsigned long arg, bool compat)
{
    t_Error err = E_READ_FAILED;

    /* can be moved from here */
    LnxwrpAssertions();

    switch (cmd)
    {
#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_PRS_LOAD_SW_COMPAT:
#endif
        case FM_PCD_IOC_PRS_LOAD_SW:
        {
            ioc_fm_pcd_prs_sw_params_t *param;
            uint8_t                    *p_code;

            ASSERT_COND(sizeof(ioc_fm_pcd_prs_sw_params_t) == sizeof(t_FmPcdPrsSwParams));

            param = (ioc_fm_pcd_prs_sw_params_t *) XX_Malloc(sizeof(ioc_fm_pcd_prs_sw_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_prs_sw_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_prs_sw_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_prs_sw_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_prs_sw_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_prs_sw_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_fm_pcd_prs_sw(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_prs_sw_params_t *)arg,
                            sizeof(ioc_fm_pcd_prs_sw_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            p_code = (uint8_t *) XX_Malloc(param->size);
            if (!p_code)
            {
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            if (copy_from_user(p_code, param->p_code, param->size)) {
                XX_Free(p_code);
                XX_Free(param);
                RETURN_ERROR(MINOR, err, NO_MSG);
            }

            param->p_code = p_code;

            err = FM_PCD_PrsLoadSw(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdPrsSwParams*)param);
            XX_Free(p_code);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_ENABLE:
            return FM_PCD_Enable(p_LnxWrpFmDev->h_PcdDev);

        case FM_PCD_IOC_DISABLE:
            return FM_PCD_Disable(p_LnxWrpFmDev->h_PcdDev);

        case FM_PCD_IOC_FORCE_INTR:
        {
            int exception;

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (get_user(exception, (int *) compat_ptr(arg)))
                    break;
            }
            else
#endif
            {
               if (get_user(exception, (int *)arg))
                   break;
            }

            return FM_PCD_ForceIntr(p_LnxWrpFmDev->h_PcdDev, (e_FmPcdExceptions)exception);
        }

        case FM_PCD_IOC_SET_EXCEPTION:
        {
            ioc_fm_pcd_exception_params_t *param;

            param = (ioc_fm_pcd_exception_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_exception_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_pcd_exception_params_t *)compat_ptr(arg),
                                    sizeof(ioc_fm_pcd_exception_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_exception_params_t *)arg,
                                    sizeof(ioc_fm_pcd_exception_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev, param->exception, param->enable);
            XX_Free(param);
            break;
        }

        case FM_PCD_IOC_KG_SET_ADDITIONAL_DATA_AFTER_PARSING:
        {
            uint8_t payloadOffset;

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (get_user(payloadOffset, (uint8_t*) compat_ptr(arg)))
                    break;
            }
            else
#endif
            {
                if (get_user(payloadOffset, (uint8_t*) arg))
                    break;
            }

            return FM_PCD_KgSetAdditionalDataAfterParsing(p_LnxWrpFmDev->h_PcdDev, payloadOffset);
        }

        case FM_PCD_IOC_KG_SET_DFLT_VALUE:
        {
            ioc_fm_pcd_kg_dflt_value_params_t *param;

            param = (ioc_fm_pcd_kg_dflt_value_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_kg_dflt_value_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_pcd_kg_dflt_value_params_t *)compat_ptr(arg),
                                    sizeof(ioc_fm_pcd_kg_dflt_value_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_kg_dflt_value_params_t *)arg,
                                    sizeof(ioc_fm_pcd_kg_dflt_value_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_KgSetDfltValue(p_LnxWrpFmDev->h_PcdDev, param->valueId, param->value);
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_SET_NET_ENV_CHARACTERISTICS_COMPAT:
#endif
        case FM_PCD_IOC_SET_NET_ENV_CHARACTERISTICS:
        {
            ioc_fm_pcd_net_env_params_t  *param;

            param = (ioc_fm_pcd_net_env_params_t *) XX_Malloc(sizeof(ioc_fm_pcd_net_env_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_net_env_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_net_env_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_net_env_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_net_env_params_t *) compat_ptr(arg),
                                    sizeof(ioc_compat_fm_pcd_net_env_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_net_env(compat_param, param, COMPAT_US_TO_K);
                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_net_env_params_t *) arg,
                            sizeof(ioc_fm_pcd_net_env_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            param->id = FM_PCD_SetNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdNetEnvParams*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_net_env_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_net_env_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_net_env_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                compat_copy_fm_pcd_net_env(compat_param, param, COMPAT_K_TO_US);

                if (param->id && !copy_to_user((ioc_compat_fm_pcd_net_env_params_t *) compat_ptr(arg),
                            compat_param,
                            sizeof(ioc_compat_fm_pcd_net_env_params_t)))
                    err = E_OK;

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (param->id && !copy_to_user((ioc_fm_pcd_net_env_params_t *)arg, param, sizeof(ioc_fm_pcd_net_env_params_t)))
                    err = E_OK;
            }

            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_DELETE_NET_ENV_CHARACTERISTICS_COMPAT:
#endif
        case FM_PCD_IOC_DELETE_NET_ENV_CHARACTERISTICS:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PCD_DeleteNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_KG_SET_SCHEME_COMPAT:
#endif
        case FM_PCD_IOC_KG_SET_SCHEME:
        {
            ioc_fm_pcd_kg_scheme_params_t *param;

            ASSERT_COND(sizeof(t_FmPcdKgSchemeParams) + sizeof(void *) == sizeof(ioc_fm_pcd_kg_scheme_params_t));
            param = (ioc_fm_pcd_kg_scheme_params_t *) XX_Malloc(sizeof(ioc_fm_pcd_kg_scheme_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_kg_scheme_params_t *compat_param = NULL;

                compat_param = (ioc_compat_fm_pcd_kg_scheme_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_kg_scheme_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_kg_scheme_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_kg_scheme_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_kg_scheme(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_kg_scheme_params_t *)arg,
                            sizeof(ioc_fm_pcd_kg_scheme_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            param->id = FM_PCD_KgSetScheme(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdKgSchemeParams*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_kg_scheme_params_t *compat_param = NULL;

                compat_param = (ioc_compat_fm_pcd_kg_scheme_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_kg_scheme_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                compat_copy_fm_pcd_kg_scheme(compat_param, param, COMPAT_K_TO_US);

                if (param->id && !copy_to_user((ioc_compat_fm_pcd_kg_scheme_params_t *)compat_ptr(arg),
                            compat_param,
                            sizeof(ioc_compat_fm_pcd_kg_scheme_params_t)))
                    err = E_OK;
                XX_Free(compat_param);
            }
            else
#endif
            {
                if (param->id && !copy_to_user((ioc_fm_pcd_kg_scheme_params_t *)arg,
                            param,
                            sizeof(ioc_fm_pcd_kg_scheme_params_t)))
                    err = E_OK;
            }

            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_KG_DEL_SCHEME_COMPAT:
#endif
        case FM_PCD_IOC_KG_DEL_SCHEME:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PCD_KgDeleteScheme(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_SET_NODE_COMPAT:
#endif
        case FM_PCD_IOC_CC_SET_NODE:
        {
            ioc_fm_pcd_cc_node_params_t *param;
            uint8_t                     *keys;
            uint8_t                     *masks;
            int                         i,k;

            ASSERT_COND(sizeof(t_FmPcdCcNodeParams) + sizeof(void *) == sizeof(ioc_fm_pcd_cc_node_params_t));

            param = (ioc_fm_pcd_cc_node_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_params_t) +
                    2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            memset(param, 0, sizeof(ioc_fm_pcd_cc_node_params_t) +
                    2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);

            keys = (uint8_t *) (param + 1);
            masks = keys + IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY;
            memset(keys, 0, 2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_params_t) +
                        2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                memset(compat_param, 0, sizeof(ioc_compat_fm_pcd_cc_node_params_t) +
                        2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_cc_node_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_node_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_params_t *)arg, sizeof(ioc_fm_pcd_cc_node_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }
            ASSERT_COND(param->keys_params.num_of_keys <= IOC_FM_PCD_MAX_NUM_OF_KEYS);
            ASSERT_COND(param->keys_params.key_size <= IOC_FM_PCD_MAX_SIZE_OF_KEY);

            /* support for indexed lookup */
            if( !(param->extract_cc_params.type == e_IOC_FM_PCD_EXTRACT_NON_HDR &&
                  param->extract_cc_params.extract_params.extract_non_hdr.src == e_IOC_FM_PCD_EXTRACT_FROM_HASH &&
                  param->extract_cc_params.extract_params.extract_non_hdr.action == e_IOC_FM_PCD_ACTION_INDEXED_LOOKUP))
            {
                for (i=0, k=0;
                     i < param->keys_params.num_of_keys;
                     i++, k += IOC_FM_PCD_MAX_SIZE_OF_KEY)
                {
                    if (copy_from_user(&keys[k],
                                param->keys_params.key_params[i].p_key,
                                param->keys_params.key_size))
                    {
                        XX_Free(param);
                        RETURN_ERROR(MINOR, err, NO_MSG);
                    }

                    param->keys_params.key_params[i].p_key = &keys[k];

                    if (param->keys_params.key_params[i].p_mask)
                    {
                        if (copy_from_user(&masks[k],
                                    param->keys_params.key_params[i].p_mask,
                                    param->keys_params.key_size))
                        {
                            XX_Free(param);
                            RETURN_ERROR(MINOR, err, NO_MSG);
                        }
                        param->keys_params.key_params[i].p_mask = &masks[k];
                    }
                }
            }

            param->id = FM_PCD_CcSetNode(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdCcNodeParams*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_params_t *compat_param;
                compat_param = (ioc_compat_fm_pcd_cc_node_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_params_t) +
                        2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                memset(compat_param, 0, sizeof(ioc_compat_fm_pcd_cc_node_params_t) +
                        2 * IOC_FM_PCD_MAX_NUM_OF_KEYS * IOC_FM_PCD_MAX_SIZE_OF_KEY);

                /* setup user space structure */
                compat_copy_fm_pcd_cc_node(compat_param, param, COMPAT_K_TO_US);

                compat_param->id = compat_add_ptr2id(param->id);

                if (param->id && !copy_to_user((ioc_compat_fm_pcd_cc_node_params_t *)compat_ptr(arg),
                            compat_param,
                            sizeof(ioc_compat_fm_pcd_cc_node_params_t)))
                    err = E_OK;

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (param->id && !copy_to_user((ioc_fm_pcd_cc_node_params_t *)arg, param, sizeof(ioc_fm_pcd_cc_node_params_t)))
                    err = E_OK;
            }

            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_DELETE_NODE_COMPAT:
#endif
        case FM_PCD_IOC_CC_DELETE_NODE:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_get_id2ptr(compat_id.obj);
                compat_del_ptr2id(id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PCD_CcDeleteNode(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_BUILD_TREE_COMPAT:
#endif
        case FM_PCD_IOC_CC_BUILD_TREE:
        {
            ioc_fm_pcd_cc_tree_params_t *param;

            ASSERT_COND(sizeof(t_FmPcdCcTreeParams) + sizeof(void *) == sizeof(ioc_fm_pcd_cc_tree_params_t));

            param = (ioc_fm_pcd_cc_tree_params_t *) XX_Malloc(sizeof(ioc_fm_pcd_cc_tree_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            memset(param, 0, sizeof(ioc_fm_pcd_cc_tree_params_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_tree_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_tree_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_tree_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }
                memset(compat_param, 0, sizeof(ioc_compat_fm_pcd_cc_tree_params_t));

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_cc_tree_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_tree_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_tree(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_tree_params_t *)arg,
                            sizeof(ioc_fm_pcd_cc_tree_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            param->id = FM_PCD_CcBuildTree(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdCcTreeParams*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_tree_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_tree_params_t *) XX_Malloc(sizeof(ioc_compat_fm_pcd_cc_tree_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                memset(compat_param, 0, sizeof(ioc_compat_fm_pcd_cc_tree_params_t));

                compat_add_ptr2id(param->id);
                param->id = (void *)(uint64_t)compat_get_ptr2id(param->id);

                compat_copy_fm_pcd_cc_tree(compat_param, param, COMPAT_K_TO_US);

                if (param->id && !copy_to_user((ioc_compat_fm_pcd_cc_tree_params_t *)compat_ptr(arg),
                                                compat_param,
                                                sizeof(ioc_compat_fm_pcd_cc_tree_params_t)))
                    err = E_OK;

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (param->id && !copy_to_user((ioc_fm_pcd_cc_tree_params_t *)arg, param, sizeof(ioc_fm_pcd_cc_tree_params_t)))
                    err = E_OK;
            }

            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_DELETE_TREE_COMPAT:
#endif
        case FM_PCD_IOC_CC_DELETE_TREE:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_get_id2ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PCD_CcDeleteTree(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_PLCR_SET_PROFILE_COMPAT:
#endif
        case FM_PCD_IOC_PLCR_SET_PROFILE:
        {
            ioc_fm_pcd_plcr_profile_params_t *param;

            ASSERT_COND(sizeof(t_FmPcdPlcrProfileParams) + sizeof(void *) == sizeof(ioc_fm_pcd_plcr_profile_params_t));

            param = (ioc_fm_pcd_plcr_profile_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_plcr_profile_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_plcr_profile_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_plcr_profile_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_plcr_profile_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_plcr_profile_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_plcr_profile_params_t))) {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_plcr_profile(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_plcr_profile_params_t *)arg,
                                    sizeof(ioc_fm_pcd_plcr_profile_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            if (!param->modify &&
                (((t_FmPcdPlcrProfileParams*)param)->id.newParams.profileType != e_FM_PCD_PLCR_SHARED))
            {
                t_Handle h_Port;
                fm_pcd_port_params_t *port_params;

                port_params = (fm_pcd_port_params_t*) XX_Malloc(sizeof(fm_pcd_port_params_t)); 
                if (!port_params)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(port_params, (fm_pcd_port_params_t*)((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort,
                            sizeof(fm_pcd_port_params_t)))
                {
                    XX_Free(port_params);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                switch(port_params->port_type)
                {
                    case (e_IOC_FM_PORT_TYPE_RX):
                        h_Port = p_LnxWrpFmDev->rxPorts[port_params->port_id].h_Dev;
                        break;
                    case (e_IOC_FM_PORT_TYPE_RX_10G):
                        h_Port = p_LnxWrpFmDev->rxPorts[port_params->port_id + FM_MAX_NUM_OF_1G_RX_PORTS].h_Dev;
                        break;
                    case (e_IOC_FM_PORT_TYPE_OFFLINE_PARSING):
                        if (port_params->port_id)
                        {
                            h_Port = p_LnxWrpFmDev->opPorts[port_params->port_id - 1].h_Dev;
                            break;
                        }
                    default:
                        XX_Free(port_params);
                        XX_Free(param);
                        RETURN_ERROR(MINOR, E_INVALID_SELECTION, NO_MSG);
                }

                ((t_FmPcdPlcrProfileParams*)param)->id.newParams.h_FmPort = h_Port;
                XX_Free(port_params);
            }

            param->id = FM_PCD_PlcrSetProfile(p_LnxWrpFmDev->h_PcdDev, (t_FmPcdPlcrProfileParams*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_plcr_profile_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_plcr_profile_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_plcr_profile_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                compat_copy_fm_pcd_plcr_profile(compat_param, param, COMPAT_K_TO_US);

                if (param->id && !copy_to_user((ioc_compat_fm_pcd_plcr_profile_params_t *) compat_ptr(arg),
                            compat_param,
                            sizeof(ioc_compat_fm_pcd_plcr_profile_params_t)))
                    err = E_OK;

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (param->id && !copy_to_user((ioc_fm_pcd_plcr_profile_params_t *)arg, param, sizeof(ioc_fm_pcd_plcr_profile_params_t)))
                    err = E_OK;
            }

            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_PLCR_DEL_PROFILE_COMPAT:
#endif
        case FM_PCD_IOC_PLCR_DEL_PROFILE:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PCD_PlcrDeleteProfile(p_LnxWrpFmDev->h_PcdDev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_TREE_MODIFY_NEXT_ENGINE_COMPAT:
#endif
        case FM_PCD_IOC_CC_TREE_MODIFY_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_tree_modify_next_engine_params_t *param;

            ASSERT_COND(sizeof(ioc_fm_pcd_cc_next_engine_params_t) == sizeof(t_FmPcdCcNextEngineParams));

            param = (ioc_fm_pcd_cc_tree_modify_next_engine_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_tree_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_fm_pcd_cc_tree_modify_next_engine(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_tree_modify_next_engine_params_t *)arg,
                            sizeof(ioc_fm_pcd_cc_tree_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcTreeModifyNextEngine(p_LnxWrpFmDev->h_PcdDev,
                    param->id,
                    param->grp_indx,
                    param->indx,
                    (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_MODIFY_NEXT_ENGINE_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_MODIFY_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_next_engine_params_t *param;

            ASSERT_COND(sizeof(ioc_fm_pcd_cc_next_engine_params_t) == sizeof(t_FmPcdCcNextEngineParams));

            param = (ioc_fm_pcd_cc_node_modify_next_engine_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node_modify_next_engine(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_next_engine_params_t *)arg,
                            sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcNodeModifyNextEngine(p_LnxWrpFmDev->h_PcdDev,
                    param->id,
                    param->key_indx,
                    (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_MODIFY_MISS_NEXT_ENGINE_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_MODIFY_MISS_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_next_engine_params_t *param;

            ASSERT_COND(sizeof(ioc_fm_pcd_cc_next_engine_params_t) == sizeof(t_FmPcdCcNextEngineParams));

            param = (ioc_fm_pcd_cc_node_modify_next_engine_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *) compat_ptr(arg),
                                    sizeof(ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node_modify_next_engine(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_next_engine_params_t *) arg,
                                    sizeof(ioc_fm_pcd_cc_node_modify_next_engine_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcNodeModifyMissNextEngine(p_LnxWrpFmDev->h_PcdDev, param->id,
                    (t_FmPcdCcNextEngineParams*)(&param->cc_next_engine_params));
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_REMOVE_KEY_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_REMOVE_KEY:
        {
            ioc_fm_pcd_cc_node_remove_key_params_t *param;

            param = (ioc_fm_pcd_cc_node_remove_key_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_remove_key_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_remove_key_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_remove_key_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_remove_key_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_cc_node_remove_key_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_node_remove_key_params_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                param->id = compat_ptr(compat_param->id);
                param->key_indx = compat_param->key_indx;

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_remove_key_params_t *) arg,
                            sizeof(ioc_fm_pcd_cc_node_remove_key_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcNodeRemoveKey(p_LnxWrpFmDev->h_PcdDev, param->id, param->key_indx);
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_ADD_KEY_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_ADD_KEY:
        {
            ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *param;

            ASSERT_COND(sizeof(ioc_fm_pcd_cc_key_params_t) == sizeof(t_FmPcdCcKeyParams));

            param = (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
                if (!compat_param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node_modify_key_and_next_engine(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)arg,
                                    sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcNodeAddKey(p_LnxWrpFmDev->h_PcdDev,
                    param->id,
                    param->key_indx,
                    param->key_size,
                    (t_FmPcdCcKeyParams*)(&param->key_params));
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_MODIFY_KEY_AND_NEXT_ENGINE_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_MODIFY_KEY_AND_NEXT_ENGINE:
        {
            ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *param;

            ASSERT_COND(sizeof(ioc_fm_pcd_cc_key_params_t) == sizeof(t_FmPcdCcKeyParams));

            param = (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t));
                if (!compat_param)
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param,
                            (ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node_modify_key_and_next_engine(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *)arg,
                            sizeof(ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            err = FM_PCD_CcNodeModifyKeyAndNextEngine(p_LnxWrpFmDev->h_PcdDev,
                    param->id,
                    param->key_indx,
                    param->key_size,
                    (t_FmPcdCcKeyParams*)(&param->key_params));
            XX_Free(param);
            break;
        }

#if defined(CONFIG_COMPAT)
        case FM_PCD_IOC_CC_NODE_MODIFY_KEY_COMPAT:
#endif
        case FM_PCD_IOC_CC_NODE_MODIFY_KEY:
        {
            ioc_fm_pcd_cc_node_modify_key_params_t  *param = NULL;
            uint8_t                                 *key = NULL;
            uint8_t                                 *mask = NULL;

            param = (ioc_fm_pcd_cc_node_modify_key_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_cc_node_modify_key_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_cc_node_modify_key_params_t  *compat_param = NULL;
                compat_param = (ioc_compat_fm_pcd_cc_node_modify_key_params_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_cc_node_modify_key_params_t));
                if (!param)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));
                }

                if (copy_from_user(compat_param, (ioc_compat_fm_pcd_cc_node_modify_key_params_t *)compat_ptr(arg),
                                    sizeof(ioc_compat_fm_pcd_cc_node_modify_key_params_t)))
                {
                    XX_Free(compat_param);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }

                compat_copy_fm_pcd_cc_node_modify_key(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_cc_node_modify_key_params_t *)arg,
                                    sizeof(ioc_fm_pcd_cc_node_modify_key_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
            }

            if (param->p_key)
            {
                key = (uint8_t *) XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!key)
                {
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD key"));
                }
                memset(key, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);

                if (copy_from_user(key, param->p_key, param->key_size))
                {
                    XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->p_key = key;
            }

            if (param->p_mask)
            {
                mask = (uint8_t *) XX_Malloc(sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);
                if (!mask)
                {
                    if (key)
                        XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD mask"));
                }
                memset(mask, 0, sizeof(uint8_t)*IOC_FM_PCD_MAX_SIZE_OF_KEY);

                if (copy_from_user(mask, param->p_mask, param->key_size))
                {
                    if (mask)
                        XX_Free(mask);
                    if (key)
                        XX_Free(key);
                    XX_Free(param);
                    RETURN_ERROR(MINOR, err, NO_MSG);
                }
                param->p_mask = mask;
            }

            err = FM_PCD_CcNodeModifyKey(p_LnxWrpFmDev->h_PcdDev,
                    param->id,
                    param->key_indx,
                    param->key_size,
                    param->p_key,
                    param->p_mask);
            if (mask)
                XX_Free(mask);
            if (key)
                XX_Free(key);
            XX_Free(param);
            break;
        }

        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("IOCTL cmd (0x%08x):(0x%02x:0x%02x)!", cmd, _IOC_TYPE(cmd), _IOC_NR(cmd)));
            break;
    }

    return err;
}

t_Error LnxwrpFmIOCTL(t_LnxWrpFmDev *p_LnxWrpFmDev, unsigned int cmd, unsigned long arg, bool compat)
{
    t_Error err = E_READ_FAILED;

    DBG(TRACE, ("p_LnxWrpFmDev - 0x%08lx, cmd - 0x%08x, arg - 0x%08lx \n", (uintptr_t)p_LnxWrpFmDev, cmd, arg));

    switch (cmd)
    {
        case FM_IOC_SET_PORTS_BANDWIDTH:
        {
            ioc_fm_port_bandwidth_params *param;

            ASSERT_COND(sizeof(t_FmPortsBandwidthParams) == sizeof(ioc_fm_port_bandwidth_params));

            param = (ioc_fm_port_bandwidth_params*) XX_Malloc(sizeof(ioc_fm_port_bandwidth_params));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_port_bandwidth_params*)compat_ptr(arg), sizeof(ioc_fm_port_bandwidth_params)))
                {
                    XX_Free(param);
                    return err;
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_port_bandwidth_params*)arg, sizeof(ioc_fm_port_bandwidth_params)))
                {
                    XX_Free(param);
                    return err;
                }
            }

            err =  FM_SetPortsBandwidth(p_LnxWrpFmDev->h_Dev, (t_FmPortsBandwidthParams*) param);
            XX_Free(param);
            return err;
        }

        case FM_IOC_GET_REVISION:
        {
            ioc_fm_revision_info_t *param;

            ASSERT_COND(sizeof(t_FmRevisionInfo) == sizeof(ioc_fm_revision_info_t));

            param = (ioc_fm_revision_info_t *) XX_Malloc(sizeof(ioc_fm_revision_info_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

            FM_GetRevision(p_LnxWrpFmDev->h_Dev, (t_FmRevisionInfo*)param);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_to_user((ioc_fm_revision_info_t *)compat_ptr(arg),
                            param,
                            sizeof(ioc_fm_revision_info_t)))
                    err = E_WRITE_FAILED;
                else
                    err = E_OK;
            }
            else
#endif
            {
                if (copy_to_user((ioc_fm_revision_info_t *)arg,
                            param,
                            sizeof(ioc_fm_revision_info_t)))
                    err = E_WRITE_FAILED;
                else
                    err = E_OK;
            }

            XX_Free(param);
            return err;
        }

        case FM_IOC_SET_COUNTER:
        {
            ioc_fm_counters_params_t *param;

            param = (ioc_fm_counters_params_t *) XX_Malloc(sizeof(ioc_fm_counters_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_counters_params_t *)compat_ptr(arg), sizeof(ioc_fm_counters_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_counters_params_t *)arg, sizeof(ioc_fm_counters_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }

            err = FM_ModifyCounter(p_LnxWrpFmDev->h_Dev, param->cnt, param->val);

            XX_Free(param);
            return err;
        }

        case FM_IOC_GET_COUNTER:
        {
            ioc_fm_counters_params_t *param;

            param = (ioc_fm_counters_params_t *) XX_Malloc(sizeof(ioc_fm_counters_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PCD"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_counters_params_t *)compat_ptr(arg), sizeof(ioc_fm_counters_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_counters_params_t *)arg, sizeof(ioc_fm_counters_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }

            param->val = FM_GetCounter(p_LnxWrpFmDev->h_Dev, param->cnt);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_to_user((ioc_fm_counters_params_t *)compat_ptr(arg), param, sizeof(ioc_fm_counters_params_t)))
                    err = E_WRITE_FAILED;
            }
            else
#endif
            {
                if (copy_to_user((ioc_fm_counters_params_t *)arg, param, sizeof(ioc_fm_counters_params_t)))
                    err = E_WRITE_FAILED;
            }

            XX_Free(param);
            return err;
        }

        case FM_IOC_FORCE_INTR:
        {
            ioc_fm_exceptions param;

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (get_user(param, (ioc_fm_exceptions*) compat_ptr(arg)))
                    break;
            }
            else
#endif
            {
                if (get_user(param, (ioc_fm_exceptions*)arg))
                    break;
            }

            return FM_ForceIntr(p_LnxWrpFmDev->h_Dev, (e_FmExceptions)param);
        }

        default:
            return LnxwrpFmPcdIOCTL(p_LnxWrpFmDev, cmd, arg, compat);
    }

    RETURN_ERROR(MINOR, E_INVALID_OPERATION, ("IOCTL FM"));
}

t_Error LnxwrpFmPortIOCTL(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev, unsigned int cmd, unsigned long arg, bool compat)
{
    t_Error err = E_READ_FAILED;
    DBG(TRACE, ("p_LnxWrpFmPortDev - 0x%08lx, cmd - 0x%08x, arg - 0x%08lx", (uintptr_t)p_LnxWrpFmPortDev, cmd, arg));

    switch (cmd)
    {
        case FM_PORT_IOC_DISABLE:
            FM_PORT_Disable(p_LnxWrpFmPortDev->h_Dev);
            return E_OK;

        case FM_PORT_IOC_ENABLE:
            FM_PORT_Enable(p_LnxWrpFmPortDev->h_Dev);
            return E_OK;

        case FM_PORT_IOC_SET_ERRORS_ROUTE:
        {
            ioc_fm_port_frame_err_select_t errs;

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (get_user(errs, (ioc_fm_port_frame_err_select_t*)compat_ptr(arg)))
                    break;
            }
            else
#endif
            {
                if (get_user(errs, (ioc_fm_port_frame_err_select_t*)arg))
                    break;
            }

            return FM_PORT_SetErrorsRoute(p_LnxWrpFmPortDev->h_Dev, (fmPortFrameErrSelect_t)errs);
        }

        case FM_PORT_IOC_SET_RATE_LIMIT:
        {
            ioc_fm_port_rate_limit_t *param;

            ASSERT_COND(sizeof(t_FmPortRateLimit) == sizeof(ioc_fm_port_rate_limit_t));

            param = (ioc_fm_port_rate_limit_t *) XX_Malloc(sizeof(ioc_fm_port_rate_limit_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_port_rate_limit_t *)compat_ptr(arg), sizeof(ioc_fm_port_rate_limit_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_READ_FAILED, NO_MSG);
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_port_rate_limit_t *)arg, sizeof(ioc_fm_port_rate_limit_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_READ_FAILED, NO_MSG);
                }
            }

            err =  FM_PORT_SetRateLimit(p_LnxWrpFmPortDev->h_Dev, (t_FmPortRateLimit *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_REMOVE_RATE_LIMIT:
            FM_PORT_DeleteRateLimit(p_LnxWrpFmPortDev->h_Dev);
            return E_OK;
        
        case FM_PORT_IOC_ALLOC_PCD_FQIDS:
        {
            ioc_fm_port_pcd_fqids_params_t *param;

            if (!p_LnxWrpFmPortDev->pcd_owner_params.cba)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("No one to listen on this PCD!!!"));

            param = (ioc_fm_port_pcd_fqids_params_t *) XX_Malloc(sizeof(ioc_fm_port_pcd_fqids_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_port_pcd_fqids_params_t *)compat_ptr(arg),
                                    sizeof(ioc_fm_port_pcd_fqids_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_port_pcd_fqids_params_t *)arg,
                                    sizeof(ioc_fm_port_pcd_fqids_params_t)))
                {
                    XX_Free(param);
                    return err;
                }
            }

            if (p_LnxWrpFmPortDev->pcd_owner_params.cba(p_LnxWrpFmPortDev->pcd_owner_params.dev,
                                                        param->num_fqids,
                                                        param->alignment,
                                                        &param->base_fqid))
            {
                XX_Free(param);
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("can't allocate fqids for PCD!!!"));
            }

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_to_user((ioc_fm_port_pcd_fqids_params_t *)compat_ptr(arg),
                                  param, sizeof(ioc_fm_port_pcd_fqids_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }
            else
#endif
            {
                if (copy_to_user((ioc_fm_port_pcd_fqids_params_t *)arg,
                                  param, sizeof(ioc_fm_port_pcd_fqids_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }

            XX_Free(param);
            return E_OK;
        }

        case FM_PORT_IOC_FREE_PCD_FQIDS:
        {
            uint32_t base_fqid;

            if (!p_LnxWrpFmPortDev->pcd_owner_params.cbf)
                RETURN_ERROR(MINOR, E_INVALID_STATE, ("No one to listen on this PCD!!!"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (get_user(base_fqid, (uint32_t*) compat_ptr(arg)))
                    break;
            }
            else
#endif
            {
                if (get_user(base_fqid, (uint32_t*)arg))
                    break;
            }

            if (p_LnxWrpFmPortDev->pcd_owner_params.cbf(p_LnxWrpFmPortDev->pcd_owner_params.dev, base_fqid))
                RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);

            return E_OK;
        }

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_SET_PCD_COMPAT:
#endif
        case FM_PORT_IOC_SET_PCD:
        {
            ioc_fm_port_pcd_params_t      *port_pcd_params;
            ioc_fm_port_pcd_prs_params_t  *port_pcd_prs_params;
            ioc_fm_port_pcd_cc_params_t   *port_pcd_cc_params;
            ioc_fm_port_pcd_kg_params_t   *port_pcd_kg_params;
            ioc_fm_port_pcd_plcr_params_t *port_pcd_plcr_params;

            long copy_fail = 0;

            ASSERT_COND(sizeof(t_FmPortPcdParams) == sizeof(ioc_fm_port_pcd_params_t));

            port_pcd_params = (ioc_fm_port_pcd_params_t *) XX_Malloc(
                    sizeof(ioc_fm_port_pcd_params_t) +
                    sizeof(ioc_fm_port_pcd_prs_params_t) +
                    sizeof(ioc_fm_port_pcd_cc_params_t) +
                    sizeof(ioc_fm_port_pcd_kg_params_t) +
                    sizeof(ioc_fm_port_pcd_plcr_params_t));
            if (!port_pcd_params)
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            port_pcd_prs_params  = (ioc_fm_port_pcd_prs_params_t *)  (port_pcd_params + 1);
            port_pcd_cc_params   = (ioc_fm_port_pcd_cc_params_t *)   (port_pcd_prs_params + 1);
            port_pcd_kg_params   = (ioc_fm_port_pcd_kg_params_t *)   (port_pcd_cc_params + 1);
            port_pcd_plcr_params = (ioc_fm_port_pcd_plcr_params_t *) (port_pcd_kg_params + 1);

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_port_pcd_params_t      *compat_port_pcd_params;
                ioc_fm_port_pcd_prs_params_t         *same_port_pcd_prs_params;
                ioc_compat_fm_port_pcd_cc_params_t   *compat_port_pcd_cc_params;
                ioc_compat_fm_port_pcd_kg_params_t   *compat_port_pcd_kg_params;
                ioc_compat_fm_port_pcd_plcr_params_t *compat_port_pcd_plcr_params;

                compat_port_pcd_params = (ioc_compat_fm_port_pcd_params_t *) XX_Malloc(
                                sizeof(ioc_compat_fm_port_pcd_params_t) +
                                sizeof(ioc_fm_port_pcd_prs_params_t) +
                                sizeof(ioc_compat_fm_port_pcd_cc_params_t) +
                                sizeof(ioc_compat_fm_port_pcd_kg_params_t) +
                                sizeof(ioc_compat_fm_port_pcd_plcr_params_t));
                if (!compat_port_pcd_params)
                {
                    XX_Free(port_pcd_params);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                }

                same_port_pcd_prs_params    = (ioc_fm_port_pcd_prs_params_t *) (compat_port_pcd_params + 1);
                compat_port_pcd_cc_params   = (ioc_compat_fm_port_pcd_cc_params_t *) (same_port_pcd_prs_params + 1);
                compat_port_pcd_kg_params   = (ioc_compat_fm_port_pcd_kg_params_t *) (compat_port_pcd_cc_params + 1);
                compat_port_pcd_plcr_params = (ioc_compat_fm_port_pcd_plcr_params_t *) (compat_port_pcd_kg_params + 1);

                /* Pseudo-while */
                while (!(copy_fail = copy_from_user(compat_port_pcd_params,
                                        (ioc_compat_fm_port_pcd_params_t *)compat_ptr(arg),
                                        sizeof(ioc_compat_fm_port_pcd_params_t))))
                {
                    compat_copy_fm_port_pcd(compat_port_pcd_params, port_pcd_params, COMPAT_US_TO_K);

                    /* the prs member is the same, no compat structure...memcpy only */
                    if (port_pcd_params->p_prs_params && !copy_fail)
                    {
                        if(!(copy_fail = copy_from_user(same_port_pcd_prs_params,
                                port_pcd_params->p_prs_params,
                                sizeof(ioc_fm_port_pcd_prs_params_t))))
                        {
                            memcpy(port_pcd_prs_params, same_port_pcd_prs_params, sizeof(ioc_fm_port_pcd_prs_params_t));
                            port_pcd_params->p_prs_params = port_pcd_prs_params;
                        }
                        else
                            break;
                    }

                    if (port_pcd_params->p_cc_params && !copy_fail)
                    {
                        if(!(copy_fail = copy_from_user(compat_port_pcd_cc_params,
                                port_pcd_params->p_cc_params,
                                sizeof(ioc_compat_fm_port_pcd_cc_params_t))))
                        {
                            port_pcd_params->p_cc_params = port_pcd_cc_params;
                            port_pcd_params->p_cc_params->cc_tree_id = compat_get_id2ptr(compat_port_pcd_cc_params->cc_tree_id);
                        }
                        else
                            break;
                    }

                    if (port_pcd_params->p_kg_params && !copy_fail)
                    {
                        if(!(copy_fail = copy_from_user(compat_port_pcd_kg_params,
                                port_pcd_params->p_kg_params,
                                sizeof(ioc_compat_fm_port_pcd_kg_params_t))))
                        {
                            compat_copy_fm_port_pcd_kg(compat_port_pcd_kg_params, port_pcd_kg_params, COMPAT_US_TO_K);
                            port_pcd_params->p_kg_params = port_pcd_kg_params;
                        }
                        else
                            break;
                    }

                    if (port_pcd_params->p_plcr_params && !copy_fail)
                    {
                        if(!(copy_fail = copy_from_user(compat_port_pcd_plcr_params,
                                port_pcd_params->p_plcr_params,
                                sizeof(ioc_compat_fm_port_pcd_plcr_params_t))))
                        {
                            port_pcd_params->p_plcr_params = port_pcd_plcr_params;
                            port_pcd_params->p_plcr_params->plcr_profile_id = compat_ptr(compat_port_pcd_plcr_params->plcr_profile_id);
                        }
                    }

                    /* always run once! */
                    break;
                }

                XX_Free(compat_port_pcd_params);
            }
            else
#endif
            {
                /* Pseudo-while */
                while (!(copy_fail = copy_from_user(port_pcd_params,
                                        (ioc_fm_port_pcd_params_t *)arg,
                                        sizeof(ioc_fm_port_pcd_params_t))))
                {
                    if (port_pcd_params->p_prs_params && !copy_fail)
                    {
                        if (!(copy_fail = copy_from_user(port_pcd_prs_params,
                                port_pcd_params->p_prs_params,
                                sizeof(ioc_fm_port_pcd_prs_params_t))))
                            port_pcd_params->p_prs_params = port_pcd_prs_params;
                        else
                            break;
                    }

                    if (port_pcd_params->p_cc_params &&  !copy_fail)
                    {
                        if (!(copy_fail = copy_from_user(port_pcd_cc_params,
                                port_pcd_params->p_cc_params,
                                sizeof(ioc_fm_port_pcd_cc_params_t))))
                            port_pcd_params->p_cc_params = port_pcd_cc_params;
                        else
                            break;
                    }

                    if (port_pcd_params->p_kg_params && !copy_fail)
                    {
                        if (!(copy_fail = copy_from_user(port_pcd_kg_params,
                                port_pcd_params->p_kg_params,
                                sizeof(ioc_fm_port_pcd_kg_params_t))))
                            port_pcd_params->p_kg_params = port_pcd_kg_params;
                        else
                            break;
                    }

                    if (port_pcd_params->p_plcr_params && !copy_fail)
                    {
                        if (!(copy_fail = copy_from_user(port_pcd_plcr_params,
                                port_pcd_params->p_plcr_params,
                                sizeof(ioc_fm_port_pcd_plcr_params_t))))
                            port_pcd_params->p_plcr_params = port_pcd_plcr_params;
                    }

                    /* always run once! */
                    break;
                }
            }

            if (!copy_fail)
                err = FM_PORT_SetPCD(p_LnxWrpFmPortDev->h_Dev, (t_FmPortPcdParams*) port_pcd_params);
            else
                err = E_READ_FAILED;

            XX_Free(port_pcd_params);

            return err;
        }

        case FM_PORT_IOC_DELETE_PCD:
            return FM_PORT_DeletePCD(p_LnxWrpFmPortDev->h_Dev);

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_PCD_KG_MODIFY_INITIAL_SCHEME_COMPAT:
#endif
        case FM_PORT_IOC_PCD_KG_MODIFY_INITIAL_SCHEME:
        {
            ioc_fm_pcd_kg_scheme_select_t *param;

            ASSERT_COND(sizeof(t_FmPcdKgSchemeSelect) == sizeof(ioc_fm_pcd_kg_scheme_select_t));

            param = (ioc_fm_pcd_kg_scheme_select_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_kg_scheme_select_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_kg_scheme_select_t *compat_param;

                compat_param = (ioc_compat_fm_pcd_kg_scheme_select_t *) XX_Malloc(
                        sizeof(ioc_compat_fm_pcd_kg_scheme_select_t));
                if (!compat_param){
                    XX_Free(param);
                    RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));
                }

                if (copy_from_user(compat_param,
                                   (ioc_compat_fm_pcd_kg_scheme_select_t *) compat_ptr(arg),
                                   sizeof(ioc_compat_fm_pcd_kg_scheme_select_t)))
                {
                    XX_Free(param);
                    XX_Free(compat_param);
                    RETURN_ERROR(MAJOR, E_READ_FAILED, NO_MSG);
                }

                compat_copy_fm_pcd_kg_scheme_select(compat_param, param, COMPAT_US_TO_K);

                XX_Free(compat_param);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_kg_scheme_select_t *)arg,
                                   sizeof(ioc_fm_pcd_kg_scheme_select_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_READ_FAILED, NO_MSG);
                }
            }

            err =  FM_PORT_PcdKgModifyInitialScheme(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdKgSchemeSelect *)param);

            XX_Free(param);
            return err;
        }

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_PCD_PLCR_MODIFY_INITIAL_PROFILE_COMPAT:
#endif
        case FM_PORT_IOC_PCD_PLCR_MODIFY_INITIAL_PROFILE:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));
#if defined(CONFIG_COMPAT)
            if (compat) {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PORT_PcdPlcrModifyInitialProfile(p_LnxWrpFmPortDev->h_Dev, id.obj);
        }

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_PCD_KG_BIND_SCHEMES_COMPAT:
#endif
        case FM_PORT_IOC_PCD_KG_BIND_SCHEMES:
        {
            ioc_fm_pcd_port_schemes_params_t *param;

            ASSERT_COND(sizeof(t_FmPcdPortSchemesParams) == sizeof(ioc_fm_pcd_port_schemes_params_t));

            param = (ioc_fm_pcd_port_schemes_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_port_schemes_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            memset(&param, 0 , sizeof(ioc_fm_pcd_port_schemes_params_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_port_schemes_params_t compat_param;

                if (copy_from_user(&compat_param,
                            (ioc_compat_fm_pcd_port_schemes_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_port_schemes_params_t)))
                    break;

                compat_copy_fm_pcd_kg_schemes_params(&compat_param, param, COMPAT_US_TO_K);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_port_schemes_params_t *) arg,
                            sizeof(ioc_fm_pcd_port_schemes_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }

            err = FM_PORT_PcdKgBindSchemes(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPortSchemesParams *)param);

            XX_Free(param);
            return err;
        }

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_PCD_KG_UNBIND_SCHEMES_COMPAT:
#endif
        case FM_PORT_IOC_PCD_KG_UNBIND_SCHEMES:
        {
            ioc_fm_pcd_port_schemes_params_t *param;

            ASSERT_COND(sizeof(t_FmPcdPortSchemesParams) == sizeof(ioc_fm_pcd_port_schemes_params_t));

            param = (ioc_fm_pcd_port_schemes_params_t *) XX_Malloc(
                    sizeof(ioc_fm_pcd_port_schemes_params_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

            memset(&param, 0 , sizeof(ioc_fm_pcd_port_schemes_params_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_pcd_port_schemes_params_t compat_param;

                if (copy_from_user(&compat_param,
                            (ioc_compat_fm_pcd_port_schemes_params_t *) compat_ptr(arg),
                            sizeof(ioc_compat_fm_pcd_port_schemes_params_t)))
                    break;

                compat_copy_fm_pcd_kg_schemes_params(&compat_param, param, COMPAT_US_TO_K);
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_port_schemes_params_t *) arg,
                        sizeof(ioc_fm_pcd_port_schemes_params_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }

            err =  FM_PORT_PcdKgUnbindSchemes(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPortSchemesParams *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_PRS_MODIFY_START_OFFSET:
        {
            ioc_fm_pcd_prs_start_t *param;

            ASSERT_COND(sizeof(t_FmPcdPrsStart) == sizeof(ioc_fm_pcd_prs_start_t));

            param = (ioc_fm_pcd_prs_start_t *) XX_Malloc(sizeof(ioc_fm_pcd_prs_start_t));
            if (!param)
                RETURN_ERROR(MINOR, E_NO_MEMORY, ("IOCTL FM PORT"));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                if (copy_from_user(param, (ioc_fm_pcd_prs_start_t *)compat_ptr(arg),
                                   sizeof(ioc_fm_pcd_prs_start_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }
            else
#endif
            {
                if (copy_from_user(param, (ioc_fm_pcd_prs_start_t *)arg,
                                   sizeof(ioc_fm_pcd_prs_start_t)))
                {
                    XX_Free(param);
                    RETURN_ERROR(MAJOR, E_WRITE_FAILED, NO_MSG);
                }
            }
            err = FM_PORT_PcdPrsModifyStartOffset(p_LnxWrpFmPortDev->h_Dev, (t_FmPcdPrsStart *)param);

            XX_Free(param);
            return err;
        }

        case FM_PORT_IOC_PCD_PLCR_ALLOC_PROFILES:
        {
            uint16_t num;
            if (get_user(num, (uint16_t*) arg))
                break;
            return FM_PORT_PcdPlcrAllocProfiles(p_LnxWrpFmPortDev->h_Dev, num);
        }

        case FM_PORT_IOC_PCD_PLCR_FREE_PROFILES:
            return FM_PORT_PcdPlcrFreeProfiles(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_DETACH_PCD:
            return FM_PORT_DetachPCD(p_LnxWrpFmPortDev->h_Dev);

        case FM_PORT_IOC_ATTACH_PCD:
            return FM_PORT_AttachPCD(p_LnxWrpFmPortDev->h_Dev);

#if defined(CONFIG_COMPAT)
        case FM_PORT_IOC_PCD_CC_MODIFY_TREE_COMPAT:
#endif
        case FM_PORT_IOC_PCD_CC_MODIFY_TREE:
        {
            ioc_fm_obj_t id;

            memset(&id, 0 , sizeof(ioc_fm_obj_t));

#if defined(CONFIG_COMPAT)
            if (compat)
            {
                ioc_compat_fm_obj_t compat_id;

                if (copy_from_user(&compat_id, (ioc_compat_fm_obj_t *) compat_ptr(arg), sizeof(ioc_compat_fm_obj_t)))
                    break;

                id.obj = compat_get_id2ptr(compat_id.obj);
            }
            else
#endif
            {
                if (copy_from_user(&id, (ioc_fm_obj_t *) arg, sizeof(ioc_fm_obj_t)))
                    break;
            }

            return FM_PORT_PcdCcModifyTree(p_LnxWrpFmPortDev->h_Dev, id.obj);
        }

        default:
            RETURN_ERROR(MINOR, E_INVALID_SELECTION, ("IOCTL cmd (0x%08x):(0x%02x:0x%02x)!", cmd, _IOC_TYPE(cmd), _IOC_NR(cmd)));
    }

    RETURN_ERROR(MINOR, E_INVALID_OPERATION, ("IOCTL port"));
}

/*****************************************************************************/
/*               API routines for the FM Linux Device                        */
/*****************************************************************************/

static int fm_open(struct inode *inode, struct file *file)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = NULL;
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = NULL;
    unsigned int        major = imajor(inode);
    unsigned int        minor = iminor(inode);
    struct device_node  *fm_node;
    static struct of_device_id fm_node_of_match[] = {
        { .compatible = "fsl,fman", },
        { /* end of list */ },
    };

    DBG(TRACE, ("Opening minor - %d - ", minor));

    if (file->private_data != NULL)
        return 0;

    /* Get all the FM nodes */
    for_each_matching_node(fm_node, fm_node_of_match) {
        struct platform_device    *of_dev;

        of_dev = of_find_device_by_node(fm_node);
        if (unlikely(of_dev == NULL)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
            return -ENXIO;
        }

        p_LnxWrpFmDev = (t_LnxWrpFmDev *)fm_bind(&of_dev->dev);
        if (p_LnxWrpFmDev->major == major)
            break;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
        p_LnxWrpFmDev = NULL;
    }

    if (!p_LnxWrpFmDev)
        return -ENODEV;

    if (minor == DEV_FM_MINOR_BASE)
        file->private_data = p_LnxWrpFmDev;
    else if (minor == DEV_FM_PCD_MINOR_BASE)
        file->private_data = p_LnxWrpFmDev;
    else {
        if (minor == DEV_FM_OH_PORTS_MINOR_BASE)
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
        else if ((minor > DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->opPorts[minor-DEV_FM_OH_PORTS_MINOR_BASE-1];
        else if ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->rxPorts[minor-DEV_FM_RX_PORTS_MINOR_BASE];
        else if ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS))
            p_LnxWrpFmPortDev = &p_LnxWrpFmDev->txPorts[minor-DEV_FM_TX_PORTS_MINOR_BASE];
        else
            return -EINVAL;

        /* if trying to open port, check if it initialized */
        if (!p_LnxWrpFmPortDev->h_Dev)
            return -ENODEV;

        p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *)fm_port_bind(p_LnxWrpFmPortDev->dev);
        file->private_data = p_LnxWrpFmPortDev;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
    }

    if (file->private_data == NULL)
         return -ENXIO;

    return 0;
}

static int fm_close(struct inode *inode, struct file *file)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev;
    unsigned int        minor = iminor(inode);
    int                 err = 0;

    DBG(TRACE, ("Closing minor - %d - ", minor));

    if ((minor == DEV_FM_MINOR_BASE) ||
        (minor == DEV_FM_PCD_MINOR_BASE))
    {
        p_LnxWrpFmDev = (t_LnxWrpFmDev*)file->private_data;
        if (!p_LnxWrpFmDev)
            return -ENODEV;
        fm_unbind((struct fm *)p_LnxWrpFmDev);
    }
    else if (((minor >= DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS)))
    {
        p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)file->private_data;
        if (!p_LnxWrpFmPortDev)
            return -ENODEV;
        fm_port_unbind((struct fm_port *)p_LnxWrpFmPortDev);
    }

    return err;
}

static int fm_ioctls(unsigned int minor, struct file *file, unsigned int cmd, unsigned long arg, bool compat)
{
    DBG(TRACE, ("IOCTL minor - %u, cmd - 0x%08x, arg - 0x%08lx \n", minor, cmd, arg));

    if ((minor == DEV_FM_MINOR_BASE) ||
        (minor == DEV_FM_PCD_MINOR_BASE))
    {
        t_LnxWrpFmDev *p_LnxWrpFmDev = ((t_LnxWrpFmDev*)file->private_data);
        if (!p_LnxWrpFmDev)
            return -ENODEV;
        if (LnxwrpFmIOCTL(p_LnxWrpFmDev, cmd, arg, compat))
            return -EFAULT;
    }
    else if (((minor >= DEV_FM_OH_PORTS_MINOR_BASE) && (minor < DEV_FM_RX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_RX_PORTS_MINOR_BASE) && (minor < DEV_FM_TX_PORTS_MINOR_BASE)) ||
             ((minor >= DEV_FM_TX_PORTS_MINOR_BASE) && (minor < DEV_FM_MAX_MINORS)))
    {
        t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = ((t_LnxWrpFmPortDev*)file->private_data);
        if (!p_LnxWrpFmPortDev)
            return -ENODEV;
        if (LnxwrpFmPortIOCTL(p_LnxWrpFmPortDev, cmd, arg, compat))
            return -EFAULT;
    }
    else
    {
        REPORT_ERROR(MINOR, E_INVALID_VALUE, ("minor"));
        return -ENODEV;
    }

    return 0;
}

#ifdef CONFIG_COMPAT
static long fm_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int minor = iminor(file->f_path.dentry->d_inode);
    long res;

    fm_mutex_lock();
    res = fm_ioctls(minor, file, cmd, arg, true);
    fm_mutex_unlock();

    return res;
}
#endif

static long fm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int minor = iminor(file->f_path.dentry->d_inode);
    long res;

    fm_mutex_lock();
    res = fm_ioctls(minor, file, cmd, arg, false);
    fm_mutex_unlock();

    return res;
}

/* Globals for FM character device */
struct file_operations fm_fops =
{
    .owner =            THIS_MODULE,
    .unlocked_ioctl =   fm_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl =     fm_compat_ioctl,
#endif
    .open =             fm_open,
    .release =          fm_close,
};
