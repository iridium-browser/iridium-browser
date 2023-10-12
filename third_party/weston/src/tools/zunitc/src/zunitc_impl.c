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

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "zunitc/zunitc_impl.h"
#include "zunitc/zunitc.h"

#include "zuc_base_logger.h"
#include "zuc_context.h"
#include "zuc_event_listener.h"
#include "zuc_junit_reporter.h"

#include <libweston/config-parser.h>
#include "shared/helpers.h"
#include <libweston/zalloc.h>

/*
 * If CLOCK_MONOTONIC is present on the system it will give us reliable
 * results under certain edge conditions that normally require manual
 * admin actions to trigger. If not, CLOCK_REALTIME is a reasonable
 * fallback.
 */
#if _POSIX_MONOTONIC_CLOCK
static const clockid_t TARGET_TIMER = CLOCK_MONOTONIC;
#else
static const clockid_t TARGET_TIMER = CLOCK_REALTIME;
#endif

static char const DISABLED_PREFIX[] = "DISABLED_";

#define MS_PER_SEC 1000L
#define NANO_PER_MS 1000000L

/**
 * Simple single-linked list structure.
 */
struct zuc_slinked {
	void *data;
	struct zuc_slinked *next;
};

static struct zuc_context g_ctx = {
	.case_count = 0,
	.cases = NULL,

	.fatal = false,
	.repeat = 0,
	.random = 0,
	.break_on_failure = false,

	.listeners = NULL,

	.curr_case = NULL,
	.curr_test = NULL,
};

static char *g_progname = NULL;
static char *g_progbasename = NULL;

typedef int (*comp_pred2)(intptr_t lhs, intptr_t rhs);

static bool
test_has_skip(struct zuc_test *test)
{
	return test->skipped;
}

static bool
test_has_failure(struct zuc_test *test)
{
	return test->fatal || test->failed;
}

bool
zuc_has_skip(void)
{
	return g_ctx.curr_test ?
		test_has_skip(g_ctx.curr_test) : false;
}

bool
zuc_has_failure(void)
{
	return g_ctx.curr_test ?
		test_has_failure(g_ctx.curr_test) : false;
}

void
zuc_set_filter(const char *filter)
{
	g_ctx.filter = strdup(filter);
}

void
zuc_set_repeat(int repeat)
{
	g_ctx.repeat = repeat;
}

void
zuc_set_random(int random)
{
	g_ctx.random = random;
}

void
zuc_set_break_on_failure(bool break_on_failure)
{
	g_ctx.break_on_failure = break_on_failure;
}

void
zuc_set_output_junit(bool enable)
{
	g_ctx.output_junit = enable;
}

const char *
zuc_get_program_name(void)
{
	return g_progname;
}

const char *
zuc_get_program_basename(void)
{
	return g_progbasename;
}

static struct zuc_test *
create_test(int order, zucimpl_test_fn fn, zucimpl_test_fn_f fn_f,
	    char const *case_name, char const *test_name,
	    struct zuc_case *parent)
{
	struct zuc_test *test = zalloc(sizeof(struct zuc_test));
	ZUC_ASSERTG_NOT_NULL(test, out);
	test->order = order;
	test->fn = fn;
	test->fn_f = fn_f;
	test->name = strdup(test_name);
	if ((!fn && !fn_f) ||
	    (strncmp(DISABLED_PREFIX,
		     test_name, sizeof(DISABLED_PREFIX) - 1) == 0))
		test->disabled = 1;

	test->test_case = parent;

out:
	return test;
}

static int
compare_regs(const void *lhs, const void *rhs)
{
	int rc = strcmp((*(struct zuc_registration **)lhs)->tcase,
		      (*(struct zuc_registration **)rhs)->tcase);
	if (rc == 0)
		rc = strcmp((*(struct zuc_registration **)lhs)->test,
		      (*(struct zuc_registration **)rhs)->test);

	return rc;
}

/* gcc-specific markers for auto test case registration: */
extern const struct zuc_registration __start_zuc_tsect;
extern const struct zuc_registration __stop_zuc_tsect;

