// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/accessibility/accessibility_sound_proxy.h"

namespace chromecast {
namespace shell {

AccessibilitySoundProxy::AccessibilitySoundProxy(
    std::unique_ptr<AccessibilitySoundPlayer> player)
    : player_(std::move(player)) {}

AccessibilitySoundProxy::~AccessibilitySoundProxy() {}

void AccessibilitySoundProxy::ResetPlayer(
    std::unique_ptr<AccessibilitySoundPlayer> player) {
  player_ = std::move(player);
}

void AccessibilitySoundProxy::PlayPassthroughEarcon() {
  player_->PlayPassthroughEarcon();
}

void AccessibilitySoundProxy::PlayPassthroughEndEarcon() {
  player_->PlayPassthroughEndEarcon();
}

void AccessibilitySoundProxy::PlayEnterScreenEarcon() {
  player_->PlayEnterScreenEarcon();
}

void AccessibilitySoundProxy::PlayExitScreenEarcon() {
  player_->PlayExitScreenEarcon();
}

void AccessibilitySoundProxy::PlayTouchTypeEarcon() {
  player_->PlayTouchTypeEarcon();
}

}  // namespace shell
}  // namespace chromecast
