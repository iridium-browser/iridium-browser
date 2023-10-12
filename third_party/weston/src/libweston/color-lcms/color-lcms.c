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

#include "config.h"

#include <assert.h>
#include <string.h>
#include <libweston/libweston.h>

#include "color.h"
#include "color-lcms.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"

const char *
cmlcms_category_name(enum cmlcms_category cat)
{
	static const char *const category_names[] = {
		[CMLCMS_CATEGORY_INPUT_TO_BLEND] = "input-to-blend",
		[CMLCMS_CATEGORY_BLEND_TO_OUTPUT] = "blend-to-output",
		[CMLCMS_CATEGORY_INPUT_TO_OUTPUT] = "input-to-output",
	};

	if (cat < 0 || cat >= ARRAY_LENGTH(category_names))
		return "[illegal category value]";

	return category_names[cat] ?: "[undocumented category value]";
}

static cmsUInt32Number
cmlcms_get_render_intent(enum cmlcms_category cat,
			 struct weston_surface *surface,
			 struct weston_output *output)
{
	/*
	 * TODO: Take into account client provided content profile,
	 * output profile, and the category of the wanted color
	 * transformation.
	 */
	cmsUInt32Number intent = INTENT_RELATIVE_COLORIMETRIC;
	return intent;
}

static struct cmlcms_color_profile *
get_cprof_or_stock_sRGB(struct weston_color_manager_lcms *cm,
			struct weston_color_profile *cprof_base)
{
	if (cprof_base)
		return get_cprof(cprof_base);
	else
		return cm->sRGB_profile;
}

static void
cmlcms_destroy_color_transform(struct weston_color_transform *xform_base)
{
	struct cmlcms_color_transform *xform = get_xform(xform_base);

	cmlcms_color_transform_destroy(xform);
}

static bool
cmlcms_get_surface_color_transform(struct weston_color_manager *cm_base,
				   struct weston_surface *surface,
				   struct weston_output *output,
				   struct weston_surface_color_transform *surf_xform)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cm_base);
	struct cmlcms_color_transform *xform;

	/* TODO: take weston_output::eotf_mode into account */

	struct cmlcms_color_transform_search_param param = {
		.category = CMLCMS_CATEGORY_INPUT_TO_BLEND,
		.input_profile = get_cprof_or_stock_sRGB(cm, NULL /* TODO: surface->color_profile */),
		.output_profile = get_cprof_or_stock_sRGB(cm, output->color_profile),
	};
	param.intent_output = cmlcms_get_render_intent(param.category,
						       surface, output);

	xform = cmlcms_color_transform_get(cm, &param);
	if (!xform)
		return false;

	surf_xform->transform = &xform->base;
	/*
	 * When we introduce LCMS plug-in we can precisely answer this question
	 * by examining the color pipeline using precision parameters. For now
	 * we just compare if it is same pointer or not.
	 */
	if (xform->search_key.input_profile == xform->search_key.output_profile)
		surf_xform->identity_pipeline = true;
	else
		surf_xform->identity_pipeline = false;

	return true;
}

static bool
cmlcms_get_blend_to_output_color_transform(struct weston_color_manager_lcms *cm,
					   struct weston_output *output,
					   struct weston_color_transform **xform_out)
{
	struct cmlcms_color_transform *xform;

	/* TODO: take weston_output::eotf_mode into account */

	struct cmlcms_color_transform_search_param param = {
		.category = CMLCMS_CATEGORY_BLEND_TO_OUTPUT,
		.input_profile = NULL,
		.output_profile = get_cprof_or_stock_sRGB(cm, output->color_profile),
	};
	param.intent_output = cmlcms_get_render_intent(param.category,
						       NULL, output);

	xform = cmlcms_color_transform_get(cm, &param);
	if (!xform)
		return false;

	*xform_out = &xform->base;
	return true;
}

static bool
cmlcms_get_sRGB_to_output_color_transform(struct weston_color_manager_lcms *cm,
					  struct weston_output *output,
					  struct weston_color_transform **xform_out)
{
	struct cmlcms_color_transform *xform;

	/* TODO: take weston_output::eotf_mode into account */

	struct cmlcms_color_transform_search_param param = {
		.category = CMLCMS_CATEGORY_INPUT_TO_OUTPUT,
		.input_profile = cm->sRGB_profile,
		.output_profile = get_cprof_or_stock_sRGB(cm, output->color_profile),
	};
	param.intent_output = cmlcms_get_render_intent(param.category,
						       NULL, output);

	/*
	 * Create a color transformation when output profile is not stock
	 * sRGB profile.
	 */
	if (param.output_profile != cm->sRGB_profile) {
		xform = cmlcms_color_transform_get(cm, &param);
		if (!xform)
			return false;
		*xform_out = &xform->base;
	} else {
		*xform_out = NULL; /* Identity transform */
	}

	return true;
}

static bool
cmlcms_get_sRGB_to_blend_color_transform(struct weston_color_manager_lcms *cm,
					 struct weston_output *output,
					 struct weston_color_transform **xform_out)
{
	struct cmlcms_color_transform *xform;

