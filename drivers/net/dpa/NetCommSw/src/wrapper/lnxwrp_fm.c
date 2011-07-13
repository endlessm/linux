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

 @File          lnxwrp_fm.c

 @Author        Shlomi Gridish

 @Description   FM Linux wrapper functions.

*/

#include <linux/version.h>
#include <linux/slab.h>
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
#include <asm/qe.h>        /* For struct qe_firmware */
#include <sysdev/fsl_soc.h>
#include <linux/stat.h>	   /* For file access mask */
#include <linux/skbuff.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

/* NetCommSw Headers --------------- */
#include "std_ext.h"
#include "error_ext.h"
#include "sprint_ext.h"
#include "debug_ext.h"
#include "sys_io_ext.h"

#include "fm_ioctls.h"

#include "lnxwrp_fm.h"
#include "lnxwrp_resources.h"
#include "lnxwrp_sysfs_fm.h"
#include "lnxwrp_sysfs_fm_port.h"

#define PROC_PRINT(args...) offset += sprintf(buf+offset,args)

#define ADD_ADV_CONFIG_NO_RET(_func, _param)    \
    do {                                        \
        if (i<max){                             \
            p_Entry = &p_Entrys[i];             \
            p_Entry->p_Function = _func;        \
            _param                              \
            i++;                                \
        }                                       \
        else                                    \
            REPORT_ERROR(MAJOR, E_INVALID_VALUE,\
                         ("Number of advanced-configuration entries exceeded"));\
    } while (0)

static t_LnxWrpFm   lnxWrpFm;


static irqreturn_t fm_irq(int irq, void *_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)_dev;

    if (!p_LnxWrpFmDev || !p_LnxWrpFmDev->h_Dev)
        return IRQ_NONE;

    FM_EventIsr(p_LnxWrpFmDev->h_Dev);

    return IRQ_HANDLED;
}

static irqreturn_t fm_err_irq(int irq, void *_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)_dev;

    if (!p_LnxWrpFmDev || !p_LnxWrpFmDev->h_Dev)
        return IRQ_NONE;

    if (FM_ErrorIsr(p_LnxWrpFmDev->h_Dev) == E_OK)
        return IRQ_HANDLED;

    return IRQ_NONE;
}

/* used to protect FMD/LLD from concurrent calls in functions fm_mutex_lock / fm_mutex_unlock */
static struct mutex   lnxwrp_mutex;

static t_LnxWrpFmDev * CreateFmDev(uint8_t  id)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;
    int             j;

    p_LnxWrpFmDev = (t_LnxWrpFmDev *)XX_Malloc(sizeof(t_LnxWrpFmDev));
    if (!p_LnxWrpFmDev)
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
        return NULL;
    }

    memset(p_LnxWrpFmDev, 0, sizeof(t_LnxWrpFmDev));
    p_LnxWrpFmDev->fmDevSettings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->fmDevSettings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    p_LnxWrpFmDev->fmPcdDevSettings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->fmPcdDevSettings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    p_LnxWrpFmDev->hcPort.settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
    memset(p_LnxWrpFmDev->hcPort.settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    for (j=0; j<FM_MAX_NUM_OF_RX_PORTS; j++)
    {
        p_LnxWrpFmDev->rxPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->rxPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }
    for (j=0; j<FM_MAX_NUM_OF_TX_PORTS; j++)
    {
        p_LnxWrpFmDev->txPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->txPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }
    for (j=0; j<FM_MAX_NUM_OF_OH_PORTS-1; j++)
    {
        p_LnxWrpFmDev->opPorts[j].settings.advConfig = (t_SysObjectAdvConfigEntry*)XX_Malloc(FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry));
        memset(p_LnxWrpFmDev->opPorts[j].settings.advConfig, 0, (FM_MAX_NUM_OF_ADV_SETTINGS*sizeof(t_SysObjectAdvConfigEntry)));
    }

    return p_LnxWrpFmDev;
}

static void DestroyFmDev(t_LnxWrpFmDev *p_LnxWrpFmDev)
{
    int             j;

    for (j=0; j<FM_MAX_NUM_OF_OH_PORTS-1; j++)
        if (p_LnxWrpFmDev->opPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->opPorts[j].settings.advConfig);
    for (j=0; j<FM_MAX_NUM_OF_TX_PORTS; j++)
        if (p_LnxWrpFmDev->txPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->txPorts[j].settings.advConfig);
    for (j=0; j<FM_MAX_NUM_OF_RX_PORTS; j++)
        if (p_LnxWrpFmDev->rxPorts[j].settings.advConfig)
            XX_Free(p_LnxWrpFmDev->rxPorts[j].settings.advConfig);
    if (p_LnxWrpFmDev->hcPort.settings.advConfig)
        XX_Free(p_LnxWrpFmDev->hcPort.settings.advConfig);
    if (p_LnxWrpFmDev->fmPcdDevSettings.advConfig)
        XX_Free(p_LnxWrpFmDev->fmPcdDevSettings.advConfig);
    if (p_LnxWrpFmDev->fmDevSettings.advConfig)
        XX_Free(p_LnxWrpFmDev->fmDevSettings.advConfig);

    XX_Free(p_LnxWrpFmDev);
}

