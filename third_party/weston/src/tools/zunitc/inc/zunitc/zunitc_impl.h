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

#ifndef Z_UNIT_C_IMPL_H
#define Z_UNIT_C_IMPL_H

/**
 * @file
 * Internal details to bridge the public API - should not be used
 * directly in user code.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum zuc_check_op
{
	ZUC_OP_TRUE,
	ZUC_OP_FALSE,
	ZUC_OP_NULL,
	ZUC_OP_NOT_NULL,
	ZUC_OP_EQ,
	ZUC_OP_NE,
	ZUC_OP_GE,
	ZUC_OP_GT,
	ZUC_OP_LE,
	ZUC_OP_LT,
	ZUC_OP_TERMINATE,
	ZUC_OP_TRACEPOINT
};

enum zuc_check_valtype
{
	ZUC_VAL_INT,
	ZUC_VAL_CSTR,
	ZUC_VAL_PTR,
};

typedef void (*zucimpl_test_fn)(void);

typedef void (*zucimpl_test_fn_f)(void *);

/**
 * Internal use structure for automatic test case registration.
 * Should not be used directly in code.
 */
struct zuc_registration {
	const char *tcase;		/**< Name of the test case. */
	const char *test;		/**< Name of the specific test. */
	const struct zuc_fixture* fxt;	/**< Optional fixture for test/case. */
	zucimpl_test_fn fn;		/**< function implementing base test. */
	zucimpl_test_fn_f fn_f;	/**< function implementing test with
					   fixture. */
} __attribute__ ((aligned (32)));


int
zucimpl_run_tests(void);

void
zucimpl_terminate(char const *file, int line,
		  bool fail, bool fatal, const char *msg);

int
zucimpl_tracepoint(char const *file, int line, const char *fmt, ...)
	__attribute__ ((format (printf, 3, 4)));

int
zucimpl_expect_pred2(char const *file, int line,
		     enum zuc_check_op, enum zuc_check_valtype valtype,
		     bool fatal,
		     intptr_t lhs, intptr_t rhs,
		     const char *lhs_str, const char* rhs_str);

#ifdef __cplusplus
}
#endif

#endif /* Z_UNIT_C_IMPL_H */
