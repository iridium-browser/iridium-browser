// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "discovery/mdns/mdns_probe.h"

#include <memory>
#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_probe_manager.h"
#include "discovery/mdns/mdns_querier.h"
#include "discovery/mdns/mdns_random.h"
#include "discovery/mdns/mdns_receiver.h"
#include "discovery/mdns/mdns_sender.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace openscreen {
namespace discovery {

class MockMdnsSender : public MdnsSender {
 public:
  explicit MockMdnsSender(UdpSocket* socket) : MdnsSender(socket) {}
  MOCK_METHOD1(SendMulticast, Error(const MdnsMessage& message));
  MOCK_METHOD2(SendMessage,
               Error(const MdnsMessage& message, const IPEndpoint& endpoint));
};

class MockObserver : public MdnsProbeImpl::Observer {
 public:
  MOCK_METHOD1(OnProbeSuccess, void(MdnsProbe*));
  MOCK_METHOD1(OnProbeFailure, void(MdnsProbe*));
};

class MdnsProbeTests : public testing::Test {
 public:
  MdnsProbeTests()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        socket_(&task_runner_),
        sender_(&socket_),
        receiver_(config_) {
    EXPECT_EQ(task_runner_.delayed_task_count(), 0);
    probe_ = CreateProbe();
    EXPECT_EQ(task_runner_.delayed_task_count(), 1);
  }

 protected:
  std::unique_ptr<MdnsProbeImpl> CreateProbe() {
    return std::make_unique<MdnsProbeImpl>(&sender_, &receiver_, &random_,
                                           &task_runner_, FakeClock::now,
                                           &observer_, name_, address_v4_);
  }

  MdnsMessage CreateMessage(const DomainName& domain) {
    MdnsMessage message(0, MessageType::Response);
    SrvRecordRdata rdata(0, 0, 80, domain);
    MdnsRecord record(std::move(domain), DnsType::kSRV, DnsClass::kIN,
                      RecordType::kUnique, std::chrono::seconds(1),
                      std::move(rdata));
    message.AddAnswer(record);
    return message;
  }

  void OnMessageReceived(const MdnsMessage& message) {
    probe_->OnMessageReceived(message);
  }

  Config config_;
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeUdpSocket socket_;
  StrictMock<MockMdnsSender> sender_;
  MdnsReceiver receiver_;
  MdnsRandom random_;
  StrictMock<MockObserver> observer_;

  std::unique_ptr<MdnsProbeImpl> probe_;

  const DomainName name_{"test", "_googlecast", "_tcp", "local"};
  const DomainName name2_{"test2", "_googlecast", "_tcp", "local"};

  const IPAddress address_v4_{192, 168, 0, 0};
  const IPEndpoint endpoint_v4_{address_v4_, 80};
};

// TODO(issuetracker.google.com/issues/243611087): Occasionally flaky in bots.
TEST_F(MdnsProbeTests, DISABLED_TestNoCancelationFlow) {
  EXPECT_CALL(sender_, SendMulticast(_));
  clock_.Advance(kDelayBetweenProbeQueries);
  EXPECT_EQ(task_runner_.delayed_task_count(), 1);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  EXPECT_CALL(sender_, SendMulticast(_));
  clock_.Advance(kDelayBetweenProbeQueries);
  EXPECT_EQ(task_runner_.delayed_task_count(), 1);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  EXPECT_CALL(sender_, SendMulticast(_));
  clock_.Advance(kDelayBetweenProbeQueries);
  EXPECT_EQ(task_runner_.delayed_task_count(), 1);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  EXPECT_CALL(observer_, OnProbeSuccess(probe_.get())).Times(1);
  clock_.Advance(kDelayBetweenProbeQueries);
  EXPECT_EQ(task_runner_.delayed_task_count(), 0);
}

TEST_F(MdnsProbeTests, CancelationWhenMatchingMessageReceived) {
  EXPECT_CALL(observer_, OnProbeFailure(probe_.get())).Times(1);
  OnMessageReceived(CreateMessage(name_));
}

TEST_F(MdnsProbeTests, TestNoCancelationOnUnrelatedMessages) {
  OnMessageReceived(CreateMessage(name2_));

  EXPECT_CALL(sender_, SendMulticast(_));
  clock_.Advance(kDelayBetweenProbeQueries);
  EXPECT_EQ(task_runner_.delayed_task_count(), 1);
  testing::Mock::VerifyAndClearExpectations(&sender_);
}

}  // namespace discovery
}  // namespace openscreen
