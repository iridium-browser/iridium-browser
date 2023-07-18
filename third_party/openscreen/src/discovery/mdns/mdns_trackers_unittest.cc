// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_trackers.h"

#include <memory>
#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_random.h"
#include "discovery/mdns/mdns_record_changed_callback.h"
#include "discovery/mdns/mdns_sender.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"
#include "platform/test/fake_udp_socket.h"

namespace openscreen {
namespace discovery {
namespace {

constexpr Clock::duration kOneSecond =
    Clock::to_duration(std::chrono::seconds(1));
}

using testing::_;
using testing::Args;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

ACTION_P2(VerifyMessageBytesWithoutId, expected_data, expected_size) {
  const uint8_t* actual_data = reinterpret_cast<const uint8_t*>(arg0);
  const size_t actual_size = arg1;
  ASSERT_EQ(actual_size, expected_size);
  // Start at bytes[2] to skip a generated message ID.
  for (size_t i = 2; i < actual_size; ++i) {
    EXPECT_EQ(actual_data[i], expected_data[i]);
  }
}

ACTION_P(VerifyTruncated, is_truncated) {
  EXPECT_EQ(arg0.is_truncated(), is_truncated);
}

ACTION_P(VerifyRecordCount, record_count) {
  EXPECT_EQ(arg0.answers().size(), static_cast<size_t>(record_count));
}

class MockMdnsSender : public MdnsSender {
 public:
  explicit MockMdnsSender(UdpSocket* socket) : MdnsSender(socket) {}

  MOCK_METHOD1(SendMulticast, Error(const MdnsMessage&));
  MOCK_METHOD2(SendMessage, Error(const MdnsMessage&, const IPEndpoint&));
};

class MockRecordChangedCallback : public MdnsRecordChangedCallback {
 public:
  MOCK_METHOD(std::vector<PendingQueryChange>,
              OnRecordChanged,
              (const MdnsRecord&, RecordChangedEvent event),
              (override));
};

class MdnsTrackerTest : public testing::Test {
 public:
  MdnsTrackerTest()
      : clock_(Clock::now()),
        task_runner_(&clock_),
        socket_(&task_runner_),
        sender_(&socket_),
        a_question_(DomainName{"testing", "local"},
                    DnsType::kANY,
                    DnsClass::kIN,
                    ResponseType::kMulticast),
        a_record_(DomainName{"testing", "local"},
                  DnsType::kA,
                  DnsClass::kIN,
                  RecordType::kShared,
                  std::chrono::seconds(120),
                  ARecordRdata(IPAddress{172, 0, 0, 1})),
        nsec_record_(
            DomainName{"testing", "local"},
            DnsType::kNSEC,
            DnsClass::kIN,
            RecordType::kShared,
            std::chrono::seconds(120),
            NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kA)) {}

  template <class TrackerType>
  void TrackerNoQueryAfterDestruction(TrackerType tracker) {
    tracker.reset();
    // Advance fake clock by a long time interval to make sure if there's a
    // scheduled task, it will run.
    clock_.Advance(std::chrono::hours(1));
  }

  std::unique_ptr<MdnsRecordTracker> CreateRecordTracker(
      const MdnsRecord& record,
      DnsType type) {
    return std::make_unique<MdnsRecordTracker>(
        record, type, &sender_, &task_runner_, &FakeClock::now, &random_,
        [this](const MdnsRecordTracker*, const MdnsRecord&) {
          expiration_called_ = true;
        });
  }

  std::unique_ptr<MdnsRecordTracker> CreateRecordTracker(
      const MdnsRecord& record) {
    return CreateRecordTracker(record, record.dns_type());
  }

  std::unique_ptr<MdnsQuestionTracker> CreateQuestionTracker(
      const MdnsQuestion& question,
      MdnsQuestionTracker::QueryType query_type =
          MdnsQuestionTracker::QueryType::kContinuous) {
    return std::make_unique<MdnsQuestionTracker>(question, &sender_,
                                                 &task_runner_, &FakeClock::now,
                                                 &random_, config_, query_type);
  }

