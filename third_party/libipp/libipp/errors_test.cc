// Copyright 2022 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "errors.h"

#include <gtest/gtest.h>

#include "ipp_enums.h"

namespace ipp {
namespace {

TEST(AttrPath, StringRepresentation) {
  AttrPath path(GroupTag::job_attributes);
  path.PushBack(0, "abc>123");
  path.PushBack(123, "special: \n\"\t");
  EXPECT_EQ(path.AsString(),
            "job-attributes[0]>abc\\u003e123[123]>special: \\n\\\"\\t");
}

}  // namespace
}  // namespace ipp
