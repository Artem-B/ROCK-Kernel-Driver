dnl #
dnl # commit v5.3-rc1-29-g2c7933f53f6b
dnl # mm/mmu_notifiers: add a get/put scheme for the registration
dnl #
dnl # amdkcl: mmu_notifier_put() & mmu_notifier_synchronize() is
dnl # introduced in the same commit, yet rhel7.7 has different behavior
dnl #
AC_DEFUN([AC_AMDGPU_MMU_NOTIFIER_PUT],
	[AC_MSG_CHECKING([whether mmu_notifier_put() is available])
	AC_KERNEL_TRY_COMPILE([
		#include <linux/mmu_notifier.h>
	],[
		mmu_notifier_put(NULL);
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_MMU_NOTIFIER_PUT, 1, [mmu_notifier_put() is available])
	],[
		AC_MSG_RESULT(no)
	])
])

AC_DEFUN([AC_AMDGPU_MMU_NOTIFIER_SYNCHRONIZE],
	[AC_MSG_CHECKING([whether mmu_notifier_synchronize() is available])
	AC_KERNEL_TRY_COMPILE([
		#include <linux/mmu_notifier.h>
	],[
		mmu_notifier_synchronize();
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_MMU_NOTIFIER_SYNCHRONIZE, 1, [mmu_notifier_synchronize() is available])
	],[
		AC_MSG_RESULT(no)
	])
	AC_AMDGPU_MMU_NOTIFIER_PUT
])
