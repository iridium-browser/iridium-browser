/*
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

#include <libweston/libweston.h>

#include "color.h"
#include "shared/helpers.h"
#include "shared/string-helpers.h"

struct weston_color_manager_noop {
	struct weston_color_manager base;
};

static bool
check_output_eotf_mode(struct weston_output *output)
{
	if (output->eotf_mode == WESTON_EOTF_MODE_SDR)
		return true;

	weston_log("Error: color manager no-op does not support EOTF mode %s of output %s.\n",
		   weston_eotf_mode_to_str(output->eotf_mode),
		   output->name);
	return false;
}

static struct weston_color_manager_noop *
get_cmnoop(struct weston_color_manager *cm_base)
{
	return container_of(cm_base, struct weston_color_manager_noop, base);
}

static void
cmnoop_destroy_color_profile(struct weston_color_profile *cprof)
{
	/* Never called, as never creates an actual color profile. */
}

static bool
cmnoop_get_color_profile_from_icc(struct weston_color_manager *cm,
				  const void *icc_data,
				  size_t icc_len,
				  const char *name_part,
				  struct weston_color_profile **cprof_out,
				  char **errmsg)
{
	str_printf(errmsg, "ICC profiles are unsupported.");
	return false;
}

static void
cmnoop_destroy_color_transform(struct weston_color_transform *xform)
{
	/* Never called, as never creates an actual color transform. */
}

static bool
cmnoop_get_surface_color_transform(struct weston_color_manager *cm_base,
				   struct weston_surface *surface,
				   struct weston_output *output,
				   struct weston_surface_color_transform *surf_xform)
{
	/* TODO: Assert surface has no colorspace set */
	assert(output->color_profile == NULL);

	if (!check_output_eotf_mode(output))
		return false;

	/* Identity transform */
	surf_xform->transform = NULL;
	surf_xform->identity_pipeline = true;

	return true;
}

static struct weston_output_color_outcome *
cmnoop_create_output_color_outcome(struct weston_color_manager *cm_base,
				   struct weston_output *output)
{
	struct weston_output_color_outcome *co;

	assert(output->color_profile == NULL);

	if (!check_output_eotf_mode(output))
		return NULL;

	co = zalloc(sizeof *co);
	if (!co)
		return NULL;

	/* Identity transform on everything */
	co->from_blend_to_output = NULL;
	co->from_sRGB_to_blend = NULL;
	co->from_sRGB_to_output = NULL;

	co->hdr_meta.group_mask = 0;

	return co;
}

static bool
cmnoop_init(struct weston_color_manager *cm_base)
{
	/* No renderer requirements to check. */
	/* Nothing to initialize. */
	return true;
}

static void
cmnoop_destroy(struct weston_color_manager *cm_base)
{
	struct weston_color_manager_noop *cmnoop = get_cmnoop(cm_base);

	free(cmnoop);
}

struct weston_color_manager *
weston_color_manager_noop_create(struct weston_compositor *compositor)
{
	struct weston_color_manager_noop *cm;

	cm = zalloc(sizeof *cm);
	if (!cm)
		return NULL;

	cm->base.name = "no-op";
	cm->base.compositor = compositor;
	cm->base.supports_client_protocol = false;
	cm->base.init = cmnoop_init;
	cm->base.destroy = cmnoop_destroy;
	cm->base.destroy_color_profile = cmnoop_destroy_color_profile;
	cm->base.get_color_profile_from_icc = cmnoop_get_color_profile_from_icc;
	cm->base.destroy_color_transform = cmnoop_destroy_color_transform;
	cm->base.get_surface_color_transform = cmnoop_get_surface_color_transform;
	cm->base.create_output_color_outcome = cmnoop_create_output_color_outcome;

	return &cm->base;
}
