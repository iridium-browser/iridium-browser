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

/**
 * Tests of fixtures.
 */

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "zunitc/zunitc.h"



/* Use a simple string for a simplistic fixture */
static struct zuc_fixture fixture_minimal = {
	.data = "for all good men to",
};

ZUC_TEST_F(fixture_minimal, just_as_is, data)
{
	const char *str = data;
	ZUC_ASSERT_NOT_NULL(str);

	ZUC_ASSERT_EQ(0, strcmp("for all good men to", str));
}

/*
 * Not important what or how data is manipulated, just that this function
 * does something non-transparent to it.
 */
static void *
setup_test_config(void *data)
{
	int i;
	const char *str = data;
	char *upper = NULL;
	ZUC_ASSERTG_NOT_NULL(data, out);

	upper = strdup(str);
	ZUC_ASSERTG_NOT_NULL(upper, out);

	for (i = 0; upper[i]; ++i)
		upper[i] = (char)toupper(upper[i]);

out:
	return upper;
}

static void
teardown_test_config(void *data)
{
	ZUC_ASSERT_NOT_NULL(data);
	free(data);
}

static struct zuc_fixture fixture_data0 = {
	.data = "Now is the time",
	.set_up = setup_test_config,
	.tear_down = teardown_test_config
};

ZUC_TEST_F(fixture_data0, base, data)
{
	const char *str = data;
	ZUC_ASSERT_NOT_NULL(str);

	ZUC_ASSERT_EQ(0, strcmp("NOW IS THE TIME", str));
}

/* Use the same fixture for a second test. */
ZUC_TEST_F(fixture_data0, no_lower, data)
{
	int i;
	const char *str = data;
	ZUC_ASSERT_NOT_NULL(str);

	for (i = 0; str[i]; ++i)
		ZUC_ASSERT_EQ(0, islower(str[i]));
}
