/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

#include "odm_precomp.h"

/*---------------------------Define Local Constant---------------------------*/
// 2010/04/25 MH Define the max tx power tracking tx agc power.
#define		ODM_TXPWRTRACK_MAX_IDX8723B	6

// MACRO definition for pRFCalibrateInfo->TxIQC_8723B[0]
#define		PATH_S0			1 // RF_PATH_B
#define		IDX_0xC94		0
#define		IDX_0xC80		1
#define		IDX_0xC4C		2
#define		IDX_0xC14		0
#define		IDX_0xCA0		1
#define		KEY			0
#define		VAL			1

// MACRO definition for pRFCalibrateInfo->TxIQC_8723B[1]
#define		PATH_S1			0 // RF_PATH_A
#define		IDX_0xC9C		0
#define		IDX_0xC88		1
#define		IDX_0xC4C		2
#define		IDX_0xC1C		0
#define		IDX_0xC78		1

/*---------------------------Define Local Constant---------------------------*/

//3============================================================
//3 Tx Power Tracking
//3============================================================

static void setIqkMatrix_8723B(PDM_ODM_T pDM_Odm, u1Byte OFDM_index,
			       u1Byte RFPath, s4Byte IqkResult_X, s4Byte IqkResult_Y)
{
	s4Byte ele_A = 0, ele_D, ele_C = 0, value32;

	if (OFDM_index >= OFDM_TABLE_SIZE)
		OFDM_index = OFDM_TABLE_SIZE-1;

	ele_D = (OFDMSwingTable_New[OFDM_index] & 0xFFC00000) >> 22;

	//new element A = element D x X
	if((IqkResult_X != 0) && (*(pDM_Odm->pBandType) == ODM_BAND_2_4G)) {
		if ((IqkResult_X & 0x00000200) != 0)	//consider minus
			IqkResult_X = IqkResult_X | 0xFFFFFC00;
		ele_A = ((IqkResult_X * ele_D)>>8)&0x000003FF;

		//new element C = element D x Y
		if ((IqkResult_Y & 0x00000200) != 0)
			IqkResult_Y = IqkResult_Y | 0xFFFFFC00;
		ele_C = ((IqkResult_Y * ele_D)>>8)&0x000003FF;

		//if (RFPath == ODM_RF_PATH_A)
		switch (RFPath) {
		case ODM_RF_PATH_A:
			/*
			 * write new elements A, C, D to regC80 and regC94,
			 * element B is always 0
			 */
			value32 = (ele_D<<22)|((ele_C&0x3F)<<16)|ele_A;
			ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance,
				     bMaskDWord, value32);

			value32 = (ele_C&0x000003C0) >> 6;
			ODM_SetBBReg(pDM_Odm, rOFDM0_XCTxAFE,
				     bMaskH4Bits, value32);

			value32 = ((IqkResult_X * ele_D)>>7)&0x01;
			ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
				     BIT24, value32);
			break;
		case ODM_RF_PATH_B:
			/*
			 * write new elements A, C, D to regC88 and regC9C,
			 * element B is always 0
			 */
			value32 = (ele_D<<22)|((ele_C&0x3F)<<16) |ele_A;
			ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance,
				     bMaskDWord, value32);

			value32 = (ele_C&0x000003C0) >> 6;
			ODM_SetBBReg(pDM_Odm, rOFDM0_XDTxAFE,
				     bMaskH4Bits, value32);

			value32 = ((IqkResult_X * ele_D)>>7)&0x01;
			ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
				     BIT28, value32);

			break;
		default:
			break;
		}
	} else {
		switch (RFPath) {
		case ODM_RF_PATH_A:
			ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance,
				     bMaskDWord,
				     OFDMSwingTable_New[OFDM_index]);
			ODM_SetBBReg(pDM_Odm, rOFDM0_XCTxAFE,
				     bMaskH4Bits, 0x00);
			ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
				     BIT24, 0x00);
			break;

		case ODM_RF_PATH_B:
			ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance,
				     bMaskDWord,
				     OFDMSwingTable_New[OFDM_index]);
			ODM_SetBBReg(pDM_Odm, rOFDM0_XDTxAFE,
				     bMaskH4Bits, 0x00);
			ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
				     BIT28, 0x00);
			break;

		default:
			break;
		}
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
		     ("TxPwrTracking path B: X = 0x%x, Y = 0x%x ele_A = 0x%x ele_C = 0x%x ele_D = 0x%x 0xeb4 = 0x%x 0xebc = 0x%x\n",
		      (u4Byte)IqkResult_X, (u4Byte)IqkResult_Y, (u4Byte)ele_A,
		      (u4Byte)ele_C, (u4Byte)ele_D,
		      (u4Byte)IqkResult_X, (u4Byte)IqkResult_Y));
}

static VOID
setCCKFilterCoefficient(PDM_ODM_T pDM_Odm, u1Byte CCKSwingIndex)
{
	if (!pDM_Odm->RFCalibrateInfo.bCCKinCH14) {
		ODM_Write1Byte(pDM_Odm, 0xa22,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][0]);
		ODM_Write1Byte(pDM_Odm, 0xa23,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][1]);
		ODM_Write1Byte(pDM_Odm, 0xa24,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][2]);
		ODM_Write1Byte(pDM_Odm, 0xa25,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][3]);
		ODM_Write1Byte(pDM_Odm, 0xa26,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][4]);
		ODM_Write1Byte(pDM_Odm, 0xa27,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][5]);
		ODM_Write1Byte(pDM_Odm, 0xa28,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][6]);
		ODM_Write1Byte(pDM_Odm, 0xa29,
			       CCKSwingTable_Ch1_Ch13_New[CCKSwingIndex][7]);
	} else {
		ODM_Write1Byte(pDM_Odm, 0xa22,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][0]);
		ODM_Write1Byte(pDM_Odm, 0xa23,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][1]);
		ODM_Write1Byte(pDM_Odm, 0xa24,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][2]);
		ODM_Write1Byte(pDM_Odm, 0xa25,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][3]);
		ODM_Write1Byte(pDM_Odm, 0xa26,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][4]);
		ODM_Write1Byte(pDM_Odm, 0xa27,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][5]);
		ODM_Write1Byte(pDM_Odm, 0xa28,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][6]);
		ODM_Write1Byte(pDM_Odm, 0xa29,
			       CCKSwingTable_Ch14_New[CCKSwingIndex][7]);
	}
}

void DoIQK_8723B(
	PDM_ODM_T	pDM_Odm,
	u1Byte		DeltaThermalIndex,
	u1Byte		ThermalValue,
	u1Byte		Threshold
	)
{
}

/*-----------------------------------------------------------------------------
 * Function:	odm_TxPwrTrackSetPwr88E()
 *
 * Overview:	88E change all channel tx power accordign to flag.
 *				OFDM & CCK are all different.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who	Remark
 *	04/23/2012	MHC	Create Version 0.
 *
 *---------------------------------------------------------------------------*/
VOID
ODM_TxPwrTrackSetPwr_8723B(PDM_ODM_T pDM_Odm, PWRTRACK_METHOD Method,
			   u1Byte RFPath, u1Byte ChannelMappedIndex)
{
	PADAPTER	Adapter = pDM_Odm->Adapter;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	u1Byte		PwrTrackingLimit_OFDM = 34; //+0dB
	u1Byte		PwrTrackingLimit_CCK= 28;	//-2dB
	u1Byte		TxRate = 0xFF;
	u1Byte		Final_OFDM_Swing_Index = 0;
	u1Byte		Final_CCK_Swing_Index = 0;
	u1Byte		i = 0;
	u2Byte rate = *(pDM_Odm->pForcedDataRate);

	if (!rate) { //auto rate
		if (pDM_Odm->TxRate != 0xFF)
			TxRate = HwRateToMRate(pDM_Odm->TxRate);
	} else { //force rate
		TxRate = (u1Byte)rate;
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
		     ("===>ODM_TxPwrTrackSetPwr8723B\n"));

	if (TxRate != 0xFF) {
		//2 CCK
		if((TxRate >= MGN_1M)&&(TxRate <= MGN_11M))
			PwrTrackingLimit_CCK = 28;	//-2dB
		//2 OFDM
		else if((TxRate >= MGN_6M)&&(TxRate <= MGN_48M))
			PwrTrackingLimit_OFDM= 36; //+3dB
		else if(TxRate == MGN_54M)
			PwrTrackingLimit_OFDM= 34; //+2dB

		//2 HT
		else if((TxRate >= MGN_MCS0)&&(TxRate <= MGN_MCS2)) //QPSK/BPSK
			PwrTrackingLimit_OFDM= 38; //+4dB
		else if((TxRate >= MGN_MCS3)&&(TxRate <= MGN_MCS4)) //16QAM
			PwrTrackingLimit_OFDM= 36; //+3dB
		else if((TxRate >= MGN_MCS5)&&(TxRate <= MGN_MCS7)) //64QAM
			PwrTrackingLimit_OFDM= 34; //+2dB
		else
			PwrTrackingLimit_OFDM =  pDM_Odm->DefaultOfdmIndex;   //Default OFDM index = 30
	}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
		     ("TxRate=0x%x, PwrTrackingLimit=%d\n",
		      TxRate, PwrTrackingLimit_OFDM));

	if (Method == TXAGC) {
		u1Byte	rf = 0;
		u4Byte	pwr = 0, TxAGC = 0;
		PADAPTER Adapter = pDM_Odm->Adapter;

		ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
			     ("odm_TxPwrTrackSetPwr8723B CH=%d\n",
			      *(pDM_Odm->pChannel)));

		pDM_Odm->Remnant_OFDMSwingIdx[RFPath] =
			pDM_Odm->Absolute_OFDMSwingIdx[RFPath];

#if (MP_DRIVER == 1)
		if (*(pDM_Odm->mp_mode) == 1) {
			pwr = PHY_QueryBBReg(Adapter, rTxAGC_A_Rate18_06, 0xFF);
			pwr += pDM_Odm->RFCalibrateInfo.PowerIndexOffset[RFPath];
			PHY_SetBBReg(Adapter, rTxAGC_A_CCK1_Mcs32,
				     bMaskByte1, pwr);
			TxAGC = (pwr << 16) | (pwr << 8) | pwr;
			PHY_SetBBReg(Adapter, rTxAGC_B_CCK11_A_CCK2_11,
				     bMaskH3Bytes, TxAGC);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				     ("ODM_TxPwrTrackSetPwr8723B: CCK Tx-rf(A) Power = 0x%x\n", TxAGC));

			pwr = PHY_QueryBBReg(Adapter, rTxAGC_A_Rate18_06, 0xFF);
			pwr += (pDM_Odm->BbSwingIdxOfdm[RFPath] -
				pDM_Odm->BbSwingIdxOfdmBase[RFPath]);
			TxAGC |= ((pwr << 24) | (pwr << 16) | (pwr << 8) | pwr);
			PHY_SetBBReg(Adapter, rTxAGC_A_Rate18_06,
				     bMaskDWord, TxAGC);
			PHY_SetBBReg(Adapter, rTxAGC_A_Rate54_24,
				     bMaskDWord, TxAGC);
			PHY_SetBBReg(Adapter, rTxAGC_A_Mcs03_Mcs00,
				     bMaskDWord, TxAGC);
			PHY_SetBBReg(Adapter, rTxAGC_A_Mcs07_Mcs04,
				     bMaskDWord, TxAGC);
			PHY_SetBBReg(Adapter, rTxAGC_A_Mcs11_Mcs08,
				     bMaskDWord, TxAGC);
			PHY_SetBBReg(Adapter, rTxAGC_A_Mcs15_Mcs12,
				     bMaskDWord, TxAGC);
			ODM_RT_TRACE(pDM_Odm, ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				     ("ODM_TxPwrTrackSetPwr8723B: OFDM Tx-rf(A) Power = 0x%x\n", TxAGC));
		} else
