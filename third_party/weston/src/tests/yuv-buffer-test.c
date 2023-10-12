/*
 * Copyright © 2020 Collabora, Ltd.
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

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h>
#include <unistd.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"
#include "image-iter.h"
#include "shared/os-compatibility.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/xalloc.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = WESTON_RENDERER_GL;
	setup.width = 324;
	setup.height = 264;
	setup.shell = SHELL_TEST_DESKTOP;
	setup.logging_scopes = "log,gl-shader-generator";

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

struct yuv_buffer {
	void *data;
	size_t bytes;
	struct wl_buffer *proxy;
	int width;
	int height;
};

struct yuv_case {
	uint32_t drm_format;
	const char *drm_format_name;
	struct yuv_buffer *(*create_buffer)(struct client *client,
					    uint32_t drm_format,
					    pixman_image_t *rgb_image);
};

static struct yuv_buffer *
yuv_buffer_create(struct client *client,
		  size_t bytes,
		  int width,
		  int height,
		  int stride_bytes,
		  uint32_t drm_format)
{
	struct wl_shm_pool *pool;
	struct yuv_buffer *buf;
	int fd;

	buf = xzalloc(sizeof *buf);
	buf->bytes = bytes;
	buf->width = width;
	buf->height = height;

	fd = os_create_anonymous_file(buf->bytes);
	assert(fd >= 0);

	buf->data = mmap(NULL, buf->bytes,
			 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf->data == MAP_FAILED) {
		close(fd);
		assert(buf->data != MAP_FAILED);
	}

	pool = wl_shm_create_pool(client->wl_shm, fd, buf->bytes);
	buf->proxy = wl_shm_pool_create_buffer(pool, 0, buf->width, buf->height,
					       stride_bytes, drm_format);
	wl_shm_pool_destroy(pool);
	close(fd);

	return buf;
}

static void
yuv_buffer_destroy(struct yuv_buffer *buf)
{
	wl_buffer_destroy(buf->proxy);
	assert(munmap(buf->data, buf->bytes) == 0);
	free(buf);
}

/*
 * Based on Rec. ITU-R BT.601-7
 *
 * This is intended to be obvious and accurate, not fast.
 */
static void
x8r8g8b8_to_ycbcr8_bt601(uint32_t xrgb,
			 uint8_t *y_out, uint8_t *cb_out, uint8_t *cr_out)
{
	double y, cb, cr;
	double r = (xrgb >> 16) & 0xff;
	double g = (xrgb >> 8) & 0xff;
	double b = (xrgb >> 0) & 0xff;

	/* normalize to [0.0, 1.0] */
	r /= 255.0;
	g /= 255.0;
	b /= 255.0;

	/* Y normalized to [0.0, 1.0], Cb and Cr [-0.5, 0.5] */
	y = 0.299 * r + 0.587 * g + 0.114 * b;
	cr = (r - y) / 1.402;
	cb = (b - y) / 1.772;

	/* limited range quantization to 8 bit */
	*y_out = round(219.0 * y + 16.0);
	if (cr_out)
		*cr_out = round(224.0 * cr + 128.0);
	if (cb_out)
		*cb_out = round(224.0 * cb + 128.0);
}

/*
 * 3 plane YCbCr
 * plane 0: Y plane, [7:0] Y
 * plane 1: Cb plane, [7:0] Cb
 * plane 2: Cr plane, [7:0] Cr
 * YUV420: 2x2 subsampled Cb (1) and Cr (2) planes
 * YUV444: no subsampling
 */
static struct yuv_buffer *
y_u_v_create_buffer(struct client *client,
		    uint32_t drm_format,
		    pixman_image_t *rgb_image)
{
	struct image_header rgb = image_header_from(rgb_image);
	struct yuv_buffer *buf;
	size_t bytes;
	int x, y;
	uint32_t *rgb_row;
	uint8_t *y_base;
	uint8_t *u_base;
	uint8_t *v_base;
	uint8_t *y_row;
	uint8_t *u_row;
	uint8_t *v_row;
	uint32_t argb;
	int sub = (drm_format == DRM_FORMAT_YUV420) ? 2 : 1;

	assert(drm_format == DRM_FORMAT_YUV420 ||
	       drm_format == DRM_FORMAT_YUV444);

	/* Full size Y plus quarter U and V */
	bytes = rgb.width * rgb.height +
		(rgb.width / sub) * (rgb.height / sub) * 2;
	buf = yuv_buffer_create(client, bytes, rgb.width, rgb.height,
				rgb.width, drm_format);

	y_base = buf->data;
	u_base = y_base + rgb.width * rgb.height;
	v_base = u_base + (rgb.width / sub) * (rgb.height / sub);

	for (y = 0; y < rgb.height; y++) {
		rgb_row = image_header_get_row_u32(&rgb, y / 2 * 2);
		y_row = y_base + y * rgb.width;
		u_row = u_base + (y / sub) * (rgb.width / sub);
		v_row = v_base + (y / sub) * (rgb.width / sub);

		for (x = 0; x < rgb.width; x++) {
			/*
			 * Sub-sample the source image instead, so that U and V
			 * sub-sampling does not require proper
			 * filtering/averaging/siting.
			 */
			argb = *(rgb_row + x / 2 * 2);

			/*
			 * A stupid way of "sub-sampling" chroma. This does not
			 * do the necessary filtering/averaging/siting or
			 * alternate Cb/Cr rows.
			 */
			if ((y & (sub - 1)) == 0 && (x & (sub - 1)) == 0) {
				x8r8g8b8_to_ycbcr8_bt601(argb, y_row + x,
							 u_row + x / sub,
							 v_row + x / sub);
			} else {
				x8r8g8b8_to_ycbcr8_bt601(argb, y_row + x,
							 NULL, NULL);
			}
		}
	}

	return buf;
}

