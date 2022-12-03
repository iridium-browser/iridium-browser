/*
 * Copyright © 2011 Kristian Høgsberg
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <libweston/libweston.h>
#include "libweston-internal.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

struct weston_drag {
	struct wl_client *client;
	struct weston_data_source *data_source;
	struct wl_listener data_source_listener;
	struct weston_view *focus;
	struct wl_resource *focus_resource;
	struct wl_listener focus_listener;
	struct weston_view *icon;
	struct wl_listener icon_destroy_listener;
	int32_t dx, dy;
	struct weston_keyboard_grab keyboard_grab;
};

struct weston_pointer_drag {
	struct weston_drag  base;
	struct weston_pointer_grab grab;
};

struct weston_touch_drag {
	struct weston_drag base;
	struct weston_touch_grab grab;
};

#define ALL_ACTIONS (WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY | \
		     WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | \
		     WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK)

static void
data_offer_accept(struct wl_client *client, struct wl_resource *resource,
		  uint32_t serial, const char *mime_type)
{
	struct weston_data_offer *offer = wl_resource_get_user_data(resource);

	/* Protect against untimely calls from older data offers */
	if (!offer->source || offer != offer->source->offer)
		return;

	/* FIXME: Check that client is currently focused by the input
	 * device that is currently dragging this data source.  Should
	 * this be a wl_data_device request? */

	offer->source->accept(offer->source, serial, mime_type);
	offer->source->accepted = mime_type != NULL;
}

static void
data_offer_receive(struct wl_client *client, struct wl_resource *resource,
		   const char *mime_type, int32_t fd)
{
	struct weston_data_offer *offer = wl_resource_get_user_data(resource);

	if (offer->source && offer == offer->source->offer)
		offer->source->send(offer->source, mime_type, fd);
	else
		close(fd);
}

static void
data_offer_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
data_source_notify_finish(struct weston_data_source *source)
{
	if (!source->actions_set)
		return;

	if (source->offer->in_ask &&
	    wl_resource_get_version(source->resource) >=
	    WL_DATA_SOURCE_ACTION_SINCE_VERSION) {
		wl_data_source_send_action(source->resource,
					   source->current_dnd_action);
	}

	if (wl_resource_get_version(source->resource) >=
	    WL_DATA_SOURCE_DND_FINISHED_SINCE_VERSION) {
		wl_data_source_send_dnd_finished(source->resource);
	}

	source->offer = NULL;
}

static uint32_t
data_offer_choose_action(struct weston_data_offer *offer)
{
	uint32_t available_actions, preferred_action = 0;
	uint32_t source_actions, offer_actions;

	if (wl_resource_get_version(offer->resource) >=
	    WL_DATA_OFFER_ACTION_SINCE_VERSION) {
		offer_actions = offer->dnd_actions;
		preferred_action = offer->preferred_dnd_action;
	} else {
		offer_actions = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
	}

	if (wl_resource_get_version(offer->source->resource) >=
	    WL_DATA_SOURCE_ACTION_SINCE_VERSION)
		source_actions = offer->source->dnd_actions;
	else
		source_actions = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;

	available_actions = offer_actions & source_actions;

	if (!available_actions)
		return WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;

	if (offer->source->seat &&
	    offer->source->compositor_action & available_actions)
		return offer->source->compositor_action;

	/* If the dest side has a preferred DnD action, use it */
	if ((preferred_action & available_actions) != 0)
		return preferred_action;

	/* Use the first found action, in bit order */
	return 1 << (ffs(available_actions) - 1);
}

static void
data_offer_update_action(struct weston_data_offer *offer)
{
	uint32_t action;

	if (!offer->source)
		return;

	action = data_offer_choose_action(offer);

	if (offer->source->current_dnd_action == action)
		return;

	offer->source->current_dnd_action = action;

	if (offer->in_ask)
		return;

	if (wl_resource_get_version(offer->source->resource) >=
	    WL_DATA_SOURCE_ACTION_SINCE_VERSION)
		wl_data_source_send_action(offer->source->resource, action);

	if (wl_resource_get_version(offer->resource) >=
	    WL_DATA_OFFER_ACTION_SINCE_VERSION)
		wl_data_offer_send_action(offer->resource, action);
}

static void
data_offer_set_actions(struct wl_client *client,
		       struct wl_resource *resource,
		       uint32_t dnd_actions, uint32_t preferred_action)
{
	struct weston_data_offer *offer = wl_resource_get_user_data(resource);

	if (dnd_actions & ~ALL_ACTIONS) {
		wl_resource_post_error(offer->resource,
				       WL_DATA_OFFER_ERROR_INVALID_ACTION_MASK,
				       "invalid action mask %x", dnd_actions);
		return;
	}

	if (preferred_action &&
	    (!(preferred_action & dnd_actions) ||
	     __builtin_popcount(preferred_action) > 1)) {
		wl_resource_post_error(offer->resource,
				       WL_DATA_OFFER_ERROR_INVALID_ACTION,
				       "invalid action %x", preferred_action);
		return;
	}

	offer->dnd_actions = dnd_actions;
	offer->preferred_dnd_action = preferred_action;
	data_offer_update_action(offer);
}

