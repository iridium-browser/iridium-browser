// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_trackers.h"

#include <array>
#include <limits>
#include <utility>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_random.h"
#include "discovery/mdns/mdns_record_changed_callback.h"
#include "discovery/mdns/mdns_sender.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {

namespace {

// RFC 6762 Section 5.2
// https://tools.ietf.org/html/rfc6762#section-5.2

// Attempt to refresh a record should be performed at 80%, 85%, 90% and 95% TTL.
constexpr double kTtlFractions[] = {0.80, 0.85, 0.90, 0.95, 1.00};

// Intervals between successive queries must increase by at least a factor of 2.
constexpr int kIntervalIncreaseFactor = 2;

// The interval between the first two queries must be at least one second.
constexpr std::chrono::seconds kMinimumQueryInterval{1};

// The querier may cap the question refresh interval to a maximum of 60 minutes.
constexpr std::chrono::minutes kMaximumQueryInterval{60};

// RFC 6762 Section 10.1
// https://tools.ietf.org/html/rfc6762#section-10.1

// A goodbye record is a record with TTL of 0.
bool IsGoodbyeRecord(const MdnsRecord& record) {
  return record.ttl() == std::chrono::seconds(0);
}

bool IsNegativeResponseForType(const MdnsRecord& record, DnsType dns_type) {
  if (record.dns_type() != DnsType::kNSEC) {
    return false;
  }

  const auto& nsec_types = absl::get<NsecRecordRdata>(record.rdata()).types();
  return ContainsIf(nsec_types, [dns_type](DnsType type) {
    return type == dns_type || type == DnsType::kANY;
  });
}

// RFC 6762 Section 10.1
// https://tools.ietf.org/html/rfc6762#section-10.1
// In case of a goodbye record, the querier should set TTL to 1 second
constexpr std::chrono::seconds kGoodbyeRecordTtl{1};

}  // namespace

MdnsTracker::MdnsTracker(MdnsSender* sender,
                         TaskRunner* task_runner,
                         ClockNowFunctionPtr now_function,
                         MdnsRandom* random_delay,
                         TrackerType tracker_type)
    : sender_(sender),
      task_runner_(task_runner),
      now_function_(now_function),
      send_alarm_(now_function, task_runner),
      random_delay_(random_delay),
      tracker_type_(tracker_type) {
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(now_function_);
  OSP_DCHECK(random_delay_);
  OSP_DCHECK(sender_);
}

MdnsTracker::~MdnsTracker() {
  send_alarm_.Cancel();

  for (const MdnsTracker* node : adjacent_nodes_) {
    node->RemovedReverseAdjacency(this);
  }
}

bool MdnsTracker::AddAdjacentNode(const MdnsTracker* node) const {
  OSP_DCHECK(node);
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  if (Contains(adjacent_nodes_, node)) {
    return false;
  }

  adjacent_nodes_.push_back(node);
  node->AddReverseAdjacency(this);
  return true;
}

bool MdnsTracker::RemoveAdjacentNode(const MdnsTracker* node) const {
  OSP_DCHECK(node);
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  auto it = std::find(adjacent_nodes_.begin(), adjacent_nodes_.end(), node);
  if (it == adjacent_nodes_.end()) {
    return false;
  }

  adjacent_nodes_.erase(it);
  node->RemovedReverseAdjacency(this);
  return true;
}

void MdnsTracker::AddReverseAdjacency(const MdnsTracker* node) const {
  OSP_DCHECK(!Contains(adjacent_nodes_, node));
  adjacent_nodes_.push_back(node);
}

void MdnsTracker::RemovedReverseAdjacency(const MdnsTracker* node) const {
  auto it = std::find(adjacent_nodes_.begin(), adjacent_nodes_.end(), node);
  OSP_DCHECK(it != adjacent_nodes_.end());

  adjacent_nodes_.erase(it);
}

MdnsRecordTracker::MdnsRecordTracker(
    MdnsRecord record,
    DnsType dns_type,
    MdnsSender* sender,
    TaskRunner* task_runner,
    ClockNowFunctionPtr now_function,
    MdnsRandom* random_delay,
    RecordExpiredCallback record_expired_callback)
    : MdnsTracker(sender,
                  task_runner,
                  now_function,
                  random_delay,
                  TrackerType::kRecordTracker),
      record_(std::move(record)),
      dns_type_(dns_type),
      start_time_(now_function_()),
      record_expired_callback_(std::move(record_expired_callback)) {
  OSP_DCHECK(record_expired_callback_);

  // RecordTrackers cannot be created for tracking NSEC types or ANY types.
  OSP_DCHECK(dns_type_ != DnsType::kNSEC);
  OSP_DCHECK(dns_type_ != DnsType::kANY);

  // Validate that, if the provided |record| is an NSEC record, then it provides
  // a negative response for |dns_type|.
  OSP_DCHECK(record_.dns_type() != DnsType::kNSEC ||
             IsNegativeResponseForType(record_, dns_type_));

  ScheduleFollowUpQuery();
}

