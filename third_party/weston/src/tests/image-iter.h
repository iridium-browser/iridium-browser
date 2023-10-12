/*
 * Copyright 2021 Advanced Micro Devices, Inc.
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

#include <stdint.h>
#include <pixman.h>
#include <assert.h>

/** A collection of basic information extracted from a pixman_image_t */
struct image_header {
	int width;
	int height;
	pixman_format_code_t pixman_format;

	int stride_bytes;
	unsigned char *data;
};

/** Populate image_header from pixman_image_t */
static inline struct image_header
image_header_from(pixman_image_t *image)
{
	struct image_header h;

	h.width = pixman_image_get_width(image);
	h.height = pixman_image_get_height(image);
	h.pixman_format = pixman_image_get_format(image);
	h.stride_bytes = pixman_image_get_stride(image);
	h.data = (void *)pixman_image_get_data(image);

	return h;
}

/** Get pointer to the beginning of the row
 *
 * \param header Header describing the Pixman image.
 * \param y Index of the desired row, starting from zero.
 * \return Pointer to the first pixel of the row.
 *
 * Asserts that y is within image height, and that pixel format uses 32 bits
 * per pixel.
 */
static inline uint32_t *
image_header_get_row_u32(const struct image_header *header, int y)
{
	assert(y >= 0);
	assert(y < header->height);
	assert(PIXMAN_FORMAT_BPP(header->pixman_format) == 32);

	return (uint32_t *)(header->data + y * header->stride_bytes);
}
