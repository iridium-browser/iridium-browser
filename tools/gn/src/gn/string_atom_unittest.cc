// Copyright (c) 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/string_atom.h"

#include "util/test/test.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <vector>

TEST(StringAtomTest, EmptyString) {
  StringAtom key1;
  StringAtom key2("");

  ASSERT_STREQ(key1.str().c_str(), "");
  ASSERT_STREQ(key2.str().c_str(), "");
  ASSERT_EQ(&key1.str(), &key2.str());
}

TEST(StringAtomTest, Find) {
  StringAtom empty;
  EXPECT_EQ(empty.str(), std::string());

  StringAtom foo("foo");
  EXPECT_EQ(foo.str(), std::string("foo"));

  StringAtom foo2("foo");
  EXPECT_EQ(&foo.str(), &foo2.str());
}

// Default compare should always be ordered.
TEST(StringAtomTest, DefaultCompare) {
  auto foo = StringAtom("foo");
  auto bar = StringAtom("bar");
  auto zoo = StringAtom("zoo");

  EXPECT_TRUE(bar < foo);
  EXPECT_TRUE(foo < zoo);
  EXPECT_TRUE(bar < zoo);
}

TEST(StringAtomTest, NormalSet) {
  std::set<StringAtom> set;
  auto foo_ret = set.insert(std::string_view("foo"));
  auto bar_ret = set.insert(std::string_view("bar"));
  auto zoo_ret = set.insert(std::string_view("zoo"));

  StringAtom foo_key("foo");
  EXPECT_EQ(*foo_ret.first, foo_key);

  auto foo_it = set.find(foo_key);
  EXPECT_NE(foo_it, set.end());
  EXPECT_EQ(*foo_it, foo_key);

  EXPECT_EQ(set.find(std::string_view("bar")), bar_ret.first);
  EXPECT_EQ(set.find(std::string_view("zoo")), zoo_ret.first);

  // Normal sets are always ordered according to the key value.
  auto it = set.begin();
  EXPECT_EQ(it, bar_ret.first);
  ++it;

  EXPECT_EQ(it, foo_ret.first);
  ++it;

  EXPECT_EQ(it, zoo_ret.first);
  ++it;

  EXPECT_EQ(it, set.end());
}

TEST(StringAtomTest, FastSet) {
  std::set<StringAtom, StringAtom::PtrCompare> set;

  auto foo_ret = set.insert(std::string_view("foo"));
  auto bar_ret = set.insert(std::string_view("bar"));
  auto zoo_ret = set.insert(std::string_view("zoo"));

  auto atom_to_ptr = [](const StringAtom& atom) -> const std::string* {
    return &atom.str();
  };

  EXPECT_TRUE(foo_ret.second);
  EXPECT_TRUE(bar_ret.second);
  EXPECT_TRUE(zoo_ret.second);

  const std::string* foo_ptr = atom_to_ptr(*foo_ret.first);
  const std::string* bar_ptr = atom_to_ptr(*bar_ret.first);
  const std::string* zoo_ptr = atom_to_ptr(*zoo_ret.first);

  StringAtom foo_key("foo");
  EXPECT_EQ(foo_ptr, atom_to_ptr(foo_key));

  auto foo_it = set.find(foo_key);
  EXPECT_NE(foo_it, set.end());
  EXPECT_EQ(*foo_it, foo_key);

  EXPECT_EQ(set.find(std::string_view("bar")), bar_ret.first);
  EXPECT_EQ(set.find(std::string_view("zoo")), zoo_ret.first);

  // Fast sets are ordered according to the key pointer.
  // Even though a bump allocator is used to allocate AtomString
  // strings, there is no guarantee that the global StringAtom
  // set was not already populated by a different test previously,
  // which means the pointers value need to be sorted before
  // iterating over the set for comparison.
  std::array<const std::string*, 3> ptrs = {
      foo_ptr,
      bar_ptr,
      zoo_ptr,
  };
  std::sort(ptrs.begin(), ptrs.end());

  auto it = set.begin();
  EXPECT_EQ(atom_to_ptr(*it), ptrs[0]);
  ++it;

  EXPECT_EQ(atom_to_ptr(*it), ptrs[1]);
  ++it;

  EXPECT_EQ(atom_to_ptr(*it), ptrs[2]);
  ++it;

  EXPECT_EQ(it, set.end());
}

TEST(StringAtom, AllocMoreThanASingleSlabOfKeys) {
  // Verify that allocating more than 128 string keys works properly.
  const size_t kMaxCount = 16384;
  std::vector<StringAtom> keys;

  // Small lambda to create a string for the n-th key.
  auto string_for = [](size_t index) -> std::string {
    return std::to_string(index) + "_key";
  };

  for (size_t nn = 0; nn < kMaxCount; ++nn) {
    keys.push_back(StringAtom(string_for(nn)));
  }

  for (size_t nn = 0; nn < kMaxCount; ++nn) {
    ASSERT_EQ(keys[nn].str(), string_for(nn));
  }
}
