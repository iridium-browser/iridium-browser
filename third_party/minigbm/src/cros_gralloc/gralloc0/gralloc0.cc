/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "../../util.h"
#include "../cros_gralloc_driver.h"

#include <cassert>
#include <cutils/native_handle.h>
#include <hardware/gralloc.h>
#include <memory.h>

struct gralloc0_module {
	gralloc_module_t base;
	std::unique_ptr<alloc_device_t> alloc;
	cros_gralloc_driver *driver;
	bool initialized;
	std::mutex initialization_mutex;
};

struct cros_gralloc0_buffer_info {
	uint32_t drm_fourcc;
	int num_fds;
	int fds[4];
	uint64_t modifier;
	uint32_t offset[4];
	uint32_t stride[4];
};

/* This enumeration must match the one in <gralloc_drm.h>.
 * The functions supported by this gralloc's temporary private API are listed
 * below. Use of these functions is highly discouraged and should only be
 * reserved for cases where no alternative to get same information (such as
 * querying ANativeWindow) exists.
 */
// clang-format off
enum {
	GRALLOC_DRM_GET_STRIDE,
	GRALLOC_DRM_GET_FORMAT,
	GRALLOC_DRM_GET_DIMENSIONS,
	GRALLOC_DRM_GET_BACKING_STORE,
	GRALLOC_DRM_GET_BUFFER_INFO,
	GRALLOC_DRM_GET_USAGE,
};

/* This enumeration corresponds to the GRALLOC_DRM_GET_USAGE query op, which
 * defines a set of bit flags used by the client to query vendor usage bits.
 *
 * Here is the common flow:
 * 1) EGL/Vulkan calls GRALLOC_DRM_GET_USAGE to append one or multiple vendor
 *    usage bits to the existing usage and sets onto the ANativeWindow.
 * 2) Some implicit GL draw cmd or the explicit vkCreateSwapchainKHR kicks off
 *    the next dequeueBuffer on the ANativeWindow with the combined usage.
 * 3) dequeueBuffer then asks gralloc hal for an allocation/re-allocation, and
 *    calls into the below `gralloc0_alloc(...)` api.
 */
enum {
	GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT = 0x00000001,
};
// clang-format on

static int gralloc0_droid_yuv_format(int droid_format)
{
	return (droid_format == HAL_PIXEL_FORMAT_YCbCr_420_888 ||
		droid_format == HAL_PIXEL_FORMAT_YV12);
}

static int gralloc0_alloc(alloc_device_t *dev, int w, int h, int format, int usage,
			  buffer_handle_t *out_handle, int *out_stride)
{
	int32_t ret;
	native_handle_t *handle;
	struct cros_gralloc_buffer_descriptor descriptor;
	auto mod = (struct gralloc0_module const *)dev->common.module;

	descriptor.width = w;
	descriptor.height = h;
	descriptor.droid_format = format;
	descriptor.droid_usage = usage;
	descriptor.drm_format = cros_gralloc_convert_format(format);
	descriptor.use_flags = cros_gralloc_convert_usage(usage);
	descriptor.reserved_region_size = 0;

	if (!mod->driver->is_supported(&descriptor)) {
		ALOGE("Unsupported combination -- HAL format: %u, HAL usage: %u, "
		      "drv_format: %4.4s, use_flags: %llu",
		      format, usage, reinterpret_cast<char *>(&descriptor.drm_format),
		      static_cast<unsigned long long>(descriptor.use_flags));
		return -EINVAL;
	}

	ret = mod->driver->allocate(&descriptor, &handle);
	if (ret)
		return ret;

	auto hnd = cros_gralloc_convert_handle(handle);
	*out_handle = handle;
	*out_stride = hnd->pixel_stride;

	return 0;
}

static int gralloc0_free(alloc_device_t *dev, buffer_handle_t handle)
{
	int32_t ret;
	auto mod = (struct gralloc0_module const *)dev->common.module;

	ret = mod->driver->release(handle);
	if (ret)
		return ret;

	auto hnd = const_cast<native_handle_t *>(handle);
	native_handle_close(hnd);
	native_handle_delete(hnd);

	return 0;
}

static int gralloc0_close(struct hw_device_t *dev)
{
	/* Memory is freed by managed pointers on process close. */
	return 0;
}

