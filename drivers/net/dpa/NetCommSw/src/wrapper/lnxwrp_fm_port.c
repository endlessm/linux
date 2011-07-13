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

 @File          lnxwrp_fm_port.c

 @Description   FMD wrapper - FMan port functions.

*/

#include <linux/version.h>
#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODVERSIONS
#endif
#ifdef MODVERSIONS
#include <config/modversions.h>
#endif /* MODVERSIONS */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of_address.h>

#include "sprint_ext.h"
#include "fm_port_ext.h"
#include "fm_ioctls.h"
#include "lnxwrp_resources.h"
#include "lnxwrp_sysfs_fm_port.h"

/* TODO: duplicated, see lnxwrp_fm.c */
#define ADD_ADV_CONFIG_NO_RET(_func, _param)\
do {\
	if (i < max) {\
		p_Entry = &p_Entrys[i];\
		p_Entry->p_Function = _func;\
		_param\
		i++;\
	} else {\
		REPORT_ERROR(MAJOR, E_INVALID_VALUE,\
		("Number of advanced-configuration entries exceeded"));\
	} \
} while (0)


static volatile int hcFrmRcv/* = 0 */;
static spinlock_t lock;

static enum qman_cb_dqrr_result qm_tx_conf_dqrr_cb(struct qman_portal *portal,
						   struct qman_fq *fq,
						   const struct qm_dqrr_entry
						   *dq)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = ((t_FmTestFq *) fq)->h_Arg;
	unsigned long flags;

	FM_PCD_HcTxConf(p_LnxWrpFmDev->h_PcdDev, (t_DpaaFD *)&dq->fd);
	spin_lock_irqsave(&lock, flags);
	hcFrmRcv--;
	spin_unlock_irqrestore(&lock, flags);

	return qman_cb_dqrr_consume;
}

static enum qman_cb_dqrr_result qm_tx_dqrr_cb(struct qman_portal *portal,
					      struct qman_fq *fq,
					      const struct qm_dqrr_entry *dq)
{
	WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__,
	     __func__);
	return qman_cb_dqrr_consume;
}

static void qm_err_cb(struct qman_portal *portal,
		      struct qman_fq *fq, const struct qm_mr_entry *msg)
{
	WARN(1, "FMD: failure at %s:%d/%s()!\n", __FILE__, __LINE__,
	     __func__);
}

static struct qman_fq *FqAlloc(t_LnxWrpFmDev * p_LnxWrpFmDev,
			       uint32_t fqid,
			       uint32_t flags, uint16_t channel, uint8_t wq)
{
	int _errno;
	struct qman_fq *fq = NULL;
	t_FmTestFq *p_FmtFq;
	struct qm_mcc_initfq initfq;

	p_FmtFq = (t_FmTestFq *) XX_Malloc(sizeof(t_FmTestFq));
	if (!p_FmtFq) {
		REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj!!!"));
		return NULL;
	}

	p_FmtFq->fq_base.cb.dqrr =
		(QMAN_FQ_FLAG_NO_ENQUEUE ? qm_tx_conf_dqrr_cb :
		 qm_tx_dqrr_cb);
	p_FmtFq->fq_base.cb.ern = qm_err_cb;
	p_FmtFq->fq_base.cb.dc_ern = qm_err_cb;
	/* p_FmtFq->fq_base.cb.fqs = qm_err_cb; */
	/* qm_err_cb wrongly called when the FQ is parked */
	p_FmtFq->fq_base.cb.fqs = NULL;
	p_FmtFq->h_Arg = (t_Handle) p_LnxWrpFmDev;
	if (fqid == 0) {
		flags |= QMAN_FQ_FLAG_DYNAMIC_FQID;
		flags &= ~QMAN_FQ_FLAG_NO_MODIFY;
	} else {
		flags &= ~QMAN_FQ_FLAG_DYNAMIC_FQID;
	}

	if (qman_create_fq(fqid, flags, &p_FmtFq->fq_base)) {
		REPORT_ERROR(MAJOR, E_NO_MEMORY, ("FQ obj - qman_new_fq!!!"));
		XX_Free(p_FmtFq);
		return NULL;
	}
	fq = &p_FmtFq->fq_base;

	if (!(flags & QMAN_FQ_FLAG_NO_MODIFY)) {
		initfq.we_mask = QM_INITFQ_WE_DESTWQ;
		initfq.fqd.dest.channel = channel;
		initfq.fqd.dest.wq = wq;

		_errno = qman_init_fq(fq, QMAN_INITFQ_FLAG_SCHED, &initfq);
		if (unlikely(_errno < 0)) {
			REPORT_ERROR(MAJOR, E_NO_MEMORY,
				     ("FQ obj - qman_init_fq!!!"));
			qman_destroy_fq(fq, 0);
			XX_Free(p_FmtFq);
			return NULL;
		}
	}

	DBG(TRACE,
	    ("fqid %d, flags 0x%08x, channel %d, wq %d", qman_fq_fqid(fq),
	     flags, channel, wq));

	return fq;
}

static void FqFree(struct qman_fq *fq)
{
	int _errno;

	_errno = qman_retire_fq(fq, NULL);
	if (unlikely(_errno < 0))
		printk(KERN_WARNING "qman_retire_fq(%u) = %d\n", qman_fq_fqid(fq), _errno);

	_errno = qman_oos_fq(fq);
	if (unlikely(_errno < 0))
		printk(KERN_WARNING "qman_oos_fq(%u) = %d\n", qman_fq_fqid(fq), _errno);

	qman_destroy_fq(fq, 0);
	XX_Free((t_FmTestFq *) fq);
}

