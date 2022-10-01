// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CERTIFICATE_DATE_TIME_H_
#define CAST_COMMON_CERTIFICATE_DATE_TIME_H_

#include <stdint.h>

#include "cast/common/public/certificate_types.h"

namespace openscreen {
namespace cast {

bool operator<(const DateTime& a, const DateTime& b);
bool operator>(const DateTime& a, const DateTime& b);
bool DateTimeFromSeconds(uint64_t seconds, DateTime* time);

// |time| is assumed to be valid.
std::chrono::seconds DateTimeToSeconds(const DateTime& time);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CERTIFICATE_DATE_TIME_H_
