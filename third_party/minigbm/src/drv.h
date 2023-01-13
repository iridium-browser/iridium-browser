/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DRV_H_
#define DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drm_fourcc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DRV_MAX_PLANES 4

// clang-format off
/* Use flags */
#define BO_USE_NONE			0
#define BO_USE_SCANOUT			(1ull << 0)
#define BO_USE_CURSOR			(1ull << 1)
#define BO_USE_CURSOR_64X64		BO_USE_CURSOR
#define BO_USE_RENDERING		(1ull << 2)
/* Skip for GBM_BO_USE_WRITE */
#define BO_USE_LINEAR			(1ull << 4)
#define BO_USE_TEXTURE			(1ull << 5)
#define BO_USE_CAMERA_WRITE		(1ull << 6)
#define BO_USE_CAMERA_READ		(1ull << 7)
#define BO_USE_PROTECTED		(1ull << 8)
#define BO_USE_SW_READ_OFTEN		(1ull << 9)
#define BO_USE_SW_READ_RARELY	        (1ull << 10)
#define BO_USE_SW_WRITE_OFTEN	        (1ull << 11)
#define BO_USE_SW_WRITE_RARELY		(1ull << 12)
#define BO_USE_HW_VIDEO_DECODER         (1ull << 13)
#define BO_USE_HW_VIDEO_ENCODER         (1ull << 14)
#define BO_USE_TEST_ALLOC		(1ull << 15)
#define BO_USE_FRONT_RENDERING		(1ull << 16)
#define BO_USE_RENDERSCRIPT		(1ull << 17)
#define BO_USE_GPU_DATA_BUFFER		(1ull << 18)
#define BO_USE_SENSOR_DIRECT_DATA	(1ull << 19)

/* Quirks for allocating a buffer. */
#define BO_QUIRK_NONE			0
#define BO_QUIRK_DUMB32BPP		(1ull << 0)

/* Map flags */
#define BO_MAP_NONE 0
#define BO_MAP_READ (1 << 0)
#define BO_MAP_WRITE (1 << 1)
#define BO_MAP_READ_WRITE (BO_MAP_READ | BO_MAP_WRITE)

/* This is our extension to <drm_fourcc.h>.  We need to make sure we don't step
 * on the namespace of already defined formats, which can be done by using invalid
 * fourcc codes.
 */
#define DRM_FORMAT_NONE				fourcc_code('0', '0', '0', '0')
#define DRM_FORMAT_YVU420_ANDROID		fourcc_code('9', '9', '9', '7')
#define DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED	fourcc_code('9', '9', '9', '8')
#define DRM_FORMAT_FLEX_YCbCr_420_888		fourcc_code('9', '9', '9', '9')

/* This is a 10-bit bayer format for private reprocessing on MediaTek ISP. It's
 * a private RAW format that other DRM drivers will never support and thus
 * making it not upstreamable (i.e., defined in official DRM headers). */
#define DRM_FORMAT_MTISP_SXYZW10		fourcc_code('M', 'B', '1', '0')

// TODO(crbug.com/958181): remove this definition once drm_fourcc.h contains it.
#ifndef DRM_FORMAT_P010
#define DRM_FORMAT_P010 fourcc_code('P', '0', '1', '0')
#endif

//TODO: remove this defination once drm_fourcc.h contains it.
#ifndef I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS
#define I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS fourcc_mod_code(INTEL, 6)
#endif

//TODO: remove this defination once drm_fourcc.h contains it.
#ifndef I915_FORMAT_MOD_4_TILED
#define I915_FORMAT_MOD_4_TILED         fourcc_mod_code(INTEL, 9)
#endif
// clang-format on
struct driver;
struct bo;
struct combination;

union bo_handle {
	void *ptr;
	int32_t s32;
	uint32_t u32;
	int64_t s64;
	uint64_t u64;
};

struct drv_import_fd_data {
	int fds[DRV_MAX_PLANES];
	uint32_t strides[DRV_MAX_PLANES];
	uint32_t offsets[DRV_MAX_PLANES];
	uint64_t format_modifier;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t tiling;
	uint64_t use_flags;
};

struct vma {
	void *addr;
	size_t length;
	uint32_t handle;
	uint32_t map_flags;
	int32_t refcount;
	uint32_t map_strides[DRV_MAX_PLANES];
	void *priv;
};

