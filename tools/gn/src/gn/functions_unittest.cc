// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/functions.h"

#include <memory>
#include <utility>

#include "gn/parse_tree.h"
#include "gn/test_with_scope.h"
#include "gn/value.h"
#include "util/test/test.h"

TEST(Functions, Assert) {
  TestWithScope setup;

  // Verify cases where the assertion passes.
  std::vector<std::string> assert_pass_examples = {
    R"gn(assert(true))gn",
    R"gn(assert(true, "This message is ignored for passed assertions."))gn",
  };
  for (const auto& assert_pass_example : assert_pass_examples) {
    TestParseInput input(assert_pass_example);
    ASSERT_FALSE(input.has_error());
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_FALSE(err.has_error()) << assert_pass_example;
  }

  // Verify case where the assertion fails, with no message.
  {
    TestParseInput input("assert(false)");
    ASSERT_FALSE(input.has_error());
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error());
    ASSERT_EQ(err.message(), "Assertion failed.");
  }

  // Verify case where the assertion fails, with a message.
  {
    TestParseInput input("assert(false, \"What failed\")");
    ASSERT_FALSE(input.has_error());
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error());
    ASSERT_EQ(err.message(), "Assertion failed.");
    ASSERT_EQ(err.help_text(), "What failed");
  }

  // Verify usage errors are detected.
  std::vector<std::string> bad_usage_examples = {
    // Number of arguments.
    R"gn(assert())gn",
    R"gn(assert(1, 2, 3))gn",

    // Argument types.
    R"gn(assert(1))gn",
    R"gn(assert("oops"))gn",
    R"gn(assert(true, 1))gn",
    R"gn(assert(true, []))gn",
  };
  for (const auto& bad_usage_example : bad_usage_examples) {
    TestParseInput input(bad_usage_example);
    ASSERT_FALSE(input.has_error());
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error()) << bad_usage_example;
    // We are checking for usage errors, not assertion failures.
    ASSERT_NE(err.message(), "Assertion failed.") << bad_usage_example;
  }
}

TEST(Functions, Defined) {
  TestWithScope setup;

  FunctionCallNode function_call;
  Err err;

  // Test an undefined identifier.
  Token undefined_token(Location(), Token::IDENTIFIER, "undef");
  ListNode args_list_identifier_undefined;
  args_list_identifier_undefined.append_item(
      std::make_unique<IdentifierNode>(undefined_token));
  Value result = functions::RunDefined(setup.scope(), &function_call,
                                       &args_list_identifier_undefined, &err);
  ASSERT_EQ(Value::BOOLEAN, result.type());
  EXPECT_FALSE(result.boolean_value());

  // Define a value that's itself a scope value.
  const char kDef[] = "def";  // Defined variable name.
  setup.scope()->SetValue(
      kDef, Value(nullptr, std::make_unique<Scope>(setup.scope())), nullptr);

  // Test the defined identifier.
  Token defined_token(Location(), Token::IDENTIFIER, kDef);
  ListNode args_list_identifier_defined;
  args_list_identifier_defined.append_item(
      std::make_unique<IdentifierNode>(defined_token));
  result = functions::RunDefined(setup.scope(), &function_call,
                                 &args_list_identifier_defined, &err);
  ASSERT_EQ(Value::BOOLEAN, result.type());
  EXPECT_TRUE(result.boolean_value());

  // Should also work by passing an accessor node so you can do
  // "defined(def.foo)" to see if foo is defined on the def scope.
  std::unique_ptr<AccessorNode> undef_accessor =
      std::make_unique<AccessorNode>();
  undef_accessor->set_base(defined_token);
  undef_accessor->set_member(std::make_unique<IdentifierNode>(undefined_token));
  ListNode args_list_accessor_defined;
  args_list_accessor_defined.append_item(std::move(undef_accessor));
  result = functions::RunDefined(setup.scope(), &function_call,
                                 &args_list_accessor_defined, &err);
  ASSERT_EQ(Value::BOOLEAN, result.type());
  EXPECT_FALSE(result.boolean_value());
}

// Tests that an error is thrown when a {} is supplied to a function that
// doesn't take one.
TEST(Functions, FunctionsWithBlock) {
  TestWithScope setup;
  Err err;

  // No scope to print() is OK.
  TestParseInput print_no_scope("print(6)");
  EXPECT_FALSE(print_no_scope.has_error());
  Value result = print_no_scope.parsed()->Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // Passing a scope should pass parsing (it doesn't know about what kind of
  // function it is) and then throw an error during execution.
  TestParseInput print_with_scope("print(foo) {}");
  EXPECT_FALSE(print_with_scope.has_error());
  result = print_with_scope.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  err = Err();

  // defined() is a special function so test it separately.
  TestParseInput defined_no_scope("defined(foo)");
  EXPECT_FALSE(defined_no_scope.has_error());
  result = defined_no_scope.parsed()->Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // A block to defined should fail.
  TestParseInput defined_with_scope("defined(foo) {}");
  EXPECT_FALSE(defined_with_scope.has_error());
  result = defined_with_scope.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
}

