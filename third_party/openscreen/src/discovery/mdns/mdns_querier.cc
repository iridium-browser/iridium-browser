// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "discovery/mdns/mdns_querier.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "discovery/common/config.h"
#include "discovery/common/reporting_client.h"
#include "discovery/mdns/mdns_random.h"
#include "discovery/mdns/mdns_receiver.h"
#include "discovery/mdns/mdns_sender.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "util/std_util.h"

namespace openscreen {
namespace discovery {
namespace {

constexpr std::array<DnsType, 5> kTranslatedNsecAnyQueryTypes = {
    DnsType::kA, DnsType::kPTR, DnsType::kTXT, DnsType::kAAAA, DnsType::kSRV};

bool IsNegativeResponseFor(const MdnsRecord& record, DnsType type) {
  if (record.dns_type() != DnsType::kNSEC) {
    return false;
  }

  const NsecRecordRdata& nsec = absl::get<NsecRecordRdata>(record.rdata());

  // RFC 6762 section 6.1, the NSEC bit must NOT be set in the received NSEC
  // record to indicate this is an mDNS NSEC record rather than a traditional
  // DNS NSEC record.
  if (Contains(nsec.types(), DnsType::kNSEC)) {
    return false;
  }

  return ContainsIf(nsec.types(), [type](DnsType stored_type) {
    return stored_type == type || stored_type == DnsType::kANY;
  });
}

struct HashDnsType {
  inline size_t operator()(DnsType type) const {
    return static_cast<size_t>(type);
  }
};

// Helper used for sorting MDNS records. This function guarantees the following:
// - All MdnsRecords with the same name appear adjacent to each-other.
// - An NSEC record with a given name appears before all other records with the
//   same name.
bool CompareRecordByNameAndType(const MdnsRecord& first,
                                const MdnsRecord& second) {
  if (first.name() != second.name()) {
    return first.name() < second.name();
  }

  if ((first.dns_type() == DnsType::kNSEC) !=
      (second.dns_type() == DnsType::kNSEC)) {
    return first.dns_type() == DnsType::kNSEC;
  }

  return first < second;
}

class DnsTypeBitSet {
 public:
  // Returns whether any types are currently stored in this data structure.
  bool IsEmpty() { return !elements_.any(); }

  // Attempts to insert the given type into this data structure. Returns
  // true iff the type was not already present.
  bool Insert(DnsType type) {
    uint16_t bit = (type == DnsType::kANY) ? 0 : static_cast<uint16_t>(type);
    bool was_set = elements_.test(bit);
    elements_.set(bit);
    return !was_set;
  }

  // Iterates over all members of the provided container, inserting each
  // DnsType contained within to this instance. Returns true iff any element
  // inserted was not already present in this instance.
  template <typename Container>
  bool Insert(const Container& container) {
    bool has_element_been_inserted = false;
    for (DnsType type : container) {
      has_element_been_inserted |= Insert(type);
    }
    return has_element_been_inserted;
  }

  // Attempts to remove the given type from this data structure. Returns true
  // iff the type was present prior to this call.
  bool Remove(DnsType type) {
    if (IsEmpty()) {
      return false;
    } else if (type == DnsType::kANY) {
      elements_.reset();
      return true;
    }

    uint16_t bit = static_cast<uint16_t>(type);
    bool was_set = elements_.test(bit);
    elements_.reset(bit);
    return was_set;
  }

  // Returns the DnsTypes currently stored in this data structure.
  std::vector<DnsType> GetTypes() const {
    if (elements_.test(0)) {
      return {DnsType::kANY};
    }

    std::vector<DnsType> types;
    for (DnsType type : kSupportedDnsTypes) {
      if (type == DnsType::kANY) {
        continue;
      }

      uint16_t cast_int = static_cast<uint16_t>(type);
      if (elements_.test(cast_int)) {
        types.push_back(type);
      }
    }
    return types;
  }

