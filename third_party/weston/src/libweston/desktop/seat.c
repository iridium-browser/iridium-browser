/*
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2013 Raspberry Pi Foundation
 * Copyright © 2016 Quentin "Sardem FF7" Glidic
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <assert.h>

#include <wayland-server.h>

#include <libweston/libweston.h>
#include <libweston/zalloc.h>

#include <libweston/desktop.h>
#include "internal.h"
#include "shared/timespec-util.h"

struct weston_desktop_seat {
	struct wl_listener seat_destroy_listener;
	struct weston_seat *seat;
	struct {
		struct weston_keyboard_grab keyboard;
		struct weston_pointer_grab pointer;
		struct weston_touch_grab touch;
		bool initial_up;
		struct wl_client *client;
		struct wl_list surfaces;
		struct weston_desktop_surface *grab_surface;
		struct wl_listener grab_surface_destroy_listener;
	} popup_grab;
};

static void weston_desktop_seat_popup_grab_end(struct weston_desktop_seat *seat);

static void
weston_desktop_seat_popup_grab_keyboard_key(struct weston_keyboard_grab *grab,
					    const struct timespec *time,
					    uint32_t key,
					    enum wl_keyboard_key_state state)
{
	weston_keyboard_send_key(grab->keyboard, time, key, state);
}

static void
weston_desktop_seat_popup_grab_keyboard_modifiers(struct weston_keyboard_grab *grab,
						  uint32_t serial,
						  uint32_t mods_depressed,
						  uint32_t mods_latched,
						  uint32_t mods_locked,
						  uint32_t group)
{
	weston_keyboard_send_modifiers(grab->keyboard, serial, mods_depressed,
				       mods_latched, mods_locked, group);
}

static void
weston_desktop_seat_popup_grab_keyboard_cancel(struct weston_keyboard_grab *grab)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.keyboard);

	weston_desktop_seat_popup_grab_end(seat);
}

static const struct weston_keyboard_grab_interface weston_desktop_seat_keyboard_popup_grab_interface = {
   .key = weston_desktop_seat_popup_grab_keyboard_key,
   .modifiers = weston_desktop_seat_popup_grab_keyboard_modifiers,
   .cancel = weston_desktop_seat_popup_grab_keyboard_cancel,
};

static void
weston_desktop_seat_popup_grab_pointer_focus(struct weston_pointer_grab *grab)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.pointer);
	struct weston_pointer *pointer = grab->pointer;
	struct weston_view *view;

	view = weston_compositor_pick_view(pointer->seat->compositor,
					   pointer->pos);

	/* Ignore views that don't belong to the grabbing client */
	if (view != NULL &&
	    view->surface->resource != NULL &&
	    wl_resource_get_client(view->surface->resource) != seat->popup_grab.client)
		view = NULL;

	if (pointer->focus == view)
		return;

	if (view)
		weston_pointer_set_focus(pointer, view);
	else
		weston_pointer_clear_focus(pointer);
}

static void
weston_desktop_seat_popup_grab_pointer_motion(struct weston_pointer_grab *grab,
					      const struct timespec *time,
					      struct weston_pointer_motion_event *event)
{
	weston_pointer_send_motion(grab->pointer, time, event);
}

static void
weston_desktop_seat_popup_grab_pointer_button(struct weston_pointer_grab *grab,
					      const struct timespec *time,
					      uint32_t button,
					      enum wl_pointer_button_state state)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.pointer);
	struct weston_pointer *pointer = grab->pointer;
	bool initial_up = seat->popup_grab.initial_up;

	if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		seat->popup_grab.initial_up = true;

	if (weston_pointer_has_focus_resource(pointer))
		weston_pointer_send_button(pointer, time, button, state);
	else if (state == WL_POINTER_BUTTON_STATE_RELEASED &&
		 (initial_up ||
		  (timespec_sub_to_msec(time, &grab->pointer->grab_time) > 500)))
		weston_desktop_seat_popup_grab_end(seat);
}

static void
weston_desktop_seat_popup_grab_pointer_axis(struct weston_pointer_grab *grab,
					    const struct timespec *time,
					    struct weston_pointer_axis_event *event)
{
	weston_pointer_send_axis(grab->pointer, time, event);
}

static void
weston_desktop_seat_popup_grab_pointer_axis_source(struct weston_pointer_grab *grab,
						   uint32_t source)
{
	weston_pointer_send_axis_source(grab->pointer, source);
}

static void
weston_desktop_seat_popup_grab_pointer_frame(struct weston_pointer_grab *grab)
{
	weston_pointer_send_frame(grab->pointer);
}

