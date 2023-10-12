/*
 * Copyright © 2011 Intel Corporation
 * Copyright © 2016 Giulio Camuffo
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

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <libweston/libweston.h>
#include "compositor/weston.h"
#include <libweston/xwayland-api.h>
#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/process-util.h"
#include "shared/string-helpers.h"

#ifdef HAVE_XWAYLAND_LISTENFD
#  define LISTEN_STR "-listenfd"
#else
#  define LISTEN_STR "-listen"
#endif

struct wet_xwayland {
	struct weston_compositor *compositor;
	struct wl_listener compositor_destroy_listener;
	const struct weston_xwayland_api *api;
	struct weston_xwayland *xwayland;
	struct wl_event_source *display_fd_source;
	struct wl_client *client;
	int wm_fd;
	struct weston_process process;
};

static int
handle_display_fd(int fd, uint32_t mask, void *data)
{
	struct wet_xwayland *wxw = data;
	char buf[64];
	ssize_t n;

	/* xwayland exited before being ready, don't finish initialization,
	 * the process watcher will cleanup */
	if (!(mask & WL_EVENT_READABLE))
		goto out;

	/* Xwayland writes to the pipe twice, so if we close it too early
	 * it's possible the second write will fail and Xwayland shuts down.
	 * Make sure we read until end of line marker to avoid this. */
	n = read(fd, buf, sizeof buf);
	if (n < 0 && errno != EAGAIN) {
		weston_log("read from Xwayland display_fd failed: %s\n",
			   strerror(errno));
		goto out;
	}
	/* Returning 1 here means recheck and call us again if required. */
	if (n <= 0 || (n > 0 && buf[n - 1] != '\n'))
		return 1;

	wxw->api->xserver_loaded(wxw->xwayland, wxw->client, wxw->wm_fd);

out:
	wl_event_source_remove(wxw->display_fd_source);
	close(fd);

	return 0;
}

static void
xserver_cleanup(struct weston_process *process, int status)
{
	struct wet_xwayland *wxw =
		container_of(process, struct wet_xwayland, process);

	wxw->api->xserver_exited(wxw->xwayland, status);
	wxw->client = NULL;
}

