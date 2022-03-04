/*
 * Copyright © 2015 Collabora, Ltd.
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
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <linux/input.h>

#include <libweston/libweston.h>
#include "compositor/weston.h"
#include "shared/file-util.h"
#include "libweston-internal.h"

static char *
encode_PAM_comment_line(const char *comment)
{
	size_t len = strlen(comment);
	char *str = malloc(len + 2);
	char *dst = str;
	const char *src = comment;
	const char *end = src + len;

	*dst++ = '#';
	*dst++ = ' ';
	for (; src < end; src++, dst++) {
		if (*src == '\n' || !isprint(*src))
			*dst = '_';
		else
			*dst = *src;
	}

	return str;
}

/*
 * PAM image format:
 * http://en.wikipedia.org/wiki/Netpbm#PAM_graphics_format
 * RGBA is in byte address order.
 *
 * ImageMagick 'convert' can convert a PAM image to a more common format.
 * To view the image metadata: $ head -n7 image.pam
 */
static int
write_PAM_image_rgba(FILE *fp, int width, int height,
		     void *pixels, size_t size, const char *comment)
{
	char *str;
	int ret;

	assert(size == (size_t)4 * width * height);

	ret = fprintf(fp, "P7\nWIDTH %d\nHEIGHT %d\nDEPTH 4\nMAXVAL 255\n"
		      "TUPLTYPE RGB_ALPHA\n", width, height);
	if (ret < 0)
		return -1;

	if (comment) {
		str = encode_PAM_comment_line(comment);
		ret = fprintf(fp, "%s\n", str);
		free(str);

		if (ret < 0)
			return -1;
	}

	ret = fprintf(fp, "ENDHDR\n");
	if (ret < 0)
		return -1;

	if (fwrite(pixels, 1, size, fp) != size)
		return -1;

	if (ferror(fp))
		return -1;

	return 0;
}

static uint32_t
unmult(uint32_t c, uint32_t a)
{
	return (c * 255 + a / 2) / a;
}

static void
unpremultiply_and_swap_a8b8g8r8_to_PAMrgba(void *pixels, size_t size)
{
	uint32_t *p = pixels;
	uint32_t *end;

	for (end = p + size / 4; p < end; p++) {
		uint32_t v = *p;
		uint32_t a;

		a = (v & 0xff000000) >> 24;
		if (a == 0) {
			*p = 0;
		} else {
			uint8_t *dst = (uint8_t *)p;

			dst[0] = unmult((v & 0x000000ff) >> 0, a);
			dst[1] = unmult((v & 0x0000ff00) >> 8, a);
			dst[2] = unmult((v & 0x00ff0000) >> 16, a);
			dst[3] = a;
		}
	}
}

static void
trigger_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		uint32_t key, void *data)
{
	const char *prefix = "surfaceshot-";
	const char *suffix = ".pam";
	char fname[1024];
	struct weston_surface *surface;
	struct weston_seat *seat = keyboard->seat;
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	int width, height;
	char desc[512];
	void *pixels;
	const size_t bytespp = 4; /* PIXMAN_a8b8g8r8 */
	size_t sz;
	int ret;
	FILE *fp;

	if (!pointer || !pointer->focus)
		return;

	surface = pointer->focus->surface;

	weston_surface_get_content_size(surface, &width, &height);

	if (!surface->get_label ||
	    surface->get_label(surface, desc, sizeof(desc)) < 0)
		snprintf(desc, sizeof(desc), "(unknown)");

	weston_log("surface screenshot of %p: '%s', %dx%d\n",
		   surface, desc, width, height);

	sz = width * bytespp * height;
	if (sz == 0) {
		weston_log("no content for %p\n", surface);
		return;
	}

	pixels = malloc(sz);
	if (!pixels) {
		weston_log("%s: failed to malloc %zu B\n", __func__, sz);
		return;
	}

	ret = weston_surface_copy_content(surface, pixels, sz,
					  0, 0, width, height);
	if (ret < 0) {
		weston_log("shooting surface %p failed\n", surface);
		goto out;
	}

	unpremultiply_and_swap_a8b8g8r8_to_PAMrgba(pixels, sz);

	fp = file_create_dated(NULL, prefix, suffix, fname, sizeof(fname));
	if (!fp) {
		const char *msg;

		switch (errno) {
		case ETIME:
			msg = "failure in datetime formatting";
			break;
		default:
			msg = strerror(errno);
		}

		weston_log("Cannot open '%s*%s' for writing: %s\n",
			   prefix, suffix, msg);
		goto out;
	}

	ret = write_PAM_image_rgba(fp, width, height, pixels, sz, desc);
	if (fclose(fp) != 0 || ret < 0)
		weston_log("writing surface %p screenshot failed.\n", surface);
	else
		weston_log("successfully shot surface %p into '%s'\n",
			   surface, fname);

out:
	free(pixels);
}

WL_EXPORT int
wet_module_init(struct weston_compositor *ec,
		int *argc, char *argv[])
{
	weston_compositor_add_debug_binding(ec, KEY_H, trigger_binding, ec);

	return 0;
}
