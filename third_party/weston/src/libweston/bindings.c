/*
 * Copyright © 2011-2012 Intel Corporation
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
#include <linux/input.h>

#include <libweston/libweston.h>
#include "libweston-internal.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

struct weston_binding {
	uint32_t key;
	uint32_t button;
	uint32_t axis;
	uint32_t modifier;
	void *handler;
	void *data;
	struct wl_list link;
};

static struct weston_binding *
weston_compositor_add_binding(struct weston_compositor *compositor,
			      uint32_t key, uint32_t button, uint32_t axis,
			      uint32_t modifier, void *handler, void *data)
{
	struct weston_binding *binding;

	binding = malloc(sizeof *binding);
	if (binding == NULL)
		return NULL;

	binding->key = key;
	binding->button = button;
	binding->axis = axis;
	binding->modifier = modifier;
	binding->handler = handler;
	binding->data = data;

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_key_binding(struct weston_compositor *compositor,
				  uint32_t key, uint32_t modifier,
				  weston_key_binding_handler_t handler,
				  void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, key, 0, 0,
						modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compositor->key_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_modifier_binding(struct weston_compositor *compositor,
				       uint32_t modifier,
				       weston_modifier_binding_handler_t handler,
				       void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, 0, 0, 0,
						modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compositor->modifier_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_button_binding(struct weston_compositor *compositor,
				     uint32_t button, uint32_t modifier,
				     weston_button_binding_handler_t handler,
				     void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, 0, button, 0,
						modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compositor->button_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_touch_binding(struct weston_compositor *compositor,
				    uint32_t modifier,
				    weston_touch_binding_handler_t handler,
				    void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, 0, 0, 0,
						modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compositor->touch_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_axis_binding(struct weston_compositor *compositor,
				   uint32_t axis, uint32_t modifier,
				   weston_axis_binding_handler_t handler,
				   void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, 0, 0, axis,
						modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compositor->axis_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT struct weston_binding *
weston_compositor_add_debug_binding(struct weston_compositor *compositor,
				    uint32_t key,
				    weston_key_binding_handler_t handler,
				    void *data)
{
	struct weston_binding *binding;

	binding = weston_compositor_add_binding(compositor, key, 0, 0, 0,
						handler, data);

	wl_list_insert(compositor->debug_binding_list.prev, &binding->link);

	return binding;
}

WL_EXPORT void
weston_binding_destroy(struct weston_binding *binding)
{
	wl_list_remove(&binding->link);
	free(binding);
}

void
weston_binding_list_destroy_all(struct wl_list *list)
{
	struct weston_binding *binding, *tmp;

	wl_list_for_each_safe(binding, tmp, list, link)
		weston_binding_destroy(binding);
}

struct binding_keyboard_grab {
	uint32_t key;
	struct weston_keyboard_grab grab;
};

static void
binding_key(struct weston_keyboard_grab *grab,
	    const struct timespec *time, uint32_t key, uint32_t state_w)
{
	struct binding_keyboard_grab *b =
		container_of(grab, struct binding_keyboard_grab, grab);
	struct wl_resource *resource;
	enum wl_keyboard_key_state state = state_w;
	uint32_t serial;
	struct weston_keyboard *keyboard = grab->keyboard;
	struct wl_display *display = keyboard->seat->compositor->wl_display;
	uint32_t msecs;

	if (key == b->key) {
		if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
			weston_keyboard_end_grab(grab->keyboard);
			if (keyboard->input_method_resource)
				keyboard->grab = &keyboard->input_method_grab;
			free(b);
		} else {
			/* Don't send the key press event for the binding key */
			return;
		}
	}
	if (!wl_list_empty(&keyboard->focus_resource_list)) {
		serial = wl_display_next_serial(display);
		msecs = timespec_to_msec(time);
		wl_resource_for_each(resource, &keyboard->focus_resource_list) {
			wl_keyboard_send_key(resource,
					     serial,
					     msecs,
					     key,
					     state);
		}
	}
}

static void
binding_modifiers(struct weston_keyboard_grab *grab, uint32_t serial,
		  uint32_t mods_depressed, uint32_t mods_latched,
		  uint32_t mods_locked, uint32_t group)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, &grab->keyboard->focus_resource_list) {
		wl_keyboard_send_modifiers(resource, serial, mods_depressed,
					   mods_latched, mods_locked, group);
	}
}

static void
binding_cancel(struct weston_keyboard_grab *grab)
{
	struct binding_keyboard_grab *binding_grab =
		container_of(grab, struct binding_keyboard_grab, grab);

	weston_keyboard_end_grab(grab->keyboard);
	free(binding_grab);
}

static const struct weston_keyboard_grab_interface binding_grab = {
	binding_key,
	binding_modifiers,
	binding_cancel,
};

