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
#include <assert.h>

#include <cairo.h>

#include "wcap-decode.h"

static void
write_png(struct wcap_decoder *decoder, const char *filename)
{
	cairo_surface_t *surface;

	surface = cairo_image_surface_create_for_data((unsigned char *) decoder->frame,
						      CAIRO_FORMAT_ARGB32,
						      decoder->width,
						      decoder->height,
						      decoder->width * 4);
	cairo_surface_write_to_png(surface, filename);
	cairo_surface_destroy(surface);
}

static inline int
rgb_to_yuv(uint32_t format, uint32_t p, int *u, int *v)
{
	int r, g, b, y;

	switch (format) {
	case WCAP_FORMAT_XRGB8888:
		r = (p >> 16) & 0xff;
		g = (p >> 8) & 0xff;
		b = (p >> 0) & 0xff;
		break;
	case WCAP_FORMAT_XBGR8888:
		r = (p >> 0) & 0xff;
		g = (p >> 8) & 0xff;
		b = (p >> 16) & 0xff;
		break;
	default:
		assert(0);
	}

	y = (19595 * r + 38469 * g + 7472 * b) >> 16;
	if (y > 255)
		y = 255;

	*u += 46727 * (r - y);
	*v += 36962 * (b - y);

	return y;
}

static inline
int clamp_uv(int u)
{
	int clamp = (u >> 18) + 128;

	if (clamp < 0)
		return 0;
	else if (clamp > 255)
		return 255;
	else
		return clamp;
}

static void
convert_to_yv12(struct wcap_decoder *decoder, unsigned char *out)
{
	unsigned char *y1, *y2, *u, *v;
	uint32_t *p1, *p2, *end;
	int i, u_accum, v_accum, stride0, stride1;
	uint32_t format = decoder->format;

	stride0 = decoder->width;
	stride1 = decoder->width / 2;
	for (i = 0; i < decoder->height; i += 2) {
		y1 = out + stride0 * i;
		y2 = y1 + stride0;
		v = out + stride0 * decoder->height + stride1 * i / 2;
		u = v + stride1 * decoder->height / 2;
		p1 = decoder->frame + decoder->width * i;
		p2 = p1 + decoder->width;
		end = p1 + decoder->width;

		while (p1 < end) {
			u_accum = 0;
			v_accum = 0;
			y1[0] = rgb_to_yuv(format, p1[0], &u_accum, &v_accum);
			y1[1] = rgb_to_yuv(format, p1[1], &u_accum, &v_accum);
			y2[0] = rgb_to_yuv(format, p2[0], &u_accum, &v_accum);
			y2[1] = rgb_to_yuv(format, p2[1], &u_accum, &v_accum);
			u[0] = clamp_uv(u_accum);
			v[0] = clamp_uv(v_accum);

			y1 += 2;
			p1 += 2;
			y2 += 2;
			p2 += 2;
			u++;
			v++;
		}
	}
}

static void
convert_to_yuv444(struct wcap_decoder *decoder, unsigned char *out)
{

	unsigned char *yp, *up, *vp;
	uint32_t *rp, *end;
	int u, v;
	int i, stride, psize;
	uint32_t format = decoder->format;

	stride = decoder->width;
	psize = stride * decoder->height;
	for (i = 0; i < decoder->height; i++) {
		yp = out + stride * i;
		up = yp + (psize * 2);
		vp = yp + (psize * 1);
		rp = decoder->frame + decoder->width * i;
		end = rp + decoder->width;
		while (rp < end) {
			u = 0;
			v = 0;
			yp[0] = rgb_to_yuv(format, rp[0], &u, &v);
			up[0] = clamp_uv(u/.3);
			vp[0] = clamp_uv(v/.3);
			up++;
			vp++;
			yp++;
			rp++;
		}
	}
}

static void
output_yuv_frame(struct wcap_decoder *decoder, int depth)
{
	static unsigned char *out;
	int size;

	if (depth == 444) {
		size = decoder->width * decoder->height * 3;
	} else {
		size = decoder->width * decoder->height * 3 / 2;
	}
	if (out == NULL)
		out = malloc(size);

	if (depth == 444) {
		convert_to_yuv444(decoder, out);
	} else {
		convert_to_yv12(decoder, out);
	}

	printf("FRAME\n");
	fwrite(out, 1, size, stdout);
}

