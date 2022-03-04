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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "window.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"

struct clickdot {
	struct display *display;
	struct window *window;
	struct widget *widget;

	cairo_surface_t *buffer;

	struct {
		int32_t x, y;
	} dot;

	struct {
		int32_t x, y;
		int32_t old_x, old_y;
	} line;

	int reset;

	struct input *cursor_timeout_input;
	struct toytimer cursor_timeout;
};

static void
draw_line(struct clickdot *clickdot, cairo_t *cr,
	  struct rectangle *allocation)
{
	cairo_t *bcr;
	cairo_surface_t *tmp_buffer = NULL;

	if (clickdot->reset) {
		tmp_buffer = clickdot->buffer;
		clickdot->buffer = NULL;
		clickdot->line.x = -1;
		clickdot->line.y = -1;
		clickdot->line.old_x = -1;
		clickdot->line.old_y = -1;
		clickdot->reset = 0;
	}

	if (clickdot->buffer == NULL) {
		clickdot->buffer =
			cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
						   allocation->width,
						   allocation->height);
		bcr = cairo_create(clickdot->buffer);
		cairo_set_source_rgba(bcr, 0, 0, 0, 0);
		cairo_rectangle(bcr,
				0, 0,
				allocation->width, allocation->height);
		cairo_fill(bcr);
	}
	else
		bcr = cairo_create(clickdot->buffer);

	if (tmp_buffer) {
		cairo_set_source_surface(bcr, tmp_buffer, 0, 0);
		cairo_rectangle(bcr, 0, 0,
				allocation->width, allocation->height);
		cairo_clip(bcr);
		cairo_paint(bcr);

		cairo_surface_destroy(tmp_buffer);
	}

	if (clickdot->line.x != -1 && clickdot->line.y != -1) {
		if (clickdot->line.old_x != -1 &&
		    clickdot->line.old_y != -1) {
			cairo_set_line_width(bcr, 2.0);
			cairo_set_source_rgb(bcr, 1, 1, 1);
			cairo_translate(bcr,
					-allocation->x, -allocation->y);

			cairo_move_to(bcr,
				      clickdot->line.old_x,
				      clickdot->line.old_y);
			cairo_line_to(bcr,
				      clickdot->line.x,
				      clickdot->line.y);

			cairo_stroke(bcr);
		}

		clickdot->line.old_x = clickdot->line.x;
		clickdot->line.old_y = clickdot->line.y;
	}
	cairo_destroy(bcr);

	cairo_set_source_surface(cr, clickdot->buffer,
				 allocation->x, allocation->y);
	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
	cairo_rectangle(cr,
			allocation->x, allocation->y,
			allocation->width, allocation->height);
	cairo_clip(cr);
	cairo_paint(cr);
}

static void
redraw_handler(struct widget *widget, void *data)
{
	static const double r = 10.0;
	struct clickdot *clickdot = data;
	cairo_surface_t *surface;
	cairo_t *cr;
	struct rectangle allocation;

	widget_get_allocation(clickdot->widget, &allocation);

	surface = window_get_surface(clickdot->window);

	cr = cairo_create(surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.8);
	cairo_fill(cr);

	draw_line(clickdot, cr, &allocation);

	cairo_translate(cr, clickdot->dot.x + 0.5, clickdot->dot.y + 0.5);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 0.1, 0.9, 0.9);
	cairo_move_to(cr, 0.0, -r);
	cairo_line_to(cr, 0.0, r);
	cairo_move_to(cr, -r, 0.0);
	cairo_line_to(cr, r, 0.0);
	cairo_arc(cr, 0.0, 0.0, r, 0.0, 2.0 * M_PI);
	cairo_stroke(cr);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct clickdot *clickdot = data;

	window_schedule_redraw(clickdot->window);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym,
	    enum wl_keyboard_key_state state, void *data)
{
	struct clickdot *clickdot = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_Escape:
		display_exit(clickdot->display);
		break;
	}
}

static void
button_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       uint32_t button,
	       enum wl_pointer_button_state state, void *data)
{
	struct clickdot *clickdot = data;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED && button == BTN_LEFT)
		input_get_position(input, &clickdot->dot.x, &clickdot->dot.y);

	widget_schedule_redraw(widget);
}

static void
cursor_timeout_reset(struct clickdot *clickdot)
{
	toytimer_arm_once_usec(&clickdot->cursor_timeout, 500 * 1000);
}

static int
motion_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       float x, float y, void *data)
{
	struct clickdot *clickdot = data;
	clickdot->line.x = x;
	clickdot->line.y = y;

	window_schedule_redraw(clickdot->window);

	cursor_timeout_reset(clickdot);
	clickdot->cursor_timeout_input = input;

	return CURSOR_BLANK;
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height,
	       void *data)
{
	struct clickdot *clickdot = data;

	clickdot->reset = 1;
}

static void
leave_handler(struct widget *widget,
	      struct input *input, void *data)
{
	struct clickdot *clickdot = data;

	clickdot->reset = 1;
}

static void
cursor_timeout_func(struct toytimer *tt)
{
	struct clickdot *clickdot =
		container_of(tt, struct clickdot, cursor_timeout);

	input_set_pointer_image(clickdot->cursor_timeout_input,
				CURSOR_LEFT_PTR);
}

static struct clickdot *
clickdot_create(struct display *display)
{
	struct clickdot *clickdot;

	clickdot = xzalloc(sizeof *clickdot);
	clickdot->window = window_create(display);
	clickdot->widget = window_frame_create(clickdot->window, clickdot);
	window_set_title(clickdot->window, "Wayland ClickDot");
	clickdot->display = display;
	clickdot->buffer = NULL;

	window_set_key_handler(clickdot->window, key_handler);
	window_set_user_data(clickdot->window, clickdot);
	window_set_keyboard_focus_handler(clickdot->window,
					  keyboard_focus_handler);

	widget_set_redraw_handler(clickdot->widget, redraw_handler);
	widget_set_button_handler(clickdot->widget, button_handler);
	widget_set_motion_handler(clickdot->widget, motion_handler);
	widget_set_resize_handler(clickdot->widget, resize_handler);
	widget_set_leave_handler(clickdot->widget, leave_handler);

	widget_schedule_resize(clickdot->widget, 500, 400);
	clickdot->dot.x = 250;
	clickdot->dot.y = 200;
	clickdot->line.x = -1;
	clickdot->line.y = -1;
	clickdot->line.old_x = -1;
	clickdot->line.old_y = -1;
	clickdot->reset = 0;

	toytimer_init(&clickdot->cursor_timeout, CLOCK_MONOTONIC,
		      display, cursor_timeout_func);

	return clickdot;
}

static void
clickdot_destroy(struct clickdot *clickdot)
{
	toytimer_fini(&clickdot->cursor_timeout);
	if (clickdot->buffer)
		cairo_surface_destroy(clickdot->buffer);
	widget_destroy(clickdot->widget);
	window_destroy(clickdot->window);
	free(clickdot);
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct clickdot *clickdot;

	display = display_create(&argc, argv);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %s\n",
			strerror(errno));
		return -1;
	}

	clickdot = clickdot_create(display);

	display_run(display);

	clickdot_destroy(clickdot);
	display_destroy(display);

	return 0;
}
