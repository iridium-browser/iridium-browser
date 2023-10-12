/*
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

#ifndef WESTON_H
#define WESTON_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <libweston/libweston.h>
#include <libweston/config-parser.h>

void
screenshooter_create(struct weston_compositor *ec);

struct weston_process;
typedef void (*weston_process_cleanup_func_t)(struct weston_process *process,
					    int status);

struct weston_process {
	pid_t pid;
	weston_process_cleanup_func_t cleanup;
	struct wl_list link;
};

struct custom_env;

bool
weston_client_launch(struct weston_compositor *compositor,
		     struct weston_process *proc,
		     struct custom_env *custom_env,
		     int *fds_no_cloexec,
		     size_t num_fds_no_cloexec,
		     weston_process_cleanup_func_t cleanup);

struct wl_client *
weston_client_start(struct weston_compositor *compositor, const char *path);

void
wet_watch_process(struct weston_compositor *compositor,
				  struct weston_process *process);

struct weston_config *
wet_get_config(struct weston_compositor *compositor);

int
wet_shell_init(struct weston_compositor *ec,
	       int *argc, char *argv[]);
int
wet_module_init(struct weston_compositor *ec,
		int *argc, char *argv[]);
int
wet_load_module(struct weston_compositor *compositor,
	        const char *name, int *argc, char *argv[]);

int
module_init(struct weston_compositor *compositor,
	    int *argc, char *argv[]);

char *
wet_get_libexec_path(const char *name);

char *
wet_get_bindir_path(const char *name);

int
wet_load_xwayland(struct weston_compositor *comp);

struct text_backend;

struct text_backend *
text_backend_init(struct weston_compositor *ec);

void
text_backend_destroy(struct text_backend *text_backend);

/*
 * Return value from wet_main() when
 * weston_testsuite_quirks::required_capabilities are not met.
 */
#define WET_MAIN_RET_MISSING_CAPS 77

int
wet_main(int argc, char *argv[], const struct weston_testsuite_data *test_data);

#ifdef  __cplusplus
}
#endif

#endif
