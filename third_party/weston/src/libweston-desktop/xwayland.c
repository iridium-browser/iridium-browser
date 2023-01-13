/*
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2013 Raspberry Pi Foundation
 * Copyright © 2016 Quentin "Sardem FF7" Glidic
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

#include <assert.h>

#include <wayland-server.h>

#include <libweston/libweston.h>
#include <libweston/zalloc.h>

#include <libweston-desktop/libweston-desktop.h>
#include "internal.h"
#include "xwayland/xwayland-internal-interface.h"

enum weston_desktop_xwayland_surface_state {
	NONE,
	TOPLEVEL,
	MAXIMIZED,
	FULLSCREEN,
	TRANSIENT,
	XWAYLAND,
};

struct weston_desktop_xwayland {
	struct weston_desktop *desktop;
	struct weston_desktop_client *client;
	struct weston_layer layer;
};

struct weston_desktop_xwayland_surface {
	struct weston_desktop_xwayland *xwayland;
	struct weston_desktop *desktop;
	struct weston_desktop_surface *surface;
	struct wl_listener resource_destroy_listener;
	struct weston_view *view;
	const struct weston_xwayland_client_interface *client_interface;
	struct weston_geometry next_geometry;
	bool has_next_geometry;
	bool committed;
	bool added;
	enum weston_desktop_xwayland_surface_state state;
};

static void
weston_desktop_xwayland_surface_change_state(struct weston_desktop_xwayland_surface *surface,
					     enum weston_desktop_xwayland_surface_state state,
					     struct weston_desktop_surface *parent,
					     int32_t x, int32_t y)
{
	struct weston_surface *wsurface;
	bool to_add = (parent == NULL && state != XWAYLAND);

	assert(state != NONE);
	assert(!parent || state == TRANSIENT);

	if (to_add && surface->added) {
		surface->state = state;
		return;
	}

	wsurface = weston_desktop_surface_get_surface(surface->surface);

	if (surface->state != state) {
		if (surface->state == XWAYLAND) {
			assert(!surface->added);

			weston_desktop_surface_unlink_view(surface->view);
			weston_view_destroy(surface->view);
			surface->view = NULL;
			weston_surface_unmap(wsurface);
		}

		if (to_add) {
			weston_desktop_surface_unset_relative_to(surface->surface);
			weston_desktop_api_surface_added(surface->desktop,
							 surface->surface);
			surface->added = true;
			if (surface->state == NONE && surface->committed)
				/* We had a race, and wl_surface.commit() was
				 * faster, just fake a commit to map the
				 * surface */
				weston_desktop_api_committed(surface->desktop,
							     surface->surface,
							     0, 0);

		} else if (surface->added) {
			weston_desktop_api_surface_removed(surface->desktop,
							   surface->surface);
			surface->added = false;
		}

		if (state == XWAYLAND) {
			assert(!surface->added);

			surface->view =
				weston_desktop_surface_create_view(surface->surface);
			weston_layer_entry_insert(&surface->xwayland->layer.view_list,
						  &surface->view->layer_link);
			surface->view->is_mapped = true;
			wsurface->is_mapped = true;
		}

		surface->state = state;
	}

	if (parent != NULL)
		weston_desktop_surface_set_relative_to(surface->surface, parent,
						       x, y, false);
}

static void
weston_desktop_xwayland_surface_committed(struct weston_desktop_surface *dsurface,
					  void *user_data,
					  int32_t sx, int32_t sy)
{
	struct weston_desktop_xwayland_surface *surface = user_data;
	struct weston_geometry oldgeom;

	assert(dsurface == surface->surface);
	surface->committed = true;

#ifdef WM_DEBUG
	weston_log("%s: xwayland surface %p\n", __func__, surface);
#endif

	if (surface->has_next_geometry) {
		oldgeom = weston_desktop_surface_get_geometry(surface->surface);
		sx -= surface->next_geometry.x - oldgeom.x;
		sy -= surface->next_geometry.y - oldgeom.x;

		surface->has_next_geometry = false;
		weston_desktop_surface_set_geometry(surface->surface,
						    surface->next_geometry);
	}

	if (surface->added)
		weston_desktop_api_committed(surface->desktop, surface->surface,
					     sx, sy);
}

