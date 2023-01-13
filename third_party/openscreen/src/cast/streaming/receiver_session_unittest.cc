// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/streaming/receiver_session.h"

#include <utility>

#include "cast/streaming/mock_environment.h"
#include "cast/streaming/receiver.h"
#include "cast/streaming/testing/simple_message_port.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/base/ip_address.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/chrono_helpers.h"
#include "util/json/json_serialization.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

namespace openscreen {
namespace cast {

namespace {

constexpr char kValidOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": [
      {
        "index": 31337,
        "type": "video_source",
        "codecName": "vp9",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088743,
        "maxFrameRate": "60000/1000",
        "timeBase": "1/90000",
        "maxBitRate": 5000000,
        "profile": "main",
        "level": "4",
        "aesKey": "bbf109bf84513b456b13a184453b66ce",
        "aesIvMask": "edaf9e4536e2b66191f560d9c04b2a69",
        "resolutions": [
          {
            "width": 1280,
            "height": 720
          }
        ]
      },
      {
        "index": 31338,
        "type": "video_source",
        "codecName": "vp8",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088745,
        "maxFrameRate": "60000/1000",
        "timeBase": "1/90000",
        "maxBitRate": 5000000,
        "profile": "main",
        "level": "4",
        "aesKey": "040d756791711fd3adb939066e6d8690",
        "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
        "resolutions": [
          {
            "width": 1280,
            "height": 720
          }
        ]
      },
      {
        "index": 31339,
        "type": "video_source",
        "codecName": "hevc",
        "codecParameter": "hev1.1.6.L150.B0",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088746,
        "maxFrameRate": "120",
        "timeBase": "1/90000",
        "maxBitRate": 5000000,
        "aesKey": "040d756791711fd3adb939066e6d8690",
        "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
        "resolutions": [
          {
            "width": 1920,
            "height": 1080
          }
        ]
      },
      {
        "index": 1337,
        "type": "audio_source",
        "codecName": "opus",
        "rtpProfile": "cast",
        "rtpPayloadType": 97,
        "ssrc": 19088747,
        "bitRate": 124000,
        "timeBase": "1/48000",
        "channels": 2,
        "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
        "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
      }
    ]
  }
})";

constexpr char kValidRemotingOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 419,
  "offer": {
    "castMode": "remoting",
    "supportedStreams": [
      {
        "index": 31339,
        "type": "video_source",
        "codecName": "REMOTE_VIDEO",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088745,
        "maxFrameRate": "60000/1000",
        "timeBase": "1/90000",
        "maxBitRate": 5432101,
        "aesKey": "040d756791711fd3adb939066e6d8690",
        "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
        "resolutions": [
          {
            "width": 1920,
            "height":1080
          }
        ]
      },
      {
        "index": 31340,
        "type": "audio_source",
        "codecName": "REMOTE_AUDIO",
        "rtpProfile": "cast",
        "rtpPayloadType": 97,
        "ssrc": 19088747,
        "bitRate": 125000,
        "timeBase": "1/48000",
        "channels": 2,
        "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
        "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
      }
    ]
  }
})";

constexpr char kNoAudioOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": [
      {
        "index": 31338,
        "type": "video_source",
        "codecName": "vp8",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088745,
        "maxFrameRate": "60000/1000",
        "timeBase": "1/90000",
        "maxBitRate": 5000000,
        "profile": "main",
        "level": "4",
        "aesKey": "040d756791711fd3adb939066e6d8690",
        "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
        "resolutions": [
          {
            "width": 1280,
            "height": 720
          }
        ]
      }
    ]
  }
})";

constexpr char kInvalidCodecOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": [
      {
        "index": 31338,
        "type": "video_source",
        "codecName": "vp12",
        "rtpProfile": "cast",
        "rtpPayloadType": 127,
        "ssrc": 19088745,
        "maxFrameRate": "60000/1000",
        "timeBase": "1/90000",
        "maxBitRate": 5000000,
        "profile": "main",
        "level": "4",
        "aesKey": "040d756791711fd3adb939066e6d8690",
        "aesIvMask": "9ff0f022a959150e70a2d05a6c184aed",
        "resolutions": [
          {
            "width": 1280,
            "height": 720
          }
        ]
      }
    ]
  }
})";

