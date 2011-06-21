/* Copyright 2009-2011 Freescale Semiconductor, Inc.
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

#ifndef PME2_REGS_H
#define PME2_REGS_H

#define PME_REG_ISR		0x000
#define PME_REG_IER		0x004
#define PME_REG_ISDR		0x008
#define PME_REG_IIR		0x00C
#define PME_REG_RLL		0x014
#define PME_REG_CDCR		0x018
#define PME_REG_TRUNCI		0x024
#define PME_REG_RBC		0x028
#define PME_REG_ESR		0x02C
#define PME_REG_ECR0		0x030
#define PME_REG_ECR1		0x034
#define PME_REG_EFQC		0x050
#define PME_REG_FACONF		0x060
#define PME_REG_PMSTAT		0x064
#define PME_REG_FAMCR		0x068
#define PME_REG_PMTR		0x06C
#define PME_REG_PEHD		0x074
#define PME_REG_BSC0		0x080
#define PME_REG_BSC1		0x084
#define PME_REG_BSC2		0x088
#define PME_REG_BSC3		0x08C
#define PME_REG_BSC4		0x090
#define PME_REG_BSC5		0x094
#define PME_REG_BSC6		0x098
#define PME_REG_BSC7		0x09C
#define PME_REG_QMBFD0		0x0E0
#define PME_REG_QMBFD1		0x0E4
#define PME_REG_QMBFD2		0x0E8
#define PME_REG_QMBFD3		0x0EC
#define PME_REG_QMBCTXTAH	0x0F0
#define PME_REG_QMBCTXTAL	0x0F4
#define PME_REG_QMBCTXTB	0x0F8
#define PME_REG_QMBCTL		0x0FC
#define PME_REG_ECC1BES		0x100
#define PME_REG_ECC2BES		0x104
#define PME_REG_ECCADDR		0x110
#define PME_REG_ECCCODE		0x118
#define PME_REG_TBT0ECC1TH	0x180
#define PME_REG_TBT0ECC1EC	0x184
#define PME_REG_TBT1ECC1TH	0x188
#define PME_REG_TBT1ECC1EC	0x18C
#define PME_REG_VLT0ECC1TH	0x190
#define PME_REG_VLT0ECC1EC	0x194
#define PME_REG_VLT1ECC1TH	0x198
#define PME_REG_VLT1ECC1EC	0x19C
#define PME_REG_CMECC1TH	0x1A0
#define PME_REG_CMECC1EC	0x1A4
#define PME_REG_DXCMECC1TH	0x1B0
#define PME_REG_DXCMECC1EC	0x1B4
#define PME_REG_DXEMECC1TH	0x1C0
#define PME_REG_DXEMECC1EC	0x1C4
#define PME_REG_STNIB		0x200
#define PME_REG_STNIS		0x204
#define PME_REG_STNTH1		0x208
#define PME_REG_STNTH2		0x20C
#define PME_REG_STNTHV		0x210
#define PME_REG_STNTHS		0x214
#define PME_REG_STNCH		0x218
#define PME_REG_SWDB		0x21C
#define PME_REG_KVLTS		0x220
#define PME_REG_KEC		0x224
#define PME_REG_STNPM		0x280
#define PME_REG_STNS1M		0x284
#define PME_REG_DRCIC		0x288
#define PME_REG_DRCMC		0x28C
#define PME_REG_STNPMR		0x290
#define PME_REG_PDSRBAH		0x2A0
#define PME_REG_PDSRBAL		0x2A4
#define PME_REG_DMCR		0x2A8
#define PME_REG_DEC0		0x2AC
#define PME_REG_DEC1		0x2B0
#define PME_REG_DLC		0x2C0
#define PME_REG_STNDSR		0x300
#define PME_REG_STNESR		0x304
#define PME_REG_STNS1R		0x308
#define PME_REG_STNOB		0x30C
#define PME_REG_SCBARH		0x310
#define PME_REG_SCBARL		0x314
#define PME_REG_SMCR		0x318
#define PME_REG_SREC		0x320
#define PME_REG_ESRP		0x328
#define PME_REG_SRRV0		0x338
#define PME_REG_SRRV1		0x33C
#define PME_REG_SRRV2		0x340
#define PME_REG_SRRV3		0x344
#define PME_REG_SRRV4		0x348
#define PME_REG_SRRV5		0x34C
#define PME_REG_SRRV6		0x350
#define PME_REG_SRRV7		0x354
#define PME_REG_SRRFI		0x358
#define PME_REG_SRRI		0x360
#define PME_REG_SRRR		0x364
#define PME_REG_SRRWC		0x368
#define PME_REG_SFRCC		0x36C
#define PME_REG_SEC1		0x370
#define PME_REG_SEC2		0x374
#define PME_REG_SEC3		0x378
#define PME_REG_MIA_BYC		0x380
#define PME_REG_MIA_BLC		0x384
#define PME_REG_MIA_CE		0x388
#define PME_REG_MIA_CR		0x390
#define PME_REG_PPIDMR0		0x800
#define PME_REG_PPIDMR1		0x804
#define PME_REG_PPIDMR2		0x808
#define PME_REG_PPIDMR3		0x80C
#define PME_REG_PPIDMR4		0x810
#define PME_REG_PPIDMR5		0x814
#define PME_REG_PPIDMR6		0x818
#define PME_REG_PPIDMR7		0x81C
#define PME_REG_PPIDMR8		0x820
#define PME_REG_PPIDMR9		0x824
#define PME_REG_PPIDMR10	0x828
#define PME_REG_PPIDMR11	0x82C
#define PME_REG_PPIDMR12	0x830
#define PME_REG_PPIDMR13	0x834
#define PME_REG_PPIDMR14	0x838
#define PME_REG_PPIDMR15	0x83C
#define PME_REG_PPIDMR16	0x840
#define PME_REG_PPIDMR17	0x844
#define PME_REG_PPIDMR18	0x848
#define PME_REG_PPIDMR19	0x84C
#define PME_REG_PPIDMR20	0x850
#define PME_REG_PPIDMR21	0x854
#define PME_REG_PPIDMR22	0x858
#define PME_REG_PPIDMR23	0x85C
#define PME_REG_PPIDMR24	0x860
#define PME_REG_PPIDMR25	0x864
#define PME_REG_PPIDMR26	0x868
#define PME_REG_PPIDMR27	0x86C
#define PME_REG_PPIDMR28	0x870
#define PME_REG_PPIDMR29	0x874
#define PME_REG_PPIDMR30	0x878
#define PME_REG_PPIDMR31	0x87C
#define PME_REG_SRCIDR		0xA00
#define PME_REG_LIODNR		0xA0C
#define PME_REG_PM_IP_REV1	0xBF8
#define PME_REG_PM_IP_REV2	0xBFC

#endif /* REGS_H */
