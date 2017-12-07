/*
 * Copyright 2016 Advanced Micro Devices, Inc.
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

#include "ppatomfwctrl.h"
#include "atomfirmware.h"
#include "pp_debug.h"


static const union atom_voltage_object_v4 *pp_atomfwctrl_lookup_voltage_type_v4(
		const struct atom_voltage_objects_info_v4_1 *voltage_object_info_table,
		uint8_t voltage_type, uint8_t voltage_mode)
{
	unsigned int size = le16_to_cpu(
			voltage_object_info_table->table_header.structuresize);
	unsigned int offset =
			offsetof(struct atom_voltage_objects_info_v4_1, voltage_object[0]);
	unsigned long start = (unsigned long)voltage_object_info_table;

	while (offset < size) {
		const union atom_voltage_object_v4 *voltage_object =
				(const union atom_voltage_object_v4 *)(start + offset);

        if (voltage_type == voltage_object->gpio_voltage_obj.header.voltage_type &&
            voltage_mode == voltage_object->gpio_voltage_obj.header.voltage_mode)
            return voltage_object;

        offset += le16_to_cpu(voltage_object->gpio_voltage_obj.header.object_size);

    }

    return NULL;
}

static struct atom_voltage_objects_info_v4_1 *pp_atomfwctrl_get_voltage_info_table(
		struct pp_hwmgr *hwmgr)
{
    const void *table_address;
    uint16_t idx;

    idx = GetIndexIntoMasterDataTable(voltageobject_info);
    table_address =	cgs_atom_get_data_table(hwmgr->device,
    		idx, NULL, NULL, NULL);

    PP_ASSERT_WITH_CODE( 
        table_address,
        "Error retrieving BIOS Table Address!",
        return NULL);

    return (struct atom_voltage_objects_info_v4_1 *)table_address;
}

/**
* Returns TRUE if the given voltage type is controlled by GPIO pins.
* voltage_type is one of SET_VOLTAGE_TYPE_ASIC_VDDC, SET_VOLTAGE_TYPE_ASIC_MVDDC, SET_VOLTAGE_TYPE_ASIC_MVDDQ.
* voltage_mode is one of ATOM_SET_VOLTAGE, ATOM_SET_VOLTAGE_PHASE
*/
bool pp_atomfwctrl_is_voltage_controlled_by_gpio_v4(struct pp_hwmgr *hwmgr,
		uint8_t voltage_type, uint8_t voltage_mode)
{
	struct atom_voltage_objects_info_v4_1 *voltage_info =
			(struct atom_voltage_objects_info_v4_1 *)
			pp_atomfwctrl_get_voltage_info_table(hwmgr);
	bool ret;

	/* If we cannot find the table do NOT try to control this voltage. */
	PP_ASSERT_WITH_CODE(voltage_info,
			"Could not find Voltage Table in BIOS.",
			return false);

	ret = (pp_atomfwctrl_lookup_voltage_type_v4(voltage_info,
			voltage_type, voltage_mode)) ? true : false;

	return ret;
}

int pp_atomfwctrl_get_voltage_table_v4(struct pp_hwmgr *hwmgr,
		uint8_t voltage_type, uint8_t voltage_mode,
		struct pp_atomfwctrl_voltage_table *voltage_table)
{
	struct atom_voltage_objects_info_v4_1 *voltage_info =
			(struct atom_voltage_objects_info_v4_1 *)
			pp_atomfwctrl_get_voltage_info_table(hwmgr);
	const union atom_voltage_object_v4 *voltage_object;
	unsigned int i;
	int result = 0;

	PP_ASSERT_WITH_CODE(voltage_info,
			"Could not find Voltage Table in BIOS.",
			return -1);

	voltage_object = pp_atomfwctrl_lookup_voltage_type_v4(voltage_info,
			voltage_type, voltage_mode);

	if (!voltage_object)
		return -1;

