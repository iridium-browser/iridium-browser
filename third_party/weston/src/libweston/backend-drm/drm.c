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
#include <poll.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <libudev.h>

#include <libweston/libweston.h>
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>
#include "drm-internal.h"
#include "shared/hash.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include "shared/string-helpers.h"
#include "shared/weston-drm-fourcc.h"
#include "output-capture.h"
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
drm_backend_create_faked_zpos(struct drm_device *device)
{
	struct drm_backend *b = device->backend;
	struct drm_plane *plane;
	uint64_t zpos = 0ULL;
	uint64_t zpos_min_primary;
	uint64_t zpos_min_overlay;
	uint64_t zpos_min_cursor;

	zpos_min_primary = zpos;
	wl_list_for_each(plane, &device->plane_list, link) {
		/* if the property is there, bail out sooner */
		if (plane->props[WDRM_PLANE_ZPOS].prop_id != 0)
			return;

		if (plane->type != WDRM_PLANE_TYPE_PRIMARY)
			continue;
		zpos++;
	}

	zpos_min_overlay = zpos;
	wl_list_for_each(plane, &device->plane_list, link) {
		if (plane->type != WDRM_PLANE_TYPE_OVERLAY)
			continue;
		zpos++;
	}

	zpos_min_cursor = zpos;
	wl_list_for_each(plane, &device->plane_list, link) {
		if (plane->type != WDRM_PLANE_TYPE_CURSOR)
			continue;
		zpos++;
	}

	drm_debug(b, "[drm-backend] zpos property not found. "
		     "Using invented immutable zpos values:\n");
	/* assume that invented zpos values are immutable */
	wl_list_for_each(plane, &device->plane_list, link) {
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
	return !!(plane->possible_crtcs & (1 << output->crtc->pipe));
}

struct drm_crtc *
drm_crtc_find(struct drm_device *device, uint32_t crtc_id)
{
	struct drm_crtc *crtc;

	wl_list_for_each(crtc, &device->crtc_list, link) {
		if (crtc->crtc_id == crtc_id)
			return crtc;
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
		if (!head)
			continue;
		if (head->connector.connector_id == connector_id)
			return head;
	}

	return NULL;
}

static struct drm_writeback *
drm_writeback_find_by_connector(struct drm_backend *backend, uint32_t connector_id)
{
	struct drm_writeback *writeback;

	wl_list_for_each(writeback, &backend->drm->writeback_connector_list, link) {
		if (writeback->connector.connector_id == connector_id)
			return writeback;
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

static int
drm_output_apply_mode(struct drm_output *output);

/**
 * Mark a drm_output_state (the output's last state) as complete. This handles
 * any post-completion actions such as updating the repaint timer, disabling the
 * output, and finally freeing the state.
 */
void
drm_output_update_complete(struct drm_output *output, uint32_t flags,
			   unsigned int sec, unsigned int usec)
{
	struct drm_device *device = output->device;
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
		output->mode_switch_pending = false;
		drm_output_destroy(&output->base);
		return;
	} else if (output->disable_pending) {
		output->disable_pending = false;
		output->dpms_off_pending = false;
		output->mode_switch_pending = false;
		weston_output_disable(&output->base);
		return;
	} else if (output->dpms_off_pending) {
		struct drm_pending_state *pending = drm_pending_state_alloc(device);
		output->dpms_off_pending = false;
		output->mode_switch_pending = false;
		drm_output_get_disable_state(pending, output);
		drm_pending_state_apply_sync(pending);
	} else if (output->mode_switch_pending) {
		output->mode_switch_pending = false;
		drm_output_apply_mode(output);
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

	if (output->state_cur->tear)
		flags |= WESTON_FINISH_FRAME_TEARING;

	ts.tv_sec = sec;
	ts.tv_nsec = usec * 1000;

	if (output->state_cur->dpms != WESTON_DPMS_OFF)
		weston_output_finish_frame(&output->base, &ts, flags);
	else
		weston_output_finish_frame(&output->base, NULL,
					   WP_PRESENTATION_FEEDBACK_INVALID);

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

	ec->renderer->repaint_output(&output->base, damage,
				     output->renderbuffer[output->current_image]);

	return drm_fb_ref(output->dumb[output->current_image]);
}

void
drm_output_render(struct drm_output_state *state, pixman_region32_t *damage)
{
	struct drm_output *output = state->output;
	struct drm_device *device = output->device;
	struct weston_compositor *c = output->base.compositor;
	struct drm_plane_state *scanout_state;
	struct drm_plane *scanout_plane = output->scanout_plane;
	struct drm_property_info *damage_info =
		&scanout_plane->props[WDRM_PLANE_FB_DAMAGE_CLIPS];
	struct drm_fb *fb;
	pixman_region32_t scanout_damage;
	pixman_box32_t *rects;
	int n_rects;

	/* If we already have a client buffer promoted to scanout, then we don't
	 * want to render. */
	scanout_state = drm_output_state_get_plane(state, scanout_plane);
	if (scanout_state->fb)
		return;

	/*
	 * If we don't have any damage on the primary plane, and we already
	 * have a renderer buffer active, we can reuse it; else we pass
	 * the damaged region into the renderer to re-render the affected
	 * area. But, we still have to call the renderer anyway if any screen
	 * capture is pending, otherwise the capture will not complete.
	 */
	if (!pixman_region32_not_empty(damage) &&
	    wl_list_empty(&output->base.frame_signal.listener_list) &&
	    !weston_output_has_renderer_capture_tasks(&output->base) &&
	    scanout_plane->state_cur->fb &&
	    (scanout_plane->state_cur->fb->type == BUFFER_GBM_SURFACE ||
	     scanout_plane->state_cur->fb->type == BUFFER_PIXMAN_DUMB)) {
		fb = drm_fb_ref(scanout_plane->state_cur->fb);
	} else if (c->renderer->type == WESTON_RENDERER_PIXMAN) {
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

	scanout_state->zpos = scanout_plane->zpos_min;

	pixman_region32_subtract(&c->primary_plane.damage,
				 &c->primary_plane.damage, damage);

	/* Don't bother calculating plane damage if the plane doesn't support it */
	if (damage_info->prop_id == 0)
		return;

	pixman_region32_init(&scanout_damage);

	weston_region_global_to_output(&scanout_damage,
				       &output->base,
				       damage);

	assert(scanout_state->damage_blob_id == 0);

	rects = pixman_region32_rectangles(&scanout_damage, &n_rects);

	/*
	 * If this function fails, the blob id should still be 0.
	 * This tells the kernel there is no damage information, which means
	 * that it will consider the whole plane damaged. While this may
	 * affect efficiency, it should still produce correct results.
	 */
	drmModeCreatePropertyBlob(device->drm.fd, rects,
				  sizeof(*rects) * n_rects,
				  &scanout_state->damage_blob_id);

	pixman_region32_fini(&scanout_damage);
}

static uint32_t
drm_connector_get_possible_crtcs_mask(struct drm_connector *connector)
{
	struct drm_device *device = connector->device;
	uint32_t possible_crtcs = 0;
	drmModeConnector *conn = connector->conn;
	drmModeEncoder *encoder;
	int i;

	for (i = 0; i < conn->count_encoders; i++) {
		encoder = drmModeGetEncoder(device->drm.fd,
					    conn->encoders[i]);
		if (!encoder)
			continue;

		possible_crtcs |= encoder->possible_crtcs;
		drmModeFreeEncoder(encoder);
	}

	return possible_crtcs;
}

static struct drm_writeback *
drm_output_find_compatible_writeback(struct drm_output *output)
{
	struct drm_crtc *crtc;
	struct drm_writeback *wb;
	bool in_use;
	uint32_t possible_crtcs;

	wl_list_for_each(wb, &output->device->writeback_connector_list, link) {
		/* Another output may be using the writeback connector. */
		in_use = false;
		wl_list_for_each(crtc, &output->device->crtc_list, link) {
			if (crtc->output && crtc->output->wb_state &&
			    crtc->output->wb_state->wb == wb) {
				in_use = true;
				break;
			}
		}
		if (in_use)
			continue;

		/* Is the writeback connector compatible with the CRTC? */
		possible_crtcs =
			drm_connector_get_possible_crtcs_mask(&wb->connector);
		if (!(possible_crtcs & (1 << output->crtc->pipe)))
			continue;

		/* Does the writeback connector support the output gbm format? */
		if (!weston_drm_format_array_find_format(&wb->formats,
							 output->format->format))
			continue;

		return wb;
	}

	return NULL;
}

static struct drm_writeback_state *
drm_writeback_state_alloc(void)
{
	struct drm_writeback_state *state;

	state = zalloc(sizeof *state);
	if (!state)
		return NULL;

	state->state = DRM_OUTPUT_WB_SCREENSHOT_OFF;
	state->out_fence_fd = -1;
	wl_array_init(&state->referenced_fbs);

	return state;
}

static void
drm_writeback_state_free(struct drm_writeback_state *state)
{
	struct drm_fb **fb;

	if (state->out_fence_fd >= 0)
		close(state->out_fence_fd);

	/* Unref framebuffer that was given to save the content of the writeback */
	if (state->fb)
		drm_fb_unref(state->fb);

	/* Unref framebuffers that were in use in the same commit of the one with
	 * the writeback setup */
	wl_array_for_each(fb, &state->referenced_fbs)
		drm_fb_unref(*fb);
	wl_array_release(&state->referenced_fbs);

	free(state);
}

static void
drm_output_pick_writeback_capture_task(struct drm_output *output)
{
	struct weston_capture_task *ct;
	struct weston_buffer *buffer;
	struct drm_writeback *wb;
	const char *msg;
	int32_t width = output->base.current_mode->width;
	int32_t height = output->base.current_mode->height;
	uint32_t format = output->format->format;

	ct = weston_output_pull_capture_task(&output->base,
					     WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK,
					     width, height, pixel_format_get_info(format));
	if (!ct)
		return;

	assert(output->device->atomic_modeset);

	wb = drm_output_find_compatible_writeback(output);
	if (!wb) {
		msg = "drm: could not find writeback connector for output";
		goto err;
	}

	buffer = weston_capture_task_get_buffer(ct);
	assert(buffer->width == width);
	assert(buffer->height == height);
	assert(buffer->pixel_format->format == output->format->format);

	output->wb_state = drm_writeback_state_alloc();
	if (!output->wb_state) {
		msg = "drm: failed to allocate memory for writeback state";
		goto err;
	}

	output->wb_state->fb = drm_fb_create_dumb(output->device, width, height, format);
	if (!output->wb_state->fb) {
		msg = "drm: failed to create dumb buffer for writeback state";
		goto err_fb;
	}

	output->wb_state->output = output;
	output->wb_state->wb = wb;
	output->wb_state->state = DRM_OUTPUT_WB_SCREENSHOT_PREPARE_COMMIT;
	output->wb_state->ct = ct;

	return;

err_fb:
	drm_writeback_state_free(output->wb_state);
	output->wb_state = NULL;
err:
	weston_capture_task_retire_failed(ct, msg);
}

static int
drm_output_repaint(struct weston_output *output_base, pixman_region32_t *damage)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_output_state *state = NULL;
	struct drm_plane_state *scanout_state;
	struct drm_pending_state *pending_state;
	struct drm_device *device;

	assert(output);
	assert(!output->virtual);

	device = output->device;
	pending_state = device->repaint_data;

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

	if (drm_output_ensure_hdr_output_metadata_blob(output) < 0)
		goto err;

	drm_output_pick_writeback_capture_task(output);

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
drm_waitvblank_pipe(struct drm_crtc *crtc)
{
	if (crtc->pipe > 1)
		return (crtc->pipe << DRM_VBLANK_HIGH_CRTC_SHIFT) &
				DRM_VBLANK_HIGH_CRTC_MASK;
	else if (crtc->pipe > 0)
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
	struct drm_device *device = output->device;
	struct drm_backend *backend = device->backend;
	struct weston_compositor *compositor = backend->compositor;
	struct timespec ts, tnow;
	struct timespec vbl2now;
	int64_t refresh_nsec;
	uint32_t flags = WP_PRESENTATION_FEEDBACK_INVALID;
	int ret;
	drmVBlank vbl = {
		.request.type = DRM_VBLANK_RELATIVE,
		.request.sequence = 0,
		.request.signal = 0,
	};

	if (output->disable_pending || output->destroy_pending)
		return 0;

	if (!scanout_plane->state_cur->fb) {
		/* We can't page flip if there's no mode set */
		goto finish_frame;
	}

	/* Need to smash all state in from scratch; current timings might not
	 * be what we want, page flip might not work, etc.
	 */
	if (device->state_invalid)
		goto finish_frame;

	assert(scanout_plane->state_cur->output == output);

	/* If we're tearing, we've been generating timestamps from the
	 * presentation clock that don't line up with the msc timestamps,
	 * and could be more recent than the latest msc, which would cause
	 * an assert() later.
	 */
	if (output->state_cur->tear) {
		flags |= WESTON_FINISH_FRAME_TEARING;
		goto finish_frame;
	}

	/* Try to get current msc and timestamp via instant query */
	vbl.request.type |= drm_waitvblank_pipe(output->crtc);
	ret = drmWaitVBlank(device->drm.fd, &vbl);

	/* Error ret or zero timestamp means failure to get valid timestamp */
	if ((ret == 0) && (vbl.reply.tval_sec > 0 || vbl.reply.tval_usec > 0)) {
		ts.tv_sec = vbl.reply.tval_sec;
		ts.tv_nsec = vbl.reply.tval_usec * 1000;

		/* Valid timestamp for most recent vblank - not stale?
		 * Stale ts could happen on Linux 3.17+, so make sure it
		 * is not older than 1 refresh duration since now.
		 */
		weston_compositor_read_presentation_clock(compositor,
							  &tnow);
		timespec_sub(&vbl2now, &tnow, &ts);
		refresh_nsec =
			millihz_to_nsec(output->base.current_mode->refresh);
		if (timespec_to_nsec(&vbl2now) < refresh_nsec) {
			drm_output_update_msc(output, vbl.reply.sequence);
			weston_output_finish_frame(output_base, &ts, flags);
			return 0;
		}
	}

	/* Immediate query didn't provide valid timestamp.
	 * Use pageflip fallback.
	 */

	assert(!output->page_flip_pending);
	assert(!output->state_last);

	pending_state = drm_pending_state_alloc(device);
	drm_output_state_duplicate(output->state_cur, pending_state,
				   DRM_OUTPUT_STATE_PRESERVE_PLANES);

	ret = drm_pending_state_apply(pending_state);
	if (ret != 0) {
		weston_log("applying repaint-start state failed: %s\n",
			   strerror(errno));
		if (ret == -EACCES || ret == -EBUSY)
			return ret;
		goto finish_frame;
	}

	return 0;

finish_frame:
	/* if we cannot page-flip, immediately finish frame */
	weston_output_finish_frame(output_base, NULL, flags);
	return 0;
}

/**
 * Begin a new repaint cycle
 *
 * Called by the core compositor at the beginning of a repaint cycle. Creates
 * a new pending_state structure to own any output state created by individual
 * output repaint functions until the repaint is flushed or cancelled.
 */
static void
drm_repaint_begin(struct weston_backend *backend)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct drm_device *device;
	struct drm_pending_state *pending_state;

	device = b->drm;
	pending_state = drm_pending_state_alloc(device);
	device->repaint_data = pending_state;

	if (weston_log_scope_is_enabled(b->debug)) {
		char *dbg = weston_compositor_print_scene_graph(b->compositor);
		drm_debug(b, "[repaint] Beginning repaint; pending_state %p\n",
			  device->repaint_data);
		drm_debug(b, "%s", dbg);
		free(dbg);
	}

	wl_list_for_each(device, &b->kms_list, link) {
		pending_state = drm_pending_state_alloc(device);
		device->repaint_data = pending_state;

		if (weston_log_scope_is_enabled(b->debug)) {
			char *dbg = weston_compositor_print_scene_graph(b->compositor);
			drm_debug(b, "[repaint] Beginning repaint; pending_state %p\n",
				  pending_state);
			drm_debug(b, "%s", dbg);
			free(dbg);
		}
	}
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
drm_repaint_flush(struct weston_backend *backend)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct drm_device *device;
	struct drm_pending_state *pending_state;
	int ret;

	device = b->drm;
	pending_state = device->repaint_data;
	ret = drm_pending_state_apply(pending_state);
	if (ret != 0)
		weston_log("repaint-flush failed: %s\n", strerror(errno));

	drm_debug(b, "[repaint] flushed pending_state %p\n", pending_state);
	device->repaint_data = NULL;

	wl_list_for_each(device, &b->kms_list, link) {
		pending_state = device->repaint_data;
		ret = drm_pending_state_apply(pending_state);
		if (ret != 0)
			weston_log("repaint-flush failed: %s\n", strerror(errno));

		drm_debug(b, "[repaint] flushed pending_state %p\n", pending_state);
		device->repaint_data = NULL;
	}

	return (ret == -EACCES || ret == -EBUSY) ? ret : 0;
}

/**
 * Cancel a repaint set
 *
 * Called by the core compositor when a repaint has finished, so the data
 * held across the repaint cycle should be discarded.
 */
static void
drm_repaint_cancel(struct weston_backend *backend)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct drm_device *device;
	struct drm_pending_state *pending_state;

	device = b->drm;
	pending_state = device->repaint_data;
	drm_pending_state_free(pending_state);
	drm_debug(b, "[repaint] cancel pending_state %p\n", pending_state);
	device->repaint_data = NULL;

	wl_list_for_each(device, &b->kms_list, link) {
		pending_state = device->repaint_data;
		drm_pending_state_free(pending_state);
		drm_debug(b, "[repaint] cancel pending_state %p\n", pending_state);
		device->repaint_data = NULL;
	}
}

static int
drm_output_init_pixman(struct drm_output *output, struct drm_backend *b);
static void
drm_output_fini_pixman(struct drm_output *output);

static int
drm_output_switch_mode(struct weston_output *output_base, struct weston_mode *mode)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_mode *drm_mode;

	assert(output);

	drm_mode = drm_output_choose_mode(output, mode);
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

	if (output->page_flip_pending || output->atomic_complete_pending) {
		output->mode_switch_pending = true;
		return 0;
	}

	return drm_output_apply_mode(output);
}

static int
drm_output_apply_mode(struct drm_output *output)
{
	struct drm_device *device = output->device;
	struct drm_backend *b = device->backend;

	/* XXX: This drops our current buffer too early, before we've started
	 *      displaying it. Ideally this should be much more atomic and
	 *      integrated with a full repaint cycle, rather than doing a
	 *      sledgehammer modeswitch first, and only later showing new
	 *      content.
	 */
	device->state_invalid = true;

	if (b->compositor->renderer->type == WESTON_RENDERER_PIXMAN) {
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

	if (device->atomic_modeset && !output->base.disable_planes)
		weston_output_update_capture_info(&output->base,
						  WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK,
						  output->base.current_mode->width,
						  output->base.current_mode->height,
						  pixel_format_get_info(output->format->format));

	return 0;
}

static int
init_pixman(struct drm_backend *b)
{
	return weston_compositor_init_renderer(b->compositor,
					       WESTON_RENDERER_PIXMAN, NULL);
}

/**
 * Create a drm_plane for a hardware plane
 *
 * Creates one drm_plane structure for a hardware plane, and initialises its
 * properties and formats.
 *
 * This function does not add the plane to the list of usable planes in Weston
 * itself; the caller is responsible for this.
 *
 * Call drm_plane_destroy to clean up the plane.
 *
 * @sa drm_output_find_special_plane
 * @param device DRM device
 * @param kplane DRM plane to create
 */
static struct drm_plane *
drm_plane_create(struct drm_device *device, const drmModePlane *kplane)
{
	struct drm_backend *b = device->backend;
	struct weston_compositor *compositor = b->compositor;
	struct drm_plane *plane, *tmp;
	drmModeObjectProperties *props;
	uint64_t *zpos_range_values;
	uint64_t *alpha_range_values;

	plane = zalloc(sizeof(*plane));
	if (!plane) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	plane->device = device;
	plane->state_cur = drm_plane_state_alloc(NULL, plane);
	plane->state_cur->complete = true;
	plane->possible_crtcs = kplane->possible_crtcs;
	plane->plane_id = kplane->plane_id;
	plane->crtc_id = kplane->crtc_id;

	weston_drm_format_array_init(&plane->formats);

	props = drmModeObjectGetProperties(device->drm.fd, kplane->plane_id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props) {
		weston_log("couldn't get plane properties\n");
		goto err;
	}

	drm_property_info_populate(device, plane_props, plane->props,
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

	alpha_range_values =
		drm_property_get_range_values(&plane->props[WDRM_PLANE_ALPHA],
					      props);

	if (alpha_range_values) {
		plane->alpha_min = (uint16_t) alpha_range_values[0];
		plane->alpha_max = (uint16_t) alpha_range_values[1];
	} else {
		plane->alpha_min = DRM_PLANE_ALPHA_OPAQUE;
		plane->alpha_max = DRM_PLANE_ALPHA_OPAQUE;
	}

	if (drm_plane_populate_formats(plane, kplane, props,
				       device->fb_modifiers) < 0) {
		drmModeFreeObjectProperties(props);
		goto err;
	}

	drmModeFreeObjectProperties(props);

	if (plane->type == WDRM_PLANE_TYPE__COUNT)
		goto err_props;

	weston_plane_init(&plane->base, compositor);

	wl_list_for_each(tmp, &device->plane_list, link) {
		if (tmp->zpos_max < plane->zpos_max) {
			wl_list_insert(tmp->link.prev, &plane->link);
			break;
		}
	}
	if (plane->link.next == NULL)
		wl_list_insert(device->plane_list.prev, &plane->link);

	return plane;

err_props:
	drm_property_info_free(plane->props, WDRM_PLANE__COUNT);
err:
	weston_drm_format_array_fini(&plane->formats);
	drm_plane_state_free(plane->state_cur, true);
	free(plane);
	return NULL;
}

/**
 * Find, or create, a special-purpose plane
 *
 * @param device DRM device
 * @param output Output to use for plane
 * @param type Type of plane
 */
static struct drm_plane *
drm_output_find_special_plane(struct drm_device *device,
			      struct drm_output *output,
			      enum wdrm_plane_type type)
{
	struct drm_backend *b = device->backend;
	struct drm_plane *plane;

	wl_list_for_each(plane, &device->plane_list, link) {
		struct weston_output *base;
		bool found_elsewhere = false;

		if (plane->type != type)
			continue;
		if (!drm_plane_is_available(plane, output))
			continue;

		/* On some platforms, primary/cursor planes can roam
		 * between different CRTCs, so make sure we don't claim the
		 * same plane for two outputs. */
		wl_list_for_each(base, &b->compositor->output_list, link) {
			struct drm_output *tmp = to_drm_output(base);
			if (!tmp)
				continue;

			if (tmp->cursor_plane == plane ||
			    tmp->scanout_plane == plane) {
				found_elsewhere = true;
				break;
			}
		}

		if (found_elsewhere)
			continue;

		/* If a plane already has a CRTC selected and it is not our
		 * output's CRTC, then do not select this plane. We cannot
		 * switch away a plane from a CTRC when active. */
		if ((type == WDRM_PLANE_TYPE_PRIMARY) &&
		    (plane->crtc_id != 0) &&
		    (plane->crtc_id != output->crtc->crtc_id))
			continue;

		plane->possible_crtcs = (1 << output->crtc->pipe);
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
	struct drm_device *device = plane->device;

	if (plane->type == WDRM_PLANE_TYPE_OVERLAY)
		drmModeSetPlane(device->drm.fd, plane->plane_id,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	drm_plane_state_free(plane->state_cur, true);
	drm_property_info_free(plane->props, WDRM_PLANE__COUNT);
	weston_plane_release(&plane->base);
	weston_drm_format_array_fini(&plane->formats);
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
 * @param device DRM device
 */
static void
create_sprites(struct drm_device *device)
{
	struct drm_backend *b = device->backend;
	drmModePlaneRes *kplane_res;
	drmModePlane *kplane;
	struct drm_plane *drm_plane;
	uint32_t i;
	uint32_t next_plane_idx = 0;
	kplane_res = drmModeGetPlaneResources(device->drm.fd);

	if (!kplane_res) {
		weston_log("failed to get plane resources: %s\n",
			strerror(errno));
		return;
	}

	for (i = 0; i < kplane_res->count_planes; i++) {
		kplane = drmModeGetPlane(device->drm.fd, kplane_res->planes[i]);
		if (!kplane)
			continue;

		drm_plane = drm_plane_create(device, kplane);
		drmModeFreePlane(kplane);
		if (!drm_plane)
			continue;

		if (drm_plane->type == WDRM_PLANE_TYPE_OVERLAY)
			weston_compositor_stack_plane(b->compositor,
						      &drm_plane->base,
						      &b->compositor->primary_plane);
	}

	wl_list_for_each (drm_plane, &device->plane_list, link)
		drm_plane->plane_idx = next_plane_idx++;

	drmModeFreePlaneResources(kplane_res);
}

/**
 * Clean up sprites (overlay planes)
 *
 * The counterpart to create_sprites.
 *
 * @param device DRM device
 */
static void
destroy_sprites(struct drm_device *device)
{
	struct drm_plane *plane, *next;

	wl_list_for_each_safe(plane, next, &device->plane_list, link)
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
	struct drm_device *device = output->device;
	struct drm_pending_state *pending_state = device->repaint_data;
	struct drm_output_state *state;
	int ret;

	assert(output);
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

	pending_state = drm_pending_state_alloc(device);
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
	struct weston_renderer *renderer = output->base.compositor->renderer;
	const struct pixman_renderer_interface *pixman = renderer->pixman;
	struct drm_device *device = output->device;
	int w = output->base.current_mode->width;
	int h = output->base.current_mode->height;
	unsigned int i;
	const struct pixman_renderer_output_options options = {
		.use_shadow = b->use_pixman_shadow,
		.fb_size = { .width = w, .height = h },
		.format = output->format
	};

	assert(options.format);

	if (!options.format->pixman_format) {
		weston_log("Unsupported pixel format %s\n",
			   options.format->drm_format_name);
		return -1;
	}

	if (pixman->output_create(&output->base, &options) < 0)
		goto err;

	/* FIXME error checking */
	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		output->dumb[i] = drm_fb_create_dumb(device, w, h,
						     options.format->format);
		if (!output->dumb[i])
			goto err;

		output->renderbuffer[i] =
			pixman->create_image_from_ptr(&output->base,
						      options.format, w, h,
						      output->dumb[i]->map,
						      output->dumb[i]->strides[0]);
		if (!output->renderbuffer[i])
			goto err;

		pixman_region32_init_rect(&output->renderbuffer[i]->damage,
					  output->base.x, output->base.y,
					  output->base.width,
					  output->base.height);
	}

	weston_log("DRM: output %s %s shadow framebuffer.\n", output->base.name,
		   b->use_pixman_shadow ? "uses" : "does not use");

	return 0;

err:
	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		if (output->dumb[i])
			drm_fb_unref(output->dumb[i]);
		if (output->renderbuffer[i])
			weston_renderbuffer_unref(output->renderbuffer[i]);

		output->dumb[i] = NULL;
		output->renderbuffer[i] = NULL;
	}
	pixman->output_destroy(&output->base);

	return -1;
}

static void
drm_output_fini_pixman(struct drm_output *output)
{
	struct weston_renderer *renderer = output->base.compositor->renderer;
	struct drm_backend *b = output->backend;
	unsigned int i;

	/* Destroying the Pixman surface will destroy all our buffers,
	 * regardless of refcount. Ensure we destroy them here. */
	if (!b->shutting_down &&
	    output->scanout_plane->state_cur->fb &&
	    output->scanout_plane->state_cur->fb->type == BUFFER_PIXMAN_DUMB) {
		drm_plane_reset_state(output->scanout_plane);
	}

	for (i = 0; i < ARRAY_LENGTH(output->dumb); i++) {
		weston_renderbuffer_unref(output->renderbuffer[i]);
		drm_fb_unref(output->dumb[i]);
		output->dumb[i] = NULL;
		output->renderbuffer[i] = NULL;
	}

	renderer->pixman->output_destroy(&output->base);
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
			pointer->pos = weston_pointer_clamp(pointer,
							    pointer->pos);
	}
}

static int
drm_output_attach_head(struct weston_output *output_base,
		       struct weston_head *head_base)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_backend *b = output->backend;
	struct drm_device *device = b->drm;
	struct drm_head *head = to_drm_head(head_base);

	if (wl_list_length(&output_base->head_list) >= MAX_CLONED_CONNECTORS)
		return -1;

	wl_list_remove(&head->disable_head_link);
	wl_list_init(&head->disable_head_link);

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
	device->state_invalid = true;

	weston_output_schedule_repaint(output_base);

	return 0;
}

static void
drm_output_detach_head(struct weston_output *output_base,
		       struct weston_head *head_base)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_head *head = to_drm_head(head_base);

	if (!output_base->enabled)
		return;

	/* Drop connectors that should no longer be driven on next repaint. */
	wl_list_insert(&output->disable_head, &head->disable_head_link);
}

int
parse_gbm_format(const char *s, const struct pixel_format_info *default_format,
		 const struct pixel_format_info **format)
{
	if (s == NULL) {
		*format = default_format;

		return 0;
	}

	/* GBM formats and DRM formats are identical. */
	*format = pixel_format_get_info_by_drm_name(s);
	if (!*format) {
		weston_log("fatal: unrecognized pixel format: %s\n", s);

		return -1;
	}

	return 0;
}

static int
drm_head_read_current_setup(struct drm_head *head, struct drm_device *device)
{
	int drm_fd = device->drm.fd;
	drmModeConnector *conn = head->connector.conn;
	drmModeEncoder *encoder;
	drmModeCrtc *crtc;

	/* Get the current mode on the crtc that's currently driving
	 * this connector. */
	encoder = drmModeGetEncoder(drm_fd, conn->encoder_id);
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

	/* Get the current max_bpc that's currently configured to
	 * this connector. */
	head->inherited_max_bpc = drm_property_get_value(
		&head->connector.props[WDRM_CONNECTOR_MAX_BPC],
		head->connector.props_drm, 0);

	return 0;
}

static void
drm_output_set_gbm_format(struct weston_output *base,
			  const char *gbm_format)
{
	struct drm_output *output = to_drm_output(base);

	if (parse_gbm_format(gbm_format, NULL, &output->format) == -1)
		output->format = NULL;
}

static void
drm_output_set_seat(struct weston_output *base,
		    const char *seat)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = output->backend;

	setup_output_seat_constraint(b, &output->base,
				     seat ? seat : "");
}

