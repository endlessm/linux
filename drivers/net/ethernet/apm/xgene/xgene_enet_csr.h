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

#ifndef __XGENE_ENET_CSR_H__
#define __XGENE_ENET_CSR_H__

#define RSIF_INT_REG0_ADDR                                           0x00000054
#define RSIF_FINT_REG0_ADDR                                          0x0000005c
#define TSIF_INT_REG0_ADDR                                           0x0000012c
#define TSIF_FINT_REG0_ADDR                                          0x00000134
#define TSO_INT_REG0_ADDR                                            0x00000324
#define SPI2SAP_INT_REG0_ADDR                                        0x00000448
#define RX_TX_BUF_CHKSM_INT_REG0_ADDR                                0x0000052c
#define ENET_SPARE_CFG_REG_ADDR                                      0x00000750
#define RSIF_CONFIG_REG_ADDR                                         0x00000010
#define RSIF_RAM_DBG_REG0_ADDR                                       0x00000048
#define RGMII_REG_0_ADDR                                             0x000007e0
#define CFG_LINK_AGGR_RESUME_0_ADDR                                  0x000007c8
#define DEBUG_REG_ADDR                                               0x00000700
#define CFG_BYPASS_ADDR                                              0x00000294
#define CLE_BYPASS_REG0_0_ADDR                                       0x00000490
#define CLE_BYPASS_REG1_0_ADDR                                       0x00000494
#define CLE_BYPASS_REG8_0_ADDR                                       0x000004b0
#define TSIF_MSS_REG0_0_ADDR                                         0x00000108
#define TSIF_MSS_REG1_0_ADDR                                         0x00000110
#define TSO_CFG_0_ADDR                                               0x00000314
#define TSO_CFG_INSERT_VLAN_0_ADDR                                   0x0000031c
#define CFG_RSIF_FPBUFF_TIMEOUT_EN_WR(src) \
	(((u32)(src)<<31) & 0x80000000)
#define RSIF_LCL_RXBUF_FIFO_OVERFL_INTR_RXPRT10_MASK                 0x20000000
#define RSIF_LCL_RXBUF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x20000000)>>29)
#define RSIF_LCL_RXBUF_FIFO_UNDERFL_INTR_RXPRT10_MASK                0x10000000
#define RSIF_LCL_RXBUF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x10000000)>>28)
#define RSIF_CHKSUM_BUFF_FIFO_OVERFL_INTR_RXPRT10_MASK               0x08000000
#define RSIF_CHKSUM_BUFF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x08000000)>>27)
#define RSIF_CHKSUM_BUFF_FIFO_UNDERFL_INTR_RXPRT10_MASK              0x04000000
#define RSIF_CHKSUM_BUFF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x04000000)>>26)
#define RSIF_TIMESTAMP_BUFF_FIFO_OVERFL_INTR_RXPRT10_MASK            0x02000000
#define RSIF_TIMESTAMP_BUFF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x02000000)>>25)
#define RSIF_TIMESTAMP_BUFF_FIFO_UNDERFL_INTR_RXPRT10_MASK           0x01000000
#define RSIF_TIMESTAMP_BUFF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x01000000)>>24)
#define RSIF_ERR_BUFF_FIFO_OVERFL_INTR_RXPRT10_MASK                  0x00800000
#define RSIF_ERR_BUFF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00800000)>>23)
#define RSIF_ERR_BUFF_FIFO_UNDERFL_INTR_RXPRT10_MASK                 0x00400000
#define RSIF_ERR_BUFF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00400000)>>22)
#define RSIF_CLEBUFF_FIFO_OVERFL_INTR_RXPRT10_MASK                   0x00200000
#define RSIF_CLEBUFF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00200000)>>21)
#define RSIF_CLEBUFF_FIFO_UNDERFL_INTR_RXPRT10_MASK                  0x00100000
#define RSIF_CLEBUFF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00100000)>>20)
#define RSIF_CTRLBUFF_FIFO_OVERFL_INTR_RXPRT10_MASK                  0x00080000
#define RSIF_CTRLBUFF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00080000)>>19)
#define RSIF_CTRLBUFF_FIFO_UNDERFL_INTR_RXPRT10_MASK                 0x00040000
#define RSIF_CTRLBUFF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00040000)>>18)
#define RSIF_BUF_FIFO_OVERFL_INTR_RXPRT10_MASK                       0x00020000
#define RSIF_BUF_FIFO_OVERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00020000)>>17)
#define RSIF_BUF_FIFO_UNDERFL_INTR_RXPRT10_MASK                      0x00010000
#define RSIF_BUF_FIFO_UNDERFL_INTR_RXPRT10_RD(src) \
	(((src) & 0x00010000)>>16)
