// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_UDP_SOCKET_READER_POSIX_H_
#define PLATFORM_IMPL_UDP_SOCKET_READER_POSIX_H_

#include <map>
#include <mutex>
#include <vector>

#include "platform/api/task_runner.h"
#include "platform/api/time.h"
#include "platform/impl/socket_handle.h"
#include "platform/impl/socket_handle_waiter.h"
#include "platform/impl/udp_socket_posix.h"

namespace openscreen {

// This is the class responsible for watching sockets for readable data, then
// calling the function associated with these sockets once that data is read.
// NOTE: This class will only function as intended while its RunUntilStopped
// method is running.
class UdpSocketReaderPosix : public SocketHandleWaiter::Subscriber {
 public:
  using SocketHandleRef = SocketHandleWaiter::SocketHandleRef;

  // Creates a new instance of this object.
  // NOTE: The provided NetworkWaiter must outlive this object.
  explicit UdpSocketReaderPosix(SocketHandleWaiter* waiter);
  ~UdpSocketReaderPosix() override;

  // Waits for |socket| to be readable and then calls the socket's
  // RecieveMessage(...) method to process the available packet.
  // NOTE: The first read on any newly watched socket may be delayed up to 50
  // ms.
  void OnCreate(UdpSocket* socket);

  // Cancels any pending wait on reading |socket|. Following this call, any
  // pending reads will proceed but their associated callbacks will not fire.
  // NOTE: This method will block until a delete is safe.
  // NOTE: If a socket callback is removed in the middle of a wait call, data
  // may be read on this socket and but the callback may not be called. If a
  // socket callback is added in the middle of a wait call, the new socket may
  // not be watched until after this wait call ends.
  virtual void OnDestroy(UdpSocket* socket);

  // SocketHandleWaiter::Subscriber overrides.
  void ProcessReadyHandle(SocketHandleRef handle, uint32_t flags) override;

  OSP_DISALLOW_COPY_AND_ASSIGN(UdpSocketReaderPosix);

 protected:
  bool IsMappedReadForTesting(UdpSocketPosix* socket) const;

 private:
  // Helper method to allow for OnDestroy calls without blocking.
  void OnDelete(UdpSocketPosix* socket,
                bool disable_locking_for_testing = false);

  // The set of all sockets that are being read from
  std::vector<UdpSocketPosix*> sockets_;

  // Mutex to protect against concurrent modification of socket info.
  std::mutex mutex_;

  // NetworkWaiter watching this NetworkReader.
  SocketHandleWaiter* const waiter_;

  friend class TestingUdpSocketReader;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_UDP_SOCKET_READER_POSIX_H_
