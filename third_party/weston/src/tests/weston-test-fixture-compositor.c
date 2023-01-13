/*
 * Copyright 2019 Collabora, Ltd.
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

#include <string.h>
#include <assert.h>
#include <libudev.h>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>

#include "shared/helpers.h"
#include "weston-test-fixture-compositor.h"
#include "weston.h"
#include "test-config.h"

struct prog_args {
	int argc;
	char **argv;
	char **saved;
	int alloc;
};

static void
prog_args_init(struct prog_args *p)
{
	memset(p, 0, sizeof(*p));
}

static void
prog_args_take(struct prog_args *p, char *arg)
{
	assert(arg);

	if (p->argc == p->alloc) {
		p->alloc += 10;
		p->argv = realloc(p->argv, sizeof(char *) * p->alloc);
		assert(p->argv);
	}

	p->argv[p->argc++] = arg;
}

/*
 * The program to be executed will trample on argv, hence we need a copy to
 * be able to free all our args.
 */
static void
prog_args_save(struct prog_args *p)
{
	assert(p->saved == NULL);

	p->saved = calloc(p->argc, sizeof(char *));
	assert(p->saved);

	memcpy(p->saved, p->argv, sizeof(char *) * p->argc);
}

static void
prog_args_fini(struct prog_args *p)
{
	int i;

	assert(p->saved);

	for (i = 0; i < p->argc; i++)
		free(p->saved[i]);
	free(p->saved);
	free(p->argv);
	prog_args_init(p);
}

static char *
get_lock_path(void)
{
	const char *env_path, *suffix;
	char *lock_path;

	suffix = "weston-test-suite-drm-lock";
	env_path = getenv("XDG_RUNTIME_DIR");
	if (!env_path) {
		fprintf(stderr, "Failed to compute lock file path. " \
			"XDG_RUNTIME_DIR is not set.\n");
		return NULL;
	}

	if (asprintf(&lock_path, "%s/%s", env_path, suffix) == -1)
		return NULL;

	return lock_path;
}

/*
 * DRM-backend tests need to be run sequentially, since there can only be
 * one user at a time with master status in a DRM KMS device. Since the
 * test suite runs the tests in parallel, there's a mechanism to assure
 * only one DRM-backend test is running at a time: tests of this type keep
 * waiting until they acquire a lock (which is hold until they end).
 *
 * Returns -1 in failure and fd in success.
 */
static int
wait_for_lock(void)
{
	char *lock_path;
	int fd;

	lock_path = get_lock_path();
	if (!lock_path)
		goto err_path;

	fd = open(lock_path, O_RDWR | O_CLOEXEC | O_CREAT, 00700);
	if (fd == -1) {
		fprintf(stderr, "Could not open lock file %s: %s\n", lock_path, strerror(errno));
		goto err_open;
	}
	fprintf(stderr, "Waiting for lock on %s...\n", lock_path);

	/* The call is blocking, so we don't need a 'while'. Also, as we
	 * have a timeout for each test, this won't get stuck waiting. */
	if (flock(fd, LOCK_EX) == -1) {
		fprintf(stderr, "Could not lock %s: %s\n", lock_path, strerror(errno));
		goto err_lock;
	}
	fprintf(stderr, "Lock %s acquired.\n", lock_path);
	free(lock_path);

	return fd;

err_lock:
	close(fd);
err_open:
	free(lock_path);
err_path:
	return -1;
}

/** Initialize part of compositor setup
 *
 * \param setup The variable to initialize.
 * \param testset_name Value for testset_name member.
 *
 * \ingroup testharness_private
 */
void
compositor_setup_defaults_(struct compositor_setup *setup,
			   const char *testset_name)
{
	*setup = (struct compositor_setup) {
		.backend = WESTON_BACKEND_HEADLESS,
		.renderer = RENDERER_NOOP,
		.shell = SHELL_DESKTOP,
		.xwayland = false,
		.width = 320,
		.height = 240,
		.scale = 1,
		.transform = WL_OUTPUT_TRANSFORM_NORMAL,
		.config_file = NULL,
		.extra_module = NULL,
		.logging_scopes = NULL,
		.testset_name = testset_name,
	};
}

