// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/emoji/emoji_panel_helper.h"

#import <Cocoa/Cocoa.h>

#include "base/feature_list.h"

namespace ui {

bool IsEmojiPanelSupported() {
  return true;
}

void ShowEmojiPanel() {
  [NSApp orderFrontCharacterPalette:nil];
}

}  // namespace ui
