/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2013 Jonas Ådahl
 * Copyright 2017-2018 Collabora, Ltd.
 * Copyright 2017-2018 General Electric Company
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

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <libinput.h>

#include <libweston/libweston.h>
#include "backend.h"
#include "libweston-internal.h"
#include "libinput-device.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

void
evdev_led_update(struct evdev_device *device, enum weston_led weston_leds)
{
	enum libinput_led leds = 0;

	if (weston_leds & LED_NUM_LOCK)
		leds |= LIBINPUT_LED_NUM_LOCK;
	if (weston_leds & LED_CAPS_LOCK)
		leds |= LIBINPUT_LED_CAPS_LOCK;
	if (weston_leds & LED_SCROLL_LOCK)
		leds |= LIBINPUT_LED_SCROLL_LOCK;

	libinput_device_led_update(device->device, leds);
}

static void
handle_keyboard_key(struct libinput_device *libinput_device,
		    struct libinput_event_keyboard *keyboard_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	int key_state =
		libinput_event_keyboard_get_key_state(keyboard_event);
	int seat_key_count =
		libinput_event_keyboard_get_seat_key_count(keyboard_event);
	struct timespec time;

	/* Ignore key events that are not seat wide state changes. */
	if ((key_state == LIBINPUT_KEY_STATE_PRESSED &&
	     seat_key_count != 1) ||
	    (key_state == LIBINPUT_KEY_STATE_RELEASED &&
	     seat_key_count != 0))
		return;

	timespec_from_usec(&time,
			   libinput_event_keyboard_get_time_usec(keyboard_event));

	notify_key(device->seat, &time,
		   libinput_event_keyboard_get_key(keyboard_event),
		   key_state, STATE_UPDATE_AUTOMATIC);
}

static bool
handle_pointer_motion(struct libinput_device *libinput_device,
		      struct libinput_event_pointer *pointer_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	struct weston_pointer_motion_event event = { 0 };
	struct timespec time;
	double dx_unaccel, dy_unaccel;

	timespec_from_usec(&time,
			   libinput_event_pointer_get_time_usec(pointer_event));
	dx_unaccel = libinput_event_pointer_get_dx_unaccelerated(pointer_event);
	dy_unaccel = libinput_event_pointer_get_dy_unaccelerated(pointer_event);

	event = (struct weston_pointer_motion_event) {
		.mask = WESTON_POINTER_MOTION_REL |
			WESTON_POINTER_MOTION_REL_UNACCEL,
		.time = time,
		.dx = libinput_event_pointer_get_dx(pointer_event),
		.dy = libinput_event_pointer_get_dy(pointer_event),
		.dx_unaccel = dx_unaccel,
		.dy_unaccel = dy_unaccel,
	};

	notify_motion(device->seat, &time, &event);

	return true;
}

static bool
handle_pointer_motion_absolute(
	struct libinput_device *libinput_device,
	struct libinput_event_pointer *pointer_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	struct weston_output *output = device->output;
	struct timespec time;
	double x, y;
	uint32_t width, height;

	if (!output)
		return false;

	timespec_from_usec(&time,
			   libinput_event_pointer_get_time_usec(pointer_event));
	width = device->output->current_mode->width;
	height = device->output->current_mode->height;

	x = libinput_event_pointer_get_absolute_x_transformed(pointer_event,
							      width);
	y = libinput_event_pointer_get_absolute_y_transformed(pointer_event,
							      height);

	weston_output_transform_coordinate(device->output, x, y, &x, &y);
	notify_motion_absolute(device->seat, &time, x, y);

	return true;
}

static bool
handle_pointer_button(struct libinput_device *libinput_device,
		      struct libinput_event_pointer *pointer_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	int button_state =
		libinput_event_pointer_get_button_state(pointer_event);
	int seat_button_count =
		libinput_event_pointer_get_seat_button_count(pointer_event);
	struct timespec time;

	/* Ignore button events that are not seat wide state changes. */
	if ((button_state == LIBINPUT_BUTTON_STATE_PRESSED &&
	     seat_button_count != 1) ||
	    (button_state == LIBINPUT_BUTTON_STATE_RELEASED &&
	     seat_button_count != 0))
		return false;

	timespec_from_usec(&time,
			   libinput_event_pointer_get_time_usec(pointer_event));

	notify_button(device->seat, &time,
		      libinput_event_pointer_get_button(pointer_event),
                      button_state);

	return true;
}