 protected:
  void AdvanceThroughAllTtlFractions(std::chrono::seconds ttl) {
    constexpr double kTtlFractions[] = {0.83, 0.88, 0.93, 0.98, 1.00};
    Clock::duration time_passed{0};
    for (double fraction : kTtlFractions) {
      Clock::duration time_till_refresh = Clock::to_duration(ttl * fraction);
      Clock::duration delta = time_till_refresh - time_passed;
      time_passed = time_till_refresh;
      clock_.Advance(delta);
    }
  }

  const MdnsRecord& GetRecord(MdnsRecordTracker* tracker) {
    return tracker->record_;
  }

  // clang-format off
  const std::vector<uint8_t> kQuestionQueryBytes = {
      0x00, 0x00,  // ID = 0
      0x00, 0x00,  // FLAGS = None
      0x00, 0x01,  // Question count
      0x00, 0x00,  // Answer count
      0x00, 0x00,  // Authority count
      0x00, 0x00,  // Additional count
      // Question
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0xFF,  // TYPE = ANY (255)
      0x00, 0x01,  // CLASS = IN (1)
  };

  const std::vector<uint8_t> kRecordQueryBytes = {
      0x00, 0x00,  // ID = 0
      0x00, 0x00,  // FLAGS = None
      0x00, 0x01,  // Question count
      0x00, 0x00,  // Answer count
      0x00, 0x00,  // Authority count
      0x00, 0x00,  // Additional count
      // Question
      0x07, 't', 'e', 's', 't', 'i', 'n', 'g',
      0x05, 'l', 'o', 'c', 'a', 'l',
      0x00,
      0x00, 0x01,  // TYPE = A (1)
      0x00, 0x01,  // CLASS = IN (1)
  };

  // clang-format on
  Config config_;
  FakeClock clock_;
  FakeTaskRunner task_runner_;
  FakeUdpSocket socket_;
  StrictMock<MockMdnsSender> sender_;
  MdnsRandom random_;

  MdnsQuestion a_question_;
  MdnsRecord a_record_;
  MdnsRecord nsec_record_;

  bool expiration_called_ = false;
};

// Records are re-queried at 80%, 85%, 90% and 95% TTL as per RFC 6762
// Section 5.2 There are no subsequent queries to refresh the record after that,
// the record is expired after TTL has passed since the start of tracking.
// Random variance required is from 0% to 2%, making these times at most 82%,
// 87%, 92% and 97% TTL. Fake clock is advanced to 83%, 88%, 93% and 98% to make
// sure that task gets executed.
// https://tools.ietf.org/html/rfc6762#section-5.2

TEST_F(MdnsTrackerTest, RecordTrackerRecordAccessor) {
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  EXPECT_EQ(GetRecord(tracker.get()), a_record_);
}

TEST_F(MdnsTrackerTest, RecordTrackerQueryAfterDelayPerQuestionTracker) {
  std::unique_ptr<MdnsQuestionTracker> question = CreateQuestionTracker(
      a_question_, MdnsQuestionTracker::QueryType::kOneShot);
  std::unique_ptr<MdnsQuestionTracker> question2 = CreateQuestionTracker(
      a_question_, MdnsQuestionTracker::QueryType::kOneShot);
  EXPECT_CALL(sender_, SendMulticast(_)).Times(2);
  clock_.Advance(kOneSecond);
  clock_.Advance(kOneSecond);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);

  // No queries without an associated tracker.
  AdvanceThroughAllTtlFractions(a_record_.ttl());
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // 4 queries with one associated tracker.
  tracker = CreateRecordTracker(a_record_);
  tracker->AddAssociatedQuery(question.get());
  EXPECT_CALL(sender_, SendMulticast(_)).Times(4);
  AdvanceThroughAllTtlFractions(a_record_.ttl());
  testing::Mock::VerifyAndClearExpectations(&sender_);

  // 8 queries with two associated trackers.
  tracker = CreateRecordTracker(a_record_);
  tracker->AddAssociatedQuery(question.get());
  tracker->AddAssociatedQuery(question2.get());
  EXPECT_CALL(sender_, SendMulticast(_)).Times(8);
  AdvanceThroughAllTtlFractions(a_record_.ttl());
}

