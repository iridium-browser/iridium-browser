// Copyright 2019 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipp_attribute.h"
#include "frame.h"

#include <limits>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace ipp {

namespace {

void TestNewAttribute(Collection::iterator attr,
                      std::string_view name,
                      ValueTag tag) {
  EXPECT_EQ(attr->Name(), name);
  EXPECT_EQ(attr->Tag(), tag);
  // default state after creation
  if (IsOutOfBand(tag)) {
    EXPECT_EQ(attr->Size(), 0);
  } else {
    EXPECT_EQ(attr->Size(), 1);
  }
}

TEST(attribute, UnknownValueAttribute) {
  Collection coll;
  EXPECT_EQ(Code::kOK, coll.AddAttr("abc", ValueTag::nameWithLanguage,
                                    StringWithLanguage("val")));
  Collection::iterator attr = coll.GetAttr("abc");
  ASSERT_NE(attr, coll.end());
  TestNewAttribute(attr, "abc", ValueTag::nameWithLanguage);
  StringWithLanguage sl;
  EXPECT_EQ(attr->GetValue(0, sl), Code::kOK);
  EXPECT_EQ(sl.language, "");
  EXPECT_EQ(sl.value, "val");
}

TEST(attribute, UnknownCollectionAttribute) {
  Collection coll;
  CollsView::iterator new_coll;
  EXPECT_EQ(Code::kOK, coll.AddAttr("abcd", new_coll));
  Collection::iterator attr = coll.GetAttr("abcd");
  ASSERT_NE(attr, coll.end());
  EXPECT_EQ(attr->Colls().begin(), new_coll);
  TestNewAttribute(attr, "abcd", ValueTag::collection);
  EXPECT_EQ(attr->Colls().size(), 1);
  EXPECT_EQ(attr->Size(), 1);
  attr->Resize(3);
  EXPECT_EQ(attr->Colls().size(), 3);
  EXPECT_EQ(attr->Size(), 3);
  Collection::const_iterator attr_const = Collection::const_iterator(attr);
  EXPECT_EQ(attr_const->Colls().size(), 3);
  EXPECT_EQ(attr_const->Size(), 3);
}

TEST(attribute, FromStringToInt) {
  int val = 123456;
  // incorrect values: return false, no changes to val
  EXPECT_FALSE(FromString("123", static_cast<int*>(nullptr)));
  EXPECT_FALSE(FromString("12341s", &val));
  EXPECT_EQ(123456, val);
  EXPECT_FALSE(FromString("-", &val));
  EXPECT_EQ(123456, val);
  EXPECT_FALSE(FromString("", &val));
  EXPECT_EQ(123456, val);
  // correct values: return true
  EXPECT_TRUE(FromString("-239874", &val));
  EXPECT_EQ(-239874, val);
  EXPECT_TRUE(FromString("9238", &val));
  EXPECT_EQ(9238, val);
  EXPECT_TRUE(FromString("0", &val));
  EXPECT_EQ(0, val);
  const int int_min = std::numeric_limits<int>::min();
  const int int_max = std::numeric_limits<int>::max();
  EXPECT_TRUE(FromString(ToString(int_min), &val));
  EXPECT_EQ(int_min, val);
  EXPECT_TRUE(FromString(ToString(int_max), &val));
  EXPECT_EQ(int_max, val);
}

}  // end of namespace

}  // end of namespace ipp
