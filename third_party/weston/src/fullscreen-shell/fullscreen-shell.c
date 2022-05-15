/*
 * Copyright © 2013 Jason Ekstrand
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

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libweston/libweston.h>
#include "compositor/weston.h"
#include "fullscreen-shell-unstable-v1-server-protocol.h"
#include "shared/helpers.h"

struct fullscreen_shell {
	struct wl_client *client;
	struct wl_listener client_destroyed;
	struct weston_compositor *compositor;
	/* XXX: missing compositor destroy listener
	 * https://gitlab.freedesktop.org/wayland/weston/issues/299
	 */

	struct weston_layer layer;
	struct wl_list output_list;
	struct wl_listener output_created_listener;

	struct wl_listener seat_created_listener;

	/* List of one surface per client, presented for the NULL output
	 *
	 * This is implemented as a list in case someone fixes the shell
	 * implementation to support more than one client.
	 */
	struct wl_list default_surface_list; /* struct fs_client_surface::link */
};

struct fs_output {
	struct fullscreen_shell *shell;
	struct wl_list link;

	struct weston_output *output;
	struct wl_listener output_destroyed;

	struct {
		struct weston_surface *surface;
		struct wl_listener surface_destroyed;
		struct wl_resource *mode_feedback;

		int presented_for_mode;
		enum zwp_fullscreen_shell_v1_present_method method;
		int32_t framerate;
	} pending;

	struct weston_surface *surface;
	struct wl_listener surface_destroyed;
	struct weston_view *view;
	struct weston_view *black_view;
	struct weston_transform transform; /* matrix from x, y */

	int presented_for_mode;
	enum zwp_fullscreen_shell_v1_present_method method;
	uint32_t framerate;
};

struct pointer_focus_listener {
	struct fullscreen_shell *shell;
	struct wl_listener pointer_focus;
	struct wl_listener seat_caps;
	struct wl_listener seat_destroyed;
};

struct fs_client_surface {
	struct weston_surface *surface;
	enum zwp_fullscreen_shell_v1_present_method method;
	struct wl_list link; /* struct fullscreen_shell::default_surface_list */
	struct wl_listener surface_destroyed;
};

static void
remove_default_surface(struct fs_client_surface *surf)
{
	wl_list_remove(&surf->surface_destroyed.link);
	wl_list_remove(&surf->link);
	free(surf);
}

static void
default_surface_destroy_listener(struct wl_listener *listener, void *data)
{
	struct fs_client_surface *surf;

	surf = container_of(listener, struct fs_client_surface, surface_destroyed);

	remove_default_surface(surf);
}

static void
replace_default_surface(struct fullscreen_shell *shell, struct weston_surface *surface,
			enum zwp_fullscreen_shell_v1_present_method method)
{
	struct fs_client_surface *surf, *prev = NULL;

	if (!wl_list_empty(&shell->default_surface_list))
		prev = container_of(shell->default_surface_list.prev,
				    struct fs_client_surface, link);

	surf = zalloc(sizeof *surf);
	if (!surf)
		return;

	surf->surface = surface;
	surf->method = method;

	if (prev)
		remove_default_surface(prev);

	wl_list_insert(shell->default_surface_list.prev, &surf->link);

	surf->surface_destroyed.notify = default_surface_destroy_listener;
	wl_signal_add(&surface->destroy_signal, &surf->surface_destroyed);
}

static void
pointer_focus_changed(struct wl_listener *listener, void *data)
{
	struct weston_pointer *pointer = data;

	if (pointer->focus && pointer->focus->surface->resource)
		weston_seat_set_keyboard_focus(pointer->seat, pointer->focus->surface);
}

static void
seat_caps_changed(struct wl_listener *l, void *data)
{
	struct weston_seat *seat = data;
	struct weston_keyboard *keyboard = weston_seat_get_keyboard(seat);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct pointer_focus_listener *listener;
	struct fs_output *fsout;

	listener = container_of(l, struct pointer_focus_listener, seat_caps);

	/* no pointer */
	if (pointer) {
		if (!listener->pointer_focus.link.prev) {
			wl_signal_add(&pointer->focus_signal,
				      &listener->pointer_focus);
		}
	} else {
		if (listener->pointer_focus.link.prev) {
			wl_list_remove(&listener->pointer_focus.link);
		}
	}

	if (keyboard && keyboard->focus != NULL) {
		wl_list_for_each(fsout, &listener->shell->output_list, link) {
			if (fsout->surface) {
				weston_seat_set_keyboard_focus(seat, fsout->surface);
				return;
			}
		}
	}
}

