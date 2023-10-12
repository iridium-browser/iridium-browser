/*
 * Copyright © 2010-2011 Intel Corporation
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2012-2018, 2021 Collabora, Ltd.
 * Copyright © 2017, 2018 General Electric Company
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <linux/input.h>
#include <dlfcn.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <drm_fourcc.h>

#include "timeline.h"

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include "linux-dmabuf.h"
#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include "viewporter-server-protocol.h"
#include "presentation-time-server-protocol.h"
#include "xdg-output-unstable-v1-server-protocol.h"
#include "linux-explicit-synchronization-unstable-v1-server-protocol.h"
#include "linux-explicit-synchronization.h"
#include "single-pixel-buffer-v1-server-protocol.h"
#include "shared/fd-util.h"
#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/string-helpers.h"
#include "shared/timespec-util.h"
#include "shared/signal.h"
#include "shared/xalloc.h"
#include "tearing-control-v1-server-protocol.h"
#include "git-version.h"
#include <libweston/version.h>
#include <libweston/plugin-registry.h>
#include "pixel-formats.h"
#include "backend.h"
#include "libweston-internal.h"
#include "color.h"
#include "output-capture.h"
#include "pixman-renderer.h"
#include "renderer-gl/gl-renderer.h"

#include "weston-log-internal.h"

/**
 * \defgroup head Head
 * \defgroup output Output
 * \defgroup compositor Compositor
 */

#define DEFAULT_REPAINT_WINDOW 7 /* milliseconds */

static void
weston_output_transform_scale_init(struct weston_output *output,
				   uint32_t transform, uint32_t scale);

static void
weston_compositor_build_view_list(struct weston_compositor *compositor,
				  struct weston_output *output);

static char *
weston_output_create_heads_string(struct weston_output *output);

static void
subsurface_committed(struct weston_surface *surface,
		     struct weston_coord_surface new_origin);

static void
weston_view_dirty_paint_nodes(struct weston_view *view)
{
	struct weston_paint_node *node;

	wl_list_for_each(node, &view->paint_node_list, view_link) {
		assert(node->surface == view->surface);

		node->status |= PAINT_NODE_VIEW_DIRTY;
	}
}

static void
weston_surface_dirty_paint_nodes(struct weston_surface *surface)
{
	struct weston_paint_node *node;

	wl_list_for_each(node, &surface->paint_node_list, surface_link) {
		assert(node->surface == surface);

		node->status |= PAINT_NODE_VIEW_DIRTY;
	}
}

static void
weston_output_dirty_paint_nodes(struct weston_output *output)
{
	struct weston_paint_node *node;

	wl_list_for_each(node, &output->paint_node_list, output_link) {
		assert(node->output == output);

		node->status |= PAINT_NODE_OUTPUT_DIRTY;
	}
}

static void
paint_node_update(struct weston_paint_node *pnode)
{
	struct weston_matrix *mat = &pnode->buffer_to_output_matrix;
	bool view_dirty = pnode->status & PAINT_NODE_VIEW_DIRTY;
	bool output_dirty = pnode->status & PAINT_NODE_OUTPUT_DIRTY;

	if (view_dirty || output_dirty) {
		weston_view_buffer_to_output_matrix(pnode->view,
						    pnode->output, mat);
		weston_matrix_invert(&pnode->output_to_buffer_matrix, mat);
		pnode->needs_filtering = weston_matrix_needs_filtering(mat);

		pnode->valid_transform = weston_matrix_to_transform(mat,
								    &pnode->transform);
	}

	pnode->status = PAINT_NODE_CLEAN;

}

static struct weston_paint_node *
weston_paint_node_create(struct weston_surface *surface,
			 struct weston_view *view,
			 struct weston_output *output)
{
	struct weston_paint_node *pnode;
	struct weston_paint_node *existing_node;

	assert(view->surface == surface);

	pnode = zalloc(sizeof *pnode);
	if (!pnode)
		return NULL;

	/*
	 * Invariant: all paint nodes with the same surface+output have the
	 * same surf_xform state.
	 */
	wl_list_for_each(existing_node, &surface->paint_node_list, surface_link) {
		assert(existing_node->surface == surface);
		if (existing_node->output != output)
			continue;

		weston_surface_color_transform_copy(&pnode->surf_xform,
						    &existing_node->surf_xform);
		pnode->surf_xform_valid = existing_node->surf_xform_valid;
		break;
	}

	pnode->surface = surface;
	wl_list_insert(&surface->paint_node_list, &pnode->surface_link);

	pnode->view = view;
	wl_list_insert(&view->paint_node_list, &pnode->view_link);

	pnode->output = output;
	wl_list_insert(&output->paint_node_list, &pnode->output_link);

	wl_list_init(&pnode->z_order_link);

	pnode->status = PAINT_NODE_ALL_DIRTY;
	paint_node_update(pnode);

	return pnode;
}

static void
weston_paint_node_destroy(struct weston_paint_node *pnode)
{
	assert(pnode->view->surface == pnode->surface);
	wl_list_remove(&pnode->surface_link);
	wl_list_remove(&pnode->view_link);
	wl_list_remove(&pnode->output_link);
	wl_list_remove(&pnode->z_order_link);
	assert(pnode->surf_xform_valid || !pnode->surf_xform.transform);
	weston_surface_color_transform_fini(&pnode->surf_xform);
	free(pnode);
}

/** Send wl_output events for mode and scale changes
 *
 * \param head Send on all resources bound to this head.
 * \param mode_changed If true, send the current mode.
 * \param scale_changed If true, send the current scale.
 */
static void
weston_mode_switch_send_events(struct weston_head *head,
			       bool mode_changed, bool scale_changed)
{
	struct weston_output *output = head->output;
	struct wl_resource *resource;
	int version;

	wl_resource_for_each(resource, &head->resource_list) {
		if (mode_changed) {
			wl_output_send_mode(resource,
					    output->current_mode->flags,
					    output->current_mode->width,
					    output->current_mode->height,
					    output->current_mode->refresh);
		}

		version = wl_resource_get_version(resource);
		if (version >= WL_OUTPUT_SCALE_SINCE_VERSION && scale_changed)
			wl_output_send_scale(resource, output->current_scale);

		if (version >= WL_OUTPUT_NAME_SINCE_VERSION)
			wl_output_send_name(resource, head->name);

		if (version >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION)
			wl_output_send_description(resource, head->model);

		if (version >= WL_OUTPUT_DONE_SINCE_VERSION)
			wl_output_send_done(resource);
	}
	wl_resource_for_each(resource, &head->xdg_output_resource_list) {
		zxdg_output_v1_send_logical_position(resource,
						     output->x,
						     output->y);
		zxdg_output_v1_send_logical_size(resource,
						 output->width,
						 output->height);
		zxdg_output_v1_send_done(resource);
	}
}

WL_EXPORT bool
weston_output_contains_point(struct weston_output *output,
			     int32_t x, int32_t y)
{
	return pixman_region32_contains_point(&output->region, x, y, NULL);
}

static void
weston_mode_switch_finish(struct weston_output *output,
			  int mode_changed, int scale_changed)
{
	struct weston_seat *seat;
	struct weston_head *head;
	pixman_region32_t old_output_region;

	pixman_region32_init(&old_output_region);
	pixman_region32_copy(&old_output_region, &output->region);

	/* Update output region and transformation matrix */
	weston_output_transform_scale_init(output, output->transform, output->current_scale);

	pixman_region32_init_rect(&output->region, output->x, output->y,
				  output->width, output->height);

	weston_output_update_matrix(output);

	/* If a pointer falls outside the outputs new geometry, move it to its
	 * lower-right corner */
	wl_list_for_each(seat, &output->compositor->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);
		int32_t x, y;

		if (!pointer)
			continue;

		x = pointer->pos.c.x;
		y = pointer->pos.c.y;
		if (!pixman_region32_contains_point(&old_output_region,
						    x, y, NULL) ||
		    weston_output_contains_point(output, x, y))
			continue;

		if (x >= output->x + output->width)
			x = output->x + output->width - 1;
		if (y >= output->y + output->height)
			y = output->y + output->height - 1;

		pointer->pos.c = weston_coord(x, y);
	}

	pixman_region32_fini(&old_output_region);

	if (!mode_changed && !scale_changed)
		return;

	weston_output_damage(output);

	/* notify clients of the changes */
	wl_list_for_each(head, &output->head_list, output_link)
		weston_mode_switch_send_events(head,
					       mode_changed, scale_changed);
}

static void
weston_compositor_reflow_outputs(struct weston_compositor *compositor,
				struct weston_output *resized_output, int delta_width);

/**
 * \ingroup output
 */
WL_EXPORT int
weston_output_mode_set_native(struct weston_output *output,
			      struct weston_mode *mode,
			      int32_t scale)
{
	int ret;
	int mode_changed = 0, scale_changed = 0;
	int32_t old_width;

	if (!output->switch_mode)
		return -1;

	if (!output->original_mode) {
		mode_changed = 1;
		ret = output->switch_mode(output, mode);
		if (ret < 0)
			return ret;
		if (output->current_scale != scale) {
			scale_changed = 1;
			output->current_scale = scale;
		}
	}

	old_width = output->width;
	output->native_mode = mode;
	output->native_scale = scale;

	weston_mode_switch_finish(output, mode_changed, scale_changed);

	if (mode_changed || scale_changed) {
		weston_compositor_reflow_outputs(output->compositor, output, output->width - old_width);

		wl_signal_emit(&output->compositor->output_resized_signal, output);
	}
	return 0;
}

/**
 * \ingroup output
 */
WL_EXPORT int
weston_output_mode_switch_to_native(struct weston_output *output)
{
	int ret;
	int mode_changed = 0, scale_changed = 0;

	if (!output->switch_mode)
		return -1;

	if (!output->original_mode) {
		weston_log("already in the native mode\n");
		return -1;
	}
	/* the non fullscreen clients haven't seen a mode set since we
	 * switched into a temporary, so we need to notify them if the
	 * mode at that time is different from the native mode now.
	 */
	mode_changed = (output->original_mode != output->native_mode);
	scale_changed = (output->original_scale != output->native_scale);

	ret = output->switch_mode(output, output->native_mode);
	if (ret < 0)
		return ret;

	output->current_scale = output->native_scale;

	output->original_mode = NULL;
	output->original_scale = 0;

	weston_mode_switch_finish(output, mode_changed, scale_changed);

	return 0;
}

/**
 * \ingroup output
 */
WL_EXPORT int
weston_output_mode_switch_to_temporary(struct weston_output *output,
				       struct weston_mode *mode,
				       int32_t scale)
{
	int ret;

	if (!output->switch_mode)
		return -1;

	/* original_mode is the last mode non full screen clients have seen,
	 * so we shouldn't change it if we already have one set.
	 */
	if (!output->original_mode) {
		output->original_mode = output->native_mode;
		output->original_scale = output->native_scale;
	}
	ret = output->switch_mode(output, mode);
	if (ret < 0)
		return ret;

	output->current_scale = scale;

	weston_mode_switch_finish(output, 0, 0);

	return 0;
}

static void
region_init_infinite(pixman_region32_t *region)
{
	pixman_region32_init_rect(region, INT32_MIN, INT32_MIN,
				  UINT32_MAX, UINT32_MAX);
}

static struct weston_subsurface *
weston_surface_to_subsurface(struct weston_surface *surface);

WL_EXPORT struct weston_view *
weston_view_create(struct weston_surface *surface)
{
	struct weston_view *view;

	view = zalloc(sizeof *view);
	if (view == NULL)
		return NULL;

	view->surface = surface;
	view->plane = &surface->compositor->primary_plane;

	/* Assign to surface */
	wl_list_insert(&surface->views, &view->surface_link);

	wl_signal_init(&view->destroy_signal);
	wl_signal_init(&view->unmap_signal);
	wl_list_init(&view->link);
	wl_list_init(&view->layer_link.link);
	wl_list_init(&view->paint_node_list);

	pixman_region32_init(&view->clip);

	view->alpha = 1.0;
	pixman_region32_init(&view->transform.opaque);

	wl_list_init(&view->geometry.transformation_list);
	wl_list_insert(&view->geometry.transformation_list,
		       &view->transform.position.link);
	weston_matrix_init(&view->transform.position.matrix);
	wl_list_init(&view->geometry.child_list);
	pixman_region32_init(&view->geometry.scissor);
	pixman_region32_init(&view->transform.boundingbox);
	view->transform.dirty = 1;
	weston_view_update_transform(view);

	return view;
}

struct weston_presentation_feedback {
	struct wl_resource *resource;

	/* XXX: could use just wl_resource_get_link() instead */
	struct wl_list link;

	/* The per-surface feedback flags */
	uint32_t psf_flags;
};

static void
weston_presentation_feedback_discard(
		struct weston_presentation_feedback *feedback)
{
	wp_presentation_feedback_send_discarded(feedback->resource);
	wl_resource_destroy(feedback->resource);
}

static void
weston_presentation_feedback_discard_list(struct wl_list *list)
{
	struct weston_presentation_feedback *feedback, *tmp;

	wl_list_for_each_safe(feedback, tmp, list, link)
		weston_presentation_feedback_discard(feedback);
}

static void
weston_presentation_feedback_present(
		struct weston_presentation_feedback *feedback,
		struct weston_output *output,
		uint32_t refresh_nsec,
		const struct timespec *ts,
		uint64_t seq,
		uint32_t flags)
{
	struct wl_client *client = wl_resource_get_client(feedback->resource);
	struct weston_head *head;
	struct wl_resource *o;
	uint32_t tv_sec_hi;
	uint32_t tv_sec_lo;
	uint32_t tv_nsec;
	bool done = false;

	wl_list_for_each(head, &output->head_list, output_link) {
		wl_resource_for_each(o, &head->resource_list) {
			if (wl_resource_get_client(o) != client)
				continue;

			wp_presentation_feedback_send_sync_output(feedback->resource, o);
			done = true;
		}

		/* For clone mode, send it for just one wl_output global,
		 * they are all equivalent anyway.
		 */
		if (done)
			break;
	}

	timespec_to_proto(ts, &tv_sec_hi, &tv_sec_lo, &tv_nsec);
	wp_presentation_feedback_send_presented(feedback->resource,
						tv_sec_hi, tv_sec_lo, tv_nsec,
						refresh_nsec,
						seq >> 32, seq & 0xffffffff,
						flags | feedback->psf_flags);
	wl_resource_destroy(feedback->resource);
}

static void
weston_presentation_feedback_present_list(struct wl_list *list,
					  struct weston_output *output,
					  uint32_t refresh_nsec,
					  const struct timespec *ts,
					  uint64_t seq,
					  uint32_t flags)
{
	struct weston_presentation_feedback *feedback, *tmp;

	assert(!(flags & WP_PRESENTATION_FEEDBACK_INVALID) ||
	       wl_list_empty(list));

	wl_list_for_each_safe(feedback, tmp, list, link)
		weston_presentation_feedback_present(feedback, output,
						     refresh_nsec, ts, seq,
						     flags);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct weston_surface_state *state =
		container_of(listener, struct weston_surface_state,
			     buffer_destroy_listener);

	state->buffer = NULL;
}

static void
weston_surface_state_init(struct weston_surface_state *state)
{
	state->newly_attached = 0;
	state->buffer = NULL;
	state->buffer_destroy_listener.notify =
		surface_state_handle_buffer_destroy;
	state->sx = 0;
	state->sy = 0;

	pixman_region32_init(&state->damage_surface);
	pixman_region32_init(&state->damage_buffer);
	pixman_region32_init(&state->opaque);
	region_init_infinite(&state->input);

	wl_list_init(&state->frame_callback_list);
	wl_list_init(&state->feedback_list);

	state->buffer_viewport.buffer.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	state->buffer_viewport.buffer.scale = 1;
	state->buffer_viewport.buffer.src_width = wl_fixed_from_int(-1);
	state->buffer_viewport.surface.width = -1;
	state->buffer_viewport.changed = 0;

	state->acquire_fence_fd = -1;

	state->desired_protection = WESTON_HDCP_DISABLE;
	state->protection_mode = WESTON_SURFACE_PROTECTION_MODE_RELAXED;
}

static void
weston_surface_state_fini(struct weston_surface_state *state)
{
	struct wl_resource *cb, *next;

	wl_resource_for_each_safe(cb, next, &state->frame_callback_list)
		wl_resource_destroy(cb);

	weston_presentation_feedback_discard_list(&state->feedback_list);

	pixman_region32_fini(&state->input);
	pixman_region32_fini(&state->opaque);
	pixman_region32_fini(&state->damage_surface);
	pixman_region32_fini(&state->damage_buffer);

	if (state->buffer)
		wl_list_remove(&state->buffer_destroy_listener.link);
	state->buffer = NULL;

	fd_clear(&state->acquire_fence_fd);
	weston_buffer_release_reference(&state->buffer_release_ref, NULL);
}

static void
weston_surface_state_set_buffer(struct weston_surface_state *state,
				struct weston_buffer *buffer)
{
	if (state->buffer == buffer)
		return;

	if (state->buffer)
		wl_list_remove(&state->buffer_destroy_listener.link);
	state->buffer = buffer;
	if (state->buffer)
		wl_signal_add(&state->buffer->destroy_signal,
			      &state->buffer_destroy_listener);
}

WL_EXPORT struct weston_surface *
weston_surface_create(struct weston_compositor *compositor)
{
	struct weston_surface *surface;

	surface = zalloc(sizeof *surface);
	if (surface == NULL)
		return NULL;

	wl_signal_init(&surface->destroy_signal);
	wl_signal_init(&surface->commit_signal);

	surface->compositor = compositor;
	surface->ref_count = 1;

	surface->buffer_viewport.buffer.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	surface->buffer_viewport.buffer.scale = 1;
	surface->buffer_viewport.buffer.src_width = wl_fixed_from_int(-1);
	surface->buffer_viewport.surface.width = -1;

	weston_surface_state_init(&surface->pending);

	pixman_region32_init(&surface->damage);
	pixman_region32_init(&surface->opaque);
	region_init_infinite(&surface->input);

	wl_list_init(&surface->views);
	wl_list_init(&surface->paint_node_list);

	wl_list_init(&surface->frame_callback_list);
	wl_list_init(&surface->feedback_list);

	wl_list_init(&surface->subsurface_list);
	wl_list_init(&surface->subsurface_list_pending);

	weston_matrix_init(&surface->buffer_to_surface_matrix);
	weston_matrix_init(&surface->surface_to_buffer_matrix);

	wl_list_init(&surface->pointer_constraints);

	surface->acquire_fence_fd = -1;

	surface->desired_protection = WESTON_HDCP_DISABLE;
	surface->current_protection = WESTON_HDCP_DISABLE;
	surface->protection_mode = WESTON_SURFACE_PROTECTION_MODE_RELAXED;

	return surface;
}

WL_EXPORT struct weston_coord_global
weston_coord_surface_to_global(const struct weston_view *view,
			       struct weston_coord_surface coord)
{
	struct weston_coord_global out;

	assert(!view->transform.dirty);
	assert(view->surface == coord.coordinate_space_id);

	out.c = weston_matrix_transform_coord(&view->transform.matrix,
					      coord.c);
	return out;
}

WL_EXPORT struct weston_coord_surface
weston_coord_global_to_surface(const struct weston_view *view,
			       struct weston_coord_global coord)
{
	struct weston_coord_surface out;

	assert(!view->transform.dirty);
	out.c = weston_matrix_transform_coord(&view->transform.inverse,
					      coord.c);
	out.coordinate_space_id = view->surface;
	return out;
}

WL_EXPORT struct weston_coord_buffer
weston_coord_surface_to_buffer(const struct weston_surface *surface,
			       struct weston_coord_surface coord)
{
	struct weston_coord_buffer tmp;

	assert(surface == coord.coordinate_space_id);

	tmp.c = weston_matrix_transform_coord(&surface->surface_to_buffer_matrix,
					      coord.c);
	return tmp;
}

WL_EXPORT pixman_box32_t
weston_matrix_transform_rect(struct weston_matrix *matrix,
			     pixman_box32_t rect)
{
	int i;
	pixman_box32_t out;

	/* since pixman regions are defined by two corners we have
	 * to be careful with rotations that aren't multiples of 90.
	 * We need to take all four corners of the region and rotate
	 * them, then construct the largest possible two corner
	 * rectangle from the result.
	 */
	struct weston_coord corners[4] = {
		weston_coord(rect.x1, rect.y1),
		weston_coord(rect.x2, rect.y1),
		weston_coord(rect.x1, rect.y2),
		weston_coord(rect.x2, rect.y2),
	};

	for (i = 0; i < 4; i++)
		corners[i] = weston_matrix_transform_coord(matrix, corners[i]);

	out.x1 = floor(corners[0].x);
	out.y1 = floor(corners[0].y);
	out.x2 = ceil(corners[0].x);
	out.y2 = ceil(corners[0].y);

	for (i = 1; i < 4; i++) {
		if (floor(corners[i].x) < out.x1)
			out.x1 = floor(corners[i].x);
		if (floor(corners[i].y) < out.y1)
			out.y1 = floor(corners[i].y);
		if (ceil(corners[i].x) > out.x2)
			out.x2 = ceil(corners[i].x);
		if (ceil(corners[i].y) > out.y2)
			out.y2 = ceil(corners[i].y);
	}
	return out;
}

/** Transform a region by a matrix
 *
 * Warning: This function does not work perfectly for projective,
 * affine, or matrices that encode arbitrary rotations. Only 90-degree
 * step rotations are exact.
 *
 * More complicated matrices result in some expansion.
 */
WL_EXPORT void
weston_matrix_transform_region(pixman_region32_t *dest,
			       struct weston_matrix *matrix,
			       pixman_region32_t *src)
{
	pixman_box32_t *src_rects, *dest_rects;
	int nrects, i;

	src_rects = pixman_region32_rectangles(src, &nrects);
	dest_rects = malloc(nrects * sizeof(*dest_rects));
	if (!dest_rects)
		return;

	for (i = 0; i < nrects; i++)
		dest_rects[i] = weston_matrix_transform_rect(matrix, src_rects[i]);

	pixman_region32_clear(dest);
	pixman_region32_init_rects(dest, dest_rects, nrects);
	free(dest_rects);
}

/** Transform a rectangle from surface coordinates to buffer coordinates
 *
 * \param surface The surface to fetch wp_viewport and buffer transformation
 * from.
 * \param rect The rectangle to transform.
 * \return The transformed rectangle.
 *
 * Viewport and buffer transformations can only do translation, scaling,
 * and rotations in 90-degree steps. Therefore the only loss in the
 * conversion is coordinate rounding.
 *
 * However, some coordinate rounding takes place as an intermediate
 * step before the buffer scale factor is applied, so the rectangle
 * boundary may not be exactly as expected.
 *
 * This is OK for damage tracking since a little extra coverage is
 * not a problem.
 */
WL_EXPORT pixman_box32_t
weston_surface_to_buffer_rect(struct weston_surface *surface,
			      pixman_box32_t rect)
{
	return weston_matrix_transform_rect(&surface->surface_to_buffer_matrix,
					   rect);
}

/** Transform a region from surface coordinates to buffer coordinates
 *
 * \param surface The surface to fetch wp_viewport and buffer transformation
 * from.
 * \param[in] surface_region The region in surface coordinates.
 * \param[out] buffer_region The region converted to buffer coordinates.
 *
 * Buffer_region must be init'd, but will be completely overwritten.
 *
 * Viewport and buffer transformations can only do translation, scaling,
 * and rotations in 90-degree steps. Therefore the only loss in the
 * conversion is from the coordinate rounding that takes place in
 * \ref weston_surface_to_buffer_rect.
 *
 */
WL_EXPORT void
weston_surface_to_buffer_region(struct weston_surface *surface,
				pixman_region32_t *surface_region,
				pixman_region32_t *buffer_region)
{
	pixman_box32_t *src_rects, *dest_rects;
	int nrects, i;

	src_rects = pixman_region32_rectangles(surface_region, &nrects);
	dest_rects = malloc(nrects * sizeof(*dest_rects));
	if (!dest_rects)
		return;

	for (i = 0; i < nrects; i++) {
		dest_rects[i] = weston_surface_to_buffer_rect(surface,
							      src_rects[i]);
	}

	pixman_region32_fini(buffer_region);
	pixman_region32_init_rects(buffer_region, dest_rects, nrects);
	free(dest_rects);
}

WL_EXPORT void
weston_view_buffer_to_output_matrix(const struct weston_view *view,
				    const struct weston_output *output,
				    struct weston_matrix *matrix)
{
	*matrix = view->surface->buffer_to_surface_matrix;
	weston_matrix_multiply(matrix, &view->transform.matrix);
	weston_matrix_multiply(matrix, &output->matrix);
}

WL_EXPORT void
weston_view_move_to_plane(struct weston_view *view,
			     struct weston_plane *plane)
{
	if (view->plane == plane)
		return;

	weston_view_damage_below(view);
	view->plane = plane;
	weston_surface_damage(view->surface);
}

/** Inflict damage on the plane where the view is visible.
 *
 * \param view The view that causes the damage.
 *
 * If the view is currently on a plane (including the primary plane),
 * take the view's boundingbox, subtract all the opaque views that cover it,
 * and add the remaining region as damage to the plane. This corresponds
 * to the damage inflicted to the plane if this view disappeared.
 *
 * A repaint is scheduled for this view.
 *
 * The region of all opaque views covering this view is stored in
 * weston_view::clip and updated by view_accumulate_damage() during
 * weston_output_repaint(). Specifically, that region matches the
 * scenegraph as it was last painted.
 */
WL_EXPORT void
weston_view_damage_below(struct weston_view *view)
{
	pixman_region32_t damage;

	pixman_region32_init(&damage);
	pixman_region32_subtract(&damage, &view->transform.boundingbox,
				 &view->clip);
	if (view->plane)
		pixman_region32_union(&view->plane->damage,
				      &view->plane->damage, &damage);
	pixman_region32_fini(&damage);
	weston_view_schedule_repaint(view);
}

/** Send wl_surface.enter/leave events
 *
 * \param surface The surface.
 * \param head A head of the entered/left output.
 * \param enter True if entered.
 * \param leave True if left.
 *
 * Send the enter/leave events for all protocol objects bound to the given
 * output by the client owning the surface.
 */
static void
weston_surface_send_enter_leave(struct weston_surface *surface,
				struct weston_head *head,
				bool enter,
				bool leave)
{
	struct wl_resource *wloutput;
	struct wl_client *client;

	assert(enter != leave);

	client = wl_resource_get_client(surface->resource);
	wl_resource_for_each(wloutput, &head->resource_list) {
		if (wl_resource_get_client(wloutput) != client)
			continue;

		if (enter)
			wl_surface_send_enter(surface->resource, wloutput);
		if (leave)
			wl_surface_send_leave(surface->resource, wloutput);
	}
}

static void
weston_surface_compute_protection(struct protected_surface *psurface)
{
	enum weston_hdcp_protection min_protection;
	bool min_protection_valid = false;
	struct weston_surface *surface = psurface->surface;
	struct weston_output *output;

	wl_list_for_each(output, &surface->compositor->output_list, link)
		if (surface->output_mask & (1u << output->id)) {
			/*
			 * If the content-protection is enabled with protection
			 * mode as RELAXED for a surface, and if
			 * content-recording features like: screen-shooter,
			 * recorder, screen-sharing, etc are on, then notify the
			 * client, that the protection is disabled.
			 *
			 * Note: If the protection mode is ENFORCED then there
			 * is no need to bother the client as the renderer takes
			 * care of censoring the visibility of the protected
			 * content.
			 */

			if (output->disable_planes > 0 &&
			    surface->protection_mode == WESTON_SURFACE_PROTECTION_MODE_RELAXED) {
				min_protection = WESTON_HDCP_DISABLE;
				min_protection_valid = true;
				break;
			}
			if (!min_protection_valid) {
				min_protection = output->current_protection;
				min_protection_valid = true;
			}
			if (output->current_protection < min_protection)
				min_protection = output->current_protection;
		}
	if (!min_protection_valid)
		min_protection = WESTON_HDCP_DISABLE;

	surface->current_protection = min_protection;

	weston_protected_surface_send_event(psurface, surface->current_protection);
}

static void
notify_surface_protection_change(void *data)
{
	struct weston_compositor *compositor = data;
	struct content_protection *cp;
	struct protected_surface *psurface;

	cp = compositor->content_protection;
	cp->surface_protection_update = NULL;

	/* Notify the clients, whose surfaces are changed */
	wl_list_for_each(psurface, &cp->protected_list, link)
		if (psurface && psurface->surface)
			weston_surface_compute_protection(psurface);
}

/**
 * \param compositor weston_compositor
 *
 * Schedule an idle task to notify surface about the update in protection,
 * if not already scheduled.
 */
static void
weston_schedule_surface_protection_update(struct weston_compositor *compositor)
{
	struct content_protection *cp = compositor->content_protection;
	struct wl_event_loop *loop;

	if (!cp || cp->surface_protection_update)
		return;
	loop = wl_display_get_event_loop(compositor->wl_display);
	cp->surface_protection_update = wl_event_loop_add_idle(loop,
					       notify_surface_protection_change,
					       compositor);
}

/**
 * \param es    The surface
 * \param mask  The new set of outputs for the surface
 *
 * Sets the surface's set of outputs to the ones specified by
 * the new output mask provided.  Identifies the outputs that
 * have changed, the posts enter and leave events for these
 * outputs as appropriate.
 */
static void
weston_surface_update_output_mask(struct weston_surface *es, uint32_t mask)
{
	uint32_t different = es->output_mask ^ mask;
	uint32_t entered = mask & different;
	uint32_t left = es->output_mask & different;
	uint32_t output_bit;
	struct weston_output *output;
	struct weston_head *head;

	es->output_mask = mask;
	if (es->resource == NULL)
		return;
	if (different == 0)
		return;

	wl_list_for_each(output, &es->compositor->output_list, link) {
		output_bit = 1u << output->id;
		if (!(output_bit & different))
			continue;

		wl_list_for_each(head, &output->head_list, output_link) {
			weston_surface_send_enter_leave(es, head,
							output_bit & entered,
							output_bit & left);
		}
	}
	/*
	 * Change in surfaces' output mask might trigger a change in its
	 * protection.
	 */
	weston_schedule_surface_protection_update(es->compositor);
}

static void
notify_view_output_destroy(struct wl_listener *listener, void *data)
{
	struct weston_view *view =
		container_of(listener,
		     struct weston_view, output_destroy_listener);

	view->output = NULL;
	view->output_destroy_listener.notify = NULL;
}

/** Set the primary output of the view
 *
 * \param view The view whose primary output to set
 * \param output The new primary output for the view
 *
 * Set \a output to be the primary output of the \a view.
 *
 * Notice that the assignment may be temporary; the primary output could be
 * automatically changed. Hence, one cannot rely on the value persisting.
 *
 * Passing NULL as /a output will set the primary output to NULL.
 */
WL_EXPORT void
weston_view_set_output(struct weston_view *view, struct weston_output *output)
{
	if (view->output_destroy_listener.notify) {
		wl_list_remove(&view->output_destroy_listener.link);
		view->output_destroy_listener.notify = NULL;
	}
	view->output = output;
	if (output) {
		view->output_destroy_listener.notify =
			notify_view_output_destroy;
		wl_signal_add(&output->destroy_signal,
			      &view->output_destroy_listener);
	}
}

static struct weston_layer *
get_view_layer(struct weston_view *view)
{
	if (view->parent_view)
		return get_view_layer(view->parent_view);
	return view->layer_link.layer;
}

/** Recalculate which output(s) the surface has views displayed on
 *
 * \param es  The surface to remap to outputs
 *
 * Finds the output that is showing the largest amount of one
 * of the surface's various views.  This output becomes the
 * surface's primary output for vsync and frame callback purposes.
 *
 * Also notes all outputs of all of the surface's views
 * in the output_mask for the surface.
 */
