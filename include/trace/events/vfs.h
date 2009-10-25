#undef TRACE_SYSTEM
#define TRACE_SYSTEM vfs

#include <linux/module.h>

#if !defined(_TRACE_VFS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_VFS_H

/*
 * Tracepoint for dirtying an inode:
 */
TRACE_EVENT(dirty_inode,

	TP_PROTO(struct inode *inode, struct task_struct *task),

	TP_ARGS(inode, task),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__array( char,  dev,    16		)
		__array( char,  file,   32		)
	),

	TP_fast_assign(
		if (inode->i_ino || strcmp(inode->i_sb->s_id, "bdev")) {
			struct dentry *dentry;
			const char *name = "?";

			dentry = d_find_alias(inode);
			if (dentry) {
				spin_lock(&dentry->d_lock);
				name = (const char *) dentry->d_name.name;
			}

			memcpy(__entry->comm, task->comm, TASK_COMM_LEN);
			__entry->pid = task->pid;
			strlcpy(__entry->file, name, 32);
			strlcpy(__entry->dev, inode->i_sb->s_id, 16);

			if (dentry) {
				spin_unlock(&dentry->d_lock);
				dput(dentry);
			}
		}
	),

	TP_printk("task=%i (%s) file=%s dev=%s",
		__entry->pid, __entry->comm, __entry->file, __entry->dev)
);

#endif /* _TRACE_VFS_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