	voltage_table->count = 0;
	if (voltage_mode == VOLTAGE_OBJ_GPIO_LUT) {
		PP_ASSERT_WITH_CODE(
				(voltage_object->gpio_voltage_obj.gpio_entry_num <=
				PP_ATOMFWCTRL_MAX_VOLTAGE_ENTRIES),
				"Too many voltage entries!",
				result = -1);

		if (!result) {
			for (i = 0; i < voltage_object->gpio_voltage_obj.
							gpio_entry_num; i++) {
				voltage_table->entries[i].value =
						le16_to_cpu(voltage_object->gpio_voltage_obj.
						voltage_gpio_lut[i].voltage_level_mv);
				voltage_table->entries[i].smio_low =
						le32_to_cpu(voltage_object->gpio_voltage_obj.
						voltage_gpio_lut[i].voltage_gpio_reg_val);
			}
			voltage_table->count =
					voltage_object->gpio_voltage_obj.gpio_entry_num;
			voltage_table->mask_low =
					le32_to_cpu(
					voltage_object->gpio_voltage_obj.gpio_mask_val);
			voltage_table->phase_delay =
					voltage_object->gpio_voltage_obj.phase_delay_us;
		}
	} else if (voltage_mode == VOLTAGE_OBJ_SVID2) {
		voltage_table->psi1_enable =
			(voltage_object->svid2_voltage_obj.loadline_psi1 & 0x20) >> 5;
		voltage_table->psi0_enable =
			voltage_object->svid2_voltage_obj.psi0_enable & 0x1;
		voltage_table->max_vid_step =
			voltage_object->svid2_voltage_obj.maxvstep;
		voltage_table->telemetry_offset =
			voltage_object->svid2_voltage_obj.telemetry_offset;
		voltage_table->telemetry_slope =
			voltage_object->svid2_voltage_obj.telemetry_gain;
	} else
		PP_ASSERT_WITH_CODE(false,
				"Unsupported Voltage Object Mode!",
				result = -1);

	return result;
}

 
static struct atom_gpio_pin_lut_v2_1 *pp_atomfwctrl_get_gpio_lookup_table(
		struct pp_hwmgr *hwmgr)
{
	const void *table_address;
	uint16_t idx;

	idx = GetIndexIntoMasterDataTable(gpio_pin_lut);
	table_address =	cgs_atom_get_data_table(hwmgr->device,
			idx, NULL, NULL, NULL);
	PP_ASSERT_WITH_CODE(table_address,
			"Error retrieving BIOS Table Address!",
			return NULL);

	return (struct atom_gpio_pin_lut_v2_1 *)table_address;
}

static bool pp_atomfwctrl_lookup_gpio_pin(
		struct atom_gpio_pin_lut_v2_1 *gpio_lookup_table,
		const uint32_t pin_id,
		struct pp_atomfwctrl_gpio_pin_assignment *gpio_pin_assignment)
{
	unsigned int size = le16_to_cpu(
			gpio_lookup_table->table_header.structuresize);
	unsigned int offset =
			offsetof(struct atom_gpio_pin_lut_v2_1, gpio_pin[0]);
	unsigned long start = (unsigned long)gpio_lookup_table;

	while (offset < size) {
		const struct  atom_gpio_pin_assignment *pin_assignment =
				(const struct  atom_gpio_pin_assignment *)(start + offset);

		if (pin_id == pin_assignment->gpio_id)  {
			gpio_pin_assignment->uc_gpio_pin_bit_shift =
					pin_assignment->gpio_bitshift;
			gpio_pin_assignment->us_gpio_pin_aindex =
					le16_to_cpu(pin_assignment->data_a_reg_index);
			return true;
		}
		offset += offsetof(struct atom_gpio_pin_assignment, gpio_id) + 1;
	}
	return false;
}

/**
* Returns TRUE if the given pin id find in lookup table.
*/
bool pp_atomfwctrl_get_pp_assign_pin(struct pp_hwmgr *hwmgr,
		const uint32_t pin_id,
		struct pp_atomfwctrl_gpio_pin_assignment *gpio_pin_assignment)
{
	bool ret = false;
	struct atom_gpio_pin_lut_v2_1 *gpio_lookup_table =
			pp_atomfwctrl_get_gpio_lookup_table(hwmgr);

	/* If we cannot find the table do NOT try to control this voltage. */
	PP_ASSERT_WITH_CODE(gpio_lookup_table,
			"Could not find GPIO lookup Table in BIOS.",
			return false);

	ret = pp_atomfwctrl_lookup_gpio_pin(gpio_lookup_table,
			pin_id, gpio_pin_assignment);