static void
weston_surface_assign_output(struct weston_surface *es)
{
	struct weston_output *new_output;
	struct weston_view *view;
	pixman_region32_t region;
	uint32_t max, area, mask;
	pixman_box32_t *e;

	new_output = NULL;
	max = 0;
	mask = 0;
	pixman_region32_init(&region);
	wl_list_for_each(view, &es->views, surface_link) {
		/* Only views that are visible on some layer participate in
		 * output_mask calculations. */
		if (!view->output || !get_view_layer(view))
			continue;

		pixman_region32_intersect(&region, &view->transform.boundingbox,
					  &view->output->region);

		e = pixman_region32_extents(&region);
		area = (e->x2 - e->x1) * (e->y2 - e->y1);

		mask |= view->output_mask;

		if (area >= max) {
			new_output = view->output;
			max = area;
		}
	}
	pixman_region32_fini(&region);

	es->output = new_output;
	weston_surface_update_output_mask(es, mask);
}

/** Recalculate which output(s) the view is displayed on
 *
 * \param ev  The view to remap to outputs
 *
 * Identifies the set of outputs that the view is visible on,
 * noting them into the output_mask.  The output that the view
 * is most visible on is set as the view's primary output.
 *
 * Also does the same for the view's surface.  See
 * weston_surface_assign_output().
 */
static void
weston_view_assign_output(struct weston_view *ev)
{
	struct weston_compositor *ec = ev->surface->compositor;
	struct weston_output *output, *new_output;
	pixman_region32_t region;
	uint32_t max, area, mask;
	pixman_box32_t *e;

	new_output = NULL;
	max = 0;
	mask = 0;
	pixman_region32_init(&region);
	wl_list_for_each(output, &ec->output_list, link) {
		if (output->destroying)
			continue;

		pixman_region32_intersect(&region, &ev->transform.boundingbox,
					  &output->region);

		e = pixman_region32_extents(&region);
		area = (e->x2 - e->x1) * (e->y2 - e->y1);

		if (area > 0)
			mask |= 1u << output->id;

		if (area >= max) {
			new_output = output;
			max = area;
		}
	}
	pixman_region32_fini(&region);

	weston_view_set_output(ev, new_output);
	ev->output_mask = mask;

	weston_surface_assign_output(ev->surface);
}

static void
weston_view_to_view_map(struct weston_view *from, struct weston_view *to,
			int from_x, int from_y, int *to_x, int *to_y)
{
	struct weston_coord_surface cs;
	struct weston_coord_global cg;

	cs = weston_coord_surface(from_x, from_y, from->surface);
	cg = weston_coord_surface_to_global(from, cs);
	cs = weston_coord_global_to_surface(to, cg);

	*to_x = round(cs.c.x);
	*to_y = round(cs.c.y);
}

static void
weston_view_transfer_scissor(struct weston_view *from, struct weston_view *to)
{
	pixman_box32_t *a;
	pixman_box32_t b;

	a = pixman_region32_extents(&from->geometry.scissor);

	weston_view_to_view_map(from, to, a->x1, a->y1, &b.x1, &b.y1);
	weston_view_to_view_map(from, to, a->x2, a->y2, &b.x2, &b.y2);

	pixman_region32_fini(&to->geometry.scissor);
	pixman_region32_init_with_extents(&to->geometry.scissor, &b);
}

static void
view_compute_bbox(struct weston_view *view, const pixman_box32_t *inbox,
		  pixman_region32_t *bbox)
{
	float min_x = HUGE_VALF,  min_y = HUGE_VALF;
	float max_x = -HUGE_VALF, max_y = -HUGE_VALF;
	int32_t s[4][2] = {
		{ inbox->x1, inbox->y1 },
		{ inbox->x1, inbox->y2 },
		{ inbox->x2, inbox->y1 },
		{ inbox->x2, inbox->y2 },
	};
	float int_x, int_y;
	int i;

	if (inbox->x1 == inbox->x2 || inbox->y1 == inbox->y2) {
		/* avoid rounding empty bbox to 1x1 */
		pixman_region32_init(bbox);
		return;
	}

	for (i = 0; i < 4; ++i) {
		struct weston_coord_surface cs;
		struct weston_coord_global cg;

		cs = weston_coord_surface(s[i][0], s[i][1],
					  view->surface);
		cg = weston_coord_surface_to_global(view, cs);
		if (cg.c.x < min_x)
			min_x = cg.c.x;
		if (cg.c.x > max_x)
			max_x = cg.c.x;
		if (cg.c.y < min_y)
			min_y = cg.c.y;
		if (cg.c.y > max_y)
			max_y = cg.c.y;
	}

	int_x = floorf(min_x);
	int_y = floorf(min_y);
	pixman_region32_init_rect(bbox, int_x, int_y,
				  ceilf(max_x) - int_x, ceilf(max_y) - int_y);
}

static void
weston_view_update_transform_scissor(struct weston_view *view,
				     pixman_region32_t *region)
{
	struct weston_view *parent = view->geometry.parent;

	if (parent) {
		if (parent->geometry.scissor_enabled) {
			view->geometry.scissor_enabled = true;
			weston_view_transfer_scissor(parent, view);
		} else {
			view->geometry.scissor_enabled = false;
		}
	}

	if (view->geometry.scissor_enabled)
		pixman_region32_intersect(region, region,
					  &view->geometry.scissor);
}
static void
weston_view_update_transform_disable(struct weston_view *view)
{
	view->transform.enabled = 0;

	/* round off fractions when not transformed */
	view->geometry.pos_offset.x = round(view->geometry.pos_offset.x);
	view->geometry.pos_offset.y = round(view->geometry.pos_offset.y);

	/* Otherwise identity matrix, but with x and y translation. */
	view->transform.position.matrix.type = WESTON_MATRIX_TRANSFORM_TRANSLATE;
	view->transform.position.matrix.d[12] = view->geometry.pos_offset.x;
	view->transform.position.matrix.d[13] = view->geometry.pos_offset.y;

	view->transform.matrix = view->transform.position.matrix;

	view->transform.inverse = view->transform.position.matrix;
	view->transform.inverse.d[12] = -view->geometry.pos_offset.x;
	view->transform.inverse.d[13] = -view->geometry.pos_offset.y;

	pixman_region32_init_rect(&view->transform.boundingbox,
				  0, 0,
				  view->surface->width,
				  view->surface->height);

	weston_view_update_transform_scissor(view, &view->transform.boundingbox);

	pixman_region32_translate(&view->transform.boundingbox,
				  view->geometry.pos_offset.x,
				  view->geometry.pos_offset.y);

	if (view->alpha == 1.0) {
		if (view->surface->is_opaque) {
			pixman_region32_copy(&view->transform.opaque,
					     &view->transform.boundingbox);
		} else {
			pixman_region32_copy(&view->transform.opaque,
					     &view->surface->opaque);
			if (view->geometry.scissor_enabled)
				pixman_region32_intersect(&view->transform.opaque,
							  &view->transform.opaque,
							  &view->geometry.scissor);
			pixman_region32_translate(&view->transform.opaque,
						  view->geometry.pos_offset.x,
						  view->geometry.pos_offset.y);
		}
	}
}

static int
weston_view_update_transform_enable(struct weston_view *view)
{
	struct weston_view *parent = view->geometry.parent;
	struct weston_matrix *matrix = &view->transform.matrix;
	struct weston_matrix *inverse = &view->transform.inverse;
	struct weston_transform *tform;
	pixman_region32_t surfregion;
	const pixman_box32_t *surfbox;

	view->transform.enabled = 1;

	/* Otherwise identity matrix, but with x and y translation. */
	view->transform.position.matrix.type = WESTON_MATRIX_TRANSFORM_TRANSLATE;
	view->transform.position.matrix.d[12] = view->geometry.pos_offset.x;
	view->transform.position.matrix.d[13] = view->geometry.pos_offset.y;

	weston_matrix_init(matrix);
	wl_list_for_each(tform, &view->geometry.transformation_list, link)
		weston_matrix_multiply(matrix, &tform->matrix);

	if (parent)
		weston_matrix_multiply(matrix, &parent->transform.matrix);

	if (weston_matrix_invert(inverse, matrix) < 0) {
		/* Oops, bad total transformation, not invertible */
		weston_log("error: weston_view %p"
			" transformation not invertible.\n", view);
		return -1;
	}

	pixman_region32_init_rect(&surfregion, 0, 0,
				  view->surface->width, view->surface->height);

	weston_view_update_transform_scissor(view, &surfregion);

	surfbox = pixman_region32_extents(&surfregion);

	view_compute_bbox(view, surfbox, &view->transform.boundingbox);

	if (view->alpha == 1.0 &&
	    matrix->type == WESTON_MATRIX_TRANSFORM_TRANSLATE) {
		if (view->surface->is_opaque) {
			pixman_region32_copy(&view->transform.opaque,
					     &view->transform.boundingbox);
		} else {
			pixman_region32_copy(&view->transform.opaque,
					     &view->surface->opaque);
			if (view->geometry.scissor_enabled)
				pixman_region32_intersect(&view->transform.opaque,
							  &view->transform.opaque,
							  &view->geometry.scissor);
			pixman_region32_translate(&view->transform.opaque,
						  matrix->d[12],
						  matrix->d[13]);
		}
	} else if (view->alpha == 1.0 &&
		 matrix->type < WESTON_MATRIX_TRANSFORM_ROTATE &&
		 pixman_region32_n_rects(&surfregion) == 1 &&
		 (pixman_region32_equal(&surfregion, &view->surface->opaque) ||
		  view->surface->is_opaque)) {
		/* The whole surface is opaque and it is only translated and
		 * scaled and after applying the scissor, the result is still
		 * a single rectangle. In this case the boundingbox matches the
		 * view exactly and can be used as opaque area. */
		pixman_region32_copy(&view->transform.opaque,
				     &view->transform.boundingbox);
	}
	pixman_region32_fini(&surfregion);

	return 0;
}

WL_EXPORT void
weston_view_update_transform(struct weston_view *view)
{
	struct weston_view *parent = view->geometry.parent;
	struct weston_layer *layer;
	pixman_region32_t mask;

	if (!view->transform.dirty)
		return;

	if (parent)
		weston_view_update_transform(parent);

	view->transform.dirty = 0;

	weston_view_damage_below(view);

	pixman_region32_fini(&view->transform.boundingbox);
	pixman_region32_fini(&view->transform.opaque);
	pixman_region32_init(&view->transform.opaque);

	/* transform.position is always in transformation_list */
	if (view->geometry.transformation_list.next ==
	    &view->transform.position.link &&
	    view->geometry.transformation_list.prev ==
	    &view->transform.position.link &&
	    !parent) {
		weston_view_update_transform_disable(view);
	} else {
		if (weston_view_update_transform_enable(view) < 0)
			weston_view_update_transform_disable(view);
	}

	layer = get_view_layer(view);
	if (layer) {
		pixman_region32_init_with_extents(&mask, &layer->mask);
		pixman_region32_intersect(&view->transform.boundingbox,
					  &view->transform.boundingbox, &mask);
		pixman_region32_intersect(&view->transform.opaque,
					  &view->transform.opaque, &mask);
		pixman_region32_fini(&mask);
	}

	weston_view_damage_below(view);

	weston_view_assign_output(view);

	wl_signal_emit(&view->surface->compositor->transform_signal,
		       view->surface);
}

WL_EXPORT void
weston_view_geometry_dirty(struct weston_view *view)
{
	struct weston_view *child;

	/*
	 * The invariant: if view->geometry.dirty, then all views
	 * in view->geometry.child_list have geometry.dirty too.
	 * Corollary: if not parent->geometry.dirty, then all ancestors
	 * are not dirty.
	 */

	if (view->transform.dirty)
		return;

	view->transform.dirty = 1;

	wl_list_for_each(child, &view->geometry.child_list,
			 geometry.parent_link)
		weston_view_geometry_dirty(child);

	weston_view_dirty_paint_nodes(view);
}

/**
 * \param surface  The surface to be repainted
 *
 * Marks the output(s) that the surface is shown on as needing to be
 * repainted.  See weston_output_schedule_repaint().
 */
WL_EXPORT void
weston_surface_schedule_repaint(struct weston_surface *surface)
{
	struct weston_output *output;

	wl_list_for_each(output, &surface->compositor->output_list, link)
		if (surface->output_mask & (1u << output->id))
			weston_output_schedule_repaint(output);
}

/**
 * \param view  The view to be repainted
 *
 * Marks the output(s) that the view is shown on as needing to be
 * repainted.  See weston_output_schedule_repaint().
 */
WL_EXPORT void
weston_view_schedule_repaint(struct weston_view *view)
{
	struct weston_output *output;

	wl_list_for_each(output, &view->surface->compositor->output_list, link)
		if (view->output_mask & (1u << output->id))
			weston_output_schedule_repaint(output);
}

/**
 * XXX: This function does it the wrong way.
 * surface->damage is the damage from the client, and causes
 * surface_flush_damage() to copy pixels. No window management action can
 * cause damage to the client-provided content, warranting re-upload!
 *
 * Instead of surface->damage, this function should record the damage
 * with all the views for this surface to avoid extraneous texture
 * uploads.
 */
WL_EXPORT void
weston_surface_damage(struct weston_surface *surface)
{
	pixman_region32_union_rect(&surface->damage, &surface->damage,
				   0, 0, surface->width,
				   surface->height);

	weston_surface_schedule_repaint(surface);
}

WL_EXPORT void
weston_view_set_rel_position(struct weston_view *view, float x, float y)
{
	assert(view->geometry.parent);

	if (view->geometry.pos_offset.x == x &&
	    view->geometry.pos_offset.y == y)
		return;

	view->geometry.pos_offset = weston_coord(x, y);
	weston_view_geometry_dirty(view);
}

WL_EXPORT void
weston_view_set_position(struct weston_view *view, float x, float y)
{
	assert(view->surface->committed != subsurface_committed);
	assert(!view->geometry.parent);

	if (view->geometry.pos_offset.x == x &&
	    view->geometry.pos_offset.y == y)
		return;

	view->geometry.pos_offset = weston_coord(x, y);
	weston_view_geometry_dirty(view);
}

static void
transform_parent_handle_parent_destroy(struct wl_listener *listener,
				       void *data)
{
	struct weston_view *view =
		container_of(listener, struct weston_view,
			     geometry.parent_destroy_listener);

	weston_view_set_transform_parent(view, NULL);
	view->parent_view = NULL;
}

WL_EXPORT void
weston_view_set_transform_parent(struct weston_view *view,
				 struct weston_view *parent)
{
	if (view->geometry.parent) {
		wl_list_remove(&view->geometry.parent_destroy_listener.link);
		wl_list_remove(&view->geometry.parent_link);

		if (!parent)
			view->geometry.scissor_enabled = false;
	}

	view->geometry.parent = parent;

	view->geometry.parent_destroy_listener.notify =
		transform_parent_handle_parent_destroy;
	if (parent) {
		wl_signal_add(&parent->destroy_signal,
			      &view->geometry.parent_destroy_listener);
		wl_list_insert(&parent->geometry.child_list,
			       &view->geometry.parent_link);
	}

	weston_view_geometry_dirty(view);
}

/** Set a clip mask rectangle on a view
 *
 * \param view The view to set the clip mask on.
 * \param x Top-left corner X coordinate of the clip rectangle.
 * \param y Top-left corner Y coordinate of the clip rectangle.
 * \param width Width of the clip rectangle, non-negative.
 * \param height Height of the clip rectangle, non-negative.
 *
 * A shell may set a clip mask rectangle on a view. Everything outside
 * the rectangle is cut away for input and output purposes: it is
 * not drawn and cannot be hit by hit-test based input like pointer
 * motion or touch-downs. Everything inside the rectangle will behave
 * normally. Clients are unaware of clipping.
 *
 * The rectangle is set in surface-local coordinates. Setting a clip
 * mask rectangle does not affect the view position, the view is positioned
 * as it would be without a clip. The clip also does not change
 * weston_surface::width,height.
 *
 * The clip mask rectangle is part of transformation inheritance
 * (weston_view_set_transform_parent()). A clip set in the root of the
 * transformation inheritance tree will affect all views in the tree.
 * A clip can be set only on the root view. Attempting to set a clip
 * on view that has a transformation parent will fail. Assigning a parent
 * to a view that has a clip set will cause the clip to be forgotten.
 *
 * Because the clip mask is an axis-aligned rectangle, it poses restrictions
 * on the additional transformations in the child views. These transformations
 * may not rotate the coordinate axes, i.e., only translation and scaling
 * are allowed. Violating this restriction causes the clipping to malfunction.
 * Furthermore, using scaling may cause rounding errors in child clipping.
 *
 * The clip mask rectangle is not automatically adjusted based on
 * wl_surface.attach dx and dy arguments.
 *
 * A clip mask rectangle can be set only if the compositor capability
 * WESTON_CAP_VIEW_CLIP_MASK is present.
 *
 * This function sets the clip mask rectangle and schedules a repaint for
 * the view.
 */
WL_EXPORT void
weston_view_set_mask(struct weston_view *view,
		     int x, int y, int width, int height)
{
	struct weston_compositor *compositor = view->surface->compositor;

	if (!(compositor->capabilities & WESTON_CAP_VIEW_CLIP_MASK)) {
		weston_log("%s not allowed without capability!\n", __func__);
		return;
	}

	if (view->geometry.parent) {
		weston_log("view %p has a parent, clip forbidden!\n", view);
		return;
	}

	if (width < 0 || height < 0) {
		weston_log("%s: illegal args %d, %d, %d, %d\n", __func__,
			   x, y, width, height);
		return;
	}

	pixman_region32_fini(&view->geometry.scissor);
	pixman_region32_init_rect(&view->geometry.scissor, x, y, width, height);
	view->geometry.scissor_enabled = true;
	weston_view_geometry_dirty(view);
	weston_view_schedule_repaint(view);
}

/** Remove the clip mask from a view
 *
 * \param view The view to remove the clip mask from.
 *
 * Removed the clip mask rectangle and schedules a repaint.
 *
 * \sa weston_view_set_mask
 */
WL_EXPORT void
weston_view_set_mask_infinite(struct weston_view *view)
{
	view->geometry.scissor_enabled = false;
	weston_view_geometry_dirty(view);
	weston_view_schedule_repaint(view);
}

/* Check if view should be displayed
 *
 * The indicator is set manually when assigning
 * a view to a surface.
 *
 * This needs reworking. See the thread starting at:
 *
 * https://lists.freedesktop.org/archives/wayland-devel/2016-June/029656.html
 */
WL_EXPORT bool
weston_view_is_mapped(struct weston_view *view)
{
	return view->is_mapped;
}

/* Check if view is opaque in specified region
 *
 * \param view The view to check for opacity.
 * \param region The region to check for opacity, in view coordinates.
 *
 * Returns true if the view is opaque in the specified region, because view
 * alpha is 1.0 and either the opaque region set by the client contains the
 * specified region, or the buffer pixel format or solid color is opaque.
 */
WL_EXPORT bool
weston_view_is_opaque(struct weston_view *ev, pixman_region32_t *region)
{
	pixman_region32_t r;
	bool ret = false;

	if (ev->alpha < 1.0)
		return false;

	if (ev->surface->is_opaque)
		return true;

	if (ev->transform.dirty)
		return false;

	pixman_region32_init(&r);
	pixman_region32_subtract(&r, region, &ev->transform.opaque);

	if (!pixman_region32_not_empty(&r))
		ret = true;

	pixman_region32_fini(&r);

	return ret;
}

/** Check if the view has a valid buffer available
 *
 * @param ev The view to check if it has a valid buffer.
 *
 * Returns true if the view has a valid buffer or false otherwise.
 */
WL_EXPORT bool
weston_view_has_valid_buffer(struct weston_view *ev)
{
	if (!ev->surface->buffer_ref.buffer)
		return false;
	if (!ev->surface->buffer_ref.buffer->resource)
		return false;
	return true;
}

/** Check if the view matches the entire output
 *
 * @param ev The view to check.
 * @param output The output to check against.
 *
 * Returns true if the view does indeed matches the entire output.
 */
WL_EXPORT bool
weston_view_matches_output_entirely(struct weston_view *ev,
				    struct weston_output *output)
{
	pixman_box32_t *extents =
		pixman_region32_extents(&ev->transform.boundingbox);

	assert(!ev->transform.dirty);

	if (extents->x1 != output->x ||
	    extents->y1 != output->y ||
	    extents->x2 != output->x + output->width ||
	    extents->y2 != output->y + output->height)
		return false;

	return true;
}

/** Find paint node for the given view and output
 */
WL_EXPORT struct weston_paint_node *
weston_view_find_paint_node(struct weston_view *view,
			    struct weston_output *output)
{
	struct weston_paint_node *pnode;

	wl_list_for_each(pnode, &view->paint_node_list, view_link) {
		assert(pnode->surface == view->surface);
		if (pnode->output == output)
			return pnode;
	}

	return NULL;
}

/* Check if a surface has a view assigned to it
 *
 * The indicator is set manually when mapping
 * a surface and creating a view for it.
 *
 * This needs to go. See the thread starting at:
 *
 * https://lists.freedesktop.org/archives/wayland-devel/2016-June/029656.html
 *
 */
WL_EXPORT bool
weston_surface_is_mapped(struct weston_surface *surface)
{
	return surface->is_mapped;
}

/** Check if the weston_surface is emitting an unmapping commit
 *
 * @param surface The weston_surface.
 *
 * Returns true if the surface is emitting an unmapping commit.
 */
WL_EXPORT bool
weston_surface_is_unmapping(struct weston_surface *surface)
{
	return surface->is_unmapping;
}

static void
surface_set_size(struct weston_surface *surface, int32_t width, int32_t height)
{
	struct weston_view *view;

	if (surface->width == width && surface->height == height)
		return;

	surface->width = width;
	surface->height = height;

	wl_list_for_each(view, &surface->views, surface_link)
		weston_view_geometry_dirty(view);
}

WL_EXPORT void
weston_surface_set_size(struct weston_surface *surface,
			int32_t width, int32_t height)
{
	assert(!surface->resource);
	surface_set_size(surface, width, height);
}

static int
fixed_round_up_to_int(wl_fixed_t f)
{
	return wl_fixed_to_int(wl_fixed_from_int(1) - 1 + f);
}

WESTON_EXPORT_FOR_TESTS void
convert_size_by_transform_scale(int32_t *width_out, int32_t *height_out,
				int32_t width, int32_t height,
				uint32_t transform,
				int32_t scale)
{
	assert(scale > 0);

	switch (transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
	case WL_OUTPUT_TRANSFORM_180:
	case WL_OUTPUT_TRANSFORM_FLIPPED:
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		*width_out = width / scale;
		*height_out = height / scale;
		break;
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		*width_out = height / scale;
		*height_out = width / scale;
		break;
	default:
		assert(0 && "invalid transform");
	}
}

static void
weston_surface_calculate_size_from_buffer(struct weston_surface *surface)
{
	struct weston_buffer_viewport *vp = &surface->buffer_viewport;

	if (!weston_surface_has_content(surface)) {
		surface->width_from_buffer = 0;
		surface->height_from_buffer = 0;
		return;
	}

	convert_size_by_transform_scale(&surface->width_from_buffer,
					&surface->height_from_buffer,
					surface->buffer_ref.buffer->width,
					surface->buffer_ref.buffer->height,
					vp->buffer.transform,
					vp->buffer.scale);
}

static void
weston_surface_update_size(struct weston_surface *surface)
{
	struct weston_buffer_viewport *vp = &surface->buffer_viewport;
	int32_t width, height;

	width = surface->width_from_buffer;
	height = surface->height_from_buffer;

	if (width != 0 && vp->surface.width != -1) {
		surface_set_size(surface,
				 vp->surface.width, vp->surface.height);
		return;
	}

	if (width != 0 && vp->buffer.src_width != wl_fixed_from_int(-1)) {
		int32_t w = fixed_round_up_to_int(vp->buffer.src_width);
		int32_t h = fixed_round_up_to_int(vp->buffer.src_height);

		surface_set_size(surface, w ?: 1, h ?: 1);
		return;
	}

	surface_set_size(surface, width, height);
}

/** weston_compositor_get_time
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_get_time(struct timespec *time)
{
	clock_gettime(CLOCK_REALTIME, time);
}

bool
weston_view_takes_input_at_point(struct weston_view *view,
				 struct weston_coord_surface pos)
{
	assert(pos.coordinate_space_id == view->surface);

	if (!pixman_region32_contains_point(&view->surface->input,
					    pos.c.x, pos.c.y, NULL))
		return false;

	if (view->geometry.scissor_enabled &&
	    !pixman_region32_contains_point(&view->geometry.scissor,
					    pos.c.x, pos.c.y, NULL))
		return false;

	return true;
}

/** weston_compositor_pick_view
 * \ingroup compositor
 */
WL_EXPORT struct weston_view *
weston_compositor_pick_view(struct weston_compositor *compositor,
			    struct weston_coord_global pos)
{
	struct weston_view *view;

	/* Can't use paint node list: occlusion by input regions, not opaque. */
	wl_list_for_each(view, &compositor->view_list, link) {
		struct weston_coord_surface surf_pos;

		weston_view_update_transform(view);

		if (!pixman_region32_contains_point(
				&view->transform.boundingbox, pos.c.x, pos.c.y, NULL))
			continue;

		surf_pos = weston_coord_global_to_surface(view, pos);
		if (!weston_view_takes_input_at_point(view, surf_pos))
			continue;

		return view;
	}
	return NULL;
}

static void
weston_compositor_repick(struct weston_compositor *compositor)
{
	struct weston_seat *seat;

	if (!compositor->session_active)
		return;

	wl_list_for_each(seat, &compositor->seat_list, link)
		weston_seat_repick(seat);
}

WL_EXPORT void
weston_view_unmap(struct weston_view *view)
{
	struct weston_seat *seat;

	if (!weston_view_is_mapped(view))
		return;

	weston_view_damage_below(view);
	weston_view_set_output(view, NULL);
	view->plane = NULL;
	view->is_mapped = false;
	weston_layer_entry_remove(&view->layer_link);
	wl_list_remove(&view->link);
	wl_list_init(&view->link);
	view->output_mask = 0;
	weston_surface_assign_output(view->surface);

	if (!weston_surface_is_mapped(view->surface)) {
		wl_list_for_each(seat, &view->surface->compositor->seat_list, link) {
			struct weston_touch *touch = weston_seat_get_touch(seat);
			struct weston_pointer *pointer = weston_seat_get_pointer(seat);
			struct weston_keyboard *keyboard =
				weston_seat_get_keyboard(seat);
			struct weston_tablet_tool *tool;

			if (keyboard && keyboard->focus == view->surface)
				weston_keyboard_set_focus(keyboard, NULL);
			if (pointer && pointer->focus == view)
				weston_pointer_clear_focus(pointer);
			if (touch && touch->focus == view)
				weston_touch_set_focus(touch, NULL);

			wl_list_for_each(tool, &seat->tablet_tool_list, link) {
				if (tool->focus == view)
					weston_tablet_tool_set_focus(tool, NULL, 0);
			}
		}
	}
	weston_signal_emit_mutable(&view->unmap_signal, view);
}

WL_EXPORT void
weston_surface_map(struct weston_surface *surface)
{
	surface->is_mapped = true;
}

WL_EXPORT void
weston_surface_unmap(struct weston_surface *surface)
{
	struct weston_view *view;

	surface->is_mapped = false;
	wl_list_for_each(view, &surface->views, surface_link)
		weston_view_unmap(view);
	surface->output = NULL;
}

static void
weston_surface_reset_pending_buffer(struct weston_surface *surface)
{
	weston_surface_state_set_buffer(&surface->pending, NULL);
	surface->pending.newly_attached = 0;
	surface->pending.buffer_viewport.changed = 0;
}

WL_EXPORT void
weston_view_destroy(struct weston_view *view)
{
	struct weston_paint_node *pnode, *pntmp;

	weston_signal_emit_mutable(&view->destroy_signal, view);

	assert(wl_list_empty(&view->geometry.child_list));

	if (weston_view_is_mapped(view)) {
		weston_view_unmap(view);
		weston_compositor_build_view_list(view->surface->compositor,
						  NULL);
	}

	wl_list_for_each_safe(pnode, pntmp, &view->paint_node_list, view_link)
		weston_paint_node_destroy(pnode);

	wl_list_remove(&view->link);
	weston_layer_entry_remove(&view->layer_link);

	pixman_region32_fini(&view->clip);
	pixman_region32_fini(&view->geometry.scissor);
	pixman_region32_fini(&view->transform.boundingbox);
	pixman_region32_fini(&view->transform.opaque);

	weston_view_set_transform_parent(view, NULL);
	weston_view_set_output(view, NULL);

	wl_list_remove(&view->surface_link);

	free(view);
}

WL_EXPORT struct weston_surface *
weston_surface_ref(struct weston_surface *surface)
{
	assert(surface->ref_count < INT32_MAX &&
	       surface->ref_count > 0);

	surface->ref_count++;
	return surface;
}

WL_EXPORT void
weston_surface_unref(struct weston_surface *surface)
{
	struct wl_resource *cb, *next;
	struct weston_view *ev, *nv;
	struct weston_pointer_constraint *constraint, *next_constraint;
	struct weston_paint_node *pnode, *pntmp;

	if (!surface)
		return;

	assert(surface->ref_count > 0);
	if (--surface->ref_count > 0)
		return;

	assert(surface->resource == NULL);

	weston_signal_emit_mutable(&surface->destroy_signal, surface);

	assert(wl_list_empty(&surface->subsurface_list_pending));
	assert(wl_list_empty(&surface->subsurface_list));

	if (surface->dmabuf_feedback)
		weston_dmabuf_feedback_destroy(surface->dmabuf_feedback);

	wl_list_for_each_safe(ev, nv, &surface->views, surface_link)
		weston_view_destroy(ev);

	wl_list_for_each_safe(pnode, pntmp,
			      &surface->paint_node_list, surface_link) {
		weston_paint_node_destroy(pnode);
	}

	weston_surface_state_fini(&surface->pending);

	weston_buffer_reference(&surface->buffer_ref, NULL,
				BUFFER_WILL_NOT_BE_ACCESSED);
	weston_buffer_release_reference(&surface->buffer_release_ref, NULL);

	pixman_region32_fini(&surface->damage);
	pixman_region32_fini(&surface->opaque);
	pixman_region32_fini(&surface->input);

	wl_resource_for_each_safe(cb, next, &surface->frame_callback_list)
		wl_resource_destroy(cb);

	weston_presentation_feedback_discard_list(&surface->feedback_list);

	wl_list_for_each_safe(constraint, next_constraint,
			      &surface->pointer_constraints,
			      link)
		weston_pointer_constraint_destroy(constraint);

	fd_clear(&surface->acquire_fence_fd);

	if (surface->tear_control)
		surface->tear_control->surface = NULL;

	free(surface);
}

static void
destroy_surface(struct wl_resource *resource)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	assert(surface);

	/* Set the resource to NULL, since we don't want to leave a
	 * dangling pointer if the surface was refcounted and survives
	 * the weston_surface_unref() call. */
	surface->resource = NULL;

	if (surface->viewport_resource)
		wl_resource_set_user_data(surface->viewport_resource, NULL);

	if (surface->synchronization_resource) {
		wl_resource_set_user_data(surface->synchronization_resource,
					  NULL);
	}

	weston_surface_unref(surface);
}

static struct weston_solid_buffer_values *
single_pixel_buffer_get(struct wl_resource *resource);

static void
weston_buffer_destroy_handler(struct wl_listener *listener, void *data)
{
	struct weston_buffer *buffer =
		container_of(listener, struct weston_buffer, destroy_listener);

	buffer->resource = NULL;
	buffer->shm_buffer = NULL;

	if (buffer->busy_count + buffer->passive_count > 0)
		return;

	weston_signal_emit_mutable(&buffer->destroy_signal, buffer);
	free(buffer);
}

WL_EXPORT struct weston_buffer *
weston_buffer_from_resource(struct weston_compositor *ec,
			    struct wl_resource *resource)
{
	struct weston_buffer *buffer;
	struct wl_shm_buffer *shm;
	struct linux_dmabuf_buffer *dmabuf;
	struct wl_listener *listener;
	struct weston_solid_buffer_values *solid;

	listener = wl_resource_get_destroy_listener(resource,
						    weston_buffer_destroy_handler);

	if (listener)
		return container_of(listener, struct weston_buffer,
				    destroy_listener);

