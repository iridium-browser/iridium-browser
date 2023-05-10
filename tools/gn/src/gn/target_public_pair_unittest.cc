// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/target_public_pair.h"
#include "util/test/test.h"

TEST(TargetPublicPairTest, ConstructionAndMutation) {
  // Fake target pointer values.
  const auto* a_target = reinterpret_cast<const Target*>(1000);
  const auto* b_target = reinterpret_cast<const Target*>(2000);

  TargetPublicPair a_pair(a_target, true);
  EXPECT_EQ(a_target, a_pair.target());
  EXPECT_TRUE(a_pair.is_public());

  TargetPublicPair b_pair(b_target, false);
  EXPECT_EQ(b_target, b_pair.target());
  EXPECT_FALSE(b_pair.is_public());

  a_pair.set_target(b_target);
  EXPECT_EQ(b_target, a_pair.target());
  EXPECT_TRUE(a_pair.is_public());

  a_pair.set_is_public(false);
  EXPECT_EQ(b_target, a_pair.target());
  EXPECT_FALSE(a_pair.is_public());

  a_pair = TargetPublicPair(a_target, true);
  EXPECT_EQ(a_target, a_pair.target());
  EXPECT_TRUE(a_pair.is_public());

  b_pair = std::move(a_pair);
  EXPECT_EQ(a_target, b_pair.target());
  EXPECT_TRUE(b_pair.is_public());
}

TEST(TargetPublicPairTest, Builder) {
  const auto* a_target = reinterpret_cast<const Target*>(1000);
  const auto* b_target = reinterpret_cast<const Target*>(2000);
  TargetPublicPairListBuilder builder;

  builder.Append(a_target, false);
  builder.Append(b_target, false);
  builder.Append(a_target, true);
  builder.Append(b_target, false);

  auto list = builder.Build();
  EXPECT_EQ(2u, list.size());
  EXPECT_EQ(a_target, list[0].target());
  EXPECT_EQ(b_target, list[1].target());
  EXPECT_TRUE(list[0].is_public());
  EXPECT_FALSE(list[1].is_public());
}