	return ret;
}

/**
* Enter to SelfRefresh mode.
* @param hwmgr
*/
int pp_atomfwctrl_enter_self_refresh(struct pp_hwmgr *hwmgr)
{
	/* 0 - no action
	 * 1 - leave power to video memory always on
	 */
	return 0;
}

/** pp_atomfwctrl_get_gpu_pll_dividers_vega10().
 *
 * @param hwmgr       input parameter: pointer to HwMgr
 * @param clock_type  input parameter: Clock type: 1 - GFXCLK, 2 - UCLK, 0 - All other clocks
 * @param clock_value input parameter: Clock
 * @param dividers    output parameter:Clock dividers
 */
int pp_atomfwctrl_get_gpu_pll_dividers_vega10(struct pp_hwmgr *hwmgr,
		uint32_t clock_type, uint32_t clock_value,
		struct pp_atomfwctrl_clock_dividers_soc15 *dividers)
{
	struct compute_gpu_clock_input_parameter_v1_8 pll_parameters;
	struct compute_gpu_clock_output_parameter_v1_8 *pll_output;
	int result;
	uint32_t idx;

	pll_parameters.gpuclock_10khz = (uint32_t)clock_value;
	pll_parameters.gpu_clock_type = clock_type;

	idx = GetIndexIntoMasterCmdTable(computegpuclockparam);
	result = cgs_atom_exec_cmd_table(hwmgr->device, idx, &pll_parameters);

	if (!result) {
		pll_output = (struct compute_gpu_clock_output_parameter_v1_8 *)
				&pll_parameters;
		dividers->ulClock = le32_to_cpu(pll_output->gpuclock_10khz);
		dividers->ulDid = le32_to_cpu(pll_output->dfs_did);
		dividers->ulPll_fb_mult = le32_to_cpu(pll_output->pll_fb_mult);
		dividers->ulPll_ss_fbsmult = le32_to_cpu(pll_output->pll_ss_fbsmult);
		dividers->usPll_ss_slew_frac = le16_to_cpu(pll_output->pll_ss_slew_frac);
		dividers->ucPll_ss_enable = pll_output->pll_ss_enable;
	}
	return result;
}

int pp_atomfwctrl_get_avfs_information(struct pp_hwmgr *hwmgr,
		struct pp_atomfwctrl_avfs_parameters *param)
{
	uint16_t idx;
	uint8_t format_revision, content_revision;

	struct atom_asic_profiling_info_v4_1 *profile;
	struct atom_asic_profiling_info_v4_2 *profile_v4_2;

	idx = GetIndexIntoMasterDataTable(asic_profiling_info);
	profile = (struct atom_asic_profiling_info_v4_1 *)
			cgs_atom_get_data_table(hwmgr->device,
					idx, NULL, NULL, NULL);

	if (!profile)
		return -1;

	format_revision = ((struct atom_common_table_header *)profile)->format_revision;
	content_revision = ((struct atom_common_table_header *)profile)->content_revision;

