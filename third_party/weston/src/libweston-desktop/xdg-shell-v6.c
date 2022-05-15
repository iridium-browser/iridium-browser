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

#include <stdbool.h>
#include <assert.h>

#include <wayland-server.h>

#include <libweston/libweston.h>
#include <libweston/zalloc.h>
#include "xdg-shell-unstable-v6-server-protocol.h"

#include <libweston-desktop/libweston-desktop.h>
#include "internal.h"

#define WD_XDG_SHELL_PROTOCOL_VERSION 1

static const char *weston_desktop_xdg_toplevel_role = "xdg_toplevel";
static const char *weston_desktop_xdg_popup_role = "xdg_popup";

struct weston_desktop_xdg_positioner {
	struct weston_desktop *desktop;
	struct weston_desktop_client *client;
	struct wl_resource *resource;

	struct weston_size size;
	struct weston_geometry anchor_rect;
	enum zxdg_positioner_v6_anchor anchor;
	enum zxdg_positioner_v6_gravity gravity;
	enum zxdg_positioner_v6_constraint_adjustment constraint_adjustment;
	struct weston_position offset;
};

enum weston_desktop_xdg_surface_role {
	WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE,
	WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL,
	WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP,
};

struct weston_desktop_xdg_surface {
	struct wl_resource *resource;
	struct weston_desktop *desktop;
	struct weston_surface *surface;
	struct weston_desktop_surface *desktop_surface;
	bool configured;
	struct wl_event_source *configure_idle;
	struct wl_list configure_list; /* weston_desktop_xdg_surface_configure::link */

	bool has_next_geometry;
	struct weston_geometry next_geometry;

	enum weston_desktop_xdg_surface_role role;
};

struct weston_desktop_xdg_surface_configure {
	struct wl_list link; /* weston_desktop_xdg_surface::configure_list */
	uint32_t serial;
};

struct weston_desktop_xdg_toplevel_state {
	bool maximized;
	bool fullscreen;
	bool resizing;
	bool activated;
};

struct weston_desktop_xdg_toplevel_configure {
	struct weston_desktop_xdg_surface_configure base;
	struct weston_desktop_xdg_toplevel_state state;
	struct weston_size size;
};

struct weston_desktop_xdg_toplevel {
	struct weston_desktop_xdg_surface base;

	struct wl_resource *resource;
	bool added;
	struct {
		struct weston_desktop_xdg_toplevel_state state;
		struct weston_size size;
	} pending;
	struct {
		struct weston_desktop_xdg_toplevel_state state;
		struct weston_size size;
		struct weston_size min_size, max_size;
	} next;
	struct {
		struct weston_desktop_xdg_toplevel_state state;
		struct weston_size min_size, max_size;
	} current;
};

struct weston_desktop_xdg_popup {
	struct weston_desktop_xdg_surface base;

	struct wl_resource *resource;
	bool committed;
	struct weston_desktop_xdg_surface *parent;
	struct weston_desktop_seat *seat;
	struct weston_geometry geometry;
};

#define weston_desktop_surface_role_biggest_size (sizeof(struct weston_desktop_xdg_toplevel))
#define weston_desktop_surface_configure_biggest_size (sizeof(struct weston_desktop_xdg_toplevel))


static struct weston_geometry
weston_desktop_xdg_positioner_get_geometry(struct weston_desktop_xdg_positioner *positioner,
					   struct weston_desktop_surface *dsurface,
					   struct weston_desktop_surface *parent)
{
	struct weston_geometry geometry = {
		.x = positioner->offset.x,
		.y = positioner->offset.y,
		.width = positioner->size.width,
		.height = positioner->size.height,
	};

	if (positioner->anchor & ZXDG_POSITIONER_V6_ANCHOR_TOP)
		geometry.y += positioner->anchor_rect.y;
	else if (positioner->anchor & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)
		geometry.y += positioner->anchor_rect.y + positioner->anchor_rect.height;
	else
		geometry.y += positioner->anchor_rect.y + positioner->anchor_rect.height / 2;

	if (positioner->anchor & ZXDG_POSITIONER_V6_ANCHOR_LEFT)
		geometry.x += positioner->anchor_rect.x;
	else if (positioner->anchor & ZXDG_POSITIONER_V6_ANCHOR_RIGHT)
		geometry.x += positioner->anchor_rect.x + positioner->anchor_rect.width;
	else
		geometry.x += positioner->anchor_rect.x + positioner->anchor_rect.width / 2;

	if (positioner->gravity & ZXDG_POSITIONER_V6_GRAVITY_TOP)
		geometry.y -= geometry.height;
	else if (positioner->gravity & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)
		geometry.y = geometry.y;
	else
		geometry.y -= geometry.height / 2;

	if (positioner->gravity & ZXDG_POSITIONER_V6_GRAVITY_LEFT)
		geometry.x -= geometry.width;
	else if (positioner->gravity & ZXDG_POSITIONER_V6_GRAVITY_RIGHT)
		geometry.x = geometry.x;
	else
		geometry.x -= geometry.width / 2;

	if (positioner->constraint_adjustment == ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_NONE)
		return geometry;

	/* TODO: add compositor policy configuration and the code here */

	return geometry;
}