 private:
  std::bitset<64> elements_;
};

// Modifies |records| such that no NSEC record signifies the nonexistance of a
// record which is also present in the same message. Order of the input vector
// is NOT preserved.
// NOTE: |records| is not of type MdnsRecord::ConstRef because the members must
// be modified.
// TODO(b/170353378): Break this logic into a separate processing module between
// the MdnsReader and the MdnsQuerier.
void RemoveInvalidNsecFlags(std::vector<MdnsRecord>* records) {
  // Sort the records so NSEC records are first so that only one iteration
  // through all records is needed.
  std::sort(records->begin(), records->end(), CompareRecordByNameAndType);

  // The set of NSEC records that need to be removed from |records|. This can't
  // be done as part of the below loop because it would invalidate the iterator
  // that's still being used.
  std::vector<std::vector<MdnsRecord>::iterator> nsecs_to_delete;

  // Process all elements.
  for (auto it = records->begin(); it != records->end();) {
    if (it->dns_type() != DnsType::kNSEC) {
      it++;
      continue;
    }

    // Track whether the current NSEC record in the input vector has been
    // modified by some step of this algorithm, be that merging with another
    // record, removing a DnsType, or any other modification.
    bool has_changed = false;

    // The types for the new record to create, if |has_changed|.
    const NsecRecordRdata& nsec_rdata = absl::get<NsecRecordRdata>(it->rdata());
    DnsTypeBitSet types;
    for (DnsType type : nsec_rdata.types()) {
      types.Insert(type);
    }
    auto nsec = it;
    it++;

    // Combine multiple NSECs to simplify the following code. This probably
    // won't happen, but the RFC doesn't exclude the possibility, so account for
    // it. Define the TTL of this new NSEC record created by this merge process
    // to be the minimum of all merged NSEC records.
    std::chrono::seconds new_ttl = nsec->ttl();
    while (it != records->end() && it->name() == nsec->name() &&
           it->dns_type() == DnsType::kNSEC) {
      has_changed |=
          types.Insert(absl::get<NsecRecordRdata>(it->rdata()).types());
      new_ttl = std::min(new_ttl, it->ttl());
      it = records->erase(it);
    }

    // Remove any types associated with a known record type.
    for (; it != records->end() && it->name() == nsec->name(); it++) {
      OSP_DCHECK(it->dns_type() != DnsType::kNSEC);
      has_changed |= types.Remove(it->dns_type());
    }

    // Modify the stored NSEC record, if needed.
    if (has_changed && types.IsEmpty()) {
      nsecs_to_delete.push_back(nsec);
    } else if (has_changed) {
      NsecRecordRdata new_rdata(nsec_rdata.next_domain_name(),
                                types.GetTypes());
      *nsec = MdnsRecord(nsec->name(), nsec->dns_type(), nsec->dns_class(),
                         nsec->record_type(), new_ttl, std::move(new_rdata));
    }
  }

  // Erase invalid NSEC records. Go backwards to avoid invalidating the
  // remaining iterators.
  for (auto erase_it = nsecs_to_delete.rbegin();
       erase_it != nsecs_to_delete.rend(); erase_it++) {
    records->erase(*erase_it);
  }
}

}  // namespace

MdnsQuerier::RecordTrackerLruCache::RecordTrackerLruCache(
    MdnsQuerier* querier,
    MdnsSender* sender,
    MdnsRandom* random_delay,
    TaskRunner* task_runner,
    ClockNowFunctionPtr now_function,
    ReportingClient* reporting_client,
    const Config& config)
    : querier_(querier),
      sender_(sender),
      random_delay_(random_delay),
      task_runner_(task_runner),
      now_function_(now_function),
      reporting_client_(reporting_client),
      config_(config) {
  OSP_DCHECK(sender_);
  OSP_DCHECK(random_delay_);
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(reporting_client_);
  OSP_DCHECK_GT(config_.querier_max_records_cached, 0);
}

std::vector<std::reference_wrapper<const MdnsRecordTracker>>
MdnsQuerier::RecordTrackerLruCache::Find(const DomainName& name) {
  return Find(name, DnsType::kANY, DnsClass::kANY);
}

std::vector<std::reference_wrapper<const MdnsRecordTracker>>
MdnsQuerier::RecordTrackerLruCache::Find(const DomainName& name,
                                         DnsType dns_type,
                                         DnsClass dns_class) {
  std::vector<RecordTrackerConstRef> results;
  auto pair = records_.equal_range(name);
  for (auto it = pair.first; it != pair.second; it++) {
    const MdnsRecordTracker& tracker = *it->second;
    if ((dns_type == DnsType::kANY || dns_type == tracker.dns_type()) &&
        (dns_class == DnsClass::kANY || dns_class == tracker.dns_class())) {
      results.push_back(std::cref(tracker));
    }
  }

  return results;
}

int MdnsQuerier::RecordTrackerLruCache::Erase(const DomainName& domain,
                                              TrackerApplicableCheck check) {
  auto pair = records_.equal_range(domain);
  int count = 0;
  for (RecordMap::iterator it = pair.first; it != pair.second;) {
    if (check(*it->second)) {
      lru_order_.erase(it->second);
      it = records_.erase(it);
      count++;
    } else {
      it++;
    }
  }

  return count;
}

int MdnsQuerier::RecordTrackerLruCache::ExpireSoon(
    const DomainName& domain,
    TrackerApplicableCheck check) {
  auto pair = records_.equal_range(domain);
  int count = 0;
  for (RecordMap::iterator it = pair.first; it != pair.second; it++) {
    if (check(*it->second)) {
      MoveToEnd(it);
      it->second->ExpireSoon();
      count++;
    }
  }

  return count;
}

int MdnsQuerier::RecordTrackerLruCache::Update(const MdnsRecord& record,
                                               TrackerApplicableCheck check) {
  return Update(record, check, [](const MdnsRecordTracker& t) {});
}

int MdnsQuerier::RecordTrackerLruCache::Update(
    const MdnsRecord& record,
    TrackerApplicableCheck check,
    TrackerChangeCallback on_rdata_update) {
  auto pair = records_.equal_range(record.name());
  int count = 0;
  for (RecordMap::iterator it = pair.first; it != pair.second; it++) {
    if (check(*it->second)) {
      auto result = it->second->Update(record);

      if (result.is_error()) {
        reporting_client_->OnRecoverableError(
            Error(Error::Code::kUpdateReceivedRecordFailure,
                  result.error().ToString()));
        continue;
      }

      count++;
      if (result.value() == MdnsRecordTracker::UpdateType::kGoodbye) {
        it->second->ExpireSoon();
        MoveToEnd(it);
      } else {
        MoveToBeginning(it);
        if (result.value() == MdnsRecordTracker::UpdateType::kRdata) {
          on_rdata_update(*it->second);
        }
      }
    }
  }

  return count;
}

const MdnsRecordTracker& MdnsQuerier::RecordTrackerLruCache::StartTracking(
    MdnsRecord record,
    DnsType dns_type) {
  auto expiration_callback = [this](const MdnsRecordTracker* tracker,
                                    const MdnsRecord& r) {
    querier_->OnRecordExpired(tracker, r);
  };

  while (lru_order_.size() >=
         static_cast<size_t>(config_.querier_max_records_cached)) {
    // This call erases one of the tracked records.
    OSP_DVLOG << "Maximum cacheable record count exceeded ("
              << config_.querier_max_records_cached << ")";
    lru_order_.back().ExpireNow();
  }

  auto name = record.name();
  lru_order_.emplace_front(std::move(record), dns_type, sender_, task_runner_,
                           now_function_, random_delay_,
                           std::move(expiration_callback));
  records_.emplace(std::move(name), lru_order_.begin());

  return lru_order_.front();
}

void MdnsQuerier::RecordTrackerLruCache::MoveToBeginning(
    MdnsQuerier::RecordTrackerLruCache::RecordMap::iterator it) {
  lru_order_.splice(lru_order_.begin(), lru_order_, it->second);
  it->second = lru_order_.begin();
}

void MdnsQuerier::RecordTrackerLruCache::MoveToEnd(
    MdnsQuerier::RecordTrackerLruCache::RecordMap::iterator it) {
  lru_order_.splice(lru_order_.end(), lru_order_, it->second);
  it->second = --lru_order_.end();
}

MdnsQuerier::MdnsQuerier(MdnsSender* sender,
                         MdnsReceiver* receiver,
                         TaskRunner* task_runner,
                         ClockNowFunctionPtr now_function,
                         MdnsRandom* random_delay,
                         ReportingClient* reporting_client,
                         Config config)
    : sender_(sender),
      receiver_(receiver),
      task_runner_(task_runner),
      now_function_(now_function),
      random_delay_(random_delay),
      reporting_client_(reporting_client),
      config_(std::move(config)),
      records_(this,
               sender_,
               random_delay_,
               task_runner_,
               now_function_,
               reporting_client_,
               config_) {
  OSP_DCHECK(sender_);
  OSP_DCHECK(receiver_);
  OSP_DCHECK(task_runner_);
  OSP_DCHECK(now_function_);
  OSP_DCHECK(random_delay_);
  OSP_DCHECK(reporting_client_);

  receiver_->AddResponseCallback(this);
}

MdnsQuerier::~MdnsQuerier() {
  receiver_->RemoveResponseCallback(this);
}

// NOTE: The code below is range loops instead of std:find_if, for better
// readability, brevity and homogeneity.  Using std::find_if results in a few
// more lines of code, readability suffers from extra lambdas.

void MdnsQuerier::StartQuery(const DomainName& name,
                             DnsType dns_type,
                             DnsClass dns_class,
                             MdnsRecordChangedCallback* callback) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(callback);
  OSP_DCHECK(CanBeQueried(dns_type));