static void
register_tests(void)
{
	size_t case_count = 0;
	size_t count = &__stop_zuc_tsect - &__start_zuc_tsect;
	size_t i;
	int idx = 0;
	const char *last_name = NULL;
	void **array = zalloc(sizeof(void *) * count);
	ZUC_ASSERT_NOT_NULL(array);
	for (i = 0; i < count; ++i)
		array[i] = (void *)(&__start_zuc_tsect + i);

	qsort(array, count, sizeof(array[0]), compare_regs);

	/* Count transitions to get number of test cases. */
	last_name = NULL;
	for (i = 0; i < count; ++i) {
		const struct zuc_registration *reg =
			(const struct zuc_registration *)array[i];
		if (!last_name || (strcmp(last_name, reg->tcase))) {
			last_name = reg->tcase;
			case_count++;
		}
	}

	/* Create test case data items. */
	struct zuc_case **case_array =
		zalloc(sizeof(struct zuc_case *) * case_count);
	ZUC_ASSERT_NOT_NULL(case_array);
	struct zuc_case *last_case = NULL;
	size_t case_num = 0;
	for (i = 0; i < count; ++i) {
		const struct zuc_registration *reg =
			(const struct zuc_registration *)array[i];
		if (!last_case || (strcmp(last_case->name, reg->tcase))) {
			last_case = zalloc(sizeof(struct zuc_case));
			ZUC_ASSERT_NOT_NULL(last_case);
			last_case->order = count;
			last_case->name = strdup(reg->tcase);
			last_case->fxt = reg->fxt;
			last_case->test_count = i;
			if (case_num > 0) {
				int tcount = i
					- case_array[case_num - 1]->test_count;
				case_array[case_num - 1]->test_count = tcount;
			}
			case_array[case_num++] = last_case;
		}
	}
	case_array[case_count - 1]->test_count = count
		- case_array[case_count - 1]->test_count;

	/* Reserve space for tests per test case. */
	for (case_num = 0; case_num < case_count; ++case_num) {
		case_array[case_num]->tests =
			zalloc(case_array[case_num]->test_count
			       * sizeof(struct zuc_test *));
		ZUC_ASSERT_NOT_NULL(case_array[case_num]->tests);
	}

	last_name = NULL;
	case_num = -1;
	for (i = 0; i < count; ++i) {
		const struct zuc_registration *reg =
			(const struct zuc_registration *)array[i];
		int order = count - (1 + (reg - &__start_zuc_tsect));

		if (!last_name || (strcmp(last_name, reg->tcase))) {
			last_name = reg->tcase;
			case_num++;
			idx = 0;
		}
		if (order < case_array[case_num]->order)
			case_array[case_num]->order = order;
		case_array[case_num]->tests[idx] =
			create_test(order, reg->fn, reg->fn_f,
				    reg->tcase, reg->test,
				    case_array[case_num]);

		if (case_array[case_num]->fxt != reg->fxt)
			printf("%s:%d: error: Mismatched fixtures for '%s'\n",
			       __FILE__, __LINE__, case_array[case_num]->name);

		idx++;
	}
	free(array);

	g_ctx.case_count = case_count;
	g_ctx.cases = case_array;
}

static int
compare_case_order(const void *lhs, const void *rhs)
{
	return (*(struct zuc_case **)lhs)->order
		- (*(struct zuc_case **)rhs)->order;
}

static int
compare_test_order(const void *lhs, const void *rhs)
{
	return (*(struct zuc_test **)lhs)->order
		- (*(struct zuc_test **)rhs)->order;
}

static void
order_cases(int count, struct zuc_case **cases)
{
	int i;
	qsort(cases, count, sizeof(*cases), compare_case_order);
	for (i = 0; i < count; ++i) {
		qsort(cases[i]->tests, cases[i]->test_count,
		      sizeof(*cases[i]->tests), compare_test_order);
	}
}

static void
free_events(struct zuc_event **events)
{
	struct zuc_event *evt = *events;
	*events = NULL;
	while (evt) {
		struct zuc_event *old = evt;
		evt = evt->next;
		free(old->file);
		if (old->valtype == ZUC_VAL_CSTR) {
			free((void *)old->val1);
			free((void *)old->val2);
		}
		free(old->expr1);
		free(old->expr2);
		free(old);
	}
}

static void
free_test(struct zuc_test *test)
{
	free(test->name);
	free_events(&test->events);
	free_events(&test->deferred);
	free(test);
}

static void
free_test_case(struct zuc_case *test_case)
{
	int i;
	free(test_case->name);
	for (i = test_case->test_count - 1; i >= 0; --i) {
		free_test(test_case->tests[i]);
		test_case->tests[i] = NULL;
	}
	free(test_case->tests);
	free(test_case);
}

/**
 * A very simple matching that is compatible with the algorithm used in
 * Google Test.
 *
 * @param wildcard sequence of '?', '*' or normal characters to match.
 * @param str string to check for matching.
 * @return true if the wildcard matches the entire string, false otherwise.
 */
static bool
wildcard_matches(char const *wildcard, char const *str)
{
	switch (*wildcard) {
	case '\0':
		return !*str;
	case '?':
		return *str && wildcard_matches(wildcard + 1, str + 1);
	case '*':
		return (*str && wildcard_matches(wildcard, str + 1))
			|| wildcard_matches(wildcard + 1, str);
	default:
		return (*wildcard == *str)
			&& wildcard_matches(wildcard + 1, str + 1);
	};
}

