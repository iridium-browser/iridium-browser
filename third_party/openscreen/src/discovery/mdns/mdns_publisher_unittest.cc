// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_publisher.h"

#include <chrono>
#include <vector>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_probe_manager.h"
#include "discovery/mdns/mdns_sender.h"
#include "discovery/mdns/testing/mdns_test_util.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"
#include "util/std_util.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace openscreen {
namespace discovery {
namespace {

constexpr Clock::duration kAnnounceGoodbyeDelay = std::chrono::milliseconds(25);

bool ContainsRecord(const std::vector<MdnsRecord::ConstRef>& records,
                    MdnsRecord record) {
  return ContainsIf(records,
                    [&record](const MdnsRecord& ref) { return ref == record; });
}

}  // namespace

class MockMdnsSender : public MdnsSender {
 public:
  explicit MockMdnsSender(UdpSocket* socket) : MdnsSender(socket) {}

  MOCK_METHOD1(SendMulticast, Error(const MdnsMessage& message));
  MOCK_METHOD2(SendMessage,
               Error(const MdnsMessage& message, const IPEndpoint& endpoint));
};

class MockProbeManager : public MdnsProbeManager {
 public:
  MOCK_CONST_METHOD1(IsDomainClaimed, bool(const DomainName&));
  MOCK_METHOD2(RespondToProbeQuery,
               void(const MdnsMessage&, const IPEndpoint&));
};

class MdnsPublisherTesting : public MdnsPublisher {
 public:
  using MdnsPublisher::GetPtrRecords;
  using MdnsPublisher::GetRecords;
  using MdnsPublisher::MdnsPublisher;

  bool IsNonPtrRecordPresent(const DomainName& name) {
    auto it = records_.find(name);
    if (it == records_.end()) {
      return false;
    }
    return ContainsIf(it->second, [](const RecordAnnouncerPtr& announcer) {
      return announcer->record().dns_type() != DnsType::kPTR;
    });
  }
};

class MdnsPublisherTest : public testing::Test {
 public:
  MdnsPublisherTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        socket_(&task_runner_),
        sender_(&socket_),
        publisher_(&sender_,
                   &probe_manager_,
                   &task_runner_,
                   FakeClock::now,
                   config_) {}

  ~MdnsPublisherTest() {
    // Clear out any remaining calls in the task runner queue.
    clock_.Advance(Clock::to_duration(std::chrono::seconds(1)));
  }

 protected:
  Error IsAnnounced(const MdnsRecord& original, const MdnsMessage& message) {
    EXPECT_EQ(message.type(), MessageType::Response);
    EXPECT_EQ(message.questions().size(), size_t{0});
    EXPECT_EQ(message.authority_records().size(), size_t{0});
    EXPECT_EQ(message.additional_records().size(), size_t{0});
    EXPECT_EQ(message.answers().size(), size_t{1});

    const MdnsRecord& sent = message.answers()[0];
    EXPECT_EQ(original.name(), sent.name());
    EXPECT_EQ(original.dns_type(), sent.dns_type());
    EXPECT_EQ(original.dns_class(), sent.dns_class());
    EXPECT_EQ(original.record_type(), sent.record_type());
    EXPECT_EQ(original.rdata(), sent.rdata());
    EXPECT_EQ(original.ttl(), sent.ttl());
    return Error::None();
  }

  Error IsGoodbyeRecord(const MdnsRecord& original,
                        const MdnsMessage& message) {
    EXPECT_EQ(message.type(), MessageType::Response);
    EXPECT_EQ(message.questions().size(), size_t{0});
    EXPECT_EQ(message.authority_records().size(), size_t{0});
    EXPECT_EQ(message.additional_records().size(), size_t{0});
    EXPECT_EQ(message.answers().size(), size_t{1});

    const MdnsRecord& sent = message.answers()[0];
    EXPECT_EQ(original.name(), sent.name());
    EXPECT_EQ(original.dns_type(), sent.dns_type());
    EXPECT_EQ(original.dns_class(), sent.dns_class());
    EXPECT_EQ(original.record_type(), sent.record_type());
    EXPECT_EQ(original.rdata(), sent.rdata());
    EXPECT_EQ(std::chrono::seconds(0), sent.ttl());
    return Error::None();
  }

  void CheckPublishedRecords(const DomainName& domain,
                             DnsType type,
                             std::vector<MdnsRecord> expected_records) {
    EXPECT_EQ(publisher_.GetRecordCount(), expected_records.size());
    auto records = publisher_.GetRecords(domain, type, DnsClass::kIN);
    for (const auto& record : expected_records) {
      EXPECT_TRUE(ContainsRecord(records, record));
    }
  }