constexpr char kNoVideoOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": [
      {
        "index": 1337,
        "type": "audio_source",
        "codecName": "opus",
        "rtpProfile": "cast",
        "rtpPayloadType": 97,
        "ssrc": 19088747,
        "bitRate": 124000,
        "timeBase": "1/48000",
        "channels": 2,
        "aesKey": "51027e4e2347cbcb49d57ef10177aebc",
        "aesIvMask": "7f12a19be62a36c04ae4116caaeff6d1"
      }
    ]
  }
})";

constexpr char kNoAudioOrVideoOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": []
  }
})";

constexpr char kInvalidJsonOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": [
  }
})";

constexpr char kMissingMandatoryFieldOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337
})";

constexpr char kMissingSeqNumOfferMessage[] = R"({
  "type": "OFFER",
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": []
  }
})";

constexpr char kValidJsonInvalidFormatOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337,
  "offer": {
    "castMode": "mirroring",
    "supportedStreams": "anything"
  }
})";

constexpr char kNullJsonOfferMessage[] = R"({
  "type": "OFFER",
  "seqNum": 1337
})";

constexpr char kInvalidSequenceNumberMessage[] = R"({
  "type": "OFFER",
  "seqNum": "not actually a number"
})";

constexpr char kUnknownTypeMessage[] = R"({
  "type": "OFFER_VERSION_2",
  "seqNum": 1337
})";

constexpr char kInvalidTypeMessage[] = R"({
  "type": 39,
  "seqNum": 1337
})";

constexpr char kGetCapabilitiesMessage[] = R"({
  "seqNum": 820263770,
  "type": "GET_CAPABILITIES"
})";

constexpr char kRpcMessage[] = R"({
  "rpc" : "CGQQnBiCGQgSAggMGgIIBg==",
  "seqNum" : 2,
  "type" : "RPC"
})";

class FakeClient : public ReceiverSession::Client {
 public:
  MOCK_METHOD(void,
              OnNegotiated,
              (const ReceiverSession*, ReceiverSession::ConfiguredReceivers),
              (override));
  MOCK_METHOD(void,
              OnRemotingNegotiated,
              (const ReceiverSession*, ReceiverSession::RemotingNegotiation),
              (override));
  MOCK_METHOD(void,
              OnReceiversDestroying,
              (const ReceiverSession*, ReceiversDestroyingReason),
              (override));
  MOCK_METHOD(void, OnError, (const ReceiverSession*, Error error), (override));
  MOCK_METHOD(bool,
              SupportsCodecParameter,
              (const std::string& parameter),
              (override));
};

void ExpectIsErrorAnswer(const std::string& raw_message) {
  ErrorOr<Json::Value> message_or_error = json::Parse(raw_message);
  ASSERT_TRUE(message_or_error.is_value());
  const Json::Value message = std::move(message_or_error.value());
  EXPECT_TRUE(message["answer"].isNull());
  EXPECT_EQ("error", message["result"].asString());
  EXPECT_EQ(1337, message["seqNum"].asInt());
  EXPECT_EQ("ANSWER", message["type"].asString());

  const Json::Value& error = message["error"];
  EXPECT_TRUE(error.isObject());
  EXPECT_GT(error["code"].asInt(), 0);
}

// Returns the parsed and validated ANSWER message for additional field
// validation.
Json::Value ExpectIsValidAnswer(const std::string& raw_message) {
  ErrorOr<Json::Value> message_or_error = json::Parse(raw_message);
  EXPECT_TRUE(message_or_error.is_value());
  Json::Value message = std::move(message_or_error.value());

  EXPECT_EQ("ANSWER", message["type"].asString());
  EXPECT_EQ("ok", message["result"].asString());
  EXPECT_FALSE(message["answer"].isNull());
  EXPECT_TRUE(message["answer"].isObject());
  return message;
}

MATCHER_P(
    NegotiatedWithSender,
    sender_id,
    "Checks that a set of ConfiguredReceivers are from a specific sender") {
  return sender_id == arg.sender_id;
}

}  // namespace

class ReceiverSessionTest : public ::testing::Test {
 public:
  ReceiverSessionTest() : clock_(Clock::time_point{}), task_runner_(&clock_) {}

