// SPDX-License-Identifier: GPL-2.0-only
/*
 * alternative runtime patching
 * inspired by the ARM64 and x86 version
 *
 * Copyright (C) 2021 Sifive.
 */

#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/uaccess.h>
#include <asm/patch.h>
#include <asm/alternative.h>
#include <asm/sections.h>

struct alt_region {
	struct alt_entry *begin;
	struct alt_entry *end;
};

static bool (*errata_checkfunc)(struct alt_entry *alt);
typedef int (*patch_func_t)(void *addr, const void *insn, size_t size);

static void __apply_alternatives(void *alt_region, void *alt_patch_func)
{
	struct alt_entry *alt;
	struct alt_region *region = alt_region;

	for (alt = region->begin; alt < region->end; alt++) {
		if (!errata_checkfunc(alt))
			continue;
		((patch_func_t)alt_patch_func)(alt->old_ptr, alt->alt_ptr, alt->alt_len);
	}
}

static void __init init_alternative(void)
{
	struct errata_checkfunc_id *ptr;

	for (ptr = (struct errata_checkfunc_id *)__alt_checkfunc_table;
	     ptr < (struct errata_checkfunc_id *)__alt_checkfunc_table_end;
	     ptr++) {
		if (cpu_mfr_info.vendor_id == ptr->vendor_id)
			errata_checkfunc = ptr->func;
	}
}

/*
 * This is called very early in the boot process (directly after we run
 * a feature detect on the boot CPU). No need to worry about other CPUs
 * here.
 */
void __init apply_boot_alternatives(void)
{
	struct alt_region region;

	/* If called on non-boot cpu things could go wrong */
	WARN_ON(smp_processor_id() != 0);

	init_alternative();

	if (!errata_checkfunc)
		return;

	region.begin = (struct alt_entry *)__alt_start;
	region.end = (struct alt_entry *)__alt_end;
	__apply_alternatives(&region, patch_text_nosync);
}

