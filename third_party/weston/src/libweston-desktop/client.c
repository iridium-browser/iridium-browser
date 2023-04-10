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

#include "config.h"

#include <wayland-server.h>

#include <libweston/libweston.h>
#include <libweston/zalloc.h>

#include <libweston-desktop/libweston-desktop.h>
#include "internal.h"

struct weston_desktop_client {
	struct weston_desktop *desktop;
	struct wl_client *client;
	struct wl_resource *resource;
	struct wl_list surface_list;
	uint32_t ping_serial;
	struct wl_event_source *ping_timer;
	struct wl_signal destroy_signal;
};

void
weston_desktop_client_add_destroy_listener(struct weston_desktop_client *client,
					   struct wl_listener *listener)
{
	wl_signal_add(&client->destroy_signal, listener);
}

static void
weston_desktop_client_destroy(struct wl_resource *resource)
{
	struct weston_desktop_client *client =
		wl_resource_get_user_data(resource);
	struct wl_list *list = &client->surface_list;
	struct wl_list *link, *tmp;

	wl_signal_emit(&client->destroy_signal, client);

	for (link = list->next, tmp = link->next;
	     link != list;
	     link = tmp, tmp = link->next) {
		wl_list_remove(link);
		wl_list_init(link);
	}

	if (client->ping_timer != NULL)
		wl_event_source_remove(client->ping_timer);

	free(client);
}

static int
weston_desktop_client_ping_timeout(void *user_data)
{
	struct weston_desktop_client *client = user_data;

	weston_desktop_api_ping_timeout(client->desktop, client);
	return 1;
}

struct weston_desktop_client *
weston_desktop_client_create(struct weston_desktop *desktop,
			     struct wl_client *wl_client,
			     wl_dispatcher_func_t dispatcher,
			     const struct wl_interface *interface,
			     const void *implementation, uint32_t version,
			     uint32_t id)
{
	struct weston_desktop_client *client;
	struct wl_display *display;
	struct wl_event_loop *loop;

	client = zalloc(sizeof(struct weston_desktop_client));
	if (client == NULL) {
		if (wl_client != NULL)
			wl_client_post_no_memory(wl_client);
		return NULL;
	}

	client->desktop = desktop;
	client->client = wl_client;

	wl_list_init(&client->surface_list);
	wl_signal_init(&client->destroy_signal);

	if (wl_client == NULL)
		return client;

	client->resource = wl_resource_create(wl_client, interface, version, id);
	if (client->resource == NULL) {
		wl_client_post_no_memory(wl_client);
		free(client);
		return NULL;
	}

	if (dispatcher != NULL)
		wl_resource_set_dispatcher(client->resource, dispatcher,
					   weston_desktop_client_destroy, client,
					   weston_desktop_client_destroy);
	else
		wl_resource_set_implementation(client->resource, implementation,
					       client,
					       weston_desktop_client_destroy);


	display = wl_client_get_display(client->client);
	loop = wl_display_get_event_loop(display);
	client->ping_timer =
		wl_event_loop_add_timer(loop,
					weston_desktop_client_ping_timeout,
					client);
	if (client->ping_timer == NULL)
		wl_client_post_no_memory(wl_client);

	return client;
}

struct weston_desktop *
weston_desktop_client_get_desktop(struct weston_desktop_client *client)
{
	return client->desktop;
}

struct wl_resource *
weston_desktop_client_get_resource(struct weston_desktop_client *client)
{
	return client->resource;
}

struct wl_list *
weston_desktop_client_get_surface_list(struct weston_desktop_client *client)
{
	return &client->surface_list;
}

WL_EXPORT struct wl_client *
weston_desktop_client_get_client(struct weston_desktop_client *client)
{
	return client->client;
}

WL_EXPORT void
weston_desktop_client_for_each_surface(struct weston_desktop_client *client,
				       void (*callback)(struct weston_desktop_surface *surface, void *user_data),
				       void *user_data)
{
	struct wl_list *list = &client->surface_list;
	struct wl_list *link;

	for (link = list->next; link != list; link = link->next)
		callback(weston_desktop_surface_from_client_link(link),
			 user_data);
}

WL_EXPORT int
weston_desktop_client_ping(struct weston_desktop_client *client)
{
	struct weston_desktop_surface *surface =
		weston_desktop_surface_from_client_link(client->surface_list.next);
	const struct weston_desktop_surface_implementation *implementation =
		weston_desktop_surface_get_implementation(surface);
	void *implementation_data =
		weston_desktop_surface_get_implementation_data(surface);

	if (implementation->ping == NULL)
		return -1;

	if (client->ping_serial != 0)
		return 1;

	client->ping_serial =
		wl_display_next_serial(wl_client_get_display(client->client));
	wl_event_source_timer_update(client->ping_timer, 10000);

	implementation->ping(surface, client->ping_serial, implementation_data);

	return 0;
}

void
weston_desktop_client_pong(struct weston_desktop_client *client, uint32_t serial)
{
	if (client->ping_serial != serial)
		return;

	weston_desktop_api_pong(client->desktop, client);

	wl_event_source_timer_update(client->ping_timer, 0);
	client->ping_serial = 0;
}