static char**
segment_str(char *str)
{
	int count = 1;
	char **parts = NULL;
	char *saved = NULL;
	char *tok = NULL;
	int i = 0;
	for (i = 0; str[i]; ++i)
		if (str[i] == ':')
			count++;
	parts = zalloc(sizeof(char*) * (count + 1));
	ZUC_ASSERTG_NOT_NULL(parts, out);
	tok = strtok_r(str, ":", &saved);
	i = 0;
	parts[i++] = tok;
	while (tok) {
		tok = strtok_r(NULL, ":", &saved);
		parts[i++] = tok;
	}
out:
	return parts;
}

static void
filter_cases(int *count, struct zuc_case **cases, char const *filter)
{
	int i = 0;
	int j = 0;
	int num_pos = 0;
	int negative = -1;
	char *buf = strdup(filter);
	char **parts = segment_str(buf);

	for (i = 0; parts[i]; ++i) {
		if (parts[i][0] == '-') {
			parts[i]++;
			negative = i;
			break;
		}
		num_pos++;
	}

	for (i = 0; i < *count; ++i) {
		/* Walk backwards for easier pruning. */
		for (j = cases[i]->test_count - 1; j >= 0; --j) {
			int x;
			bool keep = num_pos == 0;
			char *name = NULL;
			struct zuc_test *test = cases[i]->tests[j];
			if (asprintf(&name, "%s.%s", cases[i]->name,
				     test->name) < 0) {
				printf("%s:%d: error: %d\n", __FILE__, __LINE__,
				       errno);
				exit(EXIT_FAILURE);
			}
			for (x = 0; (x < num_pos) && !keep; ++x)
				keep = wildcard_matches(parts[x], name);
			if (keep && (negative >= 0))
				for (x = negative; parts[x] && keep; ++x)
					keep &= !wildcard_matches(parts[x],
								  name);
			if (!keep) {
				int w;
				free_test(test);
				for (w = j + 1; w < cases[i]->test_count; w++)
					cases[i]->tests[w - 1] =
						cases[i]->tests[w];
				cases[i]->test_count--;
			}

			free(name);
		}
	}
	free(parts);
	parts = NULL;
	free(buf);
	buf = NULL;

	/* Prune any cases with no more tests. */
	for (i = *count - 1; i >= 0; --i) {
		if (cases[i]->test_count < 1) {
			free_test_case(cases[i]);
			for (j = i + 1; j < *count; ++j)
				cases[j - 1] = cases[j];
			cases[*count - 1] = NULL;
			(*count)--;
		}
	}
}

static unsigned int
get_seed_from_time(void)
{
	time_t sec = time(NULL);
	unsigned int seed = (unsigned int) sec % 100000;
	if (seed < 2)
		seed = 2;

	return seed;
}

static void
initialize(void)
{
	static bool init = false;
	if (init)
		return;

	init = true;
	register_tests();
	if (g_ctx.fatal)
		return;

	if (g_ctx.random > 1) {
		g_ctx.seed = g_ctx.random;
	} else if (g_ctx.random == 1) {
		g_ctx.seed = get_seed_from_time();
	}

	if (g_ctx.case_count) {
		order_cases(g_ctx.case_count, g_ctx.cases);
		if (g_ctx.filter && g_ctx.filter[0])
			filter_cases(&g_ctx.case_count, g_ctx.cases,
				     g_ctx.filter);
	}
}

