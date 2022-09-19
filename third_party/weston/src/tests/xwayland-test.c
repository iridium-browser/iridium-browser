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
 *
 * xwayland-test: Confirm that we can map a window and we're running
 *		  under Xwayland, not just X.
 *
 *		  This is done in steps:
 *		  1) Confirm that the WL_SURFACE_ID atom exists
 *		  2) Confirm that the window manager's name is "Weston WM"
 *		  3) Make sure we can map a window
 */

#include "config.h"

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

#include "weston-test-runner.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.xwayland = true;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

TEST(xwayland_client_test)
{
	Display *display;
	Window window, root, *support;
	XEvent event;
	int screen, status, actual_format;
	unsigned long nitems, bytes;
	Atom atom, type_atom, actual_type;
	char *wm_name;

	if (access(XSERVER_PATH, X_OK) != 0)
		exit(77);

	display = XOpenDisplay(NULL);
	if (!display)
		exit(EXIT_FAILURE);

	atom = XInternAtom(display, "WL_SURFACE_ID", True);
	assert(atom != None);

	atom = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", True);
	assert(atom != None);

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	status = XGetWindowProperty(display, root, atom, 0L, ~0L,
				    False, XA_WINDOW, &actual_type,
				    &actual_format, &nitems, &bytes,
				    (void *)&support);
	assert(status == Success);

	atom = XInternAtom(display, "_NET_WM_NAME", True);
	assert(atom != None);
	type_atom = XInternAtom(display, "UTF8_STRING", True);
	assert(atom != None);
	status = XGetWindowProperty(display, *support, atom, 0L, BUFSIZ,
				    False, type_atom, &actual_type,
				    &actual_format, &nitems, &bytes,
				    (void *)&wm_name);
	assert(status == Success);
	assert(nitems);
	assert(strcmp("Weston WM", wm_name) == 0);
	free(support);
	free(wm_name);

	window = XCreateSimpleWindow(display, root, 100, 100, 300, 300, 1,
				     BlackPixel(display, screen),
				     WhitePixel(display, screen));
	XSelectInput(display, window, ExposureMask);
	XMapWindow(display, window);

	alarm(4);
	while (1) {
		XNextEvent(display, &event);
		if (event.type == Expose)
			break;
	}

	XCloseDisplay(display);
}