static t_Error QmEnqueueCB(t_Handle h_Arg, void *p_Fd)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = (t_LnxWrpFmDev *) h_Arg;
	int _errno, timeout = 1000000;
	unsigned long flags;

	ASSERT_COND(p_LnxWrpFmDev);

	spin_lock_irqsave(&lock, flags);
	hcFrmRcv++;
	spin_unlock_irqrestore(&lock, flags);

	_errno = qman_enqueue(p_LnxWrpFmDev->hc_tx_fq, (struct qm_fd *) p_Fd,
			      0);
	if (_errno)
		RETURN_ERROR(MINOR, E_INVALID_STATE,
			     ("qman_enqueue() failed"));

	while (hcFrmRcv && --timeout) {
		udelay(1);
		cpu_relax();
	}
	if (timeout == 0) {
		dump_stack();
		RETURN_ERROR(MINOR, E_WRITE_FAILED,
			     ("timeout waiting for Tx confirmation"));
		return E_WRITE_FAILED;
	}

	return E_OK;
}

static t_LnxWrpFmPortDev *ReadFmPortDevTreeNode(struct platform_device
						*of_dev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev;
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev;
	struct device_node *fm_node, *port_node;
	struct resource res;
	const uint32_t *uint32_prop;
	int _errno = 0, lenp;
#ifdef CONFIG_FMAN_P1023
	static unsigned char have_oh_port/* = 0 */;
#endif

	port_node = of_node_get(of_dev->dev.of_node);

	/* Get the FM node */
	fm_node = of_get_parent(port_node);
	if (unlikely(fm_node == NULL)) {
		REPORT_ERROR(MAJOR, E_NO_DEVICE,
			     ("of_get_parent() = %d", _errno));
		return NULL;
	}

	p_LnxWrpFmDev =
		dev_get_drvdata(&of_find_device_by_node(fm_node)->dev);
	of_node_put(fm_node);

	/* if fm_probe() failed, no point in going further with port probing */
	if (p_LnxWrpFmDev == NULL)
		return NULL;

	uint32_prop =
		(uint32_t *) of_get_property(port_node, "cell-index", &lenp);
	if (unlikely(uint32_prop == NULL)) {
		REPORT_ERROR(MAJOR, E_INVALID_VALUE,
			     ("of_get_property(%s, cell-index) failed",
			      port_node->full_name));
		return NULL;
	}
	if (WARN_ON(lenp != sizeof(uint32_t)))
		return NULL;
	if (of_device_is_compatible(port_node, "fsl,fman-port-oh")) {
		if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_OH_PORTS)) {
			REPORT_ERROR(MAJOR, E_INVALID_VALUE,
				     ("of_get_property(%s, cell-index) failed",
				      port_node->full_name));
			return NULL;
		}

#ifdef CONFIG_FMAN_P1023
		/* Beware, this can be done when there is only
		   one FMan to be initialized */
		if (!have_oh_port) {
			have_oh_port = 1; /* first OP/HC port
					     is used for host command */
#else
		/* Here it is hardcoded the use of the OH port 1
		   (with cell-index 0) */
		if (*uint32_prop == 0) {
#endif
			p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
			p_LnxWrpFmPortDev->id = 0;
			/*
			p_LnxWrpFmPortDev->id = *uint32_prop-1;
			p_LnxWrpFmPortDev->id = *uint32_prop;
			*/
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_OH_HOST_COMMAND;
		} else {
			p_LnxWrpFmPortDev =
				&p_LnxWrpFmDev->opPorts[*uint32_prop - 1];
			p_LnxWrpFmPortDev->id = *uint32_prop - 1;
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_OH_OFFLINE_PARSING;
		}
		p_LnxWrpFmPortDev->settings.param.portId = *uint32_prop;

		uint32_prop =
			(uint32_t *) of_get_property(port_node,
						     "fsl,qman-channel-id",
						     &lenp);
		if (uint32_prop == NULL) {
						/*
 REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("missing fsl,qman-channel-id"));
 */
			XX_Print("FM warning: missing fsl,qman-channel-id"
					" for OH port.\n");
			return NULL;
		}
		if (WARN_ON(lenp != sizeof(uint32_t)))
			return NULL;
		p_LnxWrpFmPortDev->txCh = *uint32_prop;

		p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.
			qmChannel = p_LnxWrpFmPortDev->txCh;
	} else if (of_device_is_compatible(port_node, "fsl,fman-port-1g-tx") ||
		 of_device_is_compatible(port_node, "fsl,fman-port-10g-tx")) {
		if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_TX_PORTS)) {
			REPORT_ERROR(MAJOR, E_INVALID_VALUE,
				     ("of_get_property(%s, cell-index) failed",
				      port_node->full_name));
			return NULL;
		}
		if (of_device_is_compatible
		    (port_node, "fsl,fman-port-10g-tx"))
			p_LnxWrpFmPortDev =
				&p_LnxWrpFmDev->txPorts[*uint32_prop +
						FM_MAX_NUM_OF_1G_TX_PORTS];
		else
			p_LnxWrpFmPortDev =
				&p_LnxWrpFmDev->txPorts[*uint32_prop];

		p_LnxWrpFmPortDev->id = *uint32_prop;
		p_LnxWrpFmPortDev->settings.param.portId =
			p_LnxWrpFmPortDev->id;
		if (of_device_is_compatible
		    (port_node, "fsl,fman-port-10g-tx"))
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_TX_10G;
		else
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_TX;

		uint32_prop =
			(uint32_t *) of_get_property(port_node,
						     "fsl,qman-channel-id",
						     &lenp);
		if (uint32_prop == NULL) {
			REPORT_ERROR(MAJOR, E_INVALID_VALUE,
				     ("missing fsl,qman-channel-id"));
			return NULL;
		}
		if (WARN_ON(lenp != sizeof(uint32_t)))
			return NULL;
		p_LnxWrpFmPortDev->txCh = *uint32_prop;
		p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.
			qmChannel = p_LnxWrpFmPortDev->txCh;
	} else if (of_device_is_compatible(port_node, "fsl,fman-port-1g-rx") ||
		 of_device_is_compatible(port_node, "fsl,fman-port-10g-rx")) {
		if (unlikely(*uint32_prop >= FM_MAX_NUM_OF_RX_PORTS)) {
			REPORT_ERROR(MAJOR, E_INVALID_VALUE,
				     ("of_get_property(%s, cell-index) failed",
				      port_node->full_name));
			return NULL;
		}
		if (of_device_is_compatible
		    (port_node, "fsl,fman-port-10g-rx"))
			p_LnxWrpFmPortDev =
				&p_LnxWrpFmDev->rxPorts[*uint32_prop +
						FM_MAX_NUM_OF_1G_RX_PORTS];
		else
			p_LnxWrpFmPortDev =
				&p_LnxWrpFmDev->rxPorts[*uint32_prop];

		p_LnxWrpFmPortDev->id = *uint32_prop;
		p_LnxWrpFmPortDev->settings.param.portId =
			p_LnxWrpFmPortDev->id;
		if (of_device_is_compatible
		    (port_node, "fsl,fman-port-10g-rx"))
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_RX_10G;
		else
			p_LnxWrpFmPortDev->settings.param.portType =
				e_FM_PORT_TYPE_RX;

		if (p_LnxWrpFmDev->pcdActive)
			p_LnxWrpFmPortDev->defPcd = p_LnxWrpFmDev->defPcd;
	} else {
		REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("Illegal port type"));
		return NULL;
	}

	_errno = of_address_to_resource(port_node, 0, &res);
	if (unlikely(_errno < 0)) {
		REPORT_ERROR(MAJOR, E_INVALID_VALUE,
			     ("of_address_to_resource() = %d", _errno));
		return NULL;
	}

	p_LnxWrpFmPortDev->dev = &of_dev->dev;
	p_LnxWrpFmPortDev->baseAddr = 0;
	p_LnxWrpFmPortDev->phys_baseAddr = res.start;
	p_LnxWrpFmPortDev->memSize = res.end + 1 - res.start;
	p_LnxWrpFmPortDev->settings.param.h_Fm = p_LnxWrpFmDev->h_Dev;
	p_LnxWrpFmPortDev->h_LnxWrpFmDev = (t_Handle) p_LnxWrpFmDev;

	of_node_put(port_node);

	p_LnxWrpFmPortDev->active = TRUE;

