/*
 * Copyright © 2012 Collabora, Ltd.
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
#include "weston-test-fixture-compositor.h"

#define VERBOSE 0

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.shell = SHELL_TEST_DESKTOP;

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

#define NUM_SUBSURFACES 3

struct compound_surface {
	struct wl_subcompositor *subco;
	struct wl_surface *parent;
	struct wl_surface *child[NUM_SUBSURFACES];
	struct wl_subsurface *sub[NUM_SUBSURFACES];
};

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

static void
populate_compound_surface(struct compound_surface *com, struct client *client)
{
	int i;

	com->subco = get_subcompositor(client);

	com->parent = wl_compositor_create_surface(client->wl_compositor);

	for (i = 0; i < NUM_SUBSURFACES; i++) {
		com->child[i] =
			wl_compositor_create_surface(client->wl_compositor);
		com->sub[i] =
			wl_subcompositor_get_subsurface(com->subco,
							com->child[i],
							com->parent);
	}
}

static void
fini_compound_surface(struct compound_surface *com)
{
	int i;

	for (i = 0; i < NUM_SUBSURFACES; i++) {
		if (com->sub[i])
			wl_subsurface_destroy(com->sub[i]);
		if (com->child[i])
			wl_surface_destroy(com->child[i]);
	}

	wl_surface_destroy(com->parent);
	wl_subcompositor_destroy(com->subco);
}

TEST(test_subsurface_basic_protocol)
{
	struct client *client;
	struct compound_surface com1;
	struct compound_surface com2;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com1, client);
	populate_compound_surface(&com2, client);

	client_roundtrip(client);

	fini_compound_surface(&com1);
	fini_compound_surface(&com2);
	client_destroy(client);
}

TEST(test_subsurface_position_protocol)
{
	struct client *client;
	struct compound_surface com;
	int i;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);
	for (i = 0; i < NUM_SUBSURFACES; i++)
		wl_subsurface_set_position(com.sub[i],
					   (i + 2) * 20, (i + 2) * 10);

	client_roundtrip(client);

	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_placement_protocol)
{
	struct client *client;
	struct compound_surface com;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	wl_subsurface_place_above(com.sub[0], com.child[1]);
	wl_subsurface_place_above(com.sub[1], com.parent);
	wl_subsurface_place_below(com.sub[2], com.child[0]);
	wl_subsurface_place_below(com.sub[1], com.parent);

	client_roundtrip(client);

	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_paradox)
{
	struct client *client;
	struct wl_surface *parent;
	struct wl_subcompositor *subco;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	subco = get_subcompositor(client);
	parent = wl_compositor_create_surface(client->wl_compositor);

	/* surface is its own parent */
	sub = wl_subcompositor_get_subsurface(subco, parent, parent);

	expect_protocol_error(client, &wl_subcompositor_interface,
			      WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(parent);
	wl_subcompositor_destroy(subco);
	client_destroy(client);
}

TEST(test_subsurface_identical_link)
{
	struct client *client;
	struct compound_surface com;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	/* surface is already a subsurface */
	sub = wl_subcompositor_get_subsurface(com.subco, com.child[0], com.parent);

	expect_protocol_error(client, &wl_subcompositor_interface,
			      WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_change_link)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *stranger;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	stranger = wl_compositor_create_surface(client->wl_compositor);
	populate_compound_surface(&com, client);

	/* surface is already a subsurface */
	sub = wl_subcompositor_get_subsurface(com.subco, com.child[0], stranger);

	expect_protocol_error(client, &wl_subcompositor_interface,
			      WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE);

	fini_compound_surface(&com);
	wl_subsurface_destroy(sub);
	wl_surface_destroy(stranger);
	client_destroy(client);
}

TEST(test_subsurface_nesting)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *stranger;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	stranger = wl_compositor_create_surface(client->wl_compositor);
	populate_compound_surface(&com, client);

	/* parent is a sub-surface */
	sub = wl_subcompositor_get_subsurface(com.subco, stranger, com.child[0]);

	client_roundtrip(client);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(stranger);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_nesting_parent)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *stranger;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	stranger = wl_compositor_create_surface(client->wl_compositor);
	populate_compound_surface(&com, client);

	/* surface is already a parent */
	sub = wl_subcompositor_get_subsurface(com.subco, com.parent, stranger);

	client_roundtrip(client);

	wl_subsurface_destroy(sub);
	fini_compound_surface(&com);
	wl_surface_destroy(stranger);
	client_destroy(client);
}

