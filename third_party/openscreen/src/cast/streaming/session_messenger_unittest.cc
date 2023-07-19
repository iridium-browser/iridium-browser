// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/session_messenger.h"

#include <memory>

#include "cast/streaming/testing/message_pipe.h"
#include "cast/streaming/testing/simple_message_port.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {
namespace cast {

using ::testing::ElementsAre;

namespace {

constexpr char kSenderId[] = "sender-12345";
constexpr char kReceiverId[] = "receiver-12345";

// Generally the messages are inlined below, with the exception of the Offer,
// simply because it is massive.
Offer kExampleOffer{
    CastMode::kMirroring,
    {AudioStream{Stream{0,
                        Stream::Type::kAudioSource,
                        2,
                        RtpPayloadType::kAudioOpus,
                        12344442,
                        std::chrono::milliseconds{2000},
                        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                        false,
                        "",
                        48000},
                 AudioCodec::kOpus, 1400}},
    {VideoStream{Stream{1,
                        Stream::Type::kVideoSource,
                        1,
                        RtpPayloadType::kVideoVp8,
                        12344444,
                        std::chrono::milliseconds{2000},
                        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
                        false,
                        "",
                        90000},
                 VideoCodec::kVp8,
                 SimpleFraction{30, 1},
                 3000000,
                 "",
                 "",
                 "",
                 {Resolution{640, 480}},
                 ""}}};

struct SessionMessageStore {
 public:
  SenderSessionMessenger::ReplyCallback GetReplyCallback() {
    return [this](ErrorOr<ReceiverMessage> reply_result) {
      receiver_messages.push_back(std::move(reply_result));
    };
  }

  ReceiverSessionMessenger::RequestCallback GetRequestCallback() {
    return [this](const std::string& sender_id, SenderMessage message) {
      sender_messages.push_back(std::make_pair(sender_id, std::move(message)));
    };
  }

  SessionMessenger::ErrorCallback GetErrorCallback() {
    return [this](Error error) { errors.push_back(std::move(error)); };
  }

  std::vector<std::pair<std::string, SenderMessage>> sender_messages;
  std::vector<ErrorOr<ReceiverMessage>> receiver_messages;
  std::vector<Error> errors;
};
}  // namespace

class SessionMessengerTest : public ::testing::Test {
 public:
  SessionMessengerTest()
      : clock_{Clock::now()},
        task_runner_(&clock_),
        message_store_(),
        pipe_(kSenderId, kReceiverId),
        receiver_messenger_(std::make_unique<ReceiverSessionMessenger>(
            pipe_.right(),
            kReceiverId,
            message_store_.GetErrorCallback())),
        sender_messenger_(std::make_unique<SenderSessionMessenger>(
            pipe_.left(),
            kSenderId,
            kReceiverId,
            message_store_.GetErrorCallback(),
            &task_runner_))

  {}

  void SetUp() override {
    sender_messenger_->SetHandler(ReceiverMessage::Type::kRpc,
                                  message_store_.GetReplyCallback());
    receiver_messenger_->SetHandler(SenderMessage::Type::kOffer,
                                    message_store_.GetRequestCallback());
    receiver_messenger_->SetHandler(SenderMessage::Type::kGetCapabilities,
                                    message_store_.GetRequestCallback());
    receiver_messenger_->SetHandler(SenderMessage::Type::kRpc,
                                    message_store_.GetRequestCallback());
  }

  MessagePipeEnd& sender_pipe_end() { return *pipe_.left(); }

  MessagePipeEnd& receiver_pipe_end() { return *pipe_.right(); }

 protected:
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  SessionMessageStore message_store_;
  MessagePipe pipe_;
  std::unique_ptr<ReceiverSessionMessenger> receiver_messenger_;
  std::unique_ptr<SenderSessionMessenger> sender_messenger_;

