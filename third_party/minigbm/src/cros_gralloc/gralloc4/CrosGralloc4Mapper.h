/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include "cros_gralloc/cros_gralloc_driver.h"
#include "cros_gralloc/cros_gralloc_handle.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Metadata.h"

class CrosGralloc4Mapper : public android::hardware::graphics::mapper::V4_0::IMapper {
  public:
    CrosGralloc4Mapper() = default;

    android::hardware::Return<void> createDescriptor(const BufferDescriptorInfo& description,
                                                     createDescriptor_cb hidlCb) override;

    android::hardware::Return<void> importBuffer(const android::hardware::hidl_handle& rawHandle,
                                                 importBuffer_cb hidlCb) override;

    android::hardware::Return<android::hardware::graphics::mapper::V4_0::Error> freeBuffer(
            void* rawHandle) override;

    android::hardware::Return<android::hardware::graphics::mapper::V4_0::Error> validateBufferSize(
            void* rawHandle, const BufferDescriptorInfo& descriptor, uint32_t stride) override;

    android::hardware::Return<void> getTransportSize(void* rawHandle,
                                                     getTransportSize_cb hidlCb) override;

    android::hardware::Return<void> lock(void* rawHandle, uint64_t cpuUsage,
                                         const Rect& accessRegion,
                                         const android::hardware::hidl_handle& acquireFence,
                                         lock_cb hidlCb) override;

    android::hardware::Return<void> unlock(void* rawHandle, unlock_cb hidlCb) override;

    android::hardware::Return<void> flushLockedBuffer(void* rawHandle,
                                                      flushLockedBuffer_cb hidlCb) override;

    android::hardware::Return<android::hardware::graphics::mapper::V4_0::Error> rereadLockedBuffer(
            void* rawHandle) override;

    android::hardware::Return<void> isSupported(const BufferDescriptorInfo& descriptor,
                                                isSupported_cb hidlCb) override;

    android::hardware::Return<void> get(void* rawHandle, const MetadataType& metadataType,
                                        get_cb hidlCb) override;

    android::hardware::Return<android::hardware::graphics::mapper::V4_0::Error> set(
            void* rawHandle, const MetadataType& metadataType,
            const android::hardware::hidl_vec<uint8_t>& metadata) override;

    android::hardware::Return<void> getFromBufferDescriptorInfo(
            const BufferDescriptorInfo& descriptor, const MetadataType& metadataType,
            getFromBufferDescriptorInfo_cb hidlCb) override;

    android::hardware::Return<void> listSupportedMetadataTypes(
            listSupportedMetadataTypes_cb hidlCb) override;

    android::hardware::Return<void> dumpBuffer(void* rawHandle, dumpBuffer_cb hidlCb) override;
    android::hardware::Return<void> dumpBuffers(dumpBuffers_cb hidlCb) override;

    android::hardware::Return<void> getReservedRegion(void* rawHandle,
                                                      getReservedRegion_cb hidlCb) override;

  private:
    enum class ReservedRegionArea {
        /* CrosGralloc4Metadata */
        MAPPER4_METADATA,

        /* External user metadata */
        USER_METADATA,
    };

    android::hardware::graphics::mapper::V4_0::Error getReservedRegionArea(
            const cros_gralloc_buffer* crosBuffer, ReservedRegionArea area, void** outAddr,
            uint64_t* outSize);

    android::hardware::graphics::mapper::V4_0::Error getMetadata(
            const cros_gralloc_buffer* crosBuffer, const CrosGralloc4Metadata** outMetadata);

    android::hardware::graphics::mapper::V4_0::Error getMutableMetadata(
            cros_gralloc_buffer* crosBuffer, CrosGralloc4Metadata** outMetadata);

    android::hardware::Return<void> get(const cros_gralloc_buffer* crosBuffer,
                                        const MetadataType& metadataType, get_cb hidlCb);

    android::hardware::graphics::mapper::V4_0::Error set(
            cros_gralloc_buffer* crosBuffer, const MetadataType& metadataType,
            const android::hardware::hidl_vec<uint8_t>& metadata);

    android::hardware::Return<void> dumpBuffer(const cros_gralloc_buffer* crosBuffer,
                                               dumpBuffer_cb hidlCb);

    int getResolvedDrmFormat(android::hardware::graphics::common::V1_2::PixelFormat pixelFormat,
                             uint64_t bufferUsage, uint32_t* outDrmFormat);

    cros_gralloc_driver* mDriver = cros_gralloc_driver::get_instance();
};

extern "C" android::hardware::graphics::mapper::V4_0::IMapper* HIDL_FETCH_IMapper(const char* name);
