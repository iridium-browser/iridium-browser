/*
 * Copyright © 2014, 2016 Collabora, Ltd.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static void
set_source(struct wp_viewport *vp, int x, int y, int w, int h)
{
	wp_viewport_set_source(vp, wl_fixed_from_int(x), wl_fixed_from_int(y),
				   wl_fixed_from_int(w), wl_fixed_from_int(h));
}

TEST(test_viewporter_double_create)
{
	struct wp_viewporter *viewporter;
	struct wp_viewport *vp[2];
	struct client *client;

	client = create_client_and_test_surface(100, 50, 123, 77);

	viewporter = bind_to_singleton_global(client,
					      &wp_viewporter_interface, 1);
	vp[0] = wp_viewporter_get_viewport(viewporter, client->surface->wl_surface);
	vp[1] = wp_viewporter_get_viewport(viewporter, client->surface->wl_surface);

	expect_protocol_error(client, &wp_viewporter_interface,
			      WP_VIEWPORTER_ERROR_VIEWPORT_EXISTS);

	wp_viewport_destroy(vp[1]);
	wp_viewport_destroy(vp[0]);
	wp_viewporter_destroy(viewporter);
	client_destroy(client);
}

struct bad_source_rect_args {
	int x, y, w, h;
};

static const struct bad_source_rect_args bad_source_rect_args[] = {
	{ -5,  0,  20,  10 },
	{  0, -5,  20,  10 },
	{  5,  6,   0,  10 },
	{  5,  6,  20,   0 },
	{  5,  6, -20,  10 },
	{  5,  6,  20, -10 },
	{ -1, -1,  20,  10 },
	{  5,  6,  -1,  -1 },
};

TEST_P(test_viewporter_bad_source_rect, bad_source_rect_args)
{
	const struct bad_source_rect_args *args = data;
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);

	vp = client_create_viewport(client);

	testlog("wp_viewport.set_source x=%d, y=%d, w=%d, h=%d\n",
		args->x, args->y, args->w, args->h);
	set_source(vp, args->x, args->y, args->w, args->h);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_BAD_VALUE);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

TEST(test_viewporter_unset_source_rect)
{
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);

	vp = client_create_viewport(client);
	set_source(vp, -1, -1, -1, -1);
	wl_surface_commit(client->surface->wl_surface);

	client_roundtrip(client);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

struct bad_destination_args {
	int w, h;
};

static const struct bad_destination_args bad_destination_args[] = {
	{   0,  10 },
	{  20,   0 },
	{ -20,  10 },
	{  -1,  10 },
	{  20, -10 },
	{  20,  -1 },
};

TEST_P(test_viewporter_bad_destination_size, bad_destination_args)
{
	const struct bad_destination_args *args = data;
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);

	vp = client_create_viewport(client);

	testlog("wp_viewport.set_destination w=%d, h=%d\n", args->w, args->h);
	wp_viewport_set_destination(vp, args->w, args->h);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_BAD_VALUE);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

TEST(test_viewporter_unset_destination_size)
{
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);

	vp = client_create_viewport(client);
	wp_viewport_set_destination(vp, -1, -1);
	wl_surface_commit(client->surface->wl_surface);

	client_roundtrip(client);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

struct nonint_destination_args {
	wl_fixed_t w, h;
};

static const struct nonint_destination_args nonint_destination_args[] = {
#define F(i,f) ((i) * 256 + (f))
	{ F(20, 0),   F(10, 1) },
	{ F(20, 0),   F(10, -1) },
	{ F(20, 1),   F(10, 0) },
	{ F(20, -1),  F(10, 0) },
	{ F(20, 128), F(10, 128) },
#undef F
};

TEST_P(test_viewporter_non_integer_destination_size, nonint_destination_args)
{
	const struct nonint_destination_args *args = data;
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);

	vp = client_create_viewport(client);

	testlog("non-integer size w=%f, h=%f\n",
		wl_fixed_to_double(args->w), wl_fixed_to_double(args->h));
	wp_viewport_set_source(vp, 5, 6, args->w, args->h);
	wp_viewport_set_destination(vp, -1, -1);
	wl_surface_commit(client->surface->wl_surface);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_BAD_SIZE);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

struct source_buffer_args {
	wl_fixed_t x, y;
	wl_fixed_t w, h;
	int buffer_scale;
	enum wl_output_transform buffer_transform;
};

static int
get_surface_width(struct surface *surface,
		  int buffer_scale,
		  enum wl_output_transform buffer_transform)
{
	switch (buffer_transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
	case WL_OUTPUT_TRANSFORM_180:
	case WL_OUTPUT_TRANSFORM_FLIPPED:
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		return surface->width / buffer_scale;
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		return surface->height / buffer_scale;
	}

	return -1;
}

static int
get_surface_height(struct surface *surface,
		   int buffer_scale,
		   enum wl_output_transform buffer_transform)
{
	switch (buffer_transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
	case WL_OUTPUT_TRANSFORM_180:
	case WL_OUTPUT_TRANSFORM_FLIPPED:
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		return surface->height / buffer_scale;
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		return surface->width / buffer_scale;
	}

	return -1;
}

static struct wp_viewport *
setup_source_vs_buffer(struct client *client,
		       const struct source_buffer_args *args)
{
	struct wl_surface *surf;
	struct wp_viewport *vp;

	surf = client->surface->wl_surface;
	vp = client_create_viewport(client);

	testlog("surface %dx%d\n",
		get_surface_width(client->surface,
				  args->buffer_scale, args->buffer_transform),
		get_surface_height(client->surface,
				   args->buffer_scale, args->buffer_transform));
	testlog("source x=%f, y=%f, w=%f, h=%f; "
		"buffer scale=%d, transform=%d\n",
		wl_fixed_to_double(args->x), wl_fixed_to_double(args->y),
		wl_fixed_to_double(args->w), wl_fixed_to_double(args->h),
		args->buffer_scale, args->buffer_transform);

	wl_surface_set_buffer_scale(surf, args->buffer_scale);
	wl_surface_set_buffer_transform(surf, args->buffer_transform);
	wl_surface_attach(surf, client->surface->buffer->proxy, 0, 0);
	wp_viewport_set_source(vp, args->x, args->y, args->w, args->h);
	wp_viewport_set_destination(vp, 99, 99);
	wl_surface_commit(surf);

	return vp;
}

/* buffer dimensions */
#define WIN_W 124
#define WIN_H 78

