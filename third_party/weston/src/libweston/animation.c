/*
 * Copyright © 2011 Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#include <unistd.h>
#include <fcntl.h>

#include <libweston/libweston.h>
#include "libweston-internal.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

WL_EXPORT void
weston_spring_init(struct weston_spring *spring,
		   double k, double current, double target)
{
	spring->k = k;
	spring->friction = 400.0;
	spring->current = current;
	spring->previous = current;
	spring->target = target;
	spring->clip = WESTON_SPRING_OVERSHOOT;
	spring->min = 0.0;
	spring->max = 1.0;
}

WL_EXPORT void
weston_spring_update(struct weston_spring *spring, const struct timespec *time)
{
	double force, v, current, step;

	/* Limit the number of executions of the loop below by ensuring that
	 * the timestamp for last update of the spring is no more than 1s ago.
	 * This handles the case where time moves backwards or forwards in
	 * large jumps.
	 */
	if (timespec_sub_to_msec(time, &spring->timestamp) > 1000) {
		weston_log("unexpectedly large timestamp jump "
			   "(from %" PRId64 " to %" PRId64 ")\n",
			   timespec_to_msec(&spring->timestamp),
			   timespec_to_msec(time));
		timespec_add_msec(&spring->timestamp, time, -1000);
	}

	step = 0.01;
	while (4 < timespec_sub_to_msec(time, &spring->timestamp)) {
		current = spring->current;
		v = current - spring->previous;
		force = spring->k * (spring->target - current) / 10.0 +
			(spring->previous - current) - v * spring->friction;

		spring->current =
			current + (current - spring->previous) +
			force * step * step;
		spring->previous = current;

		switch (spring->clip) {
		case WESTON_SPRING_OVERSHOOT:
			break;

		case WESTON_SPRING_CLAMP:
			if (spring->current > spring->max) {
				spring->current = spring->max;
				spring->previous = spring->max;
			} else if (spring->current < 0.0) {
				spring->current = spring->min;
				spring->previous = spring->min;
			}
			break;

		case WESTON_SPRING_BOUNCE:
			if (spring->current > spring->max) {
				spring->current =
					2 * spring->max - spring->current;
				spring->previous =
					2 * spring->max - spring->previous;
			} else if (spring->current < spring->min) {
				spring->current =
					2 * spring->min - spring->current;
				spring->previous =
					2 * spring->min - spring->previous;
			}
			break;
		}

		timespec_add_msec(&spring->timestamp, &spring->timestamp, 4);
	}
}

WL_EXPORT int
weston_spring_done(struct weston_spring *spring)
{
	return fabs(spring->previous - spring->target) < 0.002 &&
		fabs(spring->current - spring->target) < 0.002;
}

typedef	void (*weston_view_animation_frame_func_t)(struct weston_view_animation *animation);

struct weston_view_animation {
	struct weston_view *view;
	struct weston_animation animation;
	struct weston_spring spring;
	struct weston_transform transform;
	struct wl_listener listener;
	float start, stop;
	weston_view_animation_frame_func_t frame;
	weston_view_animation_frame_func_t reset;
	weston_view_animation_done_func_t done;
	void *data;
	void *private;
};

WL_EXPORT void
weston_view_animation_destroy(struct weston_view_animation *animation)
{
	wl_list_remove(&animation->animation.link);
	wl_list_remove(&animation->listener.link);
	wl_list_remove(&animation->transform.link);
	if (animation->reset)
		animation->reset(animation);
	weston_view_geometry_dirty(animation->view);
	if (animation->done)
		animation->done(animation, animation->data);
	free(animation);
}

static void
handle_animation_view_destroy(struct wl_listener *listener, void *data)
{
	struct weston_view_animation *animation =
		container_of(listener,
			     struct weston_view_animation, listener);

	weston_view_animation_destroy(animation);
}

static void
weston_view_animation_frame(struct weston_animation *base,
			    struct weston_output *output,
			    const struct timespec *time)
{
	struct weston_view_animation *animation =
		container_of(base,
			     struct weston_view_animation, animation);
	struct weston_compositor *compositor =
		animation->view->surface->compositor;

	if (base->frame_counter <= 1)
		animation->spring.timestamp = *time;

	weston_spring_update(&animation->spring, time);

	if (weston_spring_done(&animation->spring)) {
		weston_view_schedule_repaint(animation->view);
		weston_view_animation_destroy(animation);
		return;
	}

	if (animation->frame)
		animation->frame(animation);

	weston_view_geometry_dirty(animation->view);
	weston_view_schedule_repaint(animation->view);

	/* The view's output_mask will be zero if its position is
	 * offscreen. Animations should always run but as they are also
	 * run off the repaint cycle, if there's nothing to repaint
	 * the animation stops running. Therefore if we catch this situation
	 * and schedule a repaint on all outputs it will be avoided.
	 */
	if (animation->view->output_mask == 0)
		weston_compositor_schedule_repaint(compositor);
}

