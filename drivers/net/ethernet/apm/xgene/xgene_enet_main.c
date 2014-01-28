/* AppliedMicro X-Gene SoC Ethernet Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Authors:	Ravi Patel <rapatel@apm.com>
 *		Iyappan Subramanian <isubramanian@apm.com>
 *		Fushen Chen <fchen@apm.com>
 *		Keyur Chudgar <kchudgar@apm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "xgene_enet_main.h"
#include "xgene_enet_csr.h"

inline void xgene_enet_wr32(void *addr, u32 data)
{
	pr_debug("Write addr 0x%p data 0x%08X\n", addr, data);
	writel(data, (void __iomem *)addr);
}

inline void xgene_enet_rd32(void *addr, u32 *data)
{
	*data = readl((void __iomem *)addr);
	pr_debug("data 0x%08X\n", *data);
}

inline phys_addr_t xgene_enet_enc_addr(void *vaddr)
{
	return __pa(vaddr);
}

inline void *xgene_enet_dec_addr(phys_addr_t paddr)
{
	return __va(paddr);
}

inline void xgene_enet_set_skb_data(struct xgene_qmtm_msg16 *msg16,
				    struct sk_buff *skb)
{
	u64 pa = xgene_enet_enc_addr(skb->data);
	u32 *word = &msg16->DataAddrL;

	*word++ = cpu_to_le32((u32)pa);
	*word = (*word & ~cpu_to_le32(0x3FF)) | cpu_to_le32((u32)(pa >> 32));
}

static int xgene_enet_init_fp(struct xgene_enet_qcontext *c2e, u32 nbuf)
{
	struct xgene_enet_pdev *pdev = c2e->pdev;
	struct sk_buff *skb;
	struct xgene_qmtm_msg16 *msg16;
	u32 i;

	/* Initializing common fields */
	for (i = 0; i < c2e->qdesc->count; i++) {
		msg16 = &c2e->qdesc->msg16[i];
		memset(msg16, 0, sizeof(struct xgene_qmtm_msg16));
		msg16->UserInfo = i;
		msg16->C = 1;
		msg16->BufDataLen = xgene_qmtm_encode_bufdatalen(c2e->buf_size);
		msg16->FPQNum = c2e->eqnum;
		msg16->PB = 0;
		msg16->HB = 1;
		xgene_qmtm_msg_le32(&(((u32 *)msg16)[1]), 3);
	}

	if (nbuf > c2e->qdesc->count) {
		netdev_warn(pdev->ndev,
			    "Limiting number of skb alloc to queue size\n");
		nbuf = c2e->qdesc->count;
	}

	for (i = 0; i < nbuf; i++) {
		msg16 = &c2e->qdesc->msg16[i];
		skb = dev_alloc_skb(c2e->buf_size);
		if (unlikely(!skb)) {
			netdev_err(pdev->ndev,
				   "Failed to allocate new skb size %d",
				   c2e->buf_size);
			return -ENOMEM;
		}
		skb_reserve(skb, NET_IP_ALIGN);
		c2e->skb[i] = skb;
		xgene_enet_set_skb_data(msg16, skb);
	}

	writel(nbuf, c2e->qdesc->command);

	if (nbuf == c2e->qdesc->count)
		nbuf = 0;
	c2e->qdesc->qtail = nbuf;

	return 0;
}

static int xgene_enet_refill_fp(struct xgene_enet_qcontext *c2e, u32 nbuf)
{
	register u32 qtail = c2e->qdesc->qtail;
	struct xgene_enet_pdev *pdev = c2e->pdev;
	u32 i;

	for (i = 0; i < nbuf; i++) {
		struct sk_buff *skb;
		struct xgene_qmtm_msg16 *msg16 = &c2e->qdesc->msg16[qtail];

		msg16->BufDataLen = xgene_qmtm_encode_bufdatalen(c2e->buf_size);
		skb = dev_alloc_skb(c2e->buf_size);
		if (unlikely(!skb)) {
			netdev_err(pdev->ndev,
				   "Failed to allocate new skb size %d",
				   c2e->buf_size);
			return -ENOMEM;
		}
		skb_reserve(skb, NET_IP_ALIGN);
		c2e->skb[qtail] = skb;
		xgene_enet_set_skb_data(msg16, skb);
		if (++qtail == c2e->qdesc->count)
			qtail = 0;
	}

	writel(nbuf, c2e->qdesc->command);
	c2e->qdesc->qtail = qtail;

	return 0;
}

static void xgene_enet_deinit_fp(struct xgene_enet_qcontext *c2e, int qid)
{
	u32 qtail = c2e->qdesc->qtail;
	u32 count = c2e->qdesc->count;
	u32 command = 0;
	struct xgene_enet_pdev *pdev = c2e->pdev;
	struct xgene_qmtm_msg16 *msg16;
	struct xgene_qmtm_qinfo qinfo;
	int i;

	memset(&qinfo, 0, sizeof(qinfo));
	qinfo.qmtm = pdev->sdev->qmtm;
	qinfo.queue_id = qid;
	xgene_qmtm_read_qstate(&qinfo);

	for (i = 0; i < qinfo.nummsgs; i++) {
		if (qtail == 0)
			qtail = count;

		qtail--;
		msg16 = &c2e->qdesc->msg16[qtail];
		kfree_skb(c2e->skb[msg16->UserInfo]);
		command--;
	}

	writel(command, c2e->qdesc->command);
	c2e->qdesc->qtail = qtail;
}

static int xgene_enet_change_mtu(struct net_device *ndev, int new_mtu)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;
	int eth_running;

	if (HW_MTU(new_mtu) < XGENE_ENET_MIN_MTU
	    || HW_MTU(new_mtu) > XGENE_ENET_MAX_MTU) {
		netdev_err(ndev, "Invalid MTU: %d\n", new_mtu);
		return -EINVAL;
	}

	netdev_info(ndev, "changing MTU from %d to %d\n", ndev->mtu, new_mtu);
	eth_running = netif_running(ndev);
	if (eth_running) {
		netif_stop_queue(ndev);
		xgene_enet_mac_rx_state(priv, 0);
		xgene_enet_mac_tx_state(priv, 0);
	}
	ndev->mtu = new_mtu;
	xgene_enet_mac_change_mtu(priv, HW_MTU(new_mtu));
	if (eth_running) {
		xgene_enet_mac_rx_state(priv, 1);
		xgene_enet_mac_tx_state(priv, 1);
		netif_start_queue(ndev);
	}
	return 0;
}

static int xgene_enet_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct xgene_enet_pdev *pdev = bus->priv;
	struct xgene_enet_priv *priv = &pdev->priv;
	u32 regval1;

	xgene_genericmiiphy_read(priv, mii_id, regnum, &regval1);
	pr_debug("%s: bus=%d reg=%d val=%x\n", __func__, mii_id,
		 regnum, regval1);
	return (int)regval1;
}

static int xgene_enet_mdio_write(struct mii_bus *bus, int mii_id, int regnum,
				 u16 regval)
{
	struct xgene_enet_pdev *pdev = bus->priv;
	struct xgene_enet_priv *priv = &pdev->priv;

	pr_debug("%s: bus=%d reg=%d val=%x\n", __func__, mii_id,
		 regnum, regval);
	xgene_genericmiiphy_write(priv, mii_id, regnum, regval);

	return 0;
}

