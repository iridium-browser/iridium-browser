// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "attribute.h"

#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include "frame.h"
#include <gtest/gtest.h>

namespace ipp {
namespace {

class CollectionTest : public testing::Test {
 public:
  CollectionTest() : frame_(Operation::Print_Job, Version::_1_1, 1) {
    frame_.AddGroup(GroupTag::operation_attributes, coll_);
  }

 protected:
  Frame frame_;
  CollsView::iterator coll_;
};

TEST_F(CollectionTest, AddAttrOutOfBand) {
  auto err = coll_->AddAttr("test", ValueTag::not_settable);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::not_settable);
}

TEST_F(CollectionTest, AddAttrEnumAsInt) {
  auto err = coll_->AddAttr("test-enum", ValueTag::enum_, 1234);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test-enum");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::enum_);
  int value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value, 1234);
}

TEST_F(CollectionTest, AddAttrString) {
  auto err = coll_->AddAttr("abc123", ValueTag::mimeMediaType, "abc&123 DEF");
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("abc123");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::mimeMediaType);
  std::string value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value, "abc&123 DEF");
}

TEST_F(CollectionTest, AddAttrStringWithLanguage) {
  StringWithLanguage sl;
  sl.language = "lang_def";
  sl.value = "str value";
  auto err = coll_->AddAttr("lang", ValueTag::textWithLanguage, sl);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("lang");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::textWithLanguage);
  StringWithLanguage value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value.language, "lang_def");
  EXPECT_EQ(value.value, "str value");
}

TEST_F(CollectionTest, AddAttrBool) {
  auto err = coll_->AddAttr("test", true);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::boolean);
  int value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value, 1);
}

TEST_F(CollectionTest, AddAttrInteger) {
  auto err = coll_->AddAttr("test", -1234567890);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::integer);
  int32_t value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value, -1234567890);
}

TEST_F(CollectionTest, AddAttrDateTime) {
  DateTime dt;
  dt.year = 2034;
  dt.month = 6;
  dt.day = 23;
  dt.hour = 19;
  dt.minutes = 59;
  dt.deci_seconds = 7;
  dt.UTC_hours = 5;
  dt.UTC_minutes = 44;
  auto err = coll_->AddAttr("test", dt);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::dateTime);
  DateTime value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value.year, 2034);
  EXPECT_EQ(value.month, 6);
  EXPECT_EQ(value.day, 23);
  EXPECT_EQ(value.hour, 19);
  EXPECT_EQ(value.minutes, 59);
  EXPECT_EQ(value.seconds, 0);
  EXPECT_EQ(value.deci_seconds, 7);
  EXPECT_EQ(value.UTC_direction, '+');
  EXPECT_EQ(value.UTC_hours, 5);
  EXPECT_EQ(value.UTC_minutes, 44);
}

TEST_F(CollectionTest, AddAttrResolution) {
  Resolution res(123, 456, Resolution::Units::kDotsPerInch);
  auto err = coll_->AddAttr("test", res);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::resolution);
  Resolution value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value.xres, 123);
  EXPECT_EQ(value.yres, 456);
  EXPECT_EQ(value.units, Resolution::Units::kDotsPerInch);
}

TEST_F(CollectionTest, AddAttrRangeOfInteger) {
  RangeOfInteger roi(-123, 456);
  auto err = coll_->AddAttr("test", roi);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::rangeOfInteger);
  RangeOfInteger value;
  EXPECT_EQ(Code::kOK, attr->GetValue(0, value));
  EXPECT_EQ(value.min_value, -123);
  EXPECT_EQ(value.max_value, 456);
}

TEST_F(CollectionTest, AddAttrCollection) {
  CollsView::iterator attr_coll;
  auto err = coll_->AddAttr("test", attr_coll);
  EXPECT_EQ(err, Code::kOK);
  auto attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::collection);
  EXPECT_EQ(attr->Colls().begin(), attr_coll);
}

