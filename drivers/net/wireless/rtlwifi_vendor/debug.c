/******************************************************************************
 *
 * Copyright(c) 2009-2012  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *****************************************************************************/

#include "wifi.h"
#include "cam.h"

#include <linux/moduleparam.h>
#include <linux/vmalloc.h>

void rtl_dbgp_flag_init(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 i;

	for (i = 0; i < DBGP_TYPE_MAX; i++)
		rtlpriv->dbg.dbgp_type[i] = 0;

	/*Init Debug flag enable condition */
}
EXPORT_SYMBOL_GPL(rtl_dbgp_flag_init);

#ifdef CONFIG_RTLWIFI_DEBUG
void _rtl_dbg_trace(struct rtl_priv *rtlpriv, u64 comp, int level,
		    const char *func, const char *fmt, ...)
{
	if (unlikely((comp & rtlpriv->dbg.global_debug_mask) &&
		     (level <= rtlpriv->dbg.global_debuglevel))) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;

		pr_debug("%s %pV", func, &vaf);

		va_end(args);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_trace);

void _rtl_dbg_print(struct rtl_priv *rtlpriv, u64 comp, int level,
		    const char *fmt, ...)
{
	if (unlikely((comp & rtlpriv->dbg.global_debug_mask) &&
		     (level <= rtlpriv->dbg.global_debuglevel))) {
		struct va_format vaf;
		va_list args;

		va_start(args, fmt);

		vaf.fmt = fmt;
		vaf.va = &args;

		pr_debug("%pV", &vaf);

		va_end(args);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_print);

void _rtl_dbg_print_data(struct rtl_priv *rtlpriv, u64 comp, int level,
			 const char *titlestring,
			 const void *hexdata, int hexdatalen)
{
	if (unlikely(((comp) & rtlpriv->dbg.global_debug_mask) &&
		     ((level) <= rtlpriv->dbg.global_debuglevel))) {
		pr_info("In process \"%s\" (pid %i): %s\n",
			 current->comm, current->pid, titlestring);
		print_hex_dump_bytes("", DUMP_PREFIX_NONE,
				     hexdata, hexdatalen);
	}
}
EXPORT_SYMBOL_GPL(_rtl_dbg_print_data);

#endif

static struct dentry *debugfs_topdir;
static int _rtl_debug_get_mac_page_x(struct seq_file *m, void *v, int page)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int i, n;
	int max = 0xff;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_read_dword(rtlpriv, (page | n)));
	}
	seq_puts(m, "\n");
	return 0;
}

static int rtl_debug_get_mac_0(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0000);
}

static int dl_debug_open_mac_0(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_0, inode->i_private);
}

static const struct file_operations file_ops_mac_0 = {
	.open = dl_debug_open_mac_0,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_1(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0100);
}

static int dl_debug_open_mac_1(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_1, inode->i_private);
}

static const struct file_operations file_ops_mac_1 = {
	.open = dl_debug_open_mac_1,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_2(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0200);
}

static int dl_debug_open_mac_2(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_2, inode->i_private);
}

static const struct file_operations file_ops_mac_2 = {
	.open = dl_debug_open_mac_2,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_3(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0300);

}

static int dl_debug_open_mac_3(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_3, inode->i_private);
}

static const struct file_operations file_ops_mac_3 = {
	.open = dl_debug_open_mac_3,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_4(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0400);
}

static int dl_debug_open_mac_4(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_4, inode->i_private);
}

static const struct file_operations file_ops_mac_4 = {
	.open = dl_debug_open_mac_4,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_5(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0500);
}

static int dl_debug_open_mac_5(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_5, inode->i_private);
}

static const struct file_operations file_ops_mac_5 = {
	.open = dl_debug_open_mac_5,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_6(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0600);
}

static int dl_debug_open_mac_6(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_6, inode->i_private);
}

static const struct file_operations file_ops_mac_6 = {
	.open = dl_debug_open_mac_6,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_7(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0700);
}

static int dl_debug_open_mac_7(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_7, inode->i_private);
}

static const struct file_operations file_ops_mac_7 = {
	.open = dl_debug_open_mac_7,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_10(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1000);
}

