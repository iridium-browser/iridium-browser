/*
 * Copyright © 2019 Intel Corporation
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
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include "libweston-internal.h"
#include "weston-content-protection-server-protocol.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

#define content_protection_log(cp, ...) \
	weston_log_scope_printf((cp)->debug, __VA_ARGS__)

static const char * const content_type_name [] = {
	[WESTON_PROTECTED_SURFACE_TYPE_UNPROTECTED] = "UNPROTECTED",
	[WESTON_PROTECTED_SURFACE_TYPE_HDCP_0] = "TYPE-0",
	[WESTON_PROTECTED_SURFACE_TYPE_HDCP_1] = "TYPE-1",
};

void
weston_protected_surface_send_event(struct protected_surface *psurface,
				       enum weston_hdcp_protection protection)
{
	struct wl_resource *p_resource;
	enum weston_protected_surface_type protection_type;
	struct content_protection *cp;
	struct wl_resource *surface_resource;

	p_resource = psurface->protection_resource;
	if (!p_resource)
		return;
	/* No event to be sent to client, in case of enforced mode */
	if (psurface->surface->protection_mode == WESTON_SURFACE_PROTECTION_MODE_ENFORCED)
		return;
	protection_type = (enum weston_protected_surface_type) protection;
	weston_protected_surface_send_status(p_resource, protection_type);

	cp = psurface->cp_backptr;
	surface_resource = psurface->surface->resource;
	content_protection_log(cp, "wl_surface@%"PRIu32" Protection type set to %s\n",
			       wl_resource_get_id(surface_resource),
			       content_type_name[protection_type]);
}

static void
set_type(struct wl_client *client, struct wl_resource *resource,
	 enum weston_protected_surface_type content_type)
{
	struct content_protection *cp;
	struct protected_surface *psurface;
	enum weston_hdcp_protection weston_cp;
	struct wl_resource *surface_resource;

	psurface = wl_resource_get_user_data(resource);
	if (!psurface)
		return;
	cp = psurface->cp_backptr;
	surface_resource = psurface->surface->resource;

	if (content_type < WESTON_PROTECTED_SURFACE_TYPE_UNPROTECTED ||
	    content_type > WESTON_PROTECTED_SURFACE_TYPE_HDCP_1) {
		wl_resource_post_error(resource,
				       WESTON_PROTECTED_SURFACE_ERROR_INVALID_TYPE,
				       "wl_surface@%"PRIu32" Invalid content-type %d for request:set_type\n",
				       wl_resource_get_id(surface_resource), content_type);

		content_protection_log(cp, "wl_surface@%"PRIu32" Invalid content-type %d for resquest:set_type\n",
				       wl_resource_get_id(surface_resource), content_type);
		return;
	}

	content_protection_log(cp, "wl_surface@%"PRIu32" Request: Enable Content-Protection Type: %s\n",
			       wl_resource_get_id(surface_resource),
			       content_type_name[content_type]);

	weston_cp = (enum weston_hdcp_protection) content_type;
	psurface->surface->pending.desired_protection = weston_cp;
}

static void
protected_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	struct protected_surface *psurface;

	psurface = wl_resource_get_user_data(resource);
	if (!psurface)
		return;
	psurface->surface->pending.desired_protection = WESTON_HDCP_DISABLE;
}

static void
set_enforce_mode(struct wl_client *client, struct wl_resource *resource)
{
	/*
	 * Enforce Censored-Visibility. Compositor censors the protected
	 * surface on an unsecured output.
	 * In case of a surface, being shown on an unprotected output, the
	 * compositor hides the surface, not allowing it to be displayed on
	 * the unprotected output, without bothering the client. No difference
	 * for the protected outputs.
	 *
	 * The member 'protection_mode' is "double-buffered", so setting it in
	 * pending_state will cause the setting of the corresponding
	 * 'protection_mode' in weston_surface, after the commit.
	 *
	 * This function sets the 'protection_mode' of the weston_surface_state
	 * to 'enfoced'. The renderers inspect the flag and compare the
	 * desired_protection of the surface, to the current_protection of the
	 * output, based on that the real surface or a place-holder content,
	 * (e.g. solid color) are shown.
	 */

	struct protected_surface *psurface;

	psurface = wl_resource_get_user_data(resource);
	if (!psurface)
		return;

	psurface->surface->pending.protection_mode =
		WESTON_SURFACE_PROTECTION_MODE_ENFORCED;
}

static void
set_relax_mode(struct wl_client *client, struct wl_resource *resource)
{
	/*
	 * Relaxed mode. By default this mode will be activated.
	 * In case of a surface, being shown in unprotected output,
	 * compositor just sends the event for protection status changed.
	 *
	 * On setting the relaxed mode, the 'protection_mode' member is queued
	 * to be set to 'relax' from the existing 'enforce' mode.
	 */

	struct protected_surface *psurface;

	psurface = wl_resource_get_user_data(resource);
	if (!psurface)
		return;

	psurface->surface->pending.protection_mode =
		WESTON_SURFACE_PROTECTION_MODE_RELAXED;
}

