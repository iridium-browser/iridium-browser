// Copyright 2020 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "examples/gav1_decode_cv_pixel_buffer_pool.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>

namespace {

struct CFTypeDeleter {
  void operator()(CFTypeRef cf) const { CFRelease(cf); }
};

using UniqueCFNumberRef =
    std::unique_ptr<std::remove_pointer<CFNumberRef>::type, CFTypeDeleter>;

using UniqueCFDictionaryRef =
    std::unique_ptr<std::remove_pointer<CFDictionaryRef>::type, CFTypeDeleter>;

}  // namespace

extern "C" {

libgav1::StatusCode Gav1DecodeOnCVPixelBufferSizeChanged(
    void* callback_private_data, int bitdepth,
    libgav1::ImageFormat image_format, int width, int height, int left_border,
    int right_border, int top_border, int bottom_border, int stride_alignment) {
  auto* buffer_pool =
      static_cast<Gav1DecodeCVPixelBufferPool*>(callback_private_data);
  return buffer_pool->OnCVPixelBufferSizeChanged(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment);
}

libgav1::StatusCode Gav1DecodeGetCVPixelBuffer(
    void* callback_private_data, int bitdepth,
    libgav1::ImageFormat image_format, int width, int height, int left_border,
    int right_border, int top_border, int bottom_border, int stride_alignment,
    libgav1::FrameBuffer* frame_buffer) {
  auto* buffer_pool =
      static_cast<Gav1DecodeCVPixelBufferPool*>(callback_private_data);
  return buffer_pool->GetCVPixelBuffer(
      bitdepth, image_format, width, height, left_border, right_border,
      top_border, bottom_border, stride_alignment, frame_buffer);
}

void Gav1DecodeReleaseCVPixelBuffer(void* callback_private_data,
                                    void* buffer_private_data) {
  auto* buffer_pool =
      static_cast<Gav1DecodeCVPixelBufferPool*>(callback_private_data);
  buffer_pool->ReleaseCVPixelBuffer(buffer_private_data);
}

}  // extern "C"

// static
std::unique_ptr<Gav1DecodeCVPixelBufferPool>
Gav1DecodeCVPixelBufferPool::Create(size_t num_buffers) {
  std::unique_ptr<Gav1DecodeCVPixelBufferPool> buffer_pool(
      new (std::nothrow) Gav1DecodeCVPixelBufferPool(num_buffers));
  return buffer_pool;
}

Gav1DecodeCVPixelBufferPool::Gav1DecodeCVPixelBufferPool(size_t num_buffers)
    : num_buffers_(static_cast<int>(num_buffers)) {}

Gav1DecodeCVPixelBufferPool::~Gav1DecodeCVPixelBufferPool() {
  CVPixelBufferPoolRelease(pool_);
}

