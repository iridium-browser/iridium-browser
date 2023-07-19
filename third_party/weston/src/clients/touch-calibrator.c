/*
 * Copyright 2012 Intel Corporation
 * Copyright 2017-2018 Collabora, Ltd.
 * Copyright 2017-2018 General Electric Company
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>

#include <wayland-client.h>

#include "clients/window.h"
#include "shared/helpers.h"
#include <libweston/matrix.h>

#include "weston-touch-calibration-client-protocol.h"

enum exit_code {
	CAL_EXIT_SUCCESS = 0,
	CAL_EXIT_ERROR = 1,
	CAL_EXIT_CANCELLED = 2,
};

static int debug_;
static int verbose_;

#define pr_ver(...) do { \
	if (verbose_) \
		printf(__VA_ARGS__); \
} while (0)

#define pr_dbg(...) do { \
	if (debug_) \
		fprintf(stderr, __VA_ARGS__); \
} while (0)

static void
pr_err(const char *fmt, ...) WL_PRINTF(1, 2);

/* Our points for the calibration must be not be on a line */
static const struct {
	float x_ratio, y_ratio;
} test_ratios[] =  {
	{ 0.15, 0.10 }, /* three points for calibration */
	{ 0.85, 0.13 },
	{ 0.20, 0.80 },
	{ 0.70, 0.75 }  /* and one for verification */
};

#define NR_SAMPLES ((int)ARRAY_LENGTH(test_ratios))

struct point {
	double x;
	double y;
};

struct sample {
	int ind;
	struct point drawn;	/**< drawn point, pixels */
	struct weston_touch_coordinate *pending;
	struct point drawn_cal;	/**< drawn point, converted */
	bool conv_done;
	struct point touched;	/**< touch point, normalized */
	bool touch_done;
};

struct poly {
	struct color {
		double r, g, b, a;
	} color;
	int n_verts;
	const struct point *verts;
};

/** Touch event handling state machine
 *
 * Only a complete down->up->frame sequence should be accepted with user
 * feedback "right", and anything that deviates from that (invalid_touch,
 * cancel, multiple touch-downs) needs to undo the current sample and
 * possibly show user feedback "wrong".
 *
 * \<STATE\>
 * - \<triggers\>: \<actions\>
 *
 * IDLE
 * - touch down: sample, -> DOWN
 * - touch up: no-op
 * - frame: no-op
 * - invalid_touch: (undo), wrong, -> WAIT
 * - cancel: no-op
 * DOWN (first touch down)
 * - touch down: undo, wrong, -> WAIT
 * - touch up: -> UP
 * - frame: no-op
 * - invalid_touch: undo, wrong, -> WAIT
 * - cancel: undo, -> IDLE
 * UP (first touch was down and up)
 * - touch down: undo, wrong, -> WAIT
 * - touch up: no-op
 * - frame: right, touch finish, -> WAIT
 * - invalid_touch: undo, wrong, -> WAIT
 * - cancel: undo, -> IDLE
 * WAIT (show user feedback)
 * - touch down: no-op
 * - touch up: no-op
 * - frame, cancel, timer: if num_tp == 0 && timer_done -> IDLE
 * - invalid_touch: no-op
 */
enum touch_state {
	STATE_IDLE,
	STATE_DOWN,
	STATE_UP,
	STATE_WAIT
};

struct calibrator {
	struct sample samples[NR_SAMPLES];
	int current_sample;

	struct display *display;
	struct weston_touch_calibration *calibration;
	struct weston_touch_calibrator *calibrator;
	struct window *window;
	struct widget *widget;

	int n_devices_listed;
	char *match_name;
	char *device_name;

	int width;
	int height;

	bool cancelled;

	const struct poly *current_poly;
	bool exiting;

	struct toytimer wait_timer;
	bool timer_pending;
	enum touch_state state;

	int num_tp;		/* touch points down count */
};

static struct sample *
current_sample(struct calibrator *cal)
{
	return &cal->samples[cal->current_sample];
}

