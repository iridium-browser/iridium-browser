/*
 * Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_MSM

#include <assert.h>
#include <dlfcn.h>
#include <drm_fourcc.h>
#include <errno.h>
#include <inttypes.h>
#include <msm_drm.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include "drv_helpers.h"
#include "drv_priv.h"
#include "util.h"

/* Alignment values are based on SDM845 Gfx IP */
#define DEFAULT_ALIGNMENT 64
#define BUFFER_SIZE_ALIGN 4096

#define VENUS_STRIDE_ALIGN 128
#define VENUS_SCANLINE_ALIGN 16
#define NV12_LINEAR_PADDING (12 * 1024)
#define NV12_UBWC_PADDING(y_stride) (MAX(16 * 1024, y_stride * 48))
#define MACROTILE_WIDTH_ALIGN 64
#define MACROTILE_HEIGHT_ALIGN 16
#define PLANE_SIZE_ALIGN 4096

#define MSM_UBWC_TILING 1

static const uint32_t render_target_formats[] = { DRM_FORMAT_ABGR8888,	   DRM_FORMAT_ARGB8888,
						  DRM_FORMAT_RGB565,	   DRM_FORMAT_XBGR8888,
						  DRM_FORMAT_XRGB8888,	   DRM_FORMAT_ABGR2101010,
						  DRM_FORMAT_ABGR16161616F };

static const uint32_t texture_source_formats[] = { DRM_FORMAT_NV12, DRM_FORMAT_R8,
						   DRM_FORMAT_YVU420, DRM_FORMAT_YVU420_ANDROID,
						   DRM_FORMAT_P010 };

/*
 * Each macrotile consists of m x n (mostly 4 x 4) tiles.
 * Pixel data pitch/stride is aligned with macrotile width.
 * Pixel data height is aligned with macrotile height.
 * Entire pixel data buffer is aligned with 4k(bytes).
 */
static uint32_t get_ubwc_meta_size(uint32_t width, uint32_t height, uint32_t tile_width,
				   uint32_t tile_height)
{
	uint32_t macrotile_width, macrotile_height;

	macrotile_width = DIV_ROUND_UP(width, tile_width);
	macrotile_height = DIV_ROUND_UP(height, tile_height);

	// Align meta buffer width to 64 blocks
	macrotile_width = ALIGN(macrotile_width, MACROTILE_WIDTH_ALIGN);

	// Align meta buffer height to 16 blocks
	macrotile_height = ALIGN(macrotile_height, MACROTILE_HEIGHT_ALIGN);

	return ALIGN(macrotile_width * macrotile_height, PLANE_SIZE_ALIGN);
}

static unsigned get_pitch_alignment(struct bo *bo)
{
	switch (bo->meta.format) {
	case DRM_FORMAT_NV12:
		return VENUS_STRIDE_ALIGN;
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		/* TODO other YUV formats? */
		/* Something (in the video stack?) assumes the U/V planes can use
		 * half the pitch as the Y plane.. to componsate, double the
		 * alignment:
		 */
		return 2 * DEFAULT_ALIGNMENT;
	default:
		return DEFAULT_ALIGNMENT;
	}
}

