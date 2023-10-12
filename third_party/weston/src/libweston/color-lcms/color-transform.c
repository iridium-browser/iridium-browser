/*
 * Copyright 2021-2022 Collabora, Ltd.
 * Copyright 2021-2022 Advanced Micro Devices, Inc.
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
#include <libweston/libweston.h>
#include <lcms2_plugin.h>

#include "color.h"
#include "color-lcms.h"
#include "shared/helpers.h"
#include "shared/string-helpers.h"
#include "shared/xalloc.h"

/**
 * LCMS compares this parameter with the actual version of the LCMS and enforces
 * the minimum version is plug-in. If the actual LCMS version is lower than the
 * plug-in requirement the function cmsCreateContext is failed with plug-in as
 * parameter.
 */
#define REQUIRED_LCMS_VERSION 2120

/** Precision for detecting identity matrix */
#define MATRIX_PRECISION_BITS 12

/**
 * The method is used in linearization of an arbitrary color profile
 * when EOTF is retrieved we want to know a generic way to decide the number
 * of points
 */
unsigned int
cmlcms_reasonable_1D_points(void)
{
	return 1024;
}

static unsigned int
cmlcms_reasonable_3D_points(void)
{
	return 33;
}

static void
fill_in_curves(cmsToneCurve *curves[3], float *values, unsigned len)
{
	float *R_lut = values;
	float *G_lut = R_lut + len;
	float *B_lut = G_lut + len;
	unsigned i;
	cmsFloat32Number x;

	assert(len > 1);
	for (i = 0; i < 3; i++)
		assert(curves[i]);

	for (i = 0; i < len; i++) {
		x = (double)i / (len - 1);
		R_lut[i] = cmsEvalToneCurveFloat(curves[0], x);
		G_lut[i] = cmsEvalToneCurveFloat(curves[1], x);
		B_lut[i] = cmsEvalToneCurveFloat(curves[2], x);
	}
}

static void
cmlcms_fill_in_output_inv_eotf_vcgt(struct weston_color_transform *xform_base,
				    float *values, unsigned len)
{
	struct cmlcms_color_transform *xform = get_xform(xform_base);
	struct cmlcms_color_profile *p = xform->search_key.output_profile;

	assert(p && "output_profile");
	fill_in_curves(p->output_inv_eotf_vcgt, values, len);
}

static void
cmlcms_fill_in_pre_curve(struct weston_color_transform *xform_base,
			 float *values, unsigned len)
{
	struct cmlcms_color_transform *xform = get_xform(xform_base);

	fill_in_curves(xform->pre_curve, values, len);
}

static void
cmlcms_fill_in_post_curve(struct weston_color_transform *xform_base,
			 float *values, unsigned len)
{
	struct cmlcms_color_transform *xform = get_xform(xform_base);

	fill_in_curves(xform->post_curve, values, len);
}

/**
 * Clamp value to [0.0, 1.0], except pass NaN through.
 *
 * This function is not intended for hiding NaN.
 */
static float
ensure_unorm(float v)
{
	if (v <= 0.0f)
		return 0.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
}

static void
cmlcms_fill_in_3dlut(struct weston_color_transform *xform_base,
		     float *lut, unsigned int len)
{
	struct cmlcms_color_transform *xform = get_xform(xform_base);
	float rgb_in[3];
	float rgb_out[3];
	unsigned int index;
	unsigned int value_b, value_r, value_g;
	float divider = len - 1;

	assert(xform->search_key.category == CMLCMS_CATEGORY_INPUT_TO_BLEND ||
	       xform->search_key.category == CMLCMS_CATEGORY_INPUT_TO_OUTPUT);

	for (value_b = 0; value_b < len; value_b++) {
		for (value_g = 0; value_g < len; value_g++) {
			for (value_r = 0; value_r < len; value_r++) {
				rgb_in[0] = (float)value_r / divider;
				rgb_in[1] = (float)value_g / divider;
				rgb_in[2] = (float)value_b / divider;

				cmsDoTransform(xform->cmap_3dlut, rgb_in, rgb_out, 1);

				index = 3 * (value_r + len * (value_g + len * value_b));
				lut[index    ] = ensure_unorm(rgb_out[0]);
				lut[index + 1] = ensure_unorm(rgb_out[1]);
				lut[index + 2] = ensure_unorm(rgb_out[2]);
			}
		}
	}
}