static void
drm_output_set_max_bpc(struct weston_output *base, unsigned max_bpc)
{
	struct drm_output *output = to_drm_output(base);

	assert(output);
	assert(!output->base.enabled);

	output->max_bpc = max_bpc;
}

static const struct { const char *name; uint32_t token; } content_types[] = {
	{ "no data",  WDRM_CONTENT_TYPE_NO_DATA },
	{ "graphics", WDRM_CONTENT_TYPE_GRAPHICS },
	{ "photo",    WDRM_CONTENT_TYPE_PHOTO },
	{ "cinema",   WDRM_CONTENT_TYPE_CINEMA },
	{ "game",     WDRM_CONTENT_TYPE_GAME },
};

static int
drm_output_set_content_type(struct weston_output *base,
			    const char *content_type)
{
	unsigned int i;
	struct drm_output *output = to_drm_output(base);

	if (content_type == NULL) {
		output->content_type = WDRM_CONTENT_TYPE_NO_DATA;
		return 0;
	}

	for (i = 0; i < ARRAY_LENGTH(content_types); i++)
		if (strcmp(content_types[i].name, content_type) == 0) {
			output->content_type = content_types[i].token;
			return 0;
		}

	weston_log("Error: unknown content-type for output %s: \"%s\"\n",
		   base->name, content_type);
	output->content_type = WDRM_CONTENT_TYPE_NO_DATA;
	return -1;
}

