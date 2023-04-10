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

#ifndef WESTON_TEST_FIXTURE_COMPOSITOR_H
#define WESTON_TEST_FIXTURE_COMPOSITOR_H

#include <wayland-client-protocol.h>
#include <libweston/libweston.h>

#include "weston-testsuite-data.h"

/** Weston renderer type
 *
 * \sa compositor_setup
 * \ingroup testharness
 */
enum renderer_type {
	/** Dummy renderer that does nothing. */
	RENDERER_NOOP = 0,
	/** Pixman-renderer */
	RENDERER_PIXMAN,
	/** GL-renderer */
	RENDERER_GL
};

/** Weston shell plugin
 *
 * \sa compositor_setup
 * \ingroup testharness
 */
enum shell_type {
	/** Desktop test-shell with predictable window placement and
	 * no helper clients */
	SHELL_TEST_DESKTOP = 0,
	/** The full desktop shell. */
	SHELL_DESKTOP,
	/** The ivi-shell. */
	SHELL_IVI,
	/** The fullscreen-shell. */
	SHELL_FULLSCREEN
};

/** Weston compositor configuration
 *
 * This structure determines the Weston compositor command line arguments.
 * You should always use compositor_setup_defaults() to initialize this, then
 * override any members you need with assignments.
 *
 * \ingroup testharness
 */
struct compositor_setup {
	/** The backend to use. */
	enum weston_compositor_backend backend;
	/** The renderer to use. */
	enum renderer_type renderer;
	/** The shell plugin to use. */
	enum shell_type shell;
	/** Whether to enable xwayland support. */
	bool xwayland;
	/** Default output width. */
	unsigned width;
	/** Default output height. */
	unsigned height;
	/** Default output scale. */
	int scale;
	/** Default output transform, one of WL_OUTPUT_TRANSFORM_*. */
	enum wl_output_transform transform;
	/** The absolute path to \c weston.ini to use,
	 * or NULL for \c --no-config . */
	const char *config_file;
	/** Full path to an extra plugin to load, or NULL for none. */
	const char *extra_module;
	/** Debug scopes for the compositor log,
	 * or NULL for compositor defaults. */
	const char *logging_scopes;
	/** The name of this test program, used as a unique identifier. */
	const char *testset_name;
};

void
compositor_setup_defaults_(struct compositor_setup *setup,
			   const char *testset_name);

/** Initialize compositor setup to defaults
 *
 * \param s The variable to initialize.
 *
 * The defaults are:
 * - backend: headless
 * - renderer: noop
 * - shell: desktop shell
 * - xwayland: no
 * - width: 320
 * - height: 240
 * - scale: 1
 * - transform: WL_OUTPUT_TRANSFORM_NORMAL
 * - config_file: none
 * - extra_module: none
 * - logging_scopes: compositor defaults
 * - testset_name: the test name from meson.build
 *
 * \ingroup testharness
 */
#define compositor_setup_defaults(s) do {\
	compositor_setup_defaults_(s, THIS_TEST_NAME); \
} while (0)

int
execute_compositor(const struct compositor_setup *setup,
		   struct wet_testsuite_data *data);

#endif /* WESTON_TEST_FIXTURE_COMPOSITOR_H */
