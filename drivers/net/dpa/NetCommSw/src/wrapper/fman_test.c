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

 @File          fman_test.c

 @Author        Moti Bar

 @Description   FM Linux test

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
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/fsl_qman.h>     /*struct qman_fq */
#include <linux/of_platform.h>
#include <linux/ip.h>
#include <linux/compat.h>	/*for mixed environment */
#include <linux/uaccess.h>
#include <linux/errno.h>

/* NetCommSw Headers --------------- */
#include "std_ext.h"
#include "error_ext.h"
#include "debug_ext.h"
#include "list_ext.h"
#include "fm_ext.h"

#include "fm_test_ioctls.h"
#include "fsl_fman.h"
#include "fsl_fman_test.h"
#include "fm_port_ext.h"


#define __ERR_MODULE__      MODULE_FM

#define FMT_FRM_WATERMARK   0xdeadbeefdeadbeeaLL

#ifdef CONFIG_COMPAT
/* Define this for KS64b - US32b */
#define FMAN_TEST_CONFIG_COMPAT
#warning Please make sure you have the right value for FMAN_TEST_CONFIG_COMPAT
#endif

typedef struct {
    ioc_fmt_buff_desc_t     buff;
    t_List                  node;
} t_FmTestFrame;

#define FMT_FRAME_OBJECT(ptr)  LIST_OBJECT(ptr, t_FmTestFrame, node)

typedef struct t_FmTestFq {
    struct qman_fq      fq_base;
    struct list_head    list;
    void                *port;
    bool                init;
} t_FmTestFq;

typedef struct {
    bool                valid;
    uint8_t             id;
    ioc_fmt_port_type   portType;
    ioc_diag_mode       diag;
    bool                echo;
    bool                ip_header_manip;
    struct fm_port      *p_TxPort;
    t_Handle            h_TxFmPortDev;
    struct fm_port      *p_RxPort;
    t_Handle            h_RxFmPortDev;
    t_Handle            h_Mac;
    uint64_t            fmPhysBaseAddr;
    t_List              rxFrmsQ;

    int                 numOfTxQs;
    struct qman_fq      *p_TxFqs[8];
} t_FmTestPort;

typedef struct {
    int major;
    t_FmTestPort ports[IOC_FMT_MAX_NUM_OF_PORTS];
    struct class *fm_test_class;
} t_FmTest;


static t_FmTest fmTest;


static t_Error Set1GMacIntLoopback(t_FmTestPort *p_FmTestPort, bool en)
{
#define FM_1GMAC0_OFFSET                0x000e0000
#define FM_1GMAC_CMD_CONF_CTRL_OFFSET   0x100
#define MACCFG1_LOOPBACK                0x00000100

    uint64_t    baseAddr, regAddr;
    uint32_t    tmpVal;

    baseAddr = p_FmTestPort->fmPhysBaseAddr + (FM_1GMAC0_OFFSET + (p_FmTestPort->id*0x2000));

    baseAddr = PTR_TO_UINT(ioremap(baseAddr, 0x1000));

    regAddr = baseAddr + FM_1GMAC_CMD_CONF_CTRL_OFFSET;
    tmpVal = GET_UINT32(*((uint32_t *)UINT_TO_PTR(regAddr)));
    if (en)
        tmpVal |= MACCFG1_LOOPBACK;
    else
        tmpVal &= ~MACCFG1_LOOPBACK;
    WRITE_UINT32(*((uint32_t *)UINT_TO_PTR(regAddr)), tmpVal);

    iounmap(UINT_TO_PTR(baseAddr));

    return E_OK;
}

#ifndef FM_10G_MAC_NO_CTRL_LOOPBACK
static t_Error Set10GMacIntLoopback(t_FmTestPort *p_FmTestPort, bool en)
{
#define FM_10GMAC0_OFFSET               0x000f0000
#define FM_10GMAC_CMD_CONF_CTRL_OFFSET  0x8
#define CMD_CFG_LOOPBACK_EN             0x00000400

    uint64_t    baseAddr, regAddr;
    uint32_t    tmpVal;

    baseAddr = p_FmTestPort->fmPhysBaseAddr + (FM_10GMAC0_OFFSET + ((p_FmTestPort->id-FM_MAX_NUM_OF_1G_RX_PORTS)*0x2000));

    baseAddr = PTR_TO_UINT(ioremap(baseAddr, 0x1000));

    regAddr = baseAddr + FM_10GMAC_CMD_CONF_CTRL_OFFSET;
    tmpVal = GET_UINT32(*((uint32_t *)UINT_TO_PTR(regAddr)));
    if (en)
        tmpVal |= CMD_CFG_LOOPBACK_EN;
    else
        tmpVal &= ~CMD_CFG_LOOPBACK_EN;
    WRITE_UINT32(*((uint32_t *)UINT_TO_PTR(regAddr)), tmpVal);

    iounmap(UINT_TO_PTR(baseAddr));

    return E_OK;
}
#endif /* !FM_10G_MAC_NO_CTRL_LOOPBACK */

