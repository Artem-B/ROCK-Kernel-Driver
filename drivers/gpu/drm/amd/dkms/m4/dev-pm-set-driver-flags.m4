dnl #
dnl # commit v4.14-rc4-21-g08810a4119aa
dnl # Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
dnl # Date:   Wed Oct 25 14:12:29 2017 +0200
dnl # PM / core: Add NEVER_SKIP and SMART_PREPARE driver flags
dnl #
AC_DEFUN([AC_AMDGPU_DEV_PM_SET_DRIVER_FLAGS],
	[AC_MSG_CHECKING([whether dev_pm_set_driver_flags() is available])
	AC_KERNEL_TRY_COMPILE([
		#include <linux/device.h>
	],[
		dev_pm_set_driver_flags(NULL, 1);
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_DEV_PM_SET_DRIVER_FLAGS, 1, [dev_pm_set_driver_flags() is available])
	],[
		AC_MSG_RESULT(no)
	])
])
