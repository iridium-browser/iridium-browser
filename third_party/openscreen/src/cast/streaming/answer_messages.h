// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_ANSWER_MESSAGES_H_
#define CAST_STREAMING_ANSWER_MESSAGES_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "cast/streaming/resolution.h"
#include "cast/streaming/ssrc.h"
#include "json/value.h"
#include "platform/base/error.h"
#include "util/simple_fraction.h"

namespace openscreen {
namespace cast {

// For each of the below classes, though a number of methods are shared, the use
// of a shared base class has intentionally been avoided. This is to improve
// readability of the structs provided in this file by cutting down on the
// amount of obscuring boilerplate code. For each of the following struct
// definitions, the following method definitions are shared:
// (1) TryParse. Shall return a boolean indicating whether the out
//     parameter is in a valid state after checking bounds and restrictions.
// (2) ToJson. Should return a proper JSON object. Assumes that IsValid()
//     has been called already, OSP_DCHECKs if not IsValid().
// (3) IsValid. Used by both TryParse and ToJson to ensure that the
//     object is in a good state.
struct AudioConstraints {
  static bool TryParse(const Json::Value& value, AudioConstraints* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  int max_sample_rate = 0;
  int max_channels = 0;
  int min_bit_rate = 0;  // optional
  int max_bit_rate = 0;
  absl::optional<std::chrono::milliseconds> max_delay = {};
};

struct VideoConstraints {
  static bool TryParse(const Json::Value& value, VideoConstraints* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  absl::optional<double> max_pixels_per_second = {};
  absl::optional<Dimensions> min_resolution = {};
  Dimensions max_dimensions = {};
  int min_bit_rate = 0;  // optional
  int max_bit_rate = 0;
  absl::optional<std::chrono::milliseconds> max_delay = {};
};

struct Constraints {
  static bool TryParse(const Json::Value& value, Constraints* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  AudioConstraints audio;
  VideoConstraints video;
};

// Decides whether the Sender scales and letterboxes content to 16:9, or if
// it may send video frames of any arbitrary size and the Receiver must
// handle the presentation details.
enum class AspectRatioConstraint : uint8_t { kVariable = 0, kFixed };

struct AspectRatio {
  static bool TryParse(const Json::Value& value, AspectRatio* out);
  bool IsValid() const;

  bool operator==(const AspectRatio& other) const {
    return width == other.width && height == other.height;
  }

  int width = 0;
  int height = 0;
};

struct DisplayDescription {
  static bool TryParse(const Json::Value& value, DisplayDescription* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  // May exceed, be the same, or less than those mentioned in the
  // video constraints.
  absl::optional<Dimensions> dimensions;
  absl::optional<AspectRatio> aspect_ratio = {};
  absl::optional<AspectRatioConstraint> aspect_ratio_constraint = {};
};

struct Answer {
  static bool TryParse(const Json::Value& value, Answer* out);
  Json::Value ToJson() const;
  bool IsValid() const;

  int udp_port = 0;
  std::vector<int> send_indexes;
  std::vector<Ssrc> ssrcs;

  // Constraints and display descriptions are optional fields, and maybe null in
  // the valid case.
  absl::optional<Constraints> constraints;
  absl::optional<DisplayDescription> display;
  std::vector<int> receiver_rtcp_event_log;
  std::vector<int> receiver_rtcp_dscp;

  // RTP extensions should be empty, but not null.
  std::vector<std::string> rtp_extensions = {};
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_ANSWER_MESSAGES_H_