  // Add a new callback if haven't seen it before
  auto callbacks_it = callbacks_.equal_range(name);
  for (auto entry = callbacks_it.first; entry != callbacks_it.second; ++entry) {
    const CallbackInfo& callback_info = entry->second;
    if (dns_type == callback_info.dns_type &&
        dns_class == callback_info.dns_class &&
        callback == callback_info.callback) {
      // Already have this callback
      return;
    }
  }
  callbacks_.emplace(name, CallbackInfo{callback, dns_type, dns_class});

  // Notify the new callback with previously cached records.
  // NOTE: In the future, could allow callers to fetch cached records after
  // adding a callback, for example to prime the UI.
  std::vector<PendingQueryChange> pending_changes;
  const std::vector<RecordTrackerLruCache::RecordTrackerConstRef> trackers =
      records_.Find(name, dns_type, dns_class);
  for (const MdnsRecordTracker& tracker : trackers) {
    if (!tracker.is_negative_response()) {
      MdnsRecord stored_record(name, tracker.dns_type(), tracker.dns_class(),
                               tracker.record_type(), tracker.ttl(),
                               tracker.rdata());
      std::vector<PendingQueryChange> new_changes = callback->OnRecordChanged(
          std::move(stored_record), RecordChangedEvent::kCreated);
      pending_changes.insert(pending_changes.end(), new_changes.begin(),
                             new_changes.end());
    }
  }