TEST(Functions, SplitList) {
  TestWithScope setup;

  TestParseInput input(
      // Empty input with varying result items.
      "out1 = split_list([], 1)\n"
      "out2 = split_list([], 3)\n"
      "print(\"empty = $out1 $out2\")\n"

      // One item input.
      "out3 = split_list([1], 1)\n"
      "out4 = split_list([1], 2)\n"
      "print(\"one = $out3 $out4\")\n"

      // Multiple items.
      "out5 = split_list([1, 2, 3, 4, 5, 6, 7, 8, 9], 2)\n"
      "print(\"many = $out5\")\n"

      // Rounding.
      "out6 = split_list([1, 2, 3, 4, 5, 6], 4)\n"
      "print(\"rounding = $out6\")\n");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ(
      "empty = [[]] [[], [], []]\n"
      "one = [[1]] [[1], []]\n"
      "many = [[1, 2, 3, 4, 5], [6, 7, 8, 9]]\n"
      "rounding = [[1, 2], [3, 4], [5], [6]]\n",
      setup.print_output());
}

TEST(Functions, StringJoin) {
  TestWithScope setup;

  // Verify outputs when string_join() is called correctly.
  {
    TestParseInput input(R"gn(
        # No elements in the list and empty separator.
        print("<" + string_join("", []) + ">")

        # No elements in the list.
        print("<" + string_join(" ", []) + ">")

        # One element in the list.
        print(string_join("|", ["a"]))

        # Multiple elements in the list.
        print(string_join(" ", ["a", "b", "c"]))

        # Multi-character separator.
        print(string_join("-.", ["a", "b", "c"]))

        # Empty separator.
        print(string_join("", ["x", "y", "z"]))

        # Empty string list elements.
        print(string_join("x", ["", "", ""]))

        # Empty string list elements and separator
        print(string_join("", ["", "", ""]))
        )gn");
    ASSERT_FALSE(input.has_error());

    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_FALSE(err.has_error()) << err.message();

    EXPECT_EQ(
        "<>\n"
        "<>\n"
        "a\n"
        "a b c\n"
        "a-.b-.c\n"
        "xyz\n"
        "xx\n"
        "\n",
        setup.print_output()) << setup.print_output();
  }

  // Verify usage errors are detected.
  std::vector<std::string> bad_usage_examples = {
    // Number of arguments.
    R"gn(string_join())gn",
    R"gn(string_join(["oops"]))gn",
    R"gn(string_join("kk", [], "oops"))gn",

    // Argument types.
    R"gn(string_join(1, []))gn",
    R"gn(string_join("kk", "oops"))gn",
    R"gn(string_join(["oops"], []))gn",

    // Non-string elements in list of strings.
    R"gn(string_join("kk", [1]))gn",
    R"gn(string_join("kk", ["hello", 1]))gn",
    R"gn(string_join("kk", ["hello", []]))gn",
  };
  for (const auto& bad_usage_example : bad_usage_examples) {
    TestParseInput input(bad_usage_example);
    ASSERT_FALSE(input.has_error());

    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error()) << bad_usage_example;
  }
}

TEST(Functions, StringReplace) {
  TestWithScope setup;

  TestParseInput input(
      // Replace all occurrences of string.
      "out1 = string_replace(\"abbcc\", \"b\", \"d\")\n"
      "print(out1)\n"

      // Replace only the first occurrence.
      "out2 = string_replace(\"abbcc\", \"b\", \"d\", 1)\n"
      "print(out2)\n"

      // Duplicate string to be replaced.
      "out3 = string_replace(\"abbcc\", \"b\", \"bb\")\n"
      "print(out3)\n"

      // Handle overlapping occurrences.
      "out4 = string_replace(\"aaa\", \"aa\", \"b\")\n"
      "print(out4)\n");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ(
      "addcc\n"
      "adbcc\n"
      "abbbbcc\n"
      "ba\n",
      setup.print_output());
}

