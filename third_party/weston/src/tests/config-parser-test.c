/*
 * Copyright © 2013 Intel Corporation
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#include <libweston/config-parser.h>

#include "shared/helpers.h"
#include "zunitc/zunitc.h"

struct fixture_data {
	const char *text;
	struct weston_config *config;
};

static struct weston_config *
load_config(const char *text)
{
	struct weston_config *config = NULL;
	char *content = NULL;
	size_t file_len = 0;
	int write_len;
	FILE *file;

	file = open_memstream(&content, &file_len);
	ZUC_ASSERTG_NOT_NULL(file, out);

	write_len = fwrite(text, 1, strlen(text), file);
	ZUC_ASSERTG_EQ((int)strlen(text), write_len, out_close);

	ZUC_ASSERTG_EQ(fflush(file), 0, out_close);
	fseek(file, 0L, SEEK_SET);

	config = weston_config_parse_fp(file);

out_close:
	fclose(file);
	free(content);
out:
	return config;
}

static void *
setup_test_config(void *data)
{
	struct weston_config *config = load_config(data);
	ZUC_ASSERTG_NOT_NULL(config, out);

out:
	return config;
}

static void *
setup_test_config_failing(void *data)
{
	struct weston_config *config = load_config(data);
	ZUC_ASSERTG_NULL(config, err_free);

	return config;
err_free:
	weston_config_destroy(config);
	return NULL;
}

static void
cleanup_test_config(void *data)
{
	struct weston_config *config = data;
	ZUC_ASSERT_NOT_NULL(config);
	weston_config_destroy(config);
}

static struct zuc_fixture config_test_t0 = {
	.data = "# nothing in this file...\n",
	.set_up = setup_test_config,
	.tear_down = cleanup_test_config
};

static struct zuc_fixture config_test_t1 = {
	.data =
	"# comment line here...\n"
	"\n"
	"[foo]\n"
	"a=b\n"
	"name=  Roy Batty    \n"
	"\n"
	"\n"
	"[bar]\n"
	"# more comments\n"
	"number=5252\n"
	"zero=0\n"
	"negative=-42\n"
	"flag=false\n"
	"real=4.667\n"
	"negreal=-3.2\n"
	"expval=24.687E+15\n"
	"negexpval=-3e-2\n"
	"notanumber=nan\n"
	"empty=\n"
	"tiny=0.0000000000000000000000000000000000000063548\n"
	"\n"
	"[colors]\n"
	"none=0x00000000\n"
	"low=0x11223344\n"
	"high=0xff00ff00\n"
	"oct=01234567\n"
	"dec=12345670\n"
	"short=1234567\n"
	"\n"
	"[stuff]\n"
	"flag=     true \n"
	"\n"
	"[bucket]\n"
	"color=blue \n"
	"contents=live crabs\n"
	"pinchy=true\n"
	"\n"
	"[bucket]\n"
	"material=plastic \n"
	"color=red\n"
	"contents=sand\n",
	.set_up = setup_test_config,
	.tear_down = cleanup_test_config
};

static const char *section_names[] = {
	"foo", "bar", "colors", "stuff", "bucket", "bucket"
};

/*
 * Since these next few won't parse, we don't add the tear_down to
 * attempt cleanup.
 */

static struct zuc_fixture config_test_t2 = {
	.data =
	"# invalid section...\n"
	"[this bracket isn't closed\n",
	.set_up = setup_test_config_failing,
};

static struct zuc_fixture config_test_t3 = {
	.data =
	"# line without = ...\n"
	"[bambam]\n"
	"this line isn't any kind of valid\n",
	.set_up = setup_test_config_failing,
};

static struct zuc_fixture config_test_t4 = {
	.data =
	"# starting with = ...\n"
	"[bambam]\n"
	"=not valid at all\n",
	.set_up = setup_test_config_failing,
};

