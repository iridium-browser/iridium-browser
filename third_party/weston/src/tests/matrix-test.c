/*
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <libweston/matrix.h>

struct inverse_matrix {
	double LU[16];		/* column-major */
	unsigned perm[4];	/* permutation */
};

static struct timespec begin_time;

static void
reset_timer(void)
{
	clock_gettime(CLOCK_MONOTONIC, &begin_time);
}

static double
read_timer(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)(t.tv_sec - begin_time.tv_sec) +
	       1e-9 * (t.tv_nsec - begin_time.tv_nsec);
}

static double
det3x3(const float *c0, const float *c1, const float *c2)
{
	return (double)
		c0[0] * c1[1] * c2[2] +
		c1[0] * c2[1] * c0[2] +
		c2[0] * c0[1] * c1[2] -
		c0[2] * c1[1] * c2[0] -
		c1[2] * c2[1] * c0[0] -
		c2[2] * c0[1] * c1[0];
}

static double
determinant(const struct weston_matrix *m)
{
	double det = 0;
#if 1
	/* develop on last row */
	det -= m->d[3 + 0 * 4] * det3x3(&m->d[4], &m->d[8], &m->d[12]);
	det += m->d[3 + 1 * 4] * det3x3(&m->d[0], &m->d[8], &m->d[12]);
	det -= m->d[3 + 2 * 4] * det3x3(&m->d[0], &m->d[4], &m->d[12]);
	det += m->d[3 + 3 * 4] * det3x3(&m->d[0], &m->d[4], &m->d[8]);
#else
	/* develop on first row */
	det += m->d[0 + 0 * 4] * det3x3(&m->d[5], &m->d[9], &m->d[13]);
	det -= m->d[0 + 1 * 4] * det3x3(&m->d[1], &m->d[9], &m->d[13]);
	det += m->d[0 + 2 * 4] * det3x3(&m->d[1], &m->d[5], &m->d[13]);
	det -= m->d[0 + 3 * 4] * det3x3(&m->d[1], &m->d[5], &m->d[9]);
#endif
	return det;
}

static void
print_permutation_matrix(const struct inverse_matrix *m)
{
	const unsigned *p = m->perm;
	const char *row[4] = {
		"1 0 0 0\n",
		"0 1 0 0\n",
		"0 0 1 0\n",
		"0 0 0 1\n"
	};

	printf("  P =\n%s%s%s%s", row[p[0]], row[p[1]], row[p[2]], row[p[3]]);
}

static void
print_LU_decomposition(const struct inverse_matrix *m)
{
	unsigned r, c;

	printf("                            L                      "
	       "                               U\n");
	for (r = 0; r < 4; ++r) {
		double v;

		for (c = 0; c < 4; ++c) {
			if (c < r)
				v = m->LU[r + c * 4];
			else if (c == r)
				v = 1.0;
			else
				v = 0.0;
			printf(" %12.6f", v);
		}

		printf(" | ");

		for (c = 0; c < 4; ++c) {
			if (c >= r)
				v = m->LU[r + c * 4];
			else
				v = 0.0;
			printf(" %12.6f", v);
		}
		printf("\n");
	}
}

static void
print_inverse_data_matrix(const struct inverse_matrix *m)
{
	unsigned r, c;

	for (r = 0; r < 4; ++r) {
		for (c = 0; c < 4; ++c)
			printf(" %12.6f", m->LU[r + c * 4]);
		printf("\n");
	}

	printf("permutation: ");
	for (r = 0; r < 4; ++r)
		printf(" %u", m->perm[r]);
	printf("\n");
}

static void
print_matrix(const struct weston_matrix *m)
{
	unsigned r, c;

	for (r = 0; r < 4; ++r) {
		for (c = 0; c < 4; ++c)
			printf(" %14.6e", m->d[r + c * 4]);
		printf("\n");
	}
}

static double
frand(void)
{
	double r = random();
	return r / (double)(RAND_MAX / 2) - 1.0f;
}

static void
randomize_matrix(struct weston_matrix *m)
{
	unsigned i;
	for (i = 0; i < 16; ++i)
#if 1
		m->d[i] = frand() * exp(10.0 * frand());
#else
		m->d[i] = frand();
#endif
}

/* Take a matrix, compute inverse, multiply together
 * and subtract the identity matrix to get the error matrix.
 * Return the largest absolute value from the error matrix.
 */
static double
test_inverse(struct weston_matrix *m)
{
	unsigned i;
	struct inverse_matrix q;
	double errsup = 0.0;

	if (matrix_invert(q.LU, q.perm, m) != 0)
		return INFINITY;

	for (i = 0; i < 4; ++i)
		inverse_transform(q.LU, q.perm, &m->d[i * 4]);

	m->d[0] -= 1.0f;
	m->d[5] -= 1.0f;
	m->d[10] -= 1.0f;
	m->d[15] -= 1.0f;

	for (i = 0; i < 16; ++i) {
		double err = fabs(m->d[i]);
		if (err > errsup)
			errsup = err;
	}

	return errsup;
}

enum {
	TEST_OK,
	TEST_NOT_INVERTIBLE_OK,
	TEST_FAIL,
	TEST_COUNT
};

