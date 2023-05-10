// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_handle_waiter_posix.h"

#include <sys/socket.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/impl/socket_handle_posix.h"
#include "platform/impl/timeval_posix.h"
#include "platform/test/fake_clock.h"

namespace openscreen {
namespace {

using namespace ::testing;
using ::testing::_;

class MockSubscriber : public SocketHandleWaiter::Subscriber {
 public:
  using SocketHandleRef = SocketHandleWaiter::SocketHandleRef;
  MOCK_METHOD2(ProcessReadyHandle, void(SocketHandleRef, uint32_t));
};

class TestingSocketHandleWaiter : public SocketHandleWaiter {
 public:
  using SocketHandleRef = SocketHandleWaiter::SocketHandleRef;
  using ReadyHandle = SocketHandleWaiter::ReadyHandle;

  TestingSocketHandleWaiter() : SocketHandleWaiter(&FakeClock::now) {}

  MOCK_METHOD2(
      AwaitSocketsReadable,
      ErrorOr<std::vector<ReadyHandle>>(const std::vector<SocketHandleRef>&,
                                        const Clock::duration&));

  FakeClock fake_clock{Clock::time_point{Clock::duration{1234567}}};
};

}  // namespace

TEST(SocketHandleWaiterTest, BubblesUpAwaitSocketsReadableErrors) {
  MockSubscriber subscriber;
  TestingSocketHandleWaiter waiter;
  SocketHandle handle0{0};
  SocketHandle handle1{1};
  SocketHandle handle2{2};
  const SocketHandle& handle0_ref = handle0;
  const SocketHandle& handle1_ref = handle1;
  const SocketHandle& handle2_ref = handle2;

  waiter.Subscribe(&subscriber, std::cref(handle0_ref));
  waiter.Subscribe(&subscriber, std::cref(handle1_ref));
  waiter.Subscribe(&subscriber, std::cref(handle2_ref));
  Error::Code response = Error::Code::kAgain;
  EXPECT_CALL(subscriber, ProcessReadyHandle(_, _)).Times(0);
  EXPECT_CALL(waiter, AwaitSocketsReadable(_, _))
      .WillOnce(Return(ByMove(response)));
  waiter.ProcessHandles(Clock::duration{0});
}

TEST(SocketHandleWaiterTest, WatchedSocketsReturnedToCorrectSubscribers) {
  MockSubscriber subscriber;
  MockSubscriber subscriber2;
  TestingSocketHandleWaiter waiter;
  SocketHandle handle0{0};
  SocketHandle handle1{1};
  SocketHandle handle2{2};
  SocketHandle handle3{3};
  const SocketHandle& handle0_ref = handle0;
  const SocketHandle& handle1_ref = handle1;
  const SocketHandle& handle2_ref = handle2;
  const SocketHandle& handle3_ref = handle3;

  waiter.Subscribe(&subscriber, std::cref(handle0_ref));
  waiter.Subscribe(&subscriber, std::cref(handle2_ref));
  waiter.Subscribe(&subscriber2, std::cref(handle1_ref));
  waiter.Subscribe(&subscriber2, std::cref(handle3_ref));
  constexpr uint32_t r_flags = SocketHandleWaiter::Flags::kReadable;
  constexpr uint32_t w_flags = SocketHandleWaiter::Flags::kWriteable;
  constexpr uint32_t rw_flags = SocketHandleWaiter::Flags::kReadable |
                                SocketHandleWaiter::Flags::kWriteable;
  EXPECT_CALL(subscriber, ProcessReadyHandle(std::cref(handle0_ref), r_flags))
      .Times(1);
  EXPECT_CALL(subscriber, ProcessReadyHandle(std::cref(handle2_ref), w_flags))
      .Times(1);
  EXPECT_CALL(subscriber2, ProcessReadyHandle(std::cref(handle1_ref), r_flags))
      .Times(1);
  EXPECT_CALL(subscriber2, ProcessReadyHandle(std::cref(handle3_ref), rw_flags))
      .Times(1);
  EXPECT_CALL(waiter, AwaitSocketsReadable(_, _))
      .WillOnce(
          Return(ByMove(std::vector<TestingSocketHandleWaiter::ReadyHandle>{
              {std::cref(handle0_ref), r_flags},
              {std::cref(handle1_ref), r_flags},
              {std::cref(handle2_ref), w_flags},
              {std::cref(handle3_ref), rw_flags}})));
  waiter.ProcessHandles(Clock::duration{0});
}

}  // namespace openscreen
