// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_responder.h"

#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_probe_manager.h"
#include "discovery/mdns/mdns_random.h"
#include "discovery/mdns/mdns_receiver.h"
#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/mdns_sender.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {
namespace {

constexpr Clock::duration kMaximumSharedRecordResponseDelayMs(120 * 1000);

bool ContainsRecordType(const std::vector<MdnsRecord>& records, DnsType type) {
  return ContainsIf(records, [type](const MdnsRecord& record) {
    return record.dns_type() == type;
  });
}

void CheckSingleNsecRecordType(const MdnsMessage& message, DnsType type) {
  ASSERT_EQ(message.answers().size(), size_t{1});
  const MdnsRecord record = message.answers()[0];

  ASSERT_EQ(record.dns_type(), DnsType::kNSEC);
  const NsecRecordRdata& rdata = absl::get<NsecRecordRdata>(record.rdata());

  ASSERT_EQ(rdata.types().size(), size_t{1});
  EXPECT_EQ(rdata.types()[0], type);
}

void CheckPtrDomain(const MdnsRecord& record, const DomainName& domain) {
  ASSERT_EQ(record.dns_type(), DnsType::kPTR);
  const PtrRecordRdata& rdata = absl::get<PtrRecordRdata>(record.rdata());

  EXPECT_EQ(rdata.ptr_domain(), domain);
}

void ExpectContainsNsecRecordType(const std::vector<MdnsRecord>& records,
                                  DnsType type) {
  EXPECT_TRUE(ContainsIf(records, [type](const MdnsRecord& record) {
    if (record.dns_type() != DnsType::kNSEC) {
      return false;
    }

    const NsecRecordRdata& rdata = absl::get<NsecRecordRdata>(record.rdata());
    return rdata.types().size() == 1 && rdata.types()[0] == type;
  }));
}

}  // namespace

using testing::_;
using testing::Args;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class MockRecordHandler : public MdnsResponder::RecordHandler {
 public:
  void AddRecord(MdnsRecord record) { records_.push_back(record); }

  MOCK_METHOD3(HasRecords, bool(const DomainName&, DnsType, DnsClass));

  std::vector<MdnsRecord::ConstRef> GetRecords(const DomainName& name,
                                               DnsType type,
                                               DnsClass clazz) override {
    std::vector<MdnsRecord::ConstRef> records;
    for (const auto& record : records_) {
      if (type == DnsType::kANY || record.dns_type() == type) {
        records.push_back(record);
      }
    }

    return records;
  }

  std::vector<MdnsRecord::ConstRef> GetPtrRecords(DnsClass clazz) override {
    std::vector<MdnsRecord::ConstRef> records;
    for (const auto& record : records_) {
      if (record.dns_type() == DnsType::kPTR) {
        records.push_back(record);
      }
    }

    return records;
  }

 private:
  std::vector<MdnsRecord> records_;
};

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

class MdnsResponderTest : public testing::Test {
 public:
  MdnsResponderTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        socket_(&task_runner_),
        sender_(&socket_),
        receiver_(config_),
        responder_(&record_handler_,
                   &probe_manager_,
                   &sender_,
                   &receiver_,
                   &task_runner_,
                   FakeClock::now,
                   &random_,
                   config_) {}

 protected:
  MdnsRecord GetFakePtrRecord(const DomainName& target) {
    DomainName name(++target.labels().begin(), target.labels().end());
    PtrRecordRdata rdata(target);
    return MdnsRecord(std::move(name), DnsType::kPTR, DnsClass::kIN,
                      RecordType::kUnique, std::chrono::seconds(0), rdata);
  }

  MdnsRecord GetFakeSrvRecord(const DomainName& name) {
    SrvRecordRdata rdata(0, 0, 80, name);
    return MdnsRecord(name, DnsType::kSRV, DnsClass::kIN, RecordType::kUnique,
                      std::chrono::seconds(0), rdata);
  }

