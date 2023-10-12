/*
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Jason Ekstrand
 * Copyright 2022 Collabora, Ltd.
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

#include "gl-borders.h"
#include "shared/helpers.h"

void
weston_gl_borders_update(struct weston_gl_borders *borders,
			 struct frame *frame,
			 struct weston_output *output)
{
	const struct gl_renderer_interface *glri =
		output->compositor->renderer->gl;
	int32_t ix, iy, iwidth, iheight, fwidth, fheight;

	fwidth = frame_width(frame);
	fheight = frame_height(frame);
	frame_interior(frame, &ix, &iy, &iwidth, &iheight);

	struct weston_geometry border_area[4] = {
		[GL_RENDERER_BORDER_TOP] = {
			.x = 0, .y = 0,
			.width = fwidth, .height = iy
		},
		[GL_RENDERER_BORDER_LEFT] = {
			.x = 0, .y = iy,
			.width = ix, .height = 1
		},
		[GL_RENDERER_BORDER_RIGHT] = {
			.x = iwidth + ix, .y = iy,
			.width = fwidth - (ix + iwidth), .height = 1
		},
		[GL_RENDERER_BORDER_BOTTOM] = {
			.x = 0, .y = iy + iheight,
			.width = fwidth, .height = fheight - (iy + iheight)
		},
	};

	for (unsigned i = 0; i < ARRAY_LENGTH(border_area); i++) {
		const struct weston_geometry *g = &border_area[i];
		int tex_width;
		cairo_t *cr;

		if (!borders->tile[i]) {
			borders->tile[i] =
				cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
							   g->width, g->height);
		}

		tex_width = cairo_image_surface_get_stride(borders->tile[i]) / 4;

		cr = cairo_create(borders->tile[i]);
		cairo_translate(cr, -g->x, -g->y);
		frame_repaint(frame, cr);
		cairo_destroy(cr);
		glri->output_set_border(output, i, g->width, g->height, tex_width,
					cairo_image_surface_get_data(borders->tile[i]));
	}
}

void
weston_gl_borders_fini(struct weston_gl_borders *borders,
		       struct weston_output *output)
{
	const struct gl_renderer_interface *glri =
		output->compositor->renderer->gl;

	for (unsigned i = 0; i < ARRAY_LENGTH(borders->tile); i++) {
		glri->output_set_border(output, i, 0, 0, 0, NULL);
		cairo_surface_destroy(borders->tile[i]);
		borders->tile[i] = NULL;
	}
}