void
cmlcms_color_transform_destroy(struct cmlcms_color_transform *xform)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(xform->base.cm);

	wl_list_remove(&xform->link);

	cmsFreeToneCurveTriple(xform->pre_curve);

	if (xform->cmap_3dlut)
		cmsDeleteTransform(xform->cmap_3dlut);

	cmsFreeToneCurveTriple(xform->post_curve);

	if (xform->lcms_ctx)
		cmsDeleteContext(xform->lcms_ctx);

	unref_cprof(xform->search_key.input_profile);
	unref_cprof(xform->search_key.output_profile);

	weston_log_scope_printf(cm->transforms_scope,
				"Destroyed color transformation %p.\n", xform);

	free(xform);
}

/**
 * Matrix infinity norm
 *
 * http://www.netlib.org/lapack/lug/node75.html
 */
static double
matrix_inf_norm(const cmsMAT3 *mat)
{
	unsigned row;
	double infnorm = -1.0;

	for (row = 0; row < 3; row++) {
		unsigned col;
		double sum = 0.0;

		for (col = 0; col < 3; col++)
			sum += fabs(mat->v[col].n[row]);

		if (infnorm < sum)
			infnorm = sum;
	}

	return infnorm;
}

/*
 * The method of testing for identity matrix is from
 * https://gitlab.freedesktop.org/pq/fourbyfour/-/blob/master/README.d/precision_testing.md#inversion-error
 */
static bool
matrix_is_identity(const cmsMAT3 *mat, int bits_precision)
{
	cmsMAT3 tmp = *mat;
	double err;
	int i;

	/* subtract identity matrix */
	for (i = 0; i < 3; i++)
		tmp.v[i].n[i] -= 1.0;

	err = matrix_inf_norm(&tmp);

	return -log2(err) >= bits_precision;
}

static const cmsMAT3 *
stage_matrix_transpose(const _cmsStageMatrixData *smd)
{
	/* smd is row-major, cmsMAT3 is column-major */
	return (const cmsMAT3 *)smd->Double;
}

static bool
is_matrix_stage_with_zero_offset(const cmsStage *stage)
{
	const _cmsStageMatrixData *data;
	int rows;
	int r;

	if (!stage || cmsStageType(stage) != cmsSigMatrixElemType)
		return false;

	data = cmsStageData(stage);
	if (!data->Offset)
		return true;

	rows = cmsStageOutputChannels(stage);
	for (r = 0; r < rows; r++)
		if (data->Offset[r] != 0.0f)
			return false;

	return true;
}

static bool
is_identity_matrix_stage(const cmsStage *stage)
{
	_cmsStageMatrixData *data;

	if (!is_matrix_stage_with_zero_offset(stage))
		return false;

	data = cmsStageData(stage);
	return matrix_is_identity(stage_matrix_transpose(data),
				  MATRIX_PRECISION_BITS);
}

/* Returns the matrix (next * prev). */
static cmsStage *
multiply_matrix_stages(cmsContext context_id, cmsStage *next, cmsStage *prev)
{
	_cmsStageMatrixData *prev_, *next_;
	cmsMAT3 res;
	cmsStage *ret;

	prev_ = cmsStageData(prev);
	next_ = cmsStageData(next);

	/* res = prev^T * next^T */
	_cmsMAT3per(&res, stage_matrix_transpose(next_),
		    stage_matrix_transpose(prev_));

	/*
	 * res is column-major while Alloc function takes row-major;
	 * the cast effectively transposes the matrix.
	 * We return (prev^T * next^T)^T = next * prev.
	 */
	ret = cmsStageAllocMatrix(context_id, 3, 3,
				  (const cmsFloat64Number*)&res, NULL);
	abort_oom_if_null(ret);
	return ret;
}

