/*
 * Copyright © 2011 Intel Corporation
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

#include <stdint.h>
#include <inttypes.h>

#include "config.h"

#include <libweston/libweston.h>
#include "shared/timespec-util.h"

WL_EXPORT void
weston_view_geometry_dirty(struct weston_view *view)
{
}

WL_EXPORT int
weston_log(const char *fmt, ...)
{
	return 0;
}

WL_EXPORT void
weston_view_schedule_repaint(struct weston_view *view)
{
}

WL_EXPORT void
weston_compositor_schedule_repaint(struct weston_compositor *compositor)
{
}

int
main(int argc, char *argv[])
{
	const double k = 300.0;
	const double current = 0.5;
	const double target = 1.0;
	const double friction = 1400;

	struct weston_spring spring;
	struct timespec time = { 0 };

	weston_spring_init(&spring, k, current, target);
	spring.friction = friction;
	spring.previous = 0.48;
	spring.timestamp = (struct timespec) { 0 };

	while (!weston_spring_done(&spring)) {
		printf("\t%" PRId64 "\t%f\n",
		       timespec_to_msec(&time), spring.current);
		weston_spring_update(&spring, &time);
		timespec_add_msec(&time, &time, 16);
	}

	return 0;
}