ZUC_TEST_F(config_test_t0, comment_only, data)
{
	struct weston_config *config = data;
	ZUC_ASSERT_NOT_NULL(config);
}

/** @todo individual t1 tests should have more descriptive names. */

ZUC_TEST_F(config_test_t1, test001, data)
{
	struct weston_config_section *section;
	struct weston_config *config = data;
	ZUC_ASSERT_NOT_NULL(config);
	section = weston_config_get_section(config,
					    "mollusc", NULL, NULL);
	ZUC_ASSERT_NULL(section);
}

ZUC_TEST_F(config_test_t1, test002, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "a", &s, NULL);

	ZUC_ASSERTG_EQ(0, r, out_free);
	ZUC_ASSERTG_STREQ("b", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test003, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "b", &s, NULL);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(ENOENT, errno);
	ZUC_ASSERT_NULL(s);
}

ZUC_TEST_F(config_test_t1, test004, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "foo", NULL, NULL);
	r = weston_config_section_get_string(section, "name", &s, NULL);

	ZUC_ASSERTG_EQ(0, r, out_free);
	ZUC_ASSERTG_STREQ("Roy Batty", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test005, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_string(section, "a", &s, "boo");

	ZUC_ASSERTG_EQ(-1, r, out_free);
	ZUC_ASSERTG_EQ(ENOENT, errno, out_free);
	ZUC_ASSERTG_STREQ("boo", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test006, data)
{
	int r;
	int32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "number", &n, 600);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(5252, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test007, data)
{
	int r;
	int32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "+++", &n, 700);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(ENOENT, errno);
	ZUC_ASSERT_EQ(700, n);
}

ZUC_TEST_F(config_test_t1, test008, data)
{
	int r;
	uint32_t u;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "number", &u, 600);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(5252, u);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test009, data)
{
	int r;
	uint32_t u;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "+++", &u, 600);
	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(ENOENT, errno);
	ZUC_ASSERT_EQ(600, u);
}

ZUC_TEST_F(config_test_t1, test010, data)
{
	int r;
	bool b;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_bool(section, "flag", &b, true);
	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(false, b);
}

ZUC_TEST_F(config_test_t1, test011, data)
{
	int r;
	bool b;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "stuff", NULL, NULL);
	r = weston_config_section_get_bool(section, "flag", &b, false);
	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(true, b);
}

ZUC_TEST_F(config_test_t1, test012, data)
{
	int r;
	bool b;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "stuff", NULL, NULL);
	r = weston_config_section_get_bool(section, "bonk", &b, false);
	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(ENOENT, errno);
	ZUC_ASSERT_EQ(false, b);
}

ZUC_TEST_F(config_test_t1, test013, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config,
					    "bucket", "color", "blue");
	r = weston_config_section_get_string(section, "contents", &s, NULL);

	ZUC_ASSERTG_EQ(0, r, out_free);
	ZUC_ASSERTG_STREQ("live crabs", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test014, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config,
					    "bucket", "color", "red");
	r = weston_config_section_get_string(section, "contents", &s, NULL);

	ZUC_ASSERTG_EQ(0, r, out_free);
	ZUC_ASSERTG_STREQ("sand", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test015, data)
{
	char *s;
	int r;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config,
					    "bucket", "color", "pink");
	ZUC_ASSERT_NULL(section);
	r = weston_config_section_get_string(section, "contents", &s, "eels");

	ZUC_ASSERTG_EQ(-1, r, out_free);
	ZUC_ASSERTG_EQ(ENOENT, errno, out_free);
	ZUC_ASSERTG_STREQ("eels", s, out_free);

out_free:
	free(s);
}

ZUC_TEST_F(config_test_t1, test016, data)
{
	const char *name;
	int i;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = NULL;
	i = 0;
	while (weston_config_next_section(config, &section, &name))
		ZUC_ASSERT_STREQ(section_names[i++], name);

	ZUC_ASSERT_EQ(6, i);
}

