/*
 * Copyright (C) 2014 DENSO CORPORATION
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

#ifndef IVI_LAYOUT_SHELL_H
#define IVI_LAYOUT_SHELL_H

#include <stdint.h>

/*
 * This is the interface that ivi-layout exposes to ivi-shell.
 * It is private to ivi-shell.so plugin.
 */

struct wl_listener;
struct weston_compositor;
struct weston_view;
struct weston_surface;
struct ivi_layout_surface;

void
ivi_layout_desktop_surface_configure(struct ivi_layout_surface *ivisurf,
			     int32_t width, int32_t height);

struct ivi_layout_surface*
ivi_layout_desktop_surface_create(struct weston_surface *wl_surface);

void
ivi_layout_surface_configure(struct ivi_layout_surface *ivisurf,
			     int32_t width, int32_t height);

struct ivi_layout_surface*
ivi_layout_surface_create(struct weston_surface *wl_surface,
			  uint32_t id_surface);

void
ivi_layout_init_with_compositor(struct weston_compositor *ec);

void
ivi_layout_surface_destroy(struct ivi_layout_surface *ivisurf);

int
load_controller_modules(struct weston_compositor *compositor, const char *modules,
			int *argc, char *argv[]);

#endif /* IVI_LAYOUT_SHELL_H */