#if defined(CONFIG_FMAN_DISABLE_OH_TO_REUSE_RESOURCES)
	/* for performance mode no OH port available. */
	if (p_LnxWrpFmPortDev->settings.param.portType ==
	    e_FM_PORT_TYPE_OH_OFFLINE_PARSING)
		p_LnxWrpFmPortDev->active = FALSE;
#endif

	return p_LnxWrpFmPortDev;
}

static t_Error ConfigureFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	struct resource *dev_res;

	if (!p_LnxWrpFmPortDev->active)
		RETURN_ERROR(MAJOR, E_INVALID_STATE,
			     ("FM port not configured!!!"));

	dev_res =
		__devm_request_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res,
				      p_LnxWrpFmPortDev->phys_baseAddr,
				      p_LnxWrpFmPortDev->memSize,
				      "fman-port-hc");
	if (unlikely(dev_res == NULL))
		RETURN_ERROR(MAJOR, E_INVALID_STATE,
			     ("__devm_request_region() failed"));
	p_LnxWrpFmPortDev->baseAddr =
		PTR_TO_UINT(devm_ioremap
			    (p_LnxWrpFmDev->dev,
			     p_LnxWrpFmPortDev->phys_baseAddr,
			     p_LnxWrpFmPortDev->memSize));
	if (unlikely(p_LnxWrpFmPortDev->baseAddr == 0))
		REPORT_ERROR(MAJOR, E_INVALID_STATE,
			     ("devm_ioremap() failed"));

	p_LnxWrpFmPortDev->settings.param.baseAddr =
		p_LnxWrpFmPortDev->baseAddr;

	return E_OK;
}

