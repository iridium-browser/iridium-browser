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

#define WD_WL_SHELL_PROTOCOL_VERSION 1

enum weston_desktop_wl_shell_surface_state {
	NONE,
	TOPLEVEL,
	MAXIMIZED,
	FULLSCREEN,
	TRANSIENT,
	POPUP,
};

struct weston_desktop_wl_shell_surface {
	struct wl_resource *resource;
	struct weston_desktop *desktop;
	struct wl_display *display;
	struct weston_desktop_surface *surface;
	struct weston_desktop_surface *parent;
	bool added;
	struct weston_desktop_seat *popup_seat;
	enum weston_desktop_wl_shell_surface_state state;
	struct wl_listener wl_surface_resource_destroy_listener;
};

static void
weston_desktop_wl_shell_surface_set_size(struct weston_desktop_surface *dsurface,
					 void *user_data,
					 int32_t width, int32_t height)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(surface->surface);

	if ((wsurface->width == width && wsurface->height == height) ||
	    (width == 0 && height == 0))
		return;

	wl_shell_surface_send_configure(surface->resource,
					WL_SHELL_SURFACE_RESIZE_NONE,
					width, height);
}

static void
weston_desktop_wl_shell_surface_maybe_ungrab(struct weston_desktop_wl_shell_surface *surface)
{
	if (surface->state != POPUP ||
	    !weston_desktop_surface_get_grab(surface->surface))
		return;

	weston_desktop_surface_popup_ungrab(surface->surface,
					    surface->popup_seat);
	surface->popup_seat = NULL;
}

static void
weston_desktop_wl_shell_surface_committed(struct weston_desktop_surface *dsurface,
					  void *user_data,
					  int32_t sx, int32_t sy)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(dsurface);

	if (wsurface->buffer_ref.buffer == NULL)
		weston_desktop_wl_shell_surface_maybe_ungrab(surface);

	if (surface->added)
		weston_desktop_api_committed(surface->desktop, surface->surface,
					     sx, sy);
}

static void
weston_desktop_wl_shell_surface_ping(struct weston_desktop_surface *dsurface,
				     uint32_t serial, void *user_data)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;

	wl_shell_surface_send_ping(surface->resource, serial);
}

static void
weston_desktop_wl_shell_surface_close(struct weston_desktop_surface *dsurface,
				      void *user_data)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;

	if (surface->state == POPUP)
		wl_shell_surface_send_popup_done(surface->resource);
}

static bool
weston_desktop_wl_shell_surface_get_maximized(struct weston_desktop_surface *dsurface,
					      void *user_data)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;

	return surface->state == MAXIMIZED;
}

static bool
weston_desktop_wl_shell_surface_get_fullscreen(struct weston_desktop_surface *dsurface,
					       void *user_data)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;

	return surface->state == FULLSCREEN;
}

static void
weston_desktop_wl_shell_change_state(struct weston_desktop_wl_shell_surface *surface,
				     enum weston_desktop_wl_shell_surface_state state,
				     struct weston_desktop_surface *parent,
				     int32_t x, int32_t y)
{
	bool to_add = (parent == NULL);

	assert(state != NONE);

	if (to_add && surface->added) {
		surface->state = state;
		return;
	}

	if (surface->state != state) {
		if (surface->state == POPUP)
			weston_desktop_wl_shell_surface_maybe_ungrab(surface);

		if (to_add) {
			weston_desktop_surface_unset_relative_to(surface->surface);
			weston_desktop_api_surface_added(surface->desktop,
							 surface->surface);
		} else if (surface->added) {
			weston_desktop_api_surface_removed(surface->desktop,
							   surface->surface);
		}

		surface->state = state;
		surface->added = to_add;
	}

	if (parent != NULL)
		weston_desktop_surface_set_relative_to(surface->surface, parent,
						       x, y, false);
}

static void
weston_desktop_wl_shell_surface_destroy(struct weston_desktop_surface *dsurface,
					void *user_data)
{
	struct weston_desktop_wl_shell_surface *surface = user_data;

	wl_list_remove(&surface->wl_surface_resource_destroy_listener.link);

	weston_desktop_wl_shell_surface_maybe_ungrab(surface);
	weston_desktop_surface_unset_relative_to(surface->surface);
	if (surface->added)
		weston_desktop_api_surface_removed(surface->desktop,
						   surface->surface);

	free(surface);
}

static void
weston_desktop_wl_shell_surface_protocol_pong(struct wl_client *wl_client,
					      struct wl_resource *resource,
					      uint32_t serial)
{
	struct weston_desktop_surface *surface = wl_resource_get_user_data(resource);

	weston_desktop_client_pong(weston_desktop_surface_get_client(surface), serial);
}

