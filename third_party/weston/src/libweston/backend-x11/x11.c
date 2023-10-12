/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2010-2011 Intel Corporation
 * Copyright © 2013 Vasily Khoruzhick <anarsoul@gmail.com>
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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <linux/input.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#ifdef HAVE_XCB_XKB
#include <xcb/xkb.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xkbcommon/xkbcommon.h>

#include <libweston/libweston.h>
#include <libweston/backend-x11.h>
#include "shared/helpers.h"
#include "shared/image-loader.h"
#include "shared/timespec-util.h"
#include "shared/file-util.h"
#include "renderer-gl/gl-renderer.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/weston-egl-ext.h"
#include "shared/xalloc.h"
#include "pixman-renderer.h"
#include "presentation-time-server-protocol.h"
#include "linux-dmabuf.h"
#include "linux-explicit-synchronization.h"
#include <libweston/pixel-formats.h>
#include <libweston/windowed-output-api.h>

#define DEFAULT_AXIS_STEP_DISTANCE 10

#define WINDOW_MIN_WIDTH 128
#define WINDOW_MIN_HEIGHT 128

#define WINDOW_MAX_WIDTH 8192
#define WINDOW_MAX_HEIGHT 8192

static const uint32_t x11_formats[] = {
	DRM_FORMAT_XRGB8888,
};

struct x11_backend {
	struct weston_backend	 base;
	struct weston_compositor *compositor;

	Display			*dpy;
	xcb_connection_t	*conn;
	xcb_screen_t		*screen;
	xcb_cursor_t		 null_cursor;
	struct wl_array		 keys;
	struct wl_event_source	*xcb_source;
	struct xkb_keymap	*xkb_keymap;
	unsigned int		 has_xkb;
	uint8_t			 xkb_event_base;
	int			 fullscreen;
	int			 no_input;

	int			 has_net_wm_state_fullscreen;

	/* We could map multi-pointer X to multiple wayland seats, but
	 * for now we only support core X input. */
	struct weston_seat		 core_seat;
	struct weston_coord_global	 prev_pos;

	struct {
		xcb_atom_t		 wm_protocols;
		xcb_atom_t		 wm_normal_hints;
		xcb_atom_t		 wm_size_hints;
		xcb_atom_t		 wm_delete_window;
		xcb_atom_t		 wm_class;
		xcb_atom_t		 net_wm_name;
		xcb_atom_t		 net_supporting_wm_check;
		xcb_atom_t		 net_supported;
		xcb_atom_t		 net_wm_icon;
		xcb_atom_t		 net_wm_state;
		xcb_atom_t		 net_wm_state_fullscreen;
		xcb_atom_t		 string;
		xcb_atom_t		 utf8_string;
		xcb_atom_t		 cardinal;
		xcb_atom_t		 xkb_names;
	} atom;
	xcb_generic_event_t *prev_event;

	const struct pixel_format_info **formats;
	unsigned int formats_count;
};

struct x11_head {
	struct weston_head	base;
};

struct x11_output {
	struct weston_output	base;
	struct x11_backend	*backend;

	xcb_window_t		window;
	struct weston_mode	mode;
	struct weston_mode	native;
	struct wl_event_source *finish_frame_timer;

	xcb_gc_t		gc;
	xcb_shm_seg_t		segment;
	struct weston_renderbuffer *renderbuffer;
	int			shm_id;
	void		       *buf;
	uint8_t			depth;
	int32_t                 scale;
	bool			resize_pending;
	bool			window_resized;
};

struct window_delete_data {
	struct x11_backend	*backend;
	xcb_window_t		window;
};

static void
x11_destroy(struct weston_backend *backend);

static inline struct x11_head *
to_x11_head(struct weston_head *base)
{
	if (base->backend->destroy != x11_destroy)
		return NULL;
	return container_of(base, struct x11_head, base);
}

static void
x11_output_destroy(struct weston_output *base);

static inline struct x11_output *
to_x11_output(struct weston_output *base)
{
	if (base->destroy != x11_output_destroy)
		return NULL;
	return container_of(base, struct x11_output, base);
}

static inline struct x11_backend *
to_x11_backend(struct weston_backend *backend)
{
	return container_of(backend, struct x11_backend, base);
}

static xcb_screen_t *
x11_compositor_get_default_screen(struct x11_backend *b)
{
	xcb_screen_iterator_t iter;
	int i, screen_nbr = XDefaultScreen(b->dpy);

	iter = xcb_setup_roots_iterator(xcb_get_setup(b->conn));
	for (i = 0; iter.rem; xcb_screen_next(&iter), i++)
		if (i == screen_nbr)
			return iter.data;

	return xcb_setup_roots_iterator(xcb_get_setup(b->conn)).data;
}

static struct xkb_keymap *
x11_backend_get_keymap(struct x11_backend *b)
{
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t *reply;
	struct xkb_rule_names names;
	struct xkb_keymap *ret;
	const char *value_all, *value_part;
	int length_all, length_part;

	memset(&names, 0, sizeof(names));

	cookie = xcb_get_property(b->conn, 0, b->screen->root,
				  b->atom.xkb_names, b->atom.string, 0, 1024);
	reply = xcb_get_property_reply(b->conn, cookie, NULL);
	if (reply == NULL)
		return NULL;

	value_all = xcb_get_property_value(reply);
	length_all = xcb_get_property_value_length(reply);
	value_part = value_all;

#define copy_prop_value(to) \
	length_part = strlen(value_part); \
	if (value_part + length_part < (value_all + length_all) && \
	    length_part > 0) \
		names.to = value_part; \
	value_part += length_part + 1;

	copy_prop_value(rules);
	copy_prop_value(model);
	copy_prop_value(layout);
	copy_prop_value(variant);
	copy_prop_value(options);
#undef copy_prop_value

	ret = xkb_keymap_new_from_names(b->compositor->xkb_context, &names, 0);

	free(reply);
	return ret;
}

static uint32_t
get_xkb_mod_mask(struct x11_backend *b, uint32_t in)
{
	struct weston_keyboard *keyboard =
		weston_seat_get_keyboard(&b->core_seat);
	struct weston_xkb_info *info = keyboard->xkb_info;
	uint32_t ret = 0;

	if ((in & ShiftMask) && info->shift_mod != XKB_MOD_INVALID)
		ret |= (1 << info->shift_mod);
	if ((in & LockMask) && info->caps_mod != XKB_MOD_INVALID)
		ret |= (1 << info->caps_mod);
	if ((in & ControlMask) && info->ctrl_mod != XKB_MOD_INVALID)
		ret |= (1 << info->ctrl_mod);
	if ((in & Mod1Mask) && info->alt_mod != XKB_MOD_INVALID)
		ret |= (1 << info->alt_mod);
	if ((in & Mod2Mask) && info->mod2_mod != XKB_MOD_INVALID)
		ret |= (1 << info->mod2_mod);
	if ((in & Mod3Mask) && info->mod3_mod != XKB_MOD_INVALID)
		ret |= (1 << info->mod3_mod);
	if ((in & Mod4Mask) && info->super_mod != XKB_MOD_INVALID)
		ret |= (1 << info->super_mod);
	if ((in & Mod5Mask) && info->mod5_mod != XKB_MOD_INVALID)
		ret |= (1 << info->mod5_mod);

	return ret;
}

