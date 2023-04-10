/*
 * Copyright 2019 Collabora, Ltd.
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

#ifndef WESTON_TESTSUITE_DATA_H
#define WESTON_TESTSUITE_DATA_H

/** Standard return codes
 *
 * Both Autotools and Meson use these codes as test program exit codes
 * to denote the test result for the whole process.
 *
 * \ingroup testharness
 */
enum test_result_code {
	RESULT_OK = 0,
	RESULT_SKIP = 77,
	RESULT_FAIL = 1,
	RESULT_HARD_ERROR = 99,
};

struct weston_test;
struct weston_compositor;

/** Weston test types
 *
 * \sa weston_test_harness_execute_standalone
 * weston_test_harness_execute_as_plugin
 * weston_test_harness_execute_as_client
 *
 * \ingroup testharness_private
 */
enum test_type {
	TEST_TYPE_STANDALONE,
	TEST_TYPE_PLUGIN,
	TEST_TYPE_CLIENT,
};

/** Test harness specific data for running tests
 *
 * \ingroup testharness_private
 */
struct wet_testsuite_data {
	void (*run)(struct wet_testsuite_data *);

	/* test definitions */
	const struct weston_test_entry *tests;
	unsigned tests_count;
	int case_index;
	enum test_type type;
	struct weston_compositor *compositor;

	/* client thread control */
	int thread_event_pipe;

	/* informational run state */
	int fixture_iteration;

	/* test counts */
	unsigned counter;
	unsigned passed;
	unsigned skipped;
	unsigned failed;
	unsigned total;
};

#endif /* WESTON_TESTSUITE_DATA_H */