static int
drm_output_init_gamma_size(struct drm_output *output)
{
	struct drm_device *device = output->device;
	drmModeCrtc *crtc;

	assert(output->base.compositor);
	assert(output->crtc);
	crtc = drmModeGetCrtc(device->drm.fd, output->crtc->crtc_id);
	if (!crtc)
		return -1;

	output->base.gamma_size = crtc->gamma_size;

	drmModeFreeCrtc(crtc);

	return 0;
}

enum writeback_screenshot_state
drm_output_get_writeback_state(struct drm_output *output)
{
	if (!output->wb_state)
		return DRM_OUTPUT_WB_SCREENSHOT_OFF;

	return output->wb_state->state;
}

/** Pick a CRTC that might be able to drive all attached connectors
 *
 * @param output The output whose attached heads to include.
 * @return CRTC object to pick, or NULL on failure or not found.
 */
static struct drm_crtc *
drm_output_pick_crtc(struct drm_output *output)
{
	struct drm_device *device = output->device;
	struct drm_backend *backend = device->backend;
	struct weston_compositor *compositor = backend->compositor;
	struct weston_head *base;
	struct drm_head *head;
	struct drm_crtc *crtc;
	struct drm_crtc *best_crtc = NULL;
	struct drm_crtc *fallback_crtc = NULL;
	struct drm_crtc *existing_crtc[32];
	uint32_t possible_crtcs = 0xffffffff;
	unsigned n = 0;
	uint32_t crtc_id;
	unsigned int i;
	bool match;

	/* This algorithm ignores drmModeEncoder::possible_clones restriction,
	 * because it is more often set wrong than not in the kernel. */

	/* Accumulate a mask of possible crtcs and find existing routings. */
	wl_list_for_each(base, &output->base.head_list, output_link) {
		head = to_drm_head(base);

		possible_crtcs &=
			drm_connector_get_possible_crtcs_mask(&head->connector);

		crtc_id = head->inherited_crtc_id;
		if (crtc_id > 0 && n < ARRAY_LENGTH(existing_crtc))
			existing_crtc[n++] = drm_crtc_find(device, crtc_id);
	}

	/* Find a crtc that could drive each connector individually at least,
	 * and prefer existing routings. */
	wl_list_for_each(crtc, &device->crtc_list, link) {

		/* Could the crtc not drive each connector? */
		if (!(possible_crtcs & (1 << crtc->pipe)))
			continue;

		/* Is the crtc already in use? */
		if (crtc->output)
			continue;

		/* Try to preserve the existing CRTC -> connector routing;
		 * it makes initialisation faster, and also since we have a
		 * very dumb picking algorithm, may preserve a better
		 * choice. */
		for (i = 0; i < n; i++) {
			if (existing_crtc[i] == crtc)
				return crtc;
		}

		/* Check if any other head had existing routing to this CRTC.
		 * If they did, this is not the best CRTC as it might be needed
		 * for another output we haven't enabled yet. */
		match = false;
		wl_list_for_each(base, &compositor->head_list, compositor_link) {
			head = to_drm_head(base);
			if (!head)
				continue;

			if (head->base.output == &output->base)
				continue;

			if (weston_head_is_enabled(&head->base))
				continue;

			if (head->inherited_crtc_id == crtc->crtc_id) {
				match = true;
				break;
			}
		}
		if (!match)
			best_crtc = crtc;

		fallback_crtc = crtc;
	}

	if (best_crtc)
		return best_crtc;

	if (fallback_crtc)
		return fallback_crtc;

	/* Likely possible_crtcs was empty due to asking for clones,
	 * but since the DRM documentation says the kernel lies, let's
	 * pick one crtc anyway. Trial and error is the only way to
	 * be sure if something doesn't work. */

	/* First pick any existing assignment. */
	for (i = 0; i < n; i++) {
		crtc = existing_crtc[i];
		if (!crtc->output)
			return crtc;
	}

	/* Otherwise pick any available crtc. */
	wl_list_for_each(crtc, &device->crtc_list, link) {
		if (!crtc->output)
			return crtc;
	}

	return NULL;
}

