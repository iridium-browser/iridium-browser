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

#ifndef _WESTON_DBUS_H_
#define _WESTON_DBUS_H_

#include "config.h"

#include <errno.h>
#include <wayland-server.h>

#include <libweston/libweston.h>

#ifdef HAVE_DBUS

#include <dbus/dbus.h>

/*
 * weston_dbus_open() - Open new dbus connection
 *
 * Opens a new dbus connection to the bus given as @bus. It automatically
 * integrates the new connection into the main-loop @loop. The connection
 * itself is returned in @out.
 * This also returns a context source used for dbus dispatching. It is
 * returned on success in @ctx_out and must be passed to weston_dbus_close()
 * unchanged. You must not access it from outside of a dbus helper!
 *
 * Returns 0 on success, negative error code on failure.
 */
int weston_dbus_open(struct wl_event_loop *loop, DBusBusType bus,
		     DBusConnection **out, struct wl_event_source **ctx_out);

/*
 * weston_dbus_close() - Close dbus connection
 *
 * Closes a dbus connection that was previously opened via weston_dbus_open().
 * It unbinds the connection from the main-loop it was previously bound to,
 * closes the dbus connection and frees all resources. If you want to access
 * @c after this call returns, you must hold a dbus-reference to it. But
 * notice that the connection is closed after this returns so it cannot be
 * used to spawn new dbus requests.
 * You must pass the context source returns by weston_dbus_open() as @ctx.
 */
void weston_dbus_close(DBusConnection *c, struct wl_event_source *ctx);

/*
 * weston_dbus_add_match() - Add dbus match
 *
 * Configure a dbus-match on the given dbus-connection. This match is saved
 * on the dbus-server as long as the connection is open. See dbus-manual
 * for information. Compared to the dbus_bus_add_match() this allows a
 * var-arg formatted match-string.
 */
int weston_dbus_add_match(DBusConnection *c, const char *format, ...);

/*
 * weston_dbus_add_match_signal() - Add dbus signal match
 *
 * Same as weston_dbus_add_match() but does the dbus-match formatting for
 * signals internally.
 */
int weston_dbus_add_match_signal(DBusConnection *c, const char *sender,
				 const char *iface, const char *member,
				 const char *path);

/*
 * weston_dbus_remove_match() - Remove dbus match
 *
 * Remove a previously configured dbus-match from the dbus server. There is
 * no need to remove dbus-matches if you close the connection, anyway.
 * Compared to dbus_bus_remove_match() this allows a var-arg formatted
 * match string.
 */
void weston_dbus_remove_match(DBusConnection *c, const char *format, ...);

/*
 * weston_dbus_remove_match_signal() - Remove dbus signal match
 *
 * Same as weston_dbus_remove_match() but does the dbus-match formatting for
 * signals internally.
 */
void weston_dbus_remove_match_signal(DBusConnection *c, const char *sender,
				     const char *iface, const char *member,
				     const char *path);

#endif /* HAVE_DBUS */

#endif // _WESTON_DBUS_H_