TEST_F(CollectionTest, AddAttrCollections) {
  CollsView colls;
  Code err = coll_->AddAttr("test", 3, colls);
  EXPECT_EQ(err, Code::kOK);
  EXPECT_EQ(colls.size(), 3);
  Collection::iterator attr = coll_->GetAttr("test");
  ASSERT_NE(attr, coll_->end());
  EXPECT_EQ(attr->Tag(), ValueTag::collection);
  EXPECT_EQ(attr->Colls().begin(), colls.begin());
}

TEST_F(CollectionTest, AddAttrInvalidName) {
  auto err = coll_->AddAttr("", true);
  EXPECT_EQ(err, Code::kInvalidName);
}

TEST_F(CollectionTest, AddAttrNameConflict) {
  auto err = coll_->AddAttr("test", true);
  EXPECT_EQ(err, Code::kOK);
  EXPECT_EQ(coll_->AddAttr("test", true), Code::kNameConflict);
  EXPECT_EQ(coll_->AddAttr("test", ValueTag::unknown), Code::kNameConflict);
}

TEST_F(CollectionTest, AddAttrValueOutOfRange) {
  auto err = coll_->AddAttr("aaa", ValueTag::boolean, -1);
  EXPECT_EQ(err, Code::kValueOutOfRange);
  EXPECT_EQ(coll_->GetAttr("aaa"), coll_->end());
}

TEST_F(CollectionTest, AddAttrInvalidValueTag) {
  auto err = coll_->AddAttr("xxx", static_cast<ValueTag>(0x0f));
  EXPECT_EQ(err, Code::kInvalidValueTag);
}

TEST_F(CollectionTest, AttributesOrder) {
  EXPECT_EQ(coll_->AddAttr("a3", true), Code::kOK);
  EXPECT_EQ(coll_->AddAttr("a1", false), Code::kOK);
  EXPECT_EQ(coll_->AddAttr("a5", 1234), Code::kOK);
  EXPECT_EQ(coll_->AddAttr("a4", ValueTag::no_value), Code::kOK);
  EXPECT_EQ(coll_->AddAttr("a2", ValueTag::uri, "abcde"), Code::kOK);
  Collection::const_iterator attr = coll_->cbegin();
  EXPECT_EQ(attr->Name(), "a3");
  ++attr;
  EXPECT_EQ(attr->Name(), "a1");
  ++attr;
  EXPECT_EQ(attr->Name(), "a5");
  ++attr;
  EXPECT_EQ(attr->Name(), "a4");
  ++attr;
  EXPECT_EQ(attr->Name(), "a2");
  ++attr;
  EXPECT_EQ(attr, coll_->cend());
}

TEST_F(CollectionTest, GetAttrFail) {
  Collection::iterator it = coll_->GetAttr("abc");
  EXPECT_EQ(it, coll_->end());
  Collection::const_iterator itc =
      static_cast<Collection::const_iterator>(coll_->GetAttr("abc"));
  EXPECT_EQ(itc, coll_->end());
  CollsView::const_iterator coll_const =
      static_cast<CollsView::const_iterator>(coll_);
  EXPECT_EQ(coll_const->GetAttr("abc"), coll_->end());
}

TEST_F(CollectionTest, GetAttrSuccess) {
  coll_->AddAttr("abc", std::vector{true, false});
  Collection::iterator it = coll_->GetAttr("abc");
  ASSERT_NE(it, coll_->end());
  EXPECT_EQ(it->Name(), "abc");
  EXPECT_EQ(it->Tag(), ValueTag::boolean);
  EXPECT_EQ(it->Size(), 2);
}

TEST_F(CollectionTest, GetAttrSuccessConst) {
  coll_->AddAttr("abc", std::vector{true, false});
  Collection::const_iterator it;
  it = coll_->GetAttr("abc");
  ASSERT_NE(it, coll_->end());
  EXPECT_EQ(it->Name(), "abc");
  EXPECT_EQ(it->Tag(), ValueTag::boolean);
  EXPECT_EQ(it->Size(), 2);
}

TEST_F(CollectionTest, IteratorsForEmptyCollection) {
  EXPECT_EQ(coll_->begin(), coll_->end());
  EXPECT_EQ(coll_->cbegin(), coll_->cend());
  CollsView::const_iterator coll_const =
      static_cast<CollsView::const_iterator>(coll_);
  EXPECT_EQ(coll_const->begin(), coll_const->end());
}

