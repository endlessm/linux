// SPDX-License-Identifier: GPL-2.0
/* Copyright 2019 NXP Semiconductors
 */
#include <soc/mscc/ocelot.h>
#include <linux/packing.h>
#include "dsa_priv.h"

/* The CPU injection header and the CPU extraction header can have 3 types of
 * prefixes: long, short and no prefix. The format of the header itself is the
 * same in all 3 cases.
 *
 * Extraction with long prefix:
 *
 * +-------------------+-------------------+------+------+------------+-------+
 * | ff:ff:ff:ff:ff:ff | ff:ff:ff:ff:ff:ff | 8880 | 000a | extraction | frame |
 * |                   |                   |      |      |   header   |       |
 * +-------------------+-------------------+------+------+------------+-------+
 *        48 bits             48 bits      16 bits 16 bits  128 bits
 *
 * Extraction with short prefix:
 *
 *                                         +------+------+------------+-------+
 *                                         | 8880 | 000a | extraction | frame |
 *                                         |      |      |   header   |       |
 *                                         +------+------+------------+-------+
 *                                         16 bits 16 bits  128 bits
 *
 * Extraction with no prefix:
 *
 *                                                       +------------+-------+
 *                                                       | extraction | frame |
 *                                                       |   header   |       |
 *                                                       +------------+-------+
 *                                                          128 bits
 *
 *
 * Injection with long prefix:
 *
 * +-------------------+-------------------+------+------+------------+-------+
 * |      any dmac     |      any smac     | 8880 | 000a | injection  | frame |
 * |                   |                   |      |      |   header   |       |
 * +-------------------+-------------------+------+------+------------+-------+
 *        48 bits             48 bits      16 bits 16 bits  128 bits
 *
 * Injection with short prefix:
 *
 *                                         +------+------+------------+-------+
 *                                         | 8880 | 000a | injection  | frame |
 *                                         |      |      |   header   |       |
 *                                         +------+------+------------+-------+
 *                                         16 bits 16 bits  128 bits
 *
 * Injection with no prefix:
 *
 *                                                       +------------+-------+
 *                                                       | injection  | frame |
 *                                                       |   header   |       |
 *                                                       +------------+-------+
 *                                                          128 bits
 *
 * The injection header looks like this (network byte order, bit 127
 * is part of lowest address byte in memory, bit 0 is part of highest
 * address byte):
 *
 *         +------+------+------+------+------+------+------+------+
 * 127:120 |BYPASS| MASQ |          MASQ_PORT        |REW_OP|REW_OP|
 *         +------+------+------+------+------+------+------+------+
 * 119:112 |                         REW_OP                        |
 *         +------+------+------+------+------+------+------+------+
 * 111:104 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 * 103: 96 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 *  95: 88 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 *  87: 80 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 *  79: 72 |                          RSV                          |
 *         +------+------+------+------+------+------+------+------+
 *  71: 64 |            RSV            |           DEST            |
 *         +------+------+------+------+------+------+------+------+
 *  63: 56 |                         DEST                          |
 *         +------+------+------+------+------+------+------+------+
 *  55: 48 |                          RSV                          |
 *         +------+------+------+------+------+------+------+------+
 *  47: 40 |  RSV |         SRC_PORT          |     RSV     |TFRM_TIMER|
 *         +------+------+------+------+------+------+------+------+
 *  39: 32 |     TFRM_TIMER     |               RSV                |
 *         +------+------+------+------+------+------+------+------+
 *  31: 24 |  RSV |  DP  |   POP_CNT   |           CPUQ            |
 *         +------+------+------+------+------+------+------+------+
 *  23: 16 |           CPUQ            |      QOS_CLASS     |TAG_TYPE|
 *         +------+------+------+------+------+------+------+------+
 *  15:  8 |         PCP        |  DEI |            VID            |
 *         +------+------+------+------+------+------+------+------+
 *   7:  0 |                          VID                          |
 *         +------+------+------+------+------+------+------+------+
 *
 * And the extraction header looks like this:
 *
 *         +------+------+------+------+------+------+------+------+
 * 127:120 |  RSV |                  REW_OP                        |
 *         +------+------+------+------+------+------+------+------+
 * 119:112 |       REW_OP       |              REW_VAL             |
 *         +------+------+------+------+------+------+------+------+
 * 111:104 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 * 103: 96 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 *  95: 88 |                         REW_VAL                       |
 *         +------+------+------+------+------+------+------+------+
 *  87: 80 |       REW_VAL      |               LLEN               |
 *         +------+------+------+------+------+------+------+------+
 *  79: 72 | LLEN |                      WLEN                      |
 *         +------+------+------+------+------+------+------+------+
 *  71: 64 | WLEN |                      RSV                       |
 *         +------+------+------+------+------+------+------+------+
 *  63: 56 |                          RSV                          |
 *         +------+------+------+------+------+------+------+------+
 *  55: 48 |                          RSV                          |
 *         +------+------+------+------+------+------+------+------+
 *  47: 40 | RSV  |          SRC_PORT         |       ACL_ID       |
 *         +------+------+------+------+------+------+------+------+
 *  39: 32 |       ACL_ID       |  RSV |         SFLOW_ID          |
 *         +------+------+------+------+------+------+------+------+
 *  31: 24 |ACL_HIT| DP  |  LRN_FLAGS  |           CPUQ            |
 *         +------+------+------+------+------+------+------+------+
 *  23: 16 |           CPUQ            |      QOS_CLASS     |TAG_TYPE|
 *         +------+------+------+------+------+------+------+------+
 *  15:  8 |         PCP        |  DEI |            VID            |
 *         +------+------+------+------+------+------+------+------+
 *   7:  0 |                          VID                          |
 *         +------+------+------+------+------+------+------+------+
 */

