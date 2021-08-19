/*
 * arch/arm64/kernel/topology.c
 *
 * Copyright (C) 2011,2013,2014 Linaro Limited.
 *
 * Based on the arm32 version written by Vincent Guittot in turn based on
 * arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/acpi.h>
#include <linux/arch_topology.h>
#include <linux/cacheinfo.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/percpu.h>

#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/topology.h>

void store_cpu_topology(unsigned int cpuid)
{
	struct cpu_topology *cpuid_topo = &cpu_topology[cpuid];
	u64 mpidr;

	if (cpuid_topo->package_id != -1)
		goto topology_populated;

	mpidr = read_cpuid_mpidr();

	/* Uniprocessor systems can rely on default topology values */
	if (mpidr & MPIDR_UP_BITMASK)
		return;

	/*
	 * This would be the place to create cpu topology based on MPIDR.
	 *
	 * However, it cannot be trusted to depict the actual topology; some
	 * pieces of the architecture enforce an artificial cap on Aff0 values
	 * (e.g. GICv3's ICC_SGI1R_EL1 limits it to 15), leading to an
	 * artificial cycling of Aff1, Aff2 and Aff3 values. IOW, these end up
	 * having absolutely no relationship to the actual underlying system
	 * topology, and cannot be reasonably used as core / package ID.
	 *
	 * If the MT bit is set, Aff0 *could* be used to define a thread ID, but
	 * we still wouldn't be able to obtain a sane core ID. This means we
	 * need to entirely ignore MPIDR for any topology deduction.
	 */
	cpuid_topo->thread_id  = -1;
	cpuid_topo->core_id    = cpuid;
	cpuid_topo->package_id = cpu_to_node(cpuid);

	pr_debug("CPU%u: cluster %d core %d thread %d mpidr %#016llx\n",
		 cpuid, cpuid_topo->package_id, cpuid_topo->core_id,
		 cpuid_topo->thread_id, mpidr);

topology_populated:
	update_siblings_masks(cpuid);
}

#ifdef CONFIG_ACPI
static bool __init acpi_cpu_is_threaded(int cpu)
{
	int is_threaded = acpi_pptt_cpu_is_thread(cpu);

	/*
	 * if the PPTT doesn't have thread information, assume a homogeneous
	 * machine and return the current CPU's thread state.
	 */
	if (is_threaded < 0)
		is_threaded = read_cpuid_mpidr() & MPIDR_MT_BITMASK;

	return !!is_threaded;
}

/*
 * Propagate the topology information of the processor_topology_node tree to the
 * cpu_topology array.
 */
int __init parse_acpi_topology(void)
{
	int cpu, topology_id;

	if (acpi_disabled)
		return 0;

	for_each_possible_cpu(cpu) {
		int i, cache_id;

		topology_id = find_acpi_cpu_topology(cpu, 0);
		if (topology_id < 0)
			return topology_id;

		if (acpi_cpu_is_threaded(cpu)) {
			cpu_topology[cpu].thread_id = topology_id;
			topology_id = find_acpi_cpu_topology(cpu, 1);
			cpu_topology[cpu].core_id   = topology_id;
		} else {
			cpu_topology[cpu].thread_id  = -1;
			cpu_topology[cpu].core_id    = topology_id;
		}
		topology_id = find_acpi_cpu_topology_package(cpu);
		cpu_topology[cpu].package_id = topology_id;

		i = acpi_find_last_cache_level(cpu);

		if (i > 0) {
			/*
			 * this is the only part of cpu_topology that has
			 * a direct relationship with the cache topology
			 */
			cache_id = find_acpi_cpu_cache_topology(cpu, i);
			if (cache_id > 0)
				cpu_topology[cpu].llc_id = cache_id;
		}
	}

	return 0;
}
#endif

#ifdef CONFIG_ARM64_AMU_EXTN
#define read_corecnt()	read_sysreg_s(SYS_AMEVCNTR0_CORE_EL0)
#define read_constcnt()	read_sysreg_s(SYS_AMEVCNTR0_CONST_EL0)
#else
#define read_corecnt()	(0UL)
#define read_constcnt()	(0UL)
#endif

#undef pr_fmt
#define pr_fmt(fmt) "AMU: " fmt