TEST_F(CollectionTest, IteratorsTwoElements) {
  coll_->AddAttr("test", ValueTag::not_settable);
  coll_->AddAttr("test2", ValueTag::keyword,
                 std::vector<std::string>{"ala", "ma", "kota"});
  std::vector<Collection::iterator> v;
  v.push_back(coll_->GetAttr("test"));
  v.push_back(coll_->GetAttr("test2"));
  size_t index = 0;
  for (Attribute& attr : *coll_) {
    ASSERT_LE(index, v.size());
    EXPECT_EQ(&attr, &*v[index]);
    ++index;
  }
  EXPECT_EQ(index, v.size());
}

TEST_F(CollectionTest, IteratorsTwoElementsConst) {
  coll_->AddAttr("test", ValueTag::not_settable);
  coll_->AddAttr("test2", ValueTag::keyword,
                 std::vector<std::string>{"ala", "ma", "kota"});
  const Collection& coll_const = *coll_;
  std::vector<Collection::const_iterator> v;
  v.push_back(coll_const.GetAttr("test"));
  v.push_back(coll_const.GetAttr("test2"));
  size_t index = 0;
  for (const Attribute& attr : coll_const) {
    ASSERT_LE(index, v.size());
    EXPECT_EQ(&attr, &*v[index]);
    ++index;
  }
  EXPECT_EQ(index, v.size());
}

TEST_F(CollectionTest, IteratorTraits) {
  using it_traits = std::iterator_traits<Collection::iterator>;
  EXPECT_TRUE((std::is_same<it_traits::iterator_category,
                            std::bidirectional_iterator_tag>::value));
  EXPECT_TRUE((std::is_same<it_traits::value_type, Attribute>::value));
  EXPECT_TRUE((std::is_same<it_traits::difference_type, int>::value));
  EXPECT_TRUE((std::is_same<it_traits::pointer, Attribute*>::value));
  EXPECT_TRUE((std::is_same<it_traits::reference, Attribute&>::value));
  using itc_traits = std::iterator_traits<Collection::const_iterator>;
  EXPECT_TRUE((std::is_same<itc_traits::iterator_category,
                            std::bidirectional_iterator_tag>::value));
  EXPECT_TRUE((std::is_same<itc_traits::value_type, const Attribute>::value));
  EXPECT_TRUE((std::is_same<itc_traits::difference_type, int>::value));
  EXPECT_TRUE((std::is_same<itc_traits::pointer, const Attribute*>::value));
  EXPECT_TRUE((std::is_same<itc_traits::reference, const Attribute&>::value));
}

class AttributeTest : public testing::Test {
 public:
  AttributeTest() { frame_.AddGroup(GroupTag::operation_attributes, coll_); }

 protected:
  Frame frame_;
  CollsView::iterator coll_;
};

TEST_F(AttributeTest, CollsWrongType) {
  coll_->AddAttr("out-of-band", ValueTag::not_settable);
  Attribute& attr = *coll_->GetAttr("out-of-band");
  EXPECT_TRUE(attr.Colls().empty());
  EXPECT_EQ(attr.Colls().size(), 0);
  const Attribute& attr_const = attr;
  EXPECT_TRUE(attr_const.Colls().empty());
  EXPECT_EQ(attr_const.Colls().size(), 0);
}

TEST_F(AttributeTest, AddAttrWithLongName) {
  Code code = coll_->AddAttr(std::string(32768, 'x'), ValueTag::no_value);
  EXPECT_EQ(code, Code::kInvalidName);
  code = coll_->AddAttr(std::string(32767, 'x'), ValueTag::no_value);
  EXPECT_EQ(code, Code::kOK);
}

