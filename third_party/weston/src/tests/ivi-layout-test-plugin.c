/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 DENSO CORPORATION
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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>

#include <libweston/libweston.h>
#include "compositor/weston.h"
#include "weston-test-server-protocol.h"
#include "ivi-test.h"
#include "ivi-shell/ivi-layout-export.h"
#include "shared/helpers.h"

struct test_context;

struct runner_test {
	const char *name;
	void (*run)(struct test_context *);
} __attribute__ ((aligned (64)));

#define RUNNER_TEST(name)					\
	static void runner_func_##name(struct test_context *);	\
								\
	const struct runner_test runner_test_##name		\
		__attribute__ ((used, section ("plugin_test_section"))) = \
	{							\
		#name, runner_func_##name			\
	};							\
								\
	static void runner_func_##name(struct test_context *ctx)

extern const struct runner_test __start_plugin_test_section;
extern const struct runner_test __stop_plugin_test_section;

static const struct runner_test *
find_runner_test(const char *name)
{
	const struct runner_test *t;

	for (t = &__start_plugin_test_section;
	     t < &__stop_plugin_test_section; t++) {
		if (strcmp(t->name, name) == 0)
			return t;
	}

	return NULL;
}

struct test_context {
	const struct ivi_layout_interface *layout_interface;
	struct wl_resource *runner_resource;
	uint32_t user_flags;

	struct wl_listener surface_property_changed;
	struct wl_listener surface_created;
	struct wl_listener surface_removed;
	struct wl_listener surface_configured;
};

struct test_launcher {
	struct weston_compositor *compositor;
	struct wl_listener destroy_listener;
	struct test_context context;
	const struct ivi_layout_interface *layout_interface;
};

static void
destroy_runner(struct wl_resource *resource)
{
	struct test_launcher *launcher = wl_resource_get_user_data(resource);
	struct test_context *ctx = &launcher->context;

	assert(ctx->runner_resource == NULL ||
	       ctx->runner_resource == resource);

	ctx->layout_interface = NULL;
	ctx->runner_resource = NULL;
}

static void
runner_destroy_handler(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
runner_run_handler(struct wl_client *client, struct wl_resource *resource,
		   const char *test_name)
{
	struct test_launcher *launcher;
	const struct runner_test *t;
	struct test_context *ctx;

	launcher = wl_resource_get_user_data(resource);
	ctx = &launcher->context;

	assert(ctx->runner_resource == NULL ||
	       ctx->runner_resource == resource);

	ctx->layout_interface = launcher->layout_interface;
	ctx->runner_resource = resource;

	t = find_runner_test(test_name);
	if (!t) {
		weston_log("Error: runner test \"%s\" not found.\n",
			   test_name);
		wl_resource_post_error(resource,
				       WESTON_TEST_RUNNER_ERROR_UNKNOWN_TEST,
				       "weston_test_runner: unknown: '%s'",
				       test_name);
		return;
	}

	weston_log("weston_test_runner.run(\"%s\")\n", test_name);

	t->run(ctx);

	weston_test_runner_send_finished(resource);
}

static const struct weston_test_runner_interface runner_implementation = {
	runner_destroy_handler,
	runner_run_handler
};

static void
bind_runner(struct wl_client *client, void *data,
	    uint32_t version, uint32_t id)
{
	struct test_launcher *launcher = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &weston_test_runner_interface,
				      1, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &runner_implementation,
				       launcher, destroy_runner);

	if (launcher->context.runner_resource != NULL) {
		weston_log("test FATAL: "
			   "attempting to run several tests in parallel.\n");
		wl_resource_post_error(resource,
				       WESTON_TEST_RUNNER_ERROR_TEST_FAILED,
				       "attempt to run parallel tests");
	}
}

static void
test_launcher_destroy(struct wl_listener *l, void *data)
{
	struct test_launcher *launcher;

	launcher = wl_container_of(l, launcher, destroy_listener);

	free(launcher);
}

WL_EXPORT int
wet_module_init(struct weston_compositor *compositor,
		       int *argc, char *argv[])
{
	struct test_launcher *launcher;
	const struct ivi_layout_interface *iface;

	iface = ivi_layout_get_api(compositor);

	if (!iface) {
		weston_log("fatal: cannot use ivi_layout_interface.\n");
		return -1;
	}

	launcher = zalloc(sizeof *launcher);
	if (!launcher)
		return -1;

	launcher->compositor = compositor;
	launcher->layout_interface = iface;

	if (!weston_compositor_add_destroy_listener_once(compositor,
							 &launcher->destroy_listener,
							 test_launcher_destroy)) {
		free(launcher);
		return -1;
	}

	if (wl_global_create(compositor->wl_display,
			     &weston_test_runner_interface, 1,
			     launcher, bind_runner) == NULL) {
		wl_list_remove(&launcher->destroy_listener.link);
		free(launcher);
		return -1;
	}

	return 0;
}

