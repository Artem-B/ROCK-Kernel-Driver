#ifndef AMDKCL_DRM_H
#define AMDKCL_DRM_H

#include <linux/version.h>
#include <linux/kconfig.h>
#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_rect.h>
#include <drm/drm_modes.h>
#include <linux/ctype.h>
#if defined(HAVE_DRM_PRINTF)
#include <drm/drm_print.h>
#endif
#if defined(HAVE_CHUNK_ID_SYNOBJ_IN_OUT)
#include <drm/drm_syncobj.h>
#endif
#if defined(HAVE_DRM_COLOR_LUT_SIZE)
#include <drm/drm_color_mgmt.h>
#endif

#ifndef DRM_MODE_ROTATE_0
#define DRM_MODE_ROTATE_0       (1<<0)
#endif
#ifndef DRM_MODE_ROTATE_90
#define DRM_MODE_ROTATE_90      (1<<1)
#endif
#ifndef DRM_MODE_ROTATE_180
#define DRM_MODE_ROTATE_180     (1<<2)
#endif
#ifndef DRM_MODE_ROTATE_270
#define DRM_MODE_ROTATE_270     (1<<3)
#endif

#ifndef DRM_MODE_ROTATE_MASK
#define DRM_MODE_ROTATE_MASK (\
		DRM_MODE_ROTATE_0  | \
		DRM_MODE_ROTATE_90  | \
		DRM_MODE_ROTATE_180 | \
		DRM_MODE_ROTATE_270)
#endif

extern void (*_kcl_drm_fb_helper_set_suspend)(struct drm_fb_helper *fb_helper, int state);
extern void
(*_kcl_drm_atomic_helper_update_legacy_modeset_state)(struct drm_device *dev,
					      struct drm_atomic_state *old_state);

#if !defined(HAVE_DRM_MODESET_LOCK_ALL_CTX)
int drm_modeset_lock_all_ctx(struct drm_device *dev,
			     struct drm_modeset_acquire_ctx *ctx);
#endif

#if !defined(HAVE_DRM_ATOMIC_HELPER_DISABLE_ALL)
int drm_atomic_helper_disable_all(struct drm_device *dev,
				  struct drm_modeset_acquire_ctx *ctx);
#endif

#if !defined(HAVE_DRM_ATOMIC_HELPER_DUPLICATE_STATE)
struct drm_atomic_state *
drm_atomic_helper_duplicate_state(struct drm_device *dev,
				  struct drm_modeset_acquire_ctx *ctx);
#endif

#if !defined(HAVE_DRM_ATOMIC_HELPER_SUSPEND)
struct drm_atomic_state *drm_atomic_helper_suspend(struct drm_device *dev);
#endif

#if !defined(HAVE_DRM_ATOMIC_HELPER_RESUME)
int drm_atomic_helper_resume(struct drm_device *dev,
			     struct drm_atomic_state *state);
#endif

#if !defined(HAVE_DRM_CRTC_FORCE_DISABLE_ALL)
extern int drm_crtc_force_disable_all(struct drm_device *dev);
#endif

#if !defined(HAVE_DRM_FB_HELPER_REMOVE_CONFLICTING_FRAMEBUFFERS)

#if !defined(IS_REACHABLE)
#define __ARG_PLACEHOLDER_1 0,
#define __take_second_arg(__ignored, val, ...) val

/*
 * The use of "&&" / "||" is limited in certain expressions.
 * The followings enable to calculate "and" / "or" with macro expansion only.
 */
#define __and(x, y)			___and(x, y)
#define ___and(x, y)			____and(__ARG_PLACEHOLDER_##x, y)
#define ____and(arg1_or_junk, y)	__take_second_arg(arg1_or_junk y, 0)

#define __or(x, y)			___or(x, y)
#define ___or(x, y)			____or(__ARG_PLACEHOLDER_##x, y)
#define ____or(arg1_or_junk, y)		__take_second_arg(arg1_or_junk 1, y)

#define IS_REACHABLE(option) __or(IS_BUILTIN(option), \
				__and(IS_MODULE(option), __is_defined(MODULE)))
#endif

