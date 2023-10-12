/*
 * Copyright 2022 Collabora, Ltd.
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
	setup.renderer = WESTON_RENDERER_GL;
	setup.width = 300;
	setup.height = 150;
	setup.shell = SHELL_TEST_DESKTOP;

	weston_ini_setup(&setup,
			 cfgln("[core]"),
			 cfgln("output-decorations=true"));

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

/*
 * Basic screenshot test for output decorations
 *
 * Tests that the cairo-util code for drawing window decorations works at all
 * through headless-backend. The window decorations are normally used as output
 * decorations by wayland-backend when the outputs are windows in a parent
 * compositor.
 *
 * This works only with GL-renderer. Pixman-renderer has no code for blitting
 * output decorations and does not even know they exist.
 *
 * Headless-backend sets window title string to NULL because it might be
 * difficult to ensure text rendering is pixel-precise between different
 * systems.
 */
TEST(output_decorations)
{
	struct client *client;
	struct buffer *shot;
	pixman_image_t *img;
	bool match;

	client = create_client();

	shot = client_capture_output(client, client->output,
				     WESTON_CAPTURE_V1_SOURCE_FULL_FRAMEBUFFER);
	img = image_convert_to_a8r8g8b8(shot->image);

	match = verify_image(img, "output-decorations", 0, NULL, 0);
	assert(match);

	pixman_image_unref(img);
	buffer_destroy(shot);
	client_destroy(client);
}
