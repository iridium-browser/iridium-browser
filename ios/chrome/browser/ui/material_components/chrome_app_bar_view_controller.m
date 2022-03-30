// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/material_components/chrome_app_bar_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ChromeAppBarViewController

// Return NO so same method signal can be passed to view controller or its
// navigation controller.
- (BOOL)accessibilityPerformEscape {
  return NO;
}

@end
