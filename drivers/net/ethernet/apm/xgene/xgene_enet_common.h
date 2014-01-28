/* AppliedMicro X-Gene SoC Ethernet Driver
 *
 * Copyright (c) 2013, Applied Micro Circuits Corporation
 * Authors:	Ravi Patel rapatel@apm.com>
 *		Iyappan Subramanian isubramanian@apm.com>
 *		Fushen Chen fchen@apm.com>
 *		Keyur Chudgar kchudgar@apm.com>
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

#ifndef __XGENE_ENET_COMMON_H__
#define __XGENE_ENET_COMMON_H__

#include <misc/xgene/xgene_qmtm.h>
#define MAX_LOOP_POLL_CNT	10

#ifndef UDP_HDR_SIZE
#define UDP_HDR_SIZE		2
#endif

/* Ethernet & XGENET port ids */
enum eth_port_ids {
	ENET_0 = 0,
	ENET_1,
	ENET_2,
	ENET_3,
	XGENET_0,
	XGENET_1,
	XGENET_2,
	XGENET_3,
	MENET,
	MAX_ENET_PORTS
};

/* TSO Parameters */
#define TSO_ENABLE		1
#define TSO_ENABLE_MASK		1
#define TSO_CHKSUM_ENABLE	1
#define TSO_INS_CRC_ENABLE	1
#define TSO_IPPROTO_TCP		1
#define TSO_IPPROTO_UDP		0
#define TSO_IP_HLEN_MASK	0X3F
#define TSO_TCP_HLEN_MASK	0X3F
#define TSO_ETH_HLEN_MASK	0XFF
#define TSO_MSS_MASK		0X3	/* 2b */
#define DEFAULT_TCP_MSS		1448

enum {
	XGENE_ENET_MSS0 = 0,
	XGENE_ENET_MSS1,
	XGENE_ENET_MSS2,
	XGENE_ENET_MSS3,
	XGENE_ENET_TSO_CFG,
	XGENE_ENET_INSERT_VLAN
};

/* TYPE_SEL for Ethernt egress message */
#define TYPE_SEL_WORK_MSG	1U

/* Blocks for defined regions */
enum {
	BLOCK_ETH_CSR = 1,
	BLOCK_ETH_CLE,
	BLOCK_ETH_QMI,
	BLOCK_ETH_SDS_CSR,
	BLOCK_ETH_CLKRST_CSR,
	BLOCK_ETH_DIAG_CSR,
	BLOCK_ETH_MDIO_CSR,
	BLOCK_ETH_INTPHY,
	BLOCK_ETH_EXTPHY,
	BLOCK_MCX_MAC,
	BLOCK_MCX_STATS,
	BLOCK_MCX_MAC_CSR,
	BLOCK_SATA_ENET_CSR,
	BLOCK_AXG_MAC,
	BLOCK_AXG_STATS,
	BLOCK_AXG_MAC_CSR,
	BLOCK_XGENET_PCS,
	BLOCK_XGENET_MDIO_CSR,
	BLOCK_ETH_MAX
};

/* Direct Address mode */
#define BLOCK_ETH_CSR_OFFSET		0x2000
#define BLOCK_ETH_CLE_OFFSET		0x6000
#define BLOCK_ETH_QMI_OFFSET		0x9000
#define BLOCK_ETH_SDS_CSR_OFFSET	0xA000
#define BLOCK_ETH_CLKRST_CSR_OFFSET	0xC000
#define BLOCK_ETH_DIAG_CSR_OFFSET	0xD000

/* Indirect & Direct  Address mode for MCX_MAC and AXG_MAC */
#define BLOCK_ETH_MAC_OFFSET		0x0000
#define BLOCK_ETH_STATS_OFFSET		0x0014
#define BLOCK_ETH_MAC_CSR_OFFSET	0x2800

#define BLOCK_SATA_ENET_CSR_OFFSET	0x7000

/* Constants for indirect registers */
#define MAC_ADDR_REG_OFFSET		0
#define MAC_COMMAND_REG_OFFSET		4
#define MAC_WRITE_REG_OFFSET		8
#define MAC_READ_REG_OFFSET		12
#define MAC_COMMAND_DONE_REG_OFFSET	16

#define STAT_ADDR_REG_OFFSET		0
#define STAT_COMMAND_REG_OFFSET		4
#define STAT_WRITE_REG_OFFSET		8
#define STAT_READ_REG_OFFSET		12
#define STAT_COMMAND_DONE_REG_OFFSET	16

