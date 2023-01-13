// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/cast_service.h"

#include <stdint.h>

#include <array>
#include <utility>

#include "discovery/common/config.h"
#include "platform/api/tls_connection_factory.h"
#include "platform/base/interface_info.h"
#include "platform/base/tls_listen_options.h"
#include "util/crypto/random_bytes.h"
#include "util/osp_logging.h"
#include "util/stringprintf.h"

namespace openscreen {
namespace cast {

namespace {

constexpr uint16_t kDefaultCastServicePort = 8010;
constexpr int kCastUniqueIdLength = 6;

constexpr int kDefaultMaxBacklogSize = 64;
const TlsListenOptions kDefaultListenOptions{kDefaultMaxBacklogSize};

IPEndpoint DetermineEndpoint(const InterfaceInfo& interface) {
  const IPAddress address = interface.GetIpAddressV4()
                                ? interface.GetIpAddressV4()
                                : interface.GetIpAddressV6();
  OSP_CHECK(address);
  return IPEndpoint{address, kDefaultCastServicePort};
}

discovery::Config MakeDiscoveryConfig(const InterfaceInfo& interface) {
  return discovery::Config{.network_info = {interface}};
}

}  // namespace

CastService::CastService(CastService::Configuration config)
    : local_endpoint_(DetermineEndpoint(config.interface)),
      credentials_(std::move(config.credentials)),
      agent_(config.task_runner, credentials_.provider.get()),
      mirroring_application_(config.task_runner,
                             local_endpoint_.address,
                             &agent_),
      socket_factory_(&agent_, agent_.cast_socket_client()),
      connection_factory_(
          TlsConnectionFactory::CreateFactory(&socket_factory_,
                                              config.task_runner)),
      discovery_service_(config.enable_discovery
                             ? discovery::CreateDnsSdService(
                                   config.task_runner,
                                   this,
                                   MakeDiscoveryConfig(config.interface))
                             : LazyDeletedDiscoveryService()),
      discovery_publisher_(
          discovery_service_
              ? MakeSerialDelete<
                    discovery::DnsSdServicePublisher<ReceiverInfo>>(
                    config.task_runner,
                    discovery_service_.get(),
                    kCastV2ServiceId,
                    ReceiverInfoToDnsSdInstance)
              : LazyDeletedDiscoveryPublisher()) {
  connection_factory_->SetListenCredentials(credentials_.tls_credentials);
  connection_factory_->Listen(local_endpoint_, kDefaultListenOptions);

  if (discovery_publisher_) {
    ReceiverInfo info;
    info.port = local_endpoint_.port;
    if (config.interface.HasHardwareAddress()) {
      info.unique_id = HexEncode(config.interface.hardware_address.data(),
                                 config.interface.hardware_address.size());
    } else {
      OSP_LOG_WARN << "Hardware address for interface " << config.interface.name
                   << " is empty. Generating a random unique_id.";
      std::array<uint8_t, kCastUniqueIdLength> random_bytes;
      GenerateRandomBytes(random_bytes.data(), kCastUniqueIdLength);
      info.unique_id = HexEncode(random_bytes.data(), kCastUniqueIdLength);
    }
    info.friendly_name = config.friendly_name;
    info.model_name = config.model_name;
    info.capabilities = kHasVideoOutput | kHasAudioOutput;
    const Error error = discovery_publisher_->Register(info);
    if (!error.ok()) {
      OnFatalError(std::move(error));
    }
  }
}

CastService::~CastService() {
  if (discovery_publisher_) {
    discovery_publisher_->DeregisterAll();
  }
}

void CastService::OnFatalError(Error error) {
  OSP_LOG_FATAL << "Encountered fatal discovery error: " << error;
}

void CastService::OnRecoverableError(Error error) {
  OSP_LOG_ERROR << "Encountered recoverable discovery error: " << error;
}

}  // namespace cast
}  // namespace openscreen
