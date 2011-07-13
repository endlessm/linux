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

/**

 @File          dpaa_integration_ext.h

 @Description   P4080 FM external definitions and structures.
*//***************************************************************************/
#ifndef __DPAA_INTEGRATION_EXT_H
#define __DPAA_INTEGRATION_EXT_H

#include "std_ext.h"


typedef enum {
    e_DPAA_SWPORTAL0 = 0,
    e_DPAA_SWPORTAL1,
    e_DPAA_SWPORTAL2,
    e_DPAA_SWPORTAL3,
    e_DPAA_SWPORTAL4,
    e_DPAA_SWPORTAL5,
    e_DPAA_SWPORTAL6,
    e_DPAA_SWPORTAL7,
    e_DPAA_SWPORTAL8,
    e_DPAA_SWPORTAL9,
    e_DPAA_SWPORTAL_DUMMY_LAST
} e_DpaaSwPortal;

typedef enum {
    e_DPAA_DCPORTAL0 = 0,
    e_DPAA_DCPORTAL1,
    e_DPAA_DCPORTAL2,
    e_DPAA_DCPORTAL3,
    e_DPAA_DCPORTAL4,
    e_DPAA_DCPORTAL_DUMMY_LAST
} e_DpaaDcPortal;

#define DPAA_MAX_NUM_OF_SW_PORTALS      e_DPAA_SWPORTAL_DUMMY_LAST
#define DPAA_MAX_NUM_OF_DC_PORTALS      e_DPAA_DCPORTAL_DUMMY_LAST

/*****************************************************************************
 QMan INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define QMAN_PM_DCP_COUNTERS_ERRATA_QMAN1
#define QMAN_FQD_AVOID_BLK_ERRATA_QMAN2
#define QMAN_DBG_TRC_EV_ERRATA_QMAN3
#define QMAN_WQ_CS_CFG_ERRATA_QMAN4
#define QMAN_SFDR_LEAK_ERRATA_QMAN5
#define QMAN_FQ_TD_THRESH_ERRATA_QMAN6
#define QMAN_FQ_INIT_ON_PARKED_ERRATA_QMAN7
#define QMAN_NESN_ORR_ERRATA_QMAN8
#define QMAN_ERN_REJ_CODE6_ERRATA_QMAN9
#define QMAN_ERN_MOULTI_CORE_ERRATA_QMAN10
#define QMAN_PERFMON_FOR_DCP_FQD_ERRATA_QMAN11

#define QM_MAX_NUM_OF_POOL_CHANNELS 15
#define QM_MAX_NUM_OF_WQ            8
#define QM_MAX_NUM_OF_SWP_AS        4
#define QM_MAX_NUM_OF_CGS           256
#define QM_MAX_NUM_OF_FQIDS           (16*MEGABYTE)

/**************************************************************************//**
 @Description   Work Queue Channel assignments in QMan.
*//***************************************************************************/
typedef enum
{
    e_QM_FQ_CHANNEL_SWPORTAL0 = 0,              /**< Dedicated channels serviced by software portals 0 to 9 */
    e_QM_FQ_CHANNEL_SWPORTAL1,
    e_QM_FQ_CHANNEL_SWPORTAL2,
    e_QM_FQ_CHANNEL_SWPORTAL3,
    e_QM_FQ_CHANNEL_SWPORTAL4,
    e_QM_FQ_CHANNEL_SWPORTAL5,
    e_QM_FQ_CHANNEL_SWPORTAL6,
    e_QM_FQ_CHANNEL_SWPORTAL7,
    e_QM_FQ_CHANNEL_SWPORTAL8,
    e_QM_FQ_CHANNEL_SWPORTAL9,

    e_QM_FQ_CHANNEL_POOL1 = 0x21,               /**< Pool channels that can be serviced by any of the software portals */
    e_QM_FQ_CHANNEL_POOL2,
    e_QM_FQ_CHANNEL_POOL3,
    e_QM_FQ_CHANNEL_POOL4,
    e_QM_FQ_CHANNEL_POOL5,
    e_QM_FQ_CHANNEL_POOL6,
    e_QM_FQ_CHANNEL_POOL7,
    e_QM_FQ_CHANNEL_POOL8,
    e_QM_FQ_CHANNEL_POOL9,
    e_QM_FQ_CHANNEL_POOL10,
    e_QM_FQ_CHANNEL_POOL11,
    e_QM_FQ_CHANNEL_POOL12,
    e_QM_FQ_CHANNEL_POOL13,
    e_QM_FQ_CHANNEL_POOL14,
    e_QM_FQ_CHANNEL_POOL15,

    e_QM_FQ_CHANNEL_FMAN0_SP0 = 0x40,           /**< Dedicated channels serviced by Direct Connect Portal 0:
                                                     connected to FMan 0; assigned in incrementing order to
                                                     each sub-portal (SP) in the portal */
    e_QM_FQ_CHANNEL_FMAN0_SP1,
    e_QM_FQ_CHANNEL_FMAN0_SP2,
    e_QM_FQ_CHANNEL_FMAN0_SP3,
    e_QM_FQ_CHANNEL_FMAN0_SP4,
    e_QM_FQ_CHANNEL_FMAN0_SP5,
    e_QM_FQ_CHANNEL_FMAN0_SP6,
    e_QM_FQ_CHANNEL_FMAN0_SP7,
    e_QM_FQ_CHANNEL_FMAN0_SP8,
    e_QM_FQ_CHANNEL_FMAN0_SP9,
    e_QM_FQ_CHANNEL_FMAN0_SP10,
    e_QM_FQ_CHANNEL_FMAN0_SP11,

    e_QM_FQ_CHANNEL_FMAN1_SP0 = 0x60,
    e_QM_FQ_CHANNEL_FMAN1_SP1,
    e_QM_FQ_CHANNEL_FMAN1_SP2,
    e_QM_FQ_CHANNEL_FMAN1_SP3,
    e_QM_FQ_CHANNEL_FMAN1_SP4,
    e_QM_FQ_CHANNEL_FMAN1_SP5,
    e_QM_FQ_CHANNEL_FMAN1_SP6,
    e_QM_FQ_CHANNEL_FMAN1_SP7,
    e_QM_FQ_CHANNEL_FMAN1_SP8,
    e_QM_FQ_CHANNEL_FMAN1_SP9,
    e_QM_FQ_CHANNEL_FMAN1_SP10,
    e_QM_FQ_CHANNEL_FMAN1_SP11,

    e_QM_FQ_CHANNEL_CAAM = 0x80,                /**< Dedicated channel serviced by Direct Connect Portal 2:
                                                     connected to SEC 4.x */

    e_QM_FQ_CHANNEL_PME = 0xA0,                 /**< Dedicated channel serviced by Direct Connect Portal 3:
                                                     connected to PME */
} e_QmFQChannel;

