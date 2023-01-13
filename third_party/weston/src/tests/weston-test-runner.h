/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Sam Spilsbury <smspillaz@gmail.com>
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

#ifndef _WESTON_TEST_RUNNER_H_
#define _WESTON_TEST_RUNNER_H_

#include "config.h"

#include <stdlib.h>

#include <wayland-util.h>
#include "shared/helpers.h"
#include "weston-test-fixture-compositor.h"
#include "weston-testsuite-data.h"

#ifdef NDEBUG
#error "Tests must not be built with NDEBUG defined, they rely on assert()."
#endif

/** Test program entry
 *
 * Each invocation of TEST(), TEST_P(), or PLUGIN_TEST() will create one
 * more weston_test_entry in a custom named section in the final binary.
 * Iterating through the section then allows to iterate through all
 * the defined tests.
 *
 * \ingroup testharness_private
 */
struct weston_test_entry {
	const char *name;
	void (*run)(void *);
	const void *table_data;
	size_t element_size;
	int n_elements;
} __attribute__ ((aligned (32)));

#define TEST_BEGIN(name, arg)						\
	static void name(arg)

#define TEST_COMMON(func, name, data, size, n_elem)			\
	static void func(void *);					\
									\
	const struct weston_test_entry test##name			\
		__attribute__ ((used, section ("test_section"))) =	\
	{								\
		#name, func, data, size, n_elem				\
	};

#define NO_ARG_TEST(name)						\
	TEST_COMMON(wrap##name, name, NULL, 0, 1)			\
	static void name(void);						\
	static void wrap##name(void *data)				\
	{								\
		(void) data;						\
		name();							\
	}								\
									\
	TEST_BEGIN(name, void)

#define ARG_TEST(name, test_data)					\
	TEST_COMMON(name, name, test_data,				\
		    sizeof(test_data[0]),				\
		    ARRAY_LENGTH(test_data))				\
	TEST_BEGIN(name, void *data)					\

/** Add a test with no parameters
 *
 * This defines one test as a new function. Use this macro in place of the
 * function signature and put the function body after this.
 *
 * \param name Name for the test, must be a valid function name.
 *
 * \ingroup testharness
 */
#define TEST(name) NO_ARG_TEST(name)

/** Add an array driven test with a parameter
 *
 * This defines an array of tests as a new function. Use this macro in place
 * of the function signature and put the function body after this. The function
 * will be executed once for each element in \c data_array, passing the
 * element as the argument <tt>void *data</tt> to the function.
 *
 * This macro is not usable if fixture setup is using
 * weston_test_harness_execute_as_plugin().
 *
 * \param name Name for the test, must be a valid function name.
 * \param data_array A static const array of any type. The length will be
 * recorded automatically.
 *
 * \ingroup testharness
 */
#define TEST_P(name, data_array) ARG_TEST(name, data_array)

/** Add a test with weston_compositor argument
 *
 * This defines one test as a new function. Use this macro in place of the
 * function signature and put the function body after this. The function
 * will have one argument <tt>struct weston_compositor *compositor</tt>.
 *
 * This macro is only usable if fixture setup is using
 * weston_test_harness_execute_as_plugin().
 *
 * \param name Name for the test, must be a valid function name.
 *
 * \ingroup testharness
 */
#define PLUGIN_TEST(name) 						\
	TEST_COMMON(wrap##name, name, NULL, 0, 1)			\
	static void name(struct weston_compositor *);			\
	static void wrap##name(void *compositor)			\
	{								\
		name(compositor);					\
	}								\
	TEST_BEGIN(name, struct weston_compositor *compositor)

void
testlog(const char *fmt, ...) WL_PRINTF(1, 2);

const char *
get_test_name(void);

int
get_test_fixture_index(void);

/** Fixture setup array record
 *
 * Helper to store the attributes of the data array passed in to
 * DECLARE_FIXTURE_SETUP_WITH_ARG().
 *
 * \ingroup testharness_private
 */
struct fixture_setup_array {
	const void *array;
	size_t element_size;
	int n_elements;
};

const struct fixture_setup_array *
fixture_setup_array_get_(void);

/** Test harness context
 *
 * \ingroup testharness
 */
struct weston_test_harness;

enum test_result_code
fixture_setup_run_(struct weston_test_harness *harness, const void *arg_);

/** Register a fixture setup function
 *
 * This registers the given (preferably static) function to be used for setting
 * up any fixtures you might need. The function must have the signature:
 *
 * \code
 * enum test_result_code func_(struct weston_test_harness *harness)
 * \endcode
 *
 * The function must call one of weston_test_harness_execute_standalone(),
 * weston_test_harness_execute_as_plugin() or
 * weston_test_harness_execute_as_client() passing in the \c harness argument,
 * and return the return value from that call. The function can also return a
 * test_result_code on its own if it does not want to run the tests,
 * e.g. RESULT_SKIP or RESULT_HARD_ERROR.
 *
 * The function will be called once to run all tests.
 *
 * \param func_ The function to be used as fixture setup.
 *
 * \ingroup testharness
 */
#define DECLARE_FIXTURE_SETUP(func_)					\
	enum test_result_code						\
	fixture_setup_run_(struct weston_test_harness *harness,		\
			   const void *arg_)				\
	{								\
		return func_(harness);					\
	}

/** Register a fixture setup function with a data array
 *
 * This registers the given (preferably static) function to be used for setting
 * up any fixtures you might need. The function must have the signature:
 *
 * \code
 * enum test_result_code func_(struct weston_test_harness *harness, typeof(array_[0]) *arg)
 * \endcode
 *
 * The function must call one of weston_test_harness_execute_standalone(),
 * weston_test_harness_execute_as_plugin() or
 * weston_test_harness_execute_as_client() passing in the \c harness argument,
 * and return the return value from that call. The function can also return a
 * test_result_code on its own if it does not want to run the tests,
 * e.g. RESULT_SKIP or RESULT_HARD_ERROR.
 *
 * The function will be called once with each element of the array pointed to
 * by \c arg, so that all tests would be repeated for each element in turn.
 *
 * \param func_ The function to be used as fixture setup.
 * \param array_ A static const array of arbitrary type.
 *
 * \ingroup testharness
 */
#define DECLARE_FIXTURE_SETUP_WITH_ARG(func_, array_)			\
	const struct fixture_setup_array *				\
	fixture_setup_array_get_(void)					\
	{								\
		static const struct fixture_setup_array arr = {		\
			.array = array_,				\
			.element_size = sizeof(array_[0]),		\
			.n_elements = ARRAY_LENGTH(array_)		\
		};							\
		return &arr;						\
	}								\
									\
	enum test_result_code						\
	fixture_setup_run_(struct weston_test_harness *harness,		\
			   const void *arg_)				\
	{								\
		typeof(array_[0]) *arg = arg_;				\
		return func_(harness, arg);				\
	}

enum test_result_code
weston_test_harness_execute_as_client(struct weston_test_harness *harness,
				      const struct compositor_setup *setup);

enum test_result_code
weston_test_harness_execute_as_plugin(struct weston_test_harness *harness,
				      const struct compositor_setup *setup);

enum test_result_code
weston_test_harness_execute_standalone(struct weston_test_harness *harness);

#endif
