/*
 * Copyright 2019 The libgav1 Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBGAV1_SRC_DSP_CONSTANTS_H_
#define LIBGAV1_SRC_DSP_CONSTANTS_H_

// This file contains DSP related constants that have a direct relationship with
// a DSP component.

#include <cstdint>

#include "src/utils/constants.h"

namespace libgav1 {

enum {
  // Documentation variables.
  kBitdepth8 = 8,
  kBitdepth10 = 10,
  kBitdepth12 = 12,
  // Weights are quadratic from '1' to '1 / block_size', scaled by
  // 2^kSmoothWeightScale.
  kSmoothWeightScale = 8,
  kCflLumaBufferStride = 32,
  // InterRound0, Section 7.11.3.2.
  kInterRoundBitsHorizontal = 3,  // 8 & 10-bit.
  kInterRoundBitsHorizontal12bpp = 5,
  kInterRoundBitsCompoundVertical = 7,  // 8, 10 & 12-bit compound prediction.
  kInterRoundBitsVertical = 11,         // 8 & 10-bit, single prediction.
  kInterRoundBitsVertical12bpp = 9,
  // Offset applied to 10bpp and 12bpp predictors to allow storing them in
  // uint16_t. Removed before blending.
  kCompoundOffset = (1 << 14) + (1 << 13),
  kCdefSecondaryTap0 = 2,
  kCdefSecondaryTap1 = 1,
};  // anonymous enum

extern const int8_t kFilterIntraTaps[kNumFilterIntraPredictors][8][8];

// Values in this enum can be derived as the sum of subsampling_x and
// subsampling_y (since subsampling_x == 0 && subsampling_y == 1 case is never
// allowed by the bitstream).
enum SubsamplingType : uint8_t {
  kSubsamplingType444,  // subsampling_x = 0, subsampling_y = 0.
  kSubsamplingType422,  // subsampling_x = 1, subsampling_y = 0.
  kSubsamplingType420,  // subsampling_x = 1, subsampling_y = 1.
  kNumSubsamplingTypes
};

extern const uint16_t kSgrScaleParameter[16][2];

extern const uint8_t kCdefPrimaryTaps[2][2];

extern const int8_t kCdefDirectionsPadded[12][2][2];

}  // namespace libgav1

#endif  // LIBGAV1_SRC_DSP_CONSTANTS_H_
