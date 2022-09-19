// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/capture_recommendations.h"

#include <algorithm>
#include <utility>

#include "cast/streaming/answer_messages.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace capture_recommendations {
namespace {

void ApplyDisplay(const DisplayDescription& description,
                  Recommendations* recommendations) {
  recommendations->video.supports_scaling =
      (description.aspect_ratio_constraint &&
       (description.aspect_ratio_constraint.value() ==
        AspectRatioConstraint::kVariable));

  // We should never exceed the display's resolution, since it will always
  // force scaling.
  if (description.dimensions) {
    recommendations->video.maximum = description.dimensions.value();
    recommendations->video.bit_rate_limits.maximum =
        recommendations->video.maximum.effective_bit_rate();

    if (recommendations->video.maximum.width <
        recommendations->video.minimum.width) {
      recommendations->video.minimum =
          recommendations->video.maximum.ToResolution();
    }
  }

  // If the receiver gives us an aspect ratio that doesn't match the display
  // resolution they give us, the behavior is undefined from the spec.
  // Here we prioritize the aspect ratio, and the receiver can scale the frame
  // as they wish.
  double aspect_ratio = 0.0;
  if (description.aspect_ratio) {
    aspect_ratio = static_cast<double>(description.aspect_ratio->width) /
                   description.aspect_ratio->height;
    recommendations->video.maximum.width =
        recommendations->video.maximum.height * aspect_ratio;
  } else if (description.dimensions) {
    aspect_ratio = static_cast<double>(description.dimensions->width) /
                   description.dimensions->height;
  } else {
    return;
  }
  recommendations->video.minimum.width =
      recommendations->video.minimum.height * aspect_ratio;
}

void ApplyConstraints(const Constraints& constraints,
                      Recommendations* recommendations) {
  // Audio has no fields in the display description, so we can safely
  // ignore the current recommendations when setting values here.
  if (constraints.audio.max_delay.has_value()) {
    recommendations->audio.max_delay = constraints.audio.max_delay.value();
  }
  recommendations->audio.max_channels = constraints.audio.max_channels;
  recommendations->audio.max_sample_rate = constraints.audio.max_sample_rate;

  recommendations->audio.bit_rate_limits = BitRateLimits{
      std::max(constraints.audio.min_bit_rate, kDefaultAudioMinBitRate),
      std::max(constraints.audio.max_bit_rate, kDefaultAudioMinBitRate)};

  // With video, we take the intersection of values of the constraints and
  // the display description.
  if (constraints.video.max_delay.has_value()) {
    recommendations->video.max_delay = constraints.video.max_delay.value();
  }

  if (constraints.video.max_pixels_per_second.has_value()) {
    recommendations->video.max_pixels_per_second =
        constraints.video.max_pixels_per_second.value();
  }

  recommendations->video.bit_rate_limits =
      BitRateLimits{std::max(constraints.video.min_bit_rate,
                             recommendations->video.bit_rate_limits.minimum),
                    std::min(constraints.video.max_bit_rate,
                             recommendations->video.bit_rate_limits.maximum)};
  Dimensions dimensions = constraints.video.max_dimensions;
  if (dimensions.width <= kDefaultMinResolution.width) {
    recommendations->video.maximum = {kDefaultMinResolution.width,
                                      kDefaultMinResolution.height,
                                      kDefaultFrameRate};
  } else if (dimensions.width < recommendations->video.maximum.width) {
    recommendations->video.maximum = std::move(dimensions);
  }

  if (constraints.video.min_resolution) {
    const Resolution& min = constraints.video.min_resolution->ToResolution();
    if (kDefaultMinResolution.width < min.width) {
      recommendations->video.minimum = std::move(min);
    }
  }
}

}  // namespace

bool BitRateLimits::operator==(const BitRateLimits& other) const {
  return std::tie(minimum, maximum) == std::tie(other.minimum, other.maximum);
}

bool Audio::operator==(const Audio& other) const {
  return std::tie(bit_rate_limits, max_delay, max_channels, max_sample_rate) ==
         std::tie(other.bit_rate_limits, other.max_delay, other.max_channels,
                  other.max_sample_rate);
}

bool Video::operator==(const Video& other) const {
  return std::tie(bit_rate_limits, minimum, maximum, supports_scaling,
                  max_delay, max_pixels_per_second) ==
         std::tie(other.bit_rate_limits, other.minimum, other.maximum,
                  other.supports_scaling, other.max_delay,
                  other.max_pixels_per_second);
}

bool Recommendations::operator==(const Recommendations& other) const {
  return std::tie(audio, video) == std::tie(other.audio, other.video);
}

Recommendations GetRecommendations(const Answer& answer) {
  Recommendations recommendations;
  if (answer.display.has_value() && answer.display->IsValid()) {
    ApplyDisplay(answer.display.value(), &recommendations);
  }
  if (answer.constraints.has_value() && answer.constraints->IsValid()) {
    ApplyConstraints(answer.constraints.value(), &recommendations);
  }
  return recommendations;
}

}  // namespace capture_recommendations
}  // namespace cast
}  // namespace openscreen
