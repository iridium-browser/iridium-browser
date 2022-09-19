// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/frameworks_utils.h"

#include "util/test/test.h"

TEST(FrameworksUtils, GetFrameworkName) {
  const std::string kFramework = "Foundation.framework";
  const std::string kFrameworkNoExtension = "Foundation";
  const std::string kFrameworkPath = "Foo/Foo.framework";

  EXPECT_EQ("Foundation", GetFrameworkName(kFramework));
  EXPECT_EQ("", GetFrameworkName(kFrameworkNoExtension));
  EXPECT_EQ("", GetFrameworkName(kFrameworkPath));
}
