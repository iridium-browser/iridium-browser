// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/config.h"
#include "gn/scheduler.h"
#include "gn/scope.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

using RustFunctionsTarget = TestWithScheduler;

// Checks that the appropriate crate type is used.
TEST_F(RustFunctionsTarget, CrateName) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput exe_input(
      "executable(\"foo\") {\n"
      "  crate_name = \"foo_crate\"\n"
      "  sources = [ \"foo.rs\", \"lib.rs\", \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  Err err;
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(item_collector.back()->AsTarget()->rust_values().crate_name(),
            "foo_crate");

  TestParseInput lib_input(
      "executable(\"foo\") {\n"
      "  sources = [ \"lib.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(lib_input.has_error());
  err = Err();
  lib_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(item_collector.back()->AsTarget()->rust_values().crate_name(),
            "foo")
      << item_collector.back()->AsTarget()->rust_values().crate_name();
}

// Checks that the appropriate crate root is found.
TEST_F(RustFunctionsTarget, CrateRootFind) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput normal_input(
      "executable(\"foo\") {\n"
      "  crate_root = \"foo.rs\""
      "  sources = [ \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(normal_input.has_error());
  Err err;
  normal_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/foo.rs");

  TestParseInput normal_shlib_input(
      "shared_library(\"foo\") {\n"
      "  crate_root = \"foo.rs\""
      "  crate_type = \"dylib\"\n"
      "  sources = [ \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(normal_shlib_input.has_error());
  err = Err();
  normal_shlib_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/foo.rs");

  TestParseInput exe_input(
      "executable(\"foo\") {\n"
      "  sources = [ \"foo.rs\", \"lib.rs\", \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  err = Err();
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/main.rs");

  TestParseInput lib_input(
      "rust_library(\"libfoo\") {\n"
      "  sources = [ \"foo.rs\", \"lib.rs\", \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(lib_input.has_error());
  err = Err();
  lib_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/lib.rs");

  TestParseInput singlesource_input(
      "executable(\"bar\") {\n"
      "  sources = [ \"bar.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(singlesource_input.has_error());
  err = Err();
  singlesource_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/bar.rs");

  TestParseInput error_input(
      "rust_library(\"foo\") {\n"
      "  sources = [ \"foo.rs\", \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(error_input.has_error());
  err = Err();
  error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Missing \"crate_root\" and missing \"lib.rs\" in sources.",
            err.message());

  TestParseInput nosources_input(
      "executable(\"bar\") {\n"
      "  crate_root = \"bar.rs\"\n"
      "}\n");
  ASSERT_FALSE(nosources_input.has_error());
  err = Err();
  nosources_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(
      item_collector.back()->AsTarget()->rust_values().crate_root().value(),
      "/bar.rs");
}

// Checks that the appropriate crate type is used.
TEST_F(RustFunctionsTarget, CrateTypeSelection) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput lib_input(
      "shared_library(\"libfoo\") {\n"
      "  crate_type = \"dylib\"\n"
      "  sources = [ \"lib.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(lib_input.has_error());
  Err err;
  lib_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(item_collector.back()->AsTarget()->rust_values().crate_type(),
            RustValues::CRATE_DYLIB);

  TestParseInput exe_non_default_input(
      "executable(\"foo\") {\n"
      "  crate_type = \"rlib\"\n"
      "  sources = [ \"main.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(exe_non_default_input.has_error());
  err = Err();
  exe_non_default_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
  ASSERT_EQ(item_collector.back()->AsTarget()->rust_values().crate_type(),
            RustValues::CRATE_RLIB);

  TestParseInput lib_error_input(
      "shared_library(\"foo\") {\n"
      "  crate_type = \"bad\"\n"
      "  sources = [ \"lib.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(lib_error_input.has_error());
  err = Err();
  lib_error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Inadmissible crate type \"bad\".", err.message()) << err.message();

  TestParseInput lib_missing_error_input(
      "shared_library(\"foo\") {\n"
      "  sources = [ \"lib.rs\" ]\n"
      "}\n");
  ASSERT_FALSE(lib_missing_error_input.has_error());
  err = Err();
  lib_missing_error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Must set \"crate_type\" on a Rust \"shared_library\".",
            err.message());
}

// Checks that the appropriate config values are propagated.
TEST_F(RustFunctionsTarget, ConfigValues) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput exe_input(
      "config(\"foo\") {\n"
      "  rustflags = [ \"-Cdebuginfo=2\" ]\n"
      "  rustenv = [ \"RUST_BACKTRACE=1\" ]"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  Err err;
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ(item_collector.back()->AsConfig()->own_values().rustflags().size(),
            1U);
  EXPECT_EQ(item_collector.back()->AsConfig()->own_values().rustflags()[0],
            "-Cdebuginfo=2");
  EXPECT_EQ(item_collector.back()->AsConfig()->own_values().rustenv().size(),
            1U);
  EXPECT_EQ(item_collector.back()->AsConfig()->own_values().rustenv()[0],
            "RUST_BACKTRACE=1");
}

// Checks that set_defaults works properly.
TEST_F(RustFunctionsTarget, SetDefaults) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput exe_input(
      "config(\"foo\") {\n"
      "  rustflags = [ \"-Cdebuginfo=2\" ]\n"
      "  rustenv = [ \"RUST_BACKTRACE=1\" ]"
      "}\n"
      "set_defaults(\"rust_library\") {\n"
      "  configs = [ \":foo\" ]\n"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  Err err;
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << err.message();

  EXPECT_EQ(setup.scope()
                ->GetTargetDefaults("rust_library")
                ->GetValue("configs")
                ->type(),
            Value::LIST);
  EXPECT_EQ(setup.scope()
                ->GetTargetDefaults("rust_library")
                ->GetValue("configs")
                ->list_value()
                .size(),
            1U);
  EXPECT_EQ(setup.scope()
                ->GetTargetDefaults("rust_library")
                ->GetValue("configs")
                ->list_value()[0]
                .type(),
            Value::STRING);
  EXPECT_EQ(setup.scope()
                ->GetTargetDefaults("rust_library")
                ->GetValue("configs")
                ->list_value()[0]
                .string_value(),
            ":foo");
}

// Checks aliased_deps parsing.
TEST_F(RustFunctionsTarget, AliasedDeps) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput exe_input(
      "executable(\"foo\") {\n"
      "  sources = [ \"main.rs\" ]\n"
      "  deps = [ \"//bar\", \"//baz\" ]\n"
      "  aliased_deps = {\n"
      "    bar_renamed = \"//bar\"\n"
      "    baz_renamed = \"//baz:baz\"\n"
      "  }\n"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  Err err;
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ(
      item_collector.back()->AsTarget()->rust_values().aliased_deps().size(),
      2U);
}

TEST_F(RustFunctionsTarget, PublicConfigs) {
  TestWithScope setup;

  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);
  setup.scope()->set_source_dir(SourceDir("/"));

  TestParseInput exe_input(
      "config(\"bar\") {\n"
      "  defines = [ \"DOOM_MELON\" ]"
      "}\n"
      "executable(\"foo\") {\n"
      "  crate_name = \"foo_crate\"\n"
      "  sources = [ \"foo.rs\", \"lib.rs\", \"main.rs\" ]\n"
      "  public_configs = [ \":bar\" ]"
      "}\n");
  ASSERT_FALSE(exe_input.has_error());
  Err err;
  exe_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
}
