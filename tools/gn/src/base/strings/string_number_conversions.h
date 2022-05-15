// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_
#define BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>

#include "util/build_config.h"

namespace base {

// Number -> string conversions ------------------------------------------------

// Ignores locale! see warning above.
std::string NumberToString(int value);
std::u16string NumberToString16(int value);
std::string NumberToString(unsigned int value);
std::u16string NumberToString16(unsigned int value);
std::string NumberToString(long value);
std::u16string NumberToString16(long value);
std::string NumberToString(unsigned long value);
std::u16string NumberToString16(unsigned long value);
std::string NumberToString(long long value);
std::u16string NumberToString16(long long value);
std::string NumberToString(unsigned long long value);
std::u16string NumberToString16(unsigned long long value);

// Type-specific naming for backwards compatibility.
//
// TODO(brettw) these should be removed and callers converted to the overloaded
// "NumberToString" variant.
inline std::string IntToString(int value) {
  return NumberToString(value);
}
inline std::u16string IntToString16(int value) {
  return NumberToString16(value);
}
inline std::string UintToString(unsigned value) {
  return NumberToString(value);
}
inline std::u16string UintToString16(unsigned value) {
  return NumberToString16(value);
}
inline std::string Int64ToString(int64_t value) {
  return NumberToString(value);
}
inline std::u16string Int64ToString16(int64_t value) {
  return NumberToString16(value);
}

// String -> number conversions ------------------------------------------------

// Perform a best-effort conversion of the input string to a numeric type,
// setting |*output| to the result of the conversion.  Returns true for
// "perfect" conversions; returns false in the following cases:
//  - Overflow. |*output| will be set to the maximum value supported
//    by the data type.
//  - Underflow. |*output| will be set to the minimum value supported
//    by the data type.
//  - Trailing characters in the string after parsing the number.  |*output|
//    will be set to the value of the number that was parsed.
//  - Leading whitespace in the string before parsing the number. |*output| will
//    be set to the value of the number that was parsed.
//  - No characters parseable as a number at the beginning of the string.
//    |*output| will be set to 0.
//  - Empty string.  |*output| will be set to 0.
// WARNING: Will write to |output| even when returning false.
//          Read the comments above carefully.
bool StringToInt(std::string_view input, int* output);
bool StringToInt(std::u16string_view input, int* output);

bool StringToUint(std::string_view input, unsigned* output);
bool StringToUint(std::u16string_view input, unsigned* output);

bool StringToInt64(std::string_view input, int64_t* output);
bool StringToInt64(std::u16string_view input, int64_t* output);

bool StringToUint64(std::string_view input, uint64_t* output);
bool StringToUint64(std::u16string_view input, uint64_t* output);

bool StringToSizeT(std::string_view input, size_t* output);
bool StringToSizeT(std::u16string_view input, size_t* output);

// Hex encoding ----------------------------------------------------------------

// Returns a hex string representation of a binary buffer. The returned hex
// string will be in upper case. This function does not check if |size| is
// within reasonable limits since it's written with trusted data in mind.  If
// you suspect that the data you want to format might be large, the absolute
// max size for |size| should be is
//   std::numeric_limits<size_t>::max() / 2
std::string HexEncode(const void* bytes, size_t size);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// -0x80000000 < |input| < 0x7FFFFFFF.
bool HexStringToInt(std::string_view input, int* output);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// 0x00000000 < |input| < 0xFFFFFFFF.
// The string is not required to start with 0x.
bool HexStringToUInt(std::string_view input, uint32_t* output);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// -0x8000000000000000 < |input| < 0x7FFFFFFFFFFFFFFF.
bool HexStringToInt64(std::string_view input, int64_t* output);

// Best effort conversion, see StringToInt above for restrictions.
// Will only successful parse hex values that will fit into |output|, i.e.
// 0x0000000000000000 < |input| < 0xFFFFFFFFFFFFFFFF.
// The string is not required to start with 0x.
bool HexStringToUInt64(std::string_view input, uint64_t* output);

// Similar to the previous functions, except that output is a vector of bytes.
// |*output| will contain as many bytes as were successfully parsed prior to the
// error.  There is no overflow, but input.size() must be evenly divisible by 2.
// Leading 0x or +/- are not allowed.
bool HexStringToBytes(std::string_view input, std::vector<uint8_t>* output);

}  // namespace base

#endif  // BASE_STRINGS_STRING_NUMBER_CONVERSIONS_H_
