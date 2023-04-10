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

#include "src/dsp/inverse_transform.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "src/dsp/dsp.h"
#include "src/utils/array_2d.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/logging.h"

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
#undef LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK
#endif

#if defined(LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK) && \
    LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK
#include <cinttypes>
#endif

namespace libgav1 {
namespace dsp {
namespace {

// Include the constants and utility functions inside the anonymous namespace.
#include "src/dsp/inverse_transform.inc"

constexpr uint8_t kTransformColumnShift = 4;

template <typename T>
int32_t RangeCheckValue(T value, int8_t range) {
#if defined(LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK) && \
    LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK
  static_assert(
      std::is_same<T, int32_t>::value || std::is_same<T, std::int64_t>::value,
      "");
  assert(range <= 32);
  const auto min = static_cast<int32_t>(-(uint32_t{1} << (range - 1)));
  const auto max = static_cast<int32_t>((uint32_t{1} << (range - 1)) - 1);
  if (min > value || value > max) {
    LIBGAV1_DLOG(ERROR,
                 "coeff out of bit range, value: %" PRId64 " bit range %d",
                 static_cast<int64_t>(value), range);
    assert(min <= value && value <= max);
  }
#endif  // LIBGAV1_ENABLE_TRANSFORM_RANGE_CHECK
  static_cast<void>(range);
  return static_cast<int32_t>(value);
}

template <typename Residual>
LIBGAV1_ALWAYS_INLINE void ButterflyRotation_C(Residual* const dst, int a,
                                               int b, int angle, bool flip,
                                               int8_t range) {
  // Note that we multiply in 32 bits and then add/subtract the products in 64
  // bits. The 32-bit multiplications do not overflow. Please see the comment
  // and assert() in Cos128().
  const int64_t x = static_cast<int64_t>(dst[a] * Cos128(angle)) -
                    static_cast<int64_t>(dst[b] * Sin128(angle));
  const int64_t y = static_cast<int64_t>(dst[a] * Sin128(angle)) +
                    static_cast<int64_t>(dst[b] * Cos128(angle));
  // Section 7.13.2.1: It is a requirement of bitstream conformance that the
  // values saved into the array T by this function are representable by a
  // signed integer using |range| bits of precision.
  dst[a] = RangeCheckValue(RightShiftWithRounding(flip ? y : x, 12), range);
  dst[b] = RangeCheckValue(RightShiftWithRounding(flip ? x : y, 12), range);
}

template <typename Residual>
void ButterflyRotationFirstIsZero_C(Residual* const dst, int a, int b,
                                    int angle, bool flip, int8_t range) {
  // Note that we multiply in 32 bits and then add/subtract the products in 64
  // bits. The 32-bit multiplications do not overflow. Please see the comment
  // and assert() in Cos128().
  const auto x = static_cast<int64_t>(dst[b] * -Sin128(angle));
  const auto y = static_cast<int64_t>(dst[b] * Cos128(angle));
  // Section 7.13.2.1: It is a requirement of bitstream conformance that the
  // values saved into the array T by this function are representable by a
  // signed integer using |range| bits of precision.
  dst[a] = RangeCheckValue(RightShiftWithRounding(flip ? y : x, 12), range);
  dst[b] = RangeCheckValue(RightShiftWithRounding(flip ? x : y, 12), range);
}

template <typename Residual>
void ButterflyRotationSecondIsZero_C(Residual* const dst, int a, int b,
                                     int angle, bool flip, int8_t range) {
  // Note that we multiply in 32 bits and then add/subtract the products in 64
  // bits. The 32-bit multiplications do not overflow. Please see the comment
  // and assert() in Cos128().
  const auto x = static_cast<int64_t>(dst[a] * Cos128(angle));
  const auto y = static_cast<int64_t>(dst[a] * Sin128(angle));

  // Section 7.13.2.1: It is a requirement of bitstream conformance that the
  // values saved into the array T by this function are representable by a
  // signed integer using |range| bits of precision.
  dst[a] = RangeCheckValue(RightShiftWithRounding(flip ? y : x, 12), range);
  dst[b] = RangeCheckValue(RightShiftWithRounding(flip ? x : y, 12), range);
}

template <typename Residual>
void HadamardRotation_C(Residual* const dst, int a, int b, bool flip,
                        int8_t range) {
  if (flip) std::swap(a, b);
  --range;
  // For Adst and Dct, the maximum possible value for range is 20. So min and
  // max should always fit into int32_t.
  const int32_t min = -(1 << range);
  const int32_t max = (1 << range) - 1;
  const int32_t x = dst[a] + dst[b];
  const int32_t y = dst[a] - dst[b];
  dst[a] = Clip3(x, min, max);
  dst[b] = Clip3(y, min, max);
}

template <int bitdepth, typename Residual>
void ClampIntermediate(Residual* const dst, int size) {
  // If Residual is int16_t (which implies bitdepth is 8), we don't need to
  // clip residual[i][j] to 16 bits.
  if (sizeof(Residual) > 2) {
    const Residual intermediate_clamp_max =
        (1 << (std::max(bitdepth + 6, 16) - 1)) - 1;
    const Residual intermediate_clamp_min = -intermediate_clamp_max - 1;
    for (int j = 0; j < size; ++j) {
      dst[j] = Clip3(dst[j], intermediate_clamp_min, intermediate_clamp_max);
    }
  }
}

//------------------------------------------------------------------------------
// Discrete Cosine Transforms (DCT).

// Value for index (i, j) is computed as bitreverse(j) and interpreting that as
// an integer with bit-length i + 2.
// For e.g. index (2, 3) will be computed as follows:
//   * bitreverse(3) = bitreverse(..000011) = 110000...
//   * interpreting that as an integer with bit-length 2+2 = 4 will be 1100 = 12
constexpr uint8_t kBitReverseLookup[kNumTransform1dSizes][64] = {
    {0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2,
     1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3,
     0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3},
    {0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6, 1, 5,
     3, 7, 0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6,
     1, 5, 3, 7, 0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6, 1, 5, 3, 7},
    {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15,
     0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15,
     0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15,
     0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15},
    {0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
     1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31,
     0, 16, 8, 24, 4, 20, 12, 28, 2, 18, 10, 26, 6, 22, 14, 30,
     1, 17, 9, 25, 5, 21, 13, 29, 3, 19, 11, 27, 7, 23, 15, 31},
    {0, 32, 16, 48, 8,  40, 24, 56, 4, 36, 20, 52, 12, 44, 28, 60,
     2, 34, 18, 50, 10, 42, 26, 58, 6, 38, 22, 54, 14, 46, 30, 62,
     1, 33, 17, 49, 9,  41, 25, 57, 5, 37, 21, 53, 13, 45, 29, 61,
     3, 35, 19, 51, 11, 43, 27, 59, 7, 39, 23, 55, 15, 47, 31, 63}};

template <typename Residual, int size_log2>
void Dct_C(void* dest, int8_t range) {
  static_assert(size_log2 >= 2 && size_log2 <= 6, "");
  auto* const dst = static_cast<Residual*>(dest);
  // stage 1.
  const int size = 1 << size_log2;
  Residual temp[size];
  memcpy(temp, dst, sizeof(temp));
  for (int i = 0; i < size; ++i) {
    dst[i] = temp[kBitReverseLookup[size_log2 - 2][i]];
  }
  // stages 2-32 are dependent on the value of size_log2.
  // stage 2.
  if (size_log2 == 6) {
    for (int i = 0; i < 16; ++i) {
      ButterflyRotation_C(dst, i + 32, 63 - i,
                          63 - MultiplyBy4(kBitReverseLookup[2][i]), false,
                          range);
    }
  }
  // stage 3
  if (size_log2 >= 5) {
    for (int i = 0; i < 8; ++i) {
      ButterflyRotation_C(dst, i + 16, 31 - i,
                          6 + MultiplyBy8(kBitReverseLookup[1][7 - i]), false,
                          range);
    }
  }
  // stage 4.
  if (size_log2 == 6) {
    for (int i = 0; i < 16; ++i) {
      HadamardRotation_C(dst, MultiplyBy2(i) + 32, MultiplyBy2(i) + 33,
                         static_cast<bool>(i & 1), range);
    }
  }
  // stage 5.
  if (size_log2 >= 4) {
    for (int i = 0; i < 4; ++i) {
      ButterflyRotation_C(dst, i + 8, 15 - i,
                          12 + MultiplyBy16(kBitReverseLookup[0][3 - i]), false,
                          range);
    }
  }
  // stage 6.
  if (size_log2 >= 5) {
    for (int i = 0; i < 8; ++i) {
      HadamardRotation_C(dst, MultiplyBy2(i) + 16, MultiplyBy2(i) + 17,
                         static_cast<bool>(i & 1), range);
    }
  }
  // stage 7.
  if (size_log2 == 6) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 2; ++j) {
        ButterflyRotation_C(
            dst, 62 - MultiplyBy4(i) - j, MultiplyBy4(i) + j + 33,
            60 - MultiplyBy16(kBitReverseLookup[0][i]) + MultiplyBy64(j), true,
            range);
      }
    }
  }
  // stage 8.
  if (size_log2 >= 3) {
    for (int i = 0; i < 2; ++i) {
      ButterflyRotation_C(dst, i + 4, 7 - i, 56 - 32 * i, false, range);
    }
  }
  // stage 9.
  if (size_log2 >= 4) {
    for (int i = 0; i < 4; ++i) {
      HadamardRotation_C(dst, MultiplyBy2(i) + 8, MultiplyBy2(i) + 9,
                         static_cast<bool>(i & 1), range);
    }
  }
  // stage 10.
  if (size_log2 >= 5) {
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        ButterflyRotation_C(
            dst, 30 - MultiplyBy4(i) - j, MultiplyBy4(i) + j + 17,
            24 + MultiplyBy64(j) + MultiplyBy32(1 - i), true, range);
      }
    }
  }
  // stage 11.
  if (size_log2 == 6) {
    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 2; ++j) {
        HadamardRotation_C(dst, MultiplyBy4(i) + j + 32,
                           MultiplyBy4(i) - j + 35, static_cast<bool>(i & 1),
                           range);
      }
    }
  }
  // stage 12.
  for (int i = 0; i < 2; ++i) {
    ButterflyRotation_C(dst, MultiplyBy2(i), MultiplyBy2(i) + 1, 32 + 16 * i,
                        i == 0, range);
  }
  // stage 13.
  if (size_log2 >= 3) {
    for (int i = 0; i < 2; ++i) {
      HadamardRotation_C(dst, MultiplyBy2(i) + 4, MultiplyBy2(i) + 5,
                         /*flip=*/i != 0, range);
    }
  }
  // stage 14.
  if (size_log2 >= 4) {
    for (int i = 0; i < 2; ++i) {
      ButterflyRotation_C(dst, 14 - i, i + 9, 48 + 64 * i, true, range);
    }
  }
  // stage 15.
  if (size_log2 >= 5) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 2; ++j) {
        HadamardRotation_C(dst, MultiplyBy4(i) + j + 16,
                           MultiplyBy4(i) - j + 19, static_cast<bool>(i & 1),
                           range);
      }
    }
  }
  // stage 16.
  if (size_log2 == 6) {
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 4; ++j) {
        ButterflyRotation_C(
            dst, 61 - MultiplyBy8(i) - j, MultiplyBy8(i) + j + 34,
            56 - MultiplyBy32(i) + MultiplyBy64(DivideBy2(j)), true, range);
      }
    }
  }
  // stage 17.
  for (int i = 0; i < 2; ++i) {
    HadamardRotation_C(dst, i, 3 - i, false, range);
  }
  // stage 18.
  if (size_log2 >= 3) {
    ButterflyRotation_C(dst, 6, 5, 32, true, range);
  }
  // stage 19.
  if (size_log2 >= 4) {
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        HadamardRotation_C(dst, MultiplyBy4(i) + j + 8, MultiplyBy4(i) - j + 11,
                           /*flip=*/i != 0, range);
      }
    }
  }
  // stage 20.
  if (size_log2 >= 5) {
    for (int i = 0; i < 4; ++i) {
      ButterflyRotation_C(dst, 29 - i, i + 18, 48 + 64 * DivideBy2(i), true,
                          range);
    }
  }
  // stage 21.
  if (size_log2 == 6) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        HadamardRotation_C(dst, MultiplyBy8(i) + j + 32,
                           MultiplyBy8(i) - j + 39, static_cast<bool>(i & 1),
                           range);
      }
    }
  }
  // stage 22.
  if (size_log2 >= 3) {
    for (int i = 0; i < 4; ++i) {
      HadamardRotation_C(dst, i, 7 - i, false, range);
    }
  }
  // stage 23.
  if (size_log2 >= 4) {
    for (int i = 0; i < 2; ++i) {
      ButterflyRotation_C(dst, 13 - i, i + 10, 32, true, range);
    }
  }
  // stage 24.
  if (size_log2 >= 5) {
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 4; ++j) {
        HadamardRotation_C(dst, MultiplyBy8(i) + j + 16,
                           MultiplyBy8(i) - j + 23, i == 1, range);
      }
    }
  }
  // stage 25.
  if (size_log2 == 6) {
    for (int i = 0; i < 8; ++i) {
      ButterflyRotation_C(dst, 59 - i, i + 36, (i < 4) ? 48 : 112, true, range);
    }
  }
  // stage 26.
  if (size_log2 >= 4) {
    for (int i = 0; i < 8; ++i) {
      HadamardRotation_C(dst, i, 15 - i, false, range);
    }
  }
  // stage 27.
  if (size_log2 >= 5) {
    for (int i = 0; i < 4; ++i) {
      ButterflyRotation_C(dst, 27 - i, i + 20, 32, true, range);
    }
  }
  // stage 28.
  if (size_log2 == 6) {
    for (int i = 0; i < 8; ++i) {
      HadamardRotation_C(dst, i + 32, 47 - i, false, range);
      HadamardRotation_C(dst, i + 48, 63 - i, true, range);
    }
  }
  // stage 29.
  if (size_log2 >= 5) {
    for (int i = 0; i < 16; ++i) {
      HadamardRotation_C(dst, i, 31 - i, false, range);
    }
  }
  // stage 30.
  if (size_log2 == 6) {
    for (int i = 0; i < 8; ++i) {
      ButterflyRotation_C(dst, 55 - i, i + 40, 32, true, range);
    }
  }
  // stage 31.
  if (size_log2 == 6) {
    for (int i = 0; i < 32; ++i) {
      HadamardRotation_C(dst, i, 63 - i, false, range);
    }
  }
}