#define ENET_RSIF_PLC_CLEBUFF_FIFO_OVERFL_INTR0_MASK              0x00008000
#define ENET_RSIF_PLC_CLEBUFF_FIFO_OVERFL_INTR0_RD(src) \
	(((src) & 0x00008000)>>15)
#define ENET_RSIF_PLC_CLEBUFF_FIFO_UNDERFL_INTR0_RD(src) \
	(((src) & 0x00004000)>>14)
#define RSIF_LCL_RXBUF_FIFO_UNDERFL_INTR_RXPRT00_MASK                0x00001000
#define RSIF_LCL_RXBUF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00001000)>>12)
#define RSIF_LCL_RXBUF_FIFO_OVERFL_INTR_RXPRT00_MASK                 0x00002000
#define RSIF_LCL_RXBUF_FIFO_OVERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00002000)>>13)
#define RSIF_CHKSUM_BUFF_FIFO_OVERFL_INTR_RXPRT00_MASK               0x00000800
#define RSIF_CHKSUM_BUFF_FIFO_OVERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000800)>>11)
#define RSIF_CHKSUM_BUFF_FIFO_UNDERFL_INTR_RXPRT00_MASK              0x00000400
#define RSIF_CHKSUM_BUFF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000400)>>10)
#define RSIF_TIMESTAMP_BUFF_FIFO_OVERFL_INTR_RXPRT00_MASK            0x00000200
#define RSIF_TIMESTAMP_BUFF_FIFO_OVERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000200)>>9)
#define RSIF_TIMESTAMP_BUFF_FIFO_UNDERFL_INTR_RXPRT00_MASK           0x00000100
#define RSIF_TIMESTAMP_BUFF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000100)>>8)
#define RSIF_ERR_BUFF_FIFO_OVERFL_INTR_RXPRT00_MASK                  0x00000080
#define RSIF_ERR_BUFF_FIFO_OVERFL_INTR_RXPRT00_RD(src) (((src) & 0x00000080)>>7)
#define RSIF_ERR_BUFF_FIFO_UNDERFL_INTR_RXPRT00_MASK                 0x00000040
#define RSIF_ERR_BUFF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000040)>>6)
#define RSIF_CLEBUFF_FIFO_OVERFL_INTR_RXPRT00_MASK                   0x00000020
#define RSIF_CLEBUFF_FIFO_OVERFL_INTR_RXPRT00_RD(src) (((src) & 0x00000020)>>5)
#define RSIF_CLEBUFF_FIFO_UNDERFL_INTR_RXPRT00_MASK                  0x00000010
#define RSIF_CLEBUFF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) (((src) & 0x00000010)>>4)
#define RSIF_CTRLBUFF_FIFO_OVERFL_INTR_RXPRT00_MASK                  0x00000008
#define RSIF_CTRLBUFF_FIFO_OVERFL_INTR_RXPRT00_RD(src) (((src) & 0x00000008)>>3)
#define RSIF_CTRLBUFF_FIFO_UNDERFL_INTR_RXPRT00_MASK                 0x00000004
#define RSIF_CTRLBUFF_FIFO_UNDERFL_INTR_RXPRT00_RD(src) \
	(((src) & 0x00000004)>>2)
#define RSIF_BUF_FIFO_OVERFL_INTR_RXPRT00_MASK                       0x00000002
#define RSIF_BUF_FIFO_OVERFL_INTR_RXPRT00_RD(src)     (((src) & 0x00000002)>>1)
#define RSIF_BUF_FIFO_UNDERFL_INTR_RXPRT00_MASK                      0x00000001
#define RSIF_BUF_FIFO_UNDERFL_INTR_RXPRT00_RD(src)       (((src) & 0x00000001))
#define RSIF_SS_MIRRORERR_INTR_RXPRT10_MASK                          0x00080000
#define RSIF_SS_MIRRORERR_INTR_RXPRT10_RD(src)       (((src) & 0x00080000)>>19)
#define RSIF_SS_SPLIT_BOUNDARY_INTR_RXPRT10_RD(src)  (((src) & 0x00040000)>>18)
#define RSIF_SS_FPBUFF_TIMEOUT_INTR_RXPRT10_MASK                     0x00020000
#define RSIF_SS_FPBUFF_TIMEOUT_INTR_RXPRT10_RD(src)  (((src) & 0x00020000)>>17)
#define RSIF_SS_AXI_WRERR_INTR_RXPRT10_MASK                          0x00010000
#define RSIF_SS_AXI_WRERR_INTR_RXPRT10_RD(src)       (((src) & 0x00010000)>>16)
#define RSIF_SS_MIRRORERR_INTR_RXPRT00_MASK                          0x00000010
#define RSIF_SS_MIRRORERR_INTR_RXPRT00_RD(src)        (((src) & 0x00000010)>>4)
#define RSIF_SS_SPLIT_BOUNDARY_INTR_RXPRT00_MASK                     0x00000008
#define RSIF_SS_SPLIT_BOUNDARY_INTR_RXPRT00_RD(src)   (((src) & 0x00000008)>>3)
#define RSIF_SS_FPBUFF_TIMEOUT_INTR_RXPRT00_MASK                     0x00000004
#define RSIF_SS_FPBUFF_TIMEOUT_INTR_RXPRT00_RD(src)   (((src) & 0x00000004)>>2)
#define RSIF_SS_AXI_WRERR_INTR_RXPRT00_MASK                          0x00000002
#define RSIF_SS_AXI_WRERR_INTR_RXPRT00_RD(src)        (((src) & 0x00000002)>>1)
#define STS_RSIF_PLC_DROP0_MASK                                      0x00000001
#define STS_RSIF_PLC_DROP0_RD(src)                       (((src) & 0x00000001))
#define CFG_TSIF_MSS_SZ10_SET(dst, src) \
	(((dst) & ~0x3fff0000) | (((u32)(src)<<16) & 0x3fff0000))
