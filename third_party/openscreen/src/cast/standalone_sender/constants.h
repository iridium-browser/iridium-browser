// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_CONSTANTS_H_
#define CAST_STANDALONE_SENDER_CONSTANTS_H_

#include "util/chrono_helpers.h"

namespace openscreen {
namespace cast {

// How often should the congestion control logic re-evaluate the target encode
// bitrates?
constexpr milliseconds kCongestionCheckInterval{500};

// Above what available bandwidth should the high-quality audio bitrate be used?
constexpr int kHighBandwidthThreshold = 5 << 20;  // 5 Mbps.

// How often should the file position (media timestamp) be updated on the
// console?
constexpr milliseconds kConsoleUpdateInterval{100};

// What is the default maximum bitrate setting?
constexpr int kDefaultMaxBitrate = 5 << 20;  // 5 Mbps.

// What is the minimum amount of bandwidth required?
constexpr int kMinRequiredBitrate = 384 << 10;  // 384 kbps.

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_CONSTANTS_H_
