// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_PUBLISHER_H_
#define DISCOVERY_MDNS_MDNS_PUBLISHER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "discovery/mdns/mdns_records.h"
#include "discovery/mdns/mdns_responder.h"
#include "util/alarm.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

struct Config;
class MdnsProbeManager;
class MdnsRandom;
class MdnsSender;
class MdnsQuerier;

// This class is responsible for both tracking what records have been registered
// to mDNS as well as publishing new mDNS records to the network.
// When a new record is published, it will be announced 8 times, starting at an
// interval of 1 second, with the interval doubling each successive
// announcement. This same announcement process is followed when an existing
// record is updated. When it is removed, a Goodbye message must be sent if the
// record is unique.
//
// Prior to publishing a record, the domain name for this service instance must
// be claimed using the ClaimExclusiveOwnership() function. This function probes
// the network to determine whether the chosen name exists, modifying the
// chosen name as described in RFC 6762 if a collision is found.
//
// NOTE: All MdnsPublisher instances must be run on the same task runner thread,
// due to the shared announce + goodbye message queue.
class MdnsPublisher : public MdnsResponder::RecordHandler {
 public:
  // |sender|, |ownership_manager|, and  |task_runner| must all persist for the
  // duration of this object's lifetime
  MdnsPublisher(MdnsSender* sender,
                MdnsProbeManager* ownership_manager,
                TaskRunner* task_runner,
                ClockNowFunctionPtr now_function,
                const Config& config);
  ~MdnsPublisher() override;

  // Registers a new mDNS record for advertisement by this service. For A, AAAA,
  // SRV, and TXT records, the domain name must have already been claimed by the
  // ClaimExclusiveOwnership() method and for PTR records the name being pointed
  // to must have been claimed in the same fashion, but the domain name in the
  // top-level MdnsRecord entity does not.
  // NOTE: This call is only valid for |dns_type| values:
  // - DnsType::kA
  // - DnsType::kPTR
  // - DnsType::kTXT
  // - DnsType::kAAAA
  // - DnsType::kSRV
  // - DnsType::kANY
  Error RegisterRecord(const MdnsRecord& record);

  // Updates the existing record with name matching the name of the new record.
  // NOTE: This method is not valid for PTR records.
  Error UpdateRegisteredRecord(const MdnsRecord& old_record,
                               const MdnsRecord& new_record);

  // Stops advertising the provided record.
  Error UnregisterRecord(const MdnsRecord& record);

  // Returns the total number of records currently registered;
  size_t GetRecordCount() const;

  OSP_DISALLOW_COPY_AND_ASSIGN(MdnsPublisher);

 private:
  // Class responsible for sending announcement and goodbye messages for
  // MdnsRecord instances when they are published, updated, or unpublished. The
  // announcement messages will be sent |target_announcement_attempts| times,
  // first at an interval of 1 second apart, and then with delay increasing by a
  // factor of 2 with each successive announcement.
  // NOTE: |publisher| must be the MdnsPublisher instance from which this
  // instance was created.
  class RecordAnnouncer {
   public:
    RecordAnnouncer(MdnsRecord record,
                    MdnsPublisher* publisher,
                    TaskRunner* task_runner,
                    ClockNowFunctionPtr now_function,
                    int max_announcement_attempts);
    RecordAnnouncer(const RecordAnnouncer& other) = delete;
    RecordAnnouncer(RecordAnnouncer&& other) noexcept = delete;
    ~RecordAnnouncer();

    RecordAnnouncer& operator=(const RecordAnnouncer& other) = delete;
    RecordAnnouncer& operator=(RecordAnnouncer&& other) noexcept = delete;

    const MdnsRecord& record() const { return record_; }

    // Specifies whether goodbye messages should not be sent when this announcer
    // is destroyed. This should only be called as part of the 'Update' flow,
    // for records which should not send this message.
    void DisableGoodbyeMessageTransmission() {
      should_send_goodbye_message_ = false;
    }

   private:
    // Gets the delay required before the next announcement message is sent.
    Clock::duration GetNextAnnounceDelay();

    // When announce + goodbye messages are ready to be sent, they are queued
    // up. Every 20ms, if there are any messages to send out, these records are
    // batched up and sent out.
    void QueueGoodbye();
    void QueueAnnouncement();

    MdnsPublisher* const publisher_;
    TaskRunner* const task_runner_;
    const ClockNowFunctionPtr now_function_;

    // Whether or not goodbye messages should be sent.
    bool should_send_goodbye_message_ = true;

    // Record to send.
    const MdnsRecord record_;

    // Alarm used to cancel future resend attempts if this object is deleted.
    Alarm alarm_;

    // Number of attempts at sending this record which have occurred so far.
    int attempts_ = 0;

    // Number of times to announce a newly published record.
    const int target_announcement_attempts_;
  };

  using RecordAnnouncerPtr = std::unique_ptr<RecordAnnouncer>;

  friend class MdnsPublisherTesting;

  // Creates a new published from the provided record.
  RecordAnnouncerPtr CreateAnnouncer(MdnsRecord record) {
    return std::make_unique<RecordAnnouncer>(std::move(record), this,
                                             task_runner_, now_function_,
                                             max_announcement_attempts_);
  }

  // Removes the given record from the |records_| map. A goodbye record is only
  // sent for this removal if |should_announce_deletion| is true.
  Error RemoveRecord(const MdnsRecord& record, bool should_announce_deletion);

  // Returns whether the provided record has had its name claimed so far.
  bool IsRecordNameClaimed(const MdnsRecord& record) const;

  // Processes the |records_to_send_| queue, sending out the records together as
  // a single MdnsMessage.
  void ProcessRecordQueue();

  // Adds a new record to the |records_to_send_| queue or ensures that the
  // record with lower ttl is present if it differs from an existing record by
  // only that one field.
  void QueueRecord(MdnsRecord record);

  // MdnsResponder::RecordHandler overrides.
  bool HasRecords(const DomainName& name,
                  DnsType type,
                  DnsClass clazz) override;
  std::vector<MdnsRecord::ConstRef> GetRecords(const DomainName& name,
                                               DnsType type,
                                               DnsClass clazz) override;
  std::vector<MdnsRecord::ConstRef> GetPtrRecords(DnsClass clazz) override;

  MdnsSender* const sender_;
  MdnsProbeManager* const ownership_manager_;
  TaskRunner* const task_runner_;
  ClockNowFunctionPtr now_function_;

  // Alarm to cancel batching of records when this class is destroyed, and
  // instead send them immediately. Variable is only set when it is in use.
  absl::optional<Alarm> batch_records_alarm_;

  // Number of times to announce a newly published record.
  const int max_announcement_attempts_;

  // The queue for announce and goodbye records to be sent periodically.
  std::vector<MdnsRecord> records_to_send_;

  // Stores mDNS records that have been published. The keys here are domain
  // names for valid mDNS Records, and the values are the RecordAnnouncer
  // entities associated with all published MdnsRecords for the keyed domain.
  // These are responsible for publishing a specific MdnsRecord, announcing it
  // when its created and sending a goodbye record when it's deleted.
  std::map<DomainName, std::vector<RecordAnnouncerPtr>> records_;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_PUBLISHER_H_
