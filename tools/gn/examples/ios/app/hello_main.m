// Copyright 2023 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#import "app/AppDelegate.h"

int hello_main(int argc, char** argv) {
  NSString* appDelegateClassName;
  @autoreleasepool {
    appDelegateClassName = NSStringFromClass([AppDelegate class]);
  }

  return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