static t_Error FillRestFmInfo(t_LnxWrpFmDev *p_LnxWrpFmDev)
{
#define FM_BMI_PPIDS_OFFSET                 0x00080304
#define FM_DMA_PLR_OFFSET                   0x000c2060
#define FM_FPM_IP_REV_1_OFFSET              0x000c30c4
#define DMA_HIGH_LIODN_MASK                 0x0FFF0000
#define DMA_LOW_LIODN_MASK                  0x00000FFF
#define DMA_LIODN_SHIFT                     16

typedef _Packed struct {
    uint32_t    plr[32];
} _PackedType t_Plr;

typedef _Packed struct {
   volatile uint32_t   fmbm_ppid[63];
} _PackedType t_Ppids;

    t_Plr       *p_Plr;
    t_Ppids     *p_Ppids;
    int         i,j;
    uint32_t    fmRev;

    static const uint8_t     phys1GRxPortId[] = {0x8,0x9,0xa,0xb,0xc};
    static const uint8_t     phys10GRxPortId[] = {0x10};
    static const uint8_t     physOhPortId[] = {0x1,0x2,0x3,0x4,0x5,0x6,0x7};
    static const uint8_t     phys1GTxPortId[] = {0x28,0x29,0x2a,0x2b,0x2c};
    static const uint8_t     phys10GTxPortId[] = {0x30};

    fmRev = (uint32_t)(*((volatile uint32_t *)UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr+FM_FPM_IP_REV_1_OFFSET)));
    fmRev &= 0xffff;

    p_Plr = (t_Plr *)UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr+FM_DMA_PLR_OFFSET);
#ifdef MODULE
    for (i=0;i<FM_MAX_NUM_OF_PARTITIONS/2;i++)
        p_Plr->plr[i] = 0;
#endif /* MODULE */

    for (i=0; i<FM_MAX_NUM_OF_PARTITIONS; i++)
    {
        uint16_t liodnBase = (uint16_t)((i%2) ?
                       (p_Plr->plr[i/2] & DMA_LOW_LIODN_MASK) :
                       ((p_Plr->plr[i/2] & DMA_HIGH_LIODN_MASK) >> DMA_LIODN_SHIFT));
#ifdef FM_PARTITION_ARRAY
        /* TODO: this was .liodnPerPartition[i] = liodnBase; is the index meaning the same? */
        p_LnxWrpFmDev->fmDevSettings.param.liodnBasePerPort[i] = liodnBase;
#endif /* FM_PARTITION_ARRAY */

        if ((i >= phys1GRxPortId[0]) &&
             (i <= phys1GRxPortId[FM_MAX_NUM_OF_1G_RX_PORTS-1]))
        {
            for (j=0; j<ARRAY_SIZE(phys1GRxPortId); j++)
                if (phys1GRxPortId[j] == i)
                    break;
            ASSERT_COND(j<ARRAY_SIZE(phys1GRxPortId));
            p_LnxWrpFmDev->rxPorts[j].settings.param.liodnBase = liodnBase;
        }
        else if (FM_MAX_NUM_OF_10G_RX_PORTS &&
                 (i >= phys10GRxPortId[0]) &&
                 (i <= phys10GRxPortId[FM_MAX_NUM_OF_10G_RX_PORTS-1]))
        {
            for (j=0; j<ARRAY_SIZE(phys10GRxPortId); j++)
                if (phys10GRxPortId[j] == i)
                    break;
            ASSERT_COND(j<ARRAY_SIZE(phys10GRxPortId));
            p_LnxWrpFmDev->rxPorts[FM_MAX_NUM_OF_1G_RX_PORTS+j].settings.param.liodnBase = liodnBase;
        }
        else if ((i >= physOhPortId[0]) &&
                 (i <= physOhPortId[FM_MAX_NUM_OF_OH_PORTS-1]))
        {
            for (j=0; j<ARRAY_SIZE(physOhPortId); j++)
                if (physOhPortId[j] == i)
                    break;
            ASSERT_COND(j<ARRAY_SIZE(physOhPortId));
            if (j == 0)
                p_LnxWrpFmDev->hcPort.settings.param.liodnBase = liodnBase;
            else
                p_LnxWrpFmDev->opPorts[j - 1].settings.param.liodnBase = liodnBase;
        }
        else if ((i >= phys1GTxPortId[0]) &&
                  (i <= phys1GTxPortId[FM_MAX_NUM_OF_1G_TX_PORTS-1]))
        {
            for (j=0; j<ARRAY_SIZE(phys1GTxPortId); j++)
                if (phys1GTxPortId[j] == i)
                    break;
            ASSERT_COND(j<ARRAY_SIZE(phys1GTxPortId));
            p_LnxWrpFmDev->txPorts[j].settings.param.liodnBase = liodnBase;
        }
        else if (FM_MAX_NUM_OF_10G_TX_PORTS &&
                 (i >= phys10GTxPortId[0]) &&
                 (i <= phys10GTxPortId[FM_MAX_NUM_OF_10G_TX_PORTS-1]))
        {
            for (j=0; j<ARRAY_SIZE(phys10GTxPortId); j++)
                if (phys10GTxPortId[j] == i)
                    break;
            ASSERT_COND(j<ARRAY_SIZE(phys10GTxPortId));
            p_LnxWrpFmDev->txPorts[FM_MAX_NUM_OF_1G_TX_PORTS+j].settings.param.liodnBase = liodnBase;
        }
    }

    p_Ppids = (t_Ppids *)UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr+FM_BMI_PPIDS_OFFSET);

    for (i=0; i<FM_MAX_NUM_OF_1G_RX_PORTS; i++)
        /* TODO: this was .rxPartitionId = ; liodnOffset seems to have the same meaning */
        p_LnxWrpFmDev->rxPorts[i].settings.param.specificParams.rxParams.liodnOffset =
                p_Ppids->fmbm_ppid[phys1GRxPortId[i]-1];

    for (i=0; i<FM_MAX_NUM_OF_10G_RX_PORTS; i++)
            p_LnxWrpFmDev->rxPorts[FM_MAX_NUM_OF_1G_RX_PORTS+i].settings.param.specificParams.rxParams.liodnOffset =
                p_Ppids->fmbm_ppid[phys10GRxPortId[i]-1];

#ifdef FM_OP_PARTITION_ERRATA_FMANx8
    for (i=0; i<FM_MAX_NUM_OF_OH_PORTS; i++)
    {
        /* OH port #0 is host-command, don't need this workaround */
        if (i == 0)
            continue;
        if (fmRev == 0x0100)
            /* TODO: this was opPartitionId = ; opLiodnOffset seems to have the same meaning */
            p_LnxWrpFmDev->opPorts[i-1].settings.param.specificParams.nonRxParams.opLiodnOffset =
                p_Ppids->fmbm_ppid[physOhPortId[i]-1];
    }