  std::vector<Error> receiver_errors_;
  std::vector<Error> sender_errors_;
};

TEST_F(SessionMessengerTest, RpcMessaging) {
  static const std::vector<uint8_t> kSenderMessage{1, 2, 3, 4, 5};
  static const std::vector<uint8_t> kSenderMessageTwo{11, 12, 13};
  static const std::vector<uint8_t> kReceiverResponse{6, 7, 8, 9};
  ASSERT_TRUE(
      sender_messenger_
          ->SendOutboundMessage(SenderMessage{SenderMessage::Type::kRpc, 123,
                                              true /* valid */, kSenderMessage})
          .ok());

  ASSERT_TRUE(sender_messenger_->SendRpcMessage(kSenderMessageTwo).ok());

  ASSERT_EQ(2u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
  EXPECT_EQ(SenderMessage::Type::kRpc,
            message_store_.sender_messages[0].second.type);
  ASSERT_TRUE(message_store_.sender_messages[0].second.valid);
  EXPECT_EQ(kSenderMessage, absl::get<std::vector<uint8_t>>(
                                message_store_.sender_messages[0].second.body));
  EXPECT_EQ(SenderMessage::Type::kRpc,
            message_store_.sender_messages[1].second.type);
  ASSERT_TRUE(message_store_.sender_messages[1].second.valid);
  EXPECT_EQ(kSenderMessageTwo,
            absl::get<std::vector<uint8_t>>(
                message_store_.sender_messages[1].second.body));

  message_store_.sender_messages.clear();
  ASSERT_TRUE(
      receiver_messenger_
          ->SendMessage(kSenderId,
                        ReceiverMessage{ReceiverMessage::Type::kRpc, 123,
                                        true /* valid */, kReceiverResponse})
          .ok());

  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  EXPECT_EQ(ReceiverMessage::Type::kRpc,
            message_store_.receiver_messages[0].value().type);
  EXPECT_TRUE(message_store_.receiver_messages[0].value().valid);
  EXPECT_EQ(kReceiverResponse,
            absl::get<std::vector<uint8_t>>(
                message_store_.receiver_messages[0].value().body));
}

TEST_F(SessionMessengerTest, CapabilitiesMessaging) {
  ASSERT_TRUE(
      sender_messenger_
          ->SendRequest(SenderMessage{SenderMessage::Type::kGetCapabilities,
                                      1337, true /* valid */},
                        ReceiverMessage::Type::kCapabilitiesResponse,
                        message_store_.GetReplyCallback())
          .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
  EXPECT_EQ(SenderMessage::Type::kGetCapabilities,
            message_store_.sender_messages[0].second.type);
  EXPECT_TRUE(message_store_.sender_messages[0].second.valid);

  message_store_.sender_messages.clear();
  ASSERT_TRUE(
      receiver_messenger_
          ->SendMessage(
              kSenderId,
              ReceiverMessage{
                  ReceiverMessage::Type::kCapabilitiesResponse, 1337,
                  true /* valid */,
                  ReceiverCapability{
                      47, {MediaCapability::kAac, MediaCapability::k4k}}})
          .ok());

  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  EXPECT_EQ(ReceiverMessage::Type::kCapabilitiesResponse,
            message_store_.receiver_messages[0].value().type);
  EXPECT_TRUE(message_store_.receiver_messages[0].value().valid);

  const auto& capability = absl::get<ReceiverCapability>(
      message_store_.receiver_messages[0].value().body);
  EXPECT_EQ(47, capability.remoting_version);
  EXPECT_THAT(capability.media_capabilities,
              ElementsAre(MediaCapability::kAac, MediaCapability::k4k));
}

TEST_F(SessionMessengerTest, OfferAnswerMessaging) {
  ASSERT_TRUE(sender_messenger_
                  ->SendRequest(SenderMessage{SenderMessage::Type::kOffer, 42,
                                              true /* valid */, kExampleOffer},
                                ReceiverMessage::Type::kAnswer,
                                message_store_.GetReplyCallback())
                  .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
  EXPECT_EQ(SenderMessage::Type::kOffer,
            message_store_.sender_messages[0].second.type);
  EXPECT_TRUE(message_store_.sender_messages[0].second.valid);
  message_store_.sender_messages.clear();

  EXPECT_TRUE(receiver_messenger_
                  ->SendMessage(
                      kSenderId,
                      ReceiverMessage{
                          ReceiverMessage::Type::kAnswer, 41, true /* valid */,
                          Answer{1234, {0, 1}, {12344443, 12344445}}})
                  .ok());
  // A stale answer (for offer 41) should get ignored:
  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_TRUE(message_store_.receiver_messages.empty());

  ASSERT_TRUE(receiver_messenger_
                  ->SendMessage(
                      kSenderId,
                      ReceiverMessage{
                          ReceiverMessage::Type::kAnswer, 42, true /* valid */,
                          Answer{1234, {0, 1}, {12344443, 12344445}}})
                  .ok());
  EXPECT_TRUE(message_store_.sender_messages.empty());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  EXPECT_EQ(ReceiverMessage::Type::kAnswer,
            message_store_.receiver_messages[0].value().type);
  EXPECT_TRUE(message_store_.receiver_messages[0].value().valid);

  const auto& answer =
      absl::get<Answer>(message_store_.receiver_messages[0].value().body);
  EXPECT_EQ(1234, answer.udp_port);

  EXPECT_THAT(answer.send_indexes, ElementsAre(0, 1));
  EXPECT_THAT(answer.ssrcs, ElementsAre(12344443, 12344445));
}

TEST_F(SessionMessengerTest, OfferAndReceiverError) {
  ASSERT_TRUE(sender_messenger_
                  ->SendRequest(SenderMessage{SenderMessage::Type::kOffer, 42,
                                              true /* valid */, kExampleOffer},
                                ReceiverMessage::Type::kAnswer,
                                message_store_.GetReplyCallback())
                  .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
  EXPECT_EQ(kSenderId, message_store_.sender_messages[0].first);
  EXPECT_EQ(SenderMessage::Type::kOffer,
            message_store_.sender_messages[0].second.type);
  EXPECT_TRUE(message_store_.sender_messages[0].second.valid);
  message_store_.sender_messages.clear();

  EXPECT_TRUE(receiver_messenger_
                  ->SendMessage(
                      kSenderId,
                      ReceiverMessage{
                          ReceiverMessage::Type::kAnswer, 42, false /* valid */,
                          ReceiverError{123, "Something real bad happened"}})
                  .ok());

  EXPECT_TRUE(message_store_.sender_messages.empty());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  EXPECT_EQ(ReceiverMessage::Type::kAnswer,
            message_store_.receiver_messages[0].value().type);
  EXPECT_FALSE(message_store_.receiver_messages[0].value().valid);

  const auto& error = absl::get<ReceiverError>(
      message_store_.receiver_messages[0].value().body);
  EXPECT_EQ(123, error.code);
  EXPECT_EQ("Something real bad happened", error.description);
}

TEST_F(SessionMessengerTest, UnknownSenderMessageTypesDontGetSent) {
  EXPECT_DEATH_IF_SUPPORTED(
      sender_messenger_
          ->SendOutboundMessage(SenderMessage{SenderMessage::Type::kUnknown,
                                              123, true /* valid */})
          .ok(),
      ".*Trying to send an unknown message is a developer error.*");
}

TEST_F(SessionMessengerTest, UnknownReceiverMessageTypesDontGetSent) {
  ASSERT_TRUE(sender_messenger_
                  ->SendRequest(SenderMessage{SenderMessage::Type::kOffer, 42,
                                              true /* valid */, kExampleOffer},
                                ReceiverMessage::Type::kAnswer,
                                message_store_.GetReplyCallback())
                  .ok());

  EXPECT_DEATH_IF_SUPPORTED(
      receiver_messenger_
          ->SendMessage(kSenderId,
                        ReceiverMessage{ReceiverMessage::Type::kUnknown, 3123,
                                        true /* valid */})
          .ok(),
      ".*Trying to send an unknown message is a developer error.*");
}

TEST_F(SessionMessengerTest, ReceiverHandlesUnknownMessageType) {
  receiver_pipe_end().ReceiveMessage(kCastWebrtcNamespace, R"({
    "type": "GET_VIRTUAL_REALITY",
    "seqNum": 31337
  })");
  ASSERT_TRUE(message_store_.errors.empty());
}

TEST_F(SessionMessengerTest, SenderHandlesUnknownMessageType) {
  // The behavior on the sender side is a little more interesting: we
  // test elsewhere that messages with the wrong sequence number are ignored,
  // here if the type is unknown but the message contains a valid sequence
  // number we just treat it as a bad response/same as a timeout.
  ASSERT_TRUE(sender_messenger_
                  ->SendRequest(SenderMessage{SenderMessage::Type::kOffer, 42,
                                              true /* valid */, kExampleOffer},
                                ReceiverMessage::Type::kAnswer,
                                message_store_.GetReplyCallback())
                  .ok());
  sender_pipe_end().ReceiveMessage(kCastWebrtcNamespace, R"({
    "type": "ANSWER_VERSION_2",
    "seqNum": 42
  })");

  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  ASSERT_EQ(ReceiverMessage::Type::kUnknown,
            message_store_.receiver_messages[0].value().type);
  ASSERT_EQ(false, message_store_.receiver_messages[0].value().valid);
}

TEST_F(SessionMessengerTest, SenderHandlesMessageMissingSequenceNumber) {
  ASSERT_TRUE(
      sender_messenger_
          ->SendRequest(SenderMessage{SenderMessage::Type::kGetCapabilities, 42,
                                      true /* valid */},
                        ReceiverMessage::Type::kCapabilitiesResponse,
                        message_store_.GetReplyCallback())
          .ok());
  sender_pipe_end().ReceiveMessage(kCastWebrtcNamespace, R"({
    "capabilities": {
      "keySystems": [],
      "mediaCaps": ["video"]
    },
    "result": "ok",
    "type": "CAPABILITIES_RESPONSE"
  })");

  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
}

TEST_F(SessionMessengerTest, ReceiverCannotSendToEmptyId) {
  const Error error = receiver_messenger_->SendMessage(
      "", ReceiverMessage{ReceiverMessage::Type::kCapabilitiesResponse, 3123,
                          true /* valid */,
                          ReceiverCapability{2, {MediaCapability::kAudio}}});

  EXPECT_EQ(Error::Code::kInitializationFailure, error.code());
}

TEST_F(SessionMessengerTest, ErrorMessageLoggedIfTimeout) {
  ASSERT_TRUE(
      sender_messenger_
          ->SendRequest(SenderMessage{SenderMessage::Type::kGetCapabilities,
                                      3123, true /* valid */},
                        ReceiverMessage::Type::kCapabilitiesResponse,
                        message_store_.GetReplyCallback())
          .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());

  clock_.Advance(std::chrono::seconds(10));
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages[0].is_error());
  EXPECT_EQ(Error::Code::kMessageTimeout,
            message_store_.receiver_messages[0].error().code());
}

// Make sure that we don't SEGFAULT if we cause the messenger to get deleted or
// its replies cleared as part of executing an error callback on timeout.
// See https://issuetracker.google.com/250957657 for more context.
TEST_F(SessionMessengerTest, SenderHandlesTimeoutThatCausesDelete) {
  auto reply_callback = [this, message_cb = message_store_.GetReplyCallback()](
                            ErrorOr<ReceiverMessage> reply_result) mutable {
    message_cb(std::move(reply_result));
    sender_messenger_.reset();
  };

  ASSERT_TRUE(
      sender_messenger_
          ->SendRequest(SenderMessage{SenderMessage::Type::kGetCapabilities,
                                      3123, true /* valid */},
                        ReceiverMessage::Type::kCapabilitiesResponse,
                        std::move(reply_callback))
          .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());

  clock_.Advance(std::chrono::seconds(10));
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages[0].is_error());
  EXPECT_EQ(Error::Code::kMessageTimeout,
            message_store_.receiver_messages[0].error().code());
}

