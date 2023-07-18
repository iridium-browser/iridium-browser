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

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/vt.h>
#include <assert.h>
#include <sys/mman.h>
#include <time.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include <libudev.h>

#include <libweston/libweston.h>
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>
#include "drm-internal.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include "shared/string-helpers.h"
#include "pixman-renderer.h"
#include "pixel-formats.h"
#include "libbacklight.h"
#include "libinput-seat.h"
#include "launcher-util.h"
#include "vaapi-recorder.h"
#include "presentation-time-server-protocol.h"
#include "linux-dmabuf.h"
#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include "linux-explicit-synchronization.h"

static const char default_seat[] = "seat0";

static void
drm_backend_create_faked_zpos(struct drm_backend *b)
{
	struct drm_plane *plane;
	uint64_t zpos = 0ULL;
	uint64_t zpos_min_primary;
	uint64_t zpos_min_overlay;
	uint64_t zpos_min_cursor;

	zpos_min_primary = zpos;
	wl_list_for_each(plane, &b->plane_list, link) {
		/* if the property is there, bail out sooner */
		if (plane->props[WDRM_PLANE_ZPOS].prop_id != 0)
			return;

		if (plane->type != WDRM_PLANE_TYPE_PRIMARY)
			continue;
		zpos++;
	}

	zpos_min_overlay = zpos;
	wl_list_for_each(plane, &b->plane_list, link) {
		if (plane->type != WDRM_PLANE_TYPE_OVERLAY)
			continue;
		zpos++;
	}

	zpos_min_cursor = zpos;
	wl_list_for_each(plane, &b->plane_list, link) {
		if (plane->type != WDRM_PLANE_TYPE_CURSOR)
			continue;
		zpos++;
	}

	drm_debug(b, "[drm-backend] zpos property not found. "
		     "Using invented immutable zpos values:\n");
	/* assume that invented zpos values are immutable */
	wl_list_for_each(plane, &b->plane_list, link) {
		if (plane->type == WDRM_PLANE_TYPE_PRIMARY) {
			plane->zpos_min = zpos_min_primary;
			plane->zpos_max = zpos_min_primary;
		} else if (plane->type == WDRM_PLANE_TYPE_OVERLAY) {
			plane->zpos_min = zpos_min_overlay;
			plane->zpos_max = zpos_min_overlay;
		} else if (plane->type == WDRM_PLANE_TYPE_CURSOR) {
			plane->zpos_min = zpos_min_cursor;
			plane->zpos_max = zpos_min_cursor;
		}
		drm_debug(b, "\t[plane] %s plane %d, zpos_min %"PRIu64", "
			      "zpos_max %"PRIu64"\n",
			      drm_output_get_plane_type_name(plane),
			      plane->plane_id, plane->zpos_min, plane->zpos_max);
	}
}

static void
wl_array_remove_uint32(struct wl_array *array, uint32_t elm)
{
	uint32_t *pos, *end;

	end = (uint32_t *) ((char *) array->data + array->size);

	wl_array_for_each(pos, array) {
		if (*pos != elm)
			continue;

		array->size -= sizeof(*pos);
		if (pos + 1 == end)
			break;

		memmove(pos, pos + 1, (char *) end -  (char *) (pos + 1));
		break;
	}
}

static int
pageflip_timeout(void *data) {
	/*
	 * Our timer just went off, that means we're not receiving drm
	 * page flip events anymore for that output. Let's gracefully exit
	 * weston with a return value so devs can debug what's going on.
	 */
	struct drm_output *output = data;
	struct weston_compositor *compositor = output->base.compositor;

	weston_log("Pageflip timeout reached on output %s, your "
	           "driver is probably buggy!  Exiting.\n",
		   output->base.name);
	weston_compositor_exit_with_code(compositor, EXIT_FAILURE);

	return 0;
}

/* Creates the pageflip timer. Note that it isn't armed by default */
static int
drm_output_pageflip_timer_create(struct drm_output *output)
{
	struct wl_event_loop *loop = NULL;
	struct weston_compositor *ec = output->base.compositor;

	loop = wl_display_get_event_loop(ec->wl_display);
	assert(loop);
	output->pageflip_timer = wl_event_loop_add_timer(loop,
	                                                 pageflip_timeout,
	                                                 output);

	if (output->pageflip_timer == NULL) {
		weston_log("creating drm pageflip timer failed: %s\n",
			   strerror(errno));
		return -1;
	}

	return 0;
}

static void
drm_output_destroy(struct weston_output *output_base);

/**
 * Returns true if the plane can be used on the given output for its current
 * repaint cycle.
 */
bool
drm_plane_is_available(struct drm_plane *plane, struct drm_output *output)
{
	assert(plane->state_cur);

	if (output->virtual)
		return false;

	/* The plane still has a request not yet completed by the kernel. */
	if (!plane->state_cur->complete)
		return false;

	/* The plane is still active on another output. */
	if (plane->state_cur->output && plane->state_cur->output != output)
		return false;

	/* Check whether the plane can be used with this CRTC; possible_crtcs
	 * is a bitmask of CRTC indices (pipe), rather than CRTC object ID. */
	return !!(plane->possible_crtcs & (1 << output->pipe));
}

struct drm_output *
drm_output_find_by_crtc(struct drm_backend *b, uint32_t crtc_id)
{
	struct drm_output *output;

	wl_list_for_each(output, &b->compositor->output_list, base.link) {
		if (output->crtc_id == crtc_id)
			return output;
	}

	return NULL;
}

struct drm_head *
drm_head_find_by_connector(struct drm_backend *backend, uint32_t connector_id)
{
	struct weston_head *base;
	struct drm_head *head;

	wl_list_for_each(base,
			 &backend->compositor->head_list, compositor_link) {
		head = to_drm_head(base);
		if (head->connector_id == connector_id)
			return head;
	}

	return NULL;
}

/**
 * Get output state to disable output
 *
 * Returns a pointer to an output_state object which can be used to disable
 * an output (e.g. DPMS off).
 *
 * @param pending_state The pending state object owning this update
 * @param output The output to disable
 * @returns A drm_output_state to disable the output
 */
static struct drm_output_state *
drm_output_get_disable_state(struct drm_pending_state *pending_state,
			     struct drm_output *output)
{
	struct drm_output_state *output_state;

	output_state = drm_output_state_duplicate(output->state_cur,
						  pending_state,
						  DRM_OUTPUT_STATE_CLEAR_PLANES);
	output_state->dpms = WESTON_DPMS_OFF;

	output_state->protection = WESTON_HDCP_DISABLE;

	return output_state;
}


/**
 * Mark a drm_output_state (the output's last state) as complete. This handles
 * any post-completion actions such as updating the repaint timer, disabling the
 * output, and finally freeing the state.
 */
void
drm_output_update_complete(struct drm_output *output, uint32_t flags,
			   unsigned int sec, unsigned int usec)
{
	struct drm_backend *b = to_drm_backend(output->base.compositor);
	struct drm_plane_state *ps;
	struct timespec ts;

	/* Stop the pageflip timer instead of rearming it here */
	if (output->pageflip_timer)
		wl_event_source_timer_update(output->pageflip_timer, 0);

	wl_list_for_each(ps, &output->state_cur->plane_list, link)
		ps->complete = true;

	drm_output_state_free(output->state_last);
	output->state_last = NULL;

	if (output->destroy_pending) {
		output->destroy_pending = false;
		output->disable_pending = false;
		output->dpms_off_pending = false;
		drm_output_destroy(&output->base);
		return;
	} else if (output->disable_pending) {
		output->disable_pending = false;
		output->dpms_off_pending = false;
		weston_output_disable(&output->base);
		return;
	} else if (output->dpms_off_pending) {
		struct drm_pending_state *pending = drm_pending_state_alloc(b);
		output->dpms_off_pending = false;
		drm_output_get_disable_state(pending, output);
		drm_pending_state_apply_sync(pending);
	}
	if (output->state_cur->dpms == WESTON_DPMS_OFF &&
	    output->base.repaint_status != REPAINT_AWAITING_COMPLETION) {
		/* DPMS can happen to us either in the middle of a repaint
		 * cycle (when we have painted fresh content, only to throw it
		 * away for DPMS off), or at any other random point. If the
		 * latter is true, then we cannot go through finish_frame,
		 * because the repaint machinery does not expect this. */
		return;
	}

	ts.tv_sec = sec;
	ts.tv_nsec = usec * 1000;
	weston_output_finish_frame(&output->base, &ts, flags);

	/* We can't call this from frame_notify, because the output's
	 * repaint needed flag is cleared just after that */
	if (output->recorder)
		weston_output_schedule_repaint(&output->base);
}

static struct drm_fb *
drm_output_render_pixman(struct drm_output_state *state,
			 pixman_region32_t *damage)
{
	struct drm_output *output = state->output;
	struct weston_compositor *ec = output->base.compositor;

	output->current_image ^= 1;

	pixman_renderer_output_set_buffer(&output->base,
					  output->image[output->current_image]);
	pixman_renderer_output_set_hw_extra_damage(&output->base,
						   &output->previous_damage);

	ec->renderer->repaint_output(&output->base, damage);

	pixman_region32_copy(&output->previous_damage, damage);

	return drm_fb_ref(output->dumb[output->current_image]);
}

void
drm_output_render(struct drm_output_state *state, pixman_region32_t *damage)
{
	struct drm_output *output = state->output;
	struct weston_compositor *c = output->base.compositor;
	struct drm_plane_state *scanout_state;
	struct drm_plane *scanout_plane = output->scanout_plane;
	struct drm_property_info *damage_info =
		&scanout_plane->props[WDRM_PLANE_FB_DAMAGE_CLIPS];
	struct drm_backend *b = to_drm_backend(c);
	struct drm_fb *fb;
	pixman_region32_t scanout_damage;
	pixman_box32_t *rects;
	int n_rects;

	/* If we already have a client buffer promoted to scanout, then we don't
	 * want to render. */
	scanout_state = drm_output_state_get_plane(state,
						   output->scanout_plane);
	if (scanout_state->fb)
		return;

	/*
	 * If we don't have any damage on the primary plane, and we already
	 * have a renderer buffer active, we can reuse it; else we pass
	 * the damaged region into the renderer to re-render the affected
	 * area.
	 */
	if (!pixman_region32_not_empty(damage) &&
	    scanout_plane->state_cur->fb &&
	    (scanout_plane->state_cur->fb->type == BUFFER_GBM_SURFACE ||
	     scanout_plane->state_cur->fb->type == BUFFER_PIXMAN_DUMB)) {
		fb = drm_fb_ref(scanout_plane->state_cur->fb);
	} else if (b->use_pixman) {
		fb = drm_output_render_pixman(state, damage);
	} else {
		fb = drm_output_render_gl(state, damage);
	}

	if (!fb) {
		drm_plane_state_put_back(scanout_state);
		return;
	}

	scanout_state->fb = fb;
	scanout_state->output = output;

	scanout_state->src_x = 0;
	scanout_state->src_y = 0;
	scanout_state->src_w = fb->width << 16;
	scanout_state->src_h = fb->height << 16;

	scanout_state->dest_x = 0;
	scanout_state->dest_y = 0;
	scanout_state->dest_w = output->base.current_mode->width;
	scanout_state->dest_h = output->base.current_mode->height;

	pixman_region32_subtract(&c->primary_plane.damage,
				 &c->primary_plane.damage, damage);

	/* Don't bother calculating plane damage if the plane doesn't support it */
	if (damage_info->prop_id == 0)
		return;

	pixman_region32_init(&scanout_damage);
	pixman_region32_copy(&scanout_damage, damage);

	if (output->base.zoom.active) {
		weston_matrix_transform_region(&scanout_damage,
					       &output->base.matrix,
					       &scanout_damage);
	} else {
		pixman_region32_translate(&scanout_damage,
					  -output->base.x, -output->base.y);
		weston_transformed_region(output->base.width,
					  output->base.height,
					  output->base.transform,
					  output->base.current_scale,
					  &scanout_damage,
					  &scanout_damage);
	}

	assert(scanout_state->damage_blob_id == 0);

	rects = pixman_region32_rectangles(&scanout_damage, &n_rects);

	/*
	 * If this function fails, the blob id should still be 0.
	 * This tells the kernel there is no damage information, which means
	 * that it will consider the whole plane damaged. While this may
	 * affect efficiency, it should still produce correct results.
	 */
	drmModeCreatePropertyBlob(b->drm.fd, rects,
				  sizeof(*rects) * n_rects,
				  &scanout_state->damage_blob_id);

	pixman_region32_fini(&scanout_damage);
}

