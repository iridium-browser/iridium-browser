/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc/gralloc3/CrosGralloc3Mapper.h"

#include <cutils/native_handle.h>

#include "cros_gralloc/cros_gralloc_helpers.h"
#include "cros_gralloc/gralloc3/CrosGralloc3Utils.h"

#include "helpers.h"

using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using android::hardware::graphics::mapper::V3_0::Error;
using android::hardware::graphics::mapper::V3_0::IMapper;
using android::hardware::graphics::mapper::V3_0::YCbCrLayout;

Return<void> CrosGralloc3Mapper::createDescriptor(const BufferDescriptorInfo& description,
                                                  createDescriptor_cb hidlCb) {
    hidl_vec<uint32_t> descriptor;

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

    auto descriptor_opt = encodeBufferDescriptorInfo(description);
    if (!descriptor_opt) {
        drv_log("Failed to createDescriptor. Failed to encodeBufferDescriptorInfo\n");
        hidlCb(Error::BAD_VALUE, descriptor);
        return Void();
    }

    descriptor = *descriptor_opt;
    hidlCb(Error::NONE, descriptor);
    return Void();
}

Return<void> CrosGralloc3Mapper::importBuffer(const hidl_handle& handle, importBuffer_cb hidlCb) {
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
        drv_log("Failed to importBuffer. Handle clone failed.\n");
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

Return<Error> CrosGralloc3Mapper::freeBuffer(void* rawHandle) {
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
        drv_log("Failed to freeBuffer.\n");
        return Error::BAD_BUFFER;
    }

    native_handle_close(bufferHandle);
    native_handle_delete(bufferHandle);
    return Error::NONE;
}

