// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_TESTING_MOCK_SOCKET_ERROR_HANDLER_H_
#define CAST_COMMON_CHANNEL_TESTING_MOCK_SOCKET_ERROR_HANDLER_H_

#include "cast/common/channel/virtual_connection_router.h"
#include "gmock/gmock.h"
#include "platform/base/error.h"

namespace openscreen {
namespace cast {

class MockSocketErrorHandler
    : public VirtualConnectionRouter::SocketErrorHandler {
 public:
  MOCK_METHOD(void, OnClose, (CastSocket * socket), (override));
  MOCK_METHOD(void, OnError, (CastSocket * socket, Error error), (override));
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_TESTING_MOCK_SOCKET_ERROR_HANDLER_H_
