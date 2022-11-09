// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_platform_client.h"

#include <utility>

#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_socket_error_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/receiver_info.h"
#include "cast/sender/testing/test_helpers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/json/json_serialization.h"
#include "util/json/json_value.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

using ::testing::_;

class CastPlatformClientTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_ = fake_cast_socket_pair_.socket.get();
    router_.TakeSocket(&mock_error_handler_,
                       std::move(fake_cast_socket_pair_.socket));

    receiver_.v4_address = IPAddress{192, 168, 0, 17};
    receiver_.port = 4434;
    receiver_.unique_id = "receiverId1";
    platform_client_.AddOrUpdateReceiver(receiver_, socket_->socket_id());
  }

 protected:
  CastSocket& peer_socket() { return *fake_cast_socket_pair_.peer_socket; }
  MockCastSocketClient& peer_client() {
    return fake_cast_socket_pair_.mock_peer_client;
  }

  FakeCastSocketPair fake_cast_socket_pair_;
  CastSocket* socket_ = nullptr;
  MockSocketErrorHandler mock_error_handler_;
  VirtualConnectionRouter router_;
  FakeClock clock_{Clock::now()};
  FakeTaskRunner task_runner_{&clock_};
  CastPlatformClient platform_client_{&router_, &FakeClock::now, &task_runner_};
  ReceiverInfo receiver_;
};

TEST_F(CastPlatformClientTest, AppAvailability) {
  int request_id = -1;
  std::string sender_id;
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_id, &sender_id](CastSocket* socket,
                                          CastMessage message) {
        VerifyAppAvailabilityRequest(message, "AAA", &request_id, &sender_id);
      });
  bool ran = false;
  platform_client_.RequestAppAvailability(
      "receiverId1", "AAA",
      [&ran](const std::string& app_id, AppAvailabilityResult availability) {
        EXPECT_EQ("AAA", app_id);
        EXPECT_EQ(availability, AppAvailabilityResult::kAvailable);
        ran = true;
      });

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  EXPECT_TRUE(ran);

  // NOTE: Callback should only fire once, so it should not fire again here.
  ran = false;
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  EXPECT_FALSE(ran);
}

TEST_F(CastPlatformClientTest, CancelRequest) {
  int request_id = -1;
  std::string sender_id;
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_id, &sender_id](CastSocket* socket,
                                          CastMessage message) {
        VerifyAppAvailabilityRequest(message, "AAA", &request_id, &sender_id);
      });
  absl::optional<int> maybe_request_id =
      platform_client_.RequestAppAvailability(
          "receiverId1", "AAA",
          [](const std::string& app_id, AppAvailabilityResult availability) {
            EXPECT_TRUE(false);
          });
  ASSERT_TRUE(maybe_request_id);
  int local_request_id = maybe_request_id.value();
  platform_client_.CancelRequest(local_request_id);

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
}

}  // namespace cast
}  // namespace openscreen
