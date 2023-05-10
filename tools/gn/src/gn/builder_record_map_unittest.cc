// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be

#include "gn/builder_record_map.h"
#include "gn/builder_record.h"
#include "gn/label.h"
#include "gn/source_dir.h"
#include "util/test/test.h"

TEST(BuilderRecordMap, Construction) {
  const Label kLabel1(SourceDir("//src"), "foo");
  BuilderRecordMap map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(0u, map.size());
  EXPECT_FALSE(map.find(kLabel1));
}

TEST(BuilderRecordMap, TryEmplace) {
  const Label kLabel1(SourceDir("//src"), "foo");
  const Label kLabel2(SourceDir("//src"), "bar");
  const Label kLabel3(SourceDir("//third_party/src"), "zoo");

  BuilderRecordMap map;

  auto ret = map.try_emplace(kLabel1, nullptr, BuilderRecord::ITEM_TARGET);
  EXPECT_TRUE(ret.first);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(BuilderRecord::ITEM_TARGET, ret.second->type());
  EXPECT_EQ(kLabel1, ret.second->label());
  EXPECT_EQ(ret.second, map.find(kLabel1));
  EXPECT_EQ(1u, map.size());

  BuilderRecord* record = ret.second;
  ret = map.try_emplace(kLabel1, nullptr, BuilderRecord::ITEM_CONFIG);
  EXPECT_FALSE(ret.first);
  EXPECT_EQ(record, ret.second);
  EXPECT_EQ(1u, map.size());

  ret = map.try_emplace(kLabel2, nullptr, BuilderRecord::ITEM_CONFIG);
  EXPECT_TRUE(ret.first);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(2u, map.size());
  EXPECT_EQ(BuilderRecord::ITEM_CONFIG, ret.second->type());
  EXPECT_EQ(kLabel2, ret.second->label());
  EXPECT_EQ(ret.second, map.find(kLabel2));

  ret = map.try_emplace(kLabel3, nullptr, BuilderRecord::ITEM_TOOLCHAIN);
  EXPECT_TRUE(ret.first);
  EXPECT_TRUE(ret.second);
  EXPECT_EQ(3u, map.size());
  EXPECT_EQ(BuilderRecord::ITEM_TOOLCHAIN, ret.second->type());
  EXPECT_EQ(kLabel3, ret.second->label());
  EXPECT_EQ(ret.second, map.find(kLabel3));

  ret = map.try_emplace(kLabel2, nullptr, BuilderRecord::ITEM_CONFIG);
  EXPECT_FALSE(ret.first);

  EXPECT_EQ(3u, map.size());
  EXPECT_TRUE(map.find(kLabel1));
  EXPECT_TRUE(map.find(kLabel2));
  EXPECT_TRUE(map.find(kLabel3));
}
