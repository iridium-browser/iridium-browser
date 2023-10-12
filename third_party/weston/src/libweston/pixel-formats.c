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

#include "config.h"

#include <endian.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>

#include <xf86drm.h>

#include "shared/helpers.h"
#include "shared/string-helpers.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/xalloc.h"
#include "wayland-util.h"
#include "pixel-formats.h"

#ifdef ENABLE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#define GL_FORMAT(fmt) .gl_format = (fmt)
#define GL_TYPE(type) .gl_type = (type)
#define SAMPLER_TYPE(type) .sampler_type = (type)
#else
#define GL_FORMAT(fmt) .gl_format = 0
#define GL_TYPE(type) .gl_type = 0
#define SAMPLER_TYPE(type) .sampler_type = 0
#endif

#define DRM_FORMAT(f) .format = DRM_FORMAT_ ## f, .drm_format_name = #f
#define BITS_RGBA_FIXED(r_, g_, b_, a_) \
	.bits.r = r_, \
	.bits.g = g_, \
	.bits.b = b_, \
	.bits.a = a_, \
	.component_type = PIXEL_COMPONENT_TYPE_FIXED
#define BITS_RGBA_FLOAT(r_, g_, b_, a_) \
	.bits.r = r_, \
	.bits.g = g_, \
	.bits.b = b_, \
	.bits.a = a_, \
	.component_type = PIXEL_COMPONENT_TYPE_FLOAT

#define PIXMAN_FMT(fmt) .pixman_format = (PIXMAN_ ## fmt)

#include "shared/weston-egl-ext.h"

/**
 * Table of DRM formats supported by Weston; RGB, ARGB and YUV formats are
 * supported. Indexed/greyscale formats, and formats not containing complete
 * colour channels, are not supported.
 */