static t_Error InitFmPort3TupleDefPcd(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	t_FmPcdNetEnvParams *p_netEnvParam = NULL;
	t_FmPcdKgSchemeParams *p_schemeParam = NULL;
	t_FmPortPcdParams pcdParam;
	t_FmPortPcdPrsParams prsParam;
	t_FmPortPcdKgParams kgParam;
	uint8_t i, j;

	if (!p_LnxWrpFmDev->kgActive)
		RETURN_ERROR(MAJOR, E_INVALID_STATE,
			     ("keygen must be enabled for 3-tuple PCD!"));

	if (!p_LnxWrpFmDev->prsActive)
		RETURN_ERROR(MAJOR, E_INVALID_STATE,
			     ("parser must be enabled for 3-tuple PCD!"));

	if (p_LnxWrpFmPortDev->pcdNumOfQs < 9)
		RETURN_ERROR(MINOR, E_INVALID_VALUE,
			     ("Need to save at least 18 queues for"
				"3-tuple PCD!!!"));

	p_LnxWrpFmPortDev->totalNumOfSchemes =
		p_LnxWrpFmPortDev->numOfSchemesUsed = 2;

	if (AllocSchemesForPort
	    (p_LnxWrpFmDev, p_LnxWrpFmPortDev->totalNumOfSchemes,
	     &p_LnxWrpFmPortDev->schemesBase) != E_OK)
		RETURN_ERROR(MAJOR, E_INVALID_STATE,
			     ("No schemes for Rx or OP port for"
				" 3-tuple PCD!!!"));

	p_netEnvParam = kzalloc(sizeof(*p_netEnvParam), GFP_KERNEL);
	if (!p_netEnvParam) {
		RETURN_ERROR(MAJOR, E_NO_MEMORY,
			     ("Failed to allocate p_netEnvParam"));
	}
	/* set netEnv */
	p_netEnvParam->numOfDistinctionUnits = 2;
	p_netEnvParam->units[0].hdrs[0].hdr =
		HEADER_TYPE_IPv4;	/* no special options */
	p_netEnvParam->units[1].hdrs[0].hdr = HEADER_TYPE_ETH;
	p_LnxWrpFmPortDev->h_DefNetEnv =
		FM_PCD_SetNetEnvCharacteristics(p_LnxWrpFmDev->h_PcdDev,
						p_netEnvParam);
	kfree(p_netEnvParam);
	if (!p_LnxWrpFmPortDev->h_DefNetEnv)
		RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM PCD!"));

	p_schemeParam = kmalloc(sizeof(*p_schemeParam), GFP_KERNEL);
	if (!p_schemeParam) {
		RETURN_ERROR(MAJOR, E_NO_MEMORY,
			     ("Failed to allocate p_schemeParam"));
	}
	for (i = 0; i < p_LnxWrpFmPortDev->numOfSchemesUsed; i++) {
		memset(p_schemeParam, 0, sizeof(*p_schemeParam));
		p_schemeParam->modify = FALSE;
		p_schemeParam->id.relativeSchemeId =
			i + p_LnxWrpFmPortDev->schemesBase;
		p_schemeParam->alwaysDirect = FALSE;
		p_schemeParam->netEnvParams.h_NetEnv =
			p_LnxWrpFmPortDev->h_DefNetEnv;
		p_schemeParam->schemeCounter.update = TRUE;
		p_schemeParam->schemeCounter.value = 0;

		switch (i) {
		case (0):	/* catch IPv4 */
			p_schemeParam->netEnvParams.numOfDistinctionUnits = 1;
			p_schemeParam->netEnvParams.unitIds[0] = 0;
			p_schemeParam->baseFqid = p_LnxWrpFmPortDev->pcdBaseQ;
			p_schemeParam->nextEngine = e_FM_PCD_DONE;
			p_schemeParam->numOfUsedExtractedOrs = 0;
			p_schemeParam->useHash = TRUE;
			p_schemeParam->keyExtractAndHashParams.
				numOfUsedExtracts = 3;
			for (j = 0;
			     j <
			     p_schemeParam->keyExtractAndHashParams.
			     numOfUsedExtracts; j++) {
				p_schemeParam->keyExtractAndHashParams.
					extractArray[j].type =
					e_FM_PCD_EXTRACT_BY_HDR;
				p_schemeParam->keyExtractAndHashParams.
					extractArray[j].extractByHdr.hdr =
					HEADER_TYPE_IPv4;
				p_schemeParam->keyExtractAndHashParams.
					extractArray[j].extractByHdr.
					ignoreProtocolValidation = FALSE;
				p_schemeParam->keyExtractAndHashParams.
					extractArray[j].extractByHdr.type =
					e_FM_PCD_EXTRACT_FULL_FIELD;
			}
			p_schemeParam->keyExtractAndHashParams.
				extractArray[0].extractByHdr.extractByHdrType.
				fullField.ipv4 = NET_HEADER_FIELD_IPv4_PROTO;
			p_schemeParam->keyExtractAndHashParams.
				extractArray[1].extractByHdr.extractByHdrType.
				fullField.ipv4 = NET_HEADER_FIELD_IPv4_SRC_IP;
			p_schemeParam->keyExtractAndHashParams.
				extractArray[2].extractByHdr.extractByHdrType.
				fullField.ipv4 = NET_HEADER_FIELD_IPv4_DST_IP;

			if (p_schemeParam->useHash) {
				p_schemeParam->keyExtractAndHashParams.
					privateDflt0 = 0x01020304;
				p_schemeParam->keyExtractAndHashParams.
					privateDflt1 = 0x11121314;
				p_schemeParam->keyExtractAndHashParams.
					numOfUsedDflts =
					FM_PCD_KG_NUM_OF_DEFAULT_GROUPS;
				for (j = 0;
				     j < FM_PCD_KG_NUM_OF_DEFAULT_GROUPS;
				     j++) {
						/* all types */
			p_schemeParam->keyExtractAndHashParams.dflts[j].type =
				(e_FmPcdKgKnownFieldsDfltTypes) j;
					p_schemeParam->
						keyExtractAndHashParams.
						dflts[j].dfltSelect =
						e_FM_PCD_KG_DFLT_GBL_0;
				}
				p_schemeParam->keyExtractAndHashParams.
					numOfUsedMasks = 0;
				p_schemeParam->keyExtractAndHashParams.
					hashShift = 0;
				p_schemeParam->keyExtractAndHashParams.
					hashDistributionNumOfFqids = 8;
			}
			break;

		case (1):	/* Garbage collector */
			p_schemeParam->netEnvParams.numOfDistinctionUnits = 0;
			p_schemeParam->baseFqid =
				p_LnxWrpFmPortDev->pcdBaseQ + 8;
			break;

		default:
			break;
		}

		p_LnxWrpFmPortDev->h_Schemes[i] =
			FM_PCD_KgSetScheme(p_LnxWrpFmDev->h_PcdDev,
					   p_schemeParam);
		if (!p_LnxWrpFmPortDev->h_Schemes[i]) {
			kfree(p_schemeParam);
			RETURN_ERROR(MAJOR, E_INVALID_HANDLE,
				     ("FM_PCD_KgSetScheme failed"));
		}
	}
	kfree(p_schemeParam);

	/* initialize PCD parameters */
	memset(&pcdParam, 0, sizeof(t_FmPortPcdParams));
	pcdParam.h_NetEnv = p_LnxWrpFmPortDev->h_DefNetEnv;
	pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG;

	/* initialize Keygen parameters */
	memset(&prsParam, 0, sizeof(t_FmPortPcdPrsParams));

	prsParam.parsingOffset = 0;
	prsParam.firstPrsHdr = HEADER_TYPE_ETH;
	pcdParam.p_PrsParams = &prsParam;

	/* initialize Parser parameters */
	memset(&kgParam, 0, sizeof(t_FmPortPcdKgParams));
	kgParam.numOfSchemes = p_LnxWrpFmPortDev->numOfSchemesUsed;
	for (i = 0; i < kgParam.numOfSchemes; i++)
		kgParam.h_Schemes[i] = p_LnxWrpFmPortDev->h_Schemes[i];

	pcdParam.p_KgParams = &kgParam;

	return FM_PORT_SetPCD(p_LnxWrpFmPortDev->h_Dev, &pcdParam);
}

