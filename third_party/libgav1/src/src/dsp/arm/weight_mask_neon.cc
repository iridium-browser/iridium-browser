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

#include "src/dsp/arm/weight_mask_neon.h"

#include "src/dsp/weight_mask.h"
#include "src/utils/cpu.h"

#if LIBGAV1_ENABLE_NEON

#include <arm_neon.h>

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "src/dsp/arm/common_neon.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

inline int16x8x2_t LoadPred(const int16_t* LIBGAV1_RESTRICT prediction_0,
                            const int16_t* LIBGAV1_RESTRICT prediction_1) {
  const int16x8x2_t pred = {vld1q_s16(prediction_0), vld1q_s16(prediction_1)};
  return pred;
}

#if LIBGAV1_MAX_BITDEPTH >= 10
inline uint16x8x2_t LoadPred(const uint16_t* LIBGAV1_RESTRICT prediction_0,
                             const uint16_t* LIBGAV1_RESTRICT prediction_1) {
  const uint16x8x2_t pred = {vld1q_u16(prediction_0), vld1q_u16(prediction_1)};
  return pred;
}
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

template <int bitdepth>
inline uint16x8_t AbsolutePredDifference(const int16x8x2_t pred) {
  static_assert(bitdepth == 8, "");
  constexpr int rounding_bits = bitdepth - 8 + ((bitdepth == 12) ? 2 : 4);
  return vrshrq_n_u16(
      vreinterpretq_u16_s16(vabdq_s16(pred.val[0], pred.val[1])),
      rounding_bits);
}

template <int bitdepth>
inline uint16x8_t AbsolutePredDifference(const uint16x8x2_t pred) {
  constexpr int rounding_bits = bitdepth - 8 + ((bitdepth == 12) ? 2 : 4);
  return vrshrq_n_u16(vabdq_u16(pred.val[0], pred.val[1]), rounding_bits);
}

template <bool mask_is_inverse, int bitdepth>
inline void WeightMask8_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                             const void* LIBGAV1_RESTRICT prediction_1,
                             uint8_t* LIBGAV1_RESTRICT mask) {
  using PredType =
      typename std::conditional<bitdepth == 8, int16_t, uint16_t>::type;
  using PredTypeVecx2 =
      typename std::conditional<bitdepth == 8, int16x8x2_t, uint16x8x2_t>::type;
  const PredTypeVecx2 pred =
      LoadPred(static_cast<const PredType*>(prediction_0),
               static_cast<const PredType*>(prediction_1));
  const uint16x8_t difference = AbsolutePredDifference<bitdepth>(pred);
  const uint8x8_t difference_offset = vdup_n_u8(38);
  const uint8x8_t mask_ceiling = vdup_n_u8(64);
  const uint8x8_t adjusted_difference =
      vqadd_u8(vqshrn_n_u16(difference, 4), difference_offset);
  const uint8x8_t mask_value = vmin_u8(adjusted_difference, mask_ceiling);
  if (mask_is_inverse) {
    const uint8x8_t inverted_mask_value = vsub_u8(mask_ceiling, mask_value);
    vst1_u8(mask, inverted_mask_value);
  } else {
    vst1_u8(mask, mask_value);
  }
}

#define WEIGHT8_WITHOUT_STRIDE \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0, pred_1, mask)

#define WEIGHT8_AND_STRIDE \
  WEIGHT8_WITHOUT_STRIDE;  \
  pred_0 += 8;             \
  pred_1 += 8;             \
  mask += mask_stride

// |pred_0| and |pred_1| are cast as int16_t* for the sake of pointer math. They
// are uint16_t* for 10bpp and 12bpp, and this is handled in WeightMask8_NEON.
template <bool mask_is_inverse, int bitdepth>
void WeightMask8x8_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                        const void* LIBGAV1_RESTRICT prediction_1,
                        uint8_t* LIBGAV1_RESTRICT mask, ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y = 0;
  do {
    WEIGHT8_AND_STRIDE;
  } while (++y < 7);
  WEIGHT8_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask8x16_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                         const void* LIBGAV1_RESTRICT prediction_1,
                         uint8_t* LIBGAV1_RESTRICT mask,
                         ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
  } while (++y3 < 5);
  WEIGHT8_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask8x32_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                         const void* LIBGAV1_RESTRICT prediction_1,
                         uint8_t* LIBGAV1_RESTRICT mask,
                         ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 0;
  do {
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
    WEIGHT8_AND_STRIDE;
  } while (++y5 < 6);
  WEIGHT8_AND_STRIDE;
  WEIGHT8_WITHOUT_STRIDE;
}

#define WEIGHT16_WITHOUT_STRIDE                                      \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0, pred_1, mask); \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 8, pred_1 + 8, mask + 8)