/*
 * 2 plane YCbCr
 * plane 0 = Y plane, [7:0] Y
 * plane 1 = Cr:Cb plane, [15:0] Cr:Cb little endian
 * 2x2 subsampled Cr:Cb plane
 */
static struct yuv_buffer *
nv12_create_buffer(struct client *client,
		   uint32_t drm_format,
		   pixman_image_t *rgb_image)
{
	struct image_header rgb = image_header_from(rgb_image);
	struct yuv_buffer *buf;
	size_t bytes;
	int x, y;
	uint32_t *rgb_row;
	uint8_t *y_base;
	uint16_t *uv_base;
	uint8_t *y_row;
	uint16_t *uv_row;
	uint32_t argb;
	uint8_t cr;
	uint8_t cb;

	assert(drm_format == DRM_FORMAT_NV12);

	/* Full size Y, quarter UV */
	bytes = rgb.width * rgb.height +
		(rgb.width / 2) * (rgb.height / 2) * sizeof(uint16_t);
	buf = yuv_buffer_create(client, bytes, rgb.width, rgb.height,
				rgb.width, drm_format);

	y_base = buf->data;
	uv_base = (uint16_t *)(y_base + rgb.width * rgb.height);

	for (y = 0; y < rgb.height; y++) {
		rgb_row = image_header_get_row_u32(&rgb, y / 2 * 2);
		y_row = y_base + y * rgb.width;
		uv_row = uv_base + (y / 2) * (rgb.width / 2);

		for (x = 0; x < rgb.width; x++) {
			/*
			 * Sub-sample the source image instead, so that U and V
			 * sub-sampling does not require proper
			 * filtering/averaging/siting.
			 */
			argb = *(rgb_row + x / 2 * 2);

			/*
			 * A stupid way of "sub-sampling" chroma. This does not
			 * do the necessary filtering/averaging/siting.
			 */
			if ((y & 1) == 0 && (x & 1) == 0) {
				x8r8g8b8_to_ycbcr8_bt601(argb, y_row + x,
							 &cb, &cr);
				*(uv_row + x / 2) = ((uint16_t)cr << 8) | cb;
			} else {
				x8r8g8b8_to_ycbcr8_bt601(argb, y_row + x,
							 NULL, NULL);
			}
		}
	}

	return buf;
}

/*
 * Packed YCbCr
 *
 * [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian
 * 2x1 subsampled Cr:Cb plane
 */
static struct yuv_buffer *
yuyv_create_buffer(struct client *client,
		   uint32_t drm_format,
		   pixman_image_t *rgb_image)
{
	struct image_header rgb = image_header_from(rgb_image);
	struct yuv_buffer *buf;
	size_t bytes;
	int x, y;
	uint32_t *rgb_row;
	uint32_t *yuv_base;
	uint32_t *yuv_row;
	uint8_t cr;
	uint8_t cb;
	uint8_t y0;

	assert(drm_format == DRM_FORMAT_YUYV);

	/* Full size Y, horizontally subsampled UV, 2 pixels in 32 bits */
	bytes = rgb.width / 2 * rgb.height * sizeof(uint32_t);
	buf = yuv_buffer_create(client, bytes, rgb.width, rgb.height,
				rgb.width / 2 * sizeof(uint32_t), drm_format);

	yuv_base = buf->data;

	for (y = 0; y < rgb.height; y++) {
		rgb_row = image_header_get_row_u32(&rgb, y / 2 * 2);
		yuv_row = yuv_base + y * (rgb.width / 2);

		for (x = 0; x < rgb.width; x += 2) {
			/*
			 * Sub-sample the source image instead, so that U and V
			 * sub-sampling does not require proper
			 * filtering/averaging/siting.
			 */
			x8r8g8b8_to_ycbcr8_bt601(*(rgb_row + x), &y0, &cb, &cr);
			*(yuv_row + x / 2) =
				((uint32_t)cr << 24) |
				((uint32_t)y0 << 16) |
				((uint32_t)cb << 8) |
				((uint32_t)y0 << 0);
		}
	}

	return buf;
}