static DEFINE_PER_CPU_READ_MOSTLY(unsigned long, arch_max_freq_scale);
static DEFINE_PER_CPU(u64, arch_const_cycles_prev);
static DEFINE_PER_CPU(u64, arch_core_cycles_prev);
static cpumask_var_t amu_fie_cpus;

void update_freq_counters_refs(void)
{
	this_cpu_write(arch_core_cycles_prev, read_corecnt());
	this_cpu_write(arch_const_cycles_prev, read_constcnt());
}

static inline bool freq_counters_valid(int cpu)
{
	if ((cpu >= nr_cpu_ids) || !cpumask_test_cpu(cpu, cpu_present_mask))
		return false;

	if (!cpu_has_amu_feat(cpu)) {
		pr_debug("CPU%d: counters are not supported.\n", cpu);
		return false;
	}

	if (unlikely(!per_cpu(arch_const_cycles_prev, cpu) ||
		     !per_cpu(arch_core_cycles_prev, cpu))) {
		pr_debug("CPU%d: cycle counters are not enabled.\n", cpu);
		return false;
	}

	return true;
}

static int freq_inv_set_max_ratio(int cpu, u64 max_rate, u64 ref_rate)
{
	u64 ratio;

	if (unlikely(!max_rate || !ref_rate)) {
		pr_debug("CPU%d: invalid maximum or reference frequency.\n",
			 cpu);
		return -EINVAL;
	}

	/*
	 * Pre-compute the fixed ratio between the frequency of the constant
	 * reference counter and the maximum frequency of the CPU.
	 *
	 *			    ref_rate
	 * arch_max_freq_scale =   ---------- * SCHED_CAPACITY_SCALE²
	 *			    max_rate
	 *
	 * We use a factor of 2 * SCHED_CAPACITY_SHIFT -> SCHED_CAPACITY_SCALE²
	 * in order to ensure a good resolution for arch_max_freq_scale for
	 * very low reference frequencies (down to the KHz range which should
	 * be unlikely).
	 */
	ratio = ref_rate << (2 * SCHED_CAPACITY_SHIFT);
	ratio = div64_u64(ratio, max_rate);
	if (!ratio) {
		WARN_ONCE(1, "Reference frequency too low.\n");
		return -EINVAL;
	}

	per_cpu(arch_max_freq_scale, cpu) = (unsigned long)ratio;

	return 0;
}

static inline bool
enable_policy_freq_counters(int cpu, cpumask_var_t valid_cpus)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);

	if (!policy) {
		pr_debug("CPU%d: No cpufreq policy found.\n", cpu);
		return false;
	}

	if (cpumask_subset(policy->related_cpus, valid_cpus))
		cpumask_or(amu_fie_cpus, policy->related_cpus,
			   amu_fie_cpus);

	cpufreq_cpu_put(policy);

	return true;
}

static DEFINE_STATIC_KEY_FALSE(amu_fie_key);
#define amu_freq_invariant() static_branch_unlikely(&amu_fie_key)

