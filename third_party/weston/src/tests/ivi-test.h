/*
 * Copyright © 2015 Collabora, Ltd.
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

#ifndef IVI_TEST_H
#define IVI_TEST_H

/*
 * IVI_TEST_SURFACE_ID_BASE is an arbitrary number picked for the
 * IVI tests. The only requirement is that it does not clash with
 * any other ivi-id range used in Weston.
 */
#define IVI_TEST_SURFACE_ID_BASE 0xffc01200
#define IVI_TEST_SURFACE_ID(i) (IVI_TEST_SURFACE_ID_BASE + i)
#define IVI_TEST_LAYER_ID_BASE 0xeef01200
#define IVI_TEST_LAYER_ID(i) (IVI_TEST_LAYER_ID_BASE + i)

#define IVI_TEST_SURFACE_COUNT (3)
#define IVI_TEST_LAYER_COUNT (3)

#endif /* IVI_TEST_H */