static void
weston_desktop_xdg_positioner_protocol_set_size(struct wl_client *wl_client,
						struct wl_resource *resource,
						int32_t width, int32_t height)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	if (width < 1 || height < 1) {
		wl_resource_post_error(resource,
				       ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT,
				       "width and height must be positives and non-zero");
		return;
	}

	positioner->size.width = width;
	positioner->size.height = height;
}

static void
weston_desktop_xdg_positioner_protocol_set_anchor_rect(struct wl_client *wl_client,
						       struct wl_resource *resource,
						       int32_t x, int32_t y,
						       int32_t width, int32_t height)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	if (width < 1 || height < 1) {
		wl_resource_post_error(resource,
				       ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT,
				       "width and height must be positives and non-zero");
		return;
	}

	positioner->anchor_rect.x = x;
	positioner->anchor_rect.y = y;
	positioner->anchor_rect.width = width;
	positioner->anchor_rect.height = height;
}

static void
weston_desktop_xdg_positioner_protocol_set_anchor(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  enum zxdg_positioner_v6_anchor anchor)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	if (((anchor & ZXDG_POSITIONER_V6_ANCHOR_TOP ) &&
	      (anchor & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)) ||
	    ((anchor & ZXDG_POSITIONER_V6_ANCHOR_LEFT) &&
	       (anchor & ZXDG_POSITIONER_V6_ANCHOR_RIGHT))) {
		wl_resource_post_error(resource,
				       ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT,
				       "same-axis values are not allowed");
		return;
	}

	positioner->anchor = anchor;
}

static void
weston_desktop_xdg_positioner_protocol_set_gravity(struct wl_client *wl_client,
						   struct wl_resource *resource,
						   enum zxdg_positioner_v6_gravity gravity)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	if (((gravity & ZXDG_POSITIONER_V6_GRAVITY_TOP) &&
	     (gravity & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)) ||
	    ((gravity & ZXDG_POSITIONER_V6_GRAVITY_LEFT) &&
	     (gravity & ZXDG_POSITIONER_V6_GRAVITY_RIGHT))) {
		wl_resource_post_error(resource,
				       ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT,
				       "same-axis values are not allowed");
		return;
	}

	positioner->gravity = gravity;
}

static void
weston_desktop_xdg_positioner_protocol_set_constraint_adjustment(struct wl_client *wl_client,
								 struct wl_resource *resource,
								 enum zxdg_positioner_v6_constraint_adjustment constraint_adjustment)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	positioner->constraint_adjustment = constraint_adjustment;
}

static void
weston_desktop_xdg_positioner_protocol_set_offset(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  int32_t x, int32_t y)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	positioner->offset.x = x;
	positioner->offset.y = y;
}

static void
weston_desktop_xdg_positioner_destroy(struct wl_resource *resource)
{
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(resource);

	free(positioner);
}

static const struct zxdg_positioner_v6_interface weston_desktop_xdg_positioner_implementation = {
	.destroy                   = weston_desktop_destroy_request,
	.set_size                  = weston_desktop_xdg_positioner_protocol_set_size,
	.set_anchor_rect           = weston_desktop_xdg_positioner_protocol_set_anchor_rect,
	.set_anchor                = weston_desktop_xdg_positioner_protocol_set_anchor,
	.set_gravity               = weston_desktop_xdg_positioner_protocol_set_gravity,
	.set_constraint_adjustment = weston_desktop_xdg_positioner_protocol_set_constraint_adjustment,
	.set_offset                = weston_desktop_xdg_positioner_protocol_set_offset,
};

static void
weston_desktop_xdg_surface_schedule_configure(struct weston_desktop_xdg_surface *surface);

static void
weston_desktop_xdg_toplevel_ensure_added(struct weston_desktop_xdg_toplevel *toplevel)
{
	if (toplevel->added)
		return;

	weston_desktop_api_surface_added(toplevel->base.desktop,
					 toplevel->base.desktop_surface);
	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
	toplevel->added = true;
}

static void
weston_desktop_xdg_toplevel_protocol_set_parent(struct wl_client *wl_client,
						struct wl_resource *resource,
						struct wl_resource *parent_resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_desktop_surface *parent = NULL;

	if (parent_resource != NULL)
		parent = wl_resource_get_user_data(parent_resource);

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_set_parent(toplevel->base.desktop, dsurface, parent);
}

static void
weston_desktop_xdg_toplevel_protocol_set_title(struct wl_client *wl_client,
					       struct wl_resource *resource,
					       const char *title)
{
	struct weston_desktop_surface *toplevel =
		wl_resource_get_user_data(resource);

	weston_desktop_surface_set_title(toplevel, title);
}

static void
weston_desktop_xdg_toplevel_protocol_set_app_id(struct wl_client *wl_client,
						struct wl_resource *resource,
						const char *app_id)
{
	struct weston_desktop_surface *toplevel =
		wl_resource_get_user_data(resource);

	weston_desktop_surface_set_app_id(toplevel, app_id);
}

static void
weston_desktop_xdg_toplevel_protocol_show_window_menu(struct wl_client *wl_client,
						      struct wl_resource *resource,
						      struct wl_resource *seat_resource,
						      uint32_t serial,
						      int32_t x, int32_t y)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *seat =
		wl_resource_get_user_data(seat_resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (!toplevel->base.configured) {
		wl_resource_post_error(toplevel->resource,
				       ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED,
				       "Surface has not been configured yet");
		return;
	}

	if (seat == NULL)
		return;

	weston_desktop_api_show_window_menu(toplevel->base.desktop,
					    dsurface, seat, x, y);
}

