// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_constraints.h"

#include <memory>

#include "cast/streaming/constants.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {

TEST(ReceiverConstraintsTest, VideoLimitsIsSupersetOf) {
  VideoLimits first{};
  VideoLimits second = first;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.max_pixels_per_second += 1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.max_pixels_per_second = second.max_pixels_per_second;

  first.max_dimensions = {1921, 1090, {kDefaultFrameRate, 1}};
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));

  second.max_dimensions = {1921, 1090, {kDefaultFrameRate + 1, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  second.max_dimensions = {2000, 1000, {kDefaultFrameRate, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.max_dimensions = first.max_dimensions;

  first.min_bit_rate += 1;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.min_bit_rate = second.min_bit_rate;

  first.max_bit_rate += 1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.max_bit_rate = second.max_bit_rate;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.applies_to_all_codecs = true;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.applies_to_all_codecs = true;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.codec = VideoCodec::kVp8;
  second.codec = VideoCodec::kVp9;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.applies_to_all_codecs = false;
  second.applies_to_all_codecs = false;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
}

TEST(ReceiverConstraintsTest, AudioLimitsIsSupersetOf) {
  AudioLimits first{};
  AudioLimits second = first;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.max_sample_rate += 1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.max_sample_rate = second.max_sample_rate;

  first.max_channels += 1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.max_channels = second.max_channels;

  first.min_bit_rate += 1;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.min_bit_rate = second.min_bit_rate;

  first.max_bit_rate += 1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.max_bit_rate = second.max_bit_rate;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.applies_to_all_codecs = true;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.applies_to_all_codecs = true;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.codec = AudioCodec::kOpus;
  second.codec = AudioCodec::kAac;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.applies_to_all_codecs = false;
  second.applies_to_all_codecs = false;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
}

TEST(ReceiverConstraintsTest, DisplayIsSupersetOf) {
  Display first;
  Display second = first;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.dimensions = {1921, 1090, {kDefaultFrameRate, 1}};
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));

  second.dimensions = {1921, 1090, {kDefaultFrameRate + 1, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  second.dimensions = {2000, 1000, {kDefaultFrameRate, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.dimensions = first.dimensions;

  first.can_scale_content = true;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
}

TEST(ReceiverConstraintsTest, RemotingConstraintsIsSupersetOf) {
  RemotingConstraints first;
  RemotingConstraints second = first;

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  first.supports_chrome_audio_codecs = true;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));

  second.supports_4k = true;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));

  second.supports_chrome_audio_codecs = true;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
}

TEST(ReceiverConstraintsTest, ReceiverConstraintsIsSupersetOf) {
  ReceiverConstraints first;
  ReceiverConstraints second(first);

  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));

  // Modified |display_description|.
  first.display_description = std::make_unique<Display>();
  first.display_description->dimensions = {1920, 1080, {kDefaultFrameRate, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second = first;

  first.display_description->dimensions = {192, 1080, {kDefaultFrameRate, 1}};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  second = first;

  // Modified |remoting|.
  first.remoting = std::make_unique<RemotingConstraints>();
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second = first;

  second.remoting->supports_4k = true;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  second = first;

  // Modified |video_codecs|.
  first.video_codecs = {VideoCodec::kVp8, VideoCodec::kVp9};
  second.video_codecs = {};
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.video_codecs = {VideoCodec::kHevc};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.video_codecs.emplace_back(VideoCodec::kHevc);
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first = second;

  // Modified |audio_codecs|.
  first.audio_codecs = {AudioCodec::kOpus};
  second.audio_codecs = {};
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.audio_codecs = {AudioCodec::kAac};
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first.audio_codecs.emplace_back(AudioCodec::kAac);
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  first = second;

  // Modified |video_limits|.
  first.video_limits.push_back({true, VideoCodec::kVp8});
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.video_limits.front().min_bit_rate = -1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.video_limits.push_back({true, VideoCodec::kVp9});
  second.video_limits.front().min_bit_rate = -1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.video_limits.front().applies_to_all_codecs = false;
  first.video_limits.push_back({false, VideoCodec::kHevc, 123});
  second.video_limits.front().applies_to_all_codecs = false;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.video_limits.front().min_bit_rate = kDefaultVideoMinBitRate;
  first.video_limits.front().min_bit_rate = kDefaultVideoMinBitRate;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  second = first;

  // Modified |audio_limits|.
  first.audio_limits.push_back({true, AudioCodec::kOpus});
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.audio_limits.front().min_bit_rate = -1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
  second.audio_limits.push_back({true, AudioCodec::kAac});
  second.audio_limits.front().min_bit_rate = -1;
  EXPECT_TRUE(first.IsSupersetOf(second));
  EXPECT_TRUE(second.IsSupersetOf(first));
  first.audio_limits.front().applies_to_all_codecs = false;
  first.audio_limits.push_back({false, AudioCodec::kOpus, -1});
  second.audio_limits.front().applies_to_all_codecs = false;
  EXPECT_FALSE(first.IsSupersetOf(second));
  EXPECT_FALSE(second.IsSupersetOf(first));
}

}  // namespace cast
}  // namespace openscreen
