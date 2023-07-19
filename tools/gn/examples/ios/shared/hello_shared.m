// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "hello_shared.h"

@implementation Greetings

+ (NSString*)greet {
  return [NSString stringWithFormat:@"Hello from %@!", [Greetings displayName]];
}

+ (NSString*)greetWithName:(NSString*)name {
  return [NSString stringWithFormat:@"Hello, %@!", name];
}

+ (NSString*)displayName {
  NSBundle* bundle = [NSBundle bundleForClass:[Greetings class]];
  return [[bundle infoDictionary] objectForKey:@"CFBundleName"];
}

@end