static void
runner_assert_fail(const char *cond, const char *file, int line,
		   const char *func, struct test_context *ctx)
{
	weston_log("Assert failure in %s:%d, %s: '%s'\n",
		   file, line, func, cond);

	assert(ctx->runner_resource);
	wl_resource_post_error(ctx->runner_resource,
			       WESTON_TEST_RUNNER_ERROR_TEST_FAILED,
			       "Assert failure in %s:%d, %s: '%s'\n",
			       file, line, func, cond);
}

#define runner_assert(cond) ({					\
	bool b_ = (cond);					\
	if (!b_)						\
		runner_assert_fail(#cond, __FILE__, __LINE__,	\
				   __func__, ctx);		\
	b_;							\
})

#define runner_assert_or_return(cond) do {			\
	bool b_ = (cond);					\
	if (!b_) {						\
		runner_assert_fail(#cond, __FILE__, __LINE__,	\
				   __func__, ctx);		\
		return;						\
	}							\
} while (0)


/*************************** tests **********************************/

/*
 * This is a IVI controller module requiring ivi-shell.so.
 * This module is specially written to execute tests that target the
 * ivi_layout API.
 *
 * The test program containing the fixture setup and initiating the tests is
 * test-ivi-layout-client (ivi-layout-test-client.c).
 * That program uses the weston-test-runner framework to execute each TEST()
 * in ivi-layout-test-client.c with a fresh connection to the single
 * compositor instance.
 *
 * Each TEST() in ivi-layout-test-client.c will bind to weston_test_runner global
 * interface. A TEST() will set up the client state, and issue
 * weston_test_runner.run request to execute the compositor-side of the test.
 *
 * The compositor-side parts of the tests are in this file. They are specified
 * by RUNNER_TEST() macro, where the name argument matches the name string
 * passed to weston_test_runner.run.
 *
 * A RUNNER_TEST() function simply returns when it succeeds. If it fails,
 * a fatal protocol error is sent to the client from runner_assert() or
 * runner_assert_or_return().
 *
 * A single TEST() in ivi-layout-test-client.c may use multiple RUNNER_TEST()s to
 * achieve multiple test points over a client action sequence.
 */

RUNNER_TEST(surface_create_p1)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf[2];
	uint32_t ivi_id;

	ivisurf[0] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf[0]);

	ivisurf[1] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(1));
	runner_assert(ivisurf[1]);

	ivi_id = lyt->get_id_of_surface(ivisurf[0]);
	runner_assert(ivi_id == IVI_TEST_SURFACE_ID(0));

	ivi_id = lyt->get_id_of_surface(ivisurf[1]);
	runner_assert(ivi_id == IVI_TEST_SURFACE_ID(1));
}

RUNNER_TEST(surface_create_p2)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	/* the ivi_surface was destroyed by the client */
	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf == NULL);
}

RUNNER_TEST(surface_visibility)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf);

	lyt->surface_set_visibility(ivisurf, true);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert(prop->visibility == true);
}

RUNNER_TEST(surface_opacity)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	int32_t ret;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert(prop->opacity == wl_fixed_from_double(1.0));

	ret = lyt->surface_set_opacity(ivisurf, wl_fixed_from_double(0.5));
	runner_assert(ret == IVI_SUCCEEDED);

	runner_assert(prop->opacity == wl_fixed_from_double(1.0));

	lyt->commit_changes();

	runner_assert(prop->opacity == wl_fixed_from_double(0.5));
}

RUNNER_TEST(surface_dimension)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_width == 1);
	runner_assert(prop->dest_height == 1);

	lyt->surface_set_destination_rectangle(ivisurf, prop->dest_x,
					       prop->dest_y, 200, 300);

	runner_assert(prop->dest_width == 1);
	runner_assert(prop->dest_height == 1);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_width == 200);
	runner_assert(prop->dest_height == 300);
}

RUNNER_TEST(surface_position)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_x == 0);
	runner_assert(prop->dest_y == 0);

	lyt->surface_set_destination_rectangle(ivisurf, 20, 30,
					       prop->dest_width,
					       prop->dest_height);

	runner_assert(prop->dest_x == 0);
	runner_assert(prop->dest_y == 0);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_x == 20);
	runner_assert(prop->dest_y == 30);
}