TEST_F(SessionMessengerTest, SenderHandlesMessageThatCausesDelete) {
  auto reply_callback = [this, message_cb = message_store_.GetReplyCallback()](
                            ErrorOr<ReceiverMessage> reply_result) mutable {
    message_cb(std::move(reply_result));
    sender_messenger_.reset();
  };

  ASSERT_TRUE(
      sender_messenger_
          ->SendRequest(SenderMessage{SenderMessage::Type::kGetCapabilities,
                                      3123, true /* valid */},
                        ReceiverMessage::Type::kCapabilitiesResponse,
                        std::move(reply_callback))
          .ok());

  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages.empty());

  clock_.Advance(std::chrono::seconds(10));
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_EQ(1u, message_store_.receiver_messages.size());
  ASSERT_TRUE(message_store_.receiver_messages[0].is_error());
  EXPECT_EQ(Error::Code::kMessageTimeout,
            message_store_.receiver_messages[0].error().code());
}

TEST_F(SessionMessengerTest, RecieverHandlesMessageThatCausesDelete) {
  auto request_callback =
      [this, message_cb = message_store_.GetRequestCallback()](
          const std::string& sender_id, SenderMessage request) mutable {
        message_cb(sender_id, std::move(request));
        receiver_messenger_.reset();
      };

  receiver_messenger_->ResetHandler(SenderMessage::Type::kGetCapabilities);
  receiver_messenger_->SetHandler(SenderMessage::Type::kGetCapabilities,
                                  std::move(request_callback));

  receiver_pipe_end().ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263769,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.sender_messages.size());
}

