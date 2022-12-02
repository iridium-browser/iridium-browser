// Copyright 2021 The libgav1 Authors
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

#include "src/buffer_pool.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <ostream>
#include <tuple>
#include <utility>

#include "gtest/gtest.h"
#include "src/frame_buffer_utils.h"
#include "src/gav1/decoder_buffer.h"
#include "src/gav1/frame_buffer.h"
#include "src/internal_frame_buffer_list.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"
#include "src/yuv_buffer.h"

namespace libgav1 {
namespace {

TEST(BufferPoolTest, RefCountedBufferPtr) {
  InternalFrameBufferList buffer_list;
  BufferPool buffer_pool(OnInternalFrameBufferSizeChanged,
                         GetInternalFrameBuffer, ReleaseInternalFrameBuffer,
                         &buffer_list);
  RefCountedBufferPtr buffer_ptr = buffer_pool.GetFreeBuffer();
  EXPECT_NE(buffer_ptr, nullptr);
  EXPECT_EQ(buffer_ptr.use_count(), 1);

  RefCountedBufferPtr buffer_ptr2 = buffer_ptr;
  RefCountedBufferPtr buffer_ptr3 = buffer_ptr;
  EXPECT_EQ(buffer_ptr.use_count(), 3);
  EXPECT_EQ(buffer_ptr2.use_count(), 3);
  EXPECT_EQ(buffer_ptr3.use_count(), 3);

  buffer_ptr2 = nullptr;
  EXPECT_EQ(buffer_ptr.use_count(), 2);
  EXPECT_EQ(buffer_ptr2.use_count(), 0);
  EXPECT_EQ(buffer_ptr3.use_count(), 2);

  RefCountedBufferPtr buffer_ptr4 = std::move(buffer_ptr);
  EXPECT_EQ(buffer_ptr.use_count(), 0);
  EXPECT_EQ(buffer_ptr2.use_count(), 0);
  EXPECT_EQ(buffer_ptr3.use_count(), 2);
  EXPECT_EQ(buffer_ptr4.use_count(), 2);
}

TEST(RefCountedBufferTest, SetFrameDimensions) {
  InternalFrameBufferList buffer_list;
  BufferPool buffer_pool(OnInternalFrameBufferSizeChanged,
                         GetInternalFrameBuffer, ReleaseInternalFrameBuffer,
                         &buffer_list);
  RefCountedBufferPtr buffer_ptr = buffer_pool.GetFreeBuffer();
  EXPECT_NE(buffer_ptr, nullptr);

  // Test the undocumented default values of rows4x4() and columns4x4(). (Not
  // sure if this is a good idea.)
  EXPECT_EQ(buffer_ptr->rows4x4(), 0);
  EXPECT_EQ(buffer_ptr->columns4x4(), 0);

  // Test the side effects of SetFrameDimensions().
  ObuFrameHeader frame_header = {};
  frame_header.rows4x4 = 20;
  frame_header.columns4x4 = 30;
  EXPECT_TRUE(buffer_ptr->SetFrameDimensions(frame_header));
  EXPECT_EQ(buffer_ptr->rows4x4(), 20);
  EXPECT_EQ(buffer_ptr->columns4x4(), 30);
}

TEST(RefCountedBuffertTest, WaitUntil) {
  InternalFrameBufferList buffer_list;
  BufferPool buffer_pool(OnInternalFrameBufferSizeChanged,
                         GetInternalFrameBuffer, ReleaseInternalFrameBuffer,
                         &buffer_list);
  RefCountedBufferPtr buffer_ptr = buffer_pool.GetFreeBuffer();
  EXPECT_NE(buffer_ptr, nullptr);

  int progress_row_cache;
  buffer_ptr->SetProgress(10);
  EXPECT_TRUE(buffer_ptr->WaitUntil(5, &progress_row_cache));
  EXPECT_EQ(progress_row_cache, 10);

  buffer_ptr->SetFrameState(kFrameStateDecoded);
  EXPECT_TRUE(buffer_ptr->WaitUntil(500, &progress_row_cache));
  EXPECT_EQ(progress_row_cache, INT_MAX);

  buffer_ptr->Abort();
  EXPECT_FALSE(buffer_ptr->WaitUntil(50, &progress_row_cache));
}

constexpr struct Params {
  int width;
  int height;
  int8_t subsampling_x;
  int8_t subsampling_y;
  int border;
} kParams[] = {
    {1920, 1080, 1, 1, 96},   //
    {1920, 1080, 1, 1, 64},   //
    {1920, 1080, 1, 1, 32},   //
    {1920, 1080, 1, 1, 160},  //
    {1920, 1080, 1, 0, 160},  //
    {1920, 1080, 0, 0, 160},  //
};

std::ostream& operator<<(std::ostream& os, const Params& param) {
  return os << param.width << "x" << param.height
            << ", subsampling(x/y): " << static_cast<int>(param.subsampling_x)
            << "/" << static_cast<int>(param.subsampling_y)
            << ", border: " << param.border;
}

class RefCountedBufferReallocTest
    : public testing::TestWithParam<std::tuple<bool, Params>> {
 protected:
  const bool use_external_callbacks_ = std::get<0>(GetParam());
  const Params& param_ = std::get<1>(GetParam());
};

TEST_P(RefCountedBufferReallocTest, 8Bit) {
  InternalFrameBufferList buffer_list;
  FrameBufferSizeChangedCallback on_frame_buffer_size_changed = nullptr;
  GetFrameBufferCallback get_frame_buffer = nullptr;
  ReleaseFrameBufferCallback release_frame_buffer = nullptr;
  void* callback_private_data = nullptr;
  if (use_external_callbacks_) {
    on_frame_buffer_size_changed = OnInternalFrameBufferSizeChanged;
    get_frame_buffer = GetInternalFrameBuffer;
    release_frame_buffer = ReleaseInternalFrameBuffer;
    callback_private_data = &buffer_list;
  }

  BufferPool buffer_pool(on_frame_buffer_size_changed, get_frame_buffer,
                         release_frame_buffer, callback_private_data);

  RefCountedBufferPtr buffer_ptr = buffer_pool.GetFreeBuffer();
  EXPECT_NE(buffer_ptr, nullptr);

  const Libgav1ImageFormat image_format = ComposeImageFormat(
      /*is_monochrome=*/false, param_.subsampling_x, param_.subsampling_y);
  EXPECT_TRUE(buffer_pool.OnFrameBufferSizeChanged(
      /*bitdepth=*/8, image_format, param_.width, param_.height, param_.border,
      param_.border, param_.border, param_.border));

  EXPECT_TRUE(buffer_ptr->Realloc(
      /*bitdepth=*/8, /*is_monochrome=*/false, param_.width, param_.height,
      param_.subsampling_x, param_.subsampling_y, param_.border, param_.border,
      param_.border, param_.border));

  // The first row of each plane is aligned at 16-byte boundaries.
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneY)) % 16, 0);
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneU)) % 16, 0);
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneV)) % 16, 0);

  // Subsequent rows are aligned at 16-byte boundaries.
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneY) % 16, 0);
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneU) % 16, 0);
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneV) % 16, 0);

  // Check the borders.
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneU),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneU),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneU),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneU),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneV),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneV),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneV),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneV),
            param_.border >> param_.subsampling_y);

  // Write to the upper-left corner of the border.
  uint8_t* y_buffer = buffer_ptr->buffer()->data(kPlaneY);
  int y_stride = buffer_ptr->buffer()->stride(kPlaneY);
  y_buffer[-buffer_ptr->buffer()->left_border(kPlaneY) -
           buffer_ptr->buffer()->top_border(kPlaneY) * y_stride] = 0;
  // Write to the lower-right corner of the border.
  uint8_t* v_buffer = buffer_ptr->buffer()->data(kPlaneV);
  int v_stride = buffer_ptr->buffer()->stride(kPlaneV);
  v_buffer[(buffer_ptr->buffer()->height(kPlaneV) +
            buffer_ptr->buffer()->bottom_border(kPlaneV) - 1) *
               v_stride +
           buffer_ptr->buffer()->width(kPlaneV) +
           buffer_ptr->buffer()->right_border(kPlaneV) - 1] = 0;
}