#define WEIGHT16_AND_STRIDE \
  WEIGHT16_WITHOUT_STRIDE;  \
  pred_0 += 16;             \
  pred_1 += 16;             \
  mask += mask_stride

template <bool mask_is_inverse, int bitdepth>
void WeightMask16x8_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                         const void* LIBGAV1_RESTRICT prediction_1,
                         uint8_t* LIBGAV1_RESTRICT mask,
                         ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y = 0;
  do {
    WEIGHT16_AND_STRIDE;
  } while (++y < 7);
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask16x16_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (++y3 < 5);
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask16x32_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 0;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (++y5 < 6);
  WEIGHT16_AND_STRIDE;
  WEIGHT16_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask16x64_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
    WEIGHT16_AND_STRIDE;
  } while (++y3 < 21);
  WEIGHT16_WITHOUT_STRIDE;
}

#define WEIGHT32_WITHOUT_STRIDE                                         \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0, pred_1, mask);    \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 8, pred_1 + 8,   \
                                              mask + 8);                \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 16, pred_1 + 16, \
                                              mask + 16);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 24, pred_1 + 24, \
                                              mask + 24)

#define WEIGHT32_AND_STRIDE \
  WEIGHT32_WITHOUT_STRIDE;  \
  pred_0 += 32;             \
  pred_1 += 32;             \
  mask += mask_stride

template <bool mask_is_inverse, int bitdepth>
void WeightMask32x8_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                         const void* LIBGAV1_RESTRICT prediction_1,
                         uint8_t* LIBGAV1_RESTRICT mask,
                         ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_AND_STRIDE;
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask32x16_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (++y3 < 5);
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask32x32_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 0;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (++y5 < 6);
  WEIGHT32_AND_STRIDE;
  WEIGHT32_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask32x64_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
    WEIGHT32_AND_STRIDE;
  } while (++y3 < 21);
  WEIGHT32_WITHOUT_STRIDE;
}

#define WEIGHT64_WITHOUT_STRIDE                                         \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0, pred_1, mask);    \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 8, pred_1 + 8,   \
                                              mask + 8);                \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 16, pred_1 + 16, \
                                              mask + 16);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 24, pred_1 + 24, \
                                              mask + 24);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 32, pred_1 + 32, \
                                              mask + 32);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 40, pred_1 + 40, \
                                              mask + 40);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 48, pred_1 + 48, \
                                              mask + 48);               \
  WeightMask8_NEON<mask_is_inverse, bitdepth>(pred_0 + 56, pred_1 + 56, \
                                              mask + 56)

#define WEIGHT64_AND_STRIDE \
  WEIGHT64_WITHOUT_STRIDE;  \
  pred_0 += 64;             \
  pred_1 += 64;             \
  mask += mask_stride

template <bool mask_is_inverse, int bitdepth>
void WeightMask64x16_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 5);
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask64x32_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y5 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y5 < 6);
  WEIGHT64_AND_STRIDE;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask64x64_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                          const void* LIBGAV1_RESTRICT prediction_1,
                          uint8_t* LIBGAV1_RESTRICT mask,
                          ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 21);
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask64x128_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  do {
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
    WEIGHT64_AND_STRIDE;
  } while (++y3 < 42);
  WEIGHT64_AND_STRIDE;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask128x64_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                           const void* LIBGAV1_RESTRICT prediction_1,
                           uint8_t* LIBGAV1_RESTRICT mask,
                           ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (++y3 < 21);
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
}

template <bool mask_is_inverse, int bitdepth>
void WeightMask128x128_NEON(const void* LIBGAV1_RESTRICT prediction_0,
                            const void* LIBGAV1_RESTRICT prediction_1,
                            uint8_t* LIBGAV1_RESTRICT mask,
                            ptrdiff_t mask_stride) {
  const auto* pred_0 = static_cast<const int16_t*>(prediction_0);
  const auto* pred_1 = static_cast<const int16_t*>(prediction_1);
  int y3 = 0;
  const ptrdiff_t adjusted_mask_stride = mask_stride - 64;
  do {
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;

    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += 64;
    WEIGHT64_WITHOUT_STRIDE;
    pred_0 += 64;
    pred_1 += 64;
    mask += adjusted_mask_stride;
  } while (++y3 < 42);
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += adjusted_mask_stride;

  WEIGHT64_WITHOUT_STRIDE;
  pred_0 += 64;
  pred_1 += 64;
  mask += 64;
  WEIGHT64_WITHOUT_STRIDE;
}
#undef WEIGHT8_WITHOUT_STRIDE
#undef WEIGHT8_AND_STRIDE
#undef WEIGHT16_WITHOUT_STRIDE
#undef WEIGHT16_AND_STRIDE
#undef WEIGHT32_WITHOUT_STRIDE
#undef WEIGHT32_AND_STRIDE
#undef WEIGHT64_WITHOUT_STRIDE
#undef WEIGHT64_AND_STRIDE

