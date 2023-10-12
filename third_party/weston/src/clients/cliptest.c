/*
 * Copyright © 2012 Collabora, Ltd.
 * Copyright © 2012 Rob Clark
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

/* cliptest: for debugging calculate_edges() function.
 * controls:
 *	clip box position: mouse left drag, keys: w a s d
 *	clip box size: mouse right drag, keys: i j k l
 *	surface orientation: mouse wheel, keys: n m
 *	surface transform disable key: r
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <pixman.h>
#include <cairo.h>
#include <float.h>
#include <assert.h>
#include <errno.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "libweston/vertex-clipping.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "window.h"

typedef float GLfloat;

struct geometry {
	pixman_box32_t clip;

	pixman_box32_t surf;
	float s; /* sin phi */
	float c; /* cos phi */
	float phi;
};

struct weston_surface {
};

struct weston_view {
	struct weston_surface *surface;
	struct {
		int enabled;
	} transform;

	struct geometry *geometry;
};

static void
weston_view_to_global_double(struct weston_view *view,
			     double sx, double sy, double *x, double *y)
{
	struct geometry *g = view->geometry;

	/* pure rotation around origin by sine and cosine */
	*x = g->c * sx + g->s * sy;
	*y = -g->s * sx + g->c * sy;
}

static struct weston_coord_global
weston_coord_surface_to_global(struct weston_view *view, struct weston_coord_surface pos)
{
	double gx, gy;
	struct weston_coord_global g_pos;

	weston_view_to_global_double(view, pos.c.x, pos.c.y, &gx, &gy);
	g_pos.c = weston_coord(gx, gy);

	return g_pos;
}

/* ---------------------- copied begins -----------------------*/
/* Keep this in sync with what is in gl-renderer.c! */

/*
 * Compute the boundary vertices of the intersection of the global coordinate
 * aligned rectangle 'rect', and an arbitrary quadrilateral produced from
 * 'surf_rect' when transformed from surface coordinates into global coordinates.
 * The vertices are written to 'ex' and 'ey', and the return value is the
 * number of vertices. Vertices are produced in clockwise winding order.
 * Guarantees to produce either zero vertices, or 3-8 vertices with non-zero
 * polygon area.
 */
static int
calculate_edges(struct weston_view *ev, pixman_box32_t *rect,
		pixman_box32_t *surf_rect, struct weston_coord *e)
{

	struct clip_context ctx;
	int i, n;
	GLfloat min_x, max_x, min_y, max_y;
	struct weston_surface *es = ev->surface;
	struct weston_coord_surface tmp[4] = {
		weston_coord_surface(surf_rect->x1, surf_rect->y1, es),
		weston_coord_surface(surf_rect->x2, surf_rect->y1, es),
		weston_coord_surface(surf_rect->x2, surf_rect->y2, es),
		weston_coord_surface(surf_rect->x1, surf_rect->y2, es),
	};
	struct polygon8 surf;

	surf.n = 4;

	ctx.clip.x1 = rect->x1;
	ctx.clip.y1 = rect->y1;
	ctx.clip.x2 = rect->x2;
	ctx.clip.y2 = rect->y2;

	/* transform surface to screen space: */
	for (i = 0; i < surf.n; i++)
		surf.pos[i] = weston_coord_surface_to_global(ev, tmp[i]).c;

	/* find bounding box: */
	min_x = max_x = surf.pos[0].x;
	min_y = max_y = surf.pos[0].y;

	for (i = 1; i < surf.n; i++) {
		min_x = MIN(min_x, surf.pos[i].x);
		max_x = MAX(max_x, surf.pos[i].x);
		min_y = MIN(min_y, surf.pos[i].y);
		max_y = MAX(max_y, surf.pos[i].y);
	}

	/* First, simple bounding box check to discard early transformed
	 * surface rects that do not intersect with the clip region:
	 */
	if ((min_x >= ctx.clip.x2) || (max_x <= ctx.clip.x1) ||
	    (min_y >= ctx.clip.y2) || (max_y <= ctx.clip.y1))
		return 0;

	/* Simple case, bounding box edges are parallel to surface edges,
	 * there will be only four edges.  We just need to clip the surface
	 * vertices to the clip rect bounds:
	 */
	if (!ev->transform.enabled)
		return clip_simple(&ctx, &surf, e);

	/* Transformed case: use a general polygon clipping algorithm to
	 * clip the surface rectangle with each side of 'rect'.
	 * The algorithm is Sutherland-Hodgman, as explained in
	 * http://www.codeguru.com/cpp/misc/misc/graphics/article.php/c8965/Polygon-Clipping.htm
	 * but without looking at any of that code.
	 */
	n = clip_transformed(&ctx, &surf, e);

	if (n < 3)
		return 0;

	return n;
}