static int
test(void)
{
	struct weston_matrix m;
	double det, errsup;

	randomize_matrix(&m);
	det = determinant(&m);

	errsup = test_inverse(&m);
	if (errsup < 1e-6)
		return TEST_OK;

	if (fabs(det) < 1e-5 && isinf(errsup))
		return TEST_NOT_INVERTIBLE_OK;

	printf("test fail, det: %g, error sup: %g\n", det, errsup);

	return TEST_FAIL;
}

static int running;
static void
stopme(int n)
{
	running = 0;
}

static void
test_loop_precision(void)
{
	int counts[TEST_COUNT] = { 0 };

	printf("\nRunning a test loop for 10 seconds...\n");
	running = 1;
	alarm(10);
	while (running) {
		counts[test()]++;
	}

	printf("tests: %d ok, %d not invertible but ok, %d failed.\n"
	       "Total: %d iterations.\n",
	       counts[TEST_OK], counts[TEST_NOT_INVERTIBLE_OK],
	       counts[TEST_FAIL],
	       counts[TEST_OK] + counts[TEST_NOT_INVERTIBLE_OK] +
	       counts[TEST_FAIL]);
}

static void __attribute__((noinline))
test_loop_speed_matrixvector(void)
{
	struct weston_matrix m;
	struct weston_vector v = { { 0.5, 0.5, 0.5, 1.0 } };
	unsigned long count = 0;
	double t;

	printf("\nRunning 3 s test on weston_matrix_transform()...\n");

	weston_matrix_init(&m);

	running = 1;
	alarm(3);
	reset_timer();
	while (running) {
		weston_matrix_transform(&m, &v);
		count++;
	}
	t = read_timer();

	printf("%lu iterations in %f seconds, avg. %.1f ns/iter.\n",
	       count, t, 1e9 * t / count);
}

static void __attribute__((noinline))
test_loop_speed_inversetransform(void)
{
	struct weston_matrix m;
	struct inverse_matrix inv;
	struct weston_vector v = { { 0.5, 0.5, 0.5, 1.0 } };
	unsigned long count = 0;
	double t;

	printf("\nRunning 3 s test on inverse_transform()...\n");

	weston_matrix_init(&m);
	matrix_invert(inv.LU, inv.perm, &m);

	running = 1;
	alarm(3);
	reset_timer();
	while (running) {
		inverse_transform(inv.LU, inv.perm, v.f);
		count++;
	}
	t = read_timer();

	printf("%lu iterations in %f seconds, avg. %.1f ns/iter.\n",
	       count, t, 1e9 * t / count);
}

static void __attribute__((noinline))
test_loop_speed_invert(void)
{
	struct weston_matrix m;
	struct inverse_matrix inv;
	unsigned long count = 0;
	double t;

	printf("\nRunning 3 s test on matrix_invert()...\n");

	weston_matrix_init(&m);

	running = 1;
	alarm(3);
	reset_timer();
	while (running) {
		matrix_invert(inv.LU, inv.perm, &m);
		count++;
	}
	t = read_timer();

	printf("%lu iterations in %f seconds, avg. %.1f ns/iter.\n",
	       count, t, 1e9 * t / count);
}

static void __attribute__((noinline))
test_loop_speed_invert_explicit(void)
{
	struct weston_matrix m;
	unsigned long count = 0;
	double t;

	printf("\nRunning 3 s test on weston_matrix_invert()...\n");

	weston_matrix_init(&m);

	running = 1;
	alarm(3);
	reset_timer();
	while (running) {
		weston_matrix_invert(&m, &m);
		count++;
	}
	t = read_timer();

	printf("%lu iterations in %f seconds, avg. %.1f ns/iter.\n",
	       count, t, 1e9 * t / count);
}

int main(void)
{
	struct sigaction ding;
	struct weston_matrix M;
	struct inverse_matrix Q;
	int ret;
	double errsup;
	double det;

	ding.sa_handler = stopme;
	sigemptyset(&ding.sa_mask);
	ding.sa_flags = 0;
	sigaction(SIGALRM, &ding, NULL);

	srandom(13);

	M.d[0] = 3.0;	M.d[4] = 17.0;	M.d[8] = 10.0;	M.d[12] = 0.0;
	M.d[1] = 2.0;	M.d[5] = 4.0;	M.d[9] = -2.0;	M.d[13] = 0.0;
	M.d[2] = 6.0;	M.d[6] = 18.0;	M.d[10] = -12;	M.d[14] = 0.0;
	M.d[3] = 0.0;	M.d[7] = 0.0;	M.d[11] = 0.0;	M.d[15] = 1.0;

	ret = matrix_invert(Q.LU, Q.perm, &M);
	printf("ret = %d\n", ret);
	printf("det = %g\n\n", determinant(&M));

	if (ret != 0)
		return 1;

	print_inverse_data_matrix(&Q);
	printf("P * A = L * U\n");
	print_permutation_matrix(&Q);
	print_LU_decomposition(&Q);


	printf("a random matrix:\n");
	randomize_matrix(&M);
	det = determinant(&M);
	print_matrix(&M);
	errsup = test_inverse(&M);
	printf("\nThe matrix multiplied by its inverse, error:\n");
	print_matrix(&M);
	printf("max abs error: %g, original determinant %g\n", errsup, det);

	test_loop_precision();
	test_loop_speed_matrixvector();
	test_loop_speed_inversetransform();
	test_loop_speed_invert();
	test_loop_speed_invert_explicit();

	return 0;
}
