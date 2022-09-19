/*
 * Copyright © 2013 Intel Corporation
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <values.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/timespec-util.h"
#include <libweston/libweston.h>
#include "backend.h"
#include "libweston-internal.h"
#include "relative-pointer-unstable-v1-server-protocol.h"
#include "pointer-constraints-unstable-v1-server-protocol.h"
#include "input-timestamps-unstable-v1-server-protocol.h"

enum pointer_constraint_type {
	POINTER_CONSTRAINT_TYPE_LOCK,
	POINTER_CONSTRAINT_TYPE_CONFINE,
};

enum motion_direction {
	MOTION_DIRECTION_POSITIVE_X = 1 << 0,
	MOTION_DIRECTION_NEGATIVE_X = 1 << 1,
	MOTION_DIRECTION_POSITIVE_Y = 1 << 2,
	MOTION_DIRECTION_NEGATIVE_Y = 1 << 3,
};

struct vec2d {
	double x, y;
};

struct line {
	struct vec2d a;
	struct vec2d b;
};

struct border {
	struct line line;
	enum motion_direction blocking_dir;
};

static void
maybe_warp_confined_pointer(struct weston_pointer_constraint *constraint);

static void
empty_region(pixman_region32_t *region)
{
	pixman_region32_fini(region);
	pixman_region32_init(region);
}

static void
region_init_infinite(pixman_region32_t *region)
{
	pixman_region32_init_rect(region, INT32_MIN, INT32_MIN,
				  UINT32_MAX, UINT32_MAX);
}

static void
send_timestamp(struct wl_resource *resource,
	       const struct timespec *time)
{
	uint32_t tv_sec_hi, tv_sec_lo, tv_nsec;

	timespec_to_proto(time, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	zwp_input_timestamps_v1_send_timestamp(resource, tv_sec_hi, tv_sec_lo,
					       tv_nsec);
}

static void
send_timestamps_for_input_resource(struct wl_resource *input_resource,
				   struct wl_list *list,
				   const struct timespec *time)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, list) {
		if (wl_resource_get_user_data(resource) == input_resource)
			send_timestamp(resource, time);
	}
}

static void
remove_input_resource_from_timestamps(struct wl_resource *input_resource,
				      struct wl_list *list)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, list) {
		if (wl_resource_get_user_data(resource) == input_resource)
			wl_resource_set_user_data(resource, NULL);
	}
}

/** Register a touchscreen input device
 *
 * \param touch The parent weston_touch that identifies the seat.
 * \param syspath Unique device name.
 * \param backend_data Backend private data if necessary.
 * \param ops Calibration operations, or NULL for not able to run calibration.
 * \return New touch device, or NULL on failure.
 */
WL_EXPORT struct weston_touch_device *
weston_touch_create_touch_device(struct weston_touch *touch,
				 const char *syspath,
				 void *backend_data,
				 const struct weston_touch_device_ops *ops)
{
	struct weston_touch_device *device;

	assert(syspath);
	if (ops) {
		assert(ops->get_output);
		assert(ops->get_calibration_head_name);
		assert(ops->get_calibration);
		assert(ops->set_calibration);
	}

	device = zalloc(sizeof *device);
	if (!device)
		return NULL;

	wl_signal_init(&device->destroy_signal);

	device->syspath = strdup(syspath);
	if (!device->syspath) {
		free(device);
		return NULL;
	}

	device->backend_data = backend_data;
	device->ops = ops;

	device->aggregate = touch;
	wl_list_insert(touch->device_list.prev, &device->link);

	return device;
}

/** Destroy the touch device. */
WL_EXPORT void
weston_touch_device_destroy(struct weston_touch_device *device)
{
	wl_list_remove(&device->link);
	wl_signal_emit(&device->destroy_signal, device);
	free(device->syspath);
	free(device);
}

/** Is it possible to run calibration on this touch device? */
WL_EXPORT bool
weston_touch_device_can_calibrate(struct weston_touch_device *device)
{
	return !!device->ops;
}

static enum weston_touch_mode
weston_touch_device_get_mode(struct weston_touch_device *device)
{
	return device->aggregate->seat->compositor->touch_mode;
}

static struct weston_pointer_client *
weston_pointer_client_create(struct wl_client *client)
{
	struct weston_pointer_client *pointer_client;

	pointer_client = zalloc(sizeof *pointer_client);
	if (!pointer_client)
		return NULL;

	pointer_client->client = client;
	wl_list_init(&pointer_client->pointer_resources);
	wl_list_init(&pointer_client->relative_pointer_resources);

	return pointer_client;
}

static void
weston_pointer_client_destroy(struct weston_pointer_client *pointer_client)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, &pointer_client->pointer_resources) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_resource_for_each(resource,
			     &pointer_client->relative_pointer_resources) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_list_remove(&pointer_client->pointer_resources);
	wl_list_remove(&pointer_client->relative_pointer_resources);
	free(pointer_client);
}

static bool
weston_pointer_client_is_empty(struct weston_pointer_client *pointer_client)
{
	return (wl_list_empty(&pointer_client->pointer_resources) &&
		wl_list_empty(&pointer_client->relative_pointer_resources));
}

static struct weston_pointer_client *
weston_pointer_get_pointer_client(struct weston_pointer *pointer,
				  struct wl_client *client)
{
	struct weston_pointer_client *pointer_client;

	wl_list_for_each(pointer_client, &pointer->pointer_clients, link) {
		if (pointer_client->client == client)
			return pointer_client;
	}

	return NULL;
}

static struct weston_pointer_client *
weston_pointer_ensure_pointer_client(struct weston_pointer *pointer,
				     struct wl_client *client)
{
	struct weston_pointer_client *pointer_client;

	pointer_client = weston_pointer_get_pointer_client(pointer, client);
	if (pointer_client)
		return pointer_client;

	pointer_client = weston_pointer_client_create(client);
	wl_list_insert(&pointer->pointer_clients, &pointer_client->link);

	if (pointer->focus &&
	    pointer->focus->surface->resource &&
	    wl_resource_get_client(pointer->focus->surface->resource) == client) {
		pointer->focus_client = pointer_client;
	}

	return pointer_client;
}

static void
weston_pointer_cleanup_pointer_client(struct weston_pointer *pointer,
				      struct weston_pointer_client *pointer_client)
{
	if (weston_pointer_client_is_empty(pointer_client)) {
		if (pointer->focus_client == pointer_client)
			pointer->focus_client = NULL;
		wl_list_remove(&pointer_client->link);
		weston_pointer_client_destroy(pointer_client);
	}
}

static void
unbind_pointer_client_resource(struct wl_resource *resource)
{
	struct weston_pointer *pointer = wl_resource_get_user_data(resource);
	struct wl_client *client = wl_resource_get_client(resource);
	struct weston_pointer_client *pointer_client;

	wl_list_remove(wl_resource_get_link(resource));

	if (pointer) {
		pointer_client = weston_pointer_get_pointer_client(pointer,
								   client);
		assert(pointer_client);
		remove_input_resource_from_timestamps(resource,
						      &pointer->timestamps_list);
		weston_pointer_cleanup_pointer_client(pointer, pointer_client);
	}
}

static void unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

WL_EXPORT void
weston_pointer_motion_to_abs(struct weston_pointer *pointer,
			     struct weston_pointer_motion_event *event,
			     wl_fixed_t *x, wl_fixed_t *y)
{
	if (event->mask & WESTON_POINTER_MOTION_ABS) {
		*x = wl_fixed_from_double(event->x);
		*y = wl_fixed_from_double(event->y);
	} else if (event->mask & WESTON_POINTER_MOTION_REL) {
		*x = pointer->x + wl_fixed_from_double(event->dx);
		*y = pointer->y + wl_fixed_from_double(event->dy);
	} else {
		assert(!"invalid motion event");
		*x = *y = 0;
	}
}

static bool
weston_pointer_motion_to_rel(struct weston_pointer *pointer,
			     struct weston_pointer_motion_event *event,
			     double *dx, double *dy,
			     double *dx_unaccel, double *dy_unaccel)
{
	if (event->mask & WESTON_POINTER_MOTION_REL &&
	    event->mask & WESTON_POINTER_MOTION_REL_UNACCEL) {
		*dx = event->dx;
		*dy = event->dy;
		*dx_unaccel = event->dx_unaccel;
		*dy_unaccel = event->dy_unaccel;
		return true;
	} else if (event->mask & WESTON_POINTER_MOTION_REL) {
		*dx_unaccel = *dx = event->dx;
		*dy_unaccel = *dy = event->dy;
		return true;
	} else if (event->mask & WESTON_POINTER_MOTION_REL_UNACCEL) {
		*dx_unaccel = *dx = event->dx_unaccel;
		*dy_unaccel = *dy = event->dy_unaccel;
		return true;
	} else {
		return false;
	}
}

WL_EXPORT void
weston_seat_repick(struct weston_seat *seat)
{
	const struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (!pointer)
		return;

	pointer->grab->interface->focus(pointer->grab);
}

static void
weston_compositor_idle_inhibit(struct weston_compositor *compositor)
{
	weston_compositor_wake(compositor);
	compositor->idle_inhibit++;
}

static void
weston_compositor_idle_release(struct weston_compositor *compositor)
{
	compositor->idle_inhibit--;
	weston_compositor_wake(compositor);
}

static void
pointer_focus_view_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer =
		container_of(listener, struct weston_pointer,
			     focus_view_listener);

	weston_pointer_clear_focus(pointer);
}

static void
pointer_focus_resource_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer =
		container_of(listener, struct weston_pointer,
			     focus_resource_listener);

	weston_pointer_clear_focus(pointer);
}

static void
keyboard_focus_resource_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_keyboard *keyboard =
		container_of(listener, struct weston_keyboard,
			     focus_resource_listener);

	weston_keyboard_set_focus(keyboard, NULL);
}

static void
touch_focus_view_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_touch *touch =
		container_of(listener, struct weston_touch,
			     focus_view_listener);

	weston_touch_set_focus(touch, NULL);
}

static void
touch_focus_resource_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_touch *touch =
		container_of(listener, struct weston_touch,
			     focus_resource_listener);

	weston_touch_set_focus(touch, NULL);
}

static void
move_resources(struct wl_list *destination, struct wl_list *source)
{
	wl_list_insert_list(destination, source);
	wl_list_init(source);
}

static void
move_resources_for_client(struct wl_list *destination,
			  struct wl_list *source,
			  struct wl_client *client)
{
	struct wl_resource *resource, *tmp;
	wl_resource_for_each_safe(resource, tmp, source) {
		if (wl_resource_get_client(resource) == client) {
			wl_list_remove(wl_resource_get_link(resource));
			wl_list_insert(destination,
				       wl_resource_get_link(resource));
		}
	}
}

static void
default_grab_pointer_focus(struct weston_pointer_grab *grab)
{
	struct weston_pointer *pointer = grab->pointer;
	struct weston_view *view;
	wl_fixed_t sx, sy;

	if (pointer->button_count > 0)
		return;

	view = weston_compositor_pick_view(pointer->seat->compositor,
					   pointer->x, pointer->y,
					   &sx, &sy);

	if (pointer->focus != view || pointer->sx != sx || pointer->sy != sy)
		weston_pointer_set_focus(pointer, view, sx, sy);
}

static void
pointer_send_relative_motion(struct weston_pointer *pointer,
			     const struct timespec *time,
			     struct weston_pointer_motion_event *event)
{
	uint64_t time_usec;
	double dx, dy, dx_unaccel, dy_unaccel;
	wl_fixed_t dxf, dyf, dxf_unaccel, dyf_unaccel;
	struct wl_list *resource_list;
	struct wl_resource *resource;

	if (!pointer->focus_client)
		return;

	if (!weston_pointer_motion_to_rel(pointer, event,
					  &dx, &dy,
					  &dx_unaccel, &dy_unaccel))
		return;

	resource_list = &pointer->focus_client->relative_pointer_resources;
	time_usec = timespec_to_usec(&event->time);
	if (time_usec == 0)
		time_usec = timespec_to_usec(time);

	dxf = wl_fixed_from_double(dx);
	dyf = wl_fixed_from_double(dy);
	dxf_unaccel = wl_fixed_from_double(dx_unaccel);
	dyf_unaccel = wl_fixed_from_double(dy_unaccel);

	wl_resource_for_each(resource, resource_list) {
		zwp_relative_pointer_v1_send_relative_motion(
			resource,
			(uint32_t) (time_usec >> 32),
			(uint32_t) time_usec,
			dxf, dyf,
			dxf_unaccel, dyf_unaccel);
	}
}

static void
pointer_send_motion(struct weston_pointer *pointer,
		    const struct timespec *time,
		    wl_fixed_t sx, wl_fixed_t sy)
{
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t msecs;

	if (!pointer->focus_client)
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
                                                   &pointer->timestamps_list,
                                                   time);
		wl_pointer_send_motion(resource, msecs, sx, sy);
	}
}

WL_EXPORT void
weston_pointer_send_motion(struct weston_pointer *pointer,
			   const struct timespec *time,
			   struct weston_pointer_motion_event *event)
{
	wl_fixed_t x, y;
	wl_fixed_t old_sx = pointer->sx;
	wl_fixed_t old_sy = pointer->sy;

	if (pointer->focus) {
		weston_pointer_motion_to_abs(pointer, event, &x, &y);
		weston_view_from_global_fixed(pointer->focus, x, y,
					      &pointer->sx, &pointer->sy);
	}

	weston_pointer_move(pointer, event);

	if (old_sx != pointer->sx || old_sy != pointer->sy) {
		pointer_send_motion(pointer, time,
				    pointer->sx, pointer->sy);
	}

	pointer_send_relative_motion(pointer, time, event);
}

static void
default_grab_pointer_motion(struct weston_pointer_grab *grab,
			    const struct timespec *time,
			    struct weston_pointer_motion_event *event)
{
	weston_pointer_send_motion(grab->pointer, time, event);
}

/** Check if the pointer has focused resources.
 *
 * \param pointer The pointer to check for focused resources.
 * \return Whether or not this pointer has focused resources
 */
WL_EXPORT bool
weston_pointer_has_focus_resource(struct weston_pointer *pointer)
{
	if (!pointer->focus_client)
		return false;

	if (wl_list_empty(&pointer->focus_client->pointer_resources))
		return false;

	return true;
}

/** Send wl_pointer.button events to focused resources.
 *
 * \param pointer The pointer where the button events originates from.
 * \param time The timestamp of the event
 * \param button The button value of the event
 * \param state The state enum value of the event
 *
 * For every resource that is currently in focus, send a wl_pointer.button event
 * with the passed parameters. The focused resources are the wl_pointer
 * resources of the client which currently has the surface with pointer focus.
 */
WL_EXPORT void
weston_pointer_send_button(struct weston_pointer *pointer,
			   const struct timespec *time, uint32_t button,
			   enum wl_pointer_button_state state)
{
	struct wl_display *display = pointer->seat->compositor->wl_display;
	struct wl_list *resource_list;
	struct wl_resource *resource;
	uint32_t serial;
	uint32_t msecs;

	if (!weston_pointer_has_focus_resource(pointer))
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	serial = wl_display_next_serial(display);
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
                                                   &pointer->timestamps_list,
                                                   time);
		wl_pointer_send_button(resource, serial, msecs, button, state);
	}
}

static void
default_grab_pointer_button(struct weston_pointer_grab *grab,
			    const struct timespec *time, uint32_t button,
			    enum wl_pointer_button_state state)
{
	struct weston_pointer *pointer = grab->pointer;
	struct weston_compositor *compositor = pointer->seat->compositor;
	struct weston_view *view;
	wl_fixed_t sx, sy;

	weston_pointer_send_button(pointer, time, button, state);

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		view = weston_compositor_pick_view(compositor,
						   pointer->x, pointer->y,
						   &sx, &sy);

		weston_pointer_set_focus(pointer, view, sx, sy);
	}
}

/** Send wl_pointer.axis events to focused resources.
 *
 * \param pointer The pointer where the axis events originates from.
 * \param time The timestamp of the event
 * \param event The axis value of the event
 *
 * For every resource that is currently in focus, send a wl_pointer.axis event
 * with the passed parameters. The focused resources are the wl_pointer
 * resources of the client which currently has the surface with pointer focus.
 */
WL_EXPORT void
weston_pointer_send_axis(struct weston_pointer *pointer,
			 const struct timespec *time,
			 struct weston_pointer_axis_event *event)
{
	struct wl_resource *resource;
	struct wl_list *resource_list;
	uint32_t msecs;

	if (!weston_pointer_has_focus_resource(pointer))
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		if (event->has_discrete &&
		    wl_resource_get_version(resource) >=
		    WL_POINTER_AXIS_DISCRETE_SINCE_VERSION)
			wl_pointer_send_axis_discrete(resource, event->axis,
						      event->discrete);

		if (event->value) {
			send_timestamps_for_input_resource(resource,
							   &pointer->timestamps_list,
							   time);
			wl_pointer_send_axis(resource, msecs,
					     event->axis,
					     wl_fixed_from_double(event->value));
		} else if (wl_resource_get_version(resource) >=
			 WL_POINTER_AXIS_STOP_SINCE_VERSION) {
			send_timestamps_for_input_resource(resource,
							   &pointer->timestamps_list,
							   time);
			wl_pointer_send_axis_stop(resource, msecs,
						  event->axis);
		}
	}
}

/** Send wl_pointer.axis_source events to focused resources.
 *
 * \param pointer The pointer where the axis_source events originates from.
 * \param source The axis_source enum value of the event
 *
 * For every resource that is currently in focus, send a wl_pointer.axis_source
 * event with the passed parameter. The focused resources are the wl_pointer
 * resources of the client which currently has the surface with pointer focus.
 */
WL_EXPORT void
weston_pointer_send_axis_source(struct weston_pointer *pointer,
				enum wl_pointer_axis_source source)
{
	struct wl_resource *resource;
	struct wl_list *resource_list;

	if (!weston_pointer_has_focus_resource(pointer))
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	wl_resource_for_each(resource, resource_list) {
		if (wl_resource_get_version(resource) >=
		    WL_POINTER_AXIS_SOURCE_SINCE_VERSION) {
			wl_pointer_send_axis_source(resource, source);
		}
	}
}

