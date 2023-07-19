// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/public/receiver_info.h"

#include <cctype>
#include <cinttypes>
#include <string>
#include <vector>

#include "absl/strings/numbers.h"
#include "absl/strings/str_replace.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// Maximum size for the receiver model prefix at start of MDNS service instance
// names. Any model names that are larger than this size will be truncated.
const size_t kMaxReceiverModelSize = 20;

// Build the MDNS instance name for service. This will be the receiver model (up
// to 20 bytes) appended with the virtual receiver ID (receiver UUID) and
// optionally appended with extension at the end to resolve name conflicts. The
// total MDNS service instance name is kept below 64 bytes so it can easily fit
// into a single domain name label.
//
// NOTE: This value is based on what is currently done by Eureka, not what is
// called out in the CastV2 spec. Eureka uses |model|-|uuid|, so the same
// convention will be followed here. That being said, the Eureka receiver does
// not use the instance ID in any way, so the specific calculation used should
// not be important.
std::string CalculateInstanceId(const ReceiverInfo& info) {
  // First set the receiver model, truncated to 20 bytes at most. Replace any
  // whitespace characters (" ") with hyphens ("-") in the receiver model before
  // truncation.
  std::string instance_name =
      absl::StrReplaceAll(info.model_name, {{" ", "-"}});
  instance_name = std::string(instance_name, 0, kMaxReceiverModelSize);

  // Append the receiver ID to the instance name separated by a single
  // '-' character if not empty. Strip all hyphens from the receiver ID prior
  // to appending it.
  std::string receiver_id = absl::StrReplaceAll(info.unique_id, {{"-", ""}});

  if (!instance_name.empty()) {
    instance_name.push_back('-');
  }
  instance_name.append(receiver_id);

  return std::string(instance_name, 0, discovery::kMaxLabelLength);
}

// Returns the value for the provided |key| in the |txt| record if it exists;
// otherwise, returns an empty string.
std::string GetStringFromRecord(const discovery::DnsSdTxtRecord& txt,
                                const std::string& key) {
  std::string result;
  const ErrorOr<discovery::DnsSdTxtRecord::ValueRef> value = txt.GetValue(key);
  if (value.is_value()) {
    const std::vector<uint8_t>& txt_value = value.value().get();
    result.assign(txt_value.begin(), txt_value.end());
  }
  return result;
}

}  // namespace

const std::string& ReceiverInfo::GetInstanceId() const {
  if (instance_id_ == std::string("")) {
    instance_id_ = CalculateInstanceId(*this);
  }

  return instance_id_;
}

bool ReceiverInfo::IsValid() const {
  return (
      discovery::IsInstanceValid(GetInstanceId()) && port != 0 &&
      !unique_id.empty() &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(kUniqueIdKey, unique_id) &&
      protocol_version >= 2 &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(
          kVersionKey, std::to_string(static_cast<int>(protocol_version))) &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(
          kCapabilitiesKey, std::to_string(capabilities)) &&
      (status == ReceiverStatus::kIdle || status == ReceiverStatus::kBusy) &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(
          kStatusKey, std::to_string(static_cast<int>(status))) &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(kModelNameKey, model_name) &&
      !friendly_name.empty() &&
      discovery::DnsSdTxtRecord::IsValidTxtValue(kFriendlyNameKey,
                                                 friendly_name));
}

discovery::DnsSdInstance ReceiverInfoToDnsSdInstance(const ReceiverInfo& info) {
  OSP_DCHECK(discovery::IsServiceValid(kCastV2ServiceId));
  OSP_DCHECK(discovery::IsDomainValid(kCastV2DomainId));

  OSP_DCHECK(info.IsValid());

  discovery::DnsSdTxtRecord txt;
  const bool did_set_everything =
      txt.SetValue(kUniqueIdKey, info.unique_id).ok() &&
      txt.SetValue(kVersionKey,
                   std::to_string(static_cast<int>(info.protocol_version)))
          .ok() &&
      txt.SetValue(kCapabilitiesKey, std::to_string(info.capabilities)).ok() &&
      txt.SetValue(kStatusKey, std::to_string(static_cast<int>(info.status)))
          .ok() &&
      txt.SetValue(kModelNameKey, info.model_name).ok() &&
      txt.SetValue(kFriendlyNameKey, info.friendly_name).ok();
  OSP_DCHECK(did_set_everything);

  return discovery::DnsSdInstance(info.GetInstanceId(), kCastV2ServiceId,
                                  kCastV2DomainId, std::move(txt), info.port);
}

