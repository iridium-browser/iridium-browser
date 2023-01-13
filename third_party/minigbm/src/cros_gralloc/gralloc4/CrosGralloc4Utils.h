/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string>
#include <vector>

#include <aidl/android/hardware/graphics/common/PlaneLayout.h>
#include <android/hardware/graphics/common/1.2/types.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>

struct cros_gralloc_buffer_descriptor;

std::string getPixelFormatString(android::hardware::graphics::common::V1_2::PixelFormat format);

std::string getUsageString(
        android::hardware::hidl_bitfield<android::hardware::graphics::common::V1_2::BufferUsage>
                usage);

int convertToDrmFormat(android::hardware::graphics::common::V1_2::PixelFormat format,
                       uint32_t* outDrmFormat);

int convertToBufferUsage(uint64_t grallocUsage, uint64_t* outBufferUsage);

int convertToCrosDescriptor(
        const android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo& descriptor,
        struct cros_gralloc_buffer_descriptor* outCrosDescriptor);

int convertToMapUsage(uint64_t grallocUsage, uint32_t* outMapUsage);

int convertToFenceFd(const android::hardware::hidl_handle& fence_handle, int* out_fence_fd);

int convertToFenceHandle(int fence_fd, android::hardware::hidl_handle* out_fence_handle);

int getPlaneLayouts(
        uint32_t drm_format,
        std::vector<aidl::android::hardware::graphics::common::PlaneLayout>* out_layouts);