TEST_F(MdnsTrackerTest, RecordTrackerSendsMessage) {
  std::unique_ptr<MdnsQuestionTracker> question = CreateQuestionTracker(
      a_question_, MdnsQuestionTracker::QueryType::kOneShot);
  EXPECT_CALL(sender_, SendMulticast(_)).Times(1);
  clock_.Advance(kOneSecond);
  clock_.Advance(kOneSecond);
  testing::Mock::VerifyAndClearExpectations(&sender_);

  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  tracker->AddAssociatedQuery(question.get());

  EXPECT_CALL(sender_, SendMulticast(_))
      .Times(1)
      .WillRepeatedly([this](const MdnsMessage& message) -> Error {
        EXPECT_EQ(message.questions().size(), size_t{1});
        EXPECT_EQ(message.questions()[0], a_question_);
        return Error::None();
      });

  clock_.Advance(Clock::to_duration(a_record_.ttl() * 0.83));
}

TEST_F(MdnsTrackerTest, RecordTrackerNoQueryAfterDestruction) {
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  TrackerNoQueryAfterDestruction(std::move(tracker));
}

TEST_F(MdnsTrackerTest, RecordTrackerNoQueryAfterLateTask) {
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  // If task runner was too busy and callback happened too late, there should be
  // no query and instead the record will expire.
  // Check lower bound for task being late (TTL) and an arbitrarily long time
  // interval to ensure the query is not sent a later time.
  clock_.Advance(a_record_.ttl());
  clock_.Advance(std::chrono::hours(1));
}

TEST_F(MdnsTrackerTest, RecordTrackerUpdateResetsTtl) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  // Advance time by 60% of record's TTL
  Clock::duration advance_time = Clock::to_duration(a_record_.ttl() * 0.6);
  clock_.Advance(advance_time);
  // Now update the record, this must reset expiration time
  EXPECT_EQ(tracker->Update(a_record_).value(),
            MdnsRecordTracker::UpdateType::kTTLOnly);
  // Advance time by 60% of record's TTL again
  clock_.Advance(advance_time);
  // Check that expiration callback was not called
  EXPECT_FALSE(expiration_called_);
}

TEST_F(MdnsTrackerTest, RecordTrackerForceExpiration) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  tracker->ExpireSoon();
  // Expire schedules expiration after 1 second.
  clock_.Advance(std::chrono::seconds(1));
  EXPECT_TRUE(expiration_called_);
}

TEST_F(MdnsTrackerTest, NsecRecordTrackerForceExpiration) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker =
      CreateRecordTracker(nsec_record_, DnsType::kA);
  tracker->ExpireSoon();
  // Expire schedules expiration after 1 second.
  clock_.Advance(std::chrono::seconds(1));
  EXPECT_TRUE(expiration_called_);
}

TEST_F(MdnsTrackerTest, RecordTrackerExpirationCallback) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  clock_.Advance(a_record_.ttl());
  EXPECT_TRUE(expiration_called_);
}

TEST_F(MdnsTrackerTest, RecordTrackerExpirationCallbackAfterGoodbye) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  MdnsRecord goodbye_record(a_record_.name(), a_record_.dns_type(),
                            a_record_.dns_class(), a_record_.record_type(),
                            std::chrono::seconds(0), a_record_.rdata());

  // After a goodbye record is received, expiration is schedule in a second.
  EXPECT_EQ(tracker->Update(goodbye_record).value(),
            MdnsRecordTracker::UpdateType::kGoodbye);

  // Advance clock to just before the expiration time of 1 second.
  clock_.Advance(std::chrono::microseconds(999999));
  EXPECT_FALSE(expiration_called_);
  // Advance clock to exactly the expiration time.
  clock_.Advance(std::chrono::microseconds(1));
  EXPECT_TRUE(expiration_called_);
}

