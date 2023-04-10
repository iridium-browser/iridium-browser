// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/receiver/application_agent.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "cast/common/channel/message_util.h"
#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/public/message_port.h"
#include "cast/receiver/channel/static_credentials.h"
#include "cast/receiver/channel/testing/device_auth_test_helpers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "json/writer.h"  // Included to teach gtest how to pretty-print.
#include "platform/api/time.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/paths.h"
#include "testing/util/read_file.h"
#include "util/json/json_serialization.h"

namespace openscreen {
namespace cast {
namespace {

using ::cast::channel::CastMessage;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Ne;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Sequence;
using ::testing::StrEq;
using ::testing::StrictMock;

// Returns the location of certificate and auth challenge data files for cast
// receiver tests.
std::string GetTestDataSubdir() {
  return GetTestDataPath() + "cast/receiver/channel";
}

class TestCredentialsProvider final
    : public DeviceAuthNamespaceHandler::CredentialsProvider {
 public:
  TestCredentialsProvider() {
    const std::string dir = GetTestDataSubdir();
    InitStaticCredentialsFromFiles(
        &creds_, nullptr, nullptr, dir + "/device_key.pem",
        dir + "/device_chain.pem", dir + "/device_tls.pem");
  }

  absl::Span<const uint8_t> GetCurrentTlsCertAsDer() final {
    return absl::Span<uint8_t>(creds_.tls_cert_der);
  }
  const DeviceCredentials& GetCurrentDeviceCredentials() final {
    return creds_.device_creds;
  }

 private:
  StaticCredentialsProvider creds_;
};

CastMessage TestAuthChallengeMessage() {
  CastMessage message;
  const auto result = message.ParseFromString(
      ReadEntireFileToString(GetTestDataSubdir() + "/auth_challenge.pb"));
  OSP_CHECK(result);
  return message;
}

class FakeApplication : public ApplicationAgent::Application,
                        public MessagePort::Client {
 public:
  FakeApplication(const char* app_id, const char* display_name)
      : app_ids_({app_id}), display_name_(display_name) {
    OSP_CHECK(app_ids_.front().size() == 8);
  }

  // These are called at the end of the Launch() and Stop() methods for
  // confirming those methods were called.
  MOCK_METHOD(void, DidLaunch, (Json::Value params, MessagePort* port), ());
  MOCK_METHOD(void, DidStop, (), ());

  // MessagePort::Client overrides.
  MOCK_METHOD(void,
              OnMessage,
              (const std::string& source_sender_id,
               const std::string& message_namespace,
               const std::string& message),
              (override));
  MOCK_METHOD(void, OnError, (Error error), (override));
  const std::string& source_id() override { return GetStringifiedSessionId(); }

  const std::vector<std::string>& GetAppIds() const override {
    return app_ids_;
  }

  bool Launch(const std::string& app_id,
              const Json::Value& app_params,
              MessagePort* message_port) override {
    EXPECT_EQ(GetAppIds().front(), app_id);
    EXPECT_TRUE(message_port);
    EXPECT_FALSE(is_launched_);
    ++session_id_;
    is_launched_ = true;
    DidLaunch(app_params, message_port);
    return true;
  }

  std::string GetSessionId() override { return GetStringifiedSessionId(); }

  const std::string& GetStringifiedSessionId() {
    // In these tests, the session ID can change at any time.
    if (is_launched_) {
      std::ostringstream oss;
      oss << GetAppIds().front() << "-9ABC-DEF0-1234-";
      oss << std::setfill('0') << std::hex << std::setw(12) << session_id_;
      stringified_session_id_ = oss.str();
    }
    return stringified_session_id_;
  }

  std::string GetDisplayName() override { return display_name_; }

  std::vector<std::string> GetSupportedNamespaces() override {
    return namespaces_;
  }
  void SetSupportedNamespaces(std::vector<std::string> the_namespaces) {
    namespaces_ = std::move(the_namespaces);
  }

  void Stop() override {
    EXPECT_TRUE(is_launched_);
    is_launched_ = false;
    DidStop();
  }

 private:
  const std::vector<std::string> app_ids_;
  const std::string display_name_;

  std::vector<std::string> namespaces_;

  int session_id_ = 0;
  std::string stringified_session_id_;
  bool is_launched_ = false;
};

class ApplicationAgentTest : public ::testing::Test {
 public:
  ApplicationAgentTest() {
    EXPECT_CALL(idle_app_, DidLaunch(_, NotNull()));
    agent_.RegisterApplication(&idle_app_, true);
    Mock::VerifyAndClearExpectations(&idle_app_);

    ConnectAndDoAuth();
  }

