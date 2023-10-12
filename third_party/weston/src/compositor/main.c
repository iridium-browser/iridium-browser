/*
 * Copyright © 2010-2011 Intel Corporation
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2012-2018,2022 Collabora, Ltd.
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2013 Jason Ekstrand
 * Copyright © 2017, 2018 General Electric Company
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

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <libinput.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <sys/time.h>
#include <linux/limits.h>

#include "weston.h"
#include <libweston/libweston.h>
#include "shared/os-compatibility.h"
#include "shared/helpers.h"
#include "shared/process-util.h"
#include "shared/string-helpers.h"
#include "git-version.h"
#include <libweston/version.h>
#include "weston.h"
#include "weston-private.h"

#include <libweston/backend-drm.h>
#include <libweston/backend-headless.h>
#include <libweston/backend-pipewire.h>
#include <libweston/backend-rdp.h>
#include <libweston/backend-vnc.h>
#include <libweston/backend-x11.h>
#include <libweston/backend-wayland.h>
#include <libweston/windowed-output-api.h>
#include <libweston/weston-log.h>
#include <libweston/remoting-plugin.h>
#include <libweston/pipewire-plugin.h>

#define WINDOW_TITLE "Weston Compositor"
/* flight recorder size (in bytes) */
#define DEFAULT_FLIGHT_REC_SIZE (5 * 1024 * 1024)
#define DEFAULT_FLIGHT_REC_SCOPES "log,drm-backend"

struct wet_output_config {
	int width;
	int height;
	int32_t scale;
	uint32_t transform;
};

struct wet_compositor;
struct wet_layoutput;

struct wet_head_tracker {
	struct wl_listener head_destroy_listener;
};

/** User data for each weston_output */
struct wet_output {
	struct weston_output *output;
	struct wl_listener output_destroy_listener;
	struct wet_layoutput *layoutput;
	struct wl_list link;	/**< in wet_layoutput::output_list */
};

#define MAX_CLONE_HEADS 16

struct wet_head_array {
	struct weston_head *heads[MAX_CLONE_HEADS];	/**< heads to add */
	unsigned n;				/**< the number of heads */
};

/** A layout output
 *
 * Contains wet_outputs that are all clones (independent CRTCs).
 * Stores output layout information in the future.
 */
struct wet_layoutput {
	struct wet_compositor *compositor;
	struct wl_list compositor_link;	/**< in wet_compositor::layoutput_list */
	struct wl_list output_list;	/**< wet_output::link */
	char *name;
	struct weston_config_section *section;
	struct wet_head_array add;	/**< tmp: heads to add as clones */
};

struct wet_compositor {
	struct weston_compositor *compositor;
	struct weston_config *config;
	struct wet_output_config *parsed_options;
	bool drm_use_current_mode;
	struct wl_listener heads_changed_listener;
	int (*simple_output_configure)(struct weston_output *output);
	bool init_failed;
	struct wl_list layoutput_list;	/**< wet_layoutput::compositor_link */
	struct wl_list child_process_list;
	pid_t autolaunch_pid;
	bool autolaunch_watch;
	bool use_color_manager;
	struct wl_listener screenshot_auth;
};

static FILE *weston_logfile = NULL;
static struct weston_log_scope *log_scope;
static struct weston_log_scope *protocol_scope;
static int cached_tm_mday = -1;

static void
custom_handler(const char *fmt, va_list arg)
{
	char timestr[512];

	weston_log_scope_printf(log_scope, "%s libwayland: ",
				weston_log_timestamp(timestr,
				sizeof(timestr), &cached_tm_mday));
	weston_log_scope_vprintf(log_scope, fmt, arg);
}

static bool
weston_log_file_open(const char *filename)
{
	wl_log_set_handler_server(custom_handler);

	if (filename != NULL) {
		weston_logfile = fopen(filename, "a");
		if (weston_logfile) {
			os_fd_set_cloexec(fileno(weston_logfile));
		} else {
			fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
			return false;
		}
	}

	if (weston_logfile == NULL)
		weston_logfile = stderr;
	else
		setvbuf(weston_logfile, NULL, _IOLBF, 256);

	return true;
}

static void
weston_log_file_close(void)
{
	if ((weston_logfile != stderr) && (weston_logfile != NULL))
		fclose(weston_logfile);
	weston_logfile = stderr;
}

static int
vlog(const char *fmt, va_list ap)
{
	const char *oom = "Out of memory";
	char timestr[128];
	int len = 0;
	char *str;

	if (weston_log_scope_is_enabled(log_scope)) {
		int len_va;
		char *log_timestamp = weston_log_timestamp(timestr,
							   sizeof(timestr),
							   &cached_tm_mday);
		len_va = vasprintf(&str, fmt, ap);
		if (len_va >= 0) {
			len = weston_log_scope_printf(log_scope, "%s %s",
						      log_timestamp, str);
			free(str);
		} else {
			len = weston_log_scope_printf(log_scope, "%s %s",
						      log_timestamp, oom);
		}
	}

	return len;
}

static int
vlog_continue(const char *fmt, va_list argp)
{
	return weston_log_scope_vprintf(log_scope, fmt, argp);
}

static const char *
get_next_argument(const char *signature, char* type)
{
	for(; *signature; ++signature) {
		switch(*signature) {
		case 'i':
		case 'u':
		case 'f':
		case 's':
		case 'o':
		case 'n':
		case 'a':
		case 'h':
			*type = *signature;
			return signature + 1;
		}
	}
	*type = '\0';
	return signature;
}

static void
protocol_log_fn(void *user_data,
		enum wl_protocol_logger_type direction,
		const struct wl_protocol_logger_message *message)
{
	FILE *fp;
	char *logstr;
	size_t logsize;
	char timestr[128];
	struct wl_resource *res = message->resource;
	struct wl_client *client = wl_resource_get_client(res);
	pid_t pid = 0;
	const char *signature = message->message->signature;
	int i;
	char type;

	if (!weston_log_scope_is_enabled(protocol_scope))
		return;

	fp = open_memstream(&logstr, &logsize);
	if (!fp)
		return;

	wl_client_get_credentials(client, &pid, NULL, NULL);

	weston_log_scope_timestamp(protocol_scope,
			timestr, sizeof timestr);
	fprintf(fp, "%s ", timestr);
	fprintf(fp, "client %p (PID %d) %s ", client, pid,
		direction == WL_PROTOCOL_LOGGER_REQUEST ? "rq" : "ev");
	fprintf(fp, "%s@%u.%s(",
		wl_resource_get_class(res),
		wl_resource_get_id(res),
		message->message->name);

	for (i = 0; i < message->arguments_count; i++) {
		signature = get_next_argument(signature, &type);

		if (i > 0)
			fprintf(fp, ", ");

		switch (type) {
		case 'u':
			fprintf(fp, "%u", message->arguments[i].u);
			break;
		case 'i':
			fprintf(fp, "%d", message->arguments[i].i);
			break;
		case 'f':
			fprintf(fp, "%f",
				wl_fixed_to_double(message->arguments[i].f));
			break;
		case 's':
			fprintf(fp, "\"%s\"", message->arguments[i].s);
			break;
		case 'o':
			if (message->arguments[i].o) {
				struct wl_resource* resource;
				resource = (struct wl_resource*) message->arguments[i].o;
				fprintf(fp, "%s@%u",
					wl_resource_get_class(resource),
					wl_resource_get_id(resource));
			}
			else
				fprintf(fp, "nil");
			break;
		case 'n':
			fprintf(fp, "new id %s@",
				(message->message->types[i]) ?
				message->message->types[i]->name :
				"[unknown]");
			if (message->arguments[i].n != 0)
				fprintf(fp, "%u", message->arguments[i].n);
			else
				fprintf(fp, "nil");
			break;
		case 'a':
			fprintf(fp, "array");
			break;
		case 'h':
			fprintf(fp, "fd %d", message->arguments[i].h);
			break;
		}
	}

	fprintf(fp, ")\n");

	if (fclose(fp) == 0)
		weston_log_scope_write(protocol_scope, logstr, logsize);

	free(logstr);
}

static struct wet_compositor *
to_wet_compositor(struct weston_compositor *compositor)
{
	return weston_compositor_get_user_data(compositor);
}

static int
sigchld_handler(int signal_number, void *data)
{
	struct weston_process *p;
	struct wet_compositor *wet = data;
	int status;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		if (wet->autolaunch_pid != -1 && wet->autolaunch_pid == pid) {
			if (wet->autolaunch_watch)
				wl_display_terminate(wet->compositor->wl_display);
			wet->autolaunch_pid = -1;
			continue;
		}

		wl_list_for_each(p, &wet->child_process_list, link) {
			if (p->pid == pid)
				break;
		}

		/* An unknown child process exited. Oh well. */
		if (&p->link == &wet->child_process_list)
			continue;

		wl_list_remove(&p->link);
		wl_list_init(&p->link);
		p->cleanup(p, status);
	}

	if (pid < 0 && errno != ECHILD)
		weston_log("waitpid error %s\n", strerror(errno));

	return 1;
}

static void
cleanup_for_child_process() {
	sigset_t allsigs;

	/* Put the client in a new session so it won't catch signals
	* intended for the parent. Sharing a session can be
	* confusing when launching weston under gdb, as the ctrl-c
	* intended for gdb will pass to the child, and weston
	* will cleanly shut down when the child exits.
	*/
	setsid();

	/* do not give our signal mask to the new process */
	sigfillset(&allsigs);
	sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
}

WL_EXPORT bool
weston_client_launch(struct weston_compositor *compositor,
		     struct weston_process *proc,
		     struct custom_env *child_env,
		     int *no_cloexec_fds,
		     size_t num_no_cloexec_fds,
		     weston_process_cleanup_func_t cleanup)
{
	const char *fail_cloexec = "Couldn't unset CLOEXEC on child FDs";
	const char *fail_seteuid = "Couldn't call seteuid";
	char *fail_exec;
	char * const *argp;
	char * const *envp;
	pid_t pid;
	int err;
	bool ret;
	size_t i;
	size_t written __attribute__((unused));

	argp = custom_env_get_argp(child_env);
	envp = custom_env_get_envp(child_env);

	weston_log("launching '%s'\n", argp[0]);
	str_printf(&fail_exec, "Error: Couldn't launch client '%s'\n", argp[0]);

	pid = fork();
	switch (pid) {
	case 0:
		cleanup_for_child_process();

		/* Launch clients as the user. Do not launch clients with wrong euid. */
		if (seteuid(getuid()) == -1) {
			written = write(STDERR_FILENO, fail_seteuid,
					strlen(fail_seteuid));
			_exit(EXIT_FAILURE);
		}

		for (i = 0; i < num_no_cloexec_fds; i++) {
			err = os_fd_clear_cloexec(no_cloexec_fds[i]);
			if (err < 0) {
				written = write(STDERR_FILENO, fail_cloexec,
						strlen(fail_cloexec));
				_exit(EXIT_FAILURE);
			}
		}

		execve(argp[0], argp, envp);

		if (fail_exec)
			written = write(STDERR_FILENO, fail_exec,
					strlen(fail_exec));
		_exit(EXIT_FAILURE);

	default:
		proc->pid = pid;
		proc->cleanup = cleanup;
		wet_watch_process(compositor, proc);
		ret = true;
		break;

	case -1:
		weston_log("weston_client_launch: "
			   "fork failed while launching '%s': %s\n", argp[0],
			   strerror(errno));
		ret = false;
		break;
	}

	custom_env_fini(child_env);
	free(fail_exec);
	return ret;
}

WL_EXPORT void
wet_watch_process(struct weston_compositor *compositor,
		  struct weston_process *process)
{
	struct wet_compositor *wet = to_wet_compositor(compositor);
	wl_list_insert(&wet->child_process_list, &process->link);
}

struct process_info {
	struct weston_process proc;
	char *path;
};

static void
process_handle_sigchld(struct weston_process *process, int status)
{
	struct process_info *pinfo =
		container_of(process, struct process_info, proc);

	/*
	 * There are no guarantees whether this runs before or after
	 * the wl_client destructor.
	 */

	if (WIFEXITED(status)) {
		weston_log("%s exited with status %d\n", pinfo->path,
			   WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		weston_log("%s died on signal %d\n", pinfo->path,
			   WTERMSIG(status));
	} else {
		weston_log("%s disappeared\n", pinfo->path);
	}

	free(pinfo->path);
	free(pinfo);
}

WL_EXPORT struct wl_client *
weston_client_start(struct weston_compositor *compositor, const char *path)
{
	struct process_info *pinfo;
	struct wl_client *client;
	struct custom_env child_env;
	struct fdstr wayland_socket = FDSTR_INIT;
	int no_cloexec_fds[1];
	size_t num_no_cloexec_fds = 0;
	bool ret;

	pinfo = zalloc(sizeof *pinfo);
	if (!pinfo)
		return NULL;

	pinfo->path = strdup(path);
	if (!pinfo->path)
		goto out_free;

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0,
				  wayland_socket.fds) < 0) {
		weston_log("weston_client_start: "
			   "socketpair failed while launching '%s': %s\n",
			   path, strerror(errno));
		goto out_path;
	}

	custom_env_init_from_environ(&child_env);
	custom_env_add_from_exec_string(&child_env, path);

	fdstr_update_str1(&wayland_socket);
	no_cloexec_fds[num_no_cloexec_fds++] = wayland_socket.fds[1];
	custom_env_set_env_var(&child_env, "WAYLAND_SOCKET",
			       wayland_socket.str1);

	assert(num_no_cloexec_fds <= ARRAY_LENGTH(no_cloexec_fds));

	ret = weston_client_launch(compositor, &pinfo->proc, &child_env,
				   no_cloexec_fds, num_no_cloexec_fds,
				   process_handle_sigchld);
	if (!ret)
		goto out_path;

	client = wl_client_create(compositor->wl_display,
				  wayland_socket.fds[0]);
	if (!client) {
		weston_log("weston_client_start: "
			"wl_client_create failed while launching '%s'.\n",
			path);
		/* We have no way of killing the process, so leave it hanging */
		goto out_sock;
	}

	/* Close the child end of our socket which we no longer need */
	close(wayland_socket.fds[1]);

	return client;

out_path:
	free(pinfo->path);
out_free:
	free(pinfo);
out_sock:
	fdstr_close_all(&wayland_socket);

	return NULL;
}

static void
log_uname(void)
{
	struct utsname usys;

	uname(&usys);

	weston_log("OS: %s, %s, %s, %s\n", usys.sysname, usys.release,
						usys.version, usys.machine);
}

static struct wet_output_config *
wet_init_parsed_options(struct weston_compositor *ec)
{
	struct wet_compositor *compositor = to_wet_compositor(ec);
	struct wet_output_config *config;

	config = zalloc(sizeof *config);

	if (!config) {
		perror("out of memory");
		return NULL;
	}

	config->width = 0;
	config->height = 0;
	config->scale = 0;
	config->transform = UINT32_MAX;

	compositor->parsed_options = config;

	return config;
}

WL_EXPORT struct weston_config *
wet_get_config(struct weston_compositor *ec)
{
	struct wet_compositor *compositor = to_wet_compositor(ec);

	return compositor->config;
}

static const char xdg_error_message[] =
	"fatal: environment variable XDG_RUNTIME_DIR is not set.\n";

static const char xdg_wrong_message[] =
	"fatal: environment variable XDG_RUNTIME_DIR\n"
	"is set to \"%s\", which is not a directory.\n";

static const char xdg_wrong_mode_message[] =
	"warning: XDG_RUNTIME_DIR \"%s\" is not configured\n"
	"correctly.  Unix access mode must be 0700 (current mode is %04o),\n"
	"and must be owned by the user UID %d (current owner is UID %d).\n";

