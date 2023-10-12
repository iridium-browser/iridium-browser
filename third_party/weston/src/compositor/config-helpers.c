/*
 * Copyright 2022 Collabora, Ltd.
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

#include <libweston/libweston.h>
#include <libweston/config-parser.h>

#include "shared/helpers.h"
#include "weston-private.h"

struct {
	char *short_name;
	char *long_name;
	enum weston_compositor_backend backend;
} backend_name_map[] = {
	{ "drm", "drm-backend.so", WESTON_BACKEND_DRM },
	{ "headless", "headless-backend.so", WESTON_BACKEND_HEADLESS },
	{ "pipewire", "pipewire-backend.so", WESTON_BACKEND_PIPEWIRE },
	{ "rdp", "rdp-backend.so", WESTON_BACKEND_RDP },
	{ "vnc", "vnc-backend.so", WESTON_BACKEND_VNC },
	{ "wayland", "wayland-backend.so", WESTON_BACKEND_WAYLAND },
	{ "x11", "x11-backend.so", WESTON_BACKEND_X11 },
};

bool
get_backend_from_string(const char *name,
			enum weston_compositor_backend *backend)
{
	size_t i;

	for (i = 0; i < ARRAY_LENGTH(backend_name_map); i++) {
		if (strcmp(name, backend_name_map[i].short_name) == 0 ||
		    strcmp(name, backend_name_map[i].long_name) == 0) {
			*backend = backend_name_map[i].backend;
			return true;
		}
	}

	return false;
}

struct {
	char *name;
	enum weston_renderer_type renderer;
} renderer_name_map[] = {
	{ "auto", WESTON_RENDERER_AUTO },
	{ "gl", WESTON_RENDERER_GL },
	{ "noop", WESTON_RENDERER_NOOP },
	{ "pixman", WESTON_RENDERER_PIXMAN },
};

bool
get_renderer_from_string(const char *name,
			 enum weston_renderer_type *renderer)
{
	size_t i;

	if (!name)
		name = "auto";

	for (i = 0; i < ARRAY_LENGTH(renderer_name_map); i++) {
		if (strcmp(name, renderer_name_map[i].name) == 0) {
			*renderer = renderer_name_map[i].renderer;
			return true;
		}
	}

	return false;
}
