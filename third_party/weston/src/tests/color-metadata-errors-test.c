/*
 * Copyright 2022 Collabora, Ltd.
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

#include <string.h>
#include <assert.h>
#include <math.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

#include "weston-private.h"
#include "libweston-internal.h"
#include "backend.h"
#include "color.h"

struct config_testcase {
	bool has_characteristics_key;
	const char *output_characteristics_name;
	const char *characteristics_name;
	const char *red_x;
	const char *green_y;
	const char *white_y;
	const char *min_L;
	int expected_retval;
	const char *expected_error;
};

static const struct config_testcase config_cases[] = {
	{
		false, "fred", "fred", "red_x=0.9", "green_y=0.8", "white_y=0.323", "min_L=1e-4", 0,
		""
	},
	{
		true, "fred", "fred", "red_x=0.9", "green_y= 0.8 ", "white_y=0.323", "min_L=1e-4", 0,
		""
	},
	{
		true, "fred", "fred", "red_x=0.9", "green_y= 0.8 ", "white_y=0.323", "", 0,
		""
	},
	{
		true, "notexisting", "fred", "red_x=0.9", "green_y=0.8", "white_y=0.323", "min_L=1e-4", -1,
		"Config error in weston.ini, output mockoutput: no [color_characteristics] section with 'name=notexisting' found.\n"
	},
	{
		true, "fr:ed", "fr:ed", "red_x=0.9", "green_y=0.8", "white_y=0.323", "min_L=1e-4", -1,
		"Config error in weston.ini [color_characteristics] name=fr:ed: reserved name. Do not use ':' character in the name.\n"
	},
	{
		true, "fred", "fred", "red_x=-5", "green_y=1.01", "white_y=0.323", "min_L=1e-4", -1,
		"Config error in weston.ini [color_characteristics] name=fred: red_x value -5.000000 is outside of the range 0.000000 - 1.000000.\n"
		"Config error in weston.ini [color_characteristics] name=fred: green_y value 1.010000 is outside of the range 0.000000 - 1.000000.\n"
	},
	{
		true, "fred", "fred", "red_x=haahaa", "green_y=-", "white_y=0.323", "min_L=1e-4", -1,
		"Config error in weston.ini [color_characteristics] name=fred: failed to parse the value of key red_x.\n"
		"Config error in weston.ini [color_characteristics] name=fred: failed to parse the value of key green_y.\n"
	},
	{
		true, "fred", "fred", "", "", "white_y=0.323", "min_L=1e-4", -1,
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key red_x is missing. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key red_y is set. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key green_x is set. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key green_y is missing. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key blue_x is set. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 1 key blue_y is set. You must set either none or all keys of a group.\n"
	},
	{
		true, "fred", "fred", "red_x=0.9", "green_y=0.8", "", "min_L=1e-4", -1,
		"Config error in weston.ini [color_characteristics] name=fred: group 2 key white_x is set. You must set either none or all keys of a group.\n"
		"Config error in weston.ini [color_characteristics] name=fred: group 2 key white_y is missing. You must set either none or all keys of a group.\n"
	},
};

static FILE *logfile;

static int
logger(const char *fmt, va_list arg)
{
	return vfprintf(logfile, fmt, arg);
}

static int
no_logger(const char *fmt, va_list arg)
{
	return 0;
}

static struct weston_config *
create_config(const struct config_testcase *t)
{
	struct compositor_setup setup;
	struct weston_config *wc;

	compositor_setup_defaults(&setup);
	weston_ini_setup(&setup,
			 cfgln("[output]"),
			 cfgln("name=mockoutput"),
			 t->has_characteristics_key ?
				cfgln("color_characteristics=%s", t->output_characteristics_name) :
				cfgln(""),
			 cfgln("eotf-mode=st2084"),

			 cfgln("[color_characteristics]"),
			 cfgln("name=%s", t->characteristics_name),
			 cfgln("maxFALL=1000"),
			 cfgln("%s", t->red_x),
			 cfgln("red_y=0.3"),
			 cfgln("blue_x=0.1"),
			 cfgln("blue_y=0.11"),
			 cfgln("green_x=0.1771"),
			 cfgln("%s", t->green_y),
			 cfgln("white_x=0.313"),
			 cfgln("%s", t->white_y),
			 cfgln("%s", t->min_L),
			 cfgln("max_L=65535.0"),

			 cfgln("[core]"),
			 cfgln("color-management=true"));

	wc = weston_config_parse(setup.config_file);
	free(setup.config_file);

	return wc;
}

/*
 * Manufacture various weston.ini and check what
 * wet_output_set_color_characteristics() says. Tests for the return value and
 * the error messages logged.
 */
TEST_P(color_characteristics_config_error, config_cases)
{
	const struct config_testcase *t = data;
	struct weston_config *wc;
	struct weston_config_section *section;
	int retval;
	char *logbuf;
	size_t logsize;
	struct weston_output mock_output = {};

	weston_output_init(&mock_output, NULL, "mockoutput");

	logfile = open_memstream(&logbuf, &logsize);
	weston_log_set_handler(logger, logger);

	wc = create_config(t);
	section = weston_config_get_section(wc, "output", "name", "mockoutput");
	assert(section);

	retval = wet_output_set_color_characteristics(&mock_output, wc, section);

	assert(fclose(logfile) == 0);
	logfile = NULL;

	testlog("retval %d, logs:\n%s\n", retval, logbuf);

	assert(retval == t->expected_retval);
	assert(strcmp(logbuf, t->expected_error) == 0);

	weston_config_destroy(wc);
	free(logbuf);
	weston_output_release(&mock_output);
}