static struct sk_buff *ocelot_xmit(struct sk_buff *skb,
				   struct net_device *netdev)
{
	struct dsa_port *dp = dsa_slave_to_port(netdev);
	struct sk_buff *clone = DSA_SKB_CB(skb)->clone;
	struct dsa_switch *ds = dp->ds;
	struct ocelot *ocelot = ds->priv;
	struct ocelot_port *ocelot_port;
	u8 *prefix, *injection;
	u64 qos_class, rew_op;

	ocelot_port = ocelot->ports[dp->index];

	injection = skb_push(skb, OCELOT_TAG_LEN);

	prefix = skb_push(skb, OCELOT_SHORT_PREFIX_LEN);

	memcpy(prefix, ocelot_port->xmit_template, OCELOT_TOTAL_TAG_LEN);

	/* Fix up the fields which are not statically determined
	 * in the template
	 */
	qos_class = skb->priority;
	packing(injection, &qos_class, 19,  17, OCELOT_TAG_LEN, PACK, 0);

	/* TX timestamping was requested */
	if (clone) {
		rew_op = ocelot_port->ptp_cmd;
		/* Retrieve timestamp ID populated inside skb->cb[0] of the
		 * clone by ocelot_port_add_txtstamp_skb
		 */
		if (ocelot_port->ptp_cmd == IFH_REW_OP_TWO_STEP_PTP)
			rew_op |= clone->cb[0] << 3;

		packing(injection, &rew_op, 125, 117, OCELOT_TAG_LEN, PACK, 0);
	}

	return skb;
}

static struct sk_buff *ocelot_rcv(struct sk_buff *skb,
				  struct net_device *netdev,
				  struct packet_type *pt)
{
	struct dsa_port *cpu_dp = netdev->dsa_ptr;
	struct dsa_switch *ds = cpu_dp->ds;
	struct ocelot *ocelot = ds->priv;
	u64 src_port, qos_class;
	u64 vlan_tci, tag_type;
	u8 *start = skb->data;
	u8 *extraction;
	u16 vlan_tpid;

