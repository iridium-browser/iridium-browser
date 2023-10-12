/*
 * Copyright (C) 2016 DENSO CORPORATION
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <libweston/libweston.h>
#include <libweston/plugin-registry.h>

struct weston_plugin_api {
	struct wl_list link;     /**< in weston_compositor::plugin_api_list */

	char *api_name;          /**< The search key */
	const void *vtable;      /**< The function table */
	size_t vtable_size;      /**< Size of the function table in bytes */
};

/** Register an implementation of an API
 *
 * \param compositor The compositor instance.
 * \param api_name The API name which other plugins use to find the
 * implementation.
 * \param vtable Pointer to the function table of the API.
 * \param vtable_size Size of the function table in bytes.
 * \return 0 on success, -1 on error, -2 if api_name already registered
 *
 * This call makes the given vtable to be reachable via
 * weston_plugin_api_get(). Calls through the vtable may start happening
 * as soon as the caller returns after success. Argument vtable must not be
 * NULL. Argument api_name must be non-NULL and non-zero length.
 *
 * You can increase the function table size without breaking the ABI.
 * To cater for ABI breaks, it is recommended to have api_name include a
 * version number.
 *
 * A registered API cannot be unregistered. However, calls through a
 * registered API must not be made from the compositor destroy signal handlers.
 */
WL_EXPORT int
weston_plugin_api_register(struct weston_compositor *compositor,
			   const char *api_name,
			   const void *vtable,
			   size_t vtable_size)
{
	struct weston_plugin_api *wpa;

	assert(api_name);
	assert(strlen(api_name) > 0);
	assert(vtable);

	if (!api_name || !vtable || strlen(api_name) == 0)
		return -1;

	wl_list_for_each(wpa, &compositor->plugin_api_list, link)
		if (strcmp(wpa->api_name, api_name) == 0)
			return -2;

	wpa = zalloc(sizeof(*wpa));
	if (!wpa)
		return -1;

	wpa->api_name = strdup(api_name);
	wpa->vtable = vtable;
	wpa->vtable_size = vtable_size;

	if (!wpa->api_name) {
		free(wpa);

		return -1;
	}

	wl_list_insert(&compositor->plugin_api_list, &wpa->link);
	weston_log("Registered plugin API '%s' of size %zd\n",
		   wpa->api_name, wpa->vtable_size);

	return 0;
}

/** Internal function to free registered APIs
 *
 * \param compositor The compositor instance.
 */
void
weston_plugin_api_destroy_list(struct weston_compositor *compositor)
{
	struct weston_plugin_api *wpa, *tmp;

	wl_list_for_each_safe(wpa, tmp, &compositor->plugin_api_list, link) {
		free(wpa->api_name);
		wl_list_remove(&wpa->link);
		free(wpa);
	}
}

/** Fetch the implementation of an API
 *
 * \param compositor The compositor instance.
 * \param api_name The name of the API to search for.
 * \param vtable_size The expected function table size in bytes.
 * \return Pointer to the function table, or NULL on error.
 *
 * Find the function table corresponding to api_name. The vtable_size here
 * must be less or equal to the vtable_size given in the corresponding
 * weston_plugin_api_register() call made by the implementing plugin.
 *
 * Calls can be made through the function table immediately. However, calls
 * must not be made from or after the compositor destroy signal handler.
 */
WL_EXPORT const void *
weston_plugin_api_get(struct weston_compositor *compositor,
		      const char *api_name,
		      size_t vtable_size)
{
	struct weston_plugin_api *wpa;

	assert(api_name);
	if (!api_name)
		return NULL;

	wl_list_for_each(wpa, &compositor->plugin_api_list, link) {
		if (strcmp(wpa->api_name, api_name) != 0)
			continue;

		if (vtable_size <= wpa->vtable_size)
			return wpa->vtable;

		return NULL;
	}

	return NULL;
}
