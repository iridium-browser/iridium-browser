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

#include "src/post_filter.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "src/dsp/cdef.h"
#include "src/dsp/dsp.h"
#include "src/dsp/super_res.h"
#include "src/frame_scratch_buffer.h"
#include "src/obu_parser.h"
#include "src/threading_strategy.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/memory.h"
#include "src/utils/types.h"
#include "src/yuv_buffer.h"
#include "tests/block_utils.h"
#include "tests/third_party/libvpx/acm_random.h"
#include "tests/utils.h"

namespace libgav1 {
namespace {

constexpr char kCdef[] = "Cdef";
constexpr char kApplyCdefName[] = "ApplyCdef";
constexpr int kMaxBlockWidth4x4 = 32;
constexpr int kMaxBlockHeight4x4 = 32;
constexpr int kMaxTestFrameSize = 1920 * 1080;

int GetIdFromInputParam(int subsampling_x, int subsampling_y, int height) {
  int id = subsampling_x * 8 + subsampling_y * 4;
  if (height == 288) {
    id += 0;
  } else if (height == 480) {
    id += 1;
  } else if (height == 1080) {
    id += 2;
  } else {
    id += 3;
  }
  return id;
}

const char* GetSuperResDigest8bpp(int id, int plane) {
  static const char* const kDigestSuperRes[][kMaxPlanes] = {
      {
          // all input is 0.
          "ff5f7a63d3b1f9176e216eb01a0387ad",  // kPlaneY.
          "38b6551d7ac3e86c8af407d5a1aa36dc",  // kPlaneU.
          "38b6551d7ac3e86c8af407d5a1aa36dc",  // kPlaneV.
      },
      {
          // all input is 1.
          "819f21dcce0e779180bbd613a9e3543c",  // kPlaneY.
          "e784bfa8f517d83b014c3dcd45b780a5",  // kPlaneU.
          "e784bfa8f517d83b014c3dcd45b780a5",  // kPlaneV.
      },
      {
          // all input is 128.
          "2d6ea5b39f9168d56c2e2b8846d208ec",  // kPlaneY.
          "8030b6e70f1544efbc37b902d3f88bd3",  // kPlaneU.
          "8030b6e70f1544efbc37b902d3f88bd3",  // kPlaneV.
      },
      {
          // all input is 255.
          "5c0b4bc50e0980dc6ba7c042d3b50a5e",  // kPlaneY.
          "3c566ef847c45be09ddac297123a3bad",  // kPlaneU.
          "3c566ef847c45be09ddac297123a3bad",  // kPlaneV.
      },
      {
          // random input.
          "50514467dd6a5c3a8268eddaa542c41f",  // kPlaneY.
          "3ce720c2b5b44928e1477b11040e5c00",  // kPlaneU.
          "3ce720c2b5b44928e1477b11040e5c00",  // kPlaneV.
      },
  };
  return kDigestSuperRes[id][plane];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetSuperResDigest10bpp(int id, int plane) {
  // Digests are in Y/U/V order.
  static const char* const kDigestSuperRes[][kMaxPlanes] = {
      {
          // all input is 0.
          "fccb1f57b252b1a86d335aea929d1d58",
          "2f244a56091c9705794e92e6bcc38058",
          "2f244a56091c9705794e92e6bcc38058",
      },
      {
          // all input is 1.
          "de8556204999d6e4bf74cfdde61a095b",
          "e7d0f4ce6df81c46de95da7790a67384",
          "e7d0f4ce6df81c46de95da7790a67384",
      },
      {
          // all input is 512.
          "d3b6980363eb9b808885537b3485af87",
          "bcffddb26210da6861e7b31414e58b77",
          "bcffddb26210da6861e7b31414e58b77",
      },
      {
          // all input is 1023.
          "ce0762aeee1cdef1db101e4ca39bcbd6",
          "33aeaa7f5d7c032e3dfda43925c3dcb2",
          "33aeaa7f5d7c032e3dfda43925c3dcb2",
      },
      {
          // random input.
          "63c701bceb187ffa535be15ae58f8171",
          "f570e30e9ea8d2a1e6d99202cd2f8994",
          "f570e30e9ea8d2a1e6d99202cd2f8994",
      },
  };
  return kDigestSuperRes[id][plane];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetSuperResDigest12bpp(int id, int plane) {
  // Digests are in Y/U/V order.
  static const char* const kDigestSuperRes[][kMaxPlanes] = {
      {
          // all input is 0.
          "fccb1f57b252b1a86d335aea929d1d58",
          "2f244a56091c9705794e92e6bcc38058",
          "2f244a56091c9705794e92e6bcc38058",
      },
      {
          // all input is 1.
          "de8556204999d6e4bf74cfdde61a095b",
          "e7d0f4ce6df81c46de95da7790a67384",
          "e7d0f4ce6df81c46de95da7790a67384",
      },
      {
          // all input is 2048.
          "83d600a7b3dc9bc3f710668ee2244e6b",
          "468eec1453edc1befeb8a346f61950a7",
          "468eec1453edc1befeb8a346f61950a7",
      },
      {
          // all input is 4095.
          "30bdb1dfee2b02b12b38e6b9f6287e27",
          "34d673f075d2caa93a2f648ee3569e20",
          "34d673f075d2caa93a2f648ee3569e20",
      },
      {
          // random input.
          "f10f21f5322231d991550fce7ef9787d",
          "a2d8b6140bd5002e86644ef433b8eb42",
          "a2d8b6140bd5002e86644ef433b8eb42",
      },
  };
  return kDigestSuperRes[id][plane];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

// This type is used to parameterize the tests so is defined outside the
// anonymous namespace to avoid the GCC -Wsubobject-linkage warning.
struct FrameSizeParam {
  FrameSizeParam(uint32_t width, uint32_t upscaled_width, uint32_t height,
                 int8_t ss_x, int8_t ss_y)
      : width(width),
        upscaled_width(upscaled_width),
        height(height),
        subsampling_x(ss_x),
        subsampling_y(ss_y) {}
  uint32_t width;
  uint32_t upscaled_width;
  uint32_t height;
  int8_t subsampling_x;
  int8_t subsampling_y;
};

// Print operators must be defined in the same namespace as the type for the
// lookup to work correctly.
static std::ostream& operator<<(std::ostream& os, const FrameSizeParam& param) {
  return os << param.width << "x" << param.height
            << ", upscaled_width: " << param.upscaled_width
            << ", subsampling(x/y): " << static_cast<int>(param.subsampling_x)
            << "/" << static_cast<int>(param.subsampling_y);
}

// Note the following test classes access private functions/members of
// PostFilter. To be declared friends of PostFilter they must not have internal
// linkage (they must be outside the anonymous namespace).
template <int bitdepth, typename Pixel>
class PostFilterTestBase : public testing::TestWithParam<FrameSizeParam> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  PostFilterTestBase() = default;
  PostFilterTestBase(const PostFilterTestBase&) = delete;
  PostFilterTestBase& operator=(const PostFilterTestBase&) = delete;
  ~PostFilterTestBase() override = default;

  void SetUp() override {
    // Allocate buffer_ with a border size of kBorderPixels (which is
    // subsampled for chroma planes). Some tests (for loop restoration) only use
    // the nearest 2 or 3 pixels (for both luma and chroma planes) in the
    // border.
    ASSERT_TRUE(buffer_.Realloc(
        bitdepth, /*is_monochrome=*/false, frame_size_.upscaled_width,
        frame_size_.height, frame_size_.subsampling_x,
        frame_size_.subsampling_y, kBorderPixels, kBorderPixels, kBorderPixels,
        kBorderPixels, nullptr, nullptr, nullptr));

    ASSERT_TRUE(loop_restoration_border_.Realloc(
        bitdepth, /*is_monochrome=*/false, frame_size_.upscaled_width,
        frame_size_.height, frame_size_.subsampling_x,
        frame_size_.subsampling_y, kBorderPixels, kBorderPixels, kBorderPixels,
        kBorderPixels, nullptr, nullptr, nullptr));

    for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
      const int8_t subsampling_x =
          (plane == kPlaneY) ? 0 : frame_size_.subsampling_x;
      const int8_t subsampling_y =
          (plane == kPlaneY) ? 0 : frame_size_.subsampling_y;
      width_[plane] = frame_size_.width >> subsampling_x;
      upscaled_width_[plane] = frame_size_.upscaled_width >> subsampling_x;
      stride_[plane] =
          (frame_size_.upscaled_width + 2 * kBorderPixels) >> subsampling_x;
      height_[plane] =
          (frame_size_.height + 2 * kBorderPixels) >> subsampling_y;

      reference_buffer_[plane].reserve(stride_[plane] * height_[plane]);
      reference_buffer_[plane].resize(stride_[plane] * height_[plane]);
      std::fill(reference_buffer_[plane].begin(),
                reference_buffer_[plane].end(), 0);
    }
  }

 protected:
  YuvBuffer buffer_;
  YuvBuffer cdef_border_;
  YuvBuffer loop_restoration_border_;
  uint32_t width_[kMaxPlanes];
  uint32_t upscaled_width_[kMaxPlanes];
  uint32_t stride_[kMaxPlanes];
  uint32_t height_[kMaxPlanes];
  std::vector<Pixel> reference_buffer_[kMaxPlanes];
  const FrameSizeParam frame_size_ = GetParam();
};

template <int bitdepth, typename Pixel>
class PostFilterHelperFuncTest : public PostFilterTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  PostFilterHelperFuncTest() = default;
  PostFilterHelperFuncTest(const PostFilterHelperFuncTest&) = delete;
  PostFilterHelperFuncTest& operator=(const PostFilterHelperFuncTest&) = delete;
  ~PostFilterHelperFuncTest() override = default;

 protected:
  using PostFilterTestBase<bitdepth, Pixel>::buffer_;
  using PostFilterTestBase<bitdepth, Pixel>::cdef_border_;
  using PostFilterTestBase<bitdepth, Pixel>::loop_restoration_border_;
  using PostFilterTestBase<bitdepth, Pixel>::width_;
  using PostFilterTestBase<bitdepth, Pixel>::upscaled_width_;
  using PostFilterTestBase<bitdepth, Pixel>::stride_;
  using PostFilterTestBase<bitdepth, Pixel>::height_;
  using PostFilterTestBase<bitdepth, Pixel>::reference_buffer_;
  using PostFilterTestBase<bitdepth, Pixel>::frame_size_;

  void SetUp() override {
    PostFilterTestBase<bitdepth, Pixel>::SetUp();

    for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
      const int8_t subsampling_x =
          (plane == kPlaneY) ? 0 : frame_size_.subsampling_x;
      const int8_t subsampling_y =
          (plane == kPlaneY) ? 0 : frame_size_.subsampling_y;
      width_[plane] = frame_size_.width >> subsampling_x;
      upscaled_width_[plane] = frame_size_.upscaled_width >> subsampling_x;
      stride_[plane] = (frame_size_.upscaled_width >> subsampling_x) +
                       2 * kRestorationHorizontalBorder;
      height_[plane] = (frame_size_.height >> subsampling_y) +
                       2 * kRestorationVerticalBorder;
      reference_buffer_[plane].reserve(stride_[plane] * height_[plane]);
      reference_buffer_[plane].resize(stride_[plane] * height_[plane]);
      std::fill(reference_buffer_[plane].begin(),
                reference_buffer_[plane].end(), 0);
      buffer_border_corner_[plane] =
          reinterpret_cast<Pixel*>(buffer_.data(plane)) -
          buffer_.stride(plane) / sizeof(Pixel) * kRestorationVerticalBorder -
          kRestorationHorizontalBorder;
      loop_restoration_border_corner_[plane] =
          reinterpret_cast<Pixel*>(loop_restoration_border_.data(plane)) -
          loop_restoration_border_.stride(plane) / sizeof(Pixel) *
              kRestorationVerticalBorder -
          kRestorationHorizontalBorder;
    }
  }

