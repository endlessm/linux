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

/******************************************************************************
 @File          fsl_fman.h

 @Description   Linux internal kernel API
*//***************************************************************************/

#ifndef __FSL_FMAN_H
#define __FSL_FMAN_H

#include <linux/types.h>
#include <linux/device.h>   /* struct device */
#include <linux/fsl_qman.h> /* struct qman_fq */
#include "dpaa_integration_ext.h"

/**************************************************************************//**
 @Group         FM_LnxKern_grp Frame Manager Linux wrapper API

 @Description   FM API functions, definitions and enums.

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group         FM_LnxKern_ctrl_grp Control Unit

 @Description   Control Unit

                Internal Kernel Control Unit API
 @{
*//***************************************************************************/

/*****************************************************************************/
/*                  Internal Linux kernel routines                           */
/*****************************************************************************/

/**************************************************************************//**
 @Description   A structure ..,
*//***************************************************************************/
struct fm;

/**************************************************************************//**
 @Description   A structure ..,
*//***************************************************************************/
struct fm_port;

typedef int (*alloc_pcd_fqids)(struct device    *dev,
                               uint32_t         num,
                               uint8_t          alignment,
                               uint32_t         *base_fqid);

typedef int (*free_pcd_fqids)(struct device    *dev,
                               uint32_t         base_fqid);

struct fm_port_pcd_param {
    alloc_pcd_fqids         cba;
    free_pcd_fqids          cbf;
    struct device           *dev;
};

/**************************************************************************//**
 @Description   A structure of information about each of the external
                buffer pools used by the port,
*//***************************************************************************/
struct fm_port_pool_param {
    uint8_t                 id;                 /**< External buffer pool id */
    uint16_t                size;               /**< External buffer pool buffer size */
};

/**************************************************************************//**
 @Description   structure for additional Rx port parameters
*//***************************************************************************/
struct fm_port_rx_params {
    uint32_t                    errq;               /**< Error Queue Id. */
    uint32_t                    defq;               /**< Default Queue Id.  */
    uint8_t                     num_pools;          /**< Number of pools use by this port */
    struct fm_port_pool_param   pool_param[FM_PORT_MAX_NUM_OF_EXT_POOLS];
                                                    /**< Parameters for each pool */
    uint16_t                    priv_data_size;     /**< Area that user may save for his own need (E.g. save the SKB) */
    bool                        parse_results;      /**< Whether to have the parser-results in the Received buffer */
    bool                        hash_results;       /**< Whether to have the hash-results in the Received buffer */
    bool                        time_stamp;         /**< Whether to have the time-stamp in the Received buffer */
};

/**************************************************************************//**
 @Description   structure for additional non-Rx port parameters
*//***************************************************************************/
struct fm_port_non_rx_params {
    uint32_t                errq;               /**< Error Queue Id. */
    uint32_t                defq;               /**< For Tx and HC - Default Confirmation queue,
                                                     0 means no Tx confirmation for processed
                                                     frames. For OP - default Rx queue. */
    uint16_t                priv_data_size;     /**< Area that user may save for his own need (E.g. save the SKB) */
    bool                    parse_results;      /**< Whether to put the parser-results in the Transmitted buffer */
    bool                    hash_results;       /**< Whether to have the hash-results in the Received buffer */
    bool                    time_stamp;         /**< Whether to have the time-stamp in the Received buffer */
};


/**************************************************************************//**
 @Function      fm_bind

 @Description   Bind to a specific FM device.

 @Param[in]     fm_dev  - the OF handle of the FM device.

 @Return        A handle of the FM device.

 @Cautions      Allowed only after the port was created.
*//***************************************************************************/
struct fm * fm_bind (struct device *fm_dev);

/**************************************************************************//**
 @Function      fm_unbind

 @Description   Un-bind from a specific FM device.

 @Param[in]     fm      - A handle of the FM device.

 @Cautions      Allowed only after the port was created.
*//***************************************************************************/
void fm_unbind(struct fm *fm);

