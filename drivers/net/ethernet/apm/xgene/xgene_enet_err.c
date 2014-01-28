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

int xgene_enet_parse_error(u8 LErr, int qid)
{
	/* QM Error */
	switch (LErr) {
	case 1:
		pr_err("LErr[%d] QID %d: QM msg size error\n", LErr, qid);
		return 0;
	case 2:
		pr_err("LErr[%d] QID %d: QM msg hop count error\n", LErr, qid);
		return 0;
	case 3:
		pr_err("LErr[%d] QID %d: enqueue to virtual queue error\n",
		       LErr, qid);
		return 0;
	case 4:
		pr_err("LErr[%d] QID %d: enqueue to disable queue error\n",
		       LErr, qid);
		return 0;
	case 5:
		pr_err("LErr[%d] QID %d: queue overfill error\n", LErr, qid);
		return 1;
	case 6:
		pr_err("LErr[%d] QID %d: QM enqueue error\n", LErr, qid);
		return 0;
	case 7:
		pr_err("LErr[%d] QID %d: QM dequeue error\n", LErr, qid);
		return 0;
	}
	return 0;
}

static irqreturn_t xgene_enet_qmi_err_irq(int irq, void *dev_instance)
{
	struct xgene_enet_pdev *pdev;
	struct xgene_enet_priv *priv;
	int rc;
	u32 data;
	u32 int_mask = 0;

	pdev = netdev_priv(dev_instance);
	priv = &pdev->priv;
	pr_err("Received Ethernet QMI Error Interrupt\n");
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, ENET_STSSSQMIINT0_ADDR, &data);
	if (data) {
		pr_err("Received STSSSQMIINT0 Error intr\n");
		if (ENET_FPOVERFLOW0_RD(data)) {
			pr_err("FP PB overflow indication:0x%08X\n", data);
			int_mask |= ENET_FPOVERFLOW0_MASK;
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   ENET_STSSSQMIINT0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, ENET_STSSSQMIINT1_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received STSSSQMIINT1 Error Interrupt\n");
		if (ENET_WQOVERFLOW1_RD(data)) {
			pr_err("WQ PB overflow indication:0x%08X\n", data);
			int_mask |= ENET_WQOVERFLOW1_MASK;
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   ENET_STSSSQMIINT1_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, ENET_STSSSQMIINT2_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received STSSSQMIINT2  Error Interrupt\n");
		if (ENET_FPUNDERRUN2_RD(data)) {
			pr_err("FP PB underrun indication:0x%08X\n", data);
			int_mask |= ENET_FPUNDERRUN2_MASK;
		}

		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   ENET_STSSSQMIINT2_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, ENET_STSSSQMIINT3_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received STSSSQMIINT3 Error Interrupt\n");
		if (ENET_WQUNDERRUN3_RD(data)) {
			pr_err("WQ PB underrun indication:0x%08X\n", data);
			int_mask |= ENET_WQUNDERRUN3_MASK;
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   ENET_STSSSQMIINT3_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, ENET_STSSSQMIINT4_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received STSSSQMIINT4 Error Interrupt\n");
		if (ENET_AXIWCMR_SLVERR4_RD(data)) {
			pr_err("AXI slave error on write  master channel\n");
			int_mask |= ENET_AXIWCMR_SLVERR4_MASK;
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   ENET_STSSSQMIINT4_ADDR, int_mask);
	}

	return IRQ_HANDLED;
}

static irqreturn_t xgene_enet_mac_err_irq(int irq, void *dev_instance)
{
	struct xgene_enet_pdev *pdev;
	struct xgene_enet_priv *priv;
	u8 bit;
	int rc;
	u32 data;
	u32 int_mask = 0;

	static const char *mac_int_reg0_msgs[32] = {
		"RxPort0 Mac i/f fifo overflow", /* bit 0 */
		"RxPort0 Mac i/f fifo underflow",
		"TxPort0 ECM Data fifo overflow",
		"TxPort0 ECM Data fifo underflow",
		"RxPort0 ECM Data fifo underrun",
		"RxPort0 ICM Ctrl fifo overflow", /* bit 5 */
		"RxPort0 ICM Ctrl fifo underflow",
		"RxPort0 ICM Data fifo overflow",
		"RxPort0 ICM Data fifo underflow",
		"TxPort0 ECM EOF control fifo overflow",
		"TxPort0 ECM EOF control fifo underflow", /* bit 10 */
		"TxPort0 ECM SOF control fifo overflow",
		"TxPort0 ECM SOF control fifo underflow",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"RxPort1 Mac i/f fifo overflow",
		"RxPort1 Mac i/f fifo underflow",
		"TxPort1 ECM Data fifo overflow",
		"TxPort1 ECM Data fifo underflow",
		"RxPort1 ECM Data fifo underrun", /* bit 20 */
		"RxPort1 ICM Ctrl fifo overflow",
		"RxPort1 ICM Ctrl fifo underflow",
		"RxPort1 ICM Data fifo overflow",
		"RxPort1 ICM Data fifo underflow",
		"TxPort1 ECM EOF control fifo overflow", /* bit 25 */
		"TxPort1 ECM EOF control fifo underflow",
		"TxPort1 ECM SOF control fifo overflow",
		"TxPort1 ECM SOF control fifo underflow",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	pdev = netdev_priv(dev_instance);
	priv = &pdev->priv;
	pr_err("Received Ethernet MAC Error Interrupt\n");

	rc = xgene_enet_rd(priv, BLOCK_MCX_MAC_CSR, MAC_INT_REG0_ADDR, &data);
	if (data) {
		pr_err("Received MAC Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s interrupt\n", mac_int_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_MCX_MAC_CSR,
				   MAC_INT_REG0_ADDR, int_mask);
	}

	return IRQ_HANDLED;
}

static irqreturn_t xgene_enet_err_irq(int irq, void *dev_instance)
{
	struct xgene_enet_pdev *pdev;
	struct xgene_enet_priv *priv;
	u8 bit;
	int rc;
	u32 data;
	u32 int_mask = 0;

	static const char *rsif_int_reg0_msgs[32] = {
		"Rx port0 buffer FIFO underflow", /* bit 0 */
		"Rx port0 buffer FIFO overflow",
		"Rx port0 ctrl buffer FIFO underflow",
		"Rx port0 ctrl buffer FIFO overflow",
		"Rx port0 classifier buffer FIFO underflow",
		"Rx port0 classifier buffer FIFO overflow", /* bit 5 */
		"Rx port0 err buffer FIFO underflow",
		"Rx port0 err buffer FIFO overflow",
		"Rx port0 timestamp FIFO underflow",
		"Rx port0 timestamp buffer FIFO overflow",
		"Rx port0 chksum buffer FIFO underflow", /* bit 10 */
		"Rx port0 chksum buffer FIFO overflow",
		"Rx port0 Local Rx buffer FIFO underflow",
		"Rx port0 Local Rx buffer FIFO overflow",
		"Policer CLE FIFO underflow",
		"Policer CLE FIFO overflow", /* bit 15 */
		"Rx port1 buffer FIFO underflow",
		"Rx port1 buffer FIFO overflow",
		"Rx port1 ctrl buffer FIFO underflow",
		"Rx port1 ctrl buffer FIFO overflow",
		"Rx port1 classifier buffer FIFO underflow", /* bit 20 */
		"Rx port1 classifier buffer FIFO overflow",
		"Rx port1 err buffer FIFO underflow",
		"Rx port1 err buffer FIFO overflow",
		"Rx port1 timestamp FIFO underflow",
		"Rx port1 timestamp buffer FIFO overflow", /* bit 25 */
		"Rx port1 chksum buffer FIFO underflow",
		"Rx port1 chksum buff FIFO overflow",
		"Rx port1 Local Rxbuff FIFO underflow",
		"Rx port1 Local Rxbuff FIFO overflow",
		"Rx policer EOF FIFO underflow",
		"Rx policer EOF FIFO overflow"
	};

	static const char *rsif_fint_reg0_msgs[32] = {
		"Packet dropped by policer", /* bit 0 */
		"AXI write error on port0",
		"Freepool buffer was NOT available from QMI on port0",
		"Split boundary can't be accomodated in the 1st buf in port0",
		"Mirror buf addr offset/len do not match with normal xfr port0",
		"Received FP buf w/ invalid len encoding on port0", /* bit 5 */
		"Number of LL buffers for a packet on port 0 exceeded 256",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 10 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"AXI write error on port1",
		"Freepool buffer was NOT available from QMI on port1",
		"Split boundary can't be accomodated in 1st buffer in port1",
		"Mirror buf addr offset/len do not match with normal xfr port1",
		"Received FP buf w/ invalid len encoding on port1", /* bit 20 */
		"Number of LL buffers for a packet on port 0 exceeded 256",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 25 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	static const char *tsif_int_reg0_msgs[32] = {
		"Tx port0 AMA buffer FIFO underflow", /* bit 0 */
		"Tx port0 AMA buffer FIFO overflow",
		"Tx port0 RRM buffer FIFO underflow",
		"Tx port0 RRM buffer FIFO overflow",
		"Tx port0 RDM buffer FIFO underflow",
		"Tx port0 RDM buffer FIFO overflow", /* bit 5 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 10 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"Tx port1 AMA buffer FIFO underflow",
		"Tx port1 AMA buffer FIFO overflow",
		"Tx port1 RRM buffer FIFO underflow",
		"Tx port1 RRM buffer FIFO overflow",
		"Tx port1 RDM buffer FIFO underflow", /* bit 20 */
		"Tx port1 RDM buffer FIFO overflow",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 25 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	static const char *tsif_fint_reg0_msgs[32] = {
		"Bad message received by TSIF on port0", /* bit 0 */
		"AXI error when reading data from port0",
		"AXI error when reading data from port0",
		"TSO error: header length > pkt length on port 0",
		"Msg received with LL=1, NV=0 on port 0",
		/* bit 5 */
		"TSO error: header len did not fit in the inline buffers port0",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 10 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"Bad message received by TSIF on port1",
		"AXI error when reading data from port1",
		"AXI error when reading data from port1",
		"TSO error: header length > pkt length on port 1",
		"Msg received with LL=1, NV=0 on port 1", /* bit 20 */
		"TSO error: header len did not fit in the inline buffers port1",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 25 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	static const char *tso_int_reg0_msgs[32] = {
		"Tx port0 Message Hold FIFO underflow", /* bit 0 */
		"Tx port0 Message Hold FIFO overflow",
		"Tx port0 Dealloc FIFO underflow",
		"Tx port0 Dealloc FIFO overflow",
		"Tx port0 tso_txbuff FIFO underflow",
		"Tx port0 tso_txbuff FIFO overflow", /* bit 5 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 10 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"Tx port1 Message Hold FIFO underflow",
		"Tx port1 Message Hold FIFO overflow",
		"Tx port1 Dealloc FIFO underflow",
		"Tx port1 Dealloc FIFO overflow",
		"Tx port1 tso_txbuff FIFO underflow", /* bit 20 */
		"Tx port1 tso_txbuff FIFO overflow",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 25 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	static const char *rx_tx_buf_chksm_int_reg0[32] = {
		"Tx port0 rx buffer FIFO  underflow", /* bit 0 */
		"Tx port0 rx buffer FIFO  overflow",
		"Rx port0 rx buffer FIFO  underflow",
		"Rx port0 rx buffer FIFO  overflow",
		"Tx port0 Chksum Error",
		"Rx port0 Chksum Error", /* bit 5 */
		"Rx port0 Pause",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 10 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 15 */
		"Tx port1 rx buffer FIFO  underflow",
		"Tx port1 rx buffer FIFO  overflow",
		"Rx port1 rx buffer FIFO  underflow",
		"Rx port1 rx buffer FIFO  overflow",
		"Tx port1 Chksum Error", /* bit 20 */
		"Rx port1 Chksum Error",
		"Rx port1 Pause",
		"Rsvd",
		"Rsvd",
		"Rsvd", /* bit 25 */
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd",
		"Rsvd"
	};

	pdev = netdev_priv(dev_instance);
	priv = &pdev->priv;
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, RSIF_INT_REG0_ADDR, &data);
	if (data) {
		pr_err("Received RSIF Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s interrupt\n", rsif_int_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   RSIF_INT_REG0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, RSIF_FINT_REG0_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received RSIF Error2 Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s\n", rsif_fint_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   RSIF_FINT_REG0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, TSIF_INT_REG0_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received TSIF Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s interrupt\n", tsif_int_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   TSIF_INT_REG0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, TSIF_FINT_REG0_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received TSIF Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s\n", tsif_fint_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   TSIF_FINT_REG0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR, TSO_INT_REG0_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received TSO Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s interrupt\n", tso_int_reg0_msgs[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   TSO_INT_REG0_ADDR, int_mask);
	}
	rc = xgene_enet_rd(priv, BLOCK_ETH_CSR,
			   RX_TX_BUF_CHKSM_INT_REG0_ADDR, &data);
	int_mask = 0;
	if (data) {
		pr_err("Received RX/TX Buffer Checksum Error Interrupt\n");
		for_each_set_bit(bit, (unsigned long *)&data, 32) {
			int_mask |= bit;
			pr_err("%s interrupt\n", rx_tx_buf_chksm_int_reg0[bit]);
		}
		/* Clear intrstatus bits, its COW */
		rc = xgene_enet_wr(priv, BLOCK_ETH_CSR,
				   RX_TX_BUF_CHKSM_INT_REG0_ADDR, int_mask);
	}

	return IRQ_HANDLED;
}

void xgene_enet_register_err_irqs(struct net_device *ndev)
{
	struct xgene_enet_pdev *pdev;
	struct device *dev;

	pdev = (struct xgene_enet_pdev *)netdev_priv(ndev);
	dev = &pdev->plat_dev->dev;

	if ((devm_request_irq(dev, pdev->enet_err_irq, xgene_enet_err_irq,
			 IRQF_SHARED, ndev->name, ndev)) != 0)
		netdev_err(ndev, "Failed to reg Enet Error IRQ %d\n",
			   pdev->enet_err_irq);
	if ((devm_request_irq(dev, pdev->enet_mac_err_irq,
			 xgene_enet_mac_err_irq, IRQF_SHARED,
			 ndev->name, ndev)) != 0)
		netdev_err(ndev, "Failed to reg Enet MAC Error IRQ %d\n",
			   pdev->enet_mac_err_irq);
	if ((devm_request_irq(dev, pdev->enet_qmi_err_irq,
			 xgene_enet_qmi_err_irq,
			 IRQF_SHARED, ndev->name, ndev)) != 0)
		netdev_err(ndev, "Failed to reg Enet QMI Error IRQ %d\n",
			   pdev->enet_qmi_err_irq);
}
