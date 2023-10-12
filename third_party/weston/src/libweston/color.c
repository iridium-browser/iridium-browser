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

#include <libweston/libweston.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include "color.h"
#include "libweston-internal.h"
#include <libweston/weston-log.h>
#include "shared/xalloc.h"

/**
 * Increase reference count of the color profile object
 *
 * \param cprof The color profile. NULL is accepted too.
 * \return cprof.
 */
WL_EXPORT struct weston_color_profile *
weston_color_profile_ref(struct weston_color_profile *cprof)
{
	/* NULL is a valid color space: sRGB */
	if (!cprof)
		return NULL;

	assert(cprof->ref_count > 0);
	cprof->ref_count++;
	return cprof;
}

/**
 * Decrease reference count and potentially destroy the color profile object
 *
 * \param cprof The color profile. NULL is accepted too.
 */
WL_EXPORT void
weston_color_profile_unref(struct weston_color_profile *cprof)
{
	if (!cprof)
		return;

	assert(cprof->ref_count > 0);
	if (--cprof->ref_count > 0)
		return;

	cprof->cm->destroy_color_profile(cprof);
}

/**
 * Get color profile description
 *
 * A description of the profile is meant for human readable logs.
 *
 * \param cprof The color profile, NULL is accepted too.
 * \returns The color profile description, valid as long as the
 * color profile itself is.
 */
WL_EXPORT const char *
weston_color_profile_get_description(struct weston_color_profile *cprof)
{
	if (cprof)
		return cprof->description;
	else
		return "built-in default sRGB SDR profile";
}

/**
 * Initializes a newly allocated color profile object
 *
 * This is used only by color managers. They sub-class weston_color_profile.
 *
 * The reference count starts at 1.
 *
 * To destroy a weston_color_profile, use weston_color_profile_unref().
 */
WL_EXPORT void
weston_color_profile_init(struct weston_color_profile *cprof,
			  struct weston_color_manager *cm)
{
	cprof->cm = cm;
	cprof->ref_count = 1;
}

/**
 * Increase reference count of the color transform object
 *
 * \param xform The color transform. NULL is accepted too.
 * \return xform.
 */
WL_EXPORT struct weston_color_transform *
weston_color_transform_ref(struct weston_color_transform *xform)
{
	/* NULL is a valid color transform: identity */
	if (!xform)
		return NULL;

	assert(xform->ref_count > 0);
	xform->ref_count++;
	return xform;
}

/**
 * Decrease and potentially destroy the color transform object
 *
 * \param xform The color transform. NULL is accepted too.
 */
WL_EXPORT void
weston_color_transform_unref(struct weston_color_transform *xform)
{
	if (!xform)
		return;

	assert(xform->ref_count > 0);
	if (--xform->ref_count > 0)
		return;

	wl_signal_emit(&xform->destroy_signal, xform);
	xform->cm->destroy_color_transform(xform);
}

/**
 * Initializes a newly allocated color transform object
 *
 * This is used only by color managers. They sub-class weston_color_transform.
 *
 * The reference count starts at 1.
 *
 * To destroy a weston_color_transform, use weston_color_transfor_unref().
 */
WL_EXPORT void
weston_color_transform_init(struct weston_color_transform *xform,
			    struct weston_color_manager *cm)
{
	xform->cm = cm;
	xform->ref_count = 1;
	wl_signal_init(&xform->destroy_signal);
}

static const char *
curve_type_to_str(enum weston_color_curve_type curve_type)
{
	switch (curve_type) {
	case WESTON_COLOR_CURVE_TYPE_IDENTITY:
		return "identity";
	case WESTON_COLOR_CURVE_TYPE_LUT_3x1D:
		return "3x1D LUT";
	}
	return "???";
}