TEST(test_subsurface_loop_paradox)
{
	struct client *client;
	struct wl_surface *surface[3];
	struct wl_subsurface *sub[3];
	struct wl_subcompositor *subco;
	unsigned i;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	subco = get_subcompositor(client);
	surface[0] = wl_compositor_create_surface(client->wl_compositor);
	surface[1] = wl_compositor_create_surface(client->wl_compositor);
	surface[2] = wl_compositor_create_surface(client->wl_compositor);

	/* create a nesting loop */
	sub[0] = wl_subcompositor_get_subsurface(subco, surface[1], surface[0]);
	sub[1] = wl_subcompositor_get_subsurface(subco, surface[2], surface[1]);
	sub[2] = wl_subcompositor_get_subsurface(subco, surface[0], surface[2]);

	expect_protocol_error(client, &wl_subcompositor_interface,
			      WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE);

	for (i = 0; i < ARRAY_LENGTH(sub); i++) {
		wl_subsurface_destroy(sub[i]);
		wl_surface_destroy(surface[i]);
	}

	wl_subcompositor_destroy(subco);
	client_destroy(client);
}

TEST(test_subsurface_place_above_nested_parent)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subcompositor *subco;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	wl_subsurface_place_above(sub, com.child[0]);

	client_roundtrip(client);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_above_grandparent)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subsurface *sub;
	struct wl_subcompositor *subco;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface above its grandparent */
	wl_subsurface_place_above(sub, com.parent);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_above_great_aunt)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subsurface *sub;
	struct wl_subcompositor *subco;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface above its parents' siblings */
	wl_subsurface_place_above(sub, com.child[1]);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_above_child)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subcompositor *subco;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface above its own child subsurface */
	wl_subsurface_place_above(com.sub[0], grandchild);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_below_nested_parent)
{
	struct client *client;
	struct compound_surface com;
	struct wl_subcompositor *subco;
	struct wl_surface *grandchild;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	wl_subsurface_place_below(sub, com.child[0]);

	client_roundtrip(client);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_below_grandparent)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subsurface *sub;
	struct wl_subcompositor *subco;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface below its grandparent */
	wl_subsurface_place_below(sub, com.parent);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_below_great_aunt)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subsurface *sub;
	struct wl_subcompositor *subco;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface below its parents' siblings */
	wl_subsurface_place_below(sub, com.child[1]);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_below_child)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *grandchild;
	struct wl_subcompositor *subco;
	struct wl_subsurface *sub;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	subco = get_subcompositor(client);
	grandchild = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, grandchild, com.child[0]);

	/* can't place a subsurface below its own child subsurface */
	wl_subsurface_place_below(com.sub[0], grandchild);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(grandchild);
	wl_subcompositor_destroy(subco);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_above_stranger)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *stranger;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	stranger = wl_compositor_create_surface(client->wl_compositor);
	populate_compound_surface(&com, client);

	/* bad sibling */
	wl_subsurface_place_above(com.sub[0], stranger);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_surface_destroy(stranger);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_below_stranger)
{
	struct client *client;
	struct compound_surface com;
	struct wl_surface *stranger;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	stranger = wl_compositor_create_surface(client->wl_compositor);
	populate_compound_surface(&com, client);

	/* bad sibling */
	wl_subsurface_place_below(com.sub[0], stranger);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	wl_surface_destroy(stranger);
	fini_compound_surface(&com);
	client_destroy(client);
}

TEST(test_subsurface_place_above_foreign)
{
	struct client *client;
	struct compound_surface com1;
	struct compound_surface com2;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com1, client);
	populate_compound_surface(&com2, client);

	/* bad sibling */
	wl_subsurface_place_above(com1.sub[0], com2.child[0]);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	fini_compound_surface(&com1);
	fini_compound_surface(&com2);
	client_destroy(client);
}

TEST(test_subsurface_place_below_foreign)
{
	struct client *client;
	struct compound_surface com1;
	struct compound_surface com2;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com1, client);
	populate_compound_surface(&com2, client);

	/* bad sibling */
	wl_subsurface_place_below(com1.sub[0], com2.child[0]);

	expect_protocol_error(client, &wl_subsurface_interface,
			      WL_SUBSURFACE_ERROR_BAD_SURFACE);

	fini_compound_surface(&com1);
	fini_compound_surface(&com2);
	client_destroy(client);
}

TEST(test_subsurface_destroy_protocol)
{
	struct client *client;
	struct compound_surface com;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	populate_compound_surface(&com, client);

	/* not needed anymore */
	wl_subcompositor_destroy(com.subco);

	/* detach child from parent */
	wl_subsurface_destroy(com.sub[0]);

	/* destroy: child, parent */
	wl_surface_destroy(com.child[1]);
	wl_surface_destroy(com.parent);

	/* destroy: parent, child */
	wl_surface_destroy(com.child[2]);

	/* destroy: sub, child */
	wl_surface_destroy(com.child[0]);

	/* 2x destroy: child, sub */
	wl_subsurface_destroy(com.sub[2]);
	wl_subsurface_destroy(com.sub[1]);

	client_roundtrip(client);

	client_destroy(client);
}

static void
create_subsurface_tree(struct client *client, struct wl_surface **surfs,
		       struct wl_subsurface **subs, int n)
{
	struct wl_subcompositor *subco;
	int i;

	subco = get_subcompositor(client);

	for (i = 0; i < n; i++)
		surfs[i] = wl_compositor_create_surface(client->wl_compositor);

