/*
 * Copyright 2019 Sebastian Wick
 * Copyright 2021 Collabora, Ltd.
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
#include <stdio.h>
#include <string.h>
#include <libweston/libweston.h>

#include "color.h"
#include "color-lcms.h"
#include "shared/helpers.h"
#include "shared/string-helpers.h"
#include "shared/xalloc.h"

struct xyz_arr_flt {
	float v[3];
};

static double
xyz_dot_prod(const struct xyz_arr_flt a, const struct xyz_arr_flt b)
{
	return (double)a.v[0] * b.v[0] +
	       (double)a.v[1] * b.v[1] +
	       (double)a.v[2] * b.v[2];
}

/**
 * Graeme sketched a linearization method there:
 * https://lists.freedesktop.org/archives/wayland-devel/2019-March/040171.html
 */
static bool
build_eotf_from_clut_profile(cmsContext lcms_ctx,
			     cmsHPROFILE profile,
			     cmsToneCurve *output_eotf[3],
			     int num_points)
{
	int ch, point;
	float *curve_array[3];
	float *red = NULL;
	cmsHPROFILE xyz_profile = NULL;
	cmsHTRANSFORM transform_rgb_to_xyz = NULL;
	bool ret = false;
	const float div = num_points - 1;

	red = malloc(sizeof(float) * num_points * 3);
	if (!red)
		goto release;

	curve_array[0] = red;
	curve_array[1] = red + num_points;
	curve_array[2] = red + 2 * num_points;

	xyz_profile = cmsCreateXYZProfileTHR(lcms_ctx);
	if (!xyz_profile)
		goto release;

	transform_rgb_to_xyz = cmsCreateTransformTHR(lcms_ctx, profile,
						     TYPE_RGB_FLT, xyz_profile,
						     TYPE_XYZ_FLT,
						     INTENT_ABSOLUTE_COLORIMETRIC,
						     0);
	if (!transform_rgb_to_xyz)
		goto release;

	for (ch = 0; ch < 3; ch++) {
		struct xyz_arr_flt prim_xyz_max;
		struct xyz_arr_flt prim_xyz;
		double xyz_square_magnitude;
		float rgb[3] = { 0.0f, 0.0f, 0.0f };

		rgb[ch] = 1.0f;
		cmsDoTransform(transform_rgb_to_xyz, rgb, prim_xyz_max.v, 1);

		/**
		 * Calculate xyz square of magnitude uses single channel 100% and
		 * others are zero.
		 */
		xyz_square_magnitude = xyz_dot_prod(prim_xyz_max, prim_xyz_max);
		/**
		 * Build rgb tone curves
		 */
		for (point = 0; point < num_points; point++) {
			rgb[ch] = (float)point / div;
			cmsDoTransform(transform_rgb_to_xyz, rgb, prim_xyz.v, 1);
			curve_array[ch][point] = xyz_dot_prod(prim_xyz,
							      prim_xyz_max) /
						 xyz_square_magnitude;
		}

		/**
		 * Create LCMS object of rgb tone curves and validate whether
		 * monotonic
		 */
		output_eotf[ch] = cmsBuildTabulatedToneCurveFloat(lcms_ctx,
								  num_points,
								  curve_array[ch]);
		if (!output_eotf[ch])
			goto release;
		if (!cmsIsToneCurveMonotonic(output_eotf[ch])) {
			/**
			 * It is interesting to see how this profile was created.
			 * We assume that such a curve could not be used for linearization
			 * of arbitrary profile.
			 */
			goto release;
		}
	}
	ret = true;

release:
	if (transform_rgb_to_xyz)
		cmsDeleteTransform(transform_rgb_to_xyz);
	if (xyz_profile)
		cmsCloseProfile(xyz_profile);
	free(red);
	if (ret == false)
		cmsFreeToneCurveTriple(output_eotf);

	return ret;
}

/**
 * Concatenation of two monotonic tone curves.
 * LCMS  API cmsJoinToneCurve does y = Y^-1(X(t)),
 * but want to have y = Y^(X(t))
 */
cmsToneCurve *
lcmsJoinToneCurve(cmsContext context_id, const cmsToneCurve *X,
		  const cmsToneCurve *Y, unsigned int resulting_points)
{
	cmsToneCurve *out = NULL;
	float t, x;
	float *res = NULL;
	unsigned int i;

	res = zalloc(resulting_points * sizeof(float));
	if (res == NULL)
		goto error;

	for (i = 0; i < resulting_points; i++) {
		t = (float)i / (resulting_points - 1);
		x = cmsEvalToneCurveFloat(X, t);
		res[i] = cmsEvalToneCurveFloat(Y, x);
	}

	out = cmsBuildTabulatedToneCurveFloat(context_id, resulting_points, res);

error:
	if (res != NULL)
		free(res);

	return out;
}
/**
 * Extract EOTF from matrix-shaper and cLUT profiles,
 * then invert and concatenate with 'vcgt' curve if it
 * is available.
 */
