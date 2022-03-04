// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/public/presentation/presentation_receiver.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "osp/impl/presentation/testing/mock_connection_delegate.h"
#include "osp/impl/quic/quic_client.h"
#include "osp/impl/quic/quic_server.h"
#include "osp/impl/quic/testing/fake_quic_connection_factory.h"
#include "osp/impl/quic/testing/quic_test_support.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/protocol_connection_server.h"
#include "osp/public/testing/message_demuxer_test_support.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

namespace openscreen {
namespace osp {

namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

class MockConnectRequest final
    : public ProtocolConnectionClient::ConnectionRequestCallback {
 public:
  ~MockConnectRequest() override = default;

  MOCK_METHOD2(OnConnectionOpened,
               void(uint64_t request_id,
                    std::unique_ptr<ProtocolConnection> connection));
  MOCK_METHOD1(OnConnectionFailed, void(uint64_t request_id));
};

class MockReceiverDelegate final : public ReceiverDelegate {
 public:
  ~MockReceiverDelegate() override = default;

  MOCK_METHOD3(
      OnUrlAvailabilityRequest,
      std::vector<msgs::UrlAvailability>(uint64_t watch_id,
                                         uint64_t watch_duration,
                                         std::vector<std::string> urls));
  MOCK_METHOD3(StartPresentation,
               bool(const Connection::PresentationInfo& info,
                    uint64_t source_id,
                    const std::vector<msgs::HttpHeader>& http_headers));
  MOCK_METHOD3(ConnectToPresentation,
               bool(uint64_t request_id,
                    const std::string& id,
                    uint64_t source_id));
  MOCK_METHOD2(TerminatePresentation,
               void(const std::string& id, TerminationReason reason));
};

class PresentationReceiverTest : public ::testing::Test {
 public:
  PresentationReceiverTest() {
    fake_clock_ = std::make_unique<FakeClock>(
        Clock::time_point(std::chrono::milliseconds(1298424)));
    task_runner_ = std::make_unique<FakeTaskRunner>(fake_clock_.get());
    quic_bridge_ =
        std::make_unique<FakeQuicBridge>(task_runner_.get(), FakeClock::now);
  }

 protected:
  std::unique_ptr<ProtocolConnection> MakeClientStream() {
    MockConnectRequest mock_connect_request;
    NetworkServiceManager::Get()->GetProtocolConnectionClient()->Connect(
        quic_bridge_->kReceiverEndpoint, &mock_connect_request);
    std::unique_ptr<ProtocolConnection> stream;
    EXPECT_CALL(mock_connect_request, OnConnectionOpened(_, _))
        .WillOnce([&stream](uint64_t request_id,
                            std::unique_ptr<ProtocolConnection> connection) {
          stream = std::move(connection);
        });
    quic_bridge_->RunTasksUntilIdle();
    return stream;
  }

  void SetUp() override {
    NetworkServiceManager::Create(nullptr, nullptr,
                                  std::move(quic_bridge_->quic_client),
                                  std::move(quic_bridge_->quic_server));
    Receiver::Get()->Init();
    Receiver::Get()->SetReceiverDelegate(&mock_receiver_delegate_);
  }

  void TearDown() override {
    Receiver::Get()->SetReceiverDelegate(nullptr);
    Receiver::Get()->Deinit();
    NetworkServiceManager::Dispose();
  }

  std::unique_ptr<FakeClock> fake_clock_;
  std::unique_ptr<FakeTaskRunner> task_runner_;
  const std::string url1_{"https://www.example.com/receiver.html"};
  std::unique_ptr<FakeQuicBridge> quic_bridge_;
  MockReceiverDelegate mock_receiver_delegate_;
};

}  // namespace

// TODO(btolsch): Availability CL includes watch duration, so when that lands,
// also test proper updating here.
TEST_F(PresentationReceiverTest, QueryAvailability) {
  MockMessageCallback mock_callback;
  MessageDemuxer::MessageWatch availability_watch =
      quic_bridge_->controller_demuxer->SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationUrlAvailabilityResponse, &mock_callback);

  std::unique_ptr<ProtocolConnection> stream = MakeClientStream();
  ASSERT_TRUE(stream);

