/*
 * Copyright © 2016, 2019 Collabora, Ltd.
 * Copyright (c) 2018 DisplayLink (UK) Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniels@collabora.com>
 */

#include <inttypes.h>
#include <stdbool.h>

/**
 * Contains information about pixel formats, mapping format codes from
 * wl_shm and drm_fourcc.h (which are deliberately identical, but for the
 * special cases of WL_SHM_ARGB8888 and WL_SHM_XRGB8888) into various
 * sets of information. Helper functions are provided for dealing with these
 * raw structures.
 */
struct pixel_format_info {
	/** DRM/wl_shm format code */
	uint32_t format;

	/** The DRM format name without the DRM_FORMAT_ prefix. */
	const char *drm_format_name;

	/** If non-zero, number of planes in base (non-modified) format. */
	int num_planes;

	/** If format contains alpha channel, opaque equivalent of format,
	 *  i.e. alpha channel replaced with X. */
	uint32_t opaque_substitute;

	/** How the format should be sampled, expressed in terms of tokens
	 *  from the EGL_WL_bind_wayland_display extension. If not set,
	 *  assumed to be either RGB or RGBA, depending on whether or not
	 *  the format contains an alpha channel. The samplers may still
	 *  return alpha even for opaque formats; users must manually set
	 *  the alpha channel to 1.0 (or ignore it) if the format is
	 *  opaque. */
	uint32_t sampler_type;

	/** GL format, if data can be natively/directly uploaded. Note that
	 *  whilst DRM formats are little-endian unless explicitly specified,
	 *  (i.e. DRM_FORMAT_ARGB8888 is stored BGRA as sequential bytes in
	 *  memory), GL uses the sequential byte order, so that format maps to
	 *  GL_BGRA_EXT plus GL_UNSIGNED_BYTE. To add to the confusion, the
	 *  explicitly-sized types (e.g. GL_UNSIGNED_SHORT_5_5_5_1) read in
	 *  machine-endian order, so for these types, the correspondence
	 *  depends on endianness. */
	int gl_format;

	/** GL data type, if data can be natively/directly uploaded. */
	int gl_type;

	/** If set, this format can be used with the legacy drmModeAddFB()
	 *  function (not AddFB2), using this and the bpp member. */
	int depth;

	/** See 'depth' member above. */
	int bpp;

	/** Horizontal subsampling; if non-zero, divide the width by this
	 *  member to obtain the number of columns in the source buffer for
	 *  secondary planes only. Stride is not affected by horizontal
	 *  subsampling. */
	int hsub;

	/** Vertical subsampling; if non-zero, divide the height by this
	 *  member to obtain the number of rows in the source buffer for
	 *  secondary planes only. */
	int vsub;

	/* Ordering of chroma components. */
	enum {
		ORDER_UV = 0,
		ORDER_VU,
	} chroma_order;

	/* If packed YUV (num_planes == 1), ordering of luma/chroma
	 * components. */
	enum {
		ORDER_LUMA_CHROMA = 0,
		ORDER_CHROMA_LUMA,
	} luma_chroma_order;

	/** How many significant bits each channel has, or zero if N/A. */
	struct {
		int r;
		int g;
		int b;
		int a;
	} bits;

	/** How channel bits are interpreted, fixed (uint) or floating-point */
	enum {
		PIXEL_COMPONENT_TYPE_FIXED = 0,
		PIXEL_COMPONENT_TYPE_FLOAT,
	} component_type;
};

/**
 * Get pixel format information for a DRM format code
 *
 * Given a DRM format code, return a pixel format info structure describing
 * the properties of that format.
 *
 * @param format DRM format code to get info for
 * @returns A pixel format structure (must not be freed), or NULL if the
 *          format could not be found
 */
const struct pixel_format_info *
pixel_format_get_info(uint32_t format);

/**
 * Get pixel format information for a SHM format code
 *
 * Given a SHM format code, return a DRM pixel format info structure describing
 * the properties of that format.
 *
 * @param format SHM format code to get info for.
 * @returns A pixel format structure (must not be freed), or NULL if the
 *          format could not be found.
 */
