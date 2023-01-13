// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_MDNS_RESPONDER_H_
#define DISCOVERY_MDNS_MDNS_RESPONDER_H_

#include <map>
#include <memory>
#include <vector>

#include "discovery/mdns/mdns_records.h"
#include "platform/api/time.h"
#include "platform/base/macros.h"
#include "util/alarm.h"

namespace openscreen {

struct IPEndpoint;
class TaskRunner;

namespace discovery {

struct Config;
class MdnsMessage;
class MdnsProbeManager;
class MdnsRandom;
class MdnsReceiver;
class MdnsRecordChangedCallback;
class MdnsSender;
class MdnsQuerier;

// This class is responsible for responding to any incoming mDNS Queries
// received via the OnMessageReceived() method. When responding, the generated
// MdnsMessage will contain the requested record(s) in the answers section, or
// an NSEC record to specify that the requested record was not found in the case
// of a query with DnsType aside from ANY. In the case where records are found,
// the additional records field may be populated with additional records, as
// specified in RFCs 6762 and 6763.
class MdnsResponder {
 public:
  // Class to handle querying for existing records.
  class RecordHandler {
   public:
    virtual ~RecordHandler();

    // Returns whether this service has one or more records matching the
    // provided name, type, and class.
    virtual bool HasRecords(const DomainName& name,
                            DnsType type,
                            DnsClass clazz) = 0;

    // Returns all records owned by this service with name, type, and class
    // matching the provided values.
    virtual std::vector<MdnsRecord::ConstRef> GetRecords(const DomainName& name,
                                                         DnsType type,
                                                         DnsClass clazz) = 0;

    // Enumerates all PTR records owned by this service.
    virtual std::vector<MdnsRecord::ConstRef> GetPtrRecords(DnsClass clazz) = 0;
  };

  // |record_handler|, |sender|, |receiver|, |task_runner|, |random_delay|, and
  // |config| are expected to persist for the duration of this instance's
  // lifetime.
  MdnsResponder(RecordHandler* record_handler,
                MdnsProbeManager* ownership_handler,
                MdnsSender* sender,
                MdnsReceiver* receiver,
                TaskRunner* task_runner,
                ClockNowFunctionPtr now_function,
                MdnsRandom* random_delay,
                const Config& config);
  ~MdnsResponder();

  OSP_DISALLOW_COPY_AND_ASSIGN(MdnsResponder);

 private:
  // Class which handles processing and responding to queries segmented into
  // multiple messages.
  class TruncatedQuery {
   public:
    // |responder| and |task_runner| are expected to persist for the duration of
    // this instance's lifetime.
    TruncatedQuery(MdnsResponder* responder,
                   TaskRunner* task_runner,
                   ClockNowFunctionPtr now_function,
                   IPEndpoint src,
                   const MdnsMessage& message,
                   const Config& config);
    TruncatedQuery(const TruncatedQuery& other) = delete;
    TruncatedQuery(TruncatedQuery&& other) = delete;

    TruncatedQuery& operator=(const TruncatedQuery& other) = delete;
    TruncatedQuery& operator=(TruncatedQuery&& other) = delete;

    // Sets the query associated with this instance. Must only be called if no
    // query has already been set, here or through the ctor.
    void SetQuery(const MdnsMessage& message);

    // Adds additional known answers.
    void AddKnownAnswers(const std::vector<MdnsRecord>& records);

    // Responds to the stored queries.
    void SendResponse();

    const IPEndpoint& src() const { return src_; }
    const std::vector<MdnsQuestion>& questions() const { return questions_; }
    const std::vector<MdnsRecord>& known_answers() const {
      return known_answers_;
    }

   private:
    void RescheduleSend();

    // The number of messages received so far associated with this known answer
    // query.
    int messages_received_so_far = 0;

    const int max_allowed_messages_;
    const int max_allowed_records_;
    const IPEndpoint src_;
    MdnsResponder* const responder_;

    std::vector<MdnsQuestion> questions_;
    std::vector<MdnsRecord> known_answers_;
    Alarm alarm_;
  };

  // Called when a new MdnsMessage is received.
  void OnMessageReceived(const MdnsMessage& message, const IPEndpoint& src);

  // Responds a truncated query for which all known answers have been received.
  void RespondToTruncatedQuery(TruncatedQuery* query);

  // Processes a message associated with a multi-packet truncated query.
  void ProcessMultiPacketTruncatedMessage(const MdnsMessage& message,
                                          const IPEndpoint& src);

  // Processes queries provided.
  void ProcessQueries(const IPEndpoint& src,
                      const std::vector<MdnsQuestion>& questions,
                      const std::vector<MdnsRecord>& known_answers);

  // Sends the response to the provided query.
  void SendResponse(const MdnsQuestion& question,
                    const std::vector<MdnsRecord>& known_answers,
                    std::function<void(const MdnsMessage&)> send_response,
                    bool is_exclusive_owner);

  // Set of all truncated queries received so far. Per RFC 6762 section 7.1,
  // matching of a query with additional known answers should be done based on
  // the source address.
  // NOTE: unique_ptrs used because TruncatedQuery is not movable.
  std::map<IPEndpoint, std::unique_ptr<TruncatedQuery>> truncated_queries_;

  RecordHandler* const record_handler_;
  MdnsProbeManager* const ownership_handler_;
  MdnsSender* const sender_;
  MdnsReceiver* const receiver_;
  TaskRunner* const task_runner_;
  const ClockNowFunctionPtr now_function_;
  MdnsRandom* const random_delay_;
  const Config& config_;

  friend class MdnsResponderTest;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_MDNS_RESPONDER_H_
