/*
 * Copyright © 2019-2020 Stefan Agner <stefan@agner.ch>
 * Copyright © 2021-2022 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 * Copyright © 2022 Pengutronix, Rouven Czerwinski <r.czerwinski@pengutronix.de>
 * based on backend-rdp:
 * Copyright © 2013 Hardening <rdp.effort@gmail.com>
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#define AML_UNSTABLE_API 1
#include <aml.h>
#include <neatvnc.h>
#include <drm_fourcc.h>

#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "shared/timespec-util.h"
#include <libweston/libweston.h>
#include <libweston/backend-vnc.h>
#include <libweston/weston-log.h>
#include "pixel-formats.h"
#include "pixman-renderer.h"

#define DEFAULT_AXIS_STEP_DISTANCE 10

struct vnc_output;

struct vnc_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;
	struct weston_log_scope *debug;
	struct vnc_output *output;

	struct xkb_rule_names xkb_rule_name;
	struct xkb_keymap *xkb_keymap;

	struct aml *aml;
	struct wl_event_source *aml_event;
	struct nvnc *server;
	int vnc_monitor_refresh_rate;
};

struct vnc_output {
	struct weston_output base;
	struct weston_plane cursor_plane;
	struct weston_surface *cursor_surface;
	struct vnc_backend *backend;
	struct wl_event_source *finish_frame_timer;
	struct nvnc_display *display;

	struct nvnc_fb_pool *fb_pool;

	struct wl_list peers;
};

struct vnc_peer {
	struct vnc_backend *backend;
	struct weston_seat *seat;
	struct nvnc_client *client;

	enum nvnc_button_mask last_button_mask;
	struct wl_list link;
};

struct vnc_head {
	struct weston_head base;
};

static void
vnc_output_destroy(struct weston_output *base);

static inline struct vnc_output *
to_vnc_output(struct weston_output *base)
{
	if (base->destroy != vnc_output_destroy)
		return NULL;
	return container_of(base, struct vnc_output, base);
}

static void
vnc_head_destroy(struct weston_head *base);

static void
vnc_destroy(struct weston_backend *backend);

static inline struct vnc_head *
to_vnc_head(struct weston_head *base)
{
	if (base->backend->destroy != vnc_destroy)
		return NULL;
	return container_of(base, struct vnc_head, base);
}

struct vnc_keysym_to_keycode {
	const uint32_t keysym;
	const uint32_t code;
	const bool shift;
};

static const
struct vnc_keysym_to_keycode key_translation[] = {
	{XKB_KEY_KP_Enter,	0x60,	false	},
	{XKB_KEY_Return,	0x1c,	false	},
	{XKB_KEY_space,		0x39,	false	},
	{XKB_KEY_BackSpace,	0xe,	false	},
	{XKB_KEY_Tab,		0xf,	false	},
	{XKB_KEY_Escape,	0x1,	false	},
	{XKB_KEY_Shift_L,	0x2a,	false	},
	{XKB_KEY_Shift_R,	0x36,	false	},
	{XKB_KEY_Control_L,	0x1d,	false	},
	{XKB_KEY_Control_R,	0x9d,	false	},
	{XKB_KEY_Alt_L,		0x38,	false	},
	{XKB_KEY_Alt_R,		0x64,	false	},
	{XKB_KEY_Meta_L,	0x38,	false	},
	{XKB_KEY_Meta_R,	0x64,	false	},
	{XKB_KEY_Super_L,	0x7d,	false	},
	{XKB_KEY_Print, 	0x63,	false	},
	{XKB_KEY_Pause, 	0x77,	false	},
	{XKB_KEY_Caps_Lock, 	0x3a,	false	},
	{XKB_KEY_Scroll_Lock, 	0x46,	false	},
	{XKB_KEY_A,		0x1e,	true	},
	{XKB_KEY_a,		0x1e,	false	},
	{XKB_KEY_B,		0x30,	true	},
	{XKB_KEY_b,		0x30,	false	},
	{XKB_KEY_C,		0x2e,	true	},
	{XKB_KEY_c,		0x2e,	false	},
	{XKB_KEY_D,		0x20,	true	},
	{XKB_KEY_d,		0x20,	false	},
	{XKB_KEY_E,		0x12,	true	},
	{XKB_KEY_e,		0x12,	false	},
	{XKB_KEY_F,		0x21,	true	},
	{XKB_KEY_f,		0x21,	false	},
	{XKB_KEY_G,		0x22,	true	},
	{XKB_KEY_g,		0x22,	false	},
	{XKB_KEY_H,		0x23,	true	},
	{XKB_KEY_h,		0x23,	false	},
	{XKB_KEY_I,		0x17,	true	},
	{XKB_KEY_i,		0x17,	false	},
	{XKB_KEY_J,		0x24,	true	},
	{XKB_KEY_j,		0x24,	false	},
	{XKB_KEY_K,		0x25,	true	},
	{XKB_KEY_k,		0x25,	false	},
	{XKB_KEY_L,		0x26,	true	},
	{XKB_KEY_l,		0x26,	false	},
	{XKB_KEY_M,		0x32,	true	},
	{XKB_KEY_m,		0x32,	false	},
	{XKB_KEY_N,		0x31,	true	},
	{XKB_KEY_n,		0x31,	false	},
	{XKB_KEY_O,		0x18,	true	},
	{XKB_KEY_o,		0x18,	false	},
	{XKB_KEY_P,		0x19,	true	},
	{XKB_KEY_p,		0x19,	false	},
	{XKB_KEY_Q,		0x10,	true	},
	{XKB_KEY_q,		0x10,	false	},
	{XKB_KEY_R,		0x13,	true	},
	{XKB_KEY_r,		0x13,	false	},
	{XKB_KEY_S,		0x1f,	true	},
	{XKB_KEY_s,		0x1f,	false	},
	{XKB_KEY_T,		0x14,	true	},
	{XKB_KEY_t,		0x14,	false	},
	{XKB_KEY_U,		0x16,	true	},
	{XKB_KEY_u,		0x16,	false	},
	{XKB_KEY_V,		0x2f,	true	},
	{XKB_KEY_v,		0x2f,	false	},
	{XKB_KEY_W,		0x11,	true	},
	{XKB_KEY_w,		0x11,	false	},
	{XKB_KEY_X,		0x2d,	true	},
	{XKB_KEY_x,		0x2d,	false	},
	{XKB_KEY_Y,		0x15,	true	},
	{XKB_KEY_y,		0x15,	false	},
	{XKB_KEY_Z,		0x2c,	true	},
	{XKB_KEY_z,		0x2c,	false	},
	{XKB_KEY_grave,		0x29,	false	},
	{XKB_KEY_asciitilde,	0x29,	true	},
	{XKB_KEY_1,		0x02,	false	},
	{XKB_KEY_exclam,	0x02,	true	},
	{XKB_KEY_2,		0x03,	false	},
	{XKB_KEY_at,		0x03,	true	},
	{XKB_KEY_3,		0x04,	false	},
	{XKB_KEY_numbersign,	0x04,	true	},
	{XKB_KEY_4,		0x05,	false	},
	{XKB_KEY_dollar,	0x05,	true	},
	{XKB_KEY_5,		0x06,	false	},
	{XKB_KEY_percent,	0x06,	true	},
	{XKB_KEY_6,		0x07,	false	},
	{XKB_KEY_asciicircum,	0x07,	true	},
	{XKB_KEY_7,		0x08,	false	},
	{XKB_KEY_ampersand,	0x08,	true	},
	{XKB_KEY_8,		0x09,	false	},
	{XKB_KEY_asterisk,	0x09,	true	},
	{XKB_KEY_9,		0x0a,	false	},
	{XKB_KEY_parenleft,	0x0a,	true	},
	{XKB_KEY_0,		0x0b,	false	},
	{XKB_KEY_parenright,	0x0b,	true	},
	{XKB_KEY_minus,		0x0c,	false,	},
	{XKB_KEY_underscore,	0x0c,	true	},
	{XKB_KEY_equal,		0x0d,	false	},
	{XKB_KEY_plus,		0x0d,	true	},
	{XKB_KEY_bracketleft,	0x1a,	false	},
	{XKB_KEY_braceleft,	0x1a,	true	},
	{XKB_KEY_bracketright,	0x1b,	false	},
	{XKB_KEY_braceright,	0x1b,	true	},
	{XKB_KEY_semicolon,	0x27,	false	},
	{XKB_KEY_colon,		0x27,	true	},
	{XKB_KEY_apostrophe,	0x28,	false	},
	{XKB_KEY_quotedbl,	0x28,	true	},
	{XKB_KEY_backslash,	0x2b,	false	},
	{XKB_KEY_bar,		0x2b,	true	},
	{XKB_KEY_comma,		0x33,	false	},
	{XKB_KEY_less,		0x33,	true	},
	{XKB_KEY_period,	0x34,	false	},
	{XKB_KEY_greater,	0x34,	true	},
	{XKB_KEY_slash,		0x35,	false	},
	{XKB_KEY_question,	0x35,	true	},
	{XKB_KEY_F1,		0x3b,	false	},
	{XKB_KEY_F2,		0x3c,   false	},
	{XKB_KEY_F3,		0x3d,   false	},
	{XKB_KEY_F4,		0x3e,   false	},
	{XKB_KEY_F5,		0x3f,   false	},
	{XKB_KEY_F6,		0x40,   false	},
	{XKB_KEY_F7,		0x41,   false	},
	{XKB_KEY_F8,		0x42,   false	},
	{XKB_KEY_F9,		0x43,   false	},
	{XKB_KEY_F10,		0x44,   false	},
	{XKB_KEY_F11,		0x57,   false	},
	{XKB_KEY_F12,		0x58,   false	},
	{XKB_KEY_Home,		0x66,   false	},
	{XKB_KEY_Up,		0x67,   false	},
	{XKB_KEY_Prior,		0x68,   false	},
	{XKB_KEY_Left,		0x69,   false	},
	{XKB_KEY_Right,		0x6a,   false	},
	{XKB_KEY_End,		0x6b,   false	},
	{XKB_KEY_Down,		0x6c,   false	},
	{XKB_KEY_Next,		0x6d,   false	},
	{ },
};

static void
vnc_handle_key_event(struct nvnc_client *client, uint32_t keysym,
		     bool is_pressed)
{
	struct vnc_peer *peer = nvnc_get_userdata(client);
	uint32_t key = 0;
	bool needs_shift = false;
	enum weston_key_state_update state_update;
	enum wl_keyboard_key_state state;
	struct timespec time;
	int i;

	weston_compositor_get_time(&time);

	if (is_pressed)
		state = WL_KEYBOARD_KEY_STATE_PRESSED;
	else
		state = WL_KEYBOARD_KEY_STATE_RELEASED;

	/* Generally ignore shift state as per RFC6143 Section 7.5.4 */
	if (keysym == XKB_KEY_Shift_L || keysym == XKB_KEY_Shift_R)
		return;

	/* Allow selected modifiers */
	if (keysym == XKB_KEY_Control_L || keysym == XKB_KEY_Control_R ||
	    keysym == XKB_KEY_Alt_L || keysym == XKB_KEY_Alt_R)
		state_update = STATE_UPDATE_AUTOMATIC;
	else
		state_update = STATE_UPDATE_NONE;

	for (i = 0; key_translation[i].keysym; i++) {
		if (key_translation[i].keysym == keysym) {
			key = key_translation[i].code;
			needs_shift = key_translation[i].shift;
			break;
		}
	}

	if (!key) {
		weston_log("Key not found: keysym %08x, translated %08x\n",
			    keysym, key);
		return;
	}

	/* emulate lshift press */
	if (needs_shift)
		notify_key(peer->seat, &time, KEY_LEFTSHIFT,
			   WL_KEYBOARD_KEY_STATE_PRESSED,
			   STATE_UPDATE_AUTOMATIC);

	/* send detected key code */
	notify_key(peer->seat, &time, key, state, state_update);

	/* emulate lshift release */
	if (needs_shift)
		notify_key(peer->seat, &time, KEY_LEFTSHIFT,
			   WL_KEYBOARD_KEY_STATE_RELEASED,
			   STATE_UPDATE_AUTOMATIC);
}

