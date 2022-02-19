/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <android/hardware/graphics/mapper/3.0/IMapper.h>

#include <optional>

#include "cros_gralloc/cros_gralloc_driver.h"
#include "cros_gralloc/cros_gralloc_handle.h"

class CrosGralloc3Mapper : public android::hardware::graphics::mapper::V3_0::IMapper {
  public:
    CrosGralloc3Mapper() = default;

    android::hardware::Return<void> createDescriptor(const BufferDescriptorInfo& description,
                                                     createDescriptor_cb hidlCb) override;

    android::hardware::Return<void> importBuffer(const android::hardware::hidl_handle& rawHandle,
                                                 importBuffer_cb hidlCb) override;

    android::hardware::Return<android::hardware::graphics::mapper::V3_0::Error> freeBuffer(
            void* rawHandle) override;

    android::hardware::Return<android::hardware::graphics::mapper::V3_0::Error> validateBufferSize(
            void* rawHandle, const BufferDescriptorInfo& descriptor, uint32_t stride) override;

    android::hardware::Return<void> getTransportSize(void* rawHandle,
                                                     getTransportSize_cb hidlCb) override;

    android::hardware::Return<void> lock(void* rawHandle, uint64_t cpuUsage,
                                         const Rect& accessRegion,
                                         const android::hardware::hidl_handle& acquireFence,
                                         lock_cb hidlCb) override;

    android::hardware::Return<void> lockYCbCr(void* rawHandle, uint64_t cpuUsage,
                                              const Rect& accessRegion,
                                              const android::hardware::hidl_handle& acquireFence,
                                              lockYCbCr_cb _hidl_cb) override;

    android::hardware::Return<void> unlock(void* rawHandle, unlock_cb hidlCb) override;

    android::hardware::Return<void> isSupported(const BufferDescriptorInfo& descriptor,
                                                isSupported_cb hidlCb) override;

  private:
    int getResolvedDrmFormat(android::hardware::graphics::common::V1_2::PixelFormat pixelFormat,
                             uint64_t bufferUsage, uint32_t* outDrmFormat);

    struct LockResult {
        android::hardware::graphics::mapper::V3_0::Error error;

        uint8_t* mapped[DRV_MAX_PLANES];
    };
    LockResult lockInternal(cros_gralloc_handle_t crosHandle, uint64_t cpuUsage,
                            const Rect& accessRegion,
                            const android::hardware::hidl_handle& acquireFence);

    cros_gralloc_driver* mDriver = cros_gralloc_driver::get_instance();
};

extern "C" android::hardware::graphics::mapper::V3_0::IMapper* HIDL_FETCH_IMapper(const char* name);