template <int bitdepth, typename Residual, int size_log2>
void DctDcOnly_C(void* dest, int8_t range, bool should_round, int row_shift,
                 bool is_row) {
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row && should_round) {
    dst[0] = RightShiftWithRounding(dst[0] * kTransformRowMultiplier, 12);
  }

  ButterflyRotationSecondIsZero_C(dst, 0, 1, 32, true, range);

  if (is_row && row_shift > 0) {
    dst[0] = RightShiftWithRounding(dst[0], row_shift);
  }

  ClampIntermediate<bitdepth, Residual>(dst, 1);

  const int size = 1 << size_log2;
  for (int i = 1; i < size; ++i) {
    dst[i] = dst[0];
  }
}

//------------------------------------------------------------------------------
// Asymmetric Discrete Sine Transforms (ADST).

/*
 * Row transform max range in bits for bitdepths 8/10/12: 28/30/32.
 * Column transform max range in bits for bitdepths 8/10/12: 28/28/30.
 */
template <typename Residual>
void Adst4_C(void* dest, int8_t range) {
  auto* const dst = static_cast<Residual*>(dest);
  if ((dst[0] | dst[1] | dst[2] | dst[3]) == 0) {
    return;
  }

  // stage 1.
  // Section 7.13.2.6: It is a requirement of bitstream conformance that all
  // values stored in the s and x arrays by this process are representable by
  // a signed integer using range + 12 bits of precision.
  // Note the intermediate value can only exceed INT32_MAX with invalid 12-bit
  // content. For simplicity in unoptimized code, int64_t is used for both 10 &
  // 12-bit. SIMD implementations can allow these to rollover on platforms
  // where this has defined behavior.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  Intermediate s[7];
  s[0] = RangeCheckValue(kAdst4Multiplier[0] * dst[0], range + 12);
  s[1] = RangeCheckValue(kAdst4Multiplier[1] * dst[0], range + 12);
  s[2] = RangeCheckValue(kAdst4Multiplier[2] * dst[1], range + 12);
  s[3] = RangeCheckValue(kAdst4Multiplier[3] * dst[2], range + 12);
  s[4] = RangeCheckValue(kAdst4Multiplier[0] * dst[2], range + 12);
  s[5] = RangeCheckValue(kAdst4Multiplier[1] * dst[3], range + 12);
  s[6] = RangeCheckValue(kAdst4Multiplier[3] * dst[3], range + 12);
  // stage 2.
  // Section 7.13.2.6: It is a requirement of bitstream conformance that
  // values stored in the variable a7 by this process are representable by a
  // signed integer using range + 1 bits of precision.
  const int32_t a7 = RangeCheckValue(dst[0] - dst[2], range + 1);
  // Section 7.13.2.6: It is a requirement of bitstream conformance that
  // values stored in the variable b7 by this process are representable by a
  // signed integer using |range| bits of precision.
  const int32_t b7 = RangeCheckValue(a7 + dst[3], range);
  // stage 3.
  s[0] = RangeCheckValue(s[0] + s[3], range + 12);
  s[1] = RangeCheckValue(s[1] - s[4], range + 12);
  s[3] = s[2];
  // With range checking enabled b7 would be trapped above. This prevents an
  // integer sanitizer warning. In SIMD implementations the multiply can be
  // allowed to rollover on platforms where this has defined behavior.
  const auto adst2_b7 = static_cast<Intermediate>(kAdst4Multiplier[2]) * b7;
  s[2] = RangeCheckValue(adst2_b7, range + 12);
  // stage 4.
  s[0] = RangeCheckValue(s[0] + s[5], range + 12);
  s[1] = RangeCheckValue(s[1] - s[6], range + 12);
  // stages 5 and 6.
  const Intermediate x0 = RangeCheckValue(s[0] + s[3], range + 12);
  const Intermediate x1 = RangeCheckValue(s[1] + s[3], range + 12);
  Intermediate x3 = RangeCheckValue(s[0] + s[1], range + 12);
  x3 = RangeCheckValue(x3 - s[3], range + 12);
  auto dst_0 = static_cast<int32_t>(RightShiftWithRounding(x0, 12));
  auto dst_1 = static_cast<int32_t>(RightShiftWithRounding(x1, 12));
  auto dst_2 = static_cast<int32_t>(RightShiftWithRounding(s[2], 12));
  auto dst_3 = static_cast<int32_t>(RightShiftWithRounding(x3, 12));
  if (sizeof(Residual) == 2) {
    // If the first argument to RightShiftWithRounding(..., 12) is only
    // slightly smaller than 2^27 - 1 (e.g., 0x7fffe4e), adding 2^11 to it
    // in RightShiftWithRounding(..., 12) will cause the function to return
    // 0x8000, which cannot be represented as an int16_t. Change it to 0x7fff.
    dst_0 -= (dst_0 == 0x8000);
    dst_1 -= (dst_1 == 0x8000);
    dst_3 -= (dst_3 == 0x8000);
  }
  dst[0] = dst_0;
  dst[1] = dst_1;
  dst[2] = dst_2;
  dst[3] = dst_3;
}

