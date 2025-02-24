dnl #
dnl # commit 4e544bac8267f65a0bf06aed1bde9964da4812ed
dnl # PCI: Add pci_dev_id() helper
dnl #
AC_DEFUN([AC_AMDGPU_PCI_DEV_ID], [
	AC_MSG_CHECKING([whether pci_dev_id() is available])
	AC_KERNEL_TRY_COMPILE([
		#include <linux/pci.h>
	], [
		pci_dev_id(NULL);
	], [
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_PCI_DEV_ID, 1, [pci_dev_id() is available])
	], [
		AC_MSG_RESULT(no)
	])
])
