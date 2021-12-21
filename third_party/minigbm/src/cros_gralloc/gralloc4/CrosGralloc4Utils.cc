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
    switch (format) {
        case PixelFormat::BGRA_8888:
            return "PixelFormat::BGRA_8888";
        case PixelFormat::BLOB:
            return "PixelFormat::BLOB";
        case PixelFormat::DEPTH_16:
            return "PixelFormat::DEPTH_16";
        case PixelFormat::DEPTH_24:
            return "PixelFormat::DEPTH_24";
        case PixelFormat::DEPTH_24_STENCIL_8:
            return "PixelFormat::DEPTH_24_STENCIL_8";
        case PixelFormat::DEPTH_32F:
            return "PixelFormat::DEPTH_24";
        case PixelFormat::DEPTH_32F_STENCIL_8:
            return "PixelFormat::DEPTH_24_STENCIL_8";
        case PixelFormat::HSV_888:
            return "PixelFormat::HSV_888";
        case PixelFormat::IMPLEMENTATION_DEFINED:
            return "PixelFormat::IMPLEMENTATION_DEFINED";
        case PixelFormat::RAW10:
            return "PixelFormat::RAW10";
        case PixelFormat::RAW12:
            return "PixelFormat::RAW12";
        case PixelFormat::RAW16:
            return "PixelFormat::RAW16";
        case PixelFormat::RAW_OPAQUE:
            return "PixelFormat::RAW_OPAQUE";
        case PixelFormat::RGBA_1010102:
            return "PixelFormat::RGBA_1010102";
        case PixelFormat::RGBA_8888:
            return "PixelFormat::RGBA_8888";
        case PixelFormat::RGBA_FP16:
            return "PixelFormat::RGBA_FP16";
        case PixelFormat::RGBX_8888:
            return "PixelFormat::RGBX_8888";
        case PixelFormat::RGB_565:
            return "PixelFormat::RGB_565";
        case PixelFormat::RGB_888:
            return "PixelFormat::RGB_888";
        case PixelFormat::STENCIL_8:
            return "PixelFormat::STENCIL_8";
        case PixelFormat::Y16:
            return "PixelFormat::Y16";
        case PixelFormat::Y8:
            return "PixelFormat::Y8";
        case PixelFormat::YCBCR_420_888:
            return "PixelFormat::YCBCR_420_888";
        case PixelFormat::YCBCR_422_I:
            return "PixelFormat::YCBCR_422_I";
        case PixelFormat::YCBCR_422_SP:
            return "PixelFormat::YCBCR_422_SP";
        case PixelFormat::YCBCR_P010:
            return "PixelFormat::YCBCR_P010";
        case PixelFormat::YCRCB_420_SP:
            return "PixelFormat::YCRCB_420_SP";
        case PixelFormat::YV12:
            return "PixelFormat::YV12";
    }
    return android::base::StringPrintf("PixelFormat::Unknown(%d)", static_cast<uint32_t>(format));
}