void * fm_get_handle(struct fm *fm);
void * fm_get_rtc_handle(struct fm *fm);
struct resource * fm_get_mem_region(struct fm *fm);

/**************************************************************************//**
 @Function      fm_port_bind

 @Description   Bind to a specific FM-port device (may be Rx or Tx port).

 @Param[in]     fm_port_dev - the OF handle of the FM port device.

 @Return        A handle of the FM port device.

 @Cautions      Allowed only after the port was created.
*//***************************************************************************/
struct fm_port * fm_port_bind (struct device *fm_port_dev);

/**************************************************************************//**
 @Function      fm_port_unbind

 @Description   Un-bind from a specific FM-port device (may be Rx or Tx port).

 @Param[in]     port        - A handle of the FM port device.

 @Cautions      Allowed only after the port was created.
*//***************************************************************************/
void fm_port_unbind(struct fm_port *port);

/**************************************************************************//**
 @Function      fm_set_rx_port_params

 @Description   Configure parameters for a specific Rx FM-port device.

 @Param[in]     port        - A handle of the FM port device.
 @Param[in]     params      - Rx port parameters

 @Cautions      Allowed only after the port is binded.
*//***************************************************************************/
void fm_set_rx_port_params(struct fm_port *port, struct fm_port_rx_params *params);

/**************************************************************************//**
 @Function      fm_port_pcd_bind

 @Description   Bind as a listener on a port PCD.

 @Param[in]     port        - A handle of the FM port device.
 @Param[in]     params      - PCD port parameters

 @Cautions      Allowed only after the port is binded.
*//***************************************************************************/
void fm_port_pcd_bind (struct fm_port *port, struct fm_port_pcd_param *params);

/**************************************************************************//**
 @Function      fm_get_tx_port_channel

 @Description   Get qman-channel number for this Tx port.

 @Param[in]     port        - A handle of the FM port device.

 @Return        qman-channel number for this Tx port.

 @Cautions      Allowed only after the port is binded.
*//***************************************************************************/
int fm_get_tx_port_channel(struct fm_port *port);

/**************************************************************************//**
 @Function      fm_set_tx_port_params

 @Description   Configure parameters for a specific Tx FM-port device

 @Param[in]     port        - A handle of the FM port device.
 @Param[in]     params      - Tx port parameters

 @Cautions      Allowed only after the port is binded.
*//***************************************************************************/
void fm_set_tx_port_params(struct fm_port *port, struct fm_port_non_rx_params *params);

/**************************************************************************//**
 @Function      fm_port_enable

 @Description   Enable specific FM-port device (may be Rx or Tx port).

 @Param[in]     port        - A handle of the FM port device.

 @Cautions      Allowed only the port is initialized.
*//***************************************************************************/
int fm_port_enable (struct fm_port *port);

/**************************************************************************//**
 @Function      fm_port_disable

 @Description   Disable specific FM-port device (may be Rx or Tx port).

 @Param[in]     port        - A handle of the FM port device.

 @Cautions      Allowed only the port is initialized.
*//***************************************************************************/
void fm_port_disable(struct fm_port *port);

void * fm_port_get_handle(struct fm_port *port);

/**************************************************************************//**
 @Description   Get base address of this port. Useful for accessing
                port-specific registers (i.e., not common ones).

 @Param[in]     port        - A handle of the FM port device.
 @Param[out]    base_addr   - The port's base addr (virtual address).
*//***************************************************************************/
void fm_port_get_base_addr(const struct fm_port *port, uint64_t *base_addr);

/**************************************************************************//**
 @Description   Lock function required before any FMD/LLD call.
*//***************************************************************************/
void fm_mutex_lock(void);

/**************************************************************************//**
 @Description   Unlock function required after any FMD/LLD call.
*//***************************************************************************/
void fm_mutex_unlock(void);

/** @} */ /* end of FM_LnxKern_ctrl_grp group */
/** @} */ /* end of FM_LnxKern_grp group */


#endif /* __FSL_FMAN_H */
