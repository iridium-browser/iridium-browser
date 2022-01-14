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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/major.h>

#include <libweston/libweston.h>
#include "weston-launch.h"
#include "launcher-impl.h"
#include "shared/string-helpers.h"

#define DRM_MAJOR 226

#ifndef KDSKBMUTE
#define KDSKBMUTE	0x4B51
#endif

#ifdef BUILD_DRM_COMPOSITOR

#include <xf86drm.h>

#else

static inline int
drmDropMaster(int drm_fd)
{
	return 0;
}

static inline int
drmSetMaster(int drm_fd)
{
	return 0;
}

#endif

/* major()/minor() */
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif

union cmsg_data { unsigned char b[4]; int fd; };

struct launcher_weston_launch {
	struct weston_launcher base;
	struct weston_compositor *compositor;
	struct wl_event_loop *loop;
	int fd;
	struct wl_event_source *source;

	int kb_mode, tty, drm_fd;
};

static ssize_t
launcher_weston_launch_send(int sockfd, void *buf, size_t buflen)
{
	ssize_t len;

	do {
		len = send(sockfd, buf, buflen, 0);
	} while (len < 0 && errno == EINTR);

	return len;
}

static int
launcher_weston_launch_open(struct weston_launcher *launcher_base,
		     const char *path, int flags)
{
	struct launcher_weston_launch *launcher = wl_container_of(launcher_base, launcher, base);
	int n, ret;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov;
	union cmsg_data *data;
	char control[CMSG_SPACE(sizeof data->fd)];
	ssize_t len;
	struct weston_launcher_open *message;

	n = sizeof(*message) + strlen(path) + 1;
	message = malloc(n);
	if (!message)
		return -1;

	message->header.opcode = WESTON_LAUNCHER_OPEN;
	message->flags = flags;
	strcpy(message->path, path);

	launcher_weston_launch_send(launcher->fd, message, n);
	free(message);

	memset(&msg, 0, sizeof msg);
	iov.iov_base = &ret;
	iov.iov_len = sizeof ret;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof control;

	do {
		len = recvmsg(launcher->fd, &msg, MSG_CMSG_CLOEXEC);
	} while (len < 0 && errno == EINTR);

	if (len != sizeof ret ||
	    ret < 0)
		return -1;

	cmsg = CMSG_FIRSTHDR(&msg);
	if (!cmsg ||
	    cmsg->cmsg_level != SOL_SOCKET ||
	    cmsg->cmsg_type != SCM_RIGHTS) {
		fprintf(stderr, "invalid control message\n");
		return -1;
	}

	data = (union cmsg_data *) CMSG_DATA(cmsg);
	if (data->fd == -1) {
		fprintf(stderr, "missing drm fd in socket request\n");
		return -1;
	}

	return data->fd;
}

static void
launcher_weston_launch_close(struct weston_launcher *launcher_base, int fd)
{
	close(fd);
}

static void
launcher_weston_launch_restore(struct weston_launcher *launcher_base)
{
	struct launcher_weston_launch *launcher = wl_container_of(launcher_base, launcher, base);
	struct vt_mode mode = { 0 };

	if (ioctl(launcher->tty, KDSKBMUTE, 0) &&
	    ioctl(launcher->tty, KDSKBMODE, launcher->kb_mode))
		weston_log("failed to restore kb mode: %s\n",
			   strerror(errno));

	if (ioctl(launcher->tty, KDSETMODE, KD_TEXT))
		weston_log("failed to set KD_TEXT mode on tty: %s\n",
			   strerror(errno));

	/* We have to drop master before we switch the VT back in
	 * VT_AUTO, so we don't risk switching to a VT with another
	 * display server, that will then fail to set drm master. */
	drmDropMaster(launcher->drm_fd);

	mode.mode = VT_AUTO;
	if (ioctl(launcher->tty, VT_SETMODE, &mode) < 0)
		weston_log("could not reset vt handling\n");
}

