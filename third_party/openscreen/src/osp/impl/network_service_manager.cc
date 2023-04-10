// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/network_service_manager.h"

namespace {

openscreen::osp::NetworkServiceManager* g_network_service_manager_instance =
    nullptr;

}  //  namespace

namespace openscreen {
namespace osp {

// static
NetworkServiceManager* NetworkServiceManager::Create(
    std::unique_ptr<ServiceListener> mdns_listener,
    std::unique_ptr<ServicePublisher> service_publisher,
    std::unique_ptr<ProtocolConnectionClient> connection_client,
    std::unique_ptr<ProtocolConnectionServer> connection_server) {
  // TODO(mfoltz): Convert to assertion failure
  if (g_network_service_manager_instance)
    return nullptr;
  g_network_service_manager_instance = new NetworkServiceManager(
      std::move(mdns_listener), std::move(service_publisher),
      std::move(connection_client), std::move(connection_server));
  return g_network_service_manager_instance;
}

// static
NetworkServiceManager* NetworkServiceManager::Get() {
  // TODO(mfoltz): Convert to assertion failure
  if (!g_network_service_manager_instance)
    return nullptr;
  return g_network_service_manager_instance;
}

// static
void NetworkServiceManager::Dispose() {
  // TODO(mfoltz): Convert to assertion failure
  if (!g_network_service_manager_instance)
    return;
  delete g_network_service_manager_instance;
  g_network_service_manager_instance = nullptr;
}

ServiceListener* NetworkServiceManager::GetMdnsServiceListener() {
  return mdns_listener_.get();
}

ServicePublisher* NetworkServiceManager::GetServicePublisher() {
  return service_publisher_.get();
}

ProtocolConnectionClient* NetworkServiceManager::GetProtocolConnectionClient() {
  return connection_client_.get();
}

ProtocolConnectionServer* NetworkServiceManager::GetProtocolConnectionServer() {
  return connection_server_.get();
}

NetworkServiceManager::NetworkServiceManager(
    std::unique_ptr<ServiceListener> mdns_listener,
    std::unique_ptr<ServicePublisher> service_publisher,
    std::unique_ptr<ProtocolConnectionClient> connection_client,
    std::unique_ptr<ProtocolConnectionServer> connection_server)
    : mdns_listener_(std::move(mdns_listener)),
      service_publisher_(std::move(service_publisher)),
      connection_client_(std::move(connection_client)),
      connection_server_(std::move(connection_server)) {}

NetworkServiceManager::~NetworkServiceManager() = default;

}  // namespace osp
}  // namespace openscreen
