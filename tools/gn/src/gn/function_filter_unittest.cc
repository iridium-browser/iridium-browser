// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/functions.h"
#include "gn/test_with_scope.h"
#include "util/build_config.h"
#include "util/test/test.h"

TEST(FilterExcludeTest, Filter) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*.proto"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  ASSERT_FALSE(err.has_error());
  ASSERT_EQ(result.type(), Value::LIST);
  ASSERT_EQ(result.list_value().size(), 2);
  EXPECT_EQ(result.list_value()[0].type(), Value::STRING);
  EXPECT_EQ(result.list_value()[0].string_value(), "foo.cc");
  EXPECT_EQ(result.list_value()[1].type(), Value::STRING);
  EXPECT_EQ(result.list_value()[1].string_value(), "foo.h");
}

TEST(FilterExcludeTest, NotEnoughArguments) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  FunctionCallNode function;
  std::vector<Value> args = {values};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterExcludeTest, TooManyArguments) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  Value extra_argument(nullptr, Value::LIST);

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns, extra_argument};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterExcludeTest, IncorrectValuesType) {
  TestWithScope setup;

  Value values(nullptr, "foo.cc");

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterExcludeTest, IncorrectValuesElementType) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, Value::LIST));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterExcludeTest, IncorrectPatternsType) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, "foo.cc");

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterExclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterIncludeTest, Filter) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*.proto"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  ASSERT_FALSE(err.has_error());
  ASSERT_EQ(result.type(), Value::LIST);
  ASSERT_EQ(result.list_value().size(), 1);
  EXPECT_EQ(result.list_value()[0].type(), Value::STRING);
  EXPECT_EQ(result.list_value()[0].string_value(), "foo.proto");
}

TEST(FilterIncludeTest, NotEnoughArguments) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  FunctionCallNode function;
  std::vector<Value> args = {values};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterIncludeTest, TooManyArguments) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  Value extra_argument(nullptr, Value::LIST);

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns, extra_argument};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterIncludeTest, IncorrectValuesType) {
  TestWithScope setup;

  Value values(nullptr, "foo.cc");

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterIncludeTest, IncorrectValuesElementType) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, Value::LIST));

  Value patterns(nullptr, Value::LIST);
  patterns.list_value().push_back(Value(nullptr, "*"));

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}

TEST(FilterIncludeTest, IncorrectPatternsType) {
  TestWithScope setup;

  Value values(nullptr, Value::LIST);
  values.list_value().push_back(Value(nullptr, "foo.cc"));
  values.list_value().push_back(Value(nullptr, "foo.h"));
  values.list_value().push_back(Value(nullptr, "foo.proto"));

  Value patterns(nullptr, "foo.cc");

  FunctionCallNode function;
  std::vector<Value> args = {values, patterns};

  Err err;
  Value result =
      functions::RunFilterInclude(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}
