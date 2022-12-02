// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "gn/ninja_toolchain_writer.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

TEST(NinjaToolchainWriter, WriteToolRule) {
  TestWithScope setup;

  std::ostringstream stream;
  NinjaToolchainWriter writer(setup.settings(), setup.toolchain(), stream);
  writer.WriteToolRule(setup.toolchain()->GetTool(CTool::kCToolCc),
                       std::string("prefix_"));

  EXPECT_EQ(
      "rule prefix_cc\n"
      "  command = cc ${in} ${cflags} ${cflags_c} ${defines} ${include_dirs} "
      "-o ${out}\n",
      stream.str());
}

TEST(NinjaToolchainWriter, WriteToolRuleWithLauncher) {
  TestWithScope setup;

  std::ostringstream stream;
  NinjaToolchainWriter writer(setup.settings(), setup.toolchain(), stream);
  writer.WriteToolRule(setup.toolchain()->GetTool(CTool::kCToolCxx),
                       std::string("prefix_"));

  EXPECT_EQ(
      "rule prefix_cxx\n"
      "  command = launcher c++ ${in} ${cflags} ${cflags_cc} ${defines} ${include_dirs} "
      "-o ${out}\n",
      stream.str());
}
