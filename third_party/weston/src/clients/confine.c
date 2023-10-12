/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2012 Collabora, Ltd.
 * Copyright © 2012 Jonas Ådahl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "window.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"

#define NUM_COMPLEX_REGION_RECTS 9

static bool option_complex_confine_region;
static bool option_help;

struct confine {
	struct display *display;
	struct window *window;
	struct widget *widget;

	cairo_surface_t *buffer;

	struct {
		int32_t x, y;
		int32_t old_x, old_y;
	} line;

	int reset;

	struct input *cursor_timeout_input;
	struct toytimer cursor_timeout;

	bool pointer_confined;

	bool complex_confine_region_enabled;
	bool complex_confine_region_dirty;
	struct rectangle complex_confine_region[NUM_COMPLEX_REGION_RECTS];
};

static void
draw_line(struct confine *confine, cairo_t *cr,
	  struct rectangle *allocation)
{
	cairo_t *bcr;
	cairo_surface_t *tmp_buffer = NULL;

	if (confine->reset) {
		tmp_buffer = confine->buffer;
		confine->buffer = NULL;
		confine->line.x = -1;
		confine->line.y = -1;
		confine->line.old_x = -1;
		confine->line.old_y = -1;
		confine->reset = 0;
	}

	if (confine->buffer == NULL) {
		confine->buffer =
			cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
						   allocation->width,
						   allocation->height);
		bcr = cairo_create(confine->buffer);
		cairo_set_source_rgba(bcr, 0, 0, 0, 0);
		cairo_rectangle(bcr,
				0, 0,
				allocation->width, allocation->height);
		cairo_fill(bcr);
	}
	else
		bcr = cairo_create(confine->buffer);

	if (tmp_buffer) {
		cairo_set_source_surface(bcr, tmp_buffer, 0, 0);
		cairo_rectangle(bcr, 0, 0,
				allocation->width, allocation->height);
		cairo_clip(bcr);
		cairo_paint(bcr);

		cairo_surface_destroy(tmp_buffer);
	}

	if (confine->line.x != -1 && confine->line.y != -1) {
		if (confine->line.old_x != -1 &&
		    confine->line.old_y != -1) {
			cairo_set_line_width(bcr, 2.0);
			cairo_set_source_rgb(bcr, 1, 1, 1);
			cairo_translate(bcr,
					-allocation->x, -allocation->y);

			cairo_move_to(bcr,
				      confine->line.old_x,
				      confine->line.old_y);
			cairo_line_to(bcr,
				      confine->line.x,
				      confine->line.y);

			cairo_stroke(bcr);
		}

		confine->line.old_x = confine->line.x;
		confine->line.old_y = confine->line.y;
	}
	cairo_destroy(bcr);

	cairo_set_source_surface(cr, confine->buffer,
				 allocation->x, allocation->y);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_rectangle(cr,
			allocation->x, allocation->y,
			allocation->width, allocation->height);
	cairo_clip(cr);
	cairo_paint(cr);
}

