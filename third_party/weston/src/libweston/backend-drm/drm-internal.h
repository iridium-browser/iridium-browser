/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2017, 2018 Collabora, Ltd.
 * Copyright © 2017, 2018 General Electric Company
 * Copyright (c) 2018 DisplayLink (UK) Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/vt.h>
#include <assert.h>
#include <sys/mman.h>
#include <time.h>


#include <xf86drm.h>
#include <xf86drmMode.h>

#ifdef BUILD_DRM_GBM
#include <gbm.h>
#endif
#include <libudev.h>

#include <libweston/libweston.h>
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>
#include "output-capture.h"
#include "shared/helpers.h"
#include "shared/weston-drm-fourcc.h"
#include "libinput-seat.h"
#include "backend.h"
#include "libweston-internal.h"

#ifndef GBM_BO_USE_CURSOR
#define GBM_BO_USE_CURSOR GBM_BO_USE_CURSOR_64X64
#endif

#ifndef GBM_BO_USE_LINEAR
#define GBM_BO_USE_LINEAR (1 << 4)
#endif

#ifndef DRM_PLANE_ZPOS_INVALID_PLANE
#define DRM_PLANE_ZPOS_INVALID_PLANE	0xffffffffffffffffULL
#endif

#ifndef DRM_PLANE_ALPHA_OPAQUE
#define DRM_PLANE_ALPHA_OPAQUE	0xffffUL
#endif

/**
 * A small wrapper to print information into the 'drm-backend' debug scope.
 *
 * The following conventions are used to print variables:
 *
 *  - fixed uint32_t values, including Weston object IDs such as weston_output
 *    IDs, DRM object IDs such as CRTCs or properties, and GBM/DRM formats:
 *      "%lu (0x%lx)" (unsigned long) value, (unsigned long) value
 *
 *  - fixed uint64_t values, such as DRM property values (including object IDs
 *    when used as a value):
 *      "%llu (0x%llx)" (unsigned long long) value, (unsigned long long) value
 *
 *  - non-fixed-width signed int:
 *      "%d" value
 *
 *  - non-fixed-width unsigned int:
 *      "%u (0x%x)" value, value
 *
 *  - non-fixed-width unsigned long:
 *      "%lu (0x%lx)" value, value
 *
 * Either the integer or hexadecimal forms may be omitted if it is known that
 * one representation is not useful (e.g. width/height in hex are rarely what
 * you want).
 *
 * This is to avoid implicit widening or narrowing when we use fixed-size
 * types: uint32_t can be resolved by either unsigned int or unsigned long
 * on a 32-bit system but only unsigned int on a 64-bit system, with uint64_t
 * being unsigned long long on a 32-bit system and unsigned long on a 64-bit
 * system. To avoid confusing side effects, we explicitly cast to the widest
 * possible type and use a matching format specifier.
 */
#define drm_debug(b, ...) \
	weston_log_scope_printf((b)->debug, __VA_ARGS__)

#define MAX_CLONED_CONNECTORS 4


/**
 * Represents the values of an enum-type KMS property
 */
struct drm_property_enum_info {
	const char *name; /**< name as string (static, not freed) */
	bool valid; /**< true if value is supported; ignore if false */
	uint64_t value; /**< raw value */
};

/**
 * Holds information on a DRM property, including its ID and the enum
 * values it holds.
 *
 * DRM properties are allocated dynamically, and maintained as DRM objects
 * within the normal object ID space; they thus do not have a stable ID
 * to refer to. This includes enum values, which must be referred to by
 * integer values, but these are not stable.
 *
 * drm_property_info allows a cache to be maintained where Weston can use
 * enum values internally to refer to properties, with the mapping to DRM
 * ID values being maintained internally.
 */
struct drm_property_info {
	const char *name; /**< name as string (static, not freed) */
	uint32_t prop_id; /**< KMS property object ID */
	uint32_t flags;
	unsigned int num_enum_values; /**< number of enum values */
	struct drm_property_enum_info *enum_values; /**< array of enum values */
	unsigned int num_range_values;
	uint64_t range_values[2];
};