/** Merge consecutive matrices into a single matrix, and drop identity matrices
 *
 * If we have a pipeline { M1, M2, M3 } of matrices only, then the total
 * operation is the matrix M = M3 * M2 * M1 because the pipeline first applies
 * M1, then M2, and finally M3.
 */
static bool
merge_matrices(cmsPipeline **lut, cmsContext context_id)
{
	cmsPipeline *pipe;
	cmsStage *elem;
	cmsStage *prev = NULL;
	cmsStage *freeme = NULL;
	bool modified = false;

	pipe = cmsPipelineAlloc(context_id, 3, 3);
	abort_oom_if_null(pipe);

	elem = cmsPipelineGetPtrToFirstStage(*lut);
	do {
		if (is_matrix_stage_with_zero_offset(prev) &&
		    is_matrix_stage_with_zero_offset(elem)) {
			/* replace the two matrices with a merged one */
			prev = multiply_matrix_stages(context_id, elem, prev);
			if (freeme)
				cmsStageFree(freeme);
			freeme = prev;
			modified = true;
		} else {
			if (prev) {
				if (is_identity_matrix_stage(prev)) {
					/* skip inserting it */
					modified = true;
				} else {
					cmsPipelineInsertStage(pipe, cmsAT_END,
							       cmsStageDup(prev));
				}
			}
			prev = elem;
		}

		if (elem)
			elem = cmsStageNext(elem);
	} while (prev);

	if (freeme)
		cmsStageFree(freeme);

	cmsPipelineFree(*lut);
	*lut = pipe;

	return modified;
}

/*
 * XXX: Joining curve sets pair by pair might cause precision problems,
 * especially as we convert even analytical curve types into tabulated.
 * It might be preferable to convert a whole chain of curve sets at once
 * instead.
 */
static cmsStage *
join_curvesets(cmsContext context_id, const cmsStage *prev,
	       const cmsStage *next, unsigned int num_samples)
{
	_cmsStageToneCurvesData *prev_, *next_;
	cmsToneCurve *arr[3];
	cmsUInt32Number i;
	cmsStage *ret = NULL;

	prev_ = cmsStageData(prev);
	next_ = cmsStageData(next);

	assert(prev_->nCurves == ARRAY_LENGTH(arr));
	assert(next_->nCurves == ARRAY_LENGTH(arr));

	for (i = 0; i < ARRAY_LENGTH(arr); i++) {
		arr[i] = lcmsJoinToneCurve(context_id, prev_->TheCurves[i],
					   next_->TheCurves[i], num_samples);
		abort_oom_if_null(arr[i]);
	}

	ret = cmsStageAllocToneCurves(context_id, ARRAY_LENGTH(arr), arr);
	abort_oom_if_null(ret);
	cmsFreeToneCurveTriple(arr);
	return ret;
}

static bool
is_identity_curve_stage(const cmsStage *stage)
{
	const _cmsStageToneCurvesData *data;
	unsigned int i;
	bool is_identity = true;

	assert(stage);

	if (cmsStageType(stage) != cmsSigCurveSetElemType)
		return false;

	data = cmsStageData(stage);
	for (i = 0; i < data->nCurves; i++)
		is_identity &= cmsIsToneCurveLinear(data->TheCurves[i]);

	return is_identity;
}

static bool
merge_curvesets(cmsPipeline **lut, cmsContext context_id)
{
	cmsPipeline *pipe;
	cmsStage *elem;
	cmsStage *prev = NULL;
	cmsStage *freeme = NULL;
	bool modified = false;

	pipe = cmsPipelineAlloc(context_id, 3, 3);
	abort_oom_if_null(pipe);

	elem = cmsPipelineGetPtrToFirstStage(*lut);
	do {
		if (prev && cmsStageType(prev) == cmsSigCurveSetElemType &&
		    elem && cmsStageType(elem) == cmsSigCurveSetElemType) {
			/* Replace two curve set elements with a merged one. */
			prev = join_curvesets(context_id, prev, elem,
					      cmlcms_reasonable_1D_points());
			if (freeme)
				cmsStageFree(freeme);
			freeme = prev;
			modified = true;
		} else {
			if (prev) {
				if (is_identity_curve_stage(prev)) {
					/* skip inserting it */
					modified = true;
				} else {
					cmsPipelineInsertStage(pipe, cmsAT_END,
							cmsStageDup(prev));
				}
			}
			prev = elem;
		}

		if (elem)
			elem = cmsStageNext(elem);
	} while (prev);

	if (freeme)
		cmsStageFree(freeme);

	cmsPipelineFree(*lut);
	*lut = pipe;

	return modified;
}

