// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/session_messenger.h"

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "cast/common/public/message_port.h"
#include "cast/streaming/message_fields.h"
#include "util/json/json_helpers.h"
#include "util/json/json_serialization.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

namespace {

// Default timeout to receive a reply message in response to a request message
// sent by us.
constexpr std::chrono::milliseconds kReplyTimeout{4000};

// Special character indicating message was sent to all receivers or senders.
constexpr char kAnyDestination[] = "*";

void ReplyIfTimedOut(
    int sequence_number,
    std::vector<std::pair<int, SenderSessionMessenger::ReplyCallback>>*
        replies) {
  for (auto it = replies->begin(); it != replies->end(); ++it) {
    if (it->first == sequence_number) {
      OSP_VLOG << "Reply was an error with due to timeout for sequence number: "
               << sequence_number;

      // We erase before handling the callback, since it may invalidate the
      // replies vector.
      SenderSessionMessenger::ReplyCallback callback = std::move(it->second);
      replies->erase(it);
      callback(
          Error(Error::Code::kMessageTimeout,
                absl::StrCat("message timed out (max delay of",
                             std::chrono::milliseconds(kReplyTimeout).count(),
                             "ms).")));
      return;
    }
  }
}

}  // namespace

SessionMessenger::SessionMessenger(MessagePort* message_port,
                                   std::string source_id,
                                   ErrorCallback cb)
    : message_port_(message_port),
      source_id_(source_id),
      error_callback_(std::move(cb)) {
  OSP_DCHECK(message_port_);
  OSP_DCHECK(!source_id_.empty());
  message_port_->SetClient(*this);
}

SessionMessenger::~SessionMessenger() {
  message_port_->ResetClient();
}

Error SessionMessenger::SendMessage(const std::string& destination_id,
                                    const std::string& namespace_,
                                    const Json::Value& message_root) {
  OSP_DCHECK(namespace_ == kCastRemotingNamespace ||
             namespace_ == kCastWebrtcNamespace);
  auto body_or_error = json::Stringify(message_root);
  if (body_or_error.is_error()) {
    return std::move(body_or_error.error());
  }
  OSP_VLOG << "Sending message: DESTINATION[" << destination_id
           << "], NAMESPACE[" << namespace_ << "], BODY:\n"
           << body_or_error.value();
  message_port_->PostMessage(destination_id, namespace_, body_or_error.value());
  return Error::None();
}

void SessionMessenger::ReportError(Error error) {
  error_callback_(std::move(error));
}

SenderSessionMessenger::SenderSessionMessenger(MessagePort* message_port,
                                               std::string source_id,
                                               std::string receiver_id,
                                               ErrorCallback cb,
                                               TaskRunner* task_runner)
    : SessionMessenger(message_port, std::move(source_id), std::move(cb)),
      task_runner_(task_runner),
      receiver_id_(std::move(receiver_id)) {}

void SenderSessionMessenger::SetHandler(ReceiverMessage::Type type,
                                        ReplyCallback cb) {
  // Currently the only handler allowed is for RPC messages.
  OSP_DCHECK(type == ReceiverMessage::Type::kRpc);
  rpc_callback_ = std::move(cb);
}

void SenderSessionMessenger::ResetHandler(ReceiverMessage::Type type) {
  OSP_DCHECK(type == ReceiverMessage::Type::kRpc);
  rpc_callback_ = {};
}

Error SenderSessionMessenger::SendOutboundMessage(SenderMessage message) {
  const auto namespace_ = (message.type == SenderMessage::Type::kRpc)
                              ? kCastRemotingNamespace
                              : kCastWebrtcNamespace;

  ErrorOr<Json::Value> jsonified = message.ToJson();
  OSP_CHECK(jsonified.is_value()) << "Tried to send an invalid message";
  return SessionMessenger::SendMessage(receiver_id_, namespace_,
                                       jsonified.value());
}

Error SenderSessionMessenger::SendRpcMessage(
    const std::vector<uint8_t>& message) {
  return SendOutboundMessage(
      SenderMessage{openscreen::cast::SenderMessage::Type::kRpc,
                    -1 /* sequence_number, unused by RPC messages */,
                    true /* valid */, message});
}

Error SenderSessionMessenger::SendRequest(SenderMessage message,
                                          ReceiverMessage::Type reply_type,
                                          ReplyCallback cb) {
  // RPC messages are not meant to be request/reply.
  OSP_DCHECK(reply_type != ReceiverMessage::Type::kRpc);

  if (!cb) {
    return Error(Error::Code::kParameterInvalid,
                 "Must provide a reply callback");
  }
  const Error error = SendOutboundMessage(message);
  if (!error.ok()) {
    return error;
  }

  OSP_DCHECK(awaiting_replies_.find(message.sequence_number) ==
             awaiting_replies_.end());
  awaiting_replies_.emplace_back(message.sequence_number, std::move(cb));
  task_runner_->PostTaskWithDelay(
      [self = weak_factory_.GetWeakPtr(), seq_num = message.sequence_number] {
        if (self) {
          ReplyIfTimedOut(seq_num, &self->awaiting_replies_);
        }
      },
      kReplyTimeout);

  return Error::None();
}