static void
x11_backend_setup_xkb(struct x11_backend *b)
{
#ifndef HAVE_XCB_XKB
	weston_log("XCB-XKB not available during build\n");
	b->has_xkb = 0;
	b->xkb_event_base = 0;
	return;
#else
	struct weston_keyboard *keyboard;
	const xcb_query_extension_reply_t *ext;
	xcb_generic_error_t *error;
	xcb_void_cookie_t select;
	xcb_xkb_use_extension_cookie_t use_ext;
	xcb_xkb_use_extension_reply_t *use_ext_reply;
	xcb_xkb_per_client_flags_cookie_t pcf;
	xcb_xkb_per_client_flags_reply_t *pcf_reply;
	xcb_xkb_get_state_cookie_t state;
	xcb_xkb_get_state_reply_t *state_reply;
	uint32_t values[1] = { XCB_EVENT_MASK_PROPERTY_CHANGE };

	b->has_xkb = 0;
	b->xkb_event_base = 0;

	ext = xcb_get_extension_data(b->conn, &xcb_xkb_id);
	if (!ext) {
		weston_log("XKB extension not available on host X11 server\n");
		return;
	}
	b->xkb_event_base = ext->first_event;

	select = xcb_xkb_select_events_checked(b->conn,
					       XCB_XKB_ID_USE_CORE_KBD,
					       XCB_XKB_EVENT_TYPE_STATE_NOTIFY,
					       0,
					       XCB_XKB_EVENT_TYPE_STATE_NOTIFY,
					       0,
					       0,
					       NULL);
	error = xcb_request_check(b->conn, select);
	if (error) {
		weston_log("error: failed to select for XKB state events\n");
		free(error);
		return;
	}

	use_ext = xcb_xkb_use_extension(b->conn,
					XCB_XKB_MAJOR_VERSION,
					XCB_XKB_MINOR_VERSION);
	use_ext_reply = xcb_xkb_use_extension_reply(b->conn, use_ext, NULL);
	if (!use_ext_reply) {
		weston_log("couldn't start using XKB extension\n");
		return;
	}

	if (!use_ext_reply->supported) {
		weston_log("XKB extension version on the server is too old "
			   "(want %d.%d, has %d.%d)\n",
			   XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION,
			   use_ext_reply->serverMajor, use_ext_reply->serverMinor);
		free(use_ext_reply);
		return;
	}
	free(use_ext_reply);

	pcf = xcb_xkb_per_client_flags(b->conn,
				       XCB_XKB_ID_USE_CORE_KBD,
				       XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
				       XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
				       0,
				       0,
				       0);
	pcf_reply = xcb_xkb_per_client_flags_reply(b->conn, pcf, NULL);
	if (!pcf_reply ||
	    !(pcf_reply->value & XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT)) {
		weston_log("failed to set XKB per-client flags, not using "
			   "detectable repeat\n");
		free(pcf_reply);
		return;
	}
	free(pcf_reply);

	state = xcb_xkb_get_state(b->conn, XCB_XKB_ID_USE_CORE_KBD);
	state_reply = xcb_xkb_get_state_reply(b->conn, state, NULL);
	if (!state_reply) {
		weston_log("failed to get initial XKB state\n");
		return;
	}

	keyboard = weston_seat_get_keyboard(&b->core_seat);
	xkb_state_update_mask(keyboard->xkb_state.state,
			      get_xkb_mod_mask(b, state_reply->baseMods),
			      get_xkb_mod_mask(b, state_reply->latchedMods),
			      get_xkb_mod_mask(b, state_reply->lockedMods),
			      0,
			      0,
			      state_reply->group);

	free(state_reply);

	xcb_change_window_attributes(b->conn, b->screen->root,
				     XCB_CW_EVENT_MASK, values);

	b->has_xkb = 1;
#endif
}

#ifdef HAVE_XCB_XKB
static void
update_xkb_keymap(struct x11_backend *b)
{
	struct xkb_keymap *keymap;

	keymap = x11_backend_get_keymap(b);
	if (!keymap) {
		weston_log("failed to get XKB keymap\n");
		return;
	}
	weston_seat_update_keymap(&b->core_seat, keymap);
	xkb_keymap_unref(keymap);
}
#endif

static int
x11_input_create(struct x11_backend *b, int no_input)
{
	struct xkb_keymap *keymap;

	weston_seat_init(&b->core_seat, b->compositor, "default");

	if (no_input)
		return 0;

	weston_seat_init_pointer(&b->core_seat);

	keymap = x11_backend_get_keymap(b);
	if (weston_seat_init_keyboard(&b->core_seat, keymap) < 0)
		return -1;
	xkb_keymap_unref(keymap);

	x11_backend_setup_xkb(b);

	return 0;
}

static void
x11_input_destroy(struct x11_backend *b)
{
	weston_seat_release(&b->core_seat);
}

static int
x11_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
x11_output_repaint_gl(struct weston_output *output_base,
		      pixman_region32_t *damage)
{
	struct x11_output *output = to_x11_output(output_base);
	struct weston_compositor *ec;

	assert(output);

	ec = output->base.compositor;

	ec->renderer->repaint_output(output_base, damage, NULL);

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);

	wl_event_source_timer_update(output->finish_frame_timer, 10);
	return 0;
}

static void
set_clip_for_output(struct weston_output *output_base, pixman_region32_t *region)
{
	struct x11_output *output = to_x11_output(output_base);
	struct x11_backend *b;
	pixman_region32_t transformed_region;
	pixman_box32_t *rects;
	xcb_rectangle_t *output_rects;
	xcb_void_cookie_t cookie;
	int nrects, i;
	xcb_generic_error_t *err;

	if (!output)
		return;

	b = output->backend;

	pixman_region32_init(&transformed_region);
	weston_region_global_to_output(&transformed_region,
				       output_base,
				       region);

	rects = pixman_region32_rectangles(&transformed_region, &nrects);
	output_rects = calloc(nrects, sizeof(xcb_rectangle_t));

	if (output_rects == NULL) {
		pixman_region32_fini(&transformed_region);
		return;
	}

	for (i = 0; i < nrects; i++) {
		output_rects[i].x = rects[i].x1;
		output_rects[i].y = rects[i].y1;
		output_rects[i].width = rects[i].x2 - rects[i].x1;
		output_rects[i].height = rects[i].y2 - rects[i].y1;
	}

	pixman_region32_fini(&transformed_region);

	cookie = xcb_set_clip_rectangles_checked(b->conn, XCB_CLIP_ORDERING_UNSORTED,
					output->gc,
					0, 0, nrects,
					output_rects);
	err = xcb_request_check(b->conn, cookie);
	if (err != NULL) {
		weston_log("Failed to set clip rects, err: %d\n", err->error_code);
		free(err);
	}
	free(output_rects);
}


static int
x11_output_repaint_shm(struct weston_output *output_base,
		       pixman_region32_t *damage)
{
	struct x11_output *output = to_x11_output(output_base);
	const struct weston_renderer *renderer;
	pixman_image_t *image;
	struct weston_compositor *ec;
	struct x11_backend *b;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;

	assert(output);

	ec = output->base.compositor;
	renderer = ec->renderer;
	b = output->backend;

	image = renderer->pixman->renderbuffer_get_image(output->renderbuffer);

	ec->renderer->repaint_output(output_base, damage, output->renderbuffer);

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);
	set_clip_for_output(output_base, damage);
	cookie = xcb_shm_put_image_checked(b->conn, output->window, output->gc,
					pixman_image_get_width(image),
					pixman_image_get_height(image),
					0, 0,
					pixman_image_get_width(image),
					pixman_image_get_height(image),
					0, 0, output->depth, XCB_IMAGE_FORMAT_Z_PIXMAP,
					0, output->segment, 0);
	err = xcb_request_check(b->conn, cookie);
	if (err != NULL) {
		weston_log("Failed to put shm image, err: %d\n", err->error_code);
		free(err);
	}

	wl_event_source_timer_update(output->finish_frame_timer, 10);
	return 0;
}

