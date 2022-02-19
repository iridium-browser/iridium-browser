// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_JSON_JSON_SERIALIZATION_H_
#define UTIL_JSON_JSON_SERIALIZATION_H_

#include <string>

#include "absl/strings/string_view.h"
#include "json/value.h"
#include "platform/base/error.h"

namespace openscreen {

namespace json {

ErrorOr<Json::Value> Parse(absl::string_view value);
ErrorOr<std::string> Stringify(const Json::Value& value);

}  // namespace json
}  // namespace openscreen

#endif  // UTIL_JSON_JSON_SERIALIZATION_H_
