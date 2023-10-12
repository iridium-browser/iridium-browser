/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc_helpers.h"

#include <hardware/gralloc.h>
#include <sync/sync.h>

/* Define to match AIDL BufferUsage::VIDEO_DECODER. */
#define BUFFER_USAGE_VIDEO_DECODER (1 << 22)

/* Define to match AIDL BufferUsage::SENSOR_DIRECT_DATA. */
#define BUFFER_USAGE_SENSOR_DIRECT_DATA (1 << 23)

/* Define to match AIDL BufferUsage::GPU_DATA_BUFFER. */
#define BUFFER_USAGE_GPU_DATA_BUFFER (1 << 24)

/* Define to match AIDL PixelFormat::R_8. */
#define HAL_PIXEL_FORMAT_R8 0x38

uint32_t cros_gralloc_convert_format(int format)
{
	/*
	 * Conversion from HAL to fourcc-based DRV formats based on
	 * platform_android.c in mesa.
	 */

	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		return DRM_FORMAT_ABGR8888;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		return DRM_FORMAT_XBGR8888;
	case HAL_PIXEL_FORMAT_RGB_888:
		return DRM_FORMAT_BGR888;
	/*
	 * Confusingly, HAL_PIXEL_FORMAT_RGB_565 is defined as:
	 *
	 * "16-bit packed format that has 5-bit R, 6-bit G, and 5-bit B components, in that
	 *  order, from the  most-sigfinicant bits to the least-significant bits."
	 *
	 * so the order of the components is intentionally not flipped between the pixel
	 * format and the DRM format.
	 */
	case HAL_PIXEL_FORMAT_RGB_565:
		return DRM_FORMAT_RGB565;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		return DRM_FORMAT_ARGB8888;
	case HAL_PIXEL_FORMAT_RAW16:
		return DRM_FORMAT_R16;
	/*
	 * Choose DRM_FORMAT_R8 because <system/graphics.h> requires the buffers
	 * with a format HAL_PIXEL_FORMAT_BLOB have a height of 1, and width
	 * equal to their size in bytes.
	 */
	case HAL_PIXEL_FORMAT_BLOB:
	case HAL_PIXEL_FORMAT_R8:
		return DRM_FORMAT_R8;
	case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
		return DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED;
	case HAL_PIXEL_FORMAT_YCbCr_420_888:
		return DRM_FORMAT_FLEX_YCbCr_420_888;
	case HAL_PIXEL_FORMAT_Y8:
		return DRM_FORMAT_R8;
	case HAL_PIXEL_FORMAT_Y16:
		return DRM_FORMAT_R16;
	case HAL_PIXEL_FORMAT_YV12:
		return DRM_FORMAT_YVU420_ANDROID;
#if ANDROID_API_LEVEL >= 29
	case HAL_PIXEL_FORMAT_RGBA_FP16:
		return DRM_FORMAT_ABGR16161616F;
	case HAL_PIXEL_FORMAT_RGBA_1010102:
		return DRM_FORMAT_ABGR2101010;
#endif
#if ANDROID_API_LEVEL >= 30
	case HAL_PIXEL_FORMAT_YCBCR_P010:
		return DRM_FORMAT_P010;
#endif
	}

	return DRM_FORMAT_NONE;
}

static inline void handle_usage(uint64_t *gralloc_usage, uint64_t gralloc_mask,
				uint64_t *bo_use_flags, uint64_t bo_mask)
{
	if ((*gralloc_usage) & gralloc_mask) {
		(*gralloc_usage) &= ~gralloc_mask;
		(*bo_use_flags) |= bo_mask;
	}
}