static int
finish_frame_handler(void *data)
{
	struct x11_output *output = data;
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->base.compositor, &ts);
	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static void
x11_output_deinit_shm(struct x11_backend *b, struct x11_output *output)
{
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;
	xcb_free_gc(b->conn, output->gc);

	weston_renderbuffer_unref(output->renderbuffer);
	output->renderbuffer = NULL;
	cookie = xcb_shm_detach_checked(b->conn, output->segment);
	err = xcb_request_check(b->conn, cookie);
	if (err) {
		weston_log("xcb_shm_detach failed, error %d\n", err->error_code);
		free(err);
	}
	shmdt(output->buf);
}

static void
x11_output_set_wm_protocols(struct x11_backend *b,
			    struct x11_output *output)
{
	xcb_atom_t list[1];

	list[0] = b->atom.wm_delete_window;
	xcb_change_property (b->conn,
			     XCB_PROP_MODE_REPLACE,
			     output->window,
			     b->atom.wm_protocols,
			     XCB_ATOM_ATOM,
			     32,
			     ARRAY_LENGTH(list),
			     list);
}

struct wm_normal_hints {
	uint32_t flags;
	uint32_t pad[4];
	int32_t min_width, min_height;
	int32_t max_width, max_height;
	int32_t width_inc, height_inc;
	int32_t min_aspect_x, min_aspect_y;
	int32_t max_aspect_x, max_aspect_y;
	int32_t base_width, base_height;
	int32_t win_gravity;
};

#define WM_NORMAL_HINTS_MIN_SIZE	16
#define WM_NORMAL_HINTS_MAX_SIZE	32

static void
x11_output_set_icon(struct x11_backend *b,
		    struct x11_output *output, const char *filename)
{
	uint32_t *icon;
	int32_t width, height;
	pixman_image_t *image;

	image = load_image(filename);
	if (!image)
		return;
	width = pixman_image_get_width(image);
	height = pixman_image_get_height(image);
	icon = malloc(width * height * 4 + 8);
	if (!icon) {
		pixman_image_unref(image);
		return;
	}

	icon[0] = width;
	icon[1] = height;
	memcpy(icon + 2, pixman_image_get_data(image), width * height * 4);
	xcb_change_property(b->conn, XCB_PROP_MODE_REPLACE, output->window,
			    b->atom.net_wm_icon, b->atom.cardinal, 32,
			    width * height + 2, icon);
	free(icon);
	pixman_image_unref(image);
}

static void
x11_output_wait_for_map(struct x11_backend *b, struct x11_output *output)
{
	xcb_map_notify_event_t *map_notify;
	xcb_configure_notify_event_t *configure_notify;
	xcb_generic_event_t *event;
	int mapped = 0, configured = 0;
	uint8_t response_type;

	/* This isn't the nicest way to do this.  Ideally, we could
	 * just go back to the main loop and once we get the configure
	 * notify, we add the output to the compositor.  While we do
	 * support output hotplug, we can't start up with no outputs.
	 * We could add the output and then resize once we get the
	 * configure notify, but we don't want to start up and
	 * immediately resize the output.
	 *
	 * Also, some window managers don't give us our final
	 * fullscreen size before map_notify, so if we don't get a
	 * configure_notify before map_notify, we just wait for the
	 * first one and hope that's our size. */

	xcb_flush(b->conn);

	while (!mapped || !configured) {
		event = xcb_wait_for_event(b->conn);
		response_type = event->response_type & ~0x80;

		switch (response_type) {
		case XCB_MAP_NOTIFY:
			map_notify = (xcb_map_notify_event_t *) event;
			if (map_notify->window == output->window)
				mapped = 1;
			break;

		case XCB_CONFIGURE_NOTIFY:
			configure_notify =
				(xcb_configure_notify_event_t *) event;


			if (configure_notify->width % output->scale != 0 ||
			    configure_notify->height % output->scale != 0)
				weston_log("Resolution is not a multiple of screen size, rounding\n");
			output->mode.width = configure_notify->width;
			output->mode.height = configure_notify->height;
			configured = 1;
			break;
		}
	}
}

static xcb_visualtype_t *
find_visual_by_id(xcb_screen_t *screen,
		   xcb_visualid_t id)
{
	xcb_depth_iterator_t i;
	xcb_visualtype_iterator_t j;
	for (i = xcb_screen_allowed_depths_iterator(screen);
	     i.rem;
	     xcb_depth_next(&i)) {
		for (j = xcb_depth_visuals_iterator(i.data);
		     j.rem;
		     xcb_visualtype_next(&j)) {
			if (j.data->visual_id == id)
				return j.data;
		}
	}
	return 0;
}

static uint8_t
get_depth_of_visual(xcb_screen_t *screen,
		   xcb_visualid_t id)
{
	xcb_depth_iterator_t i;
	xcb_visualtype_iterator_t j;
	for (i = xcb_screen_allowed_depths_iterator(screen);
	     i.rem;
	     xcb_depth_next(&i)) {
		for (j = xcb_depth_visuals_iterator(i.data);
		     j.rem;
		     xcb_visualtype_next(&j)) {
			if (j.data->visual_id == id)
				return i.data->depth;
		}
	}
	return 0;
}

static const struct pixel_format_info *
x11_output_get_shm_pixel_format(struct x11_output *output)
{
	struct x11_backend *b = output->backend;
	xcb_visualtype_t *visual_type;
	xcb_screen_t *screen;
	xcb_format_iterator_t fmt;
	const xcb_query_extension_reply_t *ext;
	int bitsperpixel = 0;

	/* Check if SHM is available */
	ext = xcb_get_extension_data(b->conn, &xcb_shm_id);
	if (ext == NULL || !ext->present) {
		/* SHM is missing */
		weston_log("SHM extension is not available\n");
		errno = ENOENT;
		return NULL;
	}

	screen = x11_compositor_get_default_screen(b);
	visual_type = find_visual_by_id(screen, screen->root_visual);
	if (!visual_type) {
		weston_log("Failed to lookup visual for root window\n");
		errno = ENOENT;
		return NULL;
	}
	weston_log("Found visual, bits per value: %d, red_mask: %.8x, green_mask: %.8x, blue_mask: %.8x\n",
		visual_type->bits_per_rgb_value,
		visual_type->red_mask,
		visual_type->green_mask,
		visual_type->blue_mask);
	output->depth = get_depth_of_visual(screen, screen->root_visual);
	weston_log("Visual depth is %d\n", output->depth);

	for (fmt = xcb_setup_pixmap_formats_iterator(xcb_get_setup(b->conn));
	     fmt.rem;
	     xcb_format_next(&fmt)) {
		if (fmt.data->depth == output->depth) {
			bitsperpixel = fmt.data->bits_per_pixel;
			break;
		}
	}
	weston_log("Found format for depth %d, bpp: %d\n",
		output->depth, bitsperpixel);

	if  (bitsperpixel == 32 &&
	     visual_type->red_mask == 0xff0000 &&
	     visual_type->green_mask == 0x00ff00 &&
	     visual_type->blue_mask == 0x0000ff) {
		weston_log("Will use x8r8g8b8 format for SHM surfaces\n");
		return pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8);
	} else if (bitsperpixel == 16 &&
	           visual_type->red_mask == 0x00f800 &&
	           visual_type->green_mask == 0x0007e0 &&
	           visual_type->blue_mask == 0x00001f) {
		weston_log("Will use r5g6b5 format for SHM surfaces\n");
		return pixel_format_get_info_by_pixman(PIXMAN_r5g6b5);
	} else {
		weston_log("Can't find appropriate format for SHM pixmap\n");
		errno = ENOTSUP;
		return NULL;
	}
}

