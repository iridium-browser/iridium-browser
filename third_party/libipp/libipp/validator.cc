// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "validator.h"

#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "attribute.h"
#include "errors.h"
#include "frame.h"

namespace ipp {
namespace {

constexpr std::string_view kAllowedCharsInKeyword = "-_.";
constexpr std::string_view kAllowedCharsInUri = ":/?#[]@!$&'()*+,;=-._~%";
constexpr std::string_view kAllowedCharsInUriScheme = "+-.";
constexpr std::string_view kAllowedCharsInNaturalLanguage = "-";

bool IsLowercaseLetter(char c) {
  return c >= 'a' && c <= 'z';
}

bool IsUppercaseLetter(char c) {
  return c >= 'A' && c <= 'Z';
}

bool IsDigit(char c) {
  return c >= '1' && c <= '9';
}

// Helper struct for string validation.
struct StringValidator {
  // The input string to validate.
  std::string_view value;
  // Output set of error codes.
  std::set<ValidatorCode> codes;
  // Validates the string length.
  void CheckLength(size_t max_length, bool empty_string_allowed = false) {
    if (value.empty()) {
      if (!empty_string_allowed) {
        codes.insert(ValidatorCode::kStringEmpty);
      }
      return;
    }
    if (value.size() > max_length) {
      codes.insert(ValidatorCode::kStringTooLong);
    }
  }
  // Checks if the string starts from lowercase letter. It does nothing if
  // the input string is empty.
  void CheckFirstLetterIsLowercase() {
    if (value.empty() || IsLowercaseLetter(value.front()))
      return;
    codes.insert(ValidatorCode::kStringMustStartLowercaseLetter);
  }
  // Checks if the input string consists only of letters, digits and
  // characters from `allowed_chars`.
  void CheckLettersDigits(std::string_view allowed_chars,
                          bool uppercase_letters_allowed = false) {
    for (char c : value) {
      if (IsLowercaseLetter(c))
        continue;
      if (uppercase_letters_allowed && IsUppercaseLetter(c))
        continue;
      if (IsDigit(c))
        continue;
      if (allowed_chars.find(c) == std::string_view::npos) {
        codes.insert(ValidatorCode::kStringInvalidCharacter);
        break;
      }
    }
  }
  // Checks if the input string consists only of printable characters.
  void CheckPrintable(bool uppercase_letters_allowed = false) {
    for (char c : value) {
      if (c < 0x20 || c > 0x7e) {
        codes.insert(ValidatorCode::kStringInvalidCharacter);
        break;
      }
      if (!uppercase_letters_allowed && IsUppercaseLetter(c)) {
        codes.insert(ValidatorCode::kStringInvalidCharacter);
        break;
      }
    }
  }
};

// `year` must be > 0.
bool IsLeapYear(uint16_t year) {
  if (year % 4)
    return false;
  // Is divisible by 4.
  if (year % 100)
    return true;
  // Is divisible by 4 and 100.
  return (year % 400 == 0);
}

// Validate 'text' value based on:
// * rfc8011, section 5.1.2.
std::set<ValidatorCode> validateTextWithoutLanguage(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfText, /*empty_string_allowed=*/true);
  return validator.codes;
}

// Validate 'name' value based on:
// * rfc8011, section 5.1.3.
std::set<ValidatorCode> validateNameWithoutLanguage(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfName, /*empty_string_allowed=*/true);
  return validator.codes;
}

// Validate 'keyword' value based on:
// * rfc8011, section 5.1.4.
// * rfc8011 errata
std::set<ValidatorCode> validateKeyword(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfKeyword);
  validator.CheckLettersDigits(kAllowedCharsInKeyword,
                               /*uppercase_letters_allowed=*/true);
  return validator.codes;
}

// Validate 'uri' value based on:
// * rfc8011, section 5.1.6;
// * rfc3986, section 2.
std::set<ValidatorCode> validateUri(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfUri);
  validator.CheckLettersDigits(kAllowedCharsInUri,
                               /*uppercase_letters_allowed=*/true);
  return validator.codes;
}

// Validate 'uriScheme' value based on:
// * rfc8011, section 5.1.7;
// * rfc3986, section 3.1.
std::set<ValidatorCode> validateUriScheme(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfUriScheme);
  validator.CheckFirstLetterIsLowercase();
  validator.CheckLettersDigits(kAllowedCharsInUriScheme);
  return validator.codes;
}

