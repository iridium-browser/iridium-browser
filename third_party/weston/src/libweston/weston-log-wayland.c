/*
 * Copyright © 2019 Collabora Ltd
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

#include <libweston/weston-log.h>
#include "shared/helpers.h"
#include <libweston/libweston.h>

#include "weston-log-internal.h"
#include "weston-debug-server-protocol.h"

#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

/** A debug stream created by a client
 *
 * A client provides a file descriptor for the server to write debug messages
 * into. A weston_log_debug_wayland is associated to one weston_log_scope via the
 * scope name, and the scope provides the messages.  There can be several
 * streams for the same scope, all streams getting the same messages.
 *
 * The following is specific to weston-debug protocol.
 * Subscription/unsubscription takes place in the stream_create(), respectively
 * in stream_destroy().
 */
struct weston_log_debug_wayland {
	struct weston_log_subscriber base;
	int fd;				/**< client provided fd */
	struct wl_resource *resource;	/**< weston_debug_stream_v1 object */
};

static struct weston_log_debug_wayland *
to_weston_log_debug_wayland(struct weston_log_subscriber *sub)
{
        return container_of(sub, struct weston_log_debug_wayland, base);
}

static void
stream_close_unlink(struct weston_log_debug_wayland *stream)
{
	if (stream->fd != -1)
		close(stream->fd);
	stream->fd = -1;
}

static void WL_PRINTF(2, 3)
stream_close_on_failure(struct weston_log_debug_wayland *stream,
			const char *fmt, ...)
{
	char *msg;
	va_list ap;
	int ret;

	stream_close_unlink(stream);

	va_start(ap, fmt);
	ret = vasprintf(&msg, fmt, ap);
	va_end(ap);

	if (ret > 0) {
		weston_debug_stream_v1_send_failure(stream->resource, msg);
		free(msg);
	} else {
		weston_debug_stream_v1_send_failure(stream->resource,
						    "MEMFAIL");
	}
}

/** Write data into a specific debug stream
 *
 * \param sub The subscriber's stream to write into; must not be NULL.
 * \param[in] data Pointer to the data to write.
 * \param len Number of bytes to write.
 *
 * Writes the given data (binary verbatim) into the debug stream.
 * If \c len is zero or negative, the write is silently dropped.
 *
 * Writing is continued until all data has been written or
 * a write fails. If the write fails due to a signal, it is re-tried.
 * Otherwise on failure, the stream is closed and
 * \c weston_debug_stream_v1.failure event is sent to the client.
 *
 * \memberof weston_log_debug_wayland
 */
static void
weston_log_debug_wayland_write(struct weston_log_subscriber *sub,
			       const char *data, size_t len)
{
	ssize_t len_ = len;
	ssize_t ret;
	int e;
	struct weston_log_debug_wayland *stream = to_weston_log_debug_wayland(sub);

	if (stream->fd == -1)
		return;

	while (len_ > 0) {
		ret = write(stream->fd, data, len_);
		e = errno;
		if (ret < 0) {
			if (e == EINTR)
				continue;

			stream_close_on_failure(stream,
					"Error writing %zd bytes: %s (%d)",
					len_, strerror(e), e);
			break;
		}

		len_ -= ret;
		data += ret;
	}
}

/** Close the debug stream and send success event
 *
 * \param sub Subscriber's stream to close.
 *
 * Closes the debug stream and sends \c weston_debug_stream_v1.complete
 * event to the client. This tells the client the debug information dump
 * is complete.
 *
 * \memberof weston_log_debug_wayland
 */
static void
weston_log_debug_wayland_complete(struct weston_log_subscriber *sub)
{
	struct weston_log_debug_wayland *stream = to_weston_log_debug_wayland(sub);

	stream_close_unlink(stream);
	weston_debug_stream_v1_send_complete(stream->resource);
}

static void
weston_log_debug_wayland_to_destroy(struct weston_log_subscriber *sub)
{
	struct weston_log_debug_wayland *stream = to_weston_log_debug_wayland(sub);

	if (stream->fd != -1)
		stream_close_on_failure(stream, "debug name removed");
}

static struct weston_log_debug_wayland *
stream_create(struct weston_log_context *log_ctx, const char *name,
	      int32_t streamfd, struct wl_resource *stream_resource)
{
	struct weston_log_debug_wayland *stream;
	struct weston_log_scope *scope;

	stream = zalloc(sizeof *stream);
	if (!stream)
		return NULL;

	stream->fd = streamfd;
	stream->resource = stream_resource;

	stream->base.write = weston_log_debug_wayland_write;
	stream->base.destroy = NULL;
	stream->base.destroy_subscription = weston_log_debug_wayland_to_destroy;
	stream->base.complete = weston_log_debug_wayland_complete;
	wl_list_init(&stream->base.subscription_list);

	scope = weston_log_get_scope(log_ctx, name);
	if (scope) {
		weston_log_subscription_create(&stream->base, scope);
	} else {
		stream_close_on_failure(stream,
					"Debug stream name '%s' is unknown.",
					name);
	}

	return stream;
}

static void
stream_destroy(struct wl_resource *stream_resource)
{
	struct weston_log_debug_wayland *stream;
	stream = wl_resource_get_user_data(stream_resource);

	stream_close_unlink(stream);
	weston_log_subscriber_release(&stream->base);
	free(stream);
}

static void
weston_debug_stream_destroy(struct wl_client *client,
			    struct wl_resource *stream_resource)
{
	wl_resource_destroy(stream_resource);
}

static const struct weston_debug_stream_v1_interface
						weston_debug_stream_impl = {
	weston_debug_stream_destroy
};

static void
weston_debug_destroy(struct wl_client *client,
		     struct wl_resource *global_resource)
{
	wl_resource_destroy(global_resource);
}

static void
weston_debug_subscribe(struct wl_client *client,
		       struct wl_resource *global_resource,
		       const char *name,
		       int32_t streamfd,
		       uint32_t new_stream_id)
{
	struct weston_log_context *log_ctx;
	struct wl_resource *stream_resource;
	uint32_t version;
	struct weston_log_debug_wayland *stream;

	log_ctx = wl_resource_get_user_data(global_resource);
	version = wl_resource_get_version(global_resource);

	stream_resource = wl_resource_create(client,
					&weston_debug_stream_v1_interface,
					version, new_stream_id);
	if (!stream_resource)
		goto fail;

	stream = stream_create(log_ctx, name, streamfd, stream_resource);
	if (!stream)
		goto fail;

	wl_resource_set_implementation(stream_resource,
				       &weston_debug_stream_impl,
				       stream, stream_destroy);
	return;

fail:
	close(streamfd);
	wl_client_post_no_memory(client);
}

static const struct weston_debug_v1_interface weston_debug_impl = {
	weston_debug_destroy,
	weston_debug_subscribe
};

void
weston_log_bind_weston_debug(struct wl_client *client,
			     void *data, uint32_t version, uint32_t id)
{
	struct weston_log_context *log_ctx = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &weston_debug_v1_interface,
				      version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &weston_debug_impl,
				       log_ctx, NULL);

	weston_debug_protocol_advertise_scopes(log_ctx, resource);
}