static int
x11_output_init_shm(struct x11_backend *b, struct x11_output *output,
		    const struct pixel_format_info *pfmt, int width, int height)
{
	struct weston_renderer *renderer = output->base.compositor->renderer;
	int bitsperpixel = pfmt->bpp;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;

	/* Create SHM segment and attach it */
	output->shm_id = shmget(IPC_PRIVATE, width * height * (bitsperpixel / 8), IPC_CREAT | S_IRWXU);
	if (output->shm_id == -1) {
		weston_log("x11shm: failed to allocate SHM segment\n");
		return -1;
	}
	output->buf = shmat(output->shm_id, NULL, 0 /* read/write */);
	if (-1 == (long)output->buf) {
		weston_log("x11shm: failed to attach SHM segment\n");
		return -1;
	}
	output->segment = xcb_generate_id(b->conn);
	cookie = xcb_shm_attach_checked(b->conn, output->segment, output->shm_id, 1);
	err = xcb_request_check(b->conn, cookie);
	if (err) {
		weston_log("x11shm: xcb_shm_attach error %d, op code %d, resource id %d\n",
			   err->error_code, err->major_code, err->minor_code);
		free(err);
		return -1;
	}

	shmctl(output->shm_id, IPC_RMID, NULL);

	/* Now create pixman image */
	output->renderbuffer =
		renderer->pixman->create_image_from_ptr(&output->base,
							pfmt, width, height,
							output->buf,
							width * (bitsperpixel / 8));

	output->gc = xcb_generate_id(b->conn);
	xcb_create_gc(b->conn, output->gc, output->window, 0, NULL);

	return 0;
}

static int
x11_output_switch_mode(struct weston_output *base, struct weston_mode *mode)
{
	struct x11_backend *b;
	struct x11_output *output = to_x11_output(base);
	struct weston_size fb_size;
	static uint32_t values[2];

	assert(output);

	b = output->backend;

	if (mode->width == output->mode.width &&
	    mode->height == output->mode.height)
		return 0;

	if (mode->width < WINDOW_MIN_WIDTH || mode->width > WINDOW_MAX_WIDTH)
		return -1;

	if (mode->height < WINDOW_MIN_HEIGHT || mode->height > WINDOW_MAX_HEIGHT)
		return -1;

	/* xcb_configure_window will create an event, and we could end up
	   being called twice */
	output->resize_pending = true;

	/* window could've been resized by the user, so don't do it twice */
	if (!output->window_resized) {
		values[0] = mode->width;
		values[1] = mode->height;
		xcb_configure_window(b->conn, output->window, XCB_CONFIG_WINDOW_WIDTH |
				     XCB_CONFIG_WINDOW_HEIGHT, values);
	}

	fb_size.width = output->mode.width = mode->width;
	fb_size.height = output->mode.height = mode->height;

	weston_renderer_resize_output(&output->base, &fb_size, NULL);

	if (base->compositor->renderer->type == WESTON_RENDERER_PIXMAN) {
		const struct pixel_format_info *pfmt;
		x11_output_deinit_shm(b, output);
		pfmt = x11_output_get_shm_pixel_format(output);
		if (!pfmt)
			return -1;
		if (x11_output_init_shm(b, output, pfmt,
					fb_size.width, fb_size.height) < 0) {
			weston_log("Failed to initialize SHM for the X11 output\n");
			return -1;
		}
	}

	output->resize_pending = false;
	output->window_resized = false;

	return 0;
}

static int
x11_output_disable(struct weston_output *base)
{
	const struct weston_renderer *renderer = base->compositor->renderer;
	struct x11_output *output = to_x11_output(base);
	struct x11_backend *backend;

	assert(output);

	backend = output->backend;

	if (!output->base.enabled)
		return 0;

	wl_event_source_remove(output->finish_frame_timer);

	if (renderer->type == WESTON_RENDERER_PIXMAN) {
		x11_output_deinit_shm(backend, output);
		renderer->pixman->output_destroy(&output->base);
	} else {
		renderer->gl->output_destroy(&output->base);
	}

	xcb_destroy_window(backend->conn, output->window);
	xcb_flush(backend->conn);

	return 0;
}

static void
x11_output_destroy(struct weston_output *base)
{
	struct x11_output *output = to_x11_output(base);

	assert(output);

	x11_output_disable(&output->base);
	weston_output_release(&output->base);

	free(output);
}

static int
x11_output_enable(struct weston_output *base)
{
	const struct weston_renderer *renderer = base->compositor->renderer;
	struct x11_output *output = to_x11_output(base);
	const struct weston_mode *mode = output->base.current_mode;
	struct x11_backend *b;

	assert(output);

	b = output->backend;

	static const char name[] = "Weston Compositor";
	static const char class[] = "weston-1\0Weston Compositor";
	char *title = NULL;
	xcb_screen_t *screen;
	struct wm_normal_hints normal_hints;
	struct wl_event_loop *loop;
	char *icon_filename;

	int ret;
	uint32_t mask = XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
	xcb_atom_t atom_list[1];
	uint32_t values[2] = {
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY,
		0
	};

	if (!b->no_input)
		values[0] |=
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_KEY_RELEASE |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE |
			XCB_EVENT_MASK_POINTER_MOTION |
			XCB_EVENT_MASK_ENTER_WINDOW |
			XCB_EVENT_MASK_LEAVE_WINDOW |
			XCB_EVENT_MASK_KEYMAP_STATE |
			XCB_EVENT_MASK_FOCUS_CHANGE;

	values[1] = b->null_cursor;
	output->window = xcb_generate_id(b->conn);
	screen = x11_compositor_get_default_screen(b);
	xcb_create_window(b->conn,
			  XCB_COPY_FROM_PARENT,
			  output->window,
			  screen->root,
			  0, 0,
			  mode->width, mode->height,
			  0,
			  XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  screen->root_visual,
			  mask, values);

	if (b->fullscreen) {
		atom_list[0] = b->atom.net_wm_state_fullscreen;
		xcb_change_property(b->conn, XCB_PROP_MODE_REPLACE,
				    output->window,
				    b->atom.net_wm_state,
				    XCB_ATOM_ATOM, 32,
				    ARRAY_LENGTH(atom_list), atom_list);
	} else {
		memset(&normal_hints, 0, sizeof normal_hints);
		normal_hints.flags =
			WM_NORMAL_HINTS_MAX_SIZE | WM_NORMAL_HINTS_MIN_SIZE;
		normal_hints.min_width = WINDOW_MIN_WIDTH;
		normal_hints.min_height = WINDOW_MIN_HEIGHT;
		normal_hints.max_width = WINDOW_MAX_WIDTH;
		normal_hints.max_height = WINDOW_MAX_HEIGHT;
		xcb_change_property(b->conn, XCB_PROP_MODE_REPLACE, output->window,
				    b->atom.wm_normal_hints,
				    b->atom.wm_size_hints, 32,
				    sizeof normal_hints / 4,
				    (uint8_t *) &normal_hints);
	}

	/* Set window name.  Don't bother with non-EWMH WMs. */
	if (output->base.name) {
		if (asprintf(&title, "%s - %s", name, output->base.name) < 0)
			title = NULL;
	} else {
		title = strdup(name);
	}

	if (title) {
		xcb_change_property(b->conn, XCB_PROP_MODE_REPLACE, output->window,
				    b->atom.net_wm_name, b->atom.utf8_string, 8,
				    strlen(title), title);
		free(title);
	} else {
		goto err;
	}

	xcb_change_property(b->conn, XCB_PROP_MODE_REPLACE, output->window,
			    b->atom.wm_class, b->atom.string, 8,
			    sizeof class, class);

	icon_filename = file_name_with_datadir("wayland.png");
	x11_output_set_icon(b, output, icon_filename);
	free(icon_filename);

	x11_output_set_wm_protocols(b, output);

	xcb_map_window(b->conn, output->window);

	if (b->fullscreen)
		x11_output_wait_for_map(b, output);

	if (renderer->type == WESTON_RENDERER_PIXMAN) {
		const struct pixman_renderer_output_options options = {
			.use_shadow = true,
			.fb_size = {
				.width = mode->width,
				.height = mode->height
			},
			.format = x11_output_get_shm_pixel_format(output)
		};
		if (!options.format)
			goto err;
		if (renderer->pixman->output_create(&output->base, &options) < 0) {
			weston_log("Failed to create pixman renderer for output\n");
			goto err;
		}
		if (x11_output_init_shm(b, output, options.format,
					mode->width, mode->height) < 0) {
			weston_log("Failed to initialize SHM for the X11 output\n");
			renderer->pixman->output_destroy(&output->base);
			goto err;
		}

		output->base.repaint = x11_output_repaint_shm;
	} else {
		/* eglCreatePlatformWindowSurfaceEXT takes a Window*
		 * but eglCreateWindowSurface takes a Window. */
		Window xid = (Window) output->window;
		const struct gl_renderer_output_options options = {
			.window_for_legacy = (EGLNativeWindowType) (uintptr_t) output->window,
			.window_for_platform = &xid,
			.formats = b->formats,
			.formats_count = b->formats_count,
			.area.x = 0,
			.area.y = 0,
			.area.width = mode->width,
			.area.height = mode->height,
			.fb_size.width = mode->width,
			.fb_size.height = mode->height,
		};

		ret = renderer->gl->output_window_create(base, &options);
		if (ret < 0)
			goto err;

		output->base.repaint = x11_output_repaint_gl;
	}

	output->base.start_repaint_loop = x11_output_start_repaint_loop;
	output->base.assign_planes = NULL;
	output->base.set_backlight = NULL;
	output->base.set_dpms = NULL;
	output->base.switch_mode = x11_output_switch_mode;

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, finish_frame_handler, output);

	weston_log("x11 output %dx%d, window id %d\n",
		   mode->width, mode->height, output->window);

	return 0;