static t_Error InitFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
#define MY_ADV_CONFIG_CHECK_END \
		RETURN_ERROR(MAJOR, E_INVALID_SELECTION,\
			("Advanced configuration routine"));\
		if (errCode != E_OK)\
			RETURN_ERROR(MAJOR, errCode, NO_MSG);\
	}

	int i = 0;

	if (!p_LnxWrpFmPortDev->active || p_LnxWrpFmPortDev->h_Dev)
		return E_INVALID_STATE;

	p_LnxWrpFmPortDev->h_Dev =
		     FM_PORT_Config(&p_LnxWrpFmPortDev->settings.param);
	if (p_LnxWrpFmPortDev->h_Dev == NULL)
		RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM-port"));

	if ((p_LnxWrpFmPortDev->settings.param.portType ==
	     e_FM_PORT_TYPE_TX_10G)
	    || (p_LnxWrpFmPortDev->settings.param.portType ==
		e_FM_PORT_TYPE_TX)) {
		t_Error errCode = E_OK;
		errCode =
		     FM_PORT_ConfigDeqHighPriority(p_LnxWrpFmPortDev->h_Dev,
								   TRUE);
		if (errCode != E_OK)
			RETURN_ERROR(MAJOR, errCode, NO_MSG);
#ifdef FM_QMI_DEQ_OPTIONS_SUPPORT
		errCode =
		FM_PORT_ConfigDeqPrefetchOption(p_LnxWrpFmPortDev->h_Dev,
						e_FM_PORT_DEQ_FULL_PREFETCH);
		if (errCode
		    != E_OK)
			RETURN_ERROR(MAJOR, errCode, NO_MSG);
#endif /* FM_QMI_DEQ_OPTIONS_SUPPORT */
	}

/* Call the driver's advanced configuration routines, if requested:
   Compare the function pointer of each entry to the available routines,
   and invoke the matching routine with proper casting of arguments. */
	while (p_LnxWrpFmPortDev->settings.advConfig[i].p_Function
	       && (i < FM_MAX_NUM_OF_ADV_SETTINGS)) {
		ADV_CONFIG_CHECK_START(&
				       (p_LnxWrpFmPortDev->settings.
					advConfig[i]))

			ADV_CONFIG_CHECK(p_LnxWrpFmPortDev->h_Dev,
					 FM_PORT_ConfigBufferPrefixContent,
					 PARAMS(1,
						(t_FmPortBufferPrefixContent
						 *)))

			MY_ADV_CONFIG_CHECK_END
			/* Advance to next advanced configuration entry */
			i++;
	}

	if (FM_PORT_Init(p_LnxWrpFmPortDev->h_Dev) != E_OK)
		RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);
#if defined(CONFIG_FMAN_RESOURCE_ALLOCATION_ALGORITHM)
	/* even if these functions return w/ error, do not crash kernel.
	   Do not return anything because the container function is not
	   linux complient (it should return -EIO). */
	fm_set_precalculate_fifosize(p_LnxWrpFmPortDev);
	fm_set_precalculate_open_dma(p_LnxWrpFmPortDev);
	fm_set_precalculate_tnums(p_LnxWrpFmPortDev);
#endif

/* FMan Fifo sizes behind the scene":
 * Using the following formulae (*), under a set of simplifying assumptions (.):
 *  . all ports are configured in Normal Mode (rather than Independent Mode)
 *  . the DPAA Eth driver allocates buffers of size:
 *      . MAXFRM + NET_IP_ALIGN + DPA_PRIV_DATA_SIZE + DPA_PARSE_RESULTS_SIZE
 *		 + DPA_HASH_RESULTS_SIZE, i.e.:
 *        MAXFRM + 2 + 16 + sizeof(t_FmPrsResult) + 16, i.e.:
 *        MAXFRM + 66
 *  . excessive buffer pools not accounted for
 *
 *  * for Rx ports on P4080:
 *      . IFSZ = ceil(max(FMBM_EBMPI[PBS]) / 256) * 256 + 7 * 256
 *      . no internal frame offset (FMBM_RIM[FOF] == 0) - otherwise,
 *      add up to 256 to the above
 *
 *  * for Rx ports on P1023:
 *      . IFSZ = ceil(second_largest(FMBM_EBMPI[PBS] / 256)) * 256 + 7 * 256,
 *      if at least 2 bpools are configured
 *      . IFSZ = 8 * 256, if only a single bpool is configured
 *
 *  * for Tx ports:
 *      . IFSZ = ceil(frame_size / 256) * 256 + 3 * 256
 *			+ FMBM_TFP[DPDE] * 256, i.e.:
 *        IFSZ = ceil(MAXFRM / 256) * 256 + 3 x 256 + FMBM_TFP[DPDE] * 256
 *
 *  * for OH ports on P4080:
 *      . IFSZ = ceil(frame_size / 256) * 256 + 1 * 256 + FMBM_PP[MXT] * 256
 *  * for OH ports on P1023:
 *      . IFSZ = ceil(frame_size / 256) * 256 + 3 * 256 + FMBM_TFP[DPDE] * 256
 *  * for both P4080 and P1023:
 *      . (conservative decisions, assuming that BMI must bring the entire
 *      frame, not only the frame header)
 *      . no internal frame offset (FMBM_OIM[FOF] == 0) - otherwise,
 *      add up to 256 to the above
 *
 *  . for P4080/P5020/P3041/P2040, DPDE is:
 *              > 0 or 1, for 1Gb ports, HW default: 0
 *              > 2..7 (recommended: 3..7) for 10Gb ports, HW default: 3
 *  . for P1023, DPDE should be 1
 *
 *  . for P1023, MXT is in range (0..31)
 *  . for P4080, MXT is in range (0..63)
 *
 */

	if ((p_LnxWrpFmPortDev->defPcd != e_NO_PCD) &&
	    (InitFmPort3TupleDefPcd(p_LnxWrpFmPortDev) != E_OK))
		RETURN_ERROR(MAJOR, E_INVALID_STATE, NO_MSG);

	return E_OK;
}