MdnsRecordTracker::~MdnsRecordTracker() = default;

ErrorOr<MdnsRecordTracker::UpdateType> MdnsRecordTracker::Update(
    const MdnsRecord& new_record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  const bool has_same_rdata = record_.dns_type() == new_record.dns_type() &&
                              record_.rdata() == new_record.rdata();
  const bool new_is_negative_response = new_record.dns_type() == DnsType::kNSEC;
  const bool current_is_negative_response =
      record_.dns_type() == DnsType::kNSEC;

  if ((record_.dns_class() != new_record.dns_class()) ||
      (record_.name() != new_record.name())) {
    // The new record has been passed to a wrong tracker.
    return Error::Code::kParameterInvalid;
  }

  // New response record must correspond to the correct type.
  if ((!new_is_negative_response && new_record.dns_type() != dns_type_) ||
      (new_is_negative_response &&
       !IsNegativeResponseForType(new_record, dns_type_))) {
    // The new record has been passed to a wrong tracker.
    return Error::Code::kParameterInvalid;
  }

  // Goodbye records must have the same RDATA but TTL of 0.
  // RFC 6762 Section 10.1.
  // https://tools.ietf.org/html/rfc6762#section-10.1
  if (!new_is_negative_response && !current_is_negative_response &&
      IsGoodbyeRecord(new_record) && !has_same_rdata) {
    // The new record has been passed to a wrong tracker.
    return Error::Code::kParameterInvalid;
  }

  UpdateType result = UpdateType::kGoodbye;
  if (IsGoodbyeRecord(new_record)) {
    record_ = MdnsRecord(new_record.name(), new_record.dns_type(),
                         new_record.dns_class(), new_record.record_type(),
                         kGoodbyeRecordTtl, new_record.rdata());

    // Goodbye records do not need to be re-queried, set the attempt count to
    // the last item, which is 100% of TTL, i.e. record expiration.
    attempt_count_ = countof(kTtlFractions) - 1;
  } else {
    record_ = new_record;
    attempt_count_ = 0;
    result = has_same_rdata ? UpdateType::kTTLOnly : UpdateType::kRdata;
  }

  start_time_ = now_function_();
  ScheduleFollowUpQuery();

  return result;
}

bool MdnsRecordTracker::AddAssociatedQuery(
    const MdnsQuestionTracker* question_tracker) const {
  return AddAdjacentNode(question_tracker);
}

bool MdnsRecordTracker::RemoveAssociatedQuery(
    const MdnsQuestionTracker* question_tracker) const {
  return RemoveAdjacentNode(question_tracker);
}

void MdnsRecordTracker::ExpireSoon() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  record_ =
      MdnsRecord(record_.name(), record_.dns_type(), record_.dns_class(),
                 record_.record_type(), kGoodbyeRecordTtl, record_.rdata());

  // Set the attempt count to the last item, which is 100% of TTL, i.e. record
  // expiration, to prevent any re-queries
  attempt_count_ = countof(kTtlFractions) - 1;
  start_time_ = now_function_();
  ScheduleFollowUpQuery();
}

void MdnsRecordTracker::ExpireNow() {
  record_expired_callback_(this, record_);
}

bool MdnsRecordTracker::IsNearingExpiry() const {
  return (now_function_() - start_time_) > record_.ttl() / 2;
}

bool MdnsRecordTracker::SendQuery() const {
  const Clock::time_point expiration_time = start_time_ + record_.ttl();
  bool is_expired = (now_function_() >= expiration_time);
  if (!is_expired) {
    for (const MdnsTracker* tracker : adjacent_nodes()) {
      tracker->SendQuery();
    }
  } else {
    record_expired_callback_(this, record_);
  }

  return !is_expired;
}

void MdnsRecordTracker::ScheduleFollowUpQuery() {
  send_alarm_.Schedule(
      [this] {
        if (SendQuery()) {
          ScheduleFollowUpQuery();
        }
      },
      GetNextSendTime());
}

std::vector<MdnsRecord> MdnsRecordTracker::GetRecords() const {
  return {record_};
}

Clock::time_point MdnsRecordTracker::GetNextSendTime() {
  OSP_DCHECK(attempt_count_ < countof(kTtlFractions));

  double ttl_fraction = kTtlFractions[attempt_count_++];

  // Do not add random variation to the expiration time (last fraction of TTL)
  if (attempt_count_ != countof(kTtlFractions)) {
    ttl_fraction += random_delay_->GetRecordTtlVariation();
  }

  const Clock::duration delay =
      Clock::to_duration(record_.ttl() * ttl_fraction);
  return start_time_ + delay;
}