/* Address PE_MCXMAC  Registers */
#define MII_MGMT_COMMAND_ADDR                                        0x00000024
#define MII_MGMT_ADDRESS_ADDR                                        0x00000028
#define MII_MGMT_CONTROL_ADDR                                        0x0000002c
#define MII_MGMT_STATUS_ADDR                                         0x00000030
#define MII_MGMT_INDICATORS_ADDR                                     0x00000034

#define INT_PHY_ADDR	0x1E

#define BUSY_MASK                                                    0x00000001
#define READ_CYCLE_MASK                                              0x00000001
#define PHY_CONTROL_WR(src)                         (((u32)(src)) & 0x0000ffff)

#define HW_MTU(m) ((m) + 12 + 2 + 4 /* MAC + CRC */)

enum xgene_enum_speed {
	XGENE_ENET_SPEED_0 = 0xffff,
	XGENE_ENET_SPEED_10 = 10,
	XGENE_ENET_SPEED_100 = 100,
	XGENE_ENET_SPEED_1000 = 1000,
	XGENE_ENET_SPEED_10000 = 10000
};

enum xgene_enet_mode {
	HALF_DUPLEX = 1,
	FULL_DUPLEX = 2
};

enum xgene_enet_phy_mode {
	PHY_MODE_NONE,
	PHY_MODE_RGMII,
	PHY_MODE_SGMII,
	PHY_MODE_XGMII
};

enum xgene_enet_cmd {
	XGENE_ENET_WR_CMD = 0x80000000,
	XGENE_ENET_RD_CMD = 0x40000000
};

#define CMU 0

/* ===== MII definitions ===== */

#define MII_CRC_LEN		0x4	/* CRC length in bytes */
#define MII_ETH_MAX_PCK_SZ      (ETHERMTU + SIZEOF_ETHERHEADER          \
				 + MII_CRC_LEN)
#define MII_MAX_PHY_NUM		0x20	/* max number of attached PHYs */
#define MII_MAX_REG_NUM         0x20	/* max number of registers */

#define MII_CTRL_REG		0x0	/* Control Register */
#define MII_STAT_REG		0x1	/* Status Register */
#define MII_PHY_ID1_REG		0x2	/* PHY identifier 1 Register */
#define MII_PHY_ID2_REG		0x3	/* PHY identifier 2 Register */
#define MII_AN_ADS_REG		0x4	/* Auto-Negotiation       */
					/* Advertisement Register */
#define MII_AN_PRTN_REG		0x5	/* Auto-Negotiation         */
					/* partner ability Register */
#define MII_AN_EXP_REG		0x6	/* Auto-Negotiation   */
					/* Expansion Register */
#define MII_AN_NEXT_REG		0x7	/* Auto-Negotiation            */
					/* next-page transmit Register */

#define MII_AN_PRTN_NEXT_REG	0x8	/* Link partner received next page */
#define MII_MASSLA_CTRL_REG	0x9	/* MATER-SLAVE control register */
#define MII_MASSLA_STAT_REG	0xa	/* MATER-SLAVE status register */
#define MII_EXT_STAT_REG	0xf	/* Extented status register */

/* MII control register bit  */
#define MII_CR_1000		0x0040	/* 1 = 1000mb when
					* MII_CR_100 is also 1
					*/
#define MII_CR_COLL_TEST	0x0080	/* collision test */
#define MII_CR_FDX		0x0100	/* FDX =1, half duplex =0 */
#define MII_CR_RESTART		0x0200	/* restart auto negotiation */
#define MII_CR_ISOLATE		0x0400	/* isolate PHY from MII */
#define MII_CR_POWER_DOWN	0x0800	/* power down */
#define MII_CR_AUTO_EN		0x1000	/* auto-negotiation enable */
#define MII_CR_100		0x2000	/* 0 = 10mb, 1 = 100mb */
#define MII_CR_LOOPBACK		0x4000	/* 0 = normal, 1 = loopback */
#define MII_CR_RESET		0x8000	/* 0 = normal, 1 = PHY reset */
#define MII_CR_NORM_EN		0x0000	/* just enable the PHY */
#define MII_CR_DEF_0_MASK       0xca7f	/* they must return zero */
#define MII_CR_RES_MASK		0x003f	/* reserved bits,return zero */

