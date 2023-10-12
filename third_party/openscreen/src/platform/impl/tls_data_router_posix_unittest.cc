// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/tls_data_router_posix.h"

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/ip_address.h"
#include "platform/impl/stream_socket_posix.h"
#include "platform/impl/tls_connection_posix.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {
namespace {

class MockNetworkWaiter final : public SocketHandleWaiter {
 public:
  using ReadyHandle = SocketHandleWaiter::ReadyHandle;

  MockNetworkWaiter() : SocketHandleWaiter(&FakeClock::now) {}

  MOCK_METHOD2(
      AwaitSocketsReadable,
      ErrorOr<std::vector<ReadyHandle>>(const std::vector<SocketHandleRef>&,
                                        const Clock::duration&));
};

class MockSocket : public StreamSocketPosix {
 public:
  explicit MockSocket(int fd)
      : StreamSocketPosix(IPAddress::Version::kV4), handle(fd) {}

  const SocketHandle& socket_handle() const override { return handle; }

  SocketHandle handle;
};

class MockConnection : public TlsConnectionPosix {
 public:
  explicit MockConnection(int fd, TaskRunner& task_runner)
      : TlsConnectionPosix(std::make_unique<MockSocket>(fd), task_runner) {}
  MOCK_METHOD0(SendAvailableBytes, void());
  MOCK_METHOD0(TryReceiveMessage, void());
};

}  // namespace

class TestingDataRouter : public TlsDataRouterPosix {
 public:
  explicit TestingDataRouter(SocketHandleWaiter* waiter)
      : TlsDataRouterPosix(waiter) {
    disable_locking_for_testing_ = true;
  }

  using TlsDataRouterPosix::IsSocketWatched;

  bool AnySocketsWatched() {
    std::unique_lock<std::mutex> lock(accept_socket_mutex_);
    return !accept_stream_sockets_.empty() && !accept_socket_mappings_.empty();
  }
};

class MockObserver : public TestingDataRouter::SocketObserver {
  MOCK_METHOD1(OnConnectionPending, void(StreamSocketPosix*));
};

class TlsNetworkingManagerPosixTest : public testing::Test {
 public:
  TlsNetworkingManagerPosixTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        network_manager_(&network_waiter_) {}

  FakeTaskRunner& task_runner() { return task_runner_; }
  TestingDataRouter* network_manager() { return &network_manager_; }

 private:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  MockNetworkWaiter network_waiter_;
  TestingDataRouter network_manager_;
};

TEST_F(TlsNetworkingManagerPosixTest, SocketsWatchedCorrectly) {
  auto socket = std::make_unique<StreamSocketPosix>(IPAddress::Version::kV4);
  MockObserver observer;

  auto* ptr = socket.get();
  ASSERT_FALSE(network_manager()->IsSocketWatched(ptr));

  network_manager()->RegisterAcceptObserver(std::move(socket), &observer);
  ASSERT_TRUE(network_manager()->IsSocketWatched(ptr));
  ASSERT_TRUE(network_manager()->AnySocketsWatched());

  network_manager()->DeregisterAcceptObserver(&observer);
  ASSERT_FALSE(network_manager()->IsSocketWatched(ptr));
  ASSERT_FALSE(network_manager()->AnySocketsWatched());

  network_manager()->DeregisterAcceptObserver(&observer);
  ASSERT_FALSE(network_manager()->IsSocketWatched(ptr));
  ASSERT_FALSE(network_manager()->AnySocketsWatched());
}

TEST_F(TlsNetworkingManagerPosixTest, CallsReadySocket) {
  MockConnection connection1(1, task_runner());
  MockConnection connection2(2, task_runner());
  MockConnection connection3(3, task_runner());
  network_manager()->RegisterConnection(&connection1);
  network_manager()->RegisterConnection(&connection2);
  network_manager()->RegisterConnection(&connection3);

  EXPECT_CALL(connection1, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection1, TryReceiveMessage()).Times(1);
  EXPECT_CALL(connection2, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection2, TryReceiveMessage()).Times(0);
  EXPECT_CALL(connection3, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection3, TryReceiveMessage()).Times(0);
  network_manager()->ProcessReadyHandle(connection1.socket_handle(),
                                        SocketHandleWaiter::Flags::kReadable);

  EXPECT_CALL(connection1, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection1, TryReceiveMessage()).Times(0);
  EXPECT_CALL(connection2, SendAvailableBytes()).Times(1);
  EXPECT_CALL(connection2, TryReceiveMessage()).Times(0);
  EXPECT_CALL(connection3, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection3, TryReceiveMessage()).Times(0);
  network_manager()->ProcessReadyHandle(connection2.socket_handle(),
                                        SocketHandleWaiter::Flags::kWriteable);
}

TEST_F(TlsNetworkingManagerPosixTest, DeregisterTlsConnection) {
  MockConnection connection1(1, task_runner());
  MockConnection connection2(2, task_runner());
  network_manager()->RegisterConnection(&connection1);
  network_manager()->RegisterConnection(&connection2);

  network_manager()->DeregisterConnection(&connection1);
  EXPECT_CALL(connection1, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection1, TryReceiveMessage()).Times(0);
  network_manager()->ProcessReadyHandle(connection1.socket_handle(),
                                        SocketHandleWaiter::Flags::kReadable);
  EXPECT_CALL(connection2, SendAvailableBytes()).Times(0);
  EXPECT_CALL(connection2, TryReceiveMessage()).Times(1);
  network_manager()->ProcessReadyHandle(connection2.socket_handle(),
                                        SocketHandleWaiter::Flags::kReadable);
}

}  // namespace openscreen