static void
install_binding_grab(struct weston_keyboard *keyboard,
		     const struct timespec *time, uint32_t key,
		     struct weston_surface *focus)
{
	struct binding_keyboard_grab *grab;

	grab = malloc(sizeof *grab);
	grab->key = key;
	grab->grab.interface = &binding_grab;
	weston_keyboard_start_grab(keyboard, &grab->grab);

	/* Notify the surface which had the focus before this binding
	 * triggered that we stole a keypress from under it, by forcing
	 * a wl_keyboard leave/enter pair. The enter event will contain
	 * the pressed key in the keys array, so the client will know
	 * the exact state of the keyboard.
	 * If the old focus surface is different than the new one it
	 * means it was changed in the binding handler, so it received
	 * the enter event already. */
	if (focus && keyboard->focus == focus) {
		weston_keyboard_set_focus(keyboard, NULL);
		weston_keyboard_set_focus(keyboard, focus);
	}
}

void
weston_compositor_run_key_binding(struct weston_compositor *compositor,
				  struct weston_keyboard *keyboard,
				  const struct timespec *time, uint32_t key,
				  enum wl_keyboard_key_state state)
{
	struct weston_binding *b, *tmp;
	struct weston_surface *focus;
	struct weston_seat *seat = keyboard->seat;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	/* Invalidate all active modifier bindings. */
	wl_list_for_each(b, &compositor->modifier_binding_list, link)
		b->key = key;

	wl_list_for_each_safe(b, tmp, &compositor->key_binding_list, link) {
		if (b->key == key && b->modifier == seat->modifier_state) {
			weston_key_binding_handler_t handler = b->handler;
			focus = keyboard->focus;
			handler(keyboard, time, key, b->data);

			/* If this was a key binding and it didn't
			 * install a keyboard grab, install one now to
			 * swallow the key press. */
			if (keyboard->grab ==
			    &keyboard->default_grab)
				install_binding_grab(keyboard,
						     time,
						     key,
						     focus);
		}
	}
}

void
weston_compositor_run_modifier_binding(struct weston_compositor *compositor,
				       struct weston_keyboard *keyboard,
				       enum weston_keyboard_modifier modifier,
				       enum wl_keyboard_key_state state)
{
	struct weston_binding *b, *tmp;

	if (keyboard->grab != &keyboard->default_grab)
		return;

	wl_list_for_each_safe(b, tmp, &compositor->modifier_binding_list, link) {
		weston_modifier_binding_handler_t handler = b->handler;

		if (b->modifier != modifier)
			continue;

		/* Prime the modifier binding. */
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
			b->key = 0;
			continue;
		}
		/* Ignore the binding if a key was pressed in between. */
		else if (b->key != 0) {
			return;
		}

		handler(keyboard, modifier, b->data);
	}
}

void
weston_compositor_run_button_binding(struct weston_compositor *compositor,
				     struct weston_pointer *pointer,
				     const struct timespec *time,
				     uint32_t button,
				     enum wl_pointer_button_state state)
{
	struct weston_binding *b, *tmp;

	if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		return;

	/* Invalidate all active modifier bindings. */
	wl_list_for_each(b, &compositor->modifier_binding_list, link)
		b->key = button;

	wl_list_for_each_safe(b, tmp, &compositor->button_binding_list, link) {
		if (b->button == button &&
		    b->modifier == pointer->seat->modifier_state) {
			weston_button_binding_handler_t handler = b->handler;
			handler(pointer, time, button, b->data);
		}
	}
}

void
weston_compositor_run_touch_binding(struct weston_compositor *compositor,
				    struct weston_touch *touch,
				    const struct timespec *time,
				    int touch_type)
{
	struct weston_binding *b, *tmp;

	if (touch->num_tp != 1 || touch_type != WL_TOUCH_DOWN)
		return;

	wl_list_for_each_safe(b, tmp, &compositor->touch_binding_list, link) {
		if (b->modifier == touch->seat->modifier_state) {
			weston_touch_binding_handler_t handler = b->handler;
			handler(touch, time, b->data);
		}
	}
}

int
weston_compositor_run_axis_binding(struct weston_compositor *compositor,
				   struct weston_pointer *pointer,
				   const struct timespec *time,
				   struct weston_pointer_axis_event *event)
{
	struct weston_binding *b, *tmp;

	/* Invalidate all active modifier bindings. */
	wl_list_for_each(b, &compositor->modifier_binding_list, link)
		b->key = event->axis;

	wl_list_for_each_safe(b, tmp, &compositor->axis_binding_list, link) {
		if (b->axis == event->axis &&
		    b->modifier == pointer->seat->modifier_state) {
			weston_axis_binding_handler_t handler = b->handler;
			handler(pointer, time, event, b->data);
			return 1;
		}
	}

	return 0;
}

