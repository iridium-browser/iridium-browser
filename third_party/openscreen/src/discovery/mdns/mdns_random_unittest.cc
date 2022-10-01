// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_random.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

namespace {
constexpr int kIterationCount = 100;
}

TEST(MdnsRandomTest, InitialQueryDelay) {
  constexpr std::chrono::milliseconds lower_bound{20};
  constexpr std::chrono::milliseconds upper_bound{120};
  MdnsRandom mdns_random;
  for (int i = 0; i < kIterationCount; ++i) {
    const Clock::duration delay = mdns_random.GetInitialQueryDelay();
    EXPECT_GE(delay, lower_bound);
    EXPECT_LE(delay, upper_bound);
  }
}

TEST(MdnsRandomTest, RecordTtlVariation) {
  constexpr double lower_bound = 0.0;
  constexpr double upper_bound = 0.02;
  MdnsRandom mdns_random;
  for (int i = 0; i < kIterationCount; ++i) {
    const double variation = mdns_random.GetRecordTtlVariation();
    EXPECT_GE(variation, lower_bound);
    EXPECT_LE(variation, upper_bound);
  }
}

TEST(MdnsRandomTest, SharedRecordResponseDelay) {
  constexpr std::chrono::milliseconds lower_bound{20};
  constexpr std::chrono::milliseconds upper_bound{120};
  MdnsRandom mdns_random;
  for (int i = 0; i < kIterationCount; ++i) {
    const Clock::duration delay = mdns_random.GetSharedRecordResponseDelay();
    EXPECT_GE(delay, lower_bound);
    EXPECT_LE(delay, upper_bound);
  }
}

TEST(MdnsRandomTest, TruncatedQueryResponseDelay) {
  constexpr std::chrono::milliseconds lower_bound{400};
  constexpr std::chrono::milliseconds upper_bound{500};
  MdnsRandom mdns_random;
  for (int i = 0; i < kIterationCount; ++i) {
    const Clock::duration delay = mdns_random.GetTruncatedQueryResponseDelay();
    EXPECT_GE(delay, lower_bound);
    EXPECT_LE(delay, upper_bound);
  }
}

}  // namespace discovery
}  // namespace openscreen
