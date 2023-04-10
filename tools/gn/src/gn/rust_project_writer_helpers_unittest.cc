// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_project_writer_helpers.h"

#include "base/strings/string_util.h"
#include "gn/filesystem_utils.h"
#include "gn/string_output_buffer.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

static void ExpectEqOrShowDiff(const char* expected,
                               const std::string& actual) {
  if (expected != actual) {
    printf("\nExpected: >>>\n%s<<<\n", expected);
    printf("  Actual: >>>\n%s<<<\n", actual.c_str());
  }
  EXPECT_EQ(expected, actual);
}

using RustProjectWriterHelper = TestWithScheduler;

TEST_F(RustProjectWriterHelper, WriteCrates) {
  TestWithScope setup;

  std::optional<std::string> sysroot;

  CrateList crates;
  Crate dep = Crate(SourceFile("/root/tortoise/lib.rs"), std::nullopt, 0,
                    "//tortoise:bar", "2015");
  Crate target = Crate(SourceFile("/root/hare/lib.rs"),
                       OutputFile("gendir/hare/"), 1, "//hare:bar", "2015");
  target.AddDependency(0, "tortoise");
  target.AddConfigItem("unix");
  target.AddConfigItem("feature=\"test\"");

  crates.push_back(dep);
  crates.push_back(target);

  std::ostringstream stream;
  WriteCrates(setup.build_settings(), crates, sysroot, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"/root/tortoise/lib.rs\",\n"
      "      \"label\": \"//tortoise:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"/root/tortoise/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 1,\n"
      "      \"root_module\": \"/root/hare/lib.rs\",\n"
      "      \"label\": \"//hare:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"/root/hare/\",\n"
      "               \"out/Debug/gendir/hare/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "        {\n"
      "          \"crate\": 0,\n"
      "          \"name\": \"tortoise\"\n"
      "        }\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"unix\",\n"
      "        \"feature=\\\"test\\\"\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectWriterHelper, SysrootDepsAreCorrect) {
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("/root"));

  std::optional<std::string> sysroot = "sysroot";
  CrateList crates;

  std::ostringstream stream;
  WriteCrates(setup.build_settings(), crates, sysroot, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif

  const char expected_json[] =
      "{\n"
      "  \"sysroot\": \"/root/out/Debug/sysroot\",\n"
      "  \"crates\": [\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectWriterHelper, ExtractCompilerTargetTupleSimple) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.config_values().rustflags().push_back("--target");
  target.config_values().rustflags().push_back("x86-someos");
  target.config_values().rustflags().push_back("--edition=2018");

  auto args = ExtractCompilerArgs(&target);
  std::optional<std::string> result = FindArgValue("--target", args);
  auto expected = std::optional<std::string>{"x86-someos"};
  EXPECT_EQ(expected, result);
}

TEST_F(RustProjectWriterHelper, ExtractCompilerTargetTupleMissing) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back(
      "--cfg=featur4e=\"foo_enabled\"");
  target.config_values().rustflags().push_back("x86-someos");
  target.config_values().rustflags().push_back("--edition=2018");

  auto args = ExtractCompilerArgs(&target);
  std::optional<std::string> result = FindArgValue("--target", args);
  auto expected = std::nullopt;
  EXPECT_EQ(expected, result);
}

TEST_F(RustProjectWriterHelper, ExtractCompilerTargetTupleDontFallOffEnd) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.config_values().rustflags().push_back("--edition=2018");
  target.config_values().rustflags().push_back("--target");

  auto args = ExtractCompilerArgs(&target);
  std::optional<std::string> result = FindArgValue("--target", args);
  auto expected = std::nullopt;
  EXPECT_EQ(expected, result);
}

TEST_F(RustProjectWriterHelper, ExtractFirstArgValueWithPrefixMissing) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.config_values().rustflags().push_back("--edition=2018");
  target.config_values().rustflags().push_back("--target");

  auto args = ExtractCompilerArgs(&target);
  std::optional<std::string> result =
      FindArgValueAfterPrefix("--missing", args);
  auto expected = std::nullopt;
  EXPECT_EQ(expected, result);
}

TEST_F(RustProjectWriterHelper, ExtractFirstArgValueWithPrefix) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.config_values().rustflags().push_back("--edition=2018");
  target.config_values().rustflags().push_back("--target");

  auto args = ExtractCompilerArgs(&target);
  std::optional<std::string> result =
      FindArgValueAfterPrefix("--edition=", args);
  auto expected = std::optional<std::string>{"2018"};
  EXPECT_EQ(expected, result);
}

TEST_F(RustProjectWriterHelper, ExtractAllArgValueWithPrefix) {
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.config_values().rustflags().push_back("--edition=2018");
  target.config_values().rustflags().push_back("--cfg=feature=\"bar_enabled\"");
  target.config_values().rustflags().push_back("--target");

  auto args = ExtractCompilerArgs(&target);
  std::vector<std::string> result = FindAllArgValuesAfterPrefix("--cfg=", args);
  std::vector<std::string> expected = {"feature=\"foo_enabled\"",
                                       "feature=\"bar_enabled\""};
  EXPECT_EQ(expected, result);
}