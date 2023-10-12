/*
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2013 Raspberry Pi Foundation
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <assert.h>
#include <signal.h>
#include <math.h>
#include <sys/types.h>

#include "shell.h"
#include "compositor/weston.h"
#include "weston-desktop-shell-server-protocol.h"
#include <libweston/config-parser.h>
#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include <libweston/shell-utils.h>
#include <libweston/desktop.h>

#define DEFAULT_NUM_WORKSPACES 1
#define DEFAULT_WORKSPACE_CHANGE_ANIMATION_LENGTH 200

struct focus_state {
	struct desktop_shell *shell;
	struct weston_seat *seat;
	struct workspace *ws;
	struct weston_surface *keyboard_focus;
	struct wl_list link;
	struct wl_listener seat_destroy_listener;
	struct wl_listener surface_destroy_listener;
};

/*
 * Surface stacking and ordering.
 *
 * This is handled using several linked lists of surfaces, organised into
 * ‘layers’. The layers are ordered, and each of the surfaces in one layer are
 * above all of the surfaces in the layer below. The set of layers is static and
 * in the following order (top-most first):
 *  • Lock layer (only ever displayed on its own)
 *  • Cursor layer
 *  • Input panel layer
 *  • Fullscreen layer
 *  • Panel layer
 *  • Workspace layers
 *  • Background layer
 *
 * The list of layers may be manipulated to remove whole layers of surfaces from
 * display. For example, when locking the screen, all layers except the lock
 * layer are removed.
 *
 * A surface’s layer is modified on configuring the surface, in
 * set_surface_type() (which is only called when the surface’s type change is
 * _committed_). If a surface’s type changes (e.g. when making a window
 * fullscreen) its layer changes too.
 *
 * In order to allow popup and transient surfaces to be correctly stacked above
 * their parent surfaces, each surface tracks both its parent surface, and a
 * linked list of its children. When a surface’s layer is updated, so are the
 * layers of its children. Note that child surfaces are *not* the same as
 * subsurfaces — child/parent surfaces are purely for maintaining stacking
 * order.
 *
 * The children_link list of siblings of a surface (i.e. those surfaces which
 * have the same parent) only contains weston_surfaces which have a
 * shell_surface. Stacking is not implemented for non-shell_surface
 * weston_surfaces. This means that the following implication does *not* hold:
 *     (shsurf->parent != NULL) ⇒ !wl_list_is_empty(shsurf->children_link)
 */

struct shell_surface {
	struct wl_signal destroy_signal;

	struct weston_desktop_surface *desktop_surface;
	struct weston_view *view;
	struct weston_surface *wsurface_anim_fade;
	struct weston_view *wview_anim_fade;
	int32_t last_width, last_height;

	struct desktop_shell *shell;

	struct wl_list children_list;
	struct wl_list children_link;

	struct weston_coord_global saved_pos;
	bool saved_position_valid;
	bool saved_rotation_valid;
	int unresponsive, grabbed;
	uint32_t resize_edges;
	uint32_t orientation;

	struct {
		struct weston_transform transform;
		struct weston_matrix rotation;
	} rotation;

	struct {
		struct weston_transform transform; /* matrix from x, y */
		struct weston_curtain *black_view;
	} fullscreen;

	struct weston_output *fullscreen_output;
	struct weston_output *output;
	struct wl_listener output_destroy_listener;

	struct surface_state {
		bool fullscreen;
		bool maximized;
		bool lowered;
	} state;

	struct {
		bool is_set;
		int32_t x;
		int32_t y;
	} xwayland;

	int focus_count;

	bool destroying;
};

struct shell_grab {
	struct weston_pointer_grab grab;
	struct shell_surface *shsurf;
	struct wl_listener shsurf_destroy_listener;
};

struct shell_touch_grab {
	struct weston_touch_grab grab;
	struct shell_surface *shsurf;
	struct wl_listener shsurf_destroy_listener;
	struct weston_touch *touch;
};

struct shell_tablet_tool_grab {
	struct weston_tablet_tool_grab grab;
	struct shell_surface *shsurf;
	struct wl_listener shsurf_destroy_listener;
	struct weston_tablet_tool *tool;
};

struct weston_move_grab {
	struct shell_grab base;
	wl_fixed_t dx, dy;
	bool client_initiated;
};

struct weston_touch_move_grab {
	struct shell_touch_grab base;
	int active;
	wl_fixed_t dx, dy;
};

struct weston_tablet_tool_move_grab {
	struct shell_tablet_tool_grab base;
	wl_fixed_t dx, dy;
};

struct rotate_grab {
	struct shell_grab base;
	struct weston_matrix rotation;
	struct {
		float x;
		float y;
	} center;
};

struct shell_seat {
	struct weston_seat *seat;
	struct wl_listener seat_destroy_listener;
	struct weston_surface *focused_surface;

	struct wl_listener caps_changed_listener;
	struct wl_listener pointer_focus_listener;
	struct wl_listener keyboard_focus_listener;
	struct wl_listener tablet_tool_added_listener;

	struct wl_list link;	/** shell::seat_list */
};

struct tablet_tool_listener {
	struct wl_listener base;
	struct wl_listener removed_listener;
};

static struct weston_view *
shell_fade_create_fade_out_view(struct shell_surface *shsurf,
				struct weston_surface *surface);

static struct desktop_shell *
shell_surface_get_shell(struct shell_surface *shsurf);

static void
set_busy_cursor(struct shell_surface *shsurf, struct weston_pointer *pointer);

static void
surface_rotate(struct shell_surface *surface, struct weston_pointer *pointer);

static void
shell_fade_startup(struct desktop_shell *shell);

static void
shell_fade(struct desktop_shell *shell, enum fade_type type);

static struct shell_seat *
get_shell_seat(struct weston_seat *seat);

static void
get_output_panel_size(struct desktop_shell *shell,
		      struct weston_output *output,
		      int *width, int *height);

static void
shell_surface_update_child_surface_layers(struct shell_surface *shsurf);

static void
destroy_shell_grab_shsurf(struct wl_listener *listener, void *data)
{
	struct shell_grab *grab;

	grab = container_of(listener, struct shell_grab,
			    shsurf_destroy_listener);

	grab->shsurf = NULL;
}

struct weston_view *
get_default_view(struct weston_surface *surface)
{
	struct shell_surface *shsurf;
	struct weston_view *view;

	if (!surface || wl_list_empty(&surface->views))
		return NULL;

	shsurf = get_shell_surface(surface);
	if (shsurf)
		return shsurf->view;

	wl_list_for_each(view, &surface->views, surface_link)
		if (weston_view_is_mapped(view))
			return view;

	return container_of(surface->views.next, struct weston_view, surface_link);
}

static void
desktop_shell_destroy_surface(struct shell_surface *shsurf)
{
	struct shell_surface *shsurf_child, *tmp;

	if (shsurf->fullscreen.black_view)
		weston_shell_utils_curtain_destroy(shsurf->fullscreen.black_view);

	wl_list_for_each_safe(shsurf_child, tmp, &shsurf->children_list, children_link) {
		wl_list_remove(&shsurf_child->children_link);
		wl_list_init(&shsurf_child->children_link);
	}
	wl_list_remove(&shsurf->children_link);
	weston_desktop_surface_unlink_view(shsurf->view);
	weston_view_destroy(shsurf->view);

	wl_signal_emit(&shsurf->destroy_signal, shsurf);
	weston_surface_unref(shsurf->wsurface_anim_fade);

	if (shsurf->output_destroy_listener.notify) {
		wl_list_remove(&shsurf->output_destroy_listener.link);
		shsurf->output_destroy_listener.notify = NULL;
	}

	free(shsurf);
}

static void
shell_grab_start(struct shell_grab *grab,
		 const struct weston_pointer_grab_interface *interface,
		 struct shell_surface *shsurf,
		 struct weston_pointer *pointer,
		 enum weston_desktop_shell_cursor cursor)
{
	struct desktop_shell *shell = shsurf->shell;

	weston_seat_break_desktop_grabs(pointer->seat);

	grab->grab.interface = interface;
	grab->shsurf = shsurf;
	grab->shsurf_destroy_listener.notify = destroy_shell_grab_shsurf;
	wl_signal_add(&shsurf->destroy_signal,
		      &grab->shsurf_destroy_listener);

	shsurf->grabbed = 1;
	weston_pointer_start_grab(pointer, &grab->grab);
	if (shell->child.desktop_shell) {
		weston_desktop_shell_send_grab_cursor(shell->child.desktop_shell,
					       cursor);
		weston_pointer_set_focus(pointer,
					 get_default_view(shell->grab_surface));
	}
}

static void
get_panel_size(struct desktop_shell *shell,
	       struct weston_view *view,
	       int *width,
	       int *height)
{
	struct weston_coord_global a, b;
	struct weston_coord_surface tmp_s;

	tmp_s = weston_coord_surface(0, 0, view->surface);
	a = weston_coord_surface_to_global(view, tmp_s);

	tmp_s = weston_coord_surface(view->surface->width,
				     view->surface->height,
				     view->surface);
	b = weston_coord_surface_to_global(view, tmp_s);

	a.c = weston_coord_sub(b.c, a.c);
	*width = a.c.x;
	*height = a.c.y;
}

static void
get_output_panel_size(struct desktop_shell *shell,
		      struct weston_output *output,
		      int *width,
		      int *height)
{
	struct weston_view *view;

	*width = 0;
	*height = 0;

	if (!output)
		return;

	wl_list_for_each(view, &shell->panel_layer.view_list.link, layer_link.link) {
		if (view->surface->output == output) {
			get_panel_size(shell, view, width, height);
			return;
		}
	}

	/* the correct view wasn't found */
}

void
get_output_work_area(struct desktop_shell *shell,
		     struct weston_output *output,
		     pixman_rectangle32_t *area)
{
	int32_t panel_width = 0, panel_height = 0;

	if (!output) {
		area->x = 0;
		area->y = 0;
		area->width = 0;
		area->height = 0;

		return;
	}

	area->x = output->x;
	area->y = output->y;

	get_output_panel_size(shell, output, &panel_width, &panel_height);
	switch (shell->panel_position) {
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP:
	default:
		area->y += panel_height;
		/* fallthrough */
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM:
		area->width = output->width;
		area->height = output->height - panel_height;
		break;
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT:
		area->x += panel_width;
		/* fallthrough */
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_RIGHT:
		area->width = output->width - panel_width;
		area->height = output->height;
		break;
	}
}

static void
shell_grab_end(struct shell_grab *grab)
{
	if (grab->shsurf) {
		wl_list_remove(&grab->shsurf_destroy_listener.link);
		grab->shsurf->grabbed = 0;

		if (grab->shsurf->resize_edges) {
			grab->shsurf->resize_edges = 0;
		}
	}

	weston_pointer_end_grab(grab->grab.pointer);
}

static void
shell_touch_grab_start(struct shell_touch_grab *grab,
		       const struct weston_touch_grab_interface *interface,
		       struct shell_surface *shsurf,
		       struct weston_touch *touch)
{
	struct desktop_shell *shell = shsurf->shell;

	weston_seat_break_desktop_grabs(touch->seat);

	grab->grab.interface = interface;
	grab->shsurf = shsurf;
	grab->shsurf_destroy_listener.notify = destroy_shell_grab_shsurf;
	wl_signal_add(&shsurf->destroy_signal,
		      &grab->shsurf_destroy_listener);

	grab->touch = touch;
	shsurf->grabbed = 1;

	weston_touch_start_grab(touch, &grab->grab);
	if (shell->child.desktop_shell)
		weston_touch_set_focus(touch,
				       get_default_view(shell->grab_surface));
}

static void
shell_touch_grab_end(struct shell_touch_grab *grab)
{
	if (grab->shsurf) {
		wl_list_remove(&grab->shsurf_destroy_listener.link);
		grab->shsurf->grabbed = 0;
	}

	weston_touch_end_grab(grab->touch);
}

static void
shell_tablet_tool_grab_start(struct shell_tablet_tool_grab *grab,
			     const struct weston_tablet_tool_grab_interface *interface,
			     struct shell_surface *shsurf,
			     struct weston_tablet_tool *tool)
{
	struct desktop_shell *shell = shsurf->shell;

	weston_seat_break_desktop_grabs(tool->seat);

	grab->grab.interface = interface;
	grab->shsurf = shsurf;
	grab->shsurf_destroy_listener.notify = destroy_shell_grab_shsurf;
	wl_signal_add(&shsurf->destroy_signal, &grab->shsurf_destroy_listener);

	grab->tool = tool;
	shsurf->grabbed = 1;

	weston_tablet_tool_start_grab(tool, &grab->grab);
	if (shell->child.desktop_shell)
		weston_tablet_tool_set_focus(tool,
					     get_default_view(shell->grab_surface),
					     0);
}

static void
shell_tablet_tool_grab_end(struct shell_tablet_tool_grab *grab)
{
	if (grab->shsurf) {
		wl_list_remove(&grab->shsurf_destroy_listener.link);
		grab->shsurf->grabbed = 0;
	}

	weston_tablet_tool_end_grab(grab->tool);
}

static enum animation_type
get_animation_type(char *animation)
{
	if (!animation)
		return ANIMATION_NONE;

	if (!strcmp("zoom", animation))
		return ANIMATION_ZOOM;
	else if (!strcmp("fade", animation))
		return ANIMATION_FADE;
	else if (!strcmp("dim-layer", animation))
		return ANIMATION_DIM_LAYER;
	else
		return ANIMATION_NONE;
}

static void
shell_configuration(struct desktop_shell *shell)
{
	struct weston_config_section *section;
	struct weston_config *config;
	char *s, *client;
	bool allow_zap;

	config = wet_get_config(shell->compositor);
	section = weston_config_get_section(config, "shell", NULL, NULL);
	client = wet_get_libexec_path(WESTON_SHELL_CLIENT);
	weston_config_section_get_string(section, "client", &s, client);
	free(client);
	shell->client = s;

	weston_config_section_get_bool(section,
				       "allow-zap", &allow_zap, true);
	shell->allow_zap = allow_zap;

	shell->binding_modifier = weston_config_get_binding_modifier(config, MODIFIER_SUPER);

	weston_config_section_get_string(section, "animation", &s, "none");
	shell->win_animation_type = get_animation_type(s);
	free(s);
	weston_config_section_get_string(section, "close-animation", &s, "fade");
	shell->win_close_animation_type = get_animation_type(s);
	free(s);
	weston_config_section_get_string(section,
					 "startup-animation", &s, "fade");
	shell->startup_animation_type = get_animation_type(s);
	free(s);
	if (shell->startup_animation_type == ANIMATION_ZOOM)
		shell->startup_animation_type = ANIMATION_NONE;
	weston_config_section_get_string(section, "focus-animation", &s, "none");
	shell->focus_animation_type = get_animation_type(s);
	free(s);
}

static int
focus_surface_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "focus highlight effect for output %s",
			(surface->output ? surface->output->name : "NULL"));
}

/* no-op func for checking focus surface */
static void
focus_surface_committed(struct weston_surface *es,
			struct weston_coord_surface new_origin)
{
}

static bool
is_focus_view (struct weston_view *view)
{
	return (view->surface->committed == focus_surface_committed);
}

static struct focus_surface *
create_focus_surface(struct weston_compositor *ec,
		     struct weston_output *output)
{
	struct focus_surface *fsurf = NULL;
	struct weston_curtain_params curtain_params = {
		.r = 0.0, .g = 0.0, .b = 0.0, .a = 1.0,
		.x = output->x, .y = output->y,
		.width = output->width, .height = output->height,
		.surface_committed = focus_surface_committed,
		.get_label = focus_surface_get_label,
		.surface_private = NULL,
		.capture_input = false,
	};

	fsurf = malloc(sizeof *fsurf);
	if (!fsurf)
		return NULL;

	curtain_params.surface_private = fsurf;

	fsurf->curtain = weston_shell_utils_curtain_create(ec, &curtain_params);
	weston_view_set_output(fsurf->curtain->view, output);
	fsurf->curtain->view->is_mapped = true;

	return fsurf;
}

static void
focus_surface_destroy(struct focus_surface *fsurf)
{
	weston_shell_utils_curtain_destroy(fsurf->curtain);
	free(fsurf);
}

static void
focus_animation_done(struct weston_view_animation *animation, void *data)
{
	struct workspace *ws = data;

	ws->focus_animation = NULL;
}

static void
focus_state_destroy(struct focus_state *state)
{
	wl_list_remove(&state->seat_destroy_listener.link);
	wl_list_remove(&state->surface_destroy_listener.link);
	free(state);
}

static void
focus_state_seat_destroy(struct wl_listener *listener, void *data)
{
	struct focus_state *state = container_of(listener,
						 struct focus_state,
						 seat_destroy_listener);

	wl_list_remove(&state->link);
	focus_state_destroy(state);
}