template <int bitdepth, typename Residual>
void Adst4DcOnly_C(void* dest, int8_t range, bool should_round, int row_shift,
                   bool is_row) {
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row && should_round) {
    dst[0] = RightShiftWithRounding(dst[0] * kTransformRowMultiplier, 12);
  }

  // stage 1.
  // Section 7.13.2.6: It is a requirement of bitstream conformance that all
  // values stored in the s and x arrays by this process are representable by
  // a signed integer using range + 12 bits of precision.
  int32_t s[3];
  s[0] = RangeCheckValue(kAdst4Multiplier[0] * dst[0], range + 12);
  s[1] = RangeCheckValue(kAdst4Multiplier[1] * dst[0], range + 12);
  s[2] = RangeCheckValue(kAdst4Multiplier[2] * dst[0], range + 12);
  // stage 3.
  // stage 4.
  // stages 5 and 6.
  int32_t dst_0 = RightShiftWithRounding(s[0], 12);
  int32_t dst_1 = RightShiftWithRounding(s[1], 12);
  int32_t dst_2 = RightShiftWithRounding(s[2], 12);
  int32_t dst_3 =
      RightShiftWithRounding(RangeCheckValue(s[0] + s[1], range + 12), 12);
  if (sizeof(Residual) == 2) {
    // If the first argument to RightShiftWithRounding(..., 12) is only
    // slightly smaller than 2^27 - 1 (e.g., 0x7fffe4e), adding 2^11 to it
    // in RightShiftWithRounding(..., 12) will cause the function to return
    // 0x8000, which cannot be represented as an int16_t. Change it to 0x7fff.
    dst_0 -= (dst_0 == 0x8000);
    dst_1 -= (dst_1 == 0x8000);
    dst_3 -= (dst_3 == 0x8000);
  }
  dst[0] = dst_0;
  dst[1] = dst_1;
  dst[2] = dst_2;
  dst[3] = dst_3;

  const int size = 4;
  if (is_row && row_shift > 0) {
    for (int j = 0; j < size; ++j) {
      dst[j] = RightShiftWithRounding(dst[j], row_shift);
    }
  }

  ClampIntermediate<bitdepth, Residual>(dst, 4);
}

template <typename Residual>
void AdstInputPermutation(int32_t* LIBGAV1_RESTRICT const dst,
                          const Residual* LIBGAV1_RESTRICT const src, int n) {
  assert(n == 8 || n == 16);
  for (int i = 0; i < n; ++i) {
    dst[i] = src[((i & 1) == 0) ? n - i - 1 : i - 1];
  }
}

