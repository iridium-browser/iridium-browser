// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_MDNS_PUBLIC_MDNS_SERVICE_H_
#define DISCOVERY_MDNS_PUBLIC_MDNS_SERVICE_H_

#include <functional>
#include <memory>

#include "discovery/common/config.h"
#include "discovery/mdns/public/mdns_constants.h"
#include "platform/base/error.h"
#include "platform/base/interface_info.h"
#include "platform/base/ip_address.h"

namespace openscreen {

class TaskRunner;

namespace discovery {

class DomainName;
class MdnsDomainConfirmedProvider;
class MdnsRecord;
class MdnsRecordChangedCallback;
class ReportingClient;

class MdnsService {
 public:
  MdnsService();
  virtual ~MdnsService();

  // Creates a new MdnsService instance, to be owned by the caller. On failure,
  // returns nullptr. |task_runner|, |reporting_client|, and |config| must exist
  // for the duration of the resulting instance's life.
  static std::unique_ptr<MdnsService> Create(TaskRunner* task_runner,
                                             ReportingClient* reporting_client,
                                             const Config& config,
                                             const InterfaceInfo& network_info);

  // Starts an mDNS query with the given properties. Updated records are passed
  // to |callback|.  The caller must ensure |callback| remains alive while it is
  // registered with a query.
  virtual void StartQuery(const DomainName& name,
                          DnsType dns_type,
                          DnsClass dns_class,
                          MdnsRecordChangedCallback* callback) = 0;

  // Stops an mDNS query with the given properties. |callback| must be the same
  // callback pointer that was previously passed to StartQuery.
  virtual void StopQuery(const DomainName& name,
                         DnsType dns_type,
                         DnsClass dns_class,
                         MdnsRecordChangedCallback* callback) = 0;

  // Re-initializes the process of service discovery for the provided domain
  // name. All ongoing queries for this domain are restarted and any previously
  // received query results are discarded.
  virtual void ReinitializeQueries(const DomainName& name) = 0;

  // Starts probing for a valid domain name based on the given one. |callback|
  // will be called once a valid domain is found, and the instance must persist
  // until that call is received.
  virtual Error StartProbe(MdnsDomainConfirmedProvider* callback,
                           DomainName requested_name,
                           IPAddress address) = 0;

  // Registers a new mDNS record for advertisement by this service. For A, AAAA,
  // SRV, and TXT records, the domain name must have already been claimed by the
  // ClaimExclusiveOwnership() method and for PTR records the name being pointed
  // to must have been claimed in the same fashion, but the domain name in the
  // top-level MdnsRecord entity does not.
  virtual Error RegisterRecord(const MdnsRecord& record) = 0;

  // Updates the existing record with name matching the name of the new record.
  // NOTE: This method is not valid for PTR records.
  virtual Error UpdateRegisteredRecord(const MdnsRecord& old_record,
                                       const MdnsRecord& new_record) = 0;

  // Stops advertising the provided record. If no more records with the provided
  // name are bing advertised after this call's completion, then ownership of
  // the name is released.
  virtual Error UnregisterRecord(const MdnsRecord& record) = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_MDNS_PUBLIC_MDNS_SERVICE_H_
