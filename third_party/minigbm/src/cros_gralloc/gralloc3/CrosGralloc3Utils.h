/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <optional>
#include <string>
#include <vector>

#include <android/hardware/graphics/common/1.2/types.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>

std::string getPixelFormatString(android::hardware::graphics::common::V1_2::PixelFormat format);

std::string getUsageString(
        android::hardware::hidl_bitfield<android::hardware::graphics::common::V1_2::BufferUsage>
                usage);

int convertToDrmFormat(android::hardware::graphics::common::V1_2::PixelFormat format,
                       uint32_t* outDrmFormat);

int convertToBufferUsage(uint64_t grallocUsage, uint64_t* outBufferUsage);

int convertToMapUsage(uint64_t grallocUsage, uint32_t* outMapUsage);

int convertToCrosDescriptor(
        const android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& descriptor,
        struct cros_gralloc_buffer_descriptor* outCrosDescriptor);

int convertToFenceFd(const android::hardware::hidl_handle& fence_handle, int* out_fence_fd);

int convertToFenceHandle(int fence_fd, android::hardware::hidl_handle* out_fence_handle);

std::optional<android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo>
decodeBufferDescriptorInfo(const android::hardware::hidl_vec<uint32_t>& encoded);

std::optional<android::hardware::hidl_vec<uint32_t>> encodeBufferDescriptorInfo(
        const android::hardware::graphics::mapper::V3_0::IMapper::BufferDescriptorInfo& info);
