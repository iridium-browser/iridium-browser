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

// Common 128 bit functions used for sse4/avx2 convolve implementations.
// This will be included inside an anonymous namespace on files where these are
// necessary.

#include "src/dsp/convolve.inc"

// This version checks for the special cases when filter_index == 1.
int GetNumTapsInFilter(const int filter_index, const int filter_id) {
  if (filter_index == 0) {
    // Despite the names these only use 6 taps.
    // kInterpolationFilterEightTap
    // kInterpolationFilterEightTapSmooth
    return 6;
  }

  if (filter_index == 1) {
    // Despite the names these only use 6 taps.
    // kInterpolationFilterEightTap
    // kInterpolationFilterEightTapSmooth
    if (((filter_id == 1) | (filter_id == 15) | (filter_id == 7) |
         (filter_id == 8) | (filter_id == 9)) != 0) {
      return 6;
    }
    // When |filter_index| == 1, the |filter_id| values not listed above map to
    // 4 tap filters.
    return 4;
  }

  if (filter_index == 2) {
    // kInterpolationFilterEightTapSharp
    return 8;
  }

  if (filter_index == 3) {
    // kInterpolationFilterBilinear
    return 2;
  }

  assert(filter_index > 3);
  // For small sizes (width/height <= 4) the large filters are replaced with 4
  // tap options.
  // If the original filters were |kInterpolationFilterEightTap| or
  // |kInterpolationFilterEightTapSharp| then it becomes
  // |kInterpolationFilterSwitchable|.
  // If it was |kInterpolationFilterEightTapSmooth| then it becomes an unnamed 4
  // tap filter.
  return 4;
}

// Multiply every entry in |src[]| by the corresponding entry in |taps[]| and
// sum. The filters in |taps[]| are pre-shifted by 1. This prevents the final
// sum from outranging int16_t.
template <int num_taps>
__m128i SumOnePassTaps(const __m128i* const src, const __m128i* const taps) {
  __m128i sum;
  if (num_taps == 6) {
    // 6 taps.
    const __m128i v_madd_21 = _mm_maddubs_epi16(src[0], taps[0]);  // k2k1
    const __m128i v_madd_43 = _mm_maddubs_epi16(src[1], taps[1]);  // k4k3
    const __m128i v_madd_65 = _mm_maddubs_epi16(src[2], taps[2]);  // k6k5
    sum = _mm_add_epi16(v_madd_21, v_madd_43);
    sum = _mm_add_epi16(sum, v_madd_65);
  } else if (num_taps == 8) {
    // 8 taps.
    const __m128i v_madd_10 = _mm_maddubs_epi16(src[0], taps[0]);  // k1k0
    const __m128i v_madd_32 = _mm_maddubs_epi16(src[1], taps[1]);  // k3k2
    const __m128i v_madd_54 = _mm_maddubs_epi16(src[2], taps[2]);  // k5k4
    const __m128i v_madd_76 = _mm_maddubs_epi16(src[3], taps[3]);  // k7k6
    const __m128i v_sum_3210 = _mm_add_epi16(v_madd_10, v_madd_32);
    const __m128i v_sum_7654 = _mm_add_epi16(v_madd_54, v_madd_76);
    sum = _mm_add_epi16(v_sum_7654, v_sum_3210);
  } else if (num_taps == 2) {
    // 2 taps.
    sum = _mm_maddubs_epi16(src[0], taps[0]);  // k4k3
  } else {
    // 4 taps.
    const __m128i v_madd_32 = _mm_maddubs_epi16(src[0], taps[0]);  // k3k2
    const __m128i v_madd_54 = _mm_maddubs_epi16(src[1], taps[1]);  // k5k4
    sum = _mm_add_epi16(v_madd_32, v_madd_54);
  }
  return sum;
}

template <int num_taps>
__m128i SumHorizontalTaps2x2(const uint8_t* src, const ptrdiff_t src_stride,
                             const __m128i* const v_tap) {
  // 00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
  const __m128i v_src = LoadHi8(LoadLo8(&src[0]), &src[src_stride]);

  if (num_taps == 2) {
    // 03 04 04 05 05 06 06 07 13 14 14 15 15 16 16 17
    const __m128i v_src_43 = _mm_shuffle_epi8(
        v_src, _mm_set_epi32(0x0f0e0e0d, 0x0d0c0c0b, 0x07060605, 0x05040403));
    const __m128i v_sum_43 = _mm_maddubs_epi16(v_src_43, v_tap[0]);  // k4k3
    return v_sum_43;
  }

  // 02 03 03 04 04 05 05 06 12 13 13 14 14 15 15 16
  const __m128i v_src_32 = _mm_shuffle_epi8(
      v_src, _mm_set_epi32(0x0e0d0d0c, 0x0c0b0b0a, 0x06050504, 0x04030302));
  // 04 05 05 06 06 07 07 xx 14 15 15 16 16 17 17 xx
  const __m128i v_src_54 = _mm_shuffle_epi8(
      v_src, _mm_set_epi32(static_cast<int>(0x800f0f0e), 0x0e0d0d0c,
                           static_cast<int>(0x80070706), 0x06050504));
  const __m128i v_madd_32 = _mm_maddubs_epi16(v_src_32, v_tap[0]);  // k3k2
  const __m128i v_madd_54 = _mm_maddubs_epi16(v_src_54, v_tap[1]);  // k5k4
  const __m128i v_sum_5432 = _mm_add_epi16(v_madd_54, v_madd_32);
  return v_sum_5432;
}