static void
seat_destroyed(struct wl_listener *l, void *data)
{
	struct pointer_focus_listener *listener;

	listener = container_of(l, struct pointer_focus_listener,
				seat_destroyed);

	free(listener);
}

static void
seat_created(struct wl_listener *l, void *data)
{
	struct weston_seat *seat = data;
	struct pointer_focus_listener *listener;

	listener = zalloc(sizeof *listener);
	if (!listener)
		return;

	listener->shell = container_of(l, struct fullscreen_shell,
				       seat_created_listener);
	listener->pointer_focus.notify = pointer_focus_changed;
	listener->seat_caps.notify = seat_caps_changed;
	listener->seat_destroyed.notify = seat_destroyed;

	wl_signal_add(&seat->destroy_signal, &listener->seat_destroyed);
	wl_signal_add(&seat->updated_caps_signal, &listener->seat_caps);

	seat_caps_changed(&listener->seat_caps, seat);
}

static void
black_surface_committed(struct weston_surface *es, int32_t sx, int32_t sy)
{
}

static struct weston_view *
create_black_surface(struct weston_compositor *ec, struct fs_output *fsout,
		     float x, float y, int w, int h)
{
	struct weston_surface *surface = NULL;
	struct weston_view *view;

	surface = weston_surface_create(ec);
	if (surface == NULL) {
		weston_log("no memory\n");
		return NULL;
	}
	view = weston_view_create(surface);
	if (!view) {
		weston_surface_destroy(surface);
		weston_log("no memory\n");
		return NULL;
	}

	surface->committed = black_surface_committed;
	surface->committed_private = fsout;
	weston_surface_set_color(surface, 0.0f, 0.0f, 0.0f, 1.0f);
	pixman_region32_fini(&surface->opaque);
	pixman_region32_init_rect(&surface->opaque, 0, 0, w, h);
	pixman_region32_fini(&surface->input);
	pixman_region32_init_rect(&surface->input, 0, 0, w, h);

	weston_surface_set_size(surface, w, h);
	weston_view_set_position(view, x, y);

	return view;
}

static void
fs_output_set_surface(struct fs_output *fsout, struct weston_surface *surface,
		      enum zwp_fullscreen_shell_v1_present_method method,
		      int32_t framerate, int presented_for_mode);
static void
fs_output_apply_pending(struct fs_output *fsout);
static void
fs_output_clear_pending(struct fs_output *fsout);

static void
fs_output_destroy(struct fs_output *fsout)
{
	fs_output_set_surface(fsout, NULL, 0, 0, 0);
	fs_output_clear_pending(fsout);

	wl_list_remove(&fsout->link);

	if (fsout->output)
		wl_list_remove(&fsout->output_destroyed.link);
}

static void
output_destroyed(struct wl_listener *listener, void *data)
{
	struct fs_output *output = container_of(listener,
						struct fs_output,
						output_destroyed);
	fs_output_destroy(output);
}

static void
surface_destroyed(struct wl_listener *listener, void *data)
{
	struct fs_output *fsout = container_of(listener,
					       struct fs_output,
					       surface_destroyed);
	fsout->surface = NULL;
	fsout->view = NULL;
	wl_list_remove(&fsout->transform.link);
	wl_list_init(&fsout->transform.link);
}

static void
pending_surface_destroyed(struct wl_listener *listener, void *data)
{
	struct fs_output *fsout = container_of(listener,
					       struct fs_output,
					       pending.surface_destroyed);
	fsout->pending.surface = NULL;
}

static void
configure_presented_surface(struct weston_surface *surface, int32_t sx,
			    int32_t sy);

static struct fs_output *
fs_output_create(struct fullscreen_shell *shell, struct weston_output *output)
{
	struct fs_output *fsout;
	struct fs_client_surface *surf;

	fsout = zalloc(sizeof *fsout);
	if (!fsout)
		return NULL;

	fsout->shell = shell;
	wl_list_insert(&shell->output_list, &fsout->link);

	fsout->output = output;
	fsout->output_destroyed.notify = output_destroyed;
	wl_signal_add(&output->destroy_signal, &fsout->output_destroyed);

	fsout->surface_destroyed.notify = surface_destroyed;
	fsout->pending.surface_destroyed.notify = pending_surface_destroyed;
	fsout->black_view = create_black_surface(shell->compositor, fsout,
						 output->x, output->y,
						 output->width, output->height);
	fsout->black_view->surface->is_mapped = true;
	fsout->black_view->is_mapped = true;
	weston_layer_entry_insert(&shell->layer.view_list,
		       &fsout->black_view->layer_link);
	wl_list_init(&fsout->transform.link);

	if (!wl_list_empty(&shell->default_surface_list)) {
		surf = container_of(shell->default_surface_list.prev,
				    struct fs_client_surface, link);

		fs_output_set_surface(fsout, surf->surface, surf->method, 0, 0);
		configure_presented_surface(surf->surface, 0, 0);
	}

	return fsout;
}

