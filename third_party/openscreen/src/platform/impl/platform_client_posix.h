// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_PLATFORM_CLIENT_POSIX_H_
#define PLATFORM_IMPL_PLATFORM_CLIENT_POSIX_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "absl/types/optional.h"
#include "platform/api/time.h"
#include "platform/base/macros.h"
#include "platform/impl/socket_handle_waiter_posix.h"
#include "platform/impl/task_runner.h"
#include "platform/impl/tls_data_router_posix.h"

namespace openscreen {

class UdpSocketReaderPosix;

// Creates and provides access to singletons used by the default platform
// implementation. An instance must be created before an application uses any
// public modules in the Open Screen Library.
//
// ShutDown() should be called to destroy the PlatformClientPosix's singletons
// and TaskRunner to save resources when library APIs are not in use.
// ShutDown() calls TaskRunner::RunUntilStopped() to run any pending cleanup
// tasks.
//
// Create and ShutDown must be called in the same sequence.
//
// FIXME: Remove Create and Shutdown and use the ctor/dtor directly.
class PlatformClientPosix {
 public:
  // Initializes the platform implementation.
  //
  // |networking_loop_interval| sets the minimum amount of time that should pass
  // between iterations of the loop used to handle networking operations. Higher
  // values will result in less time being spent on these operations, but also
  // less performant networking operations. Be careful setting values larger
  // than a few hundred microseconds.
  //
  // |networking_operation_timeout| sets how much time may be spent on a
  // single networking operation type.
  //
  // |task_runner| is a client-provided TaskRunner implementation.
  static void Create(Clock::duration networking_operation_timeout,
                     std::unique_ptr<TaskRunnerImpl> task_runner);

  // Initializes the platform implementation and creates a new TaskRunner (which
  // starts a new thread).
  static void Create(Clock::duration networking_operation_timeout);

  // Shuts down and deletes the PlatformClient instance currently stored as a
  // singleton. This method is expected to be called before program exit. After
  // calling this method, if the client wishes to continue using the platform
  // library, Create() must be called again.
  static void ShutDown();

  static PlatformClientPosix* GetInstance() { return instance_; }

  // This method is thread-safe.
  // FIXME: Rename to GetTlsDataRouter()
  TlsDataRouterPosix* tls_data_router();

  // This method is thread-safe.
  // FIXME: Rename to GetUdpSocketReader()
  UdpSocketReaderPosix* udp_socket_reader();

  // Returns the TaskRunner associated with this PlatformClient.
  // NOTE: This method is expected to be thread safe.
  TaskRunner* GetTaskRunner();

 protected:
  // Called by ShutDown().
  ~PlatformClientPosix();

  static void SetInstance(PlatformClientPosix* client);

 private:
  explicit PlatformClientPosix(Clock::duration networking_operation_timeout);

  PlatformClientPosix(Clock::duration networking_operation_timeout,
                      std::unique_ptr<TaskRunnerImpl> task_runner);

  // This method is thread-safe.
  SocketHandleWaiterPosix* socket_handle_waiter();

  void RunNetworkLoopUntilStopped();

  std::unique_ptr<TaskRunnerImpl> task_runner_;

  // Track whether the associated instance variable has been created yet.
  std::atomic_bool waiter_created_{false};
  std::atomic_bool tls_data_router_created_{false};

  // Parameters for networking loop.
  std::atomic_bool networking_loop_running_{true};
  Clock::duration networking_loop_timeout_;

  // Flags used to ensure that initialization of below instance objects occurs
  // only once across all threads.
  std::once_flag waiter_initialization_;
  std::once_flag udp_socket_reader_initialization_;
  std::once_flag tls_data_router_initialization_;

  // Instance objects are created at runtime when they are first needed.
  std::unique_ptr<SocketHandleWaiterPosix> waiter_;
  std::unique_ptr<UdpSocketReaderPosix> udp_socket_reader_;
  std::unique_ptr<TlsDataRouterPosix> tls_data_router_;

  // Threads for running TaskRunner and OperationLoop instances.
  // NOTE: These must be declared last to avoid nondterministic failures.
  std::thread networking_loop_thread_;
  absl::optional<std::thread> task_runner_thread_;

  static PlatformClientPosix* instance_;

  OSP_DISALLOW_COPY_AND_ASSIGN(PlatformClientPosix);
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_PLATFORM_CLIENT_POSIX_H_