static void
vnc_handle_key_code_event(struct nvnc_client *client, uint32_t key,
			  bool is_pressed)
{
	struct vnc_peer *peer = nvnc_get_userdata(client);
	enum wl_keyboard_key_state state;
	struct timespec time;

	weston_compositor_get_time(&time);

	if (is_pressed)
		state = WL_KEYBOARD_KEY_STATE_PRESSED;
	else
		state = WL_KEYBOARD_KEY_STATE_RELEASED;

	notify_key(peer->seat, &time, key, state, STATE_UPDATE_AUTOMATIC);
}

static void
vnc_pointer_event(struct nvnc_client *client, uint16_t x, uint16_t y,
		  enum nvnc_button_mask button_mask)
{
	struct vnc_peer *peer = nvnc_get_userdata(client);
	struct vnc_output *output = peer->backend->output;
	struct timespec time;
	enum nvnc_button_mask changed_button_mask;

	weston_compositor_get_time(&time);

	if (x < output->base.width && y < output->base.height) {
		struct weston_coord_global pos;

		pos = weston_coord_global_from_output_point(x, y, &output->base);
		notify_motion_absolute(peer->seat, &time, pos);
	}

	changed_button_mask = peer->last_button_mask ^ button_mask;

	if (changed_button_mask & NVNC_BUTTON_LEFT)
		notify_button(peer->seat, &time, BTN_LEFT,
			      (button_mask & NVNC_BUTTON_LEFT) ?
			      WL_POINTER_BUTTON_STATE_PRESSED :
			      WL_POINTER_BUTTON_STATE_RELEASED);