/* ---------------------- copied ends -----------------------*/

static void
geometry_set_phi(struct geometry *g, float phi)
{
	g->phi = phi;
	g->s = sin(phi);
	g->c = cos(phi);
}

static void
geometry_init(struct geometry *g)
{
	g->clip.x1 = -50;
	g->clip.y1 = -50;
	g->clip.x2 = -10;
	g->clip.y2 = -10;

	g->surf.x1 = -20;
	g->surf.y1 = -20;
	g->surf.x2 = 20;
	g->surf.y2 = 20;

	geometry_set_phi(g, 0.0);
}

struct ui_state {
	uint32_t button;
	int down;

	int down_pos[2];
	struct geometry geometry;
};

struct cliptest {
	struct window *window;
	struct widget *widget;
	struct display *display;
	int fullscreen;

	struct ui_state ui;

	struct geometry geometry;
	struct weston_surface surface;
	struct weston_view view;
};

static void
draw_polygon_closed(cairo_t *cr, struct weston_coord *pos, int n)
{
	int i;

	cairo_move_to(cr, pos[0].x, pos[0].y);
	for (i = 1; i < n; i++)
		cairo_line_to(cr, pos[i].x, pos[i].y);
	cairo_line_to(cr, pos[0].x, pos[0].y);
}

static void
draw_polygon_labels(cairo_t *cr, struct weston_coord *pos, int n)
{
	char str[16];
	int i;

	for (i = 0; i < n; i++) {
		snprintf(str, 16, "%d", i);
		cairo_move_to(cr, pos[i].x, pos[i].y);
		cairo_show_text(cr, str);
	}
}

static void
draw_coordinates(cairo_t *cr, double ox, double oy, struct weston_coord *pos, int n)
{
	char str[64];
	int i;
	cairo_font_extents_t ext;

	cairo_font_extents(cr, &ext);
	for (i = 0; i < n; i++) {
		snprintf(str, 64, "%d: %14.9f, %14.9f", i, pos[i].x, pos[i].y);
		cairo_move_to(cr, ox, oy + ext.height * (i + 1));
		cairo_show_text(cr, str);
	}
}

static void
draw_box(cairo_t *cr, pixman_box32_t *box, struct weston_view *view)
{
	struct weston_coord pos[4];

	if (view) {
		weston_view_to_global_double(view, box->x1, box->y1, &pos[0].x, &pos[0].y);
		weston_view_to_global_double(view, box->x2, box->y1, &pos[1].x, &pos[1].y);
		weston_view_to_global_double(view, box->x2, box->y2, &pos[2].x, &pos[2].y);
		weston_view_to_global_double(view, box->x1, box->y2, &pos[3].x, &pos[3].y);
	} else {
		pos[0] = weston_coord(box->x1, box->y1);
		pos[1] = weston_coord(box->x2, box->y1);
		pos[2] = weston_coord(box->x2, box->y2);
		pos[3] = weston_coord(box->x1, box->y2);
	}

	draw_polygon_closed(cr, pos, 4);
}

static void
draw_geometry(cairo_t *cr, struct weston_view *view,
	      struct weston_coord *e, int n)
{
	struct geometry *g = view->geometry;
	double cx, cy;

	draw_box(cr, &g->surf, view);
	cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.4);
	cairo_fill(cr);
	weston_view_to_global_double(view, g->surf.x1 - 4, g->surf.y1 - 4, &cx, &cy);
	cairo_arc(cr, cx, cy, 1.5, 0.0, 2.0 * M_PI);
	if (view->transform.enabled == 0)
		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.8);
	cairo_fill(cr);

	draw_box(cr, &g->clip, NULL);
	cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.4);
	cairo_fill(cr);

	if (n) {
		draw_polygon_closed(cr, e, n);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
		cairo_stroke(cr);

		cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 0.5);
		draw_polygon_labels(cr, e, n);
	}
}