  // Add a new question if haven't seen it before
  auto questions_it = questions_.equal_range(name);
  const bool is_question_already_tracked =
      std::find_if(questions_it.first, questions_it.second,
                   [dns_type, dns_class](const auto& entry) {
                     const MdnsQuestion& tracked_question =
                         entry.second->question();
                     return dns_type == tracked_question.dns_type() &&
                            dns_class == tracked_question.dns_class();
                   }) != questions_it.second;
  if (!is_question_already_tracked) {
    AddQuestion(
        MdnsQuestion(name, dns_type, dns_class, ResponseType::kMulticast));
  }

  // Apply any pending changes from the OnRecordChanged() callbacks.
  ApplyPendingChanges(std::move(pending_changes));
}

void MdnsQuerier::StopQuery(const DomainName& name,
                            DnsType dns_type,
                            DnsClass dns_class,
                            MdnsRecordChangedCallback* callback) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(callback);

  if (!CanBeQueried(dns_type)) {
    return;
  }

  // Find and remove the callback.
  int callbacks_for_key = 0;
  auto callbacks_it = callbacks_.equal_range(name);
  for (auto entry = callbacks_it.first; entry != callbacks_it.second;) {
    const CallbackInfo& callback_info = entry->second;
    if (dns_type == callback_info.dns_type &&
        dns_class == callback_info.dns_class) {
      if (callback == callback_info.callback) {
        entry = callbacks_.erase(entry);
      } else {
        ++callbacks_for_key;
        ++entry;
      }
    }
  }

  // Exit if there are still callbacks registered for DomainName + DnsType +
  // DnsClass
  if (callbacks_for_key > 0) {
    return;
  }

  // Find and delete a question that does not have any associated callbacks
  auto questions_it = questions_.equal_range(name);
  for (auto entry = questions_it.first; entry != questions_it.second; ++entry) {
    const MdnsQuestion& tracked_question = entry->second->question();
    if (dns_type == tracked_question.dns_type() &&
        dns_class == tracked_question.dns_class()) {
      questions_.erase(entry);
      return;
    }
  }
}

void MdnsQuerier::ReinitializeQueries(const DomainName& name) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  // Get the ongoing queries and their callbacks.
  std::vector<CallbackInfo> callbacks;
  auto its = callbacks_.equal_range(name);
  for (auto it = its.first; it != its.second; it++) {
    callbacks.push_back(std::move(it->second));
  }
  callbacks_.erase(name);

  // Remove all known questions and answers.
  questions_.erase(name);
  records_.Erase(name, [](const MdnsRecordTracker& tracker) { return true; });

  // Restart the queries.
  for (const auto& cb : callbacks) {
    StartQuery(name, cb.dns_type, cb.dns_class, cb.callback);
  }
}