static int dl_debug_open_mac_10(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_10, inode->i_private);
}

static const struct file_operations file_ops_mac_10 = {
	.open = dl_debug_open_mac_10,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_11(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1100);
}

static int dl_debug_open_mac_11(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_11, inode->i_private);
}

static const struct file_operations file_ops_mac_11 = {
	.open = dl_debug_open_mac_11,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_12(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1200);
}

static int dl_debug_open_mac_12(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_12, inode->i_private);
}

static const struct file_operations file_ops_mac_12 = {
	.open = dl_debug_open_mac_12,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_13(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1300);
}

static int dl_debug_open_mac_13(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_13, inode->i_private);
}

static const struct file_operations file_ops_mac_13 = {
	.open = dl_debug_open_mac_13,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_14(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1400);
}

static int dl_debug_open_mac_14(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_14, inode->i_private);
}

static const struct file_operations file_ops_mac_14 = {
	.open = dl_debug_open_mac_14,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_15(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1500);
}

static int dl_debug_open_mac_15(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_15, inode->i_private);
}

static const struct file_operations file_ops_mac_15 = {
	.open = dl_debug_open_mac_15,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_16(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1600);
}

static int dl_debug_open_mac_16(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_16, inode->i_private);
}

static const struct file_operations file_ops_mac_16 = {
	.open = dl_debug_open_mac_16,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_mac_17(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x1700);
}

static int dl_debug_open_mac_17(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_mac_17, inode->i_private);
}

static const struct file_operations file_ops_mac_17 = {
	.open = dl_debug_open_mac_17,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_8(struct seq_file *m, void *v)
{
	return _rtl_debug_get_mac_page_x(m, v, 0x0800);
}

static int dl_debug_open_bb_8(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_8, inode->i_private);
}

