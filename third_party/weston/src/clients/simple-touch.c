/*
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include "shared/helpers.h"
#include "shared/os-compatibility.h"

struct seat {
	struct touch *touch;
	struct wl_seat *seat;
	struct wl_touch *wl_touch;
};

struct touch {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_shm *shm;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	struct wl_buffer *buffer;
	int has_argb;
	int width, height;
	void *data;
};

static void
create_shm_buffer(struct touch *touch)
{
	struct wl_shm_pool *pool;
	int fd, size, stride;

	stride = touch->width * 4;
	size = stride * touch->height;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %s\n",
			size, strerror(errno));
		exit(1);
	}

	touch->data =
		mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (touch->data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s\n", strerror(errno));
		close(fd);
		exit(1);
	}

	pool = wl_shm_create_pool(touch->shm, fd, size);
	touch->buffer =
		wl_shm_pool_create_buffer(pool, 0,
					  touch->width, touch->height, stride,
					  WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);

	close(fd);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	struct touch *touch = data;

	if (format == WL_SHM_FORMAT_ARGB8888)
		touch->has_argb = 1;
}

struct wl_shm_listener shm_listener = {
	shm_format
};


static void
touch_paint(struct touch *touch, int32_t x, int32_t y, int32_t id)
{
	uint32_t *p, c;
	static const uint32_t colors[] = {
		0xffff0000,
		0xffffff00,
		0xff0000ff,
		0xffff00ff,
		0xff00ff00,
		0xff00ffff,
	};

	if (id < (int32_t) ARRAY_LENGTH(colors))
		c = colors[id];
	else
		c = 0xffffffff;

	if (x < 2 || x >= touch->width - 2 ||
	    y < 2 || y >= touch->height - 2)
		return;

	p = (uint32_t *) touch->data + (x - 2) + (y - 2) * touch->width;
	p[2] = c;
	p += touch->width;
	p[1] = c;
	p[2] = c;
	p[3] = c;
	p += touch->width;
	p[0] = c;
	p[1] = c;
	p[2] = c;
	p[3] = c;
	p[4] = c;
	p += touch->width;
	p[1] = c;
	p[2] = c;
	p[3] = c;
	p += touch->width;
	p[2] = c;

	wl_surface_attach(touch->surface, touch->buffer, 0, 0);
	wl_surface_damage(touch->surface, x - 2, y - 2, 5, 5);
	/* todo: We could queue up more damage before committing, if there
	 * are more input events to handle.
	 */
	wl_surface_commit(touch->surface);
}

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
		  uint32_t serial, uint32_t time, struct wl_surface *surface,
		  int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct touch *touch = data;
	float x = wl_fixed_to_double(x_w);
	float y = wl_fixed_to_double(y_w);

	touch_paint(touch, x, y, id);
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id)
{
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
		    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct touch *touch = data;
	float x = wl_fixed_to_double(x_w);
	float y = wl_fixed_to_double(y_w);

	touch_paint(touch, x, y, id);
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
			 enum wl_seat_capability caps)
{
	struct seat *seat = data;
	struct touch *touch = seat->touch;

	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !seat->wl_touch) {
		seat->wl_touch = wl_seat_get_touch(wl_seat);
		wl_touch_set_user_data(seat->wl_touch, touch);
		wl_touch_add_listener(seat->wl_touch, &touch_listener, touch);
	} else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && seat->wl_touch) {
		wl_touch_destroy(seat->wl_touch);
		seat->wl_touch = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
};

static void
add_seat(struct touch *touch, uint32_t name, uint32_t version)
{
	struct seat *seat;

	seat = malloc(sizeof *seat);
	assert(seat);

	seat->touch = touch;
	seat->wl_touch = NULL;
	seat->seat = wl_registry_bind(touch->registry, name,
				      &wl_seat_interface, 1);
	wl_seat_add_listener(seat->seat, &seat_listener, seat);
}

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
	    uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
		 uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done
};

static void
handle_global(void *data, struct wl_registry *registry,
	      uint32_t name, const char *interface, uint32_t version)
{
	struct touch *touch = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		touch->compositor =
			wl_registry_bind(registry, name,
					 &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_shell") == 0) {
		touch->shell =
			wl_registry_bind(registry, name,
					 &wl_shell_interface, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		touch->shm = wl_registry_bind(registry, name,
					      &wl_shm_interface, 1);
		wl_shm_add_listener(touch->shm, &shm_listener, touch);
	} else if (strcmp(interface, "wl_seat") == 0) {
		add_seat(touch, name, version);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove
};

static struct touch *
touch_create(int width, int height)
{
	struct touch *touch;

	touch = malloc(sizeof *touch);
	if (touch == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	touch->display = wl_display_connect(NULL);
	assert(touch->display);

	touch->has_argb = 0;
	touch->registry = wl_display_get_registry(touch->display);
	wl_registry_add_listener(touch->registry, &registry_listener, touch);
	wl_display_dispatch(touch->display);
	wl_display_roundtrip(touch->display);

	if (!touch->has_argb) {
		fprintf(stderr, "WL_SHM_FORMAT_ARGB32 not available\n");
		exit(1);
	}

	touch->width = width;
	touch->height = height;
	touch->surface = wl_compositor_create_surface(touch->compositor);
	touch->shell_surface = wl_shell_get_shell_surface(touch->shell,
							  touch->surface);
	create_shm_buffer(touch);

	if (touch->shell_surface) {
		wl_shell_surface_add_listener(touch->shell_surface,
					      &shell_surface_listener, touch);
		wl_shell_surface_set_toplevel(touch->shell_surface);
	}

	wl_surface_set_user_data(touch->surface, touch);
	wl_shell_surface_set_title(touch->shell_surface, "simple-touch");

	memset(touch->data, 64, width * height * 4);
	wl_surface_attach(touch->surface, touch->buffer, 0, 0);
	wl_surface_damage(touch->surface, 0, 0, width, height);
	wl_surface_commit(touch->surface);

	return touch;
}

int
main(int argc, char **argv)
{
	struct touch *touch;
	int ret = 0;

	touch = touch_create(600, 500);

	while (ret != -1)
		ret = wl_display_dispatch(touch->display);

	return 0;
}