	/* TODO: take weston_output::eotf_mode into account */

	struct cmlcms_color_transform_search_param param = {
		.category = CMLCMS_CATEGORY_INPUT_TO_BLEND,
		.input_profile = cm->sRGB_profile,
		.output_profile = get_cprof_or_stock_sRGB(cm, output->color_profile),
	};
	param.intent_output = cmlcms_get_render_intent(param.category,
						       NULL, output);

	xform = cmlcms_color_transform_get(cm, &param);
	if (!xform)
		return false;

	*xform_out = &xform->base;
	return true;
}

static float
meta_clamp(float value, const char *valname, float min, float max,
	   struct weston_output *output)
{
	float ret = value;

	/* Paranoia against NaN */
	if (!(ret >= min))
		ret = min;

	if (!(ret <= max))
		ret = max;

	if (ret != value) {
		weston_log("output '%s' clamping %s value from %f to %f.\n",
			   output->name, valname, value, ret);
	}

	return ret;
}

static bool
cmlcms_get_hdr_meta(struct weston_output *output,
		    struct weston_hdr_metadata_type1 *hdr_meta)
{
	const struct weston_color_characteristics *cc;

	hdr_meta->group_mask = 0;

	/* Only SMPTE ST 2084 mode uses HDR Static Metadata Type 1 */
	if (weston_output_get_eotf_mode(output) != WESTON_EOTF_MODE_ST2084)
		return true;

	/* ICC profile overrides color characteristics */
	if (output->color_profile) {
		/*
		 * TODO: extract characteristics from profile?
		 * Get dynamic range from weston_color_characteristics?
		 */

		return true;
	}

	cc = weston_output_get_color_characteristics(output);

	/* Target content chromaticity */
	if (cc->group_mask & WESTON_COLOR_CHARACTERISTICS_GROUP_PRIMARIES) {
		unsigned i;

		for (i = 0; i < 3; i++) {
			hdr_meta->primary[i].x = meta_clamp(cc->primary[i].x,
							    "primary", 0.0, 1.0,
							    output);
			hdr_meta->primary[i].y = meta_clamp(cc->primary[i].y,
							    "primary", 0.0, 1.0,
							    output);
		}
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_PRIMARIES;
	}

	/* Target content white point */
	if (cc->group_mask & WESTON_COLOR_CHARACTERISTICS_GROUP_WHITE) {
		hdr_meta->white.x = meta_clamp(cc->white.x, "white",
					       0.0, 1.0, output);
		hdr_meta->white.y = meta_clamp(cc->white.y, "white",
					       0.0, 1.0, output);
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_WHITE;
	}

	/* Target content peak and max mastering luminance */
	if (cc->group_mask & WESTON_COLOR_CHARACTERISTICS_GROUP_MAXL) {
		hdr_meta->maxDML = meta_clamp(cc->max_luminance, "maxDML",
					      1.0, 65535.0, output);
		hdr_meta->maxCLL = meta_clamp(cc->max_luminance, "maxCLL",
					      1.0, 65535.0, output);
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_MAXDML;
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_MAXCLL;
	}

	/* Target content min mastering luminance */
	if (cc->group_mask & WESTON_COLOR_CHARACTERISTICS_GROUP_MINL) {
		hdr_meta->minDML = meta_clamp(cc->min_luminance, "minDML",
					      0.0001, 6.5535, output);
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_MINDML;
	}

	/* Target content max frame-average luminance */
	if (cc->group_mask & WESTON_COLOR_CHARACTERISTICS_GROUP_MAXFALL) {
		hdr_meta->maxFALL = meta_clamp(cc->maxFALL, "maxFALL",
					       1.0, 65535.0, output);
		hdr_meta->group_mask |= WESTON_HDR_METADATA_TYPE1_GROUP_MAXFALL;
	}

	return true;
}

static struct weston_output_color_outcome *
cmlcms_create_output_color_outcome(struct weston_color_manager *cm_base,
				   struct weston_output *output)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cm_base);
	struct weston_output_color_outcome *co;

	co = zalloc(sizeof *co);
	if (!co)
		return NULL;

	if (!cmlcms_get_hdr_meta(output, &co->hdr_meta))
		goto out_fail;

	/*
	 * TODO: if output->color_profile is NULL, maybe manufacture a
	 * profile from weston_color_characteristics if it has enough
	 * information?
	 * Or let the frontend decide to call a "create a profile from
	 * characteristics" API?
	 */

	/* TODO: take container color space into account */

	if (!cmlcms_get_blend_to_output_color_transform(cm, output,
							&co->from_blend_to_output))
		goto out_fail;

	if (!cmlcms_get_sRGB_to_blend_color_transform(cm, output,
						      &co->from_sRGB_to_blend))
		goto out_fail;

	if (!cmlcms_get_sRGB_to_output_color_transform(cm, output,
						       &co->from_sRGB_to_output))
		goto out_fail;

	return co;

out_fail:
	weston_output_color_outcome_destroy(&co);
	return NULL;
}