  void TestUniqueRecordRegistrationWorkflow(MdnsRecord record,
                                            MdnsRecord record2) {
    EXPECT_CALL(probe_manager_, IsDomainClaimed(domain_))
        .WillRepeatedly(Return(true));
    DnsType type = record.dns_type();

    // Check preconditions.
    ASSERT_EQ(record.dns_type(), record2.dns_type());
    auto records = publisher_.GetRecords(domain_, type, DnsClass::kIN);
    ASSERT_EQ(publisher_.GetRecordCount(), size_t{0});
    ASSERT_EQ(records.size(), size_t{0});
    ASSERT_NE(record, record2);
    ASSERT_TRUE(records.empty());

    // Register a new record.
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([this, &record](const MdnsMessage& message) -> Error {
          return IsAnnounced(record, message);
        });
    EXPECT_TRUE(publisher_.RegisterRecord(record).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Re-register the same record.
    EXPECT_FALSE(publisher_.RegisterRecord(record).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Update a record that doesn't exist
    EXPECT_FALSE(publisher_.UpdateRegisteredRecord(record2, record).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Update an existing record.
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([this, &record2](const MdnsMessage& message) -> Error {
          return IsAnnounced(record2, message);
        });
    EXPECT_TRUE(publisher_.UpdateRegisteredRecord(record, record2).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record2});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Add back the original record
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([this, &record](const MdnsMessage& message) -> Error {
          return IsAnnounced(record, message);
        });
    EXPECT_TRUE(publisher_.RegisterRecord(record).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record, record2});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Delete an existing record.
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([this, &record2](const MdnsMessage& message) -> Error {
          return IsGoodbyeRecord(record2, message);
        });
    EXPECT_TRUE(publisher_.UnregisterRecord(record2).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Delete a non-existing record.
    EXPECT_FALSE(publisher_.UnregisterRecord(record2).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {record});
    EXPECT_TRUE(publisher_.IsNonPtrRecordPresent(domain_));

    // Delete the last record
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([this, &record](const MdnsMessage& message) -> Error {
          return IsGoodbyeRecord(record, message);
        });
    EXPECT_TRUE(publisher_.UnregisterRecord(record).ok());
    clock_.Advance(kAnnounceGoodbyeDelay);
    testing::Mock::VerifyAndClearExpectations(&sender_);
    CheckPublishedRecords(domain_, type, {});
    EXPECT_FALSE(publisher_.IsNonPtrRecordPresent(domain_));
  }

  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeUdpSocket socket_;
  StrictMock<MockMdnsSender> sender_;
  StrictMock<MockProbeManager> probe_manager_;
  Config config_;
  MdnsPublisherTesting publisher_;

  DomainName domain_{"instance", "_googlecast", "_tcp", "local"};
  DomainName ptr_domain_{"_googlecast", "_tcp", "local"};
};

TEST_F(MdnsPublisherTest, ARecordRegistrationWorkflow) {
  const MdnsRecord record1 = GetFakeARecord(domain_);
  const MdnsRecord record2 =
      GetFakeARecord(domain_, std::chrono::seconds(1000));
  TestUniqueRecordRegistrationWorkflow(record1, record2);
}

TEST_F(MdnsPublisherTest, AAAARecordRegistrationWorkflow) {
  const MdnsRecord record1 = GetFakeAAAARecord(domain_);
  const MdnsRecord record2 =
      GetFakeAAAARecord(domain_, std::chrono::seconds(1000));
  TestUniqueRecordRegistrationWorkflow(record1, record2);
}

TEST_F(MdnsPublisherTest, TXTRecordRegistrationWorkflow) {
  const MdnsRecord record1 = GetFakeTxtRecord(domain_);
  const MdnsRecord record2 =
      GetFakeTxtRecord(domain_, std::chrono::seconds(1000));
  TestUniqueRecordRegistrationWorkflow(record1, record2);
}

TEST_F(MdnsPublisherTest, SRVRecordRegistrationWorkflow) {
  const MdnsRecord record1 = GetFakeSrvRecord(domain_);
  const MdnsRecord record2 =
      GetFakeSrvRecord(domain_, std::chrono::seconds(1000));
  TestUniqueRecordRegistrationWorkflow(record1, record2);
}

TEST_F(MdnsPublisherTest, PTRRecordRegistrationWorkflow) {
  const MdnsRecord record = GetFakePtrRecord(domain_);
  const MdnsRecord record2 =
      GetFakePtrRecord(domain_, std::chrono::seconds(1000));

  EXPECT_CALL(probe_manager_, IsDomainClaimed(domain_))
      .WillRepeatedly(Return(true));
  DnsType type = DnsType::kPTR;

  // Check preconditions.
  ASSERT_EQ(record.dns_type(), record2.dns_type());
  ASSERT_EQ(publisher_.GetRecordCount(), size_t{0});
  auto records = publisher_.GetRecords(domain_, type, DnsClass::kIN);
  ASSERT_EQ(records.size(), size_t{0});
  records = publisher_.GetRecords(ptr_domain_, type, DnsClass::kIN);
  ASSERT_EQ(records.size(), size_t{0});
  ASSERT_NE(record, record2);
  ASSERT_TRUE(records.empty());
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{0});

  // Register a new record.
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  EXPECT_TRUE(publisher_.RegisterRecord(record).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {record});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{1});

