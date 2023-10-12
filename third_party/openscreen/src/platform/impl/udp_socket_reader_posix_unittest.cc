// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/udp_socket_reader_posix.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/api/time.h"
#include "platform/impl/socket_handle_posix.h"
#include "platform/impl/udp_socket_posix.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"

namespace openscreen {
namespace {

class MockUdpSocketPosix : public UdpSocketPosix {
 public:
  explicit MockUdpSocketPosix(TaskRunner& task_runner,
                              Client* client,
                              int fd,
                              Version version = Version::kV4)
      : UdpSocketPosix(task_runner, client, SocketHandle(fd), IPEndpoint()),
        version_(version) {}
  ~MockUdpSocketPosix() override = default;

  bool IsIPv4() const override { return version_ == UdpSocket::Version::kV4; }

  bool IsIPv6() const override { return version_ == UdpSocket::Version::kV6; }

  MOCK_METHOD0(Bind, void());
  MOCK_METHOD1(SetMulticastOutboundInterface, void(NetworkInterfaceIndex));
  MOCK_METHOD2(JoinMulticastGroup,
               void(const IPAddress&, NetworkInterfaceIndex));
  MOCK_METHOD3(SendMessage, void(const void*, size_t, const IPEndpoint&));
  MOCK_METHOD1(SetDscp, void(DscpMode));

 private:
  Version version_;
};

// Mock event waiter
class MockNetworkWaiter final : public SocketHandleWaiter {
 public:
  using ReadyHandle = SocketHandleWaiter::ReadyHandle;

  MockNetworkWaiter() : SocketHandleWaiter(&FakeClock::now) {}
  ~MockNetworkWaiter() override = default;

  MOCK_METHOD2(
      AwaitSocketsReadable,
      ErrorOr<std::vector<ReadyHandle>>(const std::vector<SocketHandleRef>&,
                                        const Clock::duration&));

  FakeClock fake_clock{Clock::time_point{Clock::duration{1234567}}};
};

// Mock Task Runner
class MockTaskRunner final : public TaskRunner {
 public:
  MockTaskRunner() {
    tasks_posted = 0;
    delayed_tasks_posted = 0;
  }

  void PostPackagedTask(Task t) override {
    tasks_posted++;
    t();
  }

  void PostPackagedTaskWithDelay(Task t, Clock::duration duration) override {
    delayed_tasks_posted++;
    t();
  }

  bool IsRunningOnTaskRunner() override { return true; }

  uint32_t tasks_posted;
  uint32_t delayed_tasks_posted;
};

}  // namespace

// Class extending NetworkWaiter to allow for looking at protected data.
class TestingUdpSocketReader final : public UdpSocketReaderPosix {
 public:
  explicit TestingUdpSocketReader(SocketHandleWaiter* waiter)
      : UdpSocketReaderPosix(waiter) {}

  void OnDestroy(UdpSocket* socket) override {
    OnDelete(static_cast<UdpSocketPosix*>(socket), true);
  }

  bool IsMappedRead(UdpSocketPosix* socket) const {
    return IsMappedReadForTesting(socket);
  }
};

TEST(UdpSocketReaderTest, WatchReadableSucceeds) {
  std::unique_ptr<SocketHandleWaiter> mock_waiter =
      std::unique_ptr<SocketHandleWaiter>(new MockNetworkWaiter());
  auto task_runner = MockTaskRunner();
  FakeUdpSocket::MockClient client;
  std::unique_ptr<MockUdpSocketPosix> socket =
      std::make_unique<MockUdpSocketPosix>(task_runner, &client, 42,
                                           UdpSocket::Version::kV4);
}

TEST(UdpSocketReaderTest, UnwatchReadableSucceeds) {
  std::unique_ptr<SocketHandleWaiter> mock_waiter =
      std::unique_ptr<SocketHandleWaiter>(new MockNetworkWaiter());
  auto task_runner = MockTaskRunner();

  FakeUdpSocket::MockClient client;
  std::unique_ptr<MockUdpSocketPosix> socket =
      std::make_unique<MockUdpSocketPosix>(task_runner, &client, 17,
                                           UdpSocket::Version::kV4);
  TestingUdpSocketReader network_waiter(mock_waiter.get());

  EXPECT_FALSE(network_waiter.IsMappedRead(socket.get()));
  network_waiter.OnDestroy(socket.get());
  EXPECT_FALSE(network_waiter.IsMappedRead(socket.get()));

  network_waiter.OnCreate(socket.get());
  EXPECT_TRUE(network_waiter.IsMappedRead(socket.get()));

  network_waiter.OnDestroy(socket.get());
  EXPECT_FALSE(network_waiter.IsMappedRead(socket.get()));

  network_waiter.OnDestroy(socket.get());
  EXPECT_FALSE(network_waiter.IsMappedRead(socket.get()));
}

}  // namespace openscreen
