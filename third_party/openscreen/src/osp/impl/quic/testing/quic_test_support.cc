// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/testing/quic_test_support.h"

#include <memory>

#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_server.h"
#include "osp/public/network_service_manager.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {
namespace osp {

FakeQuicBridge::FakeQuicBridge(FakeTaskRunner* task_runner,
                               ClockNowFunctionPtr now_function)
    : task_runner_(task_runner) {
  fake_bridge =
      std::make_unique<FakeQuicConnectionFactoryBridge>(kControllerEndpoint);

  controller_demuxer = std::make_unique<MessageDemuxer>(
      now_function, MessageDemuxer::kDefaultBufferLimit);
  receiver_demuxer = std::make_unique<MessageDemuxer>(
      now_function, MessageDemuxer::kDefaultBufferLimit);

  auto fake_client_factory =
      std::make_unique<FakeClientQuicConnectionFactory>(fake_bridge.get());
  client_socket_ =
      std::make_unique<FakeUdpSocket>(task_runner_, fake_client_factory.get());

  quic_client = std::make_unique<QuicClient>(
      controller_demuxer.get(), std::move(fake_client_factory),
      &mock_client_observer, now_function, task_runner);

  auto fake_server_factory =
      std::make_unique<FakeServerQuicConnectionFactory>(fake_bridge.get());
  server_socket_ =
      std::make_unique<FakeUdpSocket>(task_runner_, fake_server_factory.get());
  ServerConfig config;
  config.connection_endpoints.push_back(kReceiverEndpoint);
  quic_server = std::make_unique<QuicServer>(
      config, receiver_demuxer.get(), std::move(fake_server_factory),
      &mock_server_observer, now_function, task_runner);

  quic_client->Start();
  quic_server->Start();
}

FakeQuicBridge::~FakeQuicBridge() = default;

void FakeQuicBridge::PostClientPacket() {
  UdpPacket packet;
  packet.set_socket(client_socket_.get());
  client_socket_->MockReceivePacket(std::move(packet));
}

void FakeQuicBridge::PostServerPacket() {
  UdpPacket packet;
  packet.set_socket(server_socket_.get());
  server_socket_->MockReceivePacket(std::move(packet));
}

void FakeQuicBridge::PostPacketsUntilIdle() {
  bool client_idle = fake_bridge->client_idle();
  bool server_idle = fake_bridge->server_idle();
  if (!client_idle || !server_idle) {
    PostClientPacket();
    PostServerPacket();
    task_runner_->PostTask([this]() { this->PostPacketsUntilIdle(); });
  }
}

void FakeQuicBridge::RunTasksUntilIdle() {
  PostClientPacket();
  PostServerPacket();
  task_runner_->PostTask(
      std::bind(&FakeQuicBridge::PostPacketsUntilIdle, this));
  task_runner_->RunTasksUntilIdle();
}

}  // namespace osp
}  // namespace openscreen