  std::unique_ptr<MockEnvironment> MakeEnvironment() {
    auto environment = std::make_unique<NiceMock<MockEnvironment>>(
        &FakeClock::now, &task_runner_);
    ON_CALL(*environment, GetBoundLocalEndpoint())
        .WillByDefault(Return(IPEndpoint{{127, 0, 0, 1}, 12345}));
    environment->SetSocketStateForTesting(Environment::SocketState::kReady);
    return environment;
  }

  void SetUp() { SetUpWithConstraints(ReceiverConstraints{}); }

  // Since constraints are constant throughout the life of a session,
  // changing them requires configuring a new session.
  void SetUpWithConstraints(ReceiverConstraints constraints) {
    session_.reset();
    message_port_ = std::make_unique<SimpleMessagePort>("sender-12345");
    environment_ = MakeEnvironment();
    session_ = std::make_unique<ReceiverSession>(&client_, environment_.get(),
                                                 message_port_.get(),
                                                 std::move(constraints));
  }

 protected:
  void AssertGotAnErrorAnswerResponse() {
    const std::vector<std::string>& messages = message_port_->posted_messages();
    ASSERT_EQ(1u, messages.size());

    ExpectIsErrorAnswer(messages[0]);
  }

  StrictMock<FakeClient> client_;
  FakeClock clock_;
  std::unique_ptr<MockEnvironment> environment_;
  std::unique_ptr<SimpleMessagePort> message_port_;
  std::unique_ptr<ReceiverSession> session_;
  FakeTaskRunner task_runner_;
};

TEST_F(ReceiverSessionTest, CanNegotiateWithDefaultConstraints) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::ConfiguredReceivers cr) {
        EXPECT_TRUE(cr.audio_receiver);
        EXPECT_EQ(cr.audio_receiver->config().sender_ssrc, 19088747u);
        EXPECT_EQ(cr.audio_receiver->config().receiver_ssrc, 19088748u);
        EXPECT_EQ(cr.audio_receiver->config().channels, 2);
        EXPECT_EQ(cr.audio_receiver->config().rtp_timebase, 48000);

        // We should have chosen opus
        EXPECT_EQ(cr.audio_config.codec, AudioCodec::kOpus);

        EXPECT_TRUE(cr.video_receiver);
        EXPECT_EQ(cr.video_receiver->config().sender_ssrc, 19088745u);
        EXPECT_EQ(cr.video_receiver->config().receiver_ssrc, 19088746u);
        EXPECT_EQ(cr.video_receiver->config().channels, 1);
        EXPECT_EQ(cr.video_receiver->config().rtp_timebase, 90000);

        // We should have chosen vp8
        EXPECT_EQ(cr.video_config.codec, VideoCodec::kVp8);

        // This should be from the default test sender ID.
        EXPECT_EQ("sender-12345", cr.sender_id);
      });
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kValidOfferMessage);

  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  Json::Value message = ExpectIsValidAnswer(messages[0]);

  EXPECT_EQ("ANSWER", message["type"].asString());
  EXPECT_EQ(1337, message["seqNum"].asInt());
  EXPECT_EQ("ok", message["result"].asString());

  const Json::Value& answer = message["answer"];
  EXPECT_TRUE(answer.isObject());

  // Spot check the answer body fields. We have more in depth testing
  // of answer behavior in answer_messages_unittest, but here we can
  // ensure that the ReceiverSession properly configured the answer.
  EXPECT_EQ(1337, answer["sendIndexes"][0].asInt());
  EXPECT_EQ(31338, answer["sendIndexes"][1].asInt());
  EXPECT_LT(0, answer["udpPort"].asInt());
  EXPECT_GT(65535, answer["udpPort"].asInt());

  // Constraints and display should not be present with no constraints.
  EXPECT_TRUE(answer["constraints"].isNull());
  EXPECT_TRUE(answer["display"].isNull());
}