TEST_F(AttributeTest, AddAttrWithLongString) {
  Code code = coll_->AddAttr("max_length", ValueTag::octetString,
                             std::string(32767, 'x'));
  EXPECT_EQ(code, Code::kOK);
  code = coll_->AddAttr("too_large", ValueTag::octetString,
                        std::string(32768, 'x'));
  EXPECT_EQ(code, Code::kValueOutOfRange);
  Collection::iterator it = coll_->begin();
  ASSERT_NE(it, coll_->end());
  EXPECT_EQ(it->Name(), "max_length");
  ++it;
  EXPECT_EQ(it, coll_->end());
}

TEST_F(AttributeTest, SetValueLongString) {
  coll_->AddAttr("test", ValueTag::nameWithoutLanguage, "");
  Collection::iterator it = coll_->GetAttr("test");
  ASSERT_NE(it, coll_->end());
  EXPECT_EQ(it->SetValues(std::string(32768, 'x')), Code::kValueOutOfRange);
  EXPECT_EQ(it->SetValues(std::string(32767, 'x')), Code::kOK);
  EXPECT_EQ(
      it->SetValues(std::vector<std::string>{"", std::string(32768, 'x')}),
      Code::kValueOutOfRange);
  EXPECT_EQ(
      it->SetValues(std::vector<std::string>{"", std::string(32767, 'x')}),
      Code::kOK);
}

TEST_F(AttributeTest, AddAttrWithLongStringWithLanguage) {
  StringWithLanguage strlang_ok = {std::string(32763, 'x'), ""};
  StringWithLanguage strlang_too_long = {std::string(32760, 'x'),
                                         std::string(4, 'x')};
  Code code =
      coll_->AddAttr("max_length", ValueTag::nameWithLanguage, strlang_ok);
  EXPECT_EQ(code, Code::kOK);
  code =
      coll_->AddAttr("too_large", ValueTag::nameWithLanguage, strlang_too_long);
  EXPECT_EQ(code, Code::kValueOutOfRange);
}

TEST_F(AttributeTest, SetValueLongStringWithLanguage) {
  coll_->AddAttr("test", ValueTag::textWithLanguage,
                 std::vector<StringWithLanguage>(2));
  Collection::iterator it = coll_->GetAttr("test");
  ASSERT_NE(it, coll_->end());
  StringWithLanguage strlang_ok = {"", std::string(32763, 'x')};
  StringWithLanguage strlang_too_long = {std::string(4, 'x'),
                                         std::string(32760, 'x')};
  EXPECT_EQ(it->SetValues(strlang_too_long), Code::kValueOutOfRange);
  EXPECT_EQ(it->SetValues(strlang_ok), Code::kOK);
  EXPECT_EQ(it->SetValues(
                std::vector<StringWithLanguage>{strlang_ok, strlang_too_long}),
            Code::kValueOutOfRange);
  EXPECT_EQ(
      it->SetValues(std::vector<StringWithLanguage>{strlang_ok, strlang_ok}),
      Code::kOK);
}

class AttributeValuesTest : public AttributeTest {
 public:
  AttributeValuesTest() {
    coll_->AddAttr("out_of_band", ValueTag::not_settable);
    coll_->AddAttr("bool", ValueTag::boolean, true);
    coll_->AddAttr("int32", ValueTag::integer, 123);
    coll_->AddAttr("string", ValueTag::octetString, "str");
    coll_->AddAttr("name", ValueTag::nameWithoutLanguage, "name");
    coll_->AddAttr("string_lang", ValueTag::nameWithLanguage,
                   StringWithLanguage("val", "lang"));
    coll_->AddAttr("date_time", ValueTag::dateTime, DateTime());
    coll_->AddAttr("resolution", ValueTag::resolution, Resolution{123, 456});
    coll_->AddAttr("range", ValueTag::rangeOfInteger, RangeOfInteger(0, 2));
    attr_out_of_band_ = coll_->GetAttr("out_of_band");
    attr_bool_ = coll_->GetAttr("bool");
    attr_int32_ = coll_->GetAttr("int32");
    attr_string_ = coll_->GetAttr("string");
    attr_name_ = coll_->GetAttr("name");
    attr_string_lang_ = coll_->GetAttr("string_lang");
    attr_date_time_ = coll_->GetAttr("date_time");
    attr_resolution_ = coll_->GetAttr("resolution");
    attr_range_ = coll_->GetAttr("range");
  }

