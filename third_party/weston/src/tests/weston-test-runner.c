/*
 * Copyright © 2012 Intel Corporation
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

#include "weston-test-runner.h"
#include "weston-testsuite-data.h"
#include "shared/string-helpers.h"

/**
 * \defgroup testharness Test harness
 * \defgroup testharness_private Test harness private
 */

extern const struct weston_test_entry __start_test_section, __stop_test_section;

struct weston_test_run_info {
	char name[512];
	int fixture_nr;
};

static const struct weston_test_run_info *test_run_info_;

/** Get the test name string with counter
 *
 * \return The test name with fixture number \c -f%%d added. For an array
 * driven test, e.g. defined with TEST_P(), the name has also a \c -e%%d
 * suffix to indicate the array element number.
 *
 * This is only usable from code paths inside TEST(), TEST_P(), PLUGIN_TEST()
 * etc. defined functions.
 *
 * \ingroup testharness
 */
const char *
get_test_name(void)
{
	return test_run_info_->name;
}

/** Get the current fixture index
 *
 * Returns the current fixture index which can be used directly as an index
 * into the array passed as an argument to DECLARE_FIXTURE_SETUP_WITH_ARG().
 *
 * This is only usable from code paths inside TEST(), TEST_P(), PLUGIN_TEST()
 * etc. defined functions.
 *
 * \ingroup testharness
 */
int
get_test_fixture_index(void)
{
	return test_run_info_->fixture_nr - 1;
}

/** Print into test log
 *
 * This is exactly like printf() except the output goes to the test log,
 * which is at stderr.
 *
 * \param fmt printf format string
 *
 * \ingroup testharness
 */
void
testlog(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
}

static const struct weston_test_entry *
find_test(const char *name)
{
	const struct weston_test_entry *t;

	for (t = &__start_test_section; t < &__stop_test_section; t++)
		if (strcmp(t->name, name) == 0)
			return t;

	return NULL;
}

static enum test_result_code
run_test(int fixture_nr, const struct weston_test_entry *t, void *data,
	 int iteration)
{
	struct weston_test_run_info info;

	if (data) {
		snprintf(info.name, sizeof(info.name), "%s-f%02d-e%02d",
			 t->name, fixture_nr, iteration);
	} else {
		snprintf(info.name, sizeof(info.name), "%s-f%02d",
			 t->name, fixture_nr);
	}
	info.fixture_nr = fixture_nr;

	test_run_info_ = &info;
	t->run(data);
	test_run_info_ = NULL;

	/*
	 * XXX: We should return t->run(data); but that requires changing
	 * the function signature and stop using assert() in tests.
	 * https://gitlab.freedesktop.org/wayland/weston/issues/311
	 */
	return RESULT_OK;
}

static void
list_tests(void)
{
	const struct fixture_setup_array *fsa;
	const struct weston_test_entry *t;

	fsa = fixture_setup_array_get_();

	printf("Fixture setups: %d\n", fsa->n_elements);

	for (t = &__start_test_section; t < &__stop_test_section; t++) {
		printf("  %s\n", t->name);
		if (t->n_elements > 1)
			printf("    with array of %d cases\n", t->n_elements);
	}
}

struct weston_test_harness {
	int32_t fixt_ind;
	char *chosen_testname;
	int32_t case_ind;

	struct wet_testsuite_data data;
};

typedef void (*weston_test_cb)(struct wet_testsuite_data *suite_data,
			       const struct weston_test_entry *t,
			       const void *test_data,
			       int iteration);

static void
for_each_test_case(struct wet_testsuite_data *data, weston_test_cb cb)
{
	unsigned i;

	for (i = 0; i < data->tests_count; i++) {
		const struct weston_test_entry *t = &data->tests[i];
		const void *current_test_data = t->table_data;
		int elem;
		int elem_end;

		if (data->case_index == -1) {
			elem = 0;
			elem_end = t->n_elements;
		} else {
			elem = data->case_index;
			elem_end = elem + 1;
		}

		for (; elem < elem_end; elem++) {
			current_test_data = (char *)t->table_data +
					    elem * t->element_size;
			cb(data, t, current_test_data, elem);
		 }
	}
}