static const char *
backend_to_str(enum weston_compositor_backend b)
{
	static const char * const names[] = {
		[WESTON_BACKEND_DRM] = "drm-backend.so",
		[WESTON_BACKEND_FBDEV] = "fbdev-backend.so",
		[WESTON_BACKEND_HEADLESS] = "headless-backend.so",
		[WESTON_BACKEND_RDP] = "rdp-backend.so",
		[WESTON_BACKEND_WAYLAND] = "wayland-backend.so",
		[WESTON_BACKEND_X11] = "X11-backend.so",
	};
	assert(b >= 0 && b < ARRAY_LENGTH(names));
	return names[b];
}

static const char *
renderer_to_arg(enum weston_compositor_backend b, enum renderer_type r)
{
	static const char * const headless_names[] = {
		[RENDERER_NOOP] = NULL,
		[RENDERER_PIXMAN] = "--use-pixman",
		[RENDERER_GL] = "--use-gl",
	};
	static const char * const drm_names[] = {
		[RENDERER_PIXMAN] = "--use-pixman",
		[RENDERER_GL] = NULL,
	};

	switch (b) {
	case WESTON_BACKEND_HEADLESS:
		assert(r >= RENDERER_NOOP && r <= RENDERER_GL);
		return headless_names[r];
	case WESTON_BACKEND_DRM:
		assert(r >= RENDERER_PIXMAN && r <= RENDERER_GL);
		return drm_names[r];
	default:
		assert(0 && "renderer_to_str() does not know the backend");
	}

	return NULL;
}

static const char *
shell_to_str(enum shell_type t)
{
	static const char * const names[] = {
		[SHELL_TEST_DESKTOP] = "weston-test-desktop-shell.so",
		[SHELL_DESKTOP] = "desktop-shell.so",
		[SHELL_FULLSCREEN] = "fullscreen-shell.so",
		[SHELL_IVI] = "ivi-shell.so",
	};
	assert(t >= 0 && t < ARRAY_LENGTH(names));
	return names[t];
}

static const char *
transform_to_str(enum wl_output_transform t)
{
	static const char * const names[] = {
		[WL_OUTPUT_TRANSFORM_NORMAL] = "normal",
		[WL_OUTPUT_TRANSFORM_90] = "rotate-90",
		[WL_OUTPUT_TRANSFORM_180] = "rotate-180",
		[WL_OUTPUT_TRANSFORM_270] = "rotate-270",
		[WL_OUTPUT_TRANSFORM_FLIPPED] = "flipped",
		[WL_OUTPUT_TRANSFORM_FLIPPED_90] = "flipped-rotate-90",
		[WL_OUTPUT_TRANSFORM_FLIPPED_180] = "flipped-rotate-180",
		[WL_OUTPUT_TRANSFORM_FLIPPED_270] = "flipped-rotate-270",
	};

	assert(t < ARRAY_LENGTH(names) && names[t]);
	return names[t];
}

/** Execute compositor
 *
 * Manufactures the compositor command line and calls wet_main().
 *
 * Returns RESULT_SKIP if the given setup contains features that were disabled
 * in the build, e.g. GL-renderer or DRM-backend.
 *
 * \ingroup testharness_private
 */
int
execute_compositor(const struct compositor_setup *setup,
		   struct wet_testsuite_data *data)
{
	struct prog_args args;
	char *tmp;
	const char *ctmp, *drm_device;
	int ret, lock_fd = -1;

