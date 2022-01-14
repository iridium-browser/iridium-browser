// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/impl/instance_key.h"

#include <unordered_map>

#include "absl/hash/hash.h"
#include "discovery/dnssd/impl/service_key.h"
#include "discovery/dnssd/testing/fake_dns_record_factory.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

TEST(DnsSdInstanceKeyTest, TestInstanceKeyEquals) {
  InstanceKey key1("instance", "_service._udp", "domain");
  InstanceKey key2("instance", "_service._udp", "domain");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);

  key1 = InstanceKey("instance", "_service2._udp", "domain");
  EXPECT_FALSE(key1 == key2);
  EXPECT_TRUE(key1 != key2);
  key2 = InstanceKey("instance", "_service2._udp", "domain");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);

  key1 = InstanceKey("instance", "_service2._udp", "domain2");
  EXPECT_FALSE(key1 == key2);
  EXPECT_TRUE(key1 != key2);
  key2 = InstanceKey("instance", "_service2._udp", "domain2");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);

  key1 = InstanceKey("instance2", "_service2._udp", "domain2");
  EXPECT_FALSE(key1 == key2);
  EXPECT_TRUE(key1 != key2);
  key2 = InstanceKey("instance2", "_service2._udp", "domain2");
  EXPECT_TRUE(key1 == key2);
  EXPECT_FALSE(key1 != key2);
}

TEST(DnsSdInstanceKeyTest, TestIsInstanceOf) {
  ServiceKey ptr("_service._udp", "domain");
  InstanceKey svc("instance", "_service._udp", "domain");
  EXPECT_EQ(svc, ptr);

  svc = InstanceKey("other id", "_service._udp", "domain");
  EXPECT_EQ(svc, ptr);

  svc = InstanceKey("instance", "_service._udp", "domain2");
  EXPECT_FALSE(svc == ptr);
  ptr = ServiceKey("_service._udp", "domain2");
  EXPECT_EQ(svc, ptr);

  svc = InstanceKey("instance", "_service2._udp", "domain");
  EXPECT_NE(svc, ptr);
  ptr = ServiceKey("_service2._udp", "domain");
  EXPECT_EQ(svc, ptr);
}

TEST(DnsSdInstanceKeyTest, InstanceKeyInMap) {
  InstanceKey key{"instance", "_service._udp", "domain"};
  InstanceKey key2{"instance", "_service._udp", "domain"};
  InstanceKey key3{"instance2", "_service._udp", "domain"};
  InstanceKey key4{"instance", "_service2._udp", "domain"};
  InstanceKey key5{"instance", "_service2._udp", "domain2"};
  std::unordered_map<InstanceKey, int32_t, absl::Hash<InstanceKey>> map;
  map[key] = 1;
  map[key2] = 2;
  EXPECT_EQ(map[key], 2);

  map[key3] = 3;
  EXPECT_EQ(map[key], 2);
  EXPECT_EQ(map[key3], 3);

  map[key4] = 4;
  EXPECT_EQ(map[key], 2);
  EXPECT_EQ(map[key3], 3);
  EXPECT_EQ(map[key4], 4);

  map[key5] = 5;
  EXPECT_EQ(map[key], 2);
  EXPECT_EQ(map[key3], 3);
  EXPECT_EQ(map[key4], 4);
  EXPECT_EQ(map[key5], 5);
}

TEST(DnsSdInstanceKeyTest, CreateFromRecordTest) {
  MdnsRecord record = FakeDnsRecordFactory::CreateFullyPopulatedSrvRecord();
  InstanceKey key(record);
  EXPECT_EQ(key.instance_id(), FakeDnsRecordFactory::kInstanceName);
  EXPECT_EQ(key.service_id(), FakeDnsRecordFactory::kServiceName);
  EXPECT_EQ(key.domain_id(), FakeDnsRecordFactory::kDomainName);
}

TEST(DnsSdInstanceKeyTest, GetNameTest) {
  InstanceKey key("instance", "_service._udp", "domain");
  DomainName expected{"instance", "_service", "_udp", "domain"};
  EXPECT_EQ(expected, key.GetName());

  key = InstanceKey("foo", "_bar._tcp", "local");
  expected = DomainName{"foo", "_bar", "_tcp", "local"};
  EXPECT_EQ(expected, key.GetName());
}

}  // namespace discovery
}  // namespace openscreen