static void
focus_state_surface_destroy(struct wl_listener *listener, void *data)
{
	struct focus_state *state = container_of(listener,
						 struct focus_state,
						 surface_destroy_listener);
	struct weston_surface *main_surface;
	struct weston_view *next;
	struct weston_view *view;

	main_surface = weston_surface_get_main_surface(state->keyboard_focus);

	next = NULL;
	wl_list_for_each(view,
			 &state->ws->layer.view_list.link, layer_link.link) {
		if (view->surface == main_surface)
			continue;
		if (is_focus_view(view))
			continue;
		if (!get_shell_surface(view->surface))
			continue;

		next = view;
		break;
	}

	/* if the focus was a sub-surface, activate its main surface */
	if (main_surface != state->keyboard_focus)
		next = get_default_view(main_surface);

	if (next) {
		if (state->keyboard_focus) {
			wl_list_remove(&state->surface_destroy_listener.link);
			wl_list_init(&state->surface_destroy_listener.link);
		}
		state->keyboard_focus = NULL;
		activate(state->shell, next, state->seat,
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
	} else {
		if (state->shell->focus_animation_type == ANIMATION_DIM_LAYER) {
			if (state->ws->focus_animation)
				weston_view_animation_destroy(state->ws->focus_animation);

			state->ws->focus_animation = weston_fade_run(
				state->ws->fsurf_front->curtain->view,
				state->ws->fsurf_front->curtain->view->alpha, 0.0, 300,
				focus_animation_done, state->ws);
		}

		wl_list_remove(&state->link);
		focus_state_destroy(state);
	}
}

static struct focus_state *
focus_state_create(struct desktop_shell *shell, struct weston_seat *seat,
		   struct workspace *ws)
{
	struct focus_state *state;

	state = malloc(sizeof *state);
	if (state == NULL)
		return NULL;

	state->shell = shell;
	state->keyboard_focus = NULL;
	state->ws = ws;
	state->seat = seat;
	wl_list_insert(&ws->focus_list, &state->link);

	state->seat_destroy_listener.notify = focus_state_seat_destroy;
	state->surface_destroy_listener.notify = focus_state_surface_destroy;
	wl_signal_add(&seat->destroy_signal,
		      &state->seat_destroy_listener);
	wl_list_init(&state->surface_destroy_listener.link);

	return state;
}

static struct focus_state *
ensure_focus_state(struct desktop_shell *shell, struct weston_seat *seat)
{
	struct workspace *ws = get_current_workspace(shell);
	struct focus_state *state;

	wl_list_for_each(state, &ws->focus_list, link)
		if (state->seat == seat)
			break;

	if (&state->link == &ws->focus_list)
		state = focus_state_create(shell, seat, ws);

	return state;
}

static void
focus_state_set_focus(struct focus_state *state,
		      struct weston_surface *surface)
{
	if (state->keyboard_focus) {
		wl_list_remove(&state->surface_destroy_listener.link);
		wl_list_init(&state->surface_destroy_listener.link);
	}

	state->keyboard_focus = surface;
	if (surface)
		wl_signal_add(&surface->destroy_signal,
			      &state->surface_destroy_listener);
}

static void
restore_focus_state(struct desktop_shell *shell, struct workspace *ws)
{
	struct focus_state *state, *next;
	struct weston_surface *surface;
	struct wl_list pending_seat_list;
	struct weston_seat *seat, *next_seat;

	/* Temporarily steal the list of seats so that we can keep
	 * track of the seats we've already processed */
	wl_list_init(&pending_seat_list);
	wl_list_insert_list(&pending_seat_list, &shell->compositor->seat_list);
	wl_list_init(&shell->compositor->seat_list);

	wl_list_for_each_safe(state, next, &ws->focus_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(state->seat);

		wl_list_remove(&state->seat->link);
		wl_list_insert(&shell->compositor->seat_list,
			       &state->seat->link);

		if (!keyboard)
			continue;

		surface = state->keyboard_focus;

		weston_keyboard_set_focus(keyboard, surface);
	}

	/* For any remaining seats that we don't have a focus state
	 * for we'll reset the keyboard focus to NULL */
	wl_list_for_each_safe(seat, next_seat, &pending_seat_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);

		wl_list_insert(&shell->compositor->seat_list, &seat->link);

		if (!keyboard)
			continue;

		weston_keyboard_set_focus(keyboard, NULL);
	}
}

static void
drop_focus_state(struct desktop_shell *shell, struct workspace *ws,
		 struct weston_surface *surface)
{
	struct focus_state *state;

	wl_list_for_each(state, &ws->focus_list, link)
		if (state->keyboard_focus == surface)
			focus_state_set_focus(state, NULL);
}

static void
animate_focus_change(struct desktop_shell *shell, struct workspace *ws,
		     struct weston_view *from, struct weston_view *to)
{
	struct weston_output *output;
	bool focus_surface_created = false;

	/* FIXME: Only support dim animation using two layers */
	if (from == to || shell->focus_animation_type != ANIMATION_DIM_LAYER)
		return;

	output = weston_shell_utils_get_default_output(shell->compositor);
	if (ws->fsurf_front == NULL && (from || to)) {
		ws->fsurf_front = create_focus_surface(shell->compositor, output);
		if (ws->fsurf_front == NULL)
			return;
		ws->fsurf_front->curtain->view->alpha = 0.0;

		ws->fsurf_back = create_focus_surface(shell->compositor, output);
		if (ws->fsurf_back == NULL) {
			focus_surface_destroy(ws->fsurf_front);
			return;
		}
		ws->fsurf_back->curtain->view->alpha = 0.0;

		focus_surface_created = true;
	} else {
		weston_layer_entry_remove(&ws->fsurf_front->curtain->view->layer_link);
		weston_layer_entry_remove(&ws->fsurf_back->curtain->view->layer_link);
	}

	if (ws->focus_animation) {
		weston_view_animation_destroy(ws->focus_animation);
		ws->focus_animation = NULL;
	}

	if (to)
		weston_layer_entry_insert(&to->layer_link,
					  &ws->fsurf_front->curtain->view->layer_link);
	else if (from)
		weston_layer_entry_insert(&ws->layer.view_list,
					  &ws->fsurf_front->curtain->view->layer_link);

	if (focus_surface_created) {
		ws->focus_animation = weston_fade_run(
			ws->fsurf_front->curtain->view,
			ws->fsurf_front->curtain->view->alpha, 0.4, 300,
			focus_animation_done, ws);
	} else if (from) {
		weston_layer_entry_insert(&from->layer_link,
					  &ws->fsurf_back->curtain->view->layer_link);
		ws->focus_animation = weston_stable_fade_run(
			ws->fsurf_front->curtain->view, 0.0,
			ws->fsurf_back->curtain->view, 0.4,
			focus_animation_done, ws);
	} else if (to) {
		weston_layer_entry_insert(&ws->layer.view_list,
					  &ws->fsurf_back->curtain->view->layer_link);
		ws->focus_animation = weston_stable_fade_run(
			ws->fsurf_front->curtain->view, 0.0,
			ws->fsurf_back->curtain->view, 0.4,
			focus_animation_done, ws);
	}
}

static void
desktop_shell_destroy_layer(struct weston_layer *layer);

static void
workspace_destroy(struct workspace *ws)
{
	struct focus_state *state, *next;

	wl_list_for_each_safe(state, next, &ws->focus_list, link)
		focus_state_destroy(state);

	if (ws->fsurf_front)
		focus_surface_destroy(ws->fsurf_front);
	if (ws->fsurf_back)
		focus_surface_destroy(ws->fsurf_back);

	desktop_shell_destroy_layer(&ws->layer);
}

static void
seat_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_seat *seat = data;
	struct focus_state *state, *next;
	struct workspace *ws = container_of(listener,
					    struct workspace,
					    seat_destroyed_listener);

	wl_list_for_each_safe(state, next, &ws->focus_list, link)
		if (state->seat == seat)
			wl_list_remove(&state->link);
}

static void
workspace_create(struct desktop_shell *shell)
{
	struct workspace *ws = &shell->workspace;

	weston_layer_init(&ws->layer, shell->compositor);
	weston_layer_set_position(&ws->layer, WESTON_LAYER_POSITION_NORMAL);

	wl_list_init(&ws->focus_list);
	wl_list_init(&ws->seat_destroyed_listener.link);
	ws->seat_destroyed_listener.notify = seat_destroyed;
	ws->fsurf_front = NULL;
	ws->fsurf_back = NULL;
	ws->focus_animation = NULL;
}

struct workspace *
get_current_workspace(struct desktop_shell *shell)
{
	return &shell->workspace;
}

static void
surface_keyboard_focus_lost(struct weston_surface *surface)
{
	struct weston_compositor *compositor = surface->compositor;
	struct weston_seat *seat;
	struct weston_surface *focus;

	wl_list_for_each(seat, &compositor->seat_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);

		if (!keyboard)
			continue;

		focus = weston_surface_get_main_surface(keyboard->focus);
		if (focus == surface)
			weston_keyboard_set_focus(keyboard, NULL);
	}
}

static void
touch_move_grab_down(struct weston_touch_grab *grab,
		     const struct timespec *time,
		     int touch_id, wl_fixed_t x, wl_fixed_t y)
{
}

static void
touch_move_grab_up(struct weston_touch_grab *grab, const struct timespec *time,
		   int touch_id)
{
	struct weston_touch_move_grab *move =
		(struct weston_touch_move_grab *) container_of(
			grab, struct shell_touch_grab, grab);

	if (touch_id == 0)
		move->active = 0;

	if (grab->touch->num_tp == 0) {
		shell_touch_grab_end(&move->base);
		free(move);
	}
}

static void
touch_move_grab_motion(struct weston_touch_grab *grab,
		       const struct timespec *time, int touch_id,
		       wl_fixed_t x, wl_fixed_t y)
{
	struct weston_touch_move_grab *move = (struct weston_touch_move_grab *) grab;
	struct shell_surface *shsurf = move->base.shsurf;
	struct weston_surface *es;
	int dx = wl_fixed_to_int(grab->touch->grab_x + move->dx);
	int dy = wl_fixed_to_int(grab->touch->grab_y + move->dy);

	if (!shsurf || !shsurf->desktop_surface || !move->active)
		return;

	es = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	weston_view_set_position(shsurf->view, dx, dy);

	weston_compositor_schedule_repaint(es->compositor);
}

static void
touch_move_grab_frame(struct weston_touch_grab *grab)
{
}

static void
touch_move_grab_cancel(struct weston_touch_grab *grab)
{
	struct weston_touch_move_grab *move =
		(struct weston_touch_move_grab *) container_of(
			grab, struct shell_touch_grab, grab);

	shell_touch_grab_end(&move->base);
	free(move);
}

static const struct weston_touch_grab_interface touch_move_grab_interface = {
	touch_move_grab_down,
	touch_move_grab_up,
	touch_move_grab_motion,
	touch_move_grab_frame,
	touch_move_grab_cancel,
};

static int
surface_touch_move(struct shell_surface *shsurf, struct weston_touch *touch)
{
	struct weston_touch_move_grab *move;

	if (!shsurf)
		return -1;

	if (weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return 0;

	move = malloc(sizeof *move);
	if (!move)
		return -1;

	move->active = 1;
	move->dx = wl_fixed_from_double(shsurf->view->geometry.pos_offset.x) -
		   touch->grab_x;
	move->dy = wl_fixed_from_double(shsurf->view->geometry.pos_offset.y) -
		   touch->grab_y;

	shell_touch_grab_start(&move->base, &touch_move_grab_interface, shsurf,
			       touch);

	return 0;
}

static void
noop_grab_focus(struct weston_pointer_grab *grab)
{
}

static void
noop_grab_axis(struct weston_pointer_grab *grab,
	       const struct timespec *time,
	       struct weston_pointer_axis_event *event)
{
}

static void
noop_grab_axis_source(struct weston_pointer_grab *grab,
		      uint32_t source)
{
}

static void
noop_grab_frame(struct weston_pointer_grab *grab)
{
}

static void
constrain_position(struct weston_move_grab *move, int *cx, int *cy)
{
	struct shell_surface *shsurf = move->base.shsurf;
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct weston_pointer *pointer = move->base.grab.pointer;
	int x, y, bottom;
	const int safety = 50;
	pixman_rectangle32_t area;
	struct weston_geometry geometry;

	x = pointer->pos.c.x + wl_fixed_to_double(move->dx);
	y = pointer->pos.c.y + wl_fixed_to_double(move->dy);

	if (shsurf->shell->panel_position ==
	    WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP) {
		get_output_work_area(shsurf->shell, surface->output, &area);
		geometry =
			weston_desktop_surface_get_geometry(shsurf->desktop_surface);

		bottom = y + geometry.height + geometry.y;
		if (bottom - safety < area.y)
			y = area.y + safety - geometry.height
			  - geometry.y;

		if (move->client_initiated &&
		    y + geometry.y < area.y)
			y = area.y - geometry.y;
	}

	*cx = x;
	*cy = y;
}

static void
move_grab_motion(struct weston_pointer_grab *grab,
		 const struct timespec *time,
		 struct weston_pointer_motion_event *event)
{
	struct weston_move_grab *move = (struct weston_move_grab *) grab;
	struct weston_pointer *pointer = grab->pointer;
	struct shell_surface *shsurf = move->base.shsurf;
	struct weston_surface *surface;
	int cx, cy;

	weston_pointer_move(pointer, event);
	if (!shsurf || !shsurf->desktop_surface)
		return;

	surface = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	constrain_position(move, &cx, &cy);

	weston_view_set_position(shsurf->view, cx, cy);

	weston_compositor_schedule_repaint(surface->compositor);
}

static void
move_grab_button(struct weston_pointer_grab *grab,
		 const struct timespec *time, uint32_t button, uint32_t state_w)
{
	struct shell_grab *shell_grab = container_of(grab, struct shell_grab,
						    grab);
	struct weston_pointer *pointer = grab->pointer;
	enum wl_pointer_button_state state = state_w;

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		shell_grab_end(shell_grab);
		free(grab);
	}
}

static void
move_grab_cancel(struct weston_pointer_grab *grab)
{
	struct shell_grab *shell_grab =
		container_of(grab, struct shell_grab, grab);

	shell_grab_end(shell_grab);
	free(grab);
}

static const struct weston_pointer_grab_interface move_grab_interface = {
	noop_grab_focus,
	move_grab_motion,
	move_grab_button,
	noop_grab_axis,
	noop_grab_axis_source,
	noop_grab_frame,
	move_grab_cancel,
};

static int
surface_move(struct shell_surface *shsurf, struct weston_pointer *pointer,
	     bool client_initiated)
{
	struct weston_move_grab *move;
	struct weston_coord offset;

	if (!shsurf)
		return -1;

	if (shsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return 0;

	move = malloc(sizeof *move);
	if (!move)
		return -1;

	offset = weston_coord_sub(shsurf->view->geometry.pos_offset,
				  pointer->grab_pos.c);
	move->dx = wl_fixed_from_double(offset.x);
	move->dy = wl_fixed_from_double(offset.y);
	move->client_initiated = client_initiated;

	weston_desktop_surface_set_orientation(shsurf->desktop_surface,
					       WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE);
	shsurf->orientation = WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE;
	shell_grab_start(&move->base, &move_grab_interface, shsurf,
			 pointer, WESTON_DESKTOP_SHELL_CURSOR_MOVE);

	return 0;
}

struct weston_resize_grab {
	struct shell_grab base;
	uint32_t edges;
	int32_t width, height;
};

static void
tablet_tool_noop_grab_proximity_in(struct weston_tablet_tool_grab *grab,
				   const struct timespec *time,
				   struct weston_tablet *tablet)
{
}

static void
tablet_tool_move_grab_proximity_out(struct weston_tablet_tool_grab *grab,
				    const struct timespec *time)
{
	struct weston_tablet_tool_move_grab *move =
		(struct weston_tablet_tool_move_grab *)grab;

	shell_tablet_tool_grab_end(&move->base);
	free(grab);
}

static void
tablet_tool_move_grab_up(struct weston_tablet_tool_grab *grab,
			 const struct timespec *time)
{
	struct weston_tablet_tool_move_grab *move =
		(struct weston_tablet_tool_move_grab *)grab;

	shell_tablet_tool_grab_end(&move->base);
	free(grab);
}

static void
tablet_tool_noop_grab_down(struct weston_tablet_tool_grab *grab,
			   const struct timespec *time)
{
}

static void
tablet_tool_move_grab_motion(struct weston_tablet_tool_grab *grab,
			     const struct timespec *time,
			     struct weston_coord_global pos)
{
	struct weston_tablet_tool_move_grab *move =
		(struct weston_tablet_tool_move_grab *)grab;
	struct shell_surface *shsurf = move->base.shsurf;
	struct weston_surface *es;

	weston_tablet_tool_cursor_move(grab->tool, pos);

	if (!shsurf)
		return;

	es = weston_desktop_surface_get_surface(shsurf->desktop_surface);
	weston_view_set_position(shsurf->view,
				 pos.c.x + wl_fixed_to_double(move->dx),
				 pos.c.y + wl_fixed_to_double(move->dy));
	weston_compositor_schedule_repaint(es->compositor);
}

static void
tablet_tool_noop_grab_pressure(struct weston_tablet_tool_grab *grab,
			       const struct timespec *time,
			       uint32_t pressure)
{
}

static void
tablet_tool_noop_grab_distance(struct weston_tablet_tool_grab *grab,
			       const struct timespec *time,
			       uint32_t distance)
{
}

static void
tablet_tool_noop_grab_tilt(struct weston_tablet_tool_grab *grab,
			   const struct timespec *time,
			   int32_t tilt_x, int32_t tilt_y)
{
}

static void tablet_tool_noop_grab_button(struct weston_tablet_tool_grab *grab,
					 const struct timespec *time, uint32_t button,
					 uint32_t state)
{
}

static void
tablet_tool_noop_grab_frame(struct weston_tablet_tool_grab *grab,
			    const struct timespec *time)
{
}

static void
tablet_tool_move_grab_cancel(struct weston_tablet_tool_grab *grab)
{
	struct weston_tablet_tool_move_grab *move =
		(struct weston_tablet_tool_move_grab *)grab;

	shell_tablet_tool_grab_end(&move->base);
	free(grab);
}

static struct weston_tablet_tool_grab_interface tablet_tool_move_grab_interface = {
	tablet_tool_noop_grab_proximity_in,
	tablet_tool_move_grab_proximity_out,
	tablet_tool_move_grab_motion,
	tablet_tool_noop_grab_down,
	tablet_tool_move_grab_up,
	tablet_tool_noop_grab_pressure,
	tablet_tool_noop_grab_distance,
	tablet_tool_noop_grab_tilt,
	tablet_tool_noop_grab_button,
	tablet_tool_noop_grab_frame,
	tablet_tool_move_grab_cancel,
};

static int
surface_tablet_tool_move(struct shell_surface *shsurf, struct weston_tablet_tool *tool)
{
	struct weston_tablet_tool_move_grab *move;
	struct weston_coord offset;

	if (!shsurf)
		return -1;

	if (shsurf->state.fullscreen || shsurf->state.maximized)
		return 0;

	move = malloc(sizeof(*move));
	if (!move)
		return -1;

	offset = weston_coord_sub(shsurf->view->geometry.pos_offset,
				  tool->grab_pos.c);
	move->dx = wl_fixed_from_double(offset.x);
	move->dy = wl_fixed_from_double(offset.y);

	shell_tablet_tool_grab_start(&move->base, &tablet_tool_move_grab_interface,
				     shsurf, tool);

	return 0;
}


static void
resize_grab_motion(struct weston_pointer_grab *grab,
		   const struct timespec *time,
		   struct weston_pointer_motion_event *event)
{
	struct weston_resize_grab *resize = (struct weston_resize_grab *) grab;
	struct weston_pointer *pointer = grab->pointer;
	struct shell_surface *shsurf = resize->base.shsurf;
	int32_t width, height;
	struct weston_size min_size, max_size;
	struct weston_coord_surface tmp_s;
	wl_fixed_t from_x, from_y;
	wl_fixed_t to_x, to_y;

	weston_pointer_move(pointer, event);

	if (!shsurf || !shsurf->desktop_surface)
		return;

	weston_view_update_transform(shsurf->view);

	tmp_s = weston_coord_global_to_surface(shsurf->view, pointer->grab_pos);
	from_x = wl_fixed_from_double(tmp_s.c.x);
	from_y = wl_fixed_from_double(tmp_s.c.y);
	tmp_s = weston_coord_global_to_surface(shsurf->view, pointer->pos);
	to_x = wl_fixed_from_double(tmp_s.c.x);
	to_y = wl_fixed_from_double(tmp_s.c.y);

	width = resize->width;
	if (resize->edges & WESTON_DESKTOP_SURFACE_EDGE_LEFT) {
		width += wl_fixed_to_int(from_x - to_x);
	} else if (resize->edges & WESTON_DESKTOP_SURFACE_EDGE_RIGHT) {
		width += wl_fixed_to_int(to_x - from_x);
	}

	height = resize->height;
	if (resize->edges & WESTON_DESKTOP_SURFACE_EDGE_TOP) {
		height += wl_fixed_to_int(from_y - to_y);
	} else if (resize->edges & WESTON_DESKTOP_SURFACE_EDGE_BOTTOM) {
		height += wl_fixed_to_int(to_y - from_y);
	}

	max_size = weston_desktop_surface_get_max_size(shsurf->desktop_surface);
	min_size = weston_desktop_surface_get_min_size(shsurf->desktop_surface);

	min_size.width = MAX(1, min_size.width);
	min_size.height = MAX(1, min_size.height);

	if (width < min_size.width)
		width = min_size.width;
	else if (max_size.width > 0 && width > max_size.width)
		width = max_size.width;
	if (height < min_size.height)
		height = min_size.height;
	else if (max_size.height > 0 && height > max_size.height)
		height = max_size.height;
	weston_desktop_surface_set_size(shsurf->desktop_surface, width, height);
}

static void
resize_grab_button(struct weston_pointer_grab *grab,
		   const struct timespec *time,
		   uint32_t button, uint32_t state_w)
{
	struct weston_resize_grab *resize = (struct weston_resize_grab *) grab;
	struct weston_pointer *pointer = grab->pointer;
	enum wl_pointer_button_state state = state_w;

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (resize->base.shsurf && resize->base.shsurf->desktop_surface) {
			struct weston_desktop_surface *desktop_surface =
				resize->base.shsurf->desktop_surface;
			weston_desktop_surface_set_resizing(desktop_surface,
							    false);
			weston_desktop_surface_set_size(desktop_surface, 0, 0);
		}

		shell_grab_end(&resize->base);
		free(grab);
	}
}

