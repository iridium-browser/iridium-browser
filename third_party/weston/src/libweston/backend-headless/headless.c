/*
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Jason Ekstrand
 * Copyright 2022 Collabora, Ltd.
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>

#include <libweston/libweston.h>
#include <libweston/backend-headless.h>
#include "shared/helpers.h"
#include "linux-explicit-synchronization.h"
#include "pixel-formats.h"
#include "pixman-renderer.h"
#include "renderer-gl/gl-renderer.h"
#include "gl-borders.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/weston-egl-ext.h"
#include "shared/cairo-util.h"
#include "shared/xalloc.h"
#include "linux-dmabuf.h"
#include "output-capture.h"
#include "presentation-time-server-protocol.h"
#include <libweston/windowed-output-api.h>

struct headless_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct weston_seat fake_seat;

	bool decorate;
	struct theme *theme;

	const struct pixel_format_info **formats;
	unsigned int formats_count;
};

struct headless_head {
	struct weston_head base;
};

struct headless_output {
	struct weston_output base;
	struct headless_backend *backend;

	struct weston_mode mode;
	struct wl_event_source *finish_frame_timer;
	struct weston_renderbuffer *renderbuffer;

	struct frame *frame;
	struct {
		struct weston_gl_borders borders;
	} gl;
};

static const uint32_t headless_formats[] = {
	DRM_FORMAT_XRGB8888, /* default for pixman-renderer */
	DRM_FORMAT_ARGB8888,
};

static void
headless_destroy(struct weston_backend *backend);

static inline struct headless_head *
to_headless_head(struct weston_head *base)
{
	if (base->backend->destroy != headless_destroy)
		return NULL;
	return container_of(base, struct headless_head, base);
}

static void
headless_output_destroy(struct weston_output *base);

static inline struct headless_output *
to_headless_output(struct weston_output *base)
{
	if (base->destroy != headless_output_destroy)
		return NULL;
	return container_of(base, struct headless_output, base);
}

static inline struct headless_backend *
to_headless_backend(struct weston_backend *base)
{
	return container_of(base, struct headless_backend, base);
}

static int
headless_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
finish_frame_handler(void *data)
{
	struct headless_output *output = data;
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->base.compositor, &ts);
	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static void
headless_output_update_gl_border(struct headless_output *output)
{
	if (!output->frame)
		return;
	if (!(frame_status(output->frame) & FRAME_STATUS_REPAINT))
		return;

	weston_gl_borders_update(&output->gl.borders, output->frame,
				 &output->base);
}

static int
headless_output_repaint(struct weston_output *output_base,
		       pixman_region32_t *damage)
{
	struct headless_output *output = to_headless_output(output_base);
	struct weston_compositor *ec;

	assert(output);

	ec = output->base.compositor;

	headless_output_update_gl_border(output);

	ec->renderer->repaint_output(&output->base, damage,
				     output->renderbuffer);

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);

	wl_event_source_timer_update(output->finish_frame_timer, 16);

	return 0;
}

static void
headless_output_disable_gl(struct headless_output *output)
{
	struct weston_compositor *compositor = output->base.compositor;
	const struct weston_renderer *renderer = compositor->renderer;

	weston_gl_borders_fini(&output->gl.borders, &output->base);

	renderer->gl->output_destroy(&output->base);

	if (output->frame) {
		frame_destroy(output->frame);
		output->frame = NULL;
	}
}

static void
headless_output_disable_pixman(struct headless_output *output)
{
	struct weston_renderer *renderer = output->base.compositor->renderer;

	weston_renderbuffer_unref(output->renderbuffer);
	output->renderbuffer = NULL;
	renderer->pixman->output_destroy(&output->base);
}

static int
headless_output_disable(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);
	struct headless_backend *b;

	assert(output);

	if (!output->base.enabled)
		return 0;

	b = output->backend;

	wl_event_source_remove(output->finish_frame_timer);

	switch (b->compositor->renderer->type) {
	case WESTON_RENDERER_GL:
		headless_output_disable_gl(output);
		break;
	case WESTON_RENDERER_PIXMAN:
		headless_output_disable_pixman(output);
		break;
	case WESTON_RENDERER_NOOP:
		break;
	case WESTON_RENDERER_AUTO:
		unreachable("cannot have auto renderer at runtime");
	}

	return 0;
}

static void
headless_output_destroy(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);

	assert(output);

	headless_output_disable(&output->base);
	weston_output_release(&output->base);

	assert(!output->frame);
	free(output);
}