static void
redraw_handler(struct widget *widget, void *data)
{
	struct cliptest *cliptest = data;
	struct geometry *g = cliptest->view.geometry;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_surface_t *surface;
	struct weston_coord e[8];
	int n;

	n = calculate_edges(&cliptest->view, &g->clip, &g->surf, e);

	widget_get_allocation(cliptest->widget, &allocation);

	surface = window_get_surface(cliptest->window);
	cr = cairo_create(surface);
	widget_get_allocation(cliptest->widget, &allocation);
	cairo_rectangle(cr, allocation.x, allocation.y,
			allocation.width, allocation.height);
	cairo_clip(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_paint(cr);

	cairo_translate(cr, allocation.x, allocation.y);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, allocation.width / 2.0, 0.0);
	cairo_line_to(cr, allocation.width / 2.0, allocation.height);
	cairo_move_to(cr, 0.0, allocation.height / 2.0);
	cairo_line_to(cr, allocation.width, allocation.height / 2.0);
	cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
	cairo_stroke(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_push_group(cr);
		cairo_translate(cr, allocation.width / 2.0,
				allocation.height / 2.0);
		cairo_scale(cr, 4.0, 4.0);
		cairo_set_line_width(cr, 0.5);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
		cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL,
				       CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 5.0);
		draw_geometry(cr, &cliptest->view, e, n);
	cairo_pop_group_to_source(cr);
	cairo_paint(cr);

	cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	draw_coordinates(cr, 10.0, 10.0, e, n);

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}

static int
motion_handler(struct widget *widget, struct input *input,
	       uint32_t time, float x, float y, void *data)
{
	struct cliptest *cliptest = data;
	struct ui_state *ui = &cliptest->ui;
	struct geometry *ref = &ui->geometry;
	struct geometry *geom = &cliptest->geometry;
	float dx, dy;

	if (!ui->down)
		return CURSOR_LEFT_PTR;

	dx = (x - ui->down_pos[0]) * 0.25;
	dy = (y - ui->down_pos[1]) * 0.25;

	switch (ui->button) {
	case BTN_LEFT:
		geom->clip.x1 = ref->clip.x1 + dx;
		geom->clip.y1 = ref->clip.y1 + dy;
		/* fall through */
	case BTN_RIGHT:
		geom->clip.x2 = ref->clip.x2 + dx;
		geom->clip.y2 = ref->clip.y2 + dy;
		break;
	default:
		return CURSOR_LEFT_PTR;
	}

	widget_schedule_redraw(cliptest->widget);
	return CURSOR_BLANK;
}

static void
button_handler(struct widget *widget, struct input *input,
	       uint32_t time, uint32_t button,
	       enum wl_pointer_button_state state, void *data)
{
	struct cliptest *cliptest = data;
	struct ui_state *ui = &cliptest->ui;

	ui->button = button;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		ui->down = 1;
		input_get_position(input, &ui->down_pos[0], &ui->down_pos[1]);
	} else {
		ui->down = 0;
		ui->geometry = cliptest->geometry;
	}
}

static void
axis_handler(struct widget *widget, struct input *input, uint32_t time,
	     uint32_t axis, wl_fixed_t value, void *data)
{
	struct cliptest *cliptest = data;
	struct geometry *geom = &cliptest->geometry;

	if (axis != WL_POINTER_AXIS_VERTICAL_SCROLL)
		return;

	geometry_set_phi(geom, geom->phi +
				(M_PI / 12.0) * wl_fixed_to_double(value));
	cliptest->view.transform.enabled = 1;

	widget_schedule_redraw(cliptest->widget);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym,
	    enum wl_keyboard_key_state state, void *data)
{
	struct cliptest *cliptest = data;
	struct geometry *g = &cliptest->geometry;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_Escape:
		display_exit(cliptest->display);
		return;
	case XKB_KEY_w:
		g->clip.y1 -= 1;
		g->clip.y2 -= 1;
		break;
	case XKB_KEY_a:
		g->clip.x1 -= 1;
		g->clip.x2 -= 1;
		break;
	case XKB_KEY_s:
		g->clip.y1 += 1;
		g->clip.y2 += 1;
		break;
	case XKB_KEY_d:
		g->clip.x1 += 1;
		g->clip.x2 += 1;
		break;
	case XKB_KEY_i:
		g->clip.y2 -= 1;
		break;
	case XKB_KEY_j:
		g->clip.x2 -= 1;
		break;
	case XKB_KEY_k:
		g->clip.y2 += 1;
		break;
	case XKB_KEY_l:
		g->clip.x2 += 1;
		break;
	case XKB_KEY_n:
		geometry_set_phi(g, g->phi + (M_PI / 24.0));
		cliptest->view.transform.enabled = 1;
		break;
	case XKB_KEY_m:
		geometry_set_phi(g, g->phi - (M_PI / 24.0));
		cliptest->view.transform.enabled = 1;
		break;
	case XKB_KEY_r:
		geometry_set_phi(g, 0.0);
		cliptest->view.transform.enabled = 0;
		break;
	default:
		return;
	}

	widget_schedule_redraw(cliptest->widget);
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct cliptest *cliptest = data;

	window_schedule_redraw(cliptest->window);
}