static struct fs_output *
fs_output_for_output(struct weston_output *output)
{
	struct wl_listener *listener;

	if (!output)
		return NULL;

	listener = wl_signal_get(&output->destroy_signal, output_destroyed);

	return container_of(listener, struct fs_output, output_destroyed);
}

static void
restore_output_mode(struct weston_output *output)
{
	if (output && output->original_mode)
		weston_output_mode_switch_to_native(output);
}

/*
 * Returns the bounding box of a surface and all its sub-surfaces,
 * in surface-local coordinates. */
static void
surface_subsurfaces_boundingbox(struct weston_surface *surface, int32_t *x,
				int32_t *y, int32_t *w, int32_t *h) {
	pixman_region32_t region;
	pixman_box32_t *box;
	struct weston_subsurface *subsurface;

	pixman_region32_init_rect(&region, 0, 0,
	                          surface->width,
	                          surface->height);

	wl_list_for_each(subsurface, &surface->subsurface_list, parent_link) {
		pixman_region32_union_rect(&region, &region,
		                           subsurface->position.x,
		                           subsurface->position.y,
		                           subsurface->surface->width,
		                           subsurface->surface->height);
	}

	box = pixman_region32_extents(&region);
	if (x)
		*x = box->x1;
	if (y)
		*y = box->y1;
	if (w)
		*w = box->x2 - box->x1;
	if (h)
		*h = box->y2 - box->y1;

	pixman_region32_fini(&region);
}

static void
fs_output_center_view(struct fs_output *fsout)
{
	int32_t surf_x, surf_y, surf_width, surf_height;
	float x, y;
	struct weston_output *output = fsout->output;

	surface_subsurfaces_boundingbox(fsout->view->surface, &surf_x, &surf_y,
					&surf_width, &surf_height);

	x = output->x + (output->width - surf_width) / 2 - surf_x / 2;
	y = output->y + (output->height - surf_height) / 2 - surf_y / 2;

	weston_view_set_position(fsout->view, x, y);
}

static void
fs_output_scale_view(struct fs_output *fsout, float width, float height)
{
	float x, y;
	int32_t surf_x, surf_y, surf_width, surf_height;
	struct weston_matrix *matrix;
	struct weston_view *view = fsout->view;
	struct weston_output *output = fsout->output;

	surface_subsurfaces_boundingbox(view->surface, &surf_x, &surf_y,
					&surf_width, &surf_height);

	if (output->width == surf_width && output->height == surf_height) {
		weston_view_set_position(view,
					 fsout->output->x - surf_x,
					 fsout->output->y - surf_y);
	} else {
		matrix = &fsout->transform.matrix;
		weston_matrix_init(matrix);

		weston_matrix_scale(matrix, width / surf_width,
				    height / surf_height, 1);
		wl_list_remove(&fsout->transform.link);
		wl_list_insert(&fsout->view->geometry.transformation_list,
			       &fsout->transform.link);

		x = output->x + (output->width - width) / 2 - surf_x;
		y = output->y + (output->height - height) / 2 - surf_y;

		weston_view_set_position(view, x, y);
	}
}

static void
fs_output_configure(struct fs_output *fsout, struct weston_surface *surface);

static void
fs_output_configure_simple(struct fs_output *fsout,
			   struct weston_surface *configured_surface)
{
	struct weston_output *output = fsout->output;
	float output_aspect, surface_aspect;
	int32_t surf_x, surf_y, surf_width, surf_height;

	if (fsout->pending.surface == configured_surface)
		fs_output_apply_pending(fsout);

	assert(fsout->view);

	restore_output_mode(fsout->output);

	wl_list_remove(&fsout->transform.link);
	wl_list_init(&fsout->transform.link);

	surface_subsurfaces_boundingbox(fsout->view->surface,
					&surf_x, &surf_y,
					&surf_width, &surf_height);

	output_aspect = (float) output->width / (float) output->height;
	surface_aspect = (float) surf_width / (float) surf_height;