TEST_F(ReceiverSessionTest, CanNegotiateWithCustomCodecConstraints) {
  ReceiverSession session(
      &client_, environment_.get(), message_port_.get(),
      ReceiverConstraints{{VideoCodec::kVp9}, {AudioCodec::kOpus}});

  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(&session, _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::ConfiguredReceivers cr) {
        EXPECT_TRUE(cr.audio_receiver);
        EXPECT_EQ(cr.audio_receiver->config().sender_ssrc, 19088747u);
        EXPECT_EQ(cr.audio_receiver->config().receiver_ssrc, 19088748u);
        EXPECT_EQ(cr.audio_receiver->config().channels, 2);
        EXPECT_EQ(cr.audio_receiver->config().rtp_timebase, 48000);
        EXPECT_EQ(cr.audio_config.codec, AudioCodec::kOpus);

        EXPECT_TRUE(cr.video_receiver);
        EXPECT_EQ(cr.video_receiver->config().sender_ssrc, 19088743u);
        EXPECT_EQ(cr.video_receiver->config().receiver_ssrc, 19088744u);
        EXPECT_EQ(cr.video_receiver->config().channels, 1);
        EXPECT_EQ(cr.video_receiver->config().rtp_timebase, 90000);
        EXPECT_EQ(cr.video_config.codec, VideoCodec::kVp9);
      });
  EXPECT_CALL(client_, OnReceiversDestroying(
                           &session, ReceiverSession::Client::kEndOfSession));
  message_port_->ReceiveMessage(kValidOfferMessage);
}

TEST_F(ReceiverSessionTest, RejectsStreamWithUnsupportedCodecParameter) {
  ReceiverConstraints constraints({VideoCodec::kHevc}, {AudioCodec::kOpus});
  EXPECT_CALL(client_, SupportsCodecParameter(_)).WillRepeatedly(Return(false));
  ReceiverSession session(&client_, environment_.get(), message_port_.get(),
                          constraints);
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(&session, _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::ConfiguredReceivers cr) {
        EXPECT_FALSE(cr.video_receiver);
      });
  EXPECT_CALL(client_, OnReceiversDestroying(
                           &session, ReceiverSession::Client::kEndOfSession));
  message_port_->ReceiveMessage(kValidOfferMessage);
}

TEST_F(ReceiverSessionTest, AcceptsStreamWithNoCodecParameter) {
  ReceiverConstraints constraints({VideoCodec::kHevc, VideoCodec::kVp9},
                                  {AudioCodec::kOpus});
  EXPECT_CALL(client_, SupportsCodecParameter(_)).WillRepeatedly(Return(false));

  ReceiverSession session(&client_, environment_.get(), message_port_.get(),
                          std::move(constraints));
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(&session, _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::ConfiguredReceivers cr) {
        EXPECT_TRUE(cr.video_receiver);
        EXPECT_EQ(cr.video_config.codec, VideoCodec::kVp9);
      });
  EXPECT_CALL(client_, OnReceiversDestroying(
                           &session, ReceiverSession::Client::kEndOfSession));
  message_port_->ReceiveMessage(kValidOfferMessage);
}

TEST_F(ReceiverSessionTest, AcceptsStreamWithMatchingParameter) {
  ReceiverConstraints constraints({VideoCodec::kHevc}, {AudioCodec::kOpus});
  EXPECT_CALL(client_, SupportsCodecParameter(_))
      .WillRepeatedly(
          [](const std::string& param) { return param == "hev1.1.6.L150.B0"; });

  ReceiverSession session(&client_, environment_.get(), message_port_.get(),
                          std::move(constraints));
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(&session, _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::ConfiguredReceivers cr) {
        EXPECT_TRUE(cr.video_receiver);
        EXPECT_EQ(cr.video_config.codec, VideoCodec::kHevc);
      });
  EXPECT_CALL(client_, OnReceiversDestroying(
                           &session, ReceiverSession::Client::kEndOfSession));
  message_port_->ReceiveMessage(kValidOfferMessage);
}

