/*
 * Copyright © 2008-2011 Kristian Høgsberg
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>

#include <libweston/libweston.h>
#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include "backend.h"
#include "libweston-internal.h"
#include "pixel-formats.h"

#include "wcap/wcap-decode.h"

struct screenshooter_frame_listener {
	struct wl_listener frame_listener;
	struct wl_listener buffer_destroy_listener;
	struct weston_buffer *buffer;
	struct weston_output *output;
	weston_screenshooter_done_func_t done;
	void *data;
};

static void
copy_bgra_yflip(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		memcpy(dst, src, stride);
		dst += stride;
		src -= stride;
	}
}

static void
copy_bgra(uint8_t *dst, uint8_t *src, int height, int stride)
{
	/* TODO: optimize this out */
	memcpy(dst, src, height * stride);
}

static void
copy_row_swap_RB(void *vdst, void *vsrc, int bytes)
{
	uint32_t *dst = vdst;
	uint32_t *src = vsrc;
	uint32_t *end = dst + bytes / 4;

	while (dst < end) {
		uint32_t v = *src++;
		/*                    A R G B */
		uint32_t tmp = v & 0xff00ff00;
		tmp |= (v >> 16) & 0x000000ff;
		tmp |= (v << 16) & 0x00ff0000;
		*dst++ = tmp;
	}
}

static void
copy_rgba_yflip(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		copy_row_swap_RB(dst, src, stride);
		dst += stride;
		src -= stride;
	}
}

static void
copy_rgba(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		copy_row_swap_RB(dst, src, stride);
		dst += stride;
		src += stride;
	}
}

static void
screenshooter_frame_notify(struct wl_listener *listener, void *data)
{
	struct screenshooter_frame_listener *l =
		container_of(listener,
			     struct screenshooter_frame_listener,
			     frame_listener);
	struct weston_output *output = l->output;
	struct weston_compositor *compositor = output->compositor;
	const pixman_format_code_t pixman_format =
		compositor->read_format->pixman_format;
	int32_t stride;
	uint8_t *pixels, *d, *s;

	weston_output_disable_planes_decr(output);
	wl_list_remove(&listener->link);
	wl_list_remove(&l->buffer_destroy_listener.link);

	stride = l->buffer->width * (PIXMAN_FORMAT_BPP(pixman_format) / 8);
	pixels = malloc(stride * l->buffer->height);

	if (pixels == NULL) {
		l->done(l->data, WESTON_SCREENSHOOTER_NO_MEMORY);
		free(l);
		return;
	}

	compositor->renderer->read_pixels(output,
			     compositor->read_format, pixels,
			     0, 0, output->current_mode->width,
			     output->current_mode->height);

	stride = wl_shm_buffer_get_stride(l->buffer->shm_buffer);

	d = wl_shm_buffer_get_data(l->buffer->shm_buffer);
	s = pixels + stride * (l->buffer->height - 1);

	wl_shm_buffer_begin_access(l->buffer->shm_buffer);

	switch (pixman_format) {
	case PIXMAN_a8r8g8b8:
	case PIXMAN_x8r8g8b8:
		if (compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP)
			copy_bgra_yflip(d, s, output->current_mode->height, stride);
		else
			copy_bgra(d, pixels, output->current_mode->height, stride);
		break;
	case PIXMAN_x8b8g8r8:
	case PIXMAN_a8b8g8r8:
		if (compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP)
			copy_rgba_yflip(d, s, output->current_mode->height, stride);
		else
			copy_rgba(d, pixels, output->current_mode->height, stride);
		break;
	default:
		break;
	}

	wl_shm_buffer_end_access(l->buffer->shm_buffer);

	l->done(l->data, WESTON_SCREENSHOOTER_SUCCESS);
	free(pixels);
	free(l);
}

static void
buffer_destroy_handle(struct wl_listener *listener, void *data)
{
	struct screenshooter_frame_listener *l =
		container_of(listener,
			     struct screenshooter_frame_listener,
			     buffer_destroy_listener);

	weston_output_disable_planes_decr(l->output);
	wl_list_remove(&listener->link);
	wl_list_remove(&l->frame_listener.link);
	l->done(l->data, WESTON_SCREENSHOOTER_BAD_BUFFER);

	free(l);
}