#define CFG_TSIF_MSS_SZ00_SET(dst, src) \
	(((dst) & ~0x00003fff) | (((u32)(src)) & 0x00003fff))
#define CFG_TSIF_MSS_SZ20_SET(dst, src) \
	(((dst) & ~0x00003fff) | (((u32)(src)) & 0x00003fff))
#define CFG_TSIF_MSS_SZ30_SET(dst, src) \
	(((dst) & ~0x3fff0000) | (((u32)(src)<<16) & 0x3fff0000))
#define TSIF_RDMBUFF_FIFO_OVERFL_INTR_PRT10_MASK                     0x00200000
#define TSIF_RDMBUFF_FIFO_OVERFL_INTR_PRT10_RD(src)  (((src) & 0x00200000)>>21)
#define TSIF_RDMBUFF_FIFO_UNDERFL_INTR_PRT10_MASK                    0x00100000
#define TSIF_RDMBUFF_FIFO_UNDERFL_INTR_PRT10_RD(src) (((src) & 0x00100000)>>20)
#define TSIF_RRM_FIFO_OVERFL_INTR_PRT10_MASK                         0x00080000
#define TSIF_RRM_FIFO_OVERFL_INTR_PRT10_RD(src)      (((src) & 0x00080000)>>19)
#define TSIF_RRM_FIFO_UNDERFL_INTR_PRT10_MASK                        0x00040000
#define TSIF_RRM_FIFO_UNDERFL_INTR_PRT10_RD(src)     (((src) & 0x00040000)>>18)
#define TSIF_AMABUF_FIFO_OVERFL_INTR_PRT10_MASK                      0x00020000
#define TSIF_AMABUF_FIFO_OVERFL_INTR_PRT10_RD(src)   (((src) & 0x00020000)>>17)
#define TSIF_AMABUF_FIFO_UNDERFL_INTR_PRT10_MASK                     0x00010000
#define TSIF_AMABUF_FIFO_UNDERFL_INTR_PRT10_RD(src)  (((src) & 0x00010000)>>16)
#define TSIF_RDMBUFF_FIFO_OVERFL_INTR_PRT00_MASK                     0x00000020
#define TSIF_RDMBUFF_FIFO_OVERFL_INTR_PRT00_RD(src)   (((src) & 0x00000020)>>5)
#define TSIF_RDMBUFF_FIFO_UNDERFL_INTR_PRT00_MASK                    0x00000010
#define TSIF_RDMBUFF_FIFO_UNDERFL_INTR_PRT00_RD(src)  (((src) & 0x00000010)>>4)
#define TSIF_RRM_FIFO_OVERFL_INTR_PRT00_MASK                         0x00000008
#define TSIF_RRM_FIFO_OVERFL_INTR_PRT00_RD(src)       (((src) & 0x00000008)>>3)
#define TSIF_RRM_FIFO_UNDERFL_INTR_PRT00_MASK                        0x00000004
#define TSIF_RRM_FIFO_UNDERFL_INTR_PRT00_RD(src)      (((src) & 0x00000004)>>2)
#define TSIF_AMABUF_FIFO_OVERFL_INTR_PRT00_MASK                      0x00000002
#define TSIF_AMABUF_FIFO_OVERFL_INTR_PRT00_RD(src)    (((src) & 0x00000002)>>1)
#define TSIF_AMABUF_FIFO_UNDERFL_INTR_PRT00_MASK                     0x00000001
#define TSIF_AMABUF_FIFO_UNDERFL_INTR_PRT00_RD(src)      (((src) & 0x00000001))
#define TSIF_SS_AXI_LLRDERR_INTR_PRT10_MASK                          0x00040000
#define TSIF_SS_AXI_LLRDERR_INTR_PRT10_RD(src)       (((src) & 0x00040000)>>18)
#define TSIF_SS_AXI_RDERR_INTR_PRT10_MASK                            0x00020000
#define TSIF_SS_AXI_RDERR_INTR_PRT10_RD(src)         (((src) & 0x00020000)>>17)
#define TSIF_SS_BAD_MSG_INTR_PRT10_MASK                              0x00010000
#define TSIF_SS_BAD_MSG_INTR_PRT10_RD(src)           (((src) & 0x00010000)>>16)
#define TSIF_SS_AXI_LLRDERR_INTR_PRT00_MASK                          0x00000004
#define TSIF_SS_AXI_LLRDERR_INTR_PRT00_RD(src)        (((src) & 0x00000004)>>2)
#define TSIF_SS_AXI_RDERR_INTR_PRT00_MASK                            0x00000002
#define TSIF_SS_AXI_RDERR_INTR_PRT00_RD(src)          (((src) & 0x00000002)>>1)
#define TSIF_SS_BAD_MSG_INTR_PRT00_MASK                              0x00000001
#define TSIF_SS_BAD_MSG_INTR_PRT00_RD(src)               (((src) & 0x00000001))
#define RESUME_TX_WR(src)                           (((u32)(src)) & 0x00000001)
#define MB_TTF_FIFO_OVERFL_INTR_PRT10_RD(src)        (((src) & 0x00200000)>>21)
#define MB_TTF_FIFO_OVERFL_INTR_PRT10_MASK                           0x00200000
#define MB_TTF_FIFO_UNDERFL_INTR_PRT10_RD(src)       (((src) & 0x00100000)>>20)
#define MB_TTF_FIFO_UNDERFL_INTR_PRT10_MASK                          0x00100000
#define MH_DEALLOC_FIFO_OVERFL_INTR_PRT10_RD(src)    (((src) & 0x00080000)>>19)
#define MH_DEALLOC_FIFO_OVERFL_INTR_PRT10_MASK                       0x00080000
#define MH_DEALLOC_FIFO_UNDERFL_INTR_PRT10_RD(src)   (((src) & 0x00040000)>>18)
#define MH_DEALLOC_FIFO_UNDERFL_INTR_PRT10_MASK                      0x00040000
#define MH_FIFO_OVERFL_INTR_PRT10_RD(src)            (((src) & 0x00020000)>>17)
#define MH_FIFO_OVERFL_INTR_PRT10_MASK                               0x00020000
#define MH_FIFO_UNDERFL_INTR_PRT10_RD(src)           (((src) & 0x00010000)>>16)
#define MH_FIFO_UNDERFL_INTR_PRT10_MASK                              0x00010000
#define MB_TTF_FIFO_OVERFL_INTR_PRT00_RD(src)         (((src) & 0x00000020)>>5)
#define MB_TTF_FIFO_OVERFL_INTR_PRT00_MASK                           0x00000020
#define MB_TTF_FIFO_UNDERFL_INTR_PRT00_RD(src)        (((src) & 0x00000010)>>4)
#define MB_TTF_FIFO_UNDERFL_INTR_PRT00_MASK                          0x00000010
#define MH_DEALLOC_FIFO_OVERFL_INTR_PRT00_RD(src)     (((src) & 0x00000008)>>3)
#define MH_DEALLOC_FIFO_OVERFL_INTR_PRT00_MASK                       0x00000008
#define MH_DEALLOC_FIFO_UNDERFL_INTR_PRT00_RD(src)    (((src) & 0x00000004)>>2)
#define MH_DEALLOC_FIFO_UNDERFL_INTR_PRT00_MASK                      0x00000004
#define MH_FIFO_OVERFL_INTR_PRT00_RD(src)             (((src) & 0x00000002)>>1)
#define MH_FIFO_OVERFL_INTR_PRT00_MASK                               0x00000002
#define MH_FIFO_UNDERFL_INTR_PRT00_RD(src)               (((src) & 0x00000001))
#define MH_FIFO_UNDERFL_INTR_PRT00_MASK                              0x00000001
#define MACIF_FIFO_OVERFL_INTR_RXPRT10_RD(src)       (((src) & 0x00020000)>>17)
#define MACIF_FIFO_OVERFL_INTR_RXPRT10_MASK                          0x00020000
#define MACIF_FIFO_OVERFL_INTR_RXPRT00_RD(src)        (((src) & 0x00000002)>>1)
#define MACIF_FIFO_OVERFL_INTR_RXPRT00_MASK                          0x00000002
#define MACIF_FIFO_UNDERFL_INTR_RXPRT10_RD(src)      (((src) & 0x00010000)>>16)
#define MACIF_FIFO_UNDERFL_INTR_RXPRT10_MASK                         0x00010000
#define MACIF_FIFO_UNDERFL_INTR_RXPRT00_RD(src)          (((src) & 0x00000001))
#define MACIF_FIFO_UNDERFL_INTR_RXPRT00_MASK                         0x00000001
#define RXBUF_PAUSE_INTR_PORT10_RD(src)              (((src) & 0x00400000)>>22)
#define RXBUF_PAUSE_INTR_PORT10_MASK                                 0x00400000
#define RX_CHKSUM_INTR_PORT10_RD(src)                (((src) & 0x00200000)>>21)
#define RX_CHKSUM_INTR_PORT10_MASK                                   0x00200000
#define TX_CHKSUM_INTR_PORT10_RD(src)                (((src) & 0x00100000)>>20)
#define TX_CHKSUM_INTR_PORT10_MASK                                   0x00100000
#define RXBUF_FIFO_OVERFL_INTR_PRT10_RD(src)         (((src) & 0x00080000)>>19)
#define RXBUF_FIFO_OVERFL_INTR_PRT10_MASK                            0x00080000
#define RXBUF_FIFO_UNDERFL_INTR_PRT10_RD(src)        (((src) & 0x00040000)>>18)
#define RXBUF_FIFO_UNDERFL_INTR_PRT10_MASK                           0x00040000
#define TXBUF_FIFO_OVERFL_INTR_PRT10_RD(src)         (((src) & 0x00020000)>>17)
#define TXBUF_FIFO_UNDERFL_INTR_PRT10_RD(src)        (((src) & 0x00010000)>>16)
#define RXBUF_PAUSE_INTR_PORT00_RD(src)               (((src) & 0x00000040)>>6)
#define RX_CHKSUM_INTR_PORT00_RD(src)                 (((src) & 0x00000020)>>5)
#define RX_CHKSUM_INTR_PORT00_MASK                                   0x00000020
#define RXBUF_PAUSE_INTR_PORT00_MASK                                 0x00000040
#define TX_CHKSUM_INTR_PORT00_RD(src)                 (((src) & 0x00000010)>>4)
#define TX_CHKSUM_INTR_PORT00_MASK                                   0x00000010
#define RXBUF_FIFO_OVERFL_INTR_PRT00_RD(src)          (((src) & 0x00000008)>>3)
#define RXBUF_FIFO_OVERFL_INTR_PRT00_MASK                            0x00000008
#define RXBUF_FIFO_UNDERFL_INTR_PRT00_RD(src)         (((src) & 0x00000004)>>2)
#define RXBUF_FIFO_UNDERFL_INTR_PRT00_MASK                           0x00000004
#define TXBUF_FIFO_OVERFL_INTR_PRT00_RD(src)          (((src) & 0x00000002)>>1)
#define TXBUF_FIFO_UNDERFL_INTR_PRT00_RD(src)            (((src) & 0x00000001))
#define CFG_SPEED_1250_WR(src)                  (((u32)(src)<<24) & 0x01000000)
#define CFG_TXCLK_MUXSEL0_WR(src)               (((u32)(src)<<29) & 0xe0000000)
#define TX_PORT0_WR(src)                            (((u32)(src)) & 0x00000001)
#define CFG_BYPASS_UNISEC_TX_WR(src)             (((u32)(src)<<2) & 0x00000004)
#define CFG_BYPASS_UNISEC_RX_WR(src)             (((u32)(src)<<1) & 0x00000002)
#define CFG_CLE_BYPASS_EN0_SET(dst, src) \
	(((dst) & ~0x80000000) | (((u32)(src)<<31) & 0x80000000))