/* MII Status register bit definitions */
#define MII_SR_LINK_STATUS	0x0004	/* link Status -- 1 = link */
#define MII_SR_AUTO_SEL		0x0008	/* auto speed select capable */
#define MII_SR_REMOTE_FAULT     0x0010	/* Remote fault detect */
#define MII_SR_AUTO_NEG         0x0020	/* auto negotiation complete */
#define MII_SR_EXT_STS		0x0100	/* extended sts in reg 15 */
#define MII_SR_T2_HALF_DPX	0x0200	/* 100baseT2 HD capable */
#define MII_SR_T2_FULL_DPX	0x0400	/* 100baseT2 FD capable */
#define MII_SR_10T_HALF_DPX     0x0800	/* 10BaseT HD capable */
#define MII_SR_10T_FULL_DPX     0x1000	/* 10BaseT FD capable */
#define MII_SR_TX_HALF_DPX      0x2000	/* TX HD capable */
#define MII_SR_TX_FULL_DPX      0x4000	/* TX FD capable */
#define MII_SR_T4               0x8000	/* T4 capable */
#define MII_SR_ABIL_MASK        0xff80	/* abilities mask */
#define MII_SR_EXT_CAP          0x0001	/* extended capabilities */
#define MII_SR_SPEED_SEL_MASK	0xf800	/* Mask to extract just speed
					 * capabilities  from status
					 * register.
					 */

/* MII AN advertisement Register bit definition */
#define MII_ANAR_10TX_HD        0x0020
#define MII_ANAR_10TX_FD        0x0040
#define MII_ANAR_100TX_HD       0x0080
#define MII_ANAR_100TX_FD       0x0100
#define MII_ANAR_100T_4         0x0200
#define MII_ANAR_PAUSE          0x0400
#define MII_ANAR_ASM_PAUSE      0x0800
#define MII_ANAR_REMORT_FAULT   0x2000
#define MII_ANAR_NEXT_PAGE      0x8000
#define MII_ANAR_PAUSE_MASK     0x0c00

/* MII Link Code word  bit definitions */
#define MII_BP_FAULT	0x2000	/* remote fault */
#define MII_BP_ACK	0x4000	/* acknowledge */
#define MII_BP_NP	0x8000	/* nexp page is supported */

/* MII Next Page bit definitions */
#define MII_NP_TOGGLE	0x0800	/* toggle bit */
#define MII_NP_ACK2	0x1000	/* acknowledge two */
#define MII_NP_MSG	0x2000	/* message page */
#define MII_NP_ACK1	0x4000	/* acknowledge one */
#define MII_NP_NP	0x8000	/* nexp page will follow */

/* MII Master-Slave Control register bit definition */
#define MII_MASSLA_CTRL_1000T_HD    0x100
#define MII_MASSLA_CTRL_1000T_FD    0x200
#define MII_MASSLA_CTRL_PORT_TYPE   0x400
#define MII_MASSLA_CTRL_CONFIG_VAL  0x800
#define MII_MASSLA_CTRL_CONFIG_EN   0x1000

/* MII Master-Slave Status register bit definition */
#define MII_MASSLA_STAT_LP1000T_HD  0x400
#define MII_MASSLA_STAT_LP1000T_FD  0x800
#define MII_MASSLA_STAT_REMOTE_RCV  0x1000
#define MII_MASSLA_STAT_LOCAL_RCV   0x2000
#define MII_MASSLA_STAT_CONF_RES    0x4000
#define MII_MASSLA_STAT_CONF_FAULT  0x8000

/* these values may be used in the default phy mode field of the load
 * string, since that is used to force the operating mode of the PHY
 * in case any attempt to establish the link failed.
 */

#define PHY_10BASE_T            0x00	/* 10 Base-T */
#define PHY_10BASE_T_FDX        0x01	/* 10 Base Tx, full duplex */
#define PHY_100BASE_TX          0x02	/* 100 Base Tx */
#define PHY_100BASE_TX_FDX      0x03	/* 100 Base TX, full duplex */
#define PHY_100BASE_T4          0x04	/* 100 Base T4 */
#define PHY_AN_ENABLE		0x05	/* re-enable auto-negotiation */

#define MII_AN_TBL_MAX		20	/* max number of entries in the table */