static int
drm_output_repaint(struct weston_output *output_base,
		   pixman_region32_t *damage,
		   void *repaint_data)
{
	struct drm_pending_state *pending_state = repaint_data;
	struct drm_output *output = to_drm_output(output_base);
	struct drm_output_state *state = NULL;
	struct drm_plane_state *scanout_state;

	assert(!output->virtual);

	if (output->disable_pending || output->destroy_pending)
		goto err;

	assert(!output->state_last);

	/* If planes have been disabled in the core, we might not have
	 * hit assign_planes at all, so might not have valid output state
	 * here. */
	state = drm_pending_state_get_output(pending_state, output);
	if (!state)
		state = drm_output_state_duplicate(output->state_cur,
						   pending_state,
						   DRM_OUTPUT_STATE_CLEAR_PLANES);
	state->dpms = WESTON_DPMS_ON;

	if (output_base->allow_protection)
		state->protection = output_base->desired_protection;
	else
		state->protection = WESTON_HDCP_DISABLE;

	drm_output_render(state, damage);
	scanout_state = drm_output_state_get_plane(state,
						   output->scanout_plane);
	if (!scanout_state || !scanout_state->fb)
		goto err;

	return 0;

err:
	drm_output_state_free(state);
	return -1;
}

/* Determine the type of vblank synchronization to use for the output.
 *
 * The pipe parameter indicates which CRTC is in use.  Knowing this, we
 * can determine which vblank sequence type to use for it.  Traditional
 * cards had only two CRTCs, with CRTC 0 using no special flags, and
 * CRTC 1 using DRM_VBLANK_SECONDARY.  The first bit of the pipe
 * parameter indicates this.
 *
 * Bits 1-5 of the pipe parameter are 5 bit wide pipe number between
 * 0-31.  If this is non-zero it indicates we're dealing with a
 * multi-gpu situation and we need to calculate the vblank sync
 * using DRM_BLANK_HIGH_CRTC_MASK.
 */
static unsigned int
drm_waitvblank_pipe(struct drm_output *output)
{
	if (output->pipe > 1)
		return (output->pipe << DRM_VBLANK_HIGH_CRTC_SHIFT) &
				DRM_VBLANK_HIGH_CRTC_MASK;
	else if (output->pipe > 0)
		return DRM_VBLANK_SECONDARY;
	else
		return 0;
}

static int
drm_output_start_repaint_loop(struct weston_output *output_base)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_pending_state *pending_state;
	struct drm_plane *scanout_plane = output->scanout_plane;
	struct drm_backend *backend =
		to_drm_backend(output_base->compositor);
	struct timespec ts, tnow;
	struct timespec vbl2now;
	int64_t refresh_nsec;
	int ret;
	drmVBlank vbl = {
		.request.type = DRM_VBLANK_RELATIVE,
		.request.sequence = 0,
		.request.signal = 0,
	};

	if (output->disable_pending || output->destroy_pending)
		return 0;

	if (!output->scanout_plane->state_cur->fb) {
		/* We can't page flip if there's no mode set */
		goto finish_frame;
	}

	/* Need to smash all state in from scratch; current timings might not
	 * be what we want, page flip might not work, etc.
	 */
	if (backend->state_invalid)
		goto finish_frame;

	assert(scanout_plane->state_cur->output == output);

	/* Try to get current msc and timestamp via instant query */
	vbl.request.type |= drm_waitvblank_pipe(output);
	ret = drmWaitVBlank(backend->drm.fd, &vbl);

	/* Error ret or zero timestamp means failure to get valid timestamp */
	if ((ret == 0) && (vbl.reply.tval_sec > 0 || vbl.reply.tval_usec > 0)) {
		ts.tv_sec = vbl.reply.tval_sec;
		ts.tv_nsec = vbl.reply.tval_usec * 1000;

		/* Valid timestamp for most recent vblank - not stale?
		 * Stale ts could happen on Linux 3.17+, so make sure it
		 * is not older than 1 refresh duration since now.
		 */
		weston_compositor_read_presentation_clock(backend->compositor,
							  &tnow);
		timespec_sub(&vbl2now, &tnow, &ts);
		refresh_nsec =
			millihz_to_nsec(output->base.current_mode->refresh);
		if (timespec_to_nsec(&vbl2now) < refresh_nsec) {
			drm_output_update_msc(output, vbl.reply.sequence);
			weston_output_finish_frame(output_base, &ts,
						WP_PRESENTATION_FEEDBACK_INVALID);
			return 0;
		}
	}

	/* Immediate query didn't provide valid timestamp.
	 * Use pageflip fallback.
	 */

	assert(!output->page_flip_pending);
	assert(!output->state_last);

	pending_state = drm_pending_state_alloc(backend);
	drm_output_state_duplicate(output->state_cur, pending_state,
				   DRM_OUTPUT_STATE_PRESERVE_PLANES);

	ret = drm_pending_state_apply(pending_state);
	if (ret != 0) {
		weston_log("applying repaint-start state failed: %s\n",
			   strerror(errno));
		if (ret == -EACCES)
			return -1;
		goto finish_frame;
	}

	return 0;

finish_frame:
	/* if we cannot page-flip, immediately finish frame */
	weston_output_finish_frame(output_base, NULL,
				   WP_PRESENTATION_FEEDBACK_INVALID);
	return 0;
}

/**
 * Begin a new repaint cycle
 *
 * Called by the core compositor at the beginning of a repaint cycle. Creates
 * a new pending_state structure to own any output state created by individual
 * output repaint functions until the repaint is flushed or cancelled.
 */
static void *
drm_repaint_begin(struct weston_compositor *compositor)
{
	struct drm_backend *b = to_drm_backend(compositor);
	struct drm_pending_state *ret;

	ret = drm_pending_state_alloc(b);
	b->repaint_data = ret;

	if (weston_log_scope_is_enabled(b->debug)) {
		char *dbg = weston_compositor_print_scene_graph(compositor);
		drm_debug(b, "[repaint] Beginning repaint; pending_state %p\n",
			  ret);
		drm_debug(b, "%s", dbg);
		free(dbg);
	}

	return ret;
}

/**
 * Flush a repaint set
 *
 * Called by the core compositor when a repaint cycle has been completed
 * and should be flushed. Frees the pending state, transitioning ownership
 * of the output state from the pending state, to the update itself. When
 * the update completes (see drm_output_update_complete), the output
 * state will be freed.
 */
static int
drm_repaint_flush(struct weston_compositor *compositor, void *repaint_data)
{
	struct drm_backend *b = to_drm_backend(compositor);
	struct drm_pending_state *pending_state = repaint_data;
	int ret;

	ret = drm_pending_state_apply(pending_state);
	if (ret != 0)
		weston_log("repaint-flush failed: %s\n", strerror(errno));

	drm_debug(b, "[repaint] flushed pending_state %p\n", pending_state);
	b->repaint_data = NULL;

	return (ret == -EACCES) ? -1 : 0;
}

/**
 * Cancel a repaint set
 *
 * Called by the core compositor when a repaint has finished, so the data
 * held across the repaint cycle should be discarded.
 */
static void
drm_repaint_cancel(struct weston_compositor *compositor, void *repaint_data)
{
	struct drm_backend *b = to_drm_backend(compositor);
	struct drm_pending_state *pending_state = repaint_data;

	drm_pending_state_free(pending_state);
	drm_debug(b, "[repaint] cancel pending_state %p\n", pending_state);
	b->repaint_data = NULL;
}

static int
drm_output_init_pixman(struct drm_output *output, struct drm_backend *b);
static void
drm_output_fini_pixman(struct drm_output *output);

static int
drm_output_switch_mode(struct weston_output *output_base, struct weston_mode *mode)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_backend *b = to_drm_backend(output_base->compositor);
	struct drm_mode *drm_mode = drm_output_choose_mode(output, mode);

	if (!drm_mode) {
		weston_log("%s: invalid resolution %dx%d\n",
			   output_base->name, mode->width, mode->height);
		return -1;
	}

	if (&drm_mode->base == output->base.current_mode)
		return 0;

	output->base.current_mode->flags = 0;

	output->base.current_mode = &drm_mode->base;
	output->base.current_mode->flags =
		WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;

	/* XXX: This drops our current buffer too early, before we've started
	 *      displaying it. Ideally this should be much more atomic and
	 *      integrated with a full repaint cycle, rather than doing a
	 *      sledgehammer modeswitch first, and only later showing new
	 *      content.
	 */
	b->state_invalid = true;

	if (b->use_pixman) {
		drm_output_fini_pixman(output);
		if (drm_output_init_pixman(output, b) < 0) {
			weston_log("failed to init output pixman state with "
				   "new mode\n");
			return -1;
		}
	} else {
		drm_output_fini_egl(output);
		if (drm_output_init_egl(output, b) < 0) {
			weston_log("failed to init output egl state with "
				   "new mode");
			return -1;
		}
	}

	return 0;
}

static int
init_pixman(struct drm_backend *b)
{
	return pixman_renderer_init(b->compositor);
}

