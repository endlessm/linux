/*
 * Copyright 2015 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fb.h>

#include "tonga_processpptables.h"
#include "ppatomctrl.h"
#include "atombios.h"
#include "pp_debug.h"
#include "hwmgr.h"
#include "cgs_common.h"
#include "tonga_pptable.h"

/**
 * Private Function used during initialization.
 * @param hwmgr Pointer to the hardware manager.
 * @param setIt A flag indication if the capability should be set (TRUE) or reset (FALSE).
 * @param cap Which capability to set/reset.
 */
static void set_hw_cap(struct pp_hwmgr *hwmgr, bool setIt, enum phm_platform_caps cap)
{
	if (setIt)
		phm_cap_set(hwmgr->platform_descriptor.platformCaps, cap);
	else
		phm_cap_unset(hwmgr->platform_descriptor.platformCaps, cap);
}


/**
 * Private Function used during initialization.
 * @param hwmgr Pointer to the hardware manager.
 * @param powerplay_caps the bit array (from BIOS) of capability bits.
 * @exception the current implementation always returns 1.
 */
static int set_platform_caps(struct pp_hwmgr *hwmgr, uint32_t powerplay_caps)
{
	PP_ASSERT_WITH_CODE((~powerplay_caps & ____RETIRE16____),
		"ATOM_PP_PLATFORM_CAP_ASPM_L1 is not supported!", continue);
	PP_ASSERT_WITH_CODE((~powerplay_caps & ____RETIRE64____),
		"ATOM_PP_PLATFORM_CAP_GEMINIPRIMARY is not supported!", continue);
	PP_ASSERT_WITH_CODE((~powerplay_caps & ____RETIRE512____),
		"ATOM_PP_PLATFORM_CAP_SIDEPORTCONTROL is not supported!", continue);
	PP_ASSERT_WITH_CODE((~powerplay_caps & ____RETIRE1024____),
		"ATOM_PP_PLATFORM_CAP_TURNOFFPLL_ASPML1 is not supported!", continue);
	PP_ASSERT_WITH_CODE((~powerplay_caps & ____RETIRE2048____),
		"ATOM_PP_PLATFORM_CAP_HTLINKCONTROL is not supported!", continue);

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_POWERPLAY),
			PHM_PlatformCaps_PowerPlaySupport
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_SBIOSPOWERSOURCE),
			PHM_PlatformCaps_BiosPowerSourceControl
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_HARDWAREDC),
			PHM_PlatformCaps_AutomaticDCTransition
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_MVDD_CONTROL),
			PHM_PlatformCaps_EnableMVDDControl
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_VDDCI_CONTROL),
			PHM_PlatformCaps_ControlVDDCI
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_VDDGFX_CONTROL),
			PHM_PlatformCaps_ControlVDDGFX
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_BACO),
			PHM_PlatformCaps_BACO
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_CAP_DISABLE_VOLTAGE_ISLAND),
			PHM_PlatformCaps_DisableVoltageIsland
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PP_PLATFORM_COMBINE_PCC_WITH_THERMAL_SIGNAL),
			PHM_PlatformCaps_CombinePCCWithThermalSignal
		  );

	set_hw_cap(
			hwmgr,
			0 != (powerplay_caps & ATOM_TONGA_PLATFORM_LOAD_POST_PRODUCTION_FIRMWARE),
			PHM_PlatformCaps_LoadPostProductionFirmware
		  );

	return 0;
}

/**
 * Private Function to get the PowerPlay Table Address.
 */
const void *get_powerplay_table(struct pp_hwmgr *hwmgr)
{
	int index = GetIndexIntoMasterTable(DATA, PowerPlayInfo);

	u16 size;
	u8 frev, crev;
	void *table_address;

	table_address = (ATOM_Tonga_POWERPLAYTABLE *)
		cgs_atom_get_data_table(hwmgr->device, index, &size, &frev, &crev);

	hwmgr->soft_pp_table = table_address;	/*Cache the result in RAM.*/

	return table_address;
}

static int get_vddc_lookup_table(
		struct pp_hwmgr	*hwmgr,
		phm_ppt_v1_voltage_lookup_table	**lookup_table,
		const ATOM_Tonga_Voltage_Lookup_Table	*vddc_lookup_pp_tables,
		uint32_t	max_levels
		)
{
	uint32_t table_size, i;
	phm_ppt_v1_voltage_lookup_table *table;

	PP_ASSERT_WITH_CODE((0 != vddc_lookup_pp_tables->ucNumEntries),
		"Invalid CAC Leakage PowerPlay Table!", return 1);

	table_size = sizeof(uint32_t) +
		sizeof(phm_ppt_v1_voltage_lookup_record) * max_levels;

	table = (phm_ppt_v1_voltage_lookup_table *)
		kzalloc(table_size, GFP_KERNEL);

	if (NULL == table)
		return -ENOMEM;

	memset(table, 0x00, table_size);

	table->count = vddc_lookup_pp_tables->ucNumEntries;

	for (i = 0; i < vddc_lookup_pp_tables->ucNumEntries; i++) {
		table->entries[i].us_calculated = 0;
		table->entries[i].us_vdd =
			vddc_lookup_pp_tables->entries[i].usVdd;
		table->entries[i].us_cac_low =
			vddc_lookup_pp_tables->entries[i].usCACLow;
		table->entries[i].us_cac_mid =
			vddc_lookup_pp_tables->entries[i].usCACMid;
		table->entries[i].us_cac_high =
			vddc_lookup_pp_tables->entries[i].usCACHigh;
	}

	*lookup_table = table;

	return 0;
}

/**
 * Private Function used during initialization.
 * Initialize Platform Power Management Parameter table
 * @param hwmgr Pointer to the hardware manager.
 * @param atom_ppm_table Pointer to PPM table in VBIOS
 */
