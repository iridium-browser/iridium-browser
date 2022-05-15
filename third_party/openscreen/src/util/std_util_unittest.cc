// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/std_util.h"

#include "gtest/gtest.h"

namespace openscreen {

TEST(StdUtilTest, CountOf) {
  constexpr int three_ints[3] = {1, 2, 3};
  EXPECT_EQ(static_cast<size_t>(3), countof(three_ints));
}

TEST(StdUtilTest, Data) {
  std::string non_empty("Where no one has gone before");
  EXPECT_TRUE(data(non_empty) != nullptr);
  EXPECT_EQ(data(non_empty), non_empty.data());

  std::string empty;
  EXPECT_TRUE(data(empty) != nullptr);
  EXPECT_EQ(data(empty), empty.data());
}

TEST(StdUtilTest, Join) {
  std::vector<std::string> medals({"bronze", "silver", "gold"});
  EXPECT_EQ("bronzesilvergold", Join(medals, ""));
  EXPECT_EQ("bronze,silver,gold", Join(medals, ","));
  EXPECT_EQ("bronzeandsilverandgold", Join(medals, "and"));
  EXPECT_EQ("", Join(std::vector<std::string>({""}), ","));
}

TEST(StdUtilTest, RemoveValueFromMap) {
  std::string capitol1("Olympia");
  std::string capitol2("Eugene");
  std::string capitol3("Springfield");
  std::string capitol4("Sacramento");

  std::map<std::string, std::string*> capitol_map;
  capitol_map.insert(std::make_pair("Washington", &capitol1));
  capitol_map.insert(std::make_pair("Oregon", &capitol2));
  capitol_map.insert(std::make_pair("Massachusetts", &capitol3));
  capitol_map.insert(std::make_pair("Illinois", &capitol3));

  RemoveValueFromMap(&capitol_map, &capitol1);
  RemoveValueFromMap(&capitol_map, &capitol3);
  RemoveValueFromMap(&capitol_map, &capitol4);

  EXPECT_EQ(static_cast<size_t>(1), capitol_map.size());
  EXPECT_TRUE(capitol_map.find("Oregon") != capitol_map.end());
}

TEST(StdUtilTest, AreElementSortedAndUnique) {
  EXPECT_TRUE(AreElementsSortedAndUnique(std::vector<std::string>({})));
  EXPECT_TRUE(AreElementsSortedAndUnique(std::vector<std::string>({"Joey"})));
  EXPECT_TRUE(AreElementsSortedAndUnique(
      std::vector<std::string>({"Chandler", "Joey", "Phoebe"})));
  EXPECT_FALSE(AreElementsSortedAndUnique(
      std::vector<std::string>({"Chandler", "Joey", "Joey", "Phoebe"})));
  EXPECT_FALSE(AreElementsSortedAndUnique(
      std::vector<std::string>({"Chandler", "Phoebe", "Joey"})));
}

TEST(StdUtilTest, SortAndDedupeElements) {
  auto empty_vector = std::vector<std::string>({});
  SortAndDedupeElements(&empty_vector);
  EXPECT_TRUE(empty_vector.empty());

  auto singleton_vector = std::vector<std::string>({"Joey"});
  SortAndDedupeElements(&singleton_vector);
  EXPECT_EQ(singleton_vector, std::vector<std::string>({"Joey"}));

  auto all_friends =
      std::vector<std::string>({"Joey", "Rachel", "Monica", "Chandler",
                                "Phoebe", "Ross", "Rachel", "Joey"});
  SortAndDedupeElements(&all_friends);
  EXPECT_EQ(all_friends,
            std::vector<std::string>(
                {"Chandler", "Joey", "Monica", "Phoebe", "Rachel", "Ross"}));
}

TEST(StdUtilTest, Append) {
  std::vector<std::string> one_friend({"Joey"});
  auto friends = Append(std::move(one_friend), "Rachel", "Monica", "Chandler",
                        "Phoebe", "Ross");
  EXPECT_EQ(std::vector<std::string>(
                {"Joey", "Rachel", "Monica", "Chandler", "Phoebe", "Ross"}),
            friends);
}

TEST(StdUtilTest, GetVectorWithCapacity) {
  auto ten_strings(GetVectorWithCapacity<std::string>(10));
  EXPECT_EQ(static_cast<size_t>(0), ten_strings.size());
  EXPECT_EQ(static_cast<size_t>(10), ten_strings.capacity());
}

TEST(StdUtilTest, Contains) {
  auto friends = std::vector<std::string>(
      {"Joey", "Rachel", "Monica", "Chandler", "Phoebe", "Ross"});
  EXPECT_TRUE(Contains(friends, "Rachel"));
  EXPECT_FALSE(Contains(friends, "Ursula"));
}

TEST(StdUtilTest, ContainsIf) {
  auto friends = std::vector<std::string>(
      {"Joey", "Rachel", "Monica", "Chandler", "Phoebe", "Ross"});
  EXPECT_TRUE(
      ContainsIf(friends, [](const auto& f) { return f == "Chandler"; }));
  EXPECT_FALSE(
      ContainsIf(friends, [](const auto& f) { return f == "Ursula"; }));
}

}  // namespace openscreen