static t_Error SetMacIntLoopback(t_FmTestPort *p_FmTestPort, bool en)
{

    if (p_FmTestPort->portType == e_IOC_FMT_PORT_T_RXTX)
    {
        if (p_FmTestPort->id < FM_MAX_NUM_OF_1G_RX_PORTS)
            return Set1GMacIntLoopback(p_FmTestPort, en);
        else if ((p_FmTestPort->id >= FM_MAX_NUM_OF_1G_RX_PORTS) && (p_FmTestPort->id < FM_MAX_NUM_OF_1G_RX_PORTS + FM_MAX_NUM_OF_10G_RX_PORTS))
#ifndef FM_10G_MAC_NO_CTRL_LOOPBACK
            return Set10GMacIntLoopback(p_FmTestPort, en);
#else
            RETURN_ERROR(MINOR, E_NOT_SUPPORTED, ("TGEC don't have internal-loopback"));
#endif /* !FM_10G_MAC_NO_CTRL_LOOPBACK */
    }
    else if (p_FmTestPort->portType == e_IOC_FMT_PORT_T_OP)
        return E_OK; /* no Mac loopback can be set on OH ports */
    else
        RETURN_ERROR(MINOR, E_INVALID_VALUE, ("fm-port-test id!"));
    RETURN_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
}

static void EnqueueFrameToRxQ(t_FmTestPort *p_FmTestPort, t_FmTestFrame *p_FmTestFrame)
{
    uint32_t   intFlags;

    intFlags = XX_DisableAllIntr();
    LIST_AddToTail(&p_FmTestFrame->node, &p_FmTestPort->rxFrmsQ);
    XX_RestoreAllIntr(intFlags);
}

static t_FmTestFrame * DequeueFrameFromRxQ(t_FmTestPort *p_FmTestPort)
{
    t_FmTestFrame   *p_FmTestFrame = NULL;
    uint32_t        intFlags;

    intFlags = XX_DisableAllIntr();
    if (!LIST_IsEmpty(&p_FmTestPort->rxFrmsQ))
    {
        p_FmTestFrame = FMT_FRAME_OBJECT(p_FmTestPort->rxFrmsQ.p_Next);
        LIST_DelAndInit(&p_FmTestFrame->node);
    }
    XX_RestoreAllIntr(intFlags);

    return p_FmTestFrame;
}

static enum qman_cb_dqrr_result egress_dqrr(struct qman_portal          *portal,
                                            struct qman_fq              *fq,
                                            const struct qm_dqrr_entry  *dq)
{
    WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__);
    return qman_cb_dqrr_consume;
}

static void egress_ern(struct qman_portal       *portal,
                       struct qman_fq           *fq,
                       const struct qm_mr_entry *msg)
{
    WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__);
}

static struct qman_fq * FqAlloc(t_FmTestPort    *p_FmTestPort,
                                uint32_t        fqid,
                                uint32_t        flags,
                                uint16_t        channel,
                                uint8_t         wq)
{
    int                     _errno;
    struct qman_fq          *fq = NULL;
    t_FmTestFq              *p_FmtFq;
    struct qm_mcc_initfq    initfq;

    p_FmtFq = (t_FmTestFq *)XX_Malloc(sizeof(t_FmTestFq));
    if (!p_FmtFq) {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj!!!"));
        return NULL;
    }

    p_FmtFq->fq_base.cb.dqrr = egress_dqrr;
    p_FmtFq->fq_base.cb.ern = p_FmtFq->fq_base.cb.dc_ern = p_FmtFq->fq_base.cb.fqs = egress_ern;
    p_FmtFq->port = (void *)p_FmTestPort;
    if (fqid == 0) {
        flags |= QMAN_FQ_FLAG_DYNAMIC_FQID;
        flags &= ~QMAN_FQ_FLAG_NO_MODIFY;
    } else {
        flags &= ~QMAN_FQ_FLAG_DYNAMIC_FQID;
    }

    p_FmtFq->init    = !(flags & QMAN_FQ_FLAG_NO_MODIFY);

    DBG(TRACE, ("fqid %d, flags 0x%08x, channel %d, wq %d",fqid,flags,channel,wq));

    _errno = qman_create_fq(fqid, flags, &p_FmtFq->fq_base);
    if (unlikely(_errno)) {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj - qman_new_fq!!!"));
        XX_Free(p_FmtFq);
        return NULL;
    }
    fq = &p_FmtFq->fq_base;

    if (p_FmtFq->init) {
        initfq.we_mask            = QM_INITFQ_WE_DESTWQ;
        initfq.fqd.dest.channel   = channel;
        initfq.fqd.dest.wq        = wq;

        _errno = qman_init_fq(fq, QMAN_INITFQ_FLAG_SCHED, &initfq);
        if (unlikely(_errno < 0)) {
            REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj - qman_init_fq!!!"));
            qman_destroy_fq(fq, 0);
            XX_Free(p_FmtFq);
            return NULL;
        }
    }

    return fq;
}

