/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright 2022 Collabora, Ltd.
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
#include <linux/input.h>

#include <libweston/libweston.h>
#include "weston.h"
#include "shared/helpers.h"

struct screenshooter {
	struct weston_compositor *ec;
	struct wl_client *client;
	struct wl_listener client_destroy_listener;
	struct wl_listener compositor_destroy_listener;
	struct weston_recorder *recorder;
	struct wl_listener authorization;
};

static void
screenshooter_client_destroy(struct wl_listener *listener, void *data)
{
	struct screenshooter *shooter =
		container_of(listener, struct screenshooter,
			     client_destroy_listener);

	shooter->client = NULL;
}

static void
screenshooter_binding(struct weston_keyboard *keyboard,
		      const struct timespec *time, uint32_t key, void *data)
{
	struct screenshooter *shooter = data;
	char *screenshooter_exe;

	/* Don't start a screenshot whilst we already have one in progress */
	if (shooter->client)
		return;

	screenshooter_exe = wet_get_bindir_path("weston-screenshooter");
	if (!screenshooter_exe) {
		weston_log("Could not construct screenshooter path.\n");
		return;
	}

	shooter->client = weston_client_start(shooter->ec,
					      screenshooter_exe);
	free(screenshooter_exe);

	if (!shooter->client)
		return;

	shooter->client_destroy_listener.notify = screenshooter_client_destroy;
	wl_client_add_destroy_listener(shooter->client,
				       &shooter->client_destroy_listener);
}

static void
recorder_binding(struct weston_keyboard *keyboard, const struct timespec *time,
		 uint32_t key, void *data)
{
	struct weston_compositor *ec = keyboard->seat->compositor;
	struct weston_output *output;
	struct screenshooter *shooter = data;
	struct weston_recorder *recorder = shooter->recorder;;
	static const char filename[] = "capture.wcap";

	if (recorder) {
		weston_recorder_stop(recorder);
		shooter->recorder = NULL;
	} else {
		if (keyboard->focus && keyboard->focus->output)
			output = keyboard->focus->output;
		else
			output = container_of(ec->output_list.next,
					      struct weston_output, link);

		shooter->recorder = weston_recorder_start(output, filename);
	}
}

static void
authorize_screenshooter(struct wl_listener *l,
			struct weston_output_capture_attempt *att)
{
	struct screenshooter *shooter = wl_container_of(l, shooter,
							authorization);

	if (shooter->client && att->who->client == shooter->client)
		att->authorized = true;
}

static void
screenshooter_destroy(struct wl_listener *listener, void *data)
{
	struct screenshooter *shooter =
		container_of(listener, struct screenshooter,
			     compositor_destroy_listener);

	wl_list_remove(&shooter->compositor_destroy_listener.link);
	wl_list_remove(&shooter->authorization.link);

	free(shooter);
}

WL_EXPORT void
screenshooter_create(struct weston_compositor *ec)
{
	struct screenshooter *shooter;

	shooter = zalloc(sizeof *shooter);
	if (shooter == NULL)
		return;

	shooter->ec = ec;

	weston_compositor_add_key_binding(ec, KEY_S, MODIFIER_SUPER,
					  screenshooter_binding, shooter);
	weston_compositor_add_key_binding(ec, KEY_R, MODIFIER_SUPER,
					  recorder_binding, shooter);

	shooter->compositor_destroy_listener.notify = screenshooter_destroy;
	wl_signal_add(&ec->destroy_signal,
		      &shooter->compositor_destroy_listener);

	weston_compositor_add_screenshot_authority(ec, &shooter->authorization,
						   authorize_screenshooter);
}
