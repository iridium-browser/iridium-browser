/*
 * Copyright © 2013 Vasily Khoruzhick <anarsoul@gmail.com>
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

#include <libweston/libweston.h>
#include "backend.h"
#include "libweston-internal.h"
#include "pixman.h"

int
pixman_renderer_init(struct weston_compositor *ec);

struct pixman_renderer_output_options {
	/** Composite into a shadow buffer, copying to the hardware buffer */
	bool use_shadow;
	/** Initial framebuffer size */
	struct weston_size fb_size;
	/** Initial pixel format */
	const struct pixel_format_info *format;
};

struct pixman_renderer_interface {
	int (*output_create)(struct weston_output *output,
			     const struct pixman_renderer_output_options *options);
	void (*output_destroy)(struct weston_output *output);

	struct weston_renderbuffer *(*create_image_from_ptr)(struct weston_output *output,
							     const struct pixel_format_info *format,
							     int width,
							     int height,
							     uint32_t *ptr,
							     int stride);
	struct weston_renderbuffer *(*create_image)(struct weston_output *output,
						    const struct pixel_format_info *format,
						    int width, int height);
	pixman_image_t *(*renderbuffer_get_image)(struct weston_renderbuffer *renderbuffer);
};