template <int num_taps>
__m128i SimpleHorizontalTaps2x2(const uint8_t* src, const ptrdiff_t src_stride,
                                const __m128i* const v_tap) {
  __m128i sum = SumHorizontalTaps2x2<num_taps>(src, src_stride, v_tap);

  // Normally the Horizontal pass does the downshift in two passes:
  // kInterRoundBitsHorizontal - 1 and then (kFilterBits -
  // kInterRoundBitsHorizontal). Each one uses a rounding shift. Combining them
  // requires adding the rounding offset from the skipped shift.
  constexpr int first_shift_rounding_bit = 1 << (kInterRoundBitsHorizontal - 2);

  sum = _mm_add_epi16(sum, _mm_set1_epi16(first_shift_rounding_bit));
  sum = RightShiftWithRounding_S16(sum, kFilterBits - 1);
  return _mm_packus_epi16(sum, sum);
}

template <int num_taps>
__m128i HorizontalTaps8To16_2x2(const uint8_t* src, const ptrdiff_t src_stride,
                                const __m128i* const v_tap) {
  const __m128i sum = SumHorizontalTaps2x2<num_taps>(src, src_stride, v_tap);

  return RightShiftWithRounding_S16(sum, kInterRoundBitsHorizontal - 1);
}

template <int num_taps, bool is_2d_vertical = false>
LIBGAV1_ALWAYS_INLINE void SetupTaps(const __m128i* const filter,
                                     __m128i* v_tap) {
  if (num_taps == 8) {
    v_tap[0] = _mm_shufflelo_epi16(*filter, 0x0);   // k1k0
    v_tap[1] = _mm_shufflelo_epi16(*filter, 0x55);  // k3k2
    v_tap[2] = _mm_shufflelo_epi16(*filter, 0xaa);  // k5k4
    v_tap[3] = _mm_shufflelo_epi16(*filter, 0xff);  // k7k6
    if (is_2d_vertical) {
      v_tap[0] = _mm_cvtepi8_epi16(v_tap[0]);
      v_tap[1] = _mm_cvtepi8_epi16(v_tap[1]);
      v_tap[2] = _mm_cvtepi8_epi16(v_tap[2]);
      v_tap[3] = _mm_cvtepi8_epi16(v_tap[3]);
    } else {
      v_tap[0] = _mm_unpacklo_epi64(v_tap[0], v_tap[0]);
      v_tap[1] = _mm_unpacklo_epi64(v_tap[1], v_tap[1]);
      v_tap[2] = _mm_unpacklo_epi64(v_tap[2], v_tap[2]);
      v_tap[3] = _mm_unpacklo_epi64(v_tap[3], v_tap[3]);
    }
  } else if (num_taps == 6) {
    const __m128i adjusted_filter = _mm_srli_si128(*filter, 1);
    v_tap[0] = _mm_shufflelo_epi16(adjusted_filter, 0x0);   // k2k1
    v_tap[1] = _mm_shufflelo_epi16(adjusted_filter, 0x55);  // k4k3
    v_tap[2] = _mm_shufflelo_epi16(adjusted_filter, 0xaa);  // k6k5
    if (is_2d_vertical) {
      v_tap[0] = _mm_cvtepi8_epi16(v_tap[0]);
      v_tap[1] = _mm_cvtepi8_epi16(v_tap[1]);
      v_tap[2] = _mm_cvtepi8_epi16(v_tap[2]);
    } else {
      v_tap[0] = _mm_unpacklo_epi64(v_tap[0], v_tap[0]);
      v_tap[1] = _mm_unpacklo_epi64(v_tap[1], v_tap[1]);
      v_tap[2] = _mm_unpacklo_epi64(v_tap[2], v_tap[2]);
    }
  } else if (num_taps == 4) {
    v_tap[0] = _mm_shufflelo_epi16(*filter, 0x55);  // k3k2
    v_tap[1] = _mm_shufflelo_epi16(*filter, 0xaa);  // k5k4
    if (is_2d_vertical) {
      v_tap[0] = _mm_cvtepi8_epi16(v_tap[0]);
      v_tap[1] = _mm_cvtepi8_epi16(v_tap[1]);
    } else {
      v_tap[0] = _mm_unpacklo_epi64(v_tap[0], v_tap[0]);
      v_tap[1] = _mm_unpacklo_epi64(v_tap[1], v_tap[1]);
    }
  } else {  // num_taps == 2
    const __m128i adjusted_filter = _mm_srli_si128(*filter, 1);
    v_tap[0] = _mm_shufflelo_epi16(adjusted_filter, 0x55);  // k4k3
    if (is_2d_vertical) {
      v_tap[0] = _mm_cvtepi8_epi16(v_tap[0]);
    } else {
      v_tap[0] = _mm_unpacklo_epi64(v_tap[0], v_tap[0]);
    }
  }
}