	switch (fsout->method) {
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT:
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_CENTER:
		fs_output_center_view(fsout);
		break;

	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_ZOOM:
		if (output_aspect < surface_aspect)
			fs_output_scale_view(fsout,
					     output->width,
					     output->width / surface_aspect);
		else
			fs_output_scale_view(fsout,
					     output->height * surface_aspect,
					     output->height);
		break;

	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_ZOOM_CROP:
		if (output_aspect < surface_aspect)
			fs_output_scale_view(fsout,
					     output->height * surface_aspect,
					     output->height);
		else
			fs_output_scale_view(fsout,
					     output->width,
					     output->width / surface_aspect);
		break;

	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_STRETCH:
		fs_output_scale_view(fsout, output->width, output->height);
		break;
	default:
		break;
	}

	weston_view_set_position(fsout->black_view,
				 fsout->output->x - surf_x,
				 fsout->output->y - surf_y);
	weston_surface_set_size(fsout->black_view->surface,
				fsout->output->width,
				fsout->output->height);
}

static void
fs_output_configure_for_mode(struct fs_output *fsout,
			     struct weston_surface *configured_surface)
{
	int32_t surf_x, surf_y, surf_width, surf_height;
	struct weston_mode mode;
	int ret;

	if (fsout->pending.surface != configured_surface) {
		/* Nothing to really reconfigure.  We'll just recenter the
		 * view in case they played with subsurfaces */
		fs_output_center_view(fsout);
		return;
	}

	/* We have a pending surface */
	surface_subsurfaces_boundingbox(fsout->pending.surface,
					&surf_x, &surf_y,
					&surf_width, &surf_height);

	/* The actual output mode is in physical units.  We need to
	 * transform the surface size to physical unit size by flipping and
	 * possibly scaling it.
	 */
	switch (fsout->output->transform) {
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		mode.width = surf_height * fsout->output->native_scale;
		mode.height = surf_width * fsout->output->native_scale;
		break;

	case WL_OUTPUT_TRANSFORM_NORMAL:
	case WL_OUTPUT_TRANSFORM_FLIPPED:
	case WL_OUTPUT_TRANSFORM_180:
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
	default:
		mode.width = surf_width * fsout->output->native_scale;
		mode.height = surf_height * fsout->output->native_scale;
	}
	mode.flags = 0;
	mode.refresh = fsout->pending.framerate;

	ret = weston_output_mode_switch_to_temporary(fsout->output, &mode,
					fsout->output->native_scale);

	if (ret != 0) {
		/* The mode switch failed.  Clear the pending and
		 * reconfigure as per normal */
		if (fsout->pending.mode_feedback) {
			zwp_fullscreen_shell_mode_feedback_v1_send_mode_failed(
				fsout->pending.mode_feedback);
			wl_resource_destroy(fsout->pending.mode_feedback);
			fsout->pending.mode_feedback = NULL;
		}

		fs_output_clear_pending(fsout);
		return;
	}

	if (fsout->pending.mode_feedback) {
		zwp_fullscreen_shell_mode_feedback_v1_send_mode_successful(
			fsout->pending.mode_feedback);
		wl_resource_destroy(fsout->pending.mode_feedback);
		fsout->pending.mode_feedback = NULL;
	}

	fs_output_apply_pending(fsout);

	weston_view_set_position(fsout->view,
				 fsout->output->x - surf_x,
				 fsout->output->y - surf_y);
}

static void
fs_output_configure(struct fs_output *fsout,
		    struct weston_surface *surface)
{
	if (fsout->pending.surface == surface) {
		if (fsout->pending.presented_for_mode)
			fs_output_configure_for_mode(fsout, surface);
		else
			fs_output_configure_simple(fsout, surface);
	} else {
		if (fsout->presented_for_mode)
			fs_output_configure_for_mode(fsout, surface);
		else
			fs_output_configure_simple(fsout, surface);
	}

	weston_output_schedule_repaint(fsout->output);
}

static void
configure_presented_surface(struct weston_surface *surface, int32_t sx,
			    int32_t sy)
{
	struct fullscreen_shell *shell = surface->committed_private;
	struct fs_output *fsout;

	if (surface->committed != configure_presented_surface)
		return;

	wl_list_for_each(fsout, &shell->output_list, link)
		if (fsout->surface == surface ||
		    fsout->pending.surface == surface)
			fs_output_configure(fsout, surface);
}