void MdnsQuerier::OnMessageReceived(const MdnsMessage& message) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(message.type() == MessageType::Response);

  OSP_DVLOG << "Received mDNS Response message with "
            << message.answers().size() << " answers and "
            << message.additional_records().size()
            << " additional records. Processing...";

  std::vector<MdnsRecord> records_to_process;

  // Add any records that are relevant for this querier.
  bool found_relevant_records = false;
  for (const MdnsRecord& record : message.answers()) {
    if (ShouldAnswerRecordBeProcessed(record)) {
      records_to_process.push_back(record);
      found_relevant_records = true;
    }
  }

  // If any of the message's answers are relevant, add all additional records.
  // Else, since the message has already been received and parsed, use any
  // individual records relevant to this querier to update the cache.
  for (const MdnsRecord& record : message.additional_records()) {
    if (found_relevant_records || ShouldAnswerRecordBeProcessed(record)) {
      records_to_process.push_back(record);
    }
  }

  // Drop NSEC records associated with a non-NSEC record of the same type.
  RemoveInvalidNsecFlags(&records_to_process);

  // Process all remaining records.
  for (const MdnsRecord& record_to_process : records_to_process) {
    ProcessRecord(record_to_process);
  }

  OSP_DVLOG << "\tmDNS Response processed (" << records_to_process.size()
            << " records accepted)!";

  // TODO(crbug.com/openscreen/83): Check authority records.
}

bool MdnsQuerier::ShouldAnswerRecordBeProcessed(const MdnsRecord& answer) {
  // First, accept the record if it's associated with an ongoing question.
  const auto questions_range = questions_.equal_range(answer.name());
  const auto it = std::find_if(
      questions_range.first, questions_range.second,
      [&answer](const auto& pair) {
        return (pair.second->question().dns_type() == DnsType::kANY ||
                IsNegativeResponseFor(answer,
                                      pair.second->question().dns_type()) ||
                pair.second->question().dns_type() == answer.dns_type()) &&
               (pair.second->question().dns_class() == DnsClass::kANY ||
                pair.second->question().dns_class() == answer.dns_class());
      });
  if (it != questions_range.second) {
    return true;
  }

  // If not, check if it corresponds to an already existing record. This is
  // required because records which are already stored may either have been
  // received in an additional records section, or are associated with a query
  // which is no longer active.
  std::vector<DnsType> types{answer.dns_type()};
  if (answer.dns_type() == DnsType::kNSEC) {
    const auto& nsec_rdata = absl::get<NsecRecordRdata>(answer.rdata());
    types = nsec_rdata.types();
  }

  for (DnsType type : types) {
    std::vector<RecordTrackerLruCache::RecordTrackerConstRef> trackers =
        records_.Find(answer.name(), type, answer.dns_class());
    if (!trackers.empty()) {
      return true;
    }
  }

  // In all other cases, the record isn't relevant. Drop it.
  return false;
}

void MdnsQuerier::OnRecordExpired(const MdnsRecordTracker* tracker,
                                  const MdnsRecord& record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  if (!tracker->is_negative_response()) {
    ProcessCallbacks(record, RecordChangedEvent::kExpired);
  }

  records_.Erase(record.name(), [tracker](const MdnsRecordTracker& it_tracker) {
    return tracker == &it_tracker;
  });
}

void MdnsQuerier::ProcessRecord(const MdnsRecord& record) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  // Skip all records that can't be processed.
  if (!CanBeProcessed(record.dns_type())) {
    return;
  }

  // Ignore NSEC records if the embedder has configured us to do so.
  if (config_.ignore_nsec_responses && record.dns_type() == DnsType::kNSEC) {
    return;
  }

  // Get the types which the received record is associated with. In most cases
  // this will only be the type of the provided record, but in the case of
  // NSEC records this will be all records which the record dictates the
  // nonexistence of.
  std::vector<DnsType> types;
  size_t types_count = 0;
  const DnsType* types_ptr = nullptr;
  if (record.dns_type() == DnsType::kNSEC) {
    const auto& nsec_rdata = absl::get<NsecRecordRdata>(record.rdata());
    if (Contains(nsec_rdata.types(), DnsType::kANY)) {
      types_ptr = kTranslatedNsecAnyQueryTypes.data();
      types_count = kTranslatedNsecAnyQueryTypes.size();
    } else {
      types_ptr = nsec_rdata.types().data();
      types_count = nsec_rdata.types().size();
    }
  } else {
    types.push_back(record.dns_type());
    types_ptr = types.data();
    types_count = types.size();
  }

  // Apply the update for each type that the record is associated with.
  for (size_t i = 0; i < types_count; ++i) {
    DnsType dns_type = types_ptr[i];
    switch (record.record_type()) {
      case RecordType::kShared: {
        ProcessSharedRecord(record, dns_type);
        break;
      }
      case RecordType::kUnique: {
        ProcessUniqueRecord(record, dns_type);
        break;
      }
    }
  }
}