static void
weston_desktop_xdg_toplevel_protocol_move(struct wl_client *wl_client,
					  struct wl_resource *resource,
					  struct wl_resource *seat_resource,
					  uint32_t serial)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *seat =
		wl_resource_get_user_data(seat_resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (!toplevel->base.configured) {
		wl_resource_post_error(toplevel->resource,
				       ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED,
				       "Surface has not been configured yet");
		return;
	}

	if (seat == NULL)
		return;

	weston_desktop_api_move(toplevel->base.desktop, dsurface, seat, serial);
}

static void
weston_desktop_xdg_toplevel_protocol_resize(struct wl_client *wl_client,
					    struct wl_resource *resource,
					    struct wl_resource *seat_resource,
					    uint32_t serial,
					    enum zxdg_toplevel_v6_resize_edge edges)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_seat *seat =
		wl_resource_get_user_data(seat_resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);
	enum weston_desktop_surface_edge surf_edges =
		(enum weston_desktop_surface_edge) edges;

	if (!toplevel->base.configured) {
		wl_resource_post_error(toplevel->resource,
				       ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED,
				       "Surface has not been configured yet");
		return;
	}

	if (seat == NULL)
		return;

	weston_desktop_api_resize(toplevel->base.desktop,
				  dsurface, seat, serial, surf_edges);
}

static void
weston_desktop_xdg_toplevel_ack_configure(struct weston_desktop_xdg_toplevel *toplevel,
					  struct weston_desktop_xdg_toplevel_configure *configure)
{
	toplevel->next.state = configure->state;
	toplevel->next.size = configure->size;
}

static void
weston_desktop_xdg_toplevel_protocol_set_min_size(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  int32_t width, int32_t height)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	toplevel->next.min_size.width = width;
	toplevel->next.min_size.height = height;
}

static void
weston_desktop_xdg_toplevel_protocol_set_max_size(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  int32_t width, int32_t height)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	toplevel->next.max_size.width = width;
	toplevel->next.max_size.height = height;
}

static void
weston_desktop_xdg_toplevel_protocol_set_maximized(struct wl_client *wl_client,
						   struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_maximized_requested(toplevel->base.desktop, dsurface, true);
}

static void
weston_desktop_xdg_toplevel_protocol_unset_maximized(struct wl_client *wl_client,
						     struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_maximized_requested(toplevel->base.desktop, dsurface, false);
}

static void
weston_desktop_xdg_toplevel_protocol_set_fullscreen(struct wl_client *wl_client,
						    struct wl_resource *resource,
						    struct wl_resource *output_resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_output *output = NULL;

	if (output_resource != NULL)
		output = weston_head_from_resource(output_resource)->output;

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_fullscreen_requested(toplevel->base.desktop, dsurface,
						true, output);
}

static void
weston_desktop_xdg_toplevel_protocol_unset_fullscreen(struct wl_client *wl_client,
						      struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_fullscreen_requested(toplevel->base.desktop, dsurface,
						false, NULL);
}

static void
weston_desktop_xdg_toplevel_protocol_set_minimized(struct wl_client *wl_client,
						   struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	weston_desktop_xdg_toplevel_ensure_added(toplevel);
	weston_desktop_api_minimized_requested(toplevel->base.desktop, dsurface);
}

static void
weston_desktop_xdg_toplevel_send_configure(struct weston_desktop_xdg_toplevel *toplevel,
					   struct weston_desktop_xdg_toplevel_configure *configure)
{
	uint32_t *s;
	struct wl_array states;

	configure->state = toplevel->pending.state;
	configure->size = toplevel->pending.size;

	wl_array_init(&states);
	if (toplevel->pending.state.maximized) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED;
	}
	if (toplevel->pending.state.fullscreen) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN;
	}
	if (toplevel->pending.state.resizing) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = ZXDG_TOPLEVEL_V6_STATE_RESIZING;
	}
	if (toplevel->pending.state.activated) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = ZXDG_TOPLEVEL_V6_STATE_ACTIVATED;
	}

	zxdg_toplevel_v6_send_configure(toplevel->resource,
					toplevel->pending.size.width,
					toplevel->pending.size.height,
					&states);

	wl_array_release(&states);
};

static void
weston_desktop_xdg_toplevel_set_maximized(struct weston_desktop_surface *dsurface,
					  void *user_data, bool maximized)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	toplevel->pending.state.maximized = maximized;
	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
}

static void
weston_desktop_xdg_toplevel_set_fullscreen(struct weston_desktop_surface *dsurface,
					   void *user_data, bool fullscreen)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	toplevel->pending.state.fullscreen = fullscreen;
	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
}

static void
weston_desktop_xdg_toplevel_set_resizing(struct weston_desktop_surface *dsurface,
					 void *user_data, bool resizing)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	toplevel->pending.state.resizing = resizing;
	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
}

