/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ALTERNATIVE_MACROS_H
#define __ASM_ALTERNATIVE_MACROS_H

#ifdef CONFIG_RISCV_ERRATA_ALTERNATIVE

#ifndef __ASSEMBLY__

#include <asm/asm.h>
#include <linux/stringify.h>

#define ALT_ENTRY(oldptr, altptr, vendor_id, errata_id, altlen) \
	RISCV_PTR " " oldptr "\n" \
	RISCV_PTR " " altptr "\n" \
	REG_ASM " " vendor_id "\n" \
	REG_ASM " " altlen "\n" \
	".word " errata_id "\n"

#define __ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, enable) \
	"886 :\n"							\
	oldinsn "\n"							\
	".if " __stringify(enable) " == 1\n"				\
	"887 :\n"							\
	".pushsection .alternative, \"a\"\n"				\
	ALT_ENTRY("886b", "888f", __stringify(vendor_id), __stringify(errata_id), "889f - 888f") \
	".popsection\n"							\
	".subsection 1\n"						\
	"888 :\n"							\
	altinsn "\n"							\
	"889 :\n"							\
	".previous\n"							\
	".org	. - (887b - 886b) + (889b - 888b)\n"			\
	".org	. - (889b - 888b) + (887b - 886b)\n"			\
	".endif\n"

#define _ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k)	\
	__ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, IS_ENABLED(CONFIG_k))

#else /* __ASSEMBLY__ */

.macro ALT_ENTRY oldptr altptr vendor_id errata_id alt_len
	RISCV_PTR \oldptr
	RISCV_PTR \altptr
	REG_ASM \vendor_id
	REG_ASM \alt_len
	.word	\errata_id
.endm

.macro __ALTERNATIVE_CFG insn1 insn2 vendor_id errata_id enable = 1
886 :
	\insn1
	.if \enable
887 :
	.pushsection .alternative, "a"
	ALT_ENTRY 886b, 888f, \vendor_id, \errata_id, 889f - 888f
	.popsection
	.subsection 1
888 :
	\insn2
889 :
	.previous
	.org    . - (889b - 888b) + (887b - 886b)
	.org    . - (887b - 886b) + (889b - 888b)
	.endif
.endm

#define _ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k) \
	__ALTERNATIVE_CFG oldinsn, altinsn, vendor_id, errata_id, IS_ENABLED(CONFIG_k)

#endif /* !__ASSEMBLY__ */

#else /* !CONFIG_RISCV_ERRATA_ALTERNATIVE*/
#ifndef __ASSEMBLY__

#define __ALTERNATIVE_CFG(oldinsn)  \
	oldinsn "\n"

#define _ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k) \
	__ALTERNATIVE_CFG(oldinsn)

#else /* __ASSEMBLY__ */

.macro __ALTERNATIVE_CFG insn1
	\insn1
.endm

#define _ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k) \
	__ALTERNATIVE_CFG oldinsn

#endif /* !__ASSEMBLY__ */
#endif /* CONFIG_RISCV_ERRATA_ALTERNATIVE */

/*
 * Usage:
 *	ALTERNATIVE(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k)
 *  in the assembly code. Otherwise,
 *	asm(ALTERNATIVE(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k));
 *
 * oldinsn: The old instruction which will be replaced.
 * altinsn: The replacement instruction.
 * vendor_id: The CPU vendor ID.
 * errata_id: The errata ID.
 * CONFIG_k: The Kconfig of this errata. The instructions replacement can
 *           be disabled by this Kconfig. When Kconfig is disabled, the
 *           oldinsn will always be executed.
 */
#define ALTERNATIVE(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k)   \
	_ALTERNATIVE_CFG(oldinsn, altinsn, vendor_id, errata_id, CONFIG_k)

#endif