static const struct weston_protected_surface_interface protected_surface_implementation = {
	protected_surface_destroy,
	set_type,
	set_enforce_mode,
	set_relax_mode,
};

static void
cp_destroy_listener(struct wl_listener *listener, void *data)
{
	struct content_protection *cp;

	cp = container_of(listener, struct content_protection,
			  destroy_listener);
	wl_list_remove(&cp->destroy_listener.link);
	wl_list_remove(&cp->protected_list);
	weston_log_scope_destroy(cp->debug);
	cp->debug = NULL;
	cp->surface_protection_update = NULL;
	free(cp);
}

static void
free_protected_surface(struct protected_surface *psurface)
{
	psurface->surface->pending.desired_protection = WESTON_HDCP_DISABLE;
	wl_resource_set_user_data(psurface->protection_resource, NULL);
	wl_list_remove(&psurface->surface_destroy_listener.link);
	wl_list_remove(&psurface->link);
	free(psurface);
}

static void
surface_destroyed(struct wl_listener *listener, void *data)
{
	struct protected_surface *psurface;

	psurface = container_of(listener, struct protected_surface,
				surface_destroy_listener);
	free_protected_surface(psurface);
}

static void
destroy_protected_surface(struct wl_resource *resource)
{
	struct protected_surface *psurface;

	psurface = wl_resource_get_user_data(resource);
	if (!psurface)
		return;
	free_protected_surface(psurface);
}

static void
get_protection(struct wl_client *client, struct wl_resource *cp_resource,
	     uint32_t id, struct wl_resource *surface_resource)
{
	struct wl_resource *resource;
	struct weston_surface *surface;
	struct content_protection *cp;
	struct protected_surface *psurface;
	struct wl_listener *listener;

	surface = wl_resource_get_user_data(surface_resource);
	assert(surface);
	cp = wl_resource_get_user_data(cp_resource);
	assert(cp);

	/*
	 * Check if this client has a corresponding protected-surface
	 */

	listener = wl_resource_get_destroy_listener(surface->resource,
						    surface_destroyed);

	if (listener) {
		wl_resource_post_error(cp_resource,
				       WESTON_CONTENT_PROTECTION_ERROR_SURFACE_EXISTS,
				       "wl_surface@%"PRIu32" Protection already exists",
				       wl_resource_get_id(surface_resource));
		return;
	}

	psurface = zalloc(sizeof(struct protected_surface));
	if (!psurface) {
		wl_client_post_no_memory(client);
		return;
	}
	psurface->cp_backptr = cp;
	resource = wl_resource_create(client, &weston_protected_surface_interface,
				      1, id);
	if (!resource) {
		free(psurface);
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&cp->protected_list, &psurface->link);
	wl_resource_set_implementation(resource, &protected_surface_implementation,
				       psurface,
				       destroy_protected_surface);

	psurface->protection_resource = resource;
	psurface->surface = surface;
	psurface->surface_destroy_listener.notify = surface_destroyed;
	wl_resource_add_destroy_listener(surface->resource,
					 &psurface->surface_destroy_listener);
	weston_protected_surface_send_event(psurface,
					    psurface->surface->current_protection);
}

static void
destroy_protection(struct wl_client *client, struct wl_resource *cp_resource)
{
	wl_resource_destroy(cp_resource);
}

static const
struct weston_content_protection_interface content_protection_implementation = {
	destroy_protection,
	get_protection,
};

static void
bind_weston_content_protection(struct wl_client *client, void *data,
			       uint32_t version, uint32_t id)
{
	struct content_protection *cp = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client,
				      &weston_content_protection_interface,
				      1, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource,
				       &content_protection_implementation,
				       cp, NULL);
}
/* Advertise the content-protection support.
 *
 * Calling this function sets up the content-protection support via HDCP.
 * This exposes the global interface, visible to the client, enabling them to
 * request for content-protection for their surfaces according to the type of
 * content.
 */

WL_EXPORT int
weston_compositor_enable_content_protection(struct weston_compositor *compositor)
{
	struct content_protection *cp;

	cp = zalloc(sizeof(*cp));
	if (cp == NULL)
		return -1;
	cp->compositor = compositor;

	compositor->content_protection = cp;
	wl_list_init(&cp->protected_list);
	if (wl_global_create(compositor->wl_display,
			     &weston_content_protection_interface, 1, cp,
			     bind_weston_content_protection) == NULL)
		return -1;

	cp->destroy_listener.notify = cp_destroy_listener;
	wl_signal_add(&compositor->destroy_signal, &cp->destroy_listener);
	cp->debug = weston_compositor_add_log_scope(compositor, "content-protection-debug",
						    "debug-logs for content-protection",
						    NULL, NULL, NULL);
	return 0;
}
