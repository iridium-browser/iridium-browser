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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "shared/timespec-util.h"
#include "weston-test-client-helper.h"
#include "presentation-time-client-protocol.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

static struct wp_presentation *
get_presentation(struct client *client)
{
	struct global *g;
	struct global *global_pres = NULL;
	struct wp_presentation *pres;

	wl_list_for_each(g, &client->global_list, link) {
		if (strcmp(g->interface, wp_presentation_interface.name))
			continue;

		if (global_pres)
			assert(0 && "multiple presentation objects");

		global_pres = g;
	}

	assert(global_pres && "no presentation found");

	assert(global_pres->version == 1);

	pres = wl_registry_bind(client->wl_registry, global_pres->name,
				&wp_presentation_interface, 1);
	assert(pres);

	return pres;
}

struct feedback {
	struct client *client;
	struct wp_presentation_feedback *obj;

	enum {
		FB_PENDING = 0,
		FB_PRESENTED,
		FB_DISCARDED
	} result;

	struct wl_output *sync_output;
	uint64_t seq;
	struct timespec time;
	uint32_t refresh_nsec;
	uint32_t flags;
};

static void
feedback_sync_output(void *data,
		     struct wp_presentation_feedback *presentation_feedback,
		     struct wl_output *output)
{
	struct feedback *fb = data;

	assert(fb->result == FB_PENDING);

	if (output)
		fb->sync_output = output;
}

static void
feedback_presented(void *data,
		   struct wp_presentation_feedback *presentation_feedback,
		   uint32_t tv_sec_hi,
		   uint32_t tv_sec_lo,
		   uint32_t tv_nsec,
		   uint32_t refresh_nsec,
		   uint32_t seq_hi,
		   uint32_t seq_lo,
		   uint32_t flags)
{
	struct feedback *fb = data;

	assert(fb->result == FB_PENDING);
	fb->result = FB_PRESENTED;
	fb->seq = u64_from_u32s(seq_hi, seq_lo);
	timespec_from_proto(&fb->time, tv_sec_hi, tv_sec_lo, tv_nsec);
	fb->refresh_nsec = refresh_nsec;
	fb->flags = flags;
}

static void
feedback_discarded(void *data,
		   struct wp_presentation_feedback *presentation_feedback)
{
	struct feedback *fb = data;

	assert(fb->result == FB_PENDING);
	fb->result = FB_DISCARDED;
}

static const struct wp_presentation_feedback_listener feedback_listener = {
	feedback_sync_output,
	feedback_presented,
	feedback_discarded
};

static struct feedback *
feedback_create(struct client *client,
		struct wl_surface *surface,
		struct wp_presentation *pres)
{
	struct feedback *fb;

	fb = xzalloc(sizeof *fb);
	fb->client = client;
	fb->obj = wp_presentation_feedback(pres, surface);
	wp_presentation_feedback_add_listener(fb->obj, &feedback_listener, fb);

	return fb;
}

static void
feedback_wait(struct feedback *fb)
{
	while (fb->result == FB_PENDING) {
		assert(wl_display_dispatch(fb->client->wl_display) >= 0);
	}
}

static char *
pflags_to_str(uint32_t flags, char *str, unsigned len)
{
	static const struct {
		uint32_t flag;
		char sym;
	} desc[] = {
		{ WP_PRESENTATION_FEEDBACK_KIND_VSYNC, 's' },
		{ WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK, 'c' },
		{ WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION, 'e' },
		{ WP_PRESENTATION_FEEDBACK_KIND_ZERO_COPY, 'z' },
	};
	unsigned i;

	*str = '\0';
	if (len < ARRAY_LENGTH(desc) + 1)
		return str;

	for (i = 0; i < ARRAY_LENGTH(desc); i++)
		str[i] = flags & desc[i].flag ? desc[i].sym : '_';
	str[ARRAY_LENGTH(desc)] = '\0';

	return str;
}

static void
feedback_print(struct feedback *fb)
{
	char str[10];

	switch (fb->result) {
	case FB_PENDING:
		testlog("pending");
		return;
	case FB_DISCARDED:
		testlog("discarded");
		return;
	case FB_PRESENTED:
		break;
	}

	pflags_to_str(fb->flags, str, sizeof str);
	testlog("presented %lld.%09lld, refresh %u us, [%s] seq %" PRIu64,
		(long long)fb->time.tv_sec, (long long)fb->time.tv_nsec,
		fb->refresh_nsec / 1000, str, fb->seq);
}

static void
feedback_destroy(struct feedback *fb)
{
	wp_presentation_feedback_destroy(fb->obj);
	free(fb);
}

TEST(test_presentation_feedback_simple)
{
	struct client *client;
	struct feedback *fb;
	struct wp_presentation *pres;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);
	pres = get_presentation(client);

	wl_surface_attach(client->surface->wl_surface,
			  client->surface->buffer->proxy, 0, 0);
	fb = feedback_create(client, client->surface->wl_surface, pres);
	wl_surface_damage(client->surface->wl_surface, 0, 0, 100, 100);
	wl_surface_commit(client->surface->wl_surface);

	client_roundtrip(client);

	feedback_wait(fb);

	testlog("%s feedback:", __func__);
	feedback_print(fb);
	testlog("\n");

	feedback_destroy(fb);
	wp_presentation_destroy(pres);
	client_destroy(client);
}
