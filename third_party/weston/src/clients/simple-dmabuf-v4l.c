/*
 * Copyright © 2015 Collabora Ltd.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>
#include <linux/input.h>

#include <wayland-client.h>
#include <libweston/zalloc.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "weston-direct-display-client-protocol.h"

#include "shared/helpers.h"
#include "shared/weston-drm-fourcc.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define OPT_FLAG_INVERT (1 << 0)
#define OPT_FLAG_DIRECT_DISPLAY (1 << 1)

static void
redraw(void *data, struct wl_callback *callback, uint32_t time);

static int
xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (r == -1 && errno == EINTR);

	return r;
}

static uint32_t
parse_format(const char fmt[4])
{
	return fourcc_code(fmt[0], fmt[1], fmt[2], fmt[3]);
}

static inline const char *
dump_format(uint32_t format, char out[4])
{
#if BYTE_ORDER == BIG_ENDIAN
	format = __builtin_bswap32(format);
#endif
	memcpy(out, &format, 4);
	return out;
}

struct buffer_format {
	int width;
	int height;
	enum v4l2_buf_type type;
	uint32_t format;

	unsigned num_planes;
	unsigned strides[VIDEO_MAX_PLANES];
};

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct xdg_wm_base *wm_base;
	struct zwp_fullscreen_shell_v1 *fshell;
	struct zwp_linux_dmabuf_v1 *dmabuf;
	struct weston_direct_display_v1 *direct_display;
	bool requested_format_found;
	uint32_t opts;

	int v4l_fd;
	struct buffer_format format;
	uint32_t drm_format;
};

struct buffer {
	struct wl_buffer *buffer;
	struct display *display;
	int busy;
	int index;

	int dmabuf_fds[VIDEO_MAX_PLANES];
	int data_offsets[VIDEO_MAX_PLANES];
};

#define NUM_BUFFERS 4

struct window {
	struct display *display;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct buffer buffers[NUM_BUFFERS];
	struct wl_callback *callback;
	bool wait_for_configure;
	bool initialized;
};

static bool running = true;

static int
queue(struct display *display, struct buffer *buffer)
{
	struct v4l2_buffer buf;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	unsigned i;

	CLEAR(buf);
	buf.type = display->format.type;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = buffer->index;

	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		CLEAR(planes);
		buf.length = VIDEO_MAX_PLANES;
		buf.m.planes = planes;
	}

	if (xioctl(display->v4l_fd, VIDIOC_QUERYBUF, &buf) == -1) {
		perror("VIDIOC_QUERYBUF");
		return 0;
	}

	if (xioctl(display->v4l_fd, VIDIOC_QBUF, &buf) == -1) {
		perror("VIDIOC_QBUF");
		return 0;
	}

	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if (display->format.num_planes != buf.length) {
			fprintf(stderr, "Wrong number of planes returned by "
			                "QUERYBUF\n");
			return 0;
		}

		for (i = 0; i < buf.length; ++i)
			buffer->data_offsets[i] = buf.m.planes[i].data_offset;
	}

	return 1;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	struct buffer *mybuf = data;

	mybuf->busy = 0;

	if (!queue(mybuf->display, mybuf))
		running = false;
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static unsigned int
set_format(struct display *display, uint32_t format)
{
	struct v4l2_format fmt;

	CLEAR(fmt);

	fmt.type = display->format.type;

	if (xioctl(display->v4l_fd, VIDIOC_G_FMT, &fmt) == -1) {
		perror("VIDIOC_G_FMT");
		return 0;
	}

	/* NOTE: pix and pix_mp are in a union, pixelformat member maps between them. */
	const int format_matches = fmt.fmt.pix.pixelformat == format;

	/* No need to set the format if it already is the one we want */
	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE &&
            format_matches)
		return 1;
	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
            format_matches)
		return fmt.fmt.pix_mp.num_planes;

	fmt.fmt.pix.pixelformat = format;

	if (xioctl(display->v4l_fd, VIDIOC_S_FMT, &fmt) == -1) {
		perror("VIDIOC_S_FMT");
		return 0;
	}

	const int format_was_set = fmt.fmt.pix.pixelformat == format;
	if (!format_was_set) {
		char want_name[4];
		char have_name[4];

		dump_format(format, want_name);
		dump_format(fmt.fmt.pix.pixelformat, have_name);
		fprintf(stderr, "Tried to set format: %.4s but have: %.4s\n",
				 want_name, have_name);
		return 0;
	}

	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return fmt.fmt.pix_mp.num_planes;

	return 1;
}

