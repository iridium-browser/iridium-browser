/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc_buffer.h"

#include <assert.h>
#include <sys/mman.h>

cros_gralloc_buffer::cros_gralloc_buffer(uint32_t id, struct bo *acquire_bo,
					 struct cros_gralloc_handle *acquire_handle,
					 int32_t reserved_region_fd, uint64_t reserved_region_size)
    : id_(id), bo_(acquire_bo), hnd_(acquire_handle), refcount_(1), lockcount_(0),
      reserved_region_fd_(reserved_region_fd), reserved_region_size_(reserved_region_size),
      reserved_region_addr_(nullptr)
{
	assert(bo_);
	num_planes_ = drv_bo_get_num_planes(bo_);
	for (uint32_t plane = 0; plane < num_planes_; plane++)
		lock_data_[plane] = nullptr;
}

cros_gralloc_buffer::~cros_gralloc_buffer()
{
	drv_bo_destroy(bo_);
	if (hnd_) {
		native_handle_close(hnd_);
		native_handle_delete(hnd_);
	}
	if (reserved_region_addr_) {
		munmap(reserved_region_addr_, reserved_region_size_);
	}
}

uint32_t cros_gralloc_buffer::get_id() const
{
	return id_;
}

int32_t cros_gralloc_buffer::increase_refcount()
{
	return ++refcount_;
}

int32_t cros_gralloc_buffer::decrease_refcount()
{
	assert(refcount_ > 0);
	return --refcount_;
}

int32_t cros_gralloc_buffer::lock(const struct rectangle *rect, uint32_t map_flags,
				  uint8_t *addr[DRV_MAX_PLANES])
{
	void *vaddr = nullptr;

	memset(addr, 0, DRV_MAX_PLANES * sizeof(*addr));

	/*
	 * Gralloc consumers don't support more than one kernel buffer per buffer object yet, so
	 * just use the first kernel buffer.
	 */
	if (drv_num_buffers_per_bo(bo_) != 1) {
		drv_log("Can only support one buffer per bo.\n");
		return -EINVAL;
	}

	if (map_flags) {
		if (lock_data_[0]) {
			drv_bo_invalidate(bo_, lock_data_[0]);
			vaddr = lock_data_[0]->vma->addr;
		} else {
			struct rectangle r = *rect;

			if (!r.width && !r.height && !r.x && !r.y) {
				/*
				 * Android IMapper.hal: An accessRegion of all-zeros means the
				 * entire buffer.
				 */
				r.width = drv_bo_get_width(bo_);
				r.height = drv_bo_get_height(bo_);
			}

			vaddr = drv_bo_map(bo_, &r, map_flags, &lock_data_[0], 0);
		}

		if (vaddr == MAP_FAILED) {
			drv_log("Mapping failed.\n");
			return -EFAULT;
		}
	}

	for (uint32_t plane = 0; plane < num_planes_; plane++)
		addr[plane] = static_cast<uint8_t *>(vaddr) + drv_bo_get_plane_offset(bo_, plane);

	lockcount_++;
	return 0;
}

int32_t cros_gralloc_buffer::unlock()
{
	if (lockcount_ <= 0) {
		drv_log("Buffer was not locked.\n");
		return -EINVAL;
	}

	if (!--lockcount_) {
		if (lock_data_[0]) {
			drv_bo_flush_or_unmap(bo_, lock_data_[0]);
			lock_data_[0] = nullptr;
		}
	}

	return 0;
}

int32_t cros_gralloc_buffer::resource_info(uint32_t strides[DRV_MAX_PLANES],
					   uint32_t offsets[DRV_MAX_PLANES],
					   uint64_t *format_modifier)
{
	return drv_resource_info(bo_, strides, offsets, format_modifier);
}

int32_t cros_gralloc_buffer::invalidate()
{
	if (lockcount_ <= 0) {
		drv_log("Buffer was not locked.\n");
		return -EINVAL;
	}

	if (lock_data_[0])
		return drv_bo_invalidate(bo_, lock_data_[0]);

	return 0;
}

int32_t cros_gralloc_buffer::flush()
{
	if (lockcount_ <= 0) {
		drv_log("Buffer was not locked.\n");
		return -EINVAL;
	}

	if (lock_data_[0])
		return drv_bo_flush(bo_, lock_data_[0]);

	return 0;
}

int32_t cros_gralloc_buffer::get_reserved_region(void **addr, uint64_t *size)
{
	if (reserved_region_fd_ <= 0) {
		drv_log("Buffer does not have reserved region.\n");
		return -EINVAL;
	}

	if (!reserved_region_addr_) {
		reserved_region_addr_ = mmap(nullptr, reserved_region_size_, PROT_WRITE | PROT_READ,
					     MAP_SHARED, reserved_region_fd_, 0);
		if (reserved_region_addr_ == MAP_FAILED) {
			drv_log("Failed to mmap reserved region: %s.\n", strerror(errno));
			return -errno;
		}
	}

	*addr = reserved_region_addr_;
	*size = reserved_region_size_;
	return 0;
}
