/*
 * Copyright © 2022 Collabora, Ltd.
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
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <wayland-client.h>
#include "libweston-internal.h"
#include "libweston/matrix.h"

#include "weston-test-client-helper.h"

static void
transform_expect(struct weston_matrix *a, bool valid, enum wl_output_transform ewt)
{
	enum wl_output_transform wt;

	assert(weston_matrix_to_transform(a, &wt) == valid);
	if (valid)
		assert(wt == ewt);
}

TEST(transformation_matrix)
{
	struct weston_matrix a, b;
	int i;

	weston_matrix_init(&a);
	weston_matrix_init(&b);

	weston_matrix_multiply(&a, &b);
	assert(a.type == 0);

	/* Make b a matrix that rotates a surface on the x,y plane by 90
	 * degrees counter-clockwise */
	weston_matrix_rotate_xy(&b, 0, -1);
	assert(b.type == WESTON_MATRIX_TRANSFORM_ROTATE);
	for (i = 0; i < 10; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_90);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_180);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_270);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
	}

	weston_matrix_init(&b);
	/* Make b a matrix that rotates a surface on the x,y plane by 45
	 * degrees counter-clockwise. This should alternate between a
	 * standard transform and a rotation that fails to match any
	 * known rotations. */
	weston_matrix_rotate_xy(&b, cos(-M_PI / 4.0), sin(-M_PI / 4.0));
	assert(b.type == WESTON_MATRIX_TRANSFORM_ROTATE);
	for (i = 0; i < 10; i++) {
		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_90);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_180);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_270);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		assert(a.type == WESTON_MATRIX_TRANSFORM_ROTATE);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
	}

	weston_matrix_init(&b);
	/* Make b a matrix that rotates a surface on the x,y plane by 45
	 * degrees counter-clockwise. This should alternate between a
	 * standard transform and a rotation that fails to match any known
	 * rotations. */
	weston_matrix_rotate_xy(&b, cos(-M_PI / 4.0), sin(-M_PI / 4.0));
	/* Flip a */
	weston_matrix_scale(&a, -1.0, 1.0, 1.0);
	for (i = 0; i < 10; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		/* Since we're not translated or scaled, any matrix that
		 * matches a standard wl_output_transform should not need
		 * filtering when used to transform images - but any
		 * matrix that fails to match will. */
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_90);
		assert(!weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_180);
		assert(!weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_270);
		assert(!weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED);
		assert(!weston_matrix_needs_filtering(&a));
	}

	weston_matrix_init(&a);
	/* Flip a around Y*/
	weston_matrix_scale(&a, 1.0, -1.0, 1.0);
	for (i = 0; i < 100; i++) {
		/* Throw some arbitrary translation in here to make sure it
		 * doesn't have any impact. */
		weston_matrix_translate(&a, 31.0, -25.0, 0.0);
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_270);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_90);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_180);
	}

	/* Scale shouldn't matter, as long as it's positive */
	weston_matrix_scale(&a, 4.0, 3.0, 1.0);
	/* Invert b so it rotates the opposite direction, go back the other way. */
	weston_matrix_invert(&b, &b);
	for (i = 0; i < 100; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_90);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_270);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, false, 0);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_FLIPPED_180);
		assert(weston_matrix_needs_filtering(&a));
	}

	/* Flipping Y should return us from here to normal */
	weston_matrix_scale(&a, 1.0, -1.0, 1.0);
	transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);

	weston_matrix_init(&a);
	weston_matrix_init(&b);
	weston_matrix_translate(&b, 0.5, -0.75, 0);
	/* Crawl along with translations, 0.5 and .75 will both hit an integer multiple
	 * at the same time every 4th step, so assert that only the 4th steps don't need
	 * filtering */
	for (i = 0; i < 100; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(weston_matrix_needs_filtering(&a));

		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(!weston_matrix_needs_filtering(&a));
	}

	weston_matrix_init(&b);
	weston_matrix_scale(&b, 1.5, 2.0, 1.0);
	for (i = 0; i < 10; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(weston_matrix_needs_filtering(&a));
	}
	weston_matrix_invert(&b, &b);
	for (i = 0; i < 9; i++) {
		weston_matrix_multiply(&a, &b);
		transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
		assert(weston_matrix_needs_filtering(&a));
	}
	/* Last step should bring us back to a matrix that doesn't need
	 * a filter */
	weston_matrix_multiply(&a, &b);
	transform_expect(&a, true, WL_OUTPUT_TRANSFORM_NORMAL);
	assert(!weston_matrix_needs_filtering(&a));
}