static void
weston_desktop_xwayland_surface_set_size(struct weston_desktop_surface *dsurface,
					 void *user_data,
					 int32_t width, int32_t height)
{
	struct weston_desktop_xwayland_surface *surface = user_data;
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(surface->surface);

	surface->client_interface->send_configure(wsurface, width, height);
}

static void
weston_desktop_xwayland_surface_destroy(struct weston_desktop_surface *dsurface,
					void *user_data)
{
	struct weston_desktop_xwayland_surface *surface = user_data;

	wl_list_remove(&surface->resource_destroy_listener.link);

	weston_desktop_surface_unset_relative_to(surface->surface);
	if (surface->added)
		weston_desktop_api_surface_removed(surface->desktop,
						   surface->surface);
	else if (surface->state == XWAYLAND)
		weston_desktop_surface_unlink_view(surface->view);

	free(surface);
}

static bool
weston_desktop_xwayland_surface_get_maximized(struct weston_desktop_surface *dsurface,
					      void *user_data)
{
	struct weston_desktop_xwayland_surface *surface = user_data;

	return surface->state == MAXIMIZED;
}

static bool
weston_desktop_xwayland_surface_get_fullscreen(struct weston_desktop_surface *dsurface,
					       void *user_data)
{
	struct weston_desktop_xwayland_surface *surface = user_data;

	return surface->state == FULLSCREEN;
}

static const struct weston_desktop_surface_implementation weston_desktop_xwayland_surface_internal_implementation = {
	.committed = weston_desktop_xwayland_surface_committed,
	.set_size = weston_desktop_xwayland_surface_set_size,

	.get_maximized = weston_desktop_xwayland_surface_get_maximized,
	.get_fullscreen = weston_desktop_xwayland_surface_get_fullscreen,

	.destroy = weston_desktop_xwayland_surface_destroy,
};

static void
weston_destop_xwayland_resource_destroyed(struct wl_listener *listener,
					  void *data)
{
	struct weston_desktop_xwayland_surface *surface =
		wl_container_of(listener, surface, resource_destroy_listener);

	weston_desktop_surface_destroy(surface->surface);
}

static struct weston_desktop_xwayland_surface *
create_surface(struct weston_desktop_xwayland *xwayland,
	       struct weston_surface *wsurface,
	       const struct weston_xwayland_client_interface *client_interface)
{
	struct weston_desktop_xwayland_surface *surface;

	surface = zalloc(sizeof(struct weston_desktop_xwayland_surface));
	if (surface == NULL)
		return NULL;

	surface->xwayland = xwayland;
	surface->desktop = xwayland->desktop;
	surface->client_interface = client_interface;

	surface->surface =
		weston_desktop_surface_create(surface->desktop,
					      xwayland->client, wsurface,
					      &weston_desktop_xwayland_surface_internal_implementation,
					      surface);
	if (surface->surface == NULL) {
		free(surface);
		return NULL;
	}

	surface->resource_destroy_listener.notify =
		weston_destop_xwayland_resource_destroyed;
	wl_resource_add_destroy_listener(wsurface->resource,
					 &surface->resource_destroy_listener);

	weston_desktop_surface_set_pid(surface->surface, 0);

	return surface;
}

static void
set_toplevel(struct weston_desktop_xwayland_surface *surface)
{
	weston_desktop_xwayland_surface_change_state(surface, TOPLEVEL, NULL,
						     0, 0);
}

static void
set_toplevel_with_position(struct weston_desktop_xwayland_surface *surface,
			   int32_t x, int32_t y)
{
	weston_desktop_xwayland_surface_change_state(surface, TOPLEVEL, NULL,
						     0, 0);
	weston_desktop_api_set_xwayland_position(surface->desktop,
						 surface->surface, x, y);
}

static void
set_parent(struct weston_desktop_xwayland_surface *surface,
	   struct weston_surface *wparent)
{
	struct weston_desktop_surface *parent;

	if (!weston_surface_is_desktop_surface(wparent))
		return;

	parent = weston_surface_get_desktop_surface(wparent);
	weston_desktop_api_set_parent(surface->desktop, surface->surface, parent);
}

