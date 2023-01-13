// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/sender/cast_app_discovery_service_impl.h"

#include <utility>

#include "cast/common/channel/testing/fake_cast_socket.h"
#include "cast/common/channel/testing/mock_socket_error_handler.h"
#include "cast/common/channel/virtual_connection_router.h"
#include "cast/common/public/receiver_info.h"
#include "cast/sender/testing/test_helpers.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

using ::cast::channel::CastMessage;

using ::testing::_;

class CastAppDiscoveryServiceImplTest : public ::testing::Test {
 public:
  void SetUp() override {
    socket_id_ = fake_cast_socket_pair_.socket->socket_id();
    router_.TakeSocket(&mock_error_handler_,
                       std::move(fake_cast_socket_pair_.socket));

    receiver_.v4_address = fake_cast_socket_pair_.remote_endpoint.address;
    receiver_.port = fake_cast_socket_pair_.remote_endpoint.port;
    receiver_.unique_id = "receiverId1";
    receiver_.friendly_name = "Some Name";
  }

 protected:
  CastSocket& peer_socket() { return *fake_cast_socket_pair_.peer_socket; }
  MockCastSocketClient& peer_client() {
    return fake_cast_socket_pair_.mock_peer_client;
  }

  void AddOrUpdateReceiver(const ReceiverInfo& receiver, int32_t socket_id) {
    platform_client_.AddOrUpdateReceiver(receiver, socket_id);
    app_discovery_service_.AddOrUpdateReceiver(receiver);
  }

  CastAppDiscoveryService::Subscription StartObservingAvailability(
      const CastMediaSource& source,
      std::vector<ReceiverInfo>* save_receivers) {
    return app_discovery_service_.StartObservingAvailability(
        source, [save_receivers](const CastMediaSource&,
                                 const std::vector<ReceiverInfo>& receivers) {
          *save_receivers = receivers;
        });
  }

  CastAppDiscoveryService::Subscription StartSourceA1Query(
      std::vector<ReceiverInfo>* receivers,
      int* request_id,
      std::string* sender_id) {
    auto subscription = StartObservingAvailability(source_a_1_, receivers);

    // Adding a receiver after app registered causes app availability request to
    // be sent.
    *request_id = -1;
    *sender_id = "";
    EXPECT_CALL(peer_client(), OnMessage(_, _))
        .WillOnce([request_id, sender_id](CastSocket*, CastMessage message) {
          VerifyAppAvailabilityRequest(message, "AAA", request_id, sender_id);
        });

    AddOrUpdateReceiver(receiver_, socket_id_);

    return subscription;
  }

  FakeCastSocketPair fake_cast_socket_pair_;
  int32_t socket_id_;
  MockSocketErrorHandler mock_error_handler_;
  VirtualConnectionRouter router_;
  FakeClock clock_{Clock::now()};
  FakeTaskRunner task_runner_{&clock_};
  CastPlatformClient platform_client_{&router_, &FakeClock::now, &task_runner_};
  CastAppDiscoveryServiceImpl app_discovery_service_{&platform_client_,
                                                     &FakeClock::now};

  CastMediaSource source_a_1_{"cast:AAA?clientId=1", {"AAA"}};
  CastMediaSource source_a_2_{"cast:AAA?clientId=2", {"AAA"}};
  CastMediaSource source_b_1_{"cast:BBB?clientId=1", {"BBB"}};

  ReceiverInfo receiver_;
};

