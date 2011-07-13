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
 @File          fsl_fman_test.h

 @Description
*//***************************************************************************/

#ifndef __FSL_FMAN_TEST_H
#define __FSL_FMAN_TEST_H

#include <linux/types.h>


#define FMT_RX_ERR_Q    0xffffffff
#define FMT_RX_DFLT_Q   0xfffffffe
#define FMT_TX_ERR_Q    0xfffffffd
#define FMT_TX_CONF_Q   0xfffffffc


/**************************************************************************//**
 @Function      is_fman_test

 @Description   Check if arriving frame belong to the test

 @Param[in]     mac_dev - TODO
 @Param[in]     queueId - TODO
 @Param[in]     buffer  - A pointer to the buffer to check.
 @Param[in]     size    - size of the given buffer.

 @Return        true if this buffer belongs to FMan test application; false otherwise.

 @Cautions      Allowed only the port is initialized.
*//***************************************************************************/
bool is_fman_test (void     *mac_dev,
                   uint32_t queueId,
                   uint8_t  *buffer,
                   uint32_t size);

/**************************************************************************//**
 @Function      fman_test_ip_manip

 @Description   IP header manipulation

 @Param[in]     mac_dev - TODO
 @Param[in]     data    - A pointer to the data (payload) to manipulate.

 @Cautions      Allowed only the port is initialized.
*//***************************************************************************/
void fman_test_ip_manip (void *mac_dev, uint8_t *data);


#endif /* __FSL_FMAN_TEST_H */
