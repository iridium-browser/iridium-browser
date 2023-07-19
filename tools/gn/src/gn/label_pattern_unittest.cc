// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <iterator>

#include "gn/err.h"
#include "gn/label_pattern.h"
#include "gn/value.h"
#include "util/test/test.h"

namespace {

struct PatternCase {
  const char* input;
  bool success;

  LabelPattern::Type type;
  const char* dir;
  const char* name;
  const char* toolchain;
};

}  // namespace

TEST(LabelPattern, PatternParse) {
  SourceDir current_dir("//foo/");
  PatternCase cases[] = {
    // Missing stuff.
    {"", false, LabelPattern::MATCH, "", "", ""},
    {":", false, LabelPattern::MATCH, "", "", ""},
    // Normal things.
    {":bar", true, LabelPattern::MATCH, "//foo/", "bar", ""},
    {"//la:bar", true, LabelPattern::MATCH, "//la/", "bar", ""},
    {"*", true, LabelPattern::RECURSIVE_DIRECTORY, "", "", ""},
    {":*", true, LabelPattern::DIRECTORY, "//foo/", "", ""},
    {"la:*", true, LabelPattern::DIRECTORY, "//foo/la/", "", ""},
    {"la/*:*", true, LabelPattern::RECURSIVE_DIRECTORY, "//foo/la/", "", ""},
    {"//la:*", true, LabelPattern::DIRECTORY, "//la/", "", ""},
    {"./*", true, LabelPattern::RECURSIVE_DIRECTORY, "//foo/", "", ""},
    {"foo/*", true, LabelPattern::RECURSIVE_DIRECTORY, "//foo/foo/", "", ""},
    {"//l/*", true, LabelPattern::RECURSIVE_DIRECTORY, "//l/", "", ""},
    // Toolchains.
    {"//foo()", true, LabelPattern::MATCH, "//foo/", "foo", ""},
    {"//foo(//bar)", true, LabelPattern::MATCH, "//foo/", "foo", "//bar:bar"},
    {"//foo:*(//bar)", true, LabelPattern::DIRECTORY, "//foo/", "",
     "//bar:bar"},
    {"//foo/*(//bar)", true, LabelPattern::RECURSIVE_DIRECTORY, "//foo/", "",
     "//bar:bar"},
    // Wildcards in invalid places.
    {"*foo*:bar", false, LabelPattern::MATCH, "", "", ""},
    {"foo*:*bar", false, LabelPattern::MATCH, "", "", ""},
    {"*foo:bar", false, LabelPattern::MATCH, "", "", ""},
    {"foo:bar*", false, LabelPattern::MATCH, "", "", ""},
    {"*:*", true, LabelPattern::RECURSIVE_DIRECTORY, "", "", ""},
    // Invalid toolchain stuff.
    {"//foo(//foo/bar:*)", false, LabelPattern::MATCH, "", "", ""},
    {"//foo/*(*)", false, LabelPattern::MATCH, "", "", ""},
    {"//foo(//bar", false, LabelPattern::MATCH, "", "", ""},
    // Absolute paths.
    {"/la/*", true, LabelPattern::RECURSIVE_DIRECTORY, "/la/", "", ""},
    {"/la:bar", true, LabelPattern::MATCH, "/la/", "bar", ""},
#if defined(OS_WIN)
    {"/C:/la/*", true, LabelPattern::RECURSIVE_DIRECTORY, "/C:/la/", "", ""},
    {"C:/la/*", true, LabelPattern::RECURSIVE_DIRECTORY, "/C:/la/", "", ""},
    {"/C:/la:bar", true, LabelPattern::MATCH, "/C:/la/", "bar", ""},
    {"C:/la:bar", true, LabelPattern::MATCH, "/C:/la/", "bar", ""},
    {"C:foo", true, LabelPattern::MATCH, "//foo/C/", "foo", ""},
#endif
  };

  for (size_t i = 0; i < std::size(cases); i++) {
    const PatternCase& cur = cases[i];
    Err err;
    LabelPattern result = LabelPattern::GetPattern(
        current_dir, std::string_view(), Value(nullptr, cur.input), &err);

    EXPECT_EQ(cur.success, !err.has_error()) << i << " " << cur.input;
    EXPECT_EQ(cur.type, result.type()) << i << " " << cur.input;
    EXPECT_EQ(cur.dir, result.dir().value()) << i << " " << cur.input;
    EXPECT_EQ(cur.name, result.name()) << i << " " << cur.input;
    EXPECT_EQ(cur.toolchain, result.toolchain().GetUserVisibleName(false))
        << i << " " << cur.input;
  }
}

// Tests a non-empty source root which allows patterns to reference above the
// source root.
TEST(LabelPattern, PatternParseAboveSourceRoot) {
  SourceDir current_dir("//foo/");
  std::string source_root = "/foo/bar/baz/";

  Err err;
  LabelPattern result = LabelPattern::GetPattern(
      current_dir, source_root, Value(nullptr, "../../../*"), &err);
  ASSERT_FALSE(err.has_error());

  EXPECT_EQ(LabelPattern::RECURSIVE_DIRECTORY, result.type());
  EXPECT_EQ("/foo/", result.dir().value()) << result.dir().value();
}
