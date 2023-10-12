/*
 * Copyright © 2015 Collabora, Ltd.
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
#include <stdio.h>
#include <string.h>

#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "weston-test-client-helper.h"
#include "ivi-application-client-protocol.h"
#include "ivi-test.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_IVI;
	setup.extra_module = "test-ivi-layout.so";

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

struct runner {
	struct client *client;
	struct weston_test_runner *test_runner;
	int done;
};

static void
runner_finished_handler(void *data, struct weston_test_runner *test_runner)
{
	struct runner *runner = data;

	runner->done = 1;
}

static const struct weston_test_runner_listener test_runner_listener = {
	runner_finished_handler
};

static struct runner *
client_create_runner(struct client *client)
{
	struct runner *runner;
	struct global *g;
	struct global *global_runner = NULL;

	runner = xzalloc(sizeof(*runner));
	runner->client = client;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, "weston_test_runner"))
			continue;

		if (global_runner)
			assert(0 && "multiple weston_test_runner objects");

		global_runner = g;
	}

	assert(global_runner && "no weston_test_runner found");
	assert(global_runner->version == 1);

	runner->test_runner = wl_registry_bind(client->wl_registry,
					       global_runner->name,
					       &weston_test_runner_interface,
					       1);
	assert(runner->test_runner);

	weston_test_runner_add_listener(runner->test_runner,
					&test_runner_listener, runner);

	return runner;
}

static void
runner_destroy(struct runner *runner)
{
	weston_test_runner_destroy(runner->test_runner);
	client_roundtrip(runner->client);
	free(runner);
}

static void
runner_run(struct runner *runner, const char *test_name)
{
	testlog("weston_test_runner.run(\"%s\")\n", test_name);

	runner->done = 0;
	weston_test_runner_run(runner->test_runner, test_name);

	while (!runner->done) {
		if (wl_display_dispatch(runner->client->wl_display) < 0)
			assert(0 && "runner wait");
	}
}

static struct ivi_application *
get_ivi_application(struct client *client)
{
	struct global *g;
	struct global *global_iviapp = NULL;
	struct ivi_application *iviapp;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, "ivi_application"))
			continue;

		if (global_iviapp)
			assert(0 && "multiple ivi_application objects");

		global_iviapp = g;
	}

	assert(global_iviapp && "no ivi_application found");

	assert(global_iviapp->version == 1);

	iviapp = wl_registry_bind(client->wl_registry, global_iviapp->name,
				  &ivi_application_interface, 1);
	assert(iviapp);

	return iviapp;
}

struct ivi_window {
	struct wl_surface *wl_surface;
	struct ivi_surface *ivi_surface;
	uint32_t ivi_id;
};

static struct ivi_window *
client_create_ivi_window(struct client *client,
			 struct ivi_application *iviapp,
			 uint32_t ivi_id)
{
	struct ivi_window *wnd;

	wnd = xzalloc(sizeof(*wnd));
	wnd->wl_surface = wl_compositor_create_surface(client->wl_compositor);
	wnd->ivi_surface = ivi_application_surface_create(iviapp, ivi_id,
							  wnd->wl_surface);
	wnd->ivi_id = ivi_id;

	return wnd;
}

static void
ivi_window_destroy(struct ivi_window *wnd)
{
	ivi_surface_destroy(wnd->ivi_surface);
	wl_surface_destroy(wnd->wl_surface);
	free(wnd);
}

/******************************** tests ********************************/

/*
 * These tests make use of weston_test_runner global interface exposed by
 * ivi-layout-test-plugin.c. This allows these tests to trigger compositor-side
 * checks.
 *
 * See ivi-layout-test-plugin.c for further details.
 */

/**
 * RUNNER_TEST() names are defined in ivi-layout-test-plugin.c.
 * Each RUNNER_TEST name listed here uses the same simple initial client setup.
 */
const char * const basic_test_names[] = {
	"surface_visibility",
	"surface_opacity",
	"surface_dimension",
	"surface_position",
	"surface_destination_rectangle",
	"surface_source_rectangle",
	"surface_bad_opacity",
	"surface_properties_changed_notification",
	"surface_on_many_layer",
};

const char * const surface_property_commit_changes_test_names[] = {
	"commit_changes_after_visibility_set_surface_destroy",
	"commit_changes_after_opacity_set_surface_destroy",
	"commit_changes_after_source_rectangle_set_surface_destroy",
	"commit_changes_after_destination_rectangle_set_surface_destroy",
};

const char * const render_order_test_names[] = {
	"layer_render_order",
	"layer_add_surfaces",
};