template <int num_taps, bool is_compound>
__m128i SimpleSum2DVerticalTaps(const __m128i* const src,
                                const __m128i* const taps) {
  __m128i sum_lo = _mm_madd_epi16(_mm_unpacklo_epi16(src[0], src[1]), taps[0]);
  __m128i sum_hi = _mm_madd_epi16(_mm_unpackhi_epi16(src[0], src[1]), taps[0]);
  if (num_taps >= 4) {
    __m128i madd_lo =
        _mm_madd_epi16(_mm_unpacklo_epi16(src[2], src[3]), taps[1]);
    __m128i madd_hi =
        _mm_madd_epi16(_mm_unpackhi_epi16(src[2], src[3]), taps[1]);
    sum_lo = _mm_add_epi32(sum_lo, madd_lo);
    sum_hi = _mm_add_epi32(sum_hi, madd_hi);
    if (num_taps >= 6) {
      madd_lo = _mm_madd_epi16(_mm_unpacklo_epi16(src[4], src[5]), taps[2]);
      madd_hi = _mm_madd_epi16(_mm_unpackhi_epi16(src[4], src[5]), taps[2]);
      sum_lo = _mm_add_epi32(sum_lo, madd_lo);
      sum_hi = _mm_add_epi32(sum_hi, madd_hi);
      if (num_taps == 8) {
        madd_lo = _mm_madd_epi16(_mm_unpacklo_epi16(src[6], src[7]), taps[3]);
        madd_hi = _mm_madd_epi16(_mm_unpackhi_epi16(src[6], src[7]), taps[3]);
        sum_lo = _mm_add_epi32(sum_lo, madd_lo);
        sum_hi = _mm_add_epi32(sum_hi, madd_hi);
      }
    }
  }

  if (is_compound) {
    return _mm_packs_epi32(
        RightShiftWithRounding_S32(sum_lo, kInterRoundBitsCompoundVertical - 1),
        RightShiftWithRounding_S32(sum_hi,
                                   kInterRoundBitsCompoundVertical - 1));
  }

  return _mm_packs_epi32(
      RightShiftWithRounding_S32(sum_lo, kInterRoundBitsVertical - 1),
      RightShiftWithRounding_S32(sum_hi, kInterRoundBitsVertical - 1));
}

template <int num_taps, bool is_compound = false>
void Filter2DVertical(const uint16_t* src, void* const dst,
                      const ptrdiff_t dst_stride, const int width,
                      const int height, const __m128i* const taps) {
  assert(width >= 8);
  constexpr int next_row = num_taps - 1;
  // The Horizontal pass uses |width| as |stride| for the intermediate buffer.
  const ptrdiff_t src_stride = width;

  auto* dst8 = static_cast<uint8_t*>(dst);
  auto* dst16 = static_cast<uint16_t*>(dst);

  int x = 0;
  do {
    __m128i srcs[8];
    const uint16_t* src_x = src + x;
    srcs[0] = LoadAligned16(src_x);
    src_x += src_stride;
    if (num_taps >= 4) {
      srcs[1] = LoadAligned16(src_x);
      src_x += src_stride;
      srcs[2] = LoadAligned16(src_x);
      src_x += src_stride;
      if (num_taps >= 6) {
        srcs[3] = LoadAligned16(src_x);
        src_x += src_stride;
        srcs[4] = LoadAligned16(src_x);
        src_x += src_stride;
        if (num_taps == 8) {
          srcs[5] = LoadAligned16(src_x);
          src_x += src_stride;
          srcs[6] = LoadAligned16(src_x);
          src_x += src_stride;
        }
      }
    }

    auto* dst8_x = dst8 + x;
    auto* dst16_x = dst16 + x;
    int y = height;
    do {
      srcs[next_row] = LoadAligned16(src_x);
      src_x += src_stride;

      const __m128i sum =
          SimpleSum2DVerticalTaps<num_taps, is_compound>(srcs, taps);
      if (is_compound) {
        StoreUnaligned16(dst16_x, sum);
        dst16_x += dst_stride;
      } else {
        StoreLo8(dst8_x, _mm_packus_epi16(sum, sum));
        dst8_x += dst_stride;
      }

      srcs[0] = srcs[1];
      if (num_taps >= 4) {
        srcs[1] = srcs[2];
        srcs[2] = srcs[3];
        if (num_taps >= 6) {
          srcs[3] = srcs[4];
          srcs[4] = srcs[5];
          if (num_taps == 8) {
            srcs[5] = srcs[6];
            srcs[6] = srcs[7];
          }
        }
      }
    } while (--y != 0);
    x += 8;
  } while (x < width);
}