  void TestExtendFrame(bool use_fixed_values, Pixel value);
  void TestAdjustFrameBufferPointer();
  void TestPrepareLoopRestorationBlock();

  // Fill the frame buffer with either a fixed value, or random values.
  // If fill in with random values, make special operations at buffer
  // boundaries. Make the outer most 3 pixel wide borders the same value
  // as their immediate inner neighbor. For example:
  // 4 4 4   4 5 6   6 6 6
  // 4 4 4   4 5 6   6 6 6
  // 4 4 4   4 5 6   6 6 6
  //       ---------
  // 4 4 4 | 4 5 6 | 6 6 6
  // 1 1 1 | 1 0 1 | 1 1 1
  // 0 0 0 | 0 1 0 | 0 0 0
  // 1 1 1 | 1 0 1 | 1 1 1
  // 0 0 0 | 0 1 0 | 0 0 0
  // 6 6 6 | 6 5 4 | 4 4 4
  //        -------
  // 6 6 6   6 5 4   4 4 4
  // 6 6 6   6 5 4   4 4 4
  // 6 6 6   6 5 4   4 4 4
  // Pixels within box is the current block. Outside is extended area from it.
  void FillBuffer(bool use_fixed_values, Pixel value);

  // Points to the upper left corner of the restoration border in buffer_.
  Pixel* buffer_border_corner_[kMaxPlanes];
  // Points to the upper left corner of the restoration border in
  // loop_restoration_border_.
  Pixel* loop_restoration_border_corner_[kMaxPlanes];
};

template <int bitdepth, typename Pixel>
void PostFilterHelperFuncTest<bitdepth, Pixel>::FillBuffer(
    bool use_fixed_values, Pixel value) {
  if (use_fixed_values) {
    for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
      // Fill buffer with a fixed value.
      std::fill(reference_buffer_[plane].begin(),
                reference_buffer_[plane].end(), value);
      // Fill frame buffer. Note that the border is not filled.
      auto* row = reinterpret_cast<Pixel*>(buffer_.data(plane));
      for (int i = 0; i < buffer_.height(plane); ++i) {
        std::fill(row, row + width_[plane], value);
        row += buffer_.stride(plane) / sizeof(Pixel);
      }
    }
  } else {  // Random value.
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    const int mask = (1 << bitdepth) - 1;
    for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
      // Fill buffer with random values.
      std::vector<Pixel> line_buffer(stride_[plane]);
      std::fill(line_buffer.begin(), line_buffer.end(), 0);
      for (int i = kRestorationHorizontalBorder;
           i < stride_[plane] - kRestorationHorizontalBorder; ++i) {
        line_buffer[i] = rnd.Rand16() & mask;
      }
      // Copy boundary values to extended border.
      for (int i = 0; i < kRestorationHorizontalBorder; ++i) {
        line_buffer[i] = line_buffer[kRestorationHorizontalBorder];
        line_buffer[stride_[plane] - i - 1] =
            line_buffer[stride_[plane] - 1 - kRestorationHorizontalBorder];
      }
      // The first three rows are the same as the line_buffer.
      for (int i = 0; i < kRestorationVerticalBorder + 1; ++i) {
        std::copy(line_buffer.begin(), line_buffer.end(),
                  reference_buffer_[plane].begin() + i * stride_[plane]);
      }
      for (int i = kRestorationVerticalBorder + 1;
           i < height_[plane] - kRestorationVerticalBorder; ++i) {
        for (int j = kRestorationHorizontalBorder;
             j < stride_[plane] - kRestorationHorizontalBorder; ++j) {
          line_buffer[j] = rnd.Rand16() & mask;
        }
        for (int j = 0; j < kRestorationHorizontalBorder; ++j) {
          line_buffer[j] = line_buffer[kRestorationHorizontalBorder];
          line_buffer[stride_[plane] - j - 1] =
              line_buffer[stride_[plane] - 1 - kRestorationHorizontalBorder];
        }
        std::copy(line_buffer.begin(), line_buffer.end(),
                  reference_buffer_[plane].begin() + i * stride_[plane]);
      }
      // The extended border are the same as the line_buffer.
      for (int i = 0; i < kRestorationVerticalBorder; ++i) {
        std::copy(line_buffer.begin(), line_buffer.end(),
                  reference_buffer_[plane].begin() +
                      (height_[plane] - kRestorationVerticalBorder + i) *
                          stride_[plane]);
      }

      // Fill frame buffer. Note that the border is not filled.
      for (int i = 0; i < buffer_.height(plane); ++i) {
        memcpy(buffer_.data(plane) + i * buffer_.stride(plane),
               reference_buffer_[plane].data() + kRestorationHorizontalBorder +
                   (i + kRestorationVerticalBorder) * stride_[plane],
               sizeof(Pixel) * width_[plane]);
      }
    }
  }
}