err:
	xcb_destroy_window(b->conn, output->window);
	xcb_flush(b->conn);

	return -1;
}

static int
x11_output_set_size(struct weston_output *base, int width, int height)
{
	struct x11_output *output = to_x11_output(base);
	struct x11_backend *b;
	struct weston_head *head;
	xcb_screen_t *scrn;
	int output_width, output_height;

	if (!output)
		return -1;

	b = output->backend;
	scrn = b->screen;

	/* We can only be called once. */
	assert(!output->base.current_mode);

	/* Make sure we have scale set. */
	assert(output->base.scale);

	if (width < WINDOW_MIN_WIDTH) {
		weston_log("Invalid width \"%d\" for output %s\n",
			   width, output->base.name);
		return -1;
	}

	if (height < WINDOW_MIN_HEIGHT) {
		weston_log("Invalid height \"%d\" for output %s\n",
			   height, output->base.name);
		return -1;
	}

	wl_list_for_each(head, &output->base.head_list, output_link) {
		weston_head_set_monitor_strings(head, "weston-X11", "none", NULL);
		weston_head_set_physical_size(head,
			width * scrn->width_in_millimeters / scrn->width_in_pixels,
			height * scrn->height_in_millimeters / scrn->height_in_pixels);
	}

	output_width = width * output->base.scale;
	output_height = height * output->base.scale;

	output->mode.flags =
		WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;

	output->mode.width = output_width;
	output->mode.height = output_height;
	output->mode.refresh = 60000;
	output->native = output->mode;
	output->scale = output->base.scale;
	wl_list_insert(&output->base.mode_list, &output->mode.link);

	output->base.current_mode = &output->mode;
	output->base.native_mode = &output->native;
	output->base.native_scale = output->base.scale;

	return 0;
}

static struct weston_output *
x11_output_create(struct weston_backend *backend, const char *name)
{
	struct x11_backend *b = container_of(backend, struct x11_backend, base);
	struct weston_compositor *compositor = b->compositor;
	struct x11_output *output;

	/* name can't be NULL. */
	assert(name);

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	weston_output_init(&output->base, compositor, name);

	output->base.destroy = x11_output_destroy;
	output->base.disable = x11_output_disable;
	output->base.enable = x11_output_enable;
	output->base.attach_head = NULL;

	output->backend = b;

	weston_compositor_add_pending_output(&output->base, compositor);

	return &output->base;
}

static int
x11_head_create(struct weston_backend *base, const char *name)
{
	struct x11_backend *backend = to_x11_backend(base);
	struct x11_head *head;

	assert(name);

	head = zalloc(sizeof *head);
	if (!head)
		return -1;

	weston_head_init(&head->base, name);

	head->base.backend = &backend->base;

	weston_head_set_connection_status(&head->base, true);
	weston_compositor_add_head(backend->compositor, &head->base);

	return 0;
}

static void
x11_head_destroy(struct weston_head *base)
{
	struct x11_head *head = to_x11_head(base);

	assert(head);

	weston_head_release(&head->base);
	free(head);
}

static struct x11_output *
x11_backend_find_output(struct x11_backend *b, xcb_window_t window)
{
	struct x11_output *output;

	wl_list_for_each(output, &b->compositor->output_list, base.link) {
		if (output->window == window)
			return output;
	}

	return NULL;
}

static void
x11_backend_delete_window(struct x11_backend *b, xcb_window_t window)
{
	struct x11_output *output;

	output = x11_backend_find_output(b, window);
	if (output)
		x11_output_destroy(&output->base);

	if (wl_list_empty(&b->compositor->output_list))
		weston_compositor_exit(b->compositor);
}

static void delete_cb(void *data)
{
	struct window_delete_data *wd = data;

	x11_backend_delete_window(wd->backend, wd->window);
	free(wd);
}

#ifdef HAVE_XCB_XKB
static void
update_xkb_state(struct x11_backend *b, xcb_xkb_state_notify_event_t *state)
{
	struct weston_keyboard *keyboard =
		weston_seat_get_keyboard(&b->core_seat);

	xkb_state_update_mask(keyboard->xkb_state.state,
			      get_xkb_mod_mask(b, state->baseMods),
			      get_xkb_mod_mask(b, state->latchedMods),
			      get_xkb_mod_mask(b, state->lockedMods),
			      0,
			      0,
			      state->group);

	notify_modifiers(&b->core_seat,
			 wl_display_next_serial(b->compositor->wl_display));
}
#endif

/**
 * This is monumentally unpleasant.  If we don't have XCB-XKB bindings,
 * the best we can do (given that XCB also lacks XI2 support), is to take
 * the state from the core key events.  Unfortunately that only gives us
 * the effective (i.e. union of depressed/latched/locked) state, and we
 * need the granularity.
 *
 * So we still update the state with every key event we see, but also use
 * the state field from X11 events as a mask so we don't get any stuck
 * modifiers.
 */
static void
update_xkb_state_from_core(struct x11_backend *b, uint16_t x11_mask)
{
	uint32_t mask = get_xkb_mod_mask(b, x11_mask);
	struct weston_keyboard *keyboard
		= weston_seat_get_keyboard(&b->core_seat);

	xkb_state_update_mask(keyboard->xkb_state.state,
			      keyboard->modifiers.mods_depressed & mask,
			      keyboard->modifiers.mods_latched & mask,
			      keyboard->modifiers.mods_locked & mask,
			      0,
			      0,
			      (x11_mask >> 13) & 3);
	notify_modifiers(&b->core_seat,
			 wl_display_next_serial(b->compositor->wl_display));
}

