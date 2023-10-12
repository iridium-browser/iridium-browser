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

#ifndef WESTON_COLOR_H
#define WESTON_COLOR_H

#include <stdbool.h>
#include <stdint.h>
#include <libweston/libweston.h>

/**
 * Represents a color profile description (an ICC color profile)
 *
 * Sub-classed by the color manager that created this.
 */
struct weston_color_profile {
	struct weston_color_manager *cm;
	int ref_count;
	char *description;
};

/** Type or formula for a curve */
enum weston_color_curve_type {
	/** Identity function, no-op */
	WESTON_COLOR_CURVE_TYPE_IDENTITY = 0,

	/** Three-channel, one-dimensional look-up table */
	WESTON_COLOR_CURVE_TYPE_LUT_3x1D,
};

/** LUT_3x1D parameters */
struct weston_color_curve_lut_3x1d {
	/**
	 * Approximate a color curve with three 1D LUTs
	 *
	 * A 1D LUT is a mapping from [0.0, 1.0] to arbitrary values. The first
	 * element in the LUT corresponds to input value 0.0, and the last
	 * element corresponds to input value 1.0. The step from one element
	 * to the next in input space is 1.0 / (len - 1). When input value is
	 * between two elements, linear interpolation should be used.
	 *
	 * This function fills in the given array with the LUT values.
	 *
	 * \param xform This color transformation object.
	 * \param len The number of elements in each 1D LUT.
	 * \param values Array of 3 x len elements. First R channel
	 * LUT, immediately followed by G channel LUT, and then B channel LUT.
	 */
	void
	(*fill_in)(struct weston_color_transform *xform,
		   float *values, unsigned len);

	/** Optimal 1D LUT length for storage vs. precision */
	unsigned optimal_len;
};

/**
 * A scalar function for color encoding and decoding
 *
 * This object can represent a one-dimensional function that is applied
 * independently to each of the color channels. Depending on the type and
 * parameterization of the curve, all color channels may use the
 * same function or each may have separate parameters.
 *
 * This is usually used for EOTF or EOTF^-1 and to optimize a 3D LUT size
 * without sacrificing precision, both in one step.
 */
struct weston_color_curve {
	/** Which member of 'u' defines the curve. */
	enum weston_color_curve_type type;

	/** Parameters for the curve. */
	union {
		/* identity: no parameters */
		struct weston_color_curve_lut_3x1d lut_3x1d;
	} u;
};

/** Type or formula for a color mapping */
enum weston_color_mapping_type {
	/** Identity function, no-op */
	WESTON_COLOR_MAPPING_TYPE_IDENTITY = 0,

	/** 3D-dimensional look-up table */
	WESTON_COLOR_MAPPING_TYPE_3D_LUT,

	/** matrix */
	WESTON_COLOR_MAPPING_TYPE_MATRIX,
};

/**
 * A three-dimensional look-up table
 *
 * A 3D LUT is a three-dimensional array where each element is an RGB triplet.
 * A 3D LUT is usually an approximation of some arbitrary color mapping
 * function that cannot be represented in any simpler form. The array contains
 * samples from the approximated function, and values between samples are
 * estimated by interpolation. The array is accessed with three indices, one
 * for each input dimension (color channel).
 *
 * Color channel values in the range [0.0, 1.0] are mapped linearly to
 * 3D LUT indices such that 0.0 maps exactly to the first element and 1.0 maps
 * exactly to the last element in each dimension.
 *
 * This object represents a 3D LUT and offers an interface for realizing it
 * as a data array with a custom size.
 */
struct weston_color_mapping_3dlut {
	/**
	 * Create a 3D LUT data array
	 *
	 * \param xform This color transformation object.
	 * \param values Memory to hold the resulting data array.
	 * \param len The number of elements in each dimension.
	 *
	 * The array \c values must be at least 3 * len * len * len elements
	 * in size.
	 *
	 * Given the red index ri, green index gi and blue index bi, the
	 * corresponding array element index
	 *
	 *     i = 3 * (len * len * bi + len * gi + ri) + c
	 *
	 * where
	 *
	 *     c = 0 for red output value,
	 *     c = 1 for green output value, and
	 *     c = 2 for blue output value
	 */
	void
	(*fill_in)(struct weston_color_transform *xform,
		   float *values, unsigned len);

	/** Optimal 3D LUT size along each dimension */
	unsigned optimal_len;
};

/**
 * A 3x3 matrix and data is arranged as column major
 */
struct weston_color_mapping_matrix {
	float matrix[9];
};

/**
 * Color mapping function
 *
 * This object can represent a 3D LUT to do a color space conversion
 *
 */
struct weston_color_mapping {
	/** Which member of 'u' defines the color mapping type */
	enum weston_color_mapping_type type;

	/** Parameters for the color mapping function */
	union {
		/* identity: no parameters */
		struct weston_color_mapping_3dlut lut3d;
		struct weston_color_mapping_matrix mat;
	} u;
};

/**
 * Describes a color transformation formula
 *
 * Guaranteed unique, de-duplicated.
 *
 * Sub-classed by the color manager that created this.
 *
 * For a renderer to support WESTON_CAP_COLOR_OPS it must implement everything
 * that this structure can represent.
 */
struct weston_color_transform {
	struct weston_color_manager *cm;
	int ref_count;