static bool
translate_curve_element(struct weston_color_curve *curve,
			cmsToneCurve *stash[3],
			void (*func)(struct weston_color_transform *xform,
				     float *values, unsigned len),
			cmsStage *elem)
{
	_cmsStageToneCurvesData *trc_data;
	unsigned i;

	assert(cmsStageType(elem) == cmsSigCurveSetElemType);

	trc_data = cmsStageData(elem);
	if (trc_data->nCurves != 3)
		return false;

	curve->type = WESTON_COLOR_CURVE_TYPE_LUT_3x1D;
	curve->u.lut_3x1d.fill_in = func;
	curve->u.lut_3x1d.optimal_len = cmlcms_reasonable_1D_points();

	for (i = 0; i < 3; i++) {
		stash[i] = cmsDupToneCurve(trc_data->TheCurves[i]);
		abort_oom_if_null(stash[i]);
	}

	return true;
}

static bool
translate_matrix_element(struct weston_color_mapping *map, cmsStage *elem)
{
	_cmsStageMatrixData *data = cmsStageData(elem);
	int c, r;

	if (!is_matrix_stage_with_zero_offset(elem))
		return false;

	if (cmsStageInputChannels(elem) != 3 ||
	    cmsStageOutputChannels(elem) != 3)
		return false;

	map->type = WESTON_COLOR_MAPPING_TYPE_MATRIX;

	/*
	 * map->u.mat.matrix is column-major, while
	 * data->Double is row-major.
	 */
	for (c = 0; c < 3; c++)
		for (r = 0; r < 3; r++)
			map->u.mat.matrix[c * 3 + r] = data->Double[r * 3 + c];

	return true;
}

static bool
translate_pipeline(struct cmlcms_color_transform *xform, const cmsPipeline *lut)
{
	cmsStage *elem;

	xform->base.pre_curve.type = WESTON_COLOR_CURVE_TYPE_IDENTITY;
	xform->base.mapping.type = WESTON_COLOR_MAPPING_TYPE_IDENTITY;
	xform->base.post_curve.type = WESTON_COLOR_CURVE_TYPE_IDENTITY;

	elem = cmsPipelineGetPtrToFirstStage(lut);

	if (!elem)
		return true;

	if (cmsStageType(elem) == cmsSigCurveSetElemType) {
		if (!translate_curve_element(&xform->base.pre_curve,
					     xform->pre_curve,
					     cmlcms_fill_in_pre_curve, elem))
			return false;

		elem = cmsStageNext(elem);
	}

	if (!elem)
		return true;

	if (cmsStageType(elem) == cmsSigMatrixElemType) {
		if (!translate_matrix_element(&xform->base.mapping, elem))
			return false;

		elem = cmsStageNext(elem);
	}

	if (!elem)
		return true;

	if (cmsStageType(elem) == cmsSigCurveSetElemType) {
		if (!translate_curve_element(&xform->base.post_curve,
					     xform->post_curve,
					     cmlcms_fill_in_post_curve, elem))
			return false;

		elem = cmsStageNext(elem);
	}

	if (!elem)
		return true;

	return false;
}

static cmsBool
optimize_float_pipeline(cmsPipeline **lut, cmsContext context_id,
			struct cmlcms_color_transform *xform)
{
	bool cont_opt;

	/**
	 * This optimization loop will delete identity stages. Deleting
	 * identity matrix stages is harmless, but deleting identity
	 * curve set stages also removes the implicit clamping they do
	 * on their input values.
	 */
	do {
		cont_opt = merge_matrices(lut, context_id);
		cont_opt |= merge_curvesets(lut, context_id);
	} while (cont_opt);

