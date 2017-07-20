#ifndef __PHYDM_RTL8822B_RAM_H__
#define __PHYDM_RTL8822B_RAM_H__

//============================================================
//3
//3 This file defines 8822B RAM code function pointer
//3 
//============================================================

//#define phydm_InitFuncPtrTableRAM   phydm_InitFuncPtrTableRAM8822B

//void phydm_InitFuncPtrTableRAM8822B(void);

#if IS_CUT_TEST(CONFIG_CHIP_SEL)
#define FW_FIFO_Parsing_pu1Byte     FW_FIFO_Parsing_pu1Byte_RAM
#define odm_ConvertTo_dB                    odm_ConvertTo_dB_RAM
#define odm_ConvertTo_linear                odm_ConvertTo_linear_RAM
#define odm_QueryRxPwrPercentage    odm_QueryRxPwrPercentage_RAM
#define odm_EVMdbToPercentage           odm_EVMdbToPercentage_RAM
#define phydm_ResetPhyInfo                  phydm_ResetPhyInfo_RAM
#define phydm_SetPerPathPhyInfo         phydm_SetPerPathPhyInfo_RAM
#define phydm_SetCommonPhyInfo      phydm_SetCommonPhyInfo_RAM
#define phydm_GetRxPhyStatusType0   phydm_GetRxPhyStatusType0_RAM
#define phydm_GetRxPhyStatusType1   phydm_GetRxPhyStatusType1_RAM
#define phydm_GetRxPhyStatusType2   phydm_GetRxPhyStatusType2_RAM
#define phydm_GetRxPhyStatusType5   phydm_GetRxPhyStatusType5_RAM
#define phydm_Process_RSSIForDM_Jaguar2 phydm_Process_RSSIForDM_Jaguar2_RAM
#define phydm_RxPhyStatusJaguarSeries2      phydm_RxPhyStatusJaguarSeries2_RAM
#define phystatus_parsing                               phystatus_parsing_RAM
#elif IS_CUT_B(CONFIG_CHIP_SEL)

#endif

#endif //__PHYDM_RTL8821C_RAM_H__