#define CFG_CLE_IP_PROTOCOL0_SET(dst, src) \
	(((dst) & ~0x00030000) | (((u32)(src)<<16) & 0x00030000))
#define CFG_CLE_DSTQID0_SET(dst, src) \
	(((dst) & ~0x00000fff) | (((u32)(src)) & 0x00000fff))
#define CFG_CLE_FPSEL0_SET(dst, src) \
	(((dst) & ~0x000f0000) | (((u32)(src)<<16) & 0x000f0000))
#define CFG_CLE_HENQNUM0_SET(dst, src) \
	(((dst) & ~0x0fff0000) | (((u32)(src)<<16) & 0x0fff0000))
#define MAC_INT_REG0_ADDR                                            0x00000514
#define MAC_INT_REG1_ADDR                                            0x0000051c
#define ICM_CONFIG0_REG_0_ADDR                                       0x00000400
#define ICM_CONFIG2_REG_0_ADDR                                       0x00000410
#define ECM_CONFIG0_REG_0_ADDR                                       0x00000500
#define RX_DV_GATE_REG_0_ADDR                                        0x000005fc
#define TX_DV_GATE_EN0_SET(dst, src) \
	(((dst) & ~0x00000004) | (((u32)(src)<<2) & 0x00000004))
#define RX_DV_GATE_EN0_SET(dst, src) \
	(((dst) & ~0x00000002) | (((u32)(src)<<1) & 0x00000002))