static void xgene_enet_mdio_link_change(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;
	struct phy_device *phydev = pdev->phy_dev;
	int status_change = 0;

	if (phydev->link) {
		if (pdev->phy_speed != phydev->speed) {
			xgene_enet_mac_init(priv, ndev->dev_addr, phydev->speed,
					    HW_MTU(ndev->mtu), priv->crc);
			pdev->phy_speed = phydev->speed;
			status_change = 1;
		}
	}

	if (phydev->link != pdev->phy_link) {
		if (!phydev->link)
			pdev->phy_speed = 0;
		pdev->phy_link = phydev->link;
		status_change = 1;
	}

	if (status_change) {
		xgene_enet_mac_rx_state(priv, phydev->link);
		xgene_enet_mac_tx_state(priv, phydev->link);
		if (phydev->link)
			netdev_info(ndev, "%s: link up %d Mbps\n",
				    ndev->name, phydev->speed);
		else
			netdev_info(ndev, "%s: link down\n", ndev->name);
	}
}

static int xgene_enet_mdio_probe(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct phy_device *phydev = NULL;
	int phy_addr;

	/* find the first phy */
	for (phy_addr = 0; phy_addr < PHY_MAX_ADDR; phy_addr++) {
		if (pdev->mdio_bus->phy_map[phy_addr]) {
			phydev = pdev->mdio_bus->phy_map[phy_addr];
			break;
		}
	}

	if (!phydev) {
		netdev_info(ndev, "%s: no PHY found\n", ndev->name);
		return -1;
	}

	/* attach the mac to the phy */
	phydev = phy_connect(ndev, dev_name(&phydev->dev),
			     &xgene_enet_mdio_link_change,
			     PHY_INTERFACE_MODE_RGMII);

	pdev->phy_link = 0;
	pdev->phy_speed = 0;

	if (IS_ERR(phydev)) {
		pdev->phy_dev = NULL;
		netdev_err(ndev, "%s: Could not attach to PHY\n", ndev->name);
		return PTR_ERR(phydev);
	}
	pdev->phy_dev = phydev;

	netdev_info(ndev, "%s: phy_id=0x%08x phy_drv=\"%s\"",
		    ndev->name, phydev->phy_id, phydev->drv->name);

	return 0;
}

static int xgene_enet_mdio_remove(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev =
	    (struct xgene_enet_pdev *)netdev_priv(ndev);
	struct mii_bus *mdio_bus;

	mdio_bus = pdev->mdio_bus;
	mdiobus_unregister(mdio_bus);
	mdiobus_free(mdio_bus);
	pdev->mdio_bus = NULL;

	return 0;
}

static inline u32 xgene_enet_hdr_len(const void *data)
{
	const struct ethhdr *eth = data;
	return (eth->h_proto == htons(ETH_P_8021Q)) ? VLAN_ETH_HLEN : ETH_HLEN;
}

irqreturn_t xgene_enet_e2c_irq(const int irq, void *data)
{
	struct xgene_enet_qcontext *e2c = (struct xgene_enet_qcontext *)data;

	if (napi_schedule_prep(&e2c->napi)) {
		disable_irq_nosync(irq);
		__napi_schedule(&e2c->napi);
	}

	return IRQ_HANDLED;
}

static int xgene_enet_tx_completion(struct xgene_enet_qcontext *e2c,
				    struct xgene_qmtm_msg32 *msg32_1)
{
	struct sk_buff *skb;
	int rc = 0;

	skb = (struct sk_buff *)xgene_enet_dec_addr(
		((u64)msg32_1->msgup16.H0Info_msbH << 32) |
		msg32_1->msgup16.H0Info_msbL);

	if (likely(skb)) {
		dev_kfree_skb_any(skb);
	} else {
		netdev_info(e2c->pdev->ndev, "completion skb is NULL\n");
		rc = -1;
	}

	return rc;
}

static inline u16 xgene_enet_select_queue(struct net_device *ndev,
					  struct sk_buff *skb,
					  void *accel_priv)
{
	return skb_tx_hash(ndev, skb);
}

/* Checksum offload processing */
static int xgene_enet_checksum_offload(struct net_device *ndev,
				       struct sk_buff *skb,
				       struct xgene_qmtm_msg_up16 *msg_up16)
{
	u32 maclen, nr_frags, ihl;
	struct iphdr *iph;
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	int rc = 0;

	if (unlikely(!(ndev->features & NETIF_F_IP_CSUM)))
		goto out;

	if (unlikely(skb->protocol != htons(ETH_P_IP)) &&
	    unlikely(skb->protocol != htons(ETH_P_8021Q)))
		goto out;

	nr_frags = skb_shinfo(skb)->nr_frags;
	maclen = xgene_enet_hdr_len(skb->data);
	iph = ip_hdr(skb);
	ihl = ip_hdrlen(skb) >> 2;

	if (unlikely(iph->frag_off & htons(IP_MF | IP_OFFSET)))
		goto out;

	if (likely(iph->protocol == IPPROTO_TCP)) {
		int xhlen, mss_len;
		u32 mss, all_hdr_len;

		xhlen = tcp_hdrlen(skb) / 4;
		msg_up16->H0Info_lsbL |=
		    (xhlen & TSO_TCP_HLEN_MASK) |
		    ((ihl & TSO_IP_HLEN_MASK) << 6) |
		    (TSO_CHKSUM_ENABLE << 22) | (TSO_IPPROTO_TCP << 24);

		netdev_dbg(ndev,
		   "Checksum Offload H0Info 0x%04X%08X H1Info 0x%04X%08X\n",
		   msg_up16->H0Info_lsbH, msg_up16->H0Info_lsbL,
		   msg_up16->H0Info_msbH, msg_up16->H0Info_msbL);

		if (unlikely(!(ndev->features & NETIF_F_TSO)))
			goto out;

		/* TCP Segmentation offload processing */
		mss = skb_shinfo(skb)->gso_size;
		all_hdr_len = maclen + ip_hdrlen(skb) + tcp_hdrlen(skb);
		mss_len = skb->len - all_hdr_len;

		/* HW requires all header resides in the first buffer */
		if (nr_frags && (skb_headlen(skb) < all_hdr_len)) {
			netdev_err(ndev,
				   "Unsupported header len location by Eth HW\n");
			pdev->stats.estats.tx_dropped++;
			dev_kfree_skb(skb);
			rc = -1;
			goto out;
		}

		if (!mss || mss_len <= mss)
			goto out;

		if (mss != pdev->mss) {
			xgene_enet_tx_offload(&pdev->priv, XGENE_ENET_MSS0,
					      mss);
			pdev->mss = mss;
		}

		msg_up16->H0Info_lsbL |= ((0 & TSO_MSS_MASK) << 20) |
		    ((TSO_ENABLE & TSO_ENABLE_MASK) << 23);
		netdev_dbg(ndev,
			"TSO H0Info 0x%04X%08X H1Info 0x%04X%08X mss %d\n",
			msg_up16->H0Info_lsbH, msg_up16->H0Info_lsbL,
			msg_up16->H0Info_msbH, msg_up16->H0Info_msbL, mss);
	} else if (iph->protocol == IPPROTO_UDP) {
		msg_up16->H0Info_lsbL |= (UDP_HDR_SIZE & TSO_TCP_HLEN_MASK)
		    | ((ihl & TSO_IP_HLEN_MASK) << 6)
		    | (TSO_CHKSUM_ENABLE << 22)
		    | (TSO_IPPROTO_UDP << 24);
		netdev_dbg(ndev,
			"Csum Offload H0Info 0x%04X%08X H1Info 0x%04X%08X\n",
			msg_up16->H0Info_lsbH, msg_up16->H0Info_lsbL,
			msg_up16->H0Info_msbH, msg_up16->H0Info_msbL);
	} else {
		msg_up16->H0Info_lsbL |= ((ihl & TSO_IP_HLEN_MASK) << 6);
	}
out:
	return rc;
}

