/*
 * Copyright 2020 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef WESTON_KIOSK_SHELL_H
#define WESTON_KIOSK_SHELL_H

#include <libweston/desktop.h>
#include <libweston/libweston.h>
#include <libweston/config-parser.h>

struct kiosk_shell {
	struct weston_compositor *compositor;
	struct weston_desktop *desktop;

	struct wl_listener destroy_listener;
	struct wl_listener output_created_listener;
	struct wl_listener output_resized_listener;
	struct wl_listener output_moved_listener;
	struct wl_listener seat_created_listener;
	struct wl_listener transform_listener;

	struct weston_layer background_layer;
	struct weston_layer normal_layer;
	struct weston_layer inactive_layer;

	struct wl_list output_list;
	struct wl_list seat_list;

	const struct weston_xwayland_surface_api *xwayland_surface_api;
	struct weston_config *config;
};

struct kiosk_shell_surface {
	struct weston_desktop_surface *desktop_surface;
	struct weston_view *view;

	struct kiosk_shell *shell;

	struct weston_output *output;
	struct wl_listener output_destroy_listener;

	struct wl_signal destroy_signal;
	struct wl_listener parent_destroy_listener;
	struct kiosk_shell_surface *parent;

	int focus_count;

	int32_t last_width, last_height;
	bool grabbed;

	struct {
		bool is_set;
		int32_t x;
		int32_t y;
	} xwayland;

	bool appid_output_assigned;
};

struct kiosk_shell_seat {
	struct weston_seat *seat;
	struct wl_listener seat_destroy_listener;
	struct weston_surface *focused_surface;

	struct wl_list link;	/** kiosk_shell::seat_list */
};

struct kiosk_shell_output {
	struct weston_output *output;
	struct wl_listener output_destroy_listener;
	struct weston_curtain *curtain;

	struct kiosk_shell *shell;
	struct wl_list link;

	char *app_ids;
};

#endif /* WESTON_KIOSK_SHELL_H */