MdnsQuestionTracker::MdnsQuestionTracker(MdnsQuestion question,
                                         MdnsSender* sender,
                                         TaskRunner* task_runner,
                                         ClockNowFunctionPtr now_function,
                                         MdnsRandom* random_delay,
                                         const Config& config,
                                         QueryType query_type)
    : MdnsTracker(sender,
                  task_runner,
                  now_function,
                  random_delay,
                  TrackerType::kQuestionTracker),
      question_(std::move(question)),
      send_delay_(kMinimumQueryInterval),
      query_type_(query_type),
      maximum_announcement_count_(config.new_query_announcement_count < 0
                                      ? INT_MAX
                                      : config.new_query_announcement_count) {
  // Initialize the last send time to time_point::min() so that the next call to
  // SendQuery() is guaranteed to query the network.
  last_send_time_ = TrivialClockTraits::time_point::min();

  // The initial query has to be sent after a random delay of 20-120
  // milliseconds.
  if (announcements_so_far_ < maximum_announcement_count_) {
    announcements_so_far_++;

    if (query_type_ == QueryType::kOneShot) {
      task_runner_->PostTask([this] { MdnsQuestionTracker::SendQuery(); });
    } else {
      OSP_DCHECK(query_type_ == QueryType::kContinuous);
      send_alarm_.ScheduleFromNow(
          [this]() {
            MdnsQuestionTracker::SendQuery();
            ScheduleFollowUpQuery();
          },
          random_delay_->GetInitialQueryDelay());
    }
  }
}

MdnsQuestionTracker::~MdnsQuestionTracker() = default;

bool MdnsQuestionTracker::AddAssociatedRecord(
    const MdnsRecordTracker* record_tracker) const {
  return AddAdjacentNode(record_tracker);
}

bool MdnsQuestionTracker::RemoveAssociatedRecord(
    const MdnsRecordTracker* record_tracker) const {
  return RemoveAdjacentNode(record_tracker);
}

std::vector<MdnsRecord> MdnsQuestionTracker::GetRecords() const {
  std::vector<MdnsRecord> records;
  for (const MdnsTracker* tracker : adjacent_nodes()) {
    OSP_DCHECK(tracker->tracker_type() == TrackerType::kRecordTracker);

    // This call cannot result in an infinite loop because MdnsRecordTracker
    // instances only return a single record from this call.
    std::vector<MdnsRecord> node_records = tracker->GetRecords();
    OSP_DCHECK(node_records.size() == 1);

    records.push_back(std::move(node_records[0]));
  }

  return records;
}

bool MdnsQuestionTracker::SendQuery() const {
  // NOTE: The RFC does not specify the minimum interval between queries for
  // multiple records of the same query when initiated for different reasons
  // (such as for different record refreshes or for one record refresh and the
  // periodic re-querying for a continuous query). For this reason, a constant
  // outside of scope of the RFC has been chosen.
  TrivialClockTraits::time_point now = now_function_();
  if (now < last_send_time_ + kMinimumQueryInterval) {
    return true;
  }
  last_send_time_ = now;

  MdnsMessage message(CreateMessageId(), MessageType::Query);
  message.AddQuestion(question_);

  // Send the message and additional known answer packets as needed.
  for (auto it = adjacent_nodes().begin(); it != adjacent_nodes().end();) {
    OSP_DCHECK((*it)->tracker_type() == TrackerType::kRecordTracker);

    const MdnsRecordTracker* record_tracker =
        static_cast<const MdnsRecordTracker*>(*it);
    if (record_tracker->IsNearingExpiry()) {
      it++;
      continue;
    }

    // A record tracker should only contain one record.
    std::vector<MdnsRecord> node_records = (*it)->GetRecords();
    OSP_DCHECK(node_records.size() == 1);
    MdnsRecord node_record = std::move(node_records[0]);

    if (message.CanAddRecord(node_record)) {
      message.AddAnswer(std::move(node_record));
      it++;
    } else if (message.questions().empty() && message.answers().empty()) {
      // This case should never happen, because it means a record is too large
      // to fit into its own message.
      OSP_LOG_INFO
          << "Encountered unreasonably large message in cache. Skipping "
          << "known answer in suppressions...";
      it++;
    } else {
      message.set_truncated();
      sender_->SendMulticast(message);
      message = MdnsMessage(CreateMessageId(), MessageType::Query);
    }
  }
  sender_->SendMulticast(message);
  return true;
}

void MdnsQuestionTracker::ScheduleFollowUpQuery() {
  if (announcements_so_far_ >= maximum_announcement_count_) {
    return;
  }
  announcements_so_far_++;

  send_alarm_.ScheduleFromNow(
      [this] {
        if (SendQuery()) {
          ScheduleFollowUpQuery();
        }
      },
      send_delay_);
  send_delay_ = send_delay_ * kIntervalIncreaseFactor;
  if (send_delay_ > kMaximumQueryInterval) {
    send_delay_ = kMaximumQueryInterval;
  }
}

}  // namespace discovery
}  // namespace openscreen