#if !defined(HAVE_REMOVE_CONFLICTING_FRAMEBUFFERS_RETURNS_INT)
static inline void
drm_fb_helper_remove_conflicting_framebuffers(struct apertures_struct *a,
					      const char *name, bool primary)
{
#if IS_REACHABLE(CONFIG_FB)
	remove_conflicting_framebuffers(a, name, primary);
#else
	return;
#endif
}
#else
static inline int
drm_fb_helper_remove_conflicting_framebuffers(struct apertures_struct *a,
					      const char *name, bool primary)
{
#if IS_REACHABLE(CONFIG_FB)
	return remove_conflicting_framebuffers(a, name, primary);
#else
	return 0;
#endif
}
#endif
#endif

static inline void kcl_drm_fb_helper_set_suspend(struct drm_fb_helper *fb_helper, int state)
{
#ifdef BUILD_AS_DKMS
	_kcl_drm_fb_helper_set_suspend(fb_helper, state);
#else
	drm_fb_helper_set_suspend(fb_helper, state);
#endif
}

#ifndef HAVE_DRM_FB_HELPER_FILL_INFO
void drm_fb_helper_fill_info(struct fb_info *info,
			     struct drm_fb_helper *fb_helper,
			     struct drm_fb_helper_surface_size *sizes);
#endif

static inline void
kcl_drm_atomic_helper_update_legacy_modeset_state(struct drm_device *dev,
					      struct drm_atomic_state *old_state)
{
#ifdef BUILD_AS_DKMS
	_kcl_drm_atomic_helper_update_legacy_modeset_state(dev, old_state);
#else
	drm_atomic_helper_update_legacy_modeset_state(dev, old_state);
#endif
}

#ifndef DRM_DEBUG_VBL
#define DRM_UT_VBL		0x20
#define DRM_DEBUG_VBL(fmt, args...)					\
	do {								\
		if (unlikely(drm_debug & DRM_UT_VBL))			\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)
#endif

static inline bool kcl_drm_arch_can_wc_memory(void)
{
#if defined(CONFIG_PPC) && !defined(CONFIG_NOT_COHERENT_CACHE)
	return false;
#elif defined(CONFIG_MIPS) && defined(CONFIG_CPU_LOONGSON3)
	return false;
#else
	return true;
#endif
}

#if defined(HAVE_CHUNK_ID_SYNOBJ_IN_OUT)
static inline int kcl_drm_syncobj_find_fence(struct drm_file *file_private,
						u32 handle, u64 point, u64 flags,
						struct dma_fence **fence)
{
#if defined(HAVE_DRM_SYNCOBJ_FENCE_GET)
	return drm_syncobj_fence_get(file_private, handle, fence);
#elif defined(HAVE_3ARGS_DRM_SYNCOBJ_FIND_FENCE)
	return drm_syncobj_find_fence(file_private, handle, fence);
#elif defined(HAVE_4ARGS_DRM_SYNCOBJ_FIND_FENCE)
	return drm_syncobj_find_fence(file_private, handle, point, fence);
#else
	return drm_syncobj_find_fence(file_private, handle, point, flags, fence);
#endif
}
#endif

#if defined(HAVE_DRM_COLOR_LUT) && !defined(HAVE_DRM_COLOR_LUT_SIZE)
/**
 * drm_color_lut_size - calculate the number of entries in the LUT
 * @blob: blob containing the LUT
 *
 * Returns:
 * The number of entries in the color LUT stored in @blob.
 */
static inline int drm_color_lut_size(const struct drm_property_blob *blob)
{
	return blob->length / sizeof(struct drm_color_lut);
}
#endif

static inline int kcl_drm_encoder_init(struct drm_device *dev,
		      struct drm_encoder *encoder,
		      const struct drm_encoder_funcs *funcs,
		      int encoder_type, const char *name, ...)
{
#if defined(HAVE_DRM_ENCODER_INIT_VALID_WITH_NAME)
	return drm_encoder_init(dev, encoder, funcs,
			 encoder_type, name);
#else
	return drm_encoder_init(dev, encoder, funcs,
			 encoder_type);
#endif
}