static const char xdg_detail_message[] =
	"Refer to your distribution on how to get it, or\n"
	"http://www.freedesktop.org/wiki/Specifications/basedir-spec\n"
	"on how to implement it.\n";

static void
verify_xdg_runtime_dir(void)
{
	char *dir = getenv("XDG_RUNTIME_DIR");
	struct stat s;

	if (!dir) {
		weston_log(xdg_error_message);
		weston_log_continue(xdg_detail_message);
		exit(EXIT_FAILURE);
	}

	if (stat(dir, &s) || !S_ISDIR(s.st_mode)) {
		weston_log(xdg_wrong_message, dir);
		weston_log_continue(xdg_detail_message);
		exit(EXIT_FAILURE);
	}

	if ((s.st_mode & 0777) != 0700 || s.st_uid != getuid()) {
		weston_log(xdg_wrong_mode_message,
			   dir, s.st_mode & 0777, getuid(), s.st_uid);
		weston_log_continue(xdg_detail_message);
	}
}

static int
usage(int error_code)
{
	FILE *out = error_code == EXIT_SUCCESS ? stdout : stderr;

	fprintf(out,
		"Usage: weston [OPTIONS]\n\n"
		"This is weston version " VERSION ", the Wayland reference compositor.\n"
		"Weston supports multiple backends, and depending on which backend is in use\n"
		"different options will be accepted.\n\n"


		"Core options:\n\n"
		"  --version\t\tPrint weston version\n"
		"  -B, --backend=BACKEND\tBackend module, one of\n"
#if defined(BUILD_DRM_COMPOSITOR)
			"\t\t\t\tdrm\n"
#endif
#if defined(BUILD_HEADLESS_COMPOSITOR)
			"\t\t\t\theadless\n"
#endif
#if defined(BUILD_PIPEWIRE_COMPOSITOR)
			"\t\t\t\tpipewire\n"
#endif
#if defined(BUILD_RDP_COMPOSITOR)
			"\t\t\t\trdp\n"
#endif
#if defined(BUILD_VNC_COMPOSITOR)
			"\t\t\t\tvnc\n"
#endif
#if defined(BUILD_WAYLAND_COMPOSITOR)
			"\t\t\t\twayland\n"
#endif
#if defined(BUILD_X11_COMPOSITOR)
			"\t\t\t\tx11\n"
#endif
		"  --renderer=NAME\tRenderer to use, one of\n"
			"\t\t\t\tauto\tAutomatic selection of one of the below renderers\n"
#if defined(ENABLE_EGL)
			"\t\t\t\tgl\tOpenGL ES\n"
#endif
			"\t\t\t\tnoop\tNo-op renderer for testing only\n"
			"\t\t\t\tpixman\tPixman software renderer\n"
		"  --shell=NAME\tShell to load, defaults to desktop\n"
		"  -S, --socket=NAME\tName of socket to listen on\n"
		"  -i, --idle-time=SECS\tIdle time in seconds\n"
#if defined(BUILD_XWAYLAND)
		"  --xwayland\t\tLoad the xwayland module\n"
#endif
		"  --modules\t\tLoad the comma-separated list of modules\n"
		"  --log=FILE\t\tLog to the given file\n"
		"  -c, --config=FILE\tConfig file to load, defaults to weston.ini\n"
		"  --no-config\t\tDo not read weston.ini\n"
		"  --wait-for-debugger\tRaise SIGSTOP on start-up\n"
		"  --debug\t\tEnable debug extension\n"
		"  -l, --logger-scopes=SCOPE\n\t\t\tSpecify log scopes to "
			"subscribe to.\n\t\t\tCan specify multiple scopes, "
			"each followed by comma\n"
		"  -f, --flight-rec-scopes=SCOPE\n\t\t\tSpecify log scopes to "
			"subscribe to.\n\t\t\tCan specify multiple scopes, "
			"each followed by comma\n"
		"  -h, --help\t\tThis help message\n\n");

#if defined(BUILD_DRM_COMPOSITOR)
	fprintf(out,
		"Options for drm:\n\n"
		"  --seat=SEAT\t\tThe seat that weston should run on, instead of the seat defined in XDG_SEAT\n"
		"  --drm-device=CARD\tThe DRM device to use, e.g. \"card0\".\n"
		"  --use-pixman\t\tUse the pixman (CPU) renderer (deprecated alias for --renderer=pixman)\n"
		"  --current-mode\tPrefer current KMS mode over EDID preferred mode\n"
		"  --continue-without-input\tAllow the compositor to start without input devices\n\n");
#endif

#if defined(BUILD_HEADLESS_COMPOSITOR)
	fprintf(out,
		"Options for headless:\n\n"
		"  --width=WIDTH\t\tWidth of memory surface\n"
		"  --height=HEIGHT\tHeight of memory surface\n"
		"  --scale=SCALE\t\tScale factor of output\n"
		"  --transform=TR\tThe output transformation, TR is one of:\n"
		"\tnormal 90 180 270 flipped flipped-90 flipped-180 flipped-270\n"
		"  --use-pixman\t\tUse the pixman (CPU) renderer (deprecated alias for --renderer=pixman)\n"
		"  --use-gl\t\tUse the GL renderer (deprecated alias for --renderer=gl)\n"
		"  --no-outputs\t\tDo not create any virtual outputs\n"
		"\n");
#endif

#if defined(BUILD_PIPEWIRE_COMPOSITOR)
	fprintf(out,
		"Options for pipewire\n\n"
		"  --width=WIDTH\t\tWidth of desktop\n"
		"  --height=HEIGHT\tHeight of desktop\n"
		"\n");
#endif

#if defined(BUILD_RDP_COMPOSITOR)
	fprintf(out,
		"Options for rdp:\n\n"
		"  --width=WIDTH\t\tWidth of desktop\n"
		"  --height=HEIGHT\tHeight of desktop\n"
		"  --env-socket\t\tUse socket defined in RDP_FD env variable as peer connection\n"
		"  --external-listener-fd=FD\tUse socket as listener connection\n"
		"  --address=ADDR\tThe address to bind\n"
		"  --port=PORT\t\tThe port to listen on\n"
		"  --no-clients-resize\tThe RDP peers will be forced to the size of the desktop\n"
		"  --rdp4-key=FILE\tThe file containing the key for RDP4 encryption\n"
		"  --rdp-tls-cert=FILE\tThe file containing the certificate for TLS encryption\n"
		"  --rdp-tls-key=FILE\tThe file containing the private key for TLS encryption\n"
		"\n");
#endif

#if defined(BUILD_VNC_COMPOSITOR)
	fprintf(out,
		"Options for vnc:\n\n"
		"  --width=WIDTH\t\tWidth of desktop\n"
		"  --height=HEIGHT\tHeight of desktop\n"
		"  --port=PORT\t\tThe port to listen on\n"
		"  --vnc-tls-cert=FILE\tThe file containing the certificate for TLS encryption\n"
		"  --vnc-tls-key=FILE\tThe file containing the private key for TLS encryption\n"
		"\n");
#endif

#if defined(BUILD_WAYLAND_COMPOSITOR)
	fprintf(out,
		"Options for wayland:\n\n"
		"  --width=WIDTH\t\tWidth of Wayland surface\n"
		"  --height=HEIGHT\tHeight of Wayland surface\n"
		"  --scale=SCALE\t\tScale factor of output\n"
		"  --fullscreen\t\tRun in fullscreen mode\n"
		"  --use-pixman\t\tUse the pixman (CPU) renderer (deprecated alias for --renderer=pixman)\n"
		"  --output-count=COUNT\tCreate multiple outputs\n"
		"  --sprawl\t\tCreate one fullscreen output for every parent output\n"
		"  --display=DISPLAY\tWayland display to connect to\n\n");
#endif

#if defined(BUILD_X11_COMPOSITOR)
	fprintf(out,
		"Options for x11:\n\n"
		"  --width=WIDTH\t\tWidth of X window\n"
		"  --height=HEIGHT\tHeight of X window\n"
		"  --scale=SCALE\t\tScale factor of output\n"
		"  --fullscreen\t\tRun in fullscreen mode\n"
		"  --use-pixman\t\tUse the pixman (CPU) renderer (deprecated alias for --renderer=pixman)\n"
		"  --output-count=COUNT\tCreate multiple outputs\n"
		"  --no-input\t\tDont create input devices\n\n");
#endif

	exit(error_code);
}

static int on_term_signal(int signal_number, void *data)
{
	struct wl_display *display = data;

	weston_log("caught signal %d\n", signal_number);
	wl_display_terminate(display);

	return 1;
}

static const char *
clock_name(clockid_t clk_id)
{
	static const char *names[] = {
		[CLOCK_REALTIME] =		"CLOCK_REALTIME",
		[CLOCK_MONOTONIC] =		"CLOCK_MONOTONIC",
		[CLOCK_MONOTONIC_RAW] =		"CLOCK_MONOTONIC_RAW",
		[CLOCK_REALTIME_COARSE] =	"CLOCK_REALTIME_COARSE",
		[CLOCK_MONOTONIC_COARSE] =	"CLOCK_MONOTONIC_COARSE",
#ifdef CLOCK_BOOTTIME
		[CLOCK_BOOTTIME] =		"CLOCK_BOOTTIME",
#endif
	};

	if (clk_id < 0 || (unsigned)clk_id >= ARRAY_LENGTH(names))
		return "unknown";

	return names[clk_id];
}

static const struct {
	uint32_t bit; /* enum weston_capability */
	const char *desc;
} capability_strings[] = {
	{ WESTON_CAP_ROTATION_ANY, "arbitrary surface rotation" },
	{ WESTON_CAP_CAPTURE_YFLIP, "screen capture uses y-flip" },
	{ WESTON_CAP_CURSOR_PLANE, "cursor planes" },
	{ WESTON_CAP_ARBITRARY_MODES, "arbitrary resolutions" },
	{ WESTON_CAP_VIEW_CLIP_MASK, "view mask clipping" },
	{ WESTON_CAP_EXPLICIT_SYNC, "explicit sync" },
	{ WESTON_CAP_COLOR_OPS, "color operations" },
};

static void
weston_compositor_log_capabilities(struct weston_compositor *compositor)
{
	unsigned i;
	int yes;
	struct timespec res;

	weston_log("Compositor capabilities:\n");
	for (i = 0; i < ARRAY_LENGTH(capability_strings); i++) {
		yes = compositor->capabilities & capability_strings[i].bit;
		weston_log_continue(STAMP_SPACE "%s: %s\n",
				    capability_strings[i].desc,
				    yes ? "yes" : "no");
	}

	weston_log_continue(STAMP_SPACE "presentation clock: %s, id %d\n",
			    clock_name(compositor->presentation_clock),
			    compositor->presentation_clock);

	if (clock_getres(compositor->presentation_clock, &res) == 0)
		weston_log_continue(STAMP_SPACE
				"presentation clock resolution: %d.%09ld s\n",
				(int)res.tv_sec, res.tv_nsec);
	else
		weston_log_continue(STAMP_SPACE
				"presentation clock resolution: N/A\n");
}

static bool
check_compositor_capabilities(struct weston_compositor *compositor,
			      uint32_t mask)
{
	uint32_t missing = mask & ~compositor->capabilities;
	unsigned i;

	if (missing == 0)
		return true;

	weston_log("Quirk error, missing capabilities:\n");
	for (i = 0; i < ARRAY_LENGTH(capability_strings); i++) {
		if (!(missing & capability_strings[i].bit))
			continue;

		weston_log_continue(STAMP_SPACE "- %s\n",
				    capability_strings[i].desc);
		missing &= ~capability_strings[i].bit;
	}
	if (missing) {
		weston_log_continue(STAMP_SPACE "- unlisted bits 0x%x\n",
				    missing);
	}

	return false;
}

static void
handle_primary_client_destroyed(struct wl_listener *listener, void *data)
{
	struct wl_client *client = data;

	weston_log("Primary client died.  Closing...\n");

	wl_display_terminate(wl_client_get_display(client));
}

static int
weston_create_listening_socket(struct wl_display *display, const char *socket_name)
{
	char name_candidate[32];

	if (socket_name) {
		if (wl_display_add_socket(display, socket_name)) {
			weston_log("fatal: failed to add socket: %s\n",
				   strerror(errno));
			return -1;
		}

		setenv("WAYLAND_DISPLAY", socket_name, 1);
		return 0;
	} else {
		for (int i = 1; i <= 32; i++) {
			sprintf(name_candidate, "wayland-%d", i);
			if (wl_display_add_socket(display, name_candidate) >= 0) {
				setenv("WAYLAND_DISPLAY", name_candidate, 1);
				return 0;
			}
		}
		weston_log("fatal: failed to add socket: %s\n",
			   strerror(errno));
		return -1;
	}
}

WL_EXPORT int
wet_load_module(struct weston_compositor *compositor,
	        const char *name, int *argc, char *argv[])
{
	int (*module_init)(struct weston_compositor *ec,
			   int *argc, char *argv[]);

	module_init = weston_load_module(name, "wet_module_init", MODULEDIR);
	if (!module_init)
		return -1;
	if (module_init(compositor, argc, argv) < 0)
		return -1;
	return 0;
}

static int
wet_load_shell(struct weston_compositor *compositor,
	       const char *_name, int *argc, char *argv[])
{
	char *name;
	int (*shell_init)(struct weston_compositor *ec,
			  int *argc, char *argv[]);

	if (strstr(_name, "-shell.so"))
		name = strdup(_name);
	else
		str_printf(&name, "%s-shell.so", _name);
	assert(name);

	shell_init = weston_load_module(name, "wet_shell_init", MODULEDIR);
	free(name);

	if (!shell_init)
		return -1;
	if (shell_init(compositor, argc, argv) < 0)
		return -1;
	return 0;
}

static char *
wet_get_binary_path(const char *name, const char *dir)
{
	char path[PATH_MAX];
	size_t len;

	len = weston_module_path_from_env(name, path, sizeof path);
	if (len > 0)
		return strdup(path);

	len = snprintf(path, sizeof path, "%s/%s", dir, name);
	if (len >= sizeof path)
		return NULL;

	return strdup(path);
}

WL_EXPORT char *
wet_get_libexec_path(const char *name)
{
	return wet_get_binary_path(name, LIBEXECDIR);
}

WL_EXPORT char *
wet_get_bindir_path(const char *name)
{
	return wet_get_binary_path(name, BINDIR);
}

static int
load_modules(struct weston_compositor *ec, const char *modules,
	     int *argc, char *argv[])
{
	const char *p, *end;
	char buffer[256];

	if (modules == NULL)
		return 0;

	p = modules;
	while (*p) {
		end = strchrnul(p, ',');
		snprintf(buffer, sizeof buffer, "%.*s", (int) (end - p), p);

		if (strstr(buffer, "xwayland.so")) {
			weston_log("fatal: Old Xwayland module loading detected: "
				   "Please use --xwayland command line option "
				   "or set xwayland=true in the [core] section "
				   "in weston.ini\n");
			return -1;
		} else {
			if (wet_load_module(ec, buffer, argc, argv) < 0)
				return -1;
		}

		p = end;
		while (*p == ',')
			p++;
	}

	return 0;
}