	/*
	 * The tree of sub-surfaces:
	 *                            0
	 *                           / \
	 *                          1   2 - 10
	 *                         / \  |\
	 *                        3   5 9 6
	 *                       /       / \
	 *                      4       7   8
	 * Surface 0 has no wl_subsurface, others do.
	 */

	switch (n) {
	default:
		assert(0);
		break;

#define SUB_LINK(s,p) \
	subs[s] = wl_subcompositor_get_subsurface(subco, surfs[s], surfs[p])

	case 11:
		SUB_LINK(10, 2);
		/* fallthrough */
	case 10:
		SUB_LINK(9, 2);
		/* fallthrough */
	case 9:
		SUB_LINK(8, 6);
		/* fallthrough */
	case 8:
		SUB_LINK(7, 6);
		/* fallthrough */
	case 7:
		SUB_LINK(6, 2);
		/* fallthrough */
	case 6:
		SUB_LINK(5, 1);
		/* fallthrough */
	case 5:
		SUB_LINK(4, 3);
		/* fallthrough */
	case 4:
		SUB_LINK(3, 1);
		/* fallthrough */
	case 3:
		SUB_LINK(2, 0);
		/* fallthrough */
	case 2:
		SUB_LINK(1, 0);

#undef SUB_LINK
	};

	wl_subcompositor_destroy(subco);
}

static void
destroy_subsurface_tree(struct wl_surface **surfs,
			struct wl_subsurface **subs, int n)
{
	int i;

	for (i = n; i-- > 0; ) {
		if (surfs[i])
			wl_surface_destroy(surfs[i]);

		if (subs[i])
			wl_subsurface_destroy(subs[i]);
	}
}

static int
has_dupe(int *cnt, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (cnt[i] == cnt[n])
			return 1;

	return 0;
}

/* Note: number of permutations to test is: set_size! / (set_size - NSTEPS)!
 */
#define NSTEPS 3

struct permu_state {
	int set_size;
	int cnt[NSTEPS];
};

static void
permu_init(struct permu_state *s, int set_size)
{
	int i;

	s->set_size = set_size;
	for (i = 0; i < NSTEPS; i++)
		s->cnt[i] = 0;
}

static int
permu_next(struct permu_state *s)
{
	int k;

	s->cnt[NSTEPS - 1]++;

	while (1) {
		if (s->cnt[0] >= s->set_size) {
			return -1;
		}

		for (k = 1; k < NSTEPS; k++) {
			if (s->cnt[k] >= s->set_size) {
				s->cnt[k - 1]++;
				s->cnt[k] = 0;
				break;
			}

			if (has_dupe(s->cnt, k)) {
				s->cnt[k]++;
				break;
			}
		}

		if (k == NSTEPS)
			return 0;
	}
}

static void
destroy_permu_object(struct wl_surface **surfs,
		     struct wl_subsurface **subs, int i)
{
	int h = (i + 1) / 2;

	if (i & 1) {
		if (VERBOSE)
			testlog(" [sub  %2d]", h);
		wl_subsurface_destroy(subs[h]);
		subs[h] = NULL;
	} else {
		if (VERBOSE)
			testlog(" [surf %2d]", h);
		wl_surface_destroy(surfs[h]);
		surfs[h] = NULL;
	}
}

TEST(test_subsurface_destroy_permutations)
{
	/*
	 * Test wl_surface and wl_subsurface destruction orders in a
	 * complex tree of sub-surfaces.
	 *
	 * In the tree of sub-surfaces, go through every possible
	 * permutation of destroying all wl_surface and wl_subsurface
	 * objects. Execpt, to limit running time to a reasonable level,
	 * execute only the first NSTEPS destructions from each
	 * permutation, and ignore identical cases.
	 */

	const int test_size = 11;
	struct client *client;
	struct wl_surface *surfs[test_size];
	struct wl_subsurface *subs[test_size];
	struct permu_state per;
	int counter = 0;
	int i;

	client = create_client_and_test_surface(100, 50, 123, 77);
	assert(client);

	permu_init(&per, test_size * 2 - 1);
	while (permu_next(&per) != -1) {
		/* for each permutation of NSTEPS out of test_size */
		memset(surfs, 0, sizeof surfs);
		memset(subs, 0, sizeof subs);

		create_subsurface_tree(client, surfs, subs, test_size);

		if (VERBOSE)
			testlog("permu");

		if (VERBOSE)
			for (i = 0; i < NSTEPS; i++)
				testlog(" %2d", per.cnt[i]);

		for (i = 0; i < NSTEPS; i++)
			destroy_permu_object(surfs, subs, per.cnt[i]);

		if (VERBOSE)
			testlog("\n");
		client_roundtrip(client);

		destroy_subsurface_tree(surfs, subs, test_size);
		counter++;
	}

	client_roundtrip(client);
	testlog("tried %d destroy permutations\n", counter);

	client_destroy(client);
}