static void
pointer_send_frame(struct wl_resource *resource)
{
	if (wl_resource_get_version(resource) >=
	    WL_POINTER_FRAME_SINCE_VERSION) {
		wl_pointer_send_frame(resource);
	}
}

/** Send wl_pointer.frame events to focused resources.
 *
 * \param pointer The pointer where the frame events originates from.
 *
 * For every resource that is currently in focus, send a wl_pointer.frame event.
 * The focused resources are the wl_pointer resources of the client which
 * currently has the surface with pointer focus.
 */
WL_EXPORT void
weston_pointer_send_frame(struct weston_pointer *pointer)
{
	struct wl_resource *resource;
	struct wl_list *resource_list;

	if (!weston_pointer_has_focus_resource(pointer))
		return;

	resource_list = &pointer->focus_client->pointer_resources;
	wl_resource_for_each(resource, resource_list)
		pointer_send_frame(resource);
}

static void
default_grab_pointer_axis(struct weston_pointer_grab *grab,
			  const struct timespec *time,
			  struct weston_pointer_axis_event *event)
{
	weston_pointer_send_axis(grab->pointer, time, event);
}

static void
default_grab_pointer_axis_source(struct weston_pointer_grab *grab,
				 enum wl_pointer_axis_source source)
{
	weston_pointer_send_axis_source(grab->pointer, source);
}

static void
default_grab_pointer_frame(struct weston_pointer_grab *grab)
{
	weston_pointer_send_frame(grab->pointer);
}

static void
default_grab_pointer_cancel(struct weston_pointer_grab *grab)
{
}

static const struct weston_pointer_grab_interface
				default_pointer_grab_interface = {
	default_grab_pointer_focus,
	default_grab_pointer_motion,
	default_grab_pointer_button,
	default_grab_pointer_axis,
	default_grab_pointer_axis_source,
	default_grab_pointer_frame,
	default_grab_pointer_cancel,
};

/** Check if the touch has focused resources.
 *
 * \param touch The touch to check for focused resources.
 * \return Whether or not this touch has focused resources
 */
WL_EXPORT bool
weston_touch_has_focus_resource(struct weston_touch *touch)
{
	if (!touch->focus)
		return false;

	if (wl_list_empty(&touch->focus_resource_list))
		return false;

	return true;
}

/** Send wl_touch.down events to focused resources.
 *
 * \param touch The touch where the down events originates from.
 * \param time The timestamp of the event
 * \param touch_id The touch_id value of the event
 * \param x The x value of the event
 * \param y The y value of the event
 *
 * For every resource that is currently in focus, send a wl_touch.down event
 * with the passed parameters. The focused resources are the wl_touch
 * resources of the client which currently has the surface with touch focus.
 */
WL_EXPORT void
weston_touch_send_down(struct weston_touch *touch, const struct timespec *time,
		       int touch_id, wl_fixed_t x, wl_fixed_t y)
{
	struct wl_display *display = touch->seat->compositor->wl_display;
	uint32_t serial;
	struct wl_resource *resource;
	struct wl_list *resource_list;
	wl_fixed_t sx, sy;
	uint32_t msecs;

	if (!weston_touch_has_focus_resource(touch))
		return;

	weston_view_from_global_fixed(touch->focus, x, y, &sx, &sy);

	resource_list = &touch->focus_resource_list;
	serial = wl_display_next_serial(display);
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
						   &touch->timestamps_list,
						   time);
		wl_touch_send_down(resource, serial, msecs,
				   touch->focus->surface->resource,
				   touch_id, sx, sy);
	}
}

static void
default_grab_touch_down(struct weston_touch_grab *grab,
			const struct timespec *time, int touch_id,
			wl_fixed_t x, wl_fixed_t y)
{
	weston_touch_send_down(grab->touch, time, touch_id, x, y);
}

/** Send wl_touch.up events to focused resources.
 *
 * \param touch The touch where the up events originates from.
 * \param time The timestamp of the event
 * \param touch_id The touch_id value of the event
 *
 * For every resource that is currently in focus, send a wl_touch.up event
 * with the passed parameters. The focused resources are the wl_touch
 * resources of the client which currently has the surface with touch focus.
 */
WL_EXPORT void
weston_touch_send_up(struct weston_touch *touch, const struct timespec *time,
		     int touch_id)
{
	struct wl_display *display = touch->seat->compositor->wl_display;
	uint32_t serial;
	struct wl_resource *resource;
	struct wl_list *resource_list;
	uint32_t msecs;

	if (!weston_touch_has_focus_resource(touch))
		return;

	resource_list = &touch->focus_resource_list;
	serial = wl_display_next_serial(display);
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
						   &touch->timestamps_list,
						   time);
		wl_touch_send_up(resource, serial, msecs, touch_id);
	}
}

static void
default_grab_touch_up(struct weston_touch_grab *grab,
		      const struct timespec *time, int touch_id)
{
	weston_touch_send_up(grab->touch, time, touch_id);
}

/** Send wl_touch.motion events to focused resources.
 *
 * \param touch The touch where the motion events originates from.
 * \param time The timestamp of the event
 * \param touch_id The touch_id value of the event
 * \param x The x value of the event
 * \param y The y value of the event
 *
 * For every resource that is currently in focus, send a wl_touch.motion event
 * with the passed parameters. The focused resources are the wl_touch
 * resources of the client which currently has the surface with touch focus.
 */
WL_EXPORT void
weston_touch_send_motion(struct weston_touch *touch,
			 const struct timespec *time, int touch_id,
			 wl_fixed_t x, wl_fixed_t y)
{
	struct wl_resource *resource;
	struct wl_list *resource_list;
	wl_fixed_t sx, sy;
	uint32_t msecs;

	if (!weston_touch_has_focus_resource(touch))
		return;

	weston_view_from_global_fixed(touch->focus, x, y, &sx, &sy);

	resource_list = &touch->focus_resource_list;
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
						   &touch->timestamps_list,
						   time);
		wl_touch_send_motion(resource, msecs,
				     touch_id, sx, sy);
	}
}

static void
default_grab_touch_motion(struct weston_touch_grab *grab,
			  const struct timespec *time, int touch_id,
			  wl_fixed_t x, wl_fixed_t y)
{
	weston_touch_send_motion(grab->touch, time, touch_id, x, y);
}


/** Send wl_touch.frame events to focused resources.
 *
 * \param touch The touch where the frame events originates from.
 *
 * For every resource that is currently in focus, send a wl_touch.frame event.
 * The focused resources are the wl_touch resources of the client which
 * currently has the surface with touch focus.
 */
WL_EXPORT void
weston_touch_send_frame(struct weston_touch *touch)
{
	struct wl_resource *resource;

	if (!weston_touch_has_focus_resource(touch))
		return;

	wl_resource_for_each(resource, &touch->focus_resource_list)
		wl_touch_send_frame(resource);
}

static void
default_grab_touch_frame(struct weston_touch_grab *grab)
{
	weston_touch_send_frame(grab->touch);
}

static void
default_grab_touch_cancel(struct weston_touch_grab *grab)
{
}

static const struct weston_touch_grab_interface default_touch_grab_interface = {
	default_grab_touch_down,
	default_grab_touch_up,
	default_grab_touch_motion,
	default_grab_touch_frame,
	default_grab_touch_cancel,
};

/** Check if the keyboard has focused resources.
 *
 * \param keyboard The keyboard to check for focused resources.
 * \return Whether or not this keyboard has focused resources
 */
WL_EXPORT bool
weston_keyboard_has_focus_resource(struct weston_keyboard *keyboard)
{
	if (!keyboard->focus)
		return false;

	if (wl_list_empty(&keyboard->focus_resource_list))
		return false;

	return true;
}

/** Send wl_keyboard.key events to focused resources.
 *
 * \param keyboard The keyboard where the key events originates from.
 * \param time The timestamp of the event
 * \param key The key value of the event
 * \param state The state enum value of the event
 *
 * For every resource that is currently in focus, send a wl_keyboard.key event
 * with the passed parameters. The focused resources are the wl_keyboard
 * resources of the client which currently has the surface with keyboard focus.
 */
WL_EXPORT void
weston_keyboard_send_key(struct weston_keyboard *keyboard,
			 const struct timespec *time, uint32_t key,
			 enum wl_keyboard_key_state state)
{
	struct wl_resource *resource;
	struct wl_display *display = keyboard->seat->compositor->wl_display;
	uint32_t serial;
	struct wl_list *resource_list;
	uint32_t msecs;

	if (!weston_keyboard_has_focus_resource(keyboard))
		return;

	resource_list = &keyboard->focus_resource_list;
	serial = wl_display_next_serial(display);
	msecs = timespec_to_msec(time);
	wl_resource_for_each(resource, resource_list) {
		send_timestamps_for_input_resource(resource,
						   &keyboard->timestamps_list,
						   time);
		wl_keyboard_send_key(resource, serial, msecs, key, state);
	}
};

static void
default_grab_keyboard_key(struct weston_keyboard_grab *grab,
			  const struct timespec *time, uint32_t key,
			  uint32_t state)
{
	weston_keyboard_send_key(grab->keyboard, time, key, state);
}

static void
send_modifiers_to_resource(struct weston_keyboard *keyboard,
			   struct wl_resource *resource,
			   uint32_t serial)
{
	wl_keyboard_send_modifiers(resource,
				   serial,
				   keyboard->modifiers.mods_depressed,
				   keyboard->modifiers.mods_latched,
				   keyboard->modifiers.mods_locked,
				   keyboard->modifiers.group);
}

static void
send_modifiers_to_client_in_list(struct wl_client *client,
				 struct wl_list *list,
				 uint32_t serial,
				 struct weston_keyboard *keyboard)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, list) {
		if (wl_resource_get_client(resource) == client)
			send_modifiers_to_resource(keyboard,
						   resource,
						   serial);
	}
}

static struct weston_pointer_client *
find_pointer_client_for_surface(struct weston_pointer *pointer,
				struct weston_surface *surface)
{
	struct wl_client *client;

	if (!surface)
		return NULL;

	if (!surface->resource)
		return NULL;

	client = wl_resource_get_client(surface->resource);
	return weston_pointer_get_pointer_client(pointer, client);
}

static struct weston_pointer_client *
find_pointer_client_for_view(struct weston_pointer *pointer, struct weston_view *view)
{
	if (!view)
		return NULL;

	return find_pointer_client_for_surface(pointer, view->surface);
}

static struct wl_resource *
find_resource_for_surface(struct wl_list *list, struct weston_surface *surface)
{
	if (!surface)
		return NULL;

	if (!surface->resource)
		return NULL;

	return wl_resource_find_for_client(list, wl_resource_get_client(surface->resource));
}

/** Send wl_keyboard.modifiers events to focused resources and pointer
 *  focused resources.
 *
 * \param keyboard The keyboard where the modifiers events originates from.
 * \param serial The serial of the event
 * \param mods_depressed The mods_depressed value of the event
 * \param mods_latched The mods_latched value of the event
 * \param mods_locked The mods_locked value of the event
 * \param group The group value of the event
 *
 * For every resource that is currently in focus, send a wl_keyboard.modifiers
 * event with the passed parameters. The focused resources are the wl_keyboard
 * resources of the client which currently has the surface with keyboard focus.
 * This also sends wl_keyboard.modifiers events to the wl_keyboard resources of
 * the client having pointer focus (if different from the keyboard focus client).
 */
WL_EXPORT void
weston_keyboard_send_modifiers(struct weston_keyboard *keyboard,
			       uint32_t serial, uint32_t mods_depressed,
			       uint32_t mods_latched,
			       uint32_t mods_locked, uint32_t group)
{
	struct weston_pointer *pointer =
		weston_seat_get_pointer(keyboard->seat);

	if (weston_keyboard_has_focus_resource(keyboard)) {
		struct wl_list *resource_list;
		struct wl_resource *resource;

		resource_list = &keyboard->focus_resource_list;
		wl_resource_for_each(resource, resource_list) {
			wl_keyboard_send_modifiers(resource, serial,
						   mods_depressed, mods_latched,
						   mods_locked, group);
		}
	}

	if (pointer && pointer->focus && pointer->focus->surface->resource &&
	    pointer->focus->surface != keyboard->focus) {
		struct wl_client *pointer_client =
			wl_resource_get_client(pointer->focus->surface->resource);

		send_modifiers_to_client_in_list(pointer_client,
						 &keyboard->resource_list,
						 serial,
						 keyboard);
	}
}

static void
default_grab_keyboard_modifiers(struct weston_keyboard_grab *grab,
				uint32_t serial, uint32_t mods_depressed,
				uint32_t mods_latched,
				uint32_t mods_locked, uint32_t group)
{
	weston_keyboard_send_modifiers(grab->keyboard, serial, mods_depressed,
				       mods_latched, mods_locked, group);
}

static void
default_grab_keyboard_cancel(struct weston_keyboard_grab *grab)
{
}

static const struct weston_keyboard_grab_interface
				default_keyboard_grab_interface = {
	default_grab_keyboard_key,
	default_grab_keyboard_modifiers,
	default_grab_keyboard_cancel,
};

static void
pointer_unmap_sprite(struct weston_pointer *pointer)
{
	struct weston_surface *surface = pointer->sprite->surface;

	if (weston_surface_is_mapped(surface))
		weston_surface_unmap(surface);

	wl_list_remove(&pointer->sprite_destroy_listener.link);
	surface->committed = NULL;
	surface->committed_private = NULL;
	weston_surface_set_label_func(surface, NULL);
	weston_view_destroy(pointer->sprite);
	pointer->sprite = NULL;
}

static void
pointer_handle_sprite_destroy(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer =
		container_of(listener, struct weston_pointer,
			     sprite_destroy_listener);

	pointer->sprite = NULL;
}

static void
weston_pointer_reset_state(struct weston_pointer *pointer)
{
	pointer->button_count = 0;
}

static void
weston_pointer_handle_output_destroy(struct wl_listener *listener, void *data);

static struct weston_pointer *
weston_pointer_create(struct weston_seat *seat)
{
	struct weston_pointer *pointer;

	pointer = zalloc(sizeof *pointer);
	if (pointer == NULL)
		return NULL;

	wl_list_init(&pointer->pointer_clients);
	weston_pointer_set_default_grab(pointer,
					seat->compositor->default_pointer_grab);
	wl_list_init(&pointer->focus_resource_listener.link);
	pointer->focus_resource_listener.notify = pointer_focus_resource_destroyed;
	pointer->default_grab.pointer = pointer;
	pointer->grab = &pointer->default_grab;
	wl_signal_init(&pointer->motion_signal);
	wl_signal_init(&pointer->focus_signal);
	wl_list_init(&pointer->focus_view_listener.link);
	wl_signal_init(&pointer->destroy_signal);
	wl_list_init(&pointer->timestamps_list);

	pointer->sprite_destroy_listener.notify = pointer_handle_sprite_destroy;

	/* FIXME: Pick better co-ords. */
	pointer->x = wl_fixed_from_int(100);
	pointer->y = wl_fixed_from_int(100);

	pointer->output_destroy_listener.notify =
		weston_pointer_handle_output_destroy;
	wl_signal_add(&seat->compositor->output_destroyed_signal,
		      &pointer->output_destroy_listener);

	pointer->sx = wl_fixed_from_int(-1000000);
	pointer->sy = wl_fixed_from_int(-1000000);

	return pointer;
}

static void
weston_pointer_destroy(struct weston_pointer *pointer)
{
	struct weston_pointer_client *pointer_client, *tmp;

	wl_signal_emit(&pointer->destroy_signal, pointer);

	if (pointer->sprite)
		pointer_unmap_sprite(pointer);

	wl_list_for_each_safe(pointer_client, tmp, &pointer->pointer_clients,
			      link) {
		wl_list_remove(&pointer_client->link);
		weston_pointer_client_destroy(pointer_client);
	}

	wl_list_remove(&pointer->focus_resource_listener.link);
	wl_list_remove(&pointer->focus_view_listener.link);
	wl_list_remove(&pointer->output_destroy_listener.link);
	wl_list_remove(&pointer->timestamps_list);
	free(pointer);
}

void
weston_pointer_set_default_grab(struct weston_pointer *pointer,
		const struct weston_pointer_grab_interface *interface)
{
	if (interface)
		pointer->default_grab.interface = interface;
	else
		pointer->default_grab.interface =
			&default_pointer_grab_interface;
}

static struct weston_keyboard *
weston_keyboard_create(void)
{
	struct weston_keyboard *keyboard;

	keyboard = zalloc(sizeof *keyboard);
	if (keyboard == NULL)
	    return NULL;

	wl_list_init(&keyboard->resource_list);
	wl_list_init(&keyboard->focus_resource_list);
	wl_list_init(&keyboard->focus_resource_listener.link);
	keyboard->focus_resource_listener.notify = keyboard_focus_resource_destroyed;
	wl_array_init(&keyboard->keys);
	keyboard->default_grab.interface = &default_keyboard_grab_interface;
	keyboard->default_grab.keyboard = keyboard;
	keyboard->grab = &keyboard->default_grab;
	wl_signal_init(&keyboard->focus_signal);
	wl_list_init(&keyboard->timestamps_list);

	return keyboard;
}

static void
weston_xkb_info_destroy(struct weston_xkb_info *xkb_info);

static void
weston_keyboard_destroy(struct weston_keyboard *keyboard)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, &keyboard->resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_resource_for_each(resource, &keyboard->focus_resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_list_remove(&keyboard->resource_list);
	wl_list_remove(&keyboard->focus_resource_list);

	xkb_state_unref(keyboard->xkb_state.state);
	if (keyboard->xkb_info)
		weston_xkb_info_destroy(keyboard->xkb_info);
	xkb_keymap_unref(keyboard->pending_keymap);

	wl_array_release(&keyboard->keys);
	wl_list_remove(&keyboard->focus_resource_listener.link);
	wl_list_remove(&keyboard->timestamps_list);
	free(keyboard);
}

static void
weston_touch_reset_state(struct weston_touch *touch)
{
	touch->num_tp = 0;
}

