// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_TEST_MOCK_UDP_SOCKET_H_
#define PLATFORM_TEST_MOCK_UDP_SOCKET_H_

#include <algorithm>
#include <queue>

#include "gmock/gmock.h"
#include "platform/api/udp_socket.h"

namespace openscreen {

class MockUdpSocket : public UdpSocket {
 public:
  MockUdpSocket() = default;
  ~MockUdpSocket() override = default;

  MOCK_CONST_METHOD0(IsIPv4, bool());
  MOCK_CONST_METHOD0(IsIPv6, bool());
  MOCK_CONST_METHOD0(GetLocalEndpoint, IPEndpoint());
  MOCK_METHOD0(Bind, void());
  MOCK_METHOD1(SetMulticastOutboundInterface, void(NetworkInterfaceIndex));
  MOCK_METHOD2(JoinMulticastGroup,
               void(const IPAddress&, NetworkInterfaceIndex));
  MOCK_METHOD3(SendMessage, void(const void*, size_t, const IPEndpoint&));
  MOCK_METHOD1(SetDscp, void(UdpSocket::DscpMode));
};

}  // namespace openscreen

#endif  // PLATFORM_TEST_MOCK_UDP_SOCKET_H_
