// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/message_util.h"

#include <sstream>
#include <utility>

#include "cast/common/channel/virtual_connection.h"
#include "util/json/json_serialization.h"
#include "util/json/json_value.h"
#include "util/osp_logging.h"

#if defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
#endif

namespace openscreen {
namespace cast {
namespace {

using ::cast::channel::CastMessage;

// The value used for "sdkType" in a virtual CONNECT request. Historically, this
// value was used in Chrome's C++ impl even though "2" refers to the Media
// Router Extension.
constexpr int kVirtualConnectSdkType = 2;

// The value used for "connectionType" in a virtual CONNECT request. This value
// stands for CONNECTION_TYPE_LOCAL.
constexpr int kVirtualConnectTypeLocal = 1;

// The value to be set as the "platform" value in a virtual CONNECT request.
// Source (in Chromium source tree):
// src/third_party/metrics_proto/cast_logs.proto
enum VirtualConnectPlatformValue {
  kOtherPlatform = 0,
  kAndroid = 1,
  kIOS = 2,
  kWindows = 3,
  kMacOSX = 4,
  kChromeOS = 5,
  kLinux = 6,
  kCastDevice = 7,
};

#if defined(__APPLE__) || defined(__MACH__)
constexpr VirtualConnectPlatformValue GetVirtualConnectPlatformMacFlavor() {
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
  return kIOS;
#else
  return kMacOSX;
#endif
}
#endif

constexpr VirtualConnectPlatformValue GetVirtualConnectPlatform() {
  // Based on //build/build_config.h in the Chromium project. The order of these
  // matters!
#if defined(__ANDROID__)
  return kAndroid;
#elif defined(__APPLE__) || defined(__MACH__)
  return GetVirtualConnectPlatformMacFlavor();
#elif defined(_WIN32) || defined(_WIN64)
  return kWindows;
#elif defined(OS_CHROMEOS)
  // Note: OS_CHROMEOS is defined via the compiler's command line in Chromium
  // embedder builds by Chromium's //build/config/linux:runtime_library config.
  return kChromeOS;
#elif defined(__linux__)
  return kLinux;
#else
  return kOtherPlatform;
#endif
}

CastMessage MakeConnectionMessage(const std::string& source_id,
                                  const std::string& destination_id) {
  CastMessage connect_message;
  connect_message.set_protocol_version(kDefaultOutgoingMessageVersion);
  connect_message.set_source_id(source_id);
  connect_message.set_destination_id(destination_id);
  connect_message.set_namespace_(kConnectionNamespace);
  return connect_message;
}

}  // namespace

std::string ToString(AppAvailabilityResult availability) {
  switch (availability) {
    case AppAvailabilityResult::kAvailable:
      return "Available";
    case AppAvailabilityResult::kUnavailable:
      return "Unavailable";
    case AppAvailabilityResult::kUnknown:
      return "Unknown";
    default:
      OSP_NOTREACHED();
  }
}

CastMessage MakeSimpleUTF8Message(const std::string& namespace_,
                                  std::string payload) {
  CastMessage message;
  message.set_protocol_version(kDefaultOutgoingMessageVersion);
  message.set_namespace_(namespace_);
  message.set_payload_type(::cast::channel::CastMessage_PayloadType_STRING);
  message.set_payload_utf8(std::move(payload));
  return message;
}

CastMessage MakeConnectMessage(const std::string& source_id,
                               const std::string& destination_id) {
  CastMessage connect_message =
      MakeConnectionMessage(source_id, destination_id);
  connect_message.set_payload_type(
      ::cast::channel::CastMessage_PayloadType_STRING);

  // Historically, the CONNECT message was meant to come from a Chrome browser.
  // However, this library could be embedded in any app. So, properties like
  // user agent, application version, etc. are not known here.
  static constexpr char kUnknownVersion[] = "Unknown (Open Screen)";

  Json::Value message(Json::objectValue);
  message[kMessageKeyType] = CastMessageTypeToString(CastMessageType::kConnect);
  for (int i = 0; i <= 3; ++i) {
    message[kMessageKeyProtocolVersionList][i] =
        ::cast::channel::CastMessage_ProtocolVersion_CASTV2_1_0 + i;
  }
  message[kMessageKeyUserAgent] = kUnknownVersion;
  message[kMessageKeyConnType] =
      static_cast<int>(VirtualConnection::Type::kStrong);
  message[kMessageKeyOrigin] = Json::Value(Json::objectValue);

  Json::Value sender_info(Json::objectValue);
  sender_info[kMessageKeySdkType] = kVirtualConnectSdkType;
  sender_info[kMessageKeyVersion] = kUnknownVersion;
  sender_info[kMessageKeyBrowserVersion] = kUnknownVersion;
  sender_info[kMessageKeyPlatform] = GetVirtualConnectPlatform();
  sender_info[kMessageKeyConnectionType] = kVirtualConnectTypeLocal;
  message[kMessageKeySenderInfo] = std::move(sender_info);

  connect_message.set_payload_utf8(json::Stringify(std::move(message)).value());
  return connect_message;
}

CastMessage MakeCloseMessage(const std::string& source_id,
                             const std::string& destination_id) {
  CastMessage close_message = MakeConnectionMessage(source_id, destination_id);
  close_message.set_payload_type(
      ::cast::channel::CastMessage_PayloadType_STRING);
  close_message.set_payload_utf8(R"!({"type": "CLOSE"})!");
  return close_message;
}

std::string MakeUniqueSessionId(const char* prefix) {
  static int next_id = 10000;

  std::ostringstream oss;
  oss << prefix << '-' << (next_id++);
  return oss.str();
}

bool HasType(const Json::Value& object, CastMessageType type) {
  OSP_DCHECK(object.isObject());
  const Json::Value& value =
      object.get(kMessageKeyType, Json::Value::nullSingleton());
  return value.isString() && value.asString() == CastMessageTypeToString(type);
}

}  // namespace cast
}  // namespace openscreen