static inline int kcl_drm_crtc_init_with_planes(struct drm_device *dev, struct drm_crtc *crtc,
			      struct drm_plane *primary,
			      struct drm_plane *cursor,
			      const struct drm_crtc_funcs *funcs,
			      const char *name, ...)
{
#if defined(HAVE_DRM_CRTC_INIT_WITH_PLANES_VALID_WITH_NAME)
		return drm_crtc_init_with_planes(dev, crtc, primary,
				 cursor, funcs, name);
#else
		return drm_crtc_init_with_planes(dev, crtc, primary,
				 cursor, funcs);
#endif
}

static inline int kcl_drm_universal_plane_init(struct drm_device *dev, struct drm_plane *plane,
			     unsigned long possible_crtcs,
			     const struct drm_plane_funcs *funcs,
			     const uint32_t *formats, unsigned int format_count,
			     const uint64_t *format_modifiers,
			     enum drm_plane_type type,
			     const char *name, ...)
{
#if defined(HAVE_9ARGS_DRM_UNIVERSAL_PLANE_INIT)
		return drm_universal_plane_init(dev, plane, possible_crtcs, funcs,
				 formats, format_count, format_modifiers, type, name);
#elif defined(HAVE_8ARGS_DRM_UNIVERSAL_PLANE_INIT)
		return drm_universal_plane_init(dev, plane, possible_crtcs, funcs,
				 formats, format_count, type, name);
#else
		return drm_universal_plane_init(dev, plane, possible_crtcs, funcs,
				 formats, format_count, type);
#endif
}

static inline struct drm_gem_object *
kcl_drm_gem_object_lookup(struct drm_device *dev, struct drm_file *filp,
				u32 handle)
{
#if defined(HAVE_2ARGS_DRM_GEM_OBJECT_LOOKUP)
		return drm_gem_object_lookup(filp, handle);
#else
		return drm_gem_object_lookup(dev, filp, handle);
#endif
}

#if !defined(HAVE_DRM_GET_FORMAT_NAME)
/**
 * struct drm_format_name_buf - name of a DRM format
 * @str: string buffer containing the format name
 */
struct drm_format_name_buf {
	char str[32];
};

static char printable_char(int c)
{
	return isascii(c) && isprint(c) ? c : '?';
}

static inline const char *kcl_drm_get_format_name(uint32_t format, struct drm_format_name_buf *buf)
{
	snprintf(buf->str, sizeof(buf->str),
		 "%c%c%c%c %s-endian (0x%08x)",
		 printable_char(format & 0xff),
		 printable_char((format >> 8) & 0xff),
		 printable_char((format >> 16) & 0xff),
		 printable_char((format >> 24) & 0x7f),
		 format & DRM_FORMAT_BIG_ENDIAN ? "big" : "little",
		 format);

	return buf->str;
}
#else
static inline const char *kcl_drm_get_format_name(uint32_t format, struct drm_format_name_buf *buf)
{
	return drm_get_format_name(format, buf);
}
#endif

static inline void kcl_drm_gem_object_put_unlocked(struct drm_gem_object *obj)
{
#if !defined(HAVE_DRM_GEM_OBJECT_PUT_UNLOCKED)
	return drm_gem_object_unreference_unlocked(obj);
#else
	return drm_gem_object_put_unlocked(obj);
#endif
}

#ifdef BUILD_AS_DKMS
extern struct dma_buf_ops *_kcl_drm_gem_prime_dmabuf_ops;
#define drm_gem_prime_dmabuf_ops (*_kcl_drm_gem_prime_dmabuf_ops)
#endif

#if !defined(HAVE_DRM_IS_CURRENT_MASTER)
bool drm_is_current_master(struct drm_file *fpriv);
#endif

static inline struct drm_crtc_state *
kcl_drm_atomic_get_old_crtc_state_before_commit(struct drm_atomic_state *state,
					    struct drm_crtc *crtc)
{
#if defined(HAVE_DRM_ATOMIC_GET_CRTC_STATE)
	return drm_atomic_get_old_crtc_state(state, crtc);
#elif defined(HAVE_DRM_CRTCS_STATE_MEMBER)
	return state->crtcs[drm_crtc_index(crtc)].ptr->state;
#else
	return state->crtcs[drm_crtc_index(crtc)]->state;
#endif
}