/**
 * Create a drm_plane for a hardware plane
 *
 * Creates one drm_plane structure for a hardware plane, and initialises its
 * properties and formats.
 *
 * In the absence of universal plane support, where KMS does not explicitly
 * expose the primary and cursor planes to userspace, this may also create
 * an 'internal' plane for internal management.
 *
 * This function does not add the plane to the list of usable planes in Weston
 * itself; the caller is responsible for this.
 *
 * Call drm_plane_destroy to clean up the plane.
 *
 * @sa drm_output_find_special_plane
 * @param b DRM compositor backend
 * @param kplane DRM plane to create, or NULL if creating internal plane
 * @param output Output to create internal plane for, or NULL
 * @param type Type to use when creating internal plane, or invalid
 * @param format Format to use for internal planes, or 0
 */
static struct drm_plane *
drm_plane_create(struct drm_backend *b, const drmModePlane *kplane,
		 struct drm_output *output, enum wdrm_plane_type type,
		 uint32_t format)
{
	struct drm_plane *plane;
	drmModeObjectProperties *props;
	uint64_t *zpos_range_values;
	uint32_t num_formats = (kplane) ? kplane->count_formats : 1;

	plane = zalloc(sizeof(*plane) +
		       (sizeof(plane->formats[0]) * num_formats));
	if (!plane) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	plane->backend = b;
	plane->count_formats = num_formats;
	plane->state_cur = drm_plane_state_alloc(NULL, plane);
	plane->state_cur->complete = true;

	if (kplane) {
		plane->possible_crtcs = kplane->possible_crtcs;
		plane->plane_id = kplane->plane_id;

		props = drmModeObjectGetProperties(b->drm.fd, kplane->plane_id,
						   DRM_MODE_OBJECT_PLANE);
		if (!props) {
			weston_log("couldn't get plane properties\n");
			goto err;
		}
		drm_property_info_populate(b, plane_props, plane->props,
					   WDRM_PLANE__COUNT, props);
		plane->type =
			drm_property_get_value(&plane->props[WDRM_PLANE_TYPE],
					       props,
					       WDRM_PLANE_TYPE__COUNT);

		zpos_range_values =
			drm_property_get_range_values(&plane->props[WDRM_PLANE_ZPOS],
						      props);

		if (zpos_range_values) {
			plane->zpos_min = zpos_range_values[0];
			plane->zpos_max = zpos_range_values[1];
		} else {
			plane->zpos_min = DRM_PLANE_ZPOS_INVALID_PLANE;
			plane->zpos_max = DRM_PLANE_ZPOS_INVALID_PLANE;
		}

		if (drm_plane_populate_formats(plane, kplane, props,
					       b->fb_modifiers) < 0) {
			drmModeFreeObjectProperties(props);
			goto err;
		}

		drmModeFreeObjectProperties(props);
	}
	else {
		plane->possible_crtcs = (1 << output->pipe);
		plane->plane_id = 0;
		plane->count_formats = 1;
		plane->formats[0].format = format;
		plane->type = type;
		plane->zpos_max = DRM_PLANE_ZPOS_INVALID_PLANE;
		plane->zpos_min = DRM_PLANE_ZPOS_INVALID_PLANE;
	}

	if (plane->type == WDRM_PLANE_TYPE__COUNT)
		goto err_props;

	/* With universal planes, everything is a DRM plane; without
	 * universal planes, the only DRM planes are overlay planes.
	 * Everything else is a fake plane. */
	if (b->universal_planes) {
		assert(kplane);
	} else {
		if (kplane)
			assert(plane->type == WDRM_PLANE_TYPE_OVERLAY);
		else
			assert(plane->type != WDRM_PLANE_TYPE_OVERLAY &&
			       output);
	}

	weston_plane_init(&plane->base, b->compositor, 0, 0);
	wl_list_insert(&b->plane_list, &plane->link);

	return plane;

err_props:
	drm_property_info_free(plane->props, WDRM_PLANE__COUNT);
err:
	drm_plane_state_free(plane->state_cur, true);
	free(plane);
	return NULL;
}

/**
 * Find, or create, a special-purpose plane
 *
 * Primary and cursor planes are a special case, in that before universal
 * planes, they are driven by non-plane API calls. Without universal plane
 * support, the only way to configure a primary plane is via drmModeSetCrtc,
 * and the only way to configure a cursor plane is drmModeSetCursor2.
 *
 * Although they may actually be regular planes in the hardware, without
 * universal plane support, these planes are not actually exposed to
 * userspace in the regular plane list.
 *
 * However, for ease of internal tracking, we want to manage all planes
 * through the same drm_plane structures. Therefore, when we are running
 * without universal plane support, we create fake drm_plane structures
 * to track these planes.
 *
 * @param b DRM backend
 * @param output Output to use for plane
 * @param type Type of plane
 */
static struct drm_plane *
drm_output_find_special_plane(struct drm_backend *b, struct drm_output *output,
			      enum wdrm_plane_type type)
{
	struct drm_plane *plane;

	if (!b->universal_planes) {
		uint32_t format;

		switch (type) {
		case WDRM_PLANE_TYPE_CURSOR:
			format = DRM_FORMAT_ARGB8888;
			break;
		case WDRM_PLANE_TYPE_PRIMARY:
			/* We don't know what formats the primary plane supports
			 * before universal planes, so we just assume that the
			 * GBM format works; however, this isn't set until after
			 * the output is created. */
			format = 0;
			break;
		default:
			assert(!"invalid type in drm_output_find_special_plane");
			break;
		}

		return drm_plane_create(b, NULL, output, type, format);
	}

	wl_list_for_each(plane, &b->plane_list, link) {
		struct drm_output *tmp;
		bool found_elsewhere = false;

		if (plane->type != type)
			continue;
		if (!drm_plane_is_available(plane, output))
			continue;

		/* On some platforms, primary/cursor planes can roam
		 * between different CRTCs, so make sure we don't claim the
		 * same plane for two outputs. */
		wl_list_for_each(tmp, &b->compositor->output_list,
				 base.link) {
			if (tmp->cursor_plane == plane ||
			    tmp->scanout_plane == plane) {
				found_elsewhere = true;
				break;
			}
		}

		if (found_elsewhere)
			continue;

		plane->possible_crtcs = (1 << output->pipe);
		return plane;
	}

	return NULL;
}

/**
 * Destroy one DRM plane
 *
 * Destroy a DRM plane, removing it from screen and releasing its retained
 * buffers in the process. The counterpart to drm_plane_create.
 *
 * @param plane Plane to deallocate (will be freed)
 */
static void
drm_plane_destroy(struct drm_plane *plane)
{
	if (plane->type == WDRM_PLANE_TYPE_OVERLAY)
		drmModeSetPlane(plane->backend->drm.fd, plane->plane_id,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	drm_plane_state_free(plane->state_cur, true);
	drm_property_info_free(plane->props, WDRM_PLANE__COUNT);
	weston_plane_release(&plane->base);
	wl_list_remove(&plane->link);
	free(plane);
}

/**
 * Initialise sprites (overlay planes)
 *
 * Walk the list of provided DRM planes, and add overlay planes.
 *
 * Call destroy_sprites to free these planes.
 *
 * @param b DRM compositor backend
 */
static void
create_sprites(struct drm_backend *b)
{
	drmModePlaneRes *kplane_res;
	drmModePlane *kplane;
	struct drm_plane *drm_plane;
	uint32_t i;
	kplane_res = drmModeGetPlaneResources(b->drm.fd);
	if (!kplane_res) {
		weston_log("failed to get plane resources: %s\n",
			strerror(errno));
		return;
	}

	for (i = 0; i < kplane_res->count_planes; i++) {
		kplane = drmModeGetPlane(b->drm.fd, kplane_res->planes[i]);
		if (!kplane)
			continue;

		drm_plane = drm_plane_create(b, kplane, NULL,
		                             WDRM_PLANE_TYPE__COUNT, 0);
		drmModeFreePlane(kplane);
		if (!drm_plane)
			continue;

		if (drm_plane->type == WDRM_PLANE_TYPE_OVERLAY)
			weston_compositor_stack_plane(b->compositor,
						      &drm_plane->base,
						      &b->compositor->primary_plane);
	}

	drmModeFreePlaneResources(kplane_res);
}

/**
 * Clean up sprites (overlay planes)
 *
 * The counterpart to create_sprites.
 *
 * @param b DRM compositor backend
 */
static void
destroy_sprites(struct drm_backend *b)
{
	struct drm_plane *plane, *next;

	wl_list_for_each_safe(plane, next, &b->plane_list, link)
		drm_plane_destroy(plane);
}

/* returns a value between 0-255 range, where higher is brighter */
static uint32_t
drm_get_backlight(struct drm_head *head)
{
	long brightness, max_brightness, norm;

	brightness = backlight_get_brightness(head->backlight);
	max_brightness = backlight_get_max_brightness(head->backlight);

	/* convert it on a scale of 0 to 255 */
	norm = (brightness * 255)/(max_brightness);

	return (uint32_t) norm;
}

/* values accepted are between 0-255 range */
static void
drm_set_backlight(struct weston_output *output_base, uint32_t value)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_head *head;
	long max_brightness, new_brightness;

	if (value > 255)
		return;

	wl_list_for_each(head, &output->base.head_list, base.output_link) {
		if (!head->backlight)
			return;

		max_brightness = backlight_get_max_brightness(head->backlight);

		/* get denormalized value */
		new_brightness = (value * max_brightness) / 255;

		backlight_set_brightness(head->backlight, new_brightness);
	}
}

static void
drm_output_init_backlight(struct drm_output *output)
{
	struct weston_head *base;
	struct drm_head *head;

	output->base.set_backlight = NULL;

	wl_list_for_each(base, &output->base.head_list, output_link) {
		head = to_drm_head(base);

		if (head->backlight) {
			weston_log("Initialized backlight for head '%s', device %s\n",
				   head->base.name, head->backlight->path);

			if (!output->base.set_backlight) {
				output->base.set_backlight = drm_set_backlight;
				output->base.backlight_current =
							drm_get_backlight(head);
			}
		}
	}
}

/**
 * Power output on or off
 *
 * The DPMS/power level of an output is used to switch it on or off. This
 * is DRM's hook for doing so, which can called either as part of repaint,
 * or independently of the repaint loop.
 *
 * If we are called as part of repaint, we simply set the relevant bit in
 * state and return.
 *
 * This function is never called on a virtual output.
 */
