/*
 * Copyright (C) 2013 DENSO CORPORATION
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

#ifndef WESTON_IVI_SHELL_H
#define WESTON_IVI_SHELL_H

#include <stdbool.h>
#include <stdint.h>

#include <libweston/libweston.h>
#include <libweston-desktop/libweston-desktop.h>

struct ivi_shell
{
	struct wl_listener destroy_listener;
	struct wl_listener wake_listener;

	struct weston_compositor *compositor;

	struct weston_desktop *desktop;
	struct wl_list ivi_surface_list; /* struct ivi_shell_surface::link */
};

void
shell_surface_send_configure(struct weston_surface *surface,
			     int32_t width, int32_t height);

struct ivi_layout_surface;

struct ivi_layout_surface *
shell_get_ivi_layout_surface(struct weston_surface *surface);

#endif /* WESTON_IVI_SHELL_H */
