// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/scheduler.h"
#include "gn/scope.h"
#include "gn/test_with_scheduler.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

using FunctionsTarget = TestWithScheduler;

// Checks that we find unused identifiers in targets.
TEST_F(FunctionsTarget, CheckUnused) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);

  // Test a good one first.
  TestParseInput good_input(
      "source_set(\"foo\") {\n"
      "}\n");
  ASSERT_FALSE(good_input.has_error());
  Err err;
  good_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  // Test a source set with an unused variable.
  TestParseInput source_set_input(
      "source_set(\"foo\") {\n"
      "  unused = 5\n"
      "}\n");
  ASSERT_FALSE(source_set_input.has_error());
  err = Err();
  source_set_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
}

// Checks that we find uses of identifiers marked as not needed.
TEST_F(FunctionsTarget, CheckNotNeeded) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);

  TestParseInput nonscoped_input(
      "source_set(\"foo\") {\n"
      "  a = 1\n"
      "  not_needed([ \"a\" ])\n"
      "}\n");
  ASSERT_FALSE(nonscoped_input.has_error());
  Err err;
  nonscoped_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  TestParseInput scoped_input(
      "source_set(\"foo\") {\n"
      "  a = {x = 1 y = 2}\n"
      "  not_needed(a, \"*\")\n"
      "}\n");
  ASSERT_FALSE(scoped_input.has_error());
  err = Err();
  scoped_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  TestParseInput nonexistent_arg_input(
      "source_set(\"foo\") {\n"
      "  a = {x = 1}\n"
      "  not_needed(a, [ \"x\", \"y\" ])\n"
      "}\n");
  ASSERT_FALSE(nonexistent_arg_input.has_error());
  err = Err();
  nonexistent_arg_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  TestParseInput exclusion_input(
      "source_set(\"foo\") {\n"
      "  x = 1\n"
      "  y = 2\n"
      "  not_needed(\"*\", [ \"y\" ])\n"
      "}\n");
  ASSERT_FALSE(exclusion_input.has_error());
  err = Err();
  exclusion_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error()) << err.message();
  EXPECT_EQ("Assignment had no effect.", err.message());

  TestParseInput error_input(
      "source_set(\"foo\") {\n"
      "  a = {x = 1 y = 2}\n"
      "  not_needed(a, [ \"x \"], [ \"y\" ])\n"
      "}\n");
  ASSERT_FALSE(error_input.has_error());
  err = Err();
  error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Not supported with a variable list.", err.message());

  TestParseInput argcount_error_input(
      "source_set(\"foo\") {\n"
      "  not_needed()\n"
      "}\n");
  ASSERT_FALSE(argcount_error_input.has_error());
  err = Err();
  argcount_error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Wrong number of arguments.", err.message());

  TestParseInput scope_error_input(
      "source_set(\"foo\") {\n"
      "  a = {x = 1 y = 2}\n"
      "  not_needed(a)\n"
      "}\n");
  ASSERT_FALSE(scope_error_input.has_error());
  err = Err();
  scope_error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Wrong number of arguments.", err.message());

  TestParseInput string_error_input(
      "source_set(\"foo\") {\n"
      "  not_needed(\"*\", {}, \"*\")\n"
      "}\n");
  ASSERT_FALSE(string_error_input.has_error());
  err = Err();
  string_error_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Wrong number of arguments.", err.message());

  TestParseInput template_input(
      R"(# Test that not_needed() propagates through templates correctly;
      # no error should arise from not using "a".
      template("inner_templ") {
        source_set(target_name) {
          not_needed(invoker, [ "a" ])
        }
      }
      template("outer_templ") {
        inner_templ(target_name) {
          forward_variables_from(invoker, "*")
        }
      }
      outer_templ("foo") {
        a = 1
      })");
  ASSERT_FALSE(template_input.has_error());
  err = Err();
  template_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
}

// Checks that the defaults applied to a template invoked by target() use
// the name of the template, rather than the string "target" (which is the
// name of the actual function being called).
TEST_F(FunctionsTarget, TemplateDefaults) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);

  // Test a good one first.
  TestParseInput good_input(
      R"(# Make a template with defaults set.
      template("my_templ") {
        source_set(target_name) {
          forward_variables_from(invoker, "*")
        }
      }
      set_defaults("my_templ") {
        default_value = 1
      }

      # Invoke the template with target(). This will fail to execute if the
      # defaults were not set properly, because "default_value" won't exist.
      target("my_templ", "foo") {
        print(default_value)
      })");
  ASSERT_FALSE(good_input.has_error());
  Err err;
  good_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();
}

// Checks that we find unused identifiers in targets.
TEST_F(FunctionsTarget, MixedSourceError) {
  TestWithScope setup;

  // The target generator needs a place to put the targets or it will fail.
  Scope::ItemVector item_collector;
  setup.scope()->set_item_collector(&item_collector);

  // Test a good one first.
  TestParseInput good_input(
      "source_set(\"foo\") {\n"
      "  sources = [ \"cpp.cc\", \"rust.rs\" ]"
      "}\n");
  ASSERT_FALSE(good_input.has_error());
  Err err;
  good_input.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  ASSERT_EQ(err.message(), "More than one language used in target sources.");
}
