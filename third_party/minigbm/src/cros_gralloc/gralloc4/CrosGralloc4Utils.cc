/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "cros_gralloc/gralloc4/CrosGralloc4Utils.h"

#include <array>
#include <unordered_map>

#include <aidl/android/hardware/graphics/common/PlaneLayoutComponent.h>
#include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>

#include "cros_gralloc/cros_gralloc_helpers.h"

using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using android::hardware::hidl_bitfield;
using android::hardware::hidl_handle;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;

using BufferDescriptorInfo =
        android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo;

std::string getPixelFormatString(PixelFormat format) {
    return android::hardware::graphics::common::V1_2::toString(format);
}

std::string getUsageString(hidl_bitfield<BufferUsage> bufferUsage) {
    static_assert(std::is_same<std::underlying_type<BufferUsage>::type, uint64_t>::value);

    const uint64_t usage = static_cast<uint64_t>(bufferUsage);
    return android::hardware::graphics::common::V1_2::toString<BufferUsage>(usage);
}

int convertToDrmFormat(PixelFormat format, uint32_t* outDrmFormat) {
    static_assert(std::is_same<std::underlying_type<PixelFormat>::type, int32_t>::value);

    const uint32_t drmFormat = cros_gralloc_convert_format(static_cast<int32_t>(format));
    if (drmFormat == DRM_FORMAT_NONE) return -EINVAL;

    *outDrmFormat = drmFormat;
    return 0;
}

int convertToBufferUsage(uint64_t grallocUsage, uint64_t* outBufferUsage) {
    *outBufferUsage = cros_gralloc_convert_usage(grallocUsage);
    return 0;
}

int convertToCrosDescriptor(const BufferDescriptorInfo& descriptor,
                            struct cros_gralloc_buffer_descriptor* outCrosDescriptor) {
    outCrosDescriptor->name = descriptor.name;
    outCrosDescriptor->width = descriptor.width;
    outCrosDescriptor->height = descriptor.height;
    outCrosDescriptor->droid_format = static_cast<int32_t>(descriptor.format);
    outCrosDescriptor->droid_usage = descriptor.usage;
    outCrosDescriptor->reserved_region_size = descriptor.reservedSize;
    if (descriptor.layerCount > 1) {
        ALOGE("Failed to convert descriptor. Unsupported layerCount: %d", descriptor.layerCount);
        return -EINVAL;
    }
    if (convertToDrmFormat(descriptor.format, &outCrosDescriptor->drm_format)) {
        std::string pixelFormatString = getPixelFormatString(descriptor.format);
        ALOGE("Failed to convert descriptor. Unsupported format %s", pixelFormatString.c_str());
        return -EINVAL;
    }
    if (convertToBufferUsage(descriptor.usage, &outCrosDescriptor->use_flags)) {
        std::string usageString = getUsageString(descriptor.usage);
        ALOGE("Failed to convert descriptor. Unsupported usage flags %s", usageString.c_str());
        return -EINVAL;
    }
    return 0;
}

int convertToMapUsage(uint64_t grallocUsage, uint32_t* outMapUsage) {
    *outMapUsage = cros_gralloc_convert_map_usage(grallocUsage);
    return 0;
}

int convertToFenceFd(const hidl_handle& fenceHandle, int* outFenceFd) {
    if (!outFenceFd) {
        return -EINVAL;
    }

    const native_handle_t* nativeHandle = fenceHandle.getNativeHandle();
    if (nativeHandle && nativeHandle->numFds > 1) {
        return -EINVAL;
    }

    *outFenceFd = (nativeHandle && nativeHandle->numFds == 1) ? nativeHandle->data[0] : -1;
    return 0;
}

int convertToFenceHandle(int fenceFd, hidl_handle* outFenceHandle) {
    if (!outFenceHandle) {
        return -EINVAL;
    }
    if (fenceFd < 0) {
        return 0;
    }

    NATIVE_HANDLE_DECLARE_STORAGE(handleStorage, 1, 0);
    auto fenceHandle = native_handle_init(handleStorage, 1, 0);
    fenceHandle->data[0] = fenceFd;

    *outFenceHandle = fenceHandle;
    return 0;
}

const std::unordered_map<uint32_t, std::vector<PlaneLayout>>& GetPlaneLayoutsMap() {
    static const auto* kPlaneLayoutsMap =
            new std::unordered_map<uint32_t, std::vector<PlaneLayout>>({
                    {DRM_FORMAT_ABGR8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 24,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ABGR2101010,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 10,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 20,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 30,
                                             .sizeInBits = 2}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ABGR16161616F,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 16,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 32,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 48,
                                             .sizeInBits = 16}},
                             .sampleIncrementInBits = 64,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ARGB8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 24,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_NV12,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 0,
                                              .sizeInBits = 8}},
                              .sampleIncrementInBits = 8,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 0,
                                        .sizeInBits = 8},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 8,
                                        .sizeInBits = 8}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_NV21,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 0,
                                              .sizeInBits = 8}},
                              .sampleIncrementInBits = 8,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 0,
                                        .sizeInBits = 8},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 8,
                                        .sizeInBits = 8}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_P010,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 6,
                                              .sizeInBits = 10}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 6,
                                        .sizeInBits = 10},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 22,
                                        .sizeInBits = 10}},
                              .sampleIncrementInBits = 32,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_R8,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 8,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_R16,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 16}},
                             .sampleIncrementInBits = 16,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_RGB565,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 0,
                                             .sizeInBits = 5},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 5,
                                             .sizeInBits = 6},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 11,
                                             .sizeInBits = 5}},
                             .sampleIncrementInBits = 16,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_BGR888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 24,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_XBGR8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_YVU420,
                     {
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_Y,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 1,
                                     .verticalSubsampling = 1,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CR,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CB,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                     }},

                    {DRM_FORMAT_YVU420_ANDROID,
                     {
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_Y,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 1,
                                     .verticalSubsampling = 1,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CR,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CB,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                     }},
            });
    return *kPlaneLayoutsMap;
}

int getPlaneLayouts(uint32_t drmFormat, std::vector<PlaneLayout>* outPlaneLayouts) {
    const auto& planeLayoutsMap = GetPlaneLayoutsMap();
    const auto it = planeLayoutsMap.find(drmFormat);
    if (it == planeLayoutsMap.end()) {
        ALOGE("Unknown plane layout for format %d", drmFormat);
        return -EINVAL;
    }

    *outPlaneLayouts = it->second;
    return 0;
}