static int get_platform_power_management_table(
		struct pp_hwmgr *hwmgr,
		ATOM_Tonga_PPM_Table *atom_ppm_table)
{
	struct phm_ppm_table *ptr = kzalloc(sizeof(ATOM_Tonga_PPM_Table), GFP_KERNEL);
	struct phm_ppt_v1_information *pp_table_information =
		(struct phm_ppt_v1_information *)(hwmgr->pptable);

	if (NULL == ptr)
		return -ENOMEM;

	ptr->ppm_design
		= atom_ppm_table->ucPpmDesign;
	ptr->cpu_core_number
		= atom_ppm_table->usCpuCoreNumber;
	ptr->platform_tdp
		= atom_ppm_table->ulPlatformTDP;
	ptr->small_ac_platform_tdp
		= atom_ppm_table->ulSmallACPlatformTDP;
	ptr->platform_tdc
		= atom_ppm_table->ulPlatformTDC;
	ptr->small_ac_platform_tdc
		= atom_ppm_table->ulSmallACPlatformTDC;
	ptr->apu_tdp
		= atom_ppm_table->ulApuTDP;
	ptr->dgpu_tdp
		= atom_ppm_table->ulDGpuTDP;
	ptr->dgpu_ulv_power
		= atom_ppm_table->ulDGpuUlvPower;
	ptr->tj_max
		= atom_ppm_table->ulTjmax;

	pp_table_information->ppm_parameter_table = ptr;

	return 0;
}

/**
 * Private Function used during initialization.
 * Initialize TDP limits for DPM2
 * @param hwmgr Pointer to the hardware manager.
 * @param powerplay_table Pointer to the PowerPlay Table.
 */
static int init_dpm_2_parameters(
		struct pp_hwmgr *hwmgr,
		const ATOM_Tonga_POWERPLAYTABLE *powerplay_table
		)
{
	int result = 0;
	struct phm_ppt_v1_information *pp_table_information = (struct phm_ppt_v1_information *)(hwmgr->pptable);
	ATOM_Tonga_PPM_Table *atom_ppm_table;
	uint32_t disable_ppm = 0;
	uint32_t disable_power_control = 0;

	pp_table_information->us_ulv_voltage_offset =
		le16_to_cpu(powerplay_table->usUlvVoltageOffset);

	pp_table_information->ppm_parameter_table = NULL;
	pp_table_information->vddc_lookup_table = NULL;
	pp_table_information->vddgfx_lookup_table = NULL;
	/* TDP limits */
	hwmgr->platform_descriptor.TDPODLimit =
		le16_to_cpu(powerplay_table->usPowerControlLimit);
	hwmgr->platform_descriptor.TDPAdjustment = 0;
	hwmgr->platform_descriptor.VidAdjustment = 0;
	hwmgr->platform_descriptor.VidAdjustmentPolarity = 0;
	hwmgr->platform_descriptor.VidMinLimit = 0;
	hwmgr->platform_descriptor.VidMaxLimit = 1500000;
	hwmgr->platform_descriptor.VidStep = 6250;

	disable_power_control = 0;
	if (0 == disable_power_control) {
		/* enable TDP overdrive (PowerControl) feature as well if supported */
		if (hwmgr->platform_descriptor.TDPODLimit != 0)
			phm_cap_set(hwmgr->platform_descriptor.platformCaps,
			PHM_PlatformCaps_PowerControl);
	}

	if (0 != powerplay_table->usVddcLookupTableOffset) {
		const ATOM_Tonga_Voltage_Lookup_Table *pVddcCACTable =
			(ATOM_Tonga_Voltage_Lookup_Table *)(((unsigned long)powerplay_table) +
			le16_to_cpu(powerplay_table->usVddcLookupTableOffset));

		result = get_vddc_lookup_table(hwmgr,
			&pp_table_information->vddc_lookup_table, pVddcCACTable, 16);
	}

	if (0 != powerplay_table->usVddgfxLookupTableOffset) {
		const ATOM_Tonga_Voltage_Lookup_Table *pVddgfxCACTable =
			(ATOM_Tonga_Voltage_Lookup_Table *)(((unsigned long)powerplay_table) +
			le16_to_cpu(powerplay_table->usVddgfxLookupTableOffset));

		result = get_vddc_lookup_table(hwmgr,
			&pp_table_information->vddgfx_lookup_table, pVddgfxCACTable, 16);
	}

	disable_ppm = 0;
	if (0 == disable_ppm) {
		atom_ppm_table = (ATOM_Tonga_PPM_Table *)
			(((unsigned long)powerplay_table) + le16_to_cpu(powerplay_table->usPPMTableOffset));

		if (0 != powerplay_table->usPPMTableOffset) {
			if (1 == get_platform_power_management_table(hwmgr, atom_ppm_table)) {
				phm_cap_set(hwmgr->platform_descriptor.platformCaps,
					PHM_PlatformCaps_EnablePlatformPowerManagement);
			}
		}
	}

	return result;
}

static int get_valid_clk(
		struct pp_hwmgr *hwmgr,
		struct phm_clock_array **clk_table,
		const phm_ppt_v1_clock_voltage_dependency_table  * clk_volt_pp_table
		)
{
	uint32_t table_size, i;
	struct phm_clock_array *table;

	PP_ASSERT_WITH_CODE((0 != clk_volt_pp_table->count),
		"Invalid PowerPlay Table!", return -1);

	table_size = sizeof(uint32_t) +
		sizeof(uint32_t) * clk_volt_pp_table->count;

	table = (struct phm_clock_array *)kzalloc(table_size, GFP_KERNEL);

	if (NULL == table)
		return -ENOMEM;

	memset(table, 0x00, table_size);

	table->count = (uint32_t)clk_volt_pp_table->count;

	for (i = 0; i < table->count; i++)
		table->values[i] = (uint32_t)clk_volt_pp_table->entries[i].clk;

	*clk_table = table;

	return 0;
}