ErrorOr<ReceiverInfo> DnsSdInstanceEndpointToReceiverInfo(
    const discovery::DnsSdInstanceEndpoint& endpoint) {
  if (endpoint.service_id() != kCastV2ServiceId) {
    return {Error::Code::kParameterInvalid, "Not a Cast receiver."};
  }

  ReceiverInfo record;
  for (const IPAddress& address : endpoint.addresses()) {
    if (!record.v4_address && address.IsV4()) {
      record.v4_address = address;
    } else if (!record.v6_address && address.IsV6()) {
      record.v6_address = address;
    }
  }
  if (!record.v4_address && !record.v6_address) {
    return {Error::Code::kParameterInvalid,
            "No IPv4 nor IPv6 address in record."};
  }
  record.port = endpoint.port();
  if (record.port == 0) {
    return {Error::Code::kParameterInvalid, "Invalid TCP port in record."};
  }

  // 128-bit integer in hexadecimal format.
  record.unique_id = GetStringFromRecord(endpoint.txt(), kUniqueIdKey);
  if (record.unique_id.empty()) {
    return {Error::Code::kParameterInvalid,
            "Missing receiver unique ID in record."};
  }

  // Cast protocol version supported. Begins at 2 and is incremented by 1 with
  // each version.
  std::string a_decimal_number =
      GetStringFromRecord(endpoint.txt(), kVersionKey);
  if (a_decimal_number.empty()) {
    return {Error::Code::kParameterInvalid,
            "Missing Cast protocol version in record."};
  }
  constexpr int kMinVersion = 2;   // According to spec.
  constexpr int kMaxVersion = 99;  // Implied by spec (field is max of 2 bytes).
  int version;
  if (!absl::SimpleAtoi(a_decimal_number, &version) || version < kMinVersion ||
      version > kMaxVersion) {
    return {Error::Code::kParameterInvalid,
            "Invalid Cast protocol version in record."};
  }
  record.protocol_version = static_cast<uint8_t>(version);

  // A bitset of receiver capabilities.
  a_decimal_number = GetStringFromRecord(endpoint.txt(), kCapabilitiesKey);
  if (a_decimal_number.empty()) {
    return {Error::Code::kParameterInvalid,
            "Missing receiver capabilities in record."};
  }
  if (!absl::SimpleAtoi(a_decimal_number, &record.capabilities)) {
    return {Error::Code::kParameterInvalid,
            "Invalid receiver capabilities field in record."};
  }

  // Receiver status flag.
  a_decimal_number = GetStringFromRecord(endpoint.txt(), kStatusKey);
  if (a_decimal_number == "0") {
    record.status = ReceiverStatus::kIdle;
  } else if (a_decimal_number == "1") {
    record.status = ReceiverStatus::kBusy;
  } else {
    return {Error::Code::kParameterInvalid,
            "Missing/Invalid receiver status flag in record."};
  }

  // [Optional] Receiver model name.
  record.model_name = GetStringFromRecord(endpoint.txt(), kModelNameKey);

  // The friendly name of the receiver.
  record.friendly_name = GetStringFromRecord(endpoint.txt(), kFriendlyNameKey);
  if (record.friendly_name.empty()) {
    return {Error::Code::kParameterInvalid,
            "Missing receiver friendly name in record."};
  }

  return record;
}

}  // namespace cast
}  // namespace openscreen