static struct weston_touch *
weston_touch_create(void)
{
	struct weston_touch *touch;

	touch = zalloc(sizeof *touch);
	if (touch == NULL)
		return NULL;

	wl_list_init(&touch->device_list);
	wl_list_init(&touch->resource_list);
	wl_list_init(&touch->focus_resource_list);
	wl_list_init(&touch->focus_view_listener.link);
	touch->focus_view_listener.notify = touch_focus_view_destroyed;
	wl_list_init(&touch->focus_resource_listener.link);
	touch->focus_resource_listener.notify = touch_focus_resource_destroyed;
	touch->default_grab.interface = &default_touch_grab_interface;
	touch->default_grab.touch = touch;
	touch->grab = &touch->default_grab;
	wl_signal_init(&touch->focus_signal);
	wl_list_init(&touch->timestamps_list);

	return touch;
}

static void
weston_touch_destroy(struct weston_touch *touch)
{
	struct wl_resource *resource;

	assert(wl_list_empty(&touch->device_list));

	wl_resource_for_each(resource, &touch->resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_resource_for_each(resource, &touch->focus_resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_list_remove(&touch->resource_list);
	wl_list_remove(&touch->focus_resource_list);
	wl_list_remove(&touch->focus_view_listener.link);
	wl_list_remove(&touch->focus_resource_listener.link);
	wl_list_remove(&touch->timestamps_list);
	free(touch);
}

static void
seat_send_updated_caps(struct weston_seat *seat)
{
	enum wl_seat_capability caps = 0;
	struct wl_resource *resource;

	if (seat->pointer_device_count > 0)
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (seat->keyboard_device_count > 0)
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (seat->touch_device_count > 0)
		caps |= WL_SEAT_CAPABILITY_TOUCH;

	wl_resource_for_each(resource, &seat->base_resource_list) {
		wl_seat_send_capabilities(resource, caps);
	}
	wl_signal_emit(&seat->updated_caps_signal, seat);
}


/** Clear the pointer focus
 *
 * \param pointer the pointer to clear focus for.
 *
 * This can be used to unset pointer focus and set the co-ordinates to the
 * arbitrary values we use for the no focus case.
 *
 * There's no requirement to use this function.  For example, passing the
 * results of a weston_compositor_pick_view() directly to
 * weston_pointer_set_focus() will do the right thing when no view is found.
 */
WL_EXPORT void
weston_pointer_clear_focus(struct weston_pointer *pointer)
{
	weston_pointer_set_focus(pointer, NULL,
				 wl_fixed_from_int(-1000000),
				 wl_fixed_from_int(-1000000));
}

WL_EXPORT void
weston_pointer_set_focus(struct weston_pointer *pointer,
			 struct weston_view *view,
			 wl_fixed_t sx, wl_fixed_t sy)
{
	struct weston_pointer_client *pointer_client;
	struct weston_keyboard *kbd = weston_seat_get_keyboard(pointer->seat);
	struct wl_resource *resource;
	struct wl_resource *surface_resource;
	struct wl_display *display = pointer->seat->compositor->wl_display;
	uint32_t serial;
	struct wl_list *focus_resource_list;
	int refocus = 0;

	if ((!pointer->focus && view) ||
	    (pointer->focus && !view) ||
	    (pointer->focus && pointer->focus->surface != view->surface) ||
	    pointer->sx != sx || pointer->sy != sy)
		refocus = 1;

	if (pointer->focus_client && refocus) {
		focus_resource_list = &pointer->focus_client->pointer_resources;
		if (!wl_list_empty(focus_resource_list)) {
			serial = wl_display_next_serial(display);
			surface_resource = pointer->focus->surface->resource;
			wl_resource_for_each(resource, focus_resource_list) {
				wl_pointer_send_leave(resource, serial,
						      surface_resource);
				pointer_send_frame(resource);
			}
		}

		pointer->focus_client = NULL;
	}

	pointer_client = find_pointer_client_for_view(pointer, view);
	if (pointer_client && refocus) {
		struct wl_client *surface_client = pointer_client->client;

		serial = wl_display_next_serial(display);

		if (kbd && kbd->focus != view->surface)
			send_modifiers_to_client_in_list(surface_client,
							 &kbd->resource_list,
							 serial,
							 kbd);

		pointer->focus_client = pointer_client;

		focus_resource_list = &pointer->focus_client->pointer_resources;
		wl_resource_for_each(resource, focus_resource_list) {
			wl_pointer_send_enter(resource,
					      serial,
					      view->surface->resource,
					      sx, sy);
			pointer_send_frame(resource);
		}

		pointer->focus_serial = serial;
	}

	wl_list_remove(&pointer->focus_view_listener.link);
	wl_list_init(&pointer->focus_view_listener.link);
	wl_list_remove(&pointer->focus_resource_listener.link);
	wl_list_init(&pointer->focus_resource_listener.link);
	if (view)
		wl_signal_add(&view->destroy_signal, &pointer->focus_view_listener);
	if (view && view->surface->resource)
		wl_resource_add_destroy_listener(view->surface->resource,
						 &pointer->focus_resource_listener);

	pointer->focus = view;
	pointer->focus_view_listener.notify = pointer_focus_view_destroyed;
	pointer->sx = sx;
	pointer->sy = sy;

	assert(view || sx == wl_fixed_from_int(-1000000));
	assert(view || sy == wl_fixed_from_int(-1000000));

	wl_signal_emit(&pointer->focus_signal, pointer);
}

static void
send_enter_to_resource_list(struct wl_list *list,
			    struct weston_keyboard *keyboard,
			    struct weston_surface *surface,
			    uint32_t serial)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, list) {
		wl_keyboard_send_enter(resource, serial,
				       surface->resource,
				       &keyboard->keys);
		send_modifiers_to_resource(keyboard, resource, serial);
	}
}

WL_EXPORT void
weston_keyboard_set_focus(struct weston_keyboard *keyboard,
			  struct weston_surface *surface)
{
	struct weston_seat *seat = keyboard->seat;
	struct wl_resource *resource;
	struct wl_display *display = keyboard->seat->compositor->wl_display;
	uint32_t serial;
	struct wl_list *focus_resource_list;

	/* Keyboard focus on a surface without a client is equivalent to NULL
	 * focus as nothing would react to the keyboard events anyway.
	 * Just set focus to NULL instead - the destroy listener hangs on the
	 * wl_resource anyway.
	 */
	if (surface && !surface->resource)
		surface = NULL;

	focus_resource_list = &keyboard->focus_resource_list;

	if (!wl_list_empty(focus_resource_list) && keyboard->focus != surface) {
		serial = wl_display_next_serial(display);
		wl_resource_for_each(resource, focus_resource_list) {
			wl_keyboard_send_leave(resource, serial,
					keyboard->focus->resource);
		}
		move_resources(&keyboard->resource_list, focus_resource_list);
	}

	if (find_resource_for_surface(&keyboard->resource_list, surface) &&
	    keyboard->focus != surface) {
		struct wl_client *surface_client =
			wl_resource_get_client(surface->resource);

		serial = wl_display_next_serial(display);

		move_resources_for_client(focus_resource_list,
					  &keyboard->resource_list,
					  surface_client);
		send_enter_to_resource_list(focus_resource_list,
					    keyboard,
					    surface,
					    serial);
		keyboard->focus_serial = serial;
	}

	if (seat->saved_kbd_focus) {
		wl_list_remove(&seat->saved_kbd_focus_listener.link);
		seat->saved_kbd_focus = NULL;
	}

	wl_list_remove(&keyboard->focus_resource_listener.link);
	wl_list_init(&keyboard->focus_resource_listener.link);
	if (surface)
		wl_resource_add_destroy_listener(surface->resource,
						 &keyboard->focus_resource_listener);

	keyboard->focus = surface;
	wl_signal_emit(&keyboard->focus_signal, keyboard);
}

/* Users of this function must manually manage the keyboard focus */
WL_EXPORT void
weston_keyboard_start_grab(struct weston_keyboard *keyboard,
			   struct weston_keyboard_grab *grab)
{
	keyboard->grab = grab;
	grab->keyboard = keyboard;
}

WL_EXPORT void
weston_keyboard_end_grab(struct weston_keyboard *keyboard)
{
	keyboard->grab = &keyboard->default_grab;
}

static void
weston_keyboard_cancel_grab(struct weston_keyboard *keyboard)
{
	keyboard->grab->interface->cancel(keyboard->grab);
}

WL_EXPORT void
weston_pointer_start_grab(struct weston_pointer *pointer,
			  struct weston_pointer_grab *grab)
{
	pointer->grab = grab;
	grab->pointer = pointer;
	pointer->grab->interface->focus(pointer->grab);
}

WL_EXPORT void
weston_pointer_end_grab(struct weston_pointer *pointer)
{
	pointer->grab = &pointer->default_grab;
	pointer->grab->interface->focus(pointer->grab);
}

static void
weston_pointer_cancel_grab(struct weston_pointer *pointer)
{
	pointer->grab->interface->cancel(pointer->grab);
}

WL_EXPORT void
weston_touch_start_grab(struct weston_touch *touch, struct weston_touch_grab *grab)
{
	touch->grab = grab;
	grab->touch = touch;
}

WL_EXPORT void
weston_touch_end_grab(struct weston_touch *touch)
{
	touch->grab = &touch->default_grab;
}

static void
weston_touch_cancel_grab(struct weston_touch *touch)
{
	touch->grab->interface->cancel(touch->grab);
}

static void
weston_pointer_clamp_for_output(struct weston_pointer *pointer,
				struct weston_output *output,
				wl_fixed_t *fx, wl_fixed_t *fy)
{
	int x, y;

	x = wl_fixed_to_int(*fx);
	y = wl_fixed_to_int(*fy);

	if (x < output->x)
		*fx = wl_fixed_from_int(output->x);
	else if (x >= output->x + output->width)
		*fx = wl_fixed_from_int(output->x +
					output->width - 1);
	if (y < output->y)
		*fy = wl_fixed_from_int(output->y);
	else if (y >= output->y + output->height)
		*fy = wl_fixed_from_int(output->y +
					output->height - 1);
}

WL_EXPORT void
weston_pointer_clamp(struct weston_pointer *pointer, wl_fixed_t *fx, wl_fixed_t *fy)
{
	struct weston_compositor *ec = pointer->seat->compositor;
	struct weston_output *output, *prev = NULL;
	int x, y, old_x, old_y, valid = 0;

	x = wl_fixed_to_int(*fx);
	y = wl_fixed_to_int(*fy);
	old_x = wl_fixed_to_int(pointer->x);
	old_y = wl_fixed_to_int(pointer->y);

	wl_list_for_each(output, &ec->output_list, link) {
		if (pointer->seat->output && pointer->seat->output != output)
			continue;
		if (pixman_region32_contains_point(&output->region,
						   x, y, NULL))
			valid = 1;
		if (pixman_region32_contains_point(&output->region,
						   old_x, old_y, NULL))
			prev = output;
	}

	if (!prev)
		prev = pointer->seat->output;

	if (prev && !valid)
		weston_pointer_clamp_for_output(pointer, prev, fx, fy);
}

static void
weston_pointer_move_to(struct weston_pointer *pointer,
		       wl_fixed_t x, wl_fixed_t y)
{
	int32_t ix, iy;

	weston_pointer_clamp (pointer, &x, &y);

	pointer->x = x;
	pointer->y = y;

	ix = wl_fixed_to_int(x);
	iy = wl_fixed_to_int(y);

	if (pointer->sprite) {
		weston_view_set_position(pointer->sprite,
					 ix - pointer->hotspot_x,
					 iy - pointer->hotspot_y);
		weston_view_schedule_repaint(pointer->sprite);
	}

	pointer->grab->interface->focus(pointer->grab);
	wl_signal_emit(&pointer->motion_signal, pointer);
}

WL_EXPORT void
weston_pointer_move(struct weston_pointer *pointer,
		    struct weston_pointer_motion_event *event)
{
	wl_fixed_t x, y;

	weston_pointer_motion_to_abs(pointer, event, &x, &y);
	weston_pointer_move_to(pointer, x, y);
}

/** Verify if the pointer is in a valid position and move it if it isn't.
 */
static void
weston_pointer_handle_output_destroy(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer;
	struct weston_compositor *ec;
	struct weston_output *output, *closest = NULL;
	int x, y, distance, min = INT_MAX;
	wl_fixed_t fx, fy;

	pointer = container_of(listener, struct weston_pointer,
			       output_destroy_listener);
	ec = pointer->seat->compositor;

	x = wl_fixed_to_int(pointer->x);
	y = wl_fixed_to_int(pointer->y);

	wl_list_for_each(output, &ec->output_list, link) {
		if (pixman_region32_contains_point(&output->region,
						   x, y, NULL))
			return;

		/* Aproximante the distance from the pointer to the center of
		 * the output. */
		distance = abs(output->x + output->width / 2 - x) +
			   abs(output->y + output->height / 2 - y);
		if (distance < min) {
			min = distance;
			closest = output;
		}
	}

	/* Nothing to do if there's no output left. */
	if (!closest)
		return;

	fx = pointer->x;
	fy = pointer->y;

	weston_pointer_clamp_for_output(pointer, closest, &fx, &fy);
	weston_pointer_move_to(pointer, fx, fy);
}

WL_EXPORT void
notify_motion(struct weston_seat *seat,
	      const struct timespec *time,
	      struct weston_pointer_motion_event *event)
{
	struct weston_compositor *ec = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	weston_compositor_wake(ec);
	pointer->grab->interface->motion(pointer->grab, time, event);
}

static void
run_modifier_bindings(struct weston_seat *seat, uint32_t old, uint32_t new)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	uint32_t diff;
	unsigned int i;
	struct {
		uint32_t xkb;
		enum weston_keyboard_modifier weston;
	} mods[] = {
		{ keyboard->xkb_info->ctrl_mod, MODIFIER_CTRL },
		{ keyboard->xkb_info->alt_mod, MODIFIER_ALT },
		{ keyboard->xkb_info->super_mod, MODIFIER_SUPER },
		{ keyboard->xkb_info->shift_mod, MODIFIER_SHIFT },
	};

	diff = new & ~old;
	for (i = 0; i < ARRAY_LENGTH(mods); i++) {
		if (diff & (1 << mods[i].xkb))
			weston_compositor_run_modifier_binding(compositor,
			                                       keyboard,
			                                       mods[i].weston,
			                                       WL_KEYBOARD_KEY_STATE_PRESSED);
	}

	diff = old & ~new;
	for (i = 0; i < ARRAY_LENGTH(mods); i++) {
		if (diff & (1 << mods[i].xkb))
			weston_compositor_run_modifier_binding(compositor,
			                                       keyboard,
			                                       mods[i].weston,
			                                       WL_KEYBOARD_KEY_STATE_RELEASED);
	}
}

WL_EXPORT void
notify_motion_absolute(struct weston_seat *seat, const struct timespec *time,
		       double x, double y)
{
	struct weston_compositor *ec = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_pointer_motion_event event = { 0 };

	weston_compositor_wake(ec);

	event = (struct weston_pointer_motion_event) {
		.mask = WESTON_POINTER_MOTION_ABS,
		.x = x,
		.y = y,
	};

	pointer->grab->interface->motion(pointer->grab, time, &event);
}

static unsigned int
peek_next_activate_serial(struct weston_compositor *c)
{
	unsigned serial = c->activate_serial + 1;

	return serial == 0 ? 1 : serial;
}

static void
inc_activate_serial(struct weston_compositor *c)
{
	c->activate_serial = peek_next_activate_serial (c);
}

WL_EXPORT void
weston_view_activate(struct weston_view *view,
		     struct weston_seat *seat,
		     uint32_t flags)
{
	struct weston_compositor *compositor = seat->compositor;

	if (flags & WESTON_ACTIVATE_FLAG_CLICKED) {
		view->click_to_activate_serial =
			peek_next_activate_serial(compositor);
	}

	weston_seat_set_keyboard_focus(seat, view->surface);
}

WL_EXPORT void
notify_button(struct weston_seat *seat, const struct timespec *time,
	      int32_t button, enum wl_pointer_button_state state)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		weston_compositor_idle_inhibit(compositor);
		if (pointer->button_count == 0) {
			pointer->grab_button = button;
			pointer->grab_time = *time;
			pointer->grab_x = pointer->x;
			pointer->grab_y = pointer->y;
		}
		pointer->button_count++;
	} else {
		weston_compositor_idle_release(compositor);
		pointer->button_count--;
	}

	weston_compositor_run_button_binding(compositor, pointer, time, button,
					     state);

	pointer->grab->interface->button(pointer->grab, time, button, state);

	if (pointer->button_count == 1)
		pointer->grab_serial =
			wl_display_get_serial(compositor->wl_display);
}

WL_EXPORT void
notify_axis(struct weston_seat *seat, const struct timespec *time,
	    struct weston_pointer_axis_event *event)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	weston_compositor_wake(compositor);

	if (weston_compositor_run_axis_binding(compositor, pointer,
					       time, event))
		return;

	pointer->grab->interface->axis(pointer->grab, time, event);
}

WL_EXPORT void
notify_axis_source(struct weston_seat *seat, uint32_t source)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	weston_compositor_wake(compositor);

	pointer->grab->interface->axis_source(pointer->grab, source);
}

WL_EXPORT void
notify_pointer_frame(struct weston_seat *seat)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	weston_compositor_wake(compositor);

	pointer->grab->interface->frame(pointer->grab);
}

