/*
 * Copyright © 2008 Kristian Høgsberg
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

#ifndef WESTON_XALLOC_H
#define WESTON_XALLOC_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <libweston/zalloc.h>

void *
fail_on_null(void *p, size_t size, char *file, int32_t line);

#define xmalloc(s) (fail_on_null(malloc(s), (s), __FILE__, __LINE__))
#define xzalloc(s) (fail_on_null(zalloc(s), (s), __FILE__, __LINE__))
#define xstrdup(s) (fail_on_null(strdup(s), 0, __FILE__, __LINE__))
#define xrealloc(p, s) (fail_on_null(realloc(p, s), (s), __FILE__, __LINE__))

#ifdef  __cplusplus
}
#endif

#endif /* WESTON_XALLOC_H */