TEST_F(ReceiverSessionTest, CanNegotiateWithLimits) {
  std::vector<AudioLimits> audio_limits = {
      {false, AudioCodec::kOpus, 48001, 2, 32001, 32002, milliseconds(3001)}};
  std::vector<VideoLimits> video_limits = {{true,
                                            VideoCodec::kVp9,
                                            62208000,
                                            {1920, 1080, {144, 1}},
                                            300000,
                                            90000000,
                                            milliseconds(1000)}};

  auto display = std::make_unique<Display>(
      Display{{640, 480, {60, 1}}, false /* can scale content */});

  ReceiverSession session(&client_, environment_.get(), message_port_.get(),
                          ReceiverConstraints{{VideoCodec::kVp9},
                                              {AudioCodec::kOpus},
                                              std::move(audio_limits),
                                              std::move(video_limits),
                                              std::move(display)});

  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(&session, _));
  EXPECT_CALL(client_, OnReceiversDestroying(
                           &session, ReceiverSession::Client::kEndOfSession));
  message_port_->ReceiveMessage(kValidOfferMessage);

  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  Json::Value message_body = ExpectIsValidAnswer(messages[0]);
  const Json::Value& answer_json = message_body["answer"];
  ASSERT_TRUE(answer_json.isObject()) << messages[0];

  // Constraints and display should be valid with valid constraints.
  ASSERT_FALSE(answer_json["constraints"].isNull());
  ASSERT_FALSE(answer_json["display"].isNull());

  const Json::Value& display_json = answer_json["display"];
  EXPECT_EQ("60", display_json["dimensions"]["frameRate"].asString());
  EXPECT_EQ(640, display_json["dimensions"]["width"].asInt());
  EXPECT_EQ(480, display_json["dimensions"]["height"].asInt());
  EXPECT_EQ("sender", display_json["scaling"].asString());

  const Json::Value& constraints_json = answer_json["constraints"];
  ASSERT_TRUE(constraints_json.isObject());

  const Json::Value& audio = constraints_json["audio"];
  ASSERT_TRUE(audio.isObject());
  EXPECT_EQ(32002, audio["maxBitRate"].asInt());
  EXPECT_EQ(2, audio["maxChannels"].asInt());
  EXPECT_EQ(3001, audio["maxDelay"].asInt());
  EXPECT_EQ(48001, audio["maxSampleRate"].asInt());
  EXPECT_EQ(32001, audio["minBitRate"].asInt());

  const Json::Value& video = constraints_json["video"];
  ASSERT_TRUE(video.isObject());
  EXPECT_EQ(90000000, video["maxBitRate"].asInt());
  EXPECT_EQ(1000, video["maxDelay"].asInt());
  EXPECT_EQ("144", video["maxDimensions"]["frameRate"].asString());
  EXPECT_EQ(1920, video["maxDimensions"]["width"].asInt());
  EXPECT_EQ(1080, video["maxDimensions"]["height"].asInt());
  EXPECT_EQ(300000, video["minBitRate"].asInt());
}

TEST_F(ReceiverSessionTest, HandlesNoValidAudioStream) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kNoAudioOfferMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  EXPECT_EQ(1u, messages.size());
  Json::Value answer = ExpectIsValidAnswer(messages[0])["answer"];

  // Should still select video stream.
  EXPECT_EQ(1u, answer["sendIndexes"].size());
  EXPECT_EQ(31338, answer["sendIndexes"][0].asInt());
  EXPECT_EQ(1u, answer["ssrcs"].size());
  EXPECT_EQ(19088746, answer["ssrcs"][0].asInt());
}

TEST_F(ReceiverSessionTest, HandlesInvalidCodec) {
  // We didn't select any streams, but didn't have any errors either.
  message_port_->ReceiveMessage(kInvalidCodecOfferMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  EXPECT_EQ(1u, messages.size());

  // We should have failed to produce a valid answer message due to not
  // selecting any stream.
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, HandlesNoValidVideoStream) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kNoVideoOfferMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  EXPECT_EQ(1u, messages.size());
  const Json::Value answer = ExpectIsValidAnswer(messages[0])["answer"];

  // Should still select audio stream.
  EXPECT_EQ(1u, answer["sendIndexes"].size());
  EXPECT_EQ(1337, answer["sendIndexes"][0].asInt());
  EXPECT_EQ(1u, answer["ssrcs"].size());
  EXPECT_EQ(19088748, answer["ssrcs"][0].asInt());
}

TEST_F(ReceiverSessionTest, RejectsOfferIfNewOneComesBeforeNegotiationIsDone) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("first-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  // If the socket state is pending we don't process OFFERs yet.
  environment_->SetSocketStateForTesting(Environment::SocketState::kStarting);
  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(0u, message_port_->posted_messages().size());

  // We should process the first OFFER now that the second one has come in.
  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(1u, message_port_->posted_messages().size());
  ExpectIsErrorAnswer(message_port_->posted_messages()[0]);

  environment_->SetSocketStateForTesting(Environment::SocketState::kReady);
  EXPECT_EQ(2u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[1]);
}

