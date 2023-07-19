/*
 * libbacklight - userspace interface to Linux backlight control
 *
 * Copyright © 2012 Intel Corporation
 * Copyright 2010 Red Hat <mjg@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Matthew Garrett <mjg@redhat.com>
 *    Tiago Vignatti <vignatti@freedesktop.org>
 */
#ifndef LIBBACKLIGHT_H
#define LIBBACKLIGHT_H
#include <libudev.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum backlight_type {
	BACKLIGHT_RAW,
	BACKLIGHT_PLATFORM,
	BACKLIGHT_FIRMWARE
};

struct backlight {
	char *path;
	int max_brightness;
	int brightness;
	enum backlight_type type;
};

/*
 * Find and set up a backlight for a valid udev connector device, i.e. one
 * matching drm subsystem and with status of connected.
 */
struct backlight *backlight_init(struct udev_device *drm_device,
				 uint32_t connector_type);

/* Free backlight resources */
void backlight_destroy(struct backlight *backlight);

/* Provide the maximum backlight value */
long backlight_get_max_brightness(struct backlight *backlight);

/* Provide the cached backlight value */
long backlight_get_brightness(struct backlight *backlight);

/* Provide the hardware backlight value */
long backlight_get_actual_brightness(struct backlight *backlight);

/* Set the backlight to a value between 0 and max */
long backlight_set_brightness(struct backlight *backlight, long brightness);

#ifdef __cplusplus
}
#endif

#endif /* LIBBACKLIGHT_H */