void fm_set_rx_port_params(struct fm_port *port,
			   struct fm_port_rx_params *params)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *) port;
	int i;

	p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.errFqid =
		params->errq;
	p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.dfltFqid =
		params->defq;
	p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.extBufPools.
		numOfPoolsUsed = params->num_pools;
	for (i = 0; i < params->num_pools; i++) {
		p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.
			extBufPools.extBufPool[i].id =
			params->pool_param[i].id;
		p_LnxWrpFmPortDev->settings.param.specificParams.rxParams.
			extBufPools.extBufPool[i].size =
			params->pool_param[i].size;
	}

	p_LnxWrpFmPortDev->buffPrefixContent.privDataSize =
		params->priv_data_size;
	p_LnxWrpFmPortDev->buffPrefixContent.passPrsResult =
		params->parse_results;
	p_LnxWrpFmPortDev->buffPrefixContent.passHashResult =
		params->hash_results;
	p_LnxWrpFmPortDev->buffPrefixContent.passTimeStamp =
		params->time_stamp;

	ADD_ADV_CONFIG_START(p_LnxWrpFmPortDev->settings.advConfig,
			     FM_MAX_NUM_OF_ADV_SETTINGS)

		ADD_ADV_CONFIG_NO_RET(FM_PORT_ConfigBufferPrefixContent,
				      ARGS(1,
					   (&p_LnxWrpFmPortDev->
					    buffPrefixContent)));

	ADD_ADV_CONFIG_END InitFmPortDev(p_LnxWrpFmPortDev);
}
EXPORT_SYMBOL(fm_set_rx_port_params);

void fm_set_tx_port_params(struct fm_port *port,
			   struct fm_port_non_rx_params *params)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = (t_LnxWrpFmPortDev *) port;

	p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.errFqid =
		params->errq;
	p_LnxWrpFmPortDev->settings.param.specificParams.nonRxParams.
		dfltFqid = params->defq;

	p_LnxWrpFmPortDev->buffPrefixContent.privDataSize =
		params->priv_data_size;
	p_LnxWrpFmPortDev->buffPrefixContent.passPrsResult =
		params->parse_results;
	p_LnxWrpFmPortDev->buffPrefixContent.passHashResult =
		params->hash_results;
	p_LnxWrpFmPortDev->buffPrefixContent.passTimeStamp =
		params->time_stamp;

	ADD_ADV_CONFIG_START(p_LnxWrpFmPortDev->settings.advConfig,
			     FM_MAX_NUM_OF_ADV_SETTINGS)

		ADD_ADV_CONFIG_NO_RET(FM_PORT_ConfigBufferPrefixContent,
				      ARGS(1,
					   (&p_LnxWrpFmPortDev->
					    buffPrefixContent)));

	ADD_ADV_CONFIG_END InitFmPortDev(p_LnxWrpFmPortDev);
}
EXPORT_SYMBOL(fm_set_tx_port_params);

static void LnxwrpFmPcdDevExceptionsCb(t_Handle h_App,
				       e_FmPcdExceptions exception)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = (t_LnxWrpFmDev *) h_App;

	ASSERT_COND(p_LnxWrpFmDev);

	DBG(INFO, ("got fm-pcd exception %d", exception));

	/* do nothing */
	UNUSED(exception);
}

static void LnxwrpFmPcdDevIndexedExceptionsCb(t_Handle h_App,
					      e_FmPcdExceptions exception,
					      uint16_t index)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev = (t_LnxWrpFmDev *) h_App;

	ASSERT_COND(p_LnxWrpFmDev);

	DBG(INFO,
	    ("got fm-pcd-indexed exception %d, indx %d", exception, index));

	/* do nothing */
	UNUSED(exception);
	UNUSED(index);
}