// Take advantage of |src_stride| == |width| to process two rows at a time.
template <int num_taps, bool is_compound = false>
void Filter2DVertical4xH(const uint16_t* src, void* const dst,
                         const ptrdiff_t dst_stride, const int height,
                         const __m128i* const taps) {
  auto* dst8 = static_cast<uint8_t*>(dst);
  auto* dst16 = static_cast<uint16_t*>(dst);

  __m128i srcs[9];
  srcs[0] = LoadAligned16(src);
  src += 8;
  if (num_taps >= 4) {
    srcs[2] = LoadAligned16(src);
    src += 8;
    srcs[1] = _mm_unpacklo_epi64(_mm_srli_si128(srcs[0], 8), srcs[2]);
    if (num_taps >= 6) {
      srcs[4] = LoadAligned16(src);
      src += 8;
      srcs[3] = _mm_unpacklo_epi64(_mm_srli_si128(srcs[2], 8), srcs[4]);
      if (num_taps == 8) {
        srcs[6] = LoadAligned16(src);
        src += 8;
        srcs[5] = _mm_unpacklo_epi64(_mm_srli_si128(srcs[4], 8), srcs[6]);
      }
    }
  }

  int y = height;
  do {
    srcs[num_taps] = LoadAligned16(src);
    src += 8;
    srcs[num_taps - 1] = _mm_unpacklo_epi64(
        _mm_srli_si128(srcs[num_taps - 2], 8), srcs[num_taps]);

    const __m128i sum =
        SimpleSum2DVerticalTaps<num_taps, is_compound>(srcs, taps);
    if (is_compound) {
      StoreUnaligned16(dst16, sum);
      dst16 += 4 << 1;
    } else {
      const __m128i results = _mm_packus_epi16(sum, sum);
      Store4(dst8, results);
      dst8 += dst_stride;
      Store4(dst8, _mm_srli_si128(results, 4));
      dst8 += dst_stride;
    }

    srcs[0] = srcs[2];
    if (num_taps >= 4) {
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      if (num_taps >= 6) {
        srcs[3] = srcs[5];
        srcs[4] = srcs[6];
        if (num_taps == 8) {
          srcs[5] = srcs[7];
          srcs[6] = srcs[8];
        }
      }
    }
    y -= 2;
  } while (y != 0);
}

// Take advantage of |src_stride| == |width| to process four rows at a time.
template <int num_taps>
void Filter2DVertical2xH(const uint16_t* src, void* const dst,
                         const ptrdiff_t dst_stride, const int height,
                         const __m128i* const taps) {
  constexpr int next_row = (num_taps < 6) ? 4 : 8;

  auto* dst8 = static_cast<uint8_t*>(dst);

  __m128i srcs[9];
  srcs[0] = LoadAligned16(src);
  src += 8;
  if (num_taps >= 6) {
    srcs[4] = LoadAligned16(src);
    src += 8;
    srcs[1] = _mm_alignr_epi8(srcs[4], srcs[0], 4);
    if (num_taps == 8) {
      srcs[2] = _mm_alignr_epi8(srcs[4], srcs[0], 8);
      srcs[3] = _mm_alignr_epi8(srcs[4], srcs[0], 12);
    }
  }

  int y = height;
  do {
    srcs[next_row] = LoadAligned16(src);
    src += 8;
    if (num_taps == 2) {
      srcs[1] = _mm_alignr_epi8(srcs[4], srcs[0], 4);
    } else if (num_taps == 4) {
      srcs[1] = _mm_alignr_epi8(srcs[4], srcs[0], 4);
      srcs[2] = _mm_alignr_epi8(srcs[4], srcs[0], 8);
      srcs[3] = _mm_alignr_epi8(srcs[4], srcs[0], 12);
    } else if (num_taps == 6) {
      srcs[2] = _mm_alignr_epi8(srcs[4], srcs[0], 8);
      srcs[3] = _mm_alignr_epi8(srcs[4], srcs[0], 12);
      srcs[5] = _mm_alignr_epi8(srcs[8], srcs[4], 4);
    } else if (num_taps == 8) {
      srcs[5] = _mm_alignr_epi8(srcs[8], srcs[4], 4);
      srcs[6] = _mm_alignr_epi8(srcs[8], srcs[4], 8);
      srcs[7] = _mm_alignr_epi8(srcs[8], srcs[4], 12);
    }

    const __m128i sum =
        SimpleSum2DVerticalTaps<num_taps, /*is_compound=*/false>(srcs, taps);
    const __m128i results = _mm_packus_epi16(sum, sum);

    Store2(dst8, results);
    dst8 += dst_stride;
    Store2(dst8, _mm_srli_si128(results, 2));
    // When |height| <= 4 the taps are restricted to 2 and 4 tap variants.
    // Therefore we don't need to check this condition when |height| > 4.
    if (num_taps <= 4 && height == 2) return;
    dst8 += dst_stride;
    Store2(dst8, _mm_srli_si128(results, 4));
    dst8 += dst_stride;
    Store2(dst8, _mm_srli_si128(results, 6));
    dst8 += dst_stride;

    srcs[0] = srcs[4];
    if (num_taps == 6) {
      srcs[1] = srcs[5];
      srcs[4] = srcs[8];
    } else if (num_taps == 8) {
      srcs[1] = srcs[5];
      srcs[2] = srcs[6];
      srcs[3] = srcs[7];
      srcs[4] = srcs[8];
    }

    y -= 4;
  } while (y != 0);
}

