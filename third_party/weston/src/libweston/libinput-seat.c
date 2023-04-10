/*
 * Copyright © 2013 Intel Corporation
 * Copyright © 2013 Jonas Ådahl
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>

#include <libweston/libweston.h>
#include "backend.h"
#include "libweston-internal.h"
#include "weston-log-internal.h"
#include "launcher-util.h"
#include "libinput-seat.h"
#include "libinput-device.h"
#include "shared/helpers.h"

static void
process_events(struct udev_input *input);
static struct udev_seat *
udev_seat_create(struct udev_input *input, const char *seat_name);
static void
udev_seat_destroy(struct udev_seat *seat);

static struct udev_seat *
get_udev_seat(struct udev_input *input, struct libinput_device *device)
{
	struct libinput_seat *libinput_seat;
	const char *seat_name;

	libinput_seat = libinput_device_get_seat(device);
	seat_name = libinput_seat_get_logical_name(libinput_seat);
	return udev_seat_get_named(input, seat_name);
}

static struct weston_output *
output_find_by_head_name(struct weston_compositor *compositor,
			 const char *head_name)
{
	struct weston_output *output;
	struct weston_head *head;

	if (!head_name)
		return NULL;

	/* Only enabled outputs with connected heads.
	 * This means force-enabled outputs but with disconnected heads
	 * will be ignored; if the touchscreen doesn't have a video signal,
	 * touching it is meaningless.
	 */
	wl_list_for_each(output, &compositor->output_list, link) {
		wl_list_for_each(head, &output->head_list, output_link) {
			if (!weston_head_is_connected(head))
				continue;

			if (strcmp(head_name, head->name) == 0)
				return output;
		}
	}

	return NULL;
}

static void
device_added(struct udev_input *input, struct libinput_device *libinput_device)
{
	struct weston_compositor *c;
	struct evdev_device *device;
	struct weston_output *output;
	const char *output_name;
	struct weston_seat *seat;
	struct udev_seat *udev_seat;
	struct weston_pointer *pointer;

	c = input->compositor;

	udev_seat = get_udev_seat(input, libinput_device);
	if (!udev_seat)
		return;

	seat = &udev_seat->base;
	device = evdev_device_create(libinput_device, seat);
	if (device == NULL)
		return;

	if (input->configure_device != NULL)
		input->configure_device(c, device->device);
	evdev_device_set_calibration(device);
	udev_seat = (struct udev_seat *) seat;
	wl_list_insert(udev_seat->devices_list.prev, &device->link);

	pointer = weston_seat_get_pointer(seat);
	if (seat->output && pointer)
		weston_pointer_clamp(pointer,
				     &pointer->x,
				     &pointer->y);

	output_name = libinput_device_get_output_name(libinput_device);
	if (output_name) {
		device->output_name = strdup(output_name);
		output = output_find_by_head_name(c, output_name);
		evdev_device_set_output(device, output);
	} else if (!wl_list_empty(&c->output_list)) {
		/* default assignment to an arbitrary output */
		output = container_of(c->output_list.next,
				      struct weston_output, link);
		evdev_device_set_output(device, output);
	}

	if (!input->suspended)
		weston_seat_repick(seat);
}

static void
device_removed(struct udev_input *input, struct libinput_device *libinput_device)
{
	struct evdev_device *device;

	device = libinput_device_get_user_data(libinput_device);
	evdev_device_destroy(device);
}

static void
udev_seat_remove_devices(struct udev_seat *seat)
{
	struct evdev_device *device, *next;

	wl_list_for_each_safe(device, next, &seat->devices_list, link) {
		evdev_device_destroy(device);
	}
}

void
udev_input_disable(struct udev_input *input)
{
	if (input->suspended)
		return;

	wl_event_source_remove(input->libinput_source);
	input->libinput_source = NULL;
	libinput_suspend(input->libinput);
	process_events(input);
	input->suspended = 1;
}

