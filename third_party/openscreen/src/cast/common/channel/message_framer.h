// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_COMMON_CHANNEL_MESSAGE_FRAMER_H_
#define CAST_COMMON_CHANNEL_MESSAGE_FRAMER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "absl/types/span.h"
#include "cast/common/channel/proto/cast_channel.pb.h"
#include "platform/base/error.h"

namespace openscreen {
namespace cast {
namespace message_serialization {

// Serializes |message_proto| into |message_data|.
// Returns true if the message was serialized successfully, false otherwise.
ErrorOr<std::vector<uint8_t>> Serialize(
    const ::cast::channel::CastMessage& message);

struct DeserializeResult {
  ::cast::channel::CastMessage message;
  size_t length;
};

// Reads bytes from |input| and returns a new CastMessage if one is fully
// read.  Returns a parsed CastMessage if a message was received in its
// entirety, and an error otherwise.  The result also contains the number of
// bytes consumed from |input| when a parse succeeds.
ErrorOr<DeserializeResult> TryDeserialize(absl::Span<const uint8_t> input);

}  // namespace message_serialization
}  // namespace cast
}  // namespace openscreen

#endif  // CAST_COMMON_CHANNEL_MESSAGE_FRAMER_H_