static double
normalize_scroll(struct libinput_event_pointer *pointer_event,
		 enum libinput_pointer_axis axis)
{
	enum libinput_pointer_axis_source source;
	double value = 0.0;

	source = libinput_event_pointer_get_axis_source(pointer_event);
	/* libinput < 0.8 sent wheel click events with value 10. Since 0.8
	   the value is the angle of the click in degrees. To keep
	   backwards-compat with existing clients, we just send multiples of
	   the click count.
	 */
	switch (source) {
	case LIBINPUT_POINTER_AXIS_SOURCE_WHEEL:
		value = 10 * libinput_event_pointer_get_axis_value_discrete(
								   pointer_event,
								   axis);
		break;
	case LIBINPUT_POINTER_AXIS_SOURCE_FINGER:
	case LIBINPUT_POINTER_AXIS_SOURCE_CONTINUOUS:
		value = libinput_event_pointer_get_axis_value(pointer_event,
							      axis);
		break;
	default:
		assert(!"unhandled event source in normalize_scroll");
	}

	return value;
}

static int32_t
get_axis_discrete(struct libinput_event_pointer *pointer_event,
		  enum libinput_pointer_axis axis)
{
	enum libinput_pointer_axis_source source;

	source = libinput_event_pointer_get_axis_source(pointer_event);

	if (source != LIBINPUT_POINTER_AXIS_SOURCE_WHEEL)
		return 0;

	return libinput_event_pointer_get_axis_value_discrete(pointer_event,
							      axis);
}

static bool
handle_pointer_axis(struct libinput_device *libinput_device,
		    struct libinput_event_pointer *pointer_event)
{
	static int warned;
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	double vert, horiz;
	int32_t vert_discrete, horiz_discrete;
	enum libinput_pointer_axis axis;
	struct weston_pointer_axis_event weston_event;
	enum libinput_pointer_axis_source source;
	uint32_t wl_axis_source;
	bool has_vert, has_horiz;
	struct timespec time;

	has_vert = libinput_event_pointer_has_axis(pointer_event,
				   LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
	has_horiz = libinput_event_pointer_has_axis(pointer_event,
				   LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);

	if (!has_vert && !has_horiz)
		return false;

	source = libinput_event_pointer_get_axis_source(pointer_event);
	switch (source) {
	case LIBINPUT_POINTER_AXIS_SOURCE_WHEEL:
		wl_axis_source = WL_POINTER_AXIS_SOURCE_WHEEL;
		break;
	case LIBINPUT_POINTER_AXIS_SOURCE_FINGER:
		wl_axis_source = WL_POINTER_AXIS_SOURCE_FINGER;
		break;
	case LIBINPUT_POINTER_AXIS_SOURCE_CONTINUOUS:
		wl_axis_source = WL_POINTER_AXIS_SOURCE_CONTINUOUS;
		break;
	default:
		if (warned < 5) {
			weston_log("Unknown scroll source %d.\n", source);
			warned++;
		}
		return false;
	}

	notify_axis_source(device->seat, wl_axis_source);

	timespec_from_usec(&time,
			   libinput_event_pointer_get_time_usec(pointer_event));

	if (has_vert) {
		axis = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
		vert_discrete = get_axis_discrete(pointer_event, axis);
		vert = normalize_scroll(pointer_event, axis);

		weston_event.axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
		weston_event.value = vert;
		weston_event.discrete = vert_discrete;
		weston_event.has_discrete = (vert_discrete != 0);

		notify_axis(device->seat, &time, &weston_event);
	}

	if (has_horiz) {
		axis = LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL;
		horiz_discrete = get_axis_discrete(pointer_event, axis);
		horiz = normalize_scroll(pointer_event, axis);

		weston_event.axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
		weston_event.value = horiz;
		weston_event.discrete = horiz_discrete;
		weston_event.has_discrete = (horiz_discrete != 0);

		notify_axis(device->seat, &time, &weston_event);
	}

	return true;
}

static struct weston_output *
touch_get_output(struct weston_touch_device *device)
{
	struct evdev_device *evdev_device = device->backend_data;

	return evdev_device->output;
}

static const char *
touch_get_calibration_head_name(struct weston_touch_device *device)
{
	struct evdev_device *evdev_device = device->backend_data;
	struct weston_output *output = evdev_device->output;
	struct weston_head *head;

	if (!output)
		return NULL;

	assert(output->enabled);
	if (evdev_device->output_name)
		return evdev_device->output_name;

	/* No specific head was configured, so the association was made by
	 * the default rule. Just grab whatever head's name.
	 */
	wl_list_for_each(head, &output->head_list, output_link)
		return head->name;

	assert(0);
	return NULL;
}

static void
touch_get_calibration(struct weston_touch_device *device,
		      struct weston_touch_device_matrix *cal)
{
	struct evdev_device *evdev_device = device->backend_data;

