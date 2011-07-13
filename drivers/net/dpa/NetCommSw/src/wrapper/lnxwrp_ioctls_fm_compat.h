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

 @File          lnxwrp_ioctls_fm_compat.h

 @Description   FM PCD compat structures definition.

*/

#ifndef __FM_COMPAT_IOCTLS_H
#define __FM_COMPAT_IOCTLS_H

#include <linux/compat.h>

#define COMPAT_K_TO_US 0 /* copy from Kernel to User */
#define COMPAT_US_TO_K 1 /* copy from User to Kernel */

#define COMPAT_COPY_K2US(dest, src, type)	compat_copy_##type(src, dest, 0)
#define COMPAT_COPY_US2K(dest, src, type)	compat_copy_##type(dest, src, 1)

/* maping kernel pointers w/ UserSpace id's { */
/* Because compat_ptr(ptr_to_compat(X)) != X, this way we cannot exchange pointers
   back and forth (US - KS). compat_ptr is a cast and pointers are broken. */
#define COMPAT_PTR2ID_ARRAY_MAX (256+1) /* first location is not used */
#define COMPAT_PTR2ID_WATERMARK 0xface0000
#define COMPAT_PTR2ID_WM_MASK   0xffff0000

void compat_del_ptr2id(void *p);
compat_uptr_t compat_add_ptr2id(void *p);
compat_uptr_t compat_get_ptr2id(void *p);
void *compat_get_id2ptr(compat_uptr_t comp);
/* } maping kernel pointers w/ UserSpace id's  */

/* pcd compat structures { */
typedef struct ioc_compat_fm_pcd_cc_node_remove_key_params_t {
    compat_uptr_t                       id;
    uint8_t                             key_indx;
} ioc_compat_fm_pcd_cc_node_remove_key_params_t;

typedef union ioc_compat_fm_pcd_plcr_next_engine_params_u {
        ioc_fm_pcd_done_action     action;
        compat_uptr_t              p_profile;
        compat_uptr_t              p_direct_scheme;
} ioc_compat_fm_pcd_plcr_next_engine_params_u;

typedef struct ioc_compat_fm_pcd_plcr_profile_params_t {
    bool                                        modify;
    union {
        struct {
            ioc_fm_pcd_profile_type_selection   profile_type;
            compat_uptr_t                       p_port;
            uint16_t                            relative_profile_id;
        } new_params;
        compat_uptr_t                           p_profile;
    } profile_select;
    ioc_fm_pcd_plcr_algorithm_selection         alg_selection;
    ioc_fm_pcd_plcr_color_mode                  color_mode;

    union {
        ioc_fm_pcd_plcr_color                   dflt_color;
        ioc_fm_pcd_plcr_color                   override;
    } color;

    ioc_fm_pcd_plcr_non_passthrough_alg_param_t non_passthrough_alg_param;

    ioc_fm_pcd_engine                           next_engine_on_green;
    ioc_compat_fm_pcd_plcr_next_engine_params_u params_on_green;

    ioc_fm_pcd_engine                           next_engine_on_yellow;
    ioc_compat_fm_pcd_plcr_next_engine_params_u params_on_yellow;

    ioc_fm_pcd_engine                           next_engine_on_red;
    ioc_compat_fm_pcd_plcr_next_engine_params_u params_on_red;

    bool                                        trap_profile_on_flow_A;
    bool                                        trap_profile_on_flow_B;
    bool                                        trap_profile_on_flow_C;
    compat_uptr_t                               id;
} ioc_compat_fm_pcd_plcr_profile_params_t;

typedef struct ioc_compat_fm_obj_t {
    compat_uptr_t obj;
} ioc_compat_fm_obj_t;

typedef struct ioc_compat_fm_pcd_kg_scheme_select_t {
    bool          direct;
    compat_uptr_t scheme_id;
} ioc_compat_fm_pcd_kg_scheme_select_t;

