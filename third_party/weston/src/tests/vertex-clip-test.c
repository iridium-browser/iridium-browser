/*
 * Copyright © 2013 Sam Spilsbury <smspillaz@gmail.com>
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

#include <assert.h>
#include <string.h>

#include "weston-test-runner.h"

#include "shared/helpers.h"
#include "vertex-clipping.h"

#define BOUNDING_BOX_TOP_Y 100.0f
#define BOUNDING_BOX_LEFT_X 50.0f
#define BOUNDING_BOX_RIGHT_X 100.0f
#define BOUNDING_BOX_BOTTOM_Y 50.0f

#define INSIDE_X1 (BOUNDING_BOX_LEFT_X + 1.0f)
#define INSIDE_X2 (BOUNDING_BOX_RIGHT_X - 1.0f)
#define INSIDE_Y1 (BOUNDING_BOX_BOTTOM_Y + 1.0f)
#define INSIDE_Y2 (BOUNDING_BOX_TOP_Y - 1.0f)

#define OUTSIDE_X1 (BOUNDING_BOX_LEFT_X - 1.0f)
#define OUTSIDE_X2 (BOUNDING_BOX_RIGHT_X + 1.0f)
#define OUTSIDE_Y1 (BOUNDING_BOX_BOTTOM_Y - 1.0f)
#define OUTSIDE_Y2 (BOUNDING_BOX_TOP_Y + 1.0f)

static void
populate_clip_context (struct clip_context *ctx)
{
	ctx->clip.x1 = BOUNDING_BOX_LEFT_X;
	ctx->clip.y1 = BOUNDING_BOX_BOTTOM_Y;
	ctx->clip.x2 = BOUNDING_BOX_RIGHT_X;
	ctx->clip.y2 = BOUNDING_BOX_TOP_Y;
}

static int
clip_polygon (struct clip_context *ctx,
	      struct polygon8 *polygon,
	      float *vertices_x,
	      float *vertices_y)
{
	populate_clip_context(ctx);
	return clip_transformed(ctx, polygon, vertices_x, vertices_y);
}

struct vertex_clip_test_data
{
	struct polygon8 surface;
	struct polygon8 expected;
};

const struct vertex_clip_test_data test_data[] =
{
	/* All inside */
	{
		{
			{ INSIDE_X1, INSIDE_X2, INSIDE_X2, INSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		},
		{
			{ INSIDE_X1, INSIDE_X2, INSIDE_X2, INSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		}
	},
	/* Top outside */
	{
		{
			{ INSIDE_X1, INSIDE_X2, INSIDE_X2, INSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, OUTSIDE_Y2, OUTSIDE_Y2 },
			4
		},
		{
			{ INSIDE_X1, INSIDE_X1, INSIDE_X2, INSIDE_X2 },
			{ BOUNDING_BOX_TOP_Y, INSIDE_Y1, INSIDE_Y1, BOUNDING_BOX_TOP_Y },
			4
		}
	},
	/* Bottom outside */
	{
		{
			{ INSIDE_X1, INSIDE_X2, INSIDE_X2, INSIDE_X1 },
			{ OUTSIDE_Y1, OUTSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		},
		{
			{ INSIDE_X1, INSIDE_X2, INSIDE_X2, INSIDE_X1 },
			{ BOUNDING_BOX_BOTTOM_Y, BOUNDING_BOX_BOTTOM_Y, INSIDE_Y2, INSIDE_Y2 },
			4
		}
	},
	/* Left outside */
	{
		{
			{ OUTSIDE_X1, INSIDE_X2, INSIDE_X2, OUTSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		},
		{
			{ BOUNDING_BOX_LEFT_X, INSIDE_X2, INSIDE_X2, BOUNDING_BOX_LEFT_X },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		}
	},
	/* Right outside */
	{
		{
			{ INSIDE_X1, OUTSIDE_X2, OUTSIDE_X2, INSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		},
		{
			{ INSIDE_X1, BOUNDING_BOX_RIGHT_X, BOUNDING_BOX_RIGHT_X, INSIDE_X1 },
			{ INSIDE_Y1, INSIDE_Y1, INSIDE_Y2, INSIDE_Y2 },
			4
		}
	},
	/* Diamond extending from bounding box edges, clip to bounding box */
	{
		{
			{ BOUNDING_BOX_LEFT_X - 25, BOUNDING_BOX_LEFT_X + 25, BOUNDING_BOX_RIGHT_X + 25, BOUNDING_BOX_RIGHT_X - 25 },
			{ BOUNDING_BOX_BOTTOM_Y + 25, BOUNDING_BOX_TOP_Y + 25, BOUNDING_BOX_TOP_Y - 25, BOUNDING_BOX_BOTTOM_Y - 25 },
			4
		},
		{
			{ BOUNDING_BOX_LEFT_X, BOUNDING_BOX_LEFT_X, BOUNDING_BOX_RIGHT_X, BOUNDING_BOX_RIGHT_X },
			{ BOUNDING_BOX_BOTTOM_Y, BOUNDING_BOX_TOP_Y, BOUNDING_BOX_TOP_Y, BOUNDING_BOX_BOTTOM_Y },
			4
		}
	},
	/* Diamond inside of bounding box edges, clip t bounding box, 8 resulting vertices */
	{
		{
			{ BOUNDING_BOX_LEFT_X - 12.5, BOUNDING_BOX_LEFT_X + 25, BOUNDING_BOX_RIGHT_X + 12.5, BOUNDING_BOX_RIGHT_X - 25 },
			{ BOUNDING_BOX_BOTTOM_Y + 25, BOUNDING_BOX_TOP_Y + 12.5, BOUNDING_BOX_TOP_Y - 25, BOUNDING_BOX_BOTTOM_Y - 12.5 },
			4
		},
		{
			{ BOUNDING_BOX_LEFT_X + 12.5, BOUNDING_BOX_LEFT_X, BOUNDING_BOX_LEFT_X, BOUNDING_BOX_LEFT_X + 12.5,
			  BOUNDING_BOX_RIGHT_X - 12.5, BOUNDING_BOX_RIGHT_X, BOUNDING_BOX_RIGHT_X, BOUNDING_BOX_RIGHT_X - 12.5 },
			{ BOUNDING_BOX_BOTTOM_Y, BOUNDING_BOX_BOTTOM_Y + 12.5, BOUNDING_BOX_TOP_Y - 12.5, BOUNDING_BOX_TOP_Y,
			  BOUNDING_BOX_TOP_Y, BOUNDING_BOX_TOP_Y - 12.5, BOUNDING_BOX_BOTTOM_Y + 12.5, BOUNDING_BOX_BOTTOM_Y },
			8
		}
	}
};

/* clip_polygon modifies the source operand and the test data must
 * be const, so we need to deep copy it */
static void
deep_copy_polygon8(const struct polygon8 *src, struct polygon8 *dst)
{
	dst->n = src->n;
	memcpy((void *) dst->x, src->x, sizeof (src->x));
	memcpy((void *) dst->y, src->y, sizeof (src->y));
}

TEST_P(clip_polygon_n_vertices_emitted, test_data)
{
	struct vertex_clip_test_data *tdata = data;
	struct clip_context ctx;
	struct polygon8 polygon;
	float vertices_x[8];
	float vertices_y[8];
	deep_copy_polygon8(&tdata->surface, &polygon);
	int emitted = clip_polygon(&ctx, &polygon, vertices_x, vertices_y);

	assert(emitted == tdata->expected.n);
}

TEST_P(clip_polygon_expected_vertices, test_data)
{
	struct vertex_clip_test_data *tdata = data;
	struct clip_context ctx;
	struct polygon8 polygon;
	float vertices_x[8];
	float vertices_y[8];
	deep_copy_polygon8(&tdata->surface, &polygon);
	int emitted = clip_polygon(&ctx, &polygon, vertices_x, vertices_y);
	int i = 0;

	for (; i < emitted; ++i)
	{
		assert(vertices_x[i] == tdata->expected.x[i]);
		assert(vertices_y[i] == tdata->expected.y[i]);
	}
}

TEST(float_difference_different)
{
	assert(float_difference(1.0f, 0.0f) == 1.0f);
}

TEST(float_difference_same)
{
	assert(float_difference(1.0f, 1.0f) == 0.0f);
}