// Validate 'charset' value based on:
// * rfc8011, section 5.1.8;
// * https://www.iana.org/assignments/character-sets/character-sets.xhtml.
std::set<ValidatorCode> validateCharset(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfCharset);
  validator.CheckPrintable();
  return validator.codes;
}

// Validate 'naturalLanguage' value based on:
// * rfc8011, section 5.1.9;
// * rfc5646, section 2.1.
std::set<ValidatorCode> validateNaturalLanguage(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfNaturalLanguage);
  validator.CheckLettersDigits(kAllowedCharsInNaturalLanguage);
  return validator.codes;
}

// Validate 'mimeMediaType' value based on:
// * rfc8011, section 5.1.10;
// * https://www.iana.org/assignments/media-types/media-types.xhtml.
std::set<ValidatorCode> validateMimeMediaType(std::string_view value) {
  StringValidator validator = {value};
  validator.CheckLength(kMaxLengthOfMimeMediaType);
  validator.CheckPrintable(/*uppercase_letters_allowed=*/true);
  return validator.codes;
}

// Validate 'octetString' value based on:
// * rfc8011, section 5.1.11.
std::set<ValidatorCode> validateOctetString(std::string_view value) {
  std::set<ValidatorCode> codes;
  if (value.size() > kMaxLengthOfOctetString)
    codes.insert(ValidatorCode::kStringTooLong);
  return codes;
}

// Validate 'dateTime' value based on:
// * rfc8011, section 5.1.15;
// * DateAndTime defined in rfc2579, section 2;
// * also enforces 1970 <= year <= 2100.
std::set<ValidatorCode> validateDateTime(const DateTime& value) {
  std::set<ValidatorCode> codes;

  // Verify the date.
  if (value.year < 1970 || value.year > 2100 || value.month < 1 ||
      value.month > 12 || value.day < 1) {
    codes.insert(ValidatorCode::kDateTimeInvalidDate);
  } else {
    uint8_t max_day = 31;
    switch (value.month) {
      case 2:
        if (IsLeapYear(value.year)) {
          max_day = 29;
        } else {
          max_day = 28;
        }
        break;
      case 4:  // FALLTHROUGH
      case 6:  // FALLTHROUGH
      case 9:  // FALLTHROUGH
      case 11:
        max_day = 30;
        break;
    }
    if (value.day > max_day) {
      codes.insert(ValidatorCode::kDateTimeInvalidDate);
    }
  }

  // Verify the time of day (seconds == 60 means leap second).
  if (value.hour > 23 || value.minutes > 59 || value.seconds > 60 ||
      value.deci_seconds > 9) {
    codes.insert(ValidatorCode::kDateTimeInvalidTimeOfDay);
  }

  // Verify the timezone (daylight saving time in New Zealand is +13).
  if ((value.UTC_direction != '-' && value.UTC_direction != '+') ||
      value.UTC_hours > 13 || value.UTC_minutes > 59) {
    codes.insert(ValidatorCode::kDateTimeInvalidZone);
  }

  return codes;
}

// Validate 'resolution' value based on:
// * rfc8011, section 5.1.16.
std::set<ValidatorCode> validateResolution(Resolution value) {
  std::set<ValidatorCode> codes;
  if (value.units != Resolution::Units::kDotsPerCentimeter &&
      value.units != Resolution::Units::kDotsPerInch)
    codes.insert(ValidatorCode::kResolutionInvalidUnit);
  if (value.xres < 1 || value.yres < 1)
    codes.insert(ValidatorCode::kResolutionInvalidDimension);
  return codes;
}

// Validate 'rangeOfInteger' value based on:
// * rfc8011, section 5.1.14.
std::set<ValidatorCode> validateRangeOfInteger(RangeOfInteger value) {
  std::set<ValidatorCode> codes;
  if (value.min_value > value.max_value)
    codes.insert(ValidatorCode::kRangeOfIntegerMaxLessMin);
  return codes;
}

// Validate 'textWithLanguage' value based on:
// * rfc8011, section 5.1.2.2.
std::set<ValidatorCode> validateTextWithLanguage(
    const StringWithLanguage& value) {
  std::set<ValidatorCode> codes = validateTextWithoutLanguage(value.value);
  if (!value.language.empty()) {
    if (!validateNaturalLanguage(value.language).empty())
      codes.insert(ValidatorCode::kStringWithLangInvalidLanguage);
  }
  return codes;
}