static const char *
result_to_str(enum test_result_code ret)
{
	static const char *names[] = {
		[RESULT_FAIL] = "fail",
		[RESULT_HARD_ERROR] = "hard error",
		[RESULT_OK] = "ok",
		[RESULT_SKIP] = "skip",
	};

	assert(ret >= 0 && ret < ARRAY_LENGTH(names));
	return names[ret];
}

static void
run_case(struct wet_testsuite_data *suite_data,
	 const struct weston_test_entry *t,
	 const void *test_data,
	 int iteration)
{
	enum test_result_code ret;
	const char *fail = "";
	const char *skip = "";
	int fixture_nr = suite_data->fixture_iteration + 1;
	int iteration_nr = iteration + 1;

	testlog("*** Run fixture %d, %s/%d\n",
		fixture_nr, t->name, iteration_nr);

	if (suite_data->type == TEST_TYPE_PLUGIN) {
		ret = run_test(fixture_nr, t, suite_data->compositor,
			       iteration);
	} else {
		ret = run_test(fixture_nr, t, (void *)test_data, iteration);
	}

	switch (ret) {
	case RESULT_OK:
		suite_data->passed++;
		break;
	case RESULT_FAIL:
	case RESULT_HARD_ERROR:
		suite_data->failed++;
		fail = "not ";
		break;
	case RESULT_SKIP:
		suite_data->skipped++;
		skip = " # SKIP";
		break;
	}

	testlog("*** Result fixture %d, %s/%d: %s\n",
		fixture_nr, t->name, iteration_nr, result_to_str(ret));

	suite_data->counter++;
	printf("%sok %d fixture %d %s/%d%s\n", fail, suite_data->counter,
	       fixture_nr, t->name, iteration_nr, skip);
}

/* This function might run in a new thread */
static void
testsuite_run(struct wet_testsuite_data *data)
{
	for_each_test_case(data, run_case);
}

static void
count_case(struct wet_testsuite_data *suite_data,
	   const struct weston_test_entry *t,
	   const void *test_data,
	   int iteration)
{
	suite_data->total++;
}

static void
tap_plan(struct wet_testsuite_data *data, int count_fixtures)
{
	data->total = 0;
	for_each_test_case(data, count_case);

	printf("1..%d\n", data->total * count_fixtures);
}

static void
skip_case(struct wet_testsuite_data *suite_data,
	  const struct weston_test_entry *t,
	  const void *test_data,
	  int iteration)
{
	int fixture_nr = suite_data->fixture_iteration + 1;
	int iteration_nr = iteration + 1;

	suite_data->counter++;
	printf("ok %d fixture %d %s/%d # SKIP fixture\n", suite_data->counter,
	       fixture_nr, t->name, iteration_nr);
}

static void
tap_skip_fixture(struct wet_testsuite_data *data)
{
	for_each_test_case(data, skip_case);
}

static void
help(const char *exe)
{
	printf(
		"Usage: %s [options] [testname [index]]\n"
		"\n"
		"This is a Weston test suite executable that runs some tests.\n"
		"Options:\n"
		"  -f, --fixture N  Run only fixture index N. Indices start from 1.\n"
		"  -h, --help       Print this help and exit with success.\n"
		"  -l, --list       List all tests in this executable and exit with success.\n"
		"testname:          Optional; name of the test to execute instead of all tests.\n"
		"index:             Optional; for a multi-case test, run the given case only.\n",
		exe);
}

