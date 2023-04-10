// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_
#define OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_

#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "osp/impl/quic/quic_connection_factory.h"
#include "osp/impl/quic/testing/fake_quic_connection.h"
#include "osp/public/message_demuxer.h"

namespace openscreen {
namespace osp {

class FakeQuicConnectionFactoryBridge {
 public:
  explicit FakeQuicConnectionFactoryBridge(
      const IPEndpoint& controller_endpoint);

  bool server_idle() const { return server_idle_; }
  bool client_idle() const { return client_idle_; }

  void OnConnectionClosed(QuicConnection* connection);
  void OnOutgoingStream(QuicConnection* connection, QuicStream* stream);

  void SetServerDelegate(QuicConnectionFactory::ServerDelegate* delegate,
                         const IPEndpoint& endpoint);
  void RunTasks(bool is_client);
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate);

 private:
  struct ConnectionPair {
    FakeQuicConnection* controller;
    FakeQuicConnection* receiver;
  };

  const IPEndpoint controller_endpoint_;
  IPEndpoint receiver_endpoint_;
  bool client_idle_ = true;
  bool server_idle_ = true;
  uint64_t next_connection_id_ = 0;
  bool connections_pending_ = true;
  ConnectionPair connections_ = {};
  QuicConnectionFactory::ServerDelegate* delegate_ = nullptr;
};

class FakeClientQuicConnectionFactory final : public QuicConnectionFactory {
 public:
  explicit FakeClientQuicConnectionFactory(
      FakeQuicConnectionFactoryBridge* bridge);
  ~FakeClientQuicConnectionFactory() override;

  // UdpSocket::Client overrides.
  void OnError(UdpSocket* socket, Error error) override;
  void OnSendError(UdpSocket* socket, Error error) override;
  void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) override;

  // QuicConnectionFactory overrides.
  void SetServerDelegate(ServerDelegate* delegate,
                         const std::vector<IPEndpoint>& endpoints) override;
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate) override;

  bool idle() const { return idle_; }

  std::unique_ptr<UdpSocket> socket_;

 private:
  FakeQuicConnectionFactoryBridge* bridge_;
  bool idle_ = true;
};

class FakeServerQuicConnectionFactory final : public QuicConnectionFactory {
 public:
  explicit FakeServerQuicConnectionFactory(
      FakeQuicConnectionFactoryBridge* bridge);
  ~FakeServerQuicConnectionFactory() override;

  // UdpSocket::Client overrides.
  void OnError(UdpSocket* socket, Error error) override;
  void OnSendError(UdpSocket* socket, Error error) override;
  void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) override;

  // QuicConnectionFactory overrides.
  void SetServerDelegate(ServerDelegate* delegate,
                         const std::vector<IPEndpoint>& endpoints) override;
  std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate) override;

  bool idle() const { return idle_; }

 private:
  FakeQuicConnectionFactoryBridge* bridge_;
  bool idle_ = true;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_TESTING_FAKE_QUIC_CONNECTION_FACTORY_H_
