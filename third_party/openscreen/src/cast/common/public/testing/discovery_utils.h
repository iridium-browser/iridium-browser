// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_PUBLIC_TESTING_DISCOVERY_UTILS_H_
#define CAST_COMMON_PUBLIC_TESTING_DISCOVERY_UTILS_H_

#include <string>

#include "cast/common/public/receiver_info.h"
#include "discovery/dnssd/public/dns_sd_txt_record.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace cast {

// Constants used for testing.
extern const IPAddress kAddressV4;
extern const IPAddress kAddressV6;
constexpr uint16_t kPort = 80;
extern const IPEndpoint kEndpointV4;
extern const IPEndpoint kEndpointV6;
constexpr char kTestUniqueId[] = "1234";
constexpr char kFriendlyName[] = "Friendly Name 123";
constexpr char kModelName[] = "Openscreen";
constexpr char kInstanceId[] = "Openscreen-1234";
constexpr uint8_t kTestVersion = 5;
constexpr char kCapabilitiesString[] = "3";
constexpr char kCapabilitiesStringLong[] = "000003";
constexpr uint64_t kCapabilitiesParsed = 0x03;
constexpr uint8_t kStatus = 0x01;
constexpr ReceiverStatus kStatusParsed = ReceiverStatus::kBusy;

discovery::DnsSdTxtRecord CreateValidTxt();

void CompareTxtString(const discovery::DnsSdTxtRecord& txt,
                      const std::string& key,
                      const std::string& expected);

void CompareTxtInt(const discovery::DnsSdTxtRecord& txt,
                   const std::string& key,
                   int expected);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_PUBLIC_TESTING_DISCOVERY_UTILS_H_
