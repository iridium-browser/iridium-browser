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
#ifndef WESTON_LOG_INTERNAL_H
#define WESTON_LOG_INTERNAL_H

#include "wayland-util.h"

struct weston_log_subscription;

/** Subscriber allows each type of stream to customize or to provide its own
 * methods to manipulate the underlying storage. It follows also an
 * object-oriented approach, contains the ops callbacks and a list of
 * subcriptions of type weston_log_subscription. Each subscription created will
 * be both added to this subscription list and that of the weston_log_scope.
 *
 * A kind of stream can inherit the subscriber class and provide its own callbacks:
 * @code
 * struct weston_log_data_stream {
 * 	struct weston_log_subscriber base;
 * 	struct weston_data_stream opaque;
 * };
 * @endcode
 *
 * Passing the base class will require container retrieval type of methods
 * to be allowed to reach the opaque type (i.e., container_of()).
 *
 * @ingroup internal-log
 *
 */
struct weston_log_subscriber {
	/** write the data pointed by @param data */
	void (*write)(struct weston_log_subscriber *sub, const char *data, size_t len);
	/** For destroying the subscriber */
	void (*destroy)(struct weston_log_subscriber *sub);
	/** For the type of streams that required additional destroy operation
	 * for destroying the stream */
	void (*destroy_subscription)(struct weston_log_subscriber *sub);
	/** For the type of streams that can inform the 'consumer' part that
	 * write operation has been terminated/finished and should close the
	 * stream.
	 */
	void (*complete)(struct weston_log_subscriber *sub);
	struct wl_list subscription_list;       /**< weston_log_subscription::owner_link */
};

void
weston_log_subscription_create(struct weston_log_subscriber *owner,
			       struct weston_log_scope *scope);

void
weston_log_subscription_destroy(struct weston_log_subscription *sub);

void
weston_log_subscription_add(struct weston_log_scope *scope,
			    struct weston_log_subscription *sub);
void
weston_log_subscription_remove(struct weston_log_subscription *sub);

void
weston_log_subscriber_release(struct weston_log_subscriber *subscriber);

void
weston_log_bind_weston_debug(struct wl_client *client,
			     void *data, uint32_t version, uint32_t id);

struct weston_log_scope *
weston_log_get_scope(struct weston_log_context *log_ctx, const char *name);

void
weston_log_run_cb_new_subscription(struct weston_log_subscription *sub);

void
weston_debug_protocol_advertise_scopes(struct weston_log_context *log_ctx,
				       struct wl_resource *res);

int
weston_vlog(const char *fmt, va_list ap);
int
weston_vlog_continue(const char *fmt, va_list ap);

void *
weston_log_subscription_get_data(struct weston_log_subscription *sub);

void
weston_log_subscription_set_data(struct weston_log_subscription *sub, void *data);

void
weston_timeline_create_subscription(struct weston_log_subscription *sub,
				    void *user_data);

void
weston_timeline_destroy_subscription(struct weston_log_subscription *sub,
				     void *user_data);

#endif /* WESTON_LOG_INTERNAL_H */
