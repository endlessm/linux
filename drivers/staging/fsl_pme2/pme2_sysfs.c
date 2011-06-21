/* Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pme2_regs.h"
#include "pme2_private.h"

#define MAX_ACCUMULATOR_INTERVAL 10000
extern u32 pme_stat_interval;

/* The pme sysfs contains the following types of attributes
 * 1) root level: general pme confuration
 * 2) bsc: bufferpool size configuration
 * 3) stats: pme statistics
 */
static ssize_t pme_store(struct device *dev, struct device_attribute *dev_attr,
		const char *buf, size_t count, enum pme_attr attr)
{
	unsigned long val;
	size_t ret;
	if (strict_strtoul(buf, 0, &val)) {
		dev_dbg(dev, "invalid input %s\n",buf);
		return -EINVAL;
	}
	ret = pme_attr_set(attr, val);
	if (ret) {
		dev_err(dev, "attr_set err attr=%u, val=%lu\n", attr, val);
		return ret;
	}
	return count;
}

static ssize_t pme_show(struct device *dev, struct device_attribute *dev_attr,
		char *buf, enum pme_attr attr, const char *fmt)
{
	u32 data;
	int ret;

	ret =  pme_attr_get(attr, &data);
	if (!ret)
		return snprintf(buf, PAGE_SIZE, fmt, data);
	return ret;
}


static ssize_t pme_stat_show(struct device *dev,
	struct device_attribute *dev_attr, char *buf, enum pme_attr attr)
{
	u64 data = 0;
	int ret = 0;

	ret = pme_stat_get(attr, &data, 0);
	if (!ret)
		return snprintf(buf, PAGE_SIZE, "%llu\n", data);
	else
		return ret;
}

static ssize_t pme_stat_store(struct device *dev,
		struct device_attribute *dev_attr, const char *buf,
		size_t count, enum pme_attr attr)
{
	unsigned long val;
	u64 data = 0;
	size_t ret = 0;
	if (strict_strtoul(buf, 0, &val)) {
		pr_err("pme: invalid input %s\n", buf);
		return -EINVAL;
	}
	if (val) {
		pr_err("pme: invalid input %s\n", buf);
		return -EINVAL;
	}
	ret = pme_stat_get(attr, &data, 1);
	return count;
}


