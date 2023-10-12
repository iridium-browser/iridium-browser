/*
 * Copyright © 2012 Intel Corporation
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

#include <stdint.h>

#include "input-timestamps-helper.h"
#include "shared/timespec-util.h"
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

static const struct timespec t1 = { .tv_sec = 1, .tv_nsec = 1000001 };
static const struct timespec t2 = { .tv_sec = 2, .tv_nsec = 2000001 };
static const struct timespec t_other = { .tv_sec = 123, .tv_nsec = 456 };

static struct client *
create_client_with_keyboard_focus(void)
{
	struct client *cl = create_client_and_test_surface(10, 10, 1, 1);
	assert(cl);

	weston_test_activate_surface(cl->test->weston_test,
				     cl->surface->wl_surface);
	client_roundtrip(cl);

	return cl;
}

static void
send_key(struct client *client, const struct timespec *time,
	 uint32_t key, uint32_t state)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	weston_test_send_key(client->test->weston_test, tv_sec_hi, tv_sec_lo,
			     tv_nsec, key, state);
	client_roundtrip(client);
}

TEST(simple_keyboard_test)
{
	struct client *client = create_client_with_keyboard_focus();
	struct keyboard *keyboard = client->input->keyboard;
	struct surface *expect_focus = client->surface;
	uint32_t expect_key = 0;
	uint32_t expect_state = 0;

	while (1) {
		assert(keyboard->key == expect_key);
		assert(keyboard->state == expect_state);
		assert(keyboard->focus == expect_focus);

		if (keyboard->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
			expect_state = WL_KEYBOARD_KEY_STATE_RELEASED;
			send_key(client, &t1, expect_key, expect_state);
		} else if (keyboard->focus) {
			expect_focus = NULL;
			weston_test_activate_surface(
				client->test->weston_test, NULL);
			client_roundtrip(client);
		} else if (expect_key < 10) {
			expect_key++;
			expect_focus = client->surface;
			expect_state = WL_KEYBOARD_KEY_STATE_PRESSED;
			weston_test_activate_surface(
				client->test->weston_test,
				expect_focus->wl_surface);
			send_key(client, &t1, expect_key, expect_state);
		} else {
			break;
		}
	}

	client_destroy(client);
}

TEST(keyboard_key_event_time)
{
	struct client *client = create_client_with_keyboard_focus();
	struct keyboard *keyboard = client->input->keyboard;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_keyboard(client);

	send_key(client, &t1, 1, WL_KEYBOARD_KEY_STATE_PRESSED);
	assert(keyboard->key_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&keyboard->key_time_timespec, &t1));

	send_key(client, &t2, 1, WL_KEYBOARD_KEY_STATE_RELEASED);
	assert(keyboard->key_time_msec == timespec_to_msec(&t2));
	assert(timespec_eq(&keyboard->key_time_timespec, &t2));

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(keyboard_timestamps_stop_after_input_timestamps_object_is_destroyed)
{
	struct client *client = create_client_with_keyboard_focus();
	struct keyboard *keyboard = client->input->keyboard;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_keyboard(client);

	send_key(client, &t1, 1, WL_KEYBOARD_KEY_STATE_PRESSED);
	assert(keyboard->key_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&keyboard->key_time_timespec, &t1));

	input_timestamps_destroy(input_ts);

	send_key(client, &t2, 1, WL_KEYBOARD_KEY_STATE_RELEASED);
	assert(keyboard->key_time_msec == timespec_to_msec(&t2));
	assert(timespec_is_zero(&keyboard->key_time_timespec));

	client_destroy(client);
}

TEST(keyboard_timestamps_stop_after_client_releases_wl_keyboard)
{
	struct client *client = create_client_with_keyboard_focus();
	struct keyboard *keyboard = client->input->keyboard;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_keyboard(client);

	send_key(client, &t1, 1, WL_KEYBOARD_KEY_STATE_PRESSED);
	assert(keyboard->key_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&keyboard->key_time_timespec, &t1));

	wl_keyboard_release(client->input->keyboard->wl_keyboard);

	/* Set input_timestamp to an arbitrary value (different from t1, t2
	 * and 0) and check that it is not changed by sending the event.
	 * This is preferred over just checking for 0, since 0 is used
	 * internally for resetting the timestamp after handling an input
	 * event and checking for it here may lead to false negatives. */
	keyboard->input_timestamp = t_other;
	send_key(client, &t2, 1, WL_KEYBOARD_KEY_STATE_RELEASED);
	assert(timespec_eq(&keyboard->input_timestamp, &t_other));

	input_timestamps_destroy(input_ts);

	free(client->input->keyboard);
	client->input->keyboard = NULL;
	client_destroy(client);
}
