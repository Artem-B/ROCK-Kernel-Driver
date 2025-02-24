dnl #
dnl # commit b5dee3130bb4014511f5d0dd46855ed843e3fdc8
dnl # PM / sleep: Refactor filesystems sync to reduce duplication
dnl #
AC_DEFUN([AC_AMDGPU_KSYS_SYNC_HELPER],
	[AC_MSG_CHECKING([whether ksys_sync_helper() function is available])
	AC_KERNEL_TRY_COMPILE([
		#include <linux/suspend.h>
	],[
		ksys_sync_helper();
	], [
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_KSYS_SYNC_HELPER, 1, [ksys_sync_helper() is available])
	],[
		AC_MSG_RESULT(no)
	])
])
