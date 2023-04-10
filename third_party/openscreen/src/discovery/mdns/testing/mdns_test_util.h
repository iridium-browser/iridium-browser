// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_TESTING_MDNS_TEST_UTIL_H_
#define DISCOVERY_MDNS_TESTING_MDNS_TEST_UTIL_H_

#include <initializer_list>

#include "absl/strings/string_view.h"
#include "discovery/mdns/mdns_records.h"

namespace openscreen {
namespace discovery {

const IPAddress kFakeARecordAddress = IPAddress(192, 168, 0, 0);
const IPAddress kFakeAAAARecordAddress = IPAddress(1, 2, 3, 4, 5, 6, 7, 8);
constexpr uint16_t kFakeSrvRecordPort = 80;

TxtRecordRdata MakeTxtRecord(std::initializer_list<absl::string_view> strings);

// Methods to create fake MdnsRecord entities for use in UnitTests.
MdnsRecord GetFakePtrRecord(const DomainName& target,
                            std::chrono::seconds ttl = std::chrono::seconds(1));
MdnsRecord GetFakeSrvRecord(const DomainName& name,
                            std::chrono::seconds ttl = std::chrono::seconds(1));
MdnsRecord GetFakeSrvRecord(const DomainName& name,
                            const DomainName& target,
                            std::chrono::seconds ttl = std::chrono::seconds(1));
MdnsRecord GetFakeTxtRecord(const DomainName& name,
                            std::chrono::seconds ttl = std::chrono::seconds(1));
MdnsRecord GetFakeARecord(const DomainName& name,
                          std::chrono::seconds ttl = std::chrono::seconds(1));
MdnsRecord GetFakeAAAARecord(
    const DomainName& name,
    std::chrono::seconds ttl = std::chrono::seconds(1));

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_TESTING_MDNS_TEST_UTIL_H_
