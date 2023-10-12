/*
 * Copyright (C) 2013 DENSO CORPORATION
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

/*
 * ivi-shell supports a type of shell for In-Vehicle Infotainment system.
 * In-Vehicle Infotainment system traditionally manages surfaces with global
 * identification. A protocol, ivi_application, supports such a feature
 * by implementing a request, ivi_application::surface_creation defined in
 * ivi_application.xml.
 *
 *  The ivi-shell explicitly loads a module to add business logic like how to
 *  layout surfaces by using internal ivi-layout APIs.
 */
#include "config.h"

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>
#include <assert.h>
#include <linux/input.h>

#include "input-method-unstable-v1-server-protocol.h"
#include "ivi-shell.h"
#include "ivi-application-server-protocol.h"
#include "ivi-layout-private.h"
#include "ivi-layout-shell.h"
#include "libweston/libweston.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "compositor/weston.h"

/* Representation of ivi_surface protocol object. */
struct ivi_shell_surface
{
	struct wl_resource* resource;
	struct ivi_shell *shell;
	struct ivi_layout_surface *layout_surface;

	struct weston_surface *surface;
	struct wl_listener surface_destroy_listener;

	uint32_t id_surface;

	int32_t width;
	int32_t height;

	struct wl_list children_list;
	struct wl_list children_link;

	struct wl_list link;
};

struct ivi_input_panel_surface
{
	struct wl_resource* resource;
	struct ivi_shell *shell;
	struct ivi_layout_surface *layout_surface;

	struct weston_surface *surface;
	struct wl_listener surface_destroy_listener;

	int32_t width;
	int32_t height;

	struct weston_output *output;
	enum {
		INPUT_PANEL_NONE,
		INPUT_PANEL_TOPLEVEL,
		INPUT_PANEL_OVERLAY,
	} type;

	struct wl_list link;
};

/*
 * Implementation of ivi_surface
 */

static void
ivi_shell_surface_committed(struct weston_surface *, struct weston_coord_surface);

static struct ivi_shell_surface *
get_ivi_shell_surface(struct weston_surface *surface)
{
	struct weston_desktop_surface *desktop_surface
		= weston_surface_get_desktop_surface(surface);
	if (desktop_surface)
		return weston_desktop_surface_get_user_data(desktop_surface);

	if (surface->committed == ivi_shell_surface_committed)
		return surface->committed_private;

	return NULL;
}

struct ivi_layout_surface *
shell_get_ivi_layout_surface(struct weston_surface *surface)
{
	struct ivi_shell_surface *shsurf;

	shsurf = get_ivi_shell_surface(surface);
	if (!shsurf)
		return NULL;

	return shsurf->layout_surface;
}

void
shell_surface_send_configure(struct weston_surface *surface,
			     int32_t width, int32_t height)
{
	struct ivi_shell_surface *shsurf;

	shsurf = get_ivi_shell_surface(surface);
	assert(shsurf);

	if (shsurf->resource)
		ivi_surface_send_configure(shsurf->resource, width, height);
}

static void
ivi_shell_surface_committed(struct weston_surface *surface,
			    struct weston_coord_surface new_origin)
{
	struct ivi_shell_surface *ivisurf = get_ivi_shell_surface(surface);

	if (surface->width == 0 || surface->height == 0) {
		if (!weston_surface_is_unmapping(surface))
			return;
	}

	if (ivisurf->width != surface->width ||
	    ivisurf->height != surface->height) {
		ivisurf->width  = surface->width;
		ivisurf->height = surface->height;

		ivi_layout_surface_configure(ivisurf->layout_surface,
					     surface->width, surface->height);
	}
}

static int
ivi_shell_surface_get_label(struct weston_surface *surface,
			    char *buf,
			    size_t len)
{
	struct ivi_shell_surface *shell_surf = get_ivi_shell_surface(surface);

	return snprintf(buf, len, "ivi-surface %#x", shell_surf->id_surface);
}

