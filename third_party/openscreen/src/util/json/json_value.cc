// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/json/json_value.h"

namespace openscreen {

absl::optional<int> MaybeGetInt(const Json::Value& message,
                                const char* first,
                                const char* last) {
  const Json::Value* value = message.find(first, last);
  absl::optional<int> result;
  if (value && value->isInt()) {
    result = value->asInt();
  }
  return result;
}

absl::optional<absl::string_view> MaybeGetString(const Json::Value& message) {
  if (message.isString()) {
    const char* begin = nullptr;
    const char* end = nullptr;
    message.getString(&begin, &end);
    if (begin && end >= begin) {
      return absl::string_view(begin, end - begin);
    }
  }
  return absl::nullopt;
}

absl::optional<absl::string_view> MaybeGetString(const Json::Value& message,
                                                 const char* first,
                                                 const char* last) {
  const Json::Value* value = message.find(first, last);
  absl::optional<absl::string_view> result;
  if (value && value->isString()) {
    return MaybeGetString(*value);
  }
  return result;
}

}  // namespace openscreen