WL_EXPORT int
weston_keyboard_set_locks(struct weston_keyboard *keyboard,
			  uint32_t mask, uint32_t value)
{
	uint32_t serial;
	xkb_mod_mask_t mods_depressed, mods_latched, mods_locked, group;
	xkb_mod_mask_t num, caps;

	/* We don't want the leds to go out of sync with the actual state
	 * so if the backend has no way to change the leds don't try to
	 * change the state */
	if (!keyboard->seat->led_update)
		return -1;

	mods_depressed = xkb_state_serialize_mods(keyboard->xkb_state.state,
						XKB_STATE_DEPRESSED);
	mods_latched = xkb_state_serialize_mods(keyboard->xkb_state.state,
						XKB_STATE_LATCHED);
	mods_locked = xkb_state_serialize_mods(keyboard->xkb_state.state,
						XKB_STATE_LOCKED);
	group = xkb_state_serialize_group(keyboard->xkb_state.state,
                                      XKB_STATE_EFFECTIVE);

	num = (1 << keyboard->xkb_info->mod2_mod);
	caps = (1 << keyboard->xkb_info->caps_mod);
	if (mask & WESTON_NUM_LOCK) {
		if (value & WESTON_NUM_LOCK)
			mods_locked |= num;
		else
			mods_locked &= ~num;
	}
	if (mask & WESTON_CAPS_LOCK) {
		if (value & WESTON_CAPS_LOCK)
			mods_locked |= caps;
		else
			mods_locked &= ~caps;
	}

	xkb_state_update_mask(keyboard->xkb_state.state, mods_depressed,
			      mods_latched, mods_locked, 0, 0, group);

	serial = wl_display_next_serial(
				keyboard->seat->compositor->wl_display);
	notify_modifiers(keyboard->seat, serial);

	return 0;
}

WL_EXPORT void
notify_modifiers(struct weston_seat *seat, uint32_t serial)
{
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_keyboard_grab *grab = keyboard->grab;
	uint32_t mods_depressed, mods_latched, mods_locked, group;
	uint32_t mods_lookup;
	enum weston_led leds = 0;
	int changed = 0;

	/* Serialize and update our internal state, checking to see if it's
	 * different to the previous state. */
	mods_depressed = xkb_state_serialize_mods(keyboard->xkb_state.state,
						  XKB_STATE_MODS_DEPRESSED);
	mods_latched = xkb_state_serialize_mods(keyboard->xkb_state.state,
						XKB_STATE_MODS_LATCHED);
	mods_locked = xkb_state_serialize_mods(keyboard->xkb_state.state,
					       XKB_STATE_MODS_LOCKED);
	group = xkb_state_serialize_layout(keyboard->xkb_state.state,
					   XKB_STATE_LAYOUT_EFFECTIVE);

	if (mods_depressed != keyboard->modifiers.mods_depressed ||
	    mods_latched != keyboard->modifiers.mods_latched ||
	    mods_locked != keyboard->modifiers.mods_locked ||
	    group != keyboard->modifiers.group)
		changed = 1;

	run_modifier_bindings(seat, keyboard->modifiers.mods_depressed,
	                      mods_depressed);

	keyboard->modifiers.mods_depressed = mods_depressed;
	keyboard->modifiers.mods_latched = mods_latched;
	keyboard->modifiers.mods_locked = mods_locked;
	keyboard->modifiers.group = group;

	/* And update the modifier_state for bindings. */
	mods_lookup = mods_depressed | mods_latched;
	seat->modifier_state = 0;
	if (mods_lookup & (1 << keyboard->xkb_info->ctrl_mod))
		seat->modifier_state |= MODIFIER_CTRL;
	if (mods_lookup & (1 << keyboard->xkb_info->alt_mod))
		seat->modifier_state |= MODIFIER_ALT;
	if (mods_lookup & (1 << keyboard->xkb_info->super_mod))
		seat->modifier_state |= MODIFIER_SUPER;
	if (mods_lookup & (1 << keyboard->xkb_info->shift_mod))
		seat->modifier_state |= MODIFIER_SHIFT;

	/* Finally, notify the compositor that LEDs have changed. */
	if (xkb_state_led_index_is_active(keyboard->xkb_state.state,
					  keyboard->xkb_info->num_led))
		leds |= LED_NUM_LOCK;
	if (xkb_state_led_index_is_active(keyboard->xkb_state.state,
					  keyboard->xkb_info->caps_led))
		leds |= LED_CAPS_LOCK;
	if (xkb_state_led_index_is_active(keyboard->xkb_state.state,
					  keyboard->xkb_info->scroll_led))
		leds |= LED_SCROLL_LOCK;
	if (leds != keyboard->xkb_state.leds && seat->led_update)
		seat->led_update(seat, leds);
	keyboard->xkb_state.leds = leds;

	if (changed) {
		grab->interface->modifiers(grab,
					   serial,
					   keyboard->modifiers.mods_depressed,
					   keyboard->modifiers.mods_latched,
					   keyboard->modifiers.mods_locked,
					   keyboard->modifiers.group);
	}
}

static void
update_modifier_state(struct weston_seat *seat, uint32_t serial, uint32_t key,
		      enum wl_keyboard_key_state state)
{
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	enum xkb_key_direction direction;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		direction = XKB_KEY_DOWN;
	else
		direction = XKB_KEY_UP;

	/* Offset the keycode by 8, as the evdev XKB rules reflect X's
	 * broken keycode system, which starts at 8. */
	xkb_state_update_key(keyboard->xkb_state.state, key + 8, direction);

	notify_modifiers(seat, serial);
}

WL_EXPORT void
weston_keyboard_send_keymap(struct weston_keyboard *kbd, struct wl_resource *resource)
{
	struct weston_xkb_info *xkb_info = kbd->xkb_info;
	int fd;
	size_t size;
	enum ro_anonymous_file_mapmode mapmode;

	if (wl_resource_get_version(resource) < 7)
		mapmode = RO_ANONYMOUS_FILE_MAPMODE_SHARED;
	else
		mapmode = RO_ANONYMOUS_FILE_MAPMODE_PRIVATE;

	fd = os_ro_anonymous_file_get_fd(xkb_info->keymap_rofile, mapmode);
	size = os_ro_anonymous_file_size(xkb_info->keymap_rofile);

	if (fd == -1) {
		weston_log("creating a keymap file failed: %s\n",
			   strerror(errno));
		return;
	}

	wl_keyboard_send_keymap(resource,
				WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
				fd,
				size);

	os_ro_anonymous_file_put_fd(fd);
}

static void
send_modifiers(struct wl_resource *resource, uint32_t serial, struct weston_keyboard *keyboard)
{
	wl_keyboard_send_modifiers(resource, serial,
				   keyboard->modifiers.mods_depressed,
				   keyboard->modifiers.mods_latched,
				   keyboard->modifiers.mods_locked,
				   keyboard->modifiers.group);
}

static struct weston_xkb_info *
weston_xkb_info_create(struct xkb_keymap *keymap);

static void
update_keymap(struct weston_seat *seat)
{
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct wl_resource *resource;
	struct weston_xkb_info *xkb_info;
	struct xkb_state *state;
	xkb_mod_mask_t latched_mods;
	xkb_mod_mask_t locked_mods;

	xkb_info = weston_xkb_info_create(keyboard->pending_keymap);

	xkb_keymap_unref(keyboard->pending_keymap);
	keyboard->pending_keymap = NULL;

	if (!xkb_info) {
		weston_log("failed to create XKB info\n");
		return;
	}

	state = xkb_state_new(xkb_info->keymap);
	if (!state) {
		weston_log("failed to initialise XKB state\n");
		weston_xkb_info_destroy(xkb_info);
		return;
	}

	latched_mods = xkb_state_serialize_mods(keyboard->xkb_state.state,
						XKB_STATE_MODS_LATCHED);
	locked_mods = xkb_state_serialize_mods(keyboard->xkb_state.state,
					       XKB_STATE_MODS_LOCKED);
	xkb_state_update_mask(state,
			      0, /* depressed */
			      latched_mods,
			      locked_mods,
			      0, 0, 0);

	weston_xkb_info_destroy(keyboard->xkb_info);
	keyboard->xkb_info = xkb_info;

	xkb_state_unref(keyboard->xkb_state.state);
	keyboard->xkb_state.state = state;

	wl_resource_for_each(resource, &keyboard->resource_list)
		weston_keyboard_send_keymap(keyboard, resource);
	wl_resource_for_each(resource, &keyboard->focus_resource_list)
		weston_keyboard_send_keymap(keyboard, resource);

	notify_modifiers(seat, wl_display_next_serial(seat->compositor->wl_display));

	if (!latched_mods && !locked_mods)
		return;

	wl_resource_for_each(resource, &keyboard->resource_list)
		send_modifiers(resource, wl_display_get_serial(seat->compositor->wl_display), keyboard);
	wl_resource_for_each(resource, &keyboard->focus_resource_list)
		send_modifiers(resource, wl_display_get_serial(seat->compositor->wl_display), keyboard);
}

WL_EXPORT void
notify_key(struct weston_seat *seat, const struct timespec *time, uint32_t key,
	   enum wl_keyboard_key_state state,
	   enum weston_key_state_update update_state)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_keyboard_grab *grab = keyboard->grab;
	uint32_t *k, *end;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		weston_compositor_idle_inhibit(compositor);
	} else {
		weston_compositor_idle_release(compositor);
	}

	end = keyboard->keys.data + keyboard->keys.size;
	for (k = keyboard->keys.data; k < end; k++) {
		if (*k == key) {
			/* Ignore server-generated repeats. */
			if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
				return;
			*k = *--end;
		}
	}
	keyboard->keys.size = (void *) end - keyboard->keys.data;
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		k = wl_array_add(&keyboard->keys, sizeof *k);
		*k = key;
	}

	if (grab == &keyboard->default_grab ||
	    grab == &keyboard->input_method_grab) {
		weston_compositor_run_key_binding(compositor, keyboard, time,
						  key, state);
		grab = keyboard->grab;
	}

	grab->interface->key(grab, time, key, state);

	if (keyboard->pending_keymap &&
	    keyboard->keys.size == 0)
		update_keymap(seat);

	if (update_state == STATE_UPDATE_AUTOMATIC) {
		update_modifier_state(seat,
				      wl_display_get_serial(compositor->wl_display),
				      key,
				      state);
	}

	keyboard->grab_serial = wl_display_get_serial(compositor->wl_display);
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		keyboard->grab_time = *time;
		keyboard->grab_key = key;
	}
}

WL_EXPORT void
notify_pointer_focus(struct weston_seat *seat, struct weston_output *output,
		     double x, double y)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (output) {
		weston_pointer_move_to(pointer,
				       wl_fixed_from_double(x),
				       wl_fixed_from_double(y));
	} else {
		/* FIXME: We should call weston_pointer_set_focus(seat,
		 * NULL) here, but somehow that breaks re-entry... */
	}
}

static void
destroy_device_saved_kbd_focus(struct wl_listener *listener, void *data)
{
	struct weston_seat *ws;

	ws = container_of(listener, struct weston_seat,
			  saved_kbd_focus_listener);

	ws->saved_kbd_focus = NULL;
}

WL_EXPORT void
notify_keyboard_focus_in(struct weston_seat *seat, struct wl_array *keys,
			 enum weston_key_state_update update_state)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_surface *surface;
	uint32_t *k, serial;

	serial = wl_display_next_serial(compositor->wl_display);
	wl_array_copy(&keyboard->keys, keys);
	wl_array_for_each(k, &keyboard->keys) {
		weston_compositor_idle_inhibit(compositor);
		if (update_state == STATE_UPDATE_AUTOMATIC)
			update_modifier_state(seat, serial, *k,
					      WL_KEYBOARD_KEY_STATE_PRESSED);
	}

	surface = seat->saved_kbd_focus;
	if (surface) {
		weston_keyboard_set_focus(keyboard, surface);
	}
}

WL_EXPORT void
notify_keyboard_focus_out(struct weston_seat *seat)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_surface *focus = keyboard->focus;
	uint32_t *k, serial;

	serial = wl_display_next_serial(compositor->wl_display);
	wl_array_for_each(k, &keyboard->keys) {
		weston_compositor_idle_release(compositor);
		update_modifier_state(seat, serial, *k,
				      WL_KEYBOARD_KEY_STATE_RELEASED);
	}

	seat->modifier_state = 0;

	weston_keyboard_set_focus(keyboard, NULL);
	weston_keyboard_cancel_grab(keyboard);
	if (pointer)
		weston_pointer_cancel_grab(pointer);

	if (focus) {
		seat->saved_kbd_focus = focus;
		seat->saved_kbd_focus_listener.notify =
			destroy_device_saved_kbd_focus;
		wl_signal_add(&focus->destroy_signal,
			      &seat->saved_kbd_focus_listener);
	}
}

WL_EXPORT void
weston_touch_set_focus(struct weston_touch *touch, struct weston_view *view)
{
	struct wl_list *focus_resource_list;

	focus_resource_list = &touch->focus_resource_list;

	if (view && touch->focus &&
	    touch->focus->surface == view->surface) {
		touch->focus = view;
		return;
	}

	wl_list_remove(&touch->focus_resource_listener.link);
	wl_list_init(&touch->focus_resource_listener.link);
	wl_list_remove(&touch->focus_view_listener.link);
	wl_list_init(&touch->focus_view_listener.link);

	if (!wl_list_empty(focus_resource_list)) {
		move_resources(&touch->resource_list,
			       focus_resource_list);
	}

	if (view) {
		struct wl_client *surface_client;

		if (!view->surface->resource) {
			touch->focus = NULL;
			return;
		}

		surface_client = wl_resource_get_client(view->surface->resource);
		move_resources_for_client(focus_resource_list,
					  &touch->resource_list,
					  surface_client);
		wl_resource_add_destroy_listener(view->surface->resource,
						 &touch->focus_resource_listener);
		wl_signal_add(&view->destroy_signal, &touch->focus_view_listener);
	}
	touch->focus = view;
}

static void
process_touch_normal(struct weston_touch_device *device,
		     const struct timespec *time, int touch_id,
		     double double_x, double double_y, int touch_type)
{
	struct weston_touch *touch = device->aggregate;
	struct weston_touch_grab *grab = device->aggregate->grab;
	struct weston_compositor *ec = device->aggregate->seat->compositor;
	struct weston_view *ev;
	wl_fixed_t sx, sy;
	wl_fixed_t x = wl_fixed_from_double(double_x);
	wl_fixed_t y = wl_fixed_from_double(double_y);

	/* Update grab's global coordinates. */
	if (touch_id == touch->grab_touch_id && touch_type != WL_TOUCH_UP) {
		touch->grab_x = x;
		touch->grab_y = y;
	}

	switch (touch_type) {
	case WL_TOUCH_DOWN:
		/* the first finger down picks the view, and all further go
		 * to that view for the remainder of the touch session i.e.
		 * until all touch points are up again. */
		if (touch->num_tp == 1) {
			ev = weston_compositor_pick_view(ec, x, y, &sx, &sy);
			weston_touch_set_focus(touch, ev);
		} else if (!touch->focus) {
			/* Unexpected condition: We have non-initial touch but
			 * there is no focused surface.
			 */
			weston_log("touch event received with %d points down "
				   "but no surface focused\n", touch->num_tp);
			return;
		}

		weston_compositor_run_touch_binding(ec, touch,
						    time, touch_type);

		grab->interface->down(grab, time, touch_id, x, y);
		if (touch->num_tp == 1) {
			touch->grab_serial =
				wl_display_get_serial(ec->wl_display);
			touch->grab_touch_id = touch_id;
			touch->grab_time = *time;
			touch->grab_x = x;
			touch->grab_y = y;
		}

		break;
	case WL_TOUCH_MOTION:
		ev = touch->focus;
		if (!ev)
			break;

		grab->interface->motion(grab, time, touch_id, x, y);
		break;
	case WL_TOUCH_UP:
		grab->interface->up(grab, time, touch_id);
		if (touch->num_tp == 0)
			weston_touch_set_focus(touch, NULL);
		break;
	}
}

static enum weston_touch_mode
get_next_touch_mode(enum weston_touch_mode from)
{
	switch (from) {
	case WESTON_TOUCH_MODE_PREP_NORMAL:
		return WESTON_TOUCH_MODE_NORMAL;

	case WESTON_TOUCH_MODE_PREP_CALIB:
		return WESTON_TOUCH_MODE_CALIB;

	case WESTON_TOUCH_MODE_NORMAL:
	case WESTON_TOUCH_MODE_CALIB:
		return from;
	}

	return WESTON_TOUCH_MODE_NORMAL;
}

/** Global touch mode update
 *
 * If no seat has a touch down and the compositor is in a PREP touch mode,
 * set the compositor to the goal touch mode.
 *
 * Calls calibrator if touch mode changed.
 */
static void
weston_compositor_update_touch_mode(struct weston_compositor *compositor)
{
	struct weston_seat *seat;
	struct weston_touch *touch;
	enum weston_touch_mode goal;

	wl_list_for_each(seat, &compositor->seat_list, link) {
		touch = weston_seat_get_touch(seat);
		if (!touch)
			continue;

		if (touch->num_tp > 0)
			return;
	}

	goal = get_next_touch_mode(compositor->touch_mode);
	if (compositor->touch_mode != goal) {
		compositor->touch_mode = goal;
		touch_calibrator_mode_changed(compositor);
	}
}

/** Start transition to normal touch event handling
 *
 * The touch event mode changes when all touches on all touch devices have
 * been lifted. If no touches are currently down, the transition is immediate.
 *
 * \sa weston_touch_mode
 */
void
weston_compositor_set_touch_mode_normal(struct weston_compositor *compositor)
{
	switch (compositor->touch_mode) {
	case WESTON_TOUCH_MODE_PREP_NORMAL:
	case WESTON_TOUCH_MODE_NORMAL:
		return;
	case WESTON_TOUCH_MODE_PREP_CALIB:
		compositor->touch_mode = WESTON_TOUCH_MODE_NORMAL;
		touch_calibrator_mode_changed(compositor);
		return;
	case WESTON_TOUCH_MODE_CALIB:
		compositor->touch_mode = WESTON_TOUCH_MODE_PREP_NORMAL;
	}

	weston_compositor_update_touch_mode(compositor);
}

/** Start transition to calibrator touch event handling
 *
 * The touch event mode changes when all touches on all touch devices have
 * been lifted. If no touches are currently down, the transition is immediate.
 *
 * \sa weston_touch_mode
 */
void
weston_compositor_set_touch_mode_calib(struct weston_compositor *compositor)
{
	switch (compositor->touch_mode) {
	case WESTON_TOUCH_MODE_PREP_CALIB:
	case WESTON_TOUCH_MODE_CALIB:
		assert(0);
		return;
	case WESTON_TOUCH_MODE_PREP_NORMAL:
		compositor->touch_mode = WESTON_TOUCH_MODE_CALIB;
		touch_calibrator_mode_changed(compositor);
		return;
	case WESTON_TOUCH_MODE_NORMAL:
		compositor->touch_mode = WESTON_TOUCH_MODE_PREP_CALIB;
	}

	weston_compositor_update_touch_mode(compositor);
}

