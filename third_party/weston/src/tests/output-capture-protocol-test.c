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
#include "shared/xalloc.h"
#include "weston-output-capture-client-protocol.h"
#include "shared/weston-drm-fourcc.h"

struct setup_args {
	struct fixture_metadata meta;
	enum weston_renderer_type renderer;
	uint32_t expected_drm_format;
};

static const struct setup_args my_setup_args[] = {
	{
		.meta.name = "pixman",
		.renderer = WESTON_RENDERER_PIXMAN,
		.expected_drm_format = DRM_FORMAT_XRGB8888,
	},
	{
		.meta.name = "GL",
		.renderer = WESTON_RENDERER_GL,
		.expected_drm_format = DRM_FORMAT_ARGB8888,
	},
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness, const struct setup_args *arg)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.width = 100;
	setup.height = 60;
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

struct capturer {
	int width;
	int height;
	uint32_t drm_format;

	struct weston_capture_v1 *factory;
	struct weston_capture_source_v1 *source;

	enum {
		CAPTURE_TASK_PENDING = 0,
		CAPTURE_TASK_COMPLETE,
		CAPTURE_TASK_RETRY,
		CAPTURE_TASK_FAILED,
	} state;

	struct {
		bool size;
		bool format;
		bool reply;
	} events;

	char *last_failure;
};

static void
capture_source_handle_format(void *data,
			     struct weston_capture_source_v1 *proxy,
			     uint32_t drm_format)
{
	struct capturer *capt = data;

	assert(capt->source == proxy);

	capt->events.format = true;
	capt->drm_format = drm_format;
}

static void
capture_source_handle_size(void *data,
			   struct weston_capture_source_v1 *proxy,
			   int32_t width, int32_t height)
{
	struct capturer *capt = data;

	assert(capt->source == proxy);

	capt->events.size = true;
	capt->width = width;
	capt->height = height;
}

static void
capture_source_handle_complete(void *data,
			       struct weston_capture_source_v1 *proxy)
{
	struct capturer *capt = data;

	assert(capt->source == proxy);
	assert(capt->state == CAPTURE_TASK_PENDING);
	capt->state = CAPTURE_TASK_COMPLETE;
	capt->events.reply = true;
}

static void
capture_source_handle_retry(void *data,
			    struct weston_capture_source_v1 *proxy)
{
	struct capturer *capt = data;

	assert(capt->source == proxy);
	assert(capt->state == CAPTURE_TASK_PENDING);
	capt->state = CAPTURE_TASK_RETRY;
	capt->events.reply = true;
}

static void
capture_source_handle_failed(void *data,
			     struct weston_capture_source_v1 *proxy,
			     const char *msg)
{
	struct capturer *capt = data;

	assert(capt->source == proxy);
	assert(capt->state == CAPTURE_TASK_PENDING);
	capt->state = CAPTURE_TASK_FAILED;
	capt->events.reply = true;

	free(capt->last_failure);
	capt->last_failure = msg ? xstrdup(msg) : NULL;
}

static const struct weston_capture_source_v1_listener capture_source_handlers = {
	.format = capture_source_handle_format,
	.size = capture_source_handle_size,
	.complete = capture_source_handle_complete,
	.retry = capture_source_handle_retry,
	.failed = capture_source_handle_failed,
};

static struct capturer *
capturer_create(struct client *client,
		struct output *output,
		enum weston_capture_v1_source src)
{
	struct capturer *capt;

	capt = xzalloc(sizeof *capt);

	capt->factory = bind_to_singleton_global(client,
						 &weston_capture_v1_interface,
						 1);

	capt->source = weston_capture_v1_create(capt->factory,
						output->wl_output, src);
	weston_capture_source_v1_add_listener(capt->source,
					      &capture_source_handlers, capt);

	return capt;
}

static void
capturer_destroy(struct capturer *capt)
{
	weston_capture_source_v1_destroy(capt->source);
	weston_capture_v1_destroy(capt->factory);
	free(capt->last_failure);
	free(capt);
}

/*
 * Use the guaranteed source and all the right parameters to check that
 * shooting succeeds on the first try.
 */