static int
udev_input_process_event(struct libinput_event *event)
{
	struct libinput *libinput = libinput_event_get_context(event);
	struct libinput_device *libinput_device =
		libinput_event_get_device(event);
	struct udev_input *input = libinput_get_user_data(libinput);
	int handled = 1;

	switch (libinput_event_get_type(event)) {
	case LIBINPUT_EVENT_DEVICE_ADDED:
		device_added(input, libinput_device);
		break;
	case LIBINPUT_EVENT_DEVICE_REMOVED:
		device_removed(input, libinput_device);
		break;
	default:
		handled = 0;
	}

	return handled;
}

static void
process_event(struct libinput_event *event)
{
	if (udev_input_process_event(event))
		return;
	if (evdev_device_process_event(event))
		return;
}

static void
process_events(struct udev_input *input)
{
	struct libinput_event *event;

	while ((event = libinput_get_event(input->libinput))) {
		process_event(event);
		libinput_event_destroy(event);
	}
}

static int
udev_input_dispatch(struct udev_input *input)
{
	if (libinput_dispatch(input->libinput) != 0)
		weston_log("libinput: Failed to dispatch libinput\n");

	process_events(input);

	return 0;
}

static int
libinput_source_dispatch(int fd, uint32_t mask, void *data)
{
	struct udev_input *input = data;

	return udev_input_dispatch(input) != 0;
}

static int
open_restricted(const char *path, int flags, void *user_data)
{
	struct udev_input *input = user_data;
	struct weston_launcher *launcher = input->compositor->launcher;

	return weston_launcher_open(launcher, path, flags);
}

static void
close_restricted(int fd, void *user_data)
{
	struct udev_input *input = user_data;
	struct weston_launcher *launcher = input->compositor->launcher;

	weston_launcher_close(launcher, fd);
}

const struct libinput_interface libinput_interface = {
	open_restricted,
	close_restricted,
};

int
udev_input_enable(struct udev_input *input)
{
	struct wl_event_loop *loop;
	struct weston_compositor *c = input->compositor;
	int fd;
	struct udev_seat *seat;
	int devices_found = 0;

	loop = wl_display_get_event_loop(c->wl_display);
	fd = libinput_get_fd(input->libinput);
	input->libinput_source =
		wl_event_loop_add_fd(loop, fd, WL_EVENT_READABLE,
				     libinput_source_dispatch, input);
	if (!input->libinput_source) {
		return -1;
	}

	if (input->suspended) {
		if (libinput_resume(input->libinput) != 0) {
			wl_event_source_remove(input->libinput_source);
			input->libinput_source = NULL;
			return -1;
		}
		input->suspended = 0;
		process_events(input);
	}

	wl_list_for_each(seat, &input->compositor->seat_list, base.link) {
		evdev_notify_keyboard_focus(&seat->base, &seat->devices_list);

		if (!wl_list_empty(&seat->devices_list))
			devices_found = 1;
	}

	if (devices_found == 0 && !c->require_input) {
		weston_log("warning: no input devices found, but none required "
			   "as per configuration.\n");
		return 0;
	}

	if (devices_found == 0) {
		weston_log(
			"warning: no input devices on entering Weston. "
			"Possible causes:\n"
			"\t- no permissions to read /dev/input/event*\n"
			"\t- seats misconfigured "
			"(Weston backend option 'seat', "
			"udev device property ID_SEAT)\n");
		return -1;
	}

	return 0;
}

static void
libinput_log_func(struct libinput *libinput,
		  enum libinput_log_priority priority,
		  const char *format, va_list args)
{
	weston_vlog(format, args);
}