static void xgene_enet_process_frags(struct net_device *ndev,
				     struct xgene_qmtm_msg16 *msg16,
				     struct xgene_enet_qcontext *c2e,
				     struct sk_buff *skb)
{
	struct xgene_qmtm_msg_up16 *msg_up16;
	struct xgene_qmtm_msg_ext32 *msg32_2;
	struct xgene_qmtm_msg_ext8 *ext_msg;
	struct xgene_qmtm_msg_ll8 *ext_msg_ll8;
	u32 qtail = c2e->qdesc->qtail;
	phys_addr_t paddr = virt_to_phys(skb->data);
	u32 nr_frags = skb_shinfo(skb)->nr_frags;
	skb_frag_t *frag = NULL;
	u8 *vaddr = NULL;
	int frag_no = 0, len = 0, offset = 0;
	int ell_bcnt = 0, ell_cnt = 0, i;

	msg_up16 = (struct xgene_qmtm_msg_up16 *)&msg16[1];
	msg32_2 = (struct xgene_qmtm_msg_ext32 *)&c2e->qdesc->msg32[qtail];

	if (++qtail == c2e->qdesc->count)
		qtail = 0;

	memset(msg32_2, 0, sizeof(struct xgene_qmtm_msg_ext32));

	/* First Fragment, 64B message */
	msg16->BufDataLen = xgene_qmtm_encode_datalen(skb_headlen(skb));
	msg16->DataAddrL = (u32)paddr;
	msg16->DataAddrH = (u32)(paddr >> 32);
	msg16->NV = 1;

	/* 2nd, 3rd, and 4th fragments */
	ext_msg = &msg32_2->msg8_1;

	/* Terminate next pointers, will be updated later as required */
	msg32_2->msg8_2.NxtBufDataLength = 0x7800;
	msg32_2->msg8_3.NxtBufDataLength = 0x7800;
	msg32_2->msg8_4.NxtBufDataLength = 0x7800;

	for (i = 0; i < 3 && frag_no < nr_frags; i++) {
		if (!vaddr) {
			frag = &skb_shinfo(skb)->frags[frag_no];
			len = frag->size;
			vaddr = skb_frag_address(frag);
			offset = 0;
			netdev_dbg(ndev, "SKB Frag[%d] 0x%p len %d\n",
				   frag_no, vaddr, len);
		}
		paddr = virt_to_phys(vaddr + offset);
		ext_msg->NxtDataAddrL = (u32)paddr;
		ext_msg->NxtDataAddrH = (u32)(paddr >> 32);

		if (len <= 16 * 1024) {
			/* Encode using 16K buffer size format */
			ext_msg->NxtBufDataLength =
			    xgene_qmtm_encode_datalen(len);
			vaddr = NULL;
			frag_no++;
		} else {
			len -= 16 * 1024;
			offset += 16 * 1024;
			/* Encode using 16K buffer size format */
			ext_msg->NxtBufDataLength = 0;
		}

		netdev_dbg(ndev, "Frag[%d] PADDR 0x%04X%08X len %d\n", i,
			   ext_msg->NxtDataAddrH, ext_msg->NxtDataAddrL,
			   ext_msg->NxtBufDataLength);
		ext_msg = (struct xgene_qmtm_msg_ext8 *)
		    (((u8 *) msg32_2) + (8 * ((i + 1) ^ 1)));
	}

	/* Determine no more fragment, last one, or more than one */
	if (!vaddr) {
		/* Check next fragment */
		if (frag_no >= nr_frags) {
			goto out;
		} else {
			frag = &skb_shinfo(skb)->frags[frag_no];
			if (frag->size <= 16 * 1024
			    && (frag_no + 1) >= nr_frags)
				goto one_more_frag;
			else
				goto more_than_one_frag;
		}
	} else if (len <= 16 * 1024) {
		/* Current fragment <= 16K, check if last fragment */
		if ((frag_no + 1) >= nr_frags)
			goto one_more_frag;
		else
			goto more_than_one_frag;
	} else {
		/* Current fragment requires two pointers */
		goto more_than_one_frag;
	}

one_more_frag:
	if (!vaddr) {
		frag = &skb_shinfo(skb)->frags[frag_no];
		len = frag->size;
		vaddr = skb_frag_address(frag);
		offset = 0;
		netdev_dbg(ndev, "SKB Frag[%d] 0x%p len %d\n",
			   frag_no, vaddr, len);
	}

	paddr = virt_to_phys(vaddr + offset);
	ext_msg->NxtDataAddrL = (u32)paddr;
	ext_msg->NxtDataAddrH = (u32)(paddr >> 32);
	/* Encode using 16K buffer size format */
	ext_msg->NxtBufDataLength = xgene_qmtm_encode_datalen(len);
	netdev_dbg(ndev, "Frag[%d] PADDR 0x%04X%08X len %d\n", i,
		   ext_msg->NxtDataAddrH, ext_msg->NxtDataAddrL,
		   ext_msg->NxtBufDataLength);
	goto out;

more_than_one_frag:
	msg16->LL = 1;		/* Extended link list */
	ext_msg_ll8 = &msg32_2->msg8_ll;
	ext_msg = &c2e->msg8[qtail * 256];
	memset(ext_msg, 0, 255 * sizeof(struct xgene_qmtm_msg_ext8));
	paddr = virt_to_phys(ext_msg);
	ext_msg_ll8->NxtDataPtrL = (u32)paddr;
	ext_msg_ll8->NxtDataPtrH = (u32)(paddr >> 32);

	for (i = 0; i < 255 && frag_no < nr_frags;) {
		if (vaddr == NULL) {
			frag = &skb_shinfo(skb)->frags[frag_no];
			len = frag->size;
			vaddr = skb_frag_address(frag);
			offset = 0;
			netdev_dbg(ndev, "SKB Frag[%d] 0x%p len %d\n",
				   frag_no, vaddr, len);
		}
		paddr = virt_to_phys(vaddr + offset);
		ext_msg[i ^ 1].NxtDataAddrL = (u32)paddr;
		ext_msg[i ^ 1].NxtDataAddrH = (u32)(paddr >> 32);

		if (len <= 16 * 1024) {
			/* Encode using 16K buffer size format */
			ext_msg[i ^ 1].NxtBufDataLength =
			    xgene_qmtm_encode_datalen(len);
			ell_bcnt += len;
			vaddr = NULL;
			frag_no++;
		} else {
			len -= 16 * 1024;
			offset += 16 * 1024;
			ell_bcnt += 16 * 1024;
		}

		ell_cnt++;
		netdev_dbg(ndev, "Frag ELL[%d] PADDR 0x%04X%08X len %d\n", i,
			   ext_msg[i ^ 1].NxtDataAddrH,
			   ext_msg[i ^ 1].NxtDataAddrL,
			   ext_msg[i ^ 1].NxtBufDataLength);
		i++;
		xgene_qmtm_msg_le32((u32 *)&ext_msg[i ^ 1], 2);
	}