#endif  /* FM_OP_PARTITION_ERRATA_FMANx8 */

    return E_OK;
}

/* The default address for the Fman microcode in flash. Having a default
 * allows older systems to continue functioning.  0xEF000000 is the address
 * where the firmware is normally on a P4080DS.
 */
#ifdef CONFIG_PHYS_64BIT
static phys_addr_t P4080_UCAddr = 0xfef000000ull;
#else
static phys_addr_t P4080_UCAddr = 0xef000000;
#endif


/**
 * FmanUcodeAddrParam - process the fman_ucode kernel command-line parameter
 *
 * This function is called when the kernel encounters a fman_ucode command-
 * line parameter.  This parameter contains the address of the Fman microcode
 * in flash.
 */
static int FmanUcodeAddrParam(char *str)
{
    unsigned long long l;
    int ret;

    ret = strict_strtoull(str, 0, &l);
    if (!ret)
        P4080_UCAddr = (phys_addr_t) l;

    return ret;
}
__setup("fman_ucode=", FmanUcodeAddrParam);

/**
 * FindFmanMicrocode - find the Fman microcode in memory
 *
 * This function returns a pointer to the QE Firmware blob that holds
 * the Fman microcode.  We use the QE Firmware structure because Fman microcode
 * is similar to QE microcode, so there's no point in defining a new layout.
 *
 * Current versions of U-Boot embed the Fman firmware into the device tree,
 * so we check for that first.  Each Fman node in the device tree contains a
 * node or a pointer to node that holds the firmware.  Technically, we should
 * be fetching the firmware node for the current Fman, but we don't have that
 * information any more, so we assume that there is only one firmware node in
 * the device tree, and that all Fmen use the same firmware.
 *
 * If we have an older U-Boot, then we assume that the firmware is located in
 * flash at physical address 'P4080_UCAddr'
 */
static const struct qe_firmware *FindFmanMicrocode(void)
{
    static const struct qe_firmware *P4080_UCPatch;
    struct device_node *np;
#ifdef FMAN_READ_MICROCODE_FROM_NOR_FLASH
    unsigned long P4080_UCSize;
    const struct qe_header *hdr;
#endif

    if (P4080_UCPatch)
	    return P4080_UCPatch;

    /* The firmware should be inside the device tree. */
    np = of_find_compatible_node(NULL, NULL, "fsl,fman-firmware");
    if (np) {
	    P4080_UCPatch = of_get_property(np, "fsl,firmware", NULL);
            of_node_put(np);
	    if (P4080_UCPatch)
		    return P4080_UCPatch;
	    else
		    REPORT_ERROR(WARNING, E_NOT_FOUND, ("firmware node is incomplete"));
    }

#ifdef FMAN_READ_MICROCODE_FROM_NOR_FLASH
    /* If not, then we have a legacy U-Boot.  The firmware is in flash. */
    /* Only map enough to the get the core structure */
    P4080_UCPatch = ioremap(P4080_UCAddr, sizeof(struct qe_firmware));
    if (!P4080_UCPatch) {
        REPORT_ERROR(MAJOR, E_NULL_POINTER, ("ioremap(%llx) returned NULL", (u64) P4080_UCAddr));
        return NULL;
    }
    /* Make sure it really is a QE Firmware blob */
    hdr = &P4080_UCPatch->header;
    if (!hdr ||
        (hdr->magic[0] != 'Q') || (hdr->magic[1] != 'E') ||
        (hdr->magic[2] != 'F')) {
        REPORT_ERROR(MAJOR, E_NOT_FOUND, ("data at %llx is not a Fman microcode", (u64) P4080_UCAddr));
        return NULL;
    }

    /* Now we call ioremap again, this time to pick up the whole blob. We never
     * iounmap() the memory because we might reset the Fman at any time.
     */
    /* TODO: ionumap() should be performed when unloading the driver */
    P4080_UCSize = sizeof(u32) * P4080_UCPatch->microcode[0].count;
    iounmap((void *)P4080_UCPatch);
    P4080_UCPatch = ioremap(P4080_UCAddr, P4080_UCSize);
    if (!P4080_UCPatch) {
        REPORT_ERROR(MAJOR, E_NULL_POINTER, ("ioremap(%llx) returned NULL", (u64) P4080_UCAddr));
        return NULL;
    }
#else
    /* Returning NULL here forces the reuse of the IRAM content */
    P4080_UCPatch = NULL;
#endif /* FMAN_READ_MICROCODE_FROM_NOR_FLASH */
    return P4080_UCPatch;
}