#endif
		{
		        pDM_Odm->Modify_TxAGC_Flag_PathA = TRUE;
		        pDM_Odm->Modify_TxAGC_Flag_PathA_CCK = TRUE;

		        PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, CCK );
		        PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, OFDM );
		        PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, HT_MCS0_MCS7 );
		}
	} else if (Method == BBSWING) {
		Final_OFDM_Swing_Index = pDM_Odm->DefaultOfdmIndex +
			pDM_Odm->Absolute_OFDMSwingIdx[RFPath];
		Final_CCK_Swing_Index = pDM_Odm->DefaultCckIndex +
			pDM_Odm->Absolute_OFDMSwingIdx[RFPath];

		// Adjust BB swing by OFDM IQ matrix
		if (Final_OFDM_Swing_Index >= PwrTrackingLimit_OFDM)
			Final_OFDM_Swing_Index = PwrTrackingLimit_OFDM;
		else if (Final_OFDM_Swing_Index <= 0)
			Final_OFDM_Swing_Index = 0;

		if (Final_CCK_Swing_Index >= CCK_TABLE_SIZE)
			Final_CCK_Swing_Index = CCK_TABLE_SIZE-1;
		else if (pDM_Odm->BbSwingIdxCck <= 0)
			Final_CCK_Swing_Index = 0;

		setIqkMatrix_8723B(pDM_Odm, Final_OFDM_Swing_Index, RFPath,
			pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][0],
			pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][1]);

		setCCKFilterCoefficient(pDM_Odm, Final_CCK_Swing_Index);
	} else if (Method == MIX_MODE) {
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
			("pDM_Odm->DefaultOfdmIndex=%d,  pDM_Odm->DefaultCCKIndex=%d, pDM_Odm->Absolute_OFDMSwingIdx[RFPath]=%d, RF_Path = %d\n",
			 pDM_Odm->DefaultOfdmIndex, pDM_Odm->DefaultCckIndex,
			 pDM_Odm->Absolute_OFDMSwingIdx[RFPath], RFPath));

		Final_OFDM_Swing_Index = pDM_Odm->DefaultOfdmIndex +
			pDM_Odm->Absolute_OFDMSwingIdx[RFPath];
		Final_CCK_Swing_Index = pDM_Odm->DefaultCckIndex +
			pDM_Odm->Absolute_OFDMSwingIdx[RFPath];

		//BBSwing higher then Limit
		if (Final_OFDM_Swing_Index > PwrTrackingLimit_OFDM) {
			pDM_Odm->Remnant_OFDMSwingIdx[RFPath] =
				Final_OFDM_Swing_Index - PwrTrackingLimit_OFDM;

			setIqkMatrix_8723B(pDM_Odm, PwrTrackingLimit_OFDM,
					   RFPath,
					   pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][0],
					   pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][1]);

			pDM_Odm->Modify_TxAGC_Flag_PathA = TRUE;
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath,
							 pHalData->CurrentChannel, OFDM);
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath,
							 pHalData->CurrentChannel, HT_MCS0_MCS7);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				("******Path_A Over BBSwing Limit , PwrTrackingLimit = %d , Remnant TxAGC Value = %d \n",
				PwrTrackingLimit_OFDM,
				 pDM_Odm->Remnant_OFDMSwingIdx[RFPath]));
		} else if (Final_OFDM_Swing_Index <= 0) {
			pDM_Odm->Remnant_OFDMSwingIdx[RFPath] =
				Final_OFDM_Swing_Index;

			setIqkMatrix_8723B(pDM_Odm, 0, RFPath,
				pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][0],
				pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][1]);

			pDM_Odm->Modify_TxAGC_Flag_PathA = TRUE;
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath,
							 pHalData->CurrentChannel, OFDM);
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath,
							 pHalData->CurrentChannel, HT_MCS0_MCS7);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				     ("******Path_A Lower then BBSwing lower bound  0 , Remnant TxAGC Value = %d \n",
				      pDM_Odm->Remnant_OFDMSwingIdx[RFPath]));
		} else {
			setIqkMatrix_8723B(pDM_Odm, Final_OFDM_Swing_Index,
					   RFPath,
					   pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][0],
					   pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[ChannelMappedIndex].Value[0][1]);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
				     ("******Path_A Compensate with BBSwing , Final_OFDM_Swing_Index = %d \n",
				      Final_OFDM_Swing_Index));

			//If TxAGC has changed, reset TxAGC again
			if (pDM_Odm->Modify_TxAGC_Flag_PathA) {
				pDM_Odm->Remnant_OFDMSwingIdx[RFPath] = 0;
				PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, OFDM);
				PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, HT_MCS0_MCS7);
				pDM_Odm->Modify_TxAGC_Flag_PathA = FALSE;

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
					     ("******Path_A pDM_Odm->Modify_TxAGC_Flag = FALSE \n"));
			}
		}

		if (Final_CCK_Swing_Index > PwrTrackingLimit_CCK) {
			pDM_Odm->Remnant_CCKSwingIdx = Final_CCK_Swing_Index -
				PwrTrackingLimit_CCK;
			setCCKFilterCoefficient(pDM_Odm, PwrTrackingLimit_CCK);
			pDM_Odm->Modify_TxAGC_Flag_PathA_CCK = TRUE;
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, CCK );

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				     ("******Path_A CCK Over Limit , PwrTrackingLimit_CCK = %d , pDM_Odm->Remnant_CCKSwingIdx  = %d \n",
				      PwrTrackingLimit_CCK,
				      pDM_Odm->Remnant_CCKSwingIdx));
		} else if(Final_CCK_Swing_Index <= 0) {  // Lowest CCK Index = 0
			pDM_Odm->Remnant_CCKSwingIdx = Final_CCK_Swing_Index;
			setCCKFilterCoefficient(pDM_Odm, 0);
			pDM_Odm->Modify_TxAGC_Flag_PathA_CCK = TRUE;
			PHY_SetTxPowerIndexByRateSection(Adapter, RFPath,
							 pHalData->CurrentChannel, CCK);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK, ODM_DBG_LOUD,
				     ("******Path_A CCK Under Limit , PwrTrackingLimit_CCK = %d , pDM_Odm->Remnant_CCKSwingIdx  = %d \n",
				      0, pDM_Odm->Remnant_CCKSwingIdx));
		} else {
			setCCKFilterCoefficient(pDM_Odm, Final_CCK_Swing_Index);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK,
				     ODM_DBG_LOUD,
				     ("******Path_A CCK Compensate with BBSwing , Final_CCK_Swing_Index = %d \n",
				      Final_CCK_Swing_Index));

			//If TxAGC has changed, reset TxAGC again
			if (pDM_Odm->Modify_TxAGC_Flag_PathA_CCK) {
				pDM_Odm->Remnant_CCKSwingIdx = 0;
				PHY_SetTxPowerIndexByRateSection(Adapter, RFPath, pHalData->CurrentChannel, CCK );
				pDM_Odm->Modify_TxAGC_Flag_PathA_CCK= FALSE;

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_TX_PWR_TRACK,
					     ODM_DBG_LOUD,
					     ("******Path_A pDM_Odm->Modify_TxAGC_Flag_CCK = FALSE \n"));
			}
		}
	} else {
		return; // This method is not supported.
	}
}

static VOID
GetDeltaSwingTable_8723B(IN PDM_ODM_T pDM_Odm,
			 OUT pu1Byte *TemperatureUP_A,
			 OUT pu1Byte *TemperatureDOWN_A,
			 OUT pu1Byte *TemperatureUP_B,
			 OUT pu1Byte *TemperatureDOWN_B)
{
	PADAPTER	Adapter		 = pDM_Odm->Adapter;
	PODM_RF_CAL_T	RFC = &(pDM_Odm->RFCalibrateInfo);
	HAL_DATA_TYPE	*pHalData	 = GET_HAL_DATA(Adapter);
	u2Byte		rate		 = *(pDM_Odm->pForcedDataRate);
	u1Byte		channel		 = pHalData->CurrentChannel;

	if (1 <= channel && channel <= 14) {
		if (IS_CCK_RATE(rate)) {
			*TemperatureUP_A   = RFC->DeltaSwingTableIdx_2GCCKA_P;
			*TemperatureDOWN_A = RFC->DeltaSwingTableIdx_2GCCKA_N;
			*TemperatureUP_B   = RFC->DeltaSwingTableIdx_2GCCKB_P;
			*TemperatureDOWN_B = RFC->DeltaSwingTableIdx_2GCCKB_N;
		} else {
			*TemperatureUP_A   = RFC->DeltaSwingTableIdx_2GA_P;
			*TemperatureDOWN_A = RFC->DeltaSwingTableIdx_2GA_N;
			*TemperatureUP_B   = RFC->DeltaSwingTableIdx_2GB_P;
			*TemperatureDOWN_B = RFC->DeltaSwingTableIdx_2GB_N;
		}
	}/*else if ( 36 <= channel && channel <= 64) {
		*TemperatureUP_A   = RFC->DeltaSwingTableIdx_5GA_P[0];
		*TemperatureDOWN_A = RFC->DeltaSwingTableIdx_5GA_N[0];
		*TemperatureUP_B   = RFC->DeltaSwingTableIdx_5GB_P[0];
		*TemperatureDOWN_B = RFC->DeltaSwingTableIdx_5GB_N[0];
	} else if ( 100 <= channel && channel <= 140) {
		*TemperatureUP_A   = RFC->DeltaSwingTableIdx_5GA_P[1];
		*TemperatureDOWN_A = RFC->DeltaSwingTableIdx_5GA_N[1];
		*TemperatureUP_B   = RFC->DeltaSwingTableIdx_5GB_P[1];
		*TemperatureDOWN_B = RFC->DeltaSwingTableIdx_5GB_N[1];
	} else if ( 149 <= channel && channel <= 173) {
		*TemperatureUP_A   = RFC->DeltaSwingTableIdx_5GA_P[2];
		*TemperatureDOWN_A = RFC->DeltaSwingTableIdx_5GA_N[2];
		*TemperatureUP_B   = RFC->DeltaSwingTableIdx_5GB_P[2];
		*TemperatureDOWN_B = RFC->DeltaSwingTableIdx_5GB_N[2];
	}*/else {
		*TemperatureUP_A   = (pu1Byte)DeltaSwingTableIdx_2GA_P_8188E;
		*TemperatureDOWN_A = (pu1Byte)DeltaSwingTableIdx_2GA_N_8188E;
		*TemperatureUP_B   = (pu1Byte)DeltaSwingTableIdx_2GA_P_8188E;
		*TemperatureDOWN_B = (pu1Byte)DeltaSwingTableIdx_2GA_N_8188E;
	}

	return;
}

void ConfigureTxpowerTrack_8723B(PTXPWRTRACK_CFG pConfig)
{
	pConfig->SwingTableSize_CCK = CCK_TABLE_SIZE;
	pConfig->SwingTableSize_OFDM = OFDM_TABLE_SIZE;
	pConfig->Threshold_IQK = IQK_THRESHOLD;
	pConfig->AverageThermalNum = AVG_THERMAL_NUM_8723B;
	pConfig->RfPathCount = MAX_PATH_NUM_8723B;
	pConfig->ThermalRegAddr = RF_T_METER_8723B;

	pConfig->ODM_TxPwrTrackSetPwr = ODM_TxPwrTrackSetPwr_8723B;
	pConfig->DoIQK = DoIQK_8723B;
	pConfig->PHY_LCCalibrate = PHY_LCCalibrate_8723B;
	pConfig->GetDeltaSwingTable = GetDeltaSwingTable_8723B;
}

//1 7.	IQK
#define MAX_TOLERANCE		5
#define IQK_DELAY_TIME		1		//ms

static u1Byte			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
phy_PathA_IQK_8723B(IN PADAPTER pAdapter,
		    IN BOOLEAN configPathB, IN u1Byte RF_Path)
{
	u4Byte regEAC, regE94, regE9C, tmp, Path_SEL_BB /*, regEA4*/;
	u1Byte result = 0x00;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	// Save RF Path
	Path_SEL_BB = ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord);

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path A IQK!\n"));

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	//	enable path A PA in TXIQK mode
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x20000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0003f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xc7f87);
	//	disable path B PA in TXIQK mode
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, bRFRegOffsetMask, 0x00020 );
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x40ec1 );

	//1 Tx IQK
	//IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);
	//path-A IQK setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A IQK setting!\n"));
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
//	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x8214010a);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x821403ea);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28110000);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x00462911);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//Ant switch
	if (configPathB || (RF_Path == 0))
		// wifi switch to S1
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000000);
	else
		// wifi switch to S0
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path A LOK & IQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_8723B));
	//PlatformStallExecution(IQK_DELAY_TIME_8723B*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);


	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe94 = 0x%x, 0xe9c = 0x%x\n", regE94, regE9C));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe90(before IQK)= 0x%x, 0xe98(afer IQK) = 0x%x\n",
	ODM_GetBBReg(pDM_Odm, 0xe90, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xe98, bMaskDWord)));

	//Allen 20131125
	tmp= (regE9C & 0x03FF0000) >> 16;
	if ((tmp & 0x200)> 0)
		tmp = 0x400 - tmp;

	if(!(regEAC & BIT28) &&
	   (((regE94 & 0x03FF0000)>>16) != 0x142) &&
	   (((regE9C & 0x03FF0000)>>16) != 0x42) &&
	   (((regE94 & 0x03FF0000)>>16) <0x110) &&
	   (((regE94 & 0x03FF0000)>>16) >0xf0) &&
	   (tmp <0xf))
		result |= 0x01;
	else				//if Tx not OK, ignore Rx
		return result;
	return result;
}

