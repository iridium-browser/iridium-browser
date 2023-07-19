/*
 * Copyright © 2012 Benjamin Franzke
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

#include <libweston/libweston.h>

#include "launcher-util.h"
#include "launcher-impl.h"

#include <stdint.h>
#include <unistd.h>
#include <linux/input.h>

static const struct launcher_interface *ifaces[] = {
#ifdef HAVE_SYSTEMD_LOGIN
	&launcher_logind_iface,
#endif
	&launcher_weston_launch_iface,
	&launcher_direct_iface,
	NULL,
};

WL_EXPORT struct weston_launcher *
weston_launcher_connect(struct weston_compositor *compositor, int tty,
			const char *seat_id, bool sync_drm)
{
	const struct launcher_interface **it;

	for (it = ifaces; *it != NULL; it++) {
		const struct launcher_interface *iface = *it;
		struct weston_launcher *launcher;

		if (iface->connect(&launcher, compositor, tty, seat_id, sync_drm) == 0)
			return launcher;
	}

	return NULL;
}

WL_EXPORT void
weston_launcher_destroy(struct weston_launcher *launcher)
{
	launcher->iface->destroy(launcher);
}

WL_EXPORT int
weston_launcher_open(struct weston_launcher *launcher,
		     const char *path, int flags)
{
	return launcher->iface->open(launcher, path, flags);
}

WL_EXPORT void
weston_launcher_close(struct weston_launcher *launcher, int fd)
{
	launcher->iface->close(launcher, fd);
}

WL_EXPORT int
weston_launcher_activate_vt(struct weston_launcher *launcher, int vt)
{
	return launcher->iface->activate_vt(launcher, vt);
}

static void
switch_vt_binding(struct weston_keyboard *keyboard,
		  const struct timespec *time, uint32_t key, void *data)
{
	struct weston_compositor *compositor = data;
	struct weston_launcher *launcher = compositor->launcher;
	int vt = key - KEY_F1 + 1;

	if (vt == launcher->iface->get_vt(launcher))
		return;

	weston_launcher_activate_vt(launcher, vt);
}

WL_EXPORT void
weston_setup_vt_switch_bindings(struct weston_compositor *compositor)
{
	uint32_t key;
	struct weston_launcher *launcher = compositor->launcher;

	if (launcher->iface->get_vt(launcher) <= 0)
		return;

	if (compositor->vt_switching == false)
		return;

	for (key = KEY_F1; key < KEY_F9; key++)
		weston_compositor_add_key_binding(compositor, key,
						  MODIFIER_CTRL | MODIFIER_ALT,
						  switch_vt_binding,
						  compositor);
}