static int
save_touch_device_calibration(struct weston_compositor *compositor,
			      struct weston_touch_device *device,
			      const struct weston_touch_device_matrix *calibration)
{
	struct weston_config_section *s;
	struct weston_config *config = wet_get_config(compositor);
	char *helper = NULL;
	char *helper_cmd = NULL;
	int ret = -1;
	int status;
	const float *m = calibration->m;

	s = weston_config_get_section(config,
				      "libinput", NULL, NULL);

	weston_config_section_get_string(s, "calibration_helper",
					 &helper, NULL);

	if (!helper || strlen(helper) == 0) {
		ret = 0;
		goto out;
	}

	if (asprintf(&helper_cmd, "\"%s\" '%s' %f %f %f %f %f %f",
		     helper, device->syspath,
		     m[0], m[1], m[2],
		     m[3], m[4], m[5]) < 0)
		goto out;

	status = system(helper_cmd);
	free(helper_cmd);

	if (status < 0) {
		weston_log("Error: failed to run calibration helper '%s'.\n",
			   helper);
		goto out;
	}

	if (!WIFEXITED(status)) {
		weston_log("Error: calibration helper '%s' possibly killed.\n",
			   helper);
		goto out;
	}

	if (WEXITSTATUS(status) == 0) {
		ret = 0;
	} else {
		weston_log("Calibration helper '%s' exited with status %d.\n",
			   helper, WEXITSTATUS(status));
	}

out:
	free(helper);

	return ret;
}

static int
weston_compositor_init_config(struct weston_compositor *ec,
			      struct weston_config *config)
{
	struct wet_compositor *compositor = to_wet_compositor(ec);
	struct xkb_rule_names xkb_names;
	struct weston_config_section *s;
	int repaint_msec;
	bool color_management;
	bool cal;

	/* weston.ini [keyboard] */
	s = weston_config_get_section(config, "keyboard", NULL, NULL);
	weston_config_section_get_string(s, "keymap_rules",
					 (char **) &xkb_names.rules, NULL);
	weston_config_section_get_string(s, "keymap_model",
					 (char **) &xkb_names.model, NULL);
	weston_config_section_get_string(s, "keymap_layout",
					 (char **) &xkb_names.layout, NULL);
	weston_config_section_get_string(s, "keymap_variant",
					 (char **) &xkb_names.variant, NULL);
	weston_config_section_get_string(s, "keymap_options",
					 (char **) &xkb_names.options, NULL);

	if (weston_compositor_set_xkb_rule_names(ec, &xkb_names) < 0)
		return -1;

	weston_config_section_get_int(s, "repeat-rate",
				      &ec->kb_repeat_rate, 40);
	weston_config_section_get_int(s, "repeat-delay",
				      &ec->kb_repeat_delay, 400);

	weston_config_section_get_bool(s, "vt-switching",
				       &ec->vt_switching, true);

	/* weston.ini [core] */
	s = weston_config_get_section(config, "core", NULL, NULL);
	weston_config_section_get_int(s, "repaint-window", &repaint_msec,
				      ec->repaint_msec);
	if (repaint_msec < -10 || repaint_msec > 1000) {
		weston_log("Invalid repaint_window value in config: %d\n",
			   repaint_msec);
	} else {
		ec->repaint_msec = repaint_msec;
	}
	weston_log("Output repaint window is %d ms maximum.\n",
		   ec->repaint_msec);

	weston_config_section_get_bool(s, "color-management",
				       &color_management, false);
	if (color_management) {
		if (weston_compositor_load_color_manager(ec) < 0)
			return -1;
		else
			compositor->use_color_manager = true;
	}

	/* weston.ini [libinput] */
	s = weston_config_get_section(config, "libinput", NULL, NULL);
	weston_config_section_get_bool(s, "touchscreen_calibrator", &cal, 0);
	if (cal)
		weston_compositor_enable_touch_calibrator(ec,
						save_touch_device_calibration);

	return 0;
}

static char *
weston_choose_default_backend(void)
{
	char *backend = NULL;

	if (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET"))
		backend = strdup("wayland");
	else if (getenv("DISPLAY"))
		backend = strdup("x11");
	else
		backend = strdup(WESTON_NATIVE_BACKEND);

	return backend;
}

static const struct { const char *name; uint32_t token; } transforms[] = {
	{ "normal",             WL_OUTPUT_TRANSFORM_NORMAL },
	{ "rotate-90",          WL_OUTPUT_TRANSFORM_90 },
	{ "rotate-180",         WL_OUTPUT_TRANSFORM_180 },
	{ "rotate-270",         WL_OUTPUT_TRANSFORM_270 },
	{ "flipped",            WL_OUTPUT_TRANSFORM_FLIPPED },
	{ "flipped-rotate-90",  WL_OUTPUT_TRANSFORM_FLIPPED_90 },
	{ "flipped-rotate-180", WL_OUTPUT_TRANSFORM_FLIPPED_180 },
	{ "flipped-rotate-270", WL_OUTPUT_TRANSFORM_FLIPPED_270 },
};

WL_EXPORT int
weston_parse_transform(const char *transform, uint32_t *out)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(transforms); i++)
		if (strcmp(transforms[i].name, transform) == 0) {
			*out = transforms[i].token;
			return 0;
		}

	*out = WL_OUTPUT_TRANSFORM_NORMAL;
	return -1;
}

WL_EXPORT const char *
weston_transform_to_string(uint32_t output_transform)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(transforms); i++)
		if (transforms[i].token == output_transform)
			return transforms[i].name;

	return "<illegal value>";
}

static int
load_configuration(struct weston_config **config, int32_t noconfig,
		   const char *config_file)
{
	const char *file = "weston.ini";
	const char *full_path;

	*config = NULL;

	if (config_file)
		file = config_file;

	if (noconfig == 0)
		*config = weston_config_parse(file);

	if (*config) {
		full_path = weston_config_get_full_path(*config);

		weston_log("Using config file '%s'\n", full_path);
		setenv(WESTON_CONFIG_FILE_ENV_VAR, full_path, 1);

		return 0;
	}

	if (config_file && noconfig == 0) {
		weston_log("fatal: error opening or reading config file"
			   " '%s'.\n", config_file);

		return -1;
	}

	weston_log("Starting with no config file.\n");
	setenv(WESTON_CONFIG_FILE_ENV_VAR, "", 1);

	return 0;
}

static void
handle_exit(struct weston_compositor *c)
{
	wl_display_terminate(c->wl_display);
}

static void
wet_output_set_scale(struct weston_output *output,
		     struct weston_config_section *section,
		     int32_t default_scale,
		     int32_t parsed_scale)
{
	int32_t scale = default_scale;

	if (section)
		weston_config_section_get_int(section, "scale", &scale, default_scale);

	if (parsed_scale)
		scale = parsed_scale;

	weston_output_set_scale(output, scale);
}

/* UINT32_MAX is treated as invalid because 0 is a valid
 * enumeration value and the parameter is unsigned
 */
static int
wet_output_set_transform(struct weston_output *output,
			 struct weston_config_section *section,
			 uint32_t default_transform,
			 uint32_t parsed_transform)
{
	char *t = NULL;
	uint32_t transform = default_transform;

	if (section) {
		weston_config_section_get_string(section,
						 "transform", &t, NULL);
	}

	if (t) {
		if (weston_parse_transform(t, &transform) < 0) {
			weston_log("Invalid transform \"%s\" for output %s\n",
				   t, output->name);
			return -1;
		}
		free(t);
	}

	if (parsed_transform != UINT32_MAX)
		transform = parsed_transform;

	weston_output_set_transform(output, transform);

	return 0;
}

static int
wet_output_set_color_profile(struct weston_output *output,
			     struct weston_config_section *section,
			     struct weston_color_profile *parent_winsys_profile)
{
	struct wet_compositor *compositor = to_wet_compositor(output->compositor);
	struct weston_color_profile *cprof;
	char *icc_file = NULL;
	bool ok;

	if (!compositor->use_color_manager)
		return 0;

	if (section) {
		weston_config_section_get_string(section, "icc_profile",
						 &icc_file, NULL);
	}

	if (icc_file) {
		cprof = weston_compositor_load_icc_file(output->compositor,
							icc_file);
		free(icc_file);
	} else if (parent_winsys_profile) {
		cprof = weston_color_profile_ref(parent_winsys_profile);
	} else {
		return 0;
	}

	if (!cprof)
		return -1;

	ok = weston_output_set_color_profile(output, cprof);
	if (!ok) {
		weston_log("Error: failed to set color profile '%s' for output %s\n",
			   weston_color_profile_get_description(cprof),
			   output->name);
	}

	weston_color_profile_unref(cprof);
	return ok ? 0 : -1;
}

static int
wet_output_set_eotf_mode(struct weston_output *output,
			 struct weston_config_section *section)
{
	static const struct {
		const char *name;
		enum weston_eotf_mode eotf_mode;
	} modes[] = {
		{ "sdr",	WESTON_EOTF_MODE_SDR },
		{ "hdr-gamma",	WESTON_EOTF_MODE_TRADITIONAL_HDR },
		{ "st2084",	WESTON_EOTF_MODE_ST2084 },
		{ "hlg",	WESTON_EOTF_MODE_HLG },
	};
	struct wet_compositor *compositor;
	enum weston_eotf_mode eotf_mode = WESTON_EOTF_MODE_SDR;
	char *str = NULL;
	unsigned i;

	compositor = to_wet_compositor(output->compositor);

	if (section) {
		weston_config_section_get_string(section, "eotf-mode",
						 &str, NULL);
	}

	if (!str) {
		/* The default SDR mode is always supported. */
		assert(weston_output_get_supported_eotf_modes(output) & eotf_mode);
		weston_output_set_eotf_mode(output, eotf_mode);
		return 0;
	}

	for (i = 0; i < ARRAY_LENGTH(modes); i++)
		if (strcmp(str, modes[i].name) == 0)
			break;

	if (i == ARRAY_LENGTH(modes)) {
		weston_log("Error in config for output '%s': '%s' is not a valid EOTF mode. Try one of:",
			   output->name, str);
		for (i = 0; i < ARRAY_LENGTH(modes); i++)
			weston_log_continue(" %s", modes[i].name);
		weston_log_continue("\n");
		return -1;
	}
	eotf_mode = modes[i].eotf_mode;

	if ((weston_output_get_supported_eotf_modes(output) & eotf_mode) == 0) {
		weston_log("Error: output '%s' does not support EOTF mode %s.\n",
			   output->name, str);
#if !HAVE_LIBDISPLAY_INFO
		weston_log_continue(STAMP_SPACE "Weston was built without libdisplay-info, "
				    "so HDR capabilities cannot be detected.\n");
#endif
		free(str);
		return -1;
	}

	if (eotf_mode != WESTON_EOTF_MODE_SDR &&
	    !compositor->use_color_manager) {
		weston_log("Error: EOTF mode %s on output '%s' requires color-management=true in weston.ini\n",
			   str, output->name);
		free(str);
		return -1;
	}

	weston_output_set_eotf_mode(output, eotf_mode);

	free(str);
	return 0;
}

struct wet_color_characteristics_keys {
	const char *name;
	enum weston_color_characteristics_groups group;
	float minval;
	float maxval;
};

#define COLOR_CHARAC_NAME "color_characteristics"

static int
parse_color_characteristics(struct weston_color_characteristics *cc_out,
			      struct weston_config_section *section)
{
	static const struct wet_color_characteristics_keys keys[] = {
		{ "red_x",   WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "red_y",   WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "green_x", WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "green_y", WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "blue_x",  WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "blue_y",  WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES, 0.0f, 1.0f },
		{ "white_x", WESTON_COLOR_CHARACTERISTICS_GROUP_WHITE,     0.0f, 1.0f },
		{ "white_y", WESTON_COLOR_CHARACTERISTICS_GROUP_WHITE,     0.0f, 1.0f },
		{ "max_L",   WESTON_COLOR_CHARACTERISTICS_GROUP_MAXL,      0.0f, 1e5f },
		{ "min_L",   WESTON_COLOR_CHARACTERISTICS_GROUP_MINL,      0.0f, 1e5f },
		{ "maxFALL", WESTON_COLOR_CHARACTERISTICS_GROUP_MAXFALL,   0.0f, 1e5f },
	};
	static const char *msgpfx = "Config error in weston.ini [" COLOR_CHARAC_NAME "]";
	struct weston_color_characteristics cc = {};
	float *const keyvalp[ARRAY_LENGTH(keys)] = {
		/* These must be in the same order as keys[]. */
		&cc.primary[0].x, &cc.primary[0].y,
		&cc.primary[1].x, &cc.primary[1].y,
		&cc.primary[2].x, &cc.primary[2].y,
		&cc.white.x, &cc.white.y,
		&cc.max_luminance,
		&cc.min_luminance,
		&cc.maxFALL,
	};
	bool found[ARRAY_LENGTH(keys)] = {};
	uint32_t missing_group_mask = 0;
	unsigned i;
	char *section_name;
	int ret = 0;

	weston_config_section_get_string(section, "name",
					 &section_name, "<unnamed>");
	if (strchr(section_name, ':') != NULL) {
		ret = -1;
		weston_log("%s name=%s: reserved name. Do not use ':' character in the name.\n",
			   msgpfx, section_name);
	}

	/* Parse keys if they exist */
	for (i = 0; i < ARRAY_LENGTH(keys); i++) {
		double value;

		if (weston_config_section_get_double(section, keys[i].name,
						     &value, NAN) == 0) {
			float f = value;

			found[i] = true;

			/* Range check, NaN shall not pass. */
			if (f >= keys[i].minval && f <= keys[i].maxval) {
				/* Key found, parsed, and good value. */
				*keyvalp[i] = f;
				continue;
			}

			ret = -1;
			weston_log("%s name=%s: %s value %f is outside of the range %f - %f.\n",
				   msgpfx, section_name, keys[i].name, value,
				   keys[i].minval, keys[i].maxval);
			continue;
		}

		if (errno == EINVAL) {
			found[i] = true;
			ret = -1;
			weston_log("%s name=%s: failed to parse the value of key %s.\n",
				   msgpfx, section_name, keys[i].name);
		}
	}

	/* Collect set and unset groups */
	for (i = 0; i < ARRAY_LENGTH(keys); i++) {
		uint32_t group = keys[i].group;

		if (found[i])
			cc.group_mask |= group;
		else
			missing_group_mask |= group;
	}

	/* Ensure groups are given fully or not at all. */
	for (i = 0; i < ARRAY_LENGTH(keys); i++) {
		uint32_t group = keys[i].group;

		if ((cc.group_mask & group) && (missing_group_mask & group)) {
			ret = -1;
			weston_log("%s name=%s: group %d key %s is %s. "
				   "You must set either none or all keys of a group.\n",
				   msgpfx, section_name, ffs(group), keys[i].name,
				   found[i] ? "set" : "missing");
		}
	}

	free(section_name);

	if (ret == 0)
		*cc_out = cc;

	return ret;
}

WESTON_EXPORT_FOR_TESTS int
wet_output_set_color_characteristics(struct weston_output *output,
				     struct weston_config *wc,
				     struct weston_config_section *section)
{
	char *cc_name = NULL;
	struct weston_config_section *cc_section;
	struct weston_color_characteristics cc;

	weston_config_section_get_string(section, COLOR_CHARAC_NAME,
					 &cc_name, NULL);
	if (!cc_name)
		return 0;

	cc_section = weston_config_get_section(wc, COLOR_CHARAC_NAME,
					       "name", cc_name);
	if (!cc_section) {
		weston_log("Config error in weston.ini, output %s: "
			   "no [" COLOR_CHARAC_NAME "] section with 'name=%s' found.\n",
			   output->name, cc_name);
		goto out_error;
	}

	if (parse_color_characteristics(&cc, cc_section) < 0)
		goto out_error;

	weston_output_set_color_characteristics(output, &cc);
	free(cc_name);
	return 0;

out_error:
	free(cc_name);
	return -1;
}

static void
allow_content_protection(struct weston_output *output,
			struct weston_config_section *section)
{
	bool allow_hdcp = true;

