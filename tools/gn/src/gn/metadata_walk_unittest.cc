// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/metadata_walk.h"

#include "gn/metadata.h"
#include "gn/target.h"
#include "gn/test_with_scope.h"
#include "gn/unique_vector.h"
#include "util/test/test.h"

TEST(MetadataWalkTest, CollectNoRecurse) {
  TestWithScope setup;

  TestTarget one(setup, "//foo:one", Target::SOURCE_SET);
  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  Value b_expected(nullptr, Value::LIST);
  b_expected.list_value().push_back(Value(nullptr, true));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("b", b_expected));

  one.metadata().set_source_dir(SourceDir("/usr/home/files/"));

  TestTarget two(setup, "//foo:two", Target::SOURCE_SET);
  Value a_2_expected(nullptr, Value::LIST);
  a_2_expected.list_value().push_back(Value(nullptr, "bar"));
  two.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_2_expected));

  Value b_2_expected(nullptr, Value::LIST);
  b_2_expected.list_value().push_back(Value(nullptr, false));
  two.metadata().contents().insert(
      std::pair<std::string_view, Value>("b", b_2_expected));

  two.metadata().set_source_dir(SourceDir("/usr/home/files/inner"));

  UniqueVector<const Target*> targets;
  targets.push_back(&one);
  targets.push_back(&two);

  std::vector<std::string> data_keys;
  data_keys.push_back("a");
  data_keys.push_back("b");

  std::vector<std::string> walk_keys;

  Err err;
  std::set<const Target*> targets_walked;
  std::vector<Value> result = WalkMetadata(targets, data_keys, walk_keys,
                                           SourceDir(), &targets_walked, &err);
  EXPECT_FALSE(err.has_error());

  std::vector<Value> expected;
  expected.push_back(Value(nullptr, "foo"));
  expected.push_back(Value(nullptr, true));
  expected.push_back(Value(nullptr, "bar"));
  expected.push_back(Value(nullptr, false));
  EXPECT_EQ(result, expected);

  std::set<const Target*> expected_walked_targets;
  expected_walked_targets.insert(&one);
  expected_walked_targets.insert(&two);
  EXPECT_EQ(targets_walked, expected_walked_targets);
}

TEST(MetadataWalkTest, CollectWithRecurse) {
  TestWithScope setup;

  TestTarget one(setup, "//foo:one", Target::SOURCE_SET);
  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  Value b_expected(nullptr, Value::LIST);
  b_expected.list_value().push_back(Value(nullptr, true));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("b", b_expected));

  TestTarget two(setup, "//foo:two", Target::SOURCE_SET);
  Value a_2_expected(nullptr, Value::LIST);
  a_2_expected.list_value().push_back(Value(nullptr, "bar"));
  two.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_2_expected));

  one.public_deps().push_back(LabelTargetPair(&two));

  UniqueVector<const Target*> targets;
  targets.push_back(&one);

  std::vector<std::string> data_keys;
  data_keys.push_back("a");
  data_keys.push_back("b");

  std::vector<std::string> walk_keys;

  Err err;
  std::set<const Target*> targets_walked;
  std::vector<Value> result = WalkMetadata(targets, data_keys, walk_keys,
                                           SourceDir(), &targets_walked, &err);
  EXPECT_FALSE(err.has_error());

  std::vector<Value> expected;
  expected.push_back(Value(nullptr, "bar"));
  expected.push_back(Value(nullptr, "foo"));
  expected.push_back(Value(nullptr, true));
  EXPECT_EQ(result, expected);

  std::set<const Target*> expected_walked_targets;
  expected_walked_targets.insert(&one);
  expected_walked_targets.insert(&two);
  EXPECT_EQ(targets_walked, expected_walked_targets);
}

TEST(MetadataWalkTest, CollectWithBarrier) {
  TestWithScope setup;

  TestTarget one(setup, "//foo:one", Target::SOURCE_SET);
  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  Value walk_expected(nullptr, Value::LIST);
  walk_expected.list_value().push_back(
      Value(nullptr, "//foo:two(//toolchain:default)"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("walk", walk_expected));

  TestTarget two(setup, "//foo:two", Target::SOURCE_SET);
  Value a_2_expected(nullptr, Value::LIST);
  a_2_expected.list_value().push_back(Value(nullptr, "bar"));
  two.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_2_expected));

  TestTarget three(setup, "//foo:three", Target::SOURCE_SET);
  Value a_3_expected(nullptr, Value::LIST);
  a_3_expected.list_value().push_back(Value(nullptr, "baz"));
  three.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_3_expected));

  one.public_deps().push_back(LabelTargetPair(&two));
  one.public_deps().push_back(LabelTargetPair(&three));

  UniqueVector<const Target*> targets;
  targets.push_back(&one);

  std::vector<std::string> data_keys;
  data_keys.push_back("a");

  std::vector<std::string> walk_keys;
  walk_keys.push_back("walk");

  Err err;
  std::set<const Target*> targets_walked;
  std::vector<Value> result = WalkMetadata(targets, data_keys, walk_keys,
                                           SourceDir(), &targets_walked, &err);
  EXPECT_FALSE(err.has_error()) << err.message();

  std::vector<Value> expected;
  expected.push_back(Value(nullptr, "bar"));
  expected.push_back(Value(nullptr, "foo"));
  EXPECT_EQ(result, expected) << result.size();

  std::set<const Target*> expected_walked_targets;
  expected_walked_targets.insert(&one);
  expected_walked_targets.insert(&two);
  EXPECT_EQ(targets_walked, expected_walked_targets);
}

TEST(MetadataWalkTest, CollectWithError) {
  TestWithScope setup;

  TestTarget one(setup, "//foo:one", Target::SOURCE_SET);
  Value a_expected(nullptr, Value::LIST);
  a_expected.list_value().push_back(Value(nullptr, "foo"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("a", a_expected));

  Value walk_expected(nullptr, Value::LIST);
  walk_expected.list_value().push_back(Value(nullptr, "//foo:missing"));
  one.metadata().contents().insert(
      std::pair<std::string_view, Value>("walk", walk_expected));

  UniqueVector<const Target*> targets;
  targets.push_back(&one);

  std::vector<std::string> data_keys;
  data_keys.push_back("a");

  std::vector<std::string> walk_keys;
  walk_keys.push_back("walk");

  Err err;
  std::set<const Target*> targets_walked;
  std::vector<Value> result = WalkMetadata(targets, data_keys, walk_keys,
                                           SourceDir(), &targets_walked, &err);
  EXPECT_TRUE(result.empty());
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ(err.message(),
            "I was expecting //foo:missing(//toolchain:default) to be a "
            "dependency of //foo:one(//toolchain:default). "
            "Make sure it's included in the deps or data_deps, and that you've "
            "specified the appropriate toolchain.")
      << err.message();
}
