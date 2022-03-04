// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/message_fields.h"

#include <array>
#include <cstring>
#include <vector>

#include "gtest/gtest.h"

namespace openscreen {
namespace cast {
namespace {

// NOTE: We don't do an exhaustive check of all values here, to avoid
// unnecessary duplication, but want to ensure that lookup is working properly.
TEST(MessageFieldsTest, CanParseEnumToString) {
  EXPECT_STREQ("aac", CodecToString(AudioCodec::kAac));
  EXPECT_STREQ("vp8", CodecToString(VideoCodec::kVp8));
}

TEST(MessageFieldsTest, CanStringToEnum) {
  EXPECT_EQ(AudioCodec::kOpus, StringToAudioCodec("opus").value());
  EXPECT_EQ(VideoCodec::kHevc, StringToVideoCodec("hevc").value());
}

TEST(MessageFieldsTest, Identity) {
  EXPECT_STREQ("opus", CodecToString(StringToAudioCodec("opus").value()));
  EXPECT_STREQ("vp8", CodecToString(StringToVideoCodec("vp8").value()));
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