// Validate 'nameWithLanguage' value based on:
// * rfc8011, section 5.1.3.2.
std::set<ValidatorCode> validateNameWithLanguage(
    const StringWithLanguage& value) {
  std::set<ValidatorCode> codes = validateNameWithoutLanguage(value.value);
  if (!value.language.empty()) {
    if (!validateNaturalLanguage(value.language).empty())
      codes.insert(ValidatorCode::kStringWithLangInvalidLanguage);
  }
  return codes;
}

// Validate a single value in `attribute`. `attribute` must not be nullptr and
// `value_index` must be a valid index.
std::set<ValidatorCode> ValidateValue(const Attribute* attribute,
                                      size_t value_index) {
  if (IsString(attribute->Tag())) {
    std::string values_str;
    attribute->GetValue(value_index, values_str);
    switch (attribute->Tag()) {
      case ValueTag::textWithoutLanguage:
        return validateTextWithoutLanguage(values_str);
      case ValueTag::nameWithoutLanguage:
        return validateNameWithoutLanguage(values_str);
      case ValueTag::keyword:
        return validateKeyword(values_str);
      case ValueTag::uri:
        return validateUri(values_str);
      case ValueTag::uriScheme:
        return validateUriScheme(values_str);
      case ValueTag::charset:
        return validateCharset(values_str);
      case ValueTag::naturalLanguage:
        return validateNaturalLanguage(values_str);
      case ValueTag::mimeMediaType:
        return validateMimeMediaType(values_str);
      default:
        // There are no validation rules for other strings.
        return {};
    }
  }
  switch (attribute->Tag()) {
    case ValueTag::octetString: {
      std::string value;
      attribute->GetValue(value_index, value);
      return validateOctetString(value);
    }
    case ValueTag::dateTime: {
      DateTime value;
      attribute->GetValue(value_index, value);
      return validateDateTime(value);
    }
    case ValueTag::resolution: {
      Resolution value;
      attribute->GetValue(value_index, value);
      return validateResolution(value);
    }
    case ValueTag::rangeOfInteger: {
      RangeOfInteger value;
      attribute->GetValue(value_index, value);
      return validateRangeOfInteger(value);
    }
    case ValueTag::textWithLanguage: {
      StringWithLanguage value;
      attribute->GetValue(value_index, value);
      return validateTextWithLanguage(value);
    }
    case ValueTag::nameWithLanguage: {
      StringWithLanguage value;
      attribute->GetValue(value_index, value);
      return validateNameWithLanguage(value);
    }
    default:
      // Other types does not need validation.
      return {};
  }
}

struct ValidationResult {
  bool no_errors = true;
  bool keep_going = true;
};

ValidationResult operator&&(ValidationResult vr1, ValidationResult vr2) {
  ValidationResult result;
  result.keep_going = vr1.keep_going && vr2.keep_going;
  result.no_errors = vr1.no_errors && vr2.no_errors;
  return result;
}

ValidationResult ValidateCollections(ConstCollsView colls,
                                     ValidatorLog& log,
                                     AttrPath& path);

ValidationResult ValidateAttribute(const Attribute* attr,
                                   ValidatorLog& log,
                                   AttrPath& path) {
  ValidationResult result;
  std::set<ValidatorCode> name_errors = validateKeyword(attr->Name());
  if (!name_errors.empty()) {
    result.no_errors = false;
    result.keep_going =
        log.AddValidatorError({path, AttrError(std::move(name_errors))});
    if (!result.keep_going)
      return result;
  }
  const size_t values_count = attr->Size();
  if (attr->Tag() == ValueTag::collection) {
    result = result && ValidateCollections(attr->Colls(), log, path);
  } else {
    for (size_t i = 0; i < values_count; ++i) {
      std::set<ValidatorCode> value_errors = ValidateValue(attr, i);
      if (!value_errors.empty()) {
        result.no_errors = false;
        result.keep_going = log.AddValidatorError(
            {path, AttrError(i, std::move(value_errors))});
        if (!result.keep_going)
          return result;
      }
    }
  }
  return result;
}

ValidationResult ValidateCollections(ConstCollsView colls,
                                     ValidatorLog& log,
                                     AttrPath& path) {
  ValidationResult result;
  uint16_t icoll = 0;
  for (const Collection& coll : colls) {
    for (const Attribute& attr : coll) {
      path.PushBack(icoll, attr.Name());
      result = result && ValidateAttribute(&attr, log, path);
      path.PopBack();
      if (!result.keep_going)
        return result;
    }
    ++icoll;
  }
  return result;
}

