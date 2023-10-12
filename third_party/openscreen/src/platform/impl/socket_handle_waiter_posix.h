// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_HANDLE_WAITER_POSIX_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_WAITER_POSIX_H_

#include <unistd.h>

#include <atomic>
#include <mutex>
#include <vector>

#include "platform/impl/socket_handle_waiter.h"

namespace openscreen {

class SocketHandleWaiterPosix : public SocketHandleWaiter {
 public:
  using SocketHandleRef = SocketHandleWaiter::SocketHandleRef;

  explicit SocketHandleWaiterPosix(ClockNowFunctionPtr now_function);
  ~SocketHandleWaiterPosix() override;

  // Runs the Wait function in a loop until the below RequestStopSoon function
  // is called.
  void RunUntilStopped();

  // Signals for the RunUntilStopped loop to cease running.
  void RequestStopSoon();

 protected:
  using SocketHandleWaiter::ReadyHandle;

  ErrorOr<std::vector<ReadyHandle>> AwaitSocketsReadable(
      const std::vector<SocketHandleRef>& socket_fds,
      const Clock::duration& timeout) override;

 private:
  // Atomic so that we can perform atomic exchanges.
  std::atomic_bool is_running_;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_WAITER_POSIX_H_