/* Setting NULL resets group_mask */
TEST(weston_output_set_color_characteristics_null)
{
	struct weston_output mock_output = {};

	weston_output_init(&mock_output, NULL, "mockoutput");

	mock_output.color_characteristics.group_mask = 1;
	weston_output_set_color_characteristics(&mock_output, NULL);
	assert(mock_output.color_characteristics.group_mask == 0);

	weston_output_release(&mock_output);
}

struct value_testcase {
	unsigned field_index;
	float value;
	bool retval;
};

static const struct value_testcase value_cases[] = {
	{ 0, 0.0, true },
	{ 0, 1.0, true },
	{ 0, -0.001, false },
	{ 0, 1.01, false },
	{ 0, NAN, false },
	{ 0, HUGE_VALF, false },
	{ 0, -HUGE_VALF, false },
	{ 1, -1.0, false },
	{ 2, 2.0, false },
	{ 3, 2.0, false },
	{ 4, 2.0, false },
	{ 5, 2.0, false },
	{ 6, 2.0, false },
	{ 7, 2.0, false },
	{ 8, 0.99, false },
	{ 8, 65535.1, false },
	{ 9, 0.000099, false },
	{ 9, 6.55351, false },
	{ 10, 0.99, false },
	{ 10, 65535.1, false },
	{ 11, 0.99, false },
	{ 11, 65535.1, false },
};

struct mock_color_manager {
	struct weston_color_manager base;
	struct weston_hdr_metadata_type1 *test_hdr_meta;
};

static struct weston_output_color_outcome *
mock_create_output_color_outcome(struct weston_color_manager *cm_base,
				 struct weston_output *output)
{
	struct mock_color_manager *cm = container_of(cm_base, typeof(*cm), base);
	struct weston_output_color_outcome *co;

	co = zalloc(sizeof *co);
	assert(co);

	co->hdr_meta = *cm->test_hdr_meta;

	return co;
}

/*
 * Modify one value in a known good metadata structure, and see how
 * validation reacts to it.
 */
TEST_P(hdr_metadata_type1_errors, value_cases)
{
	struct value_testcase *t = data;
	struct weston_hdr_metadata_type1 meta = {
		.group_mask = WESTON_HDR_METADATA_TYPE1_GROUP_ALL_MASK,
		.primary[0] = { 0.6650, 0.3261 },
		.primary[1] = { 0.2890, 0.6435 },
		.primary[2] = { 0.1491, 0.0507 },
		.white = { 0.3134, 0.3291 },
		.maxDML = 600.0,
		.minDML = 0.0001,
		.maxCLL = 600.0,
		.maxFALL = 400.0,
	};
	float *fields[] = {
		&meta.primary[0].x, &meta.primary[0].y,
		&meta.primary[1].x, &meta.primary[1].y,
		&meta.primary[2].x, &meta.primary[2].y,
		&meta.white.x, &meta.white.y,
		&meta.maxDML, &meta.minDML,
		&meta.maxCLL, &meta.maxFALL,
	};
	struct mock_color_manager mock_cm = {
		.base.create_output_color_outcome = mock_create_output_color_outcome,
		.test_hdr_meta = &meta,
	};
	struct weston_compositor mock_compositor = {
		.color_manager = &mock_cm.base,
	};
	struct weston_output mock_output = {};
	bool ret;

	weston_log_set_handler(no_logger, no_logger);

	weston_output_init(&mock_output, &mock_compositor, "mockoutput");

	assert(t->field_index < ARRAY_LENGTH(fields));
	*fields[t->field_index] = t->value;
	ret = weston_output_set_color_outcome(&mock_output);
	assert(ret == t->retval);

	weston_output_color_outcome_destroy(&mock_output.color_outcome);
	weston_output_release(&mock_output);
}

/* Unflagged members are ignored in validity check */
TEST(hdr_metadata_type1_ignore_unflagged)
{
	/* All values invalid, but also empty mask so none actually used. */
	struct weston_hdr_metadata_type1 meta = {
		.group_mask = 0,
		.primary[0] = { -1.0, -1.0 },
		.primary[1] = { -1.0, -1.0 },
		.primary[2] = { -1.0, -1.0 },
		.white = { -1.0, -1.0 },
		.maxDML = -1.0,
		.minDML = -1.0,
		.maxCLL = -1.0,
		.maxFALL = -1.0,
	};
	struct mock_color_manager mock_cm = {
		.base.create_output_color_outcome = mock_create_output_color_outcome,
		.test_hdr_meta = &meta,
	};
	struct weston_compositor mock_compositor = {
		.color_manager = &mock_cm.base,
	};
	struct weston_output mock_output = {};
	bool ret;

	weston_log_set_handler(no_logger, no_logger);

	weston_output_init(&mock_output, &mock_compositor, "mockoutput");

	ret = weston_output_set_color_outcome(&mock_output);
	assert(ret);

	weston_output_color_outcome_destroy(&mock_output.color_outcome);
	weston_output_release(&mock_output);
}