	if (format_revision == 4 && content_revision == 1) {
		param->ulMaxVddc = le32_to_cpu(profile->maxvddc);
		param->ulMinVddc = le32_to_cpu(profile->minvddc);
		param->ulMeanNsigmaAcontant0 =
				le32_to_cpu(profile->avfs_meannsigma_acontant0);
		param->ulMeanNsigmaAcontant1 =
				le32_to_cpu(profile->avfs_meannsigma_acontant1);
		param->ulMeanNsigmaAcontant2 =
				le32_to_cpu(profile->avfs_meannsigma_acontant2);
		param->usMeanNsigmaDcTolSigma =
				le16_to_cpu(profile->avfs_meannsigma_dc_tol_sigma);
		param->usMeanNsigmaPlatformMean =
				le16_to_cpu(profile->avfs_meannsigma_platform_mean);
		param->usMeanNsigmaPlatformSigma =
				le16_to_cpu(profile->avfs_meannsigma_platform_sigma);
		param->ulGbVdroopTableCksoffA0 =
				le32_to_cpu(profile->gb_vdroop_table_cksoff_a0);
		param->ulGbVdroopTableCksoffA1 =
				le32_to_cpu(profile->gb_vdroop_table_cksoff_a1);
		param->ulGbVdroopTableCksoffA2 =
				le32_to_cpu(profile->gb_vdroop_table_cksoff_a2);
		param->ulGbVdroopTableCksonA0 =
				le32_to_cpu(profile->gb_vdroop_table_ckson_a0);
		param->ulGbVdroopTableCksonA1 =
				le32_to_cpu(profile->gb_vdroop_table_ckson_a1);
		param->ulGbVdroopTableCksonA2 =
				le32_to_cpu(profile->gb_vdroop_table_ckson_a2);
		param->ulGbFuseTableCksoffM1 =
				le32_to_cpu(profile->avfsgb_fuse_table_cksoff_m1);
		param->ulGbFuseTableCksoffM2 =
				le32_to_cpu(profile->avfsgb_fuse_table_cksoff_m2);
		param->ulGbFuseTableCksoffB =
				le32_to_cpu(profile->avfsgb_fuse_table_cksoff_b);
		param->ulGbFuseTableCksonM1 =
				le32_to_cpu(profile->avfsgb_fuse_table_ckson_m1);
		param->ulGbFuseTableCksonM2 =
				le32_to_cpu(profile->avfsgb_fuse_table_ckson_m2);
		param->ulGbFuseTableCksonB =
				le32_to_cpu(profile->avfsgb_fuse_table_ckson_b);

		param->ucEnableGbVdroopTableCkson =
				profile->enable_gb_vdroop_table_ckson;
		param->ucEnableGbFuseTableCkson =
				profile->enable_gb_fuse_table_ckson;
		param->usPsmAgeComfactor =
				le16_to_cpu(profile->psm_age_comfactor);

		param->ulDispclk2GfxclkM1 =
				le32_to_cpu(profile->dispclk2gfxclk_a);
		param->ulDispclk2GfxclkM2 =
				le32_to_cpu(profile->dispclk2gfxclk_b);
		param->ulDispclk2GfxclkB =
				le32_to_cpu(profile->dispclk2gfxclk_c);
		param->ulDcefclk2GfxclkM1 =
				le32_to_cpu(profile->dcefclk2gfxclk_a);
		param->ulDcefclk2GfxclkM2 =
				le32_to_cpu(profile->dcefclk2gfxclk_b);
		param->ulDcefclk2GfxclkB =
				le32_to_cpu(profile->dcefclk2gfxclk_c);
		param->ulPixelclk2GfxclkM1 =
				le32_to_cpu(profile->pixclk2gfxclk_a);
		param->ulPixelclk2GfxclkM2 =
				le32_to_cpu(profile->pixclk2gfxclk_b);
		param->ulPixelclk2GfxclkB =
				le32_to_cpu(profile->pixclk2gfxclk_c);
		param->ulPhyclk2GfxclkM1 =
				le32_to_cpu(profile->phyclk2gfxclk_a);
		param->ulPhyclk2GfxclkM2 =
				le32_to_cpu(profile->phyclk2gfxclk_b);
		param->ulPhyclk2GfxclkB =
				le32_to_cpu(profile->phyclk2gfxclk_c);
		param->ulAcgGbVdroopTableA0           = 0;
		param->ulAcgGbVdroopTableA1           = 0;
		param->ulAcgGbVdroopTableA2           = 0;
		param->ulAcgGbFuseTableM1             = 0;
		param->ulAcgGbFuseTableM2             = 0;
		param->ulAcgGbFuseTableB              = 0;
		param->ucAcgEnableGbVdroopTable       = 0;
		param->ucAcgEnableGbFuseTable         = 0;
	} else if (format_revision == 4 && content_revision == 2) {
		profile_v4_2 = (struct atom_asic_profiling_info_v4_2 *)profile;
		param->ulMaxVddc = le32_to_cpu(profile_v4_2->maxvddc);
		param->ulMinVddc = le32_to_cpu(profile_v4_2->minvddc);
		param->ulMeanNsigmaAcontant0 =
				le32_to_cpu(profile_v4_2->avfs_meannsigma_acontant0);
		param->ulMeanNsigmaAcontant1 =
				le32_to_cpu(profile_v4_2->avfs_meannsigma_acontant1);
		param->ulMeanNsigmaAcontant2 =
				le32_to_cpu(profile_v4_2->avfs_meannsigma_acontant2);
		param->usMeanNsigmaDcTolSigma =
				le16_to_cpu(profile_v4_2->avfs_meannsigma_dc_tol_sigma);
		param->usMeanNsigmaPlatformMean =
				le16_to_cpu(profile_v4_2->avfs_meannsigma_platform_mean);
		param->usMeanNsigmaPlatformSigma =
				le16_to_cpu(profile_v4_2->avfs_meannsigma_platform_sigma);
		param->ulGbVdroopTableCksoffA0 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_cksoff_a0);
		param->ulGbVdroopTableCksoffA1 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_cksoff_a1);
		param->ulGbVdroopTableCksoffA2 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_cksoff_a2);
		param->ulGbVdroopTableCksonA0 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_ckson_a0);
		param->ulGbVdroopTableCksonA1 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_ckson_a1);
		param->ulGbVdroopTableCksonA2 =
				le32_to_cpu(profile_v4_2->gb_vdroop_table_ckson_a2);
		param->ulGbFuseTableCksoffM1 =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_cksoff_m1);
		param->ulGbFuseTableCksoffM2 =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_cksoff_m2);
		param->ulGbFuseTableCksoffB =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_cksoff_b);
		param->ulGbFuseTableCksonM1 =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_ckson_m1);
		param->ulGbFuseTableCksonM2 =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_ckson_m2);
		param->ulGbFuseTableCksonB =
				le32_to_cpu(profile_v4_2->avfsgb_fuse_table_ckson_b);

		param->ucEnableGbVdroopTableCkson =
				profile_v4_2->enable_gb_vdroop_table_ckson;
		param->ucEnableGbFuseTableCkson =
				profile_v4_2->enable_gb_fuse_table_ckson;
		param->usPsmAgeComfactor =
				le16_to_cpu(profile_v4_2->psm_age_comfactor);

		param->ulDispclk2GfxclkM1 =
				le32_to_cpu(profile_v4_2->dispclk2gfxclk_a);
		param->ulDispclk2GfxclkM2 =
				le32_to_cpu(profile_v4_2->dispclk2gfxclk_b);
		param->ulDispclk2GfxclkB =
				le32_to_cpu(profile_v4_2->dispclk2gfxclk_c);
		param->ulDcefclk2GfxclkM1 =
				le32_to_cpu(profile_v4_2->dcefclk2gfxclk_a);
		param->ulDcefclk2GfxclkM2 =
				le32_to_cpu(profile_v4_2->dcefclk2gfxclk_b);
		param->ulDcefclk2GfxclkB =
				le32_to_cpu(profile_v4_2->dcefclk2gfxclk_c);
		param->ulPixelclk2GfxclkM1 =
				le32_to_cpu(profile_v4_2->pixclk2gfxclk_a);
		param->ulPixelclk2GfxclkM2 =
				le32_to_cpu(profile_v4_2->pixclk2gfxclk_b);
		param->ulPixelclk2GfxclkB =
				le32_to_cpu(profile_v4_2->pixclk2gfxclk_c);
		param->ulPhyclk2GfxclkM1 =
				le32_to_cpu(profile->phyclk2gfxclk_a);
		param->ulPhyclk2GfxclkM2 =
				le32_to_cpu(profile_v4_2->phyclk2gfxclk_b);
		param->ulPhyclk2GfxclkB =
				le32_to_cpu(profile_v4_2->phyclk2gfxclk_c);
		param->ulAcgGbVdroopTableA0 = le32_to_cpu(profile_v4_2->acg_gb_vdroop_table_a0);
		param->ulAcgGbVdroopTableA1 = le32_to_cpu(profile_v4_2->acg_gb_vdroop_table_a1);
		param->ulAcgGbVdroopTableA2 = le32_to_cpu(profile_v4_2->acg_gb_vdroop_table_a2);
		param->ulAcgGbFuseTableM1 = le32_to_cpu(profile_v4_2->acg_avfsgb_fuse_table_m1);
		param->ulAcgGbFuseTableM2 = le32_to_cpu(profile_v4_2->acg_avfsgb_fuse_table_m2);
		param->ulAcgGbFuseTableB = le32_to_cpu(profile_v4_2->acg_avfsgb_fuse_table_b);
		param->ucAcgEnableGbVdroopTable = le32_to_cpu(profile_v4_2->enable_acg_gb_vdroop_table);
		param->ucAcgEnableGbFuseTable = le32_to_cpu(profile_v4_2->enable_acg_gb_fuse_table);
	} else {
		pr_info("Invalid VBIOS AVFS ProfilingInfo Revision!\n");
		return -EINVAL;
	}

	return 0;
}