int
udev_input_init(struct udev_input *input, struct weston_compositor *c,
		struct udev *udev, const char *seat_id,
		udev_configure_device_t configure_device)
{
	enum libinput_log_priority priority = LIBINPUT_LOG_PRIORITY_INFO;
	const char *log_priority = NULL;

	memset(input, 0, sizeof *input);

	input->compositor = c;
	input->configure_device = configure_device;

	log_priority = getenv("WESTON_LIBINPUT_LOG_PRIORITY");

	input->libinput = libinput_udev_create_context(&libinput_interface,
						       input, udev);
	if (!input->libinput) {
		return -1;
	}

	libinput_log_set_handler(input->libinput, &libinput_log_func);

	if (log_priority) {
		if (strcmp(log_priority, "debug") == 0) {
			priority = LIBINPUT_LOG_PRIORITY_DEBUG;
		} else if (strcmp(log_priority, "info") == 0) {
			priority = LIBINPUT_LOG_PRIORITY_INFO;
		} else if (strcmp(log_priority, "error") == 0) {
			priority = LIBINPUT_LOG_PRIORITY_ERROR;
		}
	}

	libinput_log_set_priority(input->libinput, priority);

	if (libinput_udev_assign_seat(input->libinput, seat_id) != 0) {
		libinput_unref(input->libinput);
		return -1;
	}

	process_events(input);

	return udev_input_enable(input);
}

void
udev_input_destroy(struct udev_input *input)
{
	struct udev_seat *seat, *next;

	if (input->libinput_source)
		wl_event_source_remove(input->libinput_source);
	wl_list_for_each_safe(seat, next, &input->compositor->seat_list, base.link)
		udev_seat_destroy(seat);
	libinput_unref(input->libinput);
}

static void
udev_seat_led_update(struct weston_seat *seat_base, enum weston_led leds)
{
	struct udev_seat *seat = (struct udev_seat *) seat_base;
	struct evdev_device *device;

	wl_list_for_each(device, &seat->devices_list, link)
		evdev_led_update(device, leds);
}

static void
udev_seat_output_changed(struct udev_seat *seat, struct weston_output *output)
{
	struct evdev_device *device;
	struct weston_output *found;

	wl_list_for_each(device, &seat->devices_list, link) {
		/* If we find any input device without an associated output
		 * or an output name to associate with, just tie it with the
		 * output we got here - the default assignment.
		 */
		if (!device->output_name) {
			if (!device->output)
				evdev_device_set_output(device, output);

			continue;
		}

		/* Update all devices' output associations, may they gain or
		 * lose it.
		 */
		found = output_find_by_head_name(output->compositor,
						 device->output_name);
		evdev_device_set_output(device, found);
	}
}

static void
notify_output_create(struct wl_listener *listener, void *data)
{
	struct udev_seat *seat = container_of(listener, struct udev_seat,
					      output_create_listener);
	struct weston_output *output = data;

	udev_seat_output_changed(seat, output);
}

static void
notify_output_heads_changed(struct wl_listener *listener, void *data)
{
	struct udev_seat *seat = container_of(listener, struct udev_seat,
					      output_heads_listener);
	struct weston_output *output = data;

	udev_seat_output_changed(seat, output);
}

static struct udev_seat *
udev_seat_create(struct udev_input *input, const char *seat_name)
{
	struct weston_compositor *c = input->compositor;
	struct udev_seat *seat;

	seat = zalloc(sizeof *seat);
	if (!seat)
		return NULL;

	weston_seat_init(&seat->base, c, seat_name);
	seat->base.led_update = udev_seat_led_update;

	seat->output_create_listener.notify = notify_output_create;
	wl_signal_add(&c->output_created_signal,
		      &seat->output_create_listener);

	seat->output_heads_listener.notify = notify_output_heads_changed;
	wl_signal_add(&c->output_heads_changed_signal,
		      &seat->output_heads_listener);

	wl_list_init(&seat->devices_list);

	return seat;
}

static void
udev_seat_destroy(struct udev_seat *seat)
{
	struct weston_keyboard *keyboard =
		weston_seat_get_keyboard(&seat->base);

	if (keyboard)
		notify_keyboard_focus_out(&seat->base);

	udev_seat_remove_devices(seat);
	weston_seat_release(&seat->base);
	wl_list_remove(&seat->output_create_listener.link);
	wl_list_remove(&seat->output_heads_listener.link);
	free(seat);
}

struct udev_seat *
udev_seat_get_named(struct udev_input *input, const char *seat_name)
{
	struct udev_seat *seat;

	wl_list_for_each(seat, &input->compositor->seat_list, base.link) {
		if (strcmp(seat->base.seat_name, seat_name) == 0)
			return seat;
	}

	return udev_seat_create(input, seat_name);
}
