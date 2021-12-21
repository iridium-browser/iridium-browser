// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PROXY_H_
#define CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PROXY_H_

#include <memory>

#include "chromecast/browser/accessibility/accessibility_sound_player.h"

namespace chromecast {
namespace shell {

// Provides audio feedback in screen reader mode.
class AccessibilitySoundProxy : public AccessibilitySoundPlayer {
 public:
  AccessibilitySoundProxy(std::unique_ptr<AccessibilitySoundPlayer> player);
  ~AccessibilitySoundProxy() override;

  void ResetPlayer(std::unique_ptr<AccessibilitySoundPlayer> player);

  // AccessibilitySoundPlayer overrides:
  void PlayPassthroughEarcon() override;
  void PlayPassthroughEndEarcon() override;
  void PlayEnterScreenEarcon() override;
  void PlayExitScreenEarcon() override;
  void PlayTouchTypeEarcon() override;

 private:
  std::unique_ptr<AccessibilitySoundPlayer> player_;
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PROXY_H_
