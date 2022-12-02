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

#include "src/dsp/constants.h"

#include <cstdint>

namespace libgav1 {

// Each set of 7 taps is padded with a 0 to easily align and pack into the high
// and low 8 bytes. This way, we can load 16 at a time to fit mulhi and mullo.
alignas(16) const int8_t kFilterIntraTaps[kNumFilterIntraPredictors][8][8] = {
    {{-6, 10, 0, 0, 0, 12, 0, 0},
     {-5, 2, 10, 0, 0, 9, 0, 0},
     {-3, 1, 1, 10, 0, 7, 0, 0},
     {-3, 1, 1, 2, 10, 5, 0, 0},
     {-4, 6, 0, 0, 0, 2, 12, 0},
     {-3, 2, 6, 0, 0, 2, 9, 0},
     {-3, 2, 2, 6, 0, 2, 7, 0},
     {-3, 1, 2, 2, 6, 3, 5, 0}},
    {{-10, 16, 0, 0, 0, 10, 0, 0},
     {-6, 0, 16, 0, 0, 6, 0, 0},
     {-4, 0, 0, 16, 0, 4, 0, 0},
     {-2, 0, 0, 0, 16, 2, 0, 0},
     {-10, 16, 0, 0, 0, 0, 10, 0},
     {-6, 0, 16, 0, 0, 0, 6, 0},
     {-4, 0, 0, 16, 0, 0, 4, 0},
     {-2, 0, 0, 0, 16, 0, 2, 0}},
    {{-8, 8, 0, 0, 0, 16, 0, 0},
     {-8, 0, 8, 0, 0, 16, 0, 0},
     {-8, 0, 0, 8, 0, 16, 0, 0},
     {-8, 0, 0, 0, 8, 16, 0, 0},
     {-4, 4, 0, 0, 0, 0, 16, 0},
     {-4, 0, 4, 0, 0, 0, 16, 0},
     {-4, 0, 0, 4, 0, 0, 16, 0},
     {-4, 0, 0, 0, 4, 0, 16, 0}},
    {{-2, 8, 0, 0, 0, 10, 0, 0},
     {-1, 3, 8, 0, 0, 6, 0, 0},
     {-1, 2, 3, 8, 0, 4, 0, 0},
     {0, 1, 2, 3, 8, 2, 0, 0},
     {-1, 4, 0, 0, 0, 3, 10, 0},
     {-1, 3, 4, 0, 0, 4, 6, 0},
     {-1, 2, 3, 4, 0, 4, 4, 0},
     {-1, 2, 2, 3, 4, 3, 3, 0}},
    {{-12, 14, 0, 0, 0, 14, 0, 0},
     {-10, 0, 14, 0, 0, 12, 0, 0},
     {-9, 0, 0, 14, 0, 11, 0, 0},
     {-8, 0, 0, 0, 14, 10, 0, 0},
     {-10, 12, 0, 0, 0, 0, 14, 0},
     {-9, 1, 12, 0, 0, 0, 12, 0},
     {-8, 0, 0, 12, 0, 1, 11, 0},
     {-7, 0, 0, 1, 12, 1, 9, 0}}};

// A lookup table replacing the calculation of the variable s in Section 7.17.3
// (Box filter process). The first index is sgr_proj_index (the lr_sgr_set
// syntax element in the Spec, saved in the sgr_proj_info.index field of a
// RestorationUnitInfo struct). The second index is pass (0 or 1).
//
// const uint8_t scale = kSgrProjParams[sgr_proj_index][pass * 2 + 1];
// const uint32_t n2_with_scale = n * n * scale;
// const uint32_t s =
// ((1 << kSgrProjScaleBits) + (n2_with_scale >> 1)) / n2_with_scale;
// 0 is an invalid value, corresponding to radius = 0, where the filter is
// skipped.
const uint16_t kSgrScaleParameter[16][2] = {
    {140, 3236}, {112, 2158}, {93, 1618}, {80, 1438}, {70, 1295}, {58, 1177},
    {47, 1079},  {37, 996},   {30, 925},  {25, 863},  {0, 2589},  {0, 1618},
    {0, 1177},   {0, 925},    {56, 0},    {22, 0},
};

const uint8_t kCdefPrimaryTaps[2][2] = {{4, 2}, {3, 3}};

// This is Cdef_Directions (section 7.15.3) with 2 padding entries at the
// beginning and end of the table. The cdef direction range is [0, 7] and the
// first index is offset +/-2. This removes the need to constrain the first
// index to the same range using e.g., & 7.
const int8_t kCdefDirectionsPadded[12][2][2] = {
    {{1, 0}, {2, 0}},    // Padding: Cdef_Directions[6]
    {{1, 0}, {2, -1}},   // Padding: Cdef_Directions[7]
    {{-1, 1}, {-2, 2}},  // Begin Cdef_Directions
    {{0, 1}, {-1, 2}},   //
    {{0, 1}, {0, 2}},    //
    {{0, 1}, {1, 2}},    //
    {{1, 1}, {2, 2}},    //
    {{1, 0}, {2, 1}},    //
    {{1, 0}, {2, 0}},    //
    {{1, 0}, {2, -1}},   // End Cdef_Directions
    {{-1, 1}, {-2, 2}},  // Padding: Cdef_Directions[0]
    {{0, 1}, {-1, 2}},   // Padding: Cdef_Directions[1]
};

}  // namespace libgav1
