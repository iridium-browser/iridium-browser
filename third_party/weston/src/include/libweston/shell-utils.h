/*
 * Copyright 2021, 2022 Collabora, Ltd.
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
 *
 */
#pragma once

#include "config.h"
#include "shared/helpers.h"
#include <libweston/libweston.h>

#ifdef __cplusplus
extern "C" {
#endif

/* parameter for weston_curtain_create() */
struct weston_curtain_params {
	int (*get_label)(struct weston_surface *es, char *buf, size_t len);
	void (*surface_committed)(struct weston_surface *es,
				  struct weston_coord_surface new_origin);
	void *surface_private;
	float r, g, b, a;
	int x, y, width, height;
	bool capture_input;
};

struct weston_curtain {
	struct weston_view *view;
	struct weston_buffer_reference *buffer_ref;
};

struct weston_output *
weston_shell_utils_get_default_output(struct weston_compositor *compositor);

struct weston_output *
weston_shell_utils_get_focused_output(struct weston_compositor *compositor);

void
weston_shell_utils_center_on_output(struct weston_view *view,
				    struct weston_output *output);

void
weston_shell_utils_subsurfaces_boundingbox(struct weston_surface *surface,
					   int32_t *x, int32_t *y,
					   int32_t *w, int32_t *h);

int
weston_shell_utils_surface_get_label(struct weston_surface *surface,
				     char *buf, size_t len);

/* helper to create a view w/ a color */
struct weston_curtain *
weston_shell_utils_curtain_create(struct weston_compositor *compositor,
				  struct weston_curtain_params *params);
void
weston_shell_utils_curtain_destroy(struct weston_curtain *curtain);

#ifdef __cplusplus
}
#endif