	libinput_device_config_calibration_get_matrix(evdev_device->device,
						      cal->m);
}

static void
do_set_calibration(struct evdev_device *evdev_device,
		   const struct weston_touch_device_matrix *cal)
{
	enum libinput_config_status status;

	weston_log("input device %s: applying calibration:\n",
		   libinput_device_get_sysname(evdev_device->device));
	weston_log_continue(STAMP_SPACE "  %f %f %f\n",
			    cal->m[0], cal->m[1], cal->m[2]);
	weston_log_continue(STAMP_SPACE "  %f %f %f\n",
			    cal->m[3], cal->m[4], cal->m[5]);

	status = libinput_device_config_calibration_set_matrix(evdev_device->device,
							       cal->m);
	if (status != LIBINPUT_CONFIG_STATUS_SUCCESS)
		weston_log("Error: Failed to apply calibration.\n");
}

static void
touch_set_calibration(struct weston_touch_device *device,
		      const struct weston_touch_device_matrix *cal)
{
	struct evdev_device *evdev_device = device->backend_data;

	/* Stop output hotplug from reloading the WL_CALIBRATION values.
	 * libinput will maintain the latest calibration for us.
	 */
	evdev_device->override_wl_calibration = true;

	do_set_calibration(evdev_device, cal);
}

static const struct weston_touch_device_ops touch_calibration_ops = {
	.get_output = touch_get_output,
	.get_calibration_head_name = touch_get_calibration_head_name,
	.get_calibration = touch_get_calibration,
	.set_calibration = touch_set_calibration
};

static struct weston_touch_device *
create_touch_device(struct evdev_device *device)
{
	const struct weston_touch_device_ops *ops = NULL;
	struct weston_touch_device *touch_device;
	struct udev_device *udev_device;

	if (libinput_device_config_calibration_has_matrix(device->device))
		ops = &touch_calibration_ops;

	udev_device = libinput_device_get_udev_device(device->device);
	if (!udev_device)
		return NULL;

	touch_device = weston_touch_create_touch_device(device->seat->touch_state,
					udev_device_get_syspath(udev_device),
					device, ops);

	udev_device_unref(udev_device);

	if (!touch_device)
		return NULL;

	weston_log("Touchscreen - %s - %s\n",
		   libinput_device_get_name(device->device),
		   touch_device->syspath);

	return touch_device;
}

static void
handle_touch_with_coords(struct libinput_device *libinput_device,
			 struct libinput_event_touch *touch_event,
			 int touch_type)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	double x;
	double y;
	struct weston_point2d_device_normalized norm;
	uint32_t width, height;
	struct timespec time;
	int32_t slot;

	if (!device->output)
		return;

	timespec_from_usec(&time,
			   libinput_event_touch_get_time_usec(touch_event));
	slot = libinput_event_touch_get_seat_slot(touch_event);

	width = device->output->current_mode->width;
	height = device->output->current_mode->height;
	x =  libinput_event_touch_get_x_transformed(touch_event, width);
	y =  libinput_event_touch_get_y_transformed(touch_event, height);

	weston_output_transform_coordinate(device->output,
					   x, y, &x, &y);

	if (weston_touch_device_can_calibrate(device->touch_device)) {
		norm.x = libinput_event_touch_get_x_transformed(touch_event, 1);
		norm.y = libinput_event_touch_get_y_transformed(touch_event, 1);
		notify_touch_normalized(device->touch_device, &time, slot,
					x, y, &norm, touch_type);
	} else {
		notify_touch(device->touch_device, &time, slot, x, y, touch_type);
	}
}

static void
handle_touch_down(struct libinput_device *device,
		  struct libinput_event_touch *touch_event)
{
	handle_touch_with_coords(device, touch_event, WL_TOUCH_DOWN);
}

static void
handle_touch_motion(struct libinput_device *device,
		    struct libinput_event_touch *touch_event)
{
	handle_touch_with_coords(device, touch_event, WL_TOUCH_MOTION);
}

