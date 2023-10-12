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
#include <math.h>
#include <unistd.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"
#include "single-pixel-buffer-v1-client-protocol.h"
#include "shared/os-compatibility.h"
#include "shared/xalloc.h"

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
fixture_setup(struct weston_test_harness *harness, const struct setup_args *arg)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.width = 320;
	setup.height = 240;
	setup.shell = SHELL_TEST_DESKTOP;
	setup.logging_scopes = "log,test-harness-plugin";

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

TEST(solid_buffer_argb_u32)
{
	struct client *client;
	struct wp_single_pixel_buffer_manager_v1 *mgr;
	struct wp_viewport *viewport;
	struct wl_buffer *buffer;
	int done;
	bool match;

	client = create_client();
	client->surface = create_test_surface(client);
	viewport = client_create_viewport(client);
	wp_viewport_set_destination(viewport, 128, 128);

	mgr = bind_to_singleton_global(client,
				       &wp_single_pixel_buffer_manager_v1_interface,
				       1);
	buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(mgr,
									  0xcfffffff, /* r */
									  0x8fffffff, /* g */
									  0x4fffffff, /* b */
									  0xffffffff /* a */);
	assert(buffer);

	weston_test_move_surface(client->test->weston_test,
				 client->surface->wl_surface,
				 64, 64);
	wl_surface_attach(client->surface->wl_surface, buffer, 0, 0);
	wl_surface_damage_buffer(client->surface->wl_surface, 0, 0, 1, 1);
	frame_callback_set(client->surface->wl_surface, &done);
	wl_surface_commit(client->surface->wl_surface);
	frame_callback_wait(client, &done);

	match = verify_screen_content(client, "single-pixel-buffer", 0, NULL, 0, NULL);
	assert(match);

	wl_buffer_destroy(buffer);
	wp_viewport_destroy(viewport);
	wp_single_pixel_buffer_manager_v1_destroy(mgr);
	client_destroy(client);
}
