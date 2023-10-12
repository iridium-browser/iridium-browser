/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Collabora, Ltd.
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

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static int
output_contains_client(struct client *client)
{
	struct output *output = client->output;
	struct surface *surface = client->surface;

	return !(output->x >= surface->x + surface->width
		|| output->x + output->width <= surface->x
		|| output->y >= surface->y + surface->height
		|| output->y + output->height <= surface->y);
}

static void
check_client_move(struct client *client, int x, int y)
{
	move_client(client, x, y);

	if (output_contains_client(client)) {
		assert(client->surface->output == client->output);
	} else {
		assert(client->surface->output == NULL);
	}
}

TEST(test_surface_output)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);

	assert(output_contains_client(client));

	/* not visible */
	x = 0;
	y = -client->surface->height;
	check_client_move(client, x, y);

	/* visible */
	check_client_move(client, x, ++y);

	/* not visible */
	x = -client->surface->width;
	y = 0;
	check_client_move(client, x, y);

	/* visible */
	check_client_move(client, ++x, y);

	/* not visible */
	x = client->output->width;
	y = 0;
	check_client_move(client, x, y);

	/* visible */
	check_client_move(client, --x, y);
	assert(output_contains_client(client));

	/* not visible */
	x = 0;
	y = client->output->height;
	check_client_move(client, x, y);
	assert(!output_contains_client(client));

	/* visible */
	check_client_move(client, x, --y);
	assert(output_contains_client(client));

	client_destroy(client);
}

static void
buffer_release_handler(void *data, struct wl_buffer *buffer)
{
	int *released = data;

	*released = 1;
}

static struct wl_buffer_listener buffer_listener = {
	buffer_release_handler
};

TEST(buffer_release)
{
	struct client *client;
	struct wl_surface *surface;
	struct buffer *buf1;
	struct buffer *buf2;
	struct buffer *buf3;
	int buf1_released = 0;
	int buf2_released = 0;
	int buf3_released = 0;
	int frame;

	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);
	surface = client->surface->wl_surface;

	buf1 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	wl_buffer_add_listener(buf1->proxy, &buffer_listener, &buf1_released);

	buf2 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	wl_buffer_add_listener(buf2->proxy, &buffer_listener, &buf2_released);

	buf3 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	wl_buffer_add_listener(buf3->proxy, &buffer_listener, &buf3_released);

	/*
	 * buf1 must never be released, since it is replaced before
	 * it is committed, therefore it never becomes busy.
	 */

	wl_surface_attach(surface, buf1->proxy, 0, 0);
	wl_surface_attach(surface, buf2->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	assert(buf1_released == 0);
	/* buf2 may or may not be released */
	assert(buf3_released == 0);

	wl_surface_attach(surface, buf3->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	assert(buf1_released == 0);
	assert(buf2_released == 1);
	/* buf3 may or may not be released */

	wl_surface_attach(surface, client->surface->buffer->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	assert(buf1_released == 0);
	assert(buf2_released == 1);
	assert(buf3_released == 1);

	buffer_destroy(buf1);
	buffer_destroy(buf2);
	buffer_destroy(buf3);
	client_destroy(client);
}