int
zuc_initialize(int *argc, char *argv[], bool *help_flagged)
{
	int rc = EXIT_FAILURE;
	bool opt_help = false;
	bool opt_list = false;
	int opt_repeat = 0;
	int opt_random = 0;
	bool opt_break_on_failure = false;
	bool opt_junit = false;
	char *opt_filter = NULL;

	char *help_param = NULL;
	int argc_in = *argc;

	const struct weston_option options[] = {
		{ WESTON_OPTION_BOOLEAN, "zuc-list-tests", 0, &opt_list },
		{ WESTON_OPTION_INTEGER, "zuc-repeat", 0, &opt_repeat },
		{ WESTON_OPTION_INTEGER, "zuc-random", 0, &opt_random },
		{ WESTON_OPTION_BOOLEAN, "zuc-break-on-failure", 0,
		  &opt_break_on_failure },
#if ENABLE_JUNIT_XML
		{ WESTON_OPTION_BOOLEAN, "zuc-output-xml", 0, &opt_junit },
#endif
		{ WESTON_OPTION_STRING, "zuc-filter", 0, &opt_filter },
	};

	/*
	 *If a test binary is linked to our libzunitcmain it might want
	 * to access the program 'name' from argv[0]
	 */
	free(g_progname);
	g_progname = NULL;
	free(g_progbasename);
	g_progbasename = NULL;
	if ((*argc > 0) && argv) {
		char *path = NULL;
		char *base = NULL;

		g_progname = strdup(argv[0]);

		/* basename() might modify the input, so needs a writeable
		 * string.
		 * It also may return a statically allocated buffer subject to
		 * being overwritten so needs to be dup'd.
		 */
		path = strdup(g_progname);
		base = basename(path);
		g_progbasename = strdup(base);
		free(path);
	} else {
		g_progname = strdup("");
		printf("%s:%d: warning: No valid argv[0] for initialization\n",
		       __FILE__, __LINE__);
	}


	initialize();
	if (g_ctx.fatal)
		return EXIT_FAILURE;

	if (help_flagged)
		*help_flagged = false;

	{
		/* Help param will be a special case and need restoring. */
		int i = 0;
		char **argv_in = NULL;
		const struct weston_option help_options[] = {
			{ WESTON_OPTION_BOOLEAN, "help", 'h', &opt_help },
		};
		argv_in = zalloc(sizeof(char *) * argc_in);
		if (!argv_in) {
			printf("%s:%d: error: alloc failed.\n",
			       __FILE__, __LINE__);
			return EXIT_FAILURE;
		}
		for (i = 0; i < argc_in; ++i)
			argv_in[i] = argv[i];

		parse_options(help_options, ARRAY_LENGTH(help_options),
			      argc, argv);
		if (*argc < argc_in) {
			for (i = 1; (i < argc_in) && !help_param; ++i) {
				bool found = false;
				int j = 0;
				for (j = 0; (j < *argc) && !found; ++j)
					found = (argv[j] == argv_in[i]);

				if (!found)
					help_param = argv_in[i];
			}
		}
		free(argv_in);
	}

	parse_options(options, ARRAY_LENGTH(options), argc, argv);

	if (help_param && (*argc < argc_in))
		argv[(*argc)++] = help_param;

	if (opt_filter) {
		zuc_set_filter(opt_filter);
		free(opt_filter);
	}

	if (opt_help) {
		printf("Usage: %s [OPTIONS]\n"
		       "  --zuc-break-on-failure\n"
		       "  --zuc-filter=FILTER\n"
		       "  --zuc-list-tests\n"
#if ENABLE_JUNIT_XML
		       "  --zuc-output-xml\n"
#endif
		       "  --zuc-random=N            [0|1|<seed number>]\n"
		       "  --zuc-repeat=N\n"
		       "  --help\n",
		       argv[0]);
		if (help_flagged)
			*help_flagged = true;
		rc = EXIT_SUCCESS;
	} else if (opt_list) {
		zuc_list_tests();
		rc = EXIT_FAILURE;
	} else {
		zuc_set_repeat(opt_repeat);
		zuc_set_random(opt_random);
		zuc_set_break_on_failure(opt_break_on_failure);
		zuc_set_output_junit(opt_junit);
		rc = EXIT_SUCCESS;
	}

	return rc;
}

static void
dispatch_pre_run(struct zuc_context *ctx, int pass_count, int pass_num,
		 int seed, const char *filter)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->pre_run)
			listener->pre_run(listener->data,
					  pass_count,
					  pass_num,
					  seed,
					  filter);
	}
}

static void
dispatch_run_started(struct zuc_context *ctx, int live_case_count,
		     int live_test_count, int disabled_count)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->run_started)
			listener->run_started(listener->data,
					      live_case_count,
					      live_test_count,
					      disabled_count);
	}
}

static void
dispatch_run_ended(struct zuc_context *ctx,
		   int live_case_count, int live_test_count, int total_passed,
		   int total_failed, int total_disabled, long total_elapsed)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->run_ended)
			listener->run_ended(listener->data,
					    ctx->case_count,
					    ctx->cases,
					    live_case_count,
					    live_test_count,
					    total_passed,
					    total_failed,
					    total_disabled,
					    total_elapsed);
	}
}

static void
dispatch_case_started(struct zuc_context *ctx,struct zuc_case *test_case,
		      int live_test_count, int disabled_count)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->case_started)
			listener->case_started(listener->data,
					       test_case,
					       live_test_count,
					       disabled_count);
	}
}

static void
dispatch_case_ended(struct zuc_context *ctx, struct zuc_case *test_case)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->case_ended)
			listener->case_ended(listener->data, test_case);
	}
}

static void
dispatch_test_started(struct zuc_context *ctx, struct zuc_test *test)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->test_started)
			listener->test_started(listener->data, test);
	}
}

static void
dispatch_test_ended(struct zuc_context *ctx, struct zuc_test *test)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->test_ended)
			listener->test_ended(listener->data, test);
	}
}

static void
dispatch_test_disabled(struct zuc_context *ctx, struct zuc_test *test)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->test_disabled)
			listener->test_disabled(listener->data, test);
	}
}

