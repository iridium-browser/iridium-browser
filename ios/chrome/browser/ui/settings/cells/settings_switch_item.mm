// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/settings_switch_item.h"

#import "ios/chrome/browser/ui/settings/cells/settings_switch_cell.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SettingsSwitchItem

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [SettingsSwitchCell class];
    self.enabled = YES;
  }
  return self;
}

#pragma mark TableViewItem

- (void)configureCell:(SettingsSwitchCell*)cell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:cell withStyler:styler];
  cell.textLabel.text = self.text;
  cell.detailTextLabel.text = self.detailText;
  cell.switchView.enabled = self.enabled;
  cell.switchView.on = self.on;
  cell.textLabel.textColor =
      [SettingsSwitchCell defaultTextColorForState:cell.switchView.state];
  cell.selectionStyle = UITableViewCellSelectionStyleNone;

  // Update the icon image, if one is present.
  UIImage* iconImage = nil;
  if ([self.iconImageName length]) {
    iconImage = [UIImage imageNamed:self.iconImageName];
  }
  [cell setIconImage:iconImage];
}

@end
