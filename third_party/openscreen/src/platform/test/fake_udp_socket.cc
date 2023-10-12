// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/test/fake_udp_socket.h"

#include <utility>

namespace openscreen {

FakeUdpSocket::FakeUdpSocket(Client* client, Version version)
    : client_(client), version_(version) {}

FakeUdpSocket::~FakeUdpSocket() = default;

bool FakeUdpSocket::IsIPv4() const {
  return version_ == UdpSocket::Version::kV4;
}

bool FakeUdpSocket::IsIPv6() const {
  return version_ == UdpSocket::Version::kV6;
}

IPEndpoint FakeUdpSocket::GetLocalEndpoint() const {
  return IPEndpoint{};
}

void FakeUdpSocket::Bind() {
  OSP_CHECK(bind_errors_.size()) << "No Bind responses queued.";
  ProcessConfigurationMethod(&bind_errors_);
}

void FakeUdpSocket::SetMulticastOutboundInterface(
    NetworkInterfaceIndex interface) {
  OSP_CHECK(set_multicast_outbound_interface_errors_.size())
      << "No SetMulticastOutboundInterface responses queued.";
  ProcessConfigurationMethod(&set_multicast_outbound_interface_errors_);
}

void FakeUdpSocket::JoinMulticastGroup(const IPAddress& address,
                                       NetworkInterfaceIndex interface) {
  OSP_CHECK(join_multicast_group_errors_.size())
      << "No JoinMulticastGroup responses queued.";
  ProcessConfigurationMethod(&join_multicast_group_errors_);
}

void FakeUdpSocket::SetDscp(DscpMode mode) {
  OSP_CHECK(set_dscp_errors_.size()) << "No SetDscp responses queued.";
  ProcessConfigurationMethod(&set_dscp_errors_);
}

void FakeUdpSocket::MockReceivePacket(UdpPacket packet) {
  if (client_) {
    client_->OnRead(this, std::move(packet));
  }
}

void FakeUdpSocket::ProcessConfigurationMethod(std::queue<Error>* errors) {
  Error error = errors->front();
  errors->pop();

  if (!error.ok() && client_) {
    client_->OnError(this, std::move(error));
  }
}

void FakeUdpSocket::SendMessage(const void* data,
                                size_t length,
                                const IPEndpoint& dest) {
  OSP_CHECK(send_errors_.size()) << "No SendMessage responses queued.";
  Error error = send_errors_.front();
  send_errors_.pop();

  if (!error.ok() && client_) {
    client_->OnSendError(this, std::move(error));
  }
}

}  // namespace openscreen