int
weston_compositor_run_debug_binding(struct weston_compositor *compositor,
				    struct weston_keyboard *keyboard,
				    const struct timespec *time, uint32_t key,
				    enum wl_keyboard_key_state state)
{
	weston_key_binding_handler_t handler;
	struct weston_binding *binding, *tmp;
	int count = 0;

	wl_list_for_each_safe(binding, tmp, &compositor->debug_binding_list, link) {
		if (key != binding->key)
			continue;

		count++;
		handler = binding->handler;
		handler(keyboard, time, key, binding->data);
	}

	return count;
}

struct debug_binding_grab {
	struct weston_keyboard_grab grab;
	struct weston_seat *seat;
	uint32_t key[2];
	int key_released[2];
};

static void
debug_binding_key(struct weston_keyboard_grab *grab, const struct timespec *time,
		  uint32_t key, uint32_t state)
{
	struct debug_binding_grab *db = (struct debug_binding_grab *) grab;
	struct weston_compositor *ec = db->seat->compositor;
	struct wl_display *display = ec->wl_display;
	struct wl_resource *resource;
	uint32_t serial;
	int send = 0, terminate = 0;
	int check_binding = 1;
	int i;
	struct wl_list *resource_list;
	uint32_t msecs;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		/* Do not run bindings on key releases */
		check_binding = 0;

		for (i = 0; i < 2; i++)
			if (key == db->key[i])
				db->key_released[i] = 1;

		if (db->key_released[0] && db->key_released[1]) {
			/* All key releases been swalled so end the grab */
			terminate = 1;
		} else if (key != db->key[0] && key != db->key[1]) {
			/* Should not swallow release of other keys */
			send = 1;
		}
	} else if (key == db->key[0] && !db->key_released[0]) {
		/* Do not check bindings for the first press of the binding
		 * key. This allows it to be used as a debug shortcut.
		 * We still need to swallow this event. */
		check_binding = 0;
	} else if (db->key[1]) {
		/* If we already ran a binding don't process another one since
		 * we can't keep track of all the binding keys that were
		 * pressed in order to swallow the release events. */
		send = 1;
		check_binding = 0;
	}

	if (check_binding) {
		if (weston_compositor_run_debug_binding(ec, grab->keyboard,
							time, key, state)) {
			/* We ran a binding so swallow the press and keep the
			 * grab to swallow the released too. */
			send = 0;
			terminate = 0;
			db->key[1] = key;
		} else {
			/* Terminate the grab since the key pressed is not a
			 * debug binding key. */
			send = 1;
			terminate = 1;
		}
	}

	if (send) {
		serial = wl_display_next_serial(display);
		resource_list = &grab->keyboard->focus_resource_list;
		msecs = timespec_to_msec(time);
		wl_resource_for_each(resource, resource_list) {
			wl_keyboard_send_key(resource, serial, msecs, key, state);
		}
	}

	if (terminate) {
		weston_keyboard_end_grab(grab->keyboard);
		if (grab->keyboard->input_method_resource)
			grab->keyboard->grab = &grab->keyboard->input_method_grab;
		free(db);
	}
}

static void
debug_binding_modifiers(struct weston_keyboard_grab *grab, uint32_t serial,
			uint32_t mods_depressed, uint32_t mods_latched,
			uint32_t mods_locked, uint32_t group)
{
	struct wl_resource *resource;
	struct wl_list *resource_list;

	resource_list = &grab->keyboard->focus_resource_list;

	wl_resource_for_each(resource, resource_list) {
		wl_keyboard_send_modifiers(resource, serial, mods_depressed,
					   mods_latched, mods_locked, group);
	}
}

static void
debug_binding_cancel(struct weston_keyboard_grab *grab)
{
	struct debug_binding_grab *db = (struct debug_binding_grab *) grab;

	weston_keyboard_end_grab(grab->keyboard);
	free(db);
}

struct weston_keyboard_grab_interface debug_binding_keyboard_grab = {
	debug_binding_key,
	debug_binding_modifiers,
	debug_binding_cancel,
};

static void
debug_binding(struct weston_keyboard *keyboard, const struct timespec *time,
	      uint32_t key, void *data)
{
	struct debug_binding_grab *grab;

	grab = calloc(1, sizeof *grab);
	if (!grab)
		return;

	grab->seat = keyboard->seat;
	grab->key[0] = key;
	grab->grab.interface = &debug_binding_keyboard_grab;
	weston_keyboard_start_grab(keyboard, &grab->grab);
}

/** Install the trigger binding for debug bindings.
 *
 * \param compositor The compositor.
 * \param mod The modifier.
 *
 * This will add a key binding for modifier+SHIFT+SPACE that will trigger
 * debug key bindings.
 */
WL_EXPORT void
weston_install_debug_key_binding(struct weston_compositor *compositor,
				 uint32_t mod)
{
	weston_compositor_add_key_binding(compositor, KEY_SPACE,
					  mod | MODIFIER_SHIFT,
					  debug_binding, NULL);
}
