/*
 * Copyright © 2013 David Herrmann <dh.herrmann@gmail.com>
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

/*
 * DBus Helpers
 * This file contains the dbus mainloop integration and several helpers to
 * make lowlevel libdbus access easier.
 */

#include "config.h"

#include <dbus/dbus.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <wayland-server.h>

#include <libweston/libweston.h>
#include "dbus.h"

/*
 * DBus Mainloop Integration
 * weston_dbus_bind() and weston_dbus_unbind() allow to bind an existing
 * DBusConnection to an existing wl_event_loop object. All dbus dispatching
 * is then nicely integrated into the wayland event loop.
 * Note that this only provides basic watch and timeout dispatching. No
 * remote thread wakeup, signal handling or other dbus insanity is supported.
 * This is fine as long as you don't use any of the deprecated libdbus
 * interfaces (like waking up remote threads..). There is really no rational
 * reason to support these.
 */

static int weston_dbus_dispatch_watch(int fd, uint32_t mask, void *data)
{
	DBusWatch *watch = data;
	uint32_t flags = 0;

	if (dbus_watch_get_enabled(watch)) {
		if (mask & WL_EVENT_READABLE)
			flags |= DBUS_WATCH_READABLE;
		if (mask & WL_EVENT_WRITABLE)
			flags |= DBUS_WATCH_WRITABLE;
		if (mask & WL_EVENT_HANGUP)
			flags |= DBUS_WATCH_HANGUP;
		if (mask & WL_EVENT_ERROR)
			flags |= DBUS_WATCH_ERROR;

		dbus_watch_handle(watch, flags);
	}

	return 0;
}

static dbus_bool_t weston_dbus_add_watch(DBusWatch *watch, void *data)
{
	struct wl_event_loop *loop = data;
	struct wl_event_source *s;
	int fd;
	uint32_t mask = 0, flags;

	if (dbus_watch_get_enabled(watch)) {
		flags = dbus_watch_get_flags(watch);
		if (flags & DBUS_WATCH_READABLE)
			mask |= WL_EVENT_READABLE;
		if (flags & DBUS_WATCH_WRITABLE)
			mask |= WL_EVENT_WRITABLE;
	}

	fd = dbus_watch_get_unix_fd(watch);
	s = wl_event_loop_add_fd(loop, fd, mask, weston_dbus_dispatch_watch,
				 watch);
	if (!s)
		return FALSE;

	dbus_watch_set_data(watch, s, NULL);
	return TRUE;
}

static void weston_dbus_remove_watch(DBusWatch *watch, void *data)
{
	struct wl_event_source *s;

	s = dbus_watch_get_data(watch);
	if (!s)
		return;

	wl_event_source_remove(s);
}

static void weston_dbus_toggle_watch(DBusWatch *watch, void *data)
{
	struct wl_event_source *s;
	uint32_t mask = 0, flags;

	s = dbus_watch_get_data(watch);
	if (!s)
		return;

	if (dbus_watch_get_enabled(watch)) {
		flags = dbus_watch_get_flags(watch);
		if (flags & DBUS_WATCH_READABLE)
			mask |= WL_EVENT_READABLE;
		if (flags & DBUS_WATCH_WRITABLE)
			mask |= WL_EVENT_WRITABLE;
	}

	wl_event_source_fd_update(s, mask);
}

static int weston_dbus_dispatch_timeout(void *data)
{
	DBusTimeout *timeout = data;

	if (dbus_timeout_get_enabled(timeout))
		dbus_timeout_handle(timeout);

	return 0;
}

static int weston_dbus_adjust_timeout(DBusTimeout *timeout,
				      struct wl_event_source *s)
{
	int64_t t = 0;

	if (dbus_timeout_get_enabled(timeout))
		t = dbus_timeout_get_interval(timeout);

	return wl_event_source_timer_update(s, t);
}

static dbus_bool_t weston_dbus_add_timeout(DBusTimeout *timeout, void *data)
{
	struct wl_event_loop *loop = data;
	struct wl_event_source *s;
	int r;

	s = wl_event_loop_add_timer(loop, weston_dbus_dispatch_timeout,
				    timeout);
	if (!s)
		return FALSE;

	r = weston_dbus_adjust_timeout(timeout, s);
	if (r < 0) {
		wl_event_source_remove(s);
		return FALSE;
	}

	dbus_timeout_set_data(timeout, s, NULL);
	return TRUE;
}

static void weston_dbus_remove_timeout(DBusTimeout *timeout, void *data)
{
	struct wl_event_source *s;

	s = dbus_timeout_get_data(timeout);
	if (!s)
		return;

	wl_event_source_remove(s);
}

static void weston_dbus_toggle_timeout(DBusTimeout *timeout, void *data)
{
	struct wl_event_source *s;

	s = dbus_timeout_get_data(timeout);
	if (!s)
		return;

	weston_dbus_adjust_timeout(timeout, s);
}

static int weston_dbus_dispatch(int fd, uint32_t mask, void *data)
{
	DBusConnection *c = data;
	int r;

	do {
		r = dbus_connection_dispatch(c);
		if (r == DBUS_DISPATCH_COMPLETE)
			r = 0;
		else if (r == DBUS_DISPATCH_DATA_REMAINS)
			r = -EAGAIN;
		else if (r == DBUS_DISPATCH_NEED_MEMORY)
			r = -ENOMEM;
		else
			r = -EIO;
	} while (r == -EAGAIN);

	if (r)
		weston_log("cannot dispatch dbus events: %d\n", r);

	return 0;
}