/** Create an "empty" drm_crtc. It will only set its ID, pipe and props. After
 * all, it adds the object to the DRM-backend CRTC list.
 */
static struct drm_crtc *
drm_crtc_create(struct drm_device *device, uint32_t crtc_id, uint32_t pipe)
{
	struct drm_crtc *crtc;
	drmModeObjectPropertiesPtr props;

	props = drmModeObjectGetProperties(device->drm.fd, crtc_id,
					   DRM_MODE_OBJECT_CRTC);
	if (!props) {
		weston_log("failed to get CRTC properties\n");
		return NULL;
	}

	crtc = zalloc(sizeof(*crtc));
	if (!crtc)
		goto ret;

	drm_property_info_populate(device, crtc_props, crtc->props_crtc,
				   WDRM_CRTC__COUNT, props);
	crtc->device = device;
	crtc->crtc_id = crtc_id;
	crtc->pipe = pipe;
	crtc->output = NULL;

	/* Add it to the last position of the DRM-backend CRTC list */
	wl_list_insert(device->crtc_list.prev, &crtc->link);

ret:
	drmModeFreeObjectProperties(props);
	return crtc;
}

/** Destroy a drm_crtc object that was created with drm_crtc_create(). It will
 * also remove it from the DRM-backend CRTC list.
 */
static void
drm_crtc_destroy(struct drm_crtc *crtc)
{
	/* TODO: address the issue below to be able to remove the comment
	 * from the assert.
	 *
	 * https://gitlab.freedesktop.org/wayland/weston/-/issues/421
	 */

	//assert(!crtc->output);

	wl_list_remove(&crtc->link);
	drm_property_info_free(crtc->props_crtc, WDRM_CRTC__COUNT);
	free(crtc);
}

/** Find all CRTCs of the fd and create drm_crtc objects for them.
 *
 * The CRTCs are saved in a list of the drm_backend and will keep there until
 * the fd gets closed.
 *
 * @param device The DRM device structure.
 * @param resources The DRM resources, it is taken with drmModeGetResources
 * @return 0 on success (at least one CRTC in the list), -1 on failure.
 */
static int
drm_backend_create_crtc_list(struct drm_device *device, drmModeRes *resources)
{
	struct drm_crtc *crtc, *crtc_tmp;
	int i;

	/* Iterate through all CRTCs */
	for (i = 0; i < resources->count_crtcs; i++) {

		/* Let's create an object for the CRTC and add it to the list */
		crtc = drm_crtc_create(device, resources->crtcs[i], i);
		if (!crtc)
			goto err;
	}

	return 0;

err:
	wl_list_for_each_safe(crtc, crtc_tmp, &device->crtc_list, link)
		drm_crtc_destroy(crtc);
	return -1;
}


/** Populates scanout and cursor planes for the output. Also sets the topology
 * of the planes by adding them to the plane stacking list.
 */
static int
drm_output_init_planes(struct drm_output *output)
{
	struct drm_backend *b = output->backend;
	struct drm_device *device = output->device;

	output->scanout_plane =
		drm_output_find_special_plane(device, output,
					      WDRM_PLANE_TYPE_PRIMARY);
	if (!output->scanout_plane) {
		weston_log("Failed to find primary plane for output %s\n",
			   output->base.name);
		return -1;
	}

	weston_compositor_stack_plane(b->compositor,
				      &output->scanout_plane->base,
				      &b->compositor->primary_plane);

	/* Failing to find a cursor plane is not fatal, as we'll fall back
	 * to software cursor. */
	output->cursor_plane =
		drm_output_find_special_plane(device, output,
					      WDRM_PLANE_TYPE_CURSOR);

	if (output->cursor_plane)
		weston_compositor_stack_plane(b->compositor,
					      &output->cursor_plane->base,
					      NULL);
	else
		device->cursors_are_broken = true;

	return 0;
}

/** The opposite of drm_output_init_planes(). First of all it removes the planes
 * from the plane stacking list. After all it sets the planes of the output as NULL.
 */
static void
drm_output_deinit_planes(struct drm_output *output)
{
	struct drm_backend *b = output->backend;
	struct drm_device *device = output->device;

	/* If the compositor is already shutting down, the planes have already
	 * been destroyed. */
	if (!b->shutting_down) {
		wl_list_remove(&output->scanout_plane->base.link);
		wl_list_init(&output->scanout_plane->base.link);

		if (output->cursor_plane) {
			wl_list_remove(&output->cursor_plane->base.link);
			wl_list_init(&output->cursor_plane->base.link);
			/* Turn off hardware cursor */
			drmModeSetCursor(device->drm.fd, output->crtc->crtc_id, 0, 0, 0);
		}

		/* With universal planes, the planes are allocated at startup,
		 * freed at shutdown, and live on the plane list in between.
		 * We want the planes to  continue to exist and be freed up
		 * for other outputs.
		 */
		if (output->cursor_plane)
			drm_plane_reset_state(output->cursor_plane);
		if (output->scanout_plane)
			drm_plane_reset_state(output->scanout_plane);
	}

	output->cursor_plane = NULL;
	output->scanout_plane = NULL;
}

static struct weston_drm_format_array *
get_scanout_formats(struct drm_device *device)
{
	struct weston_compositor *ec = device->backend->compositor;
	const struct weston_drm_format_array *renderer_formats;
	struct weston_drm_format_array *scanout_formats, union_planes_formats;
	struct drm_plane *plane;
	int ret;

	/* If we got here it means that dma-buf feedback is supported and that
	 * the renderer has formats/modifiers to expose. */
	assert(ec->renderer->get_supported_formats != NULL);
	renderer_formats = ec->renderer->get_supported_formats(ec);

	scanout_formats = zalloc(sizeof(*scanout_formats));
	if (!scanout_formats) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	weston_drm_format_array_init(&union_planes_formats);
	weston_drm_format_array_init(scanout_formats);

	/* Compute the union of the format/modifiers of the KMS planes */
	wl_list_for_each(plane, &device->plane_list, link) {
		/* The scanout formats are used by the dma-buf feedback. But for
		 * now cursor planes do not support dma-buf buffers, only wl_shm
		 * buffers. So we skip cursor planes here. */
		if (plane->type == WDRM_PLANE_TYPE_CURSOR)
			continue;

		ret = weston_drm_format_array_join(&union_planes_formats,
						   &plane->formats);
		if (ret < 0)
			goto err;
	}

	/* Compute the intersection between the union of format/modifiers of KMS
	 * planes and the formats supported by the renderer */
	ret = weston_drm_format_array_replace(scanout_formats,
					      renderer_formats);
	if (ret < 0)
		goto err;

	ret = weston_drm_format_array_intersect(scanout_formats,
						&union_planes_formats);
	if (ret < 0)
		goto err;

	weston_drm_format_array_fini(&union_planes_formats);

	return scanout_formats;

err:
	weston_drm_format_array_fini(&union_planes_formats);
	weston_drm_format_array_fini(scanout_formats);
	free(scanout_formats);
	return NULL;
}

/** Pick a CRTC and reserve it for the output.
 *
 * On failure, the output remains without a CRTC.
 *
 * @param output The output with no CRTC associated.
 * @return 0 on success, -1 on failure.
 */
