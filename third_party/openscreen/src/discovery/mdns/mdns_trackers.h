// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_TRACKERS_H_
#define DISCOVERY_MDNS_MDNS_TRACKERS_H_

#include <tuple>
#include <unordered_map>
#include <vector>

#include "absl/hash/hash.h"
#include "discovery/mdns/mdns_records.h"
#include "platform/api/task_runner.h"
#include "platform/base/error.h"
#include "platform/base/trivial_clock_traits.h"
#include "util/alarm.h"

namespace openscreen {
namespace discovery {

struct Config;
class MdnsRandom;
class MdnsRecord;
class MdnsRecordChangedCallback;
class MdnsSender;

// MdnsTracker is a base class for MdnsRecordTracker and MdnsQuestionTracker for
// the purposes of common code sharing only.
//
// Instances of this class represent nodes of a bidirectional graph, such that
// if node A is adjacent to node B, B is also adjacent to A. In this class, the
// adjacent nodes are stored in adjacency list |associated_tracker_|, and
// exposed methods to add and remove nodes from this list also modify the added
// or removed node to remove this instance from its adjacency list.
//
// Because MdnsQuestionTracker::AddAssocaitedRecord() can only called on
// MdnsRecordTracker objects and MdnsRecordTracker::AddAssociatedQuery() is
// only called on MdnsQuestionTracker objects, this created graph is bipartite.
// This means that MdnsRecordTracker objects are only adjacent to
// MdnsQuestionTracker objects and the opposite.
class MdnsTracker {
 public:
  enum class TrackerType { kRecordTracker, kQuestionTracker };

  // MdnsTracker does not own |sender|, |task_runner| and |random_delay|
  // and expects that the lifetime of these objects exceeds the lifetime of
  // MdnsTracker.
  MdnsTracker(MdnsSender* sender,
              TaskRunner* task_runner,
              ClockNowFunctionPtr now_function,
              MdnsRandom* random_delay,
              TrackerType tracker_type);
  MdnsTracker(const MdnsTracker& other) = delete;
  MdnsTracker(MdnsTracker&& other) noexcept = delete;
  MdnsTracker& operator=(const MdnsTracker& other) = delete;
  MdnsTracker& operator=(MdnsTracker&& other) noexcept = delete;
  virtual ~MdnsTracker();

  // Returns the record type represented by this tracker.
  TrackerType tracker_type() const { return tracker_type_; }

  // Sends a query message via MdnsSender. Returns false if a follow up query
  // should NOT be scheduled and true otherwise.
  virtual bool SendQuery() const = 0;

  // Returns the records currently associated with this tracker.
  virtual std::vector<MdnsRecord> GetRecords() const = 0;

 protected:
  // Schedules a repeat query to be sent out.
  virtual void ScheduleFollowUpQuery() = 0;

  // These methods create a bidirectional adjacency with another node in the
  // graph.
  bool AddAdjacentNode(const MdnsTracker* tracker) const;
  bool RemoveAdjacentNode(const MdnsTracker* tracker) const;

  const std::vector<const MdnsTracker*>& adjacent_nodes() const {
    return adjacent_nodes_;
  }

  MdnsSender* const sender_;
  TaskRunner* const task_runner_;
  const ClockNowFunctionPtr now_function_;
  Alarm send_alarm_;
  MdnsRandom* const random_delay_;
  TrackerType tracker_type_;

 private:
  // These methods are used to ensure the bidirectional-ness of this graph.
  void AddReverseAdjacency(const MdnsTracker* tracker) const;
  void RemovedReverseAdjacency(const MdnsTracker* tracker) const;

  // Adjacency list for this graph node.
  mutable std::vector<const MdnsTracker*> adjacent_nodes_;
};

class MdnsQuestionTracker;

// MdnsRecordTracker manages automatic resending of mDNS queries for
// refreshing records as they reach their expiration time.
class MdnsRecordTracker : public MdnsTracker {
 public:
  using RecordExpiredCallback =
      std::function<void(const MdnsRecordTracker*, const MdnsRecord&)>;

  // NOTE: In the case that |record| is of type NSEC, |dns_type| is expected to
  // differ from |record|'s type.
  MdnsRecordTracker(MdnsRecord record,
                    DnsType dns_type,
                    MdnsSender* sender,
                    TaskRunner* task_runner,
                    ClockNowFunctionPtr now_function,
                    MdnsRandom* random_delay,
                    RecordExpiredCallback record_expired_callback);

  ~MdnsRecordTracker() override;

  // Possible outcomes from updating a tracked record.
  enum class UpdateType {
    kGoodbye,  // The record has a TTL of 0 and will expire.
    kTTLOnly,  // The record updated its TTL only.
    kRdata     // The record updated its RDATA.
  };

