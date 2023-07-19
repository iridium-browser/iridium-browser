// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "parser.h"

#include <string>
#include <string_view>
#include <vector>

#include "errors.h"
#include "frame.h"
#include "ipp_frame.h"
#include "ipp_parser.h"

namespace ipp {

std::string_view ToStrView(ParserCode code) {
  switch (code) {
    case ParserCode::kOK:
      return "OK";
    case ParserCode::kBooleanValueOutOfRange:
      return "BooleanValueOutOfRange";
    case ParserCode::kValueMismatchTagConverted:
      return "ValueMismatchTagConverted";
    case ParserCode::kOutOfBandValueWithNonEmptyData:
      return "OutOfBandValueWithNonEmptyData";
    case ParserCode::kOutOfBandAttributeWithManyValues:
      return "OutOfBandAttributeWithManyValues";
    case ParserCode::kValueMismatchTagOmitted:
      return "ValueMismatchTagOmitted";
    case ParserCode::kUnsupportedValueTag:
      return "UnsupportedValueTag";
    case ParserCode::kValueInvalidSize:
      return "ValueInvalidSize";
    case ParserCode::kAttributeNoValues:
      return "AttributeNoValues";
    case ParserCode::kAttributeNameConflict:
      return "AttributeNameConflict";
    case ParserCode::kErrorWhenAddingAttribute:
      return "ErrorWhenAddingAttribute";
    case ParserCode::kErrorWhenAddingGroup:
      return "ErrorWhenAddingGroup";
    case ParserCode::kAttributeNameIsEmpty:
      return "AttributeNameIsEmpty";
    case ParserCode::kUnexpectedEndOfFrame:
      return "UnexpectedEndOfFrame";
    case ParserCode::kGroupTagWasExpected:
      return "GroupTagWasExpected";
    case ParserCode::kEmptyNameExpectedInTNV:
      return "EmptyNameExpectedInTNV";
    case ParserCode::kEmptyValueExpectedInTNV:
      return "EmptyValueExpectedInTNV";
    case ParserCode::kNegativeNameLengthInTNV:
      return "NegativeNameLengthInTNV";
    case ParserCode::kNegativeValueLengthInTNV:
      return "NegativeValueLengthInTNV";
    case ParserCode::kTNVWithUnexpectedValueTag:
      return "TNVWithUnexpectedValueTag";
    case ParserCode::kUnexpectedEndOfGroup:
      return "UnexpectedEndOfGroup";
    case ParserCode::kLimitOnCollectionsLevelExceeded:
      return "LimitOnCollectionsLevelExceeded";
    case ParserCode::kLimitOnGroupsCountExceeded:
      return "LimitOnGroupsCountExceeded";
  }
  return "UnknownParserCode";
}

std::string ToString(const ParserError& error) {
  std::string msg = error.path.AsString();
  msg += "; ";
  msg += ToStrView(error.code);
  return msg;
}

void SimpleParserLog::AddParserError(const ParserError& error) {
  const bool critical = IsCritical(error.code);
  if (errors_.size() < max_entries_count_) {
    errors_.emplace_back(error);
  }
  if (critical) {
    critical_errors_.emplace_back(error);
  }
}

Frame Parse(const uint8_t* buffer, size_t size, ParserLog& log) {
  Frame frame;
  if (buffer == nullptr) {
    size = 0;
  }
  std::vector<Log> log_temp;
  FrameData frame_data;
  Parser parser(&frame_data, &log_temp, log);
  parser.ReadFrameFromBuffer(buffer, buffer + size);
  parser.SaveFrameToPackage(false, &frame);
  frame.VersionNumber() = static_cast<Version>(frame_data.version_);
  frame.OperationIdOrStatusCode() = frame_data.operation_id_or_status_code_;
  frame.RequestId() = frame_data.request_id_;
  return frame;
}

}  // namespace ipp