static void
data_offer_finish(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_data_offer *offer = wl_resource_get_user_data(resource);

	if (!offer->source || offer->source->offer != offer)
		return;

	if (offer->source->set_selection) {
		wl_resource_post_error(offer->resource,
				       WL_DATA_OFFER_ERROR_INVALID_FINISH,
				       "finish only valid for drag n drop");
		return;
	}

	/* Disallow finish while we have a grab driving drag-and-drop, or
	 * if the negotiation is not at the right stage
	 */
	if (offer->source->seat ||
	    !offer->source->accepted) {
		wl_resource_post_error(offer->resource,
				       WL_DATA_OFFER_ERROR_INVALID_FINISH,
				       "premature finish request");
		return;
	}

	switch (offer->source->current_dnd_action) {
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE:
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK:
		wl_resource_post_error(offer->resource,
				       WL_DATA_OFFER_ERROR_INVALID_OFFER,
				       "offer finished with an invalid action");
		return;
	default:
		break;
	}

	data_source_notify_finish(offer->source);
}

static const struct wl_data_offer_interface data_offer_interface = {
	data_offer_accept,
	data_offer_receive,
	data_offer_destroy,
	data_offer_finish,
	data_offer_set_actions,
};

static void
destroy_data_offer(struct wl_resource *resource)
{
	struct weston_data_offer *offer = wl_resource_get_user_data(resource);

	if (!offer->source)
		goto out;

	wl_list_remove(&offer->source_destroy_listener.link);

	if (offer->source->offer != offer)
		goto out;

	/* If the drag destination has version < 3, wl_data_offer.finish
	 * won't be called, so do this here as a safety net, because
	 * we still want the version >=3 drag source to be happy.
	 */
	if (wl_resource_get_version(offer->resource) <
	    WL_DATA_OFFER_ACTION_SINCE_VERSION) {
		data_source_notify_finish(offer->source);
	} else if (offer->source->resource &&
		   wl_resource_get_version(offer->source->resource) >=
		   WL_DATA_SOURCE_DND_FINISHED_SINCE_VERSION) {
		wl_data_source_send_cancelled(offer->source->resource);
	}

	offer->source->offer = NULL;
out:
	free(offer);
}

static void
destroy_offer_data_source(struct wl_listener *listener, void *data)
{
	struct weston_data_offer *offer;

	offer = container_of(listener, struct weston_data_offer,
			     source_destroy_listener);

	offer->source = NULL;
}

static struct weston_data_offer *
weston_data_source_send_offer(struct weston_data_source *source,
			      struct wl_resource *target)
{
	struct weston_data_offer *offer;
	char **p;

	offer = malloc(sizeof *offer);
	if (offer == NULL)
		return NULL;

	offer->resource =
		wl_resource_create(wl_resource_get_client(target),
				   &wl_data_offer_interface,
				   wl_resource_get_version(target), 0);
	if (offer->resource == NULL) {
		free(offer);
		return NULL;
	}

	wl_resource_set_implementation(offer->resource, &data_offer_interface,
				       offer, destroy_data_offer);

	offer->in_ask = false;
	offer->dnd_actions = 0;
	offer->preferred_dnd_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
	offer->source = source;
	offer->source_destroy_listener.notify = destroy_offer_data_source;
	wl_signal_add(&source->destroy_signal,
		      &offer->source_destroy_listener);

	wl_data_device_send_data_offer(target, offer->resource);

	wl_array_for_each(p, &source->mime_types)
		wl_data_offer_send_offer(offer->resource, *p);

	source->offer = offer;
	source->accepted = false;

	return offer;
}

static void
data_source_offer(struct wl_client *client,
		  struct wl_resource *resource,
		  const char *type)
{
	struct weston_data_source *source =
		wl_resource_get_user_data(resource);
	char **p;

	p = wl_array_add(&source->mime_types, sizeof *p);
	if (p)
		*p = strdup(type);
	if (!p || !*p)
		wl_resource_post_no_memory(resource);
}

static void
data_source_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
data_source_set_actions(struct wl_client *client,
			struct wl_resource *resource,
			uint32_t dnd_actions)
{
	struct weston_data_source *source =
		wl_resource_get_user_data(resource);

	if (source->actions_set) {
		wl_resource_post_error(source->resource,
				       WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK,
				       "cannot set actions more than once");
		return;
	}

	if (dnd_actions & ~ALL_ACTIONS) {
		wl_resource_post_error(source->resource,
				       WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK,
				       "invalid action mask %x", dnd_actions);
		return;
	}

	if (source->seat) {
		wl_resource_post_error(source->resource,
				       WL_DATA_SOURCE_ERROR_INVALID_ACTION_MASK,
				       "invalid action change after "
				       "wl_data_device.start_drag");
		return;
	}

	source->dnd_actions = dnd_actions;
	source->actions_set = true;
}

