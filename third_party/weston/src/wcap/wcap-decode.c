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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <cairo.h>

#include "wcap-decode.h"

static void
wcap_decoder_decode_rectangle(struct wcap_decoder *decoder,
			      struct wcap_rectangle *rect)
{
	uint32_t v, *p = decoder->p, *d;
	int width = rect->x2 - rect->x1, height = rect->y2 - rect->y1;
	int x, i, j, k, l, count = width * height;
	unsigned char r, g, b, dr, dg, db;

	d = decoder->frame + (rect->y2 - 1) * decoder->width;
	x = rect->x1;
	i = 0;
	while (i < count) {
		v = *p++;
		l = v >> 24;
		if (l < 0xe0) {
			j = l + 1;
		} else {
			j = 1 << (l - 0xe0 + 7);
		}

		dr = (v >> 16);
		dg = (v >>  8);
		db = (v >>  0);
		for (k = 0; k < j; k++) {
			r = (d[x] >> 16) + dr;
			g = (d[x] >>  8) + dg;
			b = (d[x] >>  0) + db;
			d[x] = 0xff000000 | (r << 16) | (g << 8) | b;
			x++;
			if (x == rect->x2) {
				x = rect->x1;
				d -= decoder->width;
			}
		}
		i += j;
	}

	if (i != count)
		printf("rle encoding longer than expected (%d expected %d)\n",
		       i, count);

	decoder->p = p;
}

int
wcap_decoder_get_frame(struct wcap_decoder *decoder)
{
	struct wcap_rectangle *rects;
	struct wcap_frame_header *header;
	uint32_t i;

	if (decoder->p == decoder->end)
		return 0;

	header = decoder->p;
	decoder->msecs = header->msecs;
	decoder->count++;

	rects = (void *) (header + 1);
	decoder->p = (uint32_t *) (rects + header->nrects);
	for (i = 0; i < header->nrects; i++)
		wcap_decoder_decode_rectangle(decoder, &rects[i]);

	return 1;
}

struct wcap_decoder *
wcap_decoder_create(const char *filename)
{
	struct wcap_decoder *decoder;
	struct wcap_header *header;
	int frame_size;
	struct stat buf;

	decoder = malloc(sizeof *decoder);
	if (decoder == NULL)
		return NULL;

	decoder->fd = open(filename, O_RDONLY);
	if (decoder->fd == -1) {
		free(decoder);
		return NULL;
	}

	fstat(decoder->fd, &buf);
	decoder->size = buf.st_size;
	decoder->map = mmap(NULL, decoder->size,
			    PROT_READ, MAP_PRIVATE, decoder->fd, 0);
	if (decoder->map == MAP_FAILED) {
		fprintf(stderr, "mmap failed\n");
		close(decoder->fd);
		free(decoder);
		return NULL;
	}

	header = decoder->map;
	decoder->format = header->format;
	decoder->count = 0;
	decoder->width = header->width;
	decoder->height = header->height;
	decoder->p = header + 1;
	decoder->end = decoder->map + decoder->size;

	frame_size = header->width * header->height * 4;
	decoder->frame = malloc(frame_size);
	if (decoder->frame == NULL) {
		close(decoder->fd);
		free(decoder);
		return NULL;
	}
	memset(decoder->frame, 0, frame_size);

	return decoder;
}

void
wcap_decoder_destroy(struct wcap_decoder *decoder)
{
	munmap(decoder->map, decoder->size);
	close(decoder->fd);
	free(decoder->frame);
	free(decoder);
}