static u1Byte			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
phy_PathA_RxIQK8723B(IN PADAPTER pAdapter,
		     IN BOOLEAN configPathB, IN u1Byte RF_Path)
{
	u4Byte regEAC, regE94, regE9C, regEA4, u4tmp,tmp, Path_SEL_BB;
	u1Byte result = 0x00;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	// Save RF Path
	Path_SEL_BB = ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path A RX IQK:Get TXIMR setting\n"));
	//1 Get TXIMR setting
	//modify RXIQK mode table
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A Rx IQK modify RXIQK mode table!\n"));
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x30000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0001f);
	//LNA2 off, PA on for Dcut
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xf7fb7);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x0);
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);

	//path-A IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);

//	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160c1f);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160ff0);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28110000);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a911);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//Ant switch
	if (configPathB || (RF_Path == 0))
		// wifi switch to S1
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000000);
	else
		// wifi switch to S0
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path A LOK & IQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_8723B));
	//PlatformStallExecution(IQK_DELAY_TIME_8723B*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe94 = 0x%x, 0xe9c = 0x%x\n", regE94, regE9C));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe90(before IQK)= 0x%x, 0xe98(afer IQK) = 0x%x\n",
		      ODM_GetBBReg(pDM_Odm, 0xe90, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xe98, bMaskDWord)));

	//Allen 20131125
	tmp=(regE9C & 0x03FF0000)>>16;
	if ((tmp & 0x200)> 0)
		tmp = 0x400 - tmp;

	if (!(regEAC & BIT28) &&
	    (((regE94 & 0x03FF0000)>>16) != 0x142) &&
	    (((regE9C & 0x03FF0000)>>16) != 0x42) &&
	    (((regE94 & 0x03FF0000)>>16) <0x110) &&
	    (((regE94 & 0x03FF0000)>>16) >0xf0) &&
	    (tmp <0xf))
		result |= 0x01;
	else			//if Tx not OK, ignore Rx
		return result;


	u4tmp = 0x80007C00 | (regE94&0x3FF0000) | ((regE9C & 0x3FF0000) >> 16);
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, u4tmp);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe40 = 0x%x u4tmp = 0x%x \n",
		      ODM_GetBBReg(pDM_Odm, rTx_IQK, bMaskDWord), u4tmp));

	//1 RX IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path A RX IQK\n"));

	//modify RXIQK mode table
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A Rx IQK modify RXIQK mode table 2!\n"));
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x30000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0001f);
	//LAN2 on, PA off for Dcut
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xf7d77);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x0);

	//PA, PAD setting
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xdf, bRFRegOffsetMask, 0xf80 );
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x55, bRFRegOffsetMask, 0x4021f );

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);

	//path-A IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);

	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82110000);
//	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x281604c2);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x2816001f);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a8d1);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//Ant switch
	if (configPathB || (RF_Path == 0))
		// wifi switch to S1
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000000);
	else
		// wifi switch to S0
		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path A LOK & IQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path A LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_88E));
	//PlatformStallExecution(IQK_DELAY_TIME_8723B*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regEA4= ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xea4 = 0x%x, 0xeac = 0x%x\n", regEA4, regEAC));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xea0(before IQK)= 0x%x, 0xea8(afer IQK) = 0x%x\n",
		      ODM_GetBBReg(pDM_Odm, 0xea0, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xea8, bMaskDWord)));

	//	PA/PAD controlled by 0x0
	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xdf, bRFRegOffsetMask, 0x780 );

	//Allen 20131125
	tmp=(regEAC & 0x03FF0000)>>16;
	if ((tmp & 0x200)> 0)
		tmp = 0x400 - tmp;

	if (!(regEAC & BIT27) &&	//if Tx is OK, check whether Rx is OK
	    (((regEA4 & 0x03FF0000)>>16) != 0x132) &&
	    (((regEAC & 0x03FF0000)>>16) != 0x36)&&
	    (((regEA4 & 0x03FF0000)>>16) < 0x110) &&
	    (((regEA4 & 0x03FF0000)>>16) > 0xf0) &&
	    (tmp <0xf))
		result |= 0x02;
	else				//if Tx not OK, ignore Rx
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("Path A Rx IQK fail!!\n"));


	return result;
}

static u1Byte				//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
phy_PathB_IQK_8723B(IN PADAPTER pAdapter)
{
	u4Byte regEAC, regE94, regE9C, tmp, Path_SEL_BB/*, regEC4, regECC, Path_SEL_BB*/;
	u1Byte	result = 0x00;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path B IQK!\n"));

	// Save RF Path
	Path_SEL_BB = ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	//	in TXIQK mode
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1 );
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask, 0x20000 );
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask, 0x0003f );
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask, 0xc7f87 );
	//	enable path B PA in TXIQK mode
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x20, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x40fc1);

	//1 Tx IQK
	//IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);
	//path-A IQK setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-B IQK setting!\n"));
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);

//	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82140114);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x821403ea);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28110000);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x00462911);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//switch to path B
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, bRFRegOffsetMask, 0xeffe0);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path B LOK & IQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path B LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path B LOK & IQK.\n", IQK_DELAY_TIME_88E));
	//PlatformStallExecution(IQK_DELAY_TIME_88E*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("0x948 = 0x%x\n", ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord)));

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe94 = 0x%x, 0xe9c = 0x%x\n", regE94, regE9C));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe90(before IQK)= 0x%x, 0xe98(afer IQK) = 0x%x\n",
		      ODM_GetBBReg(pDM_Odm, 0xe90, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xe98, bMaskDWord)));

	//Allen 20131125
	tmp = (regE9C & 0x03FF0000) >> 16;
	if ((tmp & 0x200)> 0)
		tmp = 0x400 - tmp;

	if (!(regEAC & BIT28) &&
	    (((regE94 & 0x03FF0000)>>16) != 0x142) &&
	    (((regE9C & 0x03FF0000)>>16) != 0x42)&&
	    (((regE94 & 0x03FF0000)>>16) <0x110) &&
	    (((regE94 & 0x03FF0000)>>16) >0xf0) &&
	    (tmp <0xf))
		result |= 0x01;
	else
		return result;

	return result;
}

static u1Byte			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
phy_PathB_RxIQK8723B(IN PADAPTER pAdapter, IN BOOLEAN configPathB)
{
	u4Byte regE94, regE9C, regEA4, regEAC, u4tmp, tmp, Path_SEL_BB;
	u1Byte result = 0x00;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	// Save RF Path
	Path_SEL_BB = ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord);
    //leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	//switch to path B
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);

	//1 Get TXIMR setting
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path B RX IQK:Get TXIMR setting!\n"));
	//modify RXIQK mode table
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Path-A Rx IQK modify RXIQK mode table!\n"));
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1 );
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x30000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0001f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xf7fb7);
	//open PA S1 & SMIXER
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x20, 0x1 );
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x60fcd );

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, 0x01007c00);
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);

	//path-B IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);

//	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160c1f );
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82160ff0);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x28110000);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a911);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//switch to path B
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, bRFRegOffsetMask, 0xeffe0);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path B TXIQK @ RXIQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path B LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_88E));
	//PlatformStallExecution(IQK_DELAY_TIME_88E*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regE94 = ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord);
	regE9C= ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe94 = 0x%x, 0xe9c = 0x%x\n", regE94, regE9C));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe90(before IQK)= 0x%x, 0xe98(afer IQK) = 0x%x\n",
		      ODM_GetBBReg(pDM_Odm, 0xe90, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xe98, bMaskDWord)));

	//Allen 20131125
	tmp = (regE9C & 0x03FF0000)>>16;
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("tmp1 = 0x%x\n", tmp));
	if ((tmp & 0x200) > 0)
		tmp = 0x400 - tmp;
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("tmp2 = 0x%x\n", tmp));

	if (!(regEAC & BIT28) &&
	    (((regE94 & 0x03FF0000)>>16) != 0x142) &&
	    (((regE9C & 0x03FF0000)>>16) != 0x42)  &&
	    (((regE94 & 0x03FF0000)>>16) <0x110) &&
	    (((regE94 & 0x03FF0000)>>16) >0xf0) &&
	    (tmp <0xf))
		result |= 0x01;
	else			//if Tx not OK, ignore Rx
		return result;

	u4tmp = 0x80007C00 | (regE94&0x3FF0000)  | ((regE9C&0x3FF0000) >> 16);
	ODM_SetBBReg(pDM_Odm, rTx_IQK, bMaskDWord, u4tmp);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xe40 = 0x%x u4tmp = 0x%x \n",
		      ODM_GetBBReg(pDM_Odm, rTx_IQK, bMaskDWord), u4tmp));


	//1 RX IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path B RX IQK\n"));

	//modify RXIQK mode table
	//<20121009, Kordan> RF Mode = 3
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x30000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0001f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xf7d77);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x0);

	//open PA S1 & close SMIXER
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x20, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x60ebd);

	//PA, PAD setting
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xdf, bRFRegOffsetMask, 0xf80);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x56, bRFRegOffsetMask, 0x51000);

	//IQK setting
	ODM_SetBBReg(pDM_Odm, rRx_IQK, bMaskDWord, 0x01004800);

	//path-B IQK setting
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x18008c1c);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord, 0x38008c1c);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord, 0x38008c1c);

	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_A, bMaskDWord, 0x82110000);
//	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x281604c2);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_A, bMaskDWord, 0x2816001f);
	ODM_SetBBReg(pDM_Odm, rTx_IQK_PI_B, bMaskDWord, 0x82110000);
	ODM_SetBBReg(pDM_Odm, rRx_IQK_PI_B, bMaskDWord, 0x28110000);

	//LO calibration setting
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("LO calibration setting!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Rsp, bMaskDWord, 0x0046a8d1);

	//enter IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);

	//switch to path B
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, 0x00000280);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, bRFRegOffsetMask, 0xeffe0);

	//GNT_BT = 0
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00000800);

	//One shot, path B LOK & IQK
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("One shot, path B LOK & IQK!\n"));
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf9000000);
	ODM_SetBBReg(pDM_Odm, rIQK_AGC_Pts, bMaskDWord, 0xf8000000);

	// delay x ms
//	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME_88E));
	//PlatformStallExecution(IQK_DELAY_TIME_88E*1000);
	ODM_delay_ms(IQK_DELAY_TIME_8723B);

	//restore Ant Path
	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord,Path_SEL_BB);
	//GNT_BT = 1
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, 0x00001800);

	//leave IQK mode
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);

	// Check failed
	regEAC = ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord);
	regEA4= ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord);;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xeac = 0x%x\n", regEAC));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xea4 = 0x%x, 0xeac = 0x%x\n", regEA4, regEAC));
	//monitor image power before & after IQK
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("0xea0(before IQK)= 0x%x, 0xea8(afer IQK) = 0x%x\n",
		ODM_GetBBReg(pDM_Odm, 0xea0, bMaskDWord),
		      ODM_GetBBReg(pDM_Odm, 0xea8, bMaskDWord)));

	//	PA/PAD controlled by 0x0
	//leave IQK mode

	//Allen 20131125
	tmp = (regEAC & 0x03FF0000) >> 16;
	if ((tmp & 0x200) > 0)
		tmp = 0x400 - tmp;

	if (!(regEAC & BIT27) && //if Tx is OK, check whether Rx is OK
	    (((regEA4 & 0x03FF0000)>>16) != 0x132) &&
	    (((regEAC & 0x03FF0000)>>16) != 0x36) &&
	    (((regEA4 & 0x03FF0000)>>16) <0x110) &&
	    (((regEA4 & 0x03FF0000)>>16) >0xf0) &&
	    (tmp <0xf))
		result |= 0x02;
	else
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("Path B Rx IQK fail!!\n"));

	return result;
}