  // Re-register the same record.
  EXPECT_FALSE(publisher_.RegisterRecord(record).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {record});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{1});

  // Register a second record.
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record2](const MdnsMessage& message) -> Error {
        return IsAnnounced(record2, message);
      });
  EXPECT_TRUE(publisher_.RegisterRecord(record2).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {record, record2});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{2});

  // Delete an existing record.
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record2](const MdnsMessage& message) -> Error {
        return IsGoodbyeRecord(record2, message);
      });
  EXPECT_TRUE(publisher_.UnregisterRecord(record2).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {record});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{1});

  // Delete a non-existing record.
  EXPECT_FALSE(publisher_.UnregisterRecord(record2).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {record});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{1});

  // Delete the last record
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsGoodbyeRecord(record, message);
      });
  EXPECT_TRUE(publisher_.UnregisterRecord(record).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  CheckPublishedRecords(ptr_domain_, type, {});
  ASSERT_EQ(publisher_.GetPtrRecords(DnsClass::kANY).size(), size_t{0});
}

TEST_F(MdnsPublisherTest, RegisteringUnownedRecordsFail) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(domain_))
      .WillRepeatedly(Return(false));
  EXPECT_FALSE(publisher_.RegisterRecord(GetFakePtrRecord(domain_)).ok());
  EXPECT_FALSE(publisher_.RegisterRecord(GetFakeSrvRecord(domain_)).ok());
  EXPECT_FALSE(publisher_.RegisterRecord(GetFakeTxtRecord(domain_)).ok());
  EXPECT_FALSE(publisher_.RegisterRecord(GetFakeARecord(domain_)).ok());
  EXPECT_FALSE(publisher_.RegisterRecord(GetFakeAAAARecord(domain_)).ok());
}

TEST_F(MdnsPublisherTest, RegistrationAnnouncesEightTimes) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(domain_))
      .WillRepeatedly(Return(true));
  constexpr Clock::duration kOneSecond =
      Clock::to_duration(std::chrono::seconds(1));

  // First announce, at registration.
  const MdnsRecord record = GetFakeARecord(domain_);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  EXPECT_TRUE(publisher_.RegisterRecord(record).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);

  // Second announce, at 2 seconds.
  testing::Mock::VerifyAndClearExpectations(&sender_);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Third announce, at 4 seconds.
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Fourth announce, at 8 seconds.
  clock_.Advance(kOneSecond * 3);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Fifth announce, at 16 seconds.
  clock_.Advance(kOneSecond * 7);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Sixth announce, at 32 seconds.
  clock_.Advance(kOneSecond * 15);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Seventh announce, at 64 seconds.
  clock_.Advance(kOneSecond * 31);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Eighth announce, at 128 seconds.
  clock_.Advance(kOneSecond * 63);
  clock_.Advance(kAnnounceGoodbyeDelay);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsAnnounced(record, message);
      });
  clock_.Advance(kOneSecond);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // No more announcements
  clock_.Advance(kOneSecond * 1024);
  clock_.Advance(kAnnounceGoodbyeDelay);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // Sends goodbye message when removed.
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &record](const MdnsMessage& message) -> Error {
        return IsGoodbyeRecord(record, message);
      });
  EXPECT_TRUE(publisher_.UnregisterRecord(record).ok());
  clock_.Advance(kAnnounceGoodbyeDelay);
}

}  // namespace discovery
}  // namespace openscreen