static t_LnxWrpFmDev * ReadFmDevTreeNode (struct platform_device *of_dev)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev;
    struct device_node  *fm_node, *dev_node, *dpa_node;
    struct of_device_id name;
    struct resource     res;
    const uint32_t      *uint32_prop;
    int                 _errno=0, lenp;
    static struct of_device_id dpa_eth_node_of_match[] = {
        { .compatible = "fsl,dpa-ethernet", },
        { /* end of list */ },
    };

    fm_node = of_node_get(of_dev->dev.of_node);

    uint32_prop = (uint32_t *)of_get_property(fm_node, "cell-index", &lenp);
    if (unlikely(uint32_prop == NULL)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", fm_node->full_name));
        return NULL;
    }
    if (WARN_ON(lenp != sizeof(uint32_t)))
        return NULL;
    if (*uint32_prop > INTG_MAX_NUM_OF_FM) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("fm id!"));
        return NULL;
    }
    p_LnxWrpFmDev = CreateFmDev(*uint32_prop);
    if (!p_LnxWrpFmDev) {
        REPORT_ERROR(MAJOR, E_NULL_POINTER, NO_MSG);
        return NULL;
    }
    p_LnxWrpFmDev->dev = &of_dev->dev;
    p_LnxWrpFmDev->id = *uint32_prop;

    /* Get the FM interrupt */
    p_LnxWrpFmDev->irq = of_irq_to_resource(fm_node, 0, NULL);
    if (unlikely(p_LnxWrpFmDev->irq == /*NO_IRQ*/0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_irq_to_resource() = %d", NO_IRQ));
        return NULL;
    }

    /* Get the FM error interrupt */
    p_LnxWrpFmDev->err_irq = of_irq_to_resource(fm_node, 1, NULL);
    /* TODO - un-comment it once there will be err_irq in the DTS */
#if 0
    if (unlikely(p_LnxWrpFmDev->err_irq == /*NO_IRQ*/0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_irq_to_resource() = %d", NO_IRQ));
        return NULL;
    }
#endif /* 0 */

    /* Get the FM address */
    _errno = of_address_to_resource(fm_node, 0, &res);
    if (unlikely(_errno < 0)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
        return NULL;
    }

    p_LnxWrpFmDev->fmBaseAddr = 0;
    p_LnxWrpFmDev->fmPhysBaseAddr = res.start;
    p_LnxWrpFmDev->fmMemSize = res.end + 1 - res.start;

    uint32_prop = (uint32_t *)of_get_property(fm_node, "clock-frequency", &lenp);
    if (unlikely(uint32_prop == NULL)) {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, clock-frequency) failed", fm_node->full_name));
        return NULL;
    }
    if (WARN_ON(lenp != sizeof(uint32_t)))
        return NULL;
    p_LnxWrpFmDev->fmDevSettings.param.fmClkFreq = (*uint32_prop + 500000)/1000000; /* In MHz, rounded */

    /* Get the MURAM base address and size */
    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("muram") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "muram");
    if (WARN_ON(strlen("fsl,fman-muram") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-muram");
    for_each_child_of_node(fm_node, dev_node) {
        if (likely(of_match_node(&name, dev_node) != NULL)) {
            _errno = of_address_to_resource(dev_node, 0, &res);
            if (unlikely(_errno < 0)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
                return NULL;
            }

            p_LnxWrpFmDev->fmMuramBaseAddr = 0;
            p_LnxWrpFmDev->fmMuramPhysBaseAddr = res.start;
            p_LnxWrpFmDev->fmMuramMemSize = res.end + 1 - res.start;
        }
    }

    /* Get the RTC base address and size */
    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("rtc") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "rtc");
    if (WARN_ON(strlen("fsl,fman-rtc") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-rtc");
    for_each_child_of_node(fm_node, dev_node) {
        if (likely(of_match_node(&name, dev_node) != NULL)) {
            _errno = of_address_to_resource(dev_node, 0, &res);
            if (unlikely(_errno < 0)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_address_to_resource() = %d", _errno));
                return NULL;
            }

            p_LnxWrpFmDev->fmRtcBaseAddr = 0;
            p_LnxWrpFmDev->fmRtcPhysBaseAddr = res.start;
            p_LnxWrpFmDev->fmRtcMemSize = res.end + 1 - res.start;
        }
    }

    /* Get all PCD nodes */
    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("parser") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "parser");
    if (WARN_ON(strlen("fsl,fman-parser") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-parser");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->prsActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("keygen") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "keygen");
    if (WARN_ON(strlen("fsl,fman-keygen") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-keygen");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->kgActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("cc") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "cc");
    if (WARN_ON(strlen("fsl,fman-cc") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-cc");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->ccActive = TRUE;

    memset(&name, 0, sizeof(struct of_device_id));
    if (WARN_ON(strlen("policer") >= sizeof(name.name)))
        return NULL;
    strcpy(name.name, "policer");
    if (WARN_ON(strlen("fsl,fman-policer") >= sizeof(name.compatible)))
        return NULL;
    strcpy(name.compatible, "fsl,fman-policer");
    for_each_child_of_node(fm_node, dev_node)
        if (likely(of_match_node(&name, dev_node) != NULL))
            p_LnxWrpFmDev->plcrActive = TRUE;

    if (p_LnxWrpFmDev->prsActive || p_LnxWrpFmDev->kgActive ||
        p_LnxWrpFmDev->ccActive || p_LnxWrpFmDev->plcrActive)
        p_LnxWrpFmDev->pcdActive = TRUE;

    if (p_LnxWrpFmDev->pcdActive)
    {
        const char *str_prop = (char *)of_get_property(fm_node, "fsl,default-pcd", &lenp);
        if (str_prop) {
            if (strncmp(str_prop, "3-tuple", strlen("3-tuple")) == 0)
                p_LnxWrpFmDev->defPcd = e_FM_PCD_3_TUPLE;
        }
        else
            p_LnxWrpFmDev->defPcd = e_NO_PCD;
    }

    of_node_put(fm_node);

    for_each_matching_node(dpa_node, dpa_eth_node_of_match) {
        struct device_node  *mac_node;
        const phandle       *phandle_prop;

        phandle_prop = (typeof(phandle_prop))of_get_property(dpa_node, "fsl,fman-mac", &lenp);
        if (phandle_prop == NULL)
            continue;

        if (WARN_ON(lenp != sizeof(phandle)))
            return NULL;

        mac_node = of_find_node_by_phandle(*phandle_prop);
        if (unlikely(mac_node == NULL)) {
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_find_node_by_phandle() failed"));
            return NULL;
        }

        fm_node = of_get_parent(mac_node);
        of_node_put(mac_node);
        if (unlikely(fm_node == NULL)) {
            REPORT_ERROR(MAJOR, E_NO_DEVICE, ("of_get_parent() = %d", _errno));
            return NULL;
        }

        uint32_prop = (uint32_t *)of_get_property(fm_node, "cell-index", &lenp);
        if (unlikely(uint32_prop == NULL)) {
            of_node_put(fm_node);
            REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, cell-index) failed", fm_node->full_name));
            return NULL;
        }
        if (WARN_ON(lenp != sizeof(uint32_t)))
            return NULL;
        of_node_put(fm_node);

        if (*uint32_prop == p_LnxWrpFmDev->id) {
            phandle_prop = (typeof(phandle_prop))of_get_property(dpa_node, "fsl,qman-channel", &lenp);
            if (unlikely(phandle_prop == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, fsl,qman-channel) failed", dpa_node->full_name));
                return NULL;
            }
            if (WARN_ON(lenp != sizeof(phandle)))
                return NULL;

            dev_node = of_find_node_by_phandle(*phandle_prop);
            if (unlikely(dev_node == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_find_node_by_phandle() failed"));
                return NULL;
            }

            uint32_prop = (typeof(uint32_prop))of_get_property(dev_node, "fsl,qman-channel-id", &lenp);
            if (unlikely(uint32_prop == NULL)) {
                REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("of_get_property(%s, fsl,qman-channel-id) failed", dev_node->full_name));
                of_node_put(dev_node);
                return NULL;
            }
            of_node_put(dev_node);
            if (WARN_ON(lenp != sizeof(uint32_t)))
                return NULL;
            p_LnxWrpFmDev->hcCh = *uint32_prop;
            break;
        }
    }

    p_LnxWrpFmDev->active = TRUE;

    return p_LnxWrpFmDev;
}

static void LnxwrpFmDevExceptionsCb(t_Handle h_App, e_FmExceptions exception)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    DBG(INFO, ("got fm exception %d", exception));

    /* do nothing */
    UNUSED(exception);
}

static void LnxwrpFmDevBusErrorCb(t_Handle        h_App,
                                  e_FmPortType    portType,
                                  uint8_t         portId,
                                  uint64_t        addr,
                                  uint8_t         tnum,
                                  uint16_t        liodn)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev *)h_App;

    ASSERT_COND(p_LnxWrpFmDev);

    /* do nothing */
    UNUSED(portType);UNUSED(portId);UNUSED(addr);UNUSED(tnum);UNUSED(liodn);
}

static t_Error ConfigureFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    struct resource     *dev_res;
    int                 _errno;

    if (!p_LnxWrpFmDev->active)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM not configured!!!"));

#ifndef MODULE
    _errno = can_request_irq(p_LnxWrpFmDev->irq, 0);
    if (unlikely(_errno < 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("can_request_irq() = %d", _errno));
#endif
    _errno = devm_request_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->irq, fm_irq, 0, "fman", p_LnxWrpFmDev);
    if (unlikely(_errno < 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_irq(%d) = %d", p_LnxWrpFmDev->irq, _errno));

    if (p_LnxWrpFmDev->err_irq != 0) {
#ifndef MODULE
        _errno = can_request_irq(p_LnxWrpFmDev->err_irq, 0);
        if (unlikely(_errno < 0))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("can_request_irq() = %d", _errno));
#endif
        _errno = devm_request_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->err_irq, fm_err_irq, IRQF_SHARED, "fman-err", p_LnxWrpFmDev);
        if (unlikely(_errno < 0))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_irq(%d) = %d", p_LnxWrpFmDev->err_irq, _errno));
    }

    p_LnxWrpFmDev->res = devm_request_mem_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmPhysBaseAddr, p_LnxWrpFmDev->fmMemSize, "fman");
    if (unlikely(p_LnxWrpFmDev->res == NULL))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("request_mem_region() failed"));

    p_LnxWrpFmDev->fmBaseAddr = PTR_TO_UINT(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmPhysBaseAddr, p_LnxWrpFmDev->fmMemSize));
    if (unlikely(p_LnxWrpFmDev->fmBaseAddr == 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

    if (SYS_RegisterIoMap((uint64_t)p_LnxWrpFmDev->fmBaseAddr, (uint64_t)p_LnxWrpFmDev->fmPhysBaseAddr, p_LnxWrpFmDev->fmMemSize) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM memory map"));

    dev_res = __devm_request_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmMuramPhysBaseAddr, p_LnxWrpFmDev->fmMuramMemSize, "fman-muram");
    if (unlikely(dev_res == NULL))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("__devm_request_region() failed"));

    p_LnxWrpFmDev->fmMuramBaseAddr = PTR_TO_UINT(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmMuramPhysBaseAddr, p_LnxWrpFmDev->fmMuramMemSize));
    if (unlikely(p_LnxWrpFmDev->fmMuramBaseAddr == 0))
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

    if (SYS_RegisterIoMap((uint64_t)p_LnxWrpFmDev->fmMuramBaseAddr, (uint64_t)p_LnxWrpFmDev->fmMuramPhysBaseAddr, p_LnxWrpFmDev->fmMuramMemSize) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM MURAM memory map"));

    if (p_LnxWrpFmDev->fmRtcPhysBaseAddr)
    {
        dev_res = __devm_request_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmRtcPhysBaseAddr, p_LnxWrpFmDev->fmRtcMemSize, "fman-rtc");
        if (unlikely(dev_res == NULL))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("__devm_request_region() failed"));

        p_LnxWrpFmDev->fmRtcBaseAddr = PTR_TO_UINT(devm_ioremap(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmRtcPhysBaseAddr, p_LnxWrpFmDev->fmRtcMemSize));
        if (unlikely(p_LnxWrpFmDev->fmRtcBaseAddr == 0))
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("devm_ioremap() failed"));

        if (SYS_RegisterIoMap((uint64_t)p_LnxWrpFmDev->fmRtcBaseAddr, (uint64_t)p_LnxWrpFmDev->fmRtcPhysBaseAddr, p_LnxWrpFmDev->fmRtcMemSize) != E_OK)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM-RTC memory map"));
    }

    p_LnxWrpFmDev->fmDevSettings.param.baseAddr     = p_LnxWrpFmDev->fmBaseAddr;
    p_LnxWrpFmDev->fmDevSettings.param.fmId         = p_LnxWrpFmDev->id;
    p_LnxWrpFmDev->fmDevSettings.param.irq          = NO_IRQ;
    p_LnxWrpFmDev->fmDevSettings.param.errIrq       = NO_IRQ;
    p_LnxWrpFmDev->fmDevSettings.param.f_Exception  = LnxwrpFmDevExceptionsCb;
    p_LnxWrpFmDev->fmDevSettings.param.f_BusError   = LnxwrpFmDevBusErrorCb;
    p_LnxWrpFmDev->fmDevSettings.param.h_App        = p_LnxWrpFmDev;

    return FillRestFmInfo(p_LnxWrpFmDev);
}

