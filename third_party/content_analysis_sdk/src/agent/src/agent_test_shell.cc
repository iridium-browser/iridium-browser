// Copyright 2022 The Chromium Authors.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"

namespace content_analysis {
namespace sdk {

namespace testing {

    TEST(HelloTest, BasicAssertions) {
        // Expect two strings not to be equal.
        EXPECT_STRNE("hello", "world");
        // Expect equality.
        EXPECT_EQ(7 * 6, 42);
    }

}  // namespace testing

}  // namespace sdk
}  // namespace content_analysis