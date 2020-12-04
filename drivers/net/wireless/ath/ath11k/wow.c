// SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
 */

#include <linux/delay.h>

#include "mac.h"
#include "core.h"
#include "hif.h"
#include "debug.h"
#include "wmi.h"
#include "wow.h"

int ath11k_wow_enable(struct ath11k *ar)
{
	int i, ret;

	ar->target_suspend_ack = false;

	for (i = 0; i < ATH11K_WOW_RETRY_NUM; i++) {
		reinit_completion(&ar->target_suspend);
		ret = ath11k_wmi_wow_enable(ar);
		if (ret) {
			ath11k_warn(ar->ab, "failed to issue wow enable: %d\n", ret);
			return ret;
		}

		ret = wait_for_completion_timeout(&ar->target_suspend, 3 * HZ);
		if (ret == 0) {
			ath11k_warn(ar->ab,
				    "timed out while waiting for suspend completion\n");
			return -ETIMEDOUT;
		} else {
			/* If suspend_nack is received, host will send
			 * wow_enable again after ATH11K_WOW_RETRY_WAIT_MS.
			 */
			if (!ar->target_suspend_ack) {
				ath11k_warn(ar->ab, "wow enbale get nack %d\n", i);
				msleep(ATH11K_WOW_RETRY_WAIT_MS);
				continue;
			}
			break;
		}
	}

	return 0;
}

int ath11k_wow_wakeup(struct ath11k *ar)
{
	int ret;

	reinit_completion(&ar->wow.wakeup_completed);

	ret = ath11k_wmi_wow_host_wakeup_ind(ar);
	if (ret) {
		ath11k_warn(ar->ab, "failed to send wow wakeup indication: %d\n",
			    ret);
		return ret;
	}

	ret = wait_for_completion_timeout(&ar->wow.wakeup_completed, 3 * HZ);
	if (ret == 0) {
			ath11k_warn(ar->ab, "timed out while waiting for wow wakeup completion\n");
			return -ETIMEDOUT;
	}

	return 0;
}
