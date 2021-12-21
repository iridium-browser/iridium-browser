// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/public/cast_streaming_app_ids.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace openscreen {
namespace cast {

TEST(CastStreamingAppIdsTest, Test) {
  EXPECT_TRUE(IsCastStreamingAppId("0F5096E8"));
  EXPECT_TRUE(IsCastStreamingAppId("85CDB22F"));
  EXPECT_FALSE(IsCastStreamingAppId("DEADBEEF"));

  EXPECT_TRUE(IsCastStreamingAudioVideoAppId("0F5096E8"));
  EXPECT_FALSE(IsCastStreamingAudioVideoAppId("85CDB22F"));
  EXPECT_FALSE(IsCastStreamingAudioVideoAppId("DEADBEEF"));

  EXPECT_FALSE(IsCastStreamingAudioOnlyAppId("0F5096E8"));
  EXPECT_TRUE(IsCastStreamingAudioOnlyAppId("85CDB22F"));
  EXPECT_FALSE(IsCastStreamingAudioOnlyAppId("DEADBEEF"));

  std::vector<std::string> app_ids(GetCastStreamingAppIds());
  EXPECT_EQ(static_cast<size_t>(2), app_ids.size());
  EXPECT_TRUE(std::find(app_ids.begin(), app_ids.end(), "0F5096E8") !=
              app_ids.end());
  EXPECT_TRUE(std::find(app_ids.begin(), app_ids.end(), "85CDB22F") !=
              app_ids.end());

  EXPECT_STREQ("0F5096E8", GetCastStreamingAudioVideoAppId());
  EXPECT_STREQ("85CDB22F", GetCastStreamingAudioOnlyAppId());
}

}  // namespace cast
}  // namespace openscreen