static void
dispatch_check_triggered(struct zuc_context *ctx, char const *file, int line,
			 enum zuc_fail_state state, enum zuc_check_op op,
			 enum zuc_check_valtype valtype,
			 intptr_t val1, intptr_t val2,
			 const char *expr1, const char *expr2)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->check_triggered)
			listener->check_triggered(listener->data,
						  file, line,
						  state, op, valtype,
						  val1, val2,
						  expr1, expr2);
	}
}

static void
dispatch_collect_event(struct zuc_context *ctx, char const *file, int line,
		       const char *expr1)
{
	struct zuc_slinked *curr;
	for (curr = ctx->listeners; curr; curr = curr->next) {
		struct zuc_event_listener *listener = curr->data;
		if (listener->collect_event)
			listener->collect_event(listener->data,
						file, line, expr1);
	}
}

static void
migrate_deferred_events(struct zuc_test *test, bool transferred)
{
	struct zuc_event *evt = test->deferred;
	if (!evt)
		return;

	test->deferred = NULL;
	if (test->events) {
		struct zuc_event *old = test->events;
		while (old->next)
			old = old->next;
		old->next = evt;
	} else {
		test->events = evt;
	}
	while (evt && !transferred) {
		dispatch_check_triggered(&g_ctx,
					 evt->file, evt->line,
					 evt->state, evt->op,
					 evt->valtype,
					 evt->val1, evt->val2,
					 evt->expr1, evt->expr2);
		evt = evt->next;
	}
}

static void
mark_single_failed(struct zuc_test *test, enum zuc_fail_state state)
{
	switch (state) {
	case ZUC_CHECK_OK:
		/* no internal state to change */
		break;
	case ZUC_CHECK_SKIP:
		if (test)
			test->skipped = 1;
		break;
	case ZUC_CHECK_FAIL:
		if (test)
			test->failed = 1;
		break;
	case ZUC_CHECK_ERROR:
	case ZUC_CHECK_FATAL:
		if (test)
			test->fatal = 1;
		break;
	}

	if (g_ctx.break_on_failure)
		raise(SIGABRT);

}

static void
mark_failed(struct zuc_test *test, enum zuc_fail_state state)
{
	if (!test && g_ctx.curr_test)
		test = g_ctx.curr_test;

	if (test) {
		mark_single_failed(test, state);
	} else if (g_ctx.curr_case) {
		/* In setup or tear-down of test suite */
		int i;
		for (i = 0; i < g_ctx.curr_case->test_count; ++i)
			mark_single_failed(g_ctx.curr_case->tests[i], state);
	}
	if ((state == ZUC_CHECK_FATAL) || (state == ZUC_CHECK_ERROR))
		g_ctx.fatal = true;
}

void
zuc_attach_event(struct zuc_test *test, struct zuc_event *event,
		 enum zuc_event_type event_type, bool transferred)
{
	if (!test) {
		/*
		 * consider adding events directly to the case.
		 * would be for use during per-suite setup and teardown.
		 */
		printf("%s:%d: error: No current test.\n", __FILE__, __LINE__);
	} else if (event_type == ZUC_EVENT_DEFERRED) {
		if (test->deferred) {
			struct zuc_event *curr = test->deferred;
			while (curr->next)
				curr = curr->next;
			curr->next = event;
		} else {
			test->deferred = event;
		}
	} else {
		if (test)
			migrate_deferred_events(test, transferred);

		if (test->events) {
			struct zuc_event *curr = test->events;
			while (curr->next)
				curr = curr->next;
			curr->next = event;
		} else {
			test->events = event;
		}
		mark_failed(test, event->state);
	}
}

void
zuc_add_event_listener(struct zuc_event_listener *event_listener)
{
	if (!event_listener) /* ensure null entries are not added */
		return;

	if (!g_ctx.listeners) {
		g_ctx.listeners = zalloc(sizeof(struct zuc_slinked));
		ZUC_ASSERT_NOT_NULL(g_ctx.listeners);
		g_ctx.listeners->data = event_listener;
	} else {
		struct zuc_slinked *curr = g_ctx.listeners;
		while (curr->next)
			curr = curr->next;
		curr->next = zalloc(sizeof(struct zuc_slinked));
		ZUC_ASSERT_NOT_NULL(curr->next);
		curr->next->data = event_listener;
	}
}


void
zuc_cleanup(void)
{
	int i;

	free(g_ctx.filter);
	g_ctx.filter = 0;

	if (g_ctx.listeners) {
		struct zuc_slinked *curr = g_ctx.listeners;
		while (curr) {
			struct zuc_slinked *old = curr;
			struct zuc_event_listener *listener = curr->data;
			if (listener->destroy)
				listener->destroy(listener->data);
			free(listener);
			curr = curr->next;
			free(old);
		}
		g_ctx.listeners = NULL;
	}

	for (i = g_ctx.case_count - 1; i >= 0; --i) {
		free_test_case(g_ctx.cases[i]);
		g_ctx.cases[i] = NULL;
	}
	free(g_ctx.cases);
	g_ctx.cases = NULL;

	free(g_progname);
	g_progname = NULL;
	free(g_progbasename);
	g_progbasename = NULL;
}

