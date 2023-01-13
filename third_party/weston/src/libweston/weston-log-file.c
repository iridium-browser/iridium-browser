/*
 * Copyright © 2019 Collabora Ltd.
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

#include <stdio.h>

/** File type of stream
 */
struct weston_debug_log_file {
	struct weston_log_subscriber base;
	FILE *file;
};

static struct weston_debug_log_file *
to_weston_debug_log_file(struct weston_log_subscriber *sub)
{
	return container_of(sub, struct weston_debug_log_file, base);
}

static void
weston_log_file_write(struct weston_log_subscriber *sub,
		      const char *data, size_t len)
{
	struct weston_debug_log_file *stream = to_weston_debug_log_file(sub);
	fwrite(data, len, 1, stream->file);
}

static void
weston_log_subscriber_destroy_log(struct weston_log_subscriber *subscriber)
{
	struct weston_debug_log_file *file = to_weston_debug_log_file(subscriber);

	weston_log_subscriber_release(subscriber);
	free(file);
}

/** Creates a file type of subscriber
 *
 * Should be destroyed using weston_log_subscriber_destroy()
 *
 * @param dump_to if specified, used for writing data to
 * @returns a weston_log_subscriber object or NULL in case of failure
 *
 * @sa weston_log_subscriber_destroy
 *
 */
WL_EXPORT struct weston_log_subscriber *
weston_log_subscriber_create_log(FILE *dump_to)
{
	struct weston_debug_log_file *file = zalloc(sizeof(*file));

	if (!file)
		return NULL;

	if (dump_to)
		file->file = dump_to;
	else
		file->file = stderr;


	file->base.write = weston_log_file_write;
	file->base.destroy = weston_log_subscriber_destroy_log;
	file->base.destroy_subscription = NULL;
	file->base.complete = NULL;

	wl_list_init(&file->base.subscription_list);

	return &file->base;
}
