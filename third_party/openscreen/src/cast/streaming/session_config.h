// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SESSION_CONFIG_H_
#define CAST_STREAMING_SESSION_CONFIG_H_

#include <array>
#include <chrono>
#include <cstdint>

#include "cast/streaming/ssrc.h"

namespace openscreen {
namespace cast {

// Common streaming configuration, established from the OFFER/ANSWER exchange,
// that the Sender and Receiver are both assuming.
struct SessionConfig final {
  SessionConfig(Ssrc sender_ssrc,
                Ssrc receiver_ssrc,
                int rtp_timebase,
                int channels,
                std::chrono::milliseconds target_playout_delay,
                std::array<uint8_t, 16> aes_secret_key,
                std::array<uint8_t, 16> aes_iv_mask,
                bool is_pli_enabled);
  SessionConfig(const SessionConfig& other);
  SessionConfig(SessionConfig&& other) noexcept;
  SessionConfig& operator=(const SessionConfig& other);
  SessionConfig& operator=(SessionConfig&& other) noexcept;
  ~SessionConfig();

  bool IsValid() const;

  // The sender and receiver's SSRC identifiers. Note: SSRC identifiers
  // are defined as unsigned 32 bit integers here:
  // https://tools.ietf.org/html/rfc5576#page-5
  Ssrc sender_ssrc = 0;
  Ssrc receiver_ssrc = 0;

  // RTP timebase: The number of RTP units advanced per second. For audio,
  // this is the sampling rate. For video, this is 90 kHz by convention.
  int rtp_timebase = 90000;

  // Number of channels. Must be 1 for video, for audio typically 2.
  int channels = 1;

  // Initial target playout delay.
  std::chrono::milliseconds target_playout_delay;

  // The AES-128 crypto key and initialization vector.
  std::array<uint8_t, 16> aes_secret_key{};
  std::array<uint8_t, 16> aes_iv_mask{};

  // Whether picture loss indication (PLI) should be used for this session.
  bool is_pli_enabled = false;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SESSION_CONFIG_H_
