/*
 * Copyright © 2020 Collabora, Ltd.
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

#define TRANSFORM(x) WL_OUTPUT_TRANSFORM_ ## x, #x
#define RENDERERS(s, t)							\
	{								\
		.renderer = WESTON_RENDERER_PIXMAN,			\
		.scale = s,						\
		.transform = WL_OUTPUT_TRANSFORM_ ## t,			\
		.transform_name = #t,					\
		.meta.name = "pixman " #s " " #t,			\
	},								\
	{								\
		.renderer = WESTON_RENDERER_GL,				\
		.scale = s,						\
		.transform = WL_OUTPUT_TRANSFORM_ ## t,			\
		.transform_name = #t,					\
		.meta.name = "GL " #s " " #t,				\
	}

struct setup_args {
	struct fixture_metadata meta;
	enum weston_renderer_type renderer;
	int scale;
	enum wl_output_transform transform;
	const char *transform_name;
};

static const struct setup_args my_setup_args[] = {
	RENDERERS(1, NORMAL),
	RENDERERS(1, 90),
	RENDERERS(1, 180),
	RENDERERS(1, 270),
	RENDERERS(1, FLIPPED),
	RENDERERS(1, FLIPPED_90),
	RENDERERS(1, FLIPPED_180),
	RENDERERS(1, FLIPPED_270),
	RENDERERS(2, NORMAL),
	RENDERERS(3, NORMAL),
	RENDERERS(2, 90),
	RENDERERS(2, 180),
	RENDERERS(2, FLIPPED),
	RENDERERS(3, FLIPPED_270),
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness, const struct setup_args *arg)
{
	struct compositor_setup setup;

	/* The width and height are chosen to produce 324x240 framebuffer, to
	 * emulate keeping the video mode constant.
	 * This resolution is divisible by 2 and 3.
	 * Headless multiplies the given size by scale.
	 */

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.width = 324 / arg->scale;
	setup.height = 240 / arg->scale;
	setup.scale = arg->scale;
	setup.transform = arg->transform;
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

struct buffer_args {
	int scale;
	enum wl_output_transform transform;
	const char *transform_name;
};

static const struct buffer_args my_buffer_args[] = {
	{ 1, TRANSFORM(NORMAL) },
	{ 2, TRANSFORM(90) },
};

TEST_P(output_transform, my_buffer_args)
{
	const struct buffer_args *bargs = data;
	const struct setup_args *oargs;
	struct client *client;
	bool match;
	char *refname;
	int ret;

	oargs = &my_setup_args[get_test_fixture_index()];

	ret = asprintf(&refname, "output_%d-%s_buffer_%d-%s",
		       oargs->scale, oargs->transform_name,
		       bargs->scale, bargs->transform_name);
	assert(ret);

	testlog("%s: %s\n", get_test_name(), refname);

	/*
	 * NOTE! The transform set below is a lie.
	 * Take that into account when analyzing screenshots.
	 */

	client = create_client();
	client->surface = create_test_surface(client);
	client->surface->width = 10000; /* used only for damage */
	client->surface->height = 10000;
	client->surface->buffer = client_buffer_from_image_file(client,
							"basic-test-card",
							bargs->scale);
	wl_surface_set_buffer_scale(client->surface->wl_surface, bargs->scale);
	wl_surface_set_buffer_transform(client->surface->wl_surface,
					bargs->transform);
	move_client(client, 19, 19);

	match = verify_screen_content(client, refname, 0, NULL, 0, NULL);
	assert(match);

	client_destroy(client);
	free(refname);
}