static void
sample_start(struct calibrator *cal, int i)
{
	struct sample *s = &cal->samples[i];

	assert(i >= 0 && i < NR_SAMPLES);

	s->ind = i;
	s->drawn.x = round(test_ratios[i].x_ratio * cal->width);
	s->drawn.y = round(test_ratios[i].y_ratio * cal->height);
	s->pending = NULL;
	s->conv_done = false;
	s->touch_done = false;

	cal->current_sample = i;
}

static struct point
wire_to_point(uint32_t xu, uint32_t yu)
{
	struct point p = {
		.x = (double)xu / 0xffffffff,
		.y = (double)yu / 0xffffffff
	};

	return p;
}

static void
sample_touch_down(struct calibrator *cal, uint32_t xu, uint32_t yu)
{
	struct sample *s = current_sample(cal);

	s->touched = wire_to_point(xu, yu);
	s->touch_done = true;

	pr_dbg("Down[%d] (%f, %f)\n", s->ind, s->touched.x, s->touched.y);
}

static void
coordinate_result_handler(void *data, struct weston_touch_coordinate *interface,
			  uint32_t xu, uint32_t yu)
{
	struct sample *s = data;

	weston_touch_coordinate_destroy(s->pending);
	s->pending = NULL;

	s->drawn_cal = wire_to_point(xu, yu);
	s->conv_done = true;

	pr_dbg("Conv[%d] (%f, %f)\n", s->ind, s->drawn_cal.x, s->drawn_cal.y);
}

struct weston_touch_coordinate_listener coordinate_listener = {
	coordinate_result_handler
};

static void
sample_undo(struct calibrator *cal)
{
	struct sample *s = current_sample(cal);

	pr_dbg("Undo[%d]\n", s->ind);

	s->touch_done = false;
	s->conv_done = false;
	if (s->pending) {
		weston_touch_coordinate_destroy(s->pending);
		s->pending = NULL;
	}
}

static void
sample_finish(struct calibrator *cal)
{
	struct sample *s = current_sample(cal);

	pr_dbg("Finish[%d]\n", s->ind);

	assert(!s->pending && !s->conv_done);

	s->pending = weston_touch_calibrator_convert(cal->calibrator,
						     (int32_t)s->drawn.x,
						     (int32_t)s->drawn.y);
	weston_touch_coordinate_add_listener(s->pending,
					     &coordinate_listener, s);

	if (cal->current_sample + 1 < NR_SAMPLES) {
		sample_start(cal, cal->current_sample + 1);
	} else {
		pr_dbg("got all touches\n");
		cal->exiting = true;
	}
}

/*
 * Calibration algorithm:
 *
 * The equation we want to apply at event time where x' and y' are the
 * calibrated co-ordinates.
 *
 * x' = Ax + By + C
 * y' = Dx + Ey + F
 *
 * For example "zero calibration" would be A=1.0 B=0.0 C=0.0, D=0.0, E=1.0,
 * and F=0.0.
 *
 * With 6 unknowns we need 6 equations to find the constants:
 *
 * x1' = Ax1 + By1 + C
 * y1' = Dx1 + Ey1 + F
 * ...
 * x3' = Ax3 + By3 + C
 * y3' = Dx3 + Ey3 + F
 *
 * In matrix form:
 *
 * x1'   x1 y1 1      A
 * x2' = x2 y2 1  x   B
 * x3'   x3 y3 1      C
 *
 * So making the matrix M we can find the constants with:
 *
 * A            x1'
 * B = M^-1  x  x2'
 * C            x3'
 *
 * (and similarly for D, E and F)
 *
 * For the calibration the desired values x, y are the same values at which
 * we've drawn at.
 *
 */
