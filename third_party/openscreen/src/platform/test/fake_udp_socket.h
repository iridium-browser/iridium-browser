// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_FAKE_UDP_SOCKET_H_
#define PLATFORM_TEST_FAKE_UDP_SOCKET_H_

#include <algorithm>
#include <queue>

#include "gmock/gmock.h"
#include "platform/api/time.h"
#include "platform/api/udp_socket.h"
#include "platform/test/fake_clock.h"
#include "util/osp_logging.h"

namespace openscreen {

class FakeUdpSocket : public UdpSocket {
 public:
  class MockClient : public UdpSocket::Client {
   public:
    MOCK_METHOD1(OnBound, void(UdpSocket*));
    MOCK_METHOD2(OnError, void(UdpSocket*, Error));
    MOCK_METHOD2(OnSendError, void(UdpSocket*, Error));
    MOCK_METHOD2(OnReadInternal, void(UdpSocket*, const ErrorOr<UdpPacket>&));

    void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) override {
      OnReadInternal(socket, packet);
    }
  };

  explicit FakeUdpSocket(Client* client = nullptr,
                         Version version = Version::kV4);
  ~FakeUdpSocket() override;

  bool IsIPv4() const override;
  bool IsIPv6() const override;
  IPEndpoint GetLocalEndpoint() const override;

  // UdpSocket overrides
  void Bind() override;
  void SendMessage(const void* data,
                   size_t length,
                   const IPEndpoint& dest) override;
  void SetMulticastOutboundInterface(NetworkInterfaceIndex interface) override;
  void JoinMulticastGroup(const IPAddress& address,
                          NetworkInterfaceIndex interface) override;
  void SetDscp(DscpMode mode) override;

  // Operatons to queue errors to be returned by the above functions
  void EnqueueBindResult(Error error) { bind_errors_.push(error); }
  void EnqueueSendResult(Error error) { send_errors_.push(error); }
  void EnqueueSetMulticastOutboundInterfaceResult(Error error) {
    set_multicast_outbound_interface_errors_.push(error);
  }
  void EnqueueJoinMulticastGroupResult(Error error) {
    join_multicast_group_errors_.push(error);
  }
  void EnqueueSetDscpResult(Error error) { set_dscp_errors_.push(error); }

  // Accessors for the size of the internal error queues.
  size_t bind_queue_size() { return bind_errors_.size(); }
  size_t send_queue_size() { return send_errors_.size(); }
  size_t set_multicast_outbound_interface_queue_size() {
    return set_multicast_outbound_interface_errors_.size();
  }
  size_t join_multicast_group_queue_size() {
    return join_multicast_group_errors_.size();
  }
  size_t set_dscp_queue_size() { return set_dscp_errors_.size(); }

  void MockReceivePacket(UdpPacket packet);

 private:
  void ProcessConfigurationMethod(std::queue<Error>* errors);

  Client* const client_;
  Version version_;

  // Queues for the response to calls above
  std::queue<Error> bind_errors_;
  std::queue<Error> send_errors_;
  std::queue<Error> set_multicast_outbound_interface_errors_;
  std::queue<Error> join_multicast_group_errors_;
  std::queue<Error> set_dscp_errors_;
};

}  // namespace openscreen

#endif  // PLATFORM_TEST_FAKE_UDP_SOCKET_H_