// The 1D compound shift is always |kInterRoundBitsHorizontal|, even for 1D
// Vertical calculations.
__m128i Compound1DShift(const __m128i sum) {
  return RightShiftWithRounding_S16(sum, kInterRoundBitsHorizontal - 1);
}

template <int num_taps>
__m128i SumVerticalTaps(const __m128i* const srcs, const __m128i* const v_tap) {
  __m128i v_src[4];

  if (num_taps == 6) {
    // 6 taps.
    v_src[0] = _mm_unpacklo_epi8(srcs[0], srcs[1]);
    v_src[1] = _mm_unpacklo_epi8(srcs[2], srcs[3]);
    v_src[2] = _mm_unpacklo_epi8(srcs[4], srcs[5]);
  } else if (num_taps == 8) {
    // 8 taps.
    v_src[0] = _mm_unpacklo_epi8(srcs[0], srcs[1]);
    v_src[1] = _mm_unpacklo_epi8(srcs[2], srcs[3]);
    v_src[2] = _mm_unpacklo_epi8(srcs[4], srcs[5]);
    v_src[3] = _mm_unpacklo_epi8(srcs[6], srcs[7]);
  } else if (num_taps == 2) {
    // 2 taps.
    v_src[0] = _mm_unpacklo_epi8(srcs[0], srcs[1]);
  } else {
    // 4 taps.
    v_src[0] = _mm_unpacklo_epi8(srcs[0], srcs[1]);
    v_src[1] = _mm_unpacklo_epi8(srcs[2], srcs[3]);
  }
  const __m128i sum = SumOnePassTaps<num_taps>(v_src, v_tap);
  return sum;
}