static void
x11_backend_deliver_button_event(struct x11_backend *b,
				 xcb_generic_event_t *event)
{
	xcb_button_press_event_t *button_event =
		(xcb_button_press_event_t *) event;
	uint32_t button;
	struct x11_output *output;
	struct weston_pointer_axis_event weston_event;
	bool is_button_pressed = event->response_type == XCB_BUTTON_PRESS;
	struct timespec time = { 0 };

	assert(event->response_type == XCB_BUTTON_PRESS ||
	       event->response_type == XCB_BUTTON_RELEASE);

	output = x11_backend_find_output(b, button_event->event);
	if (!output)
		return;

	if (is_button_pressed)
		xcb_grab_pointer(b->conn, 0, output->window,
				 XCB_EVENT_MASK_BUTTON_PRESS |
				 XCB_EVENT_MASK_BUTTON_RELEASE |
				 XCB_EVENT_MASK_POINTER_MOTION |
				 XCB_EVENT_MASK_ENTER_WINDOW |
				 XCB_EVENT_MASK_LEAVE_WINDOW,
				 XCB_GRAB_MODE_ASYNC,
				 XCB_GRAB_MODE_ASYNC,
				 output->window, XCB_CURSOR_NONE,
				 button_event->time);
	else
		xcb_ungrab_pointer(b->conn, button_event->time);

	if (!b->has_xkb)
		update_xkb_state_from_core(b, button_event->state);

	switch (button_event->detail) {
	case 1:
		button = BTN_LEFT;
		break;
	case 2:
		button = BTN_MIDDLE;
		break;
	case 3:
		button = BTN_RIGHT;
		break;
	case 4:
		/* Axis are measured in pixels, but the xcb events are discrete
		 * steps. Therefore move the axis by some pixels every step. */
		if (is_button_pressed) {
			weston_event.value = -DEFAULT_AXIS_STEP_DISTANCE;
			weston_event.discrete = -1;
			weston_event.has_discrete = true;
			weston_event.axis =
				WL_POINTER_AXIS_VERTICAL_SCROLL;
			weston_compositor_get_time(&time);
			notify_axis(&b->core_seat, &time, &weston_event);
			notify_pointer_frame(&b->core_seat);
		}
		return;
	case 5:
		if (is_button_pressed) {
			weston_event.value = DEFAULT_AXIS_STEP_DISTANCE;
			weston_event.discrete = 1;
			weston_event.has_discrete = true;
			weston_event.axis =
				WL_POINTER_AXIS_VERTICAL_SCROLL;
			weston_compositor_get_time(&time);
			notify_axis(&b->core_seat, &time, &weston_event);
			notify_pointer_frame(&b->core_seat);
		}
		return;
	case 6:
		if (is_button_pressed) {
			weston_event.value = -DEFAULT_AXIS_STEP_DISTANCE;
			weston_event.discrete = -1;
			weston_event.has_discrete = true;
			weston_event.axis =
				WL_POINTER_AXIS_HORIZONTAL_SCROLL;
			weston_compositor_get_time(&time);
			notify_axis(&b->core_seat, &time, &weston_event);
			notify_pointer_frame(&b->core_seat);
		}
		return;
	case 7:
		if (is_button_pressed) {
			weston_event.value = DEFAULT_AXIS_STEP_DISTANCE;
			weston_event.discrete = 1;
			weston_event.has_discrete = true;
			weston_event.axis =
				WL_POINTER_AXIS_HORIZONTAL_SCROLL;
			weston_compositor_get_time(&time);
			notify_axis(&b->core_seat, &time, &weston_event);
			notify_pointer_frame(&b->core_seat);
		}
		return;
	default:
		button = button_event->detail + BTN_SIDE - 8;
		break;
	}

	weston_compositor_get_time(&time);

	notify_button(&b->core_seat, &time, button,
		      is_button_pressed ? WL_POINTER_BUTTON_STATE_PRESSED :
					  WL_POINTER_BUTTON_STATE_RELEASED);
	notify_pointer_frame(&b->core_seat);
}

static void
x11_backend_deliver_motion_event(struct x11_backend *b,
				 xcb_generic_event_t *event)
{
	struct x11_output *output;
	struct weston_coord_global pos;
	struct weston_pointer_motion_event motion_event = { 0 };
	xcb_motion_notify_event_t *motion_notify =
			(xcb_motion_notify_event_t *) event;
	struct timespec time;

	if (!b->has_xkb)
		update_xkb_state_from_core(b, motion_notify->state);
	output = x11_backend_find_output(b, motion_notify->event);
	if (!output)
		return;

	pos = weston_coord_global_from_output_point(motion_notify->event_x,
						    motion_notify->event_y,
						    &output->base);

	motion_event = (struct weston_pointer_motion_event) {
		.mask = WESTON_POINTER_MOTION_REL,
		.rel = weston_coord_sub(pos.c, b->prev_pos.c),
	};
	weston_compositor_get_time(&time);
	notify_motion(&b->core_seat, &time, &motion_event);
	notify_pointer_frame(&b->core_seat);

	b->prev_pos = pos;
}

static void
x11_backend_deliver_enter_event(struct x11_backend *b,
				xcb_generic_event_t *event)
{
	struct x11_output *output;
	struct weston_coord_global pos;

	xcb_enter_notify_event_t *enter_notify =
			(xcb_enter_notify_event_t *) event;
	if (enter_notify->state >= Button1Mask)
		return;
	if (!b->has_xkb)
		update_xkb_state_from_core(b, enter_notify->state);
	output = x11_backend_find_output(b, enter_notify->event);
	if (!output)
		return;

	pos = weston_coord_global_from_output_point(enter_notify->event_x,
						    enter_notify->event_y,
						    &output->base);

	notify_pointer_focus(&b->core_seat, &output->base, pos);

	b->prev_pos = pos;
}

static int
x11_backend_next_event(struct x11_backend *b,
		       xcb_generic_event_t **event, uint32_t mask)
{
	if (mask & WL_EVENT_READABLE)
		*event = xcb_poll_for_event(b->conn);
	else
		*event = xcb_poll_for_queued_event(b->conn);

	return *event != NULL;
}