static int __init init_amu_fie(void)
{
	bool invariance_status = topology_scale_freq_invariant();
	cpumask_var_t valid_cpus;
	bool have_policy = false;
	int ret = 0;
	int cpu;

	if (!zalloc_cpumask_var(&valid_cpus, GFP_KERNEL))
		return -ENOMEM;

	if (!zalloc_cpumask_var(&amu_fie_cpus, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto free_valid_mask;
	}

	for_each_present_cpu(cpu) {
		if (!freq_counters_valid(cpu) ||
		    freq_inv_set_max_ratio(cpu,
					   cpufreq_get_hw_max_freq(cpu) * 1000,
					   arch_timer_get_rate()))
			continue;

		cpumask_set_cpu(cpu, valid_cpus);
		have_policy |= enable_policy_freq_counters(cpu, valid_cpus);
	}

	/*
	 * If we are not restricted by cpufreq policies, we only enable
	 * the use of the AMU feature for FIE if all CPUs support AMU.
	 * Otherwise, enable_policy_freq_counters has already enabled
	 * policy cpus.
	 */
	if (!have_policy && cpumask_equal(valid_cpus, cpu_present_mask))
		cpumask_or(amu_fie_cpus, amu_fie_cpus, valid_cpus);

	if (!cpumask_empty(amu_fie_cpus)) {
		pr_info("CPUs[%*pbl]: counters will be used for FIE.",
			cpumask_pr_args(amu_fie_cpus));
		static_branch_enable(&amu_fie_key);
	}

	/*
	 * If the system is not fully invariant after AMU init, disable
	 * partial use of counters for frequency invariance.
	 */
	if (!topology_scale_freq_invariant())
		static_branch_disable(&amu_fie_key);

	/*
	 * Task scheduler behavior depends on frequency invariance support,
	 * either cpufreq or counter driven. If the support status changes as
	 * a result of counter initialisation and use, retrigger the build of
	 * scheduling domains to ensure the information is propagated properly.
	 */
	if (invariance_status != topology_scale_freq_invariant())
		rebuild_sched_domains_energy();

free_valid_mask:
	free_cpumask_var(valid_cpus);

	return ret;
}
late_initcall_sync(init_amu_fie);

bool arch_freq_counters_available(const struct cpumask *cpus)
{
	return amu_freq_invariant() &&
	       cpumask_subset(cpus, amu_fie_cpus);
}

void topology_scale_freq_tick(void)
{
	u64 prev_core_cnt, prev_const_cnt;
	u64 core_cnt, const_cnt, scale;
	int cpu = smp_processor_id();

	if (!amu_freq_invariant())
		return;

	if (!cpumask_test_cpu(cpu, amu_fie_cpus))
		return;

	prev_const_cnt = this_cpu_read(arch_const_cycles_prev);
	prev_core_cnt = this_cpu_read(arch_core_cycles_prev);

	update_freq_counters_refs();

	const_cnt = this_cpu_read(arch_const_cycles_prev);
	core_cnt = this_cpu_read(arch_core_cycles_prev);

	if (unlikely(core_cnt <= prev_core_cnt ||
		     const_cnt <= prev_const_cnt))
		return;

	/*
	 *	    /\core    arch_max_freq_scale
	 * scale =  ------- * --------------------
	 *	    /\const   SCHED_CAPACITY_SCALE
	 *
	 * See validate_cpu_freq_invariance_counters() for details on
	 * arch_max_freq_scale and the use of SCHED_CAPACITY_SHIFT.
	 */
	scale = core_cnt - prev_core_cnt;
	scale *= this_cpu_read(arch_max_freq_scale);
	scale = div64_u64(scale >> SCHED_CAPACITY_SHIFT,
			  const_cnt - prev_const_cnt);

	scale = min_t(unsigned long, scale, SCHED_CAPACITY_SCALE);
	this_cpu_write(freq_scale, (unsigned long)scale);
}

#ifdef CONFIG_ACPI_CPPC_LIB
#include <acpi/cppc_acpi.h>

static void cpu_read_corecnt(void *val)
{
	*(u64 *)val = read_corecnt();
}

static void cpu_read_constcnt(void *val)
{
	*(u64 *)val = read_constcnt();
}

static inline
int counters_read_on_cpu(int cpu, smp_call_func_t func, u64 *val)
{
	/*
	 * Abort call on counterless CPU or when interrupts are
	 * disabled - can lead to deadlock in smp sync call.
	 */
	if (!cpu_has_amu_feat(cpu))
		return -EOPNOTSUPP;

	if (WARN_ON_ONCE(irqs_disabled()))
		return -EPERM;

	smp_call_function_single(cpu, func, val, 1);

	return 0;
}

/*
 * Refer to drivers/acpi/cppc_acpi.c for the description of the functions
 * below.
 */
bool cpc_ffh_supported(void)
{
	return freq_counters_valid(get_cpu_with_amu_feat());
}

int cpc_read_ffh(int cpu, struct cpc_reg *reg, u64 *val)
{
	int ret = -EOPNOTSUPP;

	switch ((u64)reg->address) {
	case 0x0:
		ret = counters_read_on_cpu(cpu, cpu_read_corecnt, val);
		break;
	case 0x1:
		ret = counters_read_on_cpu(cpu, cpu_read_constcnt, val);
		break;
	}

	if (!ret) {
		*val &= GENMASK_ULL(reg->bit_offset + reg->bit_width - 1,
				    reg->bit_offset);
		*val >>= reg->bit_offset;
	}

	return ret;
}

int cpc_write_ffh(int cpunum, struct cpc_reg *reg, u64 val)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_ACPI_CPPC_LIB */
