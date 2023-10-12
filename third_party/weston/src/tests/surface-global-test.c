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

#include <assert.h>
#include <stdint.h>

#include <libweston/libweston.h>
#include "libweston-internal.h"
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

PLUGIN_TEST(surface_to_from_global)
{
	/* struct weston_compositor *compositor; */
	struct weston_surface *surface;
	struct weston_view *view;
	struct weston_coord_global cg;
	struct weston_coord_surface cs;

	surface = weston_surface_create(compositor);
	assert(surface);
	view = weston_view_create(surface);
	assert(view);
	surface->width = 50;
	surface->height = 50;
	weston_view_set_position(view, 5, 10);
	weston_view_update_transform(view);

	cs = weston_coord_surface(33, 22, surface);
	cg = weston_coord_surface_to_global(view, cs);
	assert(cg.c.x == 38 && cg.c.y == 32);

	cs = weston_coord_surface(-8, -2, surface);
	cg = weston_coord_surface_to_global(view, cs);
	assert(cg.c.x == -3 && cg.c.y == 8);

	cs = weston_coord_surface_from_fixed(wl_fixed_from_int(12),
					     wl_fixed_from_int(5), surface);
	cg = weston_coord_surface_to_global(view, cs);
	assert(wl_fixed_from_double(cg.c.x) == wl_fixed_from_int(17) &&
	       wl_fixed_from_double(cg.c.y) == wl_fixed_from_int(15));

	cg.c = weston_coord(38, 32);
	cs = weston_coord_global_to_surface(view, cg);
	assert(cs.c.x == 33 && cs.c.y == 22);

	cg.c = weston_coord(42, 5);
	cs = weston_coord_global_to_surface(view, cg);
	assert(cs.c.x == 37 && cs.c.y == -5);

	cg.c = weston_coord_from_fixed(wl_fixed_from_int(21),
				       wl_fixed_from_int(100));
	cs = weston_coord_global_to_surface(view, cg);
	assert(wl_fixed_from_double(cs.c.x) == wl_fixed_from_int(16) &&
	       wl_fixed_from_double(cs.c.y) == wl_fixed_from_int(90));

	cg.c = weston_coord(0, 0);
	cs = weston_coord_global_to_surface(view, cg);
	assert(cs.c.x == -5 && cs.c.y == -10);

	cg.c = weston_coord(5, 10);
	cs = weston_coord_global_to_surface(view, cg);
	assert(cs.c.x == 0 && cs.c.y == 0);

	/* Destroys all views too. */
	weston_surface_unref(surface);
}
