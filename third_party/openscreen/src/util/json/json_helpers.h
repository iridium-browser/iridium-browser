// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_JSON_JSON_HELPERS_H_
#define UTIL_JSON_JSON_HELPERS_H_

#include <chrono>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "json/value.h"
#include "platform/base/error.h"
#include "util/chrono_helpers.h"
#include "util/json/json_serialization.h"
#include "util/simple_fraction.h"

// This file contains helper methods for parsing JSON, in an attempt to
// reduce boilerplate code when working with JsonCpp.
namespace openscreen {
namespace json {

inline bool TryParseBool(const Json::Value& value, bool* out) {
  if (!value.isBool()) {
    return false;
  }
  *out = value.asBool();
  return true;
}

// A general note about parsing primitives. "Validation" in this context
// generally means ensuring that the values are non-negative, excepting doubles
// which may be negative in some cases.
inline bool TryParseDouble(const Json::Value& value,
                           double* out,
                           bool allow_negative = false) {
  if (!value.isDouble()) {
    return false;
  }
  const double d = value.asDouble();
  if (std::isnan(d)) {
    return false;
  }
  if (!allow_negative && d < 0) {
    return false;
  }
  *out = d;
  return true;
}

inline bool TryParseInt(const Json::Value& value, int* out) {
  if (!value.isInt()) {
    return false;
  }
  int i = value.asInt();
  if (i < 0) {
    return false;
  }
  *out = i;
  return true;
}

inline bool TryParseUint(const Json::Value& value, uint32_t* out) {
  if (!value.isUInt()) {
    return false;
  }
  *out = value.asUInt();
  return true;
}

inline bool TryParseString(const Json::Value& value, std::string* out) {
  if (!value.isString()) {
    return false;
  }
  *out = value.asString();
  return true;
}

// We want to be more robust when we parse fractions then just
// allowing strings, this will parse numeral values such as
// value: 50 as well as value: "50" and value: "100/2".
inline bool TryParseSimpleFraction(const Json::Value& value,
                                   SimpleFraction* out) {
  if (value.isInt()) {
    int parsed = value.asInt();
    if (parsed < 0) {
      return false;
    }
    *out = SimpleFraction{parsed, 1};
    return true;
  }

  if (value.isString()) {
    auto fraction_or_error = SimpleFraction::FromString(value.asString());
    if (!fraction_or_error) {
      return false;
    }

    if (!fraction_or_error.value().is_positive() ||
        !fraction_or_error.value().is_defined()) {
      return false;
    }
    *out = std::move(fraction_or_error.value());
    return true;
  }
  return false;
}

inline bool TryParseMilliseconds(const Json::Value& value, milliseconds* out) {
  int out_ms;
  if (!TryParseInt(value, &out_ms) || out_ms < 0) {
    return false;
  }
  *out = milliseconds(out_ms);
  return true;
}

template <typename T>
using Parser = std::function<bool(const Json::Value&, T*)>;

// NOTE: array parsing methods reset the output vector to an empty vector in
// any error case. This is especially useful for optional arrays.
template <typename T>
bool TryParseArray(const Json::Value& value,
                   Parser<T> parser,
                   std::vector<T>* out) {
  out->clear();
  if (!value.isArray() || value.empty()) {
    return false;
  }

  out->reserve(value.size());
  for (Json::ArrayIndex i = 0; i < value.size(); ++i) {
    T v;
    if (!parser(value[i], &v)) {
      out->clear();
      return false;
    }
    out->push_back(v);
  }

  return true;
}

inline bool TryParseIntArray(const Json::Value& value, std::vector<int>* out) {
  return TryParseArray<int>(value, TryParseInt, out);
}

inline bool TryParseUintArray(const Json::Value& value,
                              std::vector<uint32_t>* out) {
  return TryParseArray<uint32_t>(value, TryParseUint, out);
}

inline bool TryParseStringArray(const Json::Value& value,
                                std::vector<std::string>* out) {
  return TryParseArray<std::string>(value, TryParseString, out);
}

}  // namespace json
}  // namespace openscreen

#endif  // UTIL_JSON_JSON_HELPERS_H_
