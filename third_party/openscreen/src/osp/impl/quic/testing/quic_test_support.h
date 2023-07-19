// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_
#define OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_

#include <memory>

#include "gmock/gmock.h"
#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_server.h"
#include "osp/impl/quic/testing/fake_quic_connection_factory.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_metrics.h"
#include "osp/public/protocol_connection_client.h"
#include "osp/public/protocol_connection_server.h"
#include "platform/api/time.h"
#include "platform/api/udp_socket.h"
#include "platform/base/ip_address.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"

namespace openscreen {
namespace osp {

class MockServiceObserver : public ProtocolConnectionServiceObserver {
 public:
  ~MockServiceObserver() override = default;

  MOCK_METHOD0(OnRunning, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD1(OnMetrics, void(const NetworkMetrics& metrics));
  MOCK_METHOD1(OnError, void(const Error& error));
};

class MockServerObserver : public ProtocolConnectionServer::Observer {
 public:
  ~MockServerObserver() override = default;

  MOCK_METHOD0(OnRunning, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD1(OnMetrics, void(const NetworkMetrics& metrics));
  MOCK_METHOD1(OnError, void(const Error& error));

  MOCK_METHOD0(OnSuspended, void());

  void OnIncomingConnection(
      std::unique_ptr<ProtocolConnection> connection) override {
    OnIncomingConnectionMock(connection);
  }
  MOCK_METHOD1(OnIncomingConnectionMock,
               void(std::unique_ptr<ProtocolConnection>& connection));
};

class FakeQuicBridge {
 public:
  FakeQuicBridge(FakeTaskRunner* task_runner, ClockNowFunctionPtr now_function);
  ~FakeQuicBridge();

  const IPEndpoint kControllerEndpoint{{192, 168, 1, 3}, 4321};
  const IPEndpoint kReceiverEndpoint{{192, 168, 1, 17}, 1234};

  std::unique_ptr<MessageDemuxer> controller_demuxer;
  std::unique_ptr<MessageDemuxer> receiver_demuxer;
  std::unique_ptr<QuicClient> quic_client;
  std::unique_ptr<QuicServer> quic_server;
  std::unique_ptr<FakeQuicConnectionFactoryBridge> fake_bridge;
  ::testing::NiceMock<MockServiceObserver> mock_client_observer;
  ::testing::NiceMock<MockServerObserver> mock_server_observer;

  void RunTasksUntilIdle();

 private:
  void PostClientPacket();
  void PostServerPacket();
  void PostPacketsUntilIdle();
  FakeClientQuicConnectionFactory* GetClientFactory();
  FakeServerQuicConnectionFactory* GetServerFactory();
  FakeTaskRunner* task_runner_;

  std::unique_ptr<FakeUdpSocket> client_socket_;
  std::unique_ptr<FakeUdpSocket> server_socket_;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_TESTING_QUIC_TEST_SUPPORT_H_
