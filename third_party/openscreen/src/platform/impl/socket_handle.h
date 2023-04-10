// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_HANDLE_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_H_

#include <cstdlib>

namespace openscreen {

// A SocketHandle is the handle used to access a Socket by the underlying
// platform.
struct SocketHandle;

struct SocketHandleHash {
  size_t operator()(const SocketHandle& handle) const;
};

bool operator==(const SocketHandle& lhs, const SocketHandle& rhs);
inline bool operator!=(const SocketHandle& lhs, const SocketHandle& rhs) {
  return !(lhs == rhs);
}

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_H_