static void
drm_set_dpms(struct weston_output *output_base, enum dpms_enum level)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_backend *b = to_drm_backend(output_base->compositor);
	struct drm_pending_state *pending_state = b->repaint_data;
	struct drm_output_state *state;
	int ret;

	assert(!output->virtual);

	if (output->state_cur->dpms == level)
		return;

	/* If we're being called during the repaint loop, then this is
	 * simple: discard any previously-generated state, and create a new
	 * state where we disable everything. When we come to flush, this
	 * will be applied.
	 *
	 * However, we need to be careful: we can be called whilst another
	 * output is in its repaint cycle (pending_state exists), but our
	 * output still has an incomplete state application outstanding.
	 * In that case, we need to wait until that completes. */
	if (pending_state && !output->state_last) {
		/* The repaint loop already sets DPMS on; we don't need to
		 * explicitly set it on here, as it will already happen
		 * whilst applying the repaint state. */
		if (level == WESTON_DPMS_ON)
			return;

		state = drm_pending_state_get_output(pending_state, output);
		if (state)
			drm_output_state_free(state);
		state = drm_output_get_disable_state(pending_state, output);
		return;
	}

	/* As we throw everything away when disabling, just send us back through
	 * a repaint cycle. */
	if (level == WESTON_DPMS_ON) {
		if (output->dpms_off_pending)
			output->dpms_off_pending = false;
		weston_output_schedule_repaint(output_base);
		return;
	}

	/* If we've already got a request in the pipeline, then we need to
	 * park our DPMS request until that request has quiesced. */
	if (output->state_last) {
		output->dpms_off_pending = true;
		return;
	}

	pending_state = drm_pending_state_alloc(b);
	drm_output_get_disable_state(pending_state, output);
	ret = drm_pending_state_apply_sync(pending_state);
	if (ret != 0)
		weston_log("drm_set_dpms: couldn't disable output?\n");
}

static const char * const connector_type_names[] = {
	[DRM_MODE_CONNECTOR_Unknown]     = "Unknown",
	[DRM_MODE_CONNECTOR_VGA]         = "VGA",
	[DRM_MODE_CONNECTOR_DVII]        = "DVI-I",
	[DRM_MODE_CONNECTOR_DVID]        = "DVI-D",
	[DRM_MODE_CONNECTOR_DVIA]        = "DVI-A",
	[DRM_MODE_CONNECTOR_Composite]   = "Composite",
	[DRM_MODE_CONNECTOR_SVIDEO]      = "SVIDEO",
	[DRM_MODE_CONNECTOR_LVDS]        = "LVDS",
	[DRM_MODE_CONNECTOR_Component]   = "Component",
	[DRM_MODE_CONNECTOR_9PinDIN]     = "DIN",
	[DRM_MODE_CONNECTOR_DisplayPort] = "DP",
	[DRM_MODE_CONNECTOR_HDMIA]       = "HDMI-A",
	[DRM_MODE_CONNECTOR_HDMIB]       = "HDMI-B",
	[DRM_MODE_CONNECTOR_TV]          = "TV",
	[DRM_MODE_CONNECTOR_eDP]         = "eDP",
	[DRM_MODE_CONNECTOR_VIRTUAL]     = "Virtual",
	[DRM_MODE_CONNECTOR_DSI]         = "DSI",
	[DRM_MODE_CONNECTOR_DPI]         = "DPI",
};

/** Create a name given a DRM connector
 *
 * \param con The DRM connector whose type and id form the name.
 * \return A newly allocate string, or NULL on error. Must be free()'d
 * after use.
 *
 * The name does not identify the DRM display device.
 */
static char *
make_connector_name(const drmModeConnector *con)
{
	char *name;
	const char *type_name = NULL;
	int ret;

	if (con->connector_type < ARRAY_LENGTH(connector_type_names))
		type_name = connector_type_names[con->connector_type];

	if (!type_name)
		type_name = "UNNAMED";

	ret = asprintf(&name, "%s-%d", type_name, con->connector_type_id);
	if (ret < 0)
		return NULL;

	return name;
}

static int
drm_output_init_pixman(struct drm_output *output, struct drm_backend *b)
{
	int w = output->base.current_mode->width;
	int h = output->base.current_mode->height;
	uint32_t format = output->gbm_format;
	uint32_t pixman_format;
	unsigned int i;
	const struct pixman_renderer_output_options options = {
		.use_shadow = b->use_pixman_shadow,
	};

	switch (format) {
		case DRM_FORMAT_XRGB8888:
			pixman_format = PIXMAN_x8r8g8b8;
			break;
		case DRM_FORMAT_RGB565:
			pixman_format = PIXMAN_r5g6b5;
			break;
		default:
			weston_log("Unsupported pixman format 0x%x\n", format);
			return -1;
	}

	/* FIXME error checking */
	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		output->dumb[i] = drm_fb_create_dumb(b, w, h, format);
		if (!output->dumb[i])
			goto err;

		output->image[i] =
			pixman_image_create_bits(pixman_format, w, h,
						 output->dumb[i]->map,
						 output->dumb[i]->strides[0]);
		if (!output->image[i])
			goto err;
	}

	if (pixman_renderer_output_create(&output->base, &options) < 0)
 		goto err;

	weston_log("DRM: output %s %s shadow framebuffer.\n", output->base.name,
		   b->use_pixman_shadow ? "uses" : "does not use");

	pixman_region32_init_rect(&output->previous_damage,
				  output->base.x, output->base.y, output->base.width, output->base.height);

	return 0;

err:
	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		if (output->dumb[i])
			drm_fb_unref(output->dumb[i]);
		if (output->image[i])
			pixman_image_unref(output->image[i]);

		output->dumb[i] = NULL;
		output->image[i] = NULL;
	}

	return -1;
}

static void
drm_output_fini_pixman(struct drm_output *output)
{
	struct drm_backend *b = to_drm_backend(output->base.compositor);
	unsigned int i;

	/* Destroying the Pixman surface will destroy all our buffers,
	 * regardless of refcount. Ensure we destroy them here. */
	if (!b->shutting_down &&
	    output->scanout_plane->state_cur->fb &&
	    output->scanout_plane->state_cur->fb->type == BUFFER_PIXMAN_DUMB) {
		drm_plane_reset_state(output->scanout_plane);
	}

	pixman_renderer_output_destroy(&output->base);
	pixman_region32_fini(&output->previous_damage);

	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		pixman_image_unref(output->image[i]);
		drm_fb_unref(output->dumb[i]);
		output->dumb[i] = NULL;
		output->image[i] = NULL;
	}
}

static void
setup_output_seat_constraint(struct drm_backend *b,
			     struct weston_output *output,
			     const char *s)
{
	if (strcmp(s, "") != 0) {
		struct weston_pointer *pointer;
		struct udev_seat *seat;

		seat = udev_seat_get_named(&b->input, s);
		if (!seat)
			return;

		seat->base.output = output;

		pointer = weston_seat_get_pointer(&seat->base);
		if (pointer)
			weston_pointer_clamp(pointer,
					     &pointer->x,
					     &pointer->y);
	}
}

static int
drm_output_attach_head(struct weston_output *output_base,
		       struct weston_head *head_base)
{
	struct drm_backend *b = to_drm_backend(output_base->compositor);

	if (wl_list_length(&output_base->head_list) >= MAX_CLONED_CONNECTORS)
		return -1;

	if (!output_base->enabled)
		return 0;

	/* XXX: ensure the configuration will work.
	 * This is actually impossible without major infrastructure
	 * work. */

	/* Need to go through modeset to add connectors. */
	/* XXX: Ideally we'd do this per-output, not globally. */
	/* XXX: Doing it globally, what guarantees another output's update
	 * will not clear the flag before this output is updated?
	 */
	b->state_invalid = true;

	weston_output_schedule_repaint(output_base);

	return 0;
}

static void
drm_output_detach_head(struct weston_output *output_base,
		       struct weston_head *head_base)
{
	struct drm_backend *b = to_drm_backend(output_base->compositor);

	if (!output_base->enabled)
		return;

	/* Need to go through modeset to drop connectors that should no longer
	 * be driven. */
	/* XXX: Ideally we'd do this per-output, not globally. */
	b->state_invalid = true;

	weston_output_schedule_repaint(output_base);
}

int
parse_gbm_format(const char *s, uint32_t default_value, uint32_t *gbm_format)
{
	const struct pixel_format_info *pinfo;

	if (s == NULL) {
		*gbm_format = default_value;

		return 0;
	}

	pinfo = pixel_format_get_info_by_drm_name(s);
	if (!pinfo) {
		weston_log("fatal: unrecognized pixel format: %s\n", s);

		return -1;
	}

	/* GBM formats and DRM formats are identical. */
	*gbm_format = pinfo->format;

	return 0;
}

static int
drm_head_read_current_setup(struct drm_head *head, struct drm_backend *backend)
{
	int drm_fd = backend->drm.fd;
	drmModeEncoder *encoder;
	drmModeCrtc *crtc;

	/* Get the current mode on the crtc that's currently driving
	 * this connector. */
	encoder = drmModeGetEncoder(drm_fd, head->connector->encoder_id);
	if (encoder != NULL) {
		head->inherited_crtc_id = encoder->crtc_id;

		crtc = drmModeGetCrtc(drm_fd, encoder->crtc_id);
		drmModeFreeEncoder(encoder);

		if (crtc == NULL)
			return -1;
		if (crtc->mode_valid)
			head->inherited_mode = crtc->mode;
		drmModeFreeCrtc(crtc);
	}

	return 0;
}

static void
drm_output_set_gbm_format(struct weston_output *base,
			  const char *gbm_format)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);

	if (parse_gbm_format(gbm_format, b->gbm_format, &output->gbm_format) == -1)
		output->gbm_format = b->gbm_format;
}

static void
drm_output_set_seat(struct weston_output *base,
		    const char *seat)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);

	setup_output_seat_constraint(b, &output->base,
				     seat ? seat : "");
}

static int
drm_output_init_gamma_size(struct drm_output *output)
{
	struct drm_backend *backend = to_drm_backend(output->base.compositor);
	drmModeCrtc *crtc;

	assert(output->base.compositor);
	assert(output->crtc_id != 0);
	crtc = drmModeGetCrtc(backend->drm.fd, output->crtc_id);
	if (!crtc)
		return -1;

	output->base.gamma_size = crtc->gamma_size;

	drmModeFreeCrtc(crtc);

	return 0;
}

static uint32_t
drm_head_get_possible_crtcs_mask(struct drm_head *head)
{
	uint32_t possible_crtcs = 0;
	drmModeEncoder *encoder;
	int i;

	for (i = 0; i < head->connector->count_encoders; i++) {
		encoder = drmModeGetEncoder(head->backend->drm.fd,
					    head->connector->encoders[i]);
		if (!encoder)
			continue;

		possible_crtcs |= encoder->possible_crtcs;
		drmModeFreeEncoder(encoder);
	}

	return possible_crtcs;
}

static int
drm_crtc_get_index(drmModeRes *resources, uint32_t crtc_id)
{
	int i;

	for (i = 0; i < resources->count_crtcs; i++) {
		if (resources->crtcs[i] == crtc_id)
			return i;
	}

	assert(0 && "unknown crtc id");
	return -1;
}

/** Pick a CRTC that might be able to drive all attached connectors
 *
 * @param output The output whose attached heads to include.
 * @param resources The DRM KMS resources.
 * @return CRTC index, or -1 on failure or not found.
 */