std::string getUsageString(hidl_bitfield<BufferUsage> bufferUsage) {
    using Underlying = typename std::underlying_type<BufferUsage>::type;

    Underlying usage = static_cast<Underlying>(bufferUsage);

    std::vector<std::string> usages;
    if (usage & BufferUsage::CAMERA_INPUT) {
        usage &= ~static_cast<Underlying>(BufferUsage::CAMERA_INPUT);
        usages.push_back("BufferUsage::CAMERA_INPUT");
    }
    if (usage & BufferUsage::CAMERA_OUTPUT) {
        usage &= ~static_cast<Underlying>(BufferUsage::CAMERA_OUTPUT);
        usages.push_back("BufferUsage::CAMERA_OUTPUT");
    }
    if (usage & BufferUsage::COMPOSER_CURSOR) {
        usage &= ~static_cast<Underlying>(BufferUsage::COMPOSER_CURSOR);
        usages.push_back("BufferUsage::COMPOSER_CURSOR");
    }
    if (usage & BufferUsage::COMPOSER_OVERLAY) {
        usage &= ~static_cast<Underlying>(BufferUsage::COMPOSER_OVERLAY);
        usages.push_back("BufferUsage::COMPOSER_OVERLAY");
    }
    if (usage & BufferUsage::CPU_READ_OFTEN) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_READ_OFTEN);
        usages.push_back("BufferUsage::CPU_READ_OFTEN");
    }
    if (usage & BufferUsage::CPU_READ_NEVER) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_READ_NEVER);
        usages.push_back("BufferUsage::CPU_READ_NEVER");
    }
    if (usage & BufferUsage::CPU_READ_RARELY) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_READ_RARELY);
        usages.push_back("BufferUsage::CPU_READ_RARELY");
    }
    if (usage & BufferUsage::CPU_WRITE_NEVER) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_WRITE_NEVER);
        usages.push_back("BufferUsage::CPU_WRITE_NEVER");
    }
    if (usage & BufferUsage::CPU_WRITE_OFTEN) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_WRITE_OFTEN);
        usages.push_back("BufferUsage::CPU_WRITE_OFTEN");
    }
    if (usage & BufferUsage::CPU_WRITE_RARELY) {
        usage &= ~static_cast<Underlying>(BufferUsage::CPU_WRITE_RARELY);
        usages.push_back("BufferUsage::CPU_WRITE_RARELY");
    }
    if (usage & BufferUsage::GPU_RENDER_TARGET) {
        usage &= ~static_cast<Underlying>(BufferUsage::GPU_RENDER_TARGET);
        usages.push_back("BufferUsage::GPU_RENDER_TARGET");
    }
    if (usage & BufferUsage::GPU_TEXTURE) {
        usage &= ~static_cast<Underlying>(BufferUsage::GPU_TEXTURE);
        usages.push_back("BufferUsage::GPU_TEXTURE");
    }
    if (usage & BufferUsage::PROTECTED) {
        usage &= ~static_cast<Underlying>(BufferUsage::PROTECTED);
        usages.push_back("BufferUsage::PROTECTED");
    }
    if (usage & BufferUsage::RENDERSCRIPT) {
        usage &= ~static_cast<Underlying>(BufferUsage::RENDERSCRIPT);
        usages.push_back("BufferUsage::RENDERSCRIPT");
    }
    if (usage & BufferUsage::VIDEO_DECODER) {
        usage &= ~static_cast<Underlying>(BufferUsage::VIDEO_DECODER);
        usages.push_back("BufferUsage::VIDEO_DECODER");
    }
    if (usage & BufferUsage::VIDEO_ENCODER) {
        usage &= ~static_cast<Underlying>(BufferUsage::VIDEO_ENCODER);
        usages.push_back("BufferUsage::VIDEO_ENCODER");
    }
    if (usage & BufferUsage::GPU_DATA_BUFFER) {
        usage &= ~static_cast<Underlying>(BufferUsage::GPU_DATA_BUFFER);
        usages.push_back("BufferUsage::GPU_DATA_BUFFER");
    }
    if (usage & BUFFER_USAGE_FRONT_RENDERING) {
        usage &= ~static_cast<Underlying>(BUFFER_USAGE_FRONT_RENDERING);
        usages.push_back("BUFFER_USAGE_FRONT_RENDERING");
    }

    if (usage) {
        usages.push_back(android::base::StringPrintf("UnknownUsageBits-%" PRIu64, usage));
    }

    return android::base::Join(usages, '|');
}

