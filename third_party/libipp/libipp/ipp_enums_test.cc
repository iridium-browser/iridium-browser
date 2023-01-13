// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libipp/ipp_enums.h"

#include <gtest/gtest.h>

namespace ipp {

// Tests conversion between single enum's element and its string value.
// If string_value == "", the value has no string representation.
template <typename TEnum>
void TestEnumValue(TEnum enum_value, const std::string& string_value) {
  EXPECT_EQ(ToString(enum_value), string_value);
  TEnum out_value = static_cast<TEnum>(0xabcd);
  if (string_value.empty()) {
    EXPECT_FALSE(FromString(string_value, &out_value));
    // no changes to output parameter
    EXPECT_EQ(out_value, static_cast<TEnum>(0xabcd));
  } else {
    EXPECT_TRUE(FromString(string_value, &out_value));
    EXPECT_EQ(out_value, enum_value);
  }
}

// Tests the same as TestEnumValue plus two general functions:
// ToString(attr_name,value) and FromString(string,attr_name,&out_value).
template <typename TEnum>
void TestKeywordValue(AttrName attr_name,
                      TEnum enum_value,
                      const std::string& string_value) {
  TestEnumValue(enum_value, string_value);
  EXPECT_EQ(ToString(attr_name, static_cast<int>(enum_value)), string_value);
  int out_value = 123456789;
  if (string_value.empty()) {
    EXPECT_FALSE(FromString(string_value, attr_name, &out_value));
    // no changes to output parameter
    EXPECT_EQ(out_value, 123456789);
  } else {
    EXPECT_TRUE(FromString(string_value, attr_name, &out_value));
    EXPECT_EQ(out_value, static_cast<int>(enum_value));
  }
}

TEST(enums, GroupTag) {
  TestEnumValue(GroupTag::document_attributes, "document-attributes");
  TestEnumValue(GroupTag::unsupported_attributes, "unsupported-attributes");
  TestEnumValue(GroupTag::operation_attributes, "operation-attributes");
  TestEnumValue(GroupTag::system_attributes, "system-attributes");
  // value 3 corresponds to end-of-attributes-tag [rfc8010], it cannot be here
  TestEnumValue(static_cast<GroupTag>(3), "");
}

TEST(enums, AttrName) {
  TestEnumValue(AttrName::_unknown, "");
  TestEnumValue(AttrName::attributes_charset, "attributes-charset");
  TestEnumValue(AttrName::y_side2_image_shift_supported,
                "y-side2-image-shift-supported");
}

TEST(enums, KeywordsAndEnumerations) {
  TestKeywordValue(AttrName::auth_info_required, E_auth_info_required::domain,
                   "domain");
  TestKeywordValue(AttrName::auth_info_required, E_auth_info_required::username,
                   "username");
  TestKeywordValue(AttrName::current_page_order,
                   E_current_page_order::_1_to_n_order, "1-to-n-order");
  TestKeywordValue(AttrName::current_page_order,
                   E_current_page_order::n_to_1_order, "n-to-1-order");
  TestKeywordValue(AttrName::y_image_position_supported,
                   E_y_image_position_supported::bottom, "bottom");
  TestKeywordValue(AttrName::y_image_position_supported,
                   E_y_image_position_supported::top, "top");
}
}  // namespace ipp