static void
weston_desktop_seat_popup_grab_pointer_cancel(struct weston_pointer_grab *grab)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.pointer);

	weston_desktop_seat_popup_grab_end(seat);
}

static const struct weston_pointer_grab_interface weston_desktop_seat_pointer_popup_grab_interface = {
   .focus = weston_desktop_seat_popup_grab_pointer_focus,
   .motion = weston_desktop_seat_popup_grab_pointer_motion,
   .button = weston_desktop_seat_popup_grab_pointer_button,
   .axis = weston_desktop_seat_popup_grab_pointer_axis,
   .axis_source = weston_desktop_seat_popup_grab_pointer_axis_source,
   .frame = weston_desktop_seat_popup_grab_pointer_frame,
   .cancel = weston_desktop_seat_popup_grab_pointer_cancel,
};

static void
weston_desktop_seat_popup_grab_touch_down(struct weston_touch_grab *grab,
					  const struct timespec *time,
					  int touch_id,
					  wl_fixed_t sx, wl_fixed_t sy)
{
	struct weston_coord_global pos;

	pos.c = weston_coord_from_fixed(sx, sy);
	weston_touch_send_down(grab->touch, time, touch_id, pos);
}

static void
weston_desktop_seat_popup_grab_touch_up(struct weston_touch_grab *grab,
					const struct timespec *time,
					int touch_id)
{
	weston_touch_send_up(grab->touch, time, touch_id);
}

static void
weston_desktop_seat_popup_grab_touch_motion(struct weston_touch_grab *grab,
					    const struct timespec *time,
					    int touch_id,
					    wl_fixed_t sx, wl_fixed_t sy)
{
	struct weston_coord_global pos;

	pos.c = weston_coord_from_fixed(sx, sy);
	weston_touch_send_motion(grab->touch, time, touch_id, pos);
}

static void
weston_desktop_seat_popup_grab_touch_frame(struct weston_touch_grab *grab)
{
	weston_touch_send_frame(grab->touch);
}

static void
weston_desktop_seat_popup_grab_touch_cancel(struct weston_touch_grab *grab)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.touch);

	weston_desktop_seat_popup_grab_end(seat);
}

static const struct weston_touch_grab_interface weston_desktop_seat_touch_popup_grab_interface = {
   .down = weston_desktop_seat_popup_grab_touch_down,
   .up = weston_desktop_seat_popup_grab_touch_up,
   .motion = weston_desktop_seat_popup_grab_touch_motion,
   .frame = weston_desktop_seat_popup_grab_touch_frame,
   .cancel = weston_desktop_seat_popup_grab_touch_cancel,
};

static void
weston_desktop_seat_popup_grab_tablet_tool_proximity_in(struct weston_tablet_tool_grab *grab,
				      const struct timespec *time,
				      struct weston_tablet *tablet)
{
}