Return<Error> CrosGralloc3Mapper::validateBufferSize(void* rawHandle,
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

Return<void> CrosGralloc3Mapper::getTransportSize(void* rawHandle, getTransportSize_cb hidlCb) {
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

Return<void> CrosGralloc3Mapper::lock(void* rawHandle, uint64_t cpuUsage, const Rect& accessRegion,
                                      const hidl_handle& acquireFence, lock_cb hidlCb) {
    if (!mDriver) {
        drv_log("Failed to lock. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, nullptr, 0, 0);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to lock. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr, 0, 0);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (crosHandle == nullptr) {
        drv_log("Failed to lock. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, nullptr, 0, 0);
        return Void();
    }

    LockResult result = lockInternal(crosHandle, cpuUsage, accessRegion, acquireFence);
    if (result.error != Error::NONE) {
        drv_log("Failed to lock. Failed to lockInternal.\n");
        hidlCb(result.error, nullptr, 0, 0);
        return Void();
    }

    int32_t bytesPerPixel = drv_bytes_per_pixel_from_format(crosHandle->format, 0);
    int32_t bytesPerStride = static_cast<int32_t>(crosHandle->strides[0]);

    hidlCb(Error::NONE, result.mapped[0], bytesPerPixel, bytesPerStride);
    return Void();
}

Return<void> CrosGralloc3Mapper::lockYCbCr(void* rawHandle, uint64_t cpuUsage,
                                           const Rect& accessRegion,
                                           const android::hardware::hidl_handle& acquireFence,
                                           lockYCbCr_cb hidlCb) {
    YCbCrLayout ycbcr = {};

    if (!mDriver) {
        drv_log("Failed to lock. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, ycbcr);
        return Void();
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(rawHandle);
    if (!bufferHandle) {
        drv_log("Failed to lockYCbCr. Empty handle.\n");
        hidlCb(Error::BAD_BUFFER, ycbcr);
        return Void();
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(bufferHandle);
    if (crosHandle == nullptr) {
        drv_log("Failed to lockYCbCr. Invalid handle.\n");
        hidlCb(Error::BAD_BUFFER, ycbcr);
        return Void();
    }

    LockResult result = lockInternal(crosHandle, cpuUsage, accessRegion, acquireFence);
    if (result.error != Error::NONE) {
        drv_log("Failed to lockYCbCr. Failed to lockInternal.\n");
        hidlCb(result.error, ycbcr);
        return Void();
    }

    switch (crosHandle->format) {
        case DRM_FORMAT_NV12: {
            ycbcr.y = result.mapped[0] + crosHandle->offsets[0];
            ycbcr.cb = result.mapped[0] + crosHandle->offsets[1];
            ycbcr.cr = result.mapped[0] + crosHandle->offsets[1] + 1;
            ycbcr.yStride = crosHandle->strides[0];
            ycbcr.cStride = crosHandle->strides[1];
            ycbcr.chromaStep = 2;
            break;
        }
        case DRM_FORMAT_NV21: {
            ycbcr.y = result.mapped[0] + crosHandle->offsets[0];
            ycbcr.cb = result.mapped[0] + crosHandle->offsets[1] + 1;
            ycbcr.cr = result.mapped[0] + crosHandle->offsets[1];
            ycbcr.yStride = crosHandle->strides[0];
            ycbcr.cStride = crosHandle->strides[1];
            ycbcr.chromaStep = 2;
            break;
        }
        case DRM_FORMAT_YVU420: {
            ycbcr.y = result.mapped[0] + crosHandle->offsets[0];
            ycbcr.cb = result.mapped[0] + crosHandle->offsets[1];
            ycbcr.cr = result.mapped[0] + crosHandle->offsets[2];
            ycbcr.yStride = crosHandle->strides[0];
            ycbcr.cStride = crosHandle->strides[1];
            ycbcr.chromaStep = 1;
            break;
        }
        case DRM_FORMAT_YVU420_ANDROID: {
            ycbcr.y = result.mapped[0] + crosHandle->offsets[0];
            ycbcr.cb = result.mapped[0] + crosHandle->offsets[2];
            ycbcr.cr = result.mapped[0] + crosHandle->offsets[1];
            ycbcr.yStride = crosHandle->strides[0];
            ycbcr.cStride = crosHandle->strides[1];
            ycbcr.chromaStep = 1;
            break;
        }
        default: {
            std::string format = get_drm_format_string(crosHandle->format);
            drv_log("Failed to lockYCbCr. Unhandled format: %s\n", format.c_str());
            hidlCb(Error::BAD_BUFFER, ycbcr);
            return Void();
        }
    }

    hidlCb(Error::NONE, ycbcr);
    return Void();
}

CrosGralloc3Mapper::LockResult CrosGralloc3Mapper::lockInternal(
        cros_gralloc_handle_t crosHandle, uint64_t cpuUsage, const Rect& region,
        const android::hardware::hidl_handle& acquireFence) {
    LockResult result = {};

    if (!mDriver) {
        drv_log("Failed to lock. Driver is uninitialized.\n");
        result.error = Error::NO_RESOURCES;
        return result;
    }

    if (cpuUsage == 0) {
        drv_log("Failed to lock. Bad cpu usage: %" PRIu64 ".\n", cpuUsage);
        result.error = Error::BAD_VALUE;
        return result;
    }

    uint32_t mapUsage = 0;
    int ret = convertToMapUsage(cpuUsage, &mapUsage);
    if (ret) {
        drv_log("Failed to lock. Convert usage failed.\n");
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.left < 0) {
        drv_log("Failed to lock. Invalid region: negative left value %d.\n", region.left);
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.top < 0) {
        drv_log("Failed to lock. Invalid region: negative top value %d.\n", region.top);
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.width < 0) {
        drv_log("Failed to lock. Invalid region: negative width value %d.\n", region.width);
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.height < 0) {
        drv_log("Failed to lock. Invalid region: negative height value %d.\n", region.height);
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.width > crosHandle->width) {
        drv_log("Failed to lock. Invalid region: width greater than buffer width (%d vs %d).\n",
                region.width, crosHandle->width);
        result.error = Error::BAD_VALUE;
        return result;
    }

    if (region.height > crosHandle->height) {
        drv_log("Failed to lock. Invalid region: height greater than buffer height (%d vs %d).\n",
                region.height, crosHandle->height);
        result.error = Error::BAD_VALUE;
        return result;
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
        result.error = Error::BAD_VALUE;
        return result;
    }

    buffer_handle_t bufferHandle = reinterpret_cast<buffer_handle_t>(crosHandle);
    ret = mDriver->lock(bufferHandle, acquireFenceFd, false, &rect, mapUsage, result.mapped);
    if (ret) {
        result.error = Error::BAD_VALUE;
        return result;
    }

    result.error = Error::NONE;
    return result;
}

Return<void> CrosGralloc3Mapper::unlock(void* rawHandle, unlock_cb hidlCb) {
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

Return<void> CrosGralloc3Mapper::isSupported(const BufferDescriptorInfo& descriptor,
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

int CrosGralloc3Mapper::getResolvedDrmFormat(PixelFormat pixelFormat, uint64_t bufferUsage,
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

android::hardware::graphics::mapper::V3_0::IMapper* HIDL_FETCH_IMapper(const char* /*name*/) {
    return static_cast<android::hardware::graphics::mapper::V3_0::IMapper*>(new CrosGralloc3Mapper);
}