static void
resize_grab_cancel(struct weston_pointer_grab *grab)
{
	struct weston_resize_grab *resize = (struct weston_resize_grab *) grab;

	if (resize->base.shsurf && resize->base.shsurf->desktop_surface) {
		struct weston_desktop_surface *desktop_surface =
			resize->base.shsurf->desktop_surface;
		weston_desktop_surface_set_resizing(desktop_surface, false);
		weston_desktop_surface_set_size(desktop_surface, 0, 0);
	}

	shell_grab_end(&resize->base);
	free(grab);
}

static const struct weston_pointer_grab_interface resize_grab_interface = {
	noop_grab_focus,
	resize_grab_motion,
	resize_grab_button,
	noop_grab_axis,
	noop_grab_axis_source,
	noop_grab_frame,
	resize_grab_cancel,
};


static int
surface_resize(struct shell_surface *shsurf,
	       struct weston_pointer *pointer, uint32_t edges)
{
	struct weston_resize_grab *resize;
	const unsigned resize_topbottom =
		WESTON_DESKTOP_SURFACE_EDGE_TOP | WESTON_DESKTOP_SURFACE_EDGE_BOTTOM;
	const unsigned resize_leftright =
		WESTON_DESKTOP_SURFACE_EDGE_LEFT | WESTON_DESKTOP_SURFACE_EDGE_RIGHT;
	const unsigned resize_any = resize_topbottom | resize_leftright;
	struct weston_geometry geometry;

	if (shsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return 0;

	/* Check for invalid edge combinations. */
	if (edges == WESTON_DESKTOP_SURFACE_EDGE_NONE || edges > resize_any ||
	    (edges & resize_topbottom) == resize_topbottom ||
	    (edges & resize_leftright) == resize_leftright)
		return 0;

	resize = malloc(sizeof *resize);
	if (!resize)
		return -1;

	resize->edges = edges;

	geometry = weston_desktop_surface_get_geometry(shsurf->desktop_surface);
	resize->width = geometry.width;
	resize->height = geometry.height;

	shsurf->resize_edges = edges;
	weston_desktop_surface_set_resizing(shsurf->desktop_surface, true);
	weston_desktop_surface_set_orientation(shsurf->desktop_surface,
					       WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE);
	shsurf->orientation = WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE;
	shell_grab_start(&resize->base, &resize_grab_interface, shsurf,
			 pointer, edges);

	return 0;
}

static void
busy_cursor_grab_focus(struct weston_pointer_grab *base)
{
	struct shell_grab *grab = (struct shell_grab *) base;
	struct weston_pointer *pointer = base->pointer;
	struct weston_desktop_surface *desktop_surface;
	struct weston_view *view;

	view = weston_compositor_pick_view(pointer->seat->compositor,
					   pointer->pos);
	desktop_surface = weston_surface_get_desktop_surface(view->surface);

	if (!grab->shsurf || grab->shsurf->desktop_surface != desktop_surface) {
		shell_grab_end(grab);
		free(grab);
	}
}

static void
busy_cursor_grab_motion(struct weston_pointer_grab *grab,
			const struct timespec *time,
			struct weston_pointer_motion_event *event)
{
	weston_pointer_move(grab->pointer, event);
}

static void
busy_cursor_grab_button(struct weston_pointer_grab *base,
			const struct timespec *time,
			uint32_t button, uint32_t state)
{
	struct shell_grab *grab = (struct shell_grab *) base;
	struct shell_surface *shsurf = grab->shsurf;
	struct weston_pointer *pointer = grab->grab.pointer;
	struct weston_seat *seat = pointer->seat;

	if (shsurf && button == BTN_LEFT && state) {
		activate(shsurf->shell, shsurf->view, seat,
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
		surface_move(shsurf, pointer, false);
	} else if (shsurf && button == BTN_RIGHT && state) {
		activate(shsurf->shell, shsurf->view, seat,
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
		surface_rotate(shsurf, pointer);
	}
}

static void
busy_cursor_grab_cancel(struct weston_pointer_grab *base)
{
	struct shell_grab *grab = (struct shell_grab *) base;

	shell_grab_end(grab);
	free(grab);
}

static const struct weston_pointer_grab_interface busy_cursor_grab_interface = {
	busy_cursor_grab_focus,
	busy_cursor_grab_motion,
	busy_cursor_grab_button,
	noop_grab_axis,
	noop_grab_axis_source,
	noop_grab_frame,
	busy_cursor_grab_cancel,
};

static void
handle_pointer_focus(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer = data;
	struct weston_view *view = pointer->focus;
	struct shell_surface *shsurf;
	struct weston_desktop_client *client;

	if (!view)
		return;

	shsurf = get_shell_surface(view->surface);
	if (!shsurf)
		return;

	client = weston_desktop_surface_get_client(shsurf->desktop_surface);

	if (shsurf->unresponsive)
		set_busy_cursor(shsurf, pointer);
	else
		weston_desktop_client_ping(client);
}

static void
has_keyboard_focused_child_callback(struct weston_desktop_surface *surface,
				    void *user_data);

static void
has_keyboard_focused_child_callback(struct weston_desktop_surface *surface,
				    void *user_data)
{
	struct weston_surface *es = weston_desktop_surface_get_surface(surface);
	struct shell_surface *shsurf = get_shell_surface(es);
	bool *has_keyboard_focus = user_data;

  if (!shsurf) {
    *has_keyboard_focus = false;
    return;
  } else if (shsurf->focus_count > 0) {
		*has_keyboard_focus = true;
		return;
	}

	weston_desktop_surface_foreach_child(shsurf->desktop_surface,
					     has_keyboard_focused_child_callback,
					     &has_keyboard_focus);
}

static bool
has_keyboard_focused_child(struct shell_surface *shsurf)
{
	bool has_keyboard_focus = false;

	if (shsurf->focus_count > 0)
		return true;

	weston_desktop_surface_foreach_child(shsurf->desktop_surface,
					     has_keyboard_focused_child_callback,
					     &has_keyboard_focus);

	return has_keyboard_focus;
}

static void
sync_surface_activated_state(struct shell_surface *shsurf)
{
	struct weston_desktop_surface *surface = shsurf->desktop_surface;
	struct weston_desktop_surface *parent;
	struct weston_surface *parent_surface;

	parent = weston_desktop_surface_get_parent(surface);
	if (parent) {
		parent_surface = weston_desktop_surface_get_surface(parent);
		sync_surface_activated_state(get_shell_surface(parent_surface));
		return;
	}

	if (has_keyboard_focused_child(shsurf))
		weston_desktop_surface_set_activated(surface, true);
	else
		weston_desktop_surface_set_activated(surface, false);
}

static void
handle_tablet_tool_focus(struct wl_listener *listener, void *data)
{
	struct weston_tablet_tool *tool = data;
	struct weston_view *view = tool->focus;
	struct shell_surface *shsurf;
	struct weston_desktop_client *client;

	if (!view)
		return;

	shsurf = get_shell_surface(view->surface);
	if (!shsurf)
		return;

	client = weston_desktop_surface_get_client(shsurf->desktop_surface);

	weston_desktop_client_ping(client);
}

static void
shell_surface_deactivate(struct shell_surface *shsurf)
{
	if (--shsurf->focus_count == 0)
		sync_surface_activated_state(shsurf);
}

static void
shell_surface_activate(struct shell_surface *shsurf)
{
	if (shsurf->focus_count++ == 0)
		sync_surface_activated_state(shsurf);
}

/* The surface will be inserted into the list immediately after the link
 * returned by this function (i.e. will be stacked immediately above the
 * returned link). */
static struct weston_layer_entry *
shell_surface_calculate_layer_link (struct shell_surface *shsurf)
{
	struct workspace *ws;

	if (weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) &&
	    !shsurf->state.lowered) {
		return &shsurf->shell->fullscreen_layer.view_list;
	}

	/* Move the surface to a normal workspace layer so that surfaces
	 * which were previously fullscreen or transient are no longer
	 * rendered on top. */
	ws = get_current_workspace(shsurf->shell);
	return &ws->layer.view_list;
}

static void
shell_surface_update_child_surface_layers (struct shell_surface *shsurf)
{
	weston_desktop_surface_propagate_layer(shsurf->desktop_surface);
}

/* Update the surface’s layer. Mark both the old and new views as having dirty
 * geometry to ensure the changes are redrawn.
 *
 * If any child surfaces exist and are mapped, ensure they’re in the same layer
 * as this surface. */
static void
shell_surface_update_layer(struct shell_surface *shsurf)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct weston_layer_entry *new_layer_link;

	new_layer_link = shell_surface_calculate_layer_link(shsurf);

	if (new_layer_link == NULL)
		return;
	if (new_layer_link == &shsurf->view->layer_link)
		return;

	weston_view_geometry_dirty(shsurf->view);
	weston_layer_entry_remove(&shsurf->view->layer_link);
	weston_layer_entry_insert(new_layer_link, &shsurf->view->layer_link);
	weston_view_geometry_dirty(shsurf->view);
	weston_surface_damage(surface);

	shell_surface_update_child_surface_layers(shsurf);
}

static void
notify_output_destroy(struct wl_listener *listener, void *data)
{
	struct shell_surface *shsurf =
		container_of(listener,
			     struct shell_surface, output_destroy_listener);

	shsurf->output = NULL;
	shsurf->output_destroy_listener.notify = NULL;
}

static void
shell_surface_set_output(struct shell_surface *shsurf,
                         struct weston_output *output)
{
	struct weston_surface *es =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);

	/* get the default output, if the client set it as NULL
	   check whether the output is available */
	if (output)
		shsurf->output = output;
	else if (es->output)
		shsurf->output = es->output;
	else
		shsurf->output = weston_shell_utils_get_default_output(es->compositor);

	if (shsurf->output_destroy_listener.notify) {
		wl_list_remove(&shsurf->output_destroy_listener.link);
		shsurf->output_destroy_listener.notify = NULL;
	}

	if (!shsurf->output)
		return;

	shsurf->output_destroy_listener.notify = notify_output_destroy;
	wl_signal_add(&shsurf->output->destroy_signal,
		      &shsurf->output_destroy_listener);
}

static void
weston_view_set_initial_position(struct weston_view *view,
				 struct desktop_shell *shell);

static void
unset_fullscreen(struct shell_surface *shsurf)
{
	/* Unset the fullscreen output, driver configuration and transforms. */
	wl_list_remove(&shsurf->fullscreen.transform.link);
	wl_list_init(&shsurf->fullscreen.transform.link);

	if (shsurf->fullscreen.black_view)
		weston_shell_utils_curtain_destroy(shsurf->fullscreen.black_view);
	shsurf->fullscreen.black_view = NULL;

	if (shsurf->saved_position_valid)
		weston_view_set_position(shsurf->view,
					 shsurf->saved_pos.c.x,
					 shsurf->saved_pos.c.y);
	else
		weston_view_set_initial_position(shsurf->view, shsurf->shell);
	shsurf->saved_position_valid = false;

	weston_desktop_surface_set_orientation(shsurf->desktop_surface,
					       shsurf->orientation);

	if (shsurf->saved_rotation_valid) {
		wl_list_insert(&shsurf->view->geometry.transformation_list,
		               &shsurf->rotation.transform.link);
		shsurf->saved_rotation_valid = false;
	}
}

static void
unset_maximized(struct shell_surface *shsurf)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);

	/* undo all maximized things here */
	shell_surface_set_output(shsurf,
			weston_shell_utils_get_default_output(surface->compositor));

	if (shsurf->saved_position_valid)
		weston_view_set_position(shsurf->view,
					 shsurf->saved_pos.c.x,
					 shsurf->saved_pos.c.y);
	else
		weston_view_set_initial_position(shsurf->view, shsurf->shell);
	shsurf->saved_position_valid = false;

	weston_desktop_surface_set_orientation(shsurf->desktop_surface,
					       shsurf->orientation);

	if (shsurf->saved_rotation_valid) {
		wl_list_insert(&shsurf->view->geometry.transformation_list,
			       &shsurf->rotation.transform.link);
		shsurf->saved_rotation_valid = false;
	}
}

static void
set_minimized(struct weston_surface *surface)
{
	struct shell_surface *shsurf;
	struct workspace *current_ws;
	struct weston_view *view;
	struct weston_subsurface *subsurface;

	view = get_default_view(surface);
	if (!view)
		return;

	assert(weston_surface_get_main_surface(view->surface) == view->surface);

	shsurf = get_shell_surface(surface);
	current_ws = get_current_workspace(shsurf->shell);

	weston_layer_entry_remove(&view->layer_link);
	weston_layer_entry_insert(&shsurf->shell->minimized_layer.view_list, &view->layer_link);

	drop_focus_state(shsurf->shell, current_ws, view->surface);
	surface_keyboard_focus_lost(surface);

	shell_surface_update_child_surface_layers(shsurf);

	weston_view_damage_below(shsurf->view);
	wl_list_for_each(subsurface, &surface->subsurface_list, parent_link) {
		wl_list_for_each(view, &subsurface->surface->views, surface_link)
			weston_view_damage_below(view);
	}
}


static struct desktop_shell *
shell_surface_get_shell(struct shell_surface *shsurf)
{
	return shsurf->shell;
}

static int
black_surface_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	struct weston_view *fs_view = surface->committed_private;
	struct weston_surface *fs_surface = fs_view->surface;
	int n;
	int rem;
	int ret;

	n = snprintf(buf, len, "black background surface for ");
	if (n < 0)
		return n;

	rem = (int)len - n;
	if (rem < 0)
		rem = 0;

	if (fs_surface->get_label)
		ret = fs_surface->get_label(fs_surface, buf + n, rem);
	else
		ret = snprintf(buf + n, rem, "<unknown>");

	if (ret < 0)
		return n;

	return n + ret;
}

static void
black_surface_committed(struct weston_surface *es,
			struct weston_coord_surface new_origin)
{
}

static bool
is_black_surface_view(struct weston_view *view, struct weston_view **fs_view)
{
	struct weston_surface *surface = view->surface;

	if (surface->committed == black_surface_committed) {
		if (fs_view)
			*fs_view = surface->committed_private;
		return true;
	}
	return false;
}

static void
shell_ensure_fullscreen_black_view(struct shell_surface *shsurf)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct weston_compositor *ec = surface->compositor;
	struct weston_output *output = shsurf->fullscreen_output;
	struct weston_curtain_params curtain_params = {
		.r = 0.0, .g = 0.0, .b = 0.0, .a = 1.0,
		.x = output->x, .y = output->y,
		.width = output->width, .height = output->height,
		.surface_committed = black_surface_committed,
		.get_label = black_surface_get_label,
		.surface_private = shsurf->view,
		.capture_input = true,
	};
	struct weston_view *view;

	assert(weston_desktop_surface_get_fullscreen(shsurf->desktop_surface));

	if (!shsurf->fullscreen.black_view) {
		shsurf->fullscreen.black_view =
			weston_shell_utils_curtain_create(ec, &curtain_params);
	}
	view = shsurf->fullscreen.black_view->view;

	weston_view_set_output(view, output);
	view->is_mapped = true;

	weston_layer_entry_remove(&view->layer_link);
	weston_layer_entry_insert(&shsurf->view->layer_link, &view->layer_link);
	weston_view_geometry_dirty(view);
	weston_surface_damage(surface);

	shsurf->state.lowered = false;
}