 protected:
  template <typename ApiType>
  void CompareGetValueWithGetValues();

  Collection::iterator attr_out_of_band_;
  Collection::iterator attr_bool_;
  Collection::iterator attr_int32_;
  Collection::iterator attr_name_;
  Collection::iterator attr_string_;
  Collection::iterator attr_string_lang_;
  Collection::iterator attr_date_time_;
  Collection::iterator attr_resolution_;
  Collection::iterator attr_range_;
};

TEST_F(AttributeValuesTest, GetValuesVectorBool) {
  std::vector<bool> v;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, std::vector<bool>{true});
}

TEST_F(AttributeValuesTest, GetValuesVectorInt32) {
  std::vector<int32_t> v;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kOK);
  EXPECT_EQ(v, std::vector<int32_t>{1});
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, std::vector<int32_t>{123});
}

TEST_F(AttributeValuesTest, GetValuesVectorString) {
  std::vector<std::string> v, v2;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_->GetValues(v2), Code::kOK);
  EXPECT_EQ(attr_string_lang_->GetValues(v2), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, std::vector<std::string>{"name"});
  EXPECT_EQ(v2, std::vector<std::string>{"str"});
}

TEST_F(AttributeValuesTest, GetValuesVectorStringWithLanguage) {
  std::vector<StringWithLanguage> v, v2;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v2), Code::kOK);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, std::vector<StringWithLanguage>{StringWithLanguage("name", "")});
  EXPECT_EQ(v2,
            std::vector<StringWithLanguage>{StringWithLanguage("val", "lang")});
}

TEST_F(AttributeValuesTest, GetValuesVectorDateTime) {
  std::vector<DateTime> v;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, std::vector<DateTime>(1));
}

TEST_F(AttributeValuesTest, GetValuesVectorResolution) {
  std::vector<Resolution> v;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_range_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(v, (std::vector<Resolution>{Resolution{123, 456}}));
}

TEST_F(AttributeValuesTest, GetValuesVectorRangeOfInteger) {
  std::vector<RangeOfInteger> v, v2;
  EXPECT_EQ(attr_out_of_band_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->GetValues(v), Code::kOK);
  EXPECT_EQ(attr_name_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->GetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v, (std::vector<RangeOfInteger>{RangeOfInteger(123, 123)}));
  EXPECT_EQ(v2, (std::vector<RangeOfInteger>{RangeOfInteger(0, 2)}));
}

template <typename ApiType>
void AttributeValuesTest::CompareGetValueWithGetValues() {
  const std::vector<ipp::Collection::iterator> attrs = {
      attr_out_of_band_, attr_bool_,       attr_int32_,
      attr_name_,        attr_string_,     attr_string_lang_,
      attr_date_time_,   attr_resolution_, attr_range_};
  for (ipp::Collection::iterator attr : attrs) {
    ApiType x;
    std::vector<ApiType> vx;
    ipp::Code code = attr->GetValue(0, x);
    ipp::Code vcode = attr->GetValues(vx);
    EXPECT_EQ(code, vcode);
    if (vcode == ipp::Code::kOK) {
      EXPECT_EQ(x, vx[0]);
    }
  }
}

TEST_F(AttributeValuesTest, GetValueBool) {
  CompareGetValueWithGetValues<bool>();
}

TEST_F(AttributeValuesTest, GetValueInt32) {
  CompareGetValueWithGetValues<int32_t>();
}

TEST_F(AttributeValuesTest, GetValueString) {
  CompareGetValueWithGetValues<std::string>();
}

TEST_F(AttributeValuesTest, GetValueStringWithLanguage) {
  CompareGetValueWithGetValues<StringWithLanguage>();
}

TEST_F(AttributeValuesTest, GetValueDateTime) {
  CompareGetValueWithGetValues<DateTime>();
}

TEST_F(AttributeValuesTest, GetValueResolution) {
  CompareGetValueWithGetValues<Resolution>();
}