TEST_P(ivi_layout_runner, basic_test_names)
{
	/* an element from basic_test_names */
	const char * const *test_name = data;
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wnd;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	wnd = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	runner_run(runner, *test_name);

	ivi_window_destroy(wnd);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(ivi_layout_surface_create)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *winds[2];

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	winds[0] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(0));
	winds[1] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(1));

	runner_run(runner, "surface_create_p1");

	ivi_window_destroy(winds[0]);

	runner_run(runner, "surface_create_p2");

	ivi_window_destroy(winds[1]);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST_P(commit_changes_after_properties_set_surface_destroy, surface_property_commit_changes_test_names)
{
	/* an element from surface_property_commit_changes_test_names */
	const char * const *test_name = data;
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wnd;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	wnd = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	runner_run(runner, *test_name);

	ivi_window_destroy(wnd);

	runner_run(runner, "ivi_layout_commit_changes");

	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(get_surface_after_destroy_ivi_surface)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wnd;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	wnd = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	ivi_surface_destroy(wnd->ivi_surface);

	runner_run(runner, "get_surface_after_destroy_surface");

	wl_surface_destroy(wnd->wl_surface);
	free(wnd);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(get_surface_after_destroy_wl_surface)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wnd;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	wnd = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	wl_surface_destroy(wnd->wl_surface);

	runner_run(runner, "get_surface_after_destroy_surface");

	ivi_surface_destroy(wnd->ivi_surface);
	free(wnd);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST_P(ivi_layout_layer_render_order_runner, render_order_test_names)
{
	/* an element from render_order_test_names */
	const char * const *test_name = data;
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *winds[3];

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	winds[0] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(0));
	winds[1] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(1));
	winds[2] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(2));

	runner_run(runner, *test_name);

	ivi_window_destroy(winds[0]);
	ivi_window_destroy(winds[1]);
	ivi_window_destroy(winds[2]);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(destroy_surface_after_layer_render_order)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *winds[3];

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	winds[0] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(0));
	winds[1] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(1));
	winds[2] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(2));

	runner_run(runner, "test_layer_render_order_destroy_one_surface_p1");

	ivi_window_destroy(winds[1]);

	runner_run(runner, "test_layer_render_order_destroy_one_surface_p2");

	ivi_window_destroy(winds[0]);
	ivi_window_destroy(winds[2]);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(commit_changes_after_render_order_set_surface_destroy)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *winds[3];

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	winds[0] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(0));
	winds[1] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(1));
	winds[2] = client_create_ivi_window(client, iviapp,
					    IVI_TEST_SURFACE_ID(2));

	runner_run(runner, "commit_changes_after_render_order_set_surface_destroy");

	ivi_window_destroy(winds[1]);

	runner_run(runner, "ivi_layout_commit_changes");
	runner_run(runner, "cleanup_layer");

	ivi_window_destroy(winds[0]);
	ivi_window_destroy(winds[2]);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(ivi_layout_surface_configure_notification)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wind;
	struct buffer *buffer;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	runner_run(runner, "surface_configure_notification_p1");

	wind = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	buffer = create_shm_buffer_a8r8g8b8(client, 200, 300);

	wl_surface_attach(wind->wl_surface, buffer->proxy, 0, 0);
	wl_surface_damage(wind->wl_surface, 0, 0, 20, 30);
	wl_surface_commit(wind->wl_surface);

	runner_run(runner, "surface_configure_notification_p2");

	wl_surface_attach(wind->wl_surface, buffer->proxy, 0, 0);
	wl_surface_damage(wind->wl_surface, 0, 0, 40, 50);
	wl_surface_commit(wind->wl_surface);

	runner_run(runner, "surface_configure_notification_p3");

	buffer_destroy(buffer);
	ivi_window_destroy(wind);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(ivi_layout_surface_create_notification)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wind;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	runner_run(runner, "surface_create_notification_p1");

	wind = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));

	runner_run(runner, "surface_create_notification_p2");

	ivi_window_destroy(wind);
	wind = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));
	runner_run(runner, "surface_create_notification_p3");

	ivi_window_destroy(wind);
	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}

TEST(ivi_layout_surface_remove_notification)
{
	struct client *client;
	struct runner *runner;
	struct ivi_application *iviapp;
	struct ivi_window *wind;

	client = create_client();
	runner = client_create_runner(client);
	iviapp = get_ivi_application(client);

	wind = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));
	runner_run(runner, "surface_remove_notification_p1");
	ivi_window_destroy(wind);

	runner_run(runner, "surface_remove_notification_p2");

	wind = client_create_ivi_window(client, iviapp, IVI_TEST_SURFACE_ID(0));
	ivi_window_destroy(wind);
	runner_run(runner, "surface_remove_notification_p3");

	runner_destroy(runner);
	ivi_application_destroy(iviapp);
	client_destroy(client);
}