	if (translate_pipeline(xform, *lut)) {
		xform->status = CMLCMS_TRANSFORM_OPTIMIZED;
		return TRUE;
	}

	xform->base.pre_curve.type = WESTON_COLOR_CURVE_TYPE_IDENTITY;
	xform->base.mapping.type = WESTON_COLOR_MAPPING_TYPE_3D_LUT;
	xform->base.mapping.u.lut3d.fill_in = cmlcms_fill_in_3dlut;
	xform->base.mapping.u.lut3d.optimal_len = cmlcms_reasonable_3D_points();
	xform->base.post_curve.type = WESTON_COLOR_CURVE_TYPE_IDENTITY;

	xform->status = CMLCMS_TRANSFORM_3DLUT;

	/*
	 * We use cmsDoTransform() to realize the 3D LUT. Return false so
	 * that LittleCMS installs its usual float transform machinery,
	 * running on the pipeline we optimized here.
	 */
	return FALSE;
}

static const char *
cmlcms_stage_type_to_str(cmsStage *stage)
{
	/* This table is based on cmsStageSignature enum type from the
	 * LittleCMS API. */
	switch (cmsStageType(stage))
	{
	case cmsSigCurveSetElemType:
		return "CurveSet";
	case cmsSigMatrixElemType:
		return "Matrix";
	case cmsSigCLutElemType:
		return "CLut";
	case cmsSigBAcsElemType:
		return "BAcs";
	case cmsSigEAcsElemType:
		return "EAcs";
	case cmsSigXYZ2LabElemType:
		return "XYZ2Lab";
	case cmsSigLab2XYZElemType:
		return "Lab2XYz";
	case cmsSigNamedColorElemType:
		return "NamedColor";
	case cmsSigLabV2toV4:
		return "LabV2toV4";
	case cmsSigLabV4toV2:
		return "LabV4toV2";
	case cmsSigIdentityElemType:
		return "Identity";
	case cmsSigLab2FloatPCS:
		return "Lab2FloatPCS";
	case cmsSigFloatPCS2Lab:
		return "FloatPCS2Lab";
	case cmsSigXYZ2FloatPCS:
		return "XYZ2FloatPCS";
	case cmsSigFloatPCS2XYZ:
		return "FloatPCS2XYZ";
	case cmsSigClipNegativesElemType:
		return "ClipNegatives";
	}

	return NULL;
}

static void
matrix_print(cmsStage *stage, struct weston_log_scope *scope)
{
	const _cmsStageMatrixData *data;
	const unsigned int SIZE = 3;
	unsigned int row, col;
	double elem;
	const char *sep;

	if (!weston_log_scope_is_enabled(scope))
		return;

	assert(cmsStageType(stage) == cmsSigMatrixElemType);
	data = cmsStageData(stage);

	for (row = 0; row < SIZE; row++) {
		weston_log_scope_printf(scope, "      ");

		for (col = 0, sep = ""; col < SIZE; col++) {
			elem = data->Double[row * SIZE + col];
			weston_log_scope_printf(scope, "%s% .4f", sep, elem);
			sep = " ";
		}

		/* We print offset after the last column of the matrix. */
		if (data->Offset)
			weston_log_scope_printf(scope, "% .4f", data->Offset[row]);

		weston_log_scope_printf(scope, "\n");
	}
}

static void
pipeline_print(cmsPipeline **lut, cmsContext context_id,
	       struct weston_log_scope *scope)
{
	cmsStage *stage = cmsPipelineGetPtrToFirstStage(*lut);
	const char *type_str;

	if (!weston_log_scope_is_enabled(scope))
		return;

	if (!stage) {
		weston_log_scope_printf(scope, "no elements\n");
		return;
	}

	while (stage != NULL) {
		type_str = cmlcms_stage_type_to_str(stage);
		/* Unknown type, just print the hex */
		if (!type_str)
			weston_log_scope_printf(scope, "    unknown type 0x%x\n",
						cmsStageType(stage));
		else
			weston_log_scope_printf(scope, "    %s\n", type_str);

		switch(cmsStageType(stage)) {
		case cmsSigMatrixElemType:
			matrix_print(stage, scope);
			break;
		default:
			break;
		}

		stage = cmsStageNext(stage);
	}
}