TEST_F(SessionMessengerTest, ReceiverHandlesMessagesFromMultipleSenders) {
  // The first message should be accepted from any sender.
  receiver_pipe_end().ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263769,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  message_store_.sender_messages.clear();

  // The second message should be happily accepted from another sender because
  // it's a GET_CAPABILITIES call.
  receiver_pipe_end().ReceiveMessage("sender-42", kCastWebrtcNamespace, R"({
        "seqNum": 1234,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  ASSERT_EQ("sender-42", message_store_.sender_messages[0].first);
  message_store_.sender_messages.clear();

  // The third message should also be accepted from the original sender.
  receiver_pipe_end().ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263769,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.sender_messages.size());
}

TEST_F(SessionMessengerTest, SenderRejectsMessageFromWrongReceiver) {
  sender_pipe_end().ReceiveMessage("receiver-31337", kCastWebrtcNamespace, R"({
        "seqNum": 12345,
        "sessionId": 735189,
        "type": "RPC",
        "result": "ok",
        "rpc": "SGVsbG8gZnJvbSB0aGUgQ2FzdCBSZWNlaXZlciE="
      })");

  // The message should just be ignored.
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
}

TEST_F(SessionMessengerTest, ReceiverRejectsMessagesWithoutHandler) {
  SimpleMessagePort port(kReceiverId);
  ReceiverSessionMessenger messenger(&port, kReceiverId,
                                     message_store_.GetErrorCallback());
  messenger.SetHandler(SenderMessage::Type::kGetCapabilities,
                       message_store_.GetRequestCallback());

  // The first message should be accepted since we don't have a set sender_id
  // yet.
  port.ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263769,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_EQ(1u, message_store_.sender_messages.size());
  message_store_.sender_messages.clear();

  // The second message should be rejected since it doesn't have a handler.
  port.ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263770,
        "type": "RPC"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.sender_messages.empty());

  // The third message should be rejected since the handler was removed.
  messenger.ResetHandler(SenderMessage::Type::kGetCapabilities);
  port.ReceiveMessage("sender-31337", kCastWebrtcNamespace, R"({
        "seqNum": 820263771,
        "type": "GET_CAPABILITIES"
      })");
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.sender_messages.empty());
}