static int get_hard_limits(
		struct pp_hwmgr *hwmgr,
		struct phm_clock_and_voltage_limits *limits,
		const ATOM_Tonga_Hard_Limit_Table * limitable
		)
{
	PP_ASSERT_WITH_CODE((0 != limitable->ucNumEntries), "Invalid PowerPlay Table!", return -1);

	/* currently we always take entries[0] parameters */
	limits->sclk = (uint32_t)limitable->entries[0].ulSCLKLimit;
	limits->mclk = (uint32_t)limitable->entries[0].ulMCLKLimit;
	limits->vddc = (uint16_t)limitable->entries[0].usVddcLimit;
	limits->vddci = (uint16_t)limitable->entries[0].usVddciLimit;
	limits->vddgfx = (uint16_t)limitable->entries[0].usVddgfxLimit;

	return 0;
}

static int get_mclk_voltage_dependency_table(
		struct pp_hwmgr *hwmgr,
		phm_ppt_v1_clock_voltage_dependency_table **pp_tonga_mclk_dep_table,
		const ATOM_Tonga_MCLK_Dependency_Table * mclk_dep_table
		)
{
	uint32_t table_size, i;
	phm_ppt_v1_clock_voltage_dependency_table *mclk_table;

	PP_ASSERT_WITH_CODE((0 != mclk_dep_table->ucNumEntries),
		"Invalid PowerPlay Table!", return -1);

	table_size = sizeof(uint32_t) + sizeof(phm_ppt_v1_clock_voltage_dependency_record)
		* mclk_dep_table->ucNumEntries;

	mclk_table = (phm_ppt_v1_clock_voltage_dependency_table *)
		kzalloc(table_size, GFP_KERNEL);

	if (NULL == mclk_table)
		return -ENOMEM;

	memset(mclk_table, 0x00, table_size);

	mclk_table->count = (uint32_t)mclk_dep_table->ucNumEntries;

	for (i = 0; i < mclk_dep_table->ucNumEntries; i++) {
		mclk_table->entries[i].vddInd =
			mclk_dep_table->entries[i].ucVddcInd;
		mclk_table->entries[i].vdd_offset =
			mclk_dep_table->entries[i].usVddgfxOffset;
		mclk_table->entries[i].vddci =
			mclk_dep_table->entries[i].usVddci;
		mclk_table->entries[i].mvdd =
			mclk_dep_table->entries[i].usMvdd;
		mclk_table->entries[i].clk =
			mclk_dep_table->entries[i].ulMclk;
	}

	*pp_tonga_mclk_dep_table = mclk_table;

	return 0;
}

static int get_sclk_voltage_dependency_table(
		struct pp_hwmgr *hwmgr,
		phm_ppt_v1_clock_voltage_dependency_table **pp_tonga_sclk_dep_table,
		const ATOM_Tonga_SCLK_Dependency_Table * sclk_dep_table
		)
{
	uint32_t table_size, i;
	phm_ppt_v1_clock_voltage_dependency_table *sclk_table;

	PP_ASSERT_WITH_CODE((0 != sclk_dep_table->ucNumEntries),
		"Invalid PowerPlay Table!", return -1);

	table_size = sizeof(uint32_t) + sizeof(phm_ppt_v1_clock_voltage_dependency_record)
		* sclk_dep_table->ucNumEntries;

	sclk_table = (phm_ppt_v1_clock_voltage_dependency_table *)
		kzalloc(table_size, GFP_KERNEL);

	if (NULL == sclk_table)
		return -ENOMEM;

	memset(sclk_table, 0x00, table_size);

	sclk_table->count = (uint32_t)sclk_dep_table->ucNumEntries;

	for (i = 0; i < sclk_dep_table->ucNumEntries; i++) {
		sclk_table->entries[i].vddInd =
			sclk_dep_table->entries[i].ucVddInd;
		sclk_table->entries[i].vdd_offset =
			sclk_dep_table->entries[i].usVddcOffset;
		sclk_table->entries[i].clk =
			sclk_dep_table->entries[i].ulSclk;
		sclk_table->entries[i].cks_enable =
			(((sclk_dep_table->entries[i].ucCKSVOffsetandDisable & 0x80) >> 7) == 0) ? 1 : 0;
		sclk_table->entries[i].cks_voffset =
			(sclk_dep_table->entries[i].ucCKSVOffsetandDisable & 0x7F);
	}

	*pp_tonga_sclk_dep_table = sclk_table;

	return 0;
}

static int get_pcie_table(
		struct pp_hwmgr *hwmgr,
		phm_ppt_v1_pcie_table **pp_tonga_pcie_table,
		const ATOM_Tonga_PCIE_Table * atom_pcie_table
		)
{
	uint32_t table_size, i, pcie_count;
	phm_ppt_v1_pcie_table *pcie_table;
	struct phm_ppt_v1_information *pp_table_information =
		(struct phm_ppt_v1_information *)(hwmgr->pptable);
	PP_ASSERT_WITH_CODE((0 != atom_pcie_table->ucNumEntries),
		"Invalid PowerPlay Table!", return -1);

	table_size = sizeof(uint32_t) +
		sizeof(phm_ppt_v1_pcie_record) * atom_pcie_table->ucNumEntries;

	pcie_table = (phm_ppt_v1_pcie_table *)kzalloc(table_size, GFP_KERNEL);

	if (NULL == pcie_table)
		return -ENOMEM;

	memset(pcie_table, 0x00, table_size);

	/*
	* Make sure the number of pcie entries are less than or equal to sclk dpm levels.
	* Since first PCIE entry is for ULV, #pcie has to be <= SclkLevel + 1.
	*/
	pcie_count = (pp_table_information->vdd_dep_on_sclk->count) + 1;
	if ((uint32_t)atom_pcie_table->ucNumEntries <= pcie_count)
		pcie_count = (uint32_t)atom_pcie_table->ucNumEntries;
	else
		printk(KERN_ERR "[ powerplay ] Number of Pcie Entries exceed the number of SCLK Dpm Levels! \
		Disregarding the excess entries... \n");

	pcie_table->count = pcie_count;

	for (i = 0; i < pcie_count; i++) {
		pcie_table->entries[i].gen_speed =
			atom_pcie_table->entries[i].ucPCIEGenSpeed;
		pcie_table->entries[i].lane_width =
			atom_pcie_table->entries[i].usPCIELaneWidth;
	}

	*pp_tonga_pcie_table = pcie_table;

	return 0;
}

