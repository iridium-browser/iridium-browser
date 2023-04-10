// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/version.h"

#include "util/test/test.h"

TEST(VersionTest, FromString) {
  Version v0_0_1{0, 0, 1};
  ASSERT_EQ(Version::FromString("0.0.1"), v0_0_1);
  Version v0_1_0{0, 1, 0};
  ASSERT_EQ(Version::FromString("0.1.0"), v0_1_0);
  Version v1_0_0{1, 0, 0};
  ASSERT_EQ(Version::FromString("1.0.0"), v1_0_0);
}

TEST(VersionTest, Comparison) {
  Version v0_0_1{0, 0, 1};
  Version v0_1_0{0, 1, 0};
  ASSERT_TRUE(v0_0_1 == v0_0_1);
  ASSERT_TRUE(v0_0_1 != v0_1_0);
  ASSERT_TRUE(v0_0_1 <= v0_0_1);
  ASSERT_TRUE(v0_0_1 <= v0_1_0);
  ASSERT_TRUE(v0_0_1 < v0_1_0);
  ASSERT_TRUE(v0_0_1 >= v0_0_1);
  ASSERT_TRUE(v0_1_0 > v0_0_1);
  ASSERT_TRUE(v0_1_0 >= v0_0_1);
}

TEST(VersionTest, Describe) {
  ASSERT_EQ(Version::FromString("0.0.1")->Describe(), "0.0.1");
}
