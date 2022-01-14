/*
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2012 Intel Corporation
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
#include <drm_fourcc.h>

#include <libweston/libweston.h>
#include <libweston/backend-headless.h>
#include "shared/helpers.h"
#include "linux-explicit-synchronization.h"
#include "pixman-renderer.h"
#include "renderer-gl/gl-renderer.h"
#include "shared/weston-egl-ext.h"
#include "linux-dmabuf.h"
#include "presentation-time-server-protocol.h"
#include <libweston/windowed-output-api.h>

enum headless_renderer_type {
	HEADLESS_NOOP,
	HEADLESS_PIXMAN,
	HEADLESS_GL,
};

struct headless_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	struct weston_seat fake_seat;
	enum headless_renderer_type renderer_type;

	struct gl_renderer_interface *glri;
};

struct headless_head {
	struct weston_head base;
};

struct headless_output {
	struct weston_output base;

	struct weston_mode mode;
	struct wl_event_source *finish_frame_timer;
	uint32_t *image_buf;
	pixman_image_t *image;
};

static const uint32_t headless_formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
};

static inline struct headless_head *
to_headless_head(struct weston_head *base)
{
	return container_of(base, struct headless_head, base);
}

static inline struct headless_output *
to_headless_output(struct weston_output *base)
{
	return container_of(base, struct headless_output, base);
}

static inline struct headless_backend *
to_headless_backend(struct weston_compositor *base)
{
	return container_of(base->backend, struct headless_backend, base);
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

static int
headless_output_repaint(struct weston_output *output_base,
		       pixman_region32_t *damage,
		       void *repaint_data)
{
	struct headless_output *output = to_headless_output(output_base);
	struct weston_compositor *ec = output->base.compositor;

	ec->renderer->repaint_output(&output->base, damage);

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);

	wl_event_source_timer_update(output->finish_frame_timer, 16);

	return 0;
}

static void
headless_output_disable_gl(struct headless_output *output)
{
	struct weston_compositor *compositor = output->base.compositor;
	struct headless_backend *b = to_headless_backend(compositor);

	b->glri->output_destroy(&output->base);
}

static void
headless_output_disable_pixman(struct headless_output *output)
{
	pixman_renderer_output_destroy(&output->base);
	pixman_image_unref(output->image);
	free(output->image_buf);
}

static int
headless_output_disable(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);
	struct headless_backend *b = to_headless_backend(base->compositor);

	if (!output->base.enabled)
		return 0;

	wl_event_source_remove(output->finish_frame_timer);

	switch (b->renderer_type) {
	case HEADLESS_GL:
		headless_output_disable_gl(output);
		break;
	case HEADLESS_PIXMAN:
		headless_output_disable_pixman(output);
		break;
	case HEADLESS_NOOP:
		break;
	}

	return 0;
}

static void
headless_output_destroy(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);

	headless_output_disable(&output->base);
	weston_output_release(&output->base);

	free(output);
}

static int
headless_output_enable_gl(struct headless_output *output)
{
	struct weston_compositor *compositor = output->base.compositor;
	struct headless_backend *b = to_headless_backend(compositor);
	const struct gl_renderer_pbuffer_options options = {
		.width = output->base.current_mode->width,
		.height = output->base.current_mode->height,
		.drm_formats = headless_formats,
		.drm_formats_count = ARRAY_LENGTH(headless_formats),
	};

	if (b->glri->output_pbuffer_create(&output->base, &options) < 0) {
		weston_log("failed to create gl renderer output state\n");
		return -1;
	}

	return 0;
}

static int
headless_output_enable_pixman(struct headless_output *output)
{
	const struct pixman_renderer_output_options options = {
		.use_shadow = true,
	};

	output->image_buf = malloc(output->base.current_mode->width *
				   output->base.current_mode->height * 4);
	if (!output->image_buf)
		return -1;

	output->image = pixman_image_create_bits(PIXMAN_x8r8g8b8,
						 output->base.current_mode->width,
						 output->base.current_mode->height,
						 output->image_buf,
						 output->base.current_mode->width * 4);

	if (pixman_renderer_output_create(&output->base, &options) < 0)
		goto err_renderer;

	pixman_renderer_output_set_buffer(&output->base, output->image);

	return 0;

err_renderer:
	pixman_image_unref(output->image);
	free(output->image_buf);

	return -1;
}

static int
headless_output_enable(struct weston_output *base)
{
	struct headless_output *output = to_headless_output(base);
	struct headless_backend *b = to_headless_backend(base->compositor);
	struct wl_event_loop *loop;
	int ret = 0;

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, finish_frame_handler, output);

	switch (b->renderer_type) {
	case HEADLESS_GL:
		ret = headless_output_enable_gl(output);
		break;
	case HEADLESS_PIXMAN:
		ret = headless_output_enable_pixman(output);
		break;
	case HEADLESS_NOOP:
		break;
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
headless_output_create(struct weston_compositor *compositor, const char *name)
{
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

	weston_compositor_add_pending_output(&output->base, compositor);

	return &output->base;
}

static int
headless_head_create(struct weston_compositor *compositor,
		     const char *name)
{
	struct headless_head *head;

	/* name can't be NULL. */
	assert(name);

	head = zalloc(sizeof *head);
	if (head == NULL)
		return -1;

	weston_head_init(&head->base, name);
	weston_head_set_connection_status(&head->base, true);

	/* Ideally all attributes of the head would be set here, so that the
	 * user has all the information when deciding to create outputs.
	 * We do not have those until set_size() time through.
	 */

	weston_compositor_add_head(compositor, &head->base);

	return 0;
}

static void
headless_head_destroy(struct headless_head *head)
{
	weston_head_release(&head->base);
	free(head);
}

static void
headless_destroy(struct weston_compositor *ec)
{
	struct headless_backend *b = to_headless_backend(ec);
	struct weston_head *base, *next;

	weston_compositor_shutdown(ec);

	wl_list_for_each_safe(base, next, &ec->head_list, compositor_link)
		headless_head_destroy(to_headless_head(base));

	free(b);
}

static int
headless_gl_renderer_init(struct headless_backend *b)
{
	const struct gl_renderer_display_options options = {
		.egl_platform = EGL_PLATFORM_SURFACELESS_MESA,
		.egl_native_display = EGL_DEFAULT_DISPLAY,
		.egl_surface_type = EGL_PBUFFER_BIT,
		.drm_formats = headless_formats,
		.drm_formats_count = ARRAY_LENGTH(headless_formats),
	};

	b->glri = weston_load_module("gl-renderer.so", "gl_renderer_interface");
	if (!b->glri)
		return -1;

	return b->glri->display_create(b->compositor, &options);
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

	if (config->use_pixman && config->use_gl) {
		weston_log("Error: cannot use both Pixman *and* GL renderers.\n");
		goto err_free;
	}

	if (config->use_gl)
		b->renderer_type = HEADLESS_GL;
	else if (config->use_pixman)
		b->renderer_type = HEADLESS_PIXMAN;
	else
		b->renderer_type = HEADLESS_NOOP;

	switch (b->renderer_type) {
	case HEADLESS_GL:
		ret = headless_gl_renderer_init(b);
		break;
	case HEADLESS_PIXMAN:
		ret = pixman_renderer_init(compositor);
		break;
	case HEADLESS_NOOP:
		ret = noop_renderer_init(compositor);
		break;
	default:
		assert(0 && "invalid renderer type");
		ret = -1;
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