/*
 * Packed YCbCr
 *
 * [31:0] X:Y:Cb:Cr 8:8:8:8 little endian
 * full resolution chroma
 */
static struct yuv_buffer *
xyuv8888_create_buffer(struct client *client,
		       uint32_t drm_format,
		       pixman_image_t *rgb_image)
{
	struct image_header rgb = image_header_from(rgb_image);
	struct yuv_buffer *buf;
	size_t bytes;
	int x, y;
	uint32_t *rgb_row;
	uint32_t *yuv_base;
	uint32_t *yuv_row;
	uint8_t cr;
	uint8_t cb;
	uint8_t y0;

	assert(drm_format == DRM_FORMAT_XYUV8888);

	/* Full size, 32 bits per pixel */
	bytes = rgb.width * rgb.height * sizeof(uint32_t);
	buf = yuv_buffer_create(client, bytes, rgb.width, rgb.height,
				rgb.width * sizeof(uint32_t), drm_format);

	yuv_base = buf->data;

	for (y = 0; y < rgb.height; y++) {
		rgb_row = image_header_get_row_u32(&rgb, y / 2 * 2);
		yuv_row = yuv_base + y * rgb.width;

		for (x = 0; x < rgb.width; x++) {
			/*
			 * 2x2 sub-sample the source image to get the same
			 * result as the other YUV variants, so we can use the
			 * same reference image for checking.
			 */
			x8r8g8b8_to_ycbcr8_bt601(*(rgb_row + x / 2 * 2), &y0, &cb, &cr);
			/*
			 * The unused byte is intentionally set to "garbage"
			 * to catch any accidental use of it in the compositor.
			 */
			*(yuv_row + x) =
				((uint32_t)x << 24) |
				((uint32_t)y0 << 16) |
				((uint32_t)cb << 8) |
				((uint32_t)cr << 0);
		}
	}

	return buf;
}

static void
show_window_with_yuv(struct client *client, struct yuv_buffer *buf)
{
	struct surface *surface = client->surface;
	int done;

	weston_test_move_surface(client->test->weston_test, surface->wl_surface,
				 4, 4);
	wl_surface_attach(surface->wl_surface, buf->proxy, 0, 0);
	wl_surface_damage(surface->wl_surface, 0, 0, buf->width,
			  buf->height);
	frame_callback_set(surface->wl_surface, &done);
	wl_surface_commit(surface->wl_surface);
	frame_callback_wait(client, &done);
}

static const struct yuv_case yuv_cases[] = {
#define FMT(x) DRM_FORMAT_ ##x, #x
	{ FMT(YUV420), y_u_v_create_buffer },
	{ FMT(YUV444), y_u_v_create_buffer },
	{ FMT(NV12), nv12_create_buffer },
	{ FMT(YUYV), yuyv_create_buffer },
	{ FMT(XYUV8888), xyuv8888_create_buffer },
#undef FMT
};

/*
 * Test that various YUV pixel formats result in correct coloring on screen.
 */
TEST_P(yuv_buffer_shm, yuv_cases)
{
	const struct yuv_case *my_case = data;
	char *fname;
	pixman_image_t *img;
	struct client *client;
	struct yuv_buffer *buf;
	bool match;

	testlog("%s: format %s\n", get_test_name(), my_case->drm_format_name);

	/*
	 * This test image is 256 x 256 pixels.
	 *
	 * Therefore this test does NOT exercise:
	 * - odd image dimensions
	 * - non-square image
	 * - row padding
	 * - unaligned row stride
	 * - different alignments or padding in sub-sampled planes
	 *
	 * The reason to not test these is that GL-renderer seems to be more
	 * or less broken.
	 *
	 * The source image is effectively further downscaled to 128 x 128
	 * before sampled and converted to 256 x 256 YUV, so that
	 * sub-sampling for U and V does not require proper algorithms.
	 * Therefore, this test also does not test:
	 * - chroma siting (chroma sample positioning)
	 */
	fname = image_filename("chocolate-cake");
	img = load_image_from_png(fname);
	free(fname);
	assert(img);

	client = create_client();
	client->surface = create_test_surface(client);
	buf = my_case->create_buffer(client, my_case->drm_format, img);
	show_window_with_yuv(client, buf);

	match = verify_screen_content(client, "yuv-buffer", 0, NULL, 0, NULL);
	assert(match);

	yuv_buffer_destroy(buf);
	pixman_image_unref(img);
	client_destroy(client);
}