static t_Error PortInit (t_FmTestPort *p_FmTestPort, ioc_fmt_port_param_t *p_Params)
{
    struct device_node  *fm_node, *fm_port_node;
    const uint32_t      *uint32_prop;
    int                 _errno=0, lenp;
    uint32_t            i;
    static struct of_device_id fm_node_of_match[] = {
        { .compatible = "fsl,fman", },
        { /* end of list */ },
    };

    INIT_LIST(&p_FmTestPort->rxFrmsQ);
    p_FmTestPort->numOfTxQs         = p_Params->num_tx_queues;
    p_FmTestPort->id                = p_Params->fm_port_id;
    p_FmTestPort->portType          = p_Params->fm_port_type;
    p_FmTestPort->diag              = e_IOC_DIAG_MODE_NONE;
    p_FmTestPort->ip_header_manip   = FALSE;

    /* Get all the FM nodes */
    for_each_matching_node(fm_node, fm_node_of_match) {

        uint32_prop = (uint32_t *)of_get_property(fm_node, "cell-index", &lenp);
        if (unlikely(uint32_prop == NULL)) {
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", fm_node->full_name));
        }
        if (WARN_ON(lenp != sizeof(uint32_t)))
            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) invalid", fm_node->full_name));
        if (*uint32_prop == p_Params->fm_id) {
            struct resource     res;
            /* Get the FM address */
            _errno = of_address_to_resource(fm_node, 0, &res);
            if (unlikely(_errno < 0))
                RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));

            p_FmTestPort->fmPhysBaseAddr = res.start;

            for_each_child_of_node(fm_node, fm_port_node) {
                struct platform_device    *of_dev;

                uint32_prop = (uint32_t *)of_get_property(fm_port_node, "cell-index", &lenp);
                if (uint32_prop == NULL)
                    continue;

                if (of_device_is_compatible(fm_port_node, "fsl,fman-port-oh") &&
                    (p_FmTestPort->portType  == e_IOC_FMT_PORT_T_OP)) {
                    if (*uint32_prop == p_FmTestPort->id)
                    {
                        of_dev = of_find_device_by_node(fm_port_node);
                        if (unlikely(of_dev == NULL))
                            RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
                        p_FmTestPort->p_TxPort = fm_port_bind(&of_dev->dev);
                        p_FmTestPort->h_TxFmPortDev = (t_Handle)fm_port_get_handle(p_FmTestPort->p_TxPort);
                        p_FmTestPort->h_RxFmPortDev = p_FmTestPort->h_TxFmPortDev;
                        p_FmTestPort->h_Mac = NULL;
                        break;
                    }
                }
                else if ((*uint32_prop == p_FmTestPort->id) &&
                          p_FmTestPort->portType  == e_IOC_FMT_PORT_T_RXTX) {
                    of_dev = of_find_device_by_node(fm_port_node);
                    if (unlikely(of_dev == NULL))
                        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
                    if(of_device_is_compatible(fm_port_node, "fsl,fman-port-1g-tx"))
                    {
                        p_FmTestPort->p_TxPort = fm_port_bind(&of_dev->dev);
                        p_FmTestPort->h_TxFmPortDev = (t_Handle)fm_port_get_handle(p_FmTestPort->p_TxPort);
                    }
                    else if(of_device_is_compatible(fm_port_node, "fsl,fman-port-1g-rx"))
                    {
                        p_FmTestPort->p_RxPort = fm_port_bind(&of_dev->dev);
                        p_FmTestPort->h_RxFmPortDev = (t_Handle)fm_port_get_handle(p_FmTestPort->p_RxPort);
                    }
                    else if (of_device_is_compatible(fm_port_node, "fsl,fman-1g-mac"))
                        p_FmTestPort->h_Mac = (typeof(p_FmTestPort->h_Mac))dev_get_drvdata(&of_dev->dev);
                    else
                        continue;
                    if(p_FmTestPort->h_TxFmPortDev && p_FmTestPort->h_RxFmPortDev && p_FmTestPort->h_Mac)
                        break;
                }
                else if (((*uint32_prop + FM_MAX_NUM_OF_1G_RX_PORTS )== p_FmTestPort->id) &&
                          p_FmTestPort->portType  == e_IOC_FMT_PORT_T_RXTX) {
                    of_dev = of_find_device_by_node(fm_port_node);
                    if (unlikely(of_dev == NULL))
                        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
                    if(of_device_is_compatible(fm_port_node, "fsl,fman-port-10g-tx"))
                    {
                        p_FmTestPort->p_TxPort = fm_port_bind(&of_dev->dev);
                        p_FmTestPort->h_TxFmPortDev = (t_Handle)fm_port_get_handle(p_FmTestPort->p_TxPort);
                    }
                    else if(of_device_is_compatible(fm_port_node, "fsl,fman-port-10g-rx"))
                    {
                        p_FmTestPort->p_RxPort = fm_port_bind(&of_dev->dev);
                        p_FmTestPort->h_RxFmPortDev = (t_Handle)fm_port_get_handle(p_FmTestPort->p_RxPort);
                    }
                    else if (of_device_is_compatible(fm_port_node, "fsl,fman-10g-mac"))
                        p_FmTestPort->h_Mac = (typeof(p_FmTestPort->h_Mac))dev_get_drvdata(&of_dev->dev);
                    else
                        continue;
                    if(p_FmTestPort->h_TxFmPortDev && p_FmTestPort->h_RxFmPortDev && p_FmTestPort->h_Mac)
                        break;
                }
            } //for_each_child
        }
    } //for each matching node

    DBG(TRACE, ("h_TxFmPortDev - 0x%08x, h_RxFmPortDev - 0x%08x, h_Mac - 0x%08x\n",
        p_FmTestPort->h_TxFmPortDev,p_FmTestPort->h_RxFmPortDev,p_FmTestPort->h_Mac));

    if(p_FmTestPort->h_TxFmPortDev == 0 || p_FmTestPort->h_RxFmPortDev == 0)
        RETURN_ERROR(MINOR, E_INVALID_ADDRESS, ("Bad pointers!"));

    //init Queues
    for (i=0; i<p_FmTestPort->numOfTxQs; i++) {
        p_FmTestPort->p_TxFqs[i] =
            FqAlloc(p_FmTestPort,
                    0,
                    QMAN_FQ_FLAG_TO_DCPORTAL,
                    fm_get_tx_port_channel(p_FmTestPort->p_TxPort),
                    i);
        if (IS_ERR(p_FmTestPort->p_TxFqs[i]))
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("Tx FQs!"));
    }

    p_FmTestPort->valid     = TRUE;

    return E_OK;
}


