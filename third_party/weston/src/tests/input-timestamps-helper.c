/*
 * Copyright © 2017 Collabora, Ltd.
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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "input-timestamps-helper.h"
#include "input-timestamps-unstable-v1-client-protocol.h"
#include "shared/timespec-util.h"
#include <libweston/zalloc.h>
#include "weston-test-client-helper.h"

struct input_timestamps {
	struct zwp_input_timestamps_v1 *proxy;
};

static struct zwp_input_timestamps_manager_v1 *
get_input_timestamps_manager(struct client *client)
{
	struct global *g;
	struct global *global_ts = NULL;
	struct zwp_input_timestamps_manager_v1 *ts = NULL;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, zwp_input_timestamps_manager_v1_interface.name))
			continue;

		if (global_ts)
			assert(!"Multiple input timestamp managers");

		global_ts = g;
	}

	assert(global_ts);
	assert(global_ts->version == 1);

	ts = wl_registry_bind(client->wl_registry, global_ts->name,
			      &zwp_input_timestamps_manager_v1_interface, 1);
	assert(ts);

	return ts;
}

static void
input_timestamp(void *data,
		struct zwp_input_timestamps_v1 *zwp_input_timestamps_v1,
		uint32_t tv_sec_hi,
		uint32_t tv_sec_lo,
		uint32_t tv_nsec)
{
	struct timespec *timestamp = data;

	timespec_from_proto(timestamp, tv_sec_hi, tv_sec_lo,
			    tv_nsec);

	testlog("test-client: got input timestamp %ld.%ld\n",
		timestamp->tv_sec, timestamp->tv_nsec);
}

static const struct zwp_input_timestamps_v1_listener
input_timestamps_listener = {
       .timestamp = input_timestamp,
};

struct input_timestamps *
input_timestamps_create_for_keyboard(struct client *client)
{
	struct zwp_input_timestamps_manager_v1 *manager =
		get_input_timestamps_manager(client);
	struct timespec *timestamp= &client->input->keyboard->input_timestamp;
	struct input_timestamps *input_ts;

	input_ts = zalloc(sizeof *input_ts);
	assert(input_ts);

	input_ts->proxy =
		zwp_input_timestamps_manager_v1_get_keyboard_timestamps(
			manager, client->input->keyboard->wl_keyboard);
	assert(input_ts->proxy);

	zwp_input_timestamps_v1_add_listener(input_ts->proxy,
					     &input_timestamps_listener,
					     timestamp);

	zwp_input_timestamps_manager_v1_destroy(manager);

	client_roundtrip(client);

	return input_ts;
}

struct input_timestamps *
input_timestamps_create_for_pointer(struct client *client)
{
	struct zwp_input_timestamps_manager_v1 *manager =
		get_input_timestamps_manager(client);
	struct timespec *timestamp= &client->input->pointer->input_timestamp;
	struct input_timestamps *input_ts;

	input_ts = zalloc(sizeof *input_ts);
	assert(input_ts);

	input_ts->proxy =
		zwp_input_timestamps_manager_v1_get_pointer_timestamps(
			manager, client->input->pointer->wl_pointer);
	assert(input_ts->proxy);

	zwp_input_timestamps_v1_add_listener(input_ts->proxy,
					     &input_timestamps_listener,
					     timestamp);

	zwp_input_timestamps_manager_v1_destroy(manager);

	client_roundtrip(client);

	return input_ts;
}

struct input_timestamps *
input_timestamps_create_for_touch(struct client *client)
{
	struct zwp_input_timestamps_manager_v1 *manager =
		get_input_timestamps_manager(client);
	struct timespec *timestamp= &client->input->touch->input_timestamp;
	struct input_timestamps *input_ts;

	input_ts = zalloc(sizeof *input_ts);
	assert(input_ts);

	input_ts->proxy =
		zwp_input_timestamps_manager_v1_get_touch_timestamps(
			manager, client->input->touch->wl_touch);
	assert(input_ts->proxy);

	zwp_input_timestamps_v1_add_listener(input_ts->proxy,
					     &input_timestamps_listener,
					     timestamp);

	zwp_input_timestamps_manager_v1_destroy(manager);

	client_roundtrip(client);

	return input_ts;
}

void
input_timestamps_destroy(struct input_timestamps *input_ts)
{
	zwp_input_timestamps_v1_destroy(input_ts->proxy);
	free(input_ts);
}