WL_EXPORT int
weston_screenshooter_shoot(struct weston_output *output,
			   struct weston_buffer *buffer,
			   weston_screenshooter_done_func_t done, void *data)
{
	struct screenshooter_frame_listener *l;

	if (buffer->type != WESTON_BUFFER_SHM) {
		done(data, WESTON_SCREENSHOOTER_BAD_BUFFER);
		return -1;
	}

	if (buffer->width < output->current_mode->width ||
	    buffer->height < output->current_mode->height) {
		done(data, WESTON_SCREENSHOOTER_BAD_BUFFER);
		return -1;
	}

	l = malloc(sizeof *l);
	if (l == NULL) {
		done(data, WESTON_SCREENSHOOTER_NO_MEMORY);
		return -1;
	}

	l->buffer = buffer;
	l->output = output;
	l->done = done;
	l->data = data;

	l->frame_listener.notify = screenshooter_frame_notify;
	wl_signal_add(&output->frame_signal, &l->frame_listener);

	l->buffer_destroy_listener.notify = buffer_destroy_handle;
	wl_signal_add(&buffer->destroy_signal, &l->buffer_destroy_listener);

	weston_output_disable_planes_incr(output);
	weston_output_schedule_repaint(output);

	return 0;
}

struct weston_recorder {
	struct weston_output *output;
	uint32_t *frame, *rect;
	uint32_t *tmpbuf;
	uint32_t total;
	int fd;
	struct wl_listener frame_listener;
	int count, destroying;
};

static uint32_t *
output_run(uint32_t *p, uint32_t delta, int run)
{
	int i;

	while (run > 0) {
		if (run <= 0xe0) {
			*p++ = delta | ((run - 1) << 24);
			break;
		}

		i = 24 - __builtin_clz(run);
		*p++ = delta | ((i + 0xe0) << 24);
		run -= 1 << (7 + i);
	}

	return p;
}

static uint32_t
component_delta(uint32_t next, uint32_t prev)
{
	unsigned char dr, dg, db;

	dr = (next >> 16) - (prev >> 16);
	dg = (next >>  8) - (prev >>  8);
	db = (next >>  0) - (prev >>  0);

	return (dr << 16) | (dg << 8) | (db << 0);
}

static void
weston_recorder_destroy(struct weston_recorder *recorder);

static void
weston_recorder_frame_notify(struct wl_listener *listener, void *data)
{
	struct weston_recorder *recorder =
		container_of(listener, struct weston_recorder, frame_listener);
	struct weston_output *output = recorder->output;
	struct weston_compositor *compositor = output->compositor;
	uint32_t msecs = timespec_to_msec(&output->frame_time);
	pixman_box32_t *r;
	pixman_region32_t damage, transformed_damage;
	int i, j, k, n, width, height, run, stride;
	uint32_t delta, prev, *d, *s, *p, next;
	struct {
		uint32_t msecs;
		uint32_t nrects;
	} header;
	struct iovec v[2];
	int do_yflip;
	int y_orig;
	uint32_t *outbuf;

	do_yflip = !!(compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP);
	if (do_yflip)
		outbuf = recorder->rect;
	else
		outbuf = recorder->tmpbuf;

	pixman_region32_init(&damage);
	pixman_region32_init(&transformed_damage);
	pixman_region32_intersect(&damage, &output->region, data);
	weston_region_global_to_output(&transformed_damage,
				       output,
				       &damage);
	pixman_region32_fini(&damage);

	r = pixman_region32_rectangles(&transformed_damage, &n);
	if (n == 0) {
		pixman_region32_fini(&transformed_damage);
		return;
	}

	header.msecs = msecs;
	header.nrects = n;
	v[0].iov_base = &header;
	v[0].iov_len = sizeof header;
	v[1].iov_base = r;
	v[1].iov_len = n * sizeof *r;
	recorder->total += writev(recorder->fd, v, 2);
	stride = output->current_mode->width;

	for (i = 0; i < n; i++) {
		width = r[i].x2 - r[i].x1;
		height = r[i].y2 - r[i].y1;

		if (do_yflip)
			y_orig = output->current_mode->height - r[i].y2;
		else
			y_orig = r[i].y1;

		compositor->renderer->read_pixels(output,
				compositor->read_format, recorder->rect,
				r[i].x1, y_orig, width, height);

		p = outbuf;
		run = prev = 0; /* quiet gcc */
		for (j = 0; j < height; j++) {
			if (do_yflip)
				s = recorder->rect + width * j;
			else
				s = recorder->rect + width * (height - j - 1);
			y_orig = r[i].y2 - j - 1;
			d = recorder->frame + stride * y_orig + r[i].x1;

			for (k = 0; k < width; k++) {
				next = *s++;
				delta = component_delta(next, *d);
				*d++ = next;
				if (run == 0 || delta == prev) {
					run++;
				} else {
					p = output_run(p, prev, run);
					run = 1;
				}
				prev = delta;
			}
		}

		p = output_run(p, prev, run);

		recorder->total += write(recorder->fd,
					 outbuf, (p - outbuf) * 4);

#if 0
		fprintf(stderr,
			"%dx%d at %d,%d rle from %d to %d bytes (%f) total %dM\n",
			width, height, r[i].x1, r[i].y1,
			width * height * 4, (int) (p - outbuf) * 4,
			(float) (p - outbuf) / (width * height),
			recorder->total / 1024 / 1024);
#endif
	}

	pixman_region32_fini(&transformed_damage);
	recorder->count++;

	if (recorder->destroying)
		weston_recorder_destroy(recorder);
}