bool is_fman_test (void     *mac_dev,
                   uint32_t queueId,
                   uint8_t  *buffer,
                   uint32_t size)
{
    t_FmTest                *p_FmTest = &fmTest;
    t_FmTestPort            *p_FmTestPort=NULL;
    t_FmTestFrame           *p_FmTestFrame;
    uint32_t                count=size-7;
    uint64_t                temp;
    uint8_t                 *temp_buf=buffer, i;
    bool                    fman_test_flag = false;
    uint32_t                dataOffset;
    struct fm_port          *fm_port = (struct fm_port *) mac_dev;

#if 0
    if ((queueId == FMT_TX_CONF_Q) || (queueId == FMT_TX_ERR_Q))
    {
        /* Check for the FM-test-port object existence, otherwise we make wrong frees all the time */
        for (i=0; i<IOC_FMT_MAX_NUM_OF_PORTS; i++)
            if (mac_dev == p_FmTest->ports[i].h_Mac)
                p_FmTestPort = &p_FmTest->ports[i];
        if (!p_FmTestPort)
            return false;

        if (buffer != NULL)
            XX_Free(buffer);
        return true;
    }
#endif

    /* Get the FM-test-port object */
    for (i=0; i<IOC_FMT_MAX_NUM_OF_PORTS; i++)
        if ((p_FmTest->ports[i].h_Mac && mac_dev == p_FmTest->ports[i].h_Mac) ||
                fm_port == p_FmTest->ports[i].p_TxPort)
        {
            p_FmTestPort = &p_FmTest->ports[i];
            break;
        }
    if (!p_FmTestPort)
        return false;

    /* Check according to watermark if this frame is for FM-test */
    while(count--)
    {
        temp = *(uint64_t *)temp_buf;
        if (temp == FMT_FRM_WATERMARK)
        {
            fman_test_flag = true;
            break;
        }
        temp_buf++;
    }

    if (fman_test_flag /*|| p_FmTestPort->echo*/)
    {
        if ((queueId == FMT_TX_CONF_Q) || (queueId == FMT_TX_ERR_Q))
        {
            if (buffer != NULL)
                XX_Free(buffer);
            return true;
        }

        DBG(TRACE, ("Port %d got FMUC frame\n", p_FmTestPort->id));
        dataOffset = FM_PORT_GetBufferDataOffset(p_FmTestPort->h_RxFmPortDev);

        p_FmTestFrame = (t_FmTestFrame *)XX_Malloc(sizeof(t_FmTestFrame));
        /* dump frame... no more space left on device */
        if(p_FmTestFrame == NULL)
            return false;

        memset(p_FmTestFrame, 0, sizeof(t_FmTestFrame));
        INIT_LIST(&p_FmTestFrame->node);

        p_FmTestFrame->buff.p_data = (uint8_t *)XX_Malloc(size * sizeof(uint8_t));
        /* No more space left on device*/
        if(p_FmTestFrame->buff.p_data == NULL){
            XX_Free(p_FmTestFrame);
            return false;
        }

        p_FmTestFrame->buff.size = size-dataOffset;
        p_FmTestFrame->buff.qid = queueId;

        memcpy(p_FmTestFrame->buff.p_data,
               (uint8_t *)PTR_MOVE(buffer, dataOffset),
               p_FmTestFrame->buff.size);

        memcpy(p_FmTestFrame->buff.buff_context.fm_prs_res,
               FM_PORT_GetBufferPrsResult(p_FmTestPort->h_RxFmPortDev, (char*)buffer),
               32);

        EnqueueFrameToRxQ(p_FmTestPort, p_FmTestFrame);
        return true;
    }

    return false;
}