static int
drm_output_pick_crtc(struct drm_output *output, drmModeRes *resources)
{
	struct drm_backend *backend;
	struct weston_head *base;
	struct drm_head *head;
	uint32_t possible_crtcs = 0xffffffff;
	int existing_crtc[32];
	unsigned j, n = 0;
	uint32_t crtc_id;
	int best_crtc_index = -1;
	int fallback_crtc_index = -1;
	int i;
	bool match;

	backend = to_drm_backend(output->base.compositor);

	/* This algorithm ignores drmModeEncoder::possible_clones restriction,
	 * because it is more often set wrong than not in the kernel. */

	/* Accumulate a mask of possible crtcs and find existing routings. */
	wl_list_for_each(base, &output->base.head_list, output_link) {
		head = to_drm_head(base);

		possible_crtcs &= drm_head_get_possible_crtcs_mask(head);

		crtc_id = head->inherited_crtc_id;
		if (crtc_id > 0 && n < ARRAY_LENGTH(existing_crtc))
			existing_crtc[n++] = drm_crtc_get_index(resources,
								crtc_id);
	}

	/* Find a crtc that could drive each connector individually at least,
	 * and prefer existing routings. */
	for (i = 0; i < resources->count_crtcs; i++) {
		crtc_id = resources->crtcs[i];

		/* Could the crtc not drive each connector? */
		if (!(possible_crtcs & (1 << i)))
			continue;

		/* Is the crtc already in use? */
		if (drm_output_find_by_crtc(backend, crtc_id))
			continue;

		/* Try to preserve the existing CRTC -> connector routing;
		 * it makes initialisation faster, and also since we have a
		 * very dumb picking algorithm, may preserve a better
		 * choice. */
		for (j = 0; j < n; j++) {
			if (existing_crtc[j] == i)
				return i;
		}

		/* Check if any other head had existing routing to this CRTC.
		 * If they did, this is not the best CRTC as it might be needed
		 * for another output we haven't enabled yet. */
		match = false;
		wl_list_for_each(base, &backend->compositor->head_list,
				 compositor_link) {
			head = to_drm_head(base);

			if (head->base.output == &output->base)
				continue;

			if (weston_head_is_enabled(&head->base))
				continue;

			if (head->inherited_crtc_id == crtc_id) {
				match = true;
				break;
			}
		}
		if (!match)
			best_crtc_index = i;

		fallback_crtc_index = i;
	}

	if (best_crtc_index != -1)
		return best_crtc_index;

	if (fallback_crtc_index != -1)
		return fallback_crtc_index;

	/* Likely possible_crtcs was empty due to asking for clones,
	 * but since the DRM documentation says the kernel lies, let's
	 * pick one crtc anyway. Trial and error is the only way to
	 * be sure if something doesn't work. */

	/* First pick any existing assignment. */
	for (j = 0; j < n; j++) {
		crtc_id = resources->crtcs[existing_crtc[j]];
		if (!drm_output_find_by_crtc(backend, crtc_id))
			return existing_crtc[j];
	}

	/* Otherwise pick any available crtc. */
	for (i = 0; i < resources->count_crtcs; i++) {
		crtc_id = resources->crtcs[i];

		if (!drm_output_find_by_crtc(backend, crtc_id))
			return i;
	}

	return -1;
}

/** Allocate a CRTC for the output
 *
 * @param output The output with no allocated CRTC.
 * @param resources DRM KMS resources.
 * @return 0 on success, -1 on failure.
 *
 * Finds a free CRTC that might drive the attached connectors, reserves the CRTC
 * for the output, and loads the CRTC properties.
 *
 * Populates the cursor and scanout planes.
 *
 * On failure, the output remains without a CRTC.
 */
static int
drm_output_init_crtc(struct drm_output *output, drmModeRes *resources)
{
	struct drm_backend *b = to_drm_backend(output->base.compositor);
	drmModeObjectPropertiesPtr props;
	int i;

	assert(output->crtc_id == 0);

	i = drm_output_pick_crtc(output, resources);
	if (i < 0) {
		weston_log("Output '%s': No available CRTCs.\n",
			   output->base.name);
		return -1;
	}

	output->crtc_id = resources->crtcs[i];
	output->pipe = i;

	props = drmModeObjectGetProperties(b->drm.fd, output->crtc_id,
					   DRM_MODE_OBJECT_CRTC);
	if (!props) {
		weston_log("failed to get CRTC properties\n");
		goto err_crtc;
	}
	drm_property_info_populate(b, crtc_props, output->props_crtc,
				   WDRM_CRTC__COUNT, props);
	drmModeFreeObjectProperties(props);

	output->scanout_plane =
		drm_output_find_special_plane(b, output,
					      WDRM_PLANE_TYPE_PRIMARY);
	if (!output->scanout_plane) {
		weston_log("Failed to find primary plane for output %s\n",
			   output->base.name);
		goto err_crtc;
	}

	/* Without universal planes, we can't discover which formats are
	 * supported by the primary plane; we just hope that the GBM format
	 * works. */
	if (!b->universal_planes)
		output->scanout_plane->formats[0].format = output->gbm_format;

	/* Failing to find a cursor plane is not fatal, as we'll fall back
	 * to software cursor. */
	output->cursor_plane =
		drm_output_find_special_plane(b, output,
					      WDRM_PLANE_TYPE_CURSOR);

	wl_array_remove_uint32(&b->unused_crtcs, output->crtc_id);

	return 0;

err_crtc:
	output->crtc_id = 0;
	output->pipe = 0;

	return -1;
}

/** Free the CRTC from the output
 *
 * @param output The output whose CRTC to deallocate.
 *
 * The CRTC reserved for the given output becomes free to use again.
 */
static void
drm_output_fini_crtc(struct drm_output *output)
{
	struct drm_backend *b = to_drm_backend(output->base.compositor);
	uint32_t *unused;

	/* If the compositor is already shutting down, the planes have already
	 * been destroyed. */
	if (!b->shutting_down) {
		if (!b->universal_planes) {
			/* Without universal planes, our special planes are
			 * pseudo-planes allocated at output creation, freed at
			 * output destruction, and not usable by other outputs.
			 */
			if (output->cursor_plane)
				drm_plane_destroy(output->cursor_plane);
			if (output->scanout_plane)
				drm_plane_destroy(output->scanout_plane);
		} else {
			/* With universal planes, the 'special' planes are
			 * allocated at startup, freed at shutdown, and live on
			 * the plane list in between. We want the planes to
			 * continue to exist and be freed up for other outputs.
			 */
			if (output->cursor_plane)
				drm_plane_reset_state(output->cursor_plane);
			if (output->scanout_plane)
				drm_plane_reset_state(output->scanout_plane);
		}
	}

	drm_property_info_free(output->props_crtc, WDRM_CRTC__COUNT);

	assert(output->crtc_id != 0);

	unused = wl_array_add(&b->unused_crtcs, sizeof(*unused));
	*unused = output->crtc_id;

	/* Force resetting unused CRTCs */
	b->state_invalid = true;

	output->crtc_id = 0;
	output->cursor_plane = NULL;
	output->scanout_plane = NULL;
}

static int
drm_output_enable(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);
	drmModeRes *resources;
	int ret;

	assert(!output->virtual);

	resources = drmModeGetResources(b->drm.fd);
	if (!resources) {
		weston_log("drmModeGetResources failed\n");
		return -1;
	}
	ret = drm_output_init_crtc(output, resources);
	drmModeFreeResources(resources);
	if (ret < 0)
		return -1;

	if (drm_output_init_gamma_size(output) < 0)
		goto err;

	if (b->pageflip_timeout)
		drm_output_pageflip_timer_create(output);

	if (b->use_pixman) {
		if (drm_output_init_pixman(output, b) < 0) {
			weston_log("Failed to init output pixman state\n");
			goto err;
		}
	} else if (drm_output_init_egl(output, b) < 0) {
		weston_log("Failed to init output gl state\n");
		goto err;
	}

	drm_output_init_backlight(output);

	output->base.start_repaint_loop = drm_output_start_repaint_loop;
	output->base.repaint = drm_output_repaint;
	output->base.assign_planes = drm_assign_planes;
	output->base.set_dpms = drm_set_dpms;
	output->base.switch_mode = drm_output_switch_mode;
	output->base.set_gamma = drm_output_set_gamma;

	if (output->cursor_plane)
		weston_compositor_stack_plane(b->compositor,
					      &output->cursor_plane->base,
					      NULL);
	else
		b->cursors_are_broken = true;

	weston_compositor_stack_plane(b->compositor,
				      &output->scanout_plane->base,
				      &b->compositor->primary_plane);

	weston_log("Output %s (crtc %d) video modes:\n",
		   output->base.name, output->crtc_id);
	drm_output_print_modes(output);

	return 0;

err:
	drm_output_fini_crtc(output);

	return -1;
}

static void
drm_output_deinit(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);

	if (b->use_pixman)
		drm_output_fini_pixman(output);
	else
		drm_output_fini_egl(output);

	/* Since our planes are no longer in use anywhere, remove their base
	 * weston_plane's link from the plane stacking list, unless we're
	 * shutting down, in which case the plane has already been
	 * destroyed. */
	if (!b->shutting_down) {
		wl_list_remove(&output->scanout_plane->base.link);
		wl_list_init(&output->scanout_plane->base.link);

		if (output->cursor_plane) {
			wl_list_remove(&output->cursor_plane->base.link);
			wl_list_init(&output->cursor_plane->base.link);
			/* Turn off hardware cursor */
			drmModeSetCursor(b->drm.fd, output->crtc_id, 0, 0, 0);
		}
	}

	drm_output_fini_crtc(output);
}

static void
drm_head_destroy(struct drm_head *head);

static void
drm_output_destroy(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);

	assert(!output->virtual);

	if (output->page_flip_pending || output->atomic_complete_pending) {
		output->destroy_pending = true;
		weston_log("destroy output while page flip pending\n");
		return;
	}

	if (output->base.enabled)
		drm_output_deinit(&output->base);

	drm_mode_list_destroy(b, &output->base.mode_list);

	if (output->pageflip_timer)
		wl_event_source_remove(output->pageflip_timer);

	weston_output_release(&output->base);

	assert(!output->state_last);
	drm_output_state_free(output->state_cur);

	free(output);
}

static int
drm_output_disable(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);

	assert(!output->virtual);

	if (output->page_flip_pending || output->atomic_complete_pending) {
		output->disable_pending = true;
		return -1;
	}

	weston_log("Disabling output %s\n", output->base.name);

	if (output->base.enabled)
		drm_output_deinit(&output->base);

	output->disable_pending = false;

	return 0;
}

/**
 * Update the list of unused connectors and CRTCs
 *
 * This keeps the unused_crtc arrays up to date.
 *
 * @param b Weston backend structure
 * @param resources DRM resources for this device
 */
static void
drm_backend_update_unused_outputs(struct drm_backend *b, drmModeRes *resources)
{
	int i;

	wl_array_release(&b->unused_crtcs);
	wl_array_init(&b->unused_crtcs);

	for (i = 0; i < resources->count_crtcs; i++) {
		struct drm_output *output;
		uint32_t *crtc_id;

		output = drm_output_find_by_crtc(b, resources->crtcs[i]);
		if (output && output->base.enabled)
			continue;

		crtc_id = wl_array_add(&b->unused_crtcs, sizeof(*crtc_id));
		*crtc_id = resources->crtcs[i];
	}
}

