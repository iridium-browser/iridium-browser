// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_PUBLIC_CAST_STREAMING_APP_IDS_H_
#define CAST_COMMON_PUBLIC_CAST_STREAMING_APP_IDS_H_

#include <string>
#include <vector>

namespace openscreen {
namespace cast {

// Returns true only if |app_id| matches the Cast application ID for the
// corresponding Chromium Cast Streaming receiver application.
//
// TODO(b/204583004): Use std::string_view instead of std::string following the
// move to C++17 so that these comparisons can be made constexpr.
bool IsCastStreamingAppId(const std::string& app_id);
bool IsCastStreamingAudioVideoAppId(const std::string& app_id);
bool IsCastStreamingAudioOnlyAppId(const std::string& app_id);

// Returns true only if |app_id| matches any Cast Streaming app ID.
bool IsCastStreamingReceiverAppId(const std::string& app_id);

// Returns all app IDs for Cast Streaming receivers.
std::vector<std::string> GetCastStreamingAppIds();

// Returns the app ID for for the default audio and video streaming receiver.
constexpr const char* GetCastStreamingAudioVideoAppId() {
  return "0F5096E8";
}

// Returns the app ID for the for the default audio-only streaming receiver.
constexpr const char* GetCastStreamingAudioOnlyAppId() {
  return "85CDB22F";
}

// Returns the app ID for for the Android audio and video streaming receiver.
constexpr const char* GetAndroidMirroringAudioVideoAppId() {
  return "674A0243";
}

// Returns the app ID for for the Android audio-only streaming receiver.
constexpr const char* GetAndroidMirroringAudioOnlyAppId() {
  return "8E6C866D";
}

// Returns the app ID for the audio and video streaming receiver used by Android
// apps.
constexpr const char* GetAndroidAppStreamingAudioVideoAppId() {
  return "96084372";
}

// Returns the app ID for the audio and video streaming receiver used by iOS
// apps.
constexpr const char* GetIosAppStreamingAudioVideoAppId() {
  return "BFD92C23";
}

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_PUBLIC_CAST_STREAMING_APP_IDS_H_