static void
shuffle_cases(int count, struct zuc_case **cases,
		   unsigned int seed)
{
	int i;
	unsigned int rseed = seed;
	for (i = 0; i < count; ++i) {
		int j;
		for (j = cases[i]->test_count - 1; j > 0 ; --j) {
			int val = rand_r(&rseed);
			int b = ((val / (double)RAND_MAX) * j + 0.5);
			if (j != b) {
				struct zuc_test *tmp = cases[i]->tests[j];
				cases[i]->tests[j] = cases[i]->tests[b];
				cases[i]->tests[b] = tmp;
			}
		}
	}

	for (i = count - 1; i > 0; --i) {
		int val = rand_r(&rseed);
		int j = ((val / (double)RAND_MAX) * i + 0.5);

		if (i != j) {
			struct zuc_case *tmp = cases[i];
			cases[i] = cases[j];
			cases[j] = tmp;
		}
	}
}

void
zuc_list_tests(void)
{
	int i;
	int j;
	initialize();
	if (g_ctx.fatal)
		return;
	for (i = 0; i < g_ctx.case_count; ++i) {
		printf("%s.\n", g_ctx.cases[i]->name);
		for (j = 0; j < g_ctx.cases[i]->test_count; ++j) {
			printf("  %s\n", g_ctx.cases[i]->tests[j]->name);
		}
	}
}

static void
run_single_test(struct zuc_test *test,const struct zuc_fixture *fxt,
		void *case_data)
{
	long elapsed = 0;
	struct timespec begin;
	struct timespec end;
	void *test_data = NULL;
	void *cleanup_data = NULL;
	void (*cleanup_fn)(void *data) = NULL;
	memset(&begin, 0, sizeof(begin));
	memset(&end, 0, sizeof(end));

	g_ctx.curr_test = test;
	dispatch_test_started(&g_ctx, test);

	cleanup_fn = fxt ? fxt->tear_down : NULL;
	cleanup_data = NULL;

	if (fxt && fxt->set_up) {
		test_data = fxt->set_up(case_data);
		cleanup_data = test_data;
	} else {
		test_data = case_data;
	}

	clock_gettime(TARGET_TIMER, &begin);

	/* Need to re-check these, as fixtures might have changed test state. */
	if (!test->fatal && !test->skipped) {
		if (test->fn_f)
			test->fn_f(test_data);
		else
			test->fn();
	}

	clock_gettime(TARGET_TIMER, &end);

	elapsed = (end.tv_sec - begin.tv_sec) * MS_PER_SEC;
	if (end.tv_sec != begin.tv_sec) {
		elapsed -= (begin.tv_nsec) / NANO_PER_MS;
		elapsed += (end.tv_nsec) / NANO_PER_MS;
	} else {
		elapsed += (end.tv_nsec - begin.tv_nsec) / NANO_PER_MS;
	}
	test->elapsed = elapsed;

	if (cleanup_fn)
		cleanup_fn(cleanup_data);

	if (test->deferred) {
		if (test_has_failure(test))
			migrate_deferred_events(test, false);
		else
			free_events(&test->deferred);
	}

	dispatch_test_ended(&g_ctx, test);

	g_ctx.curr_test = NULL;
}

static void
run_single_case(struct zuc_case *test_case)
{
	int count_live = test_case->test_count - test_case->disabled;
	g_ctx.curr_case = test_case;
	if (count_live) {
		int i = 0;
		const struct zuc_fixture *fxt = test_case->fxt;
		void *case_data = fxt ? (void *)fxt->data : NULL;

		dispatch_case_started(&g_ctx, test_case,
				      count_live, test_case->disabled);

		if (fxt && fxt->set_up_test_case)
			case_data = fxt->set_up_test_case(fxt->data);

		for (i = 0; i < test_case->test_count; ++i) {
			struct zuc_test *curr = test_case->tests[i];
			if (curr->disabled) {
				dispatch_test_disabled(&g_ctx, curr);
			} else {
				run_single_test(curr, fxt, case_data);
				if (curr->skipped)
					test_case->skipped++;
				if (curr->failed)
					test_case->failed++;
				if (curr->fatal)
					test_case->fatal++;
				if (!curr->failed && !curr->fatal)
					test_case->passed++;
				test_case->elapsed += curr->elapsed;
			}
		}

		if (fxt && fxt->tear_down_test_case)
			fxt->tear_down_test_case(case_data);

		dispatch_case_ended(&g_ctx, test_case);
	}
	g_ctx.curr_case = NULL;
}