	if (section)
		weston_config_section_get_bool(section, "allow_hdcp",
					       &allow_hdcp, true);

	weston_output_allow_protection(output, allow_hdcp);
}

static void
parse_simple_mode(struct weston_output *output,
		  struct weston_config_section *section, int *width,
		  int *height, struct wet_output_config *defaults,
		  struct wet_output_config *parsed_options)
{
	*width = defaults->width;
	*height = defaults->height;

	if (section) {
		char *mode;

		weston_config_section_get_string(section, "mode", &mode, NULL);
		if (!mode || sscanf(mode, "%dx%d", width, height) != 2) {
			weston_log("Invalid mode for output %s. Using defaults.\n",
				   output->name);
			*width = defaults->width;
			*height = defaults->height;
		}
		free(mode);
	}

	if (parsed_options->width)
		*width = parsed_options->width;

	if (parsed_options->height)
		*height = parsed_options->height;
}

static int
wet_configure_windowed_output_from_config(struct weston_output *output,
					  struct wet_output_config *defaults)
{
	const struct weston_windowed_output_api *api =
		weston_windowed_output_get_api(output->compositor);

	struct weston_config *wc = wet_get_config(output->compositor);
	struct weston_config_section *section = NULL;
	struct wet_compositor *compositor = to_wet_compositor(output->compositor);
	struct wet_output_config *parsed_options = compositor->parsed_options;
	int width;
	int height;

	assert(parsed_options);

	if (!api) {
		weston_log("Cannot use weston_windowed_output_api.\n");
		return -1;
	}

	section = weston_config_get_section(wc, "output", "name", output->name);

	parse_simple_mode(output, section, &width, &height, defaults,
			  parsed_options);

	allow_content_protection(output, section);

	wet_output_set_scale(output, section, defaults->scale, parsed_options->scale);
	if (wet_output_set_transform(output, section, defaults->transform,
				     parsed_options->transform) < 0) {
		return -1;
	}

	if (wet_output_set_color_profile(output, section, NULL) < 0)
		return -1;

	if (api->output_set_size(output, width, height) < 0) {
		weston_log("Cannot configure output \"%s\" using weston_windowed_output_api.\n",
			   output->name);
		return -1;
	}

	return 0;
}

static int
count_remaining_heads(struct weston_output *output, struct weston_head *to_go)
{
	struct weston_head *iter = NULL;
	int n = 0;

	while ((iter = weston_output_iterate_heads(output, iter))) {
		if (iter != to_go)
			n++;
	}

	return n;
}

static void
wet_head_tracker_destroy(struct wet_head_tracker *track)
{
	wl_list_remove(&track->head_destroy_listener.link);
	free(track);
}

static void
handle_head_destroy(struct wl_listener *listener, void *data)
{
	struct weston_head *head = data;
	struct weston_output *output;
	struct wet_head_tracker *track =
		container_of(listener, struct wet_head_tracker,
			     head_destroy_listener);

	wet_head_tracker_destroy(track);

	output = weston_head_get_output(head);

	/* On shutdown path, the output might be already gone. */
	if (!output)
		return;

	if (count_remaining_heads(output, head) > 0)
		return;

	weston_output_destroy(output);
}

static struct wet_head_tracker *
wet_head_tracker_from_head(struct weston_head *head)
{
	struct wl_listener *lis;

	lis = weston_head_get_destroy_listener(head, handle_head_destroy);
	if (!lis)
		return NULL;

	return container_of(lis, struct wet_head_tracker,
			    head_destroy_listener);
}

/* Listen for head destroy signal.
 *
 * If a head is destroyed and it was the last head on the output, we
 * destroy the associated output.
 *
 * Do not bother destroying the head trackers on shutdown, the backend will
 * destroy the heads which calls our handler to destroy the trackers.
 */
static void
wet_head_tracker_create(struct wet_compositor *compositor,
			struct weston_head *head)
{
	struct wet_head_tracker *track;

	track = zalloc(sizeof *track);
	if (!track)
		return;

	track->head_destroy_listener.notify = handle_head_destroy;
	weston_head_add_destroy_listener(head, &track->head_destroy_listener);
}

/* Place output exactly to the right of the most recently enabled output.
 *
 * Historically, we haven't given much thought to output placement,
 * simply adding outputs in a horizontal line as they're enabled. This
 * function simply sets an output's x coordinate to the right of the
 * most recently enabled output, and its y to zero.
 *
 * If you're adding new calls to this function, you're also not giving
 * much thought to output placement, so please consider carefully if
 * it's really doing what you want.
 *
 * You especially don't want to use this for any code that won't
 * immediately enable the passed output.
 */
static void
weston_output_lazy_align(struct weston_output *output)
{
	struct weston_compositor *c;
	struct weston_output *peer;
	int next_x = 0;

	/* Put this output to the right of the most recently enabled output */
	c = output->compositor;
	if (!wl_list_empty(&c->output_list)) {
		peer = container_of(c->output_list.prev,
				    struct weston_output, link);
		next_x = peer->x + peer->width;
	}
	output->x = next_x;
	output->y = 0;
}

static void
simple_head_enable(struct wet_compositor *wet, struct weston_head *head)
{
	struct weston_output *output;
	int ret = 0;

	output = weston_compositor_create_output(wet->compositor, head,
						 head->name);
	if (!output) {
		weston_log("Could not create an output for head \"%s\".\n",
			   weston_head_get_name(head));
		wet->init_failed = true;

		return;
	}

	weston_output_lazy_align(output);

	if (wet->simple_output_configure)
		ret = wet->simple_output_configure(output);
	if (ret < 0) {
		weston_log("Cannot configure output \"%s\".\n",
			   weston_head_get_name(head));
		weston_output_destroy(output);
		wet->init_failed = true;

		return;
	}

	if (weston_output_enable(output) < 0) {
		weston_log("Enabling output \"%s\" failed.\n",
			   weston_head_get_name(head));
		weston_output_destroy(output);
		wet->init_failed = true;

		return;
	}

	wet_head_tracker_create(wet, head);

	/* The weston_compositor will track and destroy the output on exit. */
}

static void
simple_head_disable(struct weston_head *head)
{
	struct weston_output *output;
	struct wet_head_tracker *track;

	track = wet_head_tracker_from_head(head);
	if (track)
		wet_head_tracker_destroy(track);

	output = weston_head_get_output(head);
	assert(output);
	weston_output_destroy(output);
}

static void
simple_heads_changed(struct wl_listener *listener, void *arg)
{
	struct weston_compositor *compositor = arg;
	struct wet_compositor *wet = to_wet_compositor(compositor);
	struct weston_head *head = NULL;
	bool connected;
	bool enabled;
	bool changed;
	bool non_desktop;

	while ((head = weston_compositor_iterate_heads(wet->compositor, head))) {
		connected = weston_head_is_connected(head);
		enabled = weston_head_is_enabled(head);
		changed = weston_head_is_device_changed(head);
		non_desktop = weston_head_is_non_desktop(head);

		if (connected && !enabled && !non_desktop) {
			simple_head_enable(wet, head);
		} else if (!connected && enabled) {
			simple_head_disable(head);
		} else if (enabled && changed) {
			weston_log("Detected a monitor change on head '%s', "
				   "not bothering to do anything about it.\n",
				   weston_head_get_name(head));
		}
		weston_head_reset_device_changed(head);
	}
}

static void
wet_set_simple_head_configurator(struct weston_compositor *compositor,
				 int (*fn)(struct weston_output *))
{
	struct wet_compositor *wet = to_wet_compositor(compositor);

	wet->simple_output_configure = fn;

	wet->heads_changed_listener.notify = simple_heads_changed;
	weston_compositor_add_heads_changed_listener(compositor,
						&wet->heads_changed_listener);
}

static void
configure_input_device_accel(struct weston_config_section *s,
		struct libinput_device *device)
{
	char *profile_string = NULL;
	int is_a_profile = 1;
	uint32_t profiles;
	enum libinput_config_accel_profile profile;
	double speed;

	if (weston_config_section_get_string(s, "accel-profile",
					     &profile_string, NULL) == 0) {
		if (strcmp(profile_string, "flat") == 0)
			profile = LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
		else if (strcmp(profile_string, "adaptive") == 0)
			profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
		else {
			weston_log("warning: no such accel-profile: %s\n",
				   profile_string);
			is_a_profile = 0;
		}

		profiles = libinput_device_config_accel_get_profiles(device);
		if (is_a_profile && (profile & profiles) != 0) {
			weston_log("          accel-profile=%s\n",
				   profile_string);
			libinput_device_config_accel_set_profile(device,
					profile);
		}
	}

	if (weston_config_section_get_double(s, "accel-speed",
					     &speed, 0) == 0 &&
	    speed >= -1. && speed <= 1.) {
		weston_log("          accel-speed=%.3f\n", speed);
		libinput_device_config_accel_set_speed(device, speed);
	}

	free(profile_string);
}

static void
configure_input_device_scroll(struct weston_config_section *s,
		struct libinput_device *device)
{
	bool natural;
	char *method_string = NULL;
	uint32_t methods;
	enum libinput_config_scroll_method method;
	char *button_string = NULL;
	int button;

	if (libinput_device_config_scroll_has_natural_scroll(device) &&
	    weston_config_section_get_bool(s, "natural-scroll",
					   &natural, false) == 0) {
		weston_log("          natural-scroll=%s\n",
			   natural ? "true" : "false");
		libinput_device_config_scroll_set_natural_scroll_enabled(
				device, natural);
	}

	if (weston_config_section_get_string(s, "scroll-method",
					     &method_string, NULL) != 0)
		goto done;
	if (strcmp(method_string, "two-finger") == 0)
		method = LIBINPUT_CONFIG_SCROLL_2FG;
	else if (strcmp(method_string, "edge") == 0)
		method = LIBINPUT_CONFIG_SCROLL_EDGE;
	else if (strcmp(method_string, "button") == 0)
		method = LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
	else if (strcmp(method_string, "none") == 0)
		method = LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
	else {
		weston_log("warning: no such scroll-method: %s\n",
			   method_string);
		goto done;
	}

	methods = libinput_device_config_scroll_get_methods(device);
	if (method != LIBINPUT_CONFIG_SCROLL_NO_SCROLL &&
	    (method & methods) == 0)
		goto done;

	weston_log("          scroll-method=%s\n", method_string);
	libinput_device_config_scroll_set_method(device, method);

	if (method == LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) {
		if (weston_config_section_get_string(s, "scroll-button",
						     &button_string,
						     NULL) != 0)
			goto done;

		button = libevdev_event_code_from_name(EV_KEY, button_string);
		if (button == -1) {
			weston_log("          Bad scroll-button: %s\n",
				   button_string);
			goto done;
		}

		weston_log("          scroll-button=%s\n", button_string);
		libinput_device_config_scroll_set_button(device, button);
	}

done:
	free(method_string);
	free(button_string);
}

static void
configure_input_device(struct weston_compositor *compositor,
		       struct libinput_device *device)
{
	struct weston_config_section *s;
	struct weston_config *config = wet_get_config(compositor);
	bool has_enable_tap = false;
	bool enable_tap;
	bool disable_while_typing;
	bool middle_emulation;
	bool tap_and_drag;
	bool tap_and_drag_lock;
	bool left_handed;
	unsigned int rotation;

	weston_log("libinput: configuring device \"%s\".\n",
		   libinput_device_get_name(device));

	s = weston_config_get_section(config,
				      "libinput", NULL, NULL);

	if (libinput_device_config_tap_get_finger_count(device) > 0) {
		if (weston_config_section_get_bool(s, "enable_tap",
						   &enable_tap, false) == 0) {
			weston_log("!!DEPRECATION WARNING!!: In weston.ini, "
				   "enable_tap is deprecated in favour of "
				   "enable-tap. Support for it may be removed "
				   "at any time!");
			has_enable_tap = true;
		}
		if (weston_config_section_get_bool(s, "enable-tap",
						   &enable_tap, false) == 0)
			has_enable_tap = true;
		if (has_enable_tap) {
			weston_log("          enable-tap=%s.\n",
				   enable_tap ? "true" : "false");
			libinput_device_config_tap_set_enabled(device,
							       enable_tap);
		}
		if (weston_config_section_get_bool(s, "tap-and-drag",
						   &tap_and_drag, false) == 0) {
			weston_log("          tap-and-drag=%s.\n",
				   tap_and_drag ? "true" : "false");
			libinput_device_config_tap_set_drag_enabled(device,
					tap_and_drag);
		}
		if (weston_config_section_get_bool(s, "tap-and-drag-lock",
					       &tap_and_drag_lock, false) == 0) {
			weston_log("          tap-and-drag-lock=%s.\n",
				   tap_and_drag_lock ? "true" : "false");
			libinput_device_config_tap_set_drag_lock_enabled(
					device, tap_and_drag_lock);
		}
	}

	if (libinput_device_config_dwt_is_available(device) &&
	    weston_config_section_get_bool(s, "disable-while-typing",
					   &disable_while_typing, false) == 0) {
		weston_log("          disable-while-typing=%s.\n",
			   disable_while_typing ? "true" : "false");
		libinput_device_config_dwt_set_enabled(device,
						       disable_while_typing);
	}

	if (libinput_device_config_middle_emulation_is_available(device) &&
	    weston_config_section_get_bool(s, "middle-button-emulation",
					   &middle_emulation, false) == 0) {
		weston_log("          middle-button-emulation=%s\n",
			   middle_emulation ? "true" : "false");
		libinput_device_config_middle_emulation_set_enabled(
				device, middle_emulation);
	}

	if (libinput_device_config_left_handed_is_available(device) &&
	    weston_config_section_get_bool(s, "left-handed",
				           &left_handed, false) == 0) {
		weston_log("          left-handed=%s\n",
			   left_handed ? "true" : "false");
		libinput_device_config_left_handed_set(device, left_handed);
	}

	if (libinput_device_config_rotation_is_available(device) &&
	    weston_config_section_get_uint(s, "rotation",
				           &rotation, false) == 0) {
		weston_log("          rotation=%u\n", rotation);
		libinput_device_config_rotation_set_angle(device, rotation);
	}

	if (libinput_device_config_accel_is_available(device))
		configure_input_device_accel(s, device);

	configure_input_device_scroll(s, device);
}

static int
drm_backend_output_configure(struct weston_output *output,
			     struct weston_config_section *section)
{
	struct wet_compositor *wet = to_wet_compositor(output->compositor);
	const struct weston_drm_output_api *api;
	enum weston_drm_backend_output_mode mode =
		WESTON_DRM_BACKEND_OUTPUT_PREFERRED;
	uint32_t transform = WL_OUTPUT_TRANSFORM_NORMAL;
	uint32_t max_bpc = 0;
	bool max_bpc_specified = false;
	char *s;
	char *modeline = NULL;
	char *gbm_format = NULL;
	char *content_type = NULL;
	char *seat = NULL;

	api = weston_drm_output_get_api(output->compositor);
	if (!api) {
		weston_log("Cannot use weston_drm_output_api.\n");
		return -1;
	}

	weston_config_section_get_string(section, "mode", &s, "preferred");
	if (weston_config_section_get_uint(section, "max-bpc", &max_bpc, 16) == 0)
		max_bpc_specified = true;

	if (strcmp(s, "off") == 0) {
		assert(0 && "off was supposed to be pruned");
		return -1;
	} else if (wet->drm_use_current_mode || strcmp(s, "current") == 0) {
		mode = WESTON_DRM_BACKEND_OUTPUT_CURRENT;
		/* If mode=current and no max-bpc was specfied on the .ini file,
		   use current max_bpc so full modeset is not done. */
		if (!max_bpc_specified)
			max_bpc = 0;
	} else if (strcmp(s, "preferred") != 0) {
		modeline = s;
		s = NULL;
	}
	free(s);

