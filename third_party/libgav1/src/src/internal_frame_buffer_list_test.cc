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

#include "src/internal_frame_buffer_list.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "src/gav1/decoder_buffer.h"
#include "src/gav1/frame_buffer.h"

namespace libgav1 {
namespace {

class InternalFrameBufferListTest : public testing::Test {
 protected:
  static constexpr int kBufferListSize = 10;

  InternalFrameBufferListTest() {
    on_frame_buffer_size_changed_ = OnInternalFrameBufferSizeChanged;
    get_frame_buffer_ = GetInternalFrameBuffer;
    release_frame_buffer_ = ReleaseInternalFrameBuffer;
    callback_private_data_ = &buffer_list_;
  }

  // Frame buffer callbacks.
  FrameBufferSizeChangedCallback on_frame_buffer_size_changed_;
  GetFrameBufferCallback get_frame_buffer_;
  ReleaseFrameBufferCallback release_frame_buffer_;
  // Private data associated with the frame buffer callbacks.
  void* callback_private_data_;

 private:
  InternalFrameBufferList buffer_list_;
};

TEST_F(InternalFrameBufferListTest, ReleaseInRandomOrder) {
  const int bitdepth = 8;
  const Libgav1ImageFormat image_format = kLibgav1ImageFormatYuv420;
  const int width = 100;
  const int height = 50;
  const int left_border = 0;
  const int right_border = 0;
  const int top_border = 0;
  const int bottom_border = 0;
  const int stride_alignment = 16;

  EXPECT_EQ(on_frame_buffer_size_changed_(callback_private_data_, bitdepth,
                                          image_format, width, height,
                                          left_border, right_border, top_border,
                                          bottom_border, stride_alignment),
            0);

  FrameBuffer frame_buffers[kBufferListSize];
  for (auto& frame_buffer : frame_buffers) {
    EXPECT_EQ(
        get_frame_buffer_(callback_private_data_, bitdepth, image_format, width,
                          height, left_border, right_border, top_border,
                          bottom_border, stride_alignment, &frame_buffer),
        0);
    EXPECT_NE(frame_buffer.plane[0], nullptr);
    EXPECT_GE(frame_buffer.stride[0], 112);
    EXPECT_NE(frame_buffer.plane[1], nullptr);
    EXPECT_GE(frame_buffer.stride[1], 64);
    EXPECT_NE(frame_buffer.plane[2], nullptr);
    EXPECT_GE(frame_buffer.stride[2], 64);
  }

  // Release and get a few buffers at indexes <= 5 in random order.
  static_assert(5 < kBufferListSize, "");
  static constexpr int indexes[] = {1, 4, 5, 5, 4, 3, 2, 3, 5, 0};
  for (int index : indexes) {
    release_frame_buffer_(callback_private_data_,
                          frame_buffers[index].private_data);

    EXPECT_EQ(get_frame_buffer_(callback_private_data_, bitdepth, image_format,
                                width, height, left_border, right_border,
                                top_border, bottom_border, stride_alignment,
                                &frame_buffers[index]),
              0);
    EXPECT_NE(frame_buffers[index].plane[0], nullptr);
    EXPECT_GE(frame_buffers[index].stride[0], 112);
    EXPECT_NE(frame_buffers[index].plane[1], nullptr);
    EXPECT_GE(frame_buffers[index].stride[1], 64);
    EXPECT_NE(frame_buffers[index].plane[2], nullptr);
    EXPECT_GE(frame_buffers[index].stride[2], 64);
  }

  for (auto& frame_buffer : frame_buffers) {
    release_frame_buffer_(callback_private_data_, frame_buffer.private_data);
  }
}

TEST_F(InternalFrameBufferListTest, VaryingBufferSizes) {
  const int bitdepth = 8;
  const Libgav1ImageFormat image_format = kLibgav1ImageFormatYuv420;
  const int width = 64;
  const int height = 48;
  const int left_border = 16;
  const int right_border = 16;
  const int top_border = 16;
  const int bottom_border = 16;
  const int stride_alignment = 16;

  EXPECT_EQ(on_frame_buffer_size_changed_(callback_private_data_, bitdepth,
                                          image_format, 16 * width, 16 * height,
                                          left_border, right_border, top_border,
                                          bottom_border, stride_alignment),
            0);

  FrameBuffer frame_buffer;
  for (int i = 1; i <= 16; ++i) {
    EXPECT_EQ(get_frame_buffer_(callback_private_data_, bitdepth, image_format,
                                i * width, i * height, left_border,
                                right_border, top_border, bottom_border,
                                stride_alignment, &frame_buffer),
              0);
    EXPECT_NE(frame_buffer.plane[0], nullptr);
    EXPECT_GE(frame_buffer.stride[0], i * width + left_border + right_border);
    EXPECT_NE(frame_buffer.plane[1], nullptr);
    EXPECT_GE(frame_buffer.stride[1],
              (i * width + left_border + right_border) >> 1);
    EXPECT_NE(frame_buffer.plane[2], nullptr);
    EXPECT_GE(frame_buffer.stride[2],
              (i * width + left_border + right_border) >> 1);
    release_frame_buffer_(callback_private_data_, frame_buffer.private_data);
  }
  for (int i = 16; i >= 1; --i) {
    EXPECT_EQ(get_frame_buffer_(callback_private_data_, bitdepth, image_format,
                                i * width, i * height, left_border,
                                right_border, top_border, bottom_border,
                                stride_alignment, &frame_buffer),
              0);
    EXPECT_NE(frame_buffer.plane[0], nullptr);
    EXPECT_GE(frame_buffer.stride[0], i * width + left_border + right_border);
    EXPECT_NE(frame_buffer.plane[1], nullptr);
    EXPECT_GE(frame_buffer.stride[1],
              (i * width + left_border + right_border) >> 1);
    EXPECT_NE(frame_buffer.plane[2], nullptr);
    EXPECT_GE(frame_buffer.stride[2],
              (i * width + left_border + right_border) >> 1);
    release_frame_buffer_(callback_private_data_, frame_buffer.private_data);
  }
}

}  // namespace
}  // namespace libgav1
