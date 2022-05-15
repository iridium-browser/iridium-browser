// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/session_config.h"

#include <algorithm>
#include <utility>

namespace openscreen {
namespace cast {

namespace {

bool IsNonZero(uint8_t byte) {
  return byte > 0;
}

}  // namespace

SessionConfig::SessionConfig(Ssrc sender_ssrc,
                             Ssrc receiver_ssrc,
                             int rtp_timebase,
                             int channels,
                             std::chrono::milliseconds target_playout_delay,
                             std::array<uint8_t, 16> aes_secret_key,
                             std::array<uint8_t, 16> aes_iv_mask,
                             bool is_pli_enabled)
    : sender_ssrc(sender_ssrc),
      receiver_ssrc(receiver_ssrc),
      rtp_timebase(rtp_timebase),
      channels(channels),
      target_playout_delay(target_playout_delay),
      aes_secret_key(std::move(aes_secret_key)),
      aes_iv_mask(std::move(aes_iv_mask)),
      is_pli_enabled(is_pli_enabled) {}

SessionConfig::SessionConfig(const SessionConfig& other) = default;
SessionConfig::SessionConfig(SessionConfig&& other) noexcept = default;
SessionConfig& SessionConfig::operator=(const SessionConfig& other) = default;
SessionConfig& SessionConfig::operator=(SessionConfig&& other) noexcept =
    default;
SessionConfig::~SessionConfig() = default;

bool SessionConfig::IsValid() const {
  return sender_ssrc > 0 && receiver_ssrc > 0 && rtp_timebase > 0 &&
         channels > 0 &&
         std::any_of(aes_secret_key.begin(), aes_secret_key.end(), IsNonZero) &&
         std::any_of(aes_iv_mask.begin(), aes_iv_mask.end(), IsNonZero);
}
}  // namespace cast
}  // namespace openscreen