	if (api->set_mode(output, mode, modeline) < 0) {
		weston_log("Cannot configure an output using weston_drm_output_api.\n");
		free(modeline);
		return -1;
	}
	free(modeline);

	api->set_max_bpc(output, max_bpc);

	if (count_remaining_heads(output, NULL) == 1) {
		struct weston_head *head = weston_output_get_first_head(output);
		transform = weston_head_get_transform(head);
	}

	wet_output_set_scale(output, section, 1, 0);
	if (wet_output_set_transform(output, section, transform,
				     UINT32_MAX) < 0) {
		return -1;
	}

	if (wet_output_set_color_profile(output, section, NULL) < 0)
		return -1;

	weston_config_section_get_string(section,
					 "gbm-format", &gbm_format, NULL);

	api->set_gbm_format(output, gbm_format);
	free(gbm_format);

	weston_config_section_get_string(section,
					 "content-type", &content_type, NULL);
	if (api->set_content_type(output, content_type) < 0)
		return -1;
	free(content_type);

	weston_config_section_get_string(section, "seat", &seat, "");

	api->set_seat(output, seat);
	free(seat);

	allow_content_protection(output, section);

	if (wet_output_set_eotf_mode(output, section) < 0)
		return -1;

	if (wet_output_set_color_characteristics(output,
						 wet->config, section) < 0)
		return -1;

	return 0;
}

/* Find the output section to use for configuring the output with the
 * named head. If an output section with the given name contains
 * a "same-as" key, ignore all other settings in the output section and
 * instead find an output section named by the "same-as". Do this
 * recursively.
 */
static struct weston_config_section *
drm_config_find_controlling_output_section(struct weston_config *config,
					   const char *head_name)
{
	struct weston_config_section *section;
	char *same_as;
	int depth = 0;

	same_as = strdup(head_name);
	do {
		section = weston_config_get_section(config, "output",
						    "name", same_as);
		if (!section && depth > 0)
			weston_log("Configuration error: "
				   "output section referred to with "
				   "'same-as=%s' not found.\n", same_as);

		free(same_as);

		if (!section)
			return NULL;

		if (++depth > 10) {
			weston_log("Configuration error: "
				   "'same-as' nested too deep for output '%s'.\n",
				   head_name);
			return NULL;
		}

		weston_config_section_get_string(section, "same-as",
						 &same_as, NULL);
	} while (same_as);

	return section;
}

static struct wet_layoutput *
wet_compositor_create_layoutput(struct wet_compositor *compositor,
				const char *name,
				struct weston_config_section *section)
{
	struct wet_layoutput *lo;

	lo = zalloc(sizeof *lo);
	if (!lo)
		return NULL;

	lo->compositor = compositor;
	wl_list_insert(compositor->layoutput_list.prev, &lo->compositor_link);
	wl_list_init(&lo->output_list);
	lo->name = strdup(name);
	lo->section = section;

	return lo;
}

static void
wet_layoutput_destroy(struct wet_layoutput *lo)
{
	wl_list_remove(&lo->compositor_link);
	assert(wl_list_empty(&lo->output_list));
	free(lo->name);
	free(lo);
}

static void
wet_output_handle_destroy(struct wl_listener *listener, void *data)
{
	struct wet_output *output;

	output = wl_container_of(listener, output, output_destroy_listener);
	assert(output->output == data);

	output->output = NULL;
	wl_list_remove(&output->output_destroy_listener.link);
}

static struct wet_output *
wet_layoutput_create_output_with_head(struct wet_layoutput *lo,
				      const char *name,
				      struct weston_head *head)
{
	struct wet_output *output;

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	output->output =
		weston_compositor_create_output(lo->compositor->compositor,
						head, name);
	if (!output->output) {
		free(output);
		return NULL;
	}

	output->layoutput = lo;
	wl_list_insert(lo->output_list.prev, &output->link);
	output->output_destroy_listener.notify = wet_output_handle_destroy;
	weston_output_add_destroy_listener(output->output,
					   &output->output_destroy_listener);

	return output;
}

static struct wet_output *
wet_output_from_weston_output(struct weston_output *base)
{
	struct wl_listener *lis;

	lis = weston_output_get_destroy_listener(base,
						 wet_output_handle_destroy);
	if (!lis)
		return NULL;

	return container_of(lis, struct wet_output, output_destroy_listener);
}

static void
wet_output_destroy(struct wet_output *output)
{
	if (output->output) {
		/* output->output destruction may be deferred in some cases (see
		 * drm_output_destroy()), so we need to forcibly trigger the
		 * destruction callback now, or otherwise would later access
		 * data that we are about to free
		 */
		struct weston_output *save = output->output;
		wet_output_handle_destroy(&output->output_destroy_listener, save);
		weston_output_destroy(save);
	}

	wl_list_remove(&output->link);
	free(output);
}

static struct wet_layoutput *
wet_compositor_find_layoutput(struct wet_compositor *wet, const char *name)
{
	struct wet_layoutput *lo;

	wl_list_for_each(lo, &wet->layoutput_list, compositor_link)
		if (strcmp(lo->name, name) == 0)
			return lo;

	return NULL;
}

static void
wet_compositor_layoutput_add_head(struct wet_compositor *wet,
				  const char *output_name,
				  struct weston_config_section *section,
				  struct weston_head *head)
{
	struct wet_layoutput *lo;

	lo = wet_compositor_find_layoutput(wet, output_name);
	if (!lo) {
		lo = wet_compositor_create_layoutput(wet, output_name, section);
		if (!lo)
			return;
	}

	if (lo->add.n + 1 >= ARRAY_LENGTH(lo->add.heads))
		return;

	lo->add.heads[lo->add.n++] = head;
}

static void
wet_compositor_destroy_layout(struct wet_compositor *wet)
{
	struct wet_layoutput *lo, *lo_tmp;
	struct wet_output *output, *output_tmp;

	wl_list_for_each_safe(lo, lo_tmp,
			      &wet->layoutput_list, compositor_link) {
		wl_list_for_each_safe(output, output_tmp,
				      &lo->output_list, link) {
			wet_output_destroy(output);
		}
		wet_layoutput_destroy(lo);
	}
}

static void
drm_head_prepare_enable(struct wet_compositor *wet,
			struct weston_head *head)
{
	const char *name = weston_head_get_name(head);
	struct weston_config_section *section;
	char *output_name = NULL;
	char *mode = NULL;

	section = drm_config_find_controlling_output_section(wet->config, name);
	if (section) {
		/* skip outputs that are explicitly off, or non-desktop and not
		 * explicitly enabled. The backend turns them off automatically.
		 */
		weston_config_section_get_string(section, "mode", &mode, NULL);
		if (mode && strcmp(mode, "off") == 0) {
			free(mode);
			return;
		}
		if (!mode && weston_head_is_non_desktop(head))
			return;
		free(mode);

		weston_config_section_get_string(section, "name",
						 &output_name, NULL);
		assert(output_name);

		wet_compositor_layoutput_add_head(wet, output_name,
						  section, head);
		free(output_name);
	} else {
		wet_compositor_layoutput_add_head(wet, name, NULL, head);
	}
}

static bool
drm_head_should_force_enable(struct wet_compositor *wet,
			     struct weston_head *head)
{
	const char *name = weston_head_get_name(head);
	struct weston_config_section *section;
	bool force;

	section = drm_config_find_controlling_output_section(wet->config, name);
	if (!section)
		return false;

	weston_config_section_get_bool(section, "force-on", &force, false);
	return force;
}

static void
drm_try_attach(struct weston_output *output,
	       struct wet_head_array *add,
	       struct wet_head_array *failed)
{
	unsigned i;

	/* try to attach remaining heads, this probably succeeds */
	for (i = 1; i < add->n; i++) {
		if (!add->heads[i])
			continue;

		if (weston_output_attach_head(output, add->heads[i]) < 0) {
			assert(failed->n < ARRAY_LENGTH(failed->heads));

			failed->heads[failed->n++] = add->heads[i];
			add->heads[i] = NULL;
		}
	}
}

static int
drm_try_enable(struct weston_output *output,
	       struct wet_head_array *undo,
	       struct wet_head_array *failed)
{
	/* Try to enable, and detach heads one by one until it succeeds. */
	while (!output->enabled) {
		weston_output_lazy_align(output);

		if (weston_output_enable(output) == 0)
			return 0;

		/* the next head to drop */
		while (undo->n > 0 && undo->heads[--undo->n] == NULL)
			;

		/* No heads left to undo and failed to enable. */
		if (undo->heads[undo->n] == NULL)
			return -1;

		assert(failed->n < ARRAY_LENGTH(failed->heads));

		/* undo one head */
		weston_head_detach(undo->heads[undo->n]);
		failed->heads[failed->n++] = undo->heads[undo->n];
		undo->heads[undo->n] = NULL;
	}

	return 0;
}

static int
drm_try_attach_enable(struct weston_output *output, struct wet_layoutput *lo)
{
	struct wet_head_array failed = {};
	unsigned i;

	assert(!output->enabled);

	drm_try_attach(output, &lo->add, &failed);
	if (drm_backend_output_configure(output, lo->section) < 0)
		return -1;

	if (drm_try_enable(output, &lo->add, &failed) < 0)
		return -1;

	/* For all successfully attached/enabled heads */
	for (i = 0; i < lo->add.n; i++)
		if (lo->add.heads[i])
			wet_head_tracker_create(lo->compositor,
						lo->add.heads[i]);

	/* Push failed heads to the next round. */
	lo->add = failed;

	return 0;
}

static int
drm_process_layoutput(struct wet_compositor *wet, struct wet_layoutput *lo)
{
	struct wet_output *output, *tmp;
	char *name = NULL;
	int ret;

	/*
	 *   For each existing wet_output:
	 *     try attach
	 *   While heads left to enable:
	 *     Create output
	 *     try attach, try enable
	 */

	wl_list_for_each_safe(output, tmp, &lo->output_list, link) {
		struct wet_head_array failed = {};

		if (!output->output) {
			/* Clean up left-overs from destroyed heads. */
			wet_output_destroy(output);
			continue;
		}

		assert(output->output->enabled);

		drm_try_attach(output->output, &lo->add, &failed);
		lo->add = failed;
		if (lo->add.n == 0)
			return 0;
	}

	if (!weston_compositor_find_output_by_name(wet->compositor, lo->name))
		name = strdup(lo->name);

	while (lo->add.n > 0) {
		if (!wl_list_empty(&lo->output_list)) {
			weston_log("Error: independent-CRTC clone mode is not implemented.\n");
			return -1;
		}

		if (!name) {
			ret = asprintf(&name, "%s:%s", lo->name,
				       weston_head_get_name(lo->add.heads[0]));
			if (ret < 0)
				return -1;
		}
		output = wet_layoutput_create_output_with_head(lo, name,
							       lo->add.heads[0]);
		free(name);
		name = NULL;

		if (!output)
			return -1;

		if (drm_try_attach_enable(output->output, lo) < 0) {
			wet_output_destroy(output);
			return -1;
		}
	}

	return 0;
}

static int
drm_process_layoutputs(struct wet_compositor *wet)
{
	struct wet_layoutput *lo;
	int ret = 0;

	wl_list_for_each(lo, &wet->layoutput_list, compositor_link) {
		if (lo->add.n == 0)
			continue;

		if (drm_process_layoutput(wet, lo) < 0) {
			lo->add = (struct wet_head_array){};
			ret = -1;
		}
	}

	return ret;
}

static void
drm_head_disable(struct weston_head *head)
{
	struct weston_output *output_base;
	struct wet_output *output;
	struct wet_head_tracker *track;

	track = wet_head_tracker_from_head(head);
	if (track)
		wet_head_tracker_destroy(track);

	output_base = weston_head_get_output(head);
	assert(output_base);
	output = wet_output_from_weston_output(output_base);
	assert(output && output->output == output_base);

	weston_head_detach(head);
	if (count_remaining_heads(output->output, NULL) == 0)
		wet_output_destroy(output);
}

static void
drm_heads_changed(struct wl_listener *listener, void *arg)
{
	struct weston_compositor *compositor = arg;
	struct wet_compositor *wet = to_wet_compositor(compositor);
	struct weston_head *head = NULL;
	bool connected;
	bool enabled;
	bool changed;
	bool forced;

	/* We need to collect all cloned heads into outputs before enabling the
	 * output.
	 */
	while ((head = weston_compositor_iterate_heads(compositor, head))) {
		connected = weston_head_is_connected(head);
		enabled = weston_head_is_enabled(head);
		changed = weston_head_is_device_changed(head);
		forced = drm_head_should_force_enable(wet, head);

		if ((connected || forced) && !enabled) {
			drm_head_prepare_enable(wet, head);
		} else if (!(connected || forced) && enabled) {
			drm_head_disable(head);
		} else if (enabled && changed) {
			weston_log("Detected a monitor change on head '%s', "
				   "not bothering to do anything about it.\n",
				   weston_head_get_name(head));
		}
		weston_head_reset_device_changed(head);
	}

	if (drm_process_layoutputs(wet) < 0)
		wet->init_failed = true;
}

static int
drm_backend_remoted_output_configure(struct weston_output *output,
				     struct weston_config_section *section,
				     char *modeline,
				     const struct weston_remoting_api *api)
{
	char *gbm_format = NULL;
	char *seat = NULL;
	char *host = NULL;
	char *pipeline = NULL;
	int port, ret;

	ret = api->set_mode(output, modeline);
	if (ret < 0) {
		weston_log("Cannot configure an output \"%s\" using "
			   "weston_remoting_api. Invalid mode\n",
			   output->name);
		return -1;
	}

	wet_output_set_scale(output, section, 1, 0);
	if (wet_output_set_transform(output, section,
				     WL_OUTPUT_TRANSFORM_NORMAL,
				     UINT32_MAX) < 0) {
		return -1;
	};

	if (wet_output_set_color_profile(output, section, NULL) < 0)
		return -1;

	weston_config_section_get_string(section, "gbm-format", &gbm_format,
					 NULL);
	api->set_gbm_format(output, gbm_format);
	free(gbm_format);

	weston_config_section_get_string(section, "seat", &seat, "");

	api->set_seat(output, seat);
	free(seat);

	weston_config_section_get_string(section, "gst-pipeline", &pipeline,
					 NULL);
	if (pipeline) {
		api->set_gst_pipeline(output, pipeline);
		free(pipeline);
		return 0;
	}

	weston_config_section_get_string(section, "host", &host, NULL);
	weston_config_section_get_int(section, "port", &port, 0);
	if (!host || port <= 0 || 65533 < port) {
		weston_log("Cannot configure an output \"%s\". "
			   "Need to specify gst-pipeline or "
			   "host and port (1-65533).\n", output->name);
	}
	api->set_host(output, host);
	free(host);
	api->set_port(output, port);

	return 0;
}

static void
remoted_output_init(struct weston_compositor *c,
		    struct weston_config_section *section,
		    const struct weston_remoting_api *api)
{
	struct weston_output *output = NULL;
	char *output_name, *modeline = NULL;
	int ret;

	weston_config_section_get_string(section, "name", &output_name,
					 NULL);
	if (!output_name)
		return;

	weston_config_section_get_string(section, "mode", &modeline, "off");
	if (strcmp(modeline, "off") == 0)
		goto err;

	output = api->create_output(c, output_name);
	if (!output) {
		weston_log("Cannot create remoted output \"%s\".\n",
			   output_name);
		goto err;
	}

	ret = drm_backend_remoted_output_configure(output, section, modeline,
						   api);
	if (ret < 0) {
		weston_log("Cannot configure remoted output \"%s\".\n",
			   output_name);
		goto err;
	}

	if (weston_output_enable(output) < 0) {
		weston_log("Enabling remoted output \"%s\" failed.\n",
			   output_name);
		goto err;
	}

	free(modeline);
	free(output_name);
	weston_log("remoted output '%s' enabled\n", output->name);
	return;

err:
	free(modeline);
	free(output_name);
	if (output)
		weston_output_destroy(output);
}