static int
x11_backend_handle_event(int fd, uint32_t mask, void *data)
{
	struct x11_backend *b = data;
	struct x11_output *output;
	xcb_generic_event_t *event;
	xcb_client_message_event_t *client_message;
	xcb_enter_notify_event_t *enter_notify;
	xcb_key_press_event_t *key_press, *key_release;
	xcb_keymap_notify_event_t *keymap_notify;
	xcb_focus_in_event_t *focus_in;
	xcb_expose_event_t *expose;
	xcb_configure_notify_event_t *configure;
	xcb_atom_t atom;
	xcb_window_t window;
	uint32_t *k;
	uint32_t i, set;
	uint8_t response_type;
	int count;
	struct timespec time;

	count = 0;
	while (x11_backend_next_event(b, &event, mask)) {
		response_type = event->response_type & ~0x80;

		switch (b->prev_event ? b->prev_event->response_type & ~0x80 : 0x80) {
		case XCB_KEY_RELEASE:
			/* Suppress key repeat events; this is only used if we
			 * don't have XCB XKB support. */
			key_release = (xcb_key_press_event_t *) b->prev_event;
			key_press = (xcb_key_press_event_t *) event;
			if (response_type == XCB_KEY_PRESS &&
			    key_release->time == key_press->time &&
			    key_release->detail == key_press->detail) {
				/* Don't deliver the held key release
				 * event or the new key press event. */
				free(event);
				free(b->prev_event);
				b->prev_event = NULL;
				continue;
			} else {
				/* Deliver the held key release now
				 * and fall through and handle the new
				 * event below. */
				update_xkb_state_from_core(b, key_release->state);
				weston_compositor_get_time(&time);
				notify_key(&b->core_seat,
					   &time,
					   key_release->detail - 8,
					   WL_KEYBOARD_KEY_STATE_RELEASED,
					   STATE_UPDATE_AUTOMATIC);
				free(b->prev_event);
				b->prev_event = NULL;
				break;
			}

		case XCB_FOCUS_IN:
			assert(response_type == XCB_KEYMAP_NOTIFY);
			keymap_notify = (xcb_keymap_notify_event_t *) event;
			b->keys.size = 0;
			for (i = 0; i < ARRAY_LENGTH(keymap_notify->keys) * 8; i++) {
				set = keymap_notify->keys[i >> 3] &
					(1 << (i & 7));
				if (set) {
					k = wl_array_add(&b->keys, sizeof *k);
					*k = i;
				}
			}

			/* Unfortunately the state only comes with the enter
			 * event, rather than with the focus event.  I'm not
			 * sure of the exact semantics around it and whether
			 * we can ensure that we get both? */
			notify_keyboard_focus_in(&b->core_seat, &b->keys,
						 STATE_UPDATE_AUTOMATIC);

			free(b->prev_event);
			b->prev_event = NULL;
			break;

		default:
			/* No previous event held */
			break;
		}

		switch (response_type) {
		case XCB_KEY_PRESS:
			key_press = (xcb_key_press_event_t *) event;
			if (!b->has_xkb)
				update_xkb_state_from_core(b, key_press->state);
			weston_compositor_get_time(&time);
			notify_key(&b->core_seat,
				   &time,
				   key_press->detail - 8,
				   WL_KEYBOARD_KEY_STATE_PRESSED,
				   b->has_xkb ? STATE_UPDATE_NONE :
						STATE_UPDATE_AUTOMATIC);
			break;
		case XCB_KEY_RELEASE:
			/* If we don't have XKB, we need to use the lame
			 * autorepeat detection above. */
			if (!b->has_xkb) {
				b->prev_event = event;
				break;
			}
			key_release = (xcb_key_press_event_t *) event;
			weston_compositor_get_time(&time);
			notify_key(&b->core_seat,
				   &time,
				   key_release->detail - 8,
				   WL_KEYBOARD_KEY_STATE_RELEASED,
				   STATE_UPDATE_NONE);
			break;
		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			x11_backend_deliver_button_event(b, event);
			break;
		case XCB_MOTION_NOTIFY:
			x11_backend_deliver_motion_event(b, event);
			break;

		case XCB_EXPOSE:
			expose = (xcb_expose_event_t *) event;
			output = x11_backend_find_output(b, expose->window);
			if (!output)
				break;

			weston_output_damage(&output->base);
			weston_output_schedule_repaint(&output->base);
			break;

		case XCB_ENTER_NOTIFY:
			x11_backend_deliver_enter_event(b, event);
			break;

		case XCB_LEAVE_NOTIFY:
			enter_notify = (xcb_enter_notify_event_t *) event;
			if (enter_notify->state >= Button1Mask)
				break;
			if (!b->has_xkb)
				update_xkb_state_from_core(b, enter_notify->state);
			clear_pointer_focus(&b->core_seat);
			break;

		case XCB_CLIENT_MESSAGE:
			client_message = (xcb_client_message_event_t *) event;
			atom = client_message->data.data32[0];
			window = client_message->window;
			if (atom == b->atom.wm_delete_window) {
				struct wl_event_loop *loop;
				struct window_delete_data *data = malloc(sizeof *data);

				/* if malloc failed we should at least try to
				 * delete the window, even if it may result in
				 * a crash.
				 */
				if (!data) {
					x11_backend_delete_window(b, window);
					break;
				}
				data->backend = b;
				data->window = window;
				loop = wl_display_get_event_loop(b->compositor->wl_display);
				wl_event_loop_add_idle(loop, delete_cb, data);
			}
			break;

		case XCB_CONFIGURE_NOTIFY:
			configure = (struct xcb_configure_notify_event_t *) event;
			struct x11_output *output =
				x11_backend_find_output(b, configure->window);

			if (!output || output->resize_pending)
				break;

			struct weston_mode mode = output->mode;

			if (mode.width == configure->width &&
			    mode.height == configure->height)
				break;

			output->window_resized = true;

			mode.width = configure->width;
			mode.height = configure->height;

			if (weston_output_mode_set_native(&output->base,
							  &mode, output->scale) < 0)
				weston_log("Mode switch failed\n");

			break;

		case XCB_FOCUS_IN:
			focus_in = (xcb_focus_in_event_t *) event;
			if (focus_in->mode == XCB_NOTIFY_MODE_WHILE_GRABBED)
				break;

			b->prev_event = event;
			break;

		case XCB_FOCUS_OUT:
			focus_in = (xcb_focus_in_event_t *) event;
			if (focus_in->mode == XCB_NOTIFY_MODE_WHILE_GRABBED ||
			    focus_in->mode == XCB_NOTIFY_MODE_UNGRAB)
				break;
			notify_keyboard_focus_out(&b->core_seat);
			break;

		default:
			break;
		}

#ifdef HAVE_XCB_XKB
		if (b->has_xkb) {
			if (response_type == b->xkb_event_base) {
				xcb_xkb_state_notify_event_t *state =
					(xcb_xkb_state_notify_event_t *) event;
				if (state->xkbType == XCB_XKB_STATE_NOTIFY)
					update_xkb_state(b, state);
			} else if (response_type == XCB_PROPERTY_NOTIFY) {
				xcb_property_notify_event_t *prop_notify =
					(xcb_property_notify_event_t *) event;
				if (prop_notify->window == b->screen->root &&
				    prop_notify->atom == b->atom.xkb_names &&
				    prop_notify->state == XCB_PROPERTY_NEW_VALUE)
					update_xkb_keymap(b);
			}
		}
#endif

		count++;
		if (b->prev_event != event)
			free(event);
	}

	switch (b->prev_event ? b->prev_event->response_type & ~0x80 : 0x80) {
	case XCB_KEY_RELEASE:
		key_release = (xcb_key_press_event_t *) b->prev_event;
		update_xkb_state_from_core(b, key_release->state);
		weston_compositor_get_time(&time);
		notify_key(&b->core_seat,
			   &time,
			   key_release->detail - 8,
			   WL_KEYBOARD_KEY_STATE_RELEASED,
			   STATE_UPDATE_AUTOMATIC);
		free(b->prev_event);
		b->prev_event = NULL;
		break;
	default:
		break;
	}

	return count;
}

#define F(field) offsetof(struct x11_backend, field)