void fman_test_ip_manip (void *mac_dev, uint8_t *data)
{
    t_FmTest                *p_FmTest = &fmTest;
    t_FmTestPort            *p_FmTestPort=NULL;
    struct iphdr            *iph;
    uint32_t                *p_Data = (uint32_t *)data;
    uint32_t                net;
    uint32_t                saddr, daddr;
    uint8_t                 i;

    /* Get the FM-test-port object */
    for (i=0; i<IOC_FMT_MAX_NUM_OF_PORTS; i++)
        if (mac_dev == p_FmTest->ports[i].h_Mac)
            p_FmTestPort = &p_FmTest->ports[i];
    if (!p_FmTestPort || !p_FmTestPort->ip_header_manip)
        return;

    iph = (struct iphdr *)p_Data;
    saddr = iph->saddr;
    daddr = iph->daddr;

    /* If it is ARP packet ... */
    if (*p_Data == 0x00010800)
    {
        saddr = *((uint32_t *)PTR_MOVE(p_Data, 14));
        daddr = *((uint32_t *)PTR_MOVE(p_Data, 24));
    }

    DBG(TRACE,
        ("\nSrc  IP before header-manipulation: %d.%d.%d.%d"
         "\nDest IP before header-manipulation: %d.%d.%d.%d",
         (int)((saddr & 0xff000000) >> 24),
         (int)((saddr & 0x00ff0000) >> 16),
         (int)((saddr & 0x0000ff00) >> 8),
         (int)((saddr & 0x000000ff) >> 0),
         (int)((daddr & 0xff000000) >> 24),
         (int)((daddr & 0x00ff0000) >> 16),
         (int)((daddr & 0x0000ff00) >> 8),
         (int)((daddr & 0x000000ff) >> 0)));

    if ((p_FmTestPort->diag == e_IOC_DIAG_MODE_CTRL_LOOPBACK) ||
        (p_FmTestPort->diag == e_IOC_DIAG_MODE_CHIP_LOOPBACK) ||
        (p_FmTestPort->diag == e_IOC_DIAG_MODE_PHY_LOOPBACK) ||
        (p_FmTestPort->diag == e_IOC_DIAG_MODE_LINE_LOOPBACK))
    {
        net   = saddr;
        saddr = daddr;
        daddr = net;
    }
    else
    {
        /* We allow only up to 10 eth ports */
        net   = ((daddr & 0x000000ff) % 10);
        saddr = (uint32_t)((saddr & ~0x0000ff00) | (net << 8));
        daddr = (uint32_t)((daddr & ~0x0000ff00) | (net << 8));
    }

    /* If not ARP ... */
    if (*p_Data != 0x00010800)
    {
        iph->check = 0;

        iph->saddr = saddr;
        iph->daddr = daddr;
        iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
    }
    else /* The packet is ARP */
    {
        *((uint32_t *)PTR_MOVE(p_Data, 14)) = saddr;
        *((uint32_t *)PTR_MOVE(p_Data, 24)) = daddr;
    }

    DBG(TRACE,
        ("\nSrc  IP after  header-manipulation: %d.%d.%d.%d"
         "\nDest IP after  header-manipulation: %d.%d.%d.%d",
         (int)((saddr & 0xff000000) >> 24),
         (int)((saddr & 0x00ff0000) >> 16),
         (int)((saddr & 0x0000ff00) >> 8),
         (int)((saddr & 0x000000ff) >> 0),
         (int)((daddr & 0xff000000) >> 24),
         (int)((daddr & 0x00ff0000) >> 16),
         (int)((daddr & 0x0000ff00) >> 8),
         (int)((daddr & 0x000000ff) >> 0)));
}


/*****************************************************************************/
/*               API routines for the FM Linux Device                        */
/*****************************************************************************/

static int fm_test_open(struct inode *inode, struct file *file)
{
    t_FmTest            *p_FmTest = &fmTest;
    //unsigned int        major = imajor(inode);
    unsigned int        minor = iminor(inode);

    DBG(TRACE, ("Opening minor - %d - ", minor));

    if (file->private_data != NULL)
        return 0;

    if ((minor >= DEV_FM_TEST_PORTS_MINOR_BASE) &&
        (minor < DEV_FM_TEST_MAX_MINORS))
        file->private_data = &p_FmTest->ports[minor];
    else
        return -ENXIO;

    return 0;
}

static int fm_test_close(struct inode *inode, struct file *file)
{
    t_FmTestPort        *p_FmTestPort;
    unsigned int        minor = iminor(inode);
    int                 err = 0;

    DBG(TRACE, ("Closing minor - %d - ", minor));

    p_FmTestPort = file->private_data;
    if (!p_FmTestPort)
        return -ENODEV;

    p_FmTestPort->valid = FALSE;

    /* Complete!!! */
    return err;
}