static void
layout_surface_cleanup(struct ivi_shell_surface *ivisurf)
{
	assert(ivisurf->layout_surface != NULL);

	/* destroy weston_surface destroy signal. */
	if (!ivisurf->layout_surface->weston_desktop_surface)
		wl_list_remove(&ivisurf->surface_destroy_listener.link);

	ivi_layout_surface_destroy(ivisurf->layout_surface);
	ivisurf->layout_surface = NULL;

	ivisurf->surface->committed = NULL;
	ivisurf->surface->committed_private = NULL;
	weston_surface_set_label_func(ivisurf->surface, NULL);
	ivisurf->surface = NULL;
}

/*
 * The ivi_surface wl_resource destructor.
 *
 * Gets called via ivi_surface.destroy request or automatic wl_client clean-up.
 */
static void
shell_destroy_shell_surface(struct wl_resource *resource)
{
	struct ivi_shell_surface *ivisurf = wl_resource_get_user_data(resource);

	if (ivisurf == NULL)
		return;

	assert(ivisurf->resource == resource);

	if (ivisurf->layout_surface != NULL)
		layout_surface_cleanup(ivisurf);

	wl_list_remove(&ivisurf->link);

	free(ivisurf);
}

/* Gets called through the weston_surface destroy signal. */
static void
shell_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct ivi_shell_surface *ivisurf =
			container_of(listener, struct ivi_shell_surface,
				     surface_destroy_listener);

	assert(ivisurf != NULL);

	if (ivisurf->layout_surface != NULL)
		layout_surface_cleanup(ivisurf);
}

/* Gets called, when a client sends ivi_surface.destroy request. */
static void
surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	/*
	 * Fires the wl_resource destroy signal, and then calls
	 * ivi_surface wl_resource destructor: shell_destroy_shell_surface()
	 */
	wl_resource_destroy(resource);
}

static const struct ivi_surface_interface surface_implementation = {
	surface_destroy,
};

/**
 * Request handler for ivi_application.surface_create.
 *
 * Creates an ivi_surface protocol object associated with the given wl_surface.
 * ivi_surface protocol object is represented by struct ivi_shell_surface.
 *
 * \param client The client.
 * \param resource The ivi_application protocol object.
 * \param id_surface The IVI surface ID.
 * \param surface_resource The wl_surface protocol object.
 * \param id The protocol object id for the new ivi_surface protocol object.
 *
 * The wl_surface is given the ivi_surface role and associated with a unique
 * IVI ID which is used to identify the surface in a controller
 * (window manager).
 */
static void
application_surface_create(struct wl_client *client,
			   struct wl_resource *resource,
			   uint32_t id_surface,
			   struct wl_resource *surface_resource,
			   uint32_t id)
{
	struct ivi_shell *shell = wl_resource_get_user_data(resource);
	struct ivi_shell_surface *ivisurf;
	struct ivi_layout_surface *layout_surface;
	struct weston_surface *weston_surface =
		wl_resource_get_user_data(surface_resource);
	struct wl_resource *res;

	if (weston_surface_set_role(weston_surface, "ivi_surface",
				    resource, IVI_APPLICATION_ERROR_ROLE) < 0)
		return;

	layout_surface = ivi_layout_surface_create(weston_surface, id_surface);

	/* check if id_ivi is already used for wl_surface*/
	if (layout_surface == NULL) {
		wl_resource_post_error(resource,
				       IVI_APPLICATION_ERROR_IVI_ID,
				       "surface_id is already assigned "
				       "by another app");
		return;
	}

	layout_surface->weston_desktop_surface = NULL;

	ivisurf = xzalloc(sizeof *ivisurf);

	wl_list_init(&ivisurf->link);
	wl_list_insert(&shell->ivi_surface_list, &ivisurf->link);

	ivisurf->shell = shell;
	ivisurf->id_surface = id_surface;

	ivisurf->width = 0;
	ivisurf->height = 0;
	ivisurf->layout_surface = layout_surface;

	/*
	 * initialize list as well as link. The latter allows to use
	 * wl_list_remove() event when this surface is not in another list.
	 */
	wl_list_init(&ivisurf->children_list);
	wl_list_init(&ivisurf->children_link);

	/*
	 * The following code relies on wl_surface destruction triggering
	 * immediateweston_surface destruction
	 */
	ivisurf->surface_destroy_listener.notify = shell_handle_surface_destroy;
	wl_signal_add(&weston_surface->destroy_signal,
		      &ivisurf->surface_destroy_listener);

	ivisurf->surface = weston_surface;

	weston_surface->committed = ivi_shell_surface_committed;
	weston_surface->committed_private = ivisurf;
	weston_surface_set_label_func(weston_surface,
				      ivi_shell_surface_get_label);

	res = wl_resource_create(client, &ivi_surface_interface, 1, id);
	if (res == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	ivisurf->resource = res;

	wl_resource_set_implementation(res, &surface_implementation,
				       ivisurf, shell_destroy_shell_surface);
}

static const struct ivi_application_interface application_implementation = {
	application_surface_create
};

/*
 * Handle wl_registry.bind of ivi_application global singleton.
 */
static void
bind_ivi_application(struct wl_client *client,
		     void *data, uint32_t version, uint32_t id)
{
	struct ivi_shell *shell = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &ivi_application_interface,
				      1, id);

	wl_resource_set_implementation(resource,
				       &application_implementation,
				       shell, NULL);
}

