// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_probe_manager.h"

#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_probe.h"
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

class MockDomainConfirmedProvider : public MdnsDomainConfirmedProvider {
 public:
  MOCK_METHOD2(OnDomainFound, void(const DomainName&, const DomainName&));
};

class MockMdnsSender : public MdnsSender {
 public:
  explicit MockMdnsSender(UdpSocket* socket) : MdnsSender(socket) {}

  MOCK_METHOD1(SendMulticast, Error(const MdnsMessage& message));
  MOCK_METHOD2(SendMessage,
               Error(const MdnsMessage& message, const IPEndpoint& endpoint));
};

class MockMdnsProbe : public MdnsProbe {
 public:
  MockMdnsProbe(DomainName target_name, IPAddress address)
      : MdnsProbe(std::move(target_name), std::move(address)) {}

  MOCK_METHOD1(Postpone, void(std::chrono::seconds));
  MOCK_METHOD1(OnMessageReceived, void(const MdnsMessage&));
};

class TestMdnsProbeManager : public MdnsProbeManagerImpl {
 public:
  using MdnsProbeManagerImpl::MdnsProbeManagerImpl;

  using MdnsProbeManagerImpl::OnProbeFailure;
  using MdnsProbeManagerImpl::OnProbeSuccess;

  std::unique_ptr<MdnsProbe> CreateProbe(DomainName name,
                                         IPAddress address) override {
    return std::make_unique<StrictMock<MockMdnsProbe>>(std::move(name),
                                                       std::move(address));
  }

  StrictMock<MockMdnsProbe>* GetOngoingMockProbeByTarget(
      const DomainName& target) {
    const auto it =
        std::find_if(ongoing_probes_.begin(), ongoing_probes_.end(),
                     [&target](const OngoingProbe& ongoing) {
                       return ongoing.probe->target_name() == target;
                     });
    if (it != ongoing_probes_.end()) {
      return static_cast<StrictMock<MockMdnsProbe>*>(it->probe.get());
    }
    return nullptr;
  }

  StrictMock<MockMdnsProbe>* GetCompletedMockProbe(const DomainName& target) {
    const auto it = FindCompletedProbe(target);
    if (it != completed_probes_.end()) {
      return static_cast<StrictMock<MockMdnsProbe>*>(it->get());
    }
    return nullptr;
  }

  bool HasOngoingProbe(const DomainName& target) {
    return GetOngoingMockProbeByTarget(target) != nullptr;
  }

  bool HasCompletedProbe(const DomainName& target) {
    return GetCompletedMockProbe(target) != nullptr;
  }

  size_t GetOngoingProbeCount() { return ongoing_probes_.size(); }

  size_t GetCompletedProbeCount() { return completed_probes_.size(); }
};

class MdnsProbeManagerTests : public testing::Test {
 public:
  MdnsProbeManagerTests()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        socket_(&task_runner_),
        sender_(&socket_),
        receiver_(config_),
        manager_(&sender_,
                 &receiver_,
                 &random_,
                 &task_runner_,
                 FakeClock::now) {
    ExpectProbeStopped(name_);
    ExpectProbeStopped(name2_);
    ExpectProbeStopped(name_retry_);
  }

 protected:
  MdnsMessage CreateProbeQueryMessage(DomainName domain,
                                      const IPAddress& address) {
    MdnsMessage message(CreateMessageId(), MessageType::Query);
    MdnsQuestion question(domain, DnsType::kANY, DnsClass::kANY,
                          ResponseType::kUnicast);
    MdnsRecord record = CreateAddressRecord(std::move(domain), address);
    message.AddQuestion(std::move(question));
    message.AddAuthorityRecord(std::move(record));
    return message;
  }

  void ExpectProbeStopped(const DomainName& name) {
    EXPECT_FALSE(manager_.HasOngoingProbe(name));
    EXPECT_FALSE(manager_.HasCompletedProbe(name));
    EXPECT_FALSE(manager_.IsDomainClaimed(name));
  }

  StrictMock<MockMdnsProbe>* ExpectProbeOngoing(const DomainName& name) {
    // Get around limitations of using an assert in a function with a return
    // value.
    auto validate = [this, &name]() {
      ASSERT_TRUE(manager_.HasOngoingProbe(name));
      EXPECT_FALSE(manager_.HasCompletedProbe(name));
      EXPECT_FALSE(manager_.IsDomainClaimed(name));
    };
    validate();

    return manager_.GetOngoingMockProbeByTarget(name);
  }

