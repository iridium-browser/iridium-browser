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

#include <time.h>

#include "input-timestamps-helper.h"
#include "shared/timespec-util.h"
#include "weston-test-client-helper.h"
#include "wayland-server-protocol.h"
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

static const struct timespec t1 = { .tv_sec = 1, .tv_nsec = 1000001 };
static const struct timespec t2 = { .tv_sec = 2, .tv_nsec = 2000001 };
static const struct timespec t3 = { .tv_sec = 3, .tv_nsec = 3000001 };
static const struct timespec t_other = { .tv_sec = 123, .tv_nsec = 456 };

static struct client *
create_touch_test_client(void)
{
	struct client *cl = create_client_and_test_surface(0, 0, 100, 100);
	assert(cl);
	return cl;
}

static void
send_broken_touch(struct client *client, const struct timespec *time)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);

	weston_test_send_touch(client->test->weston_test, tv_sec_hi, tv_sec_lo,
			       tv_nsec, 1, 1, 1, WL_TOUCH_UP);

	expect_protocol_error(client, &weston_test_interface,
			      WESTON_TEST_ERROR_TOUCH_UP_WITH_COORDINATE);
}

static void
send_touch(struct client *client, const struct timespec *time,
	   uint32_t touch_type)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;
	wl_fixed_t x = 0, y = 0;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	if (touch_type != WL_TOUCH_UP) {
		x = 1;
		y = 1;
	}

	weston_test_send_touch(client->test->weston_test, tv_sec_hi, tv_sec_lo,
			       tv_nsec, 1, x, y, touch_type);
	client_roundtrip(client);
}

TEST(broken_touch_event)
{
	struct client *client = create_touch_test_client();
	struct input_timestamps *input_ts =
		input_timestamps_create_for_touch(client);

	send_broken_touch(client, &t1);

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(touch_events)
{
	struct client *client = create_touch_test_client();
	struct touch *touch = client->input->touch;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_touch(client);

	send_touch(client, &t1, WL_TOUCH_DOWN);
	assert(touch->down_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&touch->down_time_timespec, &t1));

	send_touch(client, &t2, WL_TOUCH_MOTION);
	assert(touch->motion_time_msec == timespec_to_msec(&t2));
	assert(timespec_eq(&touch->motion_time_timespec, &t2));

	send_touch(client, &t3, WL_TOUCH_UP);
	assert(touch->up_time_msec == timespec_to_msec(&t3));
	assert(timespec_eq(&touch->up_time_timespec, &t3));

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(touch_timestamps_stop_after_input_timestamps_object_is_destroyed)
{
	struct client *client = create_touch_test_client();
	struct touch *touch = client->input->touch;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_touch(client);

	send_touch(client, &t1, WL_TOUCH_DOWN);
	assert(touch->down_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&touch->down_time_timespec, &t1));

	input_timestamps_destroy(input_ts);

	send_touch(client, &t2, WL_TOUCH_UP);
	assert(touch->up_time_msec == timespec_to_msec(&t2));
	assert(timespec_is_zero(&touch->up_time_timespec));

	client_destroy(client);
}

TEST(touch_timestamps_stop_after_client_releases_wl_touch)
{
	struct client *client = create_touch_test_client();
	struct touch *touch = client->input->touch;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_touch(client);

	send_touch(client, &t1, WL_TOUCH_DOWN);
	assert(touch->down_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&touch->down_time_timespec, &t1));

	wl_touch_release(client->input->touch->wl_touch);

	/* Set input_timestamp to an arbitrary value (different from t1, t2
	 * and 0) and check that it is not changed by sending the event.
	 * This is preferred over just checking for 0, since 0 is used
	 * internally for resetting the timestamp after handling an input
	 * event and checking for it here may lead to false negatives. */
	touch->input_timestamp = t_other;
	send_touch(client, &t2, WL_TOUCH_UP);
	assert(timespec_eq(&touch->input_timestamp, &t_other));

	input_timestamps_destroy(input_ts);

	free(client->input->touch);
	client->input->touch = NULL;
	client_destroy(client);
}