/* source rect base size */
#define SRC_W 20
#define SRC_H 10

/* margin */
#define MRG 10
/* epsilon of wl_fixed_t */
#define EPS 1

TEST(test_viewporter_source_buffer_params)
{
	const int max_scale = 2;

	/* buffer_scale requirement */
	assert(WIN_W % max_scale == 0);
	assert(WIN_H % max_scale == 0);

	/* source rect must fit inside regardless of scale and transform */
	assert(SRC_W < WIN_W / max_scale);
	assert(SRC_H < WIN_H / max_scale);
	assert(SRC_W < WIN_H / max_scale);
	assert(SRC_H < WIN_W / max_scale);

	/* If buffer scale was ignored, source rect should be inside instead */
	assert(WIN_W / max_scale + SRC_W + MRG < WIN_W);
	assert(WIN_H / max_scale + SRC_H + MRG < WIN_H);
	assert(WIN_W / max_scale + SRC_H + MRG < WIN_W);
	assert(WIN_H / max_scale + SRC_W + MRG < WIN_H);
}

static const struct source_buffer_args bad_source_buffer_args[] = {
#define F(i) ((i) * 256)

/* Flush right-top, but epsilon too far right. */
	{ F(WIN_W - SRC_W) + EPS, F(0), F(SRC_W),       F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W - SRC_W),       F(0), F(SRC_W) + EPS, F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
/* Flush left-bottom, but epsilon too far down. */
	{ F(0), F(WIN_H - SRC_H) + EPS, F(SRC_W), F(SRC_H),       1, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(0), F(WIN_H - SRC_H),       F(SRC_W), F(SRC_H) + EPS, 1, WL_OUTPUT_TRANSFORM_NORMAL },
/* Completely outside on the right. */
	{ F(WIN_W + MRG), F(0), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
/* Competely outside on the bottom. */
	{ F(0), F(WIN_H + MRG), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },

/*
 * buffer_scale=2, so the surface size will be halved.
 * If buffer_scale was not taken into account, these would all be inside.
 * These are the same as above, but adapted to buffer_scale=2.
 */
	{ F(WIN_W / 2 - SRC_W) + EPS, F(0),                       F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W / 2 - SRC_W),       F(0),                       F(SRC_W) + EPS, F(SRC_H),       2, WL_OUTPUT_TRANSFORM_NORMAL },

	{ F(0),                       F(WIN_H / 2 - SRC_H) + EPS, F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(0),                       F(WIN_H / 2 - SRC_H),       F(SRC_W),       F(SRC_H) + EPS, 2, WL_OUTPUT_TRANSFORM_NORMAL },

	{ F(WIN_W / 2 + MRG),         F(0),                       F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_NORMAL },

	{ F(0),                       F(WIN_H / 2 + MRG),         F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_NORMAL },

/* Exceeding bottom-right corner by epsilon: */
/* non-dimension-swapping transforms */
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H),       F(SRC_W),       F(SRC_H) + EPS, 1, WL_OUTPUT_TRANSFORM_FLIPPED_180 },
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H) + EPS, F(SRC_W),       F(SRC_H),       1, WL_OUTPUT_TRANSFORM_FLIPPED },
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H),       F(SRC_W) + EPS, F(SRC_H),       1, WL_OUTPUT_TRANSFORM_180 },