/*
 * This function converts the protection status from drm values to
 * weston_hdcp_protection status. The drm values as read from the connector
 * properties "Content Protection" and "HDCP Content Type" need to be converted
 * to appropriate weston values, that can be sent to a client application.
 */
static int
get_weston_protection_from_drm(enum wdrm_content_protection_state protection,
			       enum wdrm_hdcp_content_type type,
			       enum weston_hdcp_protection *weston_protection)

{
	if (protection >= WDRM_CONTENT_PROTECTION__COUNT)
		return -1;
	if (protection == WDRM_CONTENT_PROTECTION_DESIRED ||
	    protection == WDRM_CONTENT_PROTECTION_UNDESIRED) {
		*weston_protection = WESTON_HDCP_DISABLE;
		return 0;
	}
	if (type >= WDRM_HDCP_CONTENT_TYPE__COUNT)
		return -1;
	if (type == WDRM_HDCP_CONTENT_TYPE0) {
		*weston_protection = WESTON_HDCP_ENABLE_TYPE_0;
		return 0;
	}
	if (type == WDRM_HDCP_CONTENT_TYPE1) {
		*weston_protection = WESTON_HDCP_ENABLE_TYPE_1;
		return 0;
	}
	return -1;
}

/**
 * Get current content-protection status for a given head.
 *
 * @param head drm_head, whose protection is to be retrieved
 * @param props drm property object of the connector, related to the head
 * @return protection status in case of success, -1 otherwise
 */
static enum weston_hdcp_protection
drm_head_get_current_protection(struct drm_head *head,
				drmModeObjectProperties *props)
{
	struct drm_property_info *info;
	enum wdrm_content_protection_state protection;
	enum wdrm_hdcp_content_type type;
	enum weston_hdcp_protection weston_hdcp = WESTON_HDCP_DISABLE;

	info = &head->props_conn[WDRM_CONNECTOR_CONTENT_PROTECTION];
	protection = drm_property_get_value(info, props,
					    WDRM_CONTENT_PROTECTION__COUNT);

	if (protection == WDRM_CONTENT_PROTECTION__COUNT)
		return WESTON_HDCP_DISABLE;

	info = &head->props_conn[WDRM_CONNECTOR_HDCP_CONTENT_TYPE];
	type = drm_property_get_value(info, props,
				      WDRM_HDCP_CONTENT_TYPE__COUNT);

	/*
	 * In case of platforms supporting HDCP1.4, only property
	 * 'Content Protection' is exposed and not the 'HDCP Content Type'
	 * for such cases HDCP Type 0 should be considered as the content-type.
	 */

	if (type == WDRM_HDCP_CONTENT_TYPE__COUNT)
		type = WDRM_HDCP_CONTENT_TYPE0;

	if (get_weston_protection_from_drm(protection, type,
					   &weston_hdcp) == -1) {
		weston_log("Invalid drm protection:%d type:%d, for head:%s connector-id:%d\n",
			   protection, type, head->base.name,
			   head->connector_id);
		return WESTON_HDCP_DISABLE;
	}

	return weston_hdcp;
}

/** Replace connector data and monitor information
 *
 * @param head The head to update.
 * @param connector The connector data to be owned by the head, must match
 * the head's connector ID.
 * @return 0 on success, -1 on failure.
 *
 * Takes ownership of @c connector on success, not on failure.
 *
 * May schedule a heads changed call.
 */
static int
drm_head_assign_connector_info(struct drm_head *head,
			       drmModeConnector *connector)
{
	drmModeObjectProperties *props;

	assert(connector);
	assert(head->connector_id == connector->connector_id);

	props = drmModeObjectGetProperties(head->backend->drm.fd,
					   head->connector_id,
					   DRM_MODE_OBJECT_CONNECTOR);
	if (!props) {
		weston_log("Error: failed to get connector '%s' properties\n",
			   head->base.name);
		return -1;
	}

	if (head->connector)
		drmModeFreeConnector(head->connector);
	head->connector = connector;

	drm_property_info_populate(head->backend, connector_props,
				   head->props_conn,
				   WDRM_CONNECTOR__COUNT, props);
	update_head_from_connector(head, props);

	weston_head_set_content_protection_status(&head->base,
					 drm_head_get_current_protection(head, props));
	drmModeFreeObjectProperties(props);

	return 0;
}

static void
drm_head_log_info(struct drm_head *head, const char *msg)
{
	if (head->base.connected) {
		weston_log("DRM: head '%s' %s, connector %d is connected, "
			   "EDID make '%s', model '%s', serial '%s'\n",
			   head->base.name, msg, head->connector_id,
			   head->base.make, head->base.model,
			   head->base.serial_number ?: "");
	} else {
		weston_log("DRM: head '%s' %s, connector %d is disconnected.\n",
			   head->base.name, msg, head->connector_id);
	}
}

/** Update connector and monitor information
 *
 * @param head The head to update.
 *
 * Re-reads the DRM property lists for the connector and updates monitor
 * information and connection status. This may schedule a heads changed call
 * to the user.
 */
static void
drm_head_update_info(struct drm_head *head)
{
	drmModeConnector *connector;

	connector = drmModeGetConnector(head->backend->drm.fd,
					head->connector_id);
	if (!connector) {
		weston_log("DRM: getting connector info for '%s' failed.\n",
			   head->base.name);
		return;
	}

	if (drm_head_assign_connector_info(head, connector) < 0)
		drmModeFreeConnector(connector);

	if (head->base.device_changed)
		drm_head_log_info(head, "updated");
}

/**
 * Create a Weston head for a connector
 *
 * Given a DRM connector, create a matching drm_head structure and add it
 * to Weston's head list.
 *
 * @param backend Weston backend structure
 * @param connector_id DRM connector ID for the head
 * @param drm_device udev device pointer
 * @returns The new head, or NULL on failure.
 */
static struct drm_head *
drm_head_create(struct drm_backend *backend, uint32_t connector_id,
		struct udev_device *drm_device)
{
	struct drm_head *head;
	drmModeConnector *connector;
	char *name;

	head = zalloc(sizeof *head);
	if (!head)
		return NULL;

	connector = drmModeGetConnector(backend->drm.fd, connector_id);
	if (!connector)
		goto err_alloc;

	name = make_connector_name(connector);
	if (!name)
		goto err_alloc;

	weston_head_init(&head->base, name);
	free(name);

	head->connector_id = connector_id;
	head->backend = backend;

	head->backlight = backlight_init(drm_device, connector->connector_type);

	if (drm_head_assign_connector_info(head, connector) < 0)
		goto err_init;

	if (head->connector->connector_type == DRM_MODE_CONNECTOR_LVDS ||
	    head->connector->connector_type == DRM_MODE_CONNECTOR_eDP)
		weston_head_set_internal(&head->base);

	if (drm_head_read_current_setup(head, backend) < 0) {
		weston_log("Failed to retrieve current mode from connector %d.\n",
			   head->connector_id);
		/* Not fatal. */
	}

	weston_compositor_add_head(backend->compositor, &head->base);
	drm_head_log_info(head, "found");

	return head;

err_init:
	weston_head_release(&head->base);

err_alloc:
	if (connector)
		drmModeFreeConnector(connector);

	free(head);

	return NULL;
}

static void
drm_head_destroy(struct drm_head *head)
{
	weston_head_release(&head->base);

	drm_property_info_free(head->props_conn, WDRM_CONNECTOR__COUNT);
	drmModeFreeConnector(head->connector);

	if (head->backlight)
		backlight_destroy(head->backlight);

	free(head);
}

/**
 * Create a Weston output structure
 *
 * Create an "empty" drm_output. This is the implementation of
 * weston_backend::create_output.
 *
 * Creating an output is usually followed by drm_output_attach_head()
 * and drm_output_enable() to make use of it.
 *
 * @param compositor The compositor instance.
 * @param name Name for the new output.
 * @returns The output, or NULL on failure.
 */
static struct weston_output *
drm_output_create(struct weston_compositor *compositor, const char *name)
{
	struct drm_backend *b = to_drm_backend(compositor);
	struct drm_output *output;

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	output->backend = b;
#ifdef BUILD_DRM_GBM
	output->gbm_bo_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
#endif

	weston_output_init(&output->base, compositor, name);

	output->base.enable = drm_output_enable;
	output->base.destroy = drm_output_destroy;
	output->base.disable = drm_output_disable;
	output->base.attach_head = drm_output_attach_head;
	output->base.detach_head = drm_output_detach_head;

	output->destroy_pending = false;
	output->disable_pending = false;

	output->state_cur = drm_output_state_alloc(output, NULL);

	weston_compositor_add_pending_output(&output->base, b->compositor);

	return &output->base;
}

static int
drm_backend_create_heads(struct drm_backend *b, struct udev_device *drm_device)
{
	struct drm_head *head;
	drmModeRes *resources;
	int i;

	resources = drmModeGetResources(b->drm.fd);
	if (!resources) {
		weston_log("drmModeGetResources failed\n");
		return -1;
	}

	b->min_width  = resources->min_width;
	b->max_width  = resources->max_width;
	b->min_height = resources->min_height;
	b->max_height = resources->max_height;

	for (i = 0; i < resources->count_connectors; i++) {
		uint32_t connector_id = resources->connectors[i];

		head = drm_head_create(b, connector_id, drm_device);
		if (!head) {
			weston_log("DRM: failed to create head for connector %d.\n",
				   connector_id);
		}
	}

	drm_backend_update_unused_outputs(b, resources);

	drmModeFreeResources(resources);

	return 0;
}

static void
drm_backend_update_heads(struct drm_backend *b, struct udev_device *drm_device)
{
	drmModeRes *resources;
	struct weston_head *base, *next;
	struct drm_head *head;
	int i;

	resources = drmModeGetResources(b->drm.fd);
	if (!resources) {
		weston_log("drmModeGetResources failed\n");
		return;
	}

	/* collect new connectors that have appeared, e.g. MST */
	for (i = 0; i < resources->count_connectors; i++) {
		uint32_t connector_id = resources->connectors[i];

		head = drm_head_find_by_connector(b, connector_id);
		if (head) {
			drm_head_update_info(head);
		} else {
			head = drm_head_create(b, connector_id, drm_device);
			if (!head)
				weston_log("DRM: failed to create head for hot-added connector %d.\n",
					   connector_id);
		}
	}

	/* Remove connectors that have disappeared. */
	wl_list_for_each_safe(base, next,
			      &b->compositor->head_list, compositor_link) {
		bool removed = true;

		head = to_drm_head(base);

		for (i = 0; i < resources->count_connectors; i++) {
			if (resources->connectors[i] == head->connector_id) {
				removed = false;
				break;
			}
		}

		if (!removed)
			continue;

		weston_log("DRM: head '%s' (connector %d) disappeared.\n",
			   head->base.name, head->connector_id);
		drm_head_destroy(head);
	}

	drm_backend_update_unused_outputs(b, resources);

	drmModeFreeResources(resources);
}