	/* Encode the extended link list byte count and link count */
	ext_msg_ll8->NxtLinkListength = ell_cnt;
	msg_up16->TotDataLengthLinkListLSBs = (ell_bcnt & 0xFFF);
	ext_msg_ll8->TotDataLengthLinkListMSBs = ((ell_bcnt & 0xFF000) >> 12);

out:
	xgene_qmtm_msg_le32((u32 *)msg32_2, 8);
	c2e->qdesc->qtail = qtail;
}

/* Packet transmit function */
static netdev_tx_t xgene_enet_start_xmit(struct sk_buff *skb,
					 struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_qcontext *c2e = pdev->tx[skb->queue_mapping];
	struct xgene_qmtm_msg16 *msg16;
	struct xgene_qmtm_msg_up16 *msg_up16;
	u64 pa = xgene_enet_enc_addr((void *)skb);
	u32 nr_frags = skb_shinfo(skb)->nr_frags;
	u32 nummsgs = (readl(c2e->nummsgs) & 0x1fffe) >> 1;
	u32 cmd = 1;

	msg16 =
	    (struct xgene_qmtm_msg16 *)&c2e->qdesc->msg32[c2e->qdesc->qtail];
	msg_up16 = (struct xgene_qmtm_msg_up16 *)&msg16[1];

	if (nummsgs > pdev->tx_cqt_hi) {
		do {
			nummsgs = (readl(c2e->nummsgs) & 0x1fffe) >> 1;
		} while (nummsgs < pdev->tx_cqt_low);
	}

	if (++c2e->qdesc->qtail == c2e->qdesc->count)
		c2e->qdesc->qtail = 0;

	memset(msg16, 0, sizeof(struct xgene_qmtm_msg32));

	if (likely(nr_frags == 0)) {
		skb->len = (skb->len < 60) ? 60 : skb->len;
		msg16->BufDataLen = xgene_qmtm_encode_datalen(skb->len);
		msg16->DataAddrL = (u32)virt_to_phys(skb->data);
		msg16->DataAddrH = (u32)(virt_to_phys(skb->data) >> 32);
	} else {
		xgene_enet_process_frags(ndev, msg16, c2e, skb);
		cmd = 2;
	}

	msg_up16->H0Info_msbL = (u32)pa;
	msg_up16->H0Info_msbH = (u32)(pa >> 32);
	msg_up16->H0Enq_Num = c2e->eqnum;
	msg16->C = 1;

	/* Set TYPE_SEL for egress work message */
	msg_up16->H0Info_lsbH = TYPE_SEL_WORK_MSG << 12;

	/* Enable CRC insertion */
	if (!pdev->priv.crc)
		msg_up16->H0Info_lsbH |= (TSO_INS_CRC_ENABLE << 3);

	/* Setup mac header length H0Info */
	msg_up16->H0Info_lsbL |=
	    ((xgene_enet_hdr_len(skb->data) & TSO_ETH_HLEN_MASK) << 12);

	if (unlikely(xgene_enet_checksum_offload(ndev, skb, msg_up16)))
		return NETDEV_TX_OK;

	/* xmit: Push the work message to ENET HW */
	netdev_dbg(ndev, "TX CQID %d Addr 0x%04x%08x len %d\n",
		   msg_up16->H0Enq_Num, msg16->DataAddrH,
		   msg16->DataAddrL, msg16->BufDataLen);
	xgene_qmtm_msg_le32(&(((u32 *)msg16)[1]), 7);
	writel(cmd, c2e->qdesc->command);

	ndev->trans_start = jiffies;
	return NETDEV_TX_OK;
}

int xgene_enet_check_skb(struct net_device *ndev,
			 struct sk_buff *skb,
			 struct xgene_qmtm_msg32 *msg32_1, u32 qid)
{
	struct xgene_qmtm_msg16 *msg16 = &msg32_1->msg16;
	u32 UserInfo = msg16->UserInfo;
	u8 NV = msg16->NV;
	int rc = 0;

	if (unlikely(!skb)) {
		netdev_err(ndev, "ENET skb NULL UserInfo %d QID %d FP 0x%x\n",
			   UserInfo, qid, msg16->FPQNum);
		print_hex_dump(KERN_INFO, "QM msg:",
			       DUMP_PREFIX_ADDRESS, 16, 4, msg32_1,
			       NV ? 64 : 32, 1);
		rc = -1;
		goto out;
	}

	if (unlikely(!skb->head) || unlikely(!skb->data)) {
		netdev_err(ndev, "ENET skb 0x%p head 0x%p data 0x%p FP 0x%x\n",
			   skb, skb->head, skb->data, msg16->FPQNum);
		print_hex_dump(KERN_INFO, "QM msg:",
			       DUMP_PREFIX_ADDRESS, 16, 4, msg32_1,
			       NV ? 64 : 32, 1);
		rc = -1;
		goto out;
	}

	if (unlikely(skb->len)) {
		netdev_err(ndev, "ENET skb 0x%p len %d FP 0x%x\n", skb,
			   skb->len, msg16->FPQNum);
		print_hex_dump(KERN_INFO, "QM msg:",
			       DUMP_PREFIX_ADDRESS, 16, 4, msg32_1,
			       NV ? 64 : 32, 1);
		rc = -1;
		goto out;
	}

out:
	return rc;
}