static void
weston_desktop_xdg_toplevel_set_activated(struct weston_desktop_surface *dsurface,
					  void *user_data, bool activated)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	toplevel->pending.state.activated = activated;
	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
}

static void
weston_desktop_xdg_toplevel_set_size(struct weston_desktop_surface *dsurface,
				     void *user_data,
				     int32_t width, int32_t height)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	toplevel->pending.size.width = width;
	toplevel->pending.size.height = height;

	weston_desktop_xdg_surface_schedule_configure(&toplevel->base);
}

static void
weston_desktop_xdg_toplevel_committed(struct weston_desktop_xdg_toplevel *toplevel,
				      int32_t sx, int32_t sy)
{
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(toplevel->base.desktop_surface);

	if (!wsurface->buffer_ref.buffer && !toplevel->added) {
		weston_desktop_xdg_toplevel_ensure_added(toplevel);
		return;
	}
	if (!wsurface->buffer_ref.buffer)
		return;

	struct weston_geometry geometry =
		weston_desktop_surface_get_geometry(toplevel->base.desktop_surface);

	if ((toplevel->next.state.maximized || toplevel->next.state.fullscreen) &&
	    (toplevel->next.size.width != geometry.width ||
	     toplevel->next.size.height != geometry.height)) {
		struct weston_desktop_client *client =
			weston_desktop_surface_get_client(toplevel->base.desktop_surface);
		struct wl_resource *client_resource =
			weston_desktop_client_get_resource(client);

		wl_resource_post_error(client_resource,
				       ZXDG_SHELL_V6_ERROR_INVALID_SURFACE_STATE,
				       "xdg_surface buffer does not match the configured state");
		return;
	}

	toplevel->current.state = toplevel->next.state;
	toplevel->current.min_size = toplevel->next.min_size;
	toplevel->current.max_size = toplevel->next.max_size;

	weston_desktop_api_committed(toplevel->base.desktop,
				     toplevel->base.desktop_surface,
				     sx, sy);
}

static void
weston_desktop_xdg_toplevel_close(struct weston_desktop_xdg_toplevel *toplevel)
{
	zxdg_toplevel_v6_send_close(toplevel->resource);
}

static bool
weston_desktop_xdg_toplevel_get_maximized(struct weston_desktop_surface *dsurface,
					  void *user_data)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	return toplevel->current.state.maximized;
}

static bool
weston_desktop_xdg_toplevel_get_fullscreen(struct weston_desktop_surface *dsurface,
					   void *user_data)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	return toplevel->current.state.fullscreen;
}

static bool
weston_desktop_xdg_toplevel_get_resizing(struct weston_desktop_surface *dsurface,
					 void *user_data)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	return toplevel->current.state.resizing;
}

static bool
weston_desktop_xdg_toplevel_get_activated(struct weston_desktop_surface *dsurface,
					  void *user_data)
{
	struct weston_desktop_xdg_toplevel *toplevel = user_data;

	return toplevel->current.state.activated;
}

static void
weston_desktop_xdg_toplevel_destroy(struct weston_desktop_xdg_toplevel *toplevel)
{
	if (toplevel->added)
		weston_desktop_api_surface_removed(toplevel->base.desktop,
						   toplevel->base.desktop_surface);
}

static void
weston_desktop_xdg_toplevel_resource_destroy(struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);

	if (dsurface != NULL)
		weston_desktop_surface_resource_destroy(resource);
}

static const struct zxdg_toplevel_v6_interface weston_desktop_xdg_toplevel_implementation = {
	.destroy             = weston_desktop_destroy_request,
	.set_parent          = weston_desktop_xdg_toplevel_protocol_set_parent,
	.set_title           = weston_desktop_xdg_toplevel_protocol_set_title,
	.set_app_id          = weston_desktop_xdg_toplevel_protocol_set_app_id,
	.show_window_menu    = weston_desktop_xdg_toplevel_protocol_show_window_menu,
	.move                = weston_desktop_xdg_toplevel_protocol_move,
	.resize              = weston_desktop_xdg_toplevel_protocol_resize,
	.set_min_size        = weston_desktop_xdg_toplevel_protocol_set_min_size,
	.set_max_size        = weston_desktop_xdg_toplevel_protocol_set_max_size,
	.set_maximized       = weston_desktop_xdg_toplevel_protocol_set_maximized,
	.unset_maximized     = weston_desktop_xdg_toplevel_protocol_unset_maximized,
	.set_fullscreen      = weston_desktop_xdg_toplevel_protocol_set_fullscreen,
	.unset_fullscreen    = weston_desktop_xdg_toplevel_protocol_unset_fullscreen,
	.set_minimized       = weston_desktop_xdg_toplevel_protocol_set_minimized,
};

