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
#include <drm_fourcc.h>

#ifdef BUILD_DRM_GBM
#include <gbm.h>
#endif
#include <libudev.h>

#include <libweston/libweston.h>
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>
#include "shared/helpers.h"
#include "libinput-seat.h"
#include "backend.h"
#include "libweston-internal.h"

#ifndef DRM_CLIENT_CAP_ASPECT_RATIO
#define DRM_CLIENT_CAP_ASPECT_RATIO	4
#endif

#ifndef GBM_BO_USE_CURSOR
#define GBM_BO_USE_CURSOR GBM_BO_USE_CURSOR_64X64
#endif

#ifndef GBM_BO_USE_LINEAR
#define GBM_BO_USE_LINEAR (1 << 4)
#endif

#ifndef DRM_PLANE_ZPOS_INVALID_PLANE
#define DRM_PLANE_ZPOS_INVALID_PLANE	0xffffffffffffffffULL
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

#ifndef DRM_MODE_PICTURE_ASPECT_64_27
#define DRM_MODE_PICTURE_ASPECT_64_27		3
#define  DRM_MODE_FLAG_PIC_AR_64_27 \
			(DRM_MODE_PICTURE_ASPECT_64_27<<19)
#endif
#ifndef DRM_MODE_PICTURE_ASPECT_256_135
#define DRM_MODE_PICTURE_ASPECT_256_135		4
#define  DRM_MODE_FLAG_PIC_AR_256_135 \
			(DRM_MODE_PICTURE_ASPECT_256_135<<19)
#endif


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
 * List of properties attached to a DRM connector
 */
enum wdrm_connector_property {
	WDRM_CONNECTOR_EDID = 0,
	WDRM_CONNECTOR_DPMS,
	WDRM_CONNECTOR_CRTC_ID,
	WDRM_CONNECTOR_NON_DESKTOP,
	WDRM_CONNECTOR_CONTENT_PROTECTION,
	WDRM_CONNECTOR_HDCP_CONTENT_TYPE,
	WDRM_CONNECTOR_PANEL_ORIENTATION,
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

/**
 * List of properties attached to DRM CRTCs
 */
enum wdrm_crtc_property {
	WDRM_CRTC_MODE_ID = 0,
	WDRM_CRTC_ACTIVE,
	WDRM_CRTC__COUNT
};

struct drm_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct udev *udev;
	struct wl_event_source *drm_source;

	struct udev_monitor *udev_monitor;
	struct wl_event_source *udev_drm_source;

	struct {
		int id;
		int fd;
		char *filename;
		dev_t devnum;
	} drm;
	struct gbm_device *gbm;
	struct wl_listener session_listener;
	uint32_t gbm_format;

	/* we need these parameters in order to not fail drmModeAddFB2()
	 * due to out of bounds dimensions, and then mistakenly set
	 * sprites_are_broken:
	 */
	int min_width, max_width;
	int min_height, max_height;

	struct wl_list plane_list;

	void *repaint_data;

	bool state_invalid;

	/* CRTC IDs not used by any enabled output. */
	struct wl_array unused_crtcs;

	bool sprites_are_broken;
	bool cursors_are_broken;

	bool universal_planes;
	bool atomic_modeset;

	bool use_pixman;
	bool use_pixman_shadow;

	struct udev_input input;

	int32_t cursor_width;
	int32_t cursor_height;

	uint32_t pageflip_timeout;

	bool shutting_down;

	bool aspect_ratio_supported;

	bool fb_modifiers;

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
	struct weston_buffer_reference buffer_ref;
	struct weston_buffer_release_reference buffer_release_ref;

	/* Used by gbm fbs */
	struct gbm_bo *bo;
	struct gbm_surface *gbm_surface;

	/* Used by dumb fbs */
	void *map;
};

struct drm_edid {
	char eisa_id[13];
	char monitor_name[13];
	char pnp_id[5];
	char serial_number[13];
};

/**
 * Pending state holds one or more drm_output_state structures, collected from
 * performing repaint. This pending state is transient, and only lives between
 * beginning a repaint group and flushing the results: after flush, each
 * output state will complete and be retired separately.
 */
struct drm_pending_state {
	struct drm_backend *backend;
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
};

/**
 * An instance of this class is created each time we believe we have a plane
 * suitable to be used by a view as a direct scan-out. The list is initalized
 * and populated locally.
 */
struct drm_plane_zpos {
	struct drm_plane *plane;
	struct wl_list link;	/**< :candidate_plane_zpos_list */
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

	struct weston_view *ev; /**< maintained for drm_assign_planes only */

	int32_t src_x, src_y;
	uint32_t src_w, src_h;
	int32_t dest_x, dest_y;
	uint32_t dest_w, dest_h;

	uint64_t zpos;

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

	struct drm_backend *backend;

	enum wdrm_plane_type type;

	uint32_t possible_crtcs;
	uint32_t plane_id;
	uint32_t count_formats;

	struct drm_property_info props[WDRM_PLANE__COUNT];

	/* The last state submitted to the kernel for this plane. */
	struct drm_plane_state *state_cur;

	uint64_t zpos_min;
	uint64_t zpos_max;

	struct wl_list link;

	struct {
		uint32_t format;
		uint32_t count_modifiers;
		uint64_t *modifiers;
	} formats[];
};

struct drm_head {
	struct weston_head base;
	struct drm_backend *backend;

	drmModeConnector *connector;
	uint32_t connector_id;
	struct drm_edid edid;

