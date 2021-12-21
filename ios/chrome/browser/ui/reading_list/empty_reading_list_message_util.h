// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_EMPTY_READING_LIST_MESSAGE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_EMPTY_READING_LIST_MESSAGE_UTIL_H_

#import <UIKit/UIKit.h>

// Returns the attributed message to use for the empty Reading List background
// view.
NSAttributedString* GetReadingListEmptyMessage();

// Returns the accessibility label to use for the label displaying the text
// returned by GetReadingListEmptyMessage().
NSString* GetReadingListEmptyMessageA11yLabel();

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_EMPTY_READING_LIST_MESSAGE_UTIL_H_
