/*
 * Copyright © 2012 Benjamin Franzke
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

#ifndef _WESTON_LAUNCHER_UTIL_H_
#define _WESTON_LAUNCHER_UTIL_H_

#include "config.h"

#include <libweston/libweston.h>

struct weston_launcher;

struct weston_launcher *
weston_launcher_connect(struct weston_compositor *compositor, int tty,
			const char *seat_id, bool sync_drm);

void
weston_launcher_destroy(struct weston_launcher *launcher);

int
weston_launcher_open(struct weston_launcher *launcher,
		     const char *path, int flags);

void
weston_launcher_close(struct weston_launcher *launcher, int fd);

int
weston_launcher_activate_vt(struct weston_launcher *launcher, int vt);

void
weston_setup_vt_switch_bindings(struct weston_compositor *compositor);

#endif