int convertToDrmFormat(PixelFormat format, uint32_t* outDrmFormat) {
    switch (format) {
        case PixelFormat::BGRA_8888:
            *outDrmFormat = DRM_FORMAT_ARGB8888;
            return 0;
        /**
         * Choose DRM_FORMAT_R8 because <system/graphics.h> requires the buffers
         * with a format HAL_PIXEL_FORMAT_BLOB have a height of 1, and width
         * equal to their size in bytes.
         */
        case PixelFormat::BLOB:
            *outDrmFormat = DRM_FORMAT_R8;
            return 0;
        case PixelFormat::DEPTH_16:
            return -EINVAL;
        case PixelFormat::DEPTH_24:
            return -EINVAL;
        case PixelFormat::DEPTH_24_STENCIL_8:
            return -EINVAL;
        case PixelFormat::DEPTH_32F:
            return -EINVAL;
        case PixelFormat::DEPTH_32F_STENCIL_8:
            return -EINVAL;
        case PixelFormat::HSV_888:
            return -EINVAL;
        case PixelFormat::IMPLEMENTATION_DEFINED:
            *outDrmFormat = DRM_FORMAT_FLEX_IMPLEMENTATION_DEFINED;
            return 0;
        case PixelFormat::RAW10:
            return -EINVAL;
        case PixelFormat::RAW12:
            return -EINVAL;
        case PixelFormat::RAW16:
            *outDrmFormat = DRM_FORMAT_R16;
            return 0;
        /* TODO use blob */
        case PixelFormat::RAW_OPAQUE:
            return -EINVAL;
        case PixelFormat::RGBA_1010102:
            *outDrmFormat = DRM_FORMAT_ABGR2101010;
            return 0;
        case PixelFormat::RGBA_8888:
            *outDrmFormat = DRM_FORMAT_ABGR8888;
            return 0;
        case PixelFormat::RGBA_FP16:
            *outDrmFormat = DRM_FORMAT_ABGR16161616F;
            return 0;
        case PixelFormat::RGBX_8888:
            *outDrmFormat = DRM_FORMAT_XBGR8888;
            return 0;
        case PixelFormat::RGB_565:
            *outDrmFormat = DRM_FORMAT_RGB565;
            return 0;
        case PixelFormat::RGB_888:
            *outDrmFormat = DRM_FORMAT_RGB888;
            return 0;
        case PixelFormat::STENCIL_8:
            return -EINVAL;
        case PixelFormat::Y16:
            *outDrmFormat = DRM_FORMAT_R16;
            return 0;
        case PixelFormat::Y8:
            *outDrmFormat = DRM_FORMAT_R8;
            return 0;
        case PixelFormat::YCBCR_420_888:
            *outDrmFormat = DRM_FORMAT_FLEX_YCbCr_420_888;
            return 0;
        case PixelFormat::YCBCR_422_SP:
            return -EINVAL;
        case PixelFormat::YCBCR_422_I:
            return -EINVAL;
        case PixelFormat::YCBCR_P010:
            *outDrmFormat = DRM_FORMAT_P010;
            return 0;
        case PixelFormat::YCRCB_420_SP:
            *outDrmFormat = DRM_FORMAT_NV21;
            return 0;
        case PixelFormat::YV12:
            *outDrmFormat = DRM_FORMAT_YVU420_ANDROID;
            return 0;
    };
    return -EINVAL;
}