static VOID
_PHY_PathAFillIQKMatrix8723B(IN PADAPTER pAdapter, IN BOOLEAN bIQKOK,
			     IN s4Byte result[][8], IN u1Byte final_candidate,
			     IN BOOLEAN bTxOnly)
{
	u4Byte	Oldval_0, X, TX0_A, reg;
	s4Byte	Y, TX0_C;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	PODM_RF_CAL_T	RFC = &(pDM_Odm->RFCalibrateInfo);

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path A IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed"));

	if (final_candidate == 0xFF) {
		return;
	} else if (bIQKOK) {
		Oldval_0 = (ODM_GetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance,
					 bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][0];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;
		TX0_A = (X * Oldval_0) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("X = 0x%x, TX0_A = 0x%x, Oldval_0 0x%x\n",
			      X, TX0_A, Oldval_0));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance, 0x3FF, TX0_A);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
			     BIT(31), ((X* Oldval_0 >> 7) & 0x1));

		Y = result[final_candidate][1];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;

		//2 Tx IQC
		TX0_C = (Y * Oldval_0) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("Y = 0x%x, TX = 0x%x\n", Y, TX0_C));
		ODM_SetBBReg(pDM_Odm, rOFDM0_XCTxAFE,
			     0xF0000000, ((TX0_C&0x3C0) >> 6));
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC94][KEY] = rOFDM0_XCTxAFE;
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC94][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XCTxAFE, bMaskDWord);

		ODM_SetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance,
			     0x003F0000, (TX0_C & 0x3F));
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC80][KEY] =
			rOFDM0_XATxIQImbalance;
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC80][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XATxIQImbalance,
				     bMaskDWord);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold,
			     BIT(29), ((Y * Oldval_0 >> 7) & 0x1));
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC4C][KEY] =
			rOFDM0_ECCAThreshold;
		RFC->TxIQC_8723B[PATH_S1][IDX_0xC4C][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, bMaskDWord);

		if (bTxOnly) {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("_PHY_PathAFillIQKMatrix8723B only Tx OK\n"));

			// <20130226, Kordan> Saving RxIQC, otherwise not initialized.
			RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][KEY] =
				rOFDM0_RxIQExtAnta;
			RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][VAL] = 0xfffffff &
				ODM_GetBBReg(pDM_Odm, rOFDM0_RxIQExtAnta,
					     bMaskDWord);
			RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][KEY] =
				rOFDM0_XARxIQImbalance;
//			RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][VAL] = ODM_GetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, bMaskDWord);
			RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][VAL] = 0x40000100;
			return;
		}

		reg = result[final_candidate][2];

		//2 Rx IQC
		ODM_SetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, 0x3FF, reg);
		reg = result[final_candidate][3] & 0x3F;
		ODM_SetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, 0xFC00, reg);
		RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][KEY] =
			rOFDM0_XARxIQImbalance;
		RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance,
				     bMaskDWord);

		reg = (result[final_candidate][3] >> 6) & 0xF;
		ODM_SetBBReg(pDM_Odm, rOFDM0_RxIQExtAnta, 0xF0000000, reg);
		RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][KEY] = rOFDM0_RxIQExtAnta;
		RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_RxIQExtAnta, bMaskDWord);
	}
}

static VOID
_PHY_PathBFillIQKMatrix8723B(IN PADAPTER pAdapter, IN BOOLEAN bIQKOK,
			     IN s4Byte result[][8], IN u1Byte final_candidate,
			     IN BOOLEAN bTxOnly)
{
	u4Byte	Oldval_1, X, TX1_A, reg;
	s4Byte	Y, TX1_C;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	PODM_RF_CAL_T	RFC = &pDM_Odm->RFCalibrateInfo;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path B IQ Calibration %s !\n",
		      (bIQKOK) ? "Success" : "Failed"));

	if (final_candidate == 0xFF) {
		return;
	} else if (bIQKOK) {
		Oldval_1 = (ODM_GetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance,
					 bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][4];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;
		TX1_A = (X * Oldval_1) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("X = 0x%x, TX1_A = 0x%x\n", X, TX1_A));

		ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance, 0x3FF, TX1_A);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(27),
			     ((X* Oldval_1>>7) & 0x1));

		Y = result[final_candidate][5];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;

		TX1_C = (Y * Oldval_1) >> 8;
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("Y = 0x%x, TX1_C = 0x%x\n", Y, TX1_C));

		//2 Tx IQC
		ODM_SetBBReg(pDM_Odm, rOFDM0_XDTxAFE, 0xF0000000, ((TX1_C&0x3C0)>>6));
//		RFC->TxIQC_8723B[PATH_S0][IDX_0xC9C][KEY] = rOFDM0_XDTxAFE;
//		RFC->TxIQC_8723B[PATH_S0][IDX_0xC9C][VAL] = ODM_GetBBReg(pDM_Odm, rOFDM0_XDTxAFE, bMaskDWord);
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC94][KEY] = rOFDM0_XCTxAFE;
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC94][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XDTxAFE, bMaskDWord);

		ODM_SetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance,
			     0x003F0000, (TX1_C&0x3F));
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC80][KEY] =
			rOFDM0_XATxIQImbalance;
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC80][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XBTxIQImbalance,
				     bMaskDWord);

		ODM_SetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, BIT(25),
			     ((Y* Oldval_1>>7) & 0x1));
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC4C][KEY] =
			rOFDM0_ECCAThreshold;
		RFC->TxIQC_8723B[PATH_S0][IDX_0xC4C][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_ECCAThreshold, bMaskDWord);

		if (bTxOnly) {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("_PHY_PathBFillIQKMatrix8723B only Tx OK\n"));

			RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][KEY] =
				rOFDM0_XARxIQImbalance;
//			RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][VAL] = ODM_GetBBReg(pDM_Odm, rOFDM0_XARxIQImbalance, bMaskDWord);
			RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][VAL] = 0x40000100;
			RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][KEY] =
				rOFDM0_RxIQExtAnta;
			RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][VAL] = 0x0fffffff &
				ODM_GetBBReg(pDM_Odm, rOFDM0_RxIQExtAnta,
					     bMaskDWord);
			return;
		}

		//2 Rx IQC
		reg = result[final_candidate][6];
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBRxIQImbalance, 0x3FF, reg);
		reg = result[final_candidate][7] & 0x3F;
		ODM_SetBBReg(pDM_Odm, rOFDM0_XBRxIQImbalance, 0xFC00, reg);
		RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][KEY] =
			rOFDM0_XARxIQImbalance;
		RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][VAL] =
			ODM_GetBBReg(pDM_Odm, rOFDM0_XBRxIQImbalance,
				     bMaskDWord);

		reg = (result[final_candidate][7] >> 6) & 0xF;
//		ODM_SetBBReg(pDM_Odm, rOFDM0_AGCRSSITable, 0x0000F000, reg);
		RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][KEY] = rOFDM0_RxIQExtAnta;
		RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][VAL] = (reg << 28) |
			(ODM_GetBBReg(pDM_Odm,rOFDM0_RxIQExtAnta,
				      bMaskDWord) & 0x0fffffff);
	}
}

//
// 2011/07/26 MH Add an API for testing IQK fail case.
//
// MP Already declare in odm.c

VOID
ODM_SetIQCbyRFpath(IN PDM_ODM_T pDM_Odm, IN u4Byte RFpath)
{
	PODM_RF_CAL_T	RFC = &pDM_Odm->RFCalibrateInfo;

	if ((RFC->TxIQC_8723B[PATH_S0][IDX_0xC80][VAL] != 0x0) &&
	    (RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][VAL] != 0x0) &&
	    (RFC->TxIQC_8723B[PATH_S1][IDX_0xC80][VAL] != 0x0) &&
	    (RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][VAL] != 0x0)) {
		if (RFpath) { //S1: RFpath = 0, S0:RFpath = 1
			//S0 TX IQC
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC94][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC94][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC80][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC80][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC4C][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S0][IDX_0xC4C][VAL]);
			//S0 RX IQC
			ODM_SetBBReg(pDM_Odm,
				     RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][KEY],
				     bMaskDWord,
				     RFC->RxIQC_8723B[PATH_S0][IDX_0xC14][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][KEY],
				     bMaskDWord,
				     RFC->RxIQC_8723B[PATH_S0][IDX_0xCA0][VAL]);
		} else {
			//S1 TX IQC
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC94][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC94][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC80][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC80][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC4C][KEY],
				     bMaskDWord,
				     RFC->TxIQC_8723B[PATH_S1][IDX_0xC4C][VAL]);
			//S1 RX IQC
			ODM_SetBBReg(pDM_Odm,
				     RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][KEY],
				     bMaskDWord,
				     RFC->RxIQC_8723B[PATH_S1][IDX_0xC14][VAL]);
			ODM_SetBBReg(pDM_Odm,
				     RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][KEY],
				     bMaskDWord,
				     RFC->RxIQC_8723B[PATH_S1][IDX_0xCA0][VAL]);
		}
	}
}

static BOOLEAN
ODM_CheckPowerStatus(
	IN	PADAPTER		Adapter)
{
/*
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T			pDM_Odm = &pHalData->DM_OutSrc;
	RT_RF_POWER_STATE	rtState;
	PMGNT_INFO			pMgntInfo	= &(Adapter->MgntInfo);

	// 2011/07/27 MH We are not testing ready~~!! We may fail to get correct value when init sequence.
	if (pMgntInfo->init_adpt_in_progress == TRUE)
	{
		ODM_RT_TRACE(pDM_Odm,COMP_INIT, DBG_LOUD, ("ODM_CheckPowerStatus Return TRUE, due to initadapter"));
		return	TRUE;
	}

	//
	//	2011/07/19 MH We can not execute tx pwoer tracking/ LLC calibrate or IQK.
	//
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));
	if(Adapter->bDriverStopped || Adapter->bDriverIsGoingToPnpSetPowerSleep || rtState == eRfOff)
	{
		ODM_RT_TRACE(pDM_Odm,COMP_INIT, DBG_LOUD, ("ODM_CheckPowerStatus Return FALSE, due to %d/%d/%d\n",
		Adapter->bDriverStopped, Adapter->bDriverIsGoingToPnpSetPowerSleep, rtState));
		return	FALSE;
	}
*/
	return	TRUE;
}

static VOID
_PHY_SaveADDARegisters8723B(IN PADAPTER pAdapter, IN pu4Byte ADDAReg,
			    IN pu4Byte ADDABackup, IN u4Byte RegisterNum)
{
	u4Byte	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	if (ODM_CheckPowerStatus(pAdapter) == FALSE)
		return;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Save ADDA parameters.\n"));
	for (i = 0 ; i < RegisterNum ; i++) {
		ADDABackup[i] = ODM_GetBBReg(pDM_Odm, ADDAReg[i], bMaskDWord);
	}
}

static VOID
_PHY_SaveMACRegisters8723B(IN PADAPTER pAdapter,
			   IN pu4Byte MACReg, IN pu4Byte MACBackup)
{
	u4Byte	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Save MAC parameters.\n"));
	for (i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++) {
		MACBackup[i] = ODM_Read1Byte(pDM_Odm, MACReg[i]);
	}
	MACBackup[i] = ODM_Read4Byte(pDM_Odm, MACReg[i]);
}

static VOID
_PHY_ReloadADDARegisters8723B(IN PADAPTER pAdapter, IN pu4Byte ADDAReg,
			      IN pu4Byte ADDABackup, IN u4Byte RegiesterNum)
{
	u4Byte	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("Reload ADDA power saving parameters !\n"));
	for(i = 0 ; i < RegiesterNum; i++)
	{
		ODM_SetBBReg(pDM_Odm, ADDAReg[i], bMaskDWord, ADDABackup[i]);
	}
}

static VOID
_PHY_ReloadMACRegisters8723B(IN PADAPTER pAdapter,
			     IN pu4Byte MACReg, IN pu4Byte MACBackup)
{
	u4Byte	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Reload MAC parameters !\n"));
	for (i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++) {
		ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)MACBackup[i]);
	}
	ODM_Write4Byte(pDM_Odm, MACReg[i], MACBackup[i]);
}