static int
v4l_connect(struct display *display, const char *dev_name)
{
	struct v4l2_capability cap;
	struct v4l2_requestbuffers req;
	struct v4l2_input input;
	int index_input = -1;
	unsigned int num_planes;

	display->v4l_fd = open(dev_name, O_RDWR);
	if (display->v4l_fd < 0) {
		perror(dev_name);
		return 0;
	}

	if (xioctl(display->v4l_fd, VIDIOC_QUERYCAP, &cap) == -1) {
		if (errno == EINVAL) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
		} else {
			perror("VIDIOC_QUERYCAP");
		}
		return 0;
	}

	if (xioctl(display->v4l_fd, VIDIOC_G_INPUT, &index_input) == 0) {
		input.index = index_input;
		if (xioctl(display->v4l_fd, VIDIOC_ENUMINPUT, &input) == 0) {
			if (input.status & V4L2_IN_ST_VFLIP) {
				fprintf(stdout, "Found camera sensor y-flipped\n");
				display->opts |= OPT_FLAG_INVERT;
			}
		}
	}

	if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
		display->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	else if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
		display->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	else {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		return 0;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "%s does not support dmabuf i/o\n", dev_name);
		return 0;
	}

	/* Select video input, video standard and tune here */

	num_planes = set_format(display, display->format.format);
	if (num_planes < 1)
		return 0;

	CLEAR(req);

	req.type = display->format.type;
	req.memory = V4L2_MEMORY_MMAP;
	req.count = NUM_BUFFERS * num_planes;

	if (xioctl(display->v4l_fd, VIDIOC_REQBUFS, &req) == -1) {
		if (errno == EINVAL) {
			fprintf(stderr, "%s does not support dmabuf\n",
			        dev_name);
		} else {
			perror("VIDIOC_REQBUFS");
		}
		return 0;
	}

	if (req.count < NUM_BUFFERS * num_planes) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		return 0;
	}

	printf("Created %d buffers\n", req.count);

	return 1;
}

static void
v4l_shutdown(struct display *display)
{
	close(display->v4l_fd);
}

static void
create_succeeded(void *data,
		 struct zwp_linux_buffer_params_v1 *params,
		 struct wl_buffer *new_buffer)
{
	struct buffer *buffer = data;
	unsigned i;

	buffer->buffer = new_buffer;
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);

	zwp_linux_buffer_params_v1_destroy(params);

	for (i = 0; i < buffer->display->format.num_planes; ++i)
		close(buffer->dmabuf_fds[i]);
}

static void
create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
	struct buffer *buffer = data;
	unsigned i;

	buffer->buffer = NULL;

	zwp_linux_buffer_params_v1_destroy(params);

	for (i = 0; i < buffer->display->format.num_planes; ++i)
		close(buffer->dmabuf_fds[i]);

	running = false;

	fprintf(stderr, "Error: zwp_linux_buffer_params.create failed.\n");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener = {
	create_succeeded,
	create_failed
};