ZUC_TEST_F(config_test_t1, test017, data)
{
	int r;
	int32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "zero", &n, 600);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test018, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "zero", &n, 600);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test019, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "none", &n, 0xff336699);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0x000000, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test020, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "low", &n, 0xff336699);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0x11223344, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test021, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "high", &n, 0xff336699);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0xff00ff00, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test022, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	// Treat colors as hex values even if missing the leading 0x
	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "oct", &n, 0xff336699);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0x01234567, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test023, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	// Treat colors as hex values even if missing the leading 0x
	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "dec", &n, 0xff336699);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(0x12345670, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test024, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	// 7-digit colors are not valid (most likely typos)
	section = weston_config_get_section(config, "colors", NULL, NULL);
	r = weston_config_section_get_color(section, "short", &n, 0xff336699);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(0xff336699, n);
	ZUC_ASSERT_EQ(EINVAL, errno);
}

ZUC_TEST_F(config_test_t1, test025, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	// String color names are unsupported
	section = weston_config_get_section(config, "bucket", NULL, NULL);
	r = weston_config_section_get_color(section, "color", &n, 0xff336699);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(0xff336699, n);
	ZUC_ASSERT_EQ(EINVAL, errno);
}

ZUC_TEST_F(config_test_t1, test026, data)
{
	int r;
	int32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_int(section, "negative", &n, 600);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_EQ(-42, n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, test027, data)
{
	int r;
	uint32_t n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_uint(section, "negative", &n, 600);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_EQ(600, n);
	ZUC_ASSERT_EQ(ERANGE, errno);
}

ZUC_TEST_F(config_test_t1, get_double_number, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "number", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(5252.0 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_missing, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "+++", &n, 600.0);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_TRUE(600.0 == n);
	ZUC_ASSERT_EQ(ENOENT, errno);
}

ZUC_TEST_F(config_test_t1, get_double_zero, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "zero", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(0.0 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_negative, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "negative", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(-42.0 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_flag, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "flag", &n, 600.0);

	ZUC_ASSERT_EQ(-1, r);
	ZUC_ASSERT_TRUE(600.0 == n);
	ZUC_ASSERT_EQ(EINVAL, errno);
}

ZUC_TEST_F(config_test_t1, get_double_real, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "real", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(4.667 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_negreal, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "negreal", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(-3.2 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_expval, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "expval", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(24.687e+15 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_negexpval, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "negexpval", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(-3e-2 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_notanumber, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "notanumber", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(isnan(n));
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_empty, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "empty", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(0.0 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t1, get_double_tiny, data)
{
	int r;
	double n;
	struct weston_config_section *section;
	struct weston_config *config = data;

	errno = 0;
	section = weston_config_get_section(config, "bar", NULL, NULL);
	r = weston_config_section_get_double(section, "tiny", &n, 600.0);

	ZUC_ASSERT_EQ(0, r);
	ZUC_ASSERT_TRUE(6.3548e-39 == n);
	ZUC_ASSERT_EQ(0, errno);
}

ZUC_TEST_F(config_test_t2, doesnt_parse, data)
{
	struct weston_config *config = data;
	ZUC_ASSERT_NULL(config);
}

ZUC_TEST_F(config_test_t3, doesnt_parse, data)
{
	struct weston_config *config = data;
	ZUC_ASSERT_NULL(config);
}

ZUC_TEST_F(config_test_t4, doesnt_parse, data)
{
	struct weston_config *config = data;
	ZUC_ASSERT_NULL(config);
}

ZUC_TEST(config_test, destroy_null)
{
	weston_config_destroy(NULL);
	ZUC_ASSERT_EQ(0, weston_config_next_section(NULL, NULL, NULL));
}

ZUC_TEST(config_test, section_from_null)
{
	struct weston_config_section *section;
	section = weston_config_get_section(NULL, "bucket", NULL, NULL);
	ZUC_ASSERT_NULL(section);
}