/* Create black surface and append it to the associated fullscreen surface.
 * Handle size dismatch and positioning according to the method. */
static void
shell_configure_fullscreen(struct shell_surface *shsurf)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	int32_t surf_x, surf_y, surf_width, surf_height;

	/* Reverse the effect of lower_fullscreen_layer() */
	weston_layer_entry_remove(&shsurf->view->layer_link);
	weston_layer_entry_insert(&shsurf->shell->fullscreen_layer.view_list,
				  &shsurf->view->layer_link);

	if (!shsurf->fullscreen_output) {
		/* If there is no output, there's not much we can do.
		 * Position the window somewhere, whatever. */
		weston_view_set_position(shsurf->view, 0, 0);
		return;
	}

	shell_ensure_fullscreen_black_view(shsurf);

	weston_shell_utils_subsurfaces_boundingbox(surface, &surf_x, &surf_y,
						   &surf_width, &surf_height);

	if (weston_surface_has_content(surface))
		weston_shell_utils_center_on_output(shsurf->view, shsurf->fullscreen_output);
}

static void
shell_map_fullscreen(struct shell_surface *shsurf)
{
	shell_configure_fullscreen(shsurf);
}

static void
desktop_shell_destroy_seat(struct shell_seat *shseat)
{

	wl_list_remove(&shseat->keyboard_focus_listener.link);
	wl_list_remove(&shseat->caps_changed_listener.link);
	wl_list_remove(&shseat->pointer_focus_listener.link);
	wl_list_remove(&shseat->seat_destroy_listener.link);
	wl_list_remove(&shseat->tablet_tool_added_listener.link);

	wl_list_remove(&shseat->link);
	free(shseat);
}

static void
destroy_tablet_tool_listener(struct wl_listener *listener, void *data)
{
	struct tablet_tool_listener *tool_listener =
		container_of(listener, struct tablet_tool_listener, removed_listener);

	wl_list_remove(&tool_listener->removed_listener.link);
	wl_list_remove(&tool_listener->base.link);
	free(tool_listener);
}

static void
handle_tablet_tool_added(struct wl_listener *listener, void *data)
{
	struct weston_tablet_tool *tool = data;
	struct tablet_tool_listener *tool_listener;

	tool_listener = malloc(sizeof *tool_listener);
	if (!tool_listener) {
		weston_log("no memory to allocate to shell seat tablet listener\n");
		return;
	}

	tool_listener->removed_listener.notify = destroy_tablet_tool_listener;
	wl_signal_add(&tool->removed_signal,
		      &tool_listener->removed_listener);

	tool_listener->base.notify = handle_tablet_tool_focus;
	wl_signal_add(&tool->focus_signal, &tool_listener->base);
}

static void
destroy_shell_seat(struct wl_listener *listener, void *data)
{
	struct shell_seat *shseat =
		container_of(listener,
			     struct shell_seat, seat_destroy_listener);

	desktop_shell_destroy_seat(shseat);
}

static void
shell_seat_caps_changed(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer;
	struct shell_seat *seat;

	seat = container_of(listener, struct shell_seat, caps_changed_listener);
	pointer = weston_seat_get_pointer(seat->seat);

	if (pointer &&
	    wl_list_empty(&seat->pointer_focus_listener.link)) {
		wl_signal_add(&pointer->focus_signal,
			      &seat->pointer_focus_listener);
	} else if (!pointer) {
		wl_list_remove(&seat->pointer_focus_listener.link);
		wl_list_init(&seat->pointer_focus_listener.link);
	}
}

static struct shell_seat *
create_shell_seat(struct desktop_shell *shell, struct weston_seat *seat)
{
	struct shell_seat *shseat;
	struct weston_tablet_tool *tool;

	shseat = calloc(1, sizeof *shseat);
	if (!shseat) {
		weston_log("no memory to allocate shell seat\n");
		return NULL;
	}

	shseat->seat = seat;

	shseat->seat_destroy_listener.notify = destroy_shell_seat;
	wl_signal_add(&seat->destroy_signal,
	              &shseat->seat_destroy_listener);

	wl_list_init(&shseat->keyboard_focus_listener.link);

	shseat->pointer_focus_listener.notify = handle_pointer_focus;
	wl_list_init(&shseat->pointer_focus_listener.link);

	shseat->tablet_tool_added_listener.notify = handle_tablet_tool_added;
	wl_list_init(&shseat->tablet_tool_added_listener.link);

	wl_list_for_each(tool, &seat->tablet_tool_list, link) {
		struct tablet_tool_listener *listener = malloc(sizeof *listener);

		if (!listener) {
			weston_log("no memory to allocate to shell seat tablet listener\n");
			break;
		}

		listener->removed_listener.notify = destroy_tablet_tool_listener;
		wl_signal_add(&tool->removed_signal,
			      &listener->removed_listener);

		listener->base.notify = handle_tablet_tool_focus;
		wl_signal_add(&tool->focus_signal, &listener->base);
	}

	shseat->caps_changed_listener.notify = shell_seat_caps_changed;
	wl_signal_add(&seat->updated_caps_signal,
		      &shseat->caps_changed_listener);
	shell_seat_caps_changed(&shseat->caps_changed_listener, NULL);

	wl_list_insert(&shell->seat_list, &shseat->link);

	return shseat;
}

static struct shell_seat *
get_shell_seat(struct weston_seat *seat)
{
	struct wl_listener *listener;

	if (!seat)
		return NULL;

	listener = wl_signal_get(&seat->destroy_signal, destroy_shell_seat);
	if (!listener)
		return NULL;

	return container_of(listener,
			    struct shell_seat, seat_destroy_listener);
}

static void
fade_out_done_idle_cb(void *data)
{
	struct shell_surface *shsurf = data;
	desktop_shell_destroy_surface(shsurf);
}

static void
fade_out_done(struct weston_view_animation *animation, void *data)
{
	struct shell_surface *shsurf = data;
	struct wl_event_loop *loop;

	loop = wl_display_get_event_loop(shsurf->shell->compositor->wl_display);

	if (weston_view_is_mapped(shsurf->wview_anim_fade)) {
		weston_view_unmap(shsurf->wview_anim_fade);
		wl_event_loop_add_idle(loop, fade_out_done_idle_cb, shsurf);
	}
}

struct shell_surface *
get_shell_surface(struct weston_surface *surface)
{
	if (weston_surface_is_desktop_surface(surface)) {
		struct weston_desktop_surface *desktop_surface =
			weston_surface_get_desktop_surface(surface);
		return weston_desktop_surface_get_user_data(desktop_surface);
	}
	return NULL;
}

/*
 * libweston-desktop
 */

static void
desktop_surface_added(struct weston_desktop_surface *desktop_surface,
		      void *shell)
{
	struct weston_desktop_client *client =
		weston_desktop_surface_get_client(desktop_surface);
	struct wl_client *wl_client =
		weston_desktop_client_get_client(client);
	struct weston_view *view;
	struct shell_surface *shsurf;
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(desktop_surface);

	view = weston_desktop_surface_create_view(desktop_surface);
	if (!view)
		return;

	shsurf = calloc(1, sizeof *shsurf);
	if (!shsurf) {
		if (wl_client)
			wl_client_post_no_memory(wl_client);
		else
			weston_log("no memory to allocate shell surface\n");
		return;
	}

	weston_surface_set_label_func(surface, weston_shell_utils_surface_get_label);

	shsurf->shell = (struct desktop_shell *) shell;
	shsurf->unresponsive = 0;
	shsurf->saved_position_valid = false;
	shsurf->saved_rotation_valid = false;
	shsurf->desktop_surface = desktop_surface;
	shsurf->view = view;
	shsurf->fullscreen.black_view = NULL;
	wl_list_init(&shsurf->fullscreen.transform.link);

	shell_surface_set_output(
		shsurf, weston_shell_utils_get_default_output(shsurf->shell->compositor));

	wl_signal_init(&shsurf->destroy_signal);

	/* empty when not in use */
	wl_list_init(&shsurf->rotation.transform.link);
	weston_matrix_init(&shsurf->rotation.rotation);

	/*
	 * initialize list as well as link. The latter allows to use
	 * wl_list_remove() even when this surface is not in another list.
	 */
	wl_list_init(&shsurf->children_list);
	wl_list_init(&shsurf->children_link);

	weston_desktop_surface_set_user_data(desktop_surface, shsurf);
}

static void
desktop_surface_removed(struct weston_desktop_surface *desktop_surface,
			void *shell)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(desktop_surface);
	struct weston_seat *seat;

	if (!shsurf)
		return;

	wl_list_for_each(seat, &shsurf->shell->compositor->seat_list, link) {
		struct shell_seat *shseat = get_shell_seat(seat);
		/* activate() controls the focused surface activation and
		 * removal of a surface requires invalidating the
		 * focused_surface to avoid activate() use a stale (and just
		 * removed) surface when attempting to de-activate it. It will
		 * also update the focused_surface once it has a chance to run.
		 */
		if (shseat && surface == shseat->focused_surface)
			shseat->focused_surface = NULL;
	}

	if (shsurf->fullscreen.black_view) {
		weston_shell_utils_curtain_destroy(shsurf->fullscreen.black_view);
		shsurf->fullscreen.black_view = NULL;
	}

	weston_surface_set_label_func(surface, NULL);
	weston_desktop_surface_set_user_data(shsurf->desktop_surface, NULL);
	shsurf->desktop_surface = NULL;

	if (weston_surface_is_mapped(surface) &&
	    shsurf->shell->win_close_animation_type == ANIMATION_FADE) {

		if (shsurf->shell->compositor->state == WESTON_COMPOSITOR_ACTIVE &&
		    shsurf->view->output->power_state == WESTON_OUTPUT_POWER_NORMAL) {
			pixman_region32_fini(&surface->pending.input);
			pixman_region32_init(&surface->pending.input);
			pixman_region32_fini(&surface->input);
			pixman_region32_init(&surface->input);

			/* its location might have changed, but also might've
			 * migrated to a different output, so re-compute  this
			 * as the animation requires having the same output as
			 * the view */
			weston_view_set_output(shsurf->wview_anim_fade,
					       shsurf->view->output);
			weston_view_set_position(shsurf->wview_anim_fade,
						 shsurf->view->geometry.pos_offset.x,
						 shsurf->view->geometry.pos_offset.y);

			weston_layer_entry_insert(&shsurf->view->layer_link,
						  &shsurf->wview_anim_fade->layer_link);

			/* unmap the "original" view */
			weston_view_unmap(shsurf->view);
			weston_fade_run(shsurf->wview_anim_fade, 1.0, 0.0, 300.0,
					fade_out_done, shsurf);

			return;
		}
	}

	desktop_shell_destroy_surface(shsurf);
}

static void
set_maximized_position(struct desktop_shell *shell,
		       struct shell_surface *shsurf)
{
	pixman_rectangle32_t area;
	struct weston_geometry geometry;

	get_output_work_area(shell, shsurf->output, &area);
	geometry = weston_desktop_surface_get_geometry(shsurf->desktop_surface);

	weston_view_set_position(shsurf->view,
				 area.x - geometry.x,
				 area.y - geometry.y);
}

static void
set_position_from_xwayland(struct shell_surface *shsurf)
{
	struct weston_geometry geometry;
	float x;
	float y;

	assert(shsurf->xwayland.is_set);

	geometry = weston_desktop_surface_get_geometry(shsurf->desktop_surface);
	x = shsurf->xwayland.x - geometry.x;
	y = shsurf->xwayland.y - geometry.y;

	weston_view_set_position(shsurf->view, x, y);

#ifdef WM_DEBUG
	weston_log("%s: XWM %d, %d; geometry %d, %d; view %f, %f\n",
		   __func__, shsurf->xwayland.x, shsurf->xwayland.y,
		   geometry.x, geometry.y, x, y);
#endif
}

static void
map(struct desktop_shell *shell, struct shell_surface *shsurf)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct weston_compositor *compositor = shell->compositor;
	struct weston_seat *seat;

	/* initial positioning, see also configure() */
	if (shsurf->state.fullscreen) {
		weston_shell_utils_center_on_output(shsurf->view,
						    shsurf->fullscreen_output);
		shell_map_fullscreen(shsurf);
	} else if (shsurf->state.maximized) {
		set_maximized_position(shell, shsurf);
	} else if (shsurf->xwayland.is_set) {
		set_position_from_xwayland(shsurf);
	} else {
		weston_view_set_initial_position(shsurf->view, shell);
	}

	/* Surface stacking order, see also activate(). */
	shell_surface_update_layer(shsurf);

	weston_view_update_transform(shsurf->view);
	shsurf->view->is_mapped = true;
	if (shsurf->state.maximized) {
		surface->output = shsurf->output;
		weston_view_set_output(shsurf->view, shsurf->output);
	}

	weston_surface_map(surface);

	if (!shell->locked) {
		wl_list_for_each(seat, &compositor->seat_list, link)
			activate(shell, shsurf->view, seat,
				 WESTON_ACTIVATE_FLAG_CONFIGURE);
	}

	if (!shsurf->state.fullscreen && !shsurf->state.maximized) {
		switch (shell->win_animation_type) {
		case ANIMATION_FADE:
			weston_fade_run(shsurf->view, 0.0, 1.0, 300.0, NULL, NULL);
			break;
		case ANIMATION_ZOOM:
			weston_zoom_run(shsurf->view, 0.5, 1.0, NULL, NULL);
			break;
		case ANIMATION_NONE:
		default:
			break;
		}
	}
}

static void
desktop_surface_committed(struct weston_desktop_surface *desktop_surface,
			  int32_t sx, int32_t sy, void *data)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(desktop_surface);
	struct weston_view *view = shsurf->view;
	struct desktop_shell *shell = data;
	bool was_fullscreen;
	bool was_maximized;

	if (!weston_surface_has_content(surface) &&
	    weston_surface_is_unmapping(surface) &&
	    shsurf->state.fullscreen) {
		unset_fullscreen(shsurf);
		return;
	}

	if (surface->width == 0) {
		return;
	}

	was_fullscreen = shsurf->state.fullscreen;
	was_maximized = shsurf->state.maximized;

	shsurf->state.fullscreen =
		weston_desktop_surface_get_fullscreen(desktop_surface);
	shsurf->state.maximized =
		weston_desktop_surface_get_maximized(desktop_surface);

	if (!weston_surface_is_mapped(surface)) {
		map(shell, shsurf);
		/* as we need to survive the weston_surface destruction we'll
		 * need to take another reference */
		if (shsurf->shell->win_close_animation_type == ANIMATION_FADE) {
			shsurf->wsurface_anim_fade =
				weston_surface_ref(surface);
			shsurf->wview_anim_fade =
				shell_fade_create_fade_out_view(shsurf, surface);
		}

		return;
	}

	if (sx == 0 && sy == 0 &&
	    shsurf->last_width == surface->width &&
	    shsurf->last_height == surface->height &&
	    was_fullscreen == shsurf->state.fullscreen &&
	    was_maximized == shsurf->state.maximized)
	    return;

	if (was_fullscreen)
		unset_fullscreen(shsurf);
	if (was_maximized)
		unset_maximized(shsurf);

	if ((shsurf->state.fullscreen || shsurf->state.maximized) &&
	    !shsurf->saved_position_valid) {
		shsurf->saved_pos.c = shsurf->view->geometry.pos_offset;
		shsurf->saved_position_valid = true;

		if (!wl_list_empty(&shsurf->rotation.transform.link)) {
			wl_list_remove(&shsurf->rotation.transform.link);
			wl_list_init(&shsurf->rotation.transform.link);
			weston_view_geometry_dirty(shsurf->view);
			shsurf->saved_rotation_valid = true;
		}
	}

	weston_view_update_transform(shsurf->view);

	if (shsurf->state.fullscreen) {
		shell_configure_fullscreen(shsurf);
	} else if (shsurf->state.maximized) {
		set_maximized_position(shell, shsurf);
		surface->output = shsurf->output;
	} else {
		struct weston_coord_surface from_s, to_s;
		struct weston_coord_global to_g, from_g;
		struct weston_coord_global offset, pos;

		from_s = weston_coord_surface(0, 0, view->surface);

		if (shsurf->resize_edges) {
			sx = 0;
			sy = 0;
		}

		if (shsurf->resize_edges & WESTON_DESKTOP_SURFACE_EDGE_LEFT)
			sx = shsurf->last_width - surface->width;
		if (shsurf->resize_edges & WESTON_DESKTOP_SURFACE_EDGE_TOP)
			sy = shsurf->last_height - surface->height;

		to_s = weston_coord_surface(sx, sy, view->surface);

		from_g = weston_coord_surface_to_global(view, from_s);
		to_g = weston_coord_surface_to_global(view, to_s);

		offset.c = weston_coord_sub(to_g.c, from_g.c);
		pos.c = weston_coord_add(view->geometry.pos_offset, offset.c);

		weston_view_set_position(shsurf->view, pos.c.x, pos.c.y);
	}

	shsurf->last_width = surface->width;
	shsurf->last_height = surface->height;

	/* XXX: would a fullscreen surface need the same handling? */
	if (surface->output) {
		wl_list_for_each(view, &surface->views, surface_link)
			weston_view_update_transform(view);
	}
}

static void
get_maximized_size(struct shell_surface *shsurf, int32_t *width, int32_t *height)
{
	struct desktop_shell *shell;
	pixman_rectangle32_t area;

	shell = shell_surface_get_shell(shsurf);
	get_output_work_area(shell, shsurf->output, &area);

	*width = area.width;
	*height = area.height;
}

static void
set_fullscreen(struct shell_surface *shsurf, bool fullscreen,
	       struct weston_output *output)
{
	struct weston_desktop_surface *desktop_surface = shsurf->desktop_surface;
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	int32_t width = 0, height = 0;