/**
 * List of properties attached to DRM planes
 */
enum wdrm_plane_property {
	WDRM_PLANE_TYPE = 0,
	WDRM_PLANE_SRC_X,
	WDRM_PLANE_SRC_Y,
	WDRM_PLANE_SRC_W,
	WDRM_PLANE_SRC_H,
	WDRM_PLANE_CRTC_X,
	WDRM_PLANE_CRTC_Y,
	WDRM_PLANE_CRTC_W,
	WDRM_PLANE_CRTC_H,
	WDRM_PLANE_FB_ID,
	WDRM_PLANE_CRTC_ID,
	WDRM_PLANE_IN_FORMATS,
	WDRM_PLANE_IN_FENCE_FD,
	WDRM_PLANE_FB_DAMAGE_CLIPS,
	WDRM_PLANE_ZPOS,
	WDRM_PLANE_ROTATION,
	WDRM_PLANE_ALPHA,
	WDRM_PLANE__COUNT
};

/**
 * Possible values for the WDRM_PLANE_TYPE property.
 */
enum wdrm_plane_type {
	WDRM_PLANE_TYPE_PRIMARY = 0,
	WDRM_PLANE_TYPE_CURSOR,
	WDRM_PLANE_TYPE_OVERLAY,
	WDRM_PLANE_TYPE__COUNT
};

/**
 * Possible values for the WDRM_PLANE_ROTATION property.
 */
enum wdrm_plane_rotation {
	WDRM_PLANE_ROTATION_0 = 0,
	WDRM_PLANE_ROTATION_90,
	WDRM_PLANE_ROTATION_180,
	WDRM_PLANE_ROTATION_270,
	WDRM_PLANE_ROTATION_REFLECT_X,
	WDRM_PLANE_ROTATION_REFLECT_Y,
	WDRM_PLANE_ROTATION__COUNT,
};

/**
 * List of properties attached to a DRM connector
 */
enum wdrm_connector_property {
	WDRM_CONNECTOR_EDID = 0,
	WDRM_CONNECTOR_DPMS,
	WDRM_CONNECTOR_CRTC_ID,
	WDRM_CONNECTOR_WRITEBACK_PIXEL_FORMATS,
	WDRM_CONNECTOR_WRITEBACK_FB_ID,
	WDRM_CONNECTOR_WRITEBACK_OUT_FENCE_PTR,
	WDRM_CONNECTOR_NON_DESKTOP,
	WDRM_CONNECTOR_CONTENT_PROTECTION,
	WDRM_CONNECTOR_HDCP_CONTENT_TYPE,
	WDRM_CONNECTOR_PANEL_ORIENTATION,
	WDRM_CONNECTOR_HDR_OUTPUT_METADATA,
	WDRM_CONNECTOR_MAX_BPC,
	WDRM_CONNECTOR_CONTENT_TYPE,
	WDRM_CONNECTOR__COUNT
};

enum wdrm_content_protection_state {
	WDRM_CONTENT_PROTECTION_UNDESIRED = 0,
	WDRM_CONTENT_PROTECTION_DESIRED,
	WDRM_CONTENT_PROTECTION_ENABLED,
	WDRM_CONTENT_PROTECTION__COUNT
};

enum wdrm_hdcp_content_type {
	WDRM_HDCP_CONTENT_TYPE0 = 0,
	WDRM_HDCP_CONTENT_TYPE1,
	WDRM_HDCP_CONTENT_TYPE__COUNT
};

enum wdrm_dpms_state {
	WDRM_DPMS_STATE_OFF = 0,
	WDRM_DPMS_STATE_ON,
	WDRM_DPMS_STATE_STANDBY, /* unused */
	WDRM_DPMS_STATE_SUSPEND, /* unused */
	WDRM_DPMS_STATE__COUNT
};

