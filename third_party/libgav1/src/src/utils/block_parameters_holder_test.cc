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

#include "src/utils/block_parameters_holder.h"

#include "gtest/gtest.h"
#include "src/utils/constants.h"
#include "src/utils/types.h"

namespace libgav1 {
namespace {

TEST(BlockParametersHolder, TestBasic) {
  BlockParametersHolder holder;
  ASSERT_TRUE(holder.Reset(20, 20));

  // Get a BlockParameters object.
  BlockParameters* const bp1 = holder.Get(10, 10, kBlock32x32);
  ASSERT_NE(bp1, nullptr);
  // Ensure that cache was filled appropriately. From (10, 10) to (17, 17)
  // should be bp1 (10 + 4x4 width/height of 32x32 block is 18).
  for (int i = 10; i < 18; ++i) {
    for (int j = 10; j < 18; ++j) {
      EXPECT_EQ(holder.Find(i, j), bp1)
          << "Mismatch in (" << i << ", " << j << ")";
    }
  }

  // Get the maximum number of BlockParameters objects.
  for (int i = 0; i < 399; ++i) {
    EXPECT_NE(holder.Get(10, 10, kBlock32x32), nullptr)
        << "Mismatch in index " << i;
  }

  // Get() should now return nullptr since there are no more BlockParameters
  // objects available.
  EXPECT_EQ(holder.Get(10, 10, kBlock32x32), nullptr);

  // Reset the holder to the same size.
  ASSERT_TRUE(holder.Reset(20, 20));

  // Get a BlockParameters object. This should be the same as bp1 since the
  // holder was Reset to the same size.
  BlockParameters* const bp2 = holder.Get(10, 10, kBlock32x32);
  EXPECT_EQ(bp2, bp1);

  // Reset the holder to a smaller size.
  ASSERT_TRUE(holder.Reset(20, 10));

  // Get a BlockParameters object. This should be the same as bp1 since the
  // holder was Reset to a smaller size.
  BlockParameters* const bp3 = holder.Get(0, 0, kBlock32x32);
  EXPECT_EQ(bp3, bp1);

  // Reset the holder to a larger size.
  ASSERT_TRUE(holder.Reset(30, 30));

  // Get a BlockParameters object. This may or may not be the same as bp1 since
  // the holder was Reset to a larger size.
  BlockParameters* const bp4 = holder.Get(0, 0, kBlock32x32);
  EXPECT_NE(bp4, nullptr);
}

}  // namespace
}  // namespace libgav1