static void
usage(int exit_code)
{
	fprintf(stderr, "usage: wcap-decode "
		"[--help] [--yuv4mpeg2] [--frame=<frame>] [--all] \n"
		"\t[--rate=<num:denom>] <wcap file>\n\n"
		"\t--help\t\t\tthis help text\n"
		"\t--yuv4mpeg2\t\tdump wcap file to stdout in yuv4mpeg2 format\n"
		"\t--yuv4mpeg2-444\t\tdump wcap file to stdout in yuv4mpeg2 444 format\n"
		"\t--frame=<frame>\t\twrite out the given frame number as png\n"
		"\t--all\t\t\twrite all frames as pngs\n"
		"\t--rate=<num:denom>\treplay frame rate for yuv4mpeg2,\n"
		"\t\t\t\tspecified as an integer fraction\n\n");

	exit(exit_code);
}

int main(int argc, char *argv[])
{
	struct wcap_decoder *decoder;
	int i, j, output_frame = -1, yuv4mpeg2 = 0, all = 0, has_frame;
	int num = 30, denom = 1;
	char filename[200];
	char *mode;
	uint32_t msecs, frame_time;

	for (i = 1, j = 1; i < argc; i++) {
		if (strcmp(argv[i], "--yuv4mpeg2-444") == 0) {
			yuv4mpeg2 = 444;
		} else if (strcmp(argv[i], "--yuv4mpeg2") == 0) {
			yuv4mpeg2 = 420;
		} else if (strcmp(argv[i], "--help") == 0) {
			usage(EXIT_SUCCESS);
		} else if (strcmp(argv[i], "--all") == 0) {
			all = 1;
		} else if (sscanf(argv[i], "--frame=%d", &output_frame) == 1) {
			;
		} else if (sscanf(argv[i], "--rate=%d", &num) == 1) {
			;
		} else if (sscanf(argv[i], "--rate=%d:%d", &num, &denom) == 2) {
			;
		} else if (strcmp(argv[i], "--") == 0) {
			break;
		} else if (argv[i][0] == '-') {
			fprintf(stderr,
				"unknown option or invalid argument: %s\n", argv[i]);
			usage(EXIT_FAILURE);
		} else {
			argv[j++] = argv[i];
		}
	}
	argc = j;

	if (argc != 2)
		usage(EXIT_FAILURE);
	if (denom == 0) {
		fprintf(stderr, "invalid rate, denom can not be 0\n");
		exit(EXIT_FAILURE);
	}

	decoder = wcap_decoder_create(argv[1]);
	if (decoder == NULL) {
		fprintf(stderr, "Creating wcap decoder failed\n");
		exit(EXIT_FAILURE);
	}

	if (yuv4mpeg2 && isatty(1)) {
		fprintf(stderr, "Not dumping yuv4mpeg2 data to terminal.  Pipe output to a file or a process.\n");
		fprintf(stderr, "For example, to encode to webm, use something like\n\n");
		fprintf(stderr, "\t$ wcap-decode  --yuv4mpeg2 ../capture.wcap |\n"
			"\t\tvpxenc --target-bitrate=1024 --best -t 4 -o foo.webm -\n\n");

		exit(EXIT_FAILURE);
	}

	if (yuv4mpeg2) {
		if (yuv4mpeg2 == 444) {
			mode = "C444";
		} else {
			mode = "C420jpeg";
		}
		printf("YUV4MPEG2 %s W%d H%d F%d:%d Ip A0:0\n",
					 mode, decoder->width, decoder->height, num, denom);
		fflush(stdout);
	}

	i = 0;
	has_frame = wcap_decoder_get_frame(decoder);
	msecs = decoder->msecs;
	frame_time = 1000 * denom / num;
	while (has_frame) {
		if (all || i == output_frame) {
			snprintf(filename, sizeof filename,
				 "wcap-frame-%d.png", i);
			write_png(decoder, filename);
			fprintf(stderr, "wrote %s\n", filename);
		}
		if (yuv4mpeg2)
			output_yuv_frame(decoder, yuv4mpeg2);
		i++;
		msecs += frame_time;
		while (decoder->msecs < msecs && has_frame)
			has_frame = wcap_decoder_get_frame(decoder);
	}

	fprintf(stderr, "wcap file: size %dx%d, %d frames\n",
		decoder->width, decoder->height, i);

	wcap_decoder_destroy(decoder);

	return EXIT_SUCCESS;
}