static void
reset_test_values(struct zuc_case **cases, int case_count)
{
	int i;
	for (i = 0; i < case_count; ++i) {
		int j;
		cases[i]->disabled = 0;
		cases[i]->skipped = 0;
		cases[i]->failed = 0;
		cases[i]->fatal = 0;
		cases[i]->passed = 0;
		cases[i]->elapsed = 0;
		for (j = 0; j < cases[i]->test_count; ++j) {
			struct zuc_test *test = cases[i]->tests[j];
			if (test->disabled)
				cases[i]->disabled++;
			test->skipped = 0;
			test->failed = 0;
			test->fatal = 0;
			test->elapsed = 0;

			free_events(&test->events);
			free_events(&test->deferred);
		}
	}
}

static int
run_single_pass(void)
{
	long total_elapsed = 0;
	int total_passed = 0;
	int total_failed = 0;
	int total_skipped = 0;
	int live_case_count = 0;
	int live_test_count = 0;
	int disabled_test_count = 0;
	int i;

	reset_test_values(g_ctx.cases, g_ctx.case_count);
	for (i = 0; i <  g_ctx.case_count; ++i) {
		int live = g_ctx.cases[i]->test_count
			- g_ctx.cases[i]->disabled;
		if (live) {
			live_test_count += live;
			live_case_count++;
		}
		if (g_ctx.cases[i]->disabled)
			disabled_test_count++;
	}

	dispatch_run_started(&g_ctx, live_case_count, live_test_count,
			     disabled_test_count);

	for (i = 0; i <  g_ctx.case_count; ++i) {
		run_single_case(g_ctx.cases[i]);
		total_failed += g_ctx.cases[i]->test_count
			- (g_ctx.cases[i]->passed + g_ctx.cases[i]->disabled);
		total_passed += g_ctx.cases[i]->passed;
		total_elapsed += g_ctx.cases[i]->elapsed;
		total_skipped += g_ctx.cases[i]->skipped;
	}

	dispatch_run_ended(&g_ctx, live_case_count, live_test_count,
			   total_passed, total_failed, disabled_test_count,
			   total_elapsed);

	if (total_failed)
		return EXIT_FAILURE;
	else if (total_skipped)
		return ZUC_EXIT_SKIP;
	else
		return EXIT_SUCCESS;
}

int
zucimpl_run_tests(void)
{
	int rc = EXIT_SUCCESS;
	int i;
	int limit = g_ctx.repeat > 0 ? g_ctx.repeat : 1;

	initialize();
	if (g_ctx.fatal)
		return EXIT_FAILURE;

	if (g_ctx.listeners == NULL) {
		zuc_add_event_listener(zuc_base_logger_create());
		if (g_ctx.output_junit)
			zuc_add_event_listener(zuc_junit_reporter_create());
	}

	if (g_ctx.case_count < 1) {
		printf("%s:%d: error: Setup error: test tree is empty\n",
		       __FILE__, __LINE__);
		rc = EXIT_FAILURE;
	}

	for (i = 0; (i < limit) && (g_ctx.case_count > 0); ++i) {
		int pass_code = EXIT_SUCCESS;
		dispatch_pre_run(&g_ctx, limit, i + 1,
				 (g_ctx.random > 0) ? g_ctx.seed : 0,
				 g_ctx.filter);

		order_cases(g_ctx.case_count, g_ctx.cases);
		if (g_ctx.random > 0)
			shuffle_cases(g_ctx.case_count, g_ctx.cases,
				      g_ctx.seed);

		pass_code = run_single_pass();
		if (pass_code == EXIT_FAILURE)
			rc = EXIT_FAILURE;
		else if ((pass_code == ZUC_EXIT_SKIP) && (rc == EXIT_SUCCESS))
			rc = ZUC_EXIT_SKIP;

		g_ctx.seed++;
	}

	return rc;
}

int
zucimpl_tracepoint(char const *file, int line, char const *fmt, ...)
{
	int rc = -1;
	va_list argp;
	char *msg = NULL;


	va_start(argp, fmt);
	rc = vasprintf(&msg, fmt, argp);
	if (rc == -1) {
		msg = NULL;
	}
	va_end(argp);

	dispatch_collect_event(&g_ctx,
			       file, line,
			       msg);

	free(msg);

	return rc;
}

void
zucimpl_terminate(char const *file, int line,
		  bool fail, bool fatal, const char *msg)
{
	enum zuc_fail_state state = ZUC_CHECK_SKIP;
	int level = 2;
	if (fail && fatal) {
		state =  ZUC_CHECK_FATAL;
		level = 0;
	} else if (fail && !fatal) {
		state = ZUC_CHECK_FAIL;
		level = 0;
	}

	mark_failed(g_ctx.curr_test, state);

	if ((state != ZUC_CHECK_OK) && g_ctx.curr_test)
		migrate_deferred_events(g_ctx.curr_test, false);

	dispatch_check_triggered(&g_ctx,
				 file, line,
				 state,
				 ZUC_OP_TERMINATE, ZUC_VAL_INT,
				 level, 0,
				 msg, "");
}

