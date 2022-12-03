// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/service_key.h"

#include <unordered_map>

#include "absl/hash/hash.h"
#include "discovery/dnssd/testing/fake_dns_record_factory.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

TEST(DnsSdServiceKeyTest, TestServiceKeyEquals) {
  ServiceKey key1("_service._udp", "domain");
  ServiceKey key2("_service._udp", "domain");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);

  key1 = ServiceKey("_service2._udp", "domain");
  EXPECT_FALSE(key1 == key2);
  EXPECT_TRUE(key1 != key2);
  key2 = ServiceKey("_service2._udp", "domain");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);

  key1 = ServiceKey("_service._udp", "domain2");
  EXPECT_FALSE(key1 == key2);
  EXPECT_TRUE(key1 != key2);
  key2 = ServiceKey("_service._udp", "domain2");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);
}

TEST(DnsSdServiceKeyTest, ServiceKeyInMap) {
  ServiceKey ptr{"_service._udp", "domain"};
  ServiceKey ptr2{"_service._udp", "domain"};
  ServiceKey ptr3{"_service2._udp", "domain"};
  ServiceKey ptr4{"_service._udp", "domain2"};
  std::unordered_map<ServiceKey, int32_t, absl::Hash<ServiceKey>> map;
  map[ptr] = 1;
  map[ptr2] = 2;
  EXPECT_EQ(map[ptr], 2);

  map[ptr3] = 3;
  EXPECT_EQ(map[ptr], 2);
  EXPECT_EQ(map[ptr3], 3);

  map[ptr4] = 4;
  EXPECT_EQ(map[ptr], 2);
  EXPECT_EQ(map[ptr3], 3);
  EXPECT_EQ(map[ptr4], 4);
}

TEST(DnsSdServiceKeyTest, CreateFromRecordTest) {
  MdnsRecord record = FakeDnsRecordFactory::CreateFullyPopulatedSrvRecord();
  ServiceKey key(record);
  EXPECT_EQ(key.service_id(), FakeDnsRecordFactory::kServiceName);
  EXPECT_EQ(key.domain_id(), FakeDnsRecordFactory::kDomainName);
}

}  // namespace discovery
}  // namespace openscreen
