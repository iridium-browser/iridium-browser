// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_TESTING_FAKE_DNS_RECORD_FACTORY_H_
#define DISCOVERY_DNSSD_TESTING_FAKE_DNS_RECORD_FACTORY_H_

#include <stdint.h>

#include <chrono>

#include "discovery/dnssd/impl/constants.h"
#include "discovery/mdns/mdns_records.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace discovery {

class FakeDnsRecordFactory {
 public:
  static constexpr uint16_t kPortNum = 80;
  static const uint8_t kV4AddressOctets[4];
  static const uint16_t kV6AddressHextets[8];
  static const char kInstanceName[];
  static const char kServiceName[];
  static const char kServiceNameProtocolPart[];
  static const char kServiceNameServicePart[];
  static const char kDomainName[];

  static MdnsRecord CreateFullyPopulatedSrvRecord(uint16_t port = kPortNum);
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_TESTING_FAKE_DNS_RECORD_FACTORY_H_