enum wdrm_panel_orientation {
	WDRM_PANEL_ORIENTATION_NORMAL = 0,
	WDRM_PANEL_ORIENTATION_UPSIDE_DOWN,
	WDRM_PANEL_ORIENTATION_LEFT_SIDE_UP,
	WDRM_PANEL_ORIENTATION_RIGHT_SIDE_UP,
	WDRM_PANEL_ORIENTATION__COUNT
};

enum wdrm_content_type {
	WDRM_CONTENT_TYPE_NO_DATA = 0,
	WDRM_CONTENT_TYPE_GRAPHICS,
	WDRM_CONTENT_TYPE_PHOTO,
	WDRM_CONTENT_TYPE_CINEMA,
	WDRM_CONTENT_TYPE_GAME,
	WDRM_CONTENT_TYPE__COUNT
};

/**
 * List of properties attached to DRM CRTCs
 */
enum wdrm_crtc_property {
	WDRM_CRTC_MODE_ID = 0,
	WDRM_CRTC_ACTIVE,
	WDRM_CRTC_CTM,
	WDRM_CRTC_DEGAMMA_LUT,
	WDRM_CRTC_DEGAMMA_LUT_SIZE,
	WDRM_CRTC_GAMMA_LUT,
	WDRM_CRTC_GAMMA_LUT_SIZE,
	WDRM_CRTC_VRR_ENABLED,
	WDRM_CRTC__COUNT
};

/**
 * Reasons why placing a view on a plane failed. Needed by the dma-buf feedback.
 */
enum try_view_on_plane_failure_reasons {
	FAILURE_REASONS_NONE = 0,
	FAILURE_REASONS_FORCE_RENDERER = 1 << 0,
	FAILURE_REASONS_FB_FORMAT_INCOMPATIBLE = 1 << 1,
	FAILURE_REASONS_DMABUF_MODIFIER_INVALID = 1 << 2,
	FAILURE_REASONS_ADD_FB_FAILED = 1 << 3,
	FAILURE_REASONS_NO_PLANES_AVAILABLE = 1 << 4,
	FAILURE_REASONS_PLANES_REJECTED = 1 << 5,
	FAILURE_REASONS_INADEQUATE_CONTENT_PROTECTION = 1 << 6,
	FAILURE_REASONS_INCOMPATIBLE_TRANSFORM = 1 << 7,
	FAILURE_REASONS_NO_BUFFER = 1 << 8,
	FAILURE_REASONS_BUFFER_TYPE = 1 << 9,
	FAILURE_REASONS_GLOBAL_ALPHA = 1 << 10,
	FAILURE_REASONS_NO_GBM = 1 << 11,
	FAILURE_REASONS_GBM_BO_IMPORT_FAILED = 1 << 12,
	FAILURE_REASONS_GBM_BO_GET_HANDLE_FAILED = 1 << 13,
};

/**
 * We use this to keep track of actions we need to do with the dma-buf feedback
 * in order to keep it up-to-date with the info we get from the DRM-backend.
 */
enum actions_needed_dmabuf_feedback {
	ACTION_NEEDED_NONE = 0,
	ACTION_NEEDED_ADD_SCANOUT_TRANCHE = (1 << 0),
	ACTION_NEEDED_REMOVE_SCANOUT_TRANCHE = (1 << 1),
};

struct drm_device {
	struct drm_backend *backend;

	struct {
		int id;
		int fd;
		char *filename;
		dev_t devnum;
	} drm;

	/* Track the GEM handles if the device does not have a gbm device, which
	 * tracks the handles for us.
	 */
	struct hash_table *gem_handle_refcnt;

	/* drm_crtc::link */
	struct wl_list crtc_list;

	struct wl_list plane_list;

	/* drm_writeback::link */
	struct wl_list writeback_connector_list;

	bool state_invalid;

	bool atomic_modeset;

	bool tearing_supported;

	bool aspect_ratio_supported;