typedef struct ioc_compat_fm_pcd_port_schemes_params_t {
    uint8_t        num_of_schemes;
    compat_uptr_t  schemes_ids [IOC_FM_PCD_KG_NUM_OF_SCHEMES];
} ioc_compat_fm_pcd_port_schemes_params_t;

typedef struct ioc_compat_fm_pcd_net_env_params_t {
    uint8_t                         num_of_distinction_units;
    ioc_fm_pcd_distinction_unit_t   units[IOC_FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS]; /* same structure*/
    compat_uptr_t                   id;
} ioc_compat_fm_pcd_net_env_params_t;

typedef struct ioc_compat_fm_pcd_prs_sw_params_t {
    bool                            override;
    uint32_t                        size;
    uint16_t                        base;
    compat_uptr_t                   p_code;
    uint32_t                        sw_prs_data_params[IOC_FM_PCD_PRS_NUM_OF_HDRS];
    uint8_t                         num_of_labels;
    ioc_fm_pcd_prs_label_params_t   labels_table[IOC_FM_PCD_PRS_NUM_OF_LABELS];
} ioc_compat_fm_pcd_prs_sw_params_t;

typedef struct ioc_compat_fm_pcd_cc_next_kg_params_t {
    bool          override_fqid;
    uint32_t      new_fqid;
    compat_uptr_t p_direct_scheme;
} ioc_compat_fm_pcd_cc_next_kg_params_t;

typedef struct ioc_compat_fm_pcd_cc_next_cc_params_t {
    compat_uptr_t        cc_node_id;
} ioc_compat_fm_pcd_cc_next_cc_params_t;

typedef struct ioc_compat_fm_pcd_cc_next_engine_params_t {
    ioc_fm_pcd_engine                                  next_engine;
    union {
            ioc_compat_fm_pcd_cc_next_cc_params_t      cc_params;      /**< compat structure*/
            ioc_fm_pcd_cc_next_plcr_params_t           plcr_params;    /**< same structure*/
            ioc_fm_pcd_cc_next_enqueue_params_t        enqueue_params; /**< same structure*/
            ioc_compat_fm_pcd_cc_next_kg_params_t      kg_params;      /**< compat structure*/
    } params;
#ifdef FM_PCD_CC_MANIP
    compat_uptr_t                                      p_manip;
#endif
} ioc_compat_fm_pcd_cc_next_engine_params_t;


typedef struct ioc_compat_fm_pcd_cc_grp_params_t {
    uint8_t                             num_of_distinction_units;   /**< up to 4 */
    uint8_t                             unit_ids [IOC_FM_PCD_MAX_NUM_OF_CC_UNITS];
                                                                    /**< Indexes of the units as defined in
                                                                         FM_PCD_SetNetEnvCharacteristics */
    ioc_compat_fm_pcd_cc_next_engine_params_t  next_engine_per_entries_in_grp[IOC_FM_PCD_MAX_NUM_OF_CC_ENTRIES_IN_GRP];
                                                                    /**< Max size is 16 - if only one group used */
} ioc_compat_fm_pcd_cc_grp_params_t;

typedef struct ioc_compat_fm_pcd_cc_tree_params_t {
    compat_uptr_t                   net_env_id;
    uint8_t                         num_of_groups;
    ioc_compat_fm_pcd_cc_grp_params_t      fm_pcd_cc_group_params [IOC_FM_PCD_MAX_NUM_OF_CC_GROUPS];
    compat_uptr_t                   id;
} ioc_compat_fm_pcd_cc_tree_params_t;

typedef struct ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t {
    compat_uptr_t                       id;
    uint8_t                             grp_indx;
    uint8_t                             indx;
    ioc_compat_fm_pcd_cc_next_engine_params_t  cc_next_engine_params;
} ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t;

typedef struct ioc_compat_fm_pcd_cc_key_params_t {
    compat_uptr_t                              p_key;
    compat_uptr_t                              p_mask;
    ioc_compat_fm_pcd_cc_next_engine_params_t  cc_next_engine_params; /**< compat structure*/
} ioc_compat_fm_pcd_cc_key_params_t;