struct rectangle {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

struct mapping {
	struct vma *vma;
	struct rectangle rect;
	uint32_t refcount;
};

struct driver *drv_create(int fd);

void drv_destroy(struct driver *drv);

int drv_get_fd(struct driver *drv);

const char *drv_get_name(struct driver *drv);

struct combination *drv_get_combination(struct driver *drv, uint32_t format, uint64_t use_flags);

struct bo *drv_bo_new(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
		      uint64_t use_flags, bool is_test_buffer);

struct bo *drv_bo_create(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
			 uint64_t use_flags);

struct bo *drv_bo_create_with_modifiers(struct driver *drv, uint32_t width, uint32_t height,
					uint32_t format, const uint64_t *modifiers, uint32_t count);

void drv_bo_destroy(struct bo *bo);

struct bo *drv_bo_import(struct driver *drv, struct drv_import_fd_data *data);

void *drv_bo_map(struct bo *bo, const struct rectangle *rect, uint32_t map_flags,
		 struct mapping **map_data, size_t plane);

int drv_bo_unmap(struct bo *bo, struct mapping *mapping);

int drv_bo_invalidate(struct bo *bo, struct mapping *mapping);

int drv_bo_flush(struct bo *bo, struct mapping *mapping);

int drv_bo_flush_or_unmap(struct bo *bo, struct mapping *mapping);

uint32_t drv_bo_get_width(struct bo *bo);

uint32_t drv_bo_get_height(struct bo *bo);

size_t drv_bo_get_num_planes(struct bo *bo);

union bo_handle drv_bo_get_plane_handle(struct bo *bo, size_t plane);

int drv_bo_get_plane_fd(struct bo *bo, size_t plane);

uint32_t drv_bo_get_plane_offset(struct bo *bo, size_t plane);

uint32_t drv_bo_get_plane_size(struct bo *bo, size_t plane);

uint32_t drv_bo_get_plane_stride(struct bo *bo, size_t plane);

uint64_t drv_bo_get_format_modifier(struct bo *bo);

uint32_t drv_bo_get_format(struct bo *bo);

uint32_t drv_bo_get_tiling(struct bo *bo);

uint64_t drv_bo_get_use_flags(struct bo *bo);

size_t drv_bo_get_total_size(struct bo *bo);

uint32_t drv_get_standard_fourcc(uint32_t fourcc_internal);

uint32_t drv_bytes_per_pixel_from_format(uint32_t format, size_t plane);

uint32_t drv_stride_from_format(uint32_t format, uint32_t width, size_t plane);

void drv_resolve_format_and_use_flags(struct driver *drv, uint32_t format, uint64_t use_flags,
				      uint32_t *out_format, uint64_t *out_use_flags);

uint64_t drv_resolve_use_flags(struct driver *drv, uint32_t format, uint64_t use_flags);

size_t drv_num_planes_from_format(uint32_t format);

size_t drv_num_planes_from_modifier(struct driver *drv, uint32_t format, uint64_t modifier);

uint32_t drv_num_buffers_per_bo(struct bo *bo);

int drv_resource_info(struct bo *bo, uint32_t strides[DRV_MAX_PLANES],
		      uint32_t offsets[DRV_MAX_PLANES], uint64_t *format_modifier);

uint32_t drv_get_max_texture_2d_size(struct driver *drv);

enum drv_log_level {
	DRV_LOGV,
	DRV_LOGD,
	DRV_LOGI,
	DRV_LOGE,
};

#define _drv_log(level, format, ...)                                                               \
	do {                                                                                       \
		drv_log_prefix(level, "minigbm", __FILE__, __LINE__, format, ##__VA_ARGS__);       \
	} while (0)

#define drv_loge(format, ...) _drv_log(DRV_LOGE, format, ##__VA_ARGS__)
#define drv_logv(format, ...) _drv_log(DRV_LOGV, format, ##__VA_ARGS__)
#define drv_logd(format, ...) _drv_log(DRV_LOGD, format, ##__VA_ARGS__)
#define drv_logi(format, ...) _drv_log(DRV_LOGI, format, ##__VA_ARGS__)

__attribute__((format(printf, 5, 6))) void drv_log_prefix(enum drv_log_level level,
							  const char *prefix, const char *file,
							  int line, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