/* Realtek PHY definitions */
#define PHY_SPEED_RES			3
#define PHY_SPEED_1000			2
#define PHY_SPEED_100			1
#define PHY_SPEED_10			0
#define RTL_PHYSR_ADR			0X11
#define RTL_PHYSR_SPEED_RD(src)		(((src) & 0x0000C000) >> 14)
#define RTL_PHYSR_LINK_RD(src)		(((src) & 0x00000400) >> 10)

#define RTL_PHYSR_ADR           0X11

/* LErr(3b) Decoding */
enum xgene_enet_lerr {
	ENET_NO_ERR = 0,	/* No Error */
	ENET_AXI_WR_ERR = 1,	/* AXI write data error due to RSIF */
	ENET_ING_CRC_ERR = 2,	/* Rx packet had CRC */
	ENET_AXI_RD_ERR = 3,	/* AXI read data error when processing
				 * a work message in TSIF
				 */
	ENET_LL_RD_ERR = 4,	/* AXI Link List read error when
				 * processing a work message in TSIF
				 */
	ENET_ING_ERR = 5,	/* Rx packet had ingress processing error */
	ENET_CHKSUM_ERR = 5,	/* Checksum error */
	ENET_BAD_MSG_ERR = 6,	/* Bad message to subsytem */
	ENET_MISC_ERR = 7,	/* Other ingress processing error */
	ENET_MAC_TRUNC_ERR = 7,	/* MAX truncated */
	ENET_MAC_LEN_ERR = 8,	/* Packet length error */
	ENET_PKT_LESS64_ERR = 9,	/* MAC length lesser than 64B */
	ENET_MAC_OVERRUN_ERR = 10,	/* FIFO overrun on ingress */
	ENET_UNISEC_CHKSUM_ERR = 11,	/* Rx pacekt checksum error */
	ENET_UNISEC_LEN_ERR = 12,	/* Rx pkt length mismatch QM message */
	ENET_UNISEC_ICV_ERR = 13,	/* Rx pkt ICV error */
	ENET_UNISEC_PROTO_ERR = 14,	/* Rx pkt protocol field mismatch */
	ENET_FP_TIMEOUT_ERR = 15	/* Free pool buffer timeout */
};

/* Error TX/RX Statistics - maintained by software */
struct xgene_mac_error_stats {
	u64 rx_hw_errors;
	u64 rx_hw_overrun;
	u64 tx_dropped;
};

struct xgene_enet_rx_stats {
	u32 rx_byte_count;	/* Receive Byte Counter */
	u32 rx_packet_count;	/* Receive Packet Counter */
	u32 rx_fcs_err_count;	/* Receive FCS Error Counter */
	u32 rx_alignment_err_pkt_count;	/* Rx Alignment Err Packet Counter */
	u32 rx_frm_len_err_pkt_count;	/* Rx Frame Len Err Packet Counter */
	u32 rx_undersize_pkt_count;	/* Receive Undersize Packet Counter */
	u32 rx_oversize_pkt_count;	/* Receive Oversize Packet Counter */
	u32 rx_drop_pkt_count;	/* Receive Drop Packet Counter */
};

struct xgene_enet_tx_stats {
	u32 tx_byte_count;	/* Tx Byte cnt */
	u32 tx_pkt_count;	/* Tx pkt cnt */
	u32 tx_drop_frm_count;	/* Tx Drop Frame cnt */
	u32 tx_fcs_err_frm_count;	/* Tx FCS Error Frame cnt */
	u32 tx_undersize_frm_count;	/* Tx Undersize Frame cnt */
};

struct xgene_enet_detailed_stats {
	struct xgene_enet_rx_stats rx_stats;
	struct xgene_enet_tx_stats tx_stats;
	struct xgene_mac_error_stats estats;
};

/* Ethernet private structure */
struct xgene_enet_priv {
	void *eth_csr_addr_v;
	void *eth_cle_addr_v;
	void *eth_qmi_addr_v;
	void *eth_sds_csr_addr_v;
	void *eth_clkrst_csr_addr_v;
	void *eth_diag_csr_addr_v;
	void *mcx_mac_addr_v;
	void *mcx_stats_addr_v;
	void *mcx_mac_csr_addr_v;
	void *sata_enet_csr_addr_v;
	void *axg_mac_addr_v;
	void *axg_stats_addr_v;
	void *axg_mac_csr_addr_v;
	void *xgenet_pcs_addr_v;
	void *xgenet_mdio_csr_addr_v;