TEST(simple_shot)
{
	const struct setup_args *fix = &my_setup_args[get_test_fixture_index()];
	struct client *client;
	struct capturer *capt;
	struct buffer *buf;

	client = create_client();
	capt = capturer_create(client, client->output,
			       WESTON_CAPTURE_V1_SOURCE_FRAMEBUFFER);
	client_roundtrip(client);

	assert(capt->events.format);
	assert(capt->events.size);
	assert(capt->state == CAPTURE_TASK_PENDING);
	assert(capt->drm_format == fix->expected_drm_format);
	assert(capt->width > 0);
	assert(capt->height > 0);
	assert(!capt->events.reply);

	buf = create_shm_buffer(client, capt->width, capt->height,
				fix->expected_drm_format);

	weston_capture_source_v1_capture(capt->source, buf->proxy);
	while (!capt->events.reply)
		assert(wl_display_dispatch(client->wl_display) >= 0);

	assert(capt->state == CAPTURE_TASK_COMPLETE);

	capturer_destroy(capt);
	buffer_destroy(buf);
	client_destroy(client);
}

/*
 * Use a guaranteed source, but use an unsupported pixel format.
 * This should always cause a retry.
 */
TEST(retry_on_wrong_format)
{
	const uint32_t drm_format = DRM_FORMAT_ABGR2101010;
	struct client *client;
	struct capturer *capt;
	struct buffer *buf;

	client = create_client();
	capt = capturer_create(client, client->output,
			       WESTON_CAPTURE_V1_SOURCE_FRAMEBUFFER);
	client_roundtrip(client);

	assert(capt->events.format);
	assert(capt->events.size);
	assert(capt->state == CAPTURE_TASK_PENDING);
	assert(capt->drm_format != drm_format && "fix this test");
	assert(capt->width > 0);
	assert(capt->height > 0);
	assert(!capt->events.reply);

	buf = create_shm_buffer(client, capt->width, capt->height, drm_format);

	weston_capture_source_v1_capture(capt->source, buf->proxy);
	while (!capt->events.reply)
		assert(wl_display_dispatch(client->wl_display) >= 0);

	assert(capt->state == CAPTURE_TASK_RETRY);

	capturer_destroy(capt);
	buffer_destroy(buf);
	client_destroy(client);
}

/*
 * Use a guaranteed source, but use a smaller buffer size.
 * This should always cause a retry.
 */
TEST(retry_on_wrong_size)
{
	struct client *client;
	struct capturer *capt;
	struct buffer *buf;

	client = create_client();
	capt = capturer_create(client, client->output,
			       WESTON_CAPTURE_V1_SOURCE_FRAMEBUFFER);
	client_roundtrip(client);

	assert(capt->events.format);
	assert(capt->events.size);
	assert(capt->state == CAPTURE_TASK_PENDING);
	assert(capt->width > 5);
	assert(capt->height > 5);
	assert(!capt->events.reply);

	buf = create_shm_buffer(client, capt->width - 3, capt->height - 3,
				capt->drm_format);

	weston_capture_source_v1_capture(capt->source, buf->proxy);
	while (!capt->events.reply)
		assert(wl_display_dispatch(client->wl_display) >= 0);

	assert(capt->state == CAPTURE_TASK_RETRY);

	capturer_destroy(capt);
	buffer_destroy(buf);
	client_destroy(client);
}

/*
 * Try a source that is guaranteed to not exist, and check that
 * capturing fails.
 */
TEST(writeback_on_headless_fails)
{
	struct client *client;
	struct capturer *capt;
	struct buffer *buf;

	client = create_client();
	buf = create_shm_buffer_a8r8g8b8(client, 5, 5);
	capt = capturer_create(client, client->output,
			       WESTON_CAPTURE_V1_SOURCE_WRITEBACK);
	client_roundtrip(client);

	assert(!capt->events.format);
	assert(!capt->events.size);
	assert(capt->state == CAPTURE_TASK_PENDING);

	/* Trying pixel source that is not available should fail immediately */
	weston_capture_source_v1_capture(capt->source, buf->proxy);
	client_roundtrip(client);

	assert(!capt->events.format);
	assert(!capt->events.size);
	assert(capt->state == CAPTURE_TASK_FAILED);
	assert(strcmp(capt->last_failure, "source unavailable") == 0);

	capturer_destroy(capt);
	buffer_destroy(buf);
	client_destroy(client);
}
