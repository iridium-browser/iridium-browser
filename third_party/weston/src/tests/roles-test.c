/*
 * Copyright © 2014 Collabora, Ltd.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.logging_scopes = "log,proto,test-harness-plugin";

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static struct wl_subcompositor *
get_subcompositor(struct client *client)
{
	struct global *g;
	struct global *global_sub = NULL;
	struct wl_subcompositor *sub;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, "wl_subcompositor"))
			continue;

		if (global_sub)
			assert(0 && "multiple wl_subcompositor objects");

		global_sub = g;
	}

	assert(global_sub && "no wl_subcompositor found");

	assert(global_sub->version == 1);

	sub = wl_registry_bind(client->wl_registry, global_sub->name,
			       &wl_subcompositor_interface, 1);
	assert(sub);

	return sub;
}

static struct wl_shell *
get_wl_shell(struct client *client)
{
	struct global *g;
	struct global *global = NULL;
	struct wl_shell *shell;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, "wl_shell"))
			continue;

		if (global)
			assert(0 && "multiple wl_shell objects");

		global = g;
	}

	assert(global && "no wl_shell found");

	shell = wl_registry_bind(client->wl_registry, global->name,
				 &wl_shell_interface, 1);
	assert(shell);

	return shell;
}

TEST(test_role_conflict_sub_wlshell)
{
	struct client *client;
	struct wl_subcompositor *subco;
	struct wl_surface *child;
	struct wl_subsurface *sub;
	struct wl_shell *shell;
	struct wl_shell_surface *shsurf;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	subco = get_subcompositor(client);
	shell = get_wl_shell(client);

	child = wl_compositor_create_surface(client->wl_compositor);
	assert(child);
	sub = wl_subcompositor_get_subsurface(subco, child,
					      client->surface->wl_surface);
	assert(sub);

	shsurf = wl_shell_get_shell_surface(shell, child);
	assert(shsurf);

	expect_protocol_error(client, &wl_shell_interface,
			      WL_SHELL_ERROR_ROLE);
}

TEST(test_role_conflict_wlshell_sub)
{
	struct client *client;
	struct wl_subcompositor *subco;
	struct wl_surface *child;
	struct wl_subsurface *sub;
	struct wl_shell *shell;
	struct wl_shell_surface *shsurf;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	subco = get_subcompositor(client);
	shell = get_wl_shell(client);

	child = wl_compositor_create_surface(client->wl_compositor);
	assert(child);
	shsurf = wl_shell_get_shell_surface(shell, child);
	assert(shsurf);

	sub = wl_subcompositor_get_subsurface(subco, child,
					      client->surface->wl_surface);
	assert(sub);

	expect_protocol_error(client, &wl_subcompositor_interface,
			      WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE);
}
