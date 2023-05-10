// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "validator.h"

#include <string>
#include <vector>

#include "attribute.h"
#include "errors.h"
#include "frame.h"
#include <gtest/gtest.h>

namespace ipp {
namespace {

class ValidatorTest : public testing::Test {
 protected:
  SimpleValidatorLog log_;
  Frame frame_ = Frame(Operation::Activate_Printer);
  CollsView::iterator grp_ =
      frame_.Groups(GroupTag::operation_attributes).begin();
};

TEST_F(ValidatorTest, NoErrors) {
  EXPECT_TRUE(Validate(frame_, log_));
  EXPECT_TRUE(log_.Entries().empty());
}

TEST_F(ValidatorTest, InvalidHeader) {
  frame_.OperationIdOrStatusCode() = -1;
  frame_.VersionNumber() = static_cast<Version>(0x1234);
  frame_.RequestId() = -1;

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 4);
  EXPECT_EQ(log_.Entries()[0].path.AsString(),
            "header[0]>major-version-number");
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kIntegerOutOfRange}));
  EXPECT_EQ(log_.Entries()[1].path.AsString(),
            "header[0]>minor-version-number");
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kIntegerOutOfRange}));
  EXPECT_EQ(log_.Entries()[2].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[2].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kIntegerOutOfRange}));
  EXPECT_EQ(log_.Entries()[3].path.AsString(), "header[0]>request-id");
  EXPECT_EQ(log_.Entries()[3].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[3].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kIntegerOutOfRange}));
}

TEST_F(ValidatorTest, InvalidAttributeName) {
  const std::string too_long = std::string(kMaxLengthOfName + 1, 'x');
  const std::string too_long_2 = std::string(kMaxLengthOfName + 1, '%');
  const std::string just_ok = std::string(kMaxLengthOfName, 'X');
  EXPECT_EQ(Code::kOK, grp_->AddAttr("invalid char", ValueTag::integer, 123));
  EXPECT_EQ(Code::kOK, grp_->AddAttr(too_long, ValueTag::integer, 123));
  EXPECT_EQ(Code::kOK, grp_->AddAttr(too_long_2, ValueTag::integer, 123));
  EXPECT_EQ(Code::kOK, grp_->AddAttr(just_ok, ValueTag::integer, 123));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 3);
  EXPECT_EQ(log_.Entries()[0].path.AsString(),
            "operation-attributes[0]>invalid char");
  EXPECT_TRUE(log_.Entries()[0].error.IsInTheName());
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kStringInvalidCharacter});
  EXPECT_EQ(log_.Entries()[1].path.AsString(),
            "operation-attributes[0]>" + too_long);
  EXPECT_TRUE(log_.Entries()[1].error.IsInTheName());
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kStringTooLong});
  EXPECT_EQ(log_.Entries()[2].path.AsString(),
            "operation-attributes[0]>" + too_long_2);
  EXPECT_TRUE(log_.Entries()[2].error.IsInTheName());
  EXPECT_EQ(log_.Entries()[2].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong,
                         ValidatorCode::kStringInvalidCharacter}));
}

TEST_F(ValidatorTest, InvalidOctetString) {
  EXPECT_EQ(Code::kOK,
            grp_->AddAttr("can-be-empty", ValueTag::octetString, ""));
  grp_->AddAttr("too-long", ValueTag::octetString,
                std::vector{std::string(kMaxLengthOfOctetString, ' '),
                            std::string(kMaxLengthOfOctetString + 1, 'x')});

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 1);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 1);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kStringTooLong});
}

