/*
 * Copyright © 2016 Samsung Electronics Co., Ltd
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
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "shared/string-helpers.h"

#include "weston-test-client-helper.h"

TEST(strtol_conversions)
{
	bool ret;
	int32_t val = -1;
	char *str = NULL;

	str = ""; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);

	str = "."; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);

	str = "42"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == true);
	assert(val == 42);

	str = "-42"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == true);
	assert(val == -42);

	str = "0042"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == true);
	assert(val == 42);

	str = "x42"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);

	str = "42x"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);

	str = "0x42424242"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);

	str = "424748364789L"; val = -1;
	ret = safe_strtoint(str, &val);
	assert(ret == false);
	assert(val == -1);
}