template <int bitdepth, typename Pixel>
void PostFilterHelperFuncTest<bitdepth, Pixel>::TestExtendFrame(
    bool use_fixed_values, Pixel value) {
  ObuFrameHeader frame_header = {};
  frame_header.upscaled_width = frame_size_.upscaled_width;
  frame_header.width = frame_size_.width;
  frame_header.height = frame_size_.height;
  ObuSequenceHeader sequence_header;
  sequence_header.color_config.bitdepth = bitdepth;
  sequence_header.color_config.is_monochrome = false;
  sequence_header.color_config.subsampling_x = frame_size_.subsampling_x;
  sequence_header.color_config.subsampling_y = frame_size_.subsampling_y;

  const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
  ASSERT_NE(dsp, nullptr);
  FrameScratchBuffer frame_scratch_buffer;

  PostFilter post_filter(frame_header, sequence_header, &frame_scratch_buffer,
                         &buffer_, dsp,
                         /*do_post_filter_mask=*/0x00);
  FillBuffer(use_fixed_values, value);
  for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
    const int plane_width =
        plane == kPlaneY ? frame_header.upscaled_width
                         : frame_header.upscaled_width >>
                               sequence_header.color_config.subsampling_x;
    const int plane_height =
        plane == kPlaneY
            ? frame_header.height
            : frame_header.height >> sequence_header.color_config.subsampling_y;
    PostFilter::ExtendFrame<Pixel>(
        reinterpret_cast<Pixel*>(buffer_.data(plane)), plane_width,
        plane_height, buffer_.stride(plane) / sizeof(Pixel),
        kRestorationHorizontalBorder, kRestorationHorizontalBorder,
        kRestorationVerticalBorder, kRestorationVerticalBorder);
    const bool success = test_utils::CompareBlocks<Pixel>(
        buffer_border_corner_[plane], reference_buffer_[plane].data(),
        stride_[plane], height_[plane], buffer_.stride(plane) / sizeof(Pixel),
        stride_[plane], /*check_padding=*/false, /*print_diff=*/false);
    ASSERT_TRUE(success) << "Failure of extend frame at plane: " << plane;
  }
}

