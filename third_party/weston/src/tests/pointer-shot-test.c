/*
 * Copyright 2021 Collabora Ltd
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

#include "shared/timespec-util.h"
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
fixture_setup(struct weston_test_harness *harness, const struct setup_args *arg)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.width = 320;
	setup.height = 240;
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

static const struct timespec t0 = { .tv_sec = 0, .tv_nsec = 0 };
static const struct timespec t1 = { .tv_sec = 1, .tv_nsec = 0 };
static const struct timespec t2 = { .tv_sec = 2, .tv_nsec = 0 };
static const struct timespec t3 = { .tv_sec = 3, .tv_nsec = 0 };

static void
send_motion(struct client *client, const struct timespec *time, int x, int y)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	weston_test_move_pointer(client->test->weston_test, tv_sec_hi, tv_sec_lo,
				 tv_nsec, x, y);
	client_roundtrip(client);
}

static struct buffer *
surface_commit_color(struct client *client, struct surface *surface,
		     pixman_color_t *color, int width, int height)
{
	struct buffer *buf;

	buf = create_shm_buffer_a8r8g8b8(client, width, height);
	fill_image_with_color(buf->image, color);
	wl_surface_attach(surface->wl_surface, buf->proxy, 0, 0);
	wl_surface_damage(surface->wl_surface, 0, 0, width, height);
	wl_surface_commit(surface->wl_surface);

	assert(!surface->buffer);
	surface->buffer = buf;

	return buf;
}

TEST(pointer_cursor_retains_committed_buffer_after_reenter)
{
	struct client *client;
	pixman_color_t red;
	pixman_color_t green;
	pixman_color_t gray;
	pixman_color_t magenta;
	bool match;
	struct surface *main_surface;
	struct surface *back_surface;
	struct surface *main_cursor_surface;
	struct surface *back_cursor_surface;

	color_rgb888(&red, 255, 0, 0);
	color_rgb888(&green, 0, 255, 0);
	color_rgb888(&gray, 127, 127, 127);
	color_rgb888(&magenta, 255, 0, 255);

	client = create_client();

	/* Move the cursor out of the way of the main surface */
	send_motion(client, &t0, 0, 0);

	/* Create all surfaces. */
	main_surface = create_test_surface(client);
	back_surface = create_test_surface(client);
	main_cursor_surface = create_test_surface(client);
	back_cursor_surface = create_test_surface(client);

	/* Commit buffers for cursors. */
	surface_commit_color(client, main_cursor_surface, &green, 25, 25);
	surface_commit_color(client, back_cursor_surface, &magenta, 25, 25);

	/* We need our own background surface so that we are able to change the cursor
	 * when the pointer leaves the main surface.
	 */
	weston_test_move_surface(client->test->weston_test, back_surface->wl_surface, 0, 0);
	surface_commit_color(client, back_surface, &gray, 320, 240);

	/* Set up the main surface. */
	client->surface = main_surface;
	client->surface->buffer = create_shm_buffer_a8r8g8b8(client, 100, 100);
	fill_image_with_color(client->surface->buffer->image, &red);
	move_client(client, 50, 50);

	/* Move the pointer into the main surface. */
	send_motion(client, &t1, 100, 100);
	wl_pointer_set_cursor(client->input->pointer->wl_pointer,
			      client->input->pointer->serial,
			      main_cursor_surface->wl_surface, 0, 0);
	match = verify_screen_content(client, "pointer_cursor_reenter", 0,
				      NULL, 0, NULL);
	assert(match);

	/* Move the cursor just outside the main surface. */
	send_motion(client, &t2, 150, 150);
	wl_pointer_set_cursor(client->input->pointer->wl_pointer,
			      client->input->pointer->serial,
			      back_cursor_surface->wl_surface, 0, 0);
	match = verify_screen_content(client, "pointer_cursor_reenter", 1,
				      NULL, 1, NULL);
	assert(match);

	/* And back in the main surface again. */
	send_motion(client, &t3, 149, 149);
	wl_pointer_set_cursor(client->input->pointer->wl_pointer,
			      client->input->pointer->serial,
			      main_cursor_surface->wl_surface, 0, 0);
	match = verify_screen_content(client, "pointer_cursor_reenter", 2,
				      NULL, 2, NULL);
	assert(match);

	surface_destroy(back_cursor_surface);
	surface_destroy(main_cursor_surface);
	surface_destroy(back_surface);
	/* main_surface is destroyed when destroying the client. */
	client_destroy(client);
}