	if (changed_button_mask & NVNC_BUTTON_MIDDLE)
		notify_button(peer->seat, &time, BTN_MIDDLE,
			      (button_mask & NVNC_BUTTON_MIDDLE) ?
			      WL_POINTER_BUTTON_STATE_PRESSED :
			      WL_POINTER_BUTTON_STATE_RELEASED);

	if (changed_button_mask & NVNC_BUTTON_RIGHT)
		notify_button(peer->seat, &time, BTN_RIGHT,
			      (button_mask & NVNC_BUTTON_RIGHT) ?
			      WL_POINTER_BUTTON_STATE_PRESSED :
			      WL_POINTER_BUTTON_STATE_RELEASED);

	if ((button_mask & NVNC_SCROLL_UP) ||
	    (button_mask & NVNC_SCROLL_DOWN)) {
		struct weston_pointer_axis_event weston_event;

		weston_event.axis = WL_POINTER_AXIS_VERTICAL_SCROLL;

		/* DEFAULT_AXIS_STEP_DISTANCE is stolen from compositor-x11.c */
		if (button_mask & NVNC_SCROLL_UP)
			weston_event.value = -DEFAULT_AXIS_STEP_DISTANCE;
		if (button_mask & NVNC_SCROLL_DOWN)
			weston_event.value = DEFAULT_AXIS_STEP_DISTANCE;
		weston_event.has_discrete = false;

		notify_axis(peer->seat, &time, &weston_event);
	}

