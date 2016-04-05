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

#include "hwmgr.h"
#include "tonga_clockpowergating.h"
#include "tonga_ppsmc.h"
#include "tonga_hwmgr.h"

int tonga_phm_powerdown_uvd(struct pp_hwmgr *hwmgr)
{
	if (phm_cf_want_uvd_power_gating(hwmgr))
		return smum_send_msg_to_smc(hwmgr->smumgr,
						     PPSMC_MSG_UVDPowerOFF);
	return 0;
}

int tonga_phm_powerup_uvd(struct pp_hwmgr *hwmgr)
{
	if (phm_cf_want_uvd_power_gating(hwmgr)) {
		if (phm_cap_enabled(hwmgr->platform_descriptor.platformCaps,
				  PHM_PlatformCaps_UVDDynamicPowerGating)) {
			return smum_send_msg_to_smc_with_parameter(hwmgr->smumgr,
								PPSMC_MSG_UVDPowerON, 1);
		} else {
			return smum_send_msg_to_smc_with_parameter(hwmgr->smumgr,
								PPSMC_MSG_UVDPowerON, 0);
		}
	}

	return 0;
}

int tonga_phm_powerdown_vce(struct pp_hwmgr *hwmgr)
{
	if (phm_cf_want_vce_power_gating(hwmgr))
		return smum_send_msg_to_smc(hwmgr->smumgr,
						  PPSMC_MSG_VCEPowerOFF);
	return 0;
}

int tonga_phm_powerup_vce(struct pp_hwmgr *hwmgr)
{
	if (phm_cf_want_vce_power_gating(hwmgr))
		return smum_send_msg_to_smc(hwmgr->smumgr,
						  PPSMC_MSG_VCEPowerON);
	return 0;
}

int tonga_phm_set_asic_block_gating(struct pp_hwmgr *hwmgr, enum PHM_AsicBlock block, enum PHM_ClockGateSetting gating)
{
	int ret = 0;

	switch (block) {
	case PHM_AsicBlock_UVD_MVC:
	case PHM_AsicBlock_UVD:
	case PHM_AsicBlock_UVD_HD:
	case PHM_AsicBlock_UVD_SD:
		if (gating == PHM_ClockGateSetting_StaticOff)
			ret = tonga_phm_powerdown_uvd(hwmgr);
		else
			ret = tonga_phm_powerup_uvd(hwmgr);
		break;
	case PHM_AsicBlock_GFX:
	default:
		break;
	}

	return ret;
}

int tonga_phm_disable_clock_power_gating(struct pp_hwmgr *hwmgr)
{
	struct tonga_hwmgr *data = (struct tonga_hwmgr *)(hwmgr->backend);

	data->uvd_power_gated = false;
	data->vce_power_gated = false;

	tonga_phm_powerup_uvd(hwmgr);
	tonga_phm_powerup_vce(hwmgr);

	return 0;
}

int tonga_phm_powergate_uvd(struct pp_hwmgr *hwmgr, bool bgate)
{
	struct tonga_hwmgr *data = (struct tonga_hwmgr *)(hwmgr->backend);

	if (data->uvd_power_gated == bgate)
		return 0;

	data->uvd_power_gated = bgate;

	if (bgate) {
		cgs_set_clockgating_state(hwmgr->device,
						AMD_IP_BLOCK_TYPE_UVD,
						AMD_CG_STATE_UNGATE);
		cgs_set_powergating_state(hwmgr->device,
						AMD_IP_BLOCK_TYPE_UVD,
						AMD_PG_STATE_GATE);
		tonga_update_uvd_dpm(hwmgr, true);
		tonga_phm_powerdown_uvd(hwmgr);
	} else {
		tonga_phm_powerup_uvd(hwmgr);
		cgs_set_powergating_state(hwmgr->device,
						AMD_IP_BLOCK_TYPE_UVD,
						AMD_PG_STATE_UNGATE);
		cgs_set_clockgating_state(hwmgr->device,
						AMD_IP_BLOCK_TYPE_UVD,
						AMD_PG_STATE_GATE);

		tonga_update_uvd_dpm(hwmgr, false);
	}

	return 0;
}

