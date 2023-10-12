/*
 * Copyright 2020 Collabora, Ltd.
 * Copyright 2021 Advanced Micro Devices, Inc.
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
#include <stdbool.h>
#include <stdio.h>

enum color_chan_index {
	COLOR_CHAN_R = 0,
	COLOR_CHAN_G,
	COLOR_CHAN_B,
	COLOR_CHAN_NUM
};

/* column vector when used in linear algebra */
struct color_float {
	union {
		float rgb[COLOR_CHAN_NUM];
		struct {
			float r, g, b;
		};
	};
	float a;
};

/* column vector */
struct lcmsVEC3 {
	float n[3];
};

struct lcmsMAT3 {
	/* array of columns */
	struct lcmsVEC3 v[3];
};

enum transfer_fn {
	TRANSFER_FN_IDENTITY,
	TRANSFER_FN_SRGB_EOTF,
	TRANSFER_FN_SRGB_EOTF_INVERSE,
	TRANSFER_FN_ADOBE_RGB_EOTF,
	TRANSFER_FN_ADOBE_RGB_EOTF_INVERSE,
	TRANSFER_FN_POWER2_4_EOTF,
	TRANSFER_FN_POWER2_4_EOTF_INVERSE,
};

/*
 * A helper to lay out a matrix in the natural writing order in code
 * instead of needing to transpose in your mind every time you read it.
 * The matrix is laid out as written:
 *     ⎡ a11 a12 a13 ⎤
 *     ⎢ a21 a22 a23 ⎥
 *     ⎣ a31 a32 a33 ⎦
 * where the first digit is row and the second digit is column.
 */
#define LCMSMAT3(a11, a12, a13,						\
		 a21, a22, a23,						\
		 a31, a32, a33) ((struct lcmsMAT3)			\
	{ /* Each vector is a column => looks like a transpose */	\
		.v[0] = { .n = { a11, a21, a31} },			\
		.v[1] = { .n = { a12, a22, a32} },			\
		.v[2] = { .n = { a13, a23, a33} },			\
	})

void
sRGB_linearize(struct color_float *cf);

void
sRGB_delinearize(struct color_float *cf);

struct color_float
a8r8g8b8_to_float(uint32_t v);

bool
find_tone_curve_type(enum transfer_fn fn, int *type, double params[5]);

float
apply_tone_curve(enum transfer_fn fn, float r);

bool
should_include_vcgt(const double vcgt_exponents[COLOR_CHAN_NUM]);

void
process_pixel_using_pipeline(enum transfer_fn pre_curve,
			     const struct lcmsMAT3 *mat,
			     enum transfer_fn post_curve,
			     const double vcgt_exponents[COLOR_CHAN_NUM],
			     const struct color_float *in,
			     struct color_float *out);

struct color_float
color_float_unpremult(struct color_float in);

struct color_float
color_float_apply_curve(enum transfer_fn fn, struct color_float c);

struct color_float
color_float_apply_matrix(const struct lcmsMAT3 *mat, struct color_float c);

enum transfer_fn
transfer_fn_invert(enum transfer_fn fn);

const char *
transfer_fn_name(enum transfer_fn fn);

void
lcmsMAT3_invert(struct lcmsMAT3 *result, const struct lcmsMAT3 *mat);

/** Scalar statistics
 *
 * See scalar_stat_update().
 */
struct scalar_stat {
	double min;
	struct color_float min_pos;

	double max;
	struct color_float max_pos;

	double sum;
	unsigned count;

	/** Debug dump into file
	 *
	 * Initialize this to a writable file to get a record of all values
	 * ever fed through this statistics accumulator. The file shall be
	 * text with one value and its position per line:
	 *   val pos.r pos.g pos.b pos.a
	 *
	 * Set to NULL to not record.
	 */
	FILE *dump;
};

/** RGB difference statistics
 *
 * See rgb_diff_stat_update().
 */
struct rgb_diff_stat {
	struct scalar_stat rgb[COLOR_CHAN_NUM];
	struct scalar_stat two_norm;

	/** Debug dump into file
	 *
	 * Initialize this to a writable file to get a record of all values
	 * ever fed through this statistics accumulator. The file shall be
	 * text with the two-norm error, the rgb difference, and their position
	 * per line:
	 *   norm diff.r diff.g diff.b pos.r pos.g pos.b pos.a
	 *
	 * Set to NULL to not record.
	 */
	FILE *dump;
};

void
scalar_stat_update(struct scalar_stat *stat,
		   double val,
		   const struct color_float *pos);

float
scalar_stat_avg(const struct scalar_stat *stat);

void
scalar_stat_print_float(const struct scalar_stat *stat);

void
rgb_diff_stat_update(struct rgb_diff_stat *stat,
		     const struct color_float *ref,
		     const struct color_float *val,
		     const struct color_float *pos);

void
rgb_diff_stat_print(const struct rgb_diff_stat *stat,
		    const char *title, unsigned scaling_bits);