  MdnsRecord GetFakeTxtRecord(const DomainName& name) {
    TxtRecordRdata rdata;
    return MdnsRecord(name, DnsType::kTXT, DnsClass::kIN, RecordType::kUnique,
                      std::chrono::seconds(0), rdata);
  }

  MdnsRecord GetFakeARecord(const DomainName& name) {
    ARecordRdata rdata(IPAddress(192, 168, 0, 0));
    return MdnsRecord(name, DnsType::kA, DnsClass::kIN, RecordType::kUnique,
                      std::chrono::seconds(0), rdata);
  }

  MdnsRecord GetFakeAAAARecord(const DomainName& name) {
    AAAARecordRdata rdata(IPAddress(1, 2, 3, 4, 5, 6, 7, 8));
    return MdnsRecord(name, DnsType::kAAAA, DnsClass::kIN, RecordType::kUnique,
                      std::chrono::seconds(0), rdata);
  }

  void OnMessageReceived(const MdnsMessage& message, const IPEndpoint& src) {
    responder_.OnMessageReceived(message, src);
  }

  void QueryForRecordTypeWhenNonePresent(DnsType type) {
    MdnsQuestion question(domain_, type, DnsClass::kANY,
                          ResponseType::kMulticast);
    MdnsMessage message(0, MessageType::Query);
    message.AddQuestion(question);

    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce([type](const MdnsMessage& msg) -> Error {
          CheckSingleNsecRecordType(msg, type);
          return Error::None();
        });
    EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
    EXPECT_CALL(record_handler_, HasRecords(_, _, _))
        .WillRepeatedly(Return(true));
    OnMessageReceived(message, endpoint_);
  }

  MdnsMessage CreateMulticastMdnsQuery(DnsType type) {
    MdnsQuestion question(domain_, type, DnsClass::kANY,
                          ResponseType::kMulticast);
    MdnsMessage message(0, MessageType::Query);
    message.AddQuestion(std::move(question));

    return message;
  }

  MdnsMessage CreateTypeEnumerationQuery() {
    MdnsQuestion question(type_enumeration_domain_, DnsType::kPTR,
                          DnsClass::kANY, ResponseType::kMulticast);
    MdnsMessage message(0, MessageType::Query);
    message.AddQuestion(std::move(question));

    return message;
  }

  const Config config_;
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeUdpSocket socket_;
  StrictMock<MockMdnsSender> sender_;
  StrictMock<MockRecordHandler> record_handler_;
  StrictMock<MockProbeManager> probe_manager_;
  MdnsReceiver receiver_;
  MdnsRandom random_;
  MdnsResponder responder_;

  DomainName domain_{"instance", "_googlecast", "_tcp", "local"};
  DomainName type_enumeration_domain_{"_services", "_dns-sd", "_udp", "local"};
  IPEndpoint endpoint_{IPAddress(192, 168, 0, 0), 80};
};

// Validate that when records may be sent from multiple receivers, the broadcast
// is delayed and it is not delayed otherwise.
TEST_F(MdnsResponderTest, OwnedRecordsSentImmediately) {
  MdnsMessage message = CreateMulticastMdnsQuery(DnsType::kANY);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  OnMessageReceived(message, endpoint_);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  testing::Mock::VerifyAndClearExpectations(&record_handler_);
  testing::Mock::VerifyAndClearExpectations(&probe_manager_);

  EXPECT_CALL(sender_, SendMulticast(_)).Times(0);
  clock_.Advance(Clock::duration(kMaximumSharedRecordResponseDelayMs));
}

TEST_F(MdnsResponderTest, NonOwnedRecordsDelayed) {
  MdnsMessage message = CreateMulticastMdnsQuery(DnsType::kANY);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(false));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_)).Times(0);
  OnMessageReceived(message, endpoint_);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  testing::Mock::VerifyAndClearExpectations(&record_handler_);
  testing::Mock::VerifyAndClearExpectations(&probe_manager_);

  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  clock_.Advance(Clock::duration(kMaximumSharedRecordResponseDelayMs));
}