void
input_panel_destroy(struct ivi_shell *shell);

/*
 * Called through the compositor's destroy signal.
 */
static void
shell_destroy(struct wl_listener *listener, void *data)
{
	struct ivi_shell *shell =
		container_of(listener, struct ivi_shell, destroy_listener);
	struct ivi_shell_surface *ivisurf, *next;

	ivi_layout_ivi_shell_destroy();

	wl_list_remove(&shell->destroy_listener.link);
	wl_list_remove(&shell->wake_listener.link);

	if (shell->text_backend) {
		text_backend_destroy(shell->text_backend);
		input_panel_destroy(shell);
	}

	wl_list_for_each_safe(ivisurf, next, &shell->ivi_surface_list, link) {
		if (ivisurf->layout_surface != NULL)
			layout_surface_cleanup(ivisurf);
		wl_list_remove(&ivisurf->link);
		free(ivisurf);
	}

	ivi_layout_fini();

	weston_desktop_destroy(shell->desktop);
	free(shell);
}

/*
 * Called through the compositor's wake signal.
 */
static void
wake_handler(struct wl_listener *listener, void *data)
{
	struct weston_compositor *compositor = data;

	weston_compositor_damage_all(compositor);
}

static void
terminate_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		  uint32_t key, void *data)
{
	struct weston_compositor *compositor = data;

	weston_compositor_exit(compositor);
}

static void
init_ivi_shell(struct weston_compositor *compositor, struct ivi_shell *shell)
{
	struct weston_config *config = wet_get_config(compositor);
	struct weston_config_section *section;
	bool developermode;

	shell->compositor = compositor;

	wl_list_init(&shell->ivi_surface_list);

	section = weston_config_get_section(config, "ivi-shell", NULL, NULL);

	weston_config_section_get_bool(section, "developermode",
				       &developermode, 0);

	if (developermode) {
		weston_install_debug_key_binding(compositor, MODIFIER_SUPER);

		weston_compositor_add_key_binding(compositor, KEY_BACKSPACE,
						  MODIFIER_CTRL | MODIFIER_ALT,
						  terminate_binding,
						  compositor);
	}
}

static struct ivi_shell_surface *
get_last_child(struct ivi_shell_surface *ivisurf)
{
	struct ivi_shell_surface *ivisurf_child;

	wl_list_for_each_reverse(ivisurf_child, &ivisurf->children_list,
				 children_link) {
		if (weston_surface_is_mapped(ivisurf_child->surface))
			return ivisurf_child;
	}
	return NULL;
}

static void
activate_binding(struct weston_seat *seat,
		 struct weston_view *focus_view,
		 uint32_t flags)
{
	struct ivi_shell_surface *ivisurf, *ivisurf_child;
	struct weston_surface *main_surface;

	main_surface = weston_surface_get_main_surface(focus_view->surface);
	ivisurf = get_ivi_shell_surface(main_surface);
	if (ivisurf == NULL)
		return;

	ivisurf_child = get_last_child(ivisurf);
	if (ivisurf_child) {
		struct weston_view *view
			= ivisurf_child->layout_surface->ivi_view->view;
		activate_binding(seat, view, flags);
		return;
	}

	/* FIXME: need to activate the surface like
	   kiosk_shell_surface_activate() */
	weston_view_activate_input(focus_view, seat, flags);
}

