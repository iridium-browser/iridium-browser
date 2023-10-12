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

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_TEST_DESKTOP;
	setup.backend = WESTON_BACKEND_DRM;
	setup.renderer = WESTON_RENDERER_PIXMAN;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

TEST(drm_smoke) {
	struct client *client;
	struct buffer *buffer;
	struct wl_surface *surface;
	pixman_color_t red;
	int i, frame;

	color_rgb888(&red, 255, 0, 0);

	client = create_client_and_test_surface(0, 0, 200, 200);
	assert(client);

	surface = client->surface->wl_surface;
	buffer = create_shm_buffer_a8r8g8b8(client, 200, 200);

	fill_image_with_color(buffer->image, &red);

	for (i = 0; i < 5; i++) {
		wl_surface_attach(surface, buffer->proxy, 0, 0);
		wl_surface_damage(surface, 0, 0, 200, 200);
		frame_callback_set(surface, &frame);
		wl_surface_commit(surface);
		frame_callback_wait(client, &frame);
	}

	buffer_destroy(buffer);
	client_destroy(client);
}

TEST(drm_screenshot_no_damage) {
	struct client *client;
	int i;
	bool ret;

	client = create_client_and_test_surface(0, 0, 200, 200);
	assert(client);

	/*
	 * DRM-backend has an optimization to not even call the renderer if
	 * there is no damage to be repainted on the primary plane occupied by
	 * renderer's buffer. However, renderer must be called for a screenshot
	 * to complete.
	 *
	 * Therefore, if there is no damage, it is possible that screenshots
	 * might get stuck. This test makes sure they run regardless.
	 */
	for (i = 0; i < 5; i++) {
		ret = verify_screen_content(client, "drm_screenshot_no_damage",
					    0, NULL, i, "Virtual-1");
		assert(ret);
	}

	client_destroy(client);
}