TEST_F(MdnsResponderTest, MultipleQuestionsProcessed) {
  MdnsMessage message = CreateMulticastMdnsQuery(DnsType::kANY);
  MdnsQuestion question2(domain_, DnsType::kANY, DnsClass::kANY,
                         ResponseType::kMulticast);
  message.AddQuestion(std::move(question2));

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  OnMessageReceived(message, endpoint_);
  testing::Mock::VerifyAndClearExpectations(&sender_);
  testing::Mock::VerifyAndClearExpectations(&record_handler_);
  testing::Mock::VerifyAndClearExpectations(&probe_manager_);

  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  clock_.Advance(Clock::duration(kMaximumSharedRecordResponseDelayMs));
}

// Validate that the correct messaging scheme (unicast vs multicast) is used.
TEST_F(MdnsResponderTest, UnicastMessageSentOverUnicast) {
  MdnsQuestion question(domain_, DnsType::kANY, DnsClass::kANY,
                        ResponseType::kUnicast);
  MdnsMessage message(0, MessageType::Query);
  message.AddQuestion(question);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMessage(_, endpoint_)).Times(1);
  OnMessageReceived(message, endpoint_);
}

TEST_F(MdnsResponderTest, MulticastMessageSentOverMulticast) {
  MdnsMessage message = CreateMulticastMdnsQuery(DnsType::kANY);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  OnMessageReceived(message, endpoint_);
}

// Validate that records are added as expected based on the query type, and that
// additional records are populated as specified in RFC 6762 and 6763.

// TODO(issuetracker.google.com/203003316): Refactor shared code from these
// tests into the test fixture, or consider a data driven test approach, to
// remove lots of duplication
TEST_F(MdnsResponderTest, AnyQueryResultsAllApplied) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{4});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kSRV));
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kTXT));
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kAAAA));
        EXPECT_FALSE(ContainsRecordType(message.answers(), DnsType::kPTR));
        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kANY), endpoint_);
}

TEST_F(MdnsResponderTest, PtrQueryResultsApplied) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{4});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kPTR));

        const auto& records = message.additional_records();
        EXPECT_EQ(records.size(), size_t{4});
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kSRV));
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kTXT));
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kA));
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kAAAA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kPTR));

        return Error::None();
      });

  DomainName ptr_domain{"_googlecast", "_tcp", "local"};
  MdnsQuestion question(ptr_domain, DnsType::kPTR, DnsClass::kANY,
                        ResponseType::kMulticast);
  MdnsMessage message(0, MessageType::Query);
  message.AddQuestion(question);
  OnMessageReceived(message, endpoint_);
}

TEST_F(MdnsResponderTest, SrvQueryResultsApplied) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{2});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kSRV));

        const auto& records = message.additional_records();
        EXPECT_EQ(records.size(), size_t{2});
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kA));
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kAAAA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kSRV));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kTXT));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kPTR));

        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kSRV), endpoint_);
}

TEST_F(MdnsResponderTest, AQueryResultsApplied) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{1});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));

        const auto& records = message.additional_records();
        EXPECT_EQ(records.size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kAAAA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kSRV));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kTXT));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kPTR));

        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kA), endpoint_);
}

TEST_F(MdnsResponderTest, AAAAQueryResultsApplied) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{1});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kAAAA));

        const auto& records = message.additional_records();
        EXPECT_EQ(records.size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(records, DnsType::kA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kAAAA));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kSRV));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kTXT));
        EXPECT_FALSE(ContainsRecordType(records, DnsType::kPTR));

        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kAAAA), endpoint_);
}

TEST_F(MdnsResponderTest, MessageOnlySentIfAnswerNotKnown) {
  MdnsMessage message = CreateMulticastMdnsQuery(DnsType::kAAAA);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message.AddAnswer(aaaa_record);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(aaaa_record);

  OnMessageReceived(message, endpoint_);
}