	if (fullscreen) {
		/* handle clients launching in fullscreen */
		if (output == NULL && !weston_surface_is_mapped(surface)) {
			/* Set the output to the one that has focus currently. */
			output = weston_shell_utils_get_focused_output(surface->compositor);
		}

		shell_surface_set_output(shsurf, output);
		shsurf->fullscreen_output = shsurf->output;

		if (shsurf->output) {
			width = shsurf->output->width;
			height = shsurf->output->height;
		}
		weston_desktop_surface_set_orientation(shsurf->desktop_surface,
							WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE);
	} else if (weston_desktop_surface_get_maximized(desktop_surface) ||
		   weston_desktop_surface_get_pending_maximized(desktop_surface)) {
		get_maximized_size(shsurf, &width, &height);
	}
	weston_desktop_surface_set_fullscreen(desktop_surface, fullscreen);
	weston_desktop_surface_set_size(desktop_surface, width, height);
}

static void
desktop_surface_move(struct weston_desktop_surface *desktop_surface,
		     struct weston_seat *seat, uint32_t serial, void *shell)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_touch *touch = weston_seat_get_touch(seat);
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct wl_resource *resource = surface->resource;
	struct weston_surface *focus;

	if (pointer &&
	    pointer->focus &&
	    pointer->button_count > 0 &&
	    pointer->grab_serial == serial) {
		focus = weston_surface_get_main_surface(pointer->focus->surface);
		if ((focus == surface) &&
		    (surface_move(shsurf, pointer, true) < 0))
			wl_resource_post_no_memory(resource);
	} else if (touch &&
		   touch->focus &&
		   touch->grab_serial == serial) {
		focus = weston_surface_get_main_surface(touch->focus->surface);
		if ((focus == surface) &&
		    (surface_touch_move(shsurf, touch) < 0))
			wl_resource_post_no_memory(resource);
	} else if (!wl_list_empty(&seat->tablet_tool_list)) {
		struct weston_tablet_tool *tool;

		wl_list_for_each(tool, &seat->tablet_tool_list, link) {
			if (tool->focus && tool->grab_serial == serial) {
				focus = weston_surface_get_main_surface(
							  tool->focus->surface);
				if (focus == surface &&
				    surface_tablet_tool_move(shsurf, tool) < 0)
					wl_resource_post_no_memory(resource);
			}
		}
	}
}

static void
desktop_surface_resize(struct weston_desktop_surface *desktop_surface,
		       struct weston_seat *seat, uint32_t serial,
		       enum weston_desktop_surface_edge edges, void *shell)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct wl_resource *resource = surface->resource;
	struct weston_surface *focus;

	if (!pointer ||
	    pointer->button_count == 0 ||
	    pointer->grab_serial != serial ||
	    pointer->focus == NULL)
		return;

	focus = weston_surface_get_main_surface(pointer->focus->surface);
	if (focus != surface)
		return;

	if (surface_resize(shsurf, pointer, edges) < 0)
		wl_resource_post_no_memory(resource);
}

static void
desktop_surface_set_parent(struct weston_desktop_surface *desktop_surface,
			   struct weston_desktop_surface *parent,
			   void *shell)
{
	struct shell_surface *shsurf_parent;
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);

	/* unlink any potential child */
	wl_list_remove(&shsurf->children_link);

	if (parent) {
		shsurf_parent = weston_desktop_surface_get_user_data(parent);
		wl_list_insert(shsurf_parent->children_list.prev,
			       &shsurf->children_link);
	} else {
		wl_list_init(&shsurf->children_link);
	}
}

static void
desktop_surface_fullscreen_requested(struct weston_desktop_surface *desktop_surface,
				     bool fullscreen,
				     struct weston_output *output, void *shell)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);

	set_fullscreen(shsurf, fullscreen, output);
}

static void
set_maximized(struct shell_surface *shsurf, bool maximized)
{
	struct weston_desktop_surface *desktop_surface = shsurf->desktop_surface;
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	int32_t width = 0, height = 0;

	if (weston_desktop_surface_get_fullscreen(desktop_surface))
		return;

	if (maximized) {
		struct weston_output *output;

		if (!weston_surface_is_mapped(surface))
			output = weston_shell_utils_get_focused_output(surface->compositor);
		else
			output = surface->output;

		shell_surface_set_output(shsurf, output);

		get_maximized_size(shsurf, &width, &height);

		weston_desktop_surface_set_orientation(shsurf->desktop_surface,
							WESTON_TOP_LEVEL_TILED_ORIENTATION_NONE);
	}
	weston_desktop_surface_set_maximized(desktop_surface, maximized);
	weston_desktop_surface_set_size(desktop_surface, width, height);
}

static void
desktop_surface_maximized_requested(struct weston_desktop_surface *desktop_surface,
				    bool maximized, void *shell)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);

	set_maximized(shsurf, maximized);
}

static void
desktop_surface_minimized_requested(struct weston_desktop_surface *desktop_surface,
				    void *shell)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(desktop_surface);

	 /* apply compositor's own minimization logic (hide) */
	set_minimized(surface);
}

static void
set_busy_cursor(struct shell_surface *shsurf, struct weston_pointer *pointer)
{
	struct shell_grab *grab;

	if (pointer->grab->interface == &busy_cursor_grab_interface)
		return;

	grab = malloc(sizeof *grab);
	if (!grab)
		return;

	shell_grab_start(grab, &busy_cursor_grab_interface, shsurf, pointer,
			 WESTON_DESKTOP_SHELL_CURSOR_BUSY);
	/* Mark the shsurf as ungrabbed so that button binding is able
	 * to move it. */
	shsurf->grabbed = 0;
}

static void
end_busy_cursor(struct weston_compositor *compositor,
		struct weston_desktop_client *desktop_client)
{
	struct shell_surface *shsurf;
	struct shell_grab *grab;
	struct weston_seat *seat;

	wl_list_for_each(seat, &compositor->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);
		struct weston_desktop_client *grab_client;

		if (!pointer)
			continue;

		if (pointer->grab->interface != &busy_cursor_grab_interface)
			continue;

		grab = (struct shell_grab *) pointer->grab;
		shsurf = grab->shsurf;
		if (!shsurf)
			continue;

		grab_client =
			weston_desktop_surface_get_client(shsurf->desktop_surface);
		if (grab_client  == desktop_client) {
			shell_grab_end(grab);
			free(grab);
		}
	}
}

static void
desktop_surface_set_unresponsive(struct weston_desktop_surface *desktop_surface,
				 void *user_data)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	bool *unresponsive = user_data;

	shsurf->unresponsive = *unresponsive;
}

static void
desktop_surface_ping_timeout(struct weston_desktop_client *desktop_client,
			     void *shell_)
{
	struct desktop_shell *shell = shell_;
	struct shell_surface *shsurf;
	struct weston_seat *seat;
	bool unresponsive = true;

	weston_desktop_client_for_each_surface(desktop_client,
					       desktop_surface_set_unresponsive,
					       &unresponsive);


	wl_list_for_each(seat, &shell->compositor->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);
		struct weston_desktop_client *grab_client;

		if (!pointer || !pointer->focus)
			continue;

		shsurf = get_shell_surface(pointer->focus->surface);
		if (!shsurf)
			continue;

		grab_client =
			weston_desktop_surface_get_client(shsurf->desktop_surface);
		if (grab_client == desktop_client)
			set_busy_cursor(shsurf, pointer);
	}
}

static void
desktop_surface_pong(struct weston_desktop_client *desktop_client,
		     void *shell_)
{
	struct desktop_shell *shell = shell_;
	bool unresponsive = false;

	weston_desktop_client_for_each_surface(desktop_client,
					       desktop_surface_set_unresponsive,
					       &unresponsive);
	end_busy_cursor(shell->compositor, desktop_client);
}

static void
desktop_surface_get_geometry(struct weston_desktop_surface *surface, struct weston_geometry *geometry) {
	struct shell_surface *shsurf = weston_desktop_surface_get_user_data(surface);
	if (shsurf) {
		geometry->x = shsurf->view->geometry.pos_offset.x;
		geometry->y = shsurf->view->geometry.pos_offset.y;
	}
}

static void
desktop_surface_set_xwayland_position(struct weston_desktop_surface *surface,
				      int32_t x, int32_t y, void *shell_)
{
	struct shell_surface *shsurf =
		weston_desktop_surface_get_user_data(surface);

	shsurf->xwayland.x = x;
	shsurf->xwayland.y = y;
	shsurf->xwayland.is_set = true;
}

static void
desktop_surface_get_position(struct weston_desktop_surface *surface,
			     int32_t *x, int32_t *y,
			     void *shell_)
{
	struct shell_surface *shsurf = weston_desktop_surface_get_user_data(surface);

	*x = shsurf->view->geometry.pos_offset.x;
	*y = shsurf->view->geometry.pos_offset.y;
}

static const struct weston_desktop_api shell_desktop_api = {
	.struct_size = sizeof(struct weston_desktop_api),
	.surface_added = desktop_surface_added,
	.surface_removed = desktop_surface_removed,
	.committed = desktop_surface_committed,
	.move = desktop_surface_move,
	.resize = desktop_surface_resize,
	.set_parent = desktop_surface_set_parent,
	.fullscreen_requested = desktop_surface_fullscreen_requested,
	.maximized_requested = desktop_surface_maximized_requested,
	.minimized_requested = desktop_surface_minimized_requested,
	.ping_timeout = desktop_surface_ping_timeout,
	.pong = desktop_surface_pong,
	.get_desktop_surface_root_geometry = desktop_surface_get_geometry,
	.set_xwayland_position = desktop_surface_set_xwayland_position,
	.get_position = desktop_surface_get_position,
};

/* ************************ *
 * end of libweston-desktop *
 * ************************ */
static void
configure_static_view(struct weston_view *ev, struct weston_layer *layer, int x, int y)
{
	struct weston_view *v, *next;

	if (!ev->output)
		return;

	wl_list_for_each_safe(v, next, &layer->view_list.link, layer_link.link) {
		if (v->output == ev->output && v != ev) {
			weston_view_unmap(v);
			v->surface->committed = NULL;
			weston_surface_set_label_func(v->surface, NULL);
		}
	}

	weston_view_set_position(ev, ev->output->x + x, ev->output->y + y);
	weston_surface_map(ev->surface);
	ev->is_mapped = true;

	if (wl_list_empty(&ev->layer_link.link)) {
		weston_layer_entry_insert(&layer->view_list, &ev->layer_link);
		weston_view_geometry_dirty(ev);
		weston_compositor_schedule_repaint(ev->surface->compositor);
	}
}


static struct shell_output *
find_shell_output_from_weston_output(struct desktop_shell *shell,
				     struct weston_output *output)
{
	struct shell_output *shell_output;

	wl_list_for_each(shell_output, &shell->output_list, link) {
		if (shell_output->output == output)
			return shell_output;
	}

	return NULL;
}

static int
background_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "background for output %s",
			(surface->output ? surface->output->name : "NULL"));
}

static void
background_committed(struct weston_surface *es,
		     struct weston_coord_surface new_origin)
{
	struct desktop_shell *shell = es->committed_private;
	struct weston_view *view;

	view = container_of(es->views.next, struct weston_view, surface_link);

	configure_static_view(view, &shell->background_layer, 0, 0);
}

static void
handle_background_surface_destroy(struct wl_listener *listener, void *data)
{
	struct shell_output *output =
	    container_of(listener, struct shell_output, background_surface_listener);

	weston_log("background surface gone\n");
	wl_list_remove(&output->background_surface_listener.link);
	output->background_surface = NULL;
}

static void
desktop_shell_set_background(struct wl_client *client,
			     struct wl_resource *resource,
			     struct wl_resource *output_resource,
			     struct wl_resource *surface_resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct shell_output *sh_output;
	struct weston_view *view, *next;

	if (surface->committed) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface role already assigned");
		return;
	}

	wl_list_for_each_safe(view, next, &surface->views, surface_link)
		weston_view_destroy(view);
	view = weston_view_create(surface);

	surface->committed = background_committed;
	surface->committed_private = shell;
	weston_surface_set_label_func(surface, background_get_label);
	surface->output = weston_head_from_resource(output_resource)->output;
	weston_view_set_output(view, surface->output);

	sh_output = find_shell_output_from_weston_output(shell, surface->output);
	if (sh_output->background_surface) {
		/* The output already has a background, tell our helper
		 * there is no need for another one. */
		weston_desktop_shell_send_configure(resource, 0,
						    surface_resource,
						    0, 0);
	} else {
		weston_desktop_shell_send_configure(resource, 0,
						    surface_resource,
						    surface->output->width,
						    surface->output->height);

		sh_output->background_surface = surface;

		sh_output->background_surface_listener.notify =
					handle_background_surface_destroy;
		wl_signal_add(&surface->destroy_signal,
			      &sh_output->background_surface_listener);
	}
}

static int
panel_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "panel for output %s",
			(surface->output ? surface->output->name : "NULL"));
}

static void
panel_committed(struct weston_surface *es,
		struct weston_coord_surface new_origin)
{
	struct desktop_shell *shell = es->committed_private;
	struct weston_view *view;
	int width, height;
	int x = 0, y = 0;

	view = container_of(es->views.next, struct weston_view, surface_link);

	/* XXX delete me eventually - it would be better if we didn't get here
	 * with a dirty transform at all, but for now just make sure the
	 * transform is updated here. */
	weston_view_update_transform(view);

	get_panel_size(shell, view, &width, &height);
	switch (shell->panel_position) {
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP:
		break;
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM:
		y = view->output->height - height;
		break;
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT:
		break;
	case WESTON_DESKTOP_SHELL_PANEL_POSITION_RIGHT:
		x = view->output->width - width;
		break;
	}

	configure_static_view(view, &shell->panel_layer, x, y);
}

static void
handle_panel_surface_destroy(struct wl_listener *listener, void *data)
{
	struct shell_output *output =
	    container_of(listener, struct shell_output, panel_surface_listener);

	weston_log("panel surface gone\n");
	wl_list_remove(&output->panel_surface_listener.link);
	output->panel_surface = NULL;
}


static void
desktop_shell_set_panel(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *output_resource,
			struct wl_resource *surface_resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view, *next;
	struct shell_output *sh_output;

	if (surface->committed) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface role already assigned");
		return;
	}

	wl_list_for_each_safe(view, next, &surface->views, surface_link)
		weston_view_destroy(view);
	view = weston_view_create(surface);

	surface->committed = panel_committed;
	surface->committed_private = shell;
	weston_surface_set_label_func(surface, panel_get_label);
	surface->output = weston_head_from_resource(output_resource)->output;
	weston_view_set_output(view, surface->output);

	sh_output = find_shell_output_from_weston_output(shell, surface->output);
	if (sh_output->panel_surface) {
		/* The output already has a panel, tell our helper
		 * there is no need for another one. */
		weston_desktop_shell_send_configure(resource, 0,
						    surface_resource,
						    0, 0);
	} else {
		weston_desktop_shell_send_configure(resource, 0,
						    surface_resource,
						    surface->output->width,
						    surface->output->height);

		sh_output->panel_surface = surface;

		sh_output->panel_surface_listener.notify = handle_panel_surface_destroy;
		wl_signal_add(&surface->destroy_signal, &sh_output->panel_surface_listener);
	}
}

static int
lock_surface_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "lock window");
}

static void
lock_surface_committed(struct weston_surface *surface,
		       struct weston_coord_surface new_origin)
{
	struct desktop_shell *shell = surface->committed_private;
	struct weston_view *view;

	view = container_of(surface->views.next, struct weston_view, surface_link);

	if (surface->width == 0)
		return;

	weston_shell_utils_center_on_output(view,
		weston_shell_utils_get_default_output(shell->compositor));

	if (!weston_surface_is_mapped(surface)) {
		weston_layer_entry_insert(&shell->lock_layer.view_list,
					  &view->layer_link);
		weston_view_update_transform(view);
		weston_surface_map(surface);
		view->is_mapped = true;
		shell_fade(shell, FADE_IN);
	}
}

static void
handle_lock_surface_destroy(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
	    container_of(listener, struct desktop_shell, lock_surface_listener);

	weston_log("lock surface gone\n");
	shell->lock_surface = NULL;
}

static void
desktop_shell_set_lock_surface(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *surface_resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);

	shell->prepare_event_sent = false;

	if (!shell->locked)
		return;

	shell->lock_surface = surface;

	shell->lock_surface_listener.notify = handle_lock_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &shell->lock_surface_listener);

	weston_view_create(surface);
	surface->committed = lock_surface_committed;
	surface->committed_private = shell;
	weston_surface_set_label_func(surface, lock_surface_get_label);
}

static void
resume_desktop(struct desktop_shell *shell)
{
	struct workspace *ws = get_current_workspace(shell);

	weston_layer_unset_position(&shell->lock_layer);

	if (shell->showing_input_panels)
		weston_layer_set_position(&shell->input_panel_layer,
					  WESTON_LAYER_POSITION_TOP_UI);
	weston_layer_set_position(&shell->fullscreen_layer,
				  WESTON_LAYER_POSITION_FULLSCREEN);
	weston_layer_set_position(&shell->panel_layer,
				  WESTON_LAYER_POSITION_UI);
	weston_layer_set_position(&ws->layer, WESTON_LAYER_POSITION_NORMAL);

	restore_focus_state(shell, get_current_workspace(shell));

	shell->locked = false;
	shell_fade(shell, FADE_IN);
	weston_compositor_damage_all(shell->compositor);
}

static void
desktop_shell_unlock(struct wl_client *client,
		     struct wl_resource *resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);

	shell->prepare_event_sent = false;

	if (shell->locked)
		resume_desktop(shell);
}

static void
desktop_shell_set_grab_surface(struct wl_client *client,
			       struct wl_resource *resource,
			       struct wl_resource *surface_resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);

	shell->grab_surface = wl_resource_get_user_data(surface_resource);
	weston_view_create(shell->grab_surface);
}

static void
desktop_shell_desktop_ready(struct wl_client *client,
			    struct wl_resource *resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);

	shell_fade_startup(shell);
}

static void
desktop_shell_set_panel_position(struct wl_client *client,
				 struct wl_resource *resource,
				 uint32_t position)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);

	if (position != WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP &&
	    position != WESTON_DESKTOP_SHELL_PANEL_POSITION_BOTTOM &&
	    position != WESTON_DESKTOP_SHELL_PANEL_POSITION_LEFT &&
	    position != WESTON_DESKTOP_SHELL_PANEL_POSITION_RIGHT) {
		wl_resource_post_error(resource,
				       WESTON_DESKTOP_SHELL_ERROR_INVALID_ARGUMENT,
				       "bad position argument");
		return;
	}

	shell->panel_position = position;
}

