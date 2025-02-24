dnl #
dnl # commit v5.3-rc3-2032-g4d85f45c73a2
dnl # drm/atomic: Rename crtc_state->pageflip_flags to async_flip
dnl #
AC_DEFUN([AC_AMDGPU_STRUCT_DRM_CRTC_STATE],
	[AC_MSG_CHECKING([whether struct drm_crtc_state->async_flip is available])
	AC_KERNEL_TRY_COMPILE([
		#include <drm/drm_crtc.h>
	], [
		struct drm_crtc_state *crtc_state = NULL;
		crtc_state->async_flip = 0;
	],[
		AC_MSG_RESULT(yes)
		AC_DEFINE(HAVE_STRUCT_DRM_CRTC_STATE_ASYNC_FLIP, 1,
		[struct drm_crtc_state->async_flip is available])
	],[
		AC_MSG_RESULT(no)
	])
])
