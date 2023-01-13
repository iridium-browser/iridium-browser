// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/metadata.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

TEST(MetadataTest, SetContents) {
  Metadata metadata;

  ASSERT_TRUE(metadata.contents().empty());

  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo"));
  Value b_expected(nullptr, Value::LIST);
  b_expected.list_value().push_back(Value(nullptr, true));

  Metadata::Contents contents;
  contents.insert(std::pair<std::string_view, Value>("a", a_expected));
  contents.insert(std::pair<std::string_view, Value>("b", b_expected));

  metadata.set_contents(std::move(contents));

  ASSERT_EQ(metadata.contents().size(), 2);
  auto a_actual = metadata.contents().find("a");
  auto b_actual = metadata.contents().find("b");
  ASSERT_FALSE(a_actual == metadata.contents().end());
  ASSERT_FALSE(b_actual == metadata.contents().end());
  ASSERT_EQ(a_actual->second, a_expected);
  ASSERT_EQ(b_actual->second, b_expected);
}

TEST(MetadataTest, Walk) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo.cpp"));
  a_expected.list_value().push_back(Value(nullptr, "bar.h"));

  metadata.contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  std::vector<std::string> data_keys;
  data_keys.emplace_back("a");
  std::vector<std::string> walk_keys;
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected;
  expected.emplace_back(Value(nullptr, "foo.cpp"));
  expected.emplace_back(Value(nullptr, "bar.h"));

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir(), &next_walk_keys,
                                &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_EQ(results, expected);
}

TEST(MetadataTest, WalkWithRebase) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo.cpp"));
  a_expected.list_value().push_back(Value(nullptr, "foo/bar.h"));

  metadata.contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  std::vector<std::string> data_keys;
  data_keys.emplace_back("a");
  std::vector<std::string> walk_keys;
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected;
  expected.emplace_back(Value(nullptr, "../home/files/foo.cpp"));
  expected.emplace_back(Value(nullptr, "../home/files/foo/bar.h"));

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir("/usr/foo_dir/"),
                                &next_walk_keys, &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_EQ(results, expected);
}

TEST(MetadataTest, WalkWithRebaseNonString) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  Value a(nullptr, Value::LIST);
  Value inner_list(nullptr, Value::LIST);
  Value inner_scope(nullptr, Value::SCOPE);
  inner_list.list_value().push_back(Value(nullptr, "foo.cpp"));
  inner_list.list_value().push_back(Value(nullptr, "foo/bar.h"));
  a.list_value().push_back(inner_list);

  std::unique_ptr<Scope> scope(new Scope(setup.settings()));
  scope->SetValue("a1", Value(nullptr, "foo2.cpp"), nullptr);
  scope->SetValue("a2", Value(nullptr, "foo/bar2.h"), nullptr);
  inner_scope.SetScopeValue(std::move(scope));
  a.list_value().push_back(inner_scope);

  metadata.contents().insert(std::pair<std::string_view, Value>("a", a));
  std::vector<std::string> data_keys;
  data_keys.emplace_back("a");
  std::vector<std::string> walk_keys;
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected;
  Value inner_list_expected(nullptr, Value::LIST);
  Value inner_scope_expected(nullptr, Value::SCOPE);
  inner_list_expected.list_value().push_back(
      Value(nullptr, "../home/files/foo.cpp"));
  inner_list_expected.list_value().push_back(
      Value(nullptr, "../home/files/foo/bar.h"));
  expected.push_back(inner_list_expected);

  std::unique_ptr<Scope> scope_expected(new Scope(setup.settings()));
  scope_expected->SetValue("a1", Value(nullptr, "../home/files/foo2.cpp"),
                           nullptr);
  scope_expected->SetValue("a2", Value(nullptr, "../home/files/foo/bar2.h"),
                           nullptr);
  inner_scope_expected.SetScopeValue(std::move(scope_expected));
  expected.push_back(inner_scope_expected);

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir("/usr/foo_dir/"),
                                &next_walk_keys, &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_EQ(results, expected);
}

TEST(MetadataTest, WalkKeysToWalk) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "//target"));

  metadata.contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  std::vector<std::string> data_keys;
  std::vector<std::string> walk_keys;
  walk_keys.emplace_back("a");
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "//target");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir(), &next_walk_keys,
                                &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_TRUE(results.empty());
}

TEST(MetadataTest, WalkNoContents) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  std::vector<std::string> data_keys;
  std::vector<std::string> walk_keys;
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir(), &next_walk_keys,
                                &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_TRUE(results.empty());
}

TEST(MetadataTest, WalkNoKeysWithContents) {
  TestWithScope setup;
  Metadata metadata;
  metadata.set_source_dir(SourceDir("/usr/home/files/"));

  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "//target"));

  metadata.contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  std::vector<std::string> data_keys;
  std::vector<std::string> walk_keys;
  std::vector<Value> next_walk_keys;
  std::vector<Value> results;

  std::vector<Value> expected_walk_keys;
  expected_walk_keys.emplace_back(nullptr, "");

  Err err;
  EXPECT_TRUE(metadata.WalkStep(setup.settings()->build_settings(), data_keys,
                                walk_keys, SourceDir(), &next_walk_keys,
                                &results, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(next_walk_keys, expected_walk_keys);
  EXPECT_TRUE(results.empty());
}