/** Feed in touch down, motion, and up events, calibratable device.
 *
 * It assumes always the correct cycle sequence until it gets here: touch_down
 * → touch_update → ... → touch_update → touch_end. The driver is responsible
 * for sending along such order.
 *
 * \param device The physical device that generated the event.
 * \param time The event timestamp.
 * \param touch_id ID for the touch point of this event (multi-touch).
 * \param x X coordinate in compositor global space.
 * \param y Y coordinate in compositor global space.
 * \param norm Normalized device X, Y coordinates in calibration space, or NULL.
 * \param touch_type Either WL_TOUCH_DOWN, WL_TOUCH_UP, or WL_TOUCH_MOTION.
 *
 * Coordinates double_x and double_y are used for normal operation.
 *
 * Coordinates norm are only used for touch device calibration. If and only if
 * the weston_touch_device does not support calibrating, norm must be NULL.
 *
 * The calibration space is the normalized coordinate space
 * [0.0, 1.0]×[0.0, 1.0] of the weston_touch_device. This is assumed to
 * map to the similar normalized coordinate space of the associated
 * weston_output.
 */
WL_EXPORT void
notify_touch_normalized(struct weston_touch_device *device,
			const struct timespec *time,
			int touch_id,
			double x, double y,
			const struct weston_point2d_device_normalized *norm,
			int touch_type)
{
	struct weston_seat *seat = device->aggregate->seat;
	struct weston_touch *touch = device->aggregate;

	if (touch_type != WL_TOUCH_UP) {
		if (weston_touch_device_can_calibrate(device))
			assert(norm != NULL);
		else
			assert(norm == NULL);
	}

	/* Update touchpoints count regardless of the current mode. */
	switch (touch_type) {
	case WL_TOUCH_DOWN:
		weston_compositor_idle_inhibit(seat->compositor);

		touch->num_tp++;
		break;
	case WL_TOUCH_UP:
		if (touch->num_tp == 0) {
			/* This can happen if we start out with one or
			 * more fingers on the touch screen, in which
			 * case we didn't get the corresponding down
			 * event. */
			weston_log("Unmatched touch up event on seat %s, device %s\n",
				   seat->seat_name, device->syspath);
			return;
		}
		weston_compositor_idle_release(seat->compositor);

		touch->num_tp--;
		break;
	default:
		break;
	}

	/* Properly forward the touch event */
	switch (weston_touch_device_get_mode(device)) {
	case WESTON_TOUCH_MODE_NORMAL:
	case WESTON_TOUCH_MODE_PREP_CALIB:
		process_touch_normal(device, time, touch_id, x, y, touch_type);
		break;
	case WESTON_TOUCH_MODE_CALIB:
	case WESTON_TOUCH_MODE_PREP_NORMAL:
		notify_touch_calibrator(device, time, touch_id,
					norm, touch_type);
		break;
	}
}

WL_EXPORT void
notify_touch_frame(struct weston_touch_device *device)
{
	struct weston_touch_grab *grab;

	switch (weston_touch_device_get_mode(device)) {
	case WESTON_TOUCH_MODE_NORMAL:
	case WESTON_TOUCH_MODE_PREP_CALIB:
		grab = device->aggregate->grab;
		grab->interface->frame(grab);
		break;
	case WESTON_TOUCH_MODE_CALIB:
	case WESTON_TOUCH_MODE_PREP_NORMAL:
		notify_touch_calibrator_frame(device);
		break;
	}

	weston_compositor_update_touch_mode(device->aggregate->seat->compositor);
}

WL_EXPORT void
notify_touch_cancel(struct weston_touch_device *device)
{
	struct weston_touch_grab *grab;

	switch (weston_touch_device_get_mode(device)) {
	case WESTON_TOUCH_MODE_NORMAL:
	case WESTON_TOUCH_MODE_PREP_CALIB:
		grab = device->aggregate->grab;
		grab->interface->cancel(grab);
		break;
	case WESTON_TOUCH_MODE_CALIB:
	case WESTON_TOUCH_MODE_PREP_NORMAL:
		notify_touch_calibrator_cancel(device);
		break;
	}

	weston_compositor_update_touch_mode(device->aggregate->seat->compositor);
}

static int
pointer_cursor_surface_get_label(struct weston_surface *surface,
				 char *buf, size_t len)
{
	return snprintf(buf, len, "cursor");
}

static void
pointer_cursor_surface_committed(struct weston_surface *es,
				 int32_t dx, int32_t dy)
{
	struct weston_pointer *pointer = es->committed_private;
	int x, y;

	if (es->width == 0)
		return;

	assert(es == pointer->sprite->surface);

	pointer->hotspot_x -= dx;
	pointer->hotspot_y -= dy;

	x = wl_fixed_to_int(pointer->x) - pointer->hotspot_x;
	y = wl_fixed_to_int(pointer->y) - pointer->hotspot_y;

	weston_view_set_position(pointer->sprite, x, y);

	empty_region(&es->pending.input);
	empty_region(&es->input);

	if (!weston_surface_is_mapped(es)) {
		weston_layer_entry_insert(&es->compositor->cursor_layer.view_list,
					  &pointer->sprite->layer_link);
		weston_view_update_transform(pointer->sprite);
		es->is_mapped = true;
		pointer->sprite->is_mapped = true;
	}
}

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource,
		   uint32_t serial, struct wl_resource *surface_resource,
		   int32_t x, int32_t y)
{
	struct weston_pointer *pointer = wl_resource_get_user_data(resource);
	struct weston_surface *surface = NULL;

	if (!pointer)
		return;

	if (surface_resource)
		surface = wl_resource_get_user_data(surface_resource);

	if (pointer->focus == NULL)
		return;
	/* pointer->focus->surface->resource can be NULL. Surfaces like the
	black_surface used in shell.c for fullscreen don't have
	a resource, but can still have focus */
	if (pointer->focus->surface->resource == NULL)
		return;
	if (wl_resource_get_client(pointer->focus->surface->resource) != client)
		return;
	if (pointer->focus_serial - serial > UINT32_MAX / 2)
		return;

	if (!surface) {
		if (pointer->sprite)
			pointer_unmap_sprite(pointer);
		return;
	}

	if (pointer->sprite && pointer->sprite->surface == surface &&
	    pointer->hotspot_x == x && pointer->hotspot_y == y)
		return;

	if (!pointer->sprite || pointer->sprite->surface != surface) {
		if (weston_surface_set_role(surface, "wl_pointer-cursor",
					    resource,
					    WL_POINTER_ERROR_ROLE) < 0)
			return;

		if (pointer->sprite)
			pointer_unmap_sprite(pointer);

		wl_signal_add(&surface->destroy_signal,
			      &pointer->sprite_destroy_listener);

		surface->committed = pointer_cursor_surface_committed;
		surface->committed_private = pointer;
		weston_surface_set_label_func(surface,
					    pointer_cursor_surface_get_label);
		pointer->sprite = weston_view_create(surface);
	}

	pointer->hotspot_x = x;
	pointer->hotspot_y = y;

	if (surface->buffer_ref.buffer) {
		pointer_cursor_surface_committed(surface, 0, 0);
		weston_view_schedule_repaint(pointer->sprite);
	}
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_pointer_interface pointer_interface = {
	pointer_set_cursor,
	pointer_release
};

static void
seat_get_pointer(struct wl_client *client, struct wl_resource *resource,
		 uint32_t id)
{
	struct weston_seat *seat = wl_resource_get_user_data(resource);
	/* We use the pointer_state directly, which means we'll
	 * give a wl_pointer if the seat has ever had one - even though
	 * the spec explicitly states that this request only takes effect
	 * if the seat has the pointer capability.
	 *
	 * This prevents a race between the compositor sending new
	 * capabilities and the client trying to use the old ones.
	 */
	struct weston_pointer *pointer = seat ? seat->pointer_state : NULL;
	struct wl_resource *cr;
	struct weston_pointer_client *pointer_client;

        cr = wl_resource_create(client, &wl_pointer_interface,
				wl_resource_get_version(resource), id);
	if (cr == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_init(wl_resource_get_link(cr));
	wl_resource_set_implementation(cr, &pointer_interface, pointer,
				       unbind_pointer_client_resource);

	/* If we don't have a pointer_state, the resource is inert, so there
	 * is nothing more to set up */
	if (!pointer)
		return;

	pointer_client = weston_pointer_ensure_pointer_client(pointer, client);
	if (!pointer_client) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&pointer_client->pointer_resources,
		       wl_resource_get_link(cr));

	if (pointer->focus && pointer->focus->surface->resource &&
	    wl_resource_get_client(pointer->focus->surface->resource) == client) {
		wl_fixed_t sx, sy;

		weston_view_from_global_fixed(pointer->focus,
					      pointer->x,
					      pointer->y,
					      &sx, &sy);

		wl_pointer_send_enter(cr,
				      pointer->focus_serial,
				      pointer->focus->surface->resource,
				      sx, sy);
		pointer_send_frame(cr);
	}
}

static void
destroy_keyboard_resource(struct wl_resource *resource)
{
	struct weston_keyboard *keyboard = wl_resource_get_user_data(resource);

	wl_list_remove(wl_resource_get_link(resource));

	if (keyboard) {
		remove_input_resource_from_timestamps(resource,
						      &keyboard->timestamps_list);
	}
}

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_interface = {
	keyboard_release
};

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource,
		  uint32_t id)
{
	struct weston_seat *seat = wl_resource_get_user_data(resource);
	/* We use the keyboard_state directly, which means we'll
	 * give a wl_keyboard if the seat has ever had one - even though
	 * the spec explicitly states that this request only takes effect
	 * if the seat has the keyboard capability.
	 *
	 * This prevents a race between the compositor sending new
	 * capabilities and the client trying to use the old ones.
	 */
	struct weston_keyboard *keyboard = seat ? seat->keyboard_state : NULL;
	struct wl_resource *cr;

        cr = wl_resource_create(client, &wl_keyboard_interface,
				wl_resource_get_version(resource), id);
	if (cr == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_init(wl_resource_get_link(cr));
	wl_resource_set_implementation(cr, &keyboard_interface,
				       keyboard, destroy_keyboard_resource);

	/* If we don't have a keyboard_state, the resource is inert, so there
	 * is nothing more to set up */
	if (!keyboard)
		return;

	/* May be moved to focused list later by either
	 * weston_keyboard_set_focus or directly if this client is already
	 * focused */
	wl_list_insert(&keyboard->resource_list, wl_resource_get_link(cr));

	if (wl_resource_get_version(cr) >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
		wl_keyboard_send_repeat_info(cr,
					     seat->compositor->kb_repeat_rate,
					     seat->compositor->kb_repeat_delay);
	}

	weston_keyboard_send_keymap(keyboard, cr);

	if (keyboard->focus && keyboard->focus->resource &&
	    wl_resource_get_client(keyboard->focus->resource) == client) {
		struct weston_surface *surface =
			(struct weston_surface *)keyboard->focus;

		wl_list_remove(wl_resource_get_link(cr));
		wl_list_insert(&keyboard->focus_resource_list,
			       wl_resource_get_link(cr));
		wl_keyboard_send_enter(cr,
				       keyboard->focus_serial,
				       surface->resource,
				       &keyboard->keys);

		send_modifiers_to_resource(keyboard,
					   cr,
					   keyboard->focus_serial);

		/* If this is the first keyboard resource for this
		 * client... */
		if (keyboard->focus_resource_list.prev ==
		    wl_resource_get_link(cr))
			wl_data_device_set_keyboard_focus(seat);
	}
}

static void
destroy_touch_resource(struct wl_resource *resource)
{
	struct weston_touch *touch = wl_resource_get_user_data(resource);

	wl_list_remove(wl_resource_get_link(resource));

	if (touch) {
		remove_input_resource_from_timestamps(resource,
						      &touch->timestamps_list);
	}
}

static void
touch_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_touch_interface touch_interface = {
	touch_release
};

static void
seat_get_touch(struct wl_client *client, struct wl_resource *resource,
	       uint32_t id)
{
	struct weston_seat *seat = wl_resource_get_user_data(resource);
	/* We use the touch_state directly, which means we'll
	 * give a wl_touch if the seat has ever had one - even though
	 * the spec explicitly states that this request only takes effect
	 * if the seat has the touch capability.
	 *
	 * This prevents a race between the compositor sending new
	 * capabilities and the client trying to use the old ones.
	 */
	struct weston_touch *touch = seat ? seat->touch_state : NULL;
	struct wl_resource *cr;

        cr = wl_resource_create(client, &wl_touch_interface,
				wl_resource_get_version(resource), id);
	if (cr == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_init(wl_resource_get_link(cr));
	wl_resource_set_implementation(cr, &touch_interface,
				       touch, destroy_touch_resource);

	/* If we don't have a touch_state, the resource is inert, so there
	 * is nothing more to set up */
	if (!touch)
		return;

	if (touch->focus &&
	    wl_resource_get_client(touch->focus->surface->resource) == client) {
		wl_list_insert(&touch->focus_resource_list,
			       wl_resource_get_link(cr));
	} else {
		wl_list_insert(&touch->resource_list,
			       wl_resource_get_link(cr));
	}
}

static void
seat_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_seat_interface seat_interface = {
	seat_get_pointer,
	seat_get_keyboard,
	seat_get_touch,
	seat_release,
};

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct weston_seat *seat = data;
	struct wl_resource *resource;
	enum wl_seat_capability caps = 0;

	resource = wl_resource_create(client,
				      &wl_seat_interface, version, id);
	wl_list_insert(&seat->base_resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &seat_interface, data,
				       unbind_resource);

	if (weston_seat_get_pointer(seat))
		caps |= WL_SEAT_CAPABILITY_POINTER;
	if (weston_seat_get_keyboard(seat))
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (weston_seat_get_touch(seat))
		caps |= WL_SEAT_CAPABILITY_TOUCH;

	wl_seat_send_capabilities(resource, caps);
	if (version >= WL_SEAT_NAME_SINCE_VERSION)
		wl_seat_send_name(resource, seat->seat_name);
}

static void
relative_pointer_destroy(struct wl_client *client,
			 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct zwp_relative_pointer_v1_interface relative_pointer_interface = {
	relative_pointer_destroy
};

static void
relative_pointer_manager_destroy(struct wl_client *client,
				 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
relative_pointer_manager_get_relative_pointer(struct wl_client *client,
					      struct wl_resource *resource,
					      uint32_t id,
					      struct wl_resource *pointer_resource)
{
	struct weston_pointer *pointer =
		wl_resource_get_user_data(pointer_resource);
	struct weston_pointer_client *pointer_client;
	struct wl_resource *cr;

	cr = wl_resource_create(client, &zwp_relative_pointer_v1_interface,
				wl_resource_get_version(resource), id);
	if (cr == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	pointer_client = weston_pointer_ensure_pointer_client(pointer, client);
	if (!pointer_client) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&pointer_client->relative_pointer_resources,
		       wl_resource_get_link(cr));
	wl_resource_set_implementation(cr, &relative_pointer_interface,
				       pointer,
				       unbind_pointer_client_resource);
}

static const struct zwp_relative_pointer_manager_v1_interface relative_pointer_manager = {
	relative_pointer_manager_destroy,
	relative_pointer_manager_get_relative_pointer,
};

static void
bind_relative_pointer_manager(struct wl_client *client, void *data,
			      uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &zwp_relative_pointer_manager_v1_interface,
				      1, id);

	wl_resource_set_implementation(resource, &relative_pointer_manager,
				       compositor,
				       NULL);
}

WL_EXPORT int
weston_compositor_set_xkb_rule_names(struct weston_compositor *ec,
				     struct xkb_rule_names *names)
{
	if (ec->xkb_context == NULL) {
		ec->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		if (ec->xkb_context == NULL) {
			weston_log("failed to create XKB context\n");
			return -1;
		}
	}

	if (names)
		ec->xkb_names = *names;
	if (!ec->xkb_names.rules)
		ec->xkb_names.rules = strdup("evdev");
	if (!ec->xkb_names.model)
		ec->xkb_names.model = strdup("pc105");
	if (!ec->xkb_names.layout)
		ec->xkb_names.layout = strdup("us");

	return 0;
}

static void
weston_xkb_info_destroy(struct weston_xkb_info *xkb_info)
{
	if (--xkb_info->ref_count > 0)
		return;

	xkb_keymap_unref(xkb_info->keymap);

	os_ro_anonymous_file_destroy(xkb_info->keymap_rofile);
	free(xkb_info);
}

void
weston_compositor_xkb_destroy(struct weston_compositor *ec)
{
	free((char *) ec->xkb_names.rules);
	free((char *) ec->xkb_names.model);
	free((char *) ec->xkb_names.layout);
	free((char *) ec->xkb_names.variant);
	free((char *) ec->xkb_names.options);

	if (ec->xkb_info)
		weston_xkb_info_destroy(ec->xkb_info);
	xkb_context_unref(ec->xkb_context);
}

static struct weston_xkb_info *
weston_xkb_info_create(struct xkb_keymap *keymap)
{
	char *keymap_string;
	size_t keymap_size;
	struct weston_xkb_info *xkb_info = zalloc(sizeof *xkb_info);
	if (xkb_info == NULL)
		return NULL;

	xkb_info->keymap = xkb_keymap_ref(keymap);
	xkb_info->ref_count = 1;

	xkb_info->shift_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						       XKB_MOD_NAME_SHIFT);
	xkb_info->caps_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						      XKB_MOD_NAME_CAPS);
	xkb_info->ctrl_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						      XKB_MOD_NAME_CTRL);
	xkb_info->alt_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						     XKB_MOD_NAME_ALT);
	xkb_info->mod2_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						      "Mod2");
	xkb_info->mod3_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						      "Mod3");
	xkb_info->super_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						       XKB_MOD_NAME_LOGO);
	xkb_info->mod5_mod = xkb_keymap_mod_get_index(xkb_info->keymap,
						      "Mod5");

	xkb_info->num_led = xkb_keymap_led_get_index(xkb_info->keymap,
						     XKB_LED_NAME_NUM);
	xkb_info->caps_led = xkb_keymap_led_get_index(xkb_info->keymap,
						      XKB_LED_NAME_CAPS);
	xkb_info->scroll_led = xkb_keymap_led_get_index(xkb_info->keymap,
							XKB_LED_NAME_SCROLL);

	keymap_string = xkb_keymap_get_as_string(xkb_info->keymap,
							   XKB_KEYMAP_FORMAT_TEXT_V1);
	if (keymap_string == NULL) {
		weston_log("failed to get string version of keymap\n");
		goto err_keymap;
	}
	keymap_size = strlen(keymap_string) + 1;

	xkb_info->keymap_rofile = os_ro_anonymous_file_create(keymap_size,
							      keymap_string);
	free(keymap_string);

	if (!xkb_info->keymap_rofile) {
		weston_log("failed to create anonymous file for keymap\n");
		goto err_keymap;
	}

	return xkb_info;