int pp_atomfwctrl_get_gpio_information(struct pp_hwmgr *hwmgr,
		struct pp_atomfwctrl_gpio_parameters *param)
{
	struct atom_smu_info_v3_1 *info;
	uint16_t idx;

	idx = GetIndexIntoMasterDataTable(smu_info);
	info = (struct atom_smu_info_v3_1 *)
		cgs_atom_get_data_table(hwmgr->device,
				idx, NULL, NULL, NULL);

	if (!info) {
		pr_info("Error retrieving BIOS smu_info Table Address!");
		return -1;
	}

	param->ucAcDcGpio       = info->ac_dc_gpio_bit;
	param->ucAcDcPolarity   = info->ac_dc_polarity;
	param->ucVR0HotGpio     = info->vr0hot_gpio_bit;
	param->ucVR0HotPolarity = info->vr0hot_polarity;
	param->ucVR1HotGpio     = info->vr1hot_gpio_bit;
	param->ucVR1HotPolarity = info->vr1hot_polarity;
	param->ucFwCtfGpio      = info->fw_ctf_gpio_bit;
	param->ucFwCtfPolarity  = info->fw_ctf_polarity;

	return 0;
}

int pp_atomfwctrl__get_clk_information_by_clkid(struct pp_hwmgr *hwmgr, BIOS_CLKID id, uint32_t *frequency)
{
	struct atom_get_smu_clock_info_parameters_v3_1   parameters;
	struct atom_get_smu_clock_info_output_parameters_v3_1 *output;
	uint32_t ix;

	parameters.clk_id = id;
	parameters.command = GET_SMU_CLOCK_INFO_V3_1_GET_CLOCK_FREQ;

	ix = GetIndexIntoMasterCmdTable(getsmuclockinfo);
	if (!cgs_atom_exec_cmd_table(hwmgr->device, ix, &parameters)) {
		output = (struct atom_get_smu_clock_info_output_parameters_v3_1 *)&parameters;
		*frequency = output->atom_smu_outputclkfreq.smu_clock_freq_hz / 10000;
	} else {
		pr_info("Error execute_table getsmuclockinfo!");
		return -1;
	}

	return 0;
}