static int weston_dbus_bind(struct wl_event_loop *loop, DBusConnection *c,
			    struct wl_event_source **ctx_out)
{
	bool b;
	int r, fd;

	/* Idle events cannot reschedule themselves, therefore we use a dummy
	 * event-fd and mark it for post-dispatch. Hence, the dbus
	 * dispatcher is called after every dispatch-round.
	 * This is required as dbus doesn't allow dispatching events from
	 * within its own event sources. */
	fd = eventfd(0, EFD_CLOEXEC);
	if (fd < 0)
		return -errno;

	*ctx_out = wl_event_loop_add_fd(loop, fd, 0, weston_dbus_dispatch, c);
	close(fd);

	if (!*ctx_out)
		return -ENOMEM;

	wl_event_source_check(*ctx_out);

	b = dbus_connection_set_watch_functions(c,
						weston_dbus_add_watch,
						weston_dbus_remove_watch,
						weston_dbus_toggle_watch,
						loop,
						NULL);
	if (!b) {
		r = -ENOMEM;
		goto error;
	}

	b = dbus_connection_set_timeout_functions(c,
						  weston_dbus_add_timeout,
						  weston_dbus_remove_timeout,
						  weston_dbus_toggle_timeout,
						  loop,
						  NULL);
	if (!b) {
		r = -ENOMEM;
		goto error;
	}

	dbus_connection_ref(c);
	return 0;

error:
	dbus_connection_set_timeout_functions(c, NULL, NULL, NULL,
					      NULL, NULL);
	dbus_connection_set_watch_functions(c, NULL, NULL, NULL,
					    NULL, NULL);
	wl_event_source_remove(*ctx_out);
	*ctx_out = NULL;
	return r;
}

static void weston_dbus_unbind(DBusConnection *c, struct wl_event_source *ctx)
{
	dbus_connection_set_timeout_functions(c, NULL, NULL, NULL,
					      NULL, NULL);
	dbus_connection_set_watch_functions(c, NULL, NULL, NULL,
					    NULL, NULL);
	dbus_connection_unref(c);
	wl_event_source_remove(ctx);
}

/*
 * Convenience Helpers
 * Several convenience helpers are provided to make using dbus in weston
 * easier. We don't use any of the gdbus or qdbus helpers as they pull in
 * huge dependencies and actually are quite awful to use. Instead, we only
 * use the basic low-level libdbus library.
 */

int weston_dbus_open(struct wl_event_loop *loop, DBusBusType bus,
		     DBusConnection **out, struct wl_event_source **ctx_out)
{
	DBusConnection *c;
	int r;

	/* Ihhh, global state.. stupid dbus. */
	dbus_connection_set_change_sigpipe(FALSE);

	/* This is actually synchronous. It blocks for some authentication and
	 * setup. We just trust the dbus-server here and accept this blocking
	 * call. There is no real reason to complicate things further and make
	 * this asynchronous/non-blocking. A context should be created during
	 * thead/process/app setup, so blocking calls should be fine. */
	c = dbus_bus_get_private(bus, NULL);
	if (!c)
		return -EIO;

	dbus_connection_set_exit_on_disconnect(c, FALSE);

	r = weston_dbus_bind(loop, c, ctx_out);
	if (r < 0)
		goto error;

	*out = c;
	return r;

error:
	dbus_connection_close(c);
	dbus_connection_unref(c);
	return r;
}

void weston_dbus_close(DBusConnection *c, struct wl_event_source *ctx)
{
	weston_dbus_unbind(c, ctx);
	dbus_connection_close(c);
	dbus_connection_unref(c);
}

int weston_dbus_add_match(DBusConnection *c, const char *format, ...)
{
	DBusError err;
	int r;
	va_list list;
	char *str;

	va_start(list, format);
	r = vasprintf(&str, format, list);
	va_end(list);

	if (r < 0)
		return -ENOMEM;

	dbus_error_init(&err);
	dbus_bus_add_match(c, str, &err);
	free(str);
	if (dbus_error_is_set(&err)) {
		dbus_error_free(&err);
		return -EIO;
	}

	return 0;
}

int weston_dbus_add_match_signal(DBusConnection *c, const char *sender,
				 const char *iface, const char *member,
				 const char *path)
{
	return weston_dbus_add_match(c,
				     "type='signal',"
				     "sender='%s',"
				     "interface='%s',"
				     "member='%s',"
				     "path='%s'",
				     sender, iface, member, path);
}

void weston_dbus_remove_match(DBusConnection *c, const char *format, ...)
{
	int r;
	va_list list;
	char *str;

	va_start(list, format);
	r = vasprintf(&str, format, list);
	va_end(list);

	if (r < 0)
		return;

	dbus_bus_remove_match(c, str, NULL);
	free(str);
}

void weston_dbus_remove_match_signal(DBusConnection *c, const char *sender,
				     const char *iface, const char *member,
				     const char *path)
{
	weston_dbus_remove_match(c,
				 "type='signal',"
				 "sender='%s',"
				 "interface='%s',"
				 "member='%s',"
				 "path='%s'",
				 sender, iface, member, path);
}
