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
#include <stdbool.h>
#include <pixman.h>

/*
 * This is the interface that ivi-layout exposes to ivi-shell.
 * It is private to ivi-shell.so plugin.
 */

struct wl_listener;
struct weston_compositor;
struct weston_view;
struct weston_surface;
struct ivi_layout_surface;
struct ivi_shell;

void
ivi_layout_desktop_surface_configure(struct ivi_layout_surface *ivisurf,
			     int32_t width, int32_t height);

struct ivi_layout_surface*
ivi_layout_desktop_surface_create(struct weston_surface *wl_surface,
				  struct weston_desktop_surface *surface);

void
ivi_layout_input_panel_surface_configure(struct ivi_layout_surface *ivisurf,
					 int32_t width, int32_t height);

void
ivi_layout_update_text_input_cursor(pixman_box32_t *cursor_rectangle);

void
ivi_layout_show_input_panel(struct ivi_layout_surface *ivisurf,
			    struct ivi_layout_surface *target_ivisurf,
			    bool overlay_panel);

void
ivi_layout_hide_input_panel(struct ivi_layout_surface *ivisurf);

void
ivi_layout_update_input_panel(struct ivi_layout_surface *ivisurf,
			    bool overlay_panel);

struct ivi_layout_surface*
ivi_layout_input_panel_surface_create(struct weston_surface *wl_surface);

void
ivi_layout_surface_configure(struct ivi_layout_surface *ivisurf,
			     int32_t width, int32_t height);

struct ivi_layout_surface*
ivi_layout_surface_create(struct weston_surface *wl_surface,
			  uint32_t id_surface);

void
ivi_layout_init(struct weston_compositor *ec, struct ivi_shell *shell);

void
ivi_layout_fini(void);

void
ivi_layout_surface_destroy(struct ivi_layout_surface *ivisurf);

int
load_controller_modules(struct weston_compositor *compositor, const char *modules,
			int *argc, char *argv[]);

void
ivi_layout_ivi_shell_destroy(void);

#endif /* IVI_LAYOUT_SHELL_H */