void MdnsQuerier::ProcessSharedRecord(const MdnsRecord& record,
                                      DnsType dns_type) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(record.record_type() == RecordType::kShared);

  // By design, NSEC records are never shared records.
  if (record.dns_type() == DnsType::kNSEC) {
    return;
  }

  // For any records updated, this host already has this shared record. Since
  // the RDATA matches, this is only a TTL update.
  auto check = [&record](const MdnsRecordTracker& tracker) {
    return record.dns_type() == tracker.dns_type() &&
           record.dns_class() == tracker.dns_class() &&
           record.rdata() == tracker.rdata();
  };
  auto updated_count = records_.Update(record, std::move(check));

  if (!updated_count) {
    // Have never before seen this shared record, insert a new one.
    AddRecord(record, dns_type);
    ProcessCallbacks(record, RecordChangedEvent::kCreated);
  }
}

void MdnsQuerier::ProcessUniqueRecord(const MdnsRecord& record,
                                      DnsType dns_type) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());
  OSP_DCHECK(record.record_type() == RecordType::kUnique);

  std::vector<RecordTrackerLruCache::RecordTrackerConstRef> trackers =
      records_.Find(record.name(), dns_type, record.dns_class());
  size_t num_records_for_key = trackers.size();

  // Have not seen any records with this key before. This case is expected the
  // first time a record is received.
  if (num_records_for_key == size_t{0}) {
    const bool will_exist = record.dns_type() != DnsType::kNSEC;
    AddRecord(record, dns_type);
    if (will_exist) {
      ProcessCallbacks(record, RecordChangedEvent::kCreated);
    }
  } else if (num_records_for_key == size_t{1}) {
    // There is exactly one tracker associated with this key. This is the
    // expected case when a record matching this one has already been seen.
    ProcessSinglyTrackedUniqueRecord(record, trackers[0]);
  } else {
    // Multiple records with the same key.
    ProcessMultiTrackedUniqueRecord(record, dns_type);
  }
}

void MdnsQuerier::ProcessSinglyTrackedUniqueRecord(
    const MdnsRecord& record,
    const MdnsRecordTracker& tracker) {
  const bool existed_previously = !tracker.is_negative_response();
  const bool will_exist = record.dns_type() != DnsType::kNSEC;

  // Calculate the callback to call on record update success while the old
  // record still exists.
  MdnsRecord record_for_callback = record;
  if (existed_previously && !will_exist) {
    record_for_callback =
        MdnsRecord(record.name(), tracker.dns_type(), tracker.dns_class(),
                   tracker.record_type(), tracker.ttl(), tracker.rdata());
  }

  auto on_rdata_change = [this, r = std::move(record_for_callback),
                          existed_previously,
                          will_exist](const MdnsRecordTracker& t) {
    // If RDATA on the record is different, notify that the record has
    // been updated.
    if (existed_previously && will_exist) {
      ProcessCallbacks(r, RecordChangedEvent::kUpdated);
    } else if (existed_previously) {
      // Do not expire the tracker, because it still holds an NSEC record.
      ProcessCallbacks(r, RecordChangedEvent::kExpired);
    } else if (will_exist) {
      ProcessCallbacks(r, RecordChangedEvent::kCreated);
    }
  };

  int updated_count = records_.Update(
      record, [&tracker](const MdnsRecordTracker& t) { return &tracker == &t; },
      std::move(on_rdata_change));
  OSP_DCHECK_EQ(updated_count, 1);
}