static void
simple_weston_surface_prepare(struct weston_surface *surf,
			      int buffer_width, int buffer_height,
			      int surface_width, int surface_height,
			      int scale, uint32_t transform,
			      int src_x, int src_y,
			      int src_width, int src_height)
{
	struct weston_buffer_viewport vp = {
		.buffer = {
			.transform = transform,
			.scale = scale,
			.src_x = wl_fixed_from_int(src_x),
			.src_y = wl_fixed_from_int(src_y),
			.src_width = wl_fixed_from_int(src_width),
			.src_height = wl_fixed_from_int(src_height),
			},
		.surface = {
			.width = surface_width,
			.height = surface_height,
		},
	};
	surf->buffer_viewport = vp;
	convert_size_by_transform_scale(&surf->width_from_buffer,
					&surf->height_from_buffer,
					buffer_width,
					buffer_height,
					transform,
					scale);
	weston_surface_build_buffer_matrix(surf,
					   &surf->surface_to_buffer_matrix);
	weston_matrix_invert(&surf->buffer_to_surface_matrix,
			     &surf->surface_to_buffer_matrix);
}

static void
surface_test_all_transforms(struct weston_surface *surf,
			    int buffer_width, int buffer_height,
			    int surface_width, int surface_height,
			    int scale, int src_x, int src_y,
			    int src_width, int src_height)
{
	int transform;

	for (transform = WL_OUTPUT_TRANSFORM_NORMAL;
	     transform <= WL_OUTPUT_TRANSFORM_FLIPPED_270; transform++) {
		simple_weston_surface_prepare(surf,
					      buffer_width, buffer_height,
					      surface_width, surface_height,
					      scale, transform,
					      src_x, src_y,
					      src_width, src_height);
		transform_expect(&surf->surface_to_buffer_matrix,
				 true, transform);
	}
}

TEST(surface_matrix_to_standard_transform)
{
	struct weston_surface surf;
	int scale;

	for (scale = 1; scale < 8; scale++) {
		/* A simple case */
		surface_test_all_transforms(&surf, 500, 700, -1, -1, scale,
					    0, 0, 500, 700);
		/* Translate the source corner */
		surface_test_all_transforms(&surf, 500, 700, -1, -1, scale,
					    70, 20, 500, 700);
		/* Get some scaling (and fractional translation) in there */
		surface_test_all_transforms(&surf, 723, 300, 512, 77, scale,
					    120, 10, 200, 200);
	}
}

static void
simple_weston_output_prepare(struct weston_output *output,
			     int x, int y, int width, int height,
			     int scale, uint32_t transform)
{
	output->x = x;
	output->y = y;
	output->width = width;
	output->height = height;
	output->current_scale = scale;
	output->transform = transform;
	wl_list_init(&output->paint_node_list);
	weston_output_update_matrix(output);
}

static struct weston_vector
simple_transform_vector(struct weston_output *output, struct weston_vector in)
{
	struct weston_vector out = in;
	int scale = output->current_scale;

	switch (output->transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
		out.f[0] = (-output->x + in.f[0]) * scale;
		out.f[1] = (-output->y + in.f[1]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED:
		out.f[0] = (output->x + output->width - in.f[0]) * scale;
		out.f[1] = (-output->y + in.f[1]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_90:
		out.f[0] = (-output->y + in.f[1]) * scale;
		out.f[1] = (output->x + output->width - in.f[0]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		out.f[0] = (-output->y + in.f[1]) * scale;
		out.f[1] = (-output->x + in.f[0]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_180:
		out.f[0] = (output->x + output->width - in.f[0]) * scale;
		out.f[1] = (output->y + output->height - in.f[1]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		out.f[0] = (-output->x + in.f[0]) * scale;
		out.f[1] = (output->y + output->height - in.f[1]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_270:
		out.f[0] = (output->y + output->height - in.f[1]) * scale;
		out.f[1] = (-output->x + in.f[0]) * scale;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		out.f[0] = (output->y + output->height - in.f[1]) * scale;
		out.f[1] = (output->x + output->width - in.f[0]) * scale;
		break;
	}
	out.f[2] = 0;
	out.f[3] = 1;

	return out;
}

static void
output_test_all_transforms(struct weston_output *output,
			   int x, int y, int width, int height, int scale)
{
	int i;
	int transform;
	struct weston_vector t = { { 7.0, 13.0, 0.0, 1.0 } };
	struct weston_vector v, sv;

	for (transform = WL_OUTPUT_TRANSFORM_NORMAL;
	     transform <= WL_OUTPUT_TRANSFORM_FLIPPED_270; transform++) {
		simple_weston_output_prepare(output, x, y, width, height,
					      scale, transform);
		/* The inverse matrix takes us from output to global space,
		 * which makes it the one that will have the expected
		 * standard transform.
		 */
		transform_expect(&output->matrix, true, transform);

		v = t;
		weston_matrix_transform(&output->matrix, &v);
		sv = simple_transform_vector(output, t);
		for (i = 0; i < 4; i++)
			assert (sv.f[i] == v.f[i]);
	}
}

TEST(output_matrix_to_standard_transform)
{
	struct weston_output output;
	int scale;

	/* Just a few arbitrary sizes and positions to make sure we have
	 * scales and translations.
	 */
	for (scale = 1; scale < 8; scale++) {
		output_test_all_transforms(&output, 0, 0, 1024, 768, scale);
		output_test_all_transforms(&output, 1000, 1000, 1024, 768, scale);
		output_test_all_transforms(&output, 1024, 768, 1920, 1080, scale);
	}
}
