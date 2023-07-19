/*
 * Copyright 2019 Collabora, Ltd.
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

#include <wayland-util.h>
#include "weston.h"

static struct wet_testsuite_data *wet_testsuite_data_global;

/** Set global test suite data
 *
 * \param data Custom test suite data.
 *
 * The type struct wet_testsuite_data is free to be defined by any test suite
 * in any way they want. This function stores a single pointer to that data
 * in a global variable.
 *
 * The data is expected to be fetched from a test suite specific plugin that
 * knows how to interpret it.
 *
 * \sa wet_testsuite_data_get
 */
WL_EXPORT void
wet_testsuite_data_set(struct wet_testsuite_data *data)
{
	wet_testsuite_data_global = data;
}

/** Get global test suite data
 *
 * \return Custom test suite data.
 *
 * Returns the value last set with wet_testsuite_data_set().
 */
WL_EXPORT struct wet_testsuite_data *
wet_testsuite_data_get(void)
{
	return wet_testsuite_data_global;
}
