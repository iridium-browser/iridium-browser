/*
 * Copyright © 2012 Intel Corporation
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

#include <stdio.h>
#include <wayland-server.h>
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <xcb/composite.h>
#include <cairo/cairo-xcb.h>

#include <libweston/libweston.h>
#include <libweston/xwayland-api.h>
#include <libweston/weston-log.h>

#define SEND_EVENT_MASK (0x80)
#define EVENT_TYPE(event) ((event)->response_type & ~SEND_EVENT_MASK)

struct weston_xserver {
	struct wl_display *wl_display;
	struct wl_event_loop *loop;
	int abstract_fd;
	struct wl_event_source *abstract_source;
	int unix_fd;
	struct wl_event_source *unix_source;
	int display;
	pid_t pid;
	struct wl_client *client;
	struct weston_compositor *compositor;
	struct weston_wm *wm;
	struct wl_listener destroy_listener;
	weston_xwayland_spawn_xserver_func_t spawn_func;
	void *user_data;

	struct weston_log_scope *wm_debug;
};

struct weston_wm {
	xcb_connection_t *conn;
	const xcb_query_extension_reply_t *xfixes;
	struct wl_event_source *source;
	xcb_screen_t *screen;
	struct hash_table *window_hash;
	struct weston_xserver *server;
	xcb_window_t wm_window;
	struct weston_wm_window *focus_window;
	struct theme *theme;
	xcb_cursor_t *cursors;
	int last_cursor;
	xcb_render_pictforminfo_t format_rgb, format_rgba;
	xcb_visualid_t visual_id;
	xcb_colormap_t colormap;
	struct wl_listener create_surface_listener;
	struct wl_listener activate_listener;
	struct wl_listener kill_listener;
	struct wl_list unpaired_window_list;

	xcb_window_t selection_window;
	xcb_window_t selection_owner;
	int incr;
	int data_source_fd;
	struct wl_event_source *property_source;
	xcb_get_property_reply_t *property_reply;
	int property_start;
	struct wl_array source_data;
	xcb_selection_request_event_t selection_request;
	xcb_atom_t selection_target;
	xcb_timestamp_t selection_timestamp;
	int selection_property_set;
	int flush_property_on_delete;
	struct wl_listener selection_listener;

	xcb_window_t dnd_window;
	xcb_window_t dnd_owner;

	struct {
		xcb_atom_t		 wm_protocols;
		xcb_atom_t		 wm_normal_hints;
		xcb_atom_t		 wm_take_focus;
		xcb_atom_t		 wm_delete_window;
		xcb_atom_t		 wm_state;
		xcb_atom_t		 wm_s0;
		xcb_atom_t		 wm_client_machine;
		xcb_atom_t		 net_wm_cm_s0;
		xcb_atom_t		 net_wm_name;
		xcb_atom_t		 net_wm_pid;
		xcb_atom_t		 net_wm_icon;
		xcb_atom_t		 net_wm_state;
		xcb_atom_t		 net_wm_state_maximized_vert;
		xcb_atom_t		 net_wm_state_maximized_horz;
		xcb_atom_t		 net_wm_state_fullscreen;
		xcb_atom_t		 net_wm_user_time;
		xcb_atom_t		 net_wm_icon_name;
		xcb_atom_t		 net_wm_desktop;
		xcb_atom_t		 net_wm_window_type;
		xcb_atom_t		 net_wm_window_type_desktop;
		xcb_atom_t		 net_wm_window_type_dock;
		xcb_atom_t		 net_wm_window_type_toolbar;
		xcb_atom_t		 net_wm_window_type_menu;
		xcb_atom_t		 net_wm_window_type_utility;
		xcb_atom_t		 net_wm_window_type_splash;
		xcb_atom_t		 net_wm_window_type_dialog;
		xcb_atom_t		 net_wm_window_type_dropdown;
		xcb_atom_t		 net_wm_window_type_popup;
		xcb_atom_t		 net_wm_window_type_tooltip;
		xcb_atom_t		 net_wm_window_type_notification;
		xcb_atom_t		 net_wm_window_type_combo;
		xcb_atom_t		 net_wm_window_type_dnd;
		xcb_atom_t		 net_wm_window_type_normal;
		xcb_atom_t		 net_wm_moveresize;
		xcb_atom_t		 net_supporting_wm_check;
		xcb_atom_t		 net_supported;
		xcb_atom_t		 net_active_window;
		xcb_atom_t		 motif_wm_hints;
		xcb_atom_t		 clipboard;
		xcb_atom_t		 clipboard_manager;
		xcb_atom_t		 targets;
		xcb_atom_t		 utf8_string;
		xcb_atom_t		 wl_selection;
		xcb_atom_t		 incr;
		xcb_atom_t		 timestamp;
		xcb_atom_t		 multiple;
		xcb_atom_t		 compound_text;
		xcb_atom_t		 text;
		xcb_atom_t		 string;
		xcb_atom_t		 window;
		xcb_atom_t		 text_plain_utf8;
		xcb_atom_t		 text_plain;
		xcb_atom_t		 xdnd_selection;
		xcb_atom_t		 xdnd_aware;
		xcb_atom_t		 xdnd_enter;
		xcb_atom_t		 xdnd_leave;
		xcb_atom_t		 xdnd_drop;
		xcb_atom_t		 xdnd_status;
		xcb_atom_t		 xdnd_finished;
		xcb_atom_t		 xdnd_type_list;
		xcb_atom_t		 xdnd_action_copy;
		xcb_atom_t		 wl_surface_id;
		xcb_atom_t		 allow_commits;
	} atom;
};

void
dump_property(FILE *fp, struct weston_wm *wm, xcb_atom_t property,
	      xcb_get_property_reply_t *reply);

const char *
get_atom_name(xcb_connection_t *c, xcb_atom_t atom);

void
weston_wm_selection_init(struct weston_wm *wm);
int
weston_wm_handle_selection_event(struct weston_wm *wm,
				 xcb_generic_event_t *event);

struct weston_wm *
weston_wm_create(struct weston_xserver *wxs, int fd);
void
weston_wm_destroy(struct weston_wm *wm);

struct weston_seat *
weston_wm_pick_seat(struct weston_wm *wm);

int
weston_wm_handle_dnd_event(struct weston_wm *wm,
			   xcb_generic_event_t *event);
void
weston_wm_dnd_init(struct weston_wm *wm);
