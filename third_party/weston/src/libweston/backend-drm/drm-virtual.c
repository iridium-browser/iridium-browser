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

#include "drm-internal.h"
#include "renderer-gl/gl-renderer.h"

/**
 * Create a drm_plane for virtual output
 *
 * Call drm_virtual_plane_destroy to clean up the plane.
 *
 * @param b DRM compositor backend
 * @param output Output to create internal plane for
 */
static struct drm_plane *
drm_virtual_plane_create(struct drm_backend *b, struct drm_output *output)
{
	struct drm_plane *plane;

	/* num of formats is one */
	plane = zalloc(sizeof(*plane) + sizeof(plane->formats[0]));
	if (!plane) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	plane->type = WDRM_PLANE_TYPE_PRIMARY;
	plane->backend = b;
	plane->state_cur = drm_plane_state_alloc(NULL, plane);
	plane->state_cur->complete = true;
	plane->formats[0].format = output->gbm_format;
	plane->count_formats = 1;
	if ((output->gbm_bo_flags & GBM_BO_USE_LINEAR) && b->fb_modifiers) {
		uint64_t *modifiers = zalloc(sizeof *modifiers);
		if (modifiers) {
			*modifiers = DRM_FORMAT_MOD_LINEAR;
			plane->formats[0].modifiers = modifiers;
			plane->formats[0].count_modifiers = 1;
		}
	}

	weston_plane_init(&plane->base, b->compositor, 0, 0);
	wl_list_insert(&b->plane_list, &plane->link);

	return plane;
}

/**
 * Destroy one DRM plane
 *
 * @param plane Plane to deallocate (will be freed)
 */
static void
drm_virtual_plane_destroy(struct drm_plane *plane)
{
	drm_plane_state_free(plane->state_cur, true);
	weston_plane_release(&plane->base);
	wl_list_remove(&plane->link);
	if (plane->formats[0].modifiers)
		free(plane->formats[0].modifiers);
	free(plane);
}

static int
drm_virtual_output_start_repaint_loop(struct weston_output *output_base)
{
	weston_output_finish_frame(output_base, NULL,
				   WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
drm_virtual_output_submit_frame(struct drm_output *output,
				struct drm_fb *fb)
{
	struct drm_backend *b = to_drm_backend(output->base.compositor);
	int fd, ret;

	assert(fb->num_planes == 1);
	ret = drmPrimeHandleToFD(b->drm.fd, fb->handles[0], DRM_CLOEXEC, &fd);
	if (ret) {
		weston_log("drmPrimeHandleFD failed, errno=%d\n", errno);
		return -1;
	}

	drm_fb_ref(fb);
	ret = output->virtual_submit_frame(&output->base, fd, fb->strides[0],
					   fb);
	if (ret < 0) {
		drm_fb_unref(fb);
		close(fd);
	}
	return ret;
}

static int
drm_virtual_output_repaint(struct weston_output *output_base,
			   pixman_region32_t *damage,
			   void *repaint_data)
{
	struct drm_pending_state *pending_state = repaint_data;
	struct drm_output_state *state = NULL;
	struct drm_output *output = to_drm_output(output_base);
	struct drm_plane *scanout_plane = output->scanout_plane;
	struct drm_plane_state *scanout_state;

	assert(output->virtual);

	if (output->disable_pending || output->destroy_pending)
		goto err;

	/* Drop frame if there isn't free buffers */
	if (!gbm_surface_has_free_buffers(output->gbm_surface)) {
		weston_log("%s: Drop frame!!\n", __func__);
		return -1;
	}

	assert(!output->state_last);

	/* If planes have been disabled in the core, we might not have
	 * hit assign_planes at all, so might not have valid output state
	 * here. */
	state = drm_pending_state_get_output(pending_state, output);
	if (!state)
		state = drm_output_state_duplicate(output->state_cur,
						   pending_state,
						   DRM_OUTPUT_STATE_CLEAR_PLANES);

	drm_output_render(state, damage);
	scanout_state = drm_output_state_get_plane(state, scanout_plane);
	if (!scanout_state || !scanout_state->fb)
		goto err;

	if (drm_virtual_output_submit_frame(output, scanout_state->fb) < 0)
		goto err;

	return 0;

err:
	drm_output_state_free(state);
	return -1;
}

static void
drm_virtual_output_deinit(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);

	drm_output_fini_egl(output);

	drm_virtual_plane_destroy(output->scanout_plane);
}

static void
drm_virtual_output_destroy(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);

	assert(output->virtual);

	if (output->base.enabled)
		drm_virtual_output_deinit(&output->base);

	weston_output_release(&output->base);

	drm_output_state_free(output->state_cur);

	free(output);
}