	int32_t cursor_width;
	int32_t cursor_height;

	bool cursors_are_broken;
	bool sprites_are_broken;

	void *repaint_data;

	bool fb_modifiers;

	/* we need these parameters in order to not fail drmModeAddFB2()
	 * due to out of bounds dimensions, and then mistakenly set
	 * sprites_are_broken:
	 */
	int min_width, max_width;
	int min_height, max_height;

	/* drm_backend::kms_list */
	struct wl_list link;
};

struct drm_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct udev *udev;
	struct wl_event_source *drm_source;

	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_drm_source;

	struct drm_device *drm;
	/* drm_device::link */
	struct wl_list kms_list;
	struct gbm_device *gbm;
	struct wl_listener session_listener;
	const struct pixel_format_info *format;

	bool use_pixman_shadow;

	struct udev_input input;

	uint32_t pageflip_timeout;

	bool shutting_down;

	struct weston_log_scope *debug;
};

struct drm_mode {
	struct weston_mode base;
	drmModeModeInfo mode_info;
	uint32_t blob_id;
};

enum drm_fb_type {
	BUFFER_INVALID = 0, /**< never used */
	BUFFER_CLIENT, /**< directly sourced from client */
	BUFFER_DMABUF, /**< imported from linux_dmabuf client */
	BUFFER_PIXMAN_DUMB, /**< internal Pixman rendering */
	BUFFER_GBM_SURFACE, /**< internal EGL rendering */
	BUFFER_CURSOR, /**< internal cursor buffer */
};

struct drm_fb {
	enum drm_fb_type type;

	struct drm_device *scanout_device;

	int refcnt;

	uint32_t fb_id, size;
	uint32_t handles[4];
	uint32_t strides[4];
	uint32_t offsets[4];
	int num_planes;
	const struct pixel_format_info *format;
	uint64_t modifier;
	int width, height;
	int fd;

	uint32_t plane_mask;

	/* Used by gbm fbs */
	struct gbm_bo *bo;
	struct gbm_surface *gbm_surface;

	/* Used by dumb fbs */
	void *map;
};

struct drm_buffer_fb {
	struct drm_fb *fb;
	enum try_view_on_plane_failure_reasons failure_reasons;
	struct drm_device *device;
	struct wl_list link;
};

struct drm_fb_private {
	struct wl_list buffer_fb_list;
	struct wl_listener buffer_destroy_listener;
};

/**
 * Pending state holds one or more drm_output_state structures, collected from
 * performing repaint. This pending state is transient, and only lives between
 * beginning a repaint group and flushing the results: after flush, each
 * output state will complete and be retired separately.
 */
struct drm_pending_state {
	struct drm_device *device;
	struct wl_list output_list;
};

/*
 * Output state holds the dynamic state for one Weston output, i.e. a KMS CRTC,
 * plus >= 1 each of encoder/connector/plane. Since everything but the planes
 * is currently statically assigned per-output, we mainly use this to track
 * plane state.
 *
 * pending_state is set when the output state is owned by a pending_state,
 * i.e. when it is being constructed and has not yet been applied. When the
 * output state has been applied, the owning pending_state is freed.
 */
struct drm_output_state {
	struct drm_pending_state *pending_state;
	struct drm_output *output;
	struct wl_list link;
	enum dpms_enum dpms;
	enum weston_hdcp_protection protection;
	struct wl_list plane_list;
	bool tear;
};

/**
 * Plane state holds the dynamic state for a plane: where it is positioned,
 * and which buffer it is currently displaying.
 *
 * The plane state is owned by an output state, except when setting an initial
 * state. See drm_output_state for notes on state object lifetime.
 */
struct drm_plane_state {
	struct drm_plane *plane;
	struct drm_output *output;
	struct drm_output_state *output_state;

	struct drm_fb *fb;
	struct {
		struct weston_buffer_reference buffer;
		struct weston_buffer_release_reference release;
	} fb_ref;

