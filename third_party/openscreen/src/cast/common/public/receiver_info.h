// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_PUBLIC_RECEIVER_INFO_H_
#define CAST_COMMON_PUBLIC_RECEIVER_INFO_H_

#include <memory>
#include <string>
#include <utility>

#include "discovery/dnssd/public/dns_sd_instance.h"
#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"
#include "platform/base/ip_address.h"

namespace openscreen {
namespace cast {

// Constants to identify a CastV2 instance with DNS-SD.
constexpr char kCastV2ServiceId[] = "_googlecast._tcp";
constexpr char kCastV2DomainId[] = "local";

// Constants to be used as keys when storing data inside of a DNS-SD TXT record.
constexpr char kUniqueIdKey[] = "id";
constexpr char kVersionKey[] = "ve";
constexpr char kCapabilitiesKey[] = "ca";
constexpr char kStatusKey[] = "st";
constexpr char kFriendlyNameKey[] = "fn";
constexpr char kModelNameKey[] = "md";

// This represents the ‘st’ flag in the CastV2 TXT record.
enum ReceiverStatus {
  // The receiver is idle and does not need to be connected now.
  kIdle = 0,

  // The receiver is hosting an activity and invites the sender to join.  The
  // receiver should connect to the running activity using the channel
  // establishment protocol, and then query the activity to determine the next
  // step, such as showing a description of the activity and prompting the user
  // to launch the corresponding app.
  kBusy = 1,
  kJoin = kBusy
};

constexpr uint8_t kCurrentCastVersion = 2;

// Bits in the ‘ca’ bitfield, per the CastV2 spec.
constexpr uint64_t kHasVideoOutput = 1 << 0;
constexpr uint64_t kHasVideoInput = 1 << 1;
constexpr uint64_t kHasAudioOutput = 1 << 2;
constexpr uint64_t kHasAudioIntput = 1 << 3;
constexpr uint64_t kIsDevModeEnabled = 1 << 4;

constexpr uint64_t kNoCapabilities = 0;

// This is the top-level receiver info class for CastV2. It describes a specific
// service instance.
struct ReceiverInfo {
  // returns the instance id associated with this ReceiverInfo instance.
  const std::string& GetInstanceId() const;

  // Returns whether all fields of this ReceiverInfo are valid.
  bool IsValid() const;

  // Addresses for the service. Present if an address of this address type
  // exists and empty otherwise. When publishing a service instance, these
  // values will be overridden based on |network_config| values provided in the
  // discovery::Config object used to initialize discovery.
  IPAddress v4_address;
  IPAddress v6_address;

  // Port at which this service can be reached.
  uint16_t port;

  // A UUID for the Cast receiver. This should be a universally unique
  // identifier for the receiver, and should (but does not have to be) be stable
  // across factory resets.
  std::string unique_id;

  // Cast protocol version supported. Begins at 2 and is incremented by 1 with
  // each version.
  uint8_t protocol_version = kCurrentCastVersion;

  // Bitfield of ReceiverCapabilities supported by this service instance.
  uint64_t capabilities = kNoCapabilities;

  // Status of the service instance.
  ReceiverStatus status = ReceiverStatus::kIdle;

  // The model name of the receiver, e.g. “Eureka v1”, “Mollie”.
  std::string model_name;

  // The friendly name of the receiver, e.g. “Living Room TV".
  std::string friendly_name;

 private:
  mutable std::string instance_id_ = "";
};

inline bool operator==(const ReceiverInfo& lhs, const ReceiverInfo& rhs) {
  return lhs.v4_address == rhs.v4_address && lhs.v6_address == rhs.v6_address &&
         lhs.port == rhs.port && lhs.unique_id == rhs.unique_id &&
         lhs.protocol_version == rhs.protocol_version &&
         lhs.capabilities == rhs.capabilities && lhs.status == rhs.status &&
         lhs.model_name == rhs.model_name &&
         lhs.friendly_name == rhs.friendly_name;
}

inline bool operator!=(const ReceiverInfo& lhs, const ReceiverInfo& rhs) {
  return !(lhs == rhs);
}

// Functions responsible for converting between CastV2 and DNS-SD
// representations of a service instance.
discovery::DnsSdInstance ReceiverInfoToDnsSdInstance(
    const ReceiverInfo& service);

ErrorOr<ReceiverInfo> DnsSdInstanceEndpointToReceiverInfo(
    const discovery::DnsSdInstanceEndpoint& endpoint);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_PUBLIC_RECEIVER_INFO_H_
