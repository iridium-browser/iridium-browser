// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/rpc_messenger.h"

#include <memory>
#include <string>
#include <utility>

#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

namespace {

std::ostream& operator<<(std::ostream& out, const RpcMessage& message) {
  out << "handle=" << message.handle() << ", proc=" << message.proc();
  switch (message.rpc_oneof_case()) {
    case RpcMessage::kIntegerValue:
      return out << ", integer_value=" << message.integer_value();
    case RpcMessage::kInteger64Value:
      return out << ", integer64_value=" << message.integer64_value();
    case RpcMessage::kDoubleValue:
      return out << ", double_value=" << message.double_value();
    case RpcMessage::kBooleanValue:
      return out << ", boolean_value=" << message.boolean_value();
    case RpcMessage::kStringValue:
      return out << ", string_value=" << message.string_value();
    default:
      return out << ", rpc_oneof=" << message.rpc_oneof_case();
  }

  OSP_NOTREACHED();
}

}  // namespace

constexpr RpcMessenger::Handle RpcMessenger::kInvalidHandle;
constexpr RpcMessenger::Handle RpcMessenger::kAcquireRendererHandle;
constexpr RpcMessenger::Handle RpcMessenger::kAcquireDemuxerHandle;
constexpr RpcMessenger::Handle RpcMessenger::kFirstHandle;

RpcMessenger::RpcMessenger(SendMessageCallback send_message_cb)
    : next_handle_(kFirstHandle),
      send_message_cb_(std::move(send_message_cb)) {}

RpcMessenger::~RpcMessenger() {
  receive_callbacks_.clear();
}

RpcMessenger::Handle RpcMessenger::GetUniqueHandle() {
  return next_handle_++;
}

void RpcMessenger::RegisterMessageReceiverCallback(
    RpcMessenger::Handle handle,
    ReceiveMessageCallback callback) {
  OSP_DCHECK(receive_callbacks_.find(handle) == receive_callbacks_.end())
      << "must deregister before re-registering";
  receive_callbacks_.emplace_back(handle, std::move(callback));
}

void RpcMessenger::UnregisterMessageReceiverCallback(RpcMessenger::Handle handle) {
  receive_callbacks_.erase_key(handle);
}

void RpcMessenger::ProcessMessageFromRemote(const uint8_t* message,
                                         std::size_t message_len) {
  auto rpc = std::make_unique<RpcMessage>();
  if (!rpc->ParseFromArray(message, message_len)) {
    OSP_DLOG_WARN << "Failed to parse RPC message from remote: \"" << message
                  << "\"";
    return;
  }
  ProcessMessageFromRemote(std::move(rpc));
}

void RpcMessenger::ProcessMessageFromRemote(std::unique_ptr<RpcMessage> message) {
  const auto entry = receive_callbacks_.find(message->handle());
  if (entry == receive_callbacks_.end()) {
    OSP_VLOG << "Dropping message due to unregistered handle: "
             << message->handle();
    return;
  }
  entry->second(std::move(message));
}

void RpcMessenger::SendMessageToRemote(const RpcMessage& rpc) {
  OSP_VLOG << "Sending RPC message: " << rpc;
  std::vector<uint8_t> message(rpc.ByteSizeLong());
  rpc.SerializeToArray(message.data(), message.size());
  send_message_cb_(std::move(message));
}

bool RpcMessenger::IsRegisteredForTesting(RpcMessenger::Handle handle) {
  return receive_callbacks_.find(handle) != receive_callbacks_.end();
}

WeakPtr<RpcMessenger> RpcMessenger::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace cast
}  // namespace openscreen
