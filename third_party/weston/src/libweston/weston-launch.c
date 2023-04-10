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

#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>

#include <getopt.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/vt.h>
#include <linux/major.h>
#include <linux/kd.h>

#include <pwd.h>
#include <grp.h>
#include <security/pam_appl.h>

#ifdef HAVE_SYSTEMD_LOGIN
#include <systemd/sd-login.h>
#endif

#include "weston-launch.h"

#define DRM_MAJOR 226

#ifndef KDSKBMUTE
#define KDSKBMUTE	0x4B51
#endif

#ifndef EVIOCREVOKE
#define EVIOCREVOKE _IOW('E', 0x91, int)
#endif

#define MAX_ARGV_SIZE 256

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
#    include <sys/mkdev.h>
#endif
#ifdef MAJOR_IN_SYSMACROS
#    include <sys/sysmacros.h>
#endif

struct weston_launch {
	struct pam_conv pc;
	pam_handle_t *ph;
	int tty;
	int ttynr;
	int sock[2];
	int drm_fd;
	int last_input_fd;
	int kb_mode;
	struct passwd *pw;

	int signalfd;

	pid_t child;
	int verbose;
	char *new_user;
};

union cmsg_data { unsigned char b[4]; int fd; };

static gid_t *
read_groups(int *ngroups)
{
	int n;
	gid_t *groups;

	n = getgroups(0, NULL);

	if (n < 0) {
		fprintf(stderr, "Unable to retrieve groups: %s\n",
			strerror(errno));
		return NULL;
	}

	groups = malloc(n * sizeof(gid_t));
	if (!groups)
		return NULL;

	if (getgroups(n, groups) < 0) {
		fprintf(stderr, "Unable to retrieve groups: %s\n",
			strerror(errno));
		free(groups);
		return NULL;
	}

	*ngroups = n;
	return groups;
}

static bool
weston_launch_allowed(struct weston_launch *wl)
{
	struct group *gr;
	gid_t *groups;
	int ngroups;
#ifdef HAVE_SYSTEMD_LOGIN
	char *session, *seat;
	int err;
#endif

	if (getuid() == 0)
		return true;

	gr = getgrnam("weston-launch");
	if (gr) {
		groups = read_groups(&ngroups);
		if (groups && ngroups > 0) {
			while (ngroups--) {
				if (groups[ngroups] == gr->gr_gid) {
					free(groups);
					return true;
				}
			}
			free(groups);
		}
	}

#ifdef HAVE_SYSTEMD_LOGIN
	err = sd_pid_get_session(getpid(), &session);
	if (err == 0 && session) {
		if (sd_session_is_active(session) &&
		    sd_session_get_seat(session, &seat) == 0) {
			free(seat);
			free(session);
			return true;
		}
		free(session);
	}
#endif

	return false;
}

static int
pam_conversation_fn(int msg_count,
		    const struct pam_message **messages,
		    struct pam_response **responses,
		    void *user_data)
{
	return PAM_SUCCESS;
}

static int
setup_pam(struct weston_launch *wl)
{
	int err;

	wl->pc.conv = pam_conversation_fn;
	wl->pc.appdata_ptr = wl;

	err = pam_start("login", wl->pw->pw_name, &wl->pc, &wl->ph);
	if (err != PAM_SUCCESS) {
		fprintf(stderr, "failed to start pam transaction: %d: %s\n",
			err, pam_strerror(wl->ph, err));
		return -1;
	}

	err = pam_set_item(wl->ph, PAM_TTY, ttyname(wl->tty));
	if (err != PAM_SUCCESS) {
		fprintf(stderr, "failed to set PAM_TTY item: %d: %s\n",
			err, pam_strerror(wl->ph, err));
		return -1;
	}

	err = pam_open_session(wl->ph, 0);
	if (err != PAM_SUCCESS) {
		fprintf(stderr, "failed to open pam session: %d: %s\n",
			err, pam_strerror(wl->ph, err));
		return -1;
	}

	return 0;
}

