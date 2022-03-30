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

  CrateList crates;
  Crate dep =
      Crate(SourceFile("/root/tortoise/lib.rs"), 0, "//tortoise:bar", "2015");
  Crate target =
      Crate(SourceFile("/root/hare/lib.rs"), 1, "//hare:bar", "2015");
  target.AddDependency(0, "tortoise");
  target.AddConfigItem("unix");
  target.AddConfigItem("feature=\"test\"");

  crates.push_back(dep);
  crates.push_back(target);

  std::ostringstream stream;
  WriteCrates(setup.build_settings(), crates, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"roots\": [\n"
      "    \"/root/hare/\",\n"
      "    \"/root/tortoise/\"\n"
      "  ],\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"/root/tortoise/lib.rs\",\n"
      "      \"label\": \"//tortoise:bar\",\n"
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

  SysrootIndexMap sysroot_lookup;
  CrateList crates;

  AddSysroot(setup.build_settings(), "sysroot", sysroot_lookup, crates);

  std::ostringstream stream;
  WriteCrates(setup.build_settings(), crates, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif

  const char expected_json[] =
      "{\n"
      "  \"roots\": [\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/alloc/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/core/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/panic_abort/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/panic_unwind/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/proc_macro/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/std/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/test/src/\",\n"
      "    \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/unwind/src/\"\n"
      "  ],\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/core/src/lib.rs\",\n"
      "      \"label\": \"core\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 1,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/alloc/src/lib.rs\",\n"
      "      \"label\": \"alloc\",\n"
      "      \"deps\": [\n"
      "        {\n"
      "          \"crate\": 0,\n"
      "          \"name\": \"core\"\n"
      "        }\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 2,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/panic_abort/src/lib.rs\",\n"
      "      \"label\": \"panic_abort\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 3,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/unwind/src/lib.rs\",\n"
      "      \"label\": \"unwind\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 4,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/std/src/lib.rs\",\n"
      "      \"label\": \"std\",\n"
      "      \"deps\": [\n"
      "        {\n"
      "          \"crate\": 1,\n"
      "          \"name\": \"alloc\"\n"
      "        },\n"
      "        {\n"
      "          \"crate\": 0,\n"
      "          \"name\": \"core\"\n"
      "        },\n"
      "        {\n"
      "          \"crate\": 2,\n"
      "          \"name\": \"panic_abort\"\n"
      "        },\n"
      "        {\n"
      "          \"crate\": 3,\n"
      "          \"name\": \"unwind\"\n"
      "        }\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 5,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/panic_unwind/src/lib.rs\",\n"
      "      \"label\": \"panic_unwind\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 6,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/proc_macro/src/lib.rs\",\n"
      "      \"label\": \"proc_macro\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 7,\n"
      "      \"root_module\": \"/root/out/Debug/sysroot/lib/rustlib/src/rust/library/test/src/lib.rs\",\n"
      "      \"label\": \"test\",\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
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