typedef struct ioc_compat_keys_params_t {
    uint8_t                                    num_of_keys;
    uint8_t                                    key_size;
    ioc_compat_fm_pcd_cc_key_params_t          key_params[IOC_FM_PCD_MAX_NUM_OF_KEYS]; /**< compat structure*/
    ioc_compat_fm_pcd_cc_next_engine_params_t  cc_next_engine_params_for_miss;         /**< compat structure*/
} ioc_compat_keys_params_t;

typedef struct ioc_compat_fm_pcd_cc_node_params_t {
    ioc_fm_pcd_extract_entry_t                 extract_cc_params;  /**< same structure*/
    ioc_compat_keys_params_t                   keys_params;        /**< compat structure*/
    compat_uptr_t                              id;
} ioc_compat_fm_pcd_cc_node_params_t;

typedef struct ioc_compat_fm_pcd_cc_node_modify_key_params_t {
    compat_uptr_t                       id;
    uint8_t                             key_indx;
    uint8_t                             key_size;
    compat_uptr_t                       p_key;
    compat_uptr_t                       p_mask;
} ioc_compat_fm_pcd_cc_node_modify_key_params_t;

typedef struct ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t {
    compat_uptr_t                       id;
    uint8_t                             key_indx;
    uint8_t                             key_size;
    ioc_compat_fm_pcd_cc_key_params_t   key_params;
} ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t;

typedef struct ioc_compat_fm_port_pcd_plcr_params_t {
    compat_uptr_t                plcr_profile_id;
} ioc_compat_fm_port_pcd_plcr_params_t;

typedef struct ioc_compat_fm_port_pcd_cc_params_t {
    compat_uptr_t                cc_tree_id;
} ioc_compat_fm_port_pcd_cc_params_t;

typedef struct ioc_compat_fm_port_pcd_kg_params_t {
    uint8_t             num_of_schemes;
    compat_uptr_t       schemes_ids[IOC_FM_PCD_KG_NUM_OF_SCHEMES];
    bool                direct_scheme;
    compat_uptr_t       direct_scheme_id;
} ioc_compat_fm_port_pcd_kg_params_t;

typedef struct ioc_compat_fm_port_pcd_params_t {
    ioc_fm_port_pcd_support          pcd_support;
    compat_uptr_t                    net_env_id;
    compat_uptr_t                    p_prs_params;
    compat_uptr_t                    p_cc_params;
    compat_uptr_t                    p_kg_params;
    compat_uptr_t                    p_plcr_params;
} ioc_compat_fm_port_pcd_params_t;

typedef struct ioc_compat_fm_pcd_kg_cc_t {
    compat_uptr_t                   tree_id;
    uint8_t                         grp_id;
    bool                            plcr_next;
    bool                            bypass_plcr_profile_generation;
    ioc_fm_pcd_kg_plcr_profile_t    plcr_profile;
} ioc_compat_fm_pcd_kg_cc_t;

typedef struct ioc_compat_fm_pcd_kg_scheme_params_t {
    bool                                modify;
    union
    {
        uint8_t                         relative_scheme_id;
        compat_uptr_t                   scheme_id;
    } scm_id;
    bool                                always_direct;
    struct
    {
        compat_uptr_t                   net_env_id;
        uint8_t                         num_of_distinction_units;
        uint8_t                         unit_ids[IOC_FM_PCD_MAX_NUM_OF_DISTINCTION_UNITS];
    } netEnvParams;
    bool                                use_hash;
    ioc_fm_pcd_kg_key_extract_and_hash_params_t key_extract_and_hash_params;
    bool                                bypass_fqid_generation;
    uint32_t                            base_fqid;
    uint8_t                             numOfUsedExtractedOrs;
    ioc_fm_pcd_kg_extracted_or_params_t extracted_ors[IOC_FM_PCD_KG_NUM_OF_GENERIC_REGS];
    ioc_fm_pcd_engine                   next_engine;
    union{
        ioc_fm_pcd_done_action          done_action;
        ioc_fm_pcd_kg_plcr_profile_t    plcr_profile;
        ioc_compat_fm_pcd_kg_cc_t       cc;
    } kg_next_engine_params;
    ioc_fm_pcd_kg_scheme_counter_t      scheme_counter;
    compat_uptr_t                       id;
} ioc_compat_fm_pcd_kg_scheme_params_t;

