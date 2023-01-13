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

#ifndef Z_UNIT_C_H
#define Z_UNIT_C_H

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "zunitc/zunitc_impl.h"

#if !__GNUC__
#error Framework currently requires gcc or compatible compiler.
#endif

#if INTPTR_MAX < INT_MAX
#error Odd platform requires rework of value type from intptr_t to custom.
#endif

/**
 * @file
 * Simple unit test framework declarations.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @page zunitc
 */

/**
 * Structure to use when defining a test fixture.
 * @note likely pending refactoring as use cases are refined.
 * @see ZUC_TEST_F()
 */
struct zuc_fixture {
	/**
	 * Initial optional seed data to pass to setup functions and/or tests.
	 */
	const void *data;

	/**
	 * Per-suite setup called before invoking any of the tests
	 * contained in the suite.
	 *
	 * @return a pointer to test data, or NULL.
	 */
	void *(*set_up_test_case)(const void *data);

	/**
	 * Per-suite tear-down called after invoking all of the tests
	 * contained in the suite.
	 *
	 * @param data pointer returned from the setup function.
	 */
	void (*tear_down_test_case)(void *data);

	/**
	 * Setup called before running each of the tests in the suite.
	 *
	 * @param data optional data from suite setup, or NULL.
	 * @return a pointer to test data, or NULL.
	 */
	void *(*set_up)(void *data);

	/**
	 * Tear-down called after running each of the tests in the suite.
	 *
	 * @param data pointer returned from the setup function.
	 */
	void (*tear_down)(void *data);
};

/**
 * Process exit code to mark skipped tests, consistent with
 * automake tests.
 */
#define ZUC_EXIT_SKIP 77

/**
 * Accesses the test executable program name.
 * This version will include any full or partial path used to
 * launch the executable.
 *
 * @note This depends on zuc_initialize() having been called.
 *
 * @return the name of the running program.
 * The caller should not free nor hold this pointer. It will not stay
 * valid across calls to zuc_initialize() or zuc_cleanup().
 * @see zuc_get_program_basename()
 */
const char *
zuc_get_program_name(void);

/**
 * Accesses the test executable program name in trimmed format.
 * If the program is launched via a partial or full path, this
 * version trims to return only the basename.
 *
 * @note This depends on zuc_initialize() having been called.
 *
 * @return the name of the running program.
 * The caller should not free nor hold this pointer. It will not stay
 * valid across calls to zuc_initialize() or zuc_cleanup().
 * @see zuc_get_program_name()
 */
const char *
zuc_get_program_basename(void);

/**
 * Initializes the test framework and consumes any known command-line
 * parameters from the list.
 * The exception is 'h/help' which will be left in place for follow-up
 * processing by the hosting app if so desired.
 *
 * @param argc pointer to argc value to read and possibly change.
 * @param argv array of parameter pointers to read and possibly change.
 * @param help_flagged if non-NULL will be set to true if the user
 * specifies the help flag (and framework help has been output).
 * @return EXIT_SUCCESS upon success setting or help, EXIT_FAILURE otherwise.
 */
int zuc_initialize(int *argc, char *argv[], bool *help_flagged);

/**
 * Runs all tests that have been registered.
 * Expected return values include EXIT_FAILURE if any errors or failures
 * have occurred, ::ZUC_EXIT_SKIP if no failures have occurred but at least
 * one test reported skipped, otherwise EXIT_SUCCESS if nothing of note
 * was recorded.
 *
 * @note for consistency with other frameworks and to allow for additional
 * cleanup to be added later this is implemented as a wrapper macro.
 *
 * @return expected exit status - normally EXIT_SUCCESS, ::ZUC_EXIT_SKIP,
 * or EXIT_FAILURE.
 * Normally an application can use this value directly in calling exit(),
 * however there could be cases where some additional processing such as
 * resource cleanup or library shutdown that a program might want to do
 * first.
 */
#define ZUC_RUN_TESTS() \
	zucimpl_run_tests()

/**
 * Clears the test system in preparation for application shutdown.
 */
void
zuc_cleanup(void);

/**
 * Displays all known tests.
 * The list returned is affected by any filtering in place.
 *
 * @see zuc_set_filter()
 */
void
zuc_list_tests(void);

