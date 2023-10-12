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
#include <dlfcn.h>

#include "drm-internal.h"
#include "pixman-renderer.h"
#include "pixel-formats.h"
#include "renderer-gl/gl-renderer.h"
#include "shared/weston-egl-ext.h"
#include "linux-dmabuf.h"
#include "linux-explicit-synchronization.h"

static struct gbm_device *
create_gbm_device(int fd)
{
	struct gbm_device *gbm;

	/* GBM will load a dri driver, but even though they need symbols from
	 * libglapi, in some version of Mesa they are not linked to it. Since
	 * only the gl-renderer module links to it, the call above won't make
	 * these symbols globally available, and loading the DRI driver fails.
	 * Workaround this by dlopen()'ing libglapi with RTLD_GLOBAL. */
	dlopen("libglapi.so.0", RTLD_LAZY | RTLD_GLOBAL);

	gbm = gbm_create_device(fd);

	return gbm;
}

/* When initializing EGL, if the preferred buffer format isn't available
 * we may be able to substitute an ARGB format for an XRGB one.
 *
 * This returns NULL if substitution isn't possible. The caller is responsible
 * for checking for NULL before calling gl_renderer->create().
 *
 * This works around https://bugs.freedesktop.org/show_bug.cgi?id=89689
 * but it's entirely possible we'll see this again on other implementations.
 */
static const struct pixel_format_info *
fallback_format_for(const struct pixel_format_info *format)
{
	return pixel_format_get_info_by_opaque_substitute(format->format);
}

static int
drm_backend_create_gl_renderer(struct drm_backend *b)
{
	const struct pixel_format_info *format[3] = {
		b->format,
		fallback_format_for(b->format),
		NULL,
	};
	struct gl_renderer_display_options options = {
		.egl_platform = EGL_PLATFORM_GBM_KHR,
		.egl_native_display = b->gbm,
		.egl_surface_type = EGL_WINDOW_BIT,
		.formats = format,
		.formats_count = 2,
	};

	if (format[1])
		options.formats_count = 3;

	return weston_compositor_init_renderer(b->compositor,
					       WESTON_RENDERER_GL,
					       &options.base);
}

int
init_egl(struct drm_backend *b)
{
	struct drm_device *device = b->drm;

	b->gbm = create_gbm_device(device->drm.fd);
	if (!b->gbm)
		return -1;

	if (drm_backend_create_gl_renderer(b) < 0) {
		gbm_device_destroy(b->gbm);
		b->gbm = NULL;
		return -1;
	}

	return 0;
}

static void drm_output_fini_cursor_egl(struct drm_output *output)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(output->gbm_cursor_fb); i++) {
		/* This cursor does not have a GBM device */
		if (output->gbm_cursor_fb[i] && !output->gbm_cursor_fb[i]->bo)
			output->gbm_cursor_fb[i]->type = BUFFER_PIXMAN_DUMB;
		drm_fb_unref(output->gbm_cursor_fb[i]);
		output->gbm_cursor_fb[i] = NULL;
	}
}

static int
drm_output_init_cursor_egl(struct drm_output *output, struct drm_backend *b)
{
	struct drm_device *device = output->device;
	unsigned int i;

	/* No point creating cursors if we don't have a plane for them. */
	if (!output->cursor_plane)
		return 0;

	for (i = 0; i < ARRAY_LENGTH(output->gbm_cursor_fb); i++) {
		struct gbm_bo *bo;

		if (gbm_device_get_fd(b->gbm) != output->device->drm.fd) {
			output->gbm_cursor_fb[i] =
				drm_fb_create_dumb(output->device,
						   device->cursor_width,
						   device->cursor_height,
						   DRM_FORMAT_ARGB8888);
			/* Override buffer type, since we know it is a cursor */
			output->gbm_cursor_fb[i]->type = BUFFER_CURSOR;
			output->gbm_cursor_handle[i] =
				output->gbm_cursor_fb[i]->handles[0];
		} else {
			bo = gbm_bo_create(b->gbm, device->cursor_width, device->cursor_height,
					   GBM_FORMAT_ARGB8888,
					   GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE);
			if (!bo)
				goto err;

			output->gbm_cursor_fb[i] =
				drm_fb_get_from_bo(bo, device, false, BUFFER_CURSOR);
			if (!output->gbm_cursor_fb[i]) {
				gbm_bo_destroy(bo);
				goto err;
			}
			output->gbm_cursor_handle[i] = gbm_bo_get_handle(bo).s32;
		}
	}

	return 0;

err:
	weston_log("cursor buffers unavailable, using gl cursors\n");
	device->cursors_are_broken = true;
	drm_output_fini_cursor_egl(output);
	return -1;
}