void MdnsQuerier::ProcessMultiTrackedUniqueRecord(const MdnsRecord& record,
                                                  DnsType dns_type) {
  auto update_check = [&record, dns_type](const MdnsRecordTracker& tracker) {
    return tracker.dns_type() == dns_type &&
           tracker.dns_class() == record.dns_class() &&
           tracker.rdata() == record.rdata();
  };
  int update_count = records_.Update(
      record, std::move(update_check),
      [](const MdnsRecordTracker& tracker) { OSP_NOTREACHED(); });
  OSP_DCHECK_LE(update_count, 1);

  auto expire_check = [&record, dns_type](const MdnsRecordTracker& tracker) {
    return tracker.dns_type() == dns_type &&
           tracker.dns_class() == record.dns_class() &&
           tracker.rdata() != record.rdata();
  };
  int expire_count =
      records_.ExpireSoon(record.name(), std::move(expire_check));
  OSP_DCHECK_GE(expire_count, 1);

  // Did not find an existing record to update.
  if (!update_count && !expire_count) {
    AddRecord(record, dns_type);
    if (record.dns_type() != DnsType::kNSEC) {
      ProcessCallbacks(record, RecordChangedEvent::kCreated);
    }
  }
}

void MdnsQuerier::ProcessCallbacks(const MdnsRecord& record,
                                   RecordChangedEvent event) {
  OSP_DCHECK(task_runner_->IsRunningOnTaskRunner());

  std::vector<PendingQueryChange> pending_changes;
  auto callbacks_it = callbacks_.equal_range(record.name());
  for (auto entry = callbacks_it.first; entry != callbacks_it.second; ++entry) {
    const CallbackInfo& callback_info = entry->second;
    if ((callback_info.dns_type == DnsType::kANY ||
         record.dns_type() == callback_info.dns_type) &&
        (callback_info.dns_class == DnsClass::kANY ||
         record.dns_class() == callback_info.dns_class)) {
      std::vector<PendingQueryChange> new_changes =
          callback_info.callback->OnRecordChanged(record, event);
      pending_changes.insert(pending_changes.end(), new_changes.begin(),
                             new_changes.end());
    }
  }

  ApplyPendingChanges(std::move(pending_changes));
}

void MdnsQuerier::AddQuestion(const MdnsQuestion& question) {
  auto question_tracker = std::make_unique<MdnsQuestionTracker>(
      question, sender_, task_runner_, now_function_, random_delay_, config_);
  MdnsQuestionTracker* ptr = question_tracker.get();
  questions_.emplace(question.name(), std::move(question_tracker));

  // Let all records associated with this question know that there is a new
  // query that can be used for their refresh.
  std::vector<RecordTrackerLruCache::RecordTrackerConstRef> trackers =
      records_.Find(question.name(), question.dns_type(), question.dns_class());
  for (const MdnsRecordTracker& tracker : trackers) {
    // NOTE: When the pointed to object is deleted, its dtor removes itself
    // from all associated records.
    ptr->AddAssociatedRecord(&tracker);
  }
}

void MdnsQuerier::AddRecord(const MdnsRecord& record, DnsType type) {
  // Add the new record.
  const auto& tracker = records_.StartTracking(record, type);

  // Let all questions associated with this record know that there is a new
  // record that answers them (for known answer suppression).
  auto query_it = questions_.equal_range(record.name());
  for (auto entry = query_it.first; entry != query_it.second; ++entry) {
    const MdnsQuestion& query = entry->second->question();
    const bool is_relevant_type =
        type == DnsType::kANY || type == query.dns_type();
    const bool is_relevant_class = record.dns_class() == DnsClass::kANY ||
                                   record.dns_class() == query.dns_class();
    if (is_relevant_type && is_relevant_class) {
      // NOTE: When the pointed to object is deleted, its dtor removes itself
      // from all associated queries.
      entry->second->AddAssociatedRecord(&tracker);
    }
  }
}

void MdnsQuerier::ApplyPendingChanges(
    std::vector<PendingQueryChange> pending_changes) {
  for (auto& pending_change : pending_changes) {
    switch (pending_change.change_type) {
      case PendingQueryChange::kStartQuery:
        StartQuery(std::move(pending_change.name), pending_change.dns_type,
                   pending_change.dns_class, pending_change.callback);
        break;
      case PendingQueryChange::kStopQuery:
        StopQuery(std::move(pending_change.name), pending_change.dns_type,
                  pending_change.dns_class, pending_change.callback);
        break;
    }
  }
}

}  // namespace discovery
}  // namespace openscreen