static int get_cac_tdp_table(
		struct pp_hwmgr *hwmgr,
		struct phm_cac_tdp_table **cac_tdp_table,
		const PPTable_Generic_SubTable_Header * table
		)
{
	uint32_t table_size;
	struct phm_cac_tdp_table *tdp_table;

	table_size = sizeof(uint32_t) + sizeof(struct phm_cac_tdp_table);
	tdp_table = kzalloc(table_size, GFP_KERNEL);

	if (NULL == tdp_table)
		return -ENOMEM;

	memset(tdp_table, 0x00, table_size);

	hwmgr->dyn_state.cac_dtp_table = kzalloc(table_size, GFP_KERNEL);

	if (NULL == hwmgr->dyn_state.cac_dtp_table)
		return -ENOMEM;

	memset(hwmgr->dyn_state.cac_dtp_table, 0x00, table_size);

	if (table->ucRevId < 3) {
		const ATOM_Tonga_PowerTune_Table *tonga_table =
			(ATOM_Tonga_PowerTune_Table *)table;
		tdp_table->usTDP = tonga_table->usTDP;
		tdp_table->usConfigurableTDP =
			tonga_table->usConfigurableTDP;
		tdp_table->usTDC = tonga_table->usTDC;
		tdp_table->usBatteryPowerLimit =
			tonga_table->usBatteryPowerLimit;
		tdp_table->usSmallPowerLimit =
			tonga_table->usSmallPowerLimit;
		tdp_table->usLowCACLeakage =
			tonga_table->usLowCACLeakage;
		tdp_table->usHighCACLeakage =
			tonga_table->usHighCACLeakage;
		tdp_table->usMaximumPowerDeliveryLimit =
			tonga_table->usMaximumPowerDeliveryLimit;
		tdp_table->usDefaultTargetOperatingTemp =
			tonga_table->usTjMax;
		tdp_table->usTargetOperatingTemp =
			tonga_table->usTjMax; /*Set the initial temp to the same as default */
		tdp_table->usPowerTuneDataSetID =
			tonga_table->usPowerTuneDataSetID;
		tdp_table->usSoftwareShutdownTemp =
			tonga_table->usSoftwareShutdownTemp;
		tdp_table->usClockStretchAmount =
			tonga_table->usClockStretchAmount;
	} else {   /* Fiji and newer */
		const ATOM_Fiji_PowerTune_Table *fijitable =
			(ATOM_Fiji_PowerTune_Table *)table;
		tdp_table->usTDP = fijitable->usTDP;
		tdp_table->usConfigurableTDP = fijitable->usConfigurableTDP;
		tdp_table->usTDC = fijitable->usTDC;
		tdp_table->usBatteryPowerLimit = fijitable->usBatteryPowerLimit;
		tdp_table->usSmallPowerLimit = fijitable->usSmallPowerLimit;
		tdp_table->usLowCACLeakage = fijitable->usLowCACLeakage;
		tdp_table->usHighCACLeakage = fijitable->usHighCACLeakage;
		tdp_table->usMaximumPowerDeliveryLimit =
			fijitable->usMaximumPowerDeliveryLimit;
		tdp_table->usDefaultTargetOperatingTemp =
			fijitable->usTjMax;
		tdp_table->usTargetOperatingTemp =
			fijitable->usTjMax; /*Set the initial temp to the same as default */
		tdp_table->usPowerTuneDataSetID =
			fijitable->usPowerTuneDataSetID;
		tdp_table->usSoftwareShutdownTemp =
			fijitable->usSoftwareShutdownTemp;
		tdp_table->usClockStretchAmount =
			fijitable->usClockStretchAmount;
		tdp_table->usTemperatureLimitHotspot =
			fijitable->usTemperatureLimitHotspot;
		tdp_table->usTemperatureLimitLiquid1 =
			fijitable->usTemperatureLimitLiquid1;
		tdp_table->usTemperatureLimitLiquid2 =
			fijitable->usTemperatureLimitLiquid2;
		tdp_table->usTemperatureLimitVrVddc =
			fijitable->usTemperatureLimitVrVddc;
		tdp_table->usTemperatureLimitVrMvdd =
			fijitable->usTemperatureLimitVrMvdd;
		tdp_table->usTemperatureLimitPlx =
			fijitable->usTemperatureLimitPlx;
		tdp_table->ucLiquid1_I2C_address =
			fijitable->ucLiquid1_I2C_address;
		tdp_table->ucLiquid2_I2C_address =
			fijitable->ucLiquid2_I2C_address;
		tdp_table->ucLiquid_I2C_Line =
			fijitable->ucLiquid_I2C_Line;
		tdp_table->ucVr_I2C_address = fijitable->ucVr_I2C_address;
		tdp_table->ucVr_I2C_Line = fijitable->ucVr_I2C_Line;
		tdp_table->ucPlx_I2C_address = fijitable->ucPlx_I2C_address;
		tdp_table->ucPlx_I2C_Line = fijitable->ucPlx_I2C_Line;
	}

	*cac_tdp_table = tdp_table;

	return 0;
}