static int
headless_output_enable_gl(struct headless_output *output)
{
	struct headless_backend *b = output->backend;
	const struct weston_renderer *renderer = b->compositor->renderer;
	const struct weston_mode *mode = output->base.current_mode;
	struct gl_renderer_pbuffer_options options = {
		.formats = b->formats,
		.formats_count = b->formats_count,
	};

	if (b->decorate) {
		/*
		 * Start with a dummy exterior size and then resize, because
		 * there is no frame_create() with interior size.
		 */
		output->frame = frame_create(b->theme, 100, 100,
					     FRAME_BUTTON_CLOSE, NULL, NULL);
		if (!output->frame) {
			weston_log("failed to create frame for output\n");
			return -1;
		}
		frame_resize_inside(output->frame, mode->width, mode->height);

		options.fb_size.width = frame_width(output->frame);
		options.fb_size.height = frame_height(output->frame);
		frame_interior(output->frame, &options.area.x, &options.area.y,
			       &options.area.width, &options.area.height);
	} else {
		options.area.x = 0;
		options.area.y = 0;
		options.area.width = mode->width;
		options.area.height = mode->height;
		options.fb_size.width = mode->width;
		options.fb_size.height = mode->height;
	}

	if (renderer->gl->output_pbuffer_create(&output->base, &options) < 0) {
		weston_log("failed to create gl renderer output state\n");
		if (output->frame) {
			frame_destroy(output->frame);
			output->frame = NULL;
		}
		return -1;
	}

	return 0;
}

static int
headless_output_enable_pixman(struct headless_output *output)
{
	const struct pixman_renderer_interface *pixman;
	const struct pixman_renderer_output_options options = {
		.use_shadow = true,
		.fb_size = {
			.width = output->base.current_mode->width,
			.height = output->base.current_mode->height
		},
		.format = pixel_format_get_info(headless_formats[0])
	};

	pixman = output->base.compositor->renderer->pixman;

	if (pixman->output_create(&output->base, &options) < 0)
		return -1;

	output->renderbuffer =
		pixman->create_image(&output->base, options.format,
				     output->base.current_mode->width,
				     output->base.current_mode->height);
	if (!output->renderbuffer)
		goto err_renderer;

	return 0;

err_renderer:
	pixman->output_destroy(&output->base);

	return -1;
}

static int
headless_output_enable(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);
	struct headless_backend *b;
	struct wl_event_loop *loop;
	int ret = 0;

	assert(output);

	b = output->backend;

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, finish_frame_handler, output);

	if (output->finish_frame_timer == NULL) {
		weston_log("failed to add finish frame timer\n");
		return -1;
	}

	switch (b->compositor->renderer->type) {
	case WESTON_RENDERER_GL:
		ret = headless_output_enable_gl(output);
		break;
	case WESTON_RENDERER_PIXMAN:
		ret = headless_output_enable_pixman(output);
		break;
	case WESTON_RENDERER_NOOP:
		break;
	case WESTON_RENDERER_AUTO:
		unreachable("cannot have auto renderer at runtime");
	}

	if (ret < 0) {
		wl_event_source_remove(output->finish_frame_timer);
		return -1;
	}

	return 0;
}

static int
headless_output_set_size(struct weston_output *base,
			 int width, int height)
{
	struct headless_output *output = to_headless_output(base);
	struct weston_head *head;
	int output_width, output_height;

	if (!output)
		return -1;

	/* We can only be called once. */
	assert(!output->base.current_mode);

	/* Make sure we have scale set. */
	assert(output->base.scale);

	wl_list_for_each(head, &output->base.head_list, output_link) {
		weston_head_set_monitor_strings(head, "weston", "headless",
						NULL);

		/* XXX: Calculate proper size. */
		weston_head_set_physical_size(head, width, height);
	}

	output_width = width * output->base.scale;
	output_height = height * output->base.scale;

	output->mode.flags =
		WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
	output->mode.width = output_width;
	output->mode.height = output_height;
	output->mode.refresh = 60000;
	wl_list_insert(&output->base.mode_list, &output->mode.link);

	output->base.current_mode = &output->mode;

	output->base.start_repaint_loop = headless_output_start_repaint_loop;
	output->base.repaint = headless_output_repaint;
	output->base.assign_planes = NULL;
	output->base.set_backlight = NULL;
	output->base.set_dpms = NULL;
	output->base.switch_mode = NULL;

	return 0;
}

static struct weston_output *
headless_output_create(struct weston_backend *backend, const char *name)
{
	struct headless_backend *b = container_of(backend, struct headless_backend, base);
	struct weston_compositor *compositor = b->compositor;
	struct headless_output *output;

	/* name can't be NULL. */
	assert(name);

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	weston_output_init(&output->base, compositor, name);

	output->base.destroy = headless_output_destroy;
	output->base.disable = headless_output_disable;
	output->base.enable = headless_output_enable;
	output->base.attach_head = NULL;

	output->backend = b;

	weston_compositor_add_pending_output(&output->base, compositor);

	return &output->base;
}

