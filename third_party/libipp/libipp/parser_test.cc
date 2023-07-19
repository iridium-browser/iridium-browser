// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "parser.h"

#include <gtest/gtest.h>

#include "ipp_enums.h"

namespace ipp {
namespace {

TEST(SimpleParserLog, Empty) {
  SimpleParserLog log;
  EXPECT_TRUE(log.Errors().empty());
  EXPECT_TRUE(log.CriticalErrors().empty());
}

TEST(SimpleParserLog, AddParserError) {
  SimpleParserLog log;
  log.AddParserError(
      {AttrPath(GroupTag::printer_attributes), ParserCode::kValueInvalidSize});
  ASSERT_EQ(log.Errors().size(), 1);
  EXPECT_EQ(log.Errors()[0].path.AsString(), "printer-attributes");
  EXPECT_EQ(log.Errors()[0].code, ParserCode::kValueInvalidSize);
  EXPECT_TRUE(log.CriticalErrors().empty());
}

TEST(SimpleParserLog, AddParserErrorCritical) {
  SimpleParserLog log;
  log.AddParserError({AttrPath(GroupTag::printer_attributes),
                      ParserCode::kGroupTagWasExpected});
  ASSERT_EQ(log.Errors().size(), 1);
  EXPECT_EQ(log.Errors()[0].path.AsString(), "printer-attributes");
  EXPECT_EQ(log.Errors()[0].code, ParserCode::kGroupTagWasExpected);
  ASSERT_EQ(log.CriticalErrors().size(), 1);
  EXPECT_EQ(log.CriticalErrors()[0].path.AsString(), "printer-attributes");
  EXPECT_EQ(log.CriticalErrors()[0].code, ParserCode::kGroupTagWasExpected);
}

}  // namespace
}  // namespace ipp