static int get_mm_clock_voltage_table(
		struct pp_hwmgr *hwmgr,
		phm_ppt_v1_mm_clock_voltage_dependency_table **tonga_mm_table,
		const ATOM_Tonga_MM_Dependency_Table * mm_dependency_table
		)
{
	uint32_t table_size, i;
	const ATOM_Tonga_MM_Dependency_Record *mm_dependency_record;
	phm_ppt_v1_mm_clock_voltage_dependency_table *mm_table;

	PP_ASSERT_WITH_CODE((0 != mm_dependency_table->ucNumEntries),
		"Invalid PowerPlay Table!", return -1);
	table_size = sizeof(uint32_t) +
		sizeof(phm_ppt_v1_mm_clock_voltage_dependency_record)
		* mm_dependency_table->ucNumEntries;
	mm_table = (phm_ppt_v1_mm_clock_voltage_dependency_table *)
		kzalloc(table_size, GFP_KERNEL);

	if (NULL == mm_table)
		return -ENOMEM;

	memset(mm_table, 0x00, table_size);

	mm_table->count = mm_dependency_table->ucNumEntries;

	for (i = 0; i < mm_dependency_table->ucNumEntries; i++) {
		mm_dependency_record = &mm_dependency_table->entries[i];
		mm_table->entries[i].vddcInd = mm_dependency_record->ucVddcInd;
		mm_table->entries[i].vddgfx_offset = mm_dependency_record->usVddgfxOffset;
		mm_table->entries[i].aclk = mm_dependency_record->ulAClk;
		mm_table->entries[i].samclock = mm_dependency_record->ulSAMUClk;
		mm_table->entries[i].eclk = mm_dependency_record->ulEClk;
		mm_table->entries[i].vclk = mm_dependency_record->ulVClk;
		mm_table->entries[i].dclk = mm_dependency_record->ulDClk;
	}

	*tonga_mm_table = mm_table;

	return 0;
}

/**
 * Private Function used during initialization.
 * Initialize clock voltage dependency
 * @param hwmgr Pointer to the hardware manager.
 * @param powerplay_table Pointer to the PowerPlay Table.
 */
static int init_clock_voltage_dependency(
		struct pp_hwmgr *hwmgr,
		const ATOM_Tonga_POWERPLAYTABLE *powerplay_table
		)
{
	int result = 0;
	struct phm_ppt_v1_information *pp_table_information =
		(struct phm_ppt_v1_information *)(hwmgr->pptable);

	const ATOM_Tonga_MM_Dependency_Table *mm_dependency_table =
		(const ATOM_Tonga_MM_Dependency_Table *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usMMDependencyTableOffset));
	const PPTable_Generic_SubTable_Header *pPowerTuneTable =
		(const PPTable_Generic_SubTable_Header *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usPowerTuneTableOffset));
	const ATOM_Tonga_MCLK_Dependency_Table *mclk_dep_table =
		(const ATOM_Tonga_MCLK_Dependency_Table *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usMclkDependencyTableOffset));
	const ATOM_Tonga_SCLK_Dependency_Table *sclk_dep_table =
		(const ATOM_Tonga_SCLK_Dependency_Table *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usSclkDependencyTableOffset));
	const ATOM_Tonga_Hard_Limit_Table *pHardLimits =
		(const ATOM_Tonga_Hard_Limit_Table *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usHardLimitTableOffset));
	const ATOM_Tonga_PCIE_Table *pcie_table =
		(const ATOM_Tonga_PCIE_Table *)(((unsigned long) powerplay_table) +
		le16_to_cpu(powerplay_table->usPCIETableOffset));

	pp_table_information->vdd_dep_on_sclk = NULL;
	pp_table_information->vdd_dep_on_mclk = NULL;
	pp_table_information->mm_dep_table = NULL;
	pp_table_information->pcie_table = NULL;

	if (powerplay_table->usMMDependencyTableOffset != 0)
		result = get_mm_clock_voltage_table(hwmgr,
		&pp_table_information->mm_dep_table, mm_dependency_table);

	if (result == 0 && powerplay_table->usPowerTuneTableOffset != 0)
		result = get_cac_tdp_table(hwmgr,
		&pp_table_information->cac_dtp_table, pPowerTuneTable);

	if (result == 0 && powerplay_table->usSclkDependencyTableOffset != 0)
		result = get_sclk_voltage_dependency_table(hwmgr,
		&pp_table_information->vdd_dep_on_sclk, sclk_dep_table);

	if (result == 0 && powerplay_table->usMclkDependencyTableOffset != 0)
		result = get_mclk_voltage_dependency_table(hwmgr,
		&pp_table_information->vdd_dep_on_mclk, mclk_dep_table);

	if (result == 0 && powerplay_table->usPCIETableOffset != 0)
		result = get_pcie_table(hwmgr,
		&pp_table_information->pcie_table, pcie_table);

	if (result == 0 && powerplay_table->usHardLimitTableOffset != 0)
		result = get_hard_limits(hwmgr,
		&pp_table_information->max_clock_voltage_on_dc, pHardLimits);

	hwmgr->dyn_state.max_clock_voltage_on_dc.sclk =
		pp_table_information->max_clock_voltage_on_dc.sclk;
	hwmgr->dyn_state.max_clock_voltage_on_dc.mclk =
		pp_table_information->max_clock_voltage_on_dc.mclk;
	hwmgr->dyn_state.max_clock_voltage_on_dc.vddc =
		pp_table_information->max_clock_voltage_on_dc.vddc;
	hwmgr->dyn_state.max_clock_voltage_on_dc.vddci =
		pp_table_information->max_clock_voltage_on_dc.vddci;

	if (result == 0 && (NULL != pp_table_information->vdd_dep_on_mclk)
		&& (0 != pp_table_information->vdd_dep_on_mclk->count))
		result = get_valid_clk(hwmgr, &pp_table_information->valid_mclk_values,
		pp_table_information->vdd_dep_on_mclk);

	if (result == 0 && (NULL != pp_table_information->vdd_dep_on_sclk)
		&& (0 != pp_table_information->vdd_dep_on_sclk->count))
		result = get_valid_clk(hwmgr, &pp_table_information->valid_sclk_values,
		pp_table_information->vdd_dep_on_sclk);

	return result;
}

/** Retrieves the (signed) Overdrive limits from VBIOS.
 * The max engine clock, memory clock and max temperature come from the firmware info table.
 *
 * The information is placed into the platform descriptor.
 *
 * @param hwmgr source of the VBIOS table and owner of the platform descriptor to be updated.
 * @param powerplay_table the address of the PowerPlay table.
 *
 * @return 1 as long as the firmware info table was present and of a supported version.
 */