static void
click_to_activate_binding(struct weston_pointer *pointer,
			  const struct timespec *time,
			  uint32_t button, void *data)
{
	if (pointer->grab != &pointer->default_grab)
		return;
	if (pointer->focus == NULL)
		return;

	activate_binding(pointer->seat, pointer->focus,
			 WESTON_ACTIVATE_FLAG_CLICKED);
}

static void
touch_to_activate_binding(struct weston_touch *touch,
			  const struct timespec *time,
			  void *data)
{
	if (touch->grab != &touch->default_grab)
		return;
	if (touch->focus == NULL)
		return;

	activate_binding(touch->seat, touch->focus, WESTON_ACTIVATE_FLAG_NONE);
}

static void
shell_add_bindings(struct weston_compositor *compositor,
		   struct ivi_shell *shell)
{
	weston_compositor_add_button_binding(compositor, BTN_LEFT, 0,
					     click_to_activate_binding,
					     shell);
	weston_compositor_add_button_binding(compositor, BTN_RIGHT, 0,
					     click_to_activate_binding,
					     shell);
	weston_compositor_add_touch_binding(compositor, 0,
					    touch_to_activate_binding,
					    shell);
}

/*
 * libweston-desktop
 */

static void
desktop_surface_ping_timeout(struct weston_desktop_client *client,
			     void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_pong(struct weston_desktop_client *client,
		     void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_added(struct weston_desktop_surface *surface,
		      void *user_data)
{
	struct ivi_shell *shell = (struct ivi_shell *) user_data;
	struct ivi_layout_surface *layout_surface;
	struct ivi_shell_surface *ivisurf;
	struct weston_surface *weston_surf =
			weston_desktop_surface_get_surface(surface);

	layout_surface = ivi_layout_desktop_surface_create(weston_surf, surface);

	ivisurf = xzalloc(sizeof *ivisurf);

	ivisurf->shell = shell;
	ivisurf->id_surface = IVI_INVALID_ID;

	ivisurf->width = 0;
	ivisurf->height = 0;
	ivisurf->layout_surface = layout_surface;
	ivisurf->surface = weston_surf;

	wl_list_insert(&shell->ivi_surface_list, &ivisurf->link);

	/*
	 * initialize list as well as link. The latter allows to use
	 * wl_list_remove() event when this surface is not in another list.
	 */
	wl_list_init(&ivisurf->children_list);
	wl_list_init(&ivisurf->children_link);

	weston_desktop_surface_set_user_data(surface, ivisurf);
}

static void
desktop_surface_removed(struct weston_desktop_surface *surface,
			void *user_data)
{
	struct ivi_shell_surface *ivisurf = (struct ivi_shell_surface *)
			weston_desktop_surface_get_user_data(surface);
	struct ivi_shell_surface *ivisurf_child, *tmp;

	assert(ivisurf != NULL);

	weston_desktop_surface_set_user_data(surface, NULL);

	wl_list_for_each_safe(ivisurf_child, tmp, &ivisurf->children_list,
			      children_link) {
		wl_list_remove(&ivisurf_child->children_link);
		wl_list_init(&ivisurf_child->children_link);
	}
	wl_list_remove(&ivisurf->children_link);

	if (ivisurf->layout_surface)
		layout_surface_cleanup(ivisurf);

	wl_list_remove(&ivisurf->link);

	free(ivisurf);
}

static void
desktop_surface_committed(struct weston_desktop_surface *surface,
			  int32_t sx, int32_t sy, void *user_data)
{
	struct ivi_shell_surface *ivisurf = (struct ivi_shell_surface *)
			weston_desktop_surface_get_user_data(surface);
	struct weston_surface *weston_surf =
			weston_desktop_surface_get_surface(surface);

	if(!ivisurf)
		return;

	if (weston_surf->width == 0 || weston_surf->height == 0) {
		if (!weston_surface_is_unmapping(weston_surf))
			return;
	}

	if (ivisurf->width != weston_surf->width ||
	    ivisurf->height != weston_surf->height) {
		ivisurf->width  = weston_surf->width;
		ivisurf->height = weston_surf->height;

		ivi_layout_desktop_surface_configure(ivisurf->layout_surface,
						 weston_surf->width,
						 weston_surf->height);
	}
}

static void
desktop_surface_move(struct weston_desktop_surface *surface,
		     struct weston_seat *seat, uint32_t serial, void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_resize(struct weston_desktop_surface *surface,
		       struct weston_seat *seat, uint32_t serial,
		       enum weston_desktop_surface_edge edges, void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_set_parent(struct weston_desktop_surface *desktop_surface,
			   struct weston_desktop_surface *parent,
			   void *shell)
{
	struct ivi_shell_surface *ivisurf =
		weston_desktop_surface_get_user_data(desktop_surface);
	struct ivi_shell_surface *ivisurf_parent;

	if (!parent)
		return;

	ivisurf_parent = weston_desktop_surface_get_user_data(parent);
	wl_list_insert(ivisurf_parent->children_list.prev,
		       &ivisurf->children_link);
}

static void
desktop_surface_fullscreen_requested(struct weston_desktop_surface *surface,
				     bool fullscreen,
				     struct weston_output *output,
				     void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_maximized_requested(struct weston_desktop_surface *surface,
				    bool maximized, void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_minimized_requested(struct weston_desktop_surface *surface,
				    void *user_data)
{
	/* Not supported */
}

static void
desktop_surface_set_xwayland_position(struct weston_desktop_surface *surface,
				      int32_t x, int32_t y, void *user_data)
{
	/* Not supported */
}

static const struct weston_desktop_api shell_desktop_api = {
	.struct_size = sizeof(struct weston_desktop_api),
	.ping_timeout = desktop_surface_ping_timeout,
	.pong = desktop_surface_pong,
	.surface_added = desktop_surface_added,
	.surface_removed = desktop_surface_removed,
	.committed = desktop_surface_committed,

	.move = desktop_surface_move,
	.resize = desktop_surface_resize,
	.set_parent = desktop_surface_set_parent,
	.fullscreen_requested = desktop_surface_fullscreen_requested,
	.maximized_requested = desktop_surface_maximized_requested,
	.minimized_requested = desktop_surface_minimized_requested,
	.set_xwayland_position = desktop_surface_set_xwayland_position,
};

/*
 * end of libweston-desktop
 */

/*
 * input panel
 */

static void
maybe_show_input_panel(struct ivi_input_panel_surface *ipsurf,
		       struct ivi_shell_surface *target_ivisurf)
{
	if (ipsurf->surface->width == 0)
		return;

	if (ipsurf->type == INPUT_PANEL_NONE)
		return;

	ivi_layout_show_input_panel(ipsurf->layout_surface,
				    target_ivisurf->layout_surface,
				    ipsurf->type == INPUT_PANEL_OVERLAY);
}

static void
show_input_panels(struct wl_listener *listener, void *data)
{
	struct ivi_shell *shell = container_of(listener, struct ivi_shell,
					       show_input_panel_listener);
	struct ivi_shell_surface *target_ivisurf;
	struct ivi_input_panel_surface *ipsurf;

	target_ivisurf = get_ivi_shell_surface(data);
	if (!target_ivisurf)
		return;

	if (shell->text_input_surface)
		return;

	shell->text_input_surface = target_ivisurf;

	wl_list_for_each(ipsurf, &shell->input_panel.surfaces, link)
		maybe_show_input_panel(ipsurf, target_ivisurf);
}

static void
hide_input_panels(struct wl_listener *listener, void *data)
{
	struct ivi_shell *shell = container_of(listener, struct ivi_shell,
					       hide_input_panel_listener);
	struct ivi_input_panel_surface *ipsurf;

	if (!shell->text_input_surface)
		return;

	shell->text_input_surface = NULL;

	wl_list_for_each(ipsurf, &shell->input_panel.surfaces, link)
		ivi_layout_hide_input_panel(ipsurf->layout_surface);
}

static void
update_input_panels(struct wl_listener *listener, void *data)
{
	ivi_layout_update_text_input_cursor(data);
}

static int
input_panel_get_label(struct weston_surface *surface, char *buf, size_t len)
{
	return snprintf(buf, len, "input panel");
}

static void
input_panel_committed(struct weston_surface *surface,
		      struct weston_coord_surface new_origin)
{
	struct ivi_input_panel_surface *ipsurf = surface->committed_private;
	struct ivi_shell *shell = ipsurf->shell;

	if (surface->width == 0 || surface->height == 0)
		return;

	if (ipsurf->width != surface->width ||
	    ipsurf->height != surface->height) {
		ipsurf->width  = surface->width;
		ipsurf->height = surface->height;
		ivi_layout_input_panel_surface_configure(ipsurf->layout_surface,
							 surface->width,
							 surface->height);
	}

	if (shell->text_input_surface)
		maybe_show_input_panel(ipsurf, shell->text_input_surface);
}

bool
shell_is_input_panel_surface(struct weston_surface *surface)
{
	return surface->committed == input_panel_committed;
}

static struct ivi_input_panel_surface *
get_input_panel_surface(struct weston_surface *surface)
{
	if (shell_is_input_panel_surface(surface))
		return surface->committed_private;
	else
		return NULL;
}

static void
input_panel_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct ivi_input_panel_surface *ipsurf =
		container_of(listener, struct ivi_input_panel_surface,
			     surface_destroy_listener);

	wl_resource_destroy(ipsurf->resource);
}

static struct ivi_input_panel_surface *
create_input_panel_surface(struct ivi_shell *shell,
			   struct weston_surface *surface)
{
	struct ivi_input_panel_surface *ipsurf;
	struct ivi_layout_surface *layout_surface;

	layout_surface = ivi_layout_input_panel_surface_create(surface);

	ipsurf = xzalloc(sizeof *ipsurf);

	surface->committed = input_panel_committed;
	surface->committed_private = ipsurf;
	weston_surface_set_label_func(surface, input_panel_get_label);

	wl_list_init(&ipsurf->link);
	wl_list_insert(&shell->input_panel.surfaces, &ipsurf->link);

	ipsurf->shell = shell;

	ipsurf->width = 0;
	ipsurf->height = 0;
	ipsurf->layout_surface = layout_surface;
	ipsurf->surface = surface;

	if (surface->width && surface->height) {
		ipsurf->width  = surface->width;
		ipsurf->height = surface->height;
		ivi_layout_input_panel_surface_configure(ipsurf->layout_surface,
							 surface->width,
							 surface->height);
	}

	ipsurf->surface_destroy_listener.notify = input_panel_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &ipsurf->surface_destroy_listener);

	return ipsurf;
}

static void
input_panel_surface_set_toplevel(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_resource *output_resource,
				 uint32_t position)
{
	struct ivi_input_panel_surface *ipsurf =
		wl_resource_get_user_data(resource);
	struct weston_head *head;

	head = weston_head_from_resource(output_resource);

	ipsurf->type = INPUT_PANEL_TOPLEVEL;
	ipsurf->output = head->output;
}

static void
input_panel_surface_set_overlay_panel(struct wl_client *client,
				      struct wl_resource *resource)
{
	struct ivi_input_panel_surface *ipsurf =
		wl_resource_get_user_data(resource);

	ipsurf->type = INPUT_PANEL_OVERLAY;
}

static const struct zwp_input_panel_surface_v1_interface input_panel_surface_implementation = {
	input_panel_surface_set_toplevel,
	input_panel_surface_set_overlay_panel
};

static void
destroy_input_panel_surface_resource(struct wl_resource *resource)
{
	struct ivi_input_panel_surface *ipsurf =
		wl_resource_get_user_data(resource);

	assert(ipsurf->resource == resource);

	ivi_layout_surface_destroy(ipsurf->layout_surface);
	ipsurf->layout_surface = NULL;

	ipsurf->surface->committed = NULL;
	ipsurf->surface->committed_private = NULL;
	weston_surface_set_label_func(ipsurf->surface, NULL);
	ipsurf->surface = NULL;

	wl_list_remove(&ipsurf->surface_destroy_listener.link);
	wl_list_remove(&ipsurf->link);

	free(ipsurf);
}

static void
input_panel_get_input_panel_surface(struct wl_client *client,
				    struct wl_resource *resource,
				    uint32_t id,
				    struct wl_resource *surface_resource)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct ivi_shell *shell = wl_resource_get_user_data(resource);
	struct ivi_input_panel_surface *ipsurf;

	if (get_input_panel_surface(surface)) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "wl_input_panel::get_input_panel_surface already requested");
		return;
	}

	ipsurf = create_input_panel_surface(shell, surface);
	if (!ipsurf) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface->committed already set");
		return;
	}

	ipsurf->resource =
		wl_resource_create(client,
				   &zwp_input_panel_surface_v1_interface,
				   1,
				   id);
	wl_resource_set_implementation(ipsurf->resource,
				       &input_panel_surface_implementation,
				       ipsurf,
				       destroy_input_panel_surface_resource);
}

