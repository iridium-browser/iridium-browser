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

#include "src/quantizer.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "src/obu_parser.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

TEST(QuantizerTest, GetQIndex) {
  const int kBaseQIndex = 40;
  const int kDelta = 10;
  const int kOutOfRangeIndex = 200;
  Segmentation segmentation = {};

  EXPECT_EQ(GetQIndex(segmentation, 0, kBaseQIndex), kBaseQIndex);
  EXPECT_EQ(GetQIndex(segmentation, kOutOfRangeIndex, kBaseQIndex),
            kBaseQIndex);

  segmentation.enabled = true;
  EXPECT_EQ(GetQIndex(segmentation, 0, kBaseQIndex), kBaseQIndex);
  EXPECT_EQ(GetQIndex(segmentation, kOutOfRangeIndex, kBaseQIndex),
            kBaseQIndex);

  segmentation.feature_enabled[1][kSegmentFeatureQuantizer] = true;
  segmentation.feature_data[1][kSegmentFeatureQuantizer] = kDelta;
  EXPECT_EQ(GetQIndex(segmentation, 1, kBaseQIndex), kBaseQIndex + kDelta);
  EXPECT_EQ(GetQIndex(segmentation, kOutOfRangeIndex, kBaseQIndex),
            kBaseQIndex);

  segmentation.enabled = false;
  EXPECT_EQ(GetQIndex(segmentation, 1, kBaseQIndex), kBaseQIndex);
  EXPECT_EQ(GetQIndex(segmentation, kOutOfRangeIndex, kBaseQIndex),
            kBaseQIndex);
}

TEST(QuantizerTest, GetDcValue) {
  QuantizerParameters params = {};
  params.delta_dc[kPlaneY] = 1;
  params.delta_dc[kPlaneU] = 2;
  params.delta_dc[kPlaneV] = 3;

  // Test lookups of Dc_Qlookup[0][0], Dc_Qlookup[0][11], Dc_Qlookup[0][12],
  // and Dc_Qlookup[0][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(8, &params);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 10), 16);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 11), 17);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 254), 1336);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 255), 1336);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 9), 16);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 10), 17);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 253), 1336);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 254), 1336);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -4), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 8), 16);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 9), 17);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 252), 1336);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 253), 1336);
  }

#if LIBGAV1_MAX_BITDEPTH >= 10
  // Test lookups of Dc_Qlookup[1][0], Dc_Qlookup[1][11], Dc_Qlookup[1][12],
  // and Dc_Qlookup[1][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(10, &params);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 10), 34);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 11), 37);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 254), 5347);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 255), 5347);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 9), 34);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 10), 37);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 253), 5347);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 254), 5347);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -4), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 8), 34);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 9), 37);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 254), 5347);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 253), 5347);
  }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
  // Test lookups of Dc_Qlookup[2][0], Dc_Qlookup[2][11], Dc_Qlookup[2][12],
  // and Dc_Qlookup[2][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(12, &params);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 10), 103);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 11), 115);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 254), 21387);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneY, 255), 21387);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 9), 103);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 10), 115);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 253), 21387);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneU, 254), 21387);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -4), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 8), 103);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 9), 115);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 254), 21387);
    EXPECT_EQ(quantizer.GetDcValue(kPlaneV, 253), 21387);
  }
#endif  // LIBGAV1_MAX_BITDEPTH == 12
}

TEST(QuantizerTest, GetAcValue) {
  QuantizerParameters params = {};
  params.delta_ac[kPlaneU] = 1;
  params.delta_ac[kPlaneV] = 2;

  // Test lookups of Ac_Qlookup[0][0], Ac_Qlookup[0][11], Ac_Qlookup[0][12],
  // and Ac_Qlookup[0][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(8, &params);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 0), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 11), 18);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 12), 19);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 255), 1828);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 256), 1828);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 10), 18);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 11), 19);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 254), 1828);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 255), 1828);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 9), 18);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 10), 19);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 253), 1828);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 254), 1828);
  }

#if LIBGAV1_MAX_BITDEPTH >= 10
  // Test lookups of Ac_Qlookup[1][0], Ac_Qlookup[1][11], Ac_Qlookup[1][12],
  // and Ac_Qlookup[1][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(10, &params);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 0), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 11), 37);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 12), 40);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 255), 7312);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 256), 7312);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 10), 37);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 11), 40);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 254), 7312);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 255), 7312);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 9), 37);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 10), 40);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 253), 7312);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 254), 7312);
  }
#endif  // LIBGAV1_MAX_BITDEPTH >= 10

#if LIBGAV1_MAX_BITDEPTH == 12
  // Test lookups of Ac_Qlookup[1][0], Ac_Qlookup[1][11], Ac_Qlookup[1][12],
  // and Ac_Qlookup[1][255] in the spec, including the clipping of qindex.
  {
    Quantizer quantizer(12, &params);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 0), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 11), 112);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 12), 126);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 255), 29247);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneY, 256), 29247);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, -1), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 10), 112);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 11), 126);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 254), 29247);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneU, 255), 29247);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -3), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, -2), 4);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 9), 112);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 10), 126);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 253), 29247);
    EXPECT_EQ(quantizer.GetAcValue(kPlaneV, 254), 29247);
  }
#endif  // LIBGAV1_MAX_BITDEPTH == 12
}

}  // namespace
}  // namespace libgav1
