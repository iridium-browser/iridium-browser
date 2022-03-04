// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/context_menu/reading_list_context_menu_params.h"

#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ReadingListContextMenuParams
@synthesize title = _title;
@synthesize message = _message;
@synthesize rect = _rect;
@synthesize view = _view;
@synthesize entryURL = _entryURL;
@synthesize offlineURL = _offlineURL;
@end
