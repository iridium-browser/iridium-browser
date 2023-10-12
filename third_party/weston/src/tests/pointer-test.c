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

#include <linux/input.h>

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

static const struct timespec t0 = { .tv_sec = 0, .tv_nsec = 100000000 };
static const struct timespec t1 = { .tv_sec = 1, .tv_nsec = 1000001 };
static const struct timespec t2 = { .tv_sec = 2, .tv_nsec = 2000001 };
static const struct timespec t_other = { .tv_sec = 123, .tv_nsec = 456 };

static void
send_motion(struct client *client, const struct timespec *time, int x, int y)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	weston_test_move_pointer(client->test->weston_test, tv_sec_hi, tv_sec_lo,
				 tv_nsec, x, y);
	client_roundtrip(client);
}

static void
send_button(struct client *client, const struct timespec *time,
	    uint32_t button, uint32_t state)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	weston_test_send_button(client->test->weston_test, tv_sec_hi, tv_sec_lo,
				tv_nsec, button, state);
	client_roundtrip(client);
}

static void
send_axis(struct client *client, const struct timespec *time, uint32_t axis,
	  double value)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	weston_test_send_axis(client->test->weston_test, tv_sec_hi, tv_sec_lo,
			      tv_nsec, axis, wl_fixed_from_double(value));
	client_roundtrip(client);
}

static void
check_pointer(struct client *client, int x, int y)
{
	int sx, sy;

	/* check that the client got the global pointer update */
	assert(client->test->pointer_x == x);
	assert(client->test->pointer_y == y);

	/* Does global pointer map onto the surface? */
	if (surface_contains(client->surface, x, y)) {
		/* check that the surface has the pointer focus */
		assert(client->input->pointer->focus == client->surface);

		/*
		 * check that the local surface pointer maps
		 * to the global pointer.
		 */
		sx = client->input->pointer->x + client->surface->x;
		sy = client->input->pointer->y + client->surface->y;
		assert(sx == x);
		assert(sy == y);
	} else {
		/*
		 * The global pointer does not map onto surface.  So
		 * check that it doesn't have the pointer focus.
		 */
		assert(client->input->pointer->focus == NULL);
	}
}

static void
check_pointer_move(struct client *client, int x, int y)
{
	send_motion(client, &t0, x, y);
	check_pointer(client, x, y);
}

static struct client *
create_client_with_pointer_focus(int x, int y, int w, int h)
{
	struct client *cl = create_client_and_test_surface(x, y, w, h);
	assert(cl);
	/* Move the pointer inside the surface to ensure that the surface
	 * has the pointer focus. */
	check_pointer_move(cl, x, y);
	return cl;
}

TEST(test_pointer_top_left)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(46, 76, 111, 134);
	assert(client);

	/* move pointer outside top left */
	x = client->surface->x - 1;
	y = client->surface->y - 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on top left */
	x += 1; y += 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside top left */
	x -= 1; y -= 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_bottom_left)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(99, 100, 100, 98);
	assert(client);

	/* move pointer outside bottom left */
	x = client->surface->x - 1;
	y = client->surface->y + client->surface->height;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on bottom left */
	x += 1; y -= 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside bottom left */
	x -= 1; y += 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_top_right)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(48, 100, 67, 100);
	assert(client);

	/* move pointer outside top right */
	x = client->surface->x + client->surface->width;
	y = client->surface->y - 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on top right */
	x -= 1; y += 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside top right */
	x += 1; y -= 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_bottom_right)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(100, 123, 100, 69);
	assert(client);

	/* move pointer outside bottom right */
	x = client->surface->x + client->surface->width;
	y = client->surface->y + client->surface->height;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on bottom right */
	x -= 1; y -= 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside bottom right */
	x += 1; y += 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_top_center)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(100, 201, 100, 50);
	assert(client);

	/* move pointer outside top center */
	x = client->surface->x + client->surface->width/2;
	y = client->surface->y - 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on top center */
	y += 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside top center */
	y -= 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_bottom_center)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(100, 45, 67, 100);
	assert(client);

	/* move pointer outside bottom center */
	x = client->surface->x + client->surface->width/2;
	y = client->surface->y + client->surface->height;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on bottom center */
	y -= 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside bottom center */
	y += 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_left_center)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(167, 45, 78, 100);
	assert(client);

	/* move pointer outside left center */
	x = client->surface->x - 1;
	y = client->surface->y + client->surface->height/2;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on left center */
	x += 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside left center */
	x -= 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_right_center)
{
	struct client *client;
	int x, y;

	client = create_client_and_test_surface(110, 37, 100, 46);
	assert(client);

	/* move pointer outside right center */
	x = client->surface->x + client->surface->width;
	y = client->surface->y + client->surface->height/2;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer on right center */
	x -= 1;
	assert(surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	/* move pointer outside right center */
	x += 1;
	assert(!surface_contains(client->surface, x, y));
	check_pointer_move(client, x, y);

	client_destroy(client);
}

TEST(test_pointer_surface_move)
{
	struct client *client;

	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);

	/* move pointer outside of client */
	assert(!surface_contains(client->surface, 50, 50));
	check_pointer_move(client, 50, 50);

	/* move client center to pointer */
	move_client(client, 0, 0);
	assert(surface_contains(client->surface, 50, 50));
	check_pointer(client, 50, 50);

	client_destroy(client);
}