	peer->last_button_mask = button_mask;

	notify_pointer_frame(peer->seat);
}

static bool
vnc_handle_auth(const char *username, const char *password, void *userdata)
{
	struct passwd *pw = getpwnam(username);

	if (!pw || pw->pw_uid != getuid()) {
		weston_log("VNC: wrong user '%s'\n", username);
		return false;
	}

	return weston_authenticate_user(username, password);
}

static void
vnc_client_cleanup(struct nvnc_client *client)
{
	struct vnc_peer *peer = nvnc_get_userdata(client);
	struct vnc_output *output = peer->backend->output;

	wl_list_remove(&peer->link);
	weston_seat_release_keyboard(peer->seat);
	weston_seat_release_pointer(peer->seat);
	weston_seat_release(peer->seat);
	free(peer);
	weston_log("VNC Client disconnected\n");

	if (wl_list_empty(&output->peers))
		weston_output_power_off(&output->base);
}

static struct weston_pointer *
vnc_output_get_pointer(struct vnc_output *output)
{
	struct weston_pointer *pointer = NULL;
	struct weston_paint_node *pnode;
	struct vnc_peer *peer;

	wl_list_for_each(peer, &output->peers, link) {
		pointer = weston_seat_get_pointer(peer->seat);
		break;
	}

	if (!pointer)
		return NULL;

	wl_list_for_each(pnode, &output->base.paint_node_z_order_list, z_order_link) {
		if (pnode->view == pointer->sprite)
			return pointer;
	}

	return NULL;
}

static void
vnc_output_update_cursor(struct vnc_output *output)
{
	struct vnc_backend *backend = output->backend;
	struct weston_pointer *pointer;
	struct weston_view *view;
	struct weston_buffer *buffer;
	struct nvnc_fb *fb;
	int32_t stride;
	uint32_t format;
	uint8_t *src, *dst;
	int i;

	pointer = vnc_output_get_pointer(output);
	if (!pointer)
		return;

	view = pointer->sprite;
	if (!weston_view_has_valid_buffer(view))
		return;

	buffer = view->surface->buffer_ref.buffer;
	if (buffer->type != WESTON_BUFFER_SHM)
		return;

	format = wl_shm_buffer_get_format(buffer->shm_buffer);
	if (format != WL_SHM_FORMAT_ARGB8888)
		return;

	weston_view_move_to_plane(view, &output->cursor_plane);

	if (view->surface == output->cursor_surface &&
	    !pixman_region32_not_empty(&view->surface->damage))
		return;

	output->cursor_surface = view->surface;

	stride = wl_shm_buffer_get_stride(buffer->shm_buffer);

	fb = nvnc_fb_new(buffer->width, buffer->height, DRM_FORMAT_ARGB8888,
			 buffer->width);
	assert(fb);

	src = wl_shm_buffer_get_data(buffer->shm_buffer);
	dst = nvnc_fb_get_addr(fb);

	wl_shm_buffer_begin_access(buffer->shm_buffer);
	for (i = 0; i < buffer->height; i++)
		memcpy(dst + i * 4 * buffer->width, src + i * stride,
		       4 * buffer->width);
	wl_shm_buffer_end_access(buffer->shm_buffer);

	nvnc_set_cursor(backend->server, fb, buffer->width, buffer->height,
			pointer->hotspot.c.x, pointer->hotspot.c.y, true);
	nvnc_fb_unref(fb);
}

/*
 * Convert damage rectangles from 32-bit global coordinates to 16-bit local
 * coordinates. The output transformation has to be a pure translation.
 */