TEST_F(MdnsTrackerTest, RecordTrackerInvalidPositiveRecordUpdate) {
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);

  MdnsRecord invalid_name(DomainName{"invalid"}, a_record_.dns_type(),
                          a_record_.dns_class(), a_record_.record_type(),
                          a_record_.ttl(), a_record_.rdata());
  EXPECT_EQ(tracker->Update(invalid_name).error(),
            Error::Code::kParameterInvalid);

  MdnsRecord invalid_type(a_record_.name(), DnsType::kPTR,
                          a_record_.dns_class(), a_record_.record_type(),
                          a_record_.ttl(),
                          PtrRecordRdata{DomainName{"invalid"}});
  EXPECT_EQ(tracker->Update(invalid_type).error(),
            Error::Code::kParameterInvalid);

  MdnsRecord invalid_class(a_record_.name(), a_record_.dns_type(),
                           DnsClass::kANY, a_record_.record_type(),
                           a_record_.ttl(), a_record_.rdata());
  EXPECT_EQ(tracker->Update(invalid_class).error(),
            Error::Code::kParameterInvalid);

  // RDATA must match the old RDATA for goodbye records
  MdnsRecord invalid_rdata(a_record_.name(), a_record_.dns_type(),
                           a_record_.dns_class(), a_record_.record_type(),
                           std::chrono::seconds(0),
                           ARecordRdata(IPAddress{172, 0, 0, 2}));
  EXPECT_EQ(tracker->Update(invalid_rdata).error(),
            Error::Code::kParameterInvalid);
}

TEST_F(MdnsTrackerTest, RecordTrackerUpdatePositiveResponseWithNegative) {
  // Check valid update.
  std::unique_ptr<MdnsRecordTracker> tracker =
      CreateRecordTracker(a_record_, DnsType::kA);
  auto result = tracker->Update(nsec_record_);
  ASSERT_TRUE(result.is_value());
  EXPECT_EQ(result.value(), MdnsRecordTracker::UpdateType::kRdata);
  EXPECT_EQ(GetRecord(tracker.get()), nsec_record_);

  // Check invalid update.
  MdnsRecord non_a_nsec_record(
      nsec_record_.name(), nsec_record_.dns_type(), nsec_record_.dns_class(),
      nsec_record_.record_type(), nsec_record_.ttl(),
      NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kAAAA));
  tracker = CreateRecordTracker(a_record_, DnsType::kA);
  auto response = tracker->Update(non_a_nsec_record);
  ASSERT_TRUE(response.is_error());
  EXPECT_EQ(GetRecord(tracker.get()), a_record_);
}

TEST_F(MdnsTrackerTest, RecordTrackerUpdateNegativeResponseWithNegative) {
  // Check valid update.
  std::unique_ptr<MdnsRecordTracker> tracker =
      CreateRecordTracker(nsec_record_, DnsType::kA);
  MdnsRecord multiple_nsec_record(
      nsec_record_.name(), nsec_record_.dns_type(), nsec_record_.dns_class(),
      nsec_record_.record_type(), nsec_record_.ttl(),
      NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kA,
                      DnsType::kAAAA));
  auto result = tracker->Update(multiple_nsec_record);
  ASSERT_TRUE(result.is_value());
  EXPECT_EQ(result.value(), MdnsRecordTracker::UpdateType::kRdata);
  EXPECT_EQ(GetRecord(tracker.get()), multiple_nsec_record);

  // Check invalid update.
  tracker = CreateRecordTracker(nsec_record_, DnsType::kA);
  MdnsRecord non_a_nsec_record(
      nsec_record_.name(), nsec_record_.dns_type(), nsec_record_.dns_class(),
      nsec_record_.record_type(), nsec_record_.ttl(),
      NsecRecordRdata(DomainName{"testing", "local"}, DnsType::kAAAA));
  auto response = tracker->Update(non_a_nsec_record);
  EXPECT_TRUE(response.is_error());
  EXPECT_EQ(GetRecord(tracker.get()), nsec_record_);
}

TEST_F(MdnsTrackerTest, RecordTrackerUpdateNegativeResponseWithPositive) {
  // Check valid update.
  std::unique_ptr<MdnsRecordTracker> tracker =
      CreateRecordTracker(nsec_record_, DnsType::kA);
  auto result = tracker->Update(a_record_);
  ASSERT_TRUE(result.is_value());
  EXPECT_EQ(result.value(), MdnsRecordTracker::UpdateType::kRdata);
  EXPECT_EQ(GetRecord(tracker.get()), a_record_);

  // Check invalid update.
  tracker = CreateRecordTracker(nsec_record_, DnsType::kA);
  MdnsRecord aaaa_record(a_record_.name(), DnsType::kAAAA,
                         a_record_.dns_class(), a_record_.record_type(),
                         std::chrono::seconds(0),
                         AAAARecordRdata(IPAddress{0, 0, 0, 0, 0, 0, 0, 1}));
  result = tracker->Update(aaaa_record);
  EXPECT_TRUE(result.is_error());
  EXPECT_EQ(GetRecord(tracker.get()), nsec_record_);
}