static void
create_dmabuf_buffer(struct display *display, struct buffer *buffer)
{
	struct zwp_linux_buffer_params_v1 *params;
	uint64_t modifier;
	uint32_t flags;
	int i;

	modifier = 0;
	flags = 0;

	if (display->opts & OPT_FLAG_INVERT)
		flags |= ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT;

	params = zwp_linux_dmabuf_v1_create_params(display->dmabuf);

	if ((display->opts & OPT_FLAG_DIRECT_DISPLAY) && display->direct_display) {
		weston_direct_display_v1_enable(display->direct_display, params);

		if (display->opts & OPT_FLAG_INVERT) {
			flags &= ~ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT;
			fprintf(stdout, "dmabuf y-inverted attribute flag was removed"
					", as display-direct flag was set\n");
		}
	}

	const int num_planes = (int) display->format.num_planes;

	for (i = 0; i < num_planes; ++i) {
		fprintf(stderr, "buffer %d, plane %d has dma fd %d and stride "
				"%d and modifier %" PRIu64 "\n",
				buffer->index, i, buffer->dmabuf_fds[i],
				display->format.strides[i], modifier);
		zwp_linux_buffer_params_v1_add(params,
		                               buffer->dmabuf_fds[i],
		                               i, /* plane_idx */
		                               buffer->data_offsets[i], /* offset */
		                               display->format.strides[i],
		                               modifier >> 32,
		                               modifier & 0xffffffff);
	}

	/* Some v4l2 devices can output NV12, but will do so without the MPLANE
	 * api. Instead, it outputs both the luminance and chrominance planes
	 * in the same dma buffer. Here we account for that, and add an extra
	 * plane from the same buffer if necessary. If it needs an extra plane,
	 * set the stride of the chrominance plane. NOTE: Also handles cases
	 * where 3 planes are expected in 1 dma buffer (untested)
	 */
	enum plane_layout_t {
		DISJOINT = 0,
		CONTIGUOUS,
	};
	enum chrom_packing_t {
		CHROM_SEPARATE = 0, /* Cr/Cb are in their own planes. */
		CHROM_COMBINED,     /* Cr/Cb are interleaved. */
	};

	/* This table contains some planar formats we could fix-up and support. */
	const struct planar_layout_t {
		/* Format identification. */
		uint32_t v4l_fourcc;
		/* Disjoint or contigious planes? */
		enum plane_layout_t plane_layout;
		/* Zero if Cb/Cr in separate planes. */
		enum chrom_packing_t chrom_packing;
		/* Expected plane count. */
		int num_planes;
		/* Horizontal sub-sampling for chroma. */
		int chroma_subsample_hori;
		/* Vertical sub-sampling for chroma. */
		int chroma_subsample_vert;
	} planar_layouts[] = {
		{ V4L2_PIX_FMT_NV12M,	DISJOINT,	CHROM_COMBINED,	2, 2,2 },
		{ V4L2_PIX_FMT_NV21M,	DISJOINT,	CHROM_COMBINED,	2, 2,2 },
		{ V4L2_PIX_FMT_NV16M,	DISJOINT,	CHROM_COMBINED,	2, 2,1 },
		{ V4L2_PIX_FMT_NV61M,	DISJOINT,	CHROM_COMBINED,	2, 2,1 },
		{ V4L2_PIX_FMT_NV12,	CONTIGUOUS,	CHROM_COMBINED,	2, 2,2 },
		{ V4L2_PIX_FMT_NV21,	CONTIGUOUS,	CHROM_COMBINED,	2, 2,2 },
		{ V4L2_PIX_FMT_NV16,	CONTIGUOUS,	CHROM_COMBINED,	2, 2,1 },
		{ V4L2_PIX_FMT_NV61,	CONTIGUOUS,	CHROM_COMBINED,	2, 2,1 },
		{ V4L2_PIX_FMT_NV24,	CONTIGUOUS,	CHROM_COMBINED,	2, 1,1 },
		{ V4L2_PIX_FMT_NV42,	CONTIGUOUS,	CHROM_COMBINED,	2, 1,1 },
		{ V4L2_PIX_FMT_YUV420,	CONTIGUOUS,	CHROM_SEPARATE,	3, 2,2 },
		{ V4L2_PIX_FMT_YVU420,	CONTIGUOUS,    	CHROM_SEPARATE,	3, 2,2 },
		{ V4L2_PIX_FMT_YUV420M,	DISJOINT,	CHROM_SEPARATE,	3, 2,2 },
		{ V4L2_PIX_FMT_YVU420M,	DISJOINT,	CHROM_SEPARATE,	3, 2,2 },
		{ 0, 0, 0, 0, 0 },
	};

	int layoutnr = 0;
	int num_missing_planes = 0;	/* Non-zero if format needs more planes in dma buf. */
	int stride_extra_plane = 0;
	int vrtres_extra_plane = 0;
	const uint32_t stride0 = display->format.strides[0];

	/* Search the table. */
	while (planar_layouts[layoutnr].v4l_fourcc) {
		const struct planar_layout_t *layout =
			planar_layouts + layoutnr;

		if (layout->v4l_fourcc == display->format.format) {
			/* If disjoint planes are missing, there is nothing to
			 * salvage. */
			if (layout->plane_layout == DISJOINT)
				assert(num_planes == layout->num_planes);

			/* Is this a case where we need to add 1 or 2 missing
			 * planes? */
			num_missing_planes = layout->num_planes - num_planes;
			if (num_missing_planes > 0) {
				/* With this knowledge:
				 * - Stride for Y
				 * - Packing of chrominance
				 * - Horizontal subsampling ...we can compute
				 *   the stride for Cr and Cb.
				 */
				const uint32_t num_chrom_parts =
                                        layout->chrom_packing == CHROM_COMBINED ? 2 : 1;
				stride_extra_plane =
					stride0 * num_chrom_parts /
					layout->chroma_subsample_hori;
				vrtres_extra_plane =
                                        display->format.height /
					layout->chroma_subsample_vert;
				break;
			}
		}
		layoutnr += 1;
	}
	/* If we determined we need additional planes, add them. */
	int offset_in_buffer = buffer->data_offsets[0] +
			       display->format.height * stride0;

	for (i = 0; i < num_missing_planes; ++i) {
		/* Add same dma buffer, but with offset for chromimance plane. */
		fprintf(stderr,"Adding additional chrominance plane.\n");
		zwp_linux_buffer_params_v1_add(params,
					       buffer->dmabuf_fds[0],
					       1 + i, /* plane_idx */
					       offset_in_buffer,
					       stride_extra_plane,
					       modifier >> 32,
					       modifier & 0xffffffff);
		offset_in_buffer += vrtres_extra_plane * stride_extra_plane;
	}

	zwp_linux_buffer_params_v1_add_listener(params, &params_listener, buffer);

	fprintf(stderr,"creating buffer of size %dx%d format %c%c%c%c flags %d\n",
		display->format.width,
		display->format.height,
		(display->drm_format >>  0) & 0xff,
		(display->drm_format >>  8) & 0xff,
		(display->drm_format >> 16) & 0xff,
		(display->drm_format >> 24) & 0xff,
		flags
	);
	zwp_linux_buffer_params_v1_create(params,
	                                  display->format.width,
	                                  display->format.height,
	                                  display->drm_format,
	                                  flags);
}

