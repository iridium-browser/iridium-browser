// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_CAST_SERVICE_H_
#define CAST_STANDALONE_RECEIVER_CAST_SERVICE_H_

#include <memory>
#include <string>

#include "cast/common/public/receiver_info.h"
#include "cast/receiver/application_agent.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/public/receiver_socket_factory.h"
#include "cast/standalone_receiver/mirroring_application.h"
#include "discovery/common/reporting_client.h"
#include "discovery/public/dns_sd_service_factory.h"
#include "discovery/public/dns_sd_service_publisher.h"
#include "platform/api/serial_delete_ptr.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"

namespace openscreen {

struct InterfaceInfo;
class TaskRunner;
class TlsConnectionFactory;

namespace cast {

// Assembles all the necessary components and manages their lifetimes, to create
// a full Cast Receiver on the network, with the following overall
// functionality:
//
//   * Listens for TCP connections on port 8010.
//   * Establishes TLS tunneling over those connections.
//   * Wraps a CastSocket API around the TLS connections.
//   * Manages available receiver-side applications.
//   * Provides a Cast V2 Mirroring application (media streaming playback in an
//     on-screen window).
//   * Publishes over mDNS to be discoverable to all senders on the same LAN.
class CastService final : public discovery::ReportingClient {
 public:
  struct Configuration {
    // The task runner to be used for async calls.
    TaskRunner* task_runner;

    // The interface the cast service is running on.
    InterfaceInfo interface;

    // The credentials that the cast service should use for TLS.
    GeneratedCredentials credentials;

    // The friendly name to be used for broadcasting.
    std::string friendly_name;

    // The model name to be used for broadcasting.
    std::string model_name;

    // Whether we should broadcast over mDNS/DNS-SD.
    bool enable_discovery = true;
  };

  explicit CastService(Configuration config);
  ~CastService() final;

 private:
  using LazyDeletedDiscoveryService = SerialDeletePtr<discovery::DnsSdService>;
  using LazyDeletedDiscoveryPublisher =
      SerialDeletePtr<discovery::DnsSdServicePublisher<ReceiverInfo>>;

  // discovery::ReportingClient overrides.
  void OnFatalError(Error error) final;
  void OnRecoverableError(Error error) final;

  const IPEndpoint local_endpoint_;
  const GeneratedCredentials credentials_;

  ApplicationAgent agent_;
  MirroringApplication mirroring_application_;
  ReceiverSocketFactory socket_factory_;
  std::unique_ptr<TlsConnectionFactory> connection_factory_;

  LazyDeletedDiscoveryService discovery_service_;
  LazyDeletedDiscoveryPublisher discovery_publisher_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_CAST_SERVICE_H_