TEST_F(ValidatorTest, InvalidDateTime) {
  DateTime dt = {.year = 2022,
                 .month = 11,
                 .day = 30,
                 .hour = 17,
                 .minutes = 32,
                 .seconds = 56,
                 .deci_seconds = 2,
                 .UTC_direction = '+',
                 .UTC_hours = 2,
                 .UTC_minutes = 30};
  DateTime bad_date = dt;
  bad_date.month = 2;
  DateTime bad_time = dt;
  bad_time.hour = 24;
  DateTime bad_zone = dt;
  bad_zone.UTC_direction = ' ';
  DateTime all_wrong = dt;
  all_wrong.day = 0;
  all_wrong.minutes = 60;
  all_wrong.UTC_minutes = 60;
  EXPECT_EQ(Code::kOK,
            grp_->AddAttr("date-time", ValueTag::dateTime,
                          {dt, bad_date, bad_time, bad_zone, all_wrong}));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 4);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 1);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kDateTimeInvalidDate});
  EXPECT_EQ(log_.Entries()[1].error.Index(), 2);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kDateTimeInvalidTimeOfDay});
  EXPECT_EQ(log_.Entries()[2].error.Index(), 3);
  EXPECT_EQ(log_.Entries()[2].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kDateTimeInvalidZone});
  EXPECT_EQ(log_.Entries()[3].error.Index(), 4);
  EXPECT_EQ(log_.Entries()[3].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kDateTimeInvalidDate,
                         ValidatorCode::kDateTimeInvalidTimeOfDay,
                         ValidatorCode::kDateTimeInvalidZone}));
}

TEST_F(ValidatorTest, InvalidDateTimeLeapYear) {
  DateTime leap1 = {.year = 2000, .month = 2, .day = 29};
  DateTime leap2 = {.year = 2096, .month = 2, .day = 29};
  DateTime notleap1 = {.year = 2100, .month = 2, .day = 29};
  DateTime notleap2 = {.year = 2001, .month = 2, .day = 29};
  EXPECT_EQ(Code::kOK, grp_->AddAttr("date-time", ValueTag::dateTime,
                                     {leap1, leap2, notleap1, notleap2}));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 2);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kDateTimeInvalidDate});
  EXPECT_EQ(log_.Entries()[1].error.Index(), 3);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kDateTimeInvalidDate});
}

TEST_F(ValidatorTest, InvalidResolution) {
  Resolution bad_dim = {/*xres=*/0, /*yres=*/2};
  Resolution good = {/*xres=*/1, /*yres=*/1};
  Resolution all_wrong = {/*xres=*/0, /*yres=*/0,
                          /*units=*/static_cast<Resolution::Units>(1)};

  EXPECT_EQ(Code::kOK,
            grp_->AddAttr("BAD!", ValueTag::resolution, {all_wrong, bad_dim}));
  EXPECT_EQ(Code::kOK, grp_->AddAttr("good", ValueTag::resolution, good));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 3);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0xffffu);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kStringInvalidCharacter});
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kResolutionInvalidUnit,
                         ValidatorCode::kResolutionInvalidDimension}));
  EXPECT_EQ(log_.Entries()[2].error.Index(), 1);
  EXPECT_EQ(log_.Entries()[2].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kResolutionInvalidDimension});
}

TEST_F(ValidatorTest, InvalidRangeOfInteger) {
  RangeOfInteger good1 = {/*min_value=*/-123, /*max_value=*/23456};
  RangeOfInteger wrong = {/*min_value=*/123, /*max_value=*/122};
  RangeOfInteger good2 = {/*min_value=*/-12, /*max_value=*/-12};

  EXPECT_EQ(Code::kOK, grp_->AddAttr("range", ValueTag::rangeOfInteger,
                                     {good1, wrong, good2}));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 1);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 1);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            std::vector{ValidatorCode::kRangeOfIntegerMaxLessMin});
}

TEST_F(ValidatorTest, InvalidCollection) {
  CollsView colls;
  ASSERT_EQ(Code::kOK, grp_->AddAttr("colls", 3, colls));
  CollsView::iterator coll2;
  ASSERT_EQ(Code::kOK, colls[1].AddAttr("coll2", coll2));
  EXPECT_EQ(Code::kOK, colls[0].AddAttr("good-attr", true));
  EXPECT_EQ(Code::kOK, coll2->AddAttr("bad attr", ValueTag::not_settable));
  EXPECT_EQ(Code::kOK, colls[2].AddAttr("-also bad", ValueTag::enum_, 123));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].path.AsString(),
            "operation-attributes[0]>colls[1]>coll2[0]>bad attr");
  EXPECT_TRUE(log_.Entries()[0].error.IsInTheName());
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringInvalidCharacter}));
  EXPECT_EQ(log_.Entries()[1].path.AsString(),
            "operation-attributes[0]>colls[2]>-also bad");
  EXPECT_TRUE(log_.Entries()[1].error.IsInTheName());
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringInvalidCharacter}));
}