static void
weston_desktop_xdg_popup_protocol_grab(struct wl_client *wl_client,
				       struct wl_resource *resource,
				       struct wl_resource *seat_resource,
				       uint32_t serial)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_popup *popup =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_seat *wseat = wl_resource_get_user_data(seat_resource);
	struct weston_desktop_seat *seat = weston_desktop_seat_from_seat(wseat);
	struct weston_desktop_surface *topmost;
	bool parent_is_toplevel =
		popup->parent->role == WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL;

	/* Check that if we have a valid wseat we also got a valid desktop seat */
	if (wseat != NULL && seat == NULL) {
		wl_client_post_no_memory(wl_client);
		return;
	}

	if (popup->committed) {
		wl_resource_post_error(popup->resource,
				       ZXDG_POPUP_V6_ERROR_INVALID_GRAB,
				       "xdg_popup already is mapped");
		return;
	}

	/* If seat is NULL then get_topmost_surface will return NULL. In
	 * combination with setting parent_is_toplevel to TRUE here we will
	 * avoid posting an error, and we will instead gracefully fail the
	 * grab and dismiss the surface.
	 * FIXME: this is a hack because currently we cannot check the topmost
	 * parent with a destroyed weston_seat */
	if (seat == NULL)
		parent_is_toplevel = true;

	topmost = weston_desktop_seat_popup_grab_get_topmost_surface(seat);
	if ((topmost == NULL && !parent_is_toplevel) ||
	    (topmost != NULL && topmost != popup->parent->desktop_surface)) {
		struct weston_desktop_client *client =
			weston_desktop_surface_get_client(dsurface);
		struct wl_resource *client_resource =
			weston_desktop_client_get_resource(client);

		wl_resource_post_error(client_resource,
				       ZXDG_SHELL_V6_ERROR_NOT_THE_TOPMOST_POPUP,
				       "xdg_popup was not created on the topmost popup");
		return;
	}

	popup->seat = seat;
	weston_desktop_surface_popup_grab(popup->base.desktop_surface,
					  popup->seat, serial);
}

static void
weston_desktop_xdg_popup_send_configure(struct weston_desktop_xdg_popup *popup)
{
	zxdg_popup_v6_send_configure(popup->resource,
				     popup->geometry.x,
				     popup->geometry.y,
				     popup->geometry.width,
				     popup->geometry.height);
}

static void
weston_desktop_xdg_popup_update_position(struct weston_desktop_surface *dsurface,
					 void *user_data);

static void
weston_desktop_xdg_popup_committed(struct weston_desktop_xdg_popup *popup)
{
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface (popup->base.desktop_surface);
	struct weston_view *view;

	wl_list_for_each(view, &wsurface->views, surface_link)
		weston_view_update_transform(view);

	if (!popup->committed)
		weston_desktop_xdg_surface_schedule_configure(&popup->base);
	popup->committed = true;
	weston_desktop_xdg_popup_update_position(popup->base.desktop_surface,
						 popup);
}

static void
weston_desktop_xdg_popup_update_position(struct weston_desktop_surface *dsurface,
					 void *user_data)
{
}

static void
weston_desktop_xdg_popup_close(struct weston_desktop_xdg_popup *popup)
{
	zxdg_popup_v6_send_popup_done(popup->resource);
}

static void
weston_desktop_xdg_popup_destroy(struct weston_desktop_xdg_popup *popup)
{
	struct weston_desktop_surface *topmost;
	struct weston_desktop_client *client =
		weston_desktop_surface_get_client(popup->base.desktop_surface);

	if (!weston_desktop_surface_get_grab(popup->base.desktop_surface))
		return;

	topmost = weston_desktop_seat_popup_grab_get_topmost_surface(popup->seat);
	if (topmost != popup->base.desktop_surface) {
		struct wl_resource *client_resource =
			weston_desktop_client_get_resource(client);

		wl_resource_post_error(client_resource,
				       ZXDG_SHELL_V6_ERROR_NOT_THE_TOPMOST_POPUP,
				       "xdg_popup was destroyed while it was not the topmost popup.");
	}

	weston_desktop_surface_popup_ungrab(popup->base.desktop_surface,
					    popup->seat);
}

static void
weston_desktop_xdg_popup_resource_destroy(struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);

	if (dsurface != NULL)
		weston_desktop_surface_resource_destroy(resource);
}

static const struct zxdg_popup_v6_interface weston_desktop_xdg_popup_implementation = {
	.destroy             = weston_desktop_destroy_request,
	.grab                = weston_desktop_xdg_popup_protocol_grab,
};

static void
weston_desktop_xdg_surface_send_configure(void *user_data)
{
	struct weston_desktop_xdg_surface *surface = user_data;
	struct weston_desktop_xdg_surface_configure *configure;

	surface->configure_idle = NULL;

	configure = zalloc(weston_desktop_surface_configure_biggest_size);
	if (configure == NULL) {
		struct weston_desktop_client *client =
			weston_desktop_surface_get_client(surface->desktop_surface);
		struct wl_client *wl_client =
			weston_desktop_client_get_client(client);
		wl_client_post_no_memory(wl_client);
		return;
	}
	wl_list_insert(surface->configure_list.prev, &configure->link);
	configure->serial =
		wl_display_next_serial(weston_desktop_get_display(surface->desktop));

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		assert(0 && "not reached");
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		weston_desktop_xdg_toplevel_send_configure((struct weston_desktop_xdg_toplevel *) surface,
							   (struct weston_desktop_xdg_toplevel_configure *) configure);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		weston_desktop_xdg_popup_send_configure((struct weston_desktop_xdg_popup *) surface);
		break;
	}

	zxdg_surface_v6_send_configure(surface->resource, configure->serial);
}