static void
handle_touch_up(struct libinput_device *libinput_device,
		struct libinput_event_touch *touch_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	struct timespec time;
	int32_t slot = libinput_event_touch_get_seat_slot(touch_event);

	timespec_from_usec(&time,
			   libinput_event_touch_get_time_usec(touch_event));

	notify_touch(device->touch_device, &time, slot, 0, 0, WL_TOUCH_UP);
}

static void
handle_touch_frame(struct libinput_device *libinput_device,
		   struct libinput_event_touch *touch_event)
{
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);

	notify_touch_frame(device->touch_device);
}

int
evdev_device_process_event(struct libinput_event *event)
{
	struct libinput_device *libinput_device =
		libinput_event_get_device(event);
	struct evdev_device *device =
		libinput_device_get_user_data(libinput_device);
	int handled = 1;
	bool need_frame = false;

	switch (libinput_event_get_type(event)) {
	case LIBINPUT_EVENT_KEYBOARD_KEY:
		handle_keyboard_key(libinput_device,
				    libinput_event_get_keyboard_event(event));
		break;
	case LIBINPUT_EVENT_POINTER_MOTION:
		need_frame = handle_pointer_motion(libinput_device,
				      libinput_event_get_pointer_event(event));
		break;
	case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
		need_frame = handle_pointer_motion_absolute(
				libinput_device,
				libinput_event_get_pointer_event(event));
		break;
	case LIBINPUT_EVENT_POINTER_BUTTON:
		need_frame = handle_pointer_button(libinput_device,
				      libinput_event_get_pointer_event(event));
		break;
	case LIBINPUT_EVENT_POINTER_AXIS:
		need_frame = handle_pointer_axis(
				 libinput_device,
				 libinput_event_get_pointer_event(event));
		break;
	case LIBINPUT_EVENT_TOUCH_DOWN:
		handle_touch_down(libinput_device,
				  libinput_event_get_touch_event(event));
		break;
	case LIBINPUT_EVENT_TOUCH_MOTION:
		handle_touch_motion(libinput_device,
				    libinput_event_get_touch_event(event));
		break;
	case LIBINPUT_EVENT_TOUCH_UP:
		handle_touch_up(libinput_device,
				libinput_event_get_touch_event(event));
		break;
	case LIBINPUT_EVENT_TOUCH_FRAME:
		handle_touch_frame(libinput_device,
				   libinput_event_get_touch_event(event));
		break;
	default:
		handled = 0;
		weston_log("unknown libinput event %d\n",
			   libinput_event_get_type(event));
	}

	if (need_frame)
		notify_pointer_frame(device->seat);

	return handled;
}

static void
notify_output_destroy(struct wl_listener *listener, void *data)
{
	struct evdev_device *device =
		container_of(listener,
			     struct evdev_device, output_destroy_listener);

	evdev_device_set_output(device, NULL);
}

/**
 * The WL_CALIBRATION property requires a pixel-specific matrix to be
 * applied after scaling device coordinates to screen coordinates. libinput
 * can't do that, so we need to convert the calibration to the normalized
 * format libinput expects.
 */
void
evdev_device_set_calibration(struct evdev_device *device)
{
	struct udev *udev;
	struct udev_device *udev_device = NULL;
	const char *sysname = libinput_device_get_sysname(device->device);
	const char *calibration_values;
	uint32_t width, height;
	struct weston_touch_device_matrix calibration;

	if (!libinput_device_config_calibration_has_matrix(device->device))
		return;

	/* If LIBINPUT_CALIBRATION_MATRIX was set to non-identity, we will not
	 * override it with WL_CALIBRATION. It also means we don't need an
	 * output to load a calibration. */
	if (libinput_device_config_calibration_get_default_matrix(
							  device->device,
							  calibration.m) != 0)
		return;

	/* touch_set_calibration() has updated the values, do not load old
	 * values from WL_CALIBRATION.
	 */
	if (device->override_wl_calibration)
		return;

	if (!device->output) {
		weston_log("input device %s has no enabled output associated "
			   "(%s named), skipping calibration for now.\n",
			   sysname, device->output_name ?: "none");
		return;
	}

	width = device->output->width;
	height = device->output->height;
	if (width == 0 || height == 0)
		return;

	udev = udev_new();
	if (!udev)
		return;

	udev_device = udev_device_new_from_subsystem_sysname(udev,
							     "input",
							     sysname);
	if (!udev_device)
		goto out;

	calibration_values =
		udev_device_get_property_value(udev_device,
					       "WL_CALIBRATION");

	if (calibration_values) {
		weston_log("Warning: input device %s has WL_CALIBRATION property set. "
			   "Support for it will be removed in the future. "
			   "Please use LIBINPUT_CALIBRATION_MATRIX instead.\n",
			   sysname);
	}

	if (!calibration_values || sscanf(calibration_values,
					  "%f %f %f %f %f %f",
					  &calibration.m[0],
					  &calibration.m[1],
					  &calibration.m[2],
					  &calibration.m[3],
					  &calibration.m[4],
					  &calibration.m[5]) != 6)
		goto out;

	/* normalize to a format libinput can use. There is a chance of
	   this being wrong if the width/height don't match the device
	   width/height but I'm not sure how to fix that */
	calibration.m[2] /= width;
	calibration.m[5] /= height;

	do_set_calibration(device, &calibration);

	weston_log_continue(STAMP_SPACE "  raw translation %f %f for output %s\n",
		   calibration.m[2] * width,
		   calibration.m[5] * height,
		   device->output->name);

out:
	if (udev_device)
		udev_device_unref(udev_device);
	udev_unref(udev);
}

