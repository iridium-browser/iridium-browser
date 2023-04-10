// Copyright 2019 The libgav1 Authors
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

#include "src/reconstruction.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include "src/utils/common.h"

namespace libgav1 {
namespace {

// Maps TransformType to dsp::Transform1d for the row transforms.
constexpr dsp::Transform1d kRowTransform[kNumTransformTypes] = {
    dsp::kTransform1dDct,      dsp::kTransform1dAdst,
    dsp::kTransform1dDct,      dsp::kTransform1dAdst,
    dsp::kTransform1dAdst,     dsp::kTransform1dDct,
    dsp::kTransform1dAdst,     dsp::kTransform1dAdst,
    dsp::kTransform1dAdst,     dsp::kTransform1dIdentity,
    dsp::kTransform1dIdentity, dsp::kTransform1dDct,
    dsp::kTransform1dIdentity, dsp::kTransform1dAdst,
    dsp::kTransform1dIdentity, dsp::kTransform1dAdst};

// Maps TransformType to dsp::Transform1d for the column transforms.
constexpr dsp::Transform1d kColumnTransform[kNumTransformTypes] = {
    dsp::kTransform1dDct,  dsp::kTransform1dDct,
    dsp::kTransform1dAdst, dsp::kTransform1dAdst,
    dsp::kTransform1dDct,  dsp::kTransform1dAdst,
    dsp::kTransform1dAdst, dsp::kTransform1dAdst,
    dsp::kTransform1dAdst, dsp::kTransform1dIdentity,
    dsp::kTransform1dDct,  dsp::kTransform1dIdentity,
    dsp::kTransform1dAdst, dsp::kTransform1dIdentity,
    dsp::kTransform1dAdst, dsp::kTransform1dIdentity};

dsp::Transform1dSize GetTransform1dSize(int size_log2) {
  return static_cast<dsp::Transform1dSize>(size_log2 - 2);
}

// Returns the number of rows to process based on |non_zero_coeff_count|. The
// transform loops process either 4 or a multiple of 8 rows. Use the
// TransformClass derived from |tx_type| to determine the scan order.
template <int tx_width>
int GetNumRows(TransformType tx_type, int tx_height, int non_zero_coeff_count) {
  const TransformClass tx_class = GetTransformClass(tx_type);

  switch (tx_class) {
    case kTransformClass2D:
      if (tx_width == 4) {
        if (non_zero_coeff_count <= 13) return 4;
        if (non_zero_coeff_count <= 29) return 8;
      }
      if (tx_width == 8) {
        if (non_zero_coeff_count <= 10) return 4;
        if ((non_zero_coeff_count <= 14) & (tx_height > 8)) return 4;
        if (non_zero_coeff_count <= 43) return 8;
        if ((non_zero_coeff_count <= 107) & (tx_height > 16)) return 16;
        if ((non_zero_coeff_count <= 171) & (tx_height > 16)) return 24;
      }
      if (tx_width == 16) {
        if (non_zero_coeff_count <= 10) return 4;
        if ((non_zero_coeff_count <= 14) & (tx_height > 16)) return 4;
        if (non_zero_coeff_count <= 36) return 8;
        if ((non_zero_coeff_count <= 44) & (tx_height > 16)) return 8;
        if ((non_zero_coeff_count <= 151) & (tx_height > 16)) return 16;
        if ((non_zero_coeff_count <= 279) & (tx_height > 16)) return 24;
      }
      if (tx_width == 32) {
        if (non_zero_coeff_count <= 10) return 4;
        if (non_zero_coeff_count <= 36) return 8;
        if ((non_zero_coeff_count <= 136) & (tx_height > 16)) return 16;
        if ((non_zero_coeff_count <= 300) & (tx_height > 16)) return 24;
      }
      break;

    case kTransformClassHorizontal:
      if (non_zero_coeff_count <= 4) return 4;
      if (non_zero_coeff_count <= 8) return 8;
      if ((non_zero_coeff_count <= 16) & (tx_height > 16)) return 16;
      if ((non_zero_coeff_count <= 24) & (tx_height > 16)) return 24;
      break;

    default:
      assert(tx_class == kTransformClassVertical);
      if (tx_width == 4) {
        if (non_zero_coeff_count <= 16) return 4;
        if (non_zero_coeff_count <= 32) return 8;
      }
      if (tx_width == 8) {
        if (non_zero_coeff_count <= 32) return 4;
        if (non_zero_coeff_count <= 64) return 8;
        // There's no need to check tx_height since the maximum values for
        // smaller sizes are: 8x8: 63, 8x16: 127.
        if (non_zero_coeff_count <= 128) return 16;
        if (non_zero_coeff_count <= 192) return 24;
      }
      if (tx_width == 16) {
        if (non_zero_coeff_count <= 64) return 4;
        if (non_zero_coeff_count <= 128) return 8;
        // There's no need to check tx_height since the maximum values for
        // smaller sizes are: 16x8: 127, 16x16: 255.
        if (non_zero_coeff_count <= 256) return 16;
        if (non_zero_coeff_count <= 384) return 24;
      }
      if (tx_width == 32) {
        if (non_zero_coeff_count <= 128) return 4;
        if (non_zero_coeff_count <= 256) return 8;
        // There's no need to check tx_height since the maximum values for
        // smaller sizes are: 32x8 is 255, 32x16 is 511.
        if ((non_zero_coeff_count <= 512)) return 16;
        if ((non_zero_coeff_count <= 768)) return 24;
      }
      break;
  }
  return (tx_width >= 16) ? std::min(tx_height, 32) : tx_height;
}

}  // namespace

template <typename Residual, typename Pixel>
void Reconstruct(const dsp::Dsp& dsp, TransformType tx_type,
                 TransformSize tx_size, bool lossless, Residual* const buffer,
                 int start_x, int start_y, Array2DView<Pixel>* frame,
                 int non_zero_coeff_count) {
  static_assert(sizeof(Residual) == 2 || sizeof(Residual) == 4, "");
  const int tx_width_log2 = kTransformWidthLog2[tx_size];
  const int tx_height_log2 = kTransformHeightLog2[tx_size];

  int tx_height = (non_zero_coeff_count == 1) ? 1 : kTransformHeight[tx_size];
  if (tx_height > 4) {
    static constexpr int (*kGetNumRows[])(TransformType tx_type, int tx_height,
                                          int non_zero_coeff_count) = {
        &GetNumRows<4>, &GetNumRows<8>, &GetNumRows<16>, &GetNumRows<32>,
        &GetNumRows<32>};
    tx_height = kGetNumRows[tx_width_log2 - 2](tx_type, tx_height,
                                               non_zero_coeff_count);
  }
  assert(tx_height <= 32);

  // Row transform.
  const dsp::Transform1dSize row_transform_size =
      GetTransform1dSize(tx_width_log2);
  const dsp::Transform1d row_transform =
      lossless ? dsp::kTransform1dWht : kRowTransform[tx_type];
  const dsp::InverseTransformAddFunc row_transform_func =
      dsp.inverse_transforms[row_transform][row_transform_size][dsp::kRow];
  assert(row_transform_func != nullptr);

  row_transform_func(tx_type, tx_size, tx_height, buffer, start_x, start_y,
                     frame);

  // Column transform.
  const dsp::Transform1dSize column_transform_size =
      GetTransform1dSize(tx_height_log2);
  const dsp::Transform1d column_transform =
      lossless ? dsp::kTransform1dWht : kColumnTransform[tx_type];
  const dsp::InverseTransformAddFunc column_transform_func =
      dsp.inverse_transforms[column_transform][column_transform_size]
                            [dsp::kColumn];
  assert(column_transform_func != nullptr);

  column_transform_func(tx_type, tx_size, tx_height, buffer, start_x, start_y,
                        frame);
}

template void Reconstruct(const dsp::Dsp& dsp, TransformType tx_type,
                          TransformSize tx_size, bool lossless, int16_t* buffer,
                          int start_x, int start_y, Array2DView<uint8_t>* frame,
                          int non_zero_coeff_count);
#if LIBGAV1_MAX_BITDEPTH >= 10
template void Reconstruct(const dsp::Dsp& dsp, TransformType tx_type,
                          TransformSize tx_size, bool lossless, int32_t* buffer,
                          int start_x, int start_y,
                          Array2DView<uint16_t>* frame,
                          int non_zero_coeff_count);
#endif

}  // namespace libgav1
