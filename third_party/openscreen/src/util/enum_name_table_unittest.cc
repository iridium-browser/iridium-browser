// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/enum_name_table.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {

namespace {

enum class TestEnum { kFoo = -1, kBar, kBaz = 100, kBuzz };

constexpr EnumNameTable<TestEnum, 4> kTestEnumNames{{
    {"foo", TestEnum::kFoo},
    {"bar", TestEnum::kBar},
    {"baz", TestEnum::kBaz},
    {"buzz", TestEnum::kBuzz},
}};

constexpr EnumNameTable<TestEnum, 1> kTestEnumNamesMissing{{
    {"foo", TestEnum::kFoo},
}};

constexpr EnumNameTable<TestEnum, 0> kTestEnumNamesEmpty{};

}  // namespace

TEST(EnumNameTable, GetEnumNameValid) {
  EXPECT_STREQ("foo", GetEnumName(kTestEnumNames, TestEnum::kFoo).value());
  EXPECT_STREQ("bar", GetEnumName(kTestEnumNames, TestEnum::kBar).value());
  EXPECT_STREQ("baz", GetEnumName(kTestEnumNames, TestEnum::kBaz).value());
  EXPECT_STREQ("buzz", GetEnumName(kTestEnumNames, TestEnum::kBuzz).value());
}

TEST(EnumNameTable, GetEnumNameMissing) {
  EXPECT_FALSE(GetEnumName(kTestEnumNamesMissing, TestEnum::kBar).is_value());
  EXPECT_FALSE(GetEnumName(kTestEnumNamesMissing, TestEnum::kBaz).is_value());
  EXPECT_FALSE(GetEnumName(kTestEnumNamesMissing, TestEnum::kBuzz).is_value());
}

TEST(EnumNameTable, GetEnumNameEmpty) {
  EXPECT_FALSE(GetEnumName(kTestEnumNamesEmpty, TestEnum::kBar).is_value());
  EXPECT_FALSE(GetEnumName(kTestEnumNamesEmpty, TestEnum::kBaz).is_value());
  EXPECT_FALSE(GetEnumName(kTestEnumNamesEmpty, TestEnum::kBuzz).is_value());
}

TEST(EnumNameTable, GetEnumValid) {
  EXPECT_EQ(TestEnum::kFoo, GetEnum(kTestEnumNames, "foo").value());
  EXPECT_EQ(TestEnum::kFoo, GetEnum(kTestEnumNames, "FOO").value());
  EXPECT_EQ(TestEnum::kBar, GetEnum(kTestEnumNames, "bar").value());
  EXPECT_EQ(TestEnum::kBar, GetEnum(kTestEnumNames, "baR").value());
  EXPECT_EQ(TestEnum::kBaz, GetEnum(kTestEnumNames, "baz").value());
  EXPECT_EQ(TestEnum::kBaz, GetEnum(kTestEnumNames, "Baz").value());
  EXPECT_EQ(TestEnum::kBuzz, GetEnum(kTestEnumNames, "buzz").value());
  EXPECT_EQ(TestEnum::kBuzz, GetEnum(kTestEnumNames, "bUZZ").value());
}

TEST(EnumNameTable, GetEnumMissing) {
  EXPECT_FALSE(GetEnum(kTestEnumNames, "").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNames, "1").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNames, "foobar").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesMissing, "bar").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesMissing, "baz").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesMissing, "buzz").is_value());
}

TEST(EnumNameTable, GetEnumEmpty) {
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "1").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "foobar").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "foo").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "bar").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "baz").is_value());
  EXPECT_FALSE(GetEnum(kTestEnumNamesEmpty, "buzz").is_value());
}

}  // namespace openscreen
