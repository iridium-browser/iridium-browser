/*
 * Copyright © 2015 Red Hat, Inc.
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

/**
 * Test (un)plugging devices
 *
 * At the end of each test we must return Weston to the previous state
 * (add all removed devices and remove extra devices), so that
 * the environment is prepared for the other tests too
 */

#define WL_SEAT_CAPABILITY_ALL (WL_SEAT_CAPABILITY_KEYBOARD |\
				WL_SEAT_CAPABILITY_POINTER  |\
				WL_SEAT_CAPABILITY_TOUCH)

/* simply test if weston sends the right capabilities when
 * some devices are removed */
TEST(seat_capabilities_test)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);
	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	assert(cl->input->pointer);
	weston_test_device_release(cl->test->weston_test, "pointer");
	client_roundtrip(cl);
	assert(!cl->input->pointer);
	assert(!(cl->input->caps & WL_SEAT_CAPABILITY_POINTER));

	assert(cl->input->keyboard);
	weston_test_device_release(cl->test->weston_test, "keyboard");
	client_roundtrip(cl);
	assert(!cl->input->keyboard);
	assert(!(cl->input->caps & WL_SEAT_CAPABILITY_KEYBOARD));

	assert(cl->input->touch);
	weston_test_device_release(cl->test->weston_test, "touch");
	client_roundtrip(cl);
	assert(!cl->input->touch);
	assert(!(cl->input->caps & WL_SEAT_CAPABILITY_TOUCH));

	/* restore previous state */
	weston_test_device_add(cl->test->weston_test, "keyboard");
	weston_test_device_add(cl->test->weston_test, "pointer");
	weston_test_device_add(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	assert(cl->input->pointer);
	assert(cl->input->keyboard);
	assert(cl->input->touch);

	/* add extra devices */
	weston_test_device_add(cl->test->weston_test, "keyboard");
	weston_test_device_add(cl->test->weston_test, "pointer");
	weston_test_device_add(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	/* remove extra devices */
	weston_test_device_release(cl->test->weston_test, "keyboard");
	weston_test_device_release(cl->test->weston_test, "pointer");
	weston_test_device_release(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	/* we still should have all the capabilities, since the devices
	 * were doubled */
	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	assert(cl->input->pointer);
	assert(cl->input->keyboard);
	assert(cl->input->touch);

	client_destroy(cl);
}

#define COUNT 15
TEST(multiple_device_add_and_remove)
{
	int i;
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);

	/* add device a lot of times */
	for (i = 0; i < COUNT; ++i) {
		weston_test_device_add(cl->test->weston_test, "keyboard");
		weston_test_device_add(cl->test->weston_test, "pointer");
		weston_test_device_add(cl->test->weston_test, "touch");
	}

	client_roundtrip(cl);

	assert(cl->input->pointer);
	assert(cl->input->keyboard);
	assert(cl->input->touch);

	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	/* release all new devices */
	for (i = 0; i < COUNT; ++i) {
		weston_test_device_release(cl->test->weston_test, "keyboard");
		weston_test_device_release(cl->test->weston_test, "pointer");
		weston_test_device_release(cl->test->weston_test, "touch");
	}

	client_roundtrip(cl);

	/* there is still one from each device left */
	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	assert(cl->input->pointer);
	assert(cl->input->keyboard);
	assert(cl->input->touch);

	client_destroy(cl);
}

TEST(device_release_before_destroy)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);

	/* we can release pointer when we won't be using it anymore.
	 * Do it and see what happens if the device is destroyed
	 * right after that */
	wl_pointer_release(cl->input->pointer->wl_pointer);
	/* we must free and set to NULL the structures, otherwise
	 * seat capabilities will double-free them */
	free(cl->input->pointer);
	cl->input->pointer = NULL;

	wl_keyboard_release(cl->input->keyboard->wl_keyboard);
	free(cl->input->keyboard);
	cl->input->keyboard = NULL;

	wl_touch_release(cl->input->touch->wl_touch);
	free(cl->input->touch);
	cl->input->touch = NULL;

	weston_test_device_release(cl->test->weston_test, "pointer");
	weston_test_device_release(cl->test->weston_test, "keyboard");
	weston_test_device_release(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	assert(cl->input->caps == 0);

	/* restore previous state */
	weston_test_device_add(cl->test->weston_test, "pointer");
	weston_test_device_add(cl->test->weston_test, "keyboard");
	weston_test_device_add(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	client_destroy(cl);
}

TEST(device_release_before_destroy_multiple)
{
	int i;

	/* if weston crashed during this test, then there is
	 * some inconsistency */
	for (i = 0; i < 30; ++i) {
		device_release_before_destroy();
	}
}

/* normal work-flow test */
TEST(device_release_after_destroy)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);

	weston_test_device_release(cl->test->weston_test, "pointer");
	wl_pointer_release(cl->input->pointer->wl_pointer);
	/* we must free the memory manually, otherwise seat.capabilities
	 * will try to free it and will use invalid proxy */
	free(cl->input->pointer);
	cl->input->pointer = NULL;

	client_roundtrip(cl);

	weston_test_device_release(cl->test->weston_test, "keyboard");
	wl_keyboard_release(cl->input->keyboard->wl_keyboard);
	free(cl->input->keyboard);
	cl->input->keyboard = NULL;

	client_roundtrip(cl);

	weston_test_device_release(cl->test->weston_test, "touch");
	wl_touch_release(cl->input->touch->wl_touch);
	free(cl->input->touch);
	cl->input->touch = NULL;

	client_roundtrip(cl);

	assert(cl->input->caps == 0);

	/* restore previous state */
	weston_test_device_add(cl->test->weston_test, "pointer");
	weston_test_device_add(cl->test->weston_test, "keyboard");
	weston_test_device_add(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	client_destroy(cl);
}