	buffer = zalloc(sizeof *buffer);
	if (buffer == NULL)
		return NULL;

	buffer->resource = resource;
	wl_signal_init(&buffer->destroy_signal);
	buffer->destroy_listener.notify = weston_buffer_destroy_handler;
	wl_resource_add_destroy_listener(resource, &buffer->destroy_listener);

	if ((shm = wl_shm_buffer_get(buffer->resource))) {
		buffer->type = WESTON_BUFFER_SHM;
		buffer->shm_buffer = shm;
		buffer->width = wl_shm_buffer_get_width(shm);
		buffer->height = wl_shm_buffer_get_height(shm);
		buffer->buffer_origin = ORIGIN_TOP_LEFT;
		/* wl_shm might create a buffer with an unknown format, so check
		 * and reject */
		buffer->pixel_format =
			pixel_format_get_info_shm(wl_shm_buffer_get_format(shm));
		buffer->format_modifier = DRM_FORMAT_MOD_LINEAR;

		if (!buffer->pixel_format || buffer->pixel_format->hide_from_clients)
			goto fail;
	} else if ((dmabuf = linux_dmabuf_buffer_get(buffer->resource))) {
		buffer->type = WESTON_BUFFER_DMABUF;
		buffer->dmabuf = dmabuf;
		buffer->direct_display = dmabuf->direct_display;
		buffer->width = dmabuf->attributes.width;
		buffer->height = dmabuf->attributes.height;
		buffer->pixel_format =
			pixel_format_get_info(dmabuf->attributes.format);
		/* dmabuf import should assure we don't create a buffer with an
		 * unknown format */
		assert(buffer->pixel_format && !buffer->pixel_format->hide_from_clients);
		buffer->format_modifier = dmabuf->attributes.modifier[0];
		if (dmabuf->attributes.flags & ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT)
			buffer->buffer_origin = ORIGIN_BOTTOM_LEFT;
		else
			buffer->buffer_origin = ORIGIN_TOP_LEFT;
	} else if ((solid = single_pixel_buffer_get(buffer->resource))) {
		buffer->type = WESTON_BUFFER_SOLID;
		buffer->solid = *solid;
		buffer->width = 1;
		buffer->height = 1;
		if (buffer->solid.a == 1.0) {
			buffer->pixel_format =
				pixel_format_get_info(DRM_FORMAT_XRGB8888);
		} else {
			buffer->pixel_format =
				pixel_format_get_info(DRM_FORMAT_ARGB8888);
		}
		buffer->format_modifier = DRM_FORMAT_MOD_LINEAR;
	} else {
		/* Only taken for legacy EGL buffers */
		if (!ec->renderer->fill_buffer_info ||
		    !ec->renderer->fill_buffer_info(ec, buffer)) {
			goto fail;
		}
		buffer->type = WESTON_BUFFER_RENDERER_OPAQUE;
	}

	/* Don't accept any formats we can't reason about: the importer should
	 * make sure this never happens */
	assert(buffer->pixel_format);

	return buffer;

fail:
	wl_list_remove(&buffer->destroy_listener.link);
	free(buffer);
	return NULL;
}

WL_EXPORT void
weston_buffer_reference(struct weston_buffer_reference *ref,
			struct weston_buffer *buffer,
			enum weston_buffer_reference_type type)
{
	struct weston_buffer_reference old_ref = *ref;

	assert(buffer != NULL || type == BUFFER_WILL_NOT_BE_ACCESSED);

	if (buffer == ref->buffer && type == ref->type)
		return;

	/* First ref the incoming buffer, so we keep positive refcount */
	if (buffer) {
		if (type == BUFFER_MAY_BE_ACCESSED)
			buffer->busy_count++;
		else
			buffer->passive_count++;
	}

	ref->buffer = buffer;
	ref->type = type;

	/* Now drop refs to the old buffer, if any */
	if (!old_ref.buffer)
		return;

	ref = NULL; /* will no longer be accessed */

	if (old_ref.type == BUFFER_MAY_BE_ACCESSED) {
		assert(old_ref.buffer->busy_count > 0);
		old_ref.buffer->busy_count--;

		/* If the wl_buffer lives, then hold on to the weston_buffer,
		 * but send a release event to the client */
		if (old_ref.buffer->busy_count == 0 &&
		    old_ref.buffer->resource) {
			assert(wl_resource_get_client(old_ref.buffer->resource));
			wl_buffer_send_release(old_ref.buffer->resource);
		}
	} else if (old_ref.type == BUFFER_WILL_NOT_BE_ACCESSED) {
		assert(old_ref.buffer->passive_count > 0);
		old_ref.buffer->passive_count--;
	} else {
		assert(!"unknown buffer ref type");
	}

	/* If the wl_buffer has gone and this was the last ref, destroy the
	 * weston_buffer, since we'll never need it again */
	if (old_ref.buffer->busy_count + old_ref.buffer->passive_count == 0 &&
	    !old_ref.buffer->resource) {
		weston_signal_emit_mutable(&old_ref.buffer->destroy_signal,
					   old_ref.buffer);
		free(old_ref.buffer);
	}
}

static void
weston_buffer_release_reference_handle_destroy(struct wl_listener *listener,
					       void *data)
{
	struct weston_buffer_release_reference *ref =
		container_of(listener, struct weston_buffer_release_reference,
			     destroy_listener);

	assert((struct wl_resource *)data == ref->buffer_release->resource);
	ref->buffer_release = NULL;
}

static void
weston_buffer_release_destroy(struct weston_buffer_release *buffer_release)
{
	struct wl_resource *resource = buffer_release->resource;
	int release_fence_fd = buffer_release->fence_fd;

	if (release_fence_fd >= 0) {
		zwp_linux_buffer_release_v1_send_fenced_release(
			resource, release_fence_fd);
	} else {
		zwp_linux_buffer_release_v1_send_immediate_release(
			resource);
	}

	wl_resource_destroy(resource);
}

WL_EXPORT void
weston_buffer_release_reference(struct weston_buffer_release_reference *ref,
				struct weston_buffer_release *buffer_release)
{
	if (buffer_release == ref->buffer_release)
		return;

	if (ref->buffer_release) {
		ref->buffer_release->ref_count--;
		wl_list_remove(&ref->destroy_listener.link);
		if (ref->buffer_release->ref_count == 0)
			weston_buffer_release_destroy(ref->buffer_release);
	}

	if (buffer_release) {
		buffer_release->ref_count++;
		wl_resource_add_destroy_listener(buffer_release->resource,
						 &ref->destroy_listener);
	}

	ref->buffer_release = buffer_release;
	ref->destroy_listener.notify =
		weston_buffer_release_reference_handle_destroy;
}

WL_EXPORT void
weston_buffer_release_move(struct weston_buffer_release_reference *dest,
			   struct weston_buffer_release_reference *src)
{
	weston_buffer_release_reference(dest, src->buffer_release);
	weston_buffer_release_reference(src, NULL);
}

WL_EXPORT struct weston_buffer_reference *
weston_buffer_create_solid_rgba(struct weston_compositor *compositor,
				float r, float g, float b, float a)
{
	struct weston_buffer_reference *ret = zalloc(sizeof(*ret));
	struct weston_buffer *buffer;

	if (!ret)
		return NULL;

	buffer = zalloc(sizeof(*buffer));
	if (!buffer) {
		free(ret);
		return NULL;
	}

	wl_signal_init(&buffer->destroy_signal);
	buffer->type = WESTON_BUFFER_SOLID;
	buffer->width = 1;
	buffer->height = 1;
	buffer->buffer_origin = ORIGIN_TOP_LEFT;
	buffer->solid.r = r;
	buffer->solid.g = g;
	buffer->solid.b = b;
	buffer->solid.a = a;

	if (a == 1.0) {
		buffer->pixel_format =
			pixel_format_get_info_shm(WL_SHM_FORMAT_XRGB8888);
	} else {
		buffer->pixel_format =
			pixel_format_get_info_shm(WL_SHM_FORMAT_ARGB8888);
	}
	buffer->format_modifier = DRM_FORMAT_MOD_LINEAR;

	weston_buffer_reference(ret, buffer, BUFFER_MAY_BE_ACCESSED);

	return ret;
}

WL_EXPORT void
weston_surface_attach_solid(struct weston_surface *surface,
			    struct weston_buffer_reference *buffer_ref,
			    int w, int h)
{
	struct weston_buffer *buffer = buffer_ref->buffer;

	assert(buffer);
	assert(buffer->type == WESTON_BUFFER_SOLID);
	weston_buffer_reference(&surface->buffer_ref, buffer,
				BUFFER_MAY_BE_ACCESSED);
	surface->compositor->renderer->attach(surface, buffer);

	weston_surface_set_size(surface, w, h);

	pixman_region32_fini(&surface->opaque);
	if (buffer->solid.a == 1.0) {
		surface->is_opaque = true;
		pixman_region32_init_rect(&surface->opaque, 0, 0, w, h);
	} else {
		surface->is_opaque = false;
		pixman_region32_init(&surface->opaque);
	}
}

WL_EXPORT void
weston_buffer_destroy_solid(struct weston_buffer_reference *buffer_ref)
{
	assert(buffer_ref);
	assert(buffer_ref->buffer);
	assert(buffer_ref->type == BUFFER_MAY_BE_ACCESSED);
	assert(buffer_ref->buffer->type == WESTON_BUFFER_SOLID);
	weston_buffer_reference(buffer_ref, NULL, BUFFER_WILL_NOT_BE_ACCESSED);
	free(buffer_ref);
}

static void
single_pixel_buffer_destroy(struct wl_resource *resource)
{
	struct weston_solid_buffer_values *solid =
		wl_resource_get_user_data(resource);
	free(solid);
}

static void
single_pixel_buffer_handle_buffer_destroy(struct wl_client *client,
					  struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_buffer_interface single_pixel_buffer_implementation = {
	single_pixel_buffer_handle_buffer_destroy,
};

static struct weston_solid_buffer_values *
single_pixel_buffer_get(struct wl_resource *resource)
{
	if (!resource)
		return NULL;

	if (!wl_resource_instance_of(resource, &wl_buffer_interface,
				     &single_pixel_buffer_implementation))
		return NULL;

	return wl_resource_get_user_data(resource);
}

static void
single_pixel_buffer_manager_destroy(struct wl_client *client,
				    struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
single_pixel_buffer_create(struct wl_client *client, struct wl_resource *resource,
			   uint32_t id, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
	struct weston_solid_buffer_values *solid = zalloc(sizeof(*solid));
	struct wl_resource *buffer;

	if (!solid) {
		wl_client_post_no_memory(client);
		return;
	}

	solid->r = r / (double) 0xffffffff;
	solid->g = g / (double) 0xffffffff;
	solid->b = b / (double) 0xffffffff;
	solid->a = a / (double) 0xffffffff;

	buffer = wl_resource_create(client, &wl_buffer_interface, 1, id);
	if (!buffer) {
		wl_client_post_no_memory(client);
		free(solid);
		return;
	}
	wl_resource_set_implementation(buffer,
				       &single_pixel_buffer_implementation,
				       solid, single_pixel_buffer_destroy);
}

static const struct wp_single_pixel_buffer_manager_v1_interface
single_pixel_buffer_manager_implementation = {
	single_pixel_buffer_manager_destroy,
	single_pixel_buffer_create,
};

static void
bind_single_pixel_buffer(struct wl_client *client, void *data, uint32_t version,
			 uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &wp_single_pixel_buffer_manager_v1_interface, 1,
				      id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource,
				       &single_pixel_buffer_manager_implementation,
				       NULL, NULL);
}

static void
weston_surface_attach(struct weston_surface *surface,
		      struct weston_buffer *buffer)
{
	weston_buffer_reference(&surface->buffer_ref, buffer,
				buffer ? BUFFER_MAY_BE_ACCESSED :
					 BUFFER_WILL_NOT_BE_ACCESSED);

	if (!buffer) {
		if (weston_surface_is_mapped(surface)) {
			weston_surface_unmap(surface);
			/* This is the unmapping commit */
			surface->is_unmapping = true;
		}
	}

	surface->compositor->renderer->attach(surface, buffer);

	weston_surface_calculate_size_from_buffer(surface);
	weston_presentation_feedback_discard_list(&surface->feedback_list);

	if (buffer)
		surface->is_opaque = pixel_format_is_opaque(buffer->pixel_format);
}

/** weston_compositor_damage_all
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_damage_all(struct weston_compositor *compositor)
{
	struct weston_output *output;

	wl_list_for_each(output, &compositor->output_list, link)
		weston_output_damage(output);
}

/**
 * \ingroup output
 */
WL_EXPORT void
weston_output_damage(struct weston_output *output)
{
	struct weston_compositor *compositor = output->compositor;

	pixman_region32_union(&compositor->primary_plane.damage,
			      &compositor->primary_plane.damage,
			      &output->region);
	weston_output_schedule_repaint(output);
}

/* FIXME: note that we don't flush any damage when the core wants us to
 * do so as it will sometimes ask for a flush not necessarily at the
 * right time.
 *
 * A (more) proper way is to handle correctly damage whenever there's
 * compositor side damage. See the comment for weston_surface_damage().
 */
static bool
buffer_can_be_accessed_BANDAID_XXX(struct weston_buffer_reference buffer_ref)
{
	return buffer_ref.type == BUFFER_MAY_BE_ACCESSED;
}

static void
surface_flush_damage(struct weston_surface *surface, struct weston_output *output)
{
	struct weston_buffer *buffer = surface->buffer_ref.buffer;

	if (buffer->type == WESTON_BUFFER_SHM &&
	    buffer_can_be_accessed_BANDAID_XXX(surface->buffer_ref))
		surface->compositor->renderer->flush_damage(surface, buffer);

	if (pixman_region32_not_empty(&surface->damage))
		TL_POINT(surface->compositor, "core_flush_damage",
			 TLP_SURFACE(surface), TLP_OUTPUT(output), TLP_END);

	pixman_region32_clear(&surface->damage);
}

static void
view_accumulate_damage(struct weston_view *view,
		       pixman_region32_t *opaque)
{
	pixman_region32_t damage;

	assert(!view->transform.dirty);

	pixman_region32_init(&damage);
	if (view->transform.enabled) {
		pixman_box32_t *extents;

		extents = pixman_region32_extents(&view->surface->damage);
		view_compute_bbox(view, extents, &damage);
	} else {
		pixman_region32_copy(&damage, &view->surface->damage);
		pixman_region32_translate(&damage,
					  view->geometry.pos_offset.x,
					  view->geometry.pos_offset.y);
	}

	pixman_region32_intersect(&damage, &damage,
				  &view->transform.boundingbox);
	pixman_region32_subtract(&damage, &damage, opaque);
	pixman_region32_union(&view->plane->damage,
			      &view->plane->damage, &damage);
	pixman_region32_fini(&damage);
	pixman_region32_copy(&view->clip, opaque);
	pixman_region32_union(opaque, opaque, &view->transform.opaque);
}

static void
output_accumulate_damage(struct weston_output *output)
{
	struct weston_compositor *ec = output->compositor;
	struct weston_plane *plane;
	struct weston_paint_node *pnode;
	pixman_region32_t opaque, clip;

	pixman_region32_init(&clip);

	wl_list_for_each(plane, &ec->plane_list, link) {
		pixman_region32_copy(&plane->clip, &clip);

		pixman_region32_init(&opaque);

		wl_list_for_each(pnode, &output->paint_node_z_order_list,
				 z_order_link) {
			if (pnode->view->plane != plane)
				continue;

			view_accumulate_damage(pnode->view, &opaque);
		}

		pixman_region32_union(&clip, &clip, &opaque);
		pixman_region32_fini(&opaque);
	}

	pixman_region32_fini(&clip);

	wl_list_for_each(pnode, &output->paint_node_z_order_list,
			 z_order_link) {
		pnode->surface->touched = false;
	}

	wl_list_for_each(pnode, &output->paint_node_z_order_list,
			 z_order_link) {
		/* Ignore views not visible on the current output */
		/* TODO: turn this into assert once z_order_list is pruned. */
		if (!(pnode->view->output_mask & (1u << output->id)))
			continue;
		if (pnode->surface->touched)
			continue;
		pnode->surface->touched = true;

		surface_flush_damage(pnode->surface, output);

		/* Both the renderer and the backend have seen the buffer
		 * by now. If renderer needs the buffer, it has its own
		 * reference set. If the backend wants to keep the buffer
		 * around for migrating the surface into a non-primary plane
		 * later, keep_buffer is true. Otherwise, drop the core
		 * reference now, and allow early buffer release. This enables
		 * clients to use single-buffering.
		 */
		if (!pnode->surface->keep_buffer) {
			weston_buffer_reference(&pnode->surface->buffer_ref,
						pnode->surface->buffer_ref.buffer,
						BUFFER_WILL_NOT_BE_ACCESSED);
			weston_buffer_release_reference(
				&pnode->surface->buffer_release_ref, NULL);
		}
	}
}

static void
surface_stash_subsurface_views(struct weston_surface *surface)
{
	struct weston_subsurface *sub;

	wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
		if (sub->surface == surface)
			continue;

		wl_list_insert_list(&sub->unused_views, &sub->surface->views);
		wl_list_init(&sub->surface->views);

		surface_stash_subsurface_views(sub->surface);
	}
}

static void
surface_free_unused_subsurface_views(struct weston_surface *surface)
{
	struct weston_subsurface *sub;
	struct weston_view *view, *nv;

	wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
		if (sub->surface == surface)
			continue;

		wl_list_for_each_safe(view, nv, &sub->unused_views, surface_link) {
			weston_view_unmap (view);
			weston_view_destroy(view);
		}

		surface_free_unused_subsurface_views(sub->surface);
	}
}

static struct weston_paint_node *
view_ensure_paint_node(struct weston_view *view, struct weston_output *output)
{
	struct weston_paint_node *pnode;

	if (!output)
		return NULL;

	pnode = weston_view_find_paint_node(view, output);
	if (pnode) {
		paint_node_update(pnode);
		return pnode;
	}

	return weston_paint_node_create(view->surface, view, output);
}

static void
add_to_z_order_list(struct weston_output *output,
		    struct weston_paint_node *pnode)
{
	if (!pnode)
		return;

	wl_list_remove(&pnode->z_order_link);
	wl_list_insert(output->paint_node_z_order_list.prev,
		       &pnode->z_order_link);

	/*
	 * Building weston_output::paint_node_z_order_list ensures all
	 * necessary color transform objects are installed.
	 */
	weston_paint_node_ensure_color_transform(pnode);
}

static void
view_list_add_subsurface_view(struct weston_compositor *compositor,
			      struct weston_subsurface *sub,
			      struct weston_view *parent,
			      struct weston_output *output)
{
	struct weston_subsurface *child;
	struct weston_view *view = NULL, *iv;
	struct weston_paint_node *pnode;

	if (!weston_surface_is_mapped(sub->surface))
		return;

	wl_list_for_each(iv, &sub->unused_views, surface_link) {
		if (iv->geometry.parent == parent) {
			view = iv;
			break;
		}
	}

	if (view) {
		/* Put it back in the surface's list of views */
		wl_list_remove(&view->surface_link);
		wl_list_insert(&sub->surface->views, &view->surface_link);
	} else {
		view = weston_view_create(sub->surface);
		weston_view_set_transform_parent(view, parent);
		weston_view_set_rel_position(view,
					     sub->position.offset.c.x,
					     sub->position.offset.c.y);
	}

	view->parent_view = parent;
	weston_view_update_transform(view);
	view->is_mapped = true;
	pnode = view_ensure_paint_node(view, output);

	if (wl_list_empty(&sub->surface->subsurface_list)) {
		wl_list_insert(compositor->view_list.prev, &view->link);
		add_to_z_order_list(output, pnode);
		return;
	}

	wl_list_for_each(child, &sub->surface->subsurface_list, parent_link) {
		if (child->surface == sub->surface) {
			wl_list_insert(compositor->view_list.prev, &view->link);
			add_to_z_order_list(output, pnode);
		} else {
			view_list_add_subsurface_view(compositor, child, view, output);
		}
	}
}

/* This recursively adds the sub-surfaces for a view, relying on the
 * sub-surface order. Thus, if a client restacks the sub-surfaces, that
 * change first happens to the sub-surface list, and then automatically
 * propagates here. See weston_surface_damage_subsurfaces() for how the
 * sub-surfaces receive damage when the client changes the state.
 */
static void
view_list_add(struct weston_compositor *compositor,
	      struct weston_view *view,
	      struct weston_output *output)
{
	struct weston_paint_node *pnode;
	struct weston_subsurface *sub;

	weston_view_update_transform(view);

	/* It is possible for a view to appear in the layer list even though
	 * the view or the surface is unmapped. This is erroneous but difficult
	 * to fix. */
	if (!weston_surface_is_mapped(view->surface) ||
	    !weston_view_is_mapped(view) ||
	    !weston_surface_has_content(view->surface)) {
		weston_log_paced(&compositor->unmapped_surface_or_view_pacer,
				 1, 0,
				 "Detected an unmapped surface or view in "
				 "the layer list, which should not occur.\n");

		pnode = weston_view_find_paint_node(view, output);
		if (pnode)
			weston_paint_node_destroy(pnode);

		return;
	}

	pnode = view_ensure_paint_node(view, output);

	if (wl_list_empty(&view->surface->subsurface_list)) {
		wl_list_insert(compositor->view_list.prev, &view->link);
		add_to_z_order_list(output, pnode);
		return;
	}

	wl_list_for_each(sub, &view->surface->subsurface_list, parent_link) {
		if (sub->surface == view->surface) {
			wl_list_insert(compositor->view_list.prev, &view->link);
			add_to_z_order_list(output, pnode);
		} else {
			view_list_add_subsurface_view(compositor, sub, view, output);
		}
	}
}

static void
weston_compositor_build_view_list(struct weston_compositor *compositor,
				  struct weston_output *output)
{
	struct weston_view *view, *tmp;
	struct weston_layer *layer;

	if (output) {
		wl_list_remove(&output->paint_node_z_order_list);
		wl_list_init(&output->paint_node_z_order_list);
	}

	wl_list_for_each(layer, &compositor->layer_list, link)
		wl_list_for_each(view, &layer->view_list.link, layer_link.link)
			surface_stash_subsurface_views(view->surface);

	wl_list_for_each_safe(view, tmp, &compositor->view_list, link)
		wl_list_init(&view->link);
	wl_list_init(&compositor->view_list);

	wl_list_for_each(layer, &compositor->layer_list, link) {
		wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
			view_list_add(compositor, view, output);
		}
	}

	wl_list_for_each(layer, &compositor->layer_list, link)
		wl_list_for_each(view, &layer->view_list.link, layer_link.link)
			surface_free_unused_subsurface_views(view->surface);
}

static void
weston_output_take_feedback_list(struct weston_output *output,
				 struct weston_surface *surface)
{
	struct weston_view *view;
	struct weston_presentation_feedback *feedback;
	uint32_t flags = 0xffffffff;

	if (wl_list_empty(&surface->feedback_list))
		return;

	/* All views must have the flag for the flag to survive. */
	wl_list_for_each(view, &surface->views, surface_link) {
		/* ignore views that are not on this output at all */
		if (view->output_mask & (1u << output->id))
			flags &= view->psf_flags;
	}

	wl_list_for_each(feedback, &surface->feedback_list, link)
		feedback->psf_flags = flags;

	wl_list_insert_list(&output->feedback_list, &surface->feedback_list);
	wl_list_init(&surface->feedback_list);
}

static int
weston_output_repaint(struct weston_output *output)
{
	struct weston_compositor *ec = output->compositor;
	struct weston_paint_node *pnode;
	struct weston_animation *animation, *next;
	struct wl_resource *cb, *cnext;
	struct wl_list frame_callback_list;
	pixman_region32_t output_damage;
	int r;
	uint32_t frame_time_msec;
	enum weston_hdcp_protection highest_requested = WESTON_HDCP_DISABLE;

	if (output->destroying)
		return 0;

	TL_POINT(ec, "core_repaint_begin", TLP_OUTPUT(output), TLP_END);

	/* Rebuild the surface list and update surface transforms up front. */
	weston_compositor_build_view_list(ec, output);

	/* Find the highest protection desired for an output */
	wl_list_for_each(pnode, &output->paint_node_z_order_list,
			 z_order_link) {
		/* TODO: turn this into assert once z_order_list is pruned. */
		if ((pnode->surface->output_mask & (1u << output->id)) == 0)
			continue;

		/*
		 * The desired_protection of the output should be the
		 * maximum of the desired_protection of the surfaces,
		 * that are displayed on that output, to avoid
		 * reducing the protection for existing surfaces.
		 */
		if (pnode->surface->desired_protection > highest_requested)
			highest_requested = pnode->surface->desired_protection;
	}

	output->desired_protection = highest_requested;

	if (output->assign_planes && !output->disable_planes) {
		output->assign_planes(output);
	} else {
		wl_list_for_each(pnode, &output->paint_node_z_order_list,
				 z_order_link) {
			/* TODO: turn this into assert once z_order_list is pruned. */
			if ((pnode->view->output_mask & (1u << output->id)) == 0)
				continue;

			weston_view_move_to_plane(pnode->view, &ec->primary_plane);
			pnode->view->psf_flags = 0;
		}
	}

	wl_list_init(&frame_callback_list);
	wl_list_for_each(pnode, &output->paint_node_z_order_list,
			 z_order_link) {
		/* Note: This operation is safe to do multiple times on the
		 * same surface.
		 */
		if (pnode->surface->output == output) {
			wl_list_insert_list(&frame_callback_list,
					    &pnode->surface->frame_callback_list);
			wl_list_init(&pnode->surface->frame_callback_list);

			weston_output_take_feedback_list(output, pnode->surface);
		}
	}

	output_accumulate_damage(output);

	pixman_region32_init(&output_damage);
	pixman_region32_intersect(&output_damage,
				  &ec->primary_plane.damage, &output->region);
	pixman_region32_subtract(&output_damage,
				 &output_damage, &ec->primary_plane.clip);

	r = output->repaint(output, &output_damage);

	pixman_region32_fini(&output_damage);

	output->repaint_needed = false;
	if (r == 0)
		output->repaint_status = REPAINT_AWAITING_COMPLETION;

	weston_compositor_repick(ec);

	frame_time_msec = timespec_to_msec(&output->frame_time);

	wl_resource_for_each_safe(cb, cnext, &frame_callback_list) {
		wl_callback_send_done(cb, frame_time_msec);
		wl_resource_destroy(cb);
	}

	wl_list_for_each_safe(animation, next, &output->animation_list, link) {
		animation->frame_counter++;
		animation->frame(animation, output, &output->frame_time);
	}

	weston_output_capture_info_repaint_done(output->capture_info);

	TL_POINT(ec, "core_repaint_posted", TLP_OUTPUT(output), TLP_END);

	return r;
}

static void
weston_output_schedule_repaint_reset(struct weston_output *output)
{
	output->repaint_status = REPAINT_NOT_SCHEDULED;
	TL_POINT(output->compositor, "core_repaint_exit_loop",
		 TLP_OUTPUT(output), TLP_END);
}

static int
weston_output_maybe_repaint(struct weston_output *output, struct timespec *now)
{
	struct weston_compositor *compositor = output->compositor;
	int ret = 0;
	int64_t msec_to_repaint;

	/* We're not ready yet; come back to make a decision later. */
	if (output->repaint_status != REPAINT_SCHEDULED)
		return ret;

	msec_to_repaint = timespec_sub_to_msec(&output->next_repaint, now);
	if (msec_to_repaint > 1)
		return ret;

	/* If we're sleeping, drop the repaint machinery entirely; we will
	 * explicitly repaint all outputs when we come back. */
	if (compositor->state == WESTON_COMPOSITOR_SLEEPING ||
	    compositor->state == WESTON_COMPOSITOR_OFFSCREEN)
		goto err;

	/* We don't actually need to repaint this output; drop it from
	 * repaint until something causes damage. */
	if (!output->repaint_needed)
		goto err;

	if (output->power_state == WESTON_OUTPUT_POWER_FORCED_OFF)
		goto err;

	/* If repaint fails, we aren't going to get weston_output_finish_frame
	 * to trigger a new repaint, so drop it from repaint and hope
	 * something schedules a successful repaint later. As repainting may
	 * take some time, re-read our clock as a courtesy to the next
	 * output. */
	ret = weston_output_repaint(output);
	weston_compositor_read_presentation_clock(compositor, now);
	if (ret != 0)
		goto err;

	output->repainted = true;
	return ret;

err:
	weston_output_schedule_repaint_reset(output);
	return ret;
}

static void
output_repaint_timer_arm(struct weston_compositor *compositor)
{
	struct weston_output *output;
	bool any_should_repaint = false;
	struct timespec now;
	int64_t msec_to_next = INT64_MAX;

	weston_compositor_read_presentation_clock(compositor, &now);

	wl_list_for_each(output, &compositor->output_list, link) {
		int64_t msec_to_this;

		if (output->repaint_status != REPAINT_SCHEDULED)
			continue;

		msec_to_this = timespec_sub_to_msec(&output->next_repaint,
						    &now);
		if (!any_should_repaint || msec_to_this < msec_to_next)
			msec_to_next = msec_to_this;

		any_should_repaint = true;
	}

	if (!any_should_repaint)
		return;

	/* Even if we should repaint immediately, add the minimum 1 ms delay.
	 * This is a workaround to allow coalescing multiple output repaints
	 * particularly from weston_output_finish_frame()
	 * into the same call, which would not happen if we called
	 * output_repaint_timer_handler() directly.
	 */
	if (msec_to_next < 1)
		msec_to_next = 1;

	wl_event_source_timer_update(compositor->repaint_timer, msec_to_next);
}

static void
weston_output_schedule_repaint_restart(struct weston_output *output)
{
	assert(output->repaint_status == REPAINT_AWAITING_COMPLETION);
	/* The device was busy so try again one frame later */
	timespec_add_nsec(&output->next_repaint, &output->next_repaint,
			  millihz_to_nsec(output->current_mode->refresh));
	output->repaint_status = REPAINT_SCHEDULED;
	TL_POINT(output->compositor, "core_repaint_restart",
		 TLP_OUTPUT(output), TLP_END);
	output_repaint_timer_arm(output->compositor);
	weston_output_damage(output);
}

static int
output_repaint_timer_handler(void *data)
{
	struct weston_compositor *compositor = data;
	struct weston_output *output;
	struct timespec now;
	int ret = 0;

	weston_compositor_read_presentation_clock(compositor, &now);
	compositor->last_repaint_start = now;

	if (compositor->backend->repaint_begin)
		compositor->backend->repaint_begin(compositor->backend);

	wl_list_for_each(output, &compositor->output_list, link) {
		ret = weston_output_maybe_repaint(output, &now);
		if (ret)
			break;
	}

	if (ret == 0) {
		if (compositor->backend->repaint_flush)
			ret = compositor->backend->repaint_flush(compositor->backend);
	} else {
		if (compositor->backend->repaint_cancel)
			compositor->backend->repaint_cancel(compositor->backend);
	}

	if (ret != 0) {
		wl_list_for_each(output, &compositor->output_list, link) {
			if (output->repainted) {
				if (ret == -EBUSY)
					weston_output_schedule_repaint_restart(output);
				else
					weston_output_schedule_repaint_reset(output);
			}
		}
	}

	wl_list_for_each(output, &compositor->output_list, link)
		output->repainted = false;

	output_repaint_timer_arm(compositor);

	return 0;
}

/** Convert a presentation timestamp to another clock domain
 *
 * \param compositor The compositor defines the presentation clock domain.
 * \param presentation_stamp The timestamp in presentation clock domain.
 * \param presentation_now Current time in presentation clock domain.
 * \param target_clock Defines the target clock domain.
 *
 * This approximation relies on presentation_stamp to be close to current time.
 * The further it is from current time and the bigger the speed difference
 * between the two clock domains, the bigger the conversion error.
 *
 * Conversion error due to system load is biased and unbounded.
 */
