/*
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

#pragma once

#include <libweston/libweston.h>

/** Copy of weston_capture_v1.source enum from protocol */
enum weston_output_capture_source {
	/** DRM KMS hardware writeback */
	WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK = 0,

	/** framebuffer desktop area */
	WESTON_OUTPUT_CAPTURE_SOURCE_FRAMEBUFFER,

	/** complete framebuffer, including borders if any */
	WESTON_OUTPUT_CAPTURE_SOURCE_FULL_FRAMEBUFFER,

	/** blending buffer, potentially light-linear */
	WESTON_OUTPUT_CAPTURE_SOURCE_BLENDING,
};
#define WESTON_OUTPUT_CAPTURE_SOURCE__COUNT (WESTON_OUTPUT_CAPTURE_SOURCE_BLENDING + 1)

struct weston_output_capture_info;

/*
 * For weston_output core implementation:
 */

struct weston_output_capture_info *
weston_output_capture_info_create(void);

void
weston_output_capture_info_destroy(struct weston_output_capture_info **cip);

void
weston_output_capture_info_repaint_done(struct weston_output_capture_info *ci);

/*
 * For actual capturing implementations (renderers, DRM-backend):
 */

void
weston_output_update_capture_info(struct weston_output *output,
				  enum weston_output_capture_source src,
				  int width, int height,
				  const struct pixel_format_info *format);

bool
weston_output_has_renderer_capture_tasks(struct weston_output *output);

struct weston_capture_task;

struct weston_capture_task *
weston_output_pull_capture_task(struct weston_output *output,
				enum weston_output_capture_source src,
				int width, int height,
				const struct pixel_format_info *format);

struct weston_buffer *
weston_capture_task_get_buffer(struct weston_capture_task *ct);

void
weston_capture_task_retire_failed(struct weston_capture_task *ct,
				  const char *err_msg);

void
weston_capture_task_retire_complete(struct weston_capture_task *ct);


/*
 * entry point for weston_compositor
 */

void
weston_compositor_install_capture_protocol(struct weston_compositor *compositor);