  ~ApplicationAgentTest() override { EXPECT_CALL(idle_app_, DidStop()); }

  ApplicationAgent* agent() { return &agent_; }
  StrictMock<FakeApplication>* idle_app() { return &idle_app_; }

  MockCastSocketClient* sender_inbound() {
    return &socket_pair_.mock_peer_client;
  }
  CastSocket* sender_outbound() { return socket_pair_.peer_socket.get(); }

  // Examines the |message| for the correct source/destination transport IDs and
  // namespace, confirms there is JSON in the payload, and returns parsed JSON
  // (or an empty object if the parse fails).
  static Json::Value ValidateAndParseMessage(const CastMessage& message,
                                             const std::string& from,
                                             const std::string& to,
                                             const std::string& the_namespace) {
    EXPECT_EQ(from, message.source_id());
    EXPECT_EQ(to, message.destination_id());
    EXPECT_EQ(the_namespace, message.namespace_());
    EXPECT_EQ(::cast::channel::CastMessage_PayloadType_STRING,
              message.payload_type());
    EXPECT_FALSE(message.payload_utf8().empty());
    ErrorOr<Json::Value> parsed = json::Parse(message.payload_utf8());
    return parsed.value(Json::Value(Json::objectValue));
  }

  // Constructs a CastMessage proto for sending via the CastSocket::Send() API.
  static CastMessage MakeCastMessage(const std::string& source_id,
                                     const std::string& destination_id,
                                     const std::string& the_namespace,
                                     const std::string& json) {
    CastMessage message = MakeSimpleUTF8Message(the_namespace, json);
    message.set_source_id(source_id);
    message.set_destination_id(destination_id);
    return message;
  }

 private:
  // Walk through all the steps to establish a network connection to the
  // ApplicationAgent, and test the plumbing for the auth challenge/reply.
  void ConnectAndDoAuth() {
    static_cast<ReceiverSocketFactory::Client*>(&agent_)->OnConnected(
        nullptr, socket_pair_.local_endpoint, std::move(socket_pair_.socket));

    // The remote will send the auth challenge message and get a reply.
    EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
        .WillOnce(Invoke([](CastSocket*, CastMessage message) {
          EXPECT_EQ(kAuthNamespace, message.namespace_());
          EXPECT_FALSE(message.payload_binary().empty());
        }));
    const auto result = sender_outbound()->Send(TestAuthChallengeMessage());
    ASSERT_TRUE(result.ok()) << result;
    Mock::VerifyAndClearExpectations(sender_inbound());
  }

  void TearDown() override {
    // The ApplicationAgent should send a final "no apps running"
    // RECEIVER_STATUS broadcast to the sender at destruction time.
    EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
        .WillOnce(Invoke([](CastSocket*, CastMessage message) {
          constexpr char kExpectedJson[] = R"({
            "requestId":0,
            "type":"RECEIVER_STATUS",
            "status":{
              "userEq":{},
              "volume":{
                "controlType":"attenuation",
                "level":1.0,
                "muted":false,
                "stepInterval":0.05
              }
            }
          })";
          const Json::Value payload = ValidateAndParseMessage(
              message, kPlatformReceiverId, kBroadcastId, kReceiverNamespace);
          EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
        }));
  }

  FakeClock clock_{Clock::time_point() + std::chrono::hours(1)};
  FakeTaskRunner task_runner_{&clock_};
  FakeCastSocketPair socket_pair_;
  StrictMock<FakeApplication> idle_app_{"E8C28D3C", "Backdrop"};
  TestCredentialsProvider creds_;
  ApplicationAgent agent_{&task_runner_, &creds_};
};

TEST_F(ApplicationAgentTest, JustConnectsWithoutDoingAnything) {}

TEST_F(ApplicationAgentTest, IgnoresGarbageMessages) {
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _)).Times(0);

  const char* kGarbageStrings[] = {
      "",
      R"(naked text)",
      R"("")",
      R"(123)",
      R"("just a string")",
      R"([])",
      R"({})",
      R"({"type":"GET_STATUS"})",  // Note: Missing requestId.
  };
  for (const char* some_string : kGarbageStrings) {
    const auto result = sender_outbound()->Send(
        MakeCastMessage(kPlatformSenderId, kPlatformReceiverId,
                        kReceiverNamespace, some_string));
    ASSERT_TRUE(result.ok()) << result;
  }
}