static struct wl_data_source_interface data_source_interface = {
	data_source_offer,
	data_source_destroy,
	data_source_set_actions
};

static void
drag_surface_configure(struct weston_drag *drag,
		       struct weston_pointer *pointer,
		       struct weston_touch *touch,
		       struct weston_surface *es,
		       int32_t sx, int32_t sy)
{
	struct weston_layer_entry *list;
	float fx, fy;

	assert((pointer != NULL && touch == NULL) ||
			(pointer == NULL && touch != NULL));

	if (!weston_surface_is_mapped(es) && es->buffer_ref.buffer) {
		if (pointer && pointer->sprite &&
			weston_view_is_mapped(pointer->sprite))
			list = &pointer->sprite->layer_link;
		else
			list = &es->compositor->cursor_layer.view_list;

		weston_layer_entry_remove(&drag->icon->layer_link);
		weston_layer_entry_insert(list, &drag->icon->layer_link);
		weston_view_update_transform(drag->icon);
		pixman_region32_clear(&es->pending.input);
		es->is_mapped = true;
		drag->icon->is_mapped = true;
	}

	drag->dx += sx;
	drag->dy += sy;

	/* init to 0 for avoiding a compile warning */
	fx = fy = 0;
	if (pointer) {
		fx = wl_fixed_to_double(pointer->x) + drag->dx;
		fy = wl_fixed_to_double(pointer->y) + drag->dy;
	} else if (touch) {
		fx = wl_fixed_to_double(touch->grab_x) + drag->dx;
		fy = wl_fixed_to_double(touch->grab_y) + drag->dy;
	}
	weston_view_set_position(drag->icon, fx, fy);
}

static int
pointer_drag_surface_get_label(struct weston_surface *surface,
			       char *buf, size_t len)
{
	return snprintf(buf, len, "pointer drag icon");
}

static void
pointer_drag_surface_committed(struct weston_surface *es,
			       int32_t sx, int32_t sy)
{
	struct weston_pointer_drag *drag = es->committed_private;
	struct weston_pointer *pointer = drag->grab.pointer;

	assert(es->committed == pointer_drag_surface_committed);

	drag_surface_configure(&drag->base, pointer, NULL, es, sx, sy);
}

static int
touch_drag_surface_get_label(struct weston_surface *surface,
			     char *buf, size_t len)
{
	return snprintf(buf, len, "touch drag icon");
}

static void
touch_drag_surface_committed(struct weston_surface *es, int32_t sx, int32_t sy)
{
	struct weston_touch_drag *drag = es->committed_private;
	struct weston_touch *touch = drag->grab.touch;

	assert(es->committed == touch_drag_surface_committed);

	drag_surface_configure(&drag->base, NULL, touch, es, sx, sy);
}

static void
destroy_drag_focus(struct wl_listener *listener, void *data)
{
	struct weston_drag *drag =
		container_of(listener, struct weston_drag, focus_listener);

	drag->focus_resource = NULL;
}

static void
weston_drag_set_focus(struct weston_drag *drag,
			struct weston_seat *seat,
			struct weston_view *view,
			wl_fixed_t sx, wl_fixed_t sy)
{
	struct wl_resource *resource, *offer_resource = NULL;
	struct wl_display *display = seat->compositor->wl_display;
	struct weston_data_offer *offer;
	uint32_t serial;

	if (drag->focus && view && drag->focus->surface == view->surface) {
		drag->focus = view;
		return;
	}

	if (drag->focus_resource) {
		wl_data_device_send_leave(drag->focus_resource);
		wl_list_remove(&drag->focus_listener.link);
		drag->focus_resource = NULL;
		drag->focus = NULL;
	}

	if (!view || !view->surface->resource)
		return;

	if (!drag->data_source &&
	    wl_resource_get_client(view->surface->resource) != drag->client)
		return;

	if (drag->data_source &&
	    drag->data_source->offer) {
		/* Unlink the offer from the source */
		offer = drag->data_source->offer;
		offer->source = NULL;
		drag->data_source->offer = NULL;
		wl_list_remove(&offer->source_destroy_listener.link);
	}

	resource = wl_resource_find_for_client(&seat->drag_resource_list,
					       wl_resource_get_client(view->surface->resource));
	if (!resource)
		return;

	serial = wl_display_next_serial(display);

	if (drag->data_source) {
		drag->data_source->accepted = false;
		offer = weston_data_source_send_offer(drag->data_source, resource);
		if (offer == NULL)
			return;

		data_offer_update_action(offer);

		offer_resource = offer->resource;
		if (wl_resource_get_version (offer_resource) >=
		    WL_DATA_OFFER_SOURCE_ACTIONS_SINCE_VERSION) {
			wl_data_offer_send_source_actions (offer_resource,
			                                   drag->data_source->dnd_actions);
		}
	}

	wl_data_device_send_enter(resource, serial, view->surface->resource,
				  sx, sy, offer_resource);

	drag->focus = view;
	drag->focus_listener.notify = destroy_drag_focus;
	wl_resource_add_destroy_listener(resource, &drag->focus_listener);
	drag->focus_resource = resource;
}

