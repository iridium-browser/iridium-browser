// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/statistics.h"

#include <algorithm>

namespace openscreen {
namespace cast {

SimpleHistogram::SimpleHistogram() = default;
SimpleHistogram::SimpleHistogram(int64_t min, int64_t max, int64_t width)

    : min(min), max(max), width(width), buckets((max - min) / width + 2) {
  OSP_CHECK_GT(buckets.size(), 2u);
  OSP_CHECK_EQ(0, (max - min) % width);
}

SimpleHistogram::SimpleHistogram(SimpleHistogram&&) noexcept = default;
SimpleHistogram& SimpleHistogram::operator=(SimpleHistogram&&) = default;
SimpleHistogram::~SimpleHistogram() = default;

void SimpleHistogram::Add(int64_t sample) {
  if (sample < min) {
    ++buckets.front();
  } else if (sample >= max) {
    ++buckets.back();
  } else {
    size_t index = 1 + (sample - min) / width;
    OSP_CHECK_LT(index, buckets.size());
    ++buckets[index];
  }
}

void SimpleHistogram::Reset() {
  buckets.assign(buckets.size(), 0);
}

SenderStatsClient::~SenderStatsClient() {}

}  // namespace cast
}  // namespace openscreen