static bool
weston_desktop_xdg_toplevel_state_compare(struct weston_desktop_xdg_toplevel *toplevel)
{
	struct {
		struct weston_desktop_xdg_toplevel_state state;
		struct weston_size size;
	} configured;

	if (!toplevel->base.configured)
		return false;

	if (wl_list_empty(&toplevel->base.configure_list)) {
		/* Last configure is actually the current state, just use it */
		configured.state = toplevel->current.state;
		configured.size.width = toplevel->base.surface->width;
		configured.size.height = toplevel->base.surface->height;
	} else {
		struct weston_desktop_xdg_toplevel_configure *configure =
			wl_container_of(toplevel->base.configure_list.prev,
					configure, base.link);

		configured.state = configure->state;
		configured.size = configure->size;
	}

	if (toplevel->pending.state.activated != configured.state.activated)
		return false;
	if (toplevel->pending.state.fullscreen != configured.state.fullscreen)
		return false;
	if (toplevel->pending.state.maximized != configured.state.maximized)
		return false;
	if (toplevel->pending.state.resizing != configured.state.resizing)
		return false;

	if (toplevel->pending.size.width == configured.size.width &&
	    toplevel->pending.size.height == configured.size.height)
		return true;

	if (toplevel->pending.size.width == 0 &&
	    toplevel->pending.size.height == 0)
		return true;

	return false;
}

static void
weston_desktop_xdg_surface_schedule_configure(struct weston_desktop_xdg_surface *surface)
{
	struct wl_display *display = weston_desktop_get_display(surface->desktop);
	struct wl_event_loop *loop = wl_display_get_event_loop(display);
	bool pending_same = false;

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		assert(0 && "not reached");
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		pending_same = weston_desktop_xdg_toplevel_state_compare((struct weston_desktop_xdg_toplevel *) surface);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		break;
	}

	if (surface->configure_idle != NULL) {
		if (!pending_same)
			return;

		wl_event_source_remove(surface->configure_idle);
		surface->configure_idle = NULL;
	} else {
		if (pending_same)
			return;

		surface->configure_idle =
			wl_event_loop_add_idle(loop,
					       weston_desktop_xdg_surface_send_configure,
					       surface);
	}
}

static void
weston_desktop_xdg_surface_protocol_get_toplevel(struct wl_client *wl_client,
						 struct wl_resource *resource,
						 uint32_t id)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(dsurface);
	struct weston_desktop_xdg_toplevel *toplevel =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (weston_surface_set_role(wsurface, weston_desktop_xdg_toplevel_role,
				    resource, ZXDG_SHELL_V6_ERROR_ROLE) < 0)
		return;

	toplevel->resource =
		weston_desktop_surface_add_resource(toplevel->base.desktop_surface,
						    &zxdg_toplevel_v6_interface,
						    &weston_desktop_xdg_toplevel_implementation,
						    id, weston_desktop_xdg_toplevel_resource_destroy);
	if (toplevel->resource == NULL)
		return;

	toplevel->base.role = WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL;
}

static void
weston_desktop_xdg_surface_protocol_get_popup(struct wl_client *wl_client,
					      struct wl_resource *resource,
					      uint32_t id,
					      struct wl_resource *parent_resource,
					      struct wl_resource *positioner_resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(dsurface);
	struct weston_desktop_xdg_popup *popup =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_desktop_surface *parent_surface =
		wl_resource_get_user_data(parent_resource);
	struct weston_desktop_xdg_surface *parent =
		weston_desktop_surface_get_implementation_data(parent_surface);
	struct weston_desktop_xdg_positioner *positioner =
		wl_resource_get_user_data(positioner_resource);

	/* Checking whether the size and anchor rect both have a positive size
	 * is enough to verify both have been correctly set */
	if (positioner->size.width == 0 || positioner->anchor_rect.width == 0) {
		wl_resource_post_error(resource,
				       ZXDG_SHELL_V6_ERROR_INVALID_POSITIONER,
				       "positioner object is not complete");
		return;
	}

	if (weston_surface_set_role(wsurface, weston_desktop_xdg_popup_role,
				    resource, ZXDG_SHELL_V6_ERROR_ROLE) < 0)
		return;

	popup->resource =
		weston_desktop_surface_add_resource(popup->base.desktop_surface,
						    &zxdg_popup_v6_interface,
						    &weston_desktop_xdg_popup_implementation,
						    id, weston_desktop_xdg_popup_resource_destroy);
	if (popup->resource == NULL)
		return;

	popup->base.role = WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP;
	popup->parent = parent;

	popup->geometry =
		weston_desktop_xdg_positioner_get_geometry(positioner,
							   dsurface,
							   parent_surface);

	weston_desktop_surface_set_relative_to(popup->base.desktop_surface,
					       parent_surface,
					       popup->geometry.x,
					       popup->geometry.y,
					       true);
}

static bool
weston_desktop_xdg_surface_check_role(struct weston_desktop_xdg_surface *surface)
{
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(surface->desktop_surface);
	const char *role;

	role = weston_surface_get_role(wsurface);
	if (role != NULL &&
	    (role == weston_desktop_xdg_toplevel_role ||
	     role == weston_desktop_xdg_popup_role))
	     return true;

	wl_resource_post_error(surface->resource,
			       ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED,
			       "xdg_surface must have a role");
	return false;
}