const struct pixel_format_info *
pixel_format_get_info_shm(uint32_t format);

/**
 * Get pixel format information for a named DRM format
 *
 * Given a DRM format name, return a pixel format info structure describing
 * the properties of that format.
 *
 * The DRM format name is the preprocessor token name from drm_fourcc.h
 * without the DRM_FORMAT_ prefix. The search is also case-insensitive.
 * Both "xrgb8888" and "XRGB8888" searches will find DRM_FORMAT_XRGB8888
 * for example.
 *
 * @param drm_format_name DRM format name to get info for (not NULL)
 * @returns A pixel format structure (must not be freed), or NULL if the
 *          name could not be found
 */
const struct pixel_format_info *
pixel_format_get_info_by_drm_name(const char *drm_format_name);

/**
 * Get number of planes used by a pixel format
 *
 * Given a pixel format info structure, return the number of planes
 * required for a buffer. Note that this is not necessarily identical to
 * the number of samplers required to be bound, as two views into a single
 * plane are sometimes required.
 *
 * @param format Pixel format info structure
 * @returns Number of planes required for the format
 */
unsigned int
pixel_format_get_plane_count(const struct pixel_format_info *format);

/**
 * Determine if a pixel format is opaque or contains alpha
 *
 * Returns whether or not the pixel format is opaque, or contains a
 * significant alpha channel. Note that the suggested EGL sampler type may
 * still sample undefined data into the alpha channel; users must consider
 * alpha as 1.0 if the format is opaque, and not rely on the sampler to
 * return this when sampling from the alpha channel.
 *
 * @param format Pixel format info structure
 * @returns True if the format is opaque, or false if it has significant alpha
 */
bool
pixel_format_is_opaque(const struct pixel_format_info *format);

/**
 * Get compatible opaque equivalent for a format
 *
 * Given a pixel format info structure, return a format which is wholly
 * compatible with the input format, but opaque, ignoring the alpha channel.
 * If an alpha format is provided, but the content is known to all be opaque,
 * then this can be used as a substitute to avoid blending.
 *
 * If the input format is opaque, this function will return the input format.
 *
 * @param format Pixel format info structure
 * @returns A pixel format info structure for the compatible opaque substitute
 */
const struct pixel_format_info *
pixel_format_get_opaque_substitute(const struct pixel_format_info *format);

/**
 * For an opaque format, get the equivalent format with alpha instead of an
 * ignored channel
 *
 * This is the opposite lookup from pixel_format_get_opaque_substitute().
 * Finds the format whose opaque substitute is the given format.
 *
 * If the input format is not opaque or does not have ignored (X) bits, then
 * the search cannot find a match.
 *
 * @param format DRM format code to search for
 * @returns A pixel format info structure for the pixel format whose opaque
 * substitute is the argument, or NULL if no match.
 */
const struct pixel_format_info *
pixel_format_get_info_by_opaque_substitute(uint32_t format);

/**
 * Return the effective sampling width for a given plane
 *
 * When horizontal subsampling is effective, a sampler bound to a secondary
 * plane must bind the sampler with a smaller effective width. This function
 * returns the effective width to use for the sampler, i.e. dividing by hsub.
 *
 * If horizontal subsampling is not in effect, this will be equal to the
 * width.
 *
 * @param format Pixel format info structure
 * @param plane Zero-indexed plane number
 * @param width Width of the buffer
 * @returns Effective width for sampling
 */
unsigned int
pixel_format_width_for_plane(const struct pixel_format_info *format,
			     unsigned int plane,
			     unsigned int width);

/**
 * Return the effective sampling height for a given plane
 *
 * When vertical subsampling is in effect, a sampler bound to a secondary
 * plane must bind the sampler with a smaller effective height. This function
 * returns the effective height to use for the sampler, i.e. dividing by vsub.
 *
 * If vertical subsampling is not in effect, this will be equal to the height.
 *
 * @param format Pixel format info structure
 * @param plane Zero-indexed plane number
 * @param height Height of the buffer
 * @returns Effective width for sampling
 */
unsigned int
pixel_format_height_for_plane(const struct pixel_format_info *format,
			      unsigned int plane,
			      unsigned int height);