static const struct file_operations file_ops_bb_8 = {
	.open = dl_debug_open_bb_8,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_9(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0x900;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_9(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_9, inode->i_private);
}

static const struct file_operations file_ops_bb_9 = {
	.open = dl_debug_open_bb_9,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_a(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xa00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_a(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_a, inode->i_private);
}

static const struct file_operations file_ops_bb_a = {
	.open = dl_debug_open_bb_a,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_b(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xb00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_b(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_b, inode->i_private);
}

static const struct file_operations file_ops_bb_b = {
	.open = dl_debug_open_bb_b,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_c(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xc00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_c(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_c, inode->i_private);
}

static const struct file_operations file_ops_bb_c = {
	.open = dl_debug_open_bb_c,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_d(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xd00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_d(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_d, inode->i_private);
}

static const struct file_operations file_ops_bb_d = {
	.open = dl_debug_open_bb_d,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_e(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xe00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_e(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_e, inode->i_private);
}

static const struct file_operations file_ops_bb_e = {
	.open = dl_debug_open_bb_e,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_f(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	int i, n, page;
	int max = 0xff;

	page = 0xf00;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_bb_f(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_f, inode->i_private);
}

static const struct file_operations file_ops_bb_f = {
	.open = dl_debug_open_bb_f,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_bb_page_x(struct seq_file *m, void *v, int page, int max)
{
	struct ieee80211_hw *hw = m->private;
	int i, n;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtl_get_bbreg(hw, (page | n), 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int rtl_debug_get_bb_1XXX(struct seq_file *m, void *v)
{
	return rtl_debug_get_bb_page_x(m, v, 0x1800, 0x7ff);
}

static int dl_debug_open_bb_1XXX(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_bb_1XXX, inode->i_private);
}

static const struct file_operations file_ops_bb_1XXX = {
	.open = dl_debug_open_bb_1XXX,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_reg_rf_a(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	int i, n;
	int max = 0x40;

	if (rtlhal->hw_type == HARDWARE_TYPE_RTL8822BE)
		max = 0xff;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n);
		for (i = 0; i < 4 && n <= max; n += 1, i++)
			seq_printf(m, "%8.8x    ",
				rtl_get_rfreg(hw, RF90_PATH_A,
				n, 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_rf_a(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_reg_rf_a, inode->i_private);
}

static const struct file_operations file_ops_rf_a = {
	.open = dl_debug_open_rf_a,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_reg_rf_b(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	int i, n;
	int max = 0x40;

	if (rtlhal->hw_type == HARDWARE_TYPE_RTL8822BE)
		max = 0xff;

	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n);
		for (i = 0; i < 4 && n <= max; n += 1, i++)
			seq_printf(m, "%8.8x    ",
				   rtl_get_rfreg(hw, RF90_PATH_B, n,
						 0xffffffff));
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_rf_b(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_reg_rf_b, inode->i_private);
}

static const struct file_operations file_ops_rf_b = {
	.open = dl_debug_open_rf_b,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_cam_register_1(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 target_cmd = 0;
	u32 target_val = 0;
	u8 entry_i = 0;
	u32 ulstatus;
	int i = 100, j = 0;

	/* This dump the current register page */
	seq_puts(m,
	    "\n#################### SECURITY CAM (0-10) ##################\n ");

	for (j = 0; j < 11; j++) {
		seq_printf(m, "\nD:  %2x > ", j);
		for (entry_i = 0; entry_i < CAM_CONTENT_COUNT; entry_i++) {
			/* polling bit, and No Write enable, and address  */
			target_cmd = entry_i + CAM_CONTENT_COUNT * j;
			target_cmd = target_cmd | BIT(31);

			/* Check polling bit is clear */
			while ((i--) >= 0) {
				ulstatus = rtl_read_dword(rtlpriv,
						rtlpriv->cfg->maps[RWCAM]);
				if (ulstatus & BIT(31))
					continue;
				else
					break;

			}

			rtl_write_dword(rtlpriv, rtlpriv->cfg->maps[RWCAM],
					target_cmd);
			target_val = rtl_read_dword(rtlpriv,
						    rtlpriv->cfg->maps[RCAMO]);
			seq_printf(m, "%8.8x ", target_val);
		}
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_cam_1(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_cam_register_1,
			   inode->i_private);
}

static const struct file_operations file_ops_cam_1 = {
	.open = dl_debug_open_cam_1,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_cam_register_2(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 target_cmd = 0;
	u32 target_val = 0;
	u8 entry_i = 0;
	u32 ulstatus;
	int i = 100, j = 0;

	/* This dump the current register page */
	seq_puts(m,
	    "\n################### SECURITY CAM (11-21) ##################\n ");

	for (j = 11; j < 22; j++) {
		seq_printf(m, "\nD:  %2x > ", j);
		for (entry_i = 0; entry_i < CAM_CONTENT_COUNT; entry_i++) {
			target_cmd = entry_i + CAM_CONTENT_COUNT * j;
			target_cmd = target_cmd | BIT(31);

			while ((i--) >= 0) {
				ulstatus = rtl_read_dword(rtlpriv,
						rtlpriv->cfg->maps[RWCAM]);
				if (ulstatus & BIT(31))
					continue;
				else
					break;

			}

			rtl_write_dword(rtlpriv, rtlpriv->cfg->maps[RWCAM],
					target_cmd);
			target_val = rtl_read_dword(rtlpriv,
						    rtlpriv->cfg->maps[RCAMO]);
			seq_printf(m, "%8.8x ", target_val);
		}
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_cam_2(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_cam_register_2,
			   inode->i_private);
}

static const struct file_operations file_ops_cam_2 = {
	.open = dl_debug_open_cam_2,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_cam_register_3(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 target_cmd = 0;
	u32 target_val = 0;
	u8 entry_i = 0;
	u32 ulstatus;
	int i = 100, j = 0;

	/* This dump the current register page */
	seq_puts(m,
	    "\n################### SECURITY CAM (22-31) ##################\n ");

	for (j = 22; j < TOTAL_CAM_ENTRY; j++) {
		seq_printf(m, "\nD:  %2x > ", j);
		for (entry_i = 0; entry_i < CAM_CONTENT_COUNT; entry_i++) {
			target_cmd = entry_i+CAM_CONTENT_COUNT*j;
			target_cmd = target_cmd | BIT(31);

			while ((i--) >= 0) {
				ulstatus = rtl_read_dword(rtlpriv,
						rtlpriv->cfg->maps[RWCAM]);
				if (ulstatus & BIT(31))
					continue;
				else
					break;
			}

			rtl_write_dword(rtlpriv, rtlpriv->cfg->maps[RWCAM],
					target_cmd);
			target_val = rtl_read_dword(rtlpriv,
						    rtlpriv->cfg->maps[RCAMO]);
			seq_printf(m, "%8.8x ", target_val);
		}
	}
	seq_puts(m, "\n");
	return 0;
}

static int dl_debug_open_cam_3(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_cam_register_3,
			   inode->i_private);
}

static const struct file_operations file_ops_cam_3 = {
	.open = dl_debug_open_cam_3,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int rtl_debug_get_btcoex(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 *buff;
	u32 size = 30 * 100;
	int n;

	buff = kzalloc(size, GFP_KERNEL);

	if (buff == NULL)
		return 0;

	if (rtlpriv->cfg->ops->get_btc_status())
		rtlpriv->btcoexist.btc_ops->btc_display_bt_coex_info(buff,
								     size);

	n = strlen(buff);

	buff[n++] = '\n';
	buff[n++] = '\0';

	seq_write(m, buff, n);

	return 0;
}

static int dl_debug_open_btcoex(struct inode *inode, struct file *file)
{
	return single_open(file, rtl_debug_get_btcoex,
					   inode->i_private);
}

static const struct file_operations file_ops_btcoex = {
	.open = dl_debug_open_btcoex,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};


static ssize_t rtl_debugfs_set_write_reg(struct file *filp,
		const char __user *buffer, size_t count, loff_t *loff)
{
	struct ieee80211_hw *hw = filp->private_data;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	char tmp[32];
	u32 addr, val, len;

	if (count < 3)
	{
		/*printk("argument size is less than 3\n");*/
		return -EFAULT;
	}

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {

		int num = sscanf(tmp, "%x %x %x", &addr, &val, &len);

		if (num !=  3) {
			/*printk("invalid write_reg parameter!\n");*/
			return count;
		}

		switch (len)
		{
			case 1:
				rtl_write_byte(rtlpriv, addr, (u8)val);
				break;
			case 2:
				rtl_write_word(rtlpriv, addr, (u16)val);
				break;
			case 4:
				rtl_write_dword(rtlpriv, addr, val);
				break;
			default:
				/*printk("error write length=%d", len);*/
				break;
		}

	}

	return count;
}

static int rtl_debugfs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;

	return 0;
}

static int rtl_debugfs_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations file_ops_write_reg = {
	.owner = THIS_MODULE,
	.write = rtl_debugfs_set_write_reg,
	.open = rtl_debugfs_open,
	.release = rtl_debugfs_close,
};

static ssize_t rtl_debugfs_phydm_cmd(struct file *filp,
		const char __user *buffer, size_t count, loff_t *loff)
{
	struct ieee80211_hw *hw = filp->private_data;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	char tmp[64];

	if (!rtlpriv->dbg.msg_buf)
		return -ENOMEM;

	if (!rtlpriv->phydm.ops)
		return -EFAULT;

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {

		tmp[count] = '\0';

		rtlpriv->phydm.ops->phydm_debug_cmd(rtlpriv, tmp, count,
						    rtlpriv->dbg.msg_buf,
						    80*25);
	}

	return count;
}

static int rtl_debug_get_phydm_cmd(struct seq_file *m, void *v)
{
	struct ieee80211_hw *hw = m->private;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (rtlpriv->dbg.msg_buf)
		seq_puts(m, rtlpriv->dbg.msg_buf);

	return 0;
}

static int rtl_debugfs_open_rw(struct inode *inode, struct file *filp)
{
	if (filp->f_mode & FMODE_READ)
		single_open(filp, rtl_debug_get_phydm_cmd,
						   inode->i_private);
	else
		filp->private_data = inode->i_private;

	return 0;
}

static int rtl_debugfs_close_rw(struct inode *inode, struct file *filp)
{
	if (filp->f_mode == FMODE_READ)
		seq_release(inode, filp);

	return 0;
}

static const struct file_operations file_ops_phydm_cmd = {
	.owner = THIS_MODULE,
	.open = rtl_debugfs_open_rw,
	.release = rtl_debugfs_close_rw,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = rtl_debugfs_phydm_cmd,
};

void rtl_debug_add_one(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct dentry *entry1;

	rtlpriv->dbg.msg_buf = vzalloc(80 * 25);

	snprintf(rtlpriv->dbg.debugfs_name, 18, "%02x-%02x-%02x-%02x-%02x-%02x",
		rtlefuse->dev_addr[0], rtlefuse->dev_addr[1],
		rtlefuse->dev_addr[2], rtlefuse->dev_addr[3],
		rtlefuse->dev_addr[4], rtlefuse->dev_addr[5]);

	rtlpriv->dbg.debugfs_dir =
		debugfs_create_dir(rtlpriv->dbg.debugfs_name, debugfs_topdir);
	if (!rtlpriv->dbg.debugfs_dir) {
		pr_err("Unable to init debugfs:/%s/%s\n", rtlpriv->cfg->name,
		       rtlpriv->dbg.debugfs_name);
		return;
	}

	entry1 = debugfs_create_file("mac-0", S_IFREG | S_IRUGO,
			  rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_0);
	if (!entry1)
		pr_err("Unable to initialize debugfs:/%s/%s/mac-0\n",
		       rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-1", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_1);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-1\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-2", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_2);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-2\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-3", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_3);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-3\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-4", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_4);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-4\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-5", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_5);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-5\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-6", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_6);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-6\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-7", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_7);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-7\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-10", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_10);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-10\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-11", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_11);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-11\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-12", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_12);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-12\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-13", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_13);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-13\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-14", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_14);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-14\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-15", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_15);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-15\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-16", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_16);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-16\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("mac-17", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_mac_17);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/mac-17\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-8", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_8);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-8\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-9", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_9);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-9\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-a", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_a);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-a\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-b", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_b);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-b\n",
		      rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-c", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_c);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-c\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-d", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_d);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-d\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-e", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_e);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-e\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-f", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_f);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-f\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("bb-1xxx", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_bb_1XXX);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/bb-1xxx\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("rf-a", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_rf_a);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/rf-a\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("rf-b", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_rf_b);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/rf-b\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("cam-1", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_cam_1);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/cam-1\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("cam-2", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_cam_2);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/cam-2\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("cam-3", S_IFREG | S_IRUGO,
				 rtlpriv->dbg.debugfs_dir, hw, &file_ops_cam_3);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
			 "Unable to initialize debugfs:/%s/%s/cam-3\n",
			  rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("btcoex", S_IFREG | S_IRUGO,
				rtlpriv->dbg.debugfs_dir, hw, &file_ops_btcoex);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
				 "Unable to initialize debugfs:/%s/%s/btcoex\n",
				 rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("write_reg", S_IFREG | S_IWUGO,
				rtlpriv->dbg.debugfs_dir, hw,
				&file_ops_write_reg);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
				"Unable to initialize debugfs:/%s/%s/write_reg\n",
				rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

	entry1 = debugfs_create_file("phydm_cmd", S_IFREG | S_IWUGO | S_IRUGO,
				rtlpriv->dbg.debugfs_dir, hw, &file_ops_phydm_cmd);
	if (!entry1)
		RT_TRACE(rtlpriv, COMP_INIT, COMP_ERR,
				"Unable to initialize debugfs:/%s/%s/phydm_cmd\n",
				rtlpriv->cfg->name, rtlpriv->dbg.debugfs_name);

}
EXPORT_SYMBOL_GPL(rtl_debug_add_one);

void rtl_debug_remove_one(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	debugfs_remove_recursive(rtlpriv->dbg.debugfs_dir);
	rtlpriv->dbg.debugfs_dir = NULL;

	vfree(rtlpriv->dbg.msg_buf);
}
EXPORT_SYMBOL_GPL(rtl_debug_remove_one);

void rtl_debugfs_add_topdir(void)
{
	debugfs_topdir = debugfs_create_dir("rtlwifi", NULL);
}

void rtl_debugfs_remove_topdir(void)
{
	debugfs_remove_recursive(debugfs_topdir);
}
