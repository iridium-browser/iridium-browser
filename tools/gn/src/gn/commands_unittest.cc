// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "gn/commands.h"
#include "gn/label_pattern.h"
#include "gn/target.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

TEST(Commands, FilterOutMatch) {
  TestWithScope setup;
  SourceDir current_dir("//");

  Target target_afoo(setup.settings(), Label(SourceDir("//a/"), "foo"));
  Target target_cbar(setup.settings(), Label(SourceDir("//c/"), "bar"));
  std::vector<const Target*> targets{&target_afoo, &target_cbar};

  Err err;
  LabelPattern pattern_a = LabelPattern::GetPattern(
      current_dir, std::string_view(), Value(nullptr, "//a:*"), &err);
  EXPECT_FALSE(err.has_error());
  LabelPattern pattern_ef = LabelPattern::GetPattern(
      current_dir, std::string_view(), Value(nullptr, "//e:f"), &err);
  EXPECT_FALSE(err.has_error());
  std::vector<LabelPattern> label_patterns{pattern_a, pattern_ef};

  std::vector<const Target*> output;
  commands::FilterOutTargetsByPatterns(targets, label_patterns, &output);

  EXPECT_EQ(1, output.size());
  EXPECT_EQ(&target_cbar, output[0]);
}