static int fm_test_ioctls(unsigned int minor, struct file *file, unsigned int cmd, unsigned long arg, bool compat)
{
    t_FmTestPort        *p_FmTestPort;

    DBG(TRACE, ("IOCTL minor - %d, cmd - 0x%08x, arg - 0x%08x", minor, cmd, arg));

    p_FmTestPort = file->private_data;
    if (!p_FmTestPort)
        return -ENODEV;

    switch (cmd)
    {
        case FMT_PORT_IOC_INIT:
        {
            ioc_fmt_port_param_t  param;

            if (p_FmTestPort->valid) {
                REPORT_ERROR(MINOR, E_INVALID_STATE, ("port is already initialized!!!"));
                return -EFAULT;
            }
#if defined(FMAN_TEST_CONFIG_COMPAT)
            if (compat){
                if (copy_from_user(&param, (ioc_fmt_port_param_t *)compat_ptr(arg), sizeof(ioc_fmt_port_param_t))) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }
#endif
            else{
                if (copy_from_user(&param, (ioc_fmt_port_param_t *) arg, sizeof(ioc_fmt_port_param_t))) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }

            return PortInit(p_FmTestPort, &param);
        }

        case FMT_PORT_IOC_SET_DIAG_MODE:
        {
            if (get_user(p_FmTestPort->diag, (ioc_diag_mode *)arg)) {
                REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                return -EFAULT;
            }

            if (p_FmTestPort->diag == e_IOC_DIAG_MODE_CTRL_LOOPBACK)
                return SetMacIntLoopback(p_FmTestPort, TRUE);
            else
                return SetMacIntLoopback(p_FmTestPort, FALSE);
            break;
        }

        case FMT_PORT_IOC_SET_DPAECHO_MODE:
        {
#if defined(FMAN_TEST_CONFIG_COMPAT)
            if (compat){
                if (get_user(p_FmTestPort->echo, (int *)compat_ptr(arg))) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }
            else
#endif
            {
                if (get_user(p_FmTestPort->echo, (int *)arg)) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }
            break;
        }

        case FMT_PORT_IOC_SET_IP_HEADER_MANIP:
        {
#if defined(FMAN_TEST_CONFIG_COMPAT)
            if (compat){
                if (get_user(p_FmTestPort->ip_header_manip, (int *)compat_ptr(arg))) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }
            else
#endif
            {
                if (get_user(p_FmTestPort->ip_header_manip, (int *)arg)) {
                    REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                    return -EFAULT;
                }
            }
            break;
        }

        default:
            REPORT_ERROR(MINOR, E_INVALID_SELECTION, ("IOCTL TEST cmd (0x%08lx):(0x%02lx:0x%02lx)!", cmd, _IOC_TYPE(cmd), _IOC_NR(cmd)));
            return -EFAULT;
    }

    return 0;
}

#ifdef FMAN_TEST_CONFIG_COMPAT
static long fm_test_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int minor = iminor(file->f_path.dentry->d_inode);

    return fm_test_ioctls(minor, file, cmd, arg, true);
}
#endif

static long fm_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int        minor = iminor(file->f_path.dentry->d_inode);
    unsigned int        res;

    fm_mutex_lock();
    res =  fm_test_ioctls(minor, file, cmd, arg, false);
    fm_mutex_unlock();

    return res;
}

#ifdef FMAN_TEST_CONFIG_COMPAT
void copy_compat_test_frame_buffer(ioc_fmt_buff_desc_t *buff, ioc_fmt_compat_buff_desc_t *compat_buff)
{
    compat_buff->qid = buff->qid;
    compat_buff->p_data = ptr_to_compat(buff->p_data);
    compat_buff->size = buff->size;
    compat_buff->status = buff->status;

    compat_buff->buff_context.p_user_priv = ptr_to_compat(buff->buff_context.p_user_priv);
    memcpy(compat_buff->buff_context.fm_prs_res,buff->buff_context.fm_prs_res,FM_PRS_MAX * sizeof(uint8_t));
    memcpy(compat_buff->buff_context.fm_time_stamp,buff->buff_context.fm_time_stamp,FM_TIME_STAMP_MAX * sizeof(uint8_t));
}
#endif

