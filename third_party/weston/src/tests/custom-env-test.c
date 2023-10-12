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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wayland-util.h>

#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/process-util.h"
#include "shared/string-helpers.h"

#include "weston-test-runner.h"

#define ASSERT_STR_MATCH(_as, _bs) do { \
	const char *as = _as; \
	const char *bs = _bs; \
	assert(!!as == !!bs); \
	assert(!as || strcmp(as, bs) == 0); \
} while (0)

#define ASSERT_STR_ARRAY_MATCH(_name, _aa, _ba) do { \
	char * const *aa = _aa; \
	char * const *ba = _ba; \
	testlog("\tcomparing " _name ":\n"); \
	for (int _i = 0; aa[_i] || ba[_i]; _i++) { \
		testlog("\t\t[%d] '%s' == '%s'?\n", _i, aa[_i], ba[_i]); \
		ASSERT_STR_MATCH(aa[_i], ba[_i]); \
	} \
	testlog("\tsuccessfully compared " _name "\n"); \
} while (0)

static enum test_result_code
setup_env(struct weston_test_harness *harness)
{
	/* as this is a standalone test, we can clear the environment here */
	clearenv();

	putenv("ENV1=one");
	setenv("ENV2", "two", 1);
	setenv("ENV3", "three", 1);

	return weston_test_harness_execute_standalone(harness);
}

DECLARE_FIXTURE_SETUP(setup_env);

#define DEFAULT_ENVP (char * const []) { "ENV1=one", "ENV2=two", "ENV3=three", NULL }

TEST(basic_env)
{
	struct custom_env env;
	char *const envp[] = { "ENV1=one", "ENV2=two", "ENV3=four", "ENV5=five", NULL };

	custom_env_init_from_environ(&env);
	custom_env_set_env_var(&env, "ENV5", "five");
	custom_env_set_env_var(&env, "ENV3", "four");
	ASSERT_STR_ARRAY_MATCH("envp", custom_env_get_envp(&env), envp);
	assert(env.env_finalized);
	custom_env_fini(&env);
}

TEST(basic_env_arg)
{
	struct custom_env env;
	char *const argp[] = { "arg1", "arg2", "arg3", NULL };

	custom_env_init_from_environ(&env);
	custom_env_add_arg(&env, "arg1");
	custom_env_add_arg(&env, "arg2");
	custom_env_add_arg(&env, "arg3");

	ASSERT_STR_ARRAY_MATCH("envp", custom_env_get_envp(&env), DEFAULT_ENVP);
	assert(env.env_finalized);
	ASSERT_STR_ARRAY_MATCH("argp", custom_env_get_argp(&env), argp);
	assert(env.arg_finalized);
	custom_env_fini(&env);
}

struct test_str {
	const char *exec_str;
	char * const *envp;
	char * const *argp;
};

static struct test_str str_tests[] = {
	{
		.exec_str = "ENV1=1  ENV2=owt two-arghs",
		.envp = (char * const []) { "ENV1=1", "ENV2=owt", "ENV3=three", NULL },
		.argp = (char * const []) { "two-arghs", NULL },
	},
	{
		.exec_str = "ENV2=owt one-argh",
		.envp = (char * const []) { "ENV1=one", "ENV2=owt", "ENV3=three", NULL },
		.argp = (char * const []) { "one-argh", NULL },
	},
	{
		.exec_str = "FOO=bar  one-argh-again",
		.envp = (char * const []) { "ENV1=one", "ENV2=two", "ENV3=three", "FOO=bar", NULL },
		.argp = (char * const []) { "one-argh-again", NULL },
	},
	{
		.exec_str = "ENV1=number=7 one-argh-eq",
		.envp = (char * const []) { "ENV1=number=7", "ENV2=two", "ENV3=three", NULL },
		.argp = (char * const []) { "one-argh-eq", NULL },
	},
	{
		.exec_str = "no-arg-h",
		.envp = DEFAULT_ENVP,
		.argp = (char * const []) { "no-arg-h", NULL },
	},
	{
		.exec_str = "argh-w-arg argequals=thing  plainarg  ",
		.envp = DEFAULT_ENVP,
		.argp = (char * const []) { "argh-w-arg", "argequals=thing", "plainarg", NULL },
	},
};

TEST_P(env_parse_string, str_tests)
{
	struct custom_env env;
	struct test_str *test = data;

	testlog("checking exec_str '%s'\n", test->exec_str);
	custom_env_init_from_environ(&env);
	custom_env_add_from_exec_string(&env, test->exec_str);
	ASSERT_STR_ARRAY_MATCH("envp", custom_env_get_envp(&env), test->envp);
	ASSERT_STR_ARRAY_MATCH("argp", custom_env_get_argp(&env), test->argp);
	custom_env_fini(&env);
}
