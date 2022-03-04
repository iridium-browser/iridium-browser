// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_JSON_JSON_VALUE_H_
#define UTIL_JSON_JSON_VALUE_H_

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "json/value.h"

#define JSON_EXPAND_FIND_CONSTANT_ARGS(s) (s), ((s) + sizeof(s) - 1)

namespace openscreen {

absl::optional<int> MaybeGetInt(const Json::Value& message,
                                const char* first,
                                const char* last);

absl::optional<absl::string_view> MaybeGetString(const Json::Value& message);

absl::optional<absl::string_view> MaybeGetString(const Json::Value& message,
                                                 const char* first,
                                                 const char* last);

}  // namespace openscreen

#endif  // UTIL_JSON_JSON_VALUE_H_
