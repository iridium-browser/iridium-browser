/*
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

#include <string.h>
#include <assert.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"
#include "backend.h"

static enum test_result_code
fixture_setup(struct weston_test_harness *harness)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = WESTON_RENDERER_GL;
	setup.shell = SHELL_TEST_DESKTOP;

	weston_ini_setup(&setup,
			 cfgln("[output]"),
			 cfgln("name=headless"),
			 cfgln("color_characteristics=my-awesome-color"),
			 cfgln("eotf-mode=st2084"),

			 cfgln("[color_characteristics]"),
			 cfgln("name=my-awesome-color"),
			 cfgln("maxFALL=1000"),
			 cfgln("red_x=0.9999"),
			 cfgln("red_y=0.3"),
			 cfgln("blue_x=0.1"),
			 cfgln("blue_y=0.11"),
			 cfgln("green_x=0.1771"),
			 cfgln("green_y=0.80001"),
			 cfgln("white_x=0.313"),
			 cfgln("white_y=0.323"),
			 cfgln("min_L=0.0001"),
			 cfgln("max_L=65535.0"),

			 cfgln("[core]"),
			 cfgln("color-management=true"));

	return weston_test_harness_execute_as_plugin(harness, &setup);
}
DECLARE_FIXTURE_SETUP(fixture_setup);

PLUGIN_TEST(color_characteristics_from_weston_ini)
{
	struct weston_output *output = NULL;
	struct weston_output *it;
	enum weston_eotf_mode mode;
	const struct weston_color_characteristics *cc;
	const struct weston_hdr_metadata_type1 *hdr_meta;

	wl_list_for_each(it, &compositor->output_list, link) {
		if (strcmp(it->name, "headless") == 0) {
			output = it;
			break;
		}
	}

	assert(output);

	mode = weston_output_get_eotf_mode(output);
	assert(mode == WESTON_EOTF_MODE_ST2084);

	cc = weston_output_get_color_characteristics(output);
	assert(cc->group_mask == WESTON_COLOR_CHARACTERISTICS_GROUP_ALL_MASK);
	assert(cc->primary[0].x == 0.9999f);
	assert(cc->primary[0].y == 0.3f);
	assert(cc->primary[1].x == 0.1771f);
	assert(cc->primary[1].y == 0.80001f);
	assert(cc->primary[2].x == 0.1f);
	assert(cc->primary[2].y == 0.11f);
	assert(cc->white.x == 0.313f);
	assert(cc->white.y == 0.323f);
	assert(cc->min_luminance == 0.0001f);
	assert(cc->max_luminance == 65535.0f);
	assert(cc->maxFALL == 1000.0f);

	/* The below is color manager policy. */
	hdr_meta = weston_output_get_hdr_metadata_type1(output);
	assert(hdr_meta->group_mask == WESTON_HDR_METADATA_TYPE1_GROUP_ALL_MASK);
	assert(hdr_meta->primary[0].x == 0.9999f);
	assert(hdr_meta->primary[0].y == 0.3f);
	assert(hdr_meta->primary[1].x == 0.1771f);
	assert(hdr_meta->primary[1].y == 0.80001f);
	assert(hdr_meta->primary[2].x == 0.1f);
	assert(hdr_meta->primary[2].y == 0.11f);
	assert(hdr_meta->white.x == 0.313f);
	assert(hdr_meta->white.y == 0.323f);
	assert(hdr_meta->minDML == 0.0001f);
	assert(hdr_meta->maxDML == 65535.0f);
	assert(hdr_meta->maxCLL == 65535.0f);
	assert(hdr_meta->maxFALL == 1000.0f);
}
