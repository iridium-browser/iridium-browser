/*
 * Copyright 2021 Collabora, Ltd.
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

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "../shared/signal.h"
#include "weston-test-client-helper.h"

struct test_surface_state {
	struct wl_signal signal_destroy;
	struct wl_listener surface_destroy_listener;
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	return weston_test_harness_execute_standalone(harness);
}

DECLARE_FIXTURE_SETUP(fixture_setup);

static void
destroy_test_surface(struct test_surface_state *st)
{
	weston_signal_emit_mutable(&st->signal_destroy, st);
}

static void
notify_test_destroy(struct wl_listener *listener, void *data)
{
	struct test_surface_state *st = data;

	wl_list_remove(&st->surface_destroy_listener.link);
	free(st);
}

static struct test_surface_state *
create_surface(void)
{
	struct test_surface_state *st = zalloc(sizeof(*st));

	wl_signal_init(&st->signal_destroy);
	return st;
}

static void
add_destroy_listener(struct test_surface_state *st)
{
	st->surface_destroy_listener.notify = notify_test_destroy;
	wl_signal_add(&st->signal_destroy,
		      &st->surface_destroy_listener);
}

TEST(real_usecase_standalone)
{
	struct test_surface_state *st, *st_new;

	st = create_surface();
	add_destroy_listener(st);

	st_new = create_surface();
	add_destroy_listener(st_new);

	destroy_test_surface(st);
	destroy_test_surface(st_new);
}