static int
drm_output_attach_crtc(struct drm_output *output)
{
	output->crtc = drm_output_pick_crtc(output);
	if (!output->crtc) {
		weston_log("Output '%s': No available CRTCs.\n",
			   output->base.name);
		return -1;
	}

	/* Reserve the CRTC for the output */
	output->crtc->output = output;

	return 0;
}

/** Release reservation of the CRTC.
 *
 * Make the CRTC free to be reserved and used by another output.
 *
 * @param output The output that will release its CRTC.
 */
static void
drm_output_detach_crtc(struct drm_output *output)
{
	struct drm_crtc *crtc = output->crtc;

	crtc->output = NULL;
	output->crtc = NULL;
}

static int
drm_output_enable(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_device *device = output->device;
	struct drm_backend *b = device->backend;
	int ret;

	assert(output);
	assert(!output->virtual);

	if (!output->format) {
		if (output->base.eotf_mode != WESTON_EOTF_MODE_SDR)
			output->format =
				pixel_format_get_info(DRM_FORMAT_XRGB2101010);
		else
			output->format = b->format;
	}

	ret = drm_output_attach_crtc(output);
	if (ret < 0)
		return -1;

	ret = drm_output_init_planes(output);
	if (ret < 0)
		goto err_crtc;

	if (drm_output_init_gamma_size(output) < 0)
		goto err_planes;

	if (b->pageflip_timeout)
		drm_output_pageflip_timer_create(output);

	if (b->compositor->renderer->type == WESTON_RENDERER_PIXMAN) {
		if (drm_output_init_pixman(output, b) < 0) {
			weston_log("Failed to init output pixman state\n");
			goto err_planes;
		}
	} else if (drm_output_init_egl(output, b) < 0) {
		weston_log("Failed to init output gl state\n");
		goto err_planes;
	}

	drm_output_init_backlight(output);

	output->base.start_repaint_loop = drm_output_start_repaint_loop;
	output->base.repaint = drm_output_repaint;
	output->base.assign_planes = drm_assign_planes;
	output->base.set_dpms = drm_set_dpms;
	output->base.switch_mode = drm_output_switch_mode;
	output->base.set_gamma = drm_output_set_gamma;

	if (device->atomic_modeset && !base->disable_planes)
		weston_output_update_capture_info(base, WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK,
						  base->current_mode->width,
						  base->current_mode->height,
						  pixel_format_get_info(output->format->format));

	weston_log("Output %s (crtc %d) video modes:\n",
		   output->base.name, output->crtc->crtc_id);
	drm_output_print_modes(output);

	return 0;

err_planes:
	drm_output_deinit_planes(output);
err_crtc:
	drm_output_detach_crtc(output);
	return -1;
}

static void
drm_output_deinit(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = output->backend;
	struct drm_device *device = b->drm;
	struct drm_pending_state *pending;

	if (!b->shutting_down) {
		pending = drm_pending_state_alloc(device);
		drm_output_get_disable_state(pending, output);
		drm_pending_state_apply_sync(pending);
	}

	if (b->compositor->renderer->type == WESTON_RENDERER_PIXMAN)
		drm_output_fini_pixman(output);
	else
		drm_output_fini_egl(output);

	drm_output_deinit_planes(output);
	drm_output_detach_crtc(output);

	if (output->hdr_output_metadata_blob_id) {
		drmModeDestroyPropertyBlob(device->drm.fd,
					   output->hdr_output_metadata_blob_id);
		output->hdr_output_metadata_blob_id = 0;
	}
}

void
drm_output_destroy(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_device *device = output->device;

	assert(output);
	assert(!output->virtual);

	if (output->page_flip_pending || output->atomic_complete_pending) {
		output->destroy_pending = true;
		weston_log("destroy output while page flip pending\n");
		return;
	}

	drm_output_set_cursor_view(output, NULL);

	if (output->base.enabled)
		drm_output_deinit(&output->base);

	drm_mode_list_destroy(device, &output->base.mode_list);

	if (output->pageflip_timer)
		wl_event_source_remove(output->pageflip_timer);

	weston_output_release(&output->base);

	assert(!output->state_last);
	drm_output_state_free(output->state_cur);

	assert(output->hdr_output_metadata_blob_id == 0);

	free(output);
}

static int
drm_output_disable(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);

	assert(output);
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
 * @return protection status in case of success, -1 otherwise
 */
static enum weston_hdcp_protection
drm_head_get_current_protection(struct drm_head *head)
{
	drmModeObjectProperties *props = head->connector.props_drm;
	struct drm_property_info *info;
	enum wdrm_content_protection_state protection;
	enum wdrm_hdcp_content_type type;
	enum weston_hdcp_protection weston_hdcp = WESTON_HDCP_DISABLE;

	info = &head->connector.props[WDRM_CONNECTOR_CONTENT_PROTECTION];
	protection = drm_property_get_value(info, props,
					    WDRM_CONTENT_PROTECTION__COUNT);

	if (protection == WDRM_CONTENT_PROTECTION__COUNT)
		return WESTON_HDCP_DISABLE;

	info = &head->connector.props[WDRM_CONNECTOR_HDCP_CONTENT_TYPE];
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
			   head->connector.connector_id);
		return WESTON_HDCP_DISABLE;
	}

	return weston_hdcp;
}

static int
drm_connector_update_properties(struct drm_connector *connector)
{
	struct drm_device *device = connector->device;
	drmModeObjectProperties *props;

	props = drmModeObjectGetProperties(device->drm.fd,
					   connector->connector_id,
					   DRM_MODE_OBJECT_CONNECTOR);
	if (!props) {
		weston_log("Error: failed to get connector properties\n");
		return -1;
	}

	if (connector->props_drm)
		drmModeFreeObjectProperties(connector->props_drm);
	connector->props_drm = props;

	return 0;
}

/** Replace connector data and monitor information
 *
 * @param connector The drm_connector object to be updated.
 * @param conn The connector data to be owned by the drm_connector, must match
 * the current drm_connector ID.
 * @return 0 on success, -1 on failure.
 *
 * Takes ownership of @c connector on success, not on failure.
 */
static int
drm_connector_assign_connector_info(struct drm_connector *connector,
				    drmModeConnector *conn)
{
	struct drm_device *device = connector->device;

	assert(connector->conn != conn);
	assert(connector->connector_id == conn->connector_id);

	if (drm_connector_update_properties(connector) < 0)
		return -1;

	if (connector->conn)
		drmModeFreeConnector(connector->conn);
	connector->conn = conn;

	drm_property_info_free(connector->props, WDRM_CONNECTOR__COUNT);
	drm_property_info_populate(device, connector_props, connector->props,
				   WDRM_CONNECTOR__COUNT, connector->props_drm);
	return 0;
}

static void
drm_connector_init(struct drm_device *device, struct drm_connector *connector,
		   uint32_t connector_id)
{
	connector->device = device;
	connector->connector_id = connector_id;
	connector->conn = NULL;
	connector->props_drm = NULL;
}

static void
drm_connector_fini(struct drm_connector *connector)
{
	drmModeFreeConnector(connector->conn);
	drmModeFreeObjectProperties(connector->props_drm);
	drm_property_info_free(connector->props, WDRM_CONNECTOR__COUNT);
}

static void
drm_head_log_info(struct drm_head *head, const char *msg)
{
	char *eotf_list;

	if (head->base.connected) {
		weston_log("DRM: head '%s' %s, connector %d is connected, "
			   "EDID make '%s', model '%s', serial '%s'\n",
			   head->base.name, msg, head->connector.connector_id,
			   head->base.make, head->base.model,
			   head->base.serial_number ?: "");
		eotf_list = weston_eotf_mask_to_str(head->base.supported_eotf_mask);
		if (eotf_list) {
			weston_log_continue(STAMP_SPACE
					    "Supported EOTF modes: %s\n",
					    eotf_list);
		}
		free(eotf_list);
	} else {
		weston_log("DRM: head '%s' %s, connector %d is disconnected.\n",
			   head->base.name, msg, head->connector.connector_id);
	}
}

/** Update connector and monitor information
 *
 * @param head The head to update.
 * @param conn The DRM connector object.
 * @returns 0 on success, -1 on failure.
 *
 * Updates monitor information and connection status. This may schedule a
 * heads changed call to the user.
 *
 * Takes ownership of @c connector on success, not on failure.
 */
static int
drm_head_update_info(struct drm_head *head, drmModeConnector *conn)
{
	int ret;

	ret = drm_connector_assign_connector_info(&head->connector, conn);

	update_head_from_connector(head);
	weston_head_set_content_protection_status(&head->base,
					drm_head_get_current_protection(head));

	return ret;
}

/** Update writeback connector
 *
 * @param writeback The writeback to update.
 * @param conn DRM connector object.
 * @returns 0 on success, -1 on failure.
 *
 * Takes ownership of @c connector on success, not on failure.
 */
static int
drm_writeback_update_info(struct drm_writeback *writeback, drmModeConnector *conn)
{
	int ret;

	ret = drm_connector_assign_connector_info(&writeback->connector, conn);

	return ret;
}

/**
 * Create a Weston head for a connector
 *
 * Given a DRM connector, create a matching drm_head structure and add it
 * to Weston's head list.
 *
 * @param device DRM device structure
 * @param conn DRM connector object
 * @param drm_device udev device pointer
 * @returns 0 on success, -1 on failure
 *
 * Takes ownership of @c connector on success, not on failure.
 */
static int
drm_head_create(struct drm_device *device, drmModeConnector *conn,
		struct udev_device *drm_device)
{
	struct drm_backend *backend = device->backend;
	struct drm_head *head;
	char *name;
	int ret;

	head = zalloc(sizeof *head);
	if (!head)
		return -1;

	drm_connector_init(device, &head->connector, conn->connector_id);

	name = make_connector_name(conn);
	if (!name)
		goto err;

	weston_head_init(&head->base, name);
	free(name);

	head->base.backend = &backend->base;

	wl_list_init(&head->disable_head_link);

	ret = drm_head_update_info(head, conn);
	if (ret < 0)
		goto err_update;

	head->backlight = backlight_init(drm_device, conn->connector_type);

	if (conn->connector_type == DRM_MODE_CONNECTOR_LVDS ||
	    conn->connector_type == DRM_MODE_CONNECTOR_eDP)
		weston_head_set_internal(&head->base);

	if (drm_head_read_current_setup(head, device) < 0) {
		weston_log("Failed to retrieve current mode from connector %d.\n",
			   head->connector.connector_id);
		/* Not fatal. */
	}

	weston_compositor_add_head(backend->compositor, &head->base);
	drm_head_log_info(head, "found");

	return 0;

err_update:
	weston_head_release(&head->base);
err:
	drm_connector_fini(&head->connector);
	free(head);
	return -1;
}