static int init_over_drive_limits(
		struct pp_hwmgr *hwmgr,
		const ATOM_Tonga_POWERPLAYTABLE *powerplay_table)
{
	hwmgr->platform_descriptor.overdriveLimit.engineClock =
		le16_to_cpu(powerplay_table->ulMaxODEngineClock);
	hwmgr->platform_descriptor.overdriveLimit.memoryClock =
		le16_to_cpu(powerplay_table->ulMaxODMemoryClock);

	hwmgr->platform_descriptor.minOverdriveVDDC = 0;
	hwmgr->platform_descriptor.maxOverdriveVDDC = 0;
	hwmgr->platform_descriptor.overdriveVDDCStep = 0;

	if (hwmgr->platform_descriptor.overdriveLimit.engineClock > 0 \
		&& hwmgr->platform_descriptor.overdriveLimit.memoryClock > 0) {
		phm_cap_set(hwmgr->platform_descriptor.platformCaps,
			PHM_PlatformCaps_ACOverdriveSupport);
	}

	return 0;
}

/**
 * Private Function used during initialization.
 * Inspect the PowerPlay table for obvious signs of corruption.
 * @param hwmgr Pointer to the hardware manager.
 * @param powerplay_table Pointer to the PowerPlay Table.
 * @exception This implementation always returns 1.
 */
static int init_thermal_controller(
		struct pp_hwmgr *hwmgr,
		const ATOM_Tonga_POWERPLAYTABLE *powerplay_table
		)
{
	const PPTable_Generic_SubTable_Header *fan_table;
	ATOM_Tonga_Thermal_Controller *thermal_controller;

	thermal_controller = (ATOM_Tonga_Thermal_Controller *)
		(((unsigned long)powerplay_table) +
		le16_to_cpu(powerplay_table->usThermalControllerOffset));
	PP_ASSERT_WITH_CODE((0 != powerplay_table->usThermalControllerOffset),
		"Thermal controller table not set!", return -1);

	hwmgr->thermal_controller.ucType = thermal_controller->ucType;
	hwmgr->thermal_controller.ucI2cLine = thermal_controller->ucI2cLine;
	hwmgr->thermal_controller.ucI2cAddress = thermal_controller->ucI2cAddress;

	hwmgr->thermal_controller.fanInfo.bNoFan =
		(0 != (thermal_controller->ucFanParameters & ATOM_TONGA_PP_FANPARAMETERS_NOFAN));

	hwmgr->thermal_controller.fanInfo.ucTachometerPulsesPerRevolution =
		thermal_controller->ucFanParameters &
		ATOM_TONGA_PP_FANPARAMETERS_TACHOMETER_PULSES_PER_REVOLUTION_MASK;

	hwmgr->thermal_controller.fanInfo.ulMinRPM
		= thermal_controller->ucFanMinRPM * 100UL;
	hwmgr->thermal_controller.fanInfo.ulMaxRPM
		= thermal_controller->ucFanMaxRPM * 100UL;

	set_hw_cap(
			hwmgr,
			ATOM_TONGA_PP_THERMALCONTROLLER_NONE != hwmgr->thermal_controller.ucType,
			PHM_PlatformCaps_ThermalController
		  );

	if (0 == powerplay_table->usFanTableOffset)
		return 0;

	fan_table = (const PPTable_Generic_SubTable_Header *)
		(((unsigned long)powerplay_table) +
		le16_to_cpu(powerplay_table->usFanTableOffset));

	PP_ASSERT_WITH_CODE((0 != powerplay_table->usFanTableOffset),
		"Fan table not set!", return -1);
	PP_ASSERT_WITH_CODE((0 < fan_table->ucRevId),
		"Unsupported fan table format!", return -1);

	hwmgr->thermal_controller.advanceFanControlParameters.ulCycleDelay
		= 100000;
	phm_cap_set(hwmgr->platform_descriptor.platformCaps,
		PHM_PlatformCaps_MicrocodeFanControl);

	if (fan_table->ucRevId < 8) {
		const ATOM_Tonga_Fan_Table *tonga_fan_table =
			(ATOM_Tonga_Fan_Table *)fan_table;
		hwmgr->thermal_controller.advanceFanControlParameters.ucTHyst
			= tonga_fan_table->ucTHyst;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMin
			= tonga_fan_table->usTMin;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMed
			= tonga_fan_table->usTMed;
		hwmgr->thermal_controller.advanceFanControlParameters.usTHigh
			= tonga_fan_table->usTHigh;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMMin
			= tonga_fan_table->usPWMMin;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMMed
			= tonga_fan_table->usPWMMed;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMHigh
			= tonga_fan_table->usPWMHigh;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMax
			= 10900;                  /* hard coded */
		hwmgr->thermal_controller.advanceFanControlParameters.usTMax
			= tonga_fan_table->usTMax;
		hwmgr->thermal_controller.advanceFanControlParameters.ucFanControlMode
			= tonga_fan_table->ucFanControlMode;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultMaxFanPWM
			= tonga_fan_table->usFanPWMMax;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultFanOutputSensitivity
			= 4836;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanOutputSensitivity
			= tonga_fan_table->usFanOutputSensitivity;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultMaxFanRPM
			= tonga_fan_table->usFanRPMMax;
		hwmgr->thermal_controller.advanceFanControlParameters.ulMinFanSCLKAcousticLimit
			= (tonga_fan_table->ulMinFanSCLKAcousticLimit / 100); /* PPTable stores it in 10Khz unit for 2 decimal places.  SMC wants MHz. */
		hwmgr->thermal_controller.advanceFanControlParameters.ucTargetTemperature
			= tonga_fan_table->ucTargetTemperature;
		hwmgr->thermal_controller.advanceFanControlParameters.ucMinimumPWMLimit
			= tonga_fan_table->ucMinimumPWMLimit;
	} else {
		const ATOM_Fiji_Fan_Table *fiji_fan_table =
			(ATOM_Fiji_Fan_Table *)fan_table;
		hwmgr->thermal_controller.advanceFanControlParameters.ucTHyst
			= fiji_fan_table->ucTHyst;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMin
			= fiji_fan_table->usTMin;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMed
			= fiji_fan_table->usTMed;
		hwmgr->thermal_controller.advanceFanControlParameters.usTHigh
			= fiji_fan_table->usTHigh;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMMin
			= fiji_fan_table->usPWMMin;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMMed
			= fiji_fan_table->usPWMMed;
		hwmgr->thermal_controller.advanceFanControlParameters.usPWMHigh
			= fiji_fan_table->usPWMHigh;
		hwmgr->thermal_controller.advanceFanControlParameters.usTMax
			= fiji_fan_table->usTMax;
		hwmgr->thermal_controller.advanceFanControlParameters.ucFanControlMode
			= fiji_fan_table->ucFanControlMode;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultMaxFanPWM
			= fiji_fan_table->usFanPWMMax;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultFanOutputSensitivity
			= 4836;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanOutputSensitivity
			= fiji_fan_table->usFanOutputSensitivity;
		hwmgr->thermal_controller.advanceFanControlParameters.usDefaultMaxFanRPM
			= fiji_fan_table->usFanRPMMax;
		hwmgr->thermal_controller.advanceFanControlParameters.ulMinFanSCLKAcousticLimit
			= (fiji_fan_table->ulMinFanSCLKAcousticLimit / 100); /* PPTable stores it in 10Khz unit for 2 decimal places.  SMC wants MHz. */
		hwmgr->thermal_controller.advanceFanControlParameters.ucTargetTemperature
			= fiji_fan_table->ucTargetTemperature;
		hwmgr->thermal_controller.advanceFanControlParameters.ucMinimumPWMLimit
			= fiji_fan_table->ucMinimumPWMLimit;

		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainEdge
			= fiji_fan_table->usFanGainEdge;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainHotspot
			= fiji_fan_table->usFanGainHotspot;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainLiquid
			= fiji_fan_table->usFanGainLiquid;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainVrVddc
			= fiji_fan_table->usFanGainVrVddc;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainVrMvdd
			= fiji_fan_table->usFanGainVrMvdd;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainPlx
			= fiji_fan_table->usFanGainPlx;
		hwmgr->thermal_controller.advanceFanControlParameters.usFanGainHbm
			= fiji_fan_table->usFanGainHbm;
	}

	return 0;
}