static void
parse_command_line(struct weston_test_harness *harness, int argc, char **argv)
{
	int c;
	static const struct option opts[] = {
		{ "fixture", required_argument, NULL,      'f' },
		{ "help",    no_argument,       NULL,      'h' },
		{ "list",    no_argument,       NULL,      'l' },
		{ 0,         0,                 NULL,      0  }
	};

	while ((c = getopt_long(argc, argv, "f:hl", opts, NULL)) != -1) {
		switch (c) {
		case 'f':
			if (!safe_strtoint(optarg, &harness->fixt_ind)) {
				fprintf(stderr,
					"Error: '%s' does not look like a number (command line).\n",
					optarg);
				exit(RESULT_HARD_ERROR);
			}
			harness->fixt_ind--; /* convert base-1 to base 0 */
			break;
		case 'h':
			help(argv[0]);
			exit(RESULT_OK);
		case 'l':
			list_tests();
			exit(RESULT_OK);
		case 0:
			break;
		default:
			exit(RESULT_HARD_ERROR);
		}
	}

	if (optind < argc)
		harness->chosen_testname = argv[optind++];

	if (optind < argc) {
		if (!safe_strtoint(argv[optind], &harness->case_ind)) {
			fprintf(stderr,
				"Error: '%s' does not look like a number (command line).\n",
				argv[optind]);
			exit(RESULT_HARD_ERROR);
		}
		harness->case_ind--; /* convert base-1 to base 0 */
		optind++;
	}

	if (optind < argc) {
		fprintf(stderr, "Unexpected extra arguments given (command line).\n\n");
		help(argv[0]);
		exit(RESULT_HARD_ERROR);
	}
}

static struct weston_test_harness *
weston_test_harness_create(int argc, char **argv)
{
	const struct fixture_setup_array *fsa;
	struct weston_test_harness *harness;

	harness = zalloc(sizeof(*harness));
	assert(harness);

	harness->fixt_ind = -1;
	harness->case_ind = -1;
	parse_command_line(harness, argc, argv);

	fsa = fixture_setup_array_get_();
	if (harness->fixt_ind < -1 || harness->fixt_ind >= fsa->n_elements) {
		fprintf(stderr,
			"Error: fixture index %d (command line) is invalid for this program.\n",
			harness->fixt_ind + 1);
		exit(RESULT_HARD_ERROR);
	}

	if (harness->chosen_testname) {
		const struct weston_test_entry *t;

		t = find_test(harness->chosen_testname);
		if (!t) {
			fprintf(stderr,
				"Error: test '%s' not found (command line).\n",
				harness->chosen_testname);
			exit(RESULT_HARD_ERROR);
		}

		if (harness->case_ind < -1 ||
		    harness->case_ind >= t->n_elements) {
			fprintf(stderr,
				"Error: case index %d (command line) is invalid for this test.\n",
				harness->case_ind + 1);
			exit(RESULT_HARD_ERROR);
		}

		harness->data.tests = t;
		harness->data.tests_count = 1;
		harness->data.case_index = harness->case_ind;
	} else {
		harness->data.tests = &__start_test_section;
		harness->data.tests_count =
			&__stop_test_section - &__start_test_section;
		harness->data.case_index = -1;
	}

	harness->data.run = testsuite_run;

	return harness;
}

static void
weston_test_harness_destroy(struct weston_test_harness *harness)
{
	free(harness);
}

static enum test_result_code
counts_to_result(const struct wet_testsuite_data *data)
{
	/* RESULT_SKIP is reserved for fixture setup itself skipping everything */
	if (data->total == data->passed + data->skipped)
		return RESULT_OK;
	return RESULT_FAIL;
}

/** Execute all tests as client tests
 *
 * \param harness The test harness context.
 * \param setup The compositor configuration.
 *
 * Initializes the compositor with the given setup and executes the compositor.
 * The compositor creates a new thread where all tests in the test program are
 * serially executed. Once the thread finishes, the compositor returns from its
 * event loop and cleans up.
 *
 * Returns RESULT_SKIP if the requested compositor features, e.g. GL-renderer,
 * are not built.
 *
 * \sa DECLARE_FIXTURE_SETUP(), DECLARE_FIXTURE_SETUP_WITH_ARG()
 * \ingroup testharness
 */
enum test_result_code
weston_test_harness_execute_as_client(struct weston_test_harness *harness,
				      const struct compositor_setup *setup)
{
	struct wet_testsuite_data *data = &harness->data;

	data->type = TEST_TYPE_CLIENT;
	return execute_compositor(setup, data);
}

