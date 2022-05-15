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
#include <string.h>
#include <drm_fourcc.h>
#include <wayland-client-protocol.h>

#include "shared/helpers.h"
#include "wayland-util.h"
#include "pixel-formats.h"

#if ENABLE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
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

#include "shared/weston-egl-ext.h"

/**
 * Table of DRM formats supported by Weston; RGB, ARGB and YUV formats are
 * supported. Indexed/greyscale formats, and formats not containing complete
 * colour channels, are not supported.
 */
static const struct pixel_format_info pixel_format_table[] = {
	{
		DRM_FORMAT(XRGB4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
	},
	{
		DRM_FORMAT(ARGB4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.opaque_substitute = DRM_FORMAT_XRGB4444,
	},
	{
		DRM_FORMAT(XBGR4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
	},
	{
		DRM_FORMAT(ABGR4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.opaque_substitute = DRM_FORMAT_XBGR4444,
	},
	{
		DRM_FORMAT(RGBX4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_4_4_4_4),
#endif
	},
	{
		DRM_FORMAT(RGBA4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.opaque_substitute = DRM_FORMAT_RGBX4444,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_4_4_4_4),
#endif
	},
	{
		DRM_FORMAT(BGRX4444),
		BITS_RGBA_FIXED(4, 4, 4, 0),
	},
	{
		DRM_FORMAT(BGRA4444),
		BITS_RGBA_FIXED(4, 4, 4, 4),
		.opaque_substitute = DRM_FORMAT_BGRX4444,
	},
	{
		DRM_FORMAT(XRGB1555),
		BITS_RGBA_FIXED(5, 5, 5, 0),
		.depth = 15,
		.bpp = 16,
	},
	{
		DRM_FORMAT(ARGB1555),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.opaque_substitute = DRM_FORMAT_XRGB1555,
	},
	{
		DRM_FORMAT(XBGR1555),
		BITS_RGBA_FIXED(5, 5, 5, 0),
	},
	{
		DRM_FORMAT(ABGR1555),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.opaque_substitute = DRM_FORMAT_XBGR1555,
	},
	{
		DRM_FORMAT(RGBX5551),
		BITS_RGBA_FIXED(5, 5, 5, 0),
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_5_5_5_1),
#endif
	},
	{
		DRM_FORMAT(RGBA5551),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.opaque_substitute = DRM_FORMAT_RGBX5551,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_SHORT_5_5_5_1),
#endif
	},
	{
		DRM_FORMAT(BGRX5551),
		BITS_RGBA_FIXED(5, 5, 5, 0),
	},
	{
		DRM_FORMAT(BGRA5551),
		BITS_RGBA_FIXED(5, 5, 5, 1),
		.opaque_substitute = DRM_FORMAT_BGRX5551,
	},
	{
		DRM_FORMAT(RGB565),
		BITS_RGBA_FIXED(5, 6, 5, 0),
		.depth = 16,
		.bpp = 16,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGB),
		GL_TYPE(GL_UNSIGNED_SHORT_5_6_5),
#endif
	},
	{
		DRM_FORMAT(BGR565),
		BITS_RGBA_FIXED(5, 6, 5, 0),
	},
	{
		DRM_FORMAT(RGB888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
	},
	{
		DRM_FORMAT(BGR888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		GL_FORMAT(GL_RGB),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(XRGB8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		.depth = 24,
		.bpp = 32,
		GL_FORMAT(GL_BGRA_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(ARGB8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.opaque_substitute = DRM_FORMAT_XRGB8888,
		.depth = 32,
		.bpp = 32,
		GL_FORMAT(GL_BGRA_EXT),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(XBGR8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(ABGR8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.opaque_substitute = DRM_FORMAT_XBGR8888,
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_BYTE),
	},
	{
		DRM_FORMAT(RGBX8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
	},
	{
		DRM_FORMAT(RGBA8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.opaque_substitute = DRM_FORMAT_RGBX8888,
	},
	{
		DRM_FORMAT(BGRX8888),
		BITS_RGBA_FIXED(8, 8, 8, 0),
	},
	{
		DRM_FORMAT(BGRA8888),
		BITS_RGBA_FIXED(8, 8, 8, 8),
		.opaque_substitute = DRM_FORMAT_BGRX8888,
	},
	{
		DRM_FORMAT(XRGB2101010),
		BITS_RGBA_FIXED(10, 10, 10, 0),
		.depth = 30,
		.bpp = 32,
	},
	{
		DRM_FORMAT(ARGB2101010),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.opaque_substitute = DRM_FORMAT_XRGB2101010,
	},
	{
		DRM_FORMAT(XBGR2101010),
		BITS_RGBA_FIXED(10, 10, 10, 0),
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_INT_2_10_10_10_REV_EXT),
#endif
	},
	{
		DRM_FORMAT(ABGR2101010),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.opaque_substitute = DRM_FORMAT_XBGR2101010,
# if __BYTE_ORDER == __LITTLE_ENDIAN
		GL_FORMAT(GL_RGBA),
		GL_TYPE(GL_UNSIGNED_INT_2_10_10_10_REV_EXT),
#endif
	},
	{
		DRM_FORMAT(RGBX1010102),
		BITS_RGBA_FIXED(10, 10, 10, 0),
	},
	{
		DRM_FORMAT(RGBA1010102),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.opaque_substitute = DRM_FORMAT_RGBX1010102,
	},
	{
		DRM_FORMAT(BGRX1010102),
		BITS_RGBA_FIXED(10, 10, 10, 0),
	},
	{
		DRM_FORMAT(BGRA1010102),
		BITS_RGBA_FIXED(10, 10, 10, 2),
		.opaque_substitute = DRM_FORMAT_BGRX1010102,
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
pixel_format_width_for_plane(const struct pixel_format_info *info,
			     unsigned int plane,
			     unsigned int width)
{
	/* We don't support any formats where the first plane is subsampled. */
	if (plane == 0 || !info->hsub)
		return width;

	return width / info->hsub;
}

WL_EXPORT unsigned int
pixel_format_height_for_plane(const struct pixel_format_info *info,
			      unsigned int plane,
			      unsigned int height)
{
	/* We don't support any formats where the first plane is subsampled. */
	if (plane == 0 || !info->vsub)
		return height;

	return height / info->vsub;
}