static const struct weston_desktop_shell_interface desktop_shell_implementation = {
	desktop_shell_set_background,
	desktop_shell_set_panel,
	desktop_shell_set_lock_surface,
	desktop_shell_unlock,
	desktop_shell_set_grab_surface,
	desktop_shell_desktop_ready,
	desktop_shell_set_panel_position
};

static void
move_binding(struct weston_pointer *pointer, const struct timespec *time,
	     uint32_t button, void *data)
{
	struct weston_surface *focus;
	struct weston_surface *surface;
	struct shell_surface *shsurf;

	if (pointer->focus == NULL)
		return;

	focus = pointer->focus->surface;

	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return;

	surface_move(shsurf, pointer, false);
}

static void
maximize_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t button, void *data)
{
	struct weston_surface *focus = keyboard->focus;
	struct weston_surface *surface;
	struct shell_surface *shsurf;

	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL)
		return;

	set_maximized(shsurf, !weston_desktop_surface_get_maximized(shsurf->desktop_surface));
}

static void
fullscreen_binding(struct weston_keyboard *keyboard,
		   const struct timespec *time, uint32_t button, void *data)
{
	struct weston_surface *focus = keyboard->focus;
	struct weston_surface *surface;
	struct shell_surface *shsurf;
	bool fullscreen;

	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL)
		return;

	fullscreen =
		weston_desktop_surface_get_fullscreen(shsurf->desktop_surface);

	set_fullscreen(shsurf, !fullscreen, NULL);
}

static void
set_tiled_orientation(struct weston_surface *focus,
		      enum weston_top_level_tiled_orientation orientation)
{
	struct weston_surface *surface;
	struct shell_surface *shsurf;
	int width, height;
	pixman_rectangle32_t area;
	struct weston_geometry geom;
	int x, y;

	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL)
		return;

	shsurf->orientation = orientation;
	get_maximized_size(shsurf, &width, &height);
	get_output_work_area(shsurf->shell, shsurf->output, &area);
	geom = weston_desktop_surface_get_geometry(shsurf->desktop_surface);

	if (orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_LEFT ||
	    orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_RIGHT)
		width /= 2;
	else if (orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_TOP ||
		orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_BOTTOM)
		height /= 2;

	x = area.x - geom.x;
	y = area.y - geom.y;

	if (orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_RIGHT)
		x += width;
	else if (orientation & WESTON_TOP_LEVEL_TILED_ORIENTATION_BOTTOM)
		y += height;

	weston_view_set_position(shsurf->view, x, y);
	weston_desktop_surface_set_size(shsurf->desktop_surface, width, height);
	weston_desktop_surface_set_orientation(shsurf->desktop_surface, orientation);
	weston_compositor_schedule_repaint(surface->compositor);
}

static void
set_tiled_orientation_left(struct weston_keyboard *keyboard,
			   const struct timespec *time,
			   uint32_t button, void *data)
{
	set_tiled_orientation(keyboard->focus, WESTON_TOP_LEVEL_TILED_ORIENTATION_LEFT);

}

static void
set_tiled_orientation_right(struct weston_keyboard *keyboard,
			   const struct timespec *time,
			   uint32_t button, void *data)
{
	set_tiled_orientation(keyboard->focus, WESTON_TOP_LEVEL_TILED_ORIENTATION_RIGHT);
}

static void
set_tiled_orientation_up(struct weston_keyboard *keyboard,
			   const struct timespec *time,
			   uint32_t button, void *data)
{
	set_tiled_orientation(keyboard->focus, WESTON_TOP_LEVEL_TILED_ORIENTATION_TOP);
}

static void
set_tiled_orientation_down(struct weston_keyboard *keyboard,
			   const struct timespec *time,
			   uint32_t button, void *data)
{
	set_tiled_orientation(keyboard->focus, WESTON_TOP_LEVEL_TILED_ORIENTATION_BOTTOM);
}

static void
touch_move_binding(struct weston_touch *touch, const struct timespec *time, void *data)
{
	struct weston_surface *focus;
	struct weston_surface *surface;
	struct shell_surface *shsurf;

	if (touch->focus == NULL)
		return;

	focus = touch->focus->surface;
	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return;

	surface_touch_move(shsurf, touch);
}

static void
resize_binding(struct weston_pointer *pointer, const struct timespec *time,
	       uint32_t button, void *data)
{
	struct weston_surface *focus;
	struct weston_surface *surface;
	uint32_t edges = 0;
	int32_t x, y;
	struct shell_surface *shsurf;
	struct weston_coord_surface surf_pos;

	if (pointer->focus == NULL)
		return;

	focus = pointer->focus->surface;

	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (shsurf == NULL ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return;

	surf_pos = weston_coord_global_to_surface(shsurf->view, pointer->grab_pos);
	x = surf_pos.c.x;
	y = surf_pos.c.y;

	if (x < surface->width / 3)
		edges |= WESTON_DESKTOP_SURFACE_EDGE_LEFT;
	else if (x < 2 * surface->width / 3)
		edges |= 0;
	else
		edges |= WESTON_DESKTOP_SURFACE_EDGE_RIGHT;

	if (y < surface->height / 3)
		edges |= WESTON_DESKTOP_SURFACE_EDGE_TOP;
	else if (y < 2 * surface->height / 3)
		edges |= 0;
	else
		edges |= WESTON_DESKTOP_SURFACE_EDGE_BOTTOM;

	surface_resize(shsurf, pointer, edges);
}

static void
surface_opacity_binding(struct weston_pointer *pointer,
			const struct timespec *time,
			struct weston_pointer_axis_event *event,
			void *data)
{
	float step = 0.005;
	struct shell_surface *shsurf;
	struct weston_surface *focus = pointer->focus->surface;
	struct weston_surface *surface;

	/* XXX: broken for windows containing sub-surfaces */
	surface = weston_surface_get_main_surface(focus);
	if (surface == NULL)
		return;

	shsurf = get_shell_surface(surface);
	if (!shsurf)
		return;

	shsurf->view->alpha -= event->value * step;

	if (shsurf->view->alpha > 1.0)
		shsurf->view->alpha = 1.0;
	if (shsurf->view->alpha < step)
		shsurf->view->alpha = step;

	weston_view_geometry_dirty(shsurf->view);
	weston_surface_damage(surface);
}

static void
terminate_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		  uint32_t key, void *data)
{
	struct weston_compositor *compositor = data;

	weston_compositor_exit(compositor);
}

static void
rotate_grab_motion(struct weston_pointer_grab *grab,
		   const struct timespec *time,
		   struct weston_pointer_motion_event *event)
{
	struct rotate_grab *rotate =
		container_of(grab, struct rotate_grab, base.grab);
	struct weston_pointer *pointer = grab->pointer;
	struct shell_surface *shsurf = rotate->base.shsurf;
	struct weston_surface *surface;
	float cx, cy, dx, dy, cposx, cposy, dposx, dposy, r;

	weston_pointer_move(pointer, event);

	if (!shsurf)
		return;

	surface = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	cx = 0.5f * surface->width;
	cy = 0.5f * surface->height;

	dx = pointer->pos.c.x - rotate->center.x;
	dy = pointer->pos.c.y - rotate->center.y;
	r = sqrtf(dx * dx + dy * dy);

	wl_list_remove(&shsurf->rotation.transform.link);
	weston_view_geometry_dirty(shsurf->view);

	if (r > 20.0f) {
		struct weston_matrix *matrix =
			&shsurf->rotation.transform.matrix;

		weston_matrix_init(&rotate->rotation);
		weston_matrix_rotate_xy(&rotate->rotation, dx / r, dy / r);

		weston_matrix_init(matrix);
		weston_matrix_translate(matrix, -cx, -cy, 0.0f);
		weston_matrix_multiply(matrix, &shsurf->rotation.rotation);
		weston_matrix_multiply(matrix, &rotate->rotation);
		weston_matrix_translate(matrix, cx, cy, 0.0f);

		wl_list_insert(
			&shsurf->view->geometry.transformation_list,
			&shsurf->rotation.transform.link);
	} else {
		wl_list_init(&shsurf->rotation.transform.link);
		weston_matrix_init(&shsurf->rotation.rotation);
		weston_matrix_init(&rotate->rotation);
	}

	/* We need to adjust the position of the surface
	 * in case it was resized in a rotated state before */
	cposx = shsurf->view->geometry.pos_offset.x + cx;
	cposy = shsurf->view->geometry.pos_offset.y + cy;
	dposx = rotate->center.x - cposx;
	dposy = rotate->center.y - cposy;
	if (dposx != 0.0f || dposy != 0.0f) {
		weston_view_set_position(shsurf->view,
					 shsurf->view->geometry.pos_offset.x + dposx,
					 shsurf->view->geometry.pos_offset.y + dposy);
	}

	/* Repaint implies weston_view_update_transform(), which
	 * lazily applies the damage due to rotation update.
	 */
	weston_compositor_schedule_repaint(surface->compositor);
}

static void
rotate_grab_button(struct weston_pointer_grab *grab,
		   const struct timespec *time,
		   uint32_t button, uint32_t state_w)
{
	struct rotate_grab *rotate =
		container_of(grab, struct rotate_grab, base.grab);
	struct weston_pointer *pointer = grab->pointer;
	struct shell_surface *shsurf = rotate->base.shsurf;
	enum wl_pointer_button_state state = state_w;

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (shsurf)
			weston_matrix_multiply(&shsurf->rotation.rotation,
					       &rotate->rotation);
		shell_grab_end(&rotate->base);
		free(rotate);
	}
}

static void
rotate_grab_cancel(struct weston_pointer_grab *grab)
{
	struct rotate_grab *rotate =
		container_of(grab, struct rotate_grab, base.grab);

	shell_grab_end(&rotate->base);
	free(rotate);
}

static const struct weston_pointer_grab_interface rotate_grab_interface = {
	noop_grab_focus,
	rotate_grab_motion,
	rotate_grab_button,
	noop_grab_axis,
	noop_grab_axis_source,
	noop_grab_frame,
	rotate_grab_cancel,
};

static void
surface_rotate(struct shell_surface *shsurf, struct weston_pointer *pointer)
{
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct rotate_grab *rotate;
	struct weston_coord_surface center;
	struct weston_coord_global center_g;
	float dx, dy;
	float r;

	rotate = malloc(sizeof *rotate);
	if (!rotate)
		return;

	center = weston_coord_surface(surface->width * 0.5f,
				      surface->height * 0.5f,
				      shsurf->view->surface);
	center_g = weston_coord_surface_to_global(shsurf->view, center);

	rotate->center.x = center_g.c.x;
	rotate->center.y = center_g.c.y;

	dx = pointer->pos.c.x - rotate->center.x;
	dy = pointer->pos.c.y - rotate->center.y;
	r = sqrtf(dx * dx + dy * dy);
	if (r > 20.0f) {
		struct weston_matrix inverse;

		weston_matrix_init(&inverse);
		weston_matrix_rotate_xy(&inverse, dx / r, -dy / r);
		weston_matrix_multiply(&shsurf->rotation.rotation, &inverse);

		weston_matrix_init(&rotate->rotation);
		weston_matrix_rotate_xy(&rotate->rotation, dx / r, dy / r);
	} else {
		weston_matrix_init(&shsurf->rotation.rotation);
		weston_matrix_init(&rotate->rotation);
	}

	shell_grab_start(&rotate->base, &rotate_grab_interface, shsurf,
			 pointer, WESTON_DESKTOP_SHELL_CURSOR_ARROW);
}

static void
rotate_binding(struct weston_pointer *pointer, const struct timespec *time,
	       uint32_t button, void *data)
{
	struct weston_surface *focus;
	struct weston_surface *base_surface;
	struct shell_surface *surface;

	if (pointer->focus == NULL)
		return;

	focus = pointer->focus->surface;

	base_surface = weston_surface_get_main_surface(focus);
	if (base_surface == NULL)
		return;

	surface = get_shell_surface(base_surface);
	if (surface == NULL ||
	    weston_desktop_surface_get_fullscreen(surface->desktop_surface) ||
	    weston_desktop_surface_get_maximized(surface->desktop_surface))
		return;

	surface_rotate(surface, pointer);
}

/* Move all fullscreen layers down to the current workspace and hide their
 * black views. The surfaces' state is set to both fullscreen and lowered,
 * and this is reversed when such a surface is re-configured, see
 * shell_configure_fullscreen() and shell_ensure_fullscreen_black_view().
 *
 * lowering_output = NULL - Lower on all outputs, else only lower on the
 *                   specified output.
 *
 * This should be used when implementing shell-wide overlays, such as
 * the alt-tab switcher, which need to de-promote fullscreen layers. */
void
lower_fullscreen_layer(struct desktop_shell *shell,
		       struct weston_output *lowering_output)
{
	struct workspace *ws;
	struct weston_view *view, *prev;

	ws = get_current_workspace(shell);
	wl_list_for_each_reverse_safe(view, prev,
				      &shell->fullscreen_layer.view_list.link,
				      layer_link.link) {
		struct shell_surface *shsurf = get_shell_surface(view->surface);

		if (!shsurf)
			continue;

		/* Only lower surfaces which have lowering_output as their fullscreen
		 * output, unless a NULL output asks for lowering on all outputs.
		 */
		if (lowering_output && (shsurf->fullscreen_output != lowering_output))
			continue;

		/* We can have a non-fullscreen popup for a fullscreen surface
		 * in the fullscreen layer. */
		if (weston_desktop_surface_get_fullscreen(shsurf->desktop_surface)) {
			/* Hide the black view */
			weston_layer_entry_remove(&shsurf->fullscreen.black_view->view->layer_link);
			wl_list_init(&shsurf->fullscreen.black_view->view->layer_link.link);
			weston_view_damage_below(shsurf->fullscreen.black_view->view);
		}

		/* Lower the view to the workspace layer */
		weston_layer_entry_remove(&view->layer_link);
		weston_layer_entry_insert(&ws->layer.view_list, &view->layer_link);
		weston_view_damage_below(view);
		weston_surface_damage(view->surface);

		shsurf->state.lowered = true;
	}
}

static struct shell_surface *get_last_child(struct shell_surface *shsurf)
{
	struct shell_surface *shsurf_child;

	wl_list_for_each_reverse(shsurf_child, &shsurf->children_list, children_link) {
		if (weston_view_is_mapped(shsurf_child->view))
			return shsurf_child;
	}

	return NULL;
}

void
activate(struct desktop_shell *shell, struct weston_view *view,
	 struct weston_seat *seat, uint32_t flags)
{
	struct weston_surface *es = view->surface;
	struct weston_surface *main_surface;
	struct focus_state *state;
	struct workspace *ws;
	struct weston_surface *old_es;
	struct shell_surface *shsurf, *shsurf_child;
	struct shell_seat *shseat = get_shell_seat(seat);

	main_surface = weston_surface_get_main_surface(es);
	shsurf = get_shell_surface(main_surface);
	assert(shsurf);

	shsurf_child = get_last_child(shsurf);
	if (shsurf_child) {
		/* Activate last xdg child instead of parent. */
		activate(shell, shsurf_child->view, seat, flags);
		return;
	}

	/* Only demote fullscreen surfaces on the output of activated shsurf.
	 * Leave fullscreen surfaces on unrelated outputs alone. */
	if (shsurf->output)
		lower_fullscreen_layer(shell, shsurf->output);

	weston_view_activate_input(view, seat, flags);

	if (shseat && shseat->focused_surface &&
	    shseat->focused_surface != main_surface) {
		struct shell_surface *current_focus =
			get_shell_surface(shseat->focused_surface);
		assert(current_focus);
		shell_surface_deactivate(current_focus);
	}

	if (shseat && shseat->focused_surface != main_surface) {
		shell_surface_activate(shsurf);
		shseat->focused_surface = main_surface;
	}

	state = ensure_focus_state(shell, seat);
	if (state == NULL)
		return;

	old_es = state->keyboard_focus;
	focus_state_set_focus(state, es);

	if (weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) &&
	    flags & WESTON_ACTIVATE_FLAG_CONFIGURE)
		shell_configure_fullscreen(shsurf);

	/* Update the surface’s layer. This brings it to the top of the stacking
	 * order as appropriate. */
	shell_surface_update_layer(shsurf);

	if (shell->focus_animation_type != ANIMATION_NONE) {
		ws = get_current_workspace(shell);
		animate_focus_change(shell, ws, get_default_view(old_es), get_default_view(es));
	}
}

static void
activate_binding(struct weston_seat *seat,
		 struct desktop_shell *shell,
		 struct weston_view *focus_view,
		 uint32_t flags)
{
	struct weston_view *main_view;
	struct weston_surface *main_surface;

	if (!focus_view)
		return;

	if (is_black_surface_view(focus_view, &main_view))
		focus_view = main_view;

	main_surface = weston_surface_get_main_surface(focus_view->surface);
	if (!get_shell_surface(main_surface))
		return;

	activate(shell, focus_view, seat, flags);
}