/* p4080-rev1 unique features */
#define QM_CGS_NO_FRAME_MODE

/*****************************************************************************
 BMan INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define BM_MAX_NUM_OF_POOLS         64

/*****************************************************************************
 SEC INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
/* SEC erratas */
#ifdef UNDER_CONSTRUCTION_IPSEC
#define SEC_IPV6_UDP_CHECKSUM_UPDATE
#define SEC_UDP_LENGTH_UPDATE
#endif /* UNDER_CONSTRUCTION_IPSEC */

/*****************************************************************************
 FM INTEGRATION-SPECIFIC DEFINITIONS
******************************************************************************/
#define INTG_MAX_NUM_OF_FM          2

/* Ports defines */
#define FM_MAX_NUM_OF_1G_RX_PORTS   5
#define FM_MAX_NUM_OF_10G_RX_PORTS  1
#define FM_MAX_NUM_OF_RX_PORTS      (FM_MAX_NUM_OF_10G_RX_PORTS+FM_MAX_NUM_OF_1G_RX_PORTS)
#define FM_MAX_NUM_OF_1G_TX_PORTS   5
#define FM_MAX_NUM_OF_10G_TX_PORTS  1
#define FM_MAX_NUM_OF_TX_PORTS      (FM_MAX_NUM_OF_10G_TX_PORTS+FM_MAX_NUM_OF_1G_TX_PORTS)
#define FM_MAX_NUM_OF_OH_PORTS      7
#define FM_MAX_NUM_OF_1G_MACS       (FM_MAX_NUM_OF_1G_RX_PORTS)
#define FM_MAX_NUM_OF_10G_MACS      (FM_MAX_NUM_OF_10G_RX_PORTS)
#define FM_MAX_NUM_OF_MACS          (FM_MAX_NUM_OF_1G_MACS+FM_MAX_NUM_OF_10G_MACS)