static void
calculate_complex_confine_region(struct confine *confine)
{
	struct rectangle allocation;
	int32_t x, y, w, h;
	struct rectangle *rs = confine->complex_confine_region;

	if (!confine->complex_confine_region_dirty)
		return;

	widget_get_allocation(confine->widget, &allocation);
	x = allocation.x;
	y = allocation.y;
	w = allocation.width;
	h = allocation.height;

	/*
	 * The code below constructs a region made up of rectangles that
	 * is then used to set up both an illustrative shaded region in the
	 * widget and a confine region used when confining the pointer.
	 */

	rs[0].x = x + (int)round(w * 0.05);
	rs[0].y = y + (int)round(h * 0.15);
	rs[0].width = (int)round(w * 0.35);
	rs[0].height = (int)round(h * 0.7);

	rs[1].x = rs[0].x + rs[0].width;
	rs[1].y = y + (int)round(h * 0.45);
	rs[1].width = (int)round(w * 0.09);
	rs[1].height = (int)round(h * 0.1);

	rs[2].x = rs[1].x + rs[1].width;
	rs[2].y = y + (int)round(h * 0.48);
	rs[2].width = (int)round(w * 0.02);
	rs[2].height = (int)round(h * 0.04);

	rs[3].x = rs[2].x + rs[2].width;
	rs[3].y = y + (int)round(h * 0.45);
	rs[3].width = (int)round(w * 0.09);
	rs[3].height = (int)round(h * 0.1);

	rs[4].x = rs[3].x + rs[3].width;
	rs[4].y = y + (int)round(h * 0.15);
	rs[4].width = (int)round(w * 0.35);
	rs[4].height = (int)round(h * 0.7);

	rs[5].x = x + (int)round(w * 0.05);
	rs[5].y = y + (int)round(h * 0.05);
	rs[5].width = rs[0].width + rs[1].width + rs[2].width +
		rs[3].width + rs[4].width;
	rs[5].height = (int)round(h * 0.10);

	rs[6].x = x + (int)round(w * 0.1);
	rs[6].y = rs[4].y + rs[4].height + (int)round(h * 0.02);
	rs[6].width = (int)round(w * 0.8);
	rs[6].height = (int)round(h * 0.03);

	rs[7].x = x + (int)round(w * 0.05);
	rs[7].y = rs[6].y + rs[6].height;
	rs[7].width = (int)round(w * 0.9);
	rs[7].height = (int)round(h * 0.03);

	rs[8].x = x + (int)round(w * 0.1);
	rs[8].y = rs[7].y + rs[7].height;
	rs[8].width = (int)round(w * 0.8);
	rs[8].height = (int)round(h * 0.03);

	confine->complex_confine_region_dirty = false;
}

static void
draw_complex_confine_region_mask(struct confine *confine, cairo_t *cr)
{
	int i;

	calculate_complex_confine_region(confine);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	for (i = 0; i < NUM_COMPLEX_REGION_RECTS; i++) {
		cairo_rectangle(cr,
				confine->complex_confine_region[i].x,
				confine->complex_confine_region[i].y,
				confine->complex_confine_region[i].width,
				confine->complex_confine_region[i].height);
		cairo_set_source_rgba(cr, 0.14, 0.14, 0.14, 0.9);
		cairo_fill(cr);
	}
}

static void
redraw_handler(struct widget *widget, void *data)
{
	struct confine *confine = data;
	cairo_surface_t *surface;
	cairo_t *cr;
	struct rectangle allocation;

	widget_get_allocation(confine->widget, &allocation);

	surface = window_get_surface(confine->window);

	cr = cairo_create(surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.8);
	cairo_fill(cr);

	if (confine->complex_confine_region_enabled) {
		draw_complex_confine_region_mask(confine, cr);
	}

	draw_line(confine, cr, &allocation);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct confine *confine = data;

	window_schedule_redraw(confine->window);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym,
	    enum wl_keyboard_key_state state, void *data)
{
	struct confine *confine = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_Escape:
		display_exit(confine->display);
		break;
	case XKB_KEY_BackSpace:
		cairo_surface_destroy(confine->buffer);
		confine->buffer = NULL;
		window_schedule_redraw(confine->window);
		break;
	case XKB_KEY_m:
		window_set_maximized(confine->window,
				     !window_is_maximized(window));
		break;
	}
}

static void
toggle_pointer_confine(struct confine *confine, struct input *input)
{
	if (confine->pointer_confined) {
		window_unconfine_pointer(confine->window);
	} else if (confine->complex_confine_region_enabled) {
		calculate_complex_confine_region(confine);
		window_confine_pointer_to_rectangles(
				confine->window,
				input,
				confine->complex_confine_region,
				NUM_COMPLEX_REGION_RECTS);

	} else {
		window_confine_pointer_to_widget(confine->window,
						 confine->widget,
						 input);
	}

	confine->pointer_confined = !confine->pointer_confined;
}

static void
button_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       uint32_t button,
	       enum wl_pointer_button_state state, void *data)
{
	struct confine *confine = data;
	bool is_pressed = state == WL_POINTER_BUTTON_STATE_PRESSED;

	if (is_pressed && button == BTN_LEFT)
		toggle_pointer_confine(confine, input);
	widget_schedule_redraw(widget);
}