static enum wdrm_connector_property
drm_head_find_property_by_id(struct drm_head *head, uint32_t property_id)
{
	int i;
	enum wdrm_connector_property prop = WDRM_CONNECTOR__COUNT;

	if (!head || !property_id)
		return WDRM_CONNECTOR__COUNT;

	for (i = 0; i < WDRM_CONNECTOR__COUNT; i++)
		if (head->props_conn[i].prop_id == property_id) {
			prop = (enum wdrm_connector_property) i;
			break;
		}
	return prop;
}

static void
drm_backend_update_conn_props(struct drm_backend *b,
			      uint32_t	connector_id,
			      uint32_t property_id)
{
	struct drm_head *head;
	enum wdrm_connector_property conn_prop;
	drmModeObjectProperties *props;

	head = drm_head_find_by_connector(b, connector_id);
	if (!head) {
		weston_log("DRM: failed to find head for connector id: %d.\n",
			   connector_id);
		return;
	}

	conn_prop = drm_head_find_property_by_id(head, property_id);
	if (conn_prop >= WDRM_CONNECTOR__COUNT)
		return;

	props = drmModeObjectGetProperties(b->drm.fd,
					   connector_id,
					   DRM_MODE_OBJECT_CONNECTOR);
	if (!props) {
		weston_log("Error: failed to get connector '%s' properties\n",
			   head->base.name);
		return;
	}
	if (conn_prop == WDRM_CONNECTOR_CONTENT_PROTECTION) {
		weston_head_set_content_protection_status(&head->base,
					     drm_head_get_current_protection(head, props));
	}
	drmModeFreeObjectProperties(props);
}

static int
udev_event_is_hotplug(struct drm_backend *b, struct udev_device *device)
{
	const char *sysnum;
	const char *val;

	sysnum = udev_device_get_sysnum(device);
	if (!sysnum || atoi(sysnum) != b->drm.id)
		return 0;

	val = udev_device_get_property_value(device, "HOTPLUG");
	if (!val)
		return 0;

	return strcmp(val, "1") == 0;
}

static int
udev_event_is_conn_prop_change(struct drm_backend *b,
			       struct udev_device *device,
			       uint32_t *connector_id,
			       uint32_t *property_id)

{
	const char *val;
	int id;

	val = udev_device_get_property_value(device, "CONNECTOR");
	if (!val || !safe_strtoint(val, &id))
		return 0;
	else
		*connector_id = id;

	val = udev_device_get_property_value(device, "PROPERTY");
	if (!val || !safe_strtoint(val, &id))
		return 0;
	else
		*property_id = id;

	return 1;
}

static int
udev_drm_event(int fd, uint32_t mask, void *data)
{
	struct drm_backend *b = data;
	struct udev_device *event;
	uint32_t conn_id, prop_id;

	event = udev_monitor_receive_device(b->udev_monitor);

	if (udev_event_is_hotplug(b, event)) {
		if (udev_event_is_conn_prop_change(b, event, &conn_id, &prop_id))
			drm_backend_update_conn_props(b, conn_id, prop_id);
		else
			drm_backend_update_heads(b, event);
	}

	udev_device_unref(event);

	return 1;
}

static void
drm_destroy(struct weston_compositor *ec)
{
	struct drm_backend *b = to_drm_backend(ec);
	struct weston_head *base, *next;

	udev_input_destroy(&b->input);

	wl_event_source_remove(b->udev_drm_source);
	wl_event_source_remove(b->drm_source);

	b->shutting_down = true;

	destroy_sprites(b);

	weston_log_scope_destroy(b->debug);
	b->debug = NULL;
	weston_compositor_shutdown(ec);

	wl_list_for_each_safe(base, next, &ec->head_list, compositor_link)
		drm_head_destroy(to_drm_head(base));

#ifdef BUILD_DRM_GBM
	if (b->gbm)
		gbm_device_destroy(b->gbm);
#endif

	udev_monitor_unref(b->udev_monitor);
	udev_unref(b->udev);

	weston_launcher_destroy(ec->launcher);

	wl_array_release(&b->unused_crtcs);

	close(b->drm.fd);
	free(b->drm.filename);
	free(b);
}

static void
session_notify(struct wl_listener *listener, void *data)
{
	struct weston_compositor *compositor = data;
	struct drm_backend *b = to_drm_backend(compositor);
	struct drm_plane *plane;
	struct drm_output *output;

	if (compositor->session_active) {
		weston_log("activating session\n");
		weston_compositor_wake(compositor);
		weston_compositor_damage_all(compositor);
		b->state_invalid = true;
		udev_input_enable(&b->input);
	} else {
		weston_log("deactivating session\n");
		udev_input_disable(&b->input);

		weston_compositor_offscreen(compositor);

		/* If we have a repaint scheduled (either from a
		 * pending pageflip or the idle handler), make sure we
		 * cancel that so we don't try to pageflip when we're
		 * vt switched away.  The OFFSCREEN state will prevent
		 * further attempts at repainting.  When we switch
		 * back, we schedule a repaint, which will process
		 * pending frame callbacks. */

		wl_list_for_each(output, &compositor->output_list, base.link) {
			output->base.repaint_needed = false;
			if (output->cursor_plane)
				drmModeSetCursor(b->drm.fd, output->crtc_id,
						 0, 0, 0);
		}

		output = container_of(compositor->output_list.next,
				      struct drm_output, base.link);

		wl_list_for_each(plane, &b->plane_list, link) {
			if (plane->type != WDRM_PLANE_TYPE_OVERLAY)
				continue;

			drmModeSetPlane(b->drm.fd,
					plane->plane_id,
					output->crtc_id, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0);
		}
	}
}


/**
 * Handle KMS GPU being added/removed
 *
 * If the device being added/removed is the KMS device, we activate/deactivate
 * the compositor session.
 *
 * @param compositor The compositor instance.
 * @param device The device being added/removed.
 * @param added Whether the device is being added (or removed)
 */
static void
drm_device_changed(struct weston_compositor *compositor,
		dev_t device, bool added)
{
	struct drm_backend *b = to_drm_backend(compositor);

	if (b->drm.fd < 0 || b->drm.devnum != device ||
	    compositor->session_active == added)
		return;

	compositor->session_active = added;
	wl_signal_emit(&compositor->session_signal, compositor);
}

/**
 * Determines whether or not a device is capable of modesetting. If successful,
 * sets b->drm.fd and b->drm.filename to the opened device.
 */
static bool
drm_device_is_kms(struct drm_backend *b, struct udev_device *device)
{
	const char *filename = udev_device_get_devnode(device);
	const char *sysnum = udev_device_get_sysnum(device);
	dev_t devnum = udev_device_get_devnum(device);
	drmModeRes *res;
	int id = -1, fd;

	if (!filename)
		return false;

	fd = weston_launcher_open(b->compositor->launcher, filename, O_RDWR);
	if (fd < 0)
		return false;

	res = drmModeGetResources(fd);
	if (!res)
		goto out_fd;

	if (res->count_crtcs <= 0 || res->count_connectors <= 0 ||
	    res->count_encoders <= 0)
		goto out_res;

	if (sysnum)
		id = atoi(sysnum);
	if (!sysnum || id < 0) {
		weston_log("couldn't get sysnum for device %s\n", filename);
		goto out_res;
	}

	/* We can be called successfully on multiple devices; if we have,
	 * clean up old entries. */
	if (b->drm.fd >= 0)
		weston_launcher_close(b->compositor->launcher, b->drm.fd);
	free(b->drm.filename);

	b->drm.fd = fd;
	b->drm.id = id;
	b->drm.filename = strdup(filename);
	b->drm.devnum = devnum;

	drmModeFreeResources(res);

	return true;

out_res:
	drmModeFreeResources(res);
out_fd:
	weston_launcher_close(b->compositor->launcher, fd);
	return false;
}

/*
 * Find primary GPU
 * Some systems may have multiple DRM devices attached to a single seat. This
 * function loops over all devices and tries to find a PCI device with the
 * boot_vga sysfs attribute set to 1.
 * If no such device is found, the first DRM device reported by udev is used.
 * Devices are also vetted to make sure they are are capable of modesetting,
 * rather than pure render nodes (GPU with no display), or pure
 * memory-allocation devices (VGEM).
 */
static struct udev_device*
find_primary_gpu(struct drm_backend *b, const char *seat)
{
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	const char *path, *device_seat, *id;
	struct udev_device *device, *drm_device, *pci;

	e = udev_enumerate_new(b->udev);
	udev_enumerate_add_match_subsystem(e, "drm");
	udev_enumerate_add_match_sysname(e, "card[0-9]*");

	udev_enumerate_scan_devices(e);
	drm_device = NULL;
	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		bool is_boot_vga = false;

		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(b->udev, path);
		if (!device)
			continue;
		device_seat = udev_device_get_property_value(device, "ID_SEAT");
		if (!device_seat)
			device_seat = default_seat;
		if (strcmp(device_seat, seat)) {
			udev_device_unref(device);
			continue;
		}

		pci = udev_device_get_parent_with_subsystem_devtype(device,
								"pci", NULL);
		if (pci) {
			id = udev_device_get_sysattr_value(pci, "boot_vga");
			if (id && !strcmp(id, "1"))
				is_boot_vga = true;
		}

		/* If we already have a modesetting-capable device, and this
		 * device isn't our boot-VGA device, we aren't going to use
		 * it. */
		if (!is_boot_vga && drm_device) {
			udev_device_unref(device);
			continue;
		}

		/* Make sure this device is actually capable of modesetting;
		 * if this call succeeds, b->drm.{fd,filename} will be set,
		 * and any old values freed. */
		if (!drm_device_is_kms(b, device)) {
			udev_device_unref(device);
			continue;
		}

		/* There can only be one boot_vga device, and we try to use it
		 * at all costs. */
		if (is_boot_vga) {
			if (drm_device)
				udev_device_unref(drm_device);
			drm_device = device;
			break;
		}

		/* Per the (!is_boot_vga && drm_device) test above, we only
		 * trump existing saved devices with boot-VGA devices, so if
		 * we end up here, this must be the first device we've seen. */
		assert(!drm_device);
		drm_device = device;
	}

	/* If we're returning a device to use, we must have an open FD for
	 * it. */
	assert(!!drm_device == (b->drm.fd >= 0));

	udev_enumerate_unref(e);
	return drm_device;
}

static struct udev_device *
open_specific_drm_device(struct drm_backend *b, const char *name)
{
	struct udev_device *device;

	device = udev_device_new_from_subsystem_sysname(b->udev, "drm", name);
	if (!device) {
		weston_log("ERROR: could not open DRM device '%s'\n", name);
		return NULL;
	}

	if (!drm_device_is_kms(b, device)) {
		udev_device_unref(device);
		weston_log("ERROR: DRM device '%s' is not a KMS device.\n", name);
		return NULL;
	}

	/* If we're returning a device to use, we must have an open FD for
	 * it. */
	assert(b->drm.fd >= 0);

	return device;
}