template <int num_taps, bool is_compound = false>
void FilterVertical4xH(const uint8_t* src, const ptrdiff_t src_stride,
                       void* const dst, const ptrdiff_t dst_stride,
                       const int height, const __m128i* const v_tap) {
  auto* dst8 = static_cast<uint8_t*>(dst);
  auto* dst16 = static_cast<uint16_t*>(dst);

  __m128i srcs[9];

  if (num_taps == 2) {
    srcs[2] = _mm_setzero_si128();
    // 00 01 02 03
    srcs[0] = Load4(src);
    src += src_stride;

    int y = height;
    do {
      // 10 11 12 13
      const __m128i a = Load4(src);
      // 00 01 02 03 10 11 12 13
      srcs[0] = _mm_unpacklo_epi32(srcs[0], a);
      src += src_stride;
      // 20 21 22 23
      srcs[2] = Load4(src);
      src += src_stride;
      // 10 11 12 13 20 21 22 23
      srcs[1] = _mm_unpacklo_epi32(a, srcs[2]);

      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      if (is_compound) {
        const __m128i results = Compound1DShift(sums);
        StoreUnaligned16(dst16, results);
        dst16 += 4 << 1;
      } else {
        const __m128i results_16 =
            RightShiftWithRounding_S16(sums, kFilterBits - 1);
        const __m128i results = _mm_packus_epi16(results_16, results_16);
        Store4(dst8, results);
        dst8 += dst_stride;
        Store4(dst8, _mm_srli_si128(results, 4));
        dst8 += dst_stride;
      }

      srcs[0] = srcs[2];
      y -= 2;
    } while (y != 0);
  } else if (num_taps == 4) {
    srcs[4] = _mm_setzero_si128();
    // 00 01 02 03
    srcs[0] = Load4(src);
    src += src_stride;
    // 10 11 12 13
    const __m128i a = Load4(src);
    // 00 01 02 03 10 11 12 13
    srcs[0] = _mm_unpacklo_epi32(srcs[0], a);
    src += src_stride;
    // 20 21 22 23
    srcs[2] = Load4(src);
    src += src_stride;
    // 10 11 12 13 20 21 22 23
    srcs[1] = _mm_unpacklo_epi32(a, srcs[2]);

    int y = height;
    do {
      // 30 31 32 33
      const __m128i b = Load4(src);
      // 20 21 22 23 30 31 32 33
      srcs[2] = _mm_unpacklo_epi32(srcs[2], b);
      src += src_stride;
      // 40 41 42 43
      srcs[4] = Load4(src);
      src += src_stride;
      // 30 31 32 33 40 41 42 43
      srcs[3] = _mm_unpacklo_epi32(b, srcs[4]);

      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      if (is_compound) {
        const __m128i results = Compound1DShift(sums);
        StoreUnaligned16(dst16, results);
        dst16 += 4 << 1;
      } else {
        const __m128i results_16 =
            RightShiftWithRounding_S16(sums, kFilterBits - 1);
        const __m128i results = _mm_packus_epi16(results_16, results_16);
        Store4(dst8, results);
        dst8 += dst_stride;
        Store4(dst8, _mm_srli_si128(results, 4));
        dst8 += dst_stride;
      }

      srcs[0] = srcs[2];
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      y -= 2;
    } while (y != 0);
  } else if (num_taps == 6) {
    srcs[6] = _mm_setzero_si128();
    // 00 01 02 03
    srcs[0] = Load4(src);
    src += src_stride;
    // 10 11 12 13
    const __m128i a = Load4(src);
    // 00 01 02 03 10 11 12 13
    srcs[0] = _mm_unpacklo_epi32(srcs[0], a);
    src += src_stride;
    // 20 21 22 23
    srcs[2] = Load4(src);
    src += src_stride;
    // 10 11 12 13 20 21 22 23
    srcs[1] = _mm_unpacklo_epi32(a, srcs[2]);
    // 30 31 32 33
    const __m128i b = Load4(src);
    // 20 21 22 23 30 31 32 33
    srcs[2] = _mm_unpacklo_epi32(srcs[2], b);
    src += src_stride;
    // 40 41 42 43
    srcs[4] = Load4(src);
    src += src_stride;
    // 30 31 32 33 40 41 42 43
    srcs[3] = _mm_unpacklo_epi32(b, srcs[4]);

    int y = height;
    do {
      // 50 51 52 53
      const __m128i c = Load4(src);
      // 40 41 42 43 50 51 52 53
      srcs[4] = _mm_unpacklo_epi32(srcs[4], c);
      src += src_stride;
      // 60 61 62 63
      srcs[6] = Load4(src);
      src += src_stride;
      // 50 51 52 53 60 61 62 63
      srcs[5] = _mm_unpacklo_epi32(c, srcs[6]);

      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      if (is_compound) {
        const __m128i results = Compound1DShift(sums);
        StoreUnaligned16(dst16, results);
        dst16 += 4 << 1;
      } else {
        const __m128i results_16 =
            RightShiftWithRounding_S16(sums, kFilterBits - 1);
        const __m128i results = _mm_packus_epi16(results_16, results_16);
        Store4(dst8, results);
        dst8 += dst_stride;
        Store4(dst8, _mm_srli_si128(results, 4));
        dst8 += dst_stride;
      }

      srcs[0] = srcs[2];
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      srcs[3] = srcs[5];
      srcs[4] = srcs[6];
      y -= 2;
    } while (y != 0);
  } else if (num_taps == 8) {
    srcs[8] = _mm_setzero_si128();
    // 00 01 02 03
    srcs[0] = Load4(src);
    src += src_stride;
    // 10 11 12 13
    const __m128i a = Load4(src);
    // 00 01 02 03 10 11 12 13
    srcs[0] = _mm_unpacklo_epi32(srcs[0], a);
    src += src_stride;
    // 20 21 22 23
    srcs[2] = Load4(src);
    src += src_stride;
    // 10 11 12 13 20 21 22 23
    srcs[1] = _mm_unpacklo_epi32(a, srcs[2]);
    // 30 31 32 33
    const __m128i b = Load4(src);
    // 20 21 22 23 30 31 32 33
    srcs[2] = _mm_unpacklo_epi32(srcs[2], b);
    src += src_stride;
    // 40 41 42 43
    srcs[4] = Load4(src);
    src += src_stride;
    // 30 31 32 33 40 41 42 43
    srcs[3] = _mm_unpacklo_epi32(b, srcs[4]);
    // 50 51 52 53
    const __m128i c = Load4(src);
    // 40 41 42 43 50 51 52 53
    srcs[4] = _mm_unpacklo_epi32(srcs[4], c);
    src += src_stride;
    // 60 61 62 63
    srcs[6] = Load4(src);
    src += src_stride;
    // 50 51 52 53 60 61 62 63
    srcs[5] = _mm_unpacklo_epi32(c, srcs[6]);

    int y = height;
    do {
      // 70 71 72 73
      const __m128i d = Load4(src);
      // 60 61 62 63 70 71 72 73
      srcs[6] = _mm_unpacklo_epi32(srcs[6], d);
      src += src_stride;
      // 80 81 82 83
      srcs[8] = Load4(src);
      src += src_stride;
      // 70 71 72 73 80 81 82 83
      srcs[7] = _mm_unpacklo_epi32(d, srcs[8]);

      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      if (is_compound) {
        const __m128i results = Compound1DShift(sums);
        StoreUnaligned16(dst16, results);
        dst16 += 4 << 1;
      } else {
        const __m128i results_16 =
            RightShiftWithRounding_S16(sums, kFilterBits - 1);
        const __m128i results = _mm_packus_epi16(results_16, results_16);
        Store4(dst8, results);
        dst8 += dst_stride;
        Store4(dst8, _mm_srli_si128(results, 4));
        dst8 += dst_stride;
      }

      srcs[0] = srcs[2];
      srcs[1] = srcs[3];
      srcs[2] = srcs[4];
      srcs[3] = srcs[5];
      srcs[4] = srcs[6];
      srcs[5] = srcs[7];
      srcs[6] = srcs[8];
      y -= 2;
    } while (y != 0);
  }
}

