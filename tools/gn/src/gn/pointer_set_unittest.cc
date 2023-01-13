// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/pointer_set.h"
#include "util/test/test.h"

#include <algorithm>

struct Foo {
  int x;
};

static const Foo kFoo1[1] = {{1}};
static const Foo kFoo2[1] = {{2}};
static const Foo kFoo3[1] = {{3}};

static const std::initializer_list<const Foo*> kFullList = {kFoo1, kFoo2,
                                                            kFoo3};

using TestPointerSet = PointerSet<const Foo>;

TEST(PointerSet, DefaultConstruction) {
  TestPointerSet set;
  EXPECT_TRUE(set.empty());
  EXPECT_EQ(0u, set.size());
  EXPECT_FALSE(set.contains(kFoo1));
}

TEST(PointerSet, RangeConstruction) {
  TestPointerSet set(kFullList.begin(), kFullList.end());
  EXPECT_FALSE(set.empty());
  EXPECT_EQ(3u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));
  EXPECT_TRUE(set.contains(kFoo2));
  EXPECT_TRUE(set.contains(kFoo3));
}

TEST(PointerSet, CopyConstruction) {
  TestPointerSet set1(kFullList.begin(), kFullList.end());
  TestPointerSet set2(set1);
  set1.clear();
  EXPECT_TRUE(set1.empty());
  EXPECT_FALSE(set2.empty());
  EXPECT_EQ(3u, set2.size());
  EXPECT_TRUE(set2.contains(kFoo1));
  EXPECT_TRUE(set2.contains(kFoo2));
  EXPECT_TRUE(set2.contains(kFoo3));
}

TEST(PointerSet, MoveConstruction) {
  TestPointerSet set1(kFullList.begin(), kFullList.end());
  TestPointerSet set2(std::move(set1));
  EXPECT_TRUE(set1.empty());
  EXPECT_FALSE(set2.empty());
  EXPECT_EQ(3u, set2.size());
  EXPECT_TRUE(set2.contains(kFoo1));
  EXPECT_TRUE(set2.contains(kFoo2));
  EXPECT_TRUE(set2.contains(kFoo3));
}

TEST(PointerSet, Add) {
  TestPointerSet set;
  EXPECT_TRUE(set.add(kFoo1));
  EXPECT_EQ(1u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));

  EXPECT_FALSE(set.add(kFoo1));
  EXPECT_EQ(1u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));

  EXPECT_TRUE(set.add(kFoo2));
  EXPECT_EQ(2u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));
  EXPECT_TRUE(set.contains(kFoo2));

  EXPECT_FALSE(set.add(kFoo1));
  EXPECT_FALSE(set.add(kFoo2));

  EXPECT_TRUE(set.add(kFoo3));
  EXPECT_EQ(3u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));
  EXPECT_TRUE(set.contains(kFoo2));
  EXPECT_TRUE(set.contains(kFoo3));

  EXPECT_FALSE(set.add(kFoo1));
  EXPECT_FALSE(set.add(kFoo2));
  EXPECT_FALSE(set.add(kFoo3));
}

TEST(PointerSet, Erase) {
  TestPointerSet set(kFullList.begin(), kFullList.end());
  EXPECT_EQ(3u, set.size());

  EXPECT_TRUE(set.erase(kFoo1));
  EXPECT_EQ(2u, set.size());
  EXPECT_FALSE(set.contains(kFoo1));
  EXPECT_FALSE(set.erase(kFoo1));
  EXPECT_EQ(2u, set.size());

  EXPECT_TRUE(set.erase(kFoo2));
  EXPECT_EQ(1u, set.size());
  EXPECT_FALSE(set.contains(kFoo2));
  EXPECT_FALSE(set.erase(kFoo2));
  EXPECT_EQ(1u, set.size());

  EXPECT_TRUE(set.erase(kFoo3));
  EXPECT_EQ(0u, set.size());
  EXPECT_FALSE(set.contains(kFoo3));
  EXPECT_FALSE(set.erase(kFoo3));
  EXPECT_EQ(0u, set.size());
}

TEST(PointerSet, RangeInsert) {
  TestPointerSet set;
  set.insert(kFullList.begin(), kFullList.end());
  EXPECT_EQ(3u, set.size());
  EXPECT_TRUE(set.contains(kFoo1));
  EXPECT_TRUE(set.contains(kFoo2));
  EXPECT_TRUE(set.contains(kFoo3));

  set.insert(kFullList.begin(), kFullList.end());
  EXPECT_EQ(3u, set.size());
}

TEST(PointerSet, InsertOther) {
  TestPointerSet set1(kFullList.begin(), kFullList.end());
  TestPointerSet set2;
  set2.add(kFoo1);
  set1.insert(set2);
  EXPECT_EQ(3u, set1.size());
  EXPECT_EQ(1u, set2.size());

  set1.clear();
  set1.add(kFoo1);
  set2.clear();
  set2.add(kFoo3);
  set1.insert(set2);
  EXPECT_EQ(2u, set1.size());
  EXPECT_EQ(1u, set2.size());
  EXPECT_TRUE(set1.contains(kFoo1));
  EXPECT_TRUE(set1.contains(kFoo3));
}

TEST(PointerSet, IntersectionWith) {
  TestPointerSet set1;
  TestPointerSet set2;

  set1.add(kFoo1);
  set2.add(kFoo3);

  TestPointerSet set = set1.intersection_with(set2);
  EXPECT_TRUE(set.empty());

  set1.add(kFoo2);
  set2.add(kFoo2);

  set = set1.intersection_with(set2);
  EXPECT_FALSE(set.empty());
  EXPECT_EQ(1u, set.size());
  EXPECT_TRUE(set.contains(kFoo2));

  set1.insert(kFullList.begin(), kFullList.end());
  set2 = set1;
  set = set1.intersection_with(set2);
  EXPECT_EQ(3u, set.size());
  EXPECT_EQ(set1, set);
  EXPECT_EQ(set2, set);
}

TEST(PointerSet, ToVector) {
  TestPointerSet set(kFullList.begin(), kFullList.end());
  auto vector = set.ToVector();
  EXPECT_EQ(vector.size(), kFullList.size());

  // NOTE: Order of items in the result is not guaranteed
  // so just check whether items are available in it.
  EXPECT_NE(std::find(vector.begin(), vector.end(), kFoo1), vector.end());
  EXPECT_NE(std::find(vector.begin(), vector.end(), kFoo2), vector.end());
  EXPECT_NE(std::find(vector.begin(), vector.end(), kFoo3), vector.end());
}