TEST_F(CastAppDiscoveryServiceImplTest, StartObservingAvailability) {
  std::vector<ReceiverInfo> receivers1;
  int request_id;
  std::string sender_id;
  auto subscription1 = StartSourceA1Query(&receivers1, &request_id, &sender_id);

  // Same app ID should not trigger another request.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  std::vector<ReceiverInfo> receivers2;
  auto subscription2 = StartObservingAvailability(source_a_2_, &receivers2);

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  ASSERT_EQ(receivers1.size(), 1u);
  ASSERT_EQ(receivers2.size(), 1u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");
  EXPECT_EQ(receivers2[0].unique_id, "receiverId1");

  // No more updates for |source_a_1_| (i.e. |receivers1|).
  subscription1.Reset();
  platform_client_.RemoveReceiver(receiver_);
  app_discovery_service_.RemoveReceiver(receiver_);
  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers2.size(), 0u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");
}

TEST_F(CastAppDiscoveryServiceImplTest, ReAddAvailQueryUsesCachedValue) {
  std::vector<ReceiverInfo> receivers1;
  int request_id;
  std::string sender_id;
  auto subscription1 = StartSourceA1Query(&receivers1, &request_id, &sender_id);

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");

  subscription1.Reset();
  receivers1.clear();

  // Request not re-sent; cached kAvailable value is used.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  subscription1 = StartObservingAvailability(source_a_1_, &receivers1);
  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");
}

TEST_F(CastAppDiscoveryServiceImplTest, AvailQueryUpdatedOnReceiverUpdate) {
  std::vector<ReceiverInfo> receivers1;
  int request_id;
  std::string sender_id;
  auto subscription1 = StartSourceA1Query(&receivers1, &request_id, &sender_id);

  // Result set now includes |receiver_|.
  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");

  // Updating |receiver_| causes |source_a_1_| query to be updated, but it's too
  // soon for a new message to be sent.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  receiver_.friendly_name = "New Name";

  AddOrUpdateReceiver(receiver_, socket_id_);

  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers1[0].friendly_name, "New Name");
}

TEST_F(CastAppDiscoveryServiceImplTest, Refresh) {
  std::vector<ReceiverInfo> receivers1;
  auto subscription1 = StartObservingAvailability(source_a_1_, &receivers1);
  std::vector<ReceiverInfo> receivers2;
  auto subscription2 = StartObservingAvailability(source_b_1_, &receivers2);

  // Adding a receiver after app registered causes two separate app availability
  // requests to be sent.
  int request_idA = -1;
  int request_idB = -1;
  std::string sender_id = "";
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .Times(2)
      .WillRepeatedly([&request_idA, &request_idB, &sender_id](
                          CastSocket*, CastMessage message) {
        std::string app_id;
        int request_id = -1;
        VerifyAppAvailabilityRequest(message, &app_id, &request_id, &sender_id);
        if (app_id == "AAA") {
          EXPECT_EQ(request_idA, -1);
          request_idA = request_id;
        } else if (app_id == "BBB") {
          EXPECT_EQ(request_idB, -1);
          request_idB = request_id;
        } else {
          EXPECT_TRUE(false);
        }
      });

  AddOrUpdateReceiver(receiver_, socket_id_);

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_idA, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  availability_response =
      CreateAppUnavailableResponseChecked(request_idB, sender_id, "BBB");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  ASSERT_EQ(receivers1.size(), 1u);
  ASSERT_EQ(receivers2.size(), 0u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");

  // Not enough time has passed for a refresh.
  clock_.Advance(std::chrono::seconds(30));
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  app_discovery_service_.Refresh();

  // Refresh will now query again for unavailable app IDs.
  clock_.Advance(std::chrono::minutes(2));
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_idB, &sender_id](CastSocket*, CastMessage message) {
        VerifyAppAvailabilityRequest(message, "BBB", &request_idB, &sender_id);
      });
  app_discovery_service_.Refresh();
}

