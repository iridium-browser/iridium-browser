/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_MEDIATEK

// clang-format off
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <mediatek_drm.h>
// clang-format on

#include "drv_helpers.h"
#include "drv_priv.h"
#include "util.h"

#define TILE_TYPE_LINEAR 0

// clang-format off
#if defined(MTK_MT8183) || \
    defined(MTK_MT8186)
// clang-format on
#define SUPPORTS_YUV422
#endif

// All platforms except MT8173 should USE_NV12_FOR_HW_VIDEO_DECODING.
// clang-format off
#if defined(MTK_MT8183) || \
    defined(MTK_MT8186) || \
    defined(MTK_MT8188G) || \
    defined(MTK_MT8192) || \
    defined(MTK_MT8195)
// clang-format on
#define USE_NV12_FOR_HW_VIDEO_DECODING
#else
#define DONT_USE_64_ALIGNMENT_FOR_VIDEO_BUFFERS
#endif

// For Mali Sigurd based GPUs, the texture unit reads outside the specified texture dimensions.
// Therefore, certain formats require extra memory padding to its allocated surface to prevent the
// hardware from reading outside an allocation. For YVU420, we need additional padding for the last
// chroma plane.
#if defined(MTK_MT8186)
#define USE_EXTRA_PADDING_FOR_YVU420
#endif

struct mediatek_private_map_data {
	void *cached_addr;
	void *gem_addr;
	int prime_fd;
};

static const uint32_t render_target_formats[] = { DRM_FORMAT_ABGR8888, DRM_FORMAT_ARGB8888,
						  DRM_FORMAT_RGB565, DRM_FORMAT_XBGR8888,
						  DRM_FORMAT_XRGB8888 };

// clang-format off
static const uint32_t texture_source_formats[] = {
#ifdef SUPPORTS_YUV422
	DRM_FORMAT_NV21,
	DRM_FORMAT_YUYV,
#endif
	DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_ABGR16161616F,
	DRM_FORMAT_NV12,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YVU420_ANDROID
};

static const uint32_t video_yuv_formats[] = {
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV12,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YVU420_ANDROID
};
// clang-format on

static bool is_video_yuv_format(uint32_t format)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(video_yuv_formats); ++i) {
		if (format == video_yuv_formats[i])
			return true;
	}
	return false;
}

static int mediatek_init(struct driver *drv)
{
	struct format_metadata metadata;

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &LINEAR_METADATA, BO_USE_RENDER_MASK | BO_USE_SCANOUT);

	drv_add_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
			     &LINEAR_METADATA, BO_USE_TEXTURE_MASK);

	drv_add_combination(drv, DRM_FORMAT_R8, &LINEAR_METADATA, BO_USE_SW_MASK | BO_USE_LINEAR);

	/* YUYV format for video overlay and camera subsystem. */
	drv_add_combination(drv, DRM_FORMAT_YUYV, &LINEAR_METADATA,
			    BO_USE_HW_VIDEO_DECODER | BO_USE_SCANOUT | BO_USE_LINEAR |
				BO_USE_TEXTURE);

	/* Android CTS tests require this. */
	drv_add_combination(drv, DRM_FORMAT_BGR888, &LINEAR_METADATA, BO_USE_SW_MASK);

	/* Support BO_USE_HW_VIDEO_DECODER for protected content minigbm allocations. */
	metadata.tiling = TILE_TYPE_LINEAR;
	metadata.priority = 1;
	metadata.modifier = DRM_FORMAT_MOD_LINEAR;
	drv_modify_combination(drv, DRM_FORMAT_YVU420, &metadata, BO_USE_HW_VIDEO_DECODER);
	drv_modify_combination(drv, DRM_FORMAT_YVU420_ANDROID, &metadata, BO_USE_HW_VIDEO_DECODER);
#ifdef USE_NV12_FOR_HW_VIDEO_DECODING
	// TODO(hiroh): Switch to use NV12 for video decoder on MT8173 as well.
	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata, BO_USE_HW_VIDEO_DECODER);
#endif

	/*
	 * R8 format is used for Android's HAL_PIXEL_FORMAT_BLOB for input/output from
	 * hardware decoder/encoder.
	 */
	drv_modify_combination(drv, DRM_FORMAT_R8, &metadata,
			       BO_USE_HW_VIDEO_DECODER | BO_USE_HW_VIDEO_ENCODER |
				   BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE |
				   BO_USE_GPU_DATA_BUFFER | BO_USE_SENSOR_DIRECT_DATA);

	/* NV12 format for encoding and display. */
	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata,
			       BO_USE_SCANOUT | BO_USE_HW_VIDEO_ENCODER | BO_USE_CAMERA_READ |
				   BO_USE_CAMERA_WRITE);

