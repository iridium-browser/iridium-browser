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

  EXPECT_TRUE(IsCastStreamingReceiverAppId("0F5096E8"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("0F5096e8"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("85CDB22F"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("85cdB22F"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("674A0243"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("8E6C866D"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("8e6C866D"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("96084372"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("BFD92C23"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("35708D08"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("35708d08"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("E28F3A40"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("5864F981"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("225DAEF5"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("225daef5"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("D4BA69B9"));
  EXPECT_TRUE(IsCastStreamingReceiverAppId("B27C9432"));
  EXPECT_FALSE(IsCastStreamingReceiverAppId("DEADBEEF"));
  EXPECT_FALSE(IsCastStreamingReceiverAppId(""));
  EXPECT_FALSE(IsCastStreamingReceiverAppId("foo"));

  std::vector<std::string> app_ids(GetCastStreamingAppIds());
  EXPECT_EQ(static_cast<size_t>(2), app_ids.size());
  EXPECT_TRUE(std::find(app_ids.begin(), app_ids.end(), "0F5096E8") !=
              app_ids.end());
  EXPECT_TRUE(std::find(app_ids.begin(), app_ids.end(), "85CDB22F") !=
              app_ids.end());

  EXPECT_STREQ("0F5096E8", GetCastStreamingAudioVideoAppId());
  EXPECT_STREQ("85CDB22F", GetCastStreamingAudioOnlyAppId());
  EXPECT_STREQ("674A0243", GetAndroidMirroringAudioVideoAppId());
  EXPECT_STREQ("8E6C866D", GetAndroidMirroringAudioOnlyAppId());
  EXPECT_STREQ("96084372", GetAndroidAppStreamingAudioVideoAppId());
  EXPECT_STREQ("BFD92C23", GetIosAppStreamingAudioVideoAppId());
}

}  // namespace cast
}  // namespace openscreen
