// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_TESTING_MOCK_CAST_MESSAGE_HANDLER_H_
#define CAST_COMMON_CHANNEL_TESTING_MOCK_CAST_MESSAGE_HANDLER_H_

#include "cast/common/channel/cast_message_handler.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "gmock/gmock.h"

namespace openscreen {
namespace cast {

class MockCastMessageHandler final : public CastMessageHandler {
 public:
  MOCK_METHOD(void,
              OnMessage,
              (VirtualConnectionRouter * router,
               CastSocket* socket,
               ::cast::channel::CastMessage message),
              (override));
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_TESTING_MOCK_CAST_MESSAGE_HANDLER_H_
