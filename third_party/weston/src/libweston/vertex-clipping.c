/*
 * Copyright © 2012 Intel Corporation
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
#include <assert.h>
#include <float.h>
#include <math.h>

#include "shared/helpers.h"
#include "vertex-clipping.h"

WESTON_EXPORT_FOR_TESTS float
float_difference(float a, float b)
{
	/* http://www.altdevblogaday.com/2012/02/22/comparing-floating-point-numbers-2012-edition/ */
	static const float max_diff = 4.0f * FLT_MIN;
	static const float max_rel_diff = 4.0e-5;
	float diff = a - b;
	float adiff = fabsf(diff);

	if (adiff <= max_diff)
		return 0.0f;

	a = fabsf(a);
	b = fabsf(b);
	if (adiff <= (a > b ? a : b) * max_rel_diff)
		return 0.0f;

	return diff;
}

/* A line segment (p1x, p1y)-(p2x, p2y) intersects the line x = x_arg.
 * Compute the y coordinate of the intersection.
 */
static float
clip_intersect_y(float p1x, float p1y, float p2x, float p2y,
		 float x_arg)
{
	float a;
	float diff = float_difference(p1x, p2x);

	/* Practically vertical line segment, yet the end points have already
	 * been determined to be on different sides of the line. Therefore
	 * the line segment is part of the line and intersects everywhere.
	 * Return the end point, so we use the whole line segment.
	 */
	if (diff == 0.0f)
		return p2y;

	a = (x_arg - p2x) / diff;
	return p2y + (p1y - p2y) * a;
}

/* A line segment (p1x, p1y)-(p2x, p2y) intersects the line y = y_arg.
 * Compute the x coordinate of the intersection.
 */
static float
clip_intersect_x(float p1x, float p1y, float p2x, float p2y,
		 float y_arg)
{
	float a;
	float diff = float_difference(p1y, p2y);

	/* Practically horizontal line segment, yet the end points have already
	 * been determined to be on different sides of the line. Therefore
	 * the line segment is part of the line and intersects everywhere.
	 * Return the end point, so we use the whole line segment.
	 */
	if (diff == 0.0f)
		return p2x;

	a = (y_arg - p2y) / diff;
	return p2x + (p1x - p2x) * a;
}

enum path_transition {
	PATH_TRANSITION_OUT_TO_OUT = 0,
	PATH_TRANSITION_OUT_TO_IN = 1,
	PATH_TRANSITION_IN_TO_OUT = 2,
	PATH_TRANSITION_IN_TO_IN = 3,
};

static void
clip_append_vertex(struct clip_context *ctx, float x, float y)
{
	*ctx->vertices = weston_coord(x, y);
	ctx->vertices++;
}

static enum path_transition
path_transition_left_edge(struct clip_context *ctx, float x, float y)
{
	return ((ctx->prev.x >= ctx->clip.x1) << 1) | (x >= ctx->clip.x1);
}

static enum path_transition
path_transition_right_edge(struct clip_context *ctx, float x, float y)
{
	return ((ctx->prev.x < ctx->clip.x2) << 1) | (x < ctx->clip.x2);
}

static enum path_transition
path_transition_top_edge(struct clip_context *ctx, float x, float y)
{
	return ((ctx->prev.y >= ctx->clip.y1) << 1) | (y >= ctx->clip.y1);
}

static enum path_transition
path_transition_bottom_edge(struct clip_context *ctx, float x, float y)
{
	return ((ctx->prev.y < ctx->clip.y2) << 1) | (y < ctx->clip.y2);
}

static void
clip_polygon_leftright(struct clip_context *ctx,
		       enum path_transition transition,
		       float x, float y, float clip_x)
{
	float yi;

	switch (transition) {
	case PATH_TRANSITION_IN_TO_IN:
		clip_append_vertex(ctx, x, y);
		break;
	case PATH_TRANSITION_IN_TO_OUT:
		yi = clip_intersect_y(ctx->prev.x, ctx->prev.y, x, y, clip_x);
		clip_append_vertex(ctx, clip_x, yi);
		break;
	case PATH_TRANSITION_OUT_TO_IN:
		yi = clip_intersect_y(ctx->prev.x, ctx->prev.y, x, y, clip_x);
		clip_append_vertex(ctx, clip_x, yi);
		clip_append_vertex(ctx, x, y);
		break;
	case PATH_TRANSITION_OUT_TO_OUT:
		/* nothing */
		break;
	default:
		assert(0 && "bad enum path_transition");
	}

	ctx->prev.x = x;
	ctx->prev.y = y;
}