template <int num_taps, bool negative_outside_taps = false>
void FilterVertical2xH(const uint8_t* src, const ptrdiff_t src_stride,
                       void* const dst, const ptrdiff_t dst_stride,
                       const int height, const __m128i* const v_tap) {
  auto* dst8 = static_cast<uint8_t*>(dst);

  __m128i srcs[9];

  if (num_taps == 2) {
    srcs[2] = _mm_setzero_si128();
    // 00 01
    srcs[0] = Load2(src);
    src += src_stride;

    int y = height;
    do {
      // 00 01 10 11
      srcs[0] = Load2<1>(src, srcs[0]);
      src += src_stride;
      // 00 01 10 11 20 21
      srcs[0] = Load2<2>(src, srcs[0]);
      src += src_stride;
      // 00 01 10 11 20 21 30 31
      srcs[0] = Load2<3>(src, srcs[0]);
      src += src_stride;
      // 40 41
      srcs[2] = Load2<0>(src, srcs[2]);
      src += src_stride;
      // 00 01 10 11 20 21 30 31 40 41
      const __m128i srcs_0_2 = _mm_unpacklo_epi64(srcs[0], srcs[2]);
      // 10 11 20 21 30 31 40 41
      srcs[1] = _mm_srli_si128(srcs_0_2, 2);
      // This uses srcs[0]..srcs[1].
      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      const __m128i results_16 =
          RightShiftWithRounding_S16(sums, kFilterBits - 1);
      const __m128i results = _mm_packus_epi16(results_16, results_16);

      Store2(dst8, results);
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 2));
      if (height == 2) return;
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 4));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 6));
      dst8 += dst_stride;

      srcs[0] = srcs[2];
      y -= 4;
    } while (y != 0);
  } else if (num_taps == 4) {
    srcs[4] = _mm_setzero_si128();

    // 00 01
    srcs[0] = Load2(src);
    src += src_stride;
    // 00 01 10 11
    srcs[0] = Load2<1>(src, srcs[0]);
    src += src_stride;
    // 00 01 10 11 20 21
    srcs[0] = Load2<2>(src, srcs[0]);
    src += src_stride;

    int y = height;
    do {
      // 00 01 10 11 20 21 30 31
      srcs[0] = Load2<3>(src, srcs[0]);
      src += src_stride;
      // 40 41
      srcs[4] = Load2<0>(src, srcs[4]);
      src += src_stride;
      // 40 41 50 51
      srcs[4] = Load2<1>(src, srcs[4]);
      src += src_stride;
      // 40 41 50 51 60 61
      srcs[4] = Load2<2>(src, srcs[4]);
      src += src_stride;
      // 00 01 10 11 20 21 30 31 40 41 50 51 60 61
      const __m128i srcs_0_4 = _mm_unpacklo_epi64(srcs[0], srcs[4]);
      // 10 11 20 21 30 31 40 41
      srcs[1] = _mm_srli_si128(srcs_0_4, 2);
      // 20 21 30 31 40 41 50 51
      srcs[2] = _mm_srli_si128(srcs_0_4, 4);
      // 30 31 40 41 50 51 60 61
      srcs[3] = _mm_srli_si128(srcs_0_4, 6);

      // This uses srcs[0]..srcs[3].
      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      const __m128i results_16 =
          RightShiftWithRounding_S16(sums, kFilterBits - 1);
      const __m128i results = _mm_packus_epi16(results_16, results_16);

      Store2(dst8, results);
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 2));
      if (height == 2) return;
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 4));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 6));
      dst8 += dst_stride;

      srcs[0] = srcs[4];
      y -= 4;
    } while (y != 0);
  } else if (num_taps == 6) {
    // During the vertical pass the number of taps is restricted when
    // |height| <= 4.
    assert(height > 4);
    srcs[8] = _mm_setzero_si128();

    // 00 01
    srcs[0] = Load2(src);
    src += src_stride;
    // 00 01 10 11
    srcs[0] = Load2<1>(src, srcs[0]);
    src += src_stride;
    // 00 01 10 11 20 21
    srcs[0] = Load2<2>(src, srcs[0]);
    src += src_stride;
    // 00 01 10 11 20 21 30 31
    srcs[0] = Load2<3>(src, srcs[0]);
    src += src_stride;
    // 40 41
    srcs[4] = Load2(src);
    src += src_stride;
    // 00 01 10 11 20 21 30 31 40 41 50 51 60 61
    const __m128i srcs_0_4x = _mm_unpacklo_epi64(srcs[0], srcs[4]);
    // 10 11 20 21 30 31 40 41
    srcs[1] = _mm_srli_si128(srcs_0_4x, 2);

    int y = height;
    do {
      // 40 41 50 51
      srcs[4] = Load2<1>(src, srcs[4]);
      src += src_stride;
      // 40 41 50 51 60 61
      srcs[4] = Load2<2>(src, srcs[4]);
      src += src_stride;
      // 40 41 50 51 60 61 70 71
      srcs[4] = Load2<3>(src, srcs[4]);
      src += src_stride;
      // 80 81
      srcs[8] = Load2<0>(src, srcs[8]);
      src += src_stride;
      // 00 01 10 11 20 21 30 31 40 41 50 51 60 61
      const __m128i srcs_0_4 = _mm_unpacklo_epi64(srcs[0], srcs[4]);
      // 20 21 30 31 40 41 50 51
      srcs[2] = _mm_srli_si128(srcs_0_4, 4);
      // 30 31 40 41 50 51 60 61
      srcs[3] = _mm_srli_si128(srcs_0_4, 6);
      const __m128i srcs_4_8 = _mm_unpacklo_epi64(srcs[4], srcs[8]);
      // 50 51 60 61 70 71 80 81
      srcs[5] = _mm_srli_si128(srcs_4_8, 2);

      // This uses srcs[0]..srcs[5].
      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      const __m128i results_16 =
          RightShiftWithRounding_S16(sums, kFilterBits - 1);
      const __m128i results = _mm_packus_epi16(results_16, results_16);

      Store2(dst8, results);
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 2));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 4));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 6));
      dst8 += dst_stride;

      srcs[0] = srcs[4];
      srcs[1] = srcs[5];
      srcs[4] = srcs[8];
      y -= 4;
    } while (y != 0);
  } else if (num_taps == 8) {
    // During the vertical pass the number of taps is restricted when
    // |height| <= 4.
    assert(height > 4);
    srcs[8] = _mm_setzero_si128();
    // 00 01
    srcs[0] = Load2(src);
    src += src_stride;
    // 00 01 10 11
    srcs[0] = Load2<1>(src, srcs[0]);
    src += src_stride;
    // 00 01 10 11 20 21
    srcs[0] = Load2<2>(src, srcs[0]);
    src += src_stride;
    // 00 01 10 11 20 21 30 31
    srcs[0] = Load2<3>(src, srcs[0]);
    src += src_stride;
    // 40 41
    srcs[4] = Load2(src);
    src += src_stride;
    // 40 41 50 51
    srcs[4] = Load2<1>(src, srcs[4]);
    src += src_stride;
    // 40 41 50 51 60 61
    srcs[4] = Load2<2>(src, srcs[4]);
    src += src_stride;

    // 00 01 10 11 20 21 30 31 40 41 50 51 60 61
    const __m128i srcs_0_4 = _mm_unpacklo_epi64(srcs[0], srcs[4]);
    // 10 11 20 21 30 31 40 41
    srcs[1] = _mm_srli_si128(srcs_0_4, 2);
    // 20 21 30 31 40 41 50 51
    srcs[2] = _mm_srli_si128(srcs_0_4, 4);
    // 30 31 40 41 50 51 60 61
    srcs[3] = _mm_srli_si128(srcs_0_4, 6);

    int y = height;
    do {
      // 40 41 50 51 60 61 70 71
      srcs[4] = Load2<3>(src, srcs[4]);
      src += src_stride;
      // 80 81
      srcs[8] = Load2<0>(src, srcs[8]);
      src += src_stride;
      // 80 81 90 91
      srcs[8] = Load2<1>(src, srcs[8]);
      src += src_stride;
      // 80 81 90 91 a0 a1
      srcs[8] = Load2<2>(src, srcs[8]);
      src += src_stride;

      // 40 41 50 51 60 61 70 71 80 81 90 91 a0 a1
      const __m128i srcs_4_8 = _mm_unpacklo_epi64(srcs[4], srcs[8]);
      // 50 51 60 61 70 71 80 81
      srcs[5] = _mm_srli_si128(srcs_4_8, 2);
      // 60 61 70 71 80 81 90 91
      srcs[6] = _mm_srli_si128(srcs_4_8, 4);
      // 70 71 80 81 90 91 a0 a1
      srcs[7] = _mm_srli_si128(srcs_4_8, 6);

      // This uses srcs[0]..srcs[7].
      const __m128i sums = SumVerticalTaps<num_taps>(srcs, v_tap);
      const __m128i results_16 =
          RightShiftWithRounding_S16(sums, kFilterBits - 1);
      const __m128i results = _mm_packus_epi16(results_16, results_16);

      Store2(dst8, results);
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 2));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 4));
      dst8 += dst_stride;
      Store2(dst8, _mm_srli_si128(results, 6));
      dst8 += dst_stride;

      srcs[0] = srcs[4];
      srcs[1] = srcs[5];
      srcs[2] = srcs[6];
      srcs[3] = srcs[7];
      srcs[4] = srcs[8];
      y -= 4;
    } while (y != 0);
  }
}
