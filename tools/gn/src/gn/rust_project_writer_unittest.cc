// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/rust_project_writer.h"
#include "base/strings/string_util.h"
#include "base/files/file_path.h"
#include "gn/filesystem_utils.h"
#include "gn/substitution_list.h"
#include "gn/target.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"


static void ExpectEqOrShowDiff(const char* expected, const std::string& actual) {
  if(expected != actual) {
    printf("\nExpected: >>>\n%s<<<\n", expected);
    printf("  Actual: >>>\n%s<<<\n", actual.c_str());
  }
  EXPECT_EQ(expected, actual);
}

using RustProjectJSONWriter = TestWithScheduler;

TEST_F(RustProjectJSONWriter, OneRustTarget) {
  Err err;
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("path"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"path/foo/lib.rs\",\n"
      "      \"label\": \"//foo:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"path/foo/\",\n"
      "               \"path/out/Debug/gen/foo/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"compiler_args\": [\"--cfg=feature=\\\"foo_enabled\\\"\"],\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\",\n"
      "        \"feature=\\\"foo_enabled\\\"\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, RustTargetDep) {
  Err err;
  TestWithScope setup;

  Target dep(setup.settings(), Label(SourceDir("//tortoise/"), "bar"));
  dep.set_output_type(Target::RUST_LIBRARY);
  dep.visibility().SetPublic();
  SourceFile tlib("//tortoise/lib.rs");
  dep.sources().push_back(tlib);
  dep.source_types_used().Set(SourceFile::SOURCE_RS);
  dep.rust_values().set_crate_root(tlib);
  dep.rust_values().crate_name() = "tortoise";
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//hare/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile harelib("//hare/lib.rs");
  target.sources().push_back(harelib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(harelib);
  target.rust_values().crate_name() = "hare";
  target.public_deps().push_back(LabelTargetPair(&dep));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"tortoise/lib.rs\",\n"
      "      \"label\": \"//tortoise:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"tortoise/\",\n"
      "               \"out/Debug/gen/tortoise/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 1,\n"
      "      \"root_module\": \"hare/lib.rs\",\n"
      "      \"label\": \"//hare:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"hare/\",\n"
      "               \"out/Debug/gen/hare/\"\n"
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
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, RustTargetDepTwo) {
  Err err;
  TestWithScope setup;

  Target dep(setup.settings(), Label(SourceDir("//tortoise/"), "bar"));
  dep.set_output_type(Target::RUST_LIBRARY);
  dep.visibility().SetPublic();
  SourceFile tlib("//tortoise/lib.rs");
  dep.sources().push_back(tlib);
  dep.source_types_used().Set(SourceFile::SOURCE_RS);
  dep.rust_values().set_crate_root(tlib);
  dep.rust_values().crate_name() = "tortoise";
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target dep2(setup.settings(), Label(SourceDir("//achilles/"), "bar"));
  dep2.set_output_type(Target::RUST_LIBRARY);
  dep2.visibility().SetPublic();
  SourceFile alib("//achilles/lib.rs");
  dep2.sources().push_back(alib);
  dep2.source_types_used().Set(SourceFile::SOURCE_RS);
  dep2.rust_values().set_crate_root(alib);
  dep2.rust_values().crate_name() = "achilles";
  dep2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep2.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//hare/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile harelib("//hare/lib.rs");
  target.sources().push_back(harelib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(harelib);
  target.rust_values().crate_name() = "hare";
  target.public_deps().push_back(LabelTargetPair(&dep));
  target.public_deps().push_back(LabelTargetPair(&dep2));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"tortoise/lib.rs\",\n"
      "      \"label\": \"//tortoise:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"tortoise/\",\n"
      "               \"out/Debug/gen/tortoise/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 1,\n"
      "      \"root_module\": \"achilles/lib.rs\",\n"
      "      \"label\": \"//achilles:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"achilles/\",\n"
      "               \"out/Debug/gen/achilles/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 2,\n"
      "      \"root_module\": \"hare/lib.rs\",\n"
      "      \"label\": \"//hare:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"hare/\",\n"
      "               \"out/Debug/gen/hare/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "        {\n"
      "          \"crate\": 0,\n"
      "          \"name\": \"tortoise\"\n"
      "        },\n"
      "        {\n"
      "          \"crate\": 1,\n"
      "          \"name\": \"achilles\"\n"
      "        }\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";
  ExpectEqOrShowDiff(expected_json, out);
}

// Test that when outputting dependencies, only Rust deps are returned,
// and that any groups are inspected to see if they include Rust deps.
TEST_F(RustProjectJSONWriter, RustTargetGetDepRustOnly) {
  Err err;
  TestWithScope setup;

  Target dep(setup.settings(), Label(SourceDir("//tortoise/"), "bar"));
  dep.set_output_type(Target::RUST_LIBRARY);
  dep.visibility().SetPublic();
  SourceFile tlib("//tortoise/lib.rs");
  dep.sources().push_back(tlib);
  dep.source_types_used().Set(SourceFile::SOURCE_RS);
  dep.rust_values().set_crate_root(tlib);
  dep.rust_values().crate_name() = "tortoise";
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target dep2(setup.settings(), Label(SourceDir("//achilles/"), "bar"));
  dep2.set_output_type(Target::STATIC_LIBRARY);
  dep2.visibility().SetPublic();
  SourceFile alib("//achilles/lib.o");
  dep2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep2.OnResolved(&err));

  Target dep3(setup.settings(), Label(SourceDir("//achilles/"), "group"));
  dep3.set_output_type(Target::GROUP);
  dep3.visibility().SetPublic();
  dep3.public_deps().push_back(LabelTargetPair(&dep));
  dep3.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep3.OnResolved(&err));

  Target dep4(setup.settings(), Label(SourceDir("//tortoise/"), "macro"));
  dep4.set_output_type(Target::RUST_PROC_MACRO);
  dep4.visibility().SetPublic();
  SourceFile tmlib("//tortoise/macro/lib.rs");
  dep4.sources().push_back(tmlib);
  dep4.source_types_used().Set(SourceFile::SOURCE_RS);
  dep4.rust_values().set_crate_root(tmlib);
  dep4.rust_values().crate_name() = "tortoise_macro";
  dep4.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep4.OnResolved(&err));

  Target target(setup.settings(), Label(SourceDir("//hare/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile harelib("//hare/lib.rs");
  target.sources().push_back(harelib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(harelib);
  target.rust_values().crate_name() = "hare";
  target.public_deps().push_back(LabelTargetPair(&dep));
  target.public_deps().push_back(LabelTargetPair(&dep3));
  target.public_deps().push_back(LabelTargetPair(&dep4));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"tortoise/lib.rs\",\n"
      "      \"label\": \"//tortoise:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"tortoise/\",\n"
      "               \"out/Debug/gen/tortoise/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 1,\n"
      "      \"root_module\": \"tortoise/macro/lib.rs\",\n"
      "      \"label\": \"//tortoise:macro\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"tortoise/macro/\",\n"
      "               \"out/Debug/gen/tortoise/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"is_proc_macro\": true,\n"
      "      \"proc_macro_dylib_path\": "
      "\"out/Debug/obj/tortoise/libmacro.so\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    },\n"
      "    {\n"
      "      \"crate_id\": 2,\n"
      "      \"root_module\": \"hare/lib.rs\",\n"
      "      \"label\": \"//hare:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"hare/\",\n"
      "               \"out/Debug/gen/hare/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"deps\": [\n"
      "        {\n"
      "          \"crate\": 0,\n"
      "          \"name\": \"tortoise\"\n"
      "        },\n"
      "        {\n"
      "          \"crate\": 1,\n"
      "          \"name\": \"tortoise_macro\"\n"
      "        }\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, OneRustTargetWithRustcTargetSet) {
  Err err;
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("path"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--target");
  target.config_values().rustflags().push_back("x86-64_unknown");
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"path/foo/lib.rs\",\n"
      "      \"label\": \"//foo:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"path/foo/\",\n"
      "               \"path/out/Debug/gen/foo/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"target\": \"x86-64_unknown\",\n"
      "      \"compiler_args\": [\"--target\", \"x86-64_unknown\"],\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, OneRustTargetWithEditionSet) {
  Err err;
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("path"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--edition=2018");
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"path/foo/lib.rs\",\n"
      "      \"label\": \"//foo:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"path/foo/\",\n"
      "               \"path/out/Debug/gen/foo/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"compiler_args\": [\"--edition=2018\"],\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, OneRustTargetWithEditionSetAlternate) {
  Err err;
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("path"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_LIBRARY);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--edition");
  target.config_values().rustflags().push_back("2018");
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"path/foo/lib.rs\",\n"
      "      \"label\": \"//foo:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"path/foo/\",\n"
      "               \"path/out/Debug/gen/foo/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"compiler_args\": [\"--edition\", \"2018\"],\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2018\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\"\n"
      "      ]\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}

TEST_F(RustProjectJSONWriter, OneRustProcMacroTarget) {
  Err err;
  TestWithScope setup;
  setup.build_settings()->SetRootPath(UTF8ToFilePath("path"));

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::RUST_PROC_MACRO);
  target.visibility().SetPublic();
  SourceFile lib("//foo/lib.rs");
  target.sources().push_back(lib);
  target.source_types_used().Set(SourceFile::SOURCE_RS);
  target.rust_values().set_crate_root(lib);
  target.config_values().rustenv().push_back("TEST_ENV_VAR=baz");
  target.config_values().rustenv().push_back("TEST_ENV_VAR2=baz2");
  target.rust_values().crate_name() = "foo";
  target.config_values().rustflags().push_back("--cfg=feature=\"foo_enabled\"");
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  std::vector<const Target*> targets;
  targets.push_back(&target);
  RustProjectWriter::RenderJSON(setup.build_settings(), targets, stream);
  std::string out = stream.str();
#if defined(OS_WIN)
  base::ReplaceSubstringsAfterOffset(&out, 0, "\r\n", "\n");
#endif
  const char expected_json[] =
      "{\n"
      "  \"crates\": [\n"
      "    {\n"
      "      \"crate_id\": 0,\n"
      "      \"root_module\": \"path/foo/lib.rs\",\n"
      "      \"label\": \"//foo:bar\",\n"
      "      \"source\": {\n"
      "          \"include_dirs\": [\n"
      "               \"path/foo/\",\n"
      "               \"path/out/Debug/gen/foo/\"\n"
      "          ],\n"
      "          \"exclude_dirs\": []\n"
      "      },\n"
      "      \"compiler_args\": [\"--cfg=feature=\\\"foo_enabled\\\"\"],\n"
      "      \"deps\": [\n"
      "      ],\n"
      "      \"edition\": \"2015\",\n"
      "      \"is_proc_macro\": true,\n"
      "      \"proc_macro_dylib_path\": \"path/out/Debug/obj/foo/libbar.so\",\n"
      "      \"cfg\": [\n"
      "        \"test\",\n"
      "        \"debug_assertions\",\n"
      "        \"feature=\\\"foo_enabled\\\"\"\n"
      "      ],\n"
      "      \"env\": {\n"
      "        \"TEST_ENV_VAR\": \"baz\",\n"
      "        \"TEST_ENV_VAR2\": \"baz2\"\n"
      "      }\n"
      "    }\n"
      "  ]\n"
      "}\n";

  ExpectEqOrShowDiff(expected_json, out);
}
