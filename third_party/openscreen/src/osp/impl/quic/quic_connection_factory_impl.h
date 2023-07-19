// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_IMPL_H_
#define OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_IMPL_H_

#include <map>
#include <memory>
#include <vector>

#include "osp/impl/quic/quic_connection_factory.h"
#include "platform/api/udp_socket.h"
#include "platform/base/ip_address.h"
#include "third_party/chromium_quic/src/base/at_exit.h"
#include "third_party/chromium_quic/src/net/quic/quic_chromium_alarm_factory.h"
#include "third_party/chromium_quic/src/net/third_party/quic/quartc/quartc_factory.h"

namespace openscreen {
namespace osp {

class QuicTaskRunner;

class QuicConnectionFactoryImpl final : public QuicConnectionFactory {
 public:
  explicit QuicConnectionFactoryImpl(TaskRunner* task_runner);
  ~QuicConnectionFactoryImpl() override;

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

  void OnConnectionClosed(QuicConnection* connection);

 private:
  ::base::AtExitManager exit_manager_;
  scoped_refptr<QuicTaskRunner> quic_task_runner_;
  std::unique_ptr<::net::QuicChromiumAlarmFactory> alarm_factory_;
  std::unique_ptr<::quic::QuartcFactory> quartc_factory_;

  ServerDelegate* server_delegate_ = nullptr;

  std::vector<std::unique_ptr<UdpSocket>> sockets_;

  struct OpenConnection {
    QuicConnection* connection;
    UdpSocket* socket;  // References one of the owned |sockets_|.
  };
  std::map<IPEndpoint, OpenConnection> connections_;

  // NOTE: Must be provided in constructor and stored as an instance variable
  // rather than using the static accessor method to allow for UTs to mock this
  // layer.
  TaskRunner* const task_runner_;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_IMPL_H_
