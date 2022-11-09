/*
 * Copyright © 2015 Jasper St. Pierre
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

#include <libweston/libweston.h>

struct weston_launcher;

struct launcher_interface {
	int (* connect) (struct weston_launcher **launcher_out, struct weston_compositor *compositor,
			 int tty, const char *seat_id, bool sync_drm);
	void (* destroy) (struct weston_launcher *launcher);
	int (* open) (struct weston_launcher *launcher, const char *path, int flags);
	void (* close) (struct weston_launcher *launcher, int fd);
	int (* activate_vt) (struct weston_launcher *launcher, int vt);
	/* Get the number of the VT weston is running in */
	int (* get_vt) (struct weston_launcher *launcher);
};

struct weston_launcher {
	const struct launcher_interface *iface;
};

extern const struct launcher_interface launcher_logind_iface;
extern const struct launcher_interface launcher_weston_launch_iface;
extern const struct launcher_interface launcher_direct_iface;