static int
compute_calibration(struct calibrator *cal, float *result)
{
	struct weston_matrix m;
	struct weston_matrix inverse;
	struct weston_vector x_calib;
	struct weston_vector y_calib;
	int i;

	assert(NR_SAMPLES >= 3);

	/*
	 * x1 y1  1  0
	 * x2 y2  1  0
	 * x3 y3  1  0
	 *  0  0  0  1
	 */
	weston_matrix_init(&m);
	for (i = 0; i < 3; i++) {
		m.d[i + 0] = cal->samples[i].touched.x;
		m.d[i + 4] = cal->samples[i].touched.y;
		m.d[i + 8] = 1.0f;
	}
	m.type = WESTON_MATRIX_TRANSFORM_OTHER;

	if (weston_matrix_invert(&inverse, &m) < 0) {
		pr_err("non-invertible matrix during computation\n");
		return -1;
	}

	for (i = 0; i < 3; i++) {
		x_calib.f[i] = cal->samples[i].drawn_cal.x;
		y_calib.f[i] = cal->samples[i].drawn_cal.y;
	}
	x_calib.f[3] = 0.0f;
	y_calib.f[3] = 0.0f;

	/* Multiples into the vector */
	weston_matrix_transform(&inverse, &x_calib);
	weston_matrix_transform(&inverse, &y_calib);

	for (i = 0; i < 3; i++)
		result[i] = x_calib.f[i];
	for (i = 0; i < 3; i++)
		result[i + 3] = y_calib.f[i];

	return 0;
}

static int
verify_calibration(struct calibrator *cal, const float *r)
{
	double thr = 0.1; /* accepted error radius */
	struct point e; /* expected value; error */
	const struct sample *s = &cal->samples[3];

	/* transform raw touches through the matrix */
	e.x = r[0] * s->touched.x + r[1] * s->touched.y + r[2];
	e.y = r[3] * s->touched.x + r[4] * s->touched.y + r[5];

	/* compute error */
	e.x -= s->drawn_cal.x;
	e.y -= s->drawn_cal.y;

	pr_dbg("calibration test error: %f, %f\n", e.x, e.y);

	if (e.x * e.x + e.y * e.y < thr * thr)
		return 0;

	pr_err("Calibration verification failed, too large error.\n");
	return -1;
}

static void
send_calibration(struct calibrator *cal, float *values)
{
	struct wl_array matrix;
	float *f;
	int i;

	wl_array_init(&matrix);
	for (i = 0; i < 6; i++) {
		f = wl_array_add(&matrix, sizeof *f);
		*f = values[i];
	}
	weston_touch_calibration_save(cal->calibration,
				      cal->device_name, &matrix);
	wl_array_release(&matrix);
}

static const struct point cross_verts[] = {
	{ 0.1, 0.2 },
	{ 0.2, 0.1 },
	{ 0.5, 0.4 },
	{ 0.8, 0.1 },
	{ 0.9, 0.2 },
	{ 0.6, 0.5 },
	{ 0.9, 0.8 },
	{ 0.8, 0.9 },
	{ 0.5, 0.6 },
	{ 0.2, 0.9 },
	{ 0.1, 0.8 },
	{ 0.4, 0.5 },
};

/* a red cross, for "wrong" */
static const struct poly cross = {
	.color = { 0.7, 0.0, 0.0, 1.0 },
	.n_verts = ARRAY_LENGTH(cross_verts),
	.verts = cross_verts
};

static const struct point check_verts[] = {
	{ 0.5, 0.7 },
	{ 0.8, 0.1 },
	{ 0.9, 0.1 },
	{ 0.55, 0.8 },
	{ 0.45, 0.8 },
	{ 0.3, 0.5 },
	{ 0.4, 0.5 }
};

/* a green check mark, for "right" */
static const struct poly check = {
	.color = { 0.0, 0.7, 0.0, 1.0 },
	.n_verts = ARRAY_LENGTH(check_verts),
	.verts = check_verts
};

static void
draw_poly(cairo_t *cr, const struct poly *poly)
{
	int i;

	cairo_set_source_rgba(cr, poly->color.r, poly->color.g,
			      poly->color.b, poly->color.a);
	cairo_move_to(cr, poly->verts[0].x, poly->verts[0].y);
	for (i = 1; i < poly->n_verts; i++)
		cairo_line_to(cr, poly->verts[i].x, poly->verts[i].y);
	cairo_close_path(cr);
	cairo_fill(cr);
}

static void
feedback_show(struct calibrator *cal, const struct poly *what)
{
	cal->current_poly = what;
	widget_schedule_redraw(cal->widget);

	toytimer_arm_once_usec(&cal->wait_timer, 1000 * 1000);
	cal->timer_pending = true;
}

static void
feedback_hide(struct calibrator *cal)
{
	cal->current_poly = NULL;
	widget_schedule_redraw(cal->widget);
}