TEST_F(ApplicationAgentTest, HandlesInvalidCommands) {
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        constexpr char kExpectedJson[] = R"({
          "requestId":3,
          "type":"INVALID_REQUEST",
          "reason":"INVALID_COMMAND"
        })";
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  auto result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":3,
        "type":"FINISH_Q3_OKRS_BY_END_OF_Q3"
      })"));
  ASSERT_TRUE(result.ok()) << result;
}

TEST_F(ApplicationAgentTest, HandlesPings) {
  constexpr int kNumPings = 3;

  int num_pongs = 0;
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .Times(kNumPings)
      .WillRepeatedly(Invoke([&num_pongs](CastSocket*, CastMessage message) {
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kHeartbeatNamespace);
        EXPECT_EQ(json::Parse(R"({"type":"PONG"})").value(), payload);
        ++num_pongs;
      }));

  const CastMessage message =
      MakeCastMessage(kPlatformSenderId, kPlatformReceiverId,
                      kHeartbeatNamespace, R"({"type":"PING"})");
  for (int i = 0; i < kNumPings; ++i) {
    const auto result = sender_outbound()->Send(message);
    ASSERT_TRUE(result.ok()) << result;
  }
  EXPECT_EQ(kNumPings, num_pongs);
}

TEST_F(ApplicationAgentTest, HandlesGetAppAvailability) {
  // Send the request before any apps have been registered. Expect an
  // "unavailable" response.
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        constexpr char kExpectedJson[] = R"({
          "requestId":548,
          "responseType":"GET_APP_AVAILABILITY",
          "availability":{"1A2B3C4D":"APP_UNAVAILABLE"}
        })";
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  auto result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":548,
        "type":"GET_APP_AVAILABILITY",
        "appId":["1A2B3C4D"]
      })"));
  ASSERT_TRUE(result.ok()) << result;

  // Register an application.
  FakeApplication some_app("1A2B3C4D", "Something Doer");
  agent()->RegisterApplication(&some_app);

  // Send another request, and expect the application to be available.
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        constexpr char kExpectedJson[] = R"({
          "requestId":549,
          "responseType":"GET_APP_AVAILABILITY",
          "availability":{"1A2B3C4D":"APP_AVAILABLE"}
        })";
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":549,
        "type":"GET_APP_AVAILABILITY",
        "appId":["1A2B3C4D"]
      })"));
  ASSERT_TRUE(result.ok()) << result;

  agent()->UnregisterApplication(&some_app);
}

TEST_F(ApplicationAgentTest, HandlesGetStatus) {
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        constexpr char kExpectedJson[] = R"({
          "requestId":123,
          "type":"RECEIVER_STATUS",
          "status":{
            "applications":[
              {
                // NOTE: These IDs and the displayName come from |idle_app_|.
                "sessionId":"E8C28D3C-9ABC-DEF0-1234-000000000001",
                "appId":"E8C28D3C",
                "universalAppId":"E8C28D3C",
                "displayName":"Backdrop",
                "isIdleScreen":true,
                "launchedFromCloud":false,
                "namespaces":[]
              }
            ],
            "userEq":{},
            "volume":{
              "controlType":"attenuation",
              "level":1.0,
              "muted":false,
              "stepInterval":0.05
            }
          }
        })";
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  auto result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":123,
        "type":"GET_STATUS"
      })"));
  ASSERT_TRUE(result.ok()) << result;
}

TEST_F(ApplicationAgentTest, FailsLaunchRequestWithBadAppID) {
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        constexpr char kExpectedJson[] = R"({
          "requestId":1,
          "type":"LAUNCH_ERROR",
          "reason":"NOT_FOUND"
        })";
        const Json::Value payload =
            ValidateAndParseMessage(message, kPlatformReceiverId,
                                    kPlatformSenderId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  auto launch_result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":1,
        "type":"LAUNCH",
        "appId":"DEADBEEF"
      })"));
  ASSERT_TRUE(launch_result.ok()) << launch_result;
}

