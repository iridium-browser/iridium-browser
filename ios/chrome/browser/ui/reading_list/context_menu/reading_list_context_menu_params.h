// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_CONTEXT_MENU_READING_LIST_CONTEXT_MENU_PARAMS_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_CONTEXT_MENU_READING_LIST_CONTEXT_MENU_PARAMS_H_

#import <UIKit/UIKit.h>

class GURL;

// Parameter object used to construct ReadingListContextMenuCoordinators.
@interface ReadingListContextMenuParams : NSObject

// The context menu title.
@property(nonatomic, copy) NSString* title;
// The context menu message.
@property(nonatomic, copy) NSString* message;
// The location within |view| from which to display the context menu.
@property(nonatomic, assign) CGRect rect;
// The view from which the context menu should be displayed.
@property(nonatomic, weak) UIView* view;
// The URL of the reading list entry.
@property(nonatomic, assign) GURL entryURL;
// The offline URL of the reading list entry.
@property(nonatomic, assign) GURL offlineURL;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_CONTEXT_MENU_READING_LIST_CONTEXT_MENU_PARAMS_H_