template <int bitdepth, typename Pixel>
class PostFilterSuperResTest : public PostFilterTestBase<bitdepth, Pixel> {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  PostFilterSuperResTest() {
    test_utils::ResetDspTable(bitdepth);
    dsp::SuperResInit_C();
    dsp::SuperResInit_SSE4_1();
    dsp::SuperResInit_NEON();
  }
  PostFilterSuperResTest(const PostFilterSuperResTest&) = delete;
  PostFilterSuperResTest& operator=(const PostFilterSuperResTest&) = delete;
  ~PostFilterSuperResTest() override = default;

 protected:
  using PostFilterTestBase<bitdepth, Pixel>::buffer_;
  using PostFilterTestBase<bitdepth, Pixel>::width_;
  using PostFilterTestBase<bitdepth, Pixel>::upscaled_width_;
  using PostFilterTestBase<bitdepth, Pixel>::stride_;
  using PostFilterTestBase<bitdepth, Pixel>::height_;
  using PostFilterTestBase<bitdepth, Pixel>::reference_buffer_;
  using PostFilterTestBase<bitdepth, Pixel>::frame_size_;

  void TestApplySuperRes(bool use_fixed_values, Pixel value, int id,
                         bool multi_threaded);
};

// This class must be in namespace libgav1 to access private member function
// of class PostFilter in src/post_filter.h.
template <int bitdepth, typename Pixel>
void PostFilterSuperResTest<bitdepth, Pixel>::TestApplySuperRes(
    bool use_fixed_values, Pixel value, int id, bool multi_threaded) {
  ObuFrameHeader frame_header = {};
  frame_header.width = frame_size_.width;
  frame_header.upscaled_width = frame_size_.upscaled_width;
  frame_header.height = frame_size_.height;
  frame_header.rows4x4 = DivideBy4(frame_size_.height);
  frame_header.columns4x4 = DivideBy4(frame_size_.width);
  frame_header.tile_info.tile_count = 1;
  ObuSequenceHeader sequence_header;
  sequence_header.color_config.bitdepth = bitdepth;
  sequence_header.color_config.is_monochrome = false;
  sequence_header.color_config.subsampling_x = frame_size_.subsampling_x;
  sequence_header.color_config.subsampling_y = frame_size_.subsampling_y;

  // Apply SuperRes.
  Array2D<int16_t> cdef_index;
  Array2D<TransformSize> inter_transform_sizes;
  const dsp::Dsp* const dsp = dsp::GetDspTable(bitdepth);
  ASSERT_NE(dsp, nullptr);
  constexpr int kNumThreads = 4;
  FrameScratchBuffer frame_scratch_buffer;
  if (multi_threaded) {
    ASSERT_TRUE(frame_scratch_buffer.threading_strategy.Reset(frame_header,
                                                              kNumThreads));
  }
  const int pixel_size = sequence_header.color_config.bitdepth == 8
                             ? sizeof(uint8_t)
                             : sizeof(uint16_t);
  ASSERT_TRUE(frame_scratch_buffer.superres_coefficients[kPlaneTypeY].Resize(
      kSuperResFilterTaps * Align(frame_header.upscaled_width, 16) *
      pixel_size));
  if (!sequence_header.color_config.is_monochrome &&
      sequence_header.color_config.subsampling_x != 0) {
    ASSERT_TRUE(frame_scratch_buffer.superres_coefficients[kPlaneTypeUV].Resize(
        kSuperResFilterTaps *
        Align(SubsampledValue(frame_header.upscaled_width, 1), 16) *
        pixel_size));
  }
  ASSERT_TRUE(frame_scratch_buffer.superres_line_buffer.Realloc(
      sequence_header.color_config.bitdepth,
      sequence_header.color_config.is_monochrome,
      MultiplyBy4(frame_header.columns4x4), (multi_threaded ? kNumThreads : 1),
      sequence_header.color_config.subsampling_x,
      /*subsampling_y=*/0, 2 * kSuperResHorizontalBorder,
      2 * (kSuperResHorizontalBorder + kSuperResHorizontalPadding), 0, 0,
      nullptr, nullptr, nullptr));
  PostFilter post_filter(frame_header, sequence_header, &frame_scratch_buffer,
                         &buffer_, dsp,
                         /*do_post_filter_mask=*/0x04);

  const int num_planes = sequence_header.color_config.is_monochrome
                             ? kMaxPlanesMonochrome
                             : kMaxPlanes;
  int width[kMaxPlanes];
  int upscaled_width[kMaxPlanes];
  int height[kMaxPlanes];

  for (int plane = kPlaneY; plane < num_planes; ++plane) {
    const int8_t subsampling_x =
        (plane == kPlaneY) ? 0 : frame_size_.subsampling_x;
    const int8_t subsampling_y =
        (plane == kPlaneY) ? 0 : frame_size_.subsampling_y;
    width[plane] = frame_size_.width >> subsampling_x;
    upscaled_width[plane] = frame_size_.upscaled_width >> subsampling_x;
    height[plane] = frame_size_.height >> subsampling_y;
    if (use_fixed_values) {
      auto* src = reinterpret_cast<Pixel*>(post_filter.cdef_buffer_[plane]);
      for (int y = 0; y < height[plane]; ++y) {
        for (int x = 0; x < width[plane]; ++x) {
          src[x] = value;
        }
        src += buffer_.stride(plane) / sizeof(Pixel);
      }
    } else {  // Random input.
      const int mask = (1 << bitdepth) - 1;
      libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
      auto* src = reinterpret_cast<Pixel*>(post_filter.cdef_buffer_[plane]);
      for (int y = 0; y < height[plane]; ++y) {
        for (int x = 0; x < width[plane]; ++x) {
          src[x] = rnd.Rand16() & mask;
        }
        src += buffer_.stride(plane) / sizeof(Pixel);
      }
    }
  }

  if (multi_threaded) {
    post_filter.ApplySuperResThreaded();
  } else {
    std::array<uint8_t*, kMaxPlanes> buffers = {
        post_filter.cdef_buffer_[kPlaneY], post_filter.cdef_buffer_[kPlaneU],
        post_filter.cdef_buffer_[kPlaneV]};
    std::array<uint8_t*, kMaxPlanes> dst = {
        post_filter.GetSuperResBuffer(static_cast<Plane>(kPlaneY), 0, 0),
        post_filter.GetSuperResBuffer(static_cast<Plane>(kPlaneU), 0, 0),
        post_filter.GetSuperResBuffer(static_cast<Plane>(kPlaneV), 0, 0)};
    std::array<int, kMaxPlanes> rows = {
        frame_header.rows4x4 * 4,
        (frame_header.rows4x4 * 4) >> frame_size_.subsampling_y,
        (frame_header.rows4x4 * 4) >> frame_size_.subsampling_y};
    post_filter.ApplySuperRes(buffers, rows, /*line_buffer_row=*/-1, dst);
  }

  // Check md5.
  std::vector<Pixel> output;
  for (int plane = kPlaneY; plane < num_planes; ++plane) {
    output.reserve(upscaled_width[plane] * height[plane]);
    output.resize(upscaled_width[plane] * height[plane]);
    auto* dst = reinterpret_cast<Pixel*>(
        post_filter.GetSuperResBuffer(static_cast<Plane>(plane), 0, 0));
    for (int y = 0; y < height[plane]; ++y) {
      for (int x = 0; x < upscaled_width[plane]; ++x) {
        output[y * upscaled_width[plane] + x] = dst[x];
      }
      dst += buffer_.stride(plane) / sizeof(Pixel);
    }
    const std::string digest = test_utils::GetMd5Sum(
        output.data(), upscaled_width[plane] * height[plane] * sizeof(Pixel));
    printf("MD5: %s\n", digest.c_str());
    const char* expected_digest = nullptr;
    switch (bitdepth) {
      case 8:
        expected_digest = GetSuperResDigest8bpp(id, plane);
        break;
#if LIBGAV1_MAX_BITDEPTH >= 10
      case 10:
        expected_digest = GetSuperResDigest10bpp(id, plane);
        break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
      case 12:
        expected_digest = GetSuperResDigest12bpp(id, plane);
        break;
#endif
    }
    ASSERT_NE(expected_digest, nullptr);
    EXPECT_STREQ(digest.c_str(), expected_digest);
  }
}