TEST_F(ApplicationAgentTest, LaunchesApp_PassesMessages_ThenStopsApp) {
  StrictMock<FakeApplication> some_app("1A2B3C4D", "Something Doer");
  constexpr char kAppNamespace[] = "urn:x-cast:com.google.cast.something";
  some_app.SetSupportedNamespaces({std::string(kAppNamespace)});
  agent()->RegisterApplication(&some_app);

  // Phase 1: Sender sends a LAUNCH request, which causes the idle app to stop
  // and the receiver app to launch. The receiver (ApplicationAgent) broadcasts
  // a RECEIVER_STATUS to indicate the app is running; but the receiver app
  // should not get a copy of that.
  Sequence phase1;
  MessagePort* port_for_app = nullptr;
  EXPECT_CALL(*idle_app(), DidStop()).InSequence(phase1);
  EXPECT_CALL(some_app, DidLaunch(_, NotNull()))
      .InSequence(phase1)
      .WillOnce(Invoke([&](Json::Value params, MessagePort* port) {
        EXPECT_EQ(json::Parse(R"({"a":1,"b":2})").value(), params);
        port_for_app = port;
        port->SetClient(some_app);
      }));
  const std::string kRunningAppReceiverStatus = R"({
      "requestId":0,  // Note: 0 for broadcast (no requestor).
      "type":"RECEIVER_STATUS",
      "status":{
        "applications":[
          {
            // NOTE: These IDs and the displayName come from |some_app|.
            "transportId":"1A2B3C4D-9ABC-DEF0-1234-000000000001",
            "sessionId":"1A2B3C4D-9ABC-DEF0-1234-000000000001",
            "appId":"1A2B3C4D",
            "universalAppId":"1A2B3C4D",
            "displayName":"Something Doer",
            "isIdleScreen":false,
            "launchedFromCloud":false,
            "namespaces":[{"name":"urn:x-cast:com.google.cast.something"}]
          }
        ],
        "userEq":{},
        "volume":{
          "controlType":"attenuation",
          "level":1.0,
          "muted":false,
          "stepInterval":0.05
        }
      }
  })";
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .InSequence(phase1)
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        const Json::Value payload = ValidateAndParseMessage(
            message, kPlatformReceiverId, kBroadcastId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kRunningAppReceiverStatus).value(), payload);
      }));
  auto launch_result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":17,
        "type":"LAUNCH",
        "appId":"1A2B3C4D",
        "appParams":{"a":1,"b":2},
        "language":"en-US",
        "supportedAppTypes":["WEB"]
      })"));
  ASSERT_TRUE(launch_result.ok()) << launch_result;
  Mock::VerifyAndClearExpectations(idle_app());
  Mock::VerifyAndClearExpectations(&some_app);
  Mock::VerifyAndClearExpectations(sender_inbound());

  // Phase 2: Sender sends a message to the app, and the receiver app sends a
  // reply.
  constexpr char kMessage[] = R"({"type":"FOO","data":"Hello world!"})";
  constexpr char kReplyMessage[] =
      R"({"type":"FOO_REPLY","data":"Hi yourself!"})";
  constexpr char kSenderTransportId[] = "sender-1";
  Sequence phase2;
  EXPECT_CALL(some_app, OnMessage(_, _, _))
      .InSequence(phase2)
      .WillOnce(Invoke([&](const std::string& source_id,
                           const std::string& the_namespace,
                           const std::string& message) {
        EXPECT_EQ(kSenderTransportId, source_id);
        EXPECT_EQ(kAppNamespace, the_namespace);
        const auto parsed = json::Parse(message);
        EXPECT_TRUE(parsed.is_value()) << parsed.error();
        if (parsed.is_value()) {
          EXPECT_EQ(json::Parse(kMessage).value(), parsed.value());
          if (port_for_app) {
            port_for_app->PostMessage(kSenderTransportId, kAppNamespace,
                                      kReplyMessage);
          }
        }
      }));
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .InSequence(phase2)
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        const Json::Value payload =
            ValidateAndParseMessage(message, some_app.GetSessionId(),
                                    kSenderTransportId, kAppNamespace);
        EXPECT_EQ(json::Parse(kReplyMessage).value(), payload);
      }));
  auto message_send_result = sender_outbound()->Send(MakeCastMessage(
      kSenderTransportId, some_app.GetSessionId(), kAppNamespace, kMessage));
  ASSERT_TRUE(message_send_result.ok()) << message_send_result;
  Mock::VerifyAndClearExpectations(&some_app);
  Mock::VerifyAndClearExpectations(sender_inbound());

  // Phase 3: Sender sends a STOP request, which causes the receiver
  // (ApplicationAgent) to stop the app. Then, the idle app will automatically
  // be re-launched, and a RECEIVER_STATUS broadcast message will notify the
  // sender of that.
  Sequence phase3;
  EXPECT_CALL(some_app, DidStop()).InSequence(phase3);
  EXPECT_CALL(*idle_app(), DidLaunch(_, NotNull())).InSequence(phase3);
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .InSequence(phase3)
      .WillOnce(Invoke([&](CastSocket*, CastMessage message) {
        const std::string kExpectedJson = R"({
          "requestId":0,  // Note: 0 for broadcast (no requestor).
          "type":"RECEIVER_STATUS",
          "status":{
            "applications":[
              {
                // NOTE: These IDs and the displayName come from |idle_app_|.
                "sessionId":"E8C28D3C-9ABC-DEF0-1234-000000000002",
                "appId":"E8C28D3C",
                "universalAppId":"E8C28D3C",
                "displayName":"Backdrop",
                "isIdleScreen":true,
                "launchedFromCloud":false,
                "namespaces":[]
              }
            ],
            "userEq":{},
            "volume":{
              "controlType":"attenuation",
              "level":1.0,
              "muted":false,
              "stepInterval":0.05
            }
          }
        })";
        const Json::Value payload = ValidateAndParseMessage(
            message, kPlatformReceiverId, kBroadcastId, kReceiverNamespace);
        EXPECT_EQ(json::Parse(kExpectedJson).value(), payload);
      }));
  auto stop_result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
    "requestId":18,
    "type":"STOP",
    "sessionId":"1A2B3C4D-9ABC-DEF0-1234-000000000001"
  })"));
  ASSERT_TRUE(stop_result.ok()) << stop_result;
  Mock::VerifyAndClearExpectations(idle_app());
  Mock::VerifyAndClearExpectations(&some_app);
  Mock::VerifyAndClearExpectations(sender_inbound());

  agent()->UnregisterApplication(&some_app);
}

