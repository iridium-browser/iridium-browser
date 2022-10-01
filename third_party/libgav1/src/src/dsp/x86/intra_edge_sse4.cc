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
#include "src/utils/cpu.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <xmmintrin.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/x86/common_sse4.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

constexpr int kKernelTaps = 5;
constexpr int kKernels[3][kKernelTaps] = {
    {0, 4, 8, 4, 0}, {0, 5, 6, 5, 0}, {2, 4, 4, 4, 2}};
constexpr int kMaxEdgeBufferSize = 129;

// This function applies the kernel [0, 4, 8, 4, 0] to 12 values.
// Assumes |edge| has 16 packed byte values. Produces 12 filter outputs to
// write as overlapping sets of 8-bytes.
inline void ComputeKernel1Store12(uint8_t* LIBGAV1_RESTRICT dest,
                                  const uint8_t* LIBGAV1_RESTRICT source) {
  const __m128i edge_lo = LoadUnaligned16(source);
  const __m128i edge_hi = _mm_srli_si128(edge_lo, 6);
  // Samples matched with the '4' tap, expanded to 16-bit.
  const __m128i outers_lo = _mm_cvtepu8_epi16(edge_lo);
  const __m128i outers_hi = _mm_cvtepu8_epi16(edge_hi);
  // Samples matched with the '8' tap, expanded to 16-bit.
  const __m128i centers_lo = _mm_srli_si128(outers_lo, 2);
  const __m128i centers_hi = _mm_srli_si128(outers_hi, 2);

  // Apply the taps by shifting.
  const __m128i outers4_lo = _mm_slli_epi16(outers_lo, 2);
  const __m128i outers4_hi = _mm_slli_epi16(outers_hi, 2);
  const __m128i centers8_lo = _mm_slli_epi16(centers_lo, 3);
  const __m128i centers8_hi = _mm_slli_epi16(centers_hi, 3);
  // Move latter 4x values down to add with first 4x values for each output.
  const __m128i partial_sums_lo =
      _mm_add_epi16(outers4_lo, _mm_srli_si128(outers4_lo, 4));
  const __m128i partial_sums_hi =
      _mm_add_epi16(outers4_hi, _mm_srli_si128(outers4_hi, 4));
  // Move 6x values down to add for the final kernel sum for each output.
  const __m128i sums_lo = RightShiftWithRounding_U16(
      _mm_add_epi16(partial_sums_lo, centers8_lo), 4);
  const __m128i sums_hi = RightShiftWithRounding_U16(
      _mm_add_epi16(partial_sums_hi, centers8_hi), 4);

  const __m128i result_lo = _mm_packus_epi16(sums_lo, sums_lo);
  const __m128i result_hi = _mm_packus_epi16(sums_hi, sums_hi);
  const __m128i result =
      _mm_alignr_epi8(result_hi, _mm_slli_si128(result_lo, 10), 10);
  StoreUnaligned16(dest, result);
}

// This function applies the kernel [0, 5, 6, 5, 0] to 12 values.
// Assumes |edge| has 8 packed byte values, and that the 2 invalid values will
// be overwritten or safely discarded.
inline void ComputeKernel2Store12(uint8_t* LIBGAV1_RESTRICT dest,
                                  const uint8_t* LIBGAV1_RESTRICT source) {
  const __m128i edge_lo = LoadUnaligned16(source);
  const __m128i edge_hi = _mm_srli_si128(edge_lo, 6);
  const __m128i outers_lo = _mm_cvtepu8_epi16(edge_lo);
  const __m128i centers_lo = _mm_srli_si128(outers_lo, 2);
  const __m128i outers_hi = _mm_cvtepu8_epi16(edge_hi);
  const __m128i centers_hi = _mm_srli_si128(outers_hi, 2);
  // Samples matched with the '5' tap, expanded to 16-bit. Add x + 4x.
  const __m128i outers5_lo =
      _mm_add_epi16(outers_lo, _mm_slli_epi16(outers_lo, 2));
  const __m128i outers5_hi =
      _mm_add_epi16(outers_hi, _mm_slli_epi16(outers_hi, 2));
  // Samples matched with the '6' tap, expanded to 16-bit. Add 2x + 4x.
  const __m128i centers6_lo = _mm_add_epi16(_mm_slli_epi16(centers_lo, 1),
                                            _mm_slli_epi16(centers_lo, 2));
  const __m128i centers6_hi = _mm_add_epi16(_mm_slli_epi16(centers_hi, 1),
                                            _mm_slli_epi16(centers_hi, 2));
  // Move latter 5x values down to add with first 5x values for each output.
  const __m128i partial_sums_lo =
      _mm_add_epi16(outers5_lo, _mm_srli_si128(outers5_lo, 4));
  // Move 6x values down to add for the final kernel sum for each output.
  const __m128i sums_lo = RightShiftWithRounding_U16(
      _mm_add_epi16(centers6_lo, partial_sums_lo), 4);
  // Shift latter 5x values to add with first 5x values for each output.
  const __m128i partial_sums_hi =
      _mm_add_epi16(outers5_hi, _mm_srli_si128(outers5_hi, 4));
  // Move 6x values down to add for the final kernel sum for each output.
  const __m128i sums_hi = RightShiftWithRounding_U16(
      _mm_add_epi16(centers6_hi, partial_sums_hi), 4);
  // First 6 values are valid outputs.
  const __m128i result_lo = _mm_packus_epi16(sums_lo, sums_lo);
  const __m128i result_hi = _mm_packus_epi16(sums_hi, sums_hi);
  const __m128i result =
      _mm_alignr_epi8(result_hi, _mm_slli_si128(result_lo, 10), 10);
  StoreUnaligned16(dest, result);
}