static void
weston_recorder_free(struct weston_recorder *recorder)
{
	if (recorder == NULL)
		return;

	free(recorder->tmpbuf);
	free(recorder->rect);
	free(recorder->frame);
	free(recorder);
}

static struct weston_recorder *
weston_recorder_create(struct weston_output *output, const char *filename)
{
	struct weston_compositor *compositor = output->compositor;
	struct weston_recorder *recorder;
	int stride, size;
	struct { uint32_t magic, format, width, height; } header;
	int do_yflip;

	do_yflip = !!(compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP);

	recorder = zalloc(sizeof *recorder);
	if (recorder == NULL) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	stride = output->current_mode->width;
	size = stride * 4 * output->current_mode->height;
	recorder->frame = zalloc(size);
	recorder->rect = malloc(size);
	recorder->output = output;

	if ((recorder->frame == NULL) || (recorder->rect == NULL)) {
		weston_log("%s: out of memory\n", __func__);
		goto err_recorder;
	}

	if (!do_yflip) {
		recorder->tmpbuf = malloc(size);
		if (recorder->tmpbuf == NULL) {
			weston_log("%s: out of memory\n", __func__);
			goto err_recorder;
		}
	}

	header.magic = WCAP_HEADER_MAGIC;

	switch (compositor->read_format->pixman_format) {
	case PIXMAN_x8r8g8b8:
	case PIXMAN_a8r8g8b8:
		header.format = WCAP_FORMAT_XRGB8888;
		break;
	case PIXMAN_a8b8g8r8:
		header.format = WCAP_FORMAT_XBGR8888;
		break;
	default:
		weston_log("unknown recorder format\n");
		goto err_recorder;
	}

	recorder->fd = open(filename,
			    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);

	if (recorder->fd < 0) {
		weston_log("problem opening output file %s: %s\n", filename,
			   strerror(errno));
		goto err_recorder;
	}

	header.width = output->current_mode->width;
	header.height = output->current_mode->height;
	recorder->total += write(recorder->fd, &header, sizeof header);

	recorder->frame_listener.notify = weston_recorder_frame_notify;
	wl_signal_add(&output->frame_signal, &recorder->frame_listener);
	weston_output_disable_planes_incr(output);
	weston_output_damage(output);

	return recorder;

err_recorder:
	weston_recorder_free(recorder);
	return NULL;
}

static void
weston_recorder_destroy(struct weston_recorder *recorder)
{
	wl_list_remove(&recorder->frame_listener.link);
	close(recorder->fd);
	weston_output_disable_planes_decr(recorder->output);
	weston_recorder_free(recorder);
}

WL_EXPORT struct weston_recorder *
weston_recorder_start(struct weston_output *output, const char *filename)
{
	struct wl_listener *listener;

	listener = wl_signal_get(&output->frame_signal,
				 weston_recorder_frame_notify);
	if (listener) {
		weston_log("a recorder on output %s is already running\n",
			   output->name);
		return NULL;
	}

	weston_log("starting recorder for output %s, file %s\n",
		   output->name, filename);
	return weston_recorder_create(output, filename);
}

WL_EXPORT void
weston_recorder_stop(struct weston_recorder *recorder)
{
	weston_log("stopping recorder, total file size %dM, %d frames\n",
		   recorder->total / (1024 * 1024), recorder->count);

	recorder->destroying = 1;
	weston_output_schedule_repaint(recorder->output);
}