static void
weston_desktop_seat_popup_grab_tablet_tool_proximity_out(struct weston_tablet_tool_grab *grab,
				       const struct timespec *time)
{
	weston_tablet_tool_send_proximity_out(grab->tool, time);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_motion(struct weston_tablet_tool_grab *grab,
				const struct timespec *time,
				struct weston_coord_global pos)
{
	weston_tablet_tool_send_motion(grab->tool, time, pos);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_down(struct weston_tablet_tool_grab *grab,
			      const struct timespec *time)
{
	weston_tablet_tool_send_down(grab->tool, time);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_up(struct weston_tablet_tool_grab *grab,
			      const struct timespec *time)
{
	weston_tablet_tool_send_up(grab->tool, time);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_pressure(struct weston_tablet_tool_grab *grab,
				  const struct timespec *time,
				  uint32_t pressure)
{
	weston_tablet_tool_send_pressure(grab->tool, time, pressure);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_distance(struct weston_tablet_tool_grab *grab,
				  const struct timespec *time,
				  uint32_t distance)
{
	weston_tablet_tool_send_distance(grab->tool, time, distance);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_tilt(struct weston_tablet_tool_grab *grab,
			      const struct timespec *time,
			      wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
	weston_tablet_tool_send_tilt(grab->tool, time, tilt_x, tilt_y);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_button(struct weston_tablet_tool_grab *grab,
				const struct timespec *time,
				uint32_t button, uint32_t state)
{
	weston_tablet_tool_send_button(grab->tool, time, button, state);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_frame(struct weston_tablet_tool_grab *grab,
			       const struct timespec *time)
{
	weston_tablet_tool_send_frame(grab->tool, time);
}

static void
weston_desktop_seat_popup_grab_tablet_tool_cancel(struct weston_tablet_tool_grab *grab)
{
	struct weston_desktop_seat *seat =
		wl_container_of(grab, seat, popup_grab.pointer);

	weston_desktop_seat_popup_grab_end(seat);
}

static const struct weston_tablet_tool_grab_interface weston_desktop_seat_tablet_tool_popup_grab_interface = {
	weston_desktop_seat_popup_grab_tablet_tool_proximity_in,
	weston_desktop_seat_popup_grab_tablet_tool_proximity_out,
	weston_desktop_seat_popup_grab_tablet_tool_motion,
	weston_desktop_seat_popup_grab_tablet_tool_down,
	weston_desktop_seat_popup_grab_tablet_tool_up,
	weston_desktop_seat_popup_grab_tablet_tool_pressure,
	weston_desktop_seat_popup_grab_tablet_tool_distance,
	weston_desktop_seat_popup_grab_tablet_tool_tilt,
	weston_desktop_seat_popup_grab_tablet_tool_button,
	weston_desktop_seat_popup_grab_tablet_tool_frame,
	weston_desktop_seat_popup_grab_tablet_tool_cancel,
};

static void
weston_desktop_seat_destroy(struct wl_listener *listener, void *data)
{
	struct weston_desktop_seat *seat =
		wl_container_of(listener, seat, seat_destroy_listener);

	free(seat);
}

struct weston_desktop_seat *
weston_desktop_seat_from_seat(struct weston_seat *wseat)
{
	struct wl_listener *listener;
	struct weston_desktop_seat *seat;

	if (wseat == NULL)
		return NULL;

	listener = wl_signal_get(&wseat->destroy_signal,
				 weston_desktop_seat_destroy);
	if (listener != NULL)
		return wl_container_of(listener, seat, seat_destroy_listener);

	seat = zalloc(sizeof(struct weston_desktop_seat));
	if (seat == NULL)
		return NULL;

	seat->seat = wseat;

	seat->seat_destroy_listener.notify = weston_desktop_seat_destroy;
	wl_signal_add(&wseat->destroy_signal, &seat->seat_destroy_listener);

	seat->popup_grab.keyboard.interface =
		&weston_desktop_seat_keyboard_popup_grab_interface;
	seat->popup_grab.pointer.interface =
		&weston_desktop_seat_pointer_popup_grab_interface;
	seat->popup_grab.touch.interface =
		&weston_desktop_seat_touch_popup_grab_interface;
	wl_list_init(&seat->popup_grab.surfaces);

	return seat;
}

struct weston_desktop_surface *
weston_desktop_seat_popup_grab_get_topmost_surface(struct weston_desktop_seat *seat)
{
	if (seat == NULL || wl_list_empty(&seat->popup_grab.surfaces))
		return NULL;

	struct wl_list *grab_link = seat->popup_grab.surfaces.next;

	return weston_desktop_surface_from_grab_link(grab_link);
}

static void
popup_grab_grab_surface_destroy(struct wl_listener *listener, void *data)
{
	struct weston_desktop_seat *seat =
		wl_container_of(listener, seat,
				popup_grab.grab_surface_destroy_listener);

	seat->popup_grab.grab_surface = NULL;
}

bool
weston_desktop_seat_popup_grab_start(struct weston_desktop_seat *seat,
				     struct weston_desktop_surface *parent,
				     struct wl_client *client, uint32_t serial)
{
	assert(seat == NULL || seat->popup_grab.client == NULL ||
	       seat->popup_grab.client == client);

	struct weston_seat *wseat = seat != NULL ? seat->seat : NULL;
	/* weston_seat_get_* functions can properly handle a NULL wseat */
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(wseat);
	struct weston_pointer *pointer = weston_seat_get_pointer(wseat);
	struct weston_touch *touch = weston_seat_get_touch(wseat);
	struct weston_tablet_tool *tool;
	bool tool_found = false;

	if (wseat) {
		wl_list_for_each(tool, &wseat->tablet_tool_list, link) {
			if (tool->grab_serial == serial) {
				tool_found = true;
				break;
			}
		}
	}

	if ((keyboard == NULL || keyboard->grab_serial != serial) &&
	    (pointer == NULL || pointer->grab_serial != serial) &&
	    (touch == NULL || touch->grab_serial != serial) &&
	    !tool_found) {
		return false;
	}

	wl_list_for_each(tool, &wseat->tablet_tool_list, link) {
		if (tool->grab->interface != &weston_desktop_seat_tablet_tool_popup_grab_interface) {
			struct weston_tablet_tool_grab *grab = zalloc(sizeof(*grab));

			grab->interface = &weston_desktop_seat_tablet_tool_popup_grab_interface;
			weston_tablet_tool_start_grab(tool, grab);
		}
	}

	seat->popup_grab.initial_up =
		(pointer == NULL || pointer->button_count == 0);
	seat->popup_grab.client = client;

	if (keyboard != NULL &&
	    keyboard->grab->interface != &weston_desktop_seat_keyboard_popup_grab_interface) {
		struct weston_surface *parent_surface;

		weston_keyboard_start_grab(keyboard, &seat->popup_grab.keyboard);
		seat->popup_grab.grab_surface = parent;

		parent_surface = weston_desktop_surface_get_surface(parent);
		seat->popup_grab.grab_surface_destroy_listener.notify =
			popup_grab_grab_surface_destroy;
		wl_signal_add(&parent_surface->destroy_signal,
			      &seat->popup_grab.grab_surface_destroy_listener);
	}

	if (pointer != NULL &&
	    pointer->grab->interface != &weston_desktop_seat_pointer_popup_grab_interface)
		weston_pointer_start_grab(pointer, &seat->popup_grab.pointer);

	if (touch != NULL &&
	    touch->grab->interface != &weston_desktop_seat_touch_popup_grab_interface)
		weston_touch_start_grab(touch, &seat->popup_grab.touch);

	return true;
}

static void
weston_desktop_seat_popup_grab_end(struct weston_desktop_seat *seat)
{
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat->seat);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat->seat);
	struct weston_touch *touch = weston_seat_get_touch(seat->seat);
	struct weston_tablet_tool *tool;

	while (!wl_list_empty(&seat->popup_grab.surfaces)) {
		struct wl_list *link = seat->popup_grab.surfaces.prev;
		struct weston_desktop_surface *surface =
			weston_desktop_surface_from_grab_link(link);

		wl_list_remove(link);
		wl_list_init(link);
		weston_desktop_surface_popup_dismiss(surface);
	}

	if (keyboard != NULL &&
	    keyboard->grab->interface == &weston_desktop_seat_keyboard_popup_grab_interface) {
		struct weston_desktop_surface *grab_desktop_surface;
		struct weston_surface *grab_surface;

		weston_keyboard_end_grab(keyboard);

		grab_desktop_surface = seat->popup_grab.grab_surface;
		grab_surface =
			weston_desktop_surface_get_surface(grab_desktop_surface);
		weston_keyboard_set_focus(keyboard, grab_surface);
	}

	if (pointer != NULL &&
	    pointer->grab->interface == &weston_desktop_seat_pointer_popup_grab_interface)
		weston_pointer_end_grab(pointer);

	if (touch != NULL &&
	    touch->grab->interface == &weston_desktop_seat_touch_popup_grab_interface)
		weston_touch_end_grab(touch);

	wl_list_for_each(tool, &seat->seat->tablet_tool_list, link) {
		if (tool->grab->interface == &weston_desktop_seat_tablet_tool_popup_grab_interface) {
			struct weston_tablet_tool_grab *grab = tool->grab;
			weston_tablet_tool_end_grab(tool);
			free(grab);
		}
	}

	seat->popup_grab.client = NULL;
	if (seat->popup_grab.grab_surface) {
		seat->popup_grab.grab_surface = NULL;
		wl_list_remove(&seat->popup_grab.grab_surface_destroy_listener.link);
	}
}

void
weston_desktop_seat_popup_grab_add_surface(struct weston_desktop_seat *seat,
					   struct wl_list *link)
{
	struct weston_desktop_surface *desktop_surface;
	struct weston_surface *surface;

	assert(seat->popup_grab.client != NULL);

	wl_list_insert(&seat->popup_grab.surfaces, link);

	desktop_surface =
		weston_desktop_seat_popup_grab_get_topmost_surface(seat);
	surface = weston_desktop_surface_get_surface(desktop_surface);
	weston_keyboard_set_focus(seat->popup_grab.keyboard.keyboard, surface);
}

void
weston_desktop_seat_popup_grab_remove_surface(struct weston_desktop_seat *seat,
					      struct wl_list *link)
{
	assert(seat->popup_grab.client != NULL);

	wl_list_remove(link);
	wl_list_init(link);
	if (wl_list_empty(&seat->popup_grab.surfaces)) {
		weston_desktop_seat_popup_grab_end(seat);
	} else {
		struct weston_desktop_surface *desktop_surface;
		struct weston_surface *surface;

		desktop_surface =
			weston_desktop_seat_popup_grab_get_topmost_surface(seat);
		surface = weston_desktop_surface_get_surface(desktop_surface);
		weston_keyboard_set_focus(seat->popup_grab.keyboard.keyboard,
					  surface);
	}
}

WL_EXPORT void
weston_seat_break_desktop_grabs(struct weston_seat *wseat)
{
	struct weston_desktop_seat *seat = weston_desktop_seat_from_seat(wseat);

	weston_desktop_seat_popup_grab_end(seat);
}
