#ifndef _ASM_POWERPC_ASM_PROTOTYPES_H
#define _ASM_POWERPC_ASM_PROTOTYPES_H
/*
 * This file is for prototypes of C functions that are only called
 * from asm, and any associated variables.
 *
 * Copyright 2016, Daniel Axtens, IBM Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/threads.h>
#include <linux/kprobes.h>
#ifdef CONFIG_KVM
#include <linux/kvm_host.h>
#endif

/* SMP */
extern struct thread_info *current_set[NR_CPUS];
extern struct thread_info *secondary_ti;
void start_secondary(void *unused);

/* kexec */
struct paca_struct;
struct kimage;
extern struct paca_struct kexec_paca;
void kexec_copy_flush(struct kimage *image);

/* pseries hcall tracing */
extern struct static_key hcall_tracepoint_key;
void __trace_hcall_entry(unsigned long opcode, unsigned long *args);
void __trace_hcall_exit(long opcode, unsigned long retval,
			unsigned long *retbuf);
/* OPAL tracing */
#ifdef HAVE_JUMP_LABEL
extern struct static_key opal_tracepoint_key;
#endif

void __trace_opal_entry(unsigned long opcode, unsigned long *args);
void __trace_opal_exit(long opcode, unsigned long retval);

/* VMX copying */
int enter_vmx_usercopy(void);
int exit_vmx_usercopy(void);
int enter_vmx_copy(void);
void * exit_vmx_copy(void *dest);

/* Traps */
long machine_check_early(struct pt_regs *regs);
long hmi_exception_realmode(struct pt_regs *regs);
void SMIException(struct pt_regs *regs);
void handle_hmi_exception(struct pt_regs *regs);
void instruction_breakpoint_exception(struct pt_regs *regs);
void RunModeException(struct pt_regs *regs);
void __kprobes single_step_exception(struct pt_regs *regs);
void __kprobes program_check_exception(struct pt_regs *regs);
void alignment_exception(struct pt_regs *regs);
void StackOverflow(struct pt_regs *regs);
void nonrecoverable_exception(struct pt_regs *regs);
void kernel_fp_unavailable_exception(struct pt_regs *regs);
void altivec_unavailable_exception(struct pt_regs *regs);
void vsx_unavailable_exception(struct pt_regs *regs);
void fp_unavailable_tm(struct pt_regs *regs);
void altivec_unavailable_tm(struct pt_regs *regs);
void vsx_unavailable_tm(struct pt_regs *regs);
void facility_unavailable_exception(struct pt_regs *regs);
void TAUException(struct pt_regs *regs);
void altivec_assist_exception(struct pt_regs *regs);
void unrecoverable_exception(struct pt_regs *regs);
void kernel_bad_stack(struct pt_regs *regs);
void system_reset_exception(struct pt_regs *regs);
void machine_check_exception(struct pt_regs *regs);
void __kprobes emulation_assist_interrupt(struct pt_regs *regs);

/* kvm */
#ifdef CONFIG_KVM
long kvmppc_rm_h_put_tce(struct kvm_vcpu *vcpu, unsigned long liobn,
			 unsigned long ioba, unsigned long tce);
long kvmppc_rm_h_put_tce_indirect(struct kvm_vcpu *vcpu,
				  unsigned long liobn, unsigned long ioba,
				  unsigned long tce_list, unsigned long npages);
long kvmppc_rm_h_stuff_tce(struct kvm_vcpu *vcpu,
			   unsigned long liobn, unsigned long ioba,
			   unsigned long tce_value, unsigned long npages);
long int kvmppc_rm_h_confer(struct kvm_vcpu *vcpu, int target,
                            unsigned int yield_count);
long kvmppc_h_random(struct kvm_vcpu *vcpu);
void kvmhv_commence_exit(int trap);
long kvmppc_realmode_machine_check(struct kvm_vcpu *vcpu);
void kvmppc_subcore_enter_guest(void);
void kvmppc_subcore_exit_guest(void);
long kvmppc_realmode_hmi_handler(void);
long kvmppc_h_enter(struct kvm_vcpu *vcpu, unsigned long flags,
                    long pte_index, unsigned long pteh, unsigned long ptel);
long kvmppc_h_remove(struct kvm_vcpu *vcpu, unsigned long flags,
                     unsigned long pte_index, unsigned long avpn);
long kvmppc_h_bulk_remove(struct kvm_vcpu *vcpu);
long kvmppc_h_protect(struct kvm_vcpu *vcpu, unsigned long flags,
                      unsigned long pte_index, unsigned long avpn,
                      unsigned long va);
long kvmppc_h_read(struct kvm_vcpu *vcpu, unsigned long flags,
                   unsigned long pte_index);
long kvmppc_h_clear_ref(struct kvm_vcpu *vcpu, unsigned long flags,
                        unsigned long pte_index);
long kvmppc_h_clear_mod(struct kvm_vcpu *vcpu, unsigned long flags,
                        unsigned long pte_index);
long kvmppc_hpte_hv_fault(struct kvm_vcpu *vcpu, unsigned long addr,
                          unsigned long slb_v, unsigned int status, bool data);
unsigned long kvmppc_rm_h_xirr(struct kvm_vcpu *vcpu);
int kvmppc_rm_h_ipi(struct kvm_vcpu *vcpu, unsigned long server,
                    unsigned long mfrr);
int kvmppc_rm_h_cppr(struct kvm_vcpu *vcpu, unsigned long cppr);
int kvmppc_rm_h_eoi(struct kvm_vcpu *vcpu, unsigned long xirr);
#endif

#endif /* _ASM_POWERPC_ASM_PROTOTYPES_H */