TEST_F(MdnsTrackerTest, RecordTrackerNoExpirationCallbackAfterDestruction) {
  expiration_called_ = false;
  std::unique_ptr<MdnsRecordTracker> tracker = CreateRecordTracker(a_record_);
  tracker.reset();
  clock_.Advance(a_record_.ttl());
  EXPECT_FALSE(expiration_called_);
}

// Initial query is delayed for up to 120 ms as per RFC 6762 Section 5.2
// Subsequent queries happen no sooner than a second after the initial query and
// the interval between the queries increases at least by a factor of 2 for each
// next query up until it's capped at 1 hour.
// https://tools.ietf.org/html/rfc6762#section-5.2

TEST_F(MdnsTrackerTest, QuestionTrackerQuestionAccessor) {
  std::unique_ptr<MdnsQuestionTracker> tracker =
      CreateQuestionTracker(a_question_);
  EXPECT_EQ(tracker->question(), a_question_);
}

TEST_F(MdnsTrackerTest, QuestionTrackerQueryAfterDelay) {
  std::unique_ptr<MdnsQuestionTracker> tracker =
      CreateQuestionTracker(a_question_);

  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce(
          DoAll(WithArgs<0>(VerifyTruncated(false)), Return(Error::None())));
  clock_.Advance(std::chrono::milliseconds(120));

  std::chrono::seconds interval{1};
  while (interval < std::chrono::hours(1)) {
    EXPECT_CALL(sender_, SendMulticast(_))
        .WillOnce(
            DoAll(WithArgs<0>(VerifyTruncated(false)), Return(Error::None())));
    clock_.Advance(interval);
    interval *= 2;
  }
}

TEST_F(MdnsTrackerTest, QuestionTrackerSendsMessage) {
  std::unique_ptr<MdnsQuestionTracker> tracker =
      CreateQuestionTracker(a_question_);

  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce(DoAll(
          WithArgs<0>(VerifyTruncated(false)),
          [this](const MdnsMessage& message) -> Error {
            EXPECT_EQ(message.questions().size(), size_t{1});
            EXPECT_EQ(message.questions()[0], a_question_);
            return Error::None();
          },
          Return(Error::None())));

  clock_.Advance(std::chrono::milliseconds(120));
}

TEST_F(MdnsTrackerTest, QuestionTrackerNoQueryAfterDestruction) {
  std::unique_ptr<MdnsQuestionTracker> tracker =
      CreateQuestionTracker(a_question_);
  TrackerNoQueryAfterDestruction(std::move(tracker));
}

TEST_F(MdnsTrackerTest, QuestionTrackerSendsMultipleMessages) {
  std::unique_ptr<MdnsQuestionTracker> tracker =
      CreateQuestionTracker(a_question_);

  std::vector<std::unique_ptr<MdnsRecordTracker>> answers;
  for (int i = 0; i < 100; i++) {
    auto record = CreateRecordTracker(a_record_);
    tracker->AddAssociatedRecord(record.get());
    answers.push_back(std::move(record));
  }

  EXPECT_CALL(sender_, SendMulticast(_))
      .WillOnce(DoAll(WithArgs<0>(VerifyTruncated(true)),
                      WithArgs<0>(VerifyRecordCount(49)),
                      Return(Error::None())))
      .WillOnce(DoAll(WithArgs<0>(VerifyTruncated(true)),
                      WithArgs<0>(VerifyRecordCount(50)),
                      Return(Error::None())))
      .WillOnce(DoAll(WithArgs<0>(VerifyTruncated(false)),
                      WithArgs<0>(VerifyRecordCount(1)),
                      Return(Error::None())));

  clock_.Advance(std::chrono::milliseconds(120));
}

}  // namespace discovery
}  // namespace openscreen