	struct weston_view *ev; /**< maintained for drm_assign_planes only */

	int32_t src_x, src_y;
	uint32_t src_w, src_h;
	int32_t dest_x, dest_y;
	uint32_t dest_w, dest_h;

	uint32_t rotation;

	uint64_t zpos;
	uint16_t alpha;

	bool complete;

	/* We don't own the fd, so we shouldn't close it */
	int in_fence_fd;

	uint32_t damage_blob_id; /* damage to kernel */

	struct wl_list link; /* drm_output_state::plane_list */
};

/**
 * A plane represents one buffer, positioned within a CRTC, and stacked
 * relative to other planes on the same CRTC.
 *
 * Each CRTC has a 'primary plane', which use used to display the classic
 * framebuffer contents, as accessed through the legacy drmModeSetCrtc
 * call (which combines setting the CRTC's actual physical mode, and the
 * properties of the primary plane).
 *
 * The cursor plane also has its own alternate legacy API.
 *
 * Other planes are used opportunistically to display content we do not
 * wish to blit into the primary plane. These non-primary/cursor planes
 * are referred to as 'sprites'.
 */
struct drm_plane {
	struct weston_plane base;

	struct drm_device *device;

	enum wdrm_plane_type type;

	uint32_t possible_crtcs;
	uint32_t plane_id;
	uint32_t plane_idx;
	uint32_t crtc_id;

	struct drm_property_info props[WDRM_PLANE__COUNT];

	/* The last state submitted to the kernel for this plane. */
	struct drm_plane_state *state_cur;

	uint64_t zpos_min;
	uint64_t zpos_max;

	uint16_t alpha_min;
	uint16_t alpha_max;

	struct wl_list link;

	struct weston_drm_format_array formats;
};

struct drm_connector {
	struct drm_device *device;

	drmModeConnector *conn;
	uint32_t connector_id;

	drmModeObjectProperties *props_drm;

	/* Holds the properties for the connector */
	struct drm_property_info props[WDRM_CONNECTOR__COUNT];
};

enum writeback_screenshot_state {
	/* No writeback connector screenshot ongoing. */
	DRM_OUTPUT_WB_SCREENSHOT_OFF,
	/* Screenshot client just triggered a writeback connector screenshot.
         * Now we need to prepare an atomic commit that will make DRM perform
         * the writeback operation. */
	DRM_OUTPUT_WB_SCREENSHOT_PREPARE_COMMIT,
	/* The atomic commit with writeback setup has been committed. After the
	 * commit is handled by DRM it will give us a sync fd that gets
	 * signalled when the writeback is done. */
	DRM_OUTPUT_WB_SCREENSHOT_CHECK_FENCE,
	/* The atomic commit completed and we received the sync fd from the
	 * kernel. We've polled to check if the writeback was over, but it
	 * wasn't. Now we must stop the repaint loop and wait until the
	 * writeback is complete, because we can't commit with KMS objects
	 * (CRTC, planes, etc) that are in used by the writeback job. */
	DRM_OUTPUT_WB_SCREENSHOT_WAITING_SIGNAL,
};

struct drm_writeback_state {
	struct drm_writeback *wb;
	struct drm_output *output;

	enum writeback_screenshot_state state;
	struct weston_capture_task *ct;

	struct drm_fb *fb;
	int32_t out_fence_fd;
	struct wl_event_source *wb_source;

	/* Reference to fb's being used by the writeback job. These are all the
	 * framebuffers in every drm_plane_state of the output state that we've
	 * used to request the writeback job */
	struct wl_array referenced_fbs;
};

struct drm_writeback {
	/* drm_device::writeback_connector_list */
	struct wl_list link;

	struct drm_device *device;
	struct drm_connector connector;

	struct weston_drm_format_array formats;
};

struct drm_head {
	struct weston_head base;
	struct drm_connector connector;

	struct backlight *backlight;