#define INIT_WEIGHT_MASK_8BPP(width, height, w_index, h_index) \
  dsp->weight_mask[w_index][h_index][0] =                      \
      WeightMask##width##x##height##_NEON<0, 8>;               \
  dsp->weight_mask[w_index][h_index][1] =                      \
      WeightMask##width##x##height##_NEON<1, 8>
void Init8bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth8);
  assert(dsp != nullptr);
  INIT_WEIGHT_MASK_8BPP(8, 8, 0, 0);
  INIT_WEIGHT_MASK_8BPP(8, 16, 0, 1);
  INIT_WEIGHT_MASK_8BPP(8, 32, 0, 2);
  INIT_WEIGHT_MASK_8BPP(16, 8, 1, 0);
  INIT_WEIGHT_MASK_8BPP(16, 16, 1, 1);
  INIT_WEIGHT_MASK_8BPP(16, 32, 1, 2);
  INIT_WEIGHT_MASK_8BPP(16, 64, 1, 3);
  INIT_WEIGHT_MASK_8BPP(32, 8, 2, 0);
  INIT_WEIGHT_MASK_8BPP(32, 16, 2, 1);
  INIT_WEIGHT_MASK_8BPP(32, 32, 2, 2);
  INIT_WEIGHT_MASK_8BPP(32, 64, 2, 3);
  INIT_WEIGHT_MASK_8BPP(64, 16, 3, 1);
  INIT_WEIGHT_MASK_8BPP(64, 32, 3, 2);
  INIT_WEIGHT_MASK_8BPP(64, 64, 3, 3);
  INIT_WEIGHT_MASK_8BPP(64, 128, 3, 4);
  INIT_WEIGHT_MASK_8BPP(128, 64, 4, 3);
  INIT_WEIGHT_MASK_8BPP(128, 128, 4, 4);
}
#undef INIT_WEIGHT_MASK_8BPP

}  // namespace

#if LIBGAV1_MAX_BITDEPTH >= 10
namespace high_bitdepth {
namespace {

#define INIT_WEIGHT_MASK_10BPP(width, height, w_index, h_index) \
  dsp->weight_mask[w_index][h_index][0] =                       \
      WeightMask##width##x##height##_NEON<0, 10>;               \
  dsp->weight_mask[w_index][h_index][1] =                       \
      WeightMask##width##x##height##_NEON<1, 10>
void Init10bpp() {
  Dsp* const dsp = dsp_internal::GetWritableDspTable(kBitdepth10);
  assert(dsp != nullptr);
  INIT_WEIGHT_MASK_10BPP(8, 8, 0, 0);
  INIT_WEIGHT_MASK_10BPP(8, 16, 0, 1);
  INIT_WEIGHT_MASK_10BPP(8, 32, 0, 2);
  INIT_WEIGHT_MASK_10BPP(16, 8, 1, 0);
  INIT_WEIGHT_MASK_10BPP(16, 16, 1, 1);
  INIT_WEIGHT_MASK_10BPP(16, 32, 1, 2);
  INIT_WEIGHT_MASK_10BPP(16, 64, 1, 3);
  INIT_WEIGHT_MASK_10BPP(32, 8, 2, 0);
  INIT_WEIGHT_MASK_10BPP(32, 16, 2, 1);
  INIT_WEIGHT_MASK_10BPP(32, 32, 2, 2);
  INIT_WEIGHT_MASK_10BPP(32, 64, 2, 3);
  INIT_WEIGHT_MASK_10BPP(64, 16, 3, 1);
  INIT_WEIGHT_MASK_10BPP(64, 32, 3, 2);
  INIT_WEIGHT_MASK_10BPP(64, 64, 3, 3);
  INIT_WEIGHT_MASK_10BPP(64, 128, 3, 4);
  INIT_WEIGHT_MASK_10BPP(128, 64, 4, 3);
  INIT_WEIGHT_MASK_10BPP(128, 128, 4, 4);
}
#undef INIT_WEIGHT_MASK_10BPP

}  // namespace
}  // namespace high_bitdepth
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
void WeightMaskInit_NEON() {
  Init8bpp();
#if LIBGAV1_MAX_BITDEPTH >= 10
  high_bitdepth::Init10bpp();
#endif  // LIBGAV1_MAX_BITDEPTH >= 10
}

}  // namespace dsp
}  // namespace libgav1

#else   // !LIBGAV1_ENABLE_NEON

namespace libgav1 {
namespace dsp {

void WeightMaskInit_NEON() {}

}  // namespace dsp
}  // namespace libgav1
#endif  // LIBGAV1_ENABLE_NEON
