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

#include <stdio.h>
#include <string.h>

#include "weston-test-client-helper.h"
#include "ivi-application-client-protocol.h"
#include "weston-test-fixture-compositor.h"
#include "test-config.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_IVI;
	setup.logging_scopes = "log,test-harness-plugin,proto";

	weston_ini_setup(&setup,
			 cfgln("[core]"),
			 cfgln("shell=%s", "ivi-shell.so"),
			 cfgln("modules=", "hmi-controller.so"),

			 cfgln("[ivi-shell]"),
			 cfgln("ivi-shell-user-interface=%s", "weston-ivi-shell-user-interface"),
			 cfgln("cursor-theme=%s", "default"),
			 cfgln("cursor-size=%d", 32),
			 cfgln("base-layer-id=%d", 1000),
			 cfgln("base-layer-id-offset=%d", 10000),
			 cfgln("workspace-background-layer-id=%d", 2000),
			 cfgln("workspace-layer-id=%d", 3000),
			 cfgln("application-layer-id=%d", 4000),
			 cfgln("transition-duration=%d", 300),
			 cfgln("background-image=%s", WESTON_DATA_DIR "/background.png"),
			 cfgln("background-id=%d", 1001),
			 cfgln("panel-image=%s", WESTON_DATA_DIR "/panel.png"),
			 cfgln("panel-id=%d", 1002),
			 cfgln("surface-id-offset=%d", 10),
			 cfgln("tiling-image=%s", WESTON_DATA_DIR "/tiling.png"),
			 cfgln("tiling-id=%d", 1003),
			 cfgln("sidebyside-image=%s", WESTON_DATA_DIR "/sidebyside.png"),
			 cfgln("sidebyside-id=%d", 1004),
			 cfgln("fullscreen-image=%s", WESTON_DATA_DIR "/fullscreen.png"),
			 cfgln("fullscreen-id=%d", 1005),
			 cfgln("random-image=%s", WESTON_DATA_DIR "/random.png"),
			 cfgln("random-id=%d", 1006),
			 cfgln("home-image=%s", WESTON_DATA_DIR "/home.png"),
			 cfgln("home-id=%d", 1007),
			 cfgln("workspace-background-color=%s", "0x99000000"),
			 cfgln("workspace-background-id=%d", 2001),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 0),
			 cfgln("icon-id=%d", 4001),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_flower.png"),
			 cfgln("path=%s", BINDIR "/weston-flower"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 0),
			 cfgln("icon-id=%d", 4002),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_clickdot.png"),
			 cfgln("path=%s", BINDIR "/weston-clickdot"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 1),
			 cfgln("icon-id=%d", 4003),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_simple-egl.png"),
			 cfgln("path=%s", BINDIR "/weston-simple-egl"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 1),
			 cfgln("icon-id=%d", 4004),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_simple-shm.png"),
			 cfgln("path=%s", BINDIR "/weston-simple-shm"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 2),
			 cfgln("icon-id=%d", 4005),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_smoke.png"),
			 cfgln("path=%s", BINDIR "/weston-smoke"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 3),
			 cfgln("icon-id=%d", 4006),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_flower.png"),
			 cfgln("path=%s", BINDIR "/weston-flower"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 3),
			 cfgln("icon-id=%d", 4007),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_clickdot.png"),
			 cfgln("path=%s", BINDIR "/weston-clickdot"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 3),
			 cfgln("icon-id=%d", 4008),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_simple-egl.png"),
			 cfgln("path=%s", BINDIR "/weston-simple-egl"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 3),
			 cfgln("icon-id=%d", 4009),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_simple-shm.png"),
			 cfgln("path=%s", BINDIR "/weston-simple-shm"),

			 cfgln("[ivi-launcher]"),
			 cfgln("workspace-id=%d", 3),
			 cfgln("icon-id=%d", 4010),
			 cfgln("icon=%s", WESTON_DATA_DIR "/icon_ivi_smoke.png"),
			 cfgln("path=%s", BINDIR "/weston-smoke")
			);

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

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

TEST(ivi_application_exists)
{
	struct client *client;
	struct ivi_application *iviapp;

	client = create_client();
	iviapp = get_ivi_application(client);
	client_roundtrip(client);

	testlog("Successful bind: %p\n", iviapp);

	ivi_application_destroy(iviapp);
	client_destroy(client);
}
