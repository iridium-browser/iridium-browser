/*
 * Copyright 2010-2012 Intel Corporation
 * Copyright 2013 Raspberry Pi Foundation
 * Copyright 2011-2012,2020 Collabora, Ltd.
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

#include "kiosk-shell-grab.h"
#include "shared/helpers.h"

struct kiosk_shell_grab {
	struct kiosk_shell_surface *shsurf;
	struct wl_listener shsurf_destroy_listener;

	struct weston_pointer_grab pointer_grab;
	struct weston_touch_grab touch_grab;
	wl_fixed_t dx, dy;
	bool active;
};

static void
kiosk_shell_grab_destroy(struct kiosk_shell_grab *shgrab);

/*
 * pointer_move_grab_interface
 */

static void
pointer_move_grab_focus(struct weston_pointer_grab *grab)
{
}

static void
pointer_move_grab_axis(struct weston_pointer_grab *grab,
		       const struct timespec *time,
		       struct weston_pointer_axis_event *event)
{
}

static void
pointer_move_grab_axis_source(struct weston_pointer_grab *grab,
			      uint32_t source)
{
}

static void
pointer_move_grab_frame(struct weston_pointer_grab *grab)
{
}

static void
pointer_move_grab_motion(struct weston_pointer_grab *pointer_grab,
			 const struct timespec *time,
			 struct weston_pointer_motion_event *event)
{
	struct kiosk_shell_grab *shgrab =
		container_of(pointer_grab, struct kiosk_shell_grab, pointer_grab);
	struct weston_pointer *pointer = pointer_grab->pointer;
	struct kiosk_shell_surface *shsurf = shgrab->shsurf;
	struct weston_surface *surface;
	int dx, dy;

	weston_pointer_move(pointer, event);

	if (!shsurf)
		return;

	surface = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	dx = wl_fixed_to_int(pointer->x + shgrab->dx);
	dy = wl_fixed_to_int(pointer->y + shgrab->dy);

	weston_view_set_position(shsurf->view, dx, dy);

	weston_compositor_schedule_repaint(surface->compositor);
}

static void
pointer_move_grab_button(struct weston_pointer_grab *pointer_grab,
			 const struct timespec *time,
			 uint32_t button, uint32_t state_w)
{
	struct kiosk_shell_grab *shgrab =
		container_of(pointer_grab, struct kiosk_shell_grab, pointer_grab);
	struct weston_pointer *pointer = pointer_grab->pointer;
	enum wl_pointer_button_state state = state_w;

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED)
		kiosk_shell_grab_destroy(shgrab);
}

static void
pointer_move_grab_cancel(struct weston_pointer_grab *pointer_grab)
{
	struct kiosk_shell_grab *shgrab =
		container_of(pointer_grab, struct kiosk_shell_grab, pointer_grab);

	kiosk_shell_grab_destroy(shgrab);
}

static const struct weston_pointer_grab_interface pointer_move_grab_interface = {
	pointer_move_grab_focus,
	pointer_move_grab_motion,
	pointer_move_grab_button,
	pointer_move_grab_axis,
	pointer_move_grab_axis_source,
	pointer_move_grab_frame,
	pointer_move_grab_cancel,
};

/*
 * touch_move_grab_interface
 */

static void
touch_move_grab_down(struct weston_touch_grab *grab,
		     const struct timespec *time,
		     int touch_id, wl_fixed_t x, wl_fixed_t y)
{
}

static void
touch_move_grab_up(struct weston_touch_grab *touch_grab,
		   const struct timespec *time, int touch_id)
{
	struct kiosk_shell_grab *shgrab =
		container_of(touch_grab, struct kiosk_shell_grab, touch_grab);

	if (touch_id == 0)
		shgrab->active = false;

	if (touch_grab->touch->num_tp == 0)
		kiosk_shell_grab_destroy(shgrab);
}

static void
touch_move_grab_motion(struct weston_touch_grab *touch_grab,
		       const struct timespec *time, int touch_id,
		       wl_fixed_t x, wl_fixed_t y)
{
	struct kiosk_shell_grab *shgrab =
		container_of(touch_grab, struct kiosk_shell_grab, touch_grab);
	struct weston_touch *touch = touch_grab->touch;
	struct kiosk_shell_surface *shsurf = shgrab->shsurf;
	struct weston_surface *surface;
	int dx, dy;

	if (!shsurf || !shgrab->active)
		return;

	surface = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	dx = wl_fixed_to_int(touch->grab_x + shgrab->dx);
	dy = wl_fixed_to_int(touch->grab_y + shgrab->dy);

	weston_view_set_position(shsurf->view, dx, dy);

	weston_compositor_schedule_repaint(surface->compositor);
}