inline void xgene_enet_skip_csum(struct sk_buff *skb)
{
	struct iphdr *iph = (struct iphdr *)skb->data;
	if (likely(!(iph->frag_off & htons(IP_MF | IP_OFFSET)))
	    || likely(iph->protocol != IPPROTO_TCP
		      && iph->protocol != IPPROTO_UDP)) {
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
}

/* Process received frame */
static int xgene_enet_rx_frame(struct xgene_enet_qcontext *e2c,
			       struct xgene_qmtm_msg32 *msg32_1)
{
	struct xgene_enet_qcontext *c2e = e2c->c2e_skb;
	struct xgene_enet_pdev *pdev = e2c->pdev;
	struct net_device *ndev = pdev->ndev;
	struct xgene_qmtm_msg16 *msg16 = &msg32_1->msg16;
	struct sk_buff *skb = NULL;
	u32 data_len = xgene_qmtm_decode_datalen(msg16->BufDataLen);
	u8 NV = msg16->NV;
	u8 LErr = ((u8) msg16->ELErr << 3) | msg16->LErr;
	u32 UserInfo = msg16->UserInfo;
	u32 qid = pdev->qm_queues.rx[e2c->queue_index].qid;

	if (unlikely(UserInfo >= c2e->qdesc->count)) {
		netdev_err(ndev, "ENET: invalid UserInfo %d QID %d FP 0x%x\n",
			   UserInfo, qid, msg16->FPQNum);
		print_hex_dump(KERN_INFO, "QM msg:",
			       DUMP_PREFIX_ADDRESS, 16, 4, msg32_1,
			       NV ? 64 : 32, 1);
		goto err_refill;
	}

	skb = c2e->skb[UserInfo];
	if (unlikely(xgene_enet_check_skb(ndev, skb, msg32_1, qid)))
		goto err_refill;

	/* Check for error, if packet received with error */
	if (unlikely(LErr)) {
		if (LErr == 0x15)	/* ignore rx queue full error */
			goto process_pkt;
		if (LErr == 0x10 || LErr == 0x11) {
			LErr = 0;
			goto process_pkt;
		}
		if (LErr == 0x10 || LErr == 5) {
			LErr = 0;
			goto process_pkt;
		}

		xgene_enet_parse_error(LErr, qid);
		netdev_dbg(ndev, "ENET LErr 0x%x skb 0x%p FP 0x%x\n",
			   LErr, skb, msg16->FPQNum);
		print_hex_dump(KERN_ERR, "QM Msg: ",
			       DUMP_PREFIX_ADDRESS, 16, 4, msg32_1,
			       NV ? 64 : 32, 1);
		goto err_refill;
	}

process_pkt:
	prefetch(skb->data - NET_IP_ALIGN);

	if (likely(!NV)) {
		/* Strip off CRC as HW isn't doing this */
		data_len -= 4;
		skb_put(skb, data_len);
		netdev_dbg(ndev, "RX SKB len %d\n", data_len);
	}

	if (--e2c->c2e_count == 0) {
		xgene_enet_refill_fp(c2e, 32);
		e2c->c2e_count = 32;
	}

	if (pdev->num_rx_queues > 1)
		skb_record_rx_queue(skb, e2c->queue_index);

	skb->protocol = eth_type_trans(skb, ndev);
	if (likely(ndev->features & NETIF_F_IP_CSUM)
	    && likely(LErr == 0)
	    && likely(skb->protocol == htons(ETH_P_IP))) {
		xgene_enet_skip_csum(skb);
	}

	napi_gro_receive(&e2c->napi, skb);
	return 0;

err_refill:
	if (skb != NULL)
		dev_kfree_skb_any(skb);

	xgene_enet_refill_fp(e2c->c2e_skb, 1);

	if (LErr != 0x15)
		pdev->stats.estats.rx_hw_errors++;
	else
		pdev->stats.estats.rx_hw_overrun++;

	return -1;
}

static int xgene_enet_dequeue_msg(struct xgene_enet_qcontext *e2c, int budget)
{
	u32 processed = 0;
	u32 command = 0;
	u32 qhead = e2c->qdesc->qhead;
	u32 count = e2c->qdesc->count;
	u16 nummsgs;

	while (budget--) {
		struct xgene_qmtm_msg32 *msg32_1 = &e2c->qdesc->msg32[qhead];
		struct xgene_qmtm_msg_ext32 *msg32_2 = NULL;

		if (unlikely(((u32 *) msg32_1)[EMPTY_SLOT_INDEX] == EMPTY_SLOT))
			break;

		command--;
		if (++qhead == count)
			qhead = 0;

		xgene_qmtm_msg_le32(&(((u32 *)msg32_1)[1]), 7);

		if (msg32_1->msg16.NV) {
			msg32_2 = (struct xgene_qmtm_msg_ext32 *)
			    &e2c->qdesc->msg32[qhead];
			if (unlikely(((u32 *) msg32_2)[EMPTY_SLOT_INDEX]
				     == EMPTY_SLOT)) {
				command++;
				if (!qhead)
					qhead = count;
				qhead--;
				xgene_qmtm_msg_le32(&(((u32 *)msg32_1)[1]), 7);
				break;
			}
			command--;
			if (++qhead == count)
				qhead = 0;
			xgene_qmtm_msg_le32((u32 *)msg32_2, 8)
		}

		if (msg32_1->msg16.FPQNum)
			xgene_enet_rx_frame(e2c, msg32_1);
		else
			xgene_enet_tx_completion(e2c, msg32_1);

		((u32 *) msg32_1)[EMPTY_SLOT_INDEX] = EMPTY_SLOT;
		if (msg32_2)
			((u32 *) msg32_2)[EMPTY_SLOT_INDEX] = EMPTY_SLOT;
		processed++;
	}

	do {
		nummsgs = (readl(e2c->nummsgs) & 0x1fffe) >> 1;
	} while (nummsgs < (1 + ~command));
	writel(command, e2c->qdesc->command);
	e2c->qdesc->qhead = qhead;

	return processed;
}

static int xgene_enet_napi(struct napi_struct *napi, const int budget)
{
	struct xgene_enet_qcontext *e2c =
	    container_of(napi, struct xgene_enet_qcontext, napi);
	int processed = xgene_enet_dequeue_msg(e2c, budget);

	if (processed != budget) {
		napi_complete(napi);
		enable_irq(e2c->qdesc->irq);
	}

	return processed;
}

static void xgene_enet_timeout(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	xgene_enet_mac_reset(&pdev->priv);
}

static void xgene_enet_napi_add(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		netif_napi_add(pdev->ndev, &pdev->rx[qindex]->napi,
			       xgene_enet_napi, 64);
}

static void xgene_enet_napi_del(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		netif_napi_del(&pdev->rx[qindex]->napi);
}

static void xgene_enet_napi_enable(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		napi_enable(&pdev->rx[qindex]->napi);
}

static void xgene_enet_napi_disable(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		napi_disable(&pdev->rx[qindex]->napi);
}

static void xgene_enet_irq_enable(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		enable_irq(pdev->rx[qindex]->qdesc->irq);
}

static void xgene_enet_irq_disable_all(struct xgene_enet_pdev *pdev)
{
	u32 qindex;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		disable_irq_nosync(pdev->rx[qindex]->qdesc->irq);
}

static int xgene_enet_open(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;

	xgene_enet_napi_enable(pdev);
	xgene_enet_irq_enable(pdev);

	netif_tx_start_all_queues(ndev);
	netif_carrier_on(ndev);

	if (pdev->phy_dev)
		phy_start(pdev->phy_dev);

	xgene_enet_mac_tx_state(priv, 1);
	xgene_enet_mac_rx_state(priv, 1);

	return 0;
}

static int xgene_enet_close(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &pdev->priv;
	u32 qindex;

	netif_tx_stop_all_queues(ndev);
	netif_carrier_off(ndev);
	netif_tx_disable(ndev);

	if (pdev->phy_dev)
		phy_stop(pdev->phy_dev);

	xgene_enet_mac_tx_state(priv, 0);
	xgene_enet_mac_rx_state(priv, 0);

	xgene_enet_irq_disable_all(pdev);
	xgene_enet_napi_disable(pdev);

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++)
		xgene_enet_dequeue_msg(pdev->rx[qindex], -1);

	return 0;
}

static struct xgene_enet_qcontext *xgene_enet_allocq(struct xgene_enet_pdev
						     *pdev,
						     struct xgene_qmtm_qinfo
						     *qinfo,
						     struct xgene_qmtm_sdev
						     *sdev, u8 qtype, u8 qsize)
{
	struct xgene_enet_qcontext *qc;

	memset(qinfo, 0, sizeof(struct xgene_qmtm_qinfo));
	qinfo->sdev = sdev;
	qinfo->qaccess = QACCESS_ALT;
	qinfo->qtype = qtype;
	qinfo->qsize = qsize;
	qinfo->flags = XGENE_SLAVE_DEFAULT_FLAGS;

	if (xgene_qmtm_set_qinfo(qinfo)) {
		netdev_err(pdev->ndev, "Could not allocate queue\n");
		return NULL;
	}

	qc = (struct xgene_enet_qcontext *)
	    kmalloc(sizeof(struct xgene_enet_qcontext),
		    GFP_KERNEL | __GFP_ZERO);
	qc->nummsgs = &(((u32 *) qinfo->qfabric)[1]);
	qc->qdesc = qinfo->qdesc;
	qc->pdev = pdev;

	return qc;
}