static void
x11_backend_get_resources(struct x11_backend *b)
{
	static const struct { const char *name; int offset; } atoms[] = {
		{ "WM_PROTOCOLS",	F(atom.wm_protocols) },
		{ "WM_NORMAL_HINTS",	F(atom.wm_normal_hints) },
		{ "WM_SIZE_HINTS",	F(atom.wm_size_hints) },
		{ "WM_DELETE_WINDOW",	F(atom.wm_delete_window) },
		{ "WM_CLASS",		F(atom.wm_class) },
		{ "_NET_WM_NAME",	F(atom.net_wm_name) },
		{ "_NET_WM_ICON",	F(atom.net_wm_icon) },
		{ "_NET_WM_STATE",	F(atom.net_wm_state) },
		{ "_NET_WM_STATE_FULLSCREEN", F(atom.net_wm_state_fullscreen) },
		{ "_NET_SUPPORTING_WM_CHECK",
					F(atom.net_supporting_wm_check) },
		{ "_NET_SUPPORTED",     F(atom.net_supported) },
		{ "STRING",		F(atom.string) },
		{ "UTF8_STRING",	F(atom.utf8_string) },
		{ "CARDINAL",		F(atom.cardinal) },
		{ "_XKB_RULES_NAMES",	F(atom.xkb_names) },
	};

	xcb_intern_atom_cookie_t cookies[ARRAY_LENGTH(atoms)];
	xcb_intern_atom_reply_t *reply;
	xcb_pixmap_t pixmap;
	xcb_gc_t gc;
	unsigned int i;
	uint8_t data[] = { 0, 0, 0, 0 };

	for (i = 0; i < ARRAY_LENGTH(atoms); i++)
		cookies[i] = xcb_intern_atom (b->conn, 0,
					      strlen(atoms[i].name),
					      atoms[i].name);

	for (i = 0; i < ARRAY_LENGTH(atoms); i++) {
		reply = xcb_intern_atom_reply (b->conn, cookies[i], NULL);
		*(xcb_atom_t *) ((char *) b + atoms[i].offset) = reply->atom;
		free(reply);
	}

	pixmap = xcb_generate_id(b->conn);
	gc = xcb_generate_id(b->conn);
	xcb_create_pixmap(b->conn, 1, pixmap, b->screen->root, 1, 1);
	xcb_create_gc(b->conn, gc, pixmap, 0, NULL);
	xcb_put_image(b->conn, XCB_IMAGE_FORMAT_XY_PIXMAP,
		      pixmap, gc, 1, 1, 0, 0, 0, 32, sizeof data, data);
	b->null_cursor = xcb_generate_id(b->conn);
	xcb_create_cursor (b->conn, b->null_cursor,
			   pixmap, pixmap, 0, 0, 0,  0, 0, 0,  1, 1);
	xcb_free_gc(b->conn, gc);
	xcb_free_pixmap(b->conn, pixmap);
}

static void
x11_backend_get_wm_info(struct x11_backend *c)
{
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t *reply;
	xcb_atom_t *atom;
	unsigned int i;

	cookie = xcb_get_property(c->conn, 0, c->screen->root,
				  c->atom.net_supported,
				  XCB_ATOM_ATOM, 0, 1024);
	reply = xcb_get_property_reply(c->conn, cookie, NULL);
	if (reply == NULL)
		return;

	atom = (xcb_atom_t *) xcb_get_property_value(reply);

	for (i = 0; i < reply->value_len; i++) {
		if (atom[i] == c->atom.net_wm_state_fullscreen)
			c->has_net_wm_state_fullscreen = 1;
	}

	free(reply);
}

static void
x11_destroy(struct weston_backend *base)
{
	struct x11_backend *backend = container_of(base, struct x11_backend, base);
	struct weston_compositor *ec = backend->compositor;
	struct weston_head *head, *next;

	wl_event_source_remove(backend->xcb_source);
	x11_input_destroy(backend);

	weston_compositor_shutdown(ec); /* destroys outputs, too */

	wl_list_for_each_safe(head, next, &ec->head_list, compositor_link) {
		if (to_x11_head(head))
			x11_head_destroy(head);
	}

	XCloseDisplay(backend->dpy);
	free(backend->formats);
	free(backend);
}

static const struct weston_windowed_output_api api = {
	x11_output_set_size,
	x11_head_create,
};

static struct x11_backend *
x11_backend_create(struct weston_compositor *compositor,
		   struct weston_x11_backend_config *config)
{
	struct x11_backend *b;
	struct wl_event_loop *loop;
	int ret;

	b = zalloc(sizeof *b);
	if (b == NULL)
		return NULL;

	b->compositor = compositor;
	b->fullscreen = config->fullscreen;
	b->no_input = config->no_input;

	compositor->backend = &b->base;

	if (weston_compositor_set_presentation_clock_software(compositor) < 0)
		goto err_free;

	b->dpy = XOpenDisplay(NULL);
	if (b->dpy == NULL)
		goto err_free;

	b->conn = XGetXCBConnection(b->dpy);
	XSetEventQueueOwner(b->dpy, XCBOwnsEventQueue);

	if (xcb_connection_has_error(b->conn))
		goto err_xdisplay;

	b->screen = x11_compositor_get_default_screen(b);
	wl_array_init(&b->keys);

	x11_backend_get_resources(b);
	x11_backend_get_wm_info(b);

	if (!b->has_net_wm_state_fullscreen && config->fullscreen) {
		weston_log("Can not fullscreen without window manager support"
			   "(need _NET_WM_STATE_FULLSCREEN)\n");
		config->fullscreen = 0;
	}

	b->formats_count = ARRAY_LENGTH(x11_formats);
	b->formats = pixel_format_get_array(x11_formats, b->formats_count);

	if (config->renderer == WESTON_RENDERER_PIXMAN) {
		if (weston_compositor_init_renderer(compositor,
						    WESTON_RENDERER_PIXMAN,
						    NULL) < 0) {
			weston_log("Failed to initialize pixman renderer for X11 backend\n");
			goto err_xdisplay;
		}
	}
	else {
		const struct gl_renderer_display_options options = {
			.egl_platform = EGL_PLATFORM_X11_KHR,
			.egl_native_display = b->dpy,
			.egl_surface_type = EGL_WINDOW_BIT,
			.formats = b->formats,
			.formats_count = b->formats_count,
		};
		if (weston_compositor_init_renderer(compositor,
						    WESTON_RENDERER_GL,
						    &options.base) < 0)
			goto err_xdisplay;
	}

	b->base.destroy = x11_destroy;
	b->base.create_output = x11_output_create;

	if (x11_input_create(b, config->no_input) < 0) {
		weston_log("Failed to create X11 input\n");
		goto err_renderer;
	}

	loop = wl_display_get_event_loop(compositor->wl_display);
	b->xcb_source =
		wl_event_loop_add_fd(loop,
				     xcb_get_file_descriptor(b->conn),
				     WL_EVENT_READABLE,
				     x11_backend_handle_event, b);
	wl_event_source_check(b->xcb_source);

	if (compositor->renderer->import_dmabuf) {
		if (linux_dmabuf_setup(compositor) < 0)
			weston_log("Error: initializing dmabuf "
				   "support failed.\n");
	}

	if (compositor->capabilities & WESTON_CAP_EXPLICIT_SYNC) {
		if (linux_explicit_synchronization_setup(compositor) < 0)
			weston_log("Error: initializing explicit "
				   " synchronization support failed.\n");
	}

	ret = weston_plugin_api_register(compositor, WESTON_WINDOWED_OUTPUT_API_NAME,
					 &api, sizeof(api));

	if (ret < 0) {
		weston_log("Failed to register output API.\n");
		goto err_x11_input;
	}

	return b;

err_x11_input:
	x11_input_destroy(b);
err_renderer:
	compositor->renderer->destroy(compositor);
err_xdisplay:
	XCloseDisplay(b->dpy);
err_free:
	free(b->formats);
	free(b);
	return NULL;
}

static void
config_init_to_defaults(struct weston_x11_backend_config *config)
{
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct x11_backend *b;
	struct weston_x11_backend_config config = {{ 0, }};

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_X11_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_x11_backend_config)) {
		weston_log("X11 backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	b = x11_backend_create(compositor, &config);
	if (b == NULL)
		return -1;

	return 0;
}