#define PME_SYSFS_ATTR(pme_attr, perm, showhex) \
static ssize_t pme_store_##pme_attr(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count) \
{ \
	return pme_store(dev, attr, buf, count, pme_attr_##pme_attr);\
} \
static ssize_t pme_show_##pme_attr(struct device *dev, \
		struct device_attribute *attr, char *buf) \
{ \
	return pme_show(dev, attr, buf, pme_attr_##pme_attr, showhex);\
} \
static DEVICE_ATTR( pme_attr, perm, pme_show_##pme_attr, pme_store_##pme_attr);


#define PME_SYSFS_STAT_ATTR(pme_attr, perm) \
static ssize_t pme_store_##pme_attr(struct device *dev, \
		struct device_attribute *attr, const char *buf, size_t count) \
{ \
	return pme_stat_store(dev, attr, buf, count, pme_attr_##pme_attr);\
} \
static ssize_t pme_show_##pme_attr(struct device *dev, \
		struct device_attribute *attr, char *buf) \
{ \
	return pme_stat_show(dev, attr, buf, pme_attr_##pme_attr);\
} \
static DEVICE_ATTR(pme_attr, perm, pme_show_##pme_attr, pme_store_##pme_attr);


#define PME_SYSFS_BSC_ATTR(bsc_id, perm, showhex) \
static ssize_t pme_store_bsc_##bsc_id(struct device *dev,\
		struct device_attribute *attr, const char *buf, size_t count) \
{ \
	return pme_store(dev, attr, buf, count, pme_attr_bsc(bsc_id));\
} \
static ssize_t pme_show_bsc_##bsc_id(struct device *dev,\
		struct device_attribute *attr, char *buf) \
{ \
	return pme_show(dev, attr, buf, pme_attr_bsc(bsc_id), showhex);\
} \
static DEVICE_ATTR(bsc_id, perm, pme_show_bsc_##bsc_id, \
			pme_store_bsc_##bsc_id);

/* Statistics Ctrl: update interval */
static ssize_t pme_store_update_interval(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;

	if (!pme2_have_control()) {
		PMEPRERR("not on ctrl-plane\n");
		return -ENODEV;
	}
	if (strict_strtoul(buf, 0, &val)) {
		dev_info(dev, "invalid input %s\n", buf);
		return -EINVAL;
	}
	if (val > MAX_ACCUMULATOR_INTERVAL) {
		dev_info(dev, "invalid input %s\n", buf);
		return -ERANGE;
	}
	accumulator_update_interval(val);
	return count;
}
static ssize_t pme_show_update_interval(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!pme2_have_control())
		return -ENODEV;
	return snprintf(buf, PAGE_SIZE, "%u\n", pme_stat_interval);
}

#define FMT_0HEX "0x%08x\n"
#define FMT_HEX  "0x%x\n"
#define FMT_DEC  "%u\n"
#define PRIV_RO  S_IRUSR
#define PRIV_RW  (S_IRUSR | S_IWUSR)

/* Register Interfaces */
/* read-write; */
PME_SYSFS_ATTR(efqc_int, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(sw_db, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(dmcr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(smcr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(famcr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(kvlts, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(max_chain_length, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(pattern_range_counter_idx, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(pattern_range_counter_mask, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(max_allowed_test_line_per_pattern, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(max_pattern_matches_per_sui, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(max_pattern_evaluations_per_sui, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(report_length_limit, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(end_of_simple_sui_report, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(aim, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(end_of_sui_reaction_ptr, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(sre_pscl, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(sre_max_block_num, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(sre_max_instruction_limit, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(esr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(pehd, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(ecc1bes, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(ecc2bes, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(miace, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(miacr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(cdcr, PRIV_RW, FMT_0HEX);
PME_SYSFS_ATTR(pmtr, PRIV_RW, FMT_DEC);

/* read-only; */
PME_SYSFS_ATTR(max_pdsr_index, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(sre_context_size, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(sre_rule_num, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(sre_session_ctx_num, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(sre_max_index_size, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(sre_max_offset_ctrl, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(src_id, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(liodnr, PRIV_RO, FMT_DEC);
PME_SYSFS_ATTR(rev1, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(rev2, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(isr, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(ecr0, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(ecr1, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(pmstat, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(eccaddr, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(ecccode, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(faconf, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(pdsrbah, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(pdsrbal, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(scbarh, PRIV_RO, FMT_0HEX);
PME_SYSFS_ATTR(scbarl, PRIV_RO, FMT_0HEX);


/* Buffer Pool Size Configuration */
PME_SYSFS_BSC_ATTR(0, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(1, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(2, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(3, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(4, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(5, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(6, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(7, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(8, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(9, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(10, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(11, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(12, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(13, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(14, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(15, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(16, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(17, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(18, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(19, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(20, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(21, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(22, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(23, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(24, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(25, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(26, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(27, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(28, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(29, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(30, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(31, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(32, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(33, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(34, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(35, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(36, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(37, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(38, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(39, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(40, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(41, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(42, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(43, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(44, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(45, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(46, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(47, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(48, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(49, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(50, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(51, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(52, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(53, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(54, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(55, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(56, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(57, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(58, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(59, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(60, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(61, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(62, PRIV_RW, FMT_DEC);
PME_SYSFS_BSC_ATTR(63, PRIV_RW, FMT_DEC);

/* Stats Counters*/
PME_SYSFS_STAT_ATTR(trunci, PRIV_RW);
PME_SYSFS_STAT_ATTR(rbc, PRIV_RW);
PME_SYSFS_STAT_ATTR(tbt0ecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(tbt1ecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(vlt0ecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(vlt1ecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(cmecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(dxcmecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(dxemecc1ec, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnib, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnis, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnth1, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnth2, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnthv, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnths, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnch, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnpm, PRIV_RW);
PME_SYSFS_STAT_ATTR(stns1m, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnpmr, PRIV_RW);
PME_SYSFS_STAT_ATTR(stndsr, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnesr, PRIV_RW);
PME_SYSFS_STAT_ATTR(stns1r, PRIV_RW);
PME_SYSFS_STAT_ATTR(stnob, PRIV_RW);
PME_SYSFS_STAT_ATTR(mia_byc, PRIV_RW);
PME_SYSFS_STAT_ATTR(mia_blc, PRIV_RW);

/* Stats Control */
PME_SYSFS_ATTR(tbt0ecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(tbt1ecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(vlt0ecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(vlt1ecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(cmecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(dxcmecc1th, PRIV_RW, FMT_DEC);
PME_SYSFS_ATTR(dxemecc1th, PRIV_RW, FMT_DEC);

static DEVICE_ATTR(update_interval, (S_IRUSR | S_IWUSR),
		pme_show_update_interval, pme_store_update_interval);

static struct attribute *pme_dev_bsc_attributes[] = {
	&dev_attr_0.attr,
	&dev_attr_1.attr,
	&dev_attr_2.attr,
	&dev_attr_3.attr,
	&dev_attr_4.attr,
	&dev_attr_5.attr,
	&dev_attr_6.attr,
	&dev_attr_7.attr,
	&dev_attr_8.attr,
	&dev_attr_9.attr,
	&dev_attr_10.attr,
	&dev_attr_11.attr,
	&dev_attr_12.attr,
	&dev_attr_13.attr,
	&dev_attr_14.attr,
	&dev_attr_15.attr,
	&dev_attr_16.attr,
	&dev_attr_17.attr,
	&dev_attr_18.attr,
	&dev_attr_19.attr,
	&dev_attr_20.attr,
	&dev_attr_21.attr,
	&dev_attr_22.attr,
	&dev_attr_23.attr,
	&dev_attr_24.attr,
	&dev_attr_25.attr,
	&dev_attr_26.attr,
	&dev_attr_27.attr,
	&dev_attr_28.attr,
	&dev_attr_29.attr,
	&dev_attr_30.attr,
	&dev_attr_31.attr,
	&dev_attr_32.attr,
	&dev_attr_33.attr,
	&dev_attr_34.attr,
	&dev_attr_35.attr,
	&dev_attr_36.attr,
	&dev_attr_37.attr,
	&dev_attr_38.attr,
	&dev_attr_39.attr,
	&dev_attr_40.attr,
	&dev_attr_41.attr,
	&dev_attr_42.attr,
	&dev_attr_43.attr,
	&dev_attr_44.attr,
	&dev_attr_45.attr,
	&dev_attr_46.attr,
	&dev_attr_47.attr,
	&dev_attr_48.attr,
	&dev_attr_49.attr,
	&dev_attr_50.attr,
	&dev_attr_51.attr,
	&dev_attr_52.attr,
	&dev_attr_53.attr,
	&dev_attr_54.attr,
	&dev_attr_55.attr,
	&dev_attr_56.attr,
	&dev_attr_57.attr,
	&dev_attr_58.attr,
	&dev_attr_59.attr,
	&dev_attr_60.attr,
	&dev_attr_61.attr,
	&dev_attr_62.attr,
	&dev_attr_63.attr,
	NULL
};

static struct attribute *pme_dev_attributes[] = {
	&dev_attr_efqc_int.attr,
	&dev_attr_sw_db.attr,
	&dev_attr_dmcr.attr,
	&dev_attr_smcr.attr,
	&dev_attr_famcr.attr,
	&dev_attr_kvlts.attr,
	&dev_attr_max_chain_length.attr,
	&dev_attr_pattern_range_counter_idx.attr,
	&dev_attr_pattern_range_counter_mask.attr,
	&dev_attr_max_allowed_test_line_per_pattern.attr,
	&dev_attr_max_pdsr_index.attr,
	&dev_attr_max_pattern_matches_per_sui.attr,
	&dev_attr_max_pattern_evaluations_per_sui.attr,
	&dev_attr_report_length_limit.attr,
	&dev_attr_end_of_simple_sui_report.attr,
	&dev_attr_aim.attr,
	&dev_attr_sre_context_size.attr,
	&dev_attr_sre_rule_num.attr,
	&dev_attr_sre_session_ctx_num.attr,
	&dev_attr_end_of_sui_reaction_ptr.attr,
	&dev_attr_sre_pscl.attr,
	&dev_attr_sre_max_block_num.attr,
	&dev_attr_sre_max_instruction_limit.attr,
	&dev_attr_sre_max_index_size.attr,
	&dev_attr_sre_max_offset_ctrl.attr,
	&dev_attr_src_id.attr,
	&dev_attr_liodnr.attr,
	&dev_attr_rev1.attr,
	&dev_attr_rev2.attr,
	&dev_attr_isr.attr,
	&dev_attr_ecr0.attr,
	&dev_attr_ecr1.attr,
	&dev_attr_esr.attr,
	&dev_attr_pmstat.attr,
	&dev_attr_pehd.attr,
	&dev_attr_ecc1bes.attr,
	&dev_attr_ecc2bes.attr,
	&dev_attr_eccaddr.attr,
	&dev_attr_ecccode.attr,
	&dev_attr_miace.attr,
	&dev_attr_miacr.attr,
	&dev_attr_cdcr.attr,
	&dev_attr_pmtr.attr,
	&dev_attr_faconf.attr,
	&dev_attr_pdsrbah.attr,
	&dev_attr_pdsrbal.attr,
	&dev_attr_scbarh.attr,
	&dev_attr_scbarl.attr,
	NULL
};

static struct attribute *pme_dev_stats_counter_attributes[] = {
	&dev_attr_trunci.attr,
	&dev_attr_rbc.attr,
	&dev_attr_tbt0ecc1ec.attr,
	&dev_attr_tbt1ecc1ec.attr,
	&dev_attr_vlt0ecc1ec.attr,
	&dev_attr_vlt1ecc1ec.attr,
	&dev_attr_cmecc1ec.attr,
	&dev_attr_dxcmecc1ec.attr,
	&dev_attr_dxemecc1ec.attr,
	&dev_attr_stnib.attr,
	&dev_attr_stnis.attr,
	&dev_attr_stnth1.attr,
	&dev_attr_stnth2.attr,
	&dev_attr_stnthv.attr,
	&dev_attr_stnths.attr,
	&dev_attr_stnch.attr,
	&dev_attr_stnpm.attr,
	&dev_attr_stns1m.attr,
	&dev_attr_stnpmr.attr,
	&dev_attr_stndsr.attr,
	&dev_attr_stnesr.attr,
	&dev_attr_stns1r.attr,
	&dev_attr_stnob.attr,
	&dev_attr_mia_byc.attr,
	&dev_attr_mia_blc.attr,
	NULL
};

static struct attribute *pme_dev_stats_ctrl_attributes[] = {
	&dev_attr_update_interval.attr,
	&dev_attr_tbt0ecc1th.attr,
	&dev_attr_tbt1ecc1th.attr,
	&dev_attr_vlt0ecc1th.attr,
	&dev_attr_vlt1ecc1th.attr,
	&dev_attr_cmecc1th.attr,
	&dev_attr_dxcmecc1th.attr,
	&dev_attr_dxemecc1th.attr,
	NULL
};

/* root level */
static const struct attribute_group pme_dev_attr_grp = {
	.name = NULL,	/* put in device directory */
	.attrs = pme_dev_attributes
};

/* root/bsc */
static struct attribute_group pme_dev_bsc_attr_grp = {
	.name  = "bsc",
	.attrs = pme_dev_bsc_attributes
};

/* root/stats */
static struct attribute_group pme_dev_stats_counters_attr_grp = {
	.name  = "stats",
	.attrs = pme_dev_stats_counter_attributes
};

/* root/stats_ctrl */
static struct attribute_group pme_dev_stats_ctrl_attr_grp = {
	.name  = "stats_ctrl",
	.attrs = pme_dev_stats_ctrl_attributes
};


int pme2_create_sysfs_dev_files(struct platform_device *ofdev)
{
	int ret;

	ret = sysfs_create_group(&ofdev->dev.kobj, &pme_dev_attr_grp);
	if (ret)
		goto done;
	ret = sysfs_create_group(&ofdev->dev.kobj, &pme_dev_bsc_attr_grp);
	if (ret)
		goto del_group_1;
	ret = sysfs_create_group(&ofdev->dev.kobj, &pme_dev_stats_counters_attr_grp);
	if (ret)
		goto del_group_2;
	ret = sysfs_create_group(&ofdev->dev.kobj, &pme_dev_stats_ctrl_attr_grp);
	if (ret)
		goto del_group_3;
	goto done;
del_group_3:
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_stats_counters_attr_grp);
del_group_2:
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_bsc_attr_grp);
del_group_1:
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_attr_grp);
done:
	if (ret)
		dev_err(&ofdev->dev,
				"Cannot create dev attributes  ret=%d\n", ret);
	return ret;
}

void pme2_remove_sysfs_dev_files(struct platform_device *ofdev)
{
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_stats_ctrl_attr_grp);
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_stats_counters_attr_grp);
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_bsc_attr_grp);
	sysfs_remove_group(&ofdev->dev.kobj, &pme_dev_attr_grp);
}