static void
drag_grab_focus(struct weston_pointer_grab *grab)
{
	struct weston_pointer_drag *drag =
		container_of(grab, struct weston_pointer_drag, grab);
	struct weston_pointer *pointer = grab->pointer;
	struct weston_view *view;
	wl_fixed_t sx, sy;

	view = weston_compositor_pick_view(pointer->seat->compositor,
					   pointer->x, pointer->y,
					   &sx, &sy);
	if (drag->base.focus != view)
		weston_drag_set_focus(&drag->base, pointer->seat, view, sx, sy);
}

static void
drag_grab_motion(struct weston_pointer_grab *grab,
		 const struct timespec *time,
		 struct weston_pointer_motion_event *event)
{
	struct weston_pointer_drag *drag =
		container_of(grab, struct weston_pointer_drag, grab);
	struct weston_pointer *pointer = drag->grab.pointer;
	float fx, fy;
	wl_fixed_t sx, sy;
	uint32_t msecs;

	weston_pointer_move(pointer, event);

	if (drag->base.icon) {
		fx = wl_fixed_to_double(pointer->x) + drag->base.dx;
		fy = wl_fixed_to_double(pointer->y) + drag->base.dy;
		weston_view_set_position(drag->base.icon, fx, fy);
		weston_view_schedule_repaint(drag->base.icon);
	}

	if (drag->base.focus_resource) {
		msecs = timespec_to_msec(time);
		weston_view_from_global_fixed(drag->base.focus,
					      pointer->x, pointer->y,
					      &sx, &sy);

		wl_data_device_send_motion(drag->base.focus_resource, msecs, sx, sy);
	}
}

static void
data_device_end_drag_grab(struct weston_drag *drag,
		struct weston_seat *seat)
{
	if (drag->icon) {
		if (weston_view_is_mapped(drag->icon))
			weston_view_unmap(drag->icon);

		drag->icon->surface->committed = NULL;
		weston_surface_set_label_func(drag->icon->surface, NULL);
		pixman_region32_clear(&drag->icon->surface->pending.input);
		wl_list_remove(&drag->icon_destroy_listener.link);
		weston_view_destroy(drag->icon);
	}

	weston_drag_set_focus(drag, seat, NULL, 0, 0);
}

static void
data_device_end_pointer_drag_grab(struct weston_pointer_drag *drag)
{
	struct weston_pointer *pointer = drag->grab.pointer;
	struct weston_keyboard *keyboard = drag->base.keyboard_grab.keyboard;

	data_device_end_drag_grab(&drag->base, pointer->seat);
	weston_pointer_end_grab(pointer);
	weston_keyboard_end_grab(keyboard);
	free(drag);
}

static void
drag_grab_button(struct weston_pointer_grab *grab,
		 const struct timespec *time,
		 uint32_t button, uint32_t state_w)
{
	struct weston_pointer_drag *drag =
		container_of(grab, struct weston_pointer_drag, grab);
	struct weston_pointer *pointer = drag->grab.pointer;
	enum wl_pointer_button_state state = state_w;
	struct weston_data_source *data_source = drag->base.data_source;

	if (data_source &&
	    pointer->grab_button == button &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (drag->base.focus_resource &&
		    data_source->accepted &&
		    data_source->current_dnd_action) {
			wl_data_device_send_drop(drag->base.focus_resource);

			if (wl_resource_get_version(data_source->resource) >=
			    WL_DATA_SOURCE_DND_DROP_PERFORMED_SINCE_VERSION)
				wl_data_source_send_dnd_drop_performed(data_source->resource);

			data_source->offer->in_ask =
				data_source->current_dnd_action ==
				WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK;

			data_source->seat = NULL;
		} else if (wl_resource_get_version(data_source->resource) >=
			   WL_DATA_SOURCE_DND_FINISHED_SINCE_VERSION) {
			wl_data_source_send_cancelled(data_source->resource);
		}
	}

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (drag->base.data_source)
			wl_list_remove(&drag->base.data_source_listener.link);
		data_device_end_pointer_drag_grab(drag);
	}
}

