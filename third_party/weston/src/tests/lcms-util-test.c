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

#include <math.h>
#include <lcms2.h>

#include "weston-test-client-helper.h"
#include "color_util.h"
#include "lcms_util.h"

static void
compare_pipeline_to_transfer_fn(cmsPipeline *pipeline, enum transfer_fn fn,
				struct scalar_stat *stat)
{
	const unsigned N = 100000;
	unsigned i;

	for (i = 0; i < N; i++) {
		float x = (double)i / N;
		float ref = apply_tone_curve(fn, x);
		float y;

		cmsPipelineEvalFloat(&x, &y, pipeline);
		scalar_stat_update(stat, y - ref, &(struct color_float){ .r = x });
	}
}

static const enum transfer_fn build_MPE_curves_test_set[] = {
	TRANSFER_FN_SRGB_EOTF,
	TRANSFER_FN_SRGB_EOTF_INVERSE,
	TRANSFER_FN_ADOBE_RGB_EOTF,
	TRANSFER_FN_ADOBE_RGB_EOTF_INVERSE,
	TRANSFER_FN_POWER2_4_EOTF,
	TRANSFER_FN_POWER2_4_EOTF_INVERSE,
};

TEST_P(build_MPE_curves, build_MPE_curves_test_set)
{
	const enum transfer_fn *fn = data;
	const cmsContext ctx = 0;
	cmsToneCurve *curve;
	cmsStage *stage;
	cmsPipeline *pipeline;
	struct scalar_stat stat = {};

	curve = build_MPE_curve(ctx, *fn);
	stage = cmsStageAllocToneCurves(ctx, 1, &curve);
	cmsFreeToneCurve(curve);

	pipeline = cmsPipelineAlloc(ctx, 1, 1);
	cmsPipelineInsertStage(pipeline, cmsAT_END, stage);

	compare_pipeline_to_transfer_fn(pipeline, *fn, &stat);
	testlog("Transfer function %s as a segmented curve element, error:\n",
		transfer_fn_name(*fn));
	scalar_stat_print_float(&stat);
	assert(fabs(stat.max) < 1e-7);
	assert(fabs(stat.min) < 1e-7);

	cmsPipelineFree(pipeline);
}
