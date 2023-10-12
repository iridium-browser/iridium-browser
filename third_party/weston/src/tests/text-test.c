/*
 * Copyright © 2012 Intel Corporation
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

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "weston-test-client-helper.h"
#include "text-input-unstable-v1-client-protocol.h"
#include "weston-test-fixture-compositor.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

struct text_input_state {
	int activated;
	int deactivated;
};

static void
text_input_commit_string(void *data,
			 struct zwp_text_input_v1 *text_input,
			 uint32_t serial,
			 const char *text)
{
}

static void
text_input_preedit_string(void *data,
			  struct zwp_text_input_v1 *text_input,
			  uint32_t serial,
			  const char *text,
			  const char *commit)
{
}

static void
text_input_delete_surrounding_text(void *data,
				   struct zwp_text_input_v1 *text_input,
				   int32_t index,
				   uint32_t length)
{
}

static void
text_input_cursor_position(void *data,
			   struct zwp_text_input_v1 *text_input,
			   int32_t index,
			   int32_t anchor)
{
}

static void
text_input_preedit_styling(void *data,
			   struct zwp_text_input_v1 *text_input,
			   uint32_t index,
			   uint32_t length,
			   uint32_t style)
{
}

static void
text_input_preedit_cursor(void *data,
			  struct zwp_text_input_v1 *text_input,
			  int32_t index)
{
}

static void
text_input_modifiers_map(void *data,
			 struct zwp_text_input_v1 *text_input,
			 struct wl_array *map)
{
}

static void
text_input_keysym(void *data,
		  struct zwp_text_input_v1 *text_input,
		  uint32_t serial,
		  uint32_t time,
		  uint32_t sym,
		  uint32_t state,
		  uint32_t modifiers)
{
}

static void
text_input_enter(void *data,
		 struct zwp_text_input_v1 *text_input,
		 struct wl_surface *surface)

{
	struct text_input_state *state = data;

	testlog("%s\n", __FUNCTION__);

	state->activated += 1;
}

static void
text_input_leave(void *data,
		 struct zwp_text_input_v1 *text_input)
{
	struct text_input_state *state = data;

	state->deactivated += 1;
}

static void
text_input_input_panel_state(void *data,
			     struct zwp_text_input_v1 *text_input,
			     uint32_t state)
{
}

static void
text_input_language(void *data,
		    struct zwp_text_input_v1 *text_input,
		    uint32_t serial,
		    const char *language)
{
}

static void
text_input_text_direction(void *data,
			  struct zwp_text_input_v1 *text_input,
			  uint32_t serial,
			  uint32_t direction)
{
}

static const struct zwp_text_input_v1_listener text_input_listener = {
	text_input_enter,
	text_input_leave,
	text_input_modifiers_map,
	text_input_input_panel_state,
	text_input_preedit_string,
	text_input_preedit_styling,
	text_input_preedit_cursor,
	text_input_commit_string,
	text_input_cursor_position,
	text_input_delete_surrounding_text,
	text_input_keysym,
	text_input_language,
	text_input_text_direction
};

TEST(text_test)
{
	struct client *client;
	struct global *global;
	struct zwp_text_input_manager_v1 *factory;
	struct zwp_text_input_v1 *text_input;
	struct text_input_state state;

	client = create_client_and_test_surface(100, 100, 100, 100);
	assert(client);

	factory = NULL;
	wl_list_for_each(global, &client->global_list, link) {
		if (strcmp(global->interface, "zwp_text_input_manager_v1") == 0)
			factory = wl_registry_bind(client->wl_registry,
						   global->name,
						   &zwp_text_input_manager_v1_interface, 1);
	}

	assert(factory);

	memset(&state, 0, sizeof state);
	text_input = zwp_text_input_manager_v1_create_text_input(factory);
	zwp_text_input_v1_add_listener(text_input,
				       &text_input_listener,
				       &state);

	/* Make sure our test surface has keyboard focus. */
	weston_test_activate_surface(client->test->weston_test,
				 client->surface->wl_surface);
	client_roundtrip(client);
	assert(client->input->keyboard->focus == client->surface);

	/* Activate test model and make sure we get enter event. */
	zwp_text_input_v1_activate(text_input, client->input->wl_seat,
				   client->surface->wl_surface);
	client_roundtrip(client);
	assert(state.activated == 1 && state.deactivated == 0);

	/* Deactivate test model and make sure we get leave event. */
	zwp_text_input_v1_deactivate(text_input, client->input->wl_seat);
	client_roundtrip(client);
	assert(state.activated == 1 && state.deactivated == 1);

	/* Activate test model again. */
	zwp_text_input_v1_activate(text_input, client->input->wl_seat,
				   client->surface->wl_surface);
	client_roundtrip(client);
	assert(state.activated == 2 && state.deactivated == 1);

	/* Take keyboard focus away and verify we get leave event. */
	weston_test_activate_surface(client->test->weston_test, NULL);
	client_roundtrip(client);
	assert(state.activated == 2 && state.deactivated == 2);

	zwp_text_input_v1_destroy(text_input);
	zwp_text_input_manager_v1_destroy(factory);
	client_destroy(client);
}