err_keymap:
	xkb_keymap_unref(xkb_info->keymap);
	free(xkb_info);
	return NULL;
}

static int
weston_compositor_build_global_keymap(struct weston_compositor *ec)
{
	struct xkb_keymap *keymap;

	if (ec->xkb_info != NULL)
		return 0;

	keymap = xkb_keymap_new_from_names(ec->xkb_context,
					   &ec->xkb_names,
					   0);
	if (keymap == NULL) {
		weston_log("failed to compile global XKB keymap\n");
		weston_log("  tried rules %s, model %s, layout %s, variant %s, "
			"options %s\n",
			ec->xkb_names.rules, ec->xkb_names.model,
			ec->xkb_names.layout, ec->xkb_names.variant,
			ec->xkb_names.options);
		return -1;
	}

	ec->xkb_info = weston_xkb_info_create(keymap);
	xkb_keymap_unref(keymap);
	if (ec->xkb_info == NULL)
		return -1;

	return 0;
}

WL_EXPORT void
weston_seat_update_keymap(struct weston_seat *seat, struct xkb_keymap *keymap)
{
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);

	if (!keyboard || !keymap)
		return;

	xkb_keymap_unref(keyboard->pending_keymap);
	keyboard->pending_keymap = xkb_keymap_ref(keymap);

	if (keyboard->keys.size == 0)
		update_keymap(seat);
}

WL_EXPORT int
weston_seat_init_keyboard(struct weston_seat *seat, struct xkb_keymap *keymap)
{
	struct weston_keyboard *keyboard;

	if (seat->keyboard_state) {
		seat->keyboard_device_count += 1;
		if (seat->keyboard_device_count == 1)
			seat_send_updated_caps(seat);
		return 0;
	}

	keyboard = weston_keyboard_create();
	if (keyboard == NULL) {
		weston_log("failed to allocate weston keyboard struct\n");
		return -1;
	}

	if (keymap != NULL) {
		keyboard->xkb_info = weston_xkb_info_create(keymap);
		if (keyboard->xkb_info == NULL)
			goto err;
	} else {
		if (weston_compositor_build_global_keymap(seat->compositor) < 0)
			goto err;
		keyboard->xkb_info = seat->compositor->xkb_info;
		keyboard->xkb_info->ref_count++;
	}

	keyboard->xkb_state.state = xkb_state_new(keyboard->xkb_info->keymap);
	if (keyboard->xkb_state.state == NULL) {
		weston_log("failed to initialise XKB state\n");
		goto err;
	}

	keyboard->xkb_state.leds = 0;

	seat->keyboard_state = keyboard;
	seat->keyboard_device_count = 1;
	keyboard->seat = seat;

	seat_send_updated_caps(seat);

	return 0;

err:
	if (keyboard->xkb_info)
		weston_xkb_info_destroy(keyboard->xkb_info);
	free(keyboard);

	return -1;
}

static void
weston_keyboard_reset_state(struct weston_keyboard *keyboard)
{
	struct weston_seat *seat = keyboard->seat;
	struct xkb_state *state;

	state = xkb_state_new(keyboard->xkb_info->keymap);
	if (!state) {
		weston_log("failed to reset XKB state\n");
		return;
	}
	xkb_state_unref(keyboard->xkb_state.state);
	keyboard->xkb_state.state = state;

	keyboard->xkb_state.leds = 0;

	seat->modifier_state = 0;
}

WL_EXPORT void
weston_seat_release_keyboard(struct weston_seat *seat)
{
	seat->keyboard_device_count--;
	assert(seat->keyboard_device_count >= 0);
	if (seat->keyboard_device_count == 0) {
		weston_keyboard_set_focus(seat->keyboard_state, NULL);
		weston_keyboard_cancel_grab(seat->keyboard_state);
		weston_keyboard_reset_state(seat->keyboard_state);
		seat_send_updated_caps(seat);
	}
}

WL_EXPORT void
weston_seat_init_pointer(struct weston_seat *seat)
{
	struct weston_pointer *pointer;

	if (seat->pointer_state) {
		seat->pointer_device_count += 1;
		if (seat->pointer_device_count == 1)
			seat_send_updated_caps(seat);
		return;
	}

	pointer = weston_pointer_create(seat);
	if (pointer == NULL)
		return;

	seat->pointer_state = pointer;
	seat->pointer_device_count = 1;
	pointer->seat = seat;

	seat_send_updated_caps(seat);
}

WL_EXPORT void
weston_seat_release_pointer(struct weston_seat *seat)
{
	struct weston_pointer *pointer = seat->pointer_state;

	seat->pointer_device_count--;
	if (seat->pointer_device_count == 0) {
		weston_pointer_clear_focus(pointer);
		weston_pointer_cancel_grab(pointer);

		if (pointer->sprite)
			pointer_unmap_sprite(pointer);

		weston_pointer_reset_state(pointer);
		seat_send_updated_caps(seat);

		/* seat->pointer is intentionally not destroyed so that
		 * a newly attached pointer on this seat will retain
		 * the previous cursor co-ordinates.
		 */
	}
}

WL_EXPORT void
weston_seat_init_touch(struct weston_seat *seat)
{
	struct weston_touch *touch;

	if (seat->touch_state) {
		seat->touch_device_count += 1;
		if (seat->touch_device_count == 1)
			seat_send_updated_caps(seat);
		return;
	}

	touch = weston_touch_create();
	if (touch == NULL)
		return;

	seat->touch_state = touch;
	seat->touch_device_count = 1;
	touch->seat = seat;

	seat_send_updated_caps(seat);
}

WL_EXPORT void
weston_seat_release_touch(struct weston_seat *seat)
{
	seat->touch_device_count--;
	if (seat->touch_device_count == 0) {
		weston_touch_set_focus(seat->touch_state, NULL);
		weston_touch_cancel_grab(seat->touch_state);
		weston_touch_reset_state(seat->touch_state);
		seat_send_updated_caps(seat);
	}
}

WL_EXPORT void
weston_seat_init(struct weston_seat *seat, struct weston_compositor *ec,
		 const char *seat_name)
{
	memset(seat, 0, sizeof *seat);

	seat->selection_data_source = NULL;
	wl_list_init(&seat->base_resource_list);
	wl_signal_init(&seat->selection_signal);
	wl_list_init(&seat->drag_resource_list);
	wl_signal_init(&seat->destroy_signal);
	wl_signal_init(&seat->updated_caps_signal);

	seat->global = wl_global_create(ec->wl_display, &wl_seat_interface,
					MIN(wl_seat_interface.version, 7),
					seat, bind_seat);

	seat->compositor = ec;
	seat->modifier_state = 0;
	seat->seat_name = strdup(seat_name);

	wl_list_insert(ec->seat_list.prev, &seat->link);

	clipboard_create(seat);

	wl_signal_emit(&ec->seat_created_signal, seat);
}

WL_EXPORT void
weston_seat_release(struct weston_seat *seat)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, &seat->base_resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_resource_for_each(resource, &seat->drag_resource_list) {
		wl_resource_set_user_data(resource, NULL);
	}

	wl_list_remove(&seat->base_resource_list);
	wl_list_remove(&seat->drag_resource_list);

	wl_list_remove(&seat->link);

	if (seat->saved_kbd_focus)
		wl_list_remove(&seat->saved_kbd_focus_listener.link);

	if (seat->pointer_state)
		weston_pointer_destroy(seat->pointer_state);
	if (seat->keyboard_state)
		weston_keyboard_destroy(seat->keyboard_state);
	if (seat->touch_state)
		weston_touch_destroy(seat->touch_state);

	free (seat->seat_name);

	wl_global_destroy(seat->global);

	wl_signal_emit(&seat->destroy_signal, seat);
}

/** Get a seat's keyboard pointer
 *
 * \param seat The seat to query
 * \return The seat's keyboard pointer, or NULL if no keyboard is present
 *
 * The keyboard pointer for a seat isn't freed when all keyboards are removed,
 * so it should only be used when the seat's keyboard_device_count is greater
 * than zero.  This function does that test and only returns a pointer
 * when a keyboard is present.
 */
WL_EXPORT struct weston_keyboard *
weston_seat_get_keyboard(struct weston_seat *seat)
{
	if (!seat)
		return NULL;

	if (seat->keyboard_device_count)
		return seat->keyboard_state;

	return NULL;
}

/** Get a seat's pointer pointer
 *
 * \param seat The seat to query
 * \return The seat's pointer pointer, or NULL if no pointer device is present
 *
 * The pointer pointer for a seat isn't freed when all mice are removed,
 * so it should only be used when the seat's pointer_device_count is greater
 * than zero.  This function does that test and only returns a pointer
 * when a pointing device is present.
 */
WL_EXPORT struct weston_pointer *
weston_seat_get_pointer(struct weston_seat *seat)
{
	if (!seat)
		return NULL;

	if (seat->pointer_device_count)
		return seat->pointer_state;

	return NULL;
}

static const struct zwp_locked_pointer_v1_interface locked_pointer_interface;
static const struct zwp_confined_pointer_v1_interface confined_pointer_interface;

static enum pointer_constraint_type
pointer_constraint_get_type(struct weston_pointer_constraint *constraint)
{
	if (wl_resource_instance_of(constraint->resource,
				    &zwp_locked_pointer_v1_interface,
				    &locked_pointer_interface)) {
		return POINTER_CONSTRAINT_TYPE_LOCK;
	} else if (wl_resource_instance_of(constraint->resource,
					   &zwp_confined_pointer_v1_interface,
					   &confined_pointer_interface)) {
		return POINTER_CONSTRAINT_TYPE_CONFINE;
	}

	abort();
	return 0;
}

static void
pointer_constraint_notify_activated(struct weston_pointer_constraint *constraint)
{
	struct wl_resource *resource = constraint->resource;

	switch (pointer_constraint_get_type(constraint)) {
	case POINTER_CONSTRAINT_TYPE_LOCK:
		zwp_locked_pointer_v1_send_locked(resource);
		break;
	case POINTER_CONSTRAINT_TYPE_CONFINE:
		zwp_confined_pointer_v1_send_confined(resource);
		break;
	}
}

static void
pointer_constraint_notify_deactivated(struct weston_pointer_constraint *constraint)
{
	struct wl_resource *resource = constraint->resource;

	switch (pointer_constraint_get_type(constraint)) {
	case POINTER_CONSTRAINT_TYPE_LOCK:
		zwp_locked_pointer_v1_send_unlocked(resource);
		break;
	case POINTER_CONSTRAINT_TYPE_CONFINE:
		zwp_confined_pointer_v1_send_unconfined(resource);
		break;
	}
}

static struct weston_pointer_constraint *
get_pointer_constraint_for_pointer(struct weston_surface *surface,
				   struct weston_pointer *pointer)
{
	struct weston_pointer_constraint *constraint;

	wl_list_for_each(constraint, &surface->pointer_constraints, link) {
		if (constraint->pointer == pointer)
			return constraint;
	}

	return NULL;
}

/** Get a seat's touch pointer
 *
 * \param seat The seat to query
 * \return The seat's touch pointer, or NULL if no touch device is present
 *
 * The touch pointer for a seat isn't freed when all touch devices are removed,
 * so it should only be used when the seat's touch_device_count is greater
 * than zero.  This function does that test and only returns a pointer
 * when a touch device is present.
 */
WL_EXPORT struct weston_touch *
weston_seat_get_touch(struct weston_seat *seat)
{
	if (!seat)
		return NULL;

	if (seat->touch_device_count)
		return seat->touch_state;

	return NULL;
}

/** Sets the keyboard focus to the given surface
 *
 * \param surface the surface to focus on
 * \param seat The seat to query
 */
WL_EXPORT void
weston_seat_set_keyboard_focus(struct weston_seat *seat,
			       struct weston_surface *surface)
{
	struct weston_compositor *compositor = seat->compositor;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_surface_activation_data activation_data;

	if (keyboard && keyboard->focus != surface) {
		weston_keyboard_set_focus(keyboard, surface);
		wl_data_device_set_keyboard_focus(seat);
	}

	inc_activate_serial(compositor);

	activation_data = (struct weston_surface_activation_data) {
		.surface = surface,
		.seat = seat,
	};
	wl_signal_emit(&compositor->activate_signal, &activation_data);
}

static void
enable_pointer_constraint(struct weston_pointer_constraint *constraint,
			  struct weston_view *view)
{
	assert(constraint->view == NULL);
	constraint->view = view;
	pointer_constraint_notify_activated(constraint);
	weston_pointer_start_grab(constraint->pointer, &constraint->grab);
	wl_list_remove(&constraint->surface_destroy_listener.link);
	wl_list_init(&constraint->surface_destroy_listener.link);
}

static bool
is_pointer_constraint_enabled(struct weston_pointer_constraint *constraint)
{
	return constraint->view != NULL;
}

static void
weston_pointer_constraint_disable(struct weston_pointer_constraint *constraint)
{
	constraint->view = NULL;
	pointer_constraint_notify_deactivated(constraint);
	weston_pointer_end_grab(constraint->grab.pointer);
}

void
weston_pointer_constraint_destroy(struct weston_pointer_constraint *constraint)
{
	if (is_pointer_constraint_enabled(constraint))
		weston_pointer_constraint_disable(constraint);

	wl_list_remove(&constraint->pointer_destroy_listener.link);
	wl_list_remove(&constraint->surface_destroy_listener.link);
	wl_list_remove(&constraint->surface_commit_listener.link);
	wl_list_remove(&constraint->surface_activate_listener.link);

	wl_resource_set_user_data(constraint->resource, NULL);
	pixman_region32_fini(&constraint->region);
	wl_list_remove(&constraint->link);
	free(constraint);
}

static void
disable_pointer_constraint(struct weston_pointer_constraint *constraint)
{
	switch (constraint->lifetime) {
	case ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT:
		weston_pointer_constraint_destroy(constraint);
		break;
	case ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT:
		weston_pointer_constraint_disable(constraint);
		break;
	}
}

static bool
is_within_constraint_region(struct weston_pointer_constraint *constraint,
			    wl_fixed_t sx, wl_fixed_t sy)
{
	struct weston_surface *surface = constraint->surface;
	pixman_region32_t constraint_region;
	bool result;

	pixman_region32_init(&constraint_region);
	pixman_region32_intersect(&constraint_region,
				  &surface->input,
				  &constraint->region);
	result = pixman_region32_contains_point(&constraint_region,
						wl_fixed_to_int(sx),
						wl_fixed_to_int(sy),
						NULL);
	pixman_region32_fini(&constraint_region);

	return result;
}

static void
maybe_enable_pointer_constraint(struct weston_pointer_constraint *constraint)
{
	struct weston_surface *surface = constraint->surface;
	struct weston_view *vit;
	struct weston_view *view = NULL;
	struct weston_pointer *pointer = constraint->pointer;
	struct weston_keyboard *keyboard;
	struct weston_seat *seat = pointer->seat;
	int32_t x, y;

	/* Postpone if no view of the surface was most recently clicked. */
	wl_list_for_each(vit, &surface->views, surface_link) {
		if (vit->click_to_activate_serial ==
		    surface->compositor->activate_serial) {
			view = vit;
		}
	}
	if (view == NULL)
		return;

	/* Postpone if surface doesn't have keyboard focus. */
	keyboard = weston_seat_get_keyboard(seat);
	if (!keyboard || keyboard->focus != surface)
		return;

	/* Postpone constraint if the pointer is not within the
	 * constraint region.
	 */
	weston_view_from_global(view,
				wl_fixed_to_int(pointer->x),
				wl_fixed_to_int(pointer->y),
				&x, &y);
	if (!is_within_constraint_region(constraint,
					 wl_fixed_from_int(x),
					 wl_fixed_from_int(y)))
		return;

	enable_pointer_constraint(constraint, view);
}

static void
locked_pointer_grab_pointer_focus(struct weston_pointer_grab *grab)
{
}

static void
locked_pointer_grab_pointer_motion(struct weston_pointer_grab *grab,
				   const struct timespec *time,
				   struct weston_pointer_motion_event *event)
{
	pointer_send_relative_motion(grab->pointer, time, event);
}

static void
locked_pointer_grab_pointer_button(struct weston_pointer_grab *grab,
				   const struct timespec *time,
				   uint32_t button,
				   uint32_t state_w)
{
	weston_pointer_send_button(grab->pointer, time, button, state_w);
}

static void
locked_pointer_grab_pointer_axis(struct weston_pointer_grab *grab,
				 const struct timespec *time,
				 struct weston_pointer_axis_event *event)
{
	weston_pointer_send_axis(grab->pointer, time, event);
}

static void
locked_pointer_grab_pointer_axis_source(struct weston_pointer_grab *grab,
					uint32_t source)
{
	weston_pointer_send_axis_source(grab->pointer, source);
}

static void
locked_pointer_grab_pointer_frame(struct weston_pointer_grab *grab)
{
	weston_pointer_send_frame(grab->pointer);
}

static void
locked_pointer_grab_pointer_cancel(struct weston_pointer_grab *grab)
{
	struct weston_pointer_constraint *constraint =
		container_of(grab, struct weston_pointer_constraint, grab);

	disable_pointer_constraint(constraint);
}