static void
cursor_timeout_reset(struct confine *confine)
{
	toytimer_arm_once_usec(&confine->cursor_timeout, 500 * 1000);
}

static int
motion_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       float x, float y, void *data)
{
	struct confine *confine = data;
	confine->line.x = x;
	confine->line.y = y;

	window_schedule_redraw(confine->window);

	cursor_timeout_reset(confine);
	confine->cursor_timeout_input = input;

	return CURSOR_BLANK;
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height,
	       void *data)
{
	struct confine *confine = data;

	confine->reset = 1;

	if (confine->complex_confine_region_enabled) {
		confine->complex_confine_region_dirty = true;

		if (confine->pointer_confined) {
			calculate_complex_confine_region(confine);
			window_update_confine_rectangles(
					confine->window,
					confine->complex_confine_region,
					NUM_COMPLEX_REGION_RECTS);
		}
	}
}

static void
leave_handler(struct widget *widget,
	      struct input *input, void *data)
{
	struct confine *confine = data;

	confine->reset = 1;
}

static void
cursor_timeout_func(struct toytimer *tt)
{
	struct confine *confine =
		container_of(tt, struct confine, cursor_timeout);

	input_set_pointer_image(confine->cursor_timeout_input,
				CURSOR_LEFT_PTR);
}

static void
pointer_unconfined(struct window *window, struct input *input, void *data)
{
	struct confine *confine = data;

	confine->pointer_confined = false;
}

static struct confine *
confine_create(struct display *display)
{
	struct confine *confine;

	confine = xzalloc(sizeof *confine);
	confine->window = window_create(display);
	confine->widget = window_frame_create(confine->window, confine);
	window_set_title(confine->window, "Wayland Confine");
	window_set_appid(confine->window,
			"org.freedesktop.weston.wayland-confine");
	confine->display = display;
	confine->buffer = NULL;

	window_set_key_handler(confine->window, key_handler);
	window_set_user_data(confine->window, confine);
	window_set_keyboard_focus_handler(confine->window,
					  keyboard_focus_handler);
	window_set_pointer_confined_handler(confine->window,
					    NULL,
					    pointer_unconfined);

	widget_set_redraw_handler(confine->widget, redraw_handler);
	widget_set_button_handler(confine->widget, button_handler);
	widget_set_motion_handler(confine->widget, motion_handler);
	widget_set_resize_handler(confine->widget, resize_handler);
	widget_set_leave_handler(confine->widget, leave_handler);

	widget_schedule_resize(confine->widget, 500, 400);
	confine->line.x = -1;
	confine->line.y = -1;
	confine->line.old_x = -1;
	confine->line.old_y = -1;
	confine->reset = 0;

	toytimer_init(&confine->cursor_timeout, CLOCK_MONOTONIC,
		      display, cursor_timeout_func);

	return confine;
}

static void
confine_destroy(struct confine *confine)
{
	toytimer_fini(&confine->cursor_timeout);
	if (confine->buffer)
		cairo_surface_destroy(confine->buffer);
	widget_destroy(confine->widget);
	window_destroy(confine->window);
	free(confine);
}

static const struct weston_option confine_options[] = {
	{ WESTON_OPTION_BOOLEAN, "complex-confine-region", 0, &option_complex_confine_region },
	{ WESTON_OPTION_BOOLEAN, "help", 0, &option_help },
};

static void
print_help(const char *argv0)
{
	printf("Usage: %s [--complex-confine-region]\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct confine *confine;

	if (parse_options(confine_options,
			  ARRAY_LENGTH(confine_options),
			  &argc, argv) > 1 ||
	    option_help) {
		print_help(argv[0]);
		return 0;
	}

	display = display_create(&argc, argv);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %s\n",
			strerror(errno));
		return -1;
	}

	confine = confine_create(display);

	if (option_complex_confine_region) {
		confine->complex_confine_region_dirty = true;
		confine->complex_confine_region_enabled = true;
	}

	display_run(display);

	confine_destroy(confine);
	display_destroy(display);

	return 0;
}