#define FM_PORT_MAX_NUM_OF_EXT_POOLS            8           /**< Number of external BM pools per Rx port */
#define FM_PORT_NUM_OF_CONGESTION_GRPS          256         /**< Total number of congestion groups in QM */
#define FM_MAX_NUM_OF_SUB_PORTALS               12
#define FM_PORT_MAX_NUM_OF_OBSERVED_EXT_POOLS   0

/* RAMs defines */
#define FM_MURAM_SIZE                   (160 * KILOBYTE)
#define FM_IRAM_SIZE                    ( 64 * KILOBYTE)

/* PCD defines */
#define FM_PCD_PLCR_NUM_ENTRIES         256                 /**< Total number of policer profiles */
#define FM_PCD_KG_NUM_OF_SCHEMES        32                  /**< Total number of KG schemes */
#define FM_PCD_MAX_NUM_OF_CLS_PLANS     256                 /**< Number of classification plan entries. */

/* RTC defines */
#define FM_RTC_NUM_OF_ALARMS            2                   /**< RTC number of alarms */
#define FM_RTC_NUM_OF_PERIODIC_PULSES   2                   /**< RTC number of periodic pulses */
#define FM_RTC_NUM_OF_EXT_TRIGGERS      2                   /**< RTC number of external triggers */

/* QMI defines */
#define QMI_MAX_NUM_OF_TNUMS            64
#define MAX_QMI_DEQ_SUBPORTAL           12
#define QMI_DEF_TNUMS_THRESH            48

/* FPM defines */
#define FM_NUM_OF_FMAN_CTRL_EVENT_REGS  4

/* DMA defines */
#define DMA_THRESH_MAX_COMMQ            31
#define DMA_THRESH_MAX_BUF              127

/* BMI defines */
#define BMI_MAX_NUM_OF_TASKS            128
#define BMI_MAX_NUM_OF_DMAS             32
#define BMI_MAX_FIFO_SIZE               (FM_MURAM_SIZE)
#define PORT_MAX_WEIGHT                 16

#ifdef UNDER_CONSTRUCTION_FRAG_REASSEMBLY
/* Reassembly defines */
#define FM_MAX_NUM_OF_REASSEMBLY_PORTS  4
#endif /* UNDER_CONSTRUCTION_FRAG_REASSEMBLY */


/**************************************************************************//**
 @Description   Enum for inter-module interrupts registration
*//***************************************************************************/
typedef enum e_FmEventModules{
    e_FM_MOD_PRS,                   /**< Parser event */
    e_FM_MOD_KG,                    /**< Keygen event */
    e_FM_MOD_PLCR,                  /**< Policer event */
    e_FM_MOD_10G_MAC,               /**< 10G MAC error event */
    e_FM_MOD_1G_MAC,                /**< 1G MAC error event */
    e_FM_MOD_TMR,                   /**< Timer event */
    e_FM_MOD_1G_MAC_TMR,            /**< 1G MAC timer event */
    e_FM_MOD_FMAN_CTRL,             /**< FMAN Controller timer event */
    e_FM_MOD_DUMMY_LAST
} e_FmEventModules;

/**************************************************************************//**
 @Description   Enum for interrupts types
*//***************************************************************************/
typedef enum e_FmIntrType {
    e_FM_INTR_TYPE_ERR,
    e_FM_INTR_TYPE_NORMAL
} e_FmIntrType;