TEST(Functions, StringSplit) {
  TestWithScope setup;

  // Verify outputs when string_join() is called correctly.
  {
    TestParseInput input(R"gn(
        # Split on all whitespace: empty string.
        print(string_split(""))

        # Split on all whitespace: string is only whitespace
        print(string_split("      "))

        # Split on all whitespace: leading, trailing, runs; one element.
        print(string_split("hello"))
        print(string_split("  hello"))
        print(string_split("  hello   "))
        print(string_split("hello   "))

        # Split on all whitespace: leading, trailing, runs; multiple elements.
        print(string_split("a b"))          # Pre-stripped
        print(string_split("  a b"))        # Leading whitespace
        print(string_split("  a b  "))      # Leading & trailing whitespace
        print(string_split("a b  "))        # Trailing whitespace
        print(string_split("a  b  "))       # Whitespace run between words
        print(string_split(" a b cc ddd"))  # More & multi-character elements

        # Split on string.
        print(string_split("", "|"))           # Empty string
        print(string_split("|", "|"))          # Only a separator
        print(string_split("||", "|"))         # Only separators
        print(string_split("ab", "|"))         # String is missing separator
        print(string_split("a|b", "|"))        # Two elements
        print(string_split("|a|b", "|"))       # Leading separator
        print(string_split("a|b|", "|"))       # Trailing separator
        print(string_split("||x", "|"))        # Leading consecutive separators
        print(string_split("x||", "|"))        # Trailing consecutive separators
        print(string_split("a|bb|ccc", "|"))   # Multiple elements
        print(string_split(".x.x.x.", ".x."))  # Self-overlapping separators 1
        print(string_split("x.x.x.", ".x."))   # Self-overlapping separators 2
        )gn");
    ASSERT_FALSE(input.has_error());

    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_FALSE(err.has_error()) << err.message();

    EXPECT_EQ(
        // Split on all whitespace: empty string.
        "[]\n"

        // Split on all whitespace: string is only whitespace.
        "[]\n"

        // Split on all whitespace: leading, trailing, runs; one element.
        "[\"hello\"]\n"
        "[\"hello\"]\n"
        "[\"hello\"]\n"
        "[\"hello\"]\n"

        // Split on all whitespace: leading, trailing, runs; multiple elements.
        "[\"a\", \"b\"]\n"
        "[\"a\", \"b\"]\n"
        "[\"a\", \"b\"]\n"
        "[\"a\", \"b\"]\n"
        "[\"a\", \"b\"]\n"
        "[\"a\", \"b\", \"cc\", \"ddd\"]\n"

        // Split on string.
        "[\"\"]\n"                   // Empty string (like Python)
        "[\"\", \"\"]\n"             // Only a separator
        "[\"\", \"\", \"\"]\n"       // Only separators
        "[\"ab\"]\n"                 // String is missing separator
        "[\"a\", \"b\"]\n"           // Two elements
        "[\"\", \"a\", \"b\"]\n"     // Leading
        "[\"a\", \"b\", \"\"]\n"     // Trailing
        "[\"\", \"\", \"x\"]\n"      // Leading consecutive separators
        "[\"x\", \"\", \"\"]\n"      // Trailing consecutive separators
        "[\"a\", \"bb\", \"ccc\"]\n" // Multiple elements
        "[\"\", \"x\", \"\"]\n"      // Self-overlapping separators 1
        "[\"x\", \"x.\"]\n"          // Self-overlapping separators 2
        ,
        setup.print_output()) << setup.print_output();
  }

  // Verify usage errors are detected.
  std::vector<std::string> bad_usage_examples = {
    // Number of arguments.
    R"gn(string_split())gn",
    R"gn(string_split("a", "b", "c"))gn",

    // Argument types.
    R"gn(string_split(1))gn",
    R"gn(string_split(["oops"]))gn",
    R"gn(string_split("kk", 1))gn",
    R"gn(string_split("kk", ["oops"]))gn",

    // Empty separator argument.
    R"gn(string_split("kk", ""))gn",
  };
  for (const auto& bad_usage_example : bad_usage_examples) {
    TestParseInput input(bad_usage_example);
    ASSERT_FALSE(input.has_error());
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error()) << bad_usage_example;
  }
}

TEST(Functions, DeclareArgs) {
  TestWithScope setup;
  Err err;

  // It is not legal to read the value of an argument declared in a
  // declare_args() from inside the call, but outside the call and in
  // a separate call should work.

  TestParseInput reading_from_same_call(R"(
      declare_args() {
        foo = true
        bar = foo
      })");
  reading_from_same_call.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());

  TestParseInput reading_from_outside_call(R"(
      declare_args() {
        foo = true
      }

      bar = foo
      assert(bar)
      )");
  err = Err();
  reading_from_outside_call.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error());

  TestParseInput reading_from_different_call(R"(
      declare_args() {
        foo = true
      }

      declare_args() {
        bar = foo
      }

      assert(bar)
      )");
  err = Err();
  TestWithScope setup2;
  reading_from_different_call.parsed()->Execute(setup2.scope(), &err);
  ASSERT_FALSE(err.has_error());
}

TEST(Functions, NotNeeded) {
  TestWithScope setup;

  TestParseInput input("not_needed({ a = 1 }, \"*\")");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error())
      << err.message() << err.location().Describe(true);
}