static void
vnc_region_global_to_output(pixman_region16_t *dst,
			    struct weston_output *output,
			    pixman_region32_t *src)
{
	struct pixman_box32 *src_rects;
	struct pixman_box16 *dest_rects;
	int n_rects = 0;
	int i;

	src_rects = pixman_region32_rectangles(src, &n_rects);
	if (!n_rects)
		return;

	dest_rects = xcalloc(n_rects, sizeof(*dest_rects));

	for (i = 0; i < n_rects; i++) {
		dest_rects[i].x1 = src_rects[i].x1 - output->x;
		dest_rects[i].y1 = src_rects[i].y1 - output->y;
		dest_rects[i].x2 = src_rects[i].x2 - output->x;
		dest_rects[i].y2 = src_rects[i].y2 - output->y;
	}

	pixman_region_init_rects(dst, dest_rects, n_rects);
	free(dest_rects);
}

static void
vnc_log_scope_print_region(struct weston_log_scope *log, pixman_region32_t *region)
{
	struct pixman_box32 *rects;
	int n_rects = 0;
	int i;

	rects = pixman_region32_rectangles(region, &n_rects);
	if (!n_rects) {
		weston_log_scope_printf(log, " empty");
		return;
	}

	for (i = 0; i < n_rects; i++) {
		int width = rects[i].x2 - rects[i].x1;
		int height = rects[i].y2 - rects[i].y1;

		weston_log_scope_printf(log, " %dx%d(%d,%d)", width, height,
					rects[i].x1, rects[i].y1);
	}
}

static void
vnc_log_damage(struct vnc_backend *backend, pixman_region32_t *buffer_damage,
	       pixman_region32_t *update_damage)
{
	char timestr[128];

	if (!weston_log_scope_is_enabled(backend->debug))
		return;

	weston_log_scope_timestamp(backend->debug, timestr, sizeof timestr);

	weston_log_scope_printf(backend->debug, "%s buffer damage:", timestr);
	vnc_log_scope_print_region(backend->debug, buffer_damage);
	weston_log_scope_printf(backend->debug, "\n");

	weston_log_scope_printf(backend->debug, "%s update damage:", timestr);
	vnc_log_scope_print_region(backend->debug, update_damage);
	weston_log_scope_printf(backend->debug, "\n\n");
}

static void
vnc_update_buffer(struct nvnc_display *display, struct pixman_region32 *damage)
{
	struct nvnc *server = nvnc_display_get_server(display);
	struct vnc_backend *backend = nvnc_get_userdata(server);
	struct vnc_output *output = backend->output;
	struct weston_compositor *ec = output->base.compositor;
	struct weston_renderbuffer *renderbuffer;
	pixman_region16_t local_damage;
	struct nvnc_fb *fb;

	fb = nvnc_fb_pool_acquire(output->fb_pool);
	assert(fb);

	renderbuffer = nvnc_get_userdata(fb);
	if (!renderbuffer) {
		const struct pixman_renderer_interface *pixman;
		const struct pixel_format_info *pfmt;

		pixman = ec->renderer->pixman;
		pfmt = pixel_format_get_info(DRM_FORMAT_XRGB8888);

		renderbuffer =
			pixman->create_image_from_ptr(&output->base, pfmt,
						      output->base.width,
						      output->base.height,
						      nvnc_fb_get_addr(fb),
						      output->base.width * 4);

		/* This is a new buffer, so the whole surface is damaged. */
		pixman_region32_copy(&renderbuffer->damage,
				     &output->base.region);

		nvnc_set_userdata(fb, renderbuffer,
				  (nvnc_cleanup_fn)weston_renderbuffer_unref);
	}

	vnc_log_damage(backend, &renderbuffer->damage, damage);

	ec->renderer->repaint_output(&output->base, damage, renderbuffer);

	/* Convert to local coordinates */
	pixman_region_init(&local_damage);
	vnc_region_global_to_output(&local_damage, &output->base, damage);

	nvnc_display_feed_buffer(output->display, fb, &local_damage);
	nvnc_fb_unref(fb);
	pixman_region_fini(&local_damage);
}

static void
vnc_new_client(struct nvnc_client *client)
{
	struct nvnc *server = nvnc_client_get_server(client);
	struct vnc_backend *backend = nvnc_get_userdata(server);
	struct vnc_output *output = backend->output;
	struct vnc_peer *peer;
	const char *seat_name = "VNC Client";

	weston_log("New VNC client connected\n");

	peer = xzalloc(sizeof(*peer));
	peer->client = client;
	peer->backend = backend;
	peer->seat = xzalloc(sizeof(*peer->seat));

	weston_seat_init(peer->seat, backend->compositor, seat_name);
	weston_seat_init_pointer(peer->seat);
	weston_seat_init_keyboard(peer->seat, backend->xkb_keymap);

	if (wl_list_empty(&output->peers))
		weston_output_power_on(&output->base);

	wl_list_insert(&output->peers, &peer->link);

	nvnc_set_userdata(client, peer, NULL);
	nvnc_set_client_cleanup_fn(client, vnc_client_cleanup);

	/*
	 * Make up for repaints that were skipped when no clients were
	 * connected.
	 */
	weston_output_schedule_repaint(&output->base);
}


