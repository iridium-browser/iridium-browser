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
#include <libweston/shell-utils.h>
#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"

struct test_output {
       struct weston_compositor *compositor;
       struct weston_output *output;
       struct wl_listener output_destroy_listener;
       struct weston_curtain *curtain;
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
       struct compositor_setup setup;

       compositor_setup_defaults(&setup);
       setup.shell = SHELL_TEST_DESKTOP;

       return weston_test_harness_execute_as_plugin(harness, &setup);
}

DECLARE_FIXTURE_SETUP(fixture_setup);

static void
output_destroy(struct test_output *t_output)
{
       t_output->output = NULL;
       t_output->output_destroy_listener.notify = NULL;

       if (t_output->curtain)
               weston_shell_utils_curtain_destroy(t_output->curtain);

       wl_list_remove(&t_output->output_destroy_listener.link);
       free(t_output);
}

static void
notify_output_destroy(struct wl_listener *listener, void *data)
{
       struct test_output *t_output =
               container_of(listener, struct test_output, output_destroy_listener);

       output_destroy(t_output);
}

static void
output_create_view(struct test_output *t_output)
{
       struct weston_curtain_params curtain_params = {
	       .r = 0.5, .g = 0.5, .b = 0.5, .a = 1.0,
	       .x = 0, .y = 0,
	       .width = 320, .height = 240,
	       .get_label = NULL,
	       .surface_committed = NULL,
	       .surface_private = NULL,
       };

       t_output->curtain = weston_shell_utils_curtain_create(t_output->compositor,
							     &curtain_params);
       weston_view_set_output(t_output->curtain->view, t_output->output);

       /* weston_compositor_remove_output() has to be patched with
	* weston_signal_emit_mutable() to avoid signal corruption */
       weston_output_destroy(t_output->output);
}

static void
output_create(struct weston_output *output)
{
       struct test_output *t_output;

       t_output = zalloc(sizeof *t_output);
       if (t_output == NULL)
               return;

       t_output->output = output;
       t_output->compositor = output->compositor;

       t_output->output_destroy_listener.notify = notify_output_destroy;
       wl_signal_add(&t_output->output->destroy_signal,
                     &t_output->output_destroy_listener);

       output_create_view(t_output);
}

static void
create_outputs(struct weston_compositor *compositor)
{
       struct weston_output *output, *output_next;

       wl_list_for_each_safe(output, output_next, &compositor->output_list, link)
               output_create(output);
}

PLUGIN_TEST(real_usecase_one)
{
       create_outputs(compositor);
}