static t_Error InitFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    const struct qe_firmware *fw;

    if (!p_LnxWrpFmDev->active)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM not configured!!!"));

    if ((p_LnxWrpFmDev->h_MuramDev = FM_MURAM_ConfigAndInit(p_LnxWrpFmDev->fmMuramBaseAddr, p_LnxWrpFmDev->fmMuramMemSize)) == NULL)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-MURAM!"));

    /* Loading the fman-controller code */
    fw = FindFmanMicrocode();

    if (!fw) {
#ifdef FMAN_READ_MICROCODE_FROM_NOR_FLASH
        /* We already reported an error, so just return NULL*/
        return ERROR_CODE(E_NULL_POINTER);
#else
        /* this forces the reuse of the current IRAM content */
        p_LnxWrpFmDev->fmDevSettings.param.firmware.size = 0;
        p_LnxWrpFmDev->fmDevSettings.param.firmware.p_Code = NULL;
#endif
    } else {
        p_LnxWrpFmDev->fmDevSettings.param.firmware.p_Code =
            (void *) fw + fw->microcode[0].code_offset;
        p_LnxWrpFmDev->fmDevSettings.param.firmware.size =
            sizeof(u32) * fw->microcode[0].count;
        DBG(INFO, ("Loading fman-controller code version %d.%d.%d",
                   fw->microcode[0].major,
                   fw->microcode[0].minor,
                   fw->microcode[0].revision));
    }

    p_LnxWrpFmDev->fmDevSettings.param.h_FmMuram = p_LnxWrpFmDev->h_MuramDev;

    if ((p_LnxWrpFmDev->h_Dev = FM_Config(&p_LnxWrpFmDev->fmDevSettings.param)) == NULL)
        RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM"));

    if (FM_ConfigMaxNumOfOpenDmas(p_LnxWrpFmDev->h_Dev,BMI_MAX_NUM_OF_DMAS) != E_OK)
         RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));

    if (FM_ConfigResetOnInit(p_LnxWrpFmDev->h_Dev, TRUE) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));