static int xgene_enet_qconfig(struct xgene_enet_pdev *pdev)
{
	struct xgene_qmtm_qinfo qinfo;
	struct xgene_qmtm_sdev *sdev = pdev->sdev;
	struct xgene_qmtm_sdev *idev = pdev->sdev->idev;
	int qmtm_ip = sdev->qmtm_ip;
	int rc = 0;
	u32 qindex;
	struct xgene_enet_qcontext *e2c;
	struct xgene_enet_qcontext *c2e;

	memset(&pdev->qm_queues, 0, sizeof(struct eth_queue_ids));
	pdev->qm_queues.qm_ip = qmtm_ip;

	for (qindex = 0; qindex < pdev->num_tx_queues; qindex++) {
		/* Allocate EGRESS work queues from CPUx to ETHx */
		c2e = xgene_enet_allocq(pdev, &qinfo, sdev,
					      QTYPE_PQ, QSIZE_64KB);
		if (!c2e)
			goto out;

		pdev->qm_queues.tx[qindex].qid = qinfo.queue_id;

		/* Setup TX Frame cpu_to_enet info */
		c2e->msg8 =
		    (struct xgene_qmtm_msg_ext8 *)
		    kmalloc(sizeof(struct xgene_qmtm_msg_ext8) * 256 *
			    c2e->qdesc->count, GFP_KERNEL);
		c2e->queue_index = qindex;
		pdev->tx[qindex] = c2e;
		/* Assign TX completn queue threshold based on rx queue size */
		pdev->tx_cqt_hi = c2e->qdesc->count / 4;
		pdev->tx_cqt_low = pdev->tx_cqt_low / 16;
	}

	pdev->qm_queues.default_tx_qid = pdev->qm_queues.tx[0].qid;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++) {
		/* Allocate INGRESS work queue from ETHx to CPUx */
		u8 qsize = QSIZE_512KB;
		e2c = xgene_enet_allocq(pdev, &qinfo, idev,
					      QTYPE_PQ, qsize);
		if (!e2c)
			goto out;

		pdev->qm_queues.rx[qindex].qid = qinfo.queue_id;
		e2c->queue_index = qindex;
		snprintf(e2c->irq_name, sizeof(e2c->irq_name), "%s-rx%d",
			 pdev->ndev->name, qindex);
		e2c->c2e_count = 1;
		pdev->rx[qindex] = e2c;

		/* Allocate free pool for ETHx from CPUx */
		c2e = xgene_enet_allocq(pdev, &qinfo, sdev,
					      QTYPE_FP, QSIZE_16KB);
		if (!c2e)
			goto out;

		c2e->skb = kmalloc(sizeof (struct sk_buff *) *
			qinfo.qdesc->count, GFP_KERNEL | __GFP_ZERO);
		pdev->qm_queues.rx_fp[qindex].qid = qinfo.queue_id;
		pdev->qm_queues.rx_fp[qindex].pbn = qinfo.pbn;

		c2e->eqnum = QMTM_QUEUE_ID(qmtm_ip, qinfo.queue_id);
		c2e->buf_size = XGENE_ENET_PKT_BUF_SIZE;
		pdev->rx_skb_pool[qindex] = c2e;
		pdev->rx[qindex]->c2e_skb = pdev->rx_skb_pool[qindex];

		/* Configure free pool */
		xgene_enet_init_fp(pdev->rx_skb_pool[qindex],
				   pdev->rx_buff_cnt);
	}

	for (qindex = 0; qindex < pdev->num_tx_queues; qindex++) {
		u32 cqindex = pdev->num_tx_queues - qindex - 1;
		u32 rqindex = qindex % pdev->num_rx_queues;

		pdev->tx[cqindex]->nummsgs = pdev->rx[rqindex]->nummsgs;
		pdev->tx[cqindex]->eqnum = QMTM_QUEUE_ID(qmtm_ip,
							 pdev->qm_queues.
							 rx[rqindex].qid);
	}

	pdev->qm_queues.default_hw_tx_qid = pdev->qm_queues.hw_tx[0].qid;
	pdev->qm_queues.default_rx_qid = pdev->qm_queues.rx[0].qid;
	pdev->qm_queues.default_rx_fp_qid = pdev->qm_queues.rx_fp[0].qid;
	pdev->qm_queues.default_rx_fp_pbn = pdev->qm_queues.rx_fp[0].pbn;
	pdev->qm_queues.default_rx_nxtfp_qid = pdev->qm_queues.rx_nxtfp[0].qid;
	pdev->qm_queues.default_rx_nxtfp_pbn = pdev->qm_queues.rx_nxtfp[0].pbn;

	netdev_dbg(pdev->ndev, "CQID %d FP %d FP PBN %d\n",
		   pdev->qm_queues.default_comp_qid,
		   pdev->qm_queues.default_rx_fp_qid,
		   pdev->qm_queues.default_rx_fp_pbn);

out:
	return rc;
}

static void xgene_enet_delete_queue(struct xgene_enet_pdev *pdev)
{
	struct xgene_qmtm_qinfo qinfo;
	u32 qindex;
	u8 qmtm_ip = pdev->sdev->qmtm_ip;
	u16 queue_id;

	qinfo.qmtm_ip = qmtm_ip;

	for (qindex = 0; qindex < pdev->num_tx_queues; qindex++) {
		queue_id = pdev->qm_queues.tx[qindex].qid;

		if (queue_id) {
			qinfo.queue_id = queue_id;
			xgene_qmtm_clr_qinfo(&qinfo);
		}
	}

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++) {
		queue_id = pdev->qm_queues.rx[qindex].qid;

		if (queue_id) {
			qinfo.queue_id = queue_id;
			xgene_qmtm_clr_qinfo(&qinfo);
		}

		queue_id = pdev->qm_queues.rx_fp[qindex].qid;

		if (queue_id) {
			qinfo.queue_id = queue_id;
			xgene_qmtm_clr_qinfo(&qinfo);
		}
	}
}