TEST_F(SessionMessengerTest, SenderRejectsMessagesWithoutHandler) {
  SimpleMessagePort port(kReceiverId);
  SenderSessionMessenger messenger(&port, kSenderId, kReceiverId,
                                   message_store_.GetErrorCallback(),
                                   &task_runner_);

  port.ReceiveMessage(kReceiverId, kCastWebrtcNamespace, R"({
        "seqNum": 12345,
        "sessionId": 735189,
        "type": "RPC",
        "result": "ok",
        "rpc": "SGVsbG8gZnJvbSB0aGUgQ2FzdCBSZWNlaXZlciE="
      })");

  // The message should just be ignored.
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
}

TEST_F(SessionMessengerTest, UnknownNamespaceMessagesGetDropped) {
  receiver_pipe_end().ReceiveMessage(
      "urn:x-cast:com.google.cast.virtualreality",
      R"({
        "seqNum": 12345,
        "sessionId": 735190,
        "type": "RPC",
        "rpc": "SGVsbG8gZnJvbSB0aGUgQ2FzdCBSZWNlaXZlciE="
      })");

  sender_pipe_end().ReceiveMessage("urn:x-cast:com.google.cast.virtualreality",
                                   R"({
        "seqNum": 12345,
        "sessionId": 735190,
        "type": "RPC",
        "result": "ok",
        "rpc": "SGVsbG8gZnJvbSB0aGUgQ2FzdCBTZW5kZXIh="
      })");

  // The message should just be ignored.
  ASSERT_TRUE(message_store_.errors.empty());
  ASSERT_TRUE(message_store_.sender_messages.empty());
  ASSERT_TRUE(message_store_.receiver_messages.empty());
}

}  // namespace cast
}  // namespace openscreen
