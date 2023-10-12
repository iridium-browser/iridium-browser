/*
 * Copyright 2020 Collabora, Ltd.
 * Copyright 2021 Advanced Micro Devices, Inc.
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

#include <math.h>
#include <stdio.h>

#include "weston-test-client-helper.h"
#include "weston-test-fixture-compositor.h"
#include "image-iter.h"
#include "color_util.h"

struct setup_args {
	struct fixture_metadata meta;
	enum weston_renderer_type renderer;
	bool color_management;
};

static const int ALPHA_STEPS = 256;
static const int BLOCK_WIDTH = 3;

static const struct setup_args my_setup_args[] = {
	{
		.renderer = WESTON_RENDERER_PIXMAN,
		.color_management = false,
		.meta.name = "pixman"
	},
	{
		.renderer = WESTON_RENDERER_GL,
		.color_management = false,
		.meta.name = "GL"
	},
	{
		.renderer = WESTON_RENDERER_GL,
		.color_management = true,
		.meta.name = "GL sRGB EOTF"
	},
};

static enum test_result_code
fixture_setup(struct weston_test_harness *harness, const struct setup_args *arg)
{
	struct compositor_setup setup;

	compositor_setup_defaults(&setup);
	setup.renderer = arg->renderer;
	setup.width = BLOCK_WIDTH * ALPHA_STEPS;
	setup.height = 16;
	setup.shell = SHELL_TEST_DESKTOP;

	if (arg->color_management) {
		weston_ini_setup(&setup,
				 cfgln("[core]"),
				 cfgln("color-management=true"));
	}

	return weston_test_harness_execute_as_client(harness, &setup);
}
DECLARE_FIXTURE_SETUP_WITH_ARG(fixture_setup, my_setup_args, meta);

static uint32_t
premult_color(uint32_t a, uint32_t r, uint32_t g, uint32_t b)
{
	uint32_t c = 0;

	c |= a << 24;
	c |= (a * r / 255) << 16;
	c |= (a * g / 255) << 8;
	c |= a * b / 255;

	return c;
}

static void
fill_alpha_pattern(struct buffer *buf)
{
	struct image_header ih = image_header_from(buf->image);
	int y;

	assert(ih.pixman_format == PIXMAN_a8r8g8b8);
	assert(ih.width == BLOCK_WIDTH * ALPHA_STEPS);

	for (y = 0; y < ih.height; y++) {
		uint32_t *row = image_header_get_row_u32(&ih, y);
		uint32_t step;

		for (step = 0; step < (uint32_t)ALPHA_STEPS; step++) {
			uint32_t alpha = step * 255 / (ALPHA_STEPS - 1);
			uint32_t color;
			int i;

			color = premult_color(alpha, 0, 255 - alpha, 255);
			for (i = 0; i < BLOCK_WIDTH; i++)
				*row++ = color;
		}
	}
}

enum blend_space {
	BLEND_NONLINEAR,
	BLEND_LINEAR,
};

static void
compare_sRGB_blend_a8r8g8b8(uint32_t bg32, uint32_t fg32, uint32_t dst32,
			    struct rgb_diff_stat *diffstat,
			    enum blend_space space)
{
	struct color_float bg = a8r8g8b8_to_float(bg32);
	struct color_float fg = a8r8g8b8_to_float(fg32);
	struct color_float dst = a8r8g8b8_to_float(dst32);
	struct color_float ref;
	int i;

	bg = color_float_unpremult(bg);
	fg = color_float_unpremult(fg);
	dst = color_float_unpremult(dst);

	if (space == BLEND_LINEAR) {
		sRGB_linearize(&bg);
		sRGB_linearize(&fg);
	}

	for (i = 0; i < COLOR_CHAN_NUM; i++)
		ref.rgb[i] = (1.0f - fg.a) * bg.rgb[i] + fg.a * fg.rgb[i];

	if (space == BLEND_LINEAR)
		sRGB_delinearize(&ref);

	rgb_diff_stat_update(diffstat, &ref, &dst, &fg);
}

static uint8_t
red(uint32_t v)
{
	return (v >> 16) & 0xff;
}

static uint8_t
blue(uint32_t v)
{
	return v & 0xff;
}

static bool
pixels_monotonic(const uint32_t *row, int x)
{
	bool ret = true;

	if (red(row[x + 1]) > red(row[x])) {
		testlog("pixel %d -> next: red value increases\n", x);
		ret = false;
	}

	if (blue(row[x + 1]) < blue(row[x])) {
		testlog("pixel %d -> next: blue value decreases\n", x);
		ret = false;
	}

	return ret;
}

static void *
get_middle_row(struct buffer *buf)
{
	struct image_header ih = image_header_from(buf->image);

	assert(ih.width >= BLOCK_WIDTH * ALPHA_STEPS);
	assert(ih.height >= BLOCK_WIDTH);

	return image_header_get_row_u32(&ih, (BLOCK_WIDTH - 1) / 2);
}

static bool
check_blend_pattern(struct buffer *bg, struct buffer *fg, struct buffer *shot,
		    enum blend_space space)
{
	FILE *dump = NULL;
#if 0
	/*
	 * This file can be loaded in Octave for visualization. Find the script
	 * in tests/visualization/weston_plot_rgb_diff_stat.m and call it with
	 *
	 * weston_plot_rgb_diff_stat('alpha_blend-f01-dump.txt', 255, 8)
	 */
	dump = fopen_dump_file("dump");