static VOID
_PHY_PathADDAOn8723B(IN PADAPTER pAdapter, IN pu4Byte ADDAReg,
		     IN BOOLEAN isPathAOn, IN BOOLEAN is2T)
{
	u4Byte	pathOn;
	u4Byte	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("ADDA ON.\n"));

	pathOn = isPathAOn ? 0x01c00014 : 0x01c00014;
	if (FALSE == is2T){
		pathOn = 0x01c00014;
		ODM_SetBBReg(pDM_Odm, ADDAReg[0], bMaskDWord, 0x01c00014);
	} else {
		ODM_SetBBReg(pDM_Odm,ADDAReg[0], bMaskDWord, pathOn);
	}

	for (i = 1 ; i < IQK_ADDA_REG_NUM ; i++) {
		ODM_SetBBReg(pDM_Odm,ADDAReg[i], bMaskDWord, pathOn);
	}

}

static VOID
_PHY_MACSettingCalibration8723B(IN PADAPTER pAdapter,
				IN pu4Byte MACReg, IN pu4Byte MACBackup)
{
	u4Byte	i = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("MAC settings for Calibration.\n"));

	ODM_Write1Byte(pDM_Odm, MACReg[i], 0x3F);

	for(i = 1 ; i < (IQK_MAC_REG_NUM - 1); i++){
		ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)(MACBackup[i]&(~BIT3)));
	}
	ODM_Write1Byte(pDM_Odm, MACReg[i], (u1Byte)(MACBackup[i]&(~BIT5)));
}

static VOID
_PHY_PathAStandBy8723B(IN PADAPTER pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("Path-A standby mode!\n"));

	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x0);
//Allen
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bMaskDWord, 0x10000);
	//ODM_SetBBReg(pDM_Odm, 0x840, bMaskDWord, 0x00010000);
//
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x808000);
}

static VOID
_PHY_PIModeSwitch8723B(IN PADAPTER pAdapter, IN BOOLEAN PIMode)
{
	u4Byte mode;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T pDM_Odm = &pHalData->odmpriv;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("BB Switch to %s mode!\n", (PIMode ? "PI" : "SI")));

	mode = PIMode ? 0x01000100 : 0x01000000;
	ODM_SetBBReg(pDM_Odm, rFPGA0_XA_HSSIParameter1, bMaskDWord, mode);
	ODM_SetBBReg(pDM_Odm, rFPGA0_XB_HSSIParameter1, bMaskDWord, mode);
}

static BOOLEAN
phy_SimularityCompare_8723B(IN PADAPTER pAdapter, IN s4Byte result[][8],
			    IN u1Byte c1, IN u1Byte c2)
{
	u4Byte		i, j, diff, SimularityBitMap, bound = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	u1Byte		final_candidate[2] = {0xFF, 0xFF};	//for path A and path B
	BOOLEAN		bResult = TRUE;
	BOOLEAN		is2T = TRUE;
	s4Byte tmp1 = 0,tmp2 = 0;

	if (is2T)
		bound = 8;
	else
		bound = 4;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("===> IQK:phy_SimularityCompare_8192E c1 %d c2 %d!!!\n",
		      c1, c2));

	SimularityBitMap = 0;

	for (i = 0; i < bound; i++) {

		if ((i == 1) || (i == 3) || (i == 5) || (i == 7)) {
			if((result[c1][i]& 0x00000200) != 0)
				tmp1 = result[c1][i] | 0xFFFFFC00;
			else
				tmp1 = result[c1][i];

			if((result[c2][i]& 0x00000200) != 0)
				tmp2 = result[c2][i] | 0xFFFFFC00;
			else
				tmp2 = result[c2][i];
		} else {
			tmp1 = result[c1][i];
			tmp2 = result[c2][i];
		}

		diff = (tmp1 > tmp2) ? (tmp1 - tmp2) : (tmp2 - tmp1);

		if (diff > MAX_TOLERANCE) {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("IQK:differnece overflow %d index %d compare1 0x%x compare2 0x%x!!!\n",
				      diff, i, result[c1][i], result[c2][i]));

			if ((i == 2 || i == 6) && !SimularityBitMap) {
				if(result[c1][i]+result[c1][i+1] == 0)
					final_candidate[(i/4)] = c2;
				else if (result[c2][i]+result[c2][i+1] == 0)
					final_candidate[(i/4)] = c1;
				else
					SimularityBitMap =
						SimularityBitMap|(1<<i);
			} else {
				SimularityBitMap = SimularityBitMap|(1<<i);
			}
		}
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("IQK:%s SimularityBitMap %x!!!\n", __func__,
		      SimularityBitMap));

	if (SimularityBitMap == 0) {
		for (i = 0; i < (bound/4); i++) {
			if(final_candidate[i] != 0xFF) {
				for (j = i*4; j < (i+1)*4-2; j++)
					result[3][j] =
						result[final_candidate[i]][j];
				bResult = FALSE;
			}
		}
		return bResult;
	} else {
		if (!(SimularityBitMap & 0x03)) {	//path A TX OK
			for(i = 0; i < 2; i++)
				result[3][i] = result[c1][i];
		}

		if (!(SimularityBitMap & 0x0c)) {	//path A RX OK
			for(i = 2; i < 4; i++)
				result[3][i] = result[c1][i];
		}

		if (!(SimularityBitMap & 0x30)) {	//path B TX OK
			for(i = 4; i < 6; i++)
				result[3][i] = result[c1][i];
		}

		if (!(SimularityBitMap & 0xc0)) {	//path B RX OK
			for(i = 6; i < 8; i++)
				result[3][i] = result[c1][i];
		}
		return FALSE;
	}
}

static VOID
phy_IQCalibrate_8723B(IN PADAPTER pAdapter, IN s4Byte result[][8],
		      IN u1Byte t, IN BOOLEAN is2T, IN u1Byte RF_Path)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	u4Byte	i;
	u1Byte	PathAOK, PathBOK;
	u1Byte	tmp0xc50 = (u1Byte)ODM_GetBBReg(pDM_Odm, 0xC50, bMaskByte0);
	u1Byte	tmp0xc58 = (u1Byte)ODM_GetBBReg(pDM_Odm, 0xC58, bMaskByte0);
	u4Byte	ADDA_REG[IQK_ADDA_REG_NUM] = {
		rFPGA0_XCD_SwitchControl,	rBlue_Tooth,
		rRx_Wait_CCA,			rTx_CCK_RFON,
		rTx_CCK_BBON,			rTx_OFDM_RFON,
		rTx_OFDM_BBON,			rTx_To_Rx,
		rTx_To_Tx,			rRx_CCK,
		rRx_OFDM,			rRx_Wait_RIFS,
		rRx_TO_Rx,			rStandby,
		rSleep,				rPMPD_ANAEN };
	u4Byte	IQK_MAC_REG[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE,		REG_BCN_CTRL,
		REG_BCN_CTRL_1, 	REG_GPIO_MUXCFG};

	//since 92C & 92D have the different define in IQK_BB_REG
	u4Byte	IQK_BB_REG_92C[IQK_BB_REG_NUM] = {
		rOFDM0_TRxPathEnable,		rOFDM0_TRMuxPar,
		rFPGA0_XCD_RFInterfaceSW,	rConfig_AntA,	rConfig_AntB,
		rFPGA0_XAB_RFInterfaceSW,	rFPGA0_XA_RFInterfaceOE,
		rFPGA0_XB_RFInterfaceOE, rCCK0_AFESetting
	};

	u4Byte Path_SEL_BB;
//	  u4Byte Path_SEL_BB, Path_SEL_RF;

#if MP_DRIVER
	const u4Byte retryCount = 1;
#else
	const u4Byte retryCount = 2;
#endif

	// Note: IQ calibration must be performed after loading
	//		PHY_REG.txt , and radio_a, radio_b.txt

	//u4Byte bbvalue;

	if (t == 0) {
//		bbvalue = ODM_GetBBReg(pDM_Odm, rFPGA0_RFMOD, bMaskDWord);
//		RT_DISP(FINIT, INIT_IQK, ("phy_IQCalibrate_8188E()==>0x%08x\n",bbvalue));

		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("IQ Calibration for %s for %d times\n",
			      (is2T ? "2T2R" : "1T1R"), t));

		// Save ADDA parameters, turn Path A ADDA on
		_PHY_SaveADDARegisters8723B(pAdapter, ADDA_REG, pDM_Odm->RFCalibrateInfo.ADDA_backup, IQK_ADDA_REG_NUM);
		_PHY_SaveMACRegisters8723B(pAdapter, IQK_MAC_REG, pDM_Odm->RFCalibrateInfo.IQK_MAC_backup);
		_PHY_SaveADDARegisters8723B(pAdapter, IQK_BB_REG_92C, pDM_Odm->RFCalibrateInfo.IQK_BB_backup, IQK_BB_REG_NUM);
	}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("IQ Calibration for %s for %d times\n",
		      (is2T ? "2T2R" : "1T1R"), t));

	_PHY_PathADDAOn8723B(pAdapter, ADDA_REG, TRUE, is2T);

//no serial mode

	//MAC settings
	_PHY_MACSettingCalibration8723B(pAdapter, IQK_MAC_REG, pDM_Odm->RFCalibrateInfo.IQK_MAC_backup);

	//BB setting
	ODM_SetBBReg(pDM_Odm, rCCK0_AFESetting, 0x0f000000, 0xf);
	ODM_SetBBReg(pDM_Odm, rOFDM0_TRxPathEnable, bMaskDWord, 0x03a05600);
	ODM_SetBBReg(pDM_Odm, rOFDM0_TRMuxPar, bMaskDWord, 0x000800e4);
	ODM_SetBBReg(pDM_Odm, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, 0x22204000);

//RX IQ calibration setting for 8723B D cut large current issue when leaving IPS

	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0x000000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS,
		     bRFRegOffsetMask, 0x30000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1,
		     bRFRegOffsetMask, 0x0001f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2,
		     bRFRegOffsetMask, 0xf7fb7);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x20, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x60fbd);
/*
//LOK RF setting
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x2, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xef, 0x2, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x56, bRFRegOffsetMask, 0x00032);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x76, bRFRegOffsetMask, 0x00032);
*/


//path A TX IQK
#if 1
	for (i = 0 ; i < retryCount ; i++) {
		PathAOK = phy_PathA_IQK_8723B(pAdapter, is2T, RF_Path);
//		if (PathAOK == 0x03) {
		if (PathAOK == 0x01) {
			// Path A Tx IQK Success
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK,
				     bMaskH3Bytes, 0x000000);
			pDM_Odm->RFCalibrateInfo.TxLOK[ODM_RF_PATH_A] =
				ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A,
					     0x8, bRFRegOffsetMask);

			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
				     ODM_DBG_LOUD,
				     ("Path A Tx IQK Success!!\n"));
			result[t][0] = (ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord)&0x3FF0000)>>16;
			break;
		}
	}
#endif

//path A RXIQK
#if 1
	for (i = 0 ; i < retryCount ; i++) {
		PathAOK = phy_PathA_RxIQK8723B(pAdapter, is2T, RF_Path);
		if (PathAOK == 0x03) {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
				     ODM_DBG_LOUD,
				     ("Path A Rx IQK Success!!\n"));
			result[t][2] = (ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord)&0x3FF0000)>>16;
			result[t][3] = (ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord)&0x3FF0000)>>16;
			break;
		} else {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
				     ODM_DBG_LOUD, ("Path A Rx IQK Fail!!\n"));
		}
	}

	if (PathAOK == 0x00) {
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
			     ODM_DBG_LOUD, ("Path A IQK failed!!\n"));
	}
#endif

//path B IQK
	if (is2T) {
//path B TX IQK
		for (i = 0 ; i < retryCount; i++) {
			PathBOK = phy_PathB_IQK_8723B(pAdapter);
//			if(PathBOK == 0x03){
			if (PathBOK == 0x01) {
				// Path B Tx IQK Success
				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK,
					     bMaskH3Bytes, 0x000000);
				pDM_Odm->RFCalibrateInfo.TxLOK[ODM_RF_PATH_B] =
					ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_B,
						     0x8, bRFRegOffsetMask);

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("Path B Tx IQK Success!!\n"));
				result[t][4] = (ODM_GetBBReg(pDM_Odm, rTx_Power_Before_IQK_A, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (ODM_GetBBReg(pDM_Odm, rTx_Power_After_IQK_A, bMaskDWord)&0x3FF0000)>>16;
				break;
			}
		}