static void
drag_grab_axis(struct weston_pointer_grab *grab,
	       const struct timespec *time,
	       struct weston_pointer_axis_event *event)
{
}

static void
drag_grab_axis_source(struct weston_pointer_grab *grab, uint32_t source)
{
}

static void
drag_grab_frame(struct weston_pointer_grab *grab)
{
}

static void
drag_grab_cancel(struct weston_pointer_grab *grab)
{
	struct weston_pointer_drag *drag =
		container_of(grab, struct weston_pointer_drag, grab);

	if (drag->base.data_source)
		wl_list_remove(&drag->base.data_source_listener.link);

	data_device_end_pointer_drag_grab(drag);
}

static const struct weston_pointer_grab_interface pointer_drag_grab_interface = {
	drag_grab_focus,
	drag_grab_motion,
	drag_grab_button,
	drag_grab_axis,
	drag_grab_axis_source,
	drag_grab_frame,
	drag_grab_cancel,
};

static void
drag_grab_touch_down(struct weston_touch_grab *grab,
		     const struct timespec *time, int touch_id,
		     wl_fixed_t sx, wl_fixed_t sy)
{
}

static void
data_device_end_touch_drag_grab(struct weston_touch_drag *drag)
{
	struct weston_touch *touch = drag->grab.touch;
	struct weston_keyboard *keyboard = drag->base.keyboard_grab.keyboard;

	data_device_end_drag_grab(&drag->base, touch->seat);
	weston_touch_end_grab(touch);
	weston_keyboard_end_grab(keyboard);
	free(drag);
}

static void
drag_grab_touch_up(struct weston_touch_grab *grab,
		   const struct timespec *time, int touch_id)
{
	struct weston_touch_drag *touch_drag =
		container_of(grab, struct weston_touch_drag, grab);
	struct weston_touch *touch = grab->touch;

	if (touch_id != touch->grab_touch_id)
		return;

	if (touch_drag->base.focus_resource)
		wl_data_device_send_drop(touch_drag->base.focus_resource);
	if (touch_drag->base.data_source)
		wl_list_remove(&touch_drag->base.data_source_listener.link);
	data_device_end_touch_drag_grab(touch_drag);
}

static void
drag_grab_touch_focus(struct weston_touch_drag *drag)
{
	struct weston_touch *touch = drag->grab.touch;
	struct weston_view *view;
	wl_fixed_t view_x, view_y;

	view = weston_compositor_pick_view(touch->seat->compositor,
				touch->grab_x, touch->grab_y,
				&view_x, &view_y);
	if (drag->base.focus != view)
		weston_drag_set_focus(&drag->base, touch->seat,
				view, view_x, view_y);
}

static void
drag_grab_touch_motion(struct weston_touch_grab *grab,
		       const struct timespec *time,
		       int touch_id, wl_fixed_t x, wl_fixed_t y)
{
	struct weston_touch_drag *touch_drag =
		container_of(grab, struct weston_touch_drag, grab);
	struct weston_touch *touch = grab->touch;
	wl_fixed_t view_x, view_y;
	float fx, fy;
	uint32_t msecs;

	if (touch_id != touch->grab_touch_id)
		return;

	drag_grab_touch_focus(touch_drag);
	if (touch_drag->base.icon) {
		fx = wl_fixed_to_double(touch->grab_x) + touch_drag->base.dx;
		fy = wl_fixed_to_double(touch->grab_y) + touch_drag->base.dy;
		weston_view_set_position(touch_drag->base.icon, fx, fy);
		weston_view_schedule_repaint(touch_drag->base.icon);
	}

	if (touch_drag->base.focus_resource) {
		msecs = timespec_to_msec(time);
		weston_view_from_global_fixed(touch_drag->base.focus,
					touch->grab_x, touch->grab_y,
					&view_x, &view_y);
		wl_data_device_send_motion(touch_drag->base.focus_resource,
					   msecs, view_x, view_y);
	}
}

static void
drag_grab_touch_frame(struct weston_touch_grab *grab)
{
}

static void
drag_grab_touch_cancel(struct weston_touch_grab *grab)
{
	struct weston_touch_drag *touch_drag =
		container_of(grab, struct weston_touch_drag, grab);

	if (touch_drag->base.data_source)
		wl_list_remove(&touch_drag->base.data_source_listener.link);
	data_device_end_touch_drag_grab(touch_drag);
}

static const struct weston_touch_grab_interface touch_drag_grab_interface = {
	drag_grab_touch_down,
	drag_grab_touch_up,
	drag_grab_touch_motion,
	drag_grab_touch_frame,
	drag_grab_touch_cancel
};

static void
drag_grab_keyboard_key(struct weston_keyboard_grab *grab,
		       const struct timespec *time, uint32_t key, uint32_t state)
{
}