using PostFilterSuperResTest8bpp = PostFilterSuperResTest<8, uint8_t>;

const FrameSizeParam kTestParamSuperRes[] = {
    FrameSizeParam(176, 352, 288, 1, 1)};

TEST_P(PostFilterSuperResTest8bpp, ApplySuperRes) {
  TestApplySuperRes(true, 0, 0, false);
  TestApplySuperRes(true, 1, 1, false);
  TestApplySuperRes(true, 128, 2, false);
  TestApplySuperRes(true, 255, 3, false);
  TestApplySuperRes(false, 0, 4, false);
}

TEST_P(PostFilterSuperResTest8bpp, ApplySuperResThreaded) {
  TestApplySuperRes(true, 0, 0, true);
  TestApplySuperRes(true, 1, 1, true);
  TestApplySuperRes(true, 128, 2, true);
  TestApplySuperRes(true, 255, 3, true);
  TestApplySuperRes(false, 0, 4, true);
}

INSTANTIATE_TEST_SUITE_P(PostFilterSuperResTestInstance,
                         PostFilterSuperResTest8bpp,
                         testing::ValuesIn(kTestParamSuperRes));

using PostFilterHelperFuncTest8bpp = PostFilterHelperFuncTest<8, uint8_t>;

const FrameSizeParam kTestParamExtendFrame[] = {
    FrameSizeParam(16, 16, 16, 1, 1),
    FrameSizeParam(64, 64, 64, 1, 1),
    FrameSizeParam(128, 128, 64, 1, 1),
    FrameSizeParam(64, 64, 128, 1, 1),
    FrameSizeParam(352, 352, 288, 1, 1),
    FrameSizeParam(720, 720, 480, 1, 1),
    FrameSizeParam(1080, 1080, 720, 1, 1),
    FrameSizeParam(16, 16, 16, 0, 0),
    FrameSizeParam(64, 64, 64, 0, 0),
    FrameSizeParam(128, 128, 64, 0, 0),
    FrameSizeParam(64, 64, 128, 0, 0),
    FrameSizeParam(352, 352, 288, 0, 0),
    FrameSizeParam(720, 720, 480, 0, 0),
    FrameSizeParam(1080, 1080, 720, 0, 0)};

TEST_P(PostFilterHelperFuncTest8bpp, ExtendFrame) {
  TestExtendFrame(true, 0);
  TestExtendFrame(true, 1);
  TestExtendFrame(true, 128);
  TestExtendFrame(true, 255);
  TestExtendFrame(false, 0);
}

INSTANTIATE_TEST_SUITE_P(PostFilterHelperFuncTestInstance,
                         PostFilterHelperFuncTest8bpp,
                         testing::ValuesIn(kTestParamExtendFrame));

#if LIBGAV1_MAX_BITDEPTH >= 10
using PostFilterSuperResTest10bpp = PostFilterSuperResTest<10, uint16_t>;

TEST_P(PostFilterSuperResTest10bpp, ApplySuperRes) {
  TestApplySuperRes(true, 0, 0, false);
  TestApplySuperRes(true, 1, 1, false);
  TestApplySuperRes(true, 1 << 9, 2, false);
  TestApplySuperRes(true, (1 << 10) - 1, 3, false);
  TestApplySuperRes(false, 0, 4, false);
}

TEST_P(PostFilterSuperResTest10bpp, ApplySuperResThreaded) {
  TestApplySuperRes(true, 0, 0, true);
  TestApplySuperRes(true, 1, 1, true);
  TestApplySuperRes(true, 1 << 9, 2, true);
  TestApplySuperRes(true, (1 << 10) - 1, 3, true);
  TestApplySuperRes(false, 0, 4, true);
}

INSTANTIATE_TEST_SUITE_P(PostFilterSuperResTestInstance,
                         PostFilterSuperResTest10bpp,
                         testing::ValuesIn(kTestParamSuperRes));