TEST_F(AttributeValuesTest, GetValueRangeOfInteger) {
  CompareGetValueWithGetValues<RangeOfInteger>();
}

TEST_F(AttributeValuesTest, SetValuesBool) {
  const bool v = true;
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<bool> v2;
  EXPECT_EQ(attr_bool_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<bool>{v});
}

TEST_F(AttributeValuesTest, SetValuesInt32) {
  const int32_t v = 1234;
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kValueOutOfRange);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<int32_t> v2;
  EXPECT_EQ(attr_int32_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<int32_t>{v});
}

TEST_F(AttributeValuesTest, SetValuesString) {
  const std::string v = "test";
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<std::string> v2;
  EXPECT_EQ(attr_string_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<std::string>{v});
}

TEST_F(AttributeValuesTest, SetValuesStringWithLanguage) {
  const StringWithLanguage v = {"testval", "testlang"};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<StringWithLanguage> v2;
  EXPECT_EQ(attr_string_lang_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<StringWithLanguage>{v});
}

TEST_F(AttributeValuesTest, SetValuesDateTime) {
  const DateTime v = {2022, 1, 2, 3, 4, 5};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<DateTime> v2;
  EXPECT_EQ(attr_date_time_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<DateTime>{v});
}

TEST_F(AttributeValuesTest, SetValuesResolution) {
  const Resolution v = {12, 34, Resolution::Units::kDotsPerCentimeter};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<Resolution> v2;
  EXPECT_EQ(attr_resolution_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<Resolution>{v});
}

TEST_F(AttributeValuesTest, SetValuesRangeOfInteger) {
  const RangeOfInteger v = {12, 34};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kOK);
  std::vector<RangeOfInteger> v2;
  EXPECT_EQ(attr_range_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, std::vector<RangeOfInteger>{v});
}

TEST_F(AttributeValuesTest, SetValuesVectorBool) {
  const std::vector<bool> v = {true, false, true};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<bool> v2;
  EXPECT_EQ(attr_bool_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorInt32) {
  const std::vector<int32_t> v = {1, 2, 3, 4};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kValueOutOfRange);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<int32_t> v2;
  EXPECT_EQ(attr_int32_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorString) {
  const std::vector<std::string> v = {"test1", "test2", "test3"};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<std::string> v2;
  EXPECT_EQ(attr_string_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorStringWithLanguage) {
  const std::vector<StringWithLanguage> v = {{"v1", "l1"}, {"v2", "l2"}};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<StringWithLanguage> v2;
  EXPECT_EQ(attr_string_lang_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorDateTime) {
  const std::vector<DateTime> v = {{2022, 1, 2}, {2021, 3, 4}, {2023, 7, 8}};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<DateTime> v2;
  EXPECT_EQ(attr_date_time_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorResolution) {
  const std::vector<Resolution> v = {{12, 34}, {56, 78}, {90, 11}};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kOK);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kIncompatibleType);
  std::vector<Resolution> v2;
  EXPECT_EQ(attr_resolution_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST_F(AttributeValuesTest, SetValuesVectorRangeOfInteger) {
  const std::vector<RangeOfInteger> v = {{0, 0}, {12, 34}, {999, 33}};
  EXPECT_EQ(attr_out_of_band_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_bool_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_int32_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_string_lang_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_date_time_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_resolution_->SetValues(v), Code::kIncompatibleType);
  EXPECT_EQ(attr_range_->SetValues(v), Code::kOK);
  std::vector<RangeOfInteger> v2;
  EXPECT_EQ(attr_range_->GetValues(v2), Code::kOK);
  EXPECT_EQ(v2, v);
}

TEST(ToStrView, ValueTag) {
  EXPECT_EQ(ToStrView(ValueTag::keyword), "keyword");
  EXPECT_EQ(ToStrView(ValueTag::delete_attribute), "delete-attribute");
  EXPECT_EQ(ToStrView(ValueTag::enum_), "enum");
  EXPECT_EQ(ToStrView(ValueTag::nameWithoutLanguage), "nameWithoutLanguage");
}

}  // namespace
}  // namespace ipp