static int gralloc0_init(struct gralloc0_module *mod, bool initialize_alloc)
{
	std::lock_guard<std::mutex> lock(mod->initialization_mutex);

	if (mod->initialized)
		return 0;

	mod->driver = cros_gralloc_driver::get_instance();
	if (!mod->driver)
		return -ENODEV;

	if (initialize_alloc) {
		mod->alloc = std::make_unique<alloc_device_t>();
		mod->alloc->alloc = gralloc0_alloc;
		mod->alloc->free = gralloc0_free;
		mod->alloc->common.tag = HARDWARE_DEVICE_TAG;
		mod->alloc->common.version = 0;
		mod->alloc->common.module = (hw_module_t *)mod;
		mod->alloc->common.close = gralloc0_close;
	}

	mod->initialized = true;
	return 0;
}

static int gralloc0_open(const struct hw_module_t *mod, const char *name, struct hw_device_t **dev)
{
	auto const_module = reinterpret_cast<const struct gralloc0_module *>(mod);
	auto module = const_cast<struct gralloc0_module *>(const_module);

	if (module->initialized) {
		*dev = &module->alloc->common;
		return 0;
	}

	if (strcmp(name, GRALLOC_HARDWARE_GPU0)) {
		ALOGE("Incorrect device name - %s.", name);
		return -EINVAL;
	}

	if (gralloc0_init(module, true))
		return -ENODEV;

	*dev = &module->alloc->common;
	return 0;
}

static int gralloc0_register_buffer(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	auto const_module = reinterpret_cast<const struct gralloc0_module *>(module);
	auto mod = const_cast<struct gralloc0_module *>(const_module);

	if (!mod->initialized) {
		if (gralloc0_init(mod, false))
			return -ENODEV;
	}

	return mod->driver->retain(handle);
}

static int gralloc0_unregister_buffer(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	auto mod = (struct gralloc0_module const *)module;
	return mod->driver->release(handle);
}

static int gralloc0_lock(struct gralloc_module_t const *module, buffer_handle_t handle, int usage,
			 int l, int t, int w, int h, void **vaddr)
{
	return module->lockAsync(module, handle, usage, l, t, w, h, vaddr, -1);
}

static int gralloc0_unlock(struct gralloc_module_t const *module, buffer_handle_t handle)
{
	int32_t fence_fd, ret;
	auto mod = (struct gralloc0_module const *)module;
	ret = mod->driver->unlock(handle, &fence_fd);
	if (ret)
		return ret;

	ret = cros_gralloc_sync_wait(fence_fd, /*close_acquire_fence=*/true);
	if (ret)
		return ret;

	return 0;
}