static const struct pixel_format_info pixel_format_table[] = {
	{
		DRM_FORMAT(R8),
		BITS_RGBA_FIXED(8, 0, 0, 0),
		.bpp = 8,
		.hide_from_clients = true,
		GL_FORMAT(GL_R8_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(GR88),
		BITS_RGBA_FIXED(8, 8, 0, 0),
		.bpp = 16,
		.hide_from_clients = true,
		GL_FORMAT(GL_RG8_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(XRGB4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(ARGB4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_XRGB4444,
	},
	{
		DRM_FORMAT(XBGR4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(ABGR4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_XBGR4444,
	},
	{
		DRM_FORMAT(RGBX4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
		.bpp = 16,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_4_4_4_4),
#endif
	},
	{
		DRM_FORMAT(RGBA4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_RGBX4444,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_4_4_4_4),
#endif
	},
	{
		DRM_FORMAT(BGRX4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(BGRA4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_BGRX4444,
	},
	{
		DRM_FORMAT(XRGB1555),
		BITS_RGBA_FIXED(5, 5, 5, 0),
		.addfb_legacy_depth = 15,
		.bpp = 16,
	},
	{
		DRM_FORMAT(ARGB1555),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_XRGB1555,
	},
	{
		DRM_FORMAT(XBGR1555),
		BITS_RGBA_FIXED(5, 5, 5, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(ABGR1555),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_XBGR1555,
	},
	{
		DRM_FORMAT(RGBX5551),
		BITS_RGBA_FIXED(5, 5, 5, 0),
		.bpp = 16,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_5_5_5_1),
#endif
	},
	{
		DRM_FORMAT(RGBA5551),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_RGBX5551,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_5_5_5_1),
#endif
	},
	{
		DRM_FORMAT(BGRX5551),
		BITS_RGBA_FIXED(5, 5, 5, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(BGRA5551),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.bpp = 16,
		.opaque_substitute = DRM_FORMAT_BGRX5551,
	},
	{
		DRM_FORMAT(RGB565),
		BITS_RGBA_FIXED(5, 6, 5, 0),
		.addfb_legacy_depth = 16,
		.bpp = 16,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGB),
		GL_TYPE(GL_UNSIGNED_SHORT_5_6_5),
		PIXMAN_FMT(r5g6b5),
#endif
	},
	{
		DRM_FORMAT(BGR565),
		BITS_RGBA_FIXED(5, 6, 5, 0),
		.bpp = 16,
	},
	{
		DRM_FORMAT(RGB888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.bpp = 24,
	},
	{
		DRM_FORMAT(BGR888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.bpp = 24,
		GL_FORMAT(GL_RGB),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(XRGB8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.addfb_legacy_depth = 24,
		.bpp = 32,
		GL_FORMAT(GL_BGRA_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(x8r8g8b8),
#else
		PIXMAN_FMT(b8g8r8x8),
#endif
	},
	{
		DRM_FORMAT(ARGB8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.opaque_substitute = DRM_FORMAT_XRGB8888,
		.addfb_legacy_depth = 32,
		.bpp = 32,
		GL_FORMAT(GL_BGRA_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(a8r8g8b8),
#else
		PIXMAN_FMT(b8g8r8a8),
#endif
	},
	{
		DRM_FORMAT(XBGR8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.bpp = 32,
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_BYTE),
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(x8b8g8r8),
#else
		PIXMAN_FMT(r8g8b8x8),
#endif
	},
	{
		DRM_FORMAT(ABGR8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_XBGR8888,
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_BYTE),
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(a8b8g8r8),
#else
		PIXMAN_FMT(r8g8b8a8),
#endif
	},
	{
		DRM_FORMAT(RGBX8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.bpp = 32,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(r8g8b8x8),
#else
		PIXMAN_FMT(x8b8g8r8),
#endif
	},
	{
		DRM_FORMAT(RGBA8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_RGBX8888,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(r8g8b8a8),
#else
		PIXMAN_FMT(a8b8g8r8),
#endif
	},
	{
		DRM_FORMAT(BGRX8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.bpp = 32,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(b8g8r8x8),
#else
		PIXMAN_FMT(x8r8g8b8),
#endif
	},
	{
		DRM_FORMAT(BGRA8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_BGRX8888,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(b8g8r8a8),
#else
		PIXMAN_FMT(a8r8g8b8),
#endif
	},
	{
		DRM_FORMAT(XRGB2101010),
		BITS_RGBA_FIXED(10, 10, 10, 0),
		.addfb_legacy_depth = 30,
		.bpp = 32,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(x2r10g10b10),
#endif
	},
	{
		DRM_FORMAT(ARGB2101010),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_XRGB2101010,
#if __BYTE_ORDER == __LITTLE_ENDIAN
		PIXMAN_FMT(a2r10g10b10),
#endif
	},
	{
		DRM_FORMAT(XBGR2101010),
		BITS_RGBA_FIXED(10, 10, 10, 0),
		.bpp = 32,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_INT_2_10_10_10_REV_EXT),
		PIXMAN_FMT(x2b10g10r10),
#endif
	},
	{
		DRM_FORMAT(ABGR2101010),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_XBGR2101010,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_INT_2_10_10_10_REV_EXT),
		PIXMAN_FMT(a2b10g10r10),
#endif
	},
	{
		DRM_FORMAT(RGBX1010102),
		BITS_RGBA_FIXED(10, 10, 10, 0),
		.bpp = 32,
	},
	{
		DRM_FORMAT(RGBA1010102),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_RGBX1010102,
	},
	{
		DRM_FORMAT(BGRX1010102),
		BITS_RGBA_FIXED(10, 10, 10, 0),
		.bpp = 32,
	},
	{
		DRM_FORMAT(BGRA1010102),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.bpp = 32,
		.opaque_substitute = DRM_FORMAT_BGRX1010102,
	},
	{
		DRM_FORMAT(XBGR16161616),
		BITS_RGBA_FIXED(16, 16, 16, 0),
		.bpp = 64,
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA16_EXT),
		GL_TYPE(GL_UNSIGNED_SHORT),
#endif
	},
	{
		DRM_FORMAT(ABGR16161616),
		BITS_RGBA_FIXED(16, 16, 16, 16),
		.bpp = 64,
		.opaque_substitute = DRM_FORMAT_XBGR16161616,
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA16_EXT),
		GL_TYPE(GL_UNSIGNED_SHORT),
#endif
	},
	{
		DRM_FORMAT(XBGR16161616F),
		BITS_RGBA_FLOAT(16, 16, 16, 0),
		.bpp = 64,
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA16F),
		GL_TYPE(GL_HALF_FLOAT),
#endif
	},
	{
		DRM_FORMAT(ABGR16161616F),
		BITS_RGBA_FLOAT(16, 16, 16, 16),
		.bpp = 64,
		.opaque_substitute = DRM_FORMAT_XBGR16161616F,
#if __BYTE_ORDER__ == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA16F),
		GL_TYPE(GL_HALF_FLOAT),
#endif
	},
	{
		DRM_FORMAT(XRGB16161616F),
		BITS_RGBA_FLOAT(16, 16, 16, 0),
		.bpp = 64,
	},
	{
		DRM_FORMAT(ARGB16161616F),
		BITS_RGBA_FLOAT(16, 16, 16, 16),
		.bpp = 64,
		.opaque_substitute = DRM_FORMAT_XRGB16161616F,
	},
	{
		DRM_FORMAT(YUYV),
		SAMPLER_TYPE(EGL_TEXTURE_Y_XUXV_WL),
		.num_planes = 1,
		.hsub = 2,
	},
	{
		DRM_FORMAT(YVYU),
		SAMPLER_TYPE(EGL_TEXTURE_Y_XUXV_WL),
		.num_planes = 1,
		.chroma_order = ORDER_VU,
		.hsub = 2,
	},
	{
		DRM_FORMAT(UYVY),
		SAMPLER_TYPE(EGL_TEXTURE_Y_XUXV_WL),
		.num_planes = 1,
		.luma_chroma_order = ORDER_CHROMA_LUMA,
		.hsub = 2,
	},
	{
		DRM_FORMAT(VYUY),
		SAMPLER_TYPE(EGL_TEXTURE_Y_XUXV_WL),
		.num_planes = 1,
		.luma_chroma_order = ORDER_CHROMA_LUMA,
		.chroma_order = ORDER_VU,
		.hsub = 2,
	},
	{
		DRM_FORMAT(NV12),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
		.hsub = 2,
		.vsub = 2,
	},
	{
		DRM_FORMAT(NV21),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
		.chroma_order = ORDER_VU,
		.hsub = 2,
		.vsub = 2,
	},
	{
		DRM_FORMAT(NV16),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
		.hsub = 2,
		.vsub = 1,
	},
	{
		DRM_FORMAT(NV61),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
		.chroma_order = ORDER_VU,
		.hsub = 2,
		.vsub = 1,
	},
	{
		DRM_FORMAT(NV24),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
	},
	{
		DRM_FORMAT(NV42),
		SAMPLER_TYPE(EGL_TEXTURE_Y_UV_WL),
		.num_planes = 2,
		.chroma_order = ORDER_VU,
	},
	{
		DRM_FORMAT(YUV410),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.hsub = 4,
		.vsub = 4,
	},
	{
		DRM_FORMAT(YVU410),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.chroma_order = ORDER_VU,
		.hsub = 4,
		.vsub = 4,
	},
	{
		DRM_FORMAT(YUV411),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.hsub = 4,
		.vsub = 1,
	},
	{
		DRM_FORMAT(YVU411),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.chroma_order = ORDER_VU,
		.hsub = 4,
		.vsub = 1,
	},
	{
		DRM_FORMAT(YUV420),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.hsub = 2,
		.vsub = 2,
	},
	{
		DRM_FORMAT(YVU420),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.chroma_order = ORDER_VU,
		.hsub = 2,
		.vsub = 2,
	},
	{
		DRM_FORMAT(YUV422),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.hsub = 2,
		.vsub = 1,
	},
	{
		DRM_FORMAT(YVU422),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.chroma_order = ORDER_VU,
		.hsub = 2,
		.vsub = 1,
	},
	{
		DRM_FORMAT(YUV444),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
	},
	{
		DRM_FORMAT(YVU444),
		SAMPLER_TYPE(EGL_TEXTURE_Y_U_V_WL),
		.num_planes = 3,
		.chroma_order = ORDER_VU,
	},
	{
		DRM_FORMAT(XYUV8888),
		.bpp = 32,
	},
};

WL_EXPORT const struct pixel_format_info *
pixel_format_get_info_shm(uint32_t format)
{
	if (format == WL_SHM_FORMAT_XRGB8888)
		return pixel_format_get_info(DRM_FORMAT_XRGB8888);
	else if (format == WL_SHM_FORMAT_ARGB8888)
		return pixel_format_get_info(DRM_FORMAT_ARGB8888);
	else
		return pixel_format_get_info(format);
}

WL_EXPORT const struct pixel_format_info *
pixel_format_get_info(uint32_t format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(pixel_format_table); i++) {
		if (pixel_format_table[i].format == format)
			return &pixel_format_table[i];
	}

	return NULL;
}

WL_EXPORT const struct pixel_format_info *
pixel_format_get_info_by_index(unsigned int index)
{
	if (index >= ARRAY_LENGTH(pixel_format_table))
		return NULL;

	return &pixel_format_table[index];
}

WL_EXPORT unsigned int
pixel_format_get_info_count(void)
{
	return ARRAY_LENGTH(pixel_format_table);
}


WL_EXPORT const struct pixel_format_info *
pixel_format_get_info_by_drm_name(const char *drm_format_name)
{
	const struct pixel_format_info *info;
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(pixel_format_table); i++) {
		info = &pixel_format_table[i];
		if (strcasecmp(info->drm_format_name, drm_format_name) == 0)
			return info;
	}

	return NULL;
}

WL_EXPORT const struct pixel_format_info *
pixel_format_get_info_by_pixman(pixman_format_code_t pixman_format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(pixel_format_table); i++) {
		if (pixel_format_table[i].pixman_format == pixman_format)
			return &pixel_format_table[i];
	}

	return NULL;
}

WL_EXPORT unsigned int
pixel_format_get_plane_count(const struct pixel_format_info *info)
{
	return info->num_planes ? info->num_planes : 1;
}

WL_EXPORT bool
pixel_format_is_opaque(const struct pixel_format_info *info)
{
	return !info->opaque_substitute;
}

WL_EXPORT const struct pixel_format_info *
pixel_format_get_opaque_substitute(const struct pixel_format_info *info)
{
	if (!info->opaque_substitute)
		return info;
	else
		return pixel_format_get_info(info->opaque_substitute);
}

WL_EXPORT const struct pixel_format_info *
pixel_format_get_info_by_opaque_substitute(uint32_t format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(pixel_format_table); i++) {
		if (pixel_format_table[i].opaque_substitute == format)
			return &pixel_format_table[i];
	}

	return NULL;
}

WL_EXPORT unsigned int
pixel_format_hsub(const struct pixel_format_info *info,
		  unsigned int plane)
{
	/* We don't support any formats where the first plane is subsampled. */
	if (plane == 0 || info->hsub == 0)
		return 1;

	return info->hsub;
}

WL_EXPORT unsigned int
pixel_format_vsub(const struct pixel_format_info *info,
		  unsigned int plane)
{
	/* We don't support any formats where the first plane is subsampled. */
	if (plane == 0 || info->vsub == 0)
		return 1;

	return info->vsub;
}

WL_EXPORT unsigned int
pixel_format_width_for_plane(const struct pixel_format_info *info,
			     unsigned int plane,
			     unsigned int width)
{
	return width / pixel_format_hsub(info, plane);
}

WL_EXPORT unsigned int
pixel_format_height_for_plane(const struct pixel_format_info *info,
			      unsigned int plane,
			      unsigned int height)
{
	return height / pixel_format_vsub(info, plane);
}

WL_EXPORT char *
pixel_format_get_modifier(uint64_t modifier)
{
	char *modifier_name;
	char *vendor_name;
	char *mod_str;

	modifier_name = drmGetFormatModifierName(modifier);
	vendor_name = drmGetFormatModifierVendor(modifier);

	if (!modifier_name) {
		if (vendor_name)
			str_printf(&mod_str, "%s_%s (0x%llx)",
				   vendor_name, "UNKNOWN_MODIFIER",
				   (unsigned long long) modifier);
		else
			str_printf(&mod_str, "0x%llx",
				 (unsigned long long) modifier);

		free(vendor_name);
		return mod_str;
	}

	if (modifier == DRM_FORMAT_MOD_LINEAR) {
		str_printf(&mod_str, "%s (0x%llx)", modifier_name,
			   (unsigned long long) modifier);
		free(modifier_name);
		free(vendor_name);
		return mod_str;
	}

	str_printf(&mod_str, "%s_%s (0x%llx)", vendor_name, modifier_name,
		   (unsigned long long) modifier);

	free(modifier_name);
	free(vendor_name);

	return mod_str;
}

WL_EXPORT uint32_t
pixel_format_get_shm_format(const struct pixel_format_info *info)
{
	/* Only these two format codes differ between wl_shm and DRM fourcc */
	switch (info->format) {
	case DRM_FORMAT_ARGB8888:
		return WL_SHM_FORMAT_ARGB8888;
	case DRM_FORMAT_XRGB8888:
		return WL_SHM_FORMAT_XRGB8888;
	default:
		break;
	}

	return info->format;
}

WL_EXPORT const struct pixel_format_info **
pixel_format_get_array(const uint32_t *drm_formats, unsigned int formats_count)
{
	const struct pixel_format_info **formats;
	unsigned int i;

	formats = xcalloc(formats_count, sizeof(*formats));
	for (i = 0; i < formats_count; i++) {
		formats[i] = pixel_format_get_info(drm_formats[i]);
		if (!formats[i]) {
			free(formats);
			return NULL;
		}
	}

	return formats;
}