static void
drm_head_destroy(struct weston_head *base)
{
	struct drm_head *head = to_drm_head(base);

	assert(head);

	weston_head_release(&head->base);

	drm_connector_fini(&head->connector);

	if (head->backlight)
		backlight_destroy(head->backlight);

	free(head);
}

static struct drm_device *
drm_device_find_by_output(struct weston_compositor *compositor, const char *name)
{
	struct drm_device *device = NULL;
	struct weston_head *base = NULL;
	struct drm_head *head;
	const char *tmp;

	while ((base = weston_compositor_iterate_heads(compositor, base))) {
		tmp = weston_head_get_name(base);
		if (strcmp(name, tmp) != 0)
			continue;
		head = to_drm_head(base);
		device = head->connector.device;
		break;
	}

	return device;
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
 * @param backend The backend instance.
 * @param name Name for the new output.
 * @returns The output, or NULL on failure.
 */
static struct weston_output *
drm_output_create(struct weston_backend *backend, const char *name)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct drm_device *device;
	struct drm_output *output;

	device = drm_device_find_by_output(b->compositor, name);
	if (!device)
		return NULL;

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	output->device = device;
	output->crtc = NULL;

	wl_list_init(&output->disable_head);

	output->max_bpc = 16;
#ifdef BUILD_DRM_GBM
	output->gbm_bo_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
#endif

	weston_output_init(&output->base, b->compositor, name);

	output->base.enable = drm_output_enable;
	output->base.destroy = drm_output_destroy;
	output->base.disable = drm_output_disable;
	output->base.attach_head = drm_output_attach_head;
	output->base.detach_head = drm_output_detach_head;

	output->backend = b;

	output->destroy_pending = false;
	output->disable_pending = false;

	output->state_cur = drm_output_state_alloc(output, NULL);

	weston_compositor_add_pending_output(&output->base, b->compositor);

	return &output->base;
}

static void
pixman_copy_screenshot(uint32_t *dst, uint32_t *src, int dst_stride,
		       int src_stride, int pixman_format, int width, int height)
{
	pixman_image_t *pixman_dst;
	pixman_image_t *pixman_src;

	pixman_src = pixman_image_create_bits(pixman_format,
					      width, height,
					      src, src_stride);
	pixman_dst = pixman_image_create_bits(pixman_format,
					      width, height,
					      dst, dst_stride);
	assert(pixman_src);
	assert(pixman_dst);

	pixman_image_composite32(PIXMAN_OP_SRC,
				 pixman_src,     /* src */
				 NULL,           /* mask */
				 pixman_dst,     /* dst */
				 0, 0,           /* src_x, src_y */
				 0, 0,           /* mask_x, mask_y */
				 0, 0,           /* dst_x, dst_y */
				 width, height); /* width, height */

	pixman_image_unref(pixman_src);
	pixman_image_unref(pixman_dst);
}

static void
drm_writeback_success_screenshot(struct drm_writeback_state *state)
{
	struct drm_output *output = state->output;
	struct weston_buffer *buffer =
		weston_capture_task_get_buffer(state->ct);
	int width, height;
	int dst_stride, src_stride;
	uint32_t *src, *dst;

	src = state->fb->map;
	src_stride = state->fb->strides[0];

	dst = wl_shm_buffer_get_data(buffer->shm_buffer);
	dst_stride = wl_shm_buffer_get_stride(buffer->shm_buffer);

	width = state->fb->width;
	height = state->fb->height;

	wl_shm_buffer_begin_access(buffer->shm_buffer);
	pixman_copy_screenshot(dst, src, dst_stride, src_stride,
			       buffer->pixel_format->pixman_format,
			       width, height);
	wl_shm_buffer_end_access(buffer->shm_buffer);

	weston_capture_task_retire_complete(state->ct);
	drm_writeback_state_free(state);
	output->wb_state = NULL;
}

void
drm_writeback_fail_screenshot(struct drm_writeback_state *state,
			      const char *err_msg)
{
	struct drm_output *output = state->output;

	weston_capture_task_retire_failed(state->ct, err_msg);
	drm_writeback_state_free(state);
	output->wb_state = NULL;
}

static int
drm_writeback_save_callback(int fd, uint32_t mask, void *data)
{
	struct drm_writeback_state *state = data;

	wl_event_source_remove(state->wb_source);
	close(fd);

	drm_writeback_success_screenshot(state);

	return 0;
}

static bool
drm_writeback_has_finished(struct drm_writeback_state *state)
{
	struct pollfd pollfd;
	int ret;

	pollfd.fd = state->out_fence_fd;
	pollfd.events = POLLIN;

	while ((ret = poll(&pollfd, 1, 0)) == -1 && errno == EINTR)
		continue;

	if (ret < 0) {
		drm_writeback_fail_screenshot(state, "drm: polling wb fence failed");
		return true;
	} else if (ret > 0) {
		/* fence already signaled, simply save the screenshot */
		drm_writeback_success_screenshot(state);
		return true;
	}

	/* poll() returned 0, what means that out fence was not signalled yet */
	return false;
}

bool
drm_writeback_should_wait_completion(struct drm_writeback_state *state)
{
	struct weston_compositor *ec = state->output->base.compositor;
	struct wl_event_loop *event_loop;

	if (state->state == DRM_OUTPUT_WB_SCREENSHOT_WAITING_SIGNAL)
		return true;

	if (state->state == DRM_OUTPUT_WB_SCREENSHOT_CHECK_FENCE) {
		if (drm_writeback_has_finished(state))
			return false;

		/* The writeback has not finished yet. So add callback that gets
		 * called when the sync fd of the writeback job gets signalled.
		 * We need to wait for that to resume the repaint loop. */
		event_loop = wl_display_get_event_loop(ec->wl_display);
		state->wb_source =
			wl_event_loop_add_fd(event_loop, state->out_fence_fd,
					     WL_EVENT_READABLE,
					     drm_writeback_save_callback, state);
		if (!state->wb_source) {
			drm_writeback_fail_screenshot(state, "drm: out of memory");
			return false;
		}

		state->state = DRM_OUTPUT_WB_SCREENSHOT_WAITING_SIGNAL;

		return true;
	}

	return false;
}

void
drm_writeback_reference_planes(struct drm_writeback_state *state,
			       struct wl_list *plane_state_list)
{
	struct drm_plane_state *plane_state;
	struct drm_fb **fb;

	wl_list_for_each(plane_state, plane_state_list, link) {
		if (!plane_state->fb)
			continue;
		fb = wl_array_add(&state->referenced_fbs, sizeof(*fb));
		*fb = drm_fb_ref(plane_state->fb);
	}
}

static int
drm_writeback_populate_formats(struct drm_writeback *wb)
{
	struct drm_property_info *info = wb->connector.props;
	drmModeObjectProperties *props = wb->connector.props_drm;
	uint64_t blob_id;
	drmModePropertyBlobPtr blob;
	uint32_t *blob_formats;
	unsigned int i;

	blob_id = drm_property_get_value(&info[WDRM_CONNECTOR_WRITEBACK_PIXEL_FORMATS],
					 props, 0);
	if (blob_id == 0)
		return -1;

	blob = drmModeGetPropertyBlob(wb->device->drm.fd, blob_id);
	if (!blob)
		return -1;

	blob_formats = blob->data;

	for (i = 0; i < blob->length / sizeof(uint32_t); i++)
		if (!weston_drm_format_array_add_format(&wb->formats,
							blob_formats[i]))
			goto err;

	return 0;

err:
	drmModeFreePropertyBlob(blob);
	return -1;
}

/**
 * Create a Weston writeback for a writeback connector
 *
 * Given a DRM connector of type writeback, create a matching drm_writeback
 * structure and add it to Weston's writeback list.
 *
 * @param device DRM device structure
 * @param conn DRM connector object of type writeback
 * @returns 0 on success, -1 on failure
 *
 * Takes ownership of @c connector on success, not on failure.
 */
static int
drm_writeback_create(struct drm_device *device, drmModeConnector *conn)
{
	struct drm_writeback *writeback;
	int ret;

	writeback = zalloc(sizeof *writeback);
	assert(writeback);

	writeback->device = device;

	drm_connector_init(device, &writeback->connector, conn->connector_id);

	ret = drm_writeback_update_info(writeback, conn);
	if (ret < 0)
		goto err;

	weston_drm_format_array_init(&writeback->formats);
	ret = drm_writeback_populate_formats(writeback);
	if (ret < 0)
		goto err_formats;

	wl_list_insert(&device->writeback_connector_list, &writeback->link);
	return 0;

err_formats:
	weston_drm_format_array_fini(&writeback->formats);
err:
	drm_connector_fini(&writeback->connector);
	free(writeback);
	return -1;
}

static void
drm_writeback_destroy(struct drm_writeback *writeback)
{
	drm_connector_fini(&writeback->connector);
	weston_drm_format_array_fini(&writeback->formats);
	wl_list_remove(&writeback->link);

	free(writeback);
}

/** Given the DRM connector object of a connector, create drm_head or
 * drm_writeback object (depending on the type of connector) for it.
 *
 * The object is then added to the DRM-backend list of heads or writebacks.
 *
 * @param device The DRM device structure
 * @param conn The DRM connector object
 * @param drm_device udev device pointer
 * @return 0 on success, -1 on failure
 */
static int
drm_backend_add_connector(struct drm_device *device, drmModeConnector *conn,
			  struct udev_device *drm_device)
{
	int ret;

	if (conn->connector_type == DRM_MODE_CONNECTOR_WRITEBACK) {
		ret = drm_writeback_create(device, conn);
		if (ret < 0)
			weston_log("DRM: failed to create writeback for connector %d.\n",
				   conn->connector_id);
	} else {
		ret = drm_head_create(device, conn, drm_device);
		if (ret < 0)
			weston_log("DRM: failed to create head for connector %d.\n",
				   conn->connector_id);
	}

	return ret;
}

/** Find all connectors of the fd and create drm_head or drm_writeback objects
 * (depending on the type of connector they are) for each of them
 *
 * These objects are added to the DRM-backend lists of heads and writebacks.
 *
 * @param device The DRM device structure
 * @param drm_device udev device pointer
 * @param resources The DRM resources, it is taken with drmModeGetResources
 * @return 0 on success, -1 on failure
 */
static int
drm_backend_discover_connectors(struct drm_device *device,
				struct udev_device *drm_device,
				drmModeRes *resources)
{
	drmModeConnector *conn;
	int i, ret;

	device->min_width  = resources->min_width;
	device->max_width  = resources->max_width;
	device->min_height = resources->min_height;
	device->max_height = resources->max_height;

	for (i = 0; i < resources->count_connectors; i++) {
		uint32_t connector_id = resources->connectors[i];

		conn = drmModeGetConnector(device->drm.fd, connector_id);
		if (!conn)
			continue;

		ret = drm_backend_add_connector(device, conn, drm_device);
		if (ret < 0)
			drmModeFreeConnector(conn);
	}

	return 0;
}

