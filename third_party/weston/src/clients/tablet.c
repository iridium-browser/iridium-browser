/*
 * Copyright © 2014 Lyude
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE
 */
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "window.h"
#include "tablet-unstable-v2-client-protocol.h"

struct display *display;
struct window *window;
struct widget *widget;

cairo_surface_t *draw_buffer;

int old_x, old_y;
int current_x, current_y;
enum zwp_tablet_tool_v2_type tool_type;

bool tablet_is_down;

double current_pressure;

#define WL_TABLET_AXIS_MAX 65535

static void
redraw_handler(struct widget *widget, void *data)
{
	cairo_surface_t *surface;
	cairo_t *window_cr, *drawing_cr;
	struct rectangle allocation;

	widget_get_allocation(widget, &allocation);

	surface = window_get_surface(window);

	/* Setup the background */
	window_cr = cairo_create(surface);
	cairo_set_operator(window_cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(window_cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba(window_cr, 0, 0, 0, 0.8);
	cairo_fill(window_cr);

	/* Update the drawing buffer */
	if (tablet_is_down) {
		if (old_x != -1 && old_y != -1) {
			drawing_cr = cairo_create(draw_buffer);
			if (tool_type == ZWP_TABLET_TOOL_V2_TYPE_PEN) {
				cairo_set_source_rgb(drawing_cr, 1, 1, 1);
				cairo_set_line_width(drawing_cr,
						     current_pressure /
						     WL_TABLET_AXIS_MAX * 7 + 1);
			} else if (tool_type == ZWP_TABLET_TOOL_V2_TYPE_ERASER) {
				cairo_set_operator(drawing_cr, CAIRO_OPERATOR_CLEAR);
				cairo_set_source_rgb(drawing_cr, 0, 0, 0);
				cairo_set_line_width(drawing_cr,
						     current_pressure /
						     WL_TABLET_AXIS_MAX * 30 + 10);
			}

			cairo_set_line_cap(drawing_cr, CAIRO_LINE_CAP_ROUND);

			cairo_translate(drawing_cr,
					-allocation.x,
					-allocation.y);
			cairo_move_to(drawing_cr, old_x, old_y);
			cairo_line_to(drawing_cr, current_x, current_y);
			cairo_stroke(drawing_cr);

			cairo_destroy(drawing_cr);
		}

		old_x = current_x;
		old_y = current_y;
	}

	/* Squash the drawing buffer onto the window's buffer */
	cairo_set_source_surface(window_cr,
				 draw_buffer,
				 allocation.x,
				 allocation.y);
	cairo_set_operator(window_cr, CAIRO_OPERATOR_ADD);
	cairo_rectangle(window_cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_clip(window_cr);
	cairo_paint(window_cr);

	cairo_destroy(window_cr);

	cairo_surface_destroy(surface);
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height,
	       void *data)
{
	cairo_surface_t *tmp_buffer;
	cairo_t *cr;

	tmp_buffer = draw_buffer;
	draw_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
						 width, height);
	cr = cairo_create(draw_buffer);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	if (tmp_buffer) {
		cairo_set_source_surface(cr, tmp_buffer, 0, 0);
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_clip(cr);
		cairo_paint(cr);
	}

	cairo_destroy(cr);

	cairo_surface_destroy(tmp_buffer);
}

static void
proximity_in_handler(struct widget *widget, struct tablet_tool *tool,
		     struct tablet *tablet, void *data)
{
	tool_type = tablet_tool_get_type(tool);
}

static void
pressure_handler(struct widget *widget, struct tablet_tool *tool,
		 uint32_t pressure, void *data)
{
	current_pressure = pressure;
}

static int
tablet_motion_handler(struct widget *widget, struct tablet_tool *tool,
		      float x, float y, void *data)
{
	int cursor;

	current_x = x;
	current_y = y;

	if (tablet_is_down) {
		widget_schedule_redraw(widget);
		cursor = CURSOR_HAND1;
	} else {
		cursor = CURSOR_LEFT_PTR;
	}

	return cursor;
}

static void
tablet_down_handler(struct widget *widget, struct tablet_tool *tool, void *data)
{
	tablet_is_down = true;
}

static void
tablet_up_handler(struct widget *widget, struct tablet_tool *tool, void *data)
{
	tablet_is_down = false;
	old_x = -1;
	old_y = -1;
}

static void
init_globals(void)
{
	window = window_create(display);
	widget = window_frame_create(window, NULL);
	window_set_title(window, "Wayland Tablet Demo");
	old_x = -1;
	old_y = -1;

	widget_set_tablet_tool_axis_handlers(widget,
					     tablet_motion_handler,
					     pressure_handler,
					     NULL, NULL,
					     NULL, NULL, NULL);
	widget_set_tablet_tool_down_handler(widget, tablet_down_handler);
	widget_set_tablet_tool_up_handler(widget, tablet_up_handler);
	widget_set_tablet_tool_proximity_handlers(widget,
						  proximity_in_handler,
						  NULL);
	widget_set_redraw_handler(widget, redraw_handler);
	widget_set_resize_handler(widget, resize_handler);

	widget_schedule_resize(widget, 1000, 800);
}

static void
cleanup(void)
{
	widget_destroy(widget);
	window_destroy(window);
}

int
main(int argc, char *argv[])
{
	display = display_create(&argc, argv);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %m\n");
		return -1;
	}

	init_globals();

	display_run(display);

	cleanup();

	display_destroy(display);

	return 0;
}