static void
drag_grab_keyboard_modifiers(struct weston_keyboard_grab *grab,
			     uint32_t serial, uint32_t mods_depressed,
			     uint32_t mods_latched,
			     uint32_t mods_locked, uint32_t group)
{
	struct weston_keyboard *keyboard = grab->keyboard;
	struct weston_drag *drag =
		container_of(grab, struct weston_drag, keyboard_grab);
	uint32_t compositor_action;

	if (mods_depressed & (1 << keyboard->xkb_info->shift_mod))
		compositor_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
	else if (mods_depressed & (1 << keyboard->xkb_info->ctrl_mod))
		compositor_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
	else
		compositor_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;

	drag->data_source->compositor_action = compositor_action;

	if (drag->data_source->offer)
		data_offer_update_action(drag->data_source->offer);
}

static void
drag_grab_keyboard_cancel(struct weston_keyboard_grab *grab)
{
	struct weston_drag *drag =
		container_of(grab, struct weston_drag, keyboard_grab);
	struct weston_pointer *pointer = grab->keyboard->seat->pointer_state;
	struct weston_touch *touch = grab->keyboard->seat->touch_state;

	if (pointer && pointer->grab->interface == &pointer_drag_grab_interface) {
		struct weston_touch_drag *touch_drag =
			(struct weston_touch_drag *) drag;
		drag_grab_touch_cancel(&touch_drag->grab);
	} else if (touch && touch->grab->interface == &touch_drag_grab_interface) {
		struct weston_pointer_drag *pointer_drag =
			(struct weston_pointer_drag *) drag;
		drag_grab_cancel(&pointer_drag->grab);
	}
}

static const struct weston_keyboard_grab_interface keyboard_drag_grab_interface = {
	drag_grab_keyboard_key,
	drag_grab_keyboard_modifiers,
	drag_grab_keyboard_cancel
};

static void
destroy_pointer_data_device_source(struct wl_listener *listener, void *data)
{
	struct weston_pointer_drag *drag = container_of(listener,
			struct weston_pointer_drag, base.data_source_listener);

	data_device_end_pointer_drag_grab(drag);
}

static void
handle_drag_icon_destroy(struct wl_listener *listener, void *data)
{
	struct weston_drag *drag = container_of(listener, struct weston_drag,
						icon_destroy_listener);

	drag->icon = NULL;
}

WL_EXPORT int
weston_pointer_start_drag(struct weston_pointer *pointer,
		       struct weston_data_source *source,
		       struct weston_surface *icon,
		       struct wl_client *client)
{
	struct weston_pointer_drag *drag;
	struct weston_keyboard *keyboard =
		weston_seat_get_keyboard(pointer->seat);

	drag = zalloc(sizeof *drag);
	if (drag == NULL)
		return -1;

	drag->grab.interface = &pointer_drag_grab_interface;
	drag->base.keyboard_grab.interface = &keyboard_drag_grab_interface;
	drag->base.client = client;
	drag->base.data_source = source;

	if (icon) {
		drag->base.icon = weston_view_create(icon);
		if (drag->base.icon == NULL) {
			free(drag);
			return -1;
		}

		drag->base.icon_destroy_listener.notify = handle_drag_icon_destroy;
		wl_signal_add(&icon->destroy_signal,
			      &drag->base.icon_destroy_listener);

		icon->committed = pointer_drag_surface_committed;
		icon->committed_private = drag;
		weston_surface_set_label_func(icon,
					pointer_drag_surface_get_label);
	} else {
		drag->base.icon = NULL;
	}

	if (source) {
		drag->base.data_source_listener.notify = destroy_pointer_data_device_source;
		wl_signal_add(&source->destroy_signal,
			      &drag->base.data_source_listener);
	}

	weston_pointer_clear_focus(pointer);
	weston_keyboard_set_focus(keyboard, NULL);

	weston_pointer_start_grab(pointer, &drag->grab);
	weston_keyboard_start_grab(keyboard, &drag->base.keyboard_grab);

	return 0;
}

static void
destroy_touch_data_device_source(struct wl_listener *listener, void *data)
{
	struct weston_touch_drag *drag = container_of(listener,
			struct weston_touch_drag, base.data_source_listener);

	data_device_end_touch_drag_grab(drag);
}

WL_EXPORT int
weston_touch_start_drag(struct weston_touch *touch,
		       struct weston_data_source *source,
		       struct weston_surface *icon,
		       struct wl_client *client)
{
	struct weston_touch_drag *drag;
	struct weston_keyboard *keyboard =
		weston_seat_get_keyboard(touch->seat);

	drag = zalloc(sizeof *drag);
	if (drag == NULL)
		return -1;

	drag->grab.interface = &touch_drag_grab_interface;
	drag->base.client = client;
	drag->base.data_source = source;