static struct timespec
convert_presentation_time_now(struct weston_compositor *compositor,
			      const struct timespec *presentation_stamp,
			      const struct timespec *presentation_now,
			      clockid_t target_clock)
{
	struct timespec target_now = {};
	struct timespec target_stamp;
	int64_t delta_ns;

	if (compositor->presentation_clock == target_clock)
		return *presentation_stamp;

	clock_gettime(target_clock, &target_now);
	delta_ns = timespec_sub_to_nsec(presentation_stamp, presentation_now);
	timespec_add_nsec(&target_stamp, &target_now, delta_ns);

	return target_stamp;
}

/**
 * \ingroup output
 */
WL_EXPORT void
weston_output_finish_frame(struct weston_output *output,
			   const struct timespec *stamp,
			   uint32_t presented_flags)
{
	struct weston_compositor *compositor = output->compositor;
	int32_t refresh_nsec;
	struct timespec now;
	struct timespec vblank_monotonic;
	int64_t msec_rel;

	assert(output->repaint_status == REPAINT_AWAITING_COMPLETION);

	/*
	 * If timestamp of latest vblank is given, it must always go forwards.
	 * If not given, INVALID flag must be set.
	 */
	if (stamp)
		assert(timespec_sub_to_nsec(stamp, &output->frame_time) >= 0);
	else
		assert(presented_flags & WP_PRESENTATION_FEEDBACK_INVALID);

	weston_compositor_read_presentation_clock(compositor, &now);

	/* If we haven't been supplied any timestamp at all, we don't have a
	 * timebase to work against, so any delay just wastes time. Push a
	 * repaint as soon as possible so we can get on with it. */
	if (!stamp) {
		output->next_repaint = now;
		goto out;
	}

	vblank_monotonic = convert_presentation_time_now(compositor,
							 stamp, &now,
							 CLOCK_MONOTONIC);
	TL_POINT(compositor, "core_repaint_finished", TLP_OUTPUT(output),
		 TLP_VBLANK(&vblank_monotonic), TLP_END);

	refresh_nsec = millihz_to_nsec(output->current_mode->refresh);
	weston_presentation_feedback_present_list(&output->feedback_list,
						  output, refresh_nsec, stamp,
						  output->msc,
						  presented_flags);

	output->frame_time = *stamp;

	/* If we're tearing just repaint right away */
	if (presented_flags & WESTON_FINISH_FRAME_TEARING) {
		output->next_repaint = now;
		goto out;
	}

	timespec_add_nsec(&output->next_repaint, stamp, refresh_nsec);
	timespec_add_msec(&output->next_repaint, &output->next_repaint,
			  -compositor->repaint_msec);
	msec_rel = timespec_sub_to_msec(&output->next_repaint, &now);

	if (msec_rel < -1000 || msec_rel > 1000) {
		weston_log_paced(&output->repaint_delay_pacer,
				 5, 60 * 60 * 1000,
				 "Warning: computed repaint delay for output "
				 "[%s] is abnormal: %lld msec\n",
				 output->name, (long long) msec_rel);

		output->next_repaint = now;
	}

	/* Called from restart_repaint_loop and restart happens already after
	 * the deadline given by repaint_msec? In that case we delay until
	 * the deadline of the next frame, to give clients a more predictable
	 * timing of the repaint cycle to lock on. */
	if (presented_flags == WP_PRESENTATION_FEEDBACK_INVALID &&
	    msec_rel < 0) {
		while (timespec_sub_to_nsec(&output->next_repaint, &now) < 0) {
			timespec_add_nsec(&output->next_repaint,
					  &output->next_repaint,
					  refresh_nsec);
		}
	}

out:
	output->repaint_status = REPAINT_SCHEDULED;
	output_repaint_timer_arm(compositor);
}


WL_EXPORT void
weston_output_repaint_failed(struct weston_output *output)
{
	weston_log("Clearing repaint status.\n");
	assert(output->repaint_status == REPAINT_AWAITING_COMPLETION);
	output->repaint_status = REPAINT_NOT_SCHEDULED;
}

static void
idle_repaint(void *data)
{
	struct weston_output *output = data;
	int ret;

	assert(output->repaint_status == REPAINT_BEGIN_FROM_IDLE);
	output->repaint_status = REPAINT_AWAITING_COMPLETION;
	output->idle_repaint_source = NULL;
	ret = output->start_repaint_loop(output);
	if (ret == -EBUSY)
		weston_output_schedule_repaint_restart(output);
	else if (ret != 0)
		weston_output_schedule_repaint_reset(output);
}

WL_EXPORT void
weston_layer_entry_insert(struct weston_layer_entry *list,
			  struct weston_layer_entry *entry)
{
	wl_list_insert(&list->link, &entry->link);
	entry->layer = list->layer;
}

WL_EXPORT void
weston_layer_entry_remove(struct weston_layer_entry *entry)
{
	wl_list_remove(&entry->link);
	wl_list_init(&entry->link);
	entry->layer = NULL;
}


/** Initialize the weston_layer struct.
 *
 * \param compositor The compositor instance
 * \param layer The layer to initialize
 */
WL_EXPORT void
weston_layer_init(struct weston_layer *layer,
		  struct weston_compositor *compositor)
{
	layer->compositor = compositor;
	wl_list_init(&layer->link);
	wl_list_init(&layer->view_list.link);
	layer->view_list.layer = layer;
	weston_layer_set_mask_infinite(layer);
}

/** Finalize the weston_layer struct.
 *
 * \param layer The layer to finalize.
 */
WL_EXPORT void
weston_layer_fini(struct weston_layer *layer)
{
	wl_list_remove(&layer->link);

	if (!wl_list_empty(&layer->view_list.link))
		weston_log("BUG: finalizing a layer with views still on it.\n");

	wl_list_remove(&layer->view_list.link);
}

/** Sets the position of the layer in the layer list. The layer will be placed
 * below any layer with the same position value, if any.
 * This function is safe to call if the layer is already on the list, but the
 * layer may be moved below other layers at the same position, if any.
 *
 * \param layer The layer to modify
 * \param position The position the layer will be placed at
 */
WL_EXPORT void
weston_layer_set_position(struct weston_layer *layer,
			  enum weston_layer_position position)
{
	struct weston_layer *below;

	wl_list_remove(&layer->link);

	/* layer_list is ordered from top to bottom, the last layer being the
	 * background with the smallest position value */

	layer->position = position;
	wl_list_for_each_reverse(below, &layer->compositor->layer_list, link) {
		if (below->position >= layer->position) {
			wl_list_insert(&below->link, &layer->link);
			return;
		}
	}
	wl_list_insert(&layer->compositor->layer_list, &layer->link);
}

/** Hide a layer by taking it off the layer list.
 * This function is safe to call if the layer is not on the list.
 *
 * \param layer The layer to hide
 */
WL_EXPORT void
weston_layer_unset_position(struct weston_layer *layer)
{
	wl_list_remove(&layer->link);
	wl_list_init(&layer->link);
}

WL_EXPORT void
weston_layer_set_mask(struct weston_layer *layer,
		      int x, int y, int width, int height)
{
	struct weston_view *view;

	layer->mask.x1 = x;
	layer->mask.x2 = x + width;
	layer->mask.y1 = y;
	layer->mask.y2 = y + height;

	wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
		weston_view_geometry_dirty(view);
	}
}

WL_EXPORT void
weston_layer_set_mask_infinite(struct weston_layer *layer)
{
	struct weston_view *view;

	layer->mask.x1 = INT32_MIN;
	layer->mask.x2 = INT32_MAX;
	layer->mask.y1 = INT32_MIN;
	layer->mask.y2 = INT32_MAX;

	wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
		weston_view_geometry_dirty(view);
	}
}

WL_EXPORT bool
weston_layer_mask_is_infinite(struct weston_layer *layer)
{
	return layer->mask.x1 == INT32_MIN &&
	       layer->mask.y1 == INT32_MIN &&
	       layer->mask.x2 == INT32_MAX &&
	       layer->mask.y2 == INT32_MAX;
}

/**
 * \ingroup output
 */
WL_EXPORT void
weston_output_schedule_repaint(struct weston_output *output)
{
	struct weston_compositor *compositor = output->compositor;
	struct wl_event_loop *loop;

	if (compositor->state == WESTON_COMPOSITOR_SLEEPING ||
	    compositor->state == WESTON_COMPOSITOR_OFFSCREEN)
		return;

	if (output->power_state == WESTON_OUTPUT_POWER_FORCED_OFF)
		return;

	if (!output->repaint_needed)
		TL_POINT(compositor, "core_repaint_req", TLP_OUTPUT(output), TLP_END);

	loop = wl_display_get_event_loop(compositor->wl_display);
	output->repaint_needed = true;

	/* If we already have a repaint scheduled for our idle handler,
	 * no need to set it again. If the repaint has been called but
	 * not finished, then weston_output_finish_frame() will notice
	 * that a repaint is needed and schedule one. */
	if (output->repaint_status != REPAINT_NOT_SCHEDULED)
		return;

	output->repaint_status = REPAINT_BEGIN_FROM_IDLE;
	assert(!output->idle_repaint_source);
	output->idle_repaint_source = wl_event_loop_add_idle(loop, idle_repaint,
							     output);
	TL_POINT(compositor, "core_repaint_enter_loop", TLP_OUTPUT(output), TLP_END);
}

/** weston_compositor_schedule_repaint
 *  \ingroup compositor
 */
WL_EXPORT void
weston_compositor_schedule_repaint(struct weston_compositor *compositor)
{
	struct weston_output *output;

	wl_list_for_each(output, &compositor->output_list, link)
		weston_output_schedule_repaint(output);
}

/**
 * Returns true if a surface has a buffer attached to it and thus valid
 * content available.
 */
WL_EXPORT bool
weston_surface_has_content(struct weston_surface *surface)
{
	return !!surface->buffer_ref.buffer;
}

static void
surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
surface_attach(struct wl_client *client,
	       struct wl_resource *resource,
	       struct wl_resource *buffer_resource, int32_t sx, int32_t sy)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);
	struct weston_compositor *ec = surface->compositor;
	struct weston_buffer *buffer = NULL;

	if (buffer_resource) {
		buffer = weston_buffer_from_resource(ec, buffer_resource);
		if (buffer == NULL) {
			wl_client_post_no_memory(client);
			return;
		}
	}

	if (wl_resource_get_version(resource) >= WL_SURFACE_OFFSET_SINCE_VERSION) {
		if (sx != 0 || sy != 0) {
			wl_resource_post_error(resource,
					       WL_SURFACE_ERROR_INVALID_OFFSET,
					       "Can't attach with an offset");
			return;
		}
	} else {
		surface->pending.sx = sx;
		surface->pending.sy = sy;
	}

	/* Attach, attach, without commit in between does not send
	 * wl_buffer.release. */
	weston_surface_state_set_buffer(&surface->pending, buffer);

	surface->pending.newly_attached = 1;
}

static void
surface_damage(struct wl_client *client,
	       struct wl_resource *resource,
	       int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	if (width <= 0 || height <= 0)
		return;

	pixman_region32_union_rect(&surface->pending.damage_surface,
				   &surface->pending.damage_surface,
				   x, y, width, height);
}

static void
surface_damage_buffer(struct wl_client *client,
		      struct wl_resource *resource,
		      int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	if (width <= 0 || height <= 0)
		return;

	pixman_region32_union_rect(&surface->pending.damage_buffer,
				   &surface->pending.damage_buffer,
				   x, y, width, height);
}

static void
destroy_frame_callback(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
surface_frame(struct wl_client *client,
	      struct wl_resource *resource, uint32_t callback)
{
	struct wl_resource *cb;
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	cb = wl_resource_create(client, &wl_callback_interface, 1, callback);
	if (cb == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_resource_set_implementation(cb, NULL, NULL,
				       destroy_frame_callback);

	wl_list_insert(surface->pending.frame_callback_list.prev,
		       wl_resource_get_link(cb));
}

static void
surface_set_opaque_region(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *region_resource)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);
	struct weston_region *region;

	if (region_resource) {
		region = wl_resource_get_user_data(region_resource);
		pixman_region32_copy(&surface->pending.opaque,
				     &region->region);
	} else {
		pixman_region32_clear(&surface->pending.opaque);
	}
}

static void
surface_set_input_region(struct wl_client *client,
			 struct wl_resource *resource,
			 struct wl_resource *region_resource)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);
	struct weston_region *region;

	if (region_resource) {
		region = wl_resource_get_user_data(region_resource);
		pixman_region32_copy(&surface->pending.input,
				     &region->region);
	} else {
		pixman_region32_fini(&surface->pending.input);
		region_init_infinite(&surface->pending.input);
	}
}

/* Cause damage to this sub-surface and all its children.
 *
 * This is useful when there are state changes that need an implicit
 * damage, e.g. a z-order change.
 */
static void
weston_surface_damage_subsurfaces(struct weston_subsurface *sub)
{
	struct weston_subsurface *child;

	weston_surface_damage(sub->surface);
	sub->reordered = false;

	wl_list_for_each(child, &sub->surface->subsurface_list, parent_link)
		if (child != sub)
			weston_surface_damage_subsurfaces(child);
}

static void
weston_surface_commit_subsurface_order(struct weston_surface *surface)
{
	struct weston_subsurface *sub;

	wl_list_for_each_reverse(sub, &surface->subsurface_list_pending,
				 parent_link_pending) {
		wl_list_remove(&sub->parent_link);
		wl_list_insert(&surface->subsurface_list, &sub->parent_link);

		if (sub->reordered)
			weston_surface_damage_subsurfaces(sub);
	}
}

WESTON_EXPORT_FOR_TESTS void
weston_surface_build_buffer_matrix(const struct weston_surface *surface,
				   struct weston_matrix *matrix)
{
	const struct weston_buffer_viewport *vp = &surface->buffer_viewport;
	double src_width, src_height, dest_width, dest_height;
	struct weston_matrix transform_matrix;

	weston_matrix_init(matrix);

	if (vp->buffer.src_width == wl_fixed_from_int(-1)) {
		src_width = surface->width_from_buffer;
		src_height = surface->height_from_buffer;
	} else {
		src_width = wl_fixed_to_double(vp->buffer.src_width);
		src_height = wl_fixed_to_double(vp->buffer.src_height);
	}

	if (vp->surface.width == -1) {
		dest_width = src_width;
		dest_height = src_height;
	} else {
		dest_width = vp->surface.width;
		dest_height = vp->surface.height;
	}

	if (src_width != dest_width || src_height != dest_height)
		weston_matrix_scale(matrix,
				    src_width / dest_width,
				    src_height / dest_height, 1);

	if (vp->buffer.src_width != wl_fixed_from_int(-1))
		weston_matrix_translate(matrix,
					wl_fixed_to_double(vp->buffer.src_x),
					wl_fixed_to_double(vp->buffer.src_y),
					0);

	weston_matrix_init_transform(&transform_matrix,
				     vp->buffer.transform,
				     0, 0,
				     surface->width_from_buffer,
				     surface->height_from_buffer,
				     vp->buffer.scale);
	weston_matrix_multiply(matrix, &transform_matrix);
}

/**
 * Compute a + b > c while being safe to overflows.
 */
static bool
fixed_sum_gt(wl_fixed_t a, wl_fixed_t b, wl_fixed_t c)
{
	return (int64_t)a + (int64_t)b > (int64_t)c;
}

static bool
weston_surface_is_pending_viewport_source_valid(
	const struct weston_surface *surface)
{
	const struct weston_surface_state *pend = &surface->pending;
	const struct weston_buffer_viewport *vp = &pend->buffer_viewport;
	int width_from_buffer = 0;
	int height_from_buffer = 0;
	wl_fixed_t w;
	wl_fixed_t h;

	/* If viewport source rect is not set, it is always ok. */
	if (vp->buffer.src_width == wl_fixed_from_int(-1))
		return true;

	if (pend->newly_attached) {
		if (pend->buffer) {
			convert_size_by_transform_scale(&width_from_buffer,
							&height_from_buffer,
							pend->buffer->width,
							pend->buffer->height,
							vp->buffer.transform,
							vp->buffer.scale);
		} else {
			/* No buffer: viewport is irrelevant. */
			return true;
		}
	} else {
		width_from_buffer = surface->width_from_buffer;
		height_from_buffer = surface->height_from_buffer;
	}

	assert((width_from_buffer == 0) == (height_from_buffer == 0));
	assert(width_from_buffer >= 0 && height_from_buffer >= 0);

	/* No buffer: viewport is irrelevant. */
	if (width_from_buffer == 0 || height_from_buffer == 0)
		return true;

	/* overflow checks for wl_fixed_from_int() */
	if (width_from_buffer > wl_fixed_to_int(INT32_MAX))
		return false;
	if (height_from_buffer > wl_fixed_to_int(INT32_MAX))
		return false;

	w = wl_fixed_from_int(width_from_buffer);
	h = wl_fixed_from_int(height_from_buffer);

	if (fixed_sum_gt(vp->buffer.src_x, vp->buffer.src_width, w))
		return false;
	if (fixed_sum_gt(vp->buffer.src_y, vp->buffer.src_height, h))
		return false;

	return true;
}

static bool
fixed_is_integer(wl_fixed_t v)
{
	return (v & 0xff) == 0;
}

static bool
weston_surface_is_pending_viewport_dst_size_int(
	const struct weston_surface *surface)
{
	const struct weston_buffer_viewport *vp =
		&surface->pending.buffer_viewport;

	if (vp->surface.width != -1) {
		assert(vp->surface.width > 0 && vp->surface.height > 0);
		return true;
	}

	return fixed_is_integer(vp->buffer.src_width) &&
	       fixed_is_integer(vp->buffer.src_height);
}

/* Translate pending damage in buffer co-ordinates to surface
 * co-ordinates and union it with a pixman_region32_t.
 * This should only be called after the buffer is attached.
 */
static void
apply_damage_buffer(pixman_region32_t *dest,
		    struct weston_surface *surface,
		    struct weston_surface_state *state)
{
	struct weston_buffer *buffer = surface->buffer_ref.buffer;

	/* wl_surface.damage_buffer needs to be clipped to the buffer,
	 * translated into surface co-ordinates and unioned with
	 * any other surface damage.
	 * None of this makes sense if there is no buffer though.
	 */
	if (buffer && pixman_region32_not_empty(&state->damage_buffer)) {
		pixman_region32_t buffer_damage;

		pixman_region32_intersect_rect(&state->damage_buffer,
					       &state->damage_buffer,
					       0, 0, buffer->width,
					       buffer->height);
		pixman_region32_init(&buffer_damage);
		weston_matrix_transform_region(&buffer_damage,
					       &surface->buffer_to_surface_matrix,
					       &state->damage_buffer);
		pixman_region32_union(dest, dest, &buffer_damage);
		pixman_region32_fini(&buffer_damage);
	}
	/* We should clear this on commit even if there was no buffer */
	pixman_region32_clear(&state->damage_buffer);
}

static void
weston_surface_set_desired_protection(struct weston_surface *surface,
				      enum weston_hdcp_protection protection)
{
	if (surface->desired_protection == protection)
		return;
	surface->desired_protection = protection;
	weston_surface_damage(surface);
}

static void
weston_surface_set_protection_mode(struct weston_surface *surface,
				   enum weston_surface_protection_mode p_mode)
{
	struct content_protection *cp = surface->compositor->content_protection;
	struct protected_surface *psurface;

	surface->protection_mode = p_mode;
	wl_list_for_each(psurface, &cp->protected_list, link) {
		if (!psurface || psurface->surface != surface)
			continue;
		weston_protected_surface_send_event(psurface,
						    surface->current_protection);
	}
}

static void
weston_surface_commit_state(struct weston_surface *surface,
			    struct weston_surface_state *state)
{
	struct weston_view *view;
	pixman_region32_t opaque;

	/* wl_surface.set_buffer_transform */
	/* wl_surface.set_buffer_scale */
	/* wp_viewport.set_source */
	/* wp_viewport.set_destination */
	surface->buffer_viewport = state->buffer_viewport;

	/* wl_surface.attach */
	if (state->newly_attached) {
		/* zwp_surface_synchronization_v1.set_acquire_fence */
		fd_move(&surface->acquire_fence_fd,
			&state->acquire_fence_fd);
		/* zwp_surface_synchronization_v1.get_release */
		weston_buffer_release_move(&surface->buffer_release_ref,
					   &state->buffer_release_ref);
		weston_surface_attach(surface, state->buffer);
	}
	weston_surface_state_set_buffer(state, NULL);
	assert(state->acquire_fence_fd == -1);
	assert(state->buffer_release_ref.buffer_release == NULL);

	weston_surface_build_buffer_matrix(surface,
					   &surface->surface_to_buffer_matrix);
	weston_matrix_invert(&surface->buffer_to_surface_matrix,
			     &surface->surface_to_buffer_matrix);

	/* It's possible that this surface's buffer and transform changed
	 * at the same time in such a way that its size remains the same.
	 *
	 * That means we can't depend on view_geometry_dirty() from a
	 * size update to invalidate the paint node data in all relevant
	 * cases, so just smash it here.
	 */
	weston_surface_dirty_paint_nodes(surface);
	if (state->newly_attached || state->buffer_viewport.changed ||
	    state->sx != 0 || state->sy != 0) {
		weston_surface_update_size(surface);
		if (surface->committed) {
			struct weston_coord_surface new_origin;

			new_origin = weston_coord_surface(state->sx,
							  state->sy,
							  surface);
			surface->committed(surface, new_origin);
		}
	}

	state->sx = 0;
	state->sy = 0;
	state->newly_attached = 0;
	state->buffer_viewport.changed = 0;

	/* wl_surface.damage and wl_surface.damage_buffer */
	if (pixman_region32_not_empty(&state->damage_surface) ||
	     pixman_region32_not_empty(&state->damage_buffer))
		TL_POINT(surface->compositor, "core_commit_damage", TLP_SURFACE(surface), TLP_END);

	pixman_region32_union(&surface->damage, &surface->damage,
			      &state->damage_surface);

	apply_damage_buffer(&surface->damage, surface, state);

	pixman_region32_intersect_rect(&surface->damage, &surface->damage,
				       0, 0, surface->width, surface->height);
	pixman_region32_clear(&state->damage_surface);

	/* wl_surface.set_opaque_region */
	pixman_region32_init(&opaque);
	pixman_region32_intersect_rect(&opaque, &state->opaque,
				       0, 0, surface->width, surface->height);

	if (!pixman_region32_equal(&opaque, &surface->opaque)) {
		pixman_region32_copy(&surface->opaque, &opaque);
		wl_list_for_each(view, &surface->views, surface_link)
			weston_view_geometry_dirty(view);
	}

	pixman_region32_fini(&opaque);

	/* wl_surface.set_input_region */
	pixman_region32_intersect_rect(&surface->input, &state->input,
				       0, 0, surface->width, surface->height);

	/* wl_surface.frame */
	wl_list_insert_list(&surface->frame_callback_list,
			    &state->frame_callback_list);
	wl_list_init(&state->frame_callback_list);

	/* XXX:
	 * What should happen with a feedback request, if there
	 * is no wl_buffer attached for this commit?
	 */

	/* presentation.feedback */
	wl_list_insert_list(&surface->feedback_list,
			    &state->feedback_list);
	wl_list_init(&state->feedback_list);

	/* weston_protected_surface.enforced/relaxed */
	if (surface->protection_mode != state->protection_mode)
		weston_surface_set_protection_mode(surface,
						   state->protection_mode);

	/* weston_protected_surface.set_type */
	weston_surface_set_desired_protection(surface, state->desired_protection);

	wl_signal_emit(&surface->commit_signal, surface);

	/* Surface is fully unmapped now */
	surface->is_unmapping = false;
}

static void
weston_surface_commit(struct weston_surface *surface)
{
	weston_surface_commit_state(surface, &surface->pending);

	weston_surface_commit_subsurface_order(surface);

	weston_surface_schedule_repaint(surface);
}

static void
weston_subsurface_commit(struct weston_subsurface *sub);

static void
weston_subsurface_parent_commit(struct weston_subsurface *sub,
				int parent_is_synchronized);

static void
surface_commit(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);
	struct weston_subsurface *sub = weston_surface_to_subsurface(surface);

	if (!weston_surface_is_pending_viewport_source_valid(surface)) {
		assert(surface->viewport_resource);

		wl_resource_post_error(surface->viewport_resource,
			WP_VIEWPORT_ERROR_OUT_OF_BUFFER,
			"wl_surface@%d has viewport source outside buffer",
			wl_resource_get_id(resource));
		return;
	}

	if (!weston_surface_is_pending_viewport_dst_size_int(surface)) {
		assert(surface->viewport_resource);

		wl_resource_post_error(surface->viewport_resource,
			WP_VIEWPORT_ERROR_BAD_SIZE,
			"wl_surface@%d viewport dst size not integer",
			wl_resource_get_id(resource));
		return;
	}

	if (surface->pending.acquire_fence_fd >= 0) {
		assert(surface->synchronization_resource);

		if (!surface->pending.buffer) {
			fd_clear(&surface->pending.acquire_fence_fd);
			wl_resource_post_error(surface->synchronization_resource,
				ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
				"wl_surface@%"PRIu32" no buffer for synchronization",
				wl_resource_get_id(resource));
			return;
		}

		if (surface->pending.buffer->type == WESTON_BUFFER_SHM) {
			fd_clear(&surface->pending.acquire_fence_fd);
			wl_resource_post_error(surface->synchronization_resource,
				ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_UNSUPPORTED_BUFFER,
				"wl_surface@%"PRIu32" unsupported buffer for synchronization",
				wl_resource_get_id(resource));
			return;
		}
	}

	if (surface->pending.buffer_release_ref.buffer_release &&
	    !surface->pending.buffer) {
		assert(surface->synchronization_resource);

		wl_resource_post_error(surface->synchronization_resource,
			ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
			"wl_surface@%"PRIu32" no buffer for synchronization",
			wl_resource_get_id(resource));
		return;
	}

	if (sub) {
		weston_subsurface_commit(sub);
		return;
	}

	wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
		if (sub->surface != surface)
			weston_subsurface_parent_commit(sub, 0);
	}

	weston_surface_commit(surface);
}

static void
surface_set_buffer_transform(struct wl_client *client,
			     struct wl_resource *resource, int transform)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	/* if wl_output.transform grows more members this will need to be updated. */
	if (transform < 0 ||
	    transform > WL_OUTPUT_TRANSFORM_FLIPPED_270) {
		wl_resource_post_error(resource,
			WL_SURFACE_ERROR_INVALID_TRANSFORM,
			"buffer transform must be a valid transform "
			"('%d' specified)", transform);
		return;
	}

	surface->pending.buffer_viewport.buffer.transform = transform;
	surface->pending.buffer_viewport.changed = 1;
}

static void
surface_set_buffer_scale(struct wl_client *client,
			 struct wl_resource *resource,
			 int32_t scale)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	if (scale < 1) {
		wl_resource_post_error(resource,
			WL_SURFACE_ERROR_INVALID_SCALE,
			"buffer scale must be at least one "
			"('%d' specified)", scale);
		return;
	}

	surface->pending.buffer_viewport.buffer.scale = scale;
	surface->pending.buffer_viewport.changed = 1;
}

static void
surface_offset(struct wl_client *client,
	       struct wl_resource *resource,
	       int32_t sx,
	       int32_t sy)
{
	struct weston_surface *surface = wl_resource_get_user_data(resource);

	surface->pending.sx = sx;
	surface->pending.sy = sy;
}

static const struct wl_surface_interface surface_interface = {
	surface_destroy,
	surface_attach,
	surface_damage,
	surface_frame,
	surface_set_opaque_region,
	surface_set_input_region,
	surface_commit,
	surface_set_buffer_transform,
	surface_set_buffer_scale,
	surface_damage_buffer,
	surface_offset,
};

static void
compositor_create_surface(struct wl_client *client,
			  struct wl_resource *resource, uint32_t id)
{
	struct weston_compositor *ec = wl_resource_get_user_data(resource);
	struct weston_surface *surface;

	surface = weston_surface_create(ec);
	if (surface == NULL)
		goto err;

	surface->resource =
		wl_resource_create(client, &wl_surface_interface,
				   wl_resource_get_version(resource), id);
	if (surface->resource == NULL)
		goto err_res;
	wl_resource_set_implementation(surface->resource, &surface_interface,
				       surface, destroy_surface);

	wl_signal_emit(&ec->create_surface_signal, surface);

	return;

err_res:
	weston_surface_unref(surface);
err:
	wl_resource_post_no_memory(resource);
}

static void
destroy_region(struct wl_resource *resource)
{
	struct weston_region *region = wl_resource_get_user_data(resource);

	pixman_region32_fini(&region->region);
	free(region);
}

static void
region_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
region_add(struct wl_client *client, struct wl_resource *resource,
	   int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct weston_region *region = wl_resource_get_user_data(resource);

	pixman_region32_union_rect(&region->region, &region->region,
				   x, y, width, height);
}

static void
region_subtract(struct wl_client *client, struct wl_resource *resource,
		int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct weston_region *region = wl_resource_get_user_data(resource);
	pixman_region32_t rect;

	pixman_region32_init_rect(&rect, x, y, width, height);
	pixman_region32_subtract(&region->region, &region->region, &rect);
	pixman_region32_fini(&rect);
}

static const struct wl_region_interface region_interface = {
	region_destroy,
	region_add,
	region_subtract
};

static void
compositor_create_region(struct wl_client *client,
			 struct wl_resource *resource, uint32_t id)
{
	struct weston_region *region;

