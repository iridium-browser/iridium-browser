/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2017, 2018 Collabora, Ltd.
 * Copyright © 2017, 2018 General Electric Company
 * Copyright (c) 2018 DisplayLink (UK) Ltd.
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

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "drm-internal.h"

/**
 * Allocate a new, empty, plane state.
 */
struct drm_plane_state *
drm_plane_state_alloc(struct drm_output_state *state_output,
		      struct drm_plane *plane)
{
	struct drm_plane_state *state = zalloc(sizeof(*state));

	assert(state);
	state->output_state = state_output;
	state->plane = plane;
	state->in_fence_fd = -1;
	state->zpos = DRM_PLANE_ZPOS_INVALID_PLANE;

	/* Here we only add the plane state to the desired link, and not
	 * set the member. Having an output pointer set means that the
	 * plane will be displayed on the output; this won't be the case
	 * when we go to disable a plane. In this case, it must be part of
	 * the commit (and thus the output state), but the member must be
	 * NULL, as it will not be on any output when the state takes
	 * effect.
	 */
	if (state_output)
		wl_list_insert(&state_output->plane_list, &state->link);
	else
		wl_list_init(&state->link);

	return state;
}

/**
 * Free an existing plane state. As a special case, the state will not
 * normally be freed if it is the current state; see drm_plane_set_state.
 */
void
drm_plane_state_free(struct drm_plane_state *state, bool force)
{
	if (!state)
		return;

	wl_list_remove(&state->link);
	wl_list_init(&state->link);
	state->output_state = NULL;
	state->in_fence_fd = -1;
	state->zpos = DRM_PLANE_ZPOS_INVALID_PLANE;

	/* Once the damage blob has been submitted, it is refcounted internally
	 * by the kernel, which means we can safely discard it.
	 */
	if (state->damage_blob_id != 0) {
		drmModeDestroyPropertyBlob(state->plane->backend->drm.fd,
					   state->damage_blob_id);
		state->damage_blob_id = 0;
	}

	if (force || state != state->plane->state_cur) {
		drm_fb_unref(state->fb);
		free(state);
	}
}

/**
 * Duplicate an existing plane state into a new plane state, storing it within
 * the given output state. If the output state already contains a plane state
 * for the drm_plane referenced by 'src', that plane state is freed first.
 */
struct drm_plane_state *
drm_plane_state_duplicate(struct drm_output_state *state_output,
			  struct drm_plane_state *src)
{
	struct drm_plane_state *dst = zalloc(sizeof(*dst));
	struct drm_plane_state *old, *tmp;

	assert(src);
	assert(dst);
	*dst = *src;
	/* We don't want to copy this, because damage is transient, and only
	 * lasts for the duration of a single repaint.
	 */
	dst->damage_blob_id = 0;
	wl_list_init(&dst->link);

	wl_list_for_each_safe(old, tmp, &state_output->plane_list, link) {
		/* Duplicating a plane state into the same output state, so
		 * it can replace itself with an identical copy of itself,
		 * makes no sense. */
		assert(old != src);
		if (old->plane == dst->plane)
			drm_plane_state_free(old, false);
	}

	wl_list_insert(&state_output->plane_list, &dst->link);
	if (src->fb)
		dst->fb = drm_fb_ref(src->fb);
	dst->output_state = state_output;
	dst->complete = false;

	return dst;
}

/**
 * Remove a plane state from an output state; if the plane was previously
 * enabled, then replace it with a disabling state. This ensures that the
 * output state was untouched from it was before the plane state was
 * modified by the caller of this function.
 *
 * This is required as drm_output_state_get_plane may either allocate a
 * new plane state, in which case this function will just perform a matching
 * drm_plane_state_free, or it may instead repurpose an existing disabling
 * state (if the plane was previously active), in which case this function
 * will reset it.
 */
void
drm_plane_state_put_back(struct drm_plane_state *state)
{
	struct drm_output_state *state_output;
	struct drm_plane *plane;

	if (!state)
		return;

	state_output = state->output_state;
	plane = state->plane;
	drm_plane_state_free(state, false);

	/* Plane was previously disabled; no need to keep this temporary
	 * state around. */
	if (!plane->state_cur->fb)
		return;

	(void) drm_plane_state_alloc(state_output, plane);
}

/**
 * Given a weston_view, fill the drm_plane_state's co-ordinates to display on
 * a given plane.
 */