#if LIBGAV1_MAX_BITDEPTH >= 10
TEST_P(RefCountedBufferReallocTest, 10Bit) {
  InternalFrameBufferList buffer_list;
  FrameBufferSizeChangedCallback on_frame_buffer_size_changed = nullptr;
  GetFrameBufferCallback get_frame_buffer = nullptr;
  ReleaseFrameBufferCallback release_frame_buffer = nullptr;
  void* callback_private_data = nullptr;
  if (use_external_callbacks_) {
    on_frame_buffer_size_changed = OnInternalFrameBufferSizeChanged;
    get_frame_buffer = GetInternalFrameBuffer;
    release_frame_buffer = ReleaseInternalFrameBuffer;
    callback_private_data = &buffer_list;
  }

  BufferPool buffer_pool(on_frame_buffer_size_changed, get_frame_buffer,
                         release_frame_buffer, callback_private_data);

  RefCountedBufferPtr buffer_ptr = buffer_pool.GetFreeBuffer();
  EXPECT_NE(buffer_ptr, nullptr);

  const Libgav1ImageFormat image_format = ComposeImageFormat(
      /*is_monochrome=*/false, param_.subsampling_x, param_.subsampling_y);
  EXPECT_TRUE(buffer_pool.OnFrameBufferSizeChanged(
      /*bitdepth=*/8, image_format, param_.width, param_.height, param_.border,
      param_.border, param_.border, param_.border));

  EXPECT_TRUE(buffer_ptr->Realloc(
      /*bitdepth=*/10, /*is_monochrome=*/false, param_.width, param_.height,
      param_.subsampling_x, param_.subsampling_y, param_.border, param_.border,
      param_.border, param_.border));

  // The first row of each plane is aligned at 16-byte boundaries.
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneY)) % 16, 0);
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneU)) % 16, 0);
  EXPECT_EQ(
      reinterpret_cast<uintptr_t>(buffer_ptr->buffer()->data(kPlaneV)) % 16, 0);

  // Subsequent rows are aligned at 16-byte boundaries.
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneY) % 16, 0);
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneU) % 16, 0);
  EXPECT_EQ(buffer_ptr->buffer()->stride(kPlaneV) % 16, 0);

  // Check the borders.
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneY), param_.border);
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneU),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneU),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneU),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneU),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->left_border(kPlaneV),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->right_border(kPlaneV),
            param_.border >> param_.subsampling_x);
  EXPECT_EQ(buffer_ptr->buffer()->top_border(kPlaneV),
            param_.border >> param_.subsampling_y);
  EXPECT_EQ(buffer_ptr->buffer()->bottom_border(kPlaneV),
            param_.border >> param_.subsampling_y);

  // Write to the upper-left corner of the border.
  auto* y_buffer =
      reinterpret_cast<uint16_t*>(buffer_ptr->buffer()->data(kPlaneY));
  int y_stride = buffer_ptr->buffer()->stride(kPlaneY) / sizeof(uint16_t);
  y_buffer[-buffer_ptr->buffer()->left_border(kPlaneY) -
           buffer_ptr->buffer()->top_border(kPlaneY) * y_stride] = 0;
  // Write to the lower-right corner of the border.
  auto* v_buffer =
      reinterpret_cast<uint16_t*>(buffer_ptr->buffer()->data(kPlaneV));
  int v_stride = buffer_ptr->buffer()->stride(kPlaneV) / sizeof(uint16_t);
  v_buffer[(buffer_ptr->buffer()->height(kPlaneV) +
            buffer_ptr->buffer()->bottom_border(kPlaneV) - 1) *
               v_stride +
           buffer_ptr->buffer()->width(kPlaneV) +
           buffer_ptr->buffer()->right_border(kPlaneV) - 1] = 0;
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

INSTANTIATE_TEST_SUITE_P(
    Default, RefCountedBufferReallocTest,
    testing::Combine(testing::Bool(),  // use_external_callbacks
                     testing::ValuesIn(kParams)));

}  // namespace
}  // namespace libgav1
