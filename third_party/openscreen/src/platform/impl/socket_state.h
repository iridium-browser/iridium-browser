// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_STATE_H_
#define PLATFORM_IMPL_SOCKET_STATE_H_

#include <cstdint>
#include <memory>
#include <string>

namespace openscreen {

// TcpSocketState should be used by TCP and TLS sockets for indicating
// current state. NOTE: socket state transitions should only happen in
// the listed order. New states should be added in appropriate order.
enum class TcpSocketState {
  // Socket is not connected.
  kNotConnected = 0,

  // Socket is actively listening for incoming connections.
  kListening,

  // Socket is currently being connected.
  kConnecting,

  // Socket is actively connected to a remote address.
  kConnected,

  // The socket connection has been terminated, either by Close() or
  // by the remote side.
  kClosed
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_STATE_H_