static void msm_calculate_layout(struct bo *bo)
{
	uint32_t width, height;

	width = bo->meta.width;
	height = bo->meta.height;

	/* NV12 format requires extra padding with platform
	 * specific alignments for venus driver
	 */
	if (bo->meta.format == DRM_FORMAT_NV12 || bo->meta.format == DRM_FORMAT_P010) {
		uint32_t y_stride, uv_stride, y_scanline, uv_scanline, y_plane, uv_plane, size,
		    extra_padding;

		// P010 has the same layout as NV12.  The difference is that each
		// pixel in P010 takes 2 bytes, while in NV12 each pixel takes 1 byte.
		if (bo->meta.format == DRM_FORMAT_P010)
			width *= 2;

		y_stride = ALIGN(width, VENUS_STRIDE_ALIGN);
		uv_stride = ALIGN(width, VENUS_STRIDE_ALIGN);
		y_scanline = ALIGN(height, VENUS_SCANLINE_ALIGN * 2);
		uv_scanline = ALIGN(DIV_ROUND_UP(height, 2),
				    VENUS_SCANLINE_ALIGN * (bo->meta.tiling ? 2 : 1));
		y_plane = y_stride * y_scanline;
		uv_plane = uv_stride * uv_scanline;

		if (bo->meta.tiling == MSM_UBWC_TILING) {
			y_plane = ALIGN(y_plane, PLANE_SIZE_ALIGN);
			uv_plane = ALIGN(uv_plane, PLANE_SIZE_ALIGN);
			y_plane += get_ubwc_meta_size(width, height, 32, 8);
			uv_plane += get_ubwc_meta_size(width >> 1, height >> 1, 16, 8);
			extra_padding = NV12_UBWC_PADDING(y_stride);
		} else {
			extra_padding = NV12_LINEAR_PADDING;
		}

		bo->meta.strides[0] = y_stride;
		bo->meta.sizes[0] = y_plane;
		bo->meta.offsets[1] = y_plane;
		bo->meta.strides[1] = uv_stride;
		size = y_plane + uv_plane + extra_padding;
		bo->meta.total_size = ALIGN(size, BUFFER_SIZE_ALIGN);
		bo->meta.sizes[1] = bo->meta.total_size - bo->meta.sizes[0];
	} else {
		uint32_t stride, alignw, alignh;

		alignw = ALIGN(width, get_pitch_alignment(bo));
		/* HAL_PIXEL_FORMAT_YV12 requires that the buffer's height not be aligned.
			DRM_FORMAT_R8 of height one is used for JPEG camera output, so don't
			height align that. */
		if (bo->meta.format == DRM_FORMAT_YVU420_ANDROID ||
		    bo->meta.format == DRM_FORMAT_YVU420 ||
		    (bo->meta.format == DRM_FORMAT_R8 && height == 1)) {
			assert(bo->meta.tiling != MSM_UBWC_TILING);
			alignh = height;
		} else {
			alignh = ALIGN(height, DEFAULT_ALIGNMENT);
		}

		stride = drv_stride_from_format(bo->meta.format, alignw, 0);

		/* Calculate size and assign stride, size, offset to each plane based on format */
		drv_bo_from_format(bo, stride, alignh, bo->meta.format);

		/* For all RGB UBWC formats */
		if (bo->meta.tiling == MSM_UBWC_TILING) {
			bo->meta.sizes[0] += get_ubwc_meta_size(width, height, 16, 4);
			bo->meta.total_size = bo->meta.sizes[0];
			assert(IS_ALIGNED(bo->meta.total_size, BUFFER_SIZE_ALIGN));
		}
	}
}

static bool is_ubwc_fmt(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
#ifndef QCOM_DISABLE_COMPRESSED_NV12
	case DRM_FORMAT_NV12:
#endif
		return 1;
	default:
		return 0;
	}
}

static void msm_add_ubwc_combinations(struct driver *drv, const uint32_t *formats,
				      uint32_t num_formats, struct format_metadata *metadata,
				      uint64_t use_flags)
{
	for (uint32_t i = 0; i < num_formats; i++) {
		if (is_ubwc_fmt(formats[i])) {
			struct combination combo = { .format = formats[i],
						     .metadata = *metadata,
						     .use_flags = use_flags };
			drv_array_append(drv->combos, &combo);
		}
	}
}

/**
 * Check for buggy apps that are known to not support modifiers, to avoid surprising them
 * with a UBWC buffer.
 */
static bool should_avoid_ubwc(void)
{
#ifndef __ANDROID__
	/* waffle is buggy and, requests a renderable buffer (which on qcom platforms, we
	 * want to use UBWC), and then passes it to the kernel discarding the modifier.
	 * So mesa ends up correctly rendering to as tiled+compressed, but kernel tries
	 * to display as linear.  Other platforms do not see this issue, simply because
	 * they only use compressed (ex, AFBC) with the BO_USE_SCANOUT flag.
	 *
	 * See b/163137550
	 */
	if (dlsym(RTLD_DEFAULT, "waffle_display_connect")) {
		drv_logi("WARNING: waffle detected, disabling UBWC\n");
		return true;
	}
#endif
	return false;
}