ssize_t fm_test_read (struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    t_FmTestPort        *p_FmTestPort;
    t_FmTestFrame       *p_FmTestFrame;
    ssize_t             cnt = 0;

    p_FmTestPort = file->private_data;
    if (!p_FmTestPort || !p_FmTestPort->valid)
        return -ENODEV;

    p_FmTestFrame = DequeueFrameFromRxQ(p_FmTestPort);
    if (!p_FmTestFrame)
        return 0;

    if (!p_FmTestPort->echo) {
#ifdef FMAN_TEST_CONFIG_COMPAT
        cnt = sizeof(ioc_fmt_compat_buff_desc_t);
#else
        cnt = sizeof(ioc_fmt_buff_desc_t);
#endif
        if (size<cnt) {
            XX_Free(p_FmTestFrame->buff.p_data);
            XX_Free(p_FmTestFrame);
            REPORT_ERROR(MINOR, E_NO_MEMORY, ("Illegal buffer-size!"));
            return 0;
        }

        /* Copy structure */
#ifdef FMAN_TEST_CONFIG_COMPAT
        {
            ioc_fmt_compat_buff_desc_t compat_buff;
            copy_compat_test_frame_buffer(&p_FmTestFrame->buff, &compat_buff);
            
            if (copy_to_user(buf, &compat_buff, cnt)) {
                XX_Free(p_FmTestFrame->buff.p_data);
                XX_Free(p_FmTestFrame);
                REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
                return 0;
            }
        }

       ((ioc_fmt_compat_buff_desc_t *)buf)->p_data = ptr_to_compat(buf+sizeof(ioc_fmt_compat_buff_desc_t));
        cnt += MIN(p_FmTestFrame->buff.size, size-cnt);
#else
        if (copy_to_user(buf, &p_FmTestFrame->buff, cnt)) {
            XX_Free(p_FmTestFrame->buff.p_data);
            XX_Free(p_FmTestFrame);
            REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
            return 0;
        }

        ((ioc_fmt_buff_desc_t *)buf)->p_data = buf+sizeof(ioc_fmt_buff_desc_t);
        cnt += MIN(p_FmTestFrame->buff.size, size-cnt);
#endif

        if (size<cnt) {
            XX_Free(p_FmTestFrame->buff.p_data);
            XX_Free(p_FmTestFrame);
            REPORT_ERROR(MINOR, E_NO_MEMORY, ("Illegal buffer-size!"));
            return 0;
        }

        /* copy frame */
#ifdef FMAN_TEST_CONFIG_COMPAT
        if (copy_to_user(buf+sizeof(ioc_fmt_compat_buff_desc_t), p_FmTestFrame->buff.p_data, cnt)) {
            XX_Free(p_FmTestFrame->buff.p_data);
            XX_Free(p_FmTestFrame);
            REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
            return 0;
        }
#else
        if (copy_to_user(buf+sizeof(ioc_fmt_buff_desc_t), p_FmTestFrame->buff.p_data, cnt)) {
            XX_Free(p_FmTestFrame->buff.p_data);
            XX_Free(p_FmTestFrame);
            REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
            return 0;
        }
#endif

        XX_Free(p_FmTestFrame->buff.p_data);
        XX_Free(p_FmTestFrame);
    }
    else {
        uint8_t  *p_Data = NULL;
        uint32_t frameSize =0;
        t_DpaaFD fd;
        int      _errno = 0;

        p_Data = (uint8_t *)p_FmTestFrame->buff.p_data;
        frameSize = p_FmTestFrame->buff.size;

        /* paranoia ...*/
        if(!p_Data){
            XX_Free(p_FmTestFrame);
            return 0;
        }

        /* Set frame descriptor... */
        memset(&fd, 0, sizeof(fd));
        DPAA_FD_SET_ADDR(&fd, p_Data);
        DPAA_FD_SET_OFFSET(&fd, 0);
        DPAA_FD_SET_LENGTH(&fd, frameSize);

        /* Enqueue frame... */
        _errno = qman_enqueue(p_FmTestPort->p_TxFqs[0], (struct qm_fd*)&fd, 0);
        if (_errno) {
            XX_Free(p_FmTestFrame);
            XX_Free(p_Data);
            return 0;
        }

        XX_Free(p_FmTestFrame);
    }

    return cnt;
}

ssize_t fm_test_write (struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    t_FmTestPort        *p_FmTestPort;

    ioc_fmt_buff_desc_t buffDesc;
#ifdef FMAN_TEST_CONFIG_COMPAT
    ioc_fmt_compat_buff_desc_t compatBuffDesc;
#endif
    t_DpaaFD            fd;
    uint8_t             *p_Data;
    uint32_t            dataOffset;
    int                 _errno;

    p_FmTestPort = file->private_data;
    if (!p_FmTestPort || !p_FmTestPort->valid) {
        REPORT_ERROR(MINOR, E_INVALID_HANDLE, NO_MSG);
        return -1;
    }

    /* If Compat (32B UserSpace - 64B KernelSpace)  */
#ifdef FMAN_TEST_CONFIG_COMPAT
    if (copy_from_user(&compatBuffDesc, buf, sizeof(ioc_fmt_compat_buff_desc_t))) {
        REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
        return -1;
    }

    buffDesc.qid = compatBuffDesc.qid;
    buffDesc.p_data = compat_ptr(compatBuffDesc.p_data);
    buffDesc.size = compatBuffDesc.size;
    buffDesc.status = compatBuffDesc.status;

    buffDesc.buff_context.p_user_priv = compat_ptr(compatBuffDesc.buff_context.p_user_priv);
    memcpy(buffDesc.buff_context.fm_prs_res, compatBuffDesc.buff_context.fm_prs_res, FM_PRS_MAX * sizeof(uint8_t));
    memcpy(buffDesc.buff_context.fm_time_stamp, compatBuffDesc.buff_context.fm_time_stamp, FM_TIME_STAMP_MAX * sizeof(uint8_t));
#else
    if (copy_from_user(&buffDesc, (ioc_fmt_buff_desc_t *)buf, sizeof(ioc_fmt_buff_desc_t))) {
        REPORT_ERROR(MINOR, E_INVALID_STATE, NO_MSG);
        return -1;
    }
#endif

    dataOffset = FM_PORT_GetBufferDataOffset(p_FmTestPort->h_TxFmPortDev);
    p_Data = (uint8_t*)XX_Malloc(buffDesc.size+dataOffset);
    if (!p_Data) {
        REPORT_ERROR(MINOR, E_NO_MEMORY, ("data buff!"));
        return -1;
    }

    /* If Compat (32UserSpace - 64KernelSpace) the buffDesc.p_data is ok */
    if (copy_from_user ((uint8_t *)PTR_MOVE(p_Data, dataOffset),
                        buffDesc.p_data,
                        buffDesc.size)) {
        REPORT_ERROR(MINOR, E_NO_MEMORY, ("data buff!"));
        XX_Free(p_Data);
        return -1;
    }

    memset(&fd, 0, sizeof(fd));
    DPAA_FD_SET_ADDR(&fd, p_Data);
    DPAA_FD_SET_OFFSET(&fd, dataOffset);
    DPAA_FD_SET_LENGTH(&fd, buffDesc.size);

    DBG(TRACE, ("buffDesc qId %d, fqid %d, frame len %d, fq 0x%8x\n",
                buffDesc.qid, qman_fq_fqid(p_FmTestPort->p_TxFqs[buffDesc.qid]), buffDesc.size, p_FmTestPort->p_TxFqs[buffDesc.qid]));

    _errno = qman_enqueue(p_FmTestPort->p_TxFqs[buffDesc.qid], (struct qm_fd*)&fd, 0);
    if (_errno) {
        buffDesc.status = (uint32_t)_errno;
        if (copy_to_user((ioc_fmt_buff_desc_t*)buf, &buffDesc, sizeof(ioc_fmt_buff_desc_t))) {
            REPORT_ERROR(MINOR, E_WRITE_FAILED, NO_MSG);
            XX_Free(p_Data);
            return -1;
        }
    }
    return buffDesc.size;
}

