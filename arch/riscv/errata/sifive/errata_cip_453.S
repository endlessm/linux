/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 SiFive
 */

#include <linux/linkage.h>
#include <asm/asm.h>
#include <asm/asm-offsets.h>
#include <asm/alternative.h>

.macro ADD_SIGN_EXT pt_reg badaddr tmp_reg
	REG_L \badaddr, PT_BADADDR(\pt_reg)
	li \tmp_reg,1
	slli \tmp_reg,\tmp_reg,0x26
	and \tmp_reg,\tmp_reg,\badaddr
	beqz \tmp_reg, 1f
	li \tmp_reg,-1
	slli \tmp_reg,\tmp_reg,0x27
	or \badaddr,\tmp_reg,\badaddr
	REG_S \badaddr, PT_BADADDR(\pt_reg)
1:
.endm

ENTRY(do_page_fault_trampoline)
	ADD_SIGN_EXT a0, t0, t1
	la t0, do_page_fault
	jr t0
END(do_page_fault_trampoline)

ENTRY(do_trap_insn_fault_trampoline)
	ADD_SIGN_EXT a0, t0, t1
	la t0, do_trap_insn_fault
	jr t0
END(do_trap_insn_fault_trampoline)