int pp_atomfwctrl_get_vbios_bootup_values(struct pp_hwmgr *hwmgr,
			struct pp_atomfwctrl_bios_boot_up_values *boot_values)
{
	struct atom_firmware_info_v3_1 *info = NULL;
	uint16_t ix;
	uint32_t frequency = 0;

	ix = GetIndexIntoMasterDataTable(firmwareinfo);
	info = (struct atom_firmware_info_v3_1 *)
		cgs_atom_get_data_table(hwmgr->device,
				ix, NULL, NULL, NULL);

	if (!info) {
		pr_info("Error retrieving BIOS firmwareinfo!");
		return -EINVAL;
	}

	boot_values->ulRevision = info->firmware_revision;
	boot_values->ulGfxClk   = info->bootup_sclk_in10khz;
	boot_values->ulUClk     = info->bootup_mclk_in10khz;
	boot_values->usVddc     = info->bootup_vddc_mv;
	boot_values->usVddci    = info->bootup_vddci_mv;
	boot_values->usMvddc    = info->bootup_mvddc_mv;
	boot_values->usVddGfx   = info->bootup_vddgfx_mv;
	boot_values->ulSocClk   = 0;
	boot_values->ulDCEFClk   = 0;

	if (!pp_atomfwctrl__get_clk_information_by_clkid(hwmgr, SMU9_SYSPLL0_SOCCLK_ID, &frequency))
		boot_values->ulSocClk   = frequency;

	if (!pp_atomfwctrl__get_clk_information_by_clkid(hwmgr, SMU9_SYSPLL0_DCEFCLK_ID, &frequency))
		boot_values->ulDCEFClk   = frequency;

	return 0;
}