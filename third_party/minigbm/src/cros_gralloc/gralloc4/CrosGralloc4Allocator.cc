/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc/gralloc4/CrosGralloc4Allocator.h"

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <gralloctypes/Gralloc4.h>

#include "cros_gralloc/cros_gralloc_helpers.h"
#include "cros_gralloc/gralloc4/CrosGralloc4Utils.h"

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

Error CrosGralloc4Allocator::allocate(const BufferDescriptorInfo& descriptor, uint32_t* outStride,
                                      hidl_handle* outHandle) {
    if (!mDriver) {
        drv_log("Failed to allocate. Driver is uninitialized.\n");
        return Error::NO_RESOURCES;
    }

    if (!outStride || !outHandle) {
        return Error::NO_RESOURCES;
    }

    struct cros_gralloc_buffer_descriptor crosDescriptor;
    if (convertToCrosDescriptor(descriptor, &crosDescriptor)) {
        return Error::UNSUPPORTED;
    }

    bool supported = mDriver->is_supported(&crosDescriptor);
    if (!supported && (descriptor.usage & BufferUsage::COMPOSER_OVERLAY)) {
        crosDescriptor.use_flags &= ~BO_USE_SCANOUT;
        supported = mDriver->is_supported(&crosDescriptor);
    }

    if (!supported) {
        std::string drmFormatString = get_drm_format_string(crosDescriptor.drm_format);
        std::string pixelFormatString = getPixelFormatString(descriptor.format);
        std::string usageString = getUsageString(descriptor.usage);
        drv_log("Unsupported combination -- pixel format: %s, drm format:%s, usage: %s\n",
                pixelFormatString.c_str(), drmFormatString.c_str(), usageString.c_str());
        return Error::UNSUPPORTED;
    }

    buffer_handle_t handle;
    int ret = mDriver->allocate(&crosDescriptor, &handle);
    if (ret) {
        return Error::NO_RESOURCES;
    }

    cros_gralloc_handle_t crosHandle = cros_gralloc_convert_handle(handle);
    if (!crosHandle) {
        return Error::NO_RESOURCES;
    }

    *outHandle = handle;
    *outStride = crosHandle->pixel_stride;

    return Error::NONE;
}

Return<void> CrosGralloc4Allocator::allocate(const hidl_vec<uint8_t>& descriptor, uint32_t count,
                                             allocate_cb hidlCb) {
    hidl_vec<hidl_handle> handles;

    if (!mDriver) {
        drv_log("Failed to allocate. Driver is uninitialized.\n");
        hidlCb(Error::NO_RESOURCES, 0, handles);
        return Void();
    }

    BufferDescriptorInfo description;

    int ret = android::gralloc4::decodeBufferDescriptorInfo(descriptor, &description);
    if (ret) {
        drv_log("Failed to allocate. Failed to decode buffer descriptor: %d.\n", ret);
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
