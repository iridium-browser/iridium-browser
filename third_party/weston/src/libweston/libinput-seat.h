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

#ifndef _LIBINPUT_SEAT_H_
#define _LIBINPUT_SEAT_H_

#include "config.h"

#include <libudev.h>

#include <libweston/libweston.h>

struct libinput_device;

struct udev_seat {
	struct weston_seat base;
	struct wl_list devices_list;
	struct wl_listener output_create_listener;
	struct wl_listener output_heads_listener;
};

typedef void (*udev_configure_device_t)(struct weston_compositor *compositor,
					struct libinput_device *device);

struct udev_input {
	struct libinput *libinput;
	struct wl_event_source *libinput_source;
	struct weston_compositor *compositor;
	int suspended;
	udev_configure_device_t configure_device;
};

int
udev_input_enable(struct udev_input *input);
void
udev_input_disable(struct udev_input *input);
int
udev_input_init(struct udev_input *input,
		struct weston_compositor *c,
		struct udev *udev,
		const char *seat_id,
		udev_configure_device_t configure_device);
void
udev_input_destroy(struct udev_input *input);

struct udev_seat *
udev_seat_get_named(struct udev_input *u,
		    const char *seat_name);

#endif