static const struct weston_pointer_grab_interface
				locked_pointer_grab_interface = {
	locked_pointer_grab_pointer_focus,
	locked_pointer_grab_pointer_motion,
	locked_pointer_grab_pointer_button,
	locked_pointer_grab_pointer_axis,
	locked_pointer_grab_pointer_axis_source,
	locked_pointer_grab_pointer_frame,
	locked_pointer_grab_pointer_cancel,
};

static void
pointer_constraint_constrain_resource_destroyed(struct wl_resource *resource)
{
	struct weston_pointer_constraint *constraint =
		wl_resource_get_user_data(resource);

	if (!constraint)
		return;

	weston_pointer_constraint_destroy(constraint);
}

static void
pointer_constraint_surface_activate(struct wl_listener *listener, void *data)
{
	struct weston_surface_activation_data *activation = data;
	struct weston_pointer *pointer;
	struct weston_surface *focus = activation->surface;
	struct weston_pointer_constraint *constraint =
		container_of(listener, struct weston_pointer_constraint,
			     surface_activate_listener);
	bool is_constraint_surface;

	pointer = weston_seat_get_pointer(activation->seat);
	if (!pointer)
		return;

	is_constraint_surface =
		get_pointer_constraint_for_pointer(focus, pointer) == constraint;

	if (is_constraint_surface &&
	    !is_pointer_constraint_enabled(constraint))
		maybe_enable_pointer_constraint(constraint);
	else if (!is_constraint_surface &&
		 is_pointer_constraint_enabled(constraint))
		disable_pointer_constraint(constraint);
}

static void
pointer_constraint_pointer_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_pointer_constraint *constraint =
		container_of(listener, struct weston_pointer_constraint,
			     pointer_destroy_listener);

	weston_pointer_constraint_destroy(constraint);
}

static void
pointer_constraint_surface_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_pointer_constraint *constraint =
		container_of(listener, struct weston_pointer_constraint,
			     surface_destroy_listener);

	weston_pointer_constraint_destroy(constraint);
}

static void
pointer_constraint_surface_committed(struct wl_listener *listener, void *data)
{
	struct weston_pointer_constraint *constraint =
		container_of(listener, struct weston_pointer_constraint,
			     surface_commit_listener);

	if (constraint->region_is_pending) {
		constraint->region_is_pending = false;
		pixman_region32_copy(&constraint->region,
				     &constraint->region_pending);
		pixman_region32_fini(&constraint->region_pending);
		pixman_region32_init(&constraint->region_pending);
	}

	if (constraint->hint_is_pending) {
		constraint->hint_is_pending = false;

		constraint->hint_is_pending = true;
		constraint->hint_x = constraint->hint_x_pending;
		constraint->hint_y = constraint->hint_y_pending;
	}

	if (pointer_constraint_get_type(constraint) ==
	    POINTER_CONSTRAINT_TYPE_CONFINE &&
	    is_pointer_constraint_enabled(constraint))
		maybe_warp_confined_pointer(constraint);
}

static struct weston_pointer_constraint *
weston_pointer_constraint_create(struct weston_surface *surface,
				 struct weston_pointer *pointer,
				 struct weston_region *region,
				 enum zwp_pointer_constraints_v1_lifetime lifetime,
				 struct wl_resource *cr,
				 const struct weston_pointer_grab_interface *grab_interface)
{
	struct weston_pointer_constraint *constraint;

	constraint = zalloc(sizeof *constraint);
	if (!constraint)
		return NULL;

	constraint->lifetime = lifetime;
	pixman_region32_init(&constraint->region);
	pixman_region32_init(&constraint->region_pending);
	wl_list_insert(&surface->pointer_constraints, &constraint->link);
	constraint->surface = surface;
	constraint->pointer = pointer;
	constraint->resource = cr;
	constraint->grab.interface = grab_interface;
	if (region) {
		pixman_region32_copy(&constraint->region,
				     &region->region);
	} else {
		pixman_region32_fini(&constraint->region);
		region_init_infinite(&constraint->region);
	}

	constraint->surface_activate_listener.notify =
		pointer_constraint_surface_activate;
	constraint->surface_destroy_listener.notify =
		pointer_constraint_surface_destroyed;
	constraint->surface_commit_listener.notify =
		pointer_constraint_surface_committed;
	constraint->pointer_destroy_listener.notify =
		pointer_constraint_pointer_destroyed;

	wl_signal_add(&surface->compositor->activate_signal,
		      &constraint->surface_activate_listener);
	wl_signal_add(&pointer->destroy_signal,
		      &constraint->pointer_destroy_listener);
	wl_signal_add(&surface->destroy_signal,
		      &constraint->surface_destroy_listener);
	wl_signal_add(&surface->commit_signal,
		      &constraint->surface_commit_listener);

	return constraint;
}

