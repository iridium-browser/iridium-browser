// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/impl/socket_handle_posix.h"

#include <cstdlib>
#include <functional>

namespace openscreen {

SocketHandle::SocketHandle(int descriptor) : fd(descriptor) {}

bool operator==(const SocketHandle& lhs, const SocketHandle& rhs) {
  return lhs.fd == rhs.fd;
}

size_t SocketHandleHash::operator()(const SocketHandle& handle) const {
  return std::hash<int>()(handle.fd);
}

}  // namespace openscreen