static void
touch_move_grab_frame(struct weston_touch_grab *grab)
{
}

static void
touch_move_grab_cancel(struct weston_touch_grab *touch_grab)
{
	struct kiosk_shell_grab *shgrab =
		container_of(touch_grab, struct kiosk_shell_grab, touch_grab);

	kiosk_shell_grab_destroy(shgrab);
}

static const struct weston_touch_grab_interface touch_move_grab_interface = {
	touch_move_grab_down,
	touch_move_grab_up,
	touch_move_grab_motion,
	touch_move_grab_frame,
	touch_move_grab_cancel,
};

/*
 * kiosk_shell_grab
 */

static void
kiosk_shell_grab_handle_shsurf_destroy(struct wl_listener *listener, void *data)
{
	struct kiosk_shell_grab *shgrab =
		container_of(listener, struct kiosk_shell_grab,
			     shsurf_destroy_listener);

	shgrab->shsurf = NULL;
}

static struct kiosk_shell_grab *
kiosk_shell_grab_create(struct kiosk_shell_surface *shsurf)
{
	struct kiosk_shell_grab *shgrab;

	shgrab = zalloc(sizeof *shgrab);
	if (!shgrab)
		return NULL;

	shgrab->shsurf = shsurf;
	shgrab->shsurf_destroy_listener.notify =
		kiosk_shell_grab_handle_shsurf_destroy;
	wl_signal_add(&shsurf->destroy_signal,
		      &shgrab->shsurf_destroy_listener);

	shsurf->grabbed = true;

	return shgrab;
}

enum kiosk_shell_grab_result
kiosk_shell_grab_start_for_pointer_move(struct kiosk_shell_surface *shsurf,
					struct weston_pointer *pointer)
{
	struct kiosk_shell_grab *shgrab;

	if (!shsurf)
		return KIOSK_SHELL_GRAB_RESULT_ERROR;

	if (shsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return KIOSK_SHELL_GRAB_RESULT_IGNORED;

	shgrab = kiosk_shell_grab_create(shsurf);
	if (!shgrab)
		return KIOSK_SHELL_GRAB_RESULT_ERROR;

	shgrab->dx = wl_fixed_from_double(shsurf->view->geometry.x) -
		   pointer->grab_x;
	shgrab->dy = wl_fixed_from_double(shsurf->view->geometry.y) -
		   pointer->grab_y;
	shgrab->active = true;

	weston_seat_break_desktop_grabs(pointer->seat);

	shgrab->pointer_grab.interface = &pointer_move_grab_interface;
	weston_pointer_start_grab(pointer, &shgrab->pointer_grab);

	return KIOSK_SHELL_GRAB_RESULT_OK;
}

enum kiosk_shell_grab_result
kiosk_shell_grab_start_for_touch_move(struct kiosk_shell_surface *shsurf,
				      struct weston_touch *touch)
{
	struct kiosk_shell_grab *shgrab;

	if (!shsurf)
		return KIOSK_SHELL_GRAB_RESULT_ERROR;

	if (shsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return KIOSK_SHELL_GRAB_RESULT_IGNORED;

	shgrab = kiosk_shell_grab_create(shsurf);
	if (!shgrab)
		return KIOSK_SHELL_GRAB_RESULT_ERROR;

	shgrab->dx = wl_fixed_from_double(shsurf->view->geometry.x) -
		   touch->grab_x;
	shgrab->dy = wl_fixed_from_double(shsurf->view->geometry.y) -
		   touch->grab_y;
	shgrab->active = true;

	weston_seat_break_desktop_grabs(touch->seat);

	shgrab->touch_grab.interface = &touch_move_grab_interface;
	weston_touch_start_grab(touch, &shgrab->touch_grab);

	return KIOSK_SHELL_GRAB_RESULT_OK;
}

static void
kiosk_shell_grab_destroy(struct kiosk_shell_grab *shgrab)
{
	if (shgrab->shsurf) {
		wl_list_remove(&shgrab->shsurf_destroy_listener.link);
		shgrab->shsurf->grabbed = false;
	}

	if (shgrab->pointer_grab.pointer)
		weston_pointer_end_grab(shgrab->pointer_grab.pointer);
	else if (shgrab->touch_grab.touch)
		weston_touch_end_grab(shgrab->touch_grab.touch);

	free(shgrab);
}