static t_Error InitFmPcdDev(t_LnxWrpFmDev *p_LnxWrpFmDev)
{
	spin_lock_init(&lock);

	if (p_LnxWrpFmDev->pcdActive) {
		t_LnxWrpFmPortDev *p_LnxWrpFmPortDev = &p_LnxWrpFmDev->hcPort;
		t_FmPcdParams fmPcdParams;
		t_Error err;

		memset(&fmPcdParams, 0, sizeof(fmPcdParams));
		fmPcdParams.h_Fm = p_LnxWrpFmDev->h_Dev;
		fmPcdParams.prsSupport = p_LnxWrpFmDev->prsActive;
		fmPcdParams.kgSupport = p_LnxWrpFmDev->kgActive;
		fmPcdParams.plcrSupport = p_LnxWrpFmDev->plcrActive;
		fmPcdParams.ccSupport = p_LnxWrpFmDev->ccActive;
		fmPcdParams.numOfSchemes = FM_PCD_KG_NUM_OF_SCHEMES;

#ifndef CONFIG_GUEST_PARTITION
		fmPcdParams.f_Exception = LnxwrpFmPcdDevExceptionsCb;
		if (fmPcdParams.kgSupport)
			fmPcdParams.f_ExceptionId =
				LnxwrpFmPcdDevIndexedExceptionsCb;
		fmPcdParams.h_App = p_LnxWrpFmDev;
#endif /* !CONFIG_GUEST_PARTITION */

#ifdef CONFIG_MULTI_PARTITION_SUPPORT
		fmPcdParams.numOfSchemes = 0;
		fmPcdParams.numOfClsPlanEntries = 0;
		fmPcdParams.partitionId = 0;
#endif /* CONFIG_MULTI_PARTITION_SUPPORT */
		fmPcdParams.useHostCommand = TRUE;

		p_LnxWrpFmDev->hc_tx_fq =
			FqAlloc(p_LnxWrpFmDev,
				0,
				QMAN_FQ_FLAG_TO_DCPORTAL,
				p_LnxWrpFmPortDev->txCh, 0);
		if (!p_LnxWrpFmDev->hc_tx_fq)
			RETURN_ERROR(MAJOR, E_NULL_POINTER,
				     ("Frame queue allocation failed..."));

		p_LnxWrpFmDev->hc_tx_conf_fq =
			FqAlloc(p_LnxWrpFmDev,
				0,
				QMAN_FQ_FLAG_NO_ENQUEUE,
				p_LnxWrpFmDev->hcCh, 7);
		if (!p_LnxWrpFmDev->hc_tx_conf_fq)
			RETURN_ERROR(MAJOR, E_NULL_POINTER,
				     ("Frame queue allocation failed..."));

		p_LnxWrpFmDev->hc_tx_err_fq =
			FqAlloc(p_LnxWrpFmDev,
				0,
				QMAN_FQ_FLAG_NO_ENQUEUE,
				p_LnxWrpFmDev->hcCh, 7);
		if (!p_LnxWrpFmDev->hc_tx_err_fq)
			RETURN_ERROR(MAJOR, E_NULL_POINTER,
				     ("Frame queue allocation failed..."));

		fmPcdParams.hc.portBaseAddr = p_LnxWrpFmPortDev->baseAddr;
		fmPcdParams.hc.portId =
			p_LnxWrpFmPortDev->settings.param.portId;
		fmPcdParams.hc.liodnBase =
			p_LnxWrpFmPortDev->settings.param.liodnBase;
		fmPcdParams.hc.errFqid =
			qman_fq_fqid(p_LnxWrpFmDev->hc_tx_err_fq);
		fmPcdParams.hc.confFqid =
			qman_fq_fqid(p_LnxWrpFmDev->hc_tx_conf_fq);
		fmPcdParams.hc.qmChannel = p_LnxWrpFmPortDev->txCh;
		fmPcdParams.hc.f_QmEnqueue = QmEnqueueCB;
		fmPcdParams.hc.h_QmArg = (t_Handle) p_LnxWrpFmDev;

		p_LnxWrpFmDev->h_PcdDev = FM_PCD_Config(&fmPcdParams);
		if (!p_LnxWrpFmDev->h_PcdDev)
			RETURN_ERROR(MAJOR, E_INVALID_HANDLE, ("FM PCD!"));

		err =
		FM_PCD_ConfigPlcrNumOfSharedProfiles(p_LnxWrpFmDev->h_PcdDev,
				LNXWRP_FM_NUM_OF_SHARED_PROFILES);
		if (err != E_OK)
			RETURN_ERROR(MAJOR, err, NO_MSG);

		err = FM_PCD_Init(p_LnxWrpFmDev->h_PcdDev);
		if (err != E_OK)
			RETURN_ERROR(MAJOR, err, NO_MSG);

		if (p_LnxWrpFmDev->err_irq == 0) {
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_KG_EXCEPTION_DOUBLE_ECC,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_KG_EXCEPTION_KEYSIZE_OVERFLOW,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_PLCR_EXCEPTION_INIT_ENTRY_ERROR,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_PLCR_EXCEPTION_DOUBLE_ECC,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_PRS_EXCEPTION_DOUBLE_ECC,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
			    e_FM_PCD_PLCR_EXCEPTION_PRAM_SELF_INIT_COMPLETE,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_PLCR_EXCEPTION_ATOMIC_ACTION_COMPLETE,
				FALSE);
			FM_PCD_SetException(p_LnxWrpFmDev->h_PcdDev,
				e_FM_PCD_PRS_EXCEPTION_SINGLE_ECC,
				FALSE);
		}
	}

	return E_OK;
}

void FreeFmPcdDev(t_LnxWrpFmDev *p_LnxWrpFmDev)
{

	if (p_LnxWrpFmDev->h_PcdDev)
		FM_PCD_Free(p_LnxWrpFmDev->h_PcdDev);

	if (p_LnxWrpFmDev->hc_tx_err_fq)
		FqFree(p_LnxWrpFmDev->hc_tx_err_fq);

	if (p_LnxWrpFmDev->hc_tx_conf_fq)
		FqFree(p_LnxWrpFmDev->hc_tx_conf_fq);

	if (p_LnxWrpFmDev->hc_tx_fq)
		FqFree(p_LnxWrpFmDev->hc_tx_fq);
}

