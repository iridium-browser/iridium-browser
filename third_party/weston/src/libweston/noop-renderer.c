/*
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

#include <stdint.h>
#include <stdlib.h>

#include <libweston/libweston.h>
#include "libweston-internal.h"

struct noop_renderer {
	struct weston_renderer base;
	unsigned char seed; /* see comment in attach() */
};

static int
noop_renderer_read_pixels(struct weston_output *output,
			  const struct pixel_format_info *format, void *pixels,
			  uint32_t x, uint32_t y,
			  uint32_t width, uint32_t height)
{
	return 0;
}

static void
noop_renderer_repaint_output(struct weston_output *output,
			     pixman_region32_t *output_damage,
			     struct weston_renderbuffer *renderbuffer)
{
}

static bool
noop_renderer_resize_output(struct weston_output *output,
			    const struct weston_size *fb_size,
			    const struct weston_geometry *area)
{
	check_compositing_area(fb_size, area);
	return true;
}

static void
noop_renderer_flush_damage(struct weston_surface *surface,
			   struct weston_buffer *buffer)
{
}

static void
noop_renderer_attach(struct weston_surface *es, struct weston_buffer *buffer)
{
	struct noop_renderer *renderer =
		wl_container_of(es->compositor->renderer, renderer, base);
	struct wl_shm_buffer *shm_buffer;
	uint8_t *data;
	uint32_t size, i, height, stride;
	unsigned char unused = 0;

	if (!buffer)
		return;

	switch (buffer->type) {
	case WESTON_BUFFER_SOLID:
		/* no-op, early exit */
		return;
	case WESTON_BUFFER_SHM:
		/* fine */
		break;
	default:
		weston_log("No-op renderer supports only SHM buffers\n");
		return;
	}

	shm_buffer = buffer->shm_buffer;
	data = wl_shm_buffer_get_data(shm_buffer);
	stride = wl_shm_buffer_get_stride(shm_buffer);
	height = buffer->height;
	size = stride * height;

	/* Access the buffer data to make sure the buffer's client gets killed
	 * if the buffer size is invalid. This makes the bad_buffer test pass.
	 * This can be removed if we start reading the buffer contents
	 * somewhere else, e.g. in repaint_output(). */
	wl_shm_buffer_begin_access(shm_buffer);
	for (i = 0; i < size; i++)
		unused ^= data[i];
	wl_shm_buffer_end_access(shm_buffer);

	/* Make sure that our unused is actually used, otherwise the compiler
	 * is free to notice that our reads have no effect and elide them. */
	renderer->seed = unused;
}

static void
noop_renderer_destroy(struct weston_compositor *ec)
{
	struct noop_renderer *renderer =
		wl_container_of(ec->renderer, renderer, base);

	weston_log("no-op renderer SHM seed: %d\n", renderer->seed);
	free(ec->renderer);
	ec->renderer = NULL;
}

WL_EXPORT int
noop_renderer_init(struct weston_compositor *ec)
{
	struct noop_renderer *renderer;

	renderer = zalloc(sizeof *renderer);
	if (renderer == NULL)
		return -1;

	renderer->base.read_pixels = noop_renderer_read_pixels;
	renderer->base.repaint_output = noop_renderer_repaint_output;
	renderer->base.resize_output = noop_renderer_resize_output;
	renderer->base.flush_damage = noop_renderer_flush_damage;
	renderer->base.attach = noop_renderer_attach;
	renderer->base.destroy = noop_renderer_destroy;
	renderer->base.type = WESTON_RENDERER_NOOP;
	ec->renderer = &renderer->base;

	return 0;
}