bool
retrieve_eotf_and_output_inv_eotf(cmsContext lcms_ctx,
				  cmsHPROFILE hProfile,
				  cmsToneCurve *output_eotf[3],
				  cmsToneCurve *output_inv_eotf_vcgt[3],
				  cmsToneCurve *vcgt[3],
				  unsigned int num_points)
{
	cmsToneCurve *curve = NULL;
	const cmsToneCurve * const *vcgt_curves;
	unsigned i;
	cmsTagSignature tags[] = {
			cmsSigRedTRCTag, cmsSigGreenTRCTag, cmsSigBlueTRCTag
	};

	if (cmsIsMatrixShaper(hProfile)) {
		/**
		 * Optimization for matrix-shaper profile
		 * May have 1DLUT->3x3->3x3->1DLUT, 1DLUT->3x3->1DLUT
		 */
		for (i = 0 ; i < 3; i++) {
			curve = cmsReadTag(hProfile, tags[i]);
			if (!curve)
				goto fail;
			output_eotf[i] = cmsDupToneCurve(curve);
			if (!output_eotf[i])
				goto fail;
		}
	} else {
		/**
		 * Linearization of cLUT profile may have 1DLUT->3DLUT->1DLUT,
		 * 1DLUT->3DLUT, 3DLUT
		 */
		if (!build_eotf_from_clut_profile(lcms_ctx, hProfile,
						 output_eotf, num_points))
			goto fail;
	}
	/**
	 * If the caller looking for eotf only then return early.
	 * It could be used for input profile when identity case: EOTF + INV_EOTF
	 * in pipeline only.
	 */
	if (output_inv_eotf_vcgt == NULL)
		return true;

	for (i = 0; i < 3; i++) {
		curve = cmsReverseToneCurve(output_eotf[i]);
		if (!curve)
			goto fail;
		output_inv_eotf_vcgt[i] = curve;
	}
	vcgt_curves = cmsReadTag(hProfile, cmsSigVcgtTag);
	if (vcgt_curves && vcgt_curves[0] && vcgt_curves[1] && vcgt_curves[2]) {
		for (i = 0; i < 3; i++) {
			curve = lcmsJoinToneCurve(lcms_ctx,
						  output_inv_eotf_vcgt[i],
						  vcgt_curves[i], num_points);
			if (!curve)
				goto fail;
			cmsFreeToneCurve(output_inv_eotf_vcgt[i]);
			output_inv_eotf_vcgt[i] = curve;
			if (vcgt)
				vcgt[i] = cmsDupToneCurve(vcgt_curves[i]);
		}
	}
	return true;

fail:
	cmsFreeToneCurveTriple(output_eotf);
	cmsFreeToneCurveTriple(output_inv_eotf_vcgt);
	return false;
}

/* FIXME: sync with spec! */
static bool
validate_icc_profile(cmsHPROFILE profile, char **errmsg)
{
	cmsColorSpaceSignature cs = cmsGetColorSpace(profile);
	uint32_t nr_channels = cmsChannelsOf(cs);
	uint8_t version = cmsGetEncodedICCversion(profile) >> 24;

	if (version != 2 && version != 4) {
		str_printf(errmsg,
			   "ICC profile major version %d is unsupported, should be 2 or 4.",
			   version);
		return false;
	}

	if (nr_channels != 3) {
		str_printf(errmsg,
			   "ICC profile must contain 3 channels for the color space, not %u.",
			   nr_channels);
		return false;
	}

	if (cmsGetDeviceClass(profile) != cmsSigDisplayClass) {
		str_printf(errmsg, "ICC profile is required to be of Display device class, but it is not.");
		return false;
	}

	return true;
}

static struct cmlcms_color_profile *
cmlcms_find_color_profile_by_md5(const struct weston_color_manager_lcms *cm,
				 const struct cmlcms_md5_sum *md5sum)
{
	struct cmlcms_color_profile *cprof;

	wl_list_for_each(cprof, &cm->color_profile_list, link) {
		if (memcmp(cprof->md5sum.bytes,
			   md5sum->bytes, sizeof(md5sum->bytes)) == 0)
			return cprof;
	}

	return NULL;
}

char *
cmlcms_color_profile_print(const struct cmlcms_color_profile *cprof)
{
	char *str;

	str_printf(&str, "  description: %s\n", cprof->base.description);
	abort_oom_if_null(str);

	return str;
}

static struct cmlcms_color_profile *
cmlcms_color_profile_create(struct weston_color_manager_lcms *cm,
			    cmsHPROFILE profile,
			    char *desc,
			    char **errmsg)
{
	struct cmlcms_color_profile *cprof;
	char *str;

	cprof = zalloc(sizeof *cprof);
	if (!cprof)
		return NULL;

	weston_color_profile_init(&cprof->base, &cm->base);
	cprof->base.description = desc;
	cprof->profile = profile;
	cmsGetHeaderProfileID(profile, cprof->md5sum.bytes);
	wl_list_insert(&cm->color_profile_list, &cprof->link);

	weston_log_scope_printf(cm->profiles_scope,
				"New color profile: %p\n", cprof);

	str = cmlcms_color_profile_print(cprof);
	weston_log_scope_printf(cm->profiles_scope, "%s", str);
	free(str);

	return cprof;
}

