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

#include "src/dsp/x86/common_sse4.h"

#include "gtest/gtest.h"

#if LIBGAV1_TARGETING_SSE4_1

#include <cstdint>

#include "src/utils/common.h"

namespace libgav1 {
namespace dsp {
namespace {

// Show that RightShiftWithRounding_S16() is equal to
// RightShiftWithRounding() only for values less than or equal to
// INT16_MAX - ((1 << bits) >> 1). In particular, if bits == 16, then
// RightShiftWithRounding_S16() is equal to RightShiftWithRounding() only for
// negative values.
TEST(CommonDspTest, SSE41RightShiftWithRoundingS16) {
  for (int bits = 0; bits < 16; ++bits) {
    const int bias = (1 << bits) >> 1;
    for (int32_t value = INT16_MIN; value <= INT16_MAX; ++value) {
      const __m128i v_val_d = _mm_set1_epi16(value);
      const __m128i v_result_d = RightShiftWithRounding_S16(v_val_d, bits);
      const int16_t result = _mm_extract_epi16(v_result_d, 0);
      const int32_t expected = RightShiftWithRounding(value, bits);
      if (value <= INT16_MAX - bias) {
        EXPECT_EQ(result, expected) << "value: " << value << ", bits: " << bits;
      } else {
        EXPECT_EQ(expected, 1 << (15 - bits));
        EXPECT_EQ(result, -expected)
            << "value: " << value << ", bits: " << bits;
      }
    }
  }
}

}  // namespace
}  // namespace dsp
}  // namespace libgav1

#else  // !LIBGAV1_TARGETING_SSE4_1

TEST(CommonDspTest, SSE41) {
  GTEST_SKIP() << "Build this module for x86(-64) with SSE4 enabled to enable "
                  "the tests.";
}

#endif  // LIBGAV1_TARGETING_SSE4_1