static void
init_pointer_constraint(struct wl_resource *pointer_constraints_resource,
			uint32_t id,
			struct weston_surface *surface,
			struct weston_pointer *pointer,
			struct weston_region *region,
			enum zwp_pointer_constraints_v1_lifetime lifetime,
			const struct wl_interface *interface,
			const void *implementation,
			const struct weston_pointer_grab_interface *grab_interface)
{
	struct wl_client *client =
		wl_resource_get_client(pointer_constraints_resource);
	struct wl_resource *cr;
	struct weston_pointer_constraint *constraint;

	if (pointer && get_pointer_constraint_for_pointer(surface, pointer)) {
		wl_resource_post_error(pointer_constraints_resource,
				       ZWP_POINTER_CONSTRAINTS_V1_ERROR_ALREADY_CONSTRAINED,
				       "the pointer has a lock/confine request on this surface");
		return;
	}

	cr = wl_resource_create(client, interface,
				wl_resource_get_version(pointer_constraints_resource),
				id);
	if (cr == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (pointer) {
		constraint = weston_pointer_constraint_create(surface, pointer,
							      region, lifetime,
							      cr, grab_interface);
		if (constraint == NULL) {
			wl_client_post_no_memory(client);
			return;
		}
	} else {
		constraint = NULL;
	}

	wl_resource_set_implementation(cr, implementation, constraint,
				       pointer_constraint_constrain_resource_destroyed);

	if (constraint)
		maybe_enable_pointer_constraint(constraint);
}

static void
pointer_constraints_destroy(struct wl_client *client,
			    struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
locked_pointer_destroy(struct wl_client *client,
		       struct wl_resource *resource)
{
	struct weston_pointer_constraint *constraint =
		wl_resource_get_user_data(resource);
	wl_fixed_t x, y;

	if (constraint && constraint->view && constraint->hint_is_pending &&
	    is_within_constraint_region(constraint,
					constraint->hint_x,
					constraint->hint_y)) {
		weston_view_to_global_fixed(constraint->view,
					    constraint->hint_x,
					    constraint->hint_y,
					    &x, &y);
		weston_pointer_move_to(constraint->pointer, x, y);
	}
	wl_resource_destroy(resource);
}

static void
locked_pointer_set_cursor_position_hint(struct wl_client *client,
					struct wl_resource *resource,
					wl_fixed_t surface_x,
					wl_fixed_t surface_y)
{
	struct weston_pointer_constraint *constraint =
		wl_resource_get_user_data(resource);

	/* Ignore a set cursor hint that was sent after the lock was cancelled.
	 */
	if (!constraint ||
	    !constraint->resource ||
	    constraint->resource != resource)
		return;

	constraint->hint_is_pending = true;
	constraint->hint_x_pending = surface_x;
	constraint->hint_y_pending = surface_y;
}

static void
locked_pointer_set_region(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *region_resource)
{
	struct weston_pointer_constraint *constraint =
		wl_resource_get_user_data(resource);
	struct weston_region *region = region_resource ?
		wl_resource_get_user_data(region_resource) : NULL;

	if (!constraint)
		return;

	if (region) {
		pixman_region32_copy(&constraint->region_pending,
				     &region->region);
	} else {
		pixman_region32_fini(&constraint->region_pending);
		region_init_infinite(&constraint->region_pending);
	}
	constraint->region_is_pending = true;
}


static const struct zwp_locked_pointer_v1_interface locked_pointer_interface = {
	locked_pointer_destroy,
	locked_pointer_set_cursor_position_hint,
	locked_pointer_set_region,
};

static void
pointer_constraints_lock_pointer(struct wl_client *client,
				 struct wl_resource *resource,
				 uint32_t id,
				 struct wl_resource *surface_resource,
				 struct wl_resource *pointer_resource,
				 struct wl_resource *region_resource,
				 uint32_t lifetime)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_pointer *pointer = wl_resource_get_user_data(pointer_resource);
	struct weston_region *region = region_resource ?
		wl_resource_get_user_data(region_resource) : NULL;

	init_pointer_constraint(resource, id, surface, pointer, region, lifetime,
				&zwp_locked_pointer_v1_interface,
				&locked_pointer_interface,
				&locked_pointer_grab_interface);
}

static void
confined_pointer_grab_pointer_focus(struct weston_pointer_grab *grab)
{
}

static double
vec2d_cross_product(struct vec2d a, struct vec2d b)
{
	return a.x * b.y - a.y * b.x;
}

static struct vec2d
vec2d_add(struct vec2d a, struct vec2d b)
{
	return (struct vec2d) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}

static struct vec2d
vec2d_subtract(struct vec2d a, struct vec2d b)
{
	return (struct vec2d) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}

static struct vec2d
vec2d_multiply_constant(double c, struct vec2d a)
{
	return (struct vec2d) {
		.x = c * a.x,
		.y = c * a.y,
	};
}

static bool
lines_intersect(struct line *line1, struct line *line2,
		struct vec2d *intersection)
{
	struct vec2d p = line1->a;
	struct vec2d r = vec2d_subtract(line1->b, line1->a);
	struct vec2d q = line2->a;
	struct vec2d s = vec2d_subtract(line2->b, line2->a);
	double rxs;
	double sxr;
	double t;
	double u;

	/*
	 * The line (p, r) and (q, s) intersects where
	 *
	 *   p + t r = q + u s
	 *
	 * Calculate t:
	 *
	 *   (p + t r) × s = (q + u s) × s
	 *   p × s + t (r × s) = q × s + u (s × s)
	 *   p × s + t (r × s) = q × s
	 *   t (r × s) = q × s - p × s
	 *   t (r × s) = (q - p) × s
	 *   t = ((q - p) × s) / (r × s)
	 *
	 * Using the same method, for u we get:
	 *
	 *   u = ((p - q) × r) / (s × r)
	 */

	rxs = vec2d_cross_product(r, s);
	sxr = vec2d_cross_product(s, r);

	/* If r × s = 0 then the lines are either parallel or collinear. */
	if (fabs(rxs) < DBL_MIN)
		return false;

	t = vec2d_cross_product(vec2d_subtract(q, p), s) / rxs;
	u = vec2d_cross_product(vec2d_subtract(p, q), r) / sxr;

	/* The lines only intersect if 0 ≤ t ≤ 1 and 0 ≤ u ≤ 1. */
	if (t < 0.0 || t > 1.0 || u < 0.0 || u > 1.0)
		return false;

	*intersection = vec2d_add(p, vec2d_multiply_constant(t, r));
	return true;
}

static struct border *
add_border(struct wl_array *array,
	   double x1, double y1,
	   double x2, double y2,
	   enum motion_direction blocking_dir)
{
	struct border *border = wl_array_add(array, sizeof *border);

	*border = (struct border) {
		.line = (struct line) {
			.a = (struct vec2d) {
				.x = x1,
				.y = y1,
			},
			.b = (struct vec2d) {
				.x = x2,
				.y = y2,
			},
		},
		.blocking_dir = blocking_dir,
	};

	return border;
}

static int
compare_lines_x(const void *a, const void *b)
{
	const struct border *border_a = a;
	const struct border *border_b = b;


	if (border_a->line.a.x == border_b->line.a.x)
		return border_a->line.b.x < border_b->line.b.x;
	else
		return border_a->line.a.x > border_b->line.a.x;
}

static void
add_non_overlapping_edges(pixman_box32_t *boxes,
			  int band_above_start,
			  int band_below_start,
			  int band_below_end,
			  struct wl_array *borders)
{
	int i;
	struct wl_array band_merge;
	struct border *border;
	struct border *prev_border;
	struct border *new_border;

	wl_array_init(&band_merge);

	/* Add bottom band of previous row, and top band of current row, and
	 * sort them so lower left x coordinate comes first. If there are two
	 * borders with the same left x coordinate, the wider one comes first.
	 */
	for (i = band_above_start; i < band_below_start; i++) {
		pixman_box32_t *box = &boxes[i];
		add_border(&band_merge, box->x1, box->y2, box->x2, box->y2,
			   MOTION_DIRECTION_POSITIVE_Y);
	}
	for (i = band_below_start; i < band_below_end; i++) {
		pixman_box32_t *box= &boxes[i];
		add_border(&band_merge, box->x1, box->y1, box->x2, box->y1,
			   MOTION_DIRECTION_NEGATIVE_Y);
	}
	qsort(band_merge.data,
	      band_merge.size / sizeof *border,
	      sizeof *border,
	      compare_lines_x);

	/* Combine the two combined bands so that any overlapping border is
	 * eliminated. */
	prev_border = NULL;
	wl_array_for_each(border, &band_merge) {
		assert(border->line.a.y == border->line.b.y);
		assert(!prev_border ||
		       prev_border->line.a.y == border->line.a.y);
		assert(!prev_border ||
		       (prev_border->line.a.x != border->line.a.x ||
			prev_border->line.b.x != border->line.b.x));
		assert(!prev_border ||
		       prev_border->line.a.x <= border->line.a.x);

		if (prev_border &&
		    prev_border->line.a.x == border->line.a.x) {
			/*
			 * ------------ +
			 * -------      =
			 * [     ]-----
			 */
			prev_border->line.a.x = border->line.b.x;
		} else if (prev_border &&
			   prev_border->line.b.x == border->line.b.x) {
			/*
			 * ------------ +
			 *       ------ =
			 * ------[    ]
			 */
			prev_border->line.b.x = border->line.a.x;
		} else if (prev_border &&
			   prev_border->line.b.x == border->line.a.x) {
			/*
			 * --------        +
			 *         ------  =
			 * --------------
			 */
			prev_border->line.b.x = border->line.b.x;
		} else if (prev_border &&
			   prev_border->line.b.x >= border->line.a.x) {
			/*
			 * --------------- +
			 *      ------     =
			 * -----[    ]----
			 */
			new_border = add_border(borders,
						border->line.b.x,
						border->line.b.y,
						prev_border->line.b.x,
						prev_border->line.b.y,
						prev_border->blocking_dir);
			prev_border->line.b.x = border->line.a.x;
			prev_border = new_border;
		} else {
			assert(!prev_border ||
			       prev_border->line.b.x < border->line.a.x);
			/*
			 * First border or non-overlapping.
			 *
			 * -----           +
			 *        -----    =
			 * -----  -----
			 */
			new_border = wl_array_add(borders, sizeof *border);
			*new_border = *border;
			prev_border = new_border;
		}
	}

	wl_array_release(&band_merge);
}

static void
add_band_bottom_edges(pixman_box32_t *boxes,
		      int band_start,
		      int band_end,
		      struct wl_array *borders)
{
	int i;

	for (i = band_start; i < band_end; i++) {
		add_border(borders,
			   boxes[i].x1, boxes[i].y2,
			   boxes[i].x2, boxes[i].y2,
			   MOTION_DIRECTION_POSITIVE_Y);
	}
}

static void
region_to_outline(pixman_region32_t *region, struct wl_array *borders)
{
	pixman_box32_t *boxes;
	int num_boxes;
	int i;
	int top_most, bottom_most;
	int current_roof;
	int prev_top;
	int band_start, prev_band_start;

	/*
	 * Remove any overlapping lines from the set of rectangles. Note that
	 * pixman regions are grouped as rows of rectangles, where rectangles
	 * in one row never touch or overlap and are all of the same height.
	 *
	 *             -------- ---                   -------- ---
	 *             |      | | |                   |      | | |
	 *   ----------====---- ---         -----------  ----- ---
	 *   |            |            =>   |            |
	 *   ----==========---------        -----        ----------
	 *       |                 |	        |                 |
	 *       -------------------            -------------------
	 *
	 */

	boxes = pixman_region32_rectangles(region, &num_boxes);
	prev_top = 0;
	top_most = boxes[0].y1;
	current_roof = top_most;
	bottom_most = boxes[num_boxes - 1].y2;
	band_start = 0;
	prev_band_start = 0;
	for (i = 0; i < num_boxes; i++) {
		/* Detect if there is a vertical empty space, and add the lower
		 * level of the previous band if so was the case. */
		if (i > 0 &&
		    boxes[i].y1 != prev_top &&
		    boxes[i].y1 != boxes[i - 1].y2) {
			current_roof = boxes[i].y1;
			add_band_bottom_edges(boxes,
					      band_start,
					      i,
					      borders);
		}

		/* Special case adding the last band, since it won't be handled
		 * by the band change detection below. */
		if (boxes[i].y1 != current_roof && i == num_boxes - 1) {
			if (boxes[i].y1 != prev_top) {
				/* The last band is a single box, so we don't
				 * have a prev_band_start to tell us when the
				 * previous band started. */
				add_non_overlapping_edges(boxes,
							  band_start,
							  i,
							  i + 1,
							  borders);
			} else {
				add_non_overlapping_edges(boxes,
							  prev_band_start,
							  band_start,
							  i + 1,
							  borders);
			}
		}

		/* Detect when passing a band and combine the top border of the
		 * just passed band with the bottom band of the previous band.
		 */
		if (boxes[i].y1 != top_most && boxes[i].y1 != prev_top) {
			/* Combine the two passed bands. */
			if (prev_top != current_roof) {
				add_non_overlapping_edges(boxes,
							  prev_band_start,
							  band_start,
							  i,
							  borders);
			}

			prev_band_start = band_start;
			band_start = i;
		}

		/* Add the top border if the box is part of the current roof. */
		if (boxes[i].y1 == current_roof) {
			add_border(borders,
				   boxes[i].x1, boxes[i].y1,
				   boxes[i].x2, boxes[i].y1,
				   MOTION_DIRECTION_NEGATIVE_Y);
		}

		/* Add the bottom border of the last band. */
		if (boxes[i].y2 == bottom_most) {
			add_border(borders,
				   boxes[i].x1, boxes[i].y2,
				   boxes[i].x2, boxes[i].y2,
				   MOTION_DIRECTION_POSITIVE_Y);
		}

		/* Always add the left border. */
		add_border(borders,
			   boxes[i].x1, boxes[i].y1,
			   boxes[i].x1, boxes[i].y2,
			   MOTION_DIRECTION_NEGATIVE_X);

		/* Always add the right border. */
		add_border(borders,
			   boxes[i].x2, boxes[i].y1,
			   boxes[i].x2, boxes[i].y2,
			   MOTION_DIRECTION_POSITIVE_X);

		prev_top = boxes[i].y1;
	}
}

static bool
is_border_horizontal (struct border *border)
{
	return border->line.a.y == border->line.b.y;
}

static bool
is_border_blocking_directions(struct border *border,
			      uint32_t directions)
{
	/* Don't block parallel motions. */
	if (is_border_horizontal(border)) {
		if ((directions & (MOTION_DIRECTION_POSITIVE_Y |
				   MOTION_DIRECTION_NEGATIVE_Y)) == 0)
			return false;
	} else {
		if ((directions & (MOTION_DIRECTION_POSITIVE_X |
				   MOTION_DIRECTION_NEGATIVE_X)) == 0)
			return false;
	}

	return (~border->blocking_dir & directions) != directions;
}

static struct border *
get_closest_border(struct wl_array *borders,
		   struct line *motion,
		   uint32_t directions)
{
	struct border *border;
	struct vec2d intersection;
	struct vec2d delta;
	double distance_2;
	struct border *closest_border = NULL;
	double closest_distance_2 = DBL_MAX;

	wl_array_for_each(border, borders) {
		if (!is_border_blocking_directions(border, directions))
			continue;

		if (!lines_intersect(&border->line, motion, &intersection))
			continue;

		delta = vec2d_subtract(intersection, motion->a);
		distance_2 = delta.x*delta.x + delta.y*delta.y;
		if (distance_2 < closest_distance_2) {
			closest_border = border;
			closest_distance_2 = distance_2;
		}
	}

	return closest_border;
}

static void
clamp_to_border(struct border *border,
		struct line *motion,
		uint32_t *motion_dir)
{
	/*
	 * When clamping either rightward or downward motions, the motion needs
	 * to be clamped so that the destination coordinate does not end up on
	 * the border (see weston_pointer_clamp_event_to_region). Do this by
	 * clamping such motions to the border minus the smallest possible
	 * wl_fixed_t value.
	 */
	if (is_border_horizontal(border)) {
		if (*motion_dir & MOTION_DIRECTION_POSITIVE_Y)
			motion->b.y = border->line.a.y - wl_fixed_to_double(1);
		else
			motion->b.y = border->line.a.y;
		*motion_dir &= ~(MOTION_DIRECTION_POSITIVE_Y |
				 MOTION_DIRECTION_NEGATIVE_Y);
	} else {
		if (*motion_dir & MOTION_DIRECTION_POSITIVE_X)
			motion->b.x = border->line.a.x - wl_fixed_to_double(1);
		else
			motion->b.x = border->line.a.x;
		*motion_dir &= ~(MOTION_DIRECTION_POSITIVE_X |
				 MOTION_DIRECTION_NEGATIVE_X);
	}
}

static uint32_t
get_motion_directions(struct line *motion)
{
	uint32_t directions = 0;

	if (motion->a.x < motion->b.x)
		directions |= MOTION_DIRECTION_POSITIVE_X;
	else if (motion->a.x > motion->b.x)
		directions |= MOTION_DIRECTION_NEGATIVE_X;
	if (motion->a.y < motion->b.y)
		directions |= MOTION_DIRECTION_POSITIVE_Y;
	else if (motion->a.y > motion->b.y)
		directions |= MOTION_DIRECTION_NEGATIVE_Y;

	return directions;
}

static void
weston_pointer_clamp_event_to_region(struct weston_pointer *pointer,
				     struct weston_pointer_motion_event *event,
				     pixman_region32_t *region,
				     wl_fixed_t *clamped_x,
				     wl_fixed_t *clamped_y)
{
	wl_fixed_t x, y;
	wl_fixed_t sx, sy;
	wl_fixed_t old_sx = pointer->sx;
	wl_fixed_t old_sy = pointer->sy;
	struct wl_array borders;
	struct line motion;
	struct border *closest_border;
	float new_x_f, new_y_f;
	uint32_t directions;

	weston_pointer_motion_to_abs(pointer, event, &x, &y);
	weston_view_from_global_fixed(pointer->focus, x, y, &sx, &sy);

	wl_array_init(&borders);

	/*
	 * Generate borders given the confine region we are to use. The borders
	 * are defined to be the outer region of the allowed area. This means
	 * top/left borders are "within" the allowed area, while bottom/right
	 * borders are outside. This needs to be considered when clamping
	 * confined motion vectors.
	 */
	region_to_outline(region, &borders);

	motion = (struct line) {
		.a = (struct vec2d) {
			.x = wl_fixed_to_double(old_sx),
			.y = wl_fixed_to_double(old_sy),
		},
		.b = (struct vec2d) {
			.x = wl_fixed_to_double(sx),
			.y = wl_fixed_to_double(sy),
		},
	};
	directions = get_motion_directions(&motion);

	while (directions) {
		closest_border = get_closest_border(&borders,
						    &motion,
						    directions);
		if (closest_border)
			clamp_to_border(closest_border, &motion, &directions);
		else
			break;
	}

	weston_view_to_global_float(pointer->focus,
				    (float) motion.b.x, (float) motion.b.y,
				    &new_x_f, &new_y_f);
	*clamped_x = wl_fixed_from_double(new_x_f);
	*clamped_y = wl_fixed_from_double(new_y_f);

	wl_array_release(&borders);
}

static double
point_to_border_distance_2(struct border *border, double x, double y)
{
	double orig_x, orig_y;
	double dx, dy;

	if (is_border_horizontal(border)) {
		if (x < border->line.a.x)
			orig_x = border->line.a.x;
		else if (x > border->line.b.x)
			orig_x = border->line.b.x;
		else
			orig_x = x;
		orig_y = border->line.a.y;
	} else {
		if (y < border->line.a.y)
			orig_y = border->line.a.y;
		else if (y > border->line.b.y)
			orig_y = border->line.b.y;
		else
			orig_y = y;
		orig_x = border->line.a.x;
	}


	dx = fabs(orig_x - x);
	dy = fabs(orig_y - y);
	return dx*dx + dy*dy;
}

static void
warp_to_behind_border(struct border *border, wl_fixed_t *sx, wl_fixed_t *sy)
{
	switch (border->blocking_dir) {
	case MOTION_DIRECTION_POSITIVE_X:
	case MOTION_DIRECTION_NEGATIVE_X:
		if (border->blocking_dir == MOTION_DIRECTION_POSITIVE_X)
			*sx = wl_fixed_from_double(border->line.a.x) - 1;
		else
			*sx = wl_fixed_from_double(border->line.a.x) + 1;
		if (*sy < wl_fixed_from_double(border->line.a.y))
			*sy = wl_fixed_from_double(border->line.a.y) + 1;
		else if (*sy > wl_fixed_from_double(border->line.b.y))
			*sy = wl_fixed_from_double(border->line.b.y) - 1;
		break;
	case MOTION_DIRECTION_POSITIVE_Y:
	case MOTION_DIRECTION_NEGATIVE_Y:
		if (border->blocking_dir == MOTION_DIRECTION_POSITIVE_Y)
			*sy = wl_fixed_from_double(border->line.a.y) - 1;
		else
			*sy = wl_fixed_from_double(border->line.a.y) + 1;
		if (*sx < wl_fixed_from_double(border->line.a.x))
			*sx = wl_fixed_from_double(border->line.a.x) + 1;
		else if (*sx > wl_fixed_from_double(border->line.b.x))
			*sx = wl_fixed_from_double(border->line.b.x) - 1;
		break;
	}
}

static void
maybe_warp_confined_pointer(struct weston_pointer_constraint *constraint)
{
	wl_fixed_t x;
	wl_fixed_t y;
	wl_fixed_t sx;
	wl_fixed_t sy;

	weston_view_from_global_fixed(constraint->view,
				      constraint->pointer->x,
				      constraint->pointer->y,
				      &sx,
				      &sy);

	if (!is_within_constraint_region(constraint, sx, sy)) {
		double xf = wl_fixed_to_double(sx);
		double yf = wl_fixed_to_double(sy);
		pixman_region32_t confine_region;
		struct wl_array borders;
		struct border *border;
		double closest_distance_2 = DBL_MAX;
		struct border *closest_border = NULL;

		wl_array_init(&borders);

		pixman_region32_init(&confine_region);
		pixman_region32_intersect(&confine_region,
					  &constraint->view->surface->input,
					  &constraint->region);
		region_to_outline(&confine_region, &borders);
		pixman_region32_fini(&confine_region);

		wl_array_for_each(border, &borders) {
			double distance_2;

			distance_2 = point_to_border_distance_2(border, xf, yf);
			if (distance_2 < closest_distance_2) {
				closest_border = border;
				closest_distance_2 = distance_2;
			}
		}
		assert(closest_border);

		warp_to_behind_border(closest_border, &sx, &sy);

		wl_array_release(&borders);

		weston_view_to_global_fixed(constraint->view, sx, sy, &x, &y);
		weston_pointer_move_to(constraint->pointer, x, y);
	}
}

static void
confined_pointer_grab_pointer_motion(struct weston_pointer_grab *grab,
				     const struct timespec *time,
				     struct weston_pointer_motion_event *event)
{
	struct weston_pointer_constraint *constraint =
		container_of(grab, struct weston_pointer_constraint, grab);
	struct weston_pointer *pointer = grab->pointer;
	struct weston_surface *surface;
	wl_fixed_t x, y;
	wl_fixed_t old_sx = pointer->sx;
	wl_fixed_t old_sy = pointer->sy;
	pixman_region32_t confine_region;

	assert(pointer->focus);
	assert(pointer->focus->surface == constraint->surface);

	surface = pointer->focus->surface;

	pixman_region32_init(&confine_region);
	pixman_region32_intersect(&confine_region,
				  &surface->input,
				  &constraint->region);
	weston_pointer_clamp_event_to_region(pointer, event,
					     &confine_region, &x, &y);
	weston_pointer_move_to(pointer, x, y);
	pixman_region32_fini(&confine_region);

	weston_view_from_global_fixed(pointer->focus, x, y,
				      &pointer->sx, &pointer->sy);

	if (old_sx != pointer->sx || old_sy != pointer->sy) {
		pointer_send_motion(pointer, time,
				    pointer->sx, pointer->sy);
	}

	pointer_send_relative_motion(pointer, time, event);
}

static void
confined_pointer_grab_pointer_button(struct weston_pointer_grab *grab,
				     const struct timespec *time,
				     uint32_t button,
				     uint32_t state_w)
{
	weston_pointer_send_button(grab->pointer, time, button, state_w);
}

static void
confined_pointer_grab_pointer_axis(struct weston_pointer_grab *grab,
				   const struct timespec *time,
				   struct weston_pointer_axis_event *event)
{
	weston_pointer_send_axis(grab->pointer, time, event);
}

static void
confined_pointer_grab_pointer_axis_source(struct weston_pointer_grab *grab,
					  uint32_t source)
{
	weston_pointer_send_axis_source(grab->pointer, source);
}

static void
confined_pointer_grab_pointer_frame(struct weston_pointer_grab *grab)
{
	weston_pointer_send_frame(grab->pointer);
}

static void
confined_pointer_grab_pointer_cancel(struct weston_pointer_grab *grab)
{
	struct weston_pointer_constraint *constraint =
		container_of(grab, struct weston_pointer_constraint, grab);

	disable_pointer_constraint(constraint);
}

static const struct weston_pointer_grab_interface
				confined_pointer_grab_interface = {
	confined_pointer_grab_pointer_focus,
	confined_pointer_grab_pointer_motion,
	confined_pointer_grab_pointer_button,
	confined_pointer_grab_pointer_axis,
	confined_pointer_grab_pointer_axis_source,
	confined_pointer_grab_pointer_frame,
	confined_pointer_grab_pointer_cancel,
};

static void
confined_pointer_destroy(struct wl_client *client,
			 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
confined_pointer_set_region(struct wl_client *client,
			    struct wl_resource *resource,
			    struct wl_resource *region_resource)
{
	struct weston_pointer_constraint *constraint =
		wl_resource_get_user_data(resource);
	struct weston_region *region = region_resource ?
		wl_resource_get_user_data(region_resource) : NULL;

	if (!constraint)
		return;

	if (region) {
		pixman_region32_copy(&constraint->region_pending,
				     &region->region);
	} else {
		pixman_region32_fini(&constraint->region_pending);
		region_init_infinite(&constraint->region_pending);
	}
	constraint->region_is_pending = true;
}

static const struct zwp_confined_pointer_v1_interface confined_pointer_interface = {
	confined_pointer_destroy,
	confined_pointer_set_region,
};

static void
pointer_constraints_confine_pointer(struct wl_client *client,
				    struct wl_resource *resource,
				    uint32_t id,
				    struct wl_resource *surface_resource,
				    struct wl_resource *pointer_resource,
				    struct wl_resource *region_resource,
				    uint32_t lifetime)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_pointer *pointer = wl_resource_get_user_data(pointer_resource);
	struct weston_region *region = region_resource ?
		wl_resource_get_user_data(region_resource) : NULL;

	init_pointer_constraint(resource, id, surface, pointer, region, lifetime,
				&zwp_confined_pointer_v1_interface,
				&confined_pointer_interface,
				&confined_pointer_grab_interface);
}

static const struct zwp_pointer_constraints_v1_interface pointer_constraints_interface = {
	pointer_constraints_destroy,
	pointer_constraints_lock_pointer,
	pointer_constraints_confine_pointer,
};

static void
bind_pointer_constraints(struct wl_client *client, void *data,
			 uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &zwp_pointer_constraints_v1_interface,
				      1, id);

	wl_resource_set_implementation(resource, &pointer_constraints_interface,
				       NULL, NULL);
}

static void
input_timestamps_destroy(struct wl_client *client,
			 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct zwp_input_timestamps_v1_interface
				input_timestamps_interface = {
	input_timestamps_destroy,
};

static void
input_timestamps_manager_destroy(struct wl_client *client,
				 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
input_timestamps_manager_get_keyboard_timestamps(struct wl_client *client,
						 struct wl_resource *resource,
						 uint32_t id,
						 struct wl_resource *keyboard_resource)
{
	struct weston_keyboard *keyboard =
		wl_resource_get_user_data(keyboard_resource);
	struct wl_resource *input_ts;

	input_ts = wl_resource_create(client,
				      &zwp_input_timestamps_v1_interface,
				      1, id);
	if (!input_ts) {
		wl_client_post_no_memory(client);
		return;
	}

	if (keyboard) {
		wl_list_insert(&keyboard->timestamps_list,
			       wl_resource_get_link(input_ts));
	} else {
		wl_list_init(wl_resource_get_link(input_ts));
	}

	wl_resource_set_implementation(input_ts,
				       &input_timestamps_interface,
				       keyboard_resource,
				       unbind_resource);
}

static void
input_timestamps_manager_get_pointer_timestamps(struct wl_client *client,
						struct wl_resource *resource,
						uint32_t id,
						struct wl_resource *pointer_resource)
{
	struct weston_pointer *pointer =
		wl_resource_get_user_data(pointer_resource);
	struct wl_resource *input_ts;

	input_ts = wl_resource_create(client,
				      &zwp_input_timestamps_v1_interface,
				      1, id);
	if (!input_ts) {
		wl_client_post_no_memory(client);
		return;
	}

	if (pointer) {
		wl_list_insert(&pointer->timestamps_list,
			       wl_resource_get_link(input_ts));
	} else {
		wl_list_init(wl_resource_get_link(input_ts));
	}

	wl_resource_set_implementation(input_ts,
				       &input_timestamps_interface,
				       pointer_resource,
				       unbind_resource);
}

static void
input_timestamps_manager_get_touch_timestamps(struct wl_client *client,
					      struct wl_resource *resource,
					      uint32_t id,
					      struct wl_resource *touch_resource)
{
	struct weston_touch *touch = wl_resource_get_user_data(touch_resource);
	struct wl_resource *input_ts;

	input_ts = wl_resource_create(client,
				      &zwp_input_timestamps_v1_interface,
				      1, id);
	if (!input_ts) {
		wl_client_post_no_memory(client);
		return;
	}

	if (touch) {
		wl_list_insert(&touch->timestamps_list,
			       wl_resource_get_link(input_ts));
	} else {
		wl_list_init(wl_resource_get_link(input_ts));
	}

	wl_resource_set_implementation(input_ts,
				       &input_timestamps_interface,
				       touch_resource,
				       unbind_resource);
}

static const struct zwp_input_timestamps_manager_v1_interface
				input_timestamps_manager_interface = {
	input_timestamps_manager_destroy,
	input_timestamps_manager_get_keyboard_timestamps,
	input_timestamps_manager_get_pointer_timestamps,
	input_timestamps_manager_get_touch_timestamps,
};

static void
bind_input_timestamps_manager(struct wl_client *client, void *data,
			      uint32_t version, uint32_t id)
{
	struct wl_resource *resource =
		wl_resource_create(client,
				   &zwp_input_timestamps_manager_v1_interface,
				   1, id);

	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource,
				       &input_timestamps_manager_interface,
				       NULL, NULL);
}

int
weston_input_init(struct weston_compositor *compositor)
{
	if (!wl_global_create(compositor->wl_display,
			      &zwp_relative_pointer_manager_v1_interface, 1,
			      compositor, bind_relative_pointer_manager))
		return -1;

	if (!wl_global_create(compositor->wl_display,
			      &zwp_pointer_constraints_v1_interface, 1,
			      NULL, bind_pointer_constraints))
		return -1;

	if (!wl_global_create(compositor->wl_display,
			      &zwp_input_timestamps_manager_v1_interface, 1,
			      NULL, bind_input_timestamps_manager))
		return -1;

	return 0;
}
