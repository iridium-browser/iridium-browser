/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc_buffer.h"

#include <assert.h>
#include <sys/mman.h>

#include <cutils/native_handle.h>

/*static*/
std::unique_ptr<cros_gralloc_buffer>
cros_gralloc_buffer::create(struct bo *acquire_bo,
			    const struct cros_gralloc_handle *borrowed_handle)
{
	auto acquire_hnd =
	    reinterpret_cast<struct cros_gralloc_handle *>(native_handle_clone(borrowed_handle));
	if (!acquire_hnd) {
		ALOGE("Failed to create cros_gralloc_buffer: failed to clone handle.");
		return {};
	}

	std::unique_ptr<cros_gralloc_buffer> buffer(
	    new cros_gralloc_buffer(acquire_bo, acquire_hnd));
	if (!buffer) {
		ALOGE("Failed to create cros_gralloc_buffer: failed to allocate.");
		native_handle_close(acquire_hnd);
		native_handle_delete(acquire_hnd);
		return {};
	}

	return buffer;
}

cros_gralloc_buffer::cros_gralloc_buffer(struct bo *acquire_bo,
					 struct cros_gralloc_handle *acquire_handle)
    : bo_(acquire_bo), hnd_(acquire_handle)
{
	assert(bo_);
	assert(hnd_);
	for (uint32_t plane = 0; plane < DRV_MAX_PLANES; plane++)
		lock_data_[plane] = nullptr;
}

cros_gralloc_buffer::~cros_gralloc_buffer()
{
	drv_bo_destroy(bo_);
	if (reserved_region_addr_) {
		munmap(reserved_region_addr_, hnd_->reserved_region_size);
	}
	native_handle_close(hnd_);
	native_handle_delete(hnd_);
}

uint32_t cros_gralloc_buffer::get_id() const
{
	return hnd_->id;
}

uint32_t cros_gralloc_buffer::get_width() const
{
	return hnd_->width;
}

uint32_t cros_gralloc_buffer::get_height() const
{
	return hnd_->height;
}

uint32_t cros_gralloc_buffer::get_format() const
{
	return hnd_->format;
}

uint64_t cros_gralloc_buffer::get_format_modifier() const
{
	return hnd_->format_modifier;
}

uint64_t cros_gralloc_buffer::get_total_size() const
{
	return hnd_->total_size;
}

uint32_t cros_gralloc_buffer::get_num_planes() const
{
	return hnd_->num_planes;
}

uint32_t cros_gralloc_buffer::get_plane_offset(uint32_t plane) const
{
	return hnd_->offsets[plane];
}

uint32_t cros_gralloc_buffer::get_plane_stride(uint32_t plane) const
{
	return hnd_->strides[plane];
}

uint32_t cros_gralloc_buffer::get_plane_size(uint32_t plane) const
{
	return hnd_->sizes[plane];
}

int32_t cros_gralloc_buffer::get_android_format() const
{
	return hnd_->droid_format;
}

uint64_t cros_gralloc_buffer::get_android_usage() const
{
	return static_cast<uint64_t>(hnd_->usage);
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
		ALOGE("Can only support one buffer per bo.");
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
			ALOGE("Mapping failed.");
			return -EFAULT;
		}
	}

	for (uint32_t plane = 0; plane < hnd_->num_planes; plane++)
		addr[plane] = static_cast<uint8_t *>(vaddr) + drv_bo_get_plane_offset(bo_, plane);

	lockcount_++;
	return 0;
}

int32_t cros_gralloc_buffer::unlock()
{
	if (lockcount_ <= 0) {
		ALOGE("Buffer was not locked.");
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
		ALOGE("Buffer was not locked.");
		return -EINVAL;
	}

	if (lock_data_[0])
		return drv_bo_invalidate(bo_, lock_data_[0]);

	return 0;
}

int32_t cros_gralloc_buffer::flush()
{
	if (lockcount_ <= 0) {
		ALOGE("Buffer was not locked.");
		return -EINVAL;
	}

	if (lock_data_[0])
		return drv_bo_flush(bo_, lock_data_[0]);

	return 0;
}

int32_t cros_gralloc_buffer::get_reserved_region(void **addr, uint64_t *size) const
{
	int32_t reserved_region_fd = hnd_->fds[hnd_->num_planes];
	if (reserved_region_fd < 0) {
		ALOGE("Buffer does not have reserved region.");
		return -EINVAL;
	}

	if (!reserved_region_addr_) {
		reserved_region_addr_ =
		    mmap(nullptr, hnd_->reserved_region_size, PROT_WRITE | PROT_READ, MAP_SHARED,
			 reserved_region_fd, 0);
		if (reserved_region_addr_ == MAP_FAILED) {
			ALOGE("Failed to mmap reserved region: %s.", strerror(errno));
			return -errno;
		}
	}

	*addr = reserved_region_addr_;
	*size = hnd_->reserved_region_size;
	return 0;
}