typedef struct ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t {
    compat_uptr_t                       id;
    uint8_t                             key_indx;
    uint8_t                             key_size;
    ioc_compat_fm_pcd_cc_next_engine_params_t  cc_next_engine_params;
} ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t;
/* } pcd compat structures */

/* pcd compat functions { */
void compat_copy_fm_pcd_plcr_profile(
        ioc_compat_fm_pcd_plcr_profile_params_t *compat_param,
        ioc_fm_pcd_plcr_profile_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_key(
        ioc_compat_fm_pcd_cc_key_params_t *compat_param,
        ioc_fm_pcd_cc_key_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_node_modify_key_and_next_engine(
        ioc_compat_fm_pcd_cc_node_modify_key_and_next_engine_params_t *compat_param,
        ioc_fm_pcd_cc_node_modify_key_and_next_engine_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_node_modify_next_engine(
        ioc_compat_fm_pcd_cc_node_modify_next_engine_params_t *compat_param,
        ioc_fm_pcd_cc_node_modify_next_engine_params_t *param,
        uint8_t compat);

void compat_fm_pcd_cc_tree_modify_next_engine(
        ioc_compat_fm_pcd_cc_tree_modify_next_engine_params_t *compat_param,
        ioc_fm_pcd_cc_tree_modify_next_engine_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_grp(
        ioc_compat_fm_pcd_cc_grp_params_t *compat_param,
        ioc_fm_pcd_cc_grp_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_tree(
        ioc_compat_fm_pcd_cc_tree_params_t *compat_param,
        ioc_fm_pcd_cc_tree_params_t *param,
        uint8_t compat);

void compat_fm_pcd_prs_sw(
        ioc_compat_fm_pcd_prs_sw_params_t *compat_param,
        ioc_fm_pcd_prs_sw_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_kg_scheme(
        ioc_compat_fm_pcd_kg_scheme_params_t *compat_param,
        ioc_fm_pcd_kg_scheme_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_kg_scheme_select(
        ioc_compat_fm_pcd_kg_scheme_select_t *compat_param,
        ioc_fm_pcd_kg_scheme_select_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_kg_schemes_params(
        ioc_compat_fm_pcd_port_schemes_params_t *compat_param,
        ioc_fm_pcd_port_schemes_params_t *param,
        uint8_t compat);

void compat_copy_fm_port_pcd_kg(
        ioc_compat_fm_port_pcd_kg_params_t *compat_param,
        ioc_fm_port_pcd_kg_params_t *param,
        uint8_t compat);

void compat_copy_fm_port_pcd(
        ioc_compat_fm_port_pcd_params_t *compat_param,
        ioc_fm_port_pcd_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_net_env(
        ioc_compat_fm_pcd_net_env_params_t *compat_param,
        ioc_fm_pcd_net_env_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_node_modify_key(
        ioc_compat_fm_pcd_cc_node_modify_key_params_t *compat_param,
        ioc_fm_pcd_cc_node_modify_key_params_t *param,
        uint8_t compat);

void compat_copy_keys(
        ioc_compat_keys_params_t *compat_param,
        ioc_keys_params_t *param,
        uint8_t compat);

void compat_copy_fm_pcd_cc_node(
        ioc_compat_fm_pcd_cc_node_params_t *compat_param,
        ioc_fm_pcd_cc_node_params_t *param,
        uint8_t compat);

/* } pcd compat functions */
#endif
