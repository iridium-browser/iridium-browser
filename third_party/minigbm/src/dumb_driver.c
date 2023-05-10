/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "drv_helpers.h"
#include "drv_priv.h"
#include "util.h"

#define INIT_DUMB_DRIVER(driver)                                                                   \
	const struct backend backend_##driver = {                                                  \
		.name = #driver,                                                                   \
		.init = dumb_driver_init,                                                          \
		.bo_create = drv_dumb_bo_create,                                                   \
		.bo_create_with_modifiers = dumb_bo_create_with_modifiers,                         \
		.bo_destroy = drv_dumb_bo_destroy,                                                 \
		.bo_import = drv_prime_bo_import,                                                  \
		.bo_map = drv_dumb_bo_map,                                                         \
		.bo_unmap = drv_bo_munmap,                                                         \
		.resolve_format_and_use_flags = drv_resolve_format_and_use_flags_helper,           \
	};

static const uint32_t scanout_render_formats[] = { DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888,
						   DRM_FORMAT_ABGR8888, DRM_FORMAT_XBGR8888,
						   DRM_FORMAT_BGR888,	DRM_FORMAT_RGB565 };

static const uint32_t texture_only_formats[] = { DRM_FORMAT_R8, DRM_FORMAT_NV12, DRM_FORMAT_NV21,
						 DRM_FORMAT_YVU420, DRM_FORMAT_YVU420_ANDROID };

static int dumb_driver_init(struct driver *drv)
{
	drv_add_combinations(drv, scanout_render_formats, ARRAY_SIZE(scanout_render_formats),
			     &LINEAR_METADATA, BO_USE_RENDER_MASK | BO_USE_SCANOUT);

	drv_add_combinations(drv, texture_only_formats, ARRAY_SIZE(texture_only_formats),
			     &LINEAR_METADATA, BO_USE_TEXTURE_MASK);

	drv_modify_combination(drv, DRM_FORMAT_R8, &LINEAR_METADATA,
			       BO_USE_HW_VIDEO_ENCODER | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE |
				   BO_USE_GPU_DATA_BUFFER | BO_USE_SENSOR_DIRECT_DATA);
	drv_modify_combination(drv, DRM_FORMAT_NV12, &LINEAR_METADATA,
			       BO_USE_HW_VIDEO_ENCODER | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE);
	drv_modify_combination(drv, DRM_FORMAT_NV21, &LINEAR_METADATA, BO_USE_HW_VIDEO_ENCODER);

	return drv_modify_linear_combinations(drv);
}

static int dumb_bo_create_with_modifiers(struct bo *bo, uint32_t width, uint32_t height,
					 uint32_t format, const uint64_t *modifiers, uint32_t count)
{
	for (uint32_t i = 0; i < count; i++) {
		if (modifiers[i] == DRM_FORMAT_MOD_LINEAR) {
			return drv_dumb_bo_create(bo, width, height, format, 0);
		}
	}

	return -EINVAL;
}

INIT_DUMB_DRIVER(evdi)
INIT_DUMB_DRIVER(komeda)
INIT_DUMB_DRIVER(marvell)
INIT_DUMB_DRIVER(meson)
INIT_DUMB_DRIVER(nouveau)
INIT_DUMB_DRIVER(radeon)
INIT_DUMB_DRIVER(synaptics)
INIT_DUMB_DRIVER(udl)
INIT_DUMB_DRIVER(vkms)
