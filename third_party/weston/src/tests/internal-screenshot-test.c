/*
 * Copyright © 2015 Samsung Electronics Co., Ltd
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

#include <stdint.h>
#include <stdio.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"
#include "image-iter.h"
#include "test-config.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = WESTON_RENDERER_PIXMAN;
	setup.width = 320;
	setup.height = 240;
	setup.shell = SHELL_DESKTOP;

	weston_ini_setup (&setup,
			  cfgln("[shell]"),
			  cfgln("startup-animation=%s", "none"),
			  cfgln("background-color=%s", "0xCC336699"));

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static void
draw_stuff(pixman_image_t *image)
{
	struct image_header ih = image_header_from(image);
	int x, y;
	uint32_t r, g, b;

	for (y = 0; y < ih.height; y++) {
		uint32_t *pixel = image_header_get_row_u32(&ih, y);

		for (x = 0; x < ih.width; x++, pixel++) {
			b = x;
			g = x + y;
			r = y;
			*pixel = (255U << 24) | (r << 16) | (g << 8) | b;
		}
	}
}

TEST(internal_screenshot)
{
	struct buffer *buf;
	struct client *client;
	struct wl_surface *surface;
	struct buffer *screenshot = NULL;
	pixman_image_t *reference_good = NULL;
	pixman_image_t *reference_bad = NULL;
	pixman_image_t *diffimg;
	struct rectangle clip;
	char *fname;
	bool match = false;
	bool dump_all_images = true;

	/* Create the client */
	testlog("Creating client for test\n");
	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);
	surface = client->surface->wl_surface;

	/*
	 * We are racing our screenshooting against weston-desktop-shell
	 * setting the cursor. If w-d-s wins, our screenshot will have a cursor
	 * shown, which makes the image comparison fail. Our window and the
	 * default pointer position are accidentally causing an overlap that
	 * intersects our test clip rectangle.
	 *
	 * w-d-s wins very rarely though, so the race is easy to miss. You can
	 * make it happen by putting a delay before the call to
	 * create_client_and_test_surface().
	 *
	 * The weston_test_move_pointer() below makes the race irrelevant, as
	 * the cursor won't overlap with anything we care about.
	 */

	/* Move the pointer away from the screenshot area. */
	weston_test_move_pointer(client->test->weston_test, 0, 1, 0, 0, 0);

	buf = create_shm_buffer_a8r8g8b8(client, 100, 100);
	draw_stuff(buf->image);
	wl_surface_attach(surface, buf->proxy, 0, 0);
	wl_surface_damage(surface, 0, 0, 100, 100);
	wl_surface_commit(surface);

	/* Take a snapshot.  Result will be in screenshot->wl_buffer. */
	testlog("Taking a screenshot\n");
	screenshot = capture_screenshot_of_output(client, NULL);
	assert(screenshot);

	/* Load good reference image */
	fname = screenshot_reference_filename("internal-screenshot-good", 0);
	testlog("Loading good reference image %s\n", fname);
	reference_good = load_image_from_png(fname);
	assert(reference_good);
	free(fname);

	/* Load bad reference image */
	fname = screenshot_reference_filename("internal-screenshot-bad", 0);
	testlog("Loading bad reference image %s\n", fname);
	reference_bad = load_image_from_png(fname);
	assert(reference_bad);
	free(fname);

	/* Test check_images_match() without a clip.
	 * We expect this to fail since we use a bad reference image
	 */
	match = check_images_match(screenshot->image, reference_bad, NULL, NULL);
	testlog("Screenshot %s reference image\n", match? "equal to" : "different from");
	assert(!match);
	pixman_image_unref(reference_bad);

	/* Test check_images_match() with clip.
	 * Alpha-blending and other effects can cause irrelevant discrepancies, so look only
	 * at a small portion of the solid-colored background
	 */
	clip.x = 100;
	clip.y = 100;
	clip.width = 100;
	clip.height = 100;
	testlog("Clip: %d,%d %d x %d\n", clip.x, clip.y, clip.width, clip.height);
	match = check_images_match(screenshot->image, reference_good, &clip, NULL);
	testlog("Screenshot %s reference image in clipped area\n", match? "matches" : "doesn't match");
	if (!match) {
		diffimg = visualize_image_difference(screenshot->image, reference_good, &clip, NULL);
		fname = screenshot_output_filename("internal-screenshot-error", 0);
		write_image_as_png(diffimg, fname);
		pixman_image_unref(diffimg);
		free(fname);
	}
	pixman_image_unref(reference_good);

	/* Test dumping of non-matching images */
	if (!match || dump_all_images) {
		fname = screenshot_output_filename("internal-screenshot", 0);
		write_image_as_png(screenshot->image, fname);
		free(fname);
	}

	buffer_destroy(screenshot);

	testlog("Test complete\n");
	assert(match);

	buffer_destroy(buf);
	client_destroy(client);
}