static void
lcms_error_logger(cmsContext context_id,
		  cmsUInt32Number error_code,
		  const char *text)
{
	weston_log("LittleCMS error: %s\n", text);
}

static bool
cmlcms_init(struct weston_color_manager *cm_base)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cm_base);

	if (!(cm->base.compositor->capabilities & WESTON_CAP_COLOR_OPS)) {
		weston_log("color-lcms: error: color operations capability missing. Is GL-renderer not in use?\n");
		return false;
	}

	cm->lcms_ctx = cmsCreateContext(NULL, cm);
	if (!cm->lcms_ctx) {
		weston_log("color-lcms error: creating LittCMS context failed.\n");
		return false;
	}

	cmsSetLogErrorHandlerTHR(cm->lcms_ctx, lcms_error_logger);

	if (!cmlcms_create_stock_profile(cm)) {
		weston_log("color-lcms: error: cmlcms_create_stock_profile failed\n");
		return false;
	}
	weston_log("LittleCMS %d initialized.\n", cmsGetEncodedCMMversion());

	return true;
}

static void
cmlcms_destroy(struct weston_color_manager *cm_base)
{
	struct weston_color_manager_lcms *cm = get_cmlcms(cm_base);

	if (cm->sRGB_profile)
		cmlcms_color_profile_destroy(cm->sRGB_profile);
	assert(wl_list_empty(&cm->color_transform_list));
	assert(wl_list_empty(&cm->color_profile_list));

	cmsDeleteContext(cm->lcms_ctx);

	weston_log_scope_destroy(cm->transforms_scope);
	weston_log_scope_destroy(cm->optimizer_scope);
	weston_log_scope_destroy(cm->profiles_scope);

	free(cm);
}

static void
transforms_scope_new_sub(struct weston_log_subscription *subs, void *data)
{
	struct weston_color_manager_lcms *cm = data;
	struct cmlcms_color_transform *xform;
	char *str;

	if (wl_list_empty(&cm->color_transform_list))
		return;

	weston_log_subscription_printf(subs, "Existent:\n");
	wl_list_for_each(xform, &cm->color_transform_list, link) {
		weston_log_subscription_printf(subs, "Color transformation %p:\n", xform);

		str = cmlcms_color_transform_search_param_string(&xform->search_key);
		weston_log_subscription_printf(subs, "%s", str);
		free(str);

		str = weston_color_transform_string(&xform->base);
		weston_log_subscription_printf(subs, "  %s", str);
		free(str);
	}
}

static void
profiles_scope_new_sub(struct weston_log_subscription *subs, void *data)
{
	struct weston_color_manager_lcms *cm = data;
	struct cmlcms_color_profile *cprof;
	char *str;

	if (wl_list_empty(&cm->color_profile_list))
		return;

	weston_log_subscription_printf(subs, "Existent:\n");
	wl_list_for_each(cprof, &cm->color_profile_list, link) {
		weston_log_subscription_printf(subs, "Color profile %p:\n", cprof);

		str = cmlcms_color_profile_print(cprof);
		weston_log_subscription_printf(subs, "%s", str);
		free(str);
	}
}

WL_EXPORT struct weston_color_manager *
weston_color_manager_create(struct weston_compositor *compositor)
{
	struct weston_color_manager_lcms *cm;

	cm = zalloc(sizeof *cm);
	if (!cm)
		return NULL;

	cm->base.name = "work-in-progress";
	cm->base.compositor = compositor;
	cm->base.supports_client_protocol = true;
	cm->base.init = cmlcms_init;
	cm->base.destroy = cmlcms_destroy;
	cm->base.destroy_color_profile = cmlcms_destroy_color_profile;
	cm->base.get_color_profile_from_icc = cmlcms_get_color_profile_from_icc;
	cm->base.destroy_color_transform = cmlcms_destroy_color_transform;
	cm->base.get_surface_color_transform = cmlcms_get_surface_color_transform;
	cm->base.create_output_color_outcome = cmlcms_create_output_color_outcome;

	wl_list_init(&cm->color_transform_list);
	wl_list_init(&cm->color_profile_list);

	cm->transforms_scope =
		weston_compositor_add_log_scope(compositor, "color-lcms-transformations",
						"Color transformation creation and destruction.\n",
						transforms_scope_new_sub, NULL, cm);
	cm->optimizer_scope =
		weston_compositor_add_log_scope(compositor, "color-lcms-optimizer",
						"Color transformation pipeline optimizer. It's best " \
						"used together with the color-lcms-transformations " \
						"log scope.\n", NULL, NULL, NULL);
	cm->profiles_scope =
		weston_compositor_add_log_scope(compositor, "color-lcms-profiles",
						"Color profile creation and destruction.\n",
						profiles_scope_new_sub, NULL, cm);

	if (!cm->profiles_scope || !cm->transforms_scope || !cm->optimizer_scope)
		goto err;

	return &cm->base;

err:
	weston_log_scope_destroy(cm->transforms_scope);
	weston_log_scope_destroy(cm->optimizer_scope);
	weston_log_scope_destroy(cm->profiles_scope);
	free(cm);
	return NULL;
}
