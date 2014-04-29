#ifndef _ASM_POWERPC_SECTIONS_H
#define _ASM_POWERPC_SECTIONS_H
#ifdef __KERNEL__

#include <linux/elf.h>
#include <linux/uaccess.h>
#include <asm-generic/sections.h>

#ifdef __powerpc64__

#ifdef CONFIG_PPC_BOOK3E
extern char interrupt_base_book3e[];
extern char interrupt_end_book3e[];
#else
extern char __start_interrupts[];
extern char __end_interrupts[];
#endif

extern char __prom_init_toc_start[];
extern char __prom_init_toc_end[];

static inline int in_kernel_text(unsigned long addr)
{
	if (addr >= (unsigned long)_stext && addr < (unsigned long)__init_end)
		return 1;

	return 0;
}

static inline int overlaps_interrupt_vector_text(unsigned long start,
							unsigned long end)
{
	unsigned long real_start, real_end;
#ifdef CONFIG_PPC_BOOK3E
	real_start = interrupt_base_book3e - _stext;
	real_end = interrupt_end_book3e - _stext;
#else
	real_start = __start_interrupts - _stext;
	real_end = __end_interrupts - _stext;
#endif
	return start < (unsigned long)__va(real_end) &&
		(unsigned long)__va(real_start) < end;
}

static inline int overlaps_kernel_text(unsigned long start, unsigned long end)
{
	return start < (unsigned long)__init_end &&
		(unsigned long)_stext < end;
}

static inline int overlaps_kvm_tmp(unsigned long start, unsigned long end)
{
#ifdef CONFIG_KVM_GUEST
	extern char kvm_tmp[];
	return start < (unsigned long)kvm_tmp &&
		(unsigned long)&kvm_tmp[1024 * 1024] < end;
#else
	return 0;
#endif
}

#if !defined(_CALL_ELF) || _CALL_ELF != 2
#undef dereference_function_descriptor
static inline void *dereference_function_descriptor(void *ptr)
{
	struct ppc64_opd_entry *desc = ptr;
	void *p;

	if (!probe_kernel_address(&desc->funcaddr, p))
		ptr = p;
	return ptr;
}
#endif

#endif

#endif /* __KERNEL__ */
#endif	/* _ASM_POWERPC_SECTIONS_H */
