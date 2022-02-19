/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_DRIVER_H
#define CROS_GRALLOC_DRIVER_H

#include "cros_gralloc_buffer.h"

#include <functional>
#include <mutex>
#include <unordered_map>

class cros_gralloc_driver
{
      public:
	static cros_gralloc_driver *get_instance();
	bool is_supported(const struct cros_gralloc_buffer_descriptor *descriptor);
	int32_t allocate(const struct cros_gralloc_buffer_descriptor *descriptor,
			 buffer_handle_t *out_handle);

	int32_t retain(buffer_handle_t handle);
	int32_t release(buffer_handle_t handle);

	int32_t lock(buffer_handle_t handle, int32_t acquire_fence, bool close_acquire_fence,
		     const struct rectangle *rect, uint32_t map_flags,
		     uint8_t *addr[DRV_MAX_PLANES]);
	int32_t unlock(buffer_handle_t handle, int32_t *release_fence);

	int32_t invalidate(buffer_handle_t handle);
	int32_t flush(buffer_handle_t handle, int32_t *release_fence);

	int32_t get_backing_store(buffer_handle_t handle, uint64_t *out_store);
	int32_t resource_info(buffer_handle_t handle, uint32_t strides[DRV_MAX_PLANES],
			      uint32_t offsets[DRV_MAX_PLANES], uint64_t *format_modifier);

	int32_t get_reserved_region(buffer_handle_t handle, void **reserved_region_addr,
				    uint64_t *reserved_region_size);

	uint32_t get_resolved_drm_format(uint32_t drm_format, uint64_t usage);

	void for_each_handle(const std::function<void(cros_gralloc_handle_t)> &function);

      private:
	cros_gralloc_driver();
	~cros_gralloc_driver();
	bool is_initialized();
	cros_gralloc_buffer *get_buffer(cros_gralloc_handle_t hnd);
	void emplace_buffer(struct bo *bo, struct cros_gralloc_handle *hnd);

	struct driver *drv_ = nullptr;
	std::mutex mutex_;
	std::unordered_map<uint32_t, cros_gralloc_buffer *> buffers_;
	std::unordered_map<cros_gralloc_handle_t, std::pair<cros_gralloc_buffer *, int32_t>>
	    handles_;
};

#endif
