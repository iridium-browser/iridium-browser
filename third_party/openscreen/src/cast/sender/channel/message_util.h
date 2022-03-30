// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_SENDER_CHANNEL_MESSAGE_UTIL_H_
#define CAST_SENDER_CHANNEL_MESSAGE_UTIL_H_

#include "cast/common/channel/message_util.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "platform/base/error.h"

namespace openscreen {
namespace cast {

class AuthContext;

::cast::channel::CastMessage CreateAuthChallengeMessage(
    const AuthContext& auth_context);

// |request_id| must be unique for |sender_id|.
ErrorOr<::cast::channel::CastMessage> CreateAppAvailabilityRequest(
    const std::string& sender_id,
    int request_id,
    const std::string& app_id);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_SENDER_CHANNEL_MESSAGE_UTIL_H_