/** Execute all tests as plugin tests
 *
 * \param harness The test harness context.
 * \param setup The compositor configuration.
 *
 * Initializes the compositor with the given setup and executes the compositor.
 * The compositor executes all tests in the test program serially from an idle
 * handler, then returns from its event loop and cleans up.
 *
 * Returns RESULT_SKIP if the requested compositor features, e.g. GL-renderer,
 * are not built.
 *
 * \sa DECLARE_FIXTURE_SETUP(), DECLARE_FIXTURE_SETUP_WITH_ARG()
 * \ingroup testharness
 */
enum test_result_code
weston_test_harness_execute_as_plugin(struct weston_test_harness *harness,
				      const struct compositor_setup *setup)
{
	struct wet_testsuite_data *data = &harness->data;

	data->type = TEST_TYPE_PLUGIN;
	return execute_compositor(setup, data);
}

/** Execute all tests as standalone tests
 *
 * \param harness The test harness context.
 *
 * Executes all tests in the test program serially without any further setup,
 * particularly without any compositor instance created.
 *
 * \sa DECLARE_FIXTURE_SETUP(), DECLARE_FIXTURE_SETUP_WITH_ARG()
 * \ingroup testharness
 */
enum test_result_code
weston_test_harness_execute_standalone(struct weston_test_harness *harness)
{
	struct wet_testsuite_data *data = &harness->data;

	data->type = TEST_TYPE_STANDALONE;
	data->run(data);

	return RESULT_OK;
}

/** Fixture data array getter method
 *
 * DECLARE_FIXTURE_SETUP_WITH_ARG() overrides this in test programs.
 * The default implementation has no data and makes the tests run once.
 *
 * \ingroup testharness
 */
__attribute__((weak)) const struct fixture_setup_array *
fixture_setup_array_get_(void)
{
	/* A dummy fixture without a data array. */
	static const struct fixture_setup_array default_fsa = {
		.array = NULL,
		.element_size = 0,
		.n_elements = 1,
	};

	return &default_fsa;
}

/** Fixture setup function
 *
 * DECLARE_FIXTURE_SETUP() and DECLARE_FIXTURE_SETUP_WITH_ARG() override
 * this in test programs.
 * The default implementation just calls
 * weston_test_harness_execute_standalone().
 *
 * \ingroup testharness
 */
__attribute__((weak)) enum test_result_code
fixture_setup_run_(struct weston_test_harness *harness, const void *arg_)
{
	return weston_test_harness_execute_standalone(harness);
}

static void
fixture_report(const struct wet_testsuite_data *d, enum test_result_code ret)
{
	int fixture_nr = d->fixture_iteration + 1;

	testlog("--- Fixture %d %s: passed %d, skipped %d, failed %d, total %d\n",
		fixture_nr, result_to_str(ret),
		d->passed, d->skipped, d->failed, d->total);
}

int
main(int argc, char *argv[])
{
	struct weston_test_harness *harness;
	enum test_result_code ret;
	enum test_result_code result = RESULT_OK;
	const struct fixture_setup_array *fsa;
	const char *array_data;
	int fi;
	int fi_end;

	harness = weston_test_harness_create(argc, argv);

	fsa = fixture_setup_array_get_();
	array_data = fsa->array;

	if (harness->fixt_ind == -1) {
		fi = 0;
		fi_end = fsa->n_elements;
	} else {
		fi = harness->fixt_ind;
		fi_end = fi + 1;
	}

	tap_plan(&harness->data, fi_end - fi);
	testlog("Iterating through %d fixtures.\n", fi_end - fi);

	for (; fi < fi_end; fi++) {
		const void *arg = array_data + fi * fsa->element_size;

		testlog("--- Fixture %d...\n", fi + 1);
		harness->data.fixture_iteration = fi;
		harness->data.passed = 0;
		harness->data.skipped = 0;
		harness->data.failed = 0;

		ret = fixture_setup_run_(harness, arg);
		fixture_report(&harness->data, ret);

		if (ret == RESULT_SKIP) {
			tap_skip_fixture(&harness->data);
			continue;
		}

		if (ret != RESULT_OK && result != RESULT_HARD_ERROR)
			result = ret;
		else if (counts_to_result(&harness->data) != RESULT_OK)
			result = RESULT_FAIL;
	}

	weston_test_harness_destroy(harness);

	return result;
}
