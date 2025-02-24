dnl #
dnl # commit cfe63423d9be3e7020296c3dfb512768a83cd099
dnl # drm/fb-helper: Add drm_fb_helper_set_suspend_unlocked()
dnl #
AC_DEFUN([AC_AMDGPU_DRM_FB_HELPER_SET_SUSPEND_UNLOCKED],
	[AC_MSG_CHECKING([whether drm_fb_helper_set_suspend_unlocked() is available])
	AC_KERNEL_TEST_HEADER_FILE_EXIST([drm/drmP.h], [
		AC_KERNEL_TRY_COMPILE([
			#include <drm/drmP.h>
			#include <drm/drm_fb_helper.h>
		],[
			drm_fb_helper_set_suspend_unlocked(NULL,0);
		],[
			AC_MSG_RESULT(yes)
			AC_DEFINE(HAVE_DRM_FB_HELPER_SET_SUSPEND_UNLOCKED, 1, [drm_fb_helper_set_suspend_unlocked() is available])
		],[
			AC_MSG_RESULT(no)
		])
	], [
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_DRM_FB_HELPER_SET_SUSPEND_UNLOCKED, 1, [drm_fb_helper_set_suspend_unlocked() is available])
	])
])