static void
set_transient(struct weston_desktop_xwayland_surface *surface,
	      struct weston_surface *wparent, int x, int y)
{
	struct weston_desktop_surface *parent;

	if (!weston_surface_is_desktop_surface(wparent))
		return;

	parent = weston_surface_get_desktop_surface(wparent);
	weston_desktop_xwayland_surface_change_state(surface, TRANSIENT, parent,
						     x, y);
}

static void
set_fullscreen(struct weston_desktop_xwayland_surface *surface,
	       struct weston_output *output)
{
	weston_desktop_xwayland_surface_change_state(surface, FULLSCREEN, NULL,
						     0, 0);
	weston_desktop_api_fullscreen_requested(surface->desktop,
						surface->surface, true, output);
}

static void
set_xwayland(struct weston_desktop_xwayland_surface *surface, int x, int y)
{
	weston_desktop_xwayland_surface_change_state(surface, XWAYLAND, NULL,
						     x, y);
	weston_view_set_position(surface->view, x, y);
}

static int
move(struct weston_desktop_xwayland_surface *surface,
     struct weston_pointer *pointer)
{
	if (surface->state == TOPLEVEL ||
	    surface->state == MAXIMIZED ||
	    surface->state == FULLSCREEN)
		weston_desktop_api_move(surface->desktop, surface->surface,
					pointer->seat, pointer->grab_serial);
	return 0;
}

static int
resize(struct weston_desktop_xwayland_surface *surface,
       struct weston_pointer *pointer, uint32_t edges)
{
	if (surface->state == TOPLEVEL ||
	    surface->state == MAXIMIZED ||
	    surface->state == FULLSCREEN)
		weston_desktop_api_resize(surface->desktop, surface->surface,
					  pointer->seat, pointer->grab_serial,
					  edges);
	return 0;
}

static void
set_title(struct weston_desktop_xwayland_surface *surface, const char *title)
{
	weston_desktop_surface_set_title(surface->surface, title);
}

static void
set_window_geometry(struct weston_desktop_xwayland_surface *surface,
		    int32_t x, int32_t y, int32_t width, int32_t height)
{
	surface->has_next_geometry = true;
	surface->next_geometry.x = x;
	surface->next_geometry.y = y;
	surface->next_geometry.width = width;
	surface->next_geometry.height = height;
}

static void
set_maximized(struct weston_desktop_xwayland_surface *surface)
{
	weston_desktop_xwayland_surface_change_state(surface, MAXIMIZED, NULL,
						     0, 0);
	weston_desktop_api_maximized_requested(surface->desktop,
					       surface->surface, true);
}

static void
set_pid(struct weston_desktop_xwayland_surface *surface, pid_t pid)
{
	weston_desktop_surface_set_pid(surface->surface, pid);
}

static const struct weston_desktop_xwayland_interface weston_desktop_xwayland_interface = {
	.create_surface = create_surface,
	.set_toplevel = set_toplevel,
	.set_toplevel_with_position = set_toplevel_with_position,
	.set_parent = set_parent,
	.set_transient = set_transient,
	.set_fullscreen = set_fullscreen,
	.set_xwayland = set_xwayland,
	.move = move,
	.resize = resize,
	.set_title = set_title,
	.set_window_geometry = set_window_geometry,
	.set_maximized = set_maximized,
	.set_pid = set_pid,
};

void
weston_desktop_xwayland_init(struct weston_desktop *desktop)
{
	struct weston_compositor *compositor = weston_desktop_get_compositor(desktop);
	struct weston_desktop_xwayland *xwayland;

	xwayland = zalloc(sizeof(struct weston_desktop_xwayland));
	if (xwayland == NULL)
		return;

	xwayland->desktop = desktop;
	xwayland->client = weston_desktop_client_create(desktop, NULL, NULL, NULL, NULL, 0, 0);

	weston_layer_init(&xwayland->layer, compositor);
	/* We put this layer on top of regular shell surfaces, but hopefully
	 * below any UI the shell would add */
	weston_layer_set_position(&xwayland->layer,
				  WESTON_LAYER_POSITION_NORMAL + 1);

	compositor->xwayland = xwayland;
	compositor->xwayland_interface = &weston_desktop_xwayland_interface;
}