	region = malloc(sizeof *region);
	if (region == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	pixman_region32_init(&region->region);

	region->resource =
		wl_resource_create(client, &wl_region_interface, 1, id);
	if (region->resource == NULL) {
		free(region);
		wl_resource_post_no_memory(resource);
		return;
	}
	wl_resource_set_implementation(region->resource, &region_interface,
				       region, destroy_region);
}

static const struct wl_compositor_interface compositor_interface = {
	compositor_create_surface,
	compositor_create_region
};

static void
weston_subsurface_commit_from_cache(struct weston_subsurface *sub)
{
	struct weston_surface *surface = sub->surface;

	weston_surface_commit_state(surface, &sub->cached);
	weston_buffer_reference(&sub->cached_buffer_ref, NULL,
				BUFFER_WILL_NOT_BE_ACCESSED);

	weston_surface_commit_subsurface_order(surface);

	weston_surface_schedule_repaint(surface);

	sub->has_cached_data = 0;
}

static void
weston_subsurface_commit_to_cache(struct weston_subsurface *sub)
{
	struct weston_surface *surface = sub->surface;

	/*
	 * If this commit would cause the surface to move by the
	 * attach(dx, dy) parameters, the old damage region must be
	 * translated to correspond to the new surface coordinate system
	 * origin.
	 */
	pixman_region32_translate(&sub->cached.damage_surface,
				  -surface->pending.sx, -surface->pending.sy);
	pixman_region32_union(&sub->cached.damage_surface,
			      &sub->cached.damage_surface,
			      &surface->pending.damage_surface);
	pixman_region32_clear(&surface->pending.damage_surface);

	pixman_region32_union(&sub->cached.damage_buffer,
			      &sub->cached.damage_buffer,
			      &surface->pending.damage_buffer);
	pixman_region32_clear(&surface->pending.damage_buffer);

	if (surface->pending.newly_attached) {
		sub->cached.newly_attached = 1;
		weston_surface_state_set_buffer(&sub->cached,
						surface->pending.buffer);
		weston_buffer_reference(&sub->cached_buffer_ref,
					surface->pending.buffer,
					surface->pending.buffer ?
						BUFFER_MAY_BE_ACCESSED :
						BUFFER_WILL_NOT_BE_ACCESSED);
		weston_presentation_feedback_discard_list(
					&sub->cached.feedback_list);
		/* zwp_surface_synchronization_v1.set_acquire_fence */
		fd_move(&sub->cached.acquire_fence_fd,
			&surface->pending.acquire_fence_fd);
		/* zwp_surface_synchronization_v1.get_release */
		weston_buffer_release_move(&sub->cached.buffer_release_ref,
					   &surface->pending.buffer_release_ref);
	}
	sub->cached.desired_protection = surface->pending.desired_protection;
	sub->cached.protection_mode = surface->pending.protection_mode;
	assert(surface->pending.acquire_fence_fd == -1);
	assert(surface->pending.buffer_release_ref.buffer_release == NULL);
	sub->cached.sx += surface->pending.sx;
	sub->cached.sy += surface->pending.sy;

	sub->cached.buffer_viewport.changed |=
		surface->pending.buffer_viewport.changed;
	sub->cached.buffer_viewport.buffer =
		surface->pending.buffer_viewport.buffer;
	sub->cached.buffer_viewport.surface =
		surface->pending.buffer_viewport.surface;

	weston_surface_reset_pending_buffer(surface);

	surface->pending.sx = 0;
	surface->pending.sy = 0;

	pixman_region32_copy(&sub->cached.opaque, &surface->pending.opaque);

	pixman_region32_copy(&sub->cached.input, &surface->pending.input);

	wl_list_insert_list(&sub->cached.frame_callback_list,
			    &surface->pending.frame_callback_list);
	wl_list_init(&surface->pending.frame_callback_list);

	wl_list_insert_list(&sub->cached.feedback_list,
			    &surface->pending.feedback_list);
	wl_list_init(&surface->pending.feedback_list);

	sub->has_cached_data = 1;
}

static bool
weston_subsurface_is_synchronized(struct weston_subsurface *sub)
{
	while (sub) {
		if (sub->synchronized)
			return true;

		if (!sub->parent)
			return false;

		sub = weston_surface_to_subsurface(sub->parent);
	}

	return false;
}

static void
weston_subsurface_commit(struct weston_subsurface *sub)
{
	struct weston_surface *surface = sub->surface;
	struct weston_subsurface *tmp;

	/* Recursive check for effectively synchronized. */
	if (weston_subsurface_is_synchronized(sub)) {
		weston_subsurface_commit_to_cache(sub);
	} else {
		if (sub->has_cached_data) {
			/* flush accumulated state from cache */
			weston_subsurface_commit_to_cache(sub);
			weston_subsurface_commit_from_cache(sub);
		} else {
			weston_surface_commit(surface);
		}

		wl_list_for_each(tmp, &surface->subsurface_list, parent_link) {
			if (tmp->surface != surface)
				weston_subsurface_parent_commit(tmp, 0);
		}
	}
}

static void
weston_subsurface_synchronized_commit(struct weston_subsurface *sub)
{
	struct weston_surface *surface = sub->surface;
	struct weston_subsurface *tmp;

	/* From now on, commit_from_cache the whole sub-tree, regardless of
	 * the synchronized mode of each child. This sub-surface or some
	 * of its ancestors were synchronized, so we are synchronized
	 * all the way down.
	 */

	if (sub->has_cached_data)
		weston_subsurface_commit_from_cache(sub);

	wl_list_for_each(tmp, &surface->subsurface_list, parent_link) {
		if (tmp->surface != surface)
			weston_subsurface_parent_commit(tmp, 1);
	}
}

static void
weston_subsurface_parent_commit(struct weston_subsurface *sub,
				int parent_is_synchronized)
{
	struct weston_view *view;

	if (sub->position.changed) {
		wl_list_for_each(view, &sub->surface->views, surface_link)
			weston_view_set_rel_position(view,
						     sub->position.offset.c.x,
						     sub->position.offset.c.y);

		sub->position.changed = false;
	}

	if (parent_is_synchronized || sub->synchronized)
		weston_subsurface_synchronized_commit(sub);
}

static int
subsurface_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "sub-surface");
}

static void
subsurface_committed(struct weston_surface *surface,
		     struct weston_coord_surface new_origin)
{
	struct weston_view *view;

	wl_list_for_each(view, &surface->views, surface_link) {
		struct weston_coord_surface tmp = new_origin;

		if (!view->geometry.parent) {
			weston_log_paced(&view->subsurface_parent_log_pacer,
					 1, 0, "Client attempted to commit on a "
					 "subsurface without a parent surface\n");
			continue;
		}

		tmp.c = weston_coord_add(tmp.c,
					 view->geometry.pos_offset);
		weston_view_set_rel_position(view, tmp.c.x, tmp.c.y);
	}
	/* No need to check parent mappedness, because if parent is not
	 * mapped, parent is not in a visible layer, so this sub-surface
	 * will not be drawn either.
	 */
	if (!weston_surface_is_mapped(surface) &&
	    weston_surface_has_content(surface)) {
		weston_surface_map(surface);
	}

	/* Cannot call weston_view_update_transform() here, because that would
	 * call it also for the parent surface, which might not be mapped yet.
	 * That would lead to inconsistent state, where the window could never
	 * be mapped.
	 *
	 * Instead just force the child surface to appear mapped, to make
	 * weston_surface_is_mapped() return true, so that when the parent
	 * surface does get mapped, this one will get included, too. See
	 * view_list_add().
	 */
}

static struct weston_subsurface *
weston_surface_to_subsurface(struct weston_surface *surface)
{
	if (surface->committed == subsurface_committed)
		return surface->committed_private;

	return NULL;
}

WL_EXPORT struct weston_surface *
weston_surface_get_main_surface(struct weston_surface *surface)
{
	struct weston_subsurface *sub;

	while (surface && (sub = weston_surface_to_subsurface(surface)))
		surface = sub->parent;

	return surface;
}

WL_EXPORT int
weston_surface_set_role(struct weston_surface *surface,
			const char *role_name,
			struct wl_resource *error_resource,
			uint32_t error_code)
{
	assert(role_name);

	if (surface->role_name == NULL ||
	    surface->role_name == role_name ||
	    strcmp(surface->role_name, role_name) == 0) {
		surface->role_name = role_name;

		return 0;
	}

	wl_resource_post_error(error_resource, error_code,
			       "Cannot assign role %s to wl_surface@%d,"
			       " already has role %s\n",
			       role_name,
			       wl_resource_get_id(surface->resource),
			       surface->role_name);
	return -1;
}

WL_EXPORT const char *
weston_surface_get_role(struct weston_surface *surface)
{
	return surface->role_name;
}

WL_EXPORT void
weston_surface_set_label_func(struct weston_surface *surface,
			      int (*desc)(struct weston_surface *,
					  char *, size_t))
{
	surface->get_label = desc;
	weston_timeline_refresh_subscription_objects(surface->compositor,
						     surface);
}

/** Get the size of surface contents
 *
 * \param surface The surface to query.
 * \param width Returns the width of raw contents.
 * \param height Returns the height of raw contents.
 *
 * Retrieves the raw surface content size in pixels for the given surface.
 * This is the whole content size in buffer pixels. If the surface
 * has no content, zeroes are returned.
 *
 * This function is used to determine the buffer size needed for
 * a weston_surface_copy_content() call.
 */
WL_EXPORT void
weston_surface_get_content_size(struct weston_surface *surface,
				int *width, int *height)
{
	struct weston_buffer *buffer = surface->buffer_ref.buffer;

	if (buffer) {
		*width = buffer->width;
		*height = buffer->height;
	} else {
		*width = 0;
		*height = 0;
	}
}

/** Get the bounding box of a surface and its subsurfaces
 *
 * \param surface The surface to query.
 * \return The bounding box relative to the surface origin.
 *
 */
WL_EXPORT struct weston_geometry
weston_surface_get_bounding_box(struct weston_surface *surface)
{
	pixman_region32_t region;
	pixman_box32_t *box;
	struct weston_subsurface *subsurface;

	pixman_region32_init_rect(&region,
				  0, 0,
				  surface->width, surface->height);

	wl_list_for_each(subsurface, &surface->subsurface_list, parent_link)
		pixman_region32_union_rect(&region, &region,
					   subsurface->position.offset.c.x,
					   subsurface->position.offset.c.y,
					   subsurface->surface->width,
					   subsurface->surface->height);

	box = pixman_region32_extents(&region);
	struct weston_geometry geometry = {
		.x = box->x1,
		.y = box->y1,
		.width = box->x2 - box->x1,
		.height = box->y2 - box->y1,
	};

	pixman_region32_fini(&region);

	return geometry;
}

/** Copy surface contents to system memory.
 *
 * \param surface The surface to copy from.
 * \param target Pointer to the target memory buffer.
 * \param size Size of the target buffer in bytes.
 * \param src_x X location on contents to copy from.
 * \param src_y Y location on contents to copy from.
 * \param width Width in pixels of the area to copy.
 * \param height Height in pixels of the area to copy.
 * \return 0 for success, -1 for failure.
 *
 * Surface contents are maintained by the renderer. They can be in a
 * reserved weston_buffer or as a copy, e.g. a GL texture, or something
 * else.
 *
 * Surface contents are copied into memory pointed to by target,
 * which has size bytes of space available. The target memory
 * may be larger than needed, but being smaller returns an error.
 * The extra bytes in target may or may not be written; their content is
 * unspecified. Size must be large enough to hold the image.
 *
 * The image in the target memory will be arranged in rows from
 * top to bottom, and pixels on a row from left to right. The pixel
 * format is PIXMAN_a8b8g8r8, 4 bytes per pixel, and stride is exactly
 * width * 4.
 *
 * Parameters src_x and src_y define the upper-left corner in buffer
 * coordinates (pixels) to copy from. Parameters width and height
 * define the size of the area to copy in pixels.
 *
 * The rectangle defined by src_x, src_y, width, height must fit in
 * the surface contents. Otherwise an error is returned.
 *
 * Use weston_surface_get_content_size to determine the content size; the
 * needed target buffer size and rectangle limits.
 *
 * CURRENT IMPLEMENTATION RESTRICTIONS:
 * - the machine must be little-endian due to Pixman formats.
 *
 * NOTE: Pixman formats are premultiplied.
 */
WL_EXPORT int
weston_surface_copy_content(struct weston_surface *surface,
			    void *target, size_t size,
			    int src_x, int src_y,
			    int width, int height)
{
	struct weston_renderer *rer = surface->compositor->renderer;
	int cw, ch;
	const size_t bytespp = 4; /* PIXMAN_a8b8g8r8 */

	if (!rer->surface_copy_content)
		return -1;

	weston_surface_get_content_size(surface, &cw, &ch);

	if (src_x < 0 || src_y < 0)
		return -1;

	if (width <= 0 || height <= 0)
		return -1;

	if (src_x + width > cw || src_y + height > ch)
		return -1;

	if (width * bytespp * height > size)
		return -1;

	return rer->surface_copy_content(surface, target, size,
					 src_x, src_y, width, height);
}

static void
subsurface_set_position(struct wl_client *client,
			struct wl_resource *resource, int32_t x, int32_t y)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);

	if (!sub)
		return;

	sub->position.offset = weston_coord_surface(x, y, sub->surface);
	sub->position.changed = true;
}

static struct weston_subsurface *
subsurface_find_sibling(struct weston_subsurface *sub,
		       struct weston_surface *surface)
{
	struct weston_surface *parent = sub->parent;
	struct weston_subsurface *sibling;

	wl_list_for_each(sibling, &parent->subsurface_list, parent_link) {
		if (sibling->surface == surface && sibling != sub)
			return sibling;
	}

	return NULL;
}

static struct weston_subsurface *
subsurface_sibling_check(struct weston_subsurface *sub,
			 struct weston_surface *surface,
			 const char *request)
{
	struct weston_subsurface *sibling;

	sibling = subsurface_find_sibling(sub, surface);
	if (!sibling) {
		wl_resource_post_error(sub->resource,
			WL_SUBSURFACE_ERROR_BAD_SURFACE,
			"%s: wl_surface@%d is not a parent or sibling",
			request, wl_resource_get_id(surface->resource));
		return NULL;
	}

	assert(sibling->parent == sub->parent);

	return sibling;
}

static void
subsurface_place_above(struct wl_client *client,
		       struct wl_resource *resource,
		       struct wl_resource *sibling_resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(sibling_resource);
	struct weston_subsurface *sibling;

	if (!sub)
		return;

	sibling = subsurface_sibling_check(sub, surface, "place_above");
	if (!sibling)
		return;

	wl_list_remove(&sub->parent_link_pending);
	wl_list_insert(sibling->parent_link_pending.prev,
		       &sub->parent_link_pending);

	sub->reordered = true;
}

static void
subsurface_place_below(struct wl_client *client,
		       struct wl_resource *resource,
		       struct wl_resource *sibling_resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(sibling_resource);
	struct weston_subsurface *sibling;

	if (!sub)
		return;

	sibling = subsurface_sibling_check(sub, surface, "place_below");
	if (!sibling)
		return;

	wl_list_remove(&sub->parent_link_pending);
	wl_list_insert(&sibling->parent_link_pending,
		       &sub->parent_link_pending);

	sub->reordered = true;
}

static void
subsurface_set_sync(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);

	if (sub)
		sub->synchronized = 1;
}

static void
subsurface_set_desync(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);

	if (sub && sub->synchronized) {
		sub->synchronized = 0;

		/* If sub became effectively desynchronized, flush. */
		if (!weston_subsurface_is_synchronized(sub))
			weston_subsurface_synchronized_commit(sub);
	}
}

static void
weston_subsurface_unlink_parent(struct weston_subsurface *sub)
{
	wl_list_remove(&sub->parent_link);
	wl_list_remove(&sub->parent_link_pending);
	wl_list_remove(&sub->parent_destroy_listener.link);
	sub->parent = NULL;
}

static void
weston_subsurface_destroy(struct weston_subsurface *sub);

static void
subsurface_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct weston_subsurface *sub =
		container_of(listener, struct weston_subsurface,
			     surface_destroy_listener);
	assert(data == sub->surface);

	/* The protocol object (wl_resource) is left inert. */
	if (sub->resource)
		wl_resource_set_user_data(sub->resource, NULL);

	weston_subsurface_destroy(sub);
}

static void
subsurface_handle_parent_destroy(struct wl_listener *listener, void *data)
{
	struct weston_subsurface *sub =
		container_of(listener, struct weston_subsurface,
			     parent_destroy_listener);
	assert(data == sub->parent);
	assert(sub->surface != sub->parent);

	if (weston_surface_is_mapped(sub->surface))
		weston_surface_unmap(sub->surface);

	weston_subsurface_unlink_parent(sub);
}

static void
subsurface_resource_destroy(struct wl_resource *resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);

	if (sub)
		weston_subsurface_destroy(sub);
}

static void
subsurface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
weston_subsurface_link_parent(struct weston_subsurface *sub,
			      struct weston_surface *parent)
{
	sub->parent = parent;
	sub->parent_destroy_listener.notify = subsurface_handle_parent_destroy;
	wl_signal_add(&parent->destroy_signal,
		      &sub->parent_destroy_listener);

	wl_list_insert(&parent->subsurface_list, &sub->parent_link);
	wl_list_insert(&parent->subsurface_list_pending,
		       &sub->parent_link_pending);
}

static void
weston_subsurface_link_surface(struct weston_subsurface *sub,
			       struct weston_surface *surface)
{
	sub->surface = surface;
	sub->surface_destroy_listener.notify =
		subsurface_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &sub->surface_destroy_listener);
}

static void
weston_subsurface_destroy(struct weston_subsurface *sub)
{
	struct weston_view *view, *next;

	assert(sub->surface);

	if (sub->resource) {
		assert(weston_surface_to_subsurface(sub->surface) == sub);
		assert(sub->parent_destroy_listener.notify ==
		       subsurface_handle_parent_destroy);

		wl_list_for_each_safe(view, next, &sub->surface->views, surface_link) {
			weston_view_unmap(view);
			weston_view_destroy(view);
		}

		if (sub->parent)
			weston_subsurface_unlink_parent(sub);

		weston_surface_state_fini(&sub->cached);
		weston_buffer_reference(&sub->cached_buffer_ref, NULL,
					BUFFER_WILL_NOT_BE_ACCESSED);

		sub->surface->committed = NULL;
		sub->surface->committed_private = NULL;
		weston_surface_set_label_func(sub->surface, NULL);
	} else {
		/* the dummy weston_subsurface for the parent itself */
		assert(sub->parent_destroy_listener.notify == NULL);
		wl_list_remove(&sub->parent_link);
		wl_list_remove(&sub->parent_link_pending);
	}

	wl_list_remove(&sub->surface_destroy_listener.link);
	free(sub);
}

static const struct wl_subsurface_interface subsurface_implementation = {
	subsurface_destroy,
	subsurface_set_position,
	subsurface_place_above,
	subsurface_place_below,
	subsurface_set_sync,
	subsurface_set_desync
};

static struct weston_subsurface *
weston_subsurface_create(uint32_t id, struct weston_surface *surface,
			 struct weston_surface *parent)
{
	struct weston_subsurface *sub;
	struct wl_client *client = wl_resource_get_client(surface->resource);

	sub = zalloc(sizeof *sub);
	if (sub == NULL)
		return NULL;

	wl_list_init(&sub->unused_views);

	sub->resource =
		wl_resource_create(client, &wl_subsurface_interface, 1, id);
	if (!sub->resource) {
		free(sub);
		return NULL;
	}

	sub->position.offset = weston_coord_surface(0, 0, surface);

	wl_resource_set_implementation(sub->resource,
				       &subsurface_implementation,
				       sub, subsurface_resource_destroy);
	weston_subsurface_link_surface(sub, surface);
	weston_subsurface_link_parent(sub, parent);
	weston_surface_state_init(&sub->cached);
	sub->cached_buffer_ref.buffer = NULL;
	sub->synchronized = 1;

	return sub;
}

/* Create a dummy subsurface for having the parent itself in its
 * sub-surface lists. Makes stacking order manipulation easy.
 */
static struct weston_subsurface *
weston_subsurface_create_for_parent(struct weston_surface *parent)
{
	struct weston_subsurface *sub;

	sub = zalloc(sizeof *sub);
	if (sub == NULL)
		return NULL;

	weston_subsurface_link_surface(sub, parent);
	sub->parent = parent;
	wl_list_insert(&parent->subsurface_list, &sub->parent_link);
	wl_list_insert(&parent->subsurface_list_pending,
		       &sub->parent_link_pending);

	return sub;
}

static void
subcompositor_get_subsurface(struct wl_client *client,
			     struct wl_resource *resource,
			     uint32_t id,
			     struct wl_resource *surface_resource,
			     struct wl_resource *parent_resource)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_surface *parent =
		wl_resource_get_user_data(parent_resource);
	struct weston_subsurface *sub;
	static const char where[] = "get_subsurface: wl_subsurface@";

	if (surface == parent) {
		wl_resource_post_error(resource,
			WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
			"%s%d: wl_surface@%d cannot be its own parent",
			where, id, wl_resource_get_id(surface_resource));
		return;
	}

	if (weston_surface_to_subsurface(surface)) {
		wl_resource_post_error(resource,
			WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
			"%s%d: wl_surface@%d is already a sub-surface",
			where, id, wl_resource_get_id(surface_resource));
		return;
	}

	if (weston_surface_set_role(surface, "wl_subsurface", resource,
				    WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE) < 0)
		return;

	if (weston_surface_get_main_surface(parent) == surface) {
		wl_resource_post_error(resource,
			WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
			"%s%d: wl_surface@%d is an ancestor of parent",
			where, id, wl_resource_get_id(surface_resource));
		return;
	}

	/* make sure the parent is in its own list */
	if (wl_list_empty(&parent->subsurface_list)) {
		if (!weston_subsurface_create_for_parent(parent)) {
			wl_resource_post_no_memory(resource);
			return;
		}
	}

	sub = weston_subsurface_create(id, surface, parent);
	if (!sub) {
		wl_resource_post_no_memory(resource);
		return;
	}

	surface->committed = subsurface_committed;
	surface->committed_private = sub;
	weston_surface_set_label_func(surface, subsurface_get_label);
}

static void
subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_subcompositor_interface subcompositor_interface = {
	subcompositor_destroy,
	subcompositor_get_subsurface
};

static void
bind_subcompositor(struct wl_client *client,
		   void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource =
		wl_resource_create(client, &wl_subcompositor_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &subcompositor_interface,
				       compositor, NULL);
}

/** Set a DPMS mode on all of the compositor's outputs
 *
 * \param compositor The compositor instance
 * \param state The DPMS state the outputs will be set to
 */
static void
weston_compositor_dpms(struct weston_compositor *compositor,
		       enum dpms_enum state)
{
	struct weston_output *output;
	enum dpms_enum dpms;

	wl_list_for_each(output, &compositor->output_list, link) {
		dpms = output->power_state == WESTON_OUTPUT_POWER_FORCED_OFF ? WESTON_DPMS_OFF : state;
		if (output->set_dpms)
			output->set_dpms(output, dpms);
	}
}

/** Restores the compositor to active status
 *
 * \param compositor The compositor instance
 *
 * If the compositor was in a sleeping mode, all outputs are powered
 * back on via DPMS.  Otherwise if the compositor was inactive
 * (idle/locked, offscreen, or sleeping) then the compositor's wake
 * signal will fire.
 *
 * Restarts the idle timer.
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_wake(struct weston_compositor *compositor)
{
	uint32_t old_state = compositor->state;

	/* The state needs to be changed before emitting the wake
	 * signal because that may try to schedule a repaint which
	 * will not work if the compositor is still sleeping */
	compositor->state = WESTON_COMPOSITOR_ACTIVE;

	switch (old_state) {
	case WESTON_COMPOSITOR_SLEEPING:
	case WESTON_COMPOSITOR_IDLE:
	case WESTON_COMPOSITOR_OFFSCREEN:
		weston_compositor_dpms(compositor, WESTON_DPMS_ON);
		wl_signal_emit(&compositor->wake_signal, compositor);
		/* fall through */
	default:
		wl_event_source_timer_update(compositor->idle_source,
					     compositor->idle_time * 1000);
	}
}

/** Turns off rendering and frame events for the compositor.
 *
 * \param compositor The compositor instance
 *
 * This is used for example to prevent further rendering while the
 * compositor is shutting down.
 *
 * Stops the idle timer.
 *
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_offscreen(struct weston_compositor *compositor)
{
	switch (compositor->state) {
	case WESTON_COMPOSITOR_OFFSCREEN:
		return;
	case WESTON_COMPOSITOR_SLEEPING:
	default:
		compositor->state = WESTON_COMPOSITOR_OFFSCREEN;
		wl_event_source_timer_update(compositor->idle_source, 0);
	}
}

/** Powers down all attached output devices
 *
 * \param compositor The compositor instance
 *
 * Causes rendering to the outputs to cease, and no frame events to be
 * sent.  Only powers down the outputs if the compositor is not already
 * in sleep mode.
 *
 * Stops the idle timer.
 *
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_sleep(struct weston_compositor *compositor)
{
	if (compositor->state == WESTON_COMPOSITOR_SLEEPING)
		return;

	wl_event_source_timer_update(compositor->idle_source, 0);
	compositor->state = WESTON_COMPOSITOR_SLEEPING;
	weston_compositor_dpms(compositor, WESTON_DPMS_OFF);
}

/** Sets compositor to idle mode
 *
 * \param data The compositor instance
 *
 * This is called when the idle timer fires.  Once the compositor is in
 * idle mode it requires a wake action (e.g. via
 * weston_compositor_wake()) to restore it.  The compositor's
 * idle_signal will be triggered when the idle event occurs.
 *
 * Idleness can be inhibited by setting the compositor's idle_inhibit
 * property.
 */
static int
idle_handler(void *data)
{
	struct weston_compositor *compositor = data;

	if (compositor->idle_inhibit)
		return 1;

	compositor->state = WESTON_COMPOSITOR_IDLE;
	wl_signal_emit(&compositor->idle_signal, compositor);

	return 1;
}

WL_EXPORT void
weston_plane_init(struct weston_plane *plane, struct weston_compositor *ec)
{
	pixman_region32_init(&plane->damage);
	pixman_region32_init(&plane->clip);
	plane->x = 0;
	plane->y = 0;
	plane->compositor = ec;

	/* Init the link so that the call to wl_list_remove() when releasing
	 * the plane without ever stacking doesn't lead to a crash */
	wl_list_init(&plane->link);
}

WL_EXPORT void
weston_plane_release(struct weston_plane *plane)
{
	struct weston_view *view;

	pixman_region32_fini(&plane->damage);
	pixman_region32_fini(&plane->clip);

	/*
	 * Can't use paint node list here, weston_plane is not specific to an
	 * output.
	 */
	wl_list_for_each(view, &plane->compositor->view_list, link) {
		if (view->plane == plane)
			view->plane = NULL;
	}

	wl_list_remove(&plane->link);
}

/** weston_compositor_stack_plane
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_stack_plane(struct weston_compositor *ec,
			      struct weston_plane *plane,
			      struct weston_plane *above)
{
	if (above)
		wl_list_insert(above->link.prev, &plane->link);
	else
		wl_list_insert(&ec->plane_list, &plane->link);
}

static void
output_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_output_interface output_interface = {
	output_release,
};


static void unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
bind_output(struct wl_client *client,
	    void *data, uint32_t version, uint32_t id)
{
	struct weston_head *head = data;
	struct weston_output *output = head->output;
	struct weston_mode *mode;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_output_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (!output) {
		wl_resource_set_implementation(resource, &output_interface,
					       NULL, NULL);
		return;
	}

	wl_list_insert(&head->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &output_interface, head,
				       unbind_resource);

	wl_output_send_geometry(resource,
				output->x,
				output->y,
				head->mm_width,
				head->mm_height,
				head->subpixel,
				head->make, head->model,
				output->transform);
	if (version >= WL_OUTPUT_SCALE_SINCE_VERSION)
		wl_output_send_scale(resource,
				     output->current_scale);

	wl_list_for_each (mode, &output->mode_list, link) {
		wl_output_send_mode(resource,
				    mode->flags,
				    mode->width,
				    mode->height,
				    mode->refresh);
	}

	if (version >= WL_OUTPUT_NAME_SINCE_VERSION)
		wl_output_send_name(resource, head->name);

	if (version >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION)
		wl_output_send_description(resource, head->model);

	if (version >= WL_OUTPUT_DONE_SINCE_VERSION)
		wl_output_send_done(resource);
}

static void
weston_head_add_global(struct weston_head *head)
{
	head->global = wl_global_create(head->compositor->wl_display,
					&wl_output_interface, 4,
					head, bind_output);
}

struct weston_destroy_global_data {
	struct wl_global *global;
	struct wl_event_source *event_source;
	struct wl_listener destroy_listener;
};

static void
weston_destroy_global(struct weston_destroy_global_data *data)
{
	wl_list_remove(&data->destroy_listener.link);
	wl_global_destroy(data->global);
	wl_event_source_remove(data->event_source);
	free(data);
}

static void
global_compositor_destroy_handler(struct wl_listener *listener, void *_data)
{
	struct weston_destroy_global_data *data =
		wl_container_of(listener, data, destroy_listener);

	weston_destroy_global(data);
}

static int
weston_global_handle_timer_event(void *data)
{
	weston_destroy_global(data);
	return 0;
}

static void
weston_global_destroy_save(struct weston_compositor *compositor,
			   struct wl_global *global)
{
	struct weston_destroy_global_data *data;
	struct wl_event_loop *loop;

	if (compositor->state == WESTON_COMPOSITOR_OFFSCREEN) {
		wl_global_destroy(global);
		return;
	}

	wl_global_remove(global);

	data = xzalloc(sizeof *data);
	data->global = global;

	loop = wl_display_get_event_loop(compositor->wl_display);
	data->event_source =
		wl_event_loop_add_timer(loop, weston_global_handle_timer_event,
					data);
	wl_event_source_timer_update(data->event_source, 5000);

	data->destroy_listener.notify = global_compositor_destroy_handler;
	wl_signal_add(&compositor->destroy_signal, &data->destroy_listener);
}

/** Remove the global wl_output protocol object
 *
 * \param head The head whose global to remove.
 *
 * Also orphans the wl_resources for this head (wl_output).
 */
static void
weston_head_remove_global(struct weston_head *head)
{
	struct wl_resource *resource, *tmp;

	if (head->global)
		weston_global_destroy_save(head->compositor, head->global);
	head->global = NULL;

	wl_resource_for_each_safe(resource, tmp, &head->resource_list) {
		unbind_resource(resource);
		wl_resource_set_destructor(resource, NULL);
		wl_resource_set_user_data(resource, NULL);
	}

	wl_resource_for_each(resource, &head->xdg_output_resource_list) {
		/* It's sufficient to unset the destructor, then the list elements
		 * won't be accessed.
		 */
		wl_resource_set_destructor(resource, NULL);
	}
	wl_list_init(&head->xdg_output_resource_list);
}

/** Get the backing object of wl_output
 *
 * \param resource A wl_output protocol object.
 * \return The backing object (user data) of a wl_resource representing a
 * wl_output protocol object.
 *
 * \ingroup head
 */
WL_EXPORT struct weston_head *
weston_head_from_resource(struct wl_resource *resource)
{
	assert(wl_resource_instance_of(resource, &wl_output_interface,
				       &output_interface));

	return wl_resource_get_user_data(resource);
}

/** Initialize a pre-allocated weston_head
 *
 * \param head The head to initialize.
 * \param name The head name, e.g. the connector name or equivalent.
 *
 * The head will be safe to attach, detach and release.
 *
 * The name is used in logs, and can be used by compositors as a configuration
 * identifier.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_init(struct weston_head *head, const char *name)
{
	/* Add some (in)sane defaults which can be used
	 * for checking if an output was properly configured
	 */
	memset(head, 0, sizeof *head);

	wl_list_init(&head->compositor_link);
	wl_signal_init(&head->destroy_signal);
	wl_list_init(&head->output_link);
	wl_list_init(&head->resource_list);
	wl_list_init(&head->xdg_output_resource_list);
	head->name = strdup(name);
	head->supported_eotf_mask = WESTON_EOTF_MODE_SDR;
	head->current_protection = WESTON_HDCP_DISABLE;
}

/** Send output heads changed signal
 *
 * \param output The output that changed.
 *
 * Notify that the enabled output gained and/or lost heads, or that the
 * associated heads may have changed their connection status. This does not
 * include cases where the output becomes enabled or disabled. The registered
 * callbacks are called after the change has successfully happened.
 *
 * If connection status change causes the compositor to attach or detach a head
 * to an enabled output, the registered callbacks may be called multiple times.
 *
 * \ingroup output
 */
static void
weston_output_emit_heads_changed(struct weston_output *output)
{
	wl_signal_emit(&output->compositor->output_heads_changed_signal,
		       output);
}

/** Idle task for emitting heads_changed_signal */
static void
weston_compositor_call_heads_changed(void *data)
{
	struct weston_compositor *compositor = data;
	struct weston_head *head;

	compositor->heads_changed_source = NULL;

	wl_signal_emit(&compositor->heads_changed_signal, compositor);

	wl_list_for_each(head, &compositor->head_list, compositor_link) {
		if (head->output && head->output->enabled)
			weston_output_emit_heads_changed(head->output);
	}
}

/** Schedule a call on idle to heads_changed callback
 *
 * \param compositor The Compositor.
 *
 * \ingroup compositor
 * \internal
 */
static void
weston_compositor_schedule_heads_changed(struct weston_compositor *compositor)
{
	struct wl_event_loop *loop;

	if (compositor->heads_changed_source)
		return;

	loop = wl_display_get_event_loop(compositor->wl_display);
	compositor->heads_changed_source = wl_event_loop_add_idle(loop,
			weston_compositor_call_heads_changed, compositor);
}

/** Register a new head
 *
 * \param compositor The compositor.
 * \param head The head to register, must not be already registered.
 *
 * This signals the core that a new head has become available, leading to
 * heads_changed hook being called later.
 *
 * \ingroup compositor
 * \internal
 */
WL_EXPORT void
weston_compositor_add_head(struct weston_compositor *compositor,
			   struct weston_head *head)
{
	assert(wl_list_empty(&head->compositor_link));
	assert(head->name);

	wl_list_insert(compositor->head_list.prev, &head->compositor_link);
	head->compositor = compositor;
	weston_compositor_schedule_heads_changed(compositor);
}