int tonga_phm_powergate_vce(struct pp_hwmgr *hwmgr, bool bgate)
{
	struct tonga_hwmgr *data = (struct tonga_hwmgr *)(hwmgr->backend);
	struct phm_set_power_state_input states;
	const struct pp_power_state  *pcurrent;
	struct pp_power_state  *requested;

	pcurrent = hwmgr->current_ps;
	requested = hwmgr->request_ps;

	states.pcurrent_state = &(pcurrent->hardware);
	states.pnew_state = &(requested->hardware);

	if (phm_cf_want_vce_power_gating(hwmgr)) {
		if (data->vce_power_gated != bgate) {
			if (bgate) {
				cgs_set_clockgating_state(
							hwmgr->device,
							AMD_IP_BLOCK_TYPE_VCE,
							AMD_CG_STATE_UNGATE);
				cgs_set_powergating_state(
							hwmgr->device,
							AMD_IP_BLOCK_TYPE_VCE,
							AMD_PG_STATE_GATE);
				tonga_enable_disable_vce_dpm(hwmgr, false);
				data->vce_power_gated = true;
			} else {
				tonga_phm_powerup_vce(hwmgr);
				data->vce_power_gated = false;
				cgs_set_powergating_state(
							hwmgr->device,
							AMD_IP_BLOCK_TYPE_VCE,
							AMD_PG_STATE_UNGATE);
				cgs_set_clockgating_state(
							hwmgr->device,
							AMD_IP_BLOCK_TYPE_VCE,
							AMD_PG_STATE_GATE);

				tonga_update_vce_dpm(hwmgr, &states);
				tonga_enable_disable_vce_dpm(hwmgr, true);
				return 0;
			}
		}
	} else {
		tonga_update_vce_dpm(hwmgr, &states);
		tonga_enable_disable_vce_dpm(hwmgr, true);
		return 0;
	}

	if (!data->vce_power_gated)
		tonga_update_vce_dpm(hwmgr, &states);

	return 0;
}

int tonga_phm_update_clock_gatings(struct pp_hwmgr *hwmgr,
					const uint32_t *msg_id)
{
	PPSMC_Msg msg;
	uint32_t value;

	switch ((*msg_id & PP_GROUP_MASK) >> PP_GROUP_SHIFT) {
	case PP_GROUP_GFX:
		switch ((*msg_id & PP_BLOCK_MASK) >> PP_BLOCK_SHIFT) {
		case PP_BLOCK_GFX_CG:
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
				  ? PPSMC_MSG_EnableClockGatingFeature
				  : PPSMC_MSG_DisableClockGatingFeature;
				value = CG_GFX_CGCG_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			if (PP_STATE_SUPPORT_LS & *msg_id) {
				msg = (*msg_id & PP_STATE_MASK) & PP_STATE_LS
					? PPSMC_MSG_EnableClockGatingFeature
					: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_GFX_CGLS_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		case PP_BLOCK_GFX_MG:
			/* For GFX MGCG, there are three different ones;
			 * CPF, RLC, and all others.  CPF MGCG will not be used for Tonga.
			 * For GFX MGLS, Tonga will not support it.
			 * */
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = (CG_RLC_MGCG_MASK | CG_GFX_OTHERS_MGCG_MASK);

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		default:
			return -1;
		}
		break;

	case PP_GROUP_SYS:
		switch ((*msg_id & PP_BLOCK_MASK) >> PP_BLOCK_SHIFT) {
		case PP_BLOCK_SYS_BIF:
			if (PP_STATE_SUPPORT_LS & *msg_id) {
				msg = (*msg_id & PP_STATE_MASK) & PP_STATE_LS
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_BIF_MGLS_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		case PP_BLOCK_SYS_MC:
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_MC_MGCG_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}

			if (PP_STATE_SUPPORT_LS & *msg_id) {
				msg = (*msg_id & PP_STATE_MASK) & PP_STATE_LS
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_MC_MGLS_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;

			}
			break;

		case PP_BLOCK_SYS_HDP:
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
					? PPSMC_MSG_EnableClockGatingFeature
					: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_HDP_MGCG_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}

			if (PP_STATE_SUPPORT_LS & *msg_id) {
				msg = (*msg_id & PP_STATE_MASK) & PP_STATE_LS
					? PPSMC_MSG_EnableClockGatingFeature
					: PPSMC_MSG_DisableClockGatingFeature;

				value = CG_SYS_HDP_MGLS_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		case PP_BLOCK_SYS_SDMA:
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_SDMA_MGCG_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}

			if (PP_STATE_SUPPORT_LS & *msg_id) {
				msg = (*msg_id & PP_STATE_MASK) & PP_STATE_LS
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;

				value = CG_SYS_SDMA_MGLS_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		case PP_BLOCK_SYS_ROM:
			if (PP_STATE_SUPPORT_CG & *msg_id) {
				msg = ((*msg_id & PP_STATE_MASK) & PP_STATE_CG)
				? PPSMC_MSG_EnableClockGatingFeature
				: PPSMC_MSG_DisableClockGatingFeature;
				value = CG_SYS_ROM_MASK;

				if (0 != smum_send_msg_to_smc_with_parameter(hwmgr->smumgr, msg, value))
					return -1;
			}
			break;

		default:
			return -1;

		}
		break;

	default:
		return -1;

	}

	return 0;
}