static void
idle_animation_destroy(void *data)
{
	struct weston_view_animation *animation = data;

	weston_view_animation_destroy(animation);
}

static struct weston_view_animation *
weston_view_animation_create(struct weston_view *view,
			     float start, float stop,
			     weston_view_animation_frame_func_t frame,
			     weston_view_animation_frame_func_t reset,
			     weston_view_animation_done_func_t done,
			     void *data,
			     void *private)
{
	struct weston_view_animation *animation;
	struct weston_compositor *ec = view->surface->compositor;
	struct wl_event_loop *loop;

	animation = malloc(sizeof *animation);
	if (!animation)
		return NULL;

	animation->view = view;
	animation->frame = frame;
	animation->reset = reset;
	animation->done = done;
	animation->data = data;
	animation->start = start;
	animation->stop = stop;
	animation->private = private;

	weston_matrix_init(&animation->transform.matrix);
	wl_list_insert(&view->geometry.transformation_list,
		       &animation->transform.link);

	animation->animation.frame = weston_view_animation_frame;

	animation->listener.notify = handle_animation_view_destroy;
	wl_signal_add(&view->destroy_signal, &animation->listener);

	if (view->output) {
		wl_list_insert(&view->output->animation_list,
			       &animation->animation.link);
	} else {
		wl_list_init(&animation->animation.link);
		loop = wl_display_get_event_loop(ec->wl_display);
		wl_event_loop_add_idle(loop, idle_animation_destroy, animation);
	}

	return animation;
}

static void
weston_view_animation_run(struct weston_view_animation *animation)
{
	struct timespec zero_time = { 0 };

	animation->animation.frame_counter = 0;
	weston_view_animation_frame(&animation->animation, NULL, &zero_time);
}

static void
reset_alpha(struct weston_view_animation *animation)
{
	struct weston_view *view = animation->view;

	view->alpha = animation->stop;
}

static void
zoom_frame(struct weston_view_animation *animation)
{
	struct weston_view *es = animation->view;
	float scale;

	scale = animation->start +
		(animation->stop - animation->start) *
		animation->spring.current;
	weston_matrix_init(&animation->transform.matrix);
	weston_matrix_translate(&animation->transform.matrix,
				-0.5f * es->surface->width,
				-0.5f * es->surface->height, 0);
	weston_matrix_scale(&animation->transform.matrix, scale, scale, scale);
	weston_matrix_translate(&animation->transform.matrix,
				0.5f * es->surface->width,
				0.5f * es->surface->height, 0);

	es->alpha = animation->spring.current;
	if (es->alpha > 1.0)
		es->alpha = 1.0;
}

WL_EXPORT struct weston_view_animation *
weston_zoom_run(struct weston_view *view, float start, float stop,
		weston_view_animation_done_func_t done, void *data)
{
	struct weston_view_animation *zoom;

	zoom = weston_view_animation_create(view, start, stop,
					    zoom_frame, reset_alpha,
					    done, data, NULL);

	if (zoom == NULL)
		return NULL;

	weston_spring_init(&zoom->spring, 300.0, start, stop);
	zoom->spring.friction = 1400;
	zoom->spring.previous = start - (stop - start) * 0.03;

	weston_view_animation_run(zoom);

	return zoom;
}

static void
fade_frame(struct weston_view_animation *animation)
{
	if (animation->spring.current > 0.999)
		animation->view->alpha = 1;
	else if (animation->spring.current < 0.001 )
		animation->view->alpha = 0;
	else
		animation->view->alpha = animation->spring.current;
}

WL_EXPORT struct weston_view_animation *
weston_fade_run(struct weston_view *view,
		float start, float end, float k,
		weston_view_animation_done_func_t done, void *data)
{
	struct weston_view_animation *fade;

	fade = weston_view_animation_create(view, start, end,
					    fade_frame, reset_alpha,
					    done, data, NULL);

	if (fade == NULL)
		return NULL;

	weston_spring_init(&fade->spring, 1000.0, start, end);
	fade->spring.friction = 4000;
	fade->spring.previous = start - (end - start) * 0.1;

	view->alpha = start;

	weston_view_animation_run(fade);

	return fade;
}

