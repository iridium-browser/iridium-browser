// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <HelloShared/HelloShared.h>

#import "app/ViewController.h"

@implementation ViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  UILabel* label = [self labelWithText:[Greetings greet]];
  [self addCenteredView:label toParentView:self.view];
}

- (UILabel*)labelWithText:(NSString*)text {
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  label.text = text;
  [label sizeToFit];
  return label;
}

- (void)addCenteredView:(UIView*)view toParentView:(UIView*)parentView {
  view.center = [parentView convertPoint:parentView.center
                                fromView:parentView.superview];
  [parentView addSubview:view];
}

@end