TEST_F(CastAppDiscoveryServiceImplTest,
       StartObservingAvailabilityAfterReceiverAdded) {
  // No registered apps.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  AddOrUpdateReceiver(receiver_, socket_id_);

  // Registering apps immediately sends requests to |receiver_|.
  int request_idA = -1;
  std::string sender_id = "";
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_idA, &sender_id](CastSocket*, CastMessage message) {
        VerifyAppAvailabilityRequest(message, "AAA", &request_idA, &sender_id);
      });
  std::vector<ReceiverInfo> receivers1;
  auto subscription1 = StartObservingAvailability(source_a_1_, &receivers1);

  int request_idB = -1;
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_idB, &sender_id](CastSocket*, CastMessage message) {
        VerifyAppAvailabilityRequest(message, "BBB", &request_idB, &sender_id);
      });
  std::vector<ReceiverInfo> receivers2;
  auto subscription2 = StartObservingAvailability(source_b_1_, &receivers2);

  // Add a new receiver with a corresponding socket.
  FakeCastSocketPair fake_sockets2({{192, 168, 1, 17}, 2345},
                                   {{192, 168, 1, 19}, 2345});
  CastSocket* socket2 = fake_sockets2.socket.get();
  router_.TakeSocket(&mock_error_handler_, std::move(fake_sockets2.socket));
  ReceiverInfo receiver2;
  receiver2.unique_id = "receiverId2";
  receiver2.v4_address = fake_sockets2.remote_endpoint.address;
  receiver2.port = fake_sockets2.remote_endpoint.port;

  // Adding new receiver causes availability requests for both apps to be sent
  // to the new receiver.
  request_idA = -1;
  request_idB = -1;
  EXPECT_CALL(fake_sockets2.mock_peer_client, OnMessage(_, _))
      .Times(2)
      .WillRepeatedly([&request_idA, &request_idB, &sender_id](
                          CastSocket*, CastMessage message) {
        std::string app_id;
        int request_id = -1;
        VerifyAppAvailabilityRequest(message, &app_id, &request_id, &sender_id);
        if (app_id == "AAA") {
          EXPECT_EQ(request_idA, -1);
          request_idA = request_id;
        } else if (app_id == "BBB") {
          EXPECT_EQ(request_idB, -1);
          request_idB = request_id;
        } else {
          EXPECT_TRUE(false);
        }
      });

  AddOrUpdateReceiver(receiver2, socket2->socket_id());
}

TEST_F(CastAppDiscoveryServiceImplTest, StartObservingAvailabilityCachedValue) {
  std::vector<ReceiverInfo> receivers1;
  int request_id;
  std::string sender_id;
  auto subscription1 = StartSourceA1Query(&receivers1, &request_id, &sender_id);

  CastMessage availability_response =
      CreateAppAvailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());
  ASSERT_EQ(receivers1.size(), 1u);
  EXPECT_EQ(receivers1[0].unique_id, "receiverId1");

  // Same app ID should not trigger another request, but it should return
  // cached value.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  std::vector<ReceiverInfo> receivers2;
  auto subscription2 = StartObservingAvailability(source_a_2_, &receivers2);
  ASSERT_EQ(receivers2.size(), 1u);
  EXPECT_EQ(receivers2[0].unique_id, "receiverId1");
}

TEST_F(CastAppDiscoveryServiceImplTest, AvailabilityUnknownOrUnavailable) {
  std::vector<ReceiverInfo> receivers1;
  int request_id;
  std::string sender_id;
  auto subscription1 = StartSourceA1Query(&receivers1, &request_id, &sender_id);

  // The request will timeout resulting in unknown app availability.
  clock_.Advance(std::chrono::seconds(10));
  task_runner_.RunTasksUntilIdle();
  EXPECT_EQ(receivers1.size(), 0u);

  // Receiver updated together with unknown app availability will cause a
  // request to be sent again.
  request_id = -1;
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_id, &sender_id](CastSocket*, CastMessage message) {
        VerifyAppAvailabilityRequest(message, "AAA", &request_id, &sender_id);
      });
  AddOrUpdateReceiver(receiver_, socket_id_);

  CastMessage availability_response =
      CreateAppUnavailableResponseChecked(request_id, sender_id, "AAA");
  EXPECT_TRUE(peer_socket().Send(availability_response).ok());

  // Known availability so no request sent.
  EXPECT_CALL(peer_client(), OnMessage(_, _)).Times(0);
  AddOrUpdateReceiver(receiver_, socket_id_);

  // Removing the receiver will also remove previous availability information.
  // Next time the receiver is added, a new request will be sent.
  platform_client_.RemoveReceiver(receiver_);
  app_discovery_service_.RemoveReceiver(receiver_);

  request_id = -1;
  EXPECT_CALL(peer_client(), OnMessage(_, _))
      .WillOnce([&request_id, &sender_id](CastSocket*, CastMessage message) {
        VerifyAppAvailabilityRequest(message, "AAA", &request_id, &sender_id);
      });

  AddOrUpdateReceiver(receiver_, socket_id_);
}

}  // namespace cast
}  // namespace openscreen