	/* Revert skb->data by the amount consumed by the DSA master,
	 * so it points to the beginning of the frame.
	 */
	skb_push(skb, ETH_HLEN);
	/* We don't care about the short prefix, it is just for easy entrance
	 * into the DSA master's RX filter. Discard it now by moving it into
	 * the headroom.
	 */
	skb_pull(skb, OCELOT_SHORT_PREFIX_LEN);
	/* And skb->data now points to the extraction frame header.
	 * Keep a pointer to it.
	 */
	extraction = skb->data;
	/* Now the EFH is part of the headroom as well */
	skb_pull(skb, OCELOT_TAG_LEN);
	/* Reset the pointer to the real MAC header */
	skb_reset_mac_header(skb);
	skb_reset_mac_len(skb);
	/* And move skb->data to the correct location again */
	skb_pull(skb, ETH_HLEN);

	/* Remove from inet csum the extraction header */
	skb_postpull_rcsum(skb, start, OCELOT_TOTAL_TAG_LEN);

	packing(extraction, &src_port,  46, 43, OCELOT_TAG_LEN, UNPACK, 0);
	packing(extraction, &qos_class, 19, 17, OCELOT_TAG_LEN, UNPACK, 0);
	packing(extraction, &tag_type,  16, 16, OCELOT_TAG_LEN, UNPACK, 0);
	packing(extraction, &vlan_tci,  15,  0, OCELOT_TAG_LEN, UNPACK, 0);

	skb->dev = dsa_master_find_slave(netdev, 0, src_port);
	if (!skb->dev)
		/* The switch will reflect back some frames sent through
		 * sockets opened on the bare DSA master. These will come back
		 * with src_port equal to the index of the CPU port, for which
		 * there is no slave registered. So don't print any error
		 * message here (ignore and drop those frames).
		 */
		return NULL;

	skb->offload_fwd_mark = 1;
	skb->priority = qos_class;

	/* Ocelot switches copy frames unmodified to the CPU. However, it is
	 * possible for the user to request a VLAN modification through
	 * VCAP_IS1_ACT_VID_REPLACE_ENA. In this case, what will happen is that
	 * the VLAN ID field from the Extraction Header gets updated, but the
	 * 802.1Q header does not (the classified VLAN only becomes visible on
	 * egress through the "port tag" of front-panel ports).
	 * So, for traffic extracted by the CPU, we want to pick up the
	 * classified VLAN and manually replace the existing 802.1Q header from
	 * the packet with it, so that the operating system is always up to
	 * date with the result of tc-vlan actions.
	 * NOTE: In VLAN-unaware mode, we don't want to do that, we want the
	 * frame to remain unmodified, because the classified VLAN is always
	 * equal to the pvid of the ingress port and should not be used for
	 * processing.
	 */
	vlan_tpid = tag_type ? ETH_P_8021AD : ETH_P_8021Q;

	if (ocelot->ports[src_port]->vlan_aware &&
	    eth_hdr(skb)->h_proto == htons(vlan_tpid)) {
		u16 dummy_vlan_tci;

		skb_push_rcsum(skb, ETH_HLEN);
		__skb_vlan_pop(skb, &dummy_vlan_tci);
		skb_pull_rcsum(skb, ETH_HLEN);
		__vlan_hwaccel_put_tag(skb, htons(vlan_tpid), vlan_tci);
	}

	return skb;
}

static const struct dsa_device_ops ocelot_netdev_ops = {
	.name			= "ocelot",
	.proto			= DSA_TAG_PROTO_OCELOT,
	.xmit			= ocelot_xmit,
	.rcv			= ocelot_rcv,
	.overhead		= OCELOT_TOTAL_TAG_LEN,
	.promisc_on_master	= true,
};

MODULE_LICENSE("GPL v2");
MODULE_ALIAS_DSA_TAG_DRIVER(DSA_TAG_PROTO_OCELOT);

module_dsa_tag_driver(ocelot_netdev_ops);
