// Copyright 2023 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_PARSER_H_
#define LIBIPP_PARSER_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "errors.h"
#include "frame.h"

namespace ipp {

// The errors spotted by the parser. Comments next to the values describe
// actions taken by the parser.
enum class ParserCode : uint8_t {
  kOK = 0,
  kBooleanValueOutOfRange,            // the boolean value was set to 1
  kValueMismatchTagConverted,         // the value was converted
  kOutOfBandValueWithNonEmptyData,    // the data field was ignored
  kOutOfBandAttributeWithManyValues,  // additional values were ignored
  kValueMismatchTagOmitted,           // the value was omitted
  kUnsupportedValueTag,               // the value was omitted
  kValueInvalidSize,                  // the value was omitted
  kAttributeNoValues,                 // the attribute was omitted
  kAttributeNameConflict,             // the attribute was omitted
  kErrorWhenAddingAttribute,          // the attribute was omitted
  kErrorWhenAddingGroup,              // the group was omitted
  kFirstCritialError = 16,
  kAttributeNameIsEmpty = 16,        // the parser stopped
  kUnexpectedEndOfFrame,             // the parser stopped
  kGroupTagWasExpected,              // the parser stopped
  kEmptyNameExpectedInTNV,           // the parser stopped
  kEmptyValueExpectedInTNV,          // the parser stopped
  kNegativeNameLengthInTNV,          // the parser stopped
  kNegativeValueLengthInTNV,         // the parser stopped
  kTNVWithUnexpectedValueTag,        // the parser stopped
  kUnexpectedEndOfGroup,             // the parser stopped
  kLimitOnCollectionsLevelExceeded,  // the parser stopped
  kLimitOnGroupsCountExceeded,       // the parser stopped
};

// After spotting a critical error the parser cannot continue and will stop
// parsing before reaching the end of input frame.
constexpr bool IsCritical(ParserCode code) {
  return code >= ParserCode::kFirstCritialError;
}

// Returns a string representation of `code`. Returned string contains a name
// of corresponding enum's value and has no whitespaces.
LIBIPP_EXPORT std::string_view ToStrView(ParserCode code);

// Represents an error spotted by the parser when parsing an element pointed
// by `path`.
struct ParserError {
  AttrPath path;
  ParserCode code;
};

// Returns a one line string representation of the `error`. There is no EOL
// characters in the returned message.
LIBIPP_EXPORT std::string ToString(const ParserError& error);

// The interface of parser log.
class ParserLog {
 public:
  ParserLog() = default;
  ParserLog(const ParserLog&) = delete;
  ParserLog& operator=(const ParserLog&) = delete;
  virtual ~ParserLog() = default;
  // Reports an `error` when parsing an element pointed by `path`.
  // `IsCritical(error.code)` == true DOES NOT mean that this call is the last
  // one. Also, there may be more than one call with critical errors during
  // a single parser run.
  virtual void AddParserError(const ParserError& error) = 0;
};

// Simple implementation of the ParserLog interface. It just saves the first
// `max_entries_count` (see the constructor) parser errors.
class LIBIPP_EXPORT SimpleParserLog : public ParserLog {
 public:
  explicit SimpleParserLog(size_t max_entries_count = 100)
      : max_entries_count_(max_entries_count) {}
  void AddParserError(const ParserError& error) override;

  // Returns all errors added by AddParserError() in the same order they were
  // added. The log is truncated <=> the number of entries reached the value
  // `max_entries_count` passed to the constructor.
  const std::vector<ParserError>& Errors() const { return errors_; }
  // Returns all critical errors added by AddParserError() in the same order
  // they were added. The log is not truncated, but there is no more than a
  // couple of critical errors in a single parser run. All critical errors are
  // also included in Errors() (if it doesn't reach the limit).
  const std::vector<ParserError>& CriticalErrors() const {
    return critical_errors_;
  }

 private:
  const size_t max_entries_count_;
  std::vector<ParserError> errors_;
  std::vector<ParserError> critical_errors_;
};

// Parse the frame of `size` bytes saved in `buffer`. Errors detected by the
// parser are saved to `log`. If you use SimpleParserLog as `log` you can easily
// distinguish three cases:
//
// 1. When the parser completed parsing without errors then:
//     * log.Errors().empty() == true (=> log.CriticalErrors().empty() == true).
// 2. When the parser completed parsing with some non-critical errors then:
//     * log.Errors().empty() == false; AND
//     * log.CriticalErrors().empty() == true.
// 3. When the parser spotted a critical error and stopped then:
//     * log.Errors().empty() == false; AND
//     * log.CriticalErrors().empty() == false.
//
// In case 2, the output frame may have some values or attributes missing.
// In case 3, the output frame may cover only part of the input buffer or be
// empty and have all basic parameters set to zeroes like after Frame()
// constructor (it happens when nothing was parsed).
// In all cases, the returned object is consistent and can be passed to other
// libipp functions.
Frame LIBIPP_EXPORT Parse(const uint8_t* buffer, size_t size, ParserLog& log);

}  // namespace ipp

#endif  //  LIBIPP_PARSER_H_