static int
buffer_export(struct display *display, int index, int dmafd[])
{
	struct v4l2_exportbuffer expbuf;
	unsigned i;

	CLEAR(expbuf);

	for (i = 0; i < display->format.num_planes; ++i) {
		expbuf.type = display->format.type;
		expbuf.index = index;
		expbuf.plane = i;
		if (xioctl(display->v4l_fd, VIDIOC_EXPBUF, &expbuf) == -1) {
			perror("VIDIOC_EXPBUF");
			while (i)
				close(dmafd[--i]);
			return 0;
		}
		dmafd[i] = expbuf.fd;
	}

	return 1;
}

static int
queue_initial_buffers(struct display *display,
                      struct buffer buffers[NUM_BUFFERS])
{
	struct buffer *buffer;
	int index;

	for (index = 0; index < NUM_BUFFERS; ++index) {
		buffer = &buffers[index];
		buffer->display = display;
		buffer->index = index;

		if (!queue(display, buffer)) {
			fprintf(stderr, "Failed to queue buffer\n");
			return 0;
		}

		assert(!buffer->buffer);
		if (!buffer_export(display, index, buffer->dmabuf_fds))
			return 0;

		create_dmabuf_buffer(display, buffer);
	}

	return 1;
}

static int
dequeue(struct display *display)
{
	struct v4l2_buffer buf;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];

	CLEAR(buf);
	buf.type = display->format.type;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.length = VIDEO_MAX_PLANES;
	buf.m.planes = planes;

	/* This ioctl is blocking until a buffer is ready to be displayed */
	if (xioctl(display->v4l_fd, VIDIOC_DQBUF, &buf) == -1) {
		perror("VIDIOC_DQBUF");
		return -1;
	}

	return buf.index;
}

