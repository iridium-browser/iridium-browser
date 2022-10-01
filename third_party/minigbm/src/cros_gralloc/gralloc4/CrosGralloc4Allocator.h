/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <android/hardware/graphics/allocator/4.0/IAllocator.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include "cros_gralloc/cros_gralloc_driver.h"
#include "cros_gralloc/cros_gralloc_helpers.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Metadata.h"

class CrosGralloc4Allocator : public android::hardware::graphics::allocator::V4_0::IAllocator {
  public:
    CrosGralloc4Allocator() = default;

    android::hardware::Return<void> allocate(const android::hardware::hidl_vec<uint8_t>& descriptor,
                                             uint32_t count, allocate_cb hidl_cb) override;

    android::hardware::graphics::mapper::V4_0::Error init();

  private:
    android::hardware::graphics::mapper::V4_0::Error initializeMetadata(
            cros_gralloc_handle_t crosHandle,
            const struct cros_gralloc_buffer_descriptor& crosDescriptor);

    android::hardware::graphics::mapper::V4_0::Error allocate(
            const android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo&
                    description,
            uint32_t* outStride, android::hardware::hidl_handle* outHandle);

    cros_gralloc_driver* mDriver = nullptr;
};
