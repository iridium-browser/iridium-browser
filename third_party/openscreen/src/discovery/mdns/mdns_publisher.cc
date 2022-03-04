// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_publisher.h"

#include <chrono>
#include <cmath>

#include "discovery/common/config.h"
#include "discovery/mdns/mdns_probe_manager.h"
#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/mdns_sender.h"
#include "platform/api/task_runner.h"
#include "platform/base/trivial_clock_traits.h"

namespace openscreen {
namespace discovery {
namespace {

// Minimum delay between announcements of a given record in seconds.
constexpr std::chrono::seconds kMinAnnounceDelay{1};

// Intervals between successive announcements must increase by at least a
// factor of 2.
constexpr int kIntervalIncreaseFactor = 2;

// TTL for a goodbye record in seconds. This constant is called out in RFC 6762
// section 10.1.
constexpr std::chrono::seconds kGoodbyeTtl{0};

// Timespan between sending batches of announcement and goodbye records, in
// microseconds.
constexpr Clock::duration kDelayBetweenBatchedRecords =
    std::chrono::milliseconds(20);

inline MdnsRecord CreateGoodbyeRecord(const MdnsRecord& record) {
  if (record.ttl() == kGoodbyeTtl) {
    return record;
  }
  return MdnsRecord(record.name(), record.dns_type(), record.dns_class(),
                    record.record_type(), kGoodbyeTtl, record.rdata());
}

}  // namespace

MdnsPublisher::MdnsPublisher(MdnsSender* sender,
                             MdnsProbeManager* ownership_manager,
                             TaskRunner* task_runner,
                             ClockNowFunctionPtr now_function,
                             const Config& config)
    : sender_(sender),
      ownership_manager_(ownership_manager),
      task_runner_(task_runner),
      now_function_(now_function),
      max_announcement_attempts_(config.new_record_announcement_count) {
  OSP_DCHECK(ownership_manager_);
  OSP_DCHECK(sender_);
  OSP_DCHECK(task_runner_);
  OSP_DCHECK_GE(max_announcement_attempts_, 0);
}

MdnsPublisher::~MdnsPublisher() {
  if (batch_records_alarm_.has_value()) {
    batch_records_alarm_.value().Cancel();
    ProcessRecordQueue();
  }
}

Error MdnsPublisher::RegisterRecord(const MdnsRecord& record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(record.dns_class() != DnsClass::kANY);

  if (!CanBePublished(record.dns_type())) {
    return Error::Code::kParameterInvalid;
  }

  if (!IsRecordNameClaimed(record)) {
    return Error::Code::kParameterInvalid;
  }

  const DomainName& name = record.name();
  auto it = records_.emplace(name, std::vector<RecordAnnouncerPtr>{}).first;
  for (const RecordAnnouncerPtr& publisher : it->second) {
    if (publisher->record() == record) {
      return Error::Code::kItemAlreadyExists;
    }
  }

  OSP_DVLOG << "Registering record of type '" << record.dns_type() << "'";

  it->second.push_back(CreateAnnouncer(record));

  return Error::None();
}

Error MdnsPublisher::UnregisterRecord(const MdnsRecord& record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(record.dns_class() != DnsClass::kANY);

  if (!CanBePublished(record.dns_type())) {
    return Error::Code::kParameterInvalid;
  }

  OSP_DVLOG << "Unregistering record of type '" << record.dns_type() << "'";

  return RemoveRecord(record, true);
}

Error MdnsPublisher::UpdateRegisteredRecord(const MdnsRecord& old_record,
                                            const MdnsRecord& new_record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  if (!CanBePublished(new_record.dns_type())) {
    return Error::Code::kParameterInvalid;
  }

  if (old_record.dns_type() == DnsType::kPTR) {
    return Error::Code::kParameterInvalid;
  }

  // Check that the old record and new record are compatible.
  if (old_record.name() != new_record.name() ||
      old_record.dns_type() != new_record.dns_type() ||
      old_record.dns_class() != new_record.dns_class() ||
      old_record.record_type() != new_record.record_type()) {
    return Error::Code::kParameterInvalid;
  }

  OSP_DVLOG << "Updating record of type '" << new_record.dns_type() << "'";

  // Remove the old record. Per RFC 6762 section 8.4, a goodbye message will not
  // be sent, as all records which can be removed here are unique records, which
  // will be overwritten during the announcement phase when the updated record
  // is re-registered due to the cache-flush-bit's presence.
  const Error remove_result = RemoveRecord(old_record, false);
  if (!remove_result.ok()) {
    return remove_result;
  }

  // Register the new record.
  return RegisterRecord(new_record);
}

size_t MdnsPublisher::GetRecordCount() const {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  size_t count = 0;
  for (const auto& pair : records_) {
    count += pair.second.size();
  }

  return count;
}

bool MdnsPublisher::HasRecords(const DomainName& name,
                               DnsType type,
                               DnsClass clazz) {
  return !GetRecords(name, type, clazz).empty();
}

std::vector<MdnsRecord::ConstRef> MdnsPublisher::GetRecords(
    const DomainName& name,
    DnsType type,
    DnsClass clazz) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  std::vector<MdnsRecord::ConstRef> records;
  auto it = records_.find(name);
  if (it != records_.end()) {
    for (const RecordAnnouncerPtr& announcer : it->second) {
      OSP_DCHECK(announcer.get());
      const DnsType record_dns_type = announcer->record().dns_type();
      const DnsClass record_dns_class = announcer->record().dns_class();
      if ((type == DnsType::kANY || type == record_dns_type) &&
          (clazz == DnsClass::kANY || clazz == record_dns_class)) {
        records.push_back(announcer->record());
      }
    }
  }