  StrictMock<MockMdnsProbe>* ExpectProbeCompleted(const DomainName& name) {
    // Get around limitations of using an assert in a function with a return
    // value.
    auto validate = [this, &name]() {
      EXPECT_FALSE(manager_.HasOngoingProbe(name));
      ASSERT_TRUE(manager_.HasCompletedProbe(name));
      EXPECT_TRUE(manager_.IsDomainClaimed(name));
    };
    validate();

    return manager_.GetCompletedMockProbe(name);
  }

  StrictMock<MockMdnsProbe>* SetUpCompletedProbe(const DomainName& name,
                                                 const IPAddress& address) {
    EXPECT_TRUE(manager_.StartProbe(&callback_, name, address).ok());
    EXPECT_CALL(callback_, OnDomainFound(name, name));
    StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name);
    manager_.OnProbeSuccess(ongoing_probe);
    ExpectProbeCompleted(name);
    testing::Mock::VerifyAndClearExpectations(ongoing_probe);

    return ongoing_probe;
  }

  Config config_;
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeUdpSocket socket_;
  StrictMock<MockMdnsSender> sender_;
  MdnsReceiver receiver_;
  MdnsRandom random_;
  StrictMock<TestMdnsProbeManager> manager_;
  MockDomainConfirmedProvider callback_;

  const DomainName name_{"test", "_googlecast", "_tcp", "local"};
  const DomainName name_retry_{"test1", "_googlecast", "_tcp", "local"};
  const DomainName name2_{"test2", "_googlecast", "_tcp", "local"};

  // When used to create address records A, B, C, A > B because comparison of
  // the rdata in each results in the comparison of endpoints, for which
  // address_b_ < address_a_. A < C because A is DnsType kA with value 1 and
  // C is DnsType kAAAA with value 28.
  const IPAddress address_a_{192, 168, 0, 0};
  const IPAddress address_b_{190, 160, 0, 0};
  const IPAddress address_c_{0x0102, 0x0304, 0x0506, 0x0708,
                             0x090a, 0x0b0c, 0x0d0e, 0x0f10};
  const IPEndpoint endpoint_{{192, 168, 0, 0}, 80};
};

TEST_F(MdnsProbeManagerTests, StartProbeBeginsProbeWhenNoneExistsOnly) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  ExpectProbeOngoing(name_);
  EXPECT_FALSE(manager_.IsDomainClaimed(name2_));

  EXPECT_FALSE(manager_.StartProbe(&callback_, name_, address_a_).ok());

  EXPECT_CALL(callback_, OnDomainFound(name_, name_));
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);
  manager_.OnProbeSuccess(ongoing_probe);
  EXPECT_FALSE(manager_.IsDomainClaimed(name2_));
  testing::Mock::VerifyAndClearExpectations(ongoing_probe);

  EXPECT_FALSE(manager_.StartProbe(&callback_, name_, address_a_).ok());

  StrictMock<MockMdnsProbe>* completed_probe = ExpectProbeCompleted(name_);
  EXPECT_EQ(ongoing_probe, completed_probe);
  EXPECT_FALSE(manager_.IsDomainClaimed(name2_));
}

TEST_F(MdnsProbeManagerTests, StopProbeChangesOngoingProbesOnly) {
  EXPECT_FALSE(manager_.StopProbe(name_).ok());
  ExpectProbeStopped(name_);

  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  ExpectProbeOngoing(name_);

  EXPECT_TRUE(manager_.StopProbe(name_).ok());
  ExpectProbeStopped(name_);

  SetUpCompletedProbe(name_, address_a_);

  EXPECT_FALSE(manager_.StopProbe(name_).ok());
  ExpectProbeCompleted(name_);
}

TEST_F(MdnsProbeManagerTests, RespondToProbeQuerySendsNothingOnUnownedDomain) {
  const MdnsMessage query = CreateProbeQueryMessage(name_, address_c_);
  manager_.RespondToProbeQuery(query, endpoint_);
}

TEST_F(MdnsProbeManagerTests, RespondToProbeQueryWorksForCompletedProbes) {
  SetUpCompletedProbe(name_, address_a_);

  const MdnsMessage query = CreateProbeQueryMessage(name_, address_c_);
  EXPECT_CALL(sender_, SendMessage(_, endpoint_))
      .WillOnce([this](const MdnsMessage& message,
                       const IPEndpoint& endpoint) -> Error {
        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_EQ(message.answers()[0].dns_type(), DnsType::kA);
        EXPECT_EQ(message.answers()[0].name(), this->name_);
        return Error::None();
      });
  manager_.RespondToProbeQuery(query, endpoint_);
}