#ifdef MTK_MT8183
	/* Only for MT8183 Camera subsystem */
	drv_modify_combination(drv, DRM_FORMAT_NV21, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	drv_modify_combination(drv, DRM_FORMAT_YUYV, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	drv_modify_combination(drv, DRM_FORMAT_YVU420, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	/* Private formats for private reprocessing in camera */
	drv_add_combination(drv, DRM_FORMAT_MTISP_SXYZW10, &metadata,
			    BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_SW_MASK);
#endif

	return drv_modify_linear_combinations(drv);
}

static int mediatek_bo_create_with_modifiers(struct bo *bo, uint32_t width, uint32_t height,
					     uint32_t format, const uint64_t *modifiers,
					     uint32_t count)
{
	int ret;
	size_t plane;
	uint32_t stride;
	struct drm_mtk_gem_create gem_create = { 0 };
	/*
	 * We identify the ChromeOS Camera App buffers via these two USE flags. Those buffers need
	 * the same alignment as the video hardware encoding.
	 */
	const bool is_camera_preview =
	    (bo->meta.use_flags & BO_USE_SCANOUT) && (bo->meta.use_flags & BO_USE_CAMERA_WRITE);

	if (!drv_has_modifier(modifiers, count, DRM_FORMAT_MOD_LINEAR)) {
		errno = EINVAL;
		drv_loge("no usable modifier found\n");
		return -EINVAL;
	}

	/*
	 * Since the ARM L1 cache line size is 64 bytes, align to that as a
	 * performance optimization, except for video buffers on certain platforms,
	 * these should only be accessed from the GPU and VCODEC subsystems (maybe
	 * also MDP), so it's better to align to macroblocks.
	 */
	stride = drv_stride_from_format(format, width, 0);
#ifdef DONT_USE_64_ALIGNMENT_FOR_VIDEO_BUFFERS
	const uint32_t alignment = is_video_yuv_format(format) ? 16 : 64;
	stride = ALIGN(stride, alignment);
#else
	stride = ALIGN(stride, 64);
#endif

	if ((bo->meta.use_flags & BO_USE_HW_VIDEO_ENCODER) || is_camera_preview) {
		uint32_t aligned_height = ALIGN(height, 32);
		uint32_t padding[DRV_MAX_PLANES] = { 0 };

		for (plane = 0; plane < bo->meta.num_planes; ++plane) {
			uint32_t plane_stride = drv_stride_from_format(format, stride, plane);
			padding[plane] = plane_stride *
					 (32 / drv_vertical_subsampling_from_format(format, plane));
		}

		drv_bo_from_format_and_padding(bo, stride, aligned_height, format, padding);
	} else {
#ifdef SUPPORTS_YUV422
		/*
		 * JPEG Encoder Accelerator requires 16x16 alignment. We want the buffer
		 * from camera can be put in JEA directly so align the height to 16
		 * bytes.
		 */
		if (format == DRM_FORMAT_NV12)
			height = ALIGN(height, 16);
#endif
		drv_bo_from_format(bo, stride, height, format);

#ifdef USE_EXTRA_PADDING_FOR_YVU420
		/*
		 * Apply extra padding for YV12 if the height does not meet round up requirement and
		 * the image is to be sampled by gpu.
		 */
		static const uint32_t required_round_up = 4;
		const uint32_t height_mod = height % required_round_up;
		if ((format == DRM_FORMAT_YVU420 || format == DRM_FORMAT_YVU420_ANDROID) &&
		    (bo->meta.use_flags & BO_USE_TEXTURE) && height_mod) {
			const uint32_t height_padding = required_round_up - height_mod;
			const uint32_t u_padding =
			    drv_size_from_format(format, bo->meta.strides[2], height_padding, 2);

			bo->meta.total_size += u_padding;

			/*
			 * Since we are not aligning Y, we must make sure that its padding fits
			 * inside the rest of the space allocated for the V/U planes.
			 */
			const uint32_t y_padding =
			    drv_size_from_format(format, bo->meta.strides[0], height_padding, 0);
			const uint32_t vu_size = drv_bo_get_plane_size(bo, 2) * 2;
			if (y_padding > vu_size) {
				/* Align with mali workaround to pad all 3 planes. */
				bo->meta.total_size += y_padding + u_padding;
			}
		}
#endif
	}

	gem_create.size = bo->meta.total_size;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MTK_GEM_CREATE, &gem_create);
	if (ret) {
		drv_loge("DRM_IOCTL_MTK_GEM_CREATE failed (size=%" PRIu64 ")\n", gem_create.size);
		return -errno;
	}

	for (plane = 0; plane < bo->meta.num_planes; plane++)
		bo->handles[plane].u32 = gem_create.handle;

	return 0;
}

static int mediatek_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			      uint64_t use_flags)
{
	uint64_t modifiers[] = { DRM_FORMAT_MOD_LINEAR };
	return mediatek_bo_create_with_modifiers(bo, width, height, format, modifiers,
						 ARRAY_SIZE(modifiers));
}