  return records;
}

std::vector<MdnsRecord::ConstRef> MdnsPublisher::GetPtrRecords(DnsClass clazz) {
  std::vector<MdnsRecord::ConstRef> records;

  // There should be few records associated with any given domain name, so it is
  // simpler and less error prone to iterate across all records than to check
  // the domain name against format '[^.]+\.(_tcp)|(_udp)\..*''
  for (auto it = records_.begin(); it != records_.end(); it++) {
    for (const RecordAnnouncerPtr& announcer : it->second) {
      OSP_DCHECK(announcer.get());
      const DnsType record_dns_type = announcer->record().dns_type();
      if (record_dns_type != DnsType::kPTR) {
        continue;
      }

      const DnsClass record_dns_class = announcer->record().dns_class();
      if ((clazz == DnsClass::kANY || clazz == record_dns_class)) {
        records.push_back(announcer->record());
      }
    }
  }

  return records;
}

Error MdnsPublisher::RemoveRecord(const MdnsRecord& record,
                                  bool should_announce_deletion) {
  const DomainName& name = record.name();

  // Check for the domain and fail if it's not found.
  const auto it = records_.find(name);
  if (it == records_.end()) {
    return Error::Code::kItemNotFound;
  }

  // Check for the record to be removed.
  const auto records_it =
      std::find_if(it->second.begin(), it->second.end(),
                   [&record](const RecordAnnouncerPtr& publisher) {
                     return publisher->record() == record;
                   });
  if (records_it == it->second.end()) {
    return Error::Code::kItemNotFound;
  }
  if (!should_announce_deletion) {
    (*records_it)->DisableGoodbyeMessageTransmission();
  }

  it->second.erase(records_it);
  if (it->second.empty()) {
    records_.erase(it);
  }

  return Error::None();
}

bool MdnsPublisher::IsRecordNameClaimed(const MdnsRecord& record) const {
  const DomainName& name =
      record.dns_type() == DnsType::kPTR
          ? absl::get<PtrRecordRdata>(record.rdata()).ptr_domain()
          : record.name();
  return ownership_manager_->IsDomainClaimed(name);
}

MdnsPublisher::RecordAnnouncer::RecordAnnouncer(
    MdnsRecord record,
    MdnsPublisher* publisher,
    TaskRunner* task_runner,
    ClockNowFunctionPtr now_function,
    int target_announcement_attempts)
    : publisher_(publisher),
      task_runner_(task_runner),
      now_function_(now_function),
      record_(std::move(record)),
      alarm_(now_function_, task_runner_),
      target_announcement_attempts_(target_announcement_attempts) {
  OSP_DCHECK(publisher_);
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(record_.ttl() != Clock::duration::zero());

  QueueAnnouncement();
}

MdnsPublisher::RecordAnnouncer::~RecordAnnouncer() {
  alarm_.Cancel();
  if (should_send_goodbye_message_) {
    QueueGoodbye();
  }
}

void MdnsPublisher::RecordAnnouncer::QueueGoodbye() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  publisher_->QueueRecord(CreateGoodbyeRecord(record_));
}