static bool
resources_has_connector(drmModeRes *resources, uint32_t connector_id)
{
	for (int i = 0; i < resources->count_connectors; i++) {
		if (resources->connectors[i] == connector_id)
			return true;
	}

	return false;
}

static void
drm_backend_update_connectors(struct drm_device *device,
			      struct udev_device *drm_device)
{
	struct drm_backend *b = device->backend;
	drmModeRes *resources;
	drmModeConnector *conn;
	struct weston_head *base, *base_next;
	struct drm_head *head;
	struct drm_writeback *writeback, *writeback_next;
	uint32_t connector_id;
	int i, ret;

	resources = drmModeGetResources(device->drm.fd);
	if (!resources) {
		weston_log("drmModeGetResources failed\n");
		return;
	}

	/* collect new connectors that have appeared, e.g. MST */
	for (i = 0; i < resources->count_connectors; i++) {
		connector_id = resources->connectors[i];

		conn = drmModeGetConnector(device->drm.fd, connector_id);
		if (!conn)
			continue;

		head = drm_head_find_by_connector(b, connector_id);
		writeback = drm_writeback_find_by_connector(b, connector_id);

		/* Connector can't be owned by both a head and a writeback, so
		 * one of the searches must fail. */
		assert(head == NULL || writeback == NULL);

		if (head) {
			ret = drm_head_update_info(head, conn);
			if (head->base.device_changed)
				drm_head_log_info(head, "updated");
		} else if (writeback) {
			ret = drm_writeback_update_info(writeback, conn);
		} else {
			ret = drm_backend_add_connector(b->drm, conn, drm_device);
		}

		if (ret < 0)
			drmModeFreeConnector(conn);
	}

	/* Destroy head objects of connectors (except writeback connectors) that
	 * have disappeared. */
	wl_list_for_each_safe(base, base_next,
			      &b->compositor->head_list, compositor_link) {
		head = to_drm_head(base);
		if (!head)
			continue;
		connector_id = head->connector.connector_id;

		if (head->connector.device != device)
			continue;

		if (resources_has_connector(resources, connector_id))
			continue;

		weston_log("DRM: head '%s' (connector %d) disappeared.\n",
			   head->base.name, connector_id);
		drm_head_destroy(base);
	}

	/* Destroy writeback objects of writeback connectors that have
	 * disappeared. */
	wl_list_for_each_safe(writeback, writeback_next,
			      &b->drm->writeback_connector_list, link) {
		connector_id = writeback->connector.connector_id;

		if (resources_has_connector(resources, connector_id))
			continue;

		weston_log("DRM: writeback connector (connector %d) disappeared.\n",
			   connector_id);
		drm_writeback_destroy(writeback);
	}

	drmModeFreeResources(resources);
}

static enum wdrm_connector_property
drm_connector_find_property_by_id(struct drm_connector *connector,
				  uint32_t property_id)
{
	int i;
	enum wdrm_connector_property prop = WDRM_CONNECTOR__COUNT;

	if (!connector || !property_id)
		return WDRM_CONNECTOR__COUNT;

	for (i = 0; i < WDRM_CONNECTOR__COUNT; i++)
		if (connector->props[i].prop_id == property_id) {
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

	head = drm_head_find_by_connector(b, connector_id);
	if (!head) {
		weston_log("DRM: failed to find head for connector id: %d.\n",
			   connector_id);
		return;
	}

	conn_prop = drm_connector_find_property_by_id(&head->connector, property_id);
	if (conn_prop >= WDRM_CONNECTOR__COUNT)
		return;

	if (drm_connector_update_properties(&head->connector) < 0)
		return;

	if (conn_prop == WDRM_CONNECTOR_CONTENT_PROTECTION) {
		weston_head_set_content_protection_status(&head->base,
					     drm_head_get_current_protection(head));
	}
}

static int
udev_event_is_hotplug(struct drm_device *device, struct udev_device *udev_device)
{
	const char *sysnum;
	const char *val;

	sysnum = udev_device_get_sysnum(udev_device);
	if (!sysnum || atoi(sysnum) != device->drm.id)
		return 0;

	val = udev_device_get_property_value(udev_device, "HOTPLUG");
	if (!val)
		return 0;

	return strcmp(val, "1") == 0;
}

static int
udev_event_is_conn_prop_change(struct drm_backend *b,
			       struct udev_device *udev_device,
			       uint32_t *connector_id,
			       uint32_t *property_id)

{
	const char *val;
	int id;

	val = udev_device_get_property_value(udev_device, "CONNECTOR");
	if (!val || !safe_strtoint(val, &id))
		return 0;
	else
		*connector_id = id;

	val = udev_device_get_property_value(udev_device, "PROPERTY");
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
	struct drm_device *device;

	event = udev_monitor_receive_device(b->udev_monitor);

	if (udev_event_is_hotplug(b->drm, event)) {
		if (udev_event_is_conn_prop_change(b, event, &conn_id, &prop_id))
			drm_backend_update_conn_props(b, conn_id, prop_id);
		else
			drm_backend_update_connectors(b->drm, event);
	}

	wl_list_for_each(device, &b->kms_list, link) {
		if (udev_event_is_hotplug(device, event)) {
			if (udev_event_is_conn_prop_change(b, event, &conn_id, &prop_id))
				drm_backend_update_conn_props(b, conn_id, prop_id);
			else
				drm_backend_update_connectors(device, event);
		}
	}

	udev_device_unref(event);

	return 1;
}

void
drm_destroy(struct weston_backend *backend)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct weston_compositor *ec = b->compositor;
	struct drm_device *device = b->drm;
	struct weston_head *base, *next;
	struct drm_crtc *crtc, *crtc_tmp;
	struct drm_writeback *writeback, *writeback_tmp;

	udev_input_destroy(&b->input);

	wl_event_source_remove(b->udev_drm_source);
	wl_event_source_remove(b->drm_source);

	b->shutting_down = true;

	destroy_sprites(b->drm);

	weston_log_scope_destroy(b->debug);
	b->debug = NULL;
	weston_compositor_shutdown(ec);

	wl_list_for_each_safe(crtc, crtc_tmp, &b->drm->crtc_list, link)
		drm_crtc_destroy(crtc);

	wl_list_for_each_safe(base, next, &ec->head_list, compositor_link) {
		if (to_drm_head(base))
			drm_head_destroy(base);
	}

	wl_list_for_each_safe(writeback, writeback_tmp,
			      &b->drm->writeback_connector_list, link)
		drm_writeback_destroy(writeback);

#ifdef BUILD_DRM_GBM
	if (b->gbm)
		gbm_device_destroy(b->gbm);
#endif

	udev_monitor_unref(b->udev_monitor);
	udev_unref(b->udev);

	weston_launcher_close(ec->launcher, device->drm.fd);
	weston_launcher_destroy(ec->launcher);

	free(device->drm.filename);
	free(device);
	free(b);
}

static void
session_notify(struct wl_listener *listener, void *data)
{
	struct weston_compositor *compositor = data;
	struct drm_backend *b =
		container_of(listener, struct drm_backend, session_listener);
	struct drm_device *device = b->drm;
	struct weston_output *output;

	if (compositor->session_active) {
		weston_log("activating session\n");
		weston_compositor_wake(compositor);
		weston_compositor_damage_all(compositor);
		device->state_invalid = true;
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

		wl_list_for_each(output, &compositor->output_list, link)
			if (to_drm_output(output))
				output->repaint_needed = false;
	}
}


/**
 * Handle KMS GPU being added/removed
 *
 * If the device being added/removed is the KMS device, we activate/deactivate
 * the compositor session.
 *
 * @param backend The DRM backend instance.
 * @param devnum The device being added/removed.
 * @param added Whether the device is being added (or removed)
 */
