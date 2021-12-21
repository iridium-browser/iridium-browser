// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/frame/window_frame_util.h"

#include "ui/gfx/geometry/size.h"

// static
SkAlpha WindowFrameUtil::CalculateWindows10GlassCaptionButtonBackgroundAlpha(
    SkAlpha theme_alpha) {
  return theme_alpha == SK_AlphaOPAQUE ? 0xCC : theme_alpha;
}

// static
gfx::Size WindowFrameUtil::GetWindows10GlassCaptionButtonAreaSize() {
  constexpr int kNumButtons = 3;

  return gfx::Size(
      (kNumButtons * kWindows10GlassCaptionButtonWidth) +
          ((kNumButtons - 1) * kWindows10GlassCaptionButtonVisualSpacing),
      kWindows10GlassCaptionButtonHeightRestored);
}