	/* for renderer or backend to attach their own cached objects */
	struct wl_signal destroy_signal;

	/* Color transform is the series of steps: */

	/** Step 1: color model change */
	/* YCbCr→RGB conversion, but that is done elsewhere */

	/** Step 2: color curve before color mapping */
	struct weston_color_curve pre_curve;

	/** Step 3: color mapping */
	struct weston_color_mapping mapping;

	/** Step 4: color curve after color mapping */
	struct weston_color_curve post_curve;
};

/**
 * How content color needs to be transformed
 *
 * This object is specific to the color properties of the weston_surface and
 * weston_output it was created for. It is automatically destroyed if any
 * relevant color properties change.
 *
 * Fundamentally this contains the color transformation from content color
 * space to an output's blending color space. This is stored in field
 * 'transform' with NULL value corresponding to identity transformation.
 *
 * For graphics pipeline optimization purposes, the field 'identity_pipeline'
 * indicates whether the combination of 'transform' here and the output's
 * blending color space to monitor color space transformation total to
 * identity transformation. This helps detecting cases where renderer bypass
 * (direct scanout) is possible.
 */
struct weston_surface_color_transform {
	/** Transformation from source to blending space */
	struct weston_color_transform *transform;

	/** True, if source colorspace is identical to monitor color space */
	bool identity_pipeline;
};

struct weston_color_manager {
	/** Identifies this CMS component */
	const char *name;

	/** This compositor instance */
	struct weston_compositor *compositor;

	/** Supports the Wayland CM&HDR protocol extension? */
	bool supports_client_protocol;

	/** Initialize color manager */
	bool
	(*init)(struct weston_color_manager *cm);

	/** Destroy color manager */
	void
	(*destroy)(struct weston_color_manager *cm);

	/** Destroy a color profile after refcount fell to zero */
	void
	(*destroy_color_profile)(struct weston_color_profile *cprof);

	/** Create a color profile from ICC data
	 *
	 * \param cm The color manager.
	 * \param icc_data Pointer to the ICC binary data.
	 * \param icc_len Length of the ICC data in bytes.
	 * \param name_part A string to be used in describing the profile.
	 * \param cprof_out On success, the created object is returned here.
	 * On failure, untouched.
	 * \param errmsg On success, untouched. On failure, a pointer to a
	 * string describing the error is stored here. The string must be
	 * free()'d.
	 * \return True on success, false on failure.
	 *
	 * This may return a new reference to an existing color profile if
	 * that profile is identical to the one that would be created, apart
	 * from name_part.
	 */
	bool
	(*get_color_profile_from_icc)(struct weston_color_manager *cm,
				      const void *icc_data,
				      size_t icc_len,
				      const char *name_part,
				      struct weston_color_profile **cprof_out,
				      char **errmsg);

	/** Destroy a color transform after refcount fell to zero */
	void
	(*destroy_color_transform)(struct weston_color_transform *xform);

	/** Get surface to output's blending space transformation
	 *
	 * \param cm The color manager.
	 * \param surface The surface for the source color space.
	 * \param output The output for the destination blending color space.
	 * \param surf_xform For storing the color transformation and
	 * additional information.
	 *
	 * The callee is responsible for increasing the reference count on the
	 * weston_color_transform it stores into surf_xform.
	 */
	bool
	(*get_surface_color_transform)(struct weston_color_manager *cm,
				       struct weston_surface *surface,
				       struct weston_output *output,
				       struct weston_surface_color_transform *surf_xform);

	/** Compute derived color properties for an output
	 *
	 * \param cm The color manager.
	 * \param output The output.
	 * \return A new color_outcome object on success, NULL on failure.
	 *
	 * The callee (color manager) must inspect the weston_output (color
	 * profile, EOTF mode, etc.) and create a fully populated
	 * weston_output_color_outcome object.
	 */
	struct weston_output_color_outcome *
	(*create_output_color_outcome)(struct weston_color_manager *cm,
				       struct weston_output *output);
};

void
weston_color_profile_init(struct weston_color_profile *cprof,
			  struct weston_color_manager *cm);

struct weston_color_transform *
weston_color_transform_ref(struct weston_color_transform *xform);

void
weston_color_transform_unref(struct weston_color_transform *xform);

void
weston_color_transform_init(struct weston_color_transform *xform,
			    struct weston_color_manager *cm);

char *
weston_color_transform_string(const struct weston_color_transform *xform);

void
weston_surface_color_transform_copy(struct weston_surface_color_transform *dst,
				    const struct weston_surface_color_transform *src);

void
weston_surface_color_transform_fini(struct weston_surface_color_transform *surf_xform);

struct weston_paint_node;

void
weston_paint_node_ensure_color_transform(struct weston_paint_node *pnode);

struct weston_color_manager *
weston_color_manager_noop_create(struct weston_compositor *compositor);

/* DSO module entrypoint */
struct weston_color_manager *
weston_color_manager_create(struct weston_compositor *compositor);

const char *
weston_eotf_mode_to_str(enum weston_eotf_mode e);

char *
weston_eotf_mask_to_str(uint32_t eotf_mask);

void
weston_output_color_outcome_destroy(struct weston_output_color_outcome **pco);

#endif /* WESTON_COLOR_H */