TEST(Template, PrintStackTraceWithOneTemplate) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  print(invoker.foo_value)\n"
      "  print_stack_trace()\n"
      "}\n"
      "foo(\"lala\") {\n"
      "  foo_value = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ(
    "lala\n"
    "42\n"
    "print_stack_trace() initiated at:  //test:4  using: //toolchain:default\n"
    "  foo(\"lala\")  //test:6\n"
    "  print_stack_trace()  //test:4\n",
    setup.print_output());
}

TEST(Template, PrintStackTraceWithNoTemplates) {
  TestWithScope setup;
  TestParseInput input("print_stack_trace()\n");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << "\n\n" << err.help_text();

  EXPECT_EQ(
    "print_stack_trace() initiated at:  //test:1  using: //toolchain:default\n"
    "  print_stack_trace()  //test:1\n",
    setup.print_output());
}


TEST(Template, PrintStackTraceWithNestedTemplates) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  print(invoker.foo_value)\n"
      "  print_stack_trace()\n"
      "}\n"
      "template(\"baz\") {\n"
      "  foo(\"${target_name}.foo\") {\n"
      "    foo_value = invoker.bar\n"
      "  }\n"
      "}\n"
      "baz(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << "\n\n" << err.help_text();

  EXPECT_EQ(
    "lala.foo\n"
    "42\n"
    "print_stack_trace() initiated at:  //test:4  using: //toolchain:default\n"
    "  baz(\"lala\")  //test:11\n"
    "  foo(\"lala.foo\")  //test:7\n"
    "  print_stack_trace()  //test:4\n",
    setup.print_output());
}

TEST(Template, PrintStackTraceWithNonTemplateScopes) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  if (defined(invoker.foo_value)) {\n"
      "    print(invoker.foo_value)\n"
      "    print_stack_trace()\n"
      "  }\n"
      "}\n"
      "template(\"baz\") {\n"
      "  foo(\"${target_name}.foo\") {\n"
      "    foo_value = invoker.bar\n"
      "  }\n"
      "}\n"
      "baz(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << "\n\n" << err.help_text();

  EXPECT_EQ(
    "lala.foo\n"
    "42\n"
    "print_stack_trace() initiated at:  //test:5  using: //toolchain:default\n"
    "  baz(\"lala\")  //test:13\n"
    "  foo(\"lala.foo\")  //test:9\n"
    "  print_stack_trace()  //test:5\n",
    setup.print_output());
}

TEST(Template, PrintStackTraceWithNonTemplateScopesBetweenTemplateInvocations) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  if (defined(invoker.foo_value)) {\n"
      "    print(invoker.foo_value)\n"
      "    print_stack_trace()\n"
      "  }\n"
      "}\n"
      "template(\"baz\") {\n"
      "  if (invoker.bar == 42) {\n"
      "    foo(\"${target_name}.foo\") {\n"
      "      foo_value = invoker.bar\n"
      "    }\n"
      "  }\n"
      "}\n"
      "baz(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());
  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << "\n\n" << err.help_text();

  EXPECT_EQ(
    "lala.foo\n"
    "42\n"
    "print_stack_trace() initiated at:  //test:5  using: //toolchain:default\n"
    "  baz(\"lala\")  //test:15\n"
    "  foo(\"lala.foo\")  //test:10\n"
    "  print_stack_trace()  //test:5\n",
    setup.print_output());
}

TEST(Template, PrintStackTraceWithTemplateDefinedWithinATemplate) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  if (defined(invoker.foo_value)) {\n"
      "    template(\"foo_internal\") {"
      "      print(target_name)\n"
      "      print(invoker.foo_internal_value)\n"
      "      print_stack_trace()\n"
      "    }\n"
      "    foo_internal(target_name+\".internal\") {"
      "      foo_internal_value = invoker.foo_value\n"
      "    }\n"
      "  }\n"
      "}\n"
      "template(\"baz\") {\n"
      "  if (invoker.bar == 42) {\n"
      "    foo(\"${target_name}.foo\") {\n"
      "      foo_value = invoker.bar\n"
      "    }\n"
      "  }\n"
      "}\n"
      "baz(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());
  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message() << "\n\n" << err.help_text();

  EXPECT_EQ(
    "lala.foo\n"
    "lala.foo.internal\n"
    "42\n"
    "print_stack_trace() initiated at:  //test:6  using: //toolchain:default\n"
    "  baz(\"lala\")  //test:19\n"
    "  foo(\"lala.foo\")  //test:14\n"
    "  foo_internal(\"lala.foo.internal\")  //test:8\n"
    "  print_stack_trace()  //test:6\n",
    setup.print_output());
}