static void
fullscreen_handler(struct window *window, void *data)
{
	struct cliptest *cliptest = data;

	cliptest->fullscreen ^= 1;
	window_set_fullscreen(window, cliptest->fullscreen);
}

static struct cliptest *
cliptest_create(struct display *display)
{
	struct cliptest *cliptest;

	cliptest = xzalloc(sizeof *cliptest);
	cliptest->view.surface = &cliptest->surface;
	cliptest->view.geometry = &cliptest->geometry;
	cliptest->view.transform.enabled = 0;
	geometry_init(&cliptest->geometry);
	geometry_init(&cliptest->ui.geometry);

	cliptest->window = window_create(display);
	cliptest->widget = window_frame_create(cliptest->window, cliptest);
	window_set_title(cliptest->window, "cliptest");
	window_set_appid(cliptest->window, "org.freedesktop.weston.cliptest");
	cliptest->display = display;

	window_set_user_data(cliptest->window, cliptest);
	widget_set_redraw_handler(cliptest->widget, redraw_handler);
	widget_set_button_handler(cliptest->widget, button_handler);
	widget_set_motion_handler(cliptest->widget, motion_handler);
	widget_set_axis_handler(cliptest->widget, axis_handler);

	window_set_keyboard_focus_handler(cliptest->window,
					  keyboard_focus_handler);
	window_set_key_handler(cliptest->window, key_handler);
	window_set_fullscreen_handler(cliptest->window, fullscreen_handler);

	/* set minimum size */
	widget_schedule_resize(cliptest->widget, 200, 100);

	/* set current size */
	widget_schedule_resize(cliptest->widget, 500, 400);

	return cliptest;
}

static struct timespec begin_time;

static void
reset_timer(void)
{
	clock_gettime(CLOCK_MONOTONIC, &begin_time);
}

static double
read_timer(void)
{
	struct timespec t;

	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)(t.tv_sec - begin_time.tv_sec) +
	       1e-9 * (t.tv_nsec - begin_time.tv_nsec);
}

static int
benchmark(void)
{
	struct weston_surface surface;
	struct weston_view view;
	struct geometry geom;
	struct weston_coord e[8];
	int i;
	double t;
	const int N = 1000000;

	geom.clip.x1 = -19;
	geom.clip.y1 = -19;
	geom.clip.x2 = 19;
	geom.clip.y2 = 19;

	geom.surf.x1 = -20;
	geom.surf.y1 = -20;
	geom.surf.x2 = 20;
	geom.surf.y2 = 20;

	geometry_set_phi(&geom, 0.0);

	view.surface = &surface;
	view.transform.enabled = 1;
	view.geometry = &geom;

	reset_timer();
	for (i = 0; i < N; i++) {
		geometry_set_phi(&geom, (float)i / 360.0f);
		calculate_edges(&view, &geom.clip, &geom.surf, e);
	}
	t = read_timer();

	printf("%d calls took %g s, average %g us/call\n", N, t, t / N * 1e6);

	return 0;
}

static void
cliptest_destroy(struct cliptest *cliptest)
{
	widget_destroy(cliptest->widget);
	window_destroy(cliptest->window);
	free(cliptest);
}

int
main(int argc, char *argv[])
{
	struct display *d;
	struct cliptest *cliptest;

	if (argc > 1) {
		if (argc == 2 && !strcmp(argv[1], "-b"))
			return benchmark();
		printf("Usage: %s [OPTIONS]\n  -b  run benchmark\n", argv[0]);
		return 1;
	}

	d = display_create(&argc, argv);
	if (d == NULL) {
		fprintf(stderr, "failed to create display: %s\n",
			strerror(errno));
		return -1;
	}

	cliptest = cliptest_create(d);
	display_run(d);

	cliptest_destroy(cliptest);
	display_destroy(d);

	return 0;
}
