// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/flat_map.h"

#include <chrono>

#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

namespace openscreen {

namespace {

const FlatMap<int, absl::string_view> kSimpleFlatMap{{-1, "bar"},
                                                     {123, "foo"},
                                                     {10000, "baz"},
                                                     {0, ""}};

}  // namespace

TEST(FlatMapTest, Find) {
  EXPECT_EQ(kSimpleFlatMap.begin(), kSimpleFlatMap.find(-1));
  EXPECT_EQ("bar", kSimpleFlatMap.find(-1)->second);
  EXPECT_EQ("foo", kSimpleFlatMap.find(123)->second);
  EXPECT_EQ("baz", kSimpleFlatMap.find(10000)->second);
  EXPECT_EQ("", kSimpleFlatMap.find(0)->second);
  EXPECT_EQ(kSimpleFlatMap.end(), kSimpleFlatMap.find(2));
}

// Since it is backed by a vector, we don't expose an operator[] due
// to potential confusion. If the type of Key is int or std::size_t, do
// you want the std::pair<Key, Value> or just the Value?
TEST(FlatMapTest, Access) {
  EXPECT_EQ("bar", kSimpleFlatMap[0].second);
  EXPECT_EQ("foo", kSimpleFlatMap[1].second);
  EXPECT_EQ("baz", kSimpleFlatMap[2].second);
  EXPECT_EQ("", kSimpleFlatMap[3].second);

  // NOTE: vector doesn't do any range checking for operator[], only at().
  EXPECT_EQ("bar", kSimpleFlatMap.at(0).second);
  EXPECT_EQ("foo", kSimpleFlatMap.at(1).second);
  EXPECT_EQ("baz", kSimpleFlatMap.at(2).second);
  EXPECT_EQ("", kSimpleFlatMap.at(3).second);
  // The error message varies widely depending on how the test is run.
  // NOTE: Google Test doesn't support death tests on some platforms, such
  // as iOS.
#if defined(GTEST_HAS_DEATH_TEST)
  EXPECT_DEATH(kSimpleFlatMap.at(31337), ".*");
#endif
}

TEST(FlatMapTest, ErasureAndEmplacement) {
  auto mutable_vector = kSimpleFlatMap;
  EXPECT_NE(mutable_vector.find(-1), mutable_vector.end());
  mutable_vector.erase_key(-1);
  EXPECT_EQ(mutable_vector.find(-1), mutable_vector.end());

  mutable_vector.emplace_back(-1, "bar");
  EXPECT_NE(mutable_vector.find(-1), mutable_vector.end());

  // We absolutely should fail to erase something that's not there.
#if defined(GTEST_HAS_DEATH_TEST)
  EXPECT_DEATH(mutable_vector.erase_key(12345),
               ".*failed to erase: element not found.*");
#endif
}

TEST(FlatMapTest, Mutation) {
  FlatMap<int, int> mutable_vector{{1, 2}};
  EXPECT_EQ(2, mutable_vector[0].second);

  mutable_vector[0].second = 3;
  EXPECT_EQ(3, mutable_vector[0].second);
}

TEST(FlatMapTest, GenerallyBehavesLikeAVector) {
  EXPECT_EQ(kSimpleFlatMap, kSimpleFlatMap);
  EXPECT_TRUE(kSimpleFlatMap == kSimpleFlatMap);

  for (auto& entry : kSimpleFlatMap) {
    if (entry.first != 0) {
      EXPECT_LT(0u, entry.second.size());
    }
  }

  FlatMap<int, int> simple;
  simple.emplace_back(1, 10);
  EXPECT_EQ(simple, (FlatMap<int, int>{{1, 10}}));

  auto it = simple.find(1);
  ASSERT_NE(it, simple.end());
  simple.erase(it);
  EXPECT_EQ(simple.find(1), simple.end());
}

TEST(FlatMapTest, CanUseNonDefaultConstructibleThings) {
  class NonDefaultConstructible {
   public:
    NonDefaultConstructible(int x, int y) : x_(x), y_(y) {}

    int x() { return x_; }

    int y() { return y_; }

   private:
    int x_;
    int y_;
  };

  FlatMap<int, NonDefaultConstructible> flat_map;
  flat_map.emplace_back(3, NonDefaultConstructible(2, 3));
  auto it = flat_map.find(3);
  ASSERT_TRUE(it != flat_map.end());
  EXPECT_GT(it->second.y(), it->second.x());
}
}  // namespace openscreen