TEST(device_release_after_destroy_multiple)
{
	int i;

	/* if weston crashed during this test, then there is
	 * some inconsistency */
	for (i = 0; i < 30; ++i) {
		device_release_after_destroy();
	}
}

/* see https://bugzilla.gnome.org/show_bug.cgi?id=745008
 * It is a mutter bug, but highly relevant. Weston does not
 * suffer from this bug atm, but it is worth of testing. */
TEST(get_device_after_destroy)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);
	struct wl_pointer *wl_pointer;
	struct wl_keyboard *wl_keyboard;
	struct wl_touch *wl_touch;

	/* There's a race:
	 *  1) compositor destroys device
	 *  2) client asks for the device, because it has not received
	 *     the new capabilities yet
	 *  3) compositor gets the request with a new_id for the
	 *     destroyed device
	 *  4) client uses the new_id
	 *  5) client gets new capabilities, destroying the objects
	 *
	 * If the compositor just bails out in step 3) and does not
	 * create the resource, then the client gets an error in step 4)
	 * - even though it followed the protocol (it just didn't know
	 * about new capabilities).
	 *
	 * This test simulates this situation
	 */

	/* connection is buffered, so after calling client_roundtrip(),
	 * this whole batch will be delivered to compositor and will
	 * exactly simulate our situation */
	weston_test_device_release(cl->test->weston_test, "pointer");
	wl_pointer = wl_seat_get_pointer(cl->input->wl_seat);
	assert(wl_pointer);

	/* this should be ignored */
	wl_pointer_set_cursor(wl_pointer, 0, NULL, 0, 0);

	/* this should not be ignored */
	wl_pointer_release(wl_pointer);
	client_roundtrip(cl);

	weston_test_device_release(cl->test->weston_test, "keyboard");
	wl_keyboard = wl_seat_get_keyboard(cl->input->wl_seat);
	assert(wl_keyboard);
	wl_keyboard_release(wl_keyboard);
	client_roundtrip(cl);

	weston_test_device_release(cl->test->weston_test, "touch");
	wl_touch = wl_seat_get_touch(cl->input->wl_seat);
	assert(wl_touch);
	wl_touch_release(wl_touch);
	client_roundtrip(cl);

	/* get weston to the previous state */
	weston_test_device_add(cl->test->weston_test, "pointer");
	weston_test_device_add(cl->test->weston_test, "keyboard");
	weston_test_device_add(cl->test->weston_test, "touch");
	client_roundtrip(cl);

	assert(cl->input->caps == WL_SEAT_CAPABILITY_ALL);

	client_destroy(cl);
}

TEST(get_device_after_destroy_multiple)
{
	int i;

	/* if weston crashed during this test, then there is
	 * some inconsistency */
	for (i = 0; i < 30; ++i) {
		get_device_after_destroy();
	}
}

TEST(seats_have_names)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);
	struct input *input;

	wl_list_for_each(input, &cl->inputs, link) {
		assert(input->seat_name);
	}

	client_destroy(cl);
}

TEST(seat_destroy_and_recreate)
{
	struct client *cl = create_client_and_test_surface(100, 100, 100, 100);

	weston_test_device_release(cl->test->weston_test, "seat");
	/* Roundtrip to receive and handle the seat global removal event */
	client_roundtrip(cl);

	assert(!cl->input);

	weston_test_device_add(cl->test->weston_test, "seat");
	/* First roundtrip to send request and receive new seat global */
	client_roundtrip(cl);
	/* Second roundtrip to handle seat events and set up input devices */
	client_roundtrip(cl);

	assert(cl->input);
	assert(cl->input->pointer);
	assert(cl->input->keyboard);
	assert(cl->input->touch);

	client_destroy(cl);
}