libgav1::StatusCode Gav1DecodeCVPixelBufferPool::OnCVPixelBufferSizeChanged(
    int bitdepth, libgav1::ImageFormat image_format, int width, int height,
    int left_border, int right_border, int top_border, int bottom_border,
    int stride_alignment) {
  if (bitdepth != 8 || (image_format != libgav1::kImageFormatYuv420 &&
                        image_format != libgav1::kImageFormatMonochrome400)) {
    fprintf(stderr,
            "Only bitdepth 8, 4:2:0 videos are supported: bitdepth %d, "
            "image_format: %d.\n",
            bitdepth, image_format);
    return libgav1::kStatusUnimplemented;
  }

  // stride_alignment must be a power of 2.
  assert((stride_alignment & (stride_alignment - 1)) == 0);

  // The possible keys for CVPixelBufferPool are:
  //   kCVPixelBufferPoolMinimumBufferCountKey
  //   kCVPixelBufferPoolMaximumBufferAgeKey
  //   kCVPixelBufferPoolAllocationThresholdKey
  const void* pool_keys[] = {kCVPixelBufferPoolMinimumBufferCountKey};
  const int min_buffer_count = 10;
  UniqueCFNumberRef cf_min_buffer_count(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &min_buffer_count));
  if (cf_min_buffer_count == nullptr) {
    fprintf(stderr, "CFNumberCreate failed.\n");
    return libgav1::kStatusUnknownError;
  }
  const void* pool_values[] = {cf_min_buffer_count.get()};
  UniqueCFDictionaryRef pool_attributes(CFDictionaryCreate(
      nullptr, pool_keys, pool_values, 1, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks));
  if (pool_attributes == nullptr) {
    fprintf(stderr, "CFDictionaryCreate failed.\n");
    return libgav1::kStatusUnknownError;
  }

  // The pixelBufferAttributes argument to CVPixelBufferPoolCreate() cannot be
  // null and must contain the pixel format, width, and height, otherwise
  // CVPixelBufferPoolCreate() fails with kCVReturnInvalidPixelBufferAttributes
  // (-6682).

  // I420: kCVPixelFormatType_420YpCbCr8Planar (video range).
  const int pixel_format = (image_format == libgav1::kImageFormatYuv420)
                               ? kCVPixelFormatType_420YpCbCr8PlanarFullRange
                               : kCVPixelFormatType_OneComponent8;
  UniqueCFNumberRef cf_pixel_format(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pixel_format));
  UniqueCFNumberRef cf_width(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &width));
  UniqueCFNumberRef cf_height(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &height));
  UniqueCFNumberRef cf_left_border(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &left_border));
  UniqueCFNumberRef cf_right_border(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &right_border));
  UniqueCFNumberRef cf_top_border(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &top_border));
  UniqueCFNumberRef cf_bottom_border(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &bottom_border));
  UniqueCFNumberRef cf_stride_alignment(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &stride_alignment));

  const void* buffer_keys[] = {
      kCVPixelBufferPixelFormatTypeKey,
      kCVPixelBufferWidthKey,
      kCVPixelBufferHeightKey,
      kCVPixelBufferExtendedPixelsLeftKey,
      kCVPixelBufferExtendedPixelsRightKey,
      kCVPixelBufferExtendedPixelsTopKey,
      kCVPixelBufferExtendedPixelsBottomKey,
      kCVPixelBufferBytesPerRowAlignmentKey,
  };
  const void* buffer_values[] = {
      cf_pixel_format.get(),  cf_width.get(),
      cf_height.get(),        cf_left_border.get(),
      cf_right_border.get(),  cf_top_border.get(),
      cf_bottom_border.get(), cf_stride_alignment.get(),
  };
  UniqueCFDictionaryRef buffer_attributes(CFDictionaryCreate(
      kCFAllocatorDefault, buffer_keys, buffer_values, 8,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  if (buffer_attributes == nullptr) {
    fprintf(stderr, "CFDictionaryCreate of buffer_attributes failed.\n");
    return libgav1::kStatusUnknownError;
  }
  CVPixelBufferPoolRef cv_pool;
  CVReturn ret = CVPixelBufferPoolCreate(
      /*allocator=*/nullptr, pool_attributes.get(), buffer_attributes.get(),
      &cv_pool);
  if (ret != kCVReturnSuccess) {
    fprintf(stderr, "CVPixelBufferPoolCreate failed: %d.\n",
            static_cast<int>(ret));
    return libgav1::kStatusOutOfMemory;
  }
  CVPixelBufferPoolRelease(pool_);
  pool_ = cv_pool;
  return libgav1::kStatusOk;
}

libgav1::StatusCode Gav1DecodeCVPixelBufferPool::GetCVPixelBuffer(
    int bitdepth, libgav1::ImageFormat image_format, int /*width*/,
    int /*height*/, int /*left_border*/, int /*right_border*/,
    int /*top_border*/, int /*bottom_border*/, int /*stride_alignment*/,
    libgav1::FrameBuffer* frame_buffer) {
  static_cast<void>(bitdepth);
  assert(bitdepth == 8 && (image_format == libgav1::kImageFormatYuv420 ||
                           image_format == libgav1::kImageFormatMonochrome400));
  const bool is_monochrome =
      (image_format == libgav1::kImageFormatMonochrome400);

  // The dictionary must have kCVPixelBufferPoolAllocationThresholdKey,
  // otherwise CVPixelBufferPoolCreatePixelBufferWithAuxAttributes() fails with
  // kCVReturnWouldExceedAllocationThreshold (-6689).
  UniqueCFNumberRef cf_num_buffers(
      CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &num_buffers_));

  const void* buffer_keys[] = {
      kCVPixelBufferPoolAllocationThresholdKey,
  };
  const void* buffer_values[] = {
      cf_num_buffers.get(),
  };
  UniqueCFDictionaryRef aux_attributes(CFDictionaryCreate(
      kCFAllocatorDefault, buffer_keys, buffer_values, 1,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  if (aux_attributes == nullptr) {
    fprintf(stderr, "CFDictionaryCreate of aux_attributes failed.\n");
    return libgav1::kStatusUnknownError;
  }

  CVPixelBufferRef pixel_buffer;
  CVReturn ret = CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(
      /*allocator=*/nullptr, pool_, aux_attributes.get(), &pixel_buffer);
  if (ret != kCVReturnSuccess) {
    fprintf(stderr,
            "CVPixelBufferPoolCreatePixelBufferWithAuxAttributes failed: %d.\n",
            static_cast<int>(ret));
    return libgav1::kStatusOutOfMemory;
  }

  ret = CVPixelBufferLockBaseAddress(pixel_buffer, /*lockFlags=*/0);
  if (ret != kCVReturnSuccess) {
    fprintf(stderr, "CVPixelBufferLockBaseAddress failed: %d.\n",
            static_cast<int>(ret));
    CFRelease(pixel_buffer);
    return libgav1::kStatusUnknownError;
  }

  // If the pixel format type is kCVPixelFormatType_OneComponent8, the pixel
  // buffer is nonplanar (CVPixelBufferIsPlanar returns false and
  // CVPixelBufferGetPlaneCount returns 0), but
  // CVPixelBufferGetBytesPerRowOfPlane and CVPixelBufferGetBaseAddressOfPlane
  // still work for plane index 0, even though the documentation says they
  // return NULL for nonplanar pixel buffers.
  frame_buffer->stride[0] =
      static_cast<int>(CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0));
  frame_buffer->plane[0] = static_cast<uint8_t*>(
      CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
  if (is_monochrome) {
    frame_buffer->stride[1] = 0;
    frame_buffer->stride[2] = 0;
    frame_buffer->plane[1] = nullptr;
    frame_buffer->plane[2] = nullptr;
  } else {
    frame_buffer->stride[1] =
        static_cast<int>(CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1));
    frame_buffer->stride[2] =
        static_cast<int>(CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 2));
    frame_buffer->plane[1] = static_cast<uint8_t*>(
        CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
    frame_buffer->plane[2] = static_cast<uint8_t*>(
        CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 2));
  }
  frame_buffer->private_data = pixel_buffer;

  return libgav1::kStatusOk;
}

void Gav1DecodeCVPixelBufferPool::ReleaseCVPixelBuffer(
    void* buffer_private_data) {
  auto const pixel_buffer = static_cast<CVPixelBufferRef>(buffer_private_data);
  CVReturn ret =
      CVPixelBufferUnlockBaseAddress(pixel_buffer, /*unlockFlags=*/0);
  if (ret != kCVReturnSuccess) {
    fprintf(stderr, "%s:%d: CVPixelBufferUnlockBaseAddress failed: %d.\n",
            __FILE__, __LINE__, static_cast<int>(ret));
    abort();
  }
  CFRelease(pixel_buffer);
}