/**
 * Sets the filter string to use for tests.
 * The format is a series of patterns separated by a colon, with wildcards
 * and an optional flag for negative matching. For wildcards, the '*'
 * character will match any sequence and the '?' character will match any
 * single character.
 * The '-' character at the start of a pattern marks the end of the
 * patterns required to match and the beginning of patterns that names
 * must not match.
 * Defaults to use all tests.
 *
 * @param filter the filter string to apply to tests.
 */
void
zuc_set_filter(const char *filter);

/**
 * Trigger specific failure/signal upon test failures; useful when
 * running under a debugger.
 * Currently this is implemented to raise a SIGABRT signal when any
 * failure is reported.
 * Defaults to false.
 *
 * @param break_on_failure true to cause a break when tests fail, false to
 * allow normal operation upon failures.
 */
void
zuc_set_break_on_failure(bool break_on_failure);

/**
 * Sets the number of times to repeat the tests.
 * Any number higher than 1 will cause the tests to be repeated the
 * specified number of times.
 * Defaults to 1/no repeating.
 *
 * @param repeat number of times to repeat the tests.
 */
void
zuc_set_repeat(int repeat);

/**
 * Randomizes the order in which tests are executed.
 * A value of 0 (the default) means tests are executed in their natural
 * ordering. A value of 1 will pick a random seed based on the time to
 * use for running tests in a pseudo-random order. A value greater than 1
 * will be used directly for the initial seed.
 *
 * If the tests are also repeated, the seed will be incremented for each
 * subsequent run.
 * Defaults to 0/not randomize.
 *
 * @param random 0|1|seed value.
 * @see zuc_set_repeat()
 */
void
zuc_set_random(int random);

/**
 * Controls whether or not to run the tests as forked child processes.
 * Defaults to true.
 *
 * @param spawn true to spawn each test in a forked child process,
 * false to run tests directly.
 */
void
zuc_set_spawn(bool spawn);

/**
 * Enables output in the JUnit XML format.
 * Defaults to false.
 *
 * @param enable true to generate JUnit XML output, false to disable.
 */
void
zuc_set_output_junit(bool enable);

/**
 * Defines a test case that can be registered to run.
 *
 * @param tcase name to use as the containing test case.
 * @param test name used for the test under a given test case.
 */
#define ZUC_TEST(tcase, test) \
	static void zuctest_##tcase##_##test(void); \
	\
	const struct zuc_registration zzz_##tcase##_##test \
	__attribute__ ((used, section ("zuc_tsect"))) = \
	{ \
		#tcase, #test, 0,		\
		zuctest_##tcase##_##test,	\
		0				\
	}; \
	\
	static void zuctest_##tcase##_##test(void)

/**
 * Defines a test case that can be registered to run along with setup/teardown
 * support per-test and/or per test case.
 *
 * @note This defines a test that *uses* a fixture, it does not
 * actually define a test fixture itself.
 *
 * @param tcase name to use as the containing test case/fixture.
 * The name used must represent a test fixture instance. It also
 * must not duplicate any name used in a non-fixture ZUC_TEST()
 * test.
 * @note the test case name must be the name of a fixture struct
 * to be passed to the test.
 * @param test name used for the test under a given test case.
 * @param param name for the fixture data pointer.
 * @see struct zuc_fixture
 */
#define ZUC_TEST_F(tcase, test, param)			  \
	static void zuctest_##tcase##_##test(void *param); \
	\
	const struct zuc_registration zzz_##tcase##_##test \
	__attribute__ ((used, section ("zuc_tsect"))) = \
	{ \
		#tcase, #test, &tcase,		\
		0,				\
		zuctest_##tcase##_##test	\
	}; \
	\
	static void zuctest_##tcase##_##test(void *param)


/**
 * Returns true if the currently executing test has encountered any skips.
 *
 * @return true if there is currently a test executing and it has
 * encountered any skips.
 * @see zuc_has_failure
 * @see ZUC_SKIP()
 */
bool
zuc_has_skip(void);

/**
 * Returns true if the currently executing test has encountered any failures.
 *
 * @return true if there is currently a test executing and it has
 * encountered any failures.
 * @see zuc_has_skip
 */
bool
zuc_has_failure(void);

/**
 * Marks the running test as skipped without marking it as failed, and returns
 * from the current function.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param message the message to log as to why the test has been skipped.
 */