// This function applies the kernel [2, 4, 4, 4, 2] to 8 values.
inline void ComputeKernel3Store8(uint8_t* LIBGAV1_RESTRICT dest,
                                 const uint8_t* LIBGAV1_RESTRICT source) {
  const __m128i edge_lo = LoadUnaligned16(source);
  const __m128i edge_hi = _mm_srli_si128(edge_lo, 4);
  // Finish |edge_lo| life cycle quickly.
  // Multiply for 2x.
  const __m128i source2_lo = _mm_slli_epi16(_mm_cvtepu8_epi16(edge_lo), 1);
  // Multiply 2x by 2 and align.
  const __m128i source4_lo = _mm_srli_si128(_mm_slli_epi16(source2_lo, 1), 2);
  // Finish |source2| life cycle quickly.
  // Move latter 2x values down to add with first 2x values for each output.
  __m128i sum = _mm_add_epi16(source2_lo, _mm_srli_si128(source2_lo, 8));
  // First 4x values already aligned to add with running total.
  sum = _mm_add_epi16(sum, source4_lo);
  // Move second 4x values down to add with running total.
  sum = _mm_add_epi16(sum, _mm_srli_si128(source4_lo, 2));
  // Move third 4x values down to add with running total.
  sum = _mm_add_epi16(sum, _mm_srli_si128(source4_lo, 4));
  // Multiply for 2x.
  const __m128i source2_hi = _mm_slli_epi16(_mm_cvtepu8_epi16(edge_hi), 1);
  // Multiply 2x by 2 and align.
  const __m128i source4_hi = _mm_srli_si128(_mm_slli_epi16(source2_hi, 1), 2);
  // Move latter 2x values down to add with first 2x values for each output.
  __m128i sum_hi = _mm_add_epi16(source2_hi, _mm_srli_si128(source2_hi, 8));
  // First 4x values already aligned to add with running total.
  sum_hi = _mm_add_epi16(sum_hi, source4_hi);
  // Move second 4x values down to add with running total.
  sum_hi = _mm_add_epi16(sum_hi, _mm_srli_si128(source4_hi, 2));
  // Move third 4x values down to add with running total.
  sum_hi = _mm_add_epi16(sum_hi, _mm_srli_si128(source4_hi, 4));

  // Because we have only 8 values here, it is safe to align before packing down
  // to 8-bit without losing data.
  sum = _mm_alignr_epi8(sum_hi, _mm_slli_si128(sum, 8), 8);
  sum = RightShiftWithRounding_U16(sum, 4);
  StoreLo8(dest, _mm_packus_epi16(sum, sum));
}