TEST_F(MdnsProbeManagerTests, TiebreakProbeQueryWorksForSingleRecordQueries) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);

  // If the probe message received matches the currently running probe, do
  // nothing.
  MdnsMessage query = CreateProbeQueryMessage(name_, address_a_);
  manager_.RespondToProbeQuery(query, endpoint_);

  // If the probe message received is less than the ongoing probe, ignore the
  // incoming probe.
  query = CreateProbeQueryMessage(name_, address_b_);
  manager_.RespondToProbeQuery(query, endpoint_);

  // If the probe message received is greater than the ongoing probe, postpone
  // the currently running probe.
  query = CreateProbeQueryMessage(name_, address_c_);
  EXPECT_CALL(*ongoing_probe, Postpone(_)).Times(1);
  manager_.RespondToProbeQuery(query, endpoint_);
}

TEST_F(MdnsProbeManagerTests, TiebreakProbeQueryWorksForMultiRecordQueries) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);

  // For the below tests, note that if records A, B, C are generated from
  // addresses |address_a_|, |address_b_|, and |address_c_| respectively,
  // then B < A < C.
  //
  // If the received records have one record less than the tested record, they
  // are sorted and the lowest record is compared.
  MdnsMessage query = CreateProbeQueryMessage(name_, address_b_);
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_c_));
  manager_.RespondToProbeQuery(query, endpoint_);

  query = CreateProbeQueryMessage(name_, address_c_);
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_b_));
  manager_.RespondToProbeQuery(query, endpoint_);

  query = CreateProbeQueryMessage(name_, address_a_);
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_b_));
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_c_));
  manager_.RespondToProbeQuery(query, endpoint_);

  // If the probe message received has the same first record as what's being
  // compared and the query has more records, the query wins.
  query = CreateProbeQueryMessage(name_, address_a_);
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_c_));
  EXPECT_CALL(*ongoing_probe, Postpone(_)).Times(1);
  manager_.RespondToProbeQuery(query, endpoint_);
  testing::Mock::VerifyAndClearExpectations(ongoing_probe);

  query = CreateProbeQueryMessage(name_, address_c_);
  query.AddAuthorityRecord(CreateAddressRecord(name_, address_a_));
  EXPECT_CALL(*ongoing_probe, Postpone(_)).Times(1);
  manager_.RespondToProbeQuery(query, endpoint_);
}

TEST_F(MdnsProbeManagerTests, ProbeSuccessAfterProbeRemovalNoOp) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);
  EXPECT_TRUE(manager_.StopProbe(name_).ok());
  manager_.OnProbeSuccess(ongoing_probe);
}

TEST_F(MdnsProbeManagerTests, ProbeFailureAfterProbeRemovalNoOp) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);
  EXPECT_TRUE(manager_.StopProbe(name_).ok());
  manager_.OnProbeFailure(ongoing_probe);
}

TEST_F(MdnsProbeManagerTests, ProbeFailureCallsCallbackWhenAlreadyClaimed) {
  // This test first starts a probe with domain |name_retry_| so that when
  // probe with domain |name_| fails, the newly generated domain with equal
  // |name_retry_|.
  StrictMock<MockMdnsProbe>* ongoing_probe =
      SetUpCompletedProbe(name_retry_, address_a_);

  // Because |name_retry_| has already succeeded, the retry logic should skip
  // over re-querying for |name_retry_| and jump right to success.
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  ongoing_probe = ExpectProbeOngoing(name_);
  EXPECT_CALL(callback_, OnDomainFound(name_, name_retry_));
  manager_.OnProbeFailure(ongoing_probe);
  ExpectProbeStopped(name_);
  ExpectProbeCompleted(name_retry_);
}

TEST_F(MdnsProbeManagerTests, ProbeFailureCreatesNewProbeIfNameUnclaimed) {
  EXPECT_TRUE(manager_.StartProbe(&callback_, name_, address_a_).ok());
  StrictMock<MockMdnsProbe>* ongoing_probe = ExpectProbeOngoing(name_);
  manager_.OnProbeFailure(ongoing_probe);
  ExpectProbeStopped(name_);
  ongoing_probe = ExpectProbeOngoing(name_retry_);
  EXPECT_EQ(ongoing_probe->target_name(), name_retry_);

  EXPECT_CALL(callback_, OnDomainFound(name_, name_retry_));
  manager_.OnProbeSuccess(ongoing_probe);
  ExpectProbeCompleted(name_retry_);
  ExpectProbeStopped(name_);
}

}  // namespace discovery
}  // namespace openscreen
