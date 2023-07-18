// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/unique_vector.h"

#include <stddef.h>

#include <algorithm>

#include "util/test/test.h"

TEST(UniqueVector, PushBack) {
  UniqueVector<int> foo;
  EXPECT_TRUE(foo.push_back(1));
  EXPECT_FALSE(foo.push_back(1));
  EXPECT_TRUE(foo.push_back(2));
  EXPECT_TRUE(foo.push_back(0));
  EXPECT_FALSE(foo.push_back(2));
  EXPECT_FALSE(foo.push_back(1));

  EXPECT_EQ(3u, foo.size());
  EXPECT_EQ(1, foo[0]);
  EXPECT_EQ(2, foo[1]);
  EXPECT_EQ(0, foo[2]);

  // Verify those results with IndexOf as well.
  EXPECT_EQ(0u, foo.IndexOf(1));
  EXPECT_EQ(1u, foo.IndexOf(2));
  EXPECT_EQ(2u, foo.IndexOf(0));
  EXPECT_FALSE(foo.Contains(98));
  EXPECT_EQ(foo.kIndexNone, foo.IndexOf(99));
}

TEST(UniqueVector, PushBackMove) {
  UniqueVector<std::string> vect;
  std::string a("a");
  EXPECT_TRUE(vect.push_back(std::move(a)));
  EXPECT_EQ("", a);

  a = "a";
  EXPECT_FALSE(vect.push_back(std::move(a)));
  EXPECT_EQ("a", a);

  EXPECT_EQ(0u, vect.IndexOf("a"));
}

TEST(UniqueVector, EmplaceBack) {
  UniqueVector<std::string> vect;
  EXPECT_TRUE(vect.emplace_back("a"));
  EXPECT_FALSE(vect.emplace_back("a"));
  EXPECT_EQ(1u, vect.size());
  EXPECT_TRUE(vect.emplace_back("b"));

  EXPECT_EQ(2u, vect.size());
  EXPECT_TRUE(vect.Contains(std::string("a")));
  EXPECT_TRUE(vect.Contains(std::string("b")));
}

static auto MakePair(bool first, size_t second) -> std::pair<bool, size_t> {
  return {first, second};
}

TEST(UniqueVector, PushBackWithIndex) {
  UniqueVector<int> foo;

  EXPECT_EQ(MakePair(true, 0u), foo.PushBackWithIndex(1));
  EXPECT_EQ(MakePair(false, 0u), foo.PushBackWithIndex(1));
  EXPECT_EQ(MakePair(true, 1u), foo.PushBackWithIndex(2));
  EXPECT_EQ(MakePair(true, 2u), foo.PushBackWithIndex(3));
  EXPECT_EQ(MakePair(false, 0u), foo.PushBackWithIndex(1));
  EXPECT_EQ(MakePair(false, 1u), foo.PushBackWithIndex(2));
  EXPECT_EQ(MakePair(false, 2u), foo.PushBackWithIndex(3));

  EXPECT_TRUE(foo.Contains(1));
  EXPECT_TRUE(foo.Contains(2));
  EXPECT_TRUE(foo.Contains(3));
  EXPECT_EQ(0u, foo.IndexOf(1));
  EXPECT_EQ(1u, foo.IndexOf(2));
  EXPECT_EQ(2u, foo.IndexOf(3));
  EXPECT_EQ(foo.kIndexNone, foo.IndexOf(98));
}

TEST(UniqueVector, PushBackMoveWithIndex) {
  UniqueVector<std::string> vect;
  std::string a("a");
  EXPECT_EQ(MakePair(true, 0), vect.PushBackWithIndex(std::move(a)));
  EXPECT_EQ("", a);

  a = "a";
  EXPECT_EQ(MakePair(false, 0), vect.PushBackWithIndex(std::move(a)));
  EXPECT_EQ("a", a);

  EXPECT_EQ(0u, vect.IndexOf("a"));
}

TEST(UniqueVector, EmplaceBackWithIndex) {
  UniqueVector<std::string> vect;
  EXPECT_EQ(MakePair(true, 0u), vect.EmplaceBackWithIndex("a"));
  EXPECT_EQ(MakePair(false, 0u), vect.EmplaceBackWithIndex("a"));
  EXPECT_EQ(1u, vect.size());

  EXPECT_EQ(MakePair(true, 1u), vect.EmplaceBackWithIndex("b"));
  EXPECT_EQ(2u, vect.size());

  EXPECT_TRUE(vect.Contains(std::string("a")));
  EXPECT_TRUE(vect.Contains(std::string("b")));
}

TEST(UniqueVector, Release) {
  UniqueVector<std::string> vect;
  EXPECT_TRUE(vect.emplace_back("a"));
  EXPECT_TRUE(vect.emplace_back("b"));
  EXPECT_TRUE(vect.emplace_back("c"));

  std::vector<std::string> v = vect.release();
  EXPECT_TRUE(vect.empty());
  EXPECT_FALSE(v.empty());

  EXPECT_FALSE(vect.Contains(std::string("a")));
  EXPECT_FALSE(vect.Contains(std::string("b")));
  EXPECT_FALSE(vect.Contains(std::string("a")));

  EXPECT_EQ(3u, v.size());
  EXPECT_EQ(std::string("a"), v[0]);
  EXPECT_EQ(std::string("b"), v[1]);
  EXPECT_EQ(std::string("c"), v[2]);
}