	drmModeModeInfo inherited_mode;	/**< Original mode on the connector */
	uint32_t inherited_max_bpc;	/**< Original max_bpc on the connector */
	uint32_t inherited_crtc_id;	/**< Original CRTC assignment */

	/* drm_output::disable_head */
	struct wl_list disable_head_link;
};

struct drm_crtc {
	/* drm_device::crtc_list */
	struct wl_list link;
	struct drm_device *device;

	/* The output driven by the CRTC */
	struct drm_output *output;

	uint32_t crtc_id; /* object ID to pass to DRM functions */
	int pipe; /* index of CRTC in resource array / bitmasks */

	/* Holds the properties for the CRTC */
	struct drm_property_info props_crtc[WDRM_CRTC__COUNT];
};

struct drm_output {
	struct weston_output base;
	struct drm_backend *backend;
	struct drm_device *device;
	struct drm_crtc *crtc;

	/* drm_head::disable_head_link */
	struct wl_list disable_head;

	bool page_flip_pending;
	bool atomic_complete_pending;
	bool destroy_pending;
	bool disable_pending;
	bool dpms_off_pending;
	bool mode_switch_pending;

	uint32_t gbm_cursor_handle[2];
	struct drm_fb *gbm_cursor_fb[2];
	struct drm_plane *cursor_plane;
	struct weston_view *cursor_view;
	struct wl_listener cursor_view_destroy_listener;
	int current_cursor;

	struct gbm_surface *gbm_surface;
	const struct pixel_format_info *format;
	uint32_t gbm_bo_flags;

	uint32_t hdr_output_metadata_blob_id;
	uint64_t ackd_color_outcome_serial;

	unsigned max_bpc;

	bool deprecated_gamma_is_set;
	bool legacy_gamma_not_supported;

	/* Plane being displayed directly on the CRTC */
	struct drm_plane *scanout_plane;

	/* The last state submitted to the kernel for this CRTC. */
	struct drm_output_state *state_cur;
	/* The previously-submitted state, where the hardware has not
	 * yet acknowledged completion of state_cur. */
	struct drm_output_state *state_last;

	/* only set when a writeback screenshot is ongoing */
	struct drm_writeback_state *wb_state;

	struct drm_fb *dumb[2];
	struct weston_renderbuffer *renderbuffer[2];
	int current_image;

	struct vaapi_recorder *recorder;
	struct wl_listener recorder_frame_listener;

	struct wl_event_source *pageflip_timer;

	bool virtual;
	void (*virtual_destroy)(struct weston_output *base);

	submit_frame_cb virtual_submit_frame;

	enum wdrm_content_type content_type;
};

void
drm_destroy(struct weston_backend *backend);

static inline struct drm_head *
to_drm_head(struct weston_head *base)
{
	if (base->backend->destroy != drm_destroy)
		return NULL;
	return container_of(base, struct drm_head, base);
}

void
drm_writeback_reference_planes(struct drm_writeback_state *state,
			       struct wl_list *plane_state_list);
bool
drm_writeback_should_wait_completion(struct drm_writeback_state *state);
void
drm_writeback_fail_screenshot(struct drm_writeback_state *state,
			      const char *err_msg);
enum writeback_screenshot_state
drm_output_get_writeback_state(struct drm_output *output);

void
drm_output_destroy(struct weston_output *output_base);
void
drm_virtual_output_destroy(struct weston_output *output_base);

static inline struct drm_output *
to_drm_output(struct weston_output *base)
{
	if (
#ifdef BUILD_DRM_VIRTUAL
	    base->destroy != drm_virtual_output_destroy &&
#endif
	    base->destroy != drm_output_destroy)
		return NULL;
	return container_of(base, struct drm_output, base);
}

static inline struct drm_backend *
to_drm_backend(struct weston_compositor *base)
{
	return container_of(base->backend, struct drm_backend, base);
}

static inline struct drm_mode *
to_drm_mode(struct weston_mode *base)
{
	return container_of(base, struct drm_mode, base);
}