	/* Holds the properties for the connector */
	struct drm_property_info props_conn[WDRM_CONNECTOR__COUNT];

	struct backlight *backlight;

	drmModeModeInfo inherited_mode;	/**< Original mode on the connector */
	uint32_t inherited_crtc_id;	/**< Original CRTC assignment */
};

struct drm_output {
	struct weston_output base;
	struct drm_backend *backend;

	uint32_t crtc_id; /* object ID to pass to DRM functions */
	int pipe; /* index of CRTC in resource array / bitmasks */

	/* Holds the properties for the CRTC */
	struct drm_property_info props_crtc[WDRM_CRTC__COUNT];

	bool page_flip_pending;
	bool atomic_complete_pending;
	bool destroy_pending;
	bool disable_pending;
	bool dpms_off_pending;

	uint32_t gbm_cursor_handle[2];
	struct drm_fb *gbm_cursor_fb[2];
	struct drm_plane *cursor_plane;
	struct weston_view *cursor_view;
	int current_cursor;

	struct gbm_surface *gbm_surface;
	uint32_t gbm_format;
	uint32_t gbm_bo_flags;

	/* Plane being displayed directly on the CRTC */
	struct drm_plane *scanout_plane;

	/* The last state submitted to the kernel for this CRTC. */
	struct drm_output_state *state_cur;
	/* The previously-submitted state, where the hardware has not
	 * yet acknowledged completion of state_cur. */
	struct drm_output_state *state_last;

	struct drm_fb *dumb[2];
	pixman_image_t *image[2];
	int current_image;
	pixman_region32_t previous_damage;

	struct vaapi_recorder *recorder;
	struct wl_listener recorder_frame_listener;

	struct wl_event_source *pageflip_timer;

	bool virtual;

	submit_frame_cb virtual_submit_frame;
};

static inline struct drm_head *
to_drm_head(struct weston_head *base)
{
	return container_of(base, struct drm_head, base);
}

static inline struct drm_output *
to_drm_output(struct weston_output *base)
{
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

struct drm_output *
drm_output_find_by_crtc(struct drm_backend *b, uint32_t crtc_id);

struct drm_head *
drm_head_find_by_connector(struct drm_backend *backend, uint32_t connector_id);

static inline bool
drm_view_transform_supported(struct weston_view *ev, struct weston_output *output)
{
	struct weston_buffer_viewport *viewport = &ev->surface->buffer_viewport;

	/* This will incorrectly disallow cases where the combination of
	 * buffer and view transformations match the output transform.
	 * Fixing this requires a full analysis of the transformation
	 * chain. */
	if (ev->transform.enabled &&
	    ev->transform.matrix.type >= WESTON_MATRIX_TRANSFORM_ROTATE)
		return false;

	if (viewport->buffer.transform != output->transform)
		return false;

	return true;
}

int
drm_mode_ensure_blob(struct drm_backend *backend, struct drm_mode *mode);

struct drm_mode *
drm_output_choose_mode(struct drm_output *output,
		       struct weston_mode *target_mode);
void
update_head_from_connector(struct drm_head *head,
			   drmModeObjectProperties *props);

void
drm_mode_list_destroy(struct drm_backend *backend, struct wl_list *mode_list);

void
drm_output_print_modes(struct drm_output *output);

int
drm_output_set_mode(struct weston_output *base,
		    enum weston_drm_backend_output_mode mode,
		    const char *modeline);

void
drm_property_info_populate(struct drm_backend *b,
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
init_kms_caps(struct drm_backend *b);

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
drm_fb_create_dumb(struct drm_backend *b, int width, int height,
		   uint32_t format);
struct drm_fb *
drm_fb_get_from_bo(struct gbm_bo *bo, struct drm_backend *backend,
		   bool is_opaque, enum drm_fb_type type);

#ifdef BUILD_DRM_GBM
extern struct drm_fb *
drm_fb_get_from_view(struct drm_output_state *state, struct weston_view *ev);
extern bool
drm_can_scanout_dmabuf(struct weston_compositor *ec,
		       struct linux_dmabuf_buffer *dmabuf);
#else
static inline struct drm_fb *
drm_fb_get_from_view(struct drm_output_state *state, struct weston_view *ev)
{
	return NULL;
}
static inline bool
drm_can_scanout_dmabuf(struct weston_compositor *ec,
		       struct linux_dmabuf_buffer *dmabuf)
{
	return false;
}
#endif

struct drm_pending_state *
drm_pending_state_alloc(struct drm_backend *backend);
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
drm_plane_state_coords_for_view(struct drm_plane_state *state,
				struct weston_view *ev, uint64_t zpos);
void
drm_plane_reset_state(struct drm_plane *plane);

void
drm_assign_planes(struct weston_output *output_base, void *repaint_data);

bool
drm_plane_is_available(struct drm_plane *plane, struct drm_output *output);

void
drm_output_render(struct drm_output_state *state, pixman_region32_t *damage);

int
parse_gbm_format(const char *s, uint32_t default_value, uint32_t *gbm_format);

extern struct gl_renderer_interface *gl_renderer;

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

void
renderer_switch_binding(struct weston_keyboard *keyboard,
			const struct timespec *time, uint32_t key, void *data);
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

inline static void
renderer_switch_binding(struct weston_keyboard *keyboard,
			const struct timespec *time, uint32_t key, void *data)
{
	weston_log("Compiled without GBM/EGL support\n");
}
#endif