static int
fill_buffer_format(struct display *display)
{
	struct v4l2_format fmt;
	struct v4l2_pix_format *pix;
	struct v4l2_pix_format_mplane *pix_mp;
	int i;
	char buf[4];

	CLEAR(fmt);
	fmt.type = display->format.type;

	/* Preserve original settings as set by v4l2-ctl for example */
	if (xioctl(display->v4l_fd, VIDIOC_G_FMT, &fmt) == -1) {
		perror("VIDIOC_G_FMT");
		return 0;
	}

	if (display->format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		pix = &fmt.fmt.pix;

		printf("%d×%d, %.4s\n", pix->width, pix->height,
		       dump_format(pix->pixelformat, buf));

		display->format.num_planes = 1;
		display->format.width = pix->width;
		display->format.height = pix->height;
		display->format.strides[0] = pix->bytesperline;
	} else {
		pix_mp = &fmt.fmt.pix_mp;

		display->format.num_planes = pix_mp->num_planes;
		display->format.width = pix_mp->width;
		display->format.height = pix_mp->height;

		for (i = 0; i < pix_mp->num_planes; ++i)
			display->format.strides[i] = pix_mp->plane_fmt[i].bytesperline;

		printf("%d×%d, %.4s, %d planes\n",
		       pix_mp->width, pix_mp->height,
		       dump_format(pix_mp->pixelformat, buf),
		       pix_mp->num_planes);
	}

	return 1;
}

static int
v4l_init(struct display *display, struct buffer buffers[NUM_BUFFERS]) {
	if (!fill_buffer_format(display)) {
		fprintf(stderr, "Failed to fill buffer format\n");
		return 0;
	}

	if (!queue_initial_buffers(display, buffers)) {
		fprintf(stderr, "Failed to queue initial buffers\n");
		return 0;
	}

	return 1;
}

static int
start_capture(struct display *display)
{
	int type = display->format.type;

	if (xioctl(display->v4l_fd, VIDIOC_STREAMON, &type) == -1) {
		perror("VIDIOC_STREAMON");
		return 0;
	}

	return 1;
}

static void
xdg_surface_handle_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	struct window *window = data;

	xdg_surface_ack_configure(surface, serial);

	if (window->initialized && window->wait_for_configure)
		redraw(window, NULL, 0);
	window->wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	xdg_surface_handle_configure,
};

static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *states)
{
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close,
};

static struct window *
create_window(struct display *display)
{
	struct window *window;

	window = zalloc(sizeof *window);
	if (!window)
		return NULL;

	window->callback = NULL;
	window->display = display;
	window->surface = wl_compositor_create_surface(display->compositor);

	if (display->wm_base) {
		window->xdg_surface =
			xdg_wm_base_get_xdg_surface(display->wm_base,
						    window->surface);

		assert(window->xdg_surface);

		xdg_surface_add_listener(window->xdg_surface,
					 &xdg_surface_listener, window);

		window->xdg_toplevel =
			xdg_surface_get_toplevel(window->xdg_surface);

		assert(window->xdg_toplevel);

		xdg_toplevel_add_listener(window->xdg_toplevel,
					  &xdg_toplevel_listener, window);

		xdg_toplevel_set_title(window->xdg_toplevel, "simple-dmabuf-v4l");
		xdg_toplevel_set_app_id(window->xdg_toplevel,
				"org.freedesktop.weston.simple-dmabuf-v4l");

		window->wait_for_configure = true;
		wl_surface_commit(window->surface);
	} else if (display->fshell) {
		zwp_fullscreen_shell_v1_present_surface(display->fshell,
		                                        window->surface,
		                                        ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT,
		                                        NULL);
	} else {
		assert(0);
	}

	return window;
}

static void
destroy_window(struct window *window)
{
	int i;
	unsigned j;

	if (window->callback)
		wl_callback_destroy(window->callback);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	wl_surface_destroy(window->surface);

	for (i = 0; i < NUM_BUFFERS; i++) {
		if (!window->buffers[i].buffer)
			continue;

		wl_buffer_destroy(window->buffers[i].buffer);
		for (j = 0; j < window->display->format.num_planes; ++j)
			close(window->buffers[i].dmabuf_fds[j]);
	}

	v4l_shutdown(window->display);

	free(window);
}

