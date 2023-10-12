/*
 * Copyright © 2023 Collabora, Ltd.
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
#include "weston-output-capture-client-protocol.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.backend = WESTON_BACKEND_DRM;
	setup.renderer = WESTON_RENDERER_PIXMAN;
	setup.shell = SHELL_TEST_DESKTOP;

	weston_ini_setup(&setup,
			 cfgln("[shell]"),
			 cfgln("startup-animation=%s", "none"));

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static void
draw_stuff(pixman_image_t *image)
{
	int w, h;
	int stride; /* bytes */
	int x, y;
	uint32_t r, g, b;
	uint32_t *pixels;
	uint32_t *pixel;
	pixman_format_code_t fmt;

	fmt = pixman_image_get_format(image);
	w = pixman_image_get_width(image);
	h = pixman_image_get_height(image);
	stride = pixman_image_get_stride(image);
	pixels = pixman_image_get_data(image);

	assert(PIXMAN_FORMAT_BPP(fmt) == 32);

	for (x = 0; x < w; x++)
		for (y = 0; y < h; y++) {
			b = x;
			g = x + y;
			r = y;
			pixel = pixels + (y * stride / 4) + x;
			*pixel = (255U << 24) | (r << 16) | (g << 8) | b;
		}
}

TEST(drm_writeback_screenshot) {

	struct client *client;
	struct buffer *buffer;
	struct buffer *screenshot = NULL;
	pixman_image_t *reference = NULL;
	pixman_image_t *diffimg = NULL;
	struct wl_surface *surface;
	struct rectangle clip;
	const char *fname;
	bool match;
	int frame;

	/* create client */
	testlog("Creating client for test\n");
	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);
	surface = client->surface->wl_surface;

	/* move pointer away from image so it does not interfere with the
	 * comparison of the writeback screenshot with the reference image */
	weston_test_move_pointer(client->test->weston_test, 0, 1, 0, 0, 0);

	buffer = create_shm_buffer_a8r8g8b8(client, 100, 100);
	draw_stuff(buffer->image);

	wl_surface_attach(surface, buffer->proxy, 0, 0);
	wl_surface_damage(surface, 0, 0, 100, 100);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);

	/* take screenshot */
	testlog("Taking a screenshot\n");
	screenshot = client_capture_output(client, client->output,
                                           WESTON_CAPTURE_V1_SOURCE_WRITEBACK);
	assert(screenshot);

	/* take another screenshot; this is important to ensure the
	 * writeback state machine is working correctly */
	testlog("Taking another screenshot\n");
	screenshot = client_capture_output(client, client->output,
                                           WESTON_CAPTURE_V1_SOURCE_WRITEBACK);
	assert(screenshot);

	/* load reference image */
	fname = screenshot_reference_filename("drm-writeback-screenshot", 0);
	testlog("Loading good reference image %s\n", fname);
	reference = load_image_from_png(fname);
	assert(reference);

	/* check if they match - only the colored square matters, so the
	 * clip is used to ignore the background */
	clip.x = 100;
	clip.y = 100;
	clip.width = 100;
	clip.height = 100;
	match = check_images_match(screenshot->image, reference, &clip, NULL);
	testlog("Screenshot %s reference image\n", match? "equal to" : "different from");
	if (!match) {
		diffimg = visualize_image_difference(screenshot->image, reference, &clip, NULL);
		fname = screenshot_output_filename("drm-writeback-screenshot-error", 0);
		write_image_as_png(diffimg, fname);
		pixman_image_unref(diffimg);
	}

	pixman_image_unref(reference);
	buffer_destroy(screenshot);
	client_destroy(client);

	assert(match);
}