	if (icon) {
		drag->base.icon = weston_view_create(icon);
		if (drag->base.icon == NULL) {
			free(drag);
			return -1;
		}

		drag->base.icon_destroy_listener.notify = handle_drag_icon_destroy;
		wl_signal_add(&icon->destroy_signal,
			      &drag->base.icon_destroy_listener);

		icon->committed = touch_drag_surface_committed;
		icon->committed_private = drag;
		weston_surface_set_label_func(icon,
					touch_drag_surface_get_label);
	} else {
		drag->base.icon = NULL;
	}

	if (source) {
		drag->base.data_source_listener.notify = destroy_touch_data_device_source;
		wl_signal_add(&source->destroy_signal,
			      &drag->base.data_source_listener);
	}

	weston_keyboard_set_focus(keyboard, NULL);

	weston_touch_start_grab(touch, &drag->grab);
	weston_keyboard_start_grab(keyboard, &drag->base.keyboard_grab);

	drag_grab_touch_focus(drag);

	return 0;
}

static void
data_device_start_drag(struct wl_client *client, struct wl_resource *resource,
		       struct wl_resource *source_resource,
		       struct wl_resource *origin_resource,
		       struct wl_resource *icon_resource, uint32_t serial)
{
	struct weston_seat *seat = wl_resource_get_user_data(resource);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_touch *touch = weston_seat_get_touch(seat);
	struct weston_surface *origin = wl_resource_get_user_data(origin_resource);
	struct weston_data_source *source = NULL;
	struct weston_surface *icon = NULL;
	int is_pointer_grab, is_touch_grab;
	int32_t ret = 0;

	is_pointer_grab = pointer &&
			  pointer->button_count == 1 &&
			  pointer->grab_serial == serial &&
			  pointer->focus &&
			  pointer->focus->surface == origin;

	is_touch_grab = touch &&
			touch->num_tp == 1 &&
			touch->grab_serial == serial &&
			touch->focus &&
			touch->focus->surface == origin;

	if (!is_pointer_grab && !is_touch_grab)
		return;

	/* FIXME: Check that the data source type array isn't empty. */

	if (source_resource)
		source = wl_resource_get_user_data(source_resource);
	if (icon_resource)
		icon = wl_resource_get_user_data(icon_resource);

	if (icon) {
		if (weston_surface_set_role(icon, "wl_data_device-icon",
					    resource,
					    WL_DATA_DEVICE_ERROR_ROLE) < 0)
			return;
	}

	if (is_pointer_grab)
		ret = weston_pointer_start_drag(pointer, source, icon, client);
	else if (is_touch_grab)
		ret = weston_touch_start_drag(touch, source, icon, client);

	if (ret < 0)
		wl_resource_post_no_memory(resource);
	else
		source->seat = seat;
}

static void
destroy_selection_data_source(struct wl_listener *listener, void *data)
{
	struct weston_seat *seat = container_of(listener, struct weston_seat,
						selection_data_source_listener);
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct wl_resource *data_device;
	struct weston_surface *focus = NULL;

	seat->selection_data_source = NULL;

	if (keyboard)
		focus = keyboard->focus;
	if (focus && focus->resource) {
		data_device = wl_resource_find_for_client(&seat->drag_resource_list,
							  wl_resource_get_client(focus->resource));
		if (data_device)
			wl_data_device_send_selection(data_device, NULL);
	}

	wl_signal_emit(&seat->selection_signal, seat);
}

/** \brief Send the selection to the specified client
 *
 * This function creates a new wl_data_offer if there is a wl_data_source
 * currently set as the selection and sends it to the specified client,
 * followed by the wl_data_device.selection() event.
 * If there is no current selection the wl_data_device.selection() event
 * will carry a NULL wl_data_offer.
 *
 * If the client does not have a wl_data_device for the specified seat
 * nothing will be done.
 *
 * \param seat The seat owning the wl_data_device used to send the events.
 * \param client The client to which to send the selection.
 */
WL_EXPORT void
weston_seat_send_selection(struct weston_seat *seat, struct wl_client *client)
{
	struct weston_data_offer *offer;
	struct wl_resource *data_device;

	wl_resource_for_each(data_device, &seat->drag_resource_list) {
		if (wl_resource_get_client(data_device) != client)
		    continue;

		if (seat->selection_data_source) {
			offer = weston_data_source_send_offer(seat->selection_data_source,
							      data_device);
			wl_data_device_send_selection(data_device, offer->resource);
		} else {
			wl_data_device_send_selection(data_device, NULL);
		}
	}
}

