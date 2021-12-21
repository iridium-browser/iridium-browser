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
// corresponding Cast Streaming receiver application.
bool IsCastStreamingAppId(const std::string& app_id);
bool IsCastStreamingAudioVideoAppId(const std::string& app_id);
bool IsCastStreamingAudioOnlyAppId(const std::string& app_id);

// Returns all app IDs for Cast Streaming receivers.
std::vector<std::string> GetCastStreamingAppIds();

// Returns the app ID for the audio and video streaming receiver.
constexpr const char* GetCastStreamingAudioVideoAppId() {
  return "0F5096E8";
}

// Returns the app ID for the audio-only streaming receiver.
constexpr const char* GetCastStreamingAudioOnlyAppId() {
  return "85CDB22F";
}

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_PUBLIC_CAST_STREAMING_APP_IDS_H_
