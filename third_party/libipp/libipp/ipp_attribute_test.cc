// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_attribute.h"

#include <limits>

#include <gtest/gtest.h>

#include "libipp/ipp_package.h"

namespace ipp {

static bool operator==(const ipp::RangeOfInteger& a,
                       const ipp::RangeOfInteger& b) {
  return (a.min_value == b.min_value && a.max_value == b.max_value);
}

namespace {

void TestNewAttribute(Attribute* attr,
                      const std::string& name,
                      bool is_a_set,
                      AttrType type) {
  EXPECT_EQ(attr->GetName(), name);
  if (attr->GetNameAsEnum() != AttrName::_unknown) {
    EXPECT_EQ(ToString(attr->GetNameAsEnum()), name);
  }
  EXPECT_EQ(attr->IsASet(), is_a_set);
  EXPECT_EQ(attr->GetType(), type);
  // default state after creation
  EXPECT_EQ(attr->GetState(), AttrState::unset);
  if (is_a_set) {
    EXPECT_EQ(attr->GetSize(), 0);
  } else {
    EXPECT_EQ(attr->GetSize(), 1);
  }
}

struct TestCollection : public Collection {
  SingleValue<bool> hi{AttrName::attributes_charset, AttrType::boolean};
  std::vector<Attribute*> GetKnownAttributes() override { return {&hi}; }
};

TEST(attribute, SingleValue) {
  SingleValue<int> attr(AttrName::job_id, AttrType::integer);
  TestNewAttribute(&attr, "job-id", false, AttrType::integer);
  attr.Set(123);
  EXPECT_EQ(attr.Get(), 123);
  EXPECT_EQ(attr.GetState(), AttrState::set);
}

TEST(attribute, SetOfValues) {
  SetOfValues<RangeOfInteger> attr(AttrName::punching_locations,
                                   AttrType::rangeOfInteger);
  TestNewAttribute(&attr, "punching-locations", true, AttrType::rangeOfInteger);
  RangeOfInteger r1{123, 234};
  RangeOfInteger r2{123, 234};
  RangeOfInteger r3{123, 234};
  attr.Set({r1, r2, r3});
  EXPECT_EQ(attr.Get(), std::vector<RangeOfInteger>({r1, r2, r3}));
  EXPECT_EQ(attr.GetState(), AttrState::set);
  EXPECT_EQ(attr.GetSize(), 3);
  attr.Resize(2);
  EXPECT_EQ(attr.Get(), std::vector<RangeOfInteger>({r1, r2}));
  EXPECT_EQ(attr.GetSize(), 2);
}

TEST(attribute, OpenSetOfValues) {
  OpenSetOfValues<int> attr(AttrName::auth_info, AttrType::integer);
  TestNewAttribute(&attr, "auth-info", true, AttrType::integer);
  attr.Set({11, 22, 33});
  attr.Add(std::vector<std::string>({"aaa", "bbb"}));
  attr.Add({44, 55});
  EXPECT_EQ(attr.Get(), std::vector<std::string>(
                            {"11", "22", "33", "aaa", "bbb", "44", "55"}));
  EXPECT_EQ(attr.GetState(), AttrState::set);
  EXPECT_EQ(attr.GetSize(), 7);
  attr.Resize(2);
  EXPECT_EQ(attr.Get(), std::vector<std::string>({"11", "22"}));
  EXPECT_EQ(attr.GetSize(), 2);
  attr.Set(std::vector<std::string>({"xx", "yy", "zz"}));
  EXPECT_EQ(attr.GetSize(), 3);
  EXPECT_EQ(attr.Get(), std::vector<std::string>({"xx", "yy", "zz"}));
}

TEST(attribute, UnknownValueAttribute) {
  UnknownValueAttribute attr("abc", false, AttrType::name);
  TestNewAttribute(&attr, "abc", false, AttrType::name);
  ASSERT_TRUE(attr.SetValue("val"));
  StringWithLanguage sl;
  ASSERT_TRUE(attr.GetValue(&sl));
  EXPECT_EQ(sl.language, "");
  EXPECT_EQ(sl.value, "val");
}

TEST(attribute, SingleCollection) {
  SingleCollection<TestCollection> attr(AttrName::baling);
  TestNewAttribute(&attr, "baling", false, AttrType::collection);
  EXPECT_EQ(attr.hi.GetState(), AttrState::unset);
  attr.hi.Set(false);
  EXPECT_EQ(attr.hi.Get(), false);
  EXPECT_EQ(attr.hi.GetState(), AttrState::set);
  EXPECT_EQ(attr.GetState(), AttrState::set);
}

TEST(attribute, SetOfCollections) {
  SetOfCollections<TestCollection> attr(AttrName::printer_supply_description);
  TestNewAttribute(&attr, "printer-supply-description", true,
                   AttrType::collection);
  attr[3].hi.SetState(AttrState::not_settable);
  EXPECT_EQ(attr.GetState(), AttrState::set);
  EXPECT_EQ(attr.GetSize(), 4);
  EXPECT_EQ(attr[3].hi.GetState(), AttrState::not_settable);
}

TEST(attribute, UnknownCollectionAttribute) {
  UnknownCollectionAttribute attr("abcd", true);
  TestNewAttribute(&attr, "abcd", true, AttrType::collection);
  EXPECT_EQ(attr.GetCollection(), nullptr);
  attr.Resize(3);
  EXPECT_NE(attr.GetCollection(), nullptr);
  EXPECT_NE(attr.GetCollection(2), nullptr);
  EXPECT_EQ(attr.GetCollection(3), nullptr);
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
