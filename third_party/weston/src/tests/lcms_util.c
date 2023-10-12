/*
 * Copyright 2022 Collabora, Ltd.
 * Copyright (c) 1998-2022 Marti Maria Saguer
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
#include <assert.h>

#include "shared/helpers.h"
#include "color_util.h"
#include "lcms_util.h"

/*
 * MPE tone curves can only use LittleCMS parametric curve types 6-8 and not
 * inverses.
 * type 6: Y = (aX + b)^g + c;      params [g, a, b, c]
 * type 7: Y = a log(bX^g + c) + d; params [g, a, b, c, d]
 * type 8: Y = a b^(cX + d) + e;    params [a, b, c, d, e]
 * Additionally, type 0 is sampled segment.
 *
 * cmsCurveSegment.x1 is the breakpoint stored in ICC files, except for the
 * last segment. First segment always begins at -Inf, and last segment always
 * ends at Inf.
 */

static cmsToneCurve *
build_MPE_curve_sRGB(cmsContext ctx)
{
	cmsCurveSegment segments[] = {
		{
			/* Constant zero segment */
			.x0 = -HUGE_VAL,
			.x1 = 0.0,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 0.0 },
		},
		{
			/* Linear segment y = x / 12.92 */
			.x0 = 0.0,
			.x1 = 0.04045,
			.Type = 0,
			.nGridPoints = 2,
			.SampledPoints = (float[]){ 0.0, 0.04045 / 12.92 },
		},
		{
			/* Power segment y = ((x + 0.055) / 1.055)^2.4
			 * which is translated to
			 * y = (1/1.055 * x + 0.055 / 1.055)^2.4 + 0.0
			 */
			.x0 = 0.04045,
			.x1 = 1.0,
			.Type = 6,
			.Params = { 2.4, 1.0 / 1.055, 0.055 / 1.055, 0.0 },
		},
		{
			/* Constant one segment */
			.x0 = 1.0,
			.x1 = HUGE_VAL,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 1.0 },
		}
	};

	return cmsBuildSegmentedToneCurve(ctx, ARRAY_LENGTH(segments), segments);
}

static cmsToneCurve *
build_MPE_curve_sRGB_inv(cmsContext ctx)
{
	cmsCurveSegment segments[] = {
		{
			/* Constant zero segment */
			.x0 = -HUGE_VAL,
			.x1 = 0.0,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 0.0 },
		},
		{
			/* Linear segment y = x * 12.92 */
			.x0 = 0.0,
			.x1 = 0.04045 / 12.92,
			.Type = 0,
			.nGridPoints = 2,
			.SampledPoints = (float[]){ 0.0, 0.04045 },
		},
		{
			/* Power segment y = 1.055 * x^(1/2.4) - 0.055
			 * which is translated to
			 * y = (1.055^2.4 * x + 0.0)^(1/2.4) - 0.055
			 */
			.x0 = 0.04045 / 12.92,
			.x1 = 1.0,
			.Type = 6,
			.Params = { 1.0 / 2.4, pow(1.055, 2.4), 0.0, -0.055 },
		},
		{
			/* Constant one segment */
			.x0 = 1.0,
			.x1 = HUGE_VAL,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 1.0 },
		}
	};

	return cmsBuildSegmentedToneCurve(ctx, ARRAY_LENGTH(segments), segments);
}

static cmsToneCurve *
build_MPE_curve_power(cmsContext ctx, double exponent)
{
	cmsCurveSegment segments[] = {
		{
			/* Constant zero segment */
			.x0 = -HUGE_VAL,
			.x1 = 0.0,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 0.0 },
		},
		{
			/* Power segment y = x^exponent
			 * which is translated to
			 * y = (1.0 * x + 0.0)^exponent + 0.0
			 */
			.x0 = 0.0,
			.x1 = 1.0,
			.Type = 6,
			.Params = { exponent, 1.0, 0.0, 0.0 },
		},
		{
			/* Constant one segment */
			.x0 = 1.0,
			.x1 = HUGE_VAL,
			.Type = 6,
			.Params = { 1.0, 0.0, 0.0, 1.0 },
		}
	};

	return cmsBuildSegmentedToneCurve(ctx, ARRAY_LENGTH(segments), segments);
}

cmsToneCurve *
build_MPE_curve(cmsContext ctx, enum transfer_fn fn)
{
	switch (fn) {
	case TRANSFER_FN_ADOBE_RGB_EOTF:
		return build_MPE_curve_power(ctx, 563.0 / 256.0);
	case TRANSFER_FN_ADOBE_RGB_EOTF_INVERSE:
		return build_MPE_curve_power(ctx, 256.0 / 563.0);
	case TRANSFER_FN_POWER2_4_EOTF:
		return build_MPE_curve_power(ctx, 2.4);
	case TRANSFER_FN_POWER2_4_EOTF_INVERSE:
		return build_MPE_curve_power(ctx, 1.0 / 2.4);
	case TRANSFER_FN_SRGB_EOTF:
		return build_MPE_curve_sRGB(ctx);
	case TRANSFER_FN_SRGB_EOTF_INVERSE:
		return build_MPE_curve_sRGB_inv(ctx);
	default:
		assert(0 && "unimplemented MPE curve");
	}

	return NULL;
}

cmsStage *
build_MPE_curve_stage(cmsContext context_id, enum transfer_fn fn)
{
	cmsToneCurve *c;
	cmsStage *stage;

	c = build_MPE_curve(context_id, fn);
	stage = cmsStageAllocToneCurves(context_id, 3,
					(cmsToneCurve *[3]){ c, c, c });
	assert(stage);
	cmsFreeToneCurve(c);

	return stage;
}

/* This function is taken from LittleCMS, pardon the odd style */
cmsBool
SetTextTags(cmsHPROFILE hProfile, const wchar_t* Description)
{
	cmsMLU *DescriptionMLU, *CopyrightMLU;
	cmsBool  rc = FALSE;
	cmsContext ContextID = cmsGetProfileContextID(hProfile);

	DescriptionMLU  = cmsMLUalloc(ContextID, 1);
	CopyrightMLU    = cmsMLUalloc(ContextID, 1);

	if (DescriptionMLU == NULL || CopyrightMLU == NULL) goto Error;

	if (!cmsMLUsetWide(DescriptionMLU,  "en", "US", Description)) goto Error;
	if (!cmsMLUsetWide(CopyrightMLU,    "en", "US", L"No copyright, use freely")) goto Error;

	if (!cmsWriteTag(hProfile, cmsSigProfileDescriptionTag,  DescriptionMLU)) goto Error;
	if (!cmsWriteTag(hProfile, cmsSigCopyrightTag,           CopyrightMLU)) goto Error;

	rc = TRUE;

Error:

	if (DescriptionMLU)
		cmsMLUfree(DescriptionMLU);
	if (CopyrightMLU)
		cmsMLUfree(CopyrightMLU);
	return rc;
}
