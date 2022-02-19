/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <android/hardware/graphics/allocator/3.0/IAllocator.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>

#include "cros_gralloc/cros_gralloc_driver.h"

class CrosGralloc3Allocator : public android::hardware::graphics::allocator::V3_0::IAllocator {
  public:
    CrosGralloc3Allocator() = default;

    android::hardware::Return<void> allocate(
            const android::hardware::hidl_vec<uint32_t>& descriptor, uint32_t count,
            allocate_cb hidl_cb) override;

    android::hardware::graphics::mapper::V3_0::Error init();

    android::hardware::Return<void> dumpDebugInfo(dumpDebugInfo_cb hidl_cb) override;

  private:
    android::hardware::graphics::mapper::V3_0::Error allocate(
            const android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo&
                    description,
            uint32_t* outStride, android::hardware::hidl_handle* outHandle);

    cros_gralloc_driver* mDriver = nullptr;
};