#ifdef CONFIG_FMAN_P1023
    if (FM_ConfigDmaAidOverride(p_LnxWrpFmDev->h_Dev, TRUE) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));
#endif

    /* Use the entire amount of TNUMS, maybe performance will improve...
    for OPEN DMAs - are all by default = 32 and fifosize = MURAM*3/4 and
    the rest of it is for PCD */
    FM_ConfigTotalNumOfTasks(p_LnxWrpFmDev->h_Dev, BMI_MAX_NUM_OF_TASKS);

#if defined(CONFIG_FMAN_RESOURCE_ALLOCATION_ALGORITHM) && defined(CONFIG_FMAN_P3040_P4080_P5020)
    /* Enable 14g w/ jumbo frames following HW suggestion. */
    FM_ConfigTotalFifoSize(p_LnxWrpFmDev->h_Dev, 128*KILOBYTE);
#endif

    if (FM_Init(p_LnxWrpFmDev->h_Dev) != E_OK)
        RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM"));

    if (p_LnxWrpFmDev->err_irq == 0) {
        FM_SetException(p_LnxWrpFmDev->h_Dev, e_FM_EX_DMA_BUS_ERROR,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_READ_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_SYSTEM_WRITE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_DMA_FM_WRITE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_STALL_ON_TASKS , FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_DOUBLE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_IRAM_ECC,FALSE);
        /* TODO: FmDisableRamsEcc assert for ramsEccOwners.
         * FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_MURAM_ECC,FALSE);*/
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_QMI_DOUBLE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_QMI_DEQ_FROM_UNKNOWN_PORTID,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_LIST_RAM_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_PIPELINE_ECC,FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_BMI_STATISTICS_RAM_ECC, FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_FPM_SINGLE_ECC, FALSE);
        FM_SetException(p_LnxWrpFmDev->h_Dev,e_FM_EX_QMI_SINGLE_ECC, FALSE);
    }

    if (p_LnxWrpFmDev->fmRtcBaseAddr)
    {
        t_FmRtcParams   fmRtcParam;

        memset(&fmRtcParam, 0, sizeof(fmRtcParam));
        fmRtcParam.h_App = p_LnxWrpFmDev;
        fmRtcParam.h_Fm = p_LnxWrpFmDev->h_Dev;
        fmRtcParam.baseAddress = p_LnxWrpFmDev->fmRtcBaseAddr;

        if(!(p_LnxWrpFmDev->h_RtcDev = FM_RTC_Config(&fmRtcParam)))
            RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-RTC"));

	if (FM_RTC_ConfigPeriod(p_LnxWrpFmDev->h_RtcDev, 10) != E_OK)
	    RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM-RTC"));

        if (FM_RTC_Init(p_LnxWrpFmDev->h_RtcDev) != E_OK)
            RETURN_ERROR(MAJOR, E_INVALID_STATE, ("FM-RTC"));
    }

    return E_OK;
}

/* TODO: to be moved back here */
extern void FreeFmPcdDev(t_LnxWrpFmDev  *p_LnxWrpFmDev);

