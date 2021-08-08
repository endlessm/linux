// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 Sifive.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/bug.h>
#include <asm/alternative.h>
#include <asm/vendorid_list.h>
#include <asm/errata_list.h>

#define MAX_ERRATA_IMPID 5
struct errata_info_t {
	char name[ERRATA_STRING_LENGTH_MAX];
	unsigned long arch_id;
	unsigned long imp_id[MAX_ERRATA_IMPID];
} errata_info;

struct errata_info_t sifive_errata_list[ERRATA_NUMBER] = {
	{
		.name = "cip-453",
		.arch_id = 0x8000000000000007,
		.imp_id = {
			0x20181004, 0x00200504
		},
	},
};

static inline bool __init cpu_info_cmp(struct errata_info_t *errata, struct alt_entry *alt)
{
	int i;

	if (errata->arch_id != cpu_mfr_info.arch_id)
		return false;

	for (i = 0; i < MAX_ERRATA_IMPID && errata->imp_id[i]; i++)
		if (errata->imp_id[i] == cpu_mfr_info.imp_id)
			return true;

	return false;
}

static bool __init sifive_errata_check(struct alt_entry *alt)
{
	if (cpu_mfr_info.vendor_id != alt->vendor_id)
		return false;

	if (likely(alt->errata_id < ERRATA_NUMBER))
		return cpu_info_cmp(&sifive_errata_list[alt->errata_id], alt);

	WARN_ON(1);
	return false;
}

REGISTER_ERRATA_CHECKFUNC(sifive_errata_check, SIFIVE_VENDOR_ID);
