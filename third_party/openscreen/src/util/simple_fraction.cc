// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/simple_fraction.h"

#include <cmath>
#include <limits>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "util/osp_logging.h"

namespace openscreen {

// static
ErrorOr<SimpleFraction> SimpleFraction::FromString(absl::string_view value) {
  std::vector<absl::string_view> fields = absl::StrSplit(value, '/');
  if (fields.size() != 1 && fields.size() != 2) {
    return Error::Code::kParameterInvalid;
  }

  int numerator;
  int denominator = 1;
  if (!absl::SimpleAtoi(fields[0], &numerator)) {
    return Error::Code::kParameterInvalid;
  }

  if (fields.size() == 2) {
    if (!absl::SimpleAtoi(fields[1], &denominator)) {
      return Error::Code::kParameterInvalid;
    }
  }

  return SimpleFraction(numerator, denominator);
}

std::string SimpleFraction::ToString() const {
  if (denominator_ == 1) {
    return std::to_string(numerator_);
  }
  return absl::StrCat(numerator_, "/", denominator_);
}

}  // namespace openscreen
