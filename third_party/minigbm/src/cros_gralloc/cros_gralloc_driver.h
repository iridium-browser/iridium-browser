/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_DRIVER_H
#define CROS_GRALLOC_DRIVER_H

#include "cros_gralloc_buffer.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#if ANDROID_API_LEVEL >= 31 && defined(HAS_DMABUF_SYSTEM_HEAP)
#include <BufferAllocator/BufferAllocator.h>
#endif

class cros_gralloc_driver
{
      public:
	static cros_gralloc_driver *get_instance();
	bool is_supported(const struct cros_gralloc_buffer_descriptor *descriptor);
	int32_t allocate(const struct cros_gralloc_buffer_descriptor *descriptor,
			 native_handle_t **out_handle);

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

	uint32_t get_resolved_drm_format(uint32_t drm_format, uint64_t use_flags);

	void with_buffer(cros_gralloc_handle_t hnd,
			 const std::function<void(cros_gralloc_buffer *)> &function);
	void with_each_buffer(const std::function<void(cros_gralloc_buffer *)> &function);

      private:
	cros_gralloc_driver();
	~cros_gralloc_driver();
	bool is_initialized();
	cros_gralloc_buffer *get_buffer(cros_gralloc_handle_t hnd);
	bool
	get_resolved_format_and_use_flags(const struct cros_gralloc_buffer_descriptor *descriptor,
					  uint32_t *out_format, uint64_t *out_use_flags);

	int create_reserved_region(const std::string &buffer_name, uint64_t reserved_region_size);

#if ANDROID_API_LEVEL >= 31 && defined(HAS_DMABUF_SYSTEM_HEAP)
	/* For allocating cros_gralloc_buffer reserved regions for metadata. */
	BufferAllocator allocator_;
#endif

	std::unique_ptr<struct driver, void (*)(struct driver *)> drv_;

	struct cros_gralloc_imported_handle_info {
		/*
		 * The underlying buffer for referred to by this handle (as multiple handles can
		 * refer to the same buffer).
		 */
		cros_gralloc_buffer *buffer = nullptr;

		/* The handle's refcount as a handle can be imported multiple times.*/
		int32_t refcount = 1;
	};

	std::mutex mutex_;
	std::unordered_map<uint32_t, std::unique_ptr<cros_gralloc_buffer>> buffers_;
	std::unordered_map<cros_gralloc_handle_t, cros_gralloc_imported_handle_info> handles_;
};

#endif