RUNNER_TEST(surface_destination_rectangle)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_width == 1);
	runner_assert(prop->dest_height == 1);
	runner_assert(prop->dest_x == 0);
	runner_assert(prop->dest_y == 0);

	lyt->surface_set_destination_rectangle(ivisurf, 20, 30, 200, 300);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_width == 1);
	runner_assert(prop->dest_height == 1);
	runner_assert(prop->dest_x == 0);
	runner_assert(prop->dest_y == 0);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->dest_width == 200);
	runner_assert(prop->dest_height == 300);
	runner_assert(prop->dest_x == 20);
	runner_assert(prop->dest_y == 30);
}

RUNNER_TEST(surface_source_rectangle)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->source_width == 0);
	runner_assert(prop->source_height == 0);
	runner_assert(prop->source_x == 0);
	runner_assert(prop->source_y == 0);

	lyt->surface_set_source_rectangle(ivisurf, 20, 30, 200, 300);

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->source_width == 0);
	runner_assert(prop->source_height == 0);
	runner_assert(prop->source_x == 0);
	runner_assert(prop->source_y == 0);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert_or_return(prop);
	runner_assert(prop->source_width == 200);
	runner_assert(prop->source_height == 300);
	runner_assert(prop->source_x == 20);
	runner_assert(prop->source_y == 30);
}

RUNNER_TEST(surface_bad_opacity)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	const struct ivi_layout_surface_properties *prop;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	runner_assert(lyt->surface_set_opacity(
		      ivisurf, wl_fixed_from_double(0.3)) == IVI_SUCCEEDED);

	runner_assert(lyt->surface_set_opacity(
		      ivisurf, wl_fixed_from_double(-1)) == IVI_FAILED);

	lyt->commit_changes();

	prop = lyt->get_properties_of_surface(ivisurf);
	runner_assert(prop->opacity == wl_fixed_from_double(0.3));

	runner_assert(lyt->surface_set_opacity(
		      ivisurf, wl_fixed_from_double(1.1)) == IVI_FAILED);

	lyt->commit_changes();

	runner_assert(prop->opacity == wl_fixed_from_double(0.3));

	lyt->commit_changes();
}

RUNNER_TEST(surface_on_many_layer)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;
	struct ivi_layout_layer *ivilayers[IVI_TEST_LAYER_COUNT] = {};
	struct ivi_layout_layer **array;
	int32_t length = 0;
	uint32_t i;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);

	for (i = 0; i < IVI_TEST_LAYER_COUNT; i++) {
		ivilayers[i] = lyt->layer_create_with_dimension(
				IVI_TEST_LAYER_ID(i), 200, 300);
		lyt->layer_add_surface(ivilayers[i], ivisurf);
	}

	lyt->commit_changes();

	lyt->get_layers_under_surface(ivisurf, &length, &array);
	runner_assert(IVI_TEST_LAYER_COUNT == length);
	for (i = 0; i < IVI_TEST_LAYER_COUNT; i++)
		runner_assert(array[i] == ivilayers[i]);

	if (length > 0)
		free(array);

	for (i = 0; i < IVI_TEST_LAYER_COUNT; i++)
		lyt->layer_remove_surface(ivilayers[i], ivisurf);

	array = NULL;

	lyt->commit_changes();

	lyt->get_layers_under_surface( ivisurf, &length, &array);
	runner_assert(length == 0 && array == NULL);

	for (i = 0; i < IVI_TEST_LAYER_COUNT; i++)
		lyt->layer_destroy(ivilayers[i]);
}

RUNNER_TEST(ivi_layout_commit_changes)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;

	lyt->commit_changes();
}

RUNNER_TEST(commit_changes_after_visibility_set_surface_destroy)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);
	lyt->surface_set_visibility(ivisurf, true);
}

RUNNER_TEST(commit_changes_after_opacity_set_surface_destroy)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);
	runner_assert(lyt->surface_set_opacity(
		      ivisurf, wl_fixed_from_double(0.5)) == IVI_SUCCEEDED);
}

RUNNER_TEST(commit_changes_after_source_rectangle_set_surface_destroy)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);
	lyt->surface_set_source_rectangle( ivisurf, 20, 30, 200, 300);
}

RUNNER_TEST(commit_changes_after_destination_rectangle_set_surface_destroy)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf != NULL);
	lyt->surface_set_destination_rectangle( ivisurf, 20, 30, 200, 300);
}

RUNNER_TEST(get_surface_after_destroy_surface)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf;

	ivisurf = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	runner_assert(ivisurf == NULL);
}