//path B RX IQK
		for (i = 0 ; i < retryCount; i++) {
			PathBOK = phy_PathB_RxIQK8723B(pAdapter, is2T);
			if(PathBOK == 0x03){
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("Path B Rx IQK Success!!\n"));
				result[t][6] = (ODM_GetBBReg(pDM_Odm, rRx_Power_Before_IQK_A_2, bMaskDWord)&0x3FF0000)>>16;
				result[t][7] = (ODM_GetBBReg(pDM_Odm, rRx_Power_After_IQK_A_2, bMaskDWord)&0x3FF0000)>>16;
				break;
			} else {
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("Path B Rx IQK Fail!!\n"));
			}
		}

////////Allen end /////////
		if (PathBOK == 0x00) {
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
				     ODM_DBG_LOUD, ("Path B IQK failed!!\n"));
		}
	}

	//Back to BB mode, load original value
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("IQK:Back to BB mode, load original value!\n"));
	ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0);

	if (t != 0) {
		if (!pDM_Odm->RFCalibrateInfo.bRfPiEnable) {
			/*
			 * Switch back BB to SI mode after
			 * finish IQ Calibration.
			 */
//			_PHY_PIModeSwitch8723B(pAdapter, FALSE);
		}

		// Reload ADDA power saving parameters
		_PHY_ReloadADDARegisters8723B(pAdapter, ADDA_REG, pDM_Odm->RFCalibrateInfo.ADDA_backup, IQK_ADDA_REG_NUM);

		// Reload MAC parameters
		_PHY_ReloadMACRegisters8723B(pAdapter, IQK_MAC_REG, pDM_Odm->RFCalibrateInfo.IQK_MAC_backup);

		_PHY_ReloadADDARegisters8723B(pAdapter, IQK_BB_REG_92C, pDM_Odm->RFCalibrateInfo.IQK_BB_backup, IQK_BB_REG_NUM);

		//Reload RF path
//		ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, Path_SEL_BB);
//		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, 0xfffff, Path_SEL_RF);

		//Allen initial gain 0xc50
		// Restore RX initial gain
		ODM_SetBBReg(pDM_Odm, 0xc50, bMaskByte0, 0x50);
		ODM_SetBBReg(pDM_Odm, 0xc50, bMaskByte0, tmp0xc50);
		if (is2T) {
			ODM_SetBBReg(pDM_Odm, 0xc58, bMaskByte0, 0x50);
			ODM_SetBBReg(pDM_Odm, 0xc58, bMaskByte0, tmp0xc58);
		}

		//load 0xe30 IQC default value
		ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord, 0x01008c00);
		ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord, 0x01008c00);
	}
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("phy_IQCalibrate_8723B() <==\n"));
}

static VOID
phy_LCCalibrate_8723B(IN PDM_ODM_T pDM_Odm, IN BOOLEAN is2T)
{
	u1Byte	tmpReg;
	u4Byte	RF_Amode=0, RF_Bmode=0, LC_Cal;
	PADAPTER pAdapter = pDM_Odm->Adapter;

	//Check continuous TX and Packet TX
	tmpReg = ODM_Read1Byte(pDM_Odm, 0xd03);

	if ((tmpReg&0x70) != 0)			//Deal with contisuous TX case
		ODM_Write1Byte(pDM_Odm, 0xd03, tmpReg&0x8F); //disable all continuous TX
	else					// Deal with Packet TX case
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0xFF); // block all queues

	if ((tmpReg&0x70) != 0) {
		//1. Read original RF mode
		//Path-A
		RF_Amode = PHY_QueryRFReg(pAdapter, ODM_RF_PATH_A,
					  RF_AC, bMask12Bits);

		//Path-B
		if (is2T)
			RF_Bmode = PHY_QueryRFReg(pAdapter, ODM_RF_PATH_B,
						  RF_AC, bMask12Bits);

		//2. Set RF mode = standby mode
		//Path-A
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC, bMask12Bits,
			     (RF_Amode&0x8FFFF)|0x10000);

		//Path-B
		if (is2T)
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_AC,
				     bMask12Bits, (RF_Bmode&0x8FFFF)|0x10000);
	}

	//3. Read RF reg18
	LC_Cal = PHY_QueryRFReg(pAdapter, ODM_RF_PATH_A,
				RF_CHNLBW, bMask12Bits);

	//4. Set LC calibration begin	bit15
	// LDO ON
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xB0, bRFRegOffsetMask, 0xDFBE0);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW, bMask12Bits,
		     LC_Cal|0x08000);

	ODM_delay_ms(100);

	// LDO OFF
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xB0, bRFRegOffsetMask, 0xDFFE0);

	// Channel 10 LC calibration issue for 8723bs with 26M xtal
	if (pDM_Odm->SupportInterface == ODM_ITRF_SDIO &&
	    pDM_Odm->PackageType >= 0x2) {
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_CHNLBW,
			     bMask12Bits, LC_Cal);
	}

	//Restore original situation
	if ((tmpReg&0x70) != 0)	{ //Deal with contisuous TX case
		//Path-A
		ODM_Write1Byte(pDM_Odm, 0xd03, tmpReg);
		ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC,
			     bMask12Bits, RF_Amode);

		//Path-B
		if (is2T)
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B,
				     RF_AC, bMask12Bits, RF_Bmode);
	} else { // Deal with Packet TX case
		ODM_Write1Byte(pDM_Odm, REG_TXPAUSE, 0x00);
	}
}

//Analog Pre-distortion calibration
#define	APK_BB_REG_NUM		8
#define	APK_CURVE_REG_NUM	4
#define	PATH_NUM		2

static VOID
phy_APCalibrate_8723B(IN PADAPTER pAdapter, IN s1Byte delta, IN BOOLEAN is2T)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T pDM_Odm = &pHalData->odmpriv;
	u4Byte regD[PATH_NUM];
	u4Byte tmpReg, index, offset,	apkbound;
	u1Byte path, i, pathbound = PATH_NUM;
	u4Byte BB_backup[APK_BB_REG_NUM];
	u4Byte BB_REG[APK_BB_REG_NUM] = {
		rFPGA1_TxBlock,			rOFDM0_TRxPathEnable,
		rFPGA0_RFMOD,			rOFDM0_TRMuxPar,
		rFPGA0_XCD_RFInterfaceSW,	rFPGA0_XAB_RFInterfaceSW,
		rFPGA0_XA_RFInterfaceOE,	rFPGA0_XB_RFInterfaceOE
	};
	u4Byte BB_AP_MODE[APK_BB_REG_NUM] = {
		0x00000020, 0x00a05430, 0x02040000, 0x000800e4, 0x00204000
	};
	u4Byte BB_normal_AP_MODE[APK_BB_REG_NUM] = {
		0x00000020, 0x00a05430, 0x02040000, 0x000800e4, 0x22204000
	};

	u4Byte AFE_backup[IQK_ADDA_REG_NUM];
	u4Byte AFE_REG[IQK_ADDA_REG_NUM] = {
		rFPGA0_XCD_SwitchControl,	rBlue_Tooth,
		rRx_Wait_CCA,			rTx_CCK_RFON,
		rTx_CCK_BBON,			rTx_OFDM_RFON,
		rTx_OFDM_BBON,			rTx_To_Rx,
		rTx_To_Tx,			rRx_CCK,
		rRx_OFDM,			rRx_Wait_RIFS,
		rRx_TO_Rx,			rStandby,
		rSleep,				rPMPD_ANAEN
	};

	u4Byte MAC_backup[IQK_MAC_REG_NUM];
	u4Byte MAC_REG[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE,	REG_BCN_CTRL,
		REG_BCN_CTRL_1, REG_GPIO_MUXCFG};

	u4Byte APK_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
		{0x0852c, 0x1852c, 0x5852c, 0x1852c, 0x5852c},
		{0x2852e, 0x0852e, 0x3852e, 0x0852e, 0x0852e}
	};

	u4Byte APK_normal_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
		//path settings equal to path b settings
		{0x0852c, 0x0a52c, 0x3a52c, 0x5a52c, 0x5a52c},
		{0x0852c, 0x0a52c, 0x5a52c, 0x5a52c, 0x5a52c}
	};

	u4Byte APK_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
		{0x52019, 0x52014, 0x52013, 0x5200f, 0x5208d},
		{0x5201a, 0x52019, 0x52016, 0x52033, 0x52050}
	};

	u4Byte APK_normal_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
		//path settings equal to path b settings
		{0x52019, 0x52017, 0x52010, 0x5200d, 0x5206a},
		{0x52019, 0x52017, 0x52010, 0x5200d, 0x5206a}
	};

	u4Byte AFE_on_off[PATH_NUM] = {
		//path A on path B off / path A off path B on
		0x04db25a4, 0x0b1b25a4};

	u4Byte APK_offset[PATH_NUM] = {
		rConfig_AntA, rConfig_AntB};

	u4Byte APK_normal_offset[PATH_NUM] = {
		rConfig_Pmpd_AntA, rConfig_Pmpd_AntB};

	u4Byte APK_value[PATH_NUM] = {
		0x92fc0000, 0x12fc0000};

	u4Byte APK_normal_value[PATH_NUM] = {
		0x92680000, 0x12680000};

	s1Byte APK_delta_mapping[APK_BB_REG_NUM][13] = {
		{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-6, -4, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6},
		{-11, -9, -7, -5, -3, -1, 0, 0, 0, 0, 0, 0, 0}
	};

	u4Byte APK_normal_setting_value_1[13] = {
		0x01017018, 0xf7ed8f84, 0x1b1a1816, 0x2522201e, 0x322e2b28,
		0x433f3a36, 0x5b544e49, 0x7b726a62, 0xa69a8f84, 0xdfcfc0b3,
		0x12680000, 0x00880000, 0x00880000
	};

	u4Byte APK_normal_setting_value_2[16] = {
		0x01c7021d, 0x01670183, 0x01000123, 0x00bf00e2, 0x008d00a3,
		0x0068007b, 0x004d0059, 0x003a0042, 0x002b0031, 0x001f0025,
		0x0017001b, 0x00110014, 0x000c000f, 0x0009000b, 0x00070008,
		0x00050006
	};

	u4Byte APK_result[PATH_NUM][APK_BB_REG_NUM];	//val_1_1a, val_1_2a, val_2a, val_3a, val_4a
//	u4Byte AP_curve[PATH_NUM][APK_CURVE_REG_NUM];

	s4Byte BB_offset, delta_V, delta_offset;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("==>phy_APCalibrate_8188E() delta %d\n", delta));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("AP Calibration for %s\n", (is2T ? "2T2R" : "1T1R")));
	if (!is2T)
		pathbound = 1;

	//2 FOR NORMAL CHIP SETTINGS

/*
 * Temporarily do not allow normal driver to do the following settings
 * because these offset and value will cause RF internal PA to be
 * unpredictably disabled by HW, such that RF Tx signal will disappear
 * after disable/enable card many times on 88CU. RF SD and DD have not
 * find the root cause, so we remove these actions temporarily. Added
 * by tynli and SD3 Allen. 2010.05.31.
 */
#if MP_DRIVER != 1
	return;
