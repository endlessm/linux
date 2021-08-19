#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

static int version_signature_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", CONFIG_VERSION_SIGNATURE);
	return 0;
}

static int version_signature_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_signature_proc_show, NULL);
}

static const struct proc_ops version_signature_proc_fops = {
	.proc_open	= version_signature_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init proc_version_signature_init(void)
{
	proc_create("version_signature", 0, NULL, &version_signature_proc_fops);
	return 0;
}
module_init(proc_version_signature_init);