static int msm_init(struct driver *drv)
{
	struct format_metadata metadata;
	uint64_t render_use_flags = BO_USE_RENDER_MASK | BO_USE_SCANOUT;
	uint64_t texture_use_flags = BO_USE_TEXTURE_MASK | BO_USE_HW_VIDEO_DECODER;
	/*
	 * NOTE: we actually could use tiled in the BO_USE_FRONT_RENDERING case,
	 * if we had a modifier for tiled-but-not-compressed.  But we *cannot* use
	 * compressed in this case because the UBWC flags/meta data can be out of
	 * sync with pixel data while the GPU is writing a frame out to memory.
	 */
	uint64_t sw_flags =
	    (BO_USE_RENDERSCRIPT | BO_USE_SW_MASK | BO_USE_LINEAR | BO_USE_FRONT_RENDERING);

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &LINEAR_METADATA, render_use_flags);

	drv_add_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
			     &LINEAR_METADATA, texture_use_flags);

	/* The camera stack standardizes on NV12 for YUV buffers. */
	/* YVU420 and NV12 formats for camera, display and encoding. */
	drv_modify_combination(drv, DRM_FORMAT_NV12, &LINEAR_METADATA,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_SCANOUT |
				   BO_USE_HW_VIDEO_ENCODER);

	/*
	 * R8 format is used for Android's HAL_PIXEL_FORMAT_BLOB and is used for JPEG snapshots
	 * from camera and input/output from hardware decoder/encoder.
	 */
	drv_modify_combination(drv, DRM_FORMAT_R8, &LINEAR_METADATA,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_HW_VIDEO_ENCODER | BO_USE_GPU_DATA_BUFFER |
				   BO_USE_SENSOR_DIRECT_DATA);

	/*
	 * Android also frequently requests YV12 formats for some camera implementations
	 * (including the external provider implmenetation).
	 */
	drv_modify_combination(drv, DRM_FORMAT_YVU420_ANDROID, &LINEAR_METADATA,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	drv_modify_combination(drv, DRM_FORMAT_YVU420, &LINEAR_METADATA,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);

	/* Android CTS tests require this. */
	drv_add_combination(drv, DRM_FORMAT_BGR888, &LINEAR_METADATA, BO_USE_SW_MASK);

#ifdef SC_7280
	drv_modify_combination(drv, DRM_FORMAT_P010, &LINEAR_METADATA,
			       BO_USE_SCANOUT | BO_USE_HW_VIDEO_ENCODER);
#endif

	drv_modify_linear_combinations(drv);

	if (should_avoid_ubwc() || !drv->compression)
		return 0;

	metadata.tiling = MSM_UBWC_TILING;
	metadata.priority = 2;
	metadata.modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED;

	render_use_flags &= ~sw_flags;
	texture_use_flags &= ~sw_flags;

	msm_add_ubwc_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
				  &metadata, render_use_flags);

	msm_add_ubwc_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
				  &metadata, texture_use_flags);

	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata,
			       BO_USE_SCANOUT | BO_USE_HW_VIDEO_ENCODER);

	return 0;
}

static int msm_bo_create_for_modifier(struct bo *bo, uint32_t width, uint32_t height,
				      uint32_t format, const uint64_t modifier)
{
	struct drm_msm_gem_new req = { 0 };
	int ret;
	size_t i;

	bo->meta.tiling = (modifier == DRM_FORMAT_MOD_QCOM_COMPRESSED) ? MSM_UBWC_TILING : 0;
	msm_calculate_layout(bo);

	req.flags = MSM_BO_WC | MSM_BO_SCANOUT;
	req.size = bo->meta.total_size;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MSM_GEM_NEW, &req);
	if (ret) {
		drv_loge("DRM_IOCTL_MSM_GEM_NEW failed with %s\n", strerror(errno));
		return -errno;
	}

	/*
	 * Though we use only one plane, we need to set handle for
	 * all planes to pass kernel checks
	 */
	for (i = 0; i < bo->meta.num_planes; i++)
		bo->handles[i].u32 = req.handle;

	bo->meta.format_modifier = modifier;
	return 0;
}

static int msm_bo_create_with_modifiers(struct bo *bo, uint32_t width, uint32_t height,
					uint32_t format, const uint64_t *modifiers, uint32_t count)
{
	static const uint64_t modifier_order[] = {
		DRM_FORMAT_MOD_QCOM_COMPRESSED,
		DRM_FORMAT_MOD_LINEAR,
	};

	uint64_t modifier =
	    drv_pick_modifier(modifiers, count, modifier_order, ARRAY_SIZE(modifier_order));

	if (!bo->drv->compression && modifier == DRM_FORMAT_MOD_QCOM_COMPRESSED)
		modifier = DRM_FORMAT_MOD_LINEAR;

	return msm_bo_create_for_modifier(bo, width, height, format, modifier);
}

/* msm_bo_create will create linear buffers for now */
static int msm_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			 uint64_t flags)
{
	struct combination *combo = drv_get_combination(bo->drv, format, flags);

	if (!combo) {
		drv_loge("invalid format = %d, flags = %" PRIx64 " combination\n", format, flags);
		return -EINVAL;
	}

	return msm_bo_create_for_modifier(bo, width, height, format, combo->metadata.modifier);
}

static void *msm_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	struct drm_msm_gem_info req = { 0 };

	req.handle = bo->handles[0].u32;
	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MSM_GEM_INFO, &req);
	if (ret) {
		drv_loge("DRM_IOCLT_MSM_GEM_INFO failed with %s\n", strerror(errno));
		return MAP_FAILED;
	}
	vma->length = bo->meta.total_size;

	return mmap(0, bo->meta.total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
		    req.offset);
}

const struct backend backend_msm = {
	.name = "msm",
	.init = msm_init,
	.bo_create = msm_bo_create,
	.bo_create_with_modifiers = msm_bo_create_with_modifiers,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_import = drv_prime_bo_import,
	.bo_map = msm_bo_map,
	.bo_unmap = drv_bo_munmap,
	.resolve_format_and_use_flags = drv_resolve_format_and_use_flags_helper,
};
#endif /* DRV_MSM */