#endif
	//settings adjust for normal chip
	for (index = 0; index < PATH_NUM; index ++) {
		APK_offset[index] = APK_normal_offset[index];
		APK_value[index] = APK_normal_value[index];
		AFE_on_off[index] = 0x6fdb25a4;
	}

	for (index = 0; index < APK_BB_REG_NUM; index ++) {
		for (path = 0; path < pathbound; path++) {
			APK_RF_init_value[path][index] =
				APK_normal_RF_init_value[path][index];
			APK_RF_value_0[path][index] =
				APK_normal_RF_value_0[path][index];
		}
		BB_AP_MODE[index] = BB_normal_AP_MODE[index];
	}

	apkbound = 6;

	//save BB default value
	for (index = 0; index < APK_BB_REG_NUM ; index++) {
		if (index == 0)		//skip
			continue;
		BB_backup[index] = ODM_GetBBReg(pDM_Odm, BB_REG[index],
						bMaskDWord);
	}

	//save MAC default value
	_PHY_SaveMACRegisters8723B(pAdapter, MAC_REG, MAC_backup);

	//save AFE default value
	_PHY_SaveADDARegisters8723B(pAdapter, AFE_REG, AFE_backup,
				    IQK_ADDA_REG_NUM);

	for (path = 0; path < pathbound; path++) {
		if (path == ODM_RF_PATH_A) {
			//path A APK
			//load APK setting
			//path-A
			offset = rPdp_AntA;
			for (index = 0; index < 11; index ++) {
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}

			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntB,
				     bMaskDWord, 0x12680000);

			offset = rConfig_AntA;
			for (; index < 13; index ++) {
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}

			//page-B1
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes,
				     0x400000);

			//path A
			offset = rPdp_AntA;
			for (index = 0; index < 16; index++) {
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_2[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes,
				     0x000000);
		} else if (path == ODM_RF_PATH_B) {
			//path B APK
			//load APK setting
			//path-B
			offset = rPdp_AntB;
			for (index = 0; index < 10; index ++) {
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}
			ODM_SetBBReg(pDM_Odm, rConfig_Pmpd_AntA,
				     bMaskDWord, 0x12680000);
			PHY_SetBBReg(pAdapter, rConfig_Pmpd_AntB,
				     bMaskDWord, 0x12680000);

			offset = rConfig_AntA;
			index = 11;
			for(; index < 13; index ++) { //offset 0xb68, 0xb6c
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_1[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}

			//page-B1
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes,
				     0x400000);

			//path B
			offset = 0xb60;
			for (index = 0; index < 16; index++) {
				ODM_SetBBReg(pDM_Odm, offset, bMaskDWord,
					     APK_normal_setting_value_2[index]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      offset,
					      ODM_GetBBReg(pDM_Odm, offset,
							   bMaskDWord)));

				offset += 0x04;
			}
			ODM_SetBBReg(pDM_Odm, rFPGA0_IQK, bMaskH3Bytes, 0);
		}

		//save RF default value
		regD[path] = PHY_QueryRFReg(pAdapter, path, RF_TXBIAS_A,
					    bMaskDWord);

		//Path A AFE all on, path B AFE All off or vise versa
		for(index = 0; index < IQK_ADDA_REG_NUM ; index++)
			ODM_SetBBReg(pDM_Odm, AFE_REG[index], bMaskDWord, AFE_on_off[path]);
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("phy_APCalibrate_8188E() offset 0xe70 %x\n",
			      ODM_GetBBReg(pDM_Odm, rRx_Wait_CCA, bMaskDWord)));

		//BB to AP mode
		if (path == 0) {
			for (index = 0; index < APK_BB_REG_NUM ; index++) {
				if (index == 0)		//skip
					continue;
				else if (index < 5)
					ODM_SetBBReg(pDM_Odm, BB_REG[index],
						     bMaskDWord,
						     BB_AP_MODE[index]);
				else if (BB_REG[index] == 0x870)
					ODM_SetBBReg(pDM_Odm, BB_REG[index],
						     bMaskDWord,
						     BB_backup[index]|BIT10|BIT26);
				else
					ODM_SetBBReg(pDM_Odm, BB_REG[index],
						     BIT10, 0x0);
			}

			ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_A, bMaskDWord,
				     0x01008c00);
			ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_A, bMaskDWord,
				     0x01008c00);
		} else { //path B
			ODM_SetBBReg(pDM_Odm, rTx_IQK_Tone_B, bMaskDWord,
				     0x01008c00);
			ODM_SetBBReg(pDM_Odm, rRx_IQK_Tone_B, bMaskDWord,
				     0x01008c00);
		}

		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("phy_APCalibrate_8188E() offset 0x800 %x\n",
			      ODM_GetBBReg(pDM_Odm, 0x800, bMaskDWord)));

		//MAC settings
		_PHY_MACSettingCalibration8723B(pAdapter, MAC_REG, MAC_backup);

		if (path == ODM_RF_PATH_A) {	//Path B to standby mode
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_AC,
				     bMaskDWord, 0x10000);
		} else {			//Path A to standby mode
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_AC,
				     bMaskDWord, 0x10000);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_MODE1,
				     bMaskDWord, 0x1000f);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_MODE2,
				     bMaskDWord, 0x20103);
		}

		delta_offset = ((delta+14)/2);
		if(delta_offset < 0)
			delta_offset = 0;
		else if (delta_offset > 12)
			delta_offset = 12;

		//AP calibration
		for (index = 0; index < APK_BB_REG_NUM; index++) {
			if(index != 1)	//only DO PA11+PAD01001, AP RF setting
				continue;

			tmpReg = APK_RF_init_value[path][index];
#if 1
			if (!pDM_Odm->RFCalibrateInfo.bAPKThermalMeterIgnore) {
				BB_offset = (tmpReg & 0xF0000) >> 16;

				if (!(tmpReg & BIT15)) { //sign bit 0
					BB_offset = -BB_offset;
				}

				delta_V = APK_delta_mapping[index][delta_offset];

				BB_offset += delta_V;

				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() APK index %d tmpReg 0x%x delta_V %d delta_offset %d\n",
					      index, tmpReg, delta_V,
					      delta_offset));

				if (BB_offset < 0) {
					tmpReg = tmpReg & (~BIT15);
					BB_offset = -BB_offset;
				} else {
					tmpReg = tmpReg | BIT15;
				}
				tmpReg = (tmpReg & 0xFFF0FFFF) |
					(BB_offset << 16);
			}
#endif

			ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
				     RF_IPA_A, bMaskDWord, 0x8992e);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("phy_APCalibrate_8188E() offset 0xc %x\n",
				      PHY_QueryRFReg(pAdapter, path,
						     RF_IPA_A, bMaskDWord)));
			ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path, RF_AC,
				     bMaskDWord, APK_RF_value_0[path][index]);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("phy_APCalibrate_8188E() offset 0x0 %x\n",
				      PHY_QueryRFReg(pAdapter, path, RF_AC,
						     bMaskDWord)));
			ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
				     RF_TXBIAS_A, bMaskDWord, tmpReg);
			ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
				     ("phy_APCalibrate_8188E() offset 0xd %x\n",
				      PHY_QueryRFReg(pAdapter, path,
						     RF_TXBIAS_A, bMaskDWord)));

			// PA11+PAD01111, one shot
			i = 0;
			do {
				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK,
					     bMaskH3Bytes, 0x800000);

				ODM_SetBBReg(pDM_Odm, APK_offset[path],
					     bMaskDWord, APK_value[0]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      APK_offset[path],
					      ODM_GetBBReg(pDM_Odm,
							   APK_offset[path],
							   bMaskDWord)));
				ODM_delay_ms(3);
				ODM_SetBBReg(pDM_Odm, APK_offset[path],
					     bMaskDWord, APK_value[1]);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0x%x value 0x%x\n",
					      APK_offset[path],
					      ODM_GetBBReg(pDM_Odm,
							   APK_offset[path],
							   bMaskDWord)));

				ODM_delay_ms(20);

				ODM_SetBBReg(pDM_Odm, rFPGA0_IQK,
					     bMaskH3Bytes, 0x000000);

				if(path == ODM_RF_PATH_A)
					tmpReg = ODM_GetBBReg(pDM_Odm, rAPK,
							      0x03E00000);
				else
					tmpReg = ODM_GetBBReg(pDM_Odm, rAPK,
							      0xF8000000);
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION,
					     ODM_DBG_LOUD,
					     ("phy_APCalibrate_8188E() offset 0xbd8[25:21] %x\n", tmpReg));


				i++;
			} while(tmpReg > apkbound && i < 4);

			APK_result[path][index] = tmpReg;
		}
	}

	//reload MAC default value
	_PHY_ReloadMACRegisters8723B(pAdapter, MAC_REG, MAC_backup);

	//reload BB default value
	for (index = 0; index < APK_BB_REG_NUM; index++) {
		if(index == 0)		//skip
			continue;
		ODM_SetBBReg(pDM_Odm, BB_REG[index], bMaskDWord,
			     BB_backup[index]);
	}

	//reload AFE default value
	_PHY_ReloadADDARegisters8723B(pAdapter, AFE_REG,
				      AFE_backup, IQK_ADDA_REG_NUM);

	//reload RF path default value
	for (path = 0; path < pathbound; path++) {
		ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
			     0xd, bMaskDWord, regD[path]);
		if (path == ODM_RF_PATH_B) {
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A,
				     RF_MODE1, bMaskDWord, 0x1000f);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A,
				     RF_MODE2, bMaskDWord, 0x20101);
		}

		//note no index == 0
		if (APK_result[path][1] > 6)
			APK_result[path][1] = 6;
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
			     ("apk path %d result %d 0x%x \t",
			      path, 1, APK_result[path][1]));
	}

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("\n"));

	for (path = 0; path < pathbound; path++) {
		ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
			     0x3, bMaskDWord,
			     ((APK_result[path][1] << 15) |
			      (APK_result[path][1] << 10) |
			      (APK_result[path][1] << 5) |
			      APK_result[path][1]));
		if (path == ODM_RF_PATH_A)
			ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
				     0x4, bMaskDWord,
				     ((APK_result[path][1] << 15) |
				      (APK_result[path][1] << 10) |
				      (0x00 << 5) | 0x05));
		else
			ODM_SetRFReg(pDM_Odm, (ODM_RF_RADIO_PATH_E)path,
				     0x4, bMaskDWord,
				     ((APK_result[path][1] << 15) |
				      (APK_result[path][1] << 10) |
				      (0x02 << 5) | 0x05));
	}

	pDM_Odm->RFCalibrateInfo.bAPKdone = TRUE;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("<==phy_APCalibrate_8188E()\n"));
}



#define DP_BB_REG_NUM		7
#define DP_RF_REG_NUM		1
#define DP_RETRY_LIMIT		10
#define DP_PATH_NUM		2
#define DP_DPK_NUM		3
#define DP_DPK_VALUE_NUM	2

//IQK version:V2.5    20140123
//IQK is controlled by Is2ant, RF path
VOID
PHY_IQCalibrate_8723B(IN PADAPTER pAdapter, IN BOOLEAN bReCovery,
		      IN BOOLEAN bRestore, IN BOOLEAN Is2ant,
		      //false:1ant, true:2-ant
		      IN u1Byte	RF_Path)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;
	s4Byte		result[4][8];	//last is final result
	u1Byte		i, final_candidate, Indexforchannel;
	BOOLEAN		bPathAOK, bPathBOK;
	s4Byte RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC, RegTmp = 0;
	BOOLEAN	is12simular, is13simular, is23simular;
	BOOLEAN	bStartContTx = FALSE, bSingleTone = FALSE, bCarrierSuppression = FALSE;
	u4Byte	IQK_BB_REG_92C[IQK_BB_REG_NUM] = {
		rOFDM0_XARxIQImbalance,		rOFDM0_XBRxIQImbalance,
		rOFDM0_ECCAThreshold,		rOFDM0_AGCRSSITable,
		rOFDM0_XATxIQImbalance,		rOFDM0_XBTxIQImbalance,
		rOFDM0_XCTxAFE,			rOFDM0_XDTxAFE,
		rOFDM0_RxIQExtAnta};
	u4Byte		GNT_BT_default;
	u4Byte		StartTime;
	s4Byte		ProgressingTime;

	if (ODM_CheckPowerStatus(pAdapter) == FALSE)
		return;

	if (!(pDM_Odm->SupportAbility & ODM_RF_CALIBRATION)) {
		return;
	}

#if MP_DRIVER == 1
	bStartContTx = pMptCtx->bStartContTx;
	bSingleTone = pMptCtx->bSingleTone;
	bCarrierSuppression = pMptCtx->bCarrierSuppression;