static void
drm_device_changed(struct weston_backend *backend,
		dev_t devnum, bool added)
{
	struct drm_backend *b = container_of(backend, struct drm_backend, base);
	struct weston_compositor *compositor = b->compositor;
	struct drm_device *device = b->drm;

	if (device->drm.fd < 0 || device->drm.devnum != devnum ||
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
drm_device_is_kms(struct drm_backend *b, struct drm_device *device,
		  struct udev_device *udev_device)
{
	struct weston_compositor *compositor = b->compositor;
	const char *filename = udev_device_get_devnode(udev_device);
	const char *sysnum = udev_device_get_sysnum(udev_device);
	dev_t devnum = udev_device_get_devnum(udev_device);
	drmModeRes *res;
	int id = -1, fd;

	if (!filename)
		return false;

	fd = weston_launcher_open(compositor->launcher, filename, O_RDWR);
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
	if (device->drm.fd >= 0)
		weston_launcher_close(compositor->launcher, device->drm.fd);
	free(device->drm.filename);

	device->drm.fd = fd;
	device->drm.id = id;
	device->drm.filename = strdup(filename);
	device->drm.devnum = devnum;

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
	struct drm_device *device = b->drm;
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	const char *path, *device_seat, *id;
	struct udev_device *dev, *drm_device, *pci;

	e = udev_enumerate_new(b->udev);
	udev_enumerate_add_match_subsystem(e, "drm");
	udev_enumerate_add_match_sysname(e, "card[0-9]*");

	udev_enumerate_scan_devices(e);
	drm_device = NULL;
	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		bool is_boot_vga = false;

		path = udev_list_entry_get_name(entry);
		dev = udev_device_new_from_syspath(b->udev, path);
		if (!dev)
			continue;
		device_seat = udev_device_get_property_value(dev, "ID_SEAT");
		if (!device_seat)
			device_seat = default_seat;
		if (strcmp(device_seat, seat)) {
			udev_device_unref(dev);
			continue;
		}

		pci = udev_device_get_parent_with_subsystem_devtype(dev,
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
			udev_device_unref(dev);
			continue;
		}

		/* Make sure this device is actually capable of modesetting;
		 * if this call succeeds, device->drm.{fd,filename} will be set,
		 * and any old values freed. */
		if (!drm_device_is_kms(b, b->drm, dev)) {
			udev_device_unref(dev);
			continue;
		}

		/* There can only be one boot_vga device, and we try to use it
		 * at all costs. */
		if (is_boot_vga) {
			if (drm_device)
				udev_device_unref(drm_device);
			drm_device = dev;
			break;
		}

		/* Per the (!is_boot_vga && drm_device) test above, we only
		 * trump existing saved devices with boot-VGA devices, so if
		 * we end up here, this must be the first device we've seen. */
		assert(!drm_device);
		drm_device = dev;
	}

	/* If we're returning a device to use, we must have an open FD for
	 * it. */
	assert(!!drm_device == (device->drm.fd >= 0));

	udev_enumerate_unref(e);
	return drm_device;
}

static struct udev_device *
open_specific_drm_device(struct drm_backend *b, struct drm_device *device,
			 const char *name)
{
	struct udev_device *udev_device;

	udev_device = udev_device_new_from_subsystem_sysname(b->udev, "drm", name);
	if (!udev_device) {
		weston_log("ERROR: could not open DRM device '%s'\n", name);
		return NULL;
	}

	if (!drm_device_is_kms(b, device, udev_device)) {
		udev_device_unref(udev_device);
		weston_log("ERROR: DRM device '%s' is not a KMS device.\n", name);
		return NULL;
	}

	/* If we're returning a device to use, we must have an open FD for
	 * it. */
	assert(device->drm.fd >= 0);

	return udev_device;
}

static void
planes_binding(struct weston_keyboard *keyboard, const struct timespec *time,
	       uint32_t key, void *data)
{
	struct drm_backend *b = data;
	struct drm_device *device = b->drm;

	switch (key) {
	case KEY_C:
		device->cursors_are_broken ^= true;
		break;
	case KEY_V:
		/* We don't support overlay-plane usage with legacy KMS. */
		if (device->atomic_modeset)
			device->sprites_are_broken ^= true;
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
	struct drm_device *device;
	int fd, ret;

	output = container_of(listener, struct drm_output,
			      recorder_frame_listener);
	device = output->device;

	if (!output->recorder)
		return;

	ret = drmPrimeHandleToFD(device->drm.fd,
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
	struct drm_device *device = b->drm;
	int fd;
	drm_magic_t magic;

	fd = open(device->drm.filename, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return NULL;

	drmGetMagic(fd, &magic);
	drmAuthMagic(device->drm.fd, magic);

	return vaapi_recorder_create(fd, width, height, filename);
}

static void
recorder_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t key, void *data)
{
	struct drm_backend *b = data;
	struct weston_output *base_output;
	struct drm_output *output;
	int width, height;

	wl_list_for_each(base_output, &b->compositor->output_list, link) {
		output = to_drm_output(base_output);
		if (output)
			break;
	}

	if (!output->recorder) {
		if (!output->format ||
		    output->format->format != DRM_FORMAT_XRGB8888) {
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

static struct drm_device *
drm_device_create(struct drm_backend *backend, const char *name)
{
	struct weston_compositor *compositor = backend->compositor;
	struct udev_device *udev_device;
	struct drm_device *device;
	struct wl_event_loop *loop;
	drmModeRes *res;

	device = zalloc(sizeof *device);
	if (device == NULL)
		return NULL;
	device->state_invalid = true;
	device->drm.fd = -1;
	device->backend = backend;
	device->gem_handle_refcnt = hash_table_create();

	udev_device = open_specific_drm_device(backend, device, name);
	if (!udev_device) {
		free(device);
		return NULL;
	}

	if (init_kms_caps(device) < 0) {
		weston_log("failed to initialize kms\n");
		goto err;
	}

	res = drmModeGetResources(device->drm.fd);
	if (!res) {
		weston_log("Failed to get drmModeRes\n");
		goto err;
	}

	wl_list_init(&device->crtc_list);
	if (drm_backend_create_crtc_list(device, res) == -1) {
		weston_log("Failed to create CRTC list for DRM-backend\n");
		goto err;
	}

	loop = wl_display_get_event_loop(compositor->wl_display);
	wl_event_loop_add_fd(loop, device->drm.fd,
			     WL_EVENT_READABLE, on_drm_input, device);

	wl_list_init(&device->plane_list);
	create_sprites(device);

	wl_list_init(&device->writeback_connector_list);
	if (drm_backend_discover_connectors(device, udev_device, res) < 0) {
		weston_log("Failed to create heads for %s\n", device->drm.filename);
		goto err;
	}

	/* 'compute' faked zpos values in case HW doesn't expose any */
	drm_backend_create_faked_zpos(device);

	return device;
err:
	return NULL;
}

static void
open_additional_devices(struct drm_backend *backend, const char *cards)
{
	struct drm_device *device;
	char *tokenize = strdup(cards);
	char *card = strtok(tokenize, ",");

	while (card) {
		device = drm_device_create(backend, card);
		if (!device) {
			weston_log("unable to use card %s\n", card);
			goto next;
		}

		weston_log("adding secondary device %s\n",
			   device->drm.filename);
		wl_list_insert(&backend->kms_list, &device->link);

next:
		card = strtok(NULL, ",");
	}

	free(tokenize);
}

static const struct weston_drm_output_api api = {
	drm_output_set_mode,
	drm_output_set_gbm_format,
	drm_output_set_seat,
	drm_output_set_max_bpc,
	drm_output_set_content_type,
};

static struct drm_backend *
drm_backend_create(struct weston_compositor *compositor,
		   struct weston_drm_backend_config *config)
{
	struct drm_backend *b;
	struct drm_device *device;
	struct udev_device *drm_device;
	struct wl_event_loop *loop;
	const char *seat_id = default_seat;
	const char *session_seat;
	struct weston_drm_format_array *scanout_formats;
	drmModeRes *res;
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

	device = zalloc(sizeof *device);
	if (device == NULL)
		return NULL;
	device->state_invalid = true;
	device->drm.fd = -1;
	device->backend = b;

	b->drm = device;
	wl_list_init(&b->kms_list);

	b->compositor = compositor;
	b->pageflip_timeout = config->pageflip_timeout;
	b->use_pixman_shadow = config->use_pixman_shadow;

	b->debug = weston_compositor_add_log_scope(compositor, "drm-backend",
						   "Debug messages from DRM/KMS backend\n",
						   NULL, NULL, NULL);

	compositor->backend = &b->base;

	if (parse_gbm_format(config->gbm_format,
			     pixel_format_get_info(DRM_FORMAT_XRGB8888),
			     &b->format) < 0)
		goto err_compositor;

	/* Check if we run drm-backend using a compatible launcher */
	compositor->launcher = weston_launcher_connect(compositor, seat_id, true);
	if (compositor->launcher == NULL) {
		weston_log("fatal: your system should either provide the "
			   "logind D-Bus API, or use seatd.\n");
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
		drm_device = open_specific_drm_device(b, device,
						      config->specific_device);
	else
		drm_device = find_primary_gpu(b, seat_id);
	if (drm_device == NULL) {
		weston_log("no drm device found\n");
		goto err_udev;
	}

	if (init_kms_caps(device) < 0) {
		weston_log("failed to initialize kms\n");
		goto err_udev_dev;
	}

	if (config->additional_devices)
		open_additional_devices(b, config->additional_devices);

	if (config->renderer == WESTON_RENDERER_AUTO) {
#ifdef BUILD_DRM_GBM
		config->renderer = WESTON_RENDERER_GL;
#else
		config->renderer = WESTON_RENDERER_PIXMAN;
#endif
	}

	switch (config->renderer) {
	case WESTON_RENDERER_PIXMAN:
		if (init_pixman(b) < 0) {
			weston_log("failed to initialize pixman renderer\n");
			goto err_udev_dev;
		}
		break;
	case WESTON_RENDERER_GL:
		if (init_egl(b) < 0) {
			weston_log("failed to initialize egl\n");
			goto err_udev_dev;
		}
		break;
	default:
		weston_log("unsupported renderer for DRM backend\n");
		goto err_udev_dev;
	}

	b->base.destroy = drm_destroy;
	b->base.repaint_begin = drm_repaint_begin;
	b->base.repaint_flush = drm_repaint_flush;
	b->base.repaint_cancel = drm_repaint_cancel;
	b->base.create_output = drm_output_create;
	b->base.device_changed = drm_device_changed;
	b->base.can_scanout_dmabuf = drm_can_scanout_dmabuf;

	weston_setup_vt_switch_bindings(compositor);

	res = drmModeGetResources(b->drm->drm.fd);
	if (!res) {
		weston_log("Failed to get drmModeRes\n");
		goto err_udev_dev;
	}

	wl_list_init(&b->drm->crtc_list);
	if (drm_backend_create_crtc_list(b->drm, res) == -1) {
		weston_log("Failed to create CRTC list for DRM-backend\n");
		goto err_create_crtc_list;
	}

	wl_list_init(&device->plane_list);
	create_sprites(b->drm);

	if (udev_input_init(&b->input,
			    compositor, b->udev, seat_id,
			    config->configure_device) < 0) {
		weston_log("failed to create input devices\n");
		goto err_sprite;
	}

	wl_list_init(&b->drm->writeback_connector_list);
	if (drm_backend_discover_connectors(b->drm, drm_device, res) < 0) {
		weston_log("Failed to create heads for %s\n", b->drm->drm.filename);
		goto err_udev_input;
	}

	drmModeFreeResources(res);

	/* 'compute' faked zpos values in case HW doesn't expose any */
	drm_backend_create_faked_zpos(b->drm);

	/* A this point we have some idea of whether or not we have a working
	 * cursor plane. */
	if (!device->cursors_are_broken)
		compositor->capabilities |= WESTON_CAP_CURSOR_PLANE;

	loop = wl_display_get_event_loop(compositor->wl_display);
	b->drm_source =
		wl_event_loop_add_fd(loop, b->drm->drm.fd,
				     WL_EVENT_READABLE, on_drm_input, b->drm);

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

	if (compositor->renderer->import_dmabuf) {
		if (linux_dmabuf_setup(compositor) < 0)
			weston_log("Error: initializing dmabuf "
				   "support failed.\n");
		if (compositor->default_dmabuf_feedback) {
			/* We were able to create the compositor's default
			 * dma-buf feedback in the renderer, that means that the
			 * table was already created and populated with
			 * renderer's format/modifier pairs. So now we must
			 * compute the scanout formats indices in the table */
			scanout_formats = get_scanout_formats(b->drm);
			if (!scanout_formats)
				goto err_udev_monitor;
			ret = weston_dmabuf_feedback_format_table_set_scanout_indices(compositor->dmabuf_feedback_format_table,
										      scanout_formats);
			weston_drm_format_array_fini(scanout_formats);
			free(scanout_formats);
			if (ret < 0)
				goto err_udev_monitor;
		}
		if (weston_direct_display_setup(compositor) < 0)
			weston_log("Error: initializing direct-display "
				   "support failed.\n");
	}

	if (compositor->capabilities & WESTON_CAP_EXPLICIT_SYNC) {
		if (linux_explicit_synchronization_setup(compositor) < 0)
			weston_log("Error: initializing explicit "
				   " synchronization support failed.\n");
	}

	if (device->atomic_modeset)
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
	destroy_sprites(b->drm);
err_create_crtc_list:
	drmModeFreeResources(res);
err_udev_dev:
	udev_device_unref(drm_device);
err_udev:
	udev_unref(b->udev);
err_launcher:
	weston_launcher_destroy(compositor->launcher);
err_compositor:
	weston_compositor_shutdown(compositor);
#ifdef BUILD_DRM_GBM
	if (b->gbm)
		gbm_device_destroy(b->gbm);
#endif
	free(b);
	return NULL;
}

static void
config_init_to_defaults(struct weston_drm_backend_config *config)
{
	config->renderer = WESTON_RENDERER_AUTO;
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