static void FreeFmDev(t_LnxWrpFmDev  *p_LnxWrpFmDev)
{
    if (!p_LnxWrpFmDev->active)
        return;

    FreeFmPcdDev(p_LnxWrpFmDev);

    if (p_LnxWrpFmDev->h_RtcDev)
	FM_RTC_Free(p_LnxWrpFmDev->h_RtcDev);

    if (p_LnxWrpFmDev->h_Dev)
        FM_Free(p_LnxWrpFmDev->h_Dev);

    if (p_LnxWrpFmDev->h_MuramDev)
        FM_MURAM_Free(p_LnxWrpFmDev->h_MuramDev);

    if (p_LnxWrpFmDev->fmRtcBaseAddr)
    {
        SYS_UnregisterIoMap(p_LnxWrpFmDev->fmRtcBaseAddr);
        devm_iounmap(p_LnxWrpFmDev->dev, UINT_TO_PTR(p_LnxWrpFmDev->fmRtcBaseAddr));
        __devm_release_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmRtcPhysBaseAddr, p_LnxWrpFmDev->fmRtcMemSize);
    }
    SYS_UnregisterIoMap(p_LnxWrpFmDev->fmMuramBaseAddr);
    devm_iounmap(p_LnxWrpFmDev->dev, UINT_TO_PTR(p_LnxWrpFmDev->fmMuramBaseAddr));
    __devm_release_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res, p_LnxWrpFmDev->fmMuramPhysBaseAddr, p_LnxWrpFmDev->fmMuramMemSize);
    SYS_UnregisterIoMap(p_LnxWrpFmDev->fmBaseAddr);
    devm_iounmap(p_LnxWrpFmDev->dev, UINT_TO_PTR(p_LnxWrpFmDev->fmBaseAddr));
    devm_release_mem_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->fmPhysBaseAddr, p_LnxWrpFmDev->fmMemSize);
    if (p_LnxWrpFmDev->err_irq != 0) {
        devm_free_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->err_irq, p_LnxWrpFmDev);
    }

    devm_free_irq(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->irq, p_LnxWrpFmDev);
}

/* FMan character device file operations */
extern struct file_operations fm_fops;

static int fm_probe(struct platform_device *of_dev)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;

    if ((p_LnxWrpFmDev = ReadFmDevTreeNode(of_dev)) == NULL)
        return -EIO;
    if (ConfigureFmDev(p_LnxWrpFmDev) != E_OK)
        return -EIO;
    if (InitFmDev(p_LnxWrpFmDev) != E_OK)
        return -EIO;

    Sprint (p_LnxWrpFmDev->name, "%s%d", DEV_FM_NAME, p_LnxWrpFmDev->id);

    /* Register to the /dev for IOCTL API */
    /* Register dynamically a new major number for the character device: */
    if ((p_LnxWrpFmDev->major = register_chrdev(0, p_LnxWrpFmDev->name, &fm_fops)) <= 0) {
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Failed to allocate a major number for device \"%s\"", p_LnxWrpFmDev->name));
        return -EIO;
    }

    /* Creating classes for FM */
    DBG(TRACE ,("class_create fm_class"));
    p_LnxWrpFmDev->fm_class = class_create(THIS_MODULE, p_LnxWrpFmDev->name);
    if (IS_ERR(p_LnxWrpFmDev->fm_class)) {
        unregister_chrdev(p_LnxWrpFmDev->major, p_LnxWrpFmDev->name);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("class_create error fm_class"));
        return -EIO;
    }

    device_create(p_LnxWrpFmDev->fm_class, NULL, MKDEV(p_LnxWrpFmDev->major, DEV_FM_MINOR_BASE), NULL,
                  "fm%d", p_LnxWrpFmDev->id);
    device_create(p_LnxWrpFmDev->fm_class, NULL, MKDEV(p_LnxWrpFmDev->major, DEV_FM_PCD_MINOR_BASE), NULL,
                  "fm%d-pcd", p_LnxWrpFmDev->id);
    dev_set_drvdata(p_LnxWrpFmDev->dev, p_LnxWrpFmDev);

   /* create sysfs entries for stats and regs */
    if ( fm_sysfs_create(p_LnxWrpFmDev->dev) !=0 )
    {
        FreeFmDev(p_LnxWrpFmDev);
        REPORT_ERROR(MAJOR, E_INVALID_STATE, ("Unable to create sysfs entry - fm!!!"));
        return -EIO;
    }

    DBG(TRACE, ("FM%d probed", p_LnxWrpFmDev->id));

#if defined(CONFIG_FMAN_RESOURCE_ALLOCATION_ALGORITHM)
    /* Precalculate resources for FMAN based on number of 
     * FMan ports available
     */
    if(fm_set_active_fman_ports(of_dev, p_LnxWrpFmDev)!= 0)
        return -EIO;

#if defined(CONFIG_FMAN_P3040_P4080_P5020)
    /* 128K MURAM for p3,p4 and p5 */
    if(fm_precalculate_fifosizes(
        p_LnxWrpFmDev,
        128*KILOBYTE)
        != 0)
    return -EIO;
#else
    /* for all other platforms: MURAM Space for fifosize=3/4 * MURAM_SIZE*/
    if(fm_precalculate_fifosizes(
        p_LnxWrpFmDev,
        CEIL_DIV((3*FM_MURAM_SIZE-1),4))
        != 0)
    return -EIO;
#endif
    if(fm_precalculate_open_dma(
        p_LnxWrpFmDev,
        BMI_MAX_NUM_OF_DMAS,        /* max open dmas:dpaa_integration_ext.h */
        FM_DEFAULT_TX10G_OPENDMA,   /* default TX 10g open dmas */
        FM_DEFAULT_RX10G_OPENDMA,   /* default RX 10g open dmas */
        FM_10G_OPENDMA_MIN_TRESHOLD,/* TX 10g minimum treshold */
        FM_10G_OPENDMA_MIN_TRESHOLD)/* RX 10g minimum treshold */
        != 0)
        return -EIO;
    if(fm_precalculate_tnums(
        p_LnxWrpFmDev,
        BMI_MAX_NUM_OF_TASKS) /* max TNUMS: dpa integration file. */
        != 0)
        return -EIO;
