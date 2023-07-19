// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_RANDOM_H_
#define DISCOVERY_MDNS_MDNS_RANDOM_H_

#include <random>

#include "platform/api/time.h"

namespace openscreen {
namespace discovery {

class MdnsRandom {
 public:
  // RFC 6762 Section 5.2
  // https://tools.ietf.org/html/rfc6762#section-5.2

  Clock::duration GetInitialQueryDelay() {
    return std::chrono::milliseconds{initial_query_delay_(random_engine_)};
  }

  double GetRecordTtlVariation() {
    return record_ttl_variation_(random_engine_);
  }

  // RFC 6762 Section 6
  // https://tools.ietf.org/html/rfc6762#section-6

  Clock::duration GetSharedRecordResponseDelay() {
    return std::chrono::milliseconds{
        shared_record_response_delay_(random_engine_)};
  }

  Clock::duration GetTruncatedQueryResponseDelay() {
    return std::chrono::milliseconds{
        truncated_query_response_delay_(random_engine_)};
  }

  Clock::duration GetInitialProbeDelay() {
    return std::chrono::milliseconds{
        probe_initialization_delay_(random_engine_)};
  }

 private:
  static constexpr int64_t kMinimumInitialQueryDelayMs = 20;
  static constexpr int64_t kMaximumInitialQueryDelayMs = 120;

  static constexpr double kMinimumTtlVariation = 0.0;
  static constexpr double kMaximumTtlVariation = 0.02;

  static constexpr int64_t kMinimumSharedRecordResponseDelayMs = 20;
  static constexpr int64_t kMaximumSharedRecordResponseDelayMs = 120;

  static constexpr int64_t kMinimumTruncatedQueryResponseDelayMs = 400;
  static constexpr int64_t kMaximumTruncatedQueryResponseDelayMs = 500;

  static constexpr int64_t kMinimumProbeInitializationDelayMs = 0;
  static constexpr int64_t kMaximumProbeInitializationDelayMs = 250;

  std::default_random_engine random_engine_{std::random_device{}()};
  std::uniform_int_distribution<int64_t> initial_query_delay_{
      kMinimumInitialQueryDelayMs, kMaximumInitialQueryDelayMs};
  std::uniform_real_distribution<double> record_ttl_variation_{
      kMinimumTtlVariation, kMaximumTtlVariation};
  std::uniform_int_distribution<int64_t> shared_record_response_delay_{
      kMinimumSharedRecordResponseDelayMs, kMaximumSharedRecordResponseDelayMs};
  std::uniform_int_distribution<int64_t> truncated_query_response_delay_{
      kMinimumTruncatedQueryResponseDelayMs,
      kMaximumTruncatedQueryResponseDelayMs};
  std::uniform_int_distribution<int64_t> probe_initialization_delay_{
      kMinimumProbeInitializationDelayMs, kMaximumProbeInitializationDelayMs};
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_RANDOM_H_