/* Globals for FM character device */
static struct file_operations fm_test_fops =
{
    owner:      THIS_MODULE,
#ifdef FMAN_TEST_CONFIG_COMPAT
    compat_ioctl:fm_test_compat_ioctl,
#endif
    unlocked_ioctl:      fm_test_ioctl,
    open:       fm_test_open,
    release:    fm_test_close,
    read:       fm_test_read,
    write:      fm_test_write,
};

t_Handle LNXWRP_FM_TEST_Init(void)
{
    t_FmTest    *p_FmTest = &fmTest;
    int id;

    /* Register to the /dev for IOCTL API */
    /* Register dynamically a new major number for the character device: */
    if ((p_FmTest->major = register_chrdev(0, DEV_FM_TEST_NAME, &fm_test_fops)) <= 0)
    {
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Failed to allocate a major number for device \"%s\"", DEV_FM_TEST_NAME));
        return NULL;
    }

    /* Creating class for FMan_test */
    DBG(TRACE ,("class_create fm_test_class"));
    p_FmTest->fm_test_class = class_create(THIS_MODULE, DEV_FM_TEST_NAME);
    if (IS_ERR(p_FmTest->fm_test_class)) {
        unregister_chrdev(p_FmTest->major, DEV_FM_TEST_NAME);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("class_create error fm_test_class"));
        return NULL;
    }

    for (id = 0; id < IOC_FMT_MAX_NUM_OF_PORTS; id++)
        if(NULL == device_create(p_FmTest->fm_test_class, NULL, MKDEV(p_FmTest->major, DEV_FM_TEST_PORTS_MINOR_BASE + id), NULL,
                DEV_FM_TEST_NAME "%d", id))
        {
            REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Error creating device.\n"));
            return NULL;
        }

    /* Seed the QMan allocator so we'll have enough queues to run PCD with
       dinamically fqid-range allocation */
    qman_release_fqid_range(0x100, 0x400);

    return p_FmTest;
}

t_Error  LNXWRP_FM_TEST_Free(t_Handle h_FmTestLnxWrp)
{
    t_FmTest    *p_FmTest = (t_FmTest*)h_FmTestLnxWrp;
    int id;

    DBG(TRACE, ("destroy fm_test_class"));
    for (id = 0; id < IOC_FMT_MAX_NUM_OF_PORTS; id++)
        device_destroy(p_FmTest->fm_test_class, MKDEV(p_FmTest->major, DEV_FM_TEST_PORTS_MINOR_BASE + id));
    class_destroy(p_FmTest->fm_test_class);

    return E_OK;
}

static t_Handle h_FmTestLnxWrp;

static int __init __cold fm_test_load (void)
{
    if ((h_FmTestLnxWrp = LNXWRP_FM_TEST_Init()) == NULL)
    {
        printk("Failed to init FM-test wrapper!\n");
        if (h_FmTestLnxWrp)
            LNXWRP_FM_TEST_Free(h_FmTestLnxWrp);
        return -ENODEV;
    }

    printk (KERN_CRIT "Freescale FM test module ("__DATE__ ":"__TIME__")\n");

    return 0;
}

static void __exit __cold fm_test_unload (void)
{
    if (h_FmTestLnxWrp)
        LNXWRP_FM_TEST_Free(h_FmTestLnxWrp);
}

module_init (fm_test_load);
module_exit (fm_test_unload);
