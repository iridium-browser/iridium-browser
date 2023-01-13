// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"

namespace openscreen {

using testing::_;

// Tests that a UdpSocket that does not specify any address or port will
// successfully Bind(), and that the operating system will return the
// auto-assigned socket name (i.e., the local endpoint's port will not be zero).
TEST(SocketIntegrationTest, ResolvesLocalEndpoint_IPv4) {
  const uint8_t kIpV4AddrAny[4] = {};
  FakeClock clock(Clock::now());
  FakeTaskRunner task_runner(&clock);
  testing::StrictMock<FakeUdpSocket::MockClient> client;
  ErrorOr<std::unique_ptr<UdpSocket>> create_result = UdpSocket::Create(
      &task_runner, &client, IPEndpoint{IPAddress(kIpV4AddrAny), 0});
  ASSERT_TRUE(create_result) << create_result.error();
  const auto socket = std::move(create_result.value());
  EXPECT_CALL(client, OnBound(_)).Times(1);
  socket->Bind();
  const IPEndpoint local_endpoint = socket->GetLocalEndpoint();
  EXPECT_NE(local_endpoint.port, 0) << local_endpoint;
}

// Tests that a UdpSocket that does not specify any address or port will
// successfully Bind(), and that the operating system will return the
// auto-assigned socket name (i.e., the local endpoint's port will not be zero).
TEST(SocketIntegrationTest, ResolvesLocalEndpoint_IPv6) {
  const uint16_t kIpV6AddrAny[8] = {};
  FakeClock clock(Clock::now());
  FakeTaskRunner task_runner(&clock);
  testing::StrictMock<FakeUdpSocket::MockClient> client;
  ErrorOr<std::unique_ptr<UdpSocket>> create_result = UdpSocket::Create(
      &task_runner, &client, IPEndpoint{IPAddress(kIpV6AddrAny), 0});
  ASSERT_TRUE(create_result) << create_result.error();
  const auto socket = std::move(create_result.value());
  EXPECT_CALL(client, OnBound(_)).Times(1);
  socket->Bind();
  const IPEndpoint local_endpoint = socket->GetLocalEndpoint();
  EXPECT_NE(local_endpoint.port, 0) << local_endpoint;
}

}  // namespace openscreen