/* dimension-swapping transforms */
	{ F(WIN_H - SRC_W) + EPS, F(WIN_W - SRC_H),       F(SRC_W),       F(SRC_H),       1, WL_OUTPUT_TRANSFORM_90 },
	{ F(WIN_H - SRC_W),       F(WIN_W - SRC_H) + EPS, F(SRC_W),       F(SRC_H),       1, WL_OUTPUT_TRANSFORM_270 },
	{ F(WIN_H - SRC_W),       F(WIN_W - SRC_H),       F(SRC_W) + EPS, F(SRC_H),       1, WL_OUTPUT_TRANSFORM_FLIPPED_90 },
	{ F(WIN_H - SRC_W),       F(WIN_W - SRC_H),       F(SRC_W),       F(SRC_H) + EPS, 1, WL_OUTPUT_TRANSFORM_FLIPPED_270 },

/* non-dimension-swapping transforms, buffer_scale=2 */
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H) + EPS, F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_FLIPPED_180 },
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H),       F(SRC_W) + EPS, F(SRC_H),       2, WL_OUTPUT_TRANSFORM_FLIPPED },
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H),       F(SRC_W),       F(SRC_H) + EPS, 2, WL_OUTPUT_TRANSFORM_180 },

/* dimension-swapping transforms, buffer_scale=2 */
	{ F(WIN_H / 2 - SRC_W) + EPS, F(WIN_W / 2 - SRC_H),       F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_90 },
	{ F(WIN_H / 2 - SRC_W),       F(WIN_W / 2 - SRC_H) + EPS, F(SRC_W),       F(SRC_H),       2, WL_OUTPUT_TRANSFORM_270 },
	{ F(WIN_H / 2 - SRC_W),       F(WIN_W / 2 - SRC_H),       F(SRC_W) + EPS, F(SRC_H),       2, WL_OUTPUT_TRANSFORM_FLIPPED_90 },
	{ F(WIN_H / 2 - SRC_W),       F(WIN_W / 2 - SRC_H),       F(SRC_W),       F(SRC_H) + EPS, 2, WL_OUTPUT_TRANSFORM_FLIPPED_270 },

#undef F
};

TEST_P(test_viewporter_source_outside_buffer, bad_source_buffer_args)
{
	const struct source_buffer_args *args = data;
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, WIN_W, WIN_H);
	vp = setup_source_vs_buffer(client, args);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_OUT_OF_BUFFER);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

static const struct source_buffer_args good_source_buffer_args[] = {
#define F(i) ((i) * 256)

/* top-left, top-right, bottom-left, and bottom-right corner */
	{ F(0),             F(0),             F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W - SRC_W), F(0),             F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(0),             F(WIN_H - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_NORMAL },

/* buffer_scale=2, so the surface size will be halved */
	{ F(0),                 F(0),                 F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W / 2 - SRC_W), F(0),                 F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(0),                 F(WIN_H / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_NORMAL },
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_NORMAL },

/* with half pixel offset */
	{ F(WIN_W / 2 - SRC_W) + 128, F(WIN_H / 2 - SRC_H) + 128, F(SRC_W) - 128, F(SRC_H) - 128, 2, WL_OUTPUT_TRANSFORM_NORMAL },

