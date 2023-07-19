// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/tls_data_router_posix.h"

#include <memory>
#include <utility>

#include "platform/impl/stream_socket_posix.h"
#include "platform/impl/tls_connection_posix.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {

TlsDataRouterPosix::TlsDataRouterPosix(
    SocketHandleWaiter* waiter,
    std::function<Clock::time_point()> now_function)
    : waiter_(waiter), now_function_(now_function) {}

TlsDataRouterPosix::~TlsDataRouterPosix() {
  waiter_->UnsubscribeAll(this);
}

void TlsDataRouterPosix::RegisterConnection(TlsConnectionPosix* connection) {
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    OSP_DCHECK(!Contains(connections_, connection));
    connections_.push_back(connection);
  }

  waiter_->Subscribe(this, connection->socket_handle());
}

void TlsDataRouterPosix::DeregisterConnection(TlsConnectionPosix* connection) {
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = std::remove_if(
        connections_.begin(), connections_.end(),
        [connection](TlsConnectionPosix* conn) { return conn == connection; });
    if (it == connections_.end()) {
      return;
    }
    connections_.erase(it, connections_.end());
  }

  waiter_->OnHandleDeletion(this, connection->socket_handle(),
                            disable_locking_for_testing_);
}

void TlsDataRouterPosix::RegisterAcceptObserver(
    std::unique_ptr<StreamSocketPosix> socket,
    SocketObserver* observer) {
  OSP_DCHECK(observer);
  StreamSocketPosix* socket_ptr = socket.get();
  {
    std::unique_lock<std::mutex> lock(accept_socket_mutex_);
    accept_stream_sockets_.push_back(std::move(socket));
    accept_socket_mappings_[socket_ptr] = observer;
  }

  waiter_->Subscribe(this, socket_ptr->socket_handle());
}

void TlsDataRouterPosix::DeregisterAcceptObserver(SocketObserver* observer) {
  std::vector<std::unique_ptr<StreamSocketPosix>> sockets_to_delete;
  {
    std::unique_lock<std::mutex> lock(accept_socket_mutex_);
    for (auto it = accept_stream_sockets_.begin();
         it != accept_stream_sockets_.end();) {
      auto map_entry = accept_socket_mappings_.find(it->get());
      OSP_DCHECK(map_entry != accept_socket_mappings_.end());
      if (map_entry->second == observer) {
        sockets_to_delete.push_back(std::move(*it));
        accept_socket_mappings_.erase(map_entry);
        it = accept_stream_sockets_.erase(it);
      } else {
        ++it;
      }
    }
  }

  for (auto& socket : sockets_to_delete) {
    waiter_->OnHandleDeletion(this, socket->socket_handle(),
                              disable_locking_for_testing_);
  }
}

void TlsDataRouterPosix::ProcessReadyHandle(
    SocketHandleWaiter::SocketHandleRef handle,
    uint32_t flags) {
  if (flags & SocketHandleWaiter::Flags::kReadable) {
    std::unique_lock<std::mutex> lock(accept_socket_mutex_);
    for (const auto& pair : accept_socket_mappings_) {
      if (pair.first->socket_handle() == handle) {
        pair.second->OnConnectionPending(pair.first);
        return;
      }
    }
  }
  {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (TlsConnectionPosix* connection : connections_) {
      if (connection->socket_handle() == handle) {
        if (flags & SocketHandleWaiter::Flags::kReadable) {
          connection->TryReceiveMessage();
        }
        if (flags & SocketHandleWaiter::Flags::kWriteable) {
          connection->SendAvailableBytes();
        }
        return;
      }
    }
  }
}

bool TlsDataRouterPosix::HasTimedOut(Clock::time_point start_time,
                                     Clock::duration timeout) {
  return now_function_() - start_time > timeout;
}

bool TlsDataRouterPosix::IsSocketWatched(StreamSocketPosix* socket) const {
  std::unique_lock<std::mutex> lock(accept_socket_mutex_);
  return accept_socket_mappings_.find(socket) != accept_socket_mappings_.end();
}

}  // namespace openscreen