TEST_F(MdnsResponderTest, RecordOnlySentIfNotKnown) {
  MdnsMessage message_any = CreateMulticastMdnsQuery(DnsType::kANY);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message_any.AddAnswer(aaaa_record);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(aaaa_record);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));
        return Error::None();
      });

  OnMessageReceived(message_any, endpoint_);
}

TEST_F(MdnsResponderTest, RecordOnlySentIfNotKnownMultiplePackets) {
  MdnsMessage message_any = CreateMulticastMdnsQuery(DnsType::kANY);
  message_any.set_truncated();

  MdnsMessage message_query(1, MessageType::Query);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message_query.AddAnswer(aaaa_record);

  OnMessageReceived(message_any, endpoint_);
  OnMessageReceived(message_query, endpoint_);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(aaaa_record);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));
        return Error::None();
      });
  clock_.Advance(std::chrono::seconds(1));
}

TEST_F(MdnsResponderTest, RecordOnlySentIfNotKnownMultiplePacketsOutOfOrder) {
  MdnsMessage message_any = CreateMulticastMdnsQuery(DnsType::kANY);
  message_any.set_truncated();

  MdnsMessage message_query1(2, MessageType::Query);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message_query1.AddAnswer(aaaa_record);
  message_query1.set_truncated();

  MdnsMessage message_query2(3, MessageType::Query);
  MdnsRecord a_record = GetFakeARecord(domain_);
  message_query2.AddAnswer(a_record);

  OnMessageReceived(message_query1, endpoint_);
  OnMessageReceived(message_query2, endpoint_);
  OnMessageReceived(message_any, endpoint_);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(a_record);
  record_handler_.AddRecord(aaaa_record);
  record_handler_.AddRecord(aaaa_record);
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kSRV));
        return Error::None();
      });
  clock_.Advance(std::chrono::seconds(1));
}

TEST_F(MdnsResponderTest, RecordSentForMultiPacketsSuppressionIfMoreNotFound) {
  MdnsMessage message_any = CreateMulticastMdnsQuery(DnsType::kANY);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message_any.AddAnswer(aaaa_record);
  message_any.set_truncated();

  OnMessageReceived(message_any, endpoint_);

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  record_handler_.AddRecord(aaaa_record);
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});
        EXPECT_EQ(message.additional_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));
        return Error::None();
      });
  clock_.Advance(std::chrono::seconds(1));
}

TEST_F(MdnsResponderTest, RecordNotSentForMultiPacketsSuppressionIfNoQuery) {
  MdnsMessage message(1, MessageType::Query);
  MdnsRecord aaaa_record = GetFakeAAAARecord(domain_);
  message.AddAnswer(aaaa_record);

  OnMessageReceived(message, endpoint_);
  clock_.Advance(std::chrono::seconds(1));
}

// Validate NSEC records are used correctly.
TEST_F(MdnsResponderTest, QueryForRecordTypesWhenNonePresent) {
  QueryForRecordTypeWhenNonePresent(DnsType::kANY);
  QueryForRecordTypeWhenNonePresent(DnsType::kSRV);
  QueryForRecordTypeWhenNonePresent(DnsType::kTXT);
  QueryForRecordTypeWhenNonePresent(DnsType::kA);
  QueryForRecordTypeWhenNonePresent(DnsType::kAAAA);
}

TEST_F(MdnsResponderTest, AAAAQueryGiveANsec) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeAAAARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kAAAA));

        EXPECT_EQ(message.additional_records().size(), size_t{1});
        ExpectContainsNsecRecordType(message.additional_records(), DnsType::kA);

        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kAAAA), endpoint_);
}

TEST_F(MdnsResponderTest, AQueryGiveAAAANsec) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kA));

        EXPECT_EQ(message.additional_records().size(), size_t{1});
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kAAAA);

        return Error::None();
      });

  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kA), endpoint_);
}

