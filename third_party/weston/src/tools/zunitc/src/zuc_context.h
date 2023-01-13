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

#ifndef ZUC_CONTEXT_H
#define ZUC_CONTEXT_H

#include "zuc_types.h"

struct zuc_slinked;

/**
 * Internal context for processing.
 * Collecting data members here minimizes use of globals.
 */
struct zuc_context {
	int case_count;
	struct zuc_case **cases;

	bool fatal;
	int repeat;
	int random;
	unsigned int seed;
	bool spawn;
	bool break_on_failure;
	bool output_tap;
	bool output_junit;
	int fds[2];
	char *filter;

	struct zuc_slinked *listeners;

	struct zuc_case *curr_case;
	struct zuc_test *curr_test;
};

#endif /* ZUC_CONTEXT_H */