/**************************************************************************//**
 @Description   Enum for inter-module interrupts registration
*//***************************************************************************/
typedef enum e_FmInterModuleEvent {
    e_FM_EV_PRS,                    /**< Parser event */
    e_FM_EV_ERR_PRS,                /**< Parser error event */
    e_FM_EV_KG,                     /**< Keygen event */
    e_FM_EV_ERR_KG,                 /**< Keygen error event */
    e_FM_EV_PLCR,                   /**< Policer event */
    e_FM_EV_ERR_PLCR,               /**< Policer error event */
    e_FM_EV_ERR_10G_MAC0,           /**< 10G MAC 0 error event */
    e_FM_EV_ERR_1G_MAC0,            /**< 1G MAC 0 error event */
    e_FM_EV_ERR_1G_MAC1,            /**< 1G MAC 1 error event */
    e_FM_EV_ERR_1G_MAC2,            /**< 1G MAC 2 error event */
    e_FM_EV_ERR_1G_MAC3,            /**< 1G MAC 3 error event */
    e_FM_EV_ERR_1G_MAC4,            /**< 1G MAC 4 error event */
    e_FM_EV_TMR,                    /**< Timer event */
    e_FM_EV_1G_MAC1,                /**< 1G MAC 1 event */
    e_FM_EV_1G_MAC2,                /**< 1G MAC 2 event */
    e_FM_EV_1G_MAC3,                /**< 1G MAC 3 event */
    e_FM_EV_1G_MAC4,                /**< 1G MAC 3 event */
    e_FM_EV_1G_MAC0_TMR,            /**< 1G MAC 0 Timer event */
    e_FM_EV_1G_MAC1_TMR,            /**< 1G MAC 1 Timer event */
    e_FM_EV_1G_MAC2_TMR,            /**< 1G MAC 2 Timer event */
    e_FM_EV_1G_MAC3_TMR,            /**< 1G MAC 3 Timer event */
    e_FM_EV_1G_MAC4_TMR,            /**< 1G MAC 4 Timer event */
    e_FM_EV_FMAN_CTRL_0,            /**< Fman controller event 0 */
    e_FM_EV_FMAN_CTRL_1,            /**< Fman controller event 1 */
    e_FM_EV_FMAN_CTRL_2,            /**< Fman controller event 2 */
    e_FM_EV_FMAN_CTRL_3,            /**< Fman controller event 3 */
    e_FM_EV_DUMMY_LAST
} e_FmInterModuleEvent;

#define GET_FM_MODULE_EVENT(mod, id, intrType, event)                                               \
    switch(mod){                                                                                    \
        case e_FM_MOD_PRS:                                                                          \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_PRS : e_FM_EV_PRS;          \
            break;                                                                                  \
        case e_FM_MOD_KG:                                                                           \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_KG : e_FM_EV_DUMMY_LAST;    \
            break;                                                                                  \
        case e_FM_MOD_PLCR:                                                                         \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_PLCR : e_FM_EV_PLCR;        \
            break;                                                                                  \
        case e_FM_MOD_10G_MAC:                                                                      \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_10G_MAC0 : e_FM_EV_DUMMY_LAST;\
            break;                                                                                  \
        case e_FM_MOD_1G_MAC:                                                                       \
            switch(id){                                                                             \
                 case(0): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC0 : e_FM_EV_DUMMY_LAST; break; \
                 case(1): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC1 : e_FM_EV_DUMMY_LAST; break; \
                 case(2): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC2 : e_FM_EV_DUMMY_LAST; break; \
                 case(3): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC3 : e_FM_EV_DUMMY_LAST; break; \
                 case(4): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_ERR_1G_MAC4 : e_FM_EV_DUMMY_LAST; break; \
                 }                                                                                  \
            break;                                                                                  \
        case e_FM_MOD_TMR:                                                                          \
            if (id) event = e_FM_EV_DUMMY_LAST;                                                     \
            else event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_TMR;       \
            break;                                                                                  \
        case e_FM_MOD_1G_MAC_TMR:                                                                   \
            switch(id){                                                                             \
                 case(0): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_1G_MAC0_TMR; break;\
                 case(1): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_1G_MAC1_TMR; break;\
                 case(2): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_1G_MAC2_TMR; break;\
                 case(3): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_1G_MAC3_TMR; break;\
                 case(4): event = (intrType == e_FM_INTR_TYPE_ERR) ? e_FM_EV_DUMMY_LAST : e_FM_EV_1G_MAC4_TMR; break;\
                 }                                                                                  \
            break;                                                                                  \
        case e_FM_MOD_FMAN_CTRL:                                                                    \
            if (intrType == e_FM_INTR_TYPE_ERR) event = e_FM_EV_DUMMY_LAST;                         \
            else switch(id){                                                                        \
                 case(0): event = e_FM_EV_FMAN_CTRL_0; break;                                       \
                 case(1): event = e_FM_EV_FMAN_CTRL_1; break;                                       \
                 case(2): event = e_FM_EV_FMAN_CTRL_2; break;                                       \
                 case(3): event = e_FM_EV_FMAN_CTRL_3; break;                                       \
                 }                                                                                  \
            break;                                                                                  \
        default: event = e_FM_EV_DUMMY_LAST;                                                        \
        break;}