#endif

	/*
	 * Allow for +/- 1.5 code points of error in non-linear 8-bit channel
	 * value. This is necessary for the BLEND_LINEAR case.
	 *
	 * With llvmpipe, we could go as low as +/- 0.65 code points of error
	 * and still pass.
	 *
	 * AMD Polaris 11 would be ok with +/- 1.0 code points error threshold
	 * if not for one particular case of blending (a=254, r=0) into r=255,
	 * which results in error of 1.29 code points.
	 */
	const float tolerance = 1.5f / 255.f;

	uint32_t *bg_row = get_middle_row(bg);
	uint32_t *fg_row = get_middle_row(fg);
	uint32_t *shot_row = get_middle_row(shot);
	struct rgb_diff_stat diffstat = { .dump = dump, };
	bool ret = true;
	int x;

	for (x = 0; x < BLOCK_WIDTH * ALPHA_STEPS - 1; x++) {
		if (!pixels_monotonic(shot_row, x))
			ret = false;

		compare_sRGB_blend_a8r8g8b8(bg_row[x], fg_row[x], shot_row[x],
					    &diffstat, space);
	}

	if (diffstat.two_norm.max > tolerance)
		ret = false;

	rgb_diff_stat_print(&diffstat, __func__, 8);

	if (dump)
		fclose(dump);

	return ret;
}

/*
 * Test that alpha blending is roughly correct, and that an alpha ramp
 * results in a strictly monotonic color ramp. This should ensure that any
 * animation that varies alpha never goes "backwards" as that is easily
 * noticeable.
 *
 * The background is a constant color. On top of that, there is an
 * alpha-blended gradient with ramps in both alpha and color. Sub-surface
 * ensures the correct positioning and stacking.
 *
 * The gradient consists of ALPHA_STEPS number of blocks. Block size is
 * BLOCK_WIDTH x BLOCK_WIDTH and a block has a uniform color.
 *
 * In the blending result over x axis:
 * - red goes from 1.0 to 0.0, monotonic
 * - green is not monotonic
 * - blue goes from 0.0 to 1.0, monotonic
 *
 * This test has two modes: BLEND_NONLINEAR and BLEND_LINEAR.
 *
 * BLEND_NONLINEAR does blending with pixel values as is, which are non-linear,
 * and therefore result in "physically incorrect" blending result. Yet, people
 * have accustomed to seeing this effect. This mode hits pipeline_premult()
 * in fragment.glsl.
 *
 * BLEND_LINEAR has sRGB encoded pixels (non-linear). These are converted to
 * linear light (optical) values, blended, and converted back to non-linear
 * (electrical) values. This results in "physically more correct" blending
 * result for some value of "physical". This mode hits pipeline_straight()
 * in fragment.glsl, and tests even more things:
 * - gl-renderer implementation of 1D LUT is correct
 * - color-lcms instantiates the correct sRGB EOTF and inverse LUTs
 * - color space conversions do not happen when both content and output are
 *   using their default color spaces
 * - blending through gl-renderer shadow framebuffer
 */
TEST(alpha_blend)
{
	const int width = BLOCK_WIDTH * ALPHA_STEPS;
	const int height = BLOCK_WIDTH;
	const pixman_color_t background_color = {
		.red   = 0xffff,
		.green = 0x8080,
		.blue  = 0x0000,
		.alpha = 0xffff
	};
	const struct setup_args *args;
	struct client *client;
	struct buffer *bg;
	struct buffer *fg;
	struct wl_subcompositor *subco;
	struct wl_surface *surf;
	struct wl_subsurface *sub;
	struct buffer *shot;
	bool match;
	int seq_no;
	enum blend_space space;

	args = &my_setup_args[get_test_fixture_index()];
	if (args->color_management) {
		seq_no = 1;
		space = BLEND_LINEAR;
	} else {
		seq_no = 0;
		space = BLEND_NONLINEAR;
	}

	client = create_client();
	subco = bind_to_singleton_global(client, &wl_subcompositor_interface, 1);

	/* background window content */
	bg = create_shm_buffer_a8r8g8b8(client, width, height);
	fill_image_with_color(bg->image, &background_color);

	/* background window, main surface */
	client->surface = create_test_surface(client);
	client->surface->width = width;
	client->surface->height = height;
	client->surface->buffer = bg; /* pass ownership */
	surface_set_opaque_rect(client->surface,
				&(struct rectangle){ 0, 0, width, height });

	/* foreground blended content */
	fg = create_shm_buffer_a8r8g8b8(client, width, height);
	fill_alpha_pattern(fg);

	/* foreground window, sub-surface */
	surf = wl_compositor_create_surface(client->wl_compositor);
	sub = wl_subcompositor_get_subsurface(subco, surf, client->surface->wl_surface);
	/* sub-surface defaults to position 0, 0, top-most, synchronized */
	wl_surface_attach(surf, fg->proxy, 0, 0);
	wl_surface_damage(surf, 0, 0, width, height);
	wl_surface_commit(surf);

	/* attach, damage, commit background window */
	move_client(client, 0, 0);

	shot = capture_screenshot_of_output(client, NULL);
	assert(shot);
	match = verify_image(shot->image, "alpha_blend", seq_no, NULL, seq_no);
	assert(check_blend_pattern(bg, fg, shot, space));
	assert(match);

	buffer_destroy(shot);

	wl_subsurface_destroy(sub);
	wl_surface_destroy(surf);
	buffer_destroy(fg);
	wl_subcompositor_destroy(subco);
	client_destroy(client); /* destroys bg */
}