static int
finish_frame_handler(void *data)
{
	struct vnc_output *output = data;
	int refresh_nsec = millihz_to_nsec(output->base.current_mode->refresh);
	struct timespec now, ts;
	int delta;

	/* The timer only has msec precision, but if we approximately hit our
	 * target, report an exact time stamp by adding to the previous frame
	 * time.
	 */
	timespec_add_nsec(&ts, &output->base.frame_time, refresh_nsec);

	/* If we are more than 1.5 ms late, report the current time instead. */
	weston_compositor_read_presentation_clock(output->base.compositor, &now);
	delta = (int)timespec_sub_to_nsec(&now, &ts);
	if (delta > 1500000)
		ts = now;

	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static int
vnc_output_enable(struct weston_output *base)
{
	struct weston_renderer *renderer = base->compositor->renderer;
	struct vnc_output *output = to_vnc_output(base);
	struct vnc_backend *backend;
	struct wl_event_loop *loop;
	const struct pixman_renderer_output_options options = {
		.fb_size = {
			.width = output->base.width,
			.height = output->base.height,
		},
		.format = pixel_format_get_info(DRM_FORMAT_XRGB8888),
	};

	assert(output);

	backend = output->backend;
	backend->output = output;

	weston_plane_init(&output->cursor_plane, backend->compositor);

	if (renderer->pixman->output_create(&output->base, &options) < 0)
		return -1;

	loop = wl_display_get_event_loop(backend->compositor->wl_display);
	output->finish_frame_timer = wl_event_loop_add_timer(loop,
							     finish_frame_handler,
							     output);

	output->fb_pool = nvnc_fb_pool_new(output->base.width,
					   output->base.height,
					   options.format->format,
					   output->base.width);

	output->display = nvnc_display_new(0, 0);

	nvnc_add_display(backend->server, output->display);

	return 0;
}

static int
vnc_output_disable(struct weston_output *base)
{
	struct weston_renderer *renderer = base->compositor->renderer;
	struct vnc_output *output = to_vnc_output(base);
	struct vnc_backend *backend;

	assert(output);

	backend = output->backend;

	if (!output->base.enabled)
		return 0;

	nvnc_display_unref(output->display);
	nvnc_fb_pool_unref(output->fb_pool);

	renderer->pixman->output_destroy(&output->base);

	wl_event_source_remove(output->finish_frame_timer);
	backend->output = NULL;

	weston_plane_release(&output->cursor_plane);

	return 0;
}

static void
vnc_output_destroy(struct weston_output *base)
{
	struct vnc_output *output = to_vnc_output(base);

	/* Can only be called on outputs created by vnc_create_output() */
	assert(output);

	vnc_output_disable(&output->base);
	weston_output_release(&output->base);

	free(output);
}

static struct weston_output *
vnc_create_output(struct weston_backend *backend, const char *name)
{
	struct vnc_backend *b = container_of(backend, struct vnc_backend, base);
	struct vnc_output *output;

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	weston_output_init(&output->base, b->compositor, name);

	output->base.destroy = vnc_output_destroy;
	output->base.disable = vnc_output_disable;
	output->base.enable = vnc_output_enable;
	output->base.attach_head = NULL;

	output->backend = b;

	weston_compositor_add_pending_output(&output->base, b->compositor);

	return &output->base;
}

static void
vnc_destroy(struct weston_backend *base)
{
	struct vnc_backend *backend = container_of(base, struct vnc_backend, base);
	struct weston_compositor *ec = backend->compositor;
	struct weston_head *head, *next;

	nvnc_close(backend->server);

	weston_compositor_shutdown(ec);

	wl_event_source_remove(backend->aml_event);

	aml_unref(backend->aml);

	wl_list_for_each_safe(head, next, &ec->head_list, compositor_link)
		vnc_head_destroy(head);

	xkb_keymap_unref(backend->xkb_keymap);

	if (backend->debug)
		weston_log_scope_destroy(backend->debug);

	free(backend);
}

static void
vnc_head_create(struct vnc_backend *backend, const char *name)
{
	struct vnc_head *head;

	head = xzalloc(sizeof *head);

	weston_head_init(&head->base, name);
	weston_head_set_monitor_strings(&head->base, "weston", "vnc", NULL);
	weston_head_set_physical_size(&head->base, 0, 0);

	head->base.backend = &backend->base;

	weston_head_set_connection_status(&head->base, true);
	weston_compositor_add_head(backend->compositor, &head->base);
}

static void
vnc_head_destroy(struct weston_head *base)
{
	struct vnc_head *head = to_vnc_head(base);

	if (!head)
		return;

	weston_head_release(&head->base);
	free(head);
}

static int
vnc_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
vnc_output_repaint(struct weston_output *base, pixman_region32_t *damage)
{
	struct vnc_output *output = to_vnc_output(base);
	struct weston_compositor *ec = output->base.compositor;
	struct vnc_backend *backend = output->backend;
	struct timespec now, target;
	int refresh_nsec = millihz_to_nsec(output->base.current_mode->refresh);
	int refresh_msec = refresh_nsec / 1000000;
	int next_frame_delta;

	assert(output);

	if (wl_list_empty(&output->peers))
		weston_output_power_off(base);

	if (pixman_region32_not_empty(damage)) {
		vnc_update_buffer(output->display, damage);
		pixman_region32_subtract(&ec->primary_plane.damage,
					 &ec->primary_plane.damage, damage);
	}

	/*
	 * Make sure damage of this (or previous) damage is handled
	 *
	 * This will usually invoke the render callback where the (pixman)
	 * renderer gets invoked
	 */
	aml_dispatch(backend->aml);

	weston_compositor_read_presentation_clock(ec, &now);
	timespec_add_nsec(&target, &output->base.frame_time, refresh_nsec);

	next_frame_delta = (int)timespec_sub_to_msec(&target, &now);
	if (next_frame_delta < 1)
		next_frame_delta = 1;
	if (next_frame_delta > refresh_msec)
		next_frame_delta = refresh_msec;

	wl_event_source_timer_update(output->finish_frame_timer,
				     next_frame_delta);

	return 0;
}

static bool
vnc_clients_support_cursor(struct vnc_output *output)
{
	struct vnc_peer *peer;

	wl_list_for_each(peer, &output->peers, link) {
		if (!nvnc_client_supports_cursor(peer->client))
			return false;
	}

	return true;
}

static void
vnc_output_assign_planes(struct weston_output *base)
{
	struct vnc_output *output = to_vnc_output(base);

	assert(output);

	if (wl_list_empty(&output->peers))
		return;

	/* Update VNC cursor and move cursor view to plane */
	if (vnc_clients_support_cursor(output))
		vnc_output_update_cursor(output);
}

static struct weston_mode *
vnc_insert_new_mode(struct weston_output *output, int width, int height,
		    int rate)
{
	struct weston_mode *mode;

	mode = xzalloc(sizeof *mode);
	mode->width = width;
	mode->height = height;
	mode->refresh = rate;
	wl_list_insert(&output->mode_list, &mode->link);

	return mode;
}

static struct weston_mode *
vnc_ensure_matching_mode(struct vnc_output *output,
			 struct weston_mode *target)
{
	struct vnc_backend *backend = output->backend;
	struct weston_mode *local;

	wl_list_for_each(local, &output->base.mode_list, link) {
		if ((local->width == target->width) &&
		    (local->height == target->height))
			return local;
	}

	return vnc_insert_new_mode(&output->base, target->width, target->height,
				   backend->vnc_monitor_refresh_rate);
}

static int
vnc_switch_mode(struct weston_output *base, struct weston_mode *target_mode)
{
	struct vnc_output *output = to_vnc_output(base);
	struct weston_mode *local_mode;
	struct weston_size fb_size;

	assert(output);

	local_mode = vnc_ensure_matching_mode(output, target_mode);

	if (local_mode == base->current_mode)
		return 0;

	base->current_mode->flags &= ~WL_OUTPUT_MODE_CURRENT;

	base->current_mode = base->native_mode = local_mode;
	base->current_mode->flags |= WL_OUTPUT_MODE_CURRENT;

	fb_size.width = target_mode->width;
	fb_size.height = target_mode->height;

	weston_renderer_resize_output(base, &fb_size, NULL);

	nvnc_fb_pool_resize(output->fb_pool, target_mode->width,
			    target_mode->height, DRM_FORMAT_XRGB8888,
			    target_mode->width);

	return 0;
}

static int
vnc_output_set_size(struct weston_output *base, int width, int height)
{
	struct vnc_output *output = to_vnc_output(base);
	struct vnc_backend *backend = output->backend;
	struct weston_mode *current_mode;
	struct weston_mode init_mode;

	/* We can only be called once. */
	assert(!output->base.current_mode);

	wl_list_init(&output->peers);

	init_mode.width = width;
	init_mode.height = height;
	init_mode.refresh = backend->vnc_monitor_refresh_rate;

	current_mode = vnc_ensure_matching_mode(output, &init_mode);
	current_mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;

	output->base.current_mode = output->base.native_mode = current_mode;

	output->base.start_repaint_loop = vnc_output_start_repaint_loop;
	output->base.repaint = vnc_output_repaint;
	output->base.assign_planes = vnc_output_assign_planes;
	output->base.set_backlight = NULL;
	output->base.set_dpms = NULL;
	output->base.switch_mode = vnc_switch_mode;

	return 0;
}

static const struct weston_vnc_output_api api = {
	vnc_output_set_size,
};

static int
vnc_aml_dispatch(int fd, uint32_t mask, void *data)
{
	struct aml *aml = data;

	aml_poll(aml, 0);
	aml_dispatch(aml);

	return 0;
}

static struct vnc_backend *
vnc_backend_create(struct weston_compositor *compositor,
		   struct weston_vnc_backend_config *config)
{
	struct vnc_backend *backend;
	struct wl_event_loop *loop;
	struct weston_head *base, *next;
	int ret;
	int fd;

	backend = zalloc(sizeof *backend);
	if (backend == NULL)
		return NULL;

	backend->compositor = compositor;
	backend->base.destroy = vnc_destroy;
	backend->base.create_output = vnc_create_output;
	backend->vnc_monitor_refresh_rate = config->refresh_rate * 1000;

	backend->debug = weston_compositor_add_log_scope(compositor,
							 "vnc-backend",
							 "Debug messages from VNC backend\n",
							 NULL, NULL, NULL);

	compositor->backend = &backend->base;

	if (weston_compositor_set_presentation_clock_software(compositor) < 0)
		goto err_compositor;

	switch (config->renderer) {
	case WESTON_RENDERER_AUTO:
	case WESTON_RENDERER_PIXMAN:
		break;
	default:
		weston_log("Unsupported renderer requested\n");
		goto err_compositor;
	}

	if (weston_compositor_init_renderer(compositor, WESTON_RENDERER_PIXMAN,
					    NULL) < 0)
		goto err_compositor;

	vnc_head_create(backend, "vnc");

	compositor->capabilities |= WESTON_CAP_ARBITRARY_MODES;

	backend->xkb_rule_name.rules = strdup(compositor->xkb_names.rules);
	backend->xkb_rule_name.model = strdup(compositor->xkb_names.model);
	backend->xkb_rule_name.layout = strdup(compositor->xkb_names.layout);

	backend->xkb_keymap = xkb_keymap_new_from_names(
					backend->compositor->xkb_context,
					&backend->xkb_rule_name, 0);

	loop = wl_display_get_event_loop(backend->compositor->wl_display);

	backend->aml = aml_new();
	if (!backend->aml)
		goto err_output;
	aml_set_default(backend->aml);

	fd = aml_get_fd(backend->aml);

	backend->aml_event = wl_event_loop_add_fd(loop, fd, WL_EVENT_READABLE,
						  vnc_aml_dispatch,
						  backend->aml);

	backend->server = nvnc_open(config->bind_address, config->port);
	if (!backend->server)
		goto err_output;

	nvnc_set_new_client_fn(backend->server, vnc_new_client);
	nvnc_set_pointer_fn(backend->server, vnc_pointer_event);
	nvnc_set_key_fn(backend->server, vnc_handle_key_event);
	nvnc_set_key_code_fn(backend->server, vnc_handle_key_code_event);
	nvnc_set_userdata(backend->server, backend, NULL);
	nvnc_set_name(backend->server, "Weston VNC backend");

	if (!nvnc_has_auth()) {
		weston_log("Neat VNC built without TLS support\n");
		goto err_output;
	}
	if (!config->server_cert && !config->server_key) {
		weston_log("The VNC backend requires a key and a certificate for TLS security"
			   " (--vnc-tls-cert/--vnc-tls-key)\n");
		goto err_output;
	}
	if (!config->server_cert) {
		weston_log("Missing TLS certificate (--vnc-tls-cert)\n");
		goto err_output;
	}
	if (!config->server_key) {
		weston_log("Missing TLS key (--vnc-tls-key)\n");
		goto err_output;
	}

	ret = nvnc_enable_auth(backend->server, config->server_key,
			       config->server_cert, vnc_handle_auth,
			       NULL);
	if (ret) {
		weston_log("Failed to enable TLS support\n");
		goto err_output;
	}

	weston_log("TLS support activated\n");

	ret = weston_plugin_api_register(compositor, WESTON_VNC_OUTPUT_API_NAME,
					 &api, sizeof(api));
	if (ret < 0) {
		weston_log("Failed to register output API.\n");
		goto err_output;
	}

	return backend;

err_output:
	wl_list_for_each_safe(base, next, &compositor->head_list, compositor_link)
		vnc_head_destroy(base);
err_compositor:
	weston_compositor_shutdown(compositor);
	free(backend);
	return NULL;
}

static void
config_init_to_defaults(struct weston_vnc_backend_config *config)
{
	config->bind_address = NULL;
	config->port = 5900;
	config->refresh_rate = VNC_DEFAULT_FREQ;
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct vnc_backend *backend;
	struct weston_vnc_backend_config config = {{ 0, }};

	weston_log("Initializing VNC backend\n");

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_VNC_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_vnc_backend_config)) {
		weston_log("VNC backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	backend = vnc_backend_create(compositor, &config);
	if (backend == NULL)
		return -1;
	return 0;
}