#define FM_CHECK_PORT_RESTRICTIONS(__validPorts, __newPortIndx)   TRUE

/* p4080-rev1 unique features */
#define FM_PARTITION_ARRAY
#define FM_PPPOE_NO_MTU_CHECK

/* p4080 unique features */
#ifdef UNDER_CONSTRUCTION_IPSEC
#define FM_ETH_TYPE_FIX
#define FM_DISABLE_SEC_ERRORS
#endif /* UNDER_CONSTRUCTION_IPSEC */
#define FM_QMI_DEQ_OPTIONS_SUPPORT
#define FM_NO_DISPATCH_RAM_ECC
#define FM_FIFO_ALLOCATION_OLD_ALG
#define FM_NO_WATCHDOG
#define FM_NO_TNUM_AGING
#define FM_NO_TGEC_LOOPBACK
#define FM_KG_NO_BYPASS_FQID_GEN
#define FM_KG_NO_BYPASS_PLCR_PROFILE_GEN
#define FM_NO_BACKUP_POOLS
#define FM_NO_OP_OBSERVED_POOLS
#define FM_NO_ADVANCED_RATE_LIMITER
#define FM_NO_OP_OBSERVED_CGS

/* FM erratas */
#define FM_SINGLE_MDIO_ERRATA_GEN8                      /* implemented in platform */
#define FM_HALT_SIG_ERRATA_GEN12

#define FM_10G_MDIO_HOLD_ERRATA_XAUI3                   /* implemented in platform */
#define FM_10G_PCS_ALIGNMENT_ERRATA_XAUI4               /* implemented in platform */

#define FM_IEEE_BAD_TS_ERRATA_IEEE1588_A001             /* No implementation, Out of LLD scope */

#define FM_FALSE_RDRP_ERRATA_10GMAC_A001                /* No implementation, Out of LLD scope */
#define FM_RX_EXTRA_BYTES_ERRATA_10GMAC_A002            /* No implementation, Out of LLD scope */
#define FM_TX_PAUSE_ON_ENABLE_ERRATA_10GMAC_A003        /* No implementation, Out of LLD scope */
#define FM_TX_ECC_FRMS_ERRATA_10GMAC_A004
#define FM_TX_SHORT_FRAME_BAD_TS_ERRATA_10GMAC_A006     /* No implementation, Out of LLD scope */
#define FM_TX_FIFO_CORRUPTION_ERRATA_10GMAC_A007
#define FM_ECC_HALT_NO_SYNC_ERRATA_10GMAC_A008

#define FM_TX_INVALID_ECC_ERRATA_10GMAC_A009

#define FM_NO_RX_PREAM_ERRATA_DTSECx1
#define FM_RX_PREAM_4_ERRATA_DTSEC_A001                 FM_NO_RX_PREAM_ERRATA_DTSECx1
#define FM_GRS_ERRATA_DTSEC_A002
#define FM_BAD_TX_TS_IN_B_2_B_ERRATA_DTSEC_A003
#define FM_GTS_ERRATA_DTSEC_A004
#define FM_PAUSE_BLOCK_ERRATA_DTSEC_A006                        /* do nothing */
#define FM_RESERVED_ACCESS_TO_DISABLED_DEV_ERRATA_DTSEC_A0011   /* do nothing */
#define FM_GTS_AFTER_MAC_ABORTED_FRAME_ERRATA_DTSEC_A0012       FM_GTS_ERRATA_DTSEC_A004

#define FM_SHORT_PAUSE_TIME_ERRATA_DTSEC1
#define FM_MAGIC_PACKET_UNRECOGNIZED_ERRATA_DTSEC2          /* No implementation, Out of LLD scope */
#define FM_10_100_SGMII_NO_TS_ERRATA_DTSEC3
#define FM_TX_LOCKUP_ERRATA_DTSEC6