#define RESUME_RX0_SET(dst, src) \
	(((dst) & ~0x00000001) | (((u32)(src)) & 0x00000001))
#define ICM_DATA_FIFO_UNDERFL_INTR_PRT10_RD(src)     (((src) & 0x01000000)>>24)
#define ICM_DATA_FIFO_UNDERFL_INTR_PRT10_MASK                        0x01000000
#define ICM_DATA_FIFO_OVERFL_INTR_PRT10_MASK                         0x00800000
#define ICM_CTRL_FIFO_OVERFL_INTR_PRT00_RD(src)       (((src) & 0x00000020)>>5)
#define ICM_CTRL_FIFO_OVERFL_INTR_PRT00_MASK                         0x00000020
#define ICM_CTRL_FIFO_OVERFL_INTR_PRT10_RD(src)      (((src) & 0x00200000)>>21)
#define ICM_CTRL_FIFO_OVERFL_INTR_PRT10_MASK                         0x00200000
#define ICM_CTRL_FIFO_UNDERFL_INTR_PRT10_RD(src)     (((src) & 0x00400000)>>22)
#define ICM_CTRL_FIFO_UNDERFL_INTR_PRT10_MASK                        0x00400000
#define ICM_CTRL_FIFO_UNDERFL_INTR_PRT00_RD(src)      (((src) & 0x00000040)>>6)
#define ICM_CTRL_FIFO_UNDERFL_INTR_PRT00_MASK                        0x00000040
#define ICM_DATA_FIFO_OVERFL_INTR_PRT00_RD(src)       (((src) & 0x00000080)>>7)
#define ICM_DATA_FIFO_OVERFL_INTR_PRT10_RD(src)      (((src) & 0x00800000)>>23)
#define ICM_DATA_FIFO_OVERFL_INTR_PRT00_MASK                         0x00000080
#define ICM_DATA_FIFO_UNDERFL_INTR_PRT00_RD(src)      (((src) & 0x00000100)>>8)
#define ICM_DATA_FIFO_UNDERFL_INTR_PRT00_MASK                        0x00000100
#define ECM_DATA_FIFO_UNDERN_INTR_PRT00_RD(src)       (((src) & 0x00000010)>>4)
#define ECM_DATA_FIFO_UNDERN_INTR_PRT00_MASK                         0x00000010
#define ECM_DATA_FIFO_UNDERN_INTR_PRT10_RD(src)      (((src) & 0x00100000)>>20)
#define ECM_DATA_FIFO_UNDERN_INTR_PRT10_MASK                         0x00100000
#define ECM_DATA_FIFO_UNDERFL_INTR_PRT10_RD(src)     (((src) & 0x00080000)>>19)
#define ECM_DATA_FIFO_UNDERFL_INTR_PRT10_MASK                        0x00080000
#define ECM_DATA_FIFO_OVERFL_INTR_PRT10_RD(src)      (((src) & 0x00040000)>>18)
#define ECM_DATA_FIFO_OVERFL_INTR_PRT10_MASK                         0x00040000
#define CARRY_PORT01_RD(src)                             (((src) & 0x00000001))
#define CARRY_PORT01_MASK                                            0x00000001
#define LINKDOWN_PORT11_RD(src)                       (((src) & 0x00000008)>>3)
#define LINKDOWN_PORT11_MASK                                         0x00000008
#define CARRY_PORT11_RD(src)                          (((src) & 0x00000004)>>2)
#define CARRY_PORT11_MASK                                            0x00000004
#define LINKDOWN_PORT01_RD(src)                       (((src) & 0x00000002)>>1)
#define LINKDOWN_PORT01_MASK                                         0x00000002
#define MACIF_FIFO_UNDERFL_INTR_RXPRT00_F2_RD(src)    (((src) & 0x00000002)>>1)
#define MACIF_FIFO_UNDERFL_INTR_RXPRT00_F2_MASK                      0x00000002
#define MACIF_FIFO_UNDERFL_INTR_RXPRT10_F2_RD(src)   (((src) & 0x00020000)>>17)
#define MACIF_FIFO_UNDERFL_INTR_RXPRT10_F2_MASK                      0x00020000
#define MACIF_FIFO_OVERFL_INTR_RXPRT00_F2_RD(src)        (((src) & 0x00000001))
#define MACIF_FIFO_OVERFL_INTR_RXPRT00_F2_MASK                       0x00000001
#define MACIF_FIFO_OVERFL_INTR_RXPRT10_F2_RD(src)    (((src) & 0x00010000)>>16)
#define MACIF_FIFO_OVERFL_INTR_RXPRT10_F2_MASK                       0x00010000
#define ECM_DATA_FIFO_UNDERFL_INTR_PRT00_RD(src)      (((src) & 0x00000008)>>3)
#define ECM_DATA_FIFO_UNDERFL_INTR_PRT00_MASK                        0x00000008
#define ECM_DATA_FIFO_OVERFL_INTR_PRT00_RD(src)       (((src) & 0x00000004)>>2)
#define ECM_DATA_FIFO_OVERFL_INTR_PRT00_MASK                         0x00000004
#define ENET_STSSSQMIINT0_ADDR                                    0x0000009c
#define ENET_STSSSQMIINT1_ADDR                                    0x000000a4
#define ENET_STSSSQMIINT2_ADDR                                    0x000000ac
#define ENET_STSSSQMIINT3_ADDR                                    0x000000b4
#define ENET_STSSSQMIINT4_ADDR                                    0x000000bc
#define ENET_CFGSSQMIWQASSOC_ADDR                                 0x000000e0
#define ENET_CFGSSQMIFPQASSOC_ADDR                                0x000000dc
#define ENET_CFGSSQMIQMLITEFPQASSOC_ADDR                          0x000000f0
#define ENET_CFGSSQMIQMLITEWQASSOC_ADDR                           0x000000f4
#define ENET_FPOVERFLOW0_RD(src)                      (((src) & 0xffffffff))
#define ENET_WQOVERFLOW1_RD(src)                      (((src) & 0xffffffff))
#define ENET_FPUNDERRUN2_RD(src)                      (((src) & 0xffffffff))
#define ENET_WQUNDERRUN3_RD(src)                      (((src) & 0xffffffff))
#define ENET_AXIWCMR_SLVERR4_RD(src)               (((src) & 0x00000002)>>1)
#define ENET_AXIWCMR_SLVERR4_MASK                                 0x00000002
#define ENET_FPOVERFLOW0_MASK                                     0xffffffff
#define ENET_WQOVERFLOW1_MASK                                     0xffffffff
#define ENET_FPUNDERRUN2_MASK                                     0xffffffff
#define ENET_WQUNDERRUN3_MASK                                     0xffffffff
#define ENET_CLKEN_ADDR                                              0x00000008
#define ENET_SRST_ADDR                                               0x00000000
#define CSR0_RESET_WR(src)                          (((u32)(src)) & 0x00000001)
#define ENET0_RESET_WR(src)                      (((u32)(src)<<1) & 0x00000002)
#define CSR1_RESET_WR(src)                       (((u32)(src)<<2) & 0x00000004)
#define ENET1_RESET_WR(src)                      (((u32)(src)<<3) & 0x00000008)
#define CSR0_CLKEN_WR(src)                          (((u32)(src)) & 0x00000001)
#define ENET0_CLKEN_WR(src)                      (((u32)(src)<<1) & 0x00000002)
#define CSR1_CLKEN_WR(src)                       (((u32)(src)<<2) & 0x00000004)
#define ENET1_CLKEN_WR(src)                      (((u32)(src)<<3) & 0x00000008)
#define ENET_CFG_MEM_RAM_SHUTDOWN_ADDR                            0x00000070
#define ENET_BLOCK_MEM_RDY_ADDR                                   0x00000074
#define MAC_CONFIG_1_ADDR                                            0x00000000
#define MAC_CONFIG_2_ADDR                                            0x00000004
#define MAX_FRAME_LEN_ADDR                                           0x00000010
#define MII_MGMT_CONFIG_ADDR                                         0x00000020
#define MII_MGMT_COMMAND_ADDR                                        0x00000024
#define MII_MGMT_ADDRESS_ADDR                                        0x00000028
#define MII_MGMT_CONTROL_ADDR                                        0x0000002c
#define MII_MGMT_STATUS_ADDR                                         0x00000030
#define MII_MGMT_INDICATORS_ADDR                                     0x00000034
#define INTERFACE_CONTROL_ADDR                                       0x00000038
#define STATION_ADDR0_ADDR                                           0x00000040
#define STATION_ADDR1_ADDR                                           0x00000044
#define RX_FLOW_EN1_MASK                                             0x00000020
#define TX_FLOW_EN1_MASK                                             0x00000010
#define RX_FLOW_EN1_RD(src)                           (((src) & 0x00000020)>>5)
#define TX_FLOW_EN1_RD(src)                           (((src) & 0x00000010)>>4)
#define SCAN_CYCLE_MASK                                              0x00000002
#define SOFT_RESET1_MASK                                             0x80000000
#define MAX_FRAME_LEN_SET(dst, src) \
	(((dst) & ~0x0000ffff) | (((u32)(src)) & 0x0000ffff))
