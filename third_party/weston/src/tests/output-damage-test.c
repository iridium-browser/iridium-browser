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

#define RENDERERS(s, t)							\
	{								\
		.renderer = WESTON_RENDERER_PIXMAN,			\
		.scale = s,						\
		.transform = WL_OUTPUT_TRANSFORM_ ## t,			\
		.transform_name = #t,					\
		.gl_shadow_fb = false,					\
		.meta.name = "pixman " #s " " #t,			\
	},								\
	{								\
		.renderer = WESTON_RENDERER_GL,				\
		.scale = s,						\
		.transform = WL_OUTPUT_TRANSFORM_ ## t,			\
		.transform_name = #t,					\
		.gl_shadow_fb = false,					\
		.meta.name = "GL no-shadow " #s " " #t,			\
	},								\
	{								\
		.renderer = WESTON_RENDERER_GL,				\
		.scale = s,						\
		.transform = WL_OUTPUT_TRANSFORM_ ## t,			\
		.transform_name = #t,					\
		.gl_shadow_fb = true,					\
		.meta.name = "GL shadow " #s " " #t,			\
	}

struct setup_args {
	struct fixture_metadata meta;
	enum weston_renderer_type renderer;
	int scale;
	enum wl_output_transform transform;
	const char *transform_name;
	bool gl_shadow_fb;
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

	/*
	 * The width and height are chosen to produce 324x240 framebuffer, to
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

	/*
	 * The test here works by swapping the whole wl_surface into a
	 * different color but lying that there is only a small damage area.
	 * Then the test checks that only the damage area gets the new color
	 * on screen.
	 *
	 * The following quirk forces GL-renderer to update the whole texture
	 * even for partial damage. Otherwise, GL-renderer would only copy the
	 * damaged area from the wl_shm buffer into a GL texture.
	 *
	 * Those output_damage tests where the surface is scaled up by the
	 * compositor will use bilinear texture sampling due to the policy
	 * in the renderers.
	 *
	 * Pixman renderer never makes copies of wl_shm buffers, so bilinear
	 * sampling there will always produce the expected result. However,
	 * with GL-renderer if the texture is not updated beyond the strict
	 * damage region, bilinear sampling will result in a blend of the old
	 * and new colors at the edges of the damage rectangles. This blend
	 * would be detrimental to testing the damage regions and would cause
	 * test failures due to reference image mismatch. What we actually
	 * want to see is the crisp outline of the damage rectangles.
	 */
	setup.test_quirks.gl_force_full_upload = true;

	if (arg->gl_shadow_fb) {
		/*
		 * A second case for GL-renderer: the shadow framebuffer
		 *
		 * This tests blit_shadow_to_output() specifically. The quirk
		 * forces the shadow framebuffer to be redrawn completely, which
		 * means the test surface will be completely filled with a new
		 * color regardless of damage. The blit uses damage too, and
		 * the damage pattern that is tested for needs to appear in
		 * that step.
		 *
		 * The quirk also ensures the shadow framebuffer is created
		 * even if not needed.
		 */
		setup.test_quirks.gl_force_full_redraw_of_shadow_fb = true;

		/* To skip instead of fail the test if shadow not available */
		setup.test_quirks.required_capabilities = WESTON_CAP_COLOR_OPS;
	}

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

static void
commit_buffer_with_damage(struct surface *surface,
			  struct buffer *buffer,
			  struct rectangle damage)
{
	wl_surface_attach(surface->wl_surface, buffer->proxy, 0, 0);
	wl_surface_damage(surface->wl_surface, damage.x, damage.y,
			  damage.width, damage.height);
	wl_surface_commit(surface->wl_surface);
}

/*
 * Test that Weston repaints exactly the damage a client sends to it.
 *
 * NOTE: This relies on the Weston implementation detail that Weston actually
 * will repaint exactly the client's damage and nothing more. This is not
 * generally true of Wayland compositors.
 */
TEST(output_damage)
{
#define COUNT_BUFS 3
	const struct setup_args *oargs;
	struct client *client;
	bool match = true;
	char *refname;
	int ret;
	struct buffer *buf[COUNT_BUFS];
	pixman_color_t colors[COUNT_BUFS];
	static const struct rectangle damages[COUNT_BUFS] = {
		{ 0 /* full damage */ },
		{ .x = 10, .y = 10, .width = 20, .height = 10 },
		{ .x = 43, .y = 47, .width = 5, .height = 50 },
	};
	int i;
	const int width = 140;
	const int height = 110;

	color_rgb888(&colors[0], 100, 100, 100); /* grey */
	color_rgb888(&colors[1],   0, 255, 255); /* cyan */
	color_rgb888(&colors[2],   0, 255,   0); /* green */

	oargs = &my_setup_args[get_test_fixture_index()];

	ret = asprintf(&refname, "output-damage_%d-%s",
		       oargs->scale, oargs->transform_name);
	assert(ret);

	testlog("%s: %s\n", get_test_name(), refname);

	client = create_client();
	client->surface = create_test_surface(client);
	client->surface->width = width;
	client->surface->height = height;

	for (i = 0; i < COUNT_BUFS; i++) {
		buf[i] = create_shm_buffer_a8r8g8b8(client, width, height);
		fill_image_with_color(buf[i]->image, &colors[i]);
	}

	client->surface->buffer = buf[0];
	move_client(client, 19, 19);

	/*
	 * Each time we commit a buffer with a different color, the damage box
	 * should color just the box on the output.
	 */
	for (i = 1; i < COUNT_BUFS; i++) {
		commit_buffer_with_damage(client->surface, buf[i], damages[i]);
		if (!verify_screen_content(client, refname, i, NULL, i, NULL))
			match = false;
	}

	assert(match);

	for (i = 0; i < COUNT_BUFS; i++)
		buffer_destroy(buf[i]);

	client->surface->buffer = NULL;
	client_destroy(client);
	free(refname);
}