int convertToBufferUsage(uint64_t grallocUsage, uint64_t* outBufferUsage) {
    uint64_t bufferUsage = BO_USE_NONE;

    if ((grallocUsage & BufferUsage::CPU_READ_MASK) ==
        static_cast<uint64_t>(BufferUsage::CPU_READ_RARELY)) {
        bufferUsage |= BO_USE_SW_READ_RARELY;
    }
    if ((grallocUsage & BufferUsage::CPU_READ_MASK) ==
        static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN)) {
        bufferUsage |= BO_USE_SW_READ_OFTEN;
    }
    if ((grallocUsage & BufferUsage::CPU_WRITE_MASK) ==
        static_cast<uint64_t>(BufferUsage::CPU_WRITE_RARELY)) {
        bufferUsage |= BO_USE_SW_WRITE_RARELY;
    }
    if ((grallocUsage & BufferUsage::CPU_WRITE_MASK) ==
        static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN)) {
        bufferUsage |= BO_USE_SW_WRITE_OFTEN;
    }
    if (grallocUsage & BufferUsage::GPU_TEXTURE) {
        bufferUsage |= BO_USE_TEXTURE;
    }
    if (grallocUsage & BufferUsage::GPU_RENDER_TARGET) {
        bufferUsage |= BO_USE_RENDERING;
    }
    if (grallocUsage & BufferUsage::COMPOSER_OVERLAY) {
        /* HWC wants to use display hardware, but can defer to OpenGL. */
        bufferUsage |= BO_USE_SCANOUT | BO_USE_TEXTURE;
    }
    /* Map this flag to linear until real HW protection is available on Android. */
    if (grallocUsage & BufferUsage::PROTECTED) {
        bufferUsage |= BO_USE_LINEAR;
    }
    if (grallocUsage & BufferUsage::COMPOSER_CURSOR) {
        bufferUsage |= BO_USE_NONE;
    }
    if (grallocUsage & BufferUsage::VIDEO_ENCODER) {
        /*HACK: See b/30054495 */
        bufferUsage |= BO_USE_SW_READ_OFTEN;
    }
    if (grallocUsage & BufferUsage::CAMERA_OUTPUT) {
        bufferUsage |= BO_USE_CAMERA_WRITE;
    }
    if (grallocUsage & BufferUsage::CAMERA_INPUT) {
        bufferUsage |= BO_USE_CAMERA_READ;
    }
    if (grallocUsage & BufferUsage::RENDERSCRIPT) {
        bufferUsage |= BO_USE_RENDERSCRIPT;
    }
    if (grallocUsage & BufferUsage::VIDEO_DECODER) {
        bufferUsage |= BO_USE_HW_VIDEO_DECODER;
    }
    if (grallocUsage & BufferUsage::GPU_DATA_BUFFER) {
        bufferUsage |= BO_USE_GPU_DATA_BUFFER;
    }
    if (grallocUsage & BUFFER_USAGE_FRONT_RENDERING) {
        bufferUsage |= BO_USE_FRONT_RENDERING;
    }

    *outBufferUsage = bufferUsage;
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
        drv_log("Failed to convert descriptor. Unsupported layerCount: %d\n",
                descriptor.layerCount);
        return -EINVAL;
    }
    if (convertToDrmFormat(descriptor.format, &outCrosDescriptor->drm_format)) {
        std::string pixelFormatString = getPixelFormatString(descriptor.format);
        drv_log("Failed to convert descriptor. Unsupported format %s\n", pixelFormatString.c_str());
        return -EINVAL;
    }
    if (convertToBufferUsage(descriptor.usage, &outCrosDescriptor->use_flags)) {
        std::string usageString = getUsageString(descriptor.usage);
        drv_log("Failed to convert descriptor. Unsupported usage flags %s\n", usageString.c_str());
        return -EINVAL;
    }
    return 0;
}

int convertToMapUsage(uint64_t grallocUsage, uint32_t* outMapUsage) {
    uint32_t mapUsage = BO_MAP_NONE;

    if (grallocUsage & BufferUsage::CPU_READ_MASK) {
        mapUsage |= BO_MAP_READ;
    }
    if (grallocUsage & BufferUsage::CPU_WRITE_MASK) {
        mapUsage |= BO_MAP_WRITE;
    }

    *outMapUsage = mapUsage;
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
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 5},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 5,
                                             .sizeInBits = 6},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 11,
                                             .sizeInBits = 5}},
                             .sampleIncrementInBits = 16,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_RGB888,
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
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_R,
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
                                                             PlaneLayoutComponentType_CB,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
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
        drv_log("Unknown plane layout for format %d\n", drmFormat);
        return -EINVAL;
    }

    *outPlaneLayouts = it->second;
    return 0;
}