#endif

    return 0;
}

static int fm_remove(struct platform_device *of_dev)
{
    t_LnxWrpFmDev   *p_LnxWrpFmDev;
    struct device   *dev;

    dev = &of_dev->dev;
    p_LnxWrpFmDev = dev_get_drvdata(dev);

    fm_sysfs_destroy(dev);

    DBG(TRACE, ("destroy fm_class"));
    device_destroy(p_LnxWrpFmDev->fm_class, MKDEV(p_LnxWrpFmDev->major, DEV_FM_MINOR_BASE));
    device_destroy(p_LnxWrpFmDev->fm_class, MKDEV(p_LnxWrpFmDev->major, DEV_FM_PCD_MINOR_BASE));
    class_destroy(p_LnxWrpFmDev->fm_class);

    /* Destroy chardev */
    unregister_chrdev(p_LnxWrpFmDev->major, p_LnxWrpFmDev->name);

    FreeFmDev(p_LnxWrpFmDev);

    DestroyFmDev(p_LnxWrpFmDev);

    dev_set_drvdata(dev, NULL);

    return 0;
}

static const struct of_device_id fm_match[] = {
    {
        .compatible    = "fsl,fman"
    },
    {}
};
#ifndef MODULE
MODULE_DEVICE_TABLE(of, fm_match);
#endif /* !MODULE */

static struct platform_driver fm_driver = {
    .driver = {
        .name           = "fsl-fman",
        .of_match_table    = fm_match,
        .owner          = THIS_MODULE,
    },
    .probe          = fm_probe,
    .remove         = fm_remove,
};

t_Handle LNXWRP_FM_Init(void)
{
    memset(&lnxWrpFm, 0, sizeof(lnxWrpFm));
    mutex_init(&lnxwrp_mutex);

    /* Register to the DTB for basic FM API */
    platform_driver_register(&fm_driver);

    return &lnxWrpFm;
}

t_Error LNXWRP_FM_Free(t_Handle h_LnxWrpFm)
{
    platform_driver_unregister(&fm_driver);
    mutex_destroy(&lnxwrp_mutex);

    return E_OK;
}


struct fm * fm_bind(struct device *fm_dev)
{
    return (struct fm *)(dev_get_drvdata(get_device(fm_dev)));
}
EXPORT_SYMBOL(fm_bind);

void fm_unbind(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    put_device(p_LnxWrpFmDev->dev);
}
EXPORT_SYMBOL(fm_unbind);

struct resource * fm_get_mem_region(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    return p_LnxWrpFmDev->res;
}
EXPORT_SYMBOL(fm_get_mem_region);

void * fm_get_handle(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    return (void *)p_LnxWrpFmDev->h_Dev;
}
EXPORT_SYMBOL(fm_get_handle);

void * fm_get_rtc_handle(struct fm *fm)
{
    t_LnxWrpFmDev       *p_LnxWrpFmDev = (t_LnxWrpFmDev*)fm;

    return (void *)p_LnxWrpFmDev->h_RtcDev;
}
EXPORT_SYMBOL(fm_get_rtc_handle);

struct fm_port * fm_port_bind (struct device *fm_port_dev)
{
    return (struct fm_port *)(dev_get_drvdata(get_device(fm_port_dev)));
}
EXPORT_SYMBOL(fm_port_bind);

void fm_port_unbind(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    put_device(p_LnxWrpFmPortDev->dev);
}
EXPORT_SYMBOL(fm_port_unbind);

void * fm_port_get_handle(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    return (void *)p_LnxWrpFmPortDev->h_Dev;
}
EXPORT_SYMBOL(fm_port_get_handle);

void fm_port_get_base_addr(const struct fm_port *port, uint64_t *base_addr)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *)port;

    *base_addr = p_LnxWrpFmPortDev->settings.param.baseAddr;
}
EXPORT_SYMBOL(fm_port_get_base_addr);

void fm_port_pcd_bind (struct fm_port *port, struct fm_port_pcd_param *params)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    p_LnxWrpFmPortDev->pcd_owner_params.cba = params->cba;
    p_LnxWrpFmPortDev->pcd_owner_params.cbf = params->cbf;
    p_LnxWrpFmPortDev->pcd_owner_params.dev = params->dev;
}
EXPORT_SYMBOL(fm_port_pcd_bind);

int fm_get_tx_port_channel(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    return p_LnxWrpFmPortDev->txCh;
}
EXPORT_SYMBOL(fm_get_tx_port_channel);

int fm_port_enable (struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    FM_PORT_Enable(p_LnxWrpFmPortDev->h_Dev);

    return 0;
}
EXPORT_SYMBOL(fm_port_enable);

void fm_port_disable(struct fm_port *port)
{
    t_LnxWrpFmPortDev   *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev*)port;

    FM_PORT_Disable(p_LnxWrpFmPortDev->h_Dev);
}
EXPORT_SYMBOL(fm_port_disable);

void fm_mutex_lock(void)
{
    mutex_lock(&lnxwrp_mutex);
}
EXPORT_SYMBOL(fm_mutex_lock);

void fm_mutex_unlock(void)
{
    mutex_unlock(&lnxwrp_mutex);
}
EXPORT_SYMBOL(fm_mutex_unlock);

static t_Handle h_FmLnxWrp;

static int __init __cold fm_load (void)
{
    if ((h_FmLnxWrp = LNXWRP_FM_Init()) == NULL)
    {
        printk("Failed to init FM wrapper!\n");
        return -ENODEV;
    }

	printk (KERN_INFO "Freescale FM module ("__DATE__ ":"__TIME__")\n");

    return 0;
}

static void __exit __cold fm_unload (void)
{
    if (h_FmLnxWrp)
        LNXWRP_FM_Free(h_FmLnxWrp);
}

module_init (fm_load);
module_exit (fm_unload);
