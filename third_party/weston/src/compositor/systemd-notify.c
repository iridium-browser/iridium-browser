/*
 * Copyright (c) 2015 General Electric Company. All rights reserved.
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

#include <stdlib.h>
#include <systemd/sd-daemon.h>
#include <sys/socket.h>
#include <wayland-server.h>

#include "shared/helpers.h"
#include "shared/string-helpers.h"
#include <libweston/zalloc.h>
#include <libweston/libweston.h>
#include "weston.h"

struct systemd_notifier {
	int watchdog_time;
	struct wl_event_source *watchdog_source;
	struct wl_listener compositor_destroy_listener;
};

static int
add_systemd_sockets(struct weston_compositor *compositor)
{
	int fd;
	int cnt_systemd_sockets;
	int current_fd = 0;

	cnt_systemd_sockets = sd_listen_fds(1);

	if (cnt_systemd_sockets < 0) {
		weston_log("sd_listen_fds failed with: %d\n",
				cnt_systemd_sockets);
		return -1;
	}

	/* socket-based activation not used, return silently */
	if (cnt_systemd_sockets == 0)
		return 0;

	while (current_fd < cnt_systemd_sockets) {
		fd = SD_LISTEN_FDS_START + current_fd;

		if (sd_is_socket(fd, AF_UNIX, SOCK_STREAM,1) <= 0) {
			weston_log("invalid socket provided from systemd\n");
			return -1;
		}

		if (wl_display_add_socket_fd(compositor->wl_display, fd)) {
			weston_log("wl_display_add_socket_fd failed"
					"for systemd provided socket\n");
			return -1;
		}
		current_fd++;
	}

	weston_log("info: add %d socket(s) provided by systemd\n",
			current_fd);

	return current_fd;
}

static int
watchdog_handler(void *data)
{
	struct systemd_notifier *notifier = data;

	wl_event_source_timer_update(notifier->watchdog_source,
				     notifier->watchdog_time);

	sd_notify(0, "WATCHDOG=1");

	return 1;
}

static void
weston_compositor_destroy_listener(struct wl_listener *listener, void *data)
{
	struct systemd_notifier *notifier;

	sd_notify(0, "STOPPING=1");

	notifier = container_of(listener,
				struct systemd_notifier,
				compositor_destroy_listener);

	if (notifier->watchdog_source)
		wl_event_source_remove(notifier->watchdog_source);

	wl_list_remove(&notifier->compositor_destroy_listener.link);
	free(notifier);
}

WL_EXPORT int
wet_module_init(struct weston_compositor *compositor,
		int *argc, char *argv[])
{
	char *watchdog_time_env;
	struct wl_event_loop *loop;
	int32_t watchdog_time_conv;
	struct systemd_notifier *notifier;

	notifier = zalloc(sizeof *notifier);
	if (notifier == NULL)
		return -1;

	if (!weston_compositor_add_destroy_listener_once(compositor,
							 &notifier->compositor_destroy_listener,
							 weston_compositor_destroy_listener)) {
		free(notifier);
		return 0;
	}

	if (add_systemd_sockets(compositor) < 0)
		return -1;

	sd_notify(0, "READY=1");

	/* 'WATCHDOG_USEC' is environment variable that is set
	 * by systemd to transfer 'WatchdogSec' watchdog timeout
	 * setting from service file.*/
	watchdog_time_env = getenv("WATCHDOG_USEC");
	if (!watchdog_time_env)
		return 0;

	if (!safe_strtoint(watchdog_time_env, &watchdog_time_conv))
		return 0;

	/* Convert 'WATCHDOG_USEC' to milliseconds and notify
	 * systemd every half of that time.*/
	watchdog_time_conv /= 1000 * 2;
	if (watchdog_time_conv <= 0)
		return 0;

	notifier->watchdog_time = watchdog_time_conv;

	loop = wl_display_get_event_loop(compositor->wl_display);
	notifier->watchdog_source =
		wl_event_loop_add_timer(loop, watchdog_handler, notifier);
	wl_event_source_timer_update(notifier->watchdog_source,
				     notifier->watchdog_time);

	return 0;
}