static inline const char *
drm_output_get_plane_type_name(struct drm_plane *p)
{
	switch (p->type) {
	case WDRM_PLANE_TYPE_PRIMARY:
		return "primary";
	case WDRM_PLANE_TYPE_CURSOR:
		return "cursor";
	case WDRM_PLANE_TYPE_OVERLAY:
		return "overlay";
	default:
		assert(0);
		break;
	}
}

struct drm_crtc *
drm_crtc_find(struct drm_device *device, uint32_t crtc_id);

struct drm_head *
drm_head_find_by_connector(struct drm_backend *backend, uint32_t connector_id);

uint64_t
drm_rotation_from_output_transform(struct drm_plane *plane,
				   enum wl_output_transform ot);

static inline bool
drm_paint_node_transform_supported(struct weston_paint_node *node, struct drm_plane *plane)
{
	/* if false, the transform doesn't map to any of the standard
	 * (ie: 90 degree) output transformations. */
	if (!node->valid_transform)
		return false;

	if (drm_rotation_from_output_transform(plane, node->transform) == 0)
		return false;

	return true;
}

int
drm_mode_ensure_blob(struct drm_device *device, struct drm_mode *mode);

struct drm_mode *
drm_output_choose_mode(struct drm_output *output,
		       struct weston_mode *target_mode);
void
update_head_from_connector(struct drm_head *head);

void
drm_mode_list_destroy(struct drm_device *device, struct wl_list *mode_list);

void
drm_output_print_modes(struct drm_output *output);

int
drm_output_set_mode(struct weston_output *base,
		    enum weston_drm_backend_output_mode mode,
		    const char *modeline);

void
drm_property_info_populate(struct drm_device *device,
		           const struct drm_property_info *src,
			   struct drm_property_info *info,
			   unsigned int num_infos,
			   drmModeObjectProperties *props);
uint64_t
drm_property_get_value(struct drm_property_info *info,
		       const drmModeObjectProperties *props,
		       uint64_t def);
uint64_t *
drm_property_get_range_values(struct drm_property_info *info,
			      const drmModeObjectProperties *props);
int
drm_plane_populate_formats(struct drm_plane *plane, const drmModePlane *kplane,
			   const drmModeObjectProperties *props,
			   const bool use_modifiers);
void
drm_property_info_free(struct drm_property_info *info, int num_props);

extern struct drm_property_enum_info plane_type_enums[];
extern const struct drm_property_info plane_props[];
extern struct drm_property_enum_info dpms_state_enums[];
extern struct drm_property_enum_info content_protection_enums[];
extern struct drm_property_enum_info hdcp_content_type_enums[];
extern const struct drm_property_info connector_props[];
extern const struct drm_property_info crtc_props[];

int
init_kms_caps(struct drm_device *device);

int
drm_pending_state_test(struct drm_pending_state *pending_state);
int
drm_pending_state_apply(struct drm_pending_state *pending_state);
int
drm_pending_state_apply_sync(struct drm_pending_state *pending_state);

void
drm_output_set_gamma(struct weston_output *output_base,
		     uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b);

void
drm_output_update_msc(struct drm_output *output, unsigned int seq);
void
drm_output_update_complete(struct drm_output *output, uint32_t flags,
			   unsigned int sec, unsigned int usec);
int
on_drm_input(int fd, uint32_t mask, void *data);

struct drm_fb *
drm_fb_ref(struct drm_fb *fb);
void
drm_fb_unref(struct drm_fb *fb);

struct drm_fb *
drm_fb_create_dumb(struct drm_device *device, int width, int height,
		   uint32_t format);
struct drm_fb *
drm_fb_get_from_bo(struct gbm_bo *bo, struct drm_device *device,
		   bool is_opaque, enum drm_fb_type type);

void
drm_output_set_cursor_view(struct drm_output *output, struct weston_view *ev);

int
drm_output_ensure_hdr_output_metadata_blob(struct drm_output *output);

