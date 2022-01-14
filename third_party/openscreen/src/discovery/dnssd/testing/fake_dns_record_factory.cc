// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/dnssd/testing/fake_dns_record_factory.h"

#include <utility>

namespace openscreen {
namespace discovery {

// static
MdnsRecord FakeDnsRecordFactory::CreateFullyPopulatedSrvRecord(uint16_t port) {
  const DomainName kTarget{kInstanceName, "_srv-name", "_udp", kDomainName};
  constexpr auto kType = DnsType::kSRV;
  constexpr auto kClazz = DnsClass::kIN;
  constexpr auto kRecordType = RecordType::kUnique;
  constexpr auto kTtl = std::chrono::seconds(0);
  SrvRecordRdata srv(0, 0, port, kTarget);
  return MdnsRecord(kTarget, kType, kClazz, kRecordType, kTtl, std::move(srv));
}

// static
constexpr uint16_t FakeDnsRecordFactory::kPortNum;

// static
const uint8_t FakeDnsRecordFactory::kV4AddressOctets[4] = {192, 168, 0, 0};

// static
const uint16_t FakeDnsRecordFactory::kV6AddressHextets[8] = {
    0x0102, 0x0304, 0x0506, 0x0708, 0x090a, 0x0b0c, 0x0d0e, 0x0f10};

// static
const char FakeDnsRecordFactory::kInstanceName[] = "instance";

// static
const char FakeDnsRecordFactory::kServiceName[] = "_srv-name._udp";

// static
const char FakeDnsRecordFactory::kServiceNameProtocolPart[] = "_udp";

// static
const char FakeDnsRecordFactory::kServiceNameServicePart[] = "_srv-name";

// static
const char FakeDnsRecordFactory::kDomainName[] = "local";

}  // namespace discovery
}  // namespace openscreen
