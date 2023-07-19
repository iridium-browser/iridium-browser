// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_split.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/third_party/icu/icu_utf.h"

namespace base {

namespace {

// Returns either the ASCII or UTF-16 whitespace.
template <typename char_type>
std::basic_string_view<char_type> WhitespaceForType();
template <>
std::u16string_view WhitespaceForType<char16_t>() {
  return kWhitespaceUTF16;
}
template <>
std::string_view WhitespaceForType<char>() {
  return kWhitespaceASCII;
}

// Optimize the single-character case to call find() on the string instead,
// since this is the common case and can be made faster. This could have been
// done with template specialization too, but would have been less clear.
//
// There is no corresponding FindFirstNotOf because std::string_view already
// implements these different versions that do the optimized searching.
size_t FindFirstOf(std::string_view piece, char c, size_t pos) {
  return piece.find(c, pos);
}
size_t FindFirstOf(std::u16string_view piece, char16_t c, size_t pos) {
  return piece.find(c, pos);
}
size_t FindFirstOf(std::string_view piece,
                   std::string_view one_of,
                   size_t pos) {
  return piece.find_first_of(one_of, pos);
}
size_t FindFirstOf(std::u16string_view piece,
                   std::u16string_view one_of,
                   size_t pos) {
  return piece.find_first_of(one_of, pos);
}

// General string splitter template. Can take 8- or 16-bit input, can produce
// the corresponding string or std::string_view output, and can take single- or
// multiple-character delimiters.
//
// DelimiterType is either a character (Str::value_type) or a string piece of
// multiple characters (std::basic_string_view<char>). std::string_view has a
// version of find for both of these cases, and the single-character version is
// the most common and can be implemented faster, which is why this is a
// template.
template <typename char_type, typename OutputStringType, typename DelimiterType>
static std::vector<OutputStringType> SplitStringT(
    std::basic_string_view<char_type> str,
    DelimiterType delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  std::vector<OutputStringType> result;
  if (str.empty())
    return result;

  using ViewType = std::basic_string_view<char_type>;

  size_t start = 0;
  while (start != ViewType::npos) {
    size_t end = FindFirstOf(str, delimiter, start);

    ViewType piece;
    if (end == ViewType::npos) {
      piece = str.substr(start);
      start = ViewType::npos;
    } else {
      piece = str.substr(start, end - start);
      start = end + 1;
    }

    if (whitespace == TRIM_WHITESPACE)
      piece = TrimString(piece, WhitespaceForType<char_type>(), TRIM_ALL);

    if (result_type == SPLIT_WANT_ALL || !piece.empty())
      result.emplace_back(piece);
  }
  return result;
}

bool AppendStringKeyValue(std::string_view input,
                          char delimiter,
                          StringPairs* result) {
  // Always append a new item regardless of success (it might be empty). The
  // below code will copy the strings directly into the result pair.
  result->resize(result->size() + 1);
  auto& result_pair = result->back();

  // Find the delimiter.
  size_t end_key_pos = input.find_first_of(delimiter);
  if (end_key_pos == std::string::npos) {
    return false;  // No delimiter.
  }
  result_pair.first.assign(input.substr(0, end_key_pos));

  // Find the value string.
  std::string_view remains =
      input.substr(end_key_pos, input.size() - end_key_pos);
  size_t begin_value_pos = remains.find_first_not_of(delimiter);
  if (begin_value_pos == std::string_view::npos) {
    return false;  // No value.
  }
  result_pair.second.assign(
      remains.substr(begin_value_pos, remains.size() - begin_value_pos));

  return true;
}

template <typename char_type, typename OutputStringType>
void SplitStringUsingSubstrT(std::basic_string_view<char_type> input,
                             std::basic_string_view<char_type> delimiter,
                             WhitespaceHandling whitespace,
                             SplitResult result_type,
                             std::vector<OutputStringType>* result) {
  using Piece = std::basic_string_view<char_type>;
  using size_type = typename Piece::size_type;

  result->clear();
  for (size_type begin_index = 0, end_index = 0; end_index != Piece::npos;
       begin_index = end_index + delimiter.size()) {
    end_index = input.find(delimiter, begin_index);
    Piece term = end_index == Piece::npos
                     ? input.substr(begin_index)
                     : input.substr(begin_index, end_index - begin_index);

    if (whitespace == TRIM_WHITESPACE)
      term = TrimString(term, WhitespaceForType<char_type>(), TRIM_ALL);

    if (result_type == SPLIT_WANT_ALL || !term.empty())
      result->emplace_back(term);
  }
}

}  // namespace

std::vector<std::string> SplitString(std::string_view input,
                                     std::string_view separators,
                                     WhitespaceHandling whitespace,
                                     SplitResult result_type) {
  if (separators.size() == 1) {
    return SplitStringT<char, std::string, char>(input, separators[0],
                                                 whitespace, result_type);
  }
  return SplitStringT<char, std::string, std::string_view>(
      input, separators, whitespace, result_type);
}

std::vector<std::u16string> SplitString(std::u16string_view input,
                                        std::u16string_view separators,
                                        WhitespaceHandling whitespace,
                                        SplitResult result_type) {
  if (separators.size() == 1) {
    return SplitStringT<char16_t, std::u16string, char16_t>(
        input, separators[0], whitespace, result_type);
  }
  return SplitStringT<char16_t, std::u16string, std::u16string_view>(
      input, separators, whitespace, result_type);
}

std::vector<std::string_view> SplitStringPiece(std::string_view input,
                                               std::string_view separators,
                                               WhitespaceHandling whitespace,
                                               SplitResult result_type) {
  if (separators.size() == 1) {
    return SplitStringT<char, std::string_view, char>(input, separators[0],
                                                      whitespace, result_type);
  }
  return SplitStringT<char, std::string_view, std::string_view>(
      input, separators, whitespace, result_type);
}

std::vector<std::u16string_view> SplitStringPiece(
    std::u16string_view input,
    std::u16string_view separators,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  if (separators.size() == 1) {
    return SplitStringT<char16_t, std::u16string_view, char16_t>(
        input, separators[0], whitespace, result_type);
  }
  return SplitStringT<char16_t, std::u16string_view, std::u16string_view>(
      input, separators, whitespace, result_type);
}

bool SplitStringIntoKeyValuePairs(std::string_view input,
                                  char key_value_delimiter,
                                  char key_value_pair_delimiter,
                                  StringPairs* key_value_pairs) {
  key_value_pairs->clear();

  std::vector<std::string_view> pairs =
      SplitStringPiece(input, std::string(1, key_value_pair_delimiter),
                       TRIM_WHITESPACE, SPLIT_WANT_NONEMPTY);
  key_value_pairs->reserve(pairs.size());

  bool success = true;
  for (const std::string_view& pair : pairs) {
    if (!AppendStringKeyValue(pair, key_value_delimiter, key_value_pairs)) {
      // Don't return here, to allow for pairs without associated
      // value or key; just record that the split failed.
      success = false;
    }
  }
  return success;
}

std::vector<std::u16string> SplitStringUsingSubstr(
    std::u16string_view input,
    std::u16string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  std::vector<std::u16string> result;
  SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
  return result;
}

std::vector<std::string> SplitStringUsingSubstr(std::string_view input,
                                                std::string_view delimiter,
                                                WhitespaceHandling whitespace,
                                                SplitResult result_type) {
  std::vector<std::string> result;
  SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
  return result;
}

std::vector<std::u16string_view> SplitStringPieceUsingSubstr(
    std::u16string_view input,
    std::u16string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  std::vector<std::u16string_view> result;
  SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
  return result;
}

std::vector<std::string_view> SplitStringPieceUsingSubstr(
    std::string_view input,
    std::string_view delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  std::vector<std::string_view> result;
  SplitStringUsingSubstrT(input, delimiter, whitespace, result_type, &result);
  return result;
}

}  // namespace base