static void
weston_desktop_xdg_surface_protocol_set_window_geometry(struct wl_client *wl_client,
							struct wl_resource *resource,
							int32_t x, int32_t y,
							int32_t width, int32_t height)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);

	if (!weston_desktop_xdg_surface_check_role(surface))
		return;

	surface->has_next_geometry = true;
	surface->next_geometry.x = x;
	surface->next_geometry.y = y;
	surface->next_geometry.width = width;
	surface->next_geometry.height = height;
}

static void
weston_desktop_xdg_surface_protocol_ack_configure(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  uint32_t serial)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_surface *surface =
		weston_desktop_surface_get_implementation_data(dsurface);
	struct weston_desktop_xdg_surface_configure *configure, *temp;
	bool found = false;

	if (!weston_desktop_xdg_surface_check_role(surface))
		return;

	wl_list_for_each_safe(configure, temp, &surface->configure_list, link) {
		if (configure->serial < serial) {
			wl_list_remove(&configure->link);
			free(configure);
		} else if (configure->serial == serial) {
			wl_list_remove(&configure->link);
			found = true;
			break;
		} else {
			break;
		}
	}
	if (!found) {
		struct weston_desktop_client *client =
			weston_desktop_surface_get_client(dsurface);
		struct wl_resource *client_resource =
			weston_desktop_client_get_resource(client);
		wl_resource_post_error(client_resource,
				       ZXDG_SHELL_V6_ERROR_INVALID_SURFACE_STATE,
				       "Wrong configure serial: %u", serial);
		return;
	}

	surface->configured = true;

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		assert(0 && "not reached");
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		weston_desktop_xdg_toplevel_ack_configure((struct weston_desktop_xdg_toplevel *) surface,
							  (struct weston_desktop_xdg_toplevel_configure *) configure);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		break;
	}

	free(configure);
}

static void
weston_desktop_xdg_surface_ping(struct weston_desktop_surface *dsurface,
				uint32_t serial, void *user_data)
{
	struct weston_desktop_client *client =
		weston_desktop_surface_get_client(dsurface);

	zxdg_shell_v6_send_ping(weston_desktop_client_get_resource(client),
				serial);
}

static void
weston_desktop_xdg_surface_committed(struct weston_desktop_surface *dsurface,
				     void *user_data,
				     int32_t sx, int32_t sy)
{
	struct weston_desktop_xdg_surface *surface = user_data;
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface (dsurface);

	if (wsurface->buffer_ref.buffer && !surface->configured) {
		wl_resource_post_error(surface->resource,
				       ZXDG_SURFACE_V6_ERROR_UNCONFIGURED_BUFFER,
				       "xdg_surface has never been configured");
		return;
	}

	if (surface->has_next_geometry) {
		surface->has_next_geometry = false;
		weston_desktop_surface_set_geometry(surface->desktop_surface,
						    surface->next_geometry);
	}

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		wl_resource_post_error(surface->resource,
				       ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED,
				       "xdg_surface must have a role");
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		weston_desktop_xdg_toplevel_committed((struct weston_desktop_xdg_toplevel *) surface, sx, sy);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		weston_desktop_xdg_popup_committed((struct weston_desktop_xdg_popup *) surface);
		break;
	}
}

static void
weston_desktop_xdg_surface_close(struct weston_desktop_surface *dsurface,
				 void *user_data)
{
	struct weston_desktop_xdg_surface *surface = user_data;

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		assert(0 && "not reached");
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		weston_desktop_xdg_toplevel_close((struct weston_desktop_xdg_toplevel *) surface);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		weston_desktop_xdg_popup_close((struct weston_desktop_xdg_popup *) surface);
		break;
	}
}

static void
weston_desktop_xdg_surface_destroy(struct weston_desktop_surface *dsurface,
				   void *user_data)
{
	struct weston_desktop_xdg_surface *surface = user_data;
	struct weston_desktop_xdg_surface_configure *configure, *temp;

	switch (surface->role) {
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_NONE:
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_TOPLEVEL:
		weston_desktop_xdg_toplevel_destroy((struct weston_desktop_xdg_toplevel *) surface);
		break;
	case WESTON_DESKTOP_XDG_SURFACE_ROLE_POPUP:
		weston_desktop_xdg_popup_destroy((struct weston_desktop_xdg_popup *) surface);
		break;
	}

	if (surface->configure_idle != NULL)
		wl_event_source_remove(surface->configure_idle);

	wl_list_for_each_safe(configure, temp, &surface->configure_list, link)
		free(configure);

	free(surface);
}

static const struct zxdg_surface_v6_interface weston_desktop_xdg_surface_implementation = {
	.destroy             = weston_desktop_destroy_request,
	.get_toplevel        = weston_desktop_xdg_surface_protocol_get_toplevel,
	.get_popup           = weston_desktop_xdg_surface_protocol_get_popup,
	.set_window_geometry = weston_desktop_xdg_surface_protocol_set_window_geometry,
	.ack_configure       = weston_desktop_xdg_surface_protocol_ack_configure,
};