static void
load_remoting(struct weston_compositor *c, struct weston_config *wc)
{
	const struct weston_remoting_api *api = NULL;
	int (*module_init)(struct weston_compositor *ec);
	struct weston_config_section *section = NULL;
	const char *section_name;

	/* read remote-output section in weston.ini */
	while (weston_config_next_section(wc, &section, &section_name)) {
		if (strcmp(section_name, "remote-output"))
			continue;

		if (!api) {
			char *module_name;
			struct weston_config_section *core_section =
				weston_config_get_section(wc, "core", NULL,
							  NULL);

			weston_config_section_get_string(core_section,
							 "remoting",
							 &module_name,
							 "remoting-plugin.so");
			module_init = weston_load_module(module_name,
							 "weston_module_init",
							 LIBWESTON_MODULEDIR);
			free(module_name);
			if (!module_init) {
				weston_log("Can't load remoting-plugin\n");
				return;
			}
			if (module_init(c) < 0) {
				weston_log("Remoting-plugin init failed\n");
				return;
			}

			api = weston_remoting_get_api(c);
			if (!api)
				return;
		}

		remoted_output_init(c, section, api);
	}
}

static int
drm_backend_pipewire_output_configure(struct weston_output *output,
				     struct weston_config_section *section,
				     char *modeline,
				     const struct weston_pipewire_api *api)
{
	char *seat = NULL;
	int ret;

	ret = api->set_mode(output, modeline);
	if (ret < 0) {
		weston_log("Cannot configure an output \"%s\" using "
			   "weston_pipewire_api. Invalid mode\n",
			   output->name);
		return -1;
	}

	wet_output_set_scale(output, section, 1, 0);
	if (wet_output_set_transform(output, section,
				     WL_OUTPUT_TRANSFORM_NORMAL,
				     UINT32_MAX) < 0) {
		return -1;
	}

	if (wet_output_set_color_profile(output, section, NULL) < 0)
		return -1;

	weston_config_section_get_string(section, "seat", &seat, "");

	api->set_seat(output, seat);
	free(seat);

	return 0;
}

static void
pipewire_output_init(struct weston_compositor *c,
		    struct weston_config_section *section,
		    const struct weston_pipewire_api *api)
{
	struct weston_output *output = NULL;
	char *output_name, *modeline = NULL;
	int ret;

	weston_config_section_get_string(section, "name", &output_name,
					 NULL);
	if (!output_name)
		return;

	weston_config_section_get_string(section, "mode", &modeline, "off");
	if (strcmp(modeline, "off") == 0)
		goto err;

	output = api->create_output(c, output_name);
	if (!output) {
		weston_log("Cannot create pipewire output \"%s\".\n",
			   output_name);
		goto err;
	}

	ret = drm_backend_pipewire_output_configure(output, section, modeline,
						   api);
	if (ret < 0) {
		weston_log("Cannot configure pipewire output \"%s\".\n",
			   output_name);
		goto err;
	}

	if (weston_output_enable(output) < 0) {
		weston_log("Enabling pipewire output \"%s\" failed.\n",
			   output_name);
		goto err;
	}

	free(modeline);
	free(output_name);
	weston_log("pipewire output '%s' enabled\n", output->name);
	return;

err:
	free(modeline);
	free(output_name);
	if (output)
		weston_output_destroy(output);
}

static void
load_pipewire(struct weston_compositor *c, struct weston_config *wc)
{
	const struct weston_pipewire_api *api = NULL;
	int (*module_init)(struct weston_compositor *ec);
	struct weston_config_section *section = NULL;
	const char *section_name;

	/* read pipewire-output section in weston.ini */
	while (weston_config_next_section(wc, &section, &section_name)) {
		if (strcmp(section_name, "pipewire-output"))
			continue;

		if (!api) {
			char *module_name;
			struct weston_config_section *core_section =
				weston_config_get_section(wc, "core", NULL,
							  NULL);

			weston_config_section_get_string(core_section,
							 "pipewire",
							 &module_name,
							 "pipewire-plugin.so");
			module_init = weston_load_module(module_name,
							 "weston_module_init",
							 LIBWESTON_MODULEDIR);
			free(module_name);
			if (!module_init) {
				weston_log("Can't load pipewire-plugin\n");
				return;
			}
			if (module_init(c) < 0) {
				weston_log("Pipewire-plugin init failed\n");
				return;
			}

			api = weston_pipewire_get_api(c);
			if (!api)
				return;
		}

		pipewire_output_init(c, section, api);
	}
}

static int
load_drm_backend(struct weston_compositor *c, int *argc, char **argv,
		 struct weston_config *wc, enum weston_renderer_type renderer)
{
	struct weston_drm_backend_config config = {{ 0, }};
	struct weston_config_section *section;
	struct wet_compositor *wet = to_wet_compositor(c);
	bool without_input = false;
	bool force_pixman = false;
	int ret = 0;

	wet->drm_use_current_mode = false;

	section = weston_config_get_section(wc, "core", NULL, NULL);

	weston_config_section_get_bool(section, "use-pixman", &force_pixman,
				       false);

	const struct weston_option options[] = {
		{ WESTON_OPTION_STRING, "seat", 0, &config.seat_id },
		{ WESTON_OPTION_STRING, "drm-device", 0, &config.specific_device },
		{ WESTON_OPTION_STRING, "additional-devices", 0, &config.additional_devices},
		{ WESTON_OPTION_BOOLEAN, "current-mode", 0, &wet->drm_use_current_mode },
		{ WESTON_OPTION_BOOLEAN, "use-pixman", 0, &force_pixman },
		{ WESTON_OPTION_BOOLEAN, "continue-without-input", false, &without_input }
	};

	parse_options(options, ARRAY_LENGTH(options), argc, argv);

	if (force_pixman && renderer != WESTON_RENDERER_AUTO) {
		weston_log("error: conflicting renderer specification\n");
		return -1;
	} else if (force_pixman) {
		config.renderer = WESTON_RENDERER_PIXMAN;
	} else {
		config.renderer = renderer;
	}

	section = weston_config_get_section(wc, "core", NULL, NULL);
	weston_config_section_get_string(section,
					 "gbm-format", &config.gbm_format,
					 NULL);
	weston_config_section_get_uint(section, "pageflip-timeout",
	                               &config.pageflip_timeout, 0);
	weston_config_section_get_bool(section, "pixman-shadow",
				       &config.use_pixman_shadow, true);
	if (without_input)
		c->require_input = !without_input;

	config.base.struct_version = WESTON_DRM_BACKEND_CONFIG_VERSION;
	config.base.struct_size = sizeof(struct weston_drm_backend_config);
	config.configure_device = configure_input_device;

	wet->heads_changed_listener.notify = drm_heads_changed;
	weston_compositor_add_heads_changed_listener(c,
						&wet->heads_changed_listener);

	ret = weston_compositor_load_backend(c, WESTON_BACKEND_DRM,
					     &config.base);

	/* remoting */
	load_remoting(c, wc);

	/* pipewire */
	load_pipewire(c, wc);

	free(config.gbm_format);
	free(config.seat_id);
	free(config.specific_device);

	return ret;
}

static int
headless_backend_output_configure(struct weston_output *output)
{
	struct wet_output_config defaults = {
		.width = 1024,
		.height = 640,
		.scale = 1,
		.transform = WL_OUTPUT_TRANSFORM_NORMAL
	};
	struct weston_config *wc = wet_get_config(output->compositor);
	struct weston_config_section *section;

	section = weston_config_get_section(wc, "output", "name", output->name);
	if (wet_output_set_eotf_mode(output, section) < 0)
		return -1;

	if (wet_output_set_color_characteristics(output, wc, section) < 0)
		return -1;

	return wet_configure_windowed_output_from_config(output, &defaults);
}

static int
load_headless_backend(struct weston_compositor *c,
		      int *argc, char **argv, struct weston_config *wc,
		      enum weston_renderer_type renderer)
{
	const struct weston_windowed_output_api *api;
	struct weston_headless_backend_config config = {{ 0, }};
	struct weston_config_section *section;
	bool force_pixman;
	bool force_gl;
	bool no_outputs = false;
	int ret = 0;
	char *transform = NULL;

	struct wet_output_config *parsed_options = wet_init_parsed_options(c);
	if (!parsed_options)
		return -1;

	section = weston_config_get_section(wc, "core", NULL, NULL);
	weston_config_section_get_bool(section, "use-pixman", &force_pixman,
				       false);
	weston_config_section_get_bool(section, "use-gl", &force_gl,
				       false);
	weston_config_section_get_bool(section, "output-decorations", &config.decorate,
				       false);