void
cmlcms_color_profile_destroy(struct cmlcms_color_profile *cprof)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cprof->base.cm);

	wl_list_remove(&cprof->link);
	cmsFreeToneCurveTriple(cprof->vcgt);
	cmsFreeToneCurveTriple(cprof->eotf);
	cmsFreeToneCurveTriple(cprof->output_inv_eotf_vcgt);
	cmsCloseProfile(cprof->profile);

	weston_log_scope_printf(cm->profiles_scope, "Destroyed color profile %p. " \
				"Description: %s\n", cprof, cprof->base.description);

	free(cprof->base.description);
	free(cprof);
}

struct cmlcms_color_profile *
ref_cprof(struct cmlcms_color_profile *cprof)
{
	if (!cprof)
		return NULL;

	weston_color_profile_ref(&cprof->base);
	return  cprof;
}

void
unref_cprof(struct cmlcms_color_profile *cprof)
{
	if (!cprof)
		return;

	weston_color_profile_unref(&cprof->base);
}

static char *
make_icc_file_description(cmsHPROFILE profile,
			  const struct cmlcms_md5_sum *md5sum,
			  const char *name_part)
{
	char md5sum_str[sizeof(md5sum->bytes) * 2 + 1];
	char *desc;
	size_t i;

	for (i = 0; i < sizeof(md5sum->bytes); i++) {
		snprintf(md5sum_str + 2 * i, sizeof(md5sum_str) - 2 * i,
			 "%02x", md5sum->bytes[i]);
	}

	str_printf(&desc, "ICCv%.1f %s %s", cmsGetProfileVersion(profile),
		   name_part, md5sum_str);

	return desc;
}

/**
 *
 * Build stock profile which available for clients unaware of color management
 */
bool
cmlcms_create_stock_profile(struct weston_color_manager_lcms *cm)
{
	cmsHPROFILE profile;
	struct cmlcms_md5_sum md5sum;
	char *desc = NULL;

	profile = cmsCreate_sRGBProfileTHR(cm->lcms_ctx);
	if (!profile) {
		weston_log("color-lcms: error: cmsCreate_sRGBProfileTHR failed\n");
		return false;
	}
	if (!cmsMD5computeID(profile)) {
		weston_log("Failed to compute MD5 for ICC profile\n");
		goto err_close;
	}

	cmsGetHeaderProfileID(profile, md5sum.bytes);
	desc = make_icc_file_description(profile, &md5sum, "sRGB stock");
	if (!desc)
		goto err_close;

	cm->sRGB_profile = cmlcms_color_profile_create(cm, profile, desc, NULL);
	if (!cm->sRGB_profile)
		goto err_close;

	if (!retrieve_eotf_and_output_inv_eotf(cm->lcms_ctx,
					       cm->sRGB_profile->profile,
					       cm->sRGB_profile->eotf,
					       cm->sRGB_profile->output_inv_eotf_vcgt,
					       cm->sRGB_profile->vcgt,
					       cmlcms_reasonable_1D_points()))
		goto err_close;

	return true;

err_close:
	free(desc);
	cmsCloseProfile(profile);
	return false;
}

bool
cmlcms_get_color_profile_from_icc(struct weston_color_manager *cm_base,
				  const void *icc_data,
				  size_t icc_len,
				  const char *name_part,
				  struct weston_color_profile **cprof_out,
				  char **errmsg)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cm_base);
	cmsHPROFILE profile;
	struct cmlcms_md5_sum md5sum;
	struct cmlcms_color_profile *cprof;
	char *desc = NULL;

	if (!icc_data || icc_len < 1) {
		str_printf(errmsg, "No ICC data.");
		return false;
	}
	if (icc_len >= UINT32_MAX) {
		str_printf(errmsg, "Too much ICC data.");
		return false;
	}

	profile = cmsOpenProfileFromMemTHR(cm->lcms_ctx, icc_data, icc_len);
	if (!profile) {
		str_printf(errmsg, "ICC data not understood.");
		return false;
	}

	if (!validate_icc_profile(profile, errmsg))
		goto err_close;

	if (!cmsMD5computeID(profile)) {
		str_printf(errmsg, "Failed to compute MD5 for ICC profile.");
		goto err_close;
	}

	cmsGetHeaderProfileID(profile, md5sum.bytes);
	cprof = cmlcms_find_color_profile_by_md5(cm, &md5sum);
	if (cprof) {
		*cprof_out = weston_color_profile_ref(&cprof->base);
		cmsCloseProfile(profile);
		return true;
	}

	desc = make_icc_file_description(profile, &md5sum, name_part);
	if (!desc)
		goto err_close;

	cprof = cmlcms_color_profile_create(cm, profile, desc, errmsg);
	if (!cprof)
		goto err_close;

	*cprof_out = &cprof->base;
	return true;

err_close:
	free(desc);
	cmsCloseProfile(profile);
	return false;
}

void
cmlcms_destroy_color_profile(struct weston_color_profile *cprof_base)
{
	struct cmlcms_color_profile *cprof = get_cprof(cprof_base);

	cmlcms_color_profile_destroy(cprof);
}