void
evdev_device_set_output(struct evdev_device *device,
			struct weston_output *output)
{
	if (device->output == output)
		return;

	if (device->output_destroy_listener.notify) {
		wl_list_remove(&device->output_destroy_listener.link);
		device->output_destroy_listener.notify = NULL;
	}

	if (!output) {
		weston_log("output for input device %s removed\n",
			   libinput_device_get_sysname(device->device));

		device->output = NULL;
		return;
	}

	weston_log("associating input device %s with output %s "
		   "(%s by udev)\n",
		   libinput_device_get_sysname(device->device),
		   output->name,
		   device->output_name ?: "none");

	device->output = output;
	device->output_destroy_listener.notify = notify_output_destroy;
	wl_signal_add(&output->destroy_signal,
		      &device->output_destroy_listener);
	evdev_device_set_calibration(device);
}

struct evdev_device *
evdev_device_create(struct libinput_device *libinput_device,
		    struct weston_seat *seat)
{
	struct evdev_device *device;

	device = zalloc(sizeof *device);
	if (device == NULL)
		return NULL;

	device->seat = seat;
	wl_list_init(&device->link);
	device->device = libinput_device;

	if (libinput_device_has_capability(libinput_device,
					   LIBINPUT_DEVICE_CAP_KEYBOARD)) {
		weston_seat_init_keyboard(seat, NULL);
		device->seat_caps |= EVDEV_SEAT_KEYBOARD;
	}
	if (libinput_device_has_capability(libinput_device,
					   LIBINPUT_DEVICE_CAP_POINTER)) {
		weston_seat_init_pointer(seat);
		device->seat_caps |= EVDEV_SEAT_POINTER;
	}
	if (libinput_device_has_capability(libinput_device,
					   LIBINPUT_DEVICE_CAP_TOUCH)) {
		weston_seat_init_touch(seat);
		device->seat_caps |= EVDEV_SEAT_TOUCH;
		device->touch_device = create_touch_device(device);
	}

	libinput_device_set_user_data(libinput_device, device);
	libinput_device_ref(libinput_device);

	return device;
}

void
evdev_device_destroy(struct evdev_device *device)
{
	if (device->seat_caps & EVDEV_SEAT_POINTER)
		weston_seat_release_pointer(device->seat);
	if (device->seat_caps & EVDEV_SEAT_KEYBOARD)
		weston_seat_release_keyboard(device->seat);
	if (device->seat_caps & EVDEV_SEAT_TOUCH) {
		weston_touch_device_destroy(device->touch_device);
		weston_seat_release_touch(device->seat);
	}

	if (device->output)
		wl_list_remove(&device->output_destroy_listener.link);
	wl_list_remove(&device->link);
	libinput_device_unref(device->device);
	free(device->output_name);
	free(device);
}

void
evdev_notify_keyboard_focus(struct weston_seat *seat,
			    struct wl_list *evdev_devices)
{
	struct wl_array keys;

	if (seat->keyboard_device_count == 0)
		return;

	wl_array_init(&keys);
	notify_keyboard_focus_in(seat, &keys, STATE_UPDATE_AUTOMATIC);
	wl_array_release(&keys);
}