TEST_F(MdnsResponderTest, SrvQueryGiveCorrectNsecForNoAOrAAAA) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kSRV));

        EXPECT_EQ(message.additional_records().size(), size_t{2});
        ExpectContainsNsecRecordType(message.additional_records(), DnsType::kA);
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kAAAA);

        return Error::None();
      });
  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kSRV), endpoint_);
}

TEST_F(MdnsResponderTest, SrvQueryGiveCorrectNsec) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kSRV));

        EXPECT_EQ(message.additional_records().size(), size_t{2});
        EXPECT_TRUE(
            ContainsRecordType(message.additional_records(), DnsType::kA));
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kAAAA);

        return Error::None();
      });
  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kSRV), endpoint_);
}

TEST_F(MdnsResponderTest, PtrQueryGiveCorrectNsecForNoPtrOrSrv) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kPTR));

        EXPECT_EQ(message.additional_records().size(), size_t{2});
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kTXT);
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kSRV);

        return Error::None();
      });
  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kPTR), endpoint_);
}

TEST_F(MdnsResponderTest, PtrQueryGiveCorrectNsecForOnlyPtr) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kPTR));

        EXPECT_EQ(message.additional_records().size(), size_t{2});
        EXPECT_TRUE(
            ContainsRecordType(message.additional_records(), DnsType::kTXT));
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kSRV);

        return Error::None();
      });
  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kPTR), endpoint_);
}

TEST_F(MdnsResponderTest, PtrQueryGiveCorrectNsecForOnlySrv) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(true));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  record_handler_.AddRecord(GetFakePtrRecord(domain_));
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kPTR));

        EXPECT_EQ(message.additional_records().size(), size_t{4});
        EXPECT_TRUE(
            ContainsRecordType(message.additional_records(), DnsType::kSRV));
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kTXT);
        ExpectContainsNsecRecordType(message.additional_records(), DnsType::kA);
        ExpectContainsNsecRecordType(message.additional_records(),
                                     DnsType::kAAAA);

        return Error::None();
      });
  OnMessageReceived(CreateMulticastMdnsQuery(DnsType::kPTR), endpoint_);
}

TEST_F(MdnsResponderTest, EnumerateAllQuery) {
  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(false));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  const auto ptr = GetFakePtrRecord(domain_);
  record_handler_.AddRecord(ptr);
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));

  OnMessageReceived(CreateTypeEnumerationQuery(), endpoint_);

  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce([this, &ptr](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{0});
        EXPECT_EQ(message.authority_records().size(), size_t{0});

        EXPECT_EQ(message.answers().size(), size_t{1});
        EXPECT_TRUE(ContainsRecordType(message.answers(), DnsType::kPTR));
        EXPECT_EQ(message.answers()[0].name(), type_enumeration_domain_);
        CheckPtrDomain(message.answers()[0], ptr.name());
        return Error::None();
      });
  clock_.Advance(Clock::duration(kMaximumSharedRecordResponseDelayMs));
}

TEST_F(MdnsResponderTest, EnumerateAllQueryNoResults) {
  MdnsMessage message = CreateTypeEnumerationQuery();

  EXPECT_CALL(probe_manager_, IsDomainClaimed(_)).WillOnce(Return(false));
  EXPECT_CALL(record_handler_, HasRecords(_, _, _))
      .WillRepeatedly(Return(true));
  const auto ptr = GetFakePtrRecord(domain_);
  record_handler_.AddRecord(GetFakeSrvRecord(domain_));
  record_handler_.AddRecord(GetFakeTxtRecord(domain_));
  record_handler_.AddRecord(GetFakeARecord(domain_));
  OnMessageReceived(message, endpoint_);
  clock_.Advance(Clock::duration(kMaximumSharedRecordResponseDelayMs));
}

}  // namespace discovery
}  // namespace openscreen