static void
try_enter_state_idle(struct calibrator *cal)
{
	if (cal->num_tp != 0)
		return;

	if (cal->timer_pending)
		return;

	cal->state = STATE_IDLE;

	feedback_hide(cal);

	if (cal->exiting)
		display_exit(cal->display);
}

static void
enter_state_wait(struct calibrator *cal)
{
	assert(cal->timer_pending);
	cal->state = STATE_WAIT;
}

static void
wait_timer_done(struct toytimer *tt)
{
	struct calibrator *cal = container_of(tt, struct calibrator, wait_timer);

	assert(cal->state == STATE_WAIT);
	cal->timer_pending = false;
	try_enter_state_idle(cal);
}

static void
redraw_handler(struct widget *widget, void *data)
{
	struct calibrator *cal = data;
	struct sample *s = current_sample(cal);
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;

	widget_get_allocation(cal->widget, &allocation);
	assert(allocation.width == cal->width);
	assert(allocation.height == cal->height);

	surface = window_get_surface(cal->window);
	cr = cairo_create(surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	if (!cal->current_poly) {
		cairo_translate(cr, s->drawn.x, s->drawn.y);
		cairo_set_line_width(cr, 2.0);
		cairo_set_source_rgb(cr, 0.7, 0.0, 0.0);
		cairo_move_to(cr, 0, -10.0);
		cairo_line_to(cr, 0, 10.0);
		cairo_stroke(cr);
		cairo_move_to(cr, -10.0, 0);
		cairo_line_to(cr, 10.0, 0.0);
		cairo_stroke(cr);
	} else {
		cairo_scale(cr, allocation.width, allocation.height);
		draw_poly(cr, cal->current_poly);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

static struct calibrator *
calibrator_create(struct display *display, const char *match_name)
{
	struct calibrator *cal;

	cal = zalloc(sizeof *cal);
	if (!cal)
		abort();

	cal->match_name = match_name ? strdup(match_name) : NULL;
	cal->window = window_create_custom(display);
	cal->widget = window_add_widget(cal->window, cal);
	window_inhibit_redraw(cal->window);
	window_set_title(cal->window, "Touchscreen calibrator");
	cal->display = display;

	widget_set_redraw_handler(cal->widget, redraw_handler);

	toytimer_init(&cal->wait_timer, CLOCK_MONOTONIC,
		      display, wait_timer_done);

	cal->state = STATE_IDLE;
	cal->num_tp = 0;

	return cal;
}

static void
configure_handler(void *data, struct weston_touch_calibrator *interface,
		  int32_t width, int32_t height)
{
	struct calibrator *cal = data;

	pr_dbg("Configure calibrator window to size %ix%i\n", width, height);
	cal->width = width;
	cal->height = height;
	window_schedule_resize(cal->window, width, height);
	window_uninhibit_redraw(cal->window);

	sample_start(cal, 0);
	widget_schedule_redraw(cal->widget);
}

static void
cancel_calibration_handler(void *data, struct weston_touch_calibrator *interface)
{
	struct calibrator *cal = data;

	pr_dbg("calibration cancelled by the display server, quitting.\n");
	cal->cancelled = true;
	display_exit(cal->display);
}

static void
invalid_touch_handler(void *data, struct weston_touch_calibrator *interface)
{
	struct calibrator *cal = data;

	pr_dbg("invalid touch\n");

	switch (cal->state) {
	case STATE_IDLE:
	case STATE_DOWN:
	case STATE_UP:
		sample_undo(cal);
		feedback_show(cal, &cross);
		enter_state_wait(cal);
		break;
	case STATE_WAIT:
		/* no-op */
		break;
	}
}

static void
down_handler(void *data, struct weston_touch_calibrator *interface,
	     uint32_t time, int32_t id, uint32_t xu, uint32_t yu)
{
	struct calibrator *cal = data;

	cal->num_tp++;

	switch (cal->state) {
	case STATE_IDLE:
		sample_touch_down(cal, xu, yu);
		cal->state = STATE_DOWN;
		break;
	case STATE_DOWN:
	case STATE_UP:
		sample_undo(cal);
		feedback_show(cal, &cross);
		enter_state_wait(cal);
		break;
	case STATE_WAIT:
		/* no-op */
		break;
	}

	if (cal->current_poly)
		return;
}

static void
up_handler(void *data, struct weston_touch_calibrator *interface,
	   uint32_t time, int32_t id)
{
	struct calibrator *cal = data;

	cal->num_tp--;
	if (cal->num_tp < 0) {
		pr_dbg("Unmatched touch up.\n");
		cal->num_tp = 0;
	}

	switch (cal->state) {
	case STATE_DOWN:
		cal->state = STATE_UP;
		break;
	case STATE_IDLE:
	case STATE_UP:
	case STATE_WAIT:
		/* no-op */
		break;
	}
}

static void
motion_handler(void *data, struct weston_touch_calibrator *interface,
	       uint32_t time, int32_t id, uint32_t xu, uint32_t yu)
{
	/* motion is ignored */
}

static void
frame_handler(void *data, struct weston_touch_calibrator *interface)
{
	struct calibrator *cal = data;

	switch (cal->state) {
	case STATE_IDLE:
	case STATE_DOWN:
		/* no-op */
		break;
	case STATE_UP:
		feedback_show(cal, &check);
		sample_finish(cal);
		enter_state_wait(cal);
		break;
	case STATE_WAIT:
		try_enter_state_idle(cal);
		break;
	}
}

static void
cancel_handler(void *data, struct weston_touch_calibrator *interface)
{
	struct calibrator *cal = data;

	cal->num_tp = 0;

	switch (cal->state) {
	case STATE_IDLE:
		/* no-op */
		break;
	case STATE_DOWN:
	case STATE_UP:
		sample_undo(cal);
		try_enter_state_idle(cal);
		break;
	case STATE_WAIT:
		try_enter_state_idle(cal);
		break;
	}
}

struct weston_touch_calibrator_listener calibrator_listener = {
	configure_handler,
	cancel_calibration_handler,
	invalid_touch_handler,
	down_handler,
	up_handler,
	motion_handler,
	frame_handler,
	cancel_handler
};

static void
calibrator_show(struct calibrator *cal)
{
	struct wl_surface *surface = window_get_wl_surface(cal->window);

	cal->calibrator =
		weston_touch_calibration_create_calibrator(cal->calibration,
							   surface,
							   cal->device_name);
	weston_touch_calibrator_add_listener(cal->calibrator,
					     &calibrator_listener, cal);
}

static void
calibrator_destroy(struct calibrator *cal)
{
	toytimer_fini(&cal->wait_timer);
	if (cal->calibrator)
		weston_touch_calibrator_destroy(cal->calibrator);
	if (cal->calibration)
		weston_touch_calibration_destroy(cal->calibration);
	if (cal->widget)
		widget_destroy(cal->widget);
	if (cal->window)
		window_destroy(cal->window);
	free(cal->match_name);
	free(cal->device_name);
	free(cal);
}

static void
touch_device_handler(void *data, struct weston_touch_calibration *c,
		     const char *device, const char *head)
{
	struct calibrator *cal = data;

	cal->n_devices_listed++;

	if (!cal->match_name) {
		printf("device \"%s\" - head \"%s\"\n", device, head);
		return;
	}

	if (cal->device_name)
		return;

	if (strcmp(cal->match_name, device) == 0 ||
	    strcmp(cal->match_name, head) == 0)
		cal->device_name = strdup(device);
}

struct weston_touch_calibration_listener touch_calibration_listener = {
	touch_device_handler
};

static void
global_handler(struct display *display, uint32_t name,
	       const char *interface, uint32_t version, void *data)
{
	struct calibrator *cal = data;

	if (strcmp(interface, "weston_touch_calibration") == 0) {
		cal->calibration = display_bind(display, name,
						&weston_touch_calibration_interface, 1);
		weston_touch_calibration_add_listener(cal->calibration,
						      &touch_calibration_listener,
						      cal);
	}
}

static int
calibrator_run(struct calibrator *cal)
{
	struct wl_display *dpy;
	struct sample *s;
	bool wait;
	int i;
	int ret;
	float result[6];

	calibrator_show(cal);
	display_run(cal->display);

	if (cal->cancelled)
		return CAL_EXIT_CANCELLED;

	/* remove the window, no more input events */
	widget_destroy(cal->widget);
	cal->widget = NULL;
	window_destroy(cal->window);
	cal->window = NULL;

	/* wait for all conversions to return */
	dpy = display_get_display(cal->display);
	do {
		wait = false;

		for (i = 0; i < NR_SAMPLES; i++)
			if (cal->samples[i].pending)
				wait = true;

		if (wait) {
			ret = wl_display_roundtrip(dpy);
			if (ret < 0)
				return CAL_EXIT_ERROR;
		}
	} while (wait);

	for (i = 0; i < NR_SAMPLES; i++) {
		s = &cal->samples[i];
		if (!s->conv_done || !s->touch_done)
			return CAL_EXIT_ERROR;
	}

	if (compute_calibration(cal, result) < 0)
		return CAL_EXIT_ERROR;

	if (verify_calibration(cal, result) < 0)
		return CAL_EXIT_ERROR;

	pr_ver("Calibration values:");
	for (i = 0; i < 6; i++)
		pr_ver(" %f", result[i]);
	pr_ver("\n");

	send_calibration(cal, result);
	ret = wl_display_roundtrip(dpy);
	if (ret < 0)
		return CAL_EXIT_ERROR;

	return CAL_EXIT_SUCCESS;
}

static void
pr_err(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	fprintf(stderr, "%s error: ", program_invocation_short_name);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
}

static void
help(void)
{
	fprintf(stderr, "Compute a touchscreen calibration matrix for "
		"a Wayland compositor by\n"
		"having the user touch points on the screen.\n\n");
	fprintf(stderr, "Usage: %s [options...] name\n\n",
		program_invocation_short_name);
	fprintf(stderr,
		"Where 'name' can be a touch device sys path or a head name.\n"
		"If 'name' is not given, all devices available for "
		"calibration will be listed.\n"
		"If 'name' is given, it must be exactly as listed.\n"
		"Options:\n"
		"  --debug         Print messages to help debugging.\n"
		"  -h, --help      Display this help message\n"
		"  -v, --verbose   Print list header and calibration result.\n");
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct calibrator *cal;
	int c;
	char *match_name = NULL;
	int exit_code = CAL_EXIT_SUCCESS;
	static const struct option opts[] = {
		{ "help",    no_argument,       NULL,      'h' },
		{ "debug",   no_argument,       &debug_,   1 },
		{ "verbose", no_argument,       &verbose_, 1 },
		{ 0,         0,                 NULL,      0  }
	};

	while ((c = getopt_long(argc, argv, "hv", opts, NULL)) != -1) {
		switch (c) {
		case 'h':
			help();
			return CAL_EXIT_SUCCESS;
		case 'v':
			verbose_ = 1;
			break;
		case 0:
			break;
		default:
			return CAL_EXIT_ERROR;
		}
	}

	if (optind < argc)
		match_name = argv[optind++];

	if (optind < argc) {
		pr_err("extra arguments given.\n\n");
		help();
		return CAL_EXIT_ERROR;
	}

	display = display_create(&argc, argv);
	if (!display)
		return CAL_EXIT_ERROR;

	cal = calibrator_create(display, match_name);
	if (!cal)
		return CAL_EXIT_ERROR;

	display_set_user_data(display, cal);
	display_set_global_handler(display, global_handler);

	if (!match_name)
		pr_ver("Available touch devices:\n");

	/* Roundtrip to get list of available touch devices,
	 * first globals, then touch_device events */
	wl_display_roundtrip(display_get_display(display));
	wl_display_roundtrip(display_get_display(display));

	if (!cal->calibration) {
		exit_code = CAL_EXIT_ERROR;
		pr_err("the Wayland server does not expose the calibration interface.\n");
	} else if (cal->device_name) {
		exit_code = calibrator_run(cal);
	} else if (match_name) {
		exit_code = CAL_EXIT_ERROR;
		pr_err("\"%s\" was not found.\n", match_name);
	} else if (cal->n_devices_listed == 0) {
		fprintf(stderr, "No devices listed.\n");
	}

	calibrator_destroy(cal);
	display_destroy(display);

	return exit_code;
}
