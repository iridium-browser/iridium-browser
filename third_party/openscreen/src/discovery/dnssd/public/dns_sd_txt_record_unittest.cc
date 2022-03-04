// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/public/dns_sd_txt_record.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {
namespace dnssd {

TEST(TxtRecordTest, TestCaseInsensitivity) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  EXPECT_TRUE(txt.GetValue("KEY").is_value());

  EXPECT_TRUE(txt.SetFlag("KEY2", true).ok());
  ASSERT_TRUE(txt.GetFlag("key2").is_value());
  EXPECT_TRUE(txt.GetFlag("key2").value());
}

TEST(TxtRecordTest, TestEmptyValue) {
  DnsSdTxtRecord txt;
  EXPECT_TRUE(txt.SetValue("key", std::vector<uint8_t>{}).ok());
  ASSERT_TRUE(txt.GetValue("key").is_value());
  EXPECT_EQ(txt.GetValue("key").value().get().size(), size_t{0});

  EXPECT_TRUE(txt.SetValue("key2", "").ok());
  ASSERT_TRUE(txt.GetValue("key2").is_value());
  EXPECT_EQ(txt.GetValue("key2").value().get().size(), size_t{0});
}

TEST(TxtRecordTest, TestSetAndGetValue) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  ASSERT_TRUE(txt.GetValue("key").is_value());
  const std::vector<uint8_t>& value = txt.GetValue("key").value();
  ASSERT_EQ(value.size(), size_t{3});
  EXPECT_EQ(value[0], 'a');
  EXPECT_EQ(value[1], 'b');
  EXPECT_EQ(value[2], 'c');

  std::vector<uint8_t> data2{'a', 'b'};
  EXPECT_TRUE(txt.SetValue("key", data2).ok());
  ASSERT_TRUE(txt.GetValue("key").is_value());
  const std::vector<uint8_t>& value2 = txt.GetValue("key").value();
  EXPECT_EQ(value2.size(), size_t{2});
  EXPECT_EQ(value2[0], 'a');
  EXPECT_EQ(value2[1], 'b');

  EXPECT_TRUE(txt.SetValue("key", "abc").ok());
  ASSERT_TRUE(txt.GetValue("key").is_value());
  const std::vector<uint8_t>& value3 = txt.GetValue("key").value();
  ASSERT_EQ(value.size(), size_t{3});
  EXPECT_EQ(value3[0], 'a');
  EXPECT_EQ(value3[1], 'b');
  EXPECT_EQ(value3[2], 'c');

  EXPECT_TRUE(txt.SetValue("key", "ab").ok());
  ASSERT_TRUE(txt.GetValue("key").is_value());
  const std::vector<uint8_t>& value4 = txt.GetValue("key").value();
  EXPECT_EQ(value4.size(), size_t{2});
  EXPECT_EQ(value4[0], 'a');
  EXPECT_EQ(value4[1], 'b');
}

TEST(TxtRecordTest, TestClearValue) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  txt.ClearValue("key");

  EXPECT_FALSE(txt.GetValue("key").is_value());
}

TEST(TxtRecordTest, TestSetAndGetFlag) {
  DnsSdTxtRecord txt;
  EXPECT_TRUE(txt.SetFlag("key", true).ok());
  ASSERT_TRUE(txt.GetFlag("key").is_value());
  EXPECT_TRUE(txt.GetFlag("key").value());

  EXPECT_TRUE(txt.SetFlag("key", false).ok());
  ASSERT_TRUE(txt.GetFlag("key").is_value());
  EXPECT_FALSE(txt.GetFlag("key").value());
}

TEST(TxtRecordTest, TestClearFlag) {
  DnsSdTxtRecord txt;
  EXPECT_TRUE(txt.SetFlag("key", true).ok());
  txt.ClearFlag("key");

  ASSERT_TRUE(txt.GetFlag("key").is_value());
  EXPECT_FALSE(txt.GetFlag("key").value());
}

TEST(TxtRecordTest, TestGettingWrongRecordTypeFails) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  EXPECT_TRUE(txt.SetFlag("key2", true).ok());
  EXPECT_FALSE(txt.GetValue("key2").is_value());
}

TEST(TxtRecordTest, TestClearWrongRecordTypeFails) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  EXPECT_TRUE(txt.SetFlag("key2", true).ok());
}

TEST(TxtRecordTest, TestGetDataWorks) {
  DnsSdTxtRecord txt;
  std::vector<uint8_t> data{'a', 'b', 'c'};
  EXPECT_TRUE(txt.SetValue("key", data).ok());
  EXPECT_TRUE(txt.SetFlag("bool", true).ok());
  std::vector<std::vector<uint8_t>> results = txt.GetData();
  ASSERT_EQ(results.size(), size_t{2});
  bool seen_flag = false;
  bool seen_kv_pair = false;
  for (const std::vector<uint8_t>& entry : results) {
    std::string flag = "bool";
    std::string kv_pair = "key=abc";
    for (size_t i = 0; i < flag.size(); i++) {
      if (entry[i] != flag[i]) {
        break;
      }
      if (i == flag.size() - 1) {
        seen_flag = true;
      }
    }
    for (size_t i = 0; i < kv_pair.size(); i++) {
      if (entry[i] != kv_pair[i]) {
        break;
      }
      if (i == kv_pair.size() - 1) {
        seen_kv_pair = true;
      }
    }
  }

  EXPECT_TRUE(seen_flag);
  EXPECT_TRUE(seen_kv_pair);
}

}  // namespace dnssd
}  // namespace discovery
}  // namespace openscreen
