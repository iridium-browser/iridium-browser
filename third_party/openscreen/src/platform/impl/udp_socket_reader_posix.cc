// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/udp_socket_reader_posix.h"

#include <chrono>
#include <functional>

#include "platform/impl/socket_handle_posix.h"
#include "platform/impl/udp_socket_posix.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {

UdpSocketReaderPosix::UdpSocketReaderPosix(SocketHandleWaiter* waiter)
    : waiter_(waiter) {}

UdpSocketReaderPosix::~UdpSocketReaderPosix() {
  waiter_->UnsubscribeAll(this);
}

void UdpSocketReaderPosix::ProcessReadyHandle(SocketHandleRef handle,
                                              uint32_t flags) {
  if (flags & SocketHandleWaiter::Flags::kReadable) {
    std::lock_guard<std::mutex> lock(mutex_);
    // NOTE: Because sockets_ is expected to remain small, the performance here
    // is better than using an unordered_set.
    for (UdpSocketPosix* socket : sockets_) {
      if (socket->GetHandle() == handle) {
        socket->ReceiveMessage();
        break;
      }
    }
  }
}

void UdpSocketReaderPosix::OnCreate(UdpSocket* socket) {
  UdpSocketPosix* read_socket = static_cast<UdpSocketPosix*>(socket);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    sockets_.push_back(read_socket);
  }
  waiter_->Subscribe(this, std::cref(read_socket->GetHandle()));
}

void UdpSocketReaderPosix::OnDestroy(UdpSocket* socket) {
  UdpSocketPosix* destroyed_socket = static_cast<UdpSocketPosix*>(socket);
  OnDelete(destroyed_socket);
}

void UdpSocketReaderPosix::OnDelete(UdpSocketPosix* socket,
                                    bool disable_locking_for_testing) {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = std::find(sockets_.begin(), sockets_.end(), socket);
    if (it != sockets_.end()) {
      sockets_.erase(it);
    }
  }

  waiter_->OnHandleDeletion(this, std::cref(socket->GetHandle()),
                            disable_locking_for_testing);
}

bool UdpSocketReaderPosix::IsMappedReadForTesting(
    UdpSocketPosix* socket) const {
  return Contains(sockets_, socket);
}

}  // namespace openscreen