static const char *
mapping_type_to_str(enum weston_color_mapping_type mapping_type)
{
	switch (mapping_type) {
	case WESTON_COLOR_MAPPING_TYPE_IDENTITY:
		return "identity";
	case WESTON_COLOR_MAPPING_TYPE_3D_LUT:
		return "3D LUT";
	case WESTON_COLOR_MAPPING_TYPE_MATRIX:
		return "matrix";
	}
	return "???";
}

/**
 * Print the color transform pipeline to a string
 *
 * \param xform The color transform.
 * \return The string in which the pipeline is printed.
 */
WL_EXPORT char *
weston_color_transform_string(const struct weston_color_transform *xform)
{
	enum weston_color_mapping_type mapping_type = xform->mapping.type;
	enum weston_color_curve_type pre_type = xform->pre_curve.type;
	enum weston_color_curve_type post_type = xform->post_curve.type;
	const char *empty = "";
	const char *sep = empty;
	FILE *fp;
	char *str = NULL;
	size_t size = 0;

	fp = open_memstream(&str, &size);
	abort_oom_if_null(fp);

	fprintf(fp, "pipeline: ");

	if (pre_type != WESTON_COLOR_CURVE_TYPE_IDENTITY) {
		fprintf(fp, "%spre %s", sep, curve_type_to_str(pre_type));
		if (pre_type == WESTON_COLOR_CURVE_TYPE_LUT_3x1D)
			fprintf(fp, " [%u]", xform->pre_curve.u.lut_3x1d.optimal_len);
		sep = ", ";
	}

	if (mapping_type != WESTON_COLOR_MAPPING_TYPE_IDENTITY) {
		fprintf(fp, "%smapping %s", sep, mapping_type_to_str(mapping_type));
		if (mapping_type == WESTON_COLOR_MAPPING_TYPE_3D_LUT)
			fprintf(fp, " [%u]", xform->mapping.u.lut3d.optimal_len);
		sep = ", ";
	}

	if (post_type != WESTON_COLOR_CURVE_TYPE_IDENTITY) {
		fprintf(fp, "%spost %s", sep, curve_type_to_str(post_type));
		if (post_type == WESTON_COLOR_CURVE_TYPE_LUT_3x1D)
			fprintf(fp, " [%u]", xform->post_curve.u.lut_3x1d.optimal_len);
		sep = ", ";
	}

	if (sep == empty)
		fprintf(fp, "identity\n");
	else
		fprintf(fp, "\n");

	fclose(fp);
	abort_oom_if_null(str);

	return str;
}

/** Deep copy */
void
weston_surface_color_transform_copy(struct weston_surface_color_transform *dst,
				    const struct weston_surface_color_transform *src)
{
	*dst = *src;
	dst->transform = weston_color_transform_ref(src->transform);
}

/** Unref contents */
void
weston_surface_color_transform_fini(struct weston_surface_color_transform *surf_xform)
{
	weston_color_transform_unref(surf_xform->transform);
	surf_xform->transform = NULL;
	surf_xform->identity_pipeline = false;
}

/**
 * Ensure that the surface's color transformation for the given output is
 * populated in the paint nodes for all the views.
 *
 * Creates the color transformation description if necessary by calling
 * into the color manager.
 *
 * \param pnode Paint node defining the surface and the output. All
 * paint nodes with the same surface and output will be ensured.
 */
void
weston_paint_node_ensure_color_transform(struct weston_paint_node *pnode)
{
	struct weston_surface *surface = pnode->surface;
	struct weston_output *output = pnode->output;
	struct weston_color_manager *cm = surface->compositor->color_manager;
	struct weston_surface_color_transform surf_xform = {};
	struct weston_paint_node *it;
	bool ok;

	/*
	 * Invariant: all paint nodes with the same surface+output have the
	 * same surf_xform state.
	 */
	if (pnode->surf_xform_valid)
		return;

	ok = cm->get_surface_color_transform(cm, surface, output, &surf_xform);

	wl_list_for_each(it, &surface->paint_node_list, surface_link) {
		if (it->output == output) {
			assert(it->surf_xform_valid == false);
			assert(it->surf_xform.transform == NULL);
			weston_surface_color_transform_copy(&it->surf_xform,
							    &surf_xform);
			it->surf_xform_valid = ok;
		}
	}

	weston_surface_color_transform_fini(&surf_xform);

	if (!ok) {
		if (surface->resource)
			wl_resource_post_no_memory(surface->resource);
		weston_log("Failed to create color transformation for a surface.\n");
	}
}

