// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SENDER_MESSAGE_H_
#define CAST_STREAMING_SENDER_MESSAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "absl/types/variant.h"
#include "cast/streaming/offer_messages.h"
#include "json/value.h"
#include "platform/base/error.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

struct SenderMessage {
 public:
  // Receiver response message type.
  enum class Type {
    // Unknown message type.
    kUnknown,

    // OFFER request message.
    kOffer,

    // GET_CAPABILITIES request message.
    kGetCapabilities,

    // Rpc binary messages. The payload is base64-encoded.
    kRpc,
  };

  static ErrorOr<SenderMessage> Parse(const Json::Value& value);
  ErrorOr<Json::Value> ToJson() const;

  Type type = Type::kUnknown;
  int32_t sequence_number = -1;
  bool valid = false;
  absl::variant<absl::monostate,
                std::vector<uint8_t>,  // Binary-encoded RPC message.
                Offer,
                std::string>
      body;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SENDER_MESSAGE_H_
