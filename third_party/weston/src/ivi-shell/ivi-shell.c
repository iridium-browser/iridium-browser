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

#include "ivi-shell.h"
#include "ivi-application-server-protocol.h"
#include "ivi-layout-private.h"
#include "ivi-layout-shell.h"
#include "shared/helpers.h"
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

	struct wl_list link;
};

/*
 * Implementation of ivi_surface
 */

static void
ivi_shell_surface_committed(struct weston_surface *, int32_t, int32_t);

static struct ivi_shell_surface *
get_ivi_shell_surface(struct weston_surface *surface)
{
	struct ivi_shell_surface *shsurf;

	if (surface->committed != ivi_shell_surface_committed)
		return NULL;

	shsurf = surface->committed_private;
	assert(shsurf);
	assert(shsurf->surface == surface);

	return shsurf;
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
	if (!shsurf)
		return;

	if (shsurf->resource)
		ivi_surface_send_configure(shsurf->resource, width, height);
}

static void
ivi_shell_surface_committed(struct weston_surface *surface,
			    int32_t sx, int32_t sy)
{
	struct ivi_shell_surface *ivisurf = get_ivi_shell_surface(surface);

	assert(ivisurf);
	if (!ivisurf)
		return;

	if (surface->width == 0 || surface->height == 0)
		return;

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

	if (!shell_surf)
		return snprintf(buf, len, "unidentified window in ivi-shell");

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

	ivisurf = zalloc(sizeof *ivisurf);
	if (ivisurf == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	wl_list_init(&ivisurf->link);
	wl_list_insert(&shell->ivi_surface_list, &ivisurf->link);

	ivisurf->shell = shell;
	ivisurf->id_surface = id_surface;

	ivisurf->width = 0;
	ivisurf->height = 0;
	ivisurf->layout_surface = layout_surface;

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

/*
 * Called through the compositor's destroy signal.
 */
static void
shell_destroy(struct wl_listener *listener, void *data)
{
	struct ivi_shell *shell =
		container_of(listener, struct ivi_shell, destroy_listener);
	struct ivi_shell_surface *ivisurf, *next;

	wl_list_remove(&shell->destroy_listener.link);
	wl_list_remove(&shell->wake_listener.link);

	wl_list_for_each_safe(ivisurf, next, &shell->ivi_surface_list, link) {
		wl_list_remove(&ivisurf->link);
		free(ivisurf);
	}

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

static void
activate_binding(struct weston_seat *seat,
		 struct weston_view *focus_view)
{
	struct weston_surface *focus = focus_view->surface;
	struct weston_surface *main_surface =
		weston_surface_get_main_surface(focus);

	if (get_ivi_shell_surface(main_surface) == NULL)
		return;

	weston_seat_set_keyboard_focus(seat, focus);
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

	activate_binding(pointer->seat, pointer->focus);
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

	activate_binding(touch->seat, touch->focus);
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

	layout_surface = ivi_layout_desktop_surface_create(weston_surf);
	if (!layout_surface) {
		return;
	}

	layout_surface->weston_desktop_surface = surface;

	ivisurf = zalloc(sizeof *ivisurf);
	if (!ivisurf) {
		return;
	}

	ivisurf->shell = shell;
	ivisurf->id_surface = IVI_INVALID_ID;

	ivisurf->width = 0;
	ivisurf->height = 0;
	ivisurf->layout_surface = layout_surface;
	ivisurf->surface = weston_surf;

	weston_desktop_surface_set_user_data(surface, ivisurf);
}

static void
desktop_surface_removed(struct weston_desktop_surface *surface,
			void *user_data)
{
	struct ivi_shell_surface *ivisurf = (struct ivi_shell_surface *)
			weston_desktop_surface_get_user_data(surface);

	assert(ivisurf != NULL);

	if (ivisurf->layout_surface)
		layout_surface_cleanup(ivisurf);
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

	if (weston_surf->width == 0 || weston_surf->height == 0)
		return;

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
	.fullscreen_requested = desktop_surface_fullscreen_requested,
	.maximized_requested = desktop_surface_maximized_requested,
	.minimized_requested = desktop_surface_minimized_requested,
	.set_xwayland_position = desktop_surface_set_xwayland_position,
};

/*
 * end of libweston-desktop
 */

/*
 * Initialization of ivi-shell.
 */
WL_EXPORT int
wet_shell_init(struct weston_compositor *compositor,
	       int *argc, char *argv[])
{
	struct ivi_shell *shell;

	shell = zalloc(sizeof *shell);
	if (shell == NULL)
		return -1;

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

	ivi_layout_init_with_compositor(compositor);
	shell_add_bindings(compositor, shell);

	return IVI_SUCCEEDED;

err_desktop:
	weston_desktop_destroy(shell->desktop);

err_shell:
	wl_list_remove(&shell->destroy_listener.link);
	free(shell);

	return IVI_FAILED;
}