TEST_F(ReceiverSessionTest, HandlesRenegotiationFromSameSender) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("first-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kRenegotiated));
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("first-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(1u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[0]);

  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(2u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[1]);
}

TEST_F(ReceiverSessionTest, HandlesRenegotiationFromAnotherSender) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("first-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kRenegotiated));
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("some-other-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(1u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[0]);

  message_port_->ReceiveMessage("some-other-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(2u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[1]);
}

TEST_F(ReceiverSessionTest, HandlesErrorOfferFromAnotherSender) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(),
                                    NegotiatedWithSender("first-sender")));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage("first-sender", kCastWebrtcNamespace,
                                kValidOfferMessage);
  EXPECT_EQ(1u, message_port_->posted_messages().size());
  ExpectIsValidAnswer(message_port_->posted_messages()[0]);

  message_port_->ReceiveMessage("some-other-sender", kCastWebrtcNamespace,
                                kInvalidCodecOfferMessage);
  EXPECT_EQ(2u, message_port_->posted_messages().size());
  ExpectIsErrorAnswer(message_port_->posted_messages()[1]);
}

TEST_F(ReceiverSessionTest, HandlesNoValidStreams) {
  // We shouldn't call OnNegotiated if we failed to negotiate any streams.
  message_port_->ReceiveMessage(kNoAudioOrVideoOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, HandlesMalformedOffer) {
  // Note that unlike when we simply don't select any streams, when the offer
  // is not valid JSON we actually have no way of knowing it's an offer at all,
  // so we call OnError and do not reply with an Answer.
  EXPECT_CALL(client_, OnError(session_.get(), _));
  message_port_->ReceiveMessage(kInvalidJsonOfferMessage);
}

TEST_F(ReceiverSessionTest, HandlesMissingSeqNumInOffer) {
  // If the OFFER is missing a sequence number it gets rejected before being
  // parsed as an OFFER, since the sender expects all messages to come back
  // with a sequence number.
  message_port_->ReceiveMessage(kMissingSeqNumOfferMessage);
}

TEST_F(ReceiverSessionTest, HandlesOfferMissingMandatoryFields) {
  // If the OFFER is missing mandatory fields, we notify the client as well as
  // reply with an error-case Answer.
  EXPECT_CALL(client_, OnError(session_.get(), _));

  message_port_->ReceiveMessage(kMissingMandatoryFieldOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, HandlesImproperlyFormattedOffer) {
  EXPECT_CALL(client_, OnError(session_.get(), _));
  message_port_->ReceiveMessage(kValidJsonInvalidFormatOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, HandlesNullOffer) {
  EXPECT_CALL(client_, OnError(session_.get(), _));
  message_port_->ReceiveMessage(kNullJsonOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, HandlesInvalidSequenceNumber) {
  // We should just discard messages with an invalid sequence number.
  message_port_->ReceiveMessage(kInvalidSequenceNumberMessage);
}

TEST_F(ReceiverSessionTest, HandlesUnknownTypeMessage) {
  // We should just discard messages with an unknown message type.
  message_port_->ReceiveMessage(kUnknownTypeMessage);
}

TEST_F(ReceiverSessionTest, HandlesInvalidTypeMessage) {
  // We should just discard messages with an invalid message type.
  message_port_->ReceiveMessage(kInvalidTypeMessage);
}

TEST_F(ReceiverSessionTest, DoesNotCrashOnMessagePortError) {
  // We should report message port errors.
  EXPECT_CALL(client_, OnError(session_.get(), _));
  message_port_->ReceiveError(Error(Error::Code::kUnknownError));
}

TEST_F(ReceiverSessionTest, NotifiesReceiverDestruction) {
  InSequence s;
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kRenegotiated));
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kNoAudioOfferMessage);
  message_port_->ReceiveMessage(kValidOfferMessage);
}

TEST_F(ReceiverSessionTest, HandlesInvalidAnswer) {
  // Simulate an unbound local endpoint.
  EXPECT_CALL(*environment_, GetBoundLocalEndpoint)
      .WillOnce(Return(IPEndpoint{}));

  message_port_->ReceiveMessage(kValidOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, DelaysAnswerUntilEnvironmentIsReady) {
  environment_->SetSocketStateForTesting(Environment::SocketState::kStarting);

  // We should not have sent an answer yet--the UDP socket is not ready.
  message_port_->ReceiveMessage(kValidOfferMessage);
  ASSERT_TRUE(message_port_->posted_messages().empty());

  // Simulate the environment calling back into us with the socket being ready.
  // state() will not be called again--we just need to get the bind event.
  EXPECT_CALL(*environment_, GetBoundLocalEndpoint())
      .WillOnce(Return(IPEndpoint{{10, 0, 0, 2}, 4567}));
  EXPECT_CALL(client_, OnNegotiated(session_.get(), _));
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));
  session_->OnSocketReady();
  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  // We should have set the UDP port based on the ready socket value.
  const Json::Value& message_body = ExpectIsValidAnswer(messages[0]);
  EXPECT_EQ(4567, message_body["answer"]["udpPort"].asInt());
}