  // Updates record tracker with the new record:
  // 1. Resets TTL to the value specified in |new_record|.
  // 2. Schedules expiration in case of a goodbye record.
  // Returns Error::Code::kParameterInvalid if new_record is not a valid update
  // for the current tracked record.
  ErrorOr<UpdateType> Update(const MdnsRecord& new_record);

  // Adds or removed a question which this record answers.
  bool AddAssociatedQuery(const MdnsQuestionTracker* question_tracker) const;
  bool RemoveAssociatedQuery(const MdnsQuestionTracker* question_tracker) const;

  // Sets record to expire after 1 seconds as per RFC 6762
  void ExpireSoon();

  // Expires the record now
  void ExpireNow();

  // Returns true if half of the record's TTL has passed, and false otherwise.
  // Half is used due to specifications in RFC 6762 section 7.1.
  bool IsNearingExpiry() const;

  // Returns information about the stored record.
  //
  // NOTE: These methods are NOT all pass-through methods to |record_|.
  // specifically, dns_type() returns the DNS Type associated with this record
  // tracker, which may be different from the record type if |record_| is of
  // type NSEC. To avoid this case, direct access to the underlying |record_|
  // instance is not provided.
  //
  // In this case, creating an MdnsRecord with the below data will result in a
  // runtime error due to DCHECKS and that Rdata's associated type will not
  // match DnsType when |record_| is of type NSEC. Therefore, creating such
  // records should be guarded by is_negative_response() checks.
  const DomainName& name() const { return record_.name(); }
  DnsType dns_type() const { return dns_type_; }
  DnsClass dns_class() const { return record_.dns_class(); }
  RecordType record_type() const { return record_.record_type(); }
  std::chrono::seconds ttl() const { return record_.ttl(); }
  const Rdata& rdata() const { return record_.rdata(); }

  bool is_negative_response() const {
    return record_.dns_type() == DnsType::kNSEC;
  }

 private:
  using MdnsTracker::tracker_type;

  // Needed to provide the test class access to the record stored in this
  // tracker.
  friend class MdnsTrackerTest;

  Clock::time_point GetNextSendTime();

  // MdnsTracker overrides.
  bool SendQuery() const override;
  void ScheduleFollowUpQuery() override;
  std::vector<MdnsRecord> GetRecords() const override;

  // Stores MdnsRecord provided to Start method call.
  MdnsRecord record_;

  // DnsType this record tracker represents. This may not match the type of
  // |record_| if it is an NSEC record.
  const DnsType dns_type_;

  // A point in time when the record was received and the tracking has started.
  Clock::time_point start_time_;

  // Number of times record refresh has been attempted.
  size_t attempt_count_ = 0;
  RecordExpiredCallback record_expired_callback_;
};

// MdnsQuestionTracker manages automatic resending of mDNS queries for
// continuous monitoring with exponential back-off as described in RFC 6762.
class MdnsQuestionTracker : public MdnsTracker {
 public:
  // Supported query types, per RFC 6762 section 5.
  enum class QueryType { kOneShot, kContinuous };

  MdnsQuestionTracker(MdnsQuestion question,
                      MdnsSender* sender,
                      TaskRunner* task_runner,
                      ClockNowFunctionPtr now_function,
                      MdnsRandom* random_delay,
                      const Config& config,
                      QueryType query_type = QueryType::kContinuous);

  ~MdnsQuestionTracker() override;

  // Adds or removed an answer to a the question posed by this tracker.
  bool AddAssociatedRecord(const MdnsRecordTracker* record_tracker) const;
  bool RemoveAssociatedRecord(const MdnsRecordTracker* record_tracker) const;

  // Returns a reference to the tracked question.
  const MdnsQuestion& question() const { return question_; }

 private:
  using MdnsTracker::tracker_type;

  using RecordKey = std::tuple<DomainName, DnsType, DnsClass>;

  // Determines if all answers to this query have been received.
  bool HasReceivedAllResponses();

  // MdnsTracker overrides.
  bool SendQuery() const override;
  void ScheduleFollowUpQuery() override;
  std::vector<MdnsRecord> GetRecords() const override;

  // Stores MdnsQuestion provided to Start method call.
  MdnsQuestion question_;

  // A delay between the currently scheduled and the next queries.
  Clock::duration send_delay_;

  // Last time that this tracker's question was asked.
  mutable TrivialClockTraits::time_point last_send_time_;

  // Specifies whether this query is intended to be a one-shot query, as defined
  // in RFC 6762 section 5.1.
  const QueryType query_type_;

  // Signifies the maximum number of times a record should be announced.
  int maximum_announcement_count_;

  // Number of times this query has been announced.
  int announcements_so_far_ = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_TRACKERS_H_