static const struct weston_desktop_surface_implementation weston_desktop_xdg_surface_internal_implementation = {
	/* These are used for toplevel only */
	.set_maximized = weston_desktop_xdg_toplevel_set_maximized,
	.set_fullscreen = weston_desktop_xdg_toplevel_set_fullscreen,
	.set_resizing = weston_desktop_xdg_toplevel_set_resizing,
	.set_activated = weston_desktop_xdg_toplevel_set_activated,
	.set_size = weston_desktop_xdg_toplevel_set_size,

	.get_maximized = weston_desktop_xdg_toplevel_get_maximized,
	.get_fullscreen = weston_desktop_xdg_toplevel_get_fullscreen,
	.get_resizing = weston_desktop_xdg_toplevel_get_resizing,
	.get_activated = weston_desktop_xdg_toplevel_get_activated,

	/* These are used for popup only */
	.update_position = weston_desktop_xdg_popup_update_position,

	/* Common API */
	.committed = weston_desktop_xdg_surface_committed,
	.ping = weston_desktop_xdg_surface_ping,
	.close = weston_desktop_xdg_surface_close,

	.destroy = weston_desktop_xdg_surface_destroy,
};

static void
weston_desktop_xdg_shell_protocol_create_positioner(struct wl_client *wl_client,
						    struct wl_resource *resource,
						    uint32_t id)
{
	struct weston_desktop_client *client =
		wl_resource_get_user_data(resource);
	struct weston_desktop_xdg_positioner *positioner;

	positioner = zalloc(sizeof(struct weston_desktop_xdg_positioner));
	if (positioner == NULL) {
		wl_client_post_no_memory(wl_client);
		return;
	}

	positioner->client = client;
	positioner->desktop = weston_desktop_client_get_desktop(positioner->client);

	positioner->resource =
		wl_resource_create(wl_client,
				   &zxdg_positioner_v6_interface,
				   wl_resource_get_version(resource), id);
	if (positioner->resource == NULL) {
		wl_client_post_no_memory(wl_client);
		free(positioner);
		return;
	}
	wl_resource_set_implementation(positioner->resource,
				       &weston_desktop_xdg_positioner_implementation,
				       positioner, weston_desktop_xdg_positioner_destroy);
}

static void
weston_desktop_xdg_surface_resource_destroy(struct wl_resource *resource)
{
	struct weston_desktop_surface *dsurface =
		wl_resource_get_user_data(resource);

	if (dsurface != NULL)
		weston_desktop_surface_resource_destroy(resource);
}

static void
weston_desktop_xdg_shell_protocol_get_xdg_surface(struct wl_client *wl_client,
						  struct wl_resource *resource,
						  uint32_t id,
						  struct wl_resource *surface_resource)
{
	struct weston_desktop_client *client =
		wl_resource_get_user_data(resource);
	struct weston_surface *wsurface =
		wl_resource_get_user_data(surface_resource);
	struct weston_desktop_xdg_surface *surface;

	surface = zalloc(weston_desktop_surface_role_biggest_size);
	if (surface == NULL) {
		wl_client_post_no_memory(wl_client);
		return;
	}

	surface->desktop = weston_desktop_client_get_desktop(client);
	surface->surface = wsurface;
	wl_list_init(&surface->configure_list);

	surface->desktop_surface =
		weston_desktop_surface_create(surface->desktop, client,
					      surface->surface,
					      &weston_desktop_xdg_surface_internal_implementation,
					      surface);
	if (surface->desktop_surface == NULL) {
		free(surface);
		return;
	}

	surface->resource =
		weston_desktop_surface_add_resource(surface->desktop_surface,
						    &zxdg_surface_v6_interface,
						    &weston_desktop_xdg_surface_implementation,
						    id, weston_desktop_xdg_surface_resource_destroy);
	if (surface->resource == NULL)
		return;

	if (wsurface->buffer_ref.buffer != NULL) {
		wl_resource_post_error(surface->resource,
				       ZXDG_SURFACE_V6_ERROR_UNCONFIGURED_BUFFER,
				       "xdg_surface must not have a buffer at creation");
		return;
	}
}

static void
weston_desktop_xdg_shell_protocol_pong(struct wl_client *wl_client,
				       struct wl_resource *resource,
				       uint32_t serial)
{
	struct weston_desktop_client *client =
		wl_resource_get_user_data(resource);

	weston_desktop_client_pong(client, serial);
}

static const struct zxdg_shell_v6_interface weston_desktop_xdg_shell_implementation = {
	.destroy = weston_desktop_destroy_request,
	.create_positioner = weston_desktop_xdg_shell_protocol_create_positioner,
	.get_xdg_surface = weston_desktop_xdg_shell_protocol_get_xdg_surface,
	.pong = weston_desktop_xdg_shell_protocol_pong,
};

static void
weston_desktop_xdg_shell_bind(struct wl_client *client, void *data,
			      uint32_t version, uint32_t id)
{
	struct weston_desktop *desktop = data;

	weston_desktop_client_create(desktop, client, NULL,
				     &zxdg_shell_v6_interface,
				     &weston_desktop_xdg_shell_implementation,
				     version, id);
}

struct wl_global *
weston_desktop_xdg_shell_v6_create(struct weston_desktop *desktop, struct wl_display *display)
{
	return wl_global_create(display, &zxdg_shell_v6_interface,
				WD_XDG_SHELL_PROTOCOL_VERSION, desktop,
				weston_desktop_xdg_shell_bind);
}