constexpr int8_t kAdstOutputPermutationLookup[16] = {
    0, 8, 12, 4, 6, 14, 10, 2, 3, 11, 15, 7, 5, 13, 9, 1};

template <typename Residual>
void AdstOutputPermutation(Residual* LIBGAV1_RESTRICT const dst,
                           const int32_t* LIBGAV1_RESTRICT const src, int n) {
  assert(n == 8 || n == 16);
  const auto shift = static_cast<int8_t>(n == 8);
  for (int i = 0; i < n; ++i) {
    const int8_t index = kAdstOutputPermutationLookup[i] >> shift;
    int32_t dst_i = ((i & 1) == 0) ? src[index] : -src[index];
    if (sizeof(Residual) == 2) {
      // If i is odd and src[index] is -32768, dst_i will be 32768, which
      // cannot be represented as an int16_t.
      dst_i -= (dst_i == 0x8000);
    }
    dst[i] = dst_i;
  }
}

template <typename Residual>
void Adst8_C(void* dest, int8_t range) {
  auto* const dst = static_cast<Residual*>(dest);
  // stage 1.
  int32_t temp[8];
  AdstInputPermutation(temp, dst, 8);
  // stage 2.
  for (int i = 0; i < 4; ++i) {
    ButterflyRotation_C(temp, MultiplyBy2(i), MultiplyBy2(i) + 1, 60 - 16 * i,
                        true, range);
  }
  // stage 3.
  for (int i = 0; i < 4; ++i) {
    HadamardRotation_C(temp, i, i + 4, false, range);
  }
  // stage 4.
  for (int i = 0; i < 2; ++i) {
    ButterflyRotation_C(temp, i * 3 + 4, i + 5, 48 - 32 * i, true, range);
  }
  // stage 5.
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      HadamardRotation_C(temp, i + MultiplyBy4(j), i + MultiplyBy4(j) + 2,
                         false, range);
    }
  }
  // stage 6.
  for (int i = 0; i < 2; ++i) {
    ButterflyRotation_C(temp, MultiplyBy4(i) + 2, MultiplyBy4(i) + 3, 32, true,
                        range);
  }
  // stage 7.
  AdstOutputPermutation(dst, temp, 8);
}

template <int bitdepth, typename Residual>
void Adst8DcOnly_C(void* dest, int8_t range, bool should_round, int row_shift,
                   bool is_row) {
  auto* const dst = static_cast<Residual*>(dest);

  // stage 1.
  int32_t temp[8];
  // After the permutation, the dc value is in temp[1]. The remaining are zero.
  AdstInputPermutation(temp, dst, 8);

  if (is_row && should_round) {
    temp[1] = RightShiftWithRounding(temp[1] * kTransformRowMultiplier, 12);
  }

  // stage 2.
  ButterflyRotationFirstIsZero_C(temp, 0, 1, 60, true, range);

  // stage 3.
  temp[4] = temp[0];
  temp[5] = temp[1];

  // stage 4.
  ButterflyRotation_C(temp, 4, 5, 48, true, range);

  // stage 5.
  temp[2] = temp[0];
  temp[3] = temp[1];
  temp[6] = temp[4];
  temp[7] = temp[5];

  // stage 6.
  ButterflyRotation_C(temp, 2, 3, 32, true, range);
  ButterflyRotation_C(temp, 6, 7, 32, true, range);

  // stage 7.
  AdstOutputPermutation(dst, temp, 8);

  const int size = 8;
  if (is_row && row_shift > 0) {
    for (int j = 0; j < size; ++j) {
      dst[j] = RightShiftWithRounding(dst[j], row_shift);
    }
  }

  ClampIntermediate<bitdepth, Residual>(dst, 8);
}

template <typename Residual>
void Adst16_C(void* dest, int8_t range) {
  auto* const dst = static_cast<Residual*>(dest);
  // stage 1.
  int32_t temp[16];
  AdstInputPermutation(temp, dst, 16);
  // stage 2.
  for (int i = 0; i < 8; ++i) {
    ButterflyRotation_C(temp, MultiplyBy2(i), MultiplyBy2(i) + 1, 62 - 8 * i,
                        true, range);
  }
  // stage 3.
  for (int i = 0; i < 8; ++i) {
    HadamardRotation_C(temp, i, i + 8, false, range);
  }
  // stage 4.
  for (int i = 0; i < 2; ++i) {
    ButterflyRotation_C(temp, MultiplyBy2(i) + 8, MultiplyBy2(i) + 9,
                        56 - 32 * i, true, range);
    ButterflyRotation_C(temp, MultiplyBy2(i) + 13, MultiplyBy2(i) + 12,
                        8 + 32 * i, true, range);
  }
  // stage 5.
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      HadamardRotation_C(temp, i + MultiplyBy8(j), i + MultiplyBy8(j) + 4,
                         false, range);
    }
  }
  // stage 6.
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      ButterflyRotation_C(temp, i * 3 + MultiplyBy8(j) + 4,
                          i + MultiplyBy8(j) + 5, 48 - 32 * i, true, range);
    }
  }
  // stage 7.
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 4; ++j) {
      HadamardRotation_C(temp, i + MultiplyBy4(j), i + MultiplyBy4(j) + 2,
                         false, range);
    }
  }
  // stage 8.
  for (int i = 0; i < 4; ++i) {
    ButterflyRotation_C(temp, MultiplyBy4(i) + 2, MultiplyBy4(i) + 3, 32, true,
                        range);
  }
  // stage 9.
  AdstOutputPermutation(dst, temp, 16);
}

template <int bitdepth, typename Residual>
void Adst16DcOnly_C(void* dest, int8_t range, bool should_round, int row_shift,
                    bool is_row) {
  auto* const dst = static_cast<Residual*>(dest);

  // stage 1.
  int32_t temp[16];
  // After the permutation, the dc value is in temp[1].  The remaining are zero.
  AdstInputPermutation(temp, dst, 16);

  if (is_row && should_round) {
    temp[1] = RightShiftWithRounding(temp[1] * kTransformRowMultiplier, 12);
  }

  // stage 2.
  ButterflyRotationFirstIsZero_C(temp, 0, 1, 62, true, range);

  // stage 3.
  temp[8] = temp[0];
  temp[9] = temp[1];

  // stage 4.
  ButterflyRotation_C(temp, 8, 9, 56, true, range);

  // stage 5.
  temp[4] = temp[0];
  temp[5] = temp[1];
  temp[12] = temp[8];
  temp[13] = temp[9];

  // stage 6.
  ButterflyRotation_C(temp, 4, 5, 48, true, range);
  ButterflyRotation_C(temp, 12, 13, 48, true, range);

  // stage 7.
  temp[2] = temp[0];
  temp[3] = temp[1];
  temp[10] = temp[8];
  temp[11] = temp[9];

  temp[6] = temp[4];
  temp[7] = temp[5];
  temp[14] = temp[12];
  temp[15] = temp[13];

  // stage 8.
  for (int i = 0; i < 4; ++i) {
    ButterflyRotation_C(temp, MultiplyBy4(i) + 2, MultiplyBy4(i) + 3, 32, true,
                        range);
  }

  // stage 9.
  AdstOutputPermutation(dst, temp, 16);

  const int size = 16;
  if (is_row && row_shift > 0) {
    for (int j = 0; j < size; ++j) {
      dst[j] = RightShiftWithRounding(dst[j], row_shift);
    }
  }

  ClampIntermediate<bitdepth, Residual>(dst, 16);
}

