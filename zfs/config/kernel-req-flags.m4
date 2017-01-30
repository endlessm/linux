dnl #
dnl # 4.10 API
dnl #
dnl # The WRITE_* and READ_SYNC wrappers have been removed, and
dnl # REQ_* flags should be used directly.
dnl #
AC_DEFUN([ZFS_AC_KERNEL_USE_REQ_FLAGS], [
	AC_MSG_CHECKING([whether fops->aio_fsync() exists])
	ZFS_LINUX_TRY_COMPILE([
		#include <linux/fs.h>
	],[
		unsigned int flags __attribute__ ((unused)) = READ_SYNC;
	],[
		AC_MSG_RESULT(no)
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(USE_REQ_FLAGS, 1, [use REQ_* flags directly])
	])
])
