// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/public/cast_streaming_app_ids.h"

#include <array>

#include "absl/strings/match.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {
namespace {

// clang-format off
constexpr std::array<const char*, 90>
    kRemoteDisplayAppStreamingAudioVideoAppIds {
  "35708D08",
  "3185A70D",
  "B7AD3A0A",
  "A4804AE7",
  "4D8921A4",
  "D2D81108",
  "059F814C",
  "71669BE0",
  "7FB4C367",
  "416FF5B9",
  "00871CBD",
  "0282CF4D",
  "C01EB1F7",
  "2A436AF9",
  "0770F479",
  "71F4DFC4",
  "0C67B152",
  "E28F3A40",
  "E902BBBA",
  "EDE9E5B9",
  "5EC94FF2",
  "286B3A4F",
  "7010B037",
  "5864F981",
  "58CF25DA",
  "78B2BFC1",
  "421960E6",
  "CBC00E99",
  "A2A9AF2C",
  "1CB039EF",
  "6793E1E7",
  "2387D4A1",
  "5087B30D",
  "7C0C9F50",
  "FC77DA3F",
  "6C458F04",
  "CEA5969D",
  "74195EB7",
  "225DAEF5",
  "FA90D53D",
  "31EF3B86",
  "B726D7EB",
  "BBCB9078",
  "2D1BE3EE",
  "5CEC893D",
  "D7B00E8A",
  "3A545E7B",
  "2F16175C",
  "4100A9E9",
  "1FF64E4D",
  "3C0C0AF5",
  "95C8E9FF",
  "A5885141",
  "3800F05A",
  "9F5E2418",
  "D4BA69B9",
  "EA6CC65A",
  "F35328DB",
  "83E15B05",
  "0462B988",
  "FCC0B8FA",
  "25DC3C2B",
  "B76A194D",
  "C3F1A2DC",
  "DB325A57",
  "2F245CEB",
  "EF20DD43",
  "92079423",
  "8ADE65BB",
  "D02F78DA",
  "6D8AC864",
  "B27C9432",
  "79DDF33F",
  "D61B8405",
  "5F963664",
  "35BA3487",
  "CEBB9229",
  "EF393A5F",
  "610525E9",
  "B51C97AB",
  "F3F3A706",
  "BFC9EB3B",
  "DA3F6592",
  "B857FB84",
  "8C18DDB3",
  "549E0B5D",
  "705554E6",
  "A7AFE586",
  "4D6272E5",
  "80C0A14F"
};
// clang-format on

}  // namespace

bool IsCastStreamingAppId(const std::string& app_id) {
  return IsCastStreamingAudioOnlyAppId(app_id) ||
         IsCastStreamingAudioVideoAppId(app_id);
}

bool IsCastStreamingAudioVideoAppId(const std::string& app_id) {
  return absl::EqualsIgnoreCase(app_id, GetCastStreamingAudioVideoAppId());
}

bool IsCastStreamingAudioOnlyAppId(const std::string& app_id) {
  return absl::EqualsIgnoreCase(app_id, GetCastStreamingAudioOnlyAppId());
}

bool IsCastStreamingReceiverAppId(const std::string& app_id) {
  if (absl::EqualsIgnoreCase(app_id, GetCastStreamingAudioVideoAppId()) ||
      absl::EqualsIgnoreCase(app_id, GetCastStreamingAudioOnlyAppId()) ||
      absl::EqualsIgnoreCase(app_id, GetAndroidMirroringAudioVideoAppId()) ||
      absl::EqualsIgnoreCase(app_id, GetAndroidMirroringAudioOnlyAppId()) ||
      absl::EqualsIgnoreCase(app_id, GetAndroidAppStreamingAudioVideoAppId()) ||
      absl::EqualsIgnoreCase(app_id, GetIosAppStreamingAudioVideoAppId())) {
    return true;
  }

  return ContainsIf(kRemoteDisplayAppStreamingAudioVideoAppIds,
                    [app_id](const std::string& id) {
                      return absl::EqualsIgnoreCase(id, app_id);
                    });
}

std::vector<std::string> GetCastStreamingAppIds() {
  return std::vector<std::string>(
      {GetCastStreamingAudioVideoAppId(), GetCastStreamingAudioOnlyAppId()});
}

}  // namespace cast
}  // namespace openscreen
