/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc/gralloc4/CrosGralloc4Allocator.h"

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <gralloctypes/Gralloc4.h>

#include "cros_gralloc/cros_gralloc_helpers.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Metadata.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Utils.h"

using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::Dataspace;
using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using android::hardware::graphics::mapper::V4_0::Error;

using BufferDescriptorInfo =
        android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo;

Error CrosGralloc4Allocator::init() {
    mDriver = cros_gralloc_driver::get_instance();
    return mDriver ? Error::NONE : Error::NO_RESOURCES;
}

Error CrosGralloc4Allocator::initializeMetadata(
        cros_gralloc_handle_t crosHandle,
        const struct cros_gralloc_buffer_descriptor& crosDescriptor) {
    if (!mDriver) {
        ALOGE("Failed to initializeMetadata. Driver is uninitialized.");
        return Error::NO_RESOURCES;
    }

    if (!crosHandle) {
        ALOGE("Failed to initializeMetadata. Invalid handle.");
        return Error::BAD_BUFFER;
    }

    void* addr;
    uint64_t size;
    int ret = mDriver->get_reserved_region(crosHandle, &addr, &size);
    if (ret) {
        ALOGE("Failed to getReservedRegion.");
        return Error::NO_RESOURCES;
    }

    CrosGralloc4Metadata* crosMetadata = reinterpret_cast<CrosGralloc4Metadata*>(addr);

    snprintf(crosMetadata->name, CROS_GRALLOC4_METADATA_MAX_NAME_SIZE, "%s",
             crosDescriptor.name.c_str());
    crosMetadata->dataspace = Dataspace::UNKNOWN;
    crosMetadata->blendMode = BlendMode::INVALID;

    return Error::NONE;
}

Error CrosGralloc4Allocator::allocate(const BufferDescriptorInfo& descriptor, uint32_t* outStride,
                                      hidl_handle* outHandle) {
    if (!mDriver) {
        ALOGE("Failed to allocate. Driver is uninitialized.");
        return Error::NO_RESOURCES;
    }

    if (!outStride || !outHandle) {
        return Error::NO_RESOURCES;
    }

    struct cros_gralloc_buffer_descriptor crosDescriptor;
    if (convertToCrosDescriptor(descriptor, &crosDescriptor)) {
        return Error::UNSUPPORTED;
    }

    crosDescriptor.reserved_region_size += sizeof(CrosGralloc4Metadata);

    if (!mDriver->is_supported(&crosDescriptor)) {
        std::string drmFormatString = get_drm_format_string(crosDescriptor.drm_format);
        std::string pixelFormatString = getPixelFormatString(descriptor.format);
        std::string usageString = getUsageString(descriptor.usage);
        ALOGE("Unsupported combination -- pixel format: %s, drm format:%s, usage: %s",
              pixelFormatString.c_str(), drmFormatString.c_str(), usageString.c_str());
        return Error::UNSUPPORTED;
    }

    native_handle_t* handle;
    int ret = mDriver->allocate(&crosDescriptor, &handle);
    if (ret) {
        return Error::NO_RESOURCES;
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(handle);

    Error error = initializeMetadata(crosHandle, crosDescriptor);
    if (error != Error::NONE) {
        mDriver->release(handle);
        native_handle_close(handle);
        native_handle_delete(handle);
        return error;
    }

    outHandle->setTo(handle, /*shouldOwn=*/true);
    *outStride = crosHandle->pixel_stride;

    return Error::NONE;
}

Return<void> CrosGralloc4Allocator::allocate(const hidl_vec<uint8_t>& descriptor, uint32_t count,
                                             allocate_cb hidlCb) {
    hidl_vec<hidl_handle> handles;

    if (!mDriver) {
        ALOGE("Failed to allocate. Driver is uninitialized.");
        hidlCb(Error::NO_RESOURCES, 0, handles);
        return Void();
    }

    BufferDescriptorInfo description;

    int ret = android::gralloc4::decodeBufferDescriptorInfo(descriptor, &description);
    if (ret) {
        ALOGE("Failed to allocate. Failed to decode buffer descriptor: %d.", ret);
        hidlCb(Error::BAD_DESCRIPTOR, 0, handles);
        return Void();
    }

    handles.resize(count);

    uint32_t stride = 0;
    for (int i = 0; i < handles.size(); i++) {
        Error err = allocate(description, &stride, &(handles[i]));
        if (err != Error::NONE) {
            for (int j = 0; j < i; j++) {
                mDriver->release(handles[j].getNativeHandle());
            }
            handles.resize(0);
            hidlCb(err, 0, handles);
            return Void();
        }
    }

    hidlCb(Error::NONE, stride, handles);

    for (const hidl_handle& handle : handles) {
        mDriver->release(handle.getNativeHandle());
    }

    return Void();
}