/** LittleCMS transform plugin entry point
 *
 * This function is called by LittleCMS when it is creating a new
 * cmsHTRANSFORM. We have the opportunity to inspect and override everything.
 * The initial cmsPipeline resulting from e.g.
 * cmsCreateMultiprofileTransformTHR() is handed to us for inspection before
 * the said function call returns.
 *
 * \param xform_fn If we handle the given transformation, we should assign
 * our own transformation function here. We do not do that, because:
 * a) Even when we optimize the pipeline, but do not handle the transformation,
 *    we rely on LittleCMS' own float transformation machinery.
 * b) When we do handle the transformation, we will not be calling
 *    cmsDoTransform() anymore.
 *
 * \param user_data We could store a void pointer to custom user data
 * through this pointer to be carried with the cmsHTRANSFORM.
 * Here none is needed.
 *
 * \param free_private_data_fn We could store a function pointer for freeing
 * our user data when the cmsHTRANSFORM is destroyed. None needed.
 *
 * \param lut The LittleCMS pipeline that describes this transformation.
 * We can create our own and replace the original completely in
 * optimize_float_pipeline().
 *
 * \param input_format Pointer to the format used as input for this
 * transformation. I suppose we could override it if we wanted to, but
 * no need.
 *
 * \param output_format Similar to input format.
 *
 * \param flags Some flags we could also override? See cmsFLAGS_* defines.
 *
 * \return If this returns TRUE, it implies we handle the transformation. No
 * other plugin will be tried anymore and the transformation object is
 * complete. If this returns FALSE, the search for a plugin to handle this
 * transformation continues and falls back to the usual handling inside
 * LittleCMS.
 */
static cmsBool
transform_factory(_cmsTransform2Fn *xform_fn,
		  void **user_data,
		  _cmsFreeUserDataFn *free_private_data_fn,
		  cmsPipeline **lut,
		  cmsUInt32Number *input_format,
		  cmsUInt32Number *output_format,
		  cmsUInt32Number *flags)
{
	struct weston_color_manager_lcms *cm;
	struct cmlcms_color_transform *xform;
	cmsContext context_id;
	bool ret;

	if (T_CHANNELS(*input_format) != 3) {
		weston_log("color-lcms debug: input format is not 3-channel.");
		return FALSE;
	}
	if (T_CHANNELS(*output_format) != 3) {
		weston_log("color-lcms debug: output format is not 3-channel.");
		return FALSE;
	}
	if (!T_FLOAT(*input_format)) {
		weston_log("color-lcms debug: input format is not float.");
		return FALSE;
	}
	if (!T_FLOAT(*output_format)) {
		weston_log("color-lcms debug: output format is not float.");
		return FALSE;
	}
	context_id = cmsGetPipelineContextID(*lut);
	assert(context_id);
	xform = cmsGetContextUserData(context_id);
	assert(xform);

	cm = get_cmlcms(xform->base.cm);

	/* Print pipeline before optimization */
	weston_log_scope_printf(cm->optimizer_scope,
				"  transform pipeline before optimization:\n");
	pipeline_print(lut, context_id, cm->optimizer_scope);

	/* Optimize pipeline */
	ret = optimize_float_pipeline(lut, context_id, xform);

	/* Print pipeline after optimization */
	weston_log_scope_printf(cm->optimizer_scope,
				"  transform pipeline after optimization:\n");
	pipeline_print(lut, context_id, cm->optimizer_scope);

	return ret;
}

static cmsPluginTransform transform_plugin = {
	.base = {
		.Magic = cmsPluginMagicNumber,
		.ExpectedVersion = REQUIRED_LCMS_VERSION,
		.Type = cmsPluginTransformSig,
		.Next = NULL
	},
	.factories.xform = transform_factory,
};

