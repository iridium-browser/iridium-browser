/*
 * Copyright 2014, 2016, 2020 Collabora, Ltd.
 * Copyright 2020 Zodiac Inflight Innovations
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
#include <string.h>
#include <sys/mman.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

struct setup_args {
	struct fixture_metadata meta;
	enum weston_renderer_type renderer;
};

static const struct setup_args my_setup_args[] = {
	{
		.renderer = WESTON_RENDERER_PIXMAN,
		.meta.name = "pixman"
	},
	{
		.renderer = WESTON_RENDERER_GL,
		.meta.name = "GL"
	},
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness,
	      const struct setup_args *arg)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);


TEST(viewport_upscale_solid)
{
	struct client *client;
	struct wp_viewport *viewport;
	pixman_color_t color;
	const int width = 256;
	const int height = 100;
	bool match;

	color_rgb888(&color, 255, 128, 0);

	client = create_client();
	client->surface = create_test_surface(client);
	viewport = client_create_viewport(client);

	client->surface->buffer = create_shm_buffer_a8r8g8b8(client, 2, 2);
	fill_image_with_color(client->surface->buffer->image, &color);

	/* Needs output scale != buffer scale to hit bilinear filter. */
	wl_surface_set_buffer_scale(client->surface->wl_surface, 2);

	wp_viewport_set_destination(viewport, width, height);
	client->surface->width = width;
	client->surface->height = height;

	move_client(client, 19, 19);

	match = verify_screen_content(client, "viewport_upscale_solid", 0,
				      NULL, 0, NULL);
	assert(match);

	wp_viewport_destroy(viewport);
	client_destroy(client);
}