/**
 * Private Function used during initialization.
 * Inspect the PowerPlay table for obvious signs of corruption.
 * @param hwmgr Pointer to the hardware manager.
 * @param powerplay_table Pointer to the PowerPlay Table.
 * @exception 2 if the powerplay table is incorrect.
 */
static int check_powerplay_tables(
		struct pp_hwmgr *hwmgr,
		const ATOM_Tonga_POWERPLAYTABLE *powerplay_table
		)
{
	const ATOM_Tonga_State_Array *state_arrays;

	state_arrays = (ATOM_Tonga_State_Array *)(((unsigned long)powerplay_table) +
		le16_to_cpu(powerplay_table->usStateArrayOffset));

	PP_ASSERT_WITH_CODE((ATOM_Tonga_TABLE_REVISION_TONGA <=
		powerplay_table->sHeader.ucTableFormatRevision),
		"Unsupported PPTable format!", return -1);
	PP_ASSERT_WITH_CODE((0 != powerplay_table->usStateArrayOffset),
		"State table is not set!", return -1);
	PP_ASSERT_WITH_CODE((0 < powerplay_table->sHeader.usStructureSize),
		"Invalid PowerPlay Table!", return -1);
	PP_ASSERT_WITH_CODE((0 < state_arrays->ucNumEntries),
		"Invalid PowerPlay Table!", return -1);

	return 0;
}

int tonga_pp_tables_initialize(struct pp_hwmgr *hwmgr)
{
	int result = 0;
	const ATOM_Tonga_POWERPLAYTABLE *powerplay_table;

	hwmgr->pptable = kzalloc(sizeof(struct phm_ppt_v1_information), GFP_KERNEL);

	PP_ASSERT_WITH_CODE((NULL != hwmgr->pptable),
			    "Failed to allocate hwmgr->pptable!", return -ENOMEM);

	memset(hwmgr->pptable, 0x00, sizeof(struct phm_ppt_v1_information));

	powerplay_table = get_powerplay_table(hwmgr);

	PP_ASSERT_WITH_CODE((NULL != powerplay_table),
		"Missing PowerPlay Table!", return -1);

	result = check_powerplay_tables(hwmgr, powerplay_table);

	PP_ASSERT_WITH_CODE((result == 0),
			    "check_powerplay_tables failed", return result);

	result = set_platform_caps(hwmgr,
				   le32_to_cpu(powerplay_table->ulPlatformCaps));

	PP_ASSERT_WITH_CODE((result == 0),
			    "set_platform_caps failed", return result);

	result = init_thermal_controller(hwmgr, powerplay_table);

	PP_ASSERT_WITH_CODE((result == 0),
			    "init_thermal_controller failed", return result);

	result = init_over_drive_limits(hwmgr, powerplay_table);

	PP_ASSERT_WITH_CODE((result == 0),
			    "init_over_drive_limits failed", return result);

	result = init_clock_voltage_dependency(hwmgr, powerplay_table);

	PP_ASSERT_WITH_CODE((result == 0),
			    "init_clock_voltage_dependency failed", return result);

	result = init_dpm_2_parameters(hwmgr, powerplay_table);

	PP_ASSERT_WITH_CODE((result == 0),
			    "init_dpm_2_parameters failed", return result);

	return result;
}

