/*
 * Copyright © 2018 Collabora, Ltd.
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

#include <string.h>
#include <unistd.h>

#include "linux-explicit-synchronization-unstable-v1-client-protocol.h"
#include "weston-test-client-helper.h"
#include "wayland-server-protocol.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);

	setup.shell = SHELL_TEST_DESKTOP;

	/* We need to use the pixman renderer, since a few of the tests depend
	 * on the renderer holding onto a surface buffer until the next one
	 * is committed, which the noop renderer doesn't do. */
	setup.renderer = WESTON_RENDERER_PIXMAN;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static struct zwp_linux_explicit_synchronization_v1 *
get_linux_explicit_synchronization(struct client *client)
{
	struct global *g;
	struct global *global_sync = NULL;
	struct zwp_linux_explicit_synchronization_v1 *sync = NULL;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface,
			   zwp_linux_explicit_synchronization_v1_interface.name))
			continue;

		if (global_sync)
			assert(!"Multiple linux explicit sync objects");

		global_sync = g;
	}

	assert(global_sync);
	assert(global_sync->version == 2);

	sync = wl_registry_bind(
			client->wl_registry, global_sync->name,
			&zwp_linux_explicit_synchronization_v1_interface, 2);
	assert(sync);

	return sync;
}

static struct client *
create_test_client(void)
{
	struct client *cl = create_client_and_test_surface(0, 0, 100, 100);
	assert(cl);
	return cl;
}

