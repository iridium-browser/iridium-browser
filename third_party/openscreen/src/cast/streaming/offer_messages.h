// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_OFFER_MESSAGES_H_
#define CAST_STREAMING_OFFER_MESSAGES_H_

#include <chrono>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "cast/streaming/message_fields.h"
#include "cast/streaming/resolution.h"
#include "cast/streaming/rtp_defines.h"
#include "cast/streaming/session_config.h"
#include "json/value.h"
#include "platform/base/error.h"
#include "util/simple_fraction.h"

// This file contains the implementation of the Cast V2 Mirroring Control
// Protocol offer object definition.
namespace openscreen {
namespace cast {

// If the target delay provided by the sender is not bounded by
// [kMinTargetDelay, kMaxTargetDelay], it will be set to
// kDefaultTargetPlayoutDelay.
constexpr auto kMinTargetPlayoutDelay = std::chrono::milliseconds(0);
constexpr auto kMaxTargetPlayoutDelay = std::chrono::milliseconds(5000);

// If the sender provides an invalid maximum frame rate, it ill
// be set to kDefaultMaxFrameRate.
constexpr int kDefaultMaxFrameRate = 30;

constexpr int kDefaultNumVideoChannels = 1;
constexpr int kDefaultNumAudioChannels = 2;

// A stream, as detailed by the CastV2 protocol spec, is a segment of an
// offer message specifically representing a configuration object for
// a codec and its related fields, such as maximum bit rate, time base,
// and other fields.
// Composed classes include AudioStream and VideoStream, which contain
// fields specific to audio and video respectively.
struct Stream {
  enum class Type : uint8_t { kAudioSource, kVideoSource };

  static Error TryParse(const Json::Value& root,
                        Stream::Type type,
                        Stream* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  int index = 0;
  Type type = {};

  // Default channel count is 1, e.g. for video.
  int channels = 0;
  RtpPayloadType rtp_payload_type = {};
  Ssrc ssrc = {};
  std::chrono::milliseconds target_delay = {};

  // AES Key and IV mask format is very strict: a 32 digit hex string that
  // must be converted to a 16 digit byte array.
  std::array<uint8_t, 16> aes_key = {};
  std::array<uint8_t, 16> aes_iv_mask = {};
  bool receiver_rtcp_event_log = false;
  std::string receiver_rtcp_dscp;
  int rtp_timebase = 0;

  // The codec parameter field honors the format laid out in RFC 6381:
  // https://datatracker.ietf.org/doc/html/rfc6381.
  std::string codec_parameter;
};

struct AudioStream {
  static Error TryParse(const Json::Value& root, AudioStream* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  Stream stream;
  AudioCodec codec = AudioCodec::kNotSpecified;
  int bit_rate = 0;
};


struct VideoStream {
  static Error TryParse(const Json::Value& root, VideoStream* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  Stream stream;
  VideoCodec codec = VideoCodec::kNotSpecified;
  SimpleFraction max_frame_rate;
  int max_bit_rate = 0;
  std::string protection;
  std::string profile;
  std::string level;
  std::vector<Resolution> resolutions;
  std::string error_recovery_mode;
};

struct Offer {
  static Error TryParse(const Json::Value& root, Offer* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  CastMode cast_mode = CastMode::kMirroring;
  std::vector<AudioStream> audio_streams;
  std::vector<VideoStream> video_streams;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_OFFER_MESSAGES_H_
