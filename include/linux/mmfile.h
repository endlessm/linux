#ifndef H_LINUX_MMFILE_H
#define H_LINUX_MMFILE_H

#include <linux/dcache.h>
#include <linux/file.h>
#include <linux/fs.h>

/*
 * Mainly for aufs which mmap(2) diffrent file and wants to print different path
 * in /proc/PID/maps.
 */
/* #define AUFS_DEBUG_MMAP */
static inline void aufs_trace(struct file *f, struct file *pr,
			      const char func[], int line, const char func2[])
{
#ifdef AUFS_DEBUG_MMAP
	if (pr)
		pr_info("%s:%d: %s, %p\n", func, line, func2,
			f ? (char *)f->f_dentry->d_name.name : "(null)");
#endif
}

static inline struct file *vmr_do_pr_or_file(struct vm_region *region,
					     const char func[], int line)
{
	struct file *f = region->vm_file, *pr = region->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	return (f && pr) ? pr : f;
}

static inline void vmr_do_fput(struct vm_region *region,
			       const char func[], int line)
{
	struct file *f = region->vm_file, *pr = region->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	fput(f);
	if (f && pr)
		fput(pr);
}

static inline void vma_do_file_update_time(struct vm_area_struct *vma,
					   const char func[], int line)
{
	struct file *f = vma->vm_file, *pr = vma->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	file_update_time(f);
	if (f && pr)
		file_update_time(pr);
}

static inline struct file *vma_do_pr_or_file(struct vm_area_struct *vma,
					     const char func[], int line)
{
	struct file *f = vma->vm_file, *pr = vma->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	return (f && pr) ? pr : f;
}

static inline void vma_do_get_file(struct vm_area_struct *vma,
				   const char func[], int line)
{
	struct file *f = vma->vm_file, *pr = vma->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	get_file(f);
	if (f && pr)
		get_file(pr);
}

static inline void vma_do_fput(struct vm_area_struct *vma,
			       const char func[], int line)
{
	struct file *f = vma->vm_file, *pr = vma->vm_prfile;

	aufs_trace(f, pr, func, line, __func__);
	fput(f);
	if (f && pr)
		fput(pr);
}

#define vmr_pr_or_file(region)		vmr_do_pr_or_file(region, __func__, \
							  __LINE__)
#define vmr_fput(region)		vmr_do_fput(region, __func__, __LINE__)
#define vma_file_update_time(vma)	vma_do_file_update_time(vma, __func__, \
								__LINE__)
#define vma_pr_or_file(vma)		vma_do_pr_or_file(vma, __func__, \
							  __LINE__)
#define vma_get_file(vma)		vma_do_get_file(vma, __func__, __LINE__)
#define vma_fput(vma)			vma_do_fput(vma, __func__, __LINE__)

#endif