using PostFilterHelperFuncTest10bpp = PostFilterHelperFuncTest<10, uint16_t>;

TEST_P(PostFilterHelperFuncTest10bpp, ExtendFrame) {
  TestExtendFrame(true, 0);
  TestExtendFrame(true, 1);
  TestExtendFrame(true, 255);
  TestExtendFrame(true, (1 << 10) - 1);
  TestExtendFrame(false, 0);
}

INSTANTIATE_TEST_SUITE_P(PostFilterHelperFuncTestInstance,
                         PostFilterHelperFuncTest10bpp,
                         testing::ValuesIn(kTestParamExtendFrame));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using PostFilterSuperResTest12bpp = PostFilterSuperResTest<12, uint16_t>;

TEST_P(PostFilterSuperResTest12bpp, ApplySuperRes) {
  TestApplySuperRes(true, 0, 0, false);
  TestApplySuperRes(true, 1, 1, false);
  TestApplySuperRes(true, 1 << 11, 2, false);
  TestApplySuperRes(true, (1 << 12) - 1, 3, false);
  TestApplySuperRes(false, 0, 4, false);
}

TEST_P(PostFilterSuperResTest12bpp, ApplySuperResThreaded) {
  TestApplySuperRes(true, 0, 0, true);
  TestApplySuperRes(true, 1, 1, true);
  TestApplySuperRes(true, 1 << 11, 2, true);
  TestApplySuperRes(true, (1 << 12) - 1, 3, true);
  TestApplySuperRes(false, 0, 4, true);
}

INSTANTIATE_TEST_SUITE_P(PostFilterSuperResTestInstance,
                         PostFilterSuperResTest12bpp,
                         testing::ValuesIn(kTestParamSuperRes));

using PostFilterHelperFuncTest12bpp = PostFilterHelperFuncTest<12, uint16_t>;

TEST_P(PostFilterHelperFuncTest12bpp, ExtendFrame) {
  TestExtendFrame(true, 0);
  TestExtendFrame(true, 1);
  TestExtendFrame(true, 255);
  TestExtendFrame(true, (1 << 12) - 1);
  TestExtendFrame(false, 0);
}

INSTANTIATE_TEST_SUITE_P(PostFilterHelperFuncTestInstance,
                         PostFilterHelperFuncTest12bpp,
                         testing::ValuesIn(kTestParamExtendFrame));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