uint64_t cros_gralloc_convert_usage(uint64_t usage)
{
	uint64_t use_flags = BO_USE_NONE;

	/*
	 * GRALLOC_USAGE_SW_READ_OFTEN contains GRALLOC_USAGE_SW_READ_RARELY, thus OFTEN must be
	 * handled first. The same applies to GRALLOC_USAGE_SW_WRITE_OFTEN.
	 */
	handle_usage(&usage, GRALLOC_USAGE_SW_READ_OFTEN, &use_flags, BO_USE_SW_READ_OFTEN);
	handle_usage(&usage, GRALLOC_USAGE_SW_READ_RARELY, &use_flags, BO_USE_SW_READ_RARELY);
	handle_usage(&usage, GRALLOC_USAGE_SW_WRITE_OFTEN, &use_flags, BO_USE_SW_WRITE_OFTEN);
	handle_usage(&usage, GRALLOC_USAGE_SW_WRITE_RARELY, &use_flags, BO_USE_SW_WRITE_RARELY);
	handle_usage(&usage, GRALLOC_USAGE_HW_TEXTURE, &use_flags, BO_USE_TEXTURE);
	handle_usage(&usage, GRALLOC_USAGE_HW_RENDER, &use_flags, BO_USE_RENDERING);
	handle_usage(&usage, GRALLOC_USAGE_HW_2D, &use_flags, BO_USE_RENDERING);
	/* HWC wants to use display hardware, but can defer to OpenGL. */
	handle_usage(&usage, GRALLOC_USAGE_HW_COMPOSER, &use_flags,
		     BO_USE_SCANOUT | BO_USE_TEXTURE);
	handle_usage(&usage, GRALLOC_USAGE_HW_FB, &use_flags, BO_USE_NONE);
	/*
	 * This flag potentially covers external display for the normal drivers (i915/rockchip) and
	 * usb monitors (evdi/udl). It's complicated so ignore it.
	 */
	handle_usage(&usage, GRALLOC_USAGE_EXTERNAL_DISP, &use_flags, BO_USE_NONE);
	/* Map PROTECTED to linear until real HW protection is available on Android. */
	handle_usage(&usage, GRALLOC_USAGE_PROTECTED, &use_flags, BO_USE_LINEAR);
	handle_usage(&usage, GRALLOC_USAGE_CURSOR, &use_flags, BO_USE_NONE);
	/* HACK: See b/30054495 for BO_USE_SW_READ_OFTEN. */
	handle_usage(&usage, GRALLOC_USAGE_HW_VIDEO_ENCODER, &use_flags,
		     BO_USE_HW_VIDEO_ENCODER | BO_USE_SW_READ_OFTEN);
	handle_usage(&usage, GRALLOC_USAGE_HW_CAMERA_WRITE, &use_flags, BO_USE_CAMERA_WRITE);
	handle_usage(&usage, GRALLOC_USAGE_HW_CAMERA_READ, &use_flags, BO_USE_CAMERA_READ);
	handle_usage(&usage, GRALLOC_USAGE_RENDERSCRIPT, &use_flags, BO_USE_RENDERSCRIPT);
	handle_usage(&usage, BUFFER_USAGE_VIDEO_DECODER, &use_flags, BO_USE_HW_VIDEO_DECODER);
	handle_usage(&usage, BUFFER_USAGE_SENSOR_DIRECT_DATA, &use_flags,
		     BO_USE_SENSOR_DIRECT_DATA);
	handle_usage(&usage, BUFFER_USAGE_GPU_DATA_BUFFER, &use_flags, BO_USE_GPU_DATA_BUFFER);
	handle_usage(&usage, BUFFER_USAGE_FRONT_RENDERING_MASK, &use_flags, BO_USE_FRONT_RENDERING);

	if (usage) {
		ALOGE("Unhandled gralloc usage: %llx", (unsigned long long)usage);
		return BO_USE_NONE;
	}

	return use_flags;
}

uint32_t cros_gralloc_convert_map_usage(uint64_t usage)
{
	uint32_t map_flags = BO_MAP_NONE;

	if (usage & GRALLOC_USAGE_SW_READ_MASK)
		map_flags |= BO_MAP_READ;
	if (usage & GRALLOC_USAGE_SW_WRITE_MASK)
		map_flags |= BO_MAP_WRITE;

	return map_flags;
}

cros_gralloc_handle_t cros_gralloc_convert_handle(buffer_handle_t handle)
{
	auto hnd = reinterpret_cast<cros_gralloc_handle_t>(handle);
	if (!hnd || hnd->magic != cros_gralloc_magic)
		return nullptr;

	return hnd;
}

int32_t cros_gralloc_sync_wait(int32_t fence, bool close_fence)
{
	if (fence < 0)
		return 0;

	/*
	 * Wait initially for 1000 ms, and then wait indefinitely. The SYNC_IOC_WAIT
	 * documentation states the caller waits indefinitely on the fence if timeout < 0.
	 */
	int err = sync_wait(fence, 1000);
	if (err < 0) {
		ALOGE("Timed out on sync wait, err = %s", strerror(errno));
		err = sync_wait(fence, -1);
		if (err < 0) {
			ALOGE("sync wait error = %s", strerror(errno));
			return -errno;
		}
	}

	if (close_fence) {
		err = close(fence);
		if (err) {
			ALOGE("Unable to close fence fd, err = %s", strerror(errno));
			return -errno;
		}
	}

	return 0;
}

std::string get_drm_format_string(uint32_t drm_format)
{
	char *sequence = (char *)&drm_format;
	std::string s(sequence, 4);
	return "DRM_FOURCC_" + s;
}
