/*
 * Copyright © 2012 Scott Moreau
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
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libweston/libweston.h>
#include "backend.h"
#include "libweston-internal.h"
#include "text-cursor-position-server-protocol.h"
#include "shared/helpers.h"

static void
weston_zoom_frame_z(struct weston_animation *animation,
		    struct weston_output *output,
		    const struct timespec *time)
{
	if (animation->frame_counter <= 1)
		output->zoom.spring_z.timestamp = *time;

	weston_spring_update(&output->zoom.spring_z, time);

	if (output->zoom.spring_z.current > output->zoom.max_level)
		output->zoom.spring_z.current = output->zoom.max_level;
	else if (output->zoom.spring_z.current < 0.0)
		output->zoom.spring_z.current = 0.0;

	if (weston_spring_done(&output->zoom.spring_z)) {
		if (output->zoom.active && output->zoom.level <= 0.0) {
			output->zoom.active = false;
			output->zoom.seat = NULL;
			weston_output_disable_planes_decr(output);
			wl_list_remove(&output->zoom.motion_listener.link);
		}
		output->zoom.spring_z.current = output->zoom.level;
		wl_list_remove(&animation->link);
		wl_list_init(&animation->link);
	}

	output->dirty = 1;
	weston_output_damage(output);
}

static void
zoom_area_center_from_point(struct weston_output *output,
			    double *x, double *y)
{
	float level = output->zoom.spring_z.current;

	*x = (*x - output->x) * level + output->width / 2.;
	*y = (*y - output->y) * level + output->height / 2.;
}

static void
weston_output_update_zoom_transform(struct weston_output *output)
{
	double x = output->zoom.current.x; /* global pointer coords */
	double y = output->zoom.current.y;
	float level;

	level = output->zoom.spring_z.current;

	if (!output->zoom.active || level > output->zoom.max_level ||
	    level == 0.0f)
		return;

	zoom_area_center_from_point(output, &x, &y);

	output->zoom.trans_x = x - output->width / 2;
	output->zoom.trans_y = y - output->height / 2;

	if (output->zoom.trans_x < 0)
		output->zoom.trans_x = 0;
	if (output->zoom.trans_y < 0)
		output->zoom.trans_y = 0;
	if (output->zoom.trans_x > level * output->width)
		output->zoom.trans_x = level * output->width;
	if (output->zoom.trans_y > level * output->height)
		output->zoom.trans_y = level * output->height;
}

static void
weston_zoom_transition(struct weston_output *output)
{
	if (output->zoom.level != output->zoom.spring_z.current) {
		output->zoom.spring_z.target = output->zoom.level;
		if (wl_list_empty(&output->zoom.animation_z.link)) {
			output->zoom.animation_z.frame_counter = 0;
			wl_list_insert(output->animation_list.prev,
				&output->zoom.animation_z.link);
		}
	}

	output->dirty = 1;
	weston_output_damage(output);
}

WL_EXPORT void
weston_output_update_zoom(struct weston_output *output)
{
	struct weston_seat *seat = output->zoom.seat;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (!pointer)
		return;

	assert(output->zoom.active);

	output->zoom.current.x = wl_fixed_to_double(pointer->x);
	output->zoom.current.y = wl_fixed_to_double(pointer->y);

	weston_zoom_transition(output);
	weston_output_update_zoom_transform(output);
}

static void
motion(struct wl_listener *listener, void *data)
{
	struct weston_output_zoom *zoom =
		container_of(listener, struct weston_output_zoom, motion_listener);
	struct weston_output *output =
		container_of(zoom, struct weston_output, zoom);

	weston_output_update_zoom(output);
}

WL_EXPORT void
weston_output_activate_zoom(struct weston_output *output,
			    struct weston_seat *seat)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (!pointer || output->zoom.active)
		return;

	output->zoom.active = true;
	output->zoom.seat = seat;
	weston_output_disable_planes_incr(output);
	wl_signal_add(&pointer->motion_signal,
		      &output->zoom.motion_listener);
}

WL_EXPORT void
weston_output_init_zoom(struct weston_output *output)
{
	output->zoom.active = false;
	output->zoom.seat = NULL;
	output->zoom.increment = 0.07;
	output->zoom.max_level = 0.95;
	output->zoom.level = 0.0;
	output->zoom.trans_x = 0.0;
	output->zoom.trans_y = 0.0;
	weston_spring_init(&output->zoom.spring_z, 250.0, 0.0, 0.0);
	output->zoom.spring_z.friction = 1000;
	output->zoom.animation_z.frame = weston_zoom_frame_z;
	wl_list_init(&output->zoom.animation_z.link);
	output->zoom.motion_listener.notify = motion;
}