TEST_F(ApplicationAgentTest, AllowsVirtualConnectionsToApp) {
  NiceMock<FakeApplication> some_app("1A2B3C4D", "Something Doer");
  agent()->RegisterApplication(&some_app);

  // Launch the app, using gMock to simulate an app that calls
  // MessagePort::SetClient() (to permit messaging) and to get the transport ID
  // of the app.
  EXPECT_CALL(*idle_app(), DidStop());
  EXPECT_CALL(some_app, DidLaunch(_, NotNull()))
      .WillOnce(Invoke([&](Json::Value params, MessagePort* port) {
        port->SetClient(some_app);
      }));
  std::string transport_id;
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _))
      .WillRepeatedly(Invoke([&](CastSocket*, CastMessage message) {
        const Json::Value payload = ValidateAndParseMessage(
            message, kPlatformReceiverId, kBroadcastId, kReceiverNamespace);
        if (payload["type"].asString() == "RECEIVER_STATUS") {
          transport_id =
              payload["status"]["applications"][0]["transportId"].asString();
        }
      }));
  auto launch_result = sender_outbound()->Send(MakeCastMessage(
      kPlatformSenderId, kPlatformReceiverId, kReceiverNamespace, R"({
        "requestId":1,
        "type":"LAUNCH",
        "appId":"1A2B3C4D",
        "appParams":{},
        "language":"en-US",
        "supportedAppTypes":["WEB"]
      })"));
  ASSERT_TRUE(launch_result.ok()) << launch_result;
  Mock::VerifyAndClearExpectations(idle_app());
  Mock::VerifyAndClearExpectations(&some_app);
  Mock::VerifyAndClearExpectations(sender_inbound());

  // Now that the application has launched, check that the policy allows
  // connections to both the ApplicationAgent and the running application.
  auto* const policy =
      static_cast<ConnectionNamespaceHandler::VirtualConnectionPolicy*>(
          agent());
  EXPECT_TRUE(policy->IsConnectionAllowed(
      VirtualConnection{kPlatformReceiverId, "any-sender-12345", 0}));
  ASSERT_FALSE(transport_id.empty());
  EXPECT_TRUE(policy->IsConnectionAllowed(
      VirtualConnection{transport_id, "any-sender-12345", 0}));
  EXPECT_FALSE(policy->IsConnectionAllowed(
      VirtualConnection{"wherever i likes", "any-sender-12345", 0}));

  // Unregister the app, which will automatically stop it too.
  EXPECT_CALL(some_app, DidStop());
  EXPECT_CALL(*idle_app(), DidLaunch(_, NotNull()));
  EXPECT_CALL(*sender_inbound(), OnMessage(_, _));  // RECEIVER_STATUS update.
  agent()->UnregisterApplication(&some_app);

  // With the app stopped, check that the policy no longer allows connections to
  // the now-stale |transport_id|.
  EXPECT_TRUE(policy->IsConnectionAllowed(
      VirtualConnection{kPlatformReceiverId, "any-sender-12345", 0}));
  EXPECT_FALSE(policy->IsConnectionAllowed(
      VirtualConnection{transport_id, "any-sender-12345", 0}));
  EXPECT_FALSE(policy->IsConnectionAllowed(
      VirtualConnection{"wherever i likes", "any-sender-12345", 0}));
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