	if (setenv("WESTON_MODULE_MAP", WESTON_MODULE_MAP, 0) < 0 ||
	    setenv("WESTON_DATA_DIR", WESTON_DATA_DIR, 0) < 0) {
		fprintf(stderr, "Error: environment setup failed.\n");
		return RESULT_HARD_ERROR;
	}

#ifndef BUILD_DRM_COMPOSITOR
	if (setup->backend == WESTON_BACKEND_DRM) {
		fprintf(stderr, "DRM-backend required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#ifndef BUILD_FBDEV_COMPOSITOR
	if (setup->backend == WESTON_BACKEND_FBDEV) {
		fprintf(stderr, "fbdev-backend required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#ifndef BUILD_RDP_COMPOSITOR
	if (setup->backend == WESTON_BACKEND_RDP) {
		fprintf(stderr, "RDP-backend required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#ifndef BUILD_WAYLAND_COMPOSITOR
	if (setup->backend == WESTON_BACKEND_WAYLAND) {
		fprintf(stderr, "wayland-backend required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#ifndef BUILD_X11_COMPOSITOR
	if (setup->backend == WESTON_BACKEND_X11) {
		fprintf(stderr, "X11-backend required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#ifndef ENABLE_EGL
	if (setup->renderer == RENDERER_GL) {
		fprintf(stderr, "GL-renderer required but not built, skipping.\n");
		return RESULT_SKIP;
	}
#endif

#if !TEST_GL_RENDERER
	if (setup->renderer == RENDERER_GL) {
		fprintf(stderr, "GL-renderer disabled for tests, skipping.\n");
		return RESULT_SKIP;
	}
#endif

	prog_args_init(&args);

	/* argv[0] */
	asprintf(&tmp, "weston-%s", setup->testset_name);
	prog_args_take(&args, tmp);

	asprintf(&tmp, "--backend=%s", backend_to_str(setup->backend));
	prog_args_take(&args, tmp);

	if (setup->backend == WESTON_BACKEND_DRM) {

		drm_device = getenv("WESTON_TEST_SUITE_DRM_DEVICE");
		if (!drm_device) {
			fprintf(stderr, "Skipping DRM-backend tests because " \
				"WESTON_TEST_SUITE_DRM_DEVICE is not set. " \
				"See test suite documentation to learn how " \
				"to run them.\n");
			return RESULT_SKIP;
		}
		asprintf(&tmp, "--drm-device=%s", drm_device);
		prog_args_take(&args, tmp);

		prog_args_take(&args, strdup("--seat=weston-test-seat"));

		prog_args_take(&args, strdup("--continue-without-input"));

		lock_fd = wait_for_lock();
		if (lock_fd == -1)
			return RESULT_FAIL;
	}

	asprintf(&tmp, "--socket=%s", setup->testset_name);
	prog_args_take(&args, tmp);

	asprintf(&tmp, "--modules=%s%s%s", TESTSUITE_PLUGIN_PATH,
		 setup->extra_module ? "," : "",
		 setup->extra_module ? setup->extra_module : "");
	prog_args_take(&args, tmp);

	if (setup->backend != WESTON_BACKEND_DRM &&
	    setup->backend != WESTON_BACKEND_FBDEV) {
		asprintf(&tmp, "--width=%d", setup->width);
		prog_args_take(&args, tmp);

		asprintf(&tmp, "--height=%d", setup->height);
		prog_args_take(&args, tmp);
	}

	if (setup->scale != 1) {
		asprintf(&tmp, "--scale=%d", setup->scale);
		prog_args_take(&args, tmp);
	}

	if (setup->transform != WL_OUTPUT_TRANSFORM_NORMAL) {
		asprintf(&tmp, "--transform=%s",
			 transform_to_str(setup->transform));
		prog_args_take(&args, tmp);
	}

	if (setup->config_file) {
		asprintf(&tmp, "--config=%s", setup->config_file);
		prog_args_take(&args, tmp);
	} else {
		prog_args_take(&args, strdup("--no-config"));
	}

	ctmp = renderer_to_arg(setup->backend, setup->renderer);
	if (ctmp)
		prog_args_take(&args, strdup(ctmp));

	asprintf(&tmp, "--shell=%s", shell_to_str(setup->shell));
	prog_args_take(&args, tmp);

	if (setup->logging_scopes) {
		asprintf(&tmp, "--logger-scopes=%s", setup->logging_scopes);
		prog_args_take(&args, tmp);
	}

	if (setup->xwayland)
		prog_args_take(&args, strdup("--xwayland"));

	wet_testsuite_data_set(data);
	prog_args_save(&args);
	ret = wet_main(args.argc, args.argv);

	prog_args_fini(&args);

	/* We acquired a lock (if this is a DRM-backend test) and now we can
	 * close its fd and release it, as it has already been run. */
	if (lock_fd != -1)
		close(lock_fd);

	return ret;
}