#ifdef BUILD_DRM_GBM
extern struct drm_fb *
drm_fb_get_from_paint_node(struct drm_output_state *state,
			   struct weston_paint_node *pnode);

extern bool
drm_can_scanout_dmabuf(struct weston_backend *backend,
		       struct linux_dmabuf_buffer *dmabuf);
#else
static inline struct drm_fb *
drm_fb_get_from_paint_node(struct drm_output_state *state,
			   struct weston_paint_node *pnode)
{
	return NULL;
}
static inline bool
drm_can_scanout_dmabuf(struct weston_backend *backend,
		       struct linux_dmabuf_buffer *dmabuf)
{
	return false;
}
#endif

struct drm_pending_state *
drm_pending_state_alloc(struct drm_device *device);
void
drm_pending_state_free(struct drm_pending_state *pending_state);
struct drm_output_state *
drm_pending_state_get_output(struct drm_pending_state *pending_state,
			     struct drm_output *output);


/**
 * Mode for drm_output_state_duplicate.
 */
enum drm_output_state_duplicate_mode {
	DRM_OUTPUT_STATE_CLEAR_PLANES, /**< reset all planes to off */
	DRM_OUTPUT_STATE_PRESERVE_PLANES, /**< preserve plane state */
};

struct drm_output_state *
drm_output_state_alloc(struct drm_output *output,
		       struct drm_pending_state *pending_state);
struct drm_output_state *
drm_output_state_duplicate(struct drm_output_state *src,
			   struct drm_pending_state *pending_state,
			   enum drm_output_state_duplicate_mode plane_mode);
void
drm_output_state_free(struct drm_output_state *state);
struct drm_plane_state *
drm_output_state_get_plane(struct drm_output_state *state_output,
			   struct drm_plane *plane);
struct drm_plane_state *
drm_output_state_get_existing_plane(struct drm_output_state *state_output,
				    struct drm_plane *plane);



struct drm_plane_state *
drm_plane_state_alloc(struct drm_output_state *state_output,
		      struct drm_plane *plane);
struct drm_plane_state *
drm_plane_state_duplicate(struct drm_output_state *state_output,
			  struct drm_plane_state *src);
void
drm_plane_state_free(struct drm_plane_state *state, bool force);
void
drm_plane_state_put_back(struct drm_plane_state *state);
bool
drm_plane_state_coords_for_paint_node(struct drm_plane_state *state,
				      struct weston_paint_node *node,
				      uint64_t zpos);
void
drm_plane_reset_state(struct drm_plane *plane);

void
drm_assign_planes(struct weston_output *output_base);

bool
drm_plane_is_available(struct drm_plane *plane, struct drm_output *output);

void
drm_output_render(struct drm_output_state *state, pixman_region32_t *damage);

int
parse_gbm_format(const char *s, const struct pixel_format_info *default_format,
		 const struct pixel_format_info **format);

#ifdef BUILD_DRM_VIRTUAL
extern int
drm_backend_init_virtual_output_api(struct weston_compositor *compositor);
#else
inline static int
drm_backend_init_virtual_output_api(struct weston_compositor *compositor)
{
	return 0;
}
#endif

#ifdef BUILD_DRM_GBM
int
init_egl(struct drm_backend *b);

int
drm_output_init_egl(struct drm_output *output, struct drm_backend *b);

void
drm_output_fini_egl(struct drm_output *output);

struct drm_fb *
drm_output_render_gl(struct drm_output_state *state, pixman_region32_t *damage);

#else
inline static int
init_egl(struct drm_backend *b)
{
	weston_log("Compiled without GBM/EGL support\n");
	return -1;
}

inline static int
drm_output_init_egl(struct drm_output *output, struct drm_backend *b)
{
	return -1;
}

inline static void
drm_output_fini_egl(struct drm_output *output)
{
}

inline static struct drm_fb *
drm_output_render_gl(struct drm_output_state *state, pixman_region32_t *damage)
{
	return NULL;
}
#endif