#endif

	/*
	 * 20120213<Kordan> Turn on when continuous Tx to pass lab testing.
	 * (required by Edlu)
	 */
	if(bSingleTone || bCarrierSuppression)
		return;

	if (pDM_Odm->RFCalibrateInfo.bIQKInProgress)
		return;

	ODM_AcquireSpinLock(pDM_Odm, RT_IQK_SPINLOCK);
	pDM_Odm->RFCalibrateInfo.bIQKInProgress = TRUE;
	ODM_ReleaseSpinLock(pDM_Odm, RT_IQK_SPINLOCK);

	if (bRestore) {
		u4Byte offset, data;
		u1Byte path, bResult = SUCCESS;
		PODM_RF_CAL_T RFC = &pDM_Odm->RFCalibrateInfo;

		//#define PATH_S0	1 // RF_PATH_B
		//#define PATH_S1	0 // RF_PATH_A

		path = (ODM_GetBBReg(pDM_Odm, rS0S1_PathSwitch,
				     bMaskByte0) == 0x00) ?
			ODM_RF_PATH_A : ODM_RF_PATH_B;

		// Restore TX IQK
		for (i = 0; i < 3; ++i) {
			offset = RFC->TxIQC_8723B[path][i][0];
			data = RFC->TxIQC_8723B[path][i][1];
			if ((offset == 0) || (data == 0)) {
				DBG_871X("%s =>path:%s Restore TX IQK result failed \n",__FUNCTION__,(path==ODM_RF_PATH_A)?"A":"B");
				bResult = FAIL;
				break;
			}
			//RT_TRACE(_module_mp_, _drv_notice_,("Switch to S1 TxIQC(offset, data) = (0x%X, 0x%X)\n", offset, data));
			ODM_SetBBReg(pDM_Odm,offset, bMaskDWord, data);
		}

		// Restore RX IQK
		for (i = 0; i < 2; ++i) {
			offset = RFC->RxIQC_8723B[path][i][0];
			data = RFC->RxIQC_8723B[path][i][1];
			if ((offset == 0) || (data == 0)) {
				DBG_871X("%s =>path:%s  Restore RX IQK result failed \n",__FUNCTION__,(path==ODM_RF_PATH_A)?"A":"B");
				bResult = FAIL;
				break;
			}
			//RT_TRACE(_module_mp_, _drv_notice_,("Switch to S1 RxIQC (offset, data) = (0x%X, 0x%X)\n", offset, data));
			ODM_SetBBReg(pDM_Odm,offset, bMaskDWord, data);
		}

		if (pDM_Odm->RFCalibrateInfo.TxLOK[ODM_RF_PATH_A] ==0) {
			DBG_871X("%s => Restore Path-A TxLOK result failed \n",
				 __FUNCTION__);
			bResult = FAIL;
		} else {
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXM_IDAC,
				     bRFRegOffsetMask,
				     pDM_Odm->RFCalibrateInfo.TxLOK[ODM_RF_PATH_A]);
			ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_B, RF_TXM_IDAC,
				     bRFRegOffsetMask,
				     pDM_Odm->RFCalibrateInfo.TxLOK[ODM_RF_PATH_B]);
		}

		if (bResult == SUCCESS)
			return;
	}

	if (bReCovery) {
		ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, ("PHY_IQCalibrate_8723B: Return due to bReCovery!\n"));
		_PHY_ReloadADDARegisters8723B(pAdapter, IQK_BB_REG_92C, pDM_Odm->RFCalibrateInfo.IQK_BB_backup_recover, 9);
		return;
	}
	StartTime = ODM_GetCurrentTime(pDM_Odm);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("IQK:Start!!!\n"));

	//save default GNT_BT
	GNT_BT_default = ODM_GetBBReg(pDM_Odm, 0x764, bMaskDWord);
	// Save RF Path
//	Path_SEL_BB = ODM_GetBBReg(pDM_Odm, 0x948, bMaskDWord);
//	Path_SEL_RF = ODM_GetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, 0xfffff);

    //set GNT_BT=0, pause BT traffic
//	ODM_SetBBReg(pDM_Odm, 0x764, BIT12, 0x0);
//	ODM_SetBBReg(pDM_Odm, 0x764, BIT11, 0x1);


	for (i = 0; i < 8; i++) {
		result[0][i] = 0;
		result[1][i] = 0;
		result[2][i] = 0;
		result[3][i] = 0;
	}
	final_candidate = 0xff;
	bPathAOK = FALSE;
	bPathBOK = FALSE;
	is12simular = FALSE;
	is23simular = FALSE;
	is13simular = FALSE;


	for (i=0; i<3; i++) {
		phy_IQCalibrate_8723B(pAdapter, result, i, Is2ant, RF_Path);

		if (i == 1) {
			is12simular = phy_SimularityCompare_8723B(pAdapter, result, 0, 1);
			if (is12simular) {
				final_candidate = 0;
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("IQK: is12simular final_candidate is %x\n",final_candidate));
				break;
			}
		}

		if (i == 2) {
			is13simular = phy_SimularityCompare_8723B(pAdapter, result, 0, 2);
			if (is13simular) {
				final_candidate = 0;
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("IQK: is13simular final_candidate is %x\n",final_candidate));

				break;
			}
			is23simular = phy_SimularityCompare_8723B(pAdapter, result, 1, 2);
			if (is23simular) {
				final_candidate = 1;
				ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("IQK: is23simular final_candidate is %x\n",final_candidate));
			} else {
				for(i = 0; i < 8; i++)
					RegTmp += result[3][i];

				if(RegTmp != 0)
					final_candidate = 3;
				else
					final_candidate = 0xFF;
			}
		}
	}
//	RT_TRACE(COMP_INIT,DBG_LOUD,("Release Mutex in IQCalibrate \n"));

	for (i = 0; i < 4; i++) {
		RegE94 = result[i][0];
		RegE9C = result[i][1];
		RegEA4 = result[i][2];
		RegEAC = result[i][3];
		RegEB4 = result[i][4];
		RegEBC = result[i][5];
		RegEC4 = result[i][6];
		RegECC = result[i][7];
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD, ("IQK: RegE94=%x RegE9C=%x RegEA4=%x RegEAC=%x RegEB4=%x RegEBC=%x RegEC4=%x RegECC=%x\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC));
	}

	if (final_candidate != 0xff) {
		pDM_Odm->RFCalibrateInfo.RegE94 = RegE94 = result[final_candidate][0];
		pDM_Odm->RFCalibrateInfo.RegE9C = RegE9C = result[final_candidate][1];
		RegEA4 = result[final_candidate][2];
		RegEAC = result[final_candidate][3];
		pDM_Odm->RFCalibrateInfo.RegEB4 = RegEB4 = result[final_candidate][4];
		pDM_Odm->RFCalibrateInfo.RegEBC = RegEBC = result[final_candidate][5];
		RegEC4 = result[final_candidate][6];
		RegECC = result[final_candidate][7];
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("IQK: final_candidate is %x\n",final_candidate));
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("IQK: RegE94=%x RegE9C=%x RegEA4=%x RegEAC=%x RegEB4=%x RegEBC=%x RegEC4=%x RegECC=%x\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC));
		bPathAOK = bPathBOK = TRUE;
	} else {
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("IQK: FAIL use default value\n"));

		pDM_Odm->RFCalibrateInfo.RegE94 = pDM_Odm->RFCalibrateInfo.RegEB4 = 0x100;	//X default value
		pDM_Odm->RFCalibrateInfo.RegE9C = pDM_Odm->RFCalibrateInfo.RegEBC = 0x0;		//Y default value
	}

#if MP_DRIVER == 1
	if ((pMptCtx->MptRfPath == ODM_RF_PATH_A) || ( *(pDM_Odm->mp_mode) == 0))
#endif
	{
		if (RegE94 != 0) {
			_PHY_PathAFillIQKMatrix8723B(pAdapter, bPathAOK, result, final_candidate, (RegEA4 == 0));
		}
	}

#if MP_DRIVER == 1
	if ((pMptCtx->MptRfPath == ODM_RF_PATH_A) || ( *(pDM_Odm->mp_mode) == 0))
#endif
	{
		if (RegEB4 != 0)
		{
			_PHY_PathBFillIQKMatrix8723B(pAdapter, bPathBOK, result, final_candidate, (RegEC4 == 0));
		}
	}

	Indexforchannel = ODM_GetRightChnlPlaceforIQK(pHalData->CurrentChannel);

//To Fix BSOD when final_candidate is 0xff
//by sherry 20120321
	if (final_candidate < 4) {
		for( i = 0; i < IQK_Matrix_REG_NUM; i++)
			pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[Indexforchannel].Value[0][i] = result[final_candidate][i];
		pDM_Odm->RFCalibrateInfo.IQKMatrixRegSetting[Indexforchannel].bIQKDone = TRUE;
	}
	//RT_DISP(FINIT, INIT_IQK, ("\nIQK OK Indexforchannel %d.\n", Indexforchannel));
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,  ("\nIQK OK Indexforchannel %d.\n", Indexforchannel));

	_PHY_SaveADDARegisters8723B(pAdapter, IQK_BB_REG_92C, pDM_Odm->RFCalibrateInfo.IQK_BB_backup_recover, 9);

	//restore GNT_BT
	ODM_SetBBReg(pDM_Odm, 0x764, bMaskDWord, GNT_BT_default);
	// Restore RF Path
//	ODM_SetBBReg(pDM_Odm, 0x948, bMaskDWord, Path_SEL_BB);
//	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xb0, 0xfffff, Path_SEL_RF);

	//Resotr RX mode table parameter
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_WE_LUT, 0x80000, 0x1);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_RCK_OS, bRFRegOffsetMask,
		     0x18000);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G1, bRFRegOffsetMask,
		     0x0001f);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, RF_TXPA_G2, bRFRegOffsetMask,
		     0xe6177);
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0xed, 0x20, 0x1 );
	ODM_SetRFReg(pDM_Odm, ODM_RF_PATH_A, 0x43, bRFRegOffsetMask, 0x300bd);

	//set GNT_BT= HW control
//	ODM_SetBBReg(pDM_Odm, 0x764, BIT12, 0x0);
//	ODM_SetBBReg(pDM_Odm, 0x764, BIT11, 0x0);

	if (Is2ant) {
		if (RF_Path == 0x0)	//S1
			ODM_SetIQCbyRFpath(pDM_Odm, 0);
		else	//S0
			ODM_SetIQCbyRFpath(pDM_Odm, 1);
	}

	ODM_AcquireSpinLock(pDM_Odm, RT_IQK_SPINLOCK);
	pDM_Odm->RFCalibrateInfo.bIQKInProgress = FALSE;
	ODM_ReleaseSpinLock(pDM_Odm, RT_IQK_SPINLOCK);

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("IQK finished\n"));
	ProgressingTime = ODM_GetProgressingTime( pDM_Odm, StartTime);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("IQK ProgressingTime = %d\n", ProgressingTime));
}


VOID
PHY_LCCalibrate_8723B(IN PDM_ODM_T pDM_Odm)
{
	BOOLEAN		bStartContTx = FALSE, bSingleTone = FALSE, bCarrierSuppression = FALSE;
	u4Byte		timeout = 2000, timecount = 0;
	u4Byte		StartTime;
	s4Byte		ProgressingTime;
	PADAPTER	pAdapter = pDM_Odm->Adapter;

	if (!(pDM_Odm->SupportAbility & ODM_RF_CALIBRATION)) {
		return;
	}
	/*
	 * 20120213<Kordan> Turn on when continuous Tx to pass lab testing.
	 * (required by Edlu)
	 */
	if (bSingleTone || bCarrierSuppression)
		return;

	StartTime = ODM_GetCurrentTime( pDM_Odm);
	while (*(pDM_Odm->pbScanInProcess) && timecount < timeout) {
		ODM_delay_ms(50);
		timecount += 50;
	}

	pDM_Odm->RFCalibrateInfo.bLCKInProgress = TRUE;

	phy_LCCalibrate_8723B(pDM_Odm, FALSE);

	pDM_Odm->RFCalibrateInfo.bLCKInProgress = FALSE;

	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("LCK:Finish!!!interface %d\n", pDM_Odm->InterfaceIndex));
	ProgressingTime = ODM_GetProgressingTime( pDM_Odm, StartTime);
	ODM_RT_TRACE(pDM_Odm,ODM_COMP_CALIBRATION, ODM_DBG_LOUD,
		     ("LCK ProgressingTime = %d\n", ProgressingTime));
}

VOID
PHY_APCalibrate_8723B(IN PADAPTER pAdapter, IN s1Byte delta)
{
}

static VOID phy_SetRFPathSwitch_8723B(IN PADAPTER pAdapter,
			       IN BOOLEAN bMain, IN BOOLEAN is2T)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T	pDM_Odm = &pHalData->odmpriv;

	if (bMain) { // Left antenna
		ODM_SetBBReg(pDM_Odm, 0x92C, bMaskDWord, 0x1);
	} else {
		ODM_SetBBReg(pDM_Odm, 0x92C, bMaskDWord, 0x2);
	}
}

VOID PHY_SetRFPathSwitch_8723B(IN PADAPTER pAdapter, IN BOOLEAN bMain)
{
	phy_SetRFPathSwitch_8723B(pAdapter, bMain, TRUE);
}
