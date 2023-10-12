// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_IMPL_SOCKET_HANDLE_POSIX_H_
#define PLATFORM_IMPL_SOCKET_HANDLE_POSIX_H_

#include "platform/impl/socket_handle.h"

namespace openscreen {

struct SocketHandle {
  explicit SocketHandle(int descriptor);
  int fd;
};

}  // namespace openscreen

#endif  // PLATFORM_IMPL_SOCKET_HANDLE_POSIX_H_
