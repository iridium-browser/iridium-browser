/*
 * Copyright © 2011 Intel Corporation
 * Copyright © 2012 Collabora, Ltd.
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

#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef UNIT_TEST
#define WL_EXPORT
#else
#include <wayland-server.h>
#endif

#include <libweston/matrix.h>


/*
 * Matrices are stored in column-major order, that is the array indices are:
 *  0  4  8 12
 *  1  5  9 13
 *  2  6 10 14
 *  3  7 11 15
 */

WL_EXPORT void
weston_matrix_init(struct weston_matrix *matrix)
{
	static const struct weston_matrix identity = {
		.d = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 },
		.type = 0,
	};

	memcpy(matrix, &identity, sizeof identity);
}

/* m <- n * m, that is, m is multiplied on the LEFT. */
WL_EXPORT void
weston_matrix_multiply(struct weston_matrix *m, const struct weston_matrix *n)
{
	struct weston_matrix tmp;
	const float *row, *column;
	div_t d;
	int i, j;

	for (i = 0; i < 16; i++) {
		tmp.d[i] = 0;
		d = div(i, 4);
		row = m->d + d.quot * 4;
		column = n->d + d.rem;
		for (j = 0; j < 4; j++)
			tmp.d[i] += row[j] * column[j * 4];
	}
	tmp.type = m->type | n->type;
	memcpy(m, &tmp, sizeof tmp);
}

WL_EXPORT void
weston_matrix_translate(struct weston_matrix *matrix, float x, float y, float z)
{
	struct weston_matrix translate = {
		.d = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 },
		.type = WESTON_MATRIX_TRANSFORM_TRANSLATE,
	};

	weston_matrix_multiply(matrix, &translate);
}

WL_EXPORT void
weston_matrix_scale(struct weston_matrix *matrix, float x, float y,float z)
{
	struct weston_matrix scale = {
		.d = { x, 0, 0, 0,  0, y, 0, 0,  0, 0, z, 0,  0, 0, 0, 1 },
		.type = WESTON_MATRIX_TRANSFORM_SCALE,
	};

	weston_matrix_multiply(matrix, &scale);
}

WL_EXPORT void
weston_matrix_rotate_xy(struct weston_matrix *matrix, float cos, float sin)
{
	struct weston_matrix translate = {
		.d = { cos, sin, 0, 0,  -sin, cos, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 },
		.type = WESTON_MATRIX_TRANSFORM_ROTATE,
	};

	weston_matrix_multiply(matrix, &translate);
}

/* v <- m * v */
WL_EXPORT void
weston_matrix_transform(struct weston_matrix *matrix, struct weston_vector *v)
{
	int i, j;
	struct weston_vector t;

	for (i = 0; i < 4; i++) {
		t.f[i] = 0;
		for (j = 0; j < 4; j++)
			t.f[i] += v->f[j] * matrix->d[i + j * 4];
	}

	*v = t;
}

static inline void
swap_rows(double *a, double *b)
{
	unsigned k;
	double tmp;

	for (k = 0; k < 13; k += 4) {
		tmp = a[k];
		a[k] = b[k];
		b[k] = tmp;
	}
}

static inline void
swap_unsigned(unsigned *a, unsigned *b)
{
	unsigned tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

static inline unsigned
find_pivot(double *column, unsigned k)
{
	unsigned p = k;
	for (++k; k < 4; ++k)
		if (fabs(column[p]) < fabs(column[k]))
			p = k;

	return p;
}

/*
 * reference: Gene H. Golub and Charles F. van Loan. Matrix computations.
 * 3rd ed. The Johns Hopkins University Press. 1996.
 * LU decomposition, forward and back substitution: Chapter 3.
 */

MATRIX_TEST_EXPORT inline int
matrix_invert(double *A, unsigned *p, const struct weston_matrix *matrix)
{
	unsigned i, j, k;
	unsigned pivot;
	double pv;

	for (i = 0; i < 4; ++i)
		p[i] = i;
	for (i = 16; i--; )
		A[i] = matrix->d[i];

	/* LU decomposition with partial pivoting */
	for (k = 0; k < 4; ++k) {
		pivot = find_pivot(&A[k * 4], k);
		if (pivot != k) {
			swap_unsigned(&p[k], &p[pivot]);
			swap_rows(&A[k], &A[pivot]);
		}

		pv = A[k * 4 + k];
		if (fabs(pv) < 1e-9)
			return -1; /* zero pivot, not invertible */

		for (i = k + 1; i < 4; ++i) {
			A[i + k * 4] /= pv;

			for (j = k + 1; j < 4; ++j)
				A[i + j * 4] -= A[i + k * 4] * A[k + j * 4];
		}
	}

	return 0;
}

MATRIX_TEST_EXPORT inline void
inverse_transform(const double *LU, const unsigned *p, float *v)
{
	/* Solve A * x = v, when we have P * A = L * U.
	 * P * A * x = P * v  =>  L * U * x = P * v
	 * Let U * x = b, then L * b = P * v.
	 */
	double b[4];
	unsigned j;

	/* Forward substitution, column version, solves L * b = P * v */
	/* The diagonal of L is all ones, and not explicitly stored. */
	b[0] = v[p[0]];
	b[1] = (double)v[p[1]] - b[0] * LU[1 + 0 * 4];
	b[2] = (double)v[p[2]] - b[0] * LU[2 + 0 * 4];
	b[3] = (double)v[p[3]] - b[0] * LU[3 + 0 * 4];
	b[2] -= b[1] * LU[2 + 1 * 4];
	b[3] -= b[1] * LU[3 + 1 * 4];
	b[3] -= b[2] * LU[3 + 2 * 4];

	/* backward substitution, column version, solves U * y = b */
#if 1
	/* hand-unrolled, 25% faster for whole function */
	b[3] /= LU[3 + 3 * 4];
	b[0] -= b[3] * LU[0 + 3 * 4];
	b[1] -= b[3] * LU[1 + 3 * 4];
	b[2] -= b[3] * LU[2 + 3 * 4];

	b[2] /= LU[2 + 2 * 4];
	b[0] -= b[2] * LU[0 + 2 * 4];
	b[1] -= b[2] * LU[1 + 2 * 4];

	b[1] /= LU[1 + 1 * 4];
	b[0] -= b[1] * LU[0 + 1 * 4];

	b[0] /= LU[0 + 0 * 4];
#else
	for (j = 3; j > 0; --j) {
		unsigned k;
		b[j] /= LU[j + j * 4];
		for (k = 0; k < j; ++k)
			b[k] -= b[j] * LU[k + j * 4];
	}

	b[0] /= LU[0 + 0 * 4];
#endif

	/* the result */
	for (j = 0; j < 4; ++j)
		v[j] = b[j];
}

WL_EXPORT int
weston_matrix_invert(struct weston_matrix *inverse,
		     const struct weston_matrix *matrix)
{
	double LU[16];		/* column-major */
	unsigned perm[4];	/* permutation */
	unsigned c;

	if (matrix_invert(LU, perm, matrix) < 0)
		return -1;

	weston_matrix_init(inverse);
	for (c = 0; c < 4; ++c)
		inverse_transform(LU, perm, &inverse->d[c * 4]);
	inverse->type = matrix->type;

	return 0;
}