static inline struct drm_crtc_state *
kcl_drm_atomic_get_old_crtc_state_after_commit(struct drm_atomic_state *state,
				  struct drm_crtc *crtc)
{
#if defined(HAVE_DRM_ATOMIC_GET_CRTC_STATE)
	return drm_atomic_get_old_crtc_state(state, crtc);
#else
	return drm_atomic_get_existing_crtc_state(state, crtc);
#endif
}

static inline struct drm_crtc_state *
kcl_drm_atomic_get_new_crtc_state_before_commit(struct drm_atomic_state *state,
				  struct drm_crtc *crtc)
{
#if defined(HAVE_DRM_ATOMIC_GET_CRTC_STATE)
	return drm_atomic_get_new_crtc_state(state,crtc);
#else
	return drm_atomic_get_existing_crtc_state(state, crtc);
#endif
}

static inline struct drm_crtc_state *
kcl_drm_atomic_get_new_crtc_state_after_commit(struct drm_atomic_state *state,
					    struct drm_crtc *crtc)
{
#if defined(HAVE_DRM_ATOMIC_GET_CRTC_STATE)
	return drm_atomic_get_new_crtc_state(state,crtc);
#elif defined(HAVE_DRM_CRTCS_STATE_MEMBER)
	return state->crtcs[drm_crtc_index(crtc)].ptr->state;
#else
	return state->crtcs[drm_crtc_index(crtc)]->state;
#endif
}

static inline struct drm_plane_state *
kcl_drm_atomic_get_new_plane_state_before_commit(struct drm_atomic_state *state,
							struct drm_plane *plane)
{
#if defined(HAVE_DRM_ATOMIC_GET_NEW_PLANE_STATE)
	return drm_atomic_get_new_plane_state(state, plane);
#else
	return drm_atomic_get_existing_plane_state(state, plane);
#endif
}

#if !defined(HAVE_DRM_ATOMIC_HELPER_CONNECTOR_RESET)
extern void
__kcl_drm_atomic_helper_connector_reset(struct drm_connector *connector,
				    struct drm_connector_state *conn_state);
#endif

static inline void
kcl_drm_atomic_helper_connector_reset(struct drm_connector *connector,
				    struct drm_connector_state *conn_state)
{
#if !defined(HAVE_DRM_ATOMIC_HELPER_CONNECTOR_RESET)
	return __kcl_drm_atomic_helper_connector_reset(connector, conn_state);
#else
	return __drm_atomic_helper_connector_reset(connector, conn_state);
#endif
}

#if !defined(HAVE_DRM_GET_MAX_IOMEM)
u64 drm_get_max_iomem(void);
#endif

#ifndef DRM_MODE_FMT
#define DRM_MODE_FMT	"%d:\"%s\" %d %d %d %d %d %d %d %d %d %d 0x%x 0x%x"
#endif
#ifndef DRM_MODE_ARG
#define DRM_MODE_ARG(m)	\
	(m)->base.id, (m)->name, (m)->vrefresh, (m)->clock, \
	(m)->hdisplay, (m)->hsync_start, (m)->hsync_end, (m)->htotal, \
	(m)->vdisplay, (m)->vsync_start, (m)->vsync_end, (m)->vtotal, \
	(m)->type, (m)->flags
#endif

#ifndef DRM_RECT_FMT
#define DRM_RECT_FMT	"%dx%d%+d%+d"
#endif
#ifndef DRM_RECT_ARG
#define DRM_RECT_ARG(r) drm_rect_width(r), drm_rect_height(r), (r)->x1, (r)->y1
#endif

#ifndef DRM_RECT_FP_FMT
#define DRM_RECT_FP_FMT	"%d.%06ux%d.%06u%+d.%06u%+d.%06u"
#endif
#ifndef DRM_RECT_FP_ARG
#define DRM_RECT_FP_ARG(r) \
	drm_rect_width(r) >> 16, ((drm_rect_width(r) & 0xffff) * 15625) >> 10, \
	drm_rect_height(r) >> 16, ((drm_rect_height(r) & 0xffff) * 15625) >> 10, \
	(r)->x1 >> 16, (((r)->x1 & 0xffff) * 15625) >> 10, \
	(r)->y1 >> 16, (((r)->y1 & 0xffff) * 15625) >> 10