static void *mediatek_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret, prime_fd;
	struct drm_mtk_gem_map_off gem_map = { 0 };
	struct mediatek_private_map_data *priv;
	void *addr = NULL;

	gem_map.handle = bo->handles[0].u32;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_MTK_GEM_MAP_OFFSET, &gem_map);
	if (ret) {
		drv_loge("DRM_IOCTL_MTK_GEM_MAP_OFFSET failed\n");
		return MAP_FAILED;
	}

	prime_fd = drv_bo_get_plane_fd(bo, 0);
	if (prime_fd < 0) {
		drv_loge("Failed to get a prime fd\n");
		return MAP_FAILED;
	}

	addr = mmap(0, bo->meta.total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
		    gem_map.offset);
	if (addr == MAP_FAILED)
		goto out_close_prime_fd;

	vma->length = bo->meta.total_size;

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		goto out_unmap_addr;

	if (bo->meta.use_flags & BO_USE_RENDERSCRIPT) {
		priv->cached_addr = calloc(1, bo->meta.total_size);
		if (!priv->cached_addr)
			goto out_free_priv;

		priv->gem_addr = addr;
		addr = priv->cached_addr;
	}

	priv->prime_fd = prime_fd;
	vma->priv = priv;

	return addr;

out_free_priv:
	free(priv);
out_unmap_addr:
	munmap(addr, bo->meta.total_size);
out_close_prime_fd:
	close(prime_fd);
	return MAP_FAILED;
}

static int mediatek_bo_unmap(struct bo *bo, struct vma *vma)
{
	if (vma->priv) {
		struct mediatek_private_map_data *priv = vma->priv;

		if (priv->cached_addr) {
			vma->addr = priv->gem_addr;
			free(priv->cached_addr);
		}

		close(priv->prime_fd);
		free(priv);
		vma->priv = NULL;
	}

	return munmap(vma->addr, vma->length);
}

static int mediatek_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	struct mediatek_private_map_data *priv = mapping->vma->priv;

	if (priv) {
		struct pollfd fds = {
			.fd = priv->prime_fd,
		};

		if (mapping->vma->map_flags & BO_MAP_WRITE)
			fds.events |= POLLOUT;

		if (mapping->vma->map_flags & BO_MAP_READ)
			fds.events |= POLLIN;

		poll(&fds, 1, -1);
		if (fds.revents != fds.events)
			drv_loge("poll prime_fd failed\n");

		if (priv->cached_addr)
			memcpy(priv->cached_addr, priv->gem_addr, bo->meta.total_size);
	}

	return 0;
}

static int mediatek_bo_flush(struct bo *bo, struct mapping *mapping)
{
	struct mediatek_private_map_data *priv = mapping->vma->priv;
	if (priv && priv->cached_addr && (mapping->vma->map_flags & BO_MAP_WRITE))
		memcpy(priv->gem_addr, priv->cached_addr, bo->meta.total_size);

	return 0;
}

static void mediatek_resolve_format_and_use_flags(struct driver *drv, uint32_t format,
						  uint64_t use_flags, uint32_t *out_format,
						  uint64_t *out_use_flags)
{
	*out_format = format;
	*out_use_flags = use_flags;
	switch (format) {
	case DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED:
#ifdef MTK_MT8183
		/* Only MT8183 Camera subsystem offers private reprocessing
		 * capability. CAMERA_READ indicates the buffer is intended for
		 * reprocessing and hence given the private format for MTK. */
		if (use_flags & BO_USE_CAMERA_READ) {
			*out_format = DRM_FORMAT_MTISP_SXYZW10;
			break;
		}
#endif
		if (use_flags & BO_USE_CAMERA_WRITE) {
			*out_format = DRM_FORMAT_NV12;
			break;
		}

		/* HACK: See b/28671744 */
		*out_format = DRM_FORMAT_XBGR8888;
		*out_use_flags &= ~BO_USE_HW_VIDEO_ENCODER;
		break;
	case DRM_FORMAT_FLEX_YCbCr_420_888:
#ifdef USE_NV12_FOR_HW_VIDEO_DECODING
		// TODO(hiroh): Switch to use NV12 for video decoder on MT8173 as well.
		if (use_flags & (BO_USE_HW_VIDEO_DECODER)) {
			*out_format = DRM_FORMAT_NV12;
			break;
		}
#endif
		if (use_flags &
		    (BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_HW_VIDEO_ENCODER)) {
			*out_format = DRM_FORMAT_NV12;
			break;
		}

		/* HACK: See b/139714614 */
		*out_format = DRM_FORMAT_YVU420;
		*out_use_flags &= ~BO_USE_SCANOUT;
		break;
	default:
		break;
	}
	/* Mediatek doesn't support YUV overlays */
	if (is_video_yuv_format(format))
		*out_use_flags &= ~BO_USE_SCANOUT;
}

const struct backend backend_mediatek = {
	.name = "mediatek",
	.init = mediatek_init,
	.bo_create = mediatek_bo_create,
	.bo_create_with_modifiers = mediatek_bo_create_with_modifiers,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_import = drv_prime_bo_import,
	.bo_map = mediatek_bo_map,
	.bo_unmap = mediatek_bo_unmap,
	.bo_invalidate = mediatek_bo_invalidate,
	.bo_flush = mediatek_bo_flush,
	.resolve_format_and_use_flags = mediatek_resolve_format_and_use_flags,
};

#endif