static const struct zwp_input_panel_v1_interface input_panel_implementation = {
	input_panel_get_input_panel_surface
};

static void
unbind_input_panel(struct wl_resource *resource)
{
	struct ivi_shell *shell = wl_resource_get_user_data(resource);

	shell->input_panel.binding = NULL;
}

static void
bind_input_panel(struct wl_client *client,
	      void *data, uint32_t version, uint32_t id)
{
	struct ivi_shell *shell = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &zwp_input_panel_v1_interface, 1, id);

	if (shell->input_panel.binding == NULL) {
		wl_resource_set_implementation(resource,
					       &input_panel_implementation,
					       shell, unbind_input_panel);
		shell->input_panel.binding = resource;
		return;
	}

	wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
			       "interface object already bound");
}

void
input_panel_destroy(struct ivi_shell *shell)
{
	wl_list_remove(&shell->show_input_panel_listener.link);
	wl_list_remove(&shell->hide_input_panel_listener.link);
	wl_list_remove(&shell->update_input_panel_listener.link);
}

static void
input_panel_setup(struct ivi_shell *shell)
{
	struct weston_compositor *ec = shell->compositor;

	shell->show_input_panel_listener.notify = show_input_panels;
	wl_signal_add(&ec->show_input_panel_signal,
		      &shell->show_input_panel_listener);
	shell->hide_input_panel_listener.notify = hide_input_panels;
	wl_signal_add(&ec->hide_input_panel_signal,
		      &shell->hide_input_panel_listener);
	shell->update_input_panel_listener.notify = update_input_panels;
	wl_signal_add(&ec->update_input_panel_signal,
		      &shell->update_input_panel_listener);

	wl_list_init(&shell->input_panel.surfaces);

	abort_oom_if_null(wl_global_create(shell->compositor->wl_display,
					   &zwp_input_panel_v1_interface, 1,
					   shell, bind_input_panel));
}

