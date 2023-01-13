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

#ifndef ZUC_TYPES_H
#define ZUC_TYPES_H

#include "zunitc/zunitc_impl.h"

struct zuc_case;

/**
 * Represents a specific test.
 */
struct zuc_test
{
	int order;
	struct zuc_case *test_case;
	zucimpl_test_fn fn;
	zucimpl_test_fn_f fn_f;
	char *name;
	int disabled;
	int skipped;
	int failed;
	int fatal;
	long elapsed;
	struct zuc_event *events;
	struct zuc_event *deferred;
};

/**
 * Represents a test case that can hold a collection of tests.
 */
struct zuc_case
{
	int order;
	char *name;
	const struct zuc_fixture* fxt;
	int disabled;
	int skipped;
	int failed;
	int fatal;
	int passed;
	long elapsed;
	int test_count;
	struct zuc_test **tests;
};

/**
 * Returns a human-readable version of the comparison opcode.
 *
 * @param op the opcode to get a string version of.
 * @return a human-readable string of the opcode.
 * (This value should not be freed)
 */
const char *
zuc_get_opstr(enum zuc_check_op op);

#endif /* ZUC_TYPES_H */
