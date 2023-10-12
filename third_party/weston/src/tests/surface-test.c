/*
 * Copyright © 2012 Intel Corporation
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
#include <assert.h>

#include <libweston/libweston.h>
#include "compositor/weston.h"
#include "weston-test-runner.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_plugin(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

PLUGIN_TEST(surface_transform)
{
	/* struct weston_compositor *compositor; */
	struct weston_surface *surface;
	struct weston_view *view;
	struct weston_coord_surface coord_s;
	struct weston_coord_global coord_g;

	surface = weston_surface_create(compositor);
	assert(surface);
	view = weston_view_create(surface);
	assert(view);
	surface->width = 200;
	surface->height = 200;
	weston_view_set_position(view, 100, 100);
	weston_view_update_transform(view);
	coord_s = weston_coord_surface(20, 20, surface);
	coord_g = weston_coord_surface_to_global(view, coord_s);

	fprintf(stderr, "20,20 maps to %f, %f\n", coord_g.c.x, coord_g.c.y);
	assert(coord_g.c.x == 120 && coord_g.c.y == 120);

	weston_view_set_position(view, 150, 300);
	weston_view_update_transform(view);
	coord_s = weston_coord_surface(50, 40, surface);
	coord_g = weston_coord_surface_to_global(view, coord_s);
	assert(coord_g.c.x == 200 && coord_g.c.y == 340);

	/* Destroys all views too. */
	weston_surface_unref(surface);
}