static void
fs_output_apply_pending(struct fs_output *fsout)
{
	assert(fsout->pending.surface);

	if (fsout->surface && fsout->surface != fsout->pending.surface) {
		wl_list_remove(&fsout->surface_destroyed.link);

		weston_view_destroy(fsout->view);
		fsout->view = NULL;

		if (wl_list_empty(&fsout->surface->views)) {
			fsout->surface->committed = NULL;
			fsout->surface->committed_private = NULL;
		}

		fsout->surface = NULL;
	}

	fsout->method = fsout->pending.method;
	fsout->framerate = fsout->pending.framerate;
	fsout->presented_for_mode = fsout->pending.presented_for_mode;

	if (fsout->surface != fsout->pending.surface) {
		fsout->surface = fsout->pending.surface;

		fsout->view = weston_view_create(fsout->surface);
		if (!fsout->view) {
			weston_log("no memory\n");
			return;
		}
		fsout->view->is_mapped = true;

		wl_signal_add(&fsout->surface->destroy_signal,
			      &fsout->surface_destroyed);
		weston_layer_entry_insert(&fsout->shell->layer.view_list,
			       &fsout->view->layer_link);
	}

	fs_output_clear_pending(fsout);
}

static void
fs_output_clear_pending(struct fs_output *fsout)
{
	if (!fsout->pending.surface)
		return;

	if (fsout->pending.mode_feedback) {
		zwp_fullscreen_shell_mode_feedback_v1_send_present_cancelled(
			fsout->pending.mode_feedback);
		wl_resource_destroy(fsout->pending.mode_feedback);
		fsout->pending.mode_feedback = NULL;
	}

	wl_list_remove(&fsout->pending.surface_destroyed.link);
	fsout->pending.surface = NULL;
}

static void
fs_output_set_surface(struct fs_output *fsout, struct weston_surface *surface,
		      enum zwp_fullscreen_shell_v1_present_method method,
		      int32_t framerate, int presented_for_mode)
{
	fs_output_clear_pending(fsout);

	if (surface) {
		if (!surface->committed) {
			surface->committed = configure_presented_surface;
			surface->committed_private = fsout->shell;
		}

		fsout->pending.surface = surface;
		wl_signal_add(&fsout->pending.surface->destroy_signal,
			      &fsout->pending.surface_destroyed);

		fsout->pending.method = method;
		fsout->pending.framerate = framerate;
		fsout->pending.presented_for_mode = presented_for_mode;
	} else if (fsout->surface) {
		/* we clear immediately */
		wl_list_remove(&fsout->surface_destroyed.link);

		weston_view_destroy(fsout->view);
		fsout->view = NULL;

		if (wl_list_empty(&fsout->surface->views)) {
			fsout->surface->committed = NULL;
			fsout->surface->committed_private = NULL;
		}

		fsout->surface = NULL;

		weston_output_schedule_repaint(fsout->output);
	}
}

static void
fullscreen_shell_release(struct wl_client *client,
			 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
fullscreen_shell_present_surface(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_resource *surface_res,
				 uint32_t method,
				 struct wl_resource *output_res)
{
	struct fullscreen_shell *shell =
		wl_resource_get_user_data(resource);
	struct weston_output *output;
	struct weston_surface *surface;
	struct weston_seat *seat;
	struct fs_output *fsout;

	surface = surface_res ? wl_resource_get_user_data(surface_res) : NULL;

	switch(method) {
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT:
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_CENTER:
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_ZOOM:
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_ZOOM_CROP:
	case ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_STRETCH:
		break;
	default:
		wl_resource_post_error(resource,
				       ZWP_FULLSCREEN_SHELL_V1_ERROR_INVALID_METHOD,
				       "Invalid presentation method");
	}

	if (output_res) {
		output = weston_head_from_resource(output_res)->output;
		fsout = fs_output_for_output(output);
		fs_output_set_surface(fsout, surface, method, 0, 0);
	} else {
		replace_default_surface(shell, surface, method);

		wl_list_for_each(fsout, &shell->output_list, link)
			fs_output_set_surface(fsout, surface, method, 0, 0);
	}

	if (surface) {
		wl_list_for_each(seat, &shell->compositor->seat_list, link) {
			struct weston_keyboard *keyboard =
				weston_seat_get_keyboard(seat);

			if (keyboard && !keyboard->focus)
				weston_seat_set_keyboard_focus(seat, surface);
		}
	}
}

static void
mode_feedback_destroyed(struct wl_resource *resource)
{
	struct fs_output *fsout = wl_resource_get_user_data(resource);

	fsout->pending.mode_feedback = NULL;
}

static void
fullscreen_shell_present_surface_for_mode(struct wl_client *client,
					  struct wl_resource *resource,
					  struct wl_resource *surface_res,
					  struct wl_resource *output_res,
					  int32_t framerate,
					  uint32_t feedback_id)
{
	struct fullscreen_shell *shell =
		wl_resource_get_user_data(resource);
	struct weston_output *output;
	struct weston_surface *surface;
	struct weston_seat *seat;
	struct fs_output *fsout;

	output = weston_head_from_resource(output_res)->output;
	fsout = fs_output_for_output(output);

	if (surface_res == NULL) {
		fs_output_set_surface(fsout, NULL, 0, 0, 0);
		return;
	}

	surface = wl_resource_get_user_data(surface_res);
	fs_output_set_surface(fsout, surface, 0, framerate, 1);

	fsout->pending.mode_feedback =
		wl_resource_create(client,
				   &zwp_fullscreen_shell_mode_feedback_v1_interface,
				   1, feedback_id);
	wl_resource_set_implementation(fsout->pending.mode_feedback, NULL,
				       fsout, mode_feedback_destroyed);

	wl_list_for_each(seat, &shell->compositor->seat_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);

		if (keyboard && !keyboard->focus)
			weston_seat_set_keyboard_focus(seat, surface);
	}
}