bool
drm_plane_state_coords_for_view(struct drm_plane_state *state,
				struct weston_view *ev, uint64_t zpos)
{
	struct drm_output *output = state->output;
	struct weston_buffer *buffer = ev->surface->buffer_ref.buffer;
	pixman_region32_t dest_rect, src_rect;
	pixman_box32_t *box, tbox;
	float sxf1, syf1, sxf2, syf2;

	if (!drm_view_transform_supported(ev, &output->base))
		return false;

	/* Update the base weston_plane co-ordinates. */
	box = pixman_region32_extents(&ev->transform.boundingbox);
	state->plane->base.x = box->x1;
	state->plane->base.y = box->y1;

	/* First calculate the destination co-ordinates by taking the
	 * area of the view which is visible on this output, performing any
	 * transforms to account for output rotation and scale as necessary. */
	pixman_region32_init(&dest_rect);
	pixman_region32_intersect(&dest_rect, &ev->transform.boundingbox,
				  &output->base.region);
	pixman_region32_translate(&dest_rect, -output->base.x, -output->base.y);
	box = pixman_region32_extents(&dest_rect);
	tbox = weston_transformed_rect(output->base.width,
				       output->base.height,
				       output->base.transform,
				       output->base.current_scale,
				       *box);
	state->dest_x = tbox.x1;
	state->dest_y = tbox.y1;
	state->dest_w = tbox.x2 - tbox.x1;
	state->dest_h = tbox.y2 - tbox.y1;
	pixman_region32_fini(&dest_rect);

	/* Now calculate the source rectangle, by finding the extents of the
	 * view, and working backwards to source co-ordinates. */
	pixman_region32_init(&src_rect);
	pixman_region32_intersect(&src_rect, &ev->transform.boundingbox,
				  &output->base.region);
	box = pixman_region32_extents(&src_rect);
	weston_view_from_global_float(ev, box->x1, box->y1, &sxf1, &syf1);
	weston_surface_to_buffer_float(ev->surface, sxf1, syf1, &sxf1, &syf1);
	weston_view_from_global_float(ev, box->x2, box->y2, &sxf2, &syf2);
	weston_surface_to_buffer_float(ev->surface, sxf2, syf2, &sxf2, &syf2);
	pixman_region32_fini(&src_rect);

	/* Buffer transforms may mean that x2 is to the left of x1, and/or that
	 * y2 is above y1. */
	if (sxf2 < sxf1) {
		double tmp = sxf1;
		sxf1 = sxf2;
		sxf2 = tmp;
	}
	if (syf2 < syf1) {
		double tmp = syf1;
		syf1 = syf2;
		syf2 = tmp;
	}

	/* Shift from S23.8 wl_fixed to U16.16 KMS fixed-point encoding. */
	state->src_x = wl_fixed_from_double(sxf1) << 8;
	state->src_y = wl_fixed_from_double(syf1) << 8;
	state->src_w = wl_fixed_from_double(sxf2 - sxf1) << 8;
	state->src_h = wl_fixed_from_double(syf2 - syf1) << 8;

	/* Clamp our source co-ordinates to surface bounds; it's possible
	 * for intermediate translations to give us slightly incorrect
	 * co-ordinates if we have, for example, multiple zooming
	 * transformations. View bounding boxes are also explicitly rounded
	 * greedily. */
	if (state->src_x < 0)
		state->src_x = 0;
	if (state->src_y < 0)
		state->src_y = 0;
	if (state->src_w > (uint32_t) ((buffer->width << 16) - state->src_x))
		state->src_w = (buffer->width << 16) - state->src_x;
	if (state->src_h > (uint32_t) ((buffer->height << 16) - state->src_y))
		state->src_h = (buffer->height << 16) - state->src_y;

	/* apply zpos if available */
	state->zpos = zpos;

	return true;
}

/**
 * Reset the current state of a DRM plane
 *
 * The current state will be freed and replaced by a pristine state.
 *
 * @param plane The plane to reset the current state of
 */
void
drm_plane_reset_state(struct drm_plane *plane)
{
	drm_plane_state_free(plane->state_cur, true);
	plane->state_cur = drm_plane_state_alloc(NULL, plane);
	plane->state_cur->complete = true;
}

/**
 * Return a plane state from a drm_output_state.
 */
struct drm_plane_state *
drm_output_state_get_existing_plane(struct drm_output_state *state_output,
				    struct drm_plane *plane)
{
	struct drm_plane_state *ps;

	wl_list_for_each(ps, &state_output->plane_list, link) {
		if (ps->plane == plane)
			return ps;
	}