static void
validate_types(enum zuc_check_op op, enum zuc_check_valtype valtype)
{
	bool is_valid = true;

	switch (op) {
	case ZUC_OP_NULL:
	case ZUC_OP_NOT_NULL:
		is_valid = is_valid && (valtype == ZUC_VAL_PTR);
		break;
	default:
		; /* all rest OK */
	}

	switch (valtype) {
	case ZUC_VAL_CSTR:
		is_valid = is_valid && ((op == ZUC_OP_EQ) || (op == ZUC_OP_NE));
		break;
	default:
		; /* all rest OK */
	}

	if (!is_valid)
		printf("%s:%d: warning: Unexpected op+type %d/%d.\n",
		       __FILE__, __LINE__, op, valtype);
}

static int
pred2_unknown(intptr_t lhs, intptr_t rhs)
{
	return 0;
}

static int
pred2_true(intptr_t lhs, intptr_t rhs)
{
	return lhs;
}

static int
pred2_false(intptr_t lhs, intptr_t rhs)
{
	return !lhs;
}

static int
pred2_eq(intptr_t lhs, intptr_t rhs)
{
	return lhs == rhs;
}

static int
pred2_streq(intptr_t lhs, intptr_t rhs)
{
	int status = 0;
	const char *lhptr = (const char *)lhs;
	const char *rhptr = (const char *)rhs;

	if (!lhptr && !rhptr)
		status = 1;
	else if (lhptr && rhptr)
		status = strcmp(lhptr, rhptr) == 0;

	return status;
}

static int
pred2_ne(intptr_t lhs, intptr_t rhs)
{
	return lhs != rhs;
}

static int
pred2_strne(intptr_t lhs, intptr_t rhs)
{
	int status = 0;
	const char *lhptr = (const char *)lhs;
	const char *rhptr = (const char *)rhs;

	if (lhptr != rhptr) {
		if (!lhptr || !rhptr)
			status = 1;
		else
			status = strcmp(lhptr, rhptr) != 0;
	}

	return status;
}

static int
pred2_ge(intptr_t lhs, intptr_t rhs)
{
	return lhs >= rhs;
}

static int
pred2_gt(intptr_t lhs, intptr_t rhs)
{
	return lhs > rhs;
}

static int
pred2_le(intptr_t lhs, intptr_t rhs)
{
	return lhs <= rhs;
}

static int
pred2_lt(intptr_t lhs, intptr_t rhs)
{
	return lhs < rhs;
}

static comp_pred2
get_pred2(enum zuc_check_op op, enum zuc_check_valtype valtype)
{
	switch (op) {
	case ZUC_OP_TRUE:
		return pred2_true;
	case ZUC_OP_FALSE:
		return pred2_false;
	case ZUC_OP_NULL:
		return pred2_false;
	case ZUC_OP_NOT_NULL:
		return pred2_true;
	case ZUC_OP_EQ:
		if (valtype == ZUC_VAL_CSTR)
			return pred2_streq;
		else
			return pred2_eq;
	case ZUC_OP_NE:
		if (valtype == ZUC_VAL_CSTR)
			return pred2_strne;
		else
			return pred2_ne;
	case ZUC_OP_GE:
		return pred2_ge;
	case ZUC_OP_GT:
		return pred2_gt;
	case ZUC_OP_LE:
		return pred2_le;
	case ZUC_OP_LT:
		return pred2_lt;
	default:
		return pred2_unknown;
	}
}

int
zucimpl_expect_pred2(char const *file, int line,
		     enum zuc_check_op op, enum zuc_check_valtype valtype,
		     bool fatal,
		     intptr_t lhs, intptr_t rhs,
		     const char *lhs_str, const char* rhs_str)
{
	enum zuc_fail_state state = fatal ?  ZUC_CHECK_FATAL : ZUC_CHECK_FAIL;
	comp_pred2 pred = get_pred2(op, valtype);
	int failed = !pred(lhs, rhs);

	validate_types(op, valtype);

	if (failed) {
		mark_failed(g_ctx.curr_test, state);

		if (g_ctx.curr_test)
			migrate_deferred_events(g_ctx.curr_test, false);

		dispatch_check_triggered(&g_ctx,
					 file, line,
					 fatal ? ZUC_CHECK_FATAL
					 : ZUC_CHECK_FAIL,
					 op, valtype,
					 lhs, rhs,
					 lhs_str, rhs_str);
	}
	return failed;
}
