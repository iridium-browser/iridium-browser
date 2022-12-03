// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains helpers for working with enums that require
// both enum->string and string->enum conversions.

#ifndef UTIL_ENUM_NAME_TABLE_H_
#define UTIL_ENUM_NAME_TABLE_H_

#include <array>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "platform/base/error.h"
#include "util/osp_logging.h"

namespace openscreen {

constexpr char kUnknownEnumError[] = "Enum value not in array";

template <typename Enum, size_t Size>
using EnumNameTable = std::array<std::pair<const char*, Enum>, Size>;

// Get the name of an enum from the enum value.
template <typename Enum, size_t Size>
ErrorOr<const char*> GetEnumName(const EnumNameTable<Enum, Size>& map,
                                 Enum enum_) {
  for (auto pair : map) {
    if (pair.second == enum_) {
      return pair.first;
    }
  }
  return Error(Error::Code::kParameterInvalid, kUnknownEnumError);
}

// Get the value of an enum from the enum name.
template <typename Enum, size_t Size>
ErrorOr<Enum> GetEnum(const EnumNameTable<Enum, Size>& map,
                      absl::string_view name) {
  for (auto pair : map) {
    if (absl::EqualsIgnoreCase(pair.first, name)) {
      return pair.second;
    }
  }
  return Error(Error::Code::kParameterInvalid, kUnknownEnumError);
}

}  // namespace openscreen
#endif  // UTIL_ENUM_NAME_TABLE_H_
