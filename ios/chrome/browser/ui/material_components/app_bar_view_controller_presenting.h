// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_VIEW_CONTROLLER_PRESENTING_H_
#define IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_VIEW_CONTROLLER_PRESENTING_H_

#import <UIKit/UIKit.h>

@class ChromeAppBarViewController;

// An object conforming to this protocol is capable of creating and managing an
// MDCAppBar. Typically, UIViewControllers can implement this protocol to
// configure the app bar they can also optionally be presenting.
@protocol AppBarViewControllerPresenting<NSObject>

// The installed app bar view controller, if any.
@property(nonatomic, readonly, strong)
    ChromeAppBarViewController* appBarViewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_MATERIAL_COMPONENTS_APP_BAR_VIEW_CONTROLLER_PRESENTING_H_