#define PHY_ADDR_SET(dst, src) \
	(((dst) & ~0x00001f00) | (((u32)(src)<<8) & 0x00001f00))
#define REG_ADDR_SET(dst, src) \
	(((dst) & ~0x0000001f) | (((u32)(src)) & 0x0000001f))
#define RESET_TX_FUN1_WR(src)                   (((u32)(src)<<16) & 0x00010000)
#define RESET_RX_FUN1_WR(src)                   (((u32)(src)<<17) & 0x00020000)
#define RESET_TX_MC1_WR(src)                    (((u32)(src)<<18) & 0x00040000)
#define RESET_RX_MC1_WR(src)                    (((u32)(src)<<19) & 0x00080000)
#define SIM_RESET1_WR(src)                      (((u32)(src)<<30) & 0x40000000)
#define SOFT_RESET1_WR(src)                     (((u32)(src)<<31) & 0x80000000)
#define TX_EN1_WR(src)                              (((u32)(src)) & 0x00000001)
#define RX_EN1_WR(src)                           (((u32)(src)<<2) & 0x00000004)
#define TX_FLOW_EN1_WR(src)                      (((u32)(src)<<4) & 0x00000010)
#define LOOP_BACK1_WR(src)                       (((u32)(src)<<8) & 0x00000100)
#define RX_FLOW_EN1_WR(src)                      (((u32)(src)<<5) & 0x00000020)
#define ENET_LHD_MODE_WR(src)                (((u32)(src)<<25) & 0x02000000)
#define ENET_GHD_MODE_WR(src)                (((u32)(src)<<26) & 0x04000000)
#define FULL_DUPLEX2_WR(src)                        (((u32)(src)) & 0x00000001)
#define LENGTH_CHECK2_WR(src)                    (((u32)(src)<<4) & 0x00000010)
#define HUGE_FRAME_EN2_WR(src)                   (((u32)(src)<<5) & 0x00000020)
#define ENET_INTERFACE_MODE2_WR(src)          (((u32)(src)<<8) & 0x00000300)
#define PAD_CRC2_WR(src)                         (((u32)(src)<<2) & 0x00000004)
#define CRC_EN2_WR(src)                          (((u32)(src)<<1) & 0x00000002)
#define PREAMBLE_LENGTH2_WR(src)                (((u32)(src)<<12) & 0x0000f000)
#define MAX_FRAME_LEN_WR(src)                       (((u32)(src)) & 0x0000ffff)
#define MGMT_CLOCK_SEL_SET(dst, src) \
	(((dst) & ~0x00000007) | (((u32)(src)) & 0x00000007))
