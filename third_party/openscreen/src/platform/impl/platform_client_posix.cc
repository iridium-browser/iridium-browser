// Copyright (c) 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/platform_client_posix.h"

#include <functional>
#include <utility>
#include <vector>

#include "platform/impl/udp_socket_reader_posix.h"

namespace openscreen {

// static
PlatformClientPosix* PlatformClientPosix::instance_ = nullptr;

// static
void PlatformClientPosix::Create(Clock::duration networking_operation_timeout,
                                 std::unique_ptr<TaskRunnerImpl> task_runner) {
  SetInstance(new PlatformClientPosix(networking_operation_timeout,
                                      std::move(task_runner)));
}

// static
void PlatformClientPosix::Create(Clock::duration networking_operation_timeout) {
  SetInstance(new PlatformClientPosix(networking_operation_timeout));
}

// static
void PlatformClientPosix::ShutDown() {
  OSP_DCHECK(instance_);
  delete instance_;
  instance_ = nullptr;
}

TlsDataRouterPosix* PlatformClientPosix::tls_data_router() {
  std::call_once(tls_data_router_initialization_, [this]() {
    tls_data_router_ =
        std::make_unique<TlsDataRouterPosix>(socket_handle_waiter());
    tls_data_router_created_.store(true);
  });
  return tls_data_router_.get();
}

UdpSocketReaderPosix* PlatformClientPosix::udp_socket_reader() {
  std::call_once(udp_socket_reader_initialization_, [this]() {
    udp_socket_reader_ =
        std::make_unique<UdpSocketReaderPosix>(socket_handle_waiter());
  });
  return udp_socket_reader_.get();
}

TaskRunner& PlatformClientPosix::GetTaskRunner() {
  return *task_runner_;
}

PlatformClientPosix::~PlatformClientPosix() {
  OSP_DVLOG << "Shutting down the Task Runner...";
  task_runner_->RequestStopSoon();
  if (task_runner_thread_ && task_runner_thread_->joinable()) {
    task_runner_thread_->join();
    OSP_DVLOG << "\tTask Runner shutdown complete!";
  }

  OSP_DVLOG << "Shutting down network operations...";
  networking_loop_running_.store(false);
  networking_loop_thread_.join();
  OSP_DVLOG << "\tNetwork operation shutdown complete!";
}

// static
void PlatformClientPosix::SetInstance(PlatformClientPosix* instance) {
  OSP_DCHECK(!instance_);
  instance_ = instance;
}

PlatformClientPosix::PlatformClientPosix(
    Clock::duration networking_operation_timeout)
    : task_runner_(new TaskRunnerImpl(Clock::now)),
      networking_loop_timeout_(networking_operation_timeout),
      networking_loop_thread_(&PlatformClientPosix::RunNetworkLoopUntilStopped,
                              this),
      task_runner_thread_(
          std::thread(&TaskRunnerImpl::RunUntilStopped, task_runner_.get())) {}

PlatformClientPosix::PlatformClientPosix(
    Clock::duration networking_operation_timeout,
    std::unique_ptr<TaskRunnerImpl> task_runner)
    : task_runner_(std::move(task_runner)),
      networking_loop_timeout_(networking_operation_timeout),
      networking_loop_thread_(&PlatformClientPosix::RunNetworkLoopUntilStopped,
                              this) {}

SocketHandleWaiterPosix* PlatformClientPosix::socket_handle_waiter() {
  std::call_once(waiter_initialization_, [this]() {
    waiter_ = std::make_unique<SocketHandleWaiterPosix>(&Clock::now);
    waiter_created_.store(true);
  });
  return waiter_.get();
}

void PlatformClientPosix::RunNetworkLoopUntilStopped() {
  while (networking_loop_running_.load()) {
    if (!waiter_created_.load()) {
      std::this_thread::sleep_for(networking_loop_timeout_);
      continue;
    }
    socket_handle_waiter()->ProcessHandles(networking_loop_timeout_);
  }
}

}  // namespace openscreen