/**
 * Load ICC profile file
 *
 * Loads an ICC profile file, ensures it is fit for use, and returns a
 * new reference to the weston_color_profile. Use weston_color_profile_unref()
 * to free it.
 *
 * \param compositor The compositor instance, identifies the color manager.
 * \param path Path to the ICC file to be open()'d.
 * \return A color profile reference, or NULL on failure.
 *
 * Error messages are printed to libweston log.
 *
 * This function is not meant for loading profiles on behalf of Wayland
 * clients.
 */
WL_EXPORT struct weston_color_profile *
weston_compositor_load_icc_file(struct weston_compositor *compositor,
				const char *path)
{
	struct weston_color_manager *cm = compositor->color_manager;
	struct weston_color_profile *cprof = NULL;
	int fd;
	struct stat icc_stat;
	void *icc_data;
	size_t len;
	char *errmsg = NULL;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		weston_log("Error: Cannot open ICC profile \"%s\" for reading: %s\n",
			   path, strerror(errno));
		return NULL;
	}

	if (fstat(fd, &icc_stat) != 0) {
		weston_log("Error: Cannot fstat ICC profile \"%s\": %s\n",
			   path, strerror(errno));
		goto out_close;
	}
	len = icc_stat.st_size;
	if (len < 1) {
		weston_log("Error: ICC profile \"%s\" has no size.\n", path);
		goto out_close;
	}

	icc_data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (icc_data == MAP_FAILED) {
		weston_log("Error: Cannot mmap ICC profile \"%s\": %s\n",
			   path, strerror(errno));
		goto out_close;
	}

	if (!cm->get_color_profile_from_icc(cm, icc_data, len,
					    path, &cprof, &errmsg)) {
		weston_log("Error: loading ICC profile \"%s\" failed: %s\n",
			   path, errmsg);
		free(errmsg);
	}

	munmap(icc_data, len);

out_close:
	close(fd);
	return cprof;
}

/** Get a string naming the EOTF mode
 *
 * \internal
 */
WL_EXPORT const char *
weston_eotf_mode_to_str(enum weston_eotf_mode e)
{
	switch (e) {
	case WESTON_EOTF_MODE_NONE:		return "(none)";
	case WESTON_EOTF_MODE_SDR:		return "SDR";
	case WESTON_EOTF_MODE_TRADITIONAL_HDR:	return "traditional gamma HDR";
	case WESTON_EOTF_MODE_ST2084:		return "ST2084";
	case WESTON_EOTF_MODE_HLG:		return "HLG";
	}
	return "???";
}

/** A list of EOTF modes as a string
 *
 * \param eotf_mask Bitwise-or'd enum weston_eotf_mode values.
 * \return Comma separated names of the listed EOTF modes. Must be free()'d by
 * the caller.
 */
WL_EXPORT char *
weston_eotf_mask_to_str(uint32_t eotf_mask)
{
	FILE *fp;
	char *str = NULL;
	size_t size = 0;
	unsigned i;
	const char *sep = "";

	fp = open_memstream(&str, &size);
	if (!fp)
		return NULL;

	for (i = 0; eotf_mask; i++) {
		uint32_t bitmask = 1u << i;

		if (eotf_mask & bitmask) {
			fprintf(fp, "%s%s", sep,
				weston_eotf_mode_to_str(bitmask));
			sep = ", ";
		}

		eotf_mask &= ~bitmask;
	}
	fclose(fp);

	return str;
}