static const struct wl_callback_listener frame_listener;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buffer;
	int index, num_busy = 0;

	/* Check for a deadlock situation where we would block forever trying
	 * to dequeue a buffer while all of them are locked by the compositor.
	 */
	for (index = 0; index < NUM_BUFFERS; ++index)
		if (window->buffers[index].busy)
			++num_busy;

	/* A robust application would just postpone redraw until it has queued
	 * a buffer.
	 */
	assert(num_busy < NUM_BUFFERS);

	index = dequeue(window->display);
	if (index < 0) {
		/* We couldn’t get any buffer out of the camera, exiting. */
		running = false;
		return;
	}

	buffer = &window->buffers[index];
	assert(!buffer->busy);

	wl_surface_attach(window->surface, buffer->buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0,
	                  window->display->format.width,
	                  window->display->format.height);

	if (callback)
		wl_callback_destroy(callback);

	window->callback = wl_surface_frame(window->surface);
	wl_callback_add_listener(window->callback, &frame_listener, window);
	wl_surface_commit(window->surface);
	buffer->busy = 1;
}

static const struct wl_callback_listener frame_listener = {
	redraw
};

static void
dmabuf_modifier(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
		 uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
{
	struct display *d = data;
	uint64_t modifier = u64_from_u32s(modifier_hi, modifier_lo);

	if (format == d->drm_format && modifier == DRM_FORMAT_MOD_LINEAR)
		d->requested_format_found = true;
}


static void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
              uint32_t format)
{
	/* deprecated */
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
	dmabuf_format,
	dmabuf_modifier
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
	/* Just so we don’t leak the keymap fd */
	close(fd);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
	struct display *d = data;

	if (!d->wm_base)
		return;

	if (key == KEY_ESC && state)
		running = false;
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
	struct display *d = data;

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !d->keyboard) {
		d->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && d->keyboard) {
		wl_keyboard_destroy(d->keyboard);
		d->keyboard = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	xdg_wm_base_ping,
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor =
			wl_registry_bind(registry,
			                 id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		d->seat = wl_registry_bind(registry,
		                           id, &wl_seat_interface, 1);
		wl_seat_add_listener(d->seat, &seat_listener, d);
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		d->wm_base = wl_registry_bind(registry,
					      id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(d->wm_base, &wm_base_listener, d);
	} else if (strcmp(interface, "zwp_fullscreen_shell_v1") == 0) {
		d->fshell = wl_registry_bind(registry,
		                             id, &zwp_fullscreen_shell_v1_interface,
		                             1);
	} else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
		d->dmabuf = wl_registry_bind(registry,
		                             id, &zwp_linux_dmabuf_v1_interface, 3);
		zwp_linux_dmabuf_v1_add_listener(d->dmabuf, &dmabuf_listener,
		                                 d);
	} else if (strcmp(interface, "weston_direct_display_v1") == 0) {
		d->direct_display = wl_registry_bind(registry,
						     id, &weston_direct_display_v1_interface, 1);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static struct display *
create_display(uint32_t requested_format, uint32_t opt_flags)
{
	struct display *display;

	display = zalloc(sizeof *display);
	if (display == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	display->display = wl_display_connect(NULL);
	assert(display->display);

	display->drm_format = requested_format;

	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
	                         &registry_listener, display);
	wl_display_roundtrip(display->display);
	if (display->dmabuf == NULL) {
		fprintf(stderr, "No zwp_linux_dmabuf global\n");
		exit(1);
	}

	wl_display_roundtrip(display->display);

	if (!display->requested_format_found) {
		char want_name[4];

		dump_format(requested_format, want_name);
		fprintf(stderr, "Requested DRM format %4s not available\n", want_name);
		exit(1);
	}

	if (opt_flags)
		display->opts = opt_flags;
	return display;
}

static void
destroy_display(struct display *display)
{
	if (display->dmabuf)
		zwp_linux_dmabuf_v1_destroy(display->dmabuf);

	if (display->wm_base)
		xdg_wm_base_destroy(display->wm_base);

	if (display->fshell)
		zwp_fullscreen_shell_v1_release(display->fshell);

	if (display->compositor)
		wl_compositor_destroy(display->compositor);

	wl_registry_destroy(display->registry);
	wl_display_flush(display->display);
	wl_display_disconnect(display->display);
	free(display);
}

static void
usage(const char *argv0)
{
	printf("Usage: %s [-v v4l2_device] [-f v4l2_format] [-d drm_format] [-i|--y-invert] [-g|--d-display]\n"
	       "\n"
	       "The default V4L2 device is /dev/video0\n"
	       "\n"
	       "Both formats are FOURCC values (see http://fourcc.org/)\n"
	       "V4L2 formats are defined in <linux/videodev2.h>\n"
	       "DRM formats are defined in <libdrm/drm_fourcc.h>\n"
	       "The default for both formats is YUYV.\n"
	       "If the V4L2 and DRM formats differ, the data is simply "
	       "reinterpreted rather than converted.\n\n"
	       "Flags:\n"
	       "- y-invert force the image to be y-flipped;\n  note will be "
	       "automatically added if we detect if the camera sensor is "
	       "y-flipped\n"
	       "- d-display skip importing dmabuf-based buffer into the GPU\n  "
	       "and attempt pass the buffer straight to the display controller\n",
	       argv0);

	printf("\n"
	       "How to set up Vivid the virtual video driver for testing:\n"
	       "- build your kernel with CONFIG_VIDEO_VIVID=m\n"
	       "- add this to a /etc/modprobe.d/ file:\n"
	       "    options vivid node_types=0x1 num_inputs=1 input_types=0x00\n"
	       "- modprobe vivid and check which device was created,\n"
	       "  here we assume /dev/video0\n"
	       "- set the pixel format:\n"
	       "    $ v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,pixelformat=XR24\n"
	       "- optionally could add 'allocators=0x1' to options as to create"
	       "  the buffer in a dmabuf-contiguous way\n"
	       "  (as some display-controllers require it)\n"
	       "- launch the demo:\n"
	       "    $ %s -v /dev/video0 -f XR24 -d XR24\n"
	       "You should see a test pattern with color bars, and some text.\n"
	       "\n"
	       "More about vivid: https://www.kernel.org/doc/Documentation/video4linux/vivid.txt\n"
	       "\n", argv0);

	exit(0);
}

static void
signal_int(int signum)
{
	running = false;
}

int
main(int argc, char **argv)
{
	struct sigaction sigint;
	struct display *display;
	struct window *window;
	const char *v4l_device = NULL;
	uint32_t v4l_format = 0x0;
	uint32_t drm_format = 0x0;
	uint32_t opts_flags = 0x0;
	int c, opt_index, ret = 0;

	static struct option long_options[] = {
		{ "v4l2-device", required_argument, NULL, 'v' },
		{ "v4l2-format", required_argument, NULL, 'f' },
		{ "drm-format",	 required_argument, NULL, 'd' },
		{ "y-invert",    no_argument, 	    NULL, 'i' },
		{ "d-display",   no_argument, 	    NULL, 'g' },
		{ "help",        no_argument,       NULL, 'h' },
		{ 0,             0,                 NULL,  0  }
	};

	while ((c = getopt_long(argc, argv, "hiv:d:f:g", long_options,
				&opt_index)) != -1) {
		switch (c) {
		case 'v':
			v4l_device = optarg;
			break;
		case 'f':
			v4l_format = parse_format(optarg);
			break;
		case 'd':
			drm_format = parse_format(optarg);
			break;
		case 'i':
			opts_flags |= OPT_FLAG_INVERT;
			break;
		case 'g':
			opts_flags |= OPT_FLAG_DIRECT_DISPLAY;
			break;
		default:
		case 'h':
			usage(argv[0]);
			break;
		}
	}

	if (!v4l_device)
		v4l_device = "/dev/video0";

	if (v4l_format == 0x0)
		v4l_format = parse_format("YUYV");

	if (drm_format == 0x0)
		drm_format = v4l_format;

	display = create_display(drm_format, opts_flags);
	display->format.format = v4l_format;

	window = create_window(display);
	if (!window)
		return 1;

	if (!v4l_connect(display, v4l_device))
		return 1;

	if (!v4l_init(display, window->buffers))
		return 1;

	sigint.sa_handler = signal_int;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sigint, NULL);

	/* Here we retrieve the linux-dmabuf objects, or error */
	wl_display_roundtrip(display->display);

	/* In case of error, running will be 0 */
	if (!running)
		return 1;

	/* We got all of our buffers, we can start the capture! */
	if (!start_capture(display))
		return 1;

	window->initialized = true;

	if (!window->wait_for_configure)
		redraw(window, NULL, 0);

	while (running && ret != -1)
		ret = wl_display_dispatch(display->display);

	fprintf(stderr, "simple-dmabuf-v4l exiting\n");
	destroy_window(window);
	destroy_display(display);

	return 0;
}
