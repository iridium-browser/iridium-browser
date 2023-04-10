/*
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

#ifndef WESTON_DESKTOP_INTERNAL_H
#define WESTON_DESKTOP_INTERNAL_H

#include <libweston/libweston.h>

struct weston_desktop_seat;
struct weston_desktop_client;

struct weston_compositor *
weston_desktop_get_compositor(struct weston_desktop *desktop);
struct wl_display *
weston_desktop_get_display(struct weston_desktop *desktop);

void
weston_desktop_api_ping_timeout(struct weston_desktop *desktop,
				struct weston_desktop_client *client);
void
weston_desktop_api_pong(struct weston_desktop *desktop,
			struct weston_desktop_client *client);
void
weston_desktop_api_surface_added(struct weston_desktop *desktop,
				 struct weston_desktop_surface *surface);
void
weston_desktop_api_surface_removed(struct weston_desktop *desktop,
				   struct weston_desktop_surface *surface);
void
weston_desktop_api_committed(struct weston_desktop *desktop,
			     struct weston_desktop_surface *surface,
			     int32_t sx, int32_t sy);
void
weston_desktop_api_show_window_menu(struct weston_desktop *desktop,
				    struct weston_desktop_surface *surface,
				    struct weston_seat *seat,
				    int32_t x, int32_t y);
void
weston_desktop_api_set_parent(struct weston_desktop *desktop,
			      struct weston_desktop_surface *surface,
			      struct weston_desktop_surface *parent);
void
weston_desktop_api_move(struct weston_desktop *desktop,
			struct weston_desktop_surface *surface,
			struct weston_seat *seat, uint32_t serial);
void
weston_desktop_api_resize(struct weston_desktop *desktop,
			  struct weston_desktop_surface *surface,
			  struct weston_seat *seat, uint32_t serial,
			  enum weston_desktop_surface_edge edges);
void
weston_desktop_api_fullscreen_requested(struct weston_desktop *desktop,
					struct weston_desktop_surface *surface,
					bool fullscreen,
					struct weston_output *output);
void
weston_desktop_api_maximized_requested(struct weston_desktop *desktop,
				       struct weston_desktop_surface *surface,
				       bool maximized);
void
weston_desktop_api_minimized_requested(struct weston_desktop *desktop,
				       struct weston_desktop_surface *surface);

void
weston_desktop_api_set_xwayland_position(struct weston_desktop *desktop,
					 struct weston_desktop_surface *surface,
					 int32_t x, int32_t y);
void
weston_desktop_api_get_desktop_surface_root_geometry(struct weston_desktop *desktop,
						     struct weston_desktop_surface *surface,
						     struct weston_geometry *geometry);

struct weston_desktop_seat *
weston_desktop_seat_from_seat(struct weston_seat *wseat);

struct weston_desktop_surface_implementation {
	void (*set_activated)(struct weston_desktop_surface *surface,
			      void *user_data, bool activated);
	void (*set_fullscreen)(struct weston_desktop_surface *surface,
			       void *user_data, bool fullscreen);
	void (*set_maximized)(struct weston_desktop_surface *surface,
			      void *user_data, bool maximized);
	void (*set_resizing)(struct weston_desktop_surface *surface,
			     void *user_data, bool resizing);
	void (*set_size)(struct weston_desktop_surface *surface,
			 void *user_data, int32_t width, int32_t height);
	void (*committed)(struct weston_desktop_surface *surface, void *user_data,
		          int32_t sx, int32_t sy);
	void (*update_position)(struct weston_desktop_surface *surface,
				void *user_data);
	void (*ping)(struct weston_desktop_surface *surface, uint32_t serial,
		     void *user_data);
	void (*close)(struct weston_desktop_surface *surface, void *user_data);

	bool (*get_activated)(struct weston_desktop_surface *surface,
			      void *user_data);
	bool (*get_fullscreen)(struct weston_desktop_surface *surface,
			       void *user_data);
	bool (*get_maximized)(struct weston_desktop_surface *surface,
			      void *user_data);
	bool (*get_resizing)(struct weston_desktop_surface *surface,
			     void *user_data);
	struct weston_size
	(*get_max_size)(struct weston_desktop_surface *surface,
			void *user_data);
	struct weston_size
	(*get_min_size)(struct weston_desktop_surface *surface,
			void *user_data);

	void (*destroy)(struct weston_desktop_surface *surface,
			void *user_data);
};

struct weston_desktop_client *
weston_desktop_client_create(struct weston_desktop *desktop,
			     struct wl_client *client,
			     wl_dispatcher_func_t dispatcher,
			     const struct wl_interface *interface,
			     const void *implementation, uint32_t version,
			     uint32_t id);

void
weston_desktop_client_add_destroy_listener(struct weston_desktop_client *client,
					   struct wl_listener *listener);
struct weston_desktop *
weston_desktop_client_get_desktop(struct weston_desktop_client *client);
struct wl_resource *
weston_desktop_client_get_resource(struct weston_desktop_client *client);
struct wl_list *
weston_desktop_client_get_surface_list(struct weston_desktop_client *client);

void
weston_desktop_client_pong(struct weston_desktop_client *client,
			   uint32_t serial);

struct weston_desktop_surface *
weston_desktop_surface_create(struct weston_desktop *desktop,
			      struct weston_desktop_client *client,
			      struct weston_surface *surface,
			      const struct weston_desktop_surface_implementation *implementation,
			      void *implementation_data);
void
weston_desktop_surface_destroy(struct weston_desktop_surface *surface);
void
weston_desktop_surface_resource_destroy(struct wl_resource *resource);
struct wl_resource *
weston_desktop_surface_add_resource(struct weston_desktop_surface *surface,
				    const struct wl_interface *interface,
				    const void *implementation, uint32_t id,
				    wl_resource_destroy_func_t destroy);
struct weston_desktop_surface *
weston_desktop_surface_from_grab_link(struct wl_list *grab_link);

struct wl_list *
weston_desktop_surface_get_client_link(struct weston_desktop_surface *surface);
struct weston_desktop_surface *
weston_desktop_surface_from_client_link(struct wl_list *link);
bool
weston_desktop_surface_has_implementation(struct weston_desktop_surface *surface,
					  const struct weston_desktop_surface_implementation *implementation);
const struct weston_desktop_surface_implementation *
weston_desktop_surface_get_implementation(struct weston_desktop_surface *surface);
void *
weston_desktop_surface_get_implementation_data(struct weston_desktop_surface *surface);
struct weston_desktop_surface *
weston_desktop_surface_get_parent(struct weston_desktop_surface *surface);
bool
weston_desktop_surface_get_grab(struct weston_desktop_surface *surface);

void
weston_desktop_surface_set_title(struct weston_desktop_surface *surface,
				 const char *title);
void
weston_desktop_surface_set_app_id(struct weston_desktop_surface *surface,
				  const char *app_id);
void
weston_desktop_surface_set_pid(struct weston_desktop_surface *surface,
			       pid_t pid);
void
weston_desktop_surface_set_geometry(struct weston_desktop_surface *surface,
				    struct weston_geometry geometry);
void
weston_desktop_surface_set_relative_to(struct weston_desktop_surface *surface,
				       struct weston_desktop_surface *parent,
				       int32_t x, int32_t y, bool use_geometry);
void
weston_desktop_surface_unset_relative_to(struct weston_desktop_surface *surface);
void
weston_desktop_surface_popup_grab(struct weston_desktop_surface *popup,
				  struct weston_desktop_seat *seat,
				  uint32_t serial);
void
weston_desktop_surface_popup_ungrab(struct weston_desktop_surface *popup,
				    struct weston_desktop_seat *seat);
void
weston_desktop_surface_popup_dismiss(struct weston_desktop_surface *surface);

struct weston_desktop_surface *
weston_desktop_seat_popup_grab_get_topmost_surface(struct weston_desktop_seat *seat);
bool
weston_desktop_seat_popup_grab_start(struct weston_desktop_seat *seat,
				     struct wl_client *client, uint32_t serial);
void
weston_desktop_seat_popup_grab_add_surface(struct weston_desktop_seat *seat,
					   struct wl_list *link);
void
weston_desktop_seat_popup_grab_remove_surface(struct weston_desktop_seat *seat,
					      struct wl_list *link);

void
weston_desktop_destroy_request(struct wl_client *client,
			       struct wl_resource *resource);
struct wl_global *
weston_desktop_xdg_wm_base_create(struct weston_desktop *desktop,
				  struct wl_display *display);
struct wl_global *
weston_desktop_xdg_shell_v6_create(struct weston_desktop *desktop,
				   struct wl_display *display);
struct wl_global *
weston_desktop_wl_shell_create(struct weston_desktop *desktop,
			       struct wl_display *display);

void
weston_desktop_xwayland_init(struct weston_desktop *desktop);

#endif /* WESTON_DESKTOP_INTERNAL_H */