#define RX_EN1_SET(dst, src) \
	(((dst) & ~0x00000004) | (((u32)(src)<<2) & 0x00000004))
#define TX_EN1_SET(dst, src) \
	(((dst) & ~0x00000001) | (((u32)(src)) & 0x00000001))
#define SCAN_AUTO_INCR_MASK                                          0x00000020
#define RBYT_ADDR                                                    0x00000027
#define RPKT_ADDR                                                    0x00000028
#define RFCS_ADDR                                                    0x00000029
#define RALN_ADDR                                                    0x0000002f
#define RFLR_ADDR                                                    0x00000030
#define RUND_ADDR                                                    0x00000033
#define ROVR_ADDR                                                    0x00000034
#define RDRP_ADDR                                                    0x00000037
#define TBYT_ADDR                                                    0x00000038
#define TPKT_ADDR                                                    0x00000039
#define TDRP_ADDR                                                    0x00000045
#define TFCS_ADDR                                                    0x00000047
#define TUND_ADDR                                                    0x0000004a
#define RX_BYTE_CNTR_MASK                                            0x7fffffff
#define RX_PKT_CNTR_MASK                                             0x7fffffff
#define RX_FCS_ERROR_CNTR_MASK                                       0x0000ffff
#define RX_ALIGN_ERR_CNTR_MASK                                       0x0000ffff
#define RX_LEN_ERR_CNTR_MASK                                         0x0000ffff
#define RX_UNDRSIZE_PKT_CNTR_MASK                                    0x0000ffff
#define RX_OVRSIZE_PKT_CNTR_MASK                                     0x0000ffff
#define RX_DROPPED_PKT_CNTR_MASK                                     0x0000ffff
#define TX_BYTE_CNTR_MASK                                            0x7fffffff
#define TX_PKT_CNTR_MASK                                             0x7fffffff
#define TX_DROP_FRAME_CNTR_MASK                                      0x0000ffff
#define TX_FCS_ERROR_CNTR_MASK                                       0x00000fff
#define TX_UNDSIZE_FRAME_CNTR_MASK                                   0x00000fff

#endif  /* __XGENE_ENET_CSR_H__ */
