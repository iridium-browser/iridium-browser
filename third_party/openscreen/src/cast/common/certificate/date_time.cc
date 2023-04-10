// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/date_time.h"

#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

bool operator<(const DateTime& a, const DateTime& b) {
  if (a.year < b.year) {
    return true;
  } else if (a.year > b.year) {
    return false;
  }
  if (a.month < b.month) {
    return true;
  } else if (a.month > b.month) {
    return false;
  }
  if (a.day < b.day) {
    return true;
  } else if (a.day > b.day) {
    return false;
  }
  if (a.hour < b.hour) {
    return true;
  } else if (a.hour > b.hour) {
    return false;
  }
  if (a.minute < b.minute) {
    return true;
  } else if (a.minute > b.minute) {
    return false;
  }
  if (a.second < b.second) {
    return true;
  } else if (a.second > b.second) {
    return false;
  }
  return false;
}

bool operator>(const DateTime& a, const DateTime& b) {
  return (b < a);
}

bool DateTimeFromSeconds(uint64_t seconds, DateTime* time) {
  struct tm tm = {};
  time_t sec = static_cast<time_t>(seconds);
  OSP_DCHECK_GE(sec, 0);
  OSP_DCHECK_EQ(static_cast<uint64_t>(sec), seconds);
#if defined(_WIN32)
  // NOTE: This is for compiling in Chromium and is not validated in any direct
  // libcast Windows build.
  if (gmtime_s(&tm, &sec)) {
    return false;
  }
#else
  if (!gmtime_r(&sec, &tm)) {
    return false;
  }
#endif

  time->second = tm.tm_sec;
  time->minute = tm.tm_min;
  time->hour = tm.tm_hour;
  time->day = tm.tm_mday;
  time->month = tm.tm_mon + 1;
  time->year = tm.tm_year + 1900;

  return true;
}

static_assert(sizeof(time_t) >= 4, "Can't avoid overflow with < 32-bits");

std::chrono::seconds DateTimeToSeconds(const DateTime& time) {
  OSP_DCHECK_GE(time.month, 1);
  OSP_DCHECK_GE(time.year, 1900);
  // NOTE: Guard against overflow if time_t is 32-bit.
  OSP_DCHECK(sizeof(time_t) >= 8 || time.year < 2038) << time.year;
  struct tm tm = {};
  tm.tm_sec = time.second;
  tm.tm_min = time.minute;
  tm.tm_hour = time.hour;
  tm.tm_mday = time.day;
  tm.tm_mon = time.month - 1;
  tm.tm_year = time.year - 1900;
  time_t sec;
#if defined(_WIN32)
  sec = _mkgmtime(&tm);
#else
  sec = timegm(&tm);
#endif
  return std::chrono::seconds(sec);
}

}  // namespace cast
}  // namespace openscreen