static void
lcms_xform_error_logger(cmsContext context_id,
			cmsUInt32Number error_code,
			const char *text)
{
	struct cmlcms_color_transform *xform;
	struct cmlcms_color_profile *in;
	struct cmlcms_color_profile *out;

	xform = cmsGetContextUserData(context_id);
	in = xform->search_key.input_profile;
	out = xform->search_key.output_profile;

	weston_log("LittleCMS error with color transformation from "
		   "'%s' to '%s', %s: %s\n",
		   in ? in->base.description : "(none)",
		   out ? out->base.description : "(none)",
		   cmlcms_category_name(xform->search_key.category),
		   text);
}

static cmsHPROFILE
profile_from_rgb_curves(cmsContext ctx, cmsToneCurve *const curveset[3])
{
	cmsHPROFILE p;
	int i;

	for (i = 0; i < 3; i++)
		assert(curveset[i]);

	p = cmsCreateLinearizationDeviceLinkTHR(ctx, cmsSigRgbData, curveset);
	abort_oom_if_null(p);

	return p;
}

static bool
xform_realize_chain(struct cmlcms_color_transform *xform)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(xform->base.cm);
	struct cmlcms_color_profile *output_profile = xform->search_key.output_profile;
	cmsHPROFILE chain[5];
	unsigned chain_len = 0;
	cmsHPROFILE extra = NULL;

	chain[chain_len++] = xform->search_key.input_profile->profile;
	chain[chain_len++] = output_profile->profile;

	switch (xform->search_key.category) {
	case CMLCMS_CATEGORY_INPUT_TO_BLEND:
		/* Add linearization step to make blending well-defined. */
		extra = profile_from_rgb_curves(cm->lcms_ctx, output_profile->eotf);
		chain[chain_len++] = extra;
		break;
	case CMLCMS_CATEGORY_INPUT_TO_OUTPUT:
		/* Just add VCGT if it is provided. */
		if (output_profile->vcgt[0]) {
			extra = profile_from_rgb_curves(cm->lcms_ctx,
							output_profile->vcgt);
			chain[chain_len++] = extra;
		}
		break;
	case CMLCMS_CATEGORY_BLEND_TO_OUTPUT:
		assert(0 && "category handled in the caller");
		return false;
	}

	assert(chain_len <= ARRAY_LENGTH(chain));

	/**
	 * Binding to our LittleCMS plug-in occurs here.
	 * If you want to disable the plug-in while debugging,
	 * replace &transform_plugin with NULL.
	 */
	xform->lcms_ctx = cmsCreateContext(&transform_plugin, xform);
	abort_oom_if_null(xform->lcms_ctx);
	cmsSetLogErrorHandlerTHR(xform->lcms_ctx, lcms_xform_error_logger);

	assert(xform->status == CMLCMS_TRANSFORM_FAILED);
	/* transform_factory() is invoked by this call. */
	xform->cmap_3dlut = cmsCreateMultiprofileTransformTHR(xform->lcms_ctx,
							      chain,
							      chain_len,
							      TYPE_RGB_FLT,
							      TYPE_RGB_FLT,
							      xform->search_key.intent_output,
							      0);
	cmsCloseProfile(extra);

	if (!xform->cmap_3dlut)
		goto failed;

	if (xform->status != CMLCMS_TRANSFORM_3DLUT) {
		cmsDeleteTransform(xform->cmap_3dlut);
		xform->cmap_3dlut = NULL;
	}

	switch (xform->status) {
	case CMLCMS_TRANSFORM_FAILED:
		goto failed;
	case CMLCMS_TRANSFORM_OPTIMIZED:
	case CMLCMS_TRANSFORM_3DLUT:
		break;
	}

	return true;

failed:
	cmsDeleteContext(xform->lcms_ctx);
	xform->lcms_ctx = NULL;

	return false;
}

char *
cmlcms_color_transform_search_param_string(const struct cmlcms_color_transform_search_param *search_key)
{
	const char *input_prof_desc = "none", *output_prof_desc = "none";
	char *str;

	if (search_key->input_profile)
		input_prof_desc = search_key->input_profile->base.description;

	if (search_key->output_profile)
		output_prof_desc = search_key->output_profile->base.description;

	str_printf(&str, "  catergory: %s\n" \
			 "  input profile: %s\n" \
			 "  output profile: %s\n" \
			 "  selected intent from output profile: %u\n",
			 cmlcms_category_name(search_key->category),
			 input_prof_desc,
			 output_prof_desc,
			 search_key->intent_output);

	abort_oom_if_null(str);

	return str;
}

