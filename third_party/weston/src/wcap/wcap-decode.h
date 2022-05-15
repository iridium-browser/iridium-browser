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

#ifndef _WCAP_DECODE_
#define _WCAP_DECODE_

#include <stdint.h>

#define WCAP_HEADER_MAGIC	0x57434150

#define WCAP_FORMAT_XRGB8888	0x34325258
#define WCAP_FORMAT_XBGR8888	0x34324258
#define WCAP_FORMAT_RGBX8888	0x34325852
#define WCAP_FORMAT_BGRX8888	0x34325842

struct wcap_header {
	uint32_t magic;
	uint32_t format;
	uint32_t width, height;
};

struct wcap_frame_header {
	uint32_t msecs;
	uint32_t nrects;
};

struct wcap_rectangle {
	int32_t x1, y1, x2, y2;
};

struct wcap_decoder {
	int fd;
	size_t size;
	void *map, *p, *end;
	uint32_t *frame;
	uint32_t format;
	uint32_t msecs;
	uint32_t count;
	int width, height;
};

int wcap_decoder_get_frame(struct wcap_decoder *decoder);
struct wcap_decoder *wcap_decoder_create(const char *filename);
void wcap_decoder_destroy(struct wcap_decoder *decoder);

#endif