static void
planes_binding(struct weston_keyboard *keyboard, const struct timespec *time,
	       uint32_t key, void *data)
{
	struct drm_backend *b = data;

	switch (key) {
	case KEY_C:
		b->cursors_are_broken ^= true;
		break;
	case KEY_V:
		/* We don't support overlay-plane usage with legacy KMS. */
		if (b->atomic_modeset)
			b->sprites_are_broken ^= true;
		break;
	default:
		break;
	}
}

#ifdef BUILD_VAAPI_RECORDER
static void
recorder_destroy(struct drm_output *output)
{
	vaapi_recorder_destroy(output->recorder);
	output->recorder = NULL;

	weston_output_disable_planes_decr(&output->base);

	wl_list_remove(&output->recorder_frame_listener.link);
	weston_log("[libva recorder] done\n");
}

static void
recorder_frame_notify(struct wl_listener *listener, void *data)
{
	struct drm_output *output;
	struct drm_backend *b;
	int fd, ret;

	output = container_of(listener, struct drm_output,
			      recorder_frame_listener);
	b = to_drm_backend(output->base.compositor);

	if (!output->recorder)
		return;

	ret = drmPrimeHandleToFD(b->drm.fd,
				 output->scanout_plane->state_cur->fb->handles[0],
				 DRM_CLOEXEC, &fd);
	if (ret) {
		weston_log("[libva recorder] "
			   "failed to create prime fd for front buffer\n");
		return;
	}

	ret = vaapi_recorder_frame(output->recorder, fd,
				   output->scanout_plane->state_cur->fb->strides[0]);
	if (ret < 0) {
		weston_log("[libva recorder] aborted: %s\n", strerror(errno));
		recorder_destroy(output);
	}
}

static void *
create_recorder(struct drm_backend *b, int width, int height,
		const char *filename)
{
	int fd;
	drm_magic_t magic;

	fd = open(b->drm.filename, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return NULL;

	drmGetMagic(fd, &magic);
	drmAuthMagic(b->drm.fd, magic);

	return vaapi_recorder_create(fd, width, height, filename);
}

static void
recorder_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t key, void *data)
{
	struct drm_backend *b = data;
	struct drm_output *output;
	int width, height;

	output = container_of(b->compositor->output_list.next,
			      struct drm_output, base.link);

	if (!output->recorder) {
		if (output->gbm_format != DRM_FORMAT_XRGB8888) {
			weston_log("failed to start vaapi recorder: "
				   "output format not supported\n");
			return;
		}

		width = output->base.current_mode->width;
		height = output->base.current_mode->height;

		output->recorder =
			create_recorder(b, width, height, "capture.h264");
		if (!output->recorder) {
			weston_log("failed to create vaapi recorder\n");
			return;
		}

		weston_output_disable_planes_incr(&output->base);

		output->recorder_frame_listener.notify = recorder_frame_notify;
		wl_signal_add(&output->base.frame_signal,
			      &output->recorder_frame_listener);

		weston_output_schedule_repaint(&output->base);

		weston_log("[libva recorder] initialized\n");
	} else {
		recorder_destroy(output);
	}
}
#else
static void
recorder_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t key, void *data)
{
	weston_log("Compiled without libva support\n");
}
#endif


static const struct weston_drm_output_api api = {
	drm_output_set_mode,
	drm_output_set_gbm_format,
	drm_output_set_seat,
};

static struct drm_backend *
drm_backend_create(struct weston_compositor *compositor,
		   struct weston_drm_backend_config *config)
{
	struct drm_backend *b;
	struct udev_device *drm_device;
	struct wl_event_loop *loop;
	const char *seat_id = default_seat;
	const char *session_seat;
	int ret;

	session_seat = getenv("XDG_SEAT");
	if (session_seat)
		seat_id = session_seat;

	if (config->seat_id)
		seat_id = config->seat_id;

	weston_log("initializing drm backend\n");

	b = zalloc(sizeof *b);
	if (b == NULL)
		return NULL;

	b->state_invalid = true;
	b->drm.fd = -1;
	wl_array_init(&b->unused_crtcs);

	b->compositor = compositor;
	b->use_pixman = config->use_pixman;
	b->pageflip_timeout = config->pageflip_timeout;
	b->use_pixman_shadow = config->use_pixman_shadow;

	b->debug = weston_compositor_add_log_scope(compositor, "drm-backend",
						   "Debug messages from DRM/KMS backend\n",
						   NULL, NULL, NULL);

	compositor->backend = &b->base;
	compositor->require_input = !config->continue_without_input;

	if (parse_gbm_format(config->gbm_format, DRM_FORMAT_XRGB8888, &b->gbm_format) < 0)
		goto err_compositor;

	/* Check if we run drm-backend using weston-launch */
	compositor->launcher = weston_launcher_connect(compositor, config->tty,
						       seat_id, true);
	if (compositor->launcher == NULL) {
		weston_log("fatal: drm backend should be run using "
			   "weston-launch binary, or your system should "
			   "provide the logind D-Bus API.\n");
		goto err_compositor;
	}

	b->udev = udev_new();
	if (b->udev == NULL) {
		weston_log("failed to initialize udev context\n");
		goto err_launcher;
	}

	b->session_listener.notify = session_notify;
	wl_signal_add(&compositor->session_signal, &b->session_listener);

	if (config->specific_device)
		drm_device = open_specific_drm_device(b, config->specific_device);
	else
		drm_device = find_primary_gpu(b, seat_id);
	if (drm_device == NULL) {
		weston_log("no drm device found\n");
		goto err_udev;
	}

	if (init_kms_caps(b) < 0) {
		weston_log("failed to initialize kms\n");
		goto err_udev_dev;
	}

	if (b->use_pixman) {
		if (init_pixman(b) < 0) {
			weston_log("failed to initialize pixman renderer\n");
			goto err_udev_dev;
		}
	} else {
		if (init_egl(b) < 0) {
			weston_log("failed to initialize egl\n");
			goto err_udev_dev;
		}
	}

	b->base.destroy = drm_destroy;
	b->base.repaint_begin = drm_repaint_begin;
	b->base.repaint_flush = drm_repaint_flush;
	b->base.repaint_cancel = drm_repaint_cancel;
	b->base.create_output = drm_output_create;
	b->base.device_changed = drm_device_changed;
	b->base.can_scanout_dmabuf = drm_can_scanout_dmabuf;

	weston_setup_vt_switch_bindings(compositor);

	wl_list_init(&b->plane_list);
	create_sprites(b);

	if (udev_input_init(&b->input,
			    compositor, b->udev, seat_id,
			    config->configure_device) < 0) {
		weston_log("failed to create input devices\n");
		goto err_sprite;
	}

	if (drm_backend_create_heads(b, drm_device) < 0) {
		weston_log("Failed to create heads for %s\n", b->drm.filename);
		goto err_udev_input;
	}

	/* 'compute' faked zpos values in case HW doesn't expose any */
	drm_backend_create_faked_zpos(b);

	/* A this point we have some idea of whether or not we have a working
	 * cursor plane. */
	if (!b->cursors_are_broken)
		compositor->capabilities |= WESTON_CAP_CURSOR_PLANE;

	loop = wl_display_get_event_loop(compositor->wl_display);
	b->drm_source =
		wl_event_loop_add_fd(loop, b->drm.fd,
				     WL_EVENT_READABLE, on_drm_input, b);

	b->udev_monitor = udev_monitor_new_from_netlink(b->udev, "udev");
	if (b->udev_monitor == NULL) {
		weston_log("failed to initialize udev monitor\n");
		goto err_drm_source;
	}
	udev_monitor_filter_add_match_subsystem_devtype(b->udev_monitor,
							"drm", NULL);
	b->udev_drm_source =
		wl_event_loop_add_fd(loop,
				     udev_monitor_get_fd(b->udev_monitor),
				     WL_EVENT_READABLE, udev_drm_event, b);

	if (udev_monitor_enable_receiving(b->udev_monitor) < 0) {
		weston_log("failed to enable udev-monitor receiving\n");
		goto err_udev_monitor;
	}

	udev_device_unref(drm_device);

	weston_compositor_add_debug_binding(compositor, KEY_O,
					    planes_binding, b);
	weston_compositor_add_debug_binding(compositor, KEY_C,
					    planes_binding, b);
	weston_compositor_add_debug_binding(compositor, KEY_V,
					    planes_binding, b);
	weston_compositor_add_debug_binding(compositor, KEY_Q,
					    recorder_binding, b);
	weston_compositor_add_debug_binding(compositor, KEY_W,
					    renderer_switch_binding, b);

	if (compositor->renderer->import_dmabuf) {
		if (linux_dmabuf_setup(compositor) < 0)
			weston_log("Error: initializing dmabuf "
				   "support failed.\n");
		if (weston_direct_display_setup(compositor) < 0)
			weston_log("Error: initializing direct-display "
				   "support failed.\n");
	}

	if (compositor->capabilities & WESTON_CAP_EXPLICIT_SYNC) {
		if (linux_explicit_synchronization_setup(compositor) < 0)
			weston_log("Error: initializing explicit "
				   " synchronization support failed.\n");
	}

	if (b->atomic_modeset)
		if (weston_compositor_enable_content_protection(compositor) < 0)
			weston_log("Error: initializing content-protection "
				   "support failed.\n");

	ret = weston_plugin_api_register(compositor, WESTON_DRM_OUTPUT_API_NAME,
					 &api, sizeof(api));

	if (ret < 0) {
		weston_log("Failed to register output API.\n");
		goto err_udev_monitor;
	}

	ret = drm_backend_init_virtual_output_api(compositor);
	if (ret < 0) {
		weston_log("Failed to register virtual output API.\n");
		goto err_udev_monitor;
	}

	return b;

err_udev_monitor:
	wl_event_source_remove(b->udev_drm_source);
	udev_monitor_unref(b->udev_monitor);
err_drm_source:
	wl_event_source_remove(b->drm_source);
err_udev_input:
	udev_input_destroy(&b->input);
err_sprite:
#ifdef BUILD_DRM_GBM
	if (b->gbm)
		gbm_device_destroy(b->gbm);
#endif
	destroy_sprites(b);
err_udev_dev:
	udev_device_unref(drm_device);
err_udev:
	udev_unref(b->udev);
err_launcher:
	weston_launcher_destroy(compositor->launcher);
err_compositor:
	weston_compositor_shutdown(compositor);
	free(b);
	return NULL;
}

static void
config_init_to_defaults(struct weston_drm_backend_config *config)
{
	config->use_pixman_shadow = true;
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct drm_backend *b;
	struct weston_drm_backend_config config = {{ 0, }};

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_DRM_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_drm_backend_config)) {
		weston_log("drm backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	b = drm_backend_create(compositor, &config);
	if (b == NULL)
		return -1;

	return 0;
}
