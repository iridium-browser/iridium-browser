// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_CAPTURE_RECOMMENDATIONS_H_
#define CAST_STREAMING_CAPTURE_RECOMMENDATIONS_H_

#include <chrono>
#include <cmath>
#include <memory>
#include <tuple>

#include "cast/streaming/constants.h"
#include "cast/streaming/resolution.h"
namespace openscreen {
namespace cast {

struct Answer;

// This namespace contains classes and functions to be used by senders for
// determining what constraints are recommended for the capture device, based on
// the limits reported by the receiver.
//
// A general note about recommendations: they are NOT maximum operational
// limits, instead they are targeted to provide a delightful cast experience.
// For example, if a receiver is connected to a 1080P display but cannot provide
// 1080P at a stable FPS with a good experience, 1080P will not be recommended.
namespace capture_recommendations {

// Default maximum delay for both audio and video. Used if the sender fails
// to provide any constraints.
constexpr std::chrono::milliseconds kDefaultMaxDelayMs(400);

// Bit rate limits, used for both audio and video streams.
struct BitRateLimits {
  bool operator==(const BitRateLimits& other) const;

  // Minimum bit rate, in bits per second.
  int minimum;

  // Maximum bit rate, in bits per second.
  int maximum;
};

// The mirroring control protocol specifies 32kbps as the absolute minimum
// for audio. Depending on the type of audio content (narrowband, fullband,
// etc.) Opus specifically can perform very well at this bitrate.
// See: https://research.google/pubs/pub41650/
constexpr int kDefaultAudioMinBitRate = 32 * 1000;

// Opus generally sees little improvement above 192kbps, but some older codecs
// that we may consider supporting improve at up to 256kbps.
constexpr int kDefaultAudioMaxBitRate = 256 * 1000;
constexpr BitRateLimits kDefaultAudioBitRateLimits{kDefaultAudioMinBitRate,
                                                   kDefaultAudioMaxBitRate};

// While generally audio should be captured at the maximum sample rate, 16kHz is
// the recommended absolute minimum.
constexpr int kDefaultAudioMinSampleRate = 16000;

// Audio capture recommendations. Maximum delay is determined by buffer
// constraints, and capture bit rate may vary between limits as appropriate.
struct Audio {
  bool operator==(const Audio& other) const;

  // Represents the recommended bit rate range.
  BitRateLimits bit_rate_limits = kDefaultAudioBitRateLimits;

  // Represents the maximum audio delay, in milliseconds.
  std::chrono::milliseconds max_delay = kDefaultMaxDelayMs;

  // Represents the maximum number of audio channels.
  int max_channels = kDefaultAudioChannels;

  // Represents the maximum samples per second.
  int max_sample_rate = kDefaultAudioSampleRate;

  // Represents the absolute minimum samples per second. Generally speaking,
  // audio should be captured at the maximum samples per second rate.
  int min_sample_rate = kDefaultAudioMinSampleRate;
};

// The minimum dimensions are as close as possible to low-definition
// television, factoring in the receiver's aspect ratio if provided.
constexpr Resolution kDefaultMinResolution{kMinVideoWidth, kMinVideoHeight};

// Currently mirroring only supports 1080P.
constexpr Dimensions kDefaultMaxResolution{1920, 1080, kDefaultFrameRate};

// The mirroring spec suggests 300kbps as the absolute minimum bitrate.
constexpr int kDefaultVideoMinBitRate = 300 * 1000;

// The theoretical maximum pixels per second is the maximum bit rate
// divided by 8 (the max byte rate). In practice it should generally be
// less.
constexpr int kDefaultVideoMaxPixelsPerSecond =
    kDefaultMaxResolution.effective_bit_rate() / 8;

// Our default limits are merely the product of the minimum and maximum
// dimensions, and are only used if the receiver fails to give better
// constraint information.
const BitRateLimits kDefaultVideoBitRateLimits{
    kDefaultVideoMinBitRate, kDefaultMaxResolution.effective_bit_rate()};

// Video capture recommendations.
struct Video {
  bool operator==(const Video& other) const;

  // Represents the recommended bit rate range.
  BitRateLimits bit_rate_limits = kDefaultVideoBitRateLimits;

  // Represents the recommended minimum resolution.
  Resolution minimum = kDefaultMinResolution;

  // Represents the recommended maximum resolution.
  Dimensions maximum = kDefaultMaxResolution;

  // Indicates whether the receiver can scale frames from a different aspect
  // ratio, or if it needs to be done by the sender. Default is false, meaning
  // that the sender is responsible for letterboxing.
  bool supports_scaling = false;

  // Represents the maximum video delay, in milliseconds.
  std::chrono::milliseconds max_delay = kDefaultMaxDelayMs;

  // Represents the maximum pixels per second, not necessarily correlated
  // to bit rate.
  int max_pixels_per_second = kDefaultVideoMaxPixelsPerSecond;
};

// Outputted recommendations for usage by capture devices. Note that we always
// return both audio and video (it is up to the sender to determine what
// streams actually get created). If the receiver doesn't give us any
// information for making recommendations, the defaults are used.
struct Recommendations {
  bool operator==(const Recommendations& other) const;

  // Audio specific recommendations.
  Audio audio;

  // Video specific recommendations.
  Video video;
};

Recommendations GetRecommendations(const Answer& answer);

}  // namespace capture_recommendations
}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_CAPTURE_RECOMMENDATIONS_H_