//------------------------------------------------------------------------------
// Identity Transforms.
//
// In the spec, the inverse identity transform is followed by a Round2() call:
//   The row transforms with i = 0..(h-1) are applied as follows:
//     ...
//     * Otherwise, invoke the inverse identity transform process specified in
//       section 7.13.2.15 with the input variable n equal to log2W.
//     * Set Residual[ i ][ j ] equal to Round2( T[ j ], rowShift )
//       for j = 0..(w-1).
//   ...
//   The column transforms with j = 0..(w-1) are applied as follows:
//     ...
//     * Otherwise, invoke the inverse identity transform process specified in
//       section 7.13.2.15 with the input variable n equal to log2H.
//     * Residual[ i ][ j ] is set equal to Round2( T[ i ], colShift )
//       for i = 0..(h-1).
//
// Therefore, we define the identity transform functions to perform both the
// inverse identity transform and the Round2() call. This has two advantages:
// 1. The outputs of the inverse identity transform do not need to be stored
//    in the Residual array. They can be stored in int32_t local variables,
//    which have a larger range if Residual is an int16_t array.
// 2. The inverse identity transform and the Round2() call can be jointly
//    optimized.
//
// The identity transform functions have the following prototype:
//   void Identity_C(void* dest, int8_t shift);
//
// The |shift| parameter is the amount of shift for the Round2() call. For row
// transforms, |shift| is 0, 1, or 2. For column transforms, |shift| is always
// 4. Therefore, an identity transform function can detect whether it is being
// invoked as a row transform or a column transform by checking whether |shift|
// is equal to 4.
//
// Input Range
//
// The inputs of row transforms, stored in the 2D array Dequant, are
// representable by a signed integer using 8 + BitDepth bits of precision:
//   f. Dequant[ i ][ j ] is set equal to
//   Clip3( - ( 1 << ( 7 + BitDepth ) ), ( 1 << ( 7 + BitDepth ) ) - 1, dq2 ).
//
// The inputs of column transforms are representable by a signed integer using
// Max( BitDepth + 6, 16 ) bits of precision:
//   Set the variable colClampRange equal to Max( BitDepth + 6, 16 ).
//   ...
//   Between the row and column transforms, Residual[ i ][ j ] is set equal to
//   Clip3( - ( 1 << ( colClampRange - 1 ) ),
//          ( 1 << (colClampRange - 1 ) ) - 1,
//          Residual[ i ][ j ] )
//   for i = 0..(h-1), for j = 0..(w-1).
//
// Output Range
//
// The outputs of row transforms are representable by a signed integer using
// 8 + BitDepth + 1 = 9 + BitDepth bits of precision, because the net effect
// of the multiplicative factor of inverse identity transforms minus the
// smallest row shift is an increase of at most one bit.
//
// Transform | Multiplicative factor | Smallest row | Net increase
// width     | (in bits)             | shift        | in bits
// ---------------------------------------------------------------
//     4     |  sqrt(2)  (0.5 bits)  |      0       |    +0.5
//     8     |     2     (1 bit)     |      0       |    +1
//    16     | 2*sqrt(2) (1.5 bits)  |      1       |    +0.5
//    32     |     4     (2 bits)    |      1       |    +1
//
// If BitDepth is 8 and Residual is an int16_t array, to avoid truncation we
// clip the outputs (which have 17 bits of precision) to the range of int16_t
// before storing them in the Residual array. This clipping happens to be the
// same as the required clipping after the row transform (see the spec quoted
// above), so we remain compliant with the spec. (In this case,
// TransformLoop_C() skips clipping the outputs of row transforms to avoid
// duplication of effort.)
//
// The outputs of column transforms are representable by a signed integer using
// Max( BitDepth + 6, 16 ) + 2 - 4 = Max( BitDepth + 4, 14 ) bits of precision,
// because the multiplicative factor of inverse identity transforms is at most
// 4 (2 bits) and |shift| is always 4.

template <typename Residual>
void Identity4Row_C(void* dest, int8_t shift) {
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  assert(shift == 0 || shift == 1);
  auto* const dst = static_cast<Residual*>(dest);
  // If |shift| is 0, |rounding| should be 1 << 11. If |shift| is 1, |rounding|
  // should be (1 + (1 << 1)) << 11. The following expression works for both
  // values of |shift|.
  const int32_t rounding = (1 + (shift << 1)) << 11;
  for (int i = 0; i < 4; ++i) {
    const auto intermediate =
        static_cast<Intermediate>(dst[i]) * kIdentity4Multiplier;
    int32_t dst_i =
        static_cast<int32_t>((intermediate + rounding) >> (12 + shift));
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[i] = static_cast<Residual>(dst_i);
  }
}

template <typename Residual>
void Identity4Column_C(void* dest, int8_t /*shift*/) {
  auto* const dst = static_cast<Residual*>(dest);
  const int32_t rounding = (1 + (1 << kTransformColumnShift)) << 11;
  for (int i = 0; i < 4; ++i) {
    // The intermediate value here will have to fit into an int32_t for it to be
    // bitstream conformant. The multiplication is promoted to int32_t by
    // defining kIdentity4Multiplier as int32_t.
    dst[i] = static_cast<Residual>((dst[i] * kIdentity4Multiplier + rounding) >>
                                   (12 + kTransformColumnShift));
  }
}

template <int bitdepth, typename Residual>
void Identity4DcOnly_C(void* dest, int8_t /*range*/, bool should_round,
                       int row_shift, bool is_row) {
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row) {
    if (should_round) {
      const auto intermediate =
          static_cast<Intermediate>(dst[0]) * kTransformRowMultiplier;
      dst[0] = RightShiftWithRounding(intermediate, 12);
    }

    const int32_t rounding = (1 + (row_shift << 1)) << 11;
    const auto intermediate =
        static_cast<Intermediate>(dst[0]) * kIdentity4Multiplier;
    int32_t dst_i =
        static_cast<int32_t>((intermediate + rounding) >> (12 + row_shift));
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[0] = static_cast<Residual>(dst_i);

    ClampIntermediate<bitdepth, Residual>(dst, 1);
    return;
  }

  const int32_t rounding = (1 + (1 << kTransformColumnShift)) << 11;
  dst[0] = static_cast<Residual>((dst[0] * kIdentity4Multiplier + rounding) >>
                                 (12 + kTransformColumnShift));
}

template <typename Residual>
void Identity8Row_C(void* dest, int8_t shift) {
  assert(shift == 0 || shift == 1 || shift == 2);
  auto* const dst = static_cast<Residual*>(dest);
  for (int i = 0; i < 8; ++i) {
    int32_t dst_i = RightShiftWithRounding(MultiplyBy2(dst[i]), shift);
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[i] = static_cast<Residual>(dst_i);
  }
}

template <typename Residual>
void Identity8Column_C(void* dest, int8_t /*shift*/) {
  auto* const dst = static_cast<Residual*>(dest);
  for (int i = 0; i < 8; ++i) {
    dst[i] = static_cast<Residual>(
        RightShiftWithRounding(dst[i], kTransformColumnShift - 1));
  }
}