static int
setup_launcher_socket(struct weston_launch *wl)
{
	if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, wl->sock) < 0) {
		fprintf(stderr, "weston: socketpair failed: %s\n",
			strerror(errno));
		return -1;
	}

	if (fcntl(wl->sock[0], F_SETFD, FD_CLOEXEC) < 0) {
		fprintf(stderr, "weston: fcntl failed: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

static int
setup_signals(struct weston_launch *wl)
{
	int ret;
	sigset_t mask;
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	ret = sigaction(SIGCHLD, &sa, NULL);
	assert(ret == 0);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigaction(SIGHUP, &sa, NULL);

	ret = sigemptyset(&mask);
	assert(ret == 0);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	ret = sigprocmask(SIG_BLOCK, &mask, NULL);
	assert(ret == 0);

	wl->signalfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (wl->signalfd < 0)
		return -errno;

	return 0;
}

static void
setenv_fd(const char *env, int fd)
{
	char buf[32];

	snprintf(buf, sizeof buf, "%d", fd);
	setenv(env, buf, 1);
}

static int
open_tty_by_number(int ttynr)
{
	int ret;
	char filename[16];

	ret = snprintf(filename, sizeof filename, "/dev/tty%d", ttynr);
	if (ret < 0)
		return -1;

	return open(filename, O_RDWR | O_NOCTTY);
}

static int
send_reply(struct weston_launch *wl, int reply)
{
	int len;

	do {
		len = send(wl->sock[0], &reply, sizeof reply, 0);
	} while (len < 0 && errno == EINTR);

	return len;
}

static int
handle_open(struct weston_launch *wl, struct msghdr *msg, ssize_t len)
{
	int fd = -1, ret = -1;
	char control[CMSG_SPACE(sizeof(fd))];
	struct cmsghdr *cmsg;
	struct stat s;
	struct msghdr nmsg;
	struct iovec iov;
	struct weston_launcher_open *message;
	union cmsg_data *data;

	message = msg->msg_iov->iov_base;
	if ((size_t)len < sizeof(*message))
		goto err0;

	/* Ensure path is null-terminated */
	((char *) message)[len-1] = '\0';

	fd = open(message->path, message->flags);
	if (fd < 0) {
		fprintf(stderr, "Error opening device %s: %s\n",
			message->path, strerror(errno));
		goto err0;
	}

	if (fstat(fd, &s) < 0) {
		close(fd);
		fd = -1;
		fprintf(stderr, "Failed to stat %s\n", message->path);
		goto err0;
	}

	if (major(s.st_rdev) != INPUT_MAJOR &&
	    major(s.st_rdev) != DRM_MAJOR) {
		close(fd);
		fd = -1;
		fprintf(stderr, "Device %s is not an input or drm device\n",
			message->path);
		goto err0;
	}

err0:
	memset(&nmsg, 0, sizeof nmsg);
	nmsg.msg_iov = &iov;
	nmsg.msg_iovlen = 1;
	if (fd != -1) {
		nmsg.msg_control = control;
		nmsg.msg_controllen = sizeof control;
		cmsg = CMSG_FIRSTHDR(&nmsg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
		data = (union cmsg_data *) CMSG_DATA(cmsg);
		data->fd = fd;
		nmsg.msg_controllen = cmsg->cmsg_len;
		ret = 0;
	}
	iov.iov_base = &ret;
	iov.iov_len = sizeof ret;

	if (wl->verbose)
		fprintf(stderr, "weston-launch: opened %s: ret: %d, fd: %d\n",
			message->path, ret, fd);
	do {
		len = sendmsg(wl->sock[0], &nmsg, 0);
	} while (len < 0 && errno == EINTR);

	if (len < 0)
		return -1;

	if (fd != -1 && major(s.st_rdev) == DRM_MAJOR)
		wl->drm_fd = fd;
	if (fd != -1 && major(s.st_rdev) == INPUT_MAJOR &&
	    wl->last_input_fd < fd)
		wl->last_input_fd = fd;

	return 0;
}

static void
close_input_fds(struct weston_launch *wl)
{
	struct stat s;
	int fd;

	for (fd = 3; fd <= wl->last_input_fd; fd++) {
		if (fstat(fd, &s) == 0 && major(s.st_rdev) == INPUT_MAJOR) {
			/* EVIOCREVOKE may fail if the kernel doesn't
			 * support it, but all we can do is ignore it. */
			ioctl(fd, EVIOCREVOKE, 0);
			close(fd);
		}
	}
}

static int
handle_socket_msg(struct weston_launch *wl)
{
	char control[CMSG_SPACE(sizeof(int))];
	char buf[BUFSIZ];
	struct msghdr msg;
	struct iovec iov;
	int ret = -1;
	ssize_t len;
	struct weston_launcher_message *message;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = buf;
	iov.iov_len  = sizeof buf;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof control;

	do {
		len = recvmsg(wl->sock[0], &msg, 0);
	} while (len < 0 && errno == EINTR);

	if (len < 1)
		return -1;

	message = (void *) buf;
	switch (message->opcode) {
	case WESTON_LAUNCHER_OPEN:
		ret = handle_open(wl, &msg, len);
		break;
	case WESTON_LAUNCHER_DEACTIVATE_DONE:
		close_input_fds(wl);
		drmDropMaster(wl->drm_fd);
		ioctl(wl->tty, VT_RELDISP, 1);
		break;
	}

	return ret;
}

static void
quit(struct weston_launch *wl, int status)
{
	struct vt_mode mode = { 0 };
	int err;
	int oldtty;

	close(wl->signalfd);
	close(wl->sock[0]);

	if (wl->new_user) {
		err = pam_close_session(wl->ph, 0);
		if (err)
			fprintf(stderr, "pam_close_session failed: %d: %s\n",
				err, pam_strerror(wl->ph, err));
		pam_end(wl->ph, err);
	}

	/*
	 * Get a fresh handle to the tty as the previous one is in
	 * hang-up state since weston (the controlling process for
	 * the tty) exit at this point. Reopen before closing the
	 * file descriptor to avoid a potential race condition.
	 *
	 * A similar fix exists in logind, see:
	 * https://github.com/systemd/systemd/pull/990
	 */
	oldtty = wl->tty;
	wl->tty = open_tty_by_number(wl->ttynr);
	close(oldtty);

	if (ioctl(wl->tty, KDSKBMUTE, 0) &&
	    ioctl(wl->tty, KDSKBMODE, wl->kb_mode))
		fprintf(stderr, "failed to restore keyboard mode: %s\n",
			strerror(errno));

	if (ioctl(wl->tty, KDSETMODE, KD_TEXT))
		fprintf(stderr, "failed to set KD_TEXT mode on tty: %s\n",
			strerror(errno));

	/* We have to drop master before we switch the VT back in
	 * VT_AUTO, so we don't risk switching to a VT with another
	 * display server, that will then fail to set drm master. */
	drmDropMaster(wl->drm_fd);

	mode.mode = VT_AUTO;
	if (ioctl(wl->tty, VT_SETMODE, &mode) < 0)
		fprintf(stderr, "could not reset vt handling\n");

	if (wl->tty != STDIN_FILENO)
		close(wl->tty);

	exit(status);
}

static int
handle_signal(struct weston_launch *wl)
{
	struct signalfd_siginfo sig;
	int pid, status, ret;

	if (read(wl->signalfd, &sig, sizeof sig) != sizeof sig) {
		fprintf(stderr, "weston: reading signalfd failed: %s\n",
			strerror(errno));
		return -1;
	}

	switch (sig.ssi_signo) {
	case SIGCHLD:
		pid = waitpid(-1, &status, 0);
		if (pid == wl->child) {
			wl->child = 0;
			if (WIFEXITED(status))
				ret = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				/*
				 * If weston dies because of signal N, we
				 * return 10+N. This is distinct from
				 * weston-launch dying because of a signal
				 * (128+N).
				 */
				ret = 10 + WTERMSIG(status);
			else
				ret = 0;
			quit(wl, ret);
		}
		break;
	case SIGTERM:
	case SIGINT:
		if (!wl->child)
			break;

		if (wl->verbose)
			fprintf(stderr, "weston-launch: sending %s to pid %d\n",
				strsignal(sig.ssi_signo), wl->child);

		kill(wl->child, sig.ssi_signo);
		break;
	case SIGUSR1:
		send_reply(wl, WESTON_LAUNCHER_DEACTIVATE);
		break;
	case SIGUSR2:
		ioctl(wl->tty, VT_RELDISP, VT_ACKACQ);
		drmSetMaster(wl->drm_fd);
		send_reply(wl, WESTON_LAUNCHER_ACTIVATE);
		break;
	default:
		return -1;
	}

	return 0;
}

static int
setup_tty(struct weston_launch *wl, const char *tty)
{
	struct stat buf;
	struct vt_mode mode = { 0 };
	char *t;

	if (!wl->new_user) {
		wl->tty = STDIN_FILENO;
	} else if (tty) {
		t = ttyname(STDIN_FILENO);
		if (t && strcmp(t, tty) == 0)
			wl->tty = STDIN_FILENO;
		else
			wl->tty = open(tty, O_RDWR | O_NOCTTY);
	} else {
		int tty0 = open("/dev/tty0", O_WRONLY | O_CLOEXEC);

		if (tty0 < 0) {
			fprintf(stderr, "weston: could not open tty0: %s\n",
				strerror(errno));
			return -1;
		}

		if (ioctl(tty0, VT_OPENQRY, &wl->ttynr) < 0 || wl->ttynr == -1)
		{
			fprintf(stderr, "weston: failed to find non-opened console: %s\n",
				strerror(errno));
			return -1;
		}

		wl->tty = open_tty_by_number(wl->ttynr);
		close(tty0);
	}

	if (wl->tty < 0) {
		fprintf(stderr, "weston: failed to open tty: %s\n",
			strerror(errno));
		return -1;
	}

	if (fstat(wl->tty, &buf) == -1 ||
	    major(buf.st_rdev) != TTY_MAJOR || minor(buf.st_rdev) == 0) {
		fprintf(stderr, "weston: weston-launch must be run from a virtual terminal\n");
		return -1;
	}

	if (!wl->new_user || tty) {
		if (fstat(wl->tty, &buf) < 0) {
			fprintf(stderr, "weston: stat %s failed: %s\n", tty,
				strerror(errno));
			return -1;
		}

		if (major(buf.st_rdev) != TTY_MAJOR) {
			fprintf(stderr,
				"weston: invalid tty device: %s\n", tty);
			return -1;
		}

		wl->ttynr = minor(buf.st_rdev);
	}

	if (ioctl(wl->tty, VT_ACTIVATE, wl->ttynr) < 0) {
		fprintf(stderr,
			"weston: failed to activate VT: %s\n",
			strerror(errno));
		return -1;
	}

	if (ioctl(wl->tty, VT_WAITACTIVE, wl->ttynr) < 0) {
		fprintf(stderr,
			"weston: failed to wait for VT to be active: %s\n",
			strerror(errno));
		return -1;
	}

	if (ioctl(wl->tty, KDGKBMODE, &wl->kb_mode)) {
		fprintf(stderr,
			"weston: failed to get current keyboard mode: %s\n",
			strerror(errno));
		return -1;
	}

	if (ioctl(wl->tty, KDSKBMUTE, 1) &&
	    ioctl(wl->tty, KDSKBMODE, K_OFF)) {
		fprintf(stderr,
			"weston: failed to set K_OFF keyboard mode: %s\n",
			strerror(errno));
		return -1;
	}

	if (ioctl(wl->tty, KDSETMODE, KD_GRAPHICS)) {
		fprintf(stderr,
			"weston: failed to set KD_GRAPHICS mode on tty: %s\n",
			strerror(errno));
		return -1;
	}

	mode.mode = VT_PROCESS;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR2;
	if (ioctl(wl->tty, VT_SETMODE, &mode) < 0) {
		fprintf(stderr,
			"weston: failed to take control of vt handling %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

static int
setup_session(struct weston_launch *wl, char **child_argv)
{
	char **env;
	char *term;
	int i;

	if (wl->tty != STDIN_FILENO) {
		if (setsid() < 0) {
			fprintf(stderr, "weston: setsid failed %s\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (ioctl(wl->tty, TIOCSCTTY, 0) < 0) {
			fprintf(stderr, "TIOCSCTTY failed - tty is in use %s\n",
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	term = getenv("TERM");
	clearenv();
	if (term)
		setenv("TERM", term, 1);
	setenv("USER", wl->pw->pw_name, 1);
	setenv("LOGNAME", wl->pw->pw_name, 1);
	setenv("HOME", wl->pw->pw_dir, 1);
	setenv("SHELL", wl->pw->pw_shell, 1);

	env = pam_getenvlist(wl->ph);
	if (env) {
		for (i = 0; env[i]; ++i) {
			if (putenv(env[i]) != 0)
				fprintf(stderr, "putenv %s failed\n", env[i]);
		}
		free(env);
	}

	/*
	 * We open a new session, so it makes sense
	 * to run a new login shell
	 */
	child_argv[0] = "/bin/sh";
	child_argv[1] = "-l";
	child_argv[2] = "-c";
	child_argv[3] = "exec " BINDIR "/weston \"$@\"";
	child_argv[4] = "weston";
	return 5;
}

static void
drop_privileges(struct weston_launch *wl)
{
	if (setgid(wl->pw->pw_gid) < 0 ||
#ifdef HAVE_INITGROUPS
	    initgroups(wl->pw->pw_name, wl->pw->pw_gid) < 0 ||
#endif
	    setuid(wl->pw->pw_uid) < 0) {
		fprintf(stderr, "weston: dropping privileges failed %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void
launch_compositor(struct weston_launch *wl, int argc, char *argv[])
{
	char *child_argv[MAX_ARGV_SIZE];
	sigset_t mask;
	int o, i;

	if (wl->verbose)
		printf("weston-launch: spawned weston with pid: %d\n", getpid());
	if (wl->new_user) {
		o = setup_session(wl, child_argv);
	} else {
		child_argv[0] = BINDIR "/weston";
		o = 1;
	}
	for (i = 0; i < argc; ++i)
		child_argv[o + i] = argv[i];
	child_argv[o + i] = NULL;

	if (geteuid() == 0)
		drop_privileges(wl);

	setenv_fd("WESTON_TTY_FD", wl->tty);
	setenv_fd("WESTON_LAUNCHER_SOCK", wl->sock[1]);

	unsetenv("DISPLAY");

	/* Do not give our signal mask to the new process. */
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);


	execv(child_argv[0], child_argv);
	fprintf(stderr, "weston: exec failed: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
}

static void
help(const char *name)
{
	fprintf(stderr, "Usage: %s [args...] [-- [weston args..]]\n", name);
	fprintf(stderr, "  -u, --user      Start session as specified username,\n"
			"                  e.g. -u joe, requires root.\n");
	fprintf(stderr, "  -t, --tty       Start session on alternative tty,\n"
			"                  e.g. -t /dev/tty4, requires -u option.\n");
	fprintf(stderr, "  -v, --verbose   Be verbose\n");
	fprintf(stderr, "  -h, --help      Display this help message\n");
}

int
main(int argc, char *argv[])
{
	struct weston_launch wl;
	int i, c;
	char *tty = NULL;
	struct option opts[] = {
		{ "user",    required_argument, NULL, 'u' },
		{ "tty",     required_argument, NULL, 't' },
		{ "verbose", no_argument,       NULL, 'v' },
		{ "help",    no_argument,       NULL, 'h' },
		{ 0,         0,                 NULL,  0  }
	};

	memset(&wl, 0, sizeof wl);

	while ((c = getopt_long(argc, argv, "u:t:vh", opts, &i)) != -1) {
		switch (c) {
		case 'u':
			wl.new_user = optarg;
			if (getuid() != 0) {
				fprintf(stderr, "weston: Permission denied. -u allowed for root only\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			tty = optarg;
			break;
		case 'v':
			wl.verbose = 1;
			break;
		case 'h':
			help("weston-launch");
			exit(EXIT_FAILURE);
		default:
			exit(EXIT_FAILURE);
		}
	}

	if ((argc - optind) > (MAX_ARGV_SIZE - 6)) {
		fprintf(stderr,
			"weston: Too many arguments to pass to weston: %s\n",
			strerror(E2BIG));
		exit(EXIT_FAILURE);
	}

	if (tty && !wl.new_user) {
		fprintf(stderr, "weston: -t/--tty option requires -u/--user option as well\n");
		exit(EXIT_FAILURE);
	}

	if (wl.new_user)
		wl.pw = getpwnam(wl.new_user);
	else
		wl.pw = getpwuid(getuid());
	if (wl.pw == NULL) {
		fprintf(stderr, "weston: failed to get username: %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!weston_launch_allowed(&wl)) {
		fprintf(stderr, "Permission denied. You should either:\n"
#ifdef HAVE_SYSTEMD_LOGIN
		      " - run from an active and local (systemd) session.\n"
#else
		      " - enable systemd session support for weston-launch.\n"
#endif
		      " - or add yourself to the 'weston-launch' group.\n");
		exit(EXIT_FAILURE);
	}

	if (setup_tty(&wl, tty) < 0)
		exit(EXIT_FAILURE);

	if (wl.new_user && setup_pam(&wl) < 0)
		exit(EXIT_FAILURE);

	if (setup_launcher_socket(&wl) < 0)
		exit(EXIT_FAILURE);

	if (setup_signals(&wl) < 0)
		exit(EXIT_FAILURE);

	wl.child = fork();
	if (wl.child == -1) {
		fprintf(stderr, "weston: fork failed %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (wl.child == 0)
		launch_compositor(&wl, argc - optind, argv + optind);

	close(wl.sock[1]);

	while (1) {
		struct pollfd fds[2];
		int n;

		fds[0].fd = wl.sock[0];
		fds[0].events = POLLIN;
		fds[1].fd = wl.signalfd;
		fds[1].events = POLLIN;

		n = poll(fds, 2, -1);
		if (n < 0) {
			fprintf(stderr, "poll failed: %s\n", strerror(errno));
			return -1;
		}
		if (fds[0].revents & POLLIN)
			handle_socket_msg(&wl);
		if (fds[1].revents)
			handle_signal(&wl);
	}

	return 0;
}