	return NULL;
}

/**
 * Return a plane state from a drm_output_state, either existing or
 * freshly allocated.
 */
struct drm_plane_state *
drm_output_state_get_plane(struct drm_output_state *state_output,
			   struct drm_plane *plane)
{
	struct drm_plane_state *ps;

	ps = drm_output_state_get_existing_plane(state_output, plane);
	if (ps)
		return ps;

	return drm_plane_state_alloc(state_output, plane);
}

/**
 * Allocate a new, empty drm_output_state. This should not generally be used
 * in the repaint cycle; see drm_output_state_duplicate.
 */
struct drm_output_state *
drm_output_state_alloc(struct drm_output *output,
		       struct drm_pending_state *pending_state)
{
	struct drm_output_state *state = zalloc(sizeof(*state));

	assert(state);
	state->output = output;
	state->dpms = WESTON_DPMS_OFF;
	state->protection = WESTON_HDCP_DISABLE;
	state->pending_state = pending_state;
	if (pending_state)
		wl_list_insert(&pending_state->output_list, &state->link);
	else
		wl_list_init(&state->link);

	wl_list_init(&state->plane_list);

	return state;
}

/**
 * Duplicate an existing drm_output_state into a new one. This is generally
 * used during the repaint cycle, to capture the existing state of an output
 * and modify it to create a new state to be used.
 *
 * The mode determines whether the output will be reset to an a blank state,
 * or an exact mirror of the current state.
 */
struct drm_output_state *
drm_output_state_duplicate(struct drm_output_state *src,
			   struct drm_pending_state *pending_state,
			   enum drm_output_state_duplicate_mode plane_mode)
{
	struct drm_output_state *dst = malloc(sizeof(*dst));
	struct drm_plane_state *ps;

	assert(dst);

	/* Copy the whole structure, then individually modify the
	 * pending_state, as well as the list link into our pending
	 * state. */
	*dst = *src;

	dst->pending_state = pending_state;
	if (pending_state)
		wl_list_insert(&pending_state->output_list, &dst->link);
	else
		wl_list_init(&dst->link);

	wl_list_init(&dst->plane_list);

	wl_list_for_each(ps, &src->plane_list, link) {
		/* Don't carry planes which are now disabled; these should be
		 * free for other outputs to reuse. */
		if (!ps->output)
			continue;

		if (plane_mode == DRM_OUTPUT_STATE_CLEAR_PLANES)
			(void) drm_plane_state_alloc(dst, ps->plane);
		else
			(void) drm_plane_state_duplicate(dst, ps);
	}

	return dst;
}

/**
 * Free an unused drm_output_state.
 */
void
drm_output_state_free(struct drm_output_state *state)
{
	struct drm_plane_state *ps, *next;

	if (!state)
		return;

	wl_list_for_each_safe(ps, next, &state->plane_list, link)
		drm_plane_state_free(ps, false);

	wl_list_remove(&state->link);

	free(state);
}

/**
 * Allocate a new drm_pending_state
 *
 * Allocate a new, empty, 'pending state' structure to be used across a
 * repaint cycle or similar.
 *
 * @param backend DRM backend
 * @returns Newly-allocated pending state structure
 */
struct drm_pending_state *
drm_pending_state_alloc(struct drm_backend *backend)
{
	struct drm_pending_state *ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	ret->backend = backend;
	wl_list_init(&ret->output_list);

	return ret;
}

/**
 * Free a drm_pending_state structure
 *
 * Frees a pending_state structure, as well as any output_states connected
 * to this pending state.
 *
 * @param pending_state Pending state structure to free
 */
void
drm_pending_state_free(struct drm_pending_state *pending_state)
{
	struct drm_output_state *output_state, *tmp;

	if (!pending_state)
		return;

	wl_list_for_each_safe(output_state, tmp, &pending_state->output_list,
			      link) {
		drm_output_state_free(output_state);
	}

	free(pending_state);
}

/**
 * Find an output state in a pending state
 *
 * Given a pending_state structure, find the output_state for a particular
 * output.
 *
 * @param pending_state Pending state structure to search
 * @param output Output to find state for
 * @returns Output state if present, or NULL if not
 */
struct drm_output_state *
drm_pending_state_get_output(struct drm_pending_state *pending_state,
			     struct drm_output *output)
{
	struct drm_output_state *output_state;

	wl_list_for_each(output_state, &pending_state->output_list, link) {
		if (output_state->output == output)
			return output_state;
	}

	return NULL;
}