template <int bitdepth, typename Residual>
void Identity8DcOnly_C(void* dest, int8_t /*range*/, bool should_round,
                       int row_shift, bool is_row) {
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row) {
    if (should_round) {
      const auto intermediate =
          static_cast<Intermediate>(dst[0]) * kTransformRowMultiplier;
      dst[0] = RightShiftWithRounding(intermediate, 12);
    }

    int32_t dst_i = RightShiftWithRounding(MultiplyBy2(dst[0]), row_shift);
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[0] = static_cast<Residual>(dst_i);

    // If Residual is int16_t (which implies bitdepth is 8), we don't need to
    // clip residual[i][j] to 16 bits.
    if (sizeof(Residual) > 2) {
      const Residual intermediate_clamp_max =
          (1 << (std::max(bitdepth + 6, 16) - 1)) - 1;
      const Residual intermediate_clamp_min = -intermediate_clamp_max - 1;
      dst[0] = Clip3(dst[0], intermediate_clamp_min, intermediate_clamp_max);
    }
    return;
  }

  dst[0] = static_cast<Residual>(
      RightShiftWithRounding(dst[0], kTransformColumnShift - 1));
}

template <typename Residual>
void Identity16Row_C(void* dest, int8_t shift) {
  assert(shift == 1 || shift == 2);
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  auto* const dst = static_cast<Residual*>(dest);
  const int32_t rounding = (1 + (1 << shift)) << 11;
  for (int i = 0; i < 16; ++i) {
    // Note the intermediate value can only exceed 32 bits with 12-bit content.
    // For simplicity in unoptimized code, int64_t is used for all cases.
    const auto intermediate =
        static_cast<Intermediate>(dst[i]) * kIdentity16Multiplier;
    int32_t dst_i =
        static_cast<int32_t>((intermediate + rounding) >> (12 + shift));
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[i] = static_cast<Residual>(dst_i);
  }
}

template <typename Residual>
void Identity16Column_C(void* dest, int8_t /*shift*/) {
  auto* const dst = static_cast<Residual*>(dest);
  const int32_t rounding = (1 + (1 << kTransformColumnShift)) << 11;
  for (int i = 0; i < 16; ++i) {
    // The intermediate value here will have to fit into an int32_t for it to be
    // bitstream conformant. The multiplication is promoted to int32_t by
    // defining kIdentity16Multiplier as int32_t.
    dst[i] =
        static_cast<Residual>((dst[i] * kIdentity16Multiplier + rounding) >>
                              (12 + kTransformColumnShift));
  }
}

template <int bitdepth, typename Residual>
void Identity16DcOnly_C(void* dest, int8_t /*range*/, bool should_round,
                        int row_shift, bool is_row) {
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row) {
    if (should_round) {
      const auto intermediate =
          static_cast<Intermediate>(dst[0]) * kTransformRowMultiplier;
      dst[0] = RightShiftWithRounding(intermediate, 12);
    }

    const int32_t rounding = (1 + (1 << row_shift)) << 11;
    const auto intermediate =
        static_cast<Intermediate>(dst[0]) * kIdentity16Multiplier;
    int32_t dst_i =
        static_cast<int32_t>((intermediate + rounding) >> (12 + row_shift));
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[0] = static_cast<Residual>(dst_i);

    ClampIntermediate<bitdepth, Residual>(dst, 1);
    return;
  }

  const int32_t rounding = (1 + (1 << kTransformColumnShift)) << 11;
  dst[0] = static_cast<Residual>((dst[0] * kIdentity16Multiplier + rounding) >>
                                 (12 + kTransformColumnShift));
}

template <typename Residual>
void Identity32Row_C(void* dest, int8_t shift) {
  assert(shift == 1 || shift == 2);
  auto* const dst = static_cast<Residual*>(dest);
  for (int i = 0; i < 32; ++i) {
    int32_t dst_i = RightShiftWithRounding(MultiplyBy4(dst[i]), shift);
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[i] = static_cast<Residual>(dst_i);
  }
}

template <typename Residual>
void Identity32Column_C(void* dest, int8_t /*shift*/) {
  auto* const dst = static_cast<Residual*>(dest);
  for (int i = 0; i < 32; ++i) {
    dst[i] = static_cast<Residual>(
        RightShiftWithRounding(dst[i], kTransformColumnShift - 2));
  }
}

template <int bitdepth, typename Residual>
void Identity32DcOnly_C(void* dest, int8_t /*range*/, bool should_round,
                        int row_shift, bool is_row) {
  // Note the intermediate value can only exceed 32 bits with 12-bit content.
  // For simplicity in unoptimized code, int64_t is used for both 10 & 12-bit.
  using Intermediate =
      typename std::conditional<sizeof(Residual) == 2, int32_t, int64_t>::type;
  auto* const dst = static_cast<Residual*>(dest);

  if (is_row) {
    if (should_round) {
      const auto intermediate =
          static_cast<Intermediate>(dst[0]) * kTransformRowMultiplier;
      dst[0] = RightShiftWithRounding(intermediate, 12);
    }

    int32_t dst_i = RightShiftWithRounding(MultiplyBy4(dst[0]), row_shift);
    if (sizeof(Residual) == 2) {
      dst_i = Clip3(dst_i, INT16_MIN, INT16_MAX);
    }
    dst[0] = static_cast<Residual>(dst_i);

    ClampIntermediate<bitdepth, Residual>(dst, 1);
    return;
  }

  dst[0] = static_cast<Residual>(
      RightShiftWithRounding(dst[0], kTransformColumnShift - 2));
}

//------------------------------------------------------------------------------
// Walsh Hadamard Transform.

template <typename Residual>
void Wht4_C(void* dest, int8_t shift) {
  auto* const dst = static_cast<Residual*>(dest);
  Residual temp[4];
  temp[0] = dst[0] >> shift;
  temp[2] = dst[1] >> shift;
  temp[3] = dst[2] >> shift;
  temp[1] = dst[3] >> shift;
  temp[0] += temp[2];
  temp[3] -= temp[1];
  // This signed right shift must be an arithmetic shift.
  Residual e = (temp[0] - temp[3]) >> 1;
  dst[1] = e - temp[1];
  dst[2] = e - temp[2];
  dst[0] = temp[0] - dst[1];
  dst[3] = temp[3] + dst[2];
}

template <int bitdepth, typename Residual>
void Wht4DcOnly_C(void* dest, int8_t range, bool /*should_round*/,
                  int /*row_shift*/, bool /*is_row*/) {
  auto* const dst = static_cast<Residual*>(dest);
  const int shift = range;

  Residual temp = dst[0] >> shift;
  // This signed right shift must be an arithmetic shift.
  Residual e = temp >> 1;
  dst[0] = temp - e;
  dst[1] = e;
  dst[2] = e;
  dst[3] = e;

  ClampIntermediate<bitdepth, Residual>(dst, 4);
}

//------------------------------------------------------------------------------
// row/column transform loop

using InverseTransform1dFunc = void (*)(void* dst, int8_t range);
using InverseTransformDcOnlyFunc = void (*)(void* dest, int8_t range,
                                            bool should_round, int row_shift,
                                            bool is_row);

template <int bitdepth, typename Residual, typename Pixel,
          Transform1d transform1d_type,
          InverseTransformDcOnlyFunc dconly_transform1d,
          InverseTransform1dFunc transform1d_func, bool is_row>