static void FreeFmPortDev(t_LnxWrpFmPortDev *p_LnxWrpFmPortDev)
{
	t_LnxWrpFmDev *p_LnxWrpFmDev =
		(t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;

	if (!p_LnxWrpFmPortDev->active)
		return;

	if (p_LnxWrpFmPortDev->h_Dev)
		FM_PORT_Free(p_LnxWrpFmPortDev->h_Dev);

	devm_iounmap(p_LnxWrpFmDev->dev,
		     UINT_TO_PTR(p_LnxWrpFmPortDev->baseAddr));
	__devm_release_region(p_LnxWrpFmDev->dev, p_LnxWrpFmDev->res,
			      p_LnxWrpFmPortDev->phys_baseAddr,
			      p_LnxWrpFmPortDev->memSize);
}

static int fm_port_probe(struct platform_device *of_dev)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev;
	t_LnxWrpFmDev *p_LnxWrpFmDev;
	struct device *dev;

	dev = &of_dev->dev;

	p_LnxWrpFmPortDev = ReadFmPortDevTreeNode(of_dev);
	if (p_LnxWrpFmPortDev == NULL)
		return -EIO;
	/* Port can be inactive, thus will not be probed:
	   - in performance mode, OH ports are disabled
	   ...
	 */
	if (!p_LnxWrpFmPortDev->active)
		return 0;

	if (ConfigureFmPortDev(p_LnxWrpFmPortDev) != E_OK)
		return -EIO;

	dev_set_drvdata(dev, p_LnxWrpFmPortDev);

	if ((p_LnxWrpFmPortDev->settings.param.portType ==
	     e_FM_PORT_TYPE_OH_HOST_COMMAND)
	    &&
	    (InitFmPcdDev((t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev)
	     != E_OK))
		return -EIO;

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;

	if (p_LnxWrpFmPortDev->settings.param.portType == e_FM_PORT_TYPE_RX) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-rx%d",
		       p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + DEV_FM_RX_PORTS_MINOR_BASE;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_RX_10G) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-rx%d",
		       p_LnxWrpFmDev->name,
		       p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_RX_PORTS);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_RX_PORTS +
			DEV_FM_RX_PORTS_MINOR_BASE;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_TX) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-tx%d",
		       p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + DEV_FM_TX_PORTS_MINOR_BASE;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_TX_10G) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-tx%d",
		       p_LnxWrpFmDev->name,
		       p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_TX_PORTS);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + FM_MAX_NUM_OF_1G_TX_PORTS +
			DEV_FM_TX_PORTS_MINOR_BASE;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_OH_HOST_COMMAND) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-oh%d",
		       p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + DEV_FM_OH_PORTS_MINOR_BASE;
	} else if (p_LnxWrpFmPortDev->settings.param.portType ==
		 e_FM_PORT_TYPE_OH_OFFLINE_PARSING) {
		Sprint(p_LnxWrpFmPortDev->name, "%s-port-oh%d",
		       p_LnxWrpFmDev->name, p_LnxWrpFmPortDev->id + 1);
		p_LnxWrpFmPortDev->minor =
			p_LnxWrpFmPortDev->id + 1 +
			DEV_FM_OH_PORTS_MINOR_BASE;
	}

	device_create(p_LnxWrpFmDev->fm_class, NULL,
		      MKDEV(p_LnxWrpFmDev->major, p_LnxWrpFmPortDev->minor),
		      NULL, p_LnxWrpFmPortDev->name);

	/* create sysfs entries for stats and regs */

	if (fm_port_sysfs_create(dev) != 0) {
		FreeFmPortDev(p_LnxWrpFmPortDev);
		REPORT_ERROR(MAJOR, E_INVALID_STATE,
			     ("Unable to create sys entry - fm port!!!"));
		return -EIO;
	}

#ifdef FM_TX_INVALID_ECC_ERRATA_10GMAC_A009
	FM_DisableRamsEcc(p_LnxWrpFmDev->h_Dev);
#endif /* FM_TX_INVALID_ECC_ERRATA_10GMAC_A009 */

	DBG(TRACE, ("%s probed", p_LnxWrpFmPortDev->name));

	return 0;
}

static int fm_port_remove(struct platform_device *of_dev)
{
	t_LnxWrpFmPortDev *p_LnxWrpFmPortDev;
	t_LnxWrpFmDev *p_LnxWrpFmDev;
	struct device *dev;

	dev = &of_dev->dev;
	p_LnxWrpFmPortDev = dev_get_drvdata(dev);

	fm_port_sysfs_destroy(dev);

	p_LnxWrpFmDev = (t_LnxWrpFmDev *) p_LnxWrpFmPortDev->h_LnxWrpFmDev;
	device_destroy(p_LnxWrpFmDev->fm_class,
		       MKDEV(p_LnxWrpFmDev->major, p_LnxWrpFmPortDev->minor));

	FreeFmPortDev(p_LnxWrpFmPortDev);

	dev_set_drvdata(dev, NULL);

	return 0;
}

static const struct of_device_id fm_port_match[] = {
	{
	 .compatible = "fsl,fman-port-oh"},
	{
	 .compatible = "fsl,fman-port-1g-rx"},
	{
	 .compatible = "fsl,fman-port-10g-rx"},
	{
	 .compatible = "fsl,fman-port-1g-tx"},
	{
	 .compatible = "fsl,fman-port-10g-tx"},
	{}
};

#ifndef MODULE
MODULE_DEVICE_TABLE(of, fm_port_match);
#endif /* !MODULE */

static struct platform_driver fm_port_driver = {

	.driver = {
		   .name = "fsl-fman-port",
		   .of_match_table = fm_port_match,
		   .owner = THIS_MODULE,
		   },
	.probe = fm_port_probe,
	.remove = fm_port_remove,
};


t_Error LNXWRP_FM_Port_Init(void)
{
	/* Register to the DTB for basic FM port API */
	if (platform_driver_register(&fm_port_driver))
		return E_NO_DEVICE;

	return E_OK;
}

void LNXWRP_FM_Port_Free(void)
{
	platform_driver_unregister(&fm_port_driver);
}

static int __init __cold fm_port_load(void)
{
	if (LNXWRP_FM_Port_Init() != E_OK) {
		printk(KERN_CRIT "Failed to init FM Ports wrapper!\n");
		return -ENODEV;
	}

	printk(KERN_INFO "Freescale FM Ports module (" __DATE__ ":" __TIME__ ")\n");

	return 0;
}

static void __exit __cold fm_port_unload(void)
{
	LNXWRP_FM_Port_Free();
}

module_init(fm_port_load);
module_exit(fm_port_unload);