static int
headless_head_create(struct weston_backend *base,
		     const char *name)
{
	struct headless_backend *backend = to_headless_backend(base);
	struct headless_head *head;

	/* name can't be NULL. */
	assert(name);

	head = zalloc(sizeof *head);
	if (head == NULL)
		return -1;

	weston_head_init(&head->base, name);

	head->base.backend = &backend->base;

	weston_head_set_connection_status(&head->base, true);
	weston_head_set_supported_eotf_mask(&head->base,
					    WESTON_EOTF_MODE_ALL_MASK);

	/* Ideally all attributes of the head would be set here, so that the
	 * user has all the information when deciding to create outputs.
	 * We do not have those until set_size() time through.
	 */

	weston_compositor_add_head(backend->compositor, &head->base);

	return 0;
}

static void
headless_head_destroy(struct weston_head *base)
{
	struct headless_head *head = to_headless_head(base);

	assert(head);

	weston_head_release(&head->base);
	free(head);
}

static void
headless_destroy(struct weston_backend *backend)
{
	struct headless_backend *b = container_of(backend, struct headless_backend, base);
	struct weston_compositor *ec = b->compositor;
	struct weston_head *base, *next;

	weston_compositor_shutdown(ec);

	wl_list_for_each_safe(base, next, &ec->head_list, compositor_link) {
		if (to_headless_head(base))
			headless_head_destroy(base);
	}

	if (b->theme)
		theme_destroy(b->theme);

	free(b->formats);
	free(b);

	cleanup_after_cairo();
}

static const struct weston_windowed_output_api api = {
	headless_output_set_size,
	headless_head_create,
};

static struct headless_backend *
headless_backend_create(struct weston_compositor *compositor,
			struct weston_headless_backend_config *config)
{
	struct headless_backend *b;
	int ret;

	b = zalloc(sizeof *b);
	if (b == NULL)
		return NULL;

	b->compositor = compositor;
	compositor->backend = &b->base;

	if (weston_compositor_set_presentation_clock_software(compositor) < 0)
		goto err_free;

	b->base.destroy = headless_destroy;
	b->base.create_output = headless_output_create;

	b->decorate = config->decorate;
	if (b->decorate) {
		b->theme = theme_create();
		if (!b->theme) {
			weston_log("Error: could not load decorations theme.\n");
			goto err_free;
		}
	}

	b->formats_count = ARRAY_LENGTH(headless_formats);
	b->formats = pixel_format_get_array(headless_formats, b->formats_count);

	switch (config->renderer) {
	case WESTON_RENDERER_GL: {
		const struct gl_renderer_display_options options = {
			.egl_platform = EGL_PLATFORM_SURFACELESS_MESA,
			.egl_native_display = NULL,
			.egl_surface_type = EGL_PBUFFER_BIT,
			.formats = b->formats,
			.formats_count = b->formats_count,
		};
		ret = weston_compositor_init_renderer(compositor,
						      WESTON_RENDERER_GL,
						      &options.base);
		break;
	}
	case WESTON_RENDERER_PIXMAN:
		if (config->decorate) {
			weston_log("Error: Pixman renderer does not support decorations.\n");
			goto err_input;
		}
		ret = weston_compositor_init_renderer(compositor,
						      WESTON_RENDERER_PIXMAN,
						      NULL);
		break;
	case WESTON_RENDERER_AUTO:
	case WESTON_RENDERER_NOOP:
		if (config->decorate) {
			weston_log("Error: no-op renderer does not support decorations.\n");
			goto err_input;
		}
		ret = noop_renderer_init(compositor);
		break;
	default:
		weston_log("Error: unsupported renderer\n");
		break;
	}

	if (ret < 0)
		goto err_input;

	if (compositor->renderer->import_dmabuf) {
		if (linux_dmabuf_setup(compositor) < 0) {
			weston_log("Error: dmabuf protocol setup failed.\n");
			goto err_input;
		}
	}

	/* Support zwp_linux_explicit_synchronization_unstable_v1 to enable
	 * testing. */
	if (linux_explicit_synchronization_setup(compositor) < 0)
		goto err_input;

	ret = weston_plugin_api_register(compositor, WESTON_WINDOWED_OUTPUT_API_NAME,
					 &api, sizeof(api));

	if (ret < 0) {
		weston_log("Failed to register output API.\n");
		goto err_input;
	}

	return b;

err_input:
	if (b->theme)
		theme_destroy(b->theme);

	weston_compositor_shutdown(compositor);
err_free:
	free(b);
	return NULL;
}

static void
config_init_to_defaults(struct weston_headless_backend_config *config)
{
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct headless_backend *b;
	struct weston_headless_backend_config config = {{ 0, }};

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_HEADLESS_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_headless_backend_config)) {
		weston_log("headless backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	b = headless_backend_create(compositor, &config);
	if (b == NULL)
		return -1;

	return 0;
}