static void
weston_desktop_wl_shell_surface_protocol_move(struct wl_client *wl_client,
					      struct wl_resource *resource,
					      struct wl_resource *seat_resource,
					      uint32_t serial)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *seat =
		wl_resource_get_user_data(seat_resource);
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (seat == NULL)
		return;

	weston_desktop_api_move(surface->desktop, dsurface, seat, serial);
}

static void
weston_desktop_wl_shell_surface_protocol_resize(struct wl_client *wl_client,
						struct wl_resource *resource,
						struct wl_resource *seat_resource,
						uint32_t serial,
						enum wl_shell_surface_resize edges)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *seat = wl_resource_get_user_data(seat_resource);
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);
	enum weston_desktop_surface_edge surf_edges =
		(enum weston_desktop_surface_edge) edges;

	if (seat == NULL)
		return;

	weston_desktop_api_resize(surface->desktop, dsurface, seat, serial, surf_edges);
}

static void
weston_desktop_wl_shell_surface_protocol_set_toplevel(struct wl_client *wl_client,
						struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_wl_shell_change_state(surface, TOPLEVEL, NULL, 0, 0);
	if (surface->parent == NULL)
		return;
	surface->parent = NULL;
	weston_desktop_api_set_parent(surface->desktop, surface->surface, NULL);
}

static void
weston_desktop_wl_shell_surface_protocol_set_transient(struct wl_client *wl_client,
						       struct wl_resource *resource,
						       struct wl_resource *parent_resource,
						       int32_t x, int32_t y,
						       enum wl_shell_surface_transient flags)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_surface *wparent =
		wl_resource_get_user_data(parent_resource);
	struct weston_desktop_surface *parent;
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (!weston_surface_is_desktop_surface(wparent))
		return;

	parent = weston_surface_get_desktop_surface(wparent);
	if (flags & WL_SHELL_SURFACE_TRANSIENT_INACTIVE) {
		weston_desktop_wl_shell_change_state(surface, TRANSIENT, parent,
						     x, y);
	} else {
		weston_desktop_wl_shell_change_state(surface, TOPLEVEL, NULL,
						     0, 0);
		surface->parent = parent;
		weston_desktop_api_set_parent(surface->desktop,
					      surface->surface, parent);
	}
}

static void
weston_desktop_wl_shell_surface_protocol_set_fullscreen(struct wl_client *wl_client,
							struct wl_resource *resource,
							enum wl_shell_surface_fullscreen_method method,
							uint32_t framerate,
							struct wl_resource *output_resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_output *output = NULL;

	if (output_resource != NULL)
		output = weston_head_from_resource(output_resource)->output;

	weston_desktop_wl_shell_change_state(surface, FULLSCREEN, NULL, 0, 0);
	weston_desktop_api_fullscreen_requested(surface->desktop, dsurface,
						true, output);
}

static void
weston_desktop_wl_shell_surface_protocol_set_popup(struct wl_client *wl_client,
						   struct wl_resource *resource,
						   struct wl_resource *seat_resource,
						   uint32_t serial,
						   struct wl_resource *parent_resource,
						   int32_t x, int32_t y,
						   enum wl_shell_surface_transient flags)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *wseat = wl_resource_get_user_data(seat_resource);
	struct weston_desktop_seat *seat = weston_desktop_seat_from_seat(wseat);
	struct weston_surface *parent =
		wl_resource_get_user_data(parent_resource);
	struct weston_desktop_surface *parent_surface;
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	/* Check that if we have a valid wseat we also got a valid desktop seat */
	if (wseat != NULL && seat == NULL) {
		wl_client_post_no_memory(wl_client);
		return;
	}

	if (!weston_surface_is_desktop_surface(parent))
		return;

	parent_surface = weston_surface_get_desktop_surface(parent);

	weston_desktop_wl_shell_change_state(surface, POPUP,
					     parent_surface, x, y);
	weston_desktop_surface_popup_grab(surface->surface, seat, serial);
	surface->popup_seat = seat;
}

static void
weston_desktop_wl_shell_surface_protocol_set_maximized(struct wl_client *wl_client,
						       struct wl_resource *resource,
						       struct wl_resource *output_resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_wl_shell_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_wl_shell_change_state(surface, MAXIMIZED, NULL, 0, 0);
	weston_desktop_api_maximized_requested(surface->desktop, dsurface, true);
}

static void
weston_desktop_wl_shell_surface_protocol_set_title(struct wl_client *wl_client,
						   struct wl_resource *resource,
						   const char *title)
{
	struct weston_desktop_surface *surface =
		wl_resource_get_user_data(resource);

	weston_desktop_surface_set_title(surface, title);
}

