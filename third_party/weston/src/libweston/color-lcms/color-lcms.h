/*
 * Copyright 2021 Collabora, Ltd.
 * Copyright 2021 Advanced Micro Devices, Inc.
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

#ifndef WESTON_COLOR_LCMS_H
#define WESTON_COLOR_LCMS_H

#include <lcms2.h>
#include <libweston/libweston.h>
#include <libweston/weston-log.h>

#include "color.h"
#include "shared/helpers.h"

struct weston_color_manager_lcms {
	struct weston_color_manager base;
	struct weston_log_scope *profiles_scope;
	struct weston_log_scope *transforms_scope;
	struct weston_log_scope *optimizer_scope;
	cmsContext lcms_ctx;

	struct wl_list color_transform_list; /* cmlcms_color_transform::link */
	struct wl_list color_profile_list; /* cmlcms_color_profile::link */
	struct cmlcms_color_profile *sRGB_profile; /* stock profile */
};

static inline struct weston_color_manager_lcms *
get_cmlcms(struct weston_color_manager *cm_base)
{
	return container_of(cm_base, struct weston_color_manager_lcms, base);
}

struct cmlcms_md5_sum {
	uint8_t bytes[16];
};

struct cmlcms_color_profile {
	struct weston_color_profile base;

	/* struct weston_color_manager_lcms::color_profile_list */
	struct wl_list link;

	cmsHPROFILE profile;
	struct cmlcms_md5_sum md5sum;

	/** The curves to decode an electrical signal
	 *
	 * For ICC profiles, if the profile type is matrix-shaper, then eotf
	 * contains the TRC, otherwise eotf contains an approximated EOTF if the
	 * profile is used for output.
	 * The field may be populated on demand.
	 */
	cmsToneCurve *eotf[3];

	/**
	 * If the profile does support being an output profile and it is used as an
	 * output then this field represents a concatenation of inverse EOTF + VCGT,
	 * if the tag exists and it can not be null.
	 * VCGT is part of monitor calibration which means: even though we must
	 * apply VCGT in the compositor, we pretend that it happens inside the
	 * monitor. This is how the classic color management and ICC profiles work.
	 * The ICC profile (ignoring the VCGT tag) characterizes the output which
	 * is VCGT + monitor behavior. The field is null only if the profile is not
	 * usable as an output profile. The field is set when cmlcms_color_profile
	 * is created.
	 */
	cmsToneCurve *output_inv_eotf_vcgt[3];

	/**
	 * VCGT tag cached from output profile, it could be null if not exist
	 */
	cmsToneCurve *vcgt[3];
};

/**
 *  Type of LCMS transforms
 */
enum cmlcms_category {
	/**
	 * Uses combination of input profile with output profile, but
	 * without INV EOTF or with additional EOTF in the transform pipeline
	 * input→blend = input profile + output profile + output EOTF
	 */
	CMLCMS_CATEGORY_INPUT_TO_BLEND = 0,

	/**
	 * Uses INV EOTF only concatenated with VCGT tag if present
	 * blend→output = output inverse EOTF + VCGT
	 */
	CMLCMS_CATEGORY_BLEND_TO_OUTPUT,

	/**
	 * Transform uses input profile and output profile as is
	 * input→output = input profile + output profile + VCGT
	 */
	CMLCMS_CATEGORY_INPUT_TO_OUTPUT,
};

const char *
cmlcms_category_name(enum cmlcms_category cat);

static inline struct cmlcms_color_profile *
get_cprof(struct weston_color_profile *cprof_base)
{
	return container_of(cprof_base, struct cmlcms_color_profile, base);
}

bool
cmlcms_get_color_profile_from_icc(struct weston_color_manager *cm,
				  const void *icc_data,
				  size_t icc_len,
				  const char *name_part,
				  struct weston_color_profile **cprof_out,
				  char **errmsg);

void
cmlcms_destroy_color_profile(struct weston_color_profile *cprof_base);


struct cmlcms_color_transform_search_param {
	enum cmlcms_category category;
	struct cmlcms_color_profile *input_profile;
	struct cmlcms_color_profile *output_profile;
	cmsUInt32Number intent_output;	/* selected intent from output profile */
};

struct cmlcms_color_transform {
	struct weston_color_transform base;

	/* weston_color_manager_lcms::color_transform_list */
	struct wl_list link;

	struct cmlcms_color_transform_search_param search_key;

	/*
	 * Cached data in case weston_color_transform needs them.
	 * Pre-curve and post-curve refer to the weston_color_transform
	 * pipeline elements and have no semantic meaning. They both are a
	 * result of optimizing an arbitrary LittleCMS pipeline, not
	 * e.g. EOTF or VCGT per se.
	 */
	cmsToneCurve *pre_curve[3];
	cmsToneCurve *post_curve[3];

	/**
	 * 3D LUT color mapping part of the transformation, if needed by the
	 * weston_color_transform. This is used as a fallback when an
	 * arbitrary LittleCMS pipeline cannot be translated into a more
	 * specific form.
	 */
	cmsHTRANSFORM cmap_3dlut;

	/**
	 * Certain categories of transformations need their own LittleCMS
	 * contexts in order to use our LittleCMS plugin.
	 */
	cmsContext lcms_ctx;

	/**
	 * The result of pipeline construction, optimization, and analysis.
	 */
	enum {
		/** Error producing a pipeline */
		CMLCMS_TRANSFORM_FAILED = 0,

		/**
		 * Pipeline was optimized into weston_color_transform,
		 * 3D LUT not used.
		 */
		CMLCMS_TRANSFORM_OPTIMIZED,

		/** The transformation uses 3D LUT. */
		CMLCMS_TRANSFORM_3DLUT,
	} status;
};

static inline struct cmlcms_color_transform *
get_xform(struct weston_color_transform *xform_base)
{
	return container_of(xform_base, struct cmlcms_color_transform, base);
}

struct cmlcms_color_transform *
cmlcms_color_transform_get(struct weston_color_manager_lcms *cm,
			   const struct cmlcms_color_transform_search_param *param);

void
cmlcms_color_transform_destroy(struct cmlcms_color_transform *xform);

char *
cmlcms_color_transform_search_param_string(const struct cmlcms_color_transform_search_param *search_key);

struct cmlcms_color_profile *
ref_cprof(struct cmlcms_color_profile *cprof);

void
unref_cprof(struct cmlcms_color_profile *cprof);

bool
cmlcms_create_stock_profile(struct weston_color_manager_lcms *cm);

void
cmlcms_color_profile_destroy(struct cmlcms_color_profile *cprof);

char *
cmlcms_color_profile_print(const struct cmlcms_color_profile *cprof);

bool
retrieve_eotf_and_output_inv_eotf(cmsContext lcms_ctx,
				  cmsHPROFILE hProfile,
				  cmsToneCurve *output_eotf[3],
				  cmsToneCurve *output_inv_eotf_vcgt[3],
				  cmsToneCurve *vcgt[3],
				  unsigned int num_points);

unsigned int
cmlcms_reasonable_1D_points(void);

cmsToneCurve *
lcmsJoinToneCurve(cmsContext context_id, const cmsToneCurve *X,
		  const cmsToneCurve *Y, unsigned int resulting_points);

#endif /* WESTON_COLOR_LCMS_H */
