// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/public/receiver_info.h"

#include <cstdio>
#include <sstream>

#include "cast/common/public/testing/discovery_utils.h"
#include "discovery/dnssd/public/dns_sd_instance.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {
namespace {

constexpr NetworkInterfaceIndex kNetworkInterface = 0;

}

TEST(ReceiverInfoTests, ConvertValidFromDnsSd) {
  std::string instance = "InstanceId";
  discovery::DnsSdTxtRecord txt = CreateValidTxt();
  discovery::DnsSdInstanceEndpoint record(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  ErrorOr<ReceiverInfo> info = DnsSdInstanceEndpointToReceiverInfo(record);
  ASSERT_TRUE(info.is_value()) << info;
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_TRUE(info.value().v4_address);
  EXPECT_EQ(info.value().v4_address, kAddressV4);
  EXPECT_TRUE(info.value().v6_address);
  EXPECT_EQ(info.value().v6_address, kAddressV6);
  EXPECT_EQ(info.value().port, kPort);
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_EQ(info.value().protocol_version, kTestVersion);
  EXPECT_EQ(info.value().capabilities, kCapabilitiesParsed);
  EXPECT_EQ(info.value().status, kStatusParsed);
  EXPECT_EQ(info.value().model_name, kModelName);
  EXPECT_EQ(info.value().friendly_name, kFriendlyName);

  record = discovery::DnsSdInstanceEndpoint(instance, kCastV2ServiceId,
                                            kCastV2DomainId, txt,
                                            kNetworkInterface, kEndpointV4);
  info = DnsSdInstanceEndpointToReceiverInfo(record);
  ASSERT_TRUE(info.is_value());
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_TRUE(info.value().v4_address);
  EXPECT_EQ(info.value().v4_address, kAddressV4);
  EXPECT_FALSE(info.value().v6_address);
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_EQ(info.value().protocol_version, kTestVersion);
  EXPECT_EQ(info.value().capabilities, kCapabilitiesParsed);
  EXPECT_EQ(info.value().status, kStatusParsed);
  EXPECT_EQ(info.value().model_name, kModelName);
  EXPECT_EQ(info.value().friendly_name, kFriendlyName);

  record = discovery::DnsSdInstanceEndpoint(instance, kCastV2ServiceId,
                                            kCastV2DomainId, txt,
                                            kNetworkInterface, kEndpointV6);
  info = DnsSdInstanceEndpointToReceiverInfo(record);
  ASSERT_TRUE(info.is_value());
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_FALSE(info.value().v4_address);
  EXPECT_TRUE(info.value().v6_address);
  EXPECT_EQ(info.value().v6_address, kAddressV6);
  EXPECT_EQ(info.value().unique_id, kTestUniqueId);
  EXPECT_EQ(info.value().protocol_version, kTestVersion);
  EXPECT_EQ(info.value().capabilities, kCapabilitiesParsed);
  EXPECT_EQ(info.value().status, kStatusParsed);
  EXPECT_EQ(info.value().model_name, kModelName);
  EXPECT_EQ(info.value().friendly_name, kFriendlyName);
}

TEST(ReceiverInfoTests, ConvertInvalidFromDnsSd) {
  std::string instance = "InstanceId";
  discovery::DnsSdTxtRecord txt = CreateValidTxt();
  txt.ClearValue(kUniqueIdKey);
  discovery::DnsSdInstanceEndpoint record(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  EXPECT_TRUE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());

  txt = CreateValidTxt();
  txt.ClearValue(kVersionKey);
  record = discovery::DnsSdInstanceEndpoint(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  EXPECT_TRUE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());

  txt = CreateValidTxt();
  txt.ClearValue(kCapabilitiesKey);
  record = discovery::DnsSdInstanceEndpoint(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  EXPECT_TRUE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());

  txt = CreateValidTxt();
  txt.ClearValue(kStatusKey);
  record = discovery::DnsSdInstanceEndpoint(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  EXPECT_TRUE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());

  txt = CreateValidTxt();
  txt.ClearValue(kFriendlyNameKey);
  record = discovery::DnsSdInstanceEndpoint(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  EXPECT_TRUE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());

  txt = CreateValidTxt();
  txt.ClearValue(kModelNameKey);
  record = discovery::DnsSdInstanceEndpoint(
      instance, kCastV2ServiceId, kCastV2DomainId, txt, kNetworkInterface,
      kEndpointV4, kEndpointV6);
  // Note: Model name is an optional field.
  EXPECT_FALSE(DnsSdInstanceEndpointToReceiverInfo(record).is_error());
}

TEST(ReceiverInfoTests, ConvertValidToDnsSd) {
  ReceiverInfo info;
  info.v4_address = kAddressV4;
  info.v6_address = kAddressV6;
  info.port = kPort;
  info.unique_id = kTestUniqueId;
  info.protocol_version = kTestVersion;
  info.capabilities = kCapabilitiesParsed;
  info.status = kStatusParsed;
  info.model_name = kModelName;
  info.friendly_name = kFriendlyName;
  discovery::DnsSdInstance instance = ReceiverInfoToDnsSdInstance(info);
  CompareTxtString(instance.txt(), kUniqueIdKey, kTestUniqueId);
  CompareTxtString(instance.txt(), kCapabilitiesKey, kCapabilitiesString);
  CompareTxtString(instance.txt(), kModelNameKey, kModelName);
  CompareTxtString(instance.txt(), kFriendlyNameKey, kFriendlyName);
  CompareTxtInt(instance.txt(), kVersionKey, kTestVersion);
  CompareTxtInt(instance.txt(), kStatusKey, kStatus);
}

TEST(ReceiverInfoTests, ParseReceiverInfoFromRealTXT) {
  constexpr struct {
    const char* key;
    const char* value;
  } kRealTXTForReceiverCastingYoutube[] = {
      {"bs", "FA99CBBF17D0"},
      // Note: Includes bits set that are not known:
      {"ca", "208901"},
      {"cd", "FED81089FA3FF851CF088AB33AB014C0"},
      {"fn", "⚡ Yurovision® ULTRA™"},
      {"ic", "/setup/icon.png"},
      {"id", "4ef522244a5a877f35ddead7d98702e6"},
      {"md", "Chromecast Ultra"},
      {"nf", "2"},
      {"rm", "6342FE65DD269999"},
      {"rs", "YouTube"},
      {"st", "1"},
      {"ve", "05"},
  };

  discovery::DnsSdTxtRecord txt;
  for (const auto e : kRealTXTForReceiverCastingYoutube) {
    ASSERT_TRUE(txt.SetValue(e.key, e.value).ok());
  }
  const discovery::DnsSdInstanceEndpoint record(
      "InstanceId", kCastV2ServiceId, kCastV2DomainId, std::move(txt),
      kNetworkInterface, kEndpointV4, kEndpointV6);

  const ErrorOr<ReceiverInfo> result =
      DnsSdInstanceEndpointToReceiverInfo(record);
  const ReceiverInfo& info = result.value();
  EXPECT_EQ(info.unique_id, "4ef522244a5a877f35ddead7d98702e6");
  EXPECT_EQ(info.protocol_version, 5);
  EXPECT_TRUE(info.capabilities & (kHasVideoOutput | kHasAudioOutput));
  EXPECT_EQ(info.status, kBusy);
  EXPECT_EQ(info.model_name, "Chromecast Ultra");
  EXPECT_EQ(info.friendly_name, "⚡ Yurovision® ULTRA™");
}

}  // namespace cast
}  // namespace openscreen