void
shell_ensure_text_input(struct ivi_shell *shell)
{
	if (shell->text_backend)
		return;

	shell->text_backend = text_backend_init(shell->compositor);
	input_panel_setup(shell);
}

/*
 * end of input panel
 */

/*
 * Initialization of ivi-shell.
 */
WL_EXPORT int
wet_shell_init(struct weston_compositor *compositor,
	       int *argc, char *argv[])
{
	struct ivi_shell *shell;

	shell = xzalloc(sizeof *shell);

	if (!weston_compositor_add_destroy_listener_once(compositor,
							 &shell->destroy_listener,
							 shell_destroy)) {
		free(shell);
		return 0;
	}

	init_ivi_shell(compositor, shell);

	shell->wake_listener.notify = wake_handler;
	wl_signal_add(&compositor->wake_signal, &shell->wake_listener);

	shell->desktop = weston_desktop_create(compositor, &shell_desktop_api, shell);
	if (!shell->desktop)
		goto err_shell;

	if (wl_global_create(compositor->wl_display,
			     &ivi_application_interface, 1,
			     shell, bind_ivi_application) == NULL)
		goto err_desktop;

	ivi_layout_init(compositor, shell);

	screenshooter_create(compositor);

	shell_add_bindings(compositor, shell);

	return IVI_SUCCEEDED;

err_desktop:
	weston_desktop_destroy(shell->desktop);

err_shell:
	wl_list_remove(&shell->destroy_listener.link);
	free(shell);

	return IVI_FAILED;
}
