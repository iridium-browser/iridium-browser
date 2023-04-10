// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_SESSION_MESSENGER_H_
#define CAST_STREAMING_SESSION_MESSENGER_H_

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "absl/types/variant.h"
#include "cast/common/public/message_port.h"
#include "cast/streaming/answer_messages.h"
#include "cast/streaming/offer_messages.h"
#include "cast/streaming/receiver_message.h"
#include "cast/streaming/sender_message.h"
#include "json/value.h"
#include "platform/api/task_runner.h"
#include "util/flat_map.h"
#include "util/weak_ptr.h"

namespace openscreen {
namespace cast {

// A message port interface designed specifically for use by the Receiver
// and Sender session classes.
class SessionMessenger : public MessagePort::Client {
 public:
  using ErrorCallback = std::function<void(Error)>;

  SessionMessenger(MessagePort* message_port,
                   std::string source_id,
                   ErrorCallback cb);
  ~SessionMessenger() override;

 protected:
  // Barebones message sending method shared by both children.
  [[nodiscard]] Error SendMessage(const std::string& destination_id,
                                  const std::string& namespace_,
                                  const Json::Value& message_root);

  // Used to report errors in subclasses.
  void ReportError(Error error);

  const std::string& source_id() override { return source_id_; }

 private:
  MessagePort* const message_port_;
  const std::string source_id_;
  ErrorCallback error_callback_;
};

// Message port interface designed to handle sending messages to and
// from a receiver. When possible, errors receiving messages are reported
// to the ReplyCallback passed to SendRequest(), otherwise errors are
// reported to the ErrorCallback passed in the constructor.
class SenderSessionMessenger final : public SessionMessenger {
 public:
  using ReplyCallback = std::function<void(ErrorOr<ReceiverMessage>)>;

  SenderSessionMessenger(MessagePort* message_port,
                         std::string source_id,
                         std::string receiver_id,
                         ErrorCallback cb,
                         TaskRunner* task_runner);

  // Set receiver message handler. Note that this should only be
  // applied for messages that don't have sequence numbers, like RPC
  // and status messages.
  void SetHandler(ReceiverMessage::Type type, ReplyCallback cb);
  void ResetHandler(ReceiverMessage::Type type);

  // Send a message that doesn't require a reply.
  [[nodiscard]] Error SendOutboundMessage(SenderMessage message);

  // Convenience method for sending a valid RPC message.
  [[nodiscard]] Error SendRpcMessage(const std::vector<uint8_t>& message);

  // Send a request (with optional reply callback).
  [[nodiscard]] Error SendRequest(SenderMessage message,
                                  ReceiverMessage::Type reply_type,
                                  ReplyCallback cb);

  // MessagePort::Client overrides
  void OnMessage(const std::string& source_id,
                 const std::string& message_namespace,
                 const std::string& message) override;
  void OnError(Error error) override;

 private:
  TaskRunner* const task_runner_;

  // This messenger should only be connected to one receiver, so |receiver_id_|
  // should not change.
  const std::string receiver_id_;

  // We keep a list here of replies we are expecting--if the reply is
  // received for this sequence number, we call its respective callback,
  // otherwise it is called after an internally specified timeout.
  FlatMap<int, ReplyCallback> awaiting_replies_;

  // Currently we can only set a handler for RPC messages, so no need for
  // a flatmap here.
  ReplyCallback rpc_callback_;

  WeakPtrFactory<SenderSessionMessenger> weak_factory_{this};
};

// Message port interface designed for messaging to and from a sender.
class ReceiverSessionMessenger final : public SessionMessenger {
 public:
  using RequestCallback =
      std::function<void(const std::string&, SenderMessage)>;
  ReceiverSessionMessenger(MessagePort* message_port,
                           std::string source_id,
                           ErrorCallback cb);

  // Set sender message handler.
  void SetHandler(SenderMessage::Type type, RequestCallback cb);
  void ResetHandler(SenderMessage::Type type);

  // Send a JSON message.
  [[nodiscard]] Error SendMessage(const std::string& source_id,
                                  ReceiverMessage message);

  // MessagePort::Client overrides
  void OnMessage(const std::string& source_id,
                 const std::string& message_namespace,
                 const std::string& message) override;
  void OnError(Error error) override;

 private:
  FlatMap<SenderMessage::Type, RequestCallback> callbacks_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_SESSION_MESSENGER_H_