struct zwp_fullscreen_shell_v1_interface fullscreen_shell_implementation = {
	fullscreen_shell_release,
	fullscreen_shell_present_surface,
	fullscreen_shell_present_surface_for_mode,
};

static void
output_created(struct wl_listener *listener, void *data)
{
	struct fullscreen_shell *shell;

	shell = container_of(listener, struct fullscreen_shell,
			     output_created_listener);

	fs_output_create(shell, data);
}

static void
client_destroyed(struct wl_listener *listener, void *data)
{
	struct fullscreen_shell *shell = container_of(listener,
						     struct fullscreen_shell,
						     client_destroyed);
	shell->client = NULL;
}

static void
bind_fullscreen_shell(struct wl_client *client, void *data, uint32_t version,
		       uint32_t id)
{
	struct fullscreen_shell *shell = data;
	struct wl_resource *resource;

	if (shell->client != NULL && shell->client != client)
		return;
	else if (shell->client == NULL) {
		shell->client = client;
		wl_client_add_destroy_listener(client, &shell->client_destroyed);
	}

	resource = wl_resource_create(client,
				      &zwp_fullscreen_shell_v1_interface,
				      1, id);
	wl_resource_set_implementation(resource,
				       &fullscreen_shell_implementation,
				       shell, NULL);

	if (shell->compositor->capabilities & WESTON_CAP_CURSOR_PLANE)
		zwp_fullscreen_shell_v1_send_capability(resource,
			ZWP_FULLSCREEN_SHELL_V1_CAPABILITY_CURSOR_PLANE);

	if (shell->compositor->capabilities & WESTON_CAP_ARBITRARY_MODES)
		zwp_fullscreen_shell_v1_send_capability(resource,
			ZWP_FULLSCREEN_SHELL_V1_CAPABILITY_ARBITRARY_MODES);
}

WL_EXPORT int
wet_shell_init(struct weston_compositor *compositor,
	       int *argc, char *argv[])
{
	struct fullscreen_shell *shell;
	struct weston_seat *seat;
	struct weston_output *output;

	shell = zalloc(sizeof *shell);
	if (shell == NULL)
		return -1;

	shell->compositor = compositor;
	wl_list_init(&shell->default_surface_list);

	shell->client_destroyed.notify = client_destroyed;

	weston_layer_init(&shell->layer, compositor);
	weston_layer_set_position(&shell->layer,
				  WESTON_LAYER_POSITION_FULLSCREEN);

	wl_list_init(&shell->output_list);
	shell->output_created_listener.notify = output_created;
	wl_signal_add(&compositor->output_created_signal,
		      &shell->output_created_listener);
	wl_list_for_each(output, &compositor->output_list, link)
		fs_output_create(shell, output);

	shell->seat_created_listener.notify = seat_created;
	wl_signal_add(&compositor->seat_created_signal,
		      &shell->seat_created_listener);
	wl_list_for_each(seat, &compositor->seat_list, link)
		seat_created(&shell->seat_created_listener, seat);

	wl_global_create(compositor->wl_display,
			 &zwp_fullscreen_shell_v1_interface, 1, shell,
			 bind_fullscreen_shell);

	return 0;
}