/** Adds a listener to be called when heads change
 *
 * \param compositor The compositor.
 * \param listener The listener to add.
 *
 * The listener notify function argument is weston_compositor.
 *
 * The listener function will be called after heads are added or their
 * connection status has changed. Several changes may be accumulated into a
 * single call. The user is expected to iterate over the existing heads and
 * check their statuses to find out what changed.
 *
 * \sa weston_compositor_iterate_heads, weston_head_is_connected,
 * weston_head_is_enabled
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_add_heads_changed_listener(struct weston_compositor *compositor,
					     struct wl_listener *listener)
{
	wl_signal_add(&compositor->heads_changed_signal, listener);
}

/** Iterate over available heads
 *
 * \param compositor The compositor.
 * \param iter The iterator, or NULL for start.
 * \return The next available head in the list.
 *
 * Returns all available heads, regardless of being connected or enabled.
 *
 * You can iterate over all heads as follows:
 * \code
 * struct weston_head *head = NULL;
 *
 * while ((head = weston_compositor_iterate_heads(compositor, head))) {
 * 	...
 * }
 * \endcode
 *
 *  If you cause \c iter to be removed from the list, you cannot use it to
 * continue iterating. Removing any other item is safe.
 *
 * \ingroup compositor
 */
WL_EXPORT struct weston_head *
weston_compositor_iterate_heads(struct weston_compositor *compositor,
				struct weston_head *iter)
{
	struct wl_list *list = &compositor->head_list;
	struct wl_list *node;

	assert(compositor);
	assert(!iter || iter->compositor == compositor);

	if (iter)
		node = iter->compositor_link.next;
	else
		node = list->next;

	assert(node);
	assert(!iter || node != &iter->compositor_link);

	if (node == list)
		return NULL;

	return container_of(node, struct weston_head, compositor_link);
}

/** Iterate over attached heads
 *
 * \param output The output whose heads to iterate.
 * \param iter The iterator, or NULL for start.
 * \return The next attached head in the list.
 *
 * Returns all heads currently attached to the output.
 *
 * You can iterate over all heads as follows:
 * \code
 * struct weston_head *head = NULL;
 *
 * while ((head = weston_output_iterate_heads(output, head))) {
 * 	...
 * }
 * \endcode
 *
 *  If you cause \c iter to be removed from the list, you cannot use it to
 * continue iterating. Removing any other item is safe.
 *
 * \ingroup output
 */
WL_EXPORT struct weston_head *
weston_output_iterate_heads(struct weston_output *output,
			    struct weston_head *iter)
{
	struct wl_list *list = &output->head_list;
	struct wl_list *node;

	assert(output);
	assert(!iter || iter->output == output);

	if (iter)
		node = iter->output_link.next;
	else
		node = list->next;

	assert(node);
	assert(!iter || node != &iter->output_link);

	if (node == list)
		return NULL;

	return container_of(node, struct weston_head, output_link);
}

static void
weston_output_compute_protection(struct weston_output *output)
{
	struct weston_head *head;
	enum weston_hdcp_protection op_protection;
	bool op_protection_valid = false;
	struct weston_compositor *wc = output->compositor;

	wl_list_for_each(head, &output->head_list, output_link) {
		if (!op_protection_valid) {
			op_protection = head->current_protection;
			op_protection_valid = true;
		}
		if (head->current_protection < op_protection)
			op_protection = head->current_protection;
	}

	if (!op_protection_valid)
		op_protection = WESTON_HDCP_DISABLE;

	if (output->current_protection != op_protection) {
		output->current_protection = op_protection;
		weston_output_damage(output);
		weston_schedule_surface_protection_update(wc);
	}
}

/** Attach a head to an output
 *
 * \param output The output to attach to.
 * \param head The head that is not yet attached.
 * \return 0 on success, -1 on failure.
 *
 * Attaches the given head to the output. All heads of an output are clones
 * and share the resolution and timings.
 *
 * Cloning heads this way uses less resources than creating an output for
 * each head, but is not always possible due to environment, driver and hardware
 * limitations.
 *
 * On failure, the head remains unattached. Success of this function does not
 * guarantee the output configuration is actually valid. The final checks are
 * made on weston_output_enable() unless the output was already enabled.
 *
 * \ingroup output
 */
WL_EXPORT int
weston_output_attach_head(struct weston_output *output,
			  struct weston_head *head)
{
	char *head_names;

	if (!wl_list_empty(&head->output_link))
		return -1;

	if (output->attach_head) {
		if (output->attach_head(output, head) < 0)
			return -1;
	} else if (!wl_list_empty(&output->head_list)) {
		/* No support for clones in the legacy path. */
		return -1;
	}

	head->output = output;
	wl_list_insert(output->head_list.prev, &head->output_link);

	weston_output_compute_protection(output);

	if (output->enabled) {
		weston_head_add_global(head);

		head_names = weston_output_create_heads_string(output);
		weston_log("Output '%s' updated to have head(s) %s\n",
			   output->name, head_names);
		free(head_names);

		weston_output_emit_heads_changed(output);
	}

	return 0;
}

/** Detach a head from its output
 *
 * \param head The head to detach.
 *
 * It is safe to detach a non-attached head.
 *
 * If the head is attached to an enabled output and the output will be left
 * with no heads, the output will be disabled.
 *
 * \ingroup head
 * \sa weston_output_disable
 */
WL_EXPORT void
weston_head_detach(struct weston_head *head)
{
	struct weston_output *output = head->output;
	char *head_names;

	wl_list_remove(&head->output_link);
	wl_list_init(&head->output_link);
	head->output = NULL;

	if (!output)
		return;

	if (output->detach_head)
		output->detach_head(output, head);

	if (output->enabled) {
		weston_head_remove_global(head);

		if (wl_list_empty(&output->head_list)) {
			weston_log("Output '%s' no heads left, disabling.\n",
				   output->name);
			weston_output_disable(output);
		} else {
			head_names = weston_output_create_heads_string(output);
			weston_log("Output '%s' updated to have head(s) %s\n",
				   output->name, head_names);
			free(head_names);

			weston_output_emit_heads_changed(output);
		}
	}
}

/** Destroy a head
 *
 * \param head The head to be released.
 *
 * Destroys the head. The caller is responsible for freeing the memory pointed
 * to by \c head.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_release(struct weston_head *head)
{
	weston_signal_emit_mutable(&head->destroy_signal, head);

	weston_head_detach(head);

	free(head->make);
	free(head->model);
	free(head->serial_number);
	free(head->name);

	wl_list_remove(&head->compositor_link);
}

/** Propagate device information changes
 *
 * \param head The head that changed.
 *
 * The information about the connected display device, e.g. a monitor, may
 * change without being disconnected in between. Changing information
 * causes a call to the heads_changed hook.
 *
 * Normally this is handled automatically by the generic setters, but if
 * a backend has
 * specific head properties it may have to call this directly.
 *
 * \sa weston_head_reset_device_changed, weston_compositor_set_heads_changed_cb,
 * weston_head_is_device_changed
 * \ingroup head
 */
WL_EXPORT void
weston_head_set_device_changed(struct weston_head *head)
{
	head->device_changed = true;

	if (head->compositor)
		weston_compositor_schedule_heads_changed(head->compositor);
}

/** String equal comparison with NULLs being equal */
static bool
str_null_eq(const char *a, const char *b)
{
	if (!a && !b)
		return true;

	if (!!a != !!b)
		return false;

	return strcmp(a, b) == 0;
}

/** Store monitor make, model and serial number
 *
 * \param head The head to modify.
 * \param make The monitor make. If EDID is available, the PNP ID. Otherwise
 * any string, or NULL for none.
 * \param model The monitor model or name, or a made-up string, or NULL for
 * none.
 * \param serialno The monitor serial number, a made-up string, or NULL for
 * none.
 *
 * This may set the device_changed flag.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_monitor_strings(struct weston_head *head,
				const char *make,
				const char *model,
				const char *serialno)
{
	if (str_null_eq(head->make, make) &&
	    str_null_eq(head->model, model) &&
	    str_null_eq(head->serial_number, serialno))
		return;

	free(head->make);
	free(head->model);
	free(head->serial_number);

	head->make = make ? strdup(make) : NULL;
	head->model = model ? strdup(model) : NULL;
	head->serial_number = serialno ? strdup(serialno) : NULL;

	weston_head_set_device_changed(head);
}

/** Store display non-desktop status
 *
 * \param head The head to modify.
 * \param non_desktop Whether the head connects to a non-desktop display.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_non_desktop(struct weston_head *head, bool non_desktop)
{
	if (head->non_desktop == non_desktop)
		return;

	head->non_desktop = non_desktop;

	weston_head_set_device_changed(head);
}

/** Store display transformation
 *
 * \param head The head to modify.
 * \param transform The transformation to apply for this head
 *
 * This may set the device_changed flag.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_transform(struct weston_head *head, uint32_t transform)
{
	if (head->transform == transform)
		return;

	head->transform = transform;

	weston_head_set_device_changed(head);
}


/** Store physical image size
 *
 * \param head The head to modify.
 * \param mm_width Image area width in millimeters.
 * \param mm_height Image area height in millimeters.
 *
 * This may set the device_changed flag.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_physical_size(struct weston_head *head,
			      int32_t mm_width, int32_t mm_height)
{
	if (head->mm_width == mm_width &&
	    head->mm_height == mm_height)
		return;

	head->mm_width = mm_width;
	head->mm_height = mm_height;

	weston_head_set_device_changed(head);
}

/** Store monitor sub-pixel layout
 *
 * \param head The head to modify.
 * \param sp Sub-pixel layout. The possible values are:
 * - WL_OUTPUT_SUBPIXEL_UNKNOWN,
 * - WL_OUTPUT_SUBPIXEL_NONE,
 * - WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB,
 * - WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR,
 * - WL_OUTPUT_SUBPIXEL_VERTICAL_RGB,
 * - WL_OUTPUT_SUBPIXEL_VERTICAL_BGR
 *
 * This may set the device_changed flag.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_subpixel(struct weston_head *head,
			 enum wl_output_subpixel sp)
{
	if (head->subpixel == sp)
		return;

	head->subpixel = sp;

	weston_head_set_device_changed(head);
}

/** Mark the monitor as internal
 *
 * This is used for embedded screens, like laptop panels.
 *
 * \param head The head to mark as internal.
 *
 * By default a head is external. The type is often inferred from the physical
 * connector type.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_internal(struct weston_head *head)
{
	head->connection_internal = true;
}

/** Store connector status
 *
 * \param head The head to modify.
 * \param connected Whether the head is connected.
 *
 * Connectors are created as disconnected. This function can be used to
 * set the connector status.
 *
 * The status should be set to true when a physical connector is connected to
 * a video sink device like a monitor and to false when the connector is
 * disconnected. For nested backends, the connection status should reflect the
 * connection to the parent display server.
 *
 * When the connection status changes, it schedules a call to the heads_changed
 * hook and sets the device_changed flag.
 *
 * \sa weston_compositor_set_heads_changed_cb
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_connection_status(struct weston_head *head, bool connected)
{
	if (head->connected == connected)
		return;

	head->connected = connected;

	weston_head_set_device_changed(head);
}

/** Store the set of supported EOTF modes
 *
 * \param head The head to modify.
 * \param eotf_mask A bit mask with the possible bits or'ed together from
 * enum weston_eotf_mode.
 *
 * This may set the device_changed flag.
 *
 * \ingroup head
 * \internal
 */
WL_EXPORT void
weston_head_set_supported_eotf_mask(struct weston_head *head,
				    uint32_t eotf_mask)
{
	assert((eotf_mask & ~WESTON_EOTF_MODE_ALL_MASK) == 0);

	if (head->supported_eotf_mask == eotf_mask)
		return;

	head->supported_eotf_mask = eotf_mask;

	weston_head_set_device_changed(head);
}

WL_EXPORT void
weston_head_set_content_protection_status(struct weston_head *head,
					  enum weston_hdcp_protection status)
{
	head->current_protection = status;
	if (head->output)
		weston_output_compute_protection(head->output);
}

/** Is the head currently connected?
 *
 * \param head The head to query.
 * \return Connection status.
 *
 * Returns true if the head is physically connected to a monitor, or in
 * case of a nested backend returns true when there is a connection to the
 * parent display server.
 *
 * This is independent from the head being enabled.
 *
 * \sa weston_head_is_enabled
 * \ingroup head
 */
WL_EXPORT bool
weston_head_is_connected(struct weston_head *head)
{
	return head->connected;
}

/** Is the head currently enabled?
 *
 * \param head The head to query.
 * \return Video status.
 *
 * Returns true if the head is currently transmitting a video stream.
 *
 * This is independent of the head being connected.
 *
 * \sa weston_head_is_connected
 * \ingroup head
 */
WL_EXPORT bool
weston_head_is_enabled(struct weston_head *head)
{
	if (!head->output)
		return false;

	return head->output->enabled;
}

/** Has the device information changed?
 *
 * \param head The head to query.
 * \return True if the device information has changed since last reset.
 *
 * The information about the connected display device, e.g. a monitor, may
 * change without being disconnected in between. Changing information
 * causes a call to the heads_changed hook.
 *
 * The information includes make, model, serial number, physical size,
 * and sub-pixel type. The connection status is also included.
 *
 * \sa weston_head_reset_device_changed, weston_compositor_set_heads_changed_cb
 * \ingroup head
 */
WL_EXPORT bool
weston_head_is_device_changed(struct weston_head *head)
{
	return head->device_changed;
}

/** Does the head represent a non-desktop display?
 *
 * \param head The head to query.
 * \return True if the device is a non-desktop display.
 *
 * Non-desktop heads are not attached to outputs by default.
 * This stops weston from extending the desktop onto head mounted displays.
 *
 * \ingroup head
 */
WL_EXPORT bool
weston_head_is_non_desktop(struct weston_head *head)
{
	return head->non_desktop;
}

/** Acknowledge device information change
 *
 * \param head The head to acknowledge.
 *
 * Clears the device changed flag on this head. When a compositor has processed
 * device information, it should call this to be able to notice further
 * changes.
 *
 * \sa weston_head_is_device_changed
 * \ingroup head
 */
WL_EXPORT void
weston_head_reset_device_changed(struct weston_head *head)
{
	head->device_changed = false;
}

/** Get the name of a head
 *
 * \param head The head to query.
 * \return The head's name, not NULL.
 *
 * The name depends on the backend. The DRM backend uses connector names,
 * other backends may use hardcoded names or user-given names.
 *
 * \ingroup head
 */
WL_EXPORT const char *
weston_head_get_name(struct weston_head *head)
{
	return head->name;
}

/** Get the output the head is attached to
 *
 * \param head The head to query.
 * \return The output the head is attached to, or NULL if detached.
 * \ingroup head
 */
WL_EXPORT struct weston_output *
weston_head_get_output(struct weston_head *head)
{
	return head->output;
}

/** Get the head's native transformation
 *
 * \param head The head to query.
 * \return The head's native transform, as a WL_OUTPUT_TRANSFORM_* value
 *
 * A weston_head may have a 'native' transform provided by the backend.
 * Examples include panels which are physically rotated, where the rotation
 * is recorded and described as part of the system configuration. This call
 * will return any known native transform for the head.
 *
 * \ingroup head
 */
WL_EXPORT uint32_t
weston_head_get_transform(struct weston_head *head)
{
	return head->transform;
}

/** Add destroy callback for a head
 *
 * \param head The head to watch for.
 * \param listener The listener to add. The \c notify member must be set.
 *
 * Heads may get destroyed for various reasons by the backends. If a head is
 * attached to an output, the compositor should listen for head destruction
 * and reconfigure or destroy the output if necessary.
 *
 * The destroy callbacks will be called on weston_head destruction before any
 * automatic detaching from an associated weston_output and before any
 * weston_head information is lost.
 *
 * The \c data argument to the notify callback is the weston_head being
 * destroyed.
 *
 * \ingroup head
 */
WL_EXPORT void
weston_head_add_destroy_listener(struct weston_head *head,
				 struct wl_listener *listener)
{
	wl_signal_add(&head->destroy_signal, listener);
}

/** Look up destroy listener for a head
 *
 * \param head The head to query.
 * \param notify The notify function used used for the added destroy listener.
 * \return The listener, or NULL if not found.
 *
 * This looks up the previously added destroy listener struct based on the
 * notify function it has. The listener can be used to access user data
 * through \c container_of().
 *
 * \sa wl_signal_get()
 * \ingroup head
 */
WL_EXPORT struct wl_listener *
weston_head_get_destroy_listener(struct weston_head *head,
				 wl_notify_func_t notify)
{
	return wl_signal_get(&head->destroy_signal, notify);
}

static void
weston_output_set_position(struct weston_output *output,
			   struct weston_coord_global pos);

/* Move other outputs when one is resized so the space remains contiguous. */
static void
weston_compositor_reflow_outputs(struct weston_compositor *compositor,
				struct weston_output *resized_output, int delta_width)
{
	struct weston_output *output;
	bool start_resizing = false;

	if (compositor->output_flow_dirty)
		return;

	if (!delta_width)
		return;

	wl_list_for_each(output, &compositor->output_list, link) {
		if (output == resized_output) {
			start_resizing = true;
			continue;
		}

		if (start_resizing) {
			struct weston_coord_global pos;

			pos.c = weston_coord(output->x + delta_width,
					     output->y);
			weston_output_set_position(output, pos);
		}
	}
}

/** Transform a region from global to output coordinates
 *
 * \param dst The region transformed into output coordinates
 * \param output The output that defines the transformation.
 * \param src The region to be transformed, in global coordinates.
 *
 * This takes a region in the global coordinate system, and takes into account
 * output position, transform and scale, and converts the region into output
 * pixel coordinates in the framebuffer.
 *
 * \internal
 * \ingroup output
 */
WL_EXPORT void
weston_region_global_to_output(pixman_region32_t *dst,
			       struct weston_output *output,
			       pixman_region32_t *src)
{
	weston_matrix_transform_region(dst, &output->matrix, src);
}

WESTON_EXPORT_FOR_TESTS void
weston_output_update_matrix(struct weston_output *output)
{
	weston_output_dirty_paint_nodes(output);

	weston_matrix_init_transform(&output->matrix, output->transform,
				     output->x, output->y,
				     output->width, output->height,
				     output->current_scale);

	weston_matrix_invert(&output->inverse_matrix, &output->matrix);
}

static void
weston_output_transform_scale_init(struct weston_output *output, uint32_t transform, uint32_t scale)
{
	output->transform = transform;
	output->native_scale = scale;
	output->current_scale = scale;

	convert_size_by_transform_scale(&output->width, &output->height,
					output->current_mode->width,
					output->current_mode->height,
					transform, scale);
}

static void
weston_output_init_geometry(struct weston_output *output, int x, int y)
{
	output->x = x;
	output->y = y;

	pixman_region32_fini(&output->region);
	pixman_region32_init_rect(&output->region, x, y,
				  output->width,
				  output->height);
}

/**
 * \ingroup output
 */
static void
weston_output_set_position(struct weston_output *output,
			   struct weston_coord_global pos)
{
	struct weston_head *head;
	struct wl_resource *resource;
	int ver;

	if (!output->enabled) {
		output->x = pos.c.x;
		output->y = pos.c.y;
		return;
	}

	output->move_x = pos.c.x - output->x;
	output->move_y = pos.c.y - output->y;

	if (output->move_x == 0 && output->move_y == 0)
		return;

	weston_output_init_geometry(output, pos.c.x, pos.c.y);

	weston_output_update_matrix(output);

	/* Move views on this output. */
	wl_signal_emit(&output->compositor->output_moved_signal, output);

	/* Notify clients of the change for output position. */
	wl_list_for_each(head, &output->head_list, output_link) {
		wl_resource_for_each(resource, &head->resource_list) {
			wl_output_send_geometry(resource,
						output->x,
						output->y,
						head->mm_width,
						head->mm_height,
						head->subpixel,
						head->make,
						head->model,
						output->transform);

			ver = wl_resource_get_version(resource);
			if (ver >= WL_OUTPUT_DONE_SINCE_VERSION)
				wl_output_send_done(resource);
		}

		wl_resource_for_each(resource, &head->xdg_output_resource_list) {
			zxdg_output_v1_send_logical_position(resource,
							     output->x,
							     output->y);
			zxdg_output_v1_send_done(resource);
		}
	}
}

/**
 * \ingroup output
 */
WL_EXPORT void
weston_output_move(struct weston_output *output,
		   struct weston_coord_global pos)
{
	/* XXX: we should probably perform some sanity checking here
	 * as we do for weston_output_enable, and allow moves to fail.
	 *
	 * However, while a front-end is rearranging outputs it may
	 * pass through indeterminate states where outputs overlap
	 * or are discontinuous, and this may be ok as long as no
	 * input processing or rendering occurs at that time.
	 *
	 * Ultimately, we probably need a way to pass complete output
	 * config atomically to libweston.
	 */

	output->compositor->output_flow_dirty = true;
	weston_output_set_position(output, pos);
}

/** Signal that a pending output is taken into use.
 *
 * Removes the output from the pending list and adds it to the compositor's
 * list of enabled outputs. The output created signal is emitted.
 *
 * The output gets an internal ID assigned, and the wl_output global is
 * created.
 *
 * \param compositor The compositor instance.
 * \param output The output to be added.
 *
 * \internal
 * \ingroup compositor
 */
static void
weston_compositor_add_output(struct weston_compositor *compositor,
                             struct weston_output *output)
{
	struct weston_view *view, *next;
	struct weston_head *head;

	assert(!output->enabled);

	/* Verify we haven't reached the limit of 32 available output IDs */
	assert(ffs(~compositor->output_id_pool) > 0);

	/* Invert the output id pool and look for the lowest numbered
	 * switch (the least significant bit).  Take that bit's position
	 * as our ID, and mark it used in the compositor's output_id_pool.
	 */
	output->id = ffs(~compositor->output_id_pool) - 1;
	compositor->output_id_pool |= 1u << output->id;

	wl_list_remove(&output->link);
	wl_list_insert(compositor->output_list.prev, &output->link);
	output->enabled = true;

	wl_list_for_each(head, &output->head_list, output_link)
		weston_head_add_global(head);

	wl_signal_emit(&compositor->output_created_signal, output);

	/*
	 * Use view_list, as paint nodes have not been created for this
	 * output yet. Any existing view might touch this new output.
	 */
	wl_list_for_each_safe(view, next, &compositor->view_list, link)
		weston_view_geometry_dirty(view);
}

/** Create a weston_coord_global from a point and a weston_output
 *
 * \param x x coordinate on the output
 * \param y y coordinate on the output
 * \param output the weston_output object
 * \return coordinate in global space corresponding to x, y on the output
 *
 * Transforms coordinates from the device coordinate space (physical pixel
 * units) to the global coordinate space (logical pixel units).  This takes
 * into account output transform and scale.
 *
 * \ingroup output
 * \internal
 */
WL_EXPORT struct weston_coord_global
weston_coord_global_from_output_point(double x, double y,
				      const struct weston_output *output)
{
	struct weston_coord c;
	struct weston_coord_global tmp;

	c = weston_coord(x, y);
	tmp.c = weston_matrix_transform_coord(&output->inverse_matrix, c);
	return tmp;
}

static bool
validate_float_range(float val, float min, float max)
{
	return val >= min && val <= max;
}

/* Based on CTA-861-G, HDR static metadata type 1 */
static bool
weston_hdr_metadata_type1_validate(const struct weston_hdr_metadata_type1 *md)
{
	unsigned i;

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_PRIMARIES) {
		for (i = 0; i < ARRAY_LENGTH(md->primary); i++) {
			if (!validate_float_range(md->primary[i].x, 0.0, 1.0))
				return false;
			if (!validate_float_range(md->primary[i].y, 0.0, 1.0))
				return false;
		}
	}

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_WHITE) {
		if (!validate_float_range(md->white.x, 0.0, 1.0))
			return false;
		if (!validate_float_range(md->white.y, 0.0, 1.0))
			return false;
	}

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXDML) {
		if (!validate_float_range(md->maxDML, 1.0, 65535.0))
			return false;
	}

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MINDML) {
		if (!validate_float_range(md->minDML, 0.0001, 6.5535))
			return false;
	}

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXCLL) {
		if (!validate_float_range(md->maxCLL, 1.0, 65535.0))
			return false;
	}

	if (md->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXFALL) {
		if (!validate_float_range(md->maxFALL, 1.0, 65535.0))
			return false;
	}

	return true;
}

WL_EXPORT void
weston_output_color_outcome_destroy(struct weston_output_color_outcome **pco)
{
	struct weston_output_color_outcome *co = *pco;

	if (!co)
		return;

	weston_color_transform_unref(co->from_sRGB_to_output);
	weston_color_transform_unref(co->from_sRGB_to_blend);
	weston_color_transform_unref(co->from_blend_to_output);

	free(co);
	*pco = NULL;
}

WESTON_EXPORT_FOR_TESTS bool
weston_output_set_color_outcome(struct weston_output *output)
{
	struct weston_color_manager *cm = output->compositor->color_manager;
	struct weston_output_color_outcome *colorout;

	colorout = cm->create_output_color_outcome(cm, output);
	if (!colorout) {
		weston_log("Creating color transformation for output \"%s\" failed.\n",
			   output->name);
		return false;
	}

	if (!weston_hdr_metadata_type1_validate(&colorout->hdr_meta)) {
		weston_log("Internal color manager error creating Metadata Type 1 for output \"%s\".\n",
			   output->name);
		goto out_error;
	}

	weston_output_color_outcome_destroy(&output->color_outcome);
	output->color_outcome = colorout;
	output->color_outcome_serial++;

	output->from_blend_to_output_by_backend = false;

	weston_log("Output '%s' using color profile: %s\n", output->name,
		   weston_color_profile_get_description(output->color_profile));

	return true;

out_error:
	weston_output_color_outcome_destroy(&colorout);

	return false;
}

/** Removes output from compositor's list of enabled outputs
 *
 * \param output The weston_output object that is being removed.
 *
 * The following happens:
 *
 * - Destroys all paint nodes related to the output.
 *
 * - The output assignments of all views in the current scenegraph are
 *   recomputed.
 *
 * - Destroys output's color transforms.
 *
 * - Presentation feedback is discarded.
 *
 * - Compositor is notified that outputs were changed and
 *   applies the necessary changes to re-layout outputs.
 *
 * - The output is put back in the pending outputs list.
 *
 * - Signal is emitted to notify all users of the weston_output
 *   object that the output is being destroyed.
 *
 * - wl_output protocol objects referencing this weston_output
 *   are made inert, and the wl_output global is removed.
 *
 * - The output's internal ID is released.
 *
 * \ingroup compositor
 * \internal
 */
static void
weston_compositor_remove_output(struct weston_output *output)
{
	struct weston_compositor *compositor = output->compositor;
	struct weston_paint_node *pnode, *pntmp;
	struct weston_view *view;
	struct weston_head *head;

	assert(output->destroying);
	assert(output->enabled);

	if (output->idle_repaint_source) {
		wl_event_source_remove(output->idle_repaint_source);
		output->idle_repaint_source = NULL;
	}

	wl_list_for_each_safe(pnode, pntmp,
			      &output->paint_node_list, output_link) {
		weston_paint_node_destroy(pnode);
	}
	assert(wl_list_empty(&output->paint_node_z_order_list));

	/*
	 * Use view_list in case the output did not go through repaint
	 * after a view came on it, lacking a paint node. Just to be sure.
	 */
	wl_list_for_each(view, &compositor->view_list, link) {
		if (view->output_mask & (1u << output->id))
			weston_view_assign_output(view);
	}

	weston_output_color_outcome_destroy(&output->color_outcome);

	weston_presentation_feedback_discard_list(&output->feedback_list);

	weston_compositor_reflow_outputs(compositor, output, -output->width);

	wl_list_remove(&output->link);
	wl_list_insert(compositor->pending_output_list.prev, &output->link);
	output->enabled = false;

	weston_signal_emit_mutable(&compositor->output_destroyed_signal, output);
	weston_signal_emit_mutable(&output->destroy_signal, output);

	wl_list_for_each(head, &output->head_list, output_link)
		weston_head_remove_global(head);

	weston_output_capture_info_destroy(&output->capture_info);

	compositor->output_id_pool &= ~(1u << output->id);
	output->id = 0xffffffff; /* invalid */
}

/** Sets the output scale for a given output.
 *
 * \param output The weston_output object that the scale is set for.
 * \param scale  Scale factor for the given output.
 *
 * It only supports setting scale for an output that
 * is not enabled and it can only be ran once.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_set_scale(struct weston_output *output,
			int32_t scale)
{
	output->scale = scale;
	if (!output->enabled)
		return;

	if (output->current_scale == scale)
		return;

	output->current_scale = scale;
	weston_mode_switch_finish(output, false, true);
	wl_signal_emit(&output->compositor->output_resized_signal, output);
}

/** Sets the output transform for a given output.
 *
 * \param output    The weston_output object that the transform is set for.
 * \param transform Transform value for the given output.
 *
 * Refer to wl_output::transform section located at
 * https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_output
 * for list of values that can be passed to this function.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_set_transform(struct weston_output *output,
			    uint32_t transform)
{
	struct weston_pointer_motion_event ev;
	struct wl_resource *resource;
	struct weston_seat *seat;
	pixman_region32_t old_region;
	int mid_x, mid_y;
	struct weston_head *head;
	int ver;

	if (!output->enabled && output->transform == UINT32_MAX) {
		output->transform = transform;
		return;
	}

	weston_output_transform_scale_init(output, transform, output->scale);

	pixman_region32_init(&old_region);
	pixman_region32_copy(&old_region, &output->region);

	weston_output_init_geometry(output, output->x, output->y);

	weston_output_update_matrix(output);

	/* Notify clients of the change for output transform. */
	wl_list_for_each(head, &output->head_list, output_link) {
		wl_resource_for_each(resource, &head->resource_list) {
			wl_output_send_geometry(resource,
						output->x,
						output->y,
						head->mm_width,
						head->mm_height,
						head->subpixel,
						head->make,
						head->model,
						output->transform);

			ver = wl_resource_get_version(resource);
			if (ver >= WL_OUTPUT_DONE_SINCE_VERSION)
				wl_output_send_done(resource);
		}
		wl_resource_for_each(resource, &head->xdg_output_resource_list) {
			zxdg_output_v1_send_logical_position(resource,
							     output->x,
							     output->y);
			zxdg_output_v1_send_logical_size(resource,
							 output->width,
							 output->height);
			zxdg_output_v1_send_done(resource);
		}
	}

	/* we must ensure that pointers are inside output, otherwise they disappear */
	mid_x = output->x + output->width / 2;
	mid_y = output->y + output->height / 2;

	ev.mask = WESTON_POINTER_MOTION_ABS;
	ev.abs.c = weston_coord(mid_x, mid_y);
	wl_list_for_each(seat, &output->compositor->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);

		if (pointer && pixman_region32_contains_point(&old_region,
							      pointer->pos.c.x,
							      pointer->pos.c.y,
							      NULL))
			weston_pointer_move(pointer, &ev);
	}
}

/** Set output's color profile
 *
 * \param output The output to change.
 * \param cprof The color profile to set. Can be NULL for default sRGB profile.
 * \return True on success, or false on failure.
 *
 * Calling this function changes the color profile of the output. This causes
 * all existing weston_color_transform objects related to this output via
 * paint nodes to be unreferenced and later re-created on demand.
 *
 * This function may not be called from within weston_output_repaint().
 *
 * On failure, nothing is changed.
 *
 * \ingroup output
 */
WL_EXPORT bool
weston_output_set_color_profile(struct weston_output *output,
				struct weston_color_profile *cprof)
{
	struct weston_color_profile *old;
	struct weston_paint_node *pnode;

	old = output->color_profile;
	output->color_profile = weston_color_profile_ref(cprof);

	if (output->enabled) {
		if (!weston_output_set_color_outcome(output)) {
			/* Failed, roll back */
			weston_color_profile_unref(output->color_profile);
			output->color_profile = old;
			return false;
		}

		/* Remove outdated cached color transformations */
		wl_list_for_each(pnode, &output->paint_node_list, output_link) {
			weston_surface_color_transform_fini(&pnode->surf_xform);
			pnode->surf_xform_valid = false;
		}
	}

	weston_color_profile_unref(old);

	return true;
}