static struct net_device_stats *xgene_enet_stats(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &(pdev->priv);
	struct net_device_stats *nst = &pdev->nstats;
	struct xgene_enet_detailed_stats detailed_stats;
	struct xgene_enet_rx_stats *rx_stats;
	struct xgene_enet_tx_stats *tx_stats;
	u32 pkt_bytes, crc_bytes = 4;

	memset(&detailed_stats, 0, sizeof(struct xgene_enet_detailed_stats));

	rx_stats = &detailed_stats.rx_stats;
	tx_stats = &detailed_stats.tx_stats;

	local_irq_disable();
	xgene_enet_get_stats(priv, &detailed_stats);

	pkt_bytes = rx_stats->rx_byte_count;
	pkt_bytes -= (rx_stats->rx_packet_count * crc_bytes);
	nst->rx_packets += rx_stats->rx_packet_count;
	nst->rx_bytes += pkt_bytes;

	pkt_bytes = tx_stats->tx_byte_count;
	pkt_bytes -= (tx_stats->tx_pkt_count * crc_bytes);
	nst->tx_packets += tx_stats->tx_pkt_count;
	nst->tx_bytes += pkt_bytes;

	nst->rx_dropped += rx_stats->rx_drop_pkt_count;
	nst->tx_dropped += tx_stats->tx_drop_frm_count;

	nst->rx_crc_errors += rx_stats->rx_fcs_err_count;
	nst->rx_length_errors += rx_stats->rx_frm_len_err_pkt_count;
	nst->rx_frame_errors += rx_stats->rx_alignment_err_pkt_count;
	nst->rx_over_errors += (rx_stats->rx_oversize_pkt_count
				+ pdev->stats.estats.rx_hw_overrun);

	nst->rx_errors += (rx_stats->rx_fcs_err_count
			   + rx_stats->rx_frm_len_err_pkt_count
			   + rx_stats->rx_oversize_pkt_count
			   + rx_stats->rx_undersize_pkt_count
			   + pdev->stats.estats.rx_hw_overrun
			   + pdev->stats.estats.rx_hw_errors);

	nst->tx_errors += tx_stats->tx_fcs_err_frm_count +
	    tx_stats->tx_undersize_frm_count;

	local_irq_enable();

	pdev->stats.estats.rx_hw_errors = 0;
	pdev->stats.estats.rx_hw_overrun = 0;

	return nst;
}

static int xgene_enet_set_mac_address(struct net_device *ndev, void *p)
{
	struct xgene_enet_pdev *pdev = netdev_priv(ndev);
	struct xgene_enet_priv *priv = &(pdev->priv);
	struct sockaddr *addr = p;

	if (netif_running(ndev))
		return -EBUSY;

	memcpy(ndev->dev_addr, addr->sa_data, ndev->addr_len);
	xgene_enet_set_mac_addr(priv, (unsigned char *)(ndev->dev_addr));
	return 0;
}

/* net_device_ops structure for data path ethernet */
static const struct net_device_ops apm_dnetdev_ops = {
	.ndo_open = xgene_enet_open,
	.ndo_stop = xgene_enet_close,
	.ndo_select_queue = xgene_enet_select_queue,
	.ndo_start_xmit = xgene_enet_start_xmit,
	.ndo_tx_timeout = xgene_enet_timeout,
	.ndo_get_stats = xgene_enet_stats,
	.ndo_change_mtu = xgene_enet_change_mtu,
	.ndo_set_mac_address = xgene_enet_set_mac_address,
};

static void xgene_enet_register_irq(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev;
	struct device *dev;
	u32 qindex;

	pdev = (struct xgene_enet_pdev *)netdev_priv(ndev);
	dev = &pdev->plat_dev->dev;

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++) {
		if (devm_request_irq(dev, pdev->rx[qindex]->qdesc->irq,
				xgene_enet_e2c_irq, 0,
				pdev->rx[qindex]->irq_name,
				(void *)pdev->rx[qindex]) != 0) {
			netdev_err(ndev, "request_irq failed %d for RX Frame\n",
				   pdev->rx[qindex]->qdesc->irq);
			return;
		}

		/* Disable interrupts for RX queue mailboxes */
		disable_irq_nosync(pdev->rx[qindex]->qdesc->irq);
	}

	xgene_enet_register_err_irqs(ndev);
}

static int xgene_enet_get_resources(struct xgene_enet_pdev *pdev)
{
	struct platform_device *plat_dev;
	struct net_device *ndev;
	struct device *dev;
	struct xgene_enet_priv *priv;
	struct xgene_qmtm_sdev *sdev;
	struct xgene_enet_platform_data pdata;
	struct resource *res;
	u64 csr_paddr;
	void *csr_addr;
	int i, rc = 0;

	plat_dev = pdev->plat_dev;
	dev = &plat_dev->dev;
	ndev = pdev->ndev;
	priv = &pdev->priv;

	res = platform_get_resource(plat_dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Unable to retrieve ENET Port CSR region\n");
		rc = -ENODEV;
		goto out;
	}
	csr_paddr = res->start;
	csr_addr = devm_ioremap(&plat_dev->dev, csr_paddr, resource_size(res));
	priv->ppaddr_base = csr_paddr;
	priv->vpaddr_base = csr_addr;

	res = platform_get_resource(plat_dev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "Unable to retrieve ENET Global CSR region\n");
		rc = -ENODEV;
		goto out;
	}
	csr_paddr = res->start;
	csr_addr = devm_ioremap(&plat_dev->dev, csr_paddr, resource_size(res));
	priv->paddr_base = csr_paddr;
	priv->vaddr_base = csr_addr;

	res = platform_get_resource(plat_dev, IORESOURCE_MEM, 2);
	if (!res) {
		dev_err(dev, "Unable to retrieve ENET MII access region\n");
		rc = -ENODEV;
		goto out;
	}
	csr_paddr = res->start;
	csr_addr = devm_ioremap(&plat_dev->dev, csr_paddr, resource_size(res));
	priv->vmii_base = csr_addr;

	rc = of_property_read_string(plat_dev->dev.of_node, "slave-name",
				     &pdata.sname);

	sdev = xgene_qmtm_get_sdev((char *)pdata.sname);
	if (!sdev) {
		dev_err(dev, "QMTM Slave %s error\n", pdata.sname);
		rc = -ENODEV;
		goto out;
	}
	pdev->sdev = sdev;

	pdata.irq = platform_get_irq(plat_dev, 0);
	if (pdata.irq <= 0) {
		dev_err(dev, "Unable to get ENET Error IRQ\n");
		rc = pdata.irq;
		goto out;
	}
	pdev->enet_err_irq = pdata.irq;

	pdata.irq = platform_get_irq(plat_dev, 1);
	if (pdata.irq <= 0) {
		dev_err(dev, "Unable to get ENET MAC Error IRQ\n");
		rc = pdata.irq;
		goto out;
	}
	pdev->enet_mac_err_irq = pdata.irq;

	pdata.irq = platform_get_irq(plat_dev, 2);
	if (pdata.irq <= 0) {
		dev_err(dev, "Unable to get ENET QMI Error IRQ\n");
		rc = pdata.irq;
		goto out;
	}
	pdev->enet_qmi_err_irq = pdata.irq;

	rc = of_property_read_u32(plat_dev->dev.of_node, "phyid",
				  &pdata.phy_id);
	if (rc || pdata.phy_id > 0x1F) {
		dev_err(dev, "No phy ID or invalid value in DTS\n");
		rc = -EINVAL;
		goto out;
	}
	priv->phy_addr = pdata.phy_id;

	rc = of_property_read_u8_array(plat_dev->dev.of_node,
				       "local-mac-address", pdata.ethaddr,
				       ARRAY_SIZE(pdata.ethaddr));
	if (rc) {
		dev_err(dev, "Can't get Device MAC address\n");
	} else {
		for (i = 0; i < ETH_ALEN; i++)
			ndev->dev_addr[i] = pdata.ethaddr[i] & 0xff;
	}

	pdev->clk = clk_get(&plat_dev->dev, NULL);

	if (IS_ERR(pdev->clk))
		dev_err(&plat_dev->dev, "can't get clock\n");
	else if (clk_prepare_enable(pdev->clk))
		dev_err(&plat_dev->dev, "clock prepare enable failed");

	priv->phy_mode = PHY_MODE_RGMII;
	pdev->rx_buff_cnt = XGENE_NUM_PKT_BUF;

