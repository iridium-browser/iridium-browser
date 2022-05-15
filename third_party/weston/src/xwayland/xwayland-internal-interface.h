/*
 * Copyright © 2016 Quentin "Sardem FF7" Glidic
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

#ifndef XWAYLAND_INTERNAL_INTERFACE_H
#define XWAYLAND_INTERNAL_INTERFACE_H

struct weston_desktop_xwayland;
struct weston_desktop_xwayland_surface;

struct weston_xwayland_client_interface {
	void (*send_configure)(struct weston_surface *surface, int32_t width, int32_t height);
};

struct weston_desktop_xwayland_interface {
	struct weston_desktop_xwayland_surface *(*create_surface)(struct weston_desktop_xwayland *xwayland,
						      struct weston_surface *surface,
						      const struct weston_xwayland_client_interface *client);
	void (*set_toplevel)(struct weston_desktop_xwayland_surface *shsurf);
	void (*set_toplevel_with_position)(struct weston_desktop_xwayland_surface *shsurf,
					   int32_t x, int32_t y);
	void (*set_parent)(struct weston_desktop_xwayland_surface *shsurf,
			   struct weston_surface *parent);
	void (*set_transient)(struct weston_desktop_xwayland_surface *shsurf,
			      struct weston_surface *parent, int x, int y);
	void (*set_fullscreen)(struct weston_desktop_xwayland_surface *shsurf,
			       struct weston_output *output);
	void (*set_xwayland)(struct weston_desktop_xwayland_surface *shsurf,
			     int x, int y);
	int (*move)(struct weston_desktop_xwayland_surface *shsurf,
		    struct weston_pointer *pointer);
	int (*resize)(struct weston_desktop_xwayland_surface *shsurf,
		      struct weston_pointer *pointer, uint32_t edges);
	void (*set_title)(struct weston_desktop_xwayland_surface *shsurf,
	                  const char *title);
	void (*set_window_geometry)(struct weston_desktop_xwayland_surface *shsurf,
				    int32_t x, int32_t y,
				    int32_t width, int32_t height);
	void (*set_maximized)(struct weston_desktop_xwayland_surface *shsurf);
	void (*set_pid)(struct weston_desktop_xwayland_surface *shsurf, pid_t pid);
};

#endif
