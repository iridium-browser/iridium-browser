// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/vector_utils.h"

#include "util/test/test.h"

#include <string>

TEST(VectorSetSorter, AsVectorWithStrings) {
  VectorSetSorter<std::string> sorter;

  std::vector<std::string> input = {
      "World!", "Hello", "bonjour", "Hello", "monde!", "World!",
  };

  sorter.Add(input.begin(), input.end());
  auto result = sorter.AsVector();

  ASSERT_EQ(result.size(), 4u) << result.size();
  EXPECT_STREQ(result[0].c_str(), "Hello");
  EXPECT_STREQ(result[1].c_str(), "World!");
  EXPECT_STREQ(result[2].c_str(), "bonjour");
  EXPECT_STREQ(result[3].c_str(), "monde!");
}

TEST(VectorSetSorter, IterateOverWithStrings) {
  VectorSetSorter<std::string> sorter;

  std::vector<std::string> input = {
      "World!", "Hello", "bonjour", "Hello", "monde!", "World!",
  };

  sorter.Add(input.begin(), input.end());

  std::vector<std::string> result;

  sorter.IterateOver(
      [&result](const std::string& str) { result.push_back(str); });

  ASSERT_EQ(result.size(), 4u) << result.size();
  EXPECT_STREQ(result[0].c_str(), "Hello");
  EXPECT_STREQ(result[1].c_str(), "World!");
  EXPECT_STREQ(result[2].c_str(), "bonjour");
  EXPECT_STREQ(result[3].c_str(), "monde!");
}