static void
click_to_activate_binding(struct weston_pointer *pointer,
		          const struct timespec *time,
			  uint32_t button, void *data)
{
	if (pointer->grab != &pointer->default_grab)
		return;
	if (pointer->focus == NULL)
		return;

	activate_binding(pointer->seat, data, pointer->focus,
			 WESTON_ACTIVATE_FLAG_CLICKED |
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
}

static void
touch_to_activate_binding(struct weston_touch *touch,
			  const struct timespec *time,
			  void *data)
{
	if (touch->grab != &touch->default_grab)
		return;
	if (touch->focus == NULL)
		return;

	activate_binding(touch->seat, data, touch->focus,
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
}

static void
tablet_tool_activate_binding(struct weston_tablet_tool *tool,
			     uint32_t button, void *data)
{
	if (tool->grab != &tool->default_grab)
		return;
	if (tool->focus == NULL)
		return;

	activate_binding(tool->seat, data, tool->focus,
			WESTON_ACTIVATE_FLAG_CLICKED |
			WESTON_ACTIVATE_FLAG_CONFIGURE);
}

static void
unfocus_all_seats(struct desktop_shell *shell)
{
	struct weston_seat *seat, *next;

	wl_list_for_each_safe(seat, next, &shell->compositor->seat_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);

		if (!keyboard)
			continue;

		weston_keyboard_set_focus(keyboard, NULL);
	}
}

static void
lock(struct desktop_shell *shell)
{
	struct workspace *ws = get_current_workspace(shell);

	if (shell->locked) {
		weston_compositor_sleep(shell->compositor);
		return;
	}

	shell->locked = true;

	/* Hide all surfaces by removing the fullscreen, panel and
	 * toplevel layers.  This way nothing else can show or receive
	 * input events while we are locked. */

	weston_layer_unset_position(&shell->panel_layer);
	weston_layer_unset_position(&shell->fullscreen_layer);
	if (shell->showing_input_panels)
		weston_layer_unset_position(&shell->input_panel_layer);
	weston_layer_unset_position(&ws->layer);

	weston_layer_set_position(&shell->lock_layer,
				  WESTON_LAYER_POSITION_LOCK);

	weston_compositor_sleep(shell->compositor);

	/* Remove the keyboard focus on all seats. This will be
	 * restored to the workspace's saved state via
	 * restore_focus_state when the compositor is unlocked */
	unfocus_all_seats(shell);

	/* TODO: disable bindings that should not work while locked. */

	/* All this must be undone in resume_desktop(). */
}

static void
unlock(struct desktop_shell *shell)
{
	struct wl_resource *shell_resource;

	if (!shell->locked || shell->lock_surface) {
		shell_fade(shell, FADE_IN);
		return;
	}

	/* If desktop-shell client has gone away, unlock immediately. */
	if (!shell->child.desktop_shell) {
		resume_desktop(shell);
		return;
	}

	if (shell->prepare_event_sent)
		return;

	shell_resource = shell->child.desktop_shell;
	weston_desktop_shell_send_prepare_lock_surface(shell_resource);
	shell->prepare_event_sent = true;
}

static void
shell_fade_done_for_output(struct weston_view_animation *animation, void *data)
{
	struct shell_output *shell_output = data;
	struct desktop_shell *shell = shell_output->shell;

	shell_output->fade.animation = NULL;
	switch (shell_output->fade.type) {
	case FADE_IN:
		weston_shell_utils_curtain_destroy(shell_output->fade.curtain);
		shell_output->fade.curtain = NULL;
		break;
	case FADE_OUT:
		lock(shell);
		break;
	default:
		break;
	}
}

static int
fade_surface_get_label(struct weston_surface *surface,
		       char *buf, size_t len)
{
	struct shell_output *output = surface->committed_private;

	return snprintf(buf, len, "desktop shell fade surface for %s",
			output->output->name);
}

static struct weston_curtain *
shell_fade_create_view_for_output(struct desktop_shell *shell,
				  struct shell_output *shell_output)
{
	struct weston_compositor *compositor = shell->compositor;
	struct weston_output *output = shell_output->output;
	struct weston_curtain_params curtain_params = {
		.r = 0.0, .g = 0.0, .b = 0.0, .a = 1.0,
		.x = output->x, .y = output->y,
		.width = output->width, .height = output->height,
		.surface_committed = black_surface_committed,
		.get_label = fade_surface_get_label,
		.surface_private = shell_output,
		.capture_input = false,
	};
	struct weston_curtain *curtain;

	curtain = weston_shell_utils_curtain_create(compositor, &curtain_params);
	assert(curtain);

	weston_view_set_output(curtain->view, output);
	curtain->view->is_mapped = true;

	weston_layer_entry_insert(&compositor->fade_layer.view_list,
				  &curtain->view->layer_link);
	weston_view_geometry_dirty(curtain->view);
	weston_surface_damage(curtain->view->surface);

	return curtain;
}

static struct weston_view *
shell_fade_create_fade_out_view(struct shell_surface *shsurf,
				struct weston_surface *surface)
{
	struct weston_view *view;
	struct weston_output *woutput;

	view = weston_view_create(surface);
	if (!view)
		return NULL;

	woutput = weston_shell_utils_get_focused_output(surface->compositor);
	/* set the initial position and output just in case we happen to not
	 * move it around and just destroy it */
	weston_view_set_output(view, woutput);
	weston_view_set_position(view,
				 shsurf->view->geometry.pos_offset.x,
				 shsurf->view->geometry.pos_offset.y);
	view->is_mapped = true;

	return view;
}

static void
shell_fade(struct desktop_shell *shell, enum fade_type type)
{
	float tint;
	struct shell_output *shell_output;

	switch (type) {
	case FADE_IN:
		tint = 0.0;
		break;
	case FADE_OUT:
		tint = 1.0;
		break;
	default:
		weston_log("shell: invalid fade type\n");
		return;
	}

	/* Create a separate fade surface for each output */
	wl_list_for_each(shell_output, &shell->output_list, link) {
		shell_output->fade.type = type;

		if (shell_output->fade.curtain == NULL) {
			shell_output->fade.curtain =
				shell_fade_create_view_for_output(shell, shell_output);
			if (!shell_output->fade.curtain)
				continue;

			shell_output->fade.curtain->view->alpha = 1.0 - tint;
			weston_view_update_transform(shell_output->fade.curtain->view);
		}

		if (shell_output->fade.curtain->view->output == NULL) {
			/* If the black view gets a NULL output, we lost the
			 * last output and we'll just cancel the fade.  This
			 * happens when you close the last window under the
			 * X11 or Wayland backends. */
			shell->locked = false;
			weston_shell_utils_curtain_destroy(shell_output->fade.curtain);
			shell_output->fade.curtain = NULL;
		} else if (shell_output->fade.animation) {
			weston_fade_update(shell_output->fade.animation, tint);
		} else {
			shell_output->fade.animation =
				weston_fade_run(shell_output->fade.curtain->view,
						1.0 - tint, tint, 300.0,
						shell_fade_done_for_output, shell_output);
		}
	}
}

static void
do_shell_fade_startup(void *data)
{
	struct desktop_shell *shell = data;
	struct shell_output *shell_output;

	if (shell->startup_animation_type == ANIMATION_FADE) {
		shell_fade(shell, FADE_IN);
	} else {
		weston_log("desktop shell: "
			   "unexpected fade-in animation type %d\n",
			   shell->startup_animation_type);
		wl_list_for_each(shell_output, &shell->output_list, link) {
			weston_shell_utils_curtain_destroy(shell_output->fade.curtain);
			shell_output->fade.curtain = NULL;
		}
	}
}

static void
shell_fade_startup(struct desktop_shell *shell)
{
	struct wl_event_loop *loop;
	struct shell_output *shell_output;
	bool has_fade = false;

	wl_list_for_each(shell_output, &shell->output_list, link) {
		if (!shell_output->fade.startup_timer)
			continue;

		wl_event_source_remove(shell_output->fade.startup_timer);
		shell_output->fade.startup_timer = NULL;
		has_fade = true;
	}

	if (has_fade) {
		loop = wl_display_get_event_loop(shell->compositor->wl_display);
		wl_event_loop_add_idle(loop, do_shell_fade_startup, shell);
	}
}

static int
fade_startup_timeout(void *data)
{
	struct desktop_shell *shell = data;

	shell_fade_startup(shell);
	return 0;
}

static void
shell_fade_init(struct desktop_shell *shell)
{
	/* Make compositor output all black, and wait for the desktop-shell
	 * client to signal it is ready, then fade in. The timer triggers a
	 * fade-in, in case the desktop-shell client takes too long.
	 */

	struct wl_event_loop *loop;
	struct shell_output *shell_output;

	if (shell->startup_animation_type == ANIMATION_NONE)
		return;

	wl_list_for_each(shell_output, &shell->output_list, link) {
		if (shell_output->fade.curtain != NULL) {
			weston_log("%s: warning: fade surface already exists\n",
				   __func__);
			continue;
		}

		shell_output->fade.curtain =
			shell_fade_create_view_for_output(shell, shell_output);
		if (!shell_output->fade.curtain)
			continue;

		weston_view_update_transform(shell_output->fade.curtain->view);
		weston_surface_damage(shell_output->fade.curtain->view->surface);

		loop = wl_display_get_event_loop(shell->compositor->wl_display);
		shell_output->fade.startup_timer =
			wl_event_loop_add_timer(loop, fade_startup_timeout, shell);
		wl_event_source_timer_update(shell_output->fade.startup_timer, 15000);
	}
}

static void
idle_handler(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, idle_listener);

	struct weston_seat *seat;

	wl_list_for_each(seat, &shell->compositor->seat_list, link)
		weston_seat_break_desktop_grabs(seat);

	shell_fade(shell, FADE_OUT);
	/* lock() is called from shell_fade_done_for_output() */
}

static void
wake_handler(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, wake_listener);

	unlock(shell);
}

static void
transform_handler(struct wl_listener *listener, void *data)
{
	struct weston_surface *surface = data;
	struct shell_surface *shsurf = get_shell_surface(surface);
	const struct weston_xwayland_surface_api *api;
	int x, y;

	if (!shsurf)
		return;

	shell_surface_set_output(shsurf, shsurf->view->output);

	api = shsurf->shell->xwayland_surface_api;
	if (!api) {
		api = weston_xwayland_surface_get_api(shsurf->shell->compositor);
		shsurf->shell->xwayland_surface_api = api;
	}

	if (!api || !api->is_xwayland_surface(surface))
		return;

	if (!weston_view_is_mapped(shsurf->view))
		return;

	x = shsurf->view->geometry.pos_offset.x;
	y = shsurf->view->geometry.pos_offset.y;

	api->send_position(surface, x, y);
}

static void
weston_view_set_initial_position(struct weston_view *view,
				 struct desktop_shell *shell)
{
	struct weston_compositor *compositor = shell->compositor;
	int ix = 0, iy = 0;
	int32_t range_x, range_y;
	int32_t x, y;
	struct weston_output *output, *target_output = NULL;
	struct weston_seat *seat;
	pixman_rectangle32_t area;

	/* As a heuristic place the new window on the same output as the
	 * pointer. Falling back to the output containing 0, 0.
	 *
	 * TODO: Do something clever for touch too?
	 */
	wl_list_for_each(seat, &compositor->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);

		if (pointer) {
			ix = pointer->pos.c.x;
			iy = pointer->pos.c.y;
			break;
		}
	}

	wl_list_for_each(output, &compositor->output_list, link) {
		if (weston_output_contains_point(output, ix, iy)) {
			target_output = output;
			break;
		}
	}

	if (!target_output) {
		weston_view_set_position(view, 10 + random() % 400,
					 10 + random() % 400);
		return;
	}

	/* Valid range within output where the surface will still be onscreen.
	 * If this is negative it means that the surface is bigger than
	 * output.
	 */
	get_output_work_area(shell, target_output, &area);

	x = area.x;
	y = area.y;
	range_x = area.width - view->surface->width;
	range_y = area.height - view->surface->height;

	if (range_x > 0)
		x += random() % range_x;

	if (range_y > 0)
		y += random() % range_y;

	weston_view_set_position(view, x, y);
}

static bool
check_desktop_shell_crash_too_early(struct desktop_shell *shell)
{
	struct timespec now;

	if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
		return false;

	/*
	 * If the shell helper client dies before the session has been
	 * up for roughly 30 seconds, better just make Weston shut down,
	 * because the user likely has no way to interact with the desktop
	 * anyway.
	 */
	if (now.tv_sec - shell->startup_time.tv_sec < 30) {
		weston_log("Error: %s apparently cannot run at all.\n",
			   shell->client);
		weston_log_continue(STAMP_SPACE "Quitting...");
		weston_compositor_exit_with_code(shell->compositor,
						 EXIT_FAILURE);

		return true;
	}

	return false;
}

static void launch_desktop_shell_process(void *data);

static void
respawn_desktop_shell_process(struct desktop_shell *shell)
{
	struct timespec time;

	/* if desktop-shell dies more than 5 times in 30 seconds, give up */
	weston_compositor_get_time(&time);
	if (timespec_sub_to_msec(&time, &shell->child.deathstamp) > 30000) {
		shell->child.deathstamp = time;
		shell->child.deathcount = 0;
	}

	shell->child.deathcount++;
	if (shell->child.deathcount > 5) {
		weston_log("%s disconnected, giving up.\n", shell->client);
		return;
	}

	weston_log("%s disconnected, respawning...\n", shell->client);
	launch_desktop_shell_process(shell);
}

static void
desktop_shell_client_destroy(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell;

	shell = container_of(listener, struct desktop_shell,
			     child.client_destroy_listener);

	wl_list_remove(&shell->child.client_destroy_listener.link);
	shell->child.client = NULL;
	/*
	 * unbind_desktop_shell() will reset shell->child.desktop_shell
	 * before the respawned process has a chance to create a new
	 * desktop_shell object, because we are being called from the
	 * wl_client destructor which destroys all wl_resources before
	 * returning.
	 */

	if (!check_desktop_shell_crash_too_early(shell))
		respawn_desktop_shell_process(shell);

	shell_fade_startup(shell);
}

static void
launch_desktop_shell_process(void *data)
{
	struct desktop_shell *shell = data;

	shell->child.client = weston_client_start(shell->compositor,
						  shell->client);

	if (!shell->child.client) {
		weston_log("not able to start %s\n", shell->client);
		return;
	}

	shell->child.client_destroy_listener.notify =
		desktop_shell_client_destroy;
	wl_client_add_destroy_listener(shell->child.client,
				       &shell->child.client_destroy_listener);
}

static void
unbind_desktop_shell(struct wl_resource *resource)
{
	struct desktop_shell *shell = wl_resource_get_user_data(resource);

	if (shell->locked)
		resume_desktop(shell);

	shell->child.desktop_shell = NULL;
	shell->prepare_event_sent = false;
}

static void
bind_desktop_shell(struct wl_client *client,
		   void *data, uint32_t version, uint32_t id)
{
	struct desktop_shell *shell = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &weston_desktop_shell_interface,
				      1, id);

	if (client == shell->child.client) {
		wl_resource_set_implementation(resource,
					       &desktop_shell_implementation,
					       shell, unbind_desktop_shell);
		shell->child.desktop_shell = resource;
		return;
	}

	wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			       "permission to bind desktop_shell denied");
}

struct switcher {
	struct desktop_shell *shell;
	struct weston_view *current;
	struct wl_listener listener;
	struct weston_keyboard_grab grab;
	struct wl_array minimized_array;
};

static void
switcher_next(struct switcher *switcher)
{
	struct weston_view *view;
	struct weston_view *first = NULL, *prev = NULL, *next = NULL;
	struct shell_surface *shsurf;
	struct workspace *ws = get_current_workspace(switcher->shell);

	 /* temporary re-display minimized surfaces */
	struct weston_view *tmp;
	struct weston_view **minimized;
	wl_list_for_each_safe(view, tmp, &switcher->shell->minimized_layer.view_list.link, layer_link.link) {
		weston_layer_entry_remove(&view->layer_link);
		weston_layer_entry_insert(&ws->layer.view_list, &view->layer_link);
		minimized = wl_array_add(&switcher->minimized_array, sizeof *minimized);
		*minimized = view;
	}

	wl_list_for_each(view, &ws->layer.view_list.link, layer_link.link) {
		shsurf = get_shell_surface(view->surface);
		if (shsurf) {
			if (first == NULL)
				first = view;
			if (prev == switcher->current)
				next = view;
			prev = view;
			view->alpha = 0.25;
			weston_view_geometry_dirty(view);
			weston_surface_damage(view->surface);
		}

		if (is_black_surface_view(view, NULL)) {
			view->alpha = 0.25;
			weston_view_geometry_dirty(view);
			weston_surface_damage(view->surface);
		}
	}

	if (next == NULL)
		next = first;

	if (next == NULL)
		return;

	wl_list_remove(&switcher->listener.link);
	wl_signal_add(&next->destroy_signal, &switcher->listener);

	switcher->current = next;
	wl_list_for_each(view, &next->surface->views, surface_link)
		view->alpha = 1.0;

	shsurf = get_shell_surface(switcher->current->surface);
	if (shsurf && weston_desktop_surface_get_fullscreen(shsurf->desktop_surface))
		shsurf->fullscreen.black_view->view->alpha = 1.0;
}

static void
switcher_handle_view_destroy(struct wl_listener *listener, void *data)
{
	struct switcher *switcher =
		container_of(listener, struct switcher, listener);

	switcher_next(switcher);
}

static void
switcher_destroy(struct switcher *switcher)
{
	struct weston_view *view;
	struct weston_keyboard *keyboard = switcher->grab.keyboard;
	struct workspace *ws = get_current_workspace(switcher->shell);

	wl_list_for_each(view, &ws->layer.view_list.link, layer_link.link) {
		if (is_focus_view(view))
			continue;

		view->alpha = 1.0;
		weston_surface_damage(view->surface);
	}

	if (switcher->current && get_shell_surface(switcher->current->surface)) {
		activate(switcher->shell, switcher->current,
			 keyboard->seat,
			 WESTON_ACTIVATE_FLAG_CONFIGURE);
	}

	wl_list_remove(&switcher->listener.link);
	weston_keyboard_end_grab(keyboard);
	if (keyboard->input_method_resource)
		keyboard->grab = &keyboard->input_method_grab;

	 /* re-hide surfaces that were temporary shown during the switch */
	struct weston_view **minimized;
	wl_array_for_each(minimized, &switcher->minimized_array) {
		/* with the exception of the current selected */
		if ((*minimized)->surface != switcher->current->surface) {
			weston_layer_entry_remove(&(*minimized)->layer_link);
			weston_layer_entry_insert(&switcher->shell->minimized_layer.view_list, &(*minimized)->layer_link);
			weston_view_damage_below(*minimized);
		}
	}
	wl_array_release(&switcher->minimized_array);

	free(switcher);
}

static void
switcher_key(struct weston_keyboard_grab *grab,
	     const struct timespec *time, uint32_t key, uint32_t state_w)
{
	struct switcher *switcher = container_of(grab, struct switcher, grab);
	enum wl_keyboard_key_state state = state_w;

	if (key == KEY_TAB && state == WL_KEYBOARD_KEY_STATE_PRESSED)
		switcher_next(switcher);
}