#define FM_IM_TX_SYNC_SKIP_TNUM_ERRATA_FMAN_A001            /* Implemented by ucode */
#define FM_RX_PIPELINE_OF_DATA_CORRUPTION_ERRATA_FMAN_A002  /* No implementation, Out of LLD scope */
#define FM_HC_DEF_FQID_ONLY_ERRATA_FMAN_A003                /* Implemented by ucode */

#define FM_1588_SRC_CLK_ERRATA_FMAN1
#define FM_NO_RUNNING_SUM_FOR_DBG_N_SWPRS_ERRATA_FMAN2      /* No implementation, Out of LLD scope */
#define FM_IM_TX_SHARED_TNUM_ERRATA_FMAN4                   /* Implemented by ucode */
#define FM_IM_GS_DEADLOCK_ERRATA_FMAN5                      /* Implemented by ucode */
#define FM_PORT_SYNC_ERRATA_FMAN6
#define FM_RAM_LIST_ERR_IRQ_ERRATA_FMAN8
#define FM_BMI_PIPELINE_ERR_IRQ_ERRATA_FMAN9
#define FM_IM_DEQ_PIPELINE_DEPTH_ERRATA_FMAN10              /* Implemented by ucode */
#define FM_CC_GEN6_MISSMATCH_ERRATA_FMAN12                  /* Implemented by ucode */
#define FM_CC_CHANGE_SHARED_TNUM_ERRATA_FMAN13              /* Implemented by ucode */
#define FM_IM_LARGE_MRBLR_ERRATA_FMAN15                     /* Implemented by ucode */
#define FM_RESET_ERRATA_FMAN16                              /* No implementation, Out of LLD scope */
#define FM_IPV4_HDRLEN0_ERRATA_FMAN17                       /* No implementation, Out of LLD scope */
#define FM_INCORRECT_CS_ERRATA_FMAN18
#define FM_ILLEGAL_FRM_LEN_ERRATA_FMAN20                    /* No implementation, Out of LLD scope */
#define FM_OP_PORT_QMAN_REJECT_ERRATA_FMAN21

#define FM_PRS_L4_SHELL_ERRATA_FMANb
#define FM_BMI_TO_RISC_ENQ_ERRATA_FMANc
#define FM_INVALID_SWPRS_DATA_ERRATA_FMANd                  /* No implementation, Out of LLD scope */
//#define FM_PRS_L4_NO_CLEAR_ERRATA_FMANe                     /* No implementation, No patch yet */
//#define FM_PRS_MPLS_ERROR_ERRATA_FMANf                      /* No implementation, No patch yet */
#define FM_PORT_COUNTERS_ERRATA_FMANg
#define FM_BAD_RX_FD_ERRATA_FMANh                           /* No implementation, Out of LLD scope */
//#define FM_PRS_MPLS_SSA_ERRATA_FMANj                        /* No implementation, No patch yet */
//#define FM_PRS_INITIAL_PLANID_ERRATA_FMANk                  /* No implementation, No patch yet */


#define FM_OP_PARTITION_ERRATA_FMANx8
#define FM_PORT_DISABLED_ERRATA_FMANx9
#define FM_TX_PORT_IM_OR_ERRATA_FMANx11                     /* Implemented by ucode */
#define FM_PORT_EXCESSIVE_BUDGET_ERRATA_FMANx16
#define FM_PORT_OTF_CHANGES_ERRATA_FMANx12                  FM_PORT_EXCESSIVE_BUDGET_ERRATA_FMANx16
#define FM_SOFT_RESET_ERRATA_FMANx15                        /* No implementation, Out of LLD scope */

#define FM_UCODE_NOT_RESET_ERRATA_BUGZILLA6173

#define FM_PRS_MEM_ERRATA_FMAN_SW003
#define FM_LEN_CHECK_ERRATA_FMAN_SW002
#define FM_10G_REM_N_LCL_FLT_EX_ERRATA_10GMAC001

#ifdef UNDER_CONSTRUCTION_IPSEC
#define FM_NO_COPY_CTXA_CTXB_ERRATA_FMAN_SW001
#endif /* UNDER_CONSTRUCTION_IPSEC */


#endif /* __DPAA_INTEGRATION_EXT_H */