/** Set EOTF mode on an output
 *
 * \param output The output to modify, must be in disabled state.
 * \param eotf_mode The EOTF mode to set.
 *
 * Setting the output EOTF mode is used for turning HDR on/off. There are
 * multiple modes for HDR on, see enum weston_eotf_mode. This is the high level
 * choice on how to drive a video sink (monitor), either in the traditional
 * SDR mode or in one of the HDR modes.
 *
 * After attaching heads to an output, you can find out the possibly supported
 * EOTF modes with weston_output_get_supported_eotf_modes().
 *
 * This function does not check whether the given eotf_mode is actually
 * supported on the output. Enabling an output with an unsupported EOTF mode
 * has undefined visual results.
 *
 * The initial EOTF mode is SDR.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_set_eotf_mode(struct weston_output *output,
			    enum weston_eotf_mode eotf_mode)
{
	assert(!output->enabled);

	output->eotf_mode = eotf_mode;
}

/** Get EOTF mode of an output
 *
 * \param output The output to query.
 * \return The EOTF mode.
 *
 * \sa weston_output_set_eotf_mode
 * \ingroup output
 */
WL_EXPORT enum weston_eotf_mode
weston_output_get_eotf_mode(const struct weston_output *output)
{
	return output->eotf_mode;
}

/** Get HDR static metadata type 1
 *
 * \param output The output to query.
 * \return Pointer to the metadata stored in weston_output.
 *
 * This function is meant to be used by libweston backends.
 *
 * \ingroup output
 * \internal
 */
WL_EXPORT const struct weston_hdr_metadata_type1 *
weston_output_get_hdr_metadata_type1(const struct weston_output *output)
{
	assert(output->color_outcome);
	return &output->color_outcome->hdr_meta;
}

/** Set display or monitor basic color characteristics
 *
 * \param output The output to modify, must be in disabled state.
 * \param cc The new characteristics to set, or NULL to unset everything.
 *
 * This sets the metadata that describes the color characteristics of the
 * output in a very simple manner. If a non-NULL color profile is set for the
 * output, that will always take precedence.
 *
 * The initial value has everything unset.
 *
 * This function is meant to be used by compositor frontends.
 *
 * \ingroup output
 * \sa weston_output_set_color_profile
 */
WL_EXPORT void
weston_output_set_color_characteristics(struct weston_output *output,
					const struct weston_color_characteristics *cc)
{
	assert(!output->enabled);

	if (cc)
		output->color_characteristics = *cc;
	else
		output->color_characteristics.group_mask = 0;
}

/** Get display or monitor basic color characteristics
 *
 * \param output The output to query.
 * \return Pointer to the metadata stored in weston_output.
 *
 * This function is meant to be used by color manager modules.
 *
 * \ingroup output
 * \sa weston_output_set_color_characteristics
 */
WL_EXPORT const struct weston_color_characteristics *
weston_output_get_color_characteristics(struct weston_output *output)
{
	return &output->color_characteristics;
}

/** Initializes a weston_output object with enough data so
 ** an output can be configured.
 *
 * \param output     The weston_output object to initialize
 * \param compositor The compositor instance.
 * \param name       Name for the output (the string is copied).
 *
 * Sets initial values for fields that are expected to be
 * configured either by compositors or backends.
 *
 * The name is used in logs, and can be used by compositors as a configuration
 * identifier.
 *
 * \ingroup output
 * \internal
 */
WL_EXPORT void
weston_output_init(struct weston_output *output,
		   struct weston_compositor *compositor,
		   const char *name)
{
	output->compositor = compositor;
	output->destroying = 0;
	output->name = strdup(name);
	wl_list_init(&output->link);
	wl_signal_init(&output->user_destroy_signal);
	output->enabled = false;
	output->eotf_mode = WESTON_EOTF_MODE_SDR;
	output->desired_protection = WESTON_HDCP_DISABLE;
	output->allow_protection = true;
	output->power_state = WESTON_OUTPUT_POWER_NORMAL;

	wl_list_init(&output->head_list);

	/* Add some (in)sane defaults which can be used
	 * for checking if an output was properly configured
	 */
	output->scale = 0;
	/* Can't use -1 on uint32_t and 0 is valid enum value */
	output->transform = UINT32_MAX;

	pixman_region32_init(&output->region);
	wl_list_init(&output->mode_list);
}

/** Adds weston_output object to pending output list.
 *
 * \param output     The weston_output object to add
 * \param compositor The compositor instance.
 *
 * The opposite of this operation is built into weston_output_release().
 *
 * \ingroup compositor
 * \internal
 */
WL_EXPORT void
weston_compositor_add_pending_output(struct weston_output *output,
				     struct weston_compositor *compositor)
{
	assert(output->disable);
	assert(output->enable);

	wl_list_remove(&output->link);
	wl_list_insert(compositor->pending_output_list.prev, &output->link);
}

/** Create a string with the attached heads' names.
 *
 * The string must be free()'d.
 *
 * \ingroup output
 */
static char *
weston_output_create_heads_string(struct weston_output *output)
{
	FILE *fp;
	char *str = NULL;
	size_t size = 0;
	struct weston_head *head;
	const char *sep = "";

	fp = open_memstream(&str, &size);
	if (!fp)
		return NULL;

	wl_list_for_each(head, &output->head_list, output_link) {
		fprintf(fp, "%s%s", sep, head->name);
		sep = ", ";
	}
	fclose(fp);

	return str;
}

static bool
weston_outputs_overlap(struct weston_output *a, struct weston_output *b)
{
	bool overlap;
	pixman_region32_t intersection;

	pixman_region32_init(&intersection);
	pixman_region32_intersect(&intersection, &a->region, &b->region);
	overlap = pixman_region32_not_empty(&intersection);
	pixman_region32_fini(&intersection);

	return overlap;
}

/* This only works if the output region is current!
 *
 * That means we shouldn't expect it to return usable results unless
 * the output is at least undergoing enabling.
 */
static bool
weston_output_placement_ok(struct weston_output *output)
{
	struct weston_compositor *c = output->compositor;
	struct weston_output *iter;

	wl_list_for_each(iter, &c->output_list, link) {
		if (!iter->enabled)
			continue;

		if (weston_outputs_overlap(iter, output)) {
			weston_log("Error: output '%s' overlaps enabled output '%s'.\n",
				   output->name, iter->name);
			return false;
		}
	}

	return true;
}

/** Constructs a weston_output object that can be used by the compositor.
 *
 * \param output The weston_output object that needs to be enabled. Must not
 * be enabled already. Must have at least one head attached.
 *
 * Output coordinates are calculated and each new output is by default
 * assigned to the right of previous one.
 *
 * Sets up the transformation, and geometry of the output using the
 * properties that need to be configured by the compositor.
 *
 * Establishes a repaint timer for the output with the relevant display
 * object's event loop. See output_repaint_timer_handler().
 *
 * The output is assigned an ID. Weston can support up to 32 distinct
 * outputs, with IDs numbered from 0-31; the compositor's output_id_pool
 * is referred to and used to find the first available ID number, and
 * then this ID is marked as used in output_id_pool.
 *
 * The output is also assigned a Wayland global with the wl_output
 * external interface.
 *
 * Backend specific function is called to set up the output output.
 *
 * Output is added to the compositor's output list
 *
 * If the backend specific function fails, the weston_output object
 * is returned to a state it was before calling this function and
 * is added to the compositor's pending_output_list in case it needs
 * to be reconfigured or just so it can be destroyed at shutdown.
 *
 * 0 is returned on success, -1 on failure.
 *
 * \ingroup output
 */
WL_EXPORT int
weston_output_enable(struct weston_output *output)
{
	struct weston_head *head;
	char *head_names;

	if (output->enabled) {
		weston_log("Error: attempt to enable an enabled output '%s'\n",
			   output->name);
		return -1;
	}

	if (wl_list_empty(&output->head_list)) {
		weston_log("Error: cannot enable output '%s' without heads.\n",
			   output->name);
		return -1;
	}

	if (wl_list_empty(&output->mode_list) || !output->current_mode) {
		weston_log("Error: no video mode for output '%s'.\n",
			   output->name);
		return -1;
	}

	wl_list_for_each(head, &output->head_list, output_link) {
		assert(head->make);
		assert(head->model);
	}

	/* Make sure the scale is set up */
	assert(output->scale);

	/* Make sure we have a transform set */
	assert(output->transform != UINT32_MAX);

	output->original_scale = output->scale;

	wl_signal_init(&output->frame_signal);
	wl_signal_init(&output->destroy_signal);

	weston_output_transform_scale_init(output, output->transform, output->scale);

	weston_output_init_geometry(output, output->x, output->y);

	/* At this point we have a valid region so we can check placement. */
	if (!weston_output_placement_ok(output))
		return -1;

	wl_list_init(&output->animation_list);
	wl_list_init(&output->feedback_list);
	wl_list_init(&output->paint_node_list);
	wl_list_init(&output->paint_node_z_order_list);

	weston_output_update_matrix(output);

	weston_log("Output '%s' attempts EOTF mode: %s\n", output->name,
		   weston_eotf_mode_to_str(output->eotf_mode));

	if (!weston_output_set_color_outcome(output))
		return -1;

	output->capture_info = weston_output_capture_info_create();
	assert(output->capture_info);

	/* Enable the output (set up the crtc or create a
	 * window representing the output, set up the
	 * renderer, etc)
	 */
	if (output->enable(output) < 0) {
		weston_log("Enabling output \"%s\" failed.\n", output->name);
		weston_output_color_outcome_destroy(&output->color_outcome);
		weston_output_capture_info_destroy(&output->capture_info);
		return -1;
	}

	weston_compositor_add_output(output->compositor, output);
	weston_output_damage(output);

	head_names = weston_output_create_heads_string(output);
	weston_log("Output '%s' enabled with head(s) %s\n",
		   output->name, head_names);
	free(head_names);

	return 0;
}

/** Converts a weston_output object to a pending output state, so it
 ** can be configured again or destroyed.
 *
 * \param output The weston_output object that needs to be disabled.
 *
 * Calls a backend specific function to disable an output, in case
 * such function exists.
 *
 * The backend specific disable function may choose to postpone the disabling
 * by returning a negative value, in which case this function returns early.
 * In that case the backend will guarantee the output will be disabled soon
 * by the backend calling this function again. One must not attempt to re-enable
 * the output until that happens.
 *
 * Otherwise, if the output is being used by the compositor, it is removed
 * from weston's output_list (see weston_compositor_remove_output())
 * and is returned to a state it was before weston_output_enable()
 * was ran (see weston_output_enable_undo()).
 *
 * See weston_output_init() for more information on the
 * state output is returned to.
 *
 * If the output has never been enabled yet, this function can still be
 * called to ensure that the output is actually turned off rather than left
 * in the state it was discovered in.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_disable(struct weston_output *output)
{
	/* Should we rename this? */
	output->destroying = 1;

	/* Disable is called unconditionally also for not-enabled outputs,
	 * because at compositor start-up, if there is an output that is
	 * already on but the compositor wants to turn it off, we have to
	 * forward the turn-off to the backend so it knows to do it.
	 * The backend cannot initially turn off everything, because it
	 * would cause unnecessary mode-sets for all outputs the compositor
	 * wants to be on.
	 */
	if (output->disable(output) < 0)
		return;

	if (output->enabled) {
		weston_compositor_remove_output(output);

		assert(wl_list_empty(&output->paint_node_list));
	}

	output->destroying = 0;
}

/** Forces a synchronous call to heads_changed hook
 *
 * \param compositor The compositor instance
 *
 * If there are new or changed heads, calls the heads_changed hook and
 * returns after the hook returns.
 *
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_flush_heads_changed(struct weston_compositor *compositor)
{
	if (compositor->heads_changed_source) {
		wl_event_source_remove(compositor->heads_changed_source);
		weston_compositor_call_heads_changed(compositor);
	}
}

/** Add destroy callback for an output
 *
 * \param output The output to watch.
 * \param listener The listener to add. The \c notify member must be set.
 *
 * The listener callback will be called when user destroys an output. This
 * may be delayed by a backend in some cases. The main purpose of the
 * listener is to allow hooking up custom data to the output. The custom data
 * can be fetched via weston_output_get_destroy_listener() followed by
 * container_of().
 *
 * The \c data argument to the notify callback is the weston_output being
 * destroyed.
 *
 * @note This is for the final destruction of an output, not when it gets
 * disabled. If you want to keep track of enabled outputs, this is not it.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_add_destroy_listener(struct weston_output *output,
				   struct wl_listener *listener)
{
	wl_signal_add(&output->user_destroy_signal, listener);
}

/** Look up destroy listener for an output
 *
 * \param output The output to query.
 * \param notify The notify function used used for the added destroy listener.
 * \return The listener, or NULL if not found.
 *
 * This looks up the previously added destroy listener struct based on the
 * notify function it has. The listener can be used to access user data
 * through \c container_of().
 *
 * \sa wl_signal_get() weston_output_add_destroy_listener()
 * \ingroup output
 */
WL_EXPORT struct wl_listener *
weston_output_get_destroy_listener(struct weston_output *output,
				   wl_notify_func_t notify)
{
	return wl_signal_get(&output->user_destroy_signal, notify);
}

/** Uninitialize an output
 *
 * Removes the output from the list of enabled outputs if necessary, but
 * does not call the backend's output disable function. The output will no
 * longer be in the list of pending outputs either.
 *
 * All fields of weston_output become uninitialized, i.e. should not be used
 * anymore. The caller can free the memory after this.
 *
 * \ingroup output
 * \internal
 */
WL_EXPORT void
weston_output_release(struct weston_output *output)
{
	struct weston_head *head, *tmp;

	output->destroying = 1;

	weston_signal_emit_mutable(&output->user_destroy_signal, output);

	if (output->enabled)
		weston_compositor_remove_output(output);

	weston_color_profile_unref(output->color_profile);
	assert(output->color_outcome == NULL);

	pixman_region32_fini(&output->region);
	wl_list_remove(&output->link);

	wl_list_for_each_safe(head, tmp, &output->head_list, output_link)
		weston_head_detach(head);

	free(output->name);
}

/** Find an output by its given name
 *
 * \param compositor The compositor to search in.
 * \param name The output name to search for.
 * \return An existing output with the given name, or NULL if not found.
 *
 * \ingroup compositor
 */
WL_EXPORT struct weston_output *
weston_compositor_find_output_by_name(struct weston_compositor *compositor,
				      const char *name)
{
	struct weston_output *output;

	wl_list_for_each(output, &compositor->output_list, link)
		if (strcmp(output->name, name) == 0)
			return output;

	wl_list_for_each(output, &compositor->pending_output_list, link)
		if (strcmp(output->name, name) == 0)
			return output;

	return NULL;
}

/** Create a named output for an unused head
 *
 * \param compositor The compositor.
 * \param head The head to attach to the output.
 * \param name The name for the output.
 * \return A new \c weston_output, or NULL on failure.
 *
 * This creates a new weston_output that starts with the given head attached.
 * The head must not be already attached to another output.
 *
 * An output must be configured and it must have at least one head before
 * it can be enabled.
 *
 * \ingroup compositor
 */
WL_EXPORT struct weston_output *
weston_compositor_create_output(struct weston_compositor *compositor,
				struct weston_head *head,
				const char *name)
{
	struct weston_output *output;

	assert(head->backend->create_output);

	if (weston_compositor_find_output_by_name(compositor, name)) {
		weston_log("Warning: attempted to create an output with a "
			   "duplicate name '%s'.\n", name);
		return NULL;
	}

	output = head->backend->create_output(head->backend, name);
	if (!output)
		return NULL;

	if (head && weston_output_attach_head(output, head) < 0) {
		weston_output_destroy(output);
		return NULL;
	}

	return output;
}

/** Destroy an output
 *
 * \param output The output to destroy.
 *
 * The heads attached to the given output are detached and become unused again.
 *
 * It is not necessary to explicitly destroy all outputs at compositor exit.
 * weston_compositor_destroy() will automatically destroy any remaining
 * outputs.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_destroy(struct weston_output *output)
{
	output->destroy(output);
}

/** When you need a head...
 *
 * This function is a hack, used until all code has been converted to become
 * multi-head aware.
 *
 * \param output The weston_output whose head to get.
 * \return The first head in the output's list.
 *
 * \ingroup output
 */
WL_EXPORT struct weston_head *
weston_output_get_first_head(struct weston_output *output)
{
	if (wl_list_empty(&output->head_list))
		return NULL;

	return container_of(output->head_list.next,
			    struct weston_head, output_link);
}

/** Allow/Disallow content-protection support for an output
 *
 * This function sets the allow_protection member for an output. Setting of
 * this field will allow the compositor to attempt content-protection for this
 * output, for a backend that supports the content-protection protocol.
 *
 * \param output The weston_output for whom the content-protection is to be
 * allowed.
 * \param allow_protection The bool value which is to be set.
 */
WL_EXPORT void
weston_output_allow_protection(struct weston_output *output,
			       bool allow_protection)
{
	output->allow_protection = allow_protection;
}

/** Get supported EOTF modes as a bit mask
 *
 * \param output The output to query.
 * \return A bit mask with values from enum weston_eotf_mode or'ed together.
 *
 * Returns the bit mask of the EOTF modes that all the currently attached
 * heads claim to support. Adding or removing heads may change the result.
 * An output can be queried regrdless of whether it is enabled or disabled.
 *
 * If no heads are attached, no EOTF modes are deemed supported.
 *
 * \ingroup output
 */
WL_EXPORT uint32_t
weston_output_get_supported_eotf_modes(struct weston_output *output)
{
	uint32_t eotf_modes = WESTON_EOTF_MODE_ALL_MASK;
	struct weston_head *head;

	if (wl_list_empty(&output->head_list))
		return WESTON_EOTF_MODE_NONE;

	wl_list_for_each(head, &output->head_list, output_link)
		eotf_modes = eotf_modes & head->supported_eotf_mask;

	return eotf_modes;
}

/* Set the forced-power state of output
 *
 * \param output The output to set power state.
 * \param state The power state to set for output.
 *
 * Set the forced-power state of output, then update DPMS mode for output
 * when compositor is active.
 *
 * \ingroup output
 */
static void
weston_output_force_power(struct weston_output *output,
			  enum weston_output_power_state power)
{
	enum dpms_enum dpms;

	output->power_state = power;

	if (output->compositor->state == WESTON_COMPOSITOR_SLEEPING ||
	    output->compositor->state == WESTON_COMPOSITOR_OFFSCREEN)
		return;

	if (!output->set_dpms || !output->enabled)
		return;

	dpms = (power == WESTON_OUTPUT_POWER_NORMAL) ? WESTON_DPMS_ON : WESTON_DPMS_OFF;
	output->set_dpms(output, dpms);
}

/* Set the power state of output to normal mode
 *
 * \param output The output to set on.
 *
 * This function will make the forced-off power of the output to normal state.
 * In case when compositor is sleeping or offscreen, the power state will be
 * applied once the compositor wakes up.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_power_on(struct weston_output *output)
{
	weston_output_force_power(output, WESTON_OUTPUT_POWER_NORMAL);
}

/* Force the power state of output to off mode
 *
 * \param output The output to set off.
 *
 * This function ceases rendering on a given output and will power it off
 * via DPMS when compositor is active. Otherwise the output is forced off
 * when the compositor wakes up.
 *
 * \ingroup output
 */
WL_EXPORT void
weston_output_power_off(struct weston_output *output)
{
	weston_output_force_power(output, WESTON_OUTPUT_POWER_FORCED_OFF);
}

static void
xdg_output_unlist(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
xdg_output_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct zxdg_output_v1_interface xdg_output_interface = {
	xdg_output_destroy
};

static void
xdg_output_manager_destroy(struct wl_client *client,
                           struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
xdg_output_manager_get_xdg_output(struct wl_client *client,
				  struct wl_resource *manager,
				  uint32_t id,
				  struct wl_resource *output_resource)
{
	int version = wl_resource_get_version(manager);
	struct weston_head *head = wl_resource_get_user_data(output_resource);
	struct weston_output *output = head->output;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &zxdg_output_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&head->xdg_output_resource_list,
		       wl_resource_get_link(resource));

	wl_resource_set_implementation(resource, &xdg_output_interface,
				       NULL, xdg_output_unlist);

	zxdg_output_v1_send_logical_position(resource, output->x, output->y);
	zxdg_output_v1_send_logical_size(resource,
					 output->width,
					 output->height);
	if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION)
		zxdg_output_v1_send_name(resource, head->name);

	zxdg_output_v1_send_done(resource);
}

static const struct zxdg_output_manager_v1_interface xdg_output_manager_interface = {
	xdg_output_manager_destroy,
	xdg_output_manager_get_xdg_output
};

static void
bind_xdg_output_manager(struct wl_client *client,
			void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &zxdg_output_manager_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &xdg_output_manager_interface,
				       NULL, NULL);
}

static void
destroy_viewport(struct wl_resource *resource)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(resource);

	if (!surface)
		return;

	surface->viewport_resource = NULL;
	surface->pending.buffer_viewport.buffer.src_width =
		wl_fixed_from_int(-1);
	surface->pending.buffer_viewport.surface.width = -1;
	surface->pending.buffer_viewport.changed = 1;
}

static void
viewport_destroy(struct wl_client *client,
		 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
viewport_set_source(struct wl_client *client,
		    struct wl_resource *resource,
		    wl_fixed_t src_x,
		    wl_fixed_t src_y,
		    wl_fixed_t src_width,
		    wl_fixed_t src_height)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(resource);

	if (!surface) {
		wl_resource_post_error(resource,
			WP_VIEWPORT_ERROR_NO_SURFACE,
			"wl_surface for this viewport is no longer exists");
		return;
	}

	assert(surface->viewport_resource == resource);
	assert(surface->resource);

	if (src_width == wl_fixed_from_int(-1) &&
	    src_height == wl_fixed_from_int(-1) &&
	    src_x == wl_fixed_from_int(-1) &&
	    src_y == wl_fixed_from_int(-1)) {
		/* unset source rect */
		surface->pending.buffer_viewport.buffer.src_width =
			wl_fixed_from_int(-1);
		surface->pending.buffer_viewport.changed = 1;
		return;
	}

	if (src_width <= 0 || src_height <= 0 || src_x < 0 || src_y < 0) {
		wl_resource_post_error(resource,
			WP_VIEWPORT_ERROR_BAD_VALUE,
			"wl_surface@%d viewport source "
			"w=%f <= 0, h=%f <= 0, x=%f < 0, or y=%f < 0",
			wl_resource_get_id(surface->resource),
			wl_fixed_to_double(src_width),
			wl_fixed_to_double(src_height),
			wl_fixed_to_double(src_x),
			wl_fixed_to_double(src_y));
		return;
	}

	surface->pending.buffer_viewport.buffer.src_x = src_x;
	surface->pending.buffer_viewport.buffer.src_y = src_y;
	surface->pending.buffer_viewport.buffer.src_width = src_width;
	surface->pending.buffer_viewport.buffer.src_height = src_height;
	surface->pending.buffer_viewport.changed = 1;
}

static void
viewport_set_destination(struct wl_client *client,
			 struct wl_resource *resource,
			 int32_t dst_width,
			 int32_t dst_height)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(resource);

	if (!surface) {
		wl_resource_post_error(resource,
			WP_VIEWPORT_ERROR_NO_SURFACE,
			"wl_surface for this viewport no longer exists");
		return;
	}

	assert(surface->viewport_resource == resource);

	if (dst_width == -1 && dst_height == -1) {
		/* unset destination size */
		surface->pending.buffer_viewport.surface.width = -1;
		surface->pending.buffer_viewport.changed = 1;
		return;
	}

	if (dst_width <= 0 || dst_height <= 0) {
		wl_resource_post_error(resource,
			WP_VIEWPORT_ERROR_BAD_VALUE,
			"destination size must be positive (%dx%d)",
			dst_width, dst_height);
		return;
	}

	surface->pending.buffer_viewport.surface.width = dst_width;
	surface->pending.buffer_viewport.surface.height = dst_height;
	surface->pending.buffer_viewport.changed = 1;
}

static const struct wp_viewport_interface viewport_interface = {
	viewport_destroy,
	viewport_set_source,
	viewport_set_destination
};

static void
viewporter_destroy(struct wl_client *client,
		   struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
viewporter_get_viewport(struct wl_client *client,
			struct wl_resource *viewporter,
			uint32_t id,
			struct wl_resource *surface_resource)
{
	int version = wl_resource_get_version(viewporter);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct wl_resource *resource;

	if (surface->viewport_resource) {
		wl_resource_post_error(viewporter,
			WP_VIEWPORTER_ERROR_VIEWPORT_EXISTS,
			"a viewport for that surface already exists");
		return;
	}

	resource = wl_resource_create(client, &wp_viewport_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &viewport_interface,
				       surface, destroy_viewport);

	surface->viewport_resource = resource;
}

static const struct wp_viewporter_interface viewporter_interface = {
	viewporter_destroy,
	viewporter_get_viewport
};

static void
bind_viewporter(struct wl_client *client,
		void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wp_viewporter_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &viewporter_interface,
				       NULL, NULL);
}

static void
destroy_presentation_feedback(struct wl_resource *feedback_resource)
{
	struct weston_presentation_feedback *feedback;

	feedback = wl_resource_get_user_data(feedback_resource);

	wl_list_remove(&feedback->link);
	free(feedback);
}

static void
presentation_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
presentation_feedback(struct wl_client *client,
		      struct wl_resource *presentation_resource,
		      struct wl_resource *surface_resource,
		      uint32_t callback)
{
	struct weston_surface *surface;
	struct weston_presentation_feedback *feedback;

	surface = wl_resource_get_user_data(surface_resource);

	feedback = zalloc(sizeof *feedback);
	if (feedback == NULL)
		goto err_calloc;

	feedback->resource = wl_resource_create(client,
					&wp_presentation_feedback_interface,
					1, callback);
	if (!feedback->resource)
		goto err_create;

	wl_resource_set_implementation(feedback->resource, NULL, feedback,
				       destroy_presentation_feedback);

	wl_list_insert(&surface->pending.feedback_list, &feedback->link);

	return;

err_create:
	free(feedback);

err_calloc:
	wl_client_post_no_memory(client);
}

static const struct wp_presentation_interface presentation_implementation = {
	presentation_destroy,
	presentation_feedback
};

static void
bind_presentation(struct wl_client *client,
		  void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wp_presentation_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &presentation_implementation,
				       compositor, NULL);
	wp_presentation_send_clock_id(resource, compositor->presentation_clock);
}

static void
compositor_bind(struct wl_client *client,
		void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_compositor_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &compositor_interface,
				       compositor, NULL);
}

static void
set_presentation_hint(struct wl_client *client, struct wl_resource *resource, uint32_t hint)
{
	struct weston_tearing_control *tc = wl_resource_get_user_data(resource);
	struct weston_surface *surf = tc->surface;

	if (hint == WP_TEARING_CONTROL_V1_PRESENTATION_HINT_ASYNC)
		surf->tear_control->may_tear = true;
	else
		surf->tear_control->may_tear = false;
}

static void
destroy_tearing_control(struct wl_client *client, struct wl_resource *res)
{
	struct weston_tearing_control *tc = wl_resource_get_user_data(res);
	struct weston_surface *surf = tc->surface;

	if (surf)
		surf->tear_control = NULL;

	wl_resource_destroy(res);
}

static const struct wp_tearing_control_v1_interface tearing_interface = {
	set_presentation_hint,
	destroy_tearing_control,
};