// Checks the values saved in the header of `frame`.
ValidationResult ValidateHeader(const Frame& frame, ValidatorLog& log) {
  std::vector<std::string> invalid_fields;
  invalid_fields.reserve(4);
  uint8_t ver_major = static_cast<uint16_t>(frame.VersionNumber()) >> 8;
  uint8_t ver_minor = static_cast<uint16_t>(frame.VersionNumber()) & 0xffu;
  if (ver_major < 1 || ver_major > 9) {
    invalid_fields.push_back("major-version-number");
  }
  if (ver_minor > 9) {
    invalid_fields.push_back("minor-version-number");
  }
  if (frame.OperationIdOrStatusCode() < 0) {
    invalid_fields.push_back("operation-id or status-code");
  }
  if (frame.RequestId() < 1) {
    invalid_fields.push_back("request-id");
  }
  ValidationResult result;
  result.no_errors = invalid_fields.empty();
  AttrError error = AttrError(0, {ValidatorCode::kIntegerOutOfRange});
  AttrPath path = AttrPath(AttrPath::kHeader);
  for (const std::string& name : invalid_fields) {
    path.PushBack(0, name);
    result.keep_going = log.AddValidatorError({path, error});
    if (!result.keep_going) {
      break;
    }
    path.PopBack();
  }
  return result;
}

}  // namespace

std::string_view ToStrView(ValidatorCode code) {
  switch (code) {
    case ValidatorCode::kStringEmpty:
      return "StringEmpty";
    case ValidatorCode::kStringTooLong:
      return "StringTooLong";
    case ValidatorCode::kStringMustStartLowercaseLetter:
      return "StringMustStartLowercaseLetter";
    case ValidatorCode::kStringInvalidCharacter:
      return "StringInvalidCharacter";
    case ValidatorCode::kStringWithLangInvalidLanguage:
      return "StringWithLangInvalidLanguage";
    case ValidatorCode::kDateTimeInvalidDate:
      return "DateTimeInvalidDate";
    case ValidatorCode::kDateTimeInvalidTimeOfDay:
      return "DateTimeInvalidTimeOfDay";
    case ValidatorCode::kDateTimeInvalidZone:
      return "DateTimeInvalidZone";
    case ValidatorCode::kResolutionInvalidUnit:
      return "ResolutionInvalidUnit";
    case ValidatorCode::kResolutionInvalidDimension:
      return "ResolutionInvalidDimension";
    case ValidatorCode::kRangeOfIntegerMaxLessMin:
      return "RangeOfIntegerMaxLessMin";
    case ValidatorCode::kIntegerOutOfRange:
      return "IntegerOutOfRange";
  }
}

std::vector<ValidatorCode> AttrError::ErrorsAsVector() const {
  return std::vector<ValidatorCode>(errors_.begin(), errors_.end());
}

std::string ToString(const ValidatorError& error) {
  std::string msg = error.path.AsString();
  if (error.error.IsInTheName()) {
    msg += "; name; ";
  } else {
    msg += "; ";
    msg += std::to_string(error.error.Index());
    msg += "; ";
  }
  const std::vector<ValidatorCode> codes = error.error.ErrorsAsVector();
  for (ValidatorCode code : codes) {
    if (msg.back() != '\t') {
      msg += ",";
    }
    msg += ToStrView(code);
  }
  return msg;
}

bool SimpleValidatorLog::AddValidatorError(const ValidatorError& error) {
  if (entries_.size() < max_entries_count_) {
    entries_.emplace_back(error);
  }
  return (entries_.size() < max_entries_count_);
}

bool Validate(const Frame& frame, ValidatorLog& log) {
  ValidationResult result = ValidateHeader(frame, log);
  if (!result.keep_going)
    return result.no_errors;
  for (GroupTag group_tag : kGroupTags) {
    size_t coll_index = 0;
    for (const Collection& coll : frame.Groups(group_tag)) {
      AttrPath path(group_tag);
      for (const Attribute& attr : coll) {
        path.PushBack(coll_index, attr.Name());
        result = result && ValidateAttribute(&attr, log, path);
        path.PopBack();
        if (!result.keep_going)
          return result.no_errors;
      }
      ++coll_index;
    }
  }
  return result.no_errors;
}

}  // namespace ipp
