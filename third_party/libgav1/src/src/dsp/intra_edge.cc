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

#include "src/dsp/intra_edge.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kKernelTaps = 5;
constexpr int kKernels[3][kKernelTaps] = {
    {0, 4, 8, 4, 0}, {0, 5, 6, 5, 0}, {2, 4, 4, 4, 2}};
constexpr int kMaxUpsampleSize = 16;

template <typename Pixel>
void IntraEdgeFilter_C(void* buffer, int size, int strength) {
  assert(strength > 0);
  Pixel edge[129];
  memcpy(edge, buffer, sizeof(edge[0]) * size);
  auto* const dst_buffer = static_cast<Pixel*>(buffer);
  const int kernel_index = strength - 1;
  for (int i = 1; i < size; ++i) {
    int sum = 0;
    for (int j = 0; j < kKernelTaps; ++j) {
      const int k = Clip3(i + j - 2, 0, size - 1);
      sum += kKernels[kernel_index][j] * edge[k];
    }
    dst_buffer[i] = RightShiftWithRounding(sum, 4);
  }
}

template <int bitdepth, typename Pixel>
void IntraEdgeUpsampler_C(void* buffer, int size) {
  assert(size % 4 == 0 && size <= kMaxUpsampleSize);
  auto* const pixel_buffer = static_cast<Pixel*>(buffer);
  Pixel temp[kMaxUpsampleSize + 3];
  temp[0] = temp[1] = pixel_buffer[-1];
  memcpy(temp + 2, pixel_buffer, sizeof(temp[0]) * size);
  temp[size + 2] = pixel_buffer[size - 1];

  pixel_buffer[-2] = temp[0];
  for (int i = 0; i < size; ++i) {
    const int sum =
        -temp[i] + (9 * temp[i + 1]) + (9 * temp[i + 2]) - temp[i + 3];
    pixel_buffer[2 * i - 1] =
        Clip3(RightShiftWithRounding(sum, 4), 0, (1 << bitdepth) - 1);
    pixel_buffer[2 * i] = temp[i + 2];
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->intra_edge_filter = IntraEdgeFilter_C<uint8_t>;
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_C<8, uint8_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp8bpp_IntraEdgeFilter
  dsp->intra_edge_filter = IntraEdgeFilter_C<uint8_t>;
#endif
#ifndef LIBGAV1_Dsp8bpp_IntraEdgeUpsampler
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_C<8, uint8_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  dsp->intra_edge_filter = IntraEdgeFilter_C<uint16_t>;
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_C<10, uint16_t>;
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  static_cast<void>(dsp);
#ifndef LIBGAV1_Dsp10bpp_IntraEdgeFilter
  dsp->intra_edge_filter = IntraEdgeFilter_C<uint16_t>;
#endif
#ifndef LIBGAV1_Dsp10bpp_IntraEdgeUpsampler
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_C<10, uint16_t>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif

}  // namespace

void IntraEdgeInit_C() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  Init10bpp();
#endif
}

}  // namespace dsp
}  // namespace libgav1