static int
drm_virtual_output_enable(struct weston_output *output_base)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_backend *b = to_drm_backend(output_base->compositor);

	assert(output->virtual);

	if (b->use_pixman) {
		weston_log("Not support pixman renderer on Virtual output\n");
		goto err;
	}

	if (!output->virtual_submit_frame) {
		weston_log("The virtual_submit_frame hook is not set\n");
		goto err;
	}

	output->scanout_plane = drm_virtual_plane_create(b, output);
	if (!output->scanout_plane) {
		weston_log("Failed to find primary plane for output %s\n",
			   output->base.name);
		return -1;
	}

	if (drm_output_init_egl(output, b) < 0) {
		weston_log("Failed to init output gl state\n");
		goto err;
	}

	output->base.start_repaint_loop = drm_virtual_output_start_repaint_loop;
	output->base.repaint = drm_virtual_output_repaint;
	output->base.assign_planes = drm_assign_planes;
	output->base.set_dpms = NULL;
	output->base.switch_mode = NULL;
	output->base.gamma_size = 0;
	output->base.set_gamma = NULL;

	weston_compositor_stack_plane(b->compositor,
				      &output->scanout_plane->base,
				      &b->compositor->primary_plane);

	return 0;
err:
	return -1;
}

static int
drm_virtual_output_disable(struct weston_output *base)
{
	struct drm_output *output = to_drm_output(base);

	assert(output->virtual);

	if (output->base.enabled)
		drm_virtual_output_deinit(&output->base);

	return 0;
}

static struct weston_output *
drm_virtual_output_create(struct weston_compositor *c, char *name)
{
	struct drm_output *output;

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	output->virtual = true;
	output->gbm_bo_flags = GBM_BO_USE_LINEAR | GBM_BO_USE_RENDERING;

	weston_output_init(&output->base, c, name);

	output->base.enable = drm_virtual_output_enable;
	output->base.destroy = drm_virtual_output_destroy;
	output->base.disable = drm_virtual_output_disable;
	output->base.attach_head = NULL;

	output->state_cur = drm_output_state_alloc(output, NULL);

	weston_compositor_add_pending_output(&output->base, c);

	return &output->base;
}

static uint32_t
drm_virtual_output_set_gbm_format(struct weston_output *base,
				  const char *gbm_format)
{
	struct drm_output *output = to_drm_output(base);
	struct drm_backend *b = to_drm_backend(base->compositor);

	if (parse_gbm_format(gbm_format, b->gbm_format, &output->gbm_format) == -1)
		output->gbm_format = b->gbm_format;

	return output->gbm_format;
}

static void
drm_virtual_output_set_submit_frame_cb(struct weston_output *output_base,
				       submit_frame_cb cb)
{
	struct drm_output *output = to_drm_output(output_base);

	output->virtual_submit_frame = cb;
}

static int
drm_virtual_output_get_fence_fd(struct weston_output *output_base)
{
	return gl_renderer->create_fence_fd(output_base);
}

static void
drm_virtual_output_buffer_released(struct drm_fb *fb)
{
	drm_fb_unref(fb);
}

static void
drm_virtual_output_finish_frame(struct weston_output *output_base,
				struct timespec *stamp,
				uint32_t presented_flags)
{
	struct drm_output *output = to_drm_output(output_base);
	struct drm_plane_state *ps;

	wl_list_for_each(ps, &output->state_cur->plane_list, link)
		ps->complete = true;

	drm_output_state_free(output->state_last);
	output->state_last = NULL;

	weston_output_finish_frame(&output->base, stamp, presented_flags);

	/* We can't call this from frame_notify, because the output's
	 * repaint needed flag is cleared just after that */
	if (output->recorder)
		weston_output_schedule_repaint(&output->base);
}

static const struct weston_drm_virtual_output_api virt_api = {
	drm_virtual_output_create,
	drm_virtual_output_set_gbm_format,
	drm_virtual_output_set_submit_frame_cb,
	drm_virtual_output_get_fence_fd,
	drm_virtual_output_buffer_released,
	drm_virtual_output_finish_frame
};

int drm_backend_init_virtual_output_api(struct weston_compositor *compositor)
{
	return weston_plugin_api_register(compositor,
					  WESTON_DRM_VIRTUAL_OUTPUT_API_NAME,
					  &virt_api, sizeof(virt_api));
}
