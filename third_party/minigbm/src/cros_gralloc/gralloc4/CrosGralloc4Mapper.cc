/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc/gralloc4/CrosGralloc4Mapper.h"

#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/Dataspace.h>
#include <aidl/android/hardware/graphics/common/PlaneLayout.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>

#include "cros_gralloc/cros_gralloc_helpers.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Utils.h"

#include "helpers.h"

using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::Rect;
using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using android::hardware::graphics::mapper::V4_0::Error;
using android::hardware::graphics::mapper::V4_0::IMapper;

Return<void> CrosGralloc4Mapper::createDescriptor(const BufferDescriptorInfo& description,
                                                  createDescriptor_cb hidlCb) {
    hidl_vec<uint8_t> descriptor;

    if (description.width == 0) {
        drv_log("Failed to createDescriptor. Bad width: %d.\n", description.width);
        hidlCb(Error::BAD_VALUE, descriptor);
        return Void();
    }

    if (description.height == 0) {
        drv_log("Failed to createDescriptor. Bad height: %d.\n", description.height);
        hidlCb(Error::BAD_VALUE, descriptor);
        return Void();
    }

    if (description.layerCount == 0) {
        drv_log("Failed to createDescriptor. Bad layer count: %d.\n", description.layerCount);
        hidlCb(Error::BAD_VALUE, descriptor);
        return Void();
    }

    int ret = android::gralloc4::encodeBufferDescriptorInfo(description, &descriptor);
    if (ret) {
        drv_log("Failed to createDescriptor. Failed to encode: %d.\n", ret);
        hidlCb(Error::BAD_VALUE, descriptor);
        return Void();
    }

    hidlCb(Error::NONE, descriptor);
    return Void();
}