static int gralloc0_perform(struct gralloc_module_t const *module, int op, ...)
{
	va_list args;
	int32_t *out_format, ret;
	uint64_t *out_store;
	buffer_handle_t handle;
	cros_gralloc_handle_t hnd;
	uint32_t *out_width, *out_height, *out_stride;
	uint32_t strides[DRV_MAX_PLANES] = { 0, 0, 0, 0 };
	uint32_t offsets[DRV_MAX_PLANES] = { 0, 0, 0, 0 };
	uint64_t format_modifier = 0;
	struct cros_gralloc0_buffer_info *info;
	auto const_module = reinterpret_cast<const struct gralloc0_module *>(module);
	auto mod = const_cast<struct gralloc0_module *>(const_module);
	uint32_t req_usage;
	uint32_t gralloc_usage = 0;
	uint32_t *out_gralloc_usage;

	if (!mod->initialized) {
		if (gralloc0_init(mod, false))
			return -ENODEV;
	}

	va_start(args, op);

	switch (op) {
	case GRALLOC_DRM_GET_STRIDE:
	case GRALLOC_DRM_GET_FORMAT:
	case GRALLOC_DRM_GET_DIMENSIONS:
	case GRALLOC_DRM_GET_BACKING_STORE:
	case GRALLOC_DRM_GET_BUFFER_INFO:
		/* retrieve handles for ops with buffer_handle_t */
		handle = va_arg(args, buffer_handle_t);
		hnd = cros_gralloc_convert_handle(handle);
		if (!hnd) {
			va_end(args);
			ALOGE("Invalid handle.");
			return -EINVAL;
		}
		break;
	case GRALLOC_DRM_GET_USAGE:
		break;
	default:
		va_end(args);
		return -EINVAL;
	}

	ret = 0;
	switch (op) {
	case GRALLOC_DRM_GET_STRIDE:
		out_stride = va_arg(args, uint32_t *);
		ret = mod->driver->resource_info(handle, strides, offsets, &format_modifier);
		if (ret)
			break;

		if (strides[0] != hnd->strides[0]) {
			uint32_t bytes_per_pixel = drv_bytes_per_pixel_from_format(hnd->format, 0);
			*out_stride = DIV_ROUND_UP(strides[0], bytes_per_pixel);
		} else {
			*out_stride = hnd->pixel_stride;
		}

		break;
	case GRALLOC_DRM_GET_FORMAT:
		out_format = va_arg(args, int32_t *);
		*out_format = hnd->droid_format;
		break;
	case GRALLOC_DRM_GET_DIMENSIONS:
		out_width = va_arg(args, uint32_t *);
		out_height = va_arg(args, uint32_t *);
		*out_width = hnd->width;
		*out_height = hnd->height;
		break;
	case GRALLOC_DRM_GET_BACKING_STORE:
		out_store = va_arg(args, uint64_t *);
		ret = mod->driver->get_backing_store(handle, out_store);
		break;
	case GRALLOC_DRM_GET_BUFFER_INFO:
		info = va_arg(args, struct cros_gralloc0_buffer_info *);
		memset(info, 0, sizeof(*info));
		info->drm_fourcc = drv_get_standard_fourcc(hnd->format);
		info->num_fds = hnd->num_planes;
		for (int i = 0; i < info->num_fds; i++)
			info->fds[i] = hnd->fds[i];

		ret = mod->driver->resource_info(handle, strides, offsets, &format_modifier);
		if (ret)
			break;

		info->modifier = format_modifier ? format_modifier : hnd->format_modifier;
		for (uint32_t i = 0; i < DRV_MAX_PLANES; i++) {
			if (!strides[i])
				break;

			info->stride[i] = strides[i];
			info->offset[i] = offsets[i];
		}
		break;
	case GRALLOC_DRM_GET_USAGE:
		req_usage = va_arg(args, uint32_t);
		out_gralloc_usage = va_arg(args, uint32_t *);
		if (req_usage & GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT)
			gralloc_usage |= BUFFER_USAGE_FRONT_RENDERING;
		*out_gralloc_usage = gralloc_usage;
		break;
	default:
		ret = -EINVAL;
	}

	va_end(args);

	return ret;
}

static int gralloc0_lock_ycbcr(struct gralloc_module_t const *module, buffer_handle_t handle,
			       int usage, int l, int t, int w, int h, struct android_ycbcr *ycbcr)
{
	return module->lockAsync_ycbcr(module, handle, usage, l, t, w, h, ycbcr, -1);
}

static int gralloc0_lock_async(struct gralloc_module_t const *module, buffer_handle_t handle,
			       int usage, int l, int t, int w, int h, void **vaddr, int fence_fd)
{
	int32_t ret;
	uint32_t map_flags;
	uint8_t *addr[DRV_MAX_PLANES];
	auto const_module = reinterpret_cast<const struct gralloc0_module *>(module);
	auto mod = const_cast<struct gralloc0_module *>(const_module);
	struct rectangle rect = { .x = static_cast<uint32_t>(l),
				  .y = static_cast<uint32_t>(t),
				  .width = static_cast<uint32_t>(w),
				  .height = static_cast<uint32_t>(h) };

	if (!mod->initialized) {
		if (gralloc0_init(mod, false))
			return -ENODEV;
	}

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		ALOGE("Invalid handle.");
		return -EINVAL;
	}

	if (hnd->droid_format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
		ALOGE("HAL_PIXEL_FORMAT_YCbCr_*_888 format not compatible.");
		return -EINVAL;
	}

	assert(l >= 0);
	assert(t >= 0);
	assert(w >= 0);
	assert(h >= 0);

	map_flags = cros_gralloc_convert_map_usage(static_cast<uint64_t>(usage));
	ret = mod->driver->lock(handle, fence_fd, true, &rect, map_flags, addr);
	*vaddr = addr[0];
	return ret;
}

static int gralloc0_unlock_async(struct gralloc_module_t const *module, buffer_handle_t handle,
				 int *fence_fd)
{
	auto mod = (struct gralloc0_module const *)module;
	return mod->driver->unlock(handle, fence_fd);
}

