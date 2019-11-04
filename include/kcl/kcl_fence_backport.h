#ifndef AMDKCL_FENCE_BACKPORT_H
#define AMDKCL_FENCE_BACKPORT_H
#include <kcl/kcl_fence.h>

/*
 * commit v4.18-rc2-533-g418cc6ca0607
 * dma-fence: Allow wait_any_timeout for all fences)
 */
#if DRM_VERSION_CODE < DRM_VERSION(4, 19, 0)
#define dma_fence_wait_any_timeout _kcl_fence_wait_any_timeout
#endif

/*
 * commit  v4.9-rc2-472-gbcc004b629d2
 * dma-buf/fence: make timeout handling in fence_default_wait consistent (v2))
 */
#if DRM_VERSION_CODE < DRM_VERSION(4, 10, 0)
#define dma_fence_default_wait _kcl_fence_default_wait
#endif

/*
 * commit v4.9-rc2-473-g698c0f7ff216
 * dma-buf/fence: revert "don't wait when specified timeout is zero" (v2)
 */
#if DRM_VERSION_CODE < DRM_VERSION(4, 10, 0)
#define dma_fence_wait_timeout _kcl_fence_wait_timeout
#endif
#endif