TEST_F(ValidatorTest, InvalidText) {
  grp_->AddAttr("too-long", ValueTag::textWithoutLanguage,
                std::string(kMaxLengthOfText + 1, 'x'));
  grp_->AddAttr("bad-charset", ValueTag::textWithLanguage,
                StringWithLanguage(/*value=*/"1\n\t X", /*language=*/"ABC"));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringWithLangInvalidLanguage}));
}

TEST_F(ValidatorTest, InvalidName) {
  grp_->AddAttr("too-long", ValueTag::nameWithoutLanguage,
                std::string(kMaxLengthOfName + 1, 'x'));
  EXPECT_EQ(Code::kOK, grp_->AddAttr("bad-charset", ValueTag::nameWithLanguage,
                                     StringWithLanguage(/*value=*/"1\n\t X",
                                                        /*language=*/"en-us")));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 1);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
}

TEST_F(ValidatorTest, InvalidUri) {
  grp_->AddAttr("too-long", ValueTag::uri,
                std::string(kMaxLengthOfUri + 1, 'x'));
  grp_->AddAttr("empty", ValueTag::uri, std::string(""));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringEmpty}));
}

TEST_F(ValidatorTest, InvalidUriScheme) {
  grp_->AddAttr("too-long", ValueTag::uriScheme,
                std::string(kMaxLengthOfUriScheme + 1, 'x'));
  grp_->AddAttr("empty", ValueTag::uriScheme, std::string(""));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringEmpty}));
}

TEST_F(ValidatorTest, InvalidCharset) {
  grp_->AddAttr("non-printable", ValueTag::charset, "eol:\n");
  grp_->AddAttr("uppercase", ValueTag::charset, "uppercase: A");

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].path.AsString(),
            "operation-attributes[0]>non-printable");
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringInvalidCharacter}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].path.AsString(),
            "operation-attributes[0]>uppercase");
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringInvalidCharacter}));
}

TEST_F(ValidatorTest, InvalidNaturalLanguage) {
  grp_->AddAttr("too-long", ValueTag::naturalLanguage,
                std::string(kMaxLengthOfNaturalLanguage + 1, 'x'));
  grp_->AddAttr("empty", ValueTag::naturalLanguage, std::string(""));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringEmpty}));
}

TEST_F(ValidatorTest, InvalidMimeMediaType) {
  grp_->AddAttr("too-long", ValueTag::mimeMediaType,
                std::string(kMaxLengthOfMimeMediaType + 1, 'x'));
  grp_->AddAttr("empty", ValueTag::mimeMediaType, std::string(""));

  EXPECT_FALSE(Validate(frame_, log_));
  ASSERT_EQ(log_.Entries().size(), 2);
  EXPECT_EQ(log_.Entries()[0].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[0].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringTooLong}));
  EXPECT_EQ(log_.Entries()[1].error.Index(), 0);
  EXPECT_EQ(log_.Entries()[1].error.ErrorsAsVector(),
            (std::vector{ValidatorCode::kStringEmpty}));
}

TEST_F(ValidatorTest, StopAfterTheFirstError) {
  grp_->AddAttr("too-long", ValueTag::mimeMediaType,
                std::string(kMaxLengthOfMimeMediaType + 1, 'x'));
  grp_->AddAttr("too-long-2", ValueTag::mimeMediaType,
                std::string(kMaxLengthOfMimeMediaType + 1, 'x'));

  SimpleValidatorLog log_first_error(1);
  EXPECT_FALSE(Validate(frame_, log_first_error));
  ASSERT_EQ(log_first_error.Entries().size(), 1);
}

}  // namespace
}  // namespace ipp