out:
	return rc;
}

static int xgene_enet_init_hw(struct xgene_enet_pdev *pdev)
{
	struct net_device *ndev;
	struct xgene_enet_priv *priv;
	struct mii_bus *mdio_bus;
	int rc = 0;

	ndev = pdev->ndev;
	priv = &pdev->priv;

	xgene_enet_port_reset(priv);

	/* To ensure no packet enters the system, disable Rx/Tx */
	xgene_enet_mac_tx_state(priv, 0);
	xgene_enet_mac_rx_state(priv, 0);

	ndev->netdev_ops = &apm_dnetdev_ops;

	ndev->features |= NETIF_F_IP_CSUM;
	ndev->features |= NETIF_F_TSO | NETIF_F_SG;
	pdev->mss = DEFAULT_TCP_MSS;
	xgene_enet_tx_offload(priv, XGENE_ENET_MSS0, pdev->mss);
	ndev->features |= NETIF_F_GRO;

	/* Ethtool checks the capabilities/features in hw_features flag */
	ndev->hw_features = ndev->features;
	SET_ETHTOOL_OPS(ndev, &xgene_ethtool_ops);

	rc = register_netdev(ndev);
	if (rc) {
		netdev_err(ndev, "Failed to register net dev(%d)!\n", rc);
		goto out;
	}

	rc = xgene_enet_qconfig(pdev);
	if (rc) {
		netdev_err(ndev, "Error in QM configuration\n");
		goto out;
	}

	xgene_enet_napi_add(pdev);

	xgene_enet_cle_bypass(priv, QMTM_QUEUE_ID(pdev->sdev->qmtm_ip,
						  pdev->qm_queues.
						  default_rx_qid),
			      pdev->qm_queues.default_rx_fp_pbn - 0x20);

	/* Default MAC initialization */
	xgene_enet_mac_init(priv, ndev->dev_addr, SPEED_1000,
			    HW_MTU(ndev->mtu), priv->crc);

	/* Setup MDIO bus */
	mdio_bus = mdiobus_alloc();
	if (!mdio_bus) {
		netdev_err(ndev, "Not able to allocate memory for MDIO bus\n");
		rc = -ENOMEM;
		goto out;
	}

	pdev->mdio_bus = mdio_bus;
	mdio_bus->name = "APM Ethernet MII Bus";
	mdio_bus->read = xgene_enet_mdio_read;
	mdio_bus->write = xgene_enet_mdio_write;
	snprintf(mdio_bus->id, MII_BUS_ID_SIZE, "%x", priv->phy_addr);
	mdio_bus->priv = pdev;
	mdio_bus->parent = &ndev->dev;
	mdio_bus->phy_mask = ~(1 << priv->phy_addr);
	rc = mdiobus_register(mdio_bus);
	if (rc) {
		netdev_err(ndev, "Failed to register MDIO bus(%d)!\n", rc);
		return rc;
	}

	rc = xgene_enet_mdio_probe(ndev);
	xgene_enet_register_irq(ndev);

out:
	return rc;
}

static int xgene_enet_probe(struct platform_device *plat_dev)
{
	struct net_device *ndev;
	struct xgene_enet_pdev *pdev;
	struct device *dev;
	struct xgene_enet_priv *priv;
	u32 num_tx_queues, num_rx_queues;
	int rc;

	dev = &plat_dev->dev;
	num_tx_queues = MAX_TX_QUEUES;
	num_rx_queues = MAX_RX_QUEUES;

	ndev = alloc_etherdev_mqs(sizeof(struct xgene_enet_pdev),
				  num_tx_queues, num_rx_queues);

	if (!ndev) {
		dev_err(dev, "Not able to allocate memory for netdev\n");
		rc = -ENOMEM;
		goto out;
	}

	pdev = (struct xgene_enet_pdev *)netdev_priv(ndev);
	priv = &pdev->priv;
	pdev->ndev = ndev;
	pdev->num_tx_queues = num_tx_queues;
	pdev->num_rx_queues = num_rx_queues;
	pdev->plat_dev = plat_dev;
	pdev->node = plat_dev->dev.of_node;
	SET_NETDEV_DEV(ndev, &plat_dev->dev);
	dev_set_drvdata(&plat_dev->dev, pdev);

	xgene_enet_get_resources(pdev);

	xgene_enet_init_priv(priv);
	rc = xgene_enet_init_hw(pdev);

out:
	return rc;
}

static int xgene_enet_remove(struct platform_device *plat_dev)
{
	struct xgene_enet_pdev *pdev;
	struct xgene_enet_priv *priv;
	struct net_device *ndev;
	u32 qindex;
	u8 qmtm_ip;

	pdev = platform_get_drvdata(plat_dev);
	qmtm_ip = pdev->sdev->qmtm_ip;
	ndev = pdev->ndev;
	priv = &pdev->priv;

	/* Stop any traffic and disable MAC */
	xgene_enet_mac_rx_state(priv, 0);
	xgene_enet_mac_tx_state(priv, 0);

	if (netif_running(ndev)) {
		netif_device_detach(ndev);
		netif_stop_queue(ndev);
		xgene_enet_napi_disable(pdev);
	}

	xgene_enet_napi_del(pdev);
	xgene_enet_mdio_remove(ndev);

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++) {
		if (pdev->qm_queues.rx_fp[qindex].qid > 0)
			xgene_enet_deinit_fp(pdev->rx_skb_pool[qindex],
					     pdev->qm_queues.rx_fp[qindex].qid);
	}

	xgene_enet_delete_queue(pdev);

	for (qindex = 0; qindex < pdev->num_rx_queues; qindex++) {
		kfree(pdev->rx_skb_pool[qindex]->skb);
		kfree(pdev->rx_skb_pool[qindex]);
		kfree(pdev->rx[qindex]);
	}
	for (qindex = 0; qindex < pdev->num_tx_queues; qindex++) {
		kfree(pdev->tx[qindex]->msg8);
		kfree(pdev->tx[qindex]);
	}

	unregister_netdev(ndev);
	xgene_enet_port_shutdown(priv);

	free_netdev(ndev);

	return 0;
}

static struct of_device_id xgene_enet_match[] = {
	{
	 .compatible = "apm,xgene-enet",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, xgene_enet_match);

static struct platform_driver xgene_enet_driver = {
	.driver = {
		   .name = XGENE_ENET_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = xgene_enet_match,
		   },
	.probe = xgene_enet_probe,
	.remove = xgene_enet_remove,
};

static int __init xgene_enet_init(void)
{
	if (!platform_driver_register(&xgene_enet_driver))
		pr_info("%s v%s loaded\n", XGENE_ENET_DRIVER_DESC,
				XGENE_ENET_DRIVER_VERSION);

	return 0;
}

static void __exit xgene_enet_exit(void)
{
	platform_driver_unregister(&xgene_enet_driver);
	pr_info("%s v%s unloaded\n", XGENE_ENET_DRIVER_DESC,
			XGENE_ENET_DRIVER_VERSION);
}

module_init(xgene_enet_init);
module_exit(xgene_enet_exit);

MODULE_DESCRIPTION(XGENE_ENET_DRIVER_DESC);
MODULE_VERSION(XGENE_ENET_DRIVER_VERSION);
MODULE_AUTHOR("Keyur Chudgar <kchudgar@apm.com>");
MODULE_LICENSE("GPL");