TEST(second_surface_synchronization_on_surface_raises_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync1;
	struct zwp_linux_surface_synchronization_v1 *surface_sync2;

	surface_sync1 =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	client_roundtrip(client);

	/* Second surface_synchronization creation should fail */
	surface_sync2 =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	expect_protocol_error(
		client,
		&zwp_linux_explicit_synchronization_v1_interface,
		ZWP_LINUX_EXPLICIT_SYNCHRONIZATION_V1_ERROR_SYNCHRONIZATION_EXISTS);

	zwp_linux_surface_synchronization_v1_destroy(surface_sync2);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync1);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(set_acquire_fence_with_invalid_fence_raises_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	int pipefd[2] = { -1, -1 };

	assert(pipe(pipefd) == 0);

	zwp_linux_surface_synchronization_v1_set_acquire_fence(surface_sync,
							       pipefd[0]);
	expect_protocol_error(
		client,
		&zwp_linux_surface_synchronization_v1_interface,
		ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_INVALID_FENCE);

	close(pipefd[0]);
	close(pipefd[1]);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(set_acquire_fence_on_destroyed_surface_raises_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	int pipefd[2] = { -1, -1 };

	assert(pipe(pipefd) == 0);

	wl_surface_destroy(client->surface->wl_surface);
	client->surface->wl_surface = NULL;
	zwp_linux_surface_synchronization_v1_set_acquire_fence(surface_sync,
							       pipefd[0]);
	expect_protocol_error(
		client,
		&zwp_linux_surface_synchronization_v1_interface,
		ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_SURFACE);

	close(pipefd[0]);
	close(pipefd[1]);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(second_buffer_release_in_commit_raises_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	struct zwp_linux_buffer_release_v1 *buffer_release1;
	struct zwp_linux_buffer_release_v1 *buffer_release2;

	buffer_release1 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	client_roundtrip(client);

	/* Second buffer_release creation should fail */
	buffer_release2 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	expect_protocol_error(
		client,
		&zwp_linux_surface_synchronization_v1_interface,
		ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_DUPLICATE_RELEASE);

	zwp_linux_buffer_release_v1_destroy(buffer_release2);
	zwp_linux_buffer_release_v1_destroy(buffer_release1);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(get_release_without_buffer_raises_commit_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	struct wl_surface *surface = client->surface->wl_surface;
	struct zwp_linux_buffer_release_v1 *buffer_release;

	buffer_release =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	wl_surface_commit(surface);
	expect_protocol_error(
		client,
		&zwp_linux_surface_synchronization_v1_interface,
		ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER);

	zwp_linux_buffer_release_v1_destroy(buffer_release);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(get_release_on_destroyed_surface_raises_error)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	struct zwp_linux_buffer_release_v1 *buffer_release;

	wl_surface_destroy(client->surface->wl_surface);
	client->surface->wl_surface = NULL;
	buffer_release =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	expect_protocol_error(
		client,
		&zwp_linux_surface_synchronization_v1_interface,
		ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_SURFACE);

	zwp_linux_buffer_release_v1_destroy(buffer_release);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(get_release_after_commit_succeeds)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct wl_surface *surface = client->surface->wl_surface;
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, surface);
	struct buffer *buf1 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct zwp_linux_buffer_release_v1 *buffer_release1;
	struct zwp_linux_buffer_release_v1 *buffer_release2;

	buffer_release1 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	client_roundtrip(client);

	wl_surface_attach(surface, buf1->proxy, 0, 0);
	wl_surface_commit(surface);

	buffer_release2 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	client_roundtrip(client);

	buffer_destroy(buf1);
	zwp_linux_buffer_release_v1_destroy(buffer_release2);
	zwp_linux_buffer_release_v1_destroy(buffer_release1);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

static void
buffer_release_fenced_handler(void *data,
			      struct zwp_linux_buffer_release_v1 *buffer_release,
			      int32_t fence)
{
	assert(!"Fenced release not supported yet");
}

static void
buffer_release_immediate_handler(void *data,
				 struct zwp_linux_buffer_release_v1 *buffer_release)
{
	int *released = data;

	*released += 1;
}

struct zwp_linux_buffer_release_v1_listener buffer_release_listener = {
	buffer_release_fenced_handler,
	buffer_release_immediate_handler
};

/* The following release event tests depend on the behavior of the used
 * backend, in this case the pixman backend. This doesn't limit their
 * usefulness, though, since it allows them to check if, given a typical
 * backend implementation, weston core supports the per commit nature of the
 * release events.
 */

TEST(get_release_events_are_emitted_for_different_buffers)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	struct buffer *buf1 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct buffer *buf2 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct wl_surface *surface = client->surface->wl_surface;
	struct zwp_linux_buffer_release_v1 *buffer_release1;
	struct zwp_linux_buffer_release_v1 *buffer_release2;
	int buf_released1 = 0;
	int buf_released2 = 0;
	int frame;

	buffer_release1 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	zwp_linux_buffer_release_v1_add_listener(buffer_release1,
						 &buffer_release_listener,
						 &buf_released1);
	wl_surface_attach(surface, buf1->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* No release event should have been emitted yet (we are using the
	 * pixman renderer, which holds buffers until they are replaced). */
	assert(buf_released1 == 0);

	buffer_release2 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	zwp_linux_buffer_release_v1_add_listener(buffer_release2,
						 &buffer_release_listener,
						 &buf_released2);
	wl_surface_attach(surface, buf2->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* Check that exactly one buffer_release event was emitted for the
	 * previous commit (buf1). */
	assert(buf_released1 == 1);
	assert(buf_released2 == 0);

	wl_surface_attach(surface, buf1->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* Check that exactly one buffer_release event was emitted for the
	 * previous commit (buf2). */
	assert(buf_released1 == 1);
	assert(buf_released2 == 1);

	buffer_destroy(buf2);
	buffer_destroy(buf1);
	zwp_linux_buffer_release_v1_destroy(buffer_release2);
	zwp_linux_buffer_release_v1_destroy(buffer_release1);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(get_release_events_are_emitted_for_same_buffer_on_surface)
{
	struct client *client = create_test_client();
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, client->surface->wl_surface);
	struct buffer *buf = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct wl_surface *surface = client->surface->wl_surface;
	struct zwp_linux_buffer_release_v1 *buffer_release1;
	struct zwp_linux_buffer_release_v1 *buffer_release2;
	int buf_released1 = 0;
	int buf_released2 = 0;
	int frame;

	buffer_release1 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	zwp_linux_buffer_release_v1_add_listener(buffer_release1,
					   &buffer_release_listener,
					   &buf_released1);
	wl_surface_attach(surface, buf->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* No release event should have been emitted yet (we are using the
	 * pixman renderer, which holds buffers until they are replaced). */
	assert(buf_released1 == 0);

	buffer_release2 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync);
	zwp_linux_buffer_release_v1_add_listener(buffer_release2,
					   &buffer_release_listener,
					   &buf_released2);
	wl_surface_attach(surface, buf->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* Check that exactly one buffer_release event was emitted for the
	 * previous commit (buf). */
	assert(buf_released1 == 1);
	assert(buf_released2 == 0);

	wl_surface_attach(surface, buf->proxy, 0, 0);
	frame_callback_set(surface, &frame);
	wl_surface_commit(surface);
	frame_callback_wait(client, &frame);
	/* Check that exactly one buffer_release event was emitted for the
	 * previous commit (buf again). */
	assert(buf_released1 == 1);
	assert(buf_released2 == 1);

	buffer_destroy(buf);
	zwp_linux_buffer_release_v1_destroy(buffer_release2);
	zwp_linux_buffer_release_v1_destroy(buffer_release1);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	client_destroy(client);
}

TEST(get_release_events_are_emitted_for_same_buffer_on_different_surfaces)
{
	struct client *client = create_test_client();
	struct surface *other_surface = create_test_surface(client);
	struct wl_surface *surface1 = client->surface->wl_surface;
	struct wl_surface *surface2 = other_surface->wl_surface;
	struct zwp_linux_explicit_synchronization_v1 *sync =
		get_linux_explicit_synchronization(client);
	struct zwp_linux_surface_synchronization_v1 *surface_sync1 =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, surface1);
	struct zwp_linux_surface_synchronization_v1 *surface_sync2 =
		zwp_linux_explicit_synchronization_v1_get_synchronization(
			sync, surface2);
	struct buffer *buf1 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct buffer *buf2 = create_shm_buffer_a8r8g8b8(client, 100, 100);
	struct zwp_linux_buffer_release_v1 *buffer_release1;
	struct zwp_linux_buffer_release_v1 *buffer_release2;
	int buf_released1 = 0;
	int buf_released2 = 0;
	int frame;

	weston_test_move_surface(client->test->weston_test, surface2, 0, 0);

	/* Attach buf1 to both surface1 and surface2. */
	buffer_release1 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync1);
	zwp_linux_buffer_release_v1_add_listener(buffer_release1,
					   &buffer_release_listener,
					   &buf_released1);
	wl_surface_attach(surface1, buf1->proxy, 0, 0);
	frame_callback_set(surface1, &frame);
	wl_surface_commit(surface1);
	frame_callback_wait(client, &frame);

	buffer_release2 =
		zwp_linux_surface_synchronization_v1_get_release(surface_sync2);
	zwp_linux_buffer_release_v1_add_listener(buffer_release2,
					   &buffer_release_listener,
					   &buf_released2);
	wl_surface_attach(surface2, buf1->proxy, 0, 0);
	frame_callback_set(surface2, &frame);
	wl_surface_commit(surface2);
	frame_callback_wait(client, &frame);

	assert(buf_released1 == 0);
	assert(buf_released2 == 0);

	/* Attach buf2 to surface1, and check that a buffer_release event for
	 * the previous commit (buf1) for that surface is emitted. */
	wl_surface_attach(surface1, buf2->proxy, 0, 0);
	frame_callback_set(surface1, &frame);
	wl_surface_commit(surface1);
	frame_callback_wait(client, &frame);

	assert(buf_released1 == 1);
	assert(buf_released2 == 0);

	/* Attach buf2 to surface2, and check that a buffer_release event for
	 * the previous commit (buf1) for that surface is emitted. */
	wl_surface_attach(surface2, buf2->proxy, 0, 0);
	frame_callback_set(surface2, &frame);
	wl_surface_commit(surface2);
	frame_callback_wait(client, &frame);

	assert(buf_released1 == 1);
	assert(buf_released2 == 1);

	buffer_destroy(buf2);
	buffer_destroy(buf1);
	zwp_linux_buffer_release_v1_destroy(buffer_release2);
	zwp_linux_buffer_release_v1_destroy(buffer_release1);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync2);
	zwp_linux_surface_synchronization_v1_destroy(surface_sync1);
	zwp_linux_explicit_synchronization_v1_destroy(sync);
	surface_destroy(other_surface);
	client_destroy(client);
}