static void
create_gbm_surface(struct gbm_device *gbm, struct drm_output *output)
{
	struct weston_mode *mode = output->base.current_mode;
	struct drm_plane *plane = output->scanout_plane;
	struct weston_drm_format *fmt;
	const uint64_t *modifiers;
	unsigned int num_modifiers;

	fmt = weston_drm_format_array_find_format(&plane->formats,
						  output->format->format);
	if (!fmt) {
		weston_log("format %s not supported by output %s\n",
			   output->format->drm_format_name,
			   output->base.name);
		return;
	}

#ifdef HAVE_GBM_MODIFIERS
	if (!weston_drm_format_has_modifier(fmt, DRM_FORMAT_MOD_INVALID)) {
		modifiers = weston_drm_format_get_modifiers(fmt, &num_modifiers);
		output->gbm_surface =
			gbm_surface_create_with_modifiers(gbm,
							  mode->width, mode->height,
							  output->format->format,
							  modifiers, num_modifiers);
	}
#endif

	/*
	 * If we cannot use modifiers to allocate the GBM surface and the GBM
	 * device differs from the KMS display device (because we are rendering
	 * on a different GPU), we have to use linear buffers to make sure that
	 * the allocated GBM surface is correctly displayed on the KMS device.
	 */
	if (gbm_device_get_fd(gbm) != output->device->drm.fd)
		output->gbm_bo_flags |= GBM_BO_USE_LINEAR;

	/* We may allocate with no modifiers in the following situations:
	 *
	 * 1. old GBM version, so HAVE_GBM_MODIFIERS is false;
	 * 2. the KMS driver does not support modifiers;
	 * 3. if allocating with modifiers failed, what can happen when the KMS
	 *    display device supports modifiers but the GBM driver does not,
	 *    e.g. the old i915 Mesa driver.
	 */
	if (!output->gbm_surface)
		output->gbm_surface = gbm_surface_create(gbm,
							 mode->width, mode->height,
							 output->format->format,
							 output->gbm_bo_flags);
}

/* Init output state that depends on gl or gbm */
int
drm_output_init_egl(struct drm_output *output, struct drm_backend *b)
{
	const struct weston_renderer *renderer = b->compositor->renderer;
	const struct weston_mode *mode = output->base.current_mode;
	const struct pixel_format_info *format[2] = {
		output->format,
		fallback_format_for(output->format),
	};
	struct gl_renderer_output_options options = {
		.formats = format,
		.formats_count = 1,
		.area.x = 0,
		.area.y = 0,
		.area.width = mode->width,
		.area.height = mode->height,
		.fb_size.width = mode->width,
		.fb_size.height = mode->height,
	};

	assert(output->gbm_surface == NULL);
	create_gbm_surface(b->gbm, output);
	if (!output->gbm_surface) {
		weston_log("failed to create gbm surface\n");
		return -1;
	}

	if (options.formats[1])
		options.formats_count = 2;
	options.window_for_legacy = (EGLNativeWindowType) output->gbm_surface;
	options.window_for_platform = output->gbm_surface;
	if (renderer->gl->output_window_create(&output->base, &options) < 0) {
		weston_log("failed to create gl renderer output state\n");
		gbm_surface_destroy(output->gbm_surface);
		output->gbm_surface = NULL;
		return -1;
	}

	drm_output_init_cursor_egl(output, b);

	return 0;
}

void
drm_output_fini_egl(struct drm_output *output)
{
	struct drm_backend *b = output->backend;
	const struct weston_renderer *renderer = b->compositor->renderer;

	/* Destroying the GBM surface will destroy all our GBM buffers,
	 * regardless of refcount. Ensure we destroy them here. */
	if (!b->shutting_down &&
	    output->scanout_plane->state_cur->fb &&
	    output->scanout_plane->state_cur->fb->type == BUFFER_GBM_SURFACE) {
		drm_plane_reset_state(output->scanout_plane);
	}

	renderer->gl->output_destroy(&output->base);
	gbm_surface_destroy(output->gbm_surface);
	output->gbm_surface = NULL;
	drm_output_fini_cursor_egl(output);
}

struct drm_fb *
drm_output_render_gl(struct drm_output_state *state, pixman_region32_t *damage)
{
	struct drm_output *output = state->output;
	struct drm_device *device = output->device;
	struct gbm_bo *bo;
	struct drm_fb *ret;

	output->base.compositor->renderer->repaint_output(&output->base,
							  damage, NULL);

	bo = gbm_surface_lock_front_buffer(output->gbm_surface);
	if (!bo) {
		weston_log("failed to lock front buffer: %s\n",
			   strerror(errno));
		return NULL;
	}

	/* The renderer always produces an opaque image. */
	ret = drm_fb_get_from_bo(bo, device, true, BUFFER_GBM_SURFACE);
	if (!ret) {
		weston_log("failed to get drm_fb for bo\n");
		gbm_surface_release_buffer(output->gbm_surface, bo);
		return NULL;
	}
	ret->gbm_surface = output->gbm_surface;

	return ret;
}