static void
clip_polygon_topbottom(struct clip_context *ctx,
		       enum path_transition transition,
		       float x, float y, float clip_y)
{
	float xi;

	switch (transition) {
	case PATH_TRANSITION_IN_TO_IN:
		clip_append_vertex(ctx, x, y);
		break;
	case PATH_TRANSITION_IN_TO_OUT:
		xi = clip_intersect_x(ctx->prev.x, ctx->prev.y, x, y, clip_y);
		clip_append_vertex(ctx, xi, clip_y);
		break;
	case PATH_TRANSITION_OUT_TO_IN:
		xi = clip_intersect_x(ctx->prev.x, ctx->prev.y, x, y, clip_y);
		clip_append_vertex(ctx, xi, clip_y);
		clip_append_vertex(ctx, x, y);
		break;
	case PATH_TRANSITION_OUT_TO_OUT:
		/* nothing */
		break;
	default:
		assert(0 && "bad enum path_transition");
	}

	ctx->prev.x = x;
	ctx->prev.y = y;
}

static void
clip_context_prepare(struct clip_context *ctx, const struct polygon8 *src,
		     struct weston_coord *dst)
{
	ctx->prev.x = src->pos[src->n - 1].x;
	ctx->prev.y = src->pos[src->n - 1].y;
	ctx->vertices = dst;
}

static int
clip_polygon_left(struct clip_context *ctx, const struct polygon8 *src,
		  struct weston_coord *dst)
{
	enum path_transition trans;
	int i;

	if (src->n < 2)
		return 0;

	clip_context_prepare(ctx, src, dst);
	for (i = 0; i < src->n; i++) {
		trans = path_transition_left_edge(ctx, src->pos[i].x, src->pos[i].y);
		clip_polygon_leftright(ctx, trans, src->pos[i].x, src->pos[i].y,
				       ctx->clip.x1);
	}
	return ctx->vertices - dst;
}

static int
clip_polygon_right(struct clip_context *ctx, const struct polygon8 *src,
		   struct weston_coord *dst)
{
	enum path_transition trans;
	int i;

	if (src->n < 2)
		return 0;

	clip_context_prepare(ctx, src, dst);
	for (i = 0; i < src->n; i++) {
		trans = path_transition_right_edge(ctx, src->pos[i].x, src->pos[i].y);
		clip_polygon_leftright(ctx, trans, src->pos[i].x, src->pos[i].y,
				       ctx->clip.x2);
	}
	return ctx->vertices - dst;
}

static int
clip_polygon_top(struct clip_context *ctx, const struct polygon8 *src,
		 struct weston_coord *dst)
{
	enum path_transition trans;
	int i;

	if (src->n < 2)
		return 0;

	clip_context_prepare(ctx, src, dst);
	for (i = 0; i < src->n; i++) {
		trans = path_transition_top_edge(ctx, src->pos[i].x, src->pos[i].y);
		clip_polygon_topbottom(ctx, trans, src->pos[i].x, src->pos[i].y,
				       ctx->clip.y1);
	}
	return ctx->vertices - dst;
}

static int
clip_polygon_bottom(struct clip_context *ctx, const struct polygon8 *src,
		    struct weston_coord *dst)
{
	enum path_transition trans;
	int i;

	if (src->n < 2)
		return 0;

	clip_context_prepare(ctx, src, dst);
	for (i = 0; i < src->n; i++) {
		trans = path_transition_bottom_edge(ctx, src->pos[i].x, src->pos[i].y);
		clip_polygon_topbottom(ctx, trans, src->pos[i].x, src->pos[i].y,
				       ctx->clip.y2);
	}
	return ctx->vertices - dst;
}

WESTON_EXPORT_FOR_TESTS int
clip_simple(struct clip_context *ctx,
	    struct polygon8 *surf,
	    struct weston_coord *e)
{
	int i;
	for (i = 0; i < surf->n; i++) {
		e[i].x = CLIP(surf->pos[i].x, ctx->clip.x1, ctx->clip.x2);
		e[i].y = CLIP(surf->pos[i].y, ctx->clip.y1, ctx->clip.y2);
	}
	return surf->n;
}

WESTON_EXPORT_FOR_TESTS int
clip_transformed(struct clip_context *ctx,
		 struct polygon8 *surf,
		 struct weston_coord *e)
{
	struct polygon8 polygon;
	int i, n;

	polygon.n = clip_polygon_left(ctx, surf, polygon.pos);
	surf->n = clip_polygon_right(ctx, &polygon, surf->pos);
	polygon.n = clip_polygon_top(ctx, surf, polygon.pos);
	surf->n = clip_polygon_bottom(ctx, &polygon, surf->pos);

	/* Get rid of duplicate vertices */
	e[0] = surf->pos[0];
	n = 1;
	for (i = 1; i < surf->n; i++) {
		if (float_difference(e[n - 1].x, surf->pos[i].x) == 0.0f &&
		    float_difference(e[n - 1].y, surf->pos[i].y) == 0.0f)
			continue;
		e[n] = surf->pos[i];
		n++;
	}
	if (float_difference(e[n - 1].x, surf->pos[0].x) == 0.0f &&
	    float_difference(e[n - 1].y, surf->pos[0].y) == 0.0f)
		n--;

	return n;
}