WL_EXPORT void
weston_fade_update(struct weston_view_animation *fade, float target)
{
	fade->spring.target = target;
	fade->stop = target;
}

static void
stable_fade_frame(struct weston_view_animation *animation)
{
	struct weston_view *back_view;

	if (animation->spring.current > 0.999)
		animation->view->alpha = 1;
	else if (animation->spring.current < 0.001 )
		animation->view->alpha = 0;
	else
		animation->view->alpha = animation->spring.current;

	back_view = (struct weston_view *) animation->private;
	back_view->alpha =
		(animation->spring.target - animation->view->alpha) /
		(1.0 - animation->view->alpha);
	weston_view_geometry_dirty(back_view);
}

WL_EXPORT struct weston_view_animation *
weston_stable_fade_run(struct weston_view *front_view, float start,
		struct weston_view *back_view, float end,
		weston_view_animation_done_func_t done, void *data)
{
	struct weston_view_animation *fade;

	fade = weston_view_animation_create(front_view, 0, 0,
					    stable_fade_frame, NULL,
					    done, data, back_view);

	if (fade == NULL)
		return NULL;

	weston_spring_init(&fade->spring, 400, start, end);
	fade->spring.friction = 1150;

	front_view->alpha = start;
	back_view->alpha = end;

	weston_view_animation_run(fade);

	return fade;
}

static void
slide_frame(struct weston_view_animation *animation)
{
	float scale;

	scale = animation->start +
		(animation->stop - animation->start) *
		animation->spring.current;
	weston_matrix_init(&animation->transform.matrix);
	weston_matrix_translate(&animation->transform.matrix, 0, scale, 0);
}

WL_EXPORT struct weston_view_animation *
weston_slide_run(struct weston_view *view, float start, float stop,
		 weston_view_animation_done_func_t done, void *data)
{
	struct weston_view_animation *animation;

	animation = weston_view_animation_create(view, start, stop,
					      slide_frame, NULL, done,
					      data, NULL);
	if (!animation)
		return NULL;

	weston_spring_init(&animation->spring, 400.0, 0.0, 1.0);
	animation->spring.friction = 600;
	animation->spring.clip = WESTON_SPRING_BOUNCE;

	weston_view_animation_run(animation);

	return animation;
}

struct weston_move_animation {
	int dx;
	int dy;
	bool reverse;
	bool scale;
	weston_view_animation_done_func_t done;
};

static void
move_frame(struct weston_view_animation *animation)
{
	struct weston_move_animation *move = animation->private;
	float scale;
	float progress = animation->spring.current;

	if (move->reverse)
		progress = 1.0 - progress;

	scale = animation->start +
		(animation->stop - animation->start) *
		progress;
	weston_matrix_init(&animation->transform.matrix);
	if (move->scale)
		weston_matrix_scale(&animation->transform.matrix, scale, scale,
				    1.0f);
	weston_matrix_translate(&animation->transform.matrix,
	                        move->dx * progress, move->dy * progress,
	                        0);
}

static void
move_done(struct weston_view_animation *animation, void *data)
{
	struct weston_move_animation *move = animation->private;

	if (move->done)
		move->done(animation, data);

	free(move);
}

static struct weston_view_animation *
weston_move_scale_run_internal(struct weston_view *view, int dx, int dy,
			       float start, float end, bool reverse, bool scale,
			       weston_view_animation_done_func_t done,
			       void *data)
{
	struct weston_move_animation *move;
	struct weston_view_animation *animation;

	move = malloc(sizeof(*move));
	if (!move)
		return NULL;
	move->dx = dx;
	move->dy = dy;
	move->reverse = reverse;
	move->scale = scale;
	move->done = done;

	animation = weston_view_animation_create(view, start, end, move_frame,
						 NULL, move_done, data, move);

	if (animation == NULL){
		free(move);
		return NULL;
	}

	weston_spring_init(&animation->spring, 400.0, 0.0, 1.0);
	animation->spring.friction = 1150;

	weston_view_animation_run(animation);

	return animation;
}

WL_EXPORT struct weston_view_animation *
weston_move_scale_run(struct weston_view *view, int dx, int dy,
		      float start, float end, bool reverse,
		      weston_view_animation_done_func_t done, void *data)
{
	return weston_move_scale_run_internal(view, dx, dy, start, end, reverse,
					      true, done, data);
}

WL_EXPORT struct weston_view_animation *
weston_move_run(struct weston_view *view, int dx, int dy,
		float start, float end, bool reverse,
		weston_view_animation_done_func_t done, void *data)
{
	return weston_move_scale_run_internal(view, dx, dy, start, end, reverse,
					      false, done, data);
}