  msgs::PresentationUrlAvailabilityRequest request{/* .request_id = */ 0,
                                                   /* .urls = */ {url1_},
                                                   /* .watch_duration = */ 0,
                                                   /* .watch_id = */ 0};
  msgs::CborEncodeBuffer buffer;
  ASSERT_TRUE(msgs::EncodePresentationUrlAvailabilityRequest(request, &buffer));
  stream->Write(buffer.data(), buffer.size());

  EXPECT_CALL(mock_receiver_delegate_, OnUrlAvailabilityRequest(_, _, _))
      .WillOnce(Invoke([this](uint64_t watch_id, uint64_t watch_duration,
                              std::vector<std::string> urls) {
        EXPECT_EQ(std::vector<std::string>{url1_}, urls);

        return std::vector<msgs::UrlAvailability>{
            msgs::UrlAvailability::kAvailable};
      }));

  msgs::PresentationUrlAvailabilityResponse response;
  EXPECT_CALL(mock_callback, OnStreamMessage(_, _, _, _, _, _))
      .WillOnce(Invoke([&response](uint64_t endpoint_id, uint64_t cid,
                                   msgs::Type message_type, const uint8_t* buf,
                                   size_t buffer_size, Clock::time_point now) {
        ssize_t result = msgs::DecodePresentationUrlAvailabilityResponse(
            buf, buffer_size, &response);
        return result;
      }));
  quic_bridge_->RunTasksUntilIdle();
  EXPECT_EQ(request.request_id, response.request_id);
  EXPECT_EQ(
      (std::vector<msgs::UrlAvailability>{msgs::UrlAvailability::kAvailable}),
      response.url_availabilities);
}

TEST_F(PresentationReceiverTest, StartPresentation) {
  MockMessageCallback mock_callback;
  MessageDemuxer::MessageWatch initiation_watch =
      quic_bridge_->controller_demuxer->SetDefaultMessageTypeWatch(
          msgs::Type::kPresentationStartResponse, &mock_callback);

  std::unique_ptr<ProtocolConnection> stream = MakeClientStream();
  ASSERT_TRUE(stream);

  const std::string presentation_id = "KMvyNqTCvvSv7v5X";
  msgs::PresentationStartRequest request;
  request.request_id = 0;
  request.presentation_id = presentation_id;
  request.url = url1_;
  request.headers = {msgs::HttpHeader{"Accept-Language", "de"}};
  msgs::CborEncodeBuffer buffer;
  ASSERT_TRUE(msgs::EncodePresentationStartRequest(request, &buffer));
  stream->Write(buffer.data(), buffer.size());
  Connection::PresentationInfo info;
  EXPECT_CALL(mock_receiver_delegate_, StartPresentation(_, _, request.headers))
      .WillOnce(::testing::DoAll(::testing::SaveArg<0>(&info),
                                 ::testing::Return(true)));
  quic_bridge_->RunTasksUntilIdle();
  EXPECT_EQ(presentation_id, info.id);
  EXPECT_EQ(url1_, info.url);

  NiceMock<MockConnectionDelegate> null_connection_delegate;
  Connection connection(Connection::PresentationInfo{presentation_id, url1_},
                        &null_connection_delegate, Receiver::Get());
  Receiver::Get()->OnPresentationStarted(presentation_id, &connection,
                                         ResponseResult::kSuccess);
  msgs::PresentationStartResponse response;
  EXPECT_CALL(mock_callback, OnStreamMessage(_, _, _, _, _, _))
      .WillOnce(Invoke([&response](uint64_t endpoint_id, uint64_t cid,
                                   msgs::Type message_type, const uint8_t* buf,
                                   size_t buf_size, Clock::time_point now) {
        ssize_t result =
            msgs::DecodePresentationStartResponse(buf, buf_size, &response);
        return result;
      }));
  quic_bridge_->RunTasksUntilIdle();
  EXPECT_EQ(msgs::Result::kSuccess, response.result);
  EXPECT_EQ(connection.connection_id(), response.connection_id);
}

// TODO(btolsch): Connect and reconnect.
// TODO(btolsch): Terminate request and event.

}  // namespace osp
}  // namespace openscreen