#endif

#if DRM_VERSION_CODE < DRM_VERSION(4, 10, 0)
static inline struct drm_rect
drm_plane_state_src(const struct drm_plane_state *state)
{
	struct drm_rect src = {
		.x1 = state->src_x,
		.y1 = state->src_y,
		.x2 = state->src_x + state->src_w,
		.y2 = state->src_y + state->src_h,
	};
	return src;
}

static inline struct drm_rect
drm_plane_state_dest(const struct drm_plane_state *state)
{
	struct drm_rect dest = {
		.x1 = state->crtc_x,
		.y1 = state->crtc_y,
		.x2 = state->crtc_x + state->crtc_w,
		.y2 = state->crtc_y + state->crtc_h,
	};
	return dest;
}

void __drm_printfn_info(struct drm_printer *p, struct va_format *vaf);

static inline struct drm_printer drm_info_printer(struct device *dev)
{
	struct drm_printer p = {
		.printfn = __drm_printfn_info,
		.arg = dev,
	};
	return p;
}

void drm_state_dump(struct drm_device *dev, struct drm_printer *p);
#endif

#if !defined(HAVE_DRM_PRINTF)
struct drm_printer {
	void (*printfn)(struct drm_printer *p, struct va_format *vaf);
	void *arg;
	const char *prefix;
};

void drm_printf(struct drm_printer *p, const char *f, ...);
#endif

/* helper for handling conditionals in various for_each macros */
#ifndef for_each_if
#define for_each_if(condition) if (!(condition)) {} else
#endif

#if !defined(HAVE_DRM_SEND_EVENT_LOCKED)
void drm_send_event_locked(struct drm_device *dev, struct drm_pending_event *e);
#endif

#ifndef _DRM_PRINTK
#define _DRM_PRINTK(once, level, fmt, ...)				\
	do {								\
		printk##once(KERN_##level "[" DRM_NAME "] " fmt,	\
			     ##__VA_ARGS__);				\
	} while (0)
#endif

#ifndef DRM_NOTE
#define DRM_NOTE(fmt, ...)						\
	_DRM_PRINTK(, NOTICE, fmt, ##__VA_ARGS__)
#endif

#ifndef DRM_WARN
#define DRM_WARN(fmt, ...)						\
	_DRM_PRINTK(, WARNING, fmt, ##__VA_ARGS__)
#endif

#ifndef DRM_NOTE_ONCE
#define DRM_NOTE_ONCE(fmt, ...)						\
	_DRM_PRINTK(_once, NOTICE, fmt, ##__VA_ARGS__)
#endif

#ifndef DRM_WARN_ONCE
#define DRM_WARN_ONCE(fmt, ...)						\
	_DRM_PRINTK(_once, WARNING, fmt, ##__VA_ARGS__)
#endif

#ifndef DP_LANE0_1_STATUS_ESI
#define DP_LANE0_1_STATUS_ESI                  0x200c /* status same as 0x202 */
#endif
#ifndef DP_LANE2_3_STATUS_ESI
#define DP_LANE2_3_STATUS_ESI                  0x200d /* status same as 0x203 */
#endif
#ifndef DP_LANE_ALIGN_STATUS_UPDATED_ESI
#define DP_LANE_ALIGN_STATUS_UPDATED_ESI       0x200e /* status same as 0x204 */
#endif
#ifndef DP_SINK_STATUS_ESI
#define DP_SINK_STATUS_ESI                     0x200f /* status same as 0x205 */
#endif

#if !defined(HAVE_DRM_ENCODER_FIND_VALID_WITH_FILE)
#define drm_encoder_find(dev, file, id) drm_encoder_find(dev, id)
#endif

#if !defined(HAVE_DRM_HELPER_FORCE_DISABLE_ALL)
static inline
int drm_helper_force_disable_all(struct drm_device *dev)
{
       return drm_crtc_force_disable_all(dev);
}
#endif

#ifndef HAVE_DRM_DEV_PUT
static inline void drm_dev_put(struct drm_device *dev)
{
	return drm_dev_unref(dev);
}
#endif

#endif /* AMDKCL_DRM_H */