WL_EXPORT void
weston_seat_set_selection(struct weston_seat *seat,
			  struct weston_data_source *source, uint32_t serial)
{
	struct weston_surface *focus = NULL;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);

	if (seat->selection_data_source &&
	    seat->selection_serial - serial < UINT32_MAX / 2)
		return;

	if (seat->selection_data_source) {
		seat->selection_data_source->cancel(seat->selection_data_source);
		wl_list_remove(&seat->selection_data_source_listener.link);
		seat->selection_data_source = NULL;
	}

	seat->selection_data_source = source;
	seat->selection_serial = serial;

	if (source)
		source->set_selection = true;

	if (keyboard)
		focus = keyboard->focus;
	if (focus && focus->resource) {
		weston_seat_send_selection(seat, wl_resource_get_client(focus->resource));
	}

	wl_signal_emit(&seat->selection_signal, seat);

	if (source) {
		seat->selection_data_source_listener.notify =
			destroy_selection_data_source;
		wl_signal_add(&source->destroy_signal,
			      &seat->selection_data_source_listener);
	}
}

static void
data_device_set_selection(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *source_resource, uint32_t serial)
{
	struct weston_seat *seat = wl_resource_get_user_data(resource);
	struct weston_data_source *source;

	if (!seat || !source_resource)
		return;

	source = wl_resource_get_user_data(source_resource);

	if (source->actions_set) {
		wl_resource_post_error(source_resource,
				       WL_DATA_SOURCE_ERROR_INVALID_SOURCE,
				       "cannot set drag-and-drop source as selection");
		return;
	}

	/* FIXME: Store serial and check against incoming serial here. */
	weston_seat_set_selection(seat, source, serial);
}
static void
data_device_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_data_device_interface data_device_interface = {
	data_device_start_drag,
	data_device_set_selection,
	data_device_release
};

static void
destroy_data_source(struct wl_resource *resource)
{
	struct weston_data_source *source =
		wl_resource_get_user_data(resource);
	char **p;

	wl_signal_emit(&source->destroy_signal, source);

	wl_array_for_each(p, &source->mime_types)
		free(*p);

	wl_array_release(&source->mime_types);

	free(source);
}

static void
client_source_accept(struct weston_data_source *source,
		     uint32_t time, const char *mime_type)
{
	wl_data_source_send_target(source->resource, mime_type);
}

static void
client_source_send(struct weston_data_source *source,
		   const char *mime_type, int32_t fd)
{
	wl_data_source_send_send(source->resource, mime_type, fd);
	close(fd);
}

static void
client_source_cancel(struct weston_data_source *source)
{
	wl_data_source_send_cancelled(source->resource);
}

static void
create_data_source(struct wl_client *client,
		   struct wl_resource *resource, uint32_t id)
{
	struct weston_data_source *source;

	source = malloc(sizeof *source);
	if (source == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	source->resource =
		wl_resource_create(client, &wl_data_source_interface,
				   wl_resource_get_version(resource), id);
	if (source->resource == NULL) {
		free(source);
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_signal_init(&source->destroy_signal);
	source->accept = client_source_accept;
	source->send = client_source_send;
	source->cancel = client_source_cancel;
	source->offer = NULL;
	source->accepted = false;
	source->seat = NULL;
	source->actions_set = false;
	source->dnd_actions = 0;
	source->current_dnd_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
	source->compositor_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
	source->set_selection = false;

	wl_array_init(&source->mime_types);

	wl_resource_set_implementation(source->resource, &data_source_interface,
				       source, destroy_data_source);
}

static void unbind_data_device(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
get_data_device(struct wl_client *client,
		struct wl_resource *manager_resource,
		uint32_t id, struct wl_resource *seat_resource)
{
	struct weston_seat *seat = wl_resource_get_user_data(seat_resource);
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &wl_data_device_interface,
				      wl_resource_get_version(manager_resource),
				      id);
	if (resource == NULL) {
		wl_resource_post_no_memory(manager_resource);
		return;
	}

	if (seat) {
		wl_list_insert(&seat->drag_resource_list,
			       wl_resource_get_link(resource));
	} else {
		wl_list_init(wl_resource_get_link(resource));
	}

	wl_resource_set_implementation(resource, &data_device_interface,
				       seat, unbind_data_device);
}

static const struct wl_data_device_manager_interface manager_interface = {
	create_data_source,
	get_data_device
};

static void
bind_manager(struct wl_client *client,
	     void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &wl_data_device_manager_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &manager_interface,
				       NULL, NULL);
}

WL_EXPORT void
wl_data_device_set_keyboard_focus(struct weston_seat *seat)
{
	struct weston_surface *focus;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);

	if (!keyboard)
		return;

	focus = keyboard->focus;
	if (!focus || !focus->resource)
		return;

	weston_seat_send_selection(seat, wl_resource_get_client(focus->resource));
}

WL_EXPORT int
wl_data_device_manager_init(struct wl_display *display)
{
	if (wl_global_create(display,
			     &wl_data_device_manager_interface, 3,
			     NULL, bind_manager) == NULL)
		return -1;

	return 0;
}
