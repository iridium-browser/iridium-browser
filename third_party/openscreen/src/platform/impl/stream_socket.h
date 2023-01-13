// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_STREAM_SOCKET_H_
#define PLATFORM_IMPL_STREAM_SOCKET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "platform/api/network_interface.h"
#include "platform/base/error.h"
#include "platform/base/ip_address.h"
#include "platform/base/macros.h"
#include "platform/impl/socket_handle.h"
#include "platform/impl/socket_state.h"

namespace openscreen {

// StreamSocket is an incomplete abstraction of synchronous platform methods for
// creating, initializing, and closing stream sockets. Callers can use this
// class to define complete TCP and TLS socket classes, both synchronous and
// asynchronous.
class StreamSocket {
 public:
  StreamSocket() = default;
  StreamSocket(const StreamSocket& other) = delete;
  StreamSocket(StreamSocket&& other) noexcept = default;
  virtual ~StreamSocket() = default;

  StreamSocket& operator=(const StreamSocket& other) = delete;
  StreamSocket& operator=(StreamSocket&& other) = default;

  // Used by passive/server sockets to accept connection requests
  // from a client.
  virtual ErrorOr<std::unique_ptr<StreamSocket>> Accept() = 0;

  // Bind to the address provided on socket construction. Socket should be
  // unbound and not connected.
  virtual Error Bind() = 0;

  // Closes the socket. Socket must be opened before this method is called.
  virtual Error Close() = 0;

  // Connects the socket to a specified remote address. Socket should be
  // initialized and bound, but not connected.
  virtual Error Connect(const IPEndpoint& remote_endpoint) = 0;

  // Marks the socket as passive, to receive incoming connections.
  virtual Error Listen() = 0;
  virtual Error Listen(int max_backlog_size) = 0;

  // Returns the file descriptor (e.g. fd or HANDLE pointer) for this socket.
  virtual const SocketHandle& socket_handle() const = 0;

  // Returns the connected remote address, if socket is connected.
  virtual absl::optional<IPEndpoint> remote_address() const = 0;

  // Returns the local address, if one is assigned.
  virtual absl::optional<IPEndpoint> local_address() const = 0;

  // Returns the state of the socket.
  virtual TcpSocketState state() const = 0;

  // Returns the IP version of the socket.
  virtual IPAddress::Version version() const = 0;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_STREAM_SOCKET_H_
