// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The testing message pipe is intended to be used for integrating
// sender and receiver sessions (or other consumers of MessagePort).

#ifndef CAST_STREAMING_TESTING_MESSAGE_PIPE_H_
#define CAST_STREAMING_TESTING_MESSAGE_PIPE_H_

#include <string>
#include <utility>
#include <vector>

#include "cast/common/public/message_port.h"
#include "cast/streaming/message_fields.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace openscreen {
namespace cast {

class MessagePipeEnd : public MessagePort {
 public:
  explicit MessagePipeEnd(std::string destination_id)
      : destination_id_(destination_id) {}

  ~MessagePipeEnd() override = default;

  void SetClient(MessagePort::Client& client) override {
    client_ = &client;
    source_id_ = client.source_id();
  }

  void SetOtherEnd(MessagePipeEnd* other_end) {
    ASSERT_FALSE(other_end_);
    other_end_ = other_end;
  }

  void ResetClient() override { client_ = nullptr; }

  void ReceiveMessage(const std::string& sender_id,
                      const std::string& namespace_,
                      const std::string& message) {
    ASSERT_NE(client_, nullptr);
    client_->OnMessage(sender_id, namespace_, message);
  }

  void ReceiveMessage(const std::string& namespace_,
                      const std::string& message) {
    ASSERT_NE(client_, nullptr);
    client_->OnMessage(destination_id_, namespace_, message);
  }

  void PostMessage(const std::string& sender_id,
                   const std::string& message_namespace,
                   const std::string& message) override {
    ASSERT_NE(other_end_, nullptr);
    other_end_->ReceiveMessage(message_namespace, message);
  }

 private:
  std::string source_id_;
  std::string destination_id_;
  MessagePort::Client* client_ = nullptr;
  MessagePipeEnd* other_end_ = nullptr;
};

class MessagePipe {
 public:
  explicit MessagePipe(std::string left_id, std::string right_id)
      : left_id_(std::move(right_id)),
        right_id_(std::move(left_id)),
        left_(left_id_),
        right_(right_id_) {
    left_.SetOtherEnd(&right_);
    right_.SetOtherEnd(&left_);
  }

  // Access the ends of the pipe, which can be used as a standard
  // message port.
  MessagePipeEnd* left() { return &left_; }
  MessagePipeEnd* right() { return &right_; }

 private:
  std::string left_id_;
  std::string right_id_;
  MessagePipeEnd left_;
  MessagePipeEnd right_;
};

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_TESTING_MESSAGE_PIPE_H_