static int
launcher_weston_launch_data(int fd, uint32_t mask, void *data)
{
	struct launcher_weston_launch *launcher = data;
	int len, ret, reply;

	if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) {
		weston_log("launcher socket closed, exiting\n");
		/* Normally the weston-launch will reset the tty, but
		 * in this case it died or something, so do it here so
		 * we don't end up with a stuck vt. */
		launcher_weston_launch_restore(&launcher->base);
		exit(-1);
	}

	do {
		len = recv(launcher->fd, &ret, sizeof ret, 0);
	} while (len < 0 && errno == EINTR);

	switch (ret) {
	case WESTON_LAUNCHER_ACTIVATE:
		launcher->compositor->session_active = true;
		wl_signal_emit(&launcher->compositor->session_signal,
			       launcher->compositor);
		break;
	case WESTON_LAUNCHER_DEACTIVATE:
		launcher->compositor->session_active = false;
		wl_signal_emit(&launcher->compositor->session_signal,
			       launcher->compositor);

		reply = WESTON_LAUNCHER_DEACTIVATE_DONE;
		launcher_weston_launch_send(launcher->fd, &reply, sizeof reply);

		break;
	default:
		weston_log("unexpected event from weston-launch\n");
		break;
	}

	return 1;
}

static int
launcher_weston_launch_activate_vt(struct weston_launcher *launcher_base, int vt)
{
	struct launcher_weston_launch *launcher = wl_container_of(launcher_base, launcher, base);
	return ioctl(launcher->tty, VT_ACTIVATE, vt);
}

static int
launcher_weston_environment_get_fd(const char *env)
{
	char *e;
	int fd, flags;

	e = getenv(env);
	if (!e || !safe_strtoint(e, &fd))
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		return -1;

	fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	unsetenv(env);

	return fd;
}


static int
launcher_weston_launch_connect(struct weston_launcher **out, struct weston_compositor *compositor,
			       int tty, const char *seat_id, bool sync_drm)
{
	struct launcher_weston_launch *launcher;
	struct wl_event_loop *loop;

	launcher = malloc(sizeof *launcher);
	if (launcher == NULL)
		return -ENOMEM;

	launcher->base.iface = &launcher_weston_launch_iface;
	* (struct launcher_weston_launch **) out = launcher;
	launcher->compositor = compositor;
	launcher->drm_fd = -1;
	launcher->fd = launcher_weston_environment_get_fd("WESTON_LAUNCHER_SOCK");
	if (launcher->fd != -1) {
		launcher->tty = launcher_weston_environment_get_fd("WESTON_TTY_FD");
		/* We don't get a chance to read out the original kb
		 * mode for the tty, so just hard code K_UNICODE here
		 * in case we have to clean if weston-launch dies. */
		launcher->kb_mode = K_UNICODE;

		loop = wl_display_get_event_loop(compositor->wl_display);
		launcher->source = wl_event_loop_add_fd(loop, launcher->fd,
							WL_EVENT_READABLE,
							launcher_weston_launch_data,
							launcher);
		if (launcher->source == NULL) {
			free(launcher);
			return -ENOMEM;
		}

		return 0;
	} else {
		return -1;
	}
}

static void
launcher_weston_launch_destroy(struct weston_launcher *launcher_base)
{
	struct launcher_weston_launch *launcher = wl_container_of(launcher_base, launcher, base);

	if (launcher->fd != -1) {
		close(launcher->fd);
		wl_event_source_remove(launcher->source);
	} else {
		launcher_weston_launch_restore(&launcher->base);
	}

	if (launcher->tty >= 0)
		close(launcher->tty);

	free(launcher);
}

static int
launcher_weston_launch_get_vt(struct weston_launcher *base)
{
	struct launcher_weston_launch *launcher = wl_container_of(base, launcher, base);
	struct stat s;
	if (fstat(launcher->tty, &s) < 0)
		return -1;

	return minor(s.st_rdev);
}

const struct launcher_interface launcher_weston_launch_iface = {
	.connect = launcher_weston_launch_connect,
	.destroy = launcher_weston_launch_destroy,
	.open = launcher_weston_launch_open,
	.close = launcher_weston_launch_close,
	.activate_vt = launcher_weston_launch_activate_vt,
	.get_vt = launcher_weston_launch_get_vt,
};