static void
switcher_modifier(struct weston_keyboard_grab *grab, uint32_t serial,
		  uint32_t mods_depressed, uint32_t mods_latched,
		  uint32_t mods_locked, uint32_t group)
{
	struct switcher *switcher = container_of(grab, struct switcher, grab);
	struct weston_seat *seat = grab->keyboard->seat;

	if ((seat->modifier_state & switcher->shell->binding_modifier) == 0)
		switcher_destroy(switcher);
}

static void
switcher_cancel(struct weston_keyboard_grab *grab)
{
	struct switcher *switcher = container_of(grab, struct switcher, grab);

	switcher_destroy(switcher);
}

static const struct weston_keyboard_grab_interface switcher_grab = {
	switcher_key,
	switcher_modifier,
	switcher_cancel,
};

static void
switcher_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t key, void *data)
{
	struct desktop_shell *shell = data;
	struct switcher *switcher;

	switcher = malloc(sizeof *switcher);
	if (!switcher)
		return;

	switcher->shell = shell;
	switcher->current = NULL;
	switcher->listener.notify = switcher_handle_view_destroy;
	wl_list_init(&switcher->listener.link);
	wl_array_init(&switcher->minimized_array);

	lower_fullscreen_layer(switcher->shell, NULL);
	switcher->grab.interface = &switcher_grab;
	weston_keyboard_start_grab(keyboard, &switcher->grab);
	weston_keyboard_set_focus(keyboard, NULL);
	switcher_next(switcher);
}

static void
backlight_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		  uint32_t key, void *data)
{
	struct weston_compositor *compositor = data;
	struct weston_output *output;
	long backlight_new = 0;

	/* TODO: we're limiting to simple use cases, where we assume just
	 * control on the primary display. We'd have to extend later if we
	 * ever get support for setting backlights on random desktop LCD
	 * panels though */
	output = weston_shell_utils_get_default_output(compositor);
	if (!output)
		return;

	if (!output->set_backlight)
		return;

	if (key == KEY_F9 || key == KEY_BRIGHTNESSDOWN)
		backlight_new = output->backlight_current - 25;
	else if (key == KEY_F10 || key == KEY_BRIGHTNESSUP)
		backlight_new = output->backlight_current + 25;

	if (backlight_new < 5)
		backlight_new = 5;
	if (backlight_new > 255)
		backlight_new = 255;

	output->backlight_current = backlight_new;
	output->set_backlight(output, output->backlight_current);
}

static void
force_kill_binding(struct weston_keyboard *keyboard,
		   const struct timespec *time, uint32_t key, void *data)
{
	struct weston_surface *focus_surface;
	struct wl_client *client;
	struct desktop_shell *shell = data;
	struct weston_compositor *compositor = shell->compositor;
	pid_t pid;

	focus_surface = keyboard->focus;
	if (!focus_surface)
		return;

	wl_signal_emit(&compositor->kill_signal, focus_surface);

	client = wl_resource_get_client(focus_surface->resource);
	wl_client_get_credentials(client, &pid, NULL, NULL);

	/* Skip clients that we launched ourselves (the credentials of
	 * the socketpair is ours) */
	if (pid == getpid())
		return;

	kill(pid, SIGKILL);
}

static void
shell_reposition_view_on_output_change(struct weston_view *view)
{
	struct weston_output *output, *first_output;
	struct weston_compositor *ec = view->surface->compositor;
	struct shell_surface *shsurf;
	float x, y;
	int visible;

	if (wl_list_empty(&ec->output_list))
		return;

	x = view->geometry.pos_offset.x;
	y = view->geometry.pos_offset.y;

	/* At this point the destroyed output is not in the list anymore.
	 * If the view is still visible somewhere, we leave where it is,
	 * otherwise, move it to the first output. */
	visible = 0;
	wl_list_for_each(output, &ec->output_list, link) {
		if (weston_output_contains_point(output, x, y)) {
			visible = 1;
			break;
		}
	}

	shsurf = get_shell_surface(view->surface);
	if (!shsurf)
		return;

	if (!visible) {
		first_output = container_of(ec->output_list.next,
					    struct weston_output, link);

		x = first_output->x + first_output->width / 4;
		y = first_output->y + first_output->height / 4;

		weston_view_set_position(view, x, y);
	} else {
		weston_view_geometry_dirty(view);

		if (shsurf->state.maximized ||
		    shsurf->state.fullscreen)
			return;
	}


	shsurf->saved_position_valid = false;
	set_maximized(shsurf, false);
	set_fullscreen(shsurf, false, NULL);
}

void
shell_for_each_layer(struct desktop_shell *shell,
		     shell_for_each_layer_func_t func, void *data)
{
	func(shell, &shell->fullscreen_layer, data);
	func(shell, &shell->panel_layer, data);
	func(shell, &shell->background_layer, data);
	func(shell, &shell->lock_layer, data);
	func(shell, &shell->input_panel_layer, data);
	func(shell, &shell->workspace.layer, data);
}

static void
shell_output_changed_move_layer(struct desktop_shell *shell,
				struct weston_layer *layer,
				void *data)
{
	struct weston_view *view;

	wl_list_for_each(view, &layer->view_list.link, layer_link.link)
		shell_reposition_view_on_output_change(view);

}

static void
shell_output_destroy(struct shell_output *shell_output)
{
	struct desktop_shell *shell = shell_output->shell;

	shell_for_each_layer(shell, shell_output_changed_move_layer, NULL);

	if (shell_output->fade.animation) {
		weston_view_animation_destroy(shell_output->fade.animation);
		shell_output->fade.animation = NULL;
	}

	if (shell_output->fade.curtain)
		weston_shell_utils_curtain_destroy(shell_output->fade.curtain);

	if (shell_output->fade.startup_timer)
		wl_event_source_remove(shell_output->fade.startup_timer);

	if (shell_output->panel_surface)
		wl_list_remove(&shell_output->panel_surface_listener.link);
	if (shell_output->background_surface)
		wl_list_remove(&shell_output->background_surface_listener.link);
	wl_list_remove(&shell_output->destroy_listener.link);
	wl_list_remove(&shell_output->link);
	free(shell_output);
}

static void
handle_output_destroy(struct wl_listener *listener, void *data)
{
	struct shell_output *shell_output =
		container_of(listener, struct shell_output, destroy_listener);

	shell_output_destroy(shell_output);
}

static void
shell_resize_surface_to_output(struct desktop_shell *shell,
				struct weston_surface *surface,
				const struct weston_output *output)
{
	if (!surface)
		return;

	weston_desktop_shell_send_configure(shell->child.desktop_shell, 0,
					surface->resource,
					output->width,
					output->height);
}


static void
handle_output_resized(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, resized_listener);
	struct weston_output *output = (struct weston_output *)data;
	struct shell_output *sh_output = find_shell_output_from_weston_output(shell, output);

	shell_resize_surface_to_output(shell, sh_output->background_surface, output);
	shell_resize_surface_to_output(shell, sh_output->panel_surface, output);
}

static void
create_shell_output(struct desktop_shell *shell,
					struct weston_output *output)
{
	struct shell_output *shell_output;

	shell_output = zalloc(sizeof *shell_output);
	if (shell_output == NULL)
		return;

	shell_output->output = output;
	shell_output->shell = shell;
	shell_output->destroy_listener.notify = handle_output_destroy;
	wl_signal_add(&output->destroy_signal,
		      &shell_output->destroy_listener);
	wl_list_insert(shell->output_list.prev, &shell_output->link);

	if (wl_list_length(&shell->output_list) == 1)
		shell_for_each_layer(shell,
				     shell_output_changed_move_layer, NULL);
}

static void
handle_output_create(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, output_create_listener);
	struct weston_output *output = (struct weston_output *)data;

	create_shell_output(shell, output);
}

static void
handle_output_move_layer(struct desktop_shell *shell,
			 struct weston_layer *layer, void *data)
{
	struct weston_output *output = data;
	struct weston_view *view;
	float x, y;

	wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
		if (view->output != output)
			continue;

		x = view->geometry.pos_offset.x + output->move_x;
		y = view->geometry.pos_offset.y + output->move_y;
		weston_view_set_position(view, x, y);
	}
}

static void
handle_output_move(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell;

	shell = container_of(listener, struct desktop_shell,
			     output_move_listener);

	shell_for_each_layer(shell, handle_output_move_layer, data);
}

static void
setup_output_destroy_handler(struct weston_compositor *ec,
							struct desktop_shell *shell)
{
	struct weston_output *output;

	wl_list_init(&shell->output_list);
	wl_list_for_each(output, &ec->output_list, link)
		create_shell_output(shell, output);

	shell->output_create_listener.notify = handle_output_create;
	wl_signal_add(&ec->output_created_signal,
				&shell->output_create_listener);

	shell->output_move_listener.notify = handle_output_move;
	wl_signal_add(&ec->output_moved_signal, &shell->output_move_listener);
}

static void
desktop_shell_destroy_layer(struct weston_layer *layer)
{
	struct weston_view *view;
	bool removed;

	do {
		removed = false;

		/* fullscreen_layer is special as it would have a view with an
		 * additional black_view created and added to its layer_link
		 * fullscreen view. See shell_ensure_fullscreen_black_view()
		 *
		 * Note that we do not choose to destroy all other potential
		 * views we find in the layer, but instead we explicitly verify
		 * if the view in question was explicitly created by
		 * desktop-shell, rather than libweston-desktop (in
		 * desktop_surface_added()).
		 *
		 * This is particularly important because libweston-desktop
		 * could create additional views, which are managed implicitly,
		 * but which are still being added to the layer list.
		 *
		 * We avoid using wl_list_for_each_safe() as it can't handle
		 * removal of the next item in the list, so with this approach
		 * we restart the loop as long as we keep removing views from
		 * the list.
		 */
		wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
			struct shell_surface *shsurf =
				get_shell_surface(view->surface);
			if (shsurf) {
				desktop_shell_destroy_surface(shsurf);
				removed = true;
				break;
			}
		}

	} while (removed);

	weston_layer_fini(layer);
}

static void
shell_destroy(struct wl_listener *listener, void *data)
{
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, destroy_listener);
	struct shell_output *shell_output, *tmp;
	struct shell_seat *shseat, *shseat_next;

	/* Force state to unlocked so we don't try to fade */
	shell->locked = false;

	if (shell->child.client) {
		/* disable respawn */
		wl_list_remove(&shell->child.client_destroy_listener.link);
		wl_client_destroy(shell->child.client);
	}

	wl_list_remove(&shell->destroy_listener.link);
	wl_list_remove(&shell->idle_listener.link);
	wl_list_remove(&shell->wake_listener.link);
	wl_list_remove(&shell->transform_listener.link);

	text_backend_destroy(shell->text_backend);
	input_panel_destroy(shell);

	wl_list_for_each_safe(shell_output, tmp, &shell->output_list, link)
		shell_output_destroy(shell_output);

	wl_list_remove(&shell->output_create_listener.link);
	wl_list_remove(&shell->output_move_listener.link);
	wl_list_remove(&shell->resized_listener.link);

	wl_list_for_each_safe(shseat, shseat_next, &shell->seat_list, link)
		desktop_shell_destroy_seat(shseat);

	weston_desktop_destroy(shell->desktop);

	workspace_destroy(&shell->workspace);

	desktop_shell_destroy_layer(&shell->panel_layer);
	desktop_shell_destroy_layer(&shell->background_layer);
	desktop_shell_destroy_layer(&shell->lock_layer);
	desktop_shell_destroy_layer(&shell->input_panel_layer);
	desktop_shell_destroy_layer(&shell->minimized_layer);
	desktop_shell_destroy_layer(&shell->fullscreen_layer);

	free(shell->client);
	free(shell);
}

static void
shell_add_bindings(struct weston_compositor *ec, struct desktop_shell *shell)
{
	uint32_t mod;

	if (shell->allow_zap)
		weston_compositor_add_key_binding(ec, KEY_BACKSPACE,
					          MODIFIER_CTRL | MODIFIER_ALT,
					          terminate_binding, ec);

	/* fixed bindings */
	weston_compositor_add_button_binding(ec, BTN_LEFT, 0,
					     click_to_activate_binding,
					     shell);
	weston_compositor_add_button_binding(ec, BTN_RIGHT, 0,
					     click_to_activate_binding,
					     shell);
	weston_compositor_add_touch_binding(ec, 0,
					    touch_to_activate_binding,
					    shell);
	weston_compositor_add_tablet_tool_binding(ec, BTN_TOUCH, 0,
						  tablet_tool_activate_binding, shell);
	weston_compositor_add_key_binding(ec, KEY_BRIGHTNESSDOWN, 0,
				          backlight_binding, ec);
	weston_compositor_add_key_binding(ec, KEY_BRIGHTNESSUP, 0,
				          backlight_binding, ec);

	mod = shell->binding_modifier;
	if (!mod)
		return;

	/* This binding is not configurable, but is only enabled if there is a
	 * valid binding modifier. */
	weston_compositor_add_axis_binding(ec, WL_POINTER_AXIS_VERTICAL_SCROLL,
				           MODIFIER_SUPER | MODIFIER_ALT,
				           surface_opacity_binding, NULL);

	weston_compositor_add_key_binding(ec, KEY_M, mod | MODIFIER_SHIFT,
					  maximize_binding, NULL);
	weston_compositor_add_key_binding(ec, KEY_F, mod | MODIFIER_SHIFT,
					  fullscreen_binding, NULL);
	weston_compositor_add_button_binding(ec, BTN_LEFT, mod, move_binding,
					     shell);
	weston_compositor_add_touch_binding(ec, mod, touch_move_binding, shell);
	weston_compositor_add_button_binding(ec, BTN_RIGHT, mod,
					     resize_binding, shell);
	weston_compositor_add_button_binding(ec, BTN_LEFT,
					     mod | MODIFIER_SHIFT,
					     resize_binding, shell);

	weston_compositor_add_key_binding(ec, KEY_LEFT, mod | MODIFIER_SHIFT,
					  set_tiled_orientation_left, NULL);
	weston_compositor_add_key_binding(ec, KEY_RIGHT, mod | MODIFIER_SHIFT,
					  set_tiled_orientation_right, NULL);
	weston_compositor_add_key_binding(ec, KEY_UP, mod | MODIFIER_SHIFT,
					  set_tiled_orientation_up, NULL);
	weston_compositor_add_key_binding(ec, KEY_DOWN, mod | MODIFIER_SHIFT,
					  set_tiled_orientation_down, NULL);

	if (ec->capabilities & WESTON_CAP_ROTATION_ANY)
		weston_compositor_add_button_binding(ec, BTN_MIDDLE, mod,
						     rotate_binding, NULL);

	weston_compositor_add_key_binding(ec, KEY_TAB, mod, switcher_binding,
					  shell);
	weston_compositor_add_key_binding(ec, KEY_F9, mod, backlight_binding,
					  ec);
	weston_compositor_add_key_binding(ec, KEY_F10, mod, backlight_binding,
					  ec);
	weston_compositor_add_key_binding(ec, KEY_K, mod,
				          force_kill_binding, shell);

	weston_install_debug_key_binding(ec, mod);
}

static void
handle_seat_created(struct wl_listener *listener, void *data)
{
	struct weston_seat *seat = data;
	struct desktop_shell *shell =
		container_of(listener, struct desktop_shell, seat_create_listener);

	create_shell_seat(shell, seat);
}

WL_EXPORT int
wet_shell_init(struct weston_compositor *ec,
	       int *argc, char *argv[])
{
	struct weston_seat *seat;
	struct desktop_shell *shell;
	struct wl_event_loop *loop;

	shell = zalloc(sizeof *shell);
	if (shell == NULL)
		return -1;

	shell->compositor = ec;

	if (!weston_compositor_add_destroy_listener_once(ec,
							 &shell->destroy_listener,
							 shell_destroy)) {
		free(shell);
		return 0;
	}

	shell->idle_listener.notify = idle_handler;
	wl_signal_add(&ec->idle_signal, &shell->idle_listener);
	shell->wake_listener.notify = wake_handler;
	wl_signal_add(&ec->wake_signal, &shell->wake_listener);
	shell->transform_listener.notify = transform_handler;
	wl_signal_add(&ec->transform_signal, &shell->transform_listener);

	weston_layer_init(&shell->fullscreen_layer, ec);
	weston_layer_init(&shell->panel_layer, ec);
	weston_layer_init(&shell->background_layer, ec);
	weston_layer_init(&shell->lock_layer, ec);
	weston_layer_init(&shell->input_panel_layer, ec);

	weston_layer_set_position(&shell->fullscreen_layer,
				  WESTON_LAYER_POSITION_FULLSCREEN);
	weston_layer_set_position(&shell->panel_layer,
				  WESTON_LAYER_POSITION_UI);
	weston_layer_set_position(&shell->background_layer,
				  WESTON_LAYER_POSITION_BACKGROUND);

	wl_list_init(&shell->seat_list);

	if (input_panel_setup(shell) < 0)
		return -1;

	shell->text_backend = text_backend_init(ec);

	shell_configuration(shell);

	workspace_create(shell);

	weston_layer_init(&shell->minimized_layer, ec);

	shell->desktop = weston_desktop_create(ec, &shell_desktop_api, shell);
	if (!shell->desktop)
		return -1;

	if (wl_global_create(ec->wl_display,
			     &weston_desktop_shell_interface, 1,
			     shell, bind_desktop_shell) == NULL)
		return -1;

	weston_compositor_get_time(&shell->child.deathstamp);

	shell->panel_position = WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP;

	setup_output_destroy_handler(ec, shell);

	loop = wl_display_get_event_loop(ec->wl_display);
	wl_event_loop_add_idle(loop, launch_desktop_shell_process, shell);

	wl_list_for_each(seat, &ec->seat_list, link)
		create_shell_seat(shell, seat);
	shell->seat_create_listener.notify = handle_seat_created;
	wl_signal_add(&ec->seat_created_signal, &shell->seat_create_listener);

	shell->resized_listener.notify = handle_output_resized;
	wl_signal_add(&ec->output_resized_signal, &shell->resized_listener);

	screenshooter_create(ec);

	shell_add_bindings(ec, shell);

	shell_fade_init(shell);

	clock_gettime(CLOCK_MONOTONIC, &shell->startup_time);

	return 0;
}
