// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/rpc_messenger.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cast/streaming/remoting.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::Return;

namespace openscreen {
namespace cast {
namespace {

class FakeMessenger {
 public:
  void OnReceivedRpc(std::unique_ptr<RpcMessage> message) {
    received_rpc_ = std::move(message);
    received_count_++;
  }

  void OnSentRpc(const std::vector<uint8_t>& message) {
    EXPECT_TRUE(sent_rpc_.ParseFromArray(message.data(), message.size()));
    sent_count_++;
  }

  int received_count() const { return received_count_; }
  const RpcMessage& received_rpc() const { return *received_rpc_; }

  int sent_count() const { return sent_count_; }
  const RpcMessage& sent_rpc() const { return sent_rpc_; }

  void set_handle(RpcMessenger::Handle handle) { handle_ = handle; }
  RpcMessenger::Handle handle() { return handle_; }

 private:
  std::unique_ptr<RpcMessage> received_rpc_;
  int received_count_ = 0;

  RpcMessage sent_rpc_;
  int sent_count_ = 0;

  RpcMessenger::Handle handle_ = -1;
};

}  // namespace

class RpcMessengerTest : public testing::Test {
 protected:
  void SetUp() override {
    fake_messenger_ = std::make_unique<FakeMessenger>();
    ASSERT_FALSE(fake_messenger_->received_count());

    rpc_messenger_ = std::make_unique<RpcMessenger>(
        [p = fake_messenger_.get()](std::vector<uint8_t> message) {
          p->OnSentRpc(message);
        });

    const auto handle = rpc_messenger_->GetUniqueHandle();
    fake_messenger_->set_handle(handle);
    rpc_messenger_->RegisterMessageReceiverCallback(
        handle,
        [p = fake_messenger_.get()](std::unique_ptr<RpcMessage> message) {
          p->OnReceivedRpc(std::move(message));
        });
  }

  void ProcessMessage(const RpcMessage& rpc) {
    std::vector<uint8_t> message(rpc.ByteSizeLong());
    rpc.SerializeToArray(message.data(), message.size());
    rpc_messenger_->ProcessMessageFromRemote(message.data(), message.size());
  }

  std::unique_ptr<FakeMessenger> fake_messenger_;
  std::unique_ptr<RpcMessenger> rpc_messenger_;
};

TEST_F(RpcMessengerTest, TestProcessMessageFromRemoteRegistered) {
  RpcMessage rpc;
  rpc.set_handle(fake_messenger_->handle());
  ProcessMessage(rpc);
  ASSERT_EQ(1, fake_messenger_->received_count());
}

TEST_F(RpcMessengerTest, TestProcessMessageFromRemoteUnregistered) {
  RpcMessage rpc;
  rpc_messenger_->UnregisterMessageReceiverCallback(fake_messenger_->handle());
  ProcessMessage(rpc);
  ASSERT_EQ(0, fake_messenger_->received_count());
}

TEST_F(RpcMessengerTest, CanSendMultipleMessages) {
  for (int i = 0; i < 10; ++i) {
    rpc_messenger_->SendMessageToRemote(RpcMessage{});
  }
  EXPECT_EQ(10, fake_messenger_->sent_count());
}

TEST_F(RpcMessengerTest, SendMessageCallback) {
  // Send message for RPC messenger to process.
  RpcMessage sent_rpc;
  sent_rpc.set_handle(fake_messenger_->handle());
  sent_rpc.set_proc(RpcMessage::RPC_R_SETVOLUME);
  sent_rpc.set_double_value(2.3);
  rpc_messenger_->SendMessageToRemote(sent_rpc);

  // Check if received message is identical to the one sent earlier.
  ASSERT_EQ(1, fake_messenger_->sent_count());
  const RpcMessage& message = fake_messenger_->sent_rpc();
  ASSERT_EQ(fake_messenger_->handle(), message.handle());
  ASSERT_EQ(RpcMessage::RPC_R_SETVOLUME, message.proc());
  ASSERT_EQ(2.3, message.double_value());
}

TEST_F(RpcMessengerTest, ProcessMessageWithRegisteredHandle) {
  // Send message for RPC messenger to process.
  RpcMessage sent_rpc;
  sent_rpc.set_handle(fake_messenger_->handle());
  sent_rpc.set_proc(RpcMessage::RPC_DS_INITIALIZE);
  sent_rpc.set_integer_value(4004);
  ProcessMessage(sent_rpc);

  // Checks if received message is identical to the one sent earlier.
  ASSERT_EQ(1, fake_messenger_->received_count());
  const RpcMessage& received_rpc = fake_messenger_->received_rpc();
  ASSERT_EQ(fake_messenger_->handle(), received_rpc.handle());
  ASSERT_EQ(RpcMessage::RPC_DS_INITIALIZE, received_rpc.proc());
  ASSERT_EQ(4004, received_rpc.integer_value());
}

TEST_F(RpcMessengerTest, ProcessMessageWithUnregisteredHandle) {
  // Send message for RPC messenger to process.
  RpcMessage sent_rpc;
  RpcMessenger::Handle different_handle = fake_messenger_->handle() + 1;
  sent_rpc.set_handle(different_handle);
  sent_rpc.set_proc(RpcMessage::RPC_R_SETVOLUME);
  sent_rpc.set_double_value(4.5);
  ProcessMessage(sent_rpc);

  // We shouldn't have gotten the message since the handle is different.
  ASSERT_EQ(0, fake_messenger_->received_count());
}

TEST_F(RpcMessengerTest, Registration) {
  const auto handle = fake_messenger_->handle();
  ASSERT_TRUE(rpc_messenger_->IsRegisteredForTesting(handle));

  rpc_messenger_->UnregisterMessageReceiverCallback(handle);
  ASSERT_FALSE(rpc_messenger_->IsRegisteredForTesting(handle));
}

}  // namespace cast
}  // namespace openscreen