	u64 paddr_base;		/* Base physical address of device */
	void *vaddr_base;	/* Base Virtual address for the device */
	u64 ppaddr_base;	/* Per port physical address of device */
	void *vpaddr_base;	/* Per port Virtual address of device */
	void *vmii_base;	/* Base MII Virtual address of device */

	u32 phy_addr;		/* Virtual address for PHY */
	u32 phy_mode;
	u32 speed;		/* Forced Link Speed */
	u32 link_status;
	u32 crc;
	u32 autoneg_set;
	u32 mac_to_mac;		/* Tell traffic is MAC-to-MAC */
	u32 desired_speed;	/* In case of MAC-to-MAC, no autoneg,
				 * tell the desired speed to setup
				 */
	u32 phyless;		/* PHY stays away from board on
				 * common server board design
				 */
	u32 force_serdes_reset;	/* Force analog reset till stable state */

	/* Function pointers */
	void (*port_reset) (struct xgene_enet_priv *priv);
	int (*phy_autoneg_done) (struct xgene_enet_priv *priv);
	void (*phy_link_mode) (struct xgene_enet_priv *priv,
			       u32 *speed, u32 *state);
	void (*mac_reset) (struct xgene_enet_priv *priv);
	int (*mac_init) (struct xgene_enet_priv *priv,
			 unsigned char *dev_addr, int speed, int mtu, int crc);
	void (*mac_rx_state) (struct xgene_enet_priv *priv, u32 enable);
	void (*mac_tx_state) (struct xgene_enet_priv *priv, u32 enable);
	void (*mac_change_mtu) (struct xgene_enet_priv *priv, u32 new_mtu);
	void (*mac_set_ipg) (struct xgene_enet_priv *priv, u16 new_ipg);
	void (*get_stats) (struct xgene_enet_priv *priv,
			   struct xgene_enet_detailed_stats *stats);
	void (*set_mac_addr) (struct xgene_enet_priv *priv,
			      unsigned char *dev_addr);
	void (*cle_bypass) (struct xgene_enet_priv *priv, u32 dstqid,
			    u32 fpsel);
	void (*tx_offload) (struct xgene_enet_priv *priv, u32 command,
			    u32 value);
	void (*qmi_assoc) (struct xgene_enet_priv *priv);
	void (*port_shutdown) (struct xgene_enet_priv *priv);
};

int xgene_enet_wr(struct xgene_enet_priv *priv, u8 block_id,
		  u32 reg_offset, u32 value);

int xgene_enet_rd(struct xgene_enet_priv *priv, u8 block_id,
		  u32 reg_offset, u32 *value);

void xgene_enet_port_reset(struct xgene_enet_priv *priv);

/* This function resets the entire part of MAC and minimal init for phy access
 * It will put both Transmit and Receive MAC Control block in reset
 * and then init.
 */
void xgene_enet_mac_reset(struct xgene_enet_priv *priv);

int xgene_enet_mac_init(struct xgene_enet_priv *priv, unsigned char *dev_addr,
			int speed, int mtu, int crc);

void xgene_enet_mac_rx_state(struct xgene_enet_priv *priv, u32 enable);
void xgene_enet_mac_tx_state(struct xgene_enet_priv *priv, u32 enable);

void xgene_enet_mac_change_mtu(struct xgene_enet_priv *priv, u32 new_mtu);
void xgene_enet_mac_set_ipg(struct xgene_enet_priv *priv, u16 ipg);

void xgene_enet_set_mac_addr(struct xgene_enet_priv *priv,
			     unsigned char *dev_addr);

void xgene_enet_cle_bypass(struct xgene_enet_priv *priv, u32 dstqid, u32 fpsel);

void xgene_enet_tx_offload(struct xgene_enet_priv *priv,
			   u32 command, u32 value);

void xgene_enet_port_shutdown(struct xgene_enet_priv *priv);
enum xgene_qmtm_qaccess xgene_enet_get_qacess(void);
void xgene_genericmiiphy_read(struct xgene_enet_priv *priv, u8 phy_id,
			      unsigned char reg, u32 *data);
void xgene_genericmiiphy_write(struct xgene_enet_priv *priv, u8 phy_id,
			       unsigned char reg, u32 data);

void xgene_enet_get_stats(struct xgene_enet_priv *priv,
			  struct xgene_enet_detailed_stats *detailed_stats);
#endif /* __XGENE_ENET_COMMON_H__ */
