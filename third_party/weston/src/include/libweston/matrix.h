/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2012 Collabora, Ltd.
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

#ifndef WESTON_MATRIX_H
#define WESTON_MATRIX_H

#include <assert.h>
#include <stdbool.h>

#include <wayland-server-protocol.h>

#ifdef  __cplusplus
extern "C" {
#endif

enum weston_matrix_transform_type {
	WESTON_MATRIX_TRANSFORM_TRANSLATE	= (1 << 0),
	WESTON_MATRIX_TRANSFORM_SCALE		= (1 << 1),
	WESTON_MATRIX_TRANSFORM_ROTATE		= (1 << 2),
	WESTON_MATRIX_TRANSFORM_OTHER		= (1 << 3),
};

struct weston_matrix {
	float d[16];
	unsigned int type;
};

struct weston_vector {
	float f[4];
};

/** Arbitrary coordinates in any space */
struct weston_coord {
	double x;
	double y;
};

/** Coordinates in some weston_buffer (physical pixels) */
struct weston_coord_buffer {
	struct weston_coord c;
};

/** Coordinates in the global compositor space (logical pixels) */
struct weston_coord_global {
	struct weston_coord c;
};

/** surface-local coordinates on a specific surface */
struct weston_coord_surface {
	struct weston_coord c;
	const struct weston_surface *coordinate_space_id;
};

void
weston_matrix_init(struct weston_matrix *matrix);
void
weston_matrix_multiply(struct weston_matrix *m, const struct weston_matrix *n);
void
weston_matrix_scale(struct weston_matrix *matrix, float x, float y, float z);
void
weston_matrix_translate(struct weston_matrix *matrix,
			float x, float y, float z);
void
weston_matrix_rotate_xy(struct weston_matrix *matrix, float cos, float sin);
void
weston_matrix_transform(const struct weston_matrix *matrix,
			struct weston_vector *v);

struct weston_coord
weston_matrix_transform_coord(const struct weston_matrix *matrix,
			      struct weston_coord coord);

int
weston_matrix_invert(struct weston_matrix *inverse,
		     const struct weston_matrix *matrix);

bool
weston_matrix_needs_filtering(const struct weston_matrix *matrix);

bool
weston_matrix_to_transform(const struct weston_matrix *mat,
			   enum wl_output_transform *transform);

void
weston_matrix_init_transform(struct weston_matrix *matrix,
			     enum wl_output_transform transform,
			     int x, int y, int width, int height,
			     int scale);

static inline struct weston_coord __attribute__ ((warn_unused_result))
weston_coord_from_fixed(wl_fixed_t x, wl_fixed_t y)
{
	struct weston_coord out;

	out.x = wl_fixed_to_double(x);
	out.y = wl_fixed_to_double(y);
	return out;
}

static inline struct weston_coord __attribute__ ((warn_unused_result))
weston_coord(double x, double y)
{
	return (struct weston_coord){ .x = x, .y = y };
}

static inline struct weston_coord_surface __attribute__ ((warn_unused_result))
weston_coord_surface(double x, double y, const struct weston_surface *surface)
{
	struct weston_coord_surface out;

	assert(surface);

	out.c = weston_coord(x, y);
	out.coordinate_space_id = surface;

	return out;
}

static inline struct weston_coord_surface __attribute__ ((warn_unused_result))
weston_coord_surface_from_fixed(wl_fixed_t x, wl_fixed_t y,
				const struct weston_surface *surface)
{
	struct weston_coord_surface out;

	assert(surface);

	out.c.x = wl_fixed_to_double(x);
	out.c.y = wl_fixed_to_double(y);
	out.coordinate_space_id = surface;

	return out;
}

static inline struct weston_coord __attribute__ ((warn_unused_result))
weston_coord_add(struct weston_coord a, struct weston_coord b)
{
	return weston_coord(a.x + b.x, a.y + b.y);
}

static inline struct weston_coord __attribute__ ((warn_unused_result))
weston_coord_sub(struct weston_coord a, struct weston_coord b)
{
	return weston_coord(a.x - b.x, a.y - b.y);
}

#ifdef  __cplusplus
}
#endif

#endif /* WESTON_MATRIX_H */
