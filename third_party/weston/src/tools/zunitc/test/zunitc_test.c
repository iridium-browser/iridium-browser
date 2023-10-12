/*
 * Copyright © 2015 Samsung Electronics Co., Ltd
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

/*
 * A simple file to show tests being setup and run.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zunitc/zunitc.h"

/*
 * The SKIP and FAIL sets of tests are those that will cause 'make check'
 * to fail so are disabled by default. They can be re-enabled when working
 * on the test framework itself.
 */

/* #define ENABLE_FAIL_TESTS */
/* #define ENABLE_SKIP_TESTS */

ZUC_TEST(base_test, math_is_sane)
{
	ZUC_ASSERT_EQ(4, 2 + 2);
}

ZUC_TEST(base_test, math_is_hard)
{
	ZUC_TRACEPOINT("Tracepoint here.");

	ZUC_TRACEPOINT("Checking %d", 4);

#ifdef ENABLE_FAIL_TESTS

	ZUC_ASSERT_EQ(5, 2 + 2);
	ZUC_TRACEPOINT("flip %1.3f", 3.1415927); /* not seen */
#endif
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(base_test, tracepoint_after_assert)
{
	ZUC_TRACEPOINT("Should be seen in output");

	ZUC_ASSERT_EQ(5, 2 + 2);

	ZUC_TRACEPOINT("Should NOT be seen in output");
}
#endif

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(base_test, math_is_more_hard)
{
	ZUC_ASSERT_EQ(5, 2 + 2);
}

ZUC_TEST(base_test, math_is_more_hard2)
{
	ZUC_ASSERT_EQ(7, 9);
}
#endif

ZUC_TEST(base_test, time_counted)
{
	ZUC_TRACEPOINT("Never seen");

	ZUC_TRACEPOINT("Sleepy Time %d", 10000 * 5);
	ZUC_ASSERT_EQ(0, usleep(10000 * 5)); /* 50ms to show up in reporting */
}

ZUC_TEST(other_test, math_monkey)
{
	ZUC_ASSERT_TRUE(1);
	ZUC_ASSERT_TRUE(3);
	ZUC_ASSERT_FALSE(0);

	ZUC_ASSERT_TRUE(1);
	ZUC_ASSERT_TRUE(3);
	ZUC_ASSERT_FALSE(0);

	ZUC_ASSERT_EQ(5, 2 + 3);
	ZUC_ASSERT_EQ(5, 2 + 3);

	int b = 9;
	ZUC_ASSERT_NE(1, 2);
	ZUC_ASSERT_NE(b, b + 2);

	ZUC_ASSERT_NE(1, 2);
	ZUC_ASSERT_NE(b, b + 1);

	ZUC_ASSERT_LT(1, 2);
	ZUC_ASSERT_LT(1, 3);

	ZUC_ASSERT_LE(1, 2);
	ZUC_ASSERT_LE(1, 3);

	ZUC_ASSERT_LE(1, 1);
	ZUC_ASSERT_LE(1, 1);

	ZUC_ASSERT_GT(2, 1);
	ZUC_ASSERT_GT(3, 1);

	ZUC_ASSERT_GE(1, 1);
	ZUC_ASSERT_GE(1, 1);

	ZUC_ASSERT_GE(2, 1);
	ZUC_ASSERT_GE(3, 1);
}

static void
force_fatal_failure(void)
{
#ifdef ENABLE_FAIL_TESTS
	bool expected_to_fail_here = true;
	ZUC_ASSERT_FALSE(expected_to_fail_here);

	ZUC_FATAL("Should never reach here");
	ZUC_ASSERT_NE(1, 1);
#endif
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(infrastructure, fail_keeps_testing)
{
	ZUC_FATAL("Should always reach here");
	ZUC_ASSERT_NE(1, 1); /* in case ZUC_FATAL doesn't work. */
}
#endif

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(infrastructure, fatal_stops_test)
{
	ZUC_FATAL("Time to kill testing");

	ZUC_FATAL("Should never reach here");
	ZUC_ASSERT_NE(1, 1); /* in case ZUC_FATAL doesn't work. */
}
#endif

#ifdef ENABLE_SKIP_TESTS
ZUC_TEST(infrastructure, skip_stops_test)
{
	ZUC_SKIP("Time to skip testing");

	ZUC_FATAL("Should never reach here");
	ZUC_ASSERT_NE(1, 1); /* in case ZUC_FATAL doesn't work. */
}
#endif

struct fixture_data {
	int case_counter;
	int test_counter;
};

static struct fixture_data fixture_info = {0, 0};

static void *
complex_test_set_up_case(const void *data)
{
	fixture_info.case_counter++;
	return &fixture_info;
}

static void
complex_test_tear_down_case(void *data)
{
	ZUC_ASSERT_TRUE(&fixture_info == data);
	fixture_info.case_counter--;
}

static void *
complex_test_set_up(void *data)
{
	fixture_info.test_counter = fixture_info.case_counter;
	return &fixture_info;
}

static void
complex_test_tear_down(void *data)
{
	ZUC_ASSERT_EQ(1, fixture_info.case_counter);
}

struct zuc_fixture complex_test = {
	.set_up = complex_test_set_up,
	.tear_down = complex_test_tear_down,
	.set_up_test_case = complex_test_set_up_case,
	.tear_down_test_case = complex_test_tear_down_case
};

/*
 * Note that these next cases all try to modify the test_counter member,
 * but the fixture should reset that.
*/

ZUC_TEST_F(complex_test, bases_cenario, data)
{
	struct fixture_data *fdata = data;
	ZUC_ASSERT_NOT_NULL(fdata);

	ZUC_ASSERT_EQ(4, 3 + 1);
	ZUC_ASSERT_EQ(1, fdata->case_counter);
	ZUC_ASSERT_EQ(1, fdata->test_counter);
	fdata->test_counter++;
	ZUC_ASSERT_EQ(2, fdata->test_counter);
}

ZUC_TEST_F(complex_test, something, data)
{
	struct fixture_data *fdata = data;
	ZUC_ASSERT_NOT_NULL(fdata);

	ZUC_ASSERT_EQ(4, 3 + 1);
	ZUC_ASSERT_EQ(1, fdata->case_counter);
	ZUC_ASSERT_EQ(1, fdata->test_counter);
	fdata->test_counter++;
	ZUC_ASSERT_EQ(2, fdata->test_counter);
}

ZUC_TEST_F(complex_test, else_here, data)
{
	struct fixture_data *fdata = data;
	ZUC_ASSERT_NOT_NULL(fdata);

	ZUC_ASSERT_EQ(4, 3 + 1);
	ZUC_ASSERT_EQ(1, fdata->case_counter);
	ZUC_ASSERT_EQ(1, fdata->test_counter);
	fdata->test_counter++;
	ZUC_ASSERT_EQ(2, fdata->test_counter);
}

ZUC_TEST(more, DISABLED_not_run)
{
	ZUC_ASSERT_EQ(1, 2);
}

ZUC_TEST(more, failure_states)
{
#ifdef ENABLE_FAIL_TESTS
	bool expected_to_fail_here = true;
#endif

	ZUC_ASSERT_FALSE(zuc_has_failure());

#ifdef ENABLE_FAIL_TESTS
	ZUC_ASSERT_FALSE(expected_to_fail_here); /* should fail */

	ZUC_ASSERT_TRUE(zuc_has_failure());
#endif
}

ZUC_TEST(more, failure_sub_fatal)
{
	ZUC_ASSERT_FALSE(zuc_has_failure());

	force_fatal_failure();

#ifdef ENABLE_FAIL_TESTS
	ZUC_ASSERT_TRUE(zuc_has_failure());
#endif
}

ZUC_TEST(pointers, null)
{
	const char *a = NULL;

	ZUC_ASSERT_NULL(NULL);
	ZUC_ASSERT_NULL(0);
	ZUC_ASSERT_NULL(a);
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(pointers, null_fail)
{
	const char *a = "a";

	ZUC_ASSERT_NULL(!NULL);
	ZUC_ASSERT_NULL(!0);
	ZUC_ASSERT_NULL(a);
}
#endif

ZUC_TEST(pointers, not_null)
{
	const char *a = "a";

	ZUC_ASSERT_NOT_NULL(!NULL);
	ZUC_ASSERT_NOT_NULL(!0);
	ZUC_ASSERT_NOT_NULL(a);
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(pointers, not_null_fail)
{
	const char *a = NULL;

	ZUC_ASSERT_NOT_NULL(NULL);
	ZUC_ASSERT_NOT_NULL(0);
	ZUC_ASSERT_NOT_NULL(a);
}
#endif

ZUC_TEST(strings, eq)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");
	const char *str_nil = NULL;

	ZUC_ASSERT_STREQ(str_a, str_a);
	ZUC_ASSERT_STREQ("a", str_a);
	ZUC_ASSERT_STREQ(str_a, "a");

	ZUC_ASSERT_STREQ(str_nil, str_nil);
	ZUC_ASSERT_STREQ(NULL, str_nil);
	ZUC_ASSERT_STREQ(str_nil, NULL);

	free(str_a);
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(strings, eq_fail)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");
	char *str_b = strdup("b");
	const char *str_nil = NULL;

	ZUC_ASSERT_STREQ(str_a, str_b);
	ZUC_ASSERT_STREQ("b", str_a);
	ZUC_ASSERT_STREQ(str_a, "b");

	ZUC_ASSERT_STREQ(str_nil, str_a);
	ZUC_ASSERT_STREQ(str_nil, str_b);
	ZUC_ASSERT_STREQ(str_a, str_nil);
	ZUC_ASSERT_STREQ(str_b, str_nil);

	ZUC_ASSERT_STREQ(NULL, str_a);
	ZUC_ASSERT_STREQ(NULL, str_b);
	ZUC_ASSERT_STREQ(str_a, NULL);
	ZUC_ASSERT_STREQ(str_b, NULL);

	free(str_a);
	free(str_b);
}
#endif

ZUC_TEST(strings, ne)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");
	char *str_b = strdup("b");
	const char *str_nil = NULL;

	ZUC_ASSERT_STRNE(str_a, str_b);
	ZUC_ASSERT_STRNE("b", str_a);
	ZUC_ASSERT_STRNE(str_a, "b");

	ZUC_ASSERT_STRNE(str_nil, str_a);
	ZUC_ASSERT_STRNE(str_nil, str_b);
	ZUC_ASSERT_STRNE(str_a, str_nil);
	ZUC_ASSERT_STRNE(str_b, str_nil);

	ZUC_ASSERT_STRNE(NULL, str_a);
	ZUC_ASSERT_STRNE(NULL, str_b);
	ZUC_ASSERT_STRNE(str_a, NULL);
	ZUC_ASSERT_STRNE(str_b, NULL);

	free(str_a);
	free(str_b);
}

#ifdef ENABLE_FAIL_TESTS
ZUC_TEST(strings, ne_fail01)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");

	ZUC_ASSERTG_STRNE(str_a, str_a, err);

err:
	free(str_a);
}

ZUC_TEST(strings, ne_fail02)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");

	ZUC_ASSERTG_STRNE("a", str_a, err);

err:
	free(str_a);
}

ZUC_TEST(strings, ne_fail03)
{
	/* Note that we use strdup() to ensure different addresses. */
	char *str_a = strdup("a");

	ZUC_ASSERTG_STRNE(str_a, "a", err);

err:
	free(str_a);
}

ZUC_TEST(strings, ne_fail04)
{
	const char *str_nil = NULL;

	ZUC_ASSERT_STRNE(str_nil, str_nil);
}

ZUC_TEST(strings, ne_fail05)
{
	const char *str_nil = NULL;

	ZUC_ASSERT_STRNE(NULL, str_nil);
}

ZUC_TEST(strings, ne_fail06)
{
	const char *str_nil = NULL;

	ZUC_ASSERT_STRNE(str_nil, NULL);
}
#endif

ZUC_TEST(base_test, later)
{
	/* an additional test for the same case but later in source */
	ZUC_ASSERT_EQ(3, 5 - 2);
}

ZUC_TEST(base_test, zed)
{
	/* an additional test for the same case but later in source */
	ZUC_ASSERT_EQ(3, 5 - 2);
}