TEST(pointer_motion_events)
{
	struct client *client = create_client_with_pointer_focus(100, 100,
								 100, 100);
	struct pointer *pointer = client->input->pointer;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_pointer(client);

	send_motion(client, &t1, 150, 150);
	assert(pointer->x == 50);
	assert(pointer->y == 50);
	assert(pointer->motion_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&pointer->motion_time_timespec, &t1));

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(pointer_button_events)
{
	struct client *client = create_client_with_pointer_focus(100, 100,
								 100, 100);
	struct pointer *pointer = client->input->pointer;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_pointer(client);

	assert(pointer->button == 0);
	assert(pointer->state == 0);

	send_button(client, &t1, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
	assert(pointer->button == BTN_LEFT);
	assert(pointer->state == WL_POINTER_BUTTON_STATE_PRESSED);
	assert(pointer->button_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&pointer->button_time_timespec, &t1));

	send_button(client, &t2, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
	assert(pointer->button == BTN_LEFT);
	assert(pointer->state == WL_POINTER_BUTTON_STATE_RELEASED);
	assert(pointer->button_time_msec == timespec_to_msec(&t2));
	assert(timespec_eq(&pointer->button_time_timespec, &t2));

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(pointer_axis_events)
{
	struct client *client = create_client_with_pointer_focus(100, 100,
								 100, 100);
	struct pointer *pointer = client->input->pointer;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_pointer(client);

	send_axis(client, &t1, 1, 1.0);
	assert(pointer->axis == 1);
	assert(pointer->axis_value == 1.0);
	assert(pointer->axis_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&pointer->axis_time_timespec, &t1));

	send_axis(client, &t2, 2, 0.0);
	assert(pointer->axis == 2);
	assert(pointer->axis_stop_time_msec == timespec_to_msec(&t2));
	assert(timespec_eq(&pointer->axis_stop_time_timespec, &t2));

	input_timestamps_destroy(input_ts);

	client_destroy(client);
}

TEST(pointer_timestamps_stop_after_input_timestamps_object_is_destroyed)
{
	struct client *client = create_client_with_pointer_focus(100, 100,
								 100, 100);
	struct pointer *pointer = client->input->pointer;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_pointer(client);

	send_button(client, &t1, BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
	assert(pointer->button == BTN_LEFT);
	assert(pointer->state == WL_POINTER_BUTTON_STATE_PRESSED);
	assert(pointer->button_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&pointer->button_time_timespec, &t1));

	input_timestamps_destroy(input_ts);

	send_button(client, &t2, BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
	assert(pointer->button == BTN_LEFT);
	assert(pointer->state == WL_POINTER_BUTTON_STATE_RELEASED);
	assert(pointer->button_time_msec == timespec_to_msec(&t2));
	assert(timespec_is_zero(&pointer->button_time_timespec));

	client_destroy(client);
}

TEST(pointer_timestamps_stop_after_client_releases_wl_pointer)
{
	struct client *client = create_client_with_pointer_focus(100, 100,
								 100, 100);
	struct pointer *pointer = client->input->pointer;
	struct input_timestamps *input_ts =
		input_timestamps_create_for_pointer(client);

	send_motion(client, &t1, 150, 150);
	assert(pointer->x == 50);
	assert(pointer->y == 50);
	assert(pointer->motion_time_msec == timespec_to_msec(&t1));
	assert(timespec_eq(&pointer->motion_time_timespec, &t1));

	wl_pointer_release(client->input->pointer->wl_pointer);

	/* Set input_timestamp to an arbitrary value (different from t1, t2
	 * and 0) and check that it is not changed by sending the event.
	 * This is preferred over just checking for 0, since 0 is used
	 * internally for resetting the timestamp after handling an input
	 * event and checking for it here may lead to false negatives. */
	pointer->input_timestamp = t_other;
	send_motion(client, &t2, 175, 175);
	assert(timespec_eq(&pointer->input_timestamp, &t_other));

	input_timestamps_destroy(input_ts);

	free(client->input->pointer);
	client->input->pointer = NULL;
	client_destroy(client);
}