#define ZUC_SKIP(message) \
	do { \
		zucimpl_terminate(__FILE__, __LINE__, false, false, #message); \
		return; \
	} \
	while (0)

/**
 * Marks the running test as failed and returns from the current function.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param message the message to log as to why the test has failed.
 */
#define ZUC_FATAL(message) \
	do { \
		zucimpl_terminate(__FILE__, __LINE__, true, true, #message); \
		return; \
	} \
	while (0)

/**
 * Marks the current test as failed with a fatal issue, but does not
 * immediately return from the current function. ZUC_FATAL() is normally
 * preferred, but when further cleanup is needed, or the current function
 * needs to return a value, this macro may be required.
 *
 * @param message the message to log as to why the test has failed.
 * @see ZUC_FATAL()
 */
#define ZUC_MARK_FATAL(message) \
	do { \
		zucimpl_terminate(__FILE__, __LINE__, true, true, #message); \
	} \
	while (0)

/**
 * Creates a message that will be processed in the case of failure.
 * If the test encounters any failures (fatal or non-fatal) then these
 * messages are included in output. Otherwise they are discarded at the
 * end of the test run.
 *
 * @param message the format string style message.
 */
#define ZUC_TRACEPOINT(message, ...) \
	zucimpl_tracepoint(__FILE__, __LINE__, message, ##__VA_ARGS__);

/**
 * Internal use macro for ASSERT implementation.
 * Should not be used directly in code.
 */
#define ZUCIMPL_ASSERT(opcode, valtype, lhs, rhs) \
	do { \
		if (zucimpl_expect_pred2(__FILE__, __LINE__, \
					 (opcode), (valtype), true, \
					 (intptr_t)(lhs), (intptr_t)(rhs), \
					 #lhs, #rhs)) { \
			return; \
		} \
	} \
	while (0)

/**
 * Internal use macro for ASSERT with Goto implementation.
 * Should not be used directly in code.
 */
#define ZUCIMPL_ASSERTG(label, opcode, valtype, lhs, rhs)	\
	do { \
		if (zucimpl_expect_pred2(__FILE__, __LINE__, \
					 (opcode), (valtype), true, \
					 (intptr_t)(lhs), (intptr_t)(rhs), \
					 #lhs, #rhs)) { \
			goto label; \
		} \
	} \
	while (0)

/**
 * Verifies that the specified expression is true, marks the test as failed
 * and exits the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param condition the expression that is expected to be true.
 * @note it is far better to use a more specific check when possible
 * (e.g. ZUC_ASSERT_EQ(), ZUC_ASSERT_NE(), etc.)
 * @see ZUC_ASSERTG_TRUE()
 */
#define ZUC_ASSERT_TRUE(condition) \
	ZUCIMPL_ASSERT(ZUC_OP_TRUE, ZUC_VAL_INT, condition, 0)

/**
 * Verifies that the specified expression is false, marks the test as
 * failed and exits the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param condition the expression that is expected to be false.
 * @note it is far better to use a more specific check when possible
 * (e.g. ZUC_ASSERT_EQ(), ZUC_ASSERT_NE(), etc.)
 * @see ZUC_ASSERTG_FALSE()
 */
#define ZUC_ASSERT_FALSE(condition) \
	ZUCIMPL_ASSERT(ZUC_OP_FALSE, ZUC_VAL_INT, condition, 0)

/**
 * Verifies that the specified expression is NULL, marks the test as failed
 * and exits the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param condition the expression that is expected to be a NULL pointer.
 * @see ZUC_ASSERTG_NULL()
 */
#define ZUC_ASSERT_NULL(condition) \
	ZUCIMPL_ASSERT(ZUC_OP_NULL, ZUC_VAL_PTR, condition, 0)

/**
 * Verifies that the specified expression is non-NULL, marks the test as
 * failed and exits the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param condition the expression that is expected to be a non-NULL pointer.
 * @see ZUC_ASSERTG_NOT_NULL()
 */
#define ZUC_ASSERT_NOT_NULL(condition) \
	ZUCIMPL_ASSERT(ZUC_OP_NOT_NULL, ZUC_VAL_PTR, condition, 0)

/**
 * Verifies that the values of the specified expressions match, marks the
 * test as failed and exits the current function via 'return' if they do not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param expected the value the result should hold.
 * @param actual the actual value seen in testing.
 * @see ZUC_ASSERTG_EQ()
 */
#define ZUC_ASSERT_EQ(expected, actual) \
	ZUCIMPL_ASSERT(ZUC_OP_EQ, ZUC_VAL_INT, expected, actual)

/**
 * Verifies that the values of the specified expressions differ, marks the
 * test as failed and exits the current function via 'return' if they do not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param expected the value the result should not hold.
 * @param actual the actual value seen in testing.
 * @see ZUC_ASSERTG_NE()
 */
#define ZUC_ASSERT_NE(expected, actual) \
	ZUCIMPL_ASSERT(ZUC_OP_NE, ZUC_VAL_INT, expected, actual)

/**
 * Verifies that the value of the first expression is less than the value
 * of the second expression, marks the test as failed and exits the current
 * function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param lesser the expression whose value should be lesser than the other.
 * @param greater the expression whose value should be greater than the other.
 * @see ZUC_ASSERTG_LT()
 */
#define ZUC_ASSERT_LT(lesser, greater) \
	ZUCIMPL_ASSERT(ZUC_OP_LT, ZUC_VAL_INT, lesser, greater)

/**
 * Verifies that the value of the first expression is less than or equal
 * to the value of the second expression, marks the test as failed and
 * exits the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param lesser the expression whose value should be lesser than or equal to
 * the other.
 * @param greater the expression whose value should be greater than or equal to
 * the other.
 * @see ZUC_ASSERTG_LE()
 */
#define ZUC_ASSERT_LE(lesser, greater) \
	ZUCIMPL_ASSERT(ZUC_OP_LE, ZUC_VAL_INT, lesser, greater)

/**
 * Verifies that the value of the first expression is greater than the
 * value of the second expression, marks the test as failed and exits the
 * current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param greater the expression whose value should be greater than the other.
 * @param lesser the expression whose value should be lesser than the other.
 * @see ZUC_ASSERTG_GT()
 */
#define ZUC_ASSERT_GT(greater, lesser) \
	ZUCIMPL_ASSERT(ZUC_OP_GT, ZUC_VAL_INT, greater, lesser)

/**
 * Verifies that the value of the first expression is greater than or equal
 * to the value of the second expression, marks the test as failed and exits
 * the current function via 'return' if it is not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param greater the expression whose value should be greater than or equal to
 * the other.
 * @param lesser the expression whose value should be lesser than or equal to
 * the other.
 * @see ZUC_ASSERTG_GE()
 */
#define ZUC_ASSERT_GE(greater, lesser) \
	ZUCIMPL_ASSERT(ZUC_OP_GE, ZUC_VAL_INT, greater, lesser)

/**
 * Verifies that the values of the specified expressions match when
 * compared as null-terminated C-style strings, marks the test as failed
 * and exits the current function via 'return' if they do not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param expected the value the result should hold.
 * @param actual the actual value seen in testing.
 * @see ZUC_ASSERTG_STREQ()
 */
#define ZUC_ASSERT_STREQ(expected, actual) \
	ZUCIMPL_ASSERT(ZUC_OP_EQ, ZUC_VAL_CSTR, expected, actual)

/**
 * Verifies that the values of the specified expressions differ when
 * compared as null-terminated C-style strings, marks the test as failed
 * and exits the current function via 'return' if they do not.
 *
 * For details on return and test termination see @ref zunitc_overview_return.
 *
 * @param expected the value the result should not hold.
 * @param actual the actual value seen in testing.
 * @see ZUC_ASSERTG_STRNE()
 */
#define ZUC_ASSERT_STRNE(expected, actual) \
	ZUCIMPL_ASSERT(ZUC_OP_NE, ZUC_VAL_CSTR, expected, actual)

/**
 * Verifies that the specified expression is true, marks the test as failed
 * and interrupts the current execution via a 'goto' if it is not.
 *
 * @param condition the expression that is expected to be true.
 * @note it is far better to use a more specific check when possible
 * (e.g. ZUC_ASSERTG_EQ(), ZUC_ASSERTG_NE(), etc.)
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_TRUE()
 */
#define ZUC_ASSERTG_TRUE(condition, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_TRUE, ZUC_VAL_INT, condition, 0)

/**
 * Verifies that the specified expression is false, marks the test as
 * failed and interrupts the current execution via a 'goto' if it is not.
 *
 * @param condition the expression that is expected to be false.
 * @note it is far better to use a more specific check when possible
 * (e.g. ZUC_ASSERTG_EQ(), ZUC_ASSERTG_NE(), etc.)
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_FALSE()
 */
#define ZUC_ASSERTG_FALSE(condition, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_FALSE, ZUC_VAL_INT, condition, 0)

/**
 * Verifies that the specified expression is NULL, marks the test as failed
 * and interrupts the current execution via a 'goto' if it is not.
 *
 * @param condition the expression that is expected to be a NULL pointer.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_NULL()
 */
#define ZUC_ASSERTG_NULL(condition, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_NULL, ZUC_VAL_PTR, condition, 0)

/**
 * Verifies that the specified expression is non-NULL, marks the test as
 * failed and interrupts the current execution via a 'goto' if it is not.
 *
 * @param condition the expression that is expected to be a non-NULL pointer.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_NOT_NULL()
 */
#define ZUC_ASSERTG_NOT_NULL(condition, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_NOT_NULL, ZUC_VAL_PTR, condition, 0)

/**
 * Verifies that the values of the specified expressions match, marks the
 * test as failed and interrupts the current execution via a 'goto' if they
 * do not.
 *
 * @param expected the value the result should hold.
 * @param actual the actual value seen in testing.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_EQ()
 */
#define ZUC_ASSERTG_EQ(expected, actual, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_EQ, ZUC_VAL_INT, expected, actual)

/**
 * Verifies that the values of the specified expressions differ, marks the
 * test as failed and interrupts the current execution via a 'goto' if they
 * do not.
 *
 * @param expected the value the result should not hold.
 * @param actual the actual value seen in testing.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_NE()
 */
#define ZUC_ASSERTG_NE(expected, actual, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_NE, ZUC_VAL_INT, expected, actual)

/**
 * Verifies that the value of the first expression is less than the value
 * of the second expression, marks the test as failed and interrupts the
 * current execution via a 'goto' if it is not.
 *
 * @param lesser the expression whose value should be lesser than the other.
 * @param greater the expression whose value should be greater than the other.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_LT()
 */
#define ZUC_ASSERTG_LT(lesser, greater, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_LT, ZUC_VAL_INT, lesser, greater)

/**
 * Verifies that the value of the first expression is less than or equal
 * to the value of the second expression, marks the test as failed and
 * interrupts the current execution via a 'goto' if it is not.
 *
 * @param lesser the expression whose value should be lesser than or equal to
 * the other.
 * @param greater the expression whose value should be greater than or equal to
 * the other.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_LE()
 */
#define ZUC_ASSERTG_LE(lesser, greater, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_LE, ZUC_VAL_INT, lesser, greater)

/**
 * Verifies that the value of the first expression is greater than the
 * value of the second expression, marks the test as failed and interrupts the
 * current execution via a 'goto' if it is not.
 *
 * @param greater the expression whose value should be greater than the other.
 * @param lesser the expression whose value should be lesser than the other.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_GT()
 */
#define ZUC_ASSERTG_GT(greater, lesser, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_GT, ZUC_VAL_INT, greater, lesser)

/**
 * Verifies that the value of the first expression is greater than or equal
 * to the value of the second expression, marks the test as failed and
 * interrupts the current execution via a 'goto' if it is not.
 *
 * @param greater the expression whose value should be greater than or equal to
 * the other.
 * @param lesser the expression whose value should be lesser than or equal to
 * the other.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_GE()
 */
#define ZUC_ASSERTG_GE(greater, lesser, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_GE, ZUC_VAL_INT, greater, lesser)

/**
 * Verifies that the values of the specified expressions match when
 * compared as null-terminated C-style strings, marks the test as failed
 * and interrupts the current execution via a 'goto' if they do not.
 *
 * @param expected the value the result should hold.
 * @param actual the actual value seen in testing.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_STREQ()
 */
#define ZUC_ASSERTG_STREQ(expected, actual, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_EQ, ZUC_VAL_CSTR, expected, actual)

/**
 * Verifies that the values of the specified expressions differ when
 * compared as null-terminated C-style strings, marks the test as failed
 * and interrupts the current execution via a 'goto' if they do not.
 *
 * @param expected the value the result should not hold.
 * @param actual the actual value seen in testing.
 * @param label the target for 'goto' if the assertion fails.
 * @see ZUC_ASSERT_STRNE()
 */
#define ZUC_ASSERTG_STRNE(expected, actual, label) \
	ZUCIMPL_ASSERTG(label, ZUC_OP_NE, ZUC_VAL_CSTR, expected, actual)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* Z_UNIT_C_H */