int tonga_pp_tables_uninitialize(struct pp_hwmgr *hwmgr)
{
	int result = 0;
	struct phm_ppt_v1_information *pp_table_information =
		(struct phm_ppt_v1_information *)(hwmgr->pptable);

	if (NULL != hwmgr->soft_pp_table) {
		kfree(hwmgr->soft_pp_table);
		hwmgr->soft_pp_table = NULL;
	}

	if (NULL != pp_table_information->vdd_dep_on_sclk)
		pp_table_information->vdd_dep_on_sclk = NULL;

	if (NULL != pp_table_information->vdd_dep_on_mclk)
		pp_table_information->vdd_dep_on_mclk = NULL;

	if (NULL != pp_table_information->valid_mclk_values)
		pp_table_information->valid_mclk_values = NULL;

	if (NULL != pp_table_information->valid_sclk_values)
		pp_table_information->valid_sclk_values = NULL;

	if (NULL != pp_table_information->vddc_lookup_table)
		pp_table_information->vddc_lookup_table = NULL;

	if (NULL != pp_table_information->vddgfx_lookup_table)
		pp_table_information->vddgfx_lookup_table = NULL;

	if (NULL != pp_table_information->mm_dep_table)
		pp_table_information->mm_dep_table = NULL;

	if (NULL != pp_table_information->cac_dtp_table)
		pp_table_information->cac_dtp_table = NULL;

	if (NULL != hwmgr->dyn_state.cac_dtp_table)
		hwmgr->dyn_state.cac_dtp_table = NULL;

	if (NULL != pp_table_information->ppm_parameter_table)
		pp_table_information->ppm_parameter_table = NULL;

	if (NULL != pp_table_information->pcie_table)
		pp_table_information->pcie_table = NULL;

	if (NULL != hwmgr->pptable) {
		kfree(hwmgr->pptable);
		hwmgr->pptable = NULL;
	}

	return result;
}

const struct pp_table_func tonga_pptable_funcs = {
	.pptable_init = tonga_pp_tables_initialize,
	.pptable_fini = tonga_pp_tables_uninitialize,
};

int tonga_get_number_of_powerplay_table_entries(struct pp_hwmgr *hwmgr)
{
	const ATOM_Tonga_State_Array * state_arrays;
	const ATOM_Tonga_POWERPLAYTABLE *pp_table = get_powerplay_table(hwmgr);

	PP_ASSERT_WITH_CODE((NULL != pp_table),
			"Missing PowerPlay Table!", return -1);
	PP_ASSERT_WITH_CODE((pp_table->sHeader.ucTableFormatRevision >=
			ATOM_Tonga_TABLE_REVISION_TONGA),
			"Incorrect PowerPlay table revision!", return -1);

	state_arrays = (ATOM_Tonga_State_Array *)(((unsigned long)pp_table) +
			le16_to_cpu(pp_table->usStateArrayOffset));

	return (uint32_t)(state_arrays->ucNumEntries);
}

/**
* Private function to convert flags stored in the BIOS to software flags in PowerPlay.
*/
static uint32_t make_classification_flags(struct pp_hwmgr *hwmgr,
		uint16_t classification, uint16_t classification2)
{
	uint32_t result = 0;

	if (classification & ATOM_PPLIB_CLASSIFICATION_BOOT)
		result |= PP_StateClassificationFlag_Boot;

	if (classification & ATOM_PPLIB_CLASSIFICATION_THERMAL)
		result |= PP_StateClassificationFlag_Thermal;

	if (classification & ATOM_PPLIB_CLASSIFICATION_LIMITEDPOWERSOURCE)
		result |= PP_StateClassificationFlag_LimitedPowerSource;

	if (classification & ATOM_PPLIB_CLASSIFICATION_REST)
		result |= PP_StateClassificationFlag_Rest;

	if (classification & ATOM_PPLIB_CLASSIFICATION_FORCED)
		result |= PP_StateClassificationFlag_Forced;

	if (classification & ATOM_PPLIB_CLASSIFICATION_ACPI)
		result |= PP_StateClassificationFlag_ACPI;

	if (classification2 & ATOM_PPLIB_CLASSIFICATION2_LIMITEDPOWERSOURCE_2)
		result |= PP_StateClassificationFlag_LimitedPowerSource_2;

	return result;
}

/**
* Create a Power State out of an entry in the PowerPlay table.
* This function is called by the hardware back-end.
* @param hwmgr Pointer to the hardware manager.
* @param entry_index The index of the entry to be extracted from the table.
* @param power_state The address of the PowerState instance being created.
* @return -1 if the entry cannot be retrieved.
*/
int tonga_get_powerplay_table_entry(struct pp_hwmgr *hwmgr,
		uint32_t entry_index, struct pp_power_state *power_state,
		int (*call_back_func)(struct pp_hwmgr *, void *,
				struct pp_power_state *, void *, uint32_t))
{
	int result = 0;
	const ATOM_Tonga_State_Array * state_arrays;
	const ATOM_Tonga_State *state_entry;
	const ATOM_Tonga_POWERPLAYTABLE *pp_table = get_powerplay_table(hwmgr);

	PP_ASSERT_WITH_CODE((NULL != pp_table), "Missing PowerPlay Table!", return -1;);
	power_state->classification.bios_index = entry_index;

	if (pp_table->sHeader.ucTableFormatRevision >=
			ATOM_Tonga_TABLE_REVISION_TONGA) {
		state_arrays = (ATOM_Tonga_State_Array *)(((unsigned long)pp_table) +
				le16_to_cpu(pp_table->usStateArrayOffset));

		PP_ASSERT_WITH_CODE((0 < pp_table->usStateArrayOffset),
				"Invalid PowerPlay Table State Array Offset.", return -1);
		PP_ASSERT_WITH_CODE((0 < state_arrays->ucNumEntries),
				"Invalid PowerPlay Table State Array.", return -1);
		PP_ASSERT_WITH_CODE((entry_index <= state_arrays->ucNumEntries),
				"Invalid PowerPlay Table State Array Entry.", return -1);

		state_entry = &(state_arrays->states[entry_index]);

		result = call_back_func(hwmgr, (void *)state_entry, power_state,
				(void *)pp_table,
				make_classification_flags(hwmgr,
					le16_to_cpu(state_entry->usClassification),
					le16_to_cpu(state_entry->usClassification2)));
	}

	if (!result && (power_state->classification.flags &
			PP_StateClassificationFlag_Boot))
		result = hwmgr->hwmgr_func->patch_boot_state(hwmgr, &(power_state->hardware));

	return result;
}
