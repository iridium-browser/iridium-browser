// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PLAYER_H_
#define CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PLAYER_H_

namespace chromecast {
namespace shell {

// Provides audio feedback in screen reader mode.
class AccessibilitySoundPlayer {
 public:
  AccessibilitySoundPlayer() {}
  virtual ~AccessibilitySoundPlayer() {}
  virtual void PlayPassthroughEarcon() {}
  virtual void PlayPassthroughEndEarcon() {}
  virtual void PlayEnterScreenEarcon() {}
  virtual void PlayExitScreenEarcon() {}
  virtual void PlayTouchTypeEarcon() {}
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_ACCESSIBILITY_ACCESSIBILITY_SOUND_PLAYER_H_