static void
weston_desktop_wl_shell_surface_protocol_set_class(struct wl_client *wl_client,
						   struct wl_resource *resource,
						   const char *class_)
{
	struct weston_desktop_surface *surface =
		wl_resource_get_user_data(resource);

	weston_desktop_surface_set_app_id(surface, class_);
}


static const struct wl_shell_surface_interface weston_desktop_wl_shell_surface_implementation = {
	.pong           = weston_desktop_wl_shell_surface_protocol_pong,
	.move           = weston_desktop_wl_shell_surface_protocol_move,
	.resize         = weston_desktop_wl_shell_surface_protocol_resize,
	.set_toplevel   = weston_desktop_wl_shell_surface_protocol_set_toplevel,
	.set_transient  = weston_desktop_wl_shell_surface_protocol_set_transient,
	.set_fullscreen = weston_desktop_wl_shell_surface_protocol_set_fullscreen,
	.set_popup      = weston_desktop_wl_shell_surface_protocol_set_popup,
	.set_maximized  = weston_desktop_wl_shell_surface_protocol_set_maximized,
	.set_title      = weston_desktop_wl_shell_surface_protocol_set_title,
	.set_class      = weston_desktop_wl_shell_surface_protocol_set_class,
};

static const struct weston_desktop_surface_implementation weston_desktop_wl_shell_surface_internal_implementation = {
	.set_size = weston_desktop_wl_shell_surface_set_size,
	.committed = weston_desktop_wl_shell_surface_committed,
	.ping = weston_desktop_wl_shell_surface_ping,
	.close = weston_desktop_wl_shell_surface_close,

	.get_maximized = weston_desktop_wl_shell_surface_get_maximized,
	.get_fullscreen = weston_desktop_wl_shell_surface_get_fullscreen,

	.destroy = weston_desktop_wl_shell_surface_destroy,
};

static void
wl_surface_resource_destroyed(struct wl_listener *listener,
					     void *data)
{
	struct weston_desktop_wl_shell_surface *surface =
		wl_container_of(listener, surface,
				wl_surface_resource_destroy_listener);

	/* the wl_shell_surface spec says that wl_shell_surfaces are to be
	 * destroyed automatically when the wl_surface is destroyed. */
	weston_desktop_surface_destroy(surface->surface);
}

static void
weston_desktop_wl_shell_protocol_get_shell_surface(struct wl_client *wl_client,
						   struct wl_resource *resource,
						   uint32_t id,
						   struct wl_resource *surface_resource)
{
	struct weston_desktop_client *client = wl_resource_get_user_data(resource);
	struct weston_surface *wsurface = wl_resource_get_user_data(surface_resource);
	struct weston_desktop_wl_shell_surface *surface;


	if (weston_surface_set_role(wsurface, "wl_shell_surface", resource, WL_SHELL_ERROR_ROLE) < 0)
		return;

	surface = zalloc(sizeof(struct weston_desktop_wl_shell_surface));
	if (surface == NULL) {
		wl_client_post_no_memory(wl_client);
		return;
	}

	surface->desktop = weston_desktop_client_get_desktop(client);
	surface->display = weston_desktop_get_display(surface->desktop);

	surface->surface =
		weston_desktop_surface_create(surface->desktop, client, wsurface,
					      &weston_desktop_wl_shell_surface_internal_implementation,
					      surface);
	if (surface->surface == NULL) {
		free(surface);
		return;
	}

	surface->wl_surface_resource_destroy_listener.notify =
		wl_surface_resource_destroyed;
	wl_resource_add_destroy_listener(wsurface->resource,
					 &surface->wl_surface_resource_destroy_listener);

	surface->resource =
		weston_desktop_surface_add_resource(surface->surface,
						    &wl_shell_surface_interface,
						    &weston_desktop_wl_shell_surface_implementation,
						    id, NULL);
}


static const struct wl_shell_interface weston_desktop_wl_shell_implementation = {
	.get_shell_surface = weston_desktop_wl_shell_protocol_get_shell_surface,
};

static void
weston_desktop_wl_shell_bind(struct wl_client *client, void *data,
			     uint32_t version, uint32_t id)
{
	struct weston_desktop *desktop = data;

	weston_desktop_client_create(desktop, client, NULL, &wl_shell_interface,
				     &weston_desktop_wl_shell_implementation,
				     version, id);
}

struct wl_global *
weston_desktop_wl_shell_create(struct weston_desktop *desktop,
			       struct wl_display *display)
{
	return wl_global_create(display,
				&wl_shell_interface,
				WD_WL_SHELL_PROTOCOL_VERSION, desktop,
				weston_desktop_wl_shell_bind);
}
