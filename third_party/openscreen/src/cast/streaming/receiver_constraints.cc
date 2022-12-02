// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_constraints.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {

namespace {
// Calculates whether any codecs present in |second| are not present in |first|.
template <typename T>
bool IsMissingCodecs(const std::vector<T>& first,
                     const std::vector<T>& second) {
  if (second.size() > first.size()) {
    return true;
  }

  for (auto codec : second) {
    if (!Contains(first, codec)) {
      return true;
    }
  }

  return false;
}

// Calculates whether the limits defined by |first| are less restrictive than
// those defined by |second|.
// NOTE: These variables are intentionally passed by copy - the function will
// mutate them.
template <typename T>
bool HasLessRestrictiveLimits(std::vector<T> first, std::vector<T> second) {
  // Sort both vectors to allow for element-by-element comparison between the
  // two. All elements with |applies_to_all_codecs| set are sorted to the front.
  std::function<bool(const T&, const T&)> sorter = [](const T& x, const T& y) {
    if (x.applies_to_all_codecs != y.applies_to_all_codecs) {
      return x.applies_to_all_codecs;
    }
    return static_cast<int>(x.codec) < static_cast<int>(y.codec);
  };
  std::sort(first.begin(), first.end(), sorter);
  std::sort(second.begin(), second.end(), sorter);
  auto first_it = first.begin();
  auto second_it = second.begin();

  // |applies_to_all_codecs| is a special case, so handle that first.
  T fake_applies_to_all_codecs_struct;
  fake_applies_to_all_codecs_struct.applies_to_all_codecs = true;
  T* first_applies_to_all_codecs_struct =
      !first.empty() && first.front().applies_to_all_codecs
          ? &(*first_it++)
          : &fake_applies_to_all_codecs_struct;
  T* second_applies_to_all_codecs_struct =
      !second.empty() && second.front().applies_to_all_codecs
          ? &(*second_it++)
          : &fake_applies_to_all_codecs_struct;
  if (!first_applies_to_all_codecs_struct->IsSupersetOf(
          *second_applies_to_all_codecs_struct)) {
    return false;
  }

  // Now all elements of the vectors can be assumed to NOT have
  // |applies_to_all_codecs| set. So iterate through all codecs set in either
  // vector and check that the first has the less restrictive configuration set.
  while (first_it != first.end() || second_it != second.end()) {
    // Calculate the current codec to process, and whether each vector contains
    // an instance of this codec.
    decltype(T::codec) current_codec;
    bool use_first_fake = false;
    bool use_second_fake = false;
    if (first_it == first.end()) {
      current_codec = second_it->codec;
      use_first_fake = true;
    } else if (second_it == second.end()) {
      current_codec = first_it->codec;
      use_second_fake = true;
    } else {
      current_codec = std::min(first_it->codec, second_it->codec);
      use_first_fake = first_it->codec != current_codec;
      use_second_fake = second_it->codec != current_codec;
    }

    // Compare each vector's limit associated with this codec, or compare
    // against the default limits if no such codec limits are set.
    T fake_codecs_struct;
    fake_codecs_struct.codec = current_codec;
    T* first_codec_struct =
        use_first_fake ? &fake_codecs_struct : &(*first_it++);
    T* second_codec_struct =
        use_second_fake ? &fake_codecs_struct : &(*second_it++);
    OSP_DCHECK(!first_codec_struct->applies_to_all_codecs);
    OSP_DCHECK(!second_codec_struct->applies_to_all_codecs);
    if (!first_codec_struct->IsSupersetOf(*second_codec_struct)) {
      return false;
    }
  }

  return true;
}

}  // namespace

bool Display::IsSupersetOf(const Display& other) const {
  return dimensions.IsSupersetOf(other.dimensions) &&
         (can_scale_content || !other.can_scale_content);
}

bool AudioLimits::IsSupersetOf(const AudioLimits& second) const {
  return (applies_to_all_codecs == second.applies_to_all_codecs) &&
         (applies_to_all_codecs || codec == second.codec) &&
         (max_sample_rate >= second.max_sample_rate) &&
         (max_channels >= second.max_channels) &&
         (min_bit_rate <= second.min_bit_rate) &&
         (max_bit_rate >= second.max_bit_rate) &&
         (max_delay >= second.max_delay);
}

bool VideoLimits::IsSupersetOf(const VideoLimits& second) const {
  return (applies_to_all_codecs == second.applies_to_all_codecs) &&
         (applies_to_all_codecs || codec == second.codec) &&
         (max_pixels_per_second >= second.max_pixels_per_second) &&
         (min_bit_rate <= second.min_bit_rate) &&
         (max_bit_rate >= second.max_bit_rate) &&
         (max_delay >= second.max_delay) &&
         (max_dimensions.IsSupersetOf(second.max_dimensions));
}

bool RemotingConstraints::IsSupersetOf(const RemotingConstraints& other) const {
  return (supports_chrome_audio_codecs ||
          !other.supports_chrome_audio_codecs) &&
         (supports_4k || !other.supports_4k);
}

ReceiverConstraints::ReceiverConstraints() = default;
ReceiverConstraints::ReceiverConstraints(std::vector<VideoCodec> video_codecs,
                                         std::vector<AudioCodec> audio_codecs)
    : video_codecs(std::move(video_codecs)),
      audio_codecs(std::move(audio_codecs)) {}

ReceiverConstraints::ReceiverConstraints(std::vector<VideoCodec> video_codecs,
                                         std::vector<AudioCodec> audio_codecs,
                                         std::vector<AudioLimits> audio_limits,
                                         std::vector<VideoLimits> video_limits,
                                         std::unique_ptr<Display> description)
    : video_codecs(std::move(video_codecs)),
      audio_codecs(std::move(audio_codecs)),
      audio_limits(std::move(audio_limits)),
      video_limits(std::move(video_limits)),
      display_description(std::move(description)) {}

ReceiverConstraints::ReceiverConstraints(ReceiverConstraints&&) noexcept =
    default;
ReceiverConstraints& ReceiverConstraints::operator=(
    ReceiverConstraints&&) noexcept = default;

ReceiverConstraints::ReceiverConstraints(const ReceiverConstraints& other) {
  *this = other;
}

ReceiverConstraints& ReceiverConstraints::operator=(
    const ReceiverConstraints& other) {
  video_codecs = other.video_codecs;
  audio_codecs = other.audio_codecs;
  audio_limits = other.audio_limits;
  video_limits = other.video_limits;
  if (other.display_description) {
    display_description = std::make_unique<Display>(*other.display_description);
  }
  if (other.remoting) {
    remoting = std::make_unique<RemotingConstraints>(*other.remoting);
  }
  return *this;
}

bool ReceiverConstraints::IsSupersetOf(const ReceiverConstraints& other) const {
  // Check simple cases first.
  if ((!!display_description != !!other.display_description) ||
      (display_description &&
       !display_description->IsSupersetOf(*other.display_description))) {
    return false;
  } else if (other.remoting &&
             (!remoting || !remoting->IsSupersetOf(*other.remoting))) {
    return false;
  }

  // Then check set codecs.
  if (IsMissingCodecs(video_codecs, other.video_codecs) ||
      IsMissingCodecs(audio_codecs, other.audio_codecs)) {
    return false;
  }

  // Then check limits. Do this last because it's the most resource intensive to
  // check.
  return HasLessRestrictiveLimits(video_limits, other.video_limits) &&
         HasLessRestrictiveLimits(audio_limits, other.audio_limits);
}

}  // namespace cast
}  // namespace openscreen