void TransformLoop_C(TransformType tx_type, TransformSize tx_size,
                     int adjusted_tx_height, void* LIBGAV1_RESTRICT src_buffer,
                     int start_x, int start_y,
                     void* LIBGAV1_RESTRICT dst_frame) {
  constexpr bool lossless = transform1d_type == kTransform1dWht;
  constexpr bool is_identity = transform1d_type == kTransform1dIdentity;
  // The transform size of the WHT is always 4x4. Setting tx_width and
  // tx_height to the constant 4 for the WHT speeds the code up.
  assert(!lossless || tx_size == kTransformSize4x4);
  const int tx_width = lossless ? 4 : kTransformWidth[tx_size];
  const int tx_height = lossless ? 4 : kTransformHeight[tx_size];
  const int tx_width_log2 = kTransformWidthLog2[tx_size];
  const int tx_height_log2 = kTransformHeightLog2[tx_size];
  auto* frame = static_cast<Array2DView<Pixel>*>(dst_frame);

  // Initially this points to the dequantized values. After the transforms are
  // applied, this buffer contains the residual.
  Array2DView<Residual> residual(tx_height, tx_width,
                                 static_cast<Residual*>(src_buffer));

  if (is_row) {
    // Row transform.
    const uint8_t row_shift = lossless ? 0 : kTransformRowShift[tx_size];
    // This is the |range| parameter of the InverseTransform1dFunc.  For lossy
    // transforms, this will be equal to the clamping range.
    const int8_t row_clamp_range = lossless ? 2 : (bitdepth + 8);
    // If the width:height ratio of the transform size is 2:1 or 1:2, multiply
    // the input to the row transform by 1 / sqrt(2), which is approximated by
    // the fraction 2896 / 2^12.
    const bool should_round = std::abs(tx_width_log2 - tx_height_log2) == 1;

    if (adjusted_tx_height == 1) {
      dconly_transform1d(residual[0], row_clamp_range, should_round, row_shift,
                         true);
      return;
    }

    // Row transforms need to be done only up to 32 because the rest of the rows
    // are always all zero if |tx_height| is 64.  Otherwise, only process the
    // rows that have a non zero coefficients.
    for (int i = 0; i < adjusted_tx_height; ++i) {
      // If lossless, the transform size is 4x4, so should_round is false.
      if (!lossless && should_round) {
        // The last 32 values of every row are always zero if the |tx_width| is
        // 64.
        for (int j = 0; j < std::min(tx_width, 32); ++j) {
          residual[i][j] = RightShiftWithRounding(
              residual[i][j] * kTransformRowMultiplier, 12);
        }
      }
      // For identity transform, |transform1d_func| also performs the
      // Round2(T[j], rowShift) call in the spec.
      transform1d_func(residual[i], is_identity ? row_shift : row_clamp_range);
      if (!lossless && !is_identity && row_shift > 0) {
        for (int j = 0; j < tx_width; ++j) {
          residual[i][j] = RightShiftWithRounding(residual[i][j], row_shift);
        }
      }

      ClampIntermediate<bitdepth, Residual>(residual[i], tx_width);
    }
    return;
  }

  assert(!is_row);
  constexpr uint8_t column_shift = lossless ? 0 : kTransformColumnShift;
  // This is the |range| parameter of the InverseTransform1dFunc.  For lossy
  // transforms, this will be equal to the clamping range.
  const int8_t column_clamp_range = lossless ? 0 : std::max(bitdepth + 6, 16);
  const bool flip_rows = transform1d_type == kTransform1dAdst &&
                         kTransformFlipRowsMask.Contains(tx_type);
  const bool flip_columns =
      !lossless && kTransformFlipColumnsMask.Contains(tx_type);
  const int min_value = 0;
  const int max_value = (1 << bitdepth) - 1;
  // Note: 64 is the maximum size of a 1D transform buffer (the largest
  // transform size is kTransformSize64x64).
  Residual tx_buffer[64];
  for (int j = 0; j < tx_width; ++j) {
    const int flipped_j = flip_columns ? tx_width - j - 1 : j;
    int i = 0;
    do {
      tx_buffer[i] = residual[i][flipped_j];
    } while (++i != tx_height);
    if (adjusted_tx_height == 1) {
      dconly_transform1d(tx_buffer, column_clamp_range, false, 0, false);
    } else {
      // For identity transform, |transform1d_func| also performs the
      // Round2(T[i], colShift) call in the spec.
      transform1d_func(tx_buffer,
                       is_identity ? column_shift : column_clamp_range);
    }
    const int x = start_x + j;
    for (int i = 0; i < tx_height; ++i) {
      const int y = start_y + i;
      const int index = flip_rows ? tx_height - i - 1 : i;
      Residual residual_value = tx_buffer[index];
      if (!lossless && !is_identity) {
        residual_value = RightShiftWithRounding(residual_value, column_shift);
      }
      (*frame)[y][x] =
          Clip3((*frame)[y][x] + residual_value, min_value, max_value);
    }
  }
}

//------------------------------------------------------------------------------

#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
template <int bitdepth, typename Residual, typename Pixel>
void InitAll(Dsp* const dsp) {
  // Maximum transform size for Dct is 64.
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 2>, Dct_C<Residual, 2>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 2>, Dct_C<Residual, 2>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 3>, Dct_C<Residual, 3>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 3>, Dct_C<Residual, 3>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 4>, Dct_C<Residual, 4>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 4>, Dct_C<Residual, 4>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 5>, Dct_C<Residual, 5>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 5>, Dct_C<Residual, 5>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 6>, Dct_C<Residual, 6>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dDct,
                      DctDcOnly_C<bitdepth, Residual, 6>, Dct_C<Residual, 6>,
                      /*is_row=*/false>;

  // Maximum transform size for Adst is 16.
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst4DcOnly_C<bitdepth, Residual>, Adst4_C<Residual>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst4DcOnly_C<bitdepth, Residual>, Adst4_C<Residual>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst8DcOnly_C<bitdepth, Residual>, Adst8_C<Residual>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst8DcOnly_C<bitdepth, Residual>, Adst8_C<Residual>,
                      /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst16DcOnly_C<bitdepth, Residual>, Adst16_C<Residual>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dAdst,
                      Adst16DcOnly_C<bitdepth, Residual>, Adst16_C<Residual>,
                      /*is_row=*/false>;

  // Maximum transform size for Identity transform is 32.
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity4DcOnly_C<bitdepth, Residual>,
                      Identity4Row_C<Residual>, /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity4DcOnly_C<bitdepth, Residual>,
                      Identity4Column_C<Residual>, /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity8DcOnly_C<bitdepth, Residual>,
                      Identity8Row_C<Residual>, /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity8DcOnly_C<bitdepth, Residual>,
                      Identity8Column_C<Residual>, /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity16DcOnly_C<bitdepth, Residual>,
                      Identity16Row_C<Residual>, /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity16DcOnly_C<bitdepth, Residual>,
                      Identity16Column_C<Residual>, /*is_row=*/false>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity32DcOnly_C<bitdepth, Residual>,
                      Identity32Row_C<Residual>, /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dIdentity,
                      Identity32DcOnly_C<bitdepth, Residual>,
                      Identity32Column_C<Residual>, /*is_row=*/false>;

  // Maximum transform size for Wht is 4.
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dWht,
                      Wht4DcOnly_C<bitdepth, Residual>, Wht4_C<Residual>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      TransformLoop_C<bitdepth, Residual, Pixel, kTransform1dWht,
                      Wht4DcOnly_C<bitdepth, Residual>, Wht4_C<Residual>,
                      /*is_row=*/false>;
}
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS

void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(8);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  InitAll<8, int16_t, uint8_t>(dsp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize4_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 2>, Dct_C<int16_t, 2>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 2>, Dct_C<int16_t, 2>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize8_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 3>, Dct_C<int16_t, 3>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 3>, Dct_C<int16_t, 3>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize16_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 4>, Dct_C<int16_t, 4>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 4>, Dct_C<int16_t, 4>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize32_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 5>, Dct_C<int16_t, 5>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 5>, Dct_C<int16_t, 5>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize64_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 6>, Dct_C<int16_t, 6>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dDct,
                      DctDcOnly_C<8, int16_t, 6>, Dct_C<int16_t, 6>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize4_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst4DcOnly_C<8, int16_t>, Adst4_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst4DcOnly_C<8, int16_t>, Adst4_C<int16_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize8_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst8DcOnly_C<8, int16_t>, Adst8_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst8DcOnly_C<8, int16_t>, Adst8_C<int16_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize16_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst16DcOnly_C<8, int16_t>, Adst16_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dAdst,
                      Adst16DcOnly_C<8, int16_t>, Adst16_C<int16_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize4_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity4DcOnly_C<8, int16_t>, Identity4Row_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity4DcOnly_C<8, int16_t>, Identity4Column_C<int16_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize8_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity8DcOnly_C<8, int16_t>, Identity8Row_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity8DcOnly_C<8, int16_t>, Identity8Column_C<int16_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize16_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity16DcOnly_C<8, int16_t>, Identity16Row_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity16DcOnly_C<8, int16_t>,
                      Identity16Column_C<int16_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize32_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity32DcOnly_C<8, int16_t>, Identity32Row_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dIdentity,
                      Identity32DcOnly_C<8, int16_t>,
                      Identity32Column_C<int16_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp8bpp_Transform1dSize4_Transform1dWht
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dWht,
                      Wht4DcOnly_C<8, int16_t>, Wht4_C<int16_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      TransformLoop_C<8, int16_t, uint8_t, kTransform1dWht,
                      Wht4DcOnly_C<8, int16_t>, Wht4_C<int16_t>,
                      /*is_row=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}

#if LIBGAV1_MAX_BITDEPTH >= 10
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(10);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  InitAll<10, int32_t, uint16_t>(dsp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize4_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 2>, Dct_C<int32_t, 2>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 2>, Dct_C<int32_t, 2>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize8_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 3>, Dct_C<int32_t, 3>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 3>, Dct_C<int32_t, 3>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize16_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 4>, Dct_C<int32_t, 4>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 4>, Dct_C<int32_t, 4>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize32_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 5>, Dct_C<int32_t, 5>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 5>, Dct_C<int32_t, 5>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize64_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 6>, Dct_C<int32_t, 6>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<10, int32_t, 6>, Dct_C<int32_t, 6>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize4_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst4DcOnly_C<10, int32_t>, Adst4_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst4DcOnly_C<10, int32_t>, Adst4_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize8_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst8DcOnly_C<10, int32_t>, Adst8_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst8DcOnly_C<10, int32_t>, Adst8_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize16_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst16DcOnly_C<10, int32_t>, Adst16_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dAdst,
                      Adst16DcOnly_C<10, int32_t>, Adst16_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize4_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity4DcOnly_C<10, int32_t>, Identity4Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity4DcOnly_C<10, int32_t>,
                      Identity4Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize8_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity8DcOnly_C<10, int32_t>, Identity8Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity8DcOnly_C<10, int32_t>,
                      Identity8Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize16_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity16DcOnly_C<10, int32_t>, Identity16Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity16DcOnly_C<10, int32_t>,
                      Identity16Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize32_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity32DcOnly_C<10, int32_t>, Identity32Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dIdentity,
                      Identity32DcOnly_C<10, int32_t>,
                      Identity32Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp10bpp_Transform1dSize4_Transform1dWht
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dWht,
                      Wht4DcOnly_C<10, int32_t>, Wht4_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      TransformLoop_C<10, int32_t, uint16_t, kTransform1dWht,
                      Wht4DcOnly_C<10, int32_t>, Wht4_C<int32_t>,
                      /*is_row=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
void Init12bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(12);
  assert(dsp != nullptr);
  static_cast<void>(dsp);
#if LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
  InitAll<12, int32_t, uint16_t>(dsp);
#else  // !LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize4_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 2>, Dct_C<int32_t, 2>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize4][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 2>, Dct_C<int32_t, 2>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize8_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 3>, Dct_C<int32_t, 3>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize8][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 3>, Dct_C<int32_t, 3>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize16_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 4>, Dct_C<int32_t, 4>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize16][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 4>, Dct_C<int32_t, 4>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize32_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 5>, Dct_C<int32_t, 5>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize32][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 5>, Dct_C<int32_t, 5>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize64_Transform1dDct
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 6>, Dct_C<int32_t, 6>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dDct][kTransform1dSize64][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dDct,
                      DctDcOnly_C<12, int32_t, 6>, Dct_C<int32_t, 6>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize4_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst4DcOnly_C<12, int32_t>, Adst4_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize4][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst4DcOnly_C<12, int32_t>, Adst4_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize8_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst8DcOnly_C<12, int32_t>, Adst8_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize8][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst8DcOnly_C<12, int32_t>, Adst8_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize16_Transform1dAdst
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst16DcOnly_C<12, int32_t>, Adst16_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dAdst][kTransform1dSize16][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dAdst,
                      Adst16DcOnly_C<12, int32_t>, Adst16_C<int32_t>,
                      /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize4_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity4DcOnly_C<12, int32_t>, Identity4Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize4][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity4DcOnly_C<12, int32_t>,
                      Identity4Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize8_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity8DcOnly_C<12, int32_t>, Identity8Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize8][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity8DcOnly_C<12, int32_t>,
                      Identity8Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize16_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity16DcOnly_C<12, int32_t>, Identity16Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize16][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity16DcOnly_C<12, int32_t>,
                      Identity16Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize32_Transform1dIdentity
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity32DcOnly_C<12, int32_t>, Identity32Row_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dIdentity][kTransform1dSize32][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dIdentity,
                      Identity32DcOnly_C<12, int32_t>,
                      Identity32Column_C<int32_t>, /*is_row=*/false>;
#endif
#ifndef LIBGAV1_Dsp12bpp_Transform1dSize4_Transform1dWht
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kRow] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dWht,
                      Wht4DcOnly_C<12, int32_t>, Wht4_C<int32_t>,
                      /*is_row=*/true>;
  dsp->inverse_transforms[kTransform1dWht][kTransform1dSize4][kColumn] =
      TransformLoop_C<12, int32_t, uint16_t, kTransform1dWht,
                      Wht4DcOnly_C<12, int32_t>, Wht4_C<int32_t>,
                      /*is_row=*/false>;
#endif
#endif  // LIBGAV1_ENABLE_ALL_DSP_FUNCTIONS
}
#endif  // LIBGAV1_MAX_BITDEPTH == 12

}  // namespace

void InverseTransformInit_C() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  Init10bpp();
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
  Init12bpp();
#endif

  // Local functions that may be unused depending on the optimizations
  // available.
  static_cast<void>(kBitReverseLookup);
}

}  // namespace dsp
}  // namespace libgav1