Return<void> CrosGralloc4Mapper::importBuffer(const hidl_handle& handle, importBuffer_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to import buffer. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    const native_handle_t* bufferHandle = handle.getNativeHandle();
    if (!bufferHandle || bufferHandle->numFds == 0) {
        drv_log("Failed to importBuffer. Bad handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    native_handle_t* importedBufferHandle = native_handle_clone(bufferHandle);
    if (!importedBufferHandle) {
        drv_log("Failed to importBuffer. Handle clone failed: %s.\n", strerror(errno));
        hidlCb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    int ret = mDriver->retain(importedBufferHandle);
    if (ret) {
        native_handle_close(importedBufferHandle);
        native_handle_delete(importedBufferHandle);
        hidlCb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    hidlCb(Error::NONE, importedBufferHandle);
    return Void();
}

Return<Error> CrosGralloc4Mapper::freeBuffer(void* rawHandle) {
    if (!mDriver) {
        drv_log("Failed to freeBuffer. Driver is uninitialized.\n");
        return Error::NO_RESOURCES;
    }

    native_handle_t* bufferHandle = reinterpret_cast<native_handle_t*>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to freeBuffer. Empty handle.\n");
        return Error::BAD_BUFFER;
    }

    int ret = mDriver->release(bufferHandle);
    if (ret) {
        return Error::BAD_BUFFER;
    }

    native_handle_close(bufferHandle);
    native_handle_delete(bufferHandle);
    return Error::NONE;
}

Return<Error> CrosGralloc4Mapper::validateBufferSize(void* rawHandle,
                                                     const BufferDescriptorInfo& descriptor,
                                                     uint32_t stride) {
    if (!mDriver) {
        drv_log("Failed to validateBufferSize. Driver is uninitialized.\n");
        return Error::NO_RESOURCES;
    }

    native_handle_t* bufferHandle = reinterpret_cast<native_handle_t*>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to validateBufferSize. Empty handle.\n");
        return Error::BAD_BUFFER;
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (!crosHandle) {
        drv_log("Failed to validateBufferSize. Invalid handle.\n");
        return Error::BAD_BUFFER;
    }

    PixelFormat crosHandleFormat = static_cast<PixelFormat>(crosHandle->droid_format);
    if (descriptor.format != crosHandleFormat) {
        drv_log("Failed to validateBufferSize. Format mismatch.\n");
        return Error::BAD_BUFFER;
    }

    if (descriptor.width != crosHandle->width) {
        drv_log("Failed to validateBufferSize. Width mismatch (%d vs %d).\n", descriptor.width,
                crosHandle->width);
        return Error::BAD_VALUE;
    }

    if (descriptor.height != crosHandle->height) {
        drv_log("Failed to validateBufferSize. Height mismatch (%d vs %d).\n", descriptor.height,
                crosHandle->height);
        return Error::BAD_VALUE;
    }

    if (stride != crosHandle->pixel_stride) {
        drv_log("Failed to validateBufferSize. Stride mismatch (%d vs %d).\n", stride,
                crosHandle->pixel_stride);
        return Error::BAD_VALUE;
    }

    return Error::NONE;
}

Return<void> CrosGralloc4Mapper::getTransportSize(void* rawHandle, getTransportSize_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to getTransportSize. Driver is uninitialized.\n");
        hidlCb(Error::BAD_BUFFER, 0, 0);
        return Void();
    }

    native_handle_t* bufferHandle = reinterpret_cast<native_handle_t*>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to getTransportSize. Bad handle.\n");
        hidlCb(Error::BAD_BUFFER, 0, 0);
        return Void();
    }

    // No local process data is currently stored on the native handle.
    hidlCb(Error::NONE, bufferHandle->numFds, bufferHandle->numInts);
    return Void();
}

Return<void> CrosGralloc4Mapper::lock(void* rawBuffer, uint64_t cpuUsage, const Rect& region,
                                      const hidl_handle& acquireFence, lock_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to lock. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawBuffer);
    if (!bufferHandle) {
        drv_log("Failed to lock. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    if (cpuUsage == 0) {
        drv_log("Failed to lock. Bad cpu usage: %" PRIu64 ".\n", cpuUsage);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    uint32_t mapUsage = 0;
    int ret = convertToMapUsage(cpuUsage, &mapUsage);
    if (ret) {
        drv_log("Failed to lock. Convert usage failed.\n");
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (crosHandle == nullptr) {
        drv_log("Failed to lock. Invalid handle.\n");
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.left < 0) {
        drv_log("Failed to lock. Invalid region: negative left value %d.\n", region.left);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.top < 0) {
        drv_log("Failed to lock. Invalid region: negative top value %d.\n", region.top);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.width < 0) {
        drv_log("Failed to lock. Invalid region: negative width value %d.\n", region.width);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.height < 0) {
        drv_log("Failed to lock. Invalid region: negative height value %d.\n", region.height);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.width > crosHandle->width) {
        drv_log("Failed to lock. Invalid region: width greater than buffer width (%d vs %d).\n",
                region.width, crosHandle->width);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    if (region.height > crosHandle->height) {
        drv_log("Failed to lock. Invalid region: height greater than buffer height (%d vs %d).\n",
                region.height, crosHandle->height);
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    struct rectangle rect = {static_cast<uint32_t>(region.left), static_cast<uint32_t>(region.top),
                             static_cast<uint32_t>(region.width),
                             static_cast<uint32_t>(region.height)};

    // An access region of all zeros means the entire buffer.
    if (rect.x == 0 && rect.y == 0 && rect.width == 0 && rect.height == 0) {
        rect.width = crosHandle->width;
        rect.height = crosHandle->height;
    }

    int acquireFenceFd = -1;
    ret = convertToFenceFd(acquireFence, &acquireFenceFd);
    if (ret) {
        drv_log("Failed to lock. Bad acquire fence.\n");
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    uint8_t* addr[DRV_MAX_PLANES];
    ret = mDriver->lock(bufferHandle, acquireFenceFd, /*close_acquire_fence=*/false, &rect,
                        mapUsage, addr);
    if (ret) {
        hidlCb(Error::BAD_VALUE, nullptr);
        return Void();
    }

    hidlCb(Error::NONE, addr[0]);
    return Void();
}

Return<void> CrosGralloc4Mapper::unlock(void* rawHandle, unlock_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to unlock. Driver is uninitialized.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to unlock. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    int releaseFenceFd = -1;
    int ret = mDriver->unlock(bufferHandle, &releaseFenceFd);
    if (ret) {
        drv_log("Failed to unlock.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    hidl_handle releaseFenceHandle;
    ret = convertToFenceHandle(releaseFenceFd, &releaseFenceHandle);
    if (ret) {
        drv_log("Failed to unlock. Failed to convert release fence to handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    hidlCb(Error::NONE, releaseFenceHandle);
    return Void();
}

Return<void> CrosGralloc4Mapper::flushLockedBuffer(void* rawHandle, flushLockedBuffer_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to flushLockedBuffer. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to flushLockedBuffer. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    int releaseFenceFd = -1;
    int ret = mDriver->flush(bufferHandle, &releaseFenceFd);
    if (ret) {
        drv_log("Failed to flushLockedBuffer. Flush failed.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    hidl_handle releaseFenceHandle;
    ret = convertToFenceHandle(releaseFenceFd, &releaseFenceHandle);
    if (ret) {
        drv_log("Failed to flushLockedBuffer. Failed to convert release fence to handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr);
        return Void();
    }

    hidlCb(Error::NONE, releaseFenceHandle);
    return Void();
}

Return<Error> CrosGralloc4Mapper::rereadLockedBuffer(void* rawHandle) {
    if (!mDriver) {
        drv_log("Failed to rereadLockedBuffer. Driver is uninitialized.\n");
        return Error::NO_RESOURCES;
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to rereadLockedBuffer. Empty handle.\n");
        return Error::BAD_BUFFER;
    }

    int ret = mDriver->invalidate(bufferHandle);
    if (ret) {
        drv_log("Failed to rereadLockedBuffer. Failed to invalidate.\n");
        return Error::BAD_BUFFER;
    }

    return Error::NONE;
}

Return<void> CrosGralloc4Mapper::isSupported(const BufferDescriptorInfo& descriptor,
                                             isSupported_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to isSupported. Driver is uninitialized.\n");
        hidlCb(Error::BAD_VALUE, false);
        return Void();
    }

    struct cros_gralloc_buffer_descriptor crosDescriptor;
    if (convertToCrosDescriptor(descriptor, &crosDescriptor)) {
        hidlCb(Error::NONE, false);
        return Void();
    }

    bool supported = mDriver->is_supported(&crosDescriptor);
    if (!supported) {
        crosDescriptor.use_flags &= ~BO_USE_SCANOUT;
        supported = mDriver->is_supported(&crosDescriptor);
    }

    hidlCb(Error::NONE, supported);
    return Void();
}

Return<void> CrosGralloc4Mapper::get(void* rawHandle, const MetadataType& metadataType,
                                     get_cb hidlCb) {
    hidl_vec<uint8_t> encodedMetadata;

    if (!mDriver) {
        drv_log("Failed to get. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, encodedMetadata);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to get. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, encodedMetadata);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (!crosHandle) {
        drv_log("Failed to get. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, encodedMetadata);
        return Void();
    }

    get(crosHandle, metadataType, hidlCb);
    return Void();
}

Return<void> CrosGralloc4Mapper::get(cros_gralloc_handle_t crosHandle,
                                     const MetadataType& metadataType, get_cb hidlCb) {
    hidl_vec<uint8_t> encodedMetadata;

    if (!mDriver) {
        drv_log("Failed to get. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, encodedMetadata);
        return Void();
    }

    if (!crosHandle) {
        drv_log("Failed to get. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, encodedMetadata);
        return Void();
    }

    android::status_t status = android::NO_ERROR;
    if (metadataType == android::gralloc4::MetadataType_BufferId) {
        status = android::gralloc4::encodeBufferId(crosHandle->id, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Name) {
        const char* name = (const char*)(&crosHandle->data[crosHandle->name_offset]);
        status = android::gralloc4::encodeName(name, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Width) {
        status = android::gralloc4::encodeWidth(crosHandle->width, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Height) {
        status = android::gralloc4::encodeHeight(crosHandle->height, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_LayerCount) {
        status = android::gralloc4::encodeLayerCount(1, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatRequested) {
        PixelFormat pixelFormat = static_cast<PixelFormat>(crosHandle->droid_format);
        status = android::gralloc4::encodePixelFormatRequested(pixelFormat, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatFourCC) {
        status = android::gralloc4::encodePixelFormatFourCC(
                drv_get_standard_fourcc(crosHandle->format), &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatModifier) {
        status = android::gralloc4::encodePixelFormatModifier(crosHandle->format_modifier,
                                                              &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Usage) {
        uint64_t usage = static_cast<uint64_t>(crosHandle->usage);
        status = android::gralloc4::encodeUsage(usage, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_AllocationSize) {
        status = android::gralloc4::encodeAllocationSize(crosHandle->total_size, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_ProtectedContent) {
        uint64_t hasProtectedContent = crosHandle->usage & BufferUsage::PROTECTED ? 1 : 0;
        status = android::gralloc4::encodeProtectedContent(hasProtectedContent, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Compression) {
        status = android::gralloc4::encodeCompression(android::gralloc4::Compression_None,
                                                      &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Interlaced) {
        status = android::gralloc4::encodeInterlaced(android::gralloc4::Interlaced_None,
                                                     &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_ChromaSiting) {
        status = android::gralloc4::encodeChromaSiting(android::gralloc4::ChromaSiting_None,
                                                       &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PlaneLayouts) {
        std::vector<PlaneLayout> planeLayouts;
        getPlaneLayouts(crosHandle->format, &planeLayouts);

        for (size_t plane = 0; plane < planeLayouts.size(); plane++) {
            PlaneLayout& planeLayout = planeLayouts[plane];
            planeLayout.offsetInBytes = crosHandle->offsets[plane];
            planeLayout.strideInBytes = crosHandle->strides[plane];
            planeLayout.totalSizeInBytes = crosHandle->sizes[plane];
            planeLayout.widthInSamples = crosHandle->width / planeLayout.horizontalSubsampling;
            planeLayout.heightInSamples = crosHandle->height / planeLayout.verticalSubsampling;
        }

        status = android::gralloc4::encodePlaneLayouts(planeLayouts, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Crop) {
        std::vector<aidl::android::hardware::graphics::common::Rect> crops;
        for (size_t plane = 0; plane < crosHandle->num_planes; plane++) {
            aidl::android::hardware::graphics::common::Rect crop;
            crop.left = 0;
            crop.top = 0;
            crop.right = crosHandle->width;
            crop.bottom = crosHandle->height;
            crops.push_back(crop);
        }

        status = android::gralloc4::encodeCrop(crops, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Dataspace) {
        status = android::gralloc4::encodeDataspace(Dataspace::UNKNOWN, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_BlendMode) {
        status = android::gralloc4::encodeBlendMode(BlendMode::INVALID, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Smpte2086) {
        status = android::gralloc4::encodeSmpte2086(std::nullopt, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Cta861_3) {
        status = android::gralloc4::encodeCta861_3(std::nullopt, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Smpte2094_40) {
        status = android::gralloc4::encodeSmpte2094_40(std::nullopt, &encodedMetadata);
    } else {
        hidlCb(Error::UNSUPPORTED, encodedMetadata);
        return Void();
    }

    if (status != android::NO_ERROR) {
        hidlCb(Error::NO_RESOURCES, encodedMetadata);
        drv_log("Failed to get. Failed to encode metadata.\n");
        return Void();
    }

    hidlCb(Error::NONE, encodedMetadata);
    return Void();
}

Return<Error> CrosGralloc4Mapper::set(void* rawHandle, const MetadataType& metadataType,
                                      const hidl_vec<uint8_t>& /*metadata*/) {
    if (!mDriver) {
        drv_log("Failed to set. Driver is uninitialized.\n");
        return Error::NO_RESOURCES;
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to set. Empty handle.\n");
        return Error::BAD_BUFFER;
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (!crosHandle) {
        drv_log("Failed to set. Invalid handle.\n");
        return Error::BAD_BUFFER;
    }

    if (metadataType == android::gralloc4::MetadataType_BufferId) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_Name) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_Width) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_Height) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_LayerCount) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatRequested) {
        return Error::BAD_VALUE;
    } else if (metadataType == android::gralloc4::MetadataType_Usage) {
        return Error::BAD_VALUE;
    }

    return Error::UNSUPPORTED;
}

int CrosGralloc4Mapper::getResolvedDrmFormat(PixelFormat pixelFormat, uint64_t bufferUsage,
                                             uint32_t* outDrmFormat) {
    uint32_t drmFormat;
    if (convertToDrmFormat(pixelFormat, &drmFormat)) {
        std::string pixelFormatString = getPixelFormatString(pixelFormat);
        drv_log("Failed to getResolvedDrmFormat. Failed to convert format %s\n",
                pixelFormatString.c_str());
        return -EINVAL;
    }

    uint64_t usage;
    if (convertToBufferUsage(bufferUsage, &usage)) {
        std::string usageString = getUsageString(bufferUsage);
        drv_log("Failed to getResolvedDrmFormat. Failed to convert usage %s\n",
                usageString.c_str());
        return -EINVAL;
    }

    uint32_t resolvedDrmFormat = mDriver->get_resolved_drm_format(drmFormat, usage);
    if (resolvedDrmFormat == DRM_FORMAT_INVALID) {
        std::string drmFormatString = get_drm_format_string(drmFormat);
        drv_log("Failed to getResolvedDrmFormat. Failed to resolve drm format %s\n",
                drmFormatString.c_str());
        return -EINVAL;
    }

    *outDrmFormat = resolvedDrmFormat;

    return 0;
}

Return<void> CrosGralloc4Mapper::getFromBufferDescriptorInfo(
        const BufferDescriptorInfo& descriptor, const MetadataType& metadataType,
        getFromBufferDescriptorInfo_cb hidlCb) {
    hidl_vec<uint8_t> encodedMetadata;

    if (!mDriver) {
        drv_log("Failed to getFromBufferDescriptorInfo. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, encodedMetadata);
        return Void();
    }

    android::status_t status = android::NO_ERROR;
    if (metadataType == android::gralloc4::MetadataType_Name) {
        status = android::gralloc4::encodeName(descriptor.name, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Width) {
        status = android::gralloc4::encodeWidth(descriptor.width, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Height) {
        status = android::gralloc4::encodeHeight(descriptor.height, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_LayerCount) {
        status = android::gralloc4::encodeLayerCount(1, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatRequested) {
        status = android::gralloc4::encodePixelFormatRequested(descriptor.format, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_PixelFormatFourCC) {
        uint32_t drmFormat;
        if (getResolvedDrmFormat(descriptor.format, descriptor.usage, &drmFormat)) {
            hidlCb(Error::BAD_VALUE, encodedMetadata);
            return Void();
        }
        status = android::gralloc4::encodePixelFormatFourCC(drv_get_standard_fourcc(drmFormat),
                                                            &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Usage) {
        status = android::gralloc4::encodeUsage(descriptor.usage, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_ProtectedContent) {
        uint64_t hasProtectedContent = descriptor.usage & BufferUsage::PROTECTED ? 1 : 0;
        status = android::gralloc4::encodeProtectedContent(hasProtectedContent, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Compression) {
        status = android::gralloc4::encodeCompression(android::gralloc4::Compression_None,
                                                      &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Interlaced) {
        status = android::gralloc4::encodeInterlaced(android::gralloc4::Interlaced_None,
                                                     &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_ChromaSiting) {
        status = android::gralloc4::encodeChromaSiting(android::gralloc4::ChromaSiting_None,
                                                       &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Crop) {
        uint32_t drmFormat;
        if (getResolvedDrmFormat(descriptor.format, descriptor.usage, &drmFormat)) {
            hidlCb(Error::BAD_VALUE, encodedMetadata);
            return Void();
        }

        size_t numPlanes = drv_num_planes_from_format(drmFormat);

        std::vector<aidl::android::hardware::graphics::common::Rect> crops;
        for (size_t plane = 0; plane < numPlanes; plane++) {
            aidl::android::hardware::graphics::common::Rect crop;
            crop.left = 0;
            crop.top = 0;
            crop.right = descriptor.width;
            crop.bottom = descriptor.height;
            crops.push_back(crop);
        }
        status = android::gralloc4::encodeCrop(crops, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Dataspace) {
        status = android::gralloc4::encodeDataspace(Dataspace::UNKNOWN, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_BlendMode) {
        status = android::gralloc4::encodeBlendMode(BlendMode::INVALID, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Smpte2086) {
        status = android::gralloc4::encodeSmpte2086(std::nullopt, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Cta861_3) {
        status = android::gralloc4::encodeCta861_3(std::nullopt, &encodedMetadata);
    } else if (metadataType == android::gralloc4::MetadataType_Smpte2094_40) {
        status = android::gralloc4::encodeSmpte2094_40(std::nullopt, &encodedMetadata);
    } else {
        hidlCb(Error::UNSUPPORTED, encodedMetadata);
        return Void();
    }

    if (status != android::NO_ERROR) {
        hidlCb(Error::NO_RESOURCES, encodedMetadata);
        return Void();
    }

    hidlCb(Error::NONE, encodedMetadata);
    return Void();
}

Return<void> CrosGralloc4Mapper::listSupportedMetadataTypes(listSupportedMetadataTypes_cb hidlCb) {
    hidl_vec<MetadataTypeDescription> supported;

    if (!mDriver) {
        drv_log("Failed to listSupportedMetadataTypes. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, supported);
        return Void();
    }

    supported = hidl_vec<IMapper::MetadataTypeDescription>({
            {
                    android::gralloc4::MetadataType_BufferId,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Name,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Width,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Height,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_LayerCount,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_PixelFormatRequested,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_PixelFormatFourCC,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_PixelFormatModifier,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Usage,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_AllocationSize,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_ProtectedContent,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Compression,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Interlaced,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_ChromaSiting,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_PlaneLayouts,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Dataspace,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_BlendMode,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Smpte2086,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Cta861_3,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
            {
                    android::gralloc4::MetadataType_Smpte2094_40,
                    "",
                    /*isGettable=*/true,
                    /*isSettable=*/false,
            },
    });

    hidlCb(Error::NONE, supported);
    return Void();
}

Return<void> CrosGralloc4Mapper::dumpBuffer(void* rawHandle, dumpBuffer_cb hidlCb) {
    BufferDump bufferDump;

    if (!mDriver) {
        drv_log("Failed to dumpBuffer. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, bufferDump);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to dumpBuffer. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, bufferDump);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (!crosHandle) {
        drv_log("Failed to dumpBuffer. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, bufferDump);
        return Void();
    }

    return dumpBuffer(crosHandle, hidlCb);
}

Return<void> CrosGralloc4Mapper::dumpBuffer(cros_gralloc_handle_t crosHandle,
                                            dumpBuffer_cb hidlCb) {
    BufferDump bufferDump;

    if (!mDriver) {
        drv_log("Failed to dumpBuffer. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, bufferDump);
        return Void();
    }

    if (!crosHandle) {
        drv_log("Failed to dumpBuffer. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, bufferDump);
        return Void();
    }

    std::vector<MetadataDump> metadataDumps;

    MetadataType metadataType = android::gralloc4::MetadataType_BufferId;
    auto metadata_get_callback = [&](Error, hidl_vec<uint8_t> metadata) {
        MetadataDump metadataDump;
        metadataDump.metadataType = metadataType;
        metadataDump.metadata = metadata;
        metadataDumps.push_back(metadataDump);
    };

    metadataType = android::gralloc4::MetadataType_BufferId;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Name;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Width;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Height;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_LayerCount;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_PixelFormatRequested;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_PixelFormatFourCC;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_PixelFormatModifier;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Usage;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_AllocationSize;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_ProtectedContent;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Compression;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Interlaced;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_ChromaSiting;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_PlaneLayouts;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_Dataspace;
    get(crosHandle, metadataType, metadata_get_callback);

    metadataType = android::gralloc4::MetadataType_BlendMode;
    get(crosHandle, metadataType, metadata_get_callback);

    bufferDump.metadataDump = metadataDumps;
    hidlCb(Error::NONE, bufferDump);
    return Void();
}

Return<void> CrosGralloc4Mapper::dumpBuffers(dumpBuffers_cb hidlCb) {
    std::vector<BufferDump> bufferDumps;

    if (!mDriver) {
        drv_log("Failed to dumpBuffers. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, bufferDumps);
        return Void();
    }

    Error error = Error::NONE;

    auto handleCallback = [&](cros_gralloc_handle_t crosHandle) {
        auto dumpBufferCallback = [&](Error err, BufferDump bufferDump) {
            error = err;
            if (error == Error::NONE) {
                bufferDumps.push_back(bufferDump);
            }
        };

        dumpBuffer(crosHandle, dumpBufferCallback);
    };
    mDriver->for_each_handle(handleCallback);

    hidlCb(error, bufferDumps);
    return Void();
}

Return<void> CrosGralloc4Mapper::getReservedRegion(void* rawHandle, getReservedRegion_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to getReservedRegion. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, nullptr, 0);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to getReservedRegion. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr, 0);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (!crosHandle) {
        drv_log("Failed to getReservedRegion. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr, 0);
        return Void();
    }

    void* reservedRegionAddr = nullptr;
    uint64_t reservedRegionSize = 0;
    int ret = mDriver->get_reserved_region(bufferHandle, &reservedRegionAddr, &reservedRegionSize);
    if (ret) {
        drv_log("Failed to getReservedRegion.\n");
        hidlCb(Error::BAD_BUFFER, nullptr, 0);
        return Void();
    }

    hidlCb(Error::NONE, reservedRegionAddr, reservedRegionSize);
    return Void();
}

android::hardware::graphics::mapper::V4_0::IMapper* HIDL_FETCH_IMapper(const char* /*name*/) {
    return static_cast<android::hardware::graphics::mapper::V4_0::IMapper*>(new CrosGralloc4Mapper);
}
