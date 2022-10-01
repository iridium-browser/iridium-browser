// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DISCOVERY_DNSSD_IMPL_DNS_DATA_GRAPH_H_
#define DISCOVERY_DNSSD_IMPL_DNS_DATA_GRAPH_H_

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "absl/types/optional.h"
#include "discovery/dnssd/impl/constants.h"
#include "discovery/dnssd/public/dns_sd_instance_endpoint.h"
#include "discovery/mdns/mdns_record_changed_callback.h"
#include "discovery/mdns/mdns_records.h"

namespace openscreen {
namespace discovery {

/*
 Per RFC 6763, the following mappings exist between the domains of the called
 out mDNS records:

                     --------------
                    | PTR Record |
                    --------------
                          /\
                         /  \
                        /    \
                       /      \
                      /        \
            -------------- --------------
            | SRV Record | | TXT Record |
            -------------- --------------
                  /\
                 /  \
                /    \
               /      \
              /        \
             /          \
    -------------- ---------------
    |  A Record  | | AAAA Record |
    -------------- ---------------

 Such that PTR records point to the domain of SRV and TXT records, and SRV
 records point to the domain of A and AAAA records. Below, these 3 separate
 sets are referred to as "Domain Groups".

 Though it is frequently the case that each A or AAAA record will only be
 pointed to by one SRV record domain, this is not a requirement for DNS-SD and
 in the wild this case does come up. On the other hand, it is expected that
 each PTR record domain will point to multiple SRV records.

 To represent this data, a multigraph structure has been used.
 - Each node of the graph represents a specific domain name
 - Each edge represents a parent-child relationship, such that node A is a
   parent of node B iff there exists some record x in A such that x points to
   the domain represented by B.
 In practice, it is expected that no more than one edge will ever exist
 between two nodes. A multigraph is used despite this to simplify the code and
 avoid a number of tricky edge cases (both literally and figuratively).

 Note the following:
 - This definition allows for cycles in the multigraph (which are unexpected
   but allowed by the RFC).
 - This definition allows for self loops (which are expected when a SRV record
   points to address records with the same domain).
 - The memory requirement for this graph is bounded due to a bound on the
   number of tracked records in the mDNS layer as part of
   discovery/mdns/mdns_querier.h.
*/
class DnsDataGraph {
 public:
  // The set of valid groups of domains, as called out in the hierarchy
  // described above.
  enum class DomainGroup { kNone = 0, kPtr, kSrvAndTxt, kAddress };

  // Get the domain group associated with the provided object.
  static DomainGroup GetDomainGroup(DnsType type);
  static DomainGroup GetDomainGroup(const MdnsRecord record);

  // Creates a new DnsDataGraph.
  static std::unique_ptr<DnsDataGraph> Create(
      NetworkInterfaceIndex network_index);

  // Callback to use when a domain change occurs.
  using DomainChangeCallback = std::function<void(DomainName)>;

  virtual ~DnsDataGraph();

  // Manually starts or stops tracking the provided domain. These methods should
  // only be called for top-level PTR domains.
  virtual void StartTracking(const DomainName& domain,
                             DomainChangeCallback on_start_tracking) = 0;
  virtual void StopTracking(const DomainName& domain,
                            DomainChangeCallback on_stop_tracking) = 0;

  // Attempts to create all DnsSdInstanceEndpoint objects with |name| associated
  // with the provided |domain_group|. If all required data for one such
  // endpoint has been received, and an error occurs while parsing this data,
  // then an error is returned in place of that endpoint.
  virtual std::vector<ErrorOr<DnsSdInstanceEndpoint>> CreateEndpoints(
      DomainGroup domain_group,
      const DomainName& name) const = 0;

  // Modifies this entity with the provided DnsRecord. If called with a valid
  // record type, the provided change will only be applied if the provided event
  // is valid at the time of calling. The returned result will be an error if
  // the change does not make sense from our current data state, and
  // Error::None() otherwise. Valid record types with which this method can be
  // called are PTR, SRV, TXT, A, and AAAA record types.
  //
  // TODO(issuetracker.google.com/157822423): Allow for duplicate records of
  // non-PTR types.
  virtual Error ApplyDataRecordChange(
      MdnsRecord record,
      RecordChangedEvent event,
      DomainChangeCallback on_start_tracking,
      DomainChangeCallback on_stop_tracking) = 0;

  virtual size_t GetTrackedDomainCount() const = 0;

  // Returns whether the provided domain is tracked or not. This may either be
  // due to a direct call to StartTracking() or due to the result of a received
  // record.
  virtual bool IsTracked(const DomainName& name) const = 0;
};

}  // namespace discovery
}  // namespace openscreen

#endif  // DISCOVERY_DNSSD_IMPL_DNS_DATA_GRAPH_H_
