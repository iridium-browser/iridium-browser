/*
 * Copyright © 2011, 2012 Intel Corporation
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

#ifndef _LIBINPUT_DEVICE_H_
#define _LIBINPUT_DEVICE_H_

#include "config.h"

#include <linux/input.h>
#include <wayland-util.h>
#include <libinput.h>
#include <stdbool.h>

#include <libweston/libweston.h>

enum evdev_device_seat_capability {
	EVDEV_SEAT_POINTER = (1 << 0),
	EVDEV_SEAT_KEYBOARD = (1 << 1),
	EVDEV_SEAT_TOUCH = (1 << 2)
};

struct evdev_device {
	struct weston_seat *seat;
	enum evdev_device_seat_capability seat_caps;
	struct libinput_device *device;
	struct weston_touch_device *touch_device;
	struct wl_list link;
	struct weston_output *output;
	struct wl_listener output_destroy_listener;
	char *output_name;
	int fd;
	bool override_wl_calibration;
};

void
evdev_led_update(struct evdev_device *device, enum weston_led leds);

struct evdev_device *
evdev_device_create(struct libinput_device *libinput_device,
		    struct weston_seat *seat);

int
evdev_device_process_event(struct libinput_event *event);

void
evdev_device_set_output(struct evdev_device *device,
			struct weston_output *output);
void
evdev_device_destroy(struct evdev_device *device);

void
evdev_notify_keyboard_focus(struct weston_seat *seat,
			    struct wl_list *evdev_devices);
void
evdev_device_set_calibration(struct evdev_device *device);

int
dispatch_libinput(struct libinput *libinput);

#endif /* _LIBINPUT_DEVICE_H_ */
