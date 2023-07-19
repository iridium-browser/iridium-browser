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

#ifndef ZUC_EVENT_HANDLER_H
#define ZUC_EVENT_HANDLER_H

#include <stdint.h>

#include "zuc_context.h"
#include "zuc_event.h"

struct zuc_test;
struct zuc_case;

/**
 * Interface to allow components to process testing events as they occur.
 *
 * Event listeners that will stream output as testing progresses are often
 * named "*_logger" whereas those that produce their output upon test run
 * completion are named "*_reporter".
 */
struct zuc_event_listener {
	/**
	 * User data pointer.
	 */
	void *data;

	/**
	 * Destructor.
	 * @param data the user data associated with this instance.
	 */
	void (*destroy)(void *data);

	/**
	 * Handler for simple pre-run state.
	 *
	 * @param pass_count total number of expected test passes.
	 * @param pass_num current pass iteration number.
	 * @param seed random seed being used, or 0 for no randomization.
	 * @param filter filter string used for tests, or NULL/blank for none.
	 */
	void (*pre_run)(void *data,
			int pass_count,
			int pass_num,
			int seed,
			const char *filter);

	/**
	 * Handler for test runs starting.
	 *
	 * @param data the user data associated with this instance.
	 * @param live_case_count number of active test cases in this run.
	 * @param live_test_count number of active tests in this run.
	 * @param disabled_count number of disabled tests in this run.
	 */
	void (*run_started)(void *data,
			    int live_case_count,
			    int live_test_count,
			    int disabled_count);

	/**
	 * Handler for test runs ending.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*run_ended)(void *data,
			  int case_count,
			  struct zuc_case** cases,
			  int live_case_count,
			  int live_test_count,
			  int total_passed,
			  int total_failed,
			  int total_disabled,
			  long total_elapsed);

	/**
	 * Handler for test case starting.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*case_started)(void *data,
			     struct zuc_case *test_case,
			     int live_test_count,
			     int disabled_count);

	/**
	 * Handler for test case ending.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*case_ended)(void *data,
			   struct zuc_case *test_case);

	/**
	 * Handler for test starting.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*test_started)(void *data,
			     struct zuc_test *test);

	/**
	 * Handler for test ending.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*test_ended)(void *data,
			   struct zuc_test *test);

	/**
	 * Handler for disabled test notification.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*test_disabled)(void *data,
			      struct zuc_test *test);

	/**
	 * Handler for check/assertion fired due to failure, warning, etc.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*check_triggered)(void *data,
				char const *file,
				int line,
				enum zuc_fail_state state,
				enum zuc_check_op op,
				enum zuc_check_valtype valtype,
				intptr_t val1,
				intptr_t val2,
				const char *expr1,
				const char *expr2);

	/**
	 * Handler for tracepoints and such that may be displayed later.
	 *
	 * @param data the user data associated with this instance.
	 */
	void (*collect_event)(void *data,
			      char const *file,
			      int line,
			      const char *expr1);
};

/**
 * Registers an event listener instance to be called.
 */
void
zuc_add_event_listener(struct zuc_event_listener *event_listener);


#endif /* ZUC_EVENT_HANDLER_H */