static void
destroy_tearing_controller(struct wl_client *client,
			   struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
free_tearing_control(struct wl_resource *res)
{
	struct weston_tearing_control *tc = wl_resource_get_user_data(res);
	struct weston_surface *surf = tc->surface;

	if (surf)
		surf->tear_control = NULL;

	free(tc);
}

static void
get_tearing_control(struct wl_client *client,
		    struct wl_resource *resource,
		    uint32_t id,
		    struct wl_resource *surface_resource)
{
	struct wl_resource *ctl_res;
	struct weston_tearing_control *control;
	struct weston_surface *surface;
	uint32_t version;

	surface = wl_resource_get_user_data(surface_resource);
	if (surface->tear_control) {
		wl_resource_post_error(resource,
				       WP_TEARING_CONTROL_MANAGER_V1_ERROR_TEARING_CONTROL_EXISTS,
				       "Surface already has a tearing controller");
		return;
	}

	version = wl_resource_get_version(resource);
	ctl_res = wl_resource_create(client,
				     &wp_tearing_control_v1_interface,
				     version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	control = xzalloc(sizeof *control);
	control->may_tear = false;
	control->surface = surface;
	surface->tear_control = control;
	wl_resource_set_implementation(ctl_res, &tearing_interface,
				       control, free_tearing_control);
}

static const struct wp_tearing_control_manager_v1_interface
tearing_control_manager_implementation = {
	destroy_tearing_controller,
	get_tearing_control,
};

static void
bind_tearing_controller(struct wl_client *client, void *data,
			uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &wp_tearing_control_manager_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &tearing_control_manager_implementation,
				       compositor, NULL);
}

static const char *
output_repaint_status_text(struct weston_output *output)
{
	switch (output->repaint_status) {
	case REPAINT_NOT_SCHEDULED:
		return "no repaint";
	case REPAINT_BEGIN_FROM_IDLE:
		return "start_repaint_loop scheduled";
	case REPAINT_SCHEDULED:
		return "repaint scheduled";
	case REPAINT_AWAITING_COMPLETION:
		return "awaiting completion";
	}

	assert(!"output_repaint_status_text missing enum");
	return NULL;
}

static void
debug_scene_view_print_buffer(FILE *fp, struct weston_view *view)
{
	struct weston_buffer *buffer = view->surface->buffer_ref.buffer;
	char *modifier_name;

	if (!buffer) {
		fprintf(fp, "\t\t[buffer not available]\n");
		return;
	}

	switch (buffer->type) {
	case WESTON_BUFFER_SHM:
		fprintf(fp, "\t\tSHM buffer\n");
		break;
	case WESTON_BUFFER_DMABUF:
		fprintf(fp, "\t\tdmabuf buffer\n");
		break;
	case WESTON_BUFFER_SOLID:
		fprintf(fp, "\t\tsolid-colour buffer\n");
		fprintf(fp, "\t\t\t[R %f, G %f, B %f, A %f]\n",
			buffer->solid.r, buffer->solid.g, buffer->solid.b,
			buffer->solid.a);
		break;
	case WESTON_BUFFER_RENDERER_OPAQUE:
		fprintf(fp, "\t\tEGL buffer:\n");
		fprintf(fp, "\t\t\t[format may be inaccurate]\n");
		break;
	}

	if (buffer->busy_count > 0) {
		fprintf(fp, "\t\t\t[%d references may use buffer content]\n",
			buffer->busy_count);
	} else {
		fprintf(fp, "\t\t\t[buffer has been released to client]\n");
	}

	if (buffer->pixel_format) {
		fprintf(fp, "\t\t\tformat: 0x%lx %s\n",
			(unsigned long) buffer->pixel_format->format,
			buffer->pixel_format->drm_format_name);
	} else {
		fprintf(fp, "\t\t\t[unknown format]\n");
	}

	modifier_name = pixel_format_get_modifier(buffer->format_modifier);
	fprintf(fp, "\t\t\tmodifier: %s\n",
		modifier_name ?
			modifier_name : "Failed to convert to a modifier name");
	free(modifier_name);

	fprintf(fp, "\t\t\twidth: %d, height: %d\n",
		buffer->width, buffer->height);
	if (buffer->buffer_origin == ORIGIN_BOTTOM_LEFT)
		fprintf(fp, "\t\t\tbottom-left origin\n");

	if (buffer->direct_display)
		fprintf(fp, "\t\t\tdirect-display buffer (no renderer access)\n");
}

static void
debug_scene_view_print(FILE *fp, struct weston_view *view, int view_idx)
{
	struct weston_compositor *ec = view->surface->compositor;
	struct weston_output *output;
	char desc[512];
	pixman_box32_t *box;
	uint32_t surface_id = 0;
	pid_t pid = 0;

	if (view->surface->resource) {
		struct wl_resource *resource = view->surface->resource;
		wl_client_get_credentials(wl_resource_get_client(resource),
					  &pid, NULL, NULL);
		surface_id = wl_resource_get_id(view->surface->resource);
	}

	if (!view->surface->get_label ||
	    view->surface->get_label(view->surface, desc, sizeof(desc)) < 0) {
		strcpy(desc, "[no description available]");
	}
	fprintf(fp, "\tView %d (role %s, PID %d, surface ID %u, %s, %p):\n",
		view_idx, view->surface->role_name, pid, surface_id,
		desc, view);

	if (!weston_view_is_mapped(view))
		fprintf(fp, "\t[view is not mapped!]\n");
	if (!weston_surface_is_mapped(view->surface))
		fprintf(fp, "\t[surface is not mapped!]\n");
	if (wl_list_empty(&view->layer_link.link)) {
		if (!get_view_layer(view))
			fprintf(fp, "\t[view is not part of any layer]\n");
		else
			fprintf(fp, "\t[view is under parent view layer]\n");
	}

	box = pixman_region32_extents(&view->transform.boundingbox);
	fprintf(fp, "\t\tposition: (%d, %d) -> (%d, %d)\n",
		box->x1, box->y1, box->x2, box->y2);
	box = pixman_region32_extents(&view->transform.opaque);

	if (weston_view_is_opaque(view, &view->transform.boundingbox)) {
		fprintf(fp, "\t\t[fully opaque]\n");
	} else if (!pixman_region32_not_empty(&view->transform.opaque)) {
		fprintf(fp, "\t\t[not opaque]\n");
	} else {
		fprintf(fp, "\t\t[opaque: (%d, %d) -> (%d, %d)]\n",
			box->x1, box->y1, box->x2, box->y2);
	}

	if (view->alpha < 1.0)
		fprintf(fp, "\t\talpha: %f\n", view->alpha);

	if (view->output_mask != 0) {
		bool first_output = true;
		fprintf(fp, "\t\toutputs: ");
		wl_list_for_each(output, &ec->output_list, link) {
			if (!(view->output_mask & (1 << output->id)))
				continue;
			fprintf(fp, "%s%d (%s)%s",
				(first_output) ? "" : ", ",
				output->id, output->name,
				(view->output == output) ? " (primary)" : "");
			first_output = false;
		}
	} else {
		fprintf(fp, "\t\t[no outputs]");
	}

	fprintf(fp, "\n");

	debug_scene_view_print_buffer(fp, view);
}

static void
debug_scene_view_print_tree(struct weston_view *view,
			    FILE *fp, int *view_idx)
{
	struct weston_subsurface *sub;
	struct weston_view *ev;

	/*
	 * print the view first, then we recursively go on printing
	 * sub-surfaces. We bail out once no more sub-surfaces are available.
	 */
	debug_scene_view_print(fp, view, *view_idx);

	/* no more sub-surfaces */
	if (wl_list_empty(&view->surface->subsurface_list))
		return;

	wl_list_for_each(sub, &view->surface->subsurface_list, parent_link) {
		wl_list_for_each(ev, &sub->surface->views, surface_link) {
			/* only print the child views of the current view */
			if (ev->parent_view != view)
				continue;

			(*view_idx)++;
			debug_scene_view_print_tree(ev, fp, view_idx);
		}
	}
}

/**
 * Output information on how libweston is currently composing the scene
 * graph.
 *
 * \ingroup compositor
 */
WL_EXPORT char *
weston_compositor_print_scene_graph(struct weston_compositor *ec)
{
	struct weston_output *output;
	struct weston_layer *layer;
	struct timespec now;
	int layer_idx = 0;
	FILE *fp;
	char *ret;
	size_t len;
	int err;

	fp = open_memstream(&ret, &len);
	assert(fp);

	weston_compositor_read_presentation_clock(ec, &now);
	fprintf(fp, "Weston scene graph at %ld.%09ld:\n\n",
		now.tv_sec, now.tv_nsec);

	wl_list_for_each(output, &ec->output_list, link) {
		struct weston_head *head;
		int head_idx = 0;

		fprintf(fp, "Output %d (%s):\n", output->id, output->name);
		assert(output->enabled);

		fprintf(fp, "\tposition: (%d, %d) -> (%d, %d)\n",
			output->x, output->y,
			output->x + output->width,
			output->y + output->height);
		fprintf(fp, "\tmode: %dx%d@%.3fHz\n",
			output->current_mode->width,
			output->current_mode->height,
			output->current_mode->refresh / 1000.0);
		fprintf(fp, "\tscale: %d\n", output->scale);

		fprintf(fp, "\trepaint status: %s\n",
			output_repaint_status_text(output));
		if (output->repaint_status == REPAINT_SCHEDULED)
			fprintf(fp, "\tnext repaint: %ld.%09ld\n",
				output->next_repaint.tv_sec,
				output->next_repaint.tv_nsec);

		wl_list_for_each(head, &output->head_list, output_link) {
			fprintf(fp, "\tHead %d (%s): %sconnected\n",
				head_idx++, head->name,
				(head->connected) ? "" : "not ");
		}
	}

	fprintf(fp, "\n");

	wl_list_for_each(layer, &ec->layer_list, link) {
		struct weston_view *view;
		int view_idx = 0;

		fprintf(fp, "Layer %d (pos 0x%lx):\n", layer_idx++,
			(unsigned long) layer->position);

		if (!weston_layer_mask_is_infinite(layer)) {
			fprintf(fp, "\t[mask: (%d, %d) -> (%d,%d)]\n\n",
				layer->mask.x1, layer->mask.y1,
				layer->mask.x2, layer->mask.y2);
		}

		wl_list_for_each(view, &layer->view_list.link, layer_link.link) {
			debug_scene_view_print_tree(view, fp, &view_idx);
			view_idx++;
		}

		if (wl_list_empty(&layer->view_list.link))
			fprintf(fp, "\t[no views]\n");

		fprintf(fp, "\n");
	}

	err = fclose(fp);
	assert(err == 0);

	return ret;
}

/**
 * Called when the 'scene-graph' debug scope is bound by a client. This
 * one-shot weston-debug scope prints the current scene graph when bound,
 * and then terminates the stream.
 */
static void
debug_scene_graph_cb(struct weston_log_subscription *sub, void *data)
{
	struct weston_compositor *ec = data;
	char *str = weston_compositor_print_scene_graph(ec);

	weston_log_subscription_printf(sub, "%s", str);
	free(str);
	weston_log_subscription_complete(sub);
}

/** Retrieve testsuite data from compositor
 *
 * The testsuite data can be defined by the test suite of projects that uses
 * libweston and given to the compositor at the moment of its creation. This
 * function should be used when we need to retrieve the testsuite private data
 * from the compositor.
 *
 * \param ec The weston compositor.
 * \return The testsuite data.
 *
 * \ingroup compositor
 * \sa weston_compositor_test_data_init
 */
WL_EXPORT void *
weston_compositor_get_test_data(struct weston_compositor *ec)
{
	return ec->test_data.test_private_data;
}

/** Create the compositor.
 *
 * This functions creates and initializes a compositor instance.
 *
 * \param display The Wayland display to be used.
 * \param user_data A pointer to an object that can later be retrieved
 * \param log_ctx A pointer to weston_debug_compositor
 * \param test_data Optional testsuite data, or NULL.
 * using the \ref weston_compositor_get_user_data function.
 * \return The compositor instance on success or NULL on failure.
 *
 * \ingroup compositor
 */
WL_EXPORT struct weston_compositor *
weston_compositor_create(struct wl_display *display,
			 struct weston_log_context *log_ctx, void *user_data,
			 const struct weston_testsuite_data *test_data)
{
	struct weston_compositor *ec;
	struct wl_event_loop *loop;

	if (!log_ctx)
		return NULL;

	ec = zalloc(sizeof *ec);
	if (!ec)
		return NULL;

	if (test_data)
		ec->test_data = *test_data;

	ec->weston_log_ctx = log_ctx;
	ec->wl_display = display;
	ec->user_data = user_data;
	wl_signal_init(&ec->destroy_signal);
	wl_signal_init(&ec->create_surface_signal);
	wl_signal_init(&ec->activate_signal);
	wl_signal_init(&ec->transform_signal);
	wl_signal_init(&ec->kill_signal);
	wl_signal_init(&ec->idle_signal);
	wl_signal_init(&ec->wake_signal);
	wl_signal_init(&ec->show_input_panel_signal);
	wl_signal_init(&ec->hide_input_panel_signal);
	wl_signal_init(&ec->update_input_panel_signal);
	wl_signal_init(&ec->seat_created_signal);
	wl_signal_init(&ec->output_created_signal);
	wl_signal_init(&ec->output_destroyed_signal);
	wl_signal_init(&ec->output_moved_signal);
	wl_signal_init(&ec->output_resized_signal);
	wl_signal_init(&ec->heads_changed_signal);
	wl_signal_init(&ec->output_heads_changed_signal);
	wl_signal_init(&ec->session_signal);
	wl_signal_init(&ec->output_capture.ask_auth);
	ec->session_active = true;

	ec->output_id_pool = 0;
	ec->repaint_msec = DEFAULT_REPAINT_WINDOW;

	ec->activate_serial = 1;

	ec->touch_mode = WESTON_TOUCH_MODE_NORMAL;

	ec->content_protection = NULL;

	if (!wl_global_create(ec->wl_display, &wl_compositor_interface, 5,
			      ec, compositor_bind))
		goto fail;

	if (!wl_global_create(ec->wl_display, &wl_subcompositor_interface, 1,
			      ec, bind_subcompositor))
		goto fail;

	if (!wl_global_create(ec->wl_display, &wp_viewporter_interface, 1,
			      ec, bind_viewporter))
		goto fail;

	if (!wl_global_create(ec->wl_display, &zxdg_output_manager_v1_interface, 2,
			      ec, bind_xdg_output_manager))
		goto fail;

	if (!wl_global_create(ec->wl_display, &wp_presentation_interface, 1,
			      ec, bind_presentation))
		goto fail;

	if (!wl_global_create(ec->wl_display,
			      &wp_single_pixel_buffer_manager_v1_interface, 1,
			      NULL, bind_single_pixel_buffer))
		goto fail;

	if (!wl_global_create(ec->wl_display,
			      &wp_tearing_control_manager_v1_interface, 1,
			      ec, bind_tearing_controller))
		goto fail;

	if (weston_input_init(ec) != 0)
		goto fail;

	weston_compositor_install_capture_protocol(ec);

	wl_list_init(&ec->view_list);
	wl_list_init(&ec->plane_list);
	wl_list_init(&ec->layer_list);
	wl_list_init(&ec->seat_list);
	wl_list_init(&ec->pending_output_list);
	wl_list_init(&ec->output_list);
	wl_list_init(&ec->head_list);
	wl_list_init(&ec->key_binding_list);
	wl_list_init(&ec->modifier_binding_list);
	wl_list_init(&ec->button_binding_list);
	wl_list_init(&ec->touch_binding_list);
	wl_list_init(&ec->tablet_tool_binding_list);
	wl_list_init(&ec->axis_binding_list);
	wl_list_init(&ec->debug_binding_list);
	wl_list_init(&ec->tablet_manager_resource_list);

	wl_list_init(&ec->plugin_api_list);

	weston_plane_init(&ec->primary_plane, ec);
	weston_compositor_stack_plane(ec, &ec->primary_plane, NULL);

	wl_data_device_manager_init(ec->wl_display);

	wl_display_init_shm(ec->wl_display);

	loop = wl_display_get_event_loop(ec->wl_display);
	ec->idle_source = wl_event_loop_add_timer(loop, idle_handler, ec);
	ec->repaint_timer =
		wl_event_loop_add_timer(loop, output_repaint_timer_handler,
					ec);

	weston_layer_init(&ec->fade_layer, ec);
	weston_layer_init(&ec->cursor_layer, ec);

	weston_layer_set_position(&ec->fade_layer, WESTON_LAYER_POSITION_FADE);
	weston_layer_set_position(&ec->cursor_layer,
				  WESTON_LAYER_POSITION_CURSOR);

	ec->debug_scene =
		weston_compositor_add_log_scope(ec, "scene-graph",
						"Scene graph details\n",
						debug_scene_graph_cb, NULL,
						ec);

	ec->timeline =
		weston_compositor_add_log_scope(ec, "timeline",
						"Timeline event points\n",
						weston_timeline_create_subscription,
						weston_timeline_destroy_subscription,
						ec);
	ec->libseat_debug =
		weston_compositor_add_log_scope(ec, "libseat-debug",
						"libseat debug messages\n",
						NULL, NULL, NULL);
	return ec;

fail:
	free(ec);
	return NULL;
}

/** weston_compositor_shutdown
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_shutdown(struct weston_compositor *ec)
{
	struct weston_output *output, *next;

	wl_event_source_remove(ec->idle_source);
	wl_event_source_remove(ec->repaint_timer);

	if (ec->touch_calibration)
		weston_compositor_destroy_touch_calibrator(ec);

	/* Destroy all outputs associated with this compositor */
	wl_list_for_each_safe(output, next, &ec->output_list, link)
		output->destroy(output);

	/* Destroy all pending outputs associated with this compositor */
	wl_list_for_each_safe(output, next, &ec->pending_output_list, link)
		output->destroy(output);

	/* Color manager objects may have renderer hooks */
	if (ec->color_manager) {
		ec->color_manager->destroy(ec->color_manager);
		ec->color_manager = NULL;
	}

	if (ec->renderer)
		ec->renderer->destroy(ec);

	weston_binding_list_destroy_all(&ec->key_binding_list);
	weston_binding_list_destroy_all(&ec->modifier_binding_list);
	weston_binding_list_destroy_all(&ec->button_binding_list);
	weston_binding_list_destroy_all(&ec->touch_binding_list);
	weston_binding_list_destroy_all(&ec->axis_binding_list);
	weston_binding_list_destroy_all(&ec->debug_binding_list);
	weston_binding_list_destroy_all(&ec->tablet_tool_binding_list);

	weston_plane_release(&ec->primary_plane);

	weston_layer_fini(&ec->fade_layer);
	weston_layer_fini(&ec->cursor_layer);

	if (!wl_list_empty(&ec->layer_list))
		weston_log("BUG: layer_list is not empty after shutdown. Calls to weston_layer_fini() are missing somwhere.\n");
}

/** weston_compositor_exit_with_code
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_exit_with_code(struct weston_compositor *compositor,
				 int exit_code)
{
	if (compositor->exit_code == EXIT_SUCCESS)
		compositor->exit_code = exit_code;

	weston_compositor_exit(compositor);
}

/** weston_compositor_set_default_pointer_grab
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_set_default_pointer_grab(struct weston_compositor *ec,
			const struct weston_pointer_grab_interface *interface)
{
	struct weston_seat *seat;

	ec->default_pointer_grab = interface;
	wl_list_for_each(seat, &ec->seat_list, link) {
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);

		if (pointer)
			weston_pointer_set_default_grab(pointer, interface);
	}
}

/** weston_compositor_set_presentation_clock
 * \ingroup compositor
 */
WL_EXPORT int
weston_compositor_set_presentation_clock(struct weston_compositor *compositor,
					 clockid_t clk_id)
{
	struct timespec ts;

	if (clock_gettime(clk_id, &ts) < 0)
		return -1;

	compositor->presentation_clock = clk_id;

	return 0;
}

/** For choosing the software clock, when the display hardware or API
 * does not expose a compatible presentation timestamp.
 *
 * \ingroup compositor
 */
WL_EXPORT int
weston_compositor_set_presentation_clock_software(
					struct weston_compositor *compositor)
{
	/* In order of preference */
	static const clockid_t clocks[] = {
		CLOCK_MONOTONIC_RAW,	/* no jumps, no crawling */
		CLOCK_MONOTONIC_COARSE,	/* no jumps, may crawl, fast & coarse */
		CLOCK_MONOTONIC,	/* no jumps, may crawl */
	};
	unsigned i;

	for (i = 0; i < ARRAY_LENGTH(clocks); i++)
		if (weston_compositor_set_presentation_clock(compositor,
							     clocks[i]) == 0)
			return 0;

	weston_log("Error: no suitable presentation clock available.\n");

	return -1;
}

/** Read the current time from the Presentation clock
 *
 * \param compositor
 * \param[out] ts The current time.
 *
 * \note Reading the current time in user space is always imprecise to some
 * degree.
 *
 * This function is never meant to fail. If reading the clock does fail,
 * an error message is logged and a zero time is returned. Callers are not
 * supposed to detect or react to failures.
 *
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_read_presentation_clock(
			struct weston_compositor *compositor,
			struct timespec *ts)
{
	int ret;

	ret = clock_gettime(compositor->presentation_clock, ts);
	if (ret < 0) {
		ts->tv_sec = 0;
		ts->tv_nsec = 0;

		weston_log_paced(&compositor->presentation_clock_failure_pacer,
				 1, 0,
				 "Error: failure to read "
				 "the presentation clock %#x: '%s' (%d)\n",
				 compositor->presentation_clock,
				 strerror(errno), errno);
	}
}

/** Import dmabuf buffer into current renderer
 *
 * \param compositor
 * \param buffer the dmabuf buffer to import
 * \return true on usable buffers, false otherwise
 *
 * This function tests that the linux_dmabuf_buffer is usable
 * for the current renderer. Returns false on unusable buffers. Usually
 * usability is tested by importing the dmabufs for composition.
 *
 * This hook is also used for detecting if the renderer supports
 * dmabufs at all. If the renderer hook is NULL, dmabufs are not
 * supported.
 *
 * \ingroup compositor
 */
WL_EXPORT bool
weston_compositor_import_dmabuf(struct weston_compositor *compositor,
				struct linux_dmabuf_buffer *buffer)
{
	struct weston_renderer *renderer;

	renderer = compositor->renderer;

	if (renderer->import_dmabuf == NULL)
		return false;

	return renderer->import_dmabuf(compositor, buffer);
}

WL_EXPORT bool
weston_compositor_dmabuf_can_scanout(struct weston_compositor *compositor,
		struct linux_dmabuf_buffer *buffer)
{
	struct weston_backend *backend = compositor->backend;

	if (backend->can_scanout_dmabuf == NULL)
		return false;

	return backend->can_scanout_dmabuf(backend, buffer);
}

WL_EXPORT void
weston_version(int *major, int *minor, int *micro)
{
	*major = WESTON_VERSION_MAJOR;
	*minor = WESTON_VERSION_MINOR;
	*micro = WESTON_VERSION_MICRO;
}

/**
 * Attempts to find a module path from the module map specified in the
 * environment. If found, writes the full path into the path variable.
 *
 * The module map is a string in environment variable WESTON_MODULE_MAP, where
 * each entry is of the form "name=path" and entries are separated by
 * semicolons. Whitespace is significant.
 *
 * \param name The name to search for.
 * \param path Where the path is written to if found.
 * \param path_len Allocated bytes at \c path .
 * \returns The length of the string written to path on success, or 0 if the
 * module was not specified in the environment map or path_len was too small.
 */
WL_EXPORT size_t
weston_module_path_from_env(const char *name, char *path, size_t path_len)
{
	const char *mapping = getenv("WESTON_MODULE_MAP");
	const char *end;
	const int name_len = strlen(name);

	if (!mapping)
		return 0;

	end = mapping + strlen(mapping);
	while (mapping < end && *mapping) {
		const char *filename, *next;

		/* early out: impossibly short string */
		if (end - mapping < name_len + 1)
			return 0;

		filename = &mapping[name_len + 1];
		next = strchrnul(mapping, ';');

		if (strncmp(mapping, name, name_len) == 0 &&
		    mapping[name_len] == '=') {
			size_t file_len = next - filename; /* no trailing NUL */
			if (file_len >= path_len)
				return 0;
			strncpy(path, filename, file_len);
			path[file_len] = '\0';
			return file_len;
		}

		mapping = next + 1;
	}

	return 0;
}

/** A wrapper function to open and return the entry point of a shared library
 * module
 *
 * This function loads the module and provides the caller with the entry point
 * address which can be later used to execute shared library code. It can be
 * used to load-up libweston modules but also other modules, specific to the
 * compositor (i.e., weston).
 *
 * \param name the name of the shared library
 * \param entrypoint the entry point of the shared library
 * \param module_dir the path where to look for the shared library module
 * \return the address of the module specified the entry point, or NULL otherwise
 *
 */
WL_EXPORT void *
weston_load_module(const char *name, const char *entrypoint,
		   const char *module_dir)
{
	char path[PATH_MAX];
	void *module, *init;
	size_t len;

	if (name == NULL)
		return NULL;

	if (name[0] != '/') {
		len = weston_module_path_from_env(name, path, sizeof path);
		if (len == 0)
			len = snprintf(path, sizeof path, "%s/%s",
				       module_dir, name);
	} else {
		len = snprintf(path, sizeof path, "%s", name);
	}

	/* snprintf returns the length of the string it would've written,
	 * _excluding_ the NUL byte. So even being equal to the size of
	 * our buffer is an error here. */
	if (len >= sizeof path)
		return NULL;

	module = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
	if (module) {
		weston_log("Module '%s' already loaded\n", path);
	} else {
		weston_log("Loading module '%s'\n", path);
		module = dlopen(path, RTLD_NOW);
		if (!module) {
			weston_log("Failed to load module: %s\n", dlerror());
			return NULL;
		}
	}

	init = dlsym(module, entrypoint);
	if (!init) {
		weston_log("Failed to lookup init function: %s\n", dlerror());
		dlclose(module);
		return NULL;
	}

	return init;
}

/** Add a compositor destroy listener only once
 *
 * \param compositor The compositor whose destroy to watch for.
 * \param listener The listener struct to initialize.
 * \param destroy_handler The callback when compositor is destroyed.
 * \return True if listener is added, or false if there already is a listener
 * with the given \c destroy_handler.
 *
 * This function does nothing and returns false if the given callback function
 * is already present in the weston_compositor destroy callbacks list.
 * Otherwise, this function initializes the given listener with the given
 * callback pointer and adds it to the compositor's destroy callbacks list.
 *
 * This can be used to ensure that plugin initialization is done only once
 * in case the same plugin is loaded multiple times. If this function returns
 * false, the plugin should be already initialized successfully.
 *
 * All plugins should register a destroy listener for cleaning up. Note, that
 * the plugin destruction order is not guaranteed: plugins that depend on other
 * plugins must be able to be torn down in arbitrary order.
 *
 * \sa weston_compositor_destroy
 */
WL_EXPORT bool
weston_compositor_add_destroy_listener_once(struct weston_compositor *compositor,
					    struct wl_listener *listener,
					    wl_notify_func_t destroy_handler)
{
	if (wl_signal_get(&compositor->destroy_signal, destroy_handler))
		return false;

	listener->notify = destroy_handler;
	wl_signal_add(&compositor->destroy_signal, listener);
	return true;
}

/** Destroys the compositor.
 *
 * This function cleans up the compositor state and then destroys it.
 *
 * @param compositor The compositor to be destroyed.
 *
 * @ingroup compositor
 */
WL_EXPORT void
weston_compositor_destroy(struct weston_compositor *compositor)
{
	/* prevent further rendering while shutting down */
	compositor->state = WESTON_COMPOSITOR_OFFSCREEN;

	weston_signal_emit_mutable(&compositor->destroy_signal, compositor);

	weston_compositor_xkb_destroy(compositor);

	if (compositor->backend)
		compositor->backend->destroy(compositor->backend);

	/* The backend is responsible for destroying the heads. */
	assert(wl_list_empty(&compositor->head_list));

	weston_plugin_api_destroy_list(compositor);

	if (compositor->heads_changed_source)
		wl_event_source_remove(compositor->heads_changed_source);

	weston_log_scope_destroy(compositor->debug_scene);
	compositor->debug_scene = NULL;

	weston_log_scope_destroy(compositor->timeline);
	compositor->timeline = NULL;

	weston_log_scope_destroy(compositor->libseat_debug);
	compositor->libseat_debug = NULL;

	if (compositor->default_dmabuf_feedback) {
		weston_dmabuf_feedback_destroy(compositor->default_dmabuf_feedback);
		weston_dmabuf_feedback_format_table_destroy(compositor->dmabuf_feedback_format_table);
	}

	free(compositor);
}

/** Instruct the compositor to exit.
 *
 * This functions does not directly destroy the compositor object, it merely
 * command it to start the tear down process. It is not guaranteed that the
 * tear down will happen immediately.
 *
 * \param compositor The compositor to tear down.
 *
 * \ingroup compositor
 */
WL_EXPORT void
weston_compositor_exit(struct weston_compositor *compositor)
{
	compositor->exit(compositor);
}

/** Return the user data stored in the compositor.
 *
 * This function returns the user data pointer set with user_data parameter
 * to the \ref weston_compositor_create function.
 *
 * \ingroup compositor
 */
WL_EXPORT void *
weston_compositor_get_user_data(struct weston_compositor *compositor)
{
	return compositor->user_data;
}

static const char * const backend_map[] = {
	[WESTON_BACKEND_DRM] =		"drm-backend.so",
	[WESTON_BACKEND_HEADLESS] =	"headless-backend.so",
	[WESTON_BACKEND_PIPEWIRE] =	"pipewire-backend.so",
	[WESTON_BACKEND_RDP] =		"rdp-backend.so",
	[WESTON_BACKEND_VNC] =		"vnc-backend.so",
	[WESTON_BACKEND_WAYLAND] =	"wayland-backend.so",
	[WESTON_BACKEND_X11] =		"x11-backend.so",
};

/** Load a backend into a weston_compositor
 *
 * A backend must be loaded to make a weston_compositor work. A backend
 * provides input and output capabilities, and determines the renderer to use.
 *
 * \param compositor A compositor that has not had a backend loaded yet.
 * \param backend Name of the backend file.
 * \param config_base A pointer to a backend-specific configuration
 * structure's 'base' member.
 *
 * \return 0 on success, or -1 on error.
 *
 * \ingroup compositor
 */
WL_EXPORT int
weston_compositor_load_backend(struct weston_compositor *compositor,
			       enum weston_compositor_backend backend,
			       struct weston_backend_config *config_base)
{
	int (*backend_init)(struct weston_compositor *c,
			    struct weston_backend_config *config_base);

	if (compositor->backend) {
		weston_log("Error: attempt to load a backend when one is already loaded\n");
		return -1;
	}

	if (backend >= ARRAY_LENGTH(backend_map))
		return -1;

	backend_init = weston_load_module(backend_map[backend],
					  "weston_backend_init",
					  LIBWESTON_MODULEDIR);
	if (!backend_init)
		return -1;

	if (backend_init(compositor, config_base) < 0) {
		compositor->backend = NULL;
		return -1;
	}

	if (!compositor->color_manager) {
		compositor->color_manager =
			weston_color_manager_noop_create(compositor);
	}

	if (!compositor->color_manager)
		return -1;

	if (!compositor->color_manager->init(compositor->color_manager))
		return -1;

	weston_log("Color manager: %s\n", compositor->color_manager->name);

	return 0;
}

WL_EXPORT int
weston_compositor_init_renderer(struct weston_compositor *compositor,
				enum weston_renderer_type renderer_type,
				const struct weston_renderer_options *options)
{
	const struct gl_renderer_interface *gl_renderer;
	const struct gl_renderer_display_options *gl_options;
	int ret;

	switch (renderer_type) {
	case WESTON_RENDERER_GL:
		gl_renderer = weston_load_module("gl-renderer.so",
						 "gl_renderer_interface",
						 LIBWESTON_MODULEDIR);
		if (!gl_renderer)
			return -1;

		gl_options = container_of(options,
					  struct gl_renderer_display_options,
					  base);
		ret = gl_renderer->display_create(compositor, gl_options);
		if (ret < 0)
			return ret;

		compositor->renderer->gl = gl_renderer;
		weston_log("Using GL renderer\n");
		break;
	case WESTON_RENDERER_PIXMAN:
		ret = pixman_renderer_init(compositor);
		if (ret < 0)
			return ret;
		weston_log("Using Pixman renderer\n");
		break;
	default:
		ret = -1;
	}

	return ret;
}

/** weston_compositor_load_xwayland
 * \ingroup compositor
 */
WL_EXPORT int
weston_compositor_load_xwayland(struct weston_compositor *compositor)
{
	int (*module_init)(struct weston_compositor *ec);

	module_init = weston_load_module("xwayland.so",
					 "weston_module_init",
					 LIBWESTON_MODULEDIR);
	if (!module_init)
		return -1;
	if (module_init(compositor) < 0)
		return -1;
	return 0;
}

/** Load Little CMS color manager plugin
 *
 * Calling this function before loading any backend sets Little CMS
 * as the active color matching module (CMM) instead of the default no-op
 * color manager.
 *
 * \ingroup compositor
 */
WL_EXPORT int
weston_compositor_load_color_manager(struct weston_compositor *compositor)
{
	struct weston_color_manager *
	(*cm_create)(struct weston_compositor *compositor);

	if (compositor->color_manager) {
		weston_log("Error: Color manager '%s' is loaded, cannot load another.\n",
			   compositor->color_manager->name);
		return -1;
	}

	cm_create = weston_load_module("color-lcms.so",
				       "weston_color_manager_create",
				       LIBWESTON_MODULEDIR);
	if (!cm_create) {
		weston_log("Error: Could not load color-lcms.so.\n");
		return -1;
	}

	compositor->color_manager = cm_create(compositor);
	if (!compositor->color_manager) {
		weston_log("Error: loading color-lcms.so failed.\n");
		return -1;
	}

	return 0;
}

/** Resolve an internal compositor error by disconnecting the client.
 *
 * This function is used in cases when the wl_buffer turns out
 * unusable and there is no fallback path.
 *
 * It is possible the fault is caused by a compositor bug, the underlying
 * graphics stack bug or normal behaviour, or perhaps a client mistake.
 * In any case, the options are to either composite garbage or nothing,
 * or disconnect the client. This is a helper function for the latter.
 *
 * The error is sent as an INVALID_OBJECT error on the client's wl_display.
 *
 * \param buffer The weston buffer that is unusable.
 * \param msg A custom error message attached to the protocol error.
 */
WL_EXPORT void
weston_buffer_send_server_error(struct weston_buffer *buffer,
				      const char *msg)
{
	struct wl_client *client;
	struct wl_resource *display_resource;
	uint32_t id;

	assert(buffer->resource);
	id = wl_resource_get_id(buffer->resource);
	client = wl_resource_get_client(buffer->resource);
	display_resource = wl_client_get_object(client, 1);

	assert(display_resource);
	wl_resource_post_error(display_resource,
			       WL_DISPLAY_ERROR_INVALID_OBJECT,
			       "server error with "
			       "wl_buffer@%u: %s", id, msg);
}

WL_EXPORT void
weston_output_disable_planes_incr(struct weston_output *output)
{
	output->disable_planes++;
	/*
	 * If disable_planes changes from 0 to non-zero, it means some type of
	 * recording of content has started, and therefore protection level of
	 * the protected surfaces must be updated to avoid the recording of
	 * the protected content.
	 */
	if (output->disable_planes == 1)
		weston_schedule_surface_protection_update(output->compositor);
}

WL_EXPORT void
weston_output_disable_planes_decr(struct weston_output *output)
{
	output->disable_planes--;
	/*
	 * If disable_planes changes from non-zero to 0, it means no content
	 * recording is going on any more, and the protected and surfaces can be
	 * shown without any apprehensions about content being recorded.
	 */
	if (output->disable_planes == 0)
		weston_schedule_surface_protection_update(output->compositor);

}

WL_EXPORT struct weston_renderbuffer *
weston_renderbuffer_ref(struct weston_renderbuffer *renderbuffer)
{
	renderbuffer->refcount++;

	return renderbuffer;
}

WL_EXPORT void
weston_renderbuffer_unref(struct weston_renderbuffer *renderbuffer)
{
	assert(renderbuffer->refcount > 0);

	if (--renderbuffer->refcount > 0)
		return;

	renderbuffer->destroy(renderbuffer);
}

/** Tell the renderer that the target framebuffer size has changed
 *
 * \param output The output that was resized.
 * \param fb_size The framebuffer size, including output decorations.
 * \param area The composited area inside the framebuffer, excluding
 * decorations. This can also be NULL, which means the whole fb_size is
 * the composited area.
 */
WL_EXPORT void
weston_renderer_resize_output(struct weston_output *output,
			      const struct weston_size *fb_size,
			      const struct weston_geometry *area)
{
	struct weston_renderer *r = output->compositor->renderer;
	struct weston_geometry def = {
		.x = 0,
		.y = 0,
		.width = fb_size->width,
		.height = fb_size->height
	};

	if (!r->resize_output(output, fb_size, area ?: &def)) {
		weston_log("Error: Resizing output '%s' failed.\n",
			   output->name);
	}
}