/* Flushed to bottom-right corner: */
/* non-dimension-swapping transforms */
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_FLIPPED_180 },
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_FLIPPED },
	{ F(WIN_W - SRC_W), F(WIN_H - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_180 },

/* dimension-swapping transforms */
	{ F(WIN_H - SRC_W), F(WIN_W - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_90 },
	{ F(WIN_H - SRC_W), F(WIN_W - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_270 },
	{ F(WIN_H - SRC_W), F(WIN_W - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_FLIPPED_90 },
	{ F(WIN_H - SRC_W), F(WIN_W - SRC_H), F(SRC_W), F(SRC_H), 1, WL_OUTPUT_TRANSFORM_FLIPPED_270 },

/* non-dimension-swapping transforms, buffer_scale=2 */
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_FLIPPED_180 },
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_FLIPPED },
	{ F(WIN_W / 2 - SRC_W), F(WIN_H / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_180 },

/* dimension-swapping transforms, buffer_scale=2 */
	{ F(WIN_H / 2 - SRC_W), F(WIN_W / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_90 },
	{ F(WIN_H / 2 - SRC_W), F(WIN_W / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_270 },
	{ F(WIN_H / 2 - SRC_W), F(WIN_W / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_FLIPPED_90 },
	{ F(WIN_H / 2 - SRC_W), F(WIN_W / 2 - SRC_H), F(SRC_W), F(SRC_H), 2, WL_OUTPUT_TRANSFORM_FLIPPED_270 },

#undef F
};

TEST_P(test_viewporter_source_inside_buffer, good_source_buffer_args)
{
	const struct source_buffer_args *args = data;
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, WIN_W, WIN_H);
	vp = setup_source_vs_buffer(client, args);
	wp_viewport_destroy(vp);
	client_destroy(client);
}

#undef WIN_W
#undef WIN_H
#undef SRC_W
#undef SRC_H
#undef MRG
#undef EPS

TEST(test_viewporter_outside_null_buffer)
{
	struct client *client;
	struct wp_viewport *vp;
	struct wl_surface *surf;

	client = create_client_and_test_surface(100, 50, 123, 77);
	surf = client->surface->wl_surface;

	/* If buffer is NULL, does not matter what the source rect is. */
	vp = client_create_viewport(client);
	wl_surface_attach(surf, NULL, 0, 0);
	set_source(vp, 1000, 1000, 20, 10);
	wp_viewport_set_destination(vp, 99, 99);
	wl_surface_commit(surf);
	client_roundtrip(client);

	/* Try again, with all old values. */
	wl_surface_commit(surf);
	client_roundtrip(client);

	/* Try once more with old NULL buffer. */
	set_source(vp, 1200, 1200, 20, 10);
	wl_surface_commit(surf);
	client_roundtrip(client);

	/* When buffer comes back, source rect matters again. */
	wl_surface_attach(surf, client->surface->buffer->proxy, 0, 0);
	wl_surface_commit(surf);
	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_OUT_OF_BUFFER);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

TEST(test_viewporter_no_surface_set_source)
{
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);
	vp = client_create_viewport(client);
	wl_surface_destroy(client->surface->wl_surface);
	client->surface->wl_surface = NULL;

	/* But the wl_surface does not exist anymore. */
	set_source(vp, 1000, 1000, 20, 10);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_NO_SURFACE);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

TEST(test_viewporter_no_surface_set_destination)
{
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);
	vp = client_create_viewport(client);
	wl_surface_destroy(client->surface->wl_surface);
	client->surface->wl_surface = NULL;

	/* But the wl_surface does not exist anymore. */
	wp_viewport_set_destination(vp, 99, 99);

	expect_protocol_error(client, &wp_viewport_interface,
			      WP_VIEWPORT_ERROR_NO_SURFACE);

	wp_viewport_destroy(vp);
	client_destroy(client);
}

TEST(test_viewporter_no_surface_destroy)
{
	struct client *client;
	struct wp_viewport *vp;

	client = create_client_and_test_surface(100, 50, 123, 77);
	vp = client_create_viewport(client);
	wl_surface_destroy(client->surface->wl_surface);
	client->surface->wl_surface = NULL;

	/* But the wl_surface does not exist anymore. */
	wp_viewport_destroy(vp);

	client_destroy(client);
}