TEST_F(ReceiverSessionTest,
       ReturnsErrorAnswerIfEnvironmentIsAlreadyInvalidated) {
  EXPECT_CALL(client_, OnError(session_.get(), _));
  environment_->SetSocketStateForTesting(Environment::SocketState::kInvalid);

  // If the environment is already in a bad state, we can respond immediately.
  message_port_->ReceiveMessage(kValidOfferMessage);
  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, ReturnsErrorAnswerIfEnvironmentIsInvalidated) {
  EXPECT_CALL(client_, OnError(session_.get(), _));
  environment_->SetSocketStateForTesting(Environment::SocketState::kStarting);

  // We should not have sent an answer yet--the environment is not ready.
  message_port_->ReceiveMessage(kValidOfferMessage);
  ASSERT_TRUE(message_port_->posted_messages().empty());

  // Simulate the environment calling back into us with invalidation.
  environment_->SetSocketStateForTesting(Environment::SocketState::kInvalid);

  AssertGotAnErrorAnswerResponse();
}

TEST_F(ReceiverSessionTest, ReturnsErrorCapabilitiesIfRemotingDisabled) {
  message_port_->ReceiveMessage(kGetCapabilitiesMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  // We should have an error response.
  ErrorOr<Json::Value> message_body = json::Parse(messages[0]);
  EXPECT_TRUE(message_body.is_value());
  EXPECT_EQ("CAPABILITIES_RESPONSE", message_body.value()["type"].asString());
  EXPECT_EQ("error", message_body.value()["result"].asString());
}

TEST_F(ReceiverSessionTest, ReturnsCapabilitiesWithRemotingDefaults) {
  ReceiverConstraints constraints;
  constraints.remoting = std::make_unique<RemotingConstraints>();

  SetUpWithConstraints(std::move(constraints));
  message_port_->ReceiveMessage(kGetCapabilitiesMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  const ErrorOr<Json::Value> message_body = json::Parse(messages[0]);
  EXPECT_TRUE(message_body.is_value());
  EXPECT_EQ("CAPABILITIES_RESPONSE", message_body.value()["type"].asString());
  EXPECT_EQ("ok", message_body.value()["result"].asString());
  const ReceiverCapability response =
      ReceiverCapability::Parse(message_body.value()["capabilities"]).value();

  EXPECT_THAT(
      response.media_capabilities,
      testing::ElementsAre(MediaCapability::kOpus, MediaCapability::kAac,
                           MediaCapability::kVp8, MediaCapability::kH264));
}

TEST_F(ReceiverSessionTest, ReturnsCapabilitiesWithRemotingConstraints) {
  ReceiverConstraints constraints;
  constraints.video_codecs = {VideoCodec::kH264};
  constraints.remoting = std::make_unique<RemotingConstraints>();
  constraints.remoting->supports_chrome_audio_codecs = true;
  constraints.remoting->supports_4k = true;

  SetUpWithConstraints(std::move(constraints));
  message_port_->ReceiveMessage(kGetCapabilitiesMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  ASSERT_EQ(1u, messages.size());

  const ErrorOr<Json::Value> message_body = json::Parse(messages[0]);
  EXPECT_TRUE(message_body.is_value());
  EXPECT_EQ("CAPABILITIES_RESPONSE", message_body.value()["type"].asString());
  EXPECT_EQ("ok", message_body.value()["result"].asString());
  const ReceiverCapability response =
      ReceiverCapability::Parse(message_body.value()["capabilities"]).value();

  EXPECT_THAT(
      response.media_capabilities,
      testing::ElementsAre(MediaCapability::kOpus, MediaCapability::kAac,
                           MediaCapability::kH264, MediaCapability::kAudio,
                           MediaCapability::k4k));
}

TEST_F(ReceiverSessionTest, CanNegotiateRemoting) {
  ReceiverConstraints constraints;
  constraints.remoting = std::make_unique<RemotingConstraints>();
  constraints.remoting->supports_chrome_audio_codecs = true;
  constraints.remoting->supports_4k = true;
  SetUpWithConstraints(std::move(constraints));

  InSequence s;
  EXPECT_CALL(client_, OnRemotingNegotiated(session_.get(), _))
      .WillOnce([](const ReceiverSession* session_,
                   ReceiverSession::RemotingNegotiation negotiation) {
        const ReceiverSession::ConfiguredReceivers& cr = negotiation.receivers;
        EXPECT_TRUE(cr.audio_receiver);
        EXPECT_EQ(cr.audio_receiver->config().sender_ssrc, 19088747u);
        EXPECT_EQ(cr.audio_receiver->config().receiver_ssrc, 19088748u);
        EXPECT_EQ(cr.audio_receiver->config().channels, 2);
        EXPECT_EQ(cr.audio_receiver->config().rtp_timebase, 48000);
        EXPECT_EQ(cr.audio_config.codec, AudioCodec::kNotSpecified);

        EXPECT_TRUE(cr.video_receiver);
        EXPECT_EQ(cr.video_receiver->config().sender_ssrc, 19088745u);
        EXPECT_EQ(cr.video_receiver->config().receiver_ssrc, 19088746u);
        EXPECT_EQ(cr.video_receiver->config().channels, 1);
        EXPECT_EQ(cr.video_receiver->config().rtp_timebase, 90000);
        EXPECT_EQ(cr.video_config.codec, VideoCodec::kNotSpecified);
      });
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kValidRemotingOfferMessage);
}

TEST_F(ReceiverSessionTest, HandlesRpcMessage) {
  ReceiverConstraints constraints;
  constraints.remoting = std::make_unique<RemotingConstraints>();
  constraints.remoting->supports_chrome_audio_codecs = true;
  constraints.remoting->supports_4k = true;
  SetUpWithConstraints(std::move(constraints));

  message_port_->ReceiveMessage(kRpcMessage);
  const std::vector<std::string>& messages = message_port_->posted_messages();
  // Nothing should happen yet, the session doesn't have a messenger.
  ASSERT_EQ(0u, messages.size());

  // We don't need to fully test that the subscription model on the RpcMessenger
  // works, but we do want to test that the ReceiverSession has properly wired
  // the RpcMessenger up to the backing SessionMessenger and can properly
  // handle received RPC messages.
  InSequence s;
  bool received_initialize_message = false;
  EXPECT_CALL(client_, OnRemotingNegotiated(session_.get(), _))
      .WillOnce([this, &received_initialize_message](
                    const ReceiverSession* session_,
                    ReceiverSession::RemotingNegotiation negotiation) mutable {
        negotiation.messenger->RegisterMessageReceiverCallback(
            100, [&received_initialize_message](
                     std::unique_ptr<RpcMessage> message) mutable {
              ASSERT_EQ(100, message->handle());
              ASSERT_EQ(RpcMessage::RPC_DS_INITIALIZE_CALLBACK,
                        message->proc());
              ASSERT_EQ(0, message->integer_value());
              received_initialize_message = true;
            });

        message_port_->ReceiveMessage(kRpcMessage);
      });
  EXPECT_CALL(client_,
              OnReceiversDestroying(session_.get(),
                                    ReceiverSession::Client::kEndOfSession));

  message_port_->ReceiveMessage(kValidRemotingOfferMessage);
  ASSERT_TRUE(received_initialize_message);
}

}  // namespace cast
}  // namespace openscreen
