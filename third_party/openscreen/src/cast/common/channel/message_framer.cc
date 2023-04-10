// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/channel/message_framer.h"

#include <stdlib.h>
#include <string.h>

#include <limits>

#include "cast/common/channel/proto/cast_channel.pb.h"
#include "util/big_endian.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace message_serialization {

namespace {

static constexpr size_t kHeaderSize = sizeof(uint32_t);

// Cast specifies a max message body size of 64 KiB.
static constexpr size_t kMaxBodySize = 65536;

}  // namespace

ErrorOr<std::vector<uint8_t>> Serialize(
    const ::cast::channel::CastMessage& message) {
  const size_t message_size = message.ByteSizeLong();
  if (message_size > kMaxBodySize || message_size == 0) {
    return Error::Code::kCastV2InvalidMessage;
  }
  std::vector<uint8_t> out(message_size + kHeaderSize, 0);
  WriteBigEndian<uint32_t>(message_size, out.data());
  if (!message.SerializeToArray(&out[kHeaderSize], message_size)) {
    return Error::Code::kCastV2InvalidMessage;
  }
  return out;
}

ErrorOr<DeserializeResult> TryDeserialize(absl::Span<const uint8_t> input) {
  if (input.size() < kHeaderSize) {
    return Error::Code::kInsufficientBuffer;
  }

  const uint32_t message_size = ReadBigEndian<uint32_t>(input.data());
  if (message_size > kMaxBodySize) {
    return Error::Code::kCastV2InvalidMessage;
  }

  if (input.size() < (kHeaderSize + message_size)) {
    return Error::Code::kInsufficientBuffer;
  }

  DeserializeResult result;
  if (!result.message.ParseFromArray(input.data() + kHeaderSize,
                                     message_size)) {
    return Error::Code::kCastV2InvalidMessage;
  }
  result.length = kHeaderSize + message_size;

  return result;
}

}  // namespace message_serialization
}  // namespace cast
}  // namespace openscreen