RUNNER_TEST(layer_render_order)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;
	struct ivi_layout_surface *ivisurfs[IVI_TEST_SURFACE_COUNT] = {};
	struct ivi_layout_surface **array;
	int32_t length = 0;
	uint32_t i;

	ivilayer = lyt->layer_create_with_dimension(IVI_TEST_LAYER_ID(0), 200, 300);

	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		ivisurfs[i] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(i));

	lyt->layer_set_render_order( ivilayer, ivisurfs, IVI_TEST_SURFACE_COUNT);

	lyt->commit_changes();

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(IVI_TEST_SURFACE_COUNT == length);
	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		runner_assert(array[i] == ivisurfs[i]);

	if (length > 0)
		free(array);

	lyt->layer_set_render_order(ivilayer, NULL, 0);

	array = NULL;

	lyt->commit_changes();

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(length == 0 && array == NULL);

	lyt->layer_destroy(ivilayer);
}

RUNNER_TEST(test_layer_render_order_destroy_one_surface_p1)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;
	struct ivi_layout_surface *ivisurfs[IVI_TEST_SURFACE_COUNT] = {};
	struct ivi_layout_surface **array;
	int32_t length = 0;
	int32_t i;

	ivilayer = lyt->layer_create_with_dimension(IVI_TEST_LAYER_ID(0), 200, 300);

	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		ivisurfs[i] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(i));

	lyt->layer_set_render_order(ivilayer, ivisurfs, IVI_TEST_SURFACE_COUNT);

	lyt->commit_changes();

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(IVI_TEST_SURFACE_COUNT == length);
	for (i = 0; i < length; i++)
		runner_assert(array[i] == ivisurfs[i]);

	if (length > 0)
		free(array);
}

RUNNER_TEST(test_layer_render_order_destroy_one_surface_p2)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;
	struct ivi_layout_surface *ivisurfs[2] = {};
	struct ivi_layout_surface **array;
	int32_t length = 0;
	int32_t i;

	ivilayer = lyt->get_layer_from_id(IVI_TEST_LAYER_ID(0));
	ivisurfs[0] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(0));
	ivisurfs[1] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(2));

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(2 == length);
	for (i = 0; i < length; i++)
		runner_assert(array[i] == ivisurfs[i]);

	if (length > 0)
		free(array);

	lyt->layer_destroy(ivilayer);
}

RUNNER_TEST(layer_add_surfaces)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;
	struct ivi_layout_surface *ivisurfs[IVI_TEST_SURFACE_COUNT] = {};
	struct ivi_layout_surface **array;
	int32_t length = 0;
	uint32_t i;

	ivilayer = lyt->layer_create_with_dimension(IVI_TEST_LAYER_ID(0), 200, 300);

	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++) {
		ivisurfs[i] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(i));
		lyt->layer_add_surface(ivilayer, ivisurfs[i]);
	}

	lyt->commit_changes();

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(IVI_TEST_SURFACE_COUNT == length);
	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		runner_assert(array[i] == ivisurfs[i]);

	if (length > 0)
		free(array);

	lyt->layer_set_render_order(ivilayer, NULL, 0);

	for (i = IVI_TEST_SURFACE_COUNT; i-- > 0;)
		lyt->layer_add_surface(ivilayer, ivisurfs[i]);

	lyt->commit_changes();

	lyt->get_surfaces_on_layer(ivilayer, &length, &array);
	runner_assert(IVI_TEST_SURFACE_COUNT == length);
	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		runner_assert(array[i] == ivisurfs[IVI_TEST_SURFACE_COUNT - (i + 1)]);

	if (length > 0)
		free(array);

	lyt->layer_destroy(ivilayer);
}

RUNNER_TEST(commit_changes_after_render_order_set_surface_destroy)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;
	struct ivi_layout_surface *ivisurfs[IVI_TEST_SURFACE_COUNT] = {};
	int i;

	ivilayer = lyt->layer_create_with_dimension(IVI_TEST_LAYER_ID(0), 200, 300);

	for (i = 0; i < IVI_TEST_SURFACE_COUNT; i++)
		ivisurfs[i] = lyt->get_surface_from_id(IVI_TEST_SURFACE_ID(i));

	lyt->layer_set_render_order(ivilayer, ivisurfs, IVI_TEST_SURFACE_COUNT);
}

RUNNER_TEST(cleanup_layer)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_layer *ivilayer;

	ivilayer = lyt->get_layer_from_id(IVI_TEST_LAYER_ID(0));
	lyt->layer_destroy(ivilayer);
}

static void
test_surface_properties_changed_notification_callback(struct wl_listener *listener, void *data)