void IntraEdgeFilter_SSE4_1(void* buffer, int size, int strength) {
  uint8_t edge[kMaxEdgeBufferSize + 4];
  memcpy(edge, buffer, size);
  auto* dst_buffer = static_cast<uint8_t*>(buffer);

  // Only process |size| - 1 elements. Nothing to do in this case.
  if (size == 1) return;

  int i = 0;
  switch (strength) {
    case 1:
      // To avoid overwriting, we stop short from the total write size plus the
      // initial offset. In this case 12 valid values are written in two blocks
      // of 8 bytes each.
      for (; i < size - 17; i += 12) {
        ComputeKernel1Store12(dst_buffer + i + 1, edge + i);
      }
      break;
    case 2:
      // See the comment for case 1.
      for (; i < size - 17; i += 12) {
        ComputeKernel2Store12(dst_buffer + i + 1, edge + i);
      }
      break;
    default:
      assert(strength == 3);
      // The first filter input is repeated for taps of value 2 and 4.
      dst_buffer[1] = RightShiftWithRounding(
          (6 * edge[0] + 4 * edge[1] + 4 * edge[2] + 2 * edge[3]), 4);
      // In this case, one block of 8 bytes is written in each iteration, with
      // an offset of 2.
      for (; i < size - 10; i += 8) {
        ComputeKernel3Store8(dst_buffer + i + 2, edge + i);
      }
  }
  const int kernel_index = strength - 1;
  for (int final_index = Clip3(i, 1, size - 2); final_index < size;
       ++final_index) {
    int sum = 0;
    for (int j = 0; j < kKernelTaps; ++j) {
      const int k = Clip3(final_index + j - 2, 0, size - 1);
      sum += kKernels[kernel_index][j] * edge[k];
    }
    dst_buffer[final_index] = RightShiftWithRounding(sum, 4);
  }
}

constexpr int kMaxUpsampleSize = 16;

// Applies the upsampling kernel [-1, 9, 9, -1] to alternating pixels, and
// interleaves the results with the original values. This implementation assumes
// that it is safe to write the maximum number of upsampled pixels (32) to the
// edge buffer, even when |size| is small.
void IntraEdgeUpsampler_SSE4_1(void* buffer, int size) {
  assert(size % 4 == 0 && size <= kMaxUpsampleSize);
  auto* const pixel_buffer = static_cast<uint8_t*>(buffer);
  uint8_t temp[kMaxUpsampleSize + 8];
  temp[0] = temp[1] = pixel_buffer[-1];
  memcpy(temp + 2, pixel_buffer, sizeof(temp[0]) * size);
  temp[size + 2] = pixel_buffer[size - 1];

  pixel_buffer[-2] = temp[0];
  const __m128i data = LoadUnaligned16(temp);
  const __m128i src_lo = _mm_cvtepu8_epi16(data);
  const __m128i src_hi = _mm_unpackhi_epi8(data, _mm_setzero_si128());
  const __m128i src9_hi = _mm_add_epi16(src_hi, _mm_slli_epi16(src_hi, 3));
  const __m128i src9_lo = _mm_add_epi16(src_lo, _mm_slli_epi16(src_lo, 3));
  __m128i sum_lo = _mm_sub_epi16(_mm_alignr_epi8(src9_hi, src9_lo, 2), src_lo);
  sum_lo = _mm_add_epi16(sum_lo, _mm_alignr_epi8(src9_hi, src9_lo, 4));
  sum_lo = _mm_sub_epi16(sum_lo, _mm_alignr_epi8(src_hi, src_lo, 6));
  sum_lo = RightShiftWithRounding_S16(sum_lo, 4);
  const __m128i result_lo = _mm_unpacklo_epi8(_mm_packus_epi16(sum_lo, sum_lo),
                                              _mm_srli_si128(data, 2));
  StoreUnaligned16(pixel_buffer - 1, result_lo);
  if (size > 8) {
    const __m128i src_hi_extra = _mm_cvtepu8_epi16(LoadLo8(temp + 16));
    const __m128i src9_hi_extra =
        _mm_add_epi16(src_hi_extra, _mm_slli_epi16(src_hi_extra, 3));
    __m128i sum_hi =
        _mm_sub_epi16(_mm_alignr_epi8(src9_hi_extra, src9_hi, 2), src_hi);
    sum_hi = _mm_add_epi16(sum_hi, _mm_alignr_epi8(src9_hi_extra, src9_hi, 4));
    sum_hi = _mm_sub_epi16(sum_hi, _mm_alignr_epi8(src_hi_extra, src_hi, 6));
    sum_hi = RightShiftWithRounding_S16(sum_hi, 4);
    const __m128i result_hi =
        _mm_unpacklo_epi8(_mm_packus_epi16(sum_hi, sum_hi), LoadLo8(temp + 10));
    StoreUnaligned16(pixel_buffer + 15, result_hi);
  }
}

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
#if DSP_ENABLED_8BPP_SSE4_1(IntraEdgeFilter)
  dsp->intra_edge_filter = IntraEdgeFilter_SSE4_1;
#endif
#if DSP_ENABLED_8BPP_SSE4_1(IntraEdgeUpsampler)
  dsp->intra_edge_upsampler = IntraEdgeUpsampler_SSE4_1;
#endif
}

}  // namespace

void IntraEdgeInit_SSE4_1() { Init8bpp(); }

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_TARGETING_SSE4_1
namespace libgav1 {
namespace dsp {

void IntraEdgeInit_SSE4_1() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_TARGETING_SSE4_1