	const struct weston_option options[] = {
		{ WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
		{ WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
		{ WESTON_OPTION_INTEGER, "scale", 0, &parsed_options->scale },
		{ WESTON_OPTION_BOOLEAN, "use-pixman", 0, &force_pixman },
		{ WESTON_OPTION_BOOLEAN, "use-gl", 0, &force_gl },
		{ WESTON_OPTION_STRING, "transform", 0, &transform },
		{ WESTON_OPTION_BOOLEAN, "no-outputs", 0, &no_outputs },
	};

	parse_options(options, ARRAY_LENGTH(options), argc, argv);

	if ((force_pixman && force_gl) ||
	    (renderer != WESTON_RENDERER_AUTO && (force_pixman || force_gl))) {
		weston_log("Conflicting renderer specifications\n");
		return -1;
	} else if (force_pixman) {
		config.renderer = WESTON_RENDERER_PIXMAN;
	} else if (force_gl) {
		config.renderer = WESTON_RENDERER_GL;
	} else {
		config.renderer = renderer;
	}

	if (transform) {
		if (weston_parse_transform(transform, &parsed_options->transform) < 0) {
			weston_log("Invalid transform \"%s\"\n", transform);
			return -1;
		}
		free(transform);
	}

	config.base.struct_version = WESTON_HEADLESS_BACKEND_CONFIG_VERSION;
	config.base.struct_size = sizeof(struct weston_headless_backend_config);

	wet_set_simple_head_configurator(c, headless_backend_output_configure);

	/* load the actual wayland backend and configure it */
	ret = weston_compositor_load_backend(c, WESTON_BACKEND_HEADLESS,
					     &config.base);

	if (ret < 0)
		return ret;

	if (!no_outputs) {
		api = weston_windowed_output_get_api(c);

		if (!api) {
			weston_log("Cannot use weston_windowed_output_api.\n");
			return -1;
		}

		if (api->create_head(c->backend, "headless") < 0)
			return -1;
	}

	return 0;
}

static int
pipewire_backend_output_configure(struct weston_output *output)
{
	struct wet_output_config defaults = {
		.width = 640,
		.height = 480,
	};
	struct wet_compositor *compositor = to_wet_compositor(output->compositor);
	struct wet_output_config *parsed_options = compositor->parsed_options;
	const struct weston_pipewire_output_api *api = weston_pipewire_output_get_api(output->compositor);
	struct weston_config *wc = wet_get_config(output->compositor);
	struct weston_config_section *section;
	char *gbm_format = NULL;
	int width;
	int height;

	assert(parsed_options);

	if (!api) {
		weston_log("Cannot use weston_pipewire_output_api.\n");
		return -1;
	}

	section = weston_config_get_section(wc, "output", "name", output->name);

	parse_simple_mode(output, section, &width, &height, &defaults,
			  parsed_options);

	if (section)
		weston_config_section_get_string(section, "gbm-format",
						 &gbm_format, NULL);

	weston_output_set_scale(output, 1);
	weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

	api->set_gbm_format(output, gbm_format);
	free(gbm_format);

	if (api->output_set_size(output, width, height) < 0) {
		weston_log("Cannot configure output \"%s\" using weston_pipewire_output_api.\n",
			   output->name);
		return -1;
	}
	weston_log("pipewire_backend_output_configure.. Done\n");

	return 0;
}

static void
weston_pipewire_backend_config_init(struct weston_pipewire_backend_config *config)
{
	config->base.struct_version = WESTON_PIPEWIRE_BACKEND_CONFIG_VERSION;
	config->base.struct_size = sizeof(struct weston_pipewire_backend_config);
}

static int
load_pipewire_backend(struct weston_compositor *c,
		      int *argc, char *argv[], struct weston_config *wc,
		      enum weston_renderer_type renderer)
{
	struct weston_pipewire_backend_config config = {{ 0, }};
	struct weston_config_section *section;
	struct wet_output_config *parsed_options = wet_init_parsed_options(c);

	if (!parsed_options)
		return -1;

	weston_pipewire_backend_config_init(&config);

	const struct weston_option pipewire_options[] = {
		{ WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
		{ WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
	};

	parse_options(pipewire_options, ARRAY_LENGTH(pipewire_options), argc, argv);

	config.renderer = renderer;

	wet_set_simple_head_configurator(c, pipewire_backend_output_configure);

	section = weston_config_get_section(wc, "core", NULL, NULL);
	weston_config_section_get_string(section, "gbm-format",
					 &config.gbm_format, NULL);

	section = weston_config_get_section(wc, "pipewire", NULL, NULL);
	weston_config_section_get_int(section, "num-outputs",
				      &config.num_outputs, 1);

	return weston_compositor_load_backend(c, WESTON_BACKEND_PIPEWIRE,
					      &config.base);
}

static void
weston_rdp_backend_config_init(struct weston_rdp_backend_config *config)
{
	config->base.struct_version = WESTON_RDP_BACKEND_CONFIG_VERSION;
	config->base.struct_size = sizeof(struct weston_rdp_backend_config);

	config->renderer = WESTON_RENDERER_AUTO;
	config->bind_address = NULL;
	config->port = 3389;
	config->rdp_key = NULL;
	config->server_cert = NULL;
	config->server_key = NULL;
	config->env_socket = 0;
	config->external_listener_fd = -1;
	config->no_clients_resize = 0;
	config->force_no_compression = 0;
	config->remotefx_codec = true;
	config->refresh_rate = RDP_DEFAULT_FREQ;
}

static void
rdp_handle_layout(struct weston_compositor *ec)
{
	struct wet_compositor *wc = to_wet_compositor(ec);
	struct wet_output_config *parsed_options = wc->parsed_options;
	const struct weston_rdp_output_api *api = weston_rdp_output_get_api(ec);
	struct weston_rdp_monitor config;
	struct weston_head *head = NULL;
	int width;
	int height;
	int scale = 1;

	while ((head = weston_compositor_iterate_heads(ec, head))) {
		struct weston_coord_global pos;
		struct weston_output *output = head->output;
		struct weston_mode new_mode = {};

		assert(output);

		api->head_get_monitor(head, &config);

		width = config.width;
		height = config.height;
		scale = config.desktop_scale / 100;

		/* If these are invalid, the backend is expecting
		 * us to provide defaults.
		 */
		width = width ? width : parsed_options->width;
		height = height ? height : parsed_options->height;
		scale = scale ? scale : parsed_options->scale;

		/* Fallback to 640 x 480 if we have nothing to use */
		width = width ? width : 640;
		height = height ? height : 480;
		scale = scale ? scale : 1;

		new_mode.width = width;
		new_mode.height = height;
		api->output_set_mode(output, &new_mode);

		weston_output_set_scale(output, scale);
		weston_output_set_transform(output,
					    WL_OUTPUT_TRANSFORM_NORMAL);
		pos.c = weston_coord(config.x, config.y);
		weston_output_move(output, pos);
	}
}

static void
rdp_heads_changed(struct wl_listener *listener, void *arg)
{
	struct weston_compositor *compositor = arg;
	struct wet_compositor *wet = to_wet_compositor(compositor);
	struct weston_head *head = NULL;

	while ((head = weston_compositor_iterate_heads(compositor, head))) {
		if (head->output)
			continue;

		struct weston_output *out;

		out = weston_compositor_create_output(compositor,
						      head, head->name);

		wet_head_tracker_create(wet, head);
		weston_output_attach_head(out, head);
	}

	rdp_handle_layout(compositor);

	while ((head = weston_compositor_iterate_heads(compositor, head))) {
		if (!head->output->enabled)
			weston_output_enable(head->output);

		weston_head_reset_device_changed(head);
	}
}

static int
load_rdp_backend(struct weston_compositor *c,
		int *argc, char *argv[], struct weston_config *wc,
		enum weston_renderer_type renderer)
{
	struct weston_rdp_backend_config config  = {{ 0, }};
	struct weston_config_section *section;
	int ret = 0;
	bool no_remotefx_codec = false;
	struct wet_output_config *parsed_options = wet_init_parsed_options(c);
	struct wet_compositor *wet = to_wet_compositor(c);

	if (!parsed_options)
		return -1;

	weston_rdp_backend_config_init(&config);

	const struct weston_option rdp_options[] = {
		{ WESTON_OPTION_BOOLEAN, "env-socket", 0, &config.env_socket },
		{ WESTON_OPTION_INTEGER, "external-listener-fd", 0, &config.external_listener_fd },
		{ WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
		{ WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
		{ WESTON_OPTION_STRING,  "address", 0, &config.bind_address },
		{ WESTON_OPTION_INTEGER, "port", 0, &config.port },
		{ WESTON_OPTION_BOOLEAN, "no-clients-resize", 0, &config.no_clients_resize },
		{ WESTON_OPTION_STRING,  "rdp4-key", 0, &config.rdp_key },
		{ WESTON_OPTION_STRING,  "rdp-tls-cert", 0, &config.server_cert },
		{ WESTON_OPTION_STRING,  "rdp-tls-key", 0, &config.server_key },
		{ WESTON_OPTION_INTEGER, "scale", 0, &parsed_options->scale },
		{ WESTON_OPTION_BOOLEAN, "force-no-compression", 0, &config.force_no_compression },
		{ WESTON_OPTION_BOOLEAN, "no-remotefx-codec", 0, &no_remotefx_codec },
	};

	parse_options(rdp_options, ARRAY_LENGTH(rdp_options), argc, argv);
	config.remotefx_codec = !no_remotefx_codec;
	config.renderer = renderer;

	section = weston_config_get_section(wc, "rdp", NULL, NULL);
	weston_config_section_get_int(section, "refresh-rate",
				      &config.refresh_rate,
				      RDP_DEFAULT_FREQ);

	wet->heads_changed_listener.notify = rdp_heads_changed;
	weston_compositor_add_heads_changed_listener(c,
						     &wet->heads_changed_listener);

	ret = weston_compositor_load_backend(c, WESTON_BACKEND_RDP,
					     &config.base);

	free(config.bind_address);
	free(config.rdp_key);
	free(config.server_cert);
	free(config.server_key);

	return ret;
}

static int
vnc_backend_output_configure(struct weston_output *output)
{
	struct wet_output_config defaults = {
		.width = 640,
		.height = 480,
	};
	struct wet_compositor *compositor = to_wet_compositor(output->compositor);
	struct wet_output_config *parsed_options = compositor->parsed_options;
	const struct weston_vnc_output_api *api = weston_vnc_output_get_api(output->compositor);
	struct weston_config *wc = wet_get_config(output->compositor);
	struct weston_config_section *section;
	int width;
	int height;

	assert(parsed_options);

	if (!api) {
		weston_log("Cannot use weston_vnc_output_api.\n");
		return -1;
	}

	section = weston_config_get_section(wc, "output", "name", output->name);

	parse_simple_mode(output, section, &width, &height, &defaults,
			  compositor->parsed_options);

	weston_output_set_scale(output, 1);
	weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

	if (api->output_set_size(output, width, height) < 0) {
		weston_log("Cannot configure output \"%s\" using weston_vnc_output_api.\n",
			   output->name);
		return -1;
	}
	weston_log("vnc_backend_output_configure.. Done\n");

	return 0;
}


static void
weston_vnc_backend_config_init(struct weston_vnc_backend_config *config)
{
	config->base.struct_version = WESTON_VNC_BACKEND_CONFIG_VERSION;
	config->base.struct_size = sizeof(struct weston_vnc_backend_config);

	config->renderer = WESTON_RENDERER_AUTO;
	config->bind_address = NULL;
	config->port = 5900;
	config->refresh_rate = VNC_DEFAULT_FREQ;
}

static int
load_vnc_backend(struct weston_compositor *c,
		int *argc, char *argv[], struct weston_config *wc,
		enum weston_renderer_type renderer)
{
	struct weston_vnc_backend_config config  = {{ 0, }};
	struct weston_config_section *section;
	int ret = 0;

	struct wet_output_config *parsed_options = wet_init_parsed_options(c);
	if (!parsed_options)
		return -1;

	weston_vnc_backend_config_init(&config);

	const struct weston_option vnc_options[] = {
		{ WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
		{ WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
		{ WESTON_OPTION_STRING,  "address", 0, &config.bind_address },
		{ WESTON_OPTION_INTEGER, "port", 0, &config.port },
		{ WESTON_OPTION_STRING,  "vnc-tls-cert", 0, &config.server_cert },
		{ WESTON_OPTION_STRING,  "vnc-tls-key", 0, &config.server_key },
	};

	parse_options(vnc_options, ARRAY_LENGTH(vnc_options), argc, argv);

	config.renderer = renderer;

	wet_set_simple_head_configurator(c, vnc_backend_output_configure);
	section = weston_config_get_section(wc, "vnc", NULL, NULL);
	weston_config_section_get_int(section, "refresh-rate",
				      &config.refresh_rate,
				      VNC_DEFAULT_FREQ);

	ret = weston_compositor_load_backend(c, WESTON_BACKEND_VNC,
					     &config.base);

	free(config.bind_address);
	free(config.server_cert);
	free(config.server_key);

	return ret;
}

static int
x11_backend_output_configure(struct weston_output *output)
{
	struct wet_output_config defaults = {
		.width = 1024,
		.height = 600,
		.scale = 1,
		.transform = WL_OUTPUT_TRANSFORM_NORMAL
	};

	return wet_configure_windowed_output_from_config(output, &defaults);
}

static int
load_x11_backend(struct weston_compositor *c,
		 int *argc, char **argv, struct weston_config *wc,
		 enum weston_renderer_type renderer)
{
	char *default_output;
	const struct weston_windowed_output_api *api;
	struct weston_x11_backend_config config = {{ 0, }};
	struct weston_config_section *section;
	bool force_pixman;
	int ret = 0;
	int option_count = 1;
	int output_count = 0;
	char const *section_name;
	int i;

	struct wet_output_config *parsed_options = wet_init_parsed_options(c);
	if (!parsed_options)
		return -1;

	section = weston_config_get_section(wc, "core", NULL, NULL);
	weston_config_section_get_bool(section, "use-pixman", &force_pixman,
				       false);

	const struct weston_option options[] = {
	       { WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
	       { WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
	       { WESTON_OPTION_INTEGER, "scale", 0, &parsed_options->scale },
	       { WESTON_OPTION_BOOLEAN, "fullscreen", 'f', &config.fullscreen },
	       { WESTON_OPTION_INTEGER, "output-count", 0, &option_count },
	       { WESTON_OPTION_BOOLEAN, "no-input", 0, &config.no_input },
	       { WESTON_OPTION_BOOLEAN, "use-pixman", 0, &force_pixman },
	};

	parse_options(options, ARRAY_LENGTH(options), argc, argv);

	config.base.struct_version = WESTON_X11_BACKEND_CONFIG_VERSION;
	config.base.struct_size = sizeof(struct weston_x11_backend_config);

	if (force_pixman && renderer != WESTON_RENDERER_AUTO) {
		weston_log("error: conflicting renderer specification\n");
		return -1;
	} else if (force_pixman) {
		config.renderer = WESTON_RENDERER_PIXMAN;
	} else {
		config.renderer = renderer;
	}

	wet_set_simple_head_configurator(c, x11_backend_output_configure);

	/* load the actual backend and configure it */
	ret = weston_compositor_load_backend(c, WESTON_BACKEND_X11,
					     &config.base);

	if (ret < 0)
		return ret;

	api = weston_windowed_output_get_api(c);

	if (!api) {
		weston_log("Cannot use weston_windowed_output_api.\n");
		return -1;
	}

	section = NULL;
	while (weston_config_next_section(wc, &section, &section_name)) {
		char *output_name;

		if (output_count >= option_count)
			break;

		if (strcmp(section_name, "output") != 0) {
			continue;
		}

		weston_config_section_get_string(section, "name", &output_name, NULL);
		if (output_name == NULL || output_name[0] != 'X') {
			free(output_name);
			continue;
		}

		if (api->create_head(c->backend, output_name) < 0) {
			free(output_name);
			return -1;
		}
		free(output_name);

		output_count++;
	}

	default_output = NULL;

	for (i = output_count; i < option_count; i++) {
		if (asprintf(&default_output, "screen%d", i) < 0) {
			return -1;
		}

		if (api->create_head(c->backend, default_output) < 0) {
			free(default_output);
			return -1;
		}
		free(default_output);
	}

	return 0;
}

static int
wayland_backend_output_configure(struct weston_output *output)
{
	struct wet_output_config defaults = {
		.width = 1024,
		.height = 640,
		.scale = 1,
		.transform = WL_OUTPUT_TRANSFORM_NORMAL
	};

	return wet_configure_windowed_output_from_config(output, &defaults);
}

static int
load_wayland_backend(struct weston_compositor *c,
		     int *argc, char **argv, struct weston_config *wc,
		     enum weston_renderer_type renderer)
{
	struct weston_wayland_backend_config config = {{ 0, }};
	struct weston_config_section *section;
	const struct weston_windowed_output_api *api;
	const char *section_name;
	char *output_name = NULL;
	bool force_pixman = false;
	int count = 1;
	int ret = 0;
	int i;

	struct wet_output_config *parsed_options = wet_init_parsed_options(c);
	if (!parsed_options)
		return -1;

	config.cursor_size = 32;
	config.cursor_theme = NULL;
	config.display_name = NULL;

	section = weston_config_get_section(wc, "core", NULL, NULL);
	weston_config_section_get_bool(section, "use-pixman", &force_pixman,
				       false);

	const struct weston_option wayland_options[] = {
		{ WESTON_OPTION_INTEGER, "width", 0, &parsed_options->width },
		{ WESTON_OPTION_INTEGER, "height", 0, &parsed_options->height },
		{ WESTON_OPTION_INTEGER, "scale", 0, &parsed_options->scale },
		{ WESTON_OPTION_STRING, "display", 0, &config.display_name },
		{ WESTON_OPTION_BOOLEAN, "use-pixman", 0, &force_pixman },
		{ WESTON_OPTION_INTEGER, "output-count", 0, &count },
		{ WESTON_OPTION_BOOLEAN, "fullscreen", 0, &config.fullscreen },
		{ WESTON_OPTION_BOOLEAN, "sprawl", 0, &config.sprawl },
	};

	parse_options(wayland_options, ARRAY_LENGTH(wayland_options), argc, argv);

	section = weston_config_get_section(wc, "shell", NULL, NULL);
	weston_config_section_get_string(section, "cursor-theme",
					 &config.cursor_theme, NULL);
	weston_config_section_get_int(section, "cursor-size",
				      &config.cursor_size, 32);

	config.base.struct_size = sizeof(struct weston_wayland_backend_config);
	config.base.struct_version = WESTON_WAYLAND_BACKEND_CONFIG_VERSION;

	if (force_pixman && renderer != WESTON_RENDERER_AUTO) {
		weston_log("error: conflicting renderer specification\n");
		return -1;
	} else if (force_pixman) {
		config.renderer = WESTON_RENDERER_PIXMAN;
	} else {
		config.renderer = renderer;
	}

	/* load the actual wayland backend and configure it */
	ret = weston_compositor_load_backend(c, WESTON_BACKEND_WAYLAND,
					     &config.base);

	free(config.cursor_theme);
	free(config.display_name);

	if (ret < 0)
		return ret;

	api = weston_windowed_output_get_api(c);

	if (api == NULL) {
		/* We will just assume if load_backend() finished cleanly and
		 * windowed_output_api is not present that wayland backend is
		 * started with --sprawl or runs on fullscreen-shell.
		 * In this case, all values are hardcoded, so nothing can be
		 * configured; simply create and enable an output. */
		wet_set_simple_head_configurator(c, NULL);

		return 0;
	}

	wet_set_simple_head_configurator(c, wayland_backend_output_configure);

	section = NULL;
	while (weston_config_next_section(wc, &section, &section_name)) {
		if (count == 0)
			break;

		if (strcmp(section_name, "output") != 0) {
			continue;
		}

		weston_config_section_get_string(section, "name", &output_name, NULL);

		if (output_name == NULL)
			continue;

		if (output_name[0] != 'W' || output_name[1] != 'L') {
			free(output_name);
			continue;
		}

		if (api->create_head(c->backend, output_name) < 0) {
			free(output_name);
			return -1;
		}
		free(output_name);

		--count;
	}

	for (i = 0; i < count; i++) {
		if (asprintf(&output_name, "wayland%d", i) < 0)
			return -1;

		if (api->create_head(c->backend, output_name) < 0) {
			free(output_name);
			return -1;
		}
		free(output_name);
	}

	return 0;
}


static int
load_backend(struct weston_compositor *compositor, const char *name,
	     int *argc, char **argv, struct weston_config *config,
	     const char *renderer_name)
{
	enum weston_compositor_backend backend;
	enum weston_renderer_type renderer;

	if (!get_backend_from_string(name, &backend)) {
		weston_log("Error: unknown backend \"%s\"\n", name);
		return -1;
	}

	if (!get_renderer_from_string(renderer_name, &renderer)) {
		weston_log("Error: unknown renderer \"%s\"\n", renderer_name);
		return -1;
	}

	switch (backend) {
	case WESTON_BACKEND_DRM:
		return load_drm_backend(compositor, argc, argv, config,
					renderer);
	case WESTON_BACKEND_HEADLESS:
		return load_headless_backend(compositor, argc, argv, config,
					     renderer);
	case WESTON_BACKEND_PIPEWIRE:
		return load_pipewire_backend(compositor, argc, argv, config,
					     renderer);
	case WESTON_BACKEND_RDP:
		return load_rdp_backend(compositor, argc, argv, config,
					renderer);
	case WESTON_BACKEND_VNC:
		return load_vnc_backend(compositor, argc, argv, config,
					renderer);
	case WESTON_BACKEND_WAYLAND:
		return load_wayland_backend(compositor, argc, argv, config,
					    renderer);
	case WESTON_BACKEND_X11:
		return load_x11_backend(compositor, argc, argv, config,
					renderer);
	default:
		unreachable("unknown backend type in load_backend()");
	}
}

static char *
copy_command_line(int argc, char * const argv[])
{
	FILE *fp;
	char *str = NULL;
	size_t size = 0;
	int i;

	fp = open_memstream(&str, &size);
	if (!fp)
		return NULL;

	fprintf(fp, "%s", argv[0]);
	for (i = 1; i < argc; i++)
		fprintf(fp, " %s", argv[i]);
	fclose(fp);

	return str;
}

#if !defined(BUILD_XWAYLAND)
int
wet_load_xwayland(struct weston_compositor *comp)
{
	return -1;
}
#endif

static int
execute_autolaunch(struct wet_compositor *wet, struct weston_config *config)
{
	int ret = -1;
	pid_t tmp_pid = -1;
	char *autolaunch_path = NULL;
	struct weston_config_section *section = NULL;

	section = weston_config_get_section(config, "autolaunch", NULL, NULL);
	weston_config_section_get_string(section, "path", &autolaunch_path, "");
	weston_config_section_get_bool(section, "watch", &wet->autolaunch_watch, false);

	if (!strlen(autolaunch_path))
		goto out_ok;

	if (access(autolaunch_path, X_OK) != 0) {
		weston_log("Specified autolaunch path (%s) is not executable\n", autolaunch_path);
		goto out;
	}

	tmp_pid = fork();
	if (tmp_pid == -1) {
		weston_log("Failed to fork autolaunch process: %s\n", strerror(errno));
		goto out;
	} else if (tmp_pid == 0) {
		cleanup_for_child_process();
		execl(autolaunch_path, autolaunch_path, NULL);
		/* execl shouldn't return */
		fprintf(stderr, "Failed to execute autolaunch: %s\n", strerror(errno));
		_exit(1);
	}

out_ok:
	ret = 0;
out:
	wet->autolaunch_pid = tmp_pid;
	free(autolaunch_path);
	return ret;
}

static void
weston_log_setup_scopes(struct weston_log_context *log_ctx,
			struct weston_log_subscriber *subscriber,
			const char *names)
{
	assert(log_ctx);
	assert(subscriber);

	char *tokenize = strdup(names);
	char *token = strtok(tokenize, ",");
	while (token) {
		weston_log_subscribe(log_ctx, subscriber, token);
		token = strtok(NULL, ",");
	}
	free(tokenize);
}

static void
flight_rec_key_binding_handler(struct weston_keyboard *keyboard,
			       const struct timespec *time, uint32_t key,
			       void *data)
{
	struct weston_log_subscriber *flight_rec = data;
	weston_log_subscriber_display_flight_rec(flight_rec);
}

static void
weston_log_subscribe_to_scopes(struct weston_log_context *log_ctx,
			       struct weston_log_subscriber *logger,
			       struct weston_log_subscriber *flight_rec,
			       const char *log_scopes,
			       const char *flight_rec_scopes)
{
	if (logger && log_scopes)
		weston_log_setup_scopes(log_ctx, logger, log_scopes);
	else
		weston_log_subscribe(log_ctx, logger, "log");

	if (flight_rec && flight_rec_scopes)
		weston_log_setup_scopes(log_ctx, flight_rec, flight_rec_scopes);
}

static void
screenshot_allow_all(struct wl_listener *l,
		     struct weston_output_capture_attempt *att)
{
	/*
	 * The effect of --debug option: indiscriminately allow everyone to
	 * take screenshots of any output.
	 */
	att->authorized = true;
}

static void
sigint_helper(int sig)
{
	raise(SIGUSR2);
}

WL_EXPORT int
wet_main(int argc, char *argv[], const struct weston_testsuite_data *test_data)
{
	int ret = EXIT_FAILURE;
	char *cmdline;
	struct wl_display *display;
	struct wl_event_source *signals[3];
	struct wl_event_loop *loop;
	int i, fd;
	char *backend = NULL;
	char *renderer = NULL;
	char *shell = NULL;
	bool xwayland = false;
	char *modules = NULL;
	char *option_modules = NULL;
	char *log = NULL;
	char *log_scopes = NULL;
	char *flight_rec_scopes = NULL;
	char *server_socket = NULL;
	int32_t idle_time = -1;
	int32_t help = 0;
	char *socket_name = NULL;
	int32_t version = 0;
	int32_t noconfig = 0;
	int32_t debug_protocol = 0;
	bool numlock_on;
	char *config_file = NULL;
	struct weston_config *config = NULL;
	struct weston_config_section *section;
	struct wl_client *primary_client;
	struct wl_listener primary_client_destroyed;
	struct weston_seat *seat;
	struct wet_compositor wet = { 0 };
	struct weston_log_context *log_ctx = NULL;
	struct weston_log_subscriber *logger = NULL;
	struct weston_log_subscriber *flight_rec = NULL;
	sigset_t mask;
	struct sigaction action;

	bool wait_for_debugger = false;
	struct wl_protocol_logger *protologger = NULL;

	const struct weston_option core_options[] = {
		{ WESTON_OPTION_STRING, "backend", 'B', &backend },
		{ WESTON_OPTION_STRING, "renderer", 0, &renderer },
		{ WESTON_OPTION_STRING, "shell", 0, &shell },
		{ WESTON_OPTION_STRING, "socket", 'S', &socket_name },
		{ WESTON_OPTION_INTEGER, "idle-time", 'i', &idle_time },
#if defined(BUILD_XWAYLAND)
		{ WESTON_OPTION_BOOLEAN, "xwayland", 0, &xwayland },
#endif
		{ WESTON_OPTION_STRING, "modules", 0, &option_modules },
		{ WESTON_OPTION_STRING, "log", 0, &log },
		{ WESTON_OPTION_BOOLEAN, "help", 'h', &help },
		{ WESTON_OPTION_BOOLEAN, "version", 0, &version },
		{ WESTON_OPTION_BOOLEAN, "no-config", 0, &noconfig },
		{ WESTON_OPTION_STRING, "config", 'c', &config_file },
		{ WESTON_OPTION_BOOLEAN, "wait-for-debugger", 0, &wait_for_debugger },
		{ WESTON_OPTION_BOOLEAN, "debug", 0, &debug_protocol },
		{ WESTON_OPTION_STRING, "logger-scopes", 'l', &log_scopes },
		{ WESTON_OPTION_STRING, "flight-rec-scopes", 'f', &flight_rec_scopes },
	};

	wl_list_init(&wet.layoutput_list);

	os_fd_set_cloexec(fileno(stdin));

	cmdline = copy_command_line(argc, argv);
	parse_options(core_options, ARRAY_LENGTH(core_options), &argc, argv);

	if (help) {
		free(cmdline);
		usage(EXIT_SUCCESS);
	}

	if (version) {
		printf(PACKAGE_STRING "\n");
		free(cmdline);

		return EXIT_SUCCESS;
	}

	log_ctx = weston_log_ctx_create();
	if (!log_ctx) {
		fprintf(stderr, "Failed to initialize weston debug framework.\n");
		return EXIT_FAILURE;
	}

	log_scope = weston_log_ctx_add_log_scope(log_ctx, "log",
			"Weston and Wayland log\n", NULL, NULL, NULL);

	if (!weston_log_file_open(log))
		return EXIT_FAILURE;

	weston_log_set_handler(vlog, vlog_continue);

	logger = weston_log_subscriber_create_log(weston_logfile);

	if (!flight_rec_scopes)
		flight_rec_scopes = DEFAULT_FLIGHT_REC_SCOPES;

	if (flight_rec_scopes && strlen(flight_rec_scopes) > 0)
		flight_rec = weston_log_subscriber_create_flight_rec(DEFAULT_FLIGHT_REC_SIZE);

	weston_log_subscribe_to_scopes(log_ctx, logger, flight_rec,
				       log_scopes, flight_rec_scopes);

	weston_log("%s\n"
		   STAMP_SPACE "%s\n"
		   STAMP_SPACE "Bug reports to: %s\n"
		   STAMP_SPACE "Build: %s\n",
		   PACKAGE_STRING, PACKAGE_URL, PACKAGE_BUGREPORT,
		   BUILD_ID);
	weston_log("Command line: %s\n", cmdline);
	free(cmdline);
	log_uname();

	weston_log("Flight recorder: %s\n", flight_rec ? "enabled" : "disabled");
	verify_xdg_runtime_dir();

	display = wl_display_create();
	if (display == NULL) {
		weston_log("fatal: failed to create display\n");
		goto out_display;
	}

	loop = wl_display_get_event_loop(display);
	signals[0] = wl_event_loop_add_signal(loop, SIGTERM, on_term_signal,
					      display);
	signals[1] = wl_event_loop_add_signal(loop, SIGUSR2, on_term_signal,
					      display);

	wl_list_init(&wet.child_process_list);
	signals[2] = wl_event_loop_add_signal(loop, SIGCHLD, sigchld_handler,
					      &wet);

	/* When debugging weston, if use wl_event_loop_add_signal() to catch
	 * SIGINT, the debugger can't catch it, and attempting to stop
	 * weston from within the debugger results in weston exiting
	 * cleanly.
	 *
	 * Instead, use the sigaction() function, which sets up the signal
	 * in a way that gdb can successfully catch, but have the handler
	 * for SIGINT send SIGUSR2 (xwayland uses SIGUSR1), which we catch
	 * via wl_event_loop_add_signal().
	 */
	action.sa_handler = sigint_helper;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT, &action, NULL);
	if (!signals[0] || !signals[1] || !signals[2])
		goto out_signals;

	/* Xwayland uses SIGUSR1 for communicating with weston. Since some
	   weston plugins may create additional threads, set up any necessary
	   signal blocking early so that these threads can inherit the settings
	   when created. */
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	if (load_configuration(&config, noconfig, config_file) < 0)
		goto out_signals;
	wet.config = config;
	wet.parsed_options = NULL;

	section = weston_config_get_section(config, "core", NULL, NULL);

	if (!wait_for_debugger) {
		weston_config_section_get_bool(section, "wait-for-debugger",
					       &wait_for_debugger, false);
	}
	if (wait_for_debugger) {
		weston_log("Weston PID is %ld - "
			   "waiting for debugger, send SIGCONT to continue...\n",
			   (long)getpid());
		raise(SIGSTOP);
	}

	if (!renderer) {
		weston_config_section_get_string(section, "renderer",
						 &renderer, NULL);
	}

	if (!backend) {
		weston_config_section_get_string(section, "backend", &backend,
						 NULL);
		if (!backend)
			backend = weston_choose_default_backend();
	}

	wet.compositor = weston_compositor_create(display, log_ctx, &wet, test_data);
	if (wet.compositor == NULL) {
		weston_log("fatal: failed to create compositor\n");
		goto out;
	}

	protocol_scope =
		weston_log_ctx_add_log_scope(log_ctx, "proto",
					     "Wayland protocol dump for all clients.\n",
					     NULL, NULL, NULL);

	protologger = wl_display_add_protocol_logger(display,
						     protocol_log_fn,
						     NULL);
	if (debug_protocol) {
		weston_compositor_enable_debug_protocol(wet.compositor);
		weston_compositor_add_screenshot_authority(wet.compositor,
							   &wet.screenshot_auth,
							   screenshot_allow_all);
	}

	if (flight_rec)
		weston_compositor_add_debug_binding(wet.compositor, KEY_D,
						    flight_rec_key_binding_handler,
						    flight_rec);

	if (weston_compositor_init_config(wet.compositor, config) < 0)
		goto out;

	weston_config_section_get_bool(section, "require-input",
				       &wet.compositor->require_input, true);

	if (load_backend(wet.compositor, backend, &argc, argv, config,
			 renderer) < 0) {
		weston_log("fatal: failed to create compositor backend\n");
		goto out;
	}

	if (test_data && !check_compositor_capabilities(wet.compositor,
				test_data->test_quirks.required_capabilities)) {
		ret = WET_MAIN_RET_MISSING_CAPS;
		goto out;
	}

	weston_compositor_flush_heads_changed(wet.compositor);
	if (wet.init_failed)
		goto out;

	if (idle_time < 0)
		weston_config_section_get_int(section, "idle-time", &idle_time, -1);
	if (idle_time < 0)
		idle_time = 300; /* default idle timeout, in seconds */

	wet.compositor->idle_time = idle_time;
	wet.compositor->default_pointer_grab = NULL;
	wet.compositor->exit = handle_exit;

	weston_compositor_log_capabilities(wet.compositor);

	server_socket = getenv("WAYLAND_SERVER_SOCKET");
	if (server_socket) {
		weston_log("Running with single client\n");
		if (!safe_strtoint(server_socket, &fd))
			fd = -1;
	} else {
		fd = -1;
	}

	if (fd != -1) {
		primary_client = wl_client_create(display, fd);
		if (!primary_client) {
			weston_log("fatal: failed to add client: %s\n",
				   strerror(errno));
			goto out;
		}
		primary_client_destroyed.notify =
			handle_primary_client_destroyed;
		wl_client_add_destroy_listener(primary_client,
					       &primary_client_destroyed);
	} else if (weston_create_listening_socket(display, socket_name)) {
		goto out;
	}

	if (!shell)
		weston_config_section_get_string(section, "shell", &shell,
						 "desktop");

	if (wet_load_shell(wet.compositor, shell, &argc, argv) < 0)
		goto out;

	/* Load xwayland before other modules - this way if we're using
	 * the systemd-notify module it will notify after we're ready
	 * to receive xwayland connections.
	 */
	if (!xwayland) {
		weston_config_section_get_bool(section, "xwayland", &xwayland,
					       false);
	}
	if (xwayland) {
		if (wet_load_xwayland(wet.compositor) < 0)
			goto out;
	}

	weston_config_section_get_string(section, "modules", &modules, "");
	if (load_modules(wet.compositor, modules, &argc, argv) < 0)
		goto out;

	if (load_modules(wet.compositor, option_modules, &argc, argv) < 0)
		goto out;

	section = weston_config_get_section(config, "keyboard", NULL, NULL);
	weston_config_section_get_bool(section, "numlock-on", &numlock_on, false);
	if (numlock_on) {
		wl_list_for_each(seat, &wet.compositor->seat_list, link) {
			struct weston_keyboard *keyboard =
				weston_seat_get_keyboard(seat);

			if (keyboard)
				weston_keyboard_set_locks(keyboard,
							  WESTON_NUM_LOCK,
							  WESTON_NUM_LOCK);
		}
	}

	for (i = 1; i < argc; i++)
		weston_log("fatal: unhandled option: %s\n", argv[i]);
	if (argc > 1)
		goto out;

	weston_compositor_wake(wet.compositor);

	if (execute_autolaunch(&wet, config) < 0)
		goto out;

	wl_display_run(display);

	/* Allow for setting return exit code after
	* wl_display_run returns normally. This is
	* useful for devs/testers and automated tests
	* that want to indicate failure status to
	* testing infrastructure above
	*/
	ret = wet.compositor->exit_code;

out:
	/* free(NULL) is valid, and it won't be NULL if it's used */
	free(wet.parsed_options);

	if (protologger)
		wl_protocol_logger_destroy(protologger);

	weston_compositor_destroy(wet.compositor);
	wet_compositor_destroy_layout(&wet);
	weston_log_scope_destroy(protocol_scope);
	protocol_scope = NULL;

out_signals:
	for (i = ARRAY_LENGTH(signals) - 1; i >= 0; i--)
		if (signals[i])
			wl_event_source_remove(signals[i]);

	wl_display_destroy(display);

out_display:
	weston_log_scope_destroy(log_scope);
	log_scope = NULL;
	weston_log_subscriber_destroy(logger);
	if (flight_rec)
		weston_log_subscriber_destroy(flight_rec);
	weston_log_ctx_destroy(log_ctx);
	weston_log_file_close();

	if (config)
		weston_config_destroy(config);
	free(config_file);
	free(backend);
	free(renderer);
	free(shell);
	free(socket_name);
	free(option_modules);
	free(log);
	free(log_scopes);
	free(modules);

	return ret;
}