static struct cmlcms_color_transform *
cmlcms_color_transform_create(struct weston_color_manager_lcms *cm,
			      const struct cmlcms_color_transform_search_param *search_param)
{
	struct cmlcms_color_transform *xform;
	const char *err_msg;
	char *str;

	xform = xzalloc(sizeof *xform);
	weston_color_transform_init(&xform->base, &cm->base);
	wl_list_init(&xform->link);
	xform->search_key = *search_param;
	xform->search_key.input_profile = ref_cprof(search_param->input_profile);
	xform->search_key.output_profile = ref_cprof(search_param->output_profile);

	weston_log_scope_printf(cm->transforms_scope,
				"New color transformation: %p\n", xform);
	str = cmlcms_color_transform_search_param_string(&xform->search_key);
	weston_log_scope_printf(cm->transforms_scope, "%s", str);
	free(str);

	/* Ensure the linearization etc. have been extracted. */
	if (!search_param->output_profile->eotf[0]) {
		if (!retrieve_eotf_and_output_inv_eotf(cm->lcms_ctx,
						       search_param->output_profile->profile,
						       search_param->output_profile->eotf,
						       search_param->output_profile->output_inv_eotf_vcgt,
						       search_param->output_profile->vcgt,
						       cmlcms_reasonable_1D_points())) {
			err_msg = "retrieve_eotf_and_output_inv_eotf failed";
			goto error;
		}
	}

	/*
	 * The blending space is chosen to be the output device space but
	 * linearized. This means that BLEND_TO_OUTPUT only needs to
	 * undo the linearization and add VCGT.
	 */
	switch (search_param->category) {
	case CMLCMS_CATEGORY_INPUT_TO_BLEND:
	case CMLCMS_CATEGORY_INPUT_TO_OUTPUT:
		if (!xform_realize_chain(xform)) {
			err_msg = "xform_realize_chain failed";
			goto error;
		}
		break;
	case CMLCMS_CATEGORY_BLEND_TO_OUTPUT:
		xform->base.pre_curve.type = WESTON_COLOR_CURVE_TYPE_LUT_3x1D;
		xform->base.pre_curve.u.lut_3x1d.fill_in = cmlcms_fill_in_output_inv_eotf_vcgt;
		xform->base.pre_curve.u.lut_3x1d.optimal_len =
				cmlcms_reasonable_1D_points();
		xform->status = CMLCMS_TRANSFORM_OPTIMIZED;
		break;
	}

	wl_list_insert(&cm->color_transform_list, &xform->link);
	assert(xform->status != CMLCMS_TRANSFORM_FAILED);

	str = weston_color_transform_string(&xform->base);
	weston_log_scope_printf(cm->transforms_scope, "  %s", str);
	free(str);

	return xform;

error:
	weston_log_scope_printf(cm->transforms_scope,
				"	%s\n", err_msg);
	cmlcms_color_transform_destroy(xform);
	return NULL;
}

static bool
transform_matches_params(const struct cmlcms_color_transform *xform,
			 const struct cmlcms_color_transform_search_param *param)
{
	if (xform->search_key.category != param->category)
		return false;

	if (xform->search_key.intent_output  != param->intent_output ||
	    xform->search_key.output_profile != param->output_profile ||
	    xform->search_key.input_profile != param->input_profile)
		return false;

	return true;
}

struct cmlcms_color_transform *
cmlcms_color_transform_get(struct weston_color_manager_lcms *cm,
			   const struct cmlcms_color_transform_search_param *param)
{
	struct cmlcms_color_transform *xform;

	wl_list_for_each(xform, &cm->color_transform_list, link) {
		if (transform_matches_params(xform, param)) {
			weston_color_transform_ref(&xform->base);
			return xform;
		}
	}

	xform = cmlcms_color_transform_create(cm, param);
	if (!xform)
		weston_log("color-lcms error: failed to create a color transformation.\n");

	return xform;
}
