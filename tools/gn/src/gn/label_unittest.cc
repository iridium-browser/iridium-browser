// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <iterator>

#include "gn/err.h"
#include "gn/label.h"
#include "gn/value.h"
#include "util/build_config.h"
#include "util/test/test.h"

namespace {

struct ParseDepStringCase {
  const char* cur_dir;
  const char* str;
  bool success;
  const char* expected_dir;
  const char* expected_name;
  const char* expected_toolchain_dir;
  const char* expected_toolchain_name;
};

}  // namespace

TEST(Label, Resolve) {
  ParseDepStringCase cases[] = {
    {"//chrome/", "", false, "", "", "", ""},
    {"//chrome/", "/", false, "", "", "", ""},
    {"//chrome/", ":", false, "", "", "", ""},
    {"//chrome/", "/:", false, "", "", "", ""},
    {"//chrome/", "blah", true, "//chrome/blah/", "blah", "//t/", "d"},
    {"//chrome/", "blah:bar", true, "//chrome/blah/", "bar", "//t/", "d"},
    // Absolute paths.
    {"//chrome/", "/chrome:bar", true, "/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "/chrome/:bar", true, "/chrome/", "bar", "//t/", "d"},
#if defined(OS_WIN)
    {"//chrome/", "/C:/chrome:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "/C:/chrome/:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
    {"//chrome/", "C:/chrome:bar", true, "/C:/chrome/", "bar", "//t/", "d"},
#endif
    // Refers to root dir.
    {"//chrome/", "//:bar", true, "//", "bar", "//t/", "d"},
    // Implicit directory
    {"//chrome/", ":bar", true, "//chrome/", "bar", "//t/", "d"},
    {"//chrome/renderer/", ":bar", true, "//chrome/renderer/", "bar", "//t/",
     "d"},
    // Implicit names.
    {"//chrome/", "//base", true, "//base/", "base", "//t/", "d"},
    {"//chrome/", "//base/i18n", true, "//base/i18n/", "i18n", "//t/", "d"},
    {"//chrome/", "//base/i18n:foo", true, "//base/i18n/", "foo", "//t/", "d"},
    {"//chrome/", "//", false, "", "", "", ""},
    // Toolchain parsing.
    {"//chrome/", "//chrome:bar(//t:n)", true, "//chrome/", "bar", "//t/", "n"},
    {"//chrome/", "//chrome:bar(//t)", true, "//chrome/", "bar", "//t/", "t"},
    {"//chrome/", "//chrome:bar(//t:)", true, "//chrome/", "bar", "//t/", "t"},
    {"//chrome/", "//chrome:bar()", true, "//chrome/", "bar", "//t/", "d"},
    {"//chrome/", "//chrome:bar(foo)", true, "//chrome/", "bar",
     "//chrome/foo/", "foo"},
    {"//chrome/", "//chrome:bar(:foo)", true, "//chrome/", "bar", "//chrome/",
     "foo"},
    // TODO(brettw) it might be nice to make this an error:
    //{"//chrome/", "//chrome:bar())", false, "", "", "", "" },
    {"//chrome/", "//chrome:bar(//t:bar(tc))", false, "", "", "", ""},
    {"//chrome/", "//chrome:bar(()", false, "", "", "", ""},
    {"//chrome/", "(t:b)", false, "", "", "", ""},
    {"//chrome/", ":bar(//t/b)", true, "//chrome/", "bar", "//t/b/", "b"},
    {"//chrome/", ":bar(/t/b)", true, "//chrome/", "bar", "/t/b/", "b"},
    {"//chrome/", ":bar(t/b)", true, "//chrome/", "bar", "//chrome/t/b/", "b"},
  };

  Label default_toolchain(SourceDir("//t/"), "d");

  for (size_t i = 0; i < std::size(cases); i++) {
    const ParseDepStringCase& cur = cases[i];

    std::string location, name;
    Err err;
    Value v(nullptr, Value::STRING);
    v.string_value() = cur.str;
    Label result = Label::Resolve(SourceDir(cur.cur_dir), std::string_view(),
                                  default_toolchain, v, &err);
    EXPECT_EQ(cur.success, !err.has_error()) << i << " " << cur.str;
    if (!err.has_error() && cur.success) {
      EXPECT_EQ(cur.expected_dir, result.dir().value()) << i << " " << cur.str;
      EXPECT_EQ(cur.expected_name, result.name()) << i << " " << cur.str;
      EXPECT_EQ(cur.expected_toolchain_dir, result.toolchain_dir().value())
          << i << " " << cur.str;
      EXPECT_EQ(cur.expected_toolchain_name, result.toolchain_name())
          << i << " " << cur.str;
    }
  }
}

// Tests the case where the path resolves to something above "//". It should get
// converted to an absolute path "/foo/bar".
TEST(Label, ResolveAboveRootBuildDir) {
  Label default_toolchain(SourceDir("//t/"), "d");

  std::string location, name;
  Err err;

  SourceDir cur_dir("//cur/");
  std::string source_root("/foo/bar/baz");

  // No source root given, should not go above the root build dir.
  Label result = Label::Resolve(cur_dir, std::string_view(), default_toolchain,
                                Value(nullptr, "../../..:target"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ("//", result.dir().value()) << result.dir().value();
  EXPECT_EQ("target", result.name());

  // Source root provided, it should go into that.
  result = Label::Resolve(cur_dir, source_root, default_toolchain,
                          Value(nullptr, "../../..:target"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ("/foo/", result.dir().value()) << result.dir().value();
  EXPECT_EQ("target", result.name());

  // It shouldn't go up higher than the system root.
  result = Label::Resolve(cur_dir, source_root, default_toolchain,
                          Value(nullptr, "../../../../..:target"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ("/", result.dir().value()) << result.dir().value();
  EXPECT_EQ("target", result.name());

  // Test an absolute label that goes above the source root. This currently
  // stops at the source root. It should arguably keep going and produce "/foo/"
  // but this test just makes sure the current behavior isn't regressed by
  // accident.
  result = Label::Resolve(cur_dir, source_root, default_toolchain,
                          Value(nullptr, "//../.."), &err);
  EXPECT_FALSE(err.has_error()) << err.message();
  EXPECT_EQ("/foo/", result.dir().value()) << result.dir().value();
  EXPECT_EQ("foo", result.name());
}
