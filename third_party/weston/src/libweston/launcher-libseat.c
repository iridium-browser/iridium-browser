/*
 * Copyright © 2020 Kenny Levinsen
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libseat.h>

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include "weston-log-internal.h"
#include "backend.h"
#include "dbus.h"
#include "launcher-impl.h"

struct launcher_libseat_device {
	struct wl_list link;
	int fd;
	int device_id;
	dev_t fsdev;
};

struct launcher_libseat {
	struct weston_launcher base;
	struct weston_compositor *compositor;
	struct libseat *seat;

	struct wl_event_source *seat_ctx;
	struct wl_list devices;
};

/* debug messages go into a dedicated libseat-debug scope, while info and err
 * log level messages go into the log_scope, which  the compositor has a
 * subscription by default*/
static struct weston_log_scope *libseat_debug_scope = NULL;

static struct launcher_libseat_device *
find_device_by_fd(struct launcher_libseat *wl, int fd)
{
	struct launcher_libseat_device *dev;
	wl_list_for_each(dev, &wl->devices, link) {
		if (dev->fd == fd) {
			return dev;
		}
	}
	return NULL;
}

static void
handle_enable_seat(struct libseat *seat, void *data)
{
	struct launcher_libseat *wl = data;
	if (wl->compositor->session_active)
		return;

	wl->compositor->session_active = true;

	wl_signal_emit(&wl->compositor->session_signal,
		       wl->compositor);
}

static void
handle_disable_seat(struct libseat *seat, void *data)
{
	struct launcher_libseat *wl = data;
	if (!wl->compositor->session_active)
		return;

	wl->compositor->session_active = false;

	wl_signal_emit(&wl->compositor->session_signal,
		       wl->compositor);
	libseat_disable_seat(wl->seat);
}

static struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};

static int
seat_open_device(struct weston_launcher *launcher, const char *path, int flags)
{
	struct launcher_libseat *wl = wl_container_of(launcher, wl, base);
	struct launcher_libseat_device *dev;
	struct stat st;

	dev = zalloc(sizeof(struct launcher_libseat_device));
	if (dev == NULL) {
		goto err_alloc;
	}

	dev->device_id = libseat_open_device(wl->seat, path, &dev->fd);
	if (dev->device_id == -1) {
		goto err_open;
	}

	if (fstat(dev->fd, &st) == -1) {
		goto err_fd;
	}
	dev->fsdev = st.st_rdev;

	wl_list_insert(&wl->devices, &dev->link);
	return dev->fd;

err_fd:
	libseat_close_device(wl->seat, dev->device_id);
	close(dev->fd);
err_open:
	free(dev);
err_alloc:
	return -1;
}

static void
seat_close_device(struct weston_launcher *launcher, int fd)
{
	struct launcher_libseat *wl = wl_container_of(launcher, wl, base);
	struct launcher_libseat_device *dev;

	dev = find_device_by_fd(wl, fd);
	if (dev == NULL) {
		weston_log("libseat: No device with fd %d found\n", fd);
		close(fd);
		return;
	}

	if (libseat_close_device(wl->seat, dev->device_id) == -1) {
		weston_log("libseat: Could not close device %d",
				dev->device_id);
	}

	wl_list_remove(&dev->link);
	free(dev);
	close(fd);
}

static int
seat_switch_session(struct weston_launcher *launcher, int vt)
{
	struct launcher_libseat *wl = wl_container_of(launcher, wl, base);
	return libseat_switch_session(wl->seat, vt);
}

static int
libseat_event(int fd, uint32_t mask, void *data)
{
	struct libseat *seat = data;
	if (libseat_dispatch(seat, 0) == -1) {
		weston_log("libseat: dispatch failed: %s\n", strerror(errno));
		exit(-1);
	}
	return 1;
}

static void
log_libseat_info_err(const char *fmt, va_list ap)
{
	/* these all have been set-up by the compositor and use the 'log' scope */
	weston_vlog(fmt, ap);
	weston_log_continue("\n");
}

static void
log_libseat_debug(const char *fmt, va_list ap)
{
	int len_va;
	char *str;
	const char *oom = "Out of memory";

	if (!weston_log_scope_is_enabled(libseat_debug_scope))
		return;

	len_va = vasprintf(&str, fmt, ap);
	if (len_va >= 0) {
		weston_log_scope_printf(libseat_debug_scope, "%s\n", str);
		free(str);
	} else {
		weston_log_scope_printf(libseat_debug_scope, "%s\n", oom);
	}
}

static void log_libseat(enum libseat_log_level level,
                       const char *fmt, va_list ap)
{
	if (level == LIBSEAT_LOG_LEVEL_DEBUG) {
		log_libseat_debug(fmt, ap);
		return;
	}

	log_libseat_info_err(fmt, ap);
}

static int
seat_open(struct weston_launcher **out, struct weston_compositor *compositor,
	  const char *seat_id, bool sync_drm)
{
	struct launcher_libseat *wl;
	struct wl_event_loop *event_loop;

	wl = zalloc(sizeof(*wl));
	if (wl == NULL) {
		goto err_out;
	}

	wl->base.iface = &launcher_libseat_iface;
	wl->compositor = compositor;
	wl_list_init(&wl->devices);

	libseat_debug_scope = compositor->libseat_debug;
	assert(libseat_debug_scope);
	libseat_set_log_handler(log_libseat);

	/* includes (all) other log levels available <= LOG_LEVEL_DEBUG */
	libseat_set_log_level(LIBSEAT_LOG_LEVEL_DEBUG);

	wl->seat = libseat_open_seat(&seat_listener, wl);
	if (wl->seat == NULL) {
		weston_log("libseat: could not open seat\n");
		goto err_seat;
	}

	event_loop = wl_display_get_event_loop(compositor->wl_display);
	wl->seat_ctx = wl_event_loop_add_fd(event_loop,
			libseat_get_fd(wl->seat), WL_EVENT_READABLE,
			libseat_event, wl->seat);
	if (wl->seat_ctx == NULL) {
		weston_log("libseat: could not register connection to event loop\n");
		goto err_session;
	}
	if (libseat_dispatch(wl->seat, 0) == -1) {
		weston_log("libseat: dispatch failed\n");
		goto err_session;
	}

	weston_log("libseat: session control granted\n");
	*out = &wl->base;
	return 0;

err_session:
	libseat_close_seat(wl->seat);
err_seat:
	free(wl);
err_out:
	return -1;
}

static void
seat_close(struct weston_launcher *launcher)
{
	struct launcher_libseat *wl = wl_container_of(launcher, wl, base);

	libseat_debug_scope = NULL;
	libseat_set_log_handler(NULL);

	if (wl->seat != NULL) {
		libseat_close_seat(wl->seat);
	}
	wl_event_source_remove(wl->seat_ctx);
	free(wl);
}

static int
seat_get_vt(struct weston_launcher *launcher)
{
	return -ENOSYS;
}

const struct launcher_interface launcher_libseat_iface = {
	.name = "libseat",
	.connect = seat_open,
	.destroy = seat_close,
	.open = seat_open_device,
	.close = seat_close_device,
	.activate_vt = seat_switch_session,
	.get_vt = seat_get_vt,
};