namespace {

const char* GetDigestApplyCdef8bpp(int id) {
  static const char* const kDigest[] = {
      "9593af24f9c6faecce53437f6e128edf", "ecb633cc2ecd6e7e0cf39d4439f4a6ea",
      "9ec4cb4124f0a686a7bda72b447f5b8e", "7ebd859a23162bc864a69dbea60bc687",
      "de7a15fc00664692a794aa68cf695980", "cf3fc8fe041f68d31ab4e34ad3643541",
      "94c116b191b0268cf7ab4a0e6996e1ec", "1ad60c943a5a914aba7bc26706620a05",
      "ce33c6f80e3608c4d18c49be2e393c20", "e140586ffc663798b74b8f6fb5b44736",
      "b7379bba8bcb97f09a74655f4e0eee91", "02ce174061c98babd3987461b3984e47",
      "64655dd1dfba8317e27d2fdcb211b7b4", "eeb6a61c70c5ee75a4c31dc5099b4dfb",
      "ee944b31148fa2e30938084f7c046464", "db7b63497750fa4c51cf45c56a2da01c",
  };
  return kDigest[id];
}

#if LIBGAV1_MAX_BITDEPTH >= 10
const char* GetDigestApplyCdef10bpp(int id) {
  static const char* const kDigest[] = {
      "53f8d68ac7f3aea65151b2066f8501c9", "021e70d5406fa182dd9713380eb66d1d",
      "bab1c84e7f06b87d81617d2d0a194b89", "58e302ff0522f64901909fb97535b270",
      "5ff95a6a798eadc7207793c03d898ce4", "1483d28cc0f1bfffedd1128966719aa0",
      "6af5a36890b465ae962c2878af874f70", "bd1ed4a2ff09d323ab98190d1805a010",
      "5ff95a6a798eadc7207793c03d898ce4", "1483d28cc0f1bfffedd1128966719aa0",
      "6af5a36890b465ae962c2878af874f70", "bd1ed4a2ff09d323ab98190d1805a010",
      "6f0299645cd6f0655fd26044cd43a37c", "56d7febf5bbebdc82e8f157ab926a0bb",
      "f54654f11006453f496be5883216a3bb", "9abc6e3230792ba78bcc65504a62075e",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
const char* GetDigestApplyCdef12bpp(int id) {
  static const char* const kDigest[] = {
      "06e2d09b6ce3924f3b5d4c00ab76eea5", "287240e4b13cb75e17932a3dd7ba3b3c",
      "265da123e3347c4fb3e434f26a3949e7", "e032ce6eb76242df6894482ac6688406",
      "f648328221f0f02a5b7fc3d55a66271a", "8f759aa84a110902025dacf8062d2f6a",
      "592b49e4b993d6b4634d8eb1ee3bba54", "29a3e8e329ec70d06910e982ea763e6b",
      "f648328221f0f02a5b7fc3d55a66271a", "8f759aa84a110902025dacf8062d2f6a",
      "592b49e4b993d6b4634d8eb1ee3bba54", "29a3e8e329ec70d06910e982ea763e6b",
      "155dd4283f8037f86cce34b6cfe67a7e", "0a022c70ead199517af9bad2002d70cd",
      "a966dfea52a7a2084545f68b2c9e1735", "e098438a23a7c9f276e594b98b2db922",
  };
  return kDigest[id];
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

template <int bitdepth, typename Pixel>
class PostFilterApplyCdefTest : public testing::TestWithParam<FrameSizeParam>,
                                public test_utils::MaxAlignedAllocable {
 public:
  static_assert(bitdepth >= kBitdepth8 && bitdepth <= LIBGAV1_MAX_BITDEPTH, "");
  PostFilterApplyCdefTest() = default;
  PostFilterApplyCdefTest(const PostFilterApplyCdefTest&) = delete;
  PostFilterApplyCdefTest& operator=(const PostFilterApplyCdefTest&) = delete;
  ~PostFilterApplyCdefTest() override = default;

 protected:
  void SetUp() override {
    test_utils::ResetDspTable(bitdepth);
    dsp::CdefInit_C();
    dsp::CdefInit_SSE4_1();
    dsp::CdefInit_NEON();

    dsp_ = dsp::GetDspTable(bitdepth);
    ASSERT_NE(dsp_, nullptr);
  }

  // Sets sequence_header_, frame_header_, cdef_index_ and cdef_skip_.
  // Allocates yuv_buffer_ but does not set it.
  void SetInput(libvpx_test::ACMRandom* rnd);
  // Sets yuv_buffer_.
  void SetInputBuffer(libvpx_test::ACMRandom* rnd, PostFilter* post_filter);
  void CopyFilterOutputToDestBuffer();
  void TestMultiThread(int num_threads);

  ObuSequenceHeader sequence_header_;
  ObuFrameHeader frame_header_ = {};
  FrameScratchBuffer frame_scratch_buffer_;
  YuvBuffer yuv_buffer_;
  const dsp::Dsp* dsp_;
  FrameSizeParam param_ = GetParam();
  Pixel dest_[kMaxTestFrameSize * kMaxPlanes];
  const size_t y_size_ = param_.width * param_.height;
  const size_t uv_size_ = y_size_ >>
                          (param_.subsampling_x + param_.subsampling_y);
  const size_t size_ = y_size_ + uv_size_ * 2;
};

template <int bitdepth, typename Pixel>
void PostFilterApplyCdefTest<bitdepth, Pixel>::SetInput(
    libvpx_test::ACMRandom* rnd) {
  sequence_header_.color_config.bitdepth = bitdepth;
  sequence_header_.color_config.subsampling_x = param_.subsampling_x;
  sequence_header_.color_config.subsampling_y = param_.subsampling_y;
  sequence_header_.color_config.is_monochrome = false;
  sequence_header_.use_128x128_superblock =
      static_cast<bool>(rnd->Rand16() & 1);

  ASSERT_TRUE(param_.width <= param_.upscaled_width);
  ASSERT_TRUE(param_.upscaled_width * param_.height <= kMaxTestFrameSize)
      << "Please adjust the max frame size.";

  frame_header_.width = param_.width;
  frame_header_.upscaled_width = param_.upscaled_width;
  frame_header_.height = param_.height;
  frame_header_.columns4x4 = DivideBy4(Align(frame_header_.width, 8));
  frame_header_.rows4x4 = DivideBy4(Align(frame_header_.height, 8));
  frame_header_.tile_info.tile_count = 1;
  frame_header_.refresh_frame_flags = 0;
  Cdef* const cdef = &frame_header_.cdef;
  const int coeff_shift = bitdepth - 8;
  do {
    cdef->damping = (rnd->Rand16() & 3) + 3 + coeff_shift;
    cdef->bits = rnd->Rand16() & 3;
  } while (cdef->bits <= 0);
  for (int i = 0; i < (1 << cdef->bits); ++i) {
    cdef->y_primary_strength[i] = (rnd->Rand16() & 15) << coeff_shift;
    cdef->y_secondary_strength[i] = rnd->Rand16() & 3;
    if (cdef->y_secondary_strength[i] == 3) {
      ++cdef->y_secondary_strength[i];
    }
    cdef->y_secondary_strength[i] <<= coeff_shift;
    cdef->uv_primary_strength[i] = (rnd->Rand16() & 15) << coeff_shift;
    cdef->uv_secondary_strength[i] = rnd->Rand16() & 3;
    if (cdef->uv_secondary_strength[i] == 3) {
      ++cdef->uv_secondary_strength[i];
    }
    cdef->uv_secondary_strength[i] <<= coeff_shift;
  }

  const int rows64x64 = DivideBy16(frame_header_.rows4x4 + kMaxBlockHeight4x4);
  const int columns64x64 =
      DivideBy16(frame_header_.columns4x4 + kMaxBlockWidth4x4);
  ASSERT_TRUE(frame_scratch_buffer_.cdef_index.Reset(rows64x64, columns64x64));
  for (int row = 0; row < rows64x64; ++row) {
    for (int column = 0; column < columns64x64; ++column) {
      frame_scratch_buffer_.cdef_index[row][column] =
          rnd->Rand16() & ((1 << cdef->bits) - 1);
    }
  }

  const int skip_rows = DivideBy2(frame_header_.rows4x4 + kMaxBlockHeight4x4);
  const int skip_columns =
      DivideBy16(frame_header_.columns4x4 + kMaxBlockWidth4x4);
  ASSERT_TRUE(frame_scratch_buffer_.cdef_skip.Reset(skip_rows, skip_columns));
  for (int row = 0; row < skip_rows; ++row) {
    memset(frame_scratch_buffer_.cdef_skip[row], 0xFF, skip_columns);
  }

  ASSERT_TRUE(yuv_buffer_.Realloc(
      sequence_header_.color_config.bitdepth,
      sequence_header_.color_config.is_monochrome, frame_header_.upscaled_width,
      frame_header_.height, sequence_header_.color_config.subsampling_x,
      sequence_header_.color_config.subsampling_y, kBorderPixels, kBorderPixels,
      kBorderPixels, kBorderPixels, nullptr, nullptr, nullptr))
      << "Failed to allocate source buffer.";
}

template <int bitdepth, typename Pixel>
void PostFilterApplyCdefTest<bitdepth, Pixel>::SetInputBuffer(
    libvpx_test::ACMRandom* rnd, PostFilter* post_filter) {
  for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
    const int subsampling_x = (plane == 0) ? 0 : param_.subsampling_x;
    const int subsampling_y = (plane == 0) ? 0 : param_.subsampling_y;
    const int plane_width =
        MultiplyBy4(frame_header_.columns4x4) >> subsampling_x;
    const int plane_height =
        MultiplyBy4(frame_header_.rows4x4) >> subsampling_y;
    auto* src =
        reinterpret_cast<Pixel*>(post_filter->GetUnfilteredBuffer(plane));
    const int src_stride = yuv_buffer_.stride(plane) / sizeof(src[0]);
    for (int y = 0; y < plane_height; ++y) {
      for (int x = 0; x < plane_width; ++x) {
        src[x] = rnd->Rand16() & ((1 << bitdepth) - 1);
      }
      src += src_stride;
    }
  }
}

template <int bitdepth, typename Pixel>
void PostFilterApplyCdefTest<bitdepth, Pixel>::CopyFilterOutputToDestBuffer() {
  for (int plane = kPlaneY; plane < kMaxPlanes; ++plane) {
    const int subsampling_x = (plane == 0) ? 0 : param_.subsampling_x;
    const int subsampling_y = (plane == 0) ? 0 : param_.subsampling_y;
    const int plane_width = SubsampledValue(param_.width, subsampling_x);
    const int plane_height = SubsampledValue(param_.height, subsampling_y);
    auto* src = reinterpret_cast<Pixel*>(yuv_buffer_.data(plane));
    const int src_stride = yuv_buffer_.stride(plane) / sizeof(src[0]);
    Pixel* dest_plane =
        dest_ +
        ((plane == 0) ? 0 : ((plane == 1) ? y_size_ : y_size_ + uv_size_));
    for (int y = 0; y < plane_height; ++y) {
      for (int x = 0; x < plane_width; ++x) {
        dest_plane[y * plane_width + x] = src[x];
      }
      src += src_stride;
    }
  }
}

template <int bitdepth, typename Pixel>
void PostFilterApplyCdefTest<bitdepth, Pixel>::TestMultiThread(
    int num_threads) {
  libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
  SetInput(&rnd);

  ASSERT_TRUE(frame_scratch_buffer_.threading_strategy.Reset(frame_header_,
                                                             num_threads));
  if (num_threads > 1) {
    const int num_units =
        MultiplyBy4(RightShiftWithCeiling(frame_header_.rows4x4, 4));
    ASSERT_TRUE(frame_scratch_buffer_.cdef_border.Realloc(
        bitdepth, /*is_monochrome=*/false,
        MultiplyBy4(frame_header_.columns4x4), num_units,
        sequence_header_.color_config.subsampling_x,
        /*subsampling_y=*/0, kBorderPixels, kBorderPixels, kBorderPixels,
        kBorderPixels, nullptr, nullptr, nullptr));
  }

  PostFilter post_filter(frame_header_, sequence_header_,
                         &frame_scratch_buffer_, &yuv_buffer_, dsp_,
                         /*do_post_filter_mask=*/0x02);
  SetInputBuffer(&rnd, &post_filter);

  const int id = GetIdFromInputParam(param_.subsampling_x, param_.subsampling_y,
                                     param_.height);
  absl::Duration elapsed_time;
  const absl::Time start = absl::Now();

  // Only ApplyCdef() and frame copy inside ApplyFilteringThreaded() are
  // triggered, since we set the filter mask to 0x02.
  post_filter.ApplyFilteringThreaded();
  elapsed_time += absl::Now() - start;

  CopyFilterOutputToDestBuffer();
  const char* expected_digest = nullptr;
  switch (bitdepth) {
    case 8:
      expected_digest = GetDigestApplyCdef8bpp(id);
      break;
#if LIBGAV1_MAX_BITDEPTH >= 10
    case 10:
      expected_digest = GetDigestApplyCdef10bpp(id);
      break;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
    case 12:
      expected_digest = GetDigestApplyCdef12bpp(id);
      break;
#endif
  }
  ASSERT_NE(expected_digest, nullptr);
  test_utils::CheckMd5Digest(kCdef, kApplyCdefName, expected_digest, dest_,
                             size_, elapsed_time);
}

const FrameSizeParam kTestParamApplyCdef[] = {
    FrameSizeParam(352, 352, 288, 0, 0),    FrameSizeParam(720, 720, 480, 0, 0),
    FrameSizeParam(1920, 1920, 1080, 0, 0), FrameSizeParam(251, 251, 187, 0, 0),
    FrameSizeParam(352, 352, 288, 0, 1),    FrameSizeParam(720, 720, 480, 0, 1),
    FrameSizeParam(1920, 1920, 1080, 0, 1), FrameSizeParam(251, 251, 187, 0, 1),
    FrameSizeParam(352, 352, 288, 1, 0),    FrameSizeParam(720, 720, 480, 1, 0),
    FrameSizeParam(1920, 1920, 1080, 1, 0), FrameSizeParam(251, 251, 187, 1, 0),
    FrameSizeParam(352, 352, 288, 1, 1),    FrameSizeParam(720, 720, 480, 1, 1),
    FrameSizeParam(1920, 1920, 1080, 1, 1), FrameSizeParam(251, 251, 187, 1, 1),
};

using PostFilterApplyCdefTest8bpp = PostFilterApplyCdefTest<8, uint8_t>;

TEST_P(PostFilterApplyCdefTest8bpp, ApplyCdef) {
  TestMultiThread(2);
  TestMultiThread(4);
  TestMultiThread(8);
}

INSTANTIATE_TEST_SUITE_P(PostFilterApplyCdefTestInstance,
                         PostFilterApplyCdefTest8bpp,
                         testing::ValuesIn(kTestParamApplyCdef));

#if LIBGAV1_MAX_BITDEPTH >= 10
using PostFilterApplyCdefTest10bpp = PostFilterApplyCdefTest<10, uint16_t>;

TEST_P(PostFilterApplyCdefTest10bpp, ApplyCdef) {
  TestMultiThread(2);
  TestMultiThread(4);
  TestMultiThread(8);
}

INSTANTIATE_TEST_SUITE_P(PostFilterApplyCdefTestInstance,
                         PostFilterApplyCdefTest10bpp,
                         testing::ValuesIn(kTestParamApplyCdef));
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
using PostFilterApplyCdefTest12bpp = PostFilterApplyCdefTest<12, uint16_t>;

TEST_P(PostFilterApplyCdefTest12bpp, ApplyCdef) {
  TestMultiThread(2);
  TestMultiThread(4);
  TestMultiThread(8);
}

INSTANTIATE_TEST_SUITE_P(PostFilterApplyCdefTestInstance,
                         PostFilterApplyCdefTest12bpp,
                         testing::ValuesIn(kTestParamApplyCdef));
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace libgav1