static int gralloc0_lock_async_ycbcr(struct gralloc_module_t const *module, buffer_handle_t handle,
				     int usage, int l, int t, int w, int h,
				     struct android_ycbcr *ycbcr, int fence_fd)
{
	int32_t ret;
	uint32_t map_flags;
	uint32_t strides[DRV_MAX_PLANES] = { 0, 0, 0, 0 };
	uint32_t offsets[DRV_MAX_PLANES] = { 0, 0, 0, 0 };
	uint64_t format_modifier = 0;
	uint8_t *addr[DRV_MAX_PLANES] = { nullptr, nullptr, nullptr, nullptr };
	auto const_module = reinterpret_cast<const struct gralloc0_module *>(module);
	auto mod = const_cast<struct gralloc0_module *>(const_module);
	struct rectangle rect = { .x = static_cast<uint32_t>(l),
				  .y = static_cast<uint32_t>(t),
				  .width = static_cast<uint32_t>(w),
				  .height = static_cast<uint32_t>(h) };

	if (!mod->initialized) {
		if (gralloc0_init(mod, false))
			return -ENODEV;
	}

	auto hnd = cros_gralloc_convert_handle(handle);
	if (!hnd) {
		ALOGE("Invalid handle.");
		return -EINVAL;
	}

	if (!gralloc0_droid_yuv_format(hnd->droid_format) &&
	    hnd->droid_format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
		ALOGE("Non-YUV format not compatible.");
		return -EINVAL;
	}

	assert(l >= 0);
	assert(t >= 0);
	assert(w >= 0);
	assert(h >= 0);

	map_flags = cros_gralloc_convert_map_usage(static_cast<uint64_t>(usage));
	ret = mod->driver->lock(handle, fence_fd, true, &rect, map_flags, addr);
	if (ret)
		return ret;

	if (!map_flags) {
		ret = mod->driver->resource_info(handle, strides, offsets, &format_modifier);
		if (ret)
			return ret;

		for (uint32_t plane = 0; plane < DRV_MAX_PLANES; plane++)
			addr[plane] =
			    reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(offsets[plane]));
	}

	switch (hnd->format) {
	case DRM_FORMAT_NV12:
		ycbcr->y = addr[0];
		ycbcr->cb = addr[1];
		ycbcr->cr = addr[1] + 1;
		ycbcr->ystride = (!map_flags) ? strides[0] : hnd->strides[0];
		ycbcr->cstride = (!map_flags) ? strides[1] : hnd->strides[1];
		ycbcr->chroma_step = 2;
		break;
	case DRM_FORMAT_YVU420:
	case DRM_FORMAT_YVU420_ANDROID:
		ycbcr->y = addr[0];
		ycbcr->cb = addr[2];
		ycbcr->cr = addr[1];
		ycbcr->ystride = (!map_flags) ? strides[0] : hnd->strides[0];
		ycbcr->cstride = (!map_flags) ? strides[1] : hnd->strides[1];
		ycbcr->chroma_step = 1;
		break;
	default:
		module->unlock(module, handle);
		return -EINVAL;
	}

	return 0;
}

// clang-format off
static struct hw_module_methods_t gralloc0_module_methods = { .open = gralloc0_open };
// clang-format on

struct gralloc0_module HAL_MODULE_INFO_SYM = {
	.base =
	    {
		.common =
		    {
			.tag = HARDWARE_MODULE_TAG,
			.module_api_version = GRALLOC_MODULE_API_VERSION_0_3,
			.hal_api_version = 0,
			.id = GRALLOC_HARDWARE_MODULE_ID,
			.name = "CrOS Gralloc",
			.author = "Chrome OS",
			.methods = &gralloc0_module_methods,
		    },

		.registerBuffer = gralloc0_register_buffer,
		.unregisterBuffer = gralloc0_unregister_buffer,
		.lock = gralloc0_lock,
		.unlock = gralloc0_unlock,
		.perform = gralloc0_perform,
		.lock_ycbcr = gralloc0_lock_ycbcr,
		.lockAsync = gralloc0_lock_async,
		.unlockAsync = gralloc0_unlock_async,
		.lockAsync_ycbcr = gralloc0_lock_async_ycbcr,
	    },

	.alloc = nullptr,
	.driver = nullptr,
	.initialized = false,
};