void SenderSessionMessenger::OnMessage(const std::string& source_id,
                                       const std::string& message_namespace,
                                       const std::string& message) {
  if (source_id != receiver_id_ && source_id != kAnyDestination) {
    OSP_DLOG_WARN << "Received message from unknown/incorrect Cast Receiver "
                  << source_id << ". Currently connected to " << receiver_id_;
    return;
  }

  if (message_namespace != kCastWebrtcNamespace &&
      message_namespace != kCastRemotingNamespace) {
    OSP_DLOG_WARN << "Received message from unknown namespace: "
                  << message_namespace << ". Message was " << message;
    return;
  }

  ErrorOr<Json::Value> message_body = json::Parse(message);
  if (!message_body) {
    ReportError(message_body.error());
    OSP_DLOG_WARN << "Received an invalid message: " << message;
    return;
  }

  // If the message is valid JSON and we don't understand it, there are two
  // options: (1) it's an unknown type, or (2) the receiver filled out the
  // message incorrectly. In the first case we can drop it, it's likely just
  // unsupported. In the second case we might need it, so worth warning the
  // client.
  ErrorOr<ReceiverMessage> receiver_message =
      ReceiverMessage::Parse(message_body.value());
  if (receiver_message.is_error()) {
    ReportError(receiver_message.error());
    OSP_DLOG_WARN << "Received an invalid receiver message: "
                  << receiver_message.error();
    return;
  }

  if (receiver_message.value().type == ReceiverMessage::Type::kRpc) {
    if (rpc_callback_) {
      rpc_callback_(receiver_message.value());
    } else {
      OSP_DLOG_INFO << "Received RPC message but no callback, dropping";
    }
  } else {
    const int sequence_number = receiver_message.value().sequence_number;
    auto it = awaiting_replies_.find(sequence_number);
    if (it == awaiting_replies_.end()) {
      OSP_DLOG_WARN << "Received a reply I wasn't waiting for: "
                    << sequence_number;
      return;
    }

    ReplyCallback callback = std::move(it->second);
    awaiting_replies_.erase(it);
    callback(std::move(receiver_message.value()));
  }
}

void SenderSessionMessenger::OnError(Error error) {
  OSP_DLOG_WARN << "Received an error in the session messenger: " << error;
  ReportError(error);
}

ReceiverSessionMessenger::ReceiverSessionMessenger(MessagePort* message_port,
                                                   std::string source_id,
                                                   ErrorCallback cb)
    : SessionMessenger(message_port, std::move(source_id), std::move(cb)) {}

void ReceiverSessionMessenger::SetHandler(SenderMessage::Type type,
                                          RequestCallback cb) {
  OSP_DCHECK(callbacks_.find(type) == callbacks_.end());
  callbacks_.emplace_back(type, std::move(cb));
}

void ReceiverSessionMessenger::ResetHandler(SenderMessage::Type type) {
  callbacks_.erase_key(type);
}

Error ReceiverSessionMessenger::SendMessage(const std::string& source_id,
                                            ReceiverMessage message) {
  if (source_id.empty()) {
    return Error(Error::Code::kInitializationFailure,
                 "Cannot send a message without a current source ID.");
  }

  const auto namespace_ = (message.type == ReceiverMessage::Type::kRpc)
                              ? kCastRemotingNamespace
                              : kCastWebrtcNamespace;

  ErrorOr<Json::Value> message_json = message.ToJson();
  OSP_CHECK(message_json.is_value()) << "Tried to send an invalid message";
  return SessionMessenger::SendMessage(source_id, namespace_,
                                       message_json.value());
}

void ReceiverSessionMessenger::OnMessage(const std::string& source_id,
                                         const std::string& message_namespace,
                                         const std::string& message) {
  if (message_namespace != kCastWebrtcNamespace &&
      message_namespace != kCastRemotingNamespace) {
    OSP_DLOG_WARN << "Received message from unknown namespace: "
                  << message_namespace;
    return;
  }

  // If the message is bad JSON, the sender is in a funky state so we
  // report an error.
  ErrorOr<Json::Value> message_body = json::Parse(message);
  if (message_body.is_error()) {
    ReportError(message_body.error());
    return;
  }

  // If the message is valid JSON and we don't understand it, there are two
  // options: (1) it's an unknown type, or (2) the sender filled out the message
  // incorrectly. In the first case we can drop it, it's likely just
  // unsupported. In the second case we might need it, so worth warning the
  // client.
  ErrorOr<SenderMessage> sender_message =
      SenderMessage::Parse(message_body.value());
  if (sender_message.is_error()) {
    ReportError(sender_message.error());
    OSP_DLOG_WARN << "Received an invalid sender message: "
                  << sender_message.error();
    return;
  }

  if (sender_message.value().type == SenderMessage::Type::kOffer ||
      sender_message.value().type == SenderMessage::Type::kGetCapabilities) {
    OSP_VLOG << "Received Message:\n" << message;
  }

  auto it = callbacks_.find(sender_message.value().type);
  if (it == callbacks_.end()) {
    OSP_DLOG_INFO << "Received message without a callback, dropping";
    return;
  }
  it->second(source_id, sender_message.value());
}

void ReceiverSessionMessenger::OnError(Error error) {
  OSP_DLOG_WARN << "Received an error in the session messenger: " << error;
  ReportError(error);
}

}  // namespace cast
}  // namespace openscreen