void MdnsPublisher::RecordAnnouncer::QueueAnnouncement() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  if (attempts_ >= target_announcement_attempts_) {
    return;
  }

  publisher_->QueueRecord(record_);

  const Clock::duration new_delay = GetNextAnnounceDelay();
  attempts_++;
  alarm_.ScheduleFromNow([this]() { QueueAnnouncement(); }, new_delay);
}

void MdnsPublisher::QueueRecord(MdnsRecord record) {
  if (!batch_records_alarm_.has_value()) {
    OSP_DCHECK(records_to_send_.empty());
    batch_records_alarm_.emplace(now_function_, task_runner_);
    batch_records_alarm_.value().ScheduleFromNow(
        [this]() { ProcessRecordQueue(); }, kDelayBetweenBatchedRecords);
  }

  // Check that we aren't announcing and goodbye'ing a record in the same batch.
  // We expect to be sending no more than 5 records at a time, so don't worry
  // about iterating across this vector for each insert.
  auto goodbye = CreateGoodbyeRecord(record);
  auto existing_record_it =
      std::find_if(records_to_send_.begin(), records_to_send_.end(),
                   [&goodbye](const MdnsRecord& r) {
                     return goodbye == CreateGoodbyeRecord(r);
                   });

  // If we didn't find it, simply add it to the queue. Else, only send the
  // goodbye record.
  if (existing_record_it == records_to_send_.end()) {
    records_to_send_.push_back(std::move(record));
  } else if (*existing_record_it == goodbye) {
    // This means that the goodbye record is already queued to be sent. This
    // means that there is no reason to also announce it, so exit early.
    return;
  } else if (record == goodbye) {
    // This means that we are sending a goodbye record right as it would also
    // be announced. Skip the announcement since the record is being
    // unregistered.
    *existing_record_it = std::move(record);
  } else if (record == *existing_record_it) {
    // This case shouldn't happen, but there is no work to do if it does. Log
    // to surface that something weird is going on.
    OSP_LOG_INFO << "Same record being announced multiple times.";
  } else {
    // This case should never occur. Support it just in case, but log to
    // surface that something weird is happening.
    OSP_LOG_INFO << "Updating the same record multiple times with multiple "
                    "TTL values.";
  }
}

void MdnsPublisher::ProcessRecordQueue() {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  if (records_to_send_.empty()) {
    return;
  }

  MdnsMessage message(CreateMessageId(), MessageType::Response);
  for (auto it = records_to_send_.begin(); it != records_to_send_.end();) {
    if (message.CanAddRecord(*it)) {
      message.AddAnswer(std::move(*it++));
    } else if (message.answers().empty()) {
      // This case should never happen, because it means a record is too large
      // to fit into its own message.
      OSP_LOG_INFO
          << "Encountered unreasonably large message in cache. Skipping "
          << "known answer in suppressions...";
      it++;
    } else {
      sender_->SendMulticast(message);
      message = MdnsMessage(CreateMessageId(), MessageType::Response);
    }
  }

  if (!message.answers().empty()) {
    sender_->SendMulticast(message);
  }

  batch_records_alarm_ = absl::nullopt;
  records_to_send_.clear();
}

Clock::duration MdnsPublisher::RecordAnnouncer::GetNextAnnounceDelay() {
  return Clock::to_duration(kMinAnnounceDelay *
                            pow(kIntervalIncreaseFactor, attempts_));
}

}  // namespace discovery
}  // namespace openscreen