{
	struct test_context *ctx =
			container_of(listener, struct test_context,
					surface_property_changed);
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf = data;

	runner_assert_or_return(lyt->get_id_of_surface(ivisurf) == IVI_TEST_SURFACE_ID(0));

	ctx->user_flags = 1;
}

RUNNER_TEST(surface_properties_changed_notification)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	const uint32_t id_surface = IVI_TEST_SURFACE_ID(0);
	struct ivi_layout_surface *ivisurf;

	ctx->user_flags = 0;

	ivisurf = lyt->get_surface_from_id(id_surface);
	runner_assert(ivisurf != NULL);

	ctx->surface_property_changed.notify = test_surface_properties_changed_notification_callback;

	lyt->surface_add_listener(ivisurf, &ctx->surface_property_changed);

	lyt->commit_changes();

	runner_assert(ctx->user_flags == 0);

	lyt->surface_set_destination_rectangle(ivisurf, 20, 30, 200, 300);

	lyt->commit_changes();

	runner_assert(ctx->user_flags == 1);

	ctx->user_flags = 0;
	lyt->surface_set_destination_rectangle(ivisurf, 20, 30, 200, 300);

	lyt->commit_changes();

	runner_assert(ctx->user_flags == 0);

	// remove surface property changed listener.
	wl_list_remove(&ctx->surface_property_changed.link);
	ctx->user_flags = 0;
	lyt->surface_set_destination_rectangle(ivisurf, 40, 50, 400, 500);

	lyt->commit_changes();

	runner_assert(ctx->user_flags == 0);
}

static void
test_surface_configure_notification_callback(struct wl_listener *listener, void *data)
{
	struct test_context *ctx =
			container_of(listener, struct test_context,
					surface_configured);
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf = data;

	runner_assert_or_return(lyt->get_id_of_surface(ivisurf) == IVI_TEST_SURFACE_ID(0));

	ctx->user_flags = 1;
}

RUNNER_TEST(surface_configure_notification_p1)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;

	ctx->surface_configured.notify = test_surface_configure_notification_callback;
	lyt->add_listener_configure_surface(&ctx->surface_configured);
	lyt->commit_changes();

	ctx->user_flags = 0;
}

RUNNER_TEST(surface_configure_notification_p2)
{
	runner_assert(ctx->user_flags == 1);

	// remove surface configured listener.
	wl_list_remove(&ctx->surface_configured.link);
	ctx->user_flags = 0;
}

RUNNER_TEST(surface_configure_notification_p3)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;

	lyt->commit_changes();
	runner_assert(ctx->user_flags == 0);
}

static void
test_surface_create_notification_callback(struct wl_listener *listener, void *data)
{
	struct test_context *ctx =
			container_of(listener, struct test_context,
					surface_created);
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf = data;

	runner_assert_or_return(lyt->get_id_of_surface(ivisurf) == IVI_TEST_SURFACE_ID(0));

	ctx->user_flags = 1;
}

RUNNER_TEST(surface_create_notification_p1)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;

	ctx->surface_created.notify = test_surface_create_notification_callback;
	lyt->add_listener_create_surface(&ctx->surface_created);

	ctx->user_flags = 0;
}

RUNNER_TEST(surface_create_notification_p2)
{
	runner_assert(ctx->user_flags == 1);

	// remove surface created listener.
	wl_list_remove(&ctx->surface_created.link);
	ctx->user_flags = 0;
}

RUNNER_TEST(surface_create_notification_p3)
{
	runner_assert(ctx->user_flags == 0);
}

static void
test_surface_remove_notification_callback(struct wl_listener *listener, void *data)
{
	struct test_context *ctx =
			container_of(listener, struct test_context,
					surface_removed);
	const struct ivi_layout_interface *lyt = ctx->layout_interface;
	struct ivi_layout_surface *ivisurf = data;

	runner_assert_or_return(lyt->get_id_of_surface(ivisurf) == IVI_TEST_SURFACE_ID(0));

	ctx->user_flags = 1;
}

RUNNER_TEST(surface_remove_notification_p1)
{
	const struct ivi_layout_interface *lyt = ctx->layout_interface;

	ctx->surface_removed.notify = test_surface_remove_notification_callback;
	lyt->add_listener_remove_surface(&ctx->surface_removed);

	ctx->user_flags = 0;
}

RUNNER_TEST(surface_remove_notification_p2)
{
	runner_assert(ctx->user_flags == 1);

	// remove surface removed listener.
	wl_list_remove(&ctx->surface_removed.link);
	ctx->user_flags = 0;
}

RUNNER_TEST(surface_remove_notification_p3)
{
	runner_assert(ctx->user_flags == 0);
}