static pid_t
spawn_xserver(void *user_data, const char *display, int abstract_fd, int unix_fd)
{
	struct wet_xwayland *wxw = user_data;
	struct fdstr wayland_socket = FDSTR_INIT;
	struct fdstr x11_abstract_socket = FDSTR_INIT;
	struct fdstr x11_unix_socket = FDSTR_INIT;
	struct fdstr x11_wm_socket = FDSTR_INIT;
	struct fdstr display_pipe = FDSTR_INIT;
	char *xserver = NULL;
	struct weston_config *config = wet_get_config(wxw->compositor);
	struct weston_config_section *section;
	struct wl_event_loop *loop;
	struct custom_env child_env;
	int no_cloexec_fds[5];
	size_t num_no_cloexec_fds = 0;
	int ret;
	size_t written __attribute__ ((unused));

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, wayland_socket.fds) < 0) {
		weston_log("wl connection socketpair failed\n");
		goto err;
	}
	fdstr_update_str1(&wayland_socket);
	no_cloexec_fds[num_no_cloexec_fds++] = wayland_socket.fds[1];

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, x11_wm_socket.fds) < 0) {
		weston_log("X wm connection socketpair failed\n");
		goto err;
	}
	fdstr_update_str1(&x11_wm_socket);
	no_cloexec_fds[num_no_cloexec_fds++] = x11_wm_socket.fds[1];

	if (pipe2(display_pipe.fds, O_CLOEXEC) < 0) {
		weston_log("pipe creation for displayfd failed\n");
		goto err;
	}
	fdstr_update_str1(&display_pipe);
	no_cloexec_fds[num_no_cloexec_fds++] = display_pipe.fds[1];

	fdstr_set_fd1(&x11_abstract_socket, abstract_fd);
	no_cloexec_fds[num_no_cloexec_fds++] = abstract_fd;

	fdstr_set_fd1(&x11_unix_socket, unix_fd);
	no_cloexec_fds[num_no_cloexec_fds++] = unix_fd;

	assert(num_no_cloexec_fds <= ARRAY_LENGTH(no_cloexec_fds));

	section = weston_config_get_section(config, "xwayland", NULL, NULL);
	weston_config_section_get_string(section, "path",
					 &xserver, XSERVER_PATH);
	custom_env_init_from_environ(&child_env);
	custom_env_set_env_var(&child_env, "WAYLAND_SOCKET", wayland_socket.str1);

	custom_env_add_arg(&child_env, xserver);
	custom_env_add_arg(&child_env, display);
	custom_env_add_arg(&child_env, "-rootless");
	custom_env_add_arg(&child_env, LISTEN_STR);
	custom_env_add_arg(&child_env, x11_abstract_socket.str1);
	custom_env_add_arg(&child_env, LISTEN_STR);
	custom_env_add_arg(&child_env, x11_unix_socket.str1);
	custom_env_add_arg(&child_env, "-displayfd");
	custom_env_add_arg(&child_env, display_pipe.str1);
	custom_env_add_arg(&child_env, "-wm");
	custom_env_add_arg(&child_env, x11_wm_socket.str1);
	custom_env_add_arg(&child_env, "-terminate");

	ret = weston_client_launch(wxw->compositor, &wxw->process, &child_env,
				   no_cloexec_fds, num_no_cloexec_fds,
				   xserver_cleanup);
	if (!ret) {
		weston_log("Couldn't start Xwayland\n");
		goto err;
	}

	wxw->client = wl_client_create(wxw->compositor->wl_display,
				       wayland_socket.fds[0]);
	if (!wxw->client) {
		weston_log("Couldn't create client for Xwayland\n");
		goto err;
	}

	wxw->wm_fd = x11_wm_socket.fds[0];

	/* Now we can no longer fail, close the child end of our sockets */
	close(wayland_socket.fds[1]);
	close(x11_wm_socket.fds[1]);
	close(display_pipe.fds[1]);

	/* During initialization the X server will round trip
	 * and block on the wayland compositor, so avoid making
	 * blocking requests (like xcb_connect_to_fd) until
	 * it's done with that. */
	loop = wl_display_get_event_loop(wxw->compositor->wl_display);
	wxw->display_fd_source =
		wl_event_loop_add_fd(loop, display_pipe.fds[0],
				     WL_EVENT_READABLE,
				     handle_display_fd, wxw);

	free(xserver);

	return wxw->process.pid;

err:
	free(xserver);
	fdstr_close_all(&display_pipe);
	fdstr_close_all(&x11_wm_socket);
	fdstr_close_all(&wayland_socket);
	return -1;
}

static void
wxw_compositor_destroy(struct wl_listener *listener, void *data)
{
	struct wet_xwayland *wxw =
		wl_container_of(listener, wxw, compositor_destroy_listener);

	wl_list_remove(&wxw->compositor_destroy_listener.link);

	/* Don't call xserver_exited because Xwayland's own destroy handler
	 * already does this for us ... */
	if (wxw->client)
		kill(wxw->process.pid, SIGTERM);

	wl_list_remove(&wxw->process.link);
	free(wxw);
}

int
wet_load_xwayland(struct weston_compositor *comp)
{
	const struct weston_xwayland_api *api;
	struct weston_xwayland *xwayland;
	struct wet_xwayland *wxw;

	if (weston_compositor_load_xwayland(comp) < 0)
		return -1;

	api = weston_xwayland_get_api(comp);
	if (!api) {
		weston_log("Failed to get the xwayland module API.\n");
		return -1;
	}

	xwayland = api->get(comp);
	if (!xwayland) {
		weston_log("Failed to get the xwayland object.\n");
		return -1;
	}

	wxw = zalloc(sizeof *wxw);
	if (!wxw)
		return -1;

	wxw->compositor = comp;
	wxw->api = api;
	wxw->xwayland = xwayland;
	wl_list_init(&wxw->process.link);
	wxw->process.cleanup = xserver_cleanup;
	wxw->compositor_destroy_listener.notify = wxw_compositor_destroy;
	if (api->listen(xwayland, wxw, spawn_xserver) < 0)
		return -1;

	wl_signal_add(&comp->destroy_signal, &wxw->compositor_destroy_listener);

	return 0